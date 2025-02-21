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

EFI_STATUS
get_mem_attrs (uintptr_t addr, size_t size, uint64_t *attrs)
{
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *proto = NULL;
	EFI_PHYSICAL_ADDRESS physaddr = addr;
	EFI_STATUS efi_status;

	efi_status = LibLocateProtocol(&EFI_MEMORY_ATTRIBUTE_PROTOCOL_GUID,
				       (VOID **)&proto);
	if (EFI_ERROR(efi_status) || !proto) {
		dprint(L"No memory attribute protocol found: %r\n", efi_status);
		if (!EFI_ERROR(efi_status))
			efi_status = EFI_UNSUPPORTED;
		return efi_status;
	}

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

EFI_STATUS
update_mem_attrs(uintptr_t addr, uint64_t size,
		 uint64_t set_attrs, uint64_t clear_attrs)
{
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *proto = NULL;
	EFI_PHYSICAL_ADDRESS physaddr = addr;
	EFI_STATUS efi_status, ret;
	uint64_t before = 0, after = 0, uefi_set_attrs, uefi_clear_attrs;

	efi_status = LibLocateProtocol(&EFI_MEMORY_ATTRIBUTE_PROTOCOL_GUID,
				       (VOID **)&proto);
	if (EFI_ERROR(efi_status) || !proto)
		return efi_status;

	efi_status = get_mem_attrs (addr, size, &before);
	if (EFI_ERROR(efi_status))
		dprint(L"get_mem_attrs(0x%llx, 0x%llx, 0x%llx) -> 0x%lx\n",
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
		return 0;
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

	efi_status = get_mem_attrs (addr, size, &after);
	if (EFI_ERROR(efi_status))
		dprint(L"get_mem_attrs(0x%llx, %llu, 0x%llx) -> 0x%lx\n",
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

void
get_hsi_mem_info(void)
{
	EFI_STATUS efi_status;
	uintptr_t addr;
	uint64_t attrs = 0;
	uint32_t *tmp_alloc;

	addr = ((uintptr_t)&get_hsi_mem_info) & ~EFI_PAGE_MASK;

	efi_status = get_mem_attrs(addr, EFI_PAGE_SIZE, &attrs);
	if (EFI_ERROR(efi_status)) {
error:
		/*
		 * In this case we can't actually tell anything, so assume
		 * and report the worst case scenario.
		 */
		hsi_status = SHIM_HSI_STATUS_HEAPX |
			     SHIM_HSI_STATUS_STACKX |
			     SHIM_HSI_STATUS_ROW;
		dprint(L"Setting HSI to 0x%lx due to error: %r\n", hsi_status, efi_status);
		return;
	} else {
		hsi_status = SHIM_HSI_STATUS_HASMAP;
		dprint(L"Setting HSI to 0x%lx\n", hsi_status);
	}

	if (!(hsi_status & SHIM_HSI_STATUS_HASMAP)) {
		dprint(L"No memory protocol, not testing further\n");
		return;
	}

	hsi_status = SHIM_HSI_STATUS_HASMAP;
	if (attrs & MEM_ATTR_W) {
		dprint(L"get_hsi_mem_info() is on a writable page: 0x%x->0x%x\n",
		       hsi_status, hsi_status | SHIM_HSI_STATUS_ROW);
		hsi_status |= SHIM_HSI_STATUS_ROW;
	}

	addr = ((uintptr_t)&addr) & ~EFI_PAGE_MASK;
	efi_status = get_mem_attrs(addr, EFI_PAGE_SIZE, &attrs);
	if (EFI_ERROR(efi_status)) {
		dprint(L"get_mem_attrs(0x%016llx, 0x%x, &attrs) failed.\n", addr, EFI_PAGE_SIZE);
		goto error;
	}

	if (attrs & MEM_ATTR_X) {
		dprint(L"Stack variable is on an executable page: 0x%x->0x%x\n",
		       hsi_status, hsi_status | SHIM_HSI_STATUS_STACKX);
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
		dprint(L"Heap variable is on an executable page: 0x%x->0x%x\n",
		       hsi_status, hsi_status | SHIM_HSI_STATUS_HEAPX);
		hsi_status |= SHIM_HSI_STATUS_HEAPX;
	}
}

// vim:fenc=utf-8:tw=75:noet
