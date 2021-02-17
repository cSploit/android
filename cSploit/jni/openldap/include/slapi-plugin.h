/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002,2003 IBM Corporation.
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

/*
 * This header is used in development of SLAPI plugins for
 * OpenLDAP slapd(8) and other directory servers supporting
 * this interface.  Your portability mileage may vary.
 */

#ifndef _SLAPI_PLUGIN_H
#define _SLAPI_PLUGIN_H

#include <ldap.h>

typedef struct slapi_pblock		Slapi_PBlock;
typedef struct slapi_entry		Slapi_Entry;
typedef struct slapi_attr		Slapi_Attr;
typedef struct slapi_value		Slapi_Value;
typedef struct slapi_valueset		Slapi_ValueSet;
typedef struct slapi_filter		Slapi_Filter;
typedef struct BackendDB		Slapi_Backend;
typedef struct Operation		Slapi_Operation;
typedef struct Connection		Slapi_Connection;
typedef struct slapi_dn			Slapi_DN;
typedef struct slapi_rdn		Slapi_RDN;
typedef struct slapi_mod		Slapi_Mod;
typedef struct slapi_mods		Slapi_Mods;
typedef struct slapi_componentid	Slapi_ComponentId;

#define SLAPI_ATTR_UNIQUEID	"entryUUID"
#define SLAPI_ATTR_OBJECTCLASS	"objectClass"

/* pblock routines */
int slapi_pblock_get( Slapi_PBlock *pb, int arg, void *value );
int slapi_pblock_set( Slapi_PBlock *pb, int arg, void *value );
Slapi_PBlock *slapi_pblock_new( void );
void slapi_pblock_destroy( Slapi_PBlock *pb );

/* entry/attr/dn routines */
Slapi_Entry *slapi_str2entry( char *s, int flags );
#define SLAPI_STR2ENTRY_REMOVEDUPVALS	1
#define SLAPI_STR2ENTRY_ADDRDNVALS	2
#define SLAPI_STR2ENTRY_BIGENTRY	4
#define SLAPI_STR2ENTRY_TOMBSTONE_CHECK	8
#define SLAPI_STR2ENTRY_IGNORE_STATE	16
#define SLAPI_STR2ENTRY_INCLUDE_VERSION_STR	32
#define SLAPI_STR2ENTRY_EXPAND_OBJECTCLASSES	64
#define SLAPI_STR2ENTRY_NOT_WELL_FORMED_LDIF	128
char *slapi_entry2str( Slapi_Entry *e, int *len );
char *slapi_entry_get_dn( Slapi_Entry *e );
int slapi_x_entry_get_id( Slapi_Entry *e );
void slapi_entry_set_dn( Slapi_Entry *e, char *dn );
Slapi_Entry *slapi_entry_dup( Slapi_Entry *e );
int slapi_entry_attr_delete( Slapi_Entry *e, char *type );
Slapi_Entry *slapi_entry_alloc();
void slapi_entry_free( Slapi_Entry *e );
int slapi_entry_attr_merge( Slapi_Entry *e, char *type, struct berval **vals );
int slapi_entry_attr_find( Slapi_Entry *e, char *type, Slapi_Attr **attr );
char *slapi_entry_attr_get_charptr( const Slapi_Entry *e, const char *type );
int slapi_entry_attr_get_int( const Slapi_Entry *e, const char *type );
long slapi_entry_attr_get_long( const Slapi_Entry *e, const char *type );
unsigned int slapi_entry_attr_get_uint( const Slapi_Entry *e, const char *type );
unsigned long slapi_entry_attr_get_ulong( const Slapi_Entry *e, const char *type );
int slapi_attr_get_values( Slapi_Attr *attr, struct berval ***vals );
char *slapi_dn_normalize( char *dn );
char *slapi_dn_normalize_case( char *dn );
int slapi_dn_issuffix( char *dn, char *suffix );
char *slapi_dn_beparent( Slapi_PBlock *pb, const char *dn );
int slapi_dn_isbesuffix( Slapi_PBlock *pb, char *dn );
char *slapi_dn_parent( const char *dn );
int slapi_dn_isparent( const char *parentdn, const char *childdn );
char *slapi_dn_ignore_case( char *dn );
int slapi_rdn2typeval( char *rdn, char **type, struct berval *bv );
char *slapi_dn_plus_rdn(const char *dn, const char *rdn);

/* DS 5.x SLAPI */
int slapi_access_allowed( Slapi_PBlock *pb, Slapi_Entry *e, char *attr, struct berval *val, int access );
int slapi_acl_check_mods( Slapi_PBlock *pb, Slapi_Entry *e, LDAPMod **mods, char **errbuf );
Slapi_Attr *slapi_attr_new( void );
Slapi_Attr *slapi_attr_init( Slapi_Attr *a, const char *type );
void slapi_attr_free( Slapi_Attr **a );
Slapi_Attr *slapi_attr_dup( const Slapi_Attr *attr );
int slapi_attr_add_value( Slapi_Attr *a, const Slapi_Value *v );
int slapi_attr_type2plugin( const char *type, void **pi );
int slapi_attr_get_type( const Slapi_Attr *attr, char **type );
int slapi_attr_get_oid_copy( const Slapi_Attr *attr, char **oidp );
int slapi_attr_get_flags( const Slapi_Attr *attr, unsigned long *flags );
int slapi_attr_flag_is_set( const Slapi_Attr *attr, unsigned long flag );
int slapi_attr_value_cmp( const Slapi_Attr *attr, const struct berval *v1, const struct berval *v2 );
int slapi_attr_value_find( const Slapi_Attr *a, struct berval *v );
#define SLAPI_TYPE_CMP_EXACT	0
#define SLAPI_TYPE_CMP_BASE	1
#define SLAPI_TYPE_CMP_SUBTYPE	2
int slapi_attr_type_cmp( const char *t1, const char *t2, int opt );
int slapi_attr_types_equivalent( const char *t1, const char *t2 );
int slapi_attr_first_value( Slapi_Attr *a, Slapi_Value **v );
int slapi_attr_next_value( Slapi_Attr *a, int hint, Slapi_Value **v );
int slapi_attr_get_numvalues( const Slapi_Attr *a, int *numValues );
int slapi_attr_get_valueset( const Slapi_Attr *a, Slapi_ValueSet **vs );
int slapi_attr_get_bervals_copy( Slapi_Attr *a, struct berval ***vals );
int slapi_entry_attr_hasvalue( Slapi_Entry *e, const char *type, const char *value );
int slapi_entry_attr_merge_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
void slapi_entry_attr_set_charptr(Slapi_Entry* e, const char *type, const char *value);
void slapi_entry_attr_set_int( Slapi_Entry* e, const char *type, int l);
void slapi_entry_attr_set_uint( Slapi_Entry* e, const char *type, unsigned int l);
void slapi_entry_attr_set_long(Slapi_Entry* e, const char *type, long l);
void slapi_entry_attr_set_ulong(Slapi_Entry* e, const char *type, unsigned long l);
int slapi_entry_has_children(const Slapi_Entry *e);
size_t slapi_entry_size(Slapi_Entry *e);
int slapi_is_rootdse( const char *dn );
int slapi_entry_attr_merge_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
int slapi_entry_add_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
int slapi_entry_add_valueset(Slapi_Entry *e, const char *type, Slapi_ValueSet *vs);
int slapi_entry_delete_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
int slapi_entry_merge_values_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
int slapi_entry_attr_replace_sv( Slapi_Entry *e, const char *type, Slapi_Value **vals );
int slapi_entry_add_value(Slapi_Entry *e, const char *type, const Slapi_Value *value);
int slapi_entry_add_string(Slapi_Entry *e, const char *type, const char *value);
int slapi_entry_delete_string(Slapi_Entry *e, const char *type, const char *value);
int slapi_entry_first_attr( const Slapi_Entry *e, Slapi_Attr **attr );
int slapi_entry_next_attr( const Slapi_Entry *e, Slapi_Attr *prevattr, Slapi_Attr **attr );
const char *slapi_entry_get_uniqueid( const Slapi_Entry *e );
void slapi_entry_set_uniqueid( Slapi_Entry *e, char *uniqueid );
int slapi_entry_schema_check( Slapi_PBlock *pb, Slapi_Entry *e );
int slapi_entry_rdn_values_present( const Slapi_Entry *e );
int slapi_entry_add_rdn_values( Slapi_Entry *e );
char *slapi_attr_syntax_normalize( const char *s );

Slapi_Value *slapi_value_new( void );
Slapi_Value *slapi_value_new_berval(const struct berval *bval);
Slapi_Value *slapi_value_new_value(const Slapi_Value *v);
Slapi_Value *slapi_value_new_string(const char *s);
Slapi_Value *slapi_value_init(Slapi_Value *v);
Slapi_Value *slapi_value_init_berval(Slapi_Value *v, struct berval *bval);
Slapi_Value *slapi_value_init_string(Slapi_Value *v, const char *s);
Slapi_Value *slapi_value_dup(const Slapi_Value *v);
void slapi_value_free(Slapi_Value **value);
const struct berval *slapi_value_get_berval( const Slapi_Value *value );
Slapi_Value *slapi_value_set_berval( Slapi_Value *value, const struct berval *bval );
Slapi_Value *slapi_value_set_value( Slapi_Value *value, const Slapi_Value *vfrom);
Slapi_Value *slapi_value_set( Slapi_Value *value, void *val, unsigned long len);
int slapi_value_set_string(Slapi_Value *value, const char *strVal);
int slapi_value_set_int(Slapi_Value *value, int intVal);
const char*slapi_value_get_string(const Slapi_Value *value);
int slapi_value_get_int(const Slapi_Value *value); 
unsigned int slapi_value_get_uint(const Slapi_Value *value); 
long slapi_value_get_long(const Slapi_Value *value); 
unsigned long slapi_value_get_ulong(const Slapi_Value *value); 
size_t slapi_value_get_length(const Slapi_Value *value);
int slapi_value_compare(const Slapi_Attr *a, const Slapi_Value *v1, const Slapi_Value *v2);

Slapi_ValueSet *slapi_valueset_new( void );
void slapi_valueset_free(Slapi_ValueSet *vs);
void slapi_valueset_init(Slapi_ValueSet *vs);
void slapi_valueset_done(Slapi_ValueSet *vs);
void slapi_valueset_add_value(Slapi_ValueSet *vs, const Slapi_Value *addval);
int slapi_valueset_first_value( Slapi_ValueSet *vs, Slapi_Value **v );
int slapi_valueset_next_value( Slapi_ValueSet *vs, int index, Slapi_Value **v);
int slapi_valueset_count( const Slapi_ValueSet *vs);
void slapi_valueset_set_valueset(Slapi_ValueSet *vs1, const Slapi_ValueSet *vs2);

/* DNs */
Slapi_DN *slapi_sdn_new( void );
Slapi_DN *slapi_sdn_new_dn_byval( const char *dn );
Slapi_DN *slapi_sdn_new_ndn_byval( const char *ndn );
Slapi_DN *slapi_sdn_new_dn_byref( const char *dn );
Slapi_DN *slapi_sdn_new_ndn_byref( const char *ndn );
Slapi_DN *slapi_sdn_new_dn_passin( const char *dn );
Slapi_DN *slapi_sdn_set_dn_byval( Slapi_DN *sdn, const char *dn );
Slapi_DN *slapi_sdn_set_dn_byref( Slapi_DN *sdn, const char *dn );
Slapi_DN *slapi_sdn_set_dn_passin( Slapi_DN *sdn, const char *dn );
Slapi_DN *slapi_sdn_set_ndn_byval( Slapi_DN *sdn, const char *ndn );
Slapi_DN *slapi_sdn_set_ndn_byref( Slapi_DN *sdn, const char *ndn );
void slapi_sdn_done( Slapi_DN *sdn );
void slapi_sdn_free( Slapi_DN **sdn );
const char * slapi_sdn_get_dn( const Slapi_DN *sdn );
const char * slapi_sdn_get_ndn( const Slapi_DN *sdn );
void slapi_sdn_get_parent( const Slapi_DN *sdn,Slapi_DN *sdn_parent );
void slapi_sdn_get_backend_parent( const Slapi_DN *sdn, Slapi_DN *sdn_parent, const Slapi_Backend *backend );
Slapi_DN * slapi_sdn_dup( const Slapi_DN *sdn );
void slapi_sdn_copy( const Slapi_DN *from, Slapi_DN *to );
int slapi_sdn_compare( const Slapi_DN *sdn1, const Slapi_DN *sdn2 );
int slapi_sdn_isempty( const Slapi_DN *sdn );
int slapi_sdn_issuffix(const Slapi_DN *sdn, const Slapi_DN *suffixsdn );
int slapi_sdn_isparent( const Slapi_DN *parent, const Slapi_DN *child );
int slapi_sdn_isgrandparent( const Slapi_DN *parent, const Slapi_DN *child );
int slapi_sdn_get_ndn_len( const Slapi_DN *sdn );
int slapi_sdn_scope_test( const Slapi_DN *dn, const Slapi_DN *base, int scope );
void slapi_sdn_get_rdn( const Slapi_DN *sdn,Slapi_RDN *rdn );
Slapi_DN *slapi_sdn_set_rdn( Slapi_DN *sdn, const Slapi_RDN *rdn );
Slapi_DN *slapi_sdn_set_parent( Slapi_DN *sdn, const Slapi_DN *parentdn );
int slapi_sdn_is_rdn_component( const Slapi_DN *rdn, const Slapi_Attr *a, const Slapi_Value *v );
char * slapi_moddn_get_newdn( Slapi_DN *dn_olddn, char *newrdn, char *newsuperiordn );

/* RDNs */
Slapi_RDN *slapi_rdn_new( void );
Slapi_RDN *slapi_rdn_new_dn( const char *dn );
Slapi_RDN *slapi_rdn_new_sdn( const Slapi_DN *sdn );
Slapi_RDN *slapi_rdn_new_rdn( const Slapi_RDN *fromrdn ); 
void slapi_rdn_init( Slapi_RDN *rdn );
void slapi_rdn_init_dn( Slapi_RDN *rdn, const char *dn );
void slapi_rdn_init_sdn( Slapi_RDN *rdn, const Slapi_DN *sdn );
void slapi_rdn_init_rdn( Slapi_RDN *rdn, const Slapi_RDN *fromrdn ); 
void slapi_rdn_set_dn( Slapi_RDN *rdn, const char *dn );
void slapi_rdn_set_sdn( Slapi_RDN *rdn, const Slapi_DN *sdn );
void slapi_rdn_set_rdn( Slapi_RDN *rdn, const Slapi_RDN *fromrdn );
void slapi_rdn_free( Slapi_RDN **rdn );
void slapi_rdn_done( Slapi_RDN *rdn );
int slapi_rdn_get_first( Slapi_RDN *rdn, char **type, char **value );
int slapi_rdn_get_next( Slapi_RDN *rdn, int index, char **type, char **value );
int slapi_rdn_get_index( Slapi_RDN *rdn, const char *type, const char *value, size_t length );
int slapi_rdn_get_index_attr( Slapi_RDN *rdn, const char *type, char **value );
int slapi_rdn_contains( Slapi_RDN *rdn, const char *type, const char *value,size_t length );
int slapi_rdn_contains_attr( Slapi_RDN *rdn, const char *type, char **value );
int slapi_rdn_add( Slapi_RDN *rdn, const char *type, const char *value );
int slapi_rdn_remove_index( Slapi_RDN *rdn, int atindex );
int slapi_rdn_remove( Slapi_RDN *rdn, const char *type, const char *value, size_t length );
int slapi_rdn_remove_attr( Slapi_RDN *rdn, const char *type );
int slapi_rdn_isempty( const Slapi_RDN *rdn );
int slapi_rdn_get_num_components( Slapi_RDN *rdn );
int slapi_rdn_compare( Slapi_RDN *rdn1, Slapi_RDN *rdn2 );
const char *slapi_rdn_get_rdn( const Slapi_RDN *rdn );
const char *slapi_rdn_get_nrdn( const Slapi_RDN *rdn );
Slapi_DN *slapi_sdn_add_rdn( Slapi_DN *sdn, const Slapi_RDN *rdn );

/* locks and synchronization */
typedef struct slapi_mutex	Slapi_Mutex;
typedef struct slapi_condvar	Slapi_CondVar;
Slapi_Mutex *slapi_new_mutex( void );
void slapi_destroy_mutex( Slapi_Mutex *mutex );
void slapi_lock_mutex( Slapi_Mutex *mutex );
int slapi_unlock_mutex( Slapi_Mutex *mutex );
Slapi_CondVar *slapi_new_condvar( Slapi_Mutex *mutex );
void slapi_destroy_condvar( Slapi_CondVar *cvar );
int slapi_wait_condvar( Slapi_CondVar *cvar, struct timeval *timeout );
int slapi_notify_condvar( Slapi_CondVar *cvar, int notify_all );

/* thread-safe LDAP connections */
LDAP *slapi_ldap_init( char *ldaphost, int ldapport, int secure, int shared );
void slapi_ldap_unbind( LDAP *ld );

char *slapi_ch_malloc( unsigned long size );
void slapi_ch_free( void **ptr );
void slapi_ch_free_string( char **ptr );
char *slapi_ch_calloc( unsigned long nelem, unsigned long size );
char *slapi_ch_realloc( char *block, unsigned long size );
char *slapi_ch_strdup( const char *s );
void slapi_ch_array_free( char **arrayp );
struct berval *slapi_ch_bvdup(const struct berval *v);
struct berval **slapi_ch_bvecdup(const struct berval **v);

/* LDAP V3 routines */
int slapi_control_present( LDAPControl **controls, char *oid,
	struct berval **val, int *iscritical);
void slapi_register_supported_control(char *controloid,
	unsigned long controlops);
#define SLAPI_OPERATION_BIND            0x00000001L
#define SLAPI_OPERATION_UNBIND          0x00000002L
#define SLAPI_OPERATION_SEARCH          0x00000004L
#define SLAPI_OPERATION_MODIFY          0x00000008L
#define SLAPI_OPERATION_ADD             0x00000010L
#define SLAPI_OPERATION_DELETE          0x00000020L
#define SLAPI_OPERATION_MODDN           0x00000040L
#define SLAPI_OPERATION_MODRDN          SLAPI_OPERATION_MODDN
#define SLAPI_OPERATION_COMPARE         0x00000080L
#define SLAPI_OPERATION_ABANDON         0x00000100L
#define SLAPI_OPERATION_EXTENDED        0x00000200L
#define SLAPI_OPERATION_ANY             0xFFFFFFFFL
#define SLAPI_OPERATION_NONE            0x00000000L
int slapi_get_supported_controls(char ***ctrloidsp, unsigned long **ctrlopsp);
LDAPControl *slapi_dup_control(LDAPControl *ctrl);
void slapi_register_supported_saslmechanism(char *mechanism);
char **slapi_get_supported_saslmechanisms();
char **slapi_get_supported_extended_ops(void);

/* operation */
int slapi_op_abandoned( Slapi_PBlock *pb );
unsigned long slapi_op_get_type(Slapi_Operation * op);
void slapi_operation_set_flag(Slapi_Operation *op, unsigned long flag);
void slapi_operation_clear_flag(Slapi_Operation *op, unsigned long flag);
int slapi_operation_is_flag_set(Slapi_Operation *op, unsigned long flag);
char *slapi_op_type_to_string(unsigned long type);

/* send ldap result back */
void slapi_send_ldap_result( Slapi_PBlock *pb, int err, char *matched,
	char *text, int nentries, struct berval **urls );
int slapi_send_ldap_search_entry( Slapi_PBlock *pb, Slapi_Entry *e,
	LDAPControl **ectrls, char **attrs, int attrsonly );
int slapi_send_ldap_search_reference( Slapi_PBlock *pb, Slapi_Entry *e,
	struct berval **urls, LDAPControl **ectrls, struct berval **v2refs );

/* filter routines */
Slapi_Filter *slapi_str2filter( char *str );
Slapi_Filter *slapi_filter_dup( Slapi_Filter *f );
void slapi_filter_free( Slapi_Filter *f, int recurse );
int slapi_filter_get_choice( Slapi_Filter *f);
int slapi_filter_get_ava( Slapi_Filter *f, char **type, struct berval **bval );
Slapi_Filter *slapi_filter_list_first( Slapi_Filter *f );
Slapi_Filter *slapi_filter_list_next( Slapi_Filter *f, Slapi_Filter *fprev );
int slapi_filter_get_attribute_type( Slapi_Filter *f, char **type ); 
int slapi_x_filter_set_attribute_type( Slapi_Filter *f, const char *type );
int slapi_filter_get_subfilt( Slapi_Filter *f, char **type, char **initial,
	char ***any, char **final );
Slapi_Filter *slapi_filter_join( int ftype, Slapi_Filter *f1, Slapi_Filter *f2);
int slapi_x_filter_append( int choice, Slapi_Filter **pContainingFilter,
	Slapi_Filter **pNextFilter, Slapi_Filter *filterToAppend );
int slapi_filter_test( Slapi_PBlock *pb, Slapi_Entry *e, Slapi_Filter *f,
	int verify_access );
int slapi_filter_test_simple( Slapi_Entry *e, Slapi_Filter *f );
typedef int (*FILTER_APPLY_FN)( Slapi_Filter *f, void *arg );
int slapi_filter_apply( Slapi_Filter *f, FILTER_APPLY_FN fn, void *arg, int *error_code );
#define SLAPI_FILTER_SCAN_STOP			-1 /* set by callback */
#define SLAPI_FILTER_SCAN_ERROR			-2 /* set by callback */
#define SLAPI_FILTER_SCAN_NOMORE		0 /* set by callback */
#define SLAPI_FILTER_SCAN_CONTINUE		1 /* set by callback */
#define SLAPI_FILTER_UNKNOWN_FILTER_TYPE	2 /* set by slapi_filter_apply() */

/* internal add/delete/search/modify routines */
Slapi_PBlock *slapi_search_internal( char *base, int scope, char *filter, 
	LDAPControl **controls, char **attrs, int attrsonly );
Slapi_PBlock *slapi_modify_internal( char *dn, LDAPMod **mods,
        LDAPControl **controls, int log_change );
Slapi_PBlock *slapi_add_internal( char * dn, LDAPMod **attrs,
	LDAPControl **controls, int log_changes );
Slapi_PBlock *slapi_add_entry_internal( Slapi_Entry * e,
	LDAPControl **controls, int log_change );
Slapi_PBlock *slapi_delete_internal( char * dn,  LDAPControl **controls,
	int log_change );
Slapi_PBlock *slapi_modrdn_internal( char * olddn, char * newrdn,
	int deloldrdn, LDAPControl **controls,
	int log_change );
Slapi_PBlock *slapi_rename_internal( const char * olddn, const char *newrdn,
	const char *newsuperior, int delolrdn,
	LDAPControl **controls, int log_change );
void slapi_free_search_results_internal(Slapi_PBlock *pb);

/* new internal add/delete/search/modify routines */
typedef void (*plugin_result_callback)( int rc, void *callback_data );
typedef int (*plugin_referral_entry_callback)( char * referral,
	void *callback_data );
typedef int (*plugin_search_entry_callback)( Slapi_Entry *e,
	void *callback_data );
void slapi_free_search_results_internal( Slapi_PBlock *pb );

#define SLAPI_OP_FLAG_NEVER_CHAIN	0x0800

int slapi_search_internal_pb( Slapi_PBlock *pb );
int slapi_search_internal_callback_pb( Slapi_PBlock *pb, void *callback_data,
	plugin_result_callback prc, plugin_search_entry_callback psec,
	plugin_referral_entry_callback prec );
int slapi_add_internal_pb( Slapi_PBlock *pb );
int slapi_modify_internal_pb( Slapi_PBlock *pb );
int slapi_modrdn_internal_pb( Slapi_PBlock *pb );
int slapi_delete_internal_pb( Slapi_PBlock *pb );

int slapi_seq_internal_callback_pb(Slapi_PBlock *pb, void *callback_data,
        plugin_result_callback res_callback,
        plugin_search_entry_callback srch_callback,
        plugin_referral_entry_callback ref_callback);

void slapi_search_internal_set_pb( Slapi_PBlock *pb, const char *base,
	int scope, const char *filter, char **attrs, int attrsonly,
	LDAPControl **controls, const char *uniqueid,
	Slapi_ComponentId *plugin_identity, int operation_flags );
void slapi_add_entry_internal_set_pb( Slapi_PBlock *pb, Slapi_Entry *e,
	LDAPControl **controls, Slapi_ComponentId *plugin_identity,
	int operation_flags );
int slapi_add_internal_set_pb( Slapi_PBlock *pb, const char *dn,
	LDAPMod **attrs, LDAPControl **controls,
	Slapi_ComponentId *plugin_identity, int operation_flags );
void slapi_modify_internal_set_pb( Slapi_PBlock *pb, const char *dn,
	LDAPMod **mods, LDAPControl **controls, const char *uniqueid,
	Slapi_ComponentId *plugin_identity, int operation_flags );
void slapi_rename_internal_set_pb( Slapi_PBlock *pb, const char *olddn,
	const char *newrdn, const char *newsuperior, int deloldrdn,
	LDAPControl **controls, const char *uniqueid,
	Slapi_ComponentId *plugin_identity, int operation_flags );
void slapi_delete_internal_set_pb( Slapi_PBlock *pb, const char *dn,
	LDAPControl **controls, const char *uniqueid,
	Slapi_ComponentId *plugin_identity, int operation_flags );
void slapi_seq_internal_set_pb( Slapi_PBlock *pb, char *ibase, int type,
	char *attrname, char *val, char **attrs, int attrsonly,
	LDAPControl **controls, Slapi_ComponentId *plugin_identity,
	int operation_flags );

/* connection related routines */
int slapi_is_connection_ssl(Slapi_PBlock *pPB, int *isSSL);
int slapi_get_client_port(Slapi_PBlock *pPB, int *fromPort);
int slapi_get_client_ip(Slapi_PBlock *pb, char **clientIP);
void slapi_free_client_ip(char **clientIP);

/* computed attributes */
typedef struct _computed_attr_context computed_attr_context;
typedef int (*slapi_compute_output_t)(computed_attr_context *c, Slapi_Attr *a, Slapi_Entry *e);
typedef int (*slapi_compute_callback_t)(computed_attr_context *c, char *type, Slapi_Entry *e, slapi_compute_output_t outputfn);
typedef int (*slapi_search_rewrite_callback_t)(Slapi_PBlock *pb);
int slapi_compute_add_evaluator(slapi_compute_callback_t function);
int slapi_compute_add_search_rewriter(slapi_search_rewrite_callback_t function);
int compute_rewrite_search_filter(Slapi_PBlock *pb);
int compute_evaluator(computed_attr_context *c, char *type, Slapi_Entry *e, slapi_compute_output_t outputfn);
int slapi_x_compute_get_pblock(computed_attr_context *c, Slapi_PBlock **pb);

/* backend routines */
void slapi_be_set_readonly( Slapi_Backend *be, int readonly );
int slapi_be_get_readonly( Slapi_Backend *be );
const char *slapi_x_be_get_updatedn( Slapi_Backend *be );
Slapi_Backend *slapi_be_select( const Slapi_DN *sdn );

/* ACL plugins; only SLAPI_PLUGIN_ACL_ALLOW_ACCESS supported now */
typedef int (*slapi_acl_callback_t)(Slapi_PBlock *pb,
	Slapi_Entry *e,
	const char *attr,
	struct berval *berval,
	int access,
	void *state);

/* object extensions */
typedef void *(*slapi_extension_constructor_fnptr)(void *object, void *parent);

typedef void (*slapi_extension_destructor_fnptr)(void *extension,
	void *object, void *parent);

int slapi_register_object_extension( const char *pluginname,
	const char *objectname, slapi_extension_constructor_fnptr constructor,
	slapi_extension_destructor_fnptr destructor, int *objecttype,
	int *extensionhandle);

#define SLAPI_EXT_CONNECTION    "Connection"
#define SLAPI_EXT_OPERATION     "Operation"
#define SLAPI_EXT_ENTRY         "Entry"
#define SLAPI_EXT_MTNODE        "Mapping Tree Node"

void *slapi_get_object_extension(int objecttype, void *object,
	int extensionhandle);
void slapi_set_object_extension(int objecttype, void *object,
	int extensionhandle, void *extension);

int slapi_x_backend_get_flags( const Slapi_Backend *be, unsigned long *flags );

/* parameters currently supported */

/*
 * Attribute flags returned by slapi_attr_get_flags()
 */
#define SLAPI_ATTR_FLAG_SINGLE		0x0001
#define SLAPI_ATTR_FLAG_OPATTR		0x0002
#define SLAPI_ATTR_FLAG_READONLY	0x0004
#define SLAPI_ATTR_FLAG_STD_ATTR	SLAPI_ATTR_FLAG_READONLY
#define SLAPI_ATTR_FLAG_OBSOLETE	0x0040
#define SLAPI_ATTR_FLAG_COLLECTIVE	0x0080
#define SLAPI_ATTR_FLAG_NOUSERMOD	0x0100

/*
 * Backend flags returned by slapi_x_backend_get_flags()
 */
#define SLAPI_BACKEND_FLAG_NOLASTMOD		0x0001U
#define SLAPI_BACKEND_FLAG_NO_SCHEMA_CHECK	0x0002U
#define SLAPI_BACKEND_FLAG_GLUE_INSTANCE	0x0010U	/* a glue backend */
#define SLAPI_BACKEND_FLAG_GLUE_SUBORDINATE	0x0020U	/* child of a glue hierarchy */
#define SLAPI_BACKEND_FLAG_GLUE_LINKED		0x0040U	/* child is connected to parent */
#define SLAPI_BACKEND_FLAG_OVERLAY		0x0080U	/* this db struct is an overlay */
#define SLAPI_BACKEND_FLAG_GLOBAL_OVERLAY	0x0100U	/* this db struct is a global overlay */
#define SLAPI_BACKEND_FLAG_SHADOW		0x8000U /* a shadow */
#define SLAPI_BACKEND_FLAG_SYNC_SHADOW		0x1000U /* a sync shadow */
#define SLAPI_BACKEND_FLAG_SLURP_SHADOW		0x2000U /* a slurp shadow */

/*
 * ACL levels
 */
#define SLAPI_ACL_COMPARE       0x01
#define SLAPI_ACL_SEARCH        0x02
#define SLAPI_ACL_READ          0x04
#define SLAPI_ACL_WRITE         0x08
#define SLAPI_ACL_DELETE        0x10    
#define SLAPI_ACL_ADD           0x20
#define SLAPI_ACL_SELF          0x40
#define SLAPI_ACL_PROXY         0x80
#define SLAPI_ACL_ALL           0x7f

/* plugin types supported */

#define SLAPI_PLUGIN_DATABASE           1
#define SLAPI_PLUGIN_EXTENDEDOP         2
#define SLAPI_PLUGIN_PREOPERATION       3
#define SLAPI_PLUGIN_POSTOPERATION      4
#define SLAPI_PLUGIN_MATCHINGRULE       5
#define SLAPI_PLUGIN_SYNTAX             6
#define SLAPI_PLUGIN_AUDIT              7   

/* misc params */

#define SLAPI_BACKEND				130
#define SLAPI_CONNECTION			131
#define SLAPI_OPERATION				132
#define SLAPI_REQUESTOR_ISROOT			133
#define SLAPI_BE_MONITORDN			134
#define SLAPI_BE_TYPE           		135
#define SLAPI_BE_READONLY       		136
#define SLAPI_BE_LASTMOD       			137
#define SLAPI_CONN_ID        			139

/* operation params */
#define SLAPI_OPINITIATED_TIME			140
#define SLAPI_REQUESTOR_DN			141
#define SLAPI_IS_REPLICATED_OPERATION		142
#define SLAPI_REQUESTOR_ISUPDATEDN		SLAPI_IS_REPLICATED_OPERATION

/* connection  structure params*/
#define SLAPI_CONN_DN        			143
#define SLAPI_CONN_AUTHTYPE    			144
#define SLAPI_CONN_CLIENTIP			145
#define SLAPI_CONN_SERVERIP			146
/* OpenLDAP extensions */
#define SLAPI_X_CONN_CLIENTPATH			1300
#define SLAPI_X_CONN_SERVERPATH			1301
#define SLAPI_X_CONN_IS_UDP			1302
#define SLAPI_X_CONN_SSF			1303
#define SLAPI_X_CONN_SASL_CONTEXT		1304
#define SLAPI_X_OPERATION_DELETE_GLUE_PARENT	1305
#define SLAPI_X_RELAX			1306
#define SLAPI_X_MANAGEDIT			SLAPI_X_RELAX
#define SLAPI_X_OPERATION_NO_SCHEMA_CHECK	1307
#define SLAPI_X_ADD_STRUCTURAL_CLASS		1308
#define SLAPI_X_OPERATION_NO_SUBORDINATE_GLUE	1309

/*  Authentication types */
#define SLAPD_AUTH_NONE   "none"
#define SLAPD_AUTH_SIMPLE "simple"
#define SLAPD_AUTH_SSL    "SSL"
#define SLAPD_AUTH_SASL   "SASL " 

/* plugin configuration parmams */
#define SLAPI_PLUGIN				3
#define SLAPI_PLUGIN_PRIVATE			4
#define SLAPI_PLUGIN_TYPE			5
#define SLAPI_PLUGIN_ARGV			6
#define SLAPI_PLUGIN_ARGC			7
#define SLAPI_PLUGIN_VERSION			8
#define SLAPI_PLUGIN_OPRETURN			9
#define SLAPI_PLUGIN_OBJECT			10
#define SLAPI_PLUGIN_DESTROY_FN			11
#define SLAPI_PLUGIN_DESCRIPTION		12
#define SLAPI_PLUGIN_IDENTITY			13

/* internal opreations params */
#define SLAPI_PLUGIN_INTOP_RESULT		15
#define SLAPI_PLUGIN_INTOP_SEARCH_ENTRIES	16
#define SLAPI_PLUGIN_INTOP_SEARCH_REFERRALS	17

/* transaction arguments */
#define SLAPI_PARENT_TXN			190
#define SLAPI_TXN				191

/* function pointer params for backends */
#define SLAPI_PLUGIN_DB_BIND_FN			200
#define SLAPI_PLUGIN_DB_UNBIND_FN		201
#define SLAPI_PLUGIN_DB_SEARCH_FN		202
#define SLAPI_PLUGIN_DB_COMPARE_FN		203
#define SLAPI_PLUGIN_DB_MODIFY_FN		204
#define SLAPI_PLUGIN_DB_MODRDN_FN		205
#define SLAPI_PLUGIN_DB_ADD_FN			206
#define SLAPI_PLUGIN_DB_DELETE_FN		207
#define SLAPI_PLUGIN_DB_ABANDON_FN		208
#define SLAPI_PLUGIN_DB_CONFIG_FN		209
#define SLAPI_PLUGIN_CLOSE_FN			210
#define SLAPI_PLUGIN_DB_FLUSH_FN		211
#define SLAPI_PLUGIN_START_FN			212
#define SLAPI_PLUGIN_DB_SEQ_FN			213
#define SLAPI_PLUGIN_DB_ENTRY_FN		214
#define SLAPI_PLUGIN_DB_REFERRAL_FN		215
#define SLAPI_PLUGIN_DB_RESULT_FN		216
#define SLAPI_PLUGIN_DB_LDIF2DB_FN		217
#define SLAPI_PLUGIN_DB_DB2LDIF_FN		218
#define SLAPI_PLUGIN_DB_BEGIN_FN		219
#define SLAPI_PLUGIN_DB_COMMIT_FN		220
#define SLAPI_PLUGIN_DB_ABORT_FN		221
#define SLAPI_PLUGIN_DB_ARCHIVE2DB_FN		222
#define SLAPI_PLUGIN_DB_DB2ARCHIVE_FN		223
#define SLAPI_PLUGIN_DB_NEXT_SEARCH_ENTRY_FN	224
#define SLAPI_PLUGIN_DB_FREE_RESULT_SET_FN	225
#define	SLAPI_PLUGIN_DB_SIZE_FN			226
#define	SLAPI_PLUGIN_DB_TEST_FN			227


/*  functions pointers for LDAP V3 extended ops */
#define SLAPI_PLUGIN_EXT_OP_FN			300
#define SLAPI_PLUGIN_EXT_OP_OIDLIST		301

/* preoperation */
#define SLAPI_PLUGIN_PRE_BIND_FN		401
#define SLAPI_PLUGIN_PRE_UNBIND_FN		402
#define SLAPI_PLUGIN_PRE_SEARCH_FN		403
#define SLAPI_PLUGIN_PRE_COMPARE_FN		404
#define SLAPI_PLUGIN_PRE_MODIFY_FN		405
#define SLAPI_PLUGIN_PRE_MODRDN_FN		406
#define SLAPI_PLUGIN_PRE_ADD_FN			407
#define SLAPI_PLUGIN_PRE_DELETE_FN		408
#define SLAPI_PLUGIN_PRE_ABANDON_FN		409
#define SLAPI_PLUGIN_PRE_ENTRY_FN		410
#define SLAPI_PLUGIN_PRE_REFERRAL_FN		411
#define SLAPI_PLUGIN_PRE_RESULT_FN		412

/* internal preoperation */
#define SLAPI_PLUGIN_INTERNAL_PRE_ADD_FN	420
#define SLAPI_PLUGIN_INTERNAL_PRE_MODIFY_FN	421
#define SLAPI_PLUGIN_INTERNAL_PRE_MODRDN_FN	422
#define SLAPI_PLUGIN_INTERNAL_PRE_DELETE_FN	423

/* backend preoperation */
#define SLAPI_PLUGIN_BE_PRE_ADD_FN		450
#define SLAPI_PLUGIN_BE_PRE_MODIFY_FN		451
#define SLAPI_PLUGIN_BE_PRE_MODRDN_FN		452
#define SLAPI_PLUGIN_BE_PRE_DELETE_FN		453

/* postoperation */
#define SLAPI_PLUGIN_POST_BIND_FN		501
#define SLAPI_PLUGIN_POST_UNBIND_FN		502
#define SLAPI_PLUGIN_POST_SEARCH_FN		503
#define SLAPI_PLUGIN_POST_COMPARE_FN		504
#define SLAPI_PLUGIN_POST_MODIFY_FN		505
#define SLAPI_PLUGIN_POST_MODRDN_FN		506
#define SLAPI_PLUGIN_POST_ADD_FN		507
#define SLAPI_PLUGIN_POST_DELETE_FN		508
#define SLAPI_PLUGIN_POST_ABANDON_FN		509
#define SLAPI_PLUGIN_POST_ENTRY_FN		510
#define SLAPI_PLUGIN_POST_REFERRAL_FN		511
#define SLAPI_PLUGIN_POST_RESULT_FN		512

/* internal postoperation */
#define SLAPI_PLUGIN_INTERNAL_POST_ADD_FN	520
#define SLAPI_PLUGIN_INTERNAL_POST_MODIFY_FN	521
#define SLAPI_PLUGIN_INTERNAL_POST_MODRDN_FN	522
#define SLAPI_PLUGIN_INTERNAL_POST_DELETE_FN	523

/* backend postoperation */
#define SLAPI_PLUGIN_BE_POST_ADD_FN		550
#define SLAPI_PLUGIN_BE_POST_MODIFY_FN		551
#define SLAPI_PLUGIN_BE_POST_MODRDN_FN		552
#define SLAPI_PLUGIN_BE_POST_DELETE_FN		553

#define SLAPI_OPERATION_TYPE			590
#define SLAPI_OPERATION_MSGID			591

#define SLAPI_PLUGIN_MR_FILTER_CREATE_FN	600
#define SLAPI_PLUGIN_MR_INDEXER_CREATE_FN	601
#define SLAPI_PLUGIN_MR_FILTER_MATCH_FN		602
#define SLAPI_PLUGIN_MR_FILTER_INDEX_FN		603
#define SLAPI_PLUGIN_MR_FILTER_RESET_FN		604
#define SLAPI_PLUGIN_MR_INDEX_FN		605
#define SLAPI_PLUGIN_MR_OID			610
#define SLAPI_PLUGIN_MR_TYPE			611
#define SLAPI_PLUGIN_MR_VALUE			612
#define SLAPI_PLUGIN_MR_VALUES			613
#define SLAPI_PLUGIN_MR_KEYS			614
#define SLAPI_PLUGIN_MR_FILTER_REUSABLE		615
#define SLAPI_PLUGIN_MR_QUERY_OPERATOR		616
#define SLAPI_PLUGIN_MR_USAGE			617

#define SLAPI_MATCHINGRULE_NAME			1
#define SLAPI_MATCHINGRULE_OID			2
#define SLAPI_MATCHINGRULE_DESC			3
#define SLAPI_MATCHINGRULE_SYNTAX		4
#define SLAPI_MATCHINGRULE_OBSOLETE		5

#define SLAPI_OP_LESS					1
#define SLAPI_OP_LESS_OR_EQUAL				2
#define SLAPI_OP_EQUAL					3
#define SLAPI_OP_GREATER_OR_EQUAL			4
#define SLAPI_OP_GREATER				5
#define SLAPI_OP_SUBSTRING				6

#define SLAPI_PLUGIN_MR_USAGE_INDEX		0
#define SLAPI_PLUGIN_MR_USAGE_SORT		1

#define SLAPI_PLUGIN_SYNTAX_FILTER_AVA		700
#define SLAPI_PLUGIN_SYNTAX_FILTER_SUB		701
#define SLAPI_PLUGIN_SYNTAX_VALUES2KEYS		702
#define SLAPI_PLUGIN_SYNTAX_ASSERTION2KEYS_AVA	703
#define SLAPI_PLUGIN_SYNTAX_ASSERTION2KEYS_SUB	704
#define SLAPI_PLUGIN_SYNTAX_NAMES		705
#define SLAPI_PLUGIN_SYNTAX_OID			706
#define SLAPI_PLUGIN_SYNTAX_FLAGS		707
#define SLAPI_PLUGIN_SYNTAX_COMPARE		708

#define SLAPI_PLUGIN_SYNTAX_FLAG_ORKEYS			1
#define SLAPI_PLUGIN_SYNTAX_FLAG_ORDERING		2

#define SLAPI_PLUGIN_ACL_INIT			730
#define SLAPI_PLUGIN_ACL_SYNTAX_CHECK		731
#define SLAPI_PLUGIN_ACL_ALLOW_ACCESS		732
#define SLAPI_PLUGIN_ACL_MODS_ALLOWED		733
#define SLAPI_PLUGIN_ACL_MODS_UPDATE		734

#define SLAPI_OPERATION_AUTHTYPE                741
#define SLAPI_OPERATION_ID                      742
#define SLAPI_CONN_CERT                         743
#define SLAPI_CONN_AUTHMETHOD                   746
#define SLAPI_IS_INTERNAL_OPERATION 		748

#define SLAPI_RESULT_CODE                       881
#define SLAPI_RESULT_TEXT                       882
#define SLAPI_RESULT_MATCHED                    883

/* managedsait control */
#define SLAPI_MANAGEDSAIT       		1000

/* audit plugin defines */
#define SLAPI_PLUGIN_AUDIT_DATA                1100
#define SLAPI_PLUGIN_AUDIT_FN                  1101

/* backend_group extension */
#define SLAPI_X_PLUGIN_PRE_GROUP_FN		1202 
#define SLAPI_X_PLUGIN_POST_GROUP_FN		1203

#define SLAPI_X_GROUP_ENTRY			1250 /* group entry */
#define SLAPI_X_GROUP_ATTRIBUTE			1251 /* member attribute */
#define SLAPI_X_GROUP_OPERATION_DN		1252 /* asserted value */
#define SLAPI_X_GROUP_TARGET_ENTRY		1253 /* target entry */

/* internal preoperation extensions */
#define SLAPI_PLUGIN_INTERNAL_PRE_BIND_FN	1260
#define SLAPI_PLUGIN_INTERNAL_PRE_UNBIND_FN	1261
#define SLAPI_PLUGIN_INTERNAL_PRE_SEARCH_FN	1262
#define SLAPI_PLUGIN_INTERNAL_PRE_COMPARE_FN	1263
#define SLAPI_PLUGIN_INTERNAL_PRE_ABANDON_FN	1264

/* internal postoperation extensions */
#define SLAPI_PLUGIN_INTERNAL_POST_BIND_FN	1270
#define SLAPI_PLUGIN_INTERNAL_POST_UNBIND_FN	1271
#define SLAPI_PLUGIN_INTERNAL_POST_SEARCH_FN	1272
#define SLAPI_PLUGIN_INTERNAL_POST_COMPARE_FN	1273
#define SLAPI_PLUGIN_INTERNAL_POST_ABANDON_FN	1274

/* config stuff */
#define SLAPI_CONFIG_FILENAME			40
#define SLAPI_CONFIG_LINENO			41
#define SLAPI_CONFIG_ARGC			42
#define SLAPI_CONFIG_ARGV			43

/*  operational params */
#define SLAPI_TARGET_ADDRESS			48
#define SLAPI_TARGET_UNIQUEID			49
#define SLAPI_TARGET_DN				50

/* server LDAPv3 controls  */
#define SLAPI_REQCONTROLS			51
#define SLAPI_RESCONTROLS			55
#define SLAPI_ADD_RESCONTROL			56	
#define SLAPI_CONTROLS_ARG			58

/* add params */
#define SLAPI_ADD_TARGET			SLAPI_TARGET_DN
#define SLAPI_ADD_ENTRY				60
#define SLAPI_ADD_EXISTING_DN_ENTRY		61
#define SLAPI_ADD_PARENT_ENTRY			62
#define SLAPI_ADD_PARENT_UNIQUEID		63
#define SLAPI_ADD_EXISTING_UNIQUEID_ENTRY	64

/* bind params */
#define SLAPI_BIND_TARGET			SLAPI_TARGET_DN
#define SLAPI_BIND_METHOD			70
#define SLAPI_BIND_CREDENTIALS			71	
#define SLAPI_BIND_SASLMECHANISM		72	
#define SLAPI_BIND_RET_SASLCREDS		73	

/* compare params */
#define SLAPI_COMPARE_TARGET			SLAPI_TARGET_DN
#define SLAPI_COMPARE_TYPE			80
#define SLAPI_COMPARE_VALUE			81

/* delete params */
#define SLAPI_DELETE_TARGET			SLAPI_TARGET_DN
#define SLAPI_DELETE_EXISTING_ENTRY		SLAPI_ADD_EXISTING_DN_ENTRY

/* modify params */
#define SLAPI_MODIFY_TARGET			SLAPI_TARGET_DN
#define SLAPI_MODIFY_MODS			90
#define SLAPI_MODIFY_EXISTING_ENTRY		SLAPI_ADD_EXISTING_DN_ENTRY

/* modrdn params */
#define SLAPI_MODRDN_TARGET			SLAPI_TARGET_DN
#define SLAPI_MODRDN_NEWRDN			100
#define SLAPI_MODRDN_DELOLDRDN			101
#define SLAPI_MODRDN_NEWSUPERIOR		102	/* v3 only */
#define SLAPI_MODRDN_EXISTING_ENTRY             SLAPI_ADD_EXISTING_DN_ENTRY
#define SLAPI_MODRDN_PARENT_ENTRY		104
#define SLAPI_MODRDN_NEWPARENT_ENTRY		105
#define SLAPI_MODRDN_TARGET_ENTRY		106
#define SLAPI_MODRDN_NEWSUPERIOR_ADDRESS	107

/* search params */
#define SLAPI_SEARCH_TARGET			SLAPI_TARGET_DN
#define SLAPI_SEARCH_SCOPE			110
#define SLAPI_SEARCH_DEREF			111
#define SLAPI_SEARCH_SIZELIMIT			112
#define SLAPI_SEARCH_TIMELIMIT			113
#define SLAPI_SEARCH_FILTER			114
#define SLAPI_SEARCH_STRFILTER			115
#define SLAPI_SEARCH_ATTRS			116
#define SLAPI_SEARCH_ATTRSONLY			117

/* abandon params */
#define SLAPI_ABANDON_MSGID			120

/* extended operation params */
#define SLAPI_EXT_OP_REQ_OID			160
#define SLAPI_EXT_OP_REQ_VALUE		161	

/* extended operation return codes */
#define SLAPI_EXT_OP_RET_OID			162	
#define SLAPI_EXT_OP_RET_VALUE		163	

#define SLAPI_PLUGIN_EXTENDED_SENT_RESULT	-1

#define SLAPI_FAIL_DISKFULL		-2
#define SLAPI_FAIL_GENERAL		-1
#define SLAPI_PLUGIN_EXTENDED_NOT_HANDLED -2
#define SLAPI_BIND_SUCCESS		0
#define SLAPI_BIND_FAIL			2
#define SLAPI_BIND_ANONYMOUS		3

/* Search result params */
#define SLAPI_SEARCH_RESULT_SET			193
#define	SLAPI_SEARCH_RESULT_ENTRY		194
#define	SLAPI_NENTRIES				195
#define SLAPI_SEARCH_REFERRALS			196

/* filter types */
#ifndef LDAP_FILTER_AND
#define LDAP_FILTER_AND         0xa0L
#endif
#ifndef LDAP_FILTER_OR
#define LDAP_FILTER_OR          0xa1L
#endif
#ifndef LDAP_FILTER_NOT
#define LDAP_FILTER_NOT         0xa2L
#endif
#ifndef LDAP_FILTER_EQUALITY
#define LDAP_FILTER_EQUALITY    0xa3L
#endif
#ifndef LDAP_FILTER_SUBSTRINGS
#define LDAP_FILTER_SUBSTRINGS  0xa4L
#endif
#ifndef LDAP_FILTER_GE
#define LDAP_FILTER_GE          0xa5L
#endif
#ifndef LDAP_FILTER_LE
#define LDAP_FILTER_LE          0xa6L
#endif
#ifndef LDAP_FILTER_PRESENT
#define LDAP_FILTER_PRESENT     0x87L
#endif
#ifndef LDAP_FILTER_APPROX
#define LDAP_FILTER_APPROX      0xa8L
#endif
#ifndef LDAP_FILTER_EXT_MATCH
#define LDAP_FILTER_EXT_MATCH   0xa9L
#endif

int slapi_log_error( int severity, char *subsystem, char *fmt, ... );
#define SLAPI_LOG_FATAL                 0
#define SLAPI_LOG_TRACE                 1
#define SLAPI_LOG_PACKETS               2
#define SLAPI_LOG_ARGS                  3
#define SLAPI_LOG_CONNS                 4
#define SLAPI_LOG_BER                   5
#define SLAPI_LOG_FILTER                6
#define SLAPI_LOG_CONFIG                7
#define SLAPI_LOG_ACL                   8
#define SLAPI_LOG_SHELL                 9
#define SLAPI_LOG_PARSE                 10
#define SLAPI_LOG_HOUSE                 11
#define SLAPI_LOG_REPL                  12
#define SLAPI_LOG_CACHE                 13
#define SLAPI_LOG_PLUGIN                14
#define SLAPI_LOG_TIMING                15

#define SLAPI_PLUGIN_DESCRIPTION	12
typedef struct slapi_plugindesc {
        char    *spd_id;
        char    *spd_vendor;
        char    *spd_version;
        char    *spd_description;
} Slapi_PluginDesc;

#define SLAPI_PLUGIN_VERSION_01         "01"
#define SLAPI_PLUGIN_VERSION_02         "02"
#define SLAPI_PLUGIN_VERSION_03         "03"
#define SLAPI_PLUGIN_CURRENT_VERSION    SLAPI_PLUGIN_VERSION_03

#endif /* _SLAPI_PLUGIN_H */

