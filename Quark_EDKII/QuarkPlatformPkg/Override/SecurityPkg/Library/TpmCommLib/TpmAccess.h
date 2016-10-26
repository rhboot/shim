/** @file
  The intenal header file for TpmAccess routines.

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

#ifndef _TPMACCESS_HEADER_H_
#define _TPMACCESS_HEADER_H_

#include <Guid/PlatformInfo.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Protocol/PlatformType.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/I2CHc.h>
#include <Guid/HobList.h>
#include <Platform.h>


// Default TPM (Infineon SLB9645) I2C Slave Device Address on Crosshill board.
#define TPM_I2C_SLAVE_DEVICE_ADDRESS             0x20

// Default TPM MMIO Base Address
#define TPM_MMIO_BASE_ADDRESS                    0xfed40000

// Default TCG MMIO mapped registers (TCG PC CLient TPM Interface Specification).
#define TPM_ACCESS_0_ADDRESS_DEFAULT             0x0
#define TPM_STS_0_ADDRESS_DEFAULT                0x18
#define TPM_BURST0_COUNT_0_DEFAULT               0x19
#define TPM_BURST1_COUNT_0_DEFAULT               0x1A
#define TPM_DATA_FIFO_0_DEFAULT                  0x24

// Default Infineon SLB9645 TPM I2C mapped registers (SLB9645 I2C Comm. Protocol Application Note).
#define INFINEON_TPM_ACCESS_0_ADDRESS_DEFAULT    0x0
#define INFINEON_TPM_STS_0_ADDRESS_DEFAULT       0x01
#define INFINEON_TPM_BURST0_COUNT_0_DEFAULT      0x02
#define INFINEON_TPM_BURST1_COUNT_0_DEFAULT      0x03
#define INFINEON_TPM_DATA_FIFO_0_ADDRESS_DEFAULT 0x05
#define INFINEON_TPM_DID_VID_0_DEFAULT           0x09

// Max. retry count for read transfers (as recommended by Infineon).
#define READ_RETRY 3

// Guard time of 250us between I2C read and next I2C write transfer (as recommended by Infineon).
#define GUARD_TIME 250


/**
  Maps a standard TCG MMIO address (offset to Locality 0)
  to Infineon SLB9645 TPM I2C Address.

  @param  MMIO Address to map.

  @return Mapped Infineon SLB9645 TPM I2C Address.

**/
UINTN
MapMmioAddr2InfI2CAddr (
  IN  UINTN  Address
  );


/**
  Writes single byte data to TPM specified by MMIO address.

  Write access to TPM is MMIO or I2C (based on platform type).

  @param  Address The register to write.
  @param  Data    The data to write to the register.

**/
VOID
TpmWriteByte (
  IN UINTN  Address,
  IN UINT8  Data
  );


/**
  Reads single byte data from TPM specified by MMIO address.

  Read access to TPM is via MMIO or I2C (based on platform type).
  
  Due to stability issues when using I2C combined write/read transfers (with
  RESTART) to TPM (specifically read from status register), a single write is 
  performed followed by single read (with STOP condition in between).

  @param  Address of the MMIO mapped register to read.

  @return The value read.

**/
UINT8
EFIAPI
TpmReadByte (
  IN  UINTN  Address
  );

#endif
