// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * post-process-pe.c - fix up timestamps and checksums in broken PE files
 * Copyright Peter Jones <pjones@redhat.com>
 */

#define _GNU_SOURCE 1

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static int verbosity;
#define ERROR         0
#define WARNING       1
#define INFO          2
#define NOISE         3
#define MIN_VERBOSITY ERROR
#define MAX_VERBOSITY NOISE
#define debug(level, ...)                                        \
	({                                                       \
		if (verbosity >= (level)) {                      \
			printf("%s():%d: ", __func__, __LINE__); \
			printf(__VA_ARGS__);                     \
		}                                                \
		0;                                               \
	})

static bool set_nx_compat = false;

typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef uint16_t CHAR16;

typedef unsigned long UINTN;

typedef struct {
	UINT32 Data1;
	UINT16 Data2;
	UINT16 Data3;
	UINT8 Data4[8];
} EFI_GUID;

#include "include/peimage.h"

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GNUC_PREREQ(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define GNUC_PREREQ(maj, min) 0
#endif

#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#define CLANG_PREREQ(maj, min)        \
	((__clang_major__ > (maj)) || \
	 (__clang_major__ == (maj) && __clang_minor__ >= (min)))
#else
#define CLANG_PREREQ(maj, min) 0
#endif

#if GNUC_PREREQ(5, 1) || CLANG_PREREQ(3, 8)
#define add(a0, a1, s) __builtin_add_overflow(a0, a1, s)
#define sub(s0, s1, d) __builtin_sub_overflow(s0, s1, d)
#define mul(f0, f1, p) __builtin_mul_overflow(f0, f1, p)
#else
#define add(a0, a1, s)                \
	({                            \
		(*s) = ((a0) + (a1)); \
		0;                    \
	})
#define sub(s0, s1, d)                \
	({                            \
		(*d) = ((s0) - (s1)); \
		0;                    \
	})
#define mul(f0, f1, p)                \
	({                            \
		(*p) = ((f0) * (f1)); \
		0;                    \
	})
#endif
#define div(d0, d1, q)                           \
	({                                       \
		unsigned int ret_ = ((d1) == 0); \
		if (ret_ == 0)                   \
			(*q) = ((d0) / (d1));    \
		ret_;                            \
	})

static int
image_is_64_bit(EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr)
{
	/* .Magic is the same offset in all cases */
	if (PEHdr->Pe32Plus.OptionalHeader.Magic ==
	    EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		return 1;
	return 0;
}

static void
load_pe(const char *const file, void *const data, const size_t datasize,
        PE_COFF_LOADER_IMAGE_CONTEXT *ctx)
{
	EFI_IMAGE_DOS_HEADER *DOSHdr = data;
	EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr = data;
	size_t HeaderWithoutDataDir, SectionHeaderOffset, OptHeaderSize;
	size_t FileAlignment = 0;
	size_t sz0 = 0, sz1 = 0;
	uintptr_t loc = 0;

	debug(NOISE, "datasize:%zu sizeof(PEHdr->Pe32):%zu\n", datasize,
	      sizeof(PEHdr->Pe32));
	if (datasize < sizeof(PEHdr->Pe32))
		errx(1, "%s: Invalid image size %zu (%zu < %zu)", file,
		     datasize, datasize, sizeof(PEHdr->Pe32));

	debug(NOISE,
	      "DOSHdr->e_magic:0x%02hx EFI_IMAGE_DOS_SIGNATURE:0x%02hx\n",
	      DOSHdr->e_magic, EFI_IMAGE_DOS_SIGNATURE);
	if (DOSHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		errx(1,
		     "%s: Invalid DOS header signature 0x%04hx (expected 0x%04hx)",
		     file, DOSHdr->e_magic, EFI_IMAGE_DOS_SIGNATURE);

	debug(NOISE, "DOSHdr->e_lfanew:%u datasize:%zu\n", DOSHdr->e_lfanew,
	      datasize);
	if (DOSHdr->e_lfanew >= datasize ||
	    add((uintptr_t)data, DOSHdr->e_lfanew, &loc))
		errx(1, "%s: invalid pe header location", file);

	ctx->PEHdr = PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)loc;
	debug(NOISE, "PE signature:0x%04x EFI_IMAGE_NT_SIGNATURE:0x%04x\n",
	      PEHdr->Pe32.Signature, EFI_IMAGE_NT_SIGNATURE);
	if (PEHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE)
		errx(1, "%s: Unsupported image type", file);

	if (image_is_64_bit(PEHdr)) {
		debug(NOISE, "image is 64bit\n");
		ctx->NumberOfRvaAndSizes =
			PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
		ctx->SizeOfHeaders =
			PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
		ctx->ImageSize = PEHdr->Pe32Plus.OptionalHeader.SizeOfImage;
		ctx->SectionAlignment =
			PEHdr->Pe32Plus.OptionalHeader.SectionAlignment;
		FileAlignment = PEHdr->Pe32Plus.OptionalHeader.FileAlignment;
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER64);
	} else {
		debug(NOISE, "image is 32bit\n");
		ctx->NumberOfRvaAndSizes =
			PEHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes;
		ctx->SizeOfHeaders = PEHdr->Pe32.OptionalHeader.SizeOfHeaders;
		ctx->ImageSize = (UINT64)PEHdr->Pe32.OptionalHeader.SizeOfImage;
		ctx->SectionAlignment =
			PEHdr->Pe32.OptionalHeader.SectionAlignment;
		FileAlignment = PEHdr->Pe32.OptionalHeader.FileAlignment;
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER32);
	}

	if (FileAlignment % 2 != 0)
		errx(1, "%s: Invalid file alignment %zu", file, FileAlignment);

	if (FileAlignment == 0)
		FileAlignment = 0x200;
	if (ctx->SectionAlignment == 0)
		ctx->SectionAlignment = PAGE_SIZE;
	if (ctx->SectionAlignment < FileAlignment)
		ctx->SectionAlignment = FileAlignment;

	ctx->NumberOfSections = PEHdr->Pe32.FileHeader.NumberOfSections;

	debug(NOISE,
	      "Number of RVAs:%"PRIu64" EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES:%d\n",
	      ctx->NumberOfRvaAndSizes, EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES);
	if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES < ctx->NumberOfRvaAndSizes)
		errx(1, "%s: invalid number of RVAs (%lu entries, max is %d)",
		     file, (unsigned long)ctx->NumberOfRvaAndSizes,
		     EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES);

	if (mul(sizeof(EFI_IMAGE_DATA_DIRECTORY),
	        EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES, &sz0) ||
	    sub(OptHeaderSize, sz0, &HeaderWithoutDataDir) ||
	    sub(PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
	        HeaderWithoutDataDir, &sz0) ||
	    mul(ctx->NumberOfRvaAndSizes, sizeof(EFI_IMAGE_DATA_DIRECTORY),
	        &sz1) ||
	    (sz0 != sz1)) {
		if (mul(sizeof(EFI_IMAGE_DATA_DIRECTORY),
		        EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES, &sz0))
			debug(ERROR,
			      "sizeof(EFI_IMAGE_DATA_DIRECTORY) * EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES overflows\n");
		else
			debug(ERROR,
			      "sizeof(EFI_IMAGE_DATA_DIRECTORY) * EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES = %zu\n",
			      sz0);
		if (sub(OptHeaderSize, sz0, &HeaderWithoutDataDir))
			debug(ERROR,
			      "OptHeaderSize (%zu) - HeaderWithoutDataDir (%zu) overflows\n",
			      OptHeaderSize, HeaderWithoutDataDir);
		else
			debug(ERROR,
			      "OptHeaderSize (%zu) - HeaderWithoutDataDir (%zu) = %zu\n",
			      OptHeaderSize, sz0, HeaderWithoutDataDir);

		if (sub(PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
		        HeaderWithoutDataDir, &sz0)) {
			debug(ERROR,
			      "PEHdr->Pe32.FileHeader.SizeOfOptionalHeader (%d) - %zu overflows\n",
			      PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
			      HeaderWithoutDataDir);
		} else {
			debug(ERROR,
			      "PEHdr->Pe32.FileHeader.SizeOfOptionalHeader (%d)- %zu = %zu\n",
			      PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
			      HeaderWithoutDataDir, sz0);
		}
		if (mul(ctx->NumberOfRvaAndSizes,
		        sizeof(EFI_IMAGE_DATA_DIRECTORY), &sz1))
			debug(ERROR,
			      "ctx->NumberOfRvaAndSizes (%ld) * sizeof(EFI_IMAGE_DATA_DIRECTORY) overflows\n",
			      (unsigned long)ctx->NumberOfRvaAndSizes);
		else
			debug(ERROR,
			      "ctx->NumberOfRvaAndSizes (%ld) * sizeof(EFI_IMAGE_DATA_DIRECTORY) = %zu\n",
			      (unsigned long)ctx->NumberOfRvaAndSizes, sz1);
		debug(ERROR,
		      "space after image header:%zu data directory size:%zu\n",
		      sz0, sz1);

		errx(1, "%s: image header overflows data directory", file);
	}

	if (add(DOSHdr->e_lfanew, sizeof(UINT32), &SectionHeaderOffset) ||
	    add(SectionHeaderOffset, sizeof(EFI_IMAGE_FILE_HEADER),
	        &SectionHeaderOffset) ||
	    add(SectionHeaderOffset,
	        PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
	        &SectionHeaderOffset)) {
		debug(ERROR, "SectionHeaderOffset:%" PRIu32 " + %zu + %zu + %d",
		      DOSHdr->e_lfanew, sizeof(UINT32),
		      sizeof(EFI_IMAGE_FILE_HEADER),
		      PEHdr->Pe32.FileHeader.SizeOfOptionalHeader);
		errx(1, "%s: SectionHeaderOffset would overflow", file);
	}

	if (sub(ctx->ImageSize, SectionHeaderOffset, &sz0) ||
	    div(sz0, EFI_IMAGE_SIZEOF_SECTION_HEADER, &sz0) ||
	    (sz0 <= ctx->NumberOfSections)) {
		debug(ERROR, "(%" PRIu64 " - %zu) / %d > %d\n", ctx->ImageSize,
		      SectionHeaderOffset, EFI_IMAGE_SIZEOF_SECTION_HEADER,
		      ctx->NumberOfSections);
		errx(1, "%s: image sections overflow image size", file);
	}

	if (sub(ctx->SizeOfHeaders, SectionHeaderOffset, &sz0) ||
	    div(sz0, EFI_IMAGE_SIZEOF_SECTION_HEADER, &sz0) ||
	    (sz0 < ctx->NumberOfSections)) {
		debug(ERROR, "(%zu - %zu) / %d >= %d\n", (size_t)ctx->SizeOfHeaders,
		      SectionHeaderOffset, EFI_IMAGE_SIZEOF_SECTION_HEADER,
		      ctx->NumberOfSections);
		errx(1, "%s: image sections overflow section headers", file);
	}

	if (sub((uintptr_t)PEHdr, (uintptr_t)data, &sz0) ||
	    add(sz0, sizeof(EFI_IMAGE_OPTIONAL_HEADER_UNION), &sz0) ||
	    (sz0 > datasize)) {
		errx(1, "%s: PE Image size %zu > %zu", file, sz0, datasize);
	}

	if (PEHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED)
		errx(1, "%s: Unsupported image - Relocations have been stripped", file);

	if (image_is_64_bit(PEHdr)) {
		ctx->ImageAddress = PEHdr->Pe32Plus.OptionalHeader.ImageBase;
		ctx->EntryPoint =
			PEHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
		ctx->RelocDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory
		                         [EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
		ctx->SecDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory
		                       [EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
	} else {
		ctx->ImageAddress = PEHdr->Pe32.OptionalHeader.ImageBase;
		ctx->EntryPoint =
			PEHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
		ctx->RelocDir = &PEHdr->Pe32.OptionalHeader.DataDirectory
		                         [EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
		ctx->SecDir = &PEHdr->Pe32.OptionalHeader.DataDirectory
		                       [EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
	}

	if (add((uintptr_t)PEHdr, PEHdr->Pe32.FileHeader.SizeOfOptionalHeader,
	        &loc) ||
	    add(loc, sizeof(UINT32), &loc) ||
	    add(loc, sizeof(EFI_IMAGE_FILE_HEADER), &loc))
		errx(1, "%s: invalid location for first section", file);

	ctx->FirstSection = (EFI_IMAGE_SECTION_HEADER *)loc;

	if (ctx->ImageSize < ctx->SizeOfHeaders)
		errx(1,
		     "%s: Image size %"PRIu64" is smaller than header size %lu",
		     file, ctx->ImageSize, ctx->SizeOfHeaders);

	if (sub((uintptr_t)ctx->SecDir, (uintptr_t)data, &sz0) ||
	    sub(datasize, sizeof(EFI_IMAGE_DATA_DIRECTORY), &sz1) ||
	    sz0 > sz1)
		errx(1,
		     "%s: security direcory offset %zu past data directory at %zu",
		     file, sz0, sz1);

	if (ctx->SecDir->VirtualAddress > datasize ||
	    (ctx->SecDir->VirtualAddress == datasize &&
	     ctx->SecDir->Size > 0))
		errx(1, "%s: Security directory extends past end", file);
}

static void
set_dll_characteristics(PE_COFF_LOADER_IMAGE_CONTEXT *ctx)
{
	uint16_t oldflags, newflags;

	if (image_is_64_bit(ctx->PEHdr)) {
		oldflags = ctx->PEHdr->Pe32Plus.OptionalHeader.DllCharacteristics;
	} else {
		oldflags = ctx->PEHdr->Pe32.OptionalHeader.DllCharacteristics;
	}

	if (set_nx_compat)
		newflags = oldflags | EFI_IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
	else
		newflags = oldflags & ~(uint16_t)EFI_IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
	if (oldflags == newflags)
		return;

	debug(INFO, "Updating DLL Characteristics from 0x%04hx to 0x%04hx\n",
	      oldflags, newflags);
	if (image_is_64_bit(ctx->PEHdr)) {
		ctx->PEHdr->Pe32Plus.OptionalHeader.DllCharacteristics = newflags;
	} else {
		ctx->PEHdr->Pe32.OptionalHeader.DllCharacteristics = newflags;
	}
}

static void
fix_timestamp(PE_COFF_LOADER_IMAGE_CONTEXT *ctx)
{
	uint32_t ts;

	if (image_is_64_bit(ctx->PEHdr)) {
		ts = ctx->PEHdr->Pe32Plus.FileHeader.TimeDateStamp;
	} else {
		ts = ctx->PEHdr->Pe32.FileHeader.TimeDateStamp;
	}

	if (ts != 0) {
		debug(INFO, "Updating timestamp from 0x%08x to 0\n", ts);
		if (image_is_64_bit(ctx->PEHdr)) {
			ctx->PEHdr->Pe32Plus.FileHeader.TimeDateStamp = 0;
		} else {
			ctx->PEHdr->Pe32.FileHeader.TimeDateStamp = 0;
		}
	}
}

static void
fix_checksum(PE_COFF_LOADER_IMAGE_CONTEXT *ctx, void *map, size_t mapsize)
{
	uint32_t old;
	uint32_t checksum = 0;
	uint16_t word;
	uint8_t *data = map;

	if (image_is_64_bit(ctx->PEHdr)) {
		old = ctx->PEHdr->Pe32Plus.OptionalHeader.CheckSum;
		ctx->PEHdr->Pe32Plus.OptionalHeader.CheckSum = 0;
	} else {
		old = ctx->PEHdr->Pe32.OptionalHeader.CheckSum;
		ctx->PEHdr->Pe32.OptionalHeader.CheckSum = 0;
	}
	debug(NOISE, "old checksum was 0x%08x\n", old);

	for (size_t i = 0; i < mapsize - 1; i += 2) {
		word = (data[i + 1] << 8ul) | data[i];
		checksum += word;
		checksum = 0xffff & (checksum + (checksum >> 0x10));
	}
	debug(NOISE, "checksum = 0x%08x + 0x%08zx = 0x%08zx\n", checksum,
	      mapsize, checksum + mapsize);

	checksum += mapsize;

	if (checksum != old)
		debug(INFO, "Updating checksum from 0x%08x to 0x%08x\n",
		      old, checksum);

	if (image_is_64_bit(ctx->PEHdr)) {
		ctx->PEHdr->Pe32Plus.OptionalHeader.CheckSum = checksum;
	} else {
		ctx->PEHdr->Pe32.OptionalHeader.CheckSum = checksum;
	}
}

static void
handle_one(char *f)
{
	int fd;
	int rc;
	struct stat statbuf;
	size_t sz;
	void *map;
	int failed = 0;

	PE_COFF_LOADER_IMAGE_CONTEXT ctx = { 0, 0 };

	fd = open(f, O_RDWR | O_EXCL);
	if (fd < 0)
		err(1, "Could not open \"%s\"", f);

	rc = fstat(fd, &statbuf);
	if (rc < 0)
		err(1, "Could not stat \"%s\"", f);

	sz = statbuf.st_size;

	map = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED)
		err(1, "Could not map \"%s\"", f);

	load_pe(f, map, sz, &ctx);

	set_dll_characteristics(&ctx);

	fix_timestamp(&ctx);

	fix_checksum(&ctx, map, sz);

	rc = msync(map, sz, MS_SYNC);
	if (rc < 0) {
		warn("msync(%p, %zu, MS_SYNC) failed", map, sz);
		failed = 1;
	}
	rc = munmap(map, sz);
	if (rc < 0) {
		warn("munmap(%p, %zu) failed", map, sz);
		failed = 1;
	}
	rc = close(fd);
	if (rc < 0) {
		warn("close(%d) failed", fd);
		failed = 1;
	}
	if (failed)
		exit(1);
}

static void __attribute__((__noreturn__)) usage(int status)
{
	FILE *out = status ? stderr : stdout;

	fprintf(out,
	        "Usage: post-process-pe [OPTIONS] file0 [file1 [.. fileN]]\n");
	fprintf(out, "Options:\n");
	fprintf(out, "       -q    Be more quiet\n");
	fprintf(out, "       -v    Be more verbose\n");
	fprintf(out, "       -N    Disable the NX compatibility flag\n");
	fprintf(out, "       -n    Enable the NX compatibility flag\n");
	fprintf(out, "       -h    Print this help text and exit\n");

	exit(status);
}

int main(int argc, char **argv)
{
	int i;
	struct option options[] = {
		{.name = "help",
		 .val = '?',
		 },
		{.name = "usage",
		 .val = '?',
		 },
		{.name = "disable-nx-compat",
		 .val = 'N',
		},
		{.name = "enable-nx-compat",
		 .val = 'n',
		},
		{.name = "quiet",
		 .val = 'q',
		},
		{.name = "verbose",
		 .val = 'v',
		},
		{.name = ""}
	};
	int longindex = -1;

	while ((i = getopt_long(argc, argv, "hNnqv", options, &longindex)) != -1) {
		switch (i) {
		case 'h':
		case '?':
			usage(longindex == -1 ? 1 : 0);
			break;
		case 'N':
			set_nx_compat = false;
			break;
		case 'n':
			set_nx_compat = true;
			break;
		case 'q':
			verbosity = MAX(verbosity - 1, MIN_VERBOSITY);
			break;
		case 'v':
			verbosity = MIN(verbosity + 1, MAX_VERBOSITY);
			break;
		}
	}

	if (optind == argc)
		usage(1);

	for (i = optind; i < argc; i++)
		handle_one(argv[i]);

	return 0;
}

// vim:fenc=utf-8:tw=75:noet
