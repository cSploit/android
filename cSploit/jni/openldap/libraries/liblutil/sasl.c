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

#ifdef HAVE_CYRUS_SASL

#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/unistd.h>

#ifdef HAVE_SASL_SASL_H
#include <sasl/sasl.h>
#else
#include <sasl.h>
#endif

#include <ldap.h>
#include "ldap_pvt.h"
#include "lutil_ldap.h"


typedef struct lutil_sasl_defaults_s {
	char *mech;
	char *realm;
	char *authcid;
	char *passwd;
	char *authzid;
	char **resps;
	int nresps;
} lutilSASLdefaults;


void
lutil_sasl_freedefs(
	void *defaults )
{
	lutilSASLdefaults *defs = defaults;

	assert( defs != NULL );
	
	if (defs->mech) ber_memfree(defs->mech);
	if (defs->realm) ber_memfree(defs->realm);
	if (defs->authcid) ber_memfree(defs->authcid);
	if (defs->passwd) ber_memfree(defs->passwd);
	if (defs->authzid) ber_memfree(defs->authzid);
	if (defs->resps) ldap_charray_free(defs->resps);

	ber_memfree(defs);
}

void *
lutil_sasl_defaults(
	LDAP *ld,
	char *mech,
	char *realm,
	char *authcid,
	char *passwd,
	char *authzid )
{
	lutilSASLdefaults *defaults;
	
	defaults = ber_memalloc( sizeof( lutilSASLdefaults ) );

	if( defaults == NULL ) return NULL;

	defaults->mech = mech ? ber_strdup(mech) : NULL;
	defaults->realm = realm ? ber_strdup(realm) : NULL;
	defaults->authcid = authcid ? ber_strdup(authcid) : NULL;
	defaults->passwd = passwd ? ber_strdup(passwd) : NULL;
	defaults->authzid = authzid ? ber_strdup(authzid) : NULL;

	if( defaults->mech == NULL ) {
		ldap_get_option( ld, LDAP_OPT_X_SASL_MECH, &defaults->mech );
	}
	if( defaults->realm == NULL ) {
		ldap_get_option( ld, LDAP_OPT_X_SASL_REALM, &defaults->realm );
	}
	if( defaults->authcid == NULL ) {
		ldap_get_option( ld, LDAP_OPT_X_SASL_AUTHCID, &defaults->authcid );
	}
	if( defaults->authzid == NULL ) {
		ldap_get_option( ld, LDAP_OPT_X_SASL_AUTHZID, &defaults->authzid );
	}
	defaults->resps = NULL;
	defaults->nresps = 0;

	return defaults;
}

static int interaction(
	unsigned flags,
	sasl_interact_t *interact,
	lutilSASLdefaults *defaults )
{
	const char *dflt = interact->defresult;
	char input[1024];

	int noecho=0;
	int challenge=0;

	switch( interact->id ) {
	case SASL_CB_GETREALM:
		if( defaults ) dflt = defaults->realm;
		break;
	case SASL_CB_AUTHNAME:
		if( defaults ) dflt = defaults->authcid;
		break;
	case SASL_CB_PASS:
		if( defaults ) dflt = defaults->passwd;
		noecho = 1;
		break;
	case SASL_CB_USER:
		if( defaults ) dflt = defaults->authzid;
		break;
	case SASL_CB_NOECHOPROMPT:
		noecho = 1;
		challenge = 1;
		break;
	case SASL_CB_ECHOPROMPT:
		challenge = 1;
		break;
	}

	if( dflt && !*dflt ) dflt = NULL;

	if( flags != LDAP_SASL_INTERACTIVE &&
		( dflt || interact->id == SASL_CB_USER ) )
	{
		goto use_default;
	}

	if( flags == LDAP_SASL_QUIET ) {
		/* don't prompt */
		return LDAP_OTHER;
	}

	if( challenge ) {
		if( interact->challenge ) {
			fprintf( stderr, _("Challenge: %s\n"), interact->challenge );
		}
	}

	if( dflt ) {
		fprintf( stderr, _("Default: %s\n"), dflt );
	}

	snprintf( input, sizeof input, "%s: ",
		interact->prompt ? interact->prompt : _("Interact") );

	if( noecho ) {
		interact->result = (char *) getpassphrase( input );
		interact->len = interact->result
			? strlen( interact->result ) : 0;

	} else {
		/* prompt user */
		fputs( input, stderr );

		/* get input */
		interact->result = fgets( input, sizeof(input), stdin );

		if( interact->result == NULL ) {
			interact->len = 0;
			return LDAP_UNAVAILABLE;
		}

		/* len of input */
		interact->len = strlen(input); 

		if( interact->len > 0 && input[interact->len - 1] == '\n' ) {
			/* input includes '\n', trim it */
			interact->len--;
			input[interact->len] = '\0';
		}
	}


	if( interact->len > 0 ) {
		/* duplicate */
		char *p = (char *)interact->result;
		ldap_charray_add(&defaults->resps, interact->result);
		interact->result = defaults->resps[defaults->nresps++];

		/* zap */
		memset( p, '\0', interact->len );

	} else {
use_default:
		/* input must be empty */
		interact->result = (dflt && *dflt) ? dflt : "";
		interact->len = strlen( interact->result );
	}

	return LDAP_SUCCESS;
}

int lutil_sasl_interact(
	LDAP *ld,
	unsigned flags,
	void *defaults,
	void *in )
{
	sasl_interact_t *interact = in;

	if( ld == NULL ) return LDAP_PARAM_ERROR;

	if( flags == LDAP_SASL_INTERACTIVE ) {
		fputs( _("SASL Interaction\n"), stderr );
	}

	while( interact->id != SASL_CB_LIST_END ) {
		int rc = interaction( flags, interact, defaults );

		if( rc )  return rc;
		interact++;
	}
	
	return LDAP_SUCCESS;
}
#endif
