/* index.c - index utilities */
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
#include <ac/string.h>
#include <lutil.h>

#include "slap.h"

static slap_verbmasks idxstr[] = {
	{ BER_BVC("pres"), SLAP_INDEX_PRESENT },
	{ BER_BVC("eq"), SLAP_INDEX_EQUALITY },
	{ BER_BVC("approx"), SLAP_INDEX_APPROX },
	{ BER_BVC("subinitial"), SLAP_INDEX_SUBSTR_INITIAL },
	{ BER_BVC("subany"), SLAP_INDEX_SUBSTR_ANY },
	{ BER_BVC("subfinal"), SLAP_INDEX_SUBSTR_FINAL },
	{ BER_BVC("sub"), SLAP_INDEX_SUBSTR_DEFAULT },
	{ BER_BVC("substr"), 0 },
	{ BER_BVC("notags"), SLAP_INDEX_NOTAGS },
	{ BER_BVC("nolang"), 0 },	/* backwards compat */
	{ BER_BVC("nosubtypes"), SLAP_INDEX_NOSUBTYPES },
	{ BER_BVNULL, 0 }
};


int slap_str2index( const char *str, slap_mask_t *idx )
{
	int i;

	i = verb_to_mask( str, idxstr );
	if ( BER_BVISNULL(&idxstr[i].word) ) return LDAP_OTHER;
	while ( !idxstr[i].mask ) i--;
	*idx = idxstr[i].mask;


	return LDAP_SUCCESS;
}

void slap_index2bvlen( slap_mask_t idx, struct berval *bv )
{
	int i;

	bv->bv_len = 0;

	for ( i=0; !BER_BVISNULL( &idxstr[i].word ); i++ ) {
		if ( !idxstr[i].mask ) continue;
		if ( IS_SLAP_INDEX( idx, idxstr[i].mask )) {
			if ( (idxstr[i].mask & SLAP_INDEX_SUBSTR) &&
				((idx & SLAP_INDEX_SUBSTR_DEFAULT) != idxstr[i].mask))
				continue;
			if ( bv->bv_len ) bv->bv_len++;
			bv->bv_len += idxstr[i].word.bv_len;
		}
	}
}

/* caller must provide buffer space, after calling index2bvlen */
void slap_index2bv( slap_mask_t idx, struct berval *bv )
{
	int i;
	char *ptr;

	if ( !bv->bv_len ) return;

	ptr = bv->bv_val;
	for ( i=0; !BER_BVISNULL( &idxstr[i].word ); i++ ) {
		if ( !idxstr[i].mask ) continue;
		if ( IS_SLAP_INDEX( idx, idxstr[i].mask )) {
			if ( (idxstr[i].mask & SLAP_INDEX_SUBSTR) &&
				((idx & SLAP_INDEX_SUBSTR_DEFAULT) != idxstr[i].mask))
				continue;
			if ( ptr != bv->bv_val ) *ptr++ = ',';
			ptr = lutil_strcopy( ptr, idxstr[i].word.bv_val );
		}
	}
}
