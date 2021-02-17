/* rwm.h - dn rewrite/attribute mapping header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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

#ifndef RWM_H
#define RWM_H

#ifndef ENABLE_REWRITE
#error "librewrite must be enabled!"
#endif /* ENABLE_REWRITE */

/* String rewrite library */
#include "rewrite.h"

LDAP_BEGIN_DECL

/* define to enable referral DN massage by default */
#undef RWM_REFERRAL_REWRITE

struct ldapmap {
	int drop_missing;

	Avlnode *map;
	Avlnode *remap;
};

struct ldapmapping {
	int			m_flags;
#define	RWMMAP_F_NONE		0x00
#define	RWMMAP_F_IS_OC		0x01
#define RWMMAP_F_FREE_SRC	0x10
#define RWMMAP_F_FREE_DST	0x20
	struct berval		m_src;
	union {
		AttributeDescription	*m_s_ad;
		ObjectClass		*m_s_oc;
	} m_src_ref;
#define m_src_ad	m_src_ref.m_s_ad
#define m_src_oc	m_src_ref.m_s_oc
	struct berval		m_dst;
	union {
		AttributeDescription	*m_d_ad;
		ObjectClass		*m_d_oc;
	} m_dst_ref;
#define m_dst_ad	m_dst_ref.m_d_ad
#define m_dst_oc	m_dst_ref.m_d_oc
};

struct ldaprwmap {
	/*
	 * DN rewriting
	 */
	struct rewrite_info *rwm_rw;
	BerVarray rwm_bva_rewrite;

	/*
	 * Attribute/objectClass mapping
	 */
	struct ldapmap rwm_oc;
	struct ldapmap rwm_at;
	BerVarray rwm_bva_map;

#define	RWM_F_NONE			(0x0000U)
#define	RWM_F_NORMALIZE_MAPPED_ATTRS    (0x0001U)
#define	RWM_F_DROP_UNREQUESTED_ATTRS	(0x0002U)
#define	RWM_F_SUPPORT_T_F		(0x4000U)
#define	RWM_F_SUPPORT_T_F_DISCOVER	(0x8000U)
#define	RWM_F_SUPPORT_T_F_MASK		(RWM_F_SUPPORT_T_F)
#define	RWM_F_SUPPORT_T_F_MASK2		(RWM_F_SUPPORT_T_F|RWM_F_SUPPORT_T_F_DISCOVER)
	unsigned	rwm_flags;
};

/* Whatever context ldap_back_dn_massage needs... */
typedef struct dncookie {
	struct ldaprwmap *rwmap;

	Connection *conn;
	char *ctx;
	SlapReply *rs;
} dncookie;

int rwm_dn_massage( dncookie *dc, struct berval *in, struct berval *dn );
int rwm_dn_massage_pretty( dncookie *dc, struct berval *in, struct berval *pdn );
int rwm_dn_massage_normalize( dncookie *dc, struct berval *in, struct berval *ndn );
int rwm_dn_massage_pretty_normalize( dncookie *dc, struct berval *in, struct berval *pdn, struct berval *ndn );

/* attributeType/objectClass mapping */
int rwm_mapping_cmp (const void *, const void *);
int rwm_mapping_dup (void *, void *);

int rwm_map_init ( struct ldapmap *lm, struct ldapmapping ** );
void rwm_map ( struct ldapmap *map, struct berval *s, struct berval *m,
	int remap );
int rwm_mapping ( struct ldapmap *map, struct berval *s,
		struct ldapmapping **m, int remap );
#define RWM_MAP		0
#define RWM_REMAP	1
char *
rwm_map_filter(
		struct ldapmap *at_map,
		struct ldapmap *oc_map,
		struct berval *f );

#if 0 /* unused! */
int
rwm_map_attrs(
		struct ldapmap *at_map,
		AttributeName *a,
		int remap,
		char ***mapped_attrs );
#endif

int
rwm_map_attrnames(
		Operation *op,
		struct ldapmap *at_map,
		struct ldapmap *oc_map,
		AttributeName *an,
		AttributeName **anp,
		int remap );

extern void rwm_mapping_dst_free ( void *mapping );

extern void rwm_mapping_free ( void *mapping );

extern int rwm_map_config(
		struct ldapmap	*oc_map,
		struct ldapmap	*at_map,
		const char	*fname,
		int		lineno,
		int		argc,
		char		**argv );

extern int
rwm_filter_map_rewrite(
		Operation		*op,
		dncookie		*dc,
		Filter			*f,
		struct berval		*fstr );

/* suffix massaging by means of librewrite */
extern int
rwm_suffix_massage_config(
	struct rewrite_info	*info,
	struct berval		*pvnc,
	struct berval		*nvnc,
	struct berval		*prnc,
	struct berval		*nrnc);
extern int
rwm_dnattr_rewrite(
	Operation		*op,
	SlapReply		*rs,
	void			*cookie,
	BerVarray		a_vals,
	BerVarray		*pa_nvals );
extern int
rwm_referral_rewrite(
	Operation		*op,
	SlapReply		*rs,
	void			*cookie,
	BerVarray		a_vals,
	BerVarray		*pa_nvals );
extern int rwm_dnattr_result_rewrite( dncookie *dc, BerVarray a_vals, BerVarray a_nvals );
extern int rwm_referral_result_rewrite( dncookie *dc, BerVarray a_vals );

LDAP_END_DECL

#endif /* RWM_H */
