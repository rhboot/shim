#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include <openssl/x509.h>
#include "shim.h"

#define PASSWORD_MAX 16
#define PASSWORD_MIN 8

struct menu_item {
	CHAR16 *text;
	UINTN (* callback)(void *data, void *data2);
	void *data;
	void *data2;
	UINTN colour;
};

typedef struct {
	UINT32 MokSize;
	UINT8 *Mok;
} __attribute__ ((packed)) MokListNode;

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
	INT64 remain = DataSize;
	int i;
	void *ptr;

	if (DataSize < sizeof(UINT32))
		return NULL;

	list = AllocatePool(sizeof(MokListNode) * num);

	if (!list) {
		Print(L"Unable to allocate MOK list\n");
		return NULL;
	}

	ptr = Data;
	for (i = 0; i < num; i++) {
		CopyMem(&list[i].MokSize, ptr, sizeof(UINT32));
		remain -= sizeof(UINT32) + list[i].MokSize;

		if (remain < 0) {
			Print(L"the list was corrupted\n");
			FreePool(list);
			return NULL;
		}

		ptr += sizeof(UINT32);
		list[i].Mok = ptr;
		ptr += list[i].MokSize;
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

	if (X509ConstructCertificate(Mok, MokSize, (UINT8 **) &X509Cert) &&
	    X509Cert != NULL) {
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
	UINT32 MokNum;
	MokListNode *keys = NULL;
	INTN key_num = 0;
	UINT8 initial = 1;

	CopyMem(&MokNum, MokNew, sizeof(UINT32));
	if (MokNum == 0) {
		Print(L"No key exists\n");
		return 0;
	}

	keys = build_mok_list(MokNum,
			      (void *)MokNew + sizeof(UINT32),
			      MokNewSize - sizeof(UINT32));

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

	if (!(Sha256Update(ctx, MokNew, MokNewSize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
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

static EFI_STATUS store_keys (void *MokNew, UINTN MokNewSize)
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

	/* Write new MOK */
	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokList",
				       &shim_lock_guid,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       MokNewSize, MokNew);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set variable %d\n", efi_status);
		return efi_status;
	}

	return EFI_SUCCESS;
}

static UINTN mok_enrollment_prompt (void *MokNew, void *data2) {
	CHAR16 line[1];
	UINT32 length;
	UINTN MokNewSize = (UINTN)data2;
	EFI_STATUS efi_status;

	do {
		if (!list_keys(MokNew, MokNewSize)) {
			return 0;
		}

		Print(L"Enroll the key(s)? (y/n): ");

		get_line (&length, line, 1, 1);

		if (line[0] == 'Y' || line[0] == 'y') {
			efi_status = store_keys(MokNew, MokNewSize);

			if (efi_status != EFI_SUCCESS) {
				Print(L"Failed to enroll keys\n");
				return -1;
			}
			return 0;
		}
	} while (line[0] != 'N' && line[0] != 'n');
	return -1;
}

static UINTN mok_deletion_prompt (void *MokNew, void *data2) {
	CHAR16 line[1];
	UINT32 length;
	EFI_STATUS efi_status;

	Print(L"Erase all stored keys? (y/N): ");

	get_line (&length, line, 1, 1);

	if (line[0] == 'Y' || line[0] == 'y') {
		efi_status = store_keys(MokNew, sizeof(UINT32));

		if (efi_status != EFI_SUCCESS) {
			Print(L"Failed to erase keys\n");
			return -1;
		}
	}

	return 0;
}

static void draw_menu (struct menu_item *items, UINTN count) {
	UINTN i;

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	for (i = 0; i < count; i++) {
		uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut,
				  items[i].colour | EFI_BACKGROUND_BLACK);
		Print(L"  %s\n", items[i].text);
	}

	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 0);
	uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, TRUE);
}

static void free_menu (struct menu_item *items, UINTN count) {
	UINTN i;

#if 0
	for (i=0; i<count; i++) {
		if (items[i].text)
			FreePool(items[i].text);
	}

	FreePool(items);
#endif
}

static void run_menu (struct menu_item *items, UINTN count) {
	UINTN index, pos = 0;
	EFI_INPUT_KEY key;

	draw_menu (items, count);

	while (1) {
		uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut,
				  0, pos);
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

			items[pos].callback(items[pos].data, items[pos].data2);
			draw_menu (items, count);
			pos = 0;
			break;
		}
	}
}

static UINTN file_callback (void *data, void *data2) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0, readsize;
	EFI_STATUS status;
	EFI_FILE *file;
	CHAR16 *filename = data;
	EFI_FILE *parent = data2;
	EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
	void *mokbuffer = NULL;
	void *filebuffer;

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

	readsize = buffer->FileSize;

	mokbuffer = AllocateZeroPool(readsize + (2 * sizeof(UINT32)));
	if (!mokbuffer)
		goto out;

	((UINT32 *)mokbuffer)[0] = 1;
	((UINT32 *)mokbuffer)[1] = readsize;
	filebuffer = (UINT32 *)mokbuffer + 2;

	status = uefi_call_wrapper(file->Read, 3, file, &readsize, filebuffer);

	if (status != EFI_SUCCESS)
		goto out;

	mok_enrollment_prompt(mokbuffer,
			      (void *)buffer->FileSize + (2 * sizeof(UINT32)));
out:
	if (buffer)
		FreePool(buffer);

	if (mokbuffer)
		FreePool(mokbuffer);

	return 0;
}

static UINTN directory_callback (void *data, void *data2) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0;
	EFI_STATUS status;
	UINTN dircount = 0, i = 0;
	struct menu_item *dircontent;
	EFI_FILE *dir;
	CHAR16 *filename = data;
	EFI_FILE *root = data2;

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
			dircontent[i].colour = EFI_YELLOW;
		} else {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = file_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = dir;
			dircontent[i].colour = EFI_WHITE;
		}

		i++;
		FreePool(buffer);
		buffersize = 0;
		buffer = NULL;
	}

	run_menu(dircontent, dircount);

	return 0;
}

static UINTN filesystem_callback (void *data, void *data2) {
	EFI_FILE_INFO *buffer = NULL;
	UINTN buffersize = 0;
	EFI_STATUS status;
	UINTN dircount = 0, i = 0;
	struct menu_item *dircontent;
	EFI_FILE *root = data;
	EFI_FILE *parent = data2;

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

	if (parent)
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
			dircontent[i].colour = EFI_YELLOW;
		} else {
			dircontent[i].text = StrDuplicate(buffer->FileName);
			dircontent[i].callback = file_callback;
			dircontent[i].data = dircontent[i].text;
			dircontent[i].data2 = root;
			dircontent[i].colour = EFI_WHITE;
		}

		i++;
		FreePool(buffer);
		buffer = NULL;
		buffersize = 0;
	}

	run_menu(dircontent, dircount);

	return 0;
}

static UINTN find_fs (void *data, void *data2) {
	EFI_GUID fs_guid = SIMPLE_FILE_SYSTEM_PROTOCOL;
	UINTN count, i;
	EFI_HANDLE **filesystem_handles;
	struct menu_item *filesystems;

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
		EFI_HANDLE *fs = filesystem_handles[i-1];
		EFI_FILE_IO_INTERFACE *fs_interface;
		EFI_DEVICE_PATH *path;
		EFI_FILE *root;
		EFI_STATUS status;
		CHAR16 *VolumeLabel = NULL;
		EFI_FILE_SYSTEM_INFO *buffer = NULL;
		UINTN buffersize = 0;
		EFI_GUID file_info_guid = EFI_FILE_INFO_ID;

		status = uefi_call_wrapper(BS->HandleProtocol, 3, fs, &fs_guid,
					   &fs_interface);

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
		if (VolumeLabel)
			StrCat(filesystems[i].text, VolumeLabel);

		if (buffersize)
			FreePool(buffer);

		filesystems[i].data = root;
		filesystems[i].data2 = NULL;
		filesystems[i].callback = filesystem_callback;
		filesystems[i].colour = EFI_YELLOW;
	}

	uefi_call_wrapper(BS->FreePool, 1, filesystem_handles);

	run_menu(filesystems, count);

	return 0;
}

static EFI_STATUS enter_mok_menu(EFI_HANDLE image_handle, void *MokNew)
{
	struct menu_item *menu_item;
	UINT32 MokNum;
	UINTN menucount = 0;

	if (MokNew)
		menu_item = AllocateZeroPool(sizeof(struct menu_item) * 3);
	else
		menu_item = AllocateZeroPool(sizeof(struct menu_item) * 2);

	if (!menu_item)
		return EFI_OUT_OF_RESOURCES;

	menu_item[0].text = StrDuplicate(L"Continue boot");
	menu_item[0].colour = EFI_WHITE;
	menu_item[0].callback = NULL;

	menucount++;

	if (MokNew) {
		CopyMem(&MokNum, MokNew, sizeof(UINT32));
		if (MokNum == 0) {
			menu_item[1].text = StrDuplicate(L"Delete MOK");
			menu_item[1].colour = EFI_WHITE;
			menu_item[1].data = MokNew;
			menu_item[1].callback = mok_deletion_prompt;
		} else {
			menu_item[1].text = StrDuplicate(L"Enroll MOK\n");
			menu_item[1].colour = EFI_WHITE;
			menu_item[1].data = MokNew;
			menu_item[1].callback = mok_enrollment_prompt;
		}
		menucount++;
	}

	menu_item[menucount].text = StrDuplicate(L"Enroll key from disk");
	menu_item[menucount].colour = EFI_WHITE;
	menu_item[menucount].callback = find_fs;

	menucount++;

	run_menu(menu_item, menucount);

	return 0;
}

static EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINTN MokNewSize = 0;
	void *MokNew = NULL;

	MokNew = LibGetVariableAndSize(L"MokNew", &shim_lock_guid, &MokNewSize);

	enter_mok_menu(image_handle, MokNew);

	if (MokNew) {
		if (LibDeleteVariable(L"MokNew", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to delete MokNew\n");
		}
		FreePool (MokNew);
	}
	LibDeleteVariable(L"MokAuth", &shim_lock_guid);

	return EFI_SUCCESS;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS efi_status;

	InitializeLib(image_handle, systab);

	efi_status = check_mok_request(image_handle);

	return efi_status;
}
