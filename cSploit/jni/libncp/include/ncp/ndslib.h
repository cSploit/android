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

#ifndef _NDSLIB_H_
#define _NDSLIB_H_

#include <ncp/ncplib.h>

#define ERR_NOT_ENOUGH_MEMORY		-301
#define ERR_BAD_KEY			-302
#define ERR_BAD_CONTEXT			-303
#define ERR_BUFFER_FULL			-304

#define ERR_BAD_SYNTAX			-306
#define ERR_BUFFER_EMPTY		-307
#define ERR_BAD_VERB			-308
#define ERR_EXPECTED_IDENTIFIER		-309

#define ERR_ATTR_TYPE_EXPECTED		-311
#define ERR_ATTR_TYPE_NOT_EXPECTED	-312
#define ERR_FILTER_TREE_EMPTY		-313
#define ERR_INVALID_OBJECT_NAME		-314
#define ERR_EXPECTED_RDN_DELIMITER	-315
#define ERR_TOO_MANY_TOKENS		-316
#define ERR_INCONSISTENT_MULTIAVA	-317
#define ERR_COUNTRY_NAME_TOO_LONG	-318
#define ERR_SYSTEM_ERROR		-319

#define ERR_INVALID_HANDLE		-322
#define ERR_BUFFER_ZERO_LENGTH		-323

#define ERR_CONTEXT_CREATION		-328

#define ERR_INVALID_SERVER_RESPONSE	-330
#define ERR_NULL_POINTER		-331

#define ERR_NO_CONNECTION		-333
#define ERR_RDN_TOO_LONG		-334
#define ERR_DUPLICATE_TYPE		-335

#define ERR_NOT_LOGGED_IN		-337
#define ERR_INVALID_PASSWORD_CHARS	-338

#define ERR_TRANSPORT			-340
#define ERR_NO_SUCH_SYNTAX		-341
#define ERR_INVALID_DS_NAME		-342

#define ERR_UNICODE_FILE_NOT_FOUND	-348

#define ERR_DN_TOO_LONG			-353
#define ERR_RENAME_NOT_ALLOWED		-354

#define ERR_NO_SUCH_ENTRY		-601

#define ERR_NO_SUCH_ATTRIBUTE		-603

#define ERR_TRANSPORT_FAILURE		-625
#define ERR_ALL_REFERRALS_FAILED	-626

#define ERR_NO_REFERRALS		-634
#define ERR_REMOTE_FAILURE		-635
#define ERR_UNREACHABLE_SERVER		-636

#define ERR_INVALID_REQUEST		-641

#define ERR_CRUCIAL_REPLICA		-656

#define ERR_TIME_NOT_SYNCHRONIZED	-659

#define ERR_FAILED_AUTHENTICATION	-669

#define ERR_ALIAS_OF_AN_ALIAS		-681

#define ERR_INVALID_API_VERSION		-683
#define ERR_SECURE_NCP_VIOLATION	-684

#define ERR_OBSOLETE_API		-700

#define ERR_INVALID_SIGNATURE		-707

#ifdef NCP_OBSOLETE
#include <ncp/obsolete/o_ndslib.h>
#endif

long nds_login_auth(NWCONN_HANDLE conn, const char *user, const char *pwd);

#endif /* ifndef _NDSLIB_H_ */
