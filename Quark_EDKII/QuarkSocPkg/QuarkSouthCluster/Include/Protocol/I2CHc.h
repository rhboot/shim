/** @file
  The file provides defintion of I2C host controller management
  functions and data transactions over the I2C Bus.

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

#ifndef __I2C_HC_H__
#define __I2C_HC_H__

#define EFI_I2C_HC_PROTOCOL_GUID \
  {0x855b7d58, 0x874b, 0x47eb, { 0xa5, 0xcf, 0x98, 0xed, 0xac, 0x80, 0x67, 0x96} }

typedef struct _EFI_I2C_HC_PROTOCOL EFI_I2C_HC_PROTOCOL;

///
/// I2C Device Address
///
typedef struct {
  ///
  /// The I2C hardware address to which the I2C device is preassigned or allocated.
  ///
  UINTN I2CDeviceAddress : 10;
} EFI_I2C_DEVICE_ADDRESS;

///
/// I2C Addressing Mode (7-bit or 10 bit)
///
typedef enum _EFI_I2C_ADDR_MODE {
  EfiI2CSevenBitAddrMode,
  EfiI2CTenBitAddrMode,
} EFI_I2C_ADDR_MODE;

/**

  The WriteByte() function provides a standard way to execute a
  standard single byte write to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Buffer       Contains the value of byte data to execute to the
                      I2C slave device.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER  This or Buffer pointers are invalid.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_WRITEBYTE_OPERATION)(
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT    VOID                    *Buffer
);

/**

  The ReadByte() function provides a standard way to execute a
  standard single byte read to an IC2 device (without accessing
  sub-addresses), as defined in the I2C Specification.

  @param This         A pointer to the EFI_I2C_PROTOCOL instance.

  @param SlaveAddress The I2C slave address of the device
                      with which to communicate.

  @param AddrMode     I2C Addressing Mode: 7-bit or 10-bit address.

  @param Buffer       Contains the value of byte data read from the
                      I2C slave device.

  @retval EFI_SUCCESS           Transfer success.
  @retval EFI_INVALID_PARAMETER This or Buffer pointers are invalid.
  @retval EFI_TIMEOUT           Timeout while waiting xfer.
  @retval EFI_ABORTED           Controller aborted xfer.
  @retval EFI_DEVICE_ERROR      Device error detected by controller.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_READBYTE_OPERATION)(
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN OUT    VOID                    *Buffer
);

/**

  The WriteMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses or
  writing block of data), as defined in the I2C Specification.

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
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_WRITE_MULTIPLE_BYTE_OPERATION)(
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN        UINTN                   *Length,
  IN OUT    VOID                    *Buffer
);

/**

  The ReadMultipleByte() function provides a standard way to execute
  multiple byte writes to an IC2 device (e.g. when accessing sub-addresses
  or when reading block of data), as defined in the I2C Specification
  (I2C combined write/read protocol).

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
typedef
EFI_STATUS
(EFIAPI *EFI_I2C_READ_MULTIPLE_BYTE_OPERATION)(
  IN CONST  EFI_I2C_HC_PROTOCOL     *This,
  IN        EFI_I2C_DEVICE_ADDRESS  SlaveAddress,
  IN        EFI_I2C_ADDR_MODE       AddrMode,
  IN        UINTN                   *WriteLength,
  IN        UINTN                   *ReadLength,
  IN OUT    VOID                    *Buffer
);

//
// The EFI_I2C_HC_PROTOCOL provides I2C host controller management and basic data
// transactions over I2C. There is one EFI_I2C_HC_PROTOCOL instance for each I2C
// host controller.
//
struct _EFI_I2C_HC_PROTOCOL {
  EFI_I2C_WRITEBYTE_OPERATION             WriteByte;
  EFI_I2C_READBYTE_OPERATION              ReadByte;
  EFI_I2C_WRITE_MULTIPLE_BYTE_OPERATION   WriteMultipleByte;
  EFI_I2C_READ_MULTIPLE_BYTE_OPERATION    ReadMultipleByte;
};

extern EFI_GUID gEfiI2CHcProtocolGuid;

#endif

