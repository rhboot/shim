/** @file

  Include file for CPU DXE Module

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

  Module Name:  Cpu.h

**/

#ifndef _CPU_DXE_H_
#define _CPU_DXE_H_

#include <PiDxe.h>

#include "Exception.h"
#include "ArchSpecificDef.h"

#include <Protocol/MpService.h>
#include <Guid/StatusCodeDataTypeId.h>
#include <Protocol/Cpu.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/MtrrLib.h>
#include <Library/UefiLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiCpuLib.h>

#include "MemoryAttribute.h"
#include "MtrrSync.h"
#include "Exception.h"

#define INTERRUPT_VECTOR_NUMBER    256
#define INTERRUPT_GATE_ATTRIBUTE   0x8e00

#define APIC_REGISTER_LOCAL_ID_OFFSET         0x20
#define APIC_REGISTER_APIC_VERSION_OFFSET     0x30
#define APIC_REGISTER_SPURIOUS_VECTOR_OFFSET  0xF0
#define APIC_REGISTER_ICR_LOW_OFFSET          0x300
#define APIC_REGISTER_ICR_HIGH_OFFSET         0x310
#define APIC_REGISTER_LINT0_VECTOR_OFFSET     0x350
#define APIC_REGISTER_LINT1_VECTOR_OFFSET     0x360

extern EFI_CPU_ARCH_PROTOCOL       mCpuArchProtocol;
extern UINT64                      mValidMtrrAddressMask;
extern UINT64                      mValidMtrrBitsMask;

/**
  This function flushes CPU data cache.

  This function implements FlushDataCache() service of CPU Architecture Protocol.
  It flushes a range of the processor's data cache.

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.
  @param  Start             Physical address to start flushing from.
  @param  Length            Number of bytes to flush. Round up to chipset granularity.
  @param  FlushType         Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS       Cache is flushed.
  @retval EFI_UNSUPPORTED   Flush type is not supported.
  @retval EFI_DEVICE_ERROR  Requested range could not be flushed..

**/
EFI_STATUS
EFIAPI
FlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      Start,
  IN UINT64                    Length,
  IN EFI_CPU_FLUSH_TYPE        FlushType
  );

/**
  This function enables interrupt processing by the processor.

  This function implements EnableInterrupt() service of CPU Architecture Protocol.
  This function enables interrupt processing by the processor. This function is
  used to implement the Boot Services RaiseTPL() and RestoreTPL().

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR  Interrupts could not be enabled on the processor.

**/
EFI_STATUS
EFIAPI
EnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL     *This
  );

/**
  This function disables interrupt processing by the processor.

  This function implements DisableInterrupt() service of CPU Architecture Protocol.
  This function disables interrupt processing by the processor. This function is
  used to implement the Boot Services RaiseTPL() and RestoreTPL().

  @param  This              The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS       Interrupts are disabled on the processor.
  @retval EFI_DEVICE_ERROR  Interrupts could not be disabled on the processor.

**/
EFI_STATUS
EFIAPI
DisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL     *This
  );

/**
  This function retrieves the processor's current interrupt state.

  This function implements GetInterruptState() service of CPU Architecture Protocol.
  This function retrieves the processor's current interrupt state.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  State                  A pointer to the processor's current interrupt state.
                                 Set to TRUE if interrupts are enabled and FALSE if interrupts are disabled.

  @retval EFI_SUCCESS            The processor's current interrupt state was returned in State.
  @retval EFI_INVALID_PARAMETER  State is NULL.

**/
EFI_STATUS
EFIAPI
GetInterruptStateInstance (
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  OUT BOOLEAN                   *State
  );

/**
  This function generates an INIT on the processor.

  This function implements Init() service of CPU Architecture Protocol.
  This function generates an INIT on the processor.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InitType               The type of processor INIT to perform.

  @retval EFI_SUCCESS            The processor INIT was performed. This return code should never be seen.
  @retval EFI_UNSUPPORTED        The processor INIT operation specified by InitType is not supported by this processor.
  @retval EFI_DEVICE_ERROR       The processor INIT failed.

**/
EFI_STATUS
EFIAPI
Init (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_CPU_INIT_TYPE         InitType
  );

/**
  This function Registers a function to be called from the processor interrupt handler.

  This function implements RegisterInterruptHandler() service of CPU Architecture Protocol.
  This function Registers a function to be called from the processor interrupt handler.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InterruptType          Defines which interrupt or exception to hook.
  @param  InterruptHandler       A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER
                                 that is called when a processor interrupt occurs.
                                 If this parameter is NULL, then the handler will be uninstalled.

  @retval EFI_SUCCESS            The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED    InterruptHandler is not NULL, and a handler for InterruptType was previously installed.
  @retval EFI_INVALID_PARAMETER  InterruptHandler is NULL, and a handler for InterruptType was not previously installed.
  @retval EFI_UNSUPPORTED        The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
EFIAPI
RegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL         *This,
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

/**
  This function returns a timer value from one of the processor's internal timers.

  This function implements GetTimerValue() service of CPU Architecture Protocol.
  This function returns a timer value from one of the processor's internal timers.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  TimerIndex             Specifies which processor timer is to be returned in TimerValue.
                                 This parameter must be between 0 and EFI_CPU_ARCH_PROTOCOL.NumberOfTimers-1.
  @param  TimerValue             Pointer to the returned timer value.
  @param  TimerPeriod            A pointer to the amount of time that passes in femtoseconds for each
                                 increment of TimerValue. If TimerValue does not increment at a predictable
                                 rate, then 0 is returned.

  @retval EFI_SUCCESS            The processor timer value specified by TimerIndex was returned in TimerValue.
  @retval EFI_INVALID_PARAMETER  TimerValue is NULL.
  @retval EFI_INVALID_PARAMETER  TimerIndex is not valid.
  @retval EFI_UNSUPPORTEDT       The processor does not have any readable timers.
  @retval EFI_DEVICE_ERROR       An error occurred attempting to read one of the processor's timers.

**/
EFI_STATUS
EFIAPI
GetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL       *This,
  IN  UINT32                      TimerIndex,
  OUT UINT64                      *TimerValue,
  OUT UINT64                      *TimerPeriod OPTIONAL
  );

/**
  This function attempts to set the attributes for a memory range.

  This function implements SetMemoryAttributes() service of CPU Architecture Protocol.
  This function attempts to set the attributes for a memory range.

  @param  This                   The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress            The physical address that is the start address of a memory region.
  @param  Length                 The size in bytes of the memory region.
  @param  Attributes             The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS            The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER  Length is zero.
  @retval EFI_UNSUPPORTED        The processor does not support one or more bytes of the
                                 memory resource range specified by BaseAddress and Length.
  @retval EFI_UNSUPPORTED        The bit mask of attributes is not support for the memory resource
                                 range specified by BaseAddress and Length.
  @retval EFI_ACCESS_DENIED      The attributes for the memory resource range specified by
                                 BaseAddress and Length cannot be modified.
  @retval EFI_OUT_OF_RESOURCES   There are not enough system resources to modify the attributes of
                                 the memory resource range.

**/
EFI_STATUS
EFIAPI
SetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_PHYSICAL_ADDRESS       BaseAddress,
  IN UINT64                     Length,
  IN UINT64                     Attributes
  );

/**
  Creates an IDT and a new GDT in RAM.
  
  This function creates an IDT and a new GDT in RAM.

**/
VOID
InitializeDescriptorTables (
  VOID
  );

/**
  Creates a new GDT in RAM and load it.
  
  This function creates a new GDT in RAM and load it.

**/
VOID
EFIAPI
InitializeGdt (
  VOID
  );

/**
  Returns the actual CPU core frequency in MHz.

  This function returns the actual CPU core frequency in MHz.

  @param   Frequency              Pointer to the CPU core frequency.
  
  @retval  EFI_SUCCESS            The frequency is returned successfully.
  @retval  EFI_INVALID_PARAMETER  Frequency is NULL.            

**/
EFI_STATUS
GetActualFrequency (
  OUT UINT64                        *Frequency
  );

/**
  Returns the CPU core to processor bus frequency ratio.

  This function returns the CPU core to processor bus frequency ratio.

  @param   Ratio                   Pointer to the CPU core to processor bus frequency ratio.

  @retval  EFI_SUCCESS             The ratio is returned successfully
  @retval  EFI_UNSUPPORTED         The ratio cannot be measured
  @retval  EFI_INVALID_PARAMETER   The input parameter is not valid

**/
EFI_STATUS
GetCpuBusRatio (
  OUT UINT32                        *Ratio
  );

/**
  Converts the actual frequency in MHz to the standard frequency in MHz.

  This function converts the actual frequency in MHz to the standard frequency in MHz.

  @param   Actual              Actual CPU core frequency.
  @param   Ratio               CPU core to processor bus frequency ratio.
  @param   Standard            Standard CPU core frequency.

**/
VOID
Actual2StandardFrequency (
  IN  UINT64                        Actual,
  IN  UINT32                        Ratio,
  OUT UINT64                        *Standard
  );

/**
  Retrieves the value of CS register.

  This function retrieves the value of CS register.

  @return The value of CS register.

**/
UINT16
GetCodeSegment (
  VOID
  );

/**
  Refreshes the GCD Memory Space attributes according to MTRRs.
  
  This function refreshes the GCD Memory Space attributes according to MTRRs.

**/
VOID
RefreshGcdMemoryAttributes (
  VOID
  );

/**
  Label of base address of IDT vector 0.

  This is just a label of base address of IDT vector 0.

**/
VOID
EFIAPI
AsmIdtVector00 (
  VOID
  );

/**
  Initialize the pointer to the external vector table.

  The external vector table is an array of pointers to C based interrupt/exception
  callback routines.

  @param VectorTable   The point to the vector table.
**/
VOID
EFIAPI
InitializeExternalVectorTablePtr(
  EFI_CPU_INTERRUPT_HANDLER* VectorTable
  );

#endif
