#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prk.h"

int main(int argc, char *argv[])
{
	static unsigned char src[PRKLEN], dst[PRKLEN];
	static struct prk prk;
	FILE *fin;
	int i, r;

	if (argc < 2) {
		fprintf(stderr, "usage: prktest file1.PRK ... [fileN.PRK]\n");
		return EXIT_FAILURE;
	}
	r = EXIT_SUCCESS;
	for (i = 1; i < argc; i++) {
		if ((fin = fopen(argv[i], "rb")) == NULL) {
			fprintf(stderr, "%s: can't open file\n", argv[i]);
			r = EXIT_FAILURE;
			continue;
		}
		fread(src, 1, sizeof src, fin);
		fclose(fin);
		if (prkread(&prk, src)) {
			fprintf(stderr, "%s: prkread failed\n", argv[i]);
			r = EXIT_FAILURE;
			continue;
		}
		if (prkwrite(&prk, dst)) {
			fprintf(stderr, "%s: prkwrite failed\n", argv[i]);
			r = EXIT_FAILURE;
			continue;
		}
		if (memcmp(src, dst, PRKLEN)) {
			fprintf(stderr, "%s: input != output\n", argv[i]);
			r = EXIT_FAILURE;
			continue;
		}
	}
	return r;
}
