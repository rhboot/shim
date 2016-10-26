/*++

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


Module Name:  

  CommonIncludes.h

Abstract:

  This file defines common equates.

--*/

#ifndef _COMMON_INCLUDES_H_
#define _COMMON_INCLUDES_H_

//#include "BackCompatible.h"

#define V_INTEL_VID               0x8086

//
// Min Max
//
#define V_MIN(a, b)       (((a) < (b)) ? (a) : (b))
#define V_MAX(a, b)       (((a) > (b)) ? (a) : (b))

//
// Bit map macro
//
#ifndef BIT0
#define BIT31   0x80000000
#define BIT30   0x40000000
#define BIT29   0x20000000
#define BIT28   0x10000000
#define BIT27   0x08000000
#define BIT26   0x04000000
#define BIT25   0x02000000
#define BIT24   0x01000000
#define BIT23   0x00800000
#define BIT22   0x00400000
#define BIT21   0x00200000
#define BIT20   0x00100000
#define BIT19   0x00080000
#define BIT18   0x00040000
#define BIT17   0x00020000
#define BIT16   0x00010000
#define BIT15   0x00008000
#define BIT14   0x00004000
#define BIT13   0x00002000
#define BIT12   0x00001000
#define BIT11   0x00000800
#define BIT10   0x00000400
#define BIT9    0x00000200
#define BIT8    0x00000100
#define BIT7    0x00000080
#define BIT6    0x00000040
#define BIT5    0x00000020
#define BIT4    0x00000010
#define BIT3    0x00000008
#define BIT2    0x00000004
#define BIT1    0x00000002
#define BIT0    0x00000001

#define BIT63   0x8000000000000000
#define BIT62   0x4000000000000000
#define BIT61   0x2000000000000000
#define BIT60   0x1000000000000000
#define BIT59   0x0800000000000000
#define BIT58   0x0400000000000000
#define BIT57   0x0200000000000000
#define BIT56   0x0100000000000000
#define BIT55   0x0080000000000000
#define BIT54   0x0040000000000000
#define BIT53   0x0020000000000000
#define BIT52   0x0010000000000000
#define BIT51   0x0008000000000000
#define BIT50   0x0004000000000000
#define BIT49   0x0002000000000000
#define BIT48   0x0001000000000000
#define BIT47   0x0000800000000000
#define BIT46   0x0000400000000000
#define BIT45   0x0000200000000000
#define BIT44   0x0000100000000000
#define BIT43   0x0000080000000000
#define BIT42   0x0000040000000000
#define BIT41   0x0000020000000000
#define BIT40   0x0000010000000000
#define BIT39   0x0000008000000000
#define BIT38   0x0000004000000000
#define BIT37   0x0000002000000000
#define BIT36   0x0000001000000000
#define BIT35   0x0000000800000000
#define BIT34   0x0000000400000000
#define BIT33   0x0000000200000000
#define BIT32   0x0000000100000000
#endif

#define BITS(x)  (1<<(x))

/*
Notes : 
  1.  Bit position always starts at 0.
  2.  Following macros are applicable only for Word alligned integers. 
*/
#define BIT(Pos, Value)              ( 1<<(Pos) & (Value))
#define BITRANGE(From,Width,Value)   (((Value) >> (From)) & (( 1<<(Width)) -1))

//
// Align length up to the next step.
//
#define ALIGN_LENGTH(len, step) (((UINTN)(len) + (((step) - ((UINTN) (len))) & ((step) - 1))))

#endif

