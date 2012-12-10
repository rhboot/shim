#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include <openssl/x509.h>
#include "shim.h"
#include "signature.h"
#include "PeImage.h"

#define PASSWORD_MAX 16
#define PASSWORD_MIN 8
#define SB_PASSWORD_LEN 8

#ifndef SHIM_VENDOR
#define SHIM_VENDOR L"Shim"
#endif

#define EFI_VARIABLE_APPEND_WRITE 0x00000040

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
} __attribute__ ((packed)) MokListNode;

typedef struct {
	UINT32 MokSBState;
	UINT32 PWLen;
	CHAR16 Password[PASSWORD_MAX];
} __attribute__ ((packed)) MokSBvar;

static EFI_INPUT_KEY get_keystroke (void)
{
	EFI_INPUT_KEY key;
	UINTN EventIndex;

	uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey,
			  &EventIndex);
	uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);

	return key;
}

static EFI_STATUS get_sha1sum (void *Data, int DataSize, UINT8 *hash)
{
	EFI_STATUS status;
	unsigned int ctxsize;
	void *ctx = NULL;

	ctxsize = Sha1GetContextSize();
	ctx = AllocatePool(ctxsize);

	if (!ctx) {
		Print(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha1Init(ctx)) {
		Print(L"Unable to initialise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha1Update(ctx, Data, DataSize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha1Final(ctx, hash))) {
		Print(L"Unable to finalise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	status = EFI_SUCCESS;
done:
	return status;
}

static MokListNode *build_mok_list(UINT32 num, void *Data, UINTN DataSize) {
	MokListNode *list;
	EFI_SIGNATURE_LIST *CertList = Data;
	EFI_SIGNATURE_DATA *Cert;
	EFI_GUID CertType = EfiCertX509Guid;
	EFI_GUID HashType = EfiHashSha256Guid;
	UINTN dbsize = DataSize;
	UINTN count = 0;

	list = AllocatePool(sizeof(MokListNode) * num);

	if (!list) {
		Print(L"Unable to allocate MOK list\n");
		return NULL;
	}

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if ((CompareGuid (&CertList->SignatureType, &CertType) != 0) &&
		    (CompareGuid (&CertList->SignatureType, &HashType) != 0)) {
			dbsize -= CertList->SignatureListSize;
			CertList = (EFI_SIGNATURE_LIST *)((UINT8 *) CertList +
						  CertList->SignatureListSize);
			continue;
		}

		if ((CompareGuid (&CertList->SignatureType, &HashType) == 0) &&
		    (CertList->SignatureSize != 48)) {
			dbsize -= CertList->SignatureListSize;
			CertList = (EFI_SIGNATURE_LIST *)((UINT8 *) CertList +
						  CertList->SignatureListSize);
			continue;
		}

		Cert = (EFI_SIGNATURE_DATA *) (((UINT8 *) CertList) +
		  sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);

		list[count].MokSize = CertList->SignatureSize;
		list[count].Mok = (void *)Cert->SignatureData;

		count++;
		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
	}

	return list;
}

static void print_x509_name (X509_NAME *X509Name, CHAR16 *name)
{
	char *str;

	str = X509_NAME_oneline(X509Name, NULL, 0);
	if (str) {
		Print(L"  %s:\n    %a\n", name, str);
		OPENSSL_free(str);
	}
}

static const char *mon[12]= {
"Jan","Feb","Mar","Apr","May","Jun",
"Jul","Aug","Sep","Oct","Nov","Dec"
};

static void print_x509_GENERALIZEDTIME_time (ASN1_TIME *time, CHAR16 *time_string)
{
	char *v;
	int gmt = 0;
	int i;
	int y = 0,M = 0,d = 0,h = 0,m = 0,s = 0;
	char *f = NULL;
	int f_len = 0;

	i=time->length;
	v=(char *)time->data;

	if (i < 12)
		goto error;

	if (v[i-1] == 'Z')
		gmt=1;

	for (i=0; i<12; i++) {
		if ((v[i] > '9') || (v[i] < '0'))
			goto error;
	}

	y = (v[0]-'0')*1000+(v[1]-'0')*100 + (v[2]-'0')*10+(v[3]-'0');
	M = (v[4]-'0')*10+(v[5]-'0');

	if ((M > 12) || (M < 1))
		goto error;

	d = (v[6]-'0')*10+(v[7]-'0');
	h = (v[8]-'0')*10+(v[9]-'0');
	m = (v[10]-'0')*10+(v[11]-'0');

	if (time->length >= 14 &&
	    (v[12] >= '0') && (v[12] <= '9') &&
	    (v[13] >= '0') && (v[13] <= '9')) {
		s = (v[12]-'0')*10+(v[13]-'0');
		/* Check for fractions of seconds. */
		if (time->length >= 15 && v[14] == '.')	{
			int l = time->length;
			f = &v[14];	/* The decimal point. */
			f_len = 1;
			while (14 + f_len < l && f[f_len] >= '0' &&
			       f[f_len] <= '9')
				++f_len;
		}
	}

	SPrint(time_string, 0, L"%a %2d %02d:%02d:%02d%.*a %d%a",
	       mon[M-1], d, h, m, s, f_len, f, y, (gmt)?" GMT":"");
error:
	return;
}

static void print_x509_UTCTIME_time (ASN1_TIME *time, CHAR16 *time_string)
{
	char *v;
	int gmt=0;
	int i;
	int y = 0,M = 0,d = 0,h = 0,m = 0,s = 0;

	i=time->length;
	v=(char *)time->data;

	if (i < 10)
		goto error;

	if (v[i-1] == 'Z')
		gmt=1;

	for (i=0; i<10; i++)
		if ((v[i] > '9') || (v[i] < '0'))
			goto error;

	y = (v[0]-'0')*10+(v[1]-'0');

	if (y < 50)
		y+=100;

	M = (v[2]-'0')*10+(v[3]-'0');

	if ((M > 12) || (M < 1))
		goto error;

	d = (v[4]-'0')*10+(v[5]-'0');
	h = (v[6]-'0')*10+(v[7]-'0');
	m = (v[8]-'0')*10+(v[9]-'0');

	if (time->length >=12 &&
	    (v[10] >= '0') && (v[10] <= '9') &&
	    (v[11] >= '0') && (v[11] <= '9'))
		s = (v[10]-'0')*10+(v[11]-'0');

	SPrint(time_string, 0, L"%a %2d %02d:%02d:%02d %d%a",
	       mon[M-1], d, h, m, s, y+1900, (gmt)?" GMT":"");
error:
	return;
}

static void print_x509_time (ASN1_TIME *time, CHAR16 *name)
{
	CHAR16 time_string[30];

	if (time->type == V_ASN1_UTCTIME) {
		print_x509_UTCTIME_time(time, time_string);
	} else if (time->type == V_ASN1_GENERALIZEDTIME) {
		print_x509_GENERALIZEDTIME_time(time, time_string);
	} else {
		time_string[0] = '\0';
	}

	Print(L"  %s:\n    %s\n", name, time_string);
}

static void show_x509_info (X509 *X509Cert)
{
	ASN1_INTEGER *serial;
	BIGNUM *bnser;
	unsigned char hexbuf[30];
	X509_NAME *X509Name;
	ASN1_TIME *time;

	serial = X509_get_serialNumber(X509Cert);
	if (serial) {
		int i, n;
		bnser = ASN1_INTEGER_to_BN(serial, NULL);
		n = BN_bn2bin(bnser, hexbuf);
		Print(L"  Serial Number:\n    ");
		for (i = 0; i < n-1; i++) {
			Print(L"%02x:", hexbuf[i]);
		}
		Print(L"%02x\n", hexbuf[n-1]);
	}

	X509Name = X509_get_issuer_name(X509Cert);
	if (X509Name) {
		print_x509_name(X509Name, L"Issuer");
	}

	X509Name = X509_get_subject_name(X509Cert);
	if (X509Name) {
		print_x509_name(X509Name, L"Subject");
	}

	time = X509_get_notBefore(X509Cert);
	if (time) {
		print_x509_time(time, L"Validity from");
	}

	time = X509_get_notAfter(X509Cert);
	if (time) {
		print_x509_time(time, L"Validity till");
	}
}

static void show_mok_info (void *Mok, UINTN MokSize)
{
	EFI_STATUS efi_status;
	UINT8 hash[SHA1_DIGEST_SIZE];
	unsigned int i;
	X509 *X509Cert;

	if (!Mok || MokSize == 0)
		return;

	if (MokSize != 48) {
		if (X509ConstructCertificate(Mok, MokSize,
				 (UINT8 **) &X509Cert) && X509Cert != NULL) {
			show_x509_info(X509Cert);
			X509_free(X509Cert);
		} else {
			Print(L"  Not a valid X509 certificate: %x\n\n",
			      ((UINT32 *)Mok)[0]);
			return;
		}

		efi_status = get_sha1sum(Mok, MokSize, hash);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Failed to compute MOK fingerprint\n");
			return;
		}

		Print(L"  Fingerprint (SHA1):\n    ");
		for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
			Print(L" %02x", hash[i]);
			if (i % 10 == 9)
				Print(L"\n    ");
		}
	} else {
		Print(L"SHA256 hash:\n   ");
		for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
			Print(L" %02x", ((UINT8 *)Mok)[i]);
			if (i % 10 == 9)
				Print(L"\n   ");
		}
		Print(L"\n");
	}

	Print(L"\n");
}

static INTN get_number ()
{
	EFI_INPUT_KEY input_key;
	CHAR16 input[10];
	int count = 0;

	do {
		input_key = get_keystroke();

		if ((input_key.UnicodeChar < '0' ||
		     input_key.UnicodeChar > '9' ||
		     count >= 10) &&
		    input_key.UnicodeChar != CHAR_BACKSPACE) {
			continue;
		}

		if (count == 0 && input_key.UnicodeChar == CHAR_BACKSPACE)
			continue;

		Print(L"%c", input_key.UnicodeChar);

		if (input_key.UnicodeChar == CHAR_BACKSPACE) {
			input[--count] = '\0';
			continue;
		}

		input[count++] = input_key.UnicodeChar;
	} while (input_key.UnicodeChar != CHAR_CARRIAGE_RETURN);

	if (count == 0)
		return -1;

	input[count] = '\0';

	return (INTN)Atoi(input);
}

static UINT8 list_keys (void *MokNew, UINTN MokNewSize)
{
	UINT32 MokNum = 0;
	MokListNode *keys = NULL;
	INTN key_num = 0;
	UINT8 initial = 1;
	EFI_SIGNATURE_LIST *CertList = MokNew;
	EFI_GUID CertType = EfiCertX509Guid;
	EFI_GUID HashType = EfiHashSha256Guid;
	UINTN dbsize = MokNewSize;

	if (MokNewSize < (sizeof(EFI_SIGNATURE_LIST) +
			  sizeof(EFI_SIGNATURE_DATA))) {
		Print(L"No keys\n");
		Pause();
		return 0;
	}

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if ((CompareGuid (&CertList->SignatureType, &CertType) != 0) &&
		    (CompareGuid (&CertList->SignatureType, &HashType) != 0)) {
			Print(L"Doesn't look like a key or hash\n");
			dbsize -= CertList->SignatureListSize;
			CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
			continue;
		}

		if ((CompareGuid (&CertList->SignatureType, &CertType) != 0) &&
		    (CertList->SignatureSize != 48)) {
			Print(L"Doesn't look like a valid hash\n");
			dbsize -= CertList->SignatureListSize;
			CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
			continue;
		}

		MokNum++;
		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList +
						  CertList->SignatureListSize);
	}

	keys = build_mok_list(MokNum, MokNew, MokNewSize);

	if (!keys) {
		Print(L"Failed to construct key list in MokNew\n");
		return 0;
	}

	do {
		uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
		Print(L"Input the key number to show the details of the key or\n"
		      L"type \'0\' to continue\n\n");
		Print(L"%d key(s) in the new key list\n\n", MokNum);

		if (key_num > MokNum) {
			Print(L"[Key %d]\n", key_num);
			Print(L"No such key\n\n");
		} else if (initial != 1 && key_num > 0){
			Print(L"[Key %d]\n", key_num);
			show_mok_info(keys[key_num-1].Mok, keys[key_num-1].MokSize);
		}

		Print(L"Key Number: ");

		key_num = get_number();

		Print(L"\n\n");

		if (key_num == -1)
			continue;

		initial = 0;
	} while (key_num != 0);

	FreePool(keys);

	return 1;
}

static UINT8 get_line (UINT32 *length, CHAR16 *line, UINT32 line_max, UINT8 show)
{
	EFI_INPUT_KEY key;
	int count = 0;

	do {
		key = get_keystroke();

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

	return 1;
}

static EFI_STATUS compute_pw_hash (void *MokNew, UINTN MokNewSize, CHAR16 *password,
			     UINT32 pw_length, UINT8 *hash)
{
	EFI_STATUS status;
	unsigned int ctxsize;
	void *ctx = NULL;

	ctxsize = Sha256GetContextSize();
	ctx = AllocatePool(ctxsize);

	if (!ctx) {
		Print(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(ctx)) {
		Print(L"Unable to initialise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (MokNew && MokNewSize) {
		if (!(Sha256Update(ctx, MokNew, MokNewSize))) {
			Print(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
	}

	if (!(Sha256Update(ctx, password, pw_length * sizeof(CHAR16)))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!(Sha256Final(ctx, hash))) {
		Print(L"Unable to finalise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	status = EFI_SUCCESS;
done:
	return status;
}

static EFI_STATUS store_keys (void *MokNew, UINTN MokNewSize, int authenticate)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT8 hash[SHA256_DIGEST_SIZE];
	UINT8 auth[SHA256_DIGEST_SIZE];
	UINTN auth_size;
	UINT32 attributes;
	CHAR16 password[PASSWORD_MAX];
	UINT32 pw_length;
	UINT8 fail_count = 0;

	if (authenticate) {
		auth_size = SHA256_DIGEST_SIZE;
		efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokAuth",
					       &shim_lock_guid,
					       &attributes, &auth_size, auth);


		if (efi_status != EFI_SUCCESS || auth_size != SHA256_DIGEST_SIZE) {
			Print(L"Failed to get MokAuth %d\n", efi_status);
			return efi_status;
		}

		while (fail_count < 3) {
			Print(L"Password(%d-%d characters): ",
			      PASSWORD_MIN, PASSWORD_MAX);
			get_line(&pw_length, password, PASSWORD_MAX, 0);

			if (pw_length < 8) {
				Print(L"At least %d characters for the password\n",
				      PASSWORD_MIN);
			}

			efi_status = compute_pw_hash(MokNew, MokNewSize, password,
						     pw_length, hash);

			if (efi_status != EFI_SUCCESS) {
				return efi_status;
			}

			if (CompareMem(auth, hash, SHA256_DIGEST_SIZE) != 0) {
				Print(L"Password doesn't match\n");
				fail_count++;
			} else {
				break;
			}
		}

		if (fail_count >= 3)
			return EFI_ACCESS_DENIED;
	}

	if (!MokNewSize) {
		/* Delete MOK */
		efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokList",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE
					       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
					       0, NULL);
	} else {
		/* Write new MOK */
		efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokList",
					       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE
					       | EFI_VARIABLE_BOOTSERVICE_ACCESS
					       | EFI_VARIABLE_APPEND_WRITE,
					       MokNewSize, MokNew);
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set variable %d\n", efi_status);
		return efi_status;
	}

	return EFI_SUCCESS;
}

static UINTN mok_enrollment_prompt (void *MokNew, UINTN MokNewSize, int auth) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	CHAR16 line[1];
	UINT32 length;
	EFI_STATUS efi_status;

	do {
		if (!list_keys(MokNew, MokNewSize)) {
			return 0;
		}

		Print(L"Enroll the key(s)? (y/n): ");

		get_line (&length, line, 1, 1);

		if (line[0] == 'Y' || line[0] == 'y') {
			efi_status = store_keys(MokNew, MokNewSize, auth);

			if (efi_status != EFI_SUCCESS) {
				Print(L"Failed to enroll keys\n");
				return -1;
			}

			if (auth) {
				LibDeleteVariable(L"MokNew", &shim_lock_guid);
				LibDeleteVariable(L"MokAuth", &shim_lock_guid);

				Print(L"\nPress a key to reboot system\n");
				Pause();
				uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm,
						  EFI_SUCCESS, 0, NULL);
				Print(L"Failed to reboot\n");
				return -1;
			}

			return 0;
		}
	} while (line[0] != 'N' && line[0] != 'n');
	return -1;
}

static INTN mok_enrollment_prompt_callback (void *MokNew, void *data2,
					    void *data3)
{
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
	return mok_enrollment_prompt(MokNew, (UINTN)data2, TRUE);
}

static INTN mok_deletion_prompt (void *MokNew, void *data2, void *data3) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	CHAR16 line[1];
	UINT32 length;
	EFI_STATUS efi_status;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
	Print(L"Erase all stored keys? (y/N): ");

	get_line (&length, line, 1, 1);

	if (line[0] == 'Y' || line[0] == 'y') {
		efi_status = store_keys(NULL, 0, TRUE);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Failed to erase keys\n");
			return -1;
		}

		LibDeleteVariable(L"MokNew", &shim_lock_guid);
		LibDeleteVariable(L"MokAuth", &shim_lock_guid);

		Print(L"\nPress a key to reboot system\n");
		Pause();
		uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm,
				  EFI_SUCCESS, 0, NULL);
		Print(L"Failed to reboot\n");
		return -1;
	}

	return 0;
}

static INTN mok_sb_prompt (void *MokSB, void *data2, void *data3) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINTN MokSBSize = (UINTN)data2;
	MokSBvar *var = MokSB;
	CHAR16 pass1, pass2, pass3;
	UINT8 fail_count = 0;
	UINT32 length;
	CHAR16 line[1];
	UINT8 sbval = 1;
	UINT8 pos1, pos2, pos3;

	if (MokSBSize != sizeof(MokSBvar)) {
		Print(L"Invalid MokSB variable contents\n");
		return -1;
	}

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

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

		Print(L"Enter password character %d: ", pos1 + 1);
		get_line(&length, &pass1, 1, 0);

		Print(L"Enter password character %d: ", pos2 + 1);
		get_line(&length, &pass2, 1, 0);

		Print(L"Enter password character %d: ", pos3 + 1);
		get_line(&length, &pass3, 1, 0);

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
		Print(L"Password limit reached\n");
		return -1;
	}

	if (var->MokSBState == 0) {
		Print(L"Disable Secure Boot? (y/n): ");
	} else {
		Print(L"Enable Secure Boot? (y/n): ");
	}

	do {
		get_line (&length, line, 1, 1);

		if (line[0] == 'Y' || line[0] == 'y') {
			if (var->MokSBState == 0) {
				efi_status = uefi_call_wrapper(RT->SetVariable,
							       5, L"MokSBState",
							       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
							       1, &sbval);
				if (efi_status != EFI_SUCCESS) {
					Print(L"Failed to set Secure Boot state\n");
					return -1;
				}
			} else {
				LibDeleteVariable(L"MokSBState",
						  &shim_lock_guid);
			}

			LibDeleteVariable(L"MokSB", &shim_lock_guid);

			Print(L"Press a key to reboot system\n");
			Pause();
			uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm,
					  EFI_SUCCESS, 0, NULL);
			Print(L"Failed to reboot\n");
			return -1;
		}
	} while (line[0] != 'N' && line[0] != 'n');

	return -1;
}


static INTN mok_pw_prompt (void *MokPW, void *data2, void *data3) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINTN MokPWSize = (UINTN)data2;
	UINT8 fail_count = 0;
	UINT8 hash[SHA256_DIGEST_SIZE];
	CHAR16 password[PASSWORD_MAX];
	UINT32 length;
	CHAR16 line[1];

	if (MokPWSize != SHA256_DIGEST_SIZE) {
		Print(L"Invalid MokPW variable contents\n");
		return -1;
	}

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	SetMem(hash, SHA256_DIGEST_SIZE, 0);

	if (CompareMem(MokPW, hash, SHA256_DIGEST_SIZE) == 0) {
		Print(L"Clear MOK password? (y/n): ");

		do {
			get_line (&length, line, 1, 1);

			if (line[0] == 'Y' || line[0] == 'y') {
				LibDeleteVariable(L"MokPWStore", &shim_lock_guid);
				LibDeleteVariable(L"MokPW", &shim_lock_guid);
			}
		} while (line[0] != 'N' && line[0] != 'n');

		return 0;
	}

	while (fail_count < 3) {
		Print(L"Confirm MOK passphrase: ");
		get_line(&length, password, PASSWORD_MAX, 0);

		if ((length < PASSWORD_MIN) || (length > PASSWORD_MAX)) {
			Print(L"Invalid password length\n");
			fail_count++;
			continue;
		}

		efi_status = compute_pw_hash(NULL, 0, password, length, hash);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Unable to generate password hash\n");
			fail_count++;
			continue;
		}

		if (CompareMem(MokPW, hash, SHA256_DIGEST_SIZE) != 0) {
			Print(L"Password doesn't match\n");
			fail_count++;
			continue;
		}

		break;
	}

	if (fail_count >= 3) {
		Print(L"Password limit reached\n");
		return -1;
	}

	Print(L"Set MOK password? (y/n): ");

	do {
		get_line (&length, line, 1, 1);

		if (line[0] == 'Y' || line[0] == 'y') {
			efi_status = uefi_call_wrapper(RT->SetVariable, 5,
						       L"MokPWStore",
						       &shim_lock_guid,
					       EFI_VARIABLE_NON_VOLATILE |
					       EFI_VARIABLE_BOOTSERVICE_ACCESS,
						       MokPWSize, MokPW);
			if (efi_status != EFI_SUCCESS) {
				Print(L"Failed to set MOK password\n");
				return -1;
			}

			LibDeleteVariable(L"MokPW", &shim_lock_guid);

			Print(L"Press a key to reboot system\n");
			Pause();
			uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm,
					  EFI_SUCCESS, 0, NULL);
			Print(L"Failed to reboot\n");
			return -1;
		}
	} while (line[0] != 'N' && line[0] != 'n');

	return 0;
}

static UINTN draw_menu (CHAR16 *header, UINTN lines, struct menu_item *items,
			UINTN count) {
	UINTN i;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
			  EFI_WHITE | EFI_BACKGROUND_BLACK);

	Print(L"%s UEFI key management\n\n", SHIM_VENDOR);

	if (header)
		Print(L"%s", header);

	for (i = 0; i < count; i++) {
		uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
				  items[i].colour | EFI_BACKGROUND_BLACK);
		Print(L"  %s\n", items[i].text);
	}

	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 0);
	uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, TRUE);

	return 2 + lines;
}

static void free_menu (struct menu_item *items, UINTN count) {
	UINTN i;

	for (i=0; i<count; i++) {
		if (items[i].text)
			FreePool(items[i].text);
	}

	FreePool(items);
}

static void update_time (UINTN position, UINTN timeout)
{
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0,
			  position);

	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
			  EFI_BLACK | EFI_BACKGROUND_BLACK);

	Print(L"                       ", timeout);

	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0,
			  position);

	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
			  EFI_WHITE | EFI_BACKGROUND_BLACK);

	if (timeout > 1)
		Print(L"Booting in %d seconds\n", timeout);
	else if (timeout)
		Print(L"Booting in %d second\n", timeout);
}

static void run_menu (CHAR16 *header, UINTN lines, struct menu_item *items,
		      UINTN count, UINTN timeout) {
	UINTN index, pos = 0, wait = 0, offset;
	EFI_INPUT_KEY key;
	EFI_STATUS status;
	INTN ret;

	if (timeout)
		wait = 10000000;

	offset = draw_menu (header, lines, items, count);

	while (1) {
		update_time(count + offset + 1, timeout);

		uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut,
				  0, pos + offset);
		status = WaitForSingleEvent(ST->ConIn->WaitForKey, wait);

		if (status == EFI_TIMEOUT) {
			timeout--;
			if (!timeout) {
				free_menu(items, count);
				return;
			}
			continue;
		}

		wait = 0;
		timeout = 0;

		uefi_call_wrapper(BS->WaitForEvent, 3, 1,
				  &ST->ConIn->WaitForKey, &index);
		uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn,
				  &key);

		switch(key.ScanCode) {
		case SCAN_UP:
			if (pos == 0)
				continue;
			pos--;
			continue;
			break;
		case SCAN_DOWN:
			if (pos == (count - 1))
				continue;
			pos++;
			continue;
			break;
		}

		switch(key.UnicodeChar) {
		case CHAR_LINEFEED:
		case CHAR_CARRIAGE_RETURN:
			if (items[pos].callback == NULL) {
				free_menu(items, count);
				return;
			}

			ret = items[pos].callback(items[pos].data,
						  items[pos].data2,
						  items[pos].data3);
			if (ret < 0) {
				Print(L"Press a key to continue\n");
				Pause();
			}
			draw_menu (header, lines, items, count);
			pos = 0;
			break;
		}
	}
}

static UINTN verify_certificate(void *cert, UINTN size)
{
	X509 *X509Cert;
	if (!cert || size == 0)
		return FALSE;

	if (!(X509ConstructCertificate(cert, size, (UINT8 **) &X509Cert)) ||
	    X509Cert == NULL) {
		Print(L"Invalid X509 certificate\n");
		Pause();
		return FALSE;
	}

	X509_free(X509Cert);
	return TRUE;
}

static INTN file_callback (void *data, void *data2, void *data3) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0, mokbuffersize;
	EFI_STATUS status;
	EFI_FILE *file;
	CHAR16 *filename = data;
	EFI_FILE *parent = data2;
	BOOLEAN hash = !!data3;
	EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_SIGNATURE_LIST *CertList;
	EFI_SIGNATURE_DATA *CertData;
	void *mokbuffer = NULL;

	status = uefi_call_wrapper(parent->Open, 5, parent, &file, filename,
				   EFI_FILE_MODE_READ, 0);

	if (status != EFI_SUCCESS)
		return 1;

	status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid,
				   &buffersize, buffer);

	if (status == EFI_BUFFER_TOO_SMALL) {
		buffer = AllocatePool(buffersize);
		status = uefi_call_wrapper(file->GetInfo, 4, file,
					   &file_info_guid, &buffersize,
					   buffer);
	}

	if (!buffer)
		return 0;

	buffersize = buffer->FileSize;

	if (hash) {
		void *binary;
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

		binary = AllocatePool(buffersize);

		status = uefi_call_wrapper(file->Read, 3, file, &buffersize,
					   binary);

		if (status != EFI_SUCCESS)
			goto out;

		status = shim_lock->Context(binary, buffersize, &context);

		if (status != EFI_SUCCESS)
			goto out;

		status = shim_lock->Hash(binary, buffersize, &context, sha256,
					 sha1);

		if (status != EFI_SUCCESS)
			goto out;

		CertList = mokbuffer;
		CertList->SignatureType = EfiHashSha256Guid;
		CertList->SignatureSize = 16 + SHA256_DIGEST_SIZE;
		CertData = (EFI_SIGNATURE_DATA *)(((UINT8 *)mokbuffer) +
						  sizeof(EFI_SIGNATURE_LIST));
		CopyMem(CertData->SignatureData, sha256, SHA256_DIGEST_SIZE);
	} else {
		mokbuffersize = buffersize + sizeof(EFI_SIGNATURE_LIST) +
			sizeof(EFI_GUID);
		mokbuffer = AllocatePool(mokbuffersize);

		if (!mokbuffer)
			goto out;

		CertList = mokbuffer;
		CertList->SignatureType = EfiCertX509Guid;
		CertList->SignatureSize = 16 + buffersize;
		status = uefi_call_wrapper(file->Read, 3, file, &buffersize,
				 mokbuffer + sizeof(EFI_SIGNATURE_LIST) + 16);

		if (status != EFI_SUCCESS)
			goto out;
		CertData = (EFI_SIGNATURE_DATA *)(((UINT8 *)mokbuffer) +
						  sizeof(EFI_SIGNATURE_LIST));
	}

	CertList->SignatureListSize = mokbuffersize;
	CertList->SignatureHeaderSize = 0;
	CertData->SignatureOwner = shim_lock_guid;

	if (!hash) {
		if (!verify_certificate(CertData->SignatureData, buffersize))
			goto out;
	}

	mok_enrollment_prompt(mokbuffer, mokbuffersize, FALSE);
out:
	if (buffer)
		FreePool(buffer);

	if (mokbuffer)
		FreePool(mokbuffer);

	return 0;
}

static INTN directory_callback (void *data, void *data2, void *data3) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0;
	EFI_STATUS status;
	UINTN dircount = 0, i = 0;
	struct menu_item *dircontent;
	EFI_FILE *dir;
	CHAR16 *filename = data;
	EFI_FILE *root = data2;
	BOOLEAN hash = !!data3;

	status = uefi_call_wrapper(root->Open, 5, root, &dir, filename,
				   EFI_FILE_MODE_READ, 0);

	if (status != EFI_SUCCESS)
		return 1;

	while (1) {
		status = uefi_call_wrapper(dir->Read, 3, dir, &buffersize,
					   buffer);

		if (status == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocatePool(buffersize);
			status = uefi_call_wrapper(dir->Read, 3, dir,
						   &buffersize, buffer);
		}

		if (status != EFI_SUCCESS)
			return 1;

		if (!buffersize)
			break;

		if ((StrCmp(buffer->FileName, L".") == 0) ||
		    (StrCmp(buffer->FileName, L"..") == 0))
			continue;

		dircount++;

		FreePool(buffer);
		buffersize = 0;
	}

	dircount++;

	dircontent = AllocatePool(sizeof(struct menu_item) * dircount);

	dircontent[0].text = StrDuplicate(L"..");
	dircontent[0].callback = NULL;
	dircontent[0].colour = EFI_YELLOW;
	i++;

	uefi_call_wrapper(dir->SetPosition, 2, dir, 0);

	while (1) {
		status = uefi_call_wrapper(dir->Read, 3, dir, &buffersize,
					   buffer);

		if (status == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocatePool(buffersize);
			status = uefi_call_wrapper(dir->Read, 3, dir,
						   &buffersize, buffer);
		}

		if (status != EFI_SUCCESS)
			return 1;

		if (!buffersize)
			break;

		if ((StrCmp(buffer->FileName, L".") == 0) ||
		    (StrCmp(buffer->FileName, L"..") == 0))
			continue;

		if (buffer->Attribute & EFI_FILE_DIRECTORY) {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = directory_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = dir;
			dircontent[i].data3 = data3;
			dircontent[i].colour = EFI_YELLOW;
		} else {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = file_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = dir;
			dircontent[i].data3 = data3;
			dircontent[i].colour = EFI_WHITE;
		}

		i++;
		FreePool(buffer);
		buffersize = 0;
		buffer = NULL;
	}

	if (hash)
		run_menu(HASH_STRING, 2, dircontent, dircount, 0);
	else
		run_menu(CERT_STRING, 2, dircontent, dircount, 0);

	return 0;
}

static INTN filesystem_callback (void *data, void *data2, void *data3) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0;
	EFI_STATUS status;
	UINTN dircount = 0, i = 0;
	struct menu_item *dircontent;
	EFI_FILE *root = data;
	BOOLEAN hash = !!data3;

	uefi_call_wrapper(root->SetPosition, 2, root, 0);

	while (1) {
		status = uefi_call_wrapper(root->Read, 3, root, &buffersize,
					   buffer);

		if (status == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocatePool(buffersize);
			status = uefi_call_wrapper(root->Read, 3, root,
						   &buffersize, buffer);
		}

		if (status != EFI_SUCCESS)
			return 1;

		if (!buffersize)
			break;

		if ((StrCmp(buffer->FileName, L".") == 0) ||
		    (StrCmp(buffer->FileName, L"..") == 0))
			continue;

		dircount++;

		FreePool(buffer);
		buffersize = 0;
	}

	dircount++;

	dircontent = AllocatePool(sizeof(struct menu_item) * dircount);

	dircontent[0].text = StrDuplicate(L"Return to filesystem list");
	dircontent[0].callback = NULL;
	dircontent[0].colour = EFI_YELLOW;
	i++;

	uefi_call_wrapper(root->SetPosition, 2, root, 0);

	while (1) {
		status = uefi_call_wrapper(root->Read, 3, root, &buffersize,
					   buffer);

		if (status == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocatePool(buffersize);
			status = uefi_call_wrapper(root->Read, 3, root,
						   &buffersize, buffer);
		}

		if (status != EFI_SUCCESS)
			return 1;

		if (!buffersize)
			break;

		if ((StrCmp(buffer->FileName, L".") == 0) ||
		    (StrCmp(buffer->FileName, L"..") == 0))
			continue;

		if (buffer->Attribute & EFI_FILE_DIRECTORY) {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = directory_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = root;
			dircontent[i].data3 = data3;
			dircontent[i].colour = EFI_YELLOW;
		} else {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = file_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = root;
			dircontent[i].data3 = data3;
			dircontent[i].colour = EFI_WHITE;
		}

		i++;
		FreePool(buffer);
		buffer = NULL;
		buffersize = 0;
	}

	if (hash)
		run_menu(HASH_STRING, 2, dircontent, dircount, 0);
	else
		run_menu(CERT_STRING, 2, dircontent, dircount, 0);

	return 0;
}

static INTN find_fs (void *data, void *data2, void *data3) {
	EFI_GUID fs_guid = SIMPLE_FILE_SYSTEM_PROTOCOL;
	UINTN count, i;
	UINTN OldSize, NewSize;
	EFI_HANDLE *filesystem_handles = NULL;
	struct menu_item *filesystems;
	BOOLEAN hash = !!data3;

	uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &fs_guid,
			  NULL, &count, &filesystem_handles);

	if (!count || !filesystem_handles) {
		Print(L"No filesystems?\n");
		return 1;
	}

	count++;

	filesystems = AllocatePool(sizeof(struct menu_item) * count);

	filesystems[0].text = StrDuplicate(L"Exit");
	filesystems[0].callback = NULL;
	filesystems[0].colour = EFI_YELLOW;

	for (i=1; i<count; i++) {
		EFI_HANDLE fs = filesystem_handles[i-1];
		EFI_FILE_IO_INTERFACE *fs_interface;
		EFI_DEVICE_PATH *path;
		EFI_FILE *root;
		EFI_STATUS status;
		CHAR16 *VolumeLabel = NULL;
		EFI_FILE_SYSTEM_INFO *buffer = NULL;
		UINTN buffersize = 0;
		EFI_GUID file_info_guid = EFI_FILE_INFO_ID;

		status = uefi_call_wrapper(BS->HandleProtocol, 3, fs, &fs_guid,
					   (void **)&fs_interface);

		if (status != EFI_SUCCESS || !fs_interface)
			continue;

		path = DevicePathFromHandle(fs);

		status = uefi_call_wrapper(fs_interface->OpenVolume, 2,
					   fs_interface, &root);

		if (status != EFI_SUCCESS || !root)
			continue;

		status = uefi_call_wrapper(root->GetInfo, 4, root,
					   &file_info_guid, &buffersize,
					   buffer);

		if (status == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocatePool(buffersize);
			status = uefi_call_wrapper(root->GetInfo, 4, root,
						   &file_info_guid,
						   &buffersize, buffer);
		}

		if (status == EFI_SUCCESS)
			VolumeLabel = buffer->VolumeLabel;

		if (path)
			filesystems[i].text = DevicePathToStr(path);
		else
			filesystems[i].text = StrDuplicate(L"Unknown device\n");
		if (VolumeLabel) {
			OldSize = (StrLen(filesystems[i].text) + 1) * sizeof(CHAR16);
			NewSize = OldSize + StrLen(VolumeLabel) * sizeof(CHAR16);
			filesystems[i].text = ReallocatePool(filesystems[i].text,
							     OldSize, NewSize);
			StrCat(filesystems[i].text, VolumeLabel);
		}

		if (buffersize)
			FreePool(buffer);

		filesystems[i].data = root;
		filesystems[i].data2 = NULL;
		filesystems[i].data3 = data3;
		filesystems[i].callback = filesystem_callback;
		filesystems[i].colour = EFI_YELLOW;
	}

	uefi_call_wrapper(BS->FreePool, 1, filesystem_handles);

	if (hash)
		run_menu(HASH_STRING, 2, filesystems, count, 0);
	else
		run_menu(CERT_STRING, 2, filesystems, count, 0);

	return 0;
}

static BOOLEAN verify_pw(void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	CHAR16 password[PASSWORD_MAX];
	UINT8 fail_count = 0;
	UINT8 hash[SHA256_DIGEST_SIZE];
	UINT8 pwhash[SHA256_DIGEST_SIZE];
	UINTN size = SHA256_DIGEST_SIZE;
	UINT32 length;
	UINT32 attributes;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokPWStore",
				       &shim_lock_guid, &attributes, &size,
				       pwhash);

	/*
	 * If anything can attack the password it could just set it to a
	 * known value, so there's no safety advantage in failing to validate
	 * purely because of a failure to read the variable
	 */
	if (efi_status != EFI_SUCCESS)
		return TRUE;

	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS)
		return TRUE;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	while (fail_count < 3) {
		Print(L"Enter MOK password: ");
		get_line(&length, password, PASSWORD_MAX, 0);

		if (length < PASSWORD_MIN || length > PASSWORD_MAX) {
			Print(L"Invalid password length\n");
			fail_count++;
			continue;
		}

		efi_status = compute_pw_hash(NULL, 0, password, length, hash);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Unable to generate password hash\n");
			fail_count++;
			continue;
		}

		if (CompareMem(pwhash, hash, SHA256_DIGEST_SIZE) != 0) {
			Print(L"Password doesn't match\n");
			fail_count++;
			continue;
		}

		return TRUE;
	}

	Print(L"Password limit reached\n");
	return FALSE;
}

static EFI_STATUS enter_mok_menu(EFI_HANDLE image_handle, void *MokNew,
				 UINTN MokNewSize, void *MokSB,
				 UINTN MokSBSize, void *MokPW, UINTN MokPWSize)
{
	struct menu_item *menu_item;
	UINT32 MokAuth = 0;
	UINTN menucount = 3, i = 0;
	EFI_STATUS efi_status;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINT8 auth[SHA256_DIGEST_SIZE];
	UINTN auth_size = SHA256_DIGEST_SIZE;
	UINT32 attributes;

	if (verify_pw() == FALSE)
		return EFI_ACCESS_DENIED;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokAuth",
					       &shim_lock_guid,
					       &attributes, &auth_size, auth);

	if ((efi_status == EFI_SUCCESS) && (auth_size == SHA256_DIGEST_SIZE))
		MokAuth = 1;

	if (MokNew || MokAuth)
		menucount++;

	if (MokSB)
		menucount++;

	if (MokPW)
		menucount++;

	menu_item = AllocateZeroPool(sizeof(struct menu_item) * menucount);

	if (!menu_item)
		return EFI_OUT_OF_RESOURCES;

	menu_item[i].text = StrDuplicate(L"Continue boot");
	menu_item[i].colour = EFI_WHITE;
	menu_item[i].callback = NULL;

	i++;

	if (MokNew || MokAuth) {
		if (!MokNew) {
			menu_item[i].text = StrDuplicate(L"Delete MOK");
			menu_item[i].colour = EFI_WHITE;
			menu_item[i].callback = mok_deletion_prompt;
		} else {
			menu_item[i].text = StrDuplicate(L"Enroll MOK");
			menu_item[i].colour = EFI_WHITE;
			menu_item[i].data = MokNew;
			menu_item[i].data2 = (void *)MokNewSize;
			menu_item[i].callback = mok_enrollment_prompt_callback;
		}
		i++;
	}

	if (MokSB) {
		menu_item[i].text = StrDuplicate(L"Change Secure Boot state");
		menu_item[i].colour = EFI_WHITE;
		menu_item[i].callback = mok_sb_prompt;
		menu_item[i].data = MokSB;
		menu_item[i].data2 = (void *)MokSBSize;
		i++;
	}

	if (MokPW) {
		menu_item[i].text = StrDuplicate(L"Set MOK password");
		menu_item[i].colour = EFI_WHITE;
		menu_item[i].callback = mok_pw_prompt;
		menu_item[i].data = MokPW;
		menu_item[i].data2 = (void *)MokPWSize;
		i++;
	}

	menu_item[i].text = StrDuplicate(L"Enroll key from disk");
	menu_item[i].colour = EFI_WHITE;
	menu_item[i].callback = find_fs;
	menu_item[i].data3 = (void *)FALSE;

	i++;

	menu_item[i].text = StrDuplicate(L"Enroll hash from disk");
	menu_item[i].colour = EFI_WHITE;
	menu_item[i].callback = find_fs;
	menu_item[i].data3 = (void *)TRUE;

	i++;

	run_menu(NULL, 0, menu_item, menucount, 10);

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	return 0;
}

static EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINTN MokNewSize = 0, MokSBSize = 0, MokPWSize = 0;
	void *MokNew = NULL;
	void *MokSB = NULL;
	void *MokPW = NULL;

	MokNew = LibGetVariableAndSize(L"MokNew", &shim_lock_guid, &MokNewSize);

	MokSB = LibGetVariableAndSize(L"MokSB", &shim_lock_guid, &MokSBSize);

	MokPW = LibGetVariableAndSize(L"MokPW", &shim_lock_guid, &MokPWSize);

	enter_mok_menu(image_handle, MokNew, MokNewSize, MokSB, MokSBSize,
		       MokPW, MokPWSize);

	if (MokNew) {
		if (LibDeleteVariable(L"MokNew", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to delete MokNew\n");
		}
		FreePool (MokNew);
	}

	if (MokSB) {
		if (LibDeleteVariable(L"MokSB", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to delete MokSB\n");
		}
		FreePool (MokNew);
	}

	if (MokPW) {
		if (LibDeleteVariable(L"MokPW", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to delete MokPW\n");
		}
		FreePool (MokNew);
	}

	LibDeleteVariable(L"MokAuth", &shim_lock_guid);

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

	setup_rand();

	efi_status = check_mok_request(image_handle);

	return efi_status;
}
