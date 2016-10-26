/** @file
  PciHostBridge driver module, part of QNC module.

  Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation.

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
#include "CommonHeader.h"
#include "QNCInit.h"

EFI_STATUS
QncInitRootPorts (
  )
/*++
  
Routine Description:
  
  Perform Initialization of the Downstream Root Ports
    
Arguments:
  
Returns:
  
  EFI_SUCCESS             The function completed successfully
    
--*/
{
  EFI_STATUS                    Status;

  Status = PciExpressInit ();
  ASSERT_EFI_ERROR (Status);

  Status = SetInitRootPortDownstreamS3Item ();
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
SetInitRootPortDownstreamS3Item (
  )
/*++
  
Routine Description:
  
  Set an Init Root Port Downstream devices S3 dispatch item, this function may assert if any error happend 

Arguments:
  
Returns:
    
  EFI_SUCCESS             The function completed successfully

--*/
{
  EFI_STATUS                                    Status;
  VOID                                          *Context;
  VOID                                          *S3DispatchEntryPoint;
  STATIC EFI_QNC_S3_SUPPORT_PROTOCOL            *QncS3Support;
  STATIC UINT32                                 S3ParameterRootPortDownstream=0;
  STATIC EFI_QNC_S3_DISPATCH_ITEM               S3DispatchItem = {
    QncS3ItemTypeInitPcieRootPortDownstream,
    &S3ParameterRootPortDownstream
  };

  if (!QncS3Support) {
    //
    // Get the QNC S3 Support Protocol
    //
    Status = gBS->LocateProtocol (
                    &gEfiQncS3SupportProtocolGuid,
                    NULL,
                    &QncS3Support
                    );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = QncS3Support->SetDispatchItem (
                          QncS3Support,
                          &S3DispatchItem,
                          &S3DispatchEntryPoint,
                          &Context
                          );
  ASSERT_EFI_ERROR (Status);
  //
  // Save the script dispatch item in the Boot Script
  //  
  Status = S3BootScriptSaveDispatch2 (S3DispatchEntryPoint, Context);
  ASSERT_EFI_ERROR (Status);
  // 
  return Status;
}

