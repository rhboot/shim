#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <Library/BaseCryptLib.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/asn1.h>
#include <openssl/bn.h>
#include "shim.h"
#include "PeImage.h"
#include "PasswordCrypt.h"

#include "guid.h"
#include "console.h"
#include "variables.h"
#include "simple_file.h"
#include "efiauthenticated.h"

#define PASSWORD_MAX 256
#define PASSWORD_MIN 1
#define SB_PASSWORD_LEN 16

#define NAME_LINE_MAX 70

#ifndef SHIM_VENDOR
#define SHIM_VENDOR L"Shim"
#endif

EFI_GUID SHIM_LOCK_GUID = { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} };
EFI_GUID EFI_CERT_SHA224_GUID = { 0xb6e5233, 0xa65c, 0x44c9, {0x94, 0x7, 0xd9, 0xab, 0x83, 0xbf, 0xc8, 0xbd} };
EFI_GUID EFI_CERT_SHA384_GUID = { 0xff3e5307, 0x9fd0, 0x48c9, {0x85, 0xf1, 0x8a, 0xd5, 0x6c, 0x70, 0x1e, 0x1} };
EFI_GUID EFI_CERT_SHA512_GUID = { 0x93e0fae, 0xa6c4, 0x4f50, {0x9f, 0x1b, 0xd4, 0x1e, 0x2b, 0x89, 0xc1, 0x9a} };

#define CERT_STRING L"Select an X509 certificate to enroll:\n\n"
#define HASH_STRING L"Select a file to trust:\n\n"

struct menu_item {
	CHAR16 *text;
	INTN (* callback)(void *data, void *data2, void *data3);
	void *data;
	void *data2;
	void *data3;
	UINTN colour;
};

typedef struct {
	UINT32 MokSize;
	UINT8 *Mok;
	EFI_GUID Type;
} __attribute__ ((packed)) MokListNode;

typedef struct {
	UINT32 MokSBState;
	UINT32 PWLen;
	CHAR16 Password[SB_PASSWORD_LEN];
} __attribute__ ((packed)) MokSBvar;

typedef struct {
	UINT32 MokDBState;
	UINT32 PWLen;
	CHAR16 Password[SB_PASSWORD_LEN];
} __attribute__ ((packed)) MokDBvar;

static EFI_STATUS get_sha1sum (void *Data, int DataSize, UINT8 *hash)
{
	EFI_STATUS status;
	unsigned int ctxsize;
	void *ctx = NULL;

	ctxsize = Sha1GetContextSize();
	ctx = AllocatePool(ctxsize);

	if (!ctx) {
		console_notify(L"Unable to allocate memory for hash context");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha1Init(ctx)) {
		console_notify(L"Unable to initialise hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha1Update(ctx, Data, DataSize))) {
		console_notify(L"Unable to generate hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha1Final(ctx, hash))) {
		console_notify(L"Unable to finalise hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	status = EFI_SUCCESS;
done:
	return status;
}

static BOOLEAN is_sha2_hash (EFI_GUID Type)
{
	EFI_GUID Sha224 = EFI_CERT_SHA224_GUID;
	EFI_GUID Sha256 = EFI_CERT_SHA256_GUID;
	EFI_GUID Sha384 = EFI_CERT_SHA384_GUID;
	EFI_GUID Sha512 = EFI_CERT_SHA512_GUID;

	if (CompareGuid(&Type, &Sha224) == 0)
		return TRUE;
	else if (CompareGuid(&Type, &Sha256) == 0)
		return TRUE;
	else if (CompareGuid(&Type, &Sha384) == 0)
		return TRUE;
	else if (CompareGuid(&Type, &Sha512) == 0)
		return TRUE;

	return FALSE;
}

static UINT32 sha_size (EFI_GUID Type)
{
	EFI_GUID Sha1 = EFI_CERT_SHA1_GUID;
	EFI_GUID Sha224 = EFI_CERT_SHA224_GUID;
	EFI_GUID Sha256 = EFI_CERT_SHA256_GUID;
	EFI_GUID Sha384 = EFI_CERT_SHA384_GUID;
	EFI_GUID Sha512 = EFI_CERT_SHA512_GUID;

	if (CompareGuid(&Type, &Sha1) == 0)
		return SHA1_DIGEST_SIZE;
	else if (CompareGuid(&Type, &Sha224) == 0)
		return SHA224_DIGEST_LENGTH;
	else if (CompareGuid(&Type, &Sha256) == 0)
		return SHA256_DIGEST_SIZE;
	else if (CompareGuid(&Type, &Sha384) == 0)
		return SHA384_DIGEST_LENGTH;
	else if (CompareGuid(&Type, &Sha512) == 0)
		return SHA512_DIGEST_LENGTH;

	return 0;
}

static BOOLEAN is_valid_siglist (EFI_GUID Type, UINT32 SigSize)
{
	EFI_GUID CertType = X509_GUID;
	UINT32 hash_sig_size;

	if (CompareGuid (&Type, &CertType) == 0 && SigSize != 0)
		return TRUE;

	if (!is_sha2_hash (Type))
		return FALSE;

	hash_sig_size = sha_size (Type) + sizeof(EFI_GUID);
	if (SigSize != hash_sig_size)
		return FALSE;

	return TRUE;
}

static UINT32 count_keys(void *Data, UINTN DataSize)
{
	EFI_SIGNATURE_LIST *CertList = Data;
	UINTN dbsize = DataSize;
	UINT32 MokNum = 0;
	void *end = Data + DataSize;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {

		/* Use ptr arithmetics to ensure bounded access. Do not allow 0
		 * SignatureListSize that will cause endless loop.
		 */
		if ((void *)(CertList + 1) > end || CertList->SignatureListSize == 0) {
			console_notify(L"Invalid MOK detected! Ignoring MOK List.");
			return 0;
		}

		if (CertList->SignatureListSize == 0 ||
		    CertList->SignatureListSize <= CertList->SignatureSize) {
			console_errorbox(L"Corrupted signature list");
			return 0;
		}

		if (!is_valid_siglist(CertList->SignatureType, CertList->SignatureSize)) {
			console_errorbox(L"Invalid signature list found");
			return 0;
		}

		MokNum++;
		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
	}

	return MokNum;
}

static MokListNode *build_mok_list(UINT32 num, void *Data, UINTN DataSize) {
	MokListNode *list;
	EFI_SIGNATURE_LIST *CertList = Data;
	EFI_SIGNATURE_DATA *Cert;
	EFI_GUID CertType = X509_GUID;
	UINTN dbsize = DataSize;
	UINTN count = 0;
	void *end = Data + DataSize;

	list = AllocatePool(sizeof(MokListNode) * num);

	if (!list) {
		console_notify(L"Unable to allocate MOK list");
		return NULL;
	}

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		/* CertList out of bounds? */
		if ((void *)(CertList + 1) > end || CertList->SignatureListSize == 0) {
			FreePool(list);
			return NULL;
		}

		/* Omit the signature check here since we already did it
		   in count_keys() */

		Cert = (EFI_SIGNATURE_DATA *) (((UINT8 *) CertList) +
		  sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);

		/* Cert out of bounds? */
		if ((void *)(Cert + 1) > end || CertList->SignatureSize <= sizeof(EFI_GUID)) {
			FreePool(list);
			return NULL;
		}

		list[count].Type = CertList->SignatureType;
		if (CompareGuid (&CertList->SignatureType, &CertType) == 0) {
			list[count].MokSize = CertList->SignatureSize -
					      sizeof(EFI_GUID);
			list[count].Mok = (void *)Cert->SignatureData;
		} else {
			list[count].MokSize = CertList->SignatureListSize -
					      sizeof(EFI_SIGNATURE_LIST);
			list[count].Mok = (void *)Cert;
		}

		/* MOK out of bounds? */
		if (list[count].MokSize > (unsigned long)end -
					  (unsigned long)list[count].Mok) {
			FreePool(list);
			return NULL;
		}

		count++;
		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
	}

	return list;
}

typedef struct {
	int nid;
	CHAR16 *name;
} NidName;

static NidName nidname[] = {
	{NID_commonName, L"CN"},
	{NID_organizationName, L"O"},
	{NID_countryName, L"C"},
	{NID_stateOrProvinceName, L"ST"},
	{NID_localityName, L"L"},
	{-1, NULL}
};

static CHAR16* get_x509_name (X509_NAME *X509Name)
{
	CHAR16 name[NAME_LINE_MAX+1];
	CHAR16 part[NAME_LINE_MAX+1];
	char str[NAME_LINE_MAX];
	int i, len, rest, first;

	name[0] = '\0';
	rest = NAME_LINE_MAX;
	first = 1;
	for (i = 0; nidname[i].name != NULL; i++) {
		int add;
		len = X509_NAME_get_text_by_NID (X509Name, nidname[i].nid,
						 str, NAME_LINE_MAX);
		if (len <= 0)
			continue;

		if (first)
			add = len + (int)StrLen(nidname[i].name) + 1;
		else
			add = len + (int)StrLen(nidname[i].name) + 3;

		if (add > rest)
			continue;

		if (first) {
			SPrint(part, NAME_LINE_MAX * sizeof(CHAR16), L"%s=%a",
			       nidname[i].name, str);
		} else {
			SPrint(part, NAME_LINE_MAX * sizeof(CHAR16), L", %s=%a",
			       nidname[i].name, str);
		}
		StrCat(name, part);
		rest -= add;
		first = 0;
	}

	if (rest >= 0 && rest < NAME_LINE_MAX)
		return PoolPrint(L"%s", name);

	return NULL;
}

static CHAR16* get_x509_time (ASN1_TIME *time)
{
	BIO *bio = BIO_new (BIO_s_mem());
	char str[30];
	int len;

	ASN1_TIME_print (bio, time);
	len = BIO_read(bio, str, 29);
	if (len < 0)
		len = 0;
	str[len] = '\0';
	BIO_free (bio);

	return PoolPrint(L"%a", str);
}

static void show_x509_info (X509 *X509Cert, UINT8 *hash)
{
	ASN1_INTEGER *serial;
	BIGNUM *bnser;
	unsigned char hexbuf[30];
	X509_NAME *X509Name;
	ASN1_TIME *time;
	CHAR16 *issuer = NULL;
	CHAR16 *subject = NULL;
	CHAR16 *from = NULL;
	CHAR16 *until = NULL;
	EXTENDED_KEY_USAGE *extusage;
	POOL_PRINT hash_string1;
	POOL_PRINT hash_string2;
	POOL_PRINT serial_string;
	int fields = 0;
	CHAR16 **text;
	int i = 0;

	ZeroMem(&hash_string1, sizeof(hash_string1));
	ZeroMem(&hash_string2, sizeof(hash_string2));
	ZeroMem(&serial_string, sizeof(serial_string));

	serial = X509_get_serialNumber(X509Cert);
	if (serial) {
		int i, n;
		bnser = ASN1_INTEGER_to_BN(serial, NULL);
		n = BN_bn2bin(bnser, hexbuf);
		for (i = 0; i < n; i++) {
			CatPrint(&serial_string, L"%02x:", hexbuf[i]);
		}
	}

	if (serial_string.str)
		fields++;

	X509Name = X509_get_issuer_name(X509Cert);
	if (X509Name) {
		issuer = get_x509_name(X509Name);
		if (issuer)
			fields++;
	}

	X509Name = X509_get_subject_name(X509Cert);
	if (X509Name) {
		subject = get_x509_name(X509Name);
		if (subject)
			fields++;
	}

	time = X509_getm_notBefore(X509Cert);
	if (time) {
		from = get_x509_time(time);
		if (from)
			fields++;
	}

	time = X509_getm_notAfter(X509Cert);
	if (time) {
		until = get_x509_time(time);
		if (until)
			fields++;
	}

	for (i=0; i<10; i++)
		CatPrint(&hash_string1, L"%02x ", hash[i]);
	for (i=10; i<20; i++)
		CatPrint(&hash_string2, L"%02x ", hash[i]);

	if (hash_string1.str)
		fields++;

	if (hash_string2.str)
		fields++;

	if (!fields)
		return;

	i = 0;

	extusage = X509_get_ext_d2i(X509Cert, NID_ext_key_usage, NULL, NULL);
	text = AllocateZeroPool(sizeof(CHAR16 *) * (fields*3 + sk_ASN1_OBJECT_num(extusage) + 3));

	if (extusage) {
		int j = 0;

		text[i++] = StrDuplicate(L"[Extended Key Usage]");

		for (j = 0; j < sk_ASN1_OBJECT_num(extusage); j++) {
			POOL_PRINT extkeyusage;
			ASN1_OBJECT *obj = sk_ASN1_OBJECT_value(extusage, j);
			int buflen = 80;
			char buf[buflen];

			ZeroMem(&extkeyusage, sizeof(extkeyusage));

			OBJ_obj2txt(buf, buflen, obj, 0);
			CatPrint(&extkeyusage, L"OID: %a", buf);
			text[i++] = StrDuplicate(extkeyusage.str);
			FreePool(extkeyusage.str);
		}
		text[i++] = StrDuplicate(L"");
		EXTENDED_KEY_USAGE_free(extusage);
	}

	if (serial_string.str) {
		text[i++] = StrDuplicate(L"[Serial Number]");
		text[i++] = serial_string.str;
		text[i++] = StrDuplicate(L"");
	}
	if (issuer) {
		text[i++] = StrDuplicate(L"[Issuer]");
		text[i++] = issuer;
		text[i++] = StrDuplicate(L"");
	}
	if (subject) {
		text[i++] = StrDuplicate(L"[Subject]");
		text[i++] = subject;
		text[i++] = StrDuplicate(L"");
	}
	if (from) {
		text[i++] = StrDuplicate(L"[Valid Not Before]");
		text[i++] = from;
		text[i++] = StrDuplicate(L"");
	}
	if (until) {
		text[i++] = StrDuplicate(L"[Valid Not After]");
		text[i++] = until;
		text[i++] = StrDuplicate(L"");
	}
	if (hash_string1.str) {
		text[i++] = StrDuplicate(L"[Fingerprint]");
		text[i++] = hash_string1.str;
	}
	if (hash_string2.str) {
		text[i++] = hash_string2.str;
		text[i++] = StrDuplicate(L"");
	}
	text[i] = NULL;

	console_print_box(text, -1);

	for (i=0; text[i] != NULL; i++)
		FreePool(text[i]);

	FreePool(text);
}

static void show_sha_digest (EFI_GUID Type, UINT8 *hash)
{
	EFI_GUID Sha1 = EFI_CERT_SHA1_GUID;
	EFI_GUID Sha224 = EFI_CERT_SHA224_GUID;
	EFI_GUID Sha256 = EFI_CERT_SHA256_GUID;
	EFI_GUID Sha384 = EFI_CERT_SHA384_GUID;
	EFI_GUID Sha512 = EFI_CERT_SHA512_GUID;
	CHAR16 *text[5];
	POOL_PRINT hash_string1;
	POOL_PRINT hash_string2;
	int i;
	int length;

	if (CompareGuid(&Type, &Sha1) == 0) {
		length = SHA1_DIGEST_SIZE;
		text[0] = L"SHA1 hash";
	} else if (CompareGuid(&Type, &Sha224) == 0) {
		length = SHA224_DIGEST_LENGTH;
		text[0] = L"SHA224 hash";
	} else if (CompareGuid(&Type, &Sha256) == 0) {
		length = SHA256_DIGEST_SIZE;
		text[0] = L"SHA256 hash";
	} else if (CompareGuid(&Type, &Sha384) == 0) {
		length = SHA384_DIGEST_LENGTH;
		text[0] = L"SHA384 hash";
	} else if (CompareGuid(&Type, &Sha512) == 0) {
		length = SHA512_DIGEST_LENGTH;
		text[0] = L"SHA512 hash";
	} else {
		return;
	}

	ZeroMem(&hash_string1, sizeof(hash_string1));
	ZeroMem(&hash_string2, sizeof(hash_string2));

	text[1] = L"";

	for (i=0; i<length/2; i++)
		CatPrint(&hash_string1, L"%02x ", hash[i]);
	for (i=length/2; i<length; i++)
		CatPrint(&hash_string2, L"%02x ", hash[i]);

	text[2] = hash_string1.str;
	text[3] = hash_string2.str;
	text[4] = NULL;

	console_print_box(text, -1);

	if (hash_string1.str)
		FreePool(hash_string1.str);

	if (hash_string2.str)
		FreePool(hash_string2.str);
}

static void show_efi_hash (EFI_GUID Type, void *Mok, UINTN MokSize)
{
	UINTN sig_size;
	UINTN hash_num;
	UINT8 *hash;
	CHAR16 **menu_strings;
	UINTN key_num = 0;
	UINTN i;

	sig_size = sha_size(Type) + sizeof(EFI_GUID);
	if ((MokSize % sig_size) != 0) {
		console_errorbox(L"Corrupted Hash List");
		return;
	}
	hash_num = MokSize / sig_size;

	if (hash_num == 1) {
		hash = (UINT8 *)Mok + sizeof(EFI_GUID);
		show_sha_digest(Type, hash);
		return;
	}

	menu_strings = AllocateZeroPool(sizeof(CHAR16 *) * (hash_num + 2));
	if (!menu_strings) {
		console_errorbox(L"Out of Resources");
		return;
	}
	for (i=0; i<hash_num; i++) {
		menu_strings[i] = PoolPrint(L"View hash %d", i);
	}
	menu_strings[i] = StrDuplicate(L"Back");
	menu_strings[i+1] = NULL;

	while (key_num < hash_num) {
		key_num = console_select((CHAR16 *[]){ L"[Hash List]", NULL },
					 menu_strings, key_num);

		if (key_num < 0 || key_num >= hash_num)
			break;

		hash = (UINT8 *)Mok + sig_size*key_num + sizeof(EFI_GUID);
		show_sha_digest(Type, hash);
	}

	for (i=0; menu_strings[i] != NULL; i++)
		FreePool(menu_strings[i]);

	FreePool(menu_strings);
}

static void show_mok_info (EFI_GUID Type, void *Mok, UINTN MokSize)
{
	EFI_STATUS efi_status;
	EFI_GUID CertType = X509_GUID;

	if (!Mok || MokSize == 0)
		return;

	if (CompareGuid (&Type, &CertType) == 0) {
		UINT8 hash[SHA1_DIGEST_SIZE];
		X509 *X509Cert;
		efi_status = get_sha1sum(Mok, MokSize, hash);

		if (efi_status != EFI_SUCCESS) {
			console_notify(L"Failed to compute MOK fingerprint");
			return;
		}

		if (X509ConstructCertificate(Mok, MokSize,
				 (UINT8 **) &X509Cert) && X509Cert != NULL) {
			show_x509_info(X509Cert, hash);
			X509_free(X509Cert);
		} else {
			console_notify(L"Not a valid X509 certificate");
			return;
		}
	} else if (is_sha2_hash(Type)) {
		show_efi_hash(Type, Mok, MokSize);
	}
}

static EFI_STATUS list_keys (void *KeyList, UINTN KeyListSize, CHAR16 *title)
{
	UINTN MokNum = 0;
	MokListNode *keys = NULL;
	UINT32 key_num = 0;
	CHAR16 **menu_strings;
	unsigned int i;

	if (KeyListSize < (sizeof(EFI_SIGNATURE_LIST) +
			   sizeof(EFI_SIGNATURE_DATA))) {
		console_notify(L"No MOK keys found");
		return EFI_NOT_FOUND;
	}

	MokNum = count_keys(KeyList, KeyListSize);
	if (MokNum == 0) {
		console_errorbox(L"Invalid key list");
		return EFI_ABORTED;
	}
	keys = build_mok_list(MokNum, KeyList, KeyListSize);
	if (!keys) {
		console_errorbox(L"Failed to construct key list");
		return EFI_ABORTED;
	}

	menu_strings = AllocateZeroPool(sizeof(CHAR16 *) * (MokNum + 2));

	if (!menu_strings)
		return EFI_OUT_OF_RESOURCES;

	for (i=0; i<MokNum; i++) {
		menu_strings[i] = PoolPrint(L"View key %d", i);
	}
	menu_strings[i] = StrDuplicate(L"Continue");

	menu_strings[i+1] = NULL;

	while (key_num < MokNum) {
		key_num = console_select((CHAR16 *[]){ title, NULL },
					 menu_strings, key_num);

		if (key_num < 0 || key_num >= MokNum)
			break;

		show_mok_info(keys[key_num].Type, keys[key_num].Mok,
			      keys[key_num].MokSize);
	}

	for (i=0; menu_strings[i] != NULL; i++)
		FreePool(menu_strings[i]);

	FreePool(menu_strings);

	FreePool(keys);

	return EFI_SUCCESS;
}

static EFI_STATUS get_line (UINT32 *length, CHAR16 *line, UINT32 line_max, UINT8 show)
{
	EFI_INPUT_KEY key;
	EFI_STATUS status;
	unsigned int count = 0;

	do {
		status = console_get_keystroke(&key);
		if (EFI_ERROR (status)) {
			console_error(L"Failed to read the keystroke", status);
			*length = 0;
			return status;
		}

		if ((count >= line_max &&
		     key.UnicodeChar != CHAR_BACKSPACE) ||
		    key.UnicodeChar == CHAR_NULL ||
		    key.UnicodeChar == CHAR_TAB  ||
		    key.UnicodeChar == CHAR_LINEFEED ||
		    key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
			continue;
		}

		if (count == 0 && key.UnicodeChar == CHAR_BACKSPACE) {
			continue;
		} else if (key.UnicodeChar == CHAR_BACKSPACE) {
			if (show) {
				Print(L"\b");
			}
			line[--count] = '\0';
			continue;
		}

		if (show) {
			Print(L"%c", key.UnicodeChar);
		}

		line[count++] = key.UnicodeChar;
	} while (key.UnicodeChar != CHAR_CARRIAGE_RETURN);
	Print(L"\n");

	*length = count;

	return EFI_SUCCESS;
}

static EFI_STATUS compute_pw_hash (void *Data, UINTN DataSize, UINT8 *password,
				   UINT32 pw_length, UINT8 *hash)
{
	EFI_STATUS status;
	unsigned int ctxsize;
	void *ctx = NULL;

	ctxsize = Sha256GetContextSize();
	ctx = AllocatePool(ctxsize);

	if (!ctx) {
		console_notify(L"Unable to allocate memory for hash context");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(ctx)) {
		console_notify(L"Unable to initialise hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (Data && DataSize) {
		if (!(Sha256Update(ctx, Data, DataSize))) {
			console_notify(L"Unable to generate hash");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
	}

	if (!(Sha256Update(ctx, password, pw_length))) {
		console_notify(L"Unable to generate hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha256Final(ctx, hash))) {
		console_notify(L"Unable to finalise hash");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	status = EFI_SUCCESS;
done:
	return status;
}

static void console_save_and_set_mode (SIMPLE_TEXT_OUTPUT_MODE *SavedMode)
{
	if (!SavedMode) {
		Print(L"Invalid parameter: SavedMode\n");
		return;
	}

	CopyMem(SavedMode, ST->ConOut->Mode, sizeof(SIMPLE_TEXT_OUTPUT_MODE));
	uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, FALSE);
	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
			  EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);
}

static void console_restore_mode (SIMPLE_TEXT_OUTPUT_MODE *SavedMode)
{
	uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut,
			  SavedMode->CursorVisible);
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut,
			  SavedMode->CursorColumn, SavedMode->CursorRow);
	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
			  SavedMode->Attribute);
}

static INTN reset_system ()
{
	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm,
			  EFI_SUCCESS, 0, NULL);
	console_notify(L"Failed to reboot\n");
	return -1;
}

static UINT32 get_password (CHAR16 *prompt, CHAR16 *password, UINT32 max)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	CHAR16 *str;
	CHAR16 *message[2];
	UINTN length;
	UINT32 pw_length;

	if (!prompt)
		prompt = L"Password:";

	console_save_and_set_mode(&SavedMode);

	str = PoolPrint(L"%s            ", prompt);
	if (!str) {
		console_errorbox(L"Failed to allocate prompt");
		return 0;
	}

	message[0] = str;
	message[1] = NULL;
	length = StrLen(message[0]);
	console_print_box_at(message, -1, -length-4, -5, length+4, 3, 0, 1);
	get_line(&pw_length, password, max, 0);

	console_restore_mode(&SavedMode);

	FreePool(str);

	return pw_length;
}

static EFI_STATUS match_password (PASSWORD_CRYPT *pw_crypt,
				  void *Data, UINTN DataSize,
				  UINT8 *auth, CHAR16 *prompt)
{
	EFI_STATUS status;
	UINT8 hash[128];
	UINT8 *auth_hash;
	UINT32 auth_size;
	CHAR16 password[PASSWORD_MAX];
	UINT32 pw_length;
	UINT8 fail_count = 0;
	unsigned int i;

	if (pw_crypt) {
		auth_hash = pw_crypt->hash;
		auth_size = get_hash_size (pw_crypt->method);
		if (auth_size == 0)
			return EFI_INVALID_PARAMETER;
	} else if (auth) {
		auth_hash = auth;
		auth_size = SHA256_DIGEST_SIZE;
	} else {
		return EFI_INVALID_PARAMETER;
	}

	while (fail_count < 3) {
		pw_length = get_password(prompt, password, PASSWORD_MAX);

		if (pw_length < PASSWORD_MIN || pw_length > PASSWORD_MAX) {
			console_errorbox(L"Invalid password length");
			fail_count++;
			continue;
		}

		/*
		 * Compute password hash
		 */
		if (pw_crypt) {
			char pw_ascii[PASSWORD_MAX + 1];
			for (i = 0; i < pw_length; i++)
				pw_ascii[i] = (char)password[i];
			pw_ascii[pw_length] = '\0';

			status = password_crypt(pw_ascii, pw_length, pw_crypt, hash);
		} else {
			/*
			 * For backward compatibility
			 */
			status = compute_pw_hash(Data, DataSize, (UINT8 *)password,
						 pw_length * sizeof(CHAR16), hash);
		}
		if (status != EFI_SUCCESS) {
			console_errorbox(L"Unable to generate password hash");
			fail_count++;
			continue;
		}

		if (CompareMem(auth_hash, hash, auth_size) != 0) {
			console_errorbox(L"Password doesn't match");
			fail_count++;
			continue;
		}

		break;
	}

	if (fail_count >= 3)
		return EFI_ACCESS_DENIED;

	return EFI_SUCCESS;
}

static EFI_STATUS write_db (CHAR16 *db_name, void *MokNew, UINTN MokNewSize)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS status;
	UINT32 attributes;
	void *old_data = NULL;
	void *new_data = NULL;
	UINTN old_size;
	UINTN new_size;

	status = uefi_call_wrapper(RT->SetVariable, 5, db_name,
				   &shim_lock_guid,
				   EFI_VARIABLE_NON_VOLATILE
				   | EFI_VARIABLE_BOOTSERVICE_ACCESS
				   | EFI_VARIABLE_APPEND_WRITE,
				   MokNewSize, MokNew);
	if (status == EFI_SUCCESS || status != EFI_INVALID_PARAMETER) {
		return status;
	}

	status = get_variable_attr(db_name, (UINT8 **)&old_data, &old_size,
				   shim_lock_guid, &attributes);
	if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		return status;
	}

	/* Check if the old db is compromised or not */
	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		FreePool(old_data);
		old_data = NULL;
		old_size = 0;
	}

	new_size = old_size + MokNewSize;
	new_data = AllocatePool(new_size);
	if (new_data == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	CopyMem(new_data, old_data, old_size);
	CopyMem(new_data + old_size, MokNew, MokNewSize);

	status = uefi_call_wrapper(RT->SetVariable, 5, db_name,
				   &shim_lock_guid,
				   EFI_VARIABLE_NON_VOLATILE
				   | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				   new_size, new_data);

out:
	if (old_size > 0) {
		FreePool(old_data);
	}

	if (new_data != NULL) {
		FreePool(new_data);
	}

	return status;
}

static EFI_STATUS store_keys (void *MokNew, UINTN MokNewSize, int authenticate,
			      BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	CHAR16 *db_name;
	CHAR16 *auth_name;
	UINT8 auth[PASSWORD_CRYPT_SIZE];
	UINTN auth_size = PASSWORD_CRYPT_SIZE;
	UINT32 attributes;

	if (MokX) {
		db_name = L"MokListX";
		auth_name = L"MokXAuth";
	} else {
		db_name = L"MokList";
		auth_name = L"MokAuth";
	}

	if (authenticate) {
		efi_status = uefi_call_wrapper(RT->GetVariable, 5, auth_name,
					       &shim_lock_guid,
					       &attributes, &auth_size, auth);

		if (efi_status != EFI_SUCCESS ||
		    (auth_size != SHA256_DIGEST_SIZE &&
		     auth_size != PASSWORD_CRYPT_SIZE)) {
			if (MokX)
				console_error(L"Failed to get MokXAuth", efi_status);
			else
				console_error(L"Failed to get MokAuth", efi_status);
			return efi_status;
		}

		if (auth_size == PASSWORD_CRYPT_SIZE) {
			efi_status = match_password((PASSWORD_CRYPT *)auth,
						    NULL, 0, NULL, NULL);
		} else {
			efi_status = match_password(NULL, MokNew, MokNewSize,
						    auth, NULL);
		}
		if (efi_status != EFI_SUCCESS)
			return EFI_ACCESS_DENIED;
	}

	if (!MokNewSize) {
		/* Delete MOK */
		efi_status = uefi_call_wrapper(RT->SetVariable, 5, db_name,
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE
					       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       0, NULL);
	} else {
		/* Write new MOK */
		efi_status = write_db(db_name, MokNew, MokNewSize);
	}

	if (efi_status != EFI_SUCCESS) {
		console_error(L"Failed to set variable", efi_status);
		return efi_status;
	}

	return EFI_SUCCESS;
}

static EFI_STATUS mok_enrollment_prompt (void *MokNew, UINTN MokNewSize, int auth,
				    BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	CHAR16 *title;

	if (MokX)
		title = L"[Enroll MOKX]";
	else
		title = L"[Enroll MOK]";

	efi_status = list_keys(MokNew, MokNewSize, title);
	if (efi_status != EFI_SUCCESS)
		return efi_status;

	if (console_yes_no((CHAR16 *[]){L"Enroll the key(s)?", NULL}) == 0)
		return EFI_ABORTED;

	efi_status = store_keys(MokNew, MokNewSize, auth, MokX);

	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Failed to enroll keys\n");
		return efi_status;
	}

	if (auth) {
		if (MokX) {
			LibDeleteVariable(L"MokXNew", &shim_lock_guid);
			LibDeleteVariable(L"MokXAuth", &shim_lock_guid);
		} else {
			LibDeleteVariable(L"MokNew", &shim_lock_guid);
			LibDeleteVariable(L"MokAuth", &shim_lock_guid);
		}
	}

	if (MokNew)
		FreePool (MokNew);

	return EFI_SUCCESS;
}

static EFI_STATUS mok_reset_prompt (BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	CHAR16 *prompt;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	if (MokX)
		prompt = L"Erase all stored keys in MokListX?";
	else
		prompt = L"Erase all stored keys in MokList?";
	if (console_yes_no((CHAR16 *[]){prompt, NULL }) == 0)
		return EFI_ABORTED;

	efi_status = store_keys(NULL, 0, TRUE, MokX);

	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Failed to erase keys\n");
		return efi_status;
	}

	if (MokX) {
		LibDeleteVariable(L"MokXNew", &shim_lock_guid);
		LibDeleteVariable(L"MokXAuth", &shim_lock_guid);
	} else {
		LibDeleteVariable(L"MokNew", &shim_lock_guid);
		LibDeleteVariable(L"MokAuth", &shim_lock_guid);
	}

	return EFI_SUCCESS;
}

static EFI_STATUS write_back_mok_list (MokListNode *list, INTN key_num,
				       BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_GUID CertType = X509_GUID;
	EFI_STATUS efi_status;
	EFI_SIGNATURE_LIST *CertList;
	EFI_SIGNATURE_DATA *CertData;
	void *Data = NULL, *ptr;
	INTN DataSize = 0;
	int i;
	CHAR16 *db_name;

	if (MokX)
		db_name = L"MokListX";
	else
		db_name = L"MokList";

	for (i = 0; i < key_num; i++) {
		if (list[i].Mok == NULL)
			continue;

		DataSize += sizeof(EFI_SIGNATURE_LIST);
		if (CompareGuid(&(list[i].Type), &CertType) == 0)
			DataSize += sizeof(EFI_GUID);
		DataSize += list[i].MokSize;
	}

	Data = AllocatePool(DataSize);
	if (Data == NULL && DataSize != 0)
		return EFI_OUT_OF_RESOURCES;

	ptr = Data;

	for (i = 0; i < key_num; i++) {
		if (list[i].Mok == NULL)
			continue;

		CertList = (EFI_SIGNATURE_LIST *)ptr;
		CertData = (EFI_SIGNATURE_DATA *)(((uint8_t *)ptr) +
			   sizeof(EFI_SIGNATURE_LIST));

		CertList->SignatureType = list[i].Type;
		CertList->SignatureHeaderSize = 0;

		if (CompareGuid(&(list[i].Type), &CertType) == 0) {
			CertList->SignatureListSize = list[i].MokSize +
						      sizeof(EFI_SIGNATURE_LIST) +
						      sizeof(EFI_GUID);
			CertList->SignatureSize = list[i].MokSize + sizeof(EFI_GUID);

			CertData->SignatureOwner = shim_lock_guid;
			CopyMem(CertData->SignatureData, list[i].Mok, list[i].MokSize);
		} else {
			CertList->SignatureListSize = list[i].MokSize +
						      sizeof(EFI_SIGNATURE_LIST);
			CertList->SignatureSize = sha_size(list[i].Type) + sizeof(EFI_GUID);

			CopyMem(CertData, list[i].Mok, list[i].MokSize);
		}
		ptr = (uint8_t *)ptr + CertList->SignatureListSize;
	}

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, db_name,
				       &shim_lock_guid,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       DataSize, Data);
	if (Data)
		FreePool(Data);

	if (efi_status != EFI_SUCCESS) {
		console_error(L"Failed to set variable", efi_status);
		return efi_status;
	}

	return EFI_SUCCESS;
}

static void delete_cert (void *key, UINT32 key_size,
			 MokListNode *mok, INTN mok_num)
{
	EFI_GUID CertType = X509_GUID;
	int i;

	for (i = 0; i < mok_num; i++) {
		if (CompareGuid(&(mok[i].Type), &CertType) != 0)
			continue;

		if (mok[i].MokSize == key_size &&
		    CompareMem(key, mok[i].Mok, key_size) == 0) {
			/* Remove the key */
			mok[i].Mok = NULL;
			mok[i].MokSize = 0;
		}
	}
}

static int match_hash (UINT8 *hash, UINT32 hash_size, int start,
		       void *hash_list, UINT32 list_num)
{
	UINT8 *ptr;
	UINTN i;

	ptr = hash_list + sizeof(EFI_GUID);
	for (i = start; i < list_num; i++) {
		if (CompareMem(hash, ptr, hash_size) == 0)
			return i;
		ptr += hash_size + sizeof(EFI_GUID);
	}

	return -1;
}

static void mem_move (void *dest, void *src, UINTN size)
{
	UINT8 *d, *s;
	UINTN i;

	d = (UINT8 *)dest;
	s = (UINT8 *)src;
	for (i = 0; i < size; i++)
		d[i] = s[i];
}

static void delete_hash_in_list (EFI_GUID Type, UINT8 *hash, UINT32 hash_size,
				 MokListNode *mok, INTN mok_num)
{
	UINT32 sig_size;
	UINT32 list_num;
	int i, del_ind;
	void *start, *end;
	UINT32 remain;

	sig_size = hash_size + sizeof(EFI_GUID);

	for (i = 0; i < mok_num; i++) {
		if ((CompareGuid(&(mok[i].Type), &Type) != 0) ||
		    (mok[i].MokSize < sig_size))
			continue;

		list_num = mok[i].MokSize / sig_size;

		del_ind = match_hash(hash, hash_size, 0, mok[i].Mok,
				     list_num);
		while (del_ind >= 0) {
			/* Remove the hash */
			if (sig_size == mok[i].MokSize) {
				mok[i].Mok = NULL;
				mok[i].MokSize = 0;
				break;
			}

			start = mok[i].Mok + del_ind * sig_size;
			end = start + sig_size;
			remain = mok[i].MokSize - (del_ind + 1)*sig_size;

			mem_move(start, end, remain);
			mok[i].MokSize -= sig_size;
			list_num--;

			del_ind = match_hash(hash, hash_size, del_ind,
					     mok[i].Mok, list_num);
		}
	}
}

static void delete_hash_list (EFI_GUID Type, void *hash_list, UINT32 list_size,
			      MokListNode *mok, INTN mok_num)
{
	UINT32 hash_size;
	UINT32 hash_num;
	UINT32 sig_size;
	UINT8 *hash;
	UINT32 i;

	hash_size = sha_size (Type);
	sig_size = hash_size + sizeof(EFI_GUID);
	if (list_size < sig_size)
		return;

	hash_num = list_size / sig_size;

	hash = hash_list + sizeof(EFI_GUID);

	for (i = 0; i < hash_num; i++) {
		delete_hash_in_list (Type, hash, hash_size, mok, mok_num);
		hash += sig_size;
	}
}

static EFI_STATUS delete_keys (void *MokDel, UINTN MokDelSize, BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_GUID CertType = X509_GUID;
	EFI_STATUS efi_status;
	CHAR16 *db_name;
	CHAR16 *auth_name;
	CHAR16 *err_str1;
	CHAR16 *err_str2;
	UINT8 auth[PASSWORD_CRYPT_SIZE];
	UINTN auth_size = PASSWORD_CRYPT_SIZE;
	UINT32 attributes;
	UINT8 *MokListData = NULL;
	UINTN MokListDataSize = 0;
	MokListNode *mok, *del_key;
	INTN mok_num, del_num;
	int i;

	if (MokX) {
		db_name = L"MokListX";
		auth_name = L"MokXDelAuth";
	} else {
		db_name = L"MokList";
		auth_name = L"MokDelAuth";
	}

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, auth_name,
				       &shim_lock_guid,
				       &attributes, &auth_size, auth);

	if (efi_status != EFI_SUCCESS ||
	    (auth_size != SHA256_DIGEST_SIZE && auth_size != PASSWORD_CRYPT_SIZE)) {
		if (MokX)
			console_error(L"Failed to get MokXDelAuth", efi_status);
		else
			console_error(L"Failed to get MokDelAuth", efi_status);
		return efi_status;
	}

	if (auth_size == PASSWORD_CRYPT_SIZE) {
		efi_status = match_password((PASSWORD_CRYPT *)auth, NULL, 0,
					    NULL, NULL);
	} else {
		efi_status = match_password(NULL, MokDel, MokDelSize, auth, NULL);
	}
	if (efi_status != EFI_SUCCESS)
		return EFI_ACCESS_DENIED;

	efi_status = get_variable_attr (db_name, &MokListData, &MokListDataSize,
				        shim_lock_guid, &attributes);
	if (efi_status != EFI_SUCCESS) {
		if (MokX)
			console_errorbox(L"Failed to retrieve MokListX");
		else
			console_errorbox(L"Failed to retrieve MokList");
		return EFI_ABORTED;
	} else if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		if (MokX) {
			err_str1 = L"MokListX is compromised!";
			err_str2 = L"Erase all keys in MokListX!";
		} else {
			err_str1 = L"MokList is compromised!";
			err_str2 = L"Erase all keys in MokList!";
		}
		console_alertbox((CHAR16 *[]){err_str1, err_str2, NULL});
		uefi_call_wrapper(RT->SetVariable, 5, db_name,
				  &shim_lock_guid,
				  EFI_VARIABLE_NON_VOLATILE |
				  EFI_VARIABLE_BOOTSERVICE_ACCESS,
				  0, NULL);
		return EFI_ACCESS_DENIED;
	}

	/* Nothing to do */
	if (!MokListData || MokListDataSize == 0)
		return EFI_SUCCESS;

	/* Construct lists */
	mok_num = count_keys(MokListData, MokListDataSize);
	if (mok_num == 0) {
		if (MokX) {
			err_str1 = L"Failed to construct the key list of MokListX";
			err_str2 = L"Reset MokListX!";
		} else {
			err_str1 = L"Failed to construct the key list of MokList";
			err_str2 = L"Reset MokList!";
		}
		console_alertbox((CHAR16 *[]){err_str1, err_str2, NULL});
		uefi_call_wrapper(RT->SetVariable, 5, db_name,
				  &shim_lock_guid,
				  EFI_VARIABLE_NON_VOLATILE |
				  EFI_VARIABLE_BOOTSERVICE_ACCESS,
				  0, NULL);
		efi_status = EFI_ABORTED;
		goto error;
	}
	mok = build_mok_list(mok_num, MokListData, MokListDataSize);
	if (!mok) {
		console_errorbox(L"Failed to construct key list");
		efi_status = EFI_ABORTED;
		goto error;
	}
	del_num = count_keys(MokDel, MokDelSize);
	if (del_num == 0) {
		console_errorbox(L"Invalid key delete list");
		efi_status = EFI_ABORTED;
		goto error;
	}
	del_key = build_mok_list(del_num, MokDel, MokDelSize);
	if (!del_key) {
		console_errorbox(L"Failed to construct key list");
		efi_status = EFI_ABORTED;
		goto error;
	}

	/* Search and destroy */
	for (i = 0; i < del_num; i++) {
		if (CompareGuid(&(del_key[i].Type), &CertType) == 0) {
			delete_cert(del_key[i].Mok, del_key[i].MokSize,
				    mok, mok_num);
		} else if (is_sha2_hash(del_key[i].Type)) {
			delete_hash_list(del_key[i].Type, del_key[i].Mok,
					 del_key[i].MokSize, mok, mok_num);
		}
	}

	efi_status = write_back_mok_list(mok, mok_num, MokX);

error:
	if (MokListData)
		FreePool(MokListData);
	if (mok)
		FreePool(mok);
	if (del_key)
		FreePool(del_key);

	return efi_status;
}

static EFI_STATUS mok_deletion_prompt (void *MokDel, UINTN MokDelSize, BOOLEAN MokX)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	CHAR16 *title;

	if (MokX)
		title = L"[Delete MOKX]";
	else
		title = L"[Delete MOK]";

	efi_status = list_keys(MokDel, MokDelSize, title);
	if (efi_status != EFI_SUCCESS)
		return efi_status;

	if (console_yes_no((CHAR16 *[]){L"Delete the key(s)?", NULL}) == 0)
		return EFI_ABORTED;

	efi_status = delete_keys(MokDel, MokDelSize, MokX);

	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Failed to delete keys");
		return efi_status;
	}

	if (MokX) {
		LibDeleteVariable(L"MokXDel", &shim_lock_guid);
		LibDeleteVariable(L"MokXDelAuth", &shim_lock_guid);
	} else {
		LibDeleteVariable(L"MokDel", &shim_lock_guid);
		LibDeleteVariable(L"MokDelAuth", &shim_lock_guid);
	}

	if (MokDel)
		FreePool(MokDel);

	return EFI_SUCCESS;
}

static CHAR16 get_password_charater (CHAR16 *prompt)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	EFI_STATUS status;
	CHAR16 *message[2];
	CHAR16 character;
	UINTN length;
	UINT32 pw_length;

	if (!prompt)
		prompt = L"Password charater: ";

	console_save_and_set_mode(&SavedMode);

	message[0] = prompt;
	message[1] = NULL;
	length = StrLen(message[0]);
	console_print_box_at(message, -1, -length-4, -5, length+4, 3, 0, 1);
	status = get_line(&pw_length, &character, 1, 0);
	if (EFI_ERROR(status))
		character = 0;

	console_restore_mode(&SavedMode);

	return character;
}

static EFI_STATUS mok_sb_prompt (void *MokSB, UINTN MokSBSize) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	MokSBvar *var = MokSB;
	CHAR16 *message[4];
	CHAR16 pass1, pass2, pass3;
	CHAR16 *str;
	UINT8 fail_count = 0;
	UINT8 sbval = 1;
	UINT8 pos1, pos2, pos3;
	int ret;

	if (MokSBSize != sizeof(MokSBvar)) {
		console_notify(L"Invalid MokSB variable contents");
		return EFI_INVALID_PARAMETER;
	}

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	message[0] = L"Change Secure Boot state";
	message[1] = NULL;

	console_save_and_set_mode(&SavedMode);
	console_print_box_at(message, -1, 0, 0, -1, -1, 1, 1);
	console_restore_mode(&SavedMode);

	while (fail_count < 3) {
		RandomBytes (&pos1, sizeof(pos1));
		pos1 = (pos1 % var->PWLen);

		do {
			RandomBytes (&pos2, sizeof(pos2));
			pos2 = (pos2 % var->PWLen);
		} while (pos2 == pos1);

		do {
			RandomBytes (&pos3, sizeof(pos3));
			pos3 = (pos3 % var->PWLen) ;
		} while (pos3 == pos2 || pos3 == pos1);

		str = PoolPrint(L"Enter password character %d: ", pos1 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass1 = get_password_charater(str);
		FreePool(str);

		str = PoolPrint(L"Enter password character %d: ", pos2 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass2 = get_password_charater(str);
		FreePool(str);

		str = PoolPrint(L"Enter password character %d: ", pos3 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass3 = get_password_charater(str);
		FreePool(str);

		if (pass1 != var->Password[pos1] ||
		    pass2 != var->Password[pos2] ||
		    pass3 != var->Password[pos3]) {
			Print(L"Invalid character\n");
			fail_count++;
		} else {
			break;
		}
	}

	if (fail_count >= 3) {
		console_notify(L"Password limit reached");
		return EFI_ACCESS_DENIED;
	}

	if (var->MokSBState == 0)
		ret = console_yes_no((CHAR16 *[]){L"Disable Secure Boot", NULL});
	else
		ret = console_yes_no((CHAR16 *[]){L"Enable Secure Boot", NULL});

	if (ret == 0) {
		LibDeleteVariable(L"MokSB", &shim_lock_guid);
		return EFI_ABORTED;
	}

	if (var->MokSBState == 0) {
		efi_status = uefi_call_wrapper(RT->SetVariable,
					       5, L"MokSBState",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       1, &sbval);
		if (efi_status != EFI_SUCCESS) {
			console_notify(L"Failed to set Secure Boot state");
			return efi_status;
		}
	} else {
		efi_status = uefi_call_wrapper(RT->SetVariable,
					       5, L"MokSBState",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       0, NULL);
		if (efi_status != EFI_SUCCESS) {
			console_notify(L"Failed to delete Secure Boot state");
			return efi_status;
		}
	}

	if (MokSB)
		FreePool(MokSB);

	return EFI_SUCCESS;
}

static EFI_STATUS mok_db_prompt (void *MokDB, UINTN MokDBSize) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	MokDBvar *var = MokDB;
	CHAR16 *message[4];
	CHAR16 pass1, pass2, pass3;
	CHAR16 *str;
	UINT8 fail_count = 0;
	UINT8 dbval = 1;
	UINT8 pos1, pos2, pos3;
	int ret;

	if (MokDBSize != sizeof(MokDBvar)) {
		console_notify(L"Invalid MokDB variable contents");
		return EFI_INVALID_PARAMETER;
	}

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	message[0] = L"Change DB state";
	message[1] = NULL;

	console_save_and_set_mode(&SavedMode);
	console_print_box_at(message, -1, 0, 0, -1, -1, 1, 1);
	console_restore_mode(&SavedMode);

	while (fail_count < 3) {
		RandomBytes (&pos1, sizeof(pos1));
		pos1 = (pos1 % var->PWLen);

		do {
			RandomBytes (&pos2, sizeof(pos2));
			pos2 = (pos2 % var->PWLen);
		} while (pos2 == pos1);

		do {
			RandomBytes (&pos3, sizeof(pos3));
			pos3 = (pos3 % var->PWLen) ;
		} while (pos3 == pos2 || pos3 == pos1);

		str = PoolPrint(L"Enter password character %d: ", pos1 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass1 = get_password_charater(str);
		FreePool(str);

		str = PoolPrint(L"Enter password character %d: ", pos2 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass2 = get_password_charater(str);
		FreePool(str);

		str = PoolPrint(L"Enter password character %d: ", pos3 + 1);
		if (!str) {
			console_errorbox(L"Failed to allocate buffer");
			return EFI_OUT_OF_RESOURCES;
		}
		pass3 = get_password_charater(str);
		FreePool(str);

		if (pass1 != var->Password[pos1] ||
		    pass2 != var->Password[pos2] ||
		    pass3 != var->Password[pos3]) {
			Print(L"Invalid character\n");
			fail_count++;
		} else {
			break;
		}
	}

	if (fail_count >= 3) {
		console_notify(L"Password limit reached");
		return EFI_ACCESS_DENIED;
	}

	if (var->MokDBState == 0)
		ret = console_yes_no((CHAR16 *[]){L"Ignore DB certs/hashes", NULL});
	else
		ret = console_yes_no((CHAR16 *[]){L"Use DB certs/hashes", NULL});

	if (ret == 0) {
		LibDeleteVariable(L"MokDB", &shim_lock_guid);
		return EFI_ABORTED;
	}

	if (var->MokDBState == 0) {
		efi_status = uefi_call_wrapper(RT->SetVariable,
					       5, L"MokDBState",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       1, &dbval);
		if (efi_status != EFI_SUCCESS) {
			console_notify(L"Failed to set DB state");
			return efi_status;
		}
	} else {
		efi_status = uefi_call_wrapper(RT->SetVariable, 5,
					       L"MokDBState",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       0, NULL);
		if (efi_status != EFI_SUCCESS) {
			console_notify(L"Failed to delete DB state");
			return efi_status;
		}
	}

	if (MokDB)
		FreePool(MokDB);

	return EFI_SUCCESS;
}

static EFI_STATUS mok_pw_prompt (void *MokPW, UINTN MokPWSize) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT8 hash[PASSWORD_CRYPT_SIZE];
	UINT8 clear = 0;

	if (MokPWSize != SHA256_DIGEST_SIZE && MokPWSize != PASSWORD_CRYPT_SIZE) {
		console_notify(L"Invalid MokPW variable contents");
		return EFI_INVALID_PARAMETER;
	}

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	SetMem(hash, PASSWORD_CRYPT_SIZE, 0);

	if (MokPWSize == PASSWORD_CRYPT_SIZE) {
		if (CompareMem(MokPW, hash, PASSWORD_CRYPT_SIZE) == 0)
			clear = 1;
	} else {
		if (CompareMem(MokPW, hash, SHA256_DIGEST_SIZE) == 0)
			clear = 1;
	}

	if (clear) {
		if (console_yes_no((CHAR16 *[]){L"Clear MOK password?", NULL}) == 0)
			return EFI_ABORTED;

		uefi_call_wrapper(RT->SetVariable, 5, L"MokPWStore",
				  &shim_lock_guid,
				  EFI_VARIABLE_NON_VOLATILE
				  | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				  0, NULL);
		goto mokpw_done;
	}

	if (MokPWSize == PASSWORD_CRYPT_SIZE) {
		efi_status = match_password((PASSWORD_CRYPT *)MokPW, NULL, 0,
					    NULL, L"Confirm MOK passphrase: ");
	} else {
		efi_status = match_password(NULL, NULL, 0, MokPW,
					    L"Confirm MOK passphrase: ");
	}

	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Password limit reached");
		return efi_status;
	}

	if (console_yes_no((CHAR16 *[]){L"Set MOK password?", NULL}) == 0)
		return EFI_ABORTED;

	efi_status = uefi_call_wrapper(RT->SetVariable, 5,
				       L"MokPWStore",
				       &shim_lock_guid,
				       EFI_VARIABLE_NON_VOLATILE |
				       EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       MokPWSize, MokPW);
	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Failed to set MOK password");
		return efi_status;
	}

mokpw_done:
	LibDeleteVariable(L"MokPW", &shim_lock_guid);

	if (MokPW)
		FreePool(MokPW);

	return EFI_SUCCESS;
}

static BOOLEAN verify_certificate(UINT8 *cert, UINTN size)
{
	X509 *X509Cert;
	UINTN length;
	if (!cert || size < 0)
		return FALSE;

	/*
	 * A DER encoding x509 certificate starts with SEQUENCE(0x30),
	 * the number of length bytes, and the number of value bytes.
	 * The size of a x509 certificate is usually between 127 bytes
	 * and 64KB. For convenience, assume the number of value bytes
	 * is 2, i.e. the second byte is 0x82.
	 */
	if (cert[0] != 0x30 || cert[1] != 0x82) {
		console_notify(L"Not a DER encoding X509 certificate");
		return FALSE;
	}

	length = (cert[2]<<8 | cert[3]);
	if (length != (size - 4)) {
		console_notify(L"Invalid X509 certificate: Inconsistent size");
		return FALSE;
	}

	if (!(X509ConstructCertificate(cert, size, (UINT8 **) &X509Cert)) ||
	    X509Cert == NULL) {
		console_notify(L"Invalid X509 certificate");
		return FALSE;
	}

	X509_free(X509Cert);
	return TRUE;
}

static EFI_STATUS enroll_file (void *data, UINTN datasize, BOOLEAN hash)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_SIGNATURE_LIST *CertList;
	EFI_SIGNATURE_DATA *CertData;
	UINTN mokbuffersize;
	void *mokbuffer = NULL;

	if (hash) {
		UINT8 sha256[SHA256_DIGEST_SIZE];
		UINT8 sha1[SHA1_DIGEST_SIZE];
		SHIM_LOCK *shim_lock;
		EFI_GUID shim_guid = SHIM_LOCK_GUID;
		PE_COFF_LOADER_IMAGE_CONTEXT context;
	
		status = LibLocateProtocol(&shim_guid, (VOID **)&shim_lock);

		if (status != EFI_SUCCESS)
			goto out;

		mokbuffersize = sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_GUID) +
			SHA256_DIGEST_SIZE;

		mokbuffer = AllocatePool(mokbuffersize);

		if (!mokbuffer)
			goto out;

		status = shim_lock->Context(data, datasize, &context);

		if (status != EFI_SUCCESS)
			goto out;
	
		status = shim_lock->Hash(data, datasize, &context, sha256,
					 sha1);

		if (status != EFI_SUCCESS)
			goto out;

		CertList = mokbuffer;
		CertList->SignatureType = EFI_CERT_SHA256_GUID;
		CertList->SignatureSize = 16 + SHA256_DIGEST_SIZE;
		CertData = (EFI_SIGNATURE_DATA *)(((UINT8 *)mokbuffer) +
						  sizeof(EFI_SIGNATURE_LIST));
		CopyMem(CertData->SignatureData, sha256, SHA256_DIGEST_SIZE);
	} else {
		mokbuffersize = datasize + sizeof(EFI_SIGNATURE_LIST) +
			sizeof(EFI_GUID);
		mokbuffer = AllocatePool(mokbuffersize);

		if (!mokbuffer)
			goto out;

		CertList = mokbuffer;
		CertList->SignatureType = X509_GUID;
		CertList->SignatureSize = 16 + datasize;

		memcpy(mokbuffer + sizeof(EFI_SIGNATURE_LIST) + 16, data,
		       datasize);

		CertData = (EFI_SIGNATURE_DATA *)(((UINT8 *)mokbuffer) +
						  sizeof(EFI_SIGNATURE_LIST));
	}

	CertList->SignatureListSize = mokbuffersize;
	CertList->SignatureHeaderSize = 0;
	CertData->SignatureOwner = shim_lock_guid;

	if (!hash) {
		if (!verify_certificate(CertData->SignatureData, datasize))
			goto out;
	}

	status = mok_enrollment_prompt(mokbuffer, mokbuffersize, FALSE, FALSE);
out:
	if (mokbuffer)
		FreePool(mokbuffer);

	return status;
}

static EFI_STATUS mok_hash_enroll(void)
{
	EFI_STATUS efi_status;
        CHAR16 *file_name = NULL;
	EFI_HANDLE im = NULL;
	EFI_FILE *file = NULL;
	UINTN filesize;
	void *data;

	simple_file_selector(&im, (CHAR16 *[]){
      L"Select Binary",
      L"",
      L"The Selected Binary will have its hash Enrolled",
      L"This means it will Subsequently Boot with no prompting",
      L"Remember to make sure it is a genuine binary before Enroling its hash",
      NULL
	      }, L"\\", L"", &file_name);

	if (!file_name)
		return EFI_INVALID_PARAMETER;

	efi_status = simple_file_open(im, file_name, &file, EFI_FILE_MODE_READ);

	if (efi_status != EFI_SUCCESS) {
		console_error(L"Unable to open file", efi_status);
		return efi_status;
	}

	simple_file_read_all(file, &filesize, &data);
	simple_file_close(file);

	if (!filesize) {
		console_error(L"Unable to read file", efi_status);
		return EFI_BAD_BUFFER_SIZE;
	}

	efi_status = enroll_file(data, filesize, TRUE);

	if (efi_status != EFI_SUCCESS)
		console_error(L"Hash failed (did you select a valid EFI binary?)", efi_status);

	FreePool(data);

	return efi_status;
}

static CHAR16 *der_suffix[] = {
	L".cer",
	L".der",
	L".crt",
	NULL
};

static BOOLEAN check_der_suffix (CHAR16 *file_name)
{
	CHAR16 suffix[5];
	int i;

	if (!file_name || StrLen(file_name) <= 4)
		return FALSE;

	suffix[0] = '\0';
	StrCat(suffix, file_name + StrLen(file_name) - 4);

	StrLwr (suffix);
	for (i = 0; der_suffix[i] != NULL; i++) {
		if (StrCmp(suffix, der_suffix[i]) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

static EFI_STATUS mok_key_enroll(void)
{
	EFI_STATUS efi_status;
        CHAR16 *file_name = NULL;
	EFI_HANDLE im = NULL;
	EFI_FILE *file = NULL;
	UINTN filesize;
	void *data;

	simple_file_selector(&im, (CHAR16 *[]){
      L"Select Key",
      L"",
      L"The selected key will be enrolled into the MOK database",
      L"This means any binaries signed with it will be run without prompting",
      L"Remember to make sure it is a genuine key before Enroling it",
      NULL
	      }, L"\\", L"", &file_name);

	if (!file_name)
		return EFI_INVALID_PARAMETER;

	if (!check_der_suffix(file_name)) {
		console_alertbox((CHAR16 *[]){
			L"Unsupported Format",
			L"",
			L"Only DER encoded certificate (*.cer/der/crt) is supported",
			NULL});
		return EFI_UNSUPPORTED;
	}

	efi_status = simple_file_open(im, file_name, &file, EFI_FILE_MODE_READ);

	if (efi_status != EFI_SUCCESS) {
		console_error(L"Unable to open file", efi_status);
		return efi_status;
	}

	simple_file_read_all(file, &filesize, &data);
	simple_file_close(file);

	if (!filesize) {
		console_error(L"Unable to read file", efi_status);
		return EFI_BAD_BUFFER_SIZE;
	}

	efi_status = enroll_file(data, filesize, FALSE);
	FreePool(data);

	return efi_status;
}

static BOOLEAN verify_pw(BOOLEAN *protected)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	UINT8 pwhash[PASSWORD_CRYPT_SIZE];
	UINTN size = PASSWORD_CRYPT_SIZE;
	UINT32 attributes;
	CHAR16 *message[2];

	*protected = FALSE;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokPWStore",
				       &shim_lock_guid, &attributes, &size,
				       pwhash);

	/*
	 * If anything can attack the password it could just set it to a
	 * known value, so there's no safety advantage in failing to validate
	 * purely because of a failure to read the variable
	 */
	if (efi_status != EFI_SUCCESS ||
	    (size != SHA256_DIGEST_SIZE && size != PASSWORD_CRYPT_SIZE))
		return TRUE;

	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS)
		return TRUE;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	/* Draw the background */
	console_save_and_set_mode(&SavedMode);
	message[0] = PoolPrint (L"%s UEFI key management", SHIM_VENDOR);
	message[1] = NULL;
	console_print_box_at(message, -1, 0, 0, -1, -1, 1, 1);
	FreePool(message[0]);
	console_restore_mode(&SavedMode);

	if (size == PASSWORD_CRYPT_SIZE) {
		efi_status = match_password((PASSWORD_CRYPT *)pwhash, NULL, 0,
					    NULL, L"Enter MOK password:");
	} else {
		efi_status = match_password(NULL, NULL, 0, pwhash,
					    L"Enter MOK password:");
	}
	if (efi_status != EFI_SUCCESS) {
		console_notify(L"Password limit reached");
		return FALSE;
	}

	*protected = TRUE;

	return TRUE;
}

static int draw_countdown()
{
	SIMPLE_TEXT_OUTPUT_MODE SavedMode;
	EFI_INPUT_KEY key;
	EFI_STATUS status;
	UINTN cols, rows;
	CHAR16 *title[2];
	CHAR16 *message =  L"Press any key to perform MOK management";
	int timeout = 10, wait = 10000000;

	console_save_and_set_mode (&SavedMode);

	title[0] = PoolPrint (L"%s UEFI key management", SHIM_VENDOR);
	title[1] = NULL;

	console_print_box_at(title, -1, 0, 0, -1, -1, 1, 1);

	uefi_call_wrapper(ST->ConOut->QueryMode, 4, ST->ConOut,
			  ST->ConOut->Mode->Mode, &cols, &rows);

	PrintAt((cols - StrLen(message))/2, rows/2, message);
	while (1) {
		if (timeout > 1)
			PrintAt(2, rows - 3, L"Booting in %d seconds  ", timeout);
		else if (timeout)
			PrintAt(2, rows - 3, L"Booting in %d second   ", timeout);

		status = WaitForSingleEvent(ST->ConIn->WaitForKey, wait);

		if (status != EFI_TIMEOUT) {
			/* Clear the key in the queue */
			uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2,
					  ST->ConIn, &key);
			break;
		}

		timeout--;
		if (!timeout)
			break;
	}

	FreePool(title[0]);

	console_restore_mode(&SavedMode);

	return timeout;
}

typedef enum {
	MOK_BOOT,
	MOK_RESET_MOK,
	MOK_RESET_MOKX,
	MOK_ENROLL_MOK,
	MOK_ENROLL_MOKX,
	MOK_DELETE_MOK,
	MOK_DELETE_MOKX,
	MOK_CHANGE_SB,
	MOK_SET_PW,
	MOK_CHANGE_DB,
	MOK_KEY_ENROLL,
	MOK_HASH_ENROLL
} mok_menu_item;

static void free_menu(mok_menu_item *menu_item, CHAR16 **menu_strings)
{
	if (menu_strings)
		FreePool(menu_strings);

	if (menu_item)
		FreePool(menu_item);
}

static EFI_STATUS enter_mok_menu(EFI_HANDLE image_handle,
				 void *MokNew, UINTN MokNewSize,
				 void *MokDel, UINTN MokDelSize,
				 void *MokSB, UINTN MokSBSize,
				 void *MokPW, UINTN MokPWSize,
				 void *MokDB, UINTN MokDBSize,
				 void *MokXNew, UINTN MokXNewSize,
				 void *MokXDel, UINTN MokXDelSize)
{
	CHAR16 **menu_strings;
	mok_menu_item *menu_item;
	int choice = 0;
	int mok_changed = 0;
	EFI_STATUS efi_status;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINT8 auth[PASSWORD_CRYPT_SIZE];
	UINTN auth_size = PASSWORD_CRYPT_SIZE;
	UINT32 attributes;
	BOOLEAN protected;
	EFI_STATUS ret = EFI_SUCCESS;

	if (verify_pw(&protected) == FALSE)
		return EFI_ACCESS_DENIED;
	
	if (protected == FALSE && draw_countdown() == 0)
		goto out;

	while (choice >= 0) {
		UINTN menucount = 3, i = 0;
		UINT32 MokAuth = 0;
		UINT32 MokDelAuth = 0;
		UINT32 MokXAuth = 0;
		UINT32 MokXDelAuth = 0;

		efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokAuth",
						       &shim_lock_guid,
						       &attributes, &auth_size, auth);

		if ((efi_status == EFI_SUCCESS) &&
		    (auth_size == SHA256_DIGEST_SIZE || auth_size == PASSWORD_CRYPT_SIZE))
			MokAuth = 1;

		efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokDelAuth",
						       &shim_lock_guid,
						       &attributes, &auth_size, auth);

		if ((efi_status == EFI_SUCCESS) &&
		   (auth_size == SHA256_DIGEST_SIZE || auth_size == PASSWORD_CRYPT_SIZE))
			MokDelAuth = 1;

		efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokXAuth",
						       &shim_lock_guid,
						       &attributes, &auth_size, auth);

		if ((efi_status == EFI_SUCCESS) &&
		    (auth_size == SHA256_DIGEST_SIZE || auth_size == PASSWORD_CRYPT_SIZE))
			MokXAuth = 1;

		efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokXDelAuth",
						       &shim_lock_guid,
						       &attributes, &auth_size, auth);

		if ((efi_status == EFI_SUCCESS) &&
		   (auth_size == SHA256_DIGEST_SIZE || auth_size == PASSWORD_CRYPT_SIZE))
			MokXDelAuth = 1;

		if (MokNew || MokAuth)
			menucount++;

		if (MokDel || MokDelAuth)
			menucount++;

		if (MokXNew || MokXAuth)
			menucount++;

		if (MokXDel || MokXDelAuth)
			menucount++;

		if (MokSB)
			menucount++;

		if (MokPW)
			menucount++;

		if (MokDB)
			menucount++;

		menu_strings = AllocateZeroPool(sizeof(CHAR16 *) * (menucount + 1));

		if (!menu_strings)
			return EFI_OUT_OF_RESOURCES;

		menu_item = AllocateZeroPool(sizeof(mok_menu_item) * menucount);

		if (!menu_item) {
			FreePool(menu_strings);
			return EFI_OUT_OF_RESOURCES;
		}

		if (mok_changed)
			menu_strings[i] = L"Reboot";
		else
			menu_strings[i] = L"Continue boot";
		menu_item[i] = MOK_BOOT;

		i++;

		if (MokNew || MokAuth) {
			if (!MokNew) {
				menu_strings[i] = L"Reset MOK";
				menu_item[i] = MOK_RESET_MOK;
			} else {
				menu_strings[i] = L"Enroll MOK";
				menu_item[i] = MOK_ENROLL_MOK;
			}
			i++;
		}

		if (MokDel || MokDelAuth) {
			menu_strings[i] = L"Delete MOK";
			menu_item[i] = MOK_DELETE_MOK;
			i++;
		}

		if (MokXNew || MokXAuth) {
			if (!MokXNew) {
				menu_strings[i] = L"Reset MOKX";
				menu_item[i] = MOK_RESET_MOKX;
			} else {
				menu_strings[i] = L"Enroll MOKX";
				menu_item[i] = MOK_ENROLL_MOKX;
			}
			i++;
		}

		if (MokXDel || MokXDelAuth) {
			menu_strings[i] = L"Delete MOKX";
			menu_item[i] = MOK_DELETE_MOKX;
			i++;
		}

		if (MokSB) {
			menu_strings[i] = L"Change Secure Boot state";
			menu_item[i] = MOK_CHANGE_SB;
			i++;
		}

		if (MokPW) {
			menu_strings[i] = L"Set MOK password";
			menu_item[i] = MOK_SET_PW;
			i++;
		}

		if (MokDB) {
			menu_strings[i] = L"Change DB state";
			menu_item[i] = MOK_CHANGE_DB;
			i++;
		}

		menu_strings[i] = L"Enroll key from disk";
		menu_item[i] = MOK_KEY_ENROLL;
		i++;

		menu_strings[i] = L"Enroll hash from disk";
		menu_item[i] = MOK_HASH_ENROLL;
		i++;

		menu_strings[i] = NULL;

		choice = console_select((CHAR16 *[]){ L"Perform MOK management", NULL },
					menu_strings, 0);

		if (choice < 0)
			goto out;

		switch (menu_item[choice]) {
		case MOK_BOOT:
			goto out;
		case MOK_RESET_MOK:
			efi_status = mok_reset_prompt(FALSE);
			break;
		case MOK_ENROLL_MOK:
			efi_status = mok_enrollment_prompt(MokNew, MokNewSize, TRUE, FALSE);
			if (efi_status == EFI_SUCCESS)
				MokNew = NULL;
			break;
		case MOK_DELETE_MOK:
			efi_status = mok_deletion_prompt(MokDel, MokDelSize, FALSE);
			if (efi_status == EFI_SUCCESS)
				MokDel = NULL;
			break;
		case MOK_RESET_MOKX:
			efi_status = mok_reset_prompt(TRUE);
			break;
		case MOK_ENROLL_MOKX:
			efi_status = mok_enrollment_prompt(MokXNew, MokXNewSize, TRUE, TRUE);
			if (efi_status == EFI_SUCCESS)
				MokXNew = NULL;
			break;
		case MOK_DELETE_MOKX:
			efi_status = mok_deletion_prompt(MokXDel, MokXDelSize, TRUE);
			if (efi_status == EFI_SUCCESS)
				MokXDel = NULL;
			break;
		case MOK_CHANGE_SB:
			efi_status = mok_sb_prompt(MokSB, MokSBSize);
			if (efi_status == EFI_SUCCESS)
				MokSB = NULL;
			break;
		case MOK_SET_PW:
			efi_status = mok_pw_prompt(MokPW, MokPWSize);
			if (efi_status == EFI_SUCCESS)
				MokPW = NULL;
			break;
		case MOK_CHANGE_DB:
			efi_status = mok_db_prompt(MokDB, MokDBSize);
			if (efi_status == EFI_SUCCESS)
				MokDB = NULL;
			break;
		case MOK_KEY_ENROLL:
			efi_status = mok_key_enroll();
			break;
		case MOK_HASH_ENROLL:
			efi_status = mok_hash_enroll();
			break;
		}

		if (efi_status == EFI_SUCCESS)
			mok_changed = 1;

		free_menu(menu_item, menu_strings);
	}

out:
	free_menu(menu_item, menu_strings);

	if (mok_changed)
		return reset_system();

	console_reset();

	return ret;
}

static EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINTN MokNewSize = 0, MokDelSize = 0, MokSBSize = 0, MokPWSize = 0;
	UINTN MokDBSize = 0, MokXNewSize = 0, MokXDelSize = 0;
	void *MokNew = NULL;
	void *MokDel = NULL;
	void *MokSB = NULL;
	void *MokPW = NULL;
	void *MokDB = NULL;
	void *MokXNew = NULL;
	void *MokXDel = NULL;
	EFI_STATUS status;

	status = get_variable(L"MokNew", (UINT8 **)&MokNew, &MokNewSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokNew", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokNew");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokNew", status);
	}

	status = get_variable(L"MokDel", (UINT8 **)&MokDel, &MokDelSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokDel", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokDel");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokDel", status);
	}

	status = get_variable(L"MokSB", (UINT8 **)&MokSB, &MokSBSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokSB", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokSB");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokSB", status);
	}

	status = get_variable(L"MokPW", (UINT8 **)&MokPW, &MokPWSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokPW", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokPW");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokPW", status);
	}

	status = get_variable(L"MokDB", (UINT8 **)&MokDB, &MokDBSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokDB", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokDB");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokDB", status);
	}

	status = get_variable(L"MokXNew", (UINT8 **)&MokXNew, &MokXNewSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokXNew", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokXNew");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokXNew", status);
	}

	status = get_variable(L"MokXDel", (UINT8 **)&MokXDel, &MokXDelSize,
				shim_lock_guid);
	if (status == EFI_SUCCESS) {
		if (LibDeleteVariable(L"MokXDel", &shim_lock_guid) != EFI_SUCCESS) {
			console_notify(L"Failed to delete MokXDel");
		}
	} else if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		console_error(L"Could not retrieve MokXDel", status);
	}

	enter_mok_menu(image_handle, MokNew, MokNewSize, MokDel, MokDelSize,
		       MokSB, MokSBSize, MokPW, MokPWSize, MokDB, MokDBSize,
		       MokXNew, MokXNewSize, MokXDel, MokXDelSize);

	if (MokNew)
		FreePool (MokNew);

	if (MokDel)
		FreePool (MokDel);

	if (MokSB)
		FreePool (MokSB);

	if (MokPW)
		FreePool (MokPW);

	if (MokDB)
		FreePool (MokDB);

	if (MokXNew)
		FreePool (MokXNew);

	if (MokXDel)
		FreePool (MokXDel);

	LibDeleteVariable(L"MokAuth", &shim_lock_guid);
	LibDeleteVariable(L"MokDelAuth", &shim_lock_guid);
	LibDeleteVariable(L"MokXAuth", &shim_lock_guid);
	LibDeleteVariable(L"MokXDelAuth", &shim_lock_guid);

	return EFI_SUCCESS;
}

static EFI_STATUS setup_rand (void)
{
	EFI_TIME time;
	EFI_STATUS efi_status;
	UINT64 seed;
	BOOLEAN status;

	efi_status = uefi_call_wrapper(RT->GetTime, 2, &time, NULL);

	if (efi_status != EFI_SUCCESS)
		return efi_status;

	seed = ((UINT64)time.Year << 48) | ((UINT64)time.Month << 40) |
		((UINT64)time.Day << 32) | ((UINT64)time.Hour << 24) |
		((UINT64)time.Minute << 16) | ((UINT64)time.Second << 8) |
		((UINT64)time.Daylight);

	status = RandomSeed((UINT8 *)&seed, sizeof(seed));

	if (!status)
		return EFI_ABORTED;

	return EFI_SUCCESS;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS efi_status;

	InitializeLib(image_handle, systab);

	setup_console(1);

	setup_rand();

	efi_status = check_mok_request(image_handle);

	setup_console(0);
	return efi_status;
}
