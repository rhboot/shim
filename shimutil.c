/*
 * shimutil - utilities running in UEFI shell for shim
 *
 * Copyright openSUSE, Inc
 * Author: Dennis Tseng
 * Date  : 2022/07/15
 */

#include "shimutil.h"

extern void PRINT_MESSAGE();
	
tCMD_FUNC cmd_tbl[] = {
	{ L"sb-state", secure_boot_state },
	{ L"sbat-state", sbat_state },
	{ L"show-mok", show_mok },
	{ L"", NULL }
};

EFI_GUID SHIM_GUID = { 0x605dab50, 0xe046, 0x4300, {
                       		0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };

UINT32 verbose=0;
BOOLEAN bSHOW_DER = FALSE;

/*-----------------------------------------------------------------
 * parse_argv
 *----------------------------------------------------------------*/
void parse_argv(CHAR16 *str, int len, CHAR16 *argv[], int *argc)
{
	int  i,vi=0;

	for(argv[vi]=str,i=0; i<len; i++) {
		if (str[i] == ' ') {
			str[i] = '\0';
			for(i++; str[i] == ' '; i++);
			vi++;
			argv[vi] = &str[i];
		}
	}
	str[i] = '\0';
	*argc = vi+1;
}

/********************************************************************
 * secure_boot_state
 * IN : argv
 * OUT: state
 * RET: efi-status
 ********************************************************************/
void help_func(void)
{
	Print(L"Usage:\n");
	Print(L"  shimutil OPTIONS <args...>\n");
	Print(L"Options:\n");
	Print(L"  sb-state: show secure-boot state\n");
	Print(L"  sbat-state [get/set] [latest/previous/delete]: apply sbat\n");
	Print(L"  show-mok db [der]: show key in list\n");
	Print(L"\n\n\n");
}

/********************************************************************
 * secure_boot_state
 * IN : argv
 * OUT: state
 * RET: efi-status
 ********************************************************************/
EFI_STATUS
secure_boot_state(CHAR16 *argv[], int argc)
{
	EFI_GUID GLOBAL_GUID = EFI_GLOBAL_VARIABLE;
	UINT8 SecureBoot = 0; /* return false if variable doesn't exist */
	UINTN DataSize;
	EFI_STATUS efi_status;

	if (argc != 2) {
		Print(L"Error! invalid parameter for %s\n",argv[1]);
		return EFI_INVALID_PARAMETER;
	}

	DataSize = sizeof(SecureBoot);
	SecureBoot = 0;
	efi_status = RT->GetVariable(L"SecureBoot", &GLOBAL_GUID, NULL,
			     &DataSize, &SecureBoot);
	if (EFI_ERROR(efi_status)) {
		Print(L"Error! cannot get Secure-boot variable\n");
		return efi_status;
	}
	(SecureBoot ? Print(L"Secure-boot is enabled\n") : Print(L"Secure-boot is disabled\n"));
	return efi_status;
}

/********************************************************************
 * sbat_state
 * IN : argv
 * IN : argc
 * OUT: state
 * RET: efi-status
 ********************************************************************/
EFI_STATUS
sbat_state(CHAR16 *argv[], int argc)
{
	char  		*sbat_var = NULL;
	EFI_STATUS 	efi_status;
	char		data[256];
	char		buffer[512];
	UINT32 		attributes;
	UINTN		len,i;

	if (argc < 3) {
		Print(L"Error! invalid parameter for %s\n",argv[1]);
		help_func();
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[2],L"get",StrLen(argv[2])) && StrnCmp(argv[2],L"set",StrLen(argv[2]))) {
		Print(L"Error! invalid parameter(%s)\n",argv[2]);
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[2],L"get",StrLen(argv[2])) == 0) {
		if (argc != 3) {
			Print(L"Error! invalid parameter for %s\n",argv[1]);
			return EFI_INVALID_PARAMETER;
		}

		efi_status = RT->GetVariable(L"SbatLevel", &SHIM_GUID, 
					&attributes, &len, data);
		if (EFI_ERROR(efi_status)) {
			Print(L"variable(%s) reading failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}

		Print(L"variable(%s) reading success: %r\n",SBAT_VAR_NAME,efi_status);
		Print(L"len=%d,variable=",len);
		for(i=0; i<len; i++) {
			Print(L"%c",data[i]);
		}
		Print(L"\n");
	}
	else if (StrnCmp(argv[2],L"set",StrLen(argv[2])) == 0) {
		if (argc != 4) {
			Print(L"Error! invalid parameter for %s\n",argv[1]);
			return EFI_INVALID_PARAMETER;
		}

		if (StrnCmp(argv[3],L"latest",StrLen(argv[3])) && 
		    StrnCmp(argv[3],L"previous",StrLen(argv[3])) && 
			StrnCmp(argv[3],L"delete",StrLen(argv[3]))) {
			Print(L"Error! invalid parameter(%s)\n",argv[3]);
			return EFI_INVALID_PARAMETER;
		}

		(StrnCmp(argv[3],L"latest",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_LATEST : (
		 StrnCmp(argv[3],L"previous",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_PREVIOUS : (
		 StrnCmp(argv[3],L"delete",StrLen(argv[3])) == 0 ? sbat_var = SBAT_VAR_ORIGINAL : NULL)));

		efi_status = RT->SetVariable(L"SbatLevel", &SHIM_GUID, 
				EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
	            strlen(sbat_var), sbat_var);
		if (EFI_ERROR(efi_status)) {
			Print(L"%s variable writing failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}
		Print(L"variable(%s)/len(%d) writing success: %r\n",
			SBAT_VAR_NAME,strlen(sbat_var),efi_status);

		//---------------- double check after writing ------------------
		efi_status = RT->GetVariable(L"SbatLevel", &SHIM_GUID, 
					&attributes, &len, buffer);
		if (EFI_ERROR(efi_status)) {
			Print(L"variable(%s) reading failed: %r\n",SBAT_VAR_NAME,efi_status);
			return efi_status;
		}

		Print(L"variable(%s) reading success: %r\n",SBAT_VAR_NAME,efi_status);
		Print(L"len=%d,variable=",len);
		for(i=0; i<len; i++) {
			Print(L"%c",buffer[i]);
		}
		Print(L"\n");	
	}

	return EFI_SUCCESS;
}

/*-------------------------------------------------------------------
 * print_pkcs_der
 * IN : pkcs-tree
 * out: data of pkcs-tree
 *------------------------------------------------------------------*/
void print_pkcs_der(tPKCS_NODE *node, UINTN tab)
{
	tPKCS_NODE *cur;
	UINTN		i;

	for(cur=node; cur; cur=cur->brother) {
		gBS->Stall(200000);
		for(i=0; i<tab; i++) {
			Print(L"   ");
		}

		Print(L"%02x %02x ",cur->Tag,cur->Len);
		if (cur->child) {
			Print(L"\n");
			print_pkcs_der(cur->child, tab+1);
		}
		else{
			for(i=0; i<cur->Len; i++) {
				Print(L"%02x ",*(unsigned char*)(cur->data+i));
			}
			Print(L"\n");
		}
	}
}


/*-------------------------------------------------------------------
 * print_pkcs_text
 * IN : pkcs-tree
 * out: data of pkcs-tree
 *------------------------------------------------------------------*/
void print_pkcs_text(tPKCS_NODE *node, UINTN x, UINTN y)
{
	tPKCS_NODE *cur;
	unsigned char *cp;
	UINTN		i;

	for(cur=node; cur; cur=cur->brother,x++) {
		if (x == 0 && y == 1) {
			Print(L"      Version : %d\n",(*(unsigned char*)cur->data)+1);
		}
		else if (x == 1 && y == 0) {
			Print(L"Serial Number : ");
			for(i=0; i<cur->Len; i++) {
				Print(L"%02x ",*((unsigned char*)cur->data+i));
			}
			Print(L"\n");
		}
		else if (x == 3 && y == 3) {
			Print(L"       Issuer : CN = ");
			for(i=0; cur->brother && i<cur->brother->Len; i++) {
				Print(L"%c",*((unsigned char*)cur->brother->data+i));
			}
			Print(L"\n");
		}
		else if (x == 4 && y == 1) {
			cp = (unsigned char*)cur->data;
			Print(L"     Validity : not before 20%d%d.%d%d.%d%d:%d%d.%d%d.%d%d GMT\n",
				*cp-0x30,*(cp+1)-0x30, //2022
				*(cp+2)-0x30,*(cp+3)-0x30, //Feb(02)
				*(cp+4)-0x30,*(cp+5)-0x30, //date(10)
				*(cp+6)-0x30,*(cp+7)-0x30, //hour(04)
				*(cp+8)-0x30,*(cp+9)-0x30, //min(02)
				*(cp+10)-0x30,*(cp+11)-0x30);//sec(33)

			if (cur->brother) {
				cp = (unsigned char*)cur->brother->data;
				Print(L"     Validity : not after 20%d%d.%d%d.%d%d:%d%d.%d%d.%d%d GMT\n",
				*cp-0x30,*(cp+1)-0x30, //2022
				*(cp+2)-0x30,*(cp+3)-0x30, //Feb(02)
				*(cp+4)-0x30,*(cp+5)-0x30, //date(10)
				*(cp+6)-0x30,*(cp+7)-0x30, //hour(04)
				*(cp+8)-0x30,*(cp+9)-0x30, //min(02)
				*(cp+10)-0x30,*(cp+11)-0x30);//sec(33)
			}
		}
		else if (x == 5 && y == 3) {
			Print(L"      Subject : CN = ");
			for(i=0; cur->brother && i<cur->brother->Len; i++) {
				Print(L"%c",*((unsigned char*)cur->brother->data+i));
			}
			Print(L"\n");
		}
		else if (cur->child) {
			print_pkcs_text(cur->child, x, y+1);
		}
	}
}

/*-------------------------------------------------------------------
 * free_pkcs_tree
 * IN : data
 * out: pkcs-tree
 *------------------------------------------------------------------*/
void free_pkcs_tree(tPKCS_NODE *node)
{
	if (node == NULL)  return;
	free_pkcs_tree(node->brother);
	if (node->child) {
		free_pkcs_tree(node->child);
	}
	FreePool(node);
}

/*-------------------------------------------------------------------
 * parse_pkcs_tree
 * IN : data
 * out: pkcs-tree
 *------------------------------------------------------------------*/
tPKCS_NODE *parse_pkcs_tree(tPKCS_NODE *root, unsigned char *data, UINTN len)
{
	tPKCS_NODE *prev,*node;
	unsigned char Tag,Len,len_len;
	UINTN i,j;

	for(prev=NULL,i=0; i<len; i+=Len+2,prev=node,data+=Len) {
		Tag = *data++;
		Len = *data++;
		
		node = AllocatePool(sizeof(tPKCS_NODE));
		node->Tag = Tag;
		node->Len = Len;
		node->data = data;
		node->brother = NULL;
		node->child = NULL;

		if (root == NULL) {
			root = node;
		}
		else{
			prev->brother = node;
		}

		if (Tag == (unsigned char)TAG_CS_A0 || 
			Tag == (unsigned char)TAG_SEQ_16 ||
			Tag == (unsigned char)TAG_SET_17) {
			if (Len & LEN_OF_LEN) {
				len_len = *data & 0x7F;
				for(data++,j=1,Len=*data++; j<len_len; j++,data++) {
					Len = Len << 8 | *data; 
				}
			}
			node->child = parse_pkcs_tree(node->child, data, Len-2);
		}
	}
	return root;
}

/********************************************************************
 * show_mok
 * IN : argv
 * IN : argc
 * OUT: keys
 * RET: efi-status
 ********************************************************************/
EFI_STATUS
show_mok(CHAR16 *argv[], int argc)
{
	tPKCS_NODE *root = NULL;
	EFI_STATUS efi_status;
	UINT32 	attributes;
	CHAR16 	*mok_name = L"MokList";
	UINTN	tot_len,size = 50000; //32892;
	UINTN	len,len_len,prelen,i,j,cnt;
	void 	*data=NULL;
	unsigned char *cp;

	if (argc < 3 || argc > 4) {
		Print(L"Error parameter number for %s\n",argv[1]);
		help_func();
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[1],L"show-mok",StrLen(argv[1]))) {
		Print(L"Unknown command for %s\n",argv[1]);
		help_func();
		return EFI_INVALID_PARAMETER;
	}

	if (StrnCmp(argv[2],L"db",StrLen(argv[2]))) {
		Print(L"Invalid parameter for %s\n",argv[2]);
		help_func();
		return EFI_INVALID_PARAMETER;
	}

	if (argc == 4) {
		if (StrnCmp(argv[3],L"der",StrLen(argv[3]))) {
			Print(L"Invalid parameter for %s\n",argv[3]);
			help_func();
			return EFI_INVALID_PARAMETER;
		}
		bSHOW_DER = TRUE;
	}

	data = AllocatePool(size);
	if (data == NULL) {
		Print(L"Error! out of resources when AllocatePoll()\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = RT->GetVariable(mok_name, &SHIM_GUID, &attributes, &tot_len, data);
	if (EFI_ERROR(efi_status)) {
		//if Buffer too small, then return EFI_BUFFER_TOO_SMALL(5)
		Print(L"%d> variable(MokList)/len(%d) reading failed: %r\n",
		__LINE__,tot_len,efi_status);
		goto out;
	}

	/* 
	 * skip Tag & Len of header
	 * 0x30 0x82  
     *      0x03 0x18 
     *           0x30 0x82 
     *                0x02 0x00 
	 */
	for(i=0,cnt=0,cp=(unsigned char*)data; cnt<2 && i<tot_len; i++) {
		if (*cp == (unsigned char)TAG_SEQ_16) {
			cp++;
			if ((*cp) & LEN_OF_LEN) {
				len_len = *cp & 0x7F;
				for(cp++,j=1,len=*cp++; j<len_len; j++,cp++) {
					len = len << 8 | *cp; 
				}
				cnt++;
				if (cnt == 2) {
					for(prelen=0; prelen<len; prelen++) {
						if (*(cp+prelen) == TAG_SEQ_16 && *(cp+prelen+1) & LEN_OF_LEN)  
							break;
					}
					root = parse_pkcs_tree(root,cp,prelen);
					
					if (bSHOW_DER) {
						print_pkcs_der(root,0);
					}
					else{
						print_pkcs_text(root,0,0);
					}
					break;
				}
			}
		}
		else{
			cp++;
		}
	}

out:
	FreePool(data);
	free_pkcs_tree(root);
	return efi_status;
}

/**************************************************************************
 * efi_main
 **************************************************************************/
EFI_STATUS
efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	EFI_LOADED_IMAGE *loaded_image;
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	CHAR16 *argstr,*argv[16];
	int  len,argc,cmd_idx;
	EFI_STATUS efi_status;

	/*
	 * Ensure that gnu-efi functions are available
	 */
	InitializeLib(image_handle, systab);

	efi_status = gBS->HandleProtocol(image_handle,&loaded_image_protocol,(void**)&loaded_image);
	argstr = (CHAR16*)loaded_image->LoadOptions;
	len = StrLen((CHAR16*)loaded_image->LoadOptions);
	parse_argv(argstr,len,argv,&argc);
	if (argc <= 1) {
		Print(L"Error! Please input enough parameters\n");
		help_func();
		return efi_status;
	}

	_CMD_TBL_IDX_(cmd_idx,argv[1],cmd_tbl);
	if (cmd_tbl[cmd_idx].cmd[0]) {
		cmd_tbl[cmd_idx].func(argv,argc);
	}
	else {
		Print(L"Error! unknown request(%s)\n",argv[1]);	
		help_func();
	}

	return efi_status;
}
