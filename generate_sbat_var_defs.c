// SPDX-License-Identifier: BSD-2-Clause-Patent

/*
 * This generates the header files that produce the actual revocation
 * string payload. On the one hand this grabs the defintions from the
 * human readable SbatLevel_Variable.txt file which is nice. On the other
 * hand it's one off c code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct sbat_revocation sbat_revocation;

struct sbat_revocation {
	unsigned int date;
	char *revocations;
	sbat_revocation *next;
};

static sbat_revocation *revlisthead;

static void
free_revocation_list(void)
{
	sbat_revocation *rle = revlisthead;
	while (rle) {
		sbat_revocation *next = rle->next;
		free(rle->revocations);
		free(rle);
		rle = next;
	}
	revlisthead = NULL;
}

static int
readfile(const char *SbatLevel_Variable)
{
	FILE *varfilep;
	char line[1024];
	int ret = -1;

	sbat_revocation *revlistlast = NULL;
	sbat_revocation *revlistentry = NULL;

	free_revocation_list();

	varfilep = fopen(SbatLevel_Variable, "r");
	if (varfilep == NULL) {
		fprintf(stderr, "Error opening file %s\n", SbatLevel_Variable);
		return -1;
	}

	while (fgets(line, sizeof(line), varfilep) != NULL) {
		unsigned int date;
		size_t revocationsp = 0;

		if (!sscanf(line, "sbat,1,%u\n", &date) || strlen(line) != 18)
			continue;
		revlistentry = calloc(1, sizeof(sbat_revocation));
		if (revlistentry == NULL) {
			fprintf(stderr, "Out of memory\n");
			goto err;
		}
		if (revlisthead == NULL)
			revlisthead = revlistentry;
		else
			revlistlast->next = revlistentry;

		revlistlast = revlistentry;

		revlistentry->date = date;
		while (line[0] != '\n' &&
		       fgets(line, sizeof(line), varfilep) != NULL) {
			char *new = NULL;
			new = realloc(revlistentry->revocations,
			              revocationsp + strlen(line) + 2);
			if (new == NULL) {
				fprintf(stderr, "Out of memory\n");
				goto err;
			}
			new[revocationsp] = '\0';
			revlistentry->revocations = new;
			if (strlen(line) > 1) {
				line[strlen(line) - 1] = 0;
				sprintf(revlistentry->revocations +
				                revocationsp,
				        "%s\\n", line);
				revocationsp =
					revocationsp + strlen(line) + 2;
			}
		}
	}

	if (!ferror(varfilep))
		ret = 0;
err:
	if (ret < 0)
		free_revocation_list();
	fclose(varfilep);
	return ret;
}

static int
writefile()
{
	bool epochfound = false;
	unsigned int epochdate = 2021030218;
	unsigned int latestdate = 0;

	const sbat_revocation *revlistentry;
	const sbat_revocation *latest_revlistentry = NULL;

	revlistentry = revlisthead;

	while (revlistentry != NULL) {
		if (revlistentry->date == epochdate) {
			if (epochfound) {
				fprintf(stderr, "Only one epoch expected\n");
				return -1;
			}
			printf("#ifndef GEN_SBAT_VAR_DEFS_H_\n"
			       "#define GEN_SBAT_VAR_DEFS_H_\n"
			       "#ifndef ENABLE_SHIM_DEVEL\n\n"
			       "#ifndef SBAT_AUTOMATIC_DATE\n"
			       "#define SBAT_AUTOMATIC_DATE 2024040900\n"
			       "#endif /* SBAT_AUTOMATIC_DATE */\n"
			       "#if SBAT_AUTOMATIC_DATE == %u\n"
			       "#define SBAT_VAR_AUTOMATIC_REVOCATIONS\n",
			       revlistentry->date);
			epochfound = true;
		} else if (epochfound) {
			printf("#elif SBAT_AUTOMATIC_DATE == %u\n"
			       "#define SBAT_VAR_AUTOMATIC_REVOCATIONS \"%s\"\n",
			       revlistentry->date,
			       revlistentry->revocations);
		} else {
			fprintf(stderr, "Revocation not expected before epoch\n");
			return -1;
		}
		if (revlistentry->date > latestdate) {
			latest_revlistentry = revlistentry;
			latestdate = revlistentry->date;
		}
		revlistentry = revlistentry->next;
	}

	if (!epochfound || !latest_revlistentry) {
		fprintf(stderr, "Epoch not found\n");
		return -1;
	}

	printf("#else\n"
	       "#error \"Unknown SBAT_AUTOMATIC_DATE\"\n"
	       "#endif /* SBAT_AUTOMATIC_DATE == */\n\n"
	       "#define SBAT_VAR_AUTOMATIC_DATE QUOTEVAL(SBAT_AUTOMATIC_DATE)\n"
	       "#define SBAT_VAR_AUTOMATIC \\\n"
	       "	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_AUTOMATIC_DATE \"\\n\" \\\n"
	       "	SBAT_VAR_AUTOMATIC_REVOCATIONS\n\n");

	printf("#define SBAT_VAR_LATEST_DATE \"%u\"\n"
	       "#define SBAT_VAR_LATEST_REVOCATIONS \"%s\"\n",
	       latest_revlistentry->date,
	       latest_revlistentry->revocations);

	printf("#define SBAT_VAR_LATEST \\\n"
	       "	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_LATEST_DATE \"\\n\" \\\n"
	       "	SBAT_VAR_LATEST_REVOCATIONS\n\n"
	       "#endif /* !ENABLE_SHIM_DEVEL */\n"
	       "#endif /* !GEN_SBAT_VAR_DEFS_H_ */\n");

	return 0;
}


int
main(int argc, char *argv[])
{
	char SbatLevel_Variable[2048];

	if (argc == 2)
		snprintf(SbatLevel_Variable, 2048, "%s/SbatLevel_Variable.txt", argv[1]);
	else
		snprintf(SbatLevel_Variable, 2048, "SbatLevel_Variable.txt");

	if (readfile(SbatLevel_Variable))
		return -1;
	return writefile();
}
