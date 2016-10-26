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

 Hte.h

 Abstract:

 HTE handling routines for MRC use.

 --*/
#ifndef __HTE_H
#define __HTE_H

#define STATIC   static
#define VOID     void

#if !defined(__GNUC__) && (__STDC_VERSION__ < 199901L)
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;
#endif

typedef enum
{
  MrcNoHaltSystemOnError,
  MrcHaltSystemOnError,
  MrcHaltHteEngineOnError,
  MrcNoHaltHteEngineOnError
} HALT_TYPE;

typedef enum
{
  MrcMemInit, MrcMemTest
} MEM_INIT_OR_TEST;

#define READ_TRAIN      1
#define WRITE_TRAIN     2

#define HTE_MEMTEST_NUM                 2
#define HTE_LOOP_CNT                    5  // EXP_LOOP_CNT field of HTE_CMD_CTL. This CANNOT be less than 4
#define HTE_LFSR_VICTIM_SEED   0xF294BA21  // Random seed for victim.
#define HTE_LFSR_AGRESSOR_SEED 0xEBA7492D  // Random seed for aggressor.
UINT32
HteMemInit(
    MRC_PARAMS *CurrentMrcData,
    UINT8 MemInitFlag,
    UINT8 HaltHteEngineOnError);

UINT16
BasicWriteReadHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 Mode);

UINT16
WriteStressBitLanesHTE(
    MRC_PARAMS *CurrentMrcData,
    UINT32 Address,
    UINT8 FirstRun);

VOID
HteMemOp(
    UINT32 Address,
    UINT8 FirstRun,
    UINT8 IsWrite);

#endif
