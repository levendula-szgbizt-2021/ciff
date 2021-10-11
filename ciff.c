#include <err.h>

#include "ciff.h"

/* ciff.h */
struct ciff *   ciff_parse(struct ciff *, FILE *);
void            ciff_dump_header(struct ciff *, FILE *);
void            ciff_dump_pixels(struct ciff *, FILE *);
void            ciff_jpeg_compress(struct ciff*, FILE *);


struct ciff *
ciff_parse(struct ciff *dst, FILE *stream)
{
	warnx("not yet implemented");
	return NULL;
}

void
ciff_dump_header(struct ciff *ciff, FILE *stream)
{
	warnx("not yet implemented");
}

void
ciff_dump_pixels(struct ciff *ciff, FILE *stream)
{
	warnx("not yet implemented");
}

void
ciff_jpeg_compress(struct ciff *ciff, FILE *stream)
{
	warnx("not yet implemented");
}
