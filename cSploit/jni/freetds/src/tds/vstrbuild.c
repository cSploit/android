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

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include "replacements.h"

TDS_RCSID(var, "$Id: vstrbuild.c,v 1.21 2011-06-18 17:52:24 freddy77 Exp $");

struct string_linked_list
{
	char *str;
	struct string_linked_list *next;
};

/*
 * XXX The magic use of \xFF is bletcherous, but I can't think of anything
 *     better right now.
 */

static char *
norm_fmt(const char *fmt, int fmtlen)
{
	char *newfmt;
	char *cp;
	char skip = 0;

	if (fmtlen == TDS_NULLTERM) {
		fmtlen = strlen(fmt);
	}
	if ((newfmt = (char *) malloc(fmtlen + 1)) == NULL)
		return NULL;

	for (cp = newfmt; fmtlen > 0; fmtlen--, fmt++) {
		switch (*fmt) {
		case ',':
		case ' ':
			if (!skip) {
				*cp++ = '\377';
				skip = 1;
			}
			break;
		default:
			skip = 0;
			*cp++ = *fmt;
			break;
		}
	}
	*cp = '\0';
	return newfmt;
}

TDSRET
tds_vstrbuild(char *buffer, int buflen, int *resultlen, const char *text, int textlen, const char *formats, int formatlen, va_list ap)
{
	char *newformat;
	char *params;
	char *token;
	const char *sep = "\377";
	char *lasts;
	int tokcount = 0;
	struct string_linked_list *head = NULL;
	struct string_linked_list *item = NULL;
	struct string_linked_list **tail = &head;
	int i;
	int state;
	char **string_array = NULL;
	int pnum = 0;
	int pdigit;
	char *paramp = NULL;
	TDSRET rc = TDS_FAIL;

	*resultlen = 0;
	if (textlen == TDS_NULLTERM) {
		textlen = (int)strlen(text);
	}
	if ((newformat = norm_fmt(formats, formatlen)) == NULL) {
		return TDS_FAIL;
	}
	if (vasprintf(&params, newformat, ap) < 0) {
		free(newformat);
		return TDS_FAIL;
	}
	free(newformat);
	for (token = strtok_r(params, sep, &lasts); token != NULL; token = strtok_r(NULL, sep, &lasts)) {
		if ((*tail = (struct string_linked_list *) malloc(sizeof(struct string_linked_list))) == NULL) {
			goto out;
		}
		(*tail)->str = token;
		(*tail)->next = NULL;
		tail = &((*tail)->next);
		tokcount++;
	}
	if ((string_array = (char **) malloc((tokcount + 1) * sizeof(char *))) == NULL) {
		goto out;
	}
	for (item = head, i = 0; i < tokcount; item = item->next, i++) {
		if (item == NULL) {
			goto out;
		}
		string_array[i] = item->str;
		while (*(string_array[i]) == ' ') {
			string_array[i]++;
		}
	}

#define COPYING 1
#define CALCPARAM 2
#define OUTPARAM 3

	state = COPYING;
	while ((buflen > 0) && (textlen > 0)) {
		switch (state) {
		case COPYING:
			switch (*text) {
			case '%':
				state = CALCPARAM;
				text++;
				textlen--;
				pnum = 0;
				break;
			default:
				*buffer++ = *text++;
				buflen--;
				textlen--;
				(*resultlen)++;
				break;
			}
			break;
		case CALCPARAM:
			switch (*text) {
			case '!':
				if (pnum <= tokcount) {
					paramp = string_array[pnum - 1];
					state = OUTPARAM;
				}
				text++;
				textlen--;
				break;
			default:
				pdigit = *text++ - '0';
				if ((pdigit >= 0) && (pdigit <= 9)) {
					pnum *= 10;
					pnum += pdigit;
				}
				textlen--;
				break;
			}
			break;
		case OUTPARAM:
			switch (*paramp) {
			case 0:
				state = COPYING;
				break;
			default:
				*buffer++ = *paramp++;
				buflen--;
				(*resultlen)++;
			}
			break;
		default:
			/* unknown state */
			goto out;
			break;

		}
	}

	rc = TDS_SUCCESS;

      out:
	free(string_array);
	for (item = head; item != NULL; item = head) {
		head = head->next;
		free(item);
	}
	free(params);

	return rc;
}
