/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#ifndef PROTO_LDAP_H
#define PROTO_LDAP_H

LDAP_BEGIN_DECL

extern BI_init			ldap_back_initialize;
extern BI_open			ldap_back_open;

extern BI_db_init		ldap_back_db_init;
extern BI_db_open		ldap_back_db_open;
extern BI_db_close		ldap_back_db_close;
extern BI_db_destroy		ldap_back_db_destroy;

extern BI_op_bind		ldap_back_bind;
extern BI_op_search		ldap_back_search;
extern BI_op_compare		ldap_back_compare;
extern BI_op_modify		ldap_back_modify;
extern BI_op_modrdn		ldap_back_modrdn;
extern BI_op_add		ldap_back_add;
extern BI_op_delete		ldap_back_delete;
extern BI_op_abandon		ldap_back_abandon;
extern BI_op_extended		ldap_back_extended;

extern BI_connection_destroy	ldap_back_conn_destroy;

extern BI_entry_get_rw		ldap_back_entry_get;

void ldap_back_release_conn_lock( ldapinfo_t *li, ldapconn_t **lcp, int dolock );
#define ldap_back_release_conn(li, lc) ldap_back_release_conn_lock((li), &(lc), 1)
int ldap_back_dobind( ldapconn_t **lcp, Operation *op, SlapReply *rs, ldap_back_send_t sendok );
int ldap_back_retry( ldapconn_t **lcp, Operation *op, SlapReply *rs, ldap_back_send_t sendok );
int ldap_back_map_result( SlapReply *rs );
int ldap_back_op_result( ldapconn_t *lc, Operation *op, SlapReply *rs,
	ber_int_t msgid, time_t timeout, ldap_back_send_t sendok );
int ldap_back_cancel( ldapconn_t *lc, Operation *op, SlapReply *rs, ber_int_t msgid, ldap_back_send_t sendok );

int ldap_back_init_cf( BackendInfo *bi );
int ldap_pbind_init_cf( BackendInfo *bi );

extern int ldap_back_conndn_cmp( const void *c1, const void *c2);
extern int ldap_back_conn_cmp( const void *c1, const void *c2);
extern int ldap_back_conndn_dup( void *c1, void *c2 );
extern void ldap_back_conn_free( void *c );

extern ldapconn_t * ldap_back_conn_delete( ldapinfo_t *li, ldapconn_t *lc );

extern int ldap_back_conn2str( const ldapconn_base_t *lc, char *buf, ber_len_t buflen );
extern int ldap_back_connid2str( const ldapconn_base_t *lc, char *buf, ber_len_t buflen );

extern int
ldap_back_proxy_authz_ctrl(
		Operation	*op,
		SlapReply	*rs,
		struct berval	*bound_ndn,
		int		version,
		slap_idassert_t	*si,
		LDAPControl	*ctrl );

extern int
ldap_back_controls_add(
		Operation	*op,
		SlapReply	*rs,
		ldapconn_t	*lc,
		LDAPControl	***pctrls );

extern int
ldap_back_controls_free( Operation *op, SlapReply *rs, LDAPControl ***pctrls );

extern void
ldap_back_quarantine(
	Operation	*op,
	SlapReply	*rs );

#ifdef LDAP_BACK_PRINT_CONNTREE
extern void
ldap_back_print_conntree( ldapinfo_t *li, char *msg );
#endif /* LDAP_BACK_PRINT_CONNTREE */

extern void slap_retry_info_destroy( slap_retry_info_t *ri );
extern int slap_retry_info_parse( char *in, slap_retry_info_t *ri,
	char *buf, ber_len_t buflen );
extern int slap_retry_info_unparse( slap_retry_info_t *ri, struct berval *bvout );

extern int slap_idassert_authzfrom_parse( struct config_args_s *ca, slap_idassert_t *si );
extern int slap_idassert_passthru_parse_cf( const char *fname, int lineno, const char *arg, slap_idassert_t *si );
extern int slap_idassert_parse( struct config_args_s *ca, slap_idassert_t *si );

extern int chain_initialize( void );
extern int pbind_initialize( void );
#ifdef SLAP_DISTPROC
extern int distproc_initialize( void );
#endif

extern int ldap_back_monitor_db_init( BackendDB *be );
extern int ldap_back_monitor_db_open( BackendDB *be );
extern int ldap_back_monitor_db_close( BackendDB *be );
extern int ldap_back_monitor_db_destroy( BackendDB *be );

extern LDAP_REBIND_PROC		ldap_back_default_rebind;
extern LDAP_URLLIST_PROC	ldap_back_default_urllist;

LDAP_END_DECL

#endif /* PROTO_LDAP_H */
