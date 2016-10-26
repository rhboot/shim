/** @file
  Public include file for the CPU Configuration Library

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

  Module Name:  CpuConfigLib.h

**/

#ifndef _CPU_CONFIG_LIB_H_
#define _CPU_CONFIG_LIB_H_

//
// Bits definition of PcdProcessorFeatureUserConfiguration,
// PcdProcessorFeatureCapability, and PcdProcessorFeatureSetting
//
#define PCD_CPU_HT_BIT                           0x00000001
#define PCD_CPU_CMP_BIT                          0x00000002
#define PCD_CPU_L2_CACHE_BIT                     0x00000004
#define PCD_CPU_L2_ECC_BIT                       0x00000008
#define PCD_CPU_VT_BIT                           0x00000010
#define PCD_CPU_LT_BIT                           0x00000020
#define PCD_CPU_EXECUTE_DISABLE_BIT              0x00000040
#define PCD_CPU_L3_CACHE_BIT                     0x00000080
#define PCD_CPU_MAX_CPUID_VALUE_LIMIT_BIT        0x00000100
#define PCD_CPU_FAST_STRING_BIT                  0x00000200
#define PCD_CPU_FERR_SIGNAL_BREAK_BIT            0x00000400
#define PCD_CPU_PECI_BIT                         0x00000800
#define PCD_CPU_HARDWARE_PREFETCHER_BIT          0x00001000
#define PCD_CPU_ADJACENT_CACHE_LINE_PREFETCH_BIT 0x00002000
#define PCD_CPU_DCU_PREFETCHER_BIT               0x00004000
#define PCD_CPU_IP_PREFETCHER_BIT                0x00008000
#define PCD_CPU_MACHINE_CHECK_BIT                0x00010000
#define PCD_CPU_THERMAL_MANAGEMENT_BIT           0x00040000
#define PCD_CPU_EIST_BIT                         0x00080000
#define PCD_CPU_C1E_BIT                          0x00200000
#define PCD_CPU_C2E_BIT                          0x00400000
#define PCD_CPU_C3E_BIT                          0x00800000
#define PCD_CPU_C4E_BIT                          0x01000000
#define PCD_CPU_HARD_C4E_BIT                     0x02000000
#define PCD_CPU_DEEP_C4_BIT                      0x04000000
#define PCD_CPU_A20M_DISABLE_BIT                 0x08000000
#define PCD_CPU_MONITOR_MWAIT_BIT                0x10000000
#define PCD_CPU_TSTATE_BIT                       0x20000000
#define PCD_CPU_TURBO_MODE_BIT                   0x80000000

//
// Bits definition of PcdProcessorFeatureUserConfigurationEx1,
// PcdProcessorFeatureCapabilityEx1, and PcdProcessorFeatureSettingEx1
//
#define PCD_CPU_C_STATE_BIT                      0x00000001
#define PCD_CPU_C1_AUTO_DEMOTION_BIT             0x00000002
#define PCD_CPU_C3_AUTO_DEMOTION_BIT             0x00000004
#define PCD_CPU_MLC_STREAMER_PREFETCHER_BIT      0x00000008
#define PCD_CPU_MLC_SPATIAL_PREFETCHER_BIT       0x00000010
#define PCD_CPU_THREE_STRIKE_COUNTER_BIT         0x00000020
#define PCD_CPU_ENERGY_PERFORMANCE_BIAS_BIT      0x00000040
#define PCD_CPU_DCA_BIT                          0x00000080
#define PCD_CPU_X2APIC_BIT                       0x00000100
#define PCD_CPU_AES_BIT                          0x00000200
#define PCD_CPU_APIC_TPR_UPDATE_MESSAGE_BIT      0x00000400
#define PCD_CPU_SOCKET_ID_REASSIGNMENT_BIT       0x00000800

//
// Value definition for PcdCpuCallbackSignal
//
#define CPU_BYPASS_SIGNAL                        0x00000000
#define CPU_DATA_COLLECTION_SIGNAL               0x00000001
#define CPU_PROCESSOR_FEATURE_LIST_CONFIG_SIGNAL 0x00000002
#define CPU_REGISTER_TABLE_TRANSLATION_SIGNAL    0x00000003
#define CPU_PROCESSOR_SETTING_SIGNAL             0x00000004
#define CPU_PROCESSOR_SETTING_END_SIGNAL         0x00000005

typedef struct {
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
} EFI_CPUID_REGISTER;

//
// Enumeration of processor features
//
typedef enum {
  Ht,
  Cmp,
  Vt,
  ExecuteDisableBit,
  L3Cache,
  MaxCpuidValueLimit,
  FastString,
  FerrSignalBreak,
  Peci,
  HardwarePrefetcher,
  AdjacentCacheLinePrefetch,
  DcuPrefetcher,
  IpPrefetcher,
  ThermalManagement,
  Eist,
  BiDirectionalProchot,
  Forcepr,
  C1e,
  C2e,
  C3e,
  C4e,
  HardC4e,
  DeepC4,
  Microcode,
  Microcode2,
  MachineCheck,
  GateA20MDisable,
  MonitorMwait,
  TState,
  TurboMode,
  CState,
  C1AutoDemotion,
  C3AutoDemotion,
  MlcStreamerPrefetcher,
  MlcSpatialPrefetcher,
  ThreeStrikeCounter,
  EnergyPerformanceBias,
  Dca,
  X2Apic,
  Aes,
  ApicTprUpdateMessage,
  TccActivation,
  CpuFeatureMaximum
} CPU_FEATURE_ID;

//
// Structure for collected processor feature capability,
// and feature-specific attribute.
//
typedef struct {
  BOOLEAN                 Capability;
  VOID                    *Attribute;
} CPU_FEATURE_DATA;

//
// Structure for collected CPUID data.
//
typedef struct {
  EFI_CPUID_REGISTER         *CpuIdLeaf;
  UINTN                      NumberOfBasicCpuidLeafs;
  UINTN                      NumberOfExtendedCpuidLeafs;
  UINTN                      NumberOfCacheAndTlbCpuidLeafs;
  UINTN                      NumberOfDeterministicCacheParametersCpuidLeafs;
  UINTN                      NumberOfExtendedTopologyEnumerationLeafs;
} CPU_CPUID_DATA;

typedef struct {
  UINTN    Ratio;
  UINTN    Vid;
  UINTN    Power;
  UINTN    TransitionLatency;
  UINTN    BusMasterLatency;
} FVID_ENTRY;

//
// Miscellaneous processor data
//
typedef struct {
  //
  // Local Apic Data
  //
  UINT32                     InitialApicID;  ///< Initial APIC ID
  UINT32                     ApicID;         ///< Current APIC ID
  EFI_PHYSICAL_ADDRESS       ApicBase;
  UINT32                     ApicVersion;
  //
  // Frequency data
  //
  UINTN                      IntendedFsbFrequency;
  UINTN                      ActualFsbFrequency;
  BOOLEAN                    FrequencyLocked;
  UINTN                      MaxCoreToBusRatio;
  UINTN                      MinCoreToBusRatio;
  UINTN                      MaxTurboRatio;
  UINTN                      MaxVid;
  UINTN                      MinVid;
  UINTN                      PackageTdp;
  UINTN                      CoreTdp;
  UINTN                      NumberOfPStates;
  FVID_ENTRY                 *FvidTable;
  //
  // Config TDP data
  //
  UINTN                      PkgMinPwrLvl1;
  UINTN                      PkgMaxPwrLvl1;
  UINTN                      ConfigTDPLvl1Ratio;
  UINTN                      PkgTDPLvl1;
  UINTN                      PkgMinPwrLvl2;
  UINTN                      PkgMaxPwrLvl2;
  UINTN                      ConfigTDPLvl2Ratio;
  UINTN                      PkgTDPLvl2;

  //
  // Other data
  //
  UINT32                     PlatformRequirement;
  UINT64                     HealthData;
  UINT32                     MicrocodeRevision;
} CPU_MISC_DATA;

//
// Structure for all collected processor data
//
typedef struct {
  CPU_CPUID_DATA             CpuidData;
  EFI_CPU_PHYSICAL_LOCATION  ProcessorLocation;
  CPU_MISC_DATA              CpuMiscData;
  CPU_FEATURE_DATA           FeatureData[CpuFeatureMaximum];
  UINT8                      PackageIdBitOffset;
  BOOLEAN                    PackageBsp;
} CPU_COLLECTED_DATA;

#define GET_CPU_MISC_DATA(ProcessorNumber, Item) \
  ((mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber]).CpuMiscData.Item)

//
// Signature for feature list entry
//
#define EFI_CPU_FEATURE_ENTRY_SIGNATURE  SIGNATURE_32 ('C', 'f', 't', 'r')

//
// Node of processor feature list
//
typedef struct {
  UINT32                  Signature;
  CPU_FEATURE_ID          FeatureID;
  VOID                    *Attribute;
  LIST_ENTRY              Link;
} CPU_FEATURE_ENTRY;

#define CPU_FEATURE_ENTRY_FROM_LINK(link)  CR (link, CPU_FEATURE_ENTRY, Link, EFI_CPU_FEATURE_ENTRY_SIGNATURE)

//
// Register types in register table
//
typedef enum _REGISTER_TYPE {
  Msr,
  ControlRegister,
  MemoryMapped,
  CacheControl
} REGISTER_TYPE;

//
// Element of register table entry
//
typedef struct {
  REGISTER_TYPE RegisterType;
  UINT32        Index;
  UINT8         ValidBitStart;
  UINT8         ValidBitLength;
  UINT64        Value;
} CPU_REGISTER_TABLE_ENTRY;

//
// Register table definition, including current table length,
// allocated size of this table, and pointer to the list of table entries.
//
typedef struct {
  UINT32                   TableLength;
  UINT32                   NumberBeforeReset;
  UINT32                   AllocatedSize;
  UINT32                   InitialApicId;
  CPU_REGISTER_TABLE_ENTRY *RegisterTableEntry;
} CPU_REGISTER_TABLE;

//
// Definition of Processor Configuration Context Buffer
//
typedef struct {
  UINTN                    NumberOfProcessors;
  UINTN                    BspNumber;
  CPU_COLLECTED_DATA       *CollectedDataBuffer;
  LIST_ENTRY               *FeatureLinkListEntry;
  CPU_REGISTER_TABLE       *PreSmmInitRegisterTable;
  CPU_REGISTER_TABLE       *RegisterTable;
  UINTN                    *SettingSequence;
} CPU_CONFIG_CONTEXT_BUFFER;

//
// Structure conveying socket ID configuration information.
//
typedef struct {
  UINT8                    DefaultSocketId;
  UINT8                    NewSocketId;
} CPU_SOCKET_ID_INFO;

extern CPU_CONFIG_CONTEXT_BUFFER      *mCpuConfigLibConfigConextBuffer;

/**
  Set feature capability and related attribute.
  
  This function sets the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.
  @param  Attribute       Feature-specific data.

**/
VOID
EFIAPI
SetProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  );

/**
  Clears feature capability and related attribute.
  
  This function clears the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.

**/
VOID
EFIAPI
ClearProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID
  );

/**
  Get feature capability and related attribute.
  
  This function gets the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.
  @param  Attribute       Pointer to the output feature-specific data.

  @retval TRUE            The feature is supported by the processor
  @retval FALSE           The feature is not supported by the processor

**/
BOOLEAN
EFIAPI
GetProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  OUT VOID                **Attribute  OPTIONAL
  );

typedef enum {
  BasicCpuidLeaf,
  ExtendedCpuidLeaf,
  CacheAndTlbCpuidLeafs,
  DeterministicCacheParametersCpuidLeafs,
  ExtendedTopologyEnumerationCpuidLeafs
} CPUID_TYPE;

/**
  Get the number of CPUID leafs of various types.
  
  This function get the number of CPUID leafs of various types.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  CpuidType         The type of the CPU id.

  @return Maximal index of CPUID instruction for basic leafs.

**/
UINTN
EFIAPI
GetNumberOfCpuidLeafs (
  IN  UINTN               ProcessorNumber,
  IN  CPUID_TYPE          CpuidType
  );

/**
  Get the pointer to specified CPUID leaf.
  
  This function gets the pointer to specified CPUID leaf.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf

**/
EFI_CPUID_REGISTER*
EFIAPI
GetProcessorCpuid (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
  );

/**
  Get the pointer to specified CPUID leaf of cache and TLB parameters.
  
  This function gets the pointer to specified CPUID leaf of cache and TLB parameters.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetCacheAndTlbCpuidLeaf (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
  );

/**
  Get the pointer to specified CPUID leaf of deterministic cache parameters.
  
  This function gets the pointer to specified CPUID leaf of deterministic cache parameters.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetDeterministicCacheParametersCpuidLeaf (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
  );

/**
  Get the pointer to specified CPUID leaf of Extended Topology Enumeration.
  
  This function gets the pointer to specified CPUID leaf of Extended Topology Enumeration.

  @param  ProcessorNumber   Handle number of specified logical processor.
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetExtendedTopologyEnumerationCpuidLeafs (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
  );

/**
  Get the version information of specified logical processor.
  
  This function gets the version information of specified logical processor,
  including family ID, model ID, stepping ID and processor type.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  DisplayedFamily   Pointer to family ID for output
  @param  DisplayedModel    Pointer to model ID for output
  @param  SteppingId        Pointer to stepping ID for output
  @param  ProcessorType     Pointer to processor type for output

**/
VOID
EFIAPI
GetProcessorVersionInfo (
  IN  UINTN               ProcessorNumber,
  OUT UINT32              *DisplayedFamily OPTIONAL,
  OUT UINT32              *DisplayedModel  OPTIONAL,       
  OUT UINT32              *SteppingId      OPTIONAL,
  OUT UINT32              *ProcessorType   OPTIONAL
  );

/**
  Get initial local APIC ID of specified logical processor
  
  This function gets initial local APIC ID of specified logical processor.

  @param  ProcessorNumber Handle number of specified logical processor

  @return Initial local APIC ID of specified logical processor

**/
UINT32
EFIAPI
GetInitialLocalApicId (
  UINTN    ProcessorNumber
  );

/**
  Get the location of specified processor.
  
  This function gets the location of specified processor, including
  package number, core number within package, thread number within core.

  @param  ProcessorNumber Handle number of specified logical processor.
  @param  PackageNumber   Pointer to the output package number.
  @param  CoreNumber      Pointer to the output core number.
  @param  ThreadNumber    Pointer to the output thread number.

**/
VOID
EFIAPI
GetProcessorLocation (
  IN    UINTN     ProcessorNumber,
  OUT   UINT32    *PackageNumber   OPTIONAL,
  OUT   UINT32    *CoreNumber      OPTIONAL,
  OUT   UINT32    *ThreadNumber    OPTIONAL
  );

/**
  Get the Feature entry at specified position in a feature list.
  
  This function gets the Feature entry at specified position in a feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureIndex    The index of the node in feature list.
  @param  Attribute       Pointer to output feature-specific attribute

  @return Feature ID of specified feature. CpuFeatureMaximum means not found

**/
CPU_FEATURE_ID
EFIAPI
GetProcessorFeatureEntry (
  IN        UINTN       ProcessorNumber,
  IN        UINTN       FeatureIndex,
  OUT       VOID        **Attribute  OPTIONAL
  );

/**
  Append a feature entry at the end of a feature list.
  
  This function appends a feature entry at the end of a feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       ID of the specified feature.
  @param  Attribute       Feature-specific attribute.

  @retval EFI_SUCCESS     This function always return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
AppendProcessorFeatureIntoList (
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  );

/**
  Delete a feature entry in a feature list.
  
  This function deletes a feature entry in a feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureIndex    The index of the node in feature list.

  @retval EFI_SUCCESS            The feature node successfully removed.
  @retval EFI_INVALID_PARAMETER  Index surpasses the length of list.

**/
EFI_STATUS
EFIAPI
DeleteProcessorFeatureFromList (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex
  );

/**
  Insert a feature entry into a feature list.
  
  This function insert a feature entry into a feature list before a node specified by FeatureIndex.

  @param  ProcessorNumber        Handle number of specified logical processor
  @param  FeatureIndex           The index of the new node in feature list.
  @param  FeatureID              ID of the specified feature.
  @param  Attribute              Feature-specific attribute.

  @retval EFI_SUCCESS            The feature node successfully inserted.
  @retval EFI_INVALID_PARAMETER  Index surpasses the length of list.

**/
EFI_STATUS
EFIAPI
InsertProcessorFeatureIntoList (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  );

/**
  Add an entry in the post-SMM-init register table.
  
  This function adds an entry in the post-SMM-init register table, with given register type,
  register index, bit section and value.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  RegisterType    Type of the register to program
  @param  Index           Index of the register to program
  @param  ValidBitStart   Start of the bit section
  @param  ValidBitLength  Length of the bit section
  @param  Value           Value to write

**/
VOID
EFIAPI
WriteRegisterTable (
  IN  UINTN               ProcessorNumber,
  IN  REGISTER_TYPE       RegisterType,
  IN  UINT32              Index,
  IN  UINT8               ValidBitStart,
  IN  UINT8               ValidBitLength,
  IN  UINT64              Value
  );

/**
  Add an entry in the pre-SMM-init register table.
  
  This function adds an entry in the pre-SMM-init register table, with given register type,
  register index, bit section and value.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  RegisterType    Type of the register to program
  @param  Index           Index of the register to program
  @param  ValidBitStart   Start of the bit section
  @param  ValidBitLength  Length of the bit section
  @param  Value           Value to write

**/
VOID
EFIAPI
WritePreSmmInitRegisterTable (
  IN  UINTN               ProcessorNumber,
  IN  REGISTER_TYPE       RegisterType,
  IN  UINT32              Index,
  IN  UINT8               ValidBitStart,
  IN  UINT8               ValidBitLength,
  IN  UINT64              Value
  );

/**
  Set the sequence of processor setting.
  
  This function sets the a processor setting at the position in
  setting sequence specified by Index.

  @param  Index                  The zero-based index in the sequence.
  @param  ProcessorNumber        Handle number of the processor to set.

  @retval EFI_SUCCESS            The sequence successfully modified.
  @retval EFI_INVALID_PARAMETER  Index surpasses the boundary of sequence.
  @retval EFI_NOT_FOUND          Processor specified by ProcessorNumber does not exist. 

**/
EFI_STATUS
SetSettingSequence (
  IN UINTN  Index,
  IN UINTN  ProcessorNumber
  );

/**
  Set PcdCpuCallbackSignal, and then read the value back.
  
  This function sets PCD entry PcdCpuCallbackSignal. If there is callback
  function registered on it, the callback function will be triggered, and
  it may change the value of PcdCpuCallbackSignal. This function then reads
  the value of PcdCpuCallbackSignal back, the check whether it has been changed.

  @param  Value  The value to set to PcdCpuCallbackSignal.

  @return The value of PcdCpuCallbackSignal read back.

**/
UINT8
SetAndReadCpuCallbackSignal (
  IN UINT8  Value
  );

#endif
