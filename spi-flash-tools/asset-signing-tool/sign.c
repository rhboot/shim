/*
 * Copyright (c) 2013 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Signing tool used to generate a Quark secure boot header or file
 */

#define _GNU_SOURCE /* This is required by the asprintf function */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#define RSA_KEY_SIZE                256
#define RSA_MODULUS_SIZE            256
#define RSA_PRIVATE_EXPONENT_SIZE   256
#define RSA_PUBLIC_EXPONENT_SIZE    4
#define RSA_SIGNATURE_SIZE          256
#define ENCODED_MESSAGE_SIZE        256

struct clanton_security_header{
    uint32_t csh_identifier;                // Unique identifier
    uint32_t version;                       // Currently unused
    uint32_t module_size;                   // Size in bytes of module including CSH
    uint32_t security_version_number_index; // SVN Index
    uint32_t security_version_number;       // SVN to prevent roll back
    uint32_t reserved_module_id;            // Currently unused
    uint32_t reserved_module_vendor;        // Vendor ID, 0x00008086 for Intel
    uint32_t reserved_date;                 // BCD representation of build date
    uint32_t module_header_size;            // Total size in bytes of header
    uint32_t hash_algorithm;                // Hashing Algorithm used
    uint32_t crypto_algorithm;              // Signing Algorithm used
    uint32_t key_size;                      // Size in bytes of the key data
    uint32_t signature_size;                // Size in bytes of the signature
    uint32_t reserved_next_header_pointer;  // ???
    uint32_t reserved[2];                   // Reserved for future use
};

struct rsa_public_key{
    uint32_t modulus_size;                  // Size in bytes of modulus
    uint32_t exponent_size;                 // Size in bytes of the exponent
    uint8_t modulus[RSA_MODULUS_SIZE];
    uint32_t exponent;
};

struct signing_format{
    struct clanton_security_header csh;
    struct rsa_public_key verification_key;
    uint8_t signature[RSA_SIGNATURE_SIZE];
};

static uint32_t sizeof_signing_format = sizeof(struct signing_format);

static void usage(){
    printf("\nSign an asset for secure boot\n");
    printf("Usage: ./sign [OPTION]...\n");
    printf("\t-i <input file>\n");
    printf("\t-o <output file>\n");
    printf("\t-b <body offset>\n");
    printf("\t-s <svn>\n");
    printf("\t-x <svn index>\n");
    printf("\t-k <key file>\n");
    printf("\t-c\tcreate csbh file\n");
    printf("\t-l\tdisable output file size limit (128MB)\n");
    printf("\t-q\thide log messages\n");
}

long str_to_ul(char *arg){

    // The behaviour of strtoul on an empty input cannot be trusted
    if(strlen(arg) == 0){
        return ULONG_MAX;
    }

    char * trailer_ptr;
    errno = 0;
    const unsigned long value = strtoul(arg, &trailer_ptr, 0);

    if(errno){
        fprintf (stderr, "ERROR: %s\n", strerror(errno));
        return ULONG_MAX;
    }

    if(*trailer_ptr != '\0'){
        fprintf (stderr, "ERROR: Invalid trailing characters: %s\n", trailer_ptr);
        return ULONG_MAX;
    }

    return value;
}

int main(int argc, char *argv[]){

    /* Parse and validate the command line args */
    int c;
    char *infile_name = NULL;
    char *outfile_name = NULL;
    char *keyfile_name = NULL;
    uint32_t body_offset = sizeof_signing_format;
    uint32_t svn = 0, svn_index = 0;
    unsigned long body_offset_l = 0, svn_l = 0, svn_index_l = 0;
    bool csbh = false, svn_set = false, svni_set = false, limit = true, quiet = false;

    /* Check endianness of system */
    if(htonl(1) == 1){ 
        fprintf(stderr, "ERROR: Big-endian systems not supported\n");
        return EXIT_FAILURE;
    }

    while ((c = getopt(argc, argv, "i:o:b:s:x:k:clq")) != -1){
        switch(c){
            case 'i':
                infile_name = optarg;
                break;
            case 'o':
                outfile_name = optarg;
                break;
            case 'b':
                body_offset_l = str_to_ul(optarg);
                if(body_offset_l == ULONG_MAX){
                    fprintf (stderr, "ERROR: Body offset is not valid\n");
                    goto err;
                }
                body_offset = body_offset_l;
                break;
            case 's':
                svn_l = str_to_ul(optarg);
                if(svn_l == ULONG_MAX){
                    fprintf (stderr, "ERROR: SVN not valid\n");
                    goto err;
                }
                if(svn_l >= UINT32_MAX){
                    fprintf (stderr, "ERROR: SVN out of range\n");
                    goto err;
                }
                svn = svn_l;
                svn_set = true;
                break;
            case 'x':
                svn_index_l = str_to_ul(optarg);
                if(svn_index_l == ULONG_MAX){
                    fprintf (stderr, "ERROR: SVN index not valid\n");
                    goto err;
                }
                if(svn_index_l > 15) {
                    fprintf (stderr, "ERROR: SVN index out of range (0-15)\n");
                    goto err;
                }
                svn_index = svn_index_l;
                svni_set = true;
                break;
            case 'k':
                keyfile_name = optarg;
                break;
            case 'c':
                csbh = true;
                break;
            case 'l':
                limit = false;
                break;
            case 'q':
                quiet = true;
                break;
            case '?':
                usage();
                goto err;
        }
    }

    if(optind != argc){
        int q;
        for (q = optind; q < argc; q++){
            fprintf(stderr, "ERROR: Unrecognised argument: %s\n", argv[q]);
        }
        goto err;
    }

    if(infile_name == NULL){
        fprintf(stderr, "ERROR: Input file is a required argument\n");
        usage();
        goto err;
    }

    char *gen_outfile_name = NULL;
    if(outfile_name == NULL){
        /* asprintf is the only non-standard function in this program */
        if(-1 == asprintf(&gen_outfile_name, "%s.%s", infile_name, csbh ? "csbh" : "signed")){
            fprintf(stderr, "ERROR: Failed to generate output file name\n");
            gen_outfile_name = NULL;
            goto err_outfile_free;
        }else{
            outfile_name = gen_outfile_name;
            if(!quiet)
                printf("INFO: No output file provided, using: %s\n", outfile_name);
        }
    }

    if(body_offset < sizeof_signing_format){
        fprintf(stderr, "ERROR: Body offset out of range (%#x - %#x)\n", sizeof_signing_format, UINT32_MAX);
        goto err_outfile_free;
    }

    if(!svn_set || !svni_set){
        fprintf(stderr, "ERROR: SVN and SVN Index are required\n");
        usage();
        goto err_outfile_free;
    }

    if(keyfile_name == NULL){
        fprintf(stderr, "ERROR: Key file is a required argument\n");
        usage();
        goto err_outfile_free;
    }

    /* Get the size of the input file and read it in */
    struct stat sb;
    size_t filesize;
    size_t totalsize;
    if(stat(infile_name, &sb) != 0){
        fprintf (stderr, "ERROR: Could not stat %s\n", infile_name);
        goto err_outfile_free;
    }
    filesize = sb.st_size;
    totalsize = filesize + body_offset;

    if(totalsize > 128*1024*1024 && limit){
        fprintf (stderr, "ERROR: Output file will be > 128MB\n");
        fprintf (stderr, "The -l option disables this check (requires %.2fMB of memory)\n",
                (totalsize/1024.0)/1024);
        goto err_outfile_free;
    }

    FILE *infile;
    size_t bytes_read;
    uint8_t *data;
    infile = fopen(infile_name, "r");
    if(! infile){
        fprintf (stderr, "ERROR: Could not open %s\n", infile_name);
        goto err_outfile_free;
    }
    data = malloc(filesize);
    if(data == NULL){
        fprintf (stderr, "ERROR: Could not allocate memory\n");
        fclose(infile);
        goto err_outfile_free;
    }
    bytes_read = fread(data, sizeof(uint8_t), filesize, infile);
    if(bytes_read != filesize){
        fprintf (stderr, "ERROR: File read error\n");
        fclose(infile);
        goto err_data_free;
    }
    fclose(infile);

    /* Set up and populate header structure */
    struct signing_format header;
    memset(&header, 0x0, sizeof(header));
    header.csh.csh_identifier = 0x5F435348; // HSC_
    header.csh.version = 0x00000001;
    header.csh.module_size = (filesize + body_offset);
    header.csh.security_version_number_index = svn_index;
    header.csh.security_version_number = svn;
    header.csh.reserved_module_id = 0x00000000; //Currently unused
    header.csh.reserved_module_vendor = 0x00008086;
    header.csh.reserved_date = 0x00000000; //TODO populate this with BCD date
    header.csh.module_header_size = body_offset;
    header.csh.hash_algorithm = 0x00000001;
    header.csh.crypto_algorithm = 0x00000001;
    header.csh.key_size = sizeof(struct rsa_public_key);
    header.csh.signature_size = RSA_SIGNATURE_SIZE;

    /* Crypto stuff */
    FILE *keyfile = NULL;
    keyfile = fopen(keyfile_name, "r");
    if(!keyfile){
        fprintf (stderr, "ERROR: Could not open key file %s\n", keyfile_name);
        goto err_data_free;
    }

    RSA * const signature_key = PEM_read_RSAPrivateKey(keyfile, NULL, 0, NULL);
    if(!signature_key){
        fprintf (stderr, "ERROR: Could not read key file %s\n", keyfile_name);
        fclose(keyfile);
        goto err_data_free;
    }
    fclose(keyfile);

    if(RSA_check_key(signature_key) != 1){
        fprintf (stderr, "ERROR: RSA key check failed, invalid key\n");
        goto err_rsa_free;
    }

    uint8_t public_key_modulus[RSA_MODULUS_SIZE] = {0};
    uint8_t public_key_exponent[RSA_PUBLIC_EXPONENT_SIZE] = {0};

    const int exponent_length = BN_num_bytes(signature_key->e);
    if(exponent_length > RSA_PUBLIC_EXPONENT_SIZE){
        fprintf (stderr, "ERROR: RSA key exponent too long (%d> 4 Bytes)\n",
               	 exponent_length);
        goto err_rsa_free;
    }

    BN_bn2bin(signature_key->n, public_key_modulus);
    BN_bn2bin(signature_key->e,
              & public_key_exponent[RSA_PUBLIC_EXPONENT_SIZE - exponent_length]);
    if(public_key_modulus == NULL || public_key_exponent == NULL){
        fprintf (stderr, "ERROR: Could not convert BIGNUM for modulus or exponent\n");
        goto err_rsa_free;
    }

    /* Create hash of data */
    SHA256_CTX sha256;
    if(!SHA256_Init(&sha256)){
        fprintf (stderr, "ERROR: Failed to initialise SHA256\n");
        goto err_rsa_free;
    }
    if(!SHA256_Update(&sha256, &header.csh, sizeof(header.csh))){
        fprintf (stderr, "ERROR: Failed to update SHA256\n");
        goto err_rsa_free;
    }
    if(!SHA256_Update(&sha256, data, filesize)){
        fprintf (stderr, "ERROR: Failed to update SHA256\n");
        goto err_rsa_free;
    }

    uint8_t hash[SHA256_DIGEST_LENGTH];
    if(!SHA256_Final(hash, &sha256)){
        fprintf (stderr, "ERROR: Failed to finalise SHA256\n");
        goto err_rsa_free;
    }

    /* Create the RSA signature */
    uint8_t EM[ENCODED_MESSAGE_SIZE];

    RSA_padding_add_PKCS1_PSS(signature_key, EM, hash, EVP_sha256(), -2); // -2 is the salt length

    uint8_t signature[RSA_SIGNATURE_SIZE];
    RSA_private_encrypt(ENCODED_MESSAGE_SIZE, EM, signature, signature_key, RSA_NO_PADDING);

    header.verification_key.modulus_size = RSA_MODULUS_SIZE;
    header.verification_key.exponent_size = RSA_PUBLIC_EXPONENT_SIZE;
    memcpy(header.verification_key.modulus, public_key_modulus, sizeof(public_key_modulus));
    memcpy(&header.verification_key.exponent, public_key_exponent, sizeof(public_key_exponent));
    memcpy(header.signature, signature, RSA_SIGNATURE_SIZE);

    RSA_free(signature_key);

    /* Write data to output file */
    FILE * const outfile = fopen(outfile_name, "wb");
    if(outfile == NULL){
        fprintf (stderr, "ERROR: Failed to open output file %s\n", outfile_name);
        goto err_data_free;
    }

    fwrite(&header, sizeof(uint8_t), sizeof(header), outfile);

    if(body_offset > sizeof_signing_format){
        uint8_t * const padding =
          (uint8_t*)calloc((body_offset - sizeof_signing_format), sizeof(uint8_t));
        if(padding == NULL){
            fprintf (stderr, "ERROR: Failed to allocate memory for padding\n");
            fclose(outfile);
            goto err_data_free;
        }
        fwrite(padding, sizeof(uint8_t), body_offset - sizeof_signing_format, outfile);
        free(padding);
    }

    if(!csbh)
        fwrite(data, sizeof(uint8_t), filesize, outfile);

    if(gen_outfile_name)
        free(gen_outfile_name);

    fclose(outfile);
    free(data);

    return EXIT_SUCCESS;


err_rsa_free:
    RSA_free(signature_key);
err_data_free:
    free(data);
err_outfile_free:
    if(gen_outfile_name)
        free(gen_outfile_name);
err:
    return EXIT_FAILURE;
}
