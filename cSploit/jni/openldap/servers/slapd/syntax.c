/* syntax.c - routines to manage syntax definitions */
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

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

struct sindexrec {
	char		*sir_name;
	Syntax		*sir_syn;
};

static Avlnode	*syn_index = NULL;
static LDAP_STAILQ_HEAD(SyntaxList, Syntax) syn_list
	= LDAP_STAILQ_HEAD_INITIALIZER(syn_list);

/* Last hardcoded attribute registered */
Syntax *syn_sys_tail;

static int
syn_index_cmp(
	const void *v_sir1,
	const void *v_sir2
)
{
	const struct sindexrec *sir1 = v_sir1, *sir2 = v_sir2;
	return (strcmp( sir1->sir_name, sir2->sir_name ));
}

static int
syn_index_name_cmp(
	const void *name,
	const void *sir
)
{
	return (strcmp( name, ((const struct sindexrec *)sir)->sir_name ));
}

Syntax *
syn_find( const char *synname )
{
	struct sindexrec	*sir = NULL;

	if ( (sir = avl_find( syn_index, synname, syn_index_name_cmp )) != NULL ) {
		return( sir->sir_syn );
	}
	return( NULL );
}

Syntax *
syn_find_desc( const char *syndesc, int *len )
{
	Syntax		*synp;

	LDAP_STAILQ_FOREACH(synp, &syn_list, ssyn_next) {
		if ((*len = dscompare( synp->ssyn_syn.syn_desc, syndesc, '{' /*'}'*/ ))) {
			return synp;
		}
	}
	return( NULL );
}

int
syn_is_sup( Syntax *syn, Syntax *sup )
{
	int	i;

	assert( syn != NULL );
	assert( sup != NULL );

	if ( syn == sup ) {
		return 1;
	}

	if ( syn->ssyn_sups == NULL ) {
		return 0;
	}

	for ( i = 0; syn->ssyn_sups[i]; i++ ) {
		if ( syn->ssyn_sups[i] == sup ) {
			return 1;
		}

		if ( syn_is_sup( syn->ssyn_sups[i], sup ) ) {
			return 1;
		}
	}

	return 0;
}

void
syn_destroy( void )
{
	Syntax	*s;

	avl_free( syn_index, ldap_memfree );
	while( !LDAP_STAILQ_EMPTY( &syn_list ) ) {
		s = LDAP_STAILQ_FIRST( &syn_list );
		LDAP_STAILQ_REMOVE_HEAD( &syn_list, ssyn_next );
		if ( s->ssyn_sups ) {
			SLAP_FREE( s->ssyn_sups );
		}
		ldap_syntax_free( (LDAPSyntax *)s );
	}
}

static int
syn_insert(
	Syntax		*ssyn,
	Syntax		*prev,
	const char	**err )
{
	struct sindexrec	*sir;

	LDAP_STAILQ_NEXT( ssyn, ssyn_next ) = NULL;
 
	if ( ssyn->ssyn_oid ) {
		sir = (struct sindexrec *)
			SLAP_CALLOC( 1, sizeof(struct sindexrec) );
		if( sir == NULL ) {
			Debug( LDAP_DEBUG_ANY, "SLAP_CALLOC Error\n", 0, 0, 0 );
			return LDAP_OTHER;
		}
		sir->sir_name = ssyn->ssyn_oid;
		sir->sir_syn = ssyn;
		if ( avl_insert( &syn_index, (caddr_t) sir,
		                 syn_index_cmp, avl_dup_error ) ) {
			*err = ssyn->ssyn_oid;
			ldap_memfree(sir);
			return SLAP_SCHERR_SYN_DUP;
		}
		/* FIX: temporal consistency check */
		syn_find(sir->sir_name);
	}

	if ( ssyn->ssyn_flags & SLAP_AT_HARDCODE ) {
		prev = syn_sys_tail;
		syn_sys_tail = ssyn;
	}

	if ( prev ) {
		LDAP_STAILQ_INSERT_AFTER( &syn_list, prev, ssyn, ssyn_next );
	} else {
		LDAP_STAILQ_INSERT_TAIL( &syn_list, ssyn, ssyn_next );
	}
	return 0;
}

int
syn_add(
	LDAPSyntax		*syn,
	int			user,
	slap_syntax_defs_rec	*def,
	Syntax			**ssynp,
	Syntax			*prev,
	const char		**err )
{
	Syntax		*ssyn;
	int		code = 0;

	if ( ssynp != NULL ) {
		*ssynp = NULL;
	}

	ssyn = (Syntax *) SLAP_CALLOC( 1, sizeof(Syntax) );
	if ( ssyn == NULL ) {
		Debug( LDAP_DEBUG_ANY, "SLAP_CALLOC Error\n", 0, 0, 0 );
		return SLAP_SCHERR_OUTOFMEM;
	}

	AC_MEMCPY( &ssyn->ssyn_syn, syn, sizeof(LDAPSyntax) );

	LDAP_STAILQ_NEXT(ssyn,ssyn_next) = NULL;

	/*
	 * note: ssyn_bvoid uses the same memory of ssyn_syn.syn_oid;
	 * ssyn_oidlen is #defined as ssyn_bvoid.bv_len
	 */
	ssyn->ssyn_bvoid.bv_val = ssyn->ssyn_syn.syn_oid;
	ssyn->ssyn_oidlen = strlen(syn->syn_oid);
	ssyn->ssyn_flags = def->sd_flags;
	ssyn->ssyn_validate = def->sd_validate;
	ssyn->ssyn_pretty = def->sd_pretty;

	ssyn->ssyn_sups = NULL;

#ifdef SLAPD_BINARY_CONVERSION
	ssyn->ssyn_ber2str = def->sd_ber2str;
	ssyn->ssyn_str2ber = def->sd_str2ber;
#endif

	if ( def->sd_validate == NULL && def->sd_pretty == NULL && syn->syn_extensions != NULL ) {
		LDAPSchemaExtensionItem **lsei;
		Syntax *subst = NULL;

		for ( lsei = syn->syn_extensions; *lsei != NULL; lsei++) {
			if ( strcmp( (*lsei)->lsei_name, "X-SUBST" ) != 0 ) {
				continue;
			}

			assert( (*lsei)->lsei_values != NULL );
			if ( (*lsei)->lsei_values[0] == '\0'
				|| (*lsei)->lsei_values[1] != '\0' )
			{
				Debug( LDAP_DEBUG_ANY, "syn_add(%s): exactly one substitute syntax must be present\n",
					ssyn->ssyn_syn.syn_oid, 0, 0 );
				return SLAP_SCHERR_SYN_SUBST_NOT_SPECIFIED;
			}

			subst = syn_find( (*lsei)->lsei_values[0] );
			if ( subst == NULL ) {
				Debug( LDAP_DEBUG_ANY, "syn_add(%s): substitute syntax %s not found\n",
					ssyn->ssyn_syn.syn_oid, (*lsei)->lsei_values[0], 0 );
				return SLAP_SCHERR_SYN_SUBST_NOT_FOUND;
			}
			break;
		}

		if ( subst != NULL ) {
			ssyn->ssyn_flags = subst->ssyn_flags;
			ssyn->ssyn_validate = subst->ssyn_validate;
			ssyn->ssyn_pretty = subst->ssyn_pretty;

			ssyn->ssyn_sups = NULL;

#ifdef SLAPD_BINARY_CONVERSION
			ssyn->ssyn_ber2str = subst->ssyn_ber2str;
			ssyn->ssyn_str2ber = subst->ssyn_str2ber;
#endif
		}
	}

	if ( def->sd_sups != NULL ) {
		int	cnt;

		for ( cnt = 0; def->sd_sups[cnt] != NULL; cnt++ )
			;
		
		ssyn->ssyn_sups = (Syntax **)SLAP_CALLOC( cnt + 1,
			sizeof( Syntax * ) );
		if ( ssyn->ssyn_sups == NULL ) {
			Debug( LDAP_DEBUG_ANY, "SLAP_CALLOC Error\n", 0, 0, 0 );
			code = SLAP_SCHERR_OUTOFMEM;

		} else {
			for ( cnt = 0; def->sd_sups[cnt] != NULL; cnt++ ) {
				ssyn->ssyn_sups[cnt] = syn_find( def->sd_sups[cnt] );
				if ( ssyn->ssyn_sups[cnt] == NULL ) {
					*err = def->sd_sups[cnt];
					code = SLAP_SCHERR_SYN_SUP_NOT_FOUND;
				}
			}
		}
	}

	if ( !user )
		ssyn->ssyn_flags |= SLAP_SYNTAX_HARDCODE;

	if ( code == 0 ) {
		code = syn_insert( ssyn, prev, err );
	}

	if ( code != 0 && ssyn != NULL ) {
		if ( ssyn->ssyn_sups != NULL ) {
			SLAP_FREE( ssyn->ssyn_sups );
		}
		SLAP_FREE( ssyn );
		ssyn = NULL;
	}

	if (ssynp ) {
		*ssynp = ssyn;
	}

	return code;
}

int
register_syntax(
	slap_syntax_defs_rec *def )
{
	LDAPSyntax	*syn;
	int		code;
	const char	*err;

	syn = ldap_str2syntax( def->sd_desc, &code, &err, LDAP_SCHEMA_ALLOW_ALL);
	if ( !syn ) {
		Debug( LDAP_DEBUG_ANY, "Error in register_syntax: %s before %s in %s\n",
		    ldap_scherr2str(code), err, def->sd_desc );

		return( -1 );
	}

	code = syn_add( syn, 0, def, NULL, NULL, &err );

	if ( code ) {
		Debug( LDAP_DEBUG_ANY, "Error in register_syntax: %s %s in %s\n",
		    scherr2str(code), err, def->sd_desc );
		ldap_syntax_free( syn );

		return( -1 );
	}

	ldap_memfree( syn );

	return( 0 );
}

int
syn_schema_info( Entry *e )
{
	AttributeDescription *ad_ldapSyntaxes = slap_schema.si_ad_ldapSyntaxes;
	Syntax		*syn;
	struct berval	val;
	struct berval	nval;

	LDAP_STAILQ_FOREACH(syn, &syn_list, ssyn_next ) {
		if ( ! syn->ssyn_validate ) {
			/* skip syntaxes without validators */
			continue;
		}
		if ( syn->ssyn_flags & SLAP_SYNTAX_HIDE ) {
			/* hide syntaxes */
			continue;
		}

		if ( ldap_syntax2bv( &syn->ssyn_syn, &val ) == NULL ) {
			return -1;
		}
#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging syn [%ld] %s\n",
	       (long) val.bv_len, val.bv_val, 0 );
#endif

		nval.bv_val = syn->ssyn_oid;
		nval.bv_len = strlen(syn->ssyn_oid);

		if( attr_merge_one( e, ad_ldapSyntaxes, &val, &nval ) )
		{
			return -1;
		}
		ldap_memfree( val.bv_val );
	}
	return 0;
}

void
syn_delete( Syntax *syn )
{
	LDAP_STAILQ_REMOVE(&syn_list, syn, Syntax, ssyn_next);
}

int
syn_start( Syntax **syn )
{
	assert( syn != NULL );

	*syn = LDAP_STAILQ_FIRST(&syn_list);

	return (*syn != NULL);
}

int
syn_next( Syntax **syn )
{
	assert( syn != NULL );

#if 0	/* pedantic check: don't use this */
	{
		Syntax *tmp = NULL;

		LDAP_STAILQ_FOREACH(tmp,&syn_list,ssyn_next) {
			if ( tmp == *syn ) {
				break;
			}
		}

		assert( tmp != NULL );
	}
#endif

	*syn = LDAP_STAILQ_NEXT(*syn,ssyn_next);

	return (*syn != NULL);
}

void
syn_unparse( BerVarray *res, Syntax *start, Syntax *end, int sys )
{
	Syntax *syn;
	int i, num;
	struct berval bv, *bva = NULL, idx;
	char ibuf[32];

	if ( !start )
		start = LDAP_STAILQ_FIRST( &syn_list );

	/* count the result size */
	i = 0;
	for ( syn = start; syn; syn = LDAP_STAILQ_NEXT( syn, ssyn_next ) ) {
		if ( sys && !( syn->ssyn_flags & SLAP_SYNTAX_HARDCODE ) ) break;
		i++;
		if ( syn == end ) break;
	}
	if ( !i ) return;

	num = i;
	bva = ch_malloc( (num+1) * sizeof(struct berval) );
	BER_BVZERO( bva );
	idx.bv_val = ibuf;
	if ( sys ) {
		idx.bv_len = 0;
		ibuf[0] = '\0';
	}
	i = 0;
	for ( syn = start; syn; syn = LDAP_STAILQ_NEXT( syn, ssyn_next ) ) {
		if ( sys && !( syn->ssyn_flags & SLAP_SYNTAX_HARDCODE ) ) break;
		if ( ldap_syntax2bv( &syn->ssyn_syn, &bv ) == NULL ) {
			ber_bvarray_free( bva );
		}
		if ( !sys ) {
			idx.bv_len = sprintf(idx.bv_val, "{%d}", i);
		}
		bva[i].bv_len = idx.bv_len + bv.bv_len;
		bva[i].bv_val = ch_malloc( bva[i].bv_len + 1 );
		strcpy( bva[i].bv_val, ibuf );
		strcpy( bva[i].bv_val + idx.bv_len, bv.bv_val );
		i++;
		bva[i].bv_val = NULL;
		ldap_memfree( bv.bv_val );
		if ( syn == end ) break;
	}
	*res = bva;
}

