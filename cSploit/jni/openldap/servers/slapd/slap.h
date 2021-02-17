/* slap.h - stand alone ldap server include file */
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

#ifndef _SLAP_H_
#define _SLAP_H_

#include "ldap_defaults.h"

#include <stdio.h>
#include <ac/stdlib.h>

#include <sys/types.h>
#include <ac/syslog.h>
#include <ac/regex.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/time.h>
#include <ac/param.h>

#include "avl.h"

#ifndef ldap_debug
#define ldap_debug slap_debug
#endif

#include "ldap_log.h"

#include <ldap.h>
#include <ldap_schema.h>

#include "lber_pvt.h"
#include "ldap_pvt.h"
#include "ldap_pvt_thread.h"
#include "ldap_queue.h"

LDAP_BEGIN_DECL

#define LDAP_COLLECTIVE_ATTRIBUTES
#define LDAP_COMP_MATCH
#define LDAP_SYNC_TIMESTAMP
#define SLAP_CONTROL_X_WHATFAILED
#define SLAP_CONFIG_DELETE
#define SLAP_AUXPROP_DONTUSECOPY
#ifndef SLAP_SCHEMA_EXPOSE
#define SLAP_SCHEMA_EXPOSE
#endif

#define LDAP_DYNAMIC_OBJECTS
#define SLAP_CONTROL_X_TREE_DELETE LDAP_CONTROL_X_TREE_DELETE
#define SLAP_CONTROL_X_SESSION_TRACKING
#define SLAP_DISTPROC

#ifdef ENABLE_REWRITE
#define SLAP_AUTH_REWRITE	1 /* use librewrite for sasl-regexp */
#endif

/*
 * SLAPD Memory allocation macros
 *
 * Unlike ch_*() routines, these routines do not assert() upon
 * allocation error.  They are intended to be used instead of
 * ch_*() routines where the caller has implemented proper
 * checking for and handling of allocation errors.
 *
 * Patches to convert ch_*() calls to SLAP_*() calls welcomed.
 */
#define SLAP_MALLOC(s)      ber_memalloc((s))
#define SLAP_CALLOC(n,s)    ber_memcalloc((n),(s))
#define SLAP_REALLOC(p,s)   ber_memrealloc((p),(s))
#define SLAP_FREE(p)        ber_memfree((p))
#define SLAP_VFREE(v)       ber_memvfree((void**)(v))
#define SLAP_STRDUP(s)      ber_strdup((s))
#define SLAP_STRNDUP(s,l)   ber_strndup((s),(l))

#ifdef f_next
#undef f_next /* name conflict between sys/file.h on SCO and struct filter */
#endif

#define SERVICE_NAME  OPENLDAP_PACKAGE "-slapd"
#define SLAPD_ANONYMOUS ""

#ifdef HAVE_TCPD
# include <tcpd.h>
# define SLAP_STRING_UNKNOWN	STRING_UNKNOWN
#else /* ! TCP Wrappers */
# define SLAP_STRING_UNKNOWN	"unknown"
#endif /* ! TCP Wrappers */

/* LDAPMod.mod_op value ===> Must be kept in sync with ldap.h! */
/* These values are used internally by the backends. */
/* SLAP_MOD_SOFTADD allows adding values that already exist without getting
 * an error as required by modrdn when the new rdn was already an attribute
 * value itself.
 */
#define SLAP_MOD_SOFTADD		0x1000
/* SLAP_MOD_SOFTDEL allows deleting values if they exist without getting
 * an error otherwise.
 */
#define SLAP_MOD_SOFTDEL		0x1001
/* SLAP_MOD_ADD_IF_NOT_PRESENT allows adding values unless the attribute
 * is already present without getting an error.
 */
#define SLAP_MOD_ADD_IF_NOT_PRESENT	0x1002
/* SLAP_MOD_DEL_IF_PRESENT allows deleting values if the attribute
 * is present, without getting an error otherwise.
 * The semantics can be obtained using SLAP_MOD_SOFTDEL with NULL values.
 */

#define MAXREMATCHES (100)

#define SLAP_MAX_WORKER_THREADS		(16)

#define SLAP_SB_MAX_INCOMING_DEFAULT ((1<<18) - 1)
#define SLAP_SB_MAX_INCOMING_AUTH ((1<<24) - 1)

#define SLAP_CONN_MAX_PENDING_DEFAULT	100
#define SLAP_CONN_MAX_PENDING_AUTH	1000

#define SLAP_TEXT_BUFLEN (256)

/* pseudo error code indicating abandoned operation */
#define SLAPD_ABANDON (-1024)

/* pseudo error code indicating disconnect */
#define SLAPD_DISCONNECT (-1025)

/* unknown config file directive */
#define SLAP_CONF_UNKNOWN (-1026)

/* We assume "C" locale, that is US-ASCII */
#define ASCII_SPACE(c)	( (c) == ' ' )
#define ASCII_LOWER(c)	( (c) >= 'a' && (c) <= 'z' )
#define ASCII_UPPER(c)	( (c) >= 'A' && (c) <= 'Z' )
#define ASCII_ALPHA(c)	( ASCII_LOWER(c) || ASCII_UPPER(c) )
#define ASCII_DIGIT(c)	( (c) >= '0' && (c) <= '9' )
#define ASCII_HEXLOWER(c)	( (c) >= 'a' && (c) <= 'f' )
#define ASCII_HEXUPPER(c)	( (c) >= 'A' && (c) <= 'F' )
#define ASCII_HEX(c)	( ASCII_DIGIT(c) || \
	ASCII_HEXLOWER(c) || ASCII_HEXUPPER(c) )
#define ASCII_ALNUM(c)	( ASCII_ALPHA(c) || ASCII_DIGIT(c) )
#define ASCII_PRINTABLE(c) ( (c) >= ' ' && (c) <= '~' )

#define SLAP_NIBBLE(c) ((c)&0x0f)
#define SLAP_ESCAPE_CHAR ('\\')
#define SLAP_ESCAPE_LO(c) ( "0123456789ABCDEF"[SLAP_NIBBLE(c)] )
#define SLAP_ESCAPE_HI(c) ( SLAP_ESCAPE_LO((c)>>4) )

#define FILTER_ESCAPE(c) ( (c) == '*' || (c) == '\\' \
	|| (c) == '(' || (c) == ')' || !ASCII_PRINTABLE(c) )

#define DN_ESCAPE(c)	((c) == SLAP_ESCAPE_CHAR)
/* NOTE: for consistency, this macro must only operate
 * on normalized/pretty DN, such that ';' is never used
 * as RDN separator, and all occurrences of ';' must be escaped */
#define DN_SEPARATOR(c)	((c) == ',')
#define RDN_ATTRTYPEANDVALUE_SEPARATOR(c) ((c) == '+') /* RFC 4514 */
#define RDN_SEPARATOR(c) (DN_SEPARATOR(c) || RDN_ATTRTYPEANDVALUE_SEPARATOR(c))
#define RDN_NEEDSESCAPE(c)	((c) == '\\' || (c) == '"')

#define DESC_LEADCHAR(c)	( ASCII_ALPHA(c) )
#define DESC_CHAR(c)	( ASCII_ALNUM(c) || (c) == '-' )
#define OID_LEADCHAR(c)	( ASCII_DIGIT(c) )
#define OID_SEPARATOR(c)	( (c) == '.' )
#define OID_CHAR(c)	( OID_LEADCHAR(c) || OID_SEPARATOR(c) )

#define ATTR_LEADCHAR(c)	( DESC_LEADCHAR(c) || OID_LEADCHAR(c) )
#define ATTR_CHAR(c)	( DESC_CHAR((c)) || OID_SEPARATOR(c) )

#define AD_LEADCHAR(c)	( ATTR_LEADCHAR(c) )
#define AD_CHAR(c)		( ATTR_CHAR(c) || (c) == ';' )

#define SLAP_NUMERIC(c) ( ASCII_DIGIT(c) || ASCII_SPACE(c) )

#define SLAP_PRINTABLE(c)	( ASCII_ALNUM(c) || (c) == '\'' || \
	(c) == '(' || (c) == ')' || (c) == '+' || (c) == ',' || \
	(c) == '-' || (c) == '.' || (c) == '/' || (c) == ':' || \
	(c) == '?' || (c) == ' ' || (c) == '=' )
#define SLAP_PRINTABLES(c)	( SLAP_PRINTABLE(c) || (c) == '$' )

/* must match in schema_init.c */
#define SLAPD_DN_SYNTAX			"1.3.6.1.4.1.1466.115.121.1.12"
#define SLAPD_NAMEUID_SYNTAX	"1.3.6.1.4.1.1466.115.121.1.34"
#define SLAPD_INTEGER_SYNTAX	"1.3.6.1.4.1.1466.115.121.1.27"
#define SLAPD_GROUP_ATTR		"member"
#define SLAPD_GROUP_CLASS		"groupOfNames"
#define SLAPD_ROLE_ATTR			"roleOccupant"
#define SLAPD_ROLE_CLASS		"organizationalRole"

#define SLAPD_TOP_OID			"2.5.6.0"

LDAP_SLAPD_V (int) slap_debug;

typedef unsigned long slap_mask_t;

/* Security Strength Factor */
typedef unsigned slap_ssf_t;

typedef struct slap_ssf_set {
	slap_ssf_t sss_ssf;
	slap_ssf_t sss_transport;
	slap_ssf_t sss_tls;
	slap_ssf_t sss_sasl;
	slap_ssf_t sss_update_ssf;
	slap_ssf_t sss_update_transport;
	slap_ssf_t sss_update_tls;
	slap_ssf_t sss_update_sasl;
	slap_ssf_t sss_simple_bind;
} slap_ssf_set_t;

/* Flags for telling slap_sasl_getdn() what type of identity is being passed */
#define SLAP_GETDN_AUTHCID 2
#define SLAP_GETDN_AUTHZID 4

/*
 * Index types
 */
#define SLAP_INDEX_TYPE           0x00FFUL
#define SLAP_INDEX_UNDEFINED      0x0001UL
#define SLAP_INDEX_PRESENT        0x0002UL
#define SLAP_INDEX_EQUALITY       0x0004UL
#define SLAP_INDEX_APPROX         0x0008UL
#define SLAP_INDEX_SUBSTR         0x0010UL
#define SLAP_INDEX_EXTENDED		  0x0020UL

#define SLAP_INDEX_DEFAULT        SLAP_INDEX_EQUALITY

#define IS_SLAP_INDEX(mask, type)	(((mask) & (type)) == (type))

#define SLAP_INDEX_SUBSTR_TYPE    0x0F00UL

#define SLAP_INDEX_SUBSTR_INITIAL ( SLAP_INDEX_SUBSTR | 0x0100UL ) 
#define SLAP_INDEX_SUBSTR_ANY     ( SLAP_INDEX_SUBSTR | 0x0200UL )
#define SLAP_INDEX_SUBSTR_FINAL   ( SLAP_INDEX_SUBSTR | 0x0400UL )
#define SLAP_INDEX_SUBSTR_DEFAULT \
	( SLAP_INDEX_SUBSTR \
	| SLAP_INDEX_SUBSTR_INITIAL \
	| SLAP_INDEX_SUBSTR_ANY \
	| SLAP_INDEX_SUBSTR_FINAL )

/* defaults for initial/final substring indices */
#define SLAP_INDEX_SUBSTR_IF_MINLEN_DEFAULT	2
#define SLAP_INDEX_SUBSTR_IF_MAXLEN_DEFAULT	4

/* defaults for any substring indices */
#define SLAP_INDEX_SUBSTR_ANY_LEN_DEFAULT		4
#define SLAP_INDEX_SUBSTR_ANY_STEP_DEFAULT		2

/* default for ordered integer index keys */
#define SLAP_INDEX_INTLEN_DEFAULT	4

#define SLAP_INDEX_FLAGS         0xF000UL
#define SLAP_INDEX_NOSUBTYPES    0x1000UL /* don't use index w/ subtypes */
#define SLAP_INDEX_NOTAGS        0x2000UL /* don't use index w/ tags */

/*
 * there is a single index for each attribute.  these prefixes ensure
 * that there is no collision among keys.
 */
#define SLAP_INDEX_EQUALITY_PREFIX	'=' 	/* prefix for equality keys     */
#define SLAP_INDEX_APPROX_PREFIX	'~'		/* prefix for approx keys       */
#define SLAP_INDEX_SUBSTR_PREFIX	'*'		/* prefix for substring keys    */
#define SLAP_INDEX_SUBSTR_INITIAL_PREFIX '^'
#define SLAP_INDEX_SUBSTR_FINAL_PREFIX '$'
#define SLAP_INDEX_CONT_PREFIX		'.'		/* prefix for continuation keys */

#define SLAP_SYNTAX_MATCHINGRULES_OID	 "1.3.6.1.4.1.1466.115.121.1.30"
#define SLAP_SYNTAX_ATTRIBUTETYPES_OID	 "1.3.6.1.4.1.1466.115.121.1.3"
#define SLAP_SYNTAX_OBJECTCLASSES_OID	 "1.3.6.1.4.1.1466.115.121.1.37"
#define SLAP_SYNTAX_MATCHINGRULEUSES_OID "1.3.6.1.4.1.1466.115.121.1.31"
#define SLAP_SYNTAX_CONTENTRULE_OID	 "1.3.6.1.4.1.1466.115.121.1.16"

/*
 * represents schema information for a database
 */
enum {
	SLAP_SCHERR_OUTOFMEM = 1,
	SLAP_SCHERR_CLASS_NOT_FOUND,
	SLAP_SCHERR_CLASS_BAD_USAGE,
	SLAP_SCHERR_CLASS_BAD_SUP,
	SLAP_SCHERR_CLASS_DUP,
	SLAP_SCHERR_CLASS_INCONSISTENT,
	SLAP_SCHERR_ATTR_NOT_FOUND,
	SLAP_SCHERR_ATTR_BAD_MR,
	SLAP_SCHERR_ATTR_BAD_USAGE,
	SLAP_SCHERR_ATTR_BAD_SUP,
	SLAP_SCHERR_ATTR_INCOMPLETE,
	SLAP_SCHERR_ATTR_DUP,
	SLAP_SCHERR_ATTR_INCONSISTENT,
	SLAP_SCHERR_MR_NOT_FOUND,
	SLAP_SCHERR_MR_INCOMPLETE,
	SLAP_SCHERR_MR_DUP,
	SLAP_SCHERR_SYN_NOT_FOUND,
	SLAP_SCHERR_SYN_DUP,
	SLAP_SCHERR_SYN_SUP_NOT_FOUND,
	SLAP_SCHERR_SYN_SUBST_NOT_SPECIFIED,
	SLAP_SCHERR_SYN_SUBST_NOT_FOUND,
	SLAP_SCHERR_NO_NAME,
	SLAP_SCHERR_NOT_SUPPORTED,
	SLAP_SCHERR_BAD_DESCR,
	SLAP_SCHERR_OIDM,
	SLAP_SCHERR_CR_DUP,
	SLAP_SCHERR_CR_BAD_STRUCT,
	SLAP_SCHERR_CR_BAD_AUX,
	SLAP_SCHERR_CR_BAD_AT,

	SLAP_SCHERR_LAST
};

/* forward declarations */
typedef struct Syntax Syntax;
typedef struct MatchingRule MatchingRule;
typedef struct MatchingRuleUse MatchingRuleUse;
typedef struct MatchingRuleAssertion MatchingRuleAssertion;
typedef struct OidMacro OidMacro;
typedef struct ObjectClass ObjectClass;
typedef struct AttributeType AttributeType;
typedef struct AttributeDescription AttributeDescription;
typedef struct AttributeName AttributeName;
typedef struct ContentRule ContentRule;

typedef struct AttributeAssertion AttributeAssertion;
typedef struct SubstringsAssertion SubstringsAssertion;
typedef struct Filter Filter;
typedef struct ValuesReturnFilter ValuesReturnFilter;
typedef struct Attribute Attribute;
#ifdef LDAP_COMP_MATCH
typedef struct ComponentData ComponentData;
typedef struct ComponentFilter ComponentFilter;
#endif

typedef struct Entry Entry;
typedef struct Modification Modification;
typedef struct Modifications Modifications;
typedef struct LDAPModList LDAPModList;

typedef struct BackendInfo BackendInfo;		/* per backend type */
typedef struct BackendDB BackendDB;		/* per backend database */

typedef struct Connection Connection;
typedef struct Operation Operation;
typedef struct SlapReply SlapReply;
/* end of forward declarations */

typedef union Sockaddr {
	struct sockaddr sa_addr;
	struct sockaddr_in sa_in_addr;
#ifdef LDAP_PF_INET6
	struct sockaddr_storage sa_storage;
	struct sockaddr_in6 sa_in6_addr;
#endif
#ifdef LDAP_PF_LOCAL
	struct sockaddr_un sa_un_addr;
#endif
} Sockaddr;

#ifdef LDAP_PF_INET6
extern int slap_inet4or6;
#endif

struct OidMacro {
	struct berval som_oid;
	BerVarray som_names;
	BerVarray som_subs;
#define	SLAP_OM_HARDCODE	0x10000U	/* This is hardcoded schema */
	int som_flags;
	LDAP_STAILQ_ENTRY(OidMacro) som_next;
};

typedef int slap_syntax_validate_func LDAP_P((
	Syntax *syntax,
	struct berval * in));

typedef int slap_syntax_transform_func LDAP_P((
	Syntax *syntax,
	struct berval * in,
	struct berval * out,
	void *memctx));

#ifdef LDAP_COMP_MATCH
typedef void* slap_component_transform_func LDAP_P((
	struct berval * in ));
struct ComponentDesc;
#endif

struct Syntax {
	LDAPSyntax			ssyn_syn;
#define ssyn_oid		ssyn_syn.syn_oid
#define ssyn_desc		ssyn_syn.syn_desc
#define ssyn_extensions	ssyn_syn.syn_extensions
	/*
	 * Note: the former
	ber_len_t	ssyn_oidlen;
	 * has been replaced by a struct berval that uses the value
	 * provided by ssyn_syn.syn_oid; a macro that expands to
	 * the bv_len field of the berval is provided for backward
	 * compatibility.  CAUTION: NEVER FREE THE BERVAL
	 */
	struct berval	ssyn_bvoid;
#define	ssyn_oidlen	ssyn_bvoid.bv_len

	unsigned int ssyn_flags;

#define SLAP_SYNTAX_NONE	0x0000U
#define SLAP_SYNTAX_BLOB	0x0001U /* syntax treated as blob (audio) */
#define SLAP_SYNTAX_BINARY	0x0002U /* binary transfer required (certificate) */
#define SLAP_SYNTAX_BER		0x0004U /* stored in BER encoding (certificate) */
#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_SYNTAX_HIDE	0x0000U /* publish everything */
#else
#define SLAP_SYNTAX_HIDE	0x8000U /* hide (do not publish) */
#endif
#define	SLAP_SYNTAX_HARDCODE	0x10000U	/* This is hardcoded schema */
#define	SLAP_SYNTAX_DN		0x20000U	/* Treat like a DN */

	Syntax				**ssyn_sups;

	slap_syntax_validate_func	*ssyn_validate;
	slap_syntax_transform_func	*ssyn_pretty;

#ifdef SLAPD_BINARY_CONVERSION
	/* convert to and from binary */
	slap_syntax_transform_func	*ssyn_ber2str;
	slap_syntax_transform_func	*ssyn_str2ber;
#endif
#ifdef LDAP_COMP_MATCH
	slap_component_transform_func *ssyn_attr2comp;
	struct ComponentDesc* ssync_comp_syntax;
#endif

	LDAP_STAILQ_ENTRY(Syntax)	ssyn_next;
};

#define slap_syntax_is_flag(s,flag) ((int)((s)->ssyn_flags & (flag)) ? 1 : 0)
#define slap_syntax_is_blob(s)		slap_syntax_is_flag((s),SLAP_SYNTAX_BLOB)
#define slap_syntax_is_binary(s)	slap_syntax_is_flag((s),SLAP_SYNTAX_BINARY)
#define slap_syntax_is_ber(s)		slap_syntax_is_flag((s),SLAP_SYNTAX_BER)
#define slap_syntax_is_hidden(s)	slap_syntax_is_flag((s),SLAP_SYNTAX_HIDE)

typedef struct slap_syntax_defs_rec {
	char *sd_desc;
	int sd_flags;
	char **sd_sups;
	slap_syntax_validate_func *sd_validate;
	slap_syntax_transform_func *sd_pretty;
#ifdef SLAPD_BINARY_CONVERSION
	slap_syntax_transform_func *sd_ber2str;
	slap_syntax_transform_func *sd_str2ber;
#endif
} slap_syntax_defs_rec;

/* X -> Y Converter */
typedef int slap_mr_convert_func LDAP_P((
	struct berval * in,
	struct berval * out,
	void *memctx ));

/* Normalizer */
typedef int slap_mr_normalize_func LDAP_P((
	slap_mask_t use,
	Syntax *syntax, /* NULL if in is asserted value */
	MatchingRule *mr,
	struct berval *in,
	struct berval *out,
	void *memctx ));

/* Match (compare) function */
typedef int slap_mr_match_func LDAP_P((
	int *match,
	slap_mask_t use,
	Syntax *syntax,	/* syntax of stored value */
	MatchingRule *mr,
	struct berval *value,
	void *assertValue ));

/* Index generation function */
typedef int slap_mr_indexer_func LDAP_P((
	slap_mask_t use,
	slap_mask_t mask,
	Syntax *syntax,	/* syntax of stored value */
	MatchingRule *mr,
	struct berval *prefix,
	BerVarray values,
	BerVarray *keys,
	void *memctx ));

/* Filter index function */
typedef int slap_mr_filter_func LDAP_P((
	slap_mask_t use,
	slap_mask_t mask,
	Syntax *syntax,	/* syntax of stored value */
	MatchingRule *mr,
	struct berval *prefix,
	void *assertValue,
	BerVarray *keys,
	void *memctx ));

struct MatchingRule {
	LDAPMatchingRule		smr_mrule;
	MatchingRuleUse			*smr_mru;
	/* RFC 4512 string representation */
	struct berval			smr_str;
	/*
	 * Note: the former
	 *			ber_len_t	smr_oidlen;
	 * has been replaced by a struct berval that uses the value
	 * provided by smr_mrule.mr_oid; a macro that expands to
	 * the bv_len field of the berval is provided for backward
	 * compatibility.  CAUTION: NEVER FREE THE BERVAL
	 */
	struct berval			smr_bvoid;
#define	smr_oidlen			smr_bvoid.bv_len

	slap_mask_t			smr_usage;

#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_MR_HIDE			0x0000U
#else
#define SLAP_MR_HIDE			0x8000U
#endif

#define SLAP_MR_MUTATION_NORMALIZER	0x4000U

#define SLAP_MR_TYPE_MASK		0x0F00U
#define SLAP_MR_SUBTYPE_MASK		0x00F0U
#define SLAP_MR_USAGE			0x000FU

#define SLAP_MR_NONE			0x0000U
#define SLAP_MR_EQUALITY		0x0100U
#define SLAP_MR_ORDERING		0x0200U
#define SLAP_MR_SUBSTR			0x0400U
#define SLAP_MR_EXT			0x0800U /* implicitly extensible */
#define	SLAP_MR_ORDERED_INDEX		0x1000U
#ifdef LDAP_COMP_MATCH
#define SLAP_MR_COMPONENT		0x2000U
#endif

#define SLAP_MR_EQUALITY_APPROX	( SLAP_MR_EQUALITY | 0x0010U )

#define SLAP_MR_SUBSTR_INITIAL	( SLAP_MR_SUBSTR | 0x0010U )
#define SLAP_MR_SUBSTR_ANY	( SLAP_MR_SUBSTR | 0x0020U )
#define SLAP_MR_SUBSTR_FINAL	( SLAP_MR_SUBSTR | 0x0040U )


/*
 * The asserted value, depending on the particular usage,
 * is expected to conform to either the assertion syntax
 * or the attribute syntax.   In some cases, the syntax of
 * the value is known.  If so, these flags indicate which
 * syntax the value is expected to conform to.  If not,
 * neither of these flags is set (until the syntax of the
 * provided value is determined).  If the value is of the
 * attribute syntax, the flag is changed once a value of
 * the assertion syntax is derived from the provided value.
 */
#define SLAP_MR_VALUE_OF_ASSERTION_SYNTAX	0x0001U
#define SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX	0x0002U
#define SLAP_MR_VALUE_OF_SYNTAX			(SLAP_MR_VALUE_OF_ASSERTION_SYNTAX|SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX)
#define SLAP_MR_DENORMALIZE			(SLAP_MR_MUTATION_NORMALIZER)

#define SLAP_MR_IS_VALUE_OF_ATTRIBUTE_SYNTAX( usage ) \
	((usage) & SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX )
#define SLAP_MR_IS_VALUE_OF_ASSERTION_SYNTAX( usage ) \
	((usage) & SLAP_MR_VALUE_OF_ASSERTION_SYNTAX )
#ifdef LDAP_DEBUG
#define SLAP_MR_IS_VALUE_OF_SYNTAX( usage ) \
	((usage) & SLAP_MR_VALUE_OF_SYNTAX)
#else
#define SLAP_MR_IS_VALUE_OF_SYNTAX( usage )	(1)
#endif
#define SLAP_MR_IS_DENORMALIZE( usage ) \
	((usage) & SLAP_MR_DENORMALIZE )

/* either or both the asserted value or attribute value
 * may be provided in normalized form
 */
#define SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH		0x0004U
#define SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH	0x0008U

#define SLAP_IS_MR_ASSERTION_SYNTAX_MATCH( usage ) \
	(!((usage) & SLAP_MR_ATTRIBUTE_SYNTAX_MATCH))
#define SLAP_IS_MR_ATTRIBUTE_SYNTAX_MATCH( usage ) \
	((usage) & SLAP_MR_ATTRIBUTE_SYNTAX_MATCH)

#define SLAP_IS_MR_ATTRIBUTE_SYNTAX_CONVERTED_MATCH( usage ) \
	(((usage) & SLAP_MR_ATTRIBUTE_SYNTAX_CONVERTED_MATCH) \
		== SLAP_MR_ATTRIBUTE_SYNTAX_CONVERTED_MATCH)
#define SLAP_IS_MR_ATTRIBUTE_SYNTAX_NONCONVERTED_MATCH( usage ) \
	(((usage) & SLAP_MR_ATTRIBUTE_SYNTAX_CONVERTED_MATCH) \
		== SLAP_MR_ATTRIBUTE_SYNTAX_MATCH)

#define SLAP_IS_MR_ASSERTED_VALUE_NORMALIZED_MATCH( usage ) \
	((usage) & SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH )
#define SLAP_IS_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH( usage ) \
	((usage) & SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH )

	Syntax			*smr_syntax;
	slap_mr_convert_func	*smr_convert;
	slap_mr_normalize_func	*smr_normalize;
	slap_mr_match_func	*smr_match;
	slap_mr_indexer_func	*smr_indexer;
	slap_mr_filter_func	*smr_filter;

	/*
	 * null terminated array of syntaxes compatible with this syntax
	 * note: when MS_EXT is set, this MUST NOT contain the assertion
	 * syntax of the rule.  When MS_EXT is not set, it MAY.
	 */
	Syntax			**smr_compat_syntaxes;

	/*
	 * For equality rules, refers to an associated approximate rule.
	 * For non-equality rules, refers to an associated equality rule.
	 */
	MatchingRule	*smr_associated;

#define SLAP_MR_ASSOCIATED(mr,amr)	\
	(((mr) == (amr)) || ((mr)->smr_associated == (amr)))

	LDAP_SLIST_ENTRY(MatchingRule)	smr_next;

#define smr_oid				smr_mrule.mr_oid
#define smr_names			smr_mrule.mr_names
#define smr_desc			smr_mrule.mr_desc
#define smr_obsolete		smr_mrule.mr_obsolete
#define smr_syntax_oid		smr_mrule.mr_syntax_oid
#define smr_extensions		smr_mrule.mr_extensions
};

struct MatchingRuleUse {
	LDAPMatchingRuleUse		smru_mruleuse;
	MatchingRule			*smru_mr;
	/* RFC 4512 string representation */
	struct berval			smru_str;

	LDAP_SLIST_ENTRY(MatchingRuleUse) smru_next;

#define smru_oid			smru_mruleuse.mru_oid
#define smru_names			smru_mruleuse.mru_names
#define smru_desc			smru_mruleuse.mru_desc
#define smru_obsolete			smru_mruleuse.mru_obsolete
#define smru_applies_oids		smru_mruleuse.mru_applies_oids

#define smru_usage			smru_mr->smr_usage
} /* MatchingRuleUse */ ;

typedef struct slap_mrule_defs_rec {
	char *						mrd_desc;
	slap_mask_t					mrd_usage;
	char **						mrd_compat_syntaxes;
	slap_mr_convert_func *		mrd_convert;
	slap_mr_normalize_func *	mrd_normalize;
	slap_mr_match_func *		mrd_match;
	slap_mr_indexer_func *		mrd_indexer;
	slap_mr_filter_func *		mrd_filter;

	/* For equality rule, this may refer to an associated approximate rule */
	/* For non-equality rule, this may refer to an associated equality rule */
	char *						mrd_associated;
} slap_mrule_defs_rec;

typedef int (AttributeTypeSchemaCheckFN)(
	BackendDB *be,
	Entry *e,
	Attribute *attr,
	const char** text,
	char *textbuf, size_t textlen );

struct AttributeType {
	LDAPAttributeType		sat_atype;
	struct berval			sat_cname;
	AttributeType			*sat_sup;
	AttributeType			**sat_subtypes;
	MatchingRule			*sat_equality;
	MatchingRule			*sat_approx;
	MatchingRule			*sat_ordering;
	MatchingRule			*sat_substr;
	Syntax				*sat_syntax;

	AttributeTypeSchemaCheckFN	*sat_check;
	char				*sat_oidmacro;	/* attribute OID */
	char				*sat_soidmacro;	/* syntax OID */

#define SLAP_AT_NONE			0x0000U
#define SLAP_AT_ABSTRACT		0x0100U /* cannot be instantiated */
#define SLAP_AT_FINAL			0x0200U /* cannot be subtyped */
#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_AT_HIDE			0x0000U /* publish everything */
#else
#define SLAP_AT_HIDE			0x8000U /* hide attribute */
#endif
#define	SLAP_AT_DYNAMIC			0x0400U	/* dynamically generated */

#define SLAP_AT_MANAGEABLE		0x0800U	/* no-user-mod can be by-passed */

/* Note: ORDERED values have an ordering specifically set by the
 * user, denoted by the {x} ordering prefix on the values.
 *
 * SORTED values are simply sorted by memcmp. SORTED values can
 * be efficiently located by binary search. ORDERED values have no
 * such advantage. An attribute cannot have both properties.
 */
#define	SLAP_AT_ORDERED_VAL		0x0001U /* values are ordered */
#define	SLAP_AT_ORDERED_SIB		0x0002U /* siblings are ordered */
#define	SLAP_AT_ORDERED			0x0003U /* value has order index */

#define	SLAP_AT_SORTED_VAL		0x0010U	/* values should be sorted */

#define	SLAP_AT_HARDCODE		0x10000U	/* hardcoded schema */
#define	SLAP_AT_DELETED			0x20000U

	slap_mask_t			sat_flags;

	LDAP_STAILQ_ENTRY(AttributeType) sat_next;

#define sat_oid				sat_atype.at_oid
#define sat_names			sat_atype.at_names
#define sat_desc			sat_atype.at_desc
#define sat_obsolete			sat_atype.at_obsolete
#define sat_sup_oid			sat_atype.at_sup_oid
#define sat_equality_oid		sat_atype.at_equality_oid
#define sat_ordering_oid		sat_atype.at_ordering_oid
#define sat_substr_oid			sat_atype.at_substr_oid
#define sat_syntax_oid			sat_atype.at_syntax_oid
#define sat_single_value		sat_atype.at_single_value
#define sat_collective			sat_atype.at_collective
#define sat_no_user_mod			sat_atype.at_no_user_mod
#define sat_usage			sat_atype.at_usage
#define sat_extensions			sat_atype.at_extensions

	AttributeDescription		*sat_ad;
	ldap_pvt_thread_mutex_t		sat_ad_mutex;
};

#define is_at_operational(at)	((at)->sat_usage)
#define is_at_single_value(at)	((at)->sat_single_value)
#define is_at_collective(at)	((at)->sat_collective)
#define is_at_obsolete(at)		((at)->sat_obsolete)
#define is_at_no_user_mod(at)	((at)->sat_no_user_mod)

typedef int (ObjectClassSchemaCheckFN)(
	BackendDB *be,
	Entry *e,
	ObjectClass *oc,
	const char** text,
	char *textbuf, size_t textlen );

struct ObjectClass {
	LDAPObjectClass			soc_oclass;
	struct berval			soc_cname;
	ObjectClass			**soc_sups;
	AttributeType			**soc_required;
	AttributeType			**soc_allowed;
	ObjectClassSchemaCheckFN	*soc_check;
	char				*soc_oidmacro;
	slap_mask_t			soc_flags;
#define soc_oid				soc_oclass.oc_oid
#define soc_names			soc_oclass.oc_names
#define soc_desc			soc_oclass.oc_desc
#define soc_obsolete			soc_oclass.oc_obsolete
#define soc_sup_oids			soc_oclass.oc_sup_oids
#define soc_kind			soc_oclass.oc_kind
#define soc_at_oids_must		soc_oclass.oc_at_oids_must
#define soc_at_oids_may			soc_oclass.oc_at_oids_may
#define soc_extensions			soc_oclass.oc_extensions

	LDAP_STAILQ_ENTRY(ObjectClass)	soc_next;
};

#define	SLAP_OCF_SET_FLAGS	0x1
#define	SLAP_OCF_CHECK_SUP	0x2
#define	SLAP_OCF_MASK		(SLAP_OCF_SET_FLAGS|SLAP_OCF_CHECK_SUP)

#define	SLAP_OC_ALIAS		0x0001
#define	SLAP_OC_REFERRAL	0x0002
#define	SLAP_OC_SUBENTRY	0x0004
#define	SLAP_OC_DYNAMICOBJECT	0x0008
#define	SLAP_OC_COLLECTIVEATTRIBUTESUBENTRY	0x0010
#define SLAP_OC_GLUE		0x0020
#define SLAP_OC_SYNCPROVIDERSUBENTRY		0x0040
#define SLAP_OC_SYNCCONSUMERSUBENTRY		0x0080
#define	SLAP_OC__MASK		0x00FF
#define	SLAP_OC__END		0x0100
#define SLAP_OC_OPERATIONAL	0x4000
#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_OC_HIDE		0x0000
#else
#define SLAP_OC_HIDE		0x8000
#endif
#define	SLAP_OC_HARDCODE	0x10000U	/* This is hardcoded schema */
#define	SLAP_OC_DELETED		0x20000U

/*
 * DIT content rule
 */
struct ContentRule {
	LDAPContentRule		scr_crule;
	ObjectClass		*scr_sclass;
	ObjectClass		**scr_auxiliaries;	/* optional */
	AttributeType		**scr_required;		/* optional */
	AttributeType		**scr_allowed;		/* optional */
	AttributeType		**scr_precluded;	/* optional */
#define scr_oid			scr_crule.cr_oid
#define scr_names		scr_crule.cr_names
#define scr_desc		scr_crule.cr_desc
#define scr_obsolete		scr_crule.cr_obsolete
#define scr_oc_oids_aux		scr_crule.cr_oc_oids_aux
#define scr_at_oids_must	scr_crule.cr_at_oids_must
#define scr_at_oids_may		scr_crule.cr_at_oids_may
#define scr_at_oids_not		scr_crule.cr_at_oids_not

	char			*scr_oidmacro;
#define	SLAP_CR_HARDCODE	0x10000U
	int			scr_flags;

	LDAP_STAILQ_ENTRY( ContentRule ) scr_next;
};

/* Represents a recognized attribute description ( type + options ). */
struct AttributeDescription {
	AttributeDescription	*ad_next;
	AttributeType		*ad_type;	/* attribute type, must be specified */
	struct berval		ad_cname;	/* canonical name, must be specified */
	struct berval		ad_tags;	/* empty if no tagging options */
	unsigned ad_flags;
#define SLAP_DESC_NONE		0x00U
#define SLAP_DESC_BINARY	0x01U
#define SLAP_DESC_TAG_RANGE	0x80U
#define SLAP_DESC_TEMPORARY	0x1000U
	unsigned ad_index;
};

/* flags to slap_*2undef_ad to register undefined (0, the default)
 * or proxied (SLAP_AD_PROXIED) AttributeDescriptions; the additional
 * SLAP_AD_NOINSERT is to lookup without insert */
#define SLAP_AD_UNDEF		0x00U
#define SLAP_AD_PROXIED		0x01U
#define	SLAP_AD_NOINSERT	0x02U

#define	SLAP_AN_OCEXCLUDE	0x01
#define	SLAP_AN_OCINITED	0x02

struct AttributeName {
	struct berval		an_name;
	AttributeDescription	*an_desc;
	int			an_flags;
	ObjectClass		*an_oc;
};

#define slap_ad_is_tagged(ad)			( (ad)->ad_tags.bv_len != 0 )
#define slap_ad_is_tag_range(ad)	\
	( ((ad)->ad_flags & SLAP_DESC_TAG_RANGE) ? 1 : 0 )
#define slap_ad_is_binary(ad)		\
	( ((ad)->ad_flags & SLAP_DESC_BINARY) ? 1 : 0 )

/*
 * pointers to schema elements used internally
 */
struct slap_internal_schema {
	/* objectClass */
	ObjectClass *si_oc_top;
	ObjectClass *si_oc_extensibleObject;
	ObjectClass *si_oc_alias;
	ObjectClass *si_oc_referral;
	ObjectClass *si_oc_rootdse;
	ObjectClass *si_oc_subentry;
	ObjectClass *si_oc_subschema;
	ObjectClass *si_oc_collectiveAttributeSubentry;
	ObjectClass *si_oc_dynamicObject;

	ObjectClass *si_oc_glue;
	ObjectClass *si_oc_syncConsumerSubentry;
	ObjectClass *si_oc_syncProviderSubentry;

	/* objectClass attribute descriptions */
	AttributeDescription *si_ad_objectClass;

	/* operational attribute descriptions */
	AttributeDescription *si_ad_structuralObjectClass;
	AttributeDescription *si_ad_creatorsName;
	AttributeDescription *si_ad_createTimestamp;
	AttributeDescription *si_ad_modifiersName;
	AttributeDescription *si_ad_modifyTimestamp;
	AttributeDescription *si_ad_hasSubordinates;
	AttributeDescription *si_ad_subschemaSubentry;
	AttributeDescription *si_ad_collectiveSubentries;
	AttributeDescription *si_ad_collectiveExclusions;
	AttributeDescription *si_ad_entryDN;
	AttributeDescription *si_ad_entryUUID;
	AttributeDescription *si_ad_entryCSN;
	AttributeDescription *si_ad_namingCSN;

	AttributeDescription *si_ad_dseType;
	AttributeDescription *si_ad_syncreplCookie;
	AttributeDescription *si_ad_syncTimestamp;
	AttributeDescription *si_ad_contextCSN;

	/* root DSE attribute descriptions */
	AttributeDescription *si_ad_altServer;
	AttributeDescription *si_ad_namingContexts;
	AttributeDescription *si_ad_supportedControl;
	AttributeDescription *si_ad_supportedExtension;
	AttributeDescription *si_ad_supportedLDAPVersion;
	AttributeDescription *si_ad_supportedSASLMechanisms;
	AttributeDescription *si_ad_supportedFeatures;
	AttributeDescription *si_ad_monitorContext;
	AttributeDescription *si_ad_vendorName;
	AttributeDescription *si_ad_vendorVersion;
	AttributeDescription *si_ad_configContext;

	/* subentry attribute descriptions */
	AttributeDescription *si_ad_administrativeRole;
	AttributeDescription *si_ad_subtreeSpecification;

	/* subschema subentry attribute descriptions */
	AttributeDescription *si_ad_attributeTypes;
	AttributeDescription *si_ad_ditContentRules;
	AttributeDescription *si_ad_ditStructureRules;
	AttributeDescription *si_ad_ldapSyntaxes;
	AttributeDescription *si_ad_matchingRules;
	AttributeDescription *si_ad_matchingRuleUse;
	AttributeDescription *si_ad_nameForms;
	AttributeDescription *si_ad_objectClasses;

	/* Aliases & Referrals */
	AttributeDescription *si_ad_aliasedObjectName;
	AttributeDescription *si_ad_ref;

	/* Access Control Internals */
	AttributeDescription *si_ad_entry;
	AttributeDescription *si_ad_children;
	AttributeDescription *si_ad_saslAuthzTo;
	AttributeDescription *si_ad_saslAuthzFrom;

	/* dynamic entries */
	AttributeDescription *si_ad_entryTtl;
	AttributeDescription *si_ad_dynamicSubtrees;

	/* Other attributes descriptions */
	AttributeDescription *si_ad_distinguishedName;
	AttributeDescription *si_ad_name;
	AttributeDescription *si_ad_cn;
	AttributeDescription *si_ad_uid;
	AttributeDescription *si_ad_uidNumber;
	AttributeDescription *si_ad_gidNumber;
	AttributeDescription *si_ad_userPassword;
	AttributeDescription *si_ad_labeledURI;
#ifdef SLAPD_AUTHPASSWD
	AttributeDescription *si_ad_authPassword;
	AttributeDescription *si_ad_authPasswordSchemes;
#endif
	AttributeDescription *si_ad_description;
	AttributeDescription *si_ad_seeAlso;

	/* Undefined Attribute Type */
	AttributeType	*si_at_undefined;

	/* "Proxied" Attribute Type */
	AttributeType	*si_at_proxied;

	/* Matching Rules */
	MatchingRule	*si_mr_distinguishedNameMatch;
	MatchingRule	*si_mr_dnSubtreeMatch;
	MatchingRule	*si_mr_dnOneLevelMatch;
	MatchingRule	*si_mr_dnSubordinateMatch;
	MatchingRule	*si_mr_dnSuperiorMatch;
	MatchingRule    *si_mr_caseExactMatch;
	MatchingRule    *si_mr_caseExactSubstringsMatch;
	MatchingRule    *si_mr_caseExactIA5Match;
	MatchingRule	*si_mr_integerMatch;
	MatchingRule    *si_mr_integerFirstComponentMatch;
	MatchingRule    *si_mr_objectIdentifierFirstComponentMatch;
	MatchingRule    *si_mr_caseIgnoreMatch;
	MatchingRule    *si_mr_caseIgnoreListMatch;

	/* Syntaxes */
	Syntax		*si_syn_directoryString;
	Syntax		*si_syn_distinguishedName;
	Syntax		*si_syn_integer;
	Syntax		*si_syn_octetString;

	/* Schema Syntaxes */
	Syntax		*si_syn_attributeTypeDesc;
	Syntax		*si_syn_ditContentRuleDesc;
	Syntax		*si_syn_ditStructureRuleDesc;
	Syntax		*si_syn_ldapSyntaxDesc;
	Syntax		*si_syn_matchingRuleDesc;
	Syntax		*si_syn_matchingRuleUseDesc;
	Syntax		*si_syn_nameFormDesc;
	Syntax		*si_syn_objectClassDesc;
};

struct AttributeAssertion {
	AttributeDescription	*aa_desc;
	struct berval		aa_value;
#ifdef LDAP_COMP_MATCH
	ComponentFilter		*aa_cf;		/* for attribute aliasing */
#endif
};
#ifdef LDAP_COMP_MATCH
#define ATTRIBUTEASSERTION_INIT { NULL, BER_BVNULL, NULL }
#else
#define ATTRIBUTEASSERTION_INIT { NULL, BER_BVNULL }
#endif

struct SubstringsAssertion {
	AttributeDescription	*sa_desc;
	struct berval		sa_initial;
	struct berval		*sa_any;
	struct berval		sa_final;
};

struct MatchingRuleAssertion {
	AttributeDescription	*ma_desc;	/* optional */
	struct berval		ma_value;	/* required */
	MatchingRule		*ma_rule;	/* optional */
	struct berval		ma_rule_text;	/* optional */
	int			ma_dnattrs;	/* boolean */
#ifdef LDAP_COMP_MATCH
	ComponentFilter		*ma_cf;	/* component filter */
#endif
};

/*
 * represents a search filter
 */
struct Filter {
	ber_tag_t	f_choice;	/* values taken from ldap.h, plus: */
#define SLAPD_FILTER_COMPUTED		0
#define SLAPD_FILTER_MASK			0x7fff
#define SLAPD_FILTER_UNDEFINED		0x8000

	union f_un_u {
		/* precomputed result */
		ber_int_t		f_un_result;

		/* present */
		AttributeDescription	*f_un_desc;

		/* simple value assertion */
		AttributeAssertion	*f_un_ava;

		/* substring assertion */
		SubstringsAssertion	*f_un_ssa;

		/* matching rule assertion */
		MatchingRuleAssertion	*f_un_mra;

#define f_desc			f_un.f_un_desc
#define f_ava			f_un.f_un_ava
#define f_av_desc		f_un.f_un_ava->aa_desc
#define f_av_value		f_un.f_un_ava->aa_value
#define f_sub			f_un.f_un_ssa
#define f_sub_desc		f_un.f_un_ssa->sa_desc
#define f_sub_initial		f_un.f_un_ssa->sa_initial
#define f_sub_any		f_un.f_un_ssa->sa_any
#define f_sub_final		f_un.f_un_ssa->sa_final
#define f_mra			f_un.f_un_mra
#define f_mr_rule		f_un.f_un_mra->ma_rule
#define f_mr_rule_text		f_un.f_un_mra->ma_rule_text
#define f_mr_desc		f_un.f_un_mra->ma_desc
#define f_mr_value		f_un.f_un_mra->ma_value
#define	f_mr_dnattrs		f_un.f_un_mra->ma_dnattrs

		/* and, or, not */
		Filter			*f_un_complex;
	} f_un;

#define f_result	f_un.f_un_result
#define f_and		f_un.f_un_complex
#define f_or		f_un.f_un_complex
#define f_not		f_un.f_un_complex
#define f_list		f_un.f_un_complex

	Filter		*f_next;
};

/* compare routines can return undefined */
#define SLAPD_COMPARE_UNDEFINED	((ber_int_t) -1)

struct ValuesReturnFilter {
	ber_tag_t	vrf_choice;

	union vrf_un_u {
		/* precomputed result */
		ber_int_t vrf_un_result;

		/* DN */
		char *vrf_un_dn;

		/* present */
		AttributeDescription *vrf_un_desc;

		/* simple value assertion */
		AttributeAssertion *vrf_un_ava;

		/* substring assertion */
		SubstringsAssertion *vrf_un_ssa;

		/* matching rule assertion */
		MatchingRuleAssertion *vrf_un_mra;

#define vrf_result		vrf_un.vrf_un_result
#define vrf_dn			vrf_un.vrf_un_dn
#define vrf_desc		vrf_un.vrf_un_desc
#define vrf_ava			vrf_un.vrf_un_ava
#define vrf_av_desc		vrf_un.vrf_un_ava->aa_desc
#define vrf_av_value	vrf_un.vrf_un_ava->aa_value
#define vrf_ssa			vrf_un.vrf_un_ssa
#define vrf_sub			vrf_un.vrf_un_ssa
#define vrf_sub_desc	vrf_un.vrf_un_ssa->sa_desc
#define vrf_sub_initial	vrf_un.vrf_un_ssa->sa_initial
#define vrf_sub_any		vrf_un.vrf_un_ssa->sa_any
#define vrf_sub_final	vrf_un.vrf_un_ssa->sa_final
#define vrf_mra			vrf_un.vrf_un_mra
#define vrf_mr_rule		vrf_un.vrf_un_mra->ma_rule
#define vrf_mr_rule_text	vrf_un.vrf_un_mra->ma_rule_text
#define vrf_mr_desc		vrf_un.vrf_un_mra->ma_desc
#define vrf_mr_value		vrf_un.vrf_un_mra->ma_value
#define	vrf_mr_dnattrs	vrf_un.vrf_un_mra->ma_dnattrs


	} vrf_un;

	ValuesReturnFilter	*vrf_next;
};

/*
 * represents an attribute (description + values)
 * desc, vals, nvals, numvals fields must align with Modification
 */
struct Attribute {
	AttributeDescription	*a_desc;
	BerVarray		a_vals;		/* preserved values */
	BerVarray		a_nvals;	/* normalized values */
	unsigned		a_numvals;	/* number of vals */
	unsigned		a_flags;
#define SLAP_ATTR_IXADD			0x1U
#define SLAP_ATTR_IXDEL			0x2U
#define SLAP_ATTR_DONT_FREE_DATA	0x4U
#define SLAP_ATTR_DONT_FREE_VALS	0x8U
#define	SLAP_ATTR_SORTED_VALS		0x10U	/* values are sorted */

/* These flags persist across an attr_dup() */
#define	SLAP_ATTR_PERSISTENT_FLAGS \
	SLAP_ATTR_SORTED_VALS

	Attribute		*a_next;
#ifdef LDAP_COMP_MATCH
	ComponentData		*a_comp_data;	/* component values */
#endif
};


/*
 * the id used in the indexes to refer to an entry
 */
typedef unsigned long	ID;
#define NOID	((ID)~0)

typedef struct EntryHeader {
	struct berval bv;
	char *data;
	int nattrs;
	int nvals;
} EntryHeader;

/*
 * represents an entry in core
 */
struct Entry {
	/*
	 * The ID field should only be changed before entry is
	 * inserted into a cache.  The ID value is backend
	 * specific.
	 */
	ID		e_id;

	struct berval e_name;	/* name (DN) of this entry */
	struct berval e_nname;	/* normalized name (DN) of this entry */

	/* for migration purposes */
#define e_dn e_name.bv_val
#define e_ndn e_nname.bv_val

	Attribute	*e_attrs;	/* list of attributes + values */

	slap_mask_t	e_ocflags;

	struct berval	e_bv;		/* For entry_encode/entry_decode */

	/* for use by the backend for any purpose */
	void*	e_private;
};

/*
 * A list of LDAPMods
 * desc, values, nvalues, numvals must align with Attribute
 */
struct Modification {
	AttributeDescription *sm_desc;
	BerVarray sm_values;
	BerVarray sm_nvalues;
	unsigned sm_numvals;
	short sm_op;
	short sm_flags;
/* Set for internal mods, will bypass ACL checks. Only needed when
 * running as non-root user, for user modifiable attributes.
 */
#define	SLAP_MOD_INTERNAL	0x01
#define	SLAP_MOD_MANAGING	0x02
	struct berval sm_type;
};

struct Modifications {
	Modification	sml_mod;
#define sml_op		sml_mod.sm_op
#define sml_flags	sml_mod.sm_flags
#define sml_desc	sml_mod.sm_desc
#define	sml_type	sml_mod.sm_type
#define sml_values	sml_mod.sm_values
#define sml_nvalues	sml_mod.sm_nvalues
#define sml_numvals	sml_mod.sm_numvals
	Modifications	*sml_next;
};

/*
 * represents an access control list
 */
typedef enum slap_access_t {
	ACL_INVALID_ACCESS = -1,
	ACL_NONE = 0,
	ACL_DISCLOSE,
	ACL_AUTH,
	ACL_COMPARE,
	ACL_SEARCH,
	ACL_READ,
	ACL_WRITE_,
	ACL_MANAGE,

	/* always leave at end of levels but not greater than ACL_LEVEL_MASK */
	ACL_LAST,

	/* ACL level mask and modifiers */
	ACL_LEVEL_MASK = 0x000f,
	ACL_QUALIFIER1 = 0x0100,
	ACL_QUALIFIER2 = 0x0200,
	ACL_QUALIFIER3 = 0x0400,
	ACL_QUALIFIER4 = 0x0800,
	ACL_QUALIFIER_MASK = 0x0f00,

	/* write granularity */
	ACL_WADD = ACL_WRITE_|ACL_QUALIFIER1,
	ACL_WDEL = ACL_WRITE_|ACL_QUALIFIER2,

	ACL_WRITE = ACL_WADD|ACL_WDEL
} slap_access_t;

typedef enum slap_control_e {
	ACL_INVALID_CONTROL	= 0,
	ACL_STOP,
	ACL_CONTINUE,
	ACL_BREAK
} slap_control_t;

typedef enum slap_style_e {
	ACL_STYLE_REGEX = 0,
	ACL_STYLE_EXPAND,
	ACL_STYLE_BASE,
	ACL_STYLE_ONE,
	ACL_STYLE_SUBTREE,
	ACL_STYLE_CHILDREN,
	ACL_STYLE_LEVEL,
	ACL_STYLE_ATTROF,
	ACL_STYLE_ANONYMOUS,
	ACL_STYLE_USERS,
	ACL_STYLE_SELF,
	ACL_STYLE_IP,
	ACL_STYLE_IPV6,
	ACL_STYLE_PATH,

	ACL_STYLE_NONE
} slap_style_t;

typedef struct AuthorizationInformation {
	ber_tag_t	sai_method;			/* LDAP_AUTH_* from <ldap.h> */
	struct berval	sai_mech;		/* SASL Mechanism */
	struct berval	sai_dn;			/* DN for reporting purposes */
	struct berval	sai_ndn;		/* Normalized DN */

	/* Security Strength Factors */
	slap_ssf_t	sai_ssf;			/* Overall SSF */
	slap_ssf_t	sai_transport_ssf;	/* Transport SSF */
	slap_ssf_t	sai_tls_ssf;		/* TLS SSF */
	slap_ssf_t	sai_sasl_ssf;		/* SASL SSF */
} AuthorizationInformation;

#ifdef SLAP_DYNACL

/*
 * "dynamic" ACL infrastructure (for ACIs and more)
 */
typedef int (slap_dynacl_parse) LDAP_P(( const char *fname, int lineno,
	const char *opts, slap_style_t, const char *, void **privp ));
typedef int (slap_dynacl_unparse) LDAP_P(( void *priv, struct berval *bv ));
typedef int (slap_dynacl_mask) LDAP_P((
		void			*priv,
		Operation		*op,
		Entry			*e,
		AttributeDescription	*desc,
		struct berval		*val,
		int			nmatch,
		regmatch_t		*matches,
		slap_access_t		*grant,
		slap_access_t		*deny ));
typedef int (slap_dynacl_destroy) LDAP_P(( void *priv ));

typedef struct slap_dynacl_t {
	char			*da_name;
	slap_dynacl_parse	*da_parse;
	slap_dynacl_unparse	*da_unparse;
	slap_dynacl_mask	*da_mask;
	slap_dynacl_destroy	*da_destroy;
	
	void			*da_private;
	struct slap_dynacl_t	*da_next;
} slap_dynacl_t;
#endif /* SLAP_DYNACL */

/* the DN portion of the "by" part */
typedef struct slap_dn_access {
	/* DN pattern */
	AuthorizationInformation	a_dnauthz;
#define	a_pat			a_dnauthz.sai_dn

	slap_style_t		a_style;
	int			a_level;
	int			a_self_level;
	AttributeDescription	*a_at;
	int			a_self;
	int 			a_expand;
} slap_dn_access;

/* the "by" part */
typedef struct Access {
	slap_control_t a_type;

/* strip qualifiers */
#define ACL_LEVEL(p)			((p) & ACL_LEVEL_MASK)
#define ACL_QUALIFIERS(p)		((p) & ~ACL_LEVEL_MASK)

#define ACL_ACCESS2PRIV(access)		((0x01U << ACL_LEVEL((access))) | ACL_QUALIFIERS((access)))

#define ACL_PRIV_NONE			ACL_ACCESS2PRIV( ACL_NONE )
#define ACL_PRIV_DISCLOSE		ACL_ACCESS2PRIV( ACL_DISCLOSE )
#define ACL_PRIV_AUTH			ACL_ACCESS2PRIV( ACL_AUTH )
#define ACL_PRIV_COMPARE		ACL_ACCESS2PRIV( ACL_COMPARE )
#define ACL_PRIV_SEARCH			ACL_ACCESS2PRIV( ACL_SEARCH )
#define ACL_PRIV_READ			ACL_ACCESS2PRIV( ACL_READ )
#define ACL_PRIV_WADD			ACL_ACCESS2PRIV( ACL_WADD )
#define ACL_PRIV_WDEL			ACL_ACCESS2PRIV( ACL_WDEL )
#define ACL_PRIV_WRITE			( ACL_PRIV_WADD | ACL_PRIV_WDEL )
#define ACL_PRIV_MANAGE			ACL_ACCESS2PRIV( ACL_MANAGE )

/* NOTE: always use the highest level; current: 0x00ffUL */
#define ACL_PRIV_MASK			((ACL_ACCESS2PRIV(ACL_LAST) - 1) | ACL_QUALIFIER_MASK)

/* priv flags */
#define ACL_PRIV_LEVEL			0x1000UL
#define ACL_PRIV_ADDITIVE		0x2000UL
#define ACL_PRIV_SUBSTRACTIVE		0x4000UL

/* invalid privs */
#define ACL_PRIV_INVALID		0x0UL

#define ACL_PRIV_ISSET(m,p)		(((m) & (p)) == (p))
#define ACL_PRIV_ASSIGN(m,p)		do { (m)  =  (p); } while(0)
#define ACL_PRIV_SET(m,p)		do { (m) |=  (p); } while(0)
#define ACL_PRIV_CLR(m,p)		do { (m) &= ~(p); } while(0)

#define ACL_INIT(m)			ACL_PRIV_ASSIGN((m), ACL_PRIV_NONE)
#define ACL_INVALIDATE(m)		ACL_PRIV_ASSIGN((m), ACL_PRIV_INVALID)

#define ACL_GRANT(m,a)			ACL_PRIV_ISSET((m),ACL_ACCESS2PRIV(a))

#define ACL_IS_INVALID(m)		((m) == ACL_PRIV_INVALID)

#define ACL_IS_LEVEL(m)			ACL_PRIV_ISSET((m),ACL_PRIV_LEVEL)
#define ACL_IS_ADDITIVE(m)		ACL_PRIV_ISSET((m),ACL_PRIV_ADDITIVE)
#define ACL_IS_SUBTRACTIVE(m)		ACL_PRIV_ISSET((m),ACL_PRIV_SUBSTRACTIVE)

#define ACL_LVL_NONE			(ACL_PRIV_NONE|ACL_PRIV_LEVEL)
#define ACL_LVL_DISCLOSE		(ACL_PRIV_DISCLOSE|ACL_LVL_NONE)
#define ACL_LVL_AUTH			(ACL_PRIV_AUTH|ACL_LVL_DISCLOSE)
#define ACL_LVL_COMPARE			(ACL_PRIV_COMPARE|ACL_LVL_AUTH)
#define ACL_LVL_SEARCH			(ACL_PRIV_SEARCH|ACL_LVL_COMPARE)
#define ACL_LVL_READ			(ACL_PRIV_READ|ACL_LVL_SEARCH)
#define ACL_LVL_WADD			(ACL_PRIV_WADD|ACL_LVL_READ)
#define ACL_LVL_WDEL			(ACL_PRIV_WDEL|ACL_LVL_READ)
#define ACL_LVL_WRITE			(ACL_PRIV_WRITE|ACL_LVL_READ)
#define ACL_LVL_MANAGE			(ACL_PRIV_MANAGE|ACL_LVL_WRITE)

#define ACL_LVL(m,l)			(((m)&ACL_PRIV_MASK) == ((l)&ACL_PRIV_MASK))
#define ACL_LVL_IS_NONE(m)		ACL_LVL((m),ACL_LVL_NONE)
#define ACL_LVL_IS_DISCLOSE(m)		ACL_LVL((m),ACL_LVL_DISCLOSE)
#define ACL_LVL_IS_AUTH(m)		ACL_LVL((m),ACL_LVL_AUTH)
#define ACL_LVL_IS_COMPARE(m)		ACL_LVL((m),ACL_LVL_COMPARE)
#define ACL_LVL_IS_SEARCH(m)		ACL_LVL((m),ACL_LVL_SEARCH)
#define ACL_LVL_IS_READ(m)		ACL_LVL((m),ACL_LVL_READ)
#define ACL_LVL_IS_WADD(m)		ACL_LVL((m),ACL_LVL_WADD)
#define ACL_LVL_IS_WDEL(m)		ACL_LVL((m),ACL_LVL_WDEL)
#define ACL_LVL_IS_WRITE(m)		ACL_LVL((m),ACL_LVL_WRITE)
#define ACL_LVL_IS_MANAGE(m)		ACL_LVL((m),ACL_LVL_MANAGE)

#define ACL_LVL_ASSIGN_NONE(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_NONE)
#define ACL_LVL_ASSIGN_DISCLOSE(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_DISCLOSE)
#define ACL_LVL_ASSIGN_AUTH(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_AUTH)
#define ACL_LVL_ASSIGN_COMPARE(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_COMPARE)
#define ACL_LVL_ASSIGN_SEARCH(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_SEARCH)
#define ACL_LVL_ASSIGN_READ(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_READ)
#define ACL_LVL_ASSIGN_WADD(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_WADD)
#define ACL_LVL_ASSIGN_WDEL(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_WDEL)
#define ACL_LVL_ASSIGN_WRITE(m)		ACL_PRIV_ASSIGN((m),ACL_LVL_WRITE)
#define ACL_LVL_ASSIGN_MANAGE(m)	ACL_PRIV_ASSIGN((m),ACL_LVL_MANAGE)

	slap_mask_t	a_access_mask;

	/* DN pattern */
	slap_dn_access		a_dn;
#define a_dn_pat		a_dn.a_dnauthz.sai_dn
#define	a_dn_at			a_dn.a_at
#define	a_dn_self		a_dn.a_self

	/* real DN pattern */
	slap_dn_access		a_realdn;
#define a_realdn_pat		a_realdn.a_dnauthz.sai_dn
#define	a_realdn_at		a_realdn.a_at
#define	a_realdn_self		a_realdn.a_self

	/* used for ssf stuff
	 * NOTE: the ssf stuff in a_realdn is ignored */
#define	a_authz			a_dn.a_dnauthz

	/* connection related stuff */
	slap_style_t a_peername_style;
	struct berval	a_peername_pat;
#ifdef LDAP_PF_INET6
	union {
		struct in6_addr	ax6;
		unsigned long	ax;
	}	ax_peername_addr,
		ax_peername_mask;
#define	a_peername_addr6	ax_peername_addr.ax6
#define	a_peername_addr		ax_peername_addr.ax
#define	a_peername_mask6	ax_peername_mask.ax6
#define	a_peername_mask		ax_peername_mask.ax
/* apparently, only s6_addr is portable;
 * define a portable address mask comparison */
#define	slap_addr6_mask(val, msk, asr) ( \
	(((val)->s6_addr[0] & (msk)->s6_addr[0]) == (asr)->s6_addr[0]) \
	&& (((val)->s6_addr[1] & (msk)->s6_addr[1]) == (asr)->s6_addr[1]) \
	&& (((val)->s6_addr[2] & (msk)->s6_addr[2]) == (asr)->s6_addr[2]) \
	&& (((val)->s6_addr[3] & (msk)->s6_addr[3]) == (asr)->s6_addr[3]) \
	&& (((val)->s6_addr[4] & (msk)->s6_addr[4]) == (asr)->s6_addr[4]) \
	&& (((val)->s6_addr[5] & (msk)->s6_addr[5]) == (asr)->s6_addr[5]) \
	&& (((val)->s6_addr[6] & (msk)->s6_addr[6]) == (asr)->s6_addr[6]) \
	&& (((val)->s6_addr[7] & (msk)->s6_addr[7]) == (asr)->s6_addr[7]) \
	&& (((val)->s6_addr[8] & (msk)->s6_addr[8]) == (asr)->s6_addr[8]) \
	&& (((val)->s6_addr[9] & (msk)->s6_addr[9]) == (asr)->s6_addr[9]) \
	&& (((val)->s6_addr[10] & (msk)->s6_addr[10]) == (asr)->s6_addr[10]) \
	&& (((val)->s6_addr[11] & (msk)->s6_addr[11]) == (asr)->s6_addr[11]) \
	&& (((val)->s6_addr[12] & (msk)->s6_addr[12]) == (asr)->s6_addr[12]) \
	&& (((val)->s6_addr[13] & (msk)->s6_addr[13]) == (asr)->s6_addr[13]) \
	&& (((val)->s6_addr[14] & (msk)->s6_addr[14]) == (asr)->s6_addr[14]) \
	&& (((val)->s6_addr[15] & (msk)->s6_addr[15]) == (asr)->s6_addr[15]) \
	)
#else /* ! LDAP_PF_INET6 */
	unsigned long	a_peername_addr,
			a_peername_mask;
#endif /* ! LDAP_PF_INET6 */
	int		a_peername_port;

	slap_style_t a_sockname_style;
	struct berval	a_sockname_pat;

	slap_style_t a_domain_style;
	struct berval	a_domain_pat;
	int		a_domain_expand;

	slap_style_t a_sockurl_style;
	struct berval	a_sockurl_pat;
	slap_style_t a_set_style;
	struct berval	a_set_pat;

#ifdef SLAP_DYNACL
	slap_dynacl_t		*a_dynacl;
#endif /* SLAP_DYNACL */

	/* ACL Groups */
	slap_style_t a_group_style;
	struct berval	a_group_pat;
	ObjectClass		*a_group_oc;
	AttributeDescription	*a_group_at;

	struct Access		*a_next;
} Access;

/* the "to" part */
typedef struct AccessControl {
	/* "to" part: the entries this acl applies to */
	Filter		*acl_filter;
	slap_style_t acl_dn_style;
	regex_t		acl_dn_re;
	struct berval	acl_dn_pat;
	AttributeName	*acl_attrs;
	MatchingRule	*acl_attrval_mr;
	slap_style_t	acl_attrval_style;
	regex_t		acl_attrval_re;
	struct berval	acl_attrval;

	/* "by" part: list of who has what access to the entries */
	Access	*acl_access;

	struct AccessControl	*acl_next;
} AccessControl;

typedef struct AccessControlState {
	/* Access state */

	/* The stored state is valid when requesting as_access access
	 * to the as_desc attributes.	 */
	AttributeDescription *as_desc;
	slap_access_t	as_access;

	/* Value dependent acl where processing can restart */
	AccessControl  *as_vd_acl;
	int as_vd_acl_present;
	int as_vd_acl_count;
	slap_mask_t		as_vd_mask;

	/* The cached result after evaluating a value independent attr.
	 * Only valid when != -1 and as_vd_acl == NULL */
	int as_result;

	/* True if started to process frontend ACLs */
	int as_fe_done;
} AccessControlState;
#define ACL_STATE_INIT { NULL, ACL_NONE, NULL, 0, 0, ACL_PRIV_NONE, -1, 0 }

typedef struct AclRegexMatches {        
	int dn_count;
        regmatch_t dn_data[MAXREMATCHES];
	int val_count;
        regmatch_t val_data[MAXREMATCHES];
} AclRegexMatches;

/*
 * Backend-info
 * represents a backend 
 */

typedef LDAP_STAILQ_HEAD(BeI, BackendInfo) slap_bi_head;
typedef LDAP_STAILQ_HEAD(BeDB, BackendDB) slap_be_head;

LDAP_SLAPD_V (int) nBackendInfo;
LDAP_SLAPD_V (int) nBackendDB;
LDAP_SLAPD_V (slap_bi_head) backendInfo;
LDAP_SLAPD_V (slap_be_head) backendDB;
LDAP_SLAPD_V (BackendDB *) frontendDB;

LDAP_SLAPD_V (int) slapMode;	
#define SLAP_UNDEFINED_MODE	0x0000
#define SLAP_SERVER_MODE	0x0001
#define SLAP_TOOL_MODE		0x0002
#define SLAP_MODE			0x0003

#define SLAP_TRUNCATE_MODE	0x0100
#define	SLAP_TOOL_READMAIN	0x0200
#define	SLAP_TOOL_READONLY	0x0400
#define	SLAP_TOOL_QUICK		0x0800
#define SLAP_TOOL_NO_SCHEMA_CHECK	0x1000
#define SLAP_TOOL_VALUE_CHECK	0x2000

#define SLAP_SERVER_RUNNING	0x8000

#define SB_TLS_DEFAULT		(-1)
#define SB_TLS_OFF		0
#define SB_TLS_ON		1
#define SB_TLS_CRITICAL		2

typedef struct slap_keepalive {
	int sk_idle;
	int sk_probes;
	int sk_interval;
} slap_keepalive;

typedef struct slap_bindconf {
	struct berval sb_uri;
	int sb_version;
	int sb_tls;
	int sb_method;
	int sb_timeout_api;
	int sb_timeout_net;
	struct berval sb_binddn;
	struct berval sb_cred;
	struct berval sb_saslmech;
	char *sb_secprops;
	struct berval sb_realm;
	struct berval sb_authcId;
	struct berval sb_authzId;
	slap_keepalive sb_keepalive;
#ifdef HAVE_TLS
	void *sb_tls_ctx;
	char *sb_tls_cert;
	char *sb_tls_key;
	char *sb_tls_cacert;
	char *sb_tls_cacertdir;
	char *sb_tls_reqcert;
	char *sb_tls_cipher_suite;
	char *sb_tls_protocol_min;
#ifdef HAVE_OPENSSL_CRL
	char *sb_tls_crlcheck;
#endif
	int sb_tls_do_init;
#endif
} slap_bindconf;

typedef struct slap_verbmasks {
	struct berval word;
	const slap_mask_t mask;
} slap_verbmasks;

typedef struct slap_cf_aux_table {
	struct berval key;
	int off;
	char type;
	char quote;
	void *aux;
} slap_cf_aux_table;

typedef int 
slap_cf_aux_table_parse_x LDAP_P((
	struct berval *val,
	void *bc,
	slap_cf_aux_table *tab0,
	const char *tabmsg,
	int unparse ));

#define SLAP_LIMIT_TIME	1
#define SLAP_LIMIT_SIZE	2

struct slap_limits_set {
	/* time limits */
	int	lms_t_soft;
	int	lms_t_hard;

	/* size limits */
	int	lms_s_soft;
	int	lms_s_hard;
	int	lms_s_unchecked;
	int	lms_s_pr;
	int	lms_s_pr_hide;
	int	lms_s_pr_total;
};

/* Note: this is different from LDAP_NO_LIMIT (0); slapd internal use only */
#define SLAP_NO_LIMIT			-1
#define SLAP_MAX_LIMIT			2147483647

struct slap_limits {
	unsigned		lm_flags;	/* type of pattern */
	/* Values must match lmpats[] in limits.c */
#define SLAP_LIMITS_UNDEFINED		0x0000U
#define SLAP_LIMITS_EXACT		0x0001U
#define SLAP_LIMITS_BASE		SLAP_LIMITS_EXACT
#define SLAP_LIMITS_ONE			0x0002U
#define SLAP_LIMITS_SUBTREE		0x0003U
#define SLAP_LIMITS_CHILDREN		0x0004U
#define SLAP_LIMITS_REGEX		0x0005U
#define SLAP_LIMITS_ANONYMOUS		0x0006U
#define SLAP_LIMITS_USERS		0x0007U
#define SLAP_LIMITS_ANY			0x0008U
#define SLAP_LIMITS_MASK		0x000FU

#define SLAP_LIMITS_TYPE_SELF		0x0000U
#define SLAP_LIMITS_TYPE_DN		SLAP_LIMITS_TYPE_SELF
#define SLAP_LIMITS_TYPE_GROUP		0x0010U
#define SLAP_LIMITS_TYPE_THIS		0x0020U
#define SLAP_LIMITS_TYPE_MASK		0x00F0U

	regex_t			lm_regex;	/* regex data for REGEX */

	/*
	 * normalized DN for EXACT, BASE, ONE, SUBTREE, CHILDREN;
	 * pattern for REGEX; NULL for ANONYMOUS, USERS
	 */
	struct berval		lm_pat;

	/* if lm_flags & SLAP_LIMITS_TYPE_MASK == SLAP_LIMITS_GROUP,
	 * lm_group_oc is objectClass and lm_group_at is attributeType
	 * of member in oc for match; then lm_flags & SLAP_LIMITS_MASK
	 * can only be SLAP_LIMITS_EXACT */
	ObjectClass		*lm_group_oc;
	AttributeDescription	*lm_group_ad;

	struct slap_limits_set	lm_limits;
};

/* temporary aliases */
typedef BackendDB Backend;
#define nbackends nBackendDB
#define backends backendDB

/*
 * syncinfo structure for syncrepl
 */

struct syncinfo_s;

#define SLAP_SYNC_RID_MAX	999
#define SLAP_SYNC_SID_MAX	4095	/* based on liblutil/csn.c field width */

/* fake conn connid constructed as rid; real connids start
 * at SLAPD_SYNC_CONN_OFFSET */
#define SLAPD_SYNC_SYNCCONN_OFFSET (SLAP_SYNC_RID_MAX + 1)
#define SLAPD_SYNC_IS_SYNCCONN(connid) ((connid) < SLAPD_SYNC_SYNCCONN_OFFSET)
#define SLAPD_SYNC_RID2SYNCCONN(rid) (rid)

#define SLAP_SYNCUUID_SET_SIZE 256

struct sync_cookie {
	BerVarray ctxcsn;
	int *sids;
	int numcsns;
	int rid;
	struct berval octet_str;
	int sid;
	LDAP_STAILQ_ENTRY(sync_cookie) sc_next;
};

LDAP_STAILQ_HEAD( slap_sync_cookie_s, sync_cookie );

LDAP_TAILQ_HEAD( be_pcl, slap_csn_entry );

#ifndef SLAP_MAX_CIDS
#define	SLAP_MAX_CIDS	32	/* Maximum number of supported controls */
#endif

struct ConfigOCs;	/* config.h */

struct BackendDB {
	BackendInfo	*bd_info;	/* pointer to shared backend info */
	BackendDB	*bd_self;	/* pointer to this struct */

	/* fields in this structure (and routines acting on this structure)
	   should be renamed from be_ to bd_ */

	/* BackendInfo accessors */
#define		be_config	bd_info->bi_db_config
#define		be_type		bd_info->bi_type

#define		be_bind		bd_info->bi_op_bind
#define		be_unbind	bd_info->bi_op_unbind
#define		be_add		bd_info->bi_op_add
#define		be_compare	bd_info->bi_op_compare
#define		be_delete	bd_info->bi_op_delete
#define		be_modify	bd_info->bi_op_modify
#define		be_modrdn	bd_info->bi_op_modrdn
#define		be_search	bd_info->bi_op_search
#define		be_abandon	bd_info->bi_op_abandon

#define		be_extended	bd_info->bi_extended
#define		be_cancel	bd_info->bi_op_cancel

#define		be_chk_referrals	bd_info->bi_chk_referrals
#define		be_chk_controls		bd_info->bi_chk_controls
#define		be_fetch	bd_info->bi_entry_get_rw
#define		be_release	bd_info->bi_entry_release_rw
#define		be_group	bd_info->bi_acl_group
#define		be_attribute	bd_info->bi_acl_attribute
#define		be_operational	bd_info->bi_operational

/*
 * define to honor hasSubordinates operational attribute in search filters
 * (in previous use there was a flaw with back-bdb; now it is fixed).
 */
#define		be_has_subordinates bd_info->bi_has_subordinates

#define		be_connection_init	bd_info->bi_connection_init
#define		be_connection_destroy	bd_info->bi_connection_destroy

#ifdef SLAPD_TOOLS
#define		be_entry_open bd_info->bi_tool_entry_open
#define		be_entry_close bd_info->bi_tool_entry_close
#define		be_entry_first bd_info->bi_tool_entry_first
#define		be_entry_first_x bd_info->bi_tool_entry_first_x
#define		be_entry_next bd_info->bi_tool_entry_next
#define		be_entry_reindex bd_info->bi_tool_entry_reindex
#define		be_entry_get bd_info->bi_tool_entry_get
#define		be_entry_put bd_info->bi_tool_entry_put
#define		be_sync bd_info->bi_tool_sync
#define		be_dn2id_get bd_info->bi_tool_dn2id_get
#define		be_entry_modify	bd_info->bi_tool_entry_modify
#endif

	/* supported controls */
	/* note: set to 0 if the database does not support the control;
	 * be_ctrls[SLAP_MAX_CIDS] is set to 1 if initialized */
	char		be_ctrls[SLAP_MAX_CIDS + 1];

/* Database flags */
#define SLAP_DBFLAG_NOLASTMOD		0x0001U
#define SLAP_DBFLAG_NO_SCHEMA_CHECK	0x0002U
#define	SLAP_DBFLAG_HIDDEN		0x0004U
#define	SLAP_DBFLAG_ONE_SUFFIX		0x0008U
#define	SLAP_DBFLAG_GLUE_INSTANCE	0x0010U	/* a glue backend */
#define	SLAP_DBFLAG_GLUE_SUBORDINATE	0x0020U	/* child of a glue hierarchy */
#define	SLAP_DBFLAG_GLUE_LINKED		0x0040U	/* child is connected to parent */
#define SLAP_DBFLAG_GLUE_ADVERTISE	0x0080U /* advertise in rootDSE */
#define SLAP_DBFLAG_OVERLAY		0x0100U	/* this db struct is an overlay */
#define	SLAP_DBFLAG_GLOBAL_OVERLAY	0x0200U	/* this db struct is a global overlay */
#define SLAP_DBFLAG_DYNAMIC		0x0400U /* this db allows dynamicObjects */
#define	SLAP_DBFLAG_MONITORING		0x0800U	/* custom monitoring enabled */
#define SLAP_DBFLAG_SHADOW		0x8000U /* a shadow */
#define SLAP_DBFLAG_SINGLE_SHADOW	0x4000U	/* a single-master shadow */
#define SLAP_DBFLAG_SYNC_SHADOW		0x1000U /* a sync shadow */
#define SLAP_DBFLAG_SLURP_SHADOW	0x2000U /* a slurp shadow */
#define SLAP_DBFLAG_SHADOW_MASK		(SLAP_DBFLAG_SHADOW|SLAP_DBFLAG_SINGLE_SHADOW|SLAP_DBFLAG_SYNC_SHADOW|SLAP_DBFLAG_SLURP_SHADOW)
#define SLAP_DBFLAG_CLEAN		0x10000U /* was cleanly shutdown */
#define SLAP_DBFLAG_ACL_ADD		0x20000U /* check attr ACLs on adds */
#define SLAP_DBFLAG_SYNC_SUBENTRY	0x40000U /* use subentry for context */
#define SLAP_DBFLAG_MULTI_SHADOW	0x80000U /* uses mirrorMode/multi-master */
#define SLAP_DBFLAG_DISABLED	0x100000U
	slap_mask_t	be_flags;
#define SLAP_DBFLAGS(be)			((be)->be_flags)
#define SLAP_NOLASTMOD(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_NOLASTMOD)
#define SLAP_LASTMOD(be)			(!SLAP_NOLASTMOD(be))
#define SLAP_DBHIDDEN(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_HIDDEN)
#define SLAP_DBDISABLED(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_DISABLED)
#define SLAP_DB_ONE_SUFFIX(be)		(SLAP_DBFLAGS(be) & SLAP_DBFLAG_ONE_SUFFIX)
#define SLAP_ISOVERLAY(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_OVERLAY)
#define SLAP_ISGLOBALOVERLAY(be)		(SLAP_DBFLAGS(be) & SLAP_DBFLAG_GLOBAL_OVERLAY)
#define SLAP_DBMONITORING(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_MONITORING)
#define SLAP_NO_SCHEMA_CHECK(be)	\
	(SLAP_DBFLAGS(be) & SLAP_DBFLAG_NO_SCHEMA_CHECK)
#define	SLAP_GLUE_INSTANCE(be)		\
	(SLAP_DBFLAGS(be) & SLAP_DBFLAG_GLUE_INSTANCE)
#define	SLAP_GLUE_SUBORDINATE(be)	\
	(SLAP_DBFLAGS(be) & SLAP_DBFLAG_GLUE_SUBORDINATE)
#define	SLAP_GLUE_LINKED(be)		\
	(SLAP_DBFLAGS(be) & SLAP_DBFLAG_GLUE_LINKED)
#define	SLAP_GLUE_ADVERTISE(be)	\
	(SLAP_DBFLAGS(be) & SLAP_DBFLAG_GLUE_ADVERTISE)
#define SLAP_SHADOW(be)				(SLAP_DBFLAGS(be) & SLAP_DBFLAG_SHADOW)
#define SLAP_SYNC_SHADOW(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_SYNC_SHADOW)
#define SLAP_SLURP_SHADOW(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_SLURP_SHADOW)
#define SLAP_SINGLE_SHADOW(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_SINGLE_SHADOW)
#define SLAP_MULTIMASTER(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_MULTI_SHADOW)
#define SLAP_DBCLEAN(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_CLEAN)
#define SLAP_DBACL_ADD(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_ACL_ADD)
#define SLAP_SYNC_SUBENTRY(be)			(SLAP_DBFLAGS(be) & SLAP_DBFLAG_SYNC_SUBENTRY)

	slap_mask_t	be_restrictops;		/* restriction operations */
#define SLAP_RESTRICT_OP_ADD		0x0001U
#define	SLAP_RESTRICT_OP_BIND		0x0002U
#define SLAP_RESTRICT_OP_COMPARE	0x0004U
#define SLAP_RESTRICT_OP_DELETE		0x0008U
#define	SLAP_RESTRICT_OP_EXTENDED	0x0010U
#define SLAP_RESTRICT_OP_MODIFY		0x0020U
#define SLAP_RESTRICT_OP_RENAME		0x0040U
#define SLAP_RESTRICT_OP_SEARCH		0x0080U
#define SLAP_RESTRICT_OP_MASK		0x00FFU

#define	SLAP_RESTRICT_READONLY		0x80000000U

#define SLAP_RESTRICT_EXOP_START_TLS		0x0100U
#define	SLAP_RESTRICT_EXOP_MODIFY_PASSWD	0x0200U
#define SLAP_RESTRICT_EXOP_WHOAMI		0x0400U
#define SLAP_RESTRICT_EXOP_CANCEL		0x0800U
#define SLAP_RESTRICT_EXOP_MASK			0xFF00U

#define SLAP_RESTRICT_OP_READS	\
	( SLAP_RESTRICT_OP_COMPARE	\
	| SLAP_RESTRICT_OP_SEARCH )
#define SLAP_RESTRICT_OP_WRITES	\
	( SLAP_RESTRICT_OP_ADD    \
	| SLAP_RESTRICT_OP_DELETE \
	| SLAP_RESTRICT_OP_MODIFY \
	| SLAP_RESTRICT_OP_RENAME )
#define SLAP_RESTRICT_OP_ALL \
	( SLAP_RESTRICT_OP_READS \
	| SLAP_RESTRICT_OP_WRITES \
	| SLAP_RESTRICT_OP_BIND \
	| SLAP_RESTRICT_OP_EXTENDED )

#define SLAP_ALLOW_BIND_V2		0x0001U	/* LDAPv2 bind */
#define SLAP_ALLOW_BIND_ANON_CRED	0x0002U /* cred should be empty */
#define SLAP_ALLOW_BIND_ANON_DN		0x0004U /* dn should be empty */

#define SLAP_ALLOW_UPDATE_ANON		0x0008U /* allow anonymous updates */
#define SLAP_ALLOW_PROXY_AUTHZ_ANON	0x0010U /* allow anonymous proxyAuthz */

#define SLAP_DISALLOW_BIND_ANON		0x0001U /* no anonymous */
#define SLAP_DISALLOW_BIND_SIMPLE	0x0002U	/* simple authentication */

#define SLAP_DISALLOW_TLS_2_ANON	0x0010U /* StartTLS -> Anonymous */
#define SLAP_DISALLOW_TLS_AUTHC		0x0020U	/* TLS while authenticated */

#define SLAP_DISALLOW_PROXY_AUTHZ_N_CRIT	0x0100U
#define SLAP_DISALLOW_DONTUSECOPY_N_CRIT	0x0200U

#define SLAP_DISALLOW_AUX_WO_CR		0x4000U

	slap_mask_t	be_requires;	/* pre-operation requirements */
#define SLAP_REQUIRE_BIND		0x0001U	/* bind before op */
#define SLAP_REQUIRE_LDAP_V3	0x0002U	/* LDAPv3 before op */
#define SLAP_REQUIRE_AUTHC		0x0004U	/* authentication before op */
#define SLAP_REQUIRE_SASL		0x0008U	/* SASL before op  */
#define SLAP_REQUIRE_STRONG		0x0010U	/* strong authentication before op */

	/* Required Security Strength Factor */
	slap_ssf_set_t be_ssf_set;

	BerVarray	be_suffix;	/* the DN suffixes of data in this backend */
	BerVarray	be_nsuffix;	/* the normalized DN suffixes in this backend */
	struct berval be_schemadn;	/* per-backend subschema subentry DN */
	struct berval be_schemandn;	/* normalized subschema DN */
	struct berval be_rootdn;	/* the magic "root" name (DN) for this db */
	struct berval be_rootndn;	/* the magic "root" normalized name (DN) for this db */
	struct berval be_rootpw;	/* the magic "root" password for this db	*/
	unsigned int be_max_deref_depth; /* limit for depth of an alias deref  */
#define be_sizelimit	be_def_limit.lms_s_soft
#define be_timelimit	be_def_limit.lms_t_soft
	struct slap_limits_set be_def_limit; /* default limits */
	struct slap_limits **be_limits; /* regex-based size and time limits */
	AccessControl *be_acl;	/* access control list for this backend	   */
	slap_access_t	be_dfltaccess;	/* access given if no acl matches	   */
	AttributeName	*be_extra_anlist;	/* attributes that need to be added to search requests (ITS#6513) */

	/* Replica Information */
	struct berval be_update_ndn;	/* allowed to make changes (in replicas) */
	BerVarray	be_update_refs;	/* where to refer modifying clients to */
	struct		be_pcl	*be_pending_csn_list;
	ldap_pvt_thread_mutex_t					be_pcl_mutex;
	struct syncinfo_s						*be_syncinfo; /* For syncrepl */

	void    *be_pb;         /* Netscape plugin */
	struct ConfigOCs *be_cf_ocs;

	void	*be_private;	/* anything the backend database needs 	   */
	LDAP_STAILQ_ENTRY(BackendDB) be_next;
};

/* Backend function typedefs */
typedef int (BI_bi_func) LDAP_P((BackendInfo *bi));
typedef BI_bi_func BI_init;
typedef BI_bi_func BI_open;
typedef BI_bi_func BI_close;
typedef BI_bi_func BI_destroy;
typedef int (BI_config) LDAP_P((BackendInfo *bi,
	const char *fname, int lineno,
	int argc, char **argv));

typedef struct config_reply_s ConfigReply; /* config.h */
typedef int (BI_db_func) LDAP_P((Backend *bd, ConfigReply *cr));
typedef BI_db_func BI_db_init;
typedef BI_db_func BI_db_open;
typedef BI_db_func BI_db_close;
typedef BI_db_func BI_db_destroy;
typedef int (BI_db_config) LDAP_P((Backend *bd,
	const char *fname, int lineno,
	int argc, char **argv));

typedef struct req_bind_s {
	int rb_method;
	struct berval rb_cred;
	struct berval rb_edn;
	slap_ssf_t rb_ssf;
	struct berval rb_mech;
} req_bind_s;

typedef struct req_search_s {
	int rs_scope;
	int rs_deref;
	int rs_slimit;
	int rs_tlimit;
	/* NULL means be_isroot evaluated to TRUE */
	struct slap_limits_set *rs_limit;
	int rs_attrsonly;
	AttributeName *rs_attrs;
	Filter *rs_filter;
	struct berval rs_filterstr;
} req_search_s;

typedef struct req_compare_s {
	AttributeAssertion *rs_ava;
} req_compare_s;

typedef struct req_modifications_s {
	Modifications *rs_modlist;
	char rs_no_opattrs;		/* don't att modify operational attrs */
} req_modifications_s;

typedef struct req_modify_s {
	req_modifications_s rs_mods;	/* NOTE: must be first in req_modify_s & req_modrdn_s */
	int rs_increment;
} req_modify_s;

typedef struct req_modrdn_s {
	req_modifications_s rs_mods;	/* NOTE: must be first in req_modify_s & req_modrdn_s */
	int rs_deleteoldrdn;
	struct berval rs_newrdn;
	struct berval rs_nnewrdn;
	struct berval *rs_newSup;
	struct berval *rs_nnewSup;
} req_modrdn_s;

typedef struct req_add_s {
	Modifications *rs_modlist;
	Entry *rs_e;
} req_add_s;

typedef struct req_abandon_s {
	ber_int_t rs_msgid;
} req_abandon_s;

#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_EXOP_HIDE 0x0000
#else
#define SLAP_EXOP_HIDE 0x8000
#endif
#define SLAP_EXOP_WRITES 0x0001		/* Exop does writes */

typedef struct req_extended_s {
	struct berval rs_reqoid;
	int rs_flags;
	struct berval *rs_reqdata;
} req_extended_s;

typedef struct req_pwdexop_s {
	struct req_extended_s rs_extended;
	struct berval rs_old;
	struct berval rs_new;
	Modifications *rs_mods;
	Modifications **rs_modtail;
} req_pwdexop_s;

typedef enum slap_reply_e {
	REP_RESULT,
	REP_SASL,
	REP_EXTENDED,
	REP_SEARCH,
	REP_SEARCHREF,
	REP_INTERMEDIATE,
	REP_GLUE_RESULT
} slap_reply_t;

typedef struct rep_sasl_s {
	struct berval *r_sasldata;
} rep_sasl_s;

typedef struct rep_extended_s {
	const char *r_rspoid;
	struct berval *r_rspdata;
} rep_extended_s;

typedef struct rep_search_s {
	Entry *r_entry;
	slap_mask_t r_attr_flags;
#define SLAP_ATTRS_UNDEFINED	(0x00U)
#define SLAP_OPATTRS_NO			(0x01U)
#define SLAP_OPATTRS_YES		(0x02U)
#define SLAP_USERATTRS_NO		(0x10U)
#define SLAP_USERATTRS_YES		(0x20U)
#define SLAP_OPATTRS_MASK(f)	((f) & (SLAP_OPATTRS_NO|SLAP_OPATTRS_YES))
#define SLAP_OPATTRS(f)			(((f) & SLAP_OPATTRS_YES) == SLAP_OPATTRS_YES)
#define SLAP_USERATTRS_MASK(f)	((f) & (SLAP_USERATTRS_NO|SLAP_USERATTRS_YES))
#define SLAP_USERATTRS(f)		\
	(((f) & SLAP_USERATTRS_YES) == SLAP_USERATTRS_YES)

	Attribute *r_operational_attrs;
	AttributeName *r_attrs;
	int r_nentries;
	BerVarray r_v2ref;
} rep_search_s;

struct SlapReply {
	slap_reply_t sr_type;
	ber_tag_t sr_tag;
	ber_int_t sr_msgid;
	ber_int_t sr_err;
	const char *sr_matched;
	const char *sr_text;
	BerVarray sr_ref;
	LDAPControl **sr_ctrls;
	union sr_u {
		rep_search_s sru_search;
		rep_sasl_s sru_sasl;
		rep_extended_s sru_extended;
	} sr_un;
	slap_mask_t sr_flags;
#define	REP_ENTRY_MODIFIABLE	((slap_mask_t) 0x0001U)
#define	REP_ENTRY_MUSTBEFREED	((slap_mask_t) 0x0002U)
#define	REP_ENTRY_MUSTRELEASE	((slap_mask_t) 0x0004U)
#define	REP_ENTRY_MASK		(REP_ENTRY_MODIFIABLE|REP_ENTRY_MUSTFLUSH)
#define	REP_ENTRY_MUSTFLUSH	(REP_ENTRY_MUSTBEFREED|REP_ENTRY_MUSTRELEASE)

#define	REP_MATCHED_MUSTBEFREED	((slap_mask_t) 0x0010U)
#define	REP_MATCHED_MASK	(REP_MATCHED_MUSTBEFREED)

#define REP_REF_MUSTBEFREED	((slap_mask_t) 0x0020U)
#define REP_REF_MASK		(REP_REF_MUSTBEFREED)

#define REP_CTRLS_MUSTBEFREED	((slap_mask_t) 0x0040U)
#define REP_CTRLS_MASK		(REP_CTRLS_MUSTBEFREED)

#define	REP_NO_ENTRYDN		((slap_mask_t) 0x1000U)
#define	REP_NO_SUBSCHEMA	((slap_mask_t) 0x2000U)
#define	REP_NO_OPERATIONALS	(REP_NO_ENTRYDN|REP_NO_SUBSCHEMA)
};

/* short hands for response members */
#define	sr_attrs sr_un.sru_search.r_attrs
#define	sr_entry sr_un.sru_search.r_entry
#define	sr_operational_attrs sr_un.sru_search.r_operational_attrs
#define sr_attr_flags sr_un.sru_search.r_attr_flags
#define	sr_v2ref sr_un.sru_search.r_v2ref
#define	sr_nentries sr_un.sru_search.r_nentries
#define	sr_rspoid sr_un.sru_extended.r_rspoid
#define	sr_rspdata sr_un.sru_extended.r_rspdata
#define	sr_sasldata sr_un.sru_sasl.r_sasldata

typedef int (BI_op_func) LDAP_P(( Operation *op, SlapReply *rs ));
typedef BI_op_func BI_op_bind;
typedef BI_op_func BI_op_unbind;
typedef BI_op_func BI_op_search;
typedef BI_op_func BI_op_compare;
typedef BI_op_func BI_op_modify;
typedef BI_op_func BI_op_modrdn;
typedef BI_op_func BI_op_add;
typedef BI_op_func BI_op_delete;
typedef BI_op_func BI_op_abandon;
typedef BI_op_func BI_op_extended;
typedef BI_op_func BI_op_cancel;
typedef BI_op_func BI_chk_referrals;
typedef BI_op_func BI_chk_controls;
typedef int (BI_entry_release_rw)
	LDAP_P(( Operation *op, Entry *e, int rw ));
typedef int (BI_entry_get_rw) LDAP_P(( Operation *op, struct berval *ndn,
	ObjectClass *oc, AttributeDescription *at, int rw, Entry **e ));
typedef int (BI_operational) LDAP_P(( Operation *op, SlapReply *rs ));
typedef int (BI_has_subordinates) LDAP_P(( Operation *op,
	Entry *e, int *hasSubs ));
typedef int (BI_access_allowed) LDAP_P(( Operation *op, Entry *e,
	AttributeDescription *desc, struct berval *val, slap_access_t access,
	AccessControlState *state, slap_mask_t *maskp ));
typedef int (BI_acl_group) LDAP_P(( Operation *op, Entry *target,
	struct berval *gr_ndn, struct berval *op_ndn,
	ObjectClass *group_oc, AttributeDescription *group_at ));
typedef int (BI_acl_attribute) LDAP_P(( Operation *op, Entry *target,
	struct berval *entry_ndn, AttributeDescription *entry_at,
	BerVarray *vals, slap_access_t access ));

typedef int (BI_conn_func) LDAP_P(( BackendDB *bd, Connection *c ));
typedef BI_conn_func BI_connection_init;
typedef BI_conn_func BI_connection_destroy;

typedef int (BI_tool_entry_open) LDAP_P(( BackendDB *be, int mode ));
typedef int (BI_tool_entry_close) LDAP_P(( BackendDB *be ));
typedef ID (BI_tool_entry_first) LDAP_P(( BackendDB *be ));
typedef ID (BI_tool_entry_first_x) LDAP_P(( BackendDB *be, struct berval *base, int scope, Filter *f ));
typedef ID (BI_tool_entry_next) LDAP_P(( BackendDB *be ));
typedef Entry* (BI_tool_entry_get) LDAP_P(( BackendDB *be, ID id ));
typedef ID (BI_tool_entry_put) LDAP_P(( BackendDB *be, Entry *e, 
	struct berval *text ));
typedef int (BI_tool_entry_reindex) LDAP_P(( BackendDB *be, ID id, AttributeDescription **adv ));
typedef int (BI_tool_sync) LDAP_P(( BackendDB *be ));
typedef ID (BI_tool_dn2id_get) LDAP_P(( BackendDB *be, struct berval *dn ));
typedef ID (BI_tool_entry_modify) LDAP_P(( BackendDB *be, Entry *e, 
	struct berval *text ));

struct BackendInfo {
	char	*bi_type; /* type of backend */

	/*
	 * per backend type routines:
	 * bi_init: called to allocate a backend_info structure,
	 *		called once BEFORE configuration file is read.
	 *		bi_init() initializes this structure hence is
	 *		called directly from be_initialize()
	 * bi_config: called per 'backend' specific option
	 *		all such options must before any 'database' options
	 *		bi_config() is called only from read_config()
	 * bi_open: called to open each database, called
	 *		once AFTER configuration file is read but
	 *		BEFORE any bi_db_open() calls.
	 *		bi_open() is called from backend_startup()
	 * bi_close: called to close each database, called
	 *		once during shutdown after all bi_db_close calls.
	 *		bi_close() is called from backend_shutdown()
	 * bi_destroy: called to destroy each database, called
	 *		once during shutdown after all bi_db_destroy calls.
	 *		bi_destory() is called from backend_destroy()
	 */
	BI_init	*bi_init;
	BI_config	*bi_config;
	BI_open *bi_open;
	BI_close	*bi_close;
	BI_destroy	*bi_destroy;

	/*
	 * per database routines:
	 * bi_db_init: called to initialize each database,
	 *	called upon reading 'database <type>' 
	 *	called only from backend_db_init()
	 * bi_db_config: called to configure each database,
	 *  called per database to handle per database options
	 *	called only from read_config()
	 * bi_db_open: called to open each database
	 *	called once per database immediately AFTER bi_open()
	 *	calls but before daemon startup.
	 *  called only by backend_startup()
	 * bi_db_close: called to close each database
	 *	called once per database during shutdown but BEFORE
	 *  any bi_close call.
	 *  called only by backend_shutdown()
	 * bi_db_destroy: called to destroy each database
	 *  called once per database during shutdown AFTER all
	 *  bi_close calls but before bi_destory calls.
	 *  called only by backend_destory()
	 */
	BI_db_init	*bi_db_init;
	BI_db_config	*bi_db_config;
	BI_db_open	*bi_db_open;
	BI_db_close	*bi_db_close;
	BI_db_destroy	*bi_db_destroy;

	/* LDAP Operations Handling Routines */
	BI_op_bind	*bi_op_bind;
	BI_op_unbind	*bi_op_unbind;
	BI_op_search	*bi_op_search;
	BI_op_compare	*bi_op_compare;
	BI_op_modify	*bi_op_modify;
	BI_op_modrdn	*bi_op_modrdn;
	BI_op_add	*bi_op_add;
	BI_op_delete	*bi_op_delete;
	BI_op_abandon	*bi_op_abandon;

	/* Extended Operations Helper */
	BI_op_extended	*bi_extended;
	BI_op_cancel	*bi_op_cancel;

	/* Auxilary Functions */
	BI_operational		*bi_operational;
	BI_chk_referrals	*bi_chk_referrals;
	BI_chk_controls		*bi_chk_controls;
	BI_entry_get_rw		*bi_entry_get_rw;
	BI_entry_release_rw	*bi_entry_release_rw;

	BI_has_subordinates	*bi_has_subordinates;
	BI_access_allowed	*bi_access_allowed;
	BI_acl_group		*bi_acl_group;
	BI_acl_attribute	*bi_acl_attribute;

	BI_connection_init	*bi_connection_init;
	BI_connection_destroy	*bi_connection_destroy;

	/* hooks for slap tools */
	BI_tool_entry_open	*bi_tool_entry_open;
	BI_tool_entry_close	*bi_tool_entry_close;
	BI_tool_entry_first	*bi_tool_entry_first;	/* deprecated */
	BI_tool_entry_first_x	*bi_tool_entry_first_x;
	BI_tool_entry_next	*bi_tool_entry_next;
	BI_tool_entry_get	*bi_tool_entry_get;
	BI_tool_entry_put	*bi_tool_entry_put;
	BI_tool_entry_reindex	*bi_tool_entry_reindex;
	BI_tool_sync		*bi_tool_sync;
	BI_tool_dn2id_get	*bi_tool_dn2id_get;
	BI_tool_entry_modify	*bi_tool_entry_modify;

#define SLAP_INDEX_ADD_OP		0x0001
#define SLAP_INDEX_DELETE_OP	0x0002

	slap_mask_t	bi_flags; /* backend flags */
#define SLAP_BFLAG_MONITOR			0x0001U /* a monitor backend */
#define SLAP_BFLAG_CONFIG			0x0002U /* a config backend */
#define SLAP_BFLAG_FRONTEND			0x0004U /* the frontendDB */
#define SLAP_BFLAG_NOLASTMODCMD		0x0010U
#define SLAP_BFLAG_INCREMENT		0x0100U
#define SLAP_BFLAG_ALIASES			0x1000U
#define SLAP_BFLAG_REFERRALS		0x2000U
#define SLAP_BFLAG_SUBENTRIES		0x4000U
#define SLAP_BFLAG_DYNAMIC			0x8000U

/* overlay specific */
#define	SLAPO_BFLAG_SINGLE		0x01000000U
#define	SLAPO_BFLAG_DBONLY		0x02000000U
#define	SLAPO_BFLAG_GLOBONLY		0x04000000U
#define	SLAPO_BFLAG_DISABLED		0x08000000U
#define	SLAPO_BFLAG_MASK		0xFF000000U

#define SLAP_BFLAGS(be)		((be)->bd_info->bi_flags)
#define SLAP_MONITOR(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_MONITOR)
#define SLAP_CONFIG(be)		(SLAP_BFLAGS(be) & SLAP_BFLAG_CONFIG)
#define SLAP_FRONTEND(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_FRONTEND)
#define SLAP_INCREMENT(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_INCREMENT)
#define SLAP_ALIASES(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_ALIASES)
#define SLAP_REFERRALS(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_REFERRALS)
#define SLAP_SUBENTRIES(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_SUBENTRIES)
#define SLAP_DYNAMIC(be)	((SLAP_BFLAGS(be) & SLAP_BFLAG_DYNAMIC) || (SLAP_DBFLAGS(be) & SLAP_DBFLAG_DYNAMIC))
#define SLAP_NOLASTMODCMD(be)	(SLAP_BFLAGS(be) & SLAP_BFLAG_NOLASTMODCMD)
#define SLAP_LASTMODCMD(be)	(!SLAP_NOLASTMODCMD(be))

/* overlay specific */
#define SLAPO_SINGLE(be)	(SLAP_BFLAGS(be) & SLAPO_BFLAG_SINGLE)
#define SLAPO_DBONLY(be)	(SLAP_BFLAGS(be) & SLAPO_BFLAG_DBONLY)
#define SLAPO_GLOBONLY(be)	(SLAP_BFLAGS(be) & SLAPO_BFLAG_GLOBONLY)
#define SLAPO_DISABLED(be)	(SLAP_BFLAGS(be) & SLAPO_BFLAG_DISABLED)

	char	**bi_controls;		/* supported controls */
	char	bi_ctrls[SLAP_MAX_CIDS + 1];

	unsigned int bi_nDB;	/* number of databases of this type */
	struct ConfigOCs *bi_cf_ocs;
	char	**bi_obsolete_names;
	void	*bi_extra;		/* backend type-specific APIs */
	void	*bi_private;	/* backend type-specific config data */
	LDAP_STAILQ_ENTRY(BackendInfo) bi_next ;
};

#define c_authtype	c_authz.sai_method
#define c_authmech	c_authz.sai_mech
#define c_dn		c_authz.sai_dn
#define c_ndn		c_authz.sai_ndn
#define c_ssf		c_authz.sai_ssf
#define c_transport_ssf	c_authz.sai_transport_ssf
#define c_tls_ssf	c_authz.sai_tls_ssf
#define c_sasl_ssf	c_authz.sai_sasl_ssf

#define o_authtype	o_authz.sai_method
#define o_authmech	o_authz.sai_mech
#define o_dn		o_authz.sai_dn
#define o_ndn		o_authz.sai_ndn
#define o_ssf		o_authz.sai_ssf
#define o_transport_ssf	o_authz.sai_transport_ssf
#define o_tls_ssf	o_authz.sai_tls_ssf
#define o_sasl_ssf	o_authz.sai_sasl_ssf

typedef int (slap_response)( Operation *, SlapReply * );

typedef struct slap_callback {
	struct slap_callback *sc_next;
	slap_response *sc_response;
	slap_response *sc_cleanup;
	void *sc_private;
} slap_callback;

struct slap_overinfo;

typedef enum slap_operation_e {
	op_bind = 0,
	op_unbind,
	op_search,
	op_compare,
	op_modify,
	op_modrdn,
	op_add,
	op_delete,
	op_abandon,
	op_extended,
	op_cancel,
	op_aux_operational,
	op_aux_chk_referrals,
	op_aux_chk_controls,
	op_last
} slap_operation_t;

typedef struct slap_overinst {
	BackendInfo on_bi;
	slap_response *on_response;
	struct slap_overinfo *on_info;
	struct slap_overinst *on_next;
} slap_overinst;

typedef struct slap_overinfo {
	BackendInfo oi_bi;
	BackendInfo *oi_orig;
	BackendDB	*oi_origdb;
	struct slap_overinst *oi_list;
} slap_overinfo;

/* Should successive callbacks in a chain be processed? */
#define	SLAP_CB_BYPASS		0x08800
#define	SLAP_CB_CONTINUE	0x08000

/*
 * Paged Results state
 */
typedef unsigned long PagedResultsCookie;
typedef struct PagedResultsState {
	Backend *ps_be;
	ber_int_t ps_size;
	int ps_count;
	PagedResultsCookie ps_cookie;
	struct berval ps_cookieval;
} PagedResultsState;

struct slap_csn_entry {
	struct berval ce_csn;
	int ce_sid;
	unsigned long ce_opid;
	unsigned long ce_connid;
#define SLAP_CSN_PENDING	1
#define SLAP_CSN_COMMIT		2
	long ce_state;
	LDAP_TAILQ_ENTRY (slap_csn_entry) ce_csn_link;
};

/*
 * Caches the result of a backend_group check for ACL evaluation
 */
typedef struct GroupAssertion {
	struct GroupAssertion *ga_next;
	Backend *ga_be;
	ObjectClass *ga_oc;
	AttributeDescription *ga_at;
	int ga_res;
	ber_len_t ga_len;
	char ga_ndn[1];
} GroupAssertion;

struct slap_control_ids {
	int sc_LDAPsync;
	int sc_assert;
	int sc_domainScope;
	int sc_dontUseCopy;
	int sc_manageDSAit;
	int sc_modifyIncrement;
	int sc_noOp;
	int sc_pagedResults;
	int sc_permissiveModify;
	int sc_postRead;
	int sc_preRead;
	int sc_proxyAuthz;
	int sc_relax;
	int sc_searchOptions;
#ifdef SLAP_CONTROL_X_SORTEDRESULTS
	int sc_sortedResults;
#endif
	int sc_subentries;
#ifdef SLAP_CONTROL_X_TREE_DELETE
	int sc_treeDelete;
#endif
#ifdef LDAP_X_TXN
	int sc_txnSpec;
#endif
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	int sc_sessionTracking;
#endif
	int sc_valuesReturnFilter;
#ifdef SLAP_CONTROL_X_WHATFAILED
	int sc_whatFailed;
#endif
};

/*
 * Operation indices
 */
typedef enum {
	SLAP_OP_BIND = 0,
	SLAP_OP_UNBIND,
	SLAP_OP_SEARCH,
	SLAP_OP_COMPARE,
	SLAP_OP_MODIFY,
	SLAP_OP_MODRDN,
	SLAP_OP_ADD,
	SLAP_OP_DELETE,
	SLAP_OP_ABANDON,
	SLAP_OP_EXTENDED,
	SLAP_OP_LAST
} slap_op_t;

typedef struct slap_counters_t {
	struct slap_counters_t	*sc_next;
	ldap_pvt_thread_mutex_t	sc_mutex;
	ldap_pvt_mp_t		sc_bytes;
	ldap_pvt_mp_t		sc_pdu;
	ldap_pvt_mp_t		sc_entries;
	ldap_pvt_mp_t		sc_refs;

	ldap_pvt_mp_t		sc_ops_completed;
	ldap_pvt_mp_t		sc_ops_initiated;
#ifdef SLAPD_MONITOR
	ldap_pvt_mp_t		sc_ops_completed_[SLAP_OP_LAST];
	ldap_pvt_mp_t		sc_ops_initiated_[SLAP_OP_LAST];
#endif /* SLAPD_MONITOR */
} slap_counters_t;

/*
 * represents an operation pending from an ldap client
 */
typedef struct Opheader {
	unsigned long	oh_opid;	/* id of this operation */
	unsigned long	oh_connid;	/* id of conn initiating this op */
	Connection	*oh_conn;	/* connection spawning this op */

	ber_int_t	oh_msgid;	/* msgid of the request */
	ber_int_t	oh_protocol;	/* version of the LDAP protocol used by client */

	ldap_pvt_thread_t	oh_tid;	/* thread handling this op */

	void	*oh_threadctx;		/* thread pool thread context */
	void	*oh_tmpmemctx;		/* slab malloc context */
	BerMemoryFunctions *oh_tmpmfuncs;

	slap_counters_t	*oh_counters;

	char		oh_log_prefix[ /* sizeof("conn= op=") + 2*LDAP_PVT_INTTYPE_CHARS(unsigned long) */ SLAP_TEXT_BUFLEN ];

#ifdef LDAP_SLAPI
	void	*oh_extensions;		/* NS-SLAPI plugin */
#endif
} Opheader;

typedef union OpRequest {
	req_add_s oq_add;
	req_bind_s oq_bind;
	req_compare_s oq_compare;
	req_modify_s oq_modify;
	req_modrdn_s oq_modrdn;
	req_search_s oq_search;
	req_abandon_s oq_abandon;
	req_abandon_s oq_cancel;
	req_extended_s oq_extended;
	req_pwdexop_s oq_pwdexop;
} OpRequest;

/* This is only a header. Actual users should define their own
 * structs with the oe_next / oe_key fields at the top and
 * whatever else they need following.
 */
typedef struct OpExtra {
	LDAP_SLIST_ENTRY(OpExtra) oe_next;
	void *oe_key;
} OpExtra;

typedef struct OpExtraDB {
	OpExtra oe;
	BackendDB *oe_db;
} OpExtraDB;

struct Operation {
	Opheader *o_hdr;

#define o_opid o_hdr->oh_opid
#define o_connid o_hdr->oh_connid
#define o_conn o_hdr->oh_conn
#define o_msgid o_hdr->oh_msgid
#define o_protocol o_hdr->oh_protocol
#define o_tid o_hdr->oh_tid
#define o_threadctx o_hdr->oh_threadctx
#define o_tmpmemctx o_hdr->oh_tmpmemctx
#define o_tmpmfuncs o_hdr->oh_tmpmfuncs
#define o_counters o_hdr->oh_counters

#define	o_tmpalloc	o_tmpmfuncs->bmf_malloc
#define o_tmpcalloc	o_tmpmfuncs->bmf_calloc
#define	o_tmprealloc	o_tmpmfuncs->bmf_realloc
#define	o_tmpfree	o_tmpmfuncs->bmf_free

#define o_log_prefix o_hdr->oh_log_prefix

	ber_tag_t	o_tag;		/* tag of the request */
	time_t		o_time;		/* time op was initiated */
	int			o_tincr;	/* counter for multiple ops with same o_time */

	BackendDB	*o_bd;	/* backend DB processing this op */
	struct berval	o_req_dn;	/* DN of target of request */
	struct berval	o_req_ndn;

	OpRequest o_request;

/* short hands for union members */
#define oq_add o_request.oq_add
#define oq_bind o_request.oq_bind
#define oq_compare o_request.oq_compare
#define oq_modify o_request.oq_modify
#define oq_modrdn o_request.oq_modrdn
#define oq_search o_request.oq_search
#define oq_abandon o_request.oq_abandon
#define oq_cancel o_request.oq_cancel
#define oq_extended o_request.oq_extended
#define oq_pwdexop o_request.oq_pwdexop

/* short hands for inner request members */
#define orb_method oq_bind.rb_method
#define orb_cred oq_bind.rb_cred
#define orb_edn oq_bind.rb_edn
#define orb_ssf oq_bind.rb_ssf
#define orb_mech oq_bind.rb_mech

#define ors_scope oq_search.rs_scope
#define ors_deref oq_search.rs_deref
#define ors_slimit oq_search.rs_slimit
#define ors_tlimit oq_search.rs_tlimit
#define ors_limit oq_search.rs_limit
#define ors_attrsonly oq_search.rs_attrsonly
#define ors_attrs oq_search.rs_attrs
#define ors_filter oq_search.rs_filter
#define ors_filterstr oq_search.rs_filterstr

#define orr_modlist oq_modrdn.rs_mods.rs_modlist
#define orr_no_opattrs oq_modrdn.rs_mods.rs_no_opattrs
#define orr_deleteoldrdn oq_modrdn.rs_deleteoldrdn
#define orr_newrdn oq_modrdn.rs_newrdn
#define orr_nnewrdn oq_modrdn.rs_nnewrdn
#define orr_newSup oq_modrdn.rs_newSup
#define orr_nnewSup oq_modrdn.rs_nnewSup

#define orc_ava oq_compare.rs_ava

#define ora_e oq_add.rs_e
#define ora_modlist oq_add.rs_modlist

#define orn_msgid oq_abandon.rs_msgid

#define orm_modlist oq_modify.rs_mods.rs_modlist
#define orm_no_opattrs oq_modify.rs_mods.rs_no_opattrs
#define orm_increment oq_modify.rs_increment

#define ore_reqoid oq_extended.rs_reqoid
#define ore_flags oq_extended.rs_flags
#define ore_reqdata oq_extended.rs_reqdata
	volatile sig_atomic_t o_abandon;	/* abandon flag */
	volatile sig_atomic_t o_cancel;		/* cancel flag */
#define SLAP_CANCEL_NONE				0x00
#define SLAP_CANCEL_REQ					0x01
#define SLAP_CANCEL_ACK					0x02
#define SLAP_CANCEL_DONE				0x03

	GroupAssertion *o_groups;
	char o_do_not_cache;	/* don't cache groups from this op */
	char o_is_auth_check;	/* authorization in progress */
	char o_dont_replicate;
	slap_access_t o_acl_priv;

	char o_nocaching;
	char o_delete_glue_parent;
	char o_no_schema_check;
#define get_no_schema_check(op)			((op)->o_no_schema_check)
	char o_no_subordinate_glue;
#define get_no_subordinate_glue(op)		((op)->o_no_subordinate_glue)

#define SLAP_CONTROL_NONE	0
#define SLAP_CONTROL_IGNORED	1
#define SLAP_CONTROL_NONCRITICAL 2
#define SLAP_CONTROL_CRITICAL	3
#define	SLAP_CONTROL_MASK	3

/* spare bits for simple flags */
#define SLAP_CONTROL_SHIFT	4	/* shift to reach data bits */
#define SLAP_CONTROL_DATA0	0x10
#define SLAP_CONTROL_DATA1	0x20
#define SLAP_CONTROL_DATA2	0x40
#define SLAP_CONTROL_DATA3	0x80

#define _SCM(x)	((x) & SLAP_CONTROL_MASK)

	char o_ctrlflag[SLAP_MAX_CIDS];	/* per-control flags */
	void **o_controls;		/* per-control state */

#define o_dontUseCopy			o_ctrlflag[slap_cids.sc_dontUseCopy]
#define get_dontUseCopy(op)		_SCM((op)->o_dontUseCopy)

#define o_relax				o_ctrlflag[slap_cids.sc_relax]
#define get_relax(op)		_SCM((op)->o_relax)

#define o_managedsait	o_ctrlflag[slap_cids.sc_manageDSAit]
#define get_manageDSAit(op)				_SCM((op)->o_managedsait)

#define o_noop	o_ctrlflag[slap_cids.sc_noOp]
#define o_proxy_authz	o_ctrlflag[slap_cids.sc_proxyAuthz]
#define o_subentries	o_ctrlflag[slap_cids.sc_subentries]

#define get_subentries(op)				_SCM((op)->o_subentries)
#define	o_subentries_visibility	o_ctrlflag[slap_cids.sc_subentries]

#define set_subentries_visibility(op)	((op)->o_subentries |= SLAP_CONTROL_DATA0)
#define get_subentries_visibility(op)	(((op)->o_subentries & SLAP_CONTROL_DATA0) != 0)

#define o_assert	o_ctrlflag[slap_cids.sc_assert]
#define get_assert(op)					((int)(op)->o_assert)
#define o_assertion	o_controls[slap_cids.sc_assert]
#define get_assertion(op)				((op)->o_assertion)

#define	o_valuesreturnfilter	o_ctrlflag[slap_cids.sc_valuesReturnFilter]
#define o_vrFilter	o_controls[slap_cids.sc_valuesReturnFilter]

#define o_permissive_modify	o_ctrlflag[slap_cids.sc_permissiveModify]
#define get_permissiveModify(op)		((int)(op)->o_permissive_modify)

#define o_domain_scope	o_ctrlflag[slap_cids.sc_domainScope]
#define get_domainScope(op)				((int)(op)->o_domain_scope)

#ifdef SLAP_CONTROL_X_TREE_DELETE
#define	o_tree_delete	o_ctrlflag[slap_cids.sc_treeDelete]
#define get_treeDelete(op)				((int)(op)->o_tree_delete)
#endif

#define o_preread	o_ctrlflag[slap_cids.sc_preRead]
#define o_postread	o_ctrlflag[slap_cids.sc_postRead]

#define	o_preread_attrs	o_controls[slap_cids.sc_preRead]
#define o_postread_attrs	o_controls[slap_cids.sc_postRead]

#define o_pagedresults	o_ctrlflag[slap_cids.sc_pagedResults]
#define o_pagedresults_state	o_controls[slap_cids.sc_pagedResults]
#define get_pagedresults(op)			((int)(op)->o_pagedresults)

#ifdef SLAP_CONTROL_X_SORTEDRESULTS
#define o_sortedresults		o_ctrlflag[slap_cids.sc_sortedResults]
#endif

#ifdef LDAP_X_TXN
#define o_txnSpec		o_ctrlflag[slap_cids.sc_txnSpec]
#endif

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
#define o_session_tracking	o_ctrlflag[slap_cids.sc_sessionTracking]
#define o_tracked_sessions	o_controls[slap_cids.sc_sessionTracking]
#define get_sessionTracking(op)			((int)(op)->o_session_tracking)
#endif

#ifdef SLAP_CONTROL_X_WHATFAILED
#define o_whatFailed o_ctrlflag[slap_cids.sc_whatFailed]
#define get_whatFailed(op)				_SCM((op)->o_whatFailed)
#endif

#define o_sync			o_ctrlflag[slap_cids.sc_LDAPsync]

	AuthorizationInformation o_authz;

	BerElement	*o_ber;		/* ber of the request */
	BerElement	*o_res_ber;	/* ber of the CLDAP reply or readback control */
	slap_callback *o_callback;	/* callback pointers */
	LDAPControl	**o_ctrls;	 /* controls */
	struct berval o_csn;

	/* DEPRECATE o_private - use o_extra instead */
	void	*o_private;	/* anything the backend needs */
	LDAP_SLIST_HEAD(o_e, OpExtra) o_extra;	/* anything the backend needs */

	LDAP_STAILQ_ENTRY(Operation)	o_next;	/* next operation in list */
};

typedef struct OperationBuffer {
	Operation	ob_op;
	Opheader	ob_hdr;
	void		*ob_controls[SLAP_MAX_CIDS];
} OperationBuffer;

#define send_ldap_error( op, rs, err, text ) do { \
		(rs)->sr_err = err; (rs)->sr_text = text; \
		((op)->o_conn->c_send_ldap_result)( op, rs ); \
	} while (0)
#define send_ldap_discon( op, rs, err, text ) do { \
		(rs)->sr_err = err; (rs)->sr_text = text; \
		send_ldap_disconnect( op, rs ); \
	} while (0)

typedef void (SEND_LDAP_RESULT)(
	Operation *op, SlapReply *rs);
typedef int (SEND_SEARCH_ENTRY)(
	Operation *op, SlapReply *rs);
typedef int (SEND_SEARCH_REFERENCE)(
	Operation *op, SlapReply *rs);
typedef void (SEND_LDAP_EXTENDED)(
	Operation *op, SlapReply *rs);
typedef void (SEND_LDAP_INTERMEDIATE)(
	Operation *op, SlapReply *rs);

#define send_ldap_result( op, rs ) \
	((op)->o_conn->c_send_ldap_result)( op, rs )
#define send_search_entry( op, rs ) \
	((op)->o_conn->c_send_search_entry)( op, rs )
#define send_search_reference( op, rs ) \
	((op)->o_conn->c_send_search_reference)( op, rs )
#define send_ldap_extended( op, rs ) \
	((op)->o_conn->c_send_ldap_extended)( op, rs )
#define send_ldap_intermediate( op, rs ) \
	((op)->o_conn->c_send_ldap_intermediate)( op, rs )

typedef struct Listener Listener;

/*
 * represents a connection from an ldap client
 */
/* structure state (protected by connections_mutex) */
enum sc_struct_state {
	SLAP_C_UNINITIALIZED = 0,	/* MUST BE ZERO (0) */
	SLAP_C_UNUSED,
	SLAP_C_USED,
	SLAP_C_PENDING
};

/* connection state (protected by c_mutex ) */
enum sc_conn_state {
	SLAP_C_INVALID = 0,		/* MUST BE ZERO (0) */
	SLAP_C_INACTIVE,		/* zero threads */
	SLAP_C_CLOSING,			/* closing */
	SLAP_C_ACTIVE,			/* one or more threads */
	SLAP_C_BINDING,			/* binding */
	SLAP_C_CLIENT			/* outbound client conn */
};
struct Connection {
	enum sc_struct_state	c_struct_state; /* structure management state */
	enum sc_conn_state	c_conn_state;	/* connection state */
	int			c_conn_idx;		/* slot in connections array */
	ber_socket_t	c_sd;
	const char	*c_close_reason; /* why connection is closing */

	ldap_pvt_thread_mutex_t	c_mutex; /* protect the connection */
	Sockbuf		*c_sb;			/* ber connection stuff		  */

	/* only can be changed by connect_init */
	time_t		c_starttime;	/* when the connection was opened */
	time_t		c_activitytime;	/* when the connection was last used */
	unsigned long		c_connid;	/* id of this connection for stats*/

	struct berval	c_peer_domain;	/* DNS name of client */
	struct berval	c_peer_name;	/* peer name (trans=addr:port) */
	Listener	*c_listener;
#define c_listener_url c_listener->sl_url	/* listener URL */
#define c_sock_name c_listener->sl_name	/* sock name (trans=addr:port) */

	/* only can be changed by binding thread */
	struct berval	c_sasl_bind_mech;			/* mech in progress */
	struct berval	c_sasl_dn;	/* temporary storage */
	struct berval	c_sasl_authz_dn;	/* SASL proxy authz */

	/* authorization backend */
	Backend *c_authz_backend;
	void	*c_authz_cookie;
#define SLAP_IS_AUTHZ_BACKEND( op )	\
	( (op)->o_bd != NULL \
		&& (op)->o_bd->be_private != NULL \
		&& (op)->o_conn != NULL \
		&& (op)->o_conn->c_authz_backend != NULL \
		&& ( (op)->o_bd->be_private == (op)->o_conn->c_authz_backend->be_private \
			|| (op)->o_bd->be_private == (op)->o_conn->c_authz_cookie ) )

	AuthorizationInformation c_authz;

	ber_int_t	c_protocol;	/* version of the LDAP protocol used by client */

	LDAP_STAILQ_HEAD(c_o, Operation) c_ops;	/* list of operations being processed */
	LDAP_STAILQ_HEAD(c_po, Operation) c_pending_ops;	/* list of pending operations */

	ldap_pvt_thread_mutex_t	c_write1_mutex;	/* only one pdu written at a time */
	ldap_pvt_thread_cond_t	c_write1_cv;	/* only one pdu written at a time */
	ldap_pvt_thread_mutex_t	c_write2_mutex;	/* used to wait for sd write-ready */
	ldap_pvt_thread_cond_t	c_write2_cv;	/* used to wait for sd write-ready*/

	BerElement	*c_currentber;	/* ber we're attempting to read */
	int			c_writers;		/* number of writers waiting */
	char		c_writing;		/* someone is writing */

	char		c_sasl_bind_in_progress;	/* multi-op bind in progress */
	char		c_writewaiter;	/* true if blocked on write */


#define	CONN_IS_TLS	1
#define	CONN_IS_UDP	2
#define	CONN_IS_CLIENT	4
#define	CONN_IS_IPC	8

#ifdef LDAP_CONNECTIONLESS
	char	c_is_udp;		/* true if this is (C)LDAP over UDP */
#endif
#ifdef HAVE_TLS
	char	c_is_tls;		/* true if this LDAP over raw TLS */
	char	c_needs_tls_accept;	/* true if SSL_accept should be called */
#endif
	char	c_sasl_layers;	 /* true if we need to install SASL i/o handlers */
	char	c_sasl_done;		/* SASL completed once */
	void	*c_sasl_authctx;	/* SASL authentication context */
	void	*c_sasl_sockctx;	/* SASL security layer context */
	void	*c_sasl_extra;		/* SASL session extra stuff */
	void	*c_sasl_cbind;		/* SASL channel binding */
	Operation	*c_sasl_bindop;	/* set to current op if it's a bind */

#ifdef LDAP_X_TXN
#define CONN_TXN_INACTIVE 0
#define CONN_TXN_SPECIFY 1
#define CONN_TXN_SETTLE -1
	int c_txn;

	Backend *c_txn_backend;
	LDAP_STAILQ_HEAD(c_to, Operation) c_txn_ops; /* list of operations in txn */
#endif

	PagedResultsState c_pagedresults_state; /* paged result state */

	long	c_n_ops_received;	/* num of ops received (next op_id) */
	long	c_n_ops_executing;	/* num of ops currently executing */
	long	c_n_ops_pending;	/* num of ops pending execution */
	long	c_n_ops_completed;	/* num of ops completed */

	long	c_n_get;		/* num of get calls */
	long	c_n_read;		/* num of read calls */
	long	c_n_write;		/* num of write calls */

	void	*c_extensions;		/* Netscape plugin */

	/*
	 * Client connection handling
	 */
	ldap_pvt_thread_start_t	*c_clientfunc;
	void	*c_clientarg;

	/*
	 * These are the "callbacks" that are available for back-ends to
	 * supply data back to connected clients that are connected
	 * through the "front-end".
	 */
	SEND_LDAP_RESULT *c_send_ldap_result;
	SEND_SEARCH_ENTRY *c_send_search_entry;
	SEND_SEARCH_REFERENCE *c_send_search_reference;
	SEND_LDAP_EXTENDED *c_send_ldap_extended;
	SEND_LDAP_INTERMEDIATE *c_send_ldap_intermediate;
};

#ifdef LDAP_DEBUG
#ifdef LDAP_SYSLOG
#ifdef LOG_LOCAL4
#define SLAP_DEFAULT_SYSLOG_USER	LOG_LOCAL4
#endif /* LOG_LOCAL4 */

#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 )	\
	Log5( (level), ldap_syslog_level, (fmt), (connid), (opid), (arg1), (arg2), (arg3) )
#define StatslogTest( level ) ((ldap_debug | ldap_syslog) & (level))
#else /* !LDAP_SYSLOG */
#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 )	\
	do { \
		if ( ldap_debug & (level) ) \
			lutil_debug( ldap_debug, (level), (fmt), (connid), (opid), (arg1), (arg2), (arg3) );\
	} while (0)
#define StatslogTest( level ) (ldap_debug & (level))
#endif /* !LDAP_SYSLOG */
#else /* !LDAP_DEBUG */
#define Statslog( level, fmt, connid, opid, arg1, arg2, arg3 ) ((void) 0)
#define StatslogTest( level ) (0)
#endif /* !LDAP_DEBUG */

/*
 * listener; need to access it from monitor backend
 */
struct Listener {
	struct berval sl_url;
	struct berval sl_name;
	mode_t	sl_perms;
#ifdef HAVE_TLS
	int		sl_is_tls;
#endif
#ifdef LDAP_CONNECTIONLESS
	int	sl_is_udp;		/* UDP listener is also data port */
#endif
	int	sl_mute;	/* Listener is temporarily disabled due to emfile */
	int	sl_busy;	/* Listener is busy (accept thread activated) */
	ber_socket_t sl_sd;
	Sockaddr sl_sa;
#define sl_addr	sl_sa.sa_in_addr
#define LDAP_TCP_BUFFER
#ifdef LDAP_TCP_BUFFER
	int	sl_tcp_rmem;	/* custom TCP read buffer size */
	int	sl_tcp_wmem;	/* custom TCP write buffer size */
#endif
};

/*
 * Better know these all around slapd
 */
#define SLAP_LDAPDN_PRETTY 0x1
#define SLAP_LDAPDN_MAXLEN 8192

/* number of response controls supported */
#define SLAP_MAX_RESPONSE_CONTROLS   6

#ifdef SLAP_SCHEMA_EXPOSE
#define SLAP_CTRL_HIDE				0x00000000U
#else
#define SLAP_CTRL_HIDE				0x80000000U
#endif

#define SLAP_CTRL_REQUIRES_ROOT		0x40000000U /* for Relax */

#define SLAP_CTRL_GLOBAL			0x00800000U
#define SLAP_CTRL_GLOBAL_SEARCH		0x00010000U	/* for NOOP */

#define SLAP_CTRL_OPFLAGS			0x0000FFFFU
#define SLAP_CTRL_ABANDON			0x00000001U
#define SLAP_CTRL_ADD				0x00002002U
#define SLAP_CTRL_BIND				0x00000004U
#define SLAP_CTRL_COMPARE			0x00001008U
#define SLAP_CTRL_DELETE			0x00002010U
#define SLAP_CTRL_MODIFY			0x00002020U
#define SLAP_CTRL_RENAME			0x00002040U
#define SLAP_CTRL_SEARCH			0x00001080U
#define SLAP_CTRL_UNBIND			0x00000100U

#define SLAP_CTRL_INTROGATE	(SLAP_CTRL_COMPARE|SLAP_CTRL_SEARCH)
#define SLAP_CTRL_UPDATE \
	(SLAP_CTRL_ADD|SLAP_CTRL_DELETE|SLAP_CTRL_MODIFY|SLAP_CTRL_RENAME)
#define SLAP_CTRL_ACCESS	(SLAP_CTRL_INTROGATE|SLAP_CTRL_UPDATE)

typedef int (SLAP_CTRL_PARSE_FN) LDAP_P((
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl ));

typedef int (*SLAP_ENTRY_INFO_FN) LDAP_P(( void *arg, Entry *e ));

#define SLAP_SLAB_SIZE	(1024*1024)
#define SLAP_SLAB_STACK 1

#define SLAP_ZONE_ALLOC 1
#undef SLAP_ZONE_ALLOC

#ifdef LDAP_COMP_MATCH
/*
 * Extensible Filter Definition
 *
 * MatchingRuleAssertion := SEQUENCE {
 *	matchingRule	[1] MatchingRuleId OPTIONAL,
 *	type		[2] AttributeDescription OPTIONAL,
 *	matchValue	[3] AssertionValue,
 *	dnAttributes	[4] BOOLEAN DEFAULT FALSE }
 *
 * Following ComponentFilter is contained in matchValue
 *
 * ComponentAssertion ::= SEQUENCE {
 *	component		ComponentReference (SIZE(1..MAX)) OPTIONAL
 *	useDefaultValues	BOOLEAN DEFAULT TRUE,
 *	rule			MATCHING-RULE.&id,
 *	value			MATCHING-RULE.&AssertionType }
 *
 * ComponentFilter ::= CHOICE {
 *	item	[0] ComponentAssertion,
 *	and	[1] SEQUENCE OF ComponentFilter,
 *	or	[2] SEQUENCE OF ComponentFilter,
 *	not	[3] ComponentFilter }
 */

#define LDAP_COMPREF_IDENTIFIER		((ber_tag_t) 0x80U)
#define LDAP_COMPREF_FROM_BEGINNING	((ber_tag_t) 0x81U)
#define LDAP_COMPREF_COUNT		((ber_tag_t) 0x82U)
#define LDAP_COMPREF_FROM_END		((ber_tag_t) 0x83U)
#define LDAP_COMPREF_CONTENT		((ber_tag_t) 0x84U)
#define LDAP_COMPREF_SELECT		((ber_tag_t) 0x85U)
#define LDAP_COMPREF_ALL		((ber_tag_t) 0x86U)
#define LDAP_COMPREF_DEFINED		((ber_tag_t) 0x87U)
#define LDAP_COMPREF_UNDEFINED		((ber_tag_t) 0x88U)

#define LDAP_COMP_FILTER_AND		((ber_tag_t) 0xa0U)
#define LDAP_COMP_FILTER_OR		((ber_tag_t) 0xa1U)
#define LDAP_COMP_FILTER_NOT		((ber_tag_t) 0xa2U)
#define LDAP_COMP_FILTER_ITEM		((ber_tag_t) 0xa3U)
#define LDAP_COMP_FILTER_UNDEFINED	((ber_tag_t) 0xa4U)

typedef struct ComponentId ComponentId;
typedef struct ComponentReference ComponentReference;
typedef struct ComponentAssertion ComponentAssertion;
typedef struct ComponentAssertionValue ComponentAssertionValue;
typedef struct ComponentSyntaxInfo ComponentSyntaxInfo;
typedef struct ComponentDesc ComponentDesc;

struct ComponentData {
	void			*cd_mem_op;	/* nibble memory handler */
	ComponentSyntaxInfo**	cd_tree;	/* component tree */
};

struct ComponentId {
	int		ci_type;
	ComponentId	*ci_next;

	union comp_id_value{
		BerValue	ci_identifier;
		ber_int_t	ci_from_beginning;
		ber_int_t	ci_count;
		ber_int_t	ci_from_end;
		ber_int_t	ci_content;
		BerValue	ci_select_value;
		char		ci_all;
	} ci_val;
};

struct ComponentReference {
	ComponentId	*cr_list;
	ComponentId	*cr_curr;
	struct berval	cr_string;
	int cr_len;
	/* Component Indexing */
	int		cr_asn_type_id;
	slap_mask_t	cr_indexmask;
	AttributeDescription* cr_ad;
	BerVarray	cr_nvals;
	ComponentReference* cr_next;
};

struct ComponentAssertion {
	ComponentReference	*ca_comp_ref;
	ber_int_t		ca_use_def;
	MatchingRule		*ca_ma_rule;
	struct berval		ca_ma_value;
	ComponentData		ca_comp_data; /* componentized assertion */
	ComponentFilter		*ca_cf;
	MatchingRuleAssertion   *ca_mra;
};

struct ComponentFilter {
	ber_tag_t	cf_choice;
	union cf_un_u {
		ber_int_t		cf_un_result;
		ComponentAssertion	*cf_un_ca;
		ComponentFilter		*cf_un_complex;
	} cf_un;

#define cf_ca		cf_un.cf_un_ca
#define cf_result	cf_un.cf_un_result
#define cf_and		cf_un.cf_un_complex
#define cf_or		cf_un.cf_un_complex
#define cf_not		cf_un.cf_un_complex
#define cf_any		cf_un.cf_un_complex
	
	ComponentFilter	*cf_next;
};

struct ComponentAssertionValue {
	char* cav_buf;
	char* cav_ptr;
	char* cav_end;
};

typedef int encoder_func LDAP_P((
	void* b,
	void* comp));

typedef int gser_decoder_func LDAP_P((
	void* mem_op,
	void* b,
	ComponentSyntaxInfo** comp_syn_info,
	int* len,
	int mode));

typedef int comp_free_func LDAP_P((
	void* b));

typedef int ber_decoder_func LDAP_P((
	void* mem_op,
	void* b,
	int tag,
	int elmtLen,
	ComponentSyntaxInfo* comp_syn_info,
	int* len,
	int mode));

typedef int ber_tag_decoder_func LDAP_P((
	void* mem_op,
	void* b,
	ComponentSyntaxInfo* comp_syn_info,
	int* len,
	int mode));

typedef void* extract_component_from_id_func LDAP_P((
	void* mem_op,
	ComponentReference* cr,
	void* comp ));

typedef void* convert_attr_to_comp_func LDAP_P ((
        Attribute* a,
	Syntax* syn,
        struct berval* bv ));

typedef void* alloc_nibble_func LDAP_P ((
	int initial_size,
	int increment_size ));

typedef void free_nibble_func LDAP_P ((
	void* nm ));

typedef void convert_assert_to_comp_func LDAP_P ((
	void *mem_op,
        ComponentSyntaxInfo* csi_attr,
        struct berval* bv,
        ComponentSyntaxInfo** csi,
        int* len,
        int mode ));
                                                                          
typedef int convert_asn_to_ldap_func LDAP_P ((
        ComponentSyntaxInfo* csi,
        struct berval *bv ));

typedef void free_component_func LDAP_P ((
        void* mem_op));

typedef int test_component_func LDAP_P ((
	void* attr_mem_op,
	void* assert_mem_op,
        ComponentSyntaxInfo* csi,
	ComponentAssertion* ca));

typedef void* test_membership_func LDAP_P ((
	void* in ));

typedef void* get_component_info_func LDAP_P ((
	int in ));

typedef int component_encoder_func LDAP_P ((
	void* mem_op,
	ComponentSyntaxInfo* csi,
	struct berval* nvals ));
	
typedef int allcomponent_matching_func LDAP_P((
	char* oid,
	ComponentSyntaxInfo* comp1,
	ComponentSyntaxInfo* comp));

struct ComponentDesc {
	/* Don't change the order of following four fields */
	int				cd_tag;
	AttributeType			*cd_comp_type;
	struct berval			cd_ad_type;	/* ad_type, ad_cname */
	struct berval			cd_ad_cname;	/* ad_type, ad_cname */
	unsigned			cd_flags;	/* ad_flags */
	int				cd_type;
	int				cd_type_id;
	encoder_func			*cd_ldap_encoder;
	encoder_func			*cd_gser_encoder;
	encoder_func			*cd_ber_encoder;
	gser_decoder_func		*cd_gser_decoder;
	ber_decoder_func		*cd_ber_decoder;
	comp_free_func			*cd_free;
	extract_component_from_id_func*  cd_extract_i;
	allcomponent_matching_func	*cd_all_match;
};

struct ComponentSyntaxInfo {
	Syntax		*csi_syntax;
	ComponentDesc	*csi_comp_desc;
};

#endif /* LDAP_COMP_MATCH */

#ifdef SLAP_ZONE_ALLOC
#define SLAP_ZONE_SIZE 0x80000		/* 512KB */
#define SLAP_ZONE_SHIFT 19
#define SLAP_ZONE_INITSIZE 0x800000 /* 8MB */
#define SLAP_ZONE_MAXSIZE 0x80000000/* 2GB */
#define SLAP_ZONE_DELTA 0x800000	/* 8MB */
#define SLAP_ZONE_ZOBLOCK 256

struct zone_object {
	void *zo_ptr;
	int zo_siz;
	int zo_idx;
	int zo_blockhead;
	LDAP_LIST_ENTRY(zone_object) zo_link;
};

struct zone_latency_history {
	double zlh_latency;
	LDAP_STAILQ_ENTRY(zone_latency_history) zlh_next;
};

struct zone_heap {
	int zh_fd;
	int zh_zonesize;
	int zh_zoneorder;
	int zh_numzones;
	int zh_maxzones;
	int zh_deltazones;
	void **zh_zones;
	ldap_pvt_thread_rdwr_t *zh_znlock;
	Avlnode *zh_zonetree;
	unsigned char ***zh_maps;
	int *zh_seqno;
	LDAP_LIST_HEAD( zh_freelist, zone_object ) *zh_free;
	LDAP_LIST_HEAD( zh_so, zone_object ) zh_zopool;
	ldap_pvt_thread_mutex_t zh_mutex;
	ldap_pvt_thread_rdwr_t zh_lock;
	double zh_ema_latency;
	unsigned long zh_ema_samples;
	LDAP_STAILQ_HEAD( zh_latency_history, zone_latency_history )
				zh_latency_history_queue;
	int zh_latency_history_qlen;
	int zh_latency_jump;
	int zh_swapping;
};
#endif

#define SLAP_BACKEND_INIT_MODULE(b) \
	static BackendInfo bi;	\
	int \
	init_module( int argc, char *argv[] ) \
	{ \
		bi.bi_type = #b ; \
		bi.bi_init = b ## _back_initialize; \
		backend_add( &bi ); \
		return 0; \
	}

typedef int (OV_init)(void);
typedef struct slap_oinit_t {
	const char	*ov_type;
	OV_init		*ov_init;
} OverlayInit;

LDAP_END_DECL

#include "proto-slap.h"

#endif /* _SLAP_H_ */
