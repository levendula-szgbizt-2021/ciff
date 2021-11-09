#include <err.h>
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
static struct ciff *    _parse_header(struct ciff *, char **);
static struct ciff *    _parse_pixels(struct ciff *, char **);
static unsigned char *  _px_flatten(unsigned char *, struct pixel *,
			    unsigned long long, unsigned long long);
static void             _print_separator(FILE *stream, size_t len);

/* ciff.h */
struct ciff *           ciff_parse(struct ciff *, char *);
void                    ciff_dump_header(FILE *, struct ciff *);
void                    ciff_dump_pixels(FILE *, struct ciff *);
unsigned char **        ciff_jpeg_compress(unsigned char **,
			    unsigned long *, struct ciff *);

static unsigned long long       _csiz;


static int
_verify_magic(char *str)
{
	return memcmp(str, MAGIC, MAGIC_LENGTH) == 0;
}

static struct ciff *
_parse_header(struct ciff *dst, char **in)
{
	unsigned long long      rem;
	size_t                  i, len;
	char                   *p;

	/*
	 * See the spec for the header structure.
	 */

	/* 1. Check magic */

	if (!_verify_magic(*in)) {
		warnx("%s: invalid magic", __func__);
		return NULL;
	}
	*in += MAGIC_LENGTH;


	/* 2. Parse numerical values from header */

	/*
	 * Remaining bytes: header size minus magic and header size
	 * itself already read.
	 */
	rem = INT64(*in) - MAGIC_LENGTH - 8;
	*in += 8;

/*
 * Parse a 64-bit integer from the input, advance input pointer by 8,
 * and decrease rem counter by 8.
 */
#define PARSE64(v)                                      \
    if (rem < 8) {                                      \
	warnx("%s: unexpected end of data", __func__);  \
	return NULL;                                    \
    }                                                   \
    v = INT64(*in);                                     \
    *in += 8;                                           \
    rem -= 8;

	PARSE64(_csiz)
	PARSE64(dst->ciff_width)
	PARSE64(dst->ciff_height)

	if (_csiz != 3 * dst->ciff_width * dst->ciff_height) {
		warnx("%s: invalid content size", __func__);
		return NULL;
	}

#undef PARSE64

	/* 3. Parse caption */

/*
 * Parse input until character c is encountered or fail if no c is
 * encountered before rem runs out.
 * Set len to the length of string read.
 */
#define PARSEUNTIL(c, what)                             \
    for (p = *in; **in != (c); ++*in, --rem) {          \
	if (rem == 0) {                                 \
		warnx("%s: overlong " what, __func__);  \
		return NULL;                            \
	}                                               \
    }                                                   \
    len = *in - p;                                      \
    ++*in; --rem;

	PARSEUNTIL('\n', "caption");
	++len;
	if ((dst->ciff_cap = malloc(len)) == NULL) {
		warn("%s: malloc", __func__);
		return NULL;
	}
	(void)strncpy(dst->ciff_cap, p, len - 1);
	dst->ciff_cap[len - 1] = '\0';

	/* 4. Parse tags */

	/* allocate one more slot for NULL termination */
	if ((dst->ciff_tags = malloc((rem + 1) * sizeof (char *)))
	    == NULL) {
		warn("%s, malloc", __func__);
		return NULL;
	}
	for (i = 0; rem > 0; ++i) {
		PARSEUNTIL('\0', "tag")
		if (memchr(p, '\n', len) != NULL) {
			warnx("%s: tag contains newline", __func__);
			return NULL;
		}

		if ((dst->ciff_tags[i] = malloc(len)) == NULL) {
			warn("%s: malloc", __func__);
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
_parse_pixels(struct ciff *ciff, char **in)
{
	size_t          rem;
	struct pixel   *p;

	if ((ciff->ciff_content = malloc(ciff->ciff_width
	    * ciff->ciff_height * sizeof (struct pixel))) == NULL) {
		warn("%s: malloc", __func__);
		return NULL;
	}

	rem = _csiz;
	p = ciff->ciff_content;
	for (; rem > 0; ++p, rem -= 3) {
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

static void
_print_separator(FILE *stream, size_t len)
{
	size_t  i;

	for (i = 0; i < len; ++i)
		if (putc('-', stream) == EOF) {
			warn("%s: putc", __func__);
			return;
		}

	if (putc('\n', stream) == EOF)
		warn("%s: putc", __func__);
}


/*---------------------------------------------------------------*
 *      ciff.h                                                   *
 *---------------------------------------------------------------*/

struct ciff *
ciff_parse(struct ciff *dst, char *in)
{
	if (_parse_header(dst, &in) == NULL)
		return NULL;
	if (_parse_pixels(dst, &in) == NULL)
		return NULL;
	return dst;
}

void
ciff_dump_header(FILE *stream, struct ciff *ciff)
{
	char  **tag;

	_print_separator(stream, 64);

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

	_print_separator(stream, 64);
}

void
ciff_dump_pixels(FILE *stream, struct ciff *ciff)
{
	size_t  i, j, k;

	for (i = 0; i < ciff->ciff_height; ++i) {
		(void)fprintf(stream, "[");
		for (j = 0; j < ciff->ciff_width; ++j) {
			k = i * ciff->ciff_width + j;
			(void)fprintf(stream, "(%hhu, %hhu, %hhu)",
			    ciff->ciff_content[k].px_r,
			    ciff->ciff_content[k].px_g,
			    ciff->ciff_content[k].px_b);
			if (j != ciff->ciff_width - 1)
				(void)fprintf(stream, " ");
		}
		(void)fprintf(stream, "]\n");
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
		warn("%s: malloc", __func__);
		return NULL;
	}
	if (_px_flatten(data, ciff->ciff_content, ciff->ciff_width,
	    ciff->ciff_height)
	    == NULL)
		return NULL;

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
