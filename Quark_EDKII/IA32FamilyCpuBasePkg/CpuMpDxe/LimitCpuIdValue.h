/** @file

  Include file for Max CPUID Limit Feature

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

  Module Name:  LimitCpuIdValue.h

**/

#ifndef _CPU_LIMIT_CPUID_VALUE_H_
#define _CPU_LIMIT_CPUID_VALUE_H_

#include "Cpu.h"
#include "MpService.h"

/**
  Detect capability of Max CPUID Limit feature for specified processor.
  
  This function detects capability of Max CPUID Limit feature for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.

**/
VOID
MaxCpuidLimitDetect (
  UINTN   ProcessorNumber
  );

/**
  Configures Processor Feature Lists for Max CPUID Limit feature for all processors.
  
  This function configures Processor Feature Lists for Max CPUID Limit feature for all processors.

**/
VOID
MaxCpuidLimitConfigFeatureList (
  VOID
  );

/**
  Produces entry of Max CPUID Limit feature in Register Table for specified processor.
  
  This function produces entry of Max CPUID Limit feature in Register Table for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.
  @param  Attribute         Pointer to the attribute

**/
VOID
MaxCpuidLimitReg (
  UINTN      ProcessorNumber,
  VOID       *Attribute
  );

#endif
