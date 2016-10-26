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

    CpuPm.ASL

  Abstract:

    CPU power management control methods

--*/

DefinitionBlock (
    "CPUPM.aml",
    "SSDT",
    0x01,
    "SsgPmm",
    "CpuPm",
    0x0010
    )
{
    External(\_PR.CPU0, DeviceObj)
    External(CFGD, FieldUnitObj)

    Scope(\)
    {
        // Config DWord, modified during POST
        // Bit definitions are the same as PPMFlags:
        //     CFGD[0] = PPM_GV3     = GV3
        //     CFGD[1] = PPM_TURBO   = Turbo Mode
        //     CFGD[2] = PPM_SUPER_LFM = N/2 Ratio
        //     CFGD[4] = PPM_C1      = C1 Capable, Enabled
        //     CFGD[5] = PPM_C2      = C2 Capable, Enabled
        //     CFGD[6] = PPM_C3      = C3 Capable, Enabled
        //     CFGD[7] = PPM_C4      = C4 Capable, Enabled
        //     CFGD[8] = PPM_C5      = C5/Deep C4 Capable, Enabled
        //     CFGD[9] = PPM_C6      = C6 Capable, Enabled
        //     CFGD[10] = PPM_C1E    = C1E Enabled
        //     CFGD[11] = PPM_C2E    = C2E Enabled
        //     CFGD[12] = PPM_C3E    = C3E Enabled
        //     CFGD[13] = PPM_C4E    = C4E Enabled
        //     CFGD[14] = PPM_HARD_C4E = Hard C4E Capable, Enabled
        //     CFGD[16] = PPM_TM1    = Thermal Monitor 1
        //     CFGD[17] = PPM_TM2    = Thermal Monitor 2
        //     CFGD[19] = PPM_PHOT   = Bi-directional ProcHot
        //     CFGD[21] = PPM_MWAIT_EXT = MWAIT extensions supported
        //     CFGD[24] = PPM_CMP    = CMP supported, Enabled
        //     CFGD[28] = PPM_TSTATE = CPU T states supported
        //
        // Name(CFGD, 0x80000000)
        // External Defined in GNVS

        Name(PDC0,0x80000000)   // CPU0 _PDC Flags.

        // We load it in AcpiPlatform
        //Name(SSDT,Package() 
        //{
        //    "CPU0IST ", 0x80000000, 0x80000000,
        //    "CPU1IST ", 0x80000000, 0x80000000,
        //    "CPU0CST ", 0x80000000, 0x80000000,
        //    "CPU1CST ", 0x80000000, 0x80000000,
        //})
    }
    Scope(\_PR.CPU0)
    {
        Method(_PDC, 1)
        {
          //
          // Store result of PDC.
          //
          CreateDWordField(Arg0,8,CAP0)   // Point to 3rd DWORD.
          Store(CAP0,PDC0)                // Store It in PDC0.
        }
    }

}
