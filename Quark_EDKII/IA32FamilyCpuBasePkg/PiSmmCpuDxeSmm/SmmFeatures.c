/** @file
  The CPU specific programming for PiSmmCpuDxeSmm module, such as SMRR.
  Currently below CPUs are supported.

  0x00000590 // Quark

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
#include "SmmFeatures.h"
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/LocalApicLib.h>

#include "PiSmmCpuDxeSmm.h"

//
// The CPUID mapping for Pentium
//
CPUID_MAPPING  mPentiumMap[] = {
  {CPUID_SIGNATURE_QUARK, CPUID_MASK_NO_STEPPING}, // Quark
  };

//
// The CLASS for Pentium
//
CPU_SMM_CLASS mPentiumClass = {
  CpuPentium,
  sizeof(mPentiumMap)/sizeof(mPentiumMap[0]),
  mPentiumMap,
  };
  
//
// This table defines supported CPU class
//
CPU_SMM_CLASS *mCpuClasstable[] = {
  &mPentiumClass,
  };

////////
// Below section is common definition
////////

//
// Assumes UP, or MP with identical feature set
//
CPU_SMM_FEATURE_CONTEXT  mFeatureContext;

CPU_SMM_CLASS            *mThisCpu;
BOOLEAN                  mSmmCodeAccessCheckEnable = FALSE;

/**
  Return if SMRR is supported

  @retval TRUE  SMRR is supported.
  @retval FALSE SMRR is not supported.

**/
BOOLEAN
IsSmrrSupported (
  VOID
  )
{
  UINT64                            MtrrCap;

  MtrrCap = AsmReadMsr64(EFI_MSR_IA32_MTRR_CAP);
  if ((MtrrCap & IA32_MTRR_SMRR_SUPPORT_BIT) == 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

////////
// Below section is definition for CpuPentium
////////
/*
  Initialize SMRR in SMM relocate.

  @param  SmrrBase           The base address SMRR.
  @param  SmrrSize           The size of SMRR.
*/
VOID
PentiumInitSmrr (
  IN UINT32                SmrrBase,
  IN UINT32                SmrrSize
  )
{
  AsmWriteMsr64 (EFI_MSR_PENTIUM_SMRR_PHYS_BASE, SmrrBase);
  AsmWriteMsr64 (EFI_MSR_PENTIUM_SMRR_PHYS_MASK, (~(SmrrSize - 1) & EFI_MSR_SMRR_MASK) | EFI_MSR_SMRR_PHYS_MASK_VALID);
}

/*
  Disable SMRR register when SmmInit replace non-SMM MTRRs.
*/
VOID
PentiumDisableSmrr (
  VOID
  )
{
  AsmWriteMsr64(EFI_MSR_PENTIUM_SMRR_PHYS_MASK, AsmReadMsr64(EFI_MSR_PENTIUM_SMRR_PHYS_MASK) & ~EFI_MSR_SMRR_PHYS_MASK_VALID);
}

/*
  Enable SMRR register when SmmInit restores non-SMM MTRRs.
*/
VOID
PentiumEnableSmrr (
  VOID
  )
{
  AsmWriteMsr64(EFI_MSR_PENTIUM_SMRR_PHYS_MASK, AsmReadMsr64(EFI_MSR_PENTIUM_SMRR_PHYS_MASK) | EFI_MSR_SMRR_PHYS_MASK_VALID);
}

////////
// Below section is definition for the supported class
////////

/**
  This function will return current CPU_SMM_CLASS accroding to CPUID mapping.

  @return The point to current CPU_SMM_CLASS

**/
CPU_SMM_CLASS *
GetCpuFamily (
  VOID
  )
{
  UINT32         ClassIndex;
  UINT32         Index;
  UINT32         Count;
  CPUID_MAPPING  *CpuMapping;
  UINT32         RegEax;

  AsmCpuid (EFI_CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);
  for (ClassIndex = 0; ClassIndex < sizeof(mCpuClasstable)/sizeof(mCpuClasstable[0]); ClassIndex++) {
    CpuMapping = mCpuClasstable[ClassIndex]->MappingTable;
    Count = mCpuClasstable[ClassIndex]->MappingCount;
    for (Index = 0; Index < Count; Index++) {
      if ((CpuMapping[Index].Signature & CpuMapping[Index].Mask) == (RegEax & CpuMapping[Index].Mask)) {
        return mCpuClasstable[ClassIndex];
      }
    }
  }

  // Not found!!! Should not happen
  ASSERT (FALSE);
  return NULL;
}

////////
// Below section is external function
////////

/**
  Disable SMRR register when SmmInit set SMM MTRRs.
**/
VOID
DisableSmrr (
  VOID
  )
{
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
    if (mFeatureContext.SmrrEnabled) {
      PentiumDisableSmrr ();
    }
    return ;
  default:
    return ;
  }
}

/**
  Enable SMRR register when SmmInit restore non SMM MTRRs.
**/
VOID
ReenableSmrr (
  VOID
  )
{
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
    if (mFeatureContext.SmrrEnabled) {
      PentiumEnableSmrr ();
    }
    return ;
  default:
    return ;
  }
}

/**
  Return if it is needed to configure MTRR to set TSEG cacheability.

  @retval  TRUE  - we need configure MTRR
  @retval  FALSE - we do not need configure MTRR
**/
BOOLEAN
NeedConfigureMtrrs (
  VOID
  )
{
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
  default:
    return TRUE;
  }
}

/**
  Processor specific hook point at each SMM entry.

  @param  CpuIndex    The index of the cpu which need to check.

**/
VOID
SmmRendezvousEntry (
  IN UINTN  CpuIndex
  )
{
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
  default:
    return ;
  }
}

/**
  Processor specific hook point at each SMM exit.

  @param  CpuIndex    The index of the cpu which need to check.
**/
VOID
SmmRendezvousExit (
  IN UINTN  CpuIndex
  )
{
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
  default:
    return ;
  }
}

/**
  Initialize SMRR context in SMM Init.
**/
VOID
InitializeSmmMtrrManager (
  VOID
  )
{
  mThisCpu = GetCpuFamily ();
  ASSERT (mThisCpu != NULL);

  switch (mThisCpu->Family) {
  case CpuPentium:
    if (!IsSmrrSupported ()) {
      return ;
    }
    mFeatureContext.SmrrEnabled = TRUE;
    return ;
  default:
    return ;
  }
}

/**
  Initialize SMRR/SMBASE/SMM Sync features in SMM Relocate.

  @param  ProcessorNumber    The processor number
  @param  SmrrBase           The base address of SMRR.
  @param  SmrrSize           The size of SMRR.
  @param  SmBase             The SMBASE value.
  @param  IsBsp              If this processor treated as BSP.
**/
VOID
SmmInitiFeatures (
  IN UINTN   ProcessorNumber,
  IN UINT32  SmrrBase,
  IN UINT32  SmrrSize,
  IN UINT32  SmBase,
  IN BOOLEAN IsBsp
  )
{
  EFI_SMM_CPU_STATE                 *CpuState;
  SMM_CPU_SYNC_FEATURE              *SyncFeature;

  SyncFeature = &(gSmmCpuPrivate->SmmSyncFeature[ProcessorNumber]);
  SyncFeature->TargetedSmiSupported = FALSE;
  SyncFeature->DelayIndicationSupported = FALSE;
  SyncFeature->BlockIndicationSupported = FALSE;

  mThisCpu = GetCpuFamily ();
  ASSERT (mThisCpu != NULL);

  //
  // Configure SMBASE.
  //
  switch (mThisCpu->Family) {
    //
    // Fall back to legacy SMBASE setup.
    //
  case CpuPentium:
    CpuState = (EFI_SMM_CPU_STATE *)(UINTN)(SMM_DEFAULT_SMBASE + SMM_CPU_STATE_OFFSET);
    CpuState->x86.SMBASE = SmBase;
    break ;
  default:
    return ;
  }

  switch (mThisCpu->Family) {
  case CpuPentium:
    if (!IsSmrrSupported ()) {
      return ;
    }
    PentiumInitSmrr (SmrrBase, SmrrSize);
    return ;    
  default:
    ASSERT (FALSE);
    return ;
  }
}

/**
  Configure SMM Code Access Check feature for all processors.
  SMM Feature Control MSR will be locked after configuration.
**/
VOID
ConfigSmmCodeAccessCheck (
  VOID
  )
{

}

