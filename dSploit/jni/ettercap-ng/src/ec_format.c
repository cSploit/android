/*
    ettercap -- formatting functions

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_format.c,v 1.18 2004/10/12 15:28:38 alor Exp $

*/

#include <ec.h>
#include <ec_format.h>
#include <ec_ui.h>

#include <ctype.h>
#ifdef HAVE_UTF8
   #include <iconv.h>
#endif

/* globals */

#ifdef HAVE_UTF8
   static char *utf8_encoding;
#endif

static u_int8 EBCDIC_to_ASCII[256] = {
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 
   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x2E, 0x2E, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x2E, 0x3F,
   0x20, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x2E, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
   0x26, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
   0x2D, 0x2F, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x7C, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
   0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
   0x2E, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
   0x68, 0x69, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E,
   0x2E, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 
   0x71, 0x72, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
   0x79, 0x7A, 0x2E, 0x2E, 0x2E, 0x5B, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x5D, 0x2E, 0x2E, 
   0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
   0x48, 0x49, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 
   0x51, 0x52, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x5C, 0x2E, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 
   0x59, 0x5A, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x39, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E
};



/* protos */

int hex_len(int len);
int hex_format(const u_char *buf, size_t len, u_char *dst);
int ascii_format(const u_char *buf, size_t len, u_char *dst);
int text_format(const u_char *buf, size_t len, u_char *dst);
int ebcdic_format(const u_char *buf, size_t len, u_char *dst);
int html_format(const u_char *buf, size_t len, u_char *dst);
int bin_format(const u_char *buf, size_t len, u_char *dst);
int zero_format(const u_char *buf, size_t len, u_char *dst);
int utf8_format(const u_char *buf, size_t len, u_char *dst);
int set_utf8_encoding(u_char *fromcode);

int set_format(char *format);

/**********************************/

/*
 * parses the "format" and set the right visualization method
 */
int set_format(char *format)
{
   DEBUG_MSG("set_format: %s", format);
   
   if (!strcasecmp(format, "hex")) {
      GBL_FORMAT = &hex_format;
      return ESUCCESS;
   }

   if (!strcasecmp(format, "ascii")) {
      GBL_FORMAT = &ascii_format;
      return ESUCCESS;
   }

   if (!strcasecmp(format, "text")) {
      GBL_FORMAT = &text_format;
      return ESUCCESS;
   }

   if (!strcasecmp(format, "html")) {
      GBL_FORMAT = &html_format;
      return ESUCCESS;
   }

   if (!strcasecmp(format, "ebcdic")) {
      GBL_FORMAT = &ebcdic_format;
      return ESUCCESS;
   }

   if (!strcasecmp(format, "utf8")) {
      GBL_FORMAT = &utf8_format;
      return ESUCCESS;
   }

   FATAL_MSG("Unsupported format (%s)", format);
}

/* 
 * return the len of the resulting buffer (approximately) 
 */
int hex_len(int len)
{
   int i, nline;
  
   /* null string ? */
   if (len == 0)
      return 1;
   
   /* calculate the number of lines (ceiling) */
   nline = len / HEX_CHAR_PER_LINE;       
   if (len % HEX_CHAR_PER_LINE) nline++;
   
   /* one line is printed as 66 chars */
   i = nline * 66;

   return i;
}

/* 
 * convert a buffer to a hex notation
 *
 * the string  "HTTP/1.1 304 Not Modified" becomes:
 * 
 * 0000: 4854 5450 2f31 2e31 2033 3034 204e 6f74  HTTP/1.1 304 Not
 * 0010: 204d 6f64 6966 6965 64                    Modified
 */

int hex_format(const u_char *buf, size_t len, u_char *dst)
{
   u_int i, j, jm, c;
   int dim = 0;
  
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }
  
   /* empty the string */
   memset(dst, 0, hex_len(len));
   
   for (i = 0; i < len; i += HEX_CHAR_PER_LINE) {
           sprintf(dst, "%s %04x: ", dst, i );
           jm = len - i;
           jm = jm > HEX_CHAR_PER_LINE ? HEX_CHAR_PER_LINE : jm;

           for (j = 0; j < jm; j++) {
                   if ((j % 2) == 1) {
                      sprintf(dst, "%s%02x ", dst, (u_char) buf[i+j]);
                   } else {
                      sprintf(dst, "%s%02x", dst, (u_char) buf[i+j]);
                   }
           }
           for (; j < HEX_CHAR_PER_LINE; j++) {
                   if ((j % 2) == 1) {
                      strcat(dst, "   ");
                   } else {
                      strcat(dst, "  ");
                   }
           } 
           strcat(dst, " ");

           for (j = 0; j < jm; j++) {
                   c = (u_char) buf[i+j];
                   c = isprint(c) ? c : '.';
                   dim = sprintf(dst, "%s%c", dst, c);
           }
           strcat(dst, "\n");
   }

   return dim + 1;
}

/*
 * prints only "printable" characters, the
 * others are represented as '.'
 */

int ascii_format(const u_char *buf, size_t len, u_char *dst)
{
   u_int i = 0;
   
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }

   /* make the substitions */
   for (i = 0; i < len; i++) {
      if ( isprint((int)buf[i]) || buf[i] == '\n' || buf[i] == '\t' )
         dst[i] = buf[i];
      else
         dst[i] = '.';
   }
   
   return len;
}

/*
 * prints only printable chars, skip the others
 */

int text_format(const u_char *buf, size_t len, u_char *dst)
{
   u_int i, j = 0;
   
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }

   for (i = 0; i < len; i++) {

      /* 
       * check for escape chars for ansi color.
       * \033[ is the escape char.
       */
      if (buf[i] == 0x1b && buf[i+1] == 0x5b) {
         /* 
          * find the first alpha char, 
          * this is the end of the ansi sequence
          */
         while( !isalpha((int)buf[i++]) && i < len );
      }
      
      if ( isprint((int)buf[i]) || buf[i] == '\n' || buf[i] == '\t' )
         dst[j++] = buf[i];
   }
      
   return j;
}

/*
 * convert from ebcdic to ascii
 */

int ebcdic_format(const u_char *buf, size_t len, u_char *dst)
{
   u_int i = 0;
   
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }
   
   /* convert from ebcdic to ascii */
   for(i = 0; i < len; i++)
      dst[i] = (char) EBCDIC_to_ASCII[(u_int8)buf[i]];
   
   return ascii_format(dst, len, dst);
}

/*
 * strip ever string contained in <...>
 */

int html_format(const u_char *buf, size_t len, u_char *dst)
{
   u_int i, j = 0;
   
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }

   for (i = 0; i < len; i++) {

      /* if a tag is opened, skip till the end */ 
      if (buf[i] == '<') {
         while( buf[i++] != '>' && i < len );
      }
      
      if ( isprint((int)buf[i]) || buf[i] == '\n' || buf[i] == '\t' )
         dst[j++] = buf[i];
   }
      
   return j;
}

/*
 * return the buffer as is
 */

int bin_format(const u_char *buf, size_t len, u_char *dst)
{
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }
   
   /* copy the buffer */
   memcpy(dst, buf, len);
   
   return len;
}

/*
 * return the void string 
 */

int zero_format(const u_char *buf, size_t len, u_char *dst)
{
   strcpy(dst, "");
   return 0;
}

/*
 * convert from a specified encoding to utf8
 */

int utf8_format(const u_char *buf, size_t len, u_char *dst)
{
#ifndef HAVE_UTF8
   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }
   
   /* copy the buffer */
   memcpy(dst, buf, len);

   return len;
#else
   
   iconv_t cd;
#ifdef OS_LINUX
   char *inbuf;
#else
   const char *inbuf;
#endif
   char *outbuf;
   size_t inbytesleft, outbytesleft;

   /* some sanity checks */
   if (len == 0 || buf == NULL) {
      strcpy(dst, "");
      return 0;
   }

   if (utf8_encoding == NULL) {
      ui_msg("You must set an encoding type before using UTF8.\n");
      return 0;
   }

   inbuf = (char *)buf;
   inbytesleft = len;
   outbuf = (char *)dst;

   cd = iconv_open("UTF-8", utf8_encoding);

   iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

   iconv_close(cd);

   return len;
#endif
}

/*
 * set the encoding to use when converting to UTF-8
 */
int set_utf8_encoding(u_char *fromcode)
{
#ifndef HAVE_UTF8
   USER_MSG("UTF-8 support not compiled in.");
   return ESUCCESS;
#else
   iconv_t cd;

   DEBUG_MSG("set_utf8_encoding: %s", fromcode);
      
   if (fromcode == NULL || strlen(fromcode) < 1)
      return -EINVALID;

   SAFE_FREE(utf8_encoding);

   /* make sure encoding type is supported */
   cd = iconv_open("UTF-8", fromcode);
   
   if (cd == (iconv_t)(-1))
      SEMIFATAL_ERROR("The conversion from %s to UTF-8 is not supported.", fromcode);
   
   iconv_close(cd);

   utf8_encoding = strdup(fromcode);

   return ESUCCESS;
#endif
}

/* EOF */

// vim:ts=3:expandtab

