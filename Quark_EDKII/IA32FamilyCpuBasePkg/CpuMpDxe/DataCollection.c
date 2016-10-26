/** @file
  Code for Data Collection phase.

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

  Module Name:  DataCollection.c

**/

#include "Cpu.h"
#include "Feature.h"

BOOLEAN    HtCapable  = FALSE;
BOOLEAN    CmpCapable = FALSE;

/**
  Collects Local APIC data of the processor.

  This function collects Local APIC base, verion, and APIC ID of the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectApicData (
  UINTN    ProcessorNumber
  )
{
  CPU_MISC_DATA          *CpuMiscData;
  UINT32                 LocalApicBaseAddress;

  CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;

  LocalApicBaseAddress = (UINT32) GetLocalApicBaseAddress();
  CpuMiscData->ApicBase    = LocalApicBaseAddress;
  //
  // Read bits 0..7 of Local APIC Version Register for Local APIC version.
  //
  CpuMiscData->ApicVersion = GetApicVersion () & 0xff;
  //
  // Read bits 24..31 of Local APIC ID Register for Local APIC ID
  //
  CpuMiscData->ApicID        = GetApicId ();
  CpuMiscData->InitialApicID = GetInitialApicId ();
}

/**
  Collects all CPUID leafs the processor.

  This function collects all CPUID leafs the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectCpuidLeafs (
  UINTN    ProcessorNumber
  )
{
  CPU_COLLECTED_DATA   *CpuCollectedData;
  EFI_CPUID_REGISTER   *CpuidRegisters;
  UINT32               Index;

  CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];
  //
  // Collect basic CPUID information.
  //
  CpuidRegisters = CpuCollectedData->CpuidData.CpuIdLeaf;
  for (Index = 0; Index < GetNumberOfCpuidLeafs (ProcessorNumber, BasicCpuidLeaf); Index++) {
    AsmCpuid (
      Index,
      &CpuidRegisters->RegEax,
      &CpuidRegisters->RegEbx,
      &CpuidRegisters->RegEcx,
      &CpuidRegisters->RegEdx
      );
    CpuidRegisters++;
  }

  //
  // Collect extended function CPUID information.
  //
  for (Index = 0; Index < GetNumberOfCpuidLeafs (ProcessorNumber, ExtendedCpuidLeaf); Index++) {
    AsmCpuid (
      Index + 0x80000000,
      &CpuidRegisters->RegEax,
      &CpuidRegisters->RegEbx,
      &CpuidRegisters->RegEcx,
      &CpuidRegisters->RegEdx
      );
    CpuidRegisters++;
  }

  //
  // Collect additional Cache & TLB information, if exists.
  //
  for (Index = 1; Index < GetNumberOfCpuidLeafs (ProcessorNumber, CacheAndTlbCpuidLeafs); Index++) {
    AsmCpuid (
      2,
      &CpuidRegisters->RegEax,
      &CpuidRegisters->RegEbx,
      &CpuidRegisters->RegEcx,
      &CpuidRegisters->RegEdx
      );
    CpuidRegisters++;
  }

  //
  // Collect Deterministic Cache Parameters Leaf.
  //
  for (Index = 0; Index < GetNumberOfCpuidLeafs (ProcessorNumber, DeterministicCacheParametersCpuidLeafs); Index++) {
    AsmCpuidEx (
      4,
      Index,
      &CpuidRegisters->RegEax,
      &CpuidRegisters->RegEbx,
      &CpuidRegisters->RegEcx,
      &CpuidRegisters->RegEdx
      );
    CpuidRegisters++;
  }

  //
  // Collect Extended Topology Enumeration Leaf.
  //
  for (Index = 0; Index < GetNumberOfCpuidLeafs (ProcessorNumber, ExtendedTopologyEnumerationCpuidLeafs); Index++) {
    AsmCpuidEx (
      EFI_CPUID_CORE_TOPOLOGY,
      Index,
      &CpuidRegisters->RegEax,
      &CpuidRegisters->RegEbx,
      &CpuidRegisters->RegEcx,
      &CpuidRegisters->RegEdx
      );
    CpuidRegisters++;
  }
}

/**
  Collects physical location of the processor.

  This function gets Package ID/Core ID/Thread ID of the processor.

  The algorithm below assumes the target system has symmetry across physical package boundaries
  with respect to the number of logical processors per package, number of cores per package.

  @param  InitialApicId  Initial APIC ID for determing processor topology.
  @param  Location       Pointer to EFI_CPU_PHYSICAL_LOCATION structure.
  @param  ThreadIdBits   Number of bits occupied by Thread ID portion.
  @param  CoreIdBits     Number of bits occupied by Core ID portion.

**/
VOID
ExtractProcessorLocation (
  IN  UINT32                    InitialApicId,
  OUT EFI_CPU_PHYSICAL_LOCATION *Location,
  OUT UINTN                     *ThreadIdBits,
  OUT UINTN                     *CoreIdBits
  )
{
  BOOLEAN  TopologyLeafSupported;
  UINTN    ThreadBits;
  UINTN    CoreBits;
  UINT32   RegEax;
  UINT32   RegEbx;
  UINT32   RegEcx;
  UINT32   RegEdx;
  UINT32   MaxCpuIdIndex;
  UINT32   SubIndex;
  UINTN    LevelType;
  UINT32   MaxLogicProcessorsPerPackage;
  UINT32   MaxCoresPerPackage;

  //
  // Check if the processor is capable of supporting more than one logical processor.
  //
  AsmCpuid (EFI_CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);
  if ((RegEdx & BIT28) == 0) {
    Location->Thread  = 0;
    Location->Core    = 0;
    Location->Package = 0;
    *ThreadIdBits     = 0;
    *CoreIdBits       = 0;
    return;
  }

  ThreadBits = 0;
  CoreBits = 0;
  
  //
  // Assume three-level mapping of APIC ID: Package:Core:SMT.
  //

  TopologyLeafSupported = FALSE;
  //
  // Get the max index of basic CPUID
  //
  AsmCpuid (EFI_CPUID_SIGNATURE, &MaxCpuIdIndex, NULL, NULL, NULL);

  //
  // If the extended topology enumeration leaf is available, it
  // is the preferred mechanism for enumerating topology.
  //
  if (MaxCpuIdIndex >= EFI_CPUID_EXTENDED_TOPOLOGY) {
    AsmCpuidEx (EFI_CPUID_EXTENDED_TOPOLOGY, 0, &RegEax, &RegEbx, &RegEcx, NULL);
    //
    // If CPUID.(EAX=0BH, ECX=0H):EBX returns zero and maximum input value for
    // basic CPUID information is greater than 0BH, then CPUID.0BH leaf is not 
    // supported on that processor.
    //
    if (RegEbx != 0) {
      TopologyLeafSupported = TRUE;

      //
      // Sub-leaf index 0 (ECX= 0 as input) provides enumeration parameters to extract 
      // the SMT sub-field of x2APIC ID.
      //
      LevelType = (RegEcx >> 8) & 0xff;
      ASSERT (LevelType == EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT);
      ThreadBits = RegEax & 0x1f;

      //
      // Software must not assume any "level type" encoding 
      // value to be related to any sub-leaf index, except sub-leaf 0.
      //
      SubIndex = 1;
      do {
        AsmCpuidEx (EFI_CPUID_EXTENDED_TOPOLOGY, SubIndex, &RegEax, NULL, &RegEcx, NULL);
        LevelType = (RegEcx >> 8) & 0xff;
        if (LevelType == EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE) {
          CoreBits = (RegEax & 0x1f) - ThreadBits;
          break;
        }
        SubIndex++;
      } while (LevelType != EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID);
    }
  }

  if (!TopologyLeafSupported) {
    AsmCpuid (EFI_CPUID_VERSION_INFO, NULL, &RegEbx, NULL, NULL);
    MaxLogicProcessorsPerPackage = (RegEbx >> 16) & 0xff;
    if (MaxCpuIdIndex >= EFI_CPUID_CACHE_PARAMS) {
      AsmCpuidEx (EFI_CPUID_CACHE_PARAMS, 0, &RegEax, NULL, NULL, NULL);
      MaxCoresPerPackage = (RegEax >> 26) + 1;
    } else {
      //
      // Must be a single-core processor.
      //
      MaxCoresPerPackage = 1;
    }

    ThreadBits = (UINTN) (HighBitSet32 (MaxLogicProcessorsPerPackage / MaxCoresPerPackage - 1) + 1);
    CoreBits = (UINTN) (HighBitSet32 (MaxCoresPerPackage - 1) + 1);
  }

  Location->Thread  = InitialApicId & ~((-1) << ThreadBits);
  Location->Core    = (InitialApicId >> ThreadBits) & ~((-1) << CoreBits);
  Location->Package = (InitialApicId >> (ThreadBits+ CoreBits));
  *ThreadIdBits     = ThreadBits;
  *CoreIdBits       = CoreBits;
}

/**
  Collects physical location of the processor.

  This function collects physical location of the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectProcessorLocation (
  UINTN    ProcessorNumber
  )
{
  CPU_COLLECTED_DATA  *CpuCollectedData;
  UINT32              InitialApicID;
  UINTN               ThreadIdBits;
  UINTN               CoreIdBits;

  CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];
  InitialApicID    = CpuCollectedData->CpuMiscData.InitialApicID;

  ExtractProcessorLocation (
    InitialApicID,
    &CpuCollectedData->ProcessorLocation,
    &ThreadIdBits,
    &CoreIdBits
    );

  CpuCollectedData->PackageIdBitOffset = (UINT8)(ThreadIdBits+ CoreIdBits);

  //
  // Check CMP and HT capabilities
  //
  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    if (CoreIdBits > 0) {
      CmpCapable = TRUE;
    }
    if (ThreadIdBits > 0) {
      HtCapable = TRUE;
    }
  }
}

/**
  Collects intended FSB frequency and core to bus ratio.

  This function collects intended FSB frequency and core to bus ratio.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectFrequencyData (
  UINTN    ProcessorNumber
  )
{
  CPU_MISC_DATA          *CpuMiscData;
  UINT32                 FamilyId;
  UINT32                 ModelId;
  UINT32                 SteppingId;
  BOOLEAN                OldInterruptState;
  UINT64                 BeginValue;
  UINT64                 EndValue;
  UINT64                 ActualFrequency;
  UINT64                 ActualFsb;

  CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;
  CpuMiscData->FrequencyLocked = FALSE;
  CpuMiscData->MaxCoreToBusRatio = (UINTN) 0x01;

  GetProcessorVersionInfo (ProcessorNumber, &FamilyId, &ModelId, &SteppingId, NULL);

  switch (FamilyId) {
  case PENTIUM_FAMILY_ID:
    switch (ModelId) {
    case QUARK_MODEL_ID:
      //
      // Support for Pentium processor family
      //

      //
      // Collect intended FSB frequency
      //
      CpuMiscData->IntendedFsbFrequency = 400;

      //
      // Check whether frequency is locked
      //
      CpuMiscData->FrequencyLocked = TRUE;

      //
      // Collect core to bus ratio
      //
      CpuMiscData->MaxCoreToBusRatio = (UINTN) 0x01;
      CpuMiscData->MinCoreToBusRatio = (UINTN) 0x01;

      //
      // Collect VID
      //
      CpuMiscData->MaxVid = (UINTN) 0x03;
      CpuMiscData->MinVid = (UINTN) 0x03;

      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  //
  // Calculate actual FSB frequency
  // First calculate actual frequency by sampling some time and counts TSC
  // Use spinlock mechanism because timer library cannot handle concurrent requests.
  //
  AcquireSpinLock (&mMPSystemData.APSerializeLock);
  OldInterruptState = SaveAndDisableInterrupts ();
  BeginValue  = AsmReadTsc ();
  MicroSecondDelay (1000);
  EndValue    = AsmReadTsc ();
  SetInterruptState (OldInterruptState);
  ReleaseSpinLock (&mMPSystemData.APSerializeLock);
  //
  // Calculate raw actual FSB frequency
  //
  ActualFrequency = DivU64x32 (EndValue - BeginValue, 1000);
  ActualFsb = DivU64x32 (ActualFrequency, (UINT32) CpuMiscData->MaxCoreToBusRatio);
  //
  // Round the raw actual FSB frequency to standardized value
  //
  ActualFsb = ActualFsb + RShiftU64 (ActualFsb, 5);
  ActualFsb = DivU64x32 (MultU64x32 (ActualFsb, 3), 100);
  ActualFsb = DivU64x32 (MultU64x32 (ActualFsb, 100), 3);

  CpuMiscData->ActualFsbFrequency = (UINTN) ActualFsb;

  //
  // Default number of P-states is 1
  //
  CpuMiscData->NumberOfPStates = 1;
}

/**
  Collects capabilities of various features of the processor.

  This function collects capabilities of various features of the processor.

  @param  ProcessorNumber    Handle number of specified logical processor

**/
VOID
CollectFeatureCapability (
  UINTN    ProcessorNumber
  )
{
  //
  // Collect capability for Max CPUID Value Limit
  //
  if (FeaturePcdGet (PcdCpuMaxCpuIDValueLimitFlag)) {
    MaxCpuidLimitDetect (ProcessorNumber);
  }
  //
  // Collect capability for execute disable bit
  //
  if (FeaturePcdGet (PcdCpuExecuteDisableBitFlag)) {
    XdDetect (ProcessorNumber);
  }
}

/**
  Collects basic processor data for calling processor.

  This function collects basic processor data for calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CollectBasicProcessorData (
  IN UINTN  ProcessorNumber
  )
{
  CPU_MISC_DATA        *CpuMiscData;

  CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;

  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    //
    // BIST for the BSP will be updated in CollectBistDataFromHob().
    //
    CpuMiscData->HealthData = 0;
  } else {
    CpuMiscData->HealthData = mExchangeInfo->BistBuffer[ProcessorNumber].Bist;
  }

  //
  // A loop to check APIC ID and processor number
  //
  CollectApicData (ProcessorNumber);

  //
  // Get package number, core number and thread number.
  //
  CollectProcessorLocation (ProcessorNumber);
}

/**
  Collects processor data for calling processor.

  This function collects processor data for calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CollectProcessorData (
  IN UINTN  ProcessorNumber
  )
{
  CPU_MISC_DATA        *CpuMiscData;

  CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;

  //
  // Collect all leafs for CPUID after second time microcode load.
  //
  CollectCpuidLeafs (ProcessorNumber);

  //
  // Get intended FSB frequency and core to bus ratio
  //
  CollectFrequencyData (ProcessorNumber);

  //
  // Collect capabilities for various features.
  //
  CollectFeatureCapability (ProcessorNumber);
}

/**
  Checks the number of CPUID leafs need by a processor.

  This function check the number of CPUID leafs need by a processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CountNumberOfCpuidLeafs (
  IN UINTN  ProcessorNumber
  )
{
  UINT32               MaxCpuidIndex;
  UINT32               MaxExtendedCpuidIndex;
  UINT32               NumberOfCacheAndTlbRecords;
  UINT32               NumberOfDeterministicCacheParametersLeafs;
  UINT32               NumberOfExtendedTopologyEnumerationLeafs;
  UINT32               RegValue;
  CPU_COLLECTED_DATA   *CpuCollectedData;

  CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];

  //
  // Get the max index of basic CPUID
  //
  AsmCpuid (0, &MaxCpuidIndex, NULL, NULL, NULL);
  //
  // Get the max index of extended CPUID
  //
  AsmCpuid (0x80000000, &MaxExtendedCpuidIndex, NULL, NULL, NULL);
  //
  // Get the number of cache and TLB CPUID leafs
  //
  AsmCpuid (2, &NumberOfCacheAndTlbRecords, NULL, NULL, NULL);
  NumberOfCacheAndTlbRecords = NumberOfCacheAndTlbRecords & 0xff;

  //
  // Get the number of deterministic cache parameter CPUID leafs
  //
  NumberOfDeterministicCacheParametersLeafs = 0;
  do {
    AsmCpuidEx (4, NumberOfDeterministicCacheParametersLeafs++, &RegValue, NULL, NULL, NULL);
  } while ((RegValue & 0x0f) != 0);

  //
  // Get the number of Extended Topology Enumeration CPUID leafs
  //
  NumberOfExtendedTopologyEnumerationLeafs = 0;
  if (MaxCpuidIndex >= EFI_CPUID_CORE_TOPOLOGY) {
    do {
      AsmCpuidEx (EFI_CPUID_CORE_TOPOLOGY, NumberOfExtendedTopologyEnumerationLeafs++, NULL, &RegValue, NULL, NULL);
    } while ((RegValue & 0x0FFFF) != 0);
  }

  //
  // Save collected data in Processor Configuration Context Buffer
  //
  CpuCollectedData->CpuidData.NumberOfBasicCpuidLeafs                        = MaxCpuidIndex + 1;
  CpuCollectedData->CpuidData.NumberOfExtendedCpuidLeafs                     = (MaxExtendedCpuidIndex - 0x80000000) + 1;
  CpuCollectedData->CpuidData.NumberOfCacheAndTlbCpuidLeafs                  = NumberOfCacheAndTlbRecords;
  CpuCollectedData->CpuidData.NumberOfDeterministicCacheParametersCpuidLeafs = NumberOfDeterministicCacheParametersLeafs;
  CpuCollectedData->CpuidData.NumberOfExtendedTopologyEnumerationLeafs       = NumberOfExtendedTopologyEnumerationLeafs;
}

/**
  Checks the number of CPUID leafs of all logical processors, and allocate memory for them.

  This function checks the number of CPUID leafs of all logical processors, and allocates memory for them.

**/
VOID
AllocateMemoryForCpuidLeafs (
  VOID
  )
{
  CPU_COLLECTED_DATA   *CpuCollectedData;
  UINTN                ProcessorNumber;
  UINTN                NumberOfLeafs;

  //
  // Wakeup all APs for CPUID checking.
  //
  DispatchAPAndWait (
    TRUE,
    0,
    CountNumberOfCpuidLeafs
    );
  //
  // Check number of CPUID leafs for BSP.
  // Try to accomplish in first wakeup, and MTRR.
  //
  CountNumberOfCpuidLeafs (mCpuConfigConextBuffer.BspNumber);

  //
  // Allocate memory for CPUID leafs of all processors
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    CpuCollectedData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber];

    //
    // Get the number of basic CPUID leafs.
    //
    NumberOfLeafs = CpuCollectedData->CpuidData.NumberOfBasicCpuidLeafs;
    //
    // Get the number of extended CPUID leafs.
    //
    NumberOfLeafs += CpuCollectedData->CpuidData.NumberOfExtendedCpuidLeafs;
    //
    // Get the number of cache and TLB CPUID leafs.
    //
    NumberOfLeafs += CpuCollectedData->CpuidData.NumberOfCacheAndTlbCpuidLeafs - 1;
    //
    // Get the number of deterministic cache parameters CPUID leafs.
    //
    NumberOfLeafs += CpuCollectedData->CpuidData.NumberOfDeterministicCacheParametersCpuidLeafs;
    //
    // Get the number of Extended Topology Enumeration CPUID leafs
    //
    NumberOfLeafs += CpuCollectedData->CpuidData.NumberOfExtendedTopologyEnumerationLeafs;

    CpuCollectedData->CpuidData.CpuIdLeaf = AllocateZeroPool (sizeof (EFI_CPUID_REGISTER) * NumberOfLeafs);
  }
}

/**
  Collects BIST data from HOB.

  This function collects BIST data from HOB built by SEC_PLATFORM_PPI.

**/
VOID
CollectBistDataFromHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  UINT32                  *DataInHob;
  UINTN                   NumberOfData;
  UINTN                   ProcessorNumber;
  UINT32                  InitialLocalApicId;
  CPU_MISC_DATA           *CpuMiscData;

  GuidHob = GetFirstGuidHob (&gEfiHtBistHobGuid);
  if (GuidHob == NULL) {
    return;
  }

  DataInHob    = GET_GUID_HOB_DATA (GuidHob);
  NumberOfData = GET_GUID_HOB_DATA_SIZE (GuidHob) / sizeof (UINT32);

  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    InitialLocalApicId = GetInitialLocalApicId (ProcessorNumber);
    if (InitialLocalApicId < NumberOfData) {
      CpuMiscData = &mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData;
      CpuMiscData->HealthData = DataInHob[InitialLocalApicId];
      //
      // Initialize CPU health status for MP Services Protocol according to BIST data.
      //
      mMPSystemData.CpuHealthy[ProcessorNumber] = (BOOLEAN) (CpuMiscData->HealthData == 0);
      if (!mMPSystemData.CpuHealthy[ProcessorNumber]) {
        //
        // Report Status Code that self test is failed
        //
        REPORT_STATUS_CODE (
          EFI_ERROR_CODE | EFI_ERROR_MAJOR,
          (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)
          );        
      }
    }
  }
}

/**
  Collects data from all logical processors.

  This function collects data from all logical processors.

**/
VOID
DataCollectionPhase (
  VOID
  )
{
  UINT8                     CallbackSignalValue;

  //
  // Set PcdCpuCallbackSignal to trigger callback function, and reads the value back.
  //
  CallbackSignalValue = SetAndReadCpuCallbackSignal (CPU_DATA_COLLECTION_SIGNAL);
  //
  // Check whether the callback function requests to bypass Setting phase.
  //
  if (CallbackSignalValue == CPU_BYPASS_SIGNAL) {
    return;
  }

  //
  // Check the number of CPUID leafs of all logical processors, and allocate memory for them.
  //
  AllocateMemoryForCpuidLeafs ();

  //
  // Wakeup all APs for data collection.
  //
  DispatchAPAndWait (
    TRUE,
    0,
    CollectProcessorData
    );

  //
  // Collect data for BSP.
  //
  CollectProcessorData (mCpuConfigConextBuffer.BspNumber);

  CollectBistDataFromHob ();

  //
  // Set PCD data for HT and CMP information
  //
  if (CmpCapable) {
    PcdSet32 (PcdCpuProcessorFeatureCapability, PcdGet32 (PcdCpuProcessorFeatureCapability) | PCD_CPU_CMP_BIT);
    if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_CMP_BIT) != 0) {
      PcdSet32 (PcdCpuProcessorFeatureSetting, PcdGet32 (PcdCpuProcessorFeatureSetting) | PCD_CPU_CMP_BIT);
    }
  }
  if (HtCapable) {
    PcdSet32 (PcdCpuProcessorFeatureCapability, PcdGet32 (PcdCpuProcessorFeatureCapability) | PCD_CPU_HT_BIT);
    if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_HT_BIT) != 0) {
      PcdSet32 (PcdCpuProcessorFeatureSetting, PcdGet32 (PcdCpuProcessorFeatureSetting) | PCD_CPU_HT_BIT);
    }
  }
}
