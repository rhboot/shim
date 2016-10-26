/** @file

Copyright (c) 2013 Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Module Name:

    PlatformDataFileNameGuids.h

Abstract:

  GUIDs used for finding platform data files in firmware volumes.

  To Add file to build do the followings steps.
  1)
  Create platform data bin file and copy file to
  QuarkPlatformPkg/Binary/PlatformData directory.

  2)
  Create a guid for the new platform data file (files in EDK firmware
  volumes have a file name guid associated with them).

  3)
  Place the new guid define below and also add it to the
  PDAT_FILE_NAME_TABLE_DEFINITION marco below.

  4)
  Add file specifier in the stage1 FV volumes in the platform .fdf file.
  example for kips bay platform is :-
    FILE FREEFORM = 956EDAD3-8440-45cb-89AC-D1930C004E34 {
      SECTION RAW = QuarkPlatformPkg/Binary/PlatformData/kipsbay-platform-data.bin
    }

--*/

#ifndef _PLATFORM_DATA_FILE_NAME_GUIDS_H_
#define _PLATFORM_DATA_FILE_NAME_GUIDS_H_

//
// Known platform file name Guids.
//

#define SVP_PDAT_FILE_NAME_GUID \
  { 0xa975562, 0xdf47, 0x4dc3, { 0x8a, 0xb0, 0x3b, 0xa2, 0xc3, 0x52, 0x23, 0x2 } }

#define KIPSBAY_PDAT_FILE_NAME_GUID \
  { 0x956edad3, 0x8440, 0x45cb, { 0x89, 0xac, 0xd1, 0x93, 0xc, 0x0, 0x4e, 0x34 } }

#define CROSSHILL_PDAT_FILE_NAME_GUID \
  { 0x95b3c16, 0x6d67, 0x4c85, { 0xb5, 0x28, 0x33, 0x9d, 0x9f, 0xf6, 0x22, 0x2c } }

#define CLANTONHILL_PDAT_FILE_NAME_GUID \
  { 0xee84c5e7, 0x9412, 0x42cc, { 0xb7, 0x55, 0xa9, 0x15, 0xa7, 0xb6, 0x85, 0x36 } }

#define GALILEO_PDAT_FILE_NAME_GUID \
  { 0xe4ad87c8, 0xd20e, 0x40ce, { 0x97, 0xf5, 0x97, 0x56, 0xfd, 0xe, 0x81, 0xd4 } }

#define GALILEO_GEN2_PDAT_FILE_NAME_GUID \
  { 0x23b3c10d, 0x46e3, 0x4a78, { 0x8a, 0xaa, 0x21, 0x7b, 0x6a, 0x39, 0xef, 0x4 } }


#define PDAT_FILE_NAME_TABLE_DEFINITION \
  /* EFI_PLATFORM_TYPE - ClantonPeakSVP*/\
  SVP_PDAT_FILE_NAME_GUID,\
  /* EFI_PLATFORM_TYPE - KipsBay*/\
  KIPSBAY_PDAT_FILE_NAME_GUID,\
  /* EFI_PLATFORM_TYPE - CrossHill*/\
  CROSSHILL_PDAT_FILE_NAME_GUID,\
  /* EFI_PLATFORM_TYPE - ClantonHill*/\
  CLANTONHILL_PDAT_FILE_NAME_GUID,\
  /* EFI_PLATFORM_TYPE - Galileo*/\
  GALILEO_PDAT_FILE_NAME_GUID,\
  /* EFI_PLATFORM_TYPE - GalileoGen2*/\
  GALILEO_GEN2_PDAT_FILE_NAME_GUID,\

#endif
