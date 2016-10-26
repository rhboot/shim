/** @file
  Application to test the UEFI capsule runtime service functionality.

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

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ShellLib.h>
#include <Protocol/FirmwareVolumeBlockSecurity.h>

#define MAJOR_VERSION   1
#define MINOR_VERSION   3

//
// Define how many block descriptors we want to test with.
//
UINTN   NumberOfDescriptors = 1;

STATIC CONST SHELL_PARAM_ITEM CapsuleParamList[] = {
  {L"-?", TypeFlag},
  {L"-h", TypeFlag},
  {L"-H", TypeFlag},
  {L"-v", TypeFlag},
  {L"-V", TypeFlag},
  {NULL, TypeMax}
  };

/**
   Display current version.
**/
VOID
ShowVersion (
  )
{
  ShellPrintEx (-1, -1, L"CapsuleApp Version %d.%02d\n", MAJOR_VERSION, MINOR_VERSION);
}

/**
   Display Usage and Help information.
**/
VOID
ShowHelp (
  )
{
  ShellPrintEx (-1, -1, L"CapsuleApp:  usage\n  CapsuleApp [capsule filename]\n");
}

/** 
  Update Capsule image.
  
  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.
  
  @retval EFI_SUCCESS            Command completed successfully.
  @retval EFI_INVALID_PARAMETER  Command usage error.
  @retval EFI_NOT_FOUND          The input file can't be found.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *ParamPackage;
  CHAR16              *ParamProblem;
  EFI_SHELL_FILE_INFO *ListHead;
  EFI_SHELL_FILE_INFO *Node;
  SHELL_FILE_HANDLE   SourceHandle;
  UINTN               SignedFileSize;
  UINTN               FileSize;
  UINT8               *SignedCapsuleBuffer;
  UINT8               *CapsuleBuffer;
  CHAR16              *CapsuleFileName;
  CHAR16              *InputFileName;

  EFI_CAPSULE_BLOCK_DESCRIPTOR            *BlockDescriptors1;
  EFI_CAPSULE_BLOCK_DESCRIPTOR            *BlockDescriptors2;
  EFI_CAPSULE_BLOCK_DESCRIPTOR            *TempBlockPtr;
  UINT8                                   *TempDataPtr;
  UINTN                                   SizeLeft;
  UINTN                                   Size;
  INT32                                   Count;
  INT32                                   Number;
  EFI_CAPSULE_HEADER                      *CapsuleHeaderArray[2];
  UINT64                                  MaxCapsuleSize;
  EFI_RESET_TYPE                          ResetType;
  EFI_FILE_INFO                           *FileInfo;
  FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL *FvbSecurityProtocol;

  ListHead            = NULL;
  ParamProblem        = NULL;
  SignedCapsuleBuffer = NULL;
  CapsuleBuffer       = NULL;
  CapsuleFileName     = NULL;
  BlockDescriptors1   = NULL;
  BlockDescriptors2   = NULL;
  SignedFileSize      = 0;
  FileSize            = 0;
  FileInfo            = NULL;
  SourceHandle        = NULL;
  ParamPackage        = NULL;

  //
  // Locate Fvb Security Protocol for Capsule File Verification.
  //
  Status = gBS->LocateProtocol(
             &gFirmwareVolumeBlockSecurityGuid,
             NULL,
             (VOID**) &FvbSecurityProtocol
             );
  if (EFI_ERROR(Status)) {
    FvbSecurityProtocol = NULL;
  } else {
    ASSERT (FvbSecurityProtocol != NULL);
    ASSERT (FvbSecurityProtocol->SecurityAuthenticateImage != NULL);
  }

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (CapsuleParamList, &ParamPackage, &ParamProblem, TRUE);
  if (EFI_ERROR(Status)) {
    if (ParamProblem != NULL) {
      ShellPrintEx(-1, -1, L"CapsuleApp: %EError. %NThe argument '%B%s%N' is invalid.\n", ParamProblem);
      FreePool(ParamProblem);
    } else {
      ShellPrintEx(-1, -1, L"CapsuleApp: %EError. %NThe input parameters are not recognized.\n");
    }
    Status = EFI_INVALID_PARAMETER;
  } else {
    //
    // check for "-?" help information, and for "-v" for version inforamtion.
    //
    if (ShellCommandLineGetFlag (ParamPackage, L"-?")  ||
        ShellCommandLineGetFlag (ParamPackage, L"-h")  ||
        ShellCommandLineGetFlag (ParamPackage, L"-H")) {
      ShowHelp();
      goto Done;
    } else if (ShellCommandLineGetFlag (ParamPackage, L"-v") ||
               ShellCommandLineGetFlag (ParamPackage, L"-V")) {
      ShowVersion();
      goto Done;
    } else {
      //
      // Only support single parameter as the input file name. 
      //
      InputFileName = (CHAR16 *) ShellCommandLineGetRawValue(ParamPackage, 1);
      if (InputFileName == NULL) {
        //
        // we can't get any  for file name
        //
        ShellPrintEx(-1, -1, L"CapsuleApp: %EError. %NToo few arguments specified.\n");
        Status = EFI_INVALID_PARAMETER;
      } else if (ShellCommandLineGetRawValue(ParamPackage, 2) != NULL) {
        //
        // we get more than one input parameters for file name
        //
        ShellPrintEx(-1, -1, L"CapsuleApp: %EError. %NToo many arguments specified.\n");
        Status = EFI_INVALID_PARAMETER;
      } else {
        //
        // Read the input file. If more than one is found, the first one will be used.
        //
        Status = ShellOpenFileMetaArg(InputFileName, EFI_FILE_MODE_READ, &ListHead);
        if (!EFI_ERROR(Status)) {
          for (Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&ListHead->Link)
              ; !IsNull(&ListHead->Link, &Node->Link)
              ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&ListHead->Link, &Node->Link)
              ) {
            
            if (ShellIsFile(Node->FullName) != EFI_SUCCESS) {
              continue;
            }

            //
            // open source file
            //
            Status = ShellOpenFileByName(Node->FullName, &SourceHandle, EFI_FILE_MODE_READ, 0);
            ASSERT_EFI_ERROR(Status);
            
            //
            // get file size
            //
            FileInfo = ShellGetFileInfo (SourceHandle);
            SignedFileSize = (UINTN) FileInfo->FileSize;
            FreePool (FileInfo);

            //
            // read data from file
            //
            SignedCapsuleBuffer = AllocateZeroPool(SignedFileSize);
            Status        = ShellReadFile (SourceHandle, &SignedFileSize, SignedCapsuleBuffer);
            if (!EFI_ERROR (Status)) {
              //
              // Get Capsule file name
              //
              CapsuleFileName = AllocateZeroPool (StrSize (Node->FullName));
              ASSERT (CapsuleFileName != NULL);
              StrCpy (CapsuleFileName, Node->FullName);
            } else {
              FreePool (SignedCapsuleBuffer);
              SignedCapsuleBuffer = NULL;
            }

            //
            // close source file.
            //
            if (SourceHandle != NULL) {
              ShellCloseFile(&SourceHandle);  // map to ShellOpenFileByName()
              SourceHandle = NULL;
            }
            
            //
            // Capsule image is got. First capsule file is used. 
            //
            if (SignedCapsuleBuffer != NULL && SignedFileSize != 0) {
              Status = EFI_SUCCESS;
              break;
            }
          }

          //
          // Free ListHead
          // map to ShellOpenFileMetaArg
          //
          ShellCloseFileMetaArg(&ListHead);
        } else {
          //
          // no files are found.
          //
          ShellPrintEx(-1, -1, L"CapsuleApp: File '%B%s%N' was not found.\n", InputFileName);
          Status = EFI_NOT_FOUND;
        }
      }
    }

    //
    // free the command line package
    // map to ShellCommandLineParse
    //
    ShellCommandLineFreeVarList (ParamPackage);
    ParamPackage = NULL;
  }

  //
  // Additional check for the input parameter.
  //
  if (EFI_ERROR(Status)) {
    goto Done;
  }
  
  if (SignedCapsuleBuffer == NULL || SignedFileSize == 0) {
    ShellPrintEx(-1, -1, L"CapsuleApp: no capsule image is found.\n");
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Allocate memory for the descriptors.
  //
  if (NumberOfDescriptors == 1) {
    Count = 2;
  } else {
    Count = (INT32)(NumberOfDescriptors + 2) / 2;
  }

  Size               = Count * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
  BlockDescriptors1  = AllocateRuntimeZeroPool (Size);
  ASSERT (BlockDescriptors1 != NULL);
  if (EFI_ERROR (Status)) {
    ShellPrintEx(-1, -1, L"CapsuleApp: failed to allocate memory for descriptors\n");
    goto Done;
  } else {
    CapsuleBuffer = SignedCapsuleBuffer + FixedPcdGet32 (PcdFvSecurityHeaderSize);
    if (FvbSecurityProtocol != NULL) {
      ShellPrintEx(-1, -1, L"CapsuleApp: SecurityAuthenticateImage 0x%X\n", (UINTN) CapsuleBuffer);
      Status = FvbSecurityProtocol->SecurityAuthenticateImage ((VOID *) CapsuleBuffer);
      if (EFI_ERROR(Status)) {
        ShellPrintEx(-1, -1, L"CapsuleApp: Rom SecurityAuthenticateImage failed\n");
        goto Done;
      }
    } else {
      ShellPrintEx(-1, -1, L"CapsuleApp: No Authenticate Image Service found on this platform\n");
    }
    FileSize = SignedFileSize - FixedPcdGet32 (PcdFvSecurityHeaderSize);
    ShellPrintEx(-1, -1, L"CapsuleApp: creating capsule descriptors at 0x%X\n", (UINTN) BlockDescriptors1);
    ShellPrintEx(-1, -1, L"CapsuleApp: capsule data starts          at 0x%X with size 0x%X\n", (UINTN) CapsuleBuffer, FileSize);
  }

  //
  // Fill them in
  //
  TempBlockPtr  = BlockDescriptors1;
  TempDataPtr   = CapsuleBuffer;
  SizeLeft      = FileSize;
  for (Number = 0; Number < Count - 1; Number++) {
    //
    // Divide remaining data in half
    //
    if (NumberOfDescriptors != 1) {
      if (SizeLeft == 1) {
        Size = 1;
      } else {
        Size = SizeLeft / 2;
      }
    } else {
      Size = SizeLeft;
    }
    TempBlockPtr->Union.DataBlock    = (UINTN)TempDataPtr;
    TempBlockPtr->Length  = Size;
    ShellPrintEx(-1, -1, L"CapsuleApp: capsule block/size              0x%X/0x%X\n", (UINTN) TempDataPtr, Size);
    SizeLeft -= Size;
    TempDataPtr += Size;
    TempBlockPtr++;
  }
  //
  // Allocate the second list, point the first block's last entry to point
  // to this one, and fill this one in. Worst case is that the previous
  // list only had one element that pointed here, so we need at least two
  // elements -- one to point to all the data, another to terminate the list.
  //
  if (NumberOfDescriptors != 1) {
    Count = (INT32)(NumberOfDescriptors + 2) - Count;
    if (Count == 1) {
      Count++;
    }

    Size              = Count * sizeof (EFI_CAPSULE_BLOCK_DESCRIPTOR);
    BlockDescriptors2 = AllocateRuntimeZeroPool (Size);
    ASSERT (BlockDescriptors2 != NULL);
    if (EFI_ERROR (Status)) {
      ShellPrintEx(-1, -1, L"CapsuleApp: failed to allocate memory for descriptors\n");
      return Status;
    }
    //
    // Point the first list's last element to point to this second list.
    //
    TempBlockPtr->Union.ContinuationPointer   = (UINTN) BlockDescriptors2;
                        
    TempBlockPtr->Length  = 0;
    TempBlockPtr = BlockDescriptors2;
    for (Number = 0; Number < Count - 1; Number++) {
      //
      // If second-to-last one, then dump rest to this element
      //
      if (Number == (Count - 2)) {
        Size = SizeLeft;
      } else {
        //
        // Divide remaining data in half
        //
        if (SizeLeft == 1) {
          Size = 1;
        } else {
          Size = SizeLeft / 2;
        }
      }

      TempBlockPtr->Union.DataBlock    = (UINTN)TempDataPtr;
      TempBlockPtr->Length  = Size;
      ShellPrintEx(-1, -1, L"CapsuleApp: capsule block/size              0x%X/0x%X\n", (UINTN) TempDataPtr, Size);
      SizeLeft -= Size;
      TempDataPtr += Size;
      TempBlockPtr++;
    }
  }
  //
  // Null-terminate.
  //
  TempBlockPtr->Union.ContinuationPointer    = (UINTN)NULL;
  TempBlockPtr->Length  = 0;

  //
  // Call the runtime service capsule. 
  //
  CapsuleHeaderArray[0] = (EFI_CAPSULE_HEADER *) CapsuleBuffer;
  CapsuleHeaderArray[1] = NULL;

  //
  // Inquire platform capability of UpdateCapsule.
  //
  Status = gRT->QueryCapsuleCapabilities(CapsuleHeaderArray,1,&MaxCapsuleSize,&ResetType);
  if (Status == EFI_UNSUPPORTED) {
    ShellPrintEx(-1, -1, L"CapsuleApp: %s file is not recognized.\n", CapsuleFileName);
    goto Done;
  }
  if (EFI_ERROR(Status)) {
    ShellPrintEx(-1, -1, L"CapsuleApp: failed to query capsule capability\n");
    goto Done;
  }
  if (SignedFileSize > MaxCapsuleSize) {
    ShellPrintEx(-1, -1, L"CapsuleApp: capsule is too large to update\n");
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  //
  // Check whether the input capsule image has the flag of persist across system reset. 
  //
  if ((CapsuleHeaderArray[0]->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
    Status = gRT->UpdateCapsule(CapsuleHeaderArray,1,(UINTN) BlockDescriptors1);
    if (Status != EFI_SUCCESS) {
      ShellPrintEx(-1, -1, L"CapsuleApp: failed to update capsule\n");
      goto Done;
    }
    //
    // For capsule who has reset flag, after calling UpdateCapsule service,triger a
    // system reset to process capsule persist across a system reset.
    //
    gRT->ResetSystem (ResetType, EFI_SUCCESS, 0, NULL);
  } else {
    //
    // For capsule who has no reset flag, only call UpdateCapsule Service without a 
    // system reset. The service will process the capsule immediately.
    //
    Status = gRT->UpdateCapsule(CapsuleHeaderArray,1,(UINTN) BlockDescriptors1);
    if (Status != EFI_SUCCESS) {
      ShellPrintEx(-1, -1, L"CapsuleApp: failed to update capsule\n");
    }
  }
  
  Status = EFI_SUCCESS;

Done:
  //
  // Clean up
  //
  if (ParamPackage != NULL) {
    ShellCommandLineFreeVarList (ParamPackage);
  }

  if (SignedCapsuleBuffer != NULL) {
    FreePool (SignedCapsuleBuffer);
  }

  if (BlockDescriptors1 != NULL) {
    FreePool (BlockDescriptors1);
  }

  if (BlockDescriptors2 != NULL) {
    FreePool (BlockDescriptors2);
  }
  
  if (CapsuleFileName != NULL) {
    FreePool (CapsuleFileName);
  }

  return Status;
}
