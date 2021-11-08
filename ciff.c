#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jpeglib.h>

#include "ciff.h"

#define MAGIC "CIFF"
#define MAGIC_LENGTH 4


struct ciff_numbers {
	long    cn_hsize;
	long    cn_csize;
	long    cn_width;
	long    cn_height;
};


static int              _verify_magic(FILE *);
static int              _verify_numbers(struct ciff_numbers *);
static struct ciff *    _parse_header(struct ciff *, FILE *);
static struct ciff *    _parse_pixels(struct ciff *, FILE *);
unsigned char *         _px_flatten(unsigned char *, struct pixel *,
			    long, long);

/* ciff.h */
struct ciff *           ciff_parse(struct ciff *, FILE *);
void                    ciff_dump_header(struct ciff *, FILE *);
void                    ciff_dump_pixels(struct ciff *, FILE *);
void                    ciff_jpeg_compress(struct ciff *, FILE *);


static size_t const     ciff_numbers_size
			    = sizeof (struct ciff_numbers);
static size_t const     pixel_size = sizeof (struct pixel);


static int
_verify_magic(FILE *stream)
{
	char    magic[MAGIC_LENGTH];

	if (fread(&magic, 1, MAGIC_LENGTH, stream) != MAGIC_LENGTH) {
		warn("%s: fread", __func__);
		return -1;
	}

	return memcmp(magic, MAGIC, MAGIC_LENGTH) == 0;
}

static int
_verify_numbers(struct ciff_numbers *numbers)
{
	return numbers->cn_csize
	    == 3 * numbers->cn_width * numbers->cn_height;
}

static struct ciff *
_parse_header(struct ciff *dst, FILE *stream)
{
	ssize_t rem, len;
	size_t  n;
	int     i, j, c;
	char   *buf, **tags, *tbuf;

	/* 1. Check magic */
	if (!_verify_magic(stream)) {
		warnx("%s: invalid magic", __func__);
		return NULL;
	}

	/* 2. Read numerical values from header */
	if (fread(dst, ciff_numbers_size, 1, stream) != 1) {
		warn("%s: fread", __func__);
		return NULL;
	}
	rem = dst->ciff_hsize - ciff_numbers_size - MAGIC_LENGTH;

	/* 2.5 Verify numerical values */
	/*
	 * Casting to (struct ciff_numbers *) is OK, since at this
	 * point, dst points to an incomplete (struct ciff) that is just
	 * like a (strut ciff_numbers).
	 */
	if (!_verify_numbers((struct ciff_numbers *)dst)) {
		warnx("%s: invalid numerical header values", __func__);
		return NULL;
	}

	if ((buf = malloc(rem)) == NULL) {
		warn("%s: malloc", __func__);
		return NULL;
	}

	/* 3. Read caption */
	i = 0;
	while ((c = fgetc(stream)) != '\n' && c != EOF) {
		if (rem-- == 0) {
			warnx("%s: overlong caption", __func__);
			return NULL;
		}

		if (c == '\0')
			warnx("%s: ignoring null byte in caption...",
			    __func__);
		else
			buf[i++] = c;
	}
	if (c == '\n')
		--rem;
	else {
		warnx("%s: unexpected EOF", __func__);
		return NULL;
	}
	buf[i] = '\0';
	if ((dst->ciff_cap = malloc(i+1)) == NULL) {
		warn("%s: malloc", __func__);
		return NULL;
	}
	(void)strncpy(dst->ciff_cap, buf, i+1);


	/* allocate one more slot for NULL termination */
	if ((tags = malloc((rem + 1) * sizeof tbuf)) == NULL) {
		warn("%s, malloc", __func__);
		return NULL;
	}

	/* 4. Read tags */
	for (j = 0, tbuf = NULL, n = 0;
	     rem > 0
	     && (len = getdelim(&tbuf, &n, '\0', stream)) != EOF;
	     rem -= len, ++j) {
		if (strchr(tbuf, '\n') != NULL) {
			warnx("%s: tag contains newline", __func__);
			return NULL;
		}

		/*
		 * Allocate len - 1 characters since tbuf contains
		 * trailing NUL already.
		 */
		if ((tags[j] = malloc(len - 1)) == NULL) {
			warn("%s: malloc", __func__);
			return NULL;
		}
		(void)strncpy(tags[j], tbuf, len);
	}
	if (rem < 0) {
		warnx("%s: overlong tags", __func__);
		return NULL;
	}
	tags[j] = NULL;
	dst->ciff_tags = tags;

	free(buf);
	free(tbuf);
	return dst;
}

static struct ciff *
_parse_pixels(struct ciff *ciff, FILE *stream)
{
	size_t  size = ciff->ciff_csize / 3;

	if ((ciff->ciff_content = malloc(size * pixel_size)) == NULL) {
		warn("%s: malloc", __func__);
		return NULL;

	}

	if (fread(ciff->ciff_content, pixel_size, size, stream) != size)
	{
		if (feof(stream) == 0)
			warn("%s: fread", __func__);
		else
			warnx("%s: unexpected EOF", __func__);
		return NULL;
	}

	if (fgetc(stream) != EOF)
		warnx("%s: trailing data ignored", __func__);

	return ciff;
}

unsigned char *
_px_flatten(unsigned char *dst, struct pixel *pxs, long width,
    long height)
{
	int     i, j;

	for (i = 0, j = 0; i < width * height; ++i) {
		dst[j++] = pxs[i].px_r;
		dst[j++] = pxs[i].px_g;
		dst[j++] = pxs[i].px_b;
	}

	return dst;
}


/*---------------------------------------------------------------*
 *      ciff.h                                                   *
 *---------------------------------------------------------------*/

struct ciff *
ciff_parse(struct ciff *dst, FILE *stream)
{
	if (_parse_header(dst, stream) == NULL)
		return NULL;
	if (_parse_pixels(dst, stream) == NULL)
		return NULL;
	return dst;
}

void
ciff_dump_header(struct ciff *ciff, FILE *stream)
{
	char  **tag;

	(void)fprintf(stream, "-----------------------------\n");
	(void)fprintf(stream, "Header size:\t%ld\n", ciff->ciff_hsize);
	(void)fprintf(stream, "Content size:\t%ld\n",
	    ciff->ciff_csize);
	(void)fprintf(stream, "Image width:\t%ld\n", ciff->ciff_width);
	(void)fprintf(stream, "Image height:\t%ld\n",
	    ciff->ciff_height);
	(void)fprintf(stream, "Image caption:\t%s\n", ciff->ciff_cap);
	(void)fprintf(stream, "Image tags:\t");
	for (tag = ciff->ciff_tags; *tag != NULL; ++tag)
		(void)fprintf(stream, "[%s]", *tag);
	(void)fprintf(stream, "\n");
	(void)fprintf(stream, "-----------------------------\n");
}

void
ciff_dump_pixels(struct ciff *ciff, FILE *stream)
{
	int     i, j, k;

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

void
ciff_jpeg_compress(struct ciff *ciff, FILE *out)
{
	struct jpeg_compress_struct     cinfo;
	struct jpeg_error_mgr           jerr;
	JSAMPROW                        rowptr[1];
	int                             rowstride;
	JSAMPLE                        *data;

	/* 0. Init vars */
	if ((data = malloc(ciff->ciff_csize * sizeof (unsigned char)))
	    == NULL) {
		warn("%s: malloc", __func__);
		return;
	}
	if (_px_flatten(data, ciff->ciff_content, ciff->ciff_width,
	    ciff->ciff_height)
	    == NULL)
		return;

	/* 1. Allocate and init JPEG compression obj */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	/* 2. Specify dest */
	jpeg_stdio_dest(&cinfo, out);

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
}
