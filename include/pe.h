// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * pe.h - helper functions for pe binaries.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef PE_H_
#define PE_H_

void *
ImageAddress (void *image, uint64_t size, uint64_t address);

EFI_STATUS
read_header(void *data, unsigned int datasize,
	    PE_COFF_LOADER_IMAGE_CONTEXT *context);

EFI_STATUS verify_image(void *data, unsigned int datasize,
			EFI_LOADED_IMAGE *li,
			PE_COFF_LOADER_IMAGE_CONTEXT *context);

EFI_STATUS
verify_sbat_section(char *SBATBase, size_t SBATSize);

EFI_STATUS
handle_image (void *data, unsigned int datasize,
	      EFI_LOADED_IMAGE *li,
	      EFI_IMAGE_ENTRY_POINT *entry_point,
	      EFI_PHYSICAL_ADDRESS *alloc_address,
	      UINTN *alloc_pages);

EFI_STATUS
generate_hash (char *data, unsigned int datasize,
	       PE_COFF_LOADER_IMAGE_CONTEXT *context,
	       UINT8 *sha256hash, UINT8 *sha1hash);

EFI_STATUS
relocate_coff (PE_COFF_LOADER_IMAGE_CONTEXT *context,
	       EFI_IMAGE_SECTION_HEADER *Section,
	       void *orig, void *data);

#endif /* !PE_H_ */
// vim:fenc=utf-8:tw=75:noet
