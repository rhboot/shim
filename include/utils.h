// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef UTILS_H_
#define UTILS_H_

EFI_STATUS get_file_size(EFI_FILE_HANDLE fh, UINTN *retsize);
EFI_STATUS
read_file(EFI_FILE_HANDLE fh, CHAR16 *fullpath, CHAR16 **buffer, UINT64 *bs);

#endif /* UTILS_H_ */
