/** @file

  Include files of common functions for CPU DXE module.

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

  Module Name:  MpCommon.h

**/

#ifndef _MP_COMMON_H_
#define _MP_COMMON_H_

#include "ArchSpecificDef.h"
#include <AcpiCpuData.h>
#include <IndustryStandard/SmBios.h>

#include <Protocol/Smbios.h>
#include <Protocol/MpService.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/Cpu.h>
#include <Protocol/SmmConfiguration.h>
#include <Protocol/Timer.h>
#include <Protocol/TcgService.h>

#include <Guid/StatusCodeDataTypeId.h>
#include <Guid/HtBistHob.h>
#include <Guid/EventGroup.h>
#include <Guid/IdleLoopEvent.h>

#include <Library/BaseLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/CpuConfigLib.h>
#include <Library/CpuLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/CpuOnlyResetLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/MtrrLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LocalApicLib.h>
#include <Library/PrintLib.h>

#define INTERRUPT_HANDLER_MACHINE_CHECK       0x12

#define VACANT_FLAG           0x00
#define NOT_VACANT_FLAG       0xff

#define MICROSECOND         10

#pragma pack(1)

typedef  struct {
  UINT16  LimitLow;
  UINT16  BaseLow;
  UINT8   BaseMiddle;
  UINT8   Attributes1;
  UINT8	  Attributes2;	
  UINT8   BaseHigh;
} SEGMENT_DESCRIPTOR;

#pragma pack()

#define STARTUP_AP_SIGNAL        0x6E750000

typedef enum {
  ApInHltLoop   = 1,
  ApInMwaitLoop = 2,
  ApInRunLoop   = 3
} AP_LOOP_MODE;

typedef struct {
  UINT32                    StartupApSignal;
  UINT32                    MwaitTargetCstate;
  AP_LOOP_MODE              ApLoopMode;
  BOOLEAN                   ReadyToBootFlag;
  BOOLEAN                   CStateEnable;
} MONITOR_MWAIT_DATA;

//
// The MONITOR_FILTER_SIZE be mulitple of 16 bytes,
// to make sure AP's stack is 16-byte alignment on x64 arch.
//
#define MONITOR_FILTER_SIZE      sizeof(MONITOR_MWAIT_DATA)

typedef struct {
  UINT32                    ApicId;
  UINT32                    Bist;
} BIST_INFO;

//
// MTRR table definitions
//
typedef struct {
  UINT16  Index;
  UINT64  Value;
} EFI_MTRR_VALUES;

typedef
VOID
(EFIAPI *AP_PROC_ENTRY) (
  IN UINTN  ProcessorNumber
  );

typedef
VOID
(*AP_PROCEDURE) (
  IN UINTN  ProcessorNumber
  );

//
// Note that this structure is located immediately after the AP startup code
// in the wakeup buffer. this structure is not always writeable if the wakeup
// buffer is in the legacy region.
//
typedef struct {
  UINTN             Lock;
  VOID              *StackStart;
  UINTN             StackSize;
  AP_PROC_ENTRY     ApFunction;
  IA32_DESCRIPTOR   GdtrProfile;
  IA32_DESCRIPTOR   IdtrProfile;
  UINT32            BufferStart;
  UINT32            Cr3;
  UINT32            InitFlag;
  UINT32            ApCount;
  AP_LOOP_MODE      ApLoopMode;
  BIST_INFO         BistBuffer[FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber)];
} MP_CPU_EXCHANGE_INFO;

extern MP_CPU_EXCHANGE_INFO        *mExchangeInfo;
extern AP_PROCEDURE                mApFunction;
extern CPU_CONFIG_CONTEXT_BUFFER   mCpuConfigConextBuffer;
extern EFI_PHYSICAL_ADDRESS        mApMachineCheckHandlerBase;
extern UINT32                      mApMachineCheckHandlerSize;
extern UINTN                       mLocalApicTimerDivisor;
extern UINT32                      mLocalApicTimerInitialCount;

/**
  Allocates startup vector for APs.

  This function allocates Startup vector for APs.

  @param  Size  The size of startup vector.

**/
VOID
AllocateStartupVector (
  UINTN   Size
  );

/**
  Creates a copy of GDT and IDT for all APs.

  This function creates a copy of GDT and IDT for all APs.

  @param  Gdtr   Base and limit of GDT for AP
  @param  Idtr   Base and limit of IDT for AP

**/
VOID
PrepareGdtIdtForAP (
  OUT IA32_DESCRIPTOR  *Gdtr,
  OUT IA32_DESCRIPTOR  *Idtr
  );

/**
  Allocate aligned ACPI NVS memory below 4G.
  
  This function allocates aligned ACPI NVS memory below 4G.

  @param  Size       Size of memory region to allocate
  @param  Alignment  Alignment in bytes
  
  @return Base address of the allocated region

**/
VOID*
AllocateAlignedAcpiNvsMemory (
  IN  UINTN         Size,
  IN  UINTN         Alignment
  );

/**
  Prepares Startup Vector for APs.

  This function prepares Startup Vector for APs.

**/
VOID
PrepareAPStartupVector (
  VOID
  );

/**
  Sets specified IDT entry with given function pointer.

  This function sets specified IDT entry with given function pointer.

  @param  FunctionPointer  Function pointer for IDT entry.
  @param  IdtEntry         The IDT entry to update.

  @return The original IDT entry value.

**/
UINTN
SetIdtEntry (
  IN  UINTN                       FunctionPointer,
  OUT INTERRUPT_GATE_DESCRIPTOR   *IdtEntry
  );

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  );

/**
  Sends INIT-SIPI-SIPI to specific AP, and make it work on procedure specified by ApFunction.

  This function sends INIT-SIPI-SIPI to specific AP, and make it work on procedure specified by ApFunction.

  @param  Broadcast   If TRUE, broadcase IPI to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP.
  @param  ApFunction  The procedure for all APs to work on.

**/
VOID
SendInitSipiSipiIpis (
  IN BOOLEAN            Broadcast,
  IN UINT32             ApicID,
  IN AP_PROCEDURE       ApFunction
  );

/**
  Dispatches task to AP.

  This function dispatches task to AP. The BSP waits until specified APs have finished.

  @param  Broadcast   If TRUE, send task to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP. If Broadcast is TRUE, it is ignored.
  @param  Procedure   The procedure for AP to work on.

**/
VOID
DispatchAPAndWait (
  IN BOOLEAN             Broadcast,
  IN UINT32              ApicID,
  IN AP_PROCEDURE        Procedure
  );

/**
  Installs MP Services Protocol and register related timer event.
  
  This function installs MP Services Protocol and register related timer event.

**/
VOID
InstallMpServicesProtocol (
  VOID
  );

/**
  Protocol notification that is fired when LegacyBios protocol is installed.

  Re-allocate a wakeup buffer from E/F segment because the previous wakeup buffer
  under 640K won't be preserved by the legacy OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ReAllocateMemoryForAP (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Fixup jump instructions in the AP startup code.

  @param AddressMap    Pointer to MP_ASSEMBLY_ADDRESS_MAP.
  @param TargetBuffer  Target address of the startup vector.
**/
VOID
RedirectFarJump (
  IN MP_ASSEMBLY_ADDRESS_MAP *AddressMap,
  IN EFI_PHYSICAL_ADDRESS    TargetBuffer
  );

/**
  Protocol notification that will wake up and place AP to the suitable state
  before booting to OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ChangeApLoopModeCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Get mointor data address for specified processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

  @return Pointer to monitor data.
**/
MONITOR_MWAIT_DATA *
EFIAPI
GetMonitorDataAddress (
  IN UINTN  ProcessorNumber
  );

#endif
