/** @file
  Registers definition for Intel QuarkNcSocId.  

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

#ifndef __INTEL_QNC_REGS_H__
#define __INTEL_QNC_REGS_H__

#include <QNCAccess.h>

//
// PCI HostBridge Segment number
// 
#define QNC_PCI_HOST_BRIDGE_SEGMENT_NUMBER    0

//
// PCI RootBridge resource allocation's attribute
// 
#define QNC_PCI_ROOT_BRIDGE_RESOURCE_ALLOCATION_ATTRIBUTE \
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM

//
// PCI HostBridge resource appeture
// 
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSBASE     0x0
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSLIMIT    0xff
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_TSEG_SIZE   0x10000000

//
// PCI RootBridge configure port
// 
#define QNC_PCI_ROOT_BRIDGE_CONFIGURATION_ADDRESS_PORT  0xCF8
#define QNC_PCI_ROOT_BRIDGE_CONFIGURATION_DATA_PORT     0xCFC

//
// PCI Rootbridge's support feature
// 
#define QNC_PCI_ROOT_BRIDGE_SUPPORTED                   (EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO | \
                                                         EFI_PCI_ATTRIBUTE_ISA_IO         | \
                                                         EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO | \
                                                         EFI_PCI_ATTRIBUTE_VGA_MEMORY     | \
                                                         EFI_PCI_ATTRIBUTE_VGA_IO) 

#endif // __INTEL_QNC_REGS_H__
