/** @file

  Include file for Execute Disable Bit Feature

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

  Module Name:  Xd.h

**/

#ifndef _CPU_XD_H_
#define _CPU_XD_H_

#include "MpCommon.h"
#include "Cpu.h"

/**
  Detect capability of XD feature for specified processor.
  
  This function detects capability of XD feature for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.

**/
VOID
XdDetect (
  UINTN   ProcessorNumber
  );

/**
  Configures Processor Feature Lists for XD feature for all processors.
  
  This function configures Processor Feature Lists for XD feature for all processors.

**/
VOID
XdConfigFeatureList (
  VOID
  );

/**
  Produces entry of XD feature in Register Table for specified processor.
  
  This function produces entry of XD feature in Register Table for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.
  @param  Attribute         Pointer to the attribute

**/
VOID
XdReg (
  UINTN      ProcessorNumber,
  VOID       *Attribute
  );

#endif
