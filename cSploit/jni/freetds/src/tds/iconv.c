/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2010  Frediano Ziglio
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

/**
 * \file
 * \brief Handle character conversions to/from server
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include <freetds/tds.h>
#include <freetds/iconv.h>
#if HAVE_ICONV
#include <iconv.h>
#endif
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: iconv.c,v 1.150 2011-06-03 21:04:15 freddy77 Exp $");

#define CHARSIZE(charset) ( ((charset)->min_bytes_per_char == (charset)->max_bytes_per_char )? \
				(charset)->min_bytes_per_char : 0 )


static int collate2charset(int sql_collate, int lcid);
static size_t skip_one_input_sequence(iconv_t cd, const TDS_ENCODING * charset, const char **input, size_t * input_size);
static int tds_iconv_info_init(TDSICONV * char_conv, int client_canonic, int server_canonic);
static int tds_iconv_init(void);
static int tds_canonical_charset(const char *charset_name);
static void _iconv_close(iconv_t * cd);
static void tds_iconv_info_close(TDSICONV * char_conv);


/**
 * \ingroup libtds
 * \defgroup conv Charset conversion
 * Convert between different charsets.
 */

#define TDS_ICONV_ENCODING_TABLES
#include "encodings.h"

/* this will contain real iconv names */
static const char *iconv_names[sizeof(canonic_charsets) / sizeof(canonic_charsets[0])];
static int iconv_initialized = 0;
static const char *ucs2name;

enum
{ POS_ISO1, POS_UTF8, POS_UCS2LE, POS_UCS2BE };

#if ENABLE_EXTRA_CHECKS
int tds_iconv_force_double = 0;
#endif

/**
 * Initialize charset searching for UTF-8, UCS-2 and ISO8859-1
 */
static int
tds_iconv_init(void)
{
	int i;
	iconv_t cd;

	/* first entries should be constants */
	assert(strcmp(canonic_charsets[POS_ISO1].name, "ISO-8859-1") == 0);
	assert(strcmp(canonic_charsets[POS_UTF8].name, "UTF-8") == 0);
	assert(strcmp(canonic_charsets[POS_UCS2LE].name, "UCS-2LE") == 0);
	assert(strcmp(canonic_charsets[POS_UCS2BE].name, "UCS-2BE") == 0);

	/* fast tests for GNU-iconv */
	cd = tds_sys_iconv_open("ISO-8859-1", "UTF-8");
	if (cd != (iconv_t) -1) {
		iconv_names[POS_ISO1] = "ISO-8859-1";
		iconv_names[POS_UTF8] = "UTF-8";
		tds_sys_iconv_close(cd);
	} else {

		/* search names for ISO8859-1 and UTF-8 */
		for (i = 0; iconv_aliases[i].alias; ++i) {
			int j;

			if (iconv_aliases[i].canonic != POS_ISO1)
				continue;
			for (j = 0; iconv_aliases[j].alias; ++j) {
				if (iconv_aliases[j].canonic != POS_UTF8)
					continue;

				cd = tds_sys_iconv_open(iconv_aliases[i].alias, iconv_aliases[j].alias);
				if (cd != (iconv_t) -1) {
					iconv_names[POS_ISO1] = iconv_aliases[i].alias;
					iconv_names[POS_UTF8] = iconv_aliases[j].alias;
					tds_sys_iconv_close(cd);
					break;
				}
			}
			if (iconv_names[POS_ISO1])
				break;
		}
		/* required characters not found !!! */
		if (!iconv_names[POS_ISO1])
			return 1;
	}

	/* now search for UCS-2 */
	cd = tds_sys_iconv_open(iconv_names[POS_ISO1], "UCS-2LE");
	if (cd != (iconv_t) -1) {
		iconv_names[POS_UCS2LE] = "UCS-2LE";
		tds_sys_iconv_close(cd);
	}
	cd = tds_sys_iconv_open(iconv_names[POS_ISO1], "UCS-2BE");
	if (cd != (iconv_t) -1) {
		iconv_names[POS_UCS2BE] = "UCS-2BE";
		tds_sys_iconv_close(cd);
	}

	/* long search needed ?? */
	if (!iconv_names[POS_UCS2LE] || !iconv_names[POS_UCS2BE]) {
		for (i = 0; iconv_aliases[i].alias; ++i) {
			if (strncmp(canonic_charsets[iconv_aliases[i].canonic].name, "UCS-2", 5) != 0)
				continue;

			cd = tds_sys_iconv_open(iconv_aliases[i].alias, iconv_names[POS_ISO1]);
			if (cd != (iconv_t) -1) {
				char ib[1];
				char ob[4];
				size_t il, ol;
				ICONV_CONST char *pib;
				char *pob;
				int byte_sequence = 0;

				/* try to convert 'A' and check result */
				ib[0] = 0x41;
				pib = ib;
				pob = ob;
				il = 1;
				ol = 4;
				ob[0] = ob[1] = 0;
				if (tds_sys_iconv(cd, &pib, &il, &pob, &ol) != (size_t) - 1) {
					/* byte order sequence ?? */
					if (ol == 0) {
						ob[0] = ob[2];
						byte_sequence = 1;
						/* TODO save somewhere */
					}

					/* save name without sequence (if present) */
					if (ob[0])
						il = POS_UCS2LE;
					else
						il = POS_UCS2BE;
					if (!iconv_names[il] || !byte_sequence)
						iconv_names[il] = iconv_aliases[i].alias;
				}
				tds_sys_iconv_close(cd);
			}
		}
	}
	/* we need a UCS-2 (big endian or little endian) */
	if (!iconv_names[POS_UCS2LE] && !iconv_names[POS_UCS2BE])
		return 2;

	ucs2name = iconv_names[POS_UCS2LE] ? iconv_names[POS_UCS2LE] : iconv_names[POS_UCS2BE];

	for (i = 0; i < 4; ++i)
		tdsdump_log(TDS_DBG_INFO1, "local name for %s is %s\n", canonic_charsets[i].name,
			    iconv_names[i] ? iconv_names[i] : "(null)");

	/* success (it should always occurs) */
	return 0;
}

/**
 * Get iconv name given canonic
 */
static const char *
tds_set_iconv_name(int charset)
{
	int i;
	iconv_t cd;

	assert(iconv_initialized);

	/* try using canonic name and UTF-8 and UCS2 */
	cd = tds_sys_iconv_open(iconv_names[POS_UTF8], canonic_charsets[charset].name);
	if (cd != (iconv_t) -1) {
		iconv_names[charset] = canonic_charsets[charset].name;
		tds_sys_iconv_close(cd);
		return iconv_names[charset];
	}
	cd = tds_sys_iconv_open(ucs2name, canonic_charsets[charset].name);
	if (cd != (iconv_t) -1) {
		iconv_names[charset] = canonic_charsets[charset].name;
		tds_sys_iconv_close(cd);
		return iconv_names[charset];
	}

	/* try all alternatives */
	for (i = 0; iconv_aliases[i].alias; ++i) {
		if (iconv_aliases[i].canonic != charset)
			continue;

		cd = tds_sys_iconv_open(iconv_names[POS_UTF8], iconv_aliases[i].alias);
		if (cd != (iconv_t) -1) {
			iconv_names[charset] = iconv_aliases[i].alias;
			tds_sys_iconv_close(cd);
			return iconv_names[charset];
		}

		cd = tds_sys_iconv_open(ucs2name, iconv_aliases[i].alias);
		if (cd != (iconv_t) -1) {
			iconv_names[charset] = iconv_aliases[i].alias;
			tds_sys_iconv_close(cd);
			return iconv_names[charset];
		}
	}

	/* charset not found, pretend it's ISO 8859-1 */
	iconv_names[charset] = canonic_charsets[POS_ISO1].name;
	return NULL;
}

static void
tds_iconv_reset(TDSICONV *conv)
{
	/*
	 * (min|max)_bytes_per_char can be used to divide
	 * so init to safe values
	 */
	conv->to.charset.min_bytes_per_char = 1;
	conv->to.charset.max_bytes_per_char = 1;
	conv->from.charset.min_bytes_per_char = 1;
	conv->from.charset.max_bytes_per_char = 1;

	conv->to.charset.name = conv->from.charset.name = "";
	conv->to.charset.canonic = conv->from.charset.canonic = 0;
	conv->to.cd = (iconv_t) -1;
	conv->to.cd2 = (iconv_t) -1;
	conv->from.cd = (iconv_t) -1;
	conv->from.cd2 = (iconv_t) -1;
}

/**
 * Allocate iconv stuff
 * \return 0 for success
 */
int
tds_iconv_alloc(TDSCONNECTION * conn)
{
	int i;
	TDSICONV *char_conv;

	assert(!conn->char_convs);
	if (!(conn->char_convs = (TDSICONV **) malloc(sizeof(TDSICONV *) * (initial_char_conv_count + 1))))
		return 1;
	char_conv = (TDSICONV *) calloc(initial_char_conv_count, sizeof(TDSICONV));
	if (!char_conv) {
		TDS_ZERO_FREE(conn->char_convs);
		return 1;
	}
	conn->char_conv_count = initial_char_conv_count + 1;

	for (i = 0; i < initial_char_conv_count; ++i) {
		conn->char_convs[i] = &char_conv[i];
		tds_iconv_reset(&char_conv[i]);
	}

	/* chardata is just a pointer to another iconv info */
	conn->char_convs[initial_char_conv_count] = conn->char_convs[client2server_chardata];

	return 0;
}

/**
 * \addtogroup conv
 * @{ 
 * Set up the initial iconv conversion descriptors.
 * When the socket is allocated, three TDSICONV structures are attached to iconv.  
 * They have fixed meanings:
 * 	\li 0. Client <-> UCS-2 (client2ucs2)
 * 	\li 1. Client <-> server single-byte charset (client2server_chardata)
 *	\li 2. ISO8859-1  <-> server meta data	(iso2server_metadata)
 *
 * Other designs that use less data are possible, but these three conversion needs are 
 * very often needed.  By reserving them, we avoid searching the array for our most common purposes.
 *
 * To solve different iconv names and portability problems FreeTDS maintains 
 * a list of aliases each charset.  
 * 
 * First we discover the names of our minimum required charsets (UTF-8, ISO8859-1 and UCS2).  
 * Later, as and when it's needed, we try to discover others.
 *
 * There is one list of canonic names (GNU iconv names) and two sets of aliases
 * (one for other iconv implementations and another for Sybase). For every
 * canonic charset name we cache the iconv name found during discovery. 
 */
void
tds_iconv_open(TDSCONNECTION * conn, const char *charset)
{
	static const char UCS_2LE[] = "UCS-2LE";
	int canonic;
	int canonic_charset = tds_canonical_charset(charset);
	int canonic_env_charset = conn->env.charset ? tds_canonical_charset(conn->env.charset) : -1;
	int fOK, ret;

	TDS_ENCODING *client = &conn->char_convs[client2ucs2]->from.charset;
	TDS_ENCODING *server = &conn->char_convs[client2ucs2]->to.charset;

	tdsdump_log(TDS_DBG_FUNC, "tds_iconv_open(%p, %s)\n", conn, charset);

	/* initialize */
	if (!iconv_initialized) {
		if ((ret = tds_iconv_init()) > 0) {
			static const char names[][12] = { "ISO 8859-1", "UTF-8" };
			assert(ret < 3);
			tdsdump_log(TDS_DBG_FUNC, "error: tds_iconv_init() returned %d; "
						  "could not find a name for %s that your iconv accepts.\n"
						  "use: \"configure --disable-libiconv\"", ret, names[ret-1]);
			assert(ret == 0);
			return;
		}
		iconv_initialized = 1;
	}

	/* 
	 * Client <-> UCS-2 (client2ucs2)
	 */
	tdsdump_log(TDS_DBG_FUNC, "setting up conversions for client charset \"%s\"\n", charset);

	tdsdump_log(TDS_DBG_FUNC, "preparing iconv for \"%s\" <-> \"%s\" conversion\n", charset, UCS_2LE);

	fOK = tds_iconv_info_init(conn->char_convs[client2ucs2], canonic_charset, TDS_CHARSET_UCS_2LE);
	if (!fOK)
		return;

	/* 
	 * How many UTF-8 bytes we need is a function of what the input character set is.
	 * TODO This could definitely be more sophisticated, but it deals with the common case.
	 */
	if (client->min_bytes_per_char == 1 && client->max_bytes_per_char == 4 && server->max_bytes_per_char == 1) {
		/* ie client is UTF-8 and server is ISO-8859-1 or variant. */
		client->max_bytes_per_char = 3;
	}

	/* 
	 * Client <-> server single-byte charset
	 * TODO: the server hasn't reported its charset yet, so this logic can't work here.  
	 *       not sure what to do about that yet.  
	 */
	conn->char_convs[client2server_chardata]->flags = TDS_ENCODING_MEMCPY;
	if (canonic_env_charset >= 0) {
		tdsdump_log(TDS_DBG_FUNC, "preparing iconv for \"%s\" <-> \"%s\" conversion\n", charset, conn->env.charset);
		fOK = tds_iconv_info_init(conn->char_convs[client2server_chardata], canonic_charset, canonic_env_charset);
		if (!fOK)
			return;
	} else {
		conn->char_convs[client2server_chardata]->from.charset = canonic_charsets[canonic_charset];
		conn->char_convs[client2server_chardata]->to.charset = canonic_charsets[canonic_charset];
	}

	/* 
	 * ISO8859-1 <-> server meta data
	 */
	canonic = TDS_CHARSET_UCS_2LE;
	if (!IS_TDS7_PLUS(conn)) {
		canonic = TDS_CHARSET_ISO_8859_1;
		if (canonic_env_charset >= 0)
			canonic = canonic_env_charset;
	}
	tdsdump_log(TDS_DBG_FUNC, "preparing iconv for \"%s\" <-> \"%s\" conversion\n", "ISO-8859-1", canonic_charsets[canonic].name);
	fOK = tds_iconv_info_init(conn->char_convs[iso2server_metadata], TDS_CHARSET_ISO_8859_1, canonic);

	tdsdump_log(TDS_DBG_FUNC, "tds_iconv_open: done\n");
}

/**
 * Open iconv descriptors to convert between character sets (both directions).
 * 1.  Look up the canonical names of the character sets.
 * 2.  Look up their widths.
 * 3.  Ask iconv to open a conversion descriptor.
 * 4.  Fail if any of the above offer any resistance.  
 * \remarks The charset names written to \a iconv will be the canonical names, 
 *          not necessarily the names passed in. 
 */
static int
tds_iconv_info_init(TDSICONV * char_conv, int client_canonical, int server_canonical)
{
	TDS_ENCODING *client = &char_conv->from.charset;
	TDS_ENCODING *server = &char_conv->to.charset;

	assert(char_conv->to.cd == (iconv_t) -1);
	assert(char_conv->to.cd2 == (iconv_t) -1);
	assert(char_conv->from.cd == (iconv_t) -1);
	assert(char_conv->from.cd2 == (iconv_t) -1);

	if (client_canonical < 0) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: client charset name \"%d\" invalid\n", client_canonical);
		return 0;
	}

	if (server_canonical < 0) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: server charset name \"%d\" invalid\n", server_canonical);
		return 0;
	}

	*client = canonic_charsets[client_canonical];
	*server = canonic_charsets[server_canonical];

	/* special case, same charset, no conversion */
	if (client_canonical == server_canonical) {
		char_conv->to.cd = (iconv_t) -1;
		char_conv->from.cd = (iconv_t) -1;
		char_conv->flags = TDS_ENCODING_MEMCPY;
		return 1;
	}

	char_conv->flags = 0;
	if (!iconv_names[server_canonical]) {
		switch (server_canonical) {
		case POS_UCS2LE:
			server_canonical = POS_UCS2BE;
			char_conv->flags = TDS_ENCODING_SWAPBYTE;
			break;
		case POS_UCS2BE:
			server_canonical = POS_UCS2LE;
			char_conv->flags = TDS_ENCODING_SWAPBYTE;
			break;
		}
	}

	/* get iconv names */
	if (!iconv_names[client_canonical]) {
		if (!tds_set_iconv_name(client_canonical)) {
			tdsdump_log(TDS_DBG_FUNC, "Charset %d not supported by iconv, using \"%s\" instead\n",
						  client_canonical, iconv_names[client_canonical]);
		}
	}
	
	if (!iconv_names[server_canonical]) {
		if (!tds_set_iconv_name(server_canonical)) {
			tdsdump_log(TDS_DBG_FUNC, "Charset %d not supported by iconv, using \"%s\" instead\n",
						  server_canonical, iconv_names[server_canonical]);
		}
	}

	char_conv->to.cd = tds_sys_iconv_open(iconv_names[server_canonical], iconv_names[client_canonical]);
	if (char_conv->to.cd == (iconv_t) -1) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: cannot convert \"%s\"->\"%s\"\n", client->name, server->name);
	}

	char_conv->from.cd = tds_sys_iconv_open(iconv_names[client_canonical], iconv_names[server_canonical]);
	if (char_conv->from.cd == (iconv_t) -1) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: cannot convert \"%s\"->\"%s\"\n", server->name, client->name);
	}

#if ENABLE_EXTRA_CHECKS
	if (tds_iconv_force_double)
		_iconv_close(&char_conv->from.cd);
#endif

	/* try indirect conversions */
	if (char_conv->to.cd == (iconv_t) -1 || char_conv->from.cd == (iconv_t) -1) {
		tds_iconv_info_close(char_conv);

		/* TODO reuse some conversion, client charset is usually constant in all connection (or ISO8859-1) */
		char_conv->to.cd = tds_sys_iconv_open(iconv_names[POS_UTF8], iconv_names[client_canonical]);
		char_conv->to.cd2 = tds_sys_iconv_open(iconv_names[server_canonical], iconv_names[POS_UTF8]);
		char_conv->from.cd = tds_sys_iconv_open(iconv_names[POS_UTF8], iconv_names[server_canonical]);
		char_conv->from.cd2 = tds_sys_iconv_open(iconv_names[client_canonical], iconv_names[POS_UTF8]);

		if (char_conv->to.cd == (iconv_t) -1 || char_conv->to.cd2 == (iconv_t) -1
		    || char_conv->from.cd == (iconv_t) -1 || char_conv->from.cd2 == (iconv_t) -1) {

			tds_iconv_info_close(char_conv);
			tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: cannot convert \"%s\"->\"%s\" indirectly\n",
				    server->name, client->name);
			return 0;
		}

		char_conv->flags |= TDS_ENCODING_INDIRECT;
	}
	
	/* TODO, do some optimizations like UCS2 -> UTF8 min,max = 2,2 (UCS2) and 1,4 (UTF8) */

	/* tdsdump_log(TDS_DBG_FUNC, "tds_iconv_info_init: converting \"%s\"->\"%s\"\n", client->name, server->name); */

	return 1;
}


static void
_iconv_close(iconv_t * cd)
{
	static const iconv_t invalid = (iconv_t) -1;

	if (*cd != invalid) {
		tds_sys_iconv_close(*cd);
		*cd = invalid;
	}
}

static void
tds_iconv_info_close(TDSICONV * char_conv)
{
	_iconv_close(&char_conv->to.cd);
	_iconv_close(&char_conv->to.cd2);
	_iconv_close(&char_conv->from.cd);
	_iconv_close(&char_conv->from.cd2);
}

void
tds_iconv_close(TDSCONNECTION * conn)
{
	int i;

	for (i = 0; i < conn->char_conv_count; ++i)
		tds_iconv_info_close(conn->char_convs[i]);
}

#define CHUNK_ALLOC 4

void
tds_iconv_free(TDSCONNECTION * conn)
{
	int i;

	if (!conn->char_convs)
		return;
	tds_iconv_close(conn);

	free(conn->char_convs[0]);
	for (i = initial_char_conv_count + 1; i < conn->char_conv_count; i += CHUNK_ALLOC)
		free(conn->char_convs[i]);
	TDS_ZERO_FREE(conn->char_convs);
	conn->char_conv_count = 0;
}

static size_t
tds_iconv_indirect(TDSICONVDIR *from, TDSICONVDIR *to, int *peilseq_raised, const char **pb1, size_t * pil1, char **ob3, size_t * pil3)
{
#if ENABLE_EXTRA_CHECKS
	char tmp[8];
#else
	char tmp[256];
#endif
	char *pb2;
	size_t il1, il2, l;
	int temp_errno;
	size_t temp_irreversible, irreversible;

	do {
		pb2 = tmp;
		il2 = sizeof(tmp);

		if (to->num_left) {
			if (*pil1 < to->num_got) {
				errno = EINVAL;
				return (size_t) -1;
			}
			*pil1 -= to->num_got;
			*pb1  += to->num_got;
			to->num_got = 0;

			memcpy(tmp, to->left, to->num_left);
			pb2 += to->num_left;
			il2 -= to->num_left;
			to->num_left = 0;
		}

		/* compute maximum to translate, making sure we don't get too much 
		 * we can't read more characters than we can put into final output, we have two
		 * consecutive conversion n * m1..M1 -> n * m2..M2 -> n * m3..M3
		 * (n number of characters, m minimum, M maximum), we want to limit n * M3
		 * to final output length, so maximum input bytes = n * m1, so
		 *   inputlen = n * m1 = outlen / M3 * m1
		 * We must handle the case where we must read a characters but we are
		 * not sure it fit into output. Ie 1..2 -> 2..4 if we limit input to 1
		 * and output to 2 could be that source characters occupy 2 bytes but
		 * output encoding take 4 bytes. In this case we should record the position
		 * to return, try to read at least one characters and if we cannot translate
		 * store it for next step.
		 */
		il1 = *pil1;
		l = *pil3 * from->charset.min_bytes_per_char / to->charset.max_bytes_per_char;
		if (il1 > l)
			il1 = l;

		errno = 0;
		l = il1;
		temp_irreversible = tds_sys_iconv(to->cd, (ICONV_CONST char **) pb1, &il1, &pb2, &il2);
		*pil1 -= l - il1;
		temp_errno = errno;

		/* here are the tricky part, assure we read at least one characters */
		il2 = pb2 - tmp;
		pb2 = tmp;
		if (il2 == 0) {
			l = from->charset.min_bytes_per_char;
			while (temp_errno == EINVAL && l < from->charset.max_bytes_per_char && l < *pil1) {
				il1 = l;
				il2 = sizeof(to->left);
				pb2 = to->left;

				errno = 0;
				l = il1;
				temp_irreversible = tds_sys_iconv(to->cd, (ICONV_CONST char **) pb1, &il1, &pb2, &il2);
				to->num_got = l - il1;
				if (to->num_got) *pb1 -= to->num_got;
				to->num_left = il2 = sizeof(to->left) - il2;
				pb2 = to->left;
				temp_errno = errno;
				if (to->num_left)
					break;
				++l;
			}
		} else {
			/* avoid EINVAL cause we make input shorter */
			temp_errno = E2BIG;
		}

		/* convert partial */
		for (;;) {
			errno = 0;
			irreversible = tds_sys_iconv(to->cd2, (ICONV_CONST char **) &pb2, &il2, ob3, pil3);

			/* check if we consumed single character from input */
			if (to->num_left && il2 == 0) {
				*pil1 -= to->num_got;
				*pb1  += to->num_got;
				to->num_got = 0;

				to->num_left = 0;
			}
			if (irreversible != (size_t) - 1) {
				if (pil1 && *pil1)
					break;
				return irreversible;
			}
			/* EINVAL should be impossible, all characters came from previous iconv... */
			if (errno == E2BIG || errno == EINVAL)
				return irreversible;

			/*
			 * error should be EILSEQ, not convertible sequence 
			 * skip UTF-8 sequence, replace with '?'
			 */
			/* avoid infinite recursion */
			*peilseq_raised = 1;
			if (*pb2 == '?')
				return irreversible;
			*pb2 = (char) 0x80;
			while (il2 && (*pb2 & 0xC0) == 0x80)
				++pb2, --il2;
			--pb2;
			++il2;
			*pb2 = '?';
		}
	} while (temp_errno == E2BIG);
	errno = temp_errno;
	return temp_irreversible;
}

static size_t
tds_iconv_swap(iconv_t cd, const char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft)
{
	/* swap bytes if necessary */
#if ENABLE_EXTRA_CHECKS
	char tmp[8];
#else
	char tmp[256];
#endif
	char *pib;
	size_t il, n, irreversible;

	do {
		pib = tmp;
		il = *inbytesleft > sizeof(tmp) ? sizeof(tmp) : *inbytesleft;
		for (n = 0; n < il; n += 2) {
			tmp[n] = (*inbuf)[n + 1];
			tmp[n + 1] = (*inbuf)[n];
		}
		irreversible = tds_sys_iconv(cd, (ICONV_CONST char **) &pib, &il, outbuf, outbytesleft);
		il = pib - tmp;
		*inbuf += il;
		*inbytesleft -= il;
	} while (irreversible != (size_t) - 1 && *inbytesleft);

	return irreversible;
}

static void
tds_iconv_err(TDSSOCKET *tds, int err)
{
	if (tds)
		tdserror(tds_get_ctx(tds), tds, err, 0);
}

/** 
 * Wrapper around iconv(3).  Same parameters, with slightly different behavior.
 * \param tds state information for the socket and the TDS protocol
 * \param io Enumerated value indicating whether the data are being sent to or received from the server. 
 * \param conv information about the encodings involved, including the iconv(3) conversion descriptors. 
 * \param inbuf address of pointer to the input buffer of data to be converted.  
 * \param inbytesleft address of count of bytes in \a inbuf.
 * \param outbuf address of pointer to the output buffer.  
 * \param outbytesleft address of count of bytes in \a outbuf.
 * \retval number of irreversible conversions performed.  -1 on error, see iconv(3) documentation for 
 * a description of the possible values of \e errno.  
 * \remarks Unlike iconv(3), none of the arguments can be nor point to NULL.  Like iconv(3), all pointers will 
 *  	be updated.  Success is signified by a nonnegative return code and \a *inbytesleft == 0.  
 * 	If the conversion descriptor in \a iconv is -1 or NULL, \a inbuf is copied to \a outbuf, 
 *	and all parameters updated accordingly. 
 * 
 * 	If a character in \a inbuf cannot be converted because no such cbaracter exists in the
 * 	\a outbuf character set, we emit messages similar to the ones Sybase emits when it fails such a conversion. 
 * 	The message varies depending on the direction of the data.  
 * 	On a read error, we emit Msg 2403, Severity 16 (EX_INFO):
 * 		"WARNING! Some character(s) could not be converted into client's character set. 
 *			Unconverted bytes were changed to question marks ('?')."
 * 	On a write error we emit Msg 2402, Severity 16 (EX_USER):
 *		"Error converting client characters into server's character set. Some character(s) could not be converted."
 *  	  and return an error code.  Client libraries relying on this routine should reflect an error back to the application.  
 *
 * \todo Check for variable multibyte non-UTF-8 input character set.  
 * \todo Use more robust error message generation.  
 * \todo For reads, cope with \a outbuf encodings that don't have the equivalent of an ASCII '?'.  
 * \todo Support alternative to '?' for the replacement character.  
 */
size_t
tds_iconv(TDSSOCKET * tds, TDSICONV * conv, TDS_ICONV_DIRECTION io,
	  const char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft)
{
	static const iconv_t invalid = (iconv_t) -1;
	TDSICONVDIR *from = NULL;
	TDSICONVDIR *to = NULL;

	iconv_t error_cd = invalid;

	char quest_mark[] = "?";	/* best to leave non-const; implementations vary */
	ICONV_CONST char *pquest_mark = quest_mark;
	size_t lquest_mark;
	size_t irreversible;
	size_t one_character;
	char *p;
	int eilseq_raised = 0;
	int conv_errno;
	/* cast away const-ness */
	TDS_ERRNO_MESSAGE_FLAGS *suppress = (TDS_ERRNO_MESSAGE_FLAGS*) &conv->suppress;

	assert(inbuf && inbytesleft && outbuf && outbytesleft);

	/* if empty there's nothing to return.
	 * This fix case with some iconv implementation that does
	 * not handle *inbuf == NULL and *inbytesleft == 0 as
	 * empty strings
	 */
	if (*inbytesleft == 0)
		return 0;

	switch (io) {
	case to_server:
		from = &conv->from;
		to = &conv->to;
		break;
	case to_client:
		from = &conv->to;
		to = &conv->from;
		break;
	default:
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv: unable to determine if %d means in or out.  \n", io);
		assert(io == to_server || io == to_client);
		break;
	}

	/* silly case, memcpy */
	if (conv->flags & TDS_ENCODING_MEMCPY || to->cd == invalid) {
		size_t len = *inbytesleft < *outbytesleft ? *inbytesleft : *outbytesleft;

		memcpy(*outbuf, *inbuf, len);
		errno = *inbytesleft > *outbytesleft ? E2BIG : 0;
		*inbytesleft -= len;
		*outbytesleft -= len;
		*inbuf += len;
		*outbuf += len;
		return 0;
	}

	/*
	 * Call iconv() as many times as necessary, until we reach the end of input or exhaust output.  
	 */
	p = *outbuf;
	for (;;) {
		conv_errno = 0;
		if (conv->flags & TDS_ENCODING_INDIRECT) {
			irreversible = tds_iconv_indirect(from, to, &eilseq_raised, inbuf, inbytesleft, outbuf, outbytesleft);
		} else if (io == to_client && conv->flags & TDS_ENCODING_SWAPBYTE && inbuf) {
			irreversible = tds_iconv_swap(to->cd, inbuf, inbytesleft, outbuf, outbytesleft);
		} else {
			irreversible = tds_sys_iconv(to->cd, (ICONV_CONST char **) inbuf, inbytesleft, outbuf, outbytesleft);
		}

		/* iconv success, return */
		if (irreversible != (size_t) - 1) {
			/* here we detect end of conversion and try to reset shift state */
			if (inbuf) {
				/*
				 * if inbuf or *inbuf is NULL iconv reset the shift state.
				 * Note that setting inbytesleft to NULL can cause core so don't do it!
				 */
				inbuf = NULL;
				continue;
			}
			break;
		}

		/* save errno, other function could change its value */
		conv_errno = errno;

		if (conv_errno == EILSEQ)
			eilseq_raised = 1;

		if (conv_errno != EILSEQ || io != to_client || !inbuf)
			break;
		/* 
		 * Invalid input sequence encountered reading from server. 
		 * Skip one input sequence, adjusting pointers. 
		 */
		one_character = skip_one_input_sequence(to->cd, &from->charset, inbuf, inbytesleft);

		if (!one_character)
			break;

		/* 
		 * To replace invalid input with '?', we have to convert a UTF-8 '?' into the output character set.  
		 * In unimaginably weird circumstances, this might be impossible.
		 * We use UTF-8 instead of ASCII because some implementations 
		 * do not convert singlebyte <-> singlebyte.
		 */
		if (error_cd == invalid) {
			error_cd = tds_sys_iconv_open(to->charset.name, iconv_names[POS_UTF8]);
			if (error_cd == invalid) {
				break;	/* what to do? */
			}
		}

		lquest_mark = 1;
		pquest_mark = quest_mark;

		p = *outbuf;
		irreversible = tds_sys_iconv(error_cd, &pquest_mark, &lquest_mark, outbuf, outbytesleft);

		if (irreversible == (size_t) - 1)
			break;

		if (!*inbytesleft)
			break;
	}

	/* swap bytes if necessary */
	if (io == to_server && conv->flags & TDS_ENCODING_SWAPBYTE) {
		assert((*outbuf - p) % 2 == 0);
		for (; p < *outbuf; p += 2) {
			char tmp = p[0];

			p[0] = p[1];
			p[1] = tmp;
		}
	}

	if (eilseq_raised && !suppress->eilseq) {
		/* invalid multibyte input sequence encountered */
		if (io == to_client) {
			if (irreversible == (size_t) - 1) {
				tds_iconv_err(tds, TDSEICONV2BIG);
			} else {
				tds_iconv_err(tds, TDSEICONVI);
				conv_errno = 0;
			}
		} else {
			tds_iconv_err(tds, TDSEICONVO);
		}
		suppress->eilseq = 1;
	}

	switch (conv_errno) {
	case EINVAL:		/* incomplete multibyte sequence is encountered */
		if (suppress->einval)
			break;
		/* in chunk conversion this can mean we end a chunk inside a character */
		tds_iconv_err(tds, TDSEICONVAVAIL);
		suppress->einval = 1;
		break;
	case E2BIG:		/* output buffer has no more room */
		if (suppress->e2big)
			break;
		tds_iconv_err(tds, TDSEICONVIU);
		suppress->e2big = 1;
		break;
	default:
		break;
	}

	if (error_cd != invalid) {
		tds_sys_iconv_close(error_cd);
	}

	errno = conv_errno;
	return irreversible;
}

/**
 * Get a iconv info structure, allocate and initialize if needed
 */
static TDSICONV *
tds_iconv_get_info(TDSCONNECTION * conn, int canonic_client, int canonic_server)
{
	TDSICONV *info;
	int i;

	/* search a charset from already allocated charsets */
	for (i = conn->char_conv_count; --i >= initial_char_conv_count;)
		if (canonic_client == conn->char_convs[i]->from.charset.canonic
		    && canonic_server == conn->char_convs[i]->to.charset.canonic)
			return conn->char_convs[i];

	/* allocate a new iconv structure */
	if (conn->char_conv_count % CHUNK_ALLOC == ((initial_char_conv_count + 1) % CHUNK_ALLOC)) {
		TDSICONV **p;
		TDSICONV *infos;

		infos = (TDSICONV *) malloc(sizeof(TDSICONV) * CHUNK_ALLOC);
		if (!infos)
			return NULL;
		p = (TDSICONV **) realloc(conn->char_convs, sizeof(TDSICONV *) * (conn->char_conv_count + CHUNK_ALLOC));
		if (!p) {
			free(infos);
			return NULL;
		}
		conn->char_convs = p;
		memset(infos, 0, sizeof(TDSICONV) * CHUNK_ALLOC);
		for (i = 0; i < CHUNK_ALLOC; ++i) {
			conn->char_convs[i + conn->char_conv_count] = &infos[i];
			tds_iconv_reset(&infos[i]);
		}
	}
	info = conn->char_convs[conn->char_conv_count++];

	/* init */
	if (tds_iconv_info_init(info, canonic_client, canonic_server))
		return info;

	tds_iconv_info_close(info);
	--conn->char_conv_count;
	return NULL;
}

TDSICONV *
tds_iconv_get(TDSCONNECTION * conn, const char *client_charset, const char *server_charset)
{
	int canonic_client_charset_num = tds_canonical_charset(client_charset);
	int canonic_server_charset_num = tds_canonical_charset(server_charset);

	if (canonic_client_charset_num < 0) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_get: what is charset \"%s\"?\n", client_charset);
		return NULL;
	}
	if (canonic_server_charset_num < 0) {
		tdsdump_log(TDS_DBG_FUNC, "tds_iconv_get: what is charset \"%s\"?\n", server_charset);
		return NULL;
	}

	return tds_iconv_get_info(conn, canonic_client_charset_num, canonic_server_charset_num);
}

/* change singlebyte conversions according to server */
static void
tds_srv_charset_changed_num(TDSCONNECTION * conn, int canonic_charset_num)
{
	TDSICONV *char_conv = conn->char_convs[client2server_chardata];

	if (IS_TDS7_PLUS(conn) && canonic_charset_num == TDS_CHARSET_ISO_8859_1)
		canonic_charset_num = TDS_CHARSET_CP1252;

	tdsdump_log(TDS_DBG_FUNC, "setting server single-byte charset to \"%s\"\n", canonic_charsets[canonic_charset_num].name);

	if (canonic_charset_num == char_conv->to.charset.canonic)
		return;

	/* find and set conversion */
	char_conv = tds_iconv_get_info(conn, conn->char_convs[client2ucs2]->from.charset.canonic, canonic_charset_num);
	if (char_conv)
		conn->char_convs[client2server_chardata] = char_conv;

	/* if sybase change also server conversions */
	if (IS_TDS7_PLUS(conn))
		return;

	char_conv = conn->char_convs[iso2server_metadata];

	tds_iconv_info_close(char_conv);

	tds_iconv_info_init(char_conv, TDS_CHARSET_ISO_8859_1, canonic_charset_num);
}

void
tds_srv_charset_changed(TDSCONNECTION * conn, const char *charset)
{
	int n = tds_canonical_charset(charset);

	/* ignore request to change to unknown charset */
	if (n < 0) {
		tdsdump_log(TDS_DBG_FUNC, "tds_srv_charset_changed: what is charset \"%s\"?\n", charset);
		return;
	}

	tds_srv_charset_changed_num(conn, n);
}

/* change singlebyte conversions according to server */
void
tds7_srv_charset_changed(TDSCONNECTION * conn, int sql_collate, int lcid)
{
	tds_srv_charset_changed_num(conn, collate2charset(sql_collate, lcid));
}

/**
 * Move the input sequence pointer to the next valid position.
 * Used when an input character cannot be converted.  
 * \returns number of bytes to skip.
 */
/* FIXME possible buffer reading overflow ?? */
static size_t
skip_one_input_sequence(iconv_t cd, const TDS_ENCODING * charset, const char **input, size_t * input_size)
{
	int charsize = CHARSIZE(charset);
	char ib[16];
	char ob[16];
	ICONV_CONST char *pib;
	char *pob;
	size_t il, ol, l;
	iconv_t cd2;


	/* usually fixed size and UTF-8 do not have state, so do not reset it */
	if (charsize) {
		if (charsize > *input_size)
			return 0;
		*input += charsize;
		*input_size -= charsize;
		return charsize;
	}

	if (0 == strcmp(charset->name, "UTF-8")) {
		/*
		 * Deal with UTF-8.  
		 * bytes | bits | representation
		 *     1 |    7 | 0vvvvvvv
		 *     2 |   11 | 110vvvvv 10vvvvvv
		 *     3 |   16 | 1110vvvv 10vvvvvv 10vvvvvv
		 *     4 |   21 | 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
		 */
		int c = **input;

		c = c & (c >> 1);
		do {
			++charsize;
		} while ((c <<= 1) & 0x80);
		if (charsize > *input_size)
			return 0;
		*input += charsize;
		*input_size -= charsize;
		return charsize;
	}

	/* handle state encoding */

	/* extract state from iconv */
	pob = ib;
	ol = sizeof(ib);
	tds_sys_iconv(cd, NULL, NULL, &pob, &ol);

	/* init destination conversion */
	/* TODO use largest fixed size for this platform */
	cd2 = tds_sys_iconv_open("UCS-4", charset->name);
	if (cd2 == (iconv_t) -1)
		return 0;

	/* add part of input */
	il = ol;
	if (il > *input_size)
		il = *input_size;
	l = sizeof(ib) - ol;
	memcpy(ib + l, *input, il);
	il += l;

	/* translate a single character */
	pib = ib;
	pob = ob;
	/* TODO use size of largest fixed charset */
	ol = 4;
	tds_sys_iconv(cd2, &pib, &il, &pob, &ol);

	/* adjust input */
	l = (pib - ib) - l;
	*input += l;
	*input_size -= l;

	/* extract state */
	pob = ib;
	ol = sizeof(ib);
	tds_sys_iconv(cd, NULL, NULL, &pob, &ol);

	/* set input state */
	pib = ib;
	il = sizeof(ib) - ol;
	pob = ob;
	ol = sizeof(ob);
	tds_sys_iconv(cd, &pib, &il, &pob, &ol);

	tds_sys_iconv_close(cd2);

	return l;
}

static int
lookup_canonic(const CHARACTER_SET_ALIAS aliases[], const char *charset_name)
{
	int i;

	for (i = 0; aliases[i].alias; ++i) {
		if (0 == strcmp(charset_name, aliases[i].alias))
			return aliases[i].canonic;
	}

	return -1;
}

/**
 * Determine canonical iconv character set.
 * \returns canonical position, or -1 if lookup failed.
 * \remarks Returned name can be used in bytes_per_char(), above.
 */
static int
tds_canonical_charset(const char *charset_name)
{
	int res;

	/* search in alternative */
	res = lookup_canonic(iconv_aliases, charset_name);
	if (res >= 0)
		return res;

	/* search in sybase */
	return lookup_canonic(sybase_aliases, charset_name);
}

/**
 * Determine canonical iconv character set name.  
 * \returns canonical name, or NULL if lookup failed.
 * \remarks Returned name can be used in bytes_per_char(), above.
 */
const char *
tds_canonical_charset_name(const char *charset_name)
{
	int res;

	/* get numeric pos */
	res = tds_canonical_charset(charset_name);
	if (res >= 0)
		return canonic_charsets[res].name;

	return charset_name;	/* hope for the best */
}

static int
collate2charset(int sql_collate, int lcid)
{
	/*
	 * The table from the MSQLServer reference "Windows Collation Designators" 
	 * and from " NLS Information for Microsoft Windows XP"
	 */

	int cp = 0;

	switch (sql_collate) {
	case 30:		/* SQL_Latin1_General_CP437_BIN */
	case 31:		/* SQL_Latin1_General_CP437_CS_AS */
	case 32:		/* SQL_Latin1_General_CP437_CI_AS */
	case 33:		/* SQL_Latin1_General_Pref_CP437_CI_AS */
	case 34:		/* SQL_Latin1_General_CP437_CI_AI */
		return TDS_CHARSET_CP437;
	case 40:		/* SQL_Latin1_General_CP850_BIN */
	case 41:		/* SQL_Latin1_General_CP850_CS_AS */
	case 42:		/* SQL_Latin1_General_CP850_CI_AS */
	case 43:		/* SQL_Latin1_General_Pref_CP850_CI_AS */
	case 44:		/* SQL_Latin1_General_CP850_CI_AI */
	case 49:		/* SQL_1xCompat_CP850_CI_AS */
	case 55:		/* SQL_AltDiction_CP850_CS_AS */
	case 56:		/* SQL_AltDiction_Pref_CP850_CI_AS */
	case 57:		/* SQL_AltDiction_CP850_CI_AI */
	case 58:		/* SQL_Scandinavian_Pref_CP850_CI_AS */
	case 59:		/* SQL_Scandinavian_CP850_CS_AS */
	case 60:		/* SQL_Scandinavian_CP850_CI_AS */
	case 61:		/* SQL_AltDiction_CP850_CI_AS */
		return TDS_CHARSET_CP850;
	case 80:		/* SQL_Latin1_General_1250_BIN */
	case 81:		/* SQL_Latin1_General_CP1250_CS_AS */
	case 82:		/* SQL_Latin1_General_CP1250_CI_AS */
		return TDS_CHARSET_CP1250;
	case 105:		/* SQL_Latin1_General_CP1251_CS_AS */
	case 106:		/* SQL_Latin1_General_CP1251_CI_AS */
		return TDS_CHARSET_CP1251;
	case 113:		/* SQL_Latin1_General_CP1253_CS_AS */
	case 114:		/* SQL_Latin1_General_CP1253_CI_AS */
	case 120:		/* SQL_MixDiction_CP1253_CS_AS */
	case 121:		/* SQL_AltDiction_CP1253_CS_AS */
	case 122:		/* SQL_AltDiction2_CP1253_CS_AS */
	case 124:		/* SQL_Latin1_General_CP1253_CI_AI */
		return TDS_CHARSET_CP1253;
	case 137:		/* SQL_Latin1_General_CP1255_CS_AS */
	case 138:		/* SQL_Latin1_General_CP1255_CI_AS */
		return TDS_CHARSET_CP1255;
	case 145:		/* SQL_Latin1_General_CP1256_CS_AS */
	case 146:		/* SQL_Latin1_General_CP1256_CI_AS */
		return TDS_CHARSET_CP1256;
	case 153:		/* SQL_Latin1_General_CP1257_CS_AS */
	case 154:		/* SQL_Latin1_General_CP1257_CI_AS */
		return TDS_CHARSET_CP1257;
	}

	switch (lcid & 0xffff) {
	case 0x405:
	case 0x40e:		/* 0x1040e */
	case 0x415:
	case 0x418:
	case 0x41a:
	case 0x41b:
	case 0x41c:
	case 0x424:
		/* case 0x81a: seem wrong in XP table TODO check */
	case 0x104e:		/* ?? */
		cp = TDS_CHARSET_CP1250;
		break;
	case 0x402:
	case 0x419:
	case 0x422:
	case 0x423:
	case 0x42f:
	case 0x43f:
	case 0x440:
	case 0x444:
	case 0x450:
	case 0x81a:		/* ?? */
	case 0x82c:
	case 0x843:
	case 0xc1a:
		cp = TDS_CHARSET_CP1251;
		break;
	case 0x1007:
	case 0x1009:
	case 0x100a:
	case 0x100c:
	case 0x1407:
	case 0x1409:
	case 0x140a:
	case 0x140c:
	case 0x1809:
	case 0x180a:
	case 0x180c:
	case 0x1c09:
	case 0x1c0a:
	case 0x2009:
	case 0x200a:
	case 0x2409:
	case 0x240a:
	case 0x2809:
	case 0x280a:
	case 0x2c09:
	case 0x2c0a:
	case 0x3009:
	case 0x300a:
	case 0x3409:
	case 0x340a:
	case 0x380a:
	case 0x3c0a:
	case 0x400a:
	case 0x403:
	case 0x406:
	case 0x407:		/* 0x10407 */
	case 0x409:
	case 0x40a:
	case 0x40b:
	case 0x40c:
	case 0x40f:
	case 0x410:
	case 0x413:
	case 0x414:
	case 0x416:
	case 0x41d:
	case 0x421:
	case 0x42d:
	case 0x436:
	case 0x437:		/* 0x10437 */
	case 0x438:
		/*case 0x439:  ??? Unicode only */
	case 0x43e:
	case 0x440a:
	case 0x441:
	case 0x456:
	case 0x480a:
	case 0x4c0a:
	case 0x500a:
	case 0x807:
	case 0x809:
	case 0x80a:
	case 0x80c:
	case 0x810:
	case 0x813:
	case 0x814:
	case 0x816:
	case 0x81d:
	case 0x83e:
	case 0xc07:
	case 0xc09:
	case 0xc0a:
	case 0xc0c:
		cp = TDS_CHARSET_CP1252;
		break;
	case 0x408:
		cp = TDS_CHARSET_CP1253;
		break;
	case 0x41f:
	case 0x42c:
	case 0x443:
		cp = TDS_CHARSET_CP1254;
		break;
	case 0x40d:
		cp = TDS_CHARSET_CP1255;
		break;
	case 0x1001:
	case 0x1401:
	case 0x1801:
	case 0x1c01:
	case 0x2001:
	case 0x2401:
	case 0x2801:
	case 0x2c01:
	case 0x3001:
	case 0x3401:
	case 0x3801:
	case 0x3c01:
	case 0x4001:
	case 0x401:
	case 0x420:
	case 0x429:
	case 0x801:
	case 0xc01:
		cp = TDS_CHARSET_CP1256;
		break;
	case 0x425:
	case 0x426:
	case 0x427:
	case 0x827:		/* ?? */
		cp = TDS_CHARSET_CP1257;
		break;
	case 0x42a:
		cp = TDS_CHARSET_CP1258;
		break;
	case 0x41e:
		cp = TDS_CHARSET_CP874;
		break;
	case 0x411:		/* 0x10411 */
		cp = TDS_CHARSET_CP932;
		break;
	case 0x1004:
	case 0x804:		/* 0x20804 */
		cp = TDS_CHARSET_CP936;
		break;
	case 0x412:		/* 0x10412 */
		cp = TDS_CHARSET_CP949;
		break;
	case 0x1404:
	case 0x404:		/* 0x30404 */
	case 0xc04:
		cp = TDS_CHARSET_CP950;
		break;
	default:
		cp = TDS_CHARSET_CP1252;
	}

	return cp;
}

/**
 * Get iconv information from a LCID (to support different column encoding under MSSQL2K)
 */
TDSICONV *
tds_iconv_from_collate(TDSCONNECTION * conn, TDS_UCHAR collate[5])
{
	const int sql_collate = collate[4];
	const int lcid = collate[1] * 256 + collate[0];
	int canonic_charset = collate2charset(sql_collate, lcid);

	/* same as client (usually this is true, so this improve performance) ? */
	if (conn->char_convs[client2server_chardata]->to.charset.canonic == canonic_charset)
		return conn->char_convs[client2server_chardata];

	return tds_iconv_get_info(conn, conn->char_convs[client2ucs2]->from.charset.canonic, canonic_charset);
}

/** @} */
