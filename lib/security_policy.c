/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * Install and remove a platform security2 override policy
 */

#include <efi.h>
#include <efilib.h>

#include <guid.h>
#include <variables.h>
#include <simple_file.h>
#include <errors.h>

#if defined(OVERRIDE_SECURITY_POLICY)
#include <security_policy.h>

/*
 * See the UEFI Platform Initialization manual (Vol2: DXE) for this
 */
struct _EFI_SECURITY2_PROTOCOL;
struct _EFI_SECURITY_PROTOCOL;
typedef struct _EFI_SECURITY2_PROTOCOL EFI_SECURITY2_PROTOCOL;
typedef struct _EFI_SECURITY_PROTOCOL EFI_SECURITY_PROTOCOL;
typedef EFI_DEVICE_PATH EFI_DEVICE_PATH_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SECURITY_FILE_AUTHENTICATION_STATE) (
			const EFI_SECURITY_PROTOCOL *This,
			UINT32 AuthenticationStatus,
			const EFI_DEVICE_PATH_PROTOCOL *File
								     );
typedef EFI_STATUS (EFIAPI *EFI_SECURITY2_FILE_AUTHENTICATION) (
			const EFI_SECURITY2_PROTOCOL *This,
			const EFI_DEVICE_PATH_PROTOCOL *DevicePath,
			VOID *FileBuffer,
			UINTN FileSize,
			BOOLEAN	BootPolicy
								     );

struct _EFI_SECURITY2_PROTOCOL {
	EFI_SECURITY2_FILE_AUTHENTICATION FileAuthentication;
};

struct _EFI_SECURITY_PROTOCOL {
	EFI_SECURITY_FILE_AUTHENTICATION_STATE  FileAuthenticationState;
};


static UINT8 *security_policy_esl = NULL;
static UINTN security_policy_esl_len;
static SecurityHook extra_check = NULL;

static EFI_SECURITY_FILE_AUTHENTICATION_STATE esfas = NULL;
static EFI_SECURITY2_FILE_AUTHENTICATION es2fa = NULL;

extern EFI_STATUS thunk_security_policy_authentication(
	const EFI_SECURITY_PROTOCOL *This,
	UINT32 AuthenticationStatus,
	const EFI_DEVICE_PATH_PROTOCOL *DevicePath
						       ) 
__attribute__((unused));

extern EFI_STATUS thunk_security2_policy_authentication(
	const EFI_SECURITY2_PROTOCOL *This,
	const EFI_DEVICE_PATH_PROTOCOL *DevicePath,
	VOID *FileBuffer,
	UINTN FileSize,
	BOOLEAN	BootPolicy
						       ) 
__attribute__((unused));

static __attribute__((used)) EFI_STATUS
security2_policy_authentication (
	const EFI_SECURITY2_PROTOCOL *This,
	const EFI_DEVICE_PATH_PROTOCOL *DevicePath,
	VOID *FileBuffer,
	UINTN FileSize,
	BOOLEAN	BootPolicy
				 )
{
	EFI_STATUS status, auth;

	/* Chain original security policy */

	status = uefi_call_wrapper(es2fa, 5, This, DevicePath, FileBuffer,
				   FileSize, BootPolicy);

	/* if OK, don't bother with MOK check */
	if (status == EFI_SUCCESS)
		return status;

	if (extra_check)
		auth = extra_check(FileBuffer, FileSize);
	else
		return EFI_SECURITY_VIOLATION;

	if (auth == EFI_SECURITY_VIOLATION || auth == EFI_ACCESS_DENIED)
		/* return previous status, which is the correct one
		 * for the platform: may be either EFI_ACCESS_DENIED
		 * or EFI_SECURITY_VIOLATION */
		return status;

	return auth;
}

static __attribute__((used)) EFI_STATUS
security_policy_authentication (
	const EFI_SECURITY_PROTOCOL *This,
	UINT32 AuthenticationStatus,
	const EFI_DEVICE_PATH_PROTOCOL *DevicePathConst
	)
{
	EFI_STATUS status, fail_status;
	EFI_DEVICE_PATH *DevPath 
		= DuplicateDevicePath((EFI_DEVICE_PATH *)DevicePathConst),
		*OrigDevPath = DevPath;
	EFI_HANDLE h;
	EFI_FILE *f;
	VOID *FileBuffer;
	UINTN FileSize;
	CHAR16* DevPathStr;

	/* Chain original security policy */
	status = uefi_call_wrapper(esfas, 3, This, AuthenticationStatus,
				   DevicePathConst);

	/* if OK avoid checking MOK: It's a bit expensive to
	 * read the whole file in again (esfas already did this) */
	if (status == EFI_SUCCESS)
		goto out;

	/* capture failure status: may be either EFI_ACCESS_DENIED or
	 * EFI_SECURITY_VIOLATION */
	fail_status = status;

	status = uefi_call_wrapper(BS->LocateDevicePath, 3,
				   &SIMPLE_FS_PROTOCOL, &DevPath, &h);
	if (status != EFI_SUCCESS)
		goto out;

	DevPathStr = DevicePathToStr(DevPath);

	status = simple_file_open_by_handle(h, DevPathStr, &f,
					    EFI_FILE_MODE_READ);
	FreePool(DevPathStr);
	if (status != EFI_SUCCESS)
		goto out;

	status = simple_file_read_all(f, &FileSize, &FileBuffer);
	simple_file_close(f);
	if (status != EFI_SUCCESS)
		goto out;

	if (extra_check)
		status = extra_check(FileBuffer, FileSize);
	else
		status = EFI_SECURITY_VIOLATION;
	FreePool(FileBuffer);

	if (status == EFI_ACCESS_DENIED || status == EFI_SECURITY_VIOLATION)
		/* return what the platform originally said */
		status = fail_status;
 out:
	FreePool(OrigDevPath);
	return status;
}


/* Nasty: ELF and EFI have different calling conventions.  Here is the map for
 * calling ELF -> EFI
 *
 *   1) rdi -> rcx (32 saved)
 *   2) rsi -> rdx (32 saved)
 *   3) rdx -> r8 ( 32 saved)
 *   4) rcx -> r9 (32 saved)
 *   5) r8 -> 32(%rsp) (48 saved)
 *   6) r9 -> 40(%rsp) (48 saved)
 *   7) pad+0(%rsp) -> 48(%rsp) (64 saved)
 *   8) pad+8(%rsp) -> 56(%rsp) (64 saved)
 *   9) pad+16(%rsp) -> 64(%rsp) (80 saved)
 *  10) pad+24(%rsp) -> 72(%rsp) (80 saved)
 *  11) pad+32(%rsp) -> 80(%rsp) (96 saved)

 *
 * So for a five argument callback, the map is ignore the first two arguments
 * and then map (EFI -> ELF) assuming pad = 0.
 *
 * ARG4  -> ARG1
 * ARG3  -> ARG2
 * ARG5  -> ARG3
 * ARG6  -> ARG4
 * ARG11 -> ARG5
 *
 * Calling conventions also differ over volatile and preserved registers in
 * MS: RBX, RBP, RDI, RSI, R12, R13, R14, and R15 are considered nonvolatile .
 * In ELF: Registers %rbp, %rbx and %r12 through %r15 “belong” to the calling
 * function and the called function is required to preserve their values.
 *
 * This means when accepting a function callback from MS -> ELF, we have to do
 * separate preservation on %rdi, %rsi before swizzling the arguments and
 * handing off to the ELF function.
 */

asm (
".type security2_policy_authentication,@function\n"
"thunk_security2_policy_authentication:\n\t"
	"mov	0x28(%rsp), %r10	# ARG5\n\t"
	"push	%rdi\n\t"
	"push	%rsi\n\t"
	"mov	%r10, %rdi\n\t"
	"subq	$8, %rsp	# space for storing stack pad\n\t"
	"mov	$0x08, %rax\n\t"
	"mov	$0x10, %r10\n\t"
	"and	%rsp, %rax\n\t"
	"cmovnz	%rax, %r11\n\t"
	"cmovz	%r10, %r11\n\t"
	"subq	%r11, %rsp\n\t"
	"addq	$8, %r11\n\t"
	"mov	%r11, (%rsp)\n\t"
"# five argument swizzle\n\t"
	"mov	%rdi, %r10\n\t"
	"mov	%rcx, %rdi\n\t"
	"mov	%rdx, %rsi\n\t"
	"mov	%r8, %rdx\n\t"
	"mov	%r9, %rcx\n\t"
	"mov	%r10, %r8\n\t"
	"callq	security2_policy_authentication@PLT\n\t"
	"mov	(%rsp), %r11\n\t"
	"addq	%r11, %rsp\n\t"
	"pop	%rsi\n\t"
	"pop	%rdi\n\t"
	"ret\n"
);

asm (
".type security_policy_authentication,@function\n"
"thunk_security_policy_authentication:\n\t"
	"push	%rdi\n\t"
	"push	%rsi\n\t"
	"subq	$8, %rsp	# space for storing stack pad\n\t"
	"mov	$0x08, %rax\n\t"
	"mov	$0x10, %r10\n\t"
	"and	%rsp, %rax\n\t"
	"cmovnz	%rax, %r11\n\t"
	"cmovz	%r10, %r11\n\t"
	"subq	%r11, %rsp\n\t"
	"addq	$8, %r11\n\t"
	"mov	%r11, (%rsp)\n\t"
"# three argument swizzle\n\t"
	"mov	%rcx, %rdi\n\t"
	"mov	%rdx, %rsi\n\t"
	"mov	%r8, %rdx\n\t"
	"callq	security_policy_authentication@PLT\n\t"
	"mov	(%rsp), %r11\n\t"
	"addq	%r11, %rsp\n\t"
	"pop	%rsi\n\t"
	"pop	%rdi\n\t"
	"ret\n"
);

EFI_STATUS
security_policy_install(SecurityHook hook)
{
	EFI_SECURITY_PROTOCOL *security_protocol;
	EFI_SECURITY2_PROTOCOL *security2_protocol = NULL;
	EFI_STATUS status;

	if (esfas)
		/* Already Installed */
		return EFI_ALREADY_STARTED;

	/* Don't bother with status here.  The call is allowed
	 * to fail, since SECURITY2 was introduced in PI 1.2.1
	 * If it fails, use security2_protocol == NULL as indicator */
	uefi_call_wrapper(BS->LocateProtocol, 3,
			  &SECURITY2_PROTOCOL_GUID, NULL,
			  (VOID **) &security2_protocol);

	status = uefi_call_wrapper(BS->LocateProtocol, 3,
				   &SECURITY_PROTOCOL_GUID, NULL,
				   (VOID **) &security_protocol);
	if (status != EFI_SUCCESS)
		/* This one is mandatory, so there's a serious problem */
		return status;

	if (security2_protocol) {
		es2fa = security2_protocol->FileAuthentication;
		security2_protocol->FileAuthentication = 
			(EFI_SECURITY2_FILE_AUTHENTICATION) thunk_security2_policy_authentication;
	}

	esfas = security_protocol->FileAuthenticationState;
	security_protocol->FileAuthenticationState =
		(EFI_SECURITY_FILE_AUTHENTICATION_STATE) thunk_security_policy_authentication;

	if (hook)
		extra_check = hook;

	return EFI_SUCCESS;
}

EFI_STATUS
security_policy_uninstall(void)
{
	EFI_STATUS status;

	if (esfas) {
		EFI_SECURITY_PROTOCOL *security_protocol;

		status = uefi_call_wrapper(BS->LocateProtocol, 3,
					   &SECURITY_PROTOCOL_GUID, NULL,
					   (VOID **) &security_protocol);

		if (status != EFI_SUCCESS)
			return status;

		security_protocol->FileAuthenticationState = esfas;
		esfas = NULL;
	} else {
		/* nothing installed */
		return EFI_NOT_STARTED;
	}

	if (es2fa) {
		EFI_SECURITY2_PROTOCOL *security2_protocol;

		status = uefi_call_wrapper(BS->LocateProtocol, 3,
					   &SECURITY2_PROTOCOL_GUID, NULL,
					   (VOID **) &security2_protocol);

		if (status != EFI_SUCCESS)
			return status;

		security2_protocol->FileAuthentication = es2fa;
		es2fa = NULL;
	}

	if (extra_check)
		extra_check = NULL;

	return EFI_SUCCESS;
}

void
security_protocol_set_hashes(unsigned char *esl, int len)
{
	security_policy_esl = esl;
	security_policy_esl_len = len;
}
#endif /* OVERRIDE_SECURITY_POLICY */
