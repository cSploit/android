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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef PROTO_SLAP_H
#define PROTO_SLAP_H

#include <ldap_cdefs.h>
#include "ldap_pvt.h"

LDAP_BEGIN_DECL

struct config_args_s;	/* config.h */
struct config_reply_s;	/* config.h */

/*
 * aci.c
 */
#ifdef SLAP_DYNACL
#ifdef SLAPD_ACI_ENABLED
LDAP_SLAPD_F (int) dynacl_aci_init LDAP_P(( void ));
#endif /* SLAPD_ACI_ENABLED */
#endif /* SLAP_DYNACL */

/*
 * acl.c
 */
LDAP_SLAPD_F (int) access_allowed_mask LDAP_P((
	Operation *op,
	Entry *e, AttributeDescription *desc, struct berval *val,
	slap_access_t access,
	AccessControlState *state,
	slap_mask_t *mask ));
#define access_allowed(op,e,desc,val,access,state) access_allowed_mask(op,e,desc,val,access,state,NULL)
LDAP_SLAPD_F (int) slap_access_allowed LDAP_P((
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp ));
LDAP_SLAPD_F (int) slap_access_always_allowed LDAP_P((
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp ));

LDAP_SLAPD_F (int) acl_check_modlist LDAP_P((
	Operation *op, Entry *e, Modifications *ml ));

LDAP_SLAPD_F (void) acl_append( AccessControl **l, AccessControl *a, int pos );

#ifdef SLAP_DYNACL
LDAP_SLAPD_F (int) slap_dynacl_register LDAP_P(( slap_dynacl_t *da ));
LDAP_SLAPD_F (slap_dynacl_t *) slap_dynacl_get LDAP_P(( const char *name ));
#endif /* SLAP_DYNACL */
LDAP_SLAPD_F (int) acl_init LDAP_P(( void ));

LDAP_SLAPD_F (int) acl_get_part LDAP_P((
	struct berval	*list,
	int		ix,
	char		sep,
	struct berval	*bv ));
LDAP_SLAPD_F (int) acl_match_set LDAP_P((
	struct berval *subj,
	Operation *op,
	Entry *e,
	struct berval *default_set_attribute ));
LDAP_SLAPD_F (int) acl_string_expand LDAP_P((
	struct berval *newbuf, struct berval *pattern,
	struct berval *dnmatch, struct berval *valmatch, AclRegexMatches *matches ));

/*
 * aclparse.c
 */
LDAP_SLAPD_V (LDAP_CONST char *) style_strings[];

LDAP_SLAPD_F (int) parse_acl LDAP_P(( Backend *be,
	const char *fname, int lineno,
	int argc, char **argv, int pos ));

LDAP_SLAPD_F (char *) access2str LDAP_P(( slap_access_t access ));
LDAP_SLAPD_F (slap_access_t) str2access LDAP_P(( const char *str ));

#define ACCESSMASK_MAXLEN	sizeof("unknown (+wrscan)")
LDAP_SLAPD_F (char *) accessmask2str LDAP_P(( slap_mask_t mask, char*, int debug ));
LDAP_SLAPD_F (slap_mask_t) str2accessmask LDAP_P(( const char *str ));
LDAP_SLAPD_F (void) acl_unparse LDAP_P(( AccessControl*, struct berval* ));
LDAP_SLAPD_F (void) acl_destroy LDAP_P(( AccessControl* ));
LDAP_SLAPD_F (void) acl_free LDAP_P(( AccessControl *a ));


/*
 * ad.c
 */
LDAP_SLAPD_F (int) slap_str2ad LDAP_P((
	const char *,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (int) slap_bv2ad LDAP_P((
	struct berval *bv,
	AttributeDescription **ad,
	const char **text ));

LDAP_SLAPD_F (void) ad_destroy LDAP_P(( AttributeDescription * ));
LDAP_SLAPD_F (int) ad_keystring LDAP_P(( struct berval *bv ));

#define ad_cmp(l,r)	(((l)->ad_cname.bv_len < (r)->ad_cname.bv_len) \
	? -1 : (((l)->ad_cname.bv_len > (r)->ad_cname.bv_len) \
		? 1 : strcasecmp((l)->ad_cname.bv_val, (r)->ad_cname.bv_val )))

LDAP_SLAPD_F (int) is_ad_subtype LDAP_P((
	AttributeDescription *sub,
	AttributeDescription *super ));

LDAP_SLAPD_F (int) ad_inlist LDAP_P((
	AttributeDescription *desc,
	AttributeName *attrs ));

LDAP_SLAPD_F (int) slap_str2undef_ad LDAP_P((
	const char *,
	AttributeDescription **ad,
	const char **text,
	unsigned proxied ));

LDAP_SLAPD_F (int) slap_bv2undef_ad LDAP_P((
	struct berval *bv,
	AttributeDescription **ad,
	const char **text,
	unsigned proxied ));

LDAP_SLAPD_F (AttributeDescription *) slap_bv2tmp_ad LDAP_P((
	struct berval *bv,
	void *memctx ));

LDAP_SLAPD_F (int) slap_ad_undef_promote LDAP_P((
	char *name,
	AttributeType *nat ));

LDAP_SLAPD_F (AttributeDescription *) ad_find_tags LDAP_P((
	AttributeType *type,
	struct berval *tags ));

LDAP_SLAPD_F (AttributeName *) str2anlist LDAP_P(( AttributeName *an,
	char *str, const char *brkstr ));
LDAP_SLAPD_F (void) anlist_free LDAP_P(( AttributeName *an,
	int freename, void *ctx ));

LDAP_SLAPD_F (char **) anlist2charray_x LDAP_P((
									AttributeName *an, int dup, void *ctx ));
LDAP_SLAPD_F (char **) anlist2charray LDAP_P(( AttributeName *an, int dup ));
LDAP_SLAPD_F (char **) anlist2attrs LDAP_P(( AttributeName *anlist ));
LDAP_SLAPD_F (AttributeName *) file2anlist LDAP_P((
                        AttributeName *, const char *, const char * ));
LDAP_SLAPD_F (int) an_find LDAP_P(( AttributeName *a, struct berval *s ));
LDAP_SLAPD_F (int) ad_define_option LDAP_P(( const char *name,
	const char *fname, int lineno ));
LDAP_SLAPD_F (void) ad_unparse_options LDAP_P(( BerVarray *res ));

LDAP_SLAPD_F (MatchingRule *) ad_mr(
	AttributeDescription *ad,
	unsigned usage );

LDAP_SLAPD_V( AttributeName * ) slap_anlist_no_attrs;
LDAP_SLAPD_V( AttributeName * ) slap_anlist_all_user_attributes;
LDAP_SLAPD_V( AttributeName * ) slap_anlist_all_operational_attributes;
LDAP_SLAPD_V( AttributeName * ) slap_anlist_all_attributes;

LDAP_SLAPD_V( struct berval * ) slap_bv_no_attrs;
LDAP_SLAPD_V( struct berval * ) slap_bv_all_user_attrs;
LDAP_SLAPD_V( struct berval * ) slap_bv_all_operational_attrs;

/* deprecated; only defined for backward compatibility */
#define NoAttrs		(*slap_bv_no_attrs)
#define AllUser		(*slap_bv_all_user_attrs)
#define AllOper		(*slap_bv_all_operational_attrs)

/*
 * add.c
 */
LDAP_SLAPD_F (int) slap_mods2entry LDAP_P(( Modifications *mods, Entry **e,
	int initial, int dup, const char **text, char *textbuf, size_t textlen ));

LDAP_SLAPD_F (int) slap_entry2mods LDAP_P(( Entry *e,
						Modifications **mods, const char **text,
						char *textbuf, size_t textlen ));
LDAP_SLAPD_F( int ) slap_add_opattrs(
	Operation *op,
	const char **text,
	char *textbuf, size_t textlen,
	int manage_ctxcsn );


/*
 * at.c
 */
LDAP_SLAPD_V(int) at_oc_cache;
LDAP_SLAPD_F (void) at_config LDAP_P((
	const char *fname, int lineno,
	int argc, char **argv ));
LDAP_SLAPD_F (AttributeType *) at_find LDAP_P((
	const char *name ));
LDAP_SLAPD_F (AttributeType *) at_bvfind LDAP_P((
	struct berval *name ));
LDAP_SLAPD_F (int) at_find_in_list LDAP_P((
	AttributeType *sat, AttributeType **list ));
LDAP_SLAPD_F (int) at_append_to_list LDAP_P((
	AttributeType *sat, AttributeType ***listp ));
LDAP_SLAPD_F (int) at_delete_from_list LDAP_P((
	int pos, AttributeType ***listp ));
LDAP_SLAPD_F (int) at_schema_info LDAP_P(( Entry *e ));
LDAP_SLAPD_F (int) at_add LDAP_P((
	LDAPAttributeType *at, int user,
	AttributeType **sat, AttributeType *prev, const char **err ));
LDAP_SLAPD_F (void) at_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) is_at_subtype LDAP_P((
	AttributeType *sub,
	AttributeType *super ));

LDAP_SLAPD_F (const char *) at_syntax LDAP_P((
	AttributeType *at ));
LDAP_SLAPD_F (int) is_at_syntax LDAP_P((
	AttributeType *at,
	const char *oid ));

LDAP_SLAPD_F (int) at_start LDAP_P(( AttributeType **at ));
LDAP_SLAPD_F (int) at_next LDAP_P(( AttributeType **at ));
LDAP_SLAPD_F (void) at_delete LDAP_P(( AttributeType *at ));

LDAP_SLAPD_F (void) at_unparse LDAP_P((
	BerVarray *bva, AttributeType *start, AttributeType *end, int system ));

LDAP_SLAPD_F (int) register_at LDAP_P((
	const char *at,
	AttributeDescription **ad,
	int dupok ));

/*
 * attr.c
 */
LDAP_SLAPD_F (void) attr_free LDAP_P(( Attribute *a ));
LDAP_SLAPD_F (Attribute *) attr_dup LDAP_P(( Attribute *a ));

#ifdef LDAP_COMP_MATCH
LDAP_SLAPD_F (void) comp_tree_free LDAP_P(( Attribute *a ));
#endif

#define attr_mergeit( e, d, v ) attr_merge( e, d, v, NULL /* FIXME */ )
#define attr_mergeit_one( e, d, v ) attr_merge_one( e, d, v, NULL /* FIXME */ )

LDAP_SLAPD_F (Attribute *) attr_alloc LDAP_P(( AttributeDescription *ad ));
LDAP_SLAPD_F (Attribute *) attrs_alloc LDAP_P(( int num ));
LDAP_SLAPD_F (int) attr_prealloc LDAP_P(( int num ));
LDAP_SLAPD_F (int) attr_valfind LDAP_P(( Attribute *a,
	unsigned flags,
	struct berval *val,
	unsigned *slot,
	void *ctx ));
LDAP_SLAPD_F (int) attr_valadd LDAP_P(( Attribute *a,
	BerVarray vals,
	BerVarray nvals,
	int num ));
LDAP_SLAPD_F (int) attr_merge LDAP_P(( Entry *e,
	AttributeDescription *desc,
	BerVarray vals,
	BerVarray nvals ));
LDAP_SLAPD_F (int) attr_merge_one LDAP_P(( Entry *e,
	AttributeDescription *desc,
	struct berval *val,
	struct berval *nval ));
LDAP_SLAPD_F (int) attr_normalize LDAP_P(( AttributeDescription *desc,
	BerVarray vals, BerVarray *nvalsp, void *memctx ));
LDAP_SLAPD_F (int) attr_normalize_one LDAP_P(( AttributeDescription *desc,
	struct berval *val, struct berval *nval, void *memctx ));
LDAP_SLAPD_F (int) attr_merge_normalize LDAP_P(( Entry *e,
	AttributeDescription *desc,
	BerVarray vals, void *memctx ));
LDAP_SLAPD_F (int) attr_merge_normalize_one LDAP_P(( Entry *e,
	AttributeDescription *desc,
	struct berval *val, void *memctx ));
LDAP_SLAPD_F (Attribute *) attrs_find LDAP_P((
	Attribute *a, AttributeDescription *desc ));
LDAP_SLAPD_F (Attribute *) attr_find LDAP_P((
	Attribute *a, AttributeDescription *desc ));
LDAP_SLAPD_F (int) attr_delete LDAP_P((
	Attribute **attrs, AttributeDescription *desc ));

LDAP_SLAPD_F (void) attrs_free LDAP_P(( Attribute *a ));
LDAP_SLAPD_F (Attribute *) attrs_dup LDAP_P(( Attribute *a ));
LDAP_SLAPD_F (int) attr_init LDAP_P(( void ));
LDAP_SLAPD_F (int) attr_destroy LDAP_P(( void ));


/*
 * ava.c
 */
LDAP_SLAPD_F (int) get_ava LDAP_P((
	Operation *op,
	BerElement *ber,
	Filter *f,
	unsigned usage,
	const char **text ));
LDAP_SLAPD_F (void) ava_free LDAP_P((
	Operation *op,
	AttributeAssertion *ava,
	int freeit ));

/*
 * backend.c
 */

#define be_match( be1, be2 )	( (be1) == (be2) || \
				  ( (be1) && (be2) && (be1)->be_nsuffix == (be2)->be_nsuffix ) )

LDAP_SLAPD_F (int) backend_init LDAP_P((void));
LDAP_SLAPD_F (int) backend_add LDAP_P((BackendInfo *aBackendInfo));
LDAP_SLAPD_F (int) backend_num LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_startup LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_startup_one LDAP_P((Backend *be, struct config_reply_s *cr));
LDAP_SLAPD_F (int) backend_sync LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_shutdown LDAP_P((Backend *be));
LDAP_SLAPD_F (int) backend_destroy LDAP_P((void));
LDAP_SLAPD_F (void) backend_stopdown_one LDAP_P((BackendDB *bd ));
LDAP_SLAPD_F (void) backend_destroy_one LDAP_P((BackendDB *bd, int dynamic));

LDAP_SLAPD_F (BackendInfo *) backend_info LDAP_P(( const char *type ));
LDAP_SLAPD_F (BackendDB *) backend_db_init LDAP_P(( const char *type,
	BackendDB *be, int idx, struct config_reply_s *cr ));
LDAP_SLAPD_F (void) backend_db_insert LDAP_P((BackendDB *bd, int idx));
LDAP_SLAPD_F (void) backend_db_move LDAP_P((BackendDB *bd, int idx));

LDAP_SLAPD_F (BackendDB *) select_backend LDAP_P((
	struct berval * dn,
	int noSubordinates ));

LDAP_SLAPD_F (int) be_issuffix LDAP_P(( Backend *be,
	struct berval *suffix ));
LDAP_SLAPD_F (int) be_issubordinate LDAP_P(( Backend *be,
	struct berval *subordinate ));
LDAP_SLAPD_F (int) be_isroot LDAP_P(( Operation *op ));
LDAP_SLAPD_F (int) be_isroot_dn LDAP_P(( Backend *be, struct berval *ndn ));
LDAP_SLAPD_F (int) be_isroot_pw LDAP_P(( Operation *op ));
LDAP_SLAPD_F (int) be_rootdn_bind LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) be_slurp_update LDAP_P(( Operation *op ));
#define be_isupdate( op ) be_slurp_update( (op) )
LDAP_SLAPD_F (int) be_shadow_update LDAP_P(( Operation *op ));
LDAP_SLAPD_F (int) be_isupdate_dn LDAP_P(( Backend *be, struct berval *ndn ));
LDAP_SLAPD_F (struct berval *) be_root_dn LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int) be_entry_get_rw LDAP_P(( Operation *o,
		struct berval *ndn, ObjectClass *oc,
		AttributeDescription *at, int rw, Entry **e ));

/* "backend->ophandler(op,rs)" wrappers, applied by contrib:wrap_slap_ops */
#define SLAP_OP(which, op, rs)  slap_bi_op((op)->o_bd->bd_info, which, op, rs)
#define slap_be_op(be, which, op, rs) slap_bi_op((be)->bd_info, which, op, rs)
#if !(defined(USE_RS_ASSERT) && (USE_RS_ASSERT))
#define slap_bi_op(bi, which, op, rs) ((&(bi)->bi_op_bind)[which](op, rs))
#endif
LDAP_SLAPD_F (int) (slap_bi_op) LDAP_P(( BackendInfo *bi,
	slap_operation_t which, Operation *op, SlapReply *rs ));

LDAP_SLAPD_F (int) be_entry_release_rw LDAP_P((
	Operation *o, Entry *e, int rw ));
#define be_entry_release_r( o, e ) be_entry_release_rw( o, e, 0 )
#define be_entry_release_w( o, e ) be_entry_release_rw( o, e, 1 )

LDAP_SLAPD_F (int) backend_unbind LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) backend_connection_init LDAP_P((Connection *conn));
LDAP_SLAPD_F (int) backend_connection_destroy LDAP_P((Connection *conn));

LDAP_SLAPD_F( int ) backend_check_controls LDAP_P((
	Operation *op,
	SlapReply *rs ));
LDAP_SLAPD_F( int )	backend_check_restrictions LDAP_P((
	Operation *op,
	SlapReply *rs,
	struct berval *opdata ));

LDAP_SLAPD_F( int )	backend_check_referrals LDAP_P((
	Operation *op,
	SlapReply *rs ));

LDAP_SLAPD_F (int) backend_group LDAP_P((
	Operation *op,
	Entry *target,
	struct berval *gr_ndn,
	struct berval *op_ndn,
	ObjectClass *group_oc,
	AttributeDescription *group_at
));

LDAP_SLAPD_F (int) backend_attribute LDAP_P((
	Operation *op,
	Entry *target,
	struct berval *entry_ndn,
	AttributeDescription *entry_at,
	BerVarray *vals,
	slap_access_t access
));

LDAP_SLAPD_F (int) backend_access LDAP_P((
	Operation		*op,
	Entry			*target,
	struct berval		*edn,
	AttributeDescription	*entry_at,
	struct berval		*nval,
	slap_access_t		access,
	slap_mask_t		*mask ));

LDAP_SLAPD_F (int) backend_operational LDAP_P((
	Operation *op,
	SlapReply *rs 
));

LDAP_SLAPD_F (ID) backend_tool_entry_first LDAP_P(( BackendDB *be ));

LDAP_SLAPD_V(BackendInfo) slap_binfo[]; 

/*
 * backglue.c
 */

LDAP_SLAPD_F (int) glue_sub_init( void );
LDAP_SLAPD_F (int) glue_sub_attach( int online );
LDAP_SLAPD_F (int) glue_sub_add( BackendDB *be, int advert, int online );
LDAP_SLAPD_F (int) glue_sub_del( BackendDB *be );

/*
 * backover.c
 */
LDAP_SLAPD_F (int) overlay_register LDAP_P(( slap_overinst *on ));
LDAP_SLAPD_F (int) overlay_config LDAP_P(( BackendDB *be, const char *ov,
	int idx, BackendInfo **res, ConfigReply *cr ));
LDAP_SLAPD_F (void) overlay_destroy_one LDAP_P((
	BackendDB *be,
	slap_overinst *on ));
LDAP_SLAPD_F (slap_overinst *) overlay_next LDAP_P(( slap_overinst *on ));
LDAP_SLAPD_F (slap_overinst *) overlay_find LDAP_P(( const char *name ));
LDAP_SLAPD_F (int) overlay_is_over LDAP_P(( BackendDB *be ));
LDAP_SLAPD_F (int) overlay_is_inst LDAP_P(( BackendDB *be, const char *name ));
LDAP_SLAPD_F (int) overlay_register_control LDAP_P((
	BackendDB *be,
	const char *oid ));
LDAP_SLAPD_F (int) overlay_op_walk LDAP_P((
	Operation *op,
	SlapReply *rs,
	slap_operation_t which,
	slap_overinfo *oi,
	slap_overinst *on ));
LDAP_SLAPD_F (int) overlay_entry_get_ov LDAP_P((
	Operation *op,
	struct berval *dn,
	ObjectClass *oc,
	AttributeDescription *ad,
	int rw,
	Entry **e,
	slap_overinst *ov ));
LDAP_SLAPD_F (int) overlay_entry_release_ov LDAP_P((
	Operation *op,
	Entry *e,
	int rw,
	slap_overinst *ov ));
LDAP_SLAPD_F (void) overlay_insert LDAP_P((
	BackendDB *be, slap_overinst *on, slap_overinst ***prev, int idx ));
LDAP_SLAPD_F (void) overlay_move LDAP_P((
	BackendDB *be, slap_overinst *on, int idx ));
#ifdef SLAP_CONFIG_DELETE
LDAP_SLAPD_F (void) overlay_remove LDAP_P((
	BackendDB *be, slap_overinst *on, Operation *op ));
LDAP_SLAPD_F (void) overlay_unregister_control LDAP_P((
	BackendDB *be,
	const char *oid ));
#endif /* SLAP_CONFIG_DELETE */
LDAP_SLAPD_F (int) overlay_callback_after_backover LDAP_P((
	Operation *op, slap_callback *sc, int append ));

/*
 * bconfig.c
 */
LDAP_SLAPD_F (int) slap_loglevel_register LDAP_P (( slap_mask_t m, struct berval *s ));
LDAP_SLAPD_F (int) slap_loglevel_get LDAP_P(( struct berval *s, int *l ));
LDAP_SLAPD_F (int) str2loglevel LDAP_P(( const char *s, int *l ));
LDAP_SLAPD_F (int) loglevel2bvarray LDAP_P(( int l, BerVarray *bva ));
LDAP_SLAPD_F (const char *) loglevel2str LDAP_P(( int l ));
LDAP_SLAPD_F (int) loglevel2bv LDAP_P(( int l, struct berval *bv ));
LDAP_SLAPD_F (int) loglevel_print LDAP_P(( FILE *out ));
LDAP_SLAPD_F (int) slap_cf_aux_table_parse LDAP_P(( const char *word, void *bc, slap_cf_aux_table *tab0, LDAP_CONST char *tabmsg ));
LDAP_SLAPD_F (int) slap_cf_aux_table_unparse LDAP_P(( void *bc, struct berval *bv, slap_cf_aux_table *tab0 ));

/*
 * ch_malloc.c
 */
LDAP_SLAPD_V (BerMemoryFunctions) ch_mfuncs;
LDAP_SLAPD_F (void *) ch_malloc LDAP_P(( ber_len_t size ));
LDAP_SLAPD_F (void *) ch_realloc LDAP_P(( void *block, ber_len_t size ));
LDAP_SLAPD_F (void *) ch_calloc LDAP_P(( ber_len_t nelem, ber_len_t size ));
LDAP_SLAPD_F (char *) ch_strdup LDAP_P(( const char *string ));
LDAP_SLAPD_F (void) ch_free LDAP_P(( void * ));

#ifndef CH_FREE
#undef free
#define free ch_free
#endif

/*
 * compare.c
 */

LDAP_SLAPD_F (int) slap_compare_entry LDAP_P((
	Operation *op,
	Entry *e,
	AttributeAssertion *ava ));

/*
 * component.c
 */
#ifdef LDAP_COMP_MATCH
struct comp_attribute_aliasing;

LDAP_SLAPD_F (int) test_comp_filter_entry LDAP_P((
	Operation* op,
	Entry* e,
	MatchingRuleAssertion* mr));

LDAP_SLAPD_F (int) dup_comp_filter LDAP_P((
	Operation* op,
	struct berval *bv,
	ComponentFilter *in_f,
	ComponentFilter **out_f ));

LDAP_SLAPD_F (int) get_aliased_filter_aa LDAP_P((
	Operation* op,
	AttributeAssertion* a_assert,
	struct comp_attribute_aliasing* aa,
	const char** text ));

LDAP_SLAPD_F (int) get_aliased_filter LDAP_P((
	Operation* op,
	MatchingRuleAssertion* ma,
	struct comp_attribute_aliasing* aa,
	const char** text ));

LDAP_SLAPD_F (int) get_comp_filter LDAP_P((
	Operation* op,
	BerValue* bv,
	ComponentFilter** filt,
	const char **text ));

LDAP_SLAPD_F (int) insert_component_reference LDAP_P((
	ComponentReference *cr,
	ComponentReference** cr_list ));

LDAP_SLAPD_F (int) is_component_reference LDAP_P((
	char *attr ));

LDAP_SLAPD_F (int) extract_component_reference LDAP_P((
	char* attr,
	ComponentReference** cr ));

LDAP_SLAPD_F (int) componentFilterMatch LDAP_P(( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue ));

LDAP_SLAPD_F (int) directoryComponentsMatch LDAP_P((
        int *matchp,
        slap_mask_t flags,
        Syntax *syntax,
        MatchingRule *mr,
        struct berval *value,
        void *assertedValue ));

LDAP_SLAPD_F (int) allComponentsMatch LDAP_P((
        int *matchp,
        slap_mask_t flags,
        Syntax *syntax,
        MatchingRule *mr,
        struct berval *value,
        void *assertedValue ));

LDAP_SLAPD_F (ComponentReference*) dup_comp_ref LDAP_P((
	Operation *op,
	ComponentReference *cr ));
                                                                          
LDAP_SLAPD_F (int) componentFilterValidate LDAP_P(( 
	Syntax *syntax,
	struct berval* bv ));

LDAP_SLAPD_F (int) allComponentsValidate LDAP_P((
        Syntax *syntax,
        struct berval* bv ));

LDAP_SLAPD_F (void) component_free LDAP_P((
	ComponentFilter *f ));

LDAP_SLAPD_F (void) free_ComponentData LDAP_P((
	Attribute *a ));

LDAP_SLAPD_V (test_membership_func*) is_aliased_attribute;

LDAP_SLAPD_V (free_component_func*) component_destructor;

LDAP_SLAPD_V (get_component_info_func*) get_component_description;

LDAP_SLAPD_V (component_encoder_func*) component_encoder;

LDAP_SLAPD_V (convert_attr_to_comp_func*) attr_converter;

LDAP_SLAPD_V (alloc_nibble_func*) nibble_mem_allocator;

LDAP_SLAPD_V (free_nibble_func*) nibble_mem_free;
#endif

/*
 * controls.c
 */
LDAP_SLAPD_V( struct slap_control_ids ) slap_cids;
LDAP_SLAPD_F (void) slap_free_ctrls LDAP_P((
	Operation *op,
	LDAPControl **ctrls ));
LDAP_SLAPD_F (int) slap_add_ctrls LDAP_P((
	Operation *op,
	SlapReply *rs,
	LDAPControl **ctrls ));
LDAP_SLAPD_F (int) slap_parse_ctrl LDAP_P((
	Operation *op,
	SlapReply *rs,
	LDAPControl *control,
	const char **text ));
LDAP_SLAPD_F (int) get_ctrls LDAP_P((
	Operation *op,
	SlapReply *rs,
	int senderrors ));
LDAP_SLAPD_F (int) get_ctrls2 LDAP_P((
	Operation *op,
	SlapReply *rs,
	int senderrors,
	ber_tag_t ctag ));
LDAP_SLAPD_F (int) register_supported_control2 LDAP_P((
	const char *controloid,
	slap_mask_t controlmask,
	char **controlexops,
	SLAP_CTRL_PARSE_FN *controlparsefn,
	unsigned flags,
	int *controlcid ));
#define register_supported_control(oid, mask, exops, fn, cid) \
	register_supported_control2((oid), (mask), (exops), (fn), 0, (cid))
#ifdef SLAP_CONFIG_DELETE
LDAP_SLAPD_F (int) unregister_supported_control LDAP_P((
	const char* controloid ));
#endif /* SLAP_CONFIG_DELETE */
LDAP_SLAPD_F (int) slap_controls_init LDAP_P ((void));
LDAP_SLAPD_F (void) controls_destroy LDAP_P ((void));
LDAP_SLAPD_F (int) controls_root_dse_info LDAP_P ((Entry *e));
LDAP_SLAPD_F (int) get_supported_controls LDAP_P ((
	char ***ctrloidsp, slap_mask_t **ctrlmasks ));
LDAP_SLAPD_F (int) slap_find_control_id LDAP_P ((
	const char *oid, int *cid ));
LDAP_SLAPD_F (int) slap_global_control LDAP_P ((
	Operation *op, const char *oid, int *cid ));
LDAP_SLAPD_F (int) slap_remove_control LDAP_P((
	Operation	*op,
	SlapReply	*rs,
	int		ctrl,
	BI_chk_controls	fnc ));

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
LDAP_SLAPD_F (int)
slap_ctrl_session_tracking_add LDAP_P((
	Operation *op,
	SlapReply *rs,
	struct berval *ip,
	struct berval *name,
	struct berval *id,
	LDAPControl *ctrl ));
LDAP_SLAPD_F (int)
slap_ctrl_session_tracking_request_add LDAP_P((
	Operation *op, SlapReply *rs, LDAPControl *ctrl ));
#endif /* SLAP_CONTROL_X_SESSION_TRACKING */
#ifdef SLAP_CONTROL_X_WHATFAILED
LDAP_SLAPD_F (int)
slap_ctrl_whatFailed_add LDAP_P((
	Operation *op,
	SlapReply *rs,
	char **oids ));
#endif /* SLAP_CONTROL_X_WHATFAILED */

/*
 * config.c
 */
LDAP_SLAPD_F (int) read_config LDAP_P(( const char *fname, const char *dir ));
LDAP_SLAPD_F (void) config_destroy LDAP_P ((void));
LDAP_SLAPD_F (char **) slap_str2clist LDAP_P((
	char ***, char *, const char * ));
LDAP_SLAPD_F (int) bverb_to_mask LDAP_P((
	struct berval *bword,  slap_verbmasks *v ));
LDAP_SLAPD_F (int) verb_to_mask LDAP_P((
	const char *word,  slap_verbmasks *v ));
LDAP_SLAPD_F (int) verbs_to_mask LDAP_P((
	int argc, char *argv[], slap_verbmasks *v, slap_mask_t *m ));
LDAP_SLAPD_F (int) mask_to_verbs LDAP_P((
	slap_verbmasks *v, slap_mask_t m, BerVarray *bva ));
LDAP_SLAPD_F (int) mask_to_verbstring LDAP_P((
	slap_verbmasks *v, slap_mask_t m, char delim, struct berval *bv ));
LDAP_SLAPD_F (int) verbstring_to_mask LDAP_P((
	slap_verbmasks *v, char *str, char delim, slap_mask_t *m ));
LDAP_SLAPD_F (int) enum_to_verb LDAP_P((
	slap_verbmasks *v, slap_mask_t m, struct berval *bv ));
LDAP_SLAPD_F (int) slap_verbmasks_init LDAP_P(( slap_verbmasks **vp, slap_verbmasks *v ));
LDAP_SLAPD_F (int) slap_verbmasks_destroy LDAP_P(( slap_verbmasks *v ));
LDAP_SLAPD_F (int) slap_verbmasks_append LDAP_P(( slap_verbmasks **vp,
	slap_mask_t m, struct berval *v, slap_mask_t *ignore ));
LDAP_SLAPD_F (int) slap_tls_get_config LDAP_P((
	LDAP *ld, int opt, char **val ));
LDAP_SLAPD_F (void) bindconf_tls_defaults LDAP_P(( slap_bindconf *bc ));
LDAP_SLAPD_F (int) bindconf_tls_parse LDAP_P((
	const char *word,  slap_bindconf *bc ));
LDAP_SLAPD_F (int) bindconf_tls_unparse LDAP_P((
	slap_bindconf *bc, struct berval *bv ));
LDAP_SLAPD_F (int) bindconf_parse LDAP_P((
	const char *word,  slap_bindconf *bc ));
LDAP_SLAPD_F (int) bindconf_unparse LDAP_P((
	slap_bindconf *bc, struct berval *bv ));
LDAP_SLAPD_F (int) bindconf_tls_set LDAP_P((
	slap_bindconf *bc, LDAP *ld ));
LDAP_SLAPD_F (void) bindconf_free LDAP_P(( slap_bindconf *bc ));
LDAP_SLAPD_F (int) slap_client_connect LDAP_P(( LDAP **ldp, slap_bindconf *sb ));
LDAP_SLAPD_F (int) config_generic_wrapper LDAP_P(( Backend *be,
	const char *fname, int lineno, int argc, char **argv ));
LDAP_SLAPD_F (char *) anlist_unparse LDAP_P(( AttributeName *, char *, ber_len_t buflen ));
LDAP_SLAPD_F (int) slap_keepalive_parse( struct berval *val, void *bc,
	slap_cf_aux_table *tab0, const char *tabmsg, int unparse );

#ifdef LDAP_SLAPI
LDAP_SLAPD_V (int) slapi_plugins_used;
#endif

/*
 * connection.c
 */
LDAP_SLAPD_F (int) connections_init LDAP_P((void));
LDAP_SLAPD_F (int) connections_shutdown LDAP_P((void));
LDAP_SLAPD_F (int) connections_destroy LDAP_P((void));
LDAP_SLAPD_F (int) connections_timeout_idle LDAP_P((time_t));
LDAP_SLAPD_F (void) connections_drop LDAP_P((void));

LDAP_SLAPD_F (Connection *) connection_client_setup LDAP_P((
	ber_socket_t s,
	ldap_pvt_thread_start_t *func,
	void *arg ));
LDAP_SLAPD_F (void) connection_client_enable LDAP_P(( Connection *c ));
LDAP_SLAPD_F (void) connection_client_stop LDAP_P(( Connection *c ));

#ifdef LDAP_PF_LOCAL_SENDMSG
#define LDAP_PF_LOCAL_SENDMSG_ARG(arg)	, arg
#else
#define LDAP_PF_LOCAL_SENDMSG_ARG(arg)
#endif

LDAP_SLAPD_F (Connection *) connection_init LDAP_P((
	ber_socket_t s,
	Listener* url,
	const char* dnsname,
	const char* peername,
	int use_tls,
	slap_ssf_t ssf,
	struct berval *id
	LDAP_PF_LOCAL_SENDMSG_ARG(struct berval *peerbv)));

LDAP_SLAPD_F (void) connection_closing LDAP_P((
	Connection *c, const char *why ));
LDAP_SLAPD_F (int) connection_valid LDAP_P(( Connection *c ));
LDAP_SLAPD_F (const char *) connection_state2str LDAP_P(( int state ))
	LDAP_GCCATTR((const));

LDAP_SLAPD_F (int) connection_read_activate LDAP_P((ber_socket_t s));
LDAP_SLAPD_F (int) connection_write LDAP_P((ber_socket_t s));

LDAP_SLAPD_F (unsigned long) connections_nextid(void);

LDAP_SLAPD_F (Connection *) connection_first LDAP_P(( ber_socket_t * ));
LDAP_SLAPD_F (Connection *) connection_next LDAP_P((
	Connection *, ber_socket_t *));
LDAP_SLAPD_F (void) connection_done LDAP_P((Connection *));

LDAP_SLAPD_F (void) connection2anonymous LDAP_P((Connection *));
LDAP_SLAPD_F (void) connection_fake_init LDAP_P((
	Connection *conn,
	OperationBuffer *opbuf,
	void *threadctx ));
LDAP_SLAPD_F (void) connection_fake_init2 LDAP_P((
	Connection *conn,
	OperationBuffer *opbuf,
	void *threadctx,
	int newmem ));
LDAP_SLAPD_F (void) operation_fake_init LDAP_P((
	Connection *conn,
	Operation *op,
	void *threadctx,
	int newmem ));
LDAP_SLAPD_F (void) connection_assign_nextid LDAP_P((Connection *));

/*
 * cr.c
 */
LDAP_SLAPD_F (int) cr_schema_info( Entry *e );
LDAP_SLAPD_F (void) cr_unparse LDAP_P((
	BerVarray *bva, ContentRule *start, ContentRule *end, int system ));

LDAP_SLAPD_F (int) cr_add LDAP_P((
	LDAPContentRule *oc,
	int user,
	ContentRule **scr,
	const char **err));

LDAP_SLAPD_F (void) cr_destroy LDAP_P(( void ));

LDAP_SLAPD_F (ContentRule *) cr_find LDAP_P((
	const char *crname));
LDAP_SLAPD_F (ContentRule *) cr_bvfind LDAP_P((
	struct berval *crname));

/*
 * ctxcsn.c
 */

LDAP_SLAPD_V( int ) slap_serverID;
LDAP_SLAPD_V( const struct berval ) slap_ldapsync_bv;
LDAP_SLAPD_V( const struct berval ) slap_ldapsync_cn_bv;
LDAP_SLAPD_F (void) slap_get_commit_csn LDAP_P((
	Operation *, struct berval *maxcsn, int *foundit ));
LDAP_SLAPD_F (void) slap_rewind_commit_csn LDAP_P(( Operation * ));
LDAP_SLAPD_F (void) slap_graduate_commit_csn LDAP_P(( Operation * ));
LDAP_SLAPD_F (Entry *) slap_create_context_csn_entry LDAP_P(( Backend *, struct berval *));
LDAP_SLAPD_F (int) slap_get_csn LDAP_P(( Operation *, struct berval *, int ));
LDAP_SLAPD_F (void) slap_queue_csn LDAP_P(( Operation *, struct berval * ));

/*
 * daemon.c
 */
LDAP_SLAPD_F (void) slapd_add_internal(ber_socket_t s, int isactive);
LDAP_SLAPD_F (int) slapd_daemon_init( const char *urls );
LDAP_SLAPD_F (int) slapd_daemon_destroy(void);
LDAP_SLAPD_F (int) slapd_daemon(void);
LDAP_SLAPD_F (Listener **)	slapd_get_listeners LDAP_P((void));
LDAP_SLAPD_F (void) slapd_remove LDAP_P((ber_socket_t s, Sockbuf *sb,
	int wasactive, int wake, int locked ));

LDAP_SLAPD_F (RETSIGTYPE) slap_sig_shutdown LDAP_P((int sig));
LDAP_SLAPD_F (RETSIGTYPE) slap_sig_wake LDAP_P((int sig));
LDAP_SLAPD_F (void) slap_wake_listener LDAP_P((void));

LDAP_SLAPD_F (void) slap_suspend_listeners LDAP_P((void));
LDAP_SLAPD_F (void) slap_resume_listeners LDAP_P((void));

LDAP_SLAPD_F (void) slapd_set_write LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (void) slapd_clr_write LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (void) slapd_set_read LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (int) slapd_clr_read LDAP_P((ber_socket_t s, int wake));
LDAP_SLAPD_F (int) slapd_wait_writer( ber_socket_t sd );
LDAP_SLAPD_F (void) slapd_shutsock( ber_socket_t sd );

LDAP_SLAPD_V (volatile sig_atomic_t) slapd_abrupt_shutdown;
LDAP_SLAPD_V (volatile sig_atomic_t) slapd_shutdown;
LDAP_SLAPD_V (int) slapd_register_slp;
LDAP_SLAPD_V (const char *) slapd_slp_attrs;
LDAP_SLAPD_V (slap_ssf_t) local_ssf;
LDAP_SLAPD_V (struct runqueue_s) slapd_rq;
LDAP_SLAPD_V (int) slapd_daemon_threads;
LDAP_SLAPD_V (int) slapd_daemon_mask;
#ifdef LDAP_TCP_BUFFER
LDAP_SLAPD_V (int) slapd_tcp_rmem;
LDAP_SLAPD_V (int) slapd_tcp_wmem;
#endif /* LDAP_TCP_BUFFER */

#ifdef HAVE_WINSOCK
LDAP_SLAPD_F (ber_socket_t) slapd_socknew(ber_socket_t s);
LDAP_SLAPD_F (ber_socket_t) slapd_sock2fd(ber_socket_t s);
LDAP_SLAPD_V (SOCKET *) slapd_ws_sockets;
#define	SLAP_FD2SOCK(s)	slapd_ws_sockets[s]
#define	SLAP_SOCK2FD(s)	slapd_sock2fd(s)
#define	SLAP_SOCKNEW(s)	slapd_socknew(s)
#else
#define	SLAP_FD2SOCK(s)	s
#define	SLAP_SOCK2FD(s)	s
#define	SLAP_SOCKNEW(s)	s
#endif

/*
 * dn.c
 */

#define dn_match(dn1, dn2) 	( ber_bvcmp((dn1), (dn2)) == 0 )
#define bvmatch(bv1, bv2)	( ((bv1)->bv_len == (bv2)->bv_len) && (memcmp((bv1)->bv_val, (bv2)->bv_val, (bv1)->bv_len) == 0) )

LDAP_SLAPD_F (int) dnValidate LDAP_P((
	Syntax *syntax, 
	struct berval *val ));
LDAP_SLAPD_F (int) rdnValidate LDAP_P((
	Syntax *syntax, 
	struct berval *val ));

LDAP_SLAPD_F (slap_mr_normalize_func) dnNormalize;

LDAP_SLAPD_F (slap_mr_normalize_func) rdnNormalize;

LDAP_SLAPD_F (slap_syntax_transform_func) dnPretty;

LDAP_SLAPD_F (slap_syntax_transform_func) rdnPretty;

LDAP_SLAPD_F (int) dnPrettyNormal LDAP_P(( 
	Syntax *syntax, 
	struct berval *val, 
	struct berval *pretty,
	struct berval *normal,
	void *ctx ));

LDAP_SLAPD_F (int) dnMatch LDAP_P(( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue ));

LDAP_SLAPD_F (int) dnRelativeMatch LDAP_P(( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue ));

LDAP_SLAPD_F (int) rdnMatch LDAP_P(( 
	int *matchp, 
	slap_mask_t flags, 
	Syntax *syntax, 
	MatchingRule *mr,
	struct berval *value, 
	void *assertedValue ));


LDAP_SLAPD_F (int) dnIsSuffix LDAP_P((
	const struct berval *dn, const struct berval *suffix ));

LDAP_SLAPD_F (int) dnIsWithinScope LDAP_P((
	struct berval *ndn, struct berval *nbase, int scope ));

LDAP_SLAPD_F (int) dnIsSuffixScope LDAP_P((
	struct berval *ndn, struct berval *nbase, int scope ));

LDAP_SLAPD_F (int) dnIsOneLevelRDN LDAP_P(( struct berval *rdn ));

LDAP_SLAPD_F (int) dnExtractRdn LDAP_P((
	struct berval *dn, struct berval *rdn, void *ctx ));

LDAP_SLAPD_F (int) rdn_validate LDAP_P(( struct berval * rdn ));

LDAP_SLAPD_F (ber_len_t) dn_rdnlen LDAP_P(( Backend *be, struct berval *dn ));

LDAP_SLAPD_F (void) build_new_dn LDAP_P((
	struct berval * new_dn,
	struct berval * parent_dn,
	struct berval * newrdn,
	void *memctx ));

LDAP_SLAPD_F (void) dnParent LDAP_P(( struct berval *dn, struct berval *pdn ));
LDAP_SLAPD_F (void) dnRdn LDAP_P(( struct berval *dn, struct berval *rdn ));

LDAP_SLAPD_F (int) dnX509normalize LDAP_P(( void *x509_name, struct berval *out ));

LDAP_SLAPD_F (int) dnX509peerNormalize LDAP_P(( void *ssl, struct berval *dn ));

LDAP_SLAPD_F (int) dnPrettyNormalDN LDAP_P(( Syntax *syntax, struct berval *val, LDAPDN *dn, int flags, void *ctx ));
#define dnPrettyDN(syntax, val, dn, ctx) \
	dnPrettyNormalDN((syntax),(val),(dn), SLAP_LDAPDN_PRETTY, ctx)
#define dnNormalDN(syntax, val, dn, ctx) \
	dnPrettyNormalDN((syntax),(val),(dn), 0, ctx)

typedef int (SLAP_CERT_MAP_FN) LDAP_P(( void *ssl, struct berval *dn ));
LDAP_SLAPD_F (int) register_certificate_map_function LDAP_P(( SLAP_CERT_MAP_FN *fn ));

/*
 * entry.c
 */
LDAP_SLAPD_V (const Entry) slap_entry_root;

LDAP_SLAPD_F (int) entry_init LDAP_P((void));
LDAP_SLAPD_F (int) entry_destroy LDAP_P((void));

LDAP_SLAPD_F (Entry *) str2entry LDAP_P(( char	*s ));
LDAP_SLAPD_F (Entry *) str2entry2 LDAP_P(( char	*s, int checkvals ));
LDAP_SLAPD_F (char *) entry2str LDAP_P(( Entry *e, int *len ));
LDAP_SLAPD_F (char *) entry2str_wrap LDAP_P(( Entry *e, int *len, ber_len_t wrap ));

LDAP_SLAPD_F (ber_len_t) entry_flatsize LDAP_P(( Entry *e, int norm ));
LDAP_SLAPD_F (void) entry_partsize LDAP_P(( Entry *e, ber_len_t *len,
	int *nattrs, int *nvals, int norm ));

LDAP_SLAPD_F (int) entry_header LDAP_P(( EntryHeader *eh ));
LDAP_SLAPD_F (int) entry_decode_dn LDAP_P((
	EntryHeader *eh, struct berval *dn, struct berval *ndn ));
#ifdef SLAP_ZONE_ALLOC
LDAP_SLAPD_F (int) entry_decode LDAP_P((
						EntryHeader *eh, Entry **e, void *ctx ));
#else
LDAP_SLAPD_F (int) entry_decode LDAP_P((
						EntryHeader *eh, Entry **e ));
#endif
LDAP_SLAPD_F (int) entry_encode LDAP_P(( Entry *e, struct berval *bv ));

LDAP_SLAPD_F (void) entry_clean LDAP_P(( Entry *e ));
LDAP_SLAPD_F (void) entry_free LDAP_P(( Entry *e ));
LDAP_SLAPD_F (int) entry_cmp LDAP_P(( Entry *a, Entry *b ));
LDAP_SLAPD_F (int) entry_dn_cmp LDAP_P(( const void *v_a, const void *v_b ));
LDAP_SLAPD_F (int) entry_id_cmp LDAP_P(( const void *v_a, const void *v_b ));
LDAP_SLAPD_F (Entry *) entry_dup LDAP_P(( Entry *e ));
LDAP_SLAPD_F (Entry *) entry_dup2 LDAP_P(( Entry *dest, Entry *src ));
LDAP_SLAPD_F (Entry *) entry_dup_bv LDAP_P(( Entry *e ));
LDAP_SLAPD_F (Entry *) entry_alloc LDAP_P((void));
LDAP_SLAPD_F (int) entry_prealloc LDAP_P((int num));

/*
 * extended.c
 */
LDAP_SLAPD_F (int) exop_root_dse_info LDAP_P ((Entry *e));

#define exop_is_write( op )	((op->ore_flags & SLAP_EXOP_WRITES) != 0)

LDAP_SLAPD_V( const struct berval ) slap_EXOP_CANCEL;
LDAP_SLAPD_V( const struct berval ) slap_EXOP_WHOAMI;
LDAP_SLAPD_V( const struct berval ) slap_EXOP_MODIFY_PASSWD;
LDAP_SLAPD_V( const struct berval ) slap_EXOP_START_TLS;
#ifdef LDAP_X_TXN
LDAP_SLAPD_V( const struct berval ) slap_EXOP_TXN_START;
LDAP_SLAPD_V( const struct berval ) slap_EXOP_TXN_END;
#endif

typedef int (SLAP_EXTOP_MAIN_FN) LDAP_P(( Operation *op, SlapReply *rs ));

typedef int (SLAP_EXTOP_GETOID_FN) LDAP_P((
	int index, struct berval *oid, int blen ));

LDAP_SLAPD_F (int) load_extop2 LDAP_P((
	const struct berval *ext_oid,
	slap_mask_t flags,
	SLAP_EXTOP_MAIN_FN *ext_main,
	unsigned tmpflags ));
#define load_extop(ext_oid, flags, ext_main) \
	load_extop2((ext_oid), (flags), (ext_main), 0)
LDAP_SLAPD_F (int) unload_extop LDAP_P((
	const struct berval *ext_oid,
	SLAP_EXTOP_MAIN_FN *ext_main,
	unsigned tmpflags ));

LDAP_SLAPD_F (int) extops_init LDAP_P(( void ));

LDAP_SLAPD_F (int) extops_kill LDAP_P(( void ));

LDAP_SLAPD_F (struct berval *) get_supported_extop LDAP_P((int index));

/*
 * txn.c
 */
#ifdef LDAP_X_TXN
LDAP_SLAPD_F ( SLAP_CTRL_PARSE_FN ) txn_spec_ctrl;
LDAP_SLAPD_F ( SLAP_EXTOP_MAIN_FN ) txn_start_extop;
LDAP_SLAPD_F ( SLAP_EXTOP_MAIN_FN ) txn_end_extop;
#endif

/*
 * cancel.c
 */
LDAP_SLAPD_F ( SLAP_EXTOP_MAIN_FN ) cancel_extop;

/*
 * filter.c
 */
LDAP_SLAPD_F (int) get_filter LDAP_P((
	Operation *op,
	BerElement *ber,
	Filter **filt,
	const char **text ));

LDAP_SLAPD_F (void) filter_free LDAP_P(( Filter *f ));
LDAP_SLAPD_F (void) filter_free_x LDAP_P(( Operation *op, Filter *f, int freeme ));
LDAP_SLAPD_F (void) filter2bv LDAP_P(( Filter *f, struct berval *bv ));
LDAP_SLAPD_F (void) filter2bv_x LDAP_P(( Operation *op, Filter *f, struct berval *bv ));
LDAP_SLAPD_F (void) filter2bv_undef LDAP_P(( Filter *f, int noundef, struct berval *bv ));
LDAP_SLAPD_F (void) filter2bv_undef_x LDAP_P(( Operation *op, Filter *f, int noundef, struct berval *bv ));
LDAP_SLAPD_F (Filter *) filter_dup LDAP_P(( Filter *f, void *memctx ));

LDAP_SLAPD_F (int) get_vrFilter LDAP_P(( Operation *op, BerElement *ber,
	ValuesReturnFilter **f,
	const char **text ));

LDAP_SLAPD_F (void) vrFilter_free LDAP_P(( Operation *op, ValuesReturnFilter *f ));
LDAP_SLAPD_F (void) vrFilter2bv LDAP_P(( Operation *op, ValuesReturnFilter *f, struct berval *fstr ));

LDAP_SLAPD_F (int) filter_has_subordinates LDAP_P(( Filter *filter ));
#define filter_escape_value( in, out )		ldap_bv2escaped_filter_value_x( (in), (out), 0, NULL )
#define filter_escape_value_x( in, out, ctx )	ldap_bv2escaped_filter_value_x( (in), (out), 0, ctx )

LDAP_SLAPD_V (const Filter *) slap_filter_objectClass_pres;
LDAP_SLAPD_V (const struct berval *) slap_filterstr_objectClass_pres;

LDAP_SLAPD_F (int) filter_init LDAP_P(( void ));
LDAP_SLAPD_F (void) filter_destroy LDAP_P(( void ));
/*
 * filterentry.c
 */

LDAP_SLAPD_F (int) test_filter LDAP_P(( Operation *op, Entry *e, Filter *f ));

/*
 * frontend.c
 */
LDAP_SLAPD_F (int) frontend_init LDAP_P(( void ));

/*
 * globals.c
 */

LDAP_SLAPD_V( const struct berval ) slap_empty_bv;
LDAP_SLAPD_V( const struct berval ) slap_unknown_bv;
LDAP_SLAPD_V( const struct berval ) slap_true_bv;
LDAP_SLAPD_V( const struct berval ) slap_false_bv;
LDAP_SLAPD_V( struct slap_sync_cookie_s ) slap_sync_cookie;
LDAP_SLAPD_V( void * ) slap_tls_ctx;
LDAP_SLAPD_V( LDAP * ) slap_tls_ld;

/*
 * index.c
 */
LDAP_SLAPD_F (int) slap_str2index LDAP_P(( const char *str, slap_mask_t *idx ));
LDAP_SLAPD_F (void) slap_index2bvlen LDAP_P(( slap_mask_t idx, struct berval *bv ));
LDAP_SLAPD_F (void) slap_index2bv LDAP_P(( slap_mask_t idx, struct berval *bv ));

/*
 * init.c
 */
LDAP_SLAPD_F (int)	slap_init LDAP_P((int mode, const char* name));
LDAP_SLAPD_F (int)	slap_startup LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int)	slap_shutdown LDAP_P(( Backend *be ));
LDAP_SLAPD_F (int)	slap_destroy LDAP_P((void));
LDAP_SLAPD_F (void) slap_counters_init LDAP_P((slap_counters_t *sc));
LDAP_SLAPD_F (void) slap_counters_destroy LDAP_P((slap_counters_t *sc));

LDAP_SLAPD_V (char *)	slap_known_controls[];

/*
 * ldapsync.c
 */
LDAP_SLAPD_F (void) slap_compose_sync_cookie LDAP_P((
				Operation *, struct berval *, BerVarray, int, int ));
LDAP_SLAPD_F (void) slap_sync_cookie_free LDAP_P((
				struct sync_cookie *, int free_cookie ));
LDAP_SLAPD_F (int) slap_parse_csn_sid LDAP_P((
				struct berval * ));
LDAP_SLAPD_F (int *) slap_parse_csn_sids LDAP_P((
				BerVarray, int, void *memctx ));
LDAP_SLAPD_F (int) slap_sort_csn_sids LDAP_P((
				BerVarray, int *, int, void *memctx ));
LDAP_SLAPD_F (void) slap_insert_csn_sids LDAP_P((
				struct sync_cookie *ck, int, int, struct berval * ));
LDAP_SLAPD_F (int) slap_parse_sync_cookie LDAP_P((
				struct sync_cookie *, void *memctx ));
LDAP_SLAPD_F (void) slap_reparse_sync_cookie LDAP_P((
				struct sync_cookie *, void *memctx ));
LDAP_SLAPD_F (int) slap_init_sync_cookie_ctxcsn LDAP_P((
				struct sync_cookie * ));
LDAP_SLAPD_F (struct sync_cookie *) slap_dup_sync_cookie LDAP_P((
				struct sync_cookie *, struct sync_cookie * ));
LDAP_SLAPD_F (int) slap_build_syncUUID_set LDAP_P((
				Operation *, BerVarray *, Entry * ));

/*
 * limits.c
 */
LDAP_SLAPD_F (int) limits_parse LDAP_P((
	Backend *be, const char *fname, int lineno,
	int argc, char **argv ));
LDAP_SLAPD_F (int) limits_parse_one LDAP_P(( const char *arg, 
	struct slap_limits_set *limit ));
LDAP_SLAPD_F (int) limits_check LDAP_P((
	Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) limits_unparse_one LDAP_P(( 
	struct slap_limits_set *limit, int which, struct berval *bv, ber_len_t buflen ));
LDAP_SLAPD_F (int) limits_unparse LDAP_P(( 
	struct slap_limits *limit, struct berval *bv, ber_len_t buflen ));
LDAP_SLAPD_F (void) limits_free_one LDAP_P(( 
	struct slap_limits	*lm ));
LDAP_SLAPD_F (void) limits_destroy LDAP_P(( struct slap_limits **lm ));

/*
 * lock.c
 */
LDAP_SLAPD_F (FILE *) lock_fopen LDAP_P(( const char *fname,
	const char *type, FILE **lfp ));
LDAP_SLAPD_F (int) lock_fclose LDAP_P(( FILE *fp, FILE *lfp ));

/*
 * main.c
 */
LDAP_SLAPD_F (int)
parse_debug_level LDAP_P(( const char *arg, int *levelp, char ***unknowns ));
LDAP_SLAPD_F (int)
parse_syslog_level LDAP_P(( const char *arg, int *levelp ));
LDAP_SLAPD_F (int)
parse_syslog_user LDAP_P(( const char *arg, int *syslogUser ));
LDAP_SLAPD_F (int)
parse_debug_unknowns LDAP_P(( char **unknowns, int *levelp ));

/*
 * matchedValues.c
 */
LDAP_SLAPD_F (int) filter_matched_values( 
	Operation	*op,
	Attribute	*a,
	char		***e_flags );

/*
 * modrdn.c
 */
LDAP_SLAPD_F (int) slap_modrdn2mods LDAP_P((
	Operation	*op,
	SlapReply	*rs ));

/*
 * modify.c
 */
LDAP_SLAPD_F( int ) slap_mods_obsolete_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F( int ) slap_mods_no_user_mod_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F ( int ) slap_mods_no_repl_user_mod_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf,
	size_t textlen );

LDAP_SLAPD_F( int ) slap_mods_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf, size_t textlen, void *ctx );

LDAP_SLAPD_F( int ) slap_sort_vals(
	Modifications *ml,
	const char **text,
	int *dup,
	void *ctx );

LDAP_SLAPD_F( void ) slap_timestamp(
	time_t *tm,
	struct berval *bv );

LDAP_SLAPD_F( void ) slap_mods_opattrs(
	Operation *op,
	Modifications **modsp,
	int manage_ctxcsn );

LDAP_SLAPD_F( int ) slap_parse_modlist(
	Operation *op,
	SlapReply *rs,
	BerElement *ber,
	req_modify_s *ms );

/*
 * mods.c
 */
LDAP_SLAPD_F( int ) modify_add_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_delete_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_delete_vindex( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen, int *idx );
LDAP_SLAPD_F( int ) modify_replace_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );
LDAP_SLAPD_F( int ) modify_increment_values( Entry *e,
	Modification *mod,
	int permissive,
	const char **text, char *textbuf, size_t textlen );

LDAP_SLAPD_F( void ) slap_mod_free( Modification *mod, int freeit );
LDAP_SLAPD_F( void ) slap_mods_free( Modifications *mods, int freevals );
LDAP_SLAPD_F( void ) slap_modlist_free( LDAPModList *ml );

/*
 * module.c
 */
#ifdef SLAPD_MODULES

LDAP_SLAPD_F (int) module_init LDAP_P(( void ));
LDAP_SLAPD_F (int) module_kill LDAP_P(( void ));

LDAP_SLAPD_F (int) load_null_module(
	const void *module, const char *file_name);
LDAP_SLAPD_F (int) load_extop_module(
	const void *module, const char *file_name);

LDAP_SLAPD_F (int) module_load LDAP_P((
	const char* file_name,
	int argc, char *argv[] ));
LDAP_SLAPD_F (int) module_path LDAP_P(( const char* path ));
LDAP_SLAPD_F (int) module_unload LDAP_P(( const char* file_name ));

LDAP_SLAPD_F (void *) module_handle LDAP_P(( const char* file_name ));

LDAP_SLAPD_F (void *) module_resolve LDAP_P((
	const void *module, const char *name));

#endif /* SLAPD_MODULES */

/* mr.c */
LDAP_SLAPD_F (MatchingRule *) mr_bvfind LDAP_P((struct berval *mrname));
LDAP_SLAPD_F (MatchingRule *) mr_find LDAP_P((const char *mrname));
LDAP_SLAPD_F (int) mr_add LDAP_P(( LDAPMatchingRule *mr,
	slap_mrule_defs_rec *def,
	MatchingRule * associated,
	const char **err ));
LDAP_SLAPD_F (void) mr_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) register_matching_rule LDAP_P((
	slap_mrule_defs_rec *def ));

LDAP_SLAPD_F (void) mru_destroy LDAP_P(( void ));
LDAP_SLAPD_F (int) matching_rule_use_init LDAP_P(( void ));

LDAP_SLAPD_F (int) mr_schema_info LDAP_P(( Entry *e ));
LDAP_SLAPD_F (int) mru_schema_info LDAP_P(( Entry *e ));

LDAP_SLAPD_F (int) mr_usable_with_at LDAP_P(( MatchingRule *mr,
	AttributeType *at ));
LDAP_SLAPD_F (int) mr_make_syntax_compat_with_mr LDAP_P((
	Syntax		*syn,
	MatchingRule	*mr ));
LDAP_SLAPD_F (int) mr_make_syntax_compat_with_mrs LDAP_P((
	const char *syntax,
	char *const *mrs ));

/*
 * mra.c
 */
LDAP_SLAPD_F (int) get_mra LDAP_P((
	Operation *op,
	BerElement *ber,
	Filter *f,
	const char **text ));
LDAP_SLAPD_F (void) mra_free LDAP_P((
	Operation *op,
	MatchingRuleAssertion *mra,
	int freeit ));

/* oc.c */
LDAP_SLAPD_F (int) oc_add LDAP_P((
	LDAPObjectClass *oc,
	int user,
	ObjectClass **soc,
	ObjectClass *prev,
	const char **err));
LDAP_SLAPD_F (void) oc_destroy LDAP_P(( void ));

LDAP_SLAPD_F (ObjectClass *) oc_find LDAP_P((
	const char *ocname));
LDAP_SLAPD_F (ObjectClass *) oc_bvfind LDAP_P((
	struct berval *ocname));
LDAP_SLAPD_F (ObjectClass *) oc_bvfind_undef LDAP_P((
	struct berval *ocname));
LDAP_SLAPD_F (int) is_object_subclass LDAP_P((
	ObjectClass *sup,
	ObjectClass *sub ));

LDAP_SLAPD_F (int) is_entry_objectclass LDAP_P((
	Entry *, ObjectClass *oc, unsigned flags ));
#define	is_entry_objectclass_or_sub(e,oc) \
	(is_entry_objectclass((e),(oc),SLAP_OCF_CHECK_SUP))
#define is_entry_alias(e)		\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_ALIAS) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_alias, SLAP_OCF_SET_FLAGS))
#define is_entry_referral(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_REFERRAL) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_referral, SLAP_OCF_SET_FLAGS))
#define is_entry_subentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_SUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_subentry, SLAP_OCF_SET_FLAGS))
#define is_entry_collectiveAttributeSubentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_collectiveAttributeSubentry, SLAP_OCF_SET_FLAGS))
#define is_entry_dynamicObject(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_DYNAMICOBJECT) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_dynamicObject, SLAP_OCF_SET_FLAGS))
#define is_entry_glue(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_GLUE) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_glue, SLAP_OCF_SET_FLAGS))
#define is_entry_syncProviderSubentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_SYNCPROVIDERSUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_syncProviderSubentry, SLAP_OCF_SET_FLAGS))
#define is_entry_syncConsumerSubentry(e)	\
	(((e)->e_ocflags & SLAP_OC__END) \
	 ? (((e)->e_ocflags & SLAP_OC_SYNCCONSUMERSUBENTRY) != 0) \
	 : is_entry_objectclass((e), slap_schema.si_oc_syncConsumerSubentry, SLAP_OCF_SET_FLAGS))

LDAP_SLAPD_F (int) oc_schema_info( Entry *e );

LDAP_SLAPD_F (int) oc_start LDAP_P(( ObjectClass **oc ));
LDAP_SLAPD_F (int) oc_next LDAP_P(( ObjectClass **oc ));
LDAP_SLAPD_F (void) oc_delete LDAP_P(( ObjectClass *oc ));

LDAP_SLAPD_F (void) oc_unparse LDAP_P((
	BerVarray *bva, ObjectClass *start, ObjectClass *end, int system ));

LDAP_SLAPD_F (int) register_oc LDAP_P((
	const char *desc,
	ObjectClass **oc,
	int dupok ));

/*
 * oidm.c
 */
LDAP_SLAPD_F(char *) oidm_find(char *oid);
LDAP_SLAPD_F (void) oidm_destroy LDAP_P(( void ));
LDAP_SLAPD_F (void) oidm_unparse LDAP_P((
	BerVarray *bva, OidMacro *start, OidMacro *end, int system ));
LDAP_SLAPD_F (int) parse_oidm LDAP_P((
	struct config_args_s *ca, int user, OidMacro **om ));

/*
 * operation.c
 */
LDAP_SLAPD_F (void) slap_op_init LDAP_P(( void ));
LDAP_SLAPD_F (void) slap_op_destroy LDAP_P(( void ));
LDAP_SLAPD_F (void) slap_op_groups_free LDAP_P(( Operation *op ));
LDAP_SLAPD_F (void) slap_op_free LDAP_P(( Operation *op, void *ctx ));
LDAP_SLAPD_F (void) slap_op_time LDAP_P(( time_t *t, int *n ));
LDAP_SLAPD_F (Operation *) slap_op_alloc LDAP_P((
	BerElement *ber, ber_int_t msgid,
	ber_tag_t tag, ber_int_t id, void *ctx ));

LDAP_SLAPD_F (slap_op_t) slap_req2op LDAP_P(( ber_tag_t tag ));

/*
 * operational.c
 */
LDAP_SLAPD_F (Attribute *) slap_operational_subschemaSubentry( Backend *be );
LDAP_SLAPD_F (Attribute *) slap_operational_entryDN( Entry *e );
LDAP_SLAPD_F (Attribute *) slap_operational_hasSubordinate( int has );

/*
 * overlays.c
 */
LDAP_SLAPD_F (int) overlay_init( void );

/*
 * passwd.c
 */
LDAP_SLAPD_F (SLAP_EXTOP_MAIN_FN) passwd_extop;

LDAP_SLAPD_F (int) slap_passwd_check(
	Operation		*op,
	Entry			*e,
	Attribute		*a,
	struct berval		*cred,
	const char		**text );

LDAP_SLAPD_F (void) slap_passwd_generate( struct berval * );

LDAP_SLAPD_F (void) slap_passwd_hash(
	struct berval		*cred,
	struct berval		*hash,
	const char		**text );

LDAP_SLAPD_F (void) slap_passwd_hash_type(
	struct berval		*cred,
	struct berval		*hash,
	char			*htype,
	const char		**text );

LDAP_SLAPD_F (struct berval *) slap_passwd_return(
	struct berval		*cred );

LDAP_SLAPD_F (int) slap_passwd_parse(
	struct berval		*reqdata,
	struct berval		*id,
	struct berval		*oldpass,
	struct berval		*newpass,
	const char		**text );

LDAP_SLAPD_F (void) slap_passwd_init (void);

/*
 * phonetic.c
 */
LDAP_SLAPD_F (char *) phonetic LDAP_P(( char *s ));

/*
 * referral.c
 */
LDAP_SLAPD_F (int) validate_global_referral LDAP_P((
	const char *url ));

LDAP_SLAPD_F (BerVarray) get_entry_referrals LDAP_P((
	Operation *op, Entry *e ));

LDAP_SLAPD_F (BerVarray) referral_rewrite LDAP_P((
	BerVarray refs,
	struct berval *base,
	struct berval *target,
	int scope ));

LDAP_SLAPD_F (int) get_alias_dn LDAP_P((
	Entry *e,
	struct berval *ndn,
	int *err,
	const char **text ));

/*
 * result.c
 */
#if USE_RS_ASSERT /*defined(USE_RS_ASSERT)?(USE_RS_ASSERT):defined(LDAP_TEST)*/
#ifdef __GNUC__
# define RS_FUNC_	__FUNCTION__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__) >= 199901L
# define RS_FUNC_	__func__
#else
# define rs_assert_(file, line, func, cond) rs_assert__(file, line, cond)
#endif
LDAP_SLAPD_V(int)  rs_suppress_assert;
LDAP_SLAPD_F(void) rs_assert_(const char*, unsigned, const char*, const char*);
# define RS_ASSERT(cond)		((rs_suppress_assert > 0 || (cond)) \
	? (void) 0 : rs_assert_(__FILE__, __LINE__, RS_FUNC_, #cond))
#else
# define RS_ASSERT(cond)		((void) 0)
# define rs_assert_ok(rs)		((void) (rs))
# define rs_assert_ready(rs)	((void) (rs))
# define rs_assert_done(rs)		((void) (rs))
#endif
LDAP_SLAPD_F (void) (rs_assert_ok)		LDAP_P(( const SlapReply *rs ));
LDAP_SLAPD_F (void) (rs_assert_ready)	LDAP_P(( const SlapReply *rs ));
LDAP_SLAPD_F (void) (rs_assert_done)	LDAP_P(( const SlapReply *rs ));

#define rs_reinit(rs, type)	do {			\
		SlapReply *const rsRI = (rs);		\
		rs_assert_done( rsRI );				\
		rsRI->sr_type = (type);				\
		/* Got type before memset in case of rs_reinit(rs, rs->sr_type) */ \
		assert( !offsetof( SlapReply, sr_type ) );	\
		memset( (slap_reply_t *) rsRI + 1, 0,		\
			sizeof(*rsRI) - sizeof(slap_reply_t) );	\
	} while ( 0 )
LDAP_SLAPD_F (void) (rs_reinit)	LDAP_P(( SlapReply *rs, slap_reply_t type ));
LDAP_SLAPD_F (void) rs_flush_entry LDAP_P(( Operation *op,
	SlapReply *rs, slap_overinst *on ));
LDAP_SLAPD_F (void) rs_replace_entry LDAP_P(( Operation *op,
	SlapReply *rs, slap_overinst *on, Entry *e ));
LDAP_SLAPD_F (int) rs_entry2modifiable LDAP_P(( Operation *op,
	SlapReply *rs, slap_overinst *on ));
#define rs_ensure_entry_modifiable rs_entry2modifiable /* older name */
LDAP_SLAPD_F (void) slap_send_ldap_result LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (void) send_ldap_sasl LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (void) send_ldap_disconnect LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (void) slap_send_ldap_extended LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (void) slap_send_ldap_intermediate LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (void) slap_send_search_result LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) slap_send_search_reference LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) slap_send_search_entry LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) slap_null_cb LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) slap_freeself_cb LDAP_P(( Operation *op, SlapReply *rs ));

LDAP_SLAPD_V( const struct berval ) slap_pre_read_bv;
LDAP_SLAPD_V( const struct berval ) slap_post_read_bv;
LDAP_SLAPD_F (int) slap_read_controls LDAP_P(( Operation *op, SlapReply *rs,
	Entry *e, const struct berval *oid, LDAPControl **ctrl ));

LDAP_SLAPD_F (int) str2result LDAP_P(( char *s,
	int *code, char **matched, char **info ));
LDAP_SLAPD_F (int) slap_map_api2result LDAP_P(( SlapReply *rs ));
LDAP_SLAPD_F (slap_mask_t) slap_attr_flags LDAP_P(( AttributeName *an ));
LDAP_SLAPD_F (ber_tag_t) slap_req2res LDAP_P(( ber_tag_t tag ));

LDAP_SLAPD_V( const struct berval ) slap_dummy_bv;

/*
 * root_dse.c
 */
LDAP_SLAPD_F (int) root_dse_init LDAP_P(( void ));
LDAP_SLAPD_F (int) root_dse_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) root_dse_info LDAP_P((
	Connection *conn,
	Entry **e,
	const char **text ));

LDAP_SLAPD_F (int) root_dse_read_file LDAP_P((
	const char *file));

LDAP_SLAPD_F (int) slap_discover_feature LDAP_P((
	slap_bindconf	*sb,
	const char	*attr,
	const char	*val ));

LDAP_SLAPD_F (int) supported_feature_load LDAP_P(( struct berval *f ));
LDAP_SLAPD_F (int) supported_feature_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) entry_info_register LDAP_P(( SLAP_ENTRY_INFO_FN func, void *arg ));
LDAP_SLAPD_F (int) entry_info_unregister LDAP_P(( SLAP_ENTRY_INFO_FN func, void *arg ));
LDAP_SLAPD_F (void) entry_info_destroy LDAP_P(( void ));

/*
 * sasl.c
 */
LDAP_SLAPD_F (int) slap_sasl_init(void);
LDAP_SLAPD_F (char *) slap_sasl_secprops( const char * );
LDAP_SLAPD_F (void) slap_sasl_secprops_unparse( struct berval * );
LDAP_SLAPD_F (int) slap_sasl_destroy(void);

LDAP_SLAPD_F (int) slap_sasl_open( Connection *c, int reopen );
LDAP_SLAPD_F (char **) slap_sasl_mechs( Connection *c );

LDAP_SLAPD_F (int) slap_sasl_external( Connection *c,
	slap_ssf_t ssf,	/* relative strength of external security */
	struct berval *authid );	/* asserted authenication id */

LDAP_SLAPD_F (int) slap_sasl_reset( Connection *c );
LDAP_SLAPD_F (int) slap_sasl_close( Connection *c );

LDAP_SLAPD_F (int) slap_sasl_bind LDAP_P(( Operation *op, SlapReply *rs ));

LDAP_SLAPD_F (int) slap_sasl_setpass(
	Operation       *op,
	SlapReply	*rs );

LDAP_SLAPD_F (int) slap_sasl_getdn( Connection *conn, Operation *op,
	struct berval *id, char *user_realm, struct berval *dn, int flags );

/*
 * saslauthz.c
 */
LDAP_SLAPD_F (int) slap_parse_user LDAP_P((
	struct berval *id, struct berval *user,
	struct berval *realm, struct berval *mech ));
LDAP_SLAPD_F (int) slap_sasl_matches LDAP_P((
	Operation *op, BerVarray rules,
	struct berval *assertDN, struct berval *authc ));
LDAP_SLAPD_F (void) slap_sasl2dn LDAP_P((
	Operation *op,
	struct berval *saslname,
	struct berval *dn,
	int flags ));
LDAP_SLAPD_F (int) slap_sasl_authorized LDAP_P((
	Operation *op,
	struct berval *authcid,
	struct berval *authzid ));
LDAP_SLAPD_F (int) slap_sasl_regexp_config LDAP_P((
	const char *match, const char *replace ));
LDAP_SLAPD_F (void) slap_sasl_regexp_unparse LDAP_P(( BerVarray *bva ));
LDAP_SLAPD_F (int) slap_sasl_setpolicy LDAP_P(( const char * ));
LDAP_SLAPD_F (const char *) slap_sasl_getpolicy LDAP_P(( void ));
#ifdef SLAP_AUTH_REWRITE
LDAP_SLAPD_F (int) slap_sasl_rewrite_config LDAP_P(( 
	const char *fname,
	int lineno,
	int argc, 
	char **argv ));
LDAP_SLAPD_F (void) slap_sasl_regexp_destroy LDAP_P(( void ));
#endif /* SLAP_AUTH_REWRITE */
LDAP_SLAPD_F (int) authzValidate LDAP_P((
	Syntax *syn, struct berval *in ));
#if 0
LDAP_SLAPD_F (int) authzMatch LDAP_P((
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue ));
#endif
LDAP_SLAPD_F (int) authzPretty LDAP_P((
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx ));
LDAP_SLAPD_F (int) authzNormalize LDAP_P((
	slap_mask_t usage,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx ));

/*
 * schema.c
 */
LDAP_SLAPD_F (int) schema_info LDAP_P(( Entry **entry, const char **text ));

/*
 * schema_check.c
 */
LDAP_SLAPD_F( int ) oc_check_allowed(
	AttributeType *type,
	ObjectClass **socs,
	ObjectClass *sc );

LDAP_SLAPD_F( int ) structural_class(
	BerVarray ocs,
	ObjectClass **sc,
	ObjectClass ***socs,
	const char **text,
	char *textbuf, size_t textlen, void *ctx );

LDAP_SLAPD_F( int ) entry_schema_check(
	Operation *op,
	Entry *e,
	Attribute *attrs,
	int manage,
	int add,
	Attribute **socp,
	const char** text,
	char *textbuf, size_t textlen );

LDAP_SLAPD_F( int ) mods_structural_class(
	Modifications *mods,
	struct berval *oc,
	const char** text,
	char *textbuf, size_t textlen, void *ctx );

/*
 * schema_init.c
 */
LDAP_SLAPD_V( int ) schema_init_done;
LDAP_SLAPD_F (int) slap_schema_init LDAP_P((void));
LDAP_SLAPD_F (void) schema_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) slap_hash64 LDAP_P((int));

LDAP_SLAPD_F( slap_mr_indexer_func ) octetStringIndexer;
LDAP_SLAPD_F( slap_mr_filter_func ) octetStringFilter;

LDAP_SLAPD_F( int ) numericoidValidate LDAP_P((
	Syntax *syntax,
        struct berval *in ));
LDAP_SLAPD_F( int ) numericStringValidate LDAP_P((
	Syntax *syntax,
	struct berval *in ));
LDAP_SLAPD_F( int ) octetStringMatch LDAP_P((
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue ));
LDAP_SLAPD_F( int ) octetStringOrderingMatch LDAP_P((
	int *matchp,
	slap_mask_t flags,
	Syntax *syntax,
	MatchingRule *mr,
	struct berval *value,
	void *assertedValue ));

/*
 * schema_prep.c
 */
LDAP_SLAPD_V( struct slap_internal_schema ) slap_schema;
LDAP_SLAPD_F (int) slap_schema_load LDAP_P((void));
LDAP_SLAPD_F (int) slap_schema_check LDAP_P((void));

/*
 * schemaparse.c
 */
LDAP_SLAPD_F( int ) slap_valid_descr( const char * );

LDAP_SLAPD_F (int) parse_cr LDAP_P((
	struct config_args_s *ca, ContentRule **scr ));
LDAP_SLAPD_F (int) parse_oc LDAP_P((
	struct config_args_s *ca, ObjectClass **soc, ObjectClass *prev ));
LDAP_SLAPD_F (int) parse_at LDAP_P((
	struct config_args_s *ca, AttributeType **sat, AttributeType *prev ));
LDAP_SLAPD_F (char *) scherr2str LDAP_P((int code)) LDAP_GCCATTR((const));
LDAP_SLAPD_F (int) dscompare LDAP_P(( const char *s1, const char *s2del,
	char delim ));
LDAP_SLAPD_F (int) parse_syn LDAP_P((
	struct config_args_s *ca, Syntax **sat, Syntax *prev ));

/*
 * sessionlog.c
 */
LDAP_SLAPD_F (int) slap_send_session_log LDAP_P((
					Operation *, Operation *, SlapReply *));
LDAP_SLAPD_F (int) slap_add_session_log LDAP_P((
					Operation *, Operation *, Entry * ));

/*
 * sl_malloc.c
 */
LDAP_SLAPD_F (void *) slap_sl_malloc LDAP_P((
	ber_len_t size, void *ctx ));
LDAP_SLAPD_F (void *) slap_sl_realloc LDAP_P((
	void *block, ber_len_t size, void *ctx ));
LDAP_SLAPD_F (void *) slap_sl_calloc LDAP_P((
	ber_len_t nelem, ber_len_t size, void *ctx ));
LDAP_SLAPD_F (void) slap_sl_free LDAP_P((
	void *, void *ctx ));

LDAP_SLAPD_V (BerMemoryFunctions) slap_sl_mfuncs;

LDAP_SLAPD_F (void) slap_sl_mem_init LDAP_P(( void ));
LDAP_SLAPD_F (void *) slap_sl_mem_create LDAP_P((
						ber_len_t size, int stack, void *ctx, int flag ));
LDAP_SLAPD_F (void) slap_sl_mem_detach LDAP_P(( void *ctx, void *memctx ));
LDAP_SLAPD_F (void) slap_sl_mem_destroy LDAP_P(( void *key, void *data ));
LDAP_SLAPD_F (void *) slap_sl_context LDAP_P(( void *ptr ));

/*
 * starttls.c
 */
LDAP_SLAPD_F (SLAP_EXTOP_MAIN_FN) starttls_extop;

/*
 * str2filter.c
 */
LDAP_SLAPD_F (Filter *) str2filter LDAP_P(( const char *str ));
LDAP_SLAPD_F (Filter *) str2filter_x LDAP_P(( Operation *op, const char *str ));

/*
 * syncrepl.c
 */

LDAP_SLAPD_F (int)  syncrepl_add_glue LDAP_P(( 
					Operation*, Entry* ));
LDAP_SLAPD_F (void) syncrepl_diff_entry LDAP_P((
	Operation *op, Attribute *old, Attribute *anew,
	Modifications **mods, Modifications **ml, int is_ctx ));
LDAP_SLAPD_F (void) syncinfo_free LDAP_P(( struct syncinfo_s *, int all ));

/* syntax.c */
LDAP_SLAPD_F (int) syn_is_sup LDAP_P((
	Syntax *syn,
	Syntax *sup ));
LDAP_SLAPD_F (Syntax *) syn_find LDAP_P((
	const char *synname ));
LDAP_SLAPD_F (Syntax *) syn_find_desc LDAP_P((
	const char *syndesc, int *slen ));
LDAP_SLAPD_F (int) syn_add LDAP_P((
	LDAPSyntax *syn,
	int user,
	slap_syntax_defs_rec *def,
	Syntax **ssyn,
	Syntax *prev,
	const char **err ));
LDAP_SLAPD_F (void) syn_destroy LDAP_P(( void ));

LDAP_SLAPD_F (int) register_syntax LDAP_P((
	slap_syntax_defs_rec *def ));

LDAP_SLAPD_F (int) syn_schema_info( Entry *e );

LDAP_SLAPD_F (int) syn_start LDAP_P(( Syntax **at ));
LDAP_SLAPD_F (int) syn_next LDAP_P(( Syntax **at ));
LDAP_SLAPD_F (void) syn_delete LDAP_P(( Syntax *at ));

LDAP_SLAPD_F (void) syn_unparse LDAP_P((
	BerVarray *bva, Syntax *start, Syntax *end, int system ));

/*
 * user.c
 */
#if defined(HAVE_PWD_H) && defined(HAVE_GRP_H)
LDAP_SLAPD_F (void) slap_init_user LDAP_P(( char *username, char *groupname ));
#endif

/*
 * value.c
 */
LDAP_SLAPD_F (int) asserted_value_validate_normalize LDAP_P((
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned usage,
	struct berval *in,
	struct berval *out,
	const char ** text,
	void *ctx ));

LDAP_SLAPD_F (int) value_match LDAP_P((
	int *match,
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned flags,
	struct berval *v1,
	void *v2,
	const char ** text ));
LDAP_SLAPD_F (int) value_find_ex LDAP_P((
	AttributeDescription *ad,
	unsigned flags,
	BerVarray values,
	struct berval *value,
	void *ctx ));

LDAP_SLAPD_F (int) ordered_value_add LDAP_P((
	Entry *e,
	AttributeDescription *ad,
	Attribute *a,
	BerVarray vals,
	BerVarray nvals ));

LDAP_SLAPD_F (int) ordered_value_validate LDAP_P((
	AttributeDescription *ad,
	struct berval *in,
	int mop ));

LDAP_SLAPD_F (int) ordered_value_pretty LDAP_P((
	AttributeDescription *ad,
	struct berval *val,
	struct berval *out,
	void *ctx ));

LDAP_SLAPD_F (int) ordered_value_normalize LDAP_P((
	slap_mask_t usage,
	AttributeDescription *ad,
	MatchingRule *mr,
	struct berval *val,
	struct berval *normalized,
	void *ctx ));

LDAP_SLAPD_F (int) ordered_value_match LDAP_P((
	int *match,
	AttributeDescription *ad,
	MatchingRule *mr,
	unsigned flags,
	struct berval *v1,
	struct berval *v2,
	const char ** text ));

LDAP_SLAPD_F (void) ordered_value_renumber LDAP_P((
	Attribute *a ));

LDAP_SLAPD_F (int) ordered_value_sort LDAP_P((
	Attribute *a,
	int do_renumber ));

LDAP_SLAPD_F (int) value_add LDAP_P((
	BerVarray *vals,
	BerVarray addvals ));
LDAP_SLAPD_F (int) value_add_one LDAP_P((
	BerVarray *vals,
	struct berval *addval ));

/* assumes (x) > (y) returns 1 if true, 0 otherwise */
#define SLAP_PTRCMP(x, y) ((x) < (y) ? -1 : (x) > (y))

#ifdef SLAP_ZONE_ALLOC
/*
 * zn_malloc.c
 */
LDAP_SLAPD_F (void *) slap_zn_malloc LDAP_P((ber_len_t, void *));
LDAP_SLAPD_F (void *) slap_zn_realloc LDAP_P((void *, ber_len_t, void *));
LDAP_SLAPD_F (void *) slap_zn_calloc LDAP_P((ber_len_t, ber_len_t, void *));
LDAP_SLAPD_F (void) slap_zn_free LDAP_P((void *, void *));

LDAP_SLAPD_F (void *) slap_zn_mem_create LDAP_P((
							ber_len_t, ber_len_t, ber_len_t, ber_len_t));
LDAP_SLAPD_F (void) slap_zn_mem_destroy LDAP_P((void *));
LDAP_SLAPD_F (int) slap_zn_validate LDAP_P((void *, void *, int));
LDAP_SLAPD_F (int) slap_zn_invalidate LDAP_P((void *, void *));
LDAP_SLAPD_F (int) slap_zh_rlock LDAP_P((void*));
LDAP_SLAPD_F (int) slap_zh_runlock LDAP_P((void*));
LDAP_SLAPD_F (int) slap_zh_wlock LDAP_P((void*));
LDAP_SLAPD_F (int) slap_zh_wunlock LDAP_P((void*));
LDAP_SLAPD_F (int) slap_zn_rlock LDAP_P((void*, void*));
LDAP_SLAPD_F (int) slap_zn_runlock LDAP_P((void*, void*));
LDAP_SLAPD_F (int) slap_zn_wlock LDAP_P((void*, void*));
LDAP_SLAPD_F (int) slap_zn_wunlock LDAP_P((void*, void*));
#endif

/*
 * Other...
 */
LDAP_SLAPD_V (unsigned int) index_substr_if_minlen;
LDAP_SLAPD_V (unsigned int) index_substr_if_maxlen;
LDAP_SLAPD_V (unsigned int) index_substr_any_len;
LDAP_SLAPD_V (unsigned int) index_substr_any_step;
LDAP_SLAPD_V (unsigned int) index_intlen;
/* all signed integers from strings of this size need more than intlen bytes */
/* i.e. log(10)*(index_intlen_strlen-2) > log(2)*(8*(index_intlen)-1) */
LDAP_SLAPD_V (unsigned int) index_intlen_strlen;
#define SLAP_INDEX_INTLEN_STRLEN(intlen) ((8*(intlen)-1) * 146/485 + 3)

LDAP_SLAPD_V (ber_len_t) sockbuf_max_incoming;
LDAP_SLAPD_V (ber_len_t) sockbuf_max_incoming_auth;
LDAP_SLAPD_V (int)		slap_conn_max_pending;
LDAP_SLAPD_V (int)		slap_conn_max_pending_auth;

LDAP_SLAPD_V (slap_mask_t)	global_allows;
LDAP_SLAPD_V (slap_mask_t)	global_disallows;

LDAP_SLAPD_V (BerVarray)	default_referral;
LDAP_SLAPD_V (const char) 	Versionstr[];

LDAP_SLAPD_V (int)		global_gentlehup;
LDAP_SLAPD_V (int)		global_idletimeout;
LDAP_SLAPD_V (int)		global_writetimeout;
LDAP_SLAPD_V (char *)	global_host;
LDAP_SLAPD_V (struct berval)	global_host_bv;
LDAP_SLAPD_V (char *)	global_realm;
LDAP_SLAPD_V (char *)	sasl_host;
LDAP_SLAPD_V (char *)	slap_sasl_auxprops;
#ifdef SLAP_AUXPROP_DONTUSECOPY
LDAP_SLAPD_V (int)		slap_dontUseCopy_ignore;
LDAP_SLAPD_V (BerVarray)	slap_dontUseCopy_propnames;
#endif /* SLAP_AUXPROP_DONTUSECOPY */
LDAP_SLAPD_V (char **)	default_passwd_hash;
LDAP_SLAPD_V (int)		lber_debug;
LDAP_SLAPD_V (int)		ldap_syslog;
LDAP_SLAPD_V (struct berval)	default_search_base;
LDAP_SLAPD_V (struct berval)	default_search_nbase;

LDAP_SLAPD_V (slap_counters_t)	slap_counters;

LDAP_SLAPD_V (char *)		slapd_pid_file;
LDAP_SLAPD_V (char *)		slapd_args_file;
LDAP_SLAPD_V (time_t)		starttime;

/* use time(3) -- no mutex */
#define slap_get_time()	time( NULL )

LDAP_SLAPD_V (ldap_pvt_thread_pool_t)	connection_pool;
LDAP_SLAPD_V (int)			connection_pool_max;
LDAP_SLAPD_V (int)			connection_pool_queues;
LDAP_SLAPD_V (int)			slap_tool_thread_max;

LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	entry2str_mutex;

LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	ad_index_mutex;
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	ad_undef_mutex;
LDAP_SLAPD_V (ldap_pvt_thread_mutex_t)	oc_undef_mutex;

LDAP_SLAPD_V (ber_socket_t)	dtblsize;

LDAP_SLAPD_V (int)		use_reverse_lookup;

/*
 * operations
 */
LDAP_SLAPD_F (int) do_abandon LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_add LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_bind LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_compare LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_delete LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_modify LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_modrdn LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_search LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_unbind LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) do_extended LDAP_P((Operation *op, SlapReply *rs));

/*
 * frontend operations
 */
LDAP_SLAPD_F (int) fe_op_abandon LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_add LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_bind LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_bind_success LDAP_P(( Operation *op, SlapReply *rs ));
LDAP_SLAPD_F (int) fe_op_compare LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_delete LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_modify LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_modrdn LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_op_search LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_aux_operational LDAP_P((Operation *op, SlapReply *rs));
#if 0
LDAP_SLAPD_F (int) fe_op_unbind LDAP_P((Operation *op, SlapReply *rs));
#endif
LDAP_SLAPD_F (int) fe_extended LDAP_P((Operation *op, SlapReply *rs));
LDAP_SLAPD_F (int) fe_acl_group LDAP_P((
	Operation *op,
	Entry	*target,
	struct berval *gr_ndn,
	struct berval *op_ndn,
	ObjectClass *group_oc,
	AttributeDescription *group_at ));
LDAP_SLAPD_F (int) fe_acl_attribute LDAP_P((
	Operation *op,
	Entry	*target,
	struct berval	*edn,
	AttributeDescription *entry_at,
	BerVarray *vals,
	slap_access_t access ));
LDAP_SLAPD_F (int) fe_access_allowed LDAP_P((
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp ));

/* NOTE: this macro assumes that bv has been allocated
 * by ber_* malloc functions or is { 0L, NULL } */
#ifdef USE_MP_BIGNUM
# define UI2BVX(bv,ui,ctx) \
	do { \
		char		*val; \
		ber_len_t	len; \
		val = BN_bn2dec(ui); \
		if (val) { \
			len = strlen(val); \
			if ( len > (bv)->bv_len ) { \
				(bv)->bv_val = ber_memrealloc_x( (bv)->bv_val, len + 1, (ctx) ); \
			} \
			AC_MEMCPY((bv)->bv_val, val, len + 1); \
			(bv)->bv_len = len; \
			OPENSSL_free(val); \
		} else { \
			ber_memfree_x( (bv)->bv_val, (ctx) ); \
			BER_BVZERO( (bv) ); \
		} \
	} while ( 0 )

#elif defined( USE_MP_GMP )
/* NOTE: according to the documentation, the result 
 * of mpz_sizeinbase() can exceed the length of the
 * string representation of the number by 1
 */
# define UI2BVX(bv,ui,ctx) \
	do { \
		ber_len_t	len = mpz_sizeinbase( (ui), 10 ); \
		if ( len > (bv)->bv_len ) { \
			(bv)->bv_val = ber_memrealloc_x( (bv)->bv_val, len + 1, (ctx) ); \
		} \
		(void)mpz_get_str( (bv)->bv_val, 10, (ui) ); \
		if ( (bv)->bv_val[ len - 1 ] == '\0' ) { \
			len--; \
		} \
		(bv)->bv_len = len; \
	} while ( 0 )

#else
# ifdef USE_MP_LONG_LONG
#  define UI2BV_FORMAT	"%llu"
# elif defined USE_MP_LONG
#  define UI2BV_FORMAT	"%lu"
# elif defined HAVE_LONG_LONG
#  define UI2BV_FORMAT	"%llu"
# else
#  define UI2BV_FORMAT	"%lu"
# endif

# define UI2BVX(bv,ui,ctx) \
	do { \
		char		buf[LDAP_PVT_INTTYPE_CHARS(long)]; \
		ber_len_t	len; \
		len = snprintf( buf, sizeof( buf ), UI2BV_FORMAT, (ui) ); \
		if ( len > (bv)->bv_len ) { \
			(bv)->bv_val = ber_memrealloc_x( (bv)->bv_val, len + 1, (ctx) ); \
		} \
		(bv)->bv_len = len; \
		AC_MEMCPY( (bv)->bv_val, buf, len + 1 ); \
	} while ( 0 )
#endif

#define UI2BV(bv,ui)	UI2BVX(bv,ui,NULL)

LDAP_END_DECL

#endif /* PROTO_SLAP_H */
