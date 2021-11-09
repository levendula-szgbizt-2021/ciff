#ifndef CIFF_H
#define CIFF_H

#include <stdio.h>

struct pixel {
	unsigned char           px_r;
	unsigned char           px_g;
	unsigned char           px_b;
};

struct ciff {
	unsigned long long      ciff_width;
	unsigned long long      ciff_height;
	char                   *ciff_cap;
	char                  **ciff_tags;
	struct pixel           *ciff_content;
};

struct ciff *           ciff_parse(struct ciff *, char *);

void                    ciff_dump_header(FILE *, struct ciff *);
void                    ciff_dump_pixels(FILE *, struct ciff *);

unsigned char **        ciff_jpeg_compress(unsigned char **,
			    unsigned long *, struct ciff *);

#endif /* CIFF_H */
