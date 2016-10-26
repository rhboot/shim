;******************************************************************************
;*
;*    Copyright (c) 2013 Intel Corporation.
;*
;*    Redistribution and use in source and binary forms, with or without
;*    modification, are permitted provided that the following conditions
;*    are met:
;*
;*    * Redistributions of source code must retain the above copyright
;*    notice, this list of conditions and the following disclaimer.
;*    * Redistributions in binary form must reproduce the above copyright
;*    notice, this list of conditions and the following disclaimer in
;*    the documentation and/or other materials provided with the
;*    distribution.
;*    * Neither the name of Intel Corporation nor the names of its
;*    contributors may be used to endorse or promote products derived
;*    from this software without specific prior written permission.
;*
;*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;*    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;*    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;*    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;*    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;*    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;*    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;*    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;*    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;*    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;*    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;*
;******************************************************************************

VacantFlag          Equ   00h
NotVacantFlag       Equ   0ffh
StartupApSignal     Equ   6E750000h
MonitorFilterSize   Equ   10h
ApInHltLoop         Equ   1
ApInMwaitLoop       Equ   2
ApInRunLoop         Equ   3

LockLocation        equ        (RendezvousFunnelProcEnd - RendezvousFunnelProcStart)
StackStart          equ        (LockLocation + 4h)
StackSize           equ        (LockLocation + 8h)
RendezvousProc      equ        (LockLocation + 0Ch)
GdtrProfile         equ        (LockLocation + 10h)
IdtrProfile         equ        (LockLocation + 16h)
BufferStart         equ        (LockLocation + 1Ch)
Cr3Location         equ        (LockLocation + 20h)
InitFlag            equ        (LockLocation + 24h)
ApCountLocation     equ        (LockLocation + 28h)
ApLoopModeLocation  equ        (LockLocation + 2Ch)
BistBuffer          equ        (LockLocation + 30h)

PAUSE32   MACRO
            DB      0F3h
            DB      090h
            ENDM

;-------------------------------------------------------------------------------
