#ifndef CIFF_H
#define CIFF_H

#include <stdio.h>

enum ciff_error {
	CIFF_EERRNO     = -1,   /* standard error, consult errno */
	CIFF_EMAGIC     = -2,   /* invalid magic value */
	CIFF_ECSIZE     = -3,   /* invalid content size value */
	CIFF_ECAP       = -4,   /* caption too long */
	CIFF_ETAG       = -5,   /* tag too long or contains newline */
	CIFF_ENOMORE    = -6,   /* no more data, ie unexpected end */
};

extern enum ciff_error          cifferno;

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

struct ciff *           ciff_parse(struct ciff *, char *, size_t *);

void                    ciff_dump_header(FILE *, struct ciff *);
void                    ciff_dump_pixels(FILE *, struct ciff *);

unsigned char **        ciff_jpeg_compress(unsigned char **,
			    unsigned long *, struct ciff *);

char *                  ciff_strerror(enum ciff_error);

#endif /* CIFF_H */
