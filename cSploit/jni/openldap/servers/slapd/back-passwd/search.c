/* search.c - /etc/passwd backend search function */
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
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).  Additional significant contributors
 * include:
 *     Hallvard B. Furuseth
 *     Howard Chu
 *     Kurt D. Zeilenga
 */

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include <pwd.h>

#include "slap.h"
#include "back-passwd.h"

static void pw_start( Backend *be );

static int pw2entry(
	Backend		*be,
	struct passwd	*pw,
	Entry		*ep );

int
passwd_back_search(
    Operation	*op,
    SlapReply	*rs )
{
	struct passwd	*pw;
	time_t		stoptime = (time_t)-1;

	LDAPRDN rdn = NULL;
	struct berval parent = BER_BVNULL;

	AttributeDescription *ad_objectClass = slap_schema.si_ad_objectClass;

	if ( op->ors_tlimit != SLAP_NO_LIMIT ) {
		stoptime = op->o_time + op->ors_tlimit;
	}

	/* Handle a query for the base of this backend */
	if ( be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
		struct berval	val;

		rs->sr_matched = op->o_req_dn.bv_val;

		if( op->ors_scope != LDAP_SCOPE_ONELEVEL ) {
			AttributeDescription	*desc = NULL;
			char			*next;
			Entry			e = { 0 };

			/* Create an entry corresponding to the base DN */
			e.e_name.bv_val = ch_strdup( op->o_req_dn.bv_val );
			e.e_name.bv_len = op->o_req_dn.bv_len;
			e.e_nname.bv_val =  ch_strdup( op->o_req_ndn.bv_val );
			e.e_nname.bv_len = op->o_req_ndn.bv_len;

			/* Use the first attribute of the DN
		 	* as an attribute within the entry itself.
		 	*/
			if( ldap_bv2rdn( &op->o_req_dn, &rdn, &next, 
				LDAP_DN_FORMAT_LDAP ) )
			{
				rs->sr_err = LDAP_INVALID_DN_SYNTAX;
				goto done;
			}

			if( slap_bv2ad( &rdn[0]->la_attr, &desc, &rs->sr_text )) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				ldap_rdnfree(rdn);
				goto done;
			}

			attr_merge_normalize_one( &e, desc, &rdn[0]->la_value, NULL );

			ldap_rdnfree(rdn);
			rdn = NULL;

			/* Every entry needs an objectclass. We don't really
			 * know if our hardcoded choice here agrees with the
			 * DN that was configured for this backend, but it's
			 * better than nothing.
			 *
			 * should be a configuratable item
			 */
			BER_BVSTR( &val, "organizationalUnit" );
			attr_merge_one( &e, ad_objectClass, &val, NULL );
	
			if ( test_filter( op, &e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
				rs->sr_entry = &e;
				rs->sr_attrs = op->ors_attrs;
				rs->sr_flags = REP_ENTRY_MODIFIABLE;
				send_search_entry( op, rs );
				rs->sr_flags = 0;
				rs->sr_attrs = NULL;
			}

			entry_clean( &e );
		}

		if ( op->ors_scope != LDAP_SCOPE_BASE ) {
			/* check all our "children" */

			ldap_pvt_thread_mutex_lock( &passwd_mutex );
			pw_start( op->o_bd );
			for ( pw = getpwent(); pw != NULL; pw = getpwent() ) {
				Entry		e = { 0 };

				/* check for abandon */
				if ( op->o_abandon ) {
					endpwent();
					ldap_pvt_thread_mutex_unlock( &passwd_mutex );
					return( SLAPD_ABANDON );
				}

				/* check time limit */
				if ( op->ors_tlimit != SLAP_NO_LIMIT
						&& slap_get_time() > stoptime )
				{
					send_ldap_error( op, rs, LDAP_TIMELIMIT_EXCEEDED, NULL );
					endpwent();
					ldap_pvt_thread_mutex_unlock( &passwd_mutex );
					return( 0 );
				}

				if ( pw2entry( op->o_bd, pw, &e ) ) {
					rs->sr_err = LDAP_OTHER;
					endpwent();
					ldap_pvt_thread_mutex_unlock( &passwd_mutex );
					goto done;
				}

				if ( test_filter( op, &e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
					/* check size limit */
					if ( --op->ors_slimit == -1 ) {
						send_ldap_error( op, rs, LDAP_SIZELIMIT_EXCEEDED, NULL );
						endpwent();
						ldap_pvt_thread_mutex_unlock( &passwd_mutex );
						return( 0 );
					}

					rs->sr_entry = &e;
					rs->sr_attrs = op->ors_attrs;
					rs->sr_flags = REP_ENTRY_MODIFIABLE;
					send_search_entry( op, rs );
					rs->sr_flags = 0;
					rs->sr_entry = NULL;
				}

				entry_clean( &e );
			}
			endpwent();
			ldap_pvt_thread_mutex_unlock( &passwd_mutex );
		}

	} else {
		char	*next;
		Entry	e = { 0 };
		int	rc;

		if (! be_issuffix( op->o_bd, &op->o_req_ndn ) ) {
			dnParent( &op->o_req_ndn, &parent );
		}

		/* This backend is only one layer deep. Don't answer requests for
		 * anything deeper than that.
		 */
		if( !be_issuffix( op->o_bd, &parent ) ) {
			int i;
			for( i=0; op->o_bd->be_nsuffix[i].bv_val != NULL; i++ ) {
				if( dnIsSuffix( &op->o_req_ndn, &op->o_bd->be_nsuffix[i] ) ) {
					rs->sr_matched = op->o_bd->be_suffix[i].bv_val;
					break;
				}
			}
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			goto done;
		}

		if( op->ors_scope == LDAP_SCOPE_ONELEVEL ) {
			goto done;
		}

		if ( ldap_bv2rdn( &op->o_req_dn, &rdn, &next,
			LDAP_DN_FORMAT_LDAP ))
		{ 
			rs->sr_err = LDAP_OTHER;
			goto done;
		}

		ldap_pvt_thread_mutex_lock( &passwd_mutex );
		pw_start( op->o_bd );
		pw = getpwnam( rdn[0]->la_value.bv_val );
		if ( pw == NULL ) {
			rs->sr_matched = parent.bv_val;
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
			ldap_pvt_thread_mutex_unlock( &passwd_mutex );
			goto done;
		}

		rc = pw2entry( op->o_bd, pw, &e );
		ldap_pvt_thread_mutex_unlock( &passwd_mutex );
		if ( rc ) {
			rs->sr_err = LDAP_OTHER;
			goto done;
		}

		if ( test_filter( op, &e, op->ors_filter ) == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = &e;
			rs->sr_attrs = op->ors_attrs;
			rs->sr_flags = REP_ENTRY_MODIFIABLE;
			send_search_entry( op, rs );
			rs->sr_flags = 0;
			rs->sr_entry = NULL;
			rs->sr_attrs = NULL;
		}

		entry_clean( &e );
	}

done:
	if( rs->sr_err != LDAP_NO_SUCH_OBJECT ) rs->sr_matched = NULL;
	send_ldap_result( op, rs );

	if( rdn != NULL ) ldap_rdnfree( rdn );

	return( 0 );
}

static void
pw_start(
	Backend *be
)
{
	endpwent();

#ifdef HAVE_SETPWFILE
	if ( be->be_private != NULL ) {
		(void) setpwfile( (char *) be->be_private );
	}
#endif /* HAVE_SETPWFILE */
}

static int
pw2entry( Backend *be, struct passwd *pw, Entry *e )
{
	size_t		pwlen;
	struct berval	val;
	struct berval	bv;

	int		rc;

	/*
	 * from pw we get pw_name and make it cn
	 * give it an objectclass of person.
	 */

	pwlen = strlen( pw->pw_name );
	val.bv_len = STRLENOF("uid=,") + ( pwlen + be->be_suffix[0].bv_len );
	val.bv_val = ch_malloc( val.bv_len + 1 );

	/* rdn attribute type should be a configuratable item */
	sprintf( val.bv_val, "uid=%s,%s",
		pw->pw_name, be->be_suffix[0].bv_val );

	rc = dnNormalize( 0, NULL, NULL, &val, &bv, NULL );
	if( rc != LDAP_SUCCESS ) {
		free( val.bv_val );
		return( -1 );
	}

	e->e_name = val;
	e->e_nname = bv;

	e->e_attrs = NULL;

	/* objectclasses should be configurable items */
	BER_BVSTR( &val, "person" );
	attr_merge_one( e, slap_schema.si_ad_objectClass, &val, NULL );

	BER_BVSTR( &val, "uidObject" );
	attr_merge_one( e, slap_schema.si_ad_objectClass, &val, NULL );

	val.bv_val = pw->pw_name;
	val.bv_len = pwlen;
	attr_merge_normalize_one( e, slap_schema.si_ad_uid, &val, NULL );	/* required by uidObject */
	attr_merge_normalize_one( e, slap_schema.si_ad_cn, &val, NULL );	/* required by person */
	attr_merge_normalize_one( e, ad_sn, &val, NULL );	/* required by person */

#ifdef HAVE_STRUCT_PASSWD_PW_GECOS
	/*
	 * if gecos is present, add it as a cn. first process it
	 * according to standard BSD usage. If the processed cn has
	 * a space, use the tail as the surname.
	 */
	if (pw->pw_gecos[0]) {
		char *s;

		ber_str2bv( pw->pw_gecos, 0, 0, &val );
		attr_merge_normalize_one( e, ad_desc, &val, NULL );

		s = ber_bvchr( &val, ',' );
		if ( s ) *s = '\0';

		s = ber_bvchr( &val, '&' );
		if ( s ) {
			char buf[1024];

			if( val.bv_len + pwlen < sizeof(buf) ) {
				int i = s - val.bv_val;
				strncpy( buf, val.bv_val, i );
				s = buf + i;
				strcpy( s, pw->pw_name );
				*s = TOUPPER((unsigned char)*s);
				strcat( s, val.bv_val + i + 1 );
				val.bv_val = buf;
			}
		}
		val.bv_len = strlen( val.bv_val );

		if ( val.bv_len && strcasecmp( val.bv_val, pw->pw_name ) ) {
			attr_merge_normalize_one( e, slap_schema.si_ad_cn, &val, NULL );
		}

		if ( ( s = strrchr(val.bv_val, ' ' ) ) ) {
			ber_str2bv( s + 1, 0, 0, &val );
			attr_merge_normalize_one( e, ad_sn, &val, NULL );
		}
	}
#endif /* HAVE_STRUCT_PASSWD_PW_GECOS */

	return( 0 );
}
