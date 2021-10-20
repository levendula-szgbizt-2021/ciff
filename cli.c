#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ciff.h"

void    usage(void);

__dead void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-dhv] [-o output]\n",
	    getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE           *out;
	int             dflag, vflag, c;
	struct ciff    *ciff;

	dflag = 0, vflag = 0;
	out = stdout;
	while ((c = getopt(argc, argv, "dhvo:")) != -1) {
		switch (c) {
		case 'd':
			dflag = 1;
			break;
		case 'h':
			usage();
			break;
		case 'v':
			vflag = 1;
			break;
		case 'o':
			if ((out = fopen(optarg, "wb")) == NULL)
				err(1, "%s: %s", __func__, optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind, argv += optind;

	if ((ciff = malloc(sizeof (struct ciff))) == NULL)
		err(1, "could not allocate memory");

	if (ciff_parse(ciff, stdin) == NULL)
		errx(1, "parse failure");
	if (vflag)
		ciff_dump_header(ciff, stderr);
	if (dflag)
		ciff_dump_pixels(ciff, stderr);

	ciff_jpeg_compress(ciff, out);

	free(ciff);
	return 0;
}
