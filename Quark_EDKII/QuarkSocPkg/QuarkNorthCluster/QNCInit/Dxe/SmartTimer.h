/** @file
  Declaration of SmartTimer specific functions and Macros

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

 Module Name: SmartTimer.h

*/

#ifndef _SMART_TIMER_H_
#define _SMART_TIMER_H_
#include "CommonHeader.h"

//
// 8254 Timer
//
#define TIMER0_COUNT_PORT                         0x40
#define TIMER1_COUNT_PORT                         0x41
#define TIMER2_COUNT_PORT                         0x42
#define TIMER_CONTROL_PORT                        0x43

//
// The PCAT 8253/8254 has an input clock at 1.193182 MHz and Timer 0 is
// initialized as a 16 bit free running counter that generates an interrupt(IRQ0)
// each time the counter rolls over.
//
//   65536 counts
// ---------------- * 1,000,000 uS/S = 54925.4 uS = 549254 * 100 ns
//   1,193,182 Hz
//
#define DEFAULT_TIMER_TICK_DURATION 549254
#define TIMER0_CONTROL_WORD         0x36      //Time0, Read/Write LSB then MSB, Square wave output, binary count use.

//
// ACPI timer factor
//
#define ACPI_TIMER_FACTOR           46869689    //  10*0x1000000/3.579545, the 3.579545 MHz is the ACPI timer's clock;

/**
  This function registers the handler NotifyFunction so it is called every time
  the timer interrupt fires.  It also passes the amount of time since the last
  handler call to the NotifyFunction.  If NotifyFunction is NULL, then the
  handler is unregistered.  If the handler is registered, then EFI_SUCCESS is
  returned.  If the CPU does not support registering a timer interrupt handler,
  then EFI_UNSUPPORTED is returned.  If an attempt is made to register a handler
  when a handler is already registered, then EFI_ALREADY_STARTED is returned.
  If an attempt is made to unregister a handler when a handler is not registered,
  then EFI_INVALID_PARAMETER is returned.  If an error occurs attempting to
  register the NotifyFunction with the timer interrupt, then EFI_DEVICE_ERROR
  is returned.

  @param  This           - The EFI_TIMER_ARCH_PROTOCOL instance.

  @param  NotifyFunction - The function to call when a timer interrupt fires.  This
                   function executes at TPL_HIGH_LEVEL.  The DXE Core will
                   register a handler for the timer interrupt, so it can know
                   how much time has passed.  This information is used to
                   signal timer based events.  NULL will unregister the handler.

  @retval    EFI_SUCCESS           - The timer handler was registered.
  @retval    EFI_UNSUPPORTED       - The platform does not support timer interrupts.
  @retval    EFI_ALREADY_STARTED   - NotifyFunction is not NULL, and a handler is already
                                   registered.
  @retval    EFI_INVALID_PARAMETER - NotifyFunction is NULL, and a handler was not
                                   previously registered.
  @retval    EFI_DEVICE_ERROR      - The timer handler could not be registered.

*/
EFI_STATUS
EFIAPI
TimerDriverRegisterHandler (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN EFI_TIMER_NOTIFY         NotifyFunction
  );

/**
  This function adjusts the period of timer interrupts to the value specified
  by TimerPeriod.  If the timer period is updated, then the selected timer
  period is stored in EFI_TIMER.TimerPeriod, and EFI_SUCCESS is returned.  If
  the timer hardware is not programmable, then EFI_UNSUPPORTED is returned.
  If an error occurs while attempting to update the timer period, then the
  timer hardware will be put back in its state prior to this call, and
  EFI_DEVICE_ERROR is returned.  If TimerPeriod is 0, then the timer interrupt
  is disabled.  This is not the same as disabling the CPU's interrupts.
  Instead, it must either turn off the timer hardware, or it must adjust the
  interrupt controller so that a CPU interrupt is not generated when the timer
  interrupt fires.

  @param  This        - The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod - The rate to program the timer interrupt in 100 nS units.  If
                the timer hardware is not programmable, then EFI_UNSUPPORTED is
                returned.  If the timer is programmable, then the timer period
                will be rounded up to the nearest timer period that is supported
                by the timer hardware.  If TimerPeriod is set to 0, then the
                timer interrupts will be disabled.

  @retval  EFI_SUCCESS      - The timer period was changed.
  @retval  EFI_UNSUPPORTED  - The platform cannot change the period of the timer interrupt.
  @retval  EFI_DEVICE_ERROR - The timer period could not be changed due to a device error.

*/
EFI_STATUS
EFIAPI
TimerDriverSetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL  *This,
  IN UINT64                   TimerPeriod
  );


/**
  This function retrieves the period of timer interrupts in 100 ns units,
  returns that value in TimerPeriod, and returns EFI_SUCCESS.  If TimerPeriod
  is NULL, then EFI_INVALID_PARAMETER is returned.  If a TimerPeriod of 0 is
  returned, then the timer is currently disabled.

  @param  This        - The EFI_TIMER_ARCH_PROTOCOL instance.
  @param  TimerPeriod - A pointer to the timer period to retrieve in 100 ns units.  If
                0 is returned, then the timer is currently disabled.

  @retval  EFI_SUCCESS           - The timer period was returned in TimerPeriod.
  @retval  EFI_INVALID_PARAMETER - TimerPeriod is NULL.

*/
EFI_STATUS
EFIAPI
TimerDriverGetTimerPeriod (
  IN EFI_TIMER_ARCH_PROTOCOL   *This,
  OUT UINT64                   *TimerPeriod
  );

/**
  This function generates a soft timer interrupt. If the platform does not support soft
  timer interrupts, then EFI_UNSUPPORTED is returned. Otherwise, EFI_SUCCESS is returned.
  If a handler has been registered through the EFI_TIMER_ARCH_PROTOCOL.RegisterHandler()
  service, then a soft timer interrupt will be generated. If the timer interrupt is
  enabled when this service is called, then the registered handler will be invoked. The
  registered handler should not be able to distinguish a hardware-generated timer
  interrupt from a software-generated timer interrupt.

  @param  This   The EFI_TIMER_ARCH_PROTOCOL instance.

  @retval  EFI_SUCCESS       - The soft timer interrupt was generated.
  @retval  EFI_UNSUPPORTEDT  - The platform does not support the generation of soft timer interrupts.

*/
EFI_STATUS
EFIAPI
TimerDriverGenerateSoftInterrupt (
  IN EFI_TIMER_ARCH_PROTOCOL  *This
  );
/**
  Initialize the Timer Architectural Protocol driver

  @param  ImageHandle - ImageHandle of the loaded driver
  @param  SystemTable - Pointer to the System Table

  @retval  EFI_SUCCESS           - Timer Architectural Protocol created
  @retval  EFI_OUT_OF_RESOURCES  - Not enough resources available to initialize driver.
  @retval  EFI_DEVICE_ERROR      - A device error occured attempting to initialize the driver.

*/
EFI_STATUS
EFIAPI
TimerDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
