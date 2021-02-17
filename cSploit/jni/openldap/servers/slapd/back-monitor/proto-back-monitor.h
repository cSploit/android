/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#ifndef _PROTO_BACK_MONITOR
#define _PROTO_BACK_MONITOR

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

/*
 * backends
 */
int
monitor_subsys_backend_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * cache
 */
extern int
monitor_cache_cmp LDAP_P((
	const void		*c1,
	const void		*c2 ));
extern int
monitor_cache_dup LDAP_P((
	void			*c1,
	void			*c2 ));
extern int
monitor_cache_add LDAP_P((
	monitor_info_t		*mi,
	Entry			*e ));
extern int
monitor_cache_get LDAP_P((
	monitor_info_t		*mi,
	struct berval		*ndn,
	Entry			**ep ));
extern int
monitor_cache_remove LDAP_P((
	monitor_info_t		*mi,
	struct berval		*ndn,
	Entry			**ep ));
extern int
monitor_cache_dn2entry LDAP_P((
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry			**ep,
	Entry			**matched ));
extern int
monitor_cache_lock LDAP_P((
	Entry			*e ));
extern int
monitor_cache_release LDAP_P((
	monitor_info_t		*mi,
	Entry			*e ));

extern int
monitor_cache_destroy LDAP_P((
	monitor_info_t		*mi ));

extern int
monitor_back_release(
	Operation *op,
	Entry *e,
	int rw );

/*
 * connections
 */
extern int
monitor_subsys_conn_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * databases 
 */
extern int
monitor_subsys_database_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * entry
 */
extern int
monitor_entry_update LDAP_P((
	Operation		*op,
	SlapReply		*rs,
	Entry			*e ));
extern int
monitor_entry_create LDAP_P((
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry			*e_parent,
	Entry			**ep ));
extern int
monitor_entry_modify LDAP_P((
	Operation		*op,
	SlapReply		*rs,
	Entry			*e ));
extern int
monitor_entry_test_flags LDAP_P((
	monitor_entry_t		*mp,
	int			cond ));
extern monitor_entry_t *
monitor_back_entrypriv_create LDAP_P((
	void ));
extern Entry *
monitor_back_entry_stub LDAP_P((
	struct berval	*pdn,
	struct berval	*pndn,
	struct berval	*rdn,
	ObjectClass		*oc,
	struct berval	*create,
	struct berval	*modify ));

#define monitor_entrypriv_create monitor_back_entrypriv_create
#define monitor_entry_stub monitor_back_entry_stub

/*
 * init
 */
extern int
monitor_subsys_is_opened LDAP_P((
	void ));
extern int
monitor_back_register_subsys LDAP_P((
	monitor_subsys_t	*ms ));
extern int
monitor_back_register_subsys_late LDAP_P((
	monitor_subsys_t	*ms ));
extern int
monitor_back_register_backend LDAP_P((
	BackendInfo		*bi ));
extern int
monitor_back_register_database LDAP_P((
	BackendDB		*be,
	struct berval	*ndn ));
extern int
monitor_back_register_overlay_info LDAP_P((
	slap_overinst		*on ));
extern int
monitor_back_register_overlay LDAP_P((
	BackendDB		*be,
	struct slap_overinst	*on,
	struct berval		*ndn_out ));
extern int
monitor_back_register_backend_limbo LDAP_P((
	BackendInfo		*bi ));
extern int
monitor_back_register_database_limbo LDAP_P((
	BackendDB		*be,
	struct berval		*ndn_out ));
extern int
monitor_back_register_overlay_info_limbo LDAP_P((
	slap_overinst		*on ));
extern int
monitor_back_register_overlay_limbo LDAP_P((
	BackendDB		*be,
	struct slap_overinst	*on,
	struct berval		*ndn_out ));
extern monitor_subsys_t *
monitor_back_get_subsys LDAP_P((
	const char		*name ));
extern monitor_subsys_t *
monitor_back_get_subsys_by_dn LDAP_P((
	struct berval		*ndn,
	int			sub ));
extern int
monitor_back_is_configured LDAP_P(( void ));
extern int
monitor_back_register_entry LDAP_P((
	Entry			*e,
	monitor_callback_t	*cb,
	monitor_subsys_t	*mss,
	unsigned long		flags ));
extern int
monitor_back_register_entry_parent LDAP_P((
	Entry			*e,
	monitor_callback_t	*cb,
	monitor_subsys_t	*mss,
	unsigned long		flags,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));
extern int
monitor_search2ndn LDAP_P((
	struct berval		*base,
	int			scope,
	struct berval		*filter,
	struct berval		*ndn ));
extern int
monitor_back_register_entry_attrs LDAP_P((
	struct berval		*ndn,
	Attribute		*a,
	monitor_callback_t	*cb,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));
extern int
monitor_back_register_entry_callback LDAP_P((
	struct berval		*ndn,
	monitor_callback_t	*cb,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));
extern int
monitor_back_unregister_entry LDAP_P((
	struct berval		*ndn ));
extern int
monitor_back_unregister_entry_parent LDAP_P((
	struct berval		*nrdn,
	monitor_callback_t	*target_cb,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));
extern int
monitor_back_unregister_entry_attrs LDAP_P((
	struct berval		*ndn,
	Attribute		*a,
	monitor_callback_t	*cb,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));
extern int
monitor_back_unregister_entry_callback LDAP_P((
	struct berval		*ndn,
	monitor_callback_t	*cb,
	struct berval		*base,
	int			scope,
	struct berval		*filter ));

/*
 * listener
 */
extern int
monitor_subsys_listener_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * log
 */
extern int
monitor_subsys_log_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * operations
 */
extern int
monitor_subsys_ops_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * overlay
 */
extern int
monitor_subsys_overlay_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * sent
 */
extern int
monitor_subsys_sent_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * threads
 */
extern int
monitor_subsys_thread_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * time
 */
extern int monitor_subsys_time_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * waiters
 */
extern int
monitor_subsys_rww_init LDAP_P((
	BackendDB		*be,
	monitor_subsys_t	*ms ));

/*
 * former external.h
 */

extern BI_init			monitor_back_initialize;

extern BI_db_init		monitor_back_db_init;
extern BI_db_open		monitor_back_db_open;
extern BI_config		monitor_back_config;
extern BI_db_destroy		monitor_back_db_destroy;
extern BI_db_config		monitor_back_db_config;

extern BI_op_search		monitor_back_search;
extern BI_op_compare		monitor_back_compare;
extern BI_op_modify		monitor_back_modify;
extern BI_op_bind		monitor_back_bind;
extern BI_operational		monitor_back_operational;

LDAP_END_DECL

#endif /* _PROTO_BACK_MONITOR */

