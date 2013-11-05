/*
    strops.c
    Copyright (C) 1996-2001  Petr Vandrovec

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Revision history:

	see ncplib.c 1996-2000		Petr Vandrovec <vandrove@vc.cvut.cz>
		str*error functions
		
	see filemgmt.c 2000, May 17	Bruce Richardson <brichardson@lineone.net>
		Added ncp_perms_to_str and ncp_str_to_perms.

	1.00  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved str* functions from ncplib.c here.
		Moved ncp*perms* functions from filemgmt.c here.

	1.01  2001, February 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added #include <stdarg.h>. Someone broke something somewhere...

	1.02  2001, February 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added some new error codes.
		
	1.03  2002, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWE_VOL_INVALID, NWE_DIRHANDLE_INVALID.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <string.h>
#include <stdarg.h>

#include "private/libintl.h"
#define _(X) dgettext(NCPFS_PACKAGE, (X))
#define N_(X) (X)

#define ncp_array_size(x) (sizeof(x)/sizeof((x)[0]))

struct errxlat {
	int ecode;
	const char* estr;
};

static const char*
strdserror(int err)
{
	static const struct errxlat dserrors[] = {
		/* keep list sorted! Maybe I'll use bsearch someday */
		{ERR_NOT_ENOUGH_MEMORY,		/* -301 */
			N_("Not enough memory")},
		{ERR_BAD_KEY,			/* -302 */
			N_("Bad key passed to NWDS{Get|Set}Context")},
		{ERR_BAD_CONTEXT,		/* -303 */
			N_("Invalid context handle")},
		{ERR_BUFFER_FULL,		/* -304 */
			N_("Buffer full")},
		/* ... */
		{ERR_BAD_SYNTAX,		/* -306 */
			N_("Bad syntax")},
		{ERR_BUFFER_EMPTY,		/* -307 */
			N_("Buffer empty")}, /* Buffer underflow? Buffer exhausted? */
		{ERR_BAD_VERB,			/* -308 */
			N_("Bad verb")},
		{ERR_EXPECTED_IDENTIFIER,	/* -309 */
			N_("Expected identifier")},
		/* ... */
		{ERR_ATTR_TYPE_EXPECTED,	/* -311 */
			N_("Attribute type expected")},
		{ERR_ATTR_TYPE_NOT_EXPECTED,	/* -312 */
			N_("Attribute type not expected")},
		{ERR_FILTER_TREE_EMPTY,		/* -313 */
			N_("Filter tree empty")},
		{ERR_INVALID_OBJECT_NAME,	/* -314 */
			N_("Invalid object name")},
		{ERR_EXPECTED_RDN_DELIMITER,	/* -315 */
			N_("Expected RDN delimiter")},
		{ERR_TOO_MANY_TOKENS,		/* -316 */
			N_("Too many tokens")},
		{ERR_INCONSISTENT_MULTIAVA,	/* -317 */
			N_("Inconsistent multiava")}, /* multi attribute value?! */
		{ERR_COUNTRY_NAME_TOO_LONG,	/* -318 */
			N_("Country name too long")},
		{ERR_SYSTEM_ERROR,		/* -319 */
			N_("System error")},
		/* ... */
		{ERR_INVALID_HANDLE,		/* -322 */
			N_("Invalid iteration handle")},
		{ERR_BUFFER_ZERO_LENGTH,	/* -323 */
			N_("Empty buffer passed to API")},
		/* ... */
		{ERR_CONTEXT_CREATION,		/* -328 */
			N_("Cannot create context")},
		/* ... */
		{ERR_INVALID_SERVER_RESPONSE,	/* -330 */
			N_("Invalid server response")},
		{ERR_NULL_POINTER,		/* -331 */
			N_("NULL pointer seen")},
		/* ... */
		{ERR_NO_CONNECTION,		/* -333 */
			N_("No connection exists")},
		{ERR_RDN_TOO_LONG,		/* -334 */
			N_("RDN too long")},
		{ERR_DUPLICATE_TYPE,		/* -335 */
			N_("Duplicate type")},
		/* ... */
		{ERR_NOT_LOGGED_IN,		/* -337 */
			N_("Not logged in")},
		{ERR_INVALID_PASSWORD_CHARS,	/* -338 */
			N_("Invalid password characters")},
		/* ... */
		{ERR_TRANSPORT,			/* -340 */
			N_("Bad transport")},
		{ERR_NO_SUCH_SYNTAX,		/* -341 */
			N_("No such syntax")},
		{ERR_INVALID_DS_NAME,		/* -342 */
			N_("Invalid DS name")},
		/* ... */
		{ERR_UNICODE_FILE_NOT_FOUND,	/* -348 */
			N_("Required unicode translation not available")},
		/* ... */
		{ERR_DN_TOO_LONG,		/* -353 */
			N_("DN too long")},
		{ERR_RENAME_NOT_ALLOWED,	/* -354 */
			N_("Rename not allowed")},
		/* ... */
		{ERR_NO_SUCH_ENTRY,		/* -601 */
			N_("No such entry")},
		/* ... */
		{ERR_NO_SUCH_ATTRIBUTE,		/* -603 */
			N_("No such attribute")},
		/* ... */
		{ERR_TRANSPORT_FAILURE,		/* -625 */
			N_("Transport failure")},
		{ERR_ALL_REFERRALS_FAILED,	/* -626 */
			N_("All referrals failed")},
		/* ... */
		{ERR_NO_REFERRALS,		/* -634 */
			N_("No referrals")},
		{ERR_REMOTE_FAILURE,		/* -635 */
			N_("Remote failure")},
		{ERR_UNREACHABLE_SERVER,	/* -636 */
			N_("Unreachable server")},
		/* ... */
		{ERR_INVALID_REQUEST,		/* -641 */
			N_("Invalid request")},
		/* ... */
		{ERR_CRUCIAL_REPLICA,		/* -656 */
			N_("Crucial replica")},
		/* ... */
		{ERR_TIME_NOT_SYNCHRONIZED,	/* -659 */
			N_("Time not synchronized")},
		/* ... */
		{ERR_FAILED_AUTHENTICATION,	/* -669 */
			N_("Invalid password")},
		/* ... */
		{ERR_ALIAS_OF_AN_ALIAS,		/* -681 */
			N_("Alias of an alias")},
		/* ... */
		{ERR_INVALID_API_VERSION,	/* -683 */
			N_("Invalid API version")},
		{ERR_SECURE_NCP_VIOLATION,	/* -684 */
			N_("Packet signatures required")},
		/* ... */
		{ERR_OBSOLETE_API,		/* -700 */
			N_("Obsolete API")},
		/* ... */
		{ERR_INVALID_SIGNATURE,		/* -707 */
			N_("Invalid signature")},
		/* ... */
		{-9999,
			NULL}};
	const struct errxlat* eptr = dserrors;
	const char* msg = N_("Unknown NDS error");
		
	if (err > -9999) {
		while (eptr->ecode > err) eptr++;
		if (eptr->ecode == err)
			msg = eptr->estr;
	}
	{
		static char sbuf[256];

		sprintf(sbuf, "%s (%d)", _(msg), err);
		return sbuf;
	}
}

static const char*
strncperror(int err)
{
	static const struct errxlat rqerrors[] = {
		/* keep list sorted! Maybe I'll use bsearch someday */
		{NCPLIB_INVALID_MODE,		/* 8701 */
			N_("Invalid file mode")},
		{NCPLIB_INFORMATION_NOT_KNOWN,	/* 8702 */
			N_("Information not known")},
		{NCPLIB_NSFORMAT_INVALID,	/* 8703 */
			N_("Namespace information format is not valid")},
		{NCPLIB_REFERRAL_NEEDED,	/* 8704 */
			N_("Referral needed")},
		{NCPLIB_NCPD_DEAD,		/* 8705 */
			N_("Permanent connection broken")},
		{NCPLIB_PASSWORD_REQUIRED,	/* 8706 */
			N_("Password required")},
		/* ... */
		{0x10000,
			NULL}};
	const struct errxlat* eptr = rqerrors;
	const char* msg = N_("Unknown ncpfs error");
		
	if (err < 0x10000) {
		while (eptr->ecode < err) eptr++;
		if (eptr->ecode == err)
			msg = eptr->estr;
	}
	{
		static char sbuf[256];

		sprintf(sbuf, "%s (0x%04X)", _(msg), err);
		return sbuf;
	}
}

static const char*
strrqerror(int err)
{
	static const struct errxlat rqerrors[] = {
		/* keep list sorted! Maybe I'll use bsearch someday */
		{NWE_REQ_TOO_MANY_REQ_FRAGS,	/* 880C */
			N_("Too many request/reply fragments")},
		/* ... */
		{NWE_BUFFER_OVERFLOW,		/* 880E */
			N_("Server reply too long")},
		{NWE_SERVER_NO_CONN,		/* 880F */
			N_("Connection to specified server does not exist")},
		/* ... */
		{NWE_SCAN_COMPLETE,		/* 8812 */
			N_("Scan complete")},
		{NWE_UNSUPPORTED_NAME_FORMAT_TYP, /* 8813 */
			N_("Unsupported name format type")},
		/* ... */
		{NWE_INVALID_NCP_PACKET_LENGTH,	/* 8816 */
			N_("Invalid NCP packet length")},
		/* ... */
		{NWE_BUFFER_INVALID_LEN,	/* 8833 */
			N_("Invalid buffer length")},
		{NWE_USER_NO_NAME,		/* 8834 */
			N_("User name is not specified")},
		/* ... */
		{NWE_PARAM_INVALID,		/* 8836 */
			N_("Invalid parameter")},
		/* ... */
		{NWE_SERVER_NOT_FOUND,		/* 8847 */
			N_("Server not found")},
		/* ... */
		{NWE_SIGNATURE_LEVEL_CONFLICT,	/* 8861 */
			N_("Signature level conflict")},
		/* ... */
		{NWE_INVALID_LEVEL,		/* 886B */
			N_("Invalid information level")},
		/* ... */
		{NWE_UNSUPPORTED_TRAN_TYPE,	/* 8870 */
			N_("Unsupported transport type")},
		/* ... */
		{NWE_UNSUPPORTED_AUTHENTICATOR,	/* 8873 */
			N_("Unsupported authenticator")},
		/* ... */
		{0x10000,
			NULL}};
	const struct errxlat* eptr = rqerrors;
	const char* msg = N_("Unknown Requester error");
		
	if (err < 0x10000) {
		while (eptr->ecode < err) eptr++;
		if (eptr->ecode == err)
			msg = eptr->estr;
	}
	{
		static char sbuf[256];

		sprintf(sbuf, "%s (0x%04X)", _(msg), err);
		return sbuf;
	}
}

static const char*
strsrverror(int err)
{
	static const struct errxlat srverrors[] = {
		/* keep list sorted! Maybe I'll use bsearch someday */
		{NWE_VOL_INVALID,			/* 8998 */
			N_("Invalid volume")},
		/* ... */
		{NWE_DIRHANDLE_INVALID,			/* 899B */
			N_("Invalid directory handle")},
		/* ... */
		{NWE_LOGIN_LOCKOUT,			/* 89C5 */
			N_("Intruder detection lockout")},
		/* ... */
		{NWE_Q_NO_JOB,				/* 89D5 */
			N_("No job in queue")},
		{NWE_PASSWORD_UNENCRYPTED,		/* 89D6 */
			N_("Password unencrypted")},
		{NWE_PASSWORD_NOT_UNIQUE,		/* 89D7 */
			N_("Password not unique")},
		{NWE_PASSWORD_TOO_SHORT,		/* 89D8 */
			N_("Password too short")},
		{NWE_LOGIN_MAX_EXCEEDED,		/* 89D9 */
			N_("Connection limit count exceeded")},
		{NWE_LOGIN_UNAUTHORIZED_TIME,		/* 89DA */
			N_("Unauthorized time")},
		{NWE_LOGIN_UNAUTHORIZED_STATION,	/* 89DB */
			N_("Unauthorized station")},
		{NWE_ACCT_DISABLED,			/* 89DC */
			N_("Account disabled")},
		/* ... */
		{NWE_PASSWORD_INVALID,			/* 89DE */
			N_("Password really expired")}, /* no grace logins left */
		{NWE_PASSWORD_EXPIRED,			/* 89DF */
			N_("Password expired")},
		/* ... */
		{NWE_BIND_MEMBER_ALREADY_EXISTS,	/* 89E9 */
			N_("Member already exists")},
		/* ... */
		{NWE_NCP_NOT_SUPPORTED,			/* 89FB */
			N_("NCP not supported")},
		{NWE_SERVER_UNKNOWN,			/* 89FC */
			N_("Unknown user")},
		{NWE_CONN_NUM_INVALID,			/* 89FD */
			N_("Invalid connection number")},
		/* ... */
		{NWE_SERVER_FAILURE,			/* 89FF */
			N_("Server failure")},
		/* ... */
		{0x10000,
			NULL}};
	const struct errxlat* eptr = srverrors;
	const char* msg = N_("Unknown Server error");
		
	if (err < 0x10000) {
		while (eptr->ecode < err) eptr++;
		if (eptr->ecode == err)
			msg = eptr->estr;
	}
	{
		static char sbuf[256];

		sprintf(sbuf, "%s (0x%04X)", _(msg), err);
		return sbuf;
	}
}

const char*
strnwerror(int err)
{
	if (err < 0)
		return strdserror(err);
	else if (err < 0x8700)
		return strerror(err);
	else if (err < 0x8800)
		return strncperror(err);
	else if (err < 0x8900)
		return strrqerror(err);
	else if (err < 0x8A00)
		return strsrverror(err);
	else {
		static char sbuf[100];
		
		sprintf(sbuf, _("Unknown error %d (0x%X)"), err, err);
		return sbuf;
	}
}

void
com_err(const char* prog, int err, const char* msg, ...)
{
	if (prog)
		fprintf(stderr, "%s: ", prog);
	fprintf(stderr, "%s ", strnwerror(err));
	if (msg) {
		va_list va;
		
		va_start(va, msg);
		vfprintf(stderr, msg, va);
		va_end(va);
	}
	fprintf(stderr, "\n");
}

char* ncp_perms_to_str(char r[11], const u_int16_t rights)
{
        r[0] = '[';
        r[1] = ((rights & NCP_PERM_SUPER) != 0) ? 'S' : ' ';
        r[2] = ((rights & NCP_PERM_READ) != 0) ? 'R' : ' ';
        r[3] = ((rights & NCP_PERM_WRITE) != 0) ? 'W' : ' ';
        r[4] = ((rights & NCP_PERM_CREATE) != 0) ? 'C' : ' ';
        r[5] = ((rights & NCP_PERM_DELETE) != 0) ? 'E' : ' ';
        r[6] = ((rights & NCP_PERM_MODIFY) != 0) ? 'M' : ' ';
        r[7] = ((rights & NCP_PERM_SEARCH) != 0) ? 'F' : ' ';
        r[8] = ((rights & NCP_PERM_OWNER) != 0) ? 'A' : ' ';
        r[9] = ']';
        r[10] = '\0';
	return r;
}

/* The following function converts a rights string of format [SRWCEMFA]
   into an integer.  It will tolerate spaces, lower case and repeated 
   letters, even if this takes the length well over 10 characters, but 
   must be terminated with square brackets.  If such a string containing 
   spaces is given as a command line option it will have to be quoted. */

int ncp_str_to_perms(const char *r, u_int16_t *rights)
{
	u_int16_t result = 0;

	if (*r == '[') {
		do {
			++r;
			switch (*r) {
				case ' ' : 
				case ']' :
					break;
				case 's' :
				case 'S' : 
					result |= NCP_PERM_SUPER; break;
				case 'r' :
				case 'R' : 
					result |= NCP_PERM_READ; break;
				case 'w' :
				case 'W' : 
					result |= NCP_PERM_WRITE; break;
				case 'c' :
				case 'C' : 
					result |= NCP_PERM_CREATE; break;
				case 'e' :
				case 'E' : 
					result |= NCP_PERM_DELETE; break;
				case 'm' :
				case 'M' : 
					result |= NCP_PERM_MODIFY; break;
				case 'f' :
				case 'F' : 
					result |= NCP_PERM_SEARCH; break;
				case 'a' :
				case 'A' : 
					result |= NCP_PERM_OWNER; break;
				default :
					return -1;
			}
		} while (*r != ']');
		/* Now to be generous and ignore trailing spaces */
		do { ++r; } while (*r == ' ');
		if (*r == '\0') { 
			*rights = result; 
			return 0;
		}
	}
	return -1;
}

const char* ncp_namespace_to_str(char r[5], unsigned int ns) {
	static const char* namespaces[] = { "DOS", 	/* 0: NW_NS_DOS */
					    "MAC", 	/* 1: NW_NS_MAC */
					    "NFS",	/* 2: NW_NS_NFS */ 
					    "FTAM", 	/* 3: NW_NS_FTAM */
					    "LONG", 	/* 4: NW_NS_LONG */
					    "NT", 	/* 5: NW_NS_NT */
					    "RSVD" };
	if (ns >= ncp_array_size(namespaces)) {
		ns = ncp_array_size(namespaces) - 1;
	}
	if (r) {
		strcpy(r, namespaces[ns]);
		return r;
	} else {
		return namespaces[ns];
	}
}
