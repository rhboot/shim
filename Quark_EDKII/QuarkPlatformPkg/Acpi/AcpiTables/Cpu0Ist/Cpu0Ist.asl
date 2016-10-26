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

    Cpu0Ist.ASL

  Abstract:

    CPU EIST control methods

--*/

DefinitionBlock (
    "CPU0IST.aml",
    "SSDT",
    0x01,
    "SsgPmm",
    "Cpu0Ist",
    0x0012
    )
{
    External (PDC0, IntObj)
    External (CFGD, FieldUnitObj)
    External(\_PR.CPU0, DeviceObj)

    Scope(\_PR.CPU0)
    {
        Method(_PPC,0)
        {
            Return(ZERO)   // Return All States Available.
        }

        Method(_PCT,0)
        {
            //
            // If GV3 is supported and OSPM is capable of direct access to
            // performance state MSR, we use MSR method
            //
            //
            // PDCx[0] = Indicates whether OSPM is capable of direct access to
            // performance state MSR.
            //
            If(LAnd(And(CFGD,0x0001), And(PDC0,0x0001)))
            {
                Return(Package()    // MSR Method
                {
                    ResourceTemplate(){Register(FFixedHW, 0, 0, 0)},
                    ResourceTemplate(){Register(FFixedHW, 0, 0, 0)}
                })

            }

            //
            // Otherwise, we use smi method
            //
            Return(Package()    // SMI Method
                {
                  ResourceTemplate(){Register(SystemIO,16,0,0xB2)},
                  ResourceTemplate(){Register(SystemIO, 8,0,0xB3)}
                })
        }

        Method(_PSS,0)
        {
            //
            // If OSPM is capable of direct access to performance state MSR,
            // we report NPSS, otherwise, we report SPSS.
            If (And(PDC0,0x0001))
            {
                Return(NPSS)
            }

            Return(SPSS)
        }

        Name(SPSS,Package()
        {
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000}
        })

        Name(NPSS,Package()
        {
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000},
            Package(){0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000}
        })

        Method(_PSD,0)
        {
          //
          // If CMP is suppored, we report the dependency with two processors
          //
          If(And(CFGD,0x1000000))
          {
            //
            // If OSPM is capable of hardware coordination of P-states, we report
            // the dependency with hardware coordination.
            //
            // PDCx[11] = Indicates whether OSPM is capable of hardware coordination of P-states
            //
            If(And(PDC0,0x0800))
            {
              Return(Package(){
                Package(){
                  5,  // # entries.
                  0,  // Revision.
                  0,  // Domain #.
                  0xFE,  // Coord Type- HW_ALL.
                  2  // # processors.
                }
              })
            }

            //
            // Otherwise, the dependency with OSPM coordination
            //
            Return(Package(){
              Package(){
                5,    // # entries.
                0,    // Revision.
                0,    // Domain #.
                0xFC, // Coord Type- SW_ALL.
                2     // # processors.
              }
            })
          }

          //
          //  Otherwise, we report the dependency with one processor
          //
          Return(Package(){
            Package(){
              5,      // # entries.
              0,      // Revision.
              0,      // Domain #.
              0xFC,   // Coord Type- SW_ALL.
              1       // # processors.
            }
          })
        }
    }
}
