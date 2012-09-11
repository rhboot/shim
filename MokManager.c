#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include <openssl/x509.h>
#include "shim.h"

typedef struct {
	UINT32 MokSize;
	UINT8 *Mok;
} MokListNode;

static EFI_STATUS get_variable (CHAR16 *name, EFI_GUID guid, UINT32 *attributes,
				UINTN *size, void **buffer)
{
	EFI_STATUS efi_status;
	char allocate = !(*size);

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       attributes, size, buffer);

	if (efi_status != EFI_BUFFER_TOO_SMALL || !allocate) {
		return efi_status;
	}

	if (allocate)
		*buffer = AllocatePool(*size);

	if (!*buffer) {
		Print(L"Unable to allocate variable buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       attributes, size, *buffer);

	return efi_status;
}

static EFI_STATUS delete_variable (CHAR16 *name, EFI_GUID guid)
{
	EFI_STATUS efi_status;

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, name, &guid,
				       0, 0, (UINT8 *)NULL);

	return efi_status;
}

static EFI_INPUT_KEY get_keystroke (void)
{
	EFI_INPUT_KEY key;
	UINTN EventIndex;

	uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey,
			  &EventIndex);
	uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);

	return key;
}

static EFI_STATUS get_sha256sum (void *Data, int DataSize, UINT8 *hash)
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

	if (!(Sha256Update(ctx, Data, DataSize))) {
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

static MokListNode *build_mok_list(UINT32 num, void *Data, UINTN DataSize) {
	MokListNode *list;
	int i, remain = DataSize;
	void *ptr;

	list = AllocatePool(sizeof(MokListNode) * num);

	if (!list) {
		Print(L"Unable to allocate MOK list\n");
		return NULL;
	}

	ptr = Data;
	for (i = 0; i < num; i++) {
		if (remain < 0) {
			Print(L"MOK list was corrupted\n");
			FreePool(list);
			return NULL;
		}

		CopyMem(&list[i].MokSize, ptr, sizeof(UINT32));
		ptr += sizeof(UINT32);
		list[i].Mok = ptr;
		ptr += list[i].MokSize;

		remain -= sizeof(UINT32) + list[i].MokSize;
	}

	return list;
}

/* XXX MOK functions */
static void print_x509_name (X509_NAME *X509Name, char *name)
{
	char *str;

	str = X509_NAME_oneline(X509Name, NULL, 0);
	if (str) {
		APrint((CHAR8 *)"%a: %a\n", name, str);
		OPENSSL_free(str);
	}
}

static const char *mon[12]= {
"Jan","Feb","Mar","Apr","May","Jun",
"Jul","Aug","Sep","Oct","Nov","Dec"
};

static void print_x509_GENERALIZEDTIME_time (ASN1_TIME *time, char *name) {
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
			while (14 + f_len < l && f[f_len] >= '0' && f[f_len] <= '9')
				++f_len;
		}
	}

	APrint((CHAR8 *)"%a: %a %2d %02d:%02d:%02d%.*a %d%a",
	       name, mon[M-1],d,h,m,s,f_len,f,y,(gmt)?" GMT":"");
error:
	return;
}

static void print_x509_UTCTIME_time (ASN1_TIME *time, char *name)
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

	APrint((CHAR8 *)"%a: %a %2d %02d:%02d:%02d %d%a\n",
	       name, mon[M-1],d,h,m,s,y+1900,(gmt)?" GMT":"");
error:
	return;
}

static void print_x509_time (ASN1_TIME *time, char *name)
{
	if(time->type == V_ASN1_UTCTIME)
		print_x509_UTCTIME_time(time, name);

	if(time->type == V_ASN1_GENERALIZEDTIME)
		print_x509_GENERALIZEDTIME_time(time, name);
}

static void show_x509_info (X509 *X509Cert)
{
	X509_NAME *X509Name;
	ASN1_TIME *time;

	X509Name = X509_get_issuer_name(X509Cert);
	if (X509Name) {
		print_x509_name(X509Name, "Issuer");
	}

	X509Name = X509_get_subject_name(X509Cert);
	if (X509Name) {
		print_x509_name(X509Name, "Subject");
	}

	time = X509_get_notBefore(X509Cert);
	if (time) {
		print_x509_time(time, "Not Before");
	}

	time = X509_get_notAfter(X509Cert);
	if (time) {
		print_x509_time(time, "Not After");
	}
}

static void show_mok_info (void *Mok, UINTN MokSize)
{
	EFI_STATUS efi_status;
	UINT8 hash[SHA256_DIGEST_SIZE];
	unsigned int i;
	X509 *X509Cert;

	if (!Mok || MokSize == 0)
		return;

	efi_status = get_sha256sum(Mok, MokSize, hash);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to compute MOK fingerprint\n");
		return;
	}

	if (X509ConstructCertificate(Mok, MokSize, (UINT8 **) &X509Cert) &&
	    X509Cert != NULL) {
		show_x509_info(X509Cert);
		X509_free(X509Cert);
	}

	Print(L"Fingerprint (SHA256):\n");
	for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
		Print(L" %02x", hash[i]);
		if (i % 16 == 15)
			Print(L"\n");
	}
}

static UINT8 delete_mok(MokListNode *list, UINT32 MokNum, UINT32 delete)
{
	if (!list || !MokNum || MokNum <= delete)
		return 0;

	list[delete].Mok = NULL;
	list[delete].MokSize = 0;

	return 1;
}

static UINT8 mok_deletion_prompt(MokListNode *list, UINT32 MokNum)
{
	EFI_INPUT_KEY key;
	CHAR16 line[10];
	unsigned int word_count = 0;
	UINTN delete;

	Print(L"delete key: ");
	do {
		key = get_keystroke();
		if ((key.UnicodeChar < '0' ||
		     key.UnicodeChar > '9' ||
		     word_count >= 10) &&
		    key.UnicodeChar != CHAR_BACKSPACE)
			continue;

		if (word_count == 0 && key.UnicodeChar == CHAR_BACKSPACE)
			continue;

		Print(L"%c", key.UnicodeChar);

		if (key.UnicodeChar == CHAR_BACKSPACE) {
			word_count--;
			line[word_count] = '\0';
			continue;
		}

		line[word_count] = key.UnicodeChar;
		word_count++;
	} while (key.UnicodeChar != CHAR_CARRIAGE_RETURN);
	Print(L"\n");

	if (word_count == 0)
		return 0;

	line[word_count] = '\0';
	delete = Atoi(line)-1;

	if (delete >= MokNum) {
		Print(L"No such key\n");
		return 0;
	}

	if (!list[delete].Mok) {
		Print(L"Already deleted\n");
		return 0;
	}

	Print(L"Delete this key?\n");
	show_mok_info(list[delete].Mok, list[delete].MokSize);
	Print(L"(y/N) ");
	key = get_keystroke();
	if (key.UnicodeChar != 'y' && key.UnicodeChar != 'Y') {
		Print(L"N\nAbort\n");
		return 0;
	}
	Print(L"y\nDelete key %d\n", delete+1);

	return delete_mok(list, MokNum, delete);
}

static void write_mok_list(void *MokListData, UINTN MokListDataSize,
			   MokListNode *list, UINT32 MokNum)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT32 new_num = 0;
	unsigned int i;
	UINTN DataSize = 0;
	void *Data, *ptr;

	if (!MokListData || !list)
		return;

	for (i = 0; i < MokNum; i++) {
		if (list[i].Mok && list[i].MokSize > 0) {
			DataSize += list[i].MokSize + sizeof(UINT32);
			if (new_num < i) {
				list[new_num].Mok = list[i].Mok;
				list[new_num].MokSize = list[i].MokSize;
			}
			new_num++;
		}
	}

	if (new_num == 0) {
		Data = NULL;
		goto done;
	}

	DataSize += sizeof(UINT32);

	Data = AllocatePool(DataSize * sizeof(UINT8));
	ptr = Data;

	CopyMem(Data, &new_num, sizeof(new_num));
	ptr += sizeof(new_num);

	for (i = 0; i < new_num; i++) {
		CopyMem(ptr, &list[i].MokSize, sizeof(list[i].MokSize));
		ptr += sizeof(list[i].MokSize);
		CopyMem(ptr, list[i].Mok, list[i].MokSize);
		ptr += list[i].MokSize;
	}

done:
	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokList",
				       &shim_lock_guid,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       DataSize, Data);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set variable %d\n", efi_status);
	}

	if (Data)
		FreePool(Data);
}

static void mok_mgmt_shell (void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	unsigned int i, changed = 0;
	void *MokListData = NULL;
	UINTN MokListDataSize = 0;
	UINT32 MokNum;
	UINT32 attributes;
	MokListNode *list = NULL;
	EFI_INPUT_KEY key;

	efi_status = get_variable(L"MokList", shim_lock_guid, &attributes,
				  &MokListDataSize, &MokListData);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to get MokList\n");
		goto error;
	}

	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		Print(L"MokList is compromised!\nErase all keys in MokList!\n");
		if (delete_variable(L"MokList", shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to erase MokList\n");
		}
		goto error;
	}

	CopyMem(&MokNum, MokListData, sizeof(UINT32));
	if (MokNum == 0) {
		Print(L"No key enrolled\n");
		goto error;
	}
	list = build_mok_list(MokNum,
			      (void *)MokListData + sizeof(UINT32),
			      MokListDataSize - sizeof(UINT32));

	if (!list) {
		Print(L"Failed to construct MOK list\n");
		goto error;
	}

	do {
		Print(L"shim) ");
		key = get_keystroke();
		Print(L"%c\n", key.UnicodeChar);

		switch (key.UnicodeChar) {
			case 'l':
			case 'L':
				for (i = 0; i < MokNum; i++) {
					if (list[i].Mok) {
						Print(L"Key %d\n", i+1);
						show_mok_info(list[i].Mok, list[i].MokSize);
						Print(L"\n");
					}
				}
				break;
			case 'd':
			case 'D':
				if (mok_deletion_prompt(list, MokNum) && changed == 0)
					changed = 1;
				break;
		}
	} while (key.UnicodeChar != 'c' && key.UnicodeChar != 'C');

	if (changed) {
		write_mok_list(MokListData, MokListDataSize, list, MokNum);
	}

error:
	if (MokListData)
		FreePool(MokListData);
	if (list)
		FreePool(list);
}

static UINT8 mok_enrollment_prompt (void *Mok, UINTN MokSize)
{
	EFI_INPUT_KEY key;

	Print(L"New machine owner key:\n\n");
	show_mok_info(Mok, MokSize);
	Print(L"\nEnroll the key? (y/N): ");

	key = get_keystroke();
	Print(L"%c\n", key.UnicodeChar);

	if (key.UnicodeChar == 'Y' || key.UnicodeChar == 'y') {
		return 1;
	}

	Print(L"Abort\n");

	return 0;
}

static EFI_STATUS enroll_mok (void *Mok, UINT32 MokSize, void *OldData,
			      UINT32 OldDataSize, UINT32 MokNum)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	void *Data, *ptr;
	UINT32 DataSize = 0;

	if (OldData)
		DataSize += OldDataSize;
	else
		DataSize += sizeof(UINT32);
	DataSize += sizeof(UINT32);
	DataSize += MokSize;
	MokNum += 1;

	Data = AllocatePool(DataSize);

	if (!Data) {
		Print(L"Failed to allocate buffer for MOK list\n");
		return EFI_OUT_OF_RESOURCES;
	}

	ptr = Data;

	if (OldData) {
		CopyMem(ptr, OldData, OldDataSize);
		CopyMem(ptr, &MokNum, sizeof(MokNum));
		ptr += OldDataSize;
	} else {
		CopyMem(ptr, &MokNum, sizeof(MokNum));
		ptr += sizeof(MokNum);
	}

	/* Write new MOK */
	CopyMem(ptr, &MokSize, sizeof(MokSize));
	ptr += sizeof(MokSize);
	CopyMem(ptr, Mok, MokSize);

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokList",
				       &shim_lock_guid,
				       EFI_VARIABLE_NON_VOLATILE
				       | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				       DataSize, Data);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set variable %d\n", efi_status);
		goto error;
	}

error:
	if (Data)
		FreePool(Data);

	return efi_status;
}

static EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINTN MokSize = 0, MokListDataSize = 0;
	void *Mok = NULL, *MokListData = NULL;
	UINT32 MokNum = 0;
	UINT32 attributes;
	MokListNode *list = NULL;
	UINT8 confirmed;

	efi_status = get_variable(L"MokNew", shim_lock_guid, &attributes,
				  &MokSize, &Mok);

	if (efi_status != EFI_SUCCESS) {
		goto error;
	}

	efi_status = get_variable(L"MokList", shim_lock_guid, &attributes,
				  &MokListDataSize, &MokListData);

	if (efi_status == EFI_SUCCESS && MokListData) {
		int i;

		if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
			Print(L"MokList is compromised!\nErase all keys in MokList!\n");
			if (delete_variable(L"MokList", shim_lock_guid) != EFI_SUCCESS) {
				Print(L"Failed to erase MokList\n");
			}
			goto error;
		}

		CopyMem(&MokNum, MokListData, sizeof(UINT32));
		list = build_mok_list(MokNum,
				      (void *)MokListData + sizeof(UINT32),
				      MokListDataSize - sizeof(UINT32));

		if (!list) {
			Print(L"Failed to construct MOK list\n");
			goto error;
		}

		/* check if the key is already enrolled */
		for (i = 0; i < MokNum; i++) {
			if (list[i].MokSize == MokSize &&
			    CompareMem(list[i].Mok, Mok, MokSize) == 0) {
				Print(L"MOK was already enrolled\n");
				goto error;
			}
		}
	}

	confirmed = mok_enrollment_prompt(Mok, MokSize);

	if (!confirmed)
		goto error;

	efi_status = enroll_mok(Mok, MokSize, MokListData,
				MokListDataSize, MokNum);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to enroll MOK\n");
		goto error;
	}

	mok_mgmt_shell();

error:
	if (Mok) {
		if (delete_variable(L"MokNew", shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to delete MokNew\n");
		}
		FreePool (Mok);
	}

	if (list)
		FreePool (list);

	if (MokListData)
		FreePool (MokListData);

	return EFI_SUCCESS;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS efi_status;

	InitializeLib(image_handle, systab);

	efi_status = check_mok_request(image_handle);

	return efi_status;
}
