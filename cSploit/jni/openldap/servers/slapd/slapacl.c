/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>

#include "slapcommon.h"

static int
print_access(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	struct berval		*nval )
{
	int			rc;
	slap_mask_t		mask;
	char			accessmaskbuf[ACCESSMASK_MAXLEN];

	rc = access_allowed_mask( op, e, desc, nval, ACL_AUTH, NULL, &mask );

	fprintf( stderr, "%s%s%s: %s\n",
			desc->ad_cname.bv_val,
			( val && !BER_BVISNULL( val ) ) ? "=" : "",
			( val && !BER_BVISNULL( val ) ) ?
				( desc == slap_schema.si_ad_userPassword ?
					"****" : val->bv_val ) : "",
			accessmask2str( mask, accessmaskbuf, 1 ) );

	return rc;
}

int
slapacl( int argc, char **argv )
{
	int			rc = EXIT_SUCCESS;
	const char		*progname = "slapacl";
	Connection		conn = { 0 };
	Listener		listener;
	OperationBuffer	opbuf;
	Operation		*op = NULL;
	Entry			e = { 0 }, *ep = &e;
	char			*attr = NULL;
	int			doclose = 0;
	BackendDB		*bd;
	void			*thrctx;

	slap_tool_init( progname, SLAPACL, argc, argv );

	if ( !dryrun ) {
		int	i = 0;

		LDAP_STAILQ_FOREACH( bd, &backendDB, be_next ) {
			if ( bd != be && backend_startup( bd ) ) {
				fprintf( stderr, "backend_startup(#%d%s%s) failed\n",
						i,
						bd->be_suffix ? ": " : "",
						bd->be_suffix ? bd->be_suffix[0].bv_val : "" );
				rc = 1;
				goto destroy;
			}

			i++;
		}
	}

	argv = &argv[ optind ];
	argc -= optind;

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init( &conn, &opbuf, thrctx );
	op = &opbuf.ob_op;
	op->o_tmpmemctx = NULL;

	conn.c_listener = &listener;
	conn.c_listener_url = listener_url;
	conn.c_peer_domain = peer_domain;
	conn.c_peer_name = peer_name;
	conn.c_sock_name = sock_name;
	op->o_ssf = ssf;
	op->o_transport_ssf = transport_ssf;
	op->o_tls_ssf = tls_ssf;
	op->o_sasl_ssf = sasl_ssf;

	if ( !BER_BVISNULL( &authcID ) ) {
		if ( !BER_BVISNULL( &authcDN ) ) {
			fprintf( stderr, "both authcID=\"%s\" "
					"and authcDN=\"%s\" provided\n",
					authcID.bv_val, authcDN.bv_val );
			rc = 1;
			goto destroy;
		}

		rc = slap_sasl_getdn( &conn, op, &authcID, NULL,
				&authcDN, SLAP_GETDN_AUTHCID );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "authcID: <%s> check failed %d (%s)\n",
					authcID.bv_val, rc,
					ldap_err2string( rc ) );
			rc = 1;
			goto destroy;
		}

	} else if ( !BER_BVISNULL( &authcDN ) ) {
		struct berval	ndn;

		rc = dnNormalize( 0, NULL, NULL, &authcDN, &ndn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "autchDN=\"%s\" normalization failed %d (%s)\n",
					authcDN.bv_val, rc,
					ldap_err2string( rc ) );
			rc = 1;
			goto destroy;
		}
		ch_free( authcDN.bv_val );
		authcDN = ndn;
	}

	if ( !BER_BVISNULL( &authzID ) ) {
		if ( !BER_BVISNULL( &authzDN ) ) {
			fprintf( stderr, "both authzID=\"%s\" "
					"and authzDN=\"%s\" provided\n",
					authzID.bv_val, authzDN.bv_val );
			rc = 1;
			goto destroy;
		}

		rc = slap_sasl_getdn( &conn, op, &authzID, NULL,
				&authzDN, SLAP_GETDN_AUTHZID );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "authzID: <%s> check failed %d (%s)\n",
					authzID.bv_val, rc,
					ldap_err2string( rc ) );
			rc = 1;
			goto destroy;
		}

	} else if ( !BER_BVISNULL( &authzDN ) ) {
		struct berval	ndn;

		rc = dnNormalize( 0, NULL, NULL, &authzDN, &ndn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "autchDN=\"%s\" normalization failed %d (%s)\n",
					authzDN.bv_val, rc,
					ldap_err2string( rc ) );
			rc = 1;
			goto destroy;
		}
		ch_free( authzDN.bv_val );
		authzDN = ndn;
	}


	if ( !BER_BVISNULL( &authcDN ) ) {
		fprintf( stderr, "authcDN: \"%s\"\n", authcDN.bv_val );
	}

	if ( !BER_BVISNULL( &authzDN ) ) {
		fprintf( stderr, "authzDN: \"%s\"\n", authzDN.bv_val );
	}

	if ( !BER_BVISNULL( &authzDN ) ) {
		op->o_dn = authzDN;
		op->o_ndn = authzDN;
		
		if ( !BER_BVISNULL( &authcDN ) ) {
			op->o_conn->c_dn = authcDN;
			op->o_conn->c_ndn = authcDN;

		} else {
			op->o_conn->c_dn = authzDN;
			op->o_conn->c_ndn = authzDN;
		}

	} else if ( !BER_BVISNULL( &authcDN ) ) {
		op->o_conn->c_dn = authcDN;
		op->o_conn->c_ndn = authcDN;
		op->o_dn = authcDN;
		op->o_ndn = authcDN;
	}

	assert( !BER_BVISNULL( &baseDN ) );
	rc = dnPrettyNormal( NULL, &baseDN, &e.e_name, &e.e_nname, NULL );
	if ( rc != LDAP_SUCCESS ) {
		fprintf( stderr, "base=\"%s\" normalization failed %d (%s)\n",
				baseDN.bv_val, rc,
				ldap_err2string( rc ) );
		rc = 1;
		goto destroy;
	}

	op->o_bd = be;
	if ( op->o_bd == NULL ) {
		/* NOTE: if no database could be found (e.g. because
		 * accessing the rootDSE or so), use the frontendDB
		 * rules; might need work */
		op->o_bd = frontendDB;
	}

	if ( !dryrun ) {
		ID	id;

		if ( be == NULL ) {
			fprintf( stderr, "%s: no target database "
				"has been found for baseDN=\"%s\"; "
				"you may try with \"-u\" (dry run).\n",
				baseDN.bv_val, progname );
			rc = 1;
			goto destroy;
		}

		if ( !be->be_entry_open ||
			!be->be_entry_close ||
			!be->be_dn2id_get ||
			!be->be_entry_get )
		{
			fprintf( stderr, "%s: target database "
				"doesn't support necessary operations; "
				"you may try with \"-u\" (dry run).\n",
				progname );
			rc = 1;
			goto destroy;
		}

		if ( be->be_entry_open( be, 0 ) != 0 ) {
			fprintf( stderr, "%s: could not open database.\n",
				progname );
			rc = 1;
			goto destroy;
		}

		doclose = 1;

		id = be->be_dn2id_get( be, &e.e_nname );
		if ( id == NOID ) {
			fprintf( stderr, "%s: unable to fetch ID of DN \"%s\"\n",
				progname, e.e_nname.bv_val );
			rc = 1;
			goto destroy;
		}
		ep = be->be_entry_get( be, id );
		if ( ep == NULL ) {
			fprintf( stderr, "%s: unable to fetch entry \"%s\" (%lu)\n",
				progname, e.e_nname.bv_val, id );
			rc = 1;
			goto destroy;

		}

		if ( argc == 0 ) {
			Attribute	*a;

			(void)print_access( op, ep, slap_schema.si_ad_entry, NULL, NULL );
			(void)print_access( op, ep, slap_schema.si_ad_children, NULL, NULL );

			for ( a = ep->e_attrs; a; a = a->a_next ) {
				int	i;

				for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
					(void)print_access( op, ep, a->a_desc,
							&a->a_vals[ i ],
							&a->a_nvals[ i ] );
				}
			}
		}
	}

	for ( ; argc--; argv++ ) {
		slap_mask_t		mask;
		AttributeDescription	*desc = NULL;
		struct berval		val = BER_BVNULL,
					*valp = NULL;
		const char		*text;
		char			accessmaskbuf[ACCESSMASK_MAXLEN];
		char			*accessstr;
		slap_access_t		access = ACL_AUTH;

		if ( attr == NULL ) {
			attr = argv[ 0 ];
		}

		val.bv_val = strchr( attr, ':' );
		if ( val.bv_val != NULL ) {
			val.bv_val[0] = '\0';
			val.bv_val++;
			val.bv_len = strlen( val.bv_val );
			valp = &val;
		}

		accessstr = strchr( attr, '/' );
		if ( accessstr != NULL ) {
			int	invalid = 0;

			accessstr[0] = '\0';
			accessstr++;
			access = str2access( accessstr );
			switch ( access ) {
			case ACL_INVALID_ACCESS:
				fprintf( stderr, "unknown access \"%s\" for attribute \"%s\"\n",
						accessstr, attr );
				invalid = 1;
				break;

			case ACL_NONE:
				fprintf( stderr, "\"none\" not allowed for attribute \"%s\"\n",
						attr );
				invalid = 1;
				break;

			default:
				break;
			}

			if ( invalid ) {
				if ( continuemode ) {
					continue;
				}
				break;
			}
		}

		rc = slap_str2ad( attr, &desc, &text );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "slap_str2ad(%s) failed %d (%s)\n",
					attr, rc, ldap_err2string( rc ) );
			if ( continuemode ) {
				continue;
			}
			break;
		}

		rc = access_allowed_mask( op, ep, desc, valp, access,
				NULL, &mask );

		if ( accessstr ) {
			fprintf( stderr, "%s access to %s%s%s: %s\n",
					accessstr,
					desc->ad_cname.bv_val,
					val.bv_val ? "=" : "",
					val.bv_val ? val.bv_val : "",
					rc ? "ALLOWED" : "DENIED" );

		} else {
			fprintf( stderr, "%s%s%s: %s\n",
					desc->ad_cname.bv_val,
					val.bv_val ? "=" : "",
					val.bv_val ? val.bv_val : "",
					accessmask2str( mask, accessmaskbuf, 1 ) );
		}
		rc = 0;
		attr = NULL;
	}

destroy:;
	if ( !BER_BVISNULL( &e.e_name ) ) {
		ber_memfree( e.e_name.bv_val );
	}
	if ( !BER_BVISNULL( &e.e_nname ) ) {
		ber_memfree( e.e_nname.bv_val );
	}
	if ( !dryrun && be ) {
		if ( ep && ep != &e ) {
			be_entry_release_r( op, ep );
		}
		if ( doclose ) {
			be->be_entry_close( be );
		}

		LDAP_STAILQ_FOREACH( bd, &backendDB, be_next ) {
			if ( bd != be ) {
				backend_shutdown( bd );
			}
		}
	}

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;

	return rc;
}

