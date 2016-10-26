/**@file
  Header file for SMM Access Driver.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

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

#ifndef _SMM_ACCESS_DRIVER_H
#define _SMM_ACCESS_DRIVER_H

#include <PiDxe.h>
#include <IndustryStandard/Pci.h>

#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/PciRootBridgeIo.h>

//
// Driver Consumed GUID Prototypes
//
#include <Guid/SmramMemoryReserve.h>

//
// Driver produced protocol
//
#include <Protocol/SmmAccess2.h>

#include <Library/IntelQNCLib.h>
#include <QNCAccess.h>

#define MAX_CPU_SOCKET			1
#define MAX_SMRAM_RANGES    4

//
// Private data structure
//
#define  SMM_ACCESS_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('i', 's', 'm', 'a')

typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       Handle;
  EFI_SMM_ACCESS2_PROTOCOL          SmmAccess;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;  
  UINTN                            NumberRegions;
  EFI_SMRAM_DESCRIPTOR             SmramDesc[MAX_SMRAM_RANGES];
  UINT8                            TsegSize;
  UINT8                            MaxBusNumber;
  UINT8                            SocketPopulated[MAX_CPU_SOCKET];
  UINT64                           SMMRegionState;  
  UINT8                            ActualNLIioBusNumber;
} SMM_ACCESS_PRIVATE_DATA;


#define SMM_ACCESS_PRIVATE_DATA_FROM_THIS(a) \
  CR ( \
  a, \
  SMM_ACCESS_PRIVATE_DATA, \
  SmmAccess, \
  SMM_ACCESS_PRIVATE_DATA_SIGNATURE \
  )


//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
SmmAccessDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
  
Routine Description:

  This is the standard EFI driver point that detects
  whether there is an proper chipset in the system
  and if so, installs an SMM Access Protocol.

Arguments:

  ImageHandle  -  Handle for the image of this driver.
  SystemTable  -  Pointer to the EFI System Table.

Returns:

  EFI_SUCCESS      -  Protocol successfully started and installed.
  EFI_UNSUPPORTED  -  Protocol can't be started.

--*/
;

EFI_STATUS
EFIAPI
Open (
  IN EFI_SMM_ACCESS2_PROTOCOL *This
  )
/*++
  
Routine Description:

  This routine accepts a request to "open" a region of SMRAM.  The
  region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all boot-service
  and SMM agents.

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Open.

Returns:

  EFI_SUCCESS            -  The region was successfully opened.
  EFI_DEVICE_ERROR       -  The region could not be opened because locked by
                            chipset.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.

--*/
;

EFI_STATUS
EFIAPI
Close (
  IN EFI_SMM_ACCESS2_PROTOCOL *This
  )
/*++
  
Routine Description:

  This routine accepts a request to "close" a region of SMRAM.  This is valid for 
  compatible SMRAM region.

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Close.

Returns:

  EFI_SUCCESS            -  The region was successfully closed.
  EFI_DEVICE_ERROR       -  The region could not be closed because locked by
                            chipset.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.
  
--*/
;

EFI_STATUS
EFIAPI
Lock (
  IN EFI_SMM_ACCESS2_PROTOCOL *This
  )
/*++

Routine Description: 

  This routine accepts a request to "lock" SMRAM.  The
  region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to BS state..

Arguments:

  This             -  Pointer to the SMM Access Interface.
  DescriptorIndex  -  Region of SMRAM to Lock.

Returns:

  EFI_SUCCESS            -  The region was successfully locked.
  EFI_DEVICE_ERROR       -  The region could not be locked because at least
                            one range is still open.
  EFI_INVALID_PARAMETER  -  The descriptor index was out of bounds.
  
--*/
;

EFI_STATUS
EFIAPI
GetCapabilities (
  IN CONST EFI_SMM_ACCESS2_PROTOCOL     *This,
  IN OUT UINTN                   *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR    *SmramMap
  )
/*++
  
Routine Description:

  This routine services a user request to discover the SMRAM
  capabilities of this platform.  This will report the possible
  ranges that are possible for SMRAM access, based upon the 
  memory controller capabilities.

Arguments:

  This          -  Pointer to the SMRAM Access Interface.
  SmramMapSize  -  Pointer to the variable containing size of the
                   buffer to contain the description information.
  SmramMap      -  Buffer containing the data describing the Smram
                   region descriptors.

Returns:

  EFI_BUFFER_TOO_SMALL  -  The user did not provide a sufficient buffer.
  EFI_SUCCESS           -  The user provided a sufficiently-sized buffer.

--*/
;
VOID
SyncRegionState2SmramDesc(
  IN BOOLEAN  OrAnd,
  IN UINT64    Value
  );

#endif
