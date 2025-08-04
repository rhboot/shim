// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * pe.c - helper functions for pe binaries.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h>
#include <openssl/dso.h>

#include <Library/BaseCryptLib.h>

#define check_size_line(data, datasize_in, hashbase, hashsize, l) ({	\
	if ((unsigned long)hashbase >					\
			(unsigned long)data + datasize_in) {		\
		efi_status = EFI_INVALID_PARAMETER;			\
		perror(L"shim.c:%d Invalid hash base 0x%016x\n", l,	\
			hashbase);					\
		goto done;						\
	}								\
	if ((unsigned long)hashbase + hashsize >			\
			(unsigned long)data + datasize_in) {		\
		efi_status = EFI_INVALID_PARAMETER;			\
		perror(L"shim.c:%d Invalid hash size 0x%016x\n", l,	\
			hashsize);					\
		goto done;						\
	}								\
})
#define check_size(d, ds, h, hs) check_size_line(d, ds, h, hs, __LINE__)

static EFI_STATUS
_do_sha256_sum(void *addr, UINTN size, UINT8 *digest)
{
	unsigned int sha256ctxsize;
	void *sha256ctx = NULL;

	sha256ctxsize = Sha256GetContextSize();
	sha256ctx = AllocateZeroPool(sha256ctxsize);
	if (sha256ctx == NULL)
		return EFI_OUT_OF_RESOURCES;

	if (!Sha256Init(sha256ctx))
		return EFI_OUT_OF_RESOURCES;

	if (!Sha256Update(sha256ctx, addr, size))
		return EFI_OUT_OF_RESOURCES;

	if (!Sha256Final(sha256ctx, digest))
		return EFI_OUT_OF_RESOURCES;

	FreePool(sha256ctx);
	return EFI_SUCCESS;
}

struct shim_section_cache_entry {
	EFI_HANDLE parent_image_handle;
	UINT8 section_name[9];
	UINTN size;
	/*
	 * Since this is all internal and there's no API access to it, this is
	 * currently always sha256 and can be updated as needed.
	 */
	UINT8 digest[32];
};

static struct shim_section_cache_entry *section_cache = NULL;
static UINTN num_section_cache_entries = 0;

static EFI_STATUS
cache_section(EFI_HANDLE parent_image_handle, UINT8 section_name[8],
	      void *addr, UINTN size)
{
	struct shim_section_cache_entry *new_section_cache = NULL;
	struct shim_section_cache_entry *entry = NULL;
	size_t oscsz = num_section_cache_entries * sizeof (*new_section_cache);
	size_t nscsz = oscsz + sizeof (*new_section_cache);
	EFI_STATUS efi_status;

	new_section_cache = AllocateZeroPool(nscsz);
	if (!new_section_cache)
		return EFI_OUT_OF_RESOURCES;

	if (section_cache) {
		CopyMem(new_section_cache, section_cache, oscsz);
		FreePool(section_cache);
	}
	section_cache = new_section_cache;
	entry = &section_cache[num_section_cache_entries];

	entry->parent_image_handle = parent_image_handle;
	CopyMem(entry->section_name, section_name, sizeof(entry->section_name)-1);
	entry->size = size;

	efi_status = _do_sha256_sum(addr, size, entry->digest);
	if (EFI_ERROR(efi_status)) {
		ZeroMem(entry, sizeof (*entry));
		return efi_status;
	}
	num_section_cache_entries += 1;
	return EFI_SUCCESS;
}

EFI_STATUS
validate_cached_section(EFI_HANDLE parent_image_handle,
			void *addr, UINTN size)
{
	struct shim_section_cache_entry *section = NULL;
	EFI_STATUS efi_status;

	for (UINTN i = 0; i < num_section_cache_entries; i++) {
		struct shim_section_cache_entry *this_entry = &section_cache[i];
		UINT8 digest[32];

		dprint(L"Handles: 0x%016llx 0x%016llx section: '%a'\n",
		       (unsigned long long)(uintptr_t)this_entry->parent_image_handle,
		       (unsigned long long)(uintptr_t)parent_image_handle,
		       this_entry->section_name);

		if (this_entry->size != size)
			continue;
		if (this_entry->parent_image_handle != parent_image_handle)
			continue;

		ZeroMem(digest, sizeof(digest));

		efi_status = _do_sha256_sum(addr, size, digest);
		if (EFI_ERROR(efi_status))
			return efi_status;

		if (CompareMem(digest, this_entry->digest, sizeof(digest)) != 0)
			continue;

		section = this_entry;
		break;
	}
	if (section == NULL)
		return EFI_NOT_FOUND;

	return EFI_SUCCESS;
}

void
flush_cached_sections(EFI_HANDLE parent_image_handle)
{
	UINTN reduction = 0;
	for (UINTN i = 0; i < num_section_cache_entries; i++) {
		struct shim_section_cache_entry *this_entry = &section_cache[i];

		if (this_entry->parent_image_handle != parent_image_handle)
			continue;

		reduction += 1;
		CopyMem(&this_entry[1], &this_entry[0], sizeof(*this_entry) * (num_section_cache_entries - i - 1));
	}

	ZeroMem(&section_cache[num_section_cache_entries - reduction],
		sizeof(section_cache[0]) * reduction);

	num_section_cache_entries -= reduction;
}

/*
 * Calculate the SHA1 and SHA256 hashes of a binary
 */

EFI_STATUS
generate_hash(char *data, unsigned int datasize,
	      PE_COFF_LOADER_IMAGE_CONTEXT *context, UINT8 *sha256hash,
	      UINT8 *sha1hash)
{
	unsigned int sha256ctxsize, sha1ctxsize;
	void *sha256ctx = NULL, *sha1ctx = NULL;
	char *hashbase;
	unsigned int hashsize;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos;
	EFI_IMAGE_SECTION_HEADER *Section;
	EFI_IMAGE_SECTION_HEADER *SectionHeader = NULL;
	EFI_STATUS efi_status = EFI_SUCCESS;
	EFI_IMAGE_DOS_HEADER *DosHdr = (void *)data;
	unsigned int PEHdr_offset = 0;

	if (datasize <= sizeof (*DosHdr) ||
	    DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
		perror(L"Invalid signature\n");
		return EFI_INVALID_PARAMETER;
	}
	PEHdr_offset = DosHdr->e_lfanew;

	sha256ctxsize = Sha256GetContextSize();
	sha256ctx = AllocatePool(sha256ctxsize);

	sha1ctxsize = Sha1GetContextSize();
	sha1ctx = AllocatePool(sha1ctxsize);

	if (!sha256ctx || !sha1ctx) {
		perror(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(sha256ctx) || !Sha1Init(sha1ctx)) {
		perror(L"Unable to initialise hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash start to checksum */
	hashbase = data;
	hashsize = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum -
		hashbase;
	check_size(data, datasize, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash post-checksum to start of certificate table */
	hashbase = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum +
		sizeof (int);
	hashsize = (char *)context->SecDir - hashbase;
	check_size(data, datasize, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash end of certificate table to end of image header */
	EFI_IMAGE_DATA_DIRECTORY *dd = context->SecDir + 1;
	hashbase = (char *)dd;
	hashsize = context->SizeOfHeaders - (unsigned long)((char *)dd - data);
	if (hashsize > datasize) {
		perror(L"Data Directory size %d is invalid\n", hashsize);
		efi_status = EFI_INVALID_PARAMETER;
		goto done;
	}
	check_size(data, datasize, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Sort sections */
	SumOfBytesHashed = context->SizeOfHeaders;

	/*
	 * XXX Do we need this here, or is it already done in all cases?
	 */
	if (context->NumberOfSections == 0 ||
	    context->FirstSection == NULL) {
		uint16_t opthdrsz;
		uint64_t addr;
		uint16_t nsections;
		EFI_IMAGE_SECTION_HEADER *section0, *sectionN;

		nsections = context->PEHdr->Pe32.FileHeader.NumberOfSections;
		opthdrsz = context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader;

		/* Validate section0 is within image */
		addr = PEHdr_offset + sizeof(UINT32)
			+ sizeof(EFI_IMAGE_FILE_HEADER)
			+ opthdrsz;
		section0 = ImageAddress(data, datasize, addr);
		if (!section0) {
			perror(L"Malformed file header.\n");
			perror(L"Image address for Section Header 0 is 0x%016llx\n",
			       addr);
			perror(L"File size is 0x%016llx\n", datasize);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}

		/* Validate sectionN is within image */
		addr += (uint64_t)(intptr_t)&section0[nsections-1] -
			(uint64_t)(intptr_t)section0;
		sectionN = ImageAddress(data, datasize, addr);
		if (!sectionN) {
			perror(L"Malformed file header.\n");
			perror(L"Image address for Section Header %d is 0x%016llx\n",
			       nsections - 1, addr);
			perror(L"File size is 0x%016llx\n", datasize);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}

		context->NumberOfSections = nsections;
		context->FirstSection = section0;
	}

	/*
	 * Allocate a new section table so we can sort them without
	 * modifying the image.
	 */
	SectionHeader = AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER)
					  * context->NumberOfSections);
	if (SectionHeader == NULL) {
		perror(L"Unable to allocate section header\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/*
	 * Validate section locations and sizes, and sort the table into
	 * our newly allocated header table
	 */
	SumOfSectionBytes = 0;
	Section = context->FirstSection;
	for (index = 0; index < context->NumberOfSections; index++) {
		EFI_IMAGE_SECTION_HEADER *SectionPtr;
		char *base;
		size_t size;

		efi_status = get_section_vma(index, data, datasize, context,
					     &base, &size, &SectionPtr);
		if (efi_status == EFI_NOT_FOUND)
			break;
		if (EFI_ERROR(efi_status)) {
			perror(L"Malformed section header\n");
			goto done;
		}

		/* Validate section size is within image. */
		if (SectionPtr->SizeOfRawData >
		    datasize - SumOfBytesHashed - SumOfSectionBytes) {
			perror(L"Malformed section %d size\n", index);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		SumOfSectionBytes += SectionPtr->SizeOfRawData;

		pos = index;
		while ((pos > 0) && (Section->PointerToRawData < SectionHeader[pos - 1].PointerToRawData)) {
			CopyMem (&SectionHeader[pos], &SectionHeader[pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
			pos--;
		}
		CopyMem (&SectionHeader[pos], Section, sizeof (EFI_IMAGE_SECTION_HEADER));
		Section += 1;

	}

	/* Hash the sections */
	for (index = 0; index < context->NumberOfSections; index++) {
		Section = &SectionHeader[index];
		if (Section->SizeOfRawData == 0) {
			continue;
		}

		hashbase  = ImageAddress(data, datasize,
					 Section->PointerToRawData);
		if (!hashbase) {
			perror(L"Malformed section header\n");
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}

		/* Verify hashsize within image. */
		if (Section->SizeOfRawData >
		    datasize - Section->PointerToRawData) {
			perror(L"Malformed section raw size %d\n", index);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		hashsize  = (unsigned int) Section->SizeOfRawData;
		check_size(data, datasize, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		SumOfBytesHashed += Section->SizeOfRawData;
	}

	/* Hash all remaining data up to SecDir if SecDir->Size is not 0 */
	if (datasize > SumOfBytesHashed && context->SecDir->Size) {
		hashbase = data + SumOfBytesHashed;
		hashsize = datasize - context->SecDir->Size - SumOfBytesHashed;

		if ((datasize - SumOfBytesHashed < context->SecDir->Size) ||
		    (SumOfBytesHashed + hashsize != context->SecDir->VirtualAddress)) {
			perror(L"Malformed binary after Attribute Certificate Table\n");
			console_print(L"datasize: %u SumOfBytesHashed: %u SecDir->Size: %lu\n",
				      datasize, SumOfBytesHashed, context->SecDir->Size);
			console_print(L"hashsize: %u SecDir->VirtualAddress: 0x%08lx\n",
				      hashsize, context->SecDir->VirtualAddress);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		check_size(data, datasize, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}

		SumOfBytesHashed += hashsize;
	}

	/* Hash all remaining data. If SecDir->Size is > 0 this code should not
	 * be entered.  If it is, there are still things to hash.  For a file
	 * without a SecDir, we need to hash what remains. */
	if (datasize > SumOfBytesHashed + context->SecDir->Size) {
		char padbuf[8];
		ZeroMem(padbuf, 8);

		hashbase = data + SumOfBytesHashed;
		hashsize = datasize - SumOfBytesHashed;

		check_size(data, datasize, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}

		SumOfBytesHashed += hashsize;
		hashsize = ALIGN_VALUE(SumOfBytesHashed, 8) - SumOfBytesHashed;

		if (hashsize) {
			if (!(Sha256Update(sha256ctx, padbuf, hashsize)) ||
			    !(Sha1Update(sha1ctx, padbuf, hashsize))) {
				perror(L"Unable to generate hash\n");
				efi_status = EFI_OUT_OF_RESOURCES;
				goto done;
			}
		}
	}

	if (!(Sha256Final(sha256ctx, sha256hash)) ||
	    !(Sha1Final(sha1ctx, sha1hash))) {
		perror(L"Unable to finalise hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	dprint(L"sha1 authenticode hash:\n");
	dhexdumpat(sha1hash, SHA1_DIGEST_SIZE, 0);
	dprint(L"sha256 authenticode hash:\n");
	dhexdumpat(sha256hash, SHA256_DIGEST_SIZE, 0);

done:
	if (SectionHeader)
		FreePool(SectionHeader);
	if (sha1ctx)
		FreePool(sha1ctx);
	if (sha256ctx)
		FreePool(sha256ctx);

	return efi_status;
}

EFI_STATUS
verify_sbat_section(char *SBATBase, size_t SBATSize)
{
	unsigned int i;
	EFI_STATUS efi_status;
	size_t n;
	struct sbat_section_entry **entries = NULL;
	char *sbat_data;
	size_t sbat_size;

	if (list_empty(&sbat_var))
		return EFI_SUCCESS;

	if (SBATBase == NULL || SBATSize == 0) {
		dprint(L"No .sbat section data\n");
		/*
		 * SBAT is mandatory for binaries loaded by shim, but optional
		 * for binaries loaded outside of shim but verified via the
		 * protocol.
		 */
		return in_protocol ? EFI_SUCCESS : EFI_SECURITY_VIOLATION;
	}

	if (checked_add(SBATSize, 1, &sbat_size)) {
		dprint(L"SBATSize + 1 would overflow\n");
		return EFI_SECURITY_VIOLATION;
	}

	sbat_data = AllocatePool(sbat_size);
	if (!sbat_data) {
		console_print(L"Failed to allocate .sbat section buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}
	CopyMem(sbat_data, SBATBase, SBATSize);
	sbat_data[SBATSize] = '\0';

	efi_status = parse_sbat_section(sbat_data, sbat_size, &n, &entries);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not parse .sbat section data: %r\n", efi_status);
		goto err;
	}

	dprint(L"SBAT section data\n");
        for (i = 0; i < n; i++) {
		dprint(L"%a, %a, %a, %a, %a, %a\n",
		       entries[i]->component_name,
		       entries[i]->component_generation,
		       entries[i]->vendor_name,
		       entries[i]->vendor_package_name,
		       entries[i]->vendor_version,
		       entries[i]->vendor_url);
	}

	efi_status = verify_sbat(n, entries);

	cleanup_sbat_section_entries(n, entries);

err:
	FreePool(sbat_data);

	return efi_status;
}

EFI_STATUS verify_image(void *data, unsigned int datasize,
			EFI_LOADED_IMAGE *li,
			PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_STATUS efi_status;
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	UINT8 sha256hash[SHA256_DIGEST_SIZE];

	/*
	 * The binary header contains relevant context and section pointers
	 */
	efi_status = read_header(data, datasize, context, true);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to read header: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * Perform the image verification before we start copying data around
	 * in order to load it.
	 */
	if (secure_mode()) {
		efi_status = verify_buffer(data, datasize,
					   context, sha256hash, sha1hash,
					   false);
		if (EFI_ERROR(efi_status)) {
			if (verbose)
				console_print(L"Verification failed: %r\n", efi_status);
			else
				console_error(L"Verification failed", efi_status);
			return efi_status;
		} else if (verbose)
			console_print(L"Verification succeeded\n");
	}

	/*
	 * Calculate the hash for the TPM measurement.
	 * XXX: We're computing these twice in secure boot mode when the
	 *  buffers already contain the previously computed hashes. Also,
	 *  this is only useful for the TPM1.2 case. We should try to fix
	 *  this in a follow-up.
	 */
	efi_status = generate_hash(data, datasize, context, sha256hash,
				   sha1hash);
	if (EFI_ERROR(efi_status))
		return efi_status;

	/* Measure the binary into the TPM */
#ifdef REQUIRE_TPM
	efi_status =
#endif
	tpm_log_pe((EFI_PHYSICAL_ADDRESS)(UINTN)data, datasize,
		   (EFI_PHYSICAL_ADDRESS)(UINTN)context->ImageAddress,
		   li->FilePath, sha1hash, 4);
#ifdef REQUIRE_TPM
	if (efi_status != EFI_SUCCESS) {
		return efi_status;
	}
#endif

	return EFI_SUCCESS;
}

/*
 * Once the image has been loaded it needs to be validated and relocated
 */
EFI_STATUS
handle_image (void *data, unsigned int datasize,
	      EFI_LOADED_IMAGE *li, EFI_HANDLE image_handle,
	      EFI_IMAGE_ENTRY_POINT *entry_point,
	      EFI_PHYSICAL_ADDRESS *alloc_address,
	      UINTN *alloc_pages, unsigned int *alloc_alignment,
	      bool parent_verified)
{
	EFI_STATUS efi_status;
	char *buffer;
	int i;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	UINT32 size;
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	unsigned int alloc_size;
	int found_entry_point = 0;
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	UINT8 sha256hash[SHA256_DIGEST_SIZE];

	/*
	 * The binary header contains relevant context and section pointers
	 */
	efi_status = read_header(data, datasize, &context, true);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to read header: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * Perform the image verification before we start copying data around
	 * in order to load it.
	 */
	if (secure_mode ()) {
		efi_status = verify_buffer(data, datasize, &context, sha256hash,
					   sha1hash, parent_verified);

		if (EFI_ERROR(efi_status)) {
			if (verbose || in_protocol)
				console_print(L"Verification failed: %r\n", efi_status);
			else
				console_error(L"Verification failed", efi_status);
			return efi_status;
		} else {
			if (verbose)
				console_print(L"Verification succeeded\n");
		}
	}

	/*
	 * We had originally thought about making this much more granular
	 * and logging the child section hashes in the event log, but the
	 * EFI APIs give us extend-without-logging but not
	 * logging-without-extending, so there's no point.
	 */
	if (!parent_verified) {
		/*
		 * Calculate the hash for the TPM measurement.
		 * XXX: We're computing these twice in secure boot mode when the
		 *  buffers already contain the previously computed hashes. Also,
		 *  this is only useful for the TPM1.2 case. We should try to fix
		 *  this in a follow-up.
		 */
		efi_status = generate_hash(data, datasize, &context, sha256hash,
					   sha1hash);
		if (EFI_ERROR(efi_status))
			return efi_status;

		/* Measure the binary into the TPM */
#ifdef REQUIRE_TPM
		efi_status =
#endif
		tpm_log_pe((EFI_PHYSICAL_ADDRESS)(UINTN)data, datasize,
			   (EFI_PHYSICAL_ADDRESS)(UINTN)context.ImageAddress,
			   li->FilePath, sha1hash, 4);
#ifdef REQUIRE_TPM
		if (efi_status != EFI_SUCCESS) {
			return efi_status;
		}
#endif
	}

	/* The spec says, uselessly, of SectionAlignment:
	 * =====
	 * The alignment (in bytes) of sections when they are loaded into
	 * memory. It must be greater than or equal to FileAlignment. The
	 * default is the page size for the architecture.
	 * =====
	 * Which doesn't tell you whose responsibility it is to enforce the
	 * "default", or when.  It implies that the value in the field must
	 * be > FileAlignment (also poorly defined), but it appears visual
	 * studio will happily write 512 for FileAlignment (its default) and
	 * 0 for SectionAlignment, intending to imply PAGE_SIZE.
	 *
	 * We only support one page size, so if it's zero, nerf it to 4096.
	 */
	*alloc_alignment = context.SectionAlignment;
	if (!*alloc_alignment)
		*alloc_alignment = 4096;

	alloc_size = ALIGN_VALUE(context.ImageSize + context.SectionAlignment,
				 PAGE_SIZE);
	*alloc_pages = alloc_size / PAGE_SIZE;

	efi_status = BS->AllocatePages(AllocateAnyPages, EfiLoaderCode,
				       *alloc_pages, alloc_address);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	buffer = (void *)ALIGN_VALUE((unsigned long)*alloc_address, *alloc_alignment);
	dprint(L"Loading 0x%llx bytes at 0x%llx\n",
	       (unsigned long long)context.ImageSize,
	       (unsigned long long)(uintptr_t)buffer);
	update_mem_attrs((uintptr_t)buffer, alloc_size, MEM_ATTR_R|MEM_ATTR_W,
			 MEM_ATTR_X);

	CopyMem(buffer, data, context.SizeOfHeaders);

	/* Flush the instruction cache for the region holding the image */
	cache_invalidate(buffer, buffer + context.ImageSize);

	*entry_point = ImageAddress(buffer, context.ImageSize, context.EntryPoint);
	if (!*entry_point) {
		perror(L"Entry point is invalid\n");
		BS->FreePages(*alloc_address, *alloc_pages);
		return EFI_UNSUPPORTED;
	}

	char *RelocBase, *RelocBaseEnd;
	/*
	 * These are relative virtual addresses, so we have to check them
	 * against the image size, not the data size.
	 */
	RelocBase = ImageAddress(buffer, context.ImageSize,
				 context.RelocDir->VirtualAddress);
	/*
	 * RelocBaseEnd here is the address of the last byte of the table
	 */
	RelocBaseEnd = ImageAddress(buffer, context.ImageSize,
				    context.RelocDir->VirtualAddress +
				    context.RelocDir->Size - 1);

	EFI_IMAGE_SECTION_HEADER *RelocSection = NULL;

	/*
	 * Copy the executable's sections to their desired offsets
	 */
	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++, Section++) {
		/* Don't try to copy discardable sections with zero size */
		if ((Section->Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) &&
		    !Section->Misc.VirtualSize)
			continue;

		/*
		 * Skip sections that aren't marked readable.
		 */
		if (!(Section->Characteristics & EFI_IMAGE_SCN_MEM_READ))
			continue;

		if (!(Section->Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) &&
		    (Section->Characteristics & EFI_IMAGE_SCN_MEM_WRITE) &&
		    (Section->Characteristics & EFI_IMAGE_SCN_MEM_EXECUTE) &&
		    (mok_policy & MOK_POLICY_REQUIRE_NX)) {
			perror(L"Section %d is writable and executable\n", i);
			BS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}

		base = ImageAddress (buffer, context.ImageSize,
				     Section->VirtualAddress);
		end = ImageAddress (buffer, context.ImageSize,
				    Section->VirtualAddress
				     + Section->Misc.VirtualSize - 1);

		if (end < base) {
			perror(L"Section %d has negative size\n", i);
			BS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}

		if (Section->VirtualAddress <= context.EntryPoint &&
		    (Section->VirtualAddress + Section->Misc.VirtualSize - 1)
		    > context.EntryPoint)
			found_entry_point++;

		/* We do want to process .reloc, but it's often marked
		 * discardable, so we don't want to memcpy it. */
		if (CompareMem(Section->Name, ".reloc\0\0", 8) == 0) {
			if (RelocSection) {
				perror(L"Image has multiple relocation sections\n");
				BS->FreePages(*alloc_address, *alloc_pages);
				return EFI_UNSUPPORTED;
			}
			/* If it has nonzero sizes, and our bounds check
			 * made sense, and the VA and size match RelocDir's
			 * versions, then we believe in this section table. */
			if (Section->SizeOfRawData &&
					Section->Misc.VirtualSize &&
					base && end &&
					RelocBase == base &&
					RelocBaseEnd <= end) {
				RelocSection = Section;
			} else {
				perror(L"Relocation section is invalid \n");
				BS->FreePages(*alloc_address, *alloc_pages);
				return EFI_UNSUPPORTED;
			}
		}

		if (Section->Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		if (!base) {
			perror(L"Section %d has invalid base address\n", i);
			BS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}
		if (!end) {
			perror(L"Section %d has zero size\n", i);
			BS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}

		if (!(Section->Characteristics & EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
		    (Section->VirtualAddress < context.SizeOfHeaders ||
		     Section->PointerToRawData < context.SizeOfHeaders)) {
			perror(L"Section %d is inside image headers\n", i);
			BS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}

		if (Section->Characteristics & EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
			ZeroMem(base, Section->Misc.VirtualSize);
		} else {
			if (Section->PointerToRawData < context.SizeOfHeaders) {
				perror(L"Section %d is inside image headers\n", i);
				BS->FreePages(*alloc_address, *alloc_pages);
				return EFI_UNSUPPORTED;
			}

			size = Section->Misc.VirtualSize;
			if (size > Section->SizeOfRawData)
				size = Section->SizeOfRawData;

			if (size > 0)
				CopyMem(base, data + Section->PointerToRawData, size);

			if (size < Section->Misc.VirtualSize)
				ZeroMem(base + size, Section->Misc.VirtualSize - size);
		}
	}

	if (context.NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
		perror(L"Image has no relocation entry\n");
		BS->FreePages(*alloc_address, *alloc_pages);
		return EFI_UNSUPPORTED;
	}

	if (context.RelocDir->Size && RelocSection) {
		/*
		 * Run the relocation fixups
		 */
		efi_status = relocate_coff(&context, RelocSection, data,
					   buffer);

		if (EFI_ERROR(efi_status)) {
			perror(L"Relocation failed: %r\n", efi_status);
			BS->FreePages(*alloc_address, *alloc_pages);
			return efi_status;
		}
	}

	/*
	 * Now set the page permissions appropriately and cache appropriate
	 * section sizes, and digests.
	 */
	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++, Section++) {
		uint64_t set_attrs = MEM_ATTR_R;
		uint64_t clear_attrs = MEM_ATTR_W|MEM_ATTR_X;
		uintptr_t addr;
		uint64_t raw_length;
		uint64_t length;

		/*
		 * Skip discardable sections with zero size
		 */
		if ((Section->Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) &&
		    !Section->Misc.VirtualSize)
			continue;

		/*
		 * Skip sections that aren't marked readable.
		 */
		if (!(Section->Characteristics & EFI_IMAGE_SCN_MEM_READ))
			continue;

		base = ImageAddress (buffer, context.ImageSize,
				     Section->VirtualAddress);
		end = ImageAddress (buffer, context.ImageSize,
				    Section->VirtualAddress
				     + Section->Misc.VirtualSize - 1);

		addr = (uintptr_t)base;
		// Align the length up to PAGE_SIZE. This is required because
		// platforms generally set memory attributes at page
		// granularity, but the section length (unlike the section
		// address) is not required to be aligned.
		raw_length = (uintptr_t)end - (uintptr_t)base + 1;
		length = ALIGN_VALUE(raw_length, PAGE_SIZE);

		if (Section->Characteristics & EFI_IMAGE_SCN_MEM_WRITE) {
			set_attrs |= MEM_ATTR_W;
			clear_attrs &= ~MEM_ATTR_W;
		}
		if (Section->Characteristics & EFI_IMAGE_SCN_MEM_EXECUTE) {
			set_attrs |= MEM_ATTR_X;
			clear_attrs &= ~MEM_ATTR_X;
		}
		update_mem_attrs(addr, length, set_attrs, clear_attrs);

		/*
		 * We only cache CODE and INITIALIZED data sections that
		 * are marked readable.  Also, don't cache sections on the
		 * second level deep...
		 */
		if ((Section->Characteristics & EFI_IMAGE_SCN_CNT_CODE ||
		     Section->Characteristics & EFI_IMAGE_SCN_CNT_INITIALIZED_DATA) &&
		    Section->Characteristics & EFI_IMAGE_SCN_MEM_READ &&
		    !parent_verified) {
			efi_status = cache_section(image_handle, Section->Name, base, raw_length);
			if (EFI_ERROR(efi_status)) {
				perror(L"Failed to cache section details\n");
				BS->FreePages(*alloc_address, *alloc_pages);
				return efi_status;
			}
			dprint(L"Cached section %d (%a) at 0x%016llx, size 0x%016llx\n",
			       i, Section->Name,
			       (unsigned long long)(uintptr_t)base,
			       (unsigned long long)raw_length);
		}
	}

	/*
	 * grub needs to know its location and size in memory, so fix up
	 * the loaded image protocol values
	 */
	li->ImageBase = buffer;
	li->ImageSize = context.ImageSize;

	/* Pass the load options to the second stage loader */
	li->LoadOptions = load_options;
	li->LoadOptionsSize = load_options_size;

	if (!found_entry_point) {
		perror(L"Entry point is not within sections\n");
		flush_cached_sections(image_handle);
		BS->FreePages(*alloc_address, *alloc_pages);
		return EFI_UNSUPPORTED;
	}
	if (found_entry_point > 1) {
		perror(L"%d sections contain entry point\n", found_entry_point);
		flush_cached_sections(image_handle);
		BS->FreePages(*alloc_address, *alloc_pages);
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

// vim:fenc=utf-8:tw=75:noet
