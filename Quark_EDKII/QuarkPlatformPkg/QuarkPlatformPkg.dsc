## @file       
# Clanton Peak CRB platform with 32-bit DXE for 4MB/8MB flash devices.       
#       
# This package provides Clanton Peak CRB platform specific modules.       
# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#       
##       
 
################################################################################       
#       
# Defines Section - statements that will be processed to create a Makefile.       
#       
################################################################################       
[Defines]       
  PLATFORM_NAME                  = QuarkPlatformPkg       
  PLATFORM_GUID                  = 1BEDB57A-7904-406e-8486-C89FC7FB39EE       
  VPD_TOOL_GUID                  = 8C3D856A-9BE6-468E-850A-24F7A8D38E08        
  PLATFORM_VERSION               = 0.1       
  DSC_SPECIFICATION              = 0x00010005       
  OUTPUT_DIRECTORY               = Build/QuarkPlatform       
  SUPPORTED_ARCHITECTURES        = IA32       
  BUILD_TARGETS                  = DEBUG|RELEASE       
  SKUID_IDENTIFIER               = DEFAULT       
  
  #
  # Set the global variables
  #
  EDK_GLOBAL PLATFORM_PKG             = QuarkPlatformPkg

  #
  # Platform On/Off features are defined here
  #
  !include $(PLATFORM_PKG)/QuarkPlatformPkgConfig.dsc

  FLASH_DEFINITION                    = $(PLATFORM_PKG)/QuarkPlatformPkg.fdf

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
  #
  # Entry point
  #
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  #
  # Basic
  #
  BaseLib|QuarkSocPkg/Override/MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|QuarkSocPkg/Override/MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
!endif
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  #
  # Framework
  #
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf
  #
  # Generic Modules
  #
  UefiUsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
  SmmCorePlatformHookLib|MdeModulePkg/Library/SmmCorePlatformHookLibNull/SmmCorePlatformHookLibNull.inf
  #
  # CPU
  #
  MtrrLib|QuarkSocPkg/Override/UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  CpuConfigLib|IA32FamilyCpuBasePkg/Library/CpuConfigLib/CpuConfigLib.inf
  CpuOnlyResetLib|IA32FamilyCpuBasePkg/Library/CpuOnlyResetLibNull/CpuOnlyResetLibNull.inf
  #
  # Quark North Cluster
  #
  SmmLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCSmmLib/QNCSmmLib.inf
  SmbusLib|QuarkSocPkg/QuarkNorthCluster/Library/SmbusLib/SmbusLib.inf
  TimerLib|QuarkSocPkg/QuarkNorthCluster/Library/IntelQNCAcpiTimerLib/IntelQNCAcpiTimerLib.inf
  ResetSystemLib|QuarkSocPkg/QuarkNorthCluster/Library/ResetSystemLib/ResetSystemLib.inf
  IntelQNCLib|QuarkSocPkg/QuarkNorthCluster/Library/IntelQNCLib/IntelQNCLib.inf
  QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/QNCAccessLib.inf
  RedirectPeiServicesLib|QuarkSocPkg/QuarkNorthCluster/Library/RedirectPeiServicesLib/RedirectPeiServicesLib.inf
  #
  # Quark South Cluster
  #
  IohLib|QuarkSocPkg/QuarkSouthCluster/Library/IohLib/IohLib.inf
  SerialPortLib|QuarkSocPkg/QuarkSouthCluster/Library/IohSerialPortLib/IohSerialPortLib.inf
  #
  # Platform
  #
  PlatformSecServicesLib|QuarkPlatformPkg/Library/PlatformSecServicesLib/PlatformSecServicesLib.inf
  UefiBootManagerLib|QuarkPlatformPkg/Bds/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  LogoLib|QuarkPlatformPkg/Library/LogoLib/LogoLib.inf
  PlatformBootManagerLib|QuarkPlatformPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  RecoveryOemHookLib|QuarkPlatformPkg/Library/RecoveryOemHookLib/RecoveryOemHookLib.inf
  PlatformSecLib|QuarkPlatformPkg/Library/QuarkSecLib/QuarkSecLib.inf
  SecurityAuthenticationLib|QuarkPlatformPkg/Library/QuarkBootRomLib/QuarkBootRomLib.inf
  SmmScriptLib|QuarkPlatformPkg/Library/SmmScriptLib/SmmScriptLib.inf
  CapsuleLib|QuarkPlatformPkg/Library/PlatformCapsuleLib/PlatformCapsuleLib.inf
  MfhLib|QuarkPlatformPkg/Library/MfhLib/MfhLib.inf
  PlatformDataLib|QuarkPlatformPkg/Library/PlatformDataLib/PlatformDataLib.inf
  SmmCpuPlatformHookLib|QuarkPlatformPkg/Library/SmmCpuPlatformHookLib/SmmCpuPlatformHookLib.inf
  PlatformSecureLib|QuarkPlatformPkg/Library/PlatformSecureLib/PlatformSecureLib.inf
  PlatformPcieHelperLib|QuarkPlatformPkg/Library/PlatformPcieHelperLib/PlatformPcieHelperLib.inf
  #
  # Misc
  #
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!else
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
!endif
  #
  # Crypto
  #
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf

[LibraryClasses.IA32.PEIM,LibraryClasses.IA32.PEI_CORE]
  #
  # SEC and PEI phase common
  #
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
!endif
  SwBpeLib|QuarkPlatformPkg/Library/SwBpeLib/SwBpeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf

  #
  # Platform SEC and PEI phase common.
  #
  PlatformHelperLib|QuarkPlatformPkg/Library/PlatformHelperLib/PeiPlatformHelperLib.inf

[LibraryClasses.IA32.SEC]
  #
  # SEC specific phase
  #
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

[LibraryClasses.IA32]
  #
  # DXE phase common
  #
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf

  #
  # Platform DXE phase common.
  #
  PlatformHelperLib|QuarkPlatformPkg/Library/PlatformHelperLib/DxePlatformHelperLib.inf

[LibraryClasses.IA32.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  PerformanceLib|MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf

[LibraryClasses.IA32.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SmmDebugAgentLib.inf
!endif
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/SmmPerformanceLib/SmmPerformanceLib.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf

[LibraryClasses.IA32.SMM_CORE]
  SmmServicesTableLib|MdeModulePkg/Library/PiSmmCoreSmmServicesTableLib/PiSmmCoreSmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdeModulePkg/Library/PiSmmCoreMemoryAllocationLib/PiSmmCoreMemoryAllocationLib.inf
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/SmmCorePerformanceLib/SmmCorePerformanceLib.inf
!endif
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/RuntimeQNCAccessLib.inf

[LibraryClasses.IA32.UEFI_DRIVER,LibraryClasses.IA32.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformanceLib|MdeModulePkg/Library/DxeSmmPerformanceLib/DxeSmmPerformanceLib.inf
!endif

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdInstallAcpiSdtProtocol|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPlatformCsmSupport|FALSE
  gEfiCpuTokenSpaceGuid.PcdCpuMaxCpuIDValueLimitFlag|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdIsaBusSerialUseHalfHandshake|FALSE
  !ifdef SECURE_LD
    gQuarkPlatformTokenSpaceGuid.PcdEnableSecureLock|TRUE
    gEfiQuarkNcSocIdTokenSpaceGuid.PcdRmuDmaLock|TRUE
  !else
    gQuarkPlatformTokenSpaceGuid.PcdEnableSecureLock|FALSE
    gEfiQuarkNcSocIdTokenSpaceGuid.PcdRmuDmaLock|FALSE
  !endif

  !ifdef SECURE_BOOT
    gQuarkPlatformTokenSpaceGuid.PcdSupportSecureBoot|TRUE
  !else
    gQuarkPlatformTokenSpaceGuid.PcdSupportSecureBoot|FALSE
  !endif

  !if $(TARGET) == "RELEASE"
    gQuarkPlatformTokenSpaceGuid.WaitIfResetDueToError|FALSE
  !else
    gQuarkPlatformTokenSpaceGuid.WaitIfResetDueToError|TRUE
  !endif

[PcdsFixedAtBuild]

!ifdef DEBUG_PROPERTY_MASK
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|$(DEBUG_PROPERTY_MASK)
!endif
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision| 0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|"EDK II"
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0x18
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x00
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|0
!if $(PERFORMANCE_ENABLE) == TRUE
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0x1
!endif

  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPerformanceLogEntries|40
  gEfiMdeModulePkgTokenSpaceGuid.PcdLoadModuleAtFixAddressEnable|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdHwErrStorageSize|0x00002000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x1000
  ## RTC Update Timeout Value, need to increase timeout since also
  # waiting for RTC to be busy.
  gEfiMdeModulePkgTokenSpaceGuid.PcdRealTimeClockUpdateTimeout|500000

  gEfiCpuTokenSpaceGuid.PcdTemporaryRamSize|0x4000
  gEfiCpuTokenSpaceGuid.PcdPlatformType|1
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuMaxCoreFrequency|3800
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuMaxFsbFrequency|1066

  gPcAtChipsetPkgTokenSpaceGuid.Pcd8259LegacyModeEdgeLevel|0x0000
  gPcAtChipsetPkgTokenSpaceGuid.Pcd8259LegacyModeMask|0xFFFF

  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdLegacyBiosCacheLegacyRegion|FALSE
!ifdef SECURE_BOOT
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x01|UINT32|0x00000001
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04|UINT32|0x00000002
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04|UINT32|0x00000003
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmPhysicalPresence|FALSE|BOOLEAN|0x00010001
!endif

[PcdsPatchableInModule]
!ifdef DEBUG_PRINT_ERROR_LEVEL
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|$(DEBUG_PRINT_ERROR_LEVEL)
!endif

[PcdsDynamicExHii.common.DEFAULT]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdBootState|L"BootState"|gQuarkPlatformTokenSpaceGuid|0x0|TRUE

[PcdsDynamicExDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdS3BootScriptTablePrivateDataPtr|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdConInConnectOnDemand|FALSE

  # IntelFrameworkModulePkg.dec
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdHardwareErrorRecordLevel|0

  # IA32FamilyCpuBasePkg
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureUserConfiguration|0xffffffff
  gEfiCpuTokenSpaceGuid.PcdIsPowerOnReset|FALSE
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuAssetTags|0x0
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuSocketNames|0x0
  gEfiCpuTokenSpaceGuid.PcdPlatformCpuFrequencyLists|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuConfigContextBuffer|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuMtrrTableAddress|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuPageTableAddress|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureSetting|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuProcessorFeatureCapability|0x0
  gEfiCpuTokenSpaceGuid.PcdCpuCallbackSignal|0x0

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components.IA32]
  #
  # SEC Core
  #
  IA32FamilyCpuBasePkg/SecCore/SecCore.inf
  QuarkPlatformPkg/Cpu/Sec/ResetVector/QuarkResetVector.inf

  #
  # PEI Core
  #
  MdeModulePkg/Core/Pei/PeiMain.inf

  #
  # PEIM
  #
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/ReportStatusCodeRouter/Pei/ReportStatusCodeRouterPei.inf
  MdeModulePkg/Universal/StatusCodeHandler/Pei/StatusCodeHandlerPei.inf
  !ifdef SECURE_BOOT
    SecurityPkg/VariableAuthenticated/Pei/VariablePei.inf
  !else
    MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  !endif
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  IA32FamilyCpuBasePkg/CpuPei/CpuPei.inf
  MdeModulePkg/Universal/CapsulePei/CapsulePei.inf

  QuarkSocPkg/QuarkNorthCluster/MemoryInit/Pei/MemoryInitPei.inf
  QuarkPlatformPkg/Platform/Pei/PlatformInit/PlatformEarlyInit.inf {
    <LibraryClasses>
      PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  }
  QuarkPlatformPkg/Platform/Pei/PlatformInfo/PlatformInfo.inf
  QuarkPlatformPkg/Platform/Pei/PlatformConfig/PlatformConfigPei.inf

  IA32FamilyCpuBasePkg/PiSmmCommunication/PiSmmCommunicationPei.inf

  QuarkPlatformPkg/Override/MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  #
  # S3
  #
  QuarkSocPkg/QuarkNorthCluster/Smm/Pei/SmmAccessPei/SmmAccessPei.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Pei/SmmControlPei/SmmControlPei.inf
  QuarkPlatformPkg/Override/UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf {
    <LibraryClasses>
    !if $(PERFORMANCE_ENABLE) == TRUE
      PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
      TimerLib|IA32FamilyCpuBasePkg/Library/CpuLocalApicTimerLib/CpuLocalApicTimerLib.inf
    !endif
  }

  #
  # Recovery
  #
  QuarkSocPkg/QuarkSouthCluster/Usb/Common/Pei/UsbPei.inf
  MdeModulePkg/Bus/Pci/EhciPei/EhciPei.inf
  QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Pei/OhciPei.inf
  MdeModulePkg/Bus/Usb/UsbBotPei/UsbBotPei.inf
  MdeModulePkg/Bus/Usb/UsbBusPei/UsbBusPei.inf
  FatPkg/FatPei/FatPei.inf
  MdeModulePkg/Universal/Disk/CdExpressPei/CdExpressPei.inf

[Components.IA32]
  #
  # DXE Core
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      NULL|IntelFrameworkModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  QuarkPlatformPkg/Platform/Dxe/PlatformInit/PlatformInitDxe.inf

  #
  # Components that produce the architectural protocols
  #
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf  {
    <LibraryClasses>
      NULL|QuarkPlatformPkg/Library/PlatformUnProvisionedHandlerLib/PlatformUnProvisionedHandlerLib.inf
    !ifdef SECURE_BOOT
      NULL|QuarkPlatformPkg/Library/SecureBootHelperLib/SecureBootHelperLib.inf
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
    !endif

  }
  IA32FamilyCpuBasePkg/CpuArchDxe/CpuArchDxe.inf
  IA32FamilyCpuBasePkg/CpuMpDxe/CpuMpDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  QuarkPlatformPkg/Override/MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteSmm.inf
  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf

  !ifdef SECURE_BOOT
    SecurityPkg/VariableAuthenticated/RuntimeDxe/VariableSmmRuntimeDxe.inf
    SecurityPkg/VariableAuthenticated/RuntimeDxe/VariableSmm.inf
    SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
  !else
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
    QuarkPlatformPkg/Override/MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmm.inf
  !endif  
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf       
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf         
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  QuarkPlatformPkg/Override/PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf

  #     
  # Following are the DXE drivers (alphabetical order)
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  $(PLATFORM_PKG)/Pci/Dxe/PciHostBridge/PciHostBridge.inf
  $(PLATFORM_PKG)/Platform/SpiFvbServices/PlatformSpi.inf
  $(PLATFORM_PKG)/Platform/SpiFvbServices/PlatformSmmSpi.inf

  IntelFrameworkModulePkg/Universal/CpuIoDxe/CpuIoDxe.inf
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf {
    <LibraryClasses>
      IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  }

  #
  # Platform
  #
  QuarkPlatformPkg/Bds/BootManagerMenuApp/BootManagerMenuApp.inf
  QuarkPlatformPkg/Bds/BdsDxe/BdsDxe.inf

  QuarkSocPkg/QuarkNorthCluster/QNCInit/Dxe/QNCInitDxe.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Dxe/SmmAccessDxe/SmmAccess.inf
  QuarkPlatformPkg/Platform/Dxe/Setup/DxePlatform.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }
  QuarkSocPkg/QuarkNorthCluster/Spi/RuntimeDxe/PchSpiRuntime.inf {
    <LibraryClasses>
      PciExpressLib|MdePkg/Library/DxeRuntimePciExpressLib/DxeRuntimePciExpressLib.inf
  }
  QuarkSocPkg/QuarkNorthCluster/Spi/Smm/PchSpiSmm.inf
  QuarkSocPkg/QuarkNorthCluster/S3Support/Dxe/QncS3Support.inf {
    <LibraryClasses>
    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  IntelFrameworkModulePkg/Universal/DataHubDxe/DataHubDxe.inf
  IntelFrameworkModulePkg/Universal/DataHubStdErrDxe/DataHubStdErrDxe.inf
  IntelFrameworkModulePkg/Universal/SectionExtractionDxe/SectionExtractionDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  MdeModulePkg/Universal/ReportStatusCodeRouter/Smm/ReportStatusCodeRouterSmm.inf
  MdeModulePkg/Universal/StatusCodeHandler/Smm/StatusCodeHandlerSmm.inf
  #
  # ACPI
  #
  QuarkPlatformPkg/Platform/Dxe/SaveMemoryConfig/SaveMemoryConfig.inf
  MdeModulePkg/Universal/Acpi/S3SaveStateDxe/S3SaveStateDxe.inf
  QuarkPlatformPkg/Acpi/Dxe/BootScriptExecutorDxe/BootScriptExecutorDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }
  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf
  IntelFrameworkModulePkg/Universal/Acpi/AcpiS3SaveDxe/AcpiS3SaveDxe.inf
  QuarkPlatformPkg/Acpi/AcpiTables/AcpiTables.inf
  QuarkPlatformPkg/Acpi/Dxe/AcpiPlatform/AcpiPlatform.inf

  #
  # SMM
  #
  MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
  MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }
  IA32FamilyCpuBasePkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
  }
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Dxe/SmmControlDxe/SmmControlDxe.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Dxe/SmmRuntime/SmmRuntime.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/DxeSmm/QncSmmDispatcher/QNCSmmDispatcher.inf
  QuarkPlatformPkg/Acpi/DxeSmm/AcpiSmm/AcpiSmmPlatform.inf  {
  <LibraryClasses>
      IoLib|MdePkg/Library/SmmIoLibSmmCpuIo2/SmmIoLibSmmCpuIo2.inf
  }
  QuarkPlatformPkg/Acpi/DxeSmm/SmmPowerManagement/SmmPowerManagement.inf
  QuarkPlatformPkg/Override/MdeModulePkg/Universal/LockBox/SmmLockBox/SmmLockBox.inf
  IA32FamilyCpuBasePkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  #
  # SMBIOS
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
  QuarkPlatformPkg/Platform/Dxe/SmbiosMiscDxe/SmbiosMiscDxe.inf
  QuarkPlatformPkg/Platform/Dxe/MemorySubClass/MemorySubClass.inf
  #
  # PCI
  #
  QuarkPlatformPkg/Pci/Dxe/PciPlatform/PciPlatform.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  QuarkSocPkg/QuarkSouthCluster/IohInit/Dxe/IohInitDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Uart/Dxe/SerialDxe.inf

  #
  # USB
  #
  QuarkPlatformPkg/Override/MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Dxe/OhciDxe.inf
  QuarkPlatformPkg/Override/MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  #
  # SDIO
  #
  QuarkSocPkg/QuarkSouthCluster/Sdio/Dxe/SDControllerDxe/SDControllerDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Sdio/Dxe/SDMediaDeviceDxe/SDMediaDeviceDxe.inf

  #
  # I2C
  #
  QuarkSocPkg/QuarkSouthCluster/I2C/Dxe/I2CDxe.inf
 
  #       
  # IDE/SCSI
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf

  #
  # Console
  #
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # Legacy Modules
  #
  PcAtChipsetPkg/8259InterruptControllerDxe/8259.inf

  #
  # File System Modules
  #
  FatPkg/EnhancedFatDxe/Fat.inf

  #
  # Capsule related drivers
  #
  IntelFrameworkModulePkg/Universal/FirmwareVolume/FwVolDxe/FwVolDxe.inf
  IntelFrameworkModulePkg/Universal/FirmwareVolume/UpdateDriverDxe/UpdateDriverDxe.inf
  QuarkPlatformPkg/Platform/DxeSmm/SMIFlashDxe/SMIFlashDxe.inf

  #
  # Capsule Application
  #
  QuarkPlatformPkg/Applications/CapsuleApp/CapsuleApp.inf {
    <LibraryClasses>
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|ShellPkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      SortLib|ShellPkg/Library/UefiSortLib/UefiSortLib.inf
      DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  }

  #
  # TPM DXE Driver.
  #
  !ifdef TPM_SUPPORT
  QuarkPlatformPkg/Override/SecurityPkg/Tcg/TcgDxe/TcgDxe.inf {
    <LibraryClasses>
      TpmCommLib|QuarkPlatformPkg/Override/SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
  }
  !endif

  #
  # Performance Application
  #
!if $(PERFORMANCE_ENABLE) == TRUE
  PerformancePkg/Dp_App/Dp.inf {
    <LibraryClasses>
      ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
      FileHandleLib|ShellPkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
      SortLib|ShellPkg/Library/UefiSortLib/UefiSortLib.inf
      PerformanceLib|MdeModulePkg/Library/DxeSmmPerformanceLib/DxeSmmPerformanceLib.inf
  }
!endif

###################################################################################################
#
# BuildOptions Section - Define the module specific tool chain flags that should be used as
#                        the default flags for a module. These flags are appended to any
#                        standard flags that are defined by the build process. They can be
#                        applied for any modules or only those modules with the specific
#                        module style (EDK or EDKII) specified in [Components] section.
#
###################################################################################################
#BugID31643 - Start
[BuildOptions.Common.EDKII]
GCC:*_*_IA32_CC_FLAGS    = -include $(WORKSPACE)/QuarkPlatformPkg/Override/MdePkg/Include/Library/DebugLib.h
#BugID31643 - End

