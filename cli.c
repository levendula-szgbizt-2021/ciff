#include <err.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ciff.h"

#define CHUNKSIZ        4096

static void     _usage(void);
static void     _slurp(char **, size_t *, FILE *);
static void     _dump(FILE *, char *, unsigned long);

static void     _err(int, char const *, ...);

static char const      *_progname;


static void
_usage(void)
{
	(void)fprintf(stderr, "usage: %s [-dhv] [-o output] [input]\n",
	    _progname);
	exit(1);
}

static void
_slurp(char **dst, size_t *len, FILE *stream)
{
	size_t  n, size;
	char   *tmp;

	*len = size = 0;
	*dst = NULL;
	while (1) {
		/* Grow allocated memory as needed */
		if (*len + CHUNKSIZ + 1 > size) {
			size = *len + CHUNKSIZ + 1;

			/* overflow check */
			if (size <= *len) {
				free(*dst);
				errx(1, "%s: overflow", __func__);
			}

			/* reallocation */
			if ((tmp = realloc(*dst, size)) == NULL) {
				free(*dst);
				err(1, "%s: realloc", __func__);
			}
			*dst = tmp;
		}

		/* Read a chunk */
		n = fread(*dst + *len, 1, CHUNKSIZ, stream);
		if (n == 0)
			break;

		*len += n;
	}

	if (ferror(stream)) {
		free(*dst);
		err(1, "%s: ferror", __func__);
	}
}

static void
_dump(FILE *target, char *data, unsigned long len)
{
	if (fwrite(data, 1, len, target) < len)
		err(1, "%s: fwrite", __func__);
}

static void
_err(int eval, char const *fmt, ...)
{
	va_list ap;
	size_t  fmtlen, seplen, estrlen;
	char   *estr, *sep, *newfmt;

	va_start(ap, fmt);

	estr = ciff_strerror(cifferrno);
	sep = ": ";

	fmtlen = strlen(fmt);
	seplen = strlen(sep);
	estrlen = strlen(estr);

	if ((newfmt = malloc(fmtlen + seplen + estrlen + 1)) == NULL)
		err(1, "malloc");
	(void)strncpy(newfmt, fmt, fmtlen);
	newfmt[fmtlen] = '\0';
	(void)strncat(newfmt, sep, seplen);
	(void)strncat(newfmt, estr, estrlen);

	verrx(eval, newfmt, ap);

	va_end(ap);
	free(newfmt);
}

int
main(int argc, char **argv)
{
	size_t          len;
	unsigned long   outlen;
	int             dflag, vflag, c;
	FILE           *in, *out;
	struct ciff    *ciff;
	unsigned char  *output;
	char           *input;

	_progname = argv[0];

	dflag = 0, vflag = 0;
	in = stdin;
	out = stdout;
	while ((c = getopt(argc, argv, "dhvo:")) != -1) {
		switch (c) {
		case 'd':
			dflag = 1;
			break;
		case 'h':
			_usage();
			break;
		case 'v':
			vflag = 1;
			break;
		case 'o':
			if ((out = fopen(optarg, "wb")) == NULL)
				err(1, "%s: %s", __func__, optarg);
			break;
		default:
			_usage();
		}
	}
	argc -= optind, argv += optind;

	switch (argc) {
	case 0:         /* everything OK, input will be stdin */
		break;
	case 1:         /* explicit input file given */
		if (strcmp(argv[0], "-") == 0)
			break;
		if ((in = fopen(argv[0], "rb")) == NULL)
			err(1, "%s: %s", __func__, argv[0]);
		break;
	default:
		_usage();
	}


	if ((ciff = malloc(sizeof (struct ciff))) == NULL)
		err(1, "could not allocate memory");

	_slurp(&input, &len, in);
	if (ciff_parse(ciff, input, &len) == NULL)
		_err(1, "parse failure");
	if (vflag)
		ciff_dump_header(stderr, ciff);
	if (dflag)
		ciff_dump_pixels(stderr, ciff);

	output = NULL;
	if (ciff_jpeg_compress(&output, &outlen, ciff) == NULL)
		_err(1, "JPEG-compression failure");
	_dump(out, (char *)output, outlen);

	free(ciff);
	free(output);
	return 0;
}
