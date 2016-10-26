/** @file
  Include for Serial Driver

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

#ifndef _SERIAL_H_
#define _SERIAL_H_


#include <FrameworkDxe.h>

#include <Protocol/IsaIo.h>
#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePathToText.h>

#include <Guid/GlobalVariable.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

#include <IndustryStandard/Pci22.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL  gSerialControllerDriver;
extern EFI_COMPONENT_NAME_PROTOCOL  gIohSerialComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gIohSerialComponentName2;

//
// Internal Data Structures
//
#define SERIAL_DEV_SIGNATURE    SIGNATURE_32 ('s', 'e', 'r', 'd')
#define SERIAL_MAX_BUFFER_SIZE  16
#define TIMEOUT_STALL_INTERVAL  10

//
//  Name:   SERIAL_DEV_FIFO
//  Purpose:  To define Receive FIFO and Transmit FIFO
//  Context:  Used by serial data transmit and receive
//  Fields:
//      First UINT32: The index of the first data in array Data[]
//      Last  UINT32: The index, which you can put a new data into array Data[]
//      Surplus UINT32: Identify how many data you can put into array Data[]
//      Data[]  UINT8 : An array, which used to store data
//
typedef struct {
  UINT32  First;
  UINT32  Last;
  UINT32  Surplus;
  UINT8   Data[SERIAL_MAX_BUFFER_SIZE];
} SERIAL_DEV_FIFO;

typedef enum {
  Uart8250  = 0,
  Uart16450 = 1,
  Uart16550 = 2,
  Uart16550A= 3
} EFI_UART_TYPE;

//
//  Name:   SERIAL_DEV
//  Purpose:  To provide device specific information
//  Context:
//  Fields:
//      Signature UINTN: The identity of the serial device
//      SerialIo  SERIAL_IO_PROTOCOL: Serial I/O protocol interface
//      SerialMode  SERIAL_IO_MODE:
//      DevicePath  EFI_DEVICE_PATH_PROTOCOL *: Device path of the serial device
//      Handle      EFI_HANDLE: The handle instance attached to serial device
//      BaseAddress UINT16: The base address of specific serial device
//      Receive     SERIAL_DEV_FIFO: The FIFO used to store data,
//                  which is received by UART
//      Transmit    SERIAL_DEV_FIFO: The FIFO used to store data,
//                  which you want to transmit by UART
//      SoftwareLoopbackEnable BOOLEAN:
//      Type    EFI_UART_TYPE: Specify the UART type of certain serial device
//
typedef struct {
  UINTN                                  Signature;

  EFI_HANDLE                             Handle;
  EFI_SERIAL_IO_PROTOCOL                 SerialIo;
  EFI_SERIAL_IO_MODE                     SerialMode;
  EFI_DEVICE_PATH_PROTOCOL               *DevicePath;

  EFI_DEVICE_PATH_PROTOCOL               *ParentDevicePath;
  UART_DEVICE_PATH                       UartDevicePath;

  UINT32                                 BaseAddress;
  SERIAL_DEV_FIFO                        Receive;
  SERIAL_DEV_FIFO                        Transmit;
  BOOLEAN                                SoftwareLoopbackEnable;
  BOOLEAN                                HardwareFlowControl;
  EFI_UART_TYPE                          Type;
  EFI_UNICODE_STRING_TABLE               *ControllerNameTable;
} SERIAL_DEV;

#define SERIAL_DEV_FROM_THIS(a) CR (a, SERIAL_DEV, SerialIo, SERIAL_DEV_SIGNATURE)

//
// Serial Driver Defaults
//
#define SERIAL_PORT_DEFAULT_RECEIVE_FIFO_DEPTH  1
#define SERIAL_PORT_DEFAULT_TIMEOUT             1000000
#define SERIAL_PORT_DEFAULT_CONTROL_MASK        0

#define SYNOPSIS_UART0_DID	0x0936

//
// 115200 baud with rounding errors
//
#define SERIAL_PORT_MAX_BAUD_RATE           115400
#define SERIAL_PORT_MIN_BAUD_RATE           50

#define SERIAL_PORT_MAX_RECEIVE_FIFO_DEPTH  16
#define SERIAL_PORT_MIN_TIMEOUT             1         // 1 uS
#define SERIAL_PORT_MAX_TIMEOUT             100000000 // 100 seconds
//
// UART Registers
//
#define SERIAL_REGISTER_THR 0 // WO   Transmit Holding Register
#define SERIAL_REGISTER_RBR 0 // RO   Receive Buffer Register
#define SERIAL_REGISTER_DLL 0 // R/W  Divisor Latch LSB
#define SERIAL_REGISTER_DLM 4 // R/W  Divisor Latch MSB
#define SERIAL_REGISTER_IER 4 // R/W  Interrupt Enable Register
#define SERIAL_REGISTER_IIR 8 // RO   Interrupt Identification Register
#define SERIAL_REGISTER_FCR 8 // WO   FIFO Cotrol Register
#define SERIAL_REGISTER_LCR 12 // R/W  Line Control Register
#define SERIAL_REGISTER_MCR 16 // R/W  Modem Control Register
#define SERIAL_REGISTER_LSR 20 // R/W  Line Status Register
#define SERIAL_REGISTER_MSR 24 // R/W  Modem Status Register
#define SERIAL_REGISTER_SCR 28 // R/W  Scratch Pad Register
#pragma pack(1)
//
//  Name:   SERIAL_PORT_IER_BITS
//  Purpose:  Define each bit in Interrupt Enable Register
//  Context:
//  Fields:
//     Ravie  Bit0: Receiver Data Available Interrupt Enable
//     Theie  Bit1: Transmistter Holding Register Empty Interrupt Enable
//     Rie      Bit2: Receiver Interrupt Enable
//     Mie      Bit3: Modem Interrupt Enable
//     Reserved Bit4-Bit7: Reserved
//
typedef struct {
  UINT8 Ravie : 1;
  UINT8 Theie : 1;
  UINT8 Rie : 1;
  UINT8 Mie : 1;
  UINT8 Reserved : 4;
} SERIAL_PORT_IER_BITS;

//
//  Name:   SERIAL_PORT_IER
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_IER_BITS:  Bits of the IER
//      Data    UINT8: the value of the IER
//
typedef union {
  SERIAL_PORT_IER_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_IER;

//
//  Name:   SERIAL_PORT_FCR_BITS
//  Purpose:  Define each bit in FIFO Control Register
//  Context:
//  Fields:
//      TrFIFOE    Bit0: Transmit and Receive FIFO Enable
//      ResetRF    Bit1: Reset Reciever FIFO
//      ResetTF    Bit2: Reset Transmistter FIFO
//      Dms        Bit3: DMA Mode Select
//      Reserved   Bit4-Bit5: Reserved
//      Rtb        Bit6-Bit7: Receive Trigger Bits
//
typedef struct {
  UINT8 TrFIFOE : 1;
  UINT8 ResetRF : 1;
  UINT8 ResetTF : 1;
  UINT8 Dms : 1;
  UINT8 Reserved : 2;
  UINT8 Rtb : 2;
} SERIAL_PORT_FCR_BITS;

//
//  Name:   SERIAL_PORT_FCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_FCR_BITS:  Bits of the FCR
//      Data    UINT8: the value of the FCR
//
typedef union {
  SERIAL_PORT_FCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_FCR;

//
//  Name:   SERIAL_PORT_LCR_BITS
//  Purpose:  Define each bit in Line Control Register
//  Context:
//  Fields:
//      SerialDB  Bit0-Bit1: Number of Serial Data Bits
//      StopB     Bit2: Number of Stop Bits
//      ParEn     Bit3: Parity Enable
//      EvenPar   Bit4: Even Parity Select
//      SticPar   Bit5: Sticky Parity
//      BrCon     Bit6: Break Control
//      DLab      Bit7: Divisor Latch Access Bit
//
typedef struct {
  UINT8 SerialDB : 2;
  UINT8 StopB : 1;
  UINT8 ParEn : 1;
  UINT8 EvenPar : 1;
  UINT8 SticPar : 1;
  UINT8 BrCon : 1;
  UINT8 DLab : 1;
} SERIAL_PORT_LCR_BITS;

//
//  Name:   SERIAL_PORT_LCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_LCR_BITS:  Bits of the LCR
//      Data    UINT8: the value of the LCR
//
typedef union {
  SERIAL_PORT_LCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_LCR;

//
//  Name:   SERIAL_PORT_MCR_BITS
//  Purpose:  Define each bit in Modem Control Register
//  Context:
//  Fields:
//      DtrC     Bit0: Data Terminal Ready Control
//      Rts      Bit1: Request To Send Control
//      Out1     Bit2: Output1
//      Out2     Bit3: Output2, used to disable interrupt
//      Lme;     Bit4: Loopback Mode Enable
//      Reserved Bit5-Bit7: Reserved
//
typedef struct {
  UINT8 DtrC : 1;
  UINT8 Rts : 1;
  UINT8 Out1 : 1;
  UINT8 Out2 : 1;
  UINT8 Lme : 1;
  UINT8 Reserved : 3;
} SERIAL_PORT_MCR_BITS;

//
//  Name:   SERIAL_PORT_MCR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_MCR_BITS:  Bits of the MCR
//      Data    UINT8: the value of the MCR
//
typedef union {
  SERIAL_PORT_MCR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_MCR;

//
//  Name:   SERIAL_PORT_LSR_BITS
//  Purpose:  Define each bit in Line Status Register
//  Context:
//  Fields:
//      Dr    Bit0: Receiver Data Ready Status
//      Oe    Bit1: Overrun Error Status
//      Pe    Bit2: Parity Error Status
//      Fe    Bit3: Framing Error Status
//      Bi    Bit4: Break Interrupt Status
//      Thre  Bit5: Transmistter Holding Register Status
//      Temt  Bit6: Transmitter Empty Status
//      FIFOe Bit7: FIFO Error Status
//
typedef struct {
  UINT8 Dr : 1;
  UINT8 Oe : 1;
  UINT8 Pe : 1;
  UINT8 Fe : 1;
  UINT8 Bi : 1;
  UINT8 Thre : 1;
  UINT8 Temt : 1;
  UINT8 FIFOe : 1;
} SERIAL_PORT_LSR_BITS;

//
//  Name:   SERIAL_PORT_LSR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_LSR_BITS:  Bits of the LSR
//      Data    UINT8: the value of the LSR
//
typedef union {
  SERIAL_PORT_LSR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_LSR;

//
//  Name:   SERIAL_PORT_MSR_BITS
//  Purpose:  Define each bit in Modem Status Register
//  Context:
//  Fields:
//      DeltaCTS      Bit0: Delta Clear To Send Status
//      DeltaDSR        Bit1: Delta Data Set Ready Status
//      TrailingEdgeRI  Bit2: Trailing Edge of Ring Indicator Status
//      DeltaDCD        Bit3: Delta Data Carrier Detect Status
//      Cts             Bit4: Clear To Send Status
//      Dsr             Bit5: Data Set Ready Status
//      Ri              Bit6: Ring Indicator Status
//      Dcd             Bit7: Data Carrier Detect Status
//
typedef struct {
  UINT8 DeltaCTS : 1;
  UINT8 DeltaDSR : 1;
  UINT8 TrailingEdgeRI : 1;
  UINT8 DeltaDCD : 1;
  UINT8 Cts : 1;
  UINT8 Dsr : 1;
  UINT8 Ri : 1;
  UINT8 Dcd : 1;
} SERIAL_PORT_MSR_BITS;

//
//  Name:   SERIAL_PORT_MSR
//  Purpose:
//  Context:
//  Fields:
//      Bits    SERIAL_PORT_MSR_BITS:  Bits of the MSR
//      Data    UINT8: the value of the MSR
//
typedef union {
  SERIAL_PORT_MSR_BITS  Bits;
  UINT8                 Data;
} SERIAL_PORT_MSR;

#pragma pack()
//
// Define serial register I/O macros
//
#define READ_RBR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_RBR)
#define READ_DLL(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_DLL)
#define READ_DLM(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_DLM)
#define READ_IER(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_IER)
#define READ_IIR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_IIR)
#define READ_LCR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_LCR)
#define READ_MCR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_MCR)
#define READ_LSR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_LSR)
#define READ_MSR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_MSR)
#define READ_SCR(DEV)     IohSerialReadPort (DEV, SERIAL_REGISTER_SCR)

#define WRITE_THR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_THR, D)
#define WRITE_DLL(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_DLL, D)
#define WRITE_DLM(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_DLM, D)
#define WRITE_IER(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_IER, D)
#define WRITE_FCR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_FCR, D)
#define WRITE_LCR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_LCR, D)
#define WRITE_MCR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_MCR, D)
#define WRITE_LSR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_LSR, D)
#define WRITE_MSR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_MSR, D)
#define WRITE_SCR(DEV, D) IohSerialWritePort (DEV, SERIAL_REGISTER_SCR, D)

//
// Prototypes
// Driver model protocol interface
//
/**
  Check to see if this driver supports the given controller

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS          This driver can support the given controller

**/
EFI_STATUS
EFIAPI
SerialControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Start to management the controller passed in

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller           The handle of the controller to test.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path.

  @return EFI_SUCCESS          Driver is started successfully
**/
EFI_STATUS
EFIAPI
SerialControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Disconnect this driver with the controller, uninstall related protocol instance

  @param  This                  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The handle of the controller to test.
  @param  NumberOfChildren      Number of child device.
  @param  ChildHandleBuffer     A pointer to the remaining portion of a device path.

  @retval EFI_SUCCESS           Operation successfully
  @retval EFI_DEVICE_ERROR      Cannot stop the driver successfully

**/
EFI_STATUS
EFIAPI
SerialControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );

//
// Serial I/O Protocol Interface
//
/**
  Reset serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL

  @retval EFI_SUCCESS        Reset successfully
  @retval EFI_DEVICE_ERROR   Failed to reset

**/
EFI_STATUS
EFIAPI
IohSerialReset (
  IN EFI_SERIAL_IO_PROTOCOL         *This
  );

/**
  Set new attributes to a serial device.

  @param This                     Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BaudRate                 The baudrate of the serial device
  @param  ReceiveFifoDepth         The depth of receive FIFO buffer
  @param  Timeout                  The request timeout for a single char
  @param  Parity                   The type of parity used in serial device
  @param  DataBits                 Number of databits used in serial device
  @param  StopBits                 Number of stopbits used in serial device

  @retval  EFI_SUCCESS              The new attributes were set
  @retval  EFI_INVALID_PARAMETERS   One or more attributes have an unsupported value
  @retval  EFI_UNSUPPORTED          Data Bits can not set to 5 or 6
  @retval  EFI_DEVICE_ERROR         The serial device is not functioning correctly (no return)

**/
EFI_STATUS
EFIAPI
IohSerialSetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN UINT64                         BaudRate,
  IN UINT32                         ReceiveFifoDepth,
  IN UINT32                         Timeout,
  IN EFI_PARITY_TYPE                Parity,
  IN UINT8                          DataBits,
  IN EFI_STOP_BITS_TYPE             StopBits
  );

/**
  Set Control Bits.

  @param This              Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control           Control bits that can be settable

  @retval EFI_SUCCESS       New Control bits were set successfully
  @retval EFI_UNSUPPORTED   The Control bits wanted to set are not supported

**/
EFI_STATUS
EFIAPI
IohSerialSetControl (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN UINT32                         Control
  );

/**
  Get ControlBits.

  @param This          Pointer to EFI_SERIAL_IO_PROTOCOL
  @param Control       Control signals of the serial device

  @retval EFI_SUCCESS   Get Control signals successfully

**/
EFI_STATUS
EFIAPI
IohSerialGetControl (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  OUT UINT32                        *Control
  );

/**
  Write the specified number of bytes to serial device.

  @param This                Pointer to EFI_SERIAL_IO_PROTOCOL
  @param  BufferSize         On input the size of Buffer, on output the amount of
                             data actually written
  @param  Buffer             The buffer of data to write

  @retval EFI_SUCCESS        The data were written successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The write operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
IohSerialWrite (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  );

/**
  Read the specified number of bytes from serial device.

  @param This               Pointer to EFI_SERIAL_IO_PROTOCOL
  @param BufferSize         On input the size of Buffer, on output the amount of
                            data returned in buffer
  @param Buffer             The buffer to return the data into

  @retval EFI_SUCCESS        The data were read successfully
  @retval EFI_DEVICE_ERROR   The device reported an error
  @retval EFI_TIMEOUT        The read operation was stopped due to timeout

**/
EFI_STATUS
EFIAPI
IohSerialRead (
  IN EFI_SERIAL_IO_PROTOCOL         *This,
  IN OUT UINTN                      *BufferSize,
  OUT VOID                          *Buffer
  );

//
// Internal Functions
//
/**
  Use scratchpad register to test if this serial port is present.

  @param SerialDevice   Pointer to serial device structure

  @return if this serial port is present
**/
BOOLEAN
IohSerialPortPresent (
  IN SERIAL_DEV                     *SerialDevice
  );

/**
  Detect whether specific FIFO is full or not.

  @param Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is full or not

**/
BOOLEAN
IohSerialFifoFull (
  IN SERIAL_DEV_FIFO                *Fifo
  );

/**
  Detect whether specific FIFO is empty or not.
 
  @param  Fifo    A pointer to the Data Structure SERIAL_DEV_FIFO

  @return whether specific FIFO is empty or not

**/
BOOLEAN
IohSerialFifoEmpty (
  IN SERIAL_DEV_FIFO                *Fifo
  );

/**
  Add data to specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data added to FIFO

  @retval EFI_SUCCESS           Add data to specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to add data because FIFO is already full

**/
EFI_STATUS
IohSerialFifoAdd (
  IN SERIAL_DEV_FIFO                *Fifo,
  IN UINT8                          Data
  );

/**
  Remove data from specific FIFO.

  @param Fifo                  A pointer to the Data Structure SERIAL_DEV_FIFO
  @param Data                  the data removed from FIFO

  @retval EFI_SUCCESS           Remove data from specific FIFO successfully
  @retval EFI_OUT_OF_RESOURCE   Failed to remove data because FIFO is empty

**/
EFI_STATUS
IohSerialFifoRemove (
  IN  SERIAL_DEV_FIFO               *Fifo,
  OUT UINT8                         *Data
  );

/**
  Reads and writes all avaliable data.

  @param SerialDevice           The device to flush

  @retval EFI_SUCCESS           Data was read/written successfully.
  @retval EFI_OUT_OF_RESOURCE   Failed because software receive FIFO is full.  Note, when
                                this happens, pending writes are not done.

**/
EFI_STATUS
IohSerialReceiveTransmit (
  IN SERIAL_DEV                     *SerialDevice
  );

/**
  Use IsaIo protocol to read serial port.

  @param IsaIo         Pointer to EFI_ISA_IO_PROTOCOL instance
  @param BaseAddress   Serial port register group base address
  @param Offset        Offset in register group

  @return Data read from serial port

**/
UINT8
IohSerialReadPort (
  IN SERIAL_DEV                             *SerialDevice,
  IN UINT32                                 Offset
  );

/**
  Use IsaIo protocol to write serial port.

  @param  IsaIo         Pointer to EFI_ISA_IO_PROTOCOL instance
  @param  BaseAddress   Serial port register group base address
  @param  Offset        Offset in register group
  @param  Data          data which is to be written to some serial port register

**/
VOID
IohSerialWritePort (
  IN SERIAL_DEV                             *SerialDevice,
  IN UINT32                                 Offset,
  IN UINT8                                  Data
  );


//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
IohSerialComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
IohSerialComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Add the component name for the serial io device

  @param SerialDevice     A pointer to the SERIAL_DEV instance.

  @param IsaIo            A pointer to the EFI_ISA_IO_PROTOCOL instance.

**/
VOID
AddName (
  IN  SERIAL_DEV                                   *SerialDevice
  );

#endif
