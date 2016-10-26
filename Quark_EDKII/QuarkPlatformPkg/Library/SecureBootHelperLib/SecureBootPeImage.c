/** @file

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

  SecureBootPeImage.c

Abstract:

  Secure boot PE Image file services for Secure Boot Helper Protocol.

--*/

#include "CommonHeader.h"

/**
  Calculate hash of Pe/Coff image based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A

  if on return LoadInfo->ImageDigestSize == 0 then hash failed.

  @param[in out]   LoadInfo  On in basic file load info; on also hash info.
  @param[in]       HashAlg   Hash algorithm type.

  @retval TRUE               Successfully hash image.
  @retval FALSE              Fail in hash image.

**/
STATIC
BOOLEAN 
HashPeImage (
  IN OUT SECUREBOOT_PEIMAGE_LOAD_INFO     *LoadInfo,
  IN  UINT32                              HashAlg
  )
{
  BOOLEAN                           Ok;
  UINT16                            Magic;
  EFI_IMAGE_SECTION_HEADER          *Section;
  VOID                              *HashCtx;
  UINTN                             CtxSize;
  UINT8                             *HashBase;
  UINTN                             HashSize;
  UINTN                             SumOfBytesHashed;
  EFI_IMAGE_SECTION_HEADER          *SectionHeader;
  UINTN                             Index;
  UINTN                             Pos;
  UINTN                             ImageDigestSize;

  ASSERT (LoadInfo != NULL);
  if (LoadInfo == NULL) {
    return FALSE;
  }
  HashCtx = NULL;
  SectionHeader = NULL;
  Ok = FALSE;

  LoadInfo->ImageDigestSize = ImageDigestSize = 0;

  if ((HashAlg != HASHALG_SHA1) && (HashAlg != HASHALG_SHA256)) {
    return FALSE;
  }

  //
  // Initialize context of hash.
  //
  ZeroMem (LoadInfo->ImageDigest, sizeof(LoadInfo->ImageDigest));

  if (HashAlg == HASHALG_SHA1) {
    ImageDigestSize          = SHA1_DIGEST_SIZE;
    CopyGuid (&LoadInfo->ImageCertType, &gEfiCertSha1Guid);
  } else if (HashAlg == HASHALG_SHA256) {
    ImageDigestSize          = SHA256_DIGEST_SIZE;
    CopyGuid (&LoadInfo->ImageCertType, &gEfiCertSha256Guid);
  }
  if (ImageDigestSize > sizeof(LoadInfo->ImageDigest)) {
    return FALSE;
  }

  CtxSize = mHashTable[HashAlg].GetContextSize();

  HashCtx = AllocatePool (CtxSize);
  ASSERT (HashCtx != NULL);
  if (HashCtx == NULL) {
    return FALSE;
  }

  // 1.  Load the image header into memory.

  // 2.  Initialize a SHA hash context.
  Ok = mHashTable[HashAlg].HashInit(HashCtx);
  if (!Ok) {
    goto Done;
  }
  //
  // Measuring PE/COFF Image Header;
  // But CheckSum field and SECURITY data directory (certificate) are excluded
  // Get the magic value from the PE/COFF Optional Header
  //
  if (LoadInfo->NtHeader.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && LoadInfo->NtHeader.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value 
    //       in the PE/COFF Header. If the MachineType is Itanium(IA64) and the 
    //       Magic value in the OptionalHeader is EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
    //       then override the magic value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  } else {
    Magic = LoadInfo->NtHeader.Pe32->OptionalHeader.Magic;
  }

  //
  // 3.  Calculate the distance from the base of the image header to the image checksum address.
  // 4.  Hash the image header from its base to beginning of the image checksum.
  //
  HashBase = LoadInfo->ImageBase;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    HashSize = (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32->OptionalHeader.CheckSum) - HashBase);
  } else {
    //
    // Use PE32+ offset.
    //
    HashSize = (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32Plus->OptionalHeader.CheckSum) - HashBase);
  }

  Ok  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
  if (!Ok) {
    goto Done;
  }
  //
  // 5.  Skip over the image checksum (it occupies a single ULONG).
  // 6.  Get the address of the beginning of the Cert Directory.
  // 7.  Hash everything from the end of the checksum to the start of the Cert Directory.
  //
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    HashBase = (UINT8 *) &LoadInfo->NtHeader.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
    HashSize = (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
  } else {
    //
    // Use PE32+ offset.
    //    
    HashBase = (UINT8 *) &LoadInfo->NtHeader.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
    HashSize = (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
  }

  Ok  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
  if (!Ok) {
    goto Done;
  }
  //
  // 8.  Skip over the Cert Directory. (It is sizeof(IMAGE_DATA_DIRECTORY) bytes.)
  // 9.  Hash everything from the end of the Cert Directory to the end of image header.
  //
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset
    //
    HashBase = (UINT8 *) &LoadInfo->NtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
    HashSize = LoadInfo->NtHeader.Pe32->OptionalHeader.SizeOfHeaders - (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1]) - LoadInfo->ImageBase);
  } else {
    //
    // Use PE32+ offset.
    //
    HashBase = (UINT8 *) &LoadInfo->NtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
    HashSize = LoadInfo->NtHeader.Pe32Plus->OptionalHeader.SizeOfHeaders - (UINTN) ((UINT8 *) (&LoadInfo->NtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1]) - LoadInfo->ImageBase);
  }

  Ok  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
  if (!Ok) {
    goto Done;
  }
  //
  // 10. Set the SUM_OF_BYTES_HASHED to the size of the header.
  //
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    SumOfBytesHashed = LoadInfo->NtHeader.Pe32->OptionalHeader.SizeOfHeaders;
  } else {
    //
    // Use PE32+ offset
    //
    SumOfBytesHashed = LoadInfo->NtHeader.Pe32Plus->OptionalHeader.SizeOfHeaders;
  }

  //
  // 11. Build a temporary table of pointers to all the IMAGE_SECTION_HEADER
  //     structures in the image. The 'NumberOfSections' field of the image
  //     header indicates how big the table should be. Do not include any
  //     IMAGE_SECTION_HEADERs in the table whose 'SizeOfRawData' field is zero.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * LoadInfo->NtHeader.Pe32->FileHeader.NumberOfSections);
  ASSERT (SectionHeader != NULL);
  //
  // 12.  Using the 'PointerToRawData' in the referenced section headers as
  //      a key, arrange the elements in the table in ascending order. In other
  //      words, sort the section headers according to the disk-file offset of
  //      the section.
  //
  Section = (EFI_IMAGE_SECTION_HEADER *) (
               LoadInfo->ImageBase +
               LoadInfo->PeCoffHdrOff +
               sizeof (UINT32) +
               sizeof (EFI_IMAGE_FILE_HEADER) +
               LoadInfo->NtHeader.Pe32->FileHeader.SizeOfOptionalHeader
               );
  for (Index = 0; Index < LoadInfo->NtHeader.Pe32->FileHeader.NumberOfSections; Index++) {
    Pos = Index;
    while ((Pos > 0) && (Section->PointerToRawData < SectionHeader[Pos - 1].PointerToRawData)) {
      CopyMem (&SectionHeader[Pos], &SectionHeader[Pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
      Pos--;
    }
    CopyMem (&SectionHeader[Pos], Section, sizeof (EFI_IMAGE_SECTION_HEADER));
    Section += 1;
  }

  //
  // 13.  Walk through the sorted table, bring the corresponding section
  //      into memory, and hash the entire section (using the 'SizeOfRawData'
  //      field in the section header to determine the amount of data to hash).
  // 14.  Add the section's 'SizeOfRawData' to SUM_OF_BYTES_HASHED .
  // 15.  Repeat steps 13 and 14 for all the sections in the sorted table.
  //
  for (Index = 0; Index < LoadInfo->NtHeader.Pe32->FileHeader.NumberOfSections; Index++) {
    Section = &SectionHeader[Index];
    if (Section->SizeOfRawData == 0) {
      continue;
    }
    HashBase  = LoadInfo->ImageBase + Section->PointerToRawData;
    HashSize  = (UINTN) Section->SizeOfRawData;

    Ok  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
    if (!Ok) {
      goto Done;
    }

    SumOfBytesHashed += HashSize;
  }

  //
  // 16.  If the file size is greater than SUM_OF_BYTES_HASHED, there is extra
  //      data in the file that needs to be added to the hash. This data begins
  //      at file offset SUM_OF_BYTES_HASHED and its length is:
  //             FileSize  -  (CertDirectory->Size)
  //
  if (LoadInfo->ImageSize > SumOfBytesHashed) {
    HashBase = LoadInfo->ImageBase + SumOfBytesHashed;
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset.
      //
      HashSize = (UINTN)(
                 LoadInfo->ImageSize -
                 LoadInfo->NtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size -
                 SumOfBytesHashed);
    } else {
      //
      // Use PE32+ offset.
      //
      HashSize = (UINTN)(
                 LoadInfo->ImageSize -
                 LoadInfo->NtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size -
                 SumOfBytesHashed);
    }

    Ok  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
    if (!Ok) {
      goto Done;
    }
  }

  Ok = mHashTable[HashAlg].HashFinal(HashCtx, LoadInfo->ImageDigest);

Done:
  if (HashCtx != NULL) {
    FreePool (HashCtx);
  }
  if (SectionHeader != NULL) {
    FreePool (SectionHeader);
  }
  if (Ok) {
    LoadInfo->ImageDigestSize = ImageDigestSize;
    return TRUE;
  }
  return FALSE;
}

/**
  Recognize the Hash algorithm in PE/COFF Authenticode and caculate hash of
  Pe/Coff image based on the authenticated image hashing in PE/COFF Specification
  8.0 Appendix A.

  if on return LoadInfo->ImageDigestSize == 0 then hash failed.

  @param[in out]   LoadInfo   On in basic file load info; on also hash info.

  @retval EFI_UNSUPPORTED   Hash algorithm is not supported.
  @retval EFI_SUCCESS        Hash successfully.

**/
STATIC
EFI_STATUS
HashPeImageByType (
  IN OUT SECUREBOOT_PEIMAGE_LOAD_INFO     *LoadInfo
  )
{
  UINT8                             Index;
  WIN_CERTIFICATE_EFI_PKCS          *PkcsCertData;

  PkcsCertData = (WIN_CERTIFICATE_EFI_PKCS *) (LoadInfo->ImageBase + LoadInfo->SecDataDir->Offset);

  for (Index = 0; Index < HASHALG_MAX; Index++) {  
    //
    // Check the Hash algorithm in PE/COFF Authenticode.
    //    According to PKCS#7 Definition: 
    //        SignedData ::= SEQUENCE {
    //            version Version,
    //            digestAlgorithms DigestAlgorithmIdentifiers,
    //            contentInfo ContentInfo,
    //            .... }
    //    The DigestAlgorithmIdentifiers can be used to determine the hash algorithm in PE/COFF hashing
    //    This field has the fixed offset (+32) in final Authenticode ASN.1 data.
    //    Fixed offset (+32) is calculated based on two bytes of length encoding.
     //
    if ((*(PkcsCertData->CertData + 1) & TWO_BYTE_ENCODE) != TWO_BYTE_ENCODE) {
      //
      // Only support two bytes of Long Form of Length Encoding.
      //
      continue;
    }

    if (CompareMem (PkcsCertData->CertData + 32, mHashTable[Index].OidValue, mHashTable[Index].OidLength) == 0) {
      break;
    }
  }

  if (Index == HASHALG_MAX) {
    LoadInfo->ImageDigestSize = 0;
    return EFI_UNSUPPORTED;
  }

  //
  // HASH PE Image based on Hash algorithm in PE/COFF Authenticode.
  //
  if (!HashPeImage(LoadInfo, Index)) {
    LoadInfo->ImageDigestSize = 0;
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Load PE/COFF image information into buffer and check its validity.

  Function returns Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO see
  Protocol/SecureBootHelperDxe.h.

  Function also calculates HASH of image if CalcHash param == TRUE.
  If SecurityCert in image then hash algo. is the algo specified by the
  SecurityCert else a SHA256 digest is calculated.

  Caller must FreePool returned pointer when they are finished.

  @param[in]       FileDevicePath Device Path of PE image file.
  @param[in]       CalcHash       Flag to tell loader to calc. hash of image.

  @retval  NULL unable to load image.
  @return  Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO struct which contains is a
           memory block with image info and actual image data.

**/
SECUREBOOT_PEIMAGE_LOAD_INFO *
LoadPeImageWork (
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST BOOLEAN                        CalcHash
  )
{
  EFI_IMAGE_DOS_HEADER              *DosHdr;
  EFI_IMAGE_NT_HEADERS32            *NtHeader32;
  EFI_IMAGE_NT_HEADERS64            *NtHeader64;
  UINT32                            AStat;
  WIN_CERTIFICATE_UEFI_GUID         *GuidCertData;
  SECUREBOOT_PEIMAGE_LOAD_INFO      *LoadInfo;
  UINT8                             *FirstAlloc;
  UINTN                             FirstAllocSize;

  if (FileDevicePath == NULL) {
    return NULL;
  }

  FirstAlloc = MyGetFileBufferByFilePath (
                 FileDevicePath,
                 &FirstAllocSize,
                 &AStat);

  if (FirstAlloc == NULL) {
    return NULL;
  }
  LoadInfo = AllocatePool (sizeof (*LoadInfo) + FirstAllocSize + 16);
  if (LoadInfo == NULL) {
    FreePool (FirstAlloc);
    return NULL;
  }
  ZeroMem (LoadInfo, sizeof (*LoadInfo));
  LoadInfo->ImageBase = (UINT8 *) LoadInfo;

  LoadInfo->ImageBase = ALIGN_POINTER (
                          (LoadInfo->ImageBase + sizeof (*LoadInfo)),
                          sizeof (UINT64)
                          );
  LoadInfo->ImageSize = FirstAllocSize;
  CopyMem (LoadInfo->ImageBase, FirstAlloc, FirstAllocSize);
  FreePool (FirstAlloc);

  NtHeader32 = NULL;
  NtHeader64 = NULL;

  //
  // Read the Dos header
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER*) (LoadInfo->ImageBase);
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE)
  {
    //
    // DOS image header is present, 
    // So read the PE header after the DOS image header
    //
    LoadInfo->PeCoffHdrOff = DosHdr->e_lfanew;
  } else {
    LoadInfo->PeCoffHdrOff = 0;
  }

  //
  // Read PE header and check the signature validity and machine compatibility
  //
  NtHeader32 = (EFI_IMAGE_NT_HEADERS32*) (LoadInfo->ImageBase + LoadInfo->PeCoffHdrOff);
  if (NtHeader32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    FreePool (LoadInfo);
    return NULL;
  }

  LoadInfo->NtHeader.Pe32 = NtHeader32;

  //
  // Check the architecture field of PE header and get the Certificate Data Directory data
  // Note the size of FileHeader field is constant for both IA32 and X64 arch
  //
  if ((NtHeader32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32)
      || (NtHeader32->FileHeader.Machine == EFI_IMAGE_MACHINE_EBC)) {
    //
    // IA-32 Architecture
    //
    LoadInfo->ImageType = ImageType_IA32;
    LoadInfo->SecDataDir = (EFI_IMAGE_SECURITY_DATA_DIRECTORY*) &(NtHeader32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]);
  } else if ((NtHeader32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64)
          || (NtHeader32->FileHeader.Machine == EFI_IMAGE_MACHINE_X64)) {
    //
    // 64-bits Architecture
    //
    LoadInfo->ImageType = ImageType_X64;
    NtHeader64 = (EFI_IMAGE_NT_HEADERS64 *) (LoadInfo->ImageBase + LoadInfo->PeCoffHdrOff);
    LoadInfo->SecDataDir = (EFI_IMAGE_SECURITY_DATA_DIRECTORY*) &(NtHeader64->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]);
  } else {
    FreePool (LoadInfo);
    return NULL;
  }

  if (LoadInfo->SecDataDir->SizeOfCert != 0) {
    //
    // Get the certificate data pointer.
    //
    LoadInfo->Certificate = (WIN_CERTIFICATE *) (LoadInfo->ImageBase + LoadInfo->SecDataDir->Offset);
  }

  if (CalcHash) {
    if (LoadInfo->Certificate == NULL) {

      HashPeImage (LoadInfo, HASHALG_SHA256);
    } else {
      if (LoadInfo->Certificate->wCertificateType == WIN_CERT_TYPE_EFI_GUID) {
        GuidCertData = (WIN_CERTIFICATE_UEFI_GUID *) LoadInfo->Certificate;
        if (CompareMem (&GuidCertData->CertType, &gEfiCertTypeRsa2048Sha256Guid, sizeof(EFI_GUID)) == 0) {
          HashPeImage (LoadInfo, HASHALG_SHA256);
        }
      } else if (LoadInfo->Certificate->wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
        HashPeImageByType (LoadInfo);
      } else {
        LoadInfo->ImageDigestSize = 0;
      }
    }
  }

  return LoadInfo;
}

/** Load PE/COFF image information into buffer and check its validity.

  Function returns Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO see
  Protocol/SecureBootHelperDxe.h.

  Function also calculates HASH of image if CalcHash param == TRUE.
  If SecurityCert in image then hash algo. is the algo specified by the
  Cert else a SHA256 digest is calculated.

  Caller must FreePool returned pointer when they are finished.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       FileDevicePath Device Path of PE image file.
  @param[in]       CalcHash       Flag to tell loader to calc. hash of image.

  @retval  NULL unable to load image.
  @return  Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO struct which contains is a
           memory block with image info and actual image data.

**/
SECUREBOOT_PEIMAGE_LOAD_INFO *
EFIAPI
LoadPeImage (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST BOOLEAN                        CalcHash
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return NULL;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return NULL;
  }

  return LoadPeImageWork (FileDevicePath, CalcHash);
}

/** Get the file image from mask.

  If This or File is NULL, then ASSERT().
  If sanity checks on This pointer fail then assert.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       File  This is a pointer to the device path of the file
                         to get IMAGE_FROM_XXX mask for.

  @return UINT32 Mask which is one of IMAGE_FROM_XXX defs in protocol header file.

**/
UINT32
EFIAPI
GetImageFromMask (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *File
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;

  //
  // Verify This pointer.
  //
  ASSERT (This != NULL);
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  ASSERT (PrivateData->Signature == SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE);
  ASSERT (File != NULL);

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle      = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  //
  // Next check to see if File is from a Block I/O device
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiBlockIoProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    BlockIo = NULL;
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status) && BlockIo != NULL) {
      if (BlockIo->Media != NULL) {
        if (BlockIo->Media->RemovableMedia) {
          //
          // Block I/O is present and specifies the media is removable
          //
          return IMAGE_FROM_REMOVABLE_MEDIA;
        } else {
          //
          // Block I/O is present and specifies the media is not removable
          //
          return IMAGE_FROM_FIXED_MEDIA;
        }
      }
    }
  }

  //
  // File is not in a Firmware Volume or on a Block I/O device, so check to see if
  // the device path supports the Simple File System Protocol.
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Simple File System is present without Block I/O, so assume media is fixed.
    //
    return IMAGE_FROM_FIXED_MEDIA;
  }

  //
  // File is not from an FV, Block I/O or Simple File System, so the only options
  // left are a PCI Option ROM and a Load File Protocol such as a PXE Boot from a NIC.
  //
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {

    case MEDIA_DEVICE_PATH:
      if (DevicePathSubType (TempDevicePath) == MEDIA_RELATIVE_OFFSET_RANGE_DP) {
        return IMAGE_FROM_OPTION_ROM;
      }
      break;

    case MESSAGING_DEVICE_PATH:
      if (DevicePathSubType(TempDevicePath) == MSG_MAC_ADDR_DP) {
        return IMAGE_FROM_PXE;
      }
      break;

    default:
      break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  return IMAGE_UNKNOWN;
}
