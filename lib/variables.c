/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * Portions of this file are a direct cut and paste from Tianocore
 * (http://tianocore.sf.net)
 *
 *  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigImpl.c
 *
 * Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
 * This program and the accompanying materials
 * are licensed and made available under the terms and conditions of the BSD License
 * which accompanies this distribution.  The full text of the license may be found 
 * at
 * http://opensource.org/licenses/bsd-license.php
 *
 * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 * 
 */
#include <efi.h>
#include <efilib.h>

#include <efiauthenticated.h>

#include <variables.h>
#include <guid.h>
#include <console.h>
#include <errors.h>

EFI_STATUS
variable_create_esl(void *cert, int cert_len, EFI_GUID *type, EFI_GUID *owner,
		    void **out, int *outlen)
{
	*outlen = cert_len + sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_GUID);

	*out = AllocateZeroPool(*outlen);
	if (!*out)
		return EFI_OUT_OF_RESOURCES;

	EFI_SIGNATURE_LIST *sl = *out;

	sl->SignatureHeaderSize = 0;
	sl->SignatureType = *type;
	sl->SignatureSize = cert_len + sizeof(EFI_GUID);
	sl->SignatureListSize = *outlen;

	EFI_SIGNATURE_DATA *sd = *out + sizeof(EFI_SIGNATURE_LIST);

	if (owner)
		sd->SignatureOwner = *owner;

	CopyMem(sd->SignatureData, cert, cert_len);

	return EFI_SUCCESS;
}


EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data
  )
{
  EFI_STATUS                       Status;
  UINT8                            *NewData;
  UINT8                            *Payload;
  UINTN                            PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2    *DescriptorData;
  UINTN                            DescriptorSize;
  EFI_TIME                         Time;
  EFI_GUID efi_cert_type = EFI_CERT_TYPE_PKCS7_GUID;
  
  if (Data == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the 
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;
  
  DescriptorSize    = OFFSET_OF(EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8*) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  ZeroMem (&Time, sizeof (EFI_TIME));
  Status = uefi_call_wrapper(RT->GetTime,2, &Time, NULL);
  if (EFI_ERROR (Status)) {
    FreePool(NewData);
    return Status;
  }
  Time.Pad1       = 0;
  Time.Nanosecond = 0;
  Time.TimeZone   = 0;
  Time.Daylight   = 0;
  Time.Pad2       = 0;
  CopyMem (&DescriptorData->TimeStamp, &Time, sizeof (EFI_TIME));
 
  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  DescriptorData->AuthInfo.CertType =  efi_cert_type;
 
  /* we're expecting an EFI signature list, so don't free the input since
   * it might not be in a pool */
#if 0
  if (Payload != NULL) {
    FreePool(Payload);
  }
#endif
  
  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

EFI_STATUS
SetSecureVariable(CHAR16 *var, UINT8 *Data, UINTN len, EFI_GUID owner,
		  UINT32 options, int createtimebased)
{
	EFI_SIGNATURE_LIST *Cert;
	UINTN DataSize;
	EFI_STATUS efi_status;

	/* Microsoft request: Bugs in some UEFI platforms mean that PK or any
	 * other secure variable can be updated or deleted programmatically,
	 * so prevent */
	if (!variable_is_setupmode())
		return EFI_SECURITY_VIOLATION;

	if (createtimebased) {
		int ds;
		efi_status = variable_create_esl(Data, len, &X509_GUID, NULL,
						 (void **)&Cert, &ds);
		if (efi_status != EFI_SUCCESS) {
			Print(L"Failed to create %s certificate %d\n", var, efi_status);
			return efi_status;
		}

		DataSize = ds;
	} else {
		/* we expect an efi signature list rather than creating it */
		Cert = (EFI_SIGNATURE_LIST *)Data;
		DataSize = len;
	}
	efi_status = CreateTimeBasedPayload(&DataSize, (UINT8 **)&Cert);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to create time based payload %d\n", efi_status);
		return efi_status;
	}

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, var, &owner,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_RUNTIME_ACCESS 
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS
				       | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
				       | options,
				       DataSize, Cert);

	return efi_status;
}

UINT64
GetOSIndications(void)
{
	UINT64 indications;
	UINTN DataSize = sizeof(indications);
	EFI_STATUS efi_status;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"OsIndicationsSupported", &GV_GUID, NULL, &DataSize, &indications);
	if (efi_status != EFI_SUCCESS)
		return 0;

	return indications;
}

EFI_STATUS
SETOSIndicationsAndReboot(UINT64 indications)
{
	UINTN DataSize = sizeof(indications);
	EFI_STATUS efi_status;

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"OsIndications",
				       &GV_GUID,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_RUNTIME_ACCESS 
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       DataSize, &indications);

	if (efi_status != EFI_SUCCESS)
		return efi_status;

	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm, EFI_SUCCESS, 0, NULL);
	/* does not return */

	return EFI_SUCCESS;
}

EFI_STATUS
get_variable_attr(CHAR16 *var, UINT8 **data, UINTN *len, EFI_GUID owner,
		  UINT32 *attributes)
{
	EFI_STATUS efi_status;

	*len = 0;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, var, &owner,
				       NULL, len, NULL);
	if (efi_status != EFI_BUFFER_TOO_SMALL)
		return efi_status;

	*data = AllocateZeroPool(*len);
	if (!data)
		return EFI_OUT_OF_RESOURCES;
	
	efi_status = uefi_call_wrapper(RT->GetVariable, 5, var, &owner,
				       attributes, len, *data);

	if (efi_status != EFI_SUCCESS) {
		FreePool(*data);
		*data = NULL;
	}
	return efi_status;
}

EFI_STATUS
get_variable(CHAR16 *var, UINT8 **data, UINTN *len, EFI_GUID owner)
{
	return get_variable_attr(var, data, len, owner, NULL);
}

EFI_STATUS
find_in_esl(UINT8 *Data, UINTN DataSize, UINT8 *key, UINTN keylen)
{
	EFI_SIGNATURE_LIST *CertList;

	certlist_for_each_certentry(CertList, Data, DataSize, DataSize) {
		if (CertList->SignatureSize != keylen + sizeof(EFI_GUID))
			continue;
		EFI_SIGNATURE_DATA *Cert;

		certentry_for_each_cert(Cert, CertList)
			if (CompareMem (Cert->SignatureData, key, keylen) == 0)
				return EFI_SUCCESS;
	}
	return EFI_NOT_FOUND;
}

EFI_STATUS
find_in_variable_esl(CHAR16* var, EFI_GUID owner, UINT8 *key, UINTN keylen)
{
	UINTN DataSize;
	UINT8 *Data;
	EFI_STATUS status;

	status = get_variable(var, &Data, &DataSize, owner);
	if (status != EFI_SUCCESS)
		return status;

	status = find_in_esl(Data, DataSize, key, keylen);

	FreePool(Data);

	return status;
}

int
variable_is_setupmode(void)
{
	/* set to 1 because we return true if SetupMode doesn't exist */
	UINT8 SetupMode = 1;
	UINTN DataSize = sizeof(SetupMode);
	EFI_STATUS status;

	status = uefi_call_wrapper(RT->GetVariable, 5, L"SetupMode", &GV_GUID, NULL,
				   &DataSize, &SetupMode);
	if (EFI_ERROR(status))
		return 1;

	return SetupMode;
}

int
variable_is_secureboot(void)
{
	/* return false if variable doesn't exist */
	UINT8 SecureBoot = 0;
	UINTN DataSize;
	EFI_STATUS status;

	DataSize = sizeof(SecureBoot);
	status = uefi_call_wrapper(RT->GetVariable, 5, L"SecureBoot", &GV_GUID, NULL,
				   &DataSize, &SecureBoot);
	if (EFI_ERROR(status))
		return 0;

	return SecureBoot;
}

EFI_STATUS
variable_enroll_hash(CHAR16 *var, EFI_GUID owner,
		     UINT8 hash[SHA256_DIGEST_SIZE])
{
	EFI_STATUS status;

	if (find_in_variable_esl(var, owner, hash, SHA256_DIGEST_SIZE)
	    == EFI_SUCCESS)
		/* hash already present */
		return EFI_ALREADY_STARTED;

	UINT8 sig[sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_SIGNATURE_DATA) - 1 + SHA256_DIGEST_SIZE];
	EFI_SIGNATURE_LIST *l = (void *)sig;
	EFI_SIGNATURE_DATA *d = (void *)sig + sizeof(EFI_SIGNATURE_LIST);
	SetMem(sig, 0, sizeof(sig));
	l->SignatureType = EFI_CERT_SHA256_GUID;
	l->SignatureListSize = sizeof(sig);
	l->SignatureSize = 16 +32; /* UEFI defined */
	CopyMem(&d->SignatureData, hash, SHA256_DIGEST_SIZE);
	d->SignatureOwner = MOK_OWNER;

	if (CompareGuid(&owner, &SIG_DB) == 0)
		status = SetSecureVariable(var, sig, sizeof(sig), owner,
					   EFI_VARIABLE_APPEND_WRITE, 0);
	else
		status = uefi_call_wrapper(RT->SetVariable, 5, var, &owner,
					   EFI_VARIABLE_NON_VOLATILE
					   | EFI_VARIABLE_BOOTSERVICE_ACCESS
					   | EFI_VARIABLE_APPEND_WRITE,
					   sizeof(sig), sig);
	return status;
}
