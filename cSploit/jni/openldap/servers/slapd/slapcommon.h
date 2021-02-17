/* slapcommon.h - common definitions for the slap tools */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef SLAPCOMMON_H_
#define SLAPCOMMON_H_ 1

#define SLAPD_TOOLS 1
#include "slap.h"

enum slaptool {
	SLAPADD=1,	/* LDIF -> database tool */
	SLAPCAT,	/* database -> LDIF tool */
	SLAPDN,		/* DN check w/ syntax tool */
	SLAPINDEX,	/* database index tool */
	SLAPMODIFY,	/* database modify tool */
	SLAPPASSWD,	/* password generation tool */
	SLAPSCHEMA,	/* schema checking tool */
	SLAPTEST,	/* slapd.conf test tool */
	SLAPAUTH,	/* test authz-regexp and authc/authz stuff */
	SLAPACL,	/* test acl */
	SLAPLAST
};

typedef struct tool_vars {
	Backend *tv_be;
	int tv_dbnum;
	int tv_verbose;
	int tv_quiet;
	int tv_update_ctxcsn;
	int tv_continuemode;
	int tv_nosubordinates;
	int tv_dryrun;
	int tv_scope;
	unsigned long tv_jumpline;
	struct berval tv_sub_ndn;
	Filter *tv_filter;
	struct LDIFFP	*tv_ldiffp;
	struct berval tv_baseDN;
	struct berval tv_authcDN;
	struct berval tv_authzDN;
	struct berval tv_authcID;
	struct berval tv_authzID;
	struct berval tv_mech;
	char	*tv_realm;
	struct berval tv_listener_url;
	struct berval tv_peer_domain;
	struct berval tv_peer_name;
	struct berval tv_sock_name;
	slap_ssf_t tv_ssf;
	slap_ssf_t tv_transport_ssf;
	slap_ssf_t tv_tls_ssf;
	slap_ssf_t tv_sasl_ssf;
	unsigned tv_dn_mode;
	unsigned int tv_csnsid;
	ber_len_t tv_ldif_wrap;
	char tv_maxcsnbuf[ LDAP_PVT_CSNSTR_BUFSIZE * ( SLAP_SYNC_SID_MAX + 1 ) ];
	struct berval tv_maxcsn[ SLAP_SYNC_SID_MAX + 1 ];
} tool_vars;

extern tool_vars tool_globals;

#define	be tool_globals.tv_be
#define	dbnum tool_globals.tv_dbnum
#define verbose tool_globals.tv_verbose
#define quiet tool_globals.tv_quiet
#define jumpline tool_globals.tv_jumpline
#define update_ctxcsn tool_globals.tv_update_ctxcsn
#define continuemode tool_globals.tv_continuemode
#define nosubordinates tool_globals.tv_nosubordinates
#define dryrun tool_globals.tv_dryrun
#define sub_ndn tool_globals.tv_sub_ndn
#define scope tool_globals.tv_scope
#define filter tool_globals.tv_filter
#define ldiffp tool_globals.tv_ldiffp
#define baseDN tool_globals.tv_baseDN
#define authcDN tool_globals.tv_authcDN
#define authzDN tool_globals.tv_authzDN
#define authcID tool_globals.tv_authcID
#define authzID tool_globals.tv_authzID
#define mech tool_globals.tv_mech
#define realm tool_globals.tv_realm
#define listener_url tool_globals.tv_listener_url
#define peer_domain tool_globals.tv_peer_domain
#define peer_name tool_globals.tv_peer_name
#define sock_name tool_globals.tv_sock_name
#define ssf tool_globals.tv_ssf
#define transport_ssf tool_globals.tv_transport_ssf
#define tls_ssf tool_globals.tv_tls_ssf
#define sasl_ssf tool_globals.tv_sasl_ssf
#define dn_mode tool_globals.tv_dn_mode
#define csnsid tool_globals.tv_csnsid
#define ldif_wrap tool_globals.tv_ldif_wrap
#define maxcsn tool_globals.tv_maxcsn
#define maxcsnbuf tool_globals.tv_maxcsnbuf

#define SLAP_TOOL_LDAPDN_PRETTY		SLAP_LDAPDN_PRETTY
#define SLAP_TOOL_LDAPDN_NORMAL		(SLAP_LDAPDN_PRETTY << 1)

void slap_tool_init LDAP_P((
	const char* name,
	int tool,
	int argc, char **argv ));

int slap_tool_destroy LDAP_P((void));

int slap_tool_update_ctxcsn LDAP_P((
	const char *progname,
	unsigned long sid,
	struct berval *bvtext ));

unsigned long slap_tool_update_ctxcsn_check LDAP_P((
	const char *progname,
	Entry *e ));

int slap_tool_update_ctxcsn_init LDAP_P((void));

int slap_tool_entry_check LDAP_P((
	const char *progname,
	Operation *op,
	Entry *e,
	int lineno,
	const char **text,
	char *textbuf,
	size_t textlen ));

#endif /* SLAPCOMMON_H_ */
