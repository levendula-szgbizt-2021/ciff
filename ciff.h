#ifndef CIFF_H
#define CIFF_H

#include <stdio.h>

struct pixel {
	unsigned char   px_r;
	unsigned char   px_g;
	unsigned char   px_b;
};

struct ciff {
	long            ciff_hsize;
	long            ciff_csize;
	long            ciff_width;
	long            ciff_height;
	char           *ciff_cap;
	char          **ciff_tags;
	struct pixel   *ciff_content;
};

struct ciff *   ciff_parse(struct ciff *, FILE *);
void            ciff_dump_header(struct ciff *, FILE *);
void            ciff_dump_pixels(struct ciff *, FILE *);

void            ciff_jpeg_compress(struct ciff*, FILE *);

#endif /* CIFF_H */
