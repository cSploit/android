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
#include <ac/socket.h>

#include "slap.h"
#include "ldif.h"
#include "lutil.h"

#include "../../libraries/liblber/lber-int.h"

static SLAP_CTRL_PARSE_FN parseAssert;
static SLAP_CTRL_PARSE_FN parseDomainScope;
static SLAP_CTRL_PARSE_FN parseDontUseCopy;
static SLAP_CTRL_PARSE_FN parseManageDSAit;
static SLAP_CTRL_PARSE_FN parseNoOp;
static SLAP_CTRL_PARSE_FN parsePagedResults;
static SLAP_CTRL_PARSE_FN parsePermissiveModify;
static SLAP_CTRL_PARSE_FN parsePreRead, parsePostRead;
static SLAP_CTRL_PARSE_FN parseProxyAuthz;
static SLAP_CTRL_PARSE_FN parseRelax;
static SLAP_CTRL_PARSE_FN parseSearchOptions;
#ifdef SLAP_CONTROL_X_SORTEDRESULTS
static SLAP_CTRL_PARSE_FN parseSortedResults;
#endif
static SLAP_CTRL_PARSE_FN parseSubentries;
#ifdef SLAP_CONTROL_X_TREE_DELETE
static SLAP_CTRL_PARSE_FN parseTreeDelete;
#endif
static SLAP_CTRL_PARSE_FN parseValuesReturnFilter;
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
static SLAP_CTRL_PARSE_FN parseSessionTracking;
#endif
#ifdef SLAP_CONTROL_X_WHATFAILED
static SLAP_CTRL_PARSE_FN parseWhatFailed;
#endif

#undef sc_mask /* avoid conflict with Irix 6.5 <sys/signal.h> */

const struct berval slap_pre_read_bv = BER_BVC(LDAP_CONTROL_PRE_READ);
const struct berval slap_post_read_bv = BER_BVC(LDAP_CONTROL_POST_READ);

struct slap_control_ids slap_cids;

struct slap_control {
	/* Control OID */
	char *sc_oid;

	/* The controlID for this control */
	int sc_cid;

	/* Operations supported by control */
	slap_mask_t sc_mask;

	/* Extended operations supported by control */
	char **sc_extendedops;		/* input */
	BerVarray sc_extendedopsbv;	/* run-time use */

	/* Control parsing callback */
	SLAP_CTRL_PARSE_FN *sc_parse;

	LDAP_SLIST_ENTRY(slap_control) sc_next;
};

static LDAP_SLIST_HEAD(ControlsList, slap_control) controls_list
	= LDAP_SLIST_HEAD_INITIALIZER(&controls_list);

/*
 * all known request control OIDs should be added to this list
 */
/*
 * NOTE: initialize num_known_controls to 1 so that cid = 0 always
 * addresses an undefined control; this allows to safely test for 
 * well known controls even if they are not registered, e.g. if 
 * they get moved to modules.  An example is sc_LDAPsync, which 
 * is implemented in the syncprov overlay and thus, if configured 
 * as dynamic module, may not be registered.  One side effect is that 
 * slap_known_controls[0] == NULL, so it should always be used 
 * starting from 1.
 * FIXME: should we define the "undefined control" oid?
 */
char *slap_known_controls[SLAP_MAX_CIDS+1];
static int num_known_controls = 1;

static char *proxy_authz_extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	LDAP_EXOP_WHO_AM_I,
	LDAP_EXOP_REFRESH,
	NULL
};

static char *manageDSAit_extops[] = {
	LDAP_EXOP_REFRESH,
	NULL
};

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
static char *session_tracking_extops[] = {
	LDAP_EXOP_MODIFY_PASSWD,
	LDAP_EXOP_WHO_AM_I,
	LDAP_EXOP_REFRESH,
	NULL
};
#endif

static struct slap_control control_defs[] = {
	{  LDAP_CONTROL_ASSERT,
 		(int)offsetof(struct slap_control_ids, sc_assert),
		SLAP_CTRL_UPDATE|SLAP_CTRL_COMPARE|SLAP_CTRL_SEARCH,
		NULL, NULL,
		parseAssert, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_PRE_READ,
 		(int)offsetof(struct slap_control_ids, sc_preRead),
		SLAP_CTRL_DELETE|SLAP_CTRL_MODIFY|SLAP_CTRL_RENAME,
		NULL, NULL,
		parsePreRead, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_POST_READ,
 		(int)offsetof(struct slap_control_ids, sc_postRead),
		SLAP_CTRL_ADD|SLAP_CTRL_MODIFY|SLAP_CTRL_RENAME,
		NULL, NULL,
		parsePostRead, LDAP_SLIST_ENTRY_INITIALIZER(next) },
 	{ LDAP_CONTROL_VALUESRETURNFILTER,
 		(int)offsetof(struct slap_control_ids, sc_valuesReturnFilter),
 		SLAP_CTRL_GLOBAL|SLAP_CTRL_SEARCH,
		NULL, NULL,
		parseValuesReturnFilter, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_PAGEDRESULTS,
 		(int)offsetof(struct slap_control_ids, sc_pagedResults),
		SLAP_CTRL_SEARCH,
		NULL, NULL,
		parsePagedResults, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#ifdef SLAP_CONTROL_X_SORTEDRESULTS
	{ LDAP_CONTROL_SORTREQUEST,
 		(int)offsetof(struct slap_control_ids, sc_sortedResults),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_SEARCH|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseSortedResults, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#endif
	{ LDAP_CONTROL_X_DOMAIN_SCOPE,
 		(int)offsetof(struct slap_control_ids, sc_domainScope),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_SEARCH|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseDomainScope, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_DONTUSECOPY,
 		(int)offsetof(struct slap_control_ids, sc_dontUseCopy),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_INTROGATE,
		NULL, NULL,
		parseDontUseCopy, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_X_PERMISSIVE_MODIFY,
 		(int)offsetof(struct slap_control_ids, sc_permissiveModify),
		SLAP_CTRL_MODIFY|SLAP_CTRL_HIDE,
		NULL, NULL,
		parsePermissiveModify, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#ifdef SLAP_CONTROL_X_TREE_DELETE
	{ LDAP_CONTROL_X_TREE_DELETE,
 		(int)offsetof(struct slap_control_ids, sc_treeDelete),
		SLAP_CTRL_DELETE|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseTreeDelete, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#endif
	{ LDAP_CONTROL_X_SEARCH_OPTIONS,
 		(int)offsetof(struct slap_control_ids, sc_searchOptions),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_SEARCH|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseSearchOptions, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_SUBENTRIES,
 		(int)offsetof(struct slap_control_ids, sc_subentries),
		SLAP_CTRL_SEARCH,
		NULL, NULL,
		parseSubentries, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_NOOP,
 		(int)offsetof(struct slap_control_ids, sc_noOp),
		SLAP_CTRL_ACCESS|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseNoOp, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_RELAX,
 		(int)offsetof(struct slap_control_ids, sc_relax),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_UPDATE|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseRelax, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#ifdef LDAP_X_TXN
	{ LDAP_CONTROL_X_TXN_SPEC,
 		(int)offsetof(struct slap_control_ids, sc_txnSpec),
		SLAP_CTRL_UPDATE|SLAP_CTRL_HIDE,
		NULL, NULL,
		txn_spec_ctrl, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#endif
	{ LDAP_CONTROL_MANAGEDSAIT,
 		(int)offsetof(struct slap_control_ids, sc_manageDSAit),
		SLAP_CTRL_ACCESS,
		manageDSAit_extops, NULL,
		parseManageDSAit, LDAP_SLIST_ENTRY_INITIALIZER(next) },
	{ LDAP_CONTROL_PROXY_AUTHZ,
 		(int)offsetof(struct slap_control_ids, sc_proxyAuthz),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_ACCESS,
		proxy_authz_extops, NULL,
		parseProxyAuthz, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#ifdef SLAP_CONTROL_X_SESSION_TRACKING
	{ LDAP_CONTROL_X_SESSION_TRACKING,
 		(int)offsetof(struct slap_control_ids, sc_sessionTracking),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_ACCESS|SLAP_CTRL_BIND|SLAP_CTRL_HIDE,
		session_tracking_extops, NULL,
		parseSessionTracking, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#endif
#ifdef SLAP_CONTROL_X_WHATFAILED
	{ LDAP_CONTROL_X_WHATFAILED,
 		(int)offsetof(struct slap_control_ids, sc_whatFailed),
		SLAP_CTRL_GLOBAL|SLAP_CTRL_ACCESS|SLAP_CTRL_HIDE,
		NULL, NULL,
		parseWhatFailed, LDAP_SLIST_ENTRY_INITIALIZER(next) },
#endif

	{ NULL, 0, 0, NULL, 0, NULL, LDAP_SLIST_ENTRY_INITIALIZER(next) }
};

static struct slap_control *
find_ctrl( const char *oid );

/*
 * Register a supported control.
 *
 * This can be called by an OpenLDAP plugin or, indirectly, by a
 * SLAPI plugin calling slapi_register_supported_control().
 *
 * NOTE: if flags == 1 the control is replaced if already registered;
 * otherwise registering an already registered control is not allowed.
 */
int
register_supported_control2(const char *controloid,
	slap_mask_t controlmask,
	char **controlexops,
	SLAP_CTRL_PARSE_FN *controlparsefn,
	unsigned flags,
	int *controlcid)
{
	struct slap_control *sc = NULL;
	int i;
	BerVarray extendedopsbv = NULL;

	if ( num_known_controls >= SLAP_MAX_CIDS ) {
		Debug( LDAP_DEBUG_ANY, "Too many controls registered."
			" Recompile slapd with SLAP_MAX_CIDS defined > %d\n",
		SLAP_MAX_CIDS, 0, 0 );
		return LDAP_OTHER;
	}

	if ( controloid == NULL ) {
		return LDAP_PARAM_ERROR;
	}

	/* check if already registered */
	for ( i = 0; slap_known_controls[ i ]; i++ ) {
		if ( strcmp( controloid, slap_known_controls[ i ] ) == 0 ) {
			if ( flags == 1 ) {
				Debug( LDAP_DEBUG_TRACE,
					"Control %s already registered; replacing.\n",
					controloid, 0, 0 );
				/* (find and) replace existing handler */
				sc = find_ctrl( controloid );
				assert( sc != NULL );
				break;
			}

			Debug( LDAP_DEBUG_ANY,
				"Control %s already registered.\n",
				controloid, 0, 0 );
			return LDAP_PARAM_ERROR;
		}
	}

	/* turn compatible extended operations into bervals */
	if ( controlexops != NULL ) {
		int i;

		for ( i = 0; controlexops[ i ]; i++ );

		extendedopsbv = ber_memcalloc( i + 1, sizeof( struct berval ) );
		if ( extendedopsbv == NULL ) {
			return LDAP_NO_MEMORY;
		}

		for ( i = 0; controlexops[ i ]; i++ ) {
			ber_str2bv( controlexops[ i ], 0, 1, &extendedopsbv[ i ] );
		}
	}

	if ( sc == NULL ) {
		sc = (struct slap_control *)SLAP_MALLOC( sizeof( *sc ) );
		if ( sc == NULL ) {
			return LDAP_NO_MEMORY;
		}

		sc->sc_oid = ch_strdup( controloid );
		sc->sc_cid = num_known_controls;

		/* Update slap_known_controls, too. */
		slap_known_controls[num_known_controls - 1] = sc->sc_oid;
		slap_known_controls[num_known_controls++] = NULL;

		LDAP_SLIST_NEXT( sc, sc_next ) = NULL;
		LDAP_SLIST_INSERT_HEAD( &controls_list, sc, sc_next );

	} else {
		if ( sc->sc_extendedopsbv ) {
			/* FIXME: in principle, we should rather merge
			 * existing extops with those supported by the
			 * new control handling implementation.
			 * In fact, whether a control is compatible with
			 * an extop should not be a matter of implementation.
			 * We likely also need a means for a newly
			 * registered extop to declare that it is
			 * comptible with an already registered control.
			 */
			ber_bvarray_free( sc->sc_extendedopsbv );
			sc->sc_extendedopsbv = NULL;
			sc->sc_extendedops = NULL;
		}
	}

	sc->sc_extendedopsbv = extendedopsbv;
	sc->sc_mask = controlmask;
	sc->sc_parse = controlparsefn;
	if ( controlcid ) {
		*controlcid = sc->sc_cid;
	}

	return LDAP_SUCCESS;
}

#ifdef SLAP_CONFIG_DELETE
int
unregister_supported_control( const char *controloid )
{
	struct slap_control *sc;
	int i;

	if ( controloid == NULL || (sc = find_ctrl( controloid )) == NULL ){
		return -1;
	}

	for ( i = 0; slap_known_controls[ i ]; i++ ) {
		if ( strcmp( controloid, slap_known_controls[ i ] ) == 0 ) {
			do {
				slap_known_controls[ i ] = slap_known_controls[ i+1 ];
			} while ( slap_known_controls[ i++ ] );
			num_known_controls--;
			break;
		}
	}

	LDAP_SLIST_REMOVE(&controls_list, sc, slap_control, sc_next);
	ch_free( sc->sc_oid );
	if ( sc->sc_extendedopsbv != NULL ) {
		ber_bvarray_free( sc->sc_extendedopsbv );
	}
	ch_free( sc );

	return 0;
}
#endif /* SLAP_CONFIG_DELETE */

/*
 * One-time initialization of internal controls.
 */
int
slap_controls_init( void )
{
	int i, rc;

	rc = LDAP_SUCCESS;

	for ( i = 0; control_defs[i].sc_oid != NULL; i++ ) {
		int *cid = (int *)(((char *)&slap_cids) + control_defs[i].sc_cid );
		rc = register_supported_control( control_defs[i].sc_oid,
			control_defs[i].sc_mask, control_defs[i].sc_extendedops,
			control_defs[i].sc_parse, cid );
		if ( rc != LDAP_SUCCESS ) break;
	}

	return rc;
}

/*
 * Free memory associated with list of supported controls.
 */
void
controls_destroy( void )
{
	struct slap_control *sc;

	while ( !LDAP_SLIST_EMPTY(&controls_list) ) {
		sc = LDAP_SLIST_FIRST(&controls_list);
		LDAP_SLIST_REMOVE_HEAD(&controls_list, sc_next);

		ch_free( sc->sc_oid );
		if ( sc->sc_extendedopsbv != NULL ) {
			ber_bvarray_free( sc->sc_extendedopsbv );
		}
		ch_free( sc );
	}
}

/*
 * Format the supportedControl attribute of the root DSE,
 * detailing which controls are supported by the directory
 * server.
 */
int
controls_root_dse_info( Entry *e )
{
	AttributeDescription *ad_supportedControl
		= slap_schema.si_ad_supportedControl;
	struct berval vals[2];
	struct slap_control *sc;

	vals[1].bv_val = NULL;
	vals[1].bv_len = 0;

	LDAP_SLIST_FOREACH( sc, &controls_list, sc_next ) {
		if( sc->sc_mask & SLAP_CTRL_HIDE ) continue;

		vals[0].bv_val = sc->sc_oid;
		vals[0].bv_len = strlen( sc->sc_oid );

		if ( attr_merge( e, ad_supportedControl, vals, NULL ) ) {
			return -1;
		}
	}

	return 0;
}

/*
 * Return a list of OIDs and operation masks for supported
 * controls. Used by SLAPI.
 */
int
get_supported_controls(char ***ctrloidsp,
	slap_mask_t **ctrlmasks)
{
	int n;
	char **oids;
	slap_mask_t *masks;
	struct slap_control *sc;

	n = 0;

	LDAP_SLIST_FOREACH( sc, &controls_list, sc_next ) {
		n++;
	}

	if ( n == 0 ) {
		*ctrloidsp = NULL;
		*ctrlmasks = NULL;
		return LDAP_SUCCESS;
	}

	oids = (char **)SLAP_MALLOC( (n + 1) * sizeof(char *) );
	if ( oids == NULL ) {
		return LDAP_NO_MEMORY;
	}
	masks = (slap_mask_t *)SLAP_MALLOC( (n + 1) * sizeof(slap_mask_t) );
	if  ( masks == NULL ) {
		SLAP_FREE( oids );
		return LDAP_NO_MEMORY;
	}

	n = 0;

	LDAP_SLIST_FOREACH( sc, &controls_list, sc_next ) {
		oids[n] = ch_strdup( sc->sc_oid );
		masks[n] = sc->sc_mask;
		n++;
	}
	oids[n] = NULL;
	masks[n] = 0;

	*ctrloidsp = oids;
	*ctrlmasks = masks;

	return LDAP_SUCCESS;
}

/*
 * Find a control given its OID.
 */
static struct slap_control *
find_ctrl( const char *oid )
{
	struct slap_control *sc;

	LDAP_SLIST_FOREACH( sc, &controls_list, sc_next ) {
		if ( strcmp( oid, sc->sc_oid ) == 0 ) {
			return sc;
		}
	}

	return NULL;
}

int
slap_find_control_id(
	const char *oid,
	int *cid )
{
	struct slap_control *ctrl = find_ctrl( oid );
	if ( ctrl ) {
		if ( cid ) *cid = ctrl->sc_cid;
		return LDAP_SUCCESS;
	}
	return LDAP_CONTROL_NOT_FOUND;
}

int
slap_global_control( Operation *op, const char *oid, int *cid )
{
	struct slap_control *ctrl = find_ctrl( oid );

	if ( ctrl == NULL ) {
		/* should not be reachable */
		Debug( LDAP_DEBUG_ANY,
			"slap_global_control: unrecognized control: %s\n",      
			oid, 0, 0 );
		return LDAP_CONTROL_NOT_FOUND;
	}

	if ( cid ) *cid = ctrl->sc_cid;

	if ( ( ctrl->sc_mask & SLAP_CTRL_GLOBAL ) ||
		( ( op->o_tag & LDAP_REQ_SEARCH ) &&
		( ctrl->sc_mask & SLAP_CTRL_GLOBAL_SEARCH ) ) )
	{
		return LDAP_COMPARE_TRUE;
	}

#if 0
	Debug( LDAP_DEBUG_TRACE,
		"slap_global_control: unavailable control: %s\n",      
		oid, 0, 0 );
#endif

	return LDAP_COMPARE_FALSE;
}

void slap_free_ctrls(
	Operation *op,
	LDAPControl **ctrls )
{
	int i;

	for (i=0; ctrls[i]; i++) {
		op->o_tmpfree(ctrls[i], op->o_tmpmemctx );
	}
	op->o_tmpfree( ctrls, op->o_tmpmemctx );
}

int slap_add_ctrls(
	Operation *op,
	SlapReply *rs,
	LDAPControl **ctrls )
{
	int i = 0, j;
	LDAPControl **ctrlsp;

	if ( rs->sr_ctrls ) {
		for ( ; rs->sr_ctrls[ i ]; i++ ) ;
	}

	for ( j=0; ctrls[j]; j++ ) ;

	ctrlsp = op->o_tmpalloc(( i+j+1 )*sizeof(LDAPControl *), op->o_tmpmemctx );
	i = 0;
	if ( rs->sr_ctrls ) {
		for ( ; rs->sr_ctrls[i]; i++ )
			ctrlsp[i] = rs->sr_ctrls[i];
	}
	for ( j=0; ctrls[j]; j++)
		ctrlsp[i++] = ctrls[j];
	ctrlsp[i] = NULL;

	if ( rs->sr_flags & REP_CTRLS_MUSTBEFREED )
		op->o_tmpfree( rs->sr_ctrls, op->o_tmpmemctx );
	rs->sr_ctrls = ctrlsp;
	rs->sr_flags |= REP_CTRLS_MUSTBEFREED;
	return i;
}

int slap_parse_ctrl(
	Operation *op,
	SlapReply *rs,
	LDAPControl *control,
	const char **text )
{
	struct slap_control *sc;
	int rc = LDAP_SUCCESS;

	sc = find_ctrl( control->ldctl_oid );
	if( sc != NULL ) {
		/* recognized control */
		slap_mask_t tagmask;
		switch( op->o_tag ) {
		case LDAP_REQ_ADD:
			tagmask = SLAP_CTRL_ADD;
			break;
		case LDAP_REQ_BIND:
			tagmask = SLAP_CTRL_BIND;
			break;
		case LDAP_REQ_COMPARE:
			tagmask = SLAP_CTRL_COMPARE;
			break;
		case LDAP_REQ_DELETE:
			tagmask = SLAP_CTRL_DELETE;
			break;
		case LDAP_REQ_MODIFY:
			tagmask = SLAP_CTRL_MODIFY;
			break;
		case LDAP_REQ_RENAME:
			tagmask = SLAP_CTRL_RENAME;
			break;
		case LDAP_REQ_SEARCH:
			tagmask = SLAP_CTRL_SEARCH;
			break;
		case LDAP_REQ_UNBIND:
			tagmask = SLAP_CTRL_UNBIND;
			break;
		case LDAP_REQ_ABANDON:
			tagmask = SLAP_CTRL_ABANDON;
			break;
		case LDAP_REQ_EXTENDED:
			tagmask=~0L;
			assert( op->ore_reqoid.bv_val != NULL );
			if( sc->sc_extendedopsbv != NULL ) {
				int i;
				for( i=0; !BER_BVISNULL( &sc->sc_extendedopsbv[i] ); i++ ) {
					if( bvmatch( &op->ore_reqoid,
						&sc->sc_extendedopsbv[i] ) )
					{
						tagmask=0L;
						break;
					}
				}
			}
			break;
		default:
			*text = "controls internal error";
			return LDAP_OTHER;
		}

		if (( sc->sc_mask & tagmask ) == tagmask ) {
			/* available extension */
			if ( sc->sc_parse ) {
				rc = sc->sc_parse( op, rs, control );
				assert( rc != LDAP_UNAVAILABLE_CRITICAL_EXTENSION );

			} else if ( control->ldctl_iscritical ) {
				*text = "not yet implemented";
				rc = LDAP_OTHER;
			}


		} else if ( control->ldctl_iscritical ) {
			/* unavailable CRITICAL control */
			*text = "critical extension is unavailable";
			rc = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
		}

	} else if ( control->ldctl_iscritical ) {
		/* unrecognized CRITICAL control */
		*text = "critical extension is not recognized";
		rc = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
	}

	return rc;
}

int
get_ctrls(
	Operation *op,
	SlapReply *rs,
	int sendres )
{
	return get_ctrls2( op, rs, sendres, LDAP_TAG_CONTROLS );
}

int
get_ctrls2(
	Operation *op,
	SlapReply *rs,
	int sendres,
	ber_tag_t ctag )
{
	int nctrls = 0;
	ber_tag_t tag;
	ber_len_t len;
	char *opaque;
	BerElement *ber = op->o_ber;
	struct berval bv;
#ifdef SLAP_CONTROL_X_WHATFAILED
	/* NOTE: right now, slapd checks the validity of each control
	 * while parsing.  As a consequence, it can only detect one
	 * cause of failure at a time.  This results in returning
	 * exactly one OID with the whatFailed control, or no control
	 * at all.
	 */
	char *failed_oid = NULL;
#endif

	len = ber_pvt_ber_remaining(ber);

	if( len == 0) {
		/* no controls */
		rs->sr_err = LDAP_SUCCESS;
		return rs->sr_err;
	}

	if(( tag = ber_peek_tag( ber, &len )) != ctag ) {
		if( tag == LBER_ERROR ) {
			rs->sr_err = SLAPD_DISCONNECT;
			rs->sr_text = "unexpected data in PDU";
		}

		goto return_results;
	}

	Debug( LDAP_DEBUG_TRACE,
		"=> get_ctrls\n", 0, 0, 0 );

	if( op->o_protocol < LDAP_VERSION3 ) {
		rs->sr_err = SLAPD_DISCONNECT;
		rs->sr_text = "controls require LDAPv3";
		goto return_results;
	}

	/* one for first control, one for termination */
	op->o_ctrls = op->o_tmpalloc( 2 * sizeof(LDAPControl *), op->o_tmpmemctx );

#if 0
	if( op->ctrls == NULL ) {
		rs->sr_err = LDAP_NO_MEMORY;
		rs->sr_text = "no memory";
		goto return_results;
	}
#endif

	op->o_ctrls[nctrls] = NULL;

	/* step through each element */
	for( tag = ber_first_element( ber, &len, &opaque );
		tag != LBER_ERROR;
		tag = ber_next_element( ber, &len, opaque ) )
	{
		LDAPControl *c;
		LDAPControl **tctrls;

		c = op->o_tmpalloc( sizeof(LDAPControl), op->o_tmpmemctx );
		memset(c, 0, sizeof(LDAPControl));

		/* allocate pointer space for current controls (nctrls)
		 * + this control + extra NULL
		 */
		tctrls = op->o_tmprealloc( op->o_ctrls,
			(nctrls+2) * sizeof(LDAPControl *), op->o_tmpmemctx );

#if 0
		if( tctrls == NULL ) {
			ch_free( c );
			ldap_controls_free(op->o_ctrls);
			op->o_ctrls = NULL;

			rs->sr_err = LDAP_NO_MEMORY;
			rs->sr_text = "no memory";
			goto return_results;
		}
#endif
		op->o_ctrls = tctrls;

		op->o_ctrls[nctrls++] = c;
		op->o_ctrls[nctrls] = NULL;

		tag = ber_scanf( ber, "{m" /*}*/, &bv );
		c->ldctl_oid = bv.bv_val;

		if( tag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_TRACE, "=> get_ctrls: get oid failed.\n",
				0, 0, 0 );

			slap_free_ctrls( op, op->o_ctrls );
			op->o_ctrls = NULL;
			rs->sr_err = SLAPD_DISCONNECT;
			rs->sr_text = "decoding controls error";
			goto return_results;

		} else if( c->ldctl_oid == NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				"get_ctrls: conn %lu got emtpy OID.\n",
				op->o_connid, 0, 0 );

			slap_free_ctrls( op, op->o_ctrls );
			op->o_ctrls = NULL;
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			rs->sr_text = "OID field is empty";
			goto return_results;
		}

		tag = ber_peek_tag( ber, &len );

		if( tag == LBER_BOOLEAN ) {
			ber_int_t crit;
			tag = ber_scanf( ber, "b", &crit );

			if( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE, "=> get_ctrls: get crit failed.\n",
					0, 0, 0 );
				slap_free_ctrls( op, op->o_ctrls );
				op->o_ctrls = NULL;
				rs->sr_err = SLAPD_DISCONNECT;
				rs->sr_text = "decoding controls error";
				goto return_results;
			}

			c->ldctl_iscritical = (crit != 0);
			tag = ber_peek_tag( ber, &len );
		}

		if( tag == LBER_OCTETSTRING ) {
			tag = ber_scanf( ber, "m", &c->ldctl_value );

			if( tag == LBER_ERROR ) {
				Debug( LDAP_DEBUG_TRACE, "=> get_ctrls: conn %lu: "
					"%s (%scritical): get value failed.\n",
					op->o_connid, c->ldctl_oid,
					c->ldctl_iscritical ? "" : "non" );
				slap_free_ctrls( op, op->o_ctrls );
				op->o_ctrls = NULL;
				rs->sr_err = SLAPD_DISCONNECT;
				rs->sr_text = "decoding controls error";
				goto return_results;
			}
		}

		Debug( LDAP_DEBUG_TRACE,
			"=> get_ctrls: oid=\"%s\" (%scritical)\n",
			c->ldctl_oid, c->ldctl_iscritical ? "" : "non", 0 );

		rs->sr_err = slap_parse_ctrl( op, rs, c, &rs->sr_text );
		if ( rs->sr_err != LDAP_SUCCESS ) {
#ifdef SLAP_CONTROL_X_WHATFAILED
			failed_oid = c->ldctl_oid;
#endif
			goto return_results;
		}
	}

return_results:
	Debug( LDAP_DEBUG_TRACE,
		"<= get_ctrls: n=%d rc=%d err=\"%s\"\n",
		nctrls, rs->sr_err, rs->sr_text ? rs->sr_text : "");

	if( sendres && rs->sr_err != LDAP_SUCCESS ) {
		if( rs->sr_err == SLAPD_DISCONNECT ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			send_ldap_disconnect( op, rs );
			rs->sr_err = SLAPD_DISCONNECT;
		} else {
#ifdef SLAP_CONTROL_X_WHATFAILED
			/* might have not been parsed yet? */
			if ( failed_oid != NULL ) {
				if ( !get_whatFailed( op ) ) {
					/* look it up */

					/* step through each remaining element */
					for ( ; tag != LBER_ERROR; tag = ber_next_element( ber, &len, opaque ) )
					{
						LDAPControl c = { 0 };

						tag = ber_scanf( ber, "{m" /*}*/, &bv );
						c.ldctl_oid = bv.bv_val;

						if ( tag == LBER_ERROR ) {
							slap_free_ctrls( op, op->o_ctrls );
							op->o_ctrls = NULL;
							break;

						} else if ( c.ldctl_oid == NULL ) {
							slap_free_ctrls( op, op->o_ctrls );
							op->o_ctrls = NULL;
							break;
						}

						tag = ber_peek_tag( ber, &len );
						if ( tag == LBER_BOOLEAN ) {
							ber_int_t crit;
							tag = ber_scanf( ber, "b", &crit );
							if( tag == LBER_ERROR ) {
								slap_free_ctrls( op, op->o_ctrls );
								op->o_ctrls = NULL;
								break;
							}

							tag = ber_peek_tag( ber, &len );
						}

						if ( tag == LBER_OCTETSTRING ) {
							tag = ber_scanf( ber, "m", &c.ldctl_value );

							if( tag == LBER_ERROR ) {
								slap_free_ctrls( op, op->o_ctrls );
								op->o_ctrls = NULL;
								break;
							}
						}

						if ( strcmp( c.ldctl_oid, LDAP_CONTROL_X_WHATFAILED ) == 0 ) {
							const char *text;
							slap_parse_ctrl( op, rs, &c, &text );
							break;
						}
					}
				}

				if ( get_whatFailed( op ) ) {
					char *oids[ 2 ];
					oids[ 0 ] = failed_oid;
					oids[ 1 ] = NULL;
					slap_ctrl_whatFailed_add( op, rs, oids );
				}
			}
#endif

			send_ldap_result( op, rs );
		}
	}

	return rs->sr_err;
}

int
slap_remove_control(
	Operation	*op,
	SlapReply	*rs,
	int		ctrl,
	BI_chk_controls	fnc )
{
	int		i, j;

	switch ( op->o_ctrlflag[ ctrl ] ) {
	case SLAP_CONTROL_NONCRITICAL:
		for ( i = 0, j = -1; op->o_ctrls[ i ] != NULL; i++ ) {
			if ( strcmp( op->o_ctrls[ i ]->ldctl_oid,
				slap_known_controls[ ctrl - 1 ] ) == 0 )
			{
				j = i;
			}
		}

		if ( j == -1 ) {
			rs->sr_err = LDAP_OTHER;
			break;
		}

		if ( fnc ) {
			(void)fnc( op, rs );
		}

		op->o_tmpfree( op->o_ctrls[ j ], op->o_tmpmemctx );

		if ( i > 1 ) {
			AC_MEMCPY( &op->o_ctrls[ j ], &op->o_ctrls[ j + 1 ],
				( i - j ) * sizeof( LDAPControl * ) );

		} else {
			op->o_tmpfree( op->o_ctrls, op->o_tmpmemctx );
			op->o_ctrls = NULL;
		}

		op->o_ctrlflag[ ctrl ] = SLAP_CONTROL_IGNORED;

		Debug( LDAP_DEBUG_ANY, "%s: "
			"non-critical control \"%s\" not supported; stripped.\n",
			op->o_log_prefix, slap_known_controls[ ctrl ], 0 );
		/* fall thru */

	case SLAP_CONTROL_IGNORED:
	case SLAP_CONTROL_NONE:
		rs->sr_err = SLAP_CB_CONTINUE;
		break;

	case SLAP_CONTROL_CRITICAL:
		rs->sr_err = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
		if ( fnc ) {
			(void)fnc( op, rs );
		}
		Debug( LDAP_DEBUG_ANY, "%s: "
			"critical control \"%s\" not supported.\n",
			op->o_log_prefix, slap_known_controls[ ctrl ], 0 );
		break;

	default:
		/* handle all cases! */
		assert( 0 );
	}

	return rs->sr_err;
}

static int parseDontUseCopy (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_dontUseCopy != SLAP_CONTROL_NONE ) {
		rs->sr_text = "dontUseCopy control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "dontUseCopy control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( ( global_disallows & SLAP_DISALLOW_DONTUSECOPY_N_CRIT )
		&& !ctrl->ldctl_iscritical )
	{
		rs->sr_text = "dontUseCopy criticality of FALSE not allowed";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_dontUseCopy = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int parseRelax (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_relax != SLAP_CONTROL_NONE ) {
		rs->sr_text = "relax control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "relax control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_relax = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int parseManageDSAit (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_managedsait != SLAP_CONTROL_NONE ) {
		rs->sr_text = "manageDSAit control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "manageDSAit control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_managedsait = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int parseProxyAuthz (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	int		rc;
	struct berval	dn = BER_BVNULL;

	if ( op->o_proxy_authz != SLAP_CONTROL_NONE ) {
		rs->sr_text = "proxy authorization control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "proxy authorization control value absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( ( global_disallows & SLAP_DISALLOW_PROXY_AUTHZ_N_CRIT )
		&& !ctrl->ldctl_iscritical )
	{
		rs->sr_text = "proxied authorization criticality of FALSE not allowed";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !( global_allows & SLAP_ALLOW_PROXY_AUTHZ_ANON )
		&& BER_BVISEMPTY( &op->o_ndn ) )
	{
		rs->sr_text = "anonymous proxied authorization not allowed";
		return LDAP_PROXIED_AUTHORIZATION_DENIED;
	}

	op->o_proxy_authz = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	Debug( LDAP_DEBUG_ARGS,
		"parseProxyAuthz: conn %lu authzid=\"%s\"\n", 
		op->o_connid,
		ctrl->ldctl_value.bv_len ?  ctrl->ldctl_value.bv_val : "anonymous",
		0 );

	if ( BER_BVISEMPTY( &ctrl->ldctl_value )) {
		Debug( LDAP_DEBUG_TRACE,
			"parseProxyAuthz: conn=%lu anonymous\n", 
			op->o_connid, 0, 0 );

		/* anonymous */
		if ( !BER_BVISNULL( &op->o_ndn ) ) {
			op->o_ndn.bv_val[ 0 ] = '\0';
		}
		op->o_ndn.bv_len = 0;

		if ( !BER_BVISNULL( &op->o_dn ) ) {
			op->o_dn.bv_val[ 0 ] = '\0';
		}
		op->o_dn.bv_len = 0;

		return LDAP_SUCCESS;
	}

	rc = slap_sasl_getdn( op->o_conn, op, &ctrl->ldctl_value,
			NULL, &dn, SLAP_GETDN_AUTHZID );

	/* FIXME: empty DN in proxyAuthz control should be legal... */
	if( rc != LDAP_SUCCESS /* || !dn.bv_len */ ) {
		if ( dn.bv_val ) {
			ch_free( dn.bv_val );
		}
		rs->sr_text = "authzId mapping failed";
		return LDAP_PROXIED_AUTHORIZATION_DENIED;
	}

	Debug( LDAP_DEBUG_TRACE,
		"parseProxyAuthz: conn=%lu \"%s\"\n", 
		op->o_connid,
		dn.bv_len ? dn.bv_val : "(NULL)", 0 );

	rc = slap_sasl_authorized( op, &op->o_ndn, &dn );

	if ( rc ) {
		ch_free( dn.bv_val );
		rs->sr_text = "not authorized to assume identity";
		return LDAP_PROXIED_AUTHORIZATION_DENIED;
	}

	ch_free( op->o_ndn.bv_val );

	/*
	 * NOTE: since slap_sasl_getdn() returns a normalized dn,
	 * from now on op->o_dn is normalized
	 */
	op->o_ndn = dn;
	ber_bvreplace( &op->o_dn, &dn );

	Statslog( LDAP_DEBUG_STATS, "%s PROXYAUTHZ dn=\"%s\"\n",
	    op->o_log_prefix, dn.bv_val, 0, 0, 0 );

	return LDAP_SUCCESS;
}

static int parseNoOp (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_noop != SLAP_CONTROL_NONE ) {
		rs->sr_text = "noop control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "noop control value not empty";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_noop = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int parsePagedResults (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;
	struct berval	cookie;
	PagedResultsState	*ps;
	int		rc = LDAP_SUCCESS;
	ber_tag_t	tag;
	ber_int_t	size;

	if ( op->o_pagedresults != SLAP_CONTROL_NONE ) {
		rs->sr_text = "paged results control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "paged results control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "paged results control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	/* Parse the control value
	 *	realSearchControlValue ::= SEQUENCE {
	 *		size	INTEGER (0..maxInt),
	 *				-- requested page size from client
	 *				-- result set size estimate from server
	 *		cookie	OCTET STRING
	 * }
	 */
	ber_init2( ber, &ctrl->ldctl_value, LBER_USE_DER );

	tag = ber_scanf( ber, "{im}", &size, &cookie );

	if ( tag == LBER_ERROR ) {
		rs->sr_text = "paged results control could not be decoded";
		rc = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	if ( size < 0 ) {
		rs->sr_text = "paged results control size invalid";
		rc = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	ps = op->o_tmpalloc( sizeof(PagedResultsState), op->o_tmpmemctx );
	*ps = op->o_conn->c_pagedresults_state;
	ps->ps_size = size;
	ps->ps_cookieval = cookie;
	op->o_pagedresults_state = ps;
	if ( !cookie.bv_len ) {
		ps->ps_count = 0;
		ps->ps_cookie = 0;
		/* taint ps_cookie, to detect whether it's set */
		op->o_conn->c_pagedresults_state.ps_cookie = NOID;
	}

	/* NOTE: according to RFC 2696 3.:

    If the page size is greater than or equal to the sizeLimit value, the
    server should ignore the control as the request can be satisfied in a
    single page.

	 * NOTE: this assumes that the op->ors_slimit be set
	 * before the controls are parsed.     
	 */

	if ( op->ors_slimit > 0 && size >= op->ors_slimit ) {
		op->o_pagedresults = SLAP_CONTROL_IGNORED;

	} else if ( ctrl->ldctl_iscritical ) {
		op->o_pagedresults = SLAP_CONTROL_CRITICAL;

	} else {
		op->o_pagedresults = SLAP_CONTROL_NONCRITICAL;
	}

done:;
	return rc;
}

#ifdef SLAP_CONTROL_X_SORTEDRESULTS
static int parseSortedResults (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	int		rc = LDAP_SUCCESS;

	if ( op->o_sortedresults != SLAP_CONTROL_NONE ) {
		rs->sr_text = "sorted results control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sorted results control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sorted results control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	/* blow off parsing the value */

	op->o_sortedresults = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return rc;
}
#endif

static int parseAssert (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	BerElement	*ber;
	struct berval	fstr = BER_BVNULL;

	if ( op->o_assert != SLAP_CONTROL_NONE ) {
		rs->sr_text = "assert control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "assert control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value )) {
		rs->sr_text = "assert control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber = ber_init( &(ctrl->ldctl_value) );
	if (ber == NULL) {
		rs->sr_text = "assert control: internal error";
		return LDAP_OTHER;
	}

	rs->sr_err = get_filter( op, ber, (Filter **)&(op->o_assertion),
		&rs->sr_text);
	(void) ber_free( ber, 1 );
	if( rs->sr_err != LDAP_SUCCESS ) {
		if( rs->sr_err == SLAPD_DISCONNECT ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			send_ldap_disconnect( op, rs );
			rs->sr_err = SLAPD_DISCONNECT;
		} else {
			send_ldap_result( op, rs );
		}
		if( op->o_assertion != NULL ) {
			filter_free_x( op, op->o_assertion, 1 );
		}
		return rs->sr_err;
	}

#ifdef LDAP_DEBUG
	filter2bv_x( op, op->o_assertion, &fstr );

	Debug( LDAP_DEBUG_ARGS, "parseAssert: conn %ld assert: %s\n",
		op->o_connid, fstr.bv_len ? fstr.bv_val : "empty" , 0 );
	op->o_tmpfree( fstr.bv_val, op->o_tmpmemctx );
#endif

	op->o_assert = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	rs->sr_err = LDAP_SUCCESS;
	return LDAP_SUCCESS;
}

#define READMSG(post, msg) \
	( post ? "postread control: " msg : "preread control: " msg )

static int
parseReadAttrs(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl,
	int post )
{
	ber_len_t	siz, off, i;
	BerElement	*ber;
	AttributeName	*an = NULL;

	if ( ( post && op->o_postread != SLAP_CONTROL_NONE ) ||
		( !post && op->o_preread != SLAP_CONTROL_NONE ) )
	{
		rs->sr_text = READMSG( post, "specified multiple times" );
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = READMSG( post, "value is absent" );
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = READMSG( post, "value is empty" );
		return LDAP_PROTOCOL_ERROR;
	}

#ifdef LDAP_X_TXN
	if ( op->o_txnSpec ) { /* temporary limitation */
		rs->sr_text = READMSG( post, "cannot perform in transaction" );
		return LDAP_UNWILLING_TO_PERFORM;
	}
#endif

	ber = ber_init( &ctrl->ldctl_value );
	if ( ber == NULL ) {
		rs->sr_text = READMSG( post, "internal error" );
		return LDAP_OTHER;
	}

	rs->sr_err = LDAP_SUCCESS;
	siz = sizeof( AttributeName );
	off = offsetof( AttributeName, an_name );
	if ( ber_scanf( ber, "{M}", &an, &siz, off ) == LBER_ERROR ) {
		rs->sr_text = READMSG( post, "decoding error" );
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	for ( i = 0; i < siz; i++ ) {
		const char	*dummy = NULL;
		int		rc;

		an[i].an_desc = NULL;
		an[i].an_oc = NULL;
		an[i].an_flags = 0;
		rc = slap_bv2ad( &an[i].an_name, &an[i].an_desc, &dummy );
		if ( rc == LDAP_SUCCESS ) {
			an[i].an_name = an[i].an_desc->ad_cname;

		} else {
			int			j;
			static struct berval	special_attrs[] = {
				BER_BVC( LDAP_NO_ATTRS ),
				BER_BVC( LDAP_ALL_USER_ATTRIBUTES ),
				BER_BVC( LDAP_ALL_OPERATIONAL_ATTRIBUTES ),
				BER_BVNULL
			};

			/* deal with special attribute types */
			for ( j = 0; !BER_BVISNULL( &special_attrs[ j ] ); j++ ) {
				if ( bvmatch( &an[i].an_name, &special_attrs[ j ] ) ) {
					an[i].an_name = special_attrs[ j ];
					break;
				}
			}

			if ( BER_BVISNULL( &special_attrs[ j ] ) && ctrl->ldctl_iscritical ) {
				rs->sr_err = rc;
				rs->sr_text = dummy ? dummy
					: READMSG( post, "unknown attributeType" );
				goto done;
			}
		}
	}

	if ( post ) {
		op->o_postread_attrs = an;
		op->o_postread = ctrl->ldctl_iscritical
			? SLAP_CONTROL_CRITICAL
			: SLAP_CONTROL_NONCRITICAL;
	} else {
		op->o_preread_attrs = an;
		op->o_preread = ctrl->ldctl_iscritical
			? SLAP_CONTROL_CRITICAL
			: SLAP_CONTROL_NONCRITICAL;
	}

done:
	(void) ber_free( ber, 1 );
	return rs->sr_err;
}

static int parsePreRead (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	return parseReadAttrs( op, rs, ctrl, 0 );
}

static int parsePostRead (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	return parseReadAttrs( op, rs, ctrl, 1 );
}

static int parseValuesReturnFilter (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	BerElement	*ber;
	struct berval	fstr = BER_BVNULL;

	if ( op->o_valuesreturnfilter != SLAP_CONTROL_NONE ) {
		rs->sr_text = "valuesReturnFilter control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "valuesReturnFilter control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value )) {
		rs->sr_text = "valuesReturnFilter control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber = ber_init( &(ctrl->ldctl_value) );
	if (ber == NULL) {
		rs->sr_text = "internal error";
		return LDAP_OTHER;
	}

	rs->sr_err = get_vrFilter( op, ber,
		(ValuesReturnFilter **)&(op->o_vrFilter), &rs->sr_text);

	(void) ber_free( ber, 1 );

	if( rs->sr_err != LDAP_SUCCESS ) {
		if( rs->sr_err == SLAPD_DISCONNECT ) {
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			send_ldap_disconnect( op, rs );
			rs->sr_err = SLAPD_DISCONNECT;
		} else {
			send_ldap_result( op, rs );
		}
		if( op->o_vrFilter != NULL) vrFilter_free( op, op->o_vrFilter ); 
	}
#ifdef LDAP_DEBUG
	else {
		vrFilter2bv( op, op->o_vrFilter, &fstr );
	}

	Debug( LDAP_DEBUG_ARGS, "	vrFilter: %s\n",
		fstr.bv_len ? fstr.bv_val : "empty", 0, 0 );
	op->o_tmpfree( fstr.bv_val, op->o_tmpmemctx );
#endif

	op->o_valuesreturnfilter = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	rs->sr_err = LDAP_SUCCESS;
	return LDAP_SUCCESS;
}

static int parseSubentries (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_subentries != SLAP_CONTROL_NONE ) {
		rs->sr_text = "subentries control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	/* FIXME: should use BER library */
	if( ( ctrl->ldctl_value.bv_len != 3 )
		|| ( ctrl->ldctl_value.bv_val[0] != 0x01 )
		|| ( ctrl->ldctl_value.bv_val[1] != 0x01 ))
	{
		rs->sr_text = "subentries control value encoding is bogus";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_subentries = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	if (ctrl->ldctl_value.bv_val[2]) {
		set_subentries_visibility( op );
	}

	return LDAP_SUCCESS;
}

static int parsePermissiveModify (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_permissive_modify != SLAP_CONTROL_NONE ) {
		rs->sr_text = "permissiveModify control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "permissiveModify control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_permissive_modify = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

static int parseDomainScope (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_domain_scope != SLAP_CONTROL_NONE ) {
		rs->sr_text = "domainScope control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "domainScope control value not empty";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_domain_scope = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

#ifdef SLAP_CONTROL_X_TREE_DELETE
static int parseTreeDelete (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_tree_delete != SLAP_CONTROL_NONE ) {
		rs->sr_text = "treeDelete control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "treeDelete control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_tree_delete = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}
#endif

static int parseSearchOptions (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	BerElement *ber;
	ber_int_t search_flags;
	ber_tag_t tag;

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "searchOptions control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value )) {
		rs->sr_text = "searchOptions control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber = ber_init( &ctrl->ldctl_value );
	if( ber == NULL ) {
		rs->sr_text = "internal error";
		return LDAP_OTHER;
	}

	tag = ber_scanf( ber, "{i}", &search_flags );
	(void) ber_free( ber, 1 );

	if ( tag == LBER_ERROR ) {
		rs->sr_text = "searchOptions control decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( search_flags & ~(LDAP_SEARCH_FLAG_DOMAIN_SCOPE) ) {
		/* Search flags not recognised so far,
		 * including:
		 *		LDAP_SEARCH_FLAG_PHANTOM_ROOT
		 */
		if ( ctrl->ldctl_iscritical ) {
			rs->sr_text = "searchOptions contained unrecognized flag";
			return LDAP_UNWILLING_TO_PERFORM;
		}

		/* Ignore */
		Debug( LDAP_DEBUG_TRACE,
			"searchOptions: conn=%lu unrecognized flag(s) 0x%x (non-critical)\n", 
			op->o_connid, (unsigned)search_flags, 0 );

		return LDAP_SUCCESS;
	}

	if ( search_flags & LDAP_SEARCH_FLAG_DOMAIN_SCOPE ) {
		if ( op->o_domain_scope != SLAP_CONTROL_NONE ) {
			rs->sr_text = "searchOptions control specified multiple times "
				"or with domainScope control";
			return LDAP_PROTOCOL_ERROR;
		}

		op->o_domain_scope = ctrl->ldctl_iscritical
			? SLAP_CONTROL_CRITICAL
			: SLAP_CONTROL_NONCRITICAL;
	}

	return LDAP_SUCCESS;
}

#ifdef SLAP_CONTROL_X_SESSION_TRACKING
struct berval session_tracking_formats[] = {
	BER_BVC( LDAP_CONTROL_X_SESSION_TRACKING_RADIUS_ACCT_SESSION_ID ),
		BER_BVC( "RADIUS-Acct-Session-Id" ),
	BER_BVC( LDAP_CONTROL_X_SESSION_TRACKING_RADIUS_ACCT_MULTI_SESSION_ID ),
		BER_BVC( "RADIUS-Acct-Multi-Session-Id" ),
	BER_BVC( LDAP_CONTROL_X_SESSION_TRACKING_USERNAME ),
		BER_BVC( "USERNAME" ),

	BER_BVNULL
};

static int parseSessionTracking(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	BerElement		*ber;
	ber_tag_t		tag;
	ber_len_t		len;
	int			i, rc;

	struct berval		sessionSourceIp = BER_BVNULL,
				sessionSourceName = BER_BVNULL,
				formatOID = BER_BVNULL,
				sessionTrackingIdentifier = BER_BVNULL;

	size_t			st_len, st_pos;

	if ( ctrl->ldctl_iscritical ) {
		rs->sr_text = "sessionTracking criticality is TRUE";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sessionTracking control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "sessionTracking control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	/* TODO: add the capability to determine if a client is allowed
	 * to use this control, based on identity, ip and so */

	ber = ber_init( &ctrl->ldctl_value );
	if ( ber == NULL ) {
		rs->sr_text = "internal error";
		return LDAP_OTHER;
	}

	tag = ber_skip_tag( ber, &len );
	if ( tag != LBER_SEQUENCE ) {
		tag = LBER_ERROR;
		goto error;
	}

	/* sessionSourceIp */
	tag = ber_peek_tag( ber, &len );
	if ( tag == LBER_DEFAULT ) {
		tag = LBER_ERROR;
		goto error;
	}

	if ( len == 0 ) {
		tag = ber_skip_tag( ber, &len );

	} else if ( len > 128 ) {
		rs->sr_text = "sessionTracking.sessionSourceIp too long";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto error;

	} else {
		tag = ber_scanf( ber, "m", &sessionSourceIp );
	}

	if ( ldif_is_not_printable( sessionSourceIp.bv_val, sessionSourceIp.bv_len ) ) {
		BER_BVZERO( &sessionSourceIp );
	}

	/* sessionSourceName */
	tag = ber_peek_tag( ber, &len );
	if ( tag == LBER_DEFAULT ) {
		tag = LBER_ERROR;
		goto error;
	}

	if ( len == 0 ) {
		tag = ber_skip_tag( ber, &len );

	} else if ( len > 65536 ) {
		rs->sr_text = "sessionTracking.sessionSourceName too long";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto error;

	} else {
		tag = ber_scanf( ber, "m", &sessionSourceName );
	}

	if ( ldif_is_not_printable( sessionSourceName.bv_val, sessionSourceName.bv_len ) ) {
		BER_BVZERO( &sessionSourceName );
	}

	/* formatOID */
	tag = ber_peek_tag( ber, &len );
	if ( tag == LBER_DEFAULT ) {
		tag = LBER_ERROR;
		goto error;
	}

	if ( len == 0 ) {
		rs->sr_text = "sessionTracking.formatOID empty";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto error;

	} else if ( len > 1024 ) {
		rs->sr_text = "sessionTracking.formatOID too long";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto error;

	} else {
		tag = ber_scanf( ber, "m", &formatOID );
	}

	rc = numericoidValidate( NULL, &formatOID );
	if ( rc != LDAP_SUCCESS ) {
		rs->sr_text = "sessionTracking.formatOID invalid";
		goto error;
	}

	for ( i = 0; !BER_BVISNULL( &session_tracking_formats[ i ] ); i += 2 )
	{
		if ( bvmatch( &formatOID, &session_tracking_formats[ i ] ) ) {
			formatOID = session_tracking_formats[ i + 1 ];
			break;
		}
	}

	/* sessionTrackingIdentifier */
	tag = ber_peek_tag( ber, &len );
	if ( tag == LBER_DEFAULT ) {
		tag = LBER_ERROR;
		goto error;
	}

	if ( len == 0 ) {
		tag = ber_skip_tag( ber, &len );

	} else {
		/* note: should not be more than 65536... */
		tag = ber_scanf( ber, "m", &sessionTrackingIdentifier );
		if ( ldif_is_not_printable( sessionTrackingIdentifier.bv_val, sessionTrackingIdentifier.bv_len ) ) {
			/* we want the OID printed, at least */
			BER_BVSTR( &sessionTrackingIdentifier, "" );
		}
	}

	/* closure */
	tag = ber_skip_tag( ber, &len );
	if ( tag != LBER_DEFAULT || len != 0 ) {
		tag = LBER_ERROR;
		goto error;
	}
	tag = 0;

	st_len = 0;
	if ( !BER_BVISNULL( &sessionSourceIp ) ) {
		st_len += STRLENOF( "IP=" ) + sessionSourceIp.bv_len;
	}
	if ( !BER_BVISNULL( &sessionSourceName ) ) {
		if ( st_len ) st_len++;
		st_len += STRLENOF( "NAME=" ) + sessionSourceName.bv_len;
	}
	if ( !BER_BVISNULL( &sessionTrackingIdentifier ) ) {
		if ( st_len ) st_len++;
		st_len += formatOID.bv_len + STRLENOF( "=" )
			+ sessionTrackingIdentifier.bv_len;
	}

	if ( st_len == 0 ) {
		goto error;
	}

	st_len += STRLENOF( " []" );
	st_pos = strlen( op->o_log_prefix );

	if ( sizeof( op->o_log_prefix ) - st_pos > st_len ) {
		char	*ptr = &op->o_log_prefix[ st_pos ];

		ptr = lutil_strcopy( ptr, " [" /*]*/ );

		st_len = 0;
		if ( !BER_BVISNULL( &sessionSourceIp ) ) {
			ptr = lutil_strcopy( ptr, "IP=" );
			ptr = lutil_strcopy( ptr, sessionSourceIp.bv_val );
			st_len++;
		}

		if ( !BER_BVISNULL( &sessionSourceName ) ) {
			if ( st_len ) *ptr++ = ' ';
			ptr = lutil_strcopy( ptr, "NAME=" );
			ptr = lutil_strcopy( ptr, sessionSourceName.bv_val );
			st_len++;
		}

		if ( !BER_BVISNULL( &sessionTrackingIdentifier ) ) {
			if ( st_len ) *ptr++ = ' ';
			ptr = lutil_strcopy( ptr, formatOID.bv_val );
			*ptr++ = '=';
			ptr = lutil_strcopy( ptr, sessionTrackingIdentifier.bv_val );
		}

		*ptr++ = /*[*/ ']';
		*ptr = '\0';
	}

error:;
	(void)ber_free( ber, 1 );

	if ( tag == LBER_ERROR ) {
		rs->sr_text = "sessionTracking control decoding error";
		return LDAP_PROTOCOL_ERROR;
	}


	return rs->sr_err;
}

int
slap_ctrl_session_tracking_add(
	Operation *op,
	SlapReply *rs,
	struct berval *ip,
	struct berval *name,
	struct berval *id,
	LDAPControl *ctrl )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *)&berbuf;

	static struct berval	oid = BER_BVC( LDAP_CONTROL_X_SESSION_TRACKING_USERNAME );

	assert( ctrl != NULL );

	ber_init2( ber, NULL, LBER_USE_DER );

	ber_printf( ber, "{OOOO}", ip, name, &oid, id ); 

	if ( ber_flatten2( ber, &ctrl->ldctl_value, 0 ) == -1 ) {
		rs->sr_err = LDAP_OTHER;
		goto done;
	}

	ctrl->ldctl_oid = LDAP_CONTROL_X_SESSION_TRACKING;
	ctrl->ldctl_iscritical = 0;

	rs->sr_err = LDAP_SUCCESS;

done:;
	return rs->sr_err;
}

int
slap_ctrl_session_tracking_request_add( Operation *op, SlapReply *rs, LDAPControl *ctrl )
{
	static struct berval	bv_unknown = BER_BVC( SLAP_STRING_UNKNOWN );
	struct berval		ip = BER_BVNULL,
				name = BER_BVNULL,
				id = BER_BVNULL;

	if ( !BER_BVISNULL( &op->o_conn->c_peer_name ) &&
		memcmp( op->o_conn->c_peer_name.bv_val, "IP=", STRLENOF( "IP=" ) ) == 0 )
	{
		char	*ptr;

		ip.bv_val = op->o_conn->c_peer_name.bv_val + STRLENOF( "IP=" );
		ip.bv_len = op->o_conn->c_peer_name.bv_len - STRLENOF( "IP=" );

		ptr = ber_bvchr( &ip, ':' );
		if ( ptr ) {
			ip.bv_len = ptr - ip.bv_val;
		}
	}

	if ( !BER_BVISNULL( &op->o_conn->c_peer_domain ) &&
		!bvmatch( &op->o_conn->c_peer_domain, &bv_unknown ) )
	{
		name = op->o_conn->c_peer_domain;
	}

	if ( !BER_BVISNULL( &op->o_dn ) && !BER_BVISEMPTY( &op->o_dn ) ) {
		id = op->o_dn;
	}

	return slap_ctrl_session_tracking_add( op, rs, &ip, &name, &id, ctrl );
}
#endif

#ifdef SLAP_CONTROL_X_WHATFAILED
static int parseWhatFailed(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	if ( op->o_whatFailed != SLAP_CONTROL_NONE ) {
		rs->sr_text = "\"WHat Failed?\" control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( !BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "\"What Failed?\" control value not absent";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_whatFailed = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	return LDAP_SUCCESS;
}

int
slap_ctrl_whatFailed_add(
	Operation *op,
	SlapReply *rs,
	char **oids )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *) &berbuf;
	LDAPControl **ctrls = NULL;
	struct berval ctrlval;
	int i, rc = LDAP_SUCCESS;

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );
	ber_printf( ber, "[" /*]*/ );
	for ( i = 0; oids[ i ] != NULL; i++ ) {
		ber_printf( ber, "s", oids[ i ] );
	}
	ber_printf( ber, /*[*/ "]" );

	if ( ber_flatten2( ber, &ctrlval, 0 ) == -1 ) {
		rc = LDAP_OTHER;
		goto done;
	}

	i = 0;
	if ( rs->sr_ctrls != NULL ) {
		for ( ; rs->sr_ctrls[ i ] != NULL; i++ ) {
			if ( strcmp( rs->sr_ctrls[ i ]->ldctl_oid, LDAP_CONTROL_X_WHATFAILED ) != 0 ) {
				/* TODO: add */
				assert( 0 );
			}
		}
	}

	ctrls = op->o_tmprealloc( rs->sr_ctrls,
			sizeof(LDAPControl *)*( i + 2 )
			+ sizeof(LDAPControl)
			+ ctrlval.bv_len + 1,
			op->o_tmpmemctx );
	if ( ctrls == NULL ) {
		rc = LDAP_OTHER;
		goto done;
	}
	ctrls[ i + 1 ] = NULL;
	ctrls[ i ] = (LDAPControl *)&ctrls[ i + 2 ];
	ctrls[ i ]->ldctl_oid = LDAP_CONTROL_X_WHATFAILED;
	ctrls[ i ]->ldctl_iscritical = 0;
	ctrls[ i ]->ldctl_value.bv_val = (char *)&ctrls[ i ][ 1 ];
	AC_MEMCPY( ctrls[ i ]->ldctl_value.bv_val, ctrlval.bv_val, ctrlval.bv_len + 1 );
	ctrls[ i ]->ldctl_value.bv_len = ctrlval.bv_len;

	ber_free_buf( ber );

	rs->sr_ctrls = ctrls;

done:;
	return rc;
}
#endif
