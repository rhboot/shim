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

  PchPlatformPolicy.h

Abstract:

  QNC policy protocol produced by a platform driver specifying various
  expected QNC settings. This protocol is consumed by the QNC drivers.

--*/
#ifndef _PCH_PLATFORM_POLICY_H_
#define _PCH_PLATFORM_POLICY_H_

//
// Forward reference for ANSI C compatibility
//
typedef struct _DXE_QNC_PLATFORM_POLICY_PROTOCOL  DXE_QNC_PLATFORM_POLICY_PROTOCOL;

//
// Protocol revision number
// Any backwards compatible changes to this protocol will result in an update in the revision number
// Major changes will require publication of a new protocol
//
// Revision 1: 	Original version

#define DXE_QNC_PLATFORM_POLICY_PROTOCOL_REVISION_1 1

//
// ---------------------------- PCI Express Config ----------------------
//
// The values before AutoConfig match the setting of PCI Express Base Specification 1.1, please be careful for adding new feature
//
typedef enum {
  QncPcieAspmDisabled,
  QncPcieAspmL0s,
  QncPcieAspmL1,
  QncPcieAspmL0sL1,
  QncPcieAspmAutoConfig,
  QncPcieAspmMax
} QNC_PCI_EXPRESS_ASPM_CONTROL;

extern EFI_GUID   gDxeQncPlatformPolicyProtocolGuid;
#endif
