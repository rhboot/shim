#ifndef UTILS_H_
#define UTILS_H_

#include "Tpm20.h"
#include "hexdump.h"

#define MAX_ALG 10

#define swap_uint16(x) __builtin_bswap16(x)	/* supported both by GCC and clang */
#define swap_uint32(x) __builtin_bswap32(x)	/* supported both by GCC and clang */

typedef struct tdTPM_PCR_DIGEST {
    uint32_t        PCR;
    BYTE            DIGEST_SIZE;
    TPMI_ALG_HASH   hashAlg;
    BYTE            buffer[SHA512_DIGEST_SIZE];
} TPM_PCR_DIGEST;

/*
@Params:
    CHAR8 *hex_array            Pointer to Ascii Array
    uint32_t hex_array_len      Size of the Ascii Array or offset into the Array
    CHAR8 *char_array           Pointer to the returned Array, MUST BE ALOCATED BEFORE USED
@Return:
    return char_len             Size of returned Array, Just hex_array_len/2
@Description:
    This function takes in an Array of Characters in hex which size is divisable by 2. It then
    converts the Array of Characters into their Character Byte representation. This allows a
    perton to input Ascii strings and they will be converted into hex.
@Example:
    Before: Data = "9B7D"
    After:  Data = 0x9B 0x7D
*/
static uint32_t
__attribute__((__unused__))
hex_array_to_char_array(CHAR8 *hex_array, uint32_t hex_array_len, CHAR8 *char_array){
    if(hex_array_len % 2 != 0)
        return 0;

    uint32_t i = 0, j = 0;
    uint32_t char_len = hex_array_len / 2;
    
    while(j < char_len){
        char_array[j] = (hex_array[i] % 32 + 9) % 25 * 16 + (hex_array[i+1] % 32 + 9) % 25;
        i+=2; j++;
    }

    return char_len;
}


static uint32_t
__attribute__((__unused__))
buf_tpm_pcr_digest_sorter(TPM_PCR_DIGEST **PCR_Data, uint32_t PCR_Data_Size){
    TPM_PCR_DIGEST PCR_Data_Temp;
    uint32_t i = 0, j = 0;

    while(i < PCR_Data_Size){
        j = 0;
        while(j < PCR_Data_Size){
            if((*PCR_Data)[j].PCR < (*PCR_Data)[i].PCR){
                PCR_Data_Temp = (*PCR_Data)[i];
                (*PCR_Data)[i] = (*PCR_Data)[j];
                (*PCR_Data)[j] = PCR_Data_Temp;
            }
            j++;
        }
        i++;
    }

    return 0;
}

/*
@Params:
    CHAR8 *Buf                  Pointer to File Buffer
    UINTN BufSize               Size of the File Buffer Array
    TPM_PCR_DIGEST **PCR_Data   Pointer to the returned Struct, Alocated Dynamically
@Return:
    return PCR_Data_Iterator    Number of returned Structs
@Description:
    This function takes in the file buffer and its size. It will then loop through the
    buffer and find the number of valid policies in the file. It will then allocate the
    return struct and reloop through the file setting the valid values to the struct.
    It will then return the size of the return struct.
*/
static uint32_t
__attribute__((__unused__))
shim_policy_parsing(CHAR8 *Buf, UINTN BufSize,  TPM_PCR_DIGEST **PCR_Data){
    uint32_t Buf_Iterator = 0, MAX_ALG_Iterator = 0, PCR_Data_Iterator = 0;

    uint32_t PCR = 0;       //Stores the PCR value in the policy
    uint32_t PCR_DIGEST_Alloc_Set = 0;
   
    CHAR8 *ALG	= NULL;     //Stores the Algothim type in the policy

    CHAR8 *ALG_HASH_BUFF = NULL;    //Stores the Hash in the policy

    ALG = AllocateZeroPool(MAX_ALG);
    ALG_HASH_BUFF = AllocateZeroPool(SHA512_DIGEST_SIZE);

    //Config should look like PCR_00_SHA1:*******SHA1_HASH*******
    //PCR_DIGEST_Alloc_Set Loops through the file twice, once to get the
    //Number of PCR values in the file to alocate the array that those
    //values will be stored in. 
    //PCR_DIGEST_Alloc_Set = 0 will be getting the # of settings
    //PCR_DIGEST_Alloc_Set = 1 Actually getting the values
    while(PCR_DIGEST_Alloc_Set < 2){
        while(Buf_Iterator < BufSize){
            //Checks for the PCR_ string if it is not found the chars are incramented by 1
            if((Buf_Iterator + 4) <= BufSize && Buf[Buf_Iterator] == 0x50 && Buf[Buf_Iterator+1] == 0x43 && Buf[Buf_Iterator+2] == 0x52 && Buf[Buf_Iterator+3] == 0x5F){
                Buf_Iterator+=4;
                        //Ascii to Hex Conversion No idea how this works but it does.
                
                if ((Buf_Iterator + 2) > BufSize) //Bounds check to make sure it is reading inside the file buffer
                    continue;

                PCR = ((Buf[Buf_Iterator] % 32 + 9) % 25 * 16) + ((Buf[Buf_Iterator+1] % 32 + 9) % 25);
                if(PCR >= IMPLEMENTATION_PCR) //if the pcr value is greater than or equal to 24 then restart
                    continue;
                Buf_Iterator+=2;

                if ((Buf_Iterator + 1) > BufSize) //Bounds check to make sure it is reading inside the file buffer
                    continue;

                if(Buf[Buf_Iterator] != 0x5F)  //Checks if _ is the next char
                    continue;
                Buf_Iterator++;     //Moves past the _

                MAX_ALG_Iterator=0;

                while(MAX_ALG_Iterator < MAX_ALG){	//while loop to check for the ':' it will only ckeck up to 30 char
                    if ((Buf_Iterator + MAX_ALG_Iterator) > BufSize) //Bounds check to make sure it is reading inside the file buffer
                        continue;
                    if(Buf[Buf_Iterator+MAX_ALG_Iterator] == 0x3A){
                        break;
                    }
                    else
                        MAX_ALG_Iterator++;
                }
                
                if ((Buf_Iterator + MAX_ALG_Iterator + 1) > BufSize) //Bounds check to make sure it is reading inside the file buffer
                        continue;

                if(MAX_ALG_Iterator == MAX_ALG || Buf[Buf_Iterator+MAX_ALG_Iterator] != 0x3A)	//Checks if the ':' is found if not restart
                    continue;
                
                //Copy the Algorithm HASH type from Buf to ALG
                CopyMem(ALG, &Buf[Buf_Iterator], MAX_ALG_Iterator);

                Buf_Iterator+=MAX_ALG_Iterator+1; //Moving past the ALG_HASH:
                
                //Because Switches are stupid thus ALL THE IF
                if(MAX_ALG_Iterator == 4 && (!CompareMem(ALG, "SHA1", 4) || !CompareMem(ALG, "sha1", 4))){
                    if (PCR_DIGEST_Alloc_Set == 1){
                        if ((Buf_Iterator + (SHA1_DIGEST_SIZE * 2)) <= BufSize) //if the read goes out of bounds then zero the buffer
                            hex_array_to_char_array(&Buf[Buf_Iterator], (SHA1_DIGEST_SIZE * 2), ALG_HASH_BUFF);
                        else{
                            ZeroMem(ALG_HASH_BUFF, SHA512_DIGEST_SIZE);
                            CopyMem(ALG_HASH_BUFF, "ERROR out of Bounds", 19); //return error as the pcr value, will see in debug mode
                        }
                        
                        (*PCR_Data)[PCR_Data_Iterator].PCR = PCR;
                        (*PCR_Data)[PCR_Data_Iterator].DIGEST_SIZE = SHA1_DIGEST_SIZE;
                        (*PCR_Data)[PCR_Data_Iterator].hashAlg = TPM_ALG_SHA1;
                        CopyMem(&(*PCR_Data)[PCR_Data_Iterator].buffer, ALG_HASH_BUFF, SHA1_DIGEST_SIZE);
                    }
                    PCR_Data_Iterator++;    
                }
                if(MAX_ALG_Iterator == 6 && (!CompareMem(ALG, "SHA256", 6)|| !CompareMem(ALG, "sha256", 6))){
                    if (PCR_DIGEST_Alloc_Set == 1){
                        if((Buf_Iterator + (SHA256_DIGEST_SIZE * 2)) <= BufSize) //if the read goes out of bounds then zero the buffer
                            hex_array_to_char_array(&Buf[Buf_Iterator], (SHA256_DIGEST_SIZE * 2), ALG_HASH_BUFF);
                        else{
                            ZeroMem(ALG_HASH_BUFF, SHA512_DIGEST_SIZE);
                            CopyMem(ALG_HASH_BUFF, "ERROR out of Bounds", 19); //return error as the pcr value, will see in debug mode
                        }
                        
                        (*PCR_Data)[PCR_Data_Iterator].PCR = PCR;
                        (*PCR_Data)[PCR_Data_Iterator].DIGEST_SIZE = SHA256_DIGEST_SIZE;
                        (*PCR_Data)[PCR_Data_Iterator].hashAlg = TPM_ALG_SHA256;
                        CopyMem(&(*PCR_Data)[PCR_Data_Iterator].buffer, ALG_HASH_BUFF, SHA256_DIGEST_SIZE);
                    }
                    PCR_Data_Iterator++;
                }
                if(MAX_ALG_Iterator == 7 && (!CompareMem(ALG, "SM3_256", 7) || !CompareMem(ALG, "sm3_256", 7))){
                    if (PCR_DIGEST_Alloc_Set == 1){
                        hex_array_to_char_array(&Buf[Buf_Iterator], (SM3_256_DIGEST_SIZE * 2), ALG_HASH_BUFF);
                        (*PCR_Data)[PCR_Data_Iterator].PCR = PCR;
                        (*PCR_Data)[PCR_Data_Iterator].DIGEST_SIZE = SM3_256_DIGEST_SIZE;
                        (*PCR_Data)[PCR_Data_Iterator].hashAlg = TPM_ALG_SM3_256;
                        CopyMem(&(*PCR_Data)[PCR_Data_Iterator].buffer, ALG_HASH_BUFF, SM3_256_DIGEST_SIZE);
                    }

                    Buf_Iterator+=(SM3_256_DIGEST_SIZE * 2);
                    PCR_Data_Iterator++;
                }
                if(MAX_ALG_Iterator == 6 && (!CompareMem(ALG, "SHA384", 6) || !CompareMem(ALG, "sha384", 6))){
                    if (PCR_DIGEST_Alloc_Set == 1){
                        if((Buf_Iterator + (SHA384_DIGEST_SIZE * 2)) <= BufSize) //if the read goes out of bounds then zero the buffer
                            hex_array_to_char_array(&Buf[Buf_Iterator], (SHA384_DIGEST_SIZE * 2), ALG_HASH_BUFF);
                        else{
                            ZeroMem(ALG_HASH_BUFF, SHA512_DIGEST_SIZE);
                            CopyMem(ALG_HASH_BUFF, "ERROR out of Bounds", 19); //return error as the pcr value, will see in debug mode
                        }
                        
                        (*PCR_Data)[PCR_Data_Iterator].PCR = PCR;
                        (*PCR_Data)[PCR_Data_Iterator].DIGEST_SIZE = SHA384_DIGEST_SIZE;
                        (*PCR_Data)[PCR_Data_Iterator].hashAlg = TPM_ALG_SHA384;
                        CopyMem(&(*PCR_Data)[PCR_Data_Iterator].buffer, ALG_HASH_BUFF, SHA384_DIGEST_SIZE);
                    }
                    PCR_Data_Iterator++;
                }
                if(MAX_ALG_Iterator == 6 && (!CompareMem(ALG, "SHA512", 6) || !CompareMem(ALG, "sha512", 6))){
                    if (PCR_DIGEST_Alloc_Set == 1){
                        if((Buf_Iterator + (SHA512_DIGEST_SIZE * 2)) <= BufSize) //if the read goes out of bounds then zero the buffer
                            hex_array_to_char_array(&Buf[Buf_Iterator], (SHA512_DIGEST_SIZE * 2), ALG_HASH_BUFF);
                        else{
                            ZeroMem(ALG_HASH_BUFF, SHA512_DIGEST_SIZE);
                            CopyMem(ALG_HASH_BUFF, "ERROR out of Bounds", 19); //return error as the pcr value, will see in debug mode 
                        }
                        
                        (*PCR_Data)[PCR_Data_Iterator].PCR = PCR;
                        (*PCR_Data)[PCR_Data_Iterator].DIGEST_SIZE = SHA512_DIGEST_SIZE;
                        (*PCR_Data)[PCR_Data_Iterator].hashAlg = TPM_ALG_SHA512; //if the read goes out of bounds then zero the buffer
                        CopyMem(&(*PCR_Data)[PCR_Data_Iterator].buffer, ALG_HASH_BUFF, SHA512_DIGEST_SIZE);
                    }
                    PCR_Data_Iterator++;
                }
            }
            else
                Buf_Iterator++;
        }
        if (PCR_DIGEST_Alloc_Set == 0){
            if(PCR_Data_Iterator == 0) //If the commands finds Nothing return 0
                return 0;

            //Alocate structure and reset values for second loop through
            *PCR_Data =  AllocateZeroPool(sizeof(TPM_PCR_DIGEST) * PCR_Data_Iterator);
            PCR_Data_Iterator = 0;
            Buf_Iterator = 0;
        }

        PCR_DIGEST_Alloc_Set++;
    }

    buf_tpm_pcr_digest_sorter(PCR_Data, PCR_Data_Iterator);

    FreePool(ALG);
    FreePool(ALG_HASH_BUFF);
    return PCR_Data_Iterator;
}

#endif /* UTILS_H_ */