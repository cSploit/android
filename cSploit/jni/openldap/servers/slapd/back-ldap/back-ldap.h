/* back-ldap.h - ldap backend header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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

#ifndef SLAPD_LDAP_H
#define SLAPD_LDAP_H

#include "../back-monitor/back-monitor.h"

LDAP_BEGIN_DECL

struct ldapinfo_t;

/* stuff required for monitoring */
typedef struct ldap_monitor_info_t {
	monitor_subsys_t	lmi_mss[2];

	struct berval		lmi_ndn;
	struct berval		lmi_conn_rdn;
	struct berval		lmi_ops_rdn;
} ldap_monitor_info_t;

enum {
	/* even numbers are connection types */
	LDAP_BACK_PCONN_FIRST = 0,
	LDAP_BACK_PCONN_ROOTDN = LDAP_BACK_PCONN_FIRST,
	LDAP_BACK_PCONN_ANON = 2,
	LDAP_BACK_PCONN_BIND = 4,

	/* add the TLS bit */
	LDAP_BACK_PCONN_TLS = 0x1U,

	LDAP_BACK_PCONN_ROOTDN_TLS = (LDAP_BACK_PCONN_ROOTDN|LDAP_BACK_PCONN_TLS),
	LDAP_BACK_PCONN_ANON_TLS = (LDAP_BACK_PCONN_ANON|LDAP_BACK_PCONN_TLS),
	LDAP_BACK_PCONN_BIND_TLS = (LDAP_BACK_PCONN_BIND|LDAP_BACK_PCONN_TLS),

	LDAP_BACK_PCONN_LAST
};

typedef struct ldapconn_base_t {
	Connection		*lcb_conn;
#define	LDAP_BACK_CONN2PRIV(lc)		((unsigned long)(lc)->lc_conn)
#define LDAP_BACK_PCONN_ISPRIV(lc)	(((void *)(lc)->lc_conn) >= ((void *)LDAP_BACK_PCONN_FIRST) \
						&& ((void *)(lc)->lc_conn) < ((void *)LDAP_BACK_PCONN_LAST))
#define LDAP_BACK_PCONN_ISROOTDN(lc)	(LDAP_BACK_PCONN_ISPRIV((lc)) \
						&& (LDAP_BACK_CONN2PRIV((lc)) < LDAP_BACK_PCONN_ANON))
#define LDAP_BACK_PCONN_ISANON(lc)	(LDAP_BACK_PCONN_ISPRIV((lc)) \
						&& (LDAP_BACK_CONN2PRIV((lc)) < LDAP_BACK_PCONN_BIND) \
						&& (LDAP_BACK_CONN2PRIV((lc)) >= LDAP_BACK_PCONN_ANON))
#define LDAP_BACK_PCONN_ISBIND(lc)	(LDAP_BACK_PCONN_ISPRIV((lc)) \
						&& (LDAP_BACK_CONN2PRIV((lc)) >= LDAP_BACK_PCONN_BIND))
#define LDAP_BACK_PCONN_ISTLS(lc)	(LDAP_BACK_PCONN_ISPRIV((lc)) \
						&& (LDAP_BACK_CONN2PRIV((lc)) & LDAP_BACK_PCONN_TLS))
#ifdef HAVE_TLS
#define	LDAP_BACK_PCONN_ROOTDN_SET(lc, op) \
	((lc)->lc_conn = (void *)((op)->o_conn->c_is_tls ? (void *) LDAP_BACK_PCONN_ROOTDN_TLS : (void *) LDAP_BACK_PCONN_ROOTDN))
#define	LDAP_BACK_PCONN_ANON_SET(lc, op) \
	((lc)->lc_conn = (void *)((op)->o_conn->c_is_tls ? (void *) LDAP_BACK_PCONN_ANON_TLS : (void *) LDAP_BACK_PCONN_ANON))
#define	LDAP_BACK_PCONN_BIND_SET(lc, op) \
	((lc)->lc_conn = (void *)((op)->o_conn->c_is_tls ? (void *) LDAP_BACK_PCONN_BIND_TLS : (void *) LDAP_BACK_PCONN_BIND))
#else /* ! HAVE_TLS */
#define	LDAP_BACK_PCONN_ROOTDN_SET(lc, op) \
	((lc)->lc_conn = (void *)LDAP_BACK_PCONN_ROOTDN)
#define	LDAP_BACK_PCONN_ANON_SET(lc, op) \
	((lc)->lc_conn = (void *)LDAP_BACK_PCONN_ANON)
#define	LDAP_BACK_PCONN_BIND_SET(lc, op) \
	((lc)->lc_conn = (void *)LDAP_BACK_PCONN_BIND)
#endif /* ! HAVE_TLS */
#define	LDAP_BACK_PCONN_SET(lc, op) \
	(BER_BVISEMPTY(&(op)->o_ndn) ? \
		LDAP_BACK_PCONN_ANON_SET((lc), (op)) : LDAP_BACK_PCONN_ROOTDN_SET((lc), (op)))

	struct berval		lcb_local_ndn;
	unsigned		lcb_refcnt;
	time_t			lcb_create_time;
	time_t			lcb_time;
} ldapconn_base_t;

typedef struct ldapconn_t {
	ldapconn_base_t		lc_base;
#define	lc_conn			lc_base.lcb_conn
#define	lc_local_ndn		lc_base.lcb_local_ndn
#define	lc_refcnt		lc_base.lcb_refcnt
#define	lc_create_time		lc_base.lcb_create_time
#define	lc_time			lc_base.lcb_time

	LDAP_TAILQ_ENTRY(ldapconn_t)	lc_q;

	unsigned		lc_lcflags;
#define LDAP_BACK_CONN_ISSET_F(fp,f)	(*(fp) & (f))
#define	LDAP_BACK_CONN_SET_F(fp,f)	(*(fp) |= (f))
#define	LDAP_BACK_CONN_CLEAR_F(fp,f)	(*(fp) &= ~(f))
#define	LDAP_BACK_CONN_CPY_F(fp,f,mfp) \
	do { \
		if ( ((f) & *(mfp)) == (f) ) { \
			*(fp) |= (f); \
		} else { \
			*(fp) &= ~(f); \
		} \
	} while ( 0 )

#define LDAP_BACK_CONN_ISSET(lc,f)	LDAP_BACK_CONN_ISSET_F(&(lc)->lc_lcflags, (f))
#define	LDAP_BACK_CONN_SET(lc,f)	LDAP_BACK_CONN_SET_F(&(lc)->lc_lcflags, (f))
#define	LDAP_BACK_CONN_CLEAR(lc,f)	LDAP_BACK_CONN_CLEAR_F(&(lc)->lc_lcflags, (f))
#define	LDAP_BACK_CONN_CPY(lc,f,mlc)	LDAP_BACK_CONN_CPY_F(&(lc)->lc_lcflags, (f), &(mlc)->lc_lcflags)

/* 0xFFF00000U are reserved for back-meta */

#define	LDAP_BACK_FCONN_ISBOUND	(0x00000001U)
#define	LDAP_BACK_FCONN_ISANON	(0x00000002U)
#define	LDAP_BACK_FCONN_ISBMASK	(LDAP_BACK_FCONN_ISBOUND|LDAP_BACK_FCONN_ISANON)
#define	LDAP_BACK_FCONN_ISPRIV	(0x00000004U)
#define	LDAP_BACK_FCONN_ISTLS	(0x00000008U)
#define	LDAP_BACK_FCONN_BINDING	(0x00000010U)
#define	LDAP_BACK_FCONN_TAINTED	(0x00000020U)
#define	LDAP_BACK_FCONN_ABANDON	(0x00000040U)
#define	LDAP_BACK_FCONN_ISIDASR	(0x00000080U)
#define	LDAP_BACK_FCONN_CACHED	(0x00000100U)

#define	LDAP_BACK_CONN_ISBOUND(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ISBOUND)
#define	LDAP_BACK_CONN_ISBOUND_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ISBOUND)
#define	LDAP_BACK_CONN_ISBOUND_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_ISBMASK)
#define	LDAP_BACK_CONN_ISBOUND_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), LDAP_BACK_FCONN_ISBOUND, (mlc))
#define	LDAP_BACK_CONN_ISANON(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ISANON)
#define	LDAP_BACK_CONN_ISANON_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ISANON)
#define	LDAP_BACK_CONN_ISANON_CLEAR(lc)		LDAP_BACK_CONN_ISBOUND_CLEAR((lc))
#define	LDAP_BACK_CONN_ISANON_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), LDAP_BACK_FCONN_ISANON, (mlc))
#define	LDAP_BACK_CONN_ISPRIV(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ISPRIV)
#define	LDAP_BACK_CONN_ISPRIV_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ISPRIV)
#define	LDAP_BACK_CONN_ISPRIV_CLEAR(lc)		LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_ISPRIV)
#define	LDAP_BACK_CONN_ISPRIV_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), LDAP_BACK_FCONN_ISPRIV, (mlc))
#define	LDAP_BACK_CONN_ISTLS(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ISTLS)
#define	LDAP_BACK_CONN_ISTLS_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ISTLS)
#define	LDAP_BACK_CONN_ISTLS_CLEAR(lc)		LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_ISTLS)
#define	LDAP_BACK_CONN_ISTLS_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), LDAP_BACK_FCONN_ISTLS, (mlc))
#define	LDAP_BACK_CONN_BINDING(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_BINDING)
#define	LDAP_BACK_CONN_BINDING_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_BINDING)
#define	LDAP_BACK_CONN_BINDING_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_BINDING)
#define	LDAP_BACK_CONN_TAINTED(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_TAINTED)
#define	LDAP_BACK_CONN_TAINTED_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_TAINTED)
#define	LDAP_BACK_CONN_TAINTED_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_TAINTED)
#define	LDAP_BACK_CONN_ABANDON(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ABANDON)
#define	LDAP_BACK_CONN_ABANDON_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ABANDON)
#define	LDAP_BACK_CONN_ABANDON_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_ABANDON)
#define	LDAP_BACK_CONN_ISIDASSERT(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_ISIDASR)
#define	LDAP_BACK_CONN_ISIDASSERT_SET(lc)	LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_ISIDASR)
#define	LDAP_BACK_CONN_ISIDASSERT_CLEAR(lc)	LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_ISIDASR)
#define	LDAP_BACK_CONN_ISIDASSERT_CPY(lc, mlc)	LDAP_BACK_CONN_CPY((lc), LDAP_BACK_FCONN_ISIDASR, (mlc))
#define	LDAP_BACK_CONN_CACHED(lc)		LDAP_BACK_CONN_ISSET((lc), LDAP_BACK_FCONN_CACHED)
#define	LDAP_BACK_CONN_CACHED_SET(lc)		LDAP_BACK_CONN_SET((lc), LDAP_BACK_FCONN_CACHED)
#define	LDAP_BACK_CONN_CACHED_CLEAR(lc)		LDAP_BACK_CONN_CLEAR((lc), LDAP_BACK_FCONN_CACHED)

	LDAP			*lc_ld;
	unsigned long		lc_connid;
	struct berval		lc_cred;
	struct berval 		lc_bound_ndn;
	unsigned		lc_flags;
} ldapconn_t;

typedef struct ldap_avl_info_t {
	ldap_pvt_thread_mutex_t		lai_mutex;
	Avlnode				*lai_tree;
} ldap_avl_info_t;

typedef struct slap_retry_info_t {
	time_t		*ri_interval;
	int		*ri_num;
	int		ri_idx;
	int		ri_count;
	time_t		ri_last;

#define SLAP_RETRYNUM_FOREVER	(-1)		/* retry forever */
#define SLAP_RETRYNUM_TAIL	(-2)		/* end of retrynum array */
#define SLAP_RETRYNUM_VALID(n)	((n) >= SLAP_RETRYNUM_FOREVER)	/* valid retrynum */
#define SLAP_RETRYNUM_FINITE(n)	((n) > SLAP_RETRYNUM_FOREVER)	/* not forever */
} slap_retry_info_t;

/*
 * identity assertion modes
 */
typedef enum {
	LDAP_BACK_IDASSERT_LEGACY = 1,
	LDAP_BACK_IDASSERT_NOASSERT,
	LDAP_BACK_IDASSERT_ANONYMOUS,
	LDAP_BACK_IDASSERT_SELF,
	LDAP_BACK_IDASSERT_OTHERDN,
	LDAP_BACK_IDASSERT_OTHERID
} slap_idassert_mode_t;

/* ID assert stuff */
typedef struct slap_idassert_t {
	slap_idassert_mode_t	si_mode;
#define	li_idassert_mode	li_idassert.si_mode

	slap_bindconf	si_bc;
#define	li_idassert_authcID	li_idassert.si_bc.sb_authcId
#define	li_idassert_authcDN	li_idassert.si_bc.sb_binddn
#define	li_idassert_passwd	li_idassert.si_bc.sb_cred
#define	li_idassert_authzID	li_idassert.si_bc.sb_authzId
#define	li_idassert_authmethod	li_idassert.si_bc.sb_method
#define	li_idassert_sasl_mech	li_idassert.si_bc.sb_saslmech
#define	li_idassert_sasl_realm	li_idassert.si_bc.sb_realm
#define	li_idassert_secprops	li_idassert.si_bc.sb_secprops
#define	li_idassert_tls		li_idassert.si_bc.sb_tls

	unsigned 	si_flags;
#define LDAP_BACK_AUTH_NONE				(0x00U)
#define	LDAP_BACK_AUTH_NATIVE_AUTHZ			(0x01U)
#define	LDAP_BACK_AUTH_OVERRIDE				(0x02U)
#define	LDAP_BACK_AUTH_PRESCRIPTIVE			(0x04U)
#define	LDAP_BACK_AUTH_OBSOLETE_PROXY_AUTHZ		(0x08U)
#define	LDAP_BACK_AUTH_OBSOLETE_ENCODING_WORKAROUND	(0x10U)
#define	LDAP_BACK_AUTH_AUTHZ_ALL			(0x20U)
#define	LDAP_BACK_AUTH_PROXYAUTHZ_CRITICAL		(0x40U)
#define LDAP_BACK_AUTH_DN_AUTHZID			(0x100U)
#define LDAP_BACK_AUTH_DN_WHOAMI			(0x200U)
#define LDAP_BACK_AUTH_DN_MASK				(LDAP_BACK_AUTH_DN_AUTHZID|LDAP_BACK_AUTH_DN_WHOAMI)
#define	li_idassert_flags	li_idassert.si_flags

	BerVarray	si_authz;
#define	li_idassert_authz	li_idassert.si_authz

	BerVarray	si_passthru;
#define	li_idassert_passthru	li_idassert.si_passthru
} slap_idassert_t;

/*
 * Hook to allow mucking with ldapinfo_t when quarantine is over
 */
typedef int (*ldap_back_quarantine_f)( struct ldapinfo_t *, void * );

typedef struct ldapinfo_t {
	/* li_uri: the string that goes into ldap_initialize()
	 * TODO: use li_acl.sb_uri instead */
	char			*li_uri;
	/* li_bvuri: an array of each single URI that is equivalent;
	 * to be checked for the presence of a certain item */
	BerVarray		li_bvuri;
	ldap_pvt_thread_mutex_t	li_uri_mutex;
	/* hack because when TLS is used we need to lock and let 
	 * the li_urllist_f function to know it's locked */
	int			li_uri_mutex_do_not_lock;

	LDAP_REBIND_PROC	*li_rebind_f;
	LDAP_URLLIST_PROC	*li_urllist_f;
	void			*li_urllist_p;

	/* we only care about the TLS options here */
	slap_bindconf		li_tls;

	slap_bindconf		li_acl;
#define	li_acl_authcID		li_acl.sb_authcId
#define	li_acl_authcDN		li_acl.sb_binddn
#define	li_acl_passwd		li_acl.sb_cred
#define	li_acl_authzID		li_acl.sb_authzId
#define	li_acl_authmethod	li_acl.sb_method
#define	li_acl_sasl_mech	li_acl.sb_saslmech
#define	li_acl_sasl_realm	li_acl.sb_realm
#define	li_acl_secprops		li_acl.sb_secprops

	/* ID assert stuff */
	slap_idassert_t		li_idassert;
	/* end of ID assert stuff */

	int			li_nretries;
#define LDAP_BACK_RETRY_UNDEFINED	(-2)
#define LDAP_BACK_RETRY_FOREVER		(-1)
#define LDAP_BACK_RETRY_NEVER		(0)
#define LDAP_BACK_RETRY_DEFAULT		(3)

	unsigned		li_flags;

/* 0xFFF00000U are reserved for back-meta */

#define LDAP_BACK_F_NONE		(0x00000000U)
#define LDAP_BACK_F_SAVECRED		(0x00000001U)
#define LDAP_BACK_F_USE_TLS		(0x00000002U)
#define LDAP_BACK_F_PROPAGATE_TLS	(0x00000004U)
#define LDAP_BACK_F_TLS_CRITICAL	(0x00000008U)
#define LDAP_BACK_F_TLS_LDAPS		(0x00000010U)

#define LDAP_BACK_F_TLS_USE_MASK	(LDAP_BACK_F_USE_TLS|LDAP_BACK_F_TLS_CRITICAL)
#define LDAP_BACK_F_TLS_PROPAGATE_MASK	(LDAP_BACK_F_PROPAGATE_TLS|LDAP_BACK_F_TLS_CRITICAL)
#define LDAP_BACK_F_TLS_MASK		(LDAP_BACK_F_TLS_USE_MASK|LDAP_BACK_F_TLS_PROPAGATE_MASK|LDAP_BACK_F_TLS_LDAPS)
#define LDAP_BACK_F_CHASE_REFERRALS	(0x00000020U)
#define LDAP_BACK_F_PROXY_WHOAMI	(0x00000040U)

#define	LDAP_BACK_F_T_F			(0x00000080U)
#define	LDAP_BACK_F_T_F_DISCOVER	(0x00000100U)
#define	LDAP_BACK_F_T_F_MASK		(LDAP_BACK_F_T_F)
#define	LDAP_BACK_F_T_F_MASK2		(LDAP_BACK_F_T_F_MASK|LDAP_BACK_F_T_F_DISCOVER)

#define LDAP_BACK_F_MONITOR		(0x00000200U)
#define	LDAP_BACK_F_SINGLECONN		(0x00000400U)
#define LDAP_BACK_F_USE_TEMPORARIES	(0x00000800U)

#define	LDAP_BACK_F_ISOPEN		(0x00001000U)

#define	LDAP_BACK_F_CANCEL_ABANDON	(0x00000000U)
#define	LDAP_BACK_F_CANCEL_IGNORE	(0x00002000U)
#define	LDAP_BACK_F_CANCEL_EXOP		(0x00004000U)
#define	LDAP_BACK_F_CANCEL_EXOP_DISCOVER	(0x00008000U)
#define	LDAP_BACK_F_CANCEL_MASK		(LDAP_BACK_F_CANCEL_IGNORE|LDAP_BACK_F_CANCEL_EXOP)
#define	LDAP_BACK_F_CANCEL_MASK2	(LDAP_BACK_F_CANCEL_MASK|LDAP_BACK_F_CANCEL_EXOP_DISCOVER)

#define	LDAP_BACK_F_QUARANTINE		(0x00010000U)

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
#define	LDAP_BACK_F_ST_REQUEST		(0x00020000U)
#define	LDAP_BACK_F_ST_RESPONSE		(0x00040000U)
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

#define LDAP_BACK_F_NOREFS		(0x00080000U)
#define LDAP_BACK_F_NOUNDEFFILTER	(0x00100000U)

#define LDAP_BACK_F_ONERR_STOP		(0x00200000U)

#define	LDAP_BACK_ISSET_F(ff,f)		( ( (ff) & (f) ) == (f) )
#define	LDAP_BACK_ISMASK_F(ff,m,f)	( ( (ff) & (m) ) == (f) )

#define	LDAP_BACK_ISSET(li,f)		LDAP_BACK_ISSET_F( (li)->li_flags, (f) )
#define	LDAP_BACK_ISMASK(li,m,f)	LDAP_BACK_ISMASK_F( (li)->li_flags, (m), (f) )

#define LDAP_BACK_SAVECRED(li)		LDAP_BACK_ISSET( (li), LDAP_BACK_F_SAVECRED )
#define LDAP_BACK_USE_TLS(li)		LDAP_BACK_ISSET( (li), LDAP_BACK_F_USE_TLS )
#define LDAP_BACK_PROPAGATE_TLS(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_PROPAGATE_TLS )
#define LDAP_BACK_TLS_CRITICAL(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_TLS_CRITICAL )
#define LDAP_BACK_CHASE_REFERRALS(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_CHASE_REFERRALS )
#define LDAP_BACK_PROXY_WHOAMI(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_PROXY_WHOAMI )

#define LDAP_BACK_USE_TLS_F(ff)		LDAP_BACK_ISSET_F( (ff), LDAP_BACK_F_USE_TLS )
#define LDAP_BACK_PROPAGATE_TLS_F(ff)	LDAP_BACK_ISSET_F( (ff), LDAP_BACK_F_PROPAGATE_TLS )
#define LDAP_BACK_TLS_CRITICAL_F(ff)	LDAP_BACK_ISSET_F( (ff), LDAP_BACK_F_TLS_CRITICAL )

#define	LDAP_BACK_T_F(li)		LDAP_BACK_ISMASK( (li), LDAP_BACK_F_T_F_MASK, LDAP_BACK_F_T_F )
#define	LDAP_BACK_T_F_DISCOVER(li)	LDAP_BACK_ISMASK( (li), LDAP_BACK_F_T_F_MASK2, LDAP_BACK_F_T_F_DISCOVER )

#define LDAP_BACK_MONITOR(li)		LDAP_BACK_ISSET( (li), LDAP_BACK_F_MONITOR )
#define	LDAP_BACK_SINGLECONN(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_SINGLECONN )
#define	LDAP_BACK_USE_TEMPORARIES(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_USE_TEMPORARIES)

#define	LDAP_BACK_ISOPEN(li)		LDAP_BACK_ISSET( (li), LDAP_BACK_F_ISOPEN )

#define	LDAP_BACK_ABANDON(li)		LDAP_BACK_ISMASK( (li), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_ABANDON )
#define	LDAP_BACK_IGNORE(li)		LDAP_BACK_ISMASK( (li), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_IGNORE )
#define	LDAP_BACK_CANCEL(li)		LDAP_BACK_ISMASK( (li), LDAP_BACK_F_CANCEL_MASK, LDAP_BACK_F_CANCEL_EXOP )
#define	LDAP_BACK_CANCEL_DISCOVER(li)	LDAP_BACK_ISMASK( (li), LDAP_BACK_F_CANCEL_MASK2, LDAP_BACK_F_CANCEL_EXOP_DISCOVER )

#define	LDAP_BACK_QUARANTINE(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_QUARANTINE )

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
#define	LDAP_BACK_ST_REQUEST(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_ST_REQUEST)
#define	LDAP_BACK_ST_RESPONSE(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_ST_RESPONSE)
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */

#define	LDAP_BACK_NOREFS(li)		LDAP_BACK_ISSET( (li), LDAP_BACK_F_NOREFS)
#define	LDAP_BACK_NOUNDEFFILTER(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_NOUNDEFFILTER)

#define	LDAP_BACK_ONERR_STOP(li)	LDAP_BACK_ISSET( (li), LDAP_BACK_F_ONERR_STOP)

	int			li_version;

	unsigned long		li_conn_nextid;

	/* cached connections; 
	 * special conns are in tailq rather than in tree */
	ldap_avl_info_t		li_conninfo;
	struct {
		int						lic_num;
		LDAP_TAILQ_HEAD(lc_conn_priv_q, ldapconn_t)	lic_priv;
	}			li_conn_priv[ LDAP_BACK_PCONN_LAST ];
	int			li_conn_priv_max;
#define	LDAP_BACK_CONN_PRIV_MIN		(1)
#define	LDAP_BACK_CONN_PRIV_MAX		(256)
	/* must be between LDAP_BACK_CONN_PRIV_MIN
	 * and LDAP_BACK_CONN_PRIV_MAX ! */
#define	LDAP_BACK_CONN_PRIV_DEFAULT	(16)

	ldap_monitor_info_t	li_monitor_info;

	sig_atomic_t		li_isquarantined;
#define	LDAP_BACK_FQ_NO		(0)
#define	LDAP_BACK_FQ_YES	(1)
#define	LDAP_BACK_FQ_RETRYING	(2)

	slap_retry_info_t	li_quarantine;
	ldap_pvt_thread_mutex_t	li_quarantine_mutex;
	ldap_back_quarantine_f	li_quarantine_f;
	void			*li_quarantine_p;

	time_t			li_network_timeout;
	time_t			li_conn_ttl;
	time_t			li_idle_timeout;
	time_t			li_timeout[ SLAP_OP_LAST ];

	ldap_pvt_thread_mutex_t li_counter_mutex;
	ldap_pvt_mp_t		li_ops_completed[SLAP_OP_LAST];
} ldapinfo_t;

#define	LDAP_ERR_OK(err) ((err) == LDAP_SUCCESS || (err) == LDAP_COMPARE_FALSE || (err) == LDAP_COMPARE_TRUE)

typedef enum ldap_back_send_t {
	LDAP_BACK_DONTSEND		= 0x00,
	LDAP_BACK_SENDOK		= 0x01,
	LDAP_BACK_SENDERR		= 0x02,
	LDAP_BACK_SENDRESULT		= (LDAP_BACK_SENDOK|LDAP_BACK_SENDERR),
	LDAP_BACK_BINDING		= 0x04,

	LDAP_BACK_BIND_DONTSEND		= (LDAP_BACK_BINDING),
	LDAP_BACK_BIND_SOK		= (LDAP_BACK_BINDING|LDAP_BACK_SENDOK),
	LDAP_BACK_BIND_SERR		= (LDAP_BACK_BINDING|LDAP_BACK_SENDERR),
	LDAP_BACK_BIND_SRES		= (LDAP_BACK_BINDING|LDAP_BACK_SENDRESULT),

	LDAP_BACK_RETRYING		= 0x08,
	LDAP_BACK_RETRY_DONTSEND	= (LDAP_BACK_RETRYING),
	LDAP_BACK_RETRY_SOK		= (LDAP_BACK_RETRYING|LDAP_BACK_SENDOK),
	LDAP_BACK_RETRY_SERR		= (LDAP_BACK_RETRYING|LDAP_BACK_SENDERR),
	LDAP_BACK_RETRY_SRES		= (LDAP_BACK_RETRYING|LDAP_BACK_SENDRESULT),

	LDAP_BACK_GETCONN		= 0x10
} ldap_back_send_t;

/* define to use asynchronous StartTLS */
#define SLAP_STARTTLS_ASYNCHRONOUS

/* timeout to use when calling ldap_result() */
#define	LDAP_BACK_RESULT_TIMEOUT	(0)
#define	LDAP_BACK_RESULT_UTIMEOUT	(100000)
#define	LDAP_BACK_TV_SET(tv) \
	do { \
		(tv)->tv_sec = LDAP_BACK_RESULT_TIMEOUT; \
		(tv)->tv_usec = LDAP_BACK_RESULT_UTIMEOUT; \
	} while ( 0 )

#ifndef LDAP_BACK_PRINT_CONNTREE
#define LDAP_BACK_PRINT_CONNTREE 0
#endif /* !LDAP_BACK_PRINT_CONNTREE */

typedef struct ldap_extra_t {
	int (*proxy_authz_ctrl)( Operation *op, SlapReply *rs, struct berval *bound_ndn,
		int version, slap_idassert_t *si, LDAPControl	*ctrl );
	int (*controls_free)( Operation *op, SlapReply *rs, LDAPControl ***pctrls );
	int (*idassert_authzfrom_parse)( struct config_args_s *ca, slap_idassert_t *si );
	int (*idassert_passthru_parse_cf)( const char *fname, int lineno, const char *arg, slap_idassert_t *si );
	int (*idassert_parse)( struct config_args_s *ca, slap_idassert_t *si );
	void (*retry_info_destroy)( slap_retry_info_t *ri );
	int (*retry_info_parse)( char *in, slap_retry_info_t *ri, char *buf, ber_len_t buflen );
	int (*retry_info_unparse)( slap_retry_info_t *ri, struct berval *bvout );
	int (*connid2str)( const ldapconn_base_t *lc, char *buf, ber_len_t buflen );
} ldap_extra_t;

LDAP_END_DECL

#include "proto-ldap.h"

#endif /* SLAPD_LDAP_H */
