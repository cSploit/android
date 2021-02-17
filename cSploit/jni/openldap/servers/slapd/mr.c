/* mr.c - routines to manage matching rule definitions */
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

struct mindexrec {
	struct berval	mir_name;
	MatchingRule	*mir_mr;
};

static Avlnode	*mr_index = NULL;
static LDAP_SLIST_HEAD(MRList, MatchingRule) mr_list
	= LDAP_SLIST_HEAD_INITIALIZER(&mr_list);
static LDAP_SLIST_HEAD(MRUList, MatchingRuleUse) mru_list
	= LDAP_SLIST_HEAD_INITIALIZER(&mru_list);

static int
mr_index_cmp(
    const void	*v_mir1,
    const void	*v_mir2
)
{
	const struct mindexrec	*mir1 = v_mir1;
	const struct mindexrec	*mir2 = v_mir2;
	int i = mir1->mir_name.bv_len - mir2->mir_name.bv_len;
	if (i) return i;
	return (strcasecmp( mir1->mir_name.bv_val, mir2->mir_name.bv_val ));
}

static int
mr_index_name_cmp(
    const void	*v_name,
    const void	*v_mir
)
{
	const struct berval    *name = v_name;
	const struct mindexrec *mir  = v_mir;
	int i = name->bv_len - mir->mir_name.bv_len;
	if (i) return i;
	return (strncasecmp( name->bv_val, mir->mir_name.bv_val, name->bv_len ));
}

MatchingRule *
mr_find( const char *mrname )
{
	struct berval bv;

	bv.bv_val = (char *)mrname;
	bv.bv_len = strlen( mrname );
	return mr_bvfind( &bv );
}

MatchingRule *
mr_bvfind( struct berval *mrname )
{
	struct mindexrec	*mir = NULL;

	if ( (mir = avl_find( mr_index, mrname, mr_index_name_cmp )) != NULL ) {
		return( mir->mir_mr );
	}
	return( NULL );
}

void
mr_destroy( void )
{
	MatchingRule *m;

	avl_free(mr_index, ldap_memfree);
	while( !LDAP_SLIST_EMPTY(&mr_list) ) {
		m = LDAP_SLIST_FIRST(&mr_list);
		LDAP_SLIST_REMOVE_HEAD(&mr_list, smr_next);
		ch_free( m->smr_str.bv_val );
		ch_free( m->smr_compat_syntaxes );
		ldap_matchingrule_free((LDAPMatchingRule *)m);
	}
}

static int
mr_insert(
    MatchingRule	*smr,
    const char		**err
)
{
	struct mindexrec	*mir;
	char			**names;

	LDAP_SLIST_NEXT( smr, smr_next ) = NULL;
	LDAP_SLIST_INSERT_HEAD(&mr_list, smr, smr_next);

	if ( smr->smr_oid ) {
		mir = (struct mindexrec *)
			ch_calloc( 1, sizeof(struct mindexrec) );
		mir->mir_name.bv_val = smr->smr_oid;
		mir->mir_name.bv_len = strlen( smr->smr_oid );
		mir->mir_mr = smr;
		if ( avl_insert( &mr_index, (caddr_t) mir,
		                 mr_index_cmp, avl_dup_error ) ) {
			*err = smr->smr_oid;
			ldap_memfree(mir);
			return SLAP_SCHERR_MR_DUP;
		}
		/* FIX: temporal consistency check */
		mr_bvfind(&mir->mir_name);
	}
	if ( (names = smr->smr_names) ) {
		while ( *names ) {
			mir = (struct mindexrec *)
				ch_calloc( 1, sizeof(struct mindexrec) );
			mir->mir_name.bv_val = *names;
			mir->mir_name.bv_len = strlen( *names );
			mir->mir_mr = smr;
			if ( avl_insert( &mr_index, (caddr_t) mir,
			                 mr_index_cmp, avl_dup_error ) ) {
				*err = *names;
				ldap_memfree(mir);
				return SLAP_SCHERR_MR_DUP;
			}
			/* FIX: temporal consistency check */
			mr_bvfind(&mir->mir_name);
			names++;
		}
	}
	return 0;
}

int
mr_make_syntax_compat_with_mr(
	Syntax		*syn,
	MatchingRule	*mr )
{
	int		n = 0;

	assert( syn != NULL );
	assert( mr != NULL );

	if ( mr->smr_compat_syntaxes ) {
		/* count esisting */
		for ( n = 0;
			mr->smr_compat_syntaxes[ n ];
			n++ )
		{
			if ( mr->smr_compat_syntaxes[ n ] == syn ) {
				/* already compatible; mmmmh... */
				return 1;
			}
		}
	}

	mr->smr_compat_syntaxes = ch_realloc(
		mr->smr_compat_syntaxes,
		sizeof( Syntax * )*(n + 2) );
	mr->smr_compat_syntaxes[ n ] = syn;
	mr->smr_compat_syntaxes[ n + 1 ] = NULL;

	return 0;
}

int
mr_make_syntax_compat_with_mrs(
	const char *syntax,
	char *const *mrs )
{
	int	r, rc = 0;
	Syntax	*syn;

	assert( syntax != NULL );
	assert( mrs != NULL );

	syn = syn_find( syntax );
	if ( syn == NULL ) {
		return -1;
	}

	for ( r = 0; mrs[ r ] != NULL; r++ ) {
		MatchingRule	*mr = mr_find( mrs[ r ] );
		if ( mr == NULL ) {
			/* matchingRule not found -- ignore by now */
			continue;
		}

		rc += mr_make_syntax_compat_with_mr( syn, mr );
	}

	return rc;
}

int
mr_add(
    LDAPMatchingRule		*mr,
    slap_mrule_defs_rec	*def,
	MatchingRule	*amr,
    const char		**err
)
{
	MatchingRule	*smr;
	Syntax		*syn;
	Syntax		**compat_syn = NULL;
	int		code;

	if( def->mrd_compat_syntaxes ) {
		int i;
		for( i=0; def->mrd_compat_syntaxes[i]; i++ ) {
			/* just count em */
		}

		compat_syn = ch_malloc( sizeof(Syntax *) * (i+1) );

		for( i=0; def->mrd_compat_syntaxes[i]; i++ ) {
			compat_syn[i] = syn_find( def->mrd_compat_syntaxes[i] );
			if( compat_syn[i] == NULL ) {
				ch_free( compat_syn );
				return SLAP_SCHERR_SYN_NOT_FOUND;
			}
		}

		compat_syn[i] = NULL;
	}

	smr = (MatchingRule *) ch_calloc( 1, sizeof(MatchingRule) );
	AC_MEMCPY( &smr->smr_mrule, mr, sizeof(LDAPMatchingRule));

	/*
	 * note: smr_bvoid uses the same memory of smr_mrule.mr_oid;
	 * smr_oidlen is #defined as smr_bvoid.bv_len
	 */
	smr->smr_bvoid.bv_val = smr->smr_mrule.mr_oid;
	smr->smr_oidlen = strlen( mr->mr_oid );
	smr->smr_usage = def->mrd_usage;
	smr->smr_compat_syntaxes = compat_syn;
	smr->smr_normalize = def->mrd_normalize;
	smr->smr_match = def->mrd_match;
	smr->smr_indexer = def->mrd_indexer;
	smr->smr_filter = def->mrd_filter;
	smr->smr_associated = amr;

	if ( smr->smr_syntax_oid ) {
		if ( (syn = syn_find(smr->smr_syntax_oid)) ) {
			smr->smr_syntax = syn;
		} else {
			*err = smr->smr_syntax_oid;
			ch_free( smr );
			return SLAP_SCHERR_SYN_NOT_FOUND;
		}
	} else {
		*err = "";
		ch_free( smr );
		return SLAP_SCHERR_MR_INCOMPLETE;
	}
	code = mr_insert(smr,err);
	return code;
}

int
register_matching_rule(
	slap_mrule_defs_rec *def )
{
	LDAPMatchingRule *mr;
	MatchingRule *amr = NULL;
	int		code;
	const char	*err;

	if( def->mrd_usage == SLAP_MR_NONE && def->mrd_compat_syntaxes == NULL ) {
		Debug( LDAP_DEBUG_ANY, "register_matching_rule: not usable %s\n",
		    def->mrd_desc, 0, 0 );

		return -1;
	}

	if( def->mrd_associated != NULL ) {
		amr = mr_find( def->mrd_associated );
		if( amr == NULL ) {
			Debug( LDAP_DEBUG_ANY, "register_matching_rule: "
				"could not locate associated matching rule %s for %s\n",
				def->mrd_associated, def->mrd_desc, 0 );

			return -1;
		}

		if (( def->mrd_usage & SLAP_MR_EQUALITY ) &&
			(( def->mrd_usage & SLAP_MR_SUBTYPE_MASK ) == SLAP_MR_NONE ))
		{
			if (( def->mrd_usage & SLAP_MR_EQUALITY ) &&
				(( def->mrd_usage & SLAP_MR_SUBTYPE_MASK ) != SLAP_MR_NONE ))
			{
				Debug( LDAP_DEBUG_ANY, "register_matching_rule: "
						"inappropriate (approx) association %s for %s\n",
					def->mrd_associated, def->mrd_desc, 0 );
				return -1;
			}

		} else if (!( amr->smr_usage & SLAP_MR_EQUALITY )) {
				Debug( LDAP_DEBUG_ANY, "register_matching_rule: "
					"inappropriate (equalilty) association %s for %s\n",
					def->mrd_associated, def->mrd_desc, 0 );
				return -1;
		}
	}

	mr = ldap_str2matchingrule( def->mrd_desc, &code, &err,
		LDAP_SCHEMA_ALLOW_ALL );
	if ( !mr ) {
		Debug( LDAP_DEBUG_ANY,
			"Error in register_matching_rule: %s before %s in %s\n",
		    ldap_scherr2str(code), err, def->mrd_desc );

		return -1;
	}


	code = mr_add( mr, def, amr, &err );

	ldap_memfree( mr );

	if ( code ) {
		Debug( LDAP_DEBUG_ANY,
			"Error in register_matching_rule: %s for %s in %s\n",
		    scherr2str(code), err, def->mrd_desc );

		return -1;
	}

	return 0;
}

void
mru_destroy( void )
{
	MatchingRuleUse *m;

	while( !LDAP_SLIST_EMPTY(&mru_list) ) {
		m = LDAP_SLIST_FIRST(&mru_list);
		LDAP_SLIST_REMOVE_HEAD(&mru_list, smru_next);

		if ( m->smru_str.bv_val ) {
			ch_free( m->smru_str.bv_val );
			m->smru_str.bv_val = NULL;
		}
		/* memory borrowed from m->smru_mr */
		m->smru_oid = NULL;
		m->smru_names = NULL;
		m->smru_desc = NULL;

		/* free what's left (basically smru_mruleuse.mru_applies_oids) */
		ldap_matchingruleuse_free((LDAPMatchingRuleUse *)m);
	}
}

int
matching_rule_use_init( void )
{
	MatchingRule	*mr;
	MatchingRuleUse	**mru_ptr = &LDAP_SLIST_FIRST(&mru_list);

	Debug( LDAP_DEBUG_TRACE, "matching_rule_use_init\n", 0, 0, 0 );

	LDAP_SLIST_FOREACH( mr, &mr_list, smr_next ) {
		AttributeType	*at;
		MatchingRuleUse	mru_storage = {{ 0 }},
				*mru = &mru_storage;

		char		**applies_oids = NULL;

		mr->smr_mru = NULL;

		/* hide rules marked as HIDE */
		if ( mr->smr_usage & SLAP_MR_HIDE ) {
			continue;
		}

		/* hide rules not marked as designed for extensibility */
		/* MR_EXT means can be used any attribute type whose
		 * syntax is same as the assertion syntax.
		 * Another mechanism is needed where rule can be used
		 * with attribute of other syntaxes.
		 * Framework doesn't support this (yet).
		 */

		if (!( ( mr->smr_usage & SLAP_MR_EXT )
			|| mr->smr_compat_syntaxes ) )
		{
			continue;
		}

		/*
		 * Note: we're using the same values of the corresponding 
		 * MatchingRule structure; maybe we'd copy them ...
		 */
		mru->smru_mr = mr;
		mru->smru_obsolete = mr->smr_obsolete;
		mru->smru_applies_oids = NULL;
		LDAP_SLIST_NEXT(mru, smru_next) = NULL;
		mru->smru_oid = mr->smr_oid;
		mru->smru_names = mr->smr_names;
		mru->smru_desc = mr->smr_desc;

		Debug( LDAP_DEBUG_TRACE, "    %s (%s): ", 
				mru->smru_oid, 
				mru->smru_names ? mru->smru_names[ 0 ] : "", 0 );

		at = NULL;
		for ( at_start( &at ); at; at_next( &at ) ) {
			if( at->sat_flags & SLAP_AT_HIDE ) continue;

			if( mr_usable_with_at( mr, at )) {
				ldap_charray_add( &applies_oids, at->sat_cname.bv_val );
			}
		}

		/*
		 * Note: the matchingRules that are not used
		 * by any attributeType are not listed as
		 * matchingRuleUse
		 */
		if ( applies_oids != NULL ) {
			mru->smru_applies_oids = applies_oids;
			{
				char *str = ldap_matchingruleuse2str( &mru->smru_mruleuse );
				Debug( LDAP_DEBUG_TRACE, "matchingRuleUse: %s\n", str, 0, 0 );
				ldap_memfree( str );
			}

			mru = (MatchingRuleUse *)ber_memalloc( sizeof( MatchingRuleUse ) );
			/* call-forward from MatchingRule to MatchingRuleUse */
			mr->smr_mru = mru;
			/* copy static data to newly allocated struct */
			*mru = mru_storage;
			/* append the struct pointer to the end of the list */
			*mru_ptr = mru;
			/* update the list head pointer */
			mru_ptr = &LDAP_SLIST_NEXT(mru,smru_next);
		}
	}

	return( 0 );
}

int
mr_usable_with_at(
	MatchingRule	*mr,
	AttributeType	*at )
{
	if ( ( mr->smr_usage & SLAP_MR_EXT ) && (
		mr->smr_syntax == at->sat_syntax ||
		mr == at->sat_equality ||
		mr == at->sat_approx ||
		syn_is_sup( at->sat_syntax, mr->smr_syntax ) ) )
	{
		return 1;
	}

	if ( mr->smr_compat_syntaxes ) {
		int i;
		for( i=0; mr->smr_compat_syntaxes[i]; i++ ) {
			if( at->sat_syntax == mr->smr_compat_syntaxes[i] ) {
				return 1;
			}
		}
	}
	return 0;
}

int mr_schema_info( Entry *e )
{
	AttributeDescription *ad_matchingRules = slap_schema.si_ad_matchingRules;
	MatchingRule *mr;
	struct berval nval;

	LDAP_SLIST_FOREACH(mr, &mr_list, smr_next ) {
		if ( mr->smr_usage & SLAP_MR_HIDE ) {
			/* skip hidden rules */
			continue;
		}

		if ( ! mr->smr_match ) {
			/* skip rules without matching functions */
			continue;
		}

		if ( mr->smr_str.bv_val == NULL ) {
			if ( ldap_matchingrule2bv( &mr->smr_mrule, &mr->smr_str ) == NULL ) {
				return -1;
			}
		}
#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging mr [%lu] %s\n",
			mr->smr_str.bv_len, mr->smr_str.bv_val, 0 );
#endif

		nval.bv_val = mr->smr_oid;
		nval.bv_len = strlen(mr->smr_oid);
		if( attr_merge_one( e, ad_matchingRules, &mr->smr_str, &nval ) ) {
			return -1;
		}
	}
	return 0;
}

int mru_schema_info( Entry *e )
{
	AttributeDescription *ad_matchingRuleUse 
		= slap_schema.si_ad_matchingRuleUse;
	MatchingRuleUse	*mru;
	struct berval nval;

	LDAP_SLIST_FOREACH( mru, &mru_list, smru_next ) {
		assert( !( mru->smru_usage & SLAP_MR_HIDE ) );

		if ( mru->smru_str.bv_val == NULL ) {
			if ( ldap_matchingruleuse2bv( &mru->smru_mruleuse, &mru->smru_str )
					== NULL ) {
				return -1;
			}
		}

#if 0
		Debug( LDAP_DEBUG_TRACE, "Merging mru [%lu] %s\n",
			mru->smru_str.bv_len, mru->smru_str.bv_val, 0 );
#endif

		nval.bv_val = mru->smru_oid;
		nval.bv_len = strlen(mru->smru_oid);
		if( attr_merge_one( e, ad_matchingRuleUse, &mru->smru_str, &nval ) ) {
			return -1;
		}
	}
	return 0;
}
