/* back-monitor.h - ldap monitor back-end header file */
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

#ifndef _BACK_MONITOR_H_
#define _BACK_MONITOR_H_

#include <ldap_pvt.h>
#include <ldap_pvt_thread.h>
#include <avl.h>
#include <slap.h>

LDAP_BEGIN_DECL

/* define if si_ad_labeledURI is removed from slap_schema */
#undef MONITOR_DEFINE_LABELEDURI

typedef struct monitor_callback_t {
	int				(*mc_update)( Operation *op, SlapReply *rs, Entry *e, void *priv );
						/* update callback
						   for user-defined entries */
	int				(*mc_modify)( Operation *op, SlapReply *rs, Entry *e, void *priv );
						/* modify callback
						   for user-defined entries */
	int				(*mc_free)( Entry *e, void **priv );
						/* delete callback
						   for user-defined entries */
	void				(*mc_dispose)( void **priv );
						/* dispose callback
						   to dispose of the callback
						   private data itself */
	void				*mc_private;	/* opaque pointer to
						   private data */
	struct monitor_callback_t	*mc_next;
} monitor_callback_t;


typedef struct monitor_entry_t {
	ldap_pvt_thread_mutex_t	mp_mutex;	/* entry mutex */
	Entry			*mp_next;	/* pointer to next sibling */
	Entry			*mp_children;	/* pointer to first child */
	struct monitor_subsys_t	*mp_info;	/* subsystem info */
#define mp_type		mp_info->mss_type
	unsigned long		mp_flags;	/* flags */

#define	MONITOR_F_NONE		0x0000U
#define MONITOR_F_SUB		0x0001U		/* subentry of subsystem */
#define MONITOR_F_PERSISTENT	0x0010U		/* persistent entry */
#define MONITOR_F_PERSISTENT_CH	0x0020U		/* subsystem generates 
						   persistent entries */
#define MONITOR_F_VOLATILE	0x0040U		/* volatile entry */
#define MONITOR_F_VOLATILE_CH	0x0080U		/* subsystem generates 
						   volatile entries */
#define MONITOR_F_EXTERNAL	0x0100U		/* externally added - don't free */
/* NOTE: flags with 0xF0000000U mask are reserved for subsystem internals */

	struct monitor_callback_t	*mp_cb;		/* callback sequence */
} monitor_entry_t;

struct entry_limbo_t;			/* in init.c */

typedef struct monitor_info_t {

	/*
	 * Internal data
	 */
	Avlnode			*mi_cache;
	ldap_pvt_thread_mutex_t	mi_cache_mutex;

	/*
	 * Config parameters
	 */
	struct berval		mi_startTime;		/* don't free it! */
	struct berval		mi_creatorsName;	/* don't free it! */
	struct berval		mi_ncreatorsName;	/* don't free it! */

	/*
	 * Specific schema entities
	 */
	ObjectClass		*mi_oc_monitor;
	ObjectClass		*mi_oc_monitorServer;
	ObjectClass		*mi_oc_monitorContainer;
	ObjectClass		*mi_oc_monitorCounterObject;
	ObjectClass		*mi_oc_monitorOperation;
	ObjectClass		*mi_oc_monitorConnection;
	ObjectClass		*mi_oc_managedObject;
	ObjectClass		*mi_oc_monitoredObject;

	AttributeDescription	*mi_ad_monitoredInfo;
	AttributeDescription	*mi_ad_managedInfo;
	AttributeDescription	*mi_ad_monitorCounter;
	AttributeDescription	*mi_ad_monitorOpCompleted;
	AttributeDescription	*mi_ad_monitorOpInitiated;
	AttributeDescription	*mi_ad_monitorConnectionNumber;
	AttributeDescription	*mi_ad_monitorConnectionAuthzDN;
	AttributeDescription	*mi_ad_monitorConnectionLocalAddress;
	AttributeDescription	*mi_ad_monitorConnectionPeerAddress;
	AttributeDescription	*mi_ad_monitorTimestamp;
	AttributeDescription	*mi_ad_monitorOverlay;
	AttributeDescription	*mi_ad_monitorConnectionProtocol;
	AttributeDescription	*mi_ad_monitorConnectionOpsReceived;
	AttributeDescription	*mi_ad_monitorConnectionOpsExecuting;
	AttributeDescription	*mi_ad_monitorConnectionOpsPending;
	AttributeDescription	*mi_ad_monitorConnectionOpsCompleted;
	AttributeDescription	*mi_ad_monitorConnectionGet;
	AttributeDescription	*mi_ad_monitorConnectionRead;
	AttributeDescription	*mi_ad_monitorConnectionWrite;
	AttributeDescription	*mi_ad_monitorConnectionMask;
	AttributeDescription	*mi_ad_monitorConnectionListener;
	AttributeDescription	*mi_ad_monitorConnectionPeerDomain;
	AttributeDescription	*mi_ad_monitorConnectionStartTime;
	AttributeDescription	*mi_ad_monitorConnectionActivityTime;
	AttributeDescription	*mi_ad_monitorIsShadow;
	AttributeDescription	*mi_ad_monitorUpdateRef;
	AttributeDescription	*mi_ad_monitorRuntimeConfig;
	AttributeDescription	*mi_ad_monitorSuperiorDN;

	/*
	 * Generic description attribute
	 */
	AttributeDescription	*mi_ad_readOnly;
	AttributeDescription	*mi_ad_restrictedOperation;

	struct entry_limbo_t	*mi_entry_limbo;
} monitor_info_t;

/*
 * DNs
 */

enum {
	SLAPD_MONITOR_BACKEND = 0,
	SLAPD_MONITOR_CONN,
	SLAPD_MONITOR_DATABASE,
	SLAPD_MONITOR_LISTENER,
	SLAPD_MONITOR_LOG,
	SLAPD_MONITOR_OPS,
	SLAPD_MONITOR_OVERLAY,
	SLAPD_MONITOR_SASL,
	SLAPD_MONITOR_SENT,
	SLAPD_MONITOR_THREAD,
	SLAPD_MONITOR_TIME,
	SLAPD_MONITOR_TLS,
	SLAPD_MONITOR_RWW,

	SLAPD_MONITOR_LAST
};

#define SLAPD_MONITOR_AT		"cn"

#define SLAPD_MONITOR_BACKEND_NAME	"Backends"
#define SLAPD_MONITOR_BACKEND_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_BACKEND_NAME
#define SLAPD_MONITOR_BACKEND_DN	\
	SLAPD_MONITOR_BACKEND_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_CONN_NAME		"Connections"
#define SLAPD_MONITOR_CONN_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_CONN_NAME
#define SLAPD_MONITOR_CONN_DN	\
	SLAPD_MONITOR_CONN_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_DATABASE_NAME	"Databases"
#define SLAPD_MONITOR_DATABASE_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_DATABASE_NAME
#define SLAPD_MONITOR_DATABASE_DN	\
	SLAPD_MONITOR_DATABASE_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_LISTENER_NAME	"Listeners"
#define SLAPD_MONITOR_LISTENER_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_LISTENER_NAME
#define SLAPD_MONITOR_LISTENER_DN	\
	SLAPD_MONITOR_LISTENER_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_LOG_NAME		"Log"
#define SLAPD_MONITOR_LOG_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_LOG_NAME
#define SLAPD_MONITOR_LOG_DN	\
	SLAPD_MONITOR_LOG_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_OPS_NAME		"Operations"
#define SLAPD_MONITOR_OPS_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_OPS_NAME
#define SLAPD_MONITOR_OPS_DN	\
	SLAPD_MONITOR_OPS_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_OVERLAY_NAME	"Overlays"
#define SLAPD_MONITOR_OVERLAY_RDN  \
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_OVERLAY_NAME
#define SLAPD_MONITOR_OVERLAY_DN   \
	SLAPD_MONITOR_OVERLAY_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_SASL_NAME		"SASL"
#define SLAPD_MONITOR_SASL_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_SASL_NAME
#define SLAPD_MONITOR_SASL_DN	\
	SLAPD_MONITOR_SASL_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_SENT_NAME		"Statistics"
#define SLAPD_MONITOR_SENT_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_SENT_NAME
#define SLAPD_MONITOR_SENT_DN	\
	SLAPD_MONITOR_SENT_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_THREAD_NAME	"Threads"
#define SLAPD_MONITOR_THREAD_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_THREAD_NAME
#define SLAPD_MONITOR_THREAD_DN	\
	SLAPD_MONITOR_THREAD_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_TIME_NAME		"Time"
#define SLAPD_MONITOR_TIME_RDN  \
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_TIME_NAME
#define SLAPD_MONITOR_TIME_DN   \
	SLAPD_MONITOR_TIME_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_TLS_NAME		"TLS"
#define SLAPD_MONITOR_TLS_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_TLS_NAME
#define SLAPD_MONITOR_TLS_DN	\
	SLAPD_MONITOR_TLS_RDN "," SLAPD_MONITOR_DN

#define SLAPD_MONITOR_RWW_NAME		"Waiters"
#define SLAPD_MONITOR_RWW_RDN	\
	SLAPD_MONITOR_AT "=" SLAPD_MONITOR_RWW_NAME
#define SLAPD_MONITOR_RWW_DN	\
	SLAPD_MONITOR_RWW_RDN "," SLAPD_MONITOR_DN

typedef struct monitor_subsys_t {
	char		*mss_name;
	struct berval	mss_rdn;
	struct berval	mss_dn;
	struct berval	mss_ndn;
	struct berval	mss_desc[ 3 ];
	int		mss_flags;
#define MONITOR_F_OPENED	0x10000000U

#define MONITOR_HAS_VOLATILE_CH( mp ) \
	( ( mp )->mp_flags & MONITOR_F_VOLATILE_CH )
#define MONITOR_HAS_CHILDREN( mp ) \
	( ( mp )->mp_children || MONITOR_HAS_VOLATILE_CH( mp ) )

	/* initialize entry and subentries */
	int		( *mss_open )( BackendDB *, struct monitor_subsys_t *ms );
	/* destroy structure */
	int		( *mss_destroy )( BackendDB *, struct monitor_subsys_t *ms );
	/* update existing dynamic entry and subentries */
	int		( *mss_update )( Operation *, SlapReply *, Entry * );
	/* create new dynamic subentries */
	int		( *mss_create )( Operation *, SlapReply *,
				struct berval *ndn, Entry *, Entry ** );
	/* modify entry and subentries */
	int		( *mss_modify )( Operation *, SlapReply *, Entry * );

	void		*mss_private;
} monitor_subsys_t;

extern BackendDB *be_monitor;

/* increase this bufsize if entries in string form get too big */
#define BACKMONITOR_BUFSIZE	8192

typedef int (monitor_cbfunc)( struct berval *ndn, monitor_callback_t *cb,
	struct berval *base, int scope, struct berval *filter );

typedef int (monitor_cbafunc)( struct berval *ndn, Attribute *a,
	monitor_callback_t *cb,
	struct berval *base, int scope, struct berval *filter );

typedef struct monitor_extra_t {
	int (*is_configured)(void);
	monitor_subsys_t * (*get_subsys)( const char *name );
	monitor_subsys_t * (*get_subsys_by_dn)( struct berval *ndn, int sub );

	int (*register_subsys)( monitor_subsys_t *ms );
	int (*register_backend)( BackendInfo *bi );
	int (*register_database)( BackendDB *be, struct berval *ndn_out );
	int (*register_overlay_info)( slap_overinst *on );
	int (*register_overlay)( BackendDB *be, slap_overinst *on, struct berval *ndn_out );
	int (*register_entry)( Entry *e, monitor_callback_t *cb,
		monitor_subsys_t *ms, unsigned long flags );
	int (*register_entry_parent)( Entry *e, monitor_callback_t *cb,
		monitor_subsys_t *ms, unsigned long flags,
		struct berval *base, int scope, struct berval *filter );
	monitor_cbafunc *register_entry_attrs;
	monitor_cbfunc *register_entry_callback;

	int (*unregister_entry)( struct berval *ndn );
	monitor_cbfunc *unregister_entry_parent;
	monitor_cbafunc *unregister_entry_attrs;
	monitor_cbfunc *unregister_entry_callback;
	Entry * (*entry_stub)( struct berval *pdn,
		struct berval *pndn,
		struct berval *rdn,
		ObjectClass *oc,
		struct berval *create,
		struct berval *modify );
	monitor_entry_t * (*entrypriv_create)( void );
	int (*register_subsys_late)( monitor_subsys_t *ms );
} monitor_extra_t;

LDAP_END_DECL

#include "proto-back-monitor.h"

#endif /* _back_monitor_h_ */

