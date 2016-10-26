/*++

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

  CapsuleCreate.c

Abstract:

 Definitions for the CapsuleCreate utility.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Common/UefiBaseTypes.h>

#include "CommonLib.h"
#include "ParseInf.h"
#include "EfiUtilityMsgs.h"

#define UTILITY_NAME            "CapsuleCreate"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   3

#define BEGIN_STRING "BEGIN"
#define END_STRING   "END"
#define FILE_NAME_STRING "FILE_NAME="
#define START_ADDRESS_STRING "START_ADDRESS="

#define MAX_CAPSULE_PAYLOAD_SIZE 0xA00000
#define MAX_IMAGES_PER_CAPSULE 20

//
// 4KB SPI block size (sector erase size)
//
#define SPI_BLOCK_SIZE  0x1000
#define PAD_BYTE        0x00

CHAR8   *gCapsuleGuid;

#pragma pack(1)
typedef struct {
  EFI_GUID  CapsuleGuid;
  UINT32    HeaderSize;
  UINT32    Flags;
  UINT32    CapsuleImageSize;
  UINT8     Reserved[52];
} CAPSULE_HEADER;

typedef struct {
  UINT32    TargetAddr;
  UINT32    Size;
  UINT32    SourceOffset;
  UINT32    Reserved;
} UPDATE_HINT;
#pragma pack()


STATIC
VOID 
Version (
  VOID
  )
/*++

Routine Description:

  Print out version information for this utility.

Arguments:

  None
  
Returns:

  None
  
--*/ 
{
  fprintf (stdout, "%s Version %d.%d \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
}

STATIC
VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Print Error / Help message.

Arguments:

  VOID

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s InputConfigFileName OutputCapsuleFileName\n\n", UTILITY_NAME);
  
  //
  // Copyright declaration
  // 
  fprintf (stdout, "Copyright (c) 2011, Intel Corporation. All rights reserved.\n\n");
}

int
main (
  int   argc,
  CHAR8 *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  EFI_STATUS              Status;
  CHAR8                   *ImageBuffer;
  CHAR8                   *InputConfigFileName;
  CHAR8                   *InputCapsuleModulesDir;
  CHAR8                   *OutputCapsuleFileName;
  CHAR8                   *CapsuleFlagsString;
  CHAR8                   *ValueString;
  FILE                    *InputConfigFile;
  FILE                    *OutputCapsuleFile;
  FILE                    *InputCapsuleImage;
  CHAR8                   LineString[_MAX_PATH];
  UINTN                   LineLen;
  CHAR8                   FileName[_MAX_PATH];
  CAPSULE_HEADER          CapsuleHeader;
  UPDATE_HINT             HintData[MAX_IMAGES_PER_CAPSULE];
  UINT8                   ImageNumber;
  UINT64                  Value64;
  UINT32                  InputCapsuleImageSize;
  UINT32                  InputCapsuleImagePadSize;
  UINT32                  TotalPayloadImageSize;
  UINT32                  ReadFileSize;
  UINT32                  SourceOffset;
  UINT64                  CapsuleFlags;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "no options input");
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Parse command line
  //
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Version ();
    Usage ();
    return STATUS_SUCCESS;    
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;    
  }
  
  InputConfigFileName   = NULL;
  InputCapsuleModulesDir   = NULL;
  OutputCapsuleFileName = NULL;
  CapsuleFlagsString = NULL;
  if (argc > 0) {
    InputConfigFileName = argv[0];
    argv ++;
    argc --;
  }

  if (argc > 0) {
    InputCapsuleModulesDir = argv[0];
    argv ++;
    argc --;
  }

  if (argc > 0) {
    OutputCapsuleFileName = argv[0];
    argv ++;
    argc --;
  }

  if (argc > 0) {
    CapsuleFlagsString = argv[0];
    argv ++;
    argc --;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  InputConfigFile = NULL;
  OutputCapsuleFile = NULL;
  InputCapsuleImage = NULL;
  ImageBuffer = NULL;
  
  if (CapsuleFlagsString == NULL) {
    Error (NULL, 0, 1003, "Missing option", "CapsuleFlags parameter");
    goto Finish;
  }

  Status = AsciiStringToUint64 (CapsuleFlagsString, TRUE, &CapsuleFlags);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 1002, "Invalid CapsuleFlags parameter", "%s", CapsuleFlagsString);
    goto Finish;
  }

  //
  // Open input config file
  //
  FileName[0] = '\0';
  if (InputConfigFileName == NULL) {
    Error (NULL, 0, 1003, "Missing option", "Input Config file");
    goto Finish;
  }
  strcpy (FileName, InputConfigFileName);
  InputConfigFile = fopen (FileName, "r");
  if (InputConfigFile == NULL) {
    Error (NULL, 0, 1004, "Error opening file", FileName);
    goto Finish;
  }

  //
  // Open output capsule file
  //
  FileName[0] = '\0';
  if (OutputCapsuleFileName == NULL) {
    Error (NULL, 0, 1005, "Missing option", "Output Capsule file");
    goto Finish;
  }
  strcpy (FileName, OutputCapsuleFileName);
  OutputCapsuleFile = fopen (FileName, "wb");
  if (OutputCapsuleFile == NULL) {
    Error (NULL, 0, 1006, "Error opening file", FileName);
    goto Finish;
  }

  //
  // Allocate a buffer to read the input images into
  //
  ImageBuffer = (CHAR8*) malloc (sizeof(CHAR8)*MAX_CAPSULE_PAYLOAD_SIZE);
  if (ImageBuffer == NULL) {
    goto Finish;
  }

  //
  // Initialise data
  //
  SourceOffset = (sizeof (CapsuleHeader)) + ((sizeof (UPDATE_HINT)) * MAX_IMAGES_PER_CAPSULE);
  ImageNumber = 0;
  TotalPayloadImageSize = 0;
  memset (&HintData, 0xFF, sizeof (HintData));
  ZeroMem (ImageBuffer, sizeof(CHAR8)*MAX_CAPSULE_PAYLOAD_SIZE);

  //
  // Fill in the Hint Data
  //
  while(!feof(InputConfigFile) && ImageNumber < (MAX_IMAGES_PER_CAPSULE-1)) {
    ReadLineInStream (InputConfigFile, LineString);
    if (strstr (LineString, BEGIN_STRING) != NULL) {
      do {
        ReadLineInStream (InputConfigFile, LineString);
        //
        // Remove line break
        //
        LineLen = strlen (LineString);
				if (LineLen != 0) {
					if (LineString[LineLen - 1] == '\n') {
						if (LineString[LineLen - 2] == '\r') {
							LineString[LineLen - 2] = '\0';
						}
						LineString[LineLen - 1] = '\0';
					}
				}

        if ((ValueString = strstr (LineString, FILE_NAME_STRING)) != NULL) {
          //
          // Get the input capsule image file name
          //
          ValueString = ValueString + strlen (FILE_NAME_STRING);
          FileName[0] = '\0';
          if (InputCapsuleModulesDir != NULL) {
            strcpy (FileName, InputCapsuleModulesDir);
            if (FileName[strlen (FileName) - 1] != '/') {
              strcat (FileName, "/");
            }
          }
          strcat (FileName, ValueString);

          //
          // Open the input capsule image file
          //
          InputCapsuleImage = fopen (FileName, "rb");
          if (InputCapsuleImage == NULL) {
            Error (NULL, 0, 1007, "Error opening file", FileName);
            goto Finish;
          }

          //
          // Get size of input image
          //
          fseek(InputCapsuleImage, 0, SEEK_END);
          InputCapsuleImageSize = (UINT32) ftell(InputCapsuleImage);
          rewind(InputCapsuleImage);

          //
          // Calculate any padding required to align image to SPI_BLOCK_SIZE
          //
          if ((InputCapsuleImageSize & (SPI_BLOCK_SIZE - 1)) != 0) {
            InputCapsuleImagePadSize = SPI_BLOCK_SIZE - (InputCapsuleImageSize & (SPI_BLOCK_SIZE - 1));
          } else {
            InputCapsuleImagePadSize = 0;
          }

          //
          // Read input image into buffer
          //
          if ((TotalPayloadImageSize + InputCapsuleImageSize) >= MAX_CAPSULE_PAYLOAD_SIZE) {
            Error (NULL, 0, 1008, "Capsule Payload size is", " > MAX_CAPSULE_PAYLOAD_SIZE");
            goto Finish;
          }
          ReadFileSize = (UINT32) fread((CHAR8 *)(ImageBuffer+TotalPayloadImageSize), 1, InputCapsuleImageSize, InputCapsuleImage);
          if (ReadFileSize != InputCapsuleImageSize) {
            Error (NULL, 0, 1009, "Error reading input file", ".");
            goto Finish;
          }

          HintData[ImageNumber].Size = (UINT32) (InputCapsuleImageSize + InputCapsuleImagePadSize);
          TotalPayloadImageSize += (InputCapsuleImageSize + InputCapsuleImagePadSize);
        } else if ((ValueString = strstr (LineString, START_ADDRESS_STRING)) != NULL) {
          //
          // Get the Start address value
          //
          ValueString = ValueString + strlen (START_ADDRESS_STRING);
          Status = AsciiStringToUint64 (ValueString, TRUE, &Value64);
          if (EFI_ERROR (Status)) {
            Error (NULL, 0, 1010, "Invalid parameter", "%s%s", START_ADDRESS_STRING, ValueString);
            goto Finish;
          }
          HintData[ImageNumber].TargetAddr = (UINT32) Value64;
        }
      } while ((strstr (LineString, END_STRING) == NULL) && !feof(InputConfigFile));
      
      if (strstr (LineString, END_STRING) == NULL) {
        Status = EFI_INVALID_PARAMETER;
        Error (NULL, 0, 1011, "Invalid parameter", "no %s match %s", END_STRING, BEGIN_STRING);
        goto Finish;
      }
      
      if ((HintData[ImageNumber].Size == 0) || ((HintData[ImageNumber].Size & 0xFFF) != 0)) {
        Status = EFI_INVALID_PARAMETER;
        Error (NULL, 0, 1012, "Invalid parameter", "%s is missing or is not block (4KB) aligned", FILE_NAME_STRING);
        goto Finish;
      }
      if (HintData[ImageNumber].TargetAddr == 0) {
        Status = EFI_INVALID_PARAMETER;
        Error (NULL, 0, 1013, "Invalid parameter", "%s is missing", START_ADDRESS_STRING);
        goto Finish;
      }
      HintData[ImageNumber].SourceOffset = SourceOffset;
      SourceOffset += HintData[ImageNumber].Size;
      ImageNumber++;
    }
  }
  ZeroMem (&HintData[ImageNumber], sizeof (UPDATE_HINT));
  ImageNumber++;
  
  //
  // Fill in the Capsule Header
  //
  gCapsuleGuid = "D400D1E4-A314-442B-89ED-A92E4C8197CB";
  ZeroMem (&CapsuleHeader, sizeof (CapsuleHeader));
  Status = StringToGuid (gCapsuleGuid, &CapsuleHeader.CapsuleGuid);
  CapsuleHeader.HeaderSize = (UINT32) sizeof (CapsuleHeader);
  CapsuleHeader.Flags = (UINT32)CapsuleFlags;
  CapsuleHeader.CapsuleImageSize = (UINT32) SourceOffset;

  //
  // Write to output file
  //
  fwrite (&CapsuleHeader, 1, sizeof (CapsuleHeader), OutputCapsuleFile);
  fwrite (&HintData, 1, (sizeof (UPDATE_HINT)) * MAX_IMAGES_PER_CAPSULE, OutputCapsuleFile);
  fwrite (ImageBuffer, 1, TotalPayloadImageSize, OutputCapsuleFile);

Finish:
  if (InputConfigFile != NULL) {
    fclose (InputConfigFile);
  }
  
  if (OutputCapsuleFile != NULL) {
    fclose (OutputCapsuleFile);
  }

  if (InputCapsuleImage != NULL) {
    fclose (InputCapsuleImage);
  }

  if (ImageBuffer != NULL) {
    free (ImageBuffer);
  }

  //
  // If any errors were reported via the standard error reporting
  // routines, then the status has been saved. Get the value and
  // return it to the caller.
  //
  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}
