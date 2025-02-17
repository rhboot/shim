// SPDX-License-Identifier: BSD-2-Clause-Patent

/*
 * This generates the header files that produce the actual revocation
 * string payload. On the one hand this grabs the defintions from the
 * human readable SbatLevel_Variable.txt file which is nice. On the other
 * hand it's one off c code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sbat_revocation sbat_revocation;

struct sbat_revocation {
	int date;
	char *revocations;
	sbat_revocation *next;
};

static sbat_revocation *revlisthead;

int
readfile(char *SbatLevel_Variable)
{
	FILE *varfilep;
	char line[1024];
	int date;
	int ret = -1;

	unsigned int revocationsp = 0;

	sbat_revocation *revlistlast = NULL;
	sbat_revocation *revlistentry = NULL;

	revlisthead = NULL;

	varfilep = fopen(SbatLevel_Variable, "r");
	if (varfilep == NULL)
		return -1;

	while (fgets(line, sizeof(line), varfilep) != NULL) {
		if (sscanf(line, "sbat,1,%d\n", &date) && strlen(line) == 18) {
			revlistentry = calloc(1, sizeof(sbat_revocation));
			if (revlistentry == NULL)
				goto err;
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
				              revocationsp + strlen(line) + 1);
				if (new == NULL) {
					ret = -1;
					goto err;
				}
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
			revocationsp = 0;
		}
	}

	ret = 1;
err:
	if (ret < 0 && revlisthead) {
		sbat_revocation *rle = revlisthead;
		while (rle) {
			sbat_revocation *next = rle->next;
			if (rle->revocations)
				free(rle->revocations);
			free(rle);
			rle = next;
		}
		revlisthead = NULL;
	}
	fclose(varfilep);
	return ret;
}

int
writefile()
{
	int epochfound = 0;
	int epochdate = 2021030218;
	int latestdate = 0;

	sbat_revocation *revlistentry;
	sbat_revocation *latest_revlistentry = NULL;

	revlistentry = revlisthead;

	while (revlistentry != NULL) {
		if (revlistentry->date == epochdate) {
			printf("#ifndef GEN_SBAT_VAR_DEFS_H_\n"
			       "#define GEN_SBAT_VAR_DEFS_H_\n"
			       "#ifndef ENABLE_SHIM_DEVEL\n\n"
			       "#ifndef SBAT_AUTOMATIC_DATE\n"
			       "#define SBAT_AUTOMATIC_DATE 2024040900\n"
			       "#endif /* SBAT_AUTOMATIC_DATE */\n"
			       "#if SBAT_AUTOMATIC_DATE == %d\n"
			       "#define SBAT_VAR_AUTOMATIC_REVOCATIONS\n",
			       revlistentry->date);
			epochfound = 1;
		} else if (epochfound == 1) {
			printf("#elif SBAT_AUTOMATIC_DATE == %d\n"
			       "#define SBAT_VAR_AUTOMATIC_REVOCATIONS \"%s\"\n",
			       revlistentry->date,
			       revlistentry->revocations);
		}
		if (revlistentry->date > latestdate) {
			latest_revlistentry = revlistentry;
			latestdate = revlistentry->date;
		}
		revlistentry = revlistentry->next;
	}

	if (epochfound == 0 || !latest_revlistentry)
		return -1;

	printf("#else\n"
	       "#error \"Unknown SBAT_AUTOMATIC_DATE\"\n"
	       "#endif /* SBAT_AUTOMATIC_DATE == */\n\n"
	       "#define SBAT_VAR_AUTOMATIC_DATE QUOTEVAL(SBAT_AUTOMATIC_DATE)\n"
	       "#define SBAT_VAR_AUTOMATIC \\\n"
               "	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_AUTOMATIC_DATE \"\\n\" \\\n"
               "	SBAT_VAR_AUTOMATIC_REVOCATIONS\n\n");

	printf("#define SBAT_VAR_LATEST_DATE \"%d\"\n"
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
		return writefile();
	else
		return -1;
}
