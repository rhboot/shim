// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * memattrs.c - EFI and DXE memory attribute helpers
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

static inline uint64_t
shim_mem_attrs_to_uefi_mem_attrs (uint64_t attrs)
{
	uint64_t ret = EFI_MEMORY_RP |
		       EFI_MEMORY_RO |
		       EFI_MEMORY_XP;

	if (attrs & MEM_ATTR_R)
		ret &= ~EFI_MEMORY_RP;

	if (attrs & MEM_ATTR_W)
		ret &= ~EFI_MEMORY_RO;

	if (attrs & MEM_ATTR_X)
		ret &= ~EFI_MEMORY_XP;

	return ret;
}

static inline uint64_t
uefi_mem_attrs_to_shim_mem_attrs (uint64_t attrs)
{
	uint64_t ret = MEM_ATTR_R |
		       MEM_ATTR_W |
		       MEM_ATTR_X;

	if (attrs & EFI_MEMORY_RP)
		ret &= ~MEM_ATTR_R;

	if (attrs & EFI_MEMORY_RO)
		ret &= ~MEM_ATTR_W;

	if (attrs & EFI_MEMORY_XP)
		ret &= ~MEM_ATTR_X;

	return ret;
}

static void
get_dxe_services_table(EFI_DXE_SERVICES_TABLE **dstp)
{
	static EFI_DXE_SERVICES_TABLE *dst = NULL;

	if (dst == NULL) {
		dprint(L"Looking for configuration table " LGUID_FMT L"\n", GUID_ARGS(gEfiDxeServicesTableGuid));

		for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
			EFI_CONFIGURATION_TABLE *ct = &ST->ConfigurationTable[i];
			dprint(L"Testing configuration table " LGUID_FMT L"\n", GUID_ARGS(ct->VendorGuid));
			if (CompareMem(&ct->VendorGuid, &gEfiDxeServicesTableGuid, sizeof(EFI_GUID)) != 0)
				continue;

			dst = (EFI_DXE_SERVICES_TABLE *)ct->VendorTable;
			dprint(L"Looking for DXE Services Signature 0x%16llx, found signature 0x%16llx\n",
			       EFI_DXE_SERVICES_TABLE_SIGNATURE, dst->Hdr.Signature);
			if (dst->Hdr.Signature != EFI_DXE_SERVICES_TABLE_SIGNATURE)
				continue;

			if (!dst->GetMemorySpaceDescriptor || !dst->SetMemorySpaceAttributes) {
				/*
				 * purposefully not treating this as an error so that HSIStatus
				 * can tell us about it later.
				 */
				dprint(L"DXE Services lacks Get/SetMemorySpace* functions\n");
			}

			dprint(L"Setting dxe_services_table to 0x%llx\n", dst);
			*dstp = dst;
			return;
		}
	} else {
		*dstp = dst;
		return;
	}

	dst = NULL;
	dprint(L"Couldn't find DXE services\n");
}

static EFI_STATUS
dxe_get_mem_attrs(uintptr_t physaddr, size_t size, uint64_t *attrs)
{
	EFI_STATUS status;
	EFI_GCD_MEMORY_SPACE_DESCRIPTOR desc;
	EFI_PHYSICAL_ADDRESS start, end, next;
	EFI_DXE_SERVICES_TABLE *dst = NULL;

	get_dxe_services_table(&dst);
	if (!dst)
		return EFI_UNSUPPORTED;

	if (!dst->GetMemorySpaceDescriptor || !dst->SetMemorySpaceAttributes)
		return EFI_UNSUPPORTED;

	if (!IS_PAGE_ALIGNED(physaddr) || !IS_PAGE_ALIGNED(size) || size == 0 || attrs == NULL) {
		dprint(L"%a called on 0x%llx-0x%llx and attrs 0x%llx\n",
		       __func__, (unsigned long long)physaddr,
		       (unsigned long long)(physaddr+size-1),
		       attrs);
		return EFI_SUCCESS;
	}

	start = ALIGN_DOWN(physaddr, EFI_PAGE_SIZE);
	end = ALIGN_UP(physaddr + size, EFI_PAGE_SIZE);

	for (; start < end; start = next) {
		status = dst->GetMemorySpaceDescriptor(start, &desc);
		if (EFI_ERROR(status)) {
			dprint(L"GetMemorySpaceDescriptor(0x%llx, ...): %r\n",
			       start, status);
			return status;
		}

		next = desc.BaseAddress + desc.Length;

		if (desc.GcdMemoryType != EFI_GCD_MEMORY_TYPE_SYSTEM_MEMORY)
			continue;

		*attrs = uefi_mem_attrs_to_shim_mem_attrs(desc.Attributes);
		return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}

static EFI_STATUS
dxe_update_mem_attrs(uintptr_t addr, size_t size,
		     uint64_t set_attrs, uint64_t clear_attrs)
{
#if 0
	EFI_STATUS status;
	EFI_GCD_MEMORY_SPACE_DESCRIPTOR desc;
	EFI_PHYSICAL_ADDRESS start, end, next;
	uint64_t before = 0, after = 0, dxe_set_attrs, dxe_clear_attrs;
#endif
	EFI_DXE_SERVICES_TABLE *dst = NULL;

	get_dxe_services_table(&dst);
	if (!dst)
		return EFI_UNSUPPORTED;

	if (!dst->GetMemorySpaceDescriptor || !dst->SetMemorySpaceAttributes)
		return EFI_UNSUPPORTED;

	if (!IS_PAGE_ALIGNED(addr) || !IS_PAGE_ALIGNED(size) || size == 0) {
		perror(L"Invalid call %a(addr:0x%llx-0x%llx, size:0x%llx, +%a%a%a, -%a%a%a)\n",
		       __func__, (unsigned long long)addr,
		       (unsigned long long)(addr + size - 1),
		       (unsigned long long)size,
		       (set_attrs & MEM_ATTR_R) ? "r" : "",
		       (set_attrs & MEM_ATTR_W) ? "w" : "",
		       (set_attrs & MEM_ATTR_X) ? "x" : "",
		       (clear_attrs & MEM_ATTR_R) ? "r" : "",
		       (clear_attrs & MEM_ATTR_W) ? "w" : "",
		       (clear_attrs & MEM_ATTR_X) ? "x" : "");
		if (!IS_PAGE_ALIGNED(addr))
			perror(L" addr is not page aligned\n");
		if (!IS_PAGE_ALIGNED(size))
			perror(L" size is not page aligned\n");
		if (size == 0)
			perror(L" size is 0\n");
		return EFI_SUCCESS;
	}

	/*
	 * We know this only works coincidentally, so nerfing it for now
	 * until we have a chance to debug more thoroughly on these niche
	 * systems.
	 */
#if 0
	start = ALIGN_DOWN(addr, EFI_PAGE_SIZE);
	end = ALIGN_UP(addr + size, EFI_PAGE_SIZE);

	for (; start < end; start = next) {
		EFI_PHYSICAL_ADDRESS mod_start;
		UINT64 mod_size;

		status = dst->GetMemorySpaceDescriptor(start, &desc);
		if (EFI_ERROR(status)) {
			dprint(L"GetMemorySpaceDescriptor(0x%llx, ...): %r\n",
			       start, status);
			return status;
		}

		next = desc.BaseAddress + desc.Length;

		if (desc.GcdMemoryType != EFI_GCD_MEMORY_TYPE_SYSTEM_MEMORY)
			continue;

		mod_start = MAX(start, desc.BaseAddress);
		mod_size = MIN(end, next) - mod_start;

		before = uefi_mem_attrs_to_shim_mem_attrs(desc.Attributes);
		dxe_set_attrs = shim_mem_attrs_to_uefi_mem_attrs(set_attrs);
		dprint("translating set_attrs from 0x%lx to 0x%lx\n", set_attrs, dxe_set_attrs);
		dxe_clear_attrs = shim_mem_attrs_to_uefi_mem_attrs(clear_attrs);
		dprint("translating clear_attrs from 0x%lx to 0x%lx\n", clear_attrs, dxe_clear_attrs);
		desc.Attributes |= dxe_set_attrs;
		desc.Attributes &= ~dxe_clear_attrs;
		after = uefi_mem_attrs_to_shim_mem_attrs(desc.Attributes);

		status = dst->SetMemorySpaceAttributes(mod_start, mod_size, desc.Attributes);
		if (EFI_ERROR(status)) {
			dprint(L"Failed to update memory attrs:0x%0x addr:0x%llx size:0x%0lx status:%r\n",
				desc.Attributes, mod_start, mod_size, status);
			return status;
		}

		break;
	}

	dprint(L"set +%a%a%a -%a%a%a on 0x%llx-0x%llx before:%c%c%c after:%c%c%c\n",
	       (set_attrs & MEM_ATTR_R) ? "r" : "",
	       (set_attrs & MEM_ATTR_W) ? "w" : "",
	       (set_attrs & MEM_ATTR_X) ? "x" : "",
	       (clear_attrs & MEM_ATTR_R) ? "r" : "",
	       (clear_attrs & MEM_ATTR_W) ? "w" : "",
	       (clear_attrs & MEM_ATTR_X) ? "x" : "",
	       (unsigned long long)addr, (unsigned long long)(addr + size - 1),
	       (before & MEM_ATTR_R) ? 'r' : '-',
	       (before & MEM_ATTR_W) ? 'w' : '-',
	       (before & MEM_ATTR_X) ? 'x' : '-',
	       (after & MEM_ATTR_R) ? 'r' : '-',
	       (after & MEM_ATTR_W) ? 'w' : '-',
	       (after & MEM_ATTR_X) ? 'x' : '-');
#endif

	return EFI_SUCCESS;
}

static void
get_efi_mem_attr_protocol(EFI_MEMORY_ATTRIBUTE_PROTOCOL **protop)
{
	static EFI_MEMORY_ATTRIBUTE_PROTOCOL *proto = NULL;
	static bool has_mem_access_proto = true;

	if (proto == NULL && has_mem_access_proto == true) {
		EFI_STATUS efi_status;
		efi_status = LibLocateProtocol(&EFI_MEMORY_ATTRIBUTE_PROTOCOL_GUID,
					       (VOID **)&proto);
		if (EFI_ERROR(efi_status) || !proto) {
			has_mem_access_proto = false;
			*protop = NULL;
		}
	}

	*protop = proto;
}

static EFI_STATUS
efi_get_mem_attrs(uintptr_t addr, size_t size, uint64_t *attrs)
{
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *proto = NULL;
	EFI_PHYSICAL_ADDRESS physaddr = addr;
	EFI_STATUS efi_status;

	get_efi_mem_attr_protocol(&proto);
	if (!proto)
		return EFI_UNSUPPORTED;

	if (!IS_PAGE_ALIGNED(physaddr) || !IS_PAGE_ALIGNED(size) || size == 0 || attrs == NULL) {
		dprint(L"%a called on 0x%llx-0x%llx and attrs 0x%llx\n",
		       __func__, (unsigned long long)physaddr,
		       (unsigned long long)(physaddr+size-1),
		       attrs);
		return EFI_SUCCESS;
	}

	efi_status = proto->GetMemoryAttributes(proto, physaddr, size, attrs);
	if (EFI_ERROR(efi_status)) {
		dprint(L"GetMemoryAttributes(..., 0x%llx, 0x%x, 0x%x): %r\n",
		       physaddr, size, attrs, efi_status);
	} else {
		*attrs = uefi_mem_attrs_to_shim_mem_attrs (*attrs);
	}

	return efi_status;
}

static EFI_STATUS
efi_update_mem_attrs(uintptr_t addr, uint64_t size,
		     uint64_t set_attrs, uint64_t clear_attrs)
{
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *proto = NULL;
	EFI_PHYSICAL_ADDRESS physaddr = addr;
	EFI_STATUS efi_status, ret;
	uint64_t before = 0, after = 0, uefi_set_attrs, uefi_clear_attrs;

	get_efi_mem_attr_protocol(&proto);
	if (!proto)
		return EFI_UNSUPPORTED;

	efi_status = efi_get_mem_attrs(addr, size, &before);
	if (EFI_ERROR(efi_status))
		dprint(L"efi_get_mem_attrs(0x%llx, 0x%llx, 0x%llx) -> 0x%lx\n",
		       (unsigned long long)addr, (unsigned long long)size,
		       &before, efi_status);

	if (!IS_PAGE_ALIGNED(physaddr) || !IS_PAGE_ALIGNED(size) || size == 0) {
		perror(L"Invalid call %a(addr:0x%llx-0x%llx, size:0x%llx, +%a%a%a, -%a%a%a)\n",
		       __func__, (unsigned long long)physaddr,
		       (unsigned long long)(physaddr + size - 1),
		       (unsigned long long)size,
		       (set_attrs & MEM_ATTR_R) ? "r" : "",
		       (set_attrs & MEM_ATTR_W) ? "w" : "",
		       (set_attrs & MEM_ATTR_X) ? "x" : "",
		       (clear_attrs & MEM_ATTR_R) ? "r" : "",
		       (clear_attrs & MEM_ATTR_W) ? "w" : "",
		       (clear_attrs & MEM_ATTR_X) ? "x" : "");
		if (!IS_PAGE_ALIGNED(physaddr))
			perror(L" addr is not page aligned\n");
		if (!IS_PAGE_ALIGNED(size))
			perror(L" size is not page aligned\n");
		if (size == 0)
			perror(L" size is 0\n");
		return EFI_SUCCESS;
	}

	uefi_set_attrs = shim_mem_attrs_to_uefi_mem_attrs (set_attrs);
	dprint("translating set_attrs from 0x%lx to 0x%lx\n", set_attrs, uefi_set_attrs);
	uefi_clear_attrs = shim_mem_attrs_to_uefi_mem_attrs (clear_attrs);
	dprint("translating clear_attrs from 0x%lx to 0x%lx\n", clear_attrs, uefi_clear_attrs);
	efi_status = EFI_SUCCESS;
	if (uefi_set_attrs) {
		efi_status = proto->SetMemoryAttributes(proto, physaddr, size, uefi_set_attrs);
		if (EFI_ERROR(efi_status)) {
			dprint(L"Failed to set memory attrs:0x%0x physaddr:0x%llx size:0x%0lx status:%r\n",
				uefi_set_attrs, physaddr, size, efi_status);
		}
	}
	if (!EFI_ERROR(efi_status) && uefi_clear_attrs) {
		efi_status = proto->ClearMemoryAttributes(proto, physaddr, size, uefi_clear_attrs);
		if (EFI_ERROR(efi_status)) {
			dprint(L"Failed to clear memory attrs:0x%0x physaddr:0x%llx size:0x%0lx status:%r\n",
				uefi_clear_attrs, physaddr, size, efi_status);
		}
	}
	ret = efi_status;

	efi_status = efi_get_mem_attrs(addr, size, &after);
	if (EFI_ERROR(efi_status))
		dprint(L"efi_get_mem_attrs(0x%llx, %llu, 0x%llx) -> 0x%lx\n",
		       (unsigned long long)addr, (unsigned long long)size,
		       &after, efi_status);

	dprint(L"set +%a%a%a -%a%a%a on 0x%llx-0x%llx before:%c%c%c after:%c%c%c\n",
	       (set_attrs & MEM_ATTR_R) ? "r" : "",
	       (set_attrs & MEM_ATTR_W) ? "w" : "",
	       (set_attrs & MEM_ATTR_X) ? "x" : "",
	       (clear_attrs & MEM_ATTR_R) ? "r" : "",
	       (clear_attrs & MEM_ATTR_W) ? "w" : "",
	       (clear_attrs & MEM_ATTR_X) ? "x" : "",
	       (unsigned long long)addr, (unsigned long long)(addr + size - 1),
	       (before & MEM_ATTR_R) ? 'r' : '-',
	       (before & MEM_ATTR_W) ? 'w' : '-',
	       (before & MEM_ATTR_X) ? 'x' : '-',
	       (after & MEM_ATTR_R) ? 'r' : '-',
	       (after & MEM_ATTR_W) ? 'w' : '-',
	       (after & MEM_ATTR_X) ? 'x' : '-');

	return ret;
}

EFI_STATUS
update_mem_attrs(uintptr_t addr, uint64_t size,
		 uint64_t set_attrs, uint64_t clear_attrs)
{
	EFI_STATUS efi_status;

	efi_status = efi_update_mem_attrs(addr, size, set_attrs, clear_attrs);
	if (!EFI_ERROR(efi_status))
		return efi_status;

	if (efi_status == EFI_UNSUPPORTED)
		efi_status = dxe_update_mem_attrs(addr, size, set_attrs, clear_attrs);

	return efi_status;
}

EFI_STATUS
get_mem_attrs(uintptr_t addr, size_t size, uint64_t *attrs)
{
	EFI_STATUS efi_status;

	efi_status = efi_get_mem_attrs(addr, size, attrs);
	if (!EFI_ERROR(efi_status))
		return efi_status;

	if (efi_status == EFI_UNSUPPORTED)
		efi_status = dxe_get_mem_attrs(addr, size, attrs);

	return efi_status;
}

char *
decode_hsi_bits(UINTN hsi)
{
	static const struct {
		UINTN bit;
		char name[16];
	} bits[] = {
		{.bit = SHIM_HSI_STATUS_HEAPX, .name = "HEAPX"},
		{.bit = SHIM_HSI_STATUS_STACKX, .name = "STACKX"},
		{.bit = SHIM_HSI_STATUS_ROW, .name = "ROW"},
		{.bit = SHIM_HSI_STATUS_HASMAP, .name = "HASMAP"},
		{.bit = SHIM_HSI_STATUS_HASDST, .name = "HASDST"},
		{.bit = SHIM_HSI_STATUS_HASDSTGMSD, .name = "HASDSTGMSD"},
		{.bit = SHIM_HSI_STATUS_HASDSTSMSA, .name = "HASDSTSMSA"},
		{.bit = SHIM_HSI_STATUS_NX, .name = "NX"},
		{.bit = 0, .name = ""},
	};
	static int x = 0;
	static char retbufs[2][sizeof(bits)];
	char *retbuf = &retbufs[x % 2][0];
	char *prev = &retbuf[0];
	char *pos = &retbuf[0];

	x = ( x + 1 ) % 2;

	ZeroMem(retbuf, sizeof(bits));

	if (hsi == 0) {
		prev = stpcpy(retbuf, "0");
	} else {
		for (UINTN i = 0; bits[i].bit != 0; i++) {
			if (hsi & bits[i].bit) {
				prev = stpcpy(pos, bits[i].name);
				pos = stpcpy(prev, "|");
			}
		}
	}
	prev[0] = '\0';
	return retbuf;
}

void
get_hsi_mem_info(void)
{
	EFI_STATUS efi_status;
	uintptr_t addr;
	uint64_t attrs = 0;
	uint32_t *tmp_alloc;
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *efiproto = NULL;
	EFI_DXE_SERVICES_TABLE *dst = NULL;

	get_efi_mem_attr_protocol(&efiproto);
	if (efiproto) {
		dprint(L"Setting HSI from %a to %a\n",
		       decode_hsi_bits(hsi_status),
		       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_HASMAP));
		hsi_status |= SHIM_HSI_STATUS_HASMAP;
	}

	get_dxe_services_table(&dst);
	if (dst) {
		dprint(L"Setting HSI from %a to %a\n",
		       decode_hsi_bits(hsi_status),
		       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_HASDST));
		hsi_status |= SHIM_HSI_STATUS_HASDST;
		if (dst->GetMemorySpaceDescriptor) {
			dprint(L"Setting HSI from %a to %a\n",
			       decode_hsi_bits(hsi_status),
			       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_HASDSTGMSD));
			hsi_status |= SHIM_HSI_STATUS_HASDSTGMSD;
		}
		if (dst->SetMemorySpaceAttributes) {
			dprint(L"Setting HSI from %a to %a\n",
			       decode_hsi_bits(hsi_status),
			       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_HASDSTSMSA));
			hsi_status |= SHIM_HSI_STATUS_HASDSTSMSA;
		}
	}

	if (!(hsi_status & SHIM_HSI_STATUS_HASMAP) &&
	    !(hsi_status & SHIM_HSI_STATUS_HASDSTGMSD &&
	      hsi_status & SHIM_HSI_STATUS_HASDSTSMSA)) {
		dprint(L"No memory protocol, not testing further\n");
		return;
	}

	addr = ((uintptr_t)&get_hsi_mem_info) & ~EFI_PAGE_MASK;
	efi_status = get_mem_attrs(addr, EFI_PAGE_SIZE, &attrs);
	if (EFI_ERROR(efi_status)) {
		dprint(L"get_mem_attrs(0x%016llx, 0x%x, &attrs) failed.\n", addr, EFI_PAGE_SIZE);
		goto error;
	}

	if (attrs & MEM_ATTR_W) {
		dprint(L"get_hsi_mem_info() is on a writable page: %a->%a\n",
		       decode_hsi_bits(hsi_status),
		       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_ROW));
		hsi_status |= SHIM_HSI_STATUS_ROW;
	}

	addr = ((uintptr_t)&addr) & ~EFI_PAGE_MASK;
	efi_status = get_mem_attrs(addr, EFI_PAGE_SIZE, &attrs);
	if (EFI_ERROR(efi_status)) {
		dprint(L"get_mem_attrs(0x%016llx, 0x%x, &attrs) failed.\n", addr, EFI_PAGE_SIZE);
		goto error;
	}

	if (attrs & MEM_ATTR_X) {
		dprint(L"Stack variable is on an executable page: %a->%a\n",
		       decode_hsi_bits(hsi_status),
		       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_STACKX));
		hsi_status |= SHIM_HSI_STATUS_STACKX;
	}

	tmp_alloc = AllocatePool(EFI_PAGE_SIZE);
	if (!tmp_alloc) {
		dprint(L"Failed to allocate heap variable.\n");
		goto error;
	}

	addr = ((uintptr_t)tmp_alloc) & ~EFI_PAGE_MASK;
	efi_status = get_mem_attrs(addr, EFI_PAGE_SIZE, &attrs);
	FreePool(tmp_alloc);
	if (EFI_ERROR(efi_status)) {
		dprint(L"get_mem_attrs(0x%016llx, 0x%x, &attrs) failed.\n", addr, EFI_PAGE_SIZE);
		goto error;
	}
	if (attrs & MEM_ATTR_X) {
		dprint(L"Heap variable is on an executable page: %a->%a\n",
		       decode_hsi_bits(hsi_status),
		       decode_hsi_bits(hsi_status | SHIM_HSI_STATUS_HEAPX));
		hsi_status |= SHIM_HSI_STATUS_HEAPX;
	}

	return;

error:
	/*
	 * In this case we can't actually tell anything, so assume
	 * and report the worst case scenario.
	 */
	hsi_status = SHIM_HSI_STATUS_HEAPX |
		     SHIM_HSI_STATUS_STACKX |
		     SHIM_HSI_STATUS_ROW;
	dprint(L"Setting HSI to 0x%lx due to error: %r\n", decode_hsi_bits(hsi_status), efi_status);
}

// vim:fenc=utf-8:tw=75:noet
