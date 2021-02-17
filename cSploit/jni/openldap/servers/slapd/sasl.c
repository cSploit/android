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
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <ac/stdlib.h>
#include <ac/string.h>

#include <lber.h>
#include <ldap_log.h>

#include "slap.h"

#ifdef ENABLE_REWRITE
#include <rewrite.h>
#endif

#ifdef HAVE_CYRUS_SASL
# ifdef HAVE_SASL_SASL_H
#  include <sasl/sasl.h>
#  include <sasl/saslplug.h>
# else
#  include <sasl.h>
#  include <saslplug.h>
# endif

# define	SASL_CONST const

#define SASL_VERSION_FULL	((SASL_VERSION_MAJOR << 16) |\
	(SASL_VERSION_MINOR << 8) | SASL_VERSION_STEP)

static sasl_security_properties_t sasl_secprops;
#elif defined( SLAP_BUILTIN_SASL )
/*
 * built-in SASL implementation
 *	only supports EXTERNAL
 */
typedef struct sasl_ctx {
	slap_ssf_t sc_external_ssf;
	struct berval sc_external_id;
} SASL_CTX;

#endif

#include <lutil.h>

static struct berval ext_bv = BER_BVC( "EXTERNAL" );

char *slap_sasl_auxprops;

#ifdef HAVE_CYRUS_SASL

/* Just use our internal auxprop by default */
static int
slap_sasl_getopt(
	void *context,
	const char *plugin_name,
	const char *option,
	const char **result,
	unsigned *len)
{
	if ( strcmp( option, "auxprop_plugin" )) {
		return SASL_FAIL;
	}
	if ( slap_sasl_auxprops )
		*result = slap_sasl_auxprops;
	else
		*result = "slapd";
	return SASL_OK;
}

int
slap_sasl_log(
	void *context,
	int priority,
	const char *message) 
{
	Connection *conn = context;
	int level;
	const char * label;

	if ( message == NULL ) {
		return SASL_BADPARAM;
	}

	switch (priority) {
	case SASL_LOG_NONE:
		level = LDAP_DEBUG_NONE;
		label = "None";
		break;
	case SASL_LOG_ERR:
		level = LDAP_DEBUG_ANY;
		label = "Error";
		break;
	case SASL_LOG_FAIL:
		level = LDAP_DEBUG_ANY;
		label = "Failure";
		break;
	case SASL_LOG_WARN:
		level = LDAP_DEBUG_TRACE;
		label = "Warning";
		break;
	case SASL_LOG_NOTE:
		level = LDAP_DEBUG_TRACE;
		label = "Notice";
		break;
	case SASL_LOG_DEBUG:
		level = LDAP_DEBUG_TRACE;
		label = "Debug";
		break;
	case SASL_LOG_TRACE:
		level = LDAP_DEBUG_TRACE;
		label = "Trace";
		break;
	case SASL_LOG_PASS:
		level = LDAP_DEBUG_TRACE;
		label = "Password Trace";
		break;
	default:
		return SASL_BADPARAM;
	}

	Debug( level, "SASL [conn=%ld] %s: %s\n",
		conn ? (long) conn->c_connid: -1L,
		label, message );


	return SASL_OK;
}

static const char *slap_propnames[] = {
	"*slapConn", "*slapAuthcDNlen", "*slapAuthcDN",
	"*slapAuthzDNlen", "*slapAuthzDN", NULL };

#ifdef SLAP_AUXPROP_DONTUSECOPY
int slap_dontUseCopy_ignore;
BerVarray slap_dontUseCopy_propnames;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

static Filter generic_filter = { LDAP_FILTER_PRESENT, { 0 }, NULL };
static struct berval generic_filterstr = BER_BVC("(objectclass=*)");

#define	SLAP_SASL_PROP_CONN	0
#define	SLAP_SASL_PROP_AUTHCLEN	1
#define	SLAP_SASL_PROP_AUTHC	2
#define	SLAP_SASL_PROP_AUTHZLEN	3
#define	SLAP_SASL_PROP_AUTHZ	4
#define	SLAP_SASL_PROP_COUNT	5	/* Number of properties we used */

typedef struct lookup_info {
	int flags;
	const struct propval *list;
	sasl_server_params_t *sparams;
} lookup_info;

static slap_response sasl_ap_lookup;

static struct berval sc_cleartext = BER_BVC("{CLEARTEXT}");

static int
sasl_ap_lookup( Operation *op, SlapReply *rs )
{
	BerVarray bv;
	AttributeDescription *ad;
	Attribute *a;
	const char *text;
	int rc, i;
	lookup_info *sl = (lookup_info *)op->o_callback->sc_private;

	/* return the actual error code,
	 * to allow caller to handle specific errors
	 */
	if (rs->sr_type != REP_SEARCH) return rs->sr_err;

	for( i = 0; sl->list[i].name; i++ ) {
		const char *name = sl->list[i].name;

		if ( name[0] == '*' ) {
			if ( sl->flags & SASL_AUXPROP_AUTHZID ) continue;
			/* Skip our private properties */
			if ( !strcmp( name, slap_propnames[0] )) {
				i += SLAP_SASL_PROP_COUNT - 1;
				continue;
			}
			name++;
		} else if ( !(sl->flags & SASL_AUXPROP_AUTHZID ) )
			continue;

		if ( sl->list[i].values ) {
			if ( !(sl->flags & SASL_AUXPROP_OVERRIDE) ) continue;
		}
		ad = NULL;
		rc = slap_str2ad( name, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"slap_ap_lookup: str2ad(%s): %s\n", name, text, 0 );
			continue;
		}

		/* If it's the rootdn and a rootpw was present, we already set
		 * it so don't override it here.
		 */
		if ( ad == slap_schema.si_ad_userPassword && sl->list[i].values && 
			be_isroot_dn( op->o_bd, &op->o_req_ndn ))
			continue;

		a = attr_find( rs->sr_entry->e_attrs, ad );
		if ( !a ) continue;
		if ( ! access_allowed( op, rs->sr_entry, ad, NULL, ACL_AUTH, NULL ) ) {
			continue;
		}
		if ( sl->list[i].values && ( sl->flags & SASL_AUXPROP_OVERRIDE ) ) {
			sl->sparams->utils->prop_erase( sl->sparams->propctx,
			sl->list[i].name );
		}
		for ( bv = a->a_vals; bv->bv_val; bv++ ) {
			/* ITS#3846 don't give hashed passwords to SASL */
			if ( ad == slap_schema.si_ad_userPassword &&
				bv->bv_val[0] == '{' /*}*/ )
			{
				if ( lutil_passwd_scheme( bv->bv_val ) ) {
					/* If it's not a recognized scheme, just assume it's
					 * a cleartext password that happened to include brackets.
					 *
					 * If it's a recognized scheme, skip this value, unless the
					 * scheme is {CLEARTEXT}. In that case, skip over the
					 * scheme name and use the remainder. If there is nothing
					 * past the scheme name, skip this value.
					 */
#ifdef SLAPD_CLEARTEXT
					if ( !strncasecmp( bv->bv_val, sc_cleartext.bv_val,
						sc_cleartext.bv_len )) {
						struct berval cbv;
						cbv.bv_len = bv->bv_len - sc_cleartext.bv_len;
						if ( cbv.bv_len > 0 ) {
							cbv.bv_val = bv->bv_val + sc_cleartext.bv_len;
							sl->sparams->utils->prop_set( sl->sparams->propctx,
								sl->list[i].name, cbv.bv_val, cbv.bv_len );
						}
					}
#endif
					continue;
				}
			}
			sl->sparams->utils->prop_set( sl->sparams->propctx,
				sl->list[i].name, bv->bv_val, bv->bv_len );
		}
	}
	return LDAP_SUCCESS;
}

#if SASL_VERSION_FULL >= 0x020118
static int
#else
static void
#endif
slap_auxprop_lookup(
	void *glob_context,
	sasl_server_params_t *sparams,
	unsigned flags,
	const char *user,
	unsigned ulen)
{
	OperationBuffer opbuf = {{ NULL }};
	Operation *op = (Operation *)&opbuf;
	int i, doit = 0;
	Connection *conn = NULL;
	lookup_info sl;
	int rc = LDAP_SUCCESS;
#ifdef SLAP_AUXPROP_DONTUSECOPY
	int dontUseCopy = 0;
	BackendDB *dontUseCopy_bd = NULL;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

	sl.list = sparams->utils->prop_get( sparams->propctx );
	sl.sparams = sparams;
	sl.flags = flags;

	/* Find our DN and conn first */
	for( i = 0; sl.list[i].name; i++ ) {
		if ( sl.list[i].name[0] == '*' ) {
			if ( !strcmp( sl.list[i].name, slap_propnames[SLAP_SASL_PROP_CONN] ) ) {
				if ( sl.list[i].values && sl.list[i].values[0] )
					AC_MEMCPY( &conn, sl.list[i].values[0], sizeof( conn ) );
				continue;
			}
			if ( flags & SASL_AUXPROP_AUTHZID ) {
				if ( !strcmp( sl.list[i].name, slap_propnames[SLAP_SASL_PROP_AUTHZLEN] )) {
					if ( sl.list[i].values && sl.list[i].values[0] )
						AC_MEMCPY( &op->o_req_ndn.bv_len, sl.list[i].values[0],
							sizeof( op->o_req_ndn.bv_len ) );
				} else if ( !strcmp( sl.list[i].name, slap_propnames[SLAP_SASL_PROP_AUTHZ] )) {
					if ( sl.list[i].values )
						op->o_req_ndn.bv_val = (char *)sl.list[i].values[0];
					break;
				}
			}

			if ( !strcmp( sl.list[i].name, slap_propnames[SLAP_SASL_PROP_AUTHCLEN] )) {
				if ( sl.list[i].values && sl.list[i].values[0] )
					AC_MEMCPY( &op->o_req_ndn.bv_len, sl.list[i].values[0],
						sizeof( op->o_req_ndn.bv_len ) );
			} else if ( !strcmp( sl.list[i].name, slap_propnames[SLAP_SASL_PROP_AUTHC] ) ) {
				if ( sl.list[i].values ) {
					op->o_req_ndn.bv_val = (char *)sl.list[i].values[0];
					if ( !(flags & SASL_AUXPROP_AUTHZID) )
						break;
				}
			}
#ifdef SLAP_AUXPROP_DONTUSECOPY
			if ( slap_dontUseCopy_propnames != NULL ) {
				int j;
				struct berval bv;
				ber_str2bv( &sl.list[i].name[1], 0, 1, &bv );
				for ( j = 0; !BER_BVISNULL( &slap_dontUseCopy_propnames[ j ]); j++ ) {
					if ( bvmatch( &bv, &slap_dontUseCopy_propnames[ j ] ) ) {
						dontUseCopy = 1;
						break;
					}
				}
			}
#endif /* SLAP_AUXPROP_DONTUSECOPY */
		}
	}

	/* Now see what else needs to be fetched */
	for( i = 0; sl.list[i].name; i++ ) {
		const char *name = sl.list[i].name;

		if ( name[0] == '*' ) {
			if ( flags & SASL_AUXPROP_AUTHZID ) continue;
			/* Skip our private properties */
			if ( !strcmp( name, slap_propnames[0] )) {
				i += SLAP_SASL_PROP_COUNT - 1;
				continue;
			}
			name++;
		} else if ( !(flags & SASL_AUXPROP_AUTHZID ) )
			continue;

		if ( sl.list[i].values ) {
			if ( !(flags & SASL_AUXPROP_OVERRIDE) ) continue;
		}
		doit = 1;
		break;
	}

	if (doit) {
		slap_callback cb = { NULL, sasl_ap_lookup, NULL, NULL };

		cb.sc_private = &sl;

		op->o_bd = select_backend( &op->o_req_ndn, 1 );

		if ( op->o_bd ) {
			/* For rootdn, see if we can use the rootpw */
			if ( be_isroot_dn( op->o_bd, &op->o_req_ndn ) &&
				!BER_BVISEMPTY( &op->o_bd->be_rootpw )) {
				struct berval cbv = BER_BVNULL;

				/* If there's a recognized scheme, see if it's CLEARTEXT */
				if ( lutil_passwd_scheme( op->o_bd->be_rootpw.bv_val )) {
					if ( !strncasecmp( op->o_bd->be_rootpw.bv_val,
						sc_cleartext.bv_val, sc_cleartext.bv_len )) {

						/* If it's CLEARTEXT, skip past scheme spec */
						cbv.bv_len = op->o_bd->be_rootpw.bv_len -
							sc_cleartext.bv_len;
						if ( cbv.bv_len ) {
							cbv.bv_val = op->o_bd->be_rootpw.bv_val +
								sc_cleartext.bv_len;
						}
					}
				/* No scheme, use the whole value */
				} else {
					cbv = op->o_bd->be_rootpw;
				}
				if ( !BER_BVISEMPTY( &cbv )) {
					for( i = 0; sl.list[i].name; i++ ) {
						const char *name = sl.list[i].name;

						if ( name[0] == '*' ) {
							if ( flags & SASL_AUXPROP_AUTHZID ) continue;
								name++;
						} else if ( !(flags & SASL_AUXPROP_AUTHZID ) )
							continue;

						if ( !strcasecmp(name,"userPassword") ) {
							sl.sparams->utils->prop_set( sl.sparams->propctx,
								sl.list[i].name, cbv.bv_val, cbv.bv_len );
							break;
						}
					}
				}
			}

#ifdef SLAP_AUXPROP_DONTUSECOPY
			if ( SLAP_SHADOW( op->o_bd ) && dontUseCopy ) {
				dontUseCopy_bd = op->o_bd;
				op->o_bd = frontendDB;
			}

retry_dontUseCopy:;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

			if ( op->o_bd->be_search ) {
				SlapReply rs = {REP_RESULT};
#ifdef SLAP_AUXPROP_DONTUSECOPY
				LDAPControl **save_ctrls = NULL, c;
				int save_dontUseCopy;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

				op->o_hdr = conn->c_sasl_bindop->o_hdr;
				op->o_controls = opbuf.ob_controls;
				op->o_tag = LDAP_REQ_SEARCH;
				op->o_dn = conn->c_ndn;
				op->o_ndn = conn->c_ndn;
				op->o_callback = &cb;
				slap_op_time( &op->o_time, &op->o_tincr );
				op->o_do_not_cache = 1;
				op->o_is_auth_check = 1;
				op->o_req_dn = op->o_req_ndn;
				op->ors_scope = LDAP_SCOPE_BASE;
				op->ors_deref = LDAP_DEREF_NEVER;
				op->ors_tlimit = SLAP_NO_LIMIT;
				op->ors_slimit = 1;
				op->ors_filter = &generic_filter;
				op->ors_filterstr = generic_filterstr;
				op->o_authz = conn->c_authz;
				/* FIXME: we want all attributes, right? */
				op->ors_attrs = NULL;

#ifdef SLAP_AUXPROP_DONTUSECOPY
				if ( dontUseCopy ) {
					save_dontUseCopy = op->o_dontUseCopy;
					if ( !op->o_dontUseCopy ) {
						int cnt = 0;
						save_ctrls = op->o_ctrls;
						if ( op->o_ctrls ) {
							for ( ; op->o_ctrls[ cnt ]; cnt++ )
								;
						}
						op->o_ctrls = op->o_tmpcalloc( sizeof(LDAPControl *), cnt + 2, op->o_tmpmemctx );
						if ( cnt ) {
							for ( cnt = 0; save_ctrls[ cnt ]; cnt++ ) {
								op->o_ctrls[ cnt ] = save_ctrls[ cnt ];
							}
						}
						c.ldctl_oid = LDAP_CONTROL_DONTUSECOPY;
						c.ldctl_iscritical = 1;
						BER_BVZERO( &c.ldctl_value );
						op->o_ctrls[ cnt ] = &c;
					}
					op->o_dontUseCopy = SLAP_CONTROL_CRITICAL;
				}
#endif /* SLAP_AUXPROP_DONTUSECOPY */

				rc = op->o_bd->be_search( op, &rs );

#ifdef SLAP_AUXPROP_DONTUSECOPY
				if ( dontUseCopy ) {
					if ( save_ctrls != op->o_ctrls ) {
						op->o_tmpfree( op->o_ctrls, op->o_tmpmemctx );
						op->o_ctrls = save_ctrls;
						op->o_dontUseCopy = save_dontUseCopy;
					}

					if ( rs.sr_err == LDAP_UNAVAILABLE && slap_dontUseCopy_ignore )
					{
						op->o_bd = dontUseCopy_bd;
						dontUseCopy = 0;
						goto retry_dontUseCopy;
					}
				}
#endif /* SLAP_AUXPROP_DONTUSECOPY */
			}
		}
	}
#if SASL_VERSION_FULL >= 0x020118
	return rc != LDAP_SUCCESS ? SASL_FAIL : SASL_OK;
#endif
}

#if SASL_VERSION_FULL >= 0x020110
static int
slap_auxprop_store(
	void *glob_context,
	sasl_server_params_t *sparams,
	struct propctx *prctx,
	const char *user,
	unsigned ulen)
{
	Operation op = {0};
	Opheader oph;
	int rc, i;
	unsigned j;
	Connection *conn = NULL;
	const struct propval *pr;
	Modifications *modlist = NULL, **modtail = &modlist, *mod;
	slap_callback cb = { NULL, slap_null_cb, NULL, NULL };
	char textbuf[SLAP_TEXT_BUFLEN];
	const char *text;
	size_t textlen = sizeof(textbuf);
#ifdef SLAP_AUXPROP_DONTUSECOPY
	int dontUseCopy = 0;
	BackendDB *dontUseCopy_bd = NULL;
#endif /* SLAP_AUXPROP_DONTUSECOPY */

	/* just checking if we are enabled */
	if (!prctx) return SASL_OK;

	if (!sparams || !user) return SASL_BADPARAM;

	pr = sparams->utils->prop_get( sparams->propctx );

	/* Find our DN and conn first */
	for( i = 0; pr[i].name; i++ ) {
		if ( pr[i].name[0] == '*' ) {
			if ( !strcmp( pr[i].name, slap_propnames[SLAP_SASL_PROP_CONN] ) ) {
				if ( pr[i].values && pr[i].values[0] )
					AC_MEMCPY( &conn, pr[i].values[0], sizeof( conn ) );
				continue;
			}
			if ( !strcmp( pr[i].name, slap_propnames[SLAP_SASL_PROP_AUTHCLEN] )) {
				if ( pr[i].values && pr[i].values[0] )
					AC_MEMCPY( &op.o_req_ndn.bv_len, pr[i].values[0],
						sizeof( op.o_req_ndn.bv_len ) );
			} else if ( !strcmp( pr[i].name, slap_propnames[SLAP_SASL_PROP_AUTHC] ) ) {
				if ( pr[i].values )
					op.o_req_ndn.bv_val = (char *)pr[i].values[0];
			}
#ifdef SLAP_AUXPROP_DONTUSECOPY
			if ( slap_dontUseCopy_propnames != NULL ) {
				struct berval bv;
				ber_str2bv( &pr[i].name[1], 0, 1, &bv );
				for ( j = 0; !BER_BVISNULL( &slap_dontUseCopy_propnames[ j ] ); j++ ) {
					if ( bvmatch( &bv, &slap_dontUseCopy_propnames[ j ] ) ) {
						dontUseCopy = 1;
						break;
					}
				}
			}
#endif /* SLAP_AUXPROP_DONTUSECOPY */
		}
	}
	if (!conn || !op.o_req_ndn.bv_val) return SASL_BADPARAM;

	op.o_bd = select_backend( &op.o_req_ndn, 1 );

	if ( !op.o_bd || !op.o_bd->be_modify ) return SASL_FAIL;

#ifdef SLAP_AUXPROP_DONTUSECOPY
	if ( SLAP_SHADOW( op.o_bd ) && dontUseCopy ) {
		dontUseCopy_bd = op.o_bd;
		op.o_bd = frontendDB;
		op.o_dontUseCopy = SLAP_CONTROL_CRITICAL;
	}
#endif /* SLAP_AUXPROP_DONTUSECOPY */

	pr = sparams->utils->prop_get( prctx );
	if (!pr) return SASL_BADPARAM;

	for (i=0; pr[i].name; i++);
	if (!i) return SASL_BADPARAM;

	for (i=0; pr[i].name; i++) {
		mod = (Modifications *)ch_malloc( sizeof(Modifications) );
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_flags = 0;
		ber_str2bv( pr[i].name, 0, 0, &mod->sml_type );
		mod->sml_numvals = pr[i].nvalues;
		mod->sml_values = (struct berval *)ch_malloc( (pr[i].nvalues + 1) *
			sizeof(struct berval));
		for (j=0; j<pr[i].nvalues; j++) {
			ber_str2bv( pr[i].values[j], 0, 1, &mod->sml_values[j]);
		}
		BER_BVZERO( &mod->sml_values[j] );
		mod->sml_nvalues = NULL;
		mod->sml_desc = NULL;
		*modtail = mod;
		modtail = &mod->sml_next;
	}
	*modtail = NULL;

	rc = slap_mods_check( &op, modlist, &text, textbuf, textlen, NULL );

	if ( rc == LDAP_SUCCESS ) {
		rc = slap_mods_no_user_mod_check( &op, modlist,
			&text, textbuf, textlen );

		if ( rc == LDAP_SUCCESS ) {
			if ( conn->c_sasl_bindop ) {
				op.o_hdr = conn->c_sasl_bindop->o_hdr;
			} else {
				op.o_hdr = &oph;
				memset( &oph, 0, sizeof(oph) );
				operation_fake_init( conn, &op, ldap_pvt_thread_pool_context(), 0 );
			}
			op.o_tag = LDAP_REQ_MODIFY;
			op.o_ndn = op.o_req_ndn;
			op.o_callback = &cb;
			slap_op_time( &op.o_time, &op.o_tincr );
			op.o_do_not_cache = 1;
			op.o_is_auth_check = 1;
			op.o_req_dn = op.o_req_ndn;
			op.orm_modlist = modlist;

			for (;;) {
				SlapReply rs = {REP_RESULT};
				rc = op.o_bd->be_modify( &op, &rs );

#ifdef SLAP_AUXPROP_DONTUSECOPY
				if ( dontUseCopy &&
					rs.sr_err == LDAP_UNAVAILABLE &&
					slap_dontUseCopy_ignore )
				{
					op.o_bd = dontUseCopy_bd;
					op.o_dontUseCopy = SLAP_CONTROL_NONE;
					dontUseCopy = 0;
					continue;
				}
#endif /* SLAP_AUXPROP_DONTUSECOPY */
				break;
			}
		}
	}
	slap_mods_free( modlist, 1 );
	return rc != LDAP_SUCCESS ? SASL_FAIL : SASL_OK;
}
#endif /* SASL_VERSION_FULL >= 2.1.16 */

static sasl_auxprop_plug_t slap_auxprop_plugin = {
	0,	/* Features */
	0,	/* spare */
	NULL,	/* glob_context */
	NULL,	/* auxprop_free */
	slap_auxprop_lookup,
	"slapd",	/* name */
#if SASL_VERSION_FULL >= 0x020110
	slap_auxprop_store	/* the declaration of this member changed
				 * in cyrus SASL from 2.1.15 to 2.1.16 */
#else
	NULL
#endif
};

static int
slap_auxprop_init(
	const sasl_utils_t *utils,
	int max_version,
	int *out_version,
	sasl_auxprop_plug_t **plug,
	const char *plugname)
{
	if ( !out_version || !plug ) return SASL_BADPARAM;

	if ( max_version < SASL_AUXPROP_PLUG_VERSION ) return SASL_BADVERS;

	*out_version = SASL_AUXPROP_PLUG_VERSION;
	*plug = &slap_auxprop_plugin;
	return SASL_OK;
}

/* Convert a SASL authcid or authzid into a DN. Store the DN in an
 * auxiliary property, so that we can refer to it in sasl_authorize
 * without interfering with anything else. Also, the SASL username
 * buffer is constrained to 256 characters, and our DNs could be
 * much longer (SLAP_LDAPDN_MAXLEN, currently set to 8192)
 */
static int
slap_sasl_canonicalize(
	sasl_conn_t *sconn,
	void *context,
	const char *in,
	unsigned inlen,
	unsigned flags,
	const char *user_realm,
	char *out,
	unsigned out_max,
	unsigned *out_len)
{
	Connection *conn = (Connection *)context;
	struct propctx *props = sasl_auxprop_getctx( sconn );
	struct propval auxvals[ SLAP_SASL_PROP_COUNT ] = { { 0 } };
	struct berval dn;
	int rc, which;
	const char *names[2];
	struct berval	bvin;

	*out_len = 0;

	Debug( LDAP_DEBUG_ARGS, "SASL Canonicalize [conn=%ld]: %s=\"%s\"\n",
		conn ? (long) conn->c_connid : -1L,
		(flags & SASL_CU_AUTHID) ? "authcid" : "authzid",
		in ? in : "<empty>");

	/* If name is too big, just truncate. We don't care, we're
	 * using DNs, not the usernames.
	 */
	if ( inlen > out_max )
		inlen = out_max-1;

	/* This is a Simple Bind using SPASSWD. That means the in-directory
	 * userPassword of the Binding user already points at SASL, so it
	 * cannot be used to actually satisfy a password comparison. Just
	 * ignore it, some other mech will process it.
	 */
	if ( !conn->c_sasl_bindop ||
		conn->c_sasl_bindop->orb_method != LDAP_AUTH_SASL ) goto done;

	/* See if we need to add request, can only do it once */
	prop_getnames( props, slap_propnames, auxvals );
	if ( !auxvals[0].name )
		prop_request( props, slap_propnames );

	if ( flags & SASL_CU_AUTHID )
		which = SLAP_SASL_PROP_AUTHCLEN;
	else
		which = SLAP_SASL_PROP_AUTHZLEN;

	/* Need to store the Connection for auxprop_lookup */
	if ( !auxvals[SLAP_SASL_PROP_CONN].values ) {
		names[0] = slap_propnames[SLAP_SASL_PROP_CONN];
		names[1] = NULL;
		prop_set( props, names[0], (char *)&conn, sizeof( conn ) );
	}
		
	/* Already been here? */
	if ( auxvals[which].values )
		goto done;

	/* Normally we require an authzID to have a u: or dn: prefix.
	 * However, SASL frequently gives us an authzID that is just
	 * an exact copy of the authcID, without a prefix. We need to
	 * detect and allow this condition. If SASL calls canonicalize
	 * with SASL_CU_AUTHID|SASL_CU_AUTHZID this is a no-brainer.
	 * But if it's broken into two calls, we need to remember the
	 * authcID so that we can compare the authzID later. We store
	 * the authcID temporarily in conn->c_sasl_dn. We necessarily
	 * finish Canonicalizing before Authorizing, so there is no
	 * conflict with slap_sasl_authorize's use of this temp var.
	 *
	 * The SASL EXTERNAL mech is backwards from all the other mechs,
	 * it does authzID before the authcID. If we see that authzID
	 * has already been done, don't do anything special with authcID.
	 */
	if ( flags == SASL_CU_AUTHID && !auxvals[SLAP_SASL_PROP_AUTHZ].values ) {
		conn->c_sasl_dn.bv_val = (char *) in;
		conn->c_sasl_dn.bv_len = 0;
	} else if ( flags == SASL_CU_AUTHZID && conn->c_sasl_dn.bv_val ) {
		rc = strcmp( in, conn->c_sasl_dn.bv_val );
		conn->c_sasl_dn.bv_val = NULL;
		/* They were equal, no work needed */
		if ( !rc ) goto done;
	}

	bvin.bv_val = (char *)in;
	bvin.bv_len = inlen;
	rc = slap_sasl_getdn( conn, NULL, &bvin, (char *)user_realm, &dn,
		(flags & SASL_CU_AUTHID) ? SLAP_GETDN_AUTHCID : SLAP_GETDN_AUTHZID );
	if ( rc != LDAP_SUCCESS ) {
		sasl_seterror( sconn, 0, ldap_err2string( rc ) );
		return SASL_NOAUTHZ;
	}

	names[0] = slap_propnames[which];
	names[1] = NULL;
	prop_set( props, names[0], (char *)&dn.bv_len, sizeof( dn.bv_len ) );

	which++;
	names[0] = slap_propnames[which];
	prop_set( props, names[0], dn.bv_val, dn.bv_len );

	Debug( LDAP_DEBUG_ARGS, "SASL Canonicalize [conn=%ld]: %s=\"%s\"\n",
		conn ? (long) conn->c_connid : -1L, names[0]+1,
		dn.bv_val ? dn.bv_val : "<EMPTY>" );

	/* Not needed any more, SASL has copied it */
	if ( conn && conn->c_sasl_bindop )
		conn->c_sasl_bindop->o_tmpfree( dn.bv_val, conn->c_sasl_bindop->o_tmpmemctx );

done:
	AC_MEMCPY( out, in, inlen );
	out[inlen] = '\0';

	*out_len = inlen;

	return SASL_OK;
}

static int
slap_sasl_authorize(
	sasl_conn_t *sconn,
	void *context,
	char *requested_user,
	unsigned rlen,
	char *auth_identity,
	unsigned alen,
	const char *def_realm,
	unsigned urlen,
	struct propctx *props)
{
	Connection *conn = (Connection *)context;
	/* actually:
	 *	(SLAP_SASL_PROP_COUNT - 1)	because we skip "conn",
	 *	+ 1				for NULL termination?
	 */
	struct propval auxvals[ SLAP_SASL_PROP_COUNT ] = { { 0 } };
	struct berval authcDN, authzDN = BER_BVNULL;
	int rc;

	/* Simple Binds don't support proxy authorization, ignore it */
	if ( !conn->c_sasl_bindop ||
		conn->c_sasl_bindop->orb_method != LDAP_AUTH_SASL ) return SASL_OK;

	Debug( LDAP_DEBUG_ARGS, "SASL proxy authorize [conn=%ld]: "
		"authcid=\"%s\" authzid=\"%s\"\n",
		conn ? (long) conn->c_connid : -1L, auth_identity, requested_user );
	if ( conn->c_sasl_dn.bv_val ) {
		BER_BVZERO( &conn->c_sasl_dn );
	}

	/* Skip SLAP_SASL_PROP_CONN */
	prop_getnames( props, slap_propnames+1, auxvals );
	
	/* Should not happen */
	if ( !auxvals[0].values ) {
		sasl_seterror( sconn, 0, "invalid authcid" );
		return SASL_NOAUTHZ;
	}

	AC_MEMCPY( &authcDN.bv_len, auxvals[0].values[0], sizeof(authcDN.bv_len) );
	authcDN.bv_val = auxvals[1].values ? (char *)auxvals[1].values[0] : NULL;
	conn->c_sasl_dn = authcDN;

	/* Nothing to do if no authzID was given */
	if ( !auxvals[2].name || !auxvals[2].values ) {
		goto ok;
	}
	
	AC_MEMCPY( &authzDN.bv_len, auxvals[2].values[0], sizeof(authzDN.bv_len) );
	authzDN.bv_val = auxvals[3].values ? (char *)auxvals[3].values[0] : NULL;

	rc = slap_sasl_authorized( conn->c_sasl_bindop, &authcDN, &authzDN );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "SASL Proxy Authorize [conn=%ld]: "
			"proxy authorization disallowed (%d)\n",
			conn ? (long) conn->c_connid : -1L, rc, 0 );

		sasl_seterror( sconn, 0, "not authorized" );
		return SASL_NOAUTHZ;
	}

	/* FIXME: we need yet another dup because slap_sasl_getdn()
	 * is using the bind operation slab */
	ber_dupbv( &conn->c_sasl_authz_dn, &authzDN );

ok:
	if (conn->c_sasl_bindop) {
		Statslog( LDAP_DEBUG_STATS,
			"%s BIND authcid=\"%s\" authzid=\"%s\"\n",
			conn->c_sasl_bindop->o_log_prefix, 
			auth_identity, requested_user, 0, 0 );
	}

	Debug( LDAP_DEBUG_TRACE, "SASL Authorize [conn=%ld]: "
		" proxy authorization allowed authzDN=\"%s\"\n",
		conn ? (long) conn->c_connid : -1L, 
		authzDN.bv_val ? authzDN.bv_val : "", 0 );
	return SASL_OK;
} 

static int
slap_sasl_err2ldap( int saslerr )
{
	int rc;

	/* map SASL errors to LDAP resultCode returned by:
	 *	sasl_server_new()
	 *		SASL_OK, SASL_NOMEM
	 *	sasl_server_step()
	 *		SASL_OK, SASL_CONTINUE, SASL_TRANS, SASL_BADPARAM, SASL_BADPROT,
	 *      ...
	 *	sasl_server_start()
	 *      + SASL_NOMECH
	 *	sasl_setprop()
	 *		SASL_OK, SASL_BADPARAM
	 */

	switch (saslerr) {
		case SASL_OK:
			rc = LDAP_SUCCESS;
			break;
		case SASL_CONTINUE:
			rc = LDAP_SASL_BIND_IN_PROGRESS;
			break;
		case SASL_FAIL:
		case SASL_NOMEM:
			rc = LDAP_OTHER;
			break;
		case SASL_NOMECH:
			rc = LDAP_AUTH_METHOD_NOT_SUPPORTED;
			break;
		case SASL_BADAUTH:
		case SASL_NOUSER:
		case SASL_TRANS:
		case SASL_EXPIRED:
			rc = LDAP_INVALID_CREDENTIALS;
			break;
		case SASL_NOAUTHZ:
			rc = LDAP_INSUFFICIENT_ACCESS;
			break;
		case SASL_TOOWEAK:
		case SASL_ENCRYPT:
			rc = LDAP_INAPPROPRIATE_AUTH;
			break;
		case SASL_UNAVAIL:
		case SASL_TRYAGAIN:
			rc = LDAP_UNAVAILABLE;
			break;
		case SASL_DISABLED:
			rc = LDAP_UNWILLING_TO_PERFORM;
			break;
		default:
			rc = LDAP_OTHER;
			break;
	}

	return rc;
}

#ifdef SLAPD_SPASSWD

static struct berval sasl_pwscheme = BER_BVC("{SASL}");

static int chk_sasl(
	const struct berval *sc,
	const struct berval * passwd,
	const struct berval * cred,
	const char **text )
{
	unsigned int i;
	int rtn;
	void *ctx, *sconn = NULL;

	for( i=0; i<cred->bv_len; i++) {
		if(cred->bv_val[i] == '\0') {
			return LUTIL_PASSWD_ERR;	/* NUL character in password */
		}
	}

	if( cred->bv_val[i] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* cred must behave like a string */
	}

	for( i=0; i<passwd->bv_len; i++) {
		if(passwd->bv_val[i] == '\0') {
			return LUTIL_PASSWD_ERR;	/* NUL character in password */
		}
	}

	if( passwd->bv_val[i] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* passwd must behave like a string */
	}

	rtn = LUTIL_PASSWD_ERR;

	ctx = ldap_pvt_thread_pool_context();
	ldap_pvt_thread_pool_getkey( ctx, (void *)slap_sasl_bind, &sconn, NULL );

	if( sconn != NULL ) {
		int sc;
		sc = sasl_checkpass( sconn,
			passwd->bv_val, passwd->bv_len,
			cred->bv_val, cred->bv_len );
		rtn = ( sc != SASL_OK ) ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
	}

	return rtn;
}
#endif /* SLAPD_SPASSWD */

#endif /* HAVE_CYRUS_SASL */

#ifdef ENABLE_REWRITE

typedef struct slapd_map_data {
	struct berval base;
	struct berval filter;
	AttributeName attrs[2];
	int scope;
} slapd_map_data;

static void *
slapd_rw_config( const char *fname, int lineno, int argc, char **argv )
{
	slapd_map_data *ret = NULL;
	LDAPURLDesc *lud = NULL;
	char *uri;
	AttributeDescription *ad = NULL;
	int rc, flen = 0;
	struct berval dn, ndn;

	if ( argc != 1 ) {
		Debug( LDAP_DEBUG_ANY,
			"[%s:%d] slapd map needs URI\n",
			fname, lineno, 0 );
        return NULL;
	}

	uri = argv[0];
	if ( strncasecmp( uri, "uri=", STRLENOF( "uri=" ) ) == 0 ) {
		uri += STRLENOF( "uri=" );
	}

	if ( ldap_url_parse( uri, &lud ) != LDAP_URL_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"[%s:%d] illegal URI '%s'\n",
			fname, lineno, uri );
        return NULL;
	}

	if ( strcasecmp( lud->lud_scheme, "ldap" )) {
		Debug( LDAP_DEBUG_ANY,
			"[%s:%d] illegal URI scheme '%s'\n",
			fname, lineno, lud->lud_scheme );
		goto done;
	}

	if (( lud->lud_host && lud->lud_host[0] ) || lud->lud_exts
		|| !lud->lud_dn ) {
		Debug( LDAP_DEBUG_ANY,
			"[%s:%d] illegal URI '%s'\n",
			fname, lineno, uri );
		goto done;
	}

	if ( lud->lud_attrs ) {
		if ( lud->lud_attrs[1] ) {
			Debug( LDAP_DEBUG_ANY,
				"[%s:%d] only one attribute allowed in URI\n",
				fname, lineno, 0 );
			goto done;
		}
		if ( strcasecmp( lud->lud_attrs[0], "dn" ) &&
			strcasecmp( lud->lud_attrs[0], "entryDN" )) {
			const char *text;
			rc = slap_str2ad( lud->lud_attrs[0], &ad, &text );
			if ( rc )
				goto done;
		}
	}
	ber_str2bv( lud->lud_dn, 0, 0, &dn );
	if ( dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL ))
		goto done;

	if ( lud->lud_filter ) {
		flen = strlen( lud->lud_filter ) + 1;
	}
	ret = ch_malloc( sizeof( slapd_map_data ) + flen );
	ret->base = ndn;
	if ( flen ) {
		ret->filter.bv_val = (char *)(ret+1);
		ret->filter.bv_len = flen - 1;
		strcpy( ret->filter.bv_val, lud->lud_filter );
	} else {
		BER_BVZERO( &ret->filter );
	}
	ret->scope = lud->lud_scope;
	if ( ad ) {
		ret->attrs[0].an_name = ad->ad_cname;
	} else {
		BER_BVZERO( &ret->attrs[0].an_name );
	}
	ret->attrs[0].an_desc = ad;
	BER_BVZERO( &ret->attrs[1].an_name );
done:
	ldap_free_urldesc( lud );
	return ret;
}

struct slapd_rw_info {
	slapd_map_data *si_data;
	struct berval si_val;
};

static int
slapd_rw_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_SEARCH ) {
		struct slapd_rw_info *si = op->o_callback->sc_private;

		if ( si->si_data->attrs[0].an_desc ) {
			Attribute *a;

			a = attr_find( rs->sr_entry->e_attrs,
				si->si_data->attrs[0].an_desc );
			if ( a ) {
				ber_dupbv( &si->si_val, a->a_vals );
			}
		} else {
			ber_dupbv( &si->si_val, &rs->sr_entry->e_name );
		}
	}
	return LDAP_SUCCESS;
}

static int
slapd_rw_apply( void *private, const char *filter, struct berval *val )
{
	slapd_map_data *sl = private;
	slap_callback cb = { NULL };
	Connection conn = {0};
	OperationBuffer opbuf;
	Operation *op;
	void *thrctx;
	SlapReply rs = {REP_RESULT};
	struct slapd_rw_info si;
	char *ptr;
	int rc;

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;

	op->o_tag = LDAP_REQ_SEARCH;
	op->o_req_dn = op->o_req_ndn = sl->base;
	op->o_bd = select_backend( &op->o_req_ndn, 1 );
	if ( !op->o_bd ) {
		return REWRITE_ERR;
	}
	si.si_data = sl;
	BER_BVZERO( &si.si_val );
	op->ors_scope = sl->scope;
	op->ors_deref = LDAP_DEREF_NEVER;
	op->ors_slimit = 1;
	op->ors_tlimit = SLAP_NO_LIMIT;
	if ( sl->attrs[0].an_desc ) {
		op->ors_attrs = sl->attrs;
	} else {
		op->ors_attrs = slap_anlist_no_attrs;
	}
	if ( filter ) {
		rc = strlen( filter );
	} else {
		rc = 0;
	}
	rc += sl->filter.bv_len;
	ptr = op->ors_filterstr.bv_val = op->o_tmpalloc( rc + 1, op->o_tmpmemctx );
	if ( sl->filter.bv_len ) {
		ptr = lutil_strcopy( ptr, sl->filter.bv_val );
	} else {
		*ptr = '\0';
	}
	if ( filter ) {
		strcpy( ptr, filter );
	}
	op->ors_filter = str2filter_x( op, op->ors_filterstr.bv_val );
	if ( !op->ors_filter ) {
		op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
		return REWRITE_ERR;
	}

	op->ors_attrsonly = 0;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	op->o_do_not_cache = 1;

	cb.sc_response = slapd_rw_cb;
	cb.sc_private = &si;
	op->o_callback = &cb;

	rc = op->o_bd->be_search( op, &rs );
	if ( rc == LDAP_SUCCESS && !BER_BVISNULL( &si.si_val )) {
		*val = si.si_val;
		rc = REWRITE_SUCCESS;
	} else {
		if ( !BER_BVISNULL( &si.si_val )) {
			ch_free( si.si_val.bv_val );
		}
		rc = REWRITE_ERR;
	}
	filter_free_x( op, op->ors_filter, 1 );
	op->o_tmpfree( op->ors_filterstr.bv_val, op->o_tmpmemctx );
	return rc;
}

static int
slapd_rw_destroy( void *private )
{
	slapd_map_data *md = private;

	assert( private != NULL );

	ch_free( md->base.bv_val );
	ch_free( md );

	return 0;
}

static const rewrite_mapper slapd_mapper = {
	"slapd",
	slapd_rw_config,
	slapd_rw_apply,
	slapd_rw_destroy
};
#endif

int slap_sasl_init( void )
{
#ifdef HAVE_CYRUS_SASL
	int rc;
	static sasl_callback_t server_callbacks[] = {
		{ SASL_CB_LOG, &slap_sasl_log, NULL },
		{ SASL_CB_GETOPT, &slap_sasl_getopt, NULL },
		{ SASL_CB_LIST_END, NULL, NULL }
	};
#endif

#ifdef ENABLE_REWRITE
	rewrite_mapper_register( &slapd_mapper );
#endif

#ifdef HAVE_CYRUS_SASL
#ifdef HAVE_SASL_VERSION
	/* stringify the version number, sasl.h doesn't do it for us */
#define	VSTR0(maj, min, pat)	#maj "." #min "." #pat
#define	VSTR(maj, min, pat)	VSTR0(maj, min, pat)
#define	SASL_VERSION_STRING	VSTR(SASL_VERSION_MAJOR, SASL_VERSION_MINOR, \
				SASL_VERSION_STEP)

	sasl_version( NULL, &rc );
	if ( ((rc >> 16) != ((SASL_VERSION_MAJOR << 8)|SASL_VERSION_MINOR)) ||
		(rc & 0xffff) < SASL_VERSION_STEP)
	{
		char version[sizeof("xxx.xxx.xxxxx")];
		sprintf( version, "%u.%d.%d", (unsigned)rc >> 24, (rc >> 16) & 0xff,
			rc & 0xffff );
		Debug( LDAP_DEBUG_ANY, "slap_sasl_init: SASL library version mismatch:"
			" expected %s, got %s\n",
			SASL_VERSION_STRING, version, 0 );
		return -1;
	}
#endif

	sasl_set_mutex(
		ldap_pvt_sasl_mutex_new,
		ldap_pvt_sasl_mutex_lock,
		ldap_pvt_sasl_mutex_unlock,
		ldap_pvt_sasl_mutex_dispose );

	generic_filter.f_desc = slap_schema.si_ad_objectClass;

	rc = sasl_auxprop_add_plugin( "slapd", slap_auxprop_init );
	if( rc != SASL_OK ) {
		Debug( LDAP_DEBUG_ANY, "slap_sasl_init: auxprop add plugin failed\n",
			0, 0, 0 );
		return -1;
	}

	/* should provide callbacks for logging */
	/* server name should be configurable */
	rc = sasl_server_init( server_callbacks, "slapd" );

	if( rc != SASL_OK ) {
		Debug( LDAP_DEBUG_ANY, "slap_sasl_init: server init failed\n",
			0, 0, 0 );

		return -1;
	}

#ifdef SLAPD_SPASSWD
	lutil_passwd_add( &sasl_pwscheme, chk_sasl, NULL );
#endif

	Debug( LDAP_DEBUG_TRACE, "slap_sasl_init: initialized!\n",
		0, 0, 0 );

	/* default security properties */
	memset( &sasl_secprops, '\0', sizeof(sasl_secprops) );
	sasl_secprops.max_ssf = INT_MAX;
	sasl_secprops.maxbufsize = 65536;
	sasl_secprops.security_flags = SASL_SEC_NOPLAINTEXT|SASL_SEC_NOANONYMOUS;
#endif

	return 0;
}

int slap_sasl_destroy( void )
{
#ifdef HAVE_CYRUS_SASL
	sasl_done();

#ifdef SLAP_AUXPROP_DONTUSECOPY
	if ( slap_dontUseCopy_propnames ) {
		ber_bvarray_free( slap_dontUseCopy_propnames );
		slap_dontUseCopy_propnames = NULL;
	}
#endif /* SLAP_AUXPROP_DONTUSECOPY */
#endif
	free( sasl_host );
	sasl_host = NULL;

	return 0;
}

static char *
slap_sasl_peer2ipport( struct berval *peer )
{
	int		isv6 = 0;
	char		*ipport, *p,
			*addr = &peer->bv_val[ STRLENOF( "IP=" ) ];
	ber_len_t	plen = peer->bv_len - STRLENOF( "IP=" );

	/* IPv6? */
	if ( addr[0] == '[' ) {
		isv6 = 1;
		plen--;
	}
	ipport = ch_strdup( &addr[isv6] );

	/* Convert IPv6/IPv4 addresses to address;port syntax. */
	p = strrchr( ipport, ':' );
	if ( p != NULL ) {
		*p = ';';
		if ( isv6 ) {
			assert( p[-1] == ']' );
			AC_MEMCPY( &p[-1], p, plen - ( p - ipport ) + 1 );
		}

	} else if ( isv6 ) {
		/* trim ']' */
		plen--;
		assert( addr[plen] == ']' );
		addr[plen] = '\0';
	}

	return ipport;
}

int slap_sasl_open( Connection *conn, int reopen )
{
	int sc = LDAP_SUCCESS;
#ifdef HAVE_CYRUS_SASL
	int cb;

	sasl_conn_t *ctx = NULL;
	sasl_callback_t *session_callbacks;
	char *ipremoteport = NULL, *iplocalport = NULL;

	assert( conn->c_sasl_authctx == NULL );

	if ( !reopen ) {
		assert( conn->c_sasl_extra == NULL );

		session_callbacks =
			SLAP_CALLOC( 5, sizeof(sasl_callback_t));
		if( session_callbacks == NULL ) {
			Debug( LDAP_DEBUG_ANY, 
				"slap_sasl_open: SLAP_MALLOC failed", 0, 0, 0 );
			return -1;
		}
		conn->c_sasl_extra = session_callbacks;

		session_callbacks[cb=0].id = SASL_CB_LOG;
		session_callbacks[cb].proc = &slap_sasl_log;
		session_callbacks[cb++].context = conn;

		session_callbacks[cb].id = SASL_CB_PROXY_POLICY;
		session_callbacks[cb].proc = &slap_sasl_authorize;
		session_callbacks[cb++].context = conn;

		session_callbacks[cb].id = SASL_CB_CANON_USER;
		session_callbacks[cb].proc = &slap_sasl_canonicalize;
		session_callbacks[cb++].context = conn;

		session_callbacks[cb].id = SASL_CB_LIST_END;
		session_callbacks[cb].proc = NULL;
		session_callbacks[cb++].context = NULL;
	} else {
		session_callbacks = conn->c_sasl_extra;
	}

	conn->c_sasl_layers = 0;

	/* create new SASL context */
	if ( conn->c_sock_name.bv_len != 0 &&
		strncmp( conn->c_sock_name.bv_val, "IP=", STRLENOF( "IP=" ) ) == 0 )
	{
		iplocalport = slap_sasl_peer2ipport( &conn->c_sock_name );
	}

	if ( conn->c_peer_name.bv_len != 0 &&
		strncmp( conn->c_peer_name.bv_val, "IP=", STRLENOF( "IP=" ) ) == 0 )
	{
		ipremoteport = slap_sasl_peer2ipport( &conn->c_peer_name );
	}

	sc = sasl_server_new( "ldap", sasl_host, global_realm,
		iplocalport, ipremoteport, session_callbacks, SASL_SUCCESS_DATA, &ctx );
	if ( iplocalport != NULL ) {
		ch_free( iplocalport );
	}
	if ( ipremoteport != NULL ) {
		ch_free( ipremoteport );
	}

	if( sc != SASL_OK ) {
		Debug( LDAP_DEBUG_ANY, "sasl_server_new failed: %d\n",
			sc, 0, 0 );

		return -1;
	}

	conn->c_sasl_authctx = ctx;

	if( sc == SASL_OK ) {
		sc = sasl_setprop( ctx,
			SASL_SEC_PROPS, &sasl_secprops );

		if( sc != SASL_OK ) {
			Debug( LDAP_DEBUG_ANY, "sasl_setprop failed: %d\n",
				sc, 0, 0 );

			slap_sasl_close( conn );
			return -1;
		}
	}

	sc = slap_sasl_err2ldap( sc );

#elif defined(SLAP_BUILTIN_SASL)
	/* built-in SASL implementation */
	SASL_CTX *ctx = (SASL_CTX *) SLAP_MALLOC(sizeof(SASL_CTX));
	if( ctx == NULL ) return -1;

	ctx->sc_external_ssf = 0;
	BER_BVZERO( &ctx->sc_external_id );

	conn->c_sasl_authctx = ctx;
#endif

	return sc;
}

int slap_sasl_external(
	Connection *conn,
	slap_ssf_t ssf,
	struct berval *auth_id )
{
#ifdef HAVE_CYRUS_SASL
	int sc;
	sasl_conn_t *ctx = conn->c_sasl_authctx;
	sasl_ssf_t sasl_ssf = ssf;

	if ( ctx == NULL ) {
		return LDAP_UNAVAILABLE;
	}

	sc = sasl_setprop( ctx, SASL_SSF_EXTERNAL, &sasl_ssf );

	if ( sc != SASL_OK ) {
		return LDAP_OTHER;
	}

	sc = sasl_setprop( ctx, SASL_AUTH_EXTERNAL,
		auth_id ? auth_id->bv_val : NULL );

	if ( sc != SASL_OK ) {
		return LDAP_OTHER;
	}
#elif defined(SLAP_BUILTIN_SASL)
	/* built-in SASL implementation */
	SASL_CTX *ctx = conn->c_sasl_authctx;
	if ( ctx == NULL ) return LDAP_UNAVAILABLE;

	ctx->sc_external_ssf = ssf;
	if( auth_id ) {
		ctx->sc_external_id = *auth_id;
		BER_BVZERO( auth_id );
	} else {
		BER_BVZERO( &ctx->sc_external_id );
	}
#endif

	return LDAP_SUCCESS;
}

int slap_sasl_cbinding( Connection *conn, struct berval *cbv )
{
#ifdef SASL_CHANNEL_BINDING
	sasl_channel_binding_t *cb = ch_malloc( sizeof(*cb) + cbv->bv_len );;
	cb->name = "ldap";
	cb->critical = 0;
	cb->data = (char *)(cb+1);
	cb->len = cbv->bv_len;
	memcpy( cb->data, cbv->bv_val, cbv->bv_len );
	sasl_setprop( conn->c_sasl_authctx, SASL_CHANNEL_BINDING, cb );
	conn->c_sasl_cbind = cb;
#endif
	return LDAP_SUCCESS;
}

int slap_sasl_reset( Connection *conn )
{
	return LDAP_SUCCESS;
}

char ** slap_sasl_mechs( Connection *conn )
{
	char **mechs = NULL;

#ifdef HAVE_CYRUS_SASL
	sasl_conn_t *ctx = conn->c_sasl_authctx;

	if( ctx == NULL ) ctx = conn->c_sasl_sockctx;

	if( ctx != NULL ) {
		int sc;
		SASL_CONST char *mechstr;

		sc = sasl_listmech( ctx,
			NULL, NULL, ",", NULL,
			&mechstr, NULL, NULL );

		if( sc != SASL_OK ) {
			Debug( LDAP_DEBUG_ANY, "slap_sasl_listmech failed: %d\n",
				sc, 0, 0 );

			return NULL;
		}

		mechs = ldap_str2charray( mechstr, "," );
	}
#elif defined(SLAP_BUILTIN_SASL)
	/* builtin SASL implementation */
	SASL_CTX *ctx = conn->c_sasl_authctx;
	if ( ctx != NULL && ctx->sc_external_id.bv_val ) {
		/* should check ssf */
		mechs = ldap_str2charray( "EXTERNAL", "," );
	}
#endif

	return mechs;
}

int slap_sasl_close( Connection *conn )
{
#ifdef HAVE_CYRUS_SASL
	sasl_conn_t *ctx = conn->c_sasl_authctx;

	if( ctx != NULL ) {
		sasl_dispose( &ctx );
	}
	if ( conn->c_sasl_sockctx &&
		conn->c_sasl_authctx != conn->c_sasl_sockctx )
	{
		ctx = conn->c_sasl_sockctx;
		sasl_dispose( &ctx );
	}

	conn->c_sasl_authctx = NULL;
	conn->c_sasl_sockctx = NULL;
	conn->c_sasl_done = 0;

	free( conn->c_sasl_extra );
	conn->c_sasl_extra = NULL;

	free( conn->c_sasl_cbind );
	conn->c_sasl_cbind = NULL;

#elif defined(SLAP_BUILTIN_SASL)
	SASL_CTX *ctx = conn->c_sasl_authctx;
	if( ctx ) {
		if( ctx->sc_external_id.bv_val ) {
			free( ctx->sc_external_id.bv_val );
			BER_BVZERO( &ctx->sc_external_id );
		}
		free( ctx );
		conn->c_sasl_authctx = NULL;
	}
#endif

	return LDAP_SUCCESS;
}

int slap_sasl_bind( Operation *op, SlapReply *rs )
{
#ifdef HAVE_CYRUS_SASL
	sasl_conn_t *ctx = op->o_conn->c_sasl_authctx;
	struct berval response;
	unsigned reslen = 0;
	int sc;

	Debug(LDAP_DEBUG_ARGS,
		"==> sasl_bind: dn=\"%s\" mech=%s datalen=%ld\n",
		op->o_req_dn.bv_len ? op->o_req_dn.bv_val : "",
		op->o_conn->c_sasl_bind_in_progress ? "<continuing>" : 
		op->o_conn->c_sasl_bind_mech.bv_val,
		op->orb_cred.bv_len );

	if( ctx == NULL ) {
		send_ldap_error( op, rs, LDAP_UNAVAILABLE,
			"SASL unavailable on this session" );
		return rs->sr_err;
	}

#define	START( ctx, mech, cred, clen, resp, rlen, err ) \
	sasl_server_start( ctx, mech, cred, clen, resp, rlen )
#define	STEP( ctx, cred, clen, resp, rlen, err ) \
	sasl_server_step( ctx, cred, clen, resp, rlen )

	if ( !op->o_conn->c_sasl_bind_in_progress ) {
		/* If we already authenticated once, must use a new context */
		if ( op->o_conn->c_sasl_done ) {
			sasl_ssf_t ssf = 0;
			const char *authid = NULL;
			sasl_getprop( ctx, SASL_SSF_EXTERNAL, (void *)&ssf );
			sasl_getprop( ctx, SASL_AUTH_EXTERNAL, (void *)&authid );
			if ( authid ) authid = ch_strdup( authid );
			if ( ctx != op->o_conn->c_sasl_sockctx ) {
				sasl_dispose( &ctx );
			}
			op->o_conn->c_sasl_authctx = NULL;
				
			slap_sasl_open( op->o_conn, 1 );
			ctx = op->o_conn->c_sasl_authctx;
			if ( authid ) {
				sasl_setprop( ctx, SASL_SSF_EXTERNAL, &ssf );
				sasl_setprop( ctx, SASL_AUTH_EXTERNAL, authid );
				ch_free( (char *)authid );
			}
		}
		sc = START( ctx,
			op->o_conn->c_sasl_bind_mech.bv_val,
			op->orb_cred.bv_val, op->orb_cred.bv_len,
			(SASL_CONST char **)&response.bv_val, &reslen, &rs->sr_text );

	} else {
		sc = STEP( ctx,
			op->orb_cred.bv_val, op->orb_cred.bv_len,
			(SASL_CONST char **)&response.bv_val, &reslen, &rs->sr_text );
	}

	response.bv_len = reslen;

	if ( sc == SASL_OK ) {
		sasl_ssf_t *ssf = NULL;

		ber_dupbv_x( &op->orb_edn, &op->o_conn->c_sasl_dn, op->o_tmpmemctx );
		BER_BVZERO( &op->o_conn->c_sasl_dn );
		op->o_conn->c_sasl_done = 1;

		rs->sr_err = LDAP_SUCCESS;

		(void) sasl_getprop( ctx, SASL_SSF, (void *)&ssf );
		op->orb_ssf = ssf ? *ssf : 0;

		ctx = NULL;
		if( op->orb_ssf ) {
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			op->o_conn->c_sasl_layers++;

			/* If there's an old layer, set sockctx to NULL to
			 * tell connection_read() to wait for us to finish.
			 * Otherwise there is a race condition: we have to
			 * send the Bind response using the old security
			 * context and then remove it before reading any
			 * new messages.
			 */
			if ( op->o_conn->c_sasl_sockctx ) {
				ctx = op->o_conn->c_sasl_sockctx;
				op->o_conn->c_sasl_sockctx = NULL;
			} else {
				op->o_conn->c_sasl_sockctx = op->o_conn->c_sasl_authctx;
			}
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
		}

		/* Must send response using old security layer */
		rs->sr_sasldata = (response.bv_len ? &response : NULL);
		send_ldap_sasl( op, rs );
		
		/* Now dispose of the old security layer.
		 */
		if ( ctx ) {
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			ldap_pvt_sasl_remove( op->o_conn->c_sb );
			op->o_conn->c_sasl_sockctx = op->o_conn->c_sasl_authctx;
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );
			sasl_dispose( &ctx );
		}
	} else if ( sc == SASL_CONTINUE ) {
		rs->sr_err = LDAP_SASL_BIND_IN_PROGRESS,
		rs->sr_text = sasl_errdetail( ctx );
		rs->sr_sasldata = &response;
		send_ldap_sasl( op, rs );

	} else {
		BER_BVZERO( &op->o_conn->c_sasl_dn );
		rs->sr_text = sasl_errdetail( ctx );
		rs->sr_err = slap_sasl_err2ldap( sc ),
		send_ldap_result( op, rs );
	}

	Debug(LDAP_DEBUG_TRACE, "<== slap_sasl_bind: rc=%d\n", rs->sr_err, 0, 0);

#elif defined(SLAP_BUILTIN_SASL)
	/* built-in SASL implementation */
	SASL_CTX *ctx = op->o_conn->c_sasl_authctx;

	if ( ctx == NULL ) {
		send_ldap_error( op, rs, LDAP_OTHER,
			"Internal SASL Error" );

	} else if ( bvmatch( &ext_bv, &op->o_conn->c_sasl_bind_mech ) ) {
		/* EXTERNAL */

		if( op->orb_cred.bv_len ) {
			rs->sr_text = "proxy authorization not supported";
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			send_ldap_result( op, rs );

		} else {
			op->orb_edn = ctx->sc_external_id;
			rs->sr_err = LDAP_SUCCESS;
			rs->sr_sasldata = NULL;
			send_ldap_sasl( op, rs );
		}

	} else {
		send_ldap_error( op, rs, LDAP_AUTH_METHOD_NOT_SUPPORTED,
			"requested SASL mechanism not supported" );
	}
#else
	send_ldap_error( op, rs, LDAP_AUTH_METHOD_NOT_SUPPORTED,
		"SASL not supported" );
#endif

	return rs->sr_err;
}

char* slap_sasl_secprops( const char *in )
{
#ifdef HAVE_CYRUS_SASL
	int rc = ldap_pvt_sasl_secprops( in, &sasl_secprops );

	return rc == LDAP_SUCCESS ? NULL : "Invalid security properties";
#else
	return "SASL not supported";
#endif
}

void slap_sasl_secprops_unparse( struct berval *bv )
{
#ifdef HAVE_CYRUS_SASL
	ldap_pvt_sasl_secprops_unparse( &sasl_secprops, bv );
#endif
}

#ifdef HAVE_CYRUS_SASL
int
slap_sasl_setpass( Operation *op, SlapReply *rs )
{
	struct berval id = BER_BVNULL;	/* needs to come from connection */
	struct berval new = BER_BVNULL;
	struct berval old = BER_BVNULL;

	assert( ber_bvcmp( &slap_EXOP_MODIFY_PASSWD, &op->ore_reqoid ) == 0 );

	rs->sr_err = sasl_getprop( op->o_conn->c_sasl_authctx, SASL_USERNAME,
		(SASL_CONST void **)(char *)&id.bv_val );

	if( rs->sr_err != SASL_OK ) {
		rs->sr_text = "unable to retrieve SASL username";
		rs->sr_err = LDAP_OTHER;
		goto done;
	}

	Debug( LDAP_DEBUG_ARGS, "==> slap_sasl_setpass: \"%s\"\n",
		id.bv_val ? id.bv_val : "", 0, 0 );

	rs->sr_err = slap_passwd_parse( op->ore_reqdata,
		NULL, &old, &new, &rs->sr_text );

	if( rs->sr_err != LDAP_SUCCESS ) {
		goto done;
	}

	if( new.bv_len == 0 ) {
		slap_passwd_generate(&new);

		if( new.bv_len == 0 ) {
			rs->sr_text = "password generation failed.";
			rs->sr_err = LDAP_OTHER;
			goto done;
		}
		
		rs->sr_rspdata = slap_passwd_return( &new );
	}

	rs->sr_err = sasl_setpass( op->o_conn->c_sasl_authctx, id.bv_val,
		new.bv_val, new.bv_len, old.bv_val, old.bv_len, 0 );
	if( rs->sr_err != SASL_OK ) {
		rs->sr_text = sasl_errdetail( op->o_conn->c_sasl_authctx );
	}
	switch(rs->sr_err) {
		case SASL_OK:
			rs->sr_err = LDAP_SUCCESS;
			break;

		case SASL_NOCHANGE:
		case SASL_NOMECH:
		case SASL_DISABLED:
		case SASL_PWLOCK:
		case SASL_FAIL:
		case SASL_BADPARAM:
		default:
			rs->sr_err = LDAP_OTHER;
	}

done:
	return rs->sr_err;
}
#endif /* HAVE_CYRUS_SASL */

/* Take any sort of identity string and return a DN with the "dn:" prefix. The
 * string returned in *dn is in its own allocated memory, and must be free'd 
 * by the calling process.  -Mark Adamson, Carnegie Mellon
 *
 * The "dn:" prefix is no longer used anywhere inside slapd. It is only used
 * on strings passed in directly from SASL.  -Howard Chu, Symas Corp.
 */

#define SET_NONE	0
#define	SET_DN		1
#define	SET_U		2

int slap_sasl_getdn( Connection *conn, Operation *op, struct berval *id,
	char *user_realm, struct berval *dn, int flags )
{
	int rc, is_dn = SET_NONE, do_norm = 1;
	struct berval dn2, *mech;

	assert( conn != NULL );
	assert( id != NULL );

	Debug( LDAP_DEBUG_ARGS, "slap_sasl_getdn: conn %lu id=%s [len=%lu]\n", 
		conn->c_connid,
		BER_BVISNULL( id ) ? "NULL" : ( BER_BVISEMPTY( id ) ? "<empty>" : id->bv_val ),
		BER_BVISNULL( id ) ? 0 : ( BER_BVISEMPTY( id ) ? 0 :
		                           (unsigned long) id->bv_len ) );

	if ( !op ) {
		op = conn->c_sasl_bindop;
	}
	assert( op != NULL );

	BER_BVZERO( dn );

	if ( !BER_BVISNULL( id ) ) {
		/* Blatantly anonymous ID */
		static struct berval bv_anonymous = BER_BVC( "anonymous" );

		if ( ber_bvstrcasecmp( id, &bv_anonymous ) == 0 ) {
			return( LDAP_SUCCESS );
		}

	} else {
		/* FIXME: if empty, should we stop? */
		BER_BVSTR( id, "" );
	}

	if ( !BER_BVISEMPTY( &conn->c_sasl_bind_mech ) ) {
		mech = &conn->c_sasl_bind_mech;
	} else {
		mech = &conn->c_authmech;
	}

	/* An authcID needs to be converted to authzID form. Set the
	 * values directly into *dn; they will be normalized later. (and
	 * normalizing always makes a new copy.) An ID from a TLS certificate
	 * is already normalized, so copy it and skip normalization.
	 */
	if( flags & SLAP_GETDN_AUTHCID ) {
		if( bvmatch( mech, &ext_bv )) {
			/* EXTERNAL DNs are already normalized */
			assert( !BER_BVISNULL( id ) );

			do_norm = 0;
			is_dn = SET_DN;
			ber_dupbv_x( dn, id, op->o_tmpmemctx );

		} else {
			/* convert to u:<username> form */
			is_dn = SET_U;
			*dn = *id;
		}
	}

	if( is_dn == SET_NONE ) {
		if( !strncasecmp( id->bv_val, "u:", STRLENOF( "u:" ) ) ) {
			is_dn = SET_U;
			dn->bv_val = id->bv_val + STRLENOF( "u:" );
			dn->bv_len = id->bv_len - STRLENOF( "u:" );

		} else if ( !strncasecmp( id->bv_val, "dn:", STRLENOF( "dn:" ) ) ) {
			is_dn = SET_DN;
			dn->bv_val = id->bv_val + STRLENOF( "dn:" );
			dn->bv_len = id->bv_len - STRLENOF( "dn:" );
		}
	}

	/* No other possibilities from here */
	if( is_dn == SET_NONE ) {
		BER_BVZERO( dn );
		return( LDAP_INAPPROPRIATE_AUTH );
	}

	/* Username strings */
	if( is_dn == SET_U ) {
		/* ITS#3419: values may need escape */
		LDAPRDN		DN[ 5 ];
		LDAPAVA 	*RDNs[ 4 ][ 2 ];
		LDAPAVA 	AVAs[ 4 ];
		int		irdn;

		irdn = 0;
		DN[ irdn ] = RDNs[ irdn ];
		RDNs[ irdn ][ 0 ] = &AVAs[ irdn ];
		AVAs[ irdn ].la_attr = slap_schema.si_ad_uid->ad_cname;
		AVAs[ irdn ].la_value = *dn;
		AVAs[ irdn ].la_flags = LDAP_AVA_NULL;
		AVAs[ irdn ].la_private = NULL;
		RDNs[ irdn ][ 1 ] = NULL;

		if ( user_realm && *user_realm ) {
			irdn++;
			DN[ irdn ] = RDNs[ irdn ];
			RDNs[ irdn ][ 0 ] = &AVAs[ irdn ];
			AVAs[ irdn ].la_attr = slap_schema.si_ad_cn->ad_cname;
			ber_str2bv( user_realm, 0, 0, &AVAs[ irdn ].la_value );
			AVAs[ irdn ].la_flags = LDAP_AVA_NULL;
			AVAs[ irdn ].la_private = NULL;
			RDNs[ irdn ][ 1 ] = NULL;
		}

		if ( !BER_BVISNULL( mech ) ) {
			irdn++;
			DN[ irdn ] = RDNs[ irdn ];
			RDNs[ irdn ][ 0 ] = &AVAs[ irdn ];
			AVAs[ irdn ].la_attr = slap_schema.si_ad_cn->ad_cname;
			AVAs[ irdn ].la_value = *mech;
			AVAs[ irdn ].la_flags = LDAP_AVA_NULL;
			AVAs[ irdn ].la_private = NULL;
			RDNs[ irdn ][ 1 ] = NULL;
		}

		irdn++;
		DN[ irdn ] = RDNs[ irdn ];
		RDNs[ irdn ][ 0 ] = &AVAs[ irdn ];
		AVAs[ irdn ].la_attr = slap_schema.si_ad_cn->ad_cname;
		BER_BVSTR( &AVAs[ irdn ].la_value, "auth" );
		AVAs[ irdn ].la_flags = LDAP_AVA_NULL;
		AVAs[ irdn ].la_private = NULL;
		RDNs[ irdn ][ 1 ] = NULL;

		irdn++;
		DN[ irdn ] = NULL;

		rc = ldap_dn2bv_x( DN, dn, LDAP_DN_FORMAT_LDAPV3,
				op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			BER_BVZERO( dn );
			return rc;
		}

		Debug( LDAP_DEBUG_TRACE,
			"slap_sasl_getdn: u:id converted to %s\n",
			dn->bv_val, 0, 0 );

	} else {
		
		/* Dup the DN in any case, so we don't risk 
		 * leaks or dangling pointers later,
		 * and the DN value is '\0' terminated */
		ber_dupbv_x( &dn2, dn, op->o_tmpmemctx );
		dn->bv_val = dn2.bv_val;
	}

	/* All strings are in DN form now. Normalize if needed. */
	if ( do_norm ) {
		rc = dnNormalize( 0, NULL, NULL, dn, &dn2, op->o_tmpmemctx );

		/* User DNs were constructed above and must be freed now */
		slap_sl_free( dn->bv_val, op->o_tmpmemctx );

		if ( rc != LDAP_SUCCESS ) {
			BER_BVZERO( dn );
			return rc;
		}
		*dn = dn2;
	}

	/* Run thru regexp */
	slap_sasl2dn( op, dn, &dn2, flags );
	if( !BER_BVISNULL( &dn2 ) ) {
		slap_sl_free( dn->bv_val, op->o_tmpmemctx );
		*dn = dn2;
		Debug( LDAP_DEBUG_TRACE,
			"slap_sasl_getdn: dn:id converted to %s\n",
			dn->bv_val, 0, 0 );
	}

	return( LDAP_SUCCESS );
}
