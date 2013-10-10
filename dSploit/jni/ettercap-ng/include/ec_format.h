
/* $Id: ec_format.h,v 1.10 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_FORMAT_H
#define EC_FORMAT_H

EC_API_EXTERN int hex_len(int len);
EC_API_EXTERN int hex_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int ascii_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int text_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int ebcdic_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int html_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int bin_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int zero_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int utf8_format(const u_char *buf, size_t len, u_char *dst);
EC_API_EXTERN int set_utf8_encoding(u_char *fromcode);

EC_API_EXTERN int set_format(char *format);

#define HEX_CHAR_PER_LINE 16

#endif

/* EOF */

// vim:ts=3:expandtab

