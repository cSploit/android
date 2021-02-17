/* schema.c - routines to manage schema definitions */
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
#include "lutil.h"


int
schema_info( Entry **entry, const char **text )
{
	AttributeDescription *ad_structuralObjectClass
		= slap_schema.si_ad_structuralObjectClass;
	AttributeDescription *ad_objectClass
		= slap_schema.si_ad_objectClass;
	AttributeDescription *ad_createTimestamp
		= slap_schema.si_ad_createTimestamp;
	AttributeDescription *ad_modifyTimestamp
		= slap_schema.si_ad_modifyTimestamp;

	Entry		*e;
	struct berval	vals[5];
	struct berval	nvals[5];

	e = entry_alloc();
	if( e == NULL ) {
		/* Out of memory, do something about it */
		Debug( LDAP_DEBUG_ANY, 
			"schema_info: entry_alloc failed - out of memory.\n", 0, 0, 0 );
		*text = "out of memory";
		return LDAP_OTHER;
	}

	e->e_attrs = NULL;
	/* backend-specific schema info should be created by the
	 * backend itself
	 */
	ber_dupbv( &e->e_name, &frontendDB->be_schemadn );
	ber_dupbv( &e->e_nname, &frontendDB->be_schemandn );
	e->e_private = NULL;

	BER_BVSTR( &vals[0], "subentry" );
	if( attr_merge_one( e, ad_structuralObjectClass, vals, NULL ) ) {
		/* Out of memory, do something about it */
		entry_free( e );
		*text = "out of memory";
		return LDAP_OTHER;
	}

	BER_BVSTR( &vals[0], "top" );
	BER_BVSTR( &vals[1], "subentry" );
	BER_BVSTR( &vals[2], "subschema" );
	BER_BVSTR( &vals[3], "extensibleObject" );
	BER_BVZERO( &vals[4] );
	if ( attr_merge( e, ad_objectClass, vals, NULL ) ) {
		/* Out of memory, do something about it */
		entry_free( e );
		*text = "out of memory";
		return LDAP_OTHER;
	}

	{
		int rc;
		AttributeDescription *desc = NULL;
		struct berval rdn = frontendDB->be_schemadn;
		vals[0].bv_val = ber_bvchr( &rdn, '=' );

		if( vals[0].bv_val == NULL ) {
			*text = "improperly configured subschema subentry";
			return LDAP_OTHER;
		}

		vals[0].bv_val++;
		vals[0].bv_len = rdn.bv_len - (vals[0].bv_val - rdn.bv_val);
		rdn.bv_len -= vals[0].bv_len + 1;

		rc = slap_bv2ad( &rdn, &desc, text );

		if( rc != LDAP_SUCCESS ) {
			entry_free( e );
			*text = "improperly configured subschema subentry";
			return LDAP_OTHER;
		}

		nvals[0].bv_val = ber_bvchr( &frontendDB->be_schemandn, '=' );
		assert( nvals[0].bv_val != NULL );
		nvals[0].bv_val++;
		nvals[0].bv_len = frontendDB->be_schemandn.bv_len -
			(nvals[0].bv_val - frontendDB->be_schemandn.bv_val);

		if ( attr_merge_one( e, desc, vals, nvals ) ) {
			/* Out of memory, do something about it */
			entry_free( e );
			*text = "out of memory";
			return LDAP_OTHER;
		}
	}

	{
		char		timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];

		/*
		 * According to RFC 4512:

   Servers SHOULD maintain the 'creatorsName', 'createTimestamp',       
   'modifiersName', and 'modifyTimestamp' attributes for all entries of 
   the DIT. 

		 * to be conservative, we declare schema created 
		 * AND modified at server startup time ...
		 */

		vals[0].bv_val = timebuf;
		vals[0].bv_len = sizeof( timebuf );

		slap_timestamp( &starttime, vals );

		if( attr_merge_one( e, ad_createTimestamp, vals, NULL ) ) {
			/* Out of memory, do something about it */
			entry_free( e );
			*text = "out of memory";
			return LDAP_OTHER;
		}
		if( attr_merge_one( e, ad_modifyTimestamp, vals, NULL ) ) {
			/* Out of memory, do something about it */
			entry_free( e );
			*text = "out of memory";
			return LDAP_OTHER;
		}
	}

	if ( syn_schema_info( e ) 
		|| mr_schema_info( e )
		|| mru_schema_info( e )
		|| at_schema_info( e )
		|| oc_schema_info( e )
		|| cr_schema_info( e ) )
	{
		/* Out of memory, do something about it */
		entry_free( e );
		*text = "out of memory";
		return LDAP_OTHER;
	}
	
	*entry = e;
	return LDAP_SUCCESS;
}
