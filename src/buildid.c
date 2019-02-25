/*
 * Walk a list of input files, printing the name and buildid of any file
 * that has one.
 *
 * This program is licensed under the GNU Public License version 2.
 */

#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>

static Elf_Scn *get_scn_named(Elf * elf, char *goal, GElf_Shdr * shdrp_out)
{
	int rc;
	size_t shstrndx = -1;
	int scn_no = 0;
	Elf_Scn *scn = NULL;
	GElf_Shdr shdr_data, *shdrp;

	shdrp = shdrp_out ? shdrp_out : &shdr_data;

	rc = elf_getshdrstrndx(elf, &shstrndx);
	if (rc < 0)
		return NULL;

	do {
		GElf_Shdr *shdr;
		char *name;

		scn = elf_getscn(elf, ++scn_no);
		if (!scn)
			break;

		shdr = gelf_getshdr(scn, shdrp);
		if (!shdr)
			/*
			 * the binary is malformed, but hey, maybe the next
			 * one is fine, why not...
			 */
			continue;

		name = elf_strptr(elf, shstrndx, shdr->sh_name);
		if (name && !strcmp(name, goal))
			return scn;
	} while (scn != NULL);
	return NULL;
}

static void *get_buildid(Elf * elf, size_t * sz)
{
	Elf_Scn *scn;
	size_t notesz;
	size_t offset = 0;
	Elf_Data *data;
	GElf_Shdr shdr;

	scn = get_scn_named(elf, ".note.gnu.build-id", &shdr);
	if (!scn)
		return NULL;

	data = elf_getdata(scn, NULL);
	if (!data)
		return NULL;

	do {
		size_t nameoff;
		size_t descoff;
		GElf_Nhdr nhdr;
		char *name;

		notesz = gelf_getnote(data, offset, &nhdr, &nameoff, &descoff);
		if (!notesz)
			break;
		offset += notesz;

		if (nhdr.n_type != NT_GNU_BUILD_ID)
			continue;

		name = data->d_buf + nameoff;
		if (!name || strcmp(name, ELF_NOTE_GNU))
			continue;

		*sz = nhdr.n_descsz;
		return data->d_buf + descoff;
	} while (notesz);
	return NULL;
}

static void data2hex(uint8_t * data, size_t ds, char *str)
{
	const char hex[] = "0123456789abcdef";
	int s;
	unsigned int d;
	for (d = 0, s = 0; d < ds; d += 1, s += 2) {
		str[s + 0] = hex[(data[d] >> 4) & 0x0f];
		str[s + 1] = hex[(data[d] >> 0) & 0x0f];
	}
	str[s] = '\0';
}

static void handle_one(char *f)
{
	int fd;
	Elf *elf;
	char *b = NULL;
	size_t sz;
	uint8_t *data;
	ssize_t written;

	if (!strcmp(f, "-")) {
		fd = STDIN_FILENO;

		if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
			errx(1, "Couldn't read ELF data from \"%s\"", f);
	} else {
		if ((fd = open(f, O_RDONLY)) < 0)
			err(1, "Couldn't open \"%s\"", f);

		if ((elf = elf_begin(fd, ELF_C_READ_MMAP, NULL)) == NULL)
			errx(1, "Couldn't read ELF data from \"%s\"", f);
	}

	data = get_buildid(elf, &sz);
	if (data) {
		b = alloca(sz * 2 + 1);
		data2hex(data, sz, b);
		if (b) {
			written = write(1, f, strlen(f));
			if (written < 0)
				errx(1, "Error writing build id");
			written = write(1, " ", 1);
			written = write(1, b, strlen(b));
			if (written < 0)
				errx(1, "Error writing build id");
			written = write(1, "\n", 1);
		}
	}
	elf_end(elf);
	close(fd);
}

static void
    __attribute__ ((__noreturn__))
    usage(int status)
{
	FILE *out = status ? stderr : stdout;

	fprintf(out, "Usage: buildid [ flags | file0 [file1 [.. fileN]]]\n");
	fprintf(out, "Flags:\n");
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
		{.name = ""}
	};
	int longindex = -1;

	while ((i = getopt_long(argc, argv, "h", options, &longindex)) != -1) {
		switch (i) {
		case 'h':
		case '?':
			usage(longindex == -1 ? 1 : 0);
			break;
		}
	}

	elf_version(EV_CURRENT);

	if (optind == argc)
		usage(1);

	for (i = optind; i < argc; i++)
		handle_one(argv[i]);

	return 0;
}

// vim:fenc=utf-8:tw=75
