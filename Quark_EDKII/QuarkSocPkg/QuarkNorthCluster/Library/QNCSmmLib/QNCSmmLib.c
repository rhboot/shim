/** @file
  QNC Smm Library Services that implements both S/W SMI generation and detection. 

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


#include <Base.h>
#include <IntelQNCRegs.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/QNCAccessLib.h>

#define BOOT_SERVICE_SOFTWARE_SMI_DATA          0
#define RUNTIME_SOFTWARE_SMI_DATA               1

/**
  Triggers a run time or boot time SMI.  

  This function triggers a software SMM interrupt and set the APMC status with an 8-bit Data.

  @param  Data                 The value to set the APMC status.

**/
VOID
InternalTriggerSmi (
  IN UINT8                     Data
  )
{
  UINT16        PM1BLK_Base;
  UINT16        GPE0BLK_Base;
  UINT32        NewValue;

  //
  // Get PM1BLK_Base & GPE0BLK_Base
  //
  PM1BLK_Base = PcdGet16 (PcdPm1blkIoBaseAddress);
  GPE0BLK_Base = (UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF);


  //
  // Enable APM SMI
  //
  IoOr32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIE), B_QNC_GPE0BLK_SMIE_APM);

  //
  // Enable SMI globally
  //
  NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  NewValue |= SMI_EN;
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);

  //
  // Set APM_STS
  //
  IoWrite8 (PcdGet16 (PcdSmmDataPort), Data);

  //
  // Generate the APM SMI
  //
  IoWrite8 (PcdGet16 (PcdSmmActivationPort), PcdGet8 (PcdSmmActivationData));

  //
  // Clear the APM SMI Status Bit
  //
  IoWrite32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_APM);

  //
  // Set the EOS Bit
  //
  IoOr32 ((GPE0BLK_Base + R_QNC_GPE0BLK_SMIS), B_QNC_GPE0BLK_SMIS_EOS);
}


/**
  Triggers an SMI at boot time.  

  This function triggers a software SMM interrupt at boot time.

**/
VOID
EFIAPI
TriggerBootServiceSoftwareSmi (
  VOID
  )
{
  InternalTriggerSmi (BOOT_SERVICE_SOFTWARE_SMI_DATA);
}


/**
  Triggers an SMI at run time.  

  This function triggers a software SMM interrupt at run time.

**/
VOID
EFIAPI
TriggerRuntimeSoftwareSmi (
  VOID
  )
{
  InternalTriggerSmi (RUNTIME_SOFTWARE_SMI_DATA);
}


/**
  Gets the software SMI data.  

  This function tests if a software SMM interrupt happens. If a software SMI happens, 
  it retrieves the SMM data and returns it as a non-negative value; otherwise a negative
  value is returned. 

  @return Data                 The data retrieved from SMM data port in case of a software SMI;
                               otherwise a negative value.

**/
INTN
InternalGetSwSmiData (
  VOID
  )
{
  UINT8                        SmiStatus;
  UINT8                        Data; 

  SmiStatus = IoRead8 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS);
  if (((SmiStatus & B_QNC_GPE0BLK_SMIS_APM) != 0) &&
       (IoRead8 (PcdGet16 (PcdSmmActivationPort)) == PcdGet8 (PcdSmmActivationData))) {
    Data = IoRead8 (PcdGet16 (PcdSmmDataPort));
    return (INTN)(UINTN)Data; 
  } 
  
  return -1;
}  


/**
  Test if a boot time software SMI happened.  

  This function tests if a software SMM interrupt happened. If a software SMM interrupt happened and
  it was triggered at boot time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE   A software SMI triggered at boot time happened.
  @retval FLASE  No software SMI happened or the software SMI was triggered at run time.

**/
BOOLEAN
EFIAPI
IsBootServiceSoftwareSmi (
  VOID
  )
{
  return (BOOLEAN) (InternalGetSwSmiData () == BOOT_SERVICE_SOFTWARE_SMI_DATA);
}


/**
  Test if a run time software SMI happened.  

  This function tests if a software SMM interrupt happened. If a software SMM interrupt happened and
  it was triggered at run time, it returns TRUE. Otherwise, it returns FALSE.

  @retval TRUE   A software SMI triggered at run time happened.
  @retval FLASE  No software SMI happened or the software SMI was triggered at boot time.

**/
BOOLEAN
EFIAPI
IsRuntimeSoftwareSmi (
  VOID
  )
{
  return (BOOLEAN) (InternalGetSwSmiData () == RUNTIME_SOFTWARE_SMI_DATA);
}



/**

  Clear APM SMI Status Bit; Set the EOS bit. 
  
**/
VOID
EFIAPI
ClearSmi (
  VOID
  )
{

  UINT16                       GPE0BLK_Base;

  //
  // Get GpeBase
  //
  GPE0BLK_Base = (UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF);

  //
  // Clear the APM SMI Status Bit
  //
  IoOr16 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIS, B_QNC_GPE0BLK_SMIS_APM);

  //
  // Set the EOS Bit
  //
  IoOr32 (GPE0BLK_Base + R_QNC_GPE0BLK_SMIS, B_QNC_GPE0BLK_SMIS_EOS);
}

