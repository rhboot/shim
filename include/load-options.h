// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * load-options.h - all the stuff we need to parse the load options
 */

#ifndef SHIM_ARGV_H_
#define SHIM_ARGV_H_

EFI_STATUS generate_path_from_image_path(EFI_LOADED_IMAGE *li,
					 CHAR16 *ImagePath,
					 CHAR16 **PathName);

EFI_STATUS parse_load_options(EFI_LOADED_IMAGE *li);

extern CHAR16 *second_stage;
extern void *load_options;
extern UINT32 load_options_size;

#endif /* !SHIM_ARGV_H_ */
// vim:fenc=utf-8:tw=75:noet
