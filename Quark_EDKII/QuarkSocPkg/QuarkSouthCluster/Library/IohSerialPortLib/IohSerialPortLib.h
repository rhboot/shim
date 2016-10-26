/** @file
  Header file of Serial Port hardware definition.

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

  Module Name:  IohSerialPortLib.h

**/

#ifndef _IOH_SERIAL_PORT_LIB_H_
#define _IOH_SERIAL_PORT_LIB_H_
//
// ---------------------------------------------
// UART Register Offset and Bitmask of Value
// ---------------------------------------------
//
#define R_UART_BAUD_THR       0
#define R_UART_BAUD_LOW       0
#define R_UART_BAUD_HIGH      0x04
#define R_UART_IER            0x04
#define R_UART_FCR            0x08
#define   B_UARY_FCR_TRFIFIE  BIT0
#define   B_UARY_FCR_RESETRF  BIT1
#define   B_UARY_FCR_RESETTF  BIT2
#define R_UART_LCR            0x0C
#define   B_UARY_LCR_DLAB     BIT7
#define R_UART_MCR            0x10
#define R_UART_LSR            0x14
#define   B_UART_LSR_RXRDY    BIT0
#define   B_UART_LSR_TXRDY    BIT5
#define   B_UART_LSR_TEMT     BIT6
#define R_UART_MSR            0x18
#define R_UART_SCR            0x1C

#define UART_SIGNATURE         0x55

#endif
