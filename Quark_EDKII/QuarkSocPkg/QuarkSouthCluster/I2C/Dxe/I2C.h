/** @file

  Provides definition of entry point to the DXE Driver that produces the
  I2C Controller Protocol and definition of protocol functions.

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

#ifndef _I2C_H_
#define _I2C_H_

#include <IohAccess.h>
#include <IohCommonDefinitions.h>
#include "CommonHeader.h"
#include "I2CRegs.h"

//
// Constants that define I2C Controller timeout and max. polling time.
//
#define MAX_T_POLL_COUNT         100
#define TI2C_POLL                25  // microseconds
#define MAX_STOP_DET_POLL_COUNT ((1000 * 1000) / TI2C_POLL)  // Extreme for expected Stop detect.

/**
  The GetI2CIoPortBaseAddress() function gets IO port base address of I2C Controller.

  Always reads PCI configuration space to get MMIO base address of I2C Controller.

  @return The IO port base address of I2C controller.

**/
UINTN
GetI2CIoPortBaseAddress (
  VOID
  );

/**
  The EnableI2CMmioSpace() function enables access to I2C MMIO space.

**/
VOID
EnableI2CMmioSpace (
  VOID
  );

/**
  The DisableI2CController() functions disables I2C Controller.

**/
VOID
DisableI2CController (
  VOID
  );

/**
  The EnableI2CController() function enables the I2C Controller.

**/
VOID
EnableI2CController (
  VOID
  );

/**
  The WaitForStopDet() function waits until I2C STOP Condition occurs,
  indicating transfer completion.

  @retval EFI_SUCCESS           Stop detected.
  @retval EFI_TIMEOUT           Timeout while waiting for stop condition.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx or Rx overflow detected.

**/
EFI_STATUS
WaitForStopDet (
  VOID
  );

/**

  The InitializeInternal() function initialises internal I2C Controller
  register values that are commonly required for I2C Write and Read transfers.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @retval EFI_SUCCESS           I2C Operation completed successfully.

**/
EFI_STATUS  
InitializeInternal (
  IN EFI_I2C_ADDR_MODE  AddrMode
  );

/**

  The WriteByte() function provides a standard way to execute a
  standard single byte write to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  Value           The 8-bit value to write.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
WriteByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          Value
  );

/**

  The ReadByte() function provides a standard way to execute a
  standard single byte read to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param  I2CAddress      I2C Slave device address
  @param  ReturnDataPtr   Pointer to location to receive read byte.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS
EFIAPI
ReadByte (
  IN  UINTN          I2CAddress,
  OUT UINT8          *ReturnDataPtr
  );

/**

  The WriteMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when writing block of data), as defined in the I2C Specification.

  @param I2CAddress   The I2C slave address of the device
                      with which to communicate.

  @param Buffer       Contains the value of byte to be written to the
                      I2C slave device.

  @param Length       No. of bytes to be written.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Tx overflow detected.

**/
EFI_STATUS
EFIAPI
WriteMultipleByte (
  IN  UINTN          I2CAddress,
  IN  UINT8          *WriteBuffer,
  IN  UINTN          Length
  );

/**

  The ReadMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  when reading block of data), as defined in the I2C Specification (I2C combined
  write/read protocol).

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param Buffer       Contains the value of byte data written or read from the
                      I2C slave device.

  @param WriteLength  No. of bytes to be written. In this case data
                      written typically contains sub-address or sub-addresses
                      in Hi-Lo format, that need to be read (I2C combined
                      write/read protocol).

  @param ReadLength   No. of bytes to be read.

  @param ReadLength   No. of bytes to be read from I2C slave device.
                      need to be read.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_UNSUPPORTED       Unsupported input param.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Tx abort signaled in HW status register.
  @retval EFI_DEVICE_ERROR      Rx underflow or Rx/Tx overflow detected.

**/
EFI_STATUS
EFIAPI
ReadMultipleByte (
  IN  UINTN          I2CAddress,
  IN  OUT UINT8      *Buffer,
  IN  UINTN          WriteLength,
  IN  UINTN          ReadLength
  );

/**

  The I2CWriteByte() function is a wrapper function for the WriteByte() function.
  Provides a standard way to execute a standard single byte write to an IC2 device 
  (without accessing sub-addresses), as defined in the I2C Specification.

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Length       No. of bytes to be written.

  @param Buffer       Contains the value of byte data to execute to the
                      I2C slave device.


  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER  This or Buffer pointers are invalid.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS  
I2CWriteByte (
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT VOID                       *Buffer
  );

/**

  The I2CReadByte() function is a wrapper function for the ReadByte() function.
  Provides a standard way to execute a standard single byte read to an IC2 device
  (without accessing sub-addresses), as defined in the I2C Specification.

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Length       No. of bytes to be read.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.


  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER This or Buffer pointers are invalid.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
EFI_STATUS  
I2CReadByte (
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT VOID                       *Buffer
  );

/**

  The I2CWriteMultipleByte() function is a wrapper function for the WriteMultipleByte() function.
  Provides a standard way to execute multiple byte writes to an IC2 device (e.g. when accessing 
  sub-addresses or writing block of data), as defined in the I2C Specification.

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Length       No. of bytes to be written.

  @param Buffer       Contains the value of byte to be written to the
                      I2C slave device.

  @retval EFI_SUCCESS            Transfer success.
  @retval EFI_INVALID_PARAMETER  This, Length or Buffer pointers are invalid.
  @retval EFI_UNSUPPORTED        Unsupported input param.
  @retval EFI_TIMEOUT            Timeout while waiting xfer.
  @retval EFI_ABORTED            Controller aborted xfer.
  @retval EFI_DEVICE_ERROR       Device error detected by controller.

**/
EFI_STATUS  
I2CWriteMultipleByte (
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN UINTN                          *Length,
  IN OUT VOID                       *Buffer
  );

/**

  The I2CReadMultipleByte() function is a wrapper function for the ReadMultipleByte function.
  Provides a standard way to execute multiple byte writes to an IC2 device
  (e.g. when accessing sub-addresses or when reading block of data), as defined
  in the I2C Specification (I2C combined write/read protocol).

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param WriteLength  No. of bytes to be written. In this case data
                      written typically contains sub-address or sub-addresses
                      in Hi-Lo format, that need to be read (I2C combined
                      write/read protocol).

  @param ReadLength   No. of bytes to be read from I2C slave device.
                      need to be read.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.

  @retval EFI_SUCCESS            Transfer success.
  @retval EFI_INVALID_PARAMETER  This, WriteLength, ReadLength or Buffer
                                 pointers are invalid.
  @retval EFI_UNSUPPORTED        Unsupported input param.
  @retval EFI_TIMEOUT            Timeout while waiting xfer.
  @retval EFI_ABORTED            Controller aborted xfer.
  @retval EFI_DEVICE_ERROR       Device error detected by controller.

**/
EFI_STATUS  
I2CReadMultipleByte (
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN UINTN                          *WriteLength,
  IN UINTN                          *ReadLength,
  IN OUT VOID                       *Buffer
  );

/**
  Entry point to the DXE Driver that produces the I2C Controller Protocol.

  @param  ImageHandle      ImageHandle of the loaded driver.
  @param  SystemTable      Pointer to the EFI System Table.

  @retval EFI_SUCCESS      The entry point of I2C DXE driver is executed successfully.
  @retval !EFI_SUCESS      Some error occurs in the entry point of I2C DXE driver.

**/
EFI_STATUS
EFIAPI
InitializeI2C (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
