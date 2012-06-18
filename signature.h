#define SHA256_DIGEST_SIZE  32

EFI_GUID EfiCertSha256Guid = { 0xc1c41626, 0x504c, 0x4092, {0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28 }};

typedef struct {
	///
	/// An identifier which identifies the agent which added the signature to the list.
	///
	EFI_GUID          SignatureOwner;
	///
	/// The format of the signature is defined by the SignatureType.
	///
	UINT8             SignatureData[1];
} EFI_SIGNATURE_DATA;

typedef struct {
	///
	/// Type of the signature. GUID signature types are defined in below.
	///
	EFI_GUID            SignatureType;
	///
	/// Total size of the signature list, including this header.
	///
	UINT32              SignatureListSize;
	///
	/// Size of the signature header which precedes the array of signatures.
	///
	UINT32              SignatureHeaderSize;
	///
	/// Size of each signature.
	///
	UINT32              SignatureSize; 
	///
	/// Header before the array of signatures. The format of this header is specified 
	/// by the SignatureType.
	/// UINT8           SignatureHeader[SignatureHeaderSize];
	///
	/// An array of signatures. Each signature is SignatureSize bytes in length. 
	/// EFI_SIGNATURE_DATA Signatures[][SignatureSize];
	///
} EFI_SIGNATURE_LIST;
