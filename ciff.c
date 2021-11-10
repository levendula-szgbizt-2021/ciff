#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jpeglib.h>

#include "ciff.h"

#define MAGIC "CIFF"
#define MAGIC_LENGTH 4

/* Get a 64-bit little endian integer value */
#define INT64(x) (                                      \
	  ((unsigned long long)((x)[0] & 0xff) << 0)    \
	| ((unsigned long long)((x)[1] & 0xff) << 8)    \
	| ((unsigned long long)((x)[2] & 0xff) << 16)   \
	| ((unsigned long long)((x)[3] & 0xff) << 24)   \
	| ((unsigned long long)((x)[4] & 0xff) << 32)   \
	| ((unsigned long long)((x)[5] & 0xff) << 40)   \
	| ((unsigned long long)((x)[6] & 0xff) << 48)   \
	| ((unsigned long long)((x)[7] & 0xff) << 56)   \
)

static int              _verify_magic(char *);
static struct ciff *    _parse_header(struct ciff *, char **, size_t *);
static struct ciff *    _parse_pixels(struct ciff *, char **, size_t *);
static unsigned char *  _px_flatten(unsigned char *, struct pixel *,
			    unsigned long long, unsigned long long);
static size_t           _print_separator(FILE *, char, size_t);

/* ciff.h */
struct ciff *           ciff_parse(struct ciff *, char *, size_t);
void                    ciff_dump_header(FILE *, struct ciff *);
void                    ciff_dump_pixels(FILE *, struct ciff *);
unsigned char **        ciff_jpeg_compress(unsigned char **,
			    unsigned long *, struct ciff *);
char *                  ciff_strerror(enum ciff_error);

enum ciff_error                 cifferno;

static unsigned long long       _csiz;


static int
_verify_magic(char *str)
{
	return memcmp(str, MAGIC, MAGIC_LENGTH) == 0;
}

static struct ciff *
_parse_header(struct ciff *dst, char **in, size_t *rem)
{
	unsigned long long      hrem;
	size_t                  i, len;
	char                   *p;

	/*
	 * See the spec for the header structure.
	 */

	/* 1. Check magic */

	if (!_verify_magic(*in)) {
		cifferno = CIFF_EMAGIC;
		return NULL;
	}
	*in += MAGIC_LENGTH; *rem -= MAGIC_LENGTH;


	/* 2. Parse numerical values from header */

	/*
	 * Remaining bytes: header size minus magic and header size
	 * itself already read.
	 */
	hrem = INT64(*in) - MAGIC_LENGTH - 8;
	*rem -= 8;
	if (hrem > *rem) {
		cifferno = CIFF_ENOMORE;
		return NULL;
	}
	*in += 8;
	*rem -= hrem;

/*
 * Parse a 64-bit integer from the input, advance input pointer by 8,
 * and decrease rem counter by 8.
 */
#define PARSE64(v)                                      \
    if (hrem < 8) {                                     \
	cifferno = CIFF_ENOMORE;                        \
	return NULL;                                    \
    }                                                   \
    v = INT64(*in);                                     \
    *in += 8; hrem -= 8;

	PARSE64(_csiz)
	PARSE64(dst->ciff_width)
	PARSE64(dst->ciff_height)

	if (_csiz != 3 * dst->ciff_width * dst->ciff_height) {
		cifferno = CIFF_ECSIZE;
		return NULL;
	}

#undef PARSE64

	/* 3. Parse caption */

/*
 * Parse input until character c is encountered or fail if no c is
 * encountered before rem runs out.
 * Set len to the length of string read.
 */
#define PARSEUNTIL(c, err)                              \
    for (p = *in; **in != (c); ++*in, --hrem) {         \
	if (hrem == 0) {                                \
		cifferno = err;                         \
		return NULL;                            \
	}                                               \
    }                                                   \
    len = *in - p;                                      \
    if (hrem == 0) {                                    \
    	cifferno = err;                                 \
    	return NULL;                                    \
    }                                                   \
    ++*in; --hrem;

	PARSEUNTIL('\n', CIFF_ECAP);
	++len;
	if ((dst->ciff_cap = malloc(len)) == NULL) {
		cifferno = CIFF_EERRNO;
		return NULL;
	}
	(void)strncpy(dst->ciff_cap, p, len - 1);
	dst->ciff_cap[len - 1] = '\0';

	/* 4. Parse tags */

	/* allocate one more slot for NULL termination */
	if ((dst->ciff_tags = malloc((hrem + 1) * sizeof (char *)))
	    == NULL) {
		cifferno = CIFF_EERRNO;
		return NULL;
	}
	for (i = 0; hrem > 0; ++i) {
		PARSEUNTIL('\0', CIFF_ETAG);
		if (memchr(p, '\n', len) != NULL) {
			cifferno = CIFF_ETAG;
			return NULL;
		}

		if ((dst->ciff_tags[i] = malloc(len)) == NULL) {
			cifferno = CIFF_EERRNO;
			return NULL;
		}
		(void)strncpy(dst->ciff_tags[i], p, len);
		/* NUL already copied */
	}
	dst->ciff_tags[i] = NULL;

#undef PARSEUNTIL

	return dst;
}

static struct ciff *
_parse_pixels(struct ciff *ciff, char **in, size_t *rem)
{
	struct pixel   *p;

	if ((ciff->ciff_content = malloc(ciff->ciff_width
	    * ciff->ciff_height * sizeof (struct pixel))) == NULL) {
		cifferno = CIFF_EERRNO;
		return NULL;
	}

	if (_csiz > *rem) {
		cifferno = CIFF_ENOMORE;
		return NULL;
	}
	if (_csiz < *rem) {
		/* TODO trailing data is simply ignored */
	}

	p = ciff->ciff_content;
	for (; *rem >= 3; ++p, *rem -= 3) {
		/* (unsigned) char is guaranteed to by 1 byte */
		(void)memcpy(&p->px_r, (*in)++, 1);
		(void)memcpy(&p->px_g, (*in)++, 1);
		(void)memcpy(&p->px_b, (*in)++, 1);
	}

	return ciff;
}

static unsigned char *
_px_flatten(unsigned char *dst, struct pixel *pxs,
    unsigned long long width, unsigned long long height)
{
	size_t  i, j;

	for (i = 0, j = 0; i < width * height; ++i) {
		dst[j++] = pxs[i].px_r;
		dst[j++] = pxs[i].px_g;
		dst[j++] = pxs[i].px_b;
	}

	return dst;
}

static size_t
_print_separator(FILE *stream, char ch, size_t len)
{
	size_t  i;

	for (i = 0; i < len; ++i)
		if (putc(ch, stream) == EOF) {
			cifferno = CIFF_EERRNO;
			return i;
		}

	if (putc('\n', stream) == EOF) {
		cifferno = CIFF_EERRNO;
		return i;
	}

	return i + 1;
}


/*---------------------------------------------------------------*
 *      ciff.h                                                   *
 *---------------------------------------------------------------*/

struct ciff *
ciff_parse(struct ciff *dst, char *in, size_t len)
{
	if (_parse_header(dst, &in, &len) == NULL)
		return NULL;
	if (_parse_pixels(dst, &in, &len) == NULL)
		return NULL;
	return dst;
}

void
ciff_dump_header(FILE *stream, struct ciff *ciff)
{
	char  **tag;

	(void)_print_separator(stream, '-', 64);

	(void)fprintf(stream, "Image width:\t%llu\n",
	    ciff->ciff_width);
	(void)fprintf(stream, "Image height:\t%llu\n",
	    ciff->ciff_height);
	(void)fprintf(stream, "Image caption:\t%s\n",
	    ciff->ciff_cap);

	(void)fprintf(stream, "Image tags:\t");
	for (tag = ciff->ciff_tags; *tag != NULL; ++tag)
		(void)fprintf(stream, "[%s]", *tag);
	(void)fprintf(stream, "\n");

	(void)_print_separator(stream, '-', 64);
}

void
ciff_dump_pixels(FILE *stream, struct ciff *ciff)
{
	size_t  i, j, k;

	for (i = 0; i < ciff->ciff_height; ++i) {
		(void)fprintf(stream, "\n");
		(void)_print_separator(stream, '=', 64);
		(void)fprintf(stream, "\t\tROW #%zu\n", i + 1);
		(void)_print_separator(stream, '=', 64);
		for (j = 0; j < ciff->ciff_width; ++j) {
			k = i * ciff->ciff_width + j;
			(void)fprintf(stream, "(%hhu, %hhu, %hhu)\n",
			    ciff->ciff_content[k].px_r,
			    ciff->ciff_content[k].px_g,
			    ciff->ciff_content[k].px_b);
		}
	}
}

unsigned char **
ciff_jpeg_compress(unsigned char **dst, unsigned long *len,
    struct ciff *ciff)
{
	struct jpeg_compress_struct     cinfo;
	struct jpeg_error_mgr           jerr;
	JSAMPROW                        rowptr[1];
	int                             rowstride;
	JSAMPLE                        *data;

	/* 0. Init vars */
	if ((data = malloc(3 * ciff->ciff_width * ciff->ciff_height))
	    == NULL) {
		cifferno = CIFF_EERRNO;
		return NULL;
	}
	(void)_px_flatten(data, ciff->ciff_content, ciff->ciff_width,
	    ciff->ciff_height);

	/* 1. Allocate and init JPEG compression obj */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	/* 2. Specify dest */
	*len = 0;
	jpeg_mem_dest(&cinfo, dst, len);

	/* 3. Set params */
	cinfo.image_width = ciff->ciff_width;
	cinfo.image_height = ciff->ciff_height;
	cinfo.input_components = 3; /* r g b */
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);

	/* 4. Start compressor */
	jpeg_start_compress(&cinfo, 1);

	/* 5. iterate over data */
	rowstride = cinfo.image_width * 3; /* JSAMPLEs per row */
	while (cinfo.next_scanline < cinfo.image_height) {
		rowptr[0] = &data[cinfo.next_scanline * rowstride];
		(void)jpeg_write_scanlines(&cinfo, rowptr, 1);
	}

	/* 6. Finish */
	jpeg_finish_compress(&cinfo);

	/* 7. Cleanup */
	free(data);
	jpeg_destroy_compress(&cinfo);

	return dst;
}

char *
ciff_strerror(enum ciff_error err)
{
	switch (err) {
	case CIFF_EERRNO:
		return strerror(errno);
	case CIFF_EMAGIC:
		return "Invalid magic value";
	case CIFF_ECSIZE:
		return "Invalid content size value";
	case CIFF_ECAP:
		return "Caption too long";
	case CIFF_ETAG:
		return "Tag contains newline or is too long";
	case CIFF_ENOMORE:
		return "Unexpected end of data";
	default:
		return "Unknown error";
	}
}
