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

#define UTILITY_NAME            "DummySignTool"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   1

#define MAX_IMAGE_SIZE 0xA00000

#pragma pack(1)
typedef struct {
  UINT8     Reserved[1024];
} CSH_DUMMY_HEADER;
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
  fprintf (stdout, "\nUsage: %s InputFileName\n\n", UTILITY_NAME);
  
  //
  // Copyright declaration
  // 
  fprintf (stdout, "Copyright (c) 2013, Intel Corporation. All rights reserved.\n\n");
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
  CHAR8                   *ImageBuffer;
  CHAR8                   *InputImageFileName;
  CHAR8                   *OutputImageFileName;
  FILE                    *InputImageFile;
  FILE                    *OutputImageFile;
  CHAR8                   FileName[_MAX_PATH];
  CSH_DUMMY_HEADER        CshDummyHeader;
  UINT32                  InputImageSize;
  UINT32                  ReadFileSize;


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
  
  InputImageFileName   = NULL;
  OutputImageFileName = NULL;
  if (argc > 0) {
    InputImageFileName = argv[0];
    argv ++;
    argc --;
  }

  if (argc > 0) {
    OutputImageFileName = argv[0];
    argv ++;
    argc --;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  InputImageFile = NULL;
  OutputImageFile = NULL;
  ImageBuffer = NULL;
  
  //
  // Open the input image file
  //
  FileName[0] = '\0';
  if (InputImageFileName == NULL) {
    Error (NULL, 0, 1003, "Missing option", "Input Image file");
    goto Finish;
  }
  strcpy (FileName, InputImageFileName);
  InputImageFile = fopen (FileName, "rb");
  if (InputImageFile == NULL) {
    Error (NULL, 0, 1007, "Error opening file", FileName);
    goto Finish;
  }

  //
  // Open output image file
  //
  FileName[0] = '\0';
  if (OutputImageFileName == NULL) {
    Error (NULL, 0, 1005, "Missing option", "Output Image file");
    goto Finish;
  }
  strcpy (FileName, OutputImageFileName);
  OutputImageFile = fopen (FileName, "wb");
  if (OutputImageFile == NULL) {
    Error (NULL, 0, 1006, "Error opening file", FileName);
    goto Finish;
  }

  //
  // Get size of input image
  //
  fseek(InputImageFile, 0, SEEK_END);
  InputImageSize = (UINT32) ftell(InputImageFile);
  rewind(InputImageFile);

  //
  // Allocate a buffer to read the input images into
  //
  ImageBuffer = (CHAR8*) malloc (sizeof(CHAR8)*MAX_IMAGE_SIZE);
  if (ImageBuffer == NULL) {
    goto Finish;
  }
  ZeroMem (ImageBuffer, sizeof(CHAR8)*MAX_IMAGE_SIZE);

  //
  // Initialise Data
  //
  ZeroMem (&CshDummyHeader, sizeof (CshDummyHeader));

  //
  // Read input image into buffer
  //
  if (InputImageSize >= MAX_IMAGE_SIZE) {
    Error (NULL, 0, 1008, "Input image size is", " > MAX_IMAGE_SIZE");
    goto Finish;
  }
  ReadFileSize = (UINT32) fread((CHAR8 *)ImageBuffer, 1, InputImageSize, InputImageFile);
  if (ReadFileSize != InputImageSize) {
    Error (NULL, 0, 1009, "Error reading input file", ".");
    goto Finish;
  }

  //
  // Write to output file
  //
  fwrite (&CshDummyHeader, 1, sizeof (CshDummyHeader), OutputImageFile);
  fwrite (ImageBuffer, 1, InputImageSize, OutputImageFile);

Finish:
  if (InputImageFile != NULL) {
    fclose (InputImageFile);
  }
  
  if (OutputImageFile != NULL) {
    fclose (OutputImageFile);
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
