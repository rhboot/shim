/** @file
  Serial I/O Port library functions with no library constructor/destructor

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

  Module Name:  IohSerialPortLib.c

**/

#include <PiPei.h>
#include <IohAccess.h>

#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Protocol/SerialIo.h>
#include <IndustryStandard/Pci22.h>
#include "IohSerialPortLib.h"

//
// Routines local to this source module.
//

/**

  Get Uart memory mapped base address.

  Apply relavant mask before returning.

  @return Uart memory mapped base address.

**/
STATIC
UINT32
GetUartMmioBaseAddress (
  VOID
  )
{
  UINT32   RegData32;

  RegData32 = IohMmPci32 (
                0,
                ((UINTN) PcdGet8(PcdIohUartBusNumber)),
                ((UINTN) PcdGet8(PcdIohUartDevNumber)),
                ((UINTN) PcdGet8(PcdIohUartFunctionNumber)),
                PCI_BASE_ADDRESSREG_OFFSET
                );

  return RegData32 & B_IOH_UART_MEMBAR_ADDRESS_MASK;
}

/**

  Ensure SerialPortInitialize has been called and return uart Mmio base addr.

  @return Uart memory mapped base address.

**/
STATIC
UINT32
EnsureSerialPortInitialize (
  VOID
  )
{
  UINT32   UartMmioBase;

  UartMmioBase = GetUartMmioBaseAddress ();
  if (UartMmioBase == 0 || MmioRead8 (UartMmioBase + R_UART_SCR) != UART_SIGNATURE) {
    //
    // Mmio is not intialized or has been reprogrammed
    // So need to re-initialize the base.
    //
    SerialPortInitialize ();
    UartMmioBase = GetUartMmioBaseAddress ();
  }

  return UartMmioBase;
}

//
// Routines exported by this source module.
//

/**

  Programmed hardware of IOH Serial port.

  @return Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
IohInitSerialPort (
  VOID
  )
{
  UINT8    UartFunc;
  UINT8    RegData8;
  UINT8    IohUartBus;
  UINT8    IohUartDev;
  UINT32   UartMmioBase;

  UartFunc   = PcdGet8(PcdIohUartFunctionNumber);
  IohUartBus = PcdGet8(PcdIohUartBusNumber);
  IohUartDev = PcdGet8(PcdIohUartDevNumber);

  //
  // Check PCI memory resource is programmed or not
  //
  UartMmioBase = GetUartMmioBaseAddress ();
  if (UartMmioBase == 0) {
    UartMmioBase = (UINT32) PcdGet64(PcdIohUartMmioBase);
    //
    // Resource not programmed yet, so use predefined tempory memory resource
    //
    PciWrite32 (PCI_LIB_ADDRESS(IohUartBus,  IohUartDev, UartFunc, PCI_BASE_ADDRESSREG_OFFSET), UartMmioBase);
  }

  //
  // Check PCI command register is programmed or not
  //
  RegData8  = PciRead8 (PCI_LIB_ADDRESS(IohUartBus, IohUartDev, UartFunc, PCI_COMMAND_OFFSET));
  if ((RegData8 & EFI_PCI_COMMAND_MEMORY_SPACE) == 0) {
    //
    // Command register is not enabled, so enable it
    //    
    PciWrite8 (PCI_LIB_ADDRESS(IohUartBus, IohUartDev, UartFunc, PCI_COMMAND_OFFSET), EFI_PCI_COMMAND_MEMORY_SPACE);
  }  

  MmioWrite8 (UartMmioBase + R_UART_SCR, UART_SIGNATURE);

  return EFI_SUCCESS;
}

/**

  Programmed serial port controller.

  @return Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  )
{
  UINT32  Divisor;
  UINT8   OutputData;
  UINT32  UartMmioBase;

  UartMmioBase = GetUartMmioBaseAddress ();

  if (UartMmioBase != 0 && MmioRead8 (UartMmioBase + R_UART_SCR) == UART_SIGNATURE) {
    return EFI_SUCCESS;
  }

  //
  // Some init is done by the platform status code initialization.
  //
  IohInitSerialPort ();
  UartMmioBase = GetUartMmioBaseAddress ();  // Ensure correct Mmio Base.

  //
  // Calculate divisor for baud generator
  //
  Divisor = (UINT32)(((UINTN) PcdGet32 (PcdIohUartClkFreq)) / (UINTN)(PcdGet64 (PcdUartDefaultBaudRate)) / 16);

  //
  // Set communications format
  //
  switch (PcdGet8(PcdUartDefaultDataBits)) {
  case 5:
    OutputData = 0;
    break;

  case 6:
    OutputData = 1;
    break;

  case 7:
    OutputData = 2;
    break;

  case 8:
    OutputData = 3;
    break;

  default:
    OutputData = 0;
    break;
  }

  switch (PcdGet8(PcdUartDefaultStopBits)) {
  case OneFiveStopBits:
  case TwoStopBits:
    OutputData |= 0x04;
    break;

  case OneStopBit:
  default:
    OutputData &= 0xFB;
    break;
  }

  switch (PcdGet8(PcdUartDefaultParity)) {
  case EvenParity:
    OutputData |= 0x18;
    break;

  case OddParity:
    OutputData |= 0x08;
    OutputData &= 0xCF;
    break;

  case MarkParity:
    OutputData |= 0x28;
    break;

  case SpaceParity:
    OutputData |= 0x38;
    break;

  case NoParity:
  default:
    OutputData &= 0xF7;
  }  

  MmioWrite8 (
    UartMmioBase + R_UART_LCR,
    (UINT8)(OutputData | B_UARY_LCR_DLAB)
    );

  //
  // Configure baud rate
  //
  MmioWrite8 (
    UartMmioBase + R_UART_BAUD_HIGH,
    (UINT8) (Divisor >> 8)
    );

  MmioWrite8 (
    UartMmioBase + R_UART_BAUD_LOW,
    (UINT8) (Divisor & 0xff)
    );

  //
  // Switch back to bank 0
  //
  MmioWrite8 (
    UartMmioBase + R_UART_LCR,
    OutputData
    );

  //
  // Enable FIFO, reset receive FIFO and transmit FIFO
  //
  MmioWrite8 (
    UartMmioBase + R_UART_FCR,
    (UINT8) (B_UARY_FCR_TRFIFIE | B_UARY_FCR_RESETRF | B_UARY_FCR_RESETTF)
    );
   
  return EFI_SUCCESS;
}

/**
  Write data to serial device.

  If the buffer is NULL, then return 0;
  if NumberOfBytes is zero, then return 0.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes writed to serial device.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
)
{
  UINTN Result;
  UINT32  UartMmioBase;

  if (NULL == Buffer) {
    return 0;
  }

  UartMmioBase = EnsureSerialPortInitialize ();

  Result = NumberOfBytes;
   
  while (NumberOfBytes--) {         
    //
    // Wait for the serail port to be ready, to make sure both the transmit FIFO
    // and shift register empty.
    //
    while ((MmioRead8 (UartMmioBase + R_UART_LSR) & B_UART_LSR_TEMT) == 0);
    
    MmioWrite8 (UartMmioBase + R_UART_BAUD_THR, *Buffer++);      
  }

  return Result;
}


/*
  Read data from serial device and save the datas in buffer.

  If the buffer is NULL, then return 0;
  if NumberOfBytes is zero, then return 0.

  @param  Buffer           Point of data buffer which need to be writed.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Actual number of bytes raed to serial device.

**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
)
{
  UINTN Result;
  UINT32  UartMmioBase;

  if (NULL == Buffer) {
    return 0;
  }
  UartMmioBase = EnsureSerialPortInitialize ();

  Result = NumberOfBytes;

  while (NumberOfBytes--) {          
    //
    // Wait for the serail port to be ready.
    //
    while ((MmioRead8 (UartMmioBase + R_UART_LSR) & B_UART_LSR_RXRDY) == 0);

    *Buffer++ = MmioRead8 (UartMmioBase + R_UART_BAUD_THR); 
  }
  
  return Result;
}


/**
  Poll the serial device to see if there is any data waiting.

  If there is data waiting to be read from the serial port, then return
  TRUE.  If there is no data waiting to be read from the serial port, then
  return FALSE.

  @retval FALSE            There is no data waiting to be read.
  @retval TRUE             Data is waiting to be read.

**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  )
{
  UINT32  UartMmioBase;

  UartMmioBase = EnsureSerialPortInitialize ();

  //
  // Read the serial port status
  //
  return (BOOLEAN)((MmioRead8 (UartMmioBase + R_UART_LSR) & B_UART_LSR_RXRDY) != 0);
}
