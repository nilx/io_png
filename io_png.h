#ifndef _IO_PNG_H
#define _IO_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

#define IO_PNG_VERSION "0.20110919"

#include <stddef.h>

/* io_png.c */
char *io_png_info(void);
float *io_png_read_pp_flt(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp, const char *option);
float *io_png_read_flt(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp);
unsigned char *io_png_read_pp_uchar(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp, const char *option);
unsigned char *io_png_read_uchar(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp);
unsigned short *io_png_read_pp_ushrt(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp, const char *option);
unsigned short *io_png_read_ushrt(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp);
void io_png_write_flt(const char *fname, const float *data, size_t nx, size_t ny, size_t nc);
void io_png_write_uchar(const char *fname, const unsigned char *data, size_t nx, size_t ny, size_t nc);
void io_png_write_ushrt(const char *fname, const unsigned char *data, size_t nx, size_t ny, size_t nc);

#ifdef __cplusplus
}
#endif

#endif /* !_IO_PNG_H */
