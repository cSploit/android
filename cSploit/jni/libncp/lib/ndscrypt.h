/*
    NDS client for ncpfs
    Copyright (C) 1997  Arne de Bruijn

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _NDSCRYPT_H
#define _NDSCRYPT_H

#include <string.h>

void nwencrypt(const unsigned short *cryptbuf, const char *in, char *out);
void nwdecrypt(const unsigned short *cryptbuf, const char *in, char *out);
void nwcryptinit(unsigned short *scryptbuf, const char *key);
void nwencryptblock(const char *cryptkey, const char *buf, int buflen, 
 char *outbuf);
void nwdecryptblock(const char *cryptkey, const char *buf, int buflen, 
 char *outbuf);

#define nwhash1init(hash, hashlen) memset(hash, 0, hashlen)
void nwhash1(char *hash, int hashlen, const char *data, int datalen);

#define nwhash2init(hashbuf) memset(hashbuf, 0, 0x42)
void nwhash2(char *hashbuf, char c);
void nwhash2block(char *hashbuf, const char *data, int datalen);
void nwhash2end(char *hashbuf);

#endif /* _NDSCRYPT_H */
