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
  SecureBootHelper.h

Abstract:
  Protocol header file for Secure Boot Helper Protocol.

--*/

#ifndef _SECUREBOOT_HELPER_PROTOCOL_H_
#define _SECUREBOOT_HELPER_PROTOCOL_H_

#include <IndustryStandard/PeImage.h>
#include <Guid/ImageAuthentication.h>
#include <Protocol/DevicePath.h>

//
// Constant definitions.
//

#define TWO_BYTE_ENCODE                   0x82
#define WIN_CERT_UEFI_RSA2048_SIZE        256

///
/// Image type definitions (BIT0 - BIT4 Matchs whats shown in SecurityPkg.dec).
/// Added BIT31 (IMAGE_FROM_PXE) since GET_IMAGE_FROM_MASK service can
/// determine this from location. Lower bits left for SecurityPkg additions.
///
#define IMAGE_UNKNOWN                         0x00000001
#define IMAGE_FROM_FV                         0x00000002
#define IMAGE_FROM_OPTION_ROM                 0x00000004
#define IMAGE_FROM_REMOVABLE_MEDIA            0x00000008
#define IMAGE_FROM_FIXED_MEDIA                0x00000010
#define IMAGE_FROM_PXE                        0x80000000

///
/// Hash types.
///
#define HASHALG_SHA1                          0x00000000
#define HASHALG_SHA224                        0x00000001
#define HASHALG_SHA256                        0x00000002
#define HASHALG_SHA384                        0x00000003
#define HASHALG_SHA512                        0x00000004
#define HASHALG_MAX                           0x00000005

///
/// Set max digest size, set 96 to future proof.
///
#define MAX_DIGEST_SIZE                   96

//
// Type definitions.
//

///
/// We define another format of 5th directory entry: security directory.
///
typedef struct {
  UINT32                            Offset;       ///< Offset of certificate.
  UINT32                            SizeOfCert;   ///< size of certificate appended.
} EFI_IMAGE_SECURITY_DATA_DIRECTORY;

///
/// Image type determined when Secure Boot Helper loads a PE Image.
///
typedef enum {
  ImageType_IA32,
  ImageType_X64
} SECUREBOOT_PEIMAGE_IMAGE_TYPE;

///
/// Info on determined when Secure Boot Helper loads a PE Image.
///
typedef struct {
  UINT8                             *ImageBase;
  UINTN                             ImageSize;
  SECUREBOOT_PEIMAGE_IMAGE_TYPE     ImageType;
  UINT32                            PeCoffHdrOff;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION NtHeader;
  EFI_IMAGE_SECURITY_DATA_DIRECTORY *SecDataDir;
  WIN_CERTIFICATE                   *Certificate;
  EFI_GUID                          ImageCertType;
  UINTN                             ImageDigestSize;
  UINT8                             ImageDigest [MAX_DIGEST_SIZE];
} SECUREBOOT_PEIMAGE_LOAD_INFO;

///
/// SecureBoot statistics for Secure Boot variables.
///
typedef struct {
  BOOLEAN      PkEnrolled;              ///< Is 'pk' variable enrolled.
  UINT32       PkAttributes;            ///< 'pk' variable attributes.
  BOOLEAN      KekEnrolled;             ///< Is 'kek' variable enrolled.
  UINTN        KekAttributes;           ///< 'kek' variable attributes.
  UINTN        KekTotalCertCount;       ///< Total number of certs in 'kek' database variable.
  UINTN        KekX509CertCount;        ///< Number of X509 certs in 'kek' database variable.
  UINTN        KekRsa2048CertCount;     ///< Number of Rsa2048 storing file (*.pbk) certs in 'kek' database variable.
  UINTN        KekOtherCertCount;       ///< Unspecified cert types stored in kek.
  BOOLEAN      DbEnrolled;              ///< Is 'Db' variable enrolled.
  UINTN        DbAttributes;            ///< 'Db' variable attributes.
  UINTN        DbTotalCertCount;        ///< Total number of certs in 'Db' database variable.
  UINTN        DbX509CertCount;         ///< Number of X509 certs in 'Db' database variable.
  UINTN        DbSha256Count;           ///< Number of Sha256 digests in 'Db' database variable.
  UINTN        DbSha1Count;             ///< Number of Sha1 digests in 'Db' database variable.
  UINTN        DbOtherCertCount;        ///< Unspecified cert types stored in Db.
  BOOLEAN      DbxEnrolled;             ///< Is 'Dbx' variable enrolled.
  UINTN        DbxAttributes;           ///< 'Dbx' variable attributes.
  UINTN        DbxTotalCertCount;       ///< Total number of certs in 'Dbx' database variable.
  UINTN        DbxX509CertCount;        ///< Number of X509 certs in 'Dbx' database variable.
  UINTN        DbxSha256Count;          ///< Number of Sha256 digests in 'Dbx' database variable.
  UINTN        DbxSha1Count;            ///< Number of Sha1 digests in 'Dbx' database variable.
  UINTN        DbxOtherCertCount;       ///< Unspecified cert types stored in Dbx.
  UINTN        MaxVarSize;              ///< Sizeof biggest SecureBoot variable pk,kek,db or dbx.
} SECUREBOOT_VARIABLE_STATS;

///
/// Secure Boot Store Type.
///
typedef enum {
  PKStore = 0,                      ///< Private Key Store.
  KEKStore = 1,                     ///< Key Exchange Key Database.
  DBStore = 2,                      ///< Authorized Signature Database.
  DBXStore = 3                      ///< Forbidden Signature Database.
} SECUREBOOT_STORE_TYPE;

///
/// Filter for finding certificate lists in secure boot databases.
///
typedef struct {
  EFI_GUID                  *SignatureType;            ///< Identifier of certificate list type.
  EFI_GUID                  *SignatureOwner;           ///< Identifier of Specific cert in list.
  UINTN                     SignatureDataPayloadSize;  ///< Payload size of cert list.
} FIND_CERT_FILTER;

///
/// Optional stats returned from FIND_FIRST_CERT service.
///
typedef struct {
  UINTN                     VariableSize;              ///< Byte size of cert variable.
  UINT32                    VariableAttributes;        ///< Attributes of cert variable.
} FIND_CERT_STATS;

///
/// Forward reference for pure ANSI compatability
///
typedef struct _SECUREBOOT_HELPER_PROTOCOL  SECUREBOOT_HELPER_PROTOCOL;

///
/// Extern the GUID for protocol users.
///
extern EFI_GUID gSecureBootHelperProtocolGuid;

//
// Service typedefs to follow.
//

/** Check if platform is operating by a physically present user.

  This function provides a platform-specific method to detect whether the platform
  is operating by a physically present user.

  Programmatic changing of platform security policy (such as disable Secure Boot,
  or switch between Standard/Custom Secure Boot mode) MUST NOT be possible during
  Boot Services or after exiting EFI Boot Services. Only a physically present user
  is allowed to perform these operations.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    The platform is operated by a physically present user.
  @retval  FALSE   The platform is NOT operated by a physically present user.

**/
typedef
BOOLEAN
(EFIAPI *IS_USER_PHYSICAL_PRESENT) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

/**
  Caculate hash of multiple buffers using the specified Hash algorithm.

  @param[in]       This          Pointer to located SECUREBOOT_HELPER_PROTOCOL.
  @param[in]       HashAlg       Hash algorithm type see HASHALG_"XXX" defs.
  @param[in, out]  DigestSize    Size of Digest, on out bytes used in Digest.
  @param[out]      Digest        Hash digest written to this buffer.
  @param          ...            Pairs of parmas, Buffer Base pointer (A NULL
                                 terminates the list) followed by buffer len.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER DigestBuf == NULL or DigestSize == NULL.
  @retval EFI_BUFFER_TOO_SMALL  DigestBuf buffer is too small.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete operation.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.
  @retval EFI_UNSUPPORTED       The specified algorithm is not supported.

  **/
typedef
EFI_STATUS
(EFIAPI *HASH_MULTIPLE_BUFFERS) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST UINT32                         HashAlg,
  IN OUT UINTN                            *DigestSize,
  OUT UINT8                               *DigestBuf,
  ...
  );

/** Load PE/COFF image information into buffer and check its validity.

  Function returns Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO see
  Protocol/SecureBootHelper.h.

  Function also calculates HASH of image if CalcHash param == TRUE.
  If SecurityCert in image then hash algo. is the algo specified by the
  Cert else a SHA256 digest is calculated.

  Caller must FreePool returned pointer when they are finished.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       FileDevicePath Device Path of PE image file.
  @param[in]       CalcHash       Flag to tell loader to calc. hash of image.

  @retval  NULL unable to load image.
  @return  Pointer to SECUREBOOT_PEIMAGE_LOAD_INFO struct which contains a
           memory block with image info and actual image data.

**/
typedef
SECUREBOOT_PEIMAGE_LOAD_INFO *
(EFIAPI *LOAD_PE_IMAGE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST BOOLEAN                        CalcHash
  );

/** Check if secure boot enabled.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    Secure Boot enabled.
  @retval  FALSE   Secure Boot disabled.

**/
typedef
BOOLEAN
(EFIAPI *IS_SECURE_BOOT_ENABLED) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

/** Check if secure boot mode is in custom mode.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    Secure Boot mode is in custom mode.
  @retval  FALSE   Secure Boot mode is NOT in custom mode.

**/
typedef
BOOLEAN
(EFIAPI *IS_SECURE_BOOT_IN_CUSTOM_MODE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

/** Set Secure Boot Mode to Custom Mode.

  To be called when for SecureBoot in Setup or when the platform
  is operating by a physically present user..

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  EFI_SUCCESS            Set success.
  @retval  EFI_ACCESS_DENIED      Can't set mode if Secure Boot enabled.
  @retval  EFI_INVALID_PARAMETER  This pointer is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @return  Other                  Unexpected error - unable to verify.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_SET_SECURE_BOOT_CUSTOM_MODE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

/** Set Secure Boot Mode to default security Mode.

  To be called when for SecureBoot in Setup/Custom mode.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  EFI_SUCCESS            Set success.
  @retval  EFI_ACCESS_DENIED      Can't set mode if Secure Boot enabled.
  @retval  EFI_INVALID_PARAMETER  This pointer is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @return  Other                  Unexpected error - unable to verify.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_SET_SECURE_BOOT_DEFAULT_MODE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

/** Check if Platform Key is enrolled.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[out]      Optional return of size of 'pk' variable.
  @param[out]      Optional return of attributes of 'pk' variable.

  @retval  TRUE    Platform Key is enrolled.
  @retval  FALSE   System has no Platform Key or Error with key var.

**/
typedef
BOOLEAN
(EFIAPI *IS_PK_ENROLLED) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT UINTN                               *PkVarSize OPTIONAL,
  OUT UINT32                              *PkVarAttr OPTIONAL
  );

/**
  Create time based authenticated variable playload for SecureBoot Setup Mode.

  Also work in SecureBoot Custom Mode.

  @param[in]       This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in, out]  DataSize        On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]  Data            On input, Pointer to data buffer to be wrapped or 
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it. 

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resourses to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_CREATE_TIME_BASED_AUTH_VAR_PAYLOAD) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN OUT UINTN                            *DataSize,
  IN OUT UINT8                            **Data
  );

/**  Set authenticated variable for SecureBoot in Setup or Custom mode.

  This code sets an authenticated variable in storage blocks
  (Volatile or Non-Volatile).

  @param[in] This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in] VariableName    Name of Variable to set.
  @param[in] VendorGuid      Variable vendor GUID.
  @param[in] Attr            Attribute value of the variable found
  @param[in] DataSize        Size of Data found. If size is less than the
                             data, this value contains the required size.
  @param[in] Data            Data pointer.
  @param[in] AppendIfExists  If TRUE then force append of data if var exists.

  @retval    EFI_INVALID_PARAMETER        Invalid parameter.
  @retval    EFI_SUCCESS                  Set successfully.
  @retval    EFI_OUT_OF_RESOURCES         Resource not enough to set variable.
  @retval    EFI_NOT_FOUND                Not found.
  @retval    EFI_WRITE_PROTECTED          Variable is read-only.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_SET_TIME_BASED_AUTH_VARIABLE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN UINT32                               Attr,
  IN UINTN                                DataSize,
  IN UINT8                                *Data,
  IN BOOLEAN                              AppendIfExists
  );

/** Delete entry from secure boot database for SecureBoot in Setup/Custom mode.

  @param[in]       This                Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName        The variable name of the vendor's signature database.
  @param[in]       VendorGuid          A unique identifier for the vendor.
  @param[in]       SignatureOwner      Signature GUID to delete.

  @retval  EFI_SUCCESS                 Delete cert success.
  @retval  EFI_INVALID_PARAMETER       Invalid parameter.
  @retval  EFI_NOT_FOUND               Can't find the cert item.
  @retval  EFI_OUT_OF_RESOURCES        Could not allocate needed resources.

  **/
typedef
EFI_STATUS
(EFIAPI *SETUP_DELETE_CERT) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *SignatureOwner
  );

/** Append cert to KEK, DB or DBX vars for SecureBoot in Setup/Custom mode.

  @param[in] This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in] VariableName    Variable name of signature database.
  @param[in] VendorGuid      Variable vendor GUID.
  @param[in] CertType        Signature hash type.
  @param[in] SignatureOwner  GUID to identify signature.
  @param[in] SigDataSize     Signature data size.
  @param[in] SigData         Signature data.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_APPEND_CERT) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *CertType,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL,
  IN CONST UINTN                          SigDataSize OPTIONAL,
  IN CONST UINT8                          *SigData OPTIONAL
  );

/** Find first cert. list in security database that matches filter criteria.

  If Filter == NULL then return first cert list in database.

  Caller must call Find Cert Close to free resources allocated by this find.

  @param[in]       This          Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName  Security database variable name.
  @param[in]       VendorGuid    Security database variable vendor GUID.
  @param[in]       Filter        Filter criteria.
  @param[out]      CertList      Pointer to update with address of EFI_SIGNATURE_LIST.
  @param[out]      CertData      Pointer to update with address of EFI_SIGNATURE_DATA.

  @retval NULL   Nothing found in database.
  @retval other  Pointer to this found context to be passed to Find Next Cert
                 and Find Cert Close.
  @return Updated CertList with address of EFI_SIGNATURE_LIST structure of the
          found certificate list.
  @return Optionally update CertData with address of EFI_SIGNATURE_DATA of the
          found certificate list.

**/
typedef
VOID *
(EFIAPI *FIND_FIRST_CERT) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN FIND_CERT_FILTER                     *Filter OPTIONAL,
  OUT EFI_SIGNATURE_LIST                  **CertList,
  OUT EFI_SIGNATURE_DATA                  **CertData OPTIONAL,
  OUT FIND_CERT_STATS                     *Stats OPTIONAL
  );

/** Find next cert. list in security database that matches filter criteria.

  If Filter == NULL then return next cert list in database.

  @param[in]       FindContext   Return from previous call to Find First Cert.
  @param[in]       Filter        Filter criteria.
  @param[out]      CurrCertList  Current position in security database.
  @param[out]      CertData      Pointer to update with address of EFI_SIGNATURE_DATA.

  @retval NULL   Reached end of database.
  @retval other  Pointer to EFI_SIGNATURE_LIST structure of the
                 next certificate list.
  @return Optionally update CertData with address of EFI_SIGNATURE_DATA of the
          next certificate list.

**/
typedef
EFI_SIGNATURE_LIST *
(EFIAPI *FIND_NEXT_CERT) (
  IN VOID                                 *FindContext,
  IN FIND_CERT_FILTER                     *Filter,
  IN EFI_SIGNATURE_LIST                   *CurrCertList,
  OUT EFI_SIGNATURE_DATA                  **CertData
  );

/** Free resources allocated by Find First Cert.

  @param[in]       FindContext   Return from previous call to Find First Cert.

**/
typedef
VOID
(EFIAPI *FIND_CERT_CLOSE) (
  IN VOID                                 *FindContext
  );

/**  Enroll X509 Cert file into PK, KEK, DB or DBX database.

  For use with SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       FileDevicePath Device Path of X509 certificate file (.der /.cer / .crt)
                                  in format using ASN.1 DER encoding.
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing a copy
                                  of the certificate, overrides FileDevicePath.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_ENROLL_X509) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

/**  Enroll Rsa2048 storing file (*.pbk) into KEK database.

  SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       FileDevicePath Device Path of Rsa2048 storing file (*.pbk).
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing a copy
                                  of the pbk file, overrides FileDevicePath.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_UNSUPPORTED        Unsupported file.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_ENROLL_KEK_RSA2048) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

/**  Enroll Image Signature into secure boot database.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       FileDevicePath Device Path of image to enroll.
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing SHA256
                                  32 byte image digest. Assumed to match &gEfiCertSha256Guid.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_UNSUPPORTED        Unsupported image type.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_ENROLL_IMAGE_SIGNATURE) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

/**  Delete record in SecureBoot PK, KEK, DB or DBX or delete a complete store.

  SecureBoot in Setup/Custom mode.

  if CompleteStore then reset StoreType with all certs deleted.
  CompleteStore param ignored if StoreType == PKStore ie complete PK always reset.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       CompleteStore  If true reset complete secure boot store (StoreType).
  @param[in]       SignatureOwner Specific record in store to delete.

  @retval  EFI_SUCCESS            Delete success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to do operation.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_DELETE_STORE_RECORD) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST BOOLEAN                        CompleteStore,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

/** Get the file image from mask.

  If This or File pointers are NULL, then ASSERT().
  If sanity checks on This pointer fail then ASSERT().

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       File  This is a pointer to the device path of the file
                         to get IMAGE_FROM_XXX mask for.

  @return UINT32 Mask which is one of IMAGE_FROM_XXX defs in protocol header file.

**/

typedef
UINT32
(EFIAPI *GET_IMAGE_FROM_MASK) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *File
  );

/**  Get statistics for SecureBoot PK, KEK, DB and DBX variables.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[out]      Stats          Caller allocated SECUREBOOT_VARIABLE_STATS
                                  struct filled out by this routine.

  @retval  EFI_SUCCESS            Operation success with Stats filled out.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *GET_SECUREBOOT_VARIABLE_STATS) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT SECUREBOOT_VARIABLE_STATS           *Stats
  );

/**  Create Time based authenticated variable for SecureBoot Setup/Custom Mode.

  @param[in]       This             Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName     Name of Variable to set.
  @param[in]       VendorGuid       Variable vendor GUID.
  @param[in]       LeaveIfExists    If variable already exists do nothing.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    EFI_WRITE_PROTECTED    Variable is read-only.
  @retval    Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_CREATE_TIME_BASED_AUTH_VAR) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN  CHAR16                              *VariableName,
  IN  EFI_GUID                            *VendorGuid,
  IN CONST BOOLEAN                        LeaveIfExists
  );

/**  Create Empty DB & DBX if they do not exist.

  SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    Others                 Unexpected error happened.

**/
typedef
EFI_STATUS
(EFIAPI *SETUP_CREATE_EMPTY_DB_DBX_IF_NOT_EXIST) (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

///
/// Secure Boot helper protocol declaration.
///
struct _SECUREBOOT_HELPER_PROTOCOL {
  IS_USER_PHYSICAL_PRESENT                  IsUserPhysicalPresent;              ///< see service typedef.
  HASH_MULTIPLE_BUFFERS                     HashMultipleBuffers;                ///< see service typedef.
  LOAD_PE_IMAGE                             LoadPeImage;                        ///< see service typedef.
  IS_SECURE_BOOT_ENABLED                    IsSecureBootEnabled;                ///< see service typedef.
  IS_SECURE_BOOT_IN_CUSTOM_MODE             IsSecureBootInCustomMode;           ///< see service typedef.
  SETUP_SET_SECURE_BOOT_CUSTOM_MODE         SetupSetSecureBootCustomMode;       ///< see service typedef.
  SETUP_SET_SECURE_BOOT_DEFAULT_MODE        SetupSetSecureBootDefaultMode;      ///< see service typedef.
  SETUP_CREATE_TIME_BASED_AUTH_VAR_PAYLOAD  SetupCreateTimeBasedAuthVarPayload; ///< see service typedef.
  SETUP_SET_TIME_BASED_AUTH_VARIABLE        SetupSetTimeBasedAuthVariable;      ///< see service typedef.
  SETUP_DELETE_CERT                         SetupDeleteCert;                    ///< see service typedef.
  SETUP_APPEND_CERT                         SetupAppendCert;                    ///< see service typedef.
  FIND_FIRST_CERT                           FindFirstCert;                      ///< see service typedef.
  FIND_NEXT_CERT                            FindNextCert;                       ///< see service typedef.
  FIND_CERT_CLOSE                           FindCertClose;                      ///< see service typedef.
  IS_PK_ENROLLED                            IsPkEnrolled;                       ///< see service typedef.
  SETUP_ENROLL_X509                         SetupEnrollX509;                    ///< see service typedef.
  SETUP_ENROLL_KEK_RSA2048                  SetupEnrollKekRsa2048;              ///< see service typedef.
  SETUP_ENROLL_IMAGE_SIGNATURE              SetupEnrollImageSignature;          ///< see service typedef.
  SETUP_DELETE_STORE_RECORD                 SetupDeleteStoreRecord;             ///< see service typedef.
  GET_IMAGE_FROM_MASK                       GetImageFromMask;                   ///< see service typedef.
  GET_SECUREBOOT_VARIABLE_STATS             GetSecureBootVarStats;              ///< see service typedef.
  SETUP_CREATE_TIME_BASED_AUTH_VAR          SetupCreateTimeBasedAuthVar;        ///< see service typedef.
  SETUP_CREATE_EMPTY_DB_DBX_IF_NOT_EXIST    SetupCreateEmptyDbDbxIfNotExist;    ///< see service typedef.
};

#endif  // _SECUREBOOT_HELPER_PROTOCOL_H_
