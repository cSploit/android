/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/wait.h>

static char  software_version[]   = "$Id: debug.c,v 1.2 2001-10-13 00:02:54 brianb Exp $";
static void *no_unused_var_warn[] = {software_version,
                                     no_unused_var_warn};


extern errno;

get_incoming (int fd)
{

FILE *out;
int     len, i, offs=0;
unsigned char    buf[BUFSIZ];



	out = fopen("client.out","w");

	while ((len = read(fd, buf, BUFSIZ)) > 0) {
		fprintf (out,"len is %d\n",len);
		for (i=0;i<len;i++) {
			fprintf (out, "%d:%d",i,buf[i]);
			if (buf[i]>=' ' && buf[i]<'z') 
				fprintf(out," %c",buf[i]);
			fprintf(out,"\n");
		}
		fflush(out);
	}
        close(fd);
        fclose(out);
} 
