/* result.c - routines to send ldap results, errors, and referrals */
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

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include "slap.h"

const struct berval slap_dummy_bv = BER_BVNULL;

int slap_null_cb( Operation *op, SlapReply *rs )
{
	return 0;
}

int slap_freeself_cb( Operation *op, SlapReply *rs )
{
	assert( op->o_callback != NULL );

	op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
	op->o_callback = NULL;

	return SLAP_CB_CONTINUE;
}

static char *v2ref( BerVarray ref, const char *text )
{
	size_t len = 0, i = 0;
	char *v2;

	if(ref == NULL) {
		if (text) {
			return ch_strdup(text);
		} else {
			return NULL;
		}
	}
	
	if ( text != NULL ) {
		len = strlen( text );
		if (text[len-1] != '\n') {
		    i = 1;
		}
	}

	v2 = ch_malloc( len+i+sizeof("Referral:") );

	if( text != NULL ) {
		strcpy(v2, text);
		if( i ) {
			v2[len++] = '\n';
		}
	}
	strcpy( v2+len, "Referral:" );
	len += sizeof("Referral:");

	for( i=0; ref[i].bv_val != NULL; i++ ) {
		v2 = ch_realloc( v2, len + ref[i].bv_len + 1 );
		v2[len-1] = '\n';
		AC_MEMCPY(&v2[len], ref[i].bv_val, ref[i].bv_len );
		len += ref[i].bv_len;
		if (ref[i].bv_val[ref[i].bv_len-1] != '/') {
			++len;
		}
	}

	v2[len-1] = '\0';
	return v2;
}

ber_tag_t
slap_req2res( ber_tag_t tag )
{
	switch( tag ) {
	case LDAP_REQ_ADD:
	case LDAP_REQ_BIND:
	case LDAP_REQ_COMPARE:
	case LDAP_REQ_EXTENDED:
	case LDAP_REQ_MODIFY:
	case LDAP_REQ_MODRDN:
		tag++;
		break;

	case LDAP_REQ_DELETE:
		tag = LDAP_RES_DELETE;
		break;

	case LDAP_REQ_ABANDON:
	case LDAP_REQ_UNBIND:
		tag = LBER_SEQUENCE;
		break;

	case LDAP_REQ_SEARCH:
		tag = LDAP_RES_SEARCH_RESULT;
		break;

	default:
		tag = LBER_SEQUENCE;
	}

	return tag;
}

/*
 * SlapReply debugging enabled by USE_RS_ASSERT.
 *
 * Disabled by default, but compiled in (but still unused) when
 * LDAP_TEST.  #define USE_RS_ASSERT as nonzero to enable some
 * assertions which check the SlapReply.  USE_RS_ASSERT = 2 or higher
 * check aggressively, currently some code fail these tests.
 *
 * Environment variable $NO_RS_ASSERT controls how USE_RS_ASSERT handles
 * errors.  > 0: ignore errors, 0: abort (the default), < 0: just warn.
 *
 * Wrap LDAP operation calls in macros SLAP_OP() & co from proto-slap.h
 * to check the SlapReply.  contrib/slapd-tools/wrap_slap_ops converts
 * source code to use the macros.
 */
#if defined(LDAP_TEST) || (defined(USE_RS_ASSERT) && (USE_RS_ASSERT))

int rs_suppress_assert = 0;

/* RS_ASSERT() helper function */
void rs_assert_(const char*file, unsigned line, const char*fn, const char*cond)
{
	int no_assert = rs_suppress_assert, save_errno = errno;
	const char *s;

	if ( no_assert >= 0 ) {
		if ( no_assert == 0 && (s = getenv( "NO_RS_ASSERT" )) && *s ) {
			no_assert = rs_suppress_assert = atoi( s );
		}
		if ( no_assert > 0 ) {
			errno = save_errno;
			return;
		}
	}

#ifdef rs_assert_	/* proto-slap.h #defined away the fn parameter */
	fprintf( stderr,"%s:%u: "  "RS_ASSERT(%s) failed.\n", file,line,cond );
#else
	fprintf( stderr,"%s:%u: %s: RS_ASSERT(%s) failed.\n", file,line,fn,cond );
#endif
	fflush( stderr );

	errno = save_errno;
	/* $NO_RS_ASSERT > 0: ignore rs_asserts, 0: abort, < 0: just warn */
	if ( !no_assert /* from $NO_RS_ASSERT */ ) abort();
}

/* SlapReply is consistent */
void
(rs_assert_ok)( const SlapReply *rs )
{
	const slap_mask_t flags = rs->sr_flags;

	if ( flags & REP_ENTRY_MASK ) {
		RS_ASSERT( !(flags & REP_ENTRY_MUSTRELEASE)
			|| !(flags & (REP_ENTRY_MASK ^ REP_ENTRY_MUSTRELEASE)) );
		RS_ASSERT( rs->sr_entry != NULL );
		RS_ASSERT( (1 << rs->sr_type) &
			((1 << REP_SEARCH) | (1 << REP_SEARCHREF) |
			 (1 << REP_RESULT) | (1 << REP_GLUE_RESULT)) );
	}
#if defined(USE_RS_ASSERT) && (USE_RS_ASSERT) > 1 /* TODO: Enable when safe */
	if ( (flags & (REP_MATCHED_MASK | REP_REF_MASK | REP_CTRLS_MASK)) ) {
		RS_ASSERT( !(flags & REP_MATCHED_MASK) || rs->sr_matched );
		RS_ASSERT( !(flags & REP_CTRLS_MASK  ) || rs->sr_ctrls   );
		/* Note: LDAP_REFERRAL + !sr_ref is OK, becomes LDAP_NO_SUCH_OBJECT */
	}
#if (USE_RS_ASSERT) > 2
	if ( rs->sr_err == LDAP_SUCCESS ) {
		RS_ASSERT( rs->sr_text == NULL );
		RS_ASSERT( rs->sr_matched == NULL );
	}
#endif
#endif
}

/* Ready for calling a new backend operation */
void
(rs_assert_ready)( const SlapReply *rs )
{
	RS_ASSERT( !rs->sr_entry   );
#if defined(USE_RS_ASSERT) && (USE_RS_ASSERT) > 1 /* TODO: Enable when safe */
	RS_ASSERT( !rs->sr_text    );
	RS_ASSERT( !rs->sr_ref     );
	RS_ASSERT( !rs->sr_matched );
	RS_ASSERT( !rs->sr_ctrls   );
	RS_ASSERT( !rs->sr_flags   );
#if (USE_RS_ASSERT) > 2
	RS_ASSERT( rs->sr_err == LDAP_SUCCESS );
#endif
#else
	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MASK) );
#endif
}

/* Backend operation done */
void
(rs_assert_done)( const SlapReply *rs )
{
#if defined(USE_RS_ASSERT) && (USE_RS_ASSERT) > 1 /* TODO: Enable when safe */
	RS_ASSERT( !(rs->sr_flags & ~(REP_ENTRY_MODIFIABLE|REP_NO_OPERATIONALS)) );
	rs_assert_ok( rs );
#else
	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MUSTFLUSH) );
#endif
}

#endif /* LDAP_TEST || USE_RS_ASSERT */

/* Reset a used SlapReply whose contents has been flushed (freed/released) */
void
(rs_reinit)( SlapReply *rs, slap_reply_t type )
{
	rs_reinit( rs, type );		/* proto-slap.h macro */
}

/* Obey and clear rs->sr_flags & REP_ENTRY_MASK.  Clear sr_entry if freed. */
void
rs_flush_entry( Operation *op, SlapReply *rs, slap_overinst *on )
{
	rs_assert_ok( rs );

	if ( (rs->sr_flags & REP_ENTRY_MUSTFLUSH) && rs->sr_entry != NULL ) {
		if ( !(rs->sr_flags & REP_ENTRY_MUSTRELEASE) ) {
			entry_free( rs->sr_entry );
		} else if ( on != NULL ) {
			overlay_entry_release_ov( op, rs->sr_entry, 0, on );
		} else {
			be_entry_release_rw( op, rs->sr_entry, 0 );
		}
		rs->sr_entry = NULL;
	}

	rs->sr_flags &= ~REP_ENTRY_MASK;
}

/* Set rs->sr_entry after obeying and clearing sr_flags & REP_ENTRY_MASK. */
void
rs_replace_entry( Operation *op, SlapReply *rs, slap_overinst *on, Entry *e )
{
	rs_flush_entry( op, rs, on );
	rs->sr_entry = e;
}

/*
 * Ensure rs->sr_entry is modifiable, by duplicating it if necessary.
 * Obey sr_flags.  Set REP_ENTRY_<MODIFIABLE, and MUSTBEFREED if duplicated>.
 * Return nonzero if rs->sr_entry was replaced.
 */
int
rs_entry2modifiable( Operation *op, SlapReply *rs, slap_overinst *on )
{
	if ( rs->sr_flags & REP_ENTRY_MODIFIABLE ) {
		rs_assert_ok( rs );
		return 0;
	}
	rs_replace_entry( op, rs, on, entry_dup( rs->sr_entry ));
	rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
	return 1;
}

static long send_ldap_ber(
	Operation *op,
	BerElement *ber )
{
	Connection *conn = op->o_conn;
	ber_len_t bytes;
	long ret = 0;
	char *close_reason;

	ber_get_option( ber, LBER_OPT_BER_BYTES_TO_WRITE, &bytes );

	/* write only one pdu at a time - wait til it's our turn */
	ldap_pvt_thread_mutex_lock( &conn->c_write1_mutex );
	if (( op->o_abandon && !op->o_cancel ) || !connection_valid( conn ) ||
		conn->c_writers < 0 ) {
		ldap_pvt_thread_mutex_unlock( &conn->c_write1_mutex );
		return 0;
	}

	conn->c_writers++;

	while ( conn->c_writers > 0 && conn->c_writing ) {
		ldap_pvt_thread_pool_idle( &connection_pool );
		ldap_pvt_thread_cond_wait( &conn->c_write1_cv, &conn->c_write1_mutex );
		ldap_pvt_thread_pool_unidle( &connection_pool );
	}

	/* connection was closed under us */
	if ( conn->c_writers < 0 ) {
		/* we're the last waiter, let the closer continue */
		if ( conn->c_writers == -1 )
			ldap_pvt_thread_cond_signal( &conn->c_write1_cv );
		conn->c_writers++;
		ldap_pvt_thread_mutex_unlock( &conn->c_write1_mutex );
		return 0;
	}

	/* Our turn */
	conn->c_writing = 1;

	/* write the pdu */
	while( 1 ) {
		int err;

		if ( ber_flush2( conn->c_sb, ber, LBER_FLUSH_FREE_NEVER ) == 0 ) {
			ret = bytes;
			break;
		}

		err = sock_errno();

		/*
		 * we got an error.  if it's ewouldblock, we need to
		 * wait on the socket being writable.  otherwise, figure
		 * it's a hard error and return.
		 */

		Debug( LDAP_DEBUG_CONNS, "ber_flush2 failed errno=%d reason=\"%s\"\n",
		    err, sock_errstr(err), 0 );

		if ( err != EWOULDBLOCK && err != EAGAIN ) {
			close_reason = "connection lost on write";
fail:
			conn->c_writers--;
			conn->c_writing = 0;
			ldap_pvt_thread_mutex_unlock( &conn->c_write1_mutex );
			ldap_pvt_thread_mutex_lock( &conn->c_mutex );
			connection_closing( conn, close_reason );
			ldap_pvt_thread_mutex_unlock( &conn->c_mutex );
			return -1;
		}

		/* wait for socket to be write-ready */
		conn->c_writewaiter = 1;
		ldap_pvt_thread_mutex_unlock( &conn->c_write1_mutex );
		ldap_pvt_thread_pool_idle( &connection_pool );
		err = slapd_wait_writer( conn->c_sd );
		conn->c_writewaiter = 0;
		ldap_pvt_thread_pool_unidle( &connection_pool );
		ldap_pvt_thread_mutex_lock( &conn->c_write1_mutex );
		/* 0 is timeout, so we close it.
		 * -1 is an error, close it.
		 */
		if ( err <= 0 ) {
			if ( err == 0 )
				close_reason = "writetimeout";
			else
				close_reason = "connection lost on writewait";
			goto fail;
		}

		if ( conn->c_writers < 0 ) {
			ret = 0;
			break;
		}
	}

	conn->c_writing = 0;
	if ( conn->c_writers < 0 ) {
		conn->c_writers++;
		if ( !conn->c_writers )
			ldap_pvt_thread_cond_signal( &conn->c_write1_cv );
	} else {
		conn->c_writers--;
		ldap_pvt_thread_cond_signal( &conn->c_write1_cv );
	}
	ldap_pvt_thread_mutex_unlock( &conn->c_write1_mutex );

	return ret;
}

static int
send_ldap_control( BerElement *ber, LDAPControl *c )
{
	int rc;

	assert( c != NULL );

	rc = ber_printf( ber, "{s" /*}*/, c->ldctl_oid );

	if( c->ldctl_iscritical ) {
		rc = ber_printf( ber, "b",
			(ber_int_t) c->ldctl_iscritical ) ;
		if( rc == -1 ) return rc;
	}

	if( c->ldctl_value.bv_val != NULL ) {
		rc = ber_printf( ber, "O", &c->ldctl_value ); 
		if( rc == -1 ) return rc;
	}

	rc = ber_printf( ber, /*{*/"N}" );
	if( rc == -1 ) return rc;

	return 0;
}

static int
send_ldap_controls( Operation *o, BerElement *ber, LDAPControl **c )
{
	int rc;

	if( c == NULL )
		return 0;

	rc = ber_printf( ber, "t{"/*}*/, LDAP_TAG_CONTROLS );
	if( rc == -1 ) return rc;

	for( ; *c != NULL; c++) {
		rc = send_ldap_control( ber, *c );
		if( rc == -1 ) return rc;
	}

#ifdef SLAP_CONTROL_X_SORTEDRESULTS
	/* this is a hack to avoid having to modify op->s_ctrls */
	if( o->o_sortedresults ) {
		BerElementBuffer berbuf;
		BerElement *sber = (BerElement *) &berbuf;
		LDAPControl sorted;
		BER_BVZERO( &sorted.ldctl_value );
		sorted.ldctl_oid = LDAP_CONTROL_SORTRESPONSE;
		sorted.ldctl_iscritical = 0;

		ber_init2( sber, NULL, LBER_USE_DER );

		ber_printf( sber, "{e}", LDAP_UNWILLING_TO_PERFORM );

		if( ber_flatten2( sber, &sorted.ldctl_value, 0 ) == -1 ) {
			return -1;
		}

		(void) ber_free_buf( sber );

		rc = send_ldap_control( ber, &sorted );
		if( rc == -1 ) return rc;
	}
#endif

	rc = ber_printf( ber, /*{*/"N}" );

	return rc;
}

/*
 * slap_response_play()
 *
 * plays the callback list; rationale: a callback can
 *   - remove itself from the list, by setting op->o_callback = NULL;
 *     malloc()'ed callbacks should free themselves from inside the
 *     sc_response() function.
 *   - replace itself with another (list of) callback(s), by setting
 *     op->o_callback = a new (list of) callback(s); in this case, it
 *     is the callback's responsibility to to append existing subsequent
 *     callbacks to the end of the list that is passed to the sc_response()
 *     function.
 *   - modify the list of subsequent callbacks by modifying the value
 *     of the sc_next field from inside the sc_response() function; this
 *     case does not require any handling from inside slap_response_play()
 *
 * To stop execution of the playlist, the sc_response() function must return
 * a value different from SLAP_SC_CONTINUE.
 *
 * The same applies to slap_cleanup_play(); only, there is no means to stop
 * execution of the playlist, since all cleanup functions must be called.
 */
static int
slap_response_play(
	Operation *op,
	SlapReply *rs )
{
	int rc;

	slap_callback	*sc = op->o_callback, **scp;

	rc = SLAP_CB_CONTINUE;
	for ( scp = &sc; *scp; ) {
		slap_callback *sc_next = (*scp)->sc_next, **sc_nextp = &(*scp)->sc_next;

		op->o_callback = *scp;
		if ( op->o_callback->sc_response ) {
			rc = op->o_callback->sc_response( op, rs );
			if ( op->o_callback == NULL ) {
				/* the callback has been removed;
				 * repair the list */
				*scp = sc_next;
				sc_nextp = scp;

			} else if ( op->o_callback != *scp ) {
				/* a new callback has been inserted
				 * in place of the existing one; repair the list */
				*scp = op->o_callback;
				sc_nextp = scp;
			}
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
		scp = sc_nextp;
	}

	op->o_callback = sc;
	return rc;
}

static int
slap_cleanup_play(
	Operation *op,
	SlapReply *rs )
{
	slap_callback	*sc = op->o_callback, **scp;

	for ( scp = &sc; *scp; ) {
		slap_callback *sc_next = (*scp)->sc_next, **sc_nextp = &(*scp)->sc_next;

		op->o_callback = *scp;
		if ( op->o_callback->sc_cleanup ) {
			(void)op->o_callback->sc_cleanup( op, rs );
			if ( op->o_callback == NULL ) {
				/* the callback has been removed;
				 * repair the list */
				*scp = sc_next;
				sc_nextp = scp;

			} else if ( op->o_callback != *scp ) {
				/* a new callback has been inserted
				 * after the existing one; repair the list */
				/* a new callback has been inserted
				 * in place of the existing one; repair the list */
				*scp = op->o_callback;
				sc_nextp = scp;
			}
			/* don't care about the result; do all cleanup */
		}
		scp = sc_nextp;
	}

	op->o_callback = sc;
	return LDAP_SUCCESS;
}

static int
send_ldap_response(
	Operation *op,
	SlapReply *rs )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *) &berbuf;
	int		rc = LDAP_SUCCESS;
	long	bytes;

	/* op was actually aborted, bypass everything if client didn't Cancel */
	if (( rs->sr_err == SLAPD_ABANDON ) && !op->o_cancel ) {
		rc = SLAPD_ABANDON;
		goto clean2;
	}

	if ( op->o_callback ) {
		rc = slap_response_play( op, rs );
		if ( rc != SLAP_CB_CONTINUE ) {
			goto clean2;
		}
	}

	/* op completed, connection aborted, bypass sending response */
	if ( op->o_abandon && !op->o_cancel ) {
		rc = SLAPD_ABANDON;
		goto clean2;
	}

#ifdef LDAP_CONNECTIONLESS
	if (op->o_conn && op->o_conn->c_is_udp)
		ber = op->o_res_ber;
	else
#endif
	{
		ber_init_w_nullc( ber, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );
	}

	rc = rs->sr_err;
	if ( rc == SLAPD_ABANDON && op->o_cancel )
		rc = LDAP_CANCELLED;

	Debug( LDAP_DEBUG_TRACE,
		"send_ldap_response: msgid=%d tag=%lu err=%d\n",
		rs->sr_msgid, rs->sr_tag, rc );

	if( rs->sr_ref ) {
		Debug( LDAP_DEBUG_ARGS, "send_ldap_response: ref=\"%s\"\n",
			rs->sr_ref[0].bv_val ? rs->sr_ref[0].bv_val : "NULL",
			NULL, NULL );
	}

#ifdef LDAP_CONNECTIONLESS
	if (op->o_conn && op->o_conn->c_is_udp &&
		op->o_protocol == LDAP_VERSION2 )
	{
		rc = ber_printf( ber, "t{ess" /*"}"*/,
			rs->sr_tag, rc,
		rs->sr_matched == NULL ? "" : rs->sr_matched,
		rs->sr_text == NULL ? "" : rs->sr_text );
	} else 
#endif
	if ( rs->sr_type == REP_INTERMEDIATE ) {
	    rc = ber_printf( ber, "{it{" /*"}}"*/,
			rs->sr_msgid, rs->sr_tag );

	} else {
	    rc = ber_printf( ber, "{it{ess" /*"}}"*/,
		rs->sr_msgid, rs->sr_tag, rc,
		rs->sr_matched == NULL ? "" : rs->sr_matched,
		rs->sr_text == NULL ? "" : rs->sr_text );
	}

	if( rc != -1 ) {
		if ( rs->sr_ref != NULL ) {
			assert( rs->sr_err == LDAP_REFERRAL );
			rc = ber_printf( ber, "t{W}",
				LDAP_TAG_REFERRAL, rs->sr_ref );
		} else {
			assert( rs->sr_err != LDAP_REFERRAL );
		}
	}

	if( rc != -1 && rs->sr_type == REP_SASL && rs->sr_sasldata != NULL ) {
		rc = ber_printf( ber, "tO",
			LDAP_TAG_SASL_RES_CREDS, rs->sr_sasldata );
	}

	if( rc != -1 &&
		( rs->sr_type == REP_EXTENDED || rs->sr_type == REP_INTERMEDIATE ))
	{
		if ( rs->sr_rspoid != NULL ) {
			rc = ber_printf( ber, "ts",
				rs->sr_type == REP_EXTENDED
					? LDAP_TAG_EXOP_RES_OID : LDAP_TAG_IM_RES_OID,
				rs->sr_rspoid );
		}
		if( rc != -1 && rs->sr_rspdata != NULL ) {
			rc = ber_printf( ber, "tO",
				rs->sr_type == REP_EXTENDED
					? LDAP_TAG_EXOP_RES_VALUE : LDAP_TAG_IM_RES_VALUE,
				rs->sr_rspdata );
		}
	}

	if( rc != -1 ) {
		rc = ber_printf( ber, /*"{"*/ "N}" );
	}

	if( rc != -1 ) {
		rc = send_ldap_controls( op, ber, rs->sr_ctrls );
	}

	if( rc != -1 ) {
		rc = ber_printf( ber, /*"{"*/ "N}" );
	}

#ifdef LDAP_CONNECTIONLESS
	if( op->o_conn && op->o_conn->c_is_udp && op->o_protocol == LDAP_VERSION2
		&& rc != -1 )
	{
		rc = ber_printf( ber, /*"{"*/ "N}" );
	}
#endif
		
	if ( rc == -1 ) {
		Debug( LDAP_DEBUG_ANY, "ber_printf failed\n", 0, 0, 0 );

#ifdef LDAP_CONNECTIONLESS
		if (!op->o_conn || op->o_conn->c_is_udp == 0)
#endif
		{
			ber_free_buf( ber );
		}
		goto cleanup;
	}

	/* send BER */
	bytes = send_ldap_ber( op, ber );
#ifdef LDAP_CONNECTIONLESS
	if (!op->o_conn || op->o_conn->c_is_udp == 0)
#endif
	{
		ber_free_buf( ber );
	}

	if ( bytes < 0 ) {
		Debug( LDAP_DEBUG_ANY,
			"send_ldap_response: ber write failed\n",
			0, 0, 0 );

		goto cleanup;
	}

	ldap_pvt_thread_mutex_lock( &op->o_counters->sc_mutex );
	ldap_pvt_mp_add_ulong( op->o_counters->sc_pdu, 1 );
	ldap_pvt_mp_add_ulong( op->o_counters->sc_bytes, (unsigned long)bytes );
	ldap_pvt_thread_mutex_unlock( &op->o_counters->sc_mutex );

cleanup:;
	/* Tell caller that we did this for real, as opposed to being
	 * overridden by a callback
	 */
	rc = SLAP_CB_CONTINUE;

clean2:;
	if ( op->o_callback ) {
		(void)slap_cleanup_play( op, rs );
	}

	if ( rs->sr_flags & REP_MATCHED_MUSTBEFREED ) {
		rs->sr_flags ^= REP_MATCHED_MUSTBEFREED; /* paranoia */
		if ( rs->sr_matched ) {
			free( (char *)rs->sr_matched );
			rs->sr_matched = NULL;
		}
	}

	if ( rs->sr_flags & REP_REF_MUSTBEFREED ) {
		rs->sr_flags ^= REP_REF_MUSTBEFREED; /* paranoia */
		if ( rs->sr_ref ) {
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;
		}
	}

	if ( rs->sr_flags & REP_CTRLS_MUSTBEFREED ) {
		rs->sr_flags ^= REP_CTRLS_MUSTBEFREED; /* paranoia */
		if ( rs->sr_ctrls ) {
			slap_free_ctrls( op, rs->sr_ctrls );
			rs->sr_ctrls = NULL;
		}
	}

	return rc;
}


void
send_ldap_disconnect( Operation	*op, SlapReply *rs )
{
#define LDAP_UNSOLICITED_ERROR(e) \
	(  (e) == LDAP_PROTOCOL_ERROR \
	|| (e) == LDAP_STRONG_AUTH_REQUIRED \
	|| (e) == LDAP_UNAVAILABLE )

	Debug( LDAP_DEBUG_TRACE,
		"send_ldap_disconnect %d:%s\n",
		rs->sr_err, rs->sr_text ? rs->sr_text : "", NULL );
	assert( LDAP_UNSOLICITED_ERROR( rs->sr_err ) );

	/* TODO: Flush the entry if sr_type == REP_SEARCH/REP_SEARCHREF? */
	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MASK) );
	rs->sr_flags &= ~REP_ENTRY_MASK;	/* paranoia */

	rs->sr_type = REP_EXTENDED;
	rs->sr_rspdata = NULL;

	if ( op->o_protocol < LDAP_VERSION3 ) {
		rs->sr_rspoid = NULL;
		rs->sr_tag = slap_req2res( op->o_tag );
		rs->sr_msgid = (rs->sr_tag != LBER_SEQUENCE) ? op->o_msgid : 0;

	} else {
		rs->sr_rspoid = LDAP_NOTICE_DISCONNECT;
		rs->sr_tag = LDAP_RES_EXTENDED;
		rs->sr_msgid = LDAP_RES_UNSOLICITED;
	}

	if ( send_ldap_response( op, rs ) == SLAP_CB_CONTINUE ) {
		Statslog( LDAP_DEBUG_STATS,
			"%s DISCONNECT tag=%lu err=%d text=%s\n",
			op->o_log_prefix, rs->sr_tag, rs->sr_err,
			rs->sr_text ? rs->sr_text : "", 0 );
	}
}

void
slap_send_ldap_result( Operation *op, SlapReply *rs )
{
	char *tmp = NULL;
	const char *otext = rs->sr_text;
	BerVarray oref = rs->sr_ref;

	rs->sr_type = REP_RESULT;

	/* Propagate Abandons so that cleanup callbacks can be processed */
	if ( rs->sr_err == SLAPD_ABANDON || op->o_abandon )
		goto abandon;

	Debug( LDAP_DEBUG_TRACE,
		"send_ldap_result: %s p=%d\n",
		op->o_log_prefix, op->o_protocol, 0 );
	Debug( LDAP_DEBUG_ARGS,
		"send_ldap_result: err=%d matched=\"%s\" text=\"%s\"\n",
		rs->sr_err, rs->sr_matched ? rs->sr_matched : "",
		rs->sr_text ? rs->sr_text : "" );
	if( rs->sr_ref ) {
		Debug( LDAP_DEBUG_ARGS,
			"send_ldap_result: referral=\"%s\"\n",
			rs->sr_ref[0].bv_val ? rs->sr_ref[0].bv_val : "NULL",
			NULL, NULL );
	}
	assert( !LDAP_API_ERROR( rs->sr_err ) );
	assert( rs->sr_err != LDAP_PARTIAL_RESULTS );

	if ( rs->sr_err == LDAP_REFERRAL ) {
		if( op->o_domain_scope ) rs->sr_ref = NULL;

		if( rs->sr_ref == NULL ) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else if ( op->o_protocol < LDAP_VERSION3 ) {
			rs->sr_err = LDAP_PARTIAL_RESULTS;
		}
	}

	if ( op->o_protocol < LDAP_VERSION3 ) {
		tmp = v2ref( rs->sr_ref, rs->sr_text );
		rs->sr_text = tmp;
		rs->sr_ref = NULL;
	}

abandon:
	rs->sr_tag = slap_req2res( op->o_tag );
	rs->sr_msgid = (rs->sr_tag != LBER_SEQUENCE) ? op->o_msgid : 0;

	if ( rs->sr_flags & REP_REF_MUSTBEFREED ) {
		if ( rs->sr_ref == NULL ) {
			rs->sr_flags ^= REP_REF_MUSTBEFREED;
			ber_bvarray_free( oref );
		}
		oref = NULL; /* send_ldap_response() will free rs->sr_ref if != NULL */
	}

	if ( send_ldap_response( op, rs ) == SLAP_CB_CONTINUE ) {
		if ( op->o_tag == LDAP_REQ_SEARCH ) {
			Statslog( LDAP_DEBUG_STATS,
				"%s SEARCH RESULT tag=%lu err=%d nentries=%d text=%s\n",
				op->o_log_prefix, rs->sr_tag, rs->sr_err,
				rs->sr_nentries, rs->sr_text ? rs->sr_text : "" );
		} else {
			Statslog( LDAP_DEBUG_STATS,
				"%s RESULT tag=%lu err=%d text=%s\n",
				op->o_log_prefix, rs->sr_tag, rs->sr_err,
				rs->sr_text ? rs->sr_text : "", 0 );
		}
	}

	if( tmp != NULL ) ch_free(tmp);
	rs->sr_text = otext;
	rs->sr_ref = oref;
}

void
send_ldap_sasl( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_TRACE, "send_ldap_sasl: err=%d len=%ld\n",
		rs->sr_err,
		rs->sr_sasldata ? (long) rs->sr_sasldata->bv_len : -1, NULL );

	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MASK) );
	rs->sr_flags &= ~REP_ENTRY_MASK;	/* paranoia */

	rs->sr_type = REP_SASL;
	rs->sr_tag = slap_req2res( op->o_tag );
	rs->sr_msgid = (rs->sr_tag != LBER_SEQUENCE) ? op->o_msgid : 0;

	if ( send_ldap_response( op, rs ) == SLAP_CB_CONTINUE ) {
		Statslog( LDAP_DEBUG_STATS,
			"%s RESULT tag=%lu err=%d text=%s\n",
			op->o_log_prefix, rs->sr_tag, rs->sr_err,
			rs->sr_text ? rs->sr_text : "", 0 );
	}
}

void
slap_send_ldap_extended( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_TRACE,
		"send_ldap_extended: err=%d oid=%s len=%ld\n",
		rs->sr_err,
		rs->sr_rspoid ? rs->sr_rspoid : "",
		rs->sr_rspdata != NULL ? rs->sr_rspdata->bv_len : 0 );

	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MASK) );
	rs->sr_flags &= ~REP_ENTRY_MASK;	/* paranoia */

	rs->sr_type = REP_EXTENDED;
	rs->sr_tag = slap_req2res( op->o_tag );
	rs->sr_msgid = (rs->sr_tag != LBER_SEQUENCE) ? op->o_msgid : 0;

	if ( send_ldap_response( op, rs ) == SLAP_CB_CONTINUE ) {
		Statslog( LDAP_DEBUG_STATS,
			"%s RESULT oid=%s err=%d text=%s\n",
			op->o_log_prefix, rs->sr_rspoid ? rs->sr_rspoid : "",
			rs->sr_err, rs->sr_text ? rs->sr_text : "", 0 );
	}
}

void
slap_send_ldap_intermediate( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_TRACE,
		"send_ldap_intermediate: err=%d oid=%s len=%ld\n",
		rs->sr_err,
		rs->sr_rspoid ? rs->sr_rspoid : "",
		rs->sr_rspdata != NULL ? rs->sr_rspdata->bv_len : 0 );

	RS_ASSERT( !(rs->sr_flags & REP_ENTRY_MASK) );
	rs->sr_flags &= ~REP_ENTRY_MASK;	/* paranoia */

	rs->sr_type = REP_INTERMEDIATE;
	rs->sr_tag = LDAP_RES_INTERMEDIATE;
	rs->sr_msgid = op->o_msgid;
	if ( send_ldap_response( op, rs ) == SLAP_CB_CONTINUE ) {
		Statslog( LDAP_DEBUG_STATS2,
			"%s INTERM oid=%s\n",
			op->o_log_prefix,
			rs->sr_rspoid ? rs->sr_rspoid : "", 0, 0, 0 );
	}
}

#define set_ldap_error( rs, err, text ) do { \
		(rs)->sr_err = err; (rs)->sr_text = text; } while(0)

/*
 * returns:
 *
 * LDAP_SUCCESS			entry sent
 * LDAP_OTHER			entry not sent (other)
 * LDAP_INSUFFICIENT_ACCESS	entry not sent (ACL)
 * LDAP_UNAVAILABLE		entry not sent (connection closed)
 * LDAP_SIZELIMIT_EXCEEDED	entry not sent (caller must send sizelimitExceeded)
 */

int
slap_send_search_entry( Operation *op, SlapReply *rs )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *) &berbuf;
	Attribute	*a;
	int		i, j, rc = LDAP_UNAVAILABLE, bytes;
	int		userattrs;
	AccessControlState acl_state = ACL_STATE_INIT;
	int			 attrsonly;
	AttributeDescription *ad_entry = slap_schema.si_ad_entry;

	/* a_flags: array of flags telling if the i-th element will be
	 *          returned or filtered out
	 * e_flags: array of a_flags
	 */
	char **e_flags = NULL;

	rs->sr_type = REP_SEARCH;

	if ( op->ors_slimit >= 0 && rs->sr_nentries >= op->ors_slimit ) {
		rc = LDAP_SIZELIMIT_EXCEEDED;
		goto error_return;
	}

	/* Every 64 entries, check for thread pool pause */
	if ( ( ( rs->sr_nentries & 0x3f ) == 0x3f ) &&
		ldap_pvt_thread_pool_pausing( &connection_pool ) > 0 )
	{
		rc = LDAP_BUSY;
		goto error_return;
	}

	/* eventually will loop through generated operational attribute types
	 * currently implemented types include:
	 *	entryDN, subschemaSubentry, and hasSubordinates */
	/* NOTE: moved before overlays callback circling because
	 * they may modify entry and other stuff in rs */
	/* check for special all operational attributes ("+") type */
	/* FIXME: maybe we could set this flag at the operation level;
	 * however, in principle the caller of send_search_entry() may
	 * change the attribute list at each call */
	rs->sr_attr_flags = slap_attr_flags( rs->sr_attrs );

	rc = backend_operational( op, rs );
	if ( rc ) {
		goto error_return;
	}

	if ( op->o_callback ) {
		rc = slap_response_play( op, rs );
		if ( rc != SLAP_CB_CONTINUE ) {
			goto error_return;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "=> send_search_entry: conn %lu dn=\"%s\"%s\n",
		op->o_connid, rs->sr_entry->e_name.bv_val,
		op->ors_attrsonly ? " (attrsOnly)" : "" );

	attrsonly = op->ors_attrsonly;

	if ( !access_allowed( op, rs->sr_entry, ad_entry, NULL, ACL_READ, NULL )) {
		Debug( LDAP_DEBUG_ACL,
			"send_search_entry: conn %lu access to entry (%s) not allowed\n", 
			op->o_connid, rs->sr_entry->e_name.bv_val, 0 );

		rc = LDAP_INSUFFICIENT_ACCESS;
		goto error_return;
	}

	if ( op->o_res_ber ) {
		/* read back control or LDAP_CONNECTIONLESS */
	    ber = op->o_res_ber;
	} else {
		struct berval	bv;

		bv.bv_len = entry_flatsize( rs->sr_entry, 0 );
		bv.bv_val = op->o_tmpalloc( bv.bv_len, op->o_tmpmemctx );

		ber_init2( ber, &bv, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );
	}

#ifdef LDAP_CONNECTIONLESS
	if ( op->o_conn && op->o_conn->c_is_udp ) {
		/* CONNECTIONLESS */
		if ( op->o_protocol == LDAP_VERSION2 ) {
	    	rc = ber_printf(ber, "t{O{" /*}}*/,
				LDAP_RES_SEARCH_ENTRY, &rs->sr_entry->e_name );
		} else {
	    	rc = ber_printf( ber, "{it{O{" /*}}}*/, op->o_msgid,
				LDAP_RES_SEARCH_ENTRY, &rs->sr_entry->e_name );
		}
	} else
#endif
	if ( op->o_res_ber ) {
		/* read back control */
	    rc = ber_printf( ber, "t{O{" /*}}*/,
			LDAP_RES_SEARCH_ENTRY, &rs->sr_entry->e_name );
	} else {
	    rc = ber_printf( ber, "{it{O{" /*}}}*/, op->o_msgid,
			LDAP_RES_SEARCH_ENTRY, &rs->sr_entry->e_name );
	}

	if ( rc == -1 ) {
		Debug( LDAP_DEBUG_ANY, 
			"send_search_entry: conn %lu  ber_printf failed\n", 
			op->o_connid, 0, 0 );

		if ( op->o_res_ber == NULL ) ber_free_buf( ber );
		set_ldap_error( rs, LDAP_OTHER, "encoding DN error" );
		rc = rs->sr_err;
		goto error_return;
	}

	/* check for special all user attributes ("*") type */
	userattrs = SLAP_USERATTRS( rs->sr_attr_flags );

	/* create an array of arrays of flags. Each flag corresponds
	 * to particular value of attribute and equals 1 if value matches
	 * to ValuesReturnFilter or 0 if not
	 */	
	if ( op->o_vrFilter != NULL ) {
		int	k = 0;
		size_t	size;

		for ( a = rs->sr_entry->e_attrs, i=0; a != NULL; a = a->a_next, i++ ) {
			for ( j = 0; a->a_vals[j].bv_val != NULL; j++ ) k++;
		}

		size = i * sizeof(char *) + k;
		if ( size > 0 ) {
			char	*a_flags;
			e_flags = slap_sl_calloc ( 1, i * sizeof(char *) + k, op->o_tmpmemctx );
			if( e_flags == NULL ) {
		    	Debug( LDAP_DEBUG_ANY, 
					"send_search_entry: conn %lu slap_sl_calloc failed\n",
					op->o_connid, 0, 0 );
				ber_free( ber, 1 );
	
				set_ldap_error( rs, LDAP_OTHER, "out of memory" );
				goto error_return;
			}
			a_flags = (char *)(e_flags + i);
			memset( a_flags, 0, k );
			for ( a=rs->sr_entry->e_attrs, i=0; a != NULL; a=a->a_next, i++ ) {
				for ( j = 0; a->a_vals[j].bv_val != NULL; j++ );
				e_flags[i] = a_flags;
				a_flags += j;
			}
	
			rc = filter_matched_values(op, rs->sr_entry->e_attrs, &e_flags) ; 
			if ( rc == -1 ) {
			    	Debug( LDAP_DEBUG_ANY, "send_search_entry: "
					"conn %lu matched values filtering failed\n",
					op->o_connid, 0, 0 );
				if ( op->o_res_ber == NULL ) ber_free_buf( ber );
				set_ldap_error( rs, LDAP_OTHER,
					"matched values filtering error" );
				rc = rs->sr_err;
				goto error_return;
			}
		}
	}

	for ( a = rs->sr_entry->e_attrs, j = 0; a != NULL; a = a->a_next, j++ ) {
		AttributeDescription *desc = a->a_desc;
		int finish = 0;

		if ( rs->sr_attrs == NULL ) {
			/* all user attrs request, skip operational attributes */
			if( is_at_operational( desc->ad_type ) ) {
				continue;
			}

		} else {
			/* specific attrs requested */
			if ( is_at_operational( desc->ad_type ) ) {
				/* if not explicitly requested */
				if ( !ad_inlist( desc, rs->sr_attrs )) {
					/* if not all op attrs requested, skip */
					if ( !SLAP_OPATTRS( rs->sr_attr_flags ))
						continue;
					/* if DSA-specific and replicating, skip */
					if ( op->o_sync != SLAP_CONTROL_NONE &&
						desc->ad_type->sat_usage == LDAP_SCHEMA_DSA_OPERATION )
						continue;
				}
			} else {
				if ( !userattrs && !ad_inlist( desc, rs->sr_attrs ) ) {
					continue;
				}
			}
		}

		if ( attrsonly ) {
			if ( ! access_allowed( op, rs->sr_entry, desc, NULL,
				ACL_READ, &acl_state ) )
			{
				Debug( LDAP_DEBUG_ACL, "send_search_entry: "
					"conn %lu access to attribute %s not allowed\n",
				        op->o_connid, desc->ad_cname.bv_val, 0 );
				continue;
			}

			if (( rc = ber_printf( ber, "{O[" /*]}*/ , &desc->ad_cname )) == -1 ) {
				Debug( LDAP_DEBUG_ANY, 
					"send_search_entry: conn %lu  ber_printf failed\n", 
					op->o_connid, 0, 0 );

				if ( op->o_res_ber == NULL ) ber_free_buf( ber );
				set_ldap_error( rs, LDAP_OTHER,
					"encoding description error");
				rc = rs->sr_err;
				goto error_return;
			}
			finish = 1;

		} else {
			int first = 1;
			for ( i = 0; a->a_nvals[i].bv_val != NULL; i++ ) {
				if ( ! access_allowed( op, rs->sr_entry,
					desc, &a->a_nvals[i], ACL_READ, &acl_state ) )
				{
					Debug( LDAP_DEBUG_ACL,
						"send_search_entry: conn %lu "
						"access to attribute %s, value #%d not allowed\n",
						op->o_connid, desc->ad_cname.bv_val, i );

					continue;
				}

				if ( op->o_vrFilter && e_flags[j][i] == 0 ){
					continue;
				}

				if ( first ) {
					first = 0;
					finish = 1;
					if (( rc = ber_printf( ber, "{O[" /*]}*/ , &desc->ad_cname )) == -1 ) {
						Debug( LDAP_DEBUG_ANY,
							"send_search_entry: conn %lu  ber_printf failed\n", 
							op->o_connid, 0, 0 );

						if ( op->o_res_ber == NULL ) ber_free_buf( ber );
						set_ldap_error( rs, LDAP_OTHER,
							"encoding description error");
						rc = rs->sr_err;
						goto error_return;
					}
				}
				if (( rc = ber_printf( ber, "O", &a->a_vals[i] )) == -1 ) {
					Debug( LDAP_DEBUG_ANY,
						"send_search_entry: conn %lu  "
						"ber_printf failed.\n", op->o_connid, 0, 0 );

					if ( op->o_res_ber == NULL ) ber_free_buf( ber );
					set_ldap_error( rs, LDAP_OTHER,
						"encoding values error" );
					rc = rs->sr_err;
					goto error_return;
				}
			}
		}

		if ( finish && ( rc = ber_printf( ber, /*{[*/ "]N}" )) == -1 ) {
			Debug( LDAP_DEBUG_ANY,
				"send_search_entry: conn %lu ber_printf failed\n", 
				op->o_connid, 0, 0 );

			if ( op->o_res_ber == NULL ) ber_free_buf( ber );
			set_ldap_error( rs, LDAP_OTHER, "encode end error" );
			rc = rs->sr_err;
			goto error_return;
		}
	}

	/* NOTE: moved before overlays callback circling because
	 * they may modify entry and other stuff in rs */
	if ( rs->sr_operational_attrs != NULL && op->o_vrFilter != NULL ) {
		int	k = 0;
		size_t	size;

		for ( a = rs->sr_operational_attrs, i=0; a != NULL; a = a->a_next, i++ ) {
			for ( j = 0; a->a_vals[j].bv_val != NULL; j++ ) k++;
		}

		size = i * sizeof(char *) + k;
		if ( size > 0 ) {
			char	*a_flags, **tmp;
		
			/*
			 * Reuse previous memory - we likely need less space
			 * for operational attributes
			 */
			tmp = slap_sl_realloc( e_flags, i * sizeof(char *) + k,
				op->o_tmpmemctx );
			if ( tmp == NULL ) {
			    	Debug( LDAP_DEBUG_ANY,
					"send_search_entry: conn %lu "
					"not enough memory "
					"for matched values filtering\n",
					op->o_connid, 0, 0 );
				if ( op->o_res_ber == NULL ) ber_free_buf( ber );
				set_ldap_error( rs, LDAP_OTHER,
					"not enough memory for matched values filtering" );
				goto error_return;
			}
			e_flags = tmp;
			a_flags = (char *)(e_flags + i);
			memset( a_flags, 0, k );
			for ( a = rs->sr_operational_attrs, i=0; a != NULL; a = a->a_next, i++ ) {
				for ( j = 0; a->a_vals[j].bv_val != NULL; j++ );
				e_flags[i] = a_flags;
				a_flags += j;
			}
			rc = filter_matched_values(op, rs->sr_operational_attrs, &e_flags) ; 
		    
			if ( rc == -1 ) {
			    	Debug( LDAP_DEBUG_ANY,
					"send_search_entry: conn %lu "
					"matched values filtering failed\n", 
					op->o_connid, 0, 0);
				if ( op->o_res_ber == NULL ) ber_free_buf( ber );
				set_ldap_error( rs, LDAP_OTHER,
					"matched values filtering error" );
				rc = rs->sr_err;
				goto error_return;
			}
		}
	}

	for (a = rs->sr_operational_attrs, j=0; a != NULL; a = a->a_next, j++ ) {
		AttributeDescription *desc = a->a_desc;

		if ( rs->sr_attrs == NULL ) {
			/* all user attrs request, skip operational attributes */
			if( is_at_operational( desc->ad_type ) ) {
				continue;
			}

		} else {
			/* specific attrs requested */
			if( is_at_operational( desc->ad_type ) ) {
				if ( !SLAP_OPATTRS( rs->sr_attr_flags ) && 
					!ad_inlist( desc, rs->sr_attrs ) )
				{
					continue;
				}
				/* if DSA-specific and replicating, skip */
				if ( op->o_sync != SLAP_CONTROL_NONE &&
					desc->ad_type->sat_usage == LDAP_SCHEMA_DSA_OPERATION )
					continue;
			} else {
				if ( !userattrs && !ad_inlist( desc, rs->sr_attrs ) ) {
					continue;
				}
			}
		}

		if ( ! access_allowed( op, rs->sr_entry, desc, NULL,
			ACL_READ, &acl_state ) )
		{
			Debug( LDAP_DEBUG_ACL,
				"send_search_entry: conn %lu "
				"access to attribute %s not allowed\n",
				op->o_connid, desc->ad_cname.bv_val, 0 );

			continue;
		}

		rc = ber_printf( ber, "{O[" /*]}*/ , &desc->ad_cname );
		if ( rc == -1 ) {
			Debug( LDAP_DEBUG_ANY,
				"send_search_entry: conn %lu  "
				"ber_printf failed\n", op->o_connid, 0, 0 );

			if ( op->o_res_ber == NULL ) ber_free_buf( ber );
			set_ldap_error( rs, LDAP_OTHER,
				"encoding description error" );
			rc = rs->sr_err;
			goto error_return;
		}

		if ( ! attrsonly ) {
			for ( i = 0; a->a_vals[i].bv_val != NULL; i++ ) {
				if ( ! access_allowed( op, rs->sr_entry,
					desc, &a->a_vals[i], ACL_READ, &acl_state ) )
				{
					Debug( LDAP_DEBUG_ACL,
						"send_search_entry: conn %lu "
						"access to %s, value %d not allowed\n",
						op->o_connid, desc->ad_cname.bv_val, i );

					continue;
				}

				if ( op->o_vrFilter && e_flags[j][i] == 0 ){
					continue;
				}

				if (( rc = ber_printf( ber, "O", &a->a_vals[i] )) == -1 ) {
					Debug( LDAP_DEBUG_ANY,
						"send_search_entry: conn %lu  ber_printf failed\n", 
						op->o_connid, 0, 0 );

					if ( op->o_res_ber == NULL ) ber_free_buf( ber );
					set_ldap_error( rs, LDAP_OTHER,
						"encoding values error" );
					rc = rs->sr_err;
					goto error_return;
				}
			}
		}

		if (( rc = ber_printf( ber, /*{[*/ "]N}" )) == -1 ) {
			Debug( LDAP_DEBUG_ANY,
				"send_search_entry: conn %lu  ber_printf failed\n",
				op->o_connid, 0, 0 );

			if ( op->o_res_ber == NULL ) ber_free_buf( ber );
			set_ldap_error( rs, LDAP_OTHER, "encode end error" );
			rc = rs->sr_err;
			goto error_return;
		}
	}

	/* free e_flags */
	if ( e_flags ) {
		slap_sl_free( e_flags, op->o_tmpmemctx );
		e_flags = NULL;
	}

	rc = ber_printf( ber, /*{{*/ "}N}" );

	if( rc != -1 ) {
		rc = send_ldap_controls( op, ber, rs->sr_ctrls );
	}

	if( rc != -1 ) {
#ifdef LDAP_CONNECTIONLESS
		if( op->o_conn && op->o_conn->c_is_udp ) {
			if ( op->o_protocol != LDAP_VERSION2 ) {
				rc = ber_printf( ber, /*{*/ "N}" );
			}
		} else
#endif
		if ( op->o_res_ber == NULL ) {
			rc = ber_printf( ber, /*{*/ "N}" );
		}
	}

	if ( rc == -1 ) {
		Debug( LDAP_DEBUG_ANY, "ber_printf failed\n", 0, 0, 0 );

		if ( op->o_res_ber == NULL ) ber_free_buf( ber );
		set_ldap_error( rs, LDAP_OTHER, "encode entry end error" );
		rc = rs->sr_err;
		goto error_return;
	}

	Statslog( LDAP_DEBUG_STATS2, "%s ENTRY dn=\"%s\"\n",
	    op->o_log_prefix, rs->sr_entry->e_nname.bv_val, 0, 0, 0 );

	rs_flush_entry( op, rs, NULL );

	if ( op->o_res_ber == NULL ) {
		bytes = send_ldap_ber( op, ber );
		ber_free_buf( ber );

		if ( bytes < 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"send_search_entry: conn %lu  ber write failed.\n", 
				op->o_connid, 0, 0 );

			rc = LDAP_UNAVAILABLE;
			goto error_return;
		}
		rs->sr_nentries++;

		ldap_pvt_thread_mutex_lock( &op->o_counters->sc_mutex );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_bytes, (unsigned long)bytes );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_entries, 1 );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_pdu, 1 );
		ldap_pvt_thread_mutex_unlock( &op->o_counters->sc_mutex );
	}

	Debug( LDAP_DEBUG_TRACE,
		"<= send_search_entry: conn %lu exit.\n", op->o_connid, 0, 0 );

	rc = LDAP_SUCCESS;

error_return:;
	if ( op->o_callback ) {
		(void)slap_cleanup_play( op, rs );
	}

	if ( e_flags ) {
		slap_sl_free( e_flags, op->o_tmpmemctx );
	}

	/* FIXME: Can break if rs now contains an extended response */
	if ( rs->sr_operational_attrs ) {
		attrs_free( rs->sr_operational_attrs );
		rs->sr_operational_attrs = NULL;
	}
	rs->sr_attr_flags = SLAP_ATTRS_UNDEFINED;

	if ( op->o_tag == LDAP_REQ_SEARCH && rs->sr_type == REP_SEARCH ) {
		rs_flush_entry( op, rs, NULL );
	} else {
		RS_ASSERT( (rs->sr_flags & REP_ENTRY_MASK) == 0 );
	}

	if ( rs->sr_flags & REP_CTRLS_MUSTBEFREED ) {
		rs->sr_flags ^= REP_CTRLS_MUSTBEFREED; /* paranoia */
		if ( rs->sr_ctrls ) {
			slap_free_ctrls( op, rs->sr_ctrls );
			rs->sr_ctrls = NULL;
		}
	}

	return( rc );
}

int
slap_send_search_reference( Operation *op, SlapReply *rs )
{
	BerElementBuffer berbuf;
	BerElement	*ber = (BerElement *) &berbuf;
	int rc = 0;
	int bytes;
	char *edn = rs->sr_entry ? rs->sr_entry->e_name.bv_val : "(null)";

	AttributeDescription *ad_ref = slap_schema.si_ad_ref;
	AttributeDescription *ad_entry = slap_schema.si_ad_entry;

	rs->sr_type = REP_SEARCHREF;
	if ( op->o_callback ) {
		rc = slap_response_play( op, rs );
		if ( rc != SLAP_CB_CONTINUE ) {
			goto rel;
		}
	}

	Debug( LDAP_DEBUG_TRACE,
		"=> send_search_reference: dn=\"%s\"\n",
		edn, 0, 0 );

	if (  rs->sr_entry && ! access_allowed( op, rs->sr_entry,
		ad_entry, NULL, ACL_READ, NULL ) )
	{
		Debug( LDAP_DEBUG_ACL,
			"send_search_reference: access to entry not allowed\n",
		    0, 0, 0 );
		rc = 1;
		goto rel;
	}

	if ( rs->sr_entry && ! access_allowed( op, rs->sr_entry,
		ad_ref, NULL, ACL_READ, NULL ) )
	{
		Debug( LDAP_DEBUG_ACL,
			"send_search_reference: access "
			"to reference not allowed\n",
		    0, 0, 0 );
		rc = 1;
		goto rel;
	}

	if( op->o_domain_scope ) {
		Debug( LDAP_DEBUG_ANY,
			"send_search_reference: domainScope control in (%s)\n", 
			edn, 0, 0 );
		rc = 0;
		goto rel;
	}

	if( rs->sr_ref == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"send_search_reference: null ref in (%s)\n", 
			edn, 0, 0 );
		rc = 1;
		goto rel;
	}

	if( op->o_protocol < LDAP_VERSION3 ) {
		rc = 0;
		/* save the references for the result */
		if( rs->sr_ref[0].bv_val != NULL ) {
			if( value_add( &rs->sr_v2ref, rs->sr_ref ) )
				rc = LDAP_OTHER;
		}
		goto rel;
	}

#ifdef LDAP_CONNECTIONLESS
	if( op->o_conn && op->o_conn->c_is_udp ) {
		ber = op->o_res_ber;
	} else
#endif
	{
		ber_init_w_nullc( ber, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );
	}

	rc = ber_printf( ber, "{it{W}" /*"}"*/ , op->o_msgid,
		LDAP_RES_SEARCH_REFERENCE, rs->sr_ref );

	if( rc != -1 ) {
		rc = send_ldap_controls( op, ber, rs->sr_ctrls );
	}

	if( rc != -1 ) {
		rc = ber_printf( ber, /*"{"*/ "N}" );
	}

	if ( rc == -1 ) {
		Debug( LDAP_DEBUG_ANY,
			"send_search_reference: ber_printf failed\n", 0, 0, 0 );

#ifdef LDAP_CONNECTIONLESS
		if (!op->o_conn || op->o_conn->c_is_udp == 0)
#endif
		ber_free_buf( ber );
		set_ldap_error( rs, LDAP_OTHER, "encode DN error" );
		goto rel;
	}

	rc = 0;
	rs_flush_entry( op, rs, NULL );

#ifdef LDAP_CONNECTIONLESS
	if (!op->o_conn || op->o_conn->c_is_udp == 0) {
#endif
	bytes = send_ldap_ber( op, ber );
	ber_free_buf( ber );

	if ( bytes < 0 ) {
		rc = LDAP_UNAVAILABLE;
	} else {
		ldap_pvt_thread_mutex_lock( &op->o_counters->sc_mutex );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_bytes, (unsigned long)bytes );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_refs, 1 );
		ldap_pvt_mp_add_ulong( op->o_counters->sc_pdu, 1 );
		ldap_pvt_thread_mutex_unlock( &op->o_counters->sc_mutex );
	}
#ifdef LDAP_CONNECTIONLESS
	}
#endif
	if ( rs->sr_ref != NULL ) {
		int	r;

		for ( r = 0; !BER_BVISNULL( &rs->sr_ref[ r ] ); r++ ) {
			Statslog( LDAP_DEBUG_STATS2, "%s REF #%d \"%s\"\n",
				op->o_log_prefix, r, rs->sr_ref[0].bv_val,
				0, 0 );
		}

	} else {
		Statslog( LDAP_DEBUG_STATS2, "%s REF \"(null)\"\n",
			op->o_log_prefix, 0, 0, 0, 0 );
	}

	Debug( LDAP_DEBUG_TRACE, "<= send_search_reference\n", 0, 0, 0 );

	if ( 0 ) {
rel:
	    rs_flush_entry( op, rs, NULL );
	}

	if ( op->o_callback ) {
		(void)slap_cleanup_play( op, rs );
	}

	if ( rs->sr_flags & REP_CTRLS_MUSTBEFREED ) {
		rs->sr_flags ^= REP_CTRLS_MUSTBEFREED; /* paranoia */
		if ( rs->sr_ctrls ) {
			slap_free_ctrls( op, rs->sr_ctrls );
			rs->sr_ctrls = NULL;
		}
	}

	return rc;
}

int
str2result(
    char	*s,
    int		*code,
    char	**matched,
    char	**info )
{
	int	rc;
	char	*c;

	*code = LDAP_SUCCESS;
	*matched = NULL;
	*info = NULL;

	if ( strncasecmp( s, "RESULT", STRLENOF( "RESULT" ) ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "str2result (%s) expecting \"RESULT\"\n",
		    s, 0, 0 );

		return( -1 );
	}

	rc = 0;
	while ( (s = strchr( s, '\n' )) != NULL ) {
		*s++ = '\0';
		if ( *s == '\0' ) {
			break;
		}
		if ( (c = strchr( s, ':' )) != NULL ) {
			c++;
		}

		if ( strncasecmp( s, "code", STRLENOF( "code" ) ) == 0 ) {
			char	*next = NULL;
			long	retcode;

			if ( c == NULL ) {
				Debug( LDAP_DEBUG_ANY, "str2result (%s) missing value\n",
				    s, 0, 0 );
				rc = -1;
				continue;
			}

			while ( isspace( (unsigned char) c[ 0 ] ) ) c++;
			if ( c[ 0 ] == '\0' ) {
				Debug( LDAP_DEBUG_ANY, "str2result (%s) missing or empty value\n",
				    s, 0, 0 );
				rc = -1;
				continue;
			}

			retcode = strtol( c, &next, 10 );
			if ( next == NULL || next == c ) {
				Debug( LDAP_DEBUG_ANY, "str2result (%s) unable to parse value\n",
				    s, 0, 0 );
				rc = -1;
				continue;
			}

			while ( isspace( (unsigned char) next[ 0 ] ) && next[ 0 ] != '\n' )
				next++;
			if ( next[ 0 ] != '\0' && next[ 0 ] != '\n' ) {
				Debug( LDAP_DEBUG_ANY, "str2result (%s) extra cruft after value\n",
				    s, 0, 0 );
				rc = -1;
				continue;
			}

			/* FIXME: what if it's larger than max int? */
			*code = (int)retcode;

		} else if ( strncasecmp( s, "matched", STRLENOF( "matched" ) ) == 0 ) {
			if ( c != NULL ) {
				*matched = c;
			}
		} else if ( strncasecmp( s, "info", STRLENOF( "info" ) ) == 0 ) {
			if ( c != NULL ) {
				*info = c;
			}
		} else {
			Debug( LDAP_DEBUG_ANY, "str2result (%s) unknown\n",
			    s, 0, 0 );

			rc = -1;
		}
	}

	return( rc );
}

int slap_read_controls(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	const struct berval *oid,
	LDAPControl **ctrl )
{
	int rc;
	struct berval bv;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *) &berbuf;
	LDAPControl c;
	Operation myop;

	Debug( LDAP_DEBUG_ANY, "%s slap_read_controls: (%s) %s\n",
		op->o_log_prefix, oid->bv_val, e->e_dn );

	rs->sr_entry = e;
	rs->sr_attrs = ( oid == &slap_pre_read_bv ) ?
		op->o_preread_attrs : op->o_postread_attrs; 

	bv.bv_len = entry_flatsize( rs->sr_entry, 0 );
	bv.bv_val = op->o_tmpalloc( bv.bv_len, op->o_tmpmemctx );

	ber_init2( ber, &bv, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	/* create new operation */
	myop = *op;
	/* FIXME: o_bd needed for ACL */
	myop.o_bd = op->o_bd;
	myop.o_res_ber = ber;
	myop.o_callback = NULL;
	myop.ors_slimit = 1;
	myop.ors_attrsonly = 0;

	rc = slap_send_search_entry( &myop, rs );
	if( rc ) return rc;

	rc = ber_flatten2( ber, &c.ldctl_value, 0 );

	if( rc == -1 ) return LDAP_OTHER;

	c.ldctl_oid = oid->bv_val;
	c.ldctl_iscritical = 0;

	if ( *ctrl == NULL ) {
		/* first try */
		*ctrl = (LDAPControl *) slap_sl_calloc( 1, sizeof(LDAPControl), NULL );
	} else {
		/* retry: free previous try */
		slap_sl_free( (*ctrl)->ldctl_value.bv_val, op->o_tmpmemctx );
	}

	**ctrl = c;
	return LDAP_SUCCESS;
}

/* Map API errors to protocol errors... */
int
slap_map_api2result( SlapReply *rs )
{
	switch(rs->sr_err) {
	case LDAP_SERVER_DOWN:
		return LDAP_UNAVAILABLE;
	case LDAP_LOCAL_ERROR:
		return LDAP_OTHER;
	case LDAP_ENCODING_ERROR:
	case LDAP_DECODING_ERROR:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_TIMEOUT:
		return LDAP_UNAVAILABLE;
	case LDAP_AUTH_UNKNOWN:
		return LDAP_AUTH_METHOD_NOT_SUPPORTED;
	case LDAP_FILTER_ERROR:
		rs->sr_text = "Filter error";
		return LDAP_OTHER;
	case LDAP_USER_CANCELLED:
		rs->sr_text = "User cancelled";
		return LDAP_OTHER;
	case LDAP_PARAM_ERROR:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_NO_MEMORY:
		return LDAP_OTHER;
	case LDAP_CONNECT_ERROR:
		return LDAP_UNAVAILABLE;
	case LDAP_NOT_SUPPORTED:
		return LDAP_UNWILLING_TO_PERFORM;
	case LDAP_CONTROL_NOT_FOUND:
		return LDAP_PROTOCOL_ERROR;
	case LDAP_NO_RESULTS_RETURNED:
		return LDAP_NO_SUCH_OBJECT;
	case LDAP_MORE_RESULTS_TO_RETURN:
		rs->sr_text = "More results to return";
		return LDAP_OTHER;
	case LDAP_CLIENT_LOOP:
	case LDAP_REFERRAL_LIMIT_EXCEEDED:
		return LDAP_LOOP_DETECT;
	default:
		if ( LDAP_API_ERROR(rs->sr_err) ) return LDAP_OTHER;
		return rs->sr_err;
	}
}


slap_mask_t
slap_attr_flags( AttributeName *an )
{
	slap_mask_t	flags = SLAP_ATTRS_UNDEFINED;

	if ( an == NULL ) {
		flags |= ( SLAP_OPATTRS_NO | SLAP_USERATTRS_YES );

	} else {
		flags |= an_find( an, slap_bv_all_operational_attrs )
			? SLAP_OPATTRS_YES : SLAP_OPATTRS_NO;
		flags |= an_find( an, slap_bv_all_user_attrs )
			? SLAP_USERATTRS_YES : SLAP_USERATTRS_NO;
	}

	return flags;
}
