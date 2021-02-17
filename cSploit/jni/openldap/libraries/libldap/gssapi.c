/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Author: Stefan Metzmacher <metze@sernet.de>
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

#include <ac/socket.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/errno.h>
#include <ac/ctype.h>
#include <ac/unistd.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "ldap-int.h"

#ifdef HAVE_GSSAPI

#ifdef HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#else
#include <gssapi.h>
#endif

static char *
gsserrstr(
	char *buf,
	ber_len_t buf_len,
	gss_OID mech,
	int gss_rc,
	OM_uint32 minor_status )
{
	OM_uint32 min2;
	gss_buffer_desc mech_msg = GSS_C_EMPTY_BUFFER;
	gss_buffer_desc gss_msg = GSS_C_EMPTY_BUFFER;
	gss_buffer_desc minor_msg = GSS_C_EMPTY_BUFFER;
	OM_uint32 msg_ctx = 0;

	if (buf == NULL) {
		return NULL;
	}

	if (buf_len == 0) {
		return NULL;
	}

#ifdef HAVE_GSS_OID_TO_STR
	gss_oid_to_str(&min2, mech, &mech_msg);
#endif
	gss_display_status(&min2, gss_rc, GSS_C_GSS_CODE,
			   mech, &msg_ctx, &gss_msg);
	gss_display_status(&min2, minor_status, GSS_C_MECH_CODE,
			   mech, &msg_ctx, &minor_msg);

	snprintf(buf, buf_len, "gss_rc[%d:%*s] mech[%*s] minor[%u:%*s]",
		 gss_rc, (int)gss_msg.length,
		 (const char *)(gss_msg.value?gss_msg.value:""),
		 (int)mech_msg.length,
		 (const char *)(mech_msg.value?mech_msg.value:""),
		 minor_status, (int)minor_msg.length,
		 (const char *)(minor_msg.value?minor_msg.value:""));

	gss_release_buffer(&min2, &mech_msg);
	gss_release_buffer(&min2, &gss_msg);
	gss_release_buffer(&min2, &minor_msg);

	buf[buf_len-1] = '\0';

	return buf;
}

static void
sb_sasl_gssapi_init(
	struct sb_sasl_generic_data *p,
	ber_len_t *min_send,
	ber_len_t *max_send,
	ber_len_t *max_recv )
{
	gss_ctx_id_t gss_ctx = (gss_ctx_id_t)p->ops_private;
	int gss_rc;
	OM_uint32 minor_status;
	gss_OID ctx_mech = GSS_C_NO_OID;
	OM_uint32 ctx_flags = 0;
	int conf_req_flag = 0;
	OM_uint32 max_input_size;

	gss_inquire_context(&minor_status,
			    gss_ctx,
			    NULL,
			    NULL,
			    NULL,
			    &ctx_mech,
			    &ctx_flags,
			    NULL,
			    NULL);

	if (ctx_flags & (GSS_C_CONF_FLAG)) {
		conf_req_flag = 1;
	}

#if defined(HAVE_CYRUS_SASL)
#define SEND_PREALLOC_SIZE	SASL_MIN_BUFF_SIZE
#else
#define SEND_PREALLOC_SIZE      4096
#endif
#define SEND_MAX_WIRE_SIZE	0x00A00000
#define RECV_MAX_WIRE_SIZE	0x0FFFFFFF
#define FALLBACK_SEND_MAX_SIZE	0x009FFFB8 /* from MIT 1.5.x */

	gss_rc = gss_wrap_size_limit(&minor_status, gss_ctx,
				     conf_req_flag, GSS_C_QOP_DEFAULT,
				     SEND_MAX_WIRE_SIZE, &max_input_size);
	if ( gss_rc != GSS_S_COMPLETE ) {
		char msg[256];
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_init: failed to wrap size limit: %s\n",
				gsserrstr( msg, sizeof(msg), ctx_mech, gss_rc, minor_status ) );
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_init: fallback to default wrap size limit\n");
		/*
		 * some libgssglue/libgssapi versions
		 * have a broken gss_wrap_size_limit()
		 * implementation
		 */
		max_input_size = FALLBACK_SEND_MAX_SIZE;
	}

	*min_send = SEND_PREALLOC_SIZE;
	*max_send = max_input_size;
	*max_recv = RECV_MAX_WIRE_SIZE;
}

static ber_int_t
sb_sasl_gssapi_encode(
	struct sb_sasl_generic_data *p,
	unsigned char *buf,
	ber_len_t len,
	Sockbuf_Buf *dst )
{
	gss_ctx_id_t gss_ctx = (gss_ctx_id_t)p->ops_private;
	int gss_rc;
	OM_uint32 minor_status;
	gss_buffer_desc unwrapped, wrapped;
	gss_OID ctx_mech = GSS_C_NO_OID;
	OM_uint32 ctx_flags = 0;
	int conf_req_flag = 0;
	int conf_state;
	unsigned char *b;
	ber_len_t pkt_len;

	unwrapped.value		= buf;
	unwrapped.length	= len;

	gss_inquire_context(&minor_status,
			    gss_ctx,
			    NULL,
			    NULL,
			    NULL,
			    &ctx_mech,
			    &ctx_flags,
			    NULL,
			    NULL);

	if (ctx_flags & (GSS_C_CONF_FLAG)) {
		conf_req_flag = 1;
	}

	gss_rc = gss_wrap(&minor_status, gss_ctx,
			  conf_req_flag, GSS_C_QOP_DEFAULT,
			  &unwrapped, &conf_state,
			  &wrapped);
	if ( gss_rc != GSS_S_COMPLETE ) {
		char msg[256];
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_encode: failed to encode packet: %s\n",
				gsserrstr( msg, sizeof(msg), ctx_mech, gss_rc, minor_status ) );
		return -1;
	}

	if ( conf_req_flag && conf_state == 0 ) {
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_encode: GSS_C_CONF_FLAG was ignored by our gss_wrap()\n" );
		return -1;
	}

	pkt_len = 4 + wrapped.length;

	/* Grow the packet buffer if neccessary */
	if ( dst->buf_size < pkt_len &&
		ber_pvt_sb_grow_buffer( dst, pkt_len ) < 0 )
	{
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_encode: failed to grow the buffer to %lu bytes\n",
				pkt_len );
		return -1;
	}

	dst->buf_end = pkt_len;

	b = (unsigned char *)dst->buf_base;

	b[0] = (unsigned char)(wrapped.length >> 24);
	b[1] = (unsigned char)(wrapped.length >> 16);
	b[2] = (unsigned char)(wrapped.length >>  8);
	b[3] = (unsigned char)(wrapped.length >>  0);

	/* copy the wrapped blob to the right location */
	memcpy(b + 4, wrapped.value, wrapped.length);

	gss_release_buffer(&minor_status, &wrapped);

	return 0;
}

static ber_int_t
sb_sasl_gssapi_decode(
	struct sb_sasl_generic_data *p,
	const Sockbuf_Buf *src,
	Sockbuf_Buf *dst )
{
	gss_ctx_id_t gss_ctx = (gss_ctx_id_t)p->ops_private;
	int gss_rc;
	OM_uint32 minor_status;
	gss_buffer_desc unwrapped, wrapped;
	gss_OID ctx_mech = GSS_C_NO_OID;
	OM_uint32 ctx_flags = 0;
	int conf_req_flag = 0;
	int conf_state;
	unsigned char *b;

	wrapped.value	= src->buf_base + 4;
	wrapped.length	= src->buf_end - 4;

	gss_inquire_context(&minor_status,
			    gss_ctx,
			    NULL,
			    NULL,
			    NULL,
			    &ctx_mech,
			    &ctx_flags,
			    NULL,
			    NULL);

	if (ctx_flags & (GSS_C_CONF_FLAG)) {
		conf_req_flag = 1;
	}

	gss_rc = gss_unwrap(&minor_status, gss_ctx,
			    &wrapped, &unwrapped,
			    &conf_state, GSS_C_QOP_DEFAULT);
	if ( gss_rc != GSS_S_COMPLETE ) {
		char msg[256];
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_decode: failed to decode packet: %s\n",
				gsserrstr( msg, sizeof(msg), ctx_mech, gss_rc, minor_status ) );
		return -1;
	}

	if ( conf_req_flag && conf_state == 0 ) {
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_encode: GSS_C_CONF_FLAG was ignored by our peer\n" );
		return -1;
	}

	/* Grow the packet buffer if neccessary */
	if ( dst->buf_size < unwrapped.length &&
		ber_pvt_sb_grow_buffer( dst, unwrapped.length ) < 0 )
	{
		ber_log_printf( LDAP_DEBUG_ANY, p->sbiod->sbiod_sb->sb_debug,
				"sb_sasl_gssapi_decode: failed to grow the buffer to %lu bytes\n",
				unwrapped.length );
		return -1;
	}

	dst->buf_end = unwrapped.length;

	b = (unsigned char *)dst->buf_base;

	/* copy the wrapped blob to the right location */
	memcpy(b, unwrapped.value, unwrapped.length);

	gss_release_buffer(&minor_status, &unwrapped);

	return 0;
}

static void
sb_sasl_gssapi_reset_buf(
	struct sb_sasl_generic_data *p,
	Sockbuf_Buf *buf )
{
	ber_pvt_sb_buf_destroy( buf );
}

static void
sb_sasl_gssapi_fini( struct sb_sasl_generic_data *p )
{
}

static const struct sb_sasl_generic_ops sb_sasl_gssapi_ops = {
	sb_sasl_gssapi_init,
	sb_sasl_gssapi_encode,
	sb_sasl_gssapi_decode,
	sb_sasl_gssapi_reset_buf,
	sb_sasl_gssapi_fini
};

static int
sb_sasl_gssapi_install(
	Sockbuf *sb,
	gss_ctx_id_t gss_ctx )
{
	struct sb_sasl_generic_install install_arg;

	install_arg.ops		= &sb_sasl_gssapi_ops;
	install_arg.ops_private = gss_ctx;

	return ldap_pvt_sasl_generic_install( sb, &install_arg );
}

static void
sb_sasl_gssapi_remove( Sockbuf *sb )
{
	ldap_pvt_sasl_generic_remove( sb );
}

static int
map_gsserr2ldap(
	LDAP *ld,
	gss_OID mech,
	int gss_rc,
	OM_uint32 minor_status )
{
	char msg[256];

	Debug( LDAP_DEBUG_ANY, "%s\n",
	       gsserrstr( msg, sizeof(msg), mech, gss_rc, minor_status ),
	       NULL, NULL );

	if (gss_rc == GSS_S_COMPLETE) {
		ld->ld_errno = LDAP_SUCCESS;
	} else if (GSS_CALLING_ERROR(gss_rc)) {
		ld->ld_errno = LDAP_LOCAL_ERROR;
	} else if (GSS_ROUTINE_ERROR(gss_rc)) {
		ld->ld_errno = LDAP_INAPPROPRIATE_AUTH;
	} else if (gss_rc == GSS_S_CONTINUE_NEEDED) {
		ld->ld_errno = LDAP_SASL_BIND_IN_PROGRESS;
	} else if (GSS_SUPPLEMENTARY_INFO(gss_rc)) {
		ld->ld_errno = LDAP_AUTH_UNKNOWN;
	} else if (GSS_ERROR(gss_rc)) {
		ld->ld_errno = LDAP_AUTH_UNKNOWN;
	} else {
		ld->ld_errno = LDAP_OTHER;
	}

	return ld->ld_errno;
}


static int
ldap_gssapi_get_rootdse_infos (
	LDAP *ld,
	char **pmechlist,
	char **pldapServiceName,
	char **pdnsHostName )
{
	/* we need to query the server for supported mechs anyway */
	LDAPMessage *res, *e;
	char *attrs[] = {
		"supportedSASLMechanisms",
		"ldapServiceName",
		"dnsHostName",
		NULL
	};
	char **values, *mechlist;
	char *ldapServiceName = NULL;
	char *dnsHostName = NULL;
	int rc;

	Debug( LDAP_DEBUG_TRACE, "ldap_gssapi_get_rootdse_infos\n", 0, 0, 0 );

	rc = ldap_search_s( ld, "", LDAP_SCOPE_BASE,
		NULL, attrs, 0, &res );

	if ( rc != LDAP_SUCCESS ) {
		return ld->ld_errno;
	}

	e = ldap_first_entry( ld, res );
	if ( e == NULL ) {
		ldap_msgfree( res );
		if ( ld->ld_errno == LDAP_SUCCESS ) {
			ld->ld_errno = LDAP_NO_SUCH_OBJECT;
		}
		return ld->ld_errno;
	}

	values = ldap_get_values( ld, e, "supportedSASLMechanisms" );
	if ( values == NULL ) {
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_SUCH_ATTRIBUTE;
		return ld->ld_errno;
	}

	mechlist = ldap_charray2str( values, " " );
	if ( mechlist == NULL ) {
		LDAP_VFREE( values );
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	LDAP_VFREE( values );

	values = ldap_get_values( ld, e, "ldapServiceName" );
	if ( values == NULL ) {
		goto get_dns_host_name;
	}

	ldapServiceName = ldap_charray2str( values, " " );
	if ( ldapServiceName == NULL ) {
		LDAP_FREE( mechlist );
		LDAP_VFREE( values );
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}
	LDAP_VFREE( values );

get_dns_host_name:

	values = ldap_get_values( ld, e, "dnsHostName" );
	if ( values == NULL ) {
		goto done;
	}

	dnsHostName = ldap_charray2str( values, " " );
	if ( dnsHostName == NULL ) {
		LDAP_FREE( mechlist );
		LDAP_FREE( ldapServiceName );
		LDAP_VFREE( values );
		ldap_msgfree( res );
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}
	LDAP_VFREE( values );

done:
	ldap_msgfree( res );

	*pmechlist = mechlist;
	*pldapServiceName = ldapServiceName;
	*pdnsHostName = dnsHostName;

	return LDAP_SUCCESS;
}


static int check_for_gss_spnego_support( LDAP *ld, const char *mechs_str )
{
	int rc;
	char **mechs_list = NULL;

	mechs_list = ldap_str2charray( mechs_str, " " );
	if ( mechs_list == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	rc = ldap_charray_inlist( mechs_list, "GSS-SPNEGO" );
	ldap_charray_free( mechs_list );
	if ( rc != 1) {
		ld->ld_errno = LDAP_STRONG_AUTH_NOT_SUPPORTED;
		return ld->ld_errno;
	}

	return LDAP_SUCCESS;
}

static int
guess_service_principal(
	LDAP *ld,
	const char *ldapServiceName,
	const char *dnsHostName,
	gss_name_t *principal )
{
	gss_buffer_desc input_name;
	/* GSS_KRB5_NT_PRINCIPAL_NAME */
	gss_OID_desc nt_principal =
	{10, "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02\x01"};
	const char *host = ld->ld_defconn->lconn_server->lud_host;
	OM_uint32 minor_status;
	int gss_rc;
	int ret;
	size_t svc_principal_size;
	char *svc_principal = NULL;
	const char *principal_fmt = NULL;
	const char *str = NULL;
	const char *givenstr = NULL;
	const char *ignore = "not_defined_in_RFC4178@please_ignore";
	int allow_remote = 0;

	if (ldapServiceName) {
		givenstr = strchr(ldapServiceName, ':');
		if (givenstr && givenstr[1]) {
			givenstr++;
			if (strcmp(givenstr, ignore) == 0) {
				givenstr = NULL;
			}
		} else {
			givenstr = NULL;
		}
	}

	if ( ld->ld_options.ldo_gssapi_options & LDAP_GSSAPI_OPT_ALLOW_REMOTE_PRINCIPAL ) {
		allow_remote = 1;
	}

	if (allow_remote && givenstr) {
		principal_fmt = "%s";
		svc_principal_size = strlen(givenstr) + 1;
		str = givenstr;

	} else if (allow_remote && dnsHostName) {
		principal_fmt = "ldap/%s";
		svc_principal_size = STRLENOF("ldap/") + strlen(dnsHostName) + 1;
		str = dnsHostName;

	} else {
		principal_fmt = "ldap/%s";
		svc_principal_size = STRLENOF("ldap/") + strlen(host) + 1;
		str = host;
	}

	svc_principal = (char*) ldap_memalloc(svc_principal_size * sizeof(char));
	if ( svc_principal == NULL ) {
		ld->ld_errno = LDAP_NO_MEMORY;
		return ld->ld_errno;
	}

	ret = snprintf( svc_principal, svc_principal_size, principal_fmt, str );
	if (ret < 0 || (size_t)ret >= svc_principal_size) {
		ld->ld_errno = LDAP_LOCAL_ERROR;
		return ld->ld_errno;
	}

	Debug( LDAP_DEBUG_TRACE, "principal for host[%s]: '%s'\n",
	       host, svc_principal, 0 );

	input_name.value  = svc_principal;
	input_name.length = (size_t)ret;

	gss_rc = gss_import_name( &minor_status, &input_name, &nt_principal, principal );
	ldap_memfree( svc_principal );
	if ( gss_rc != GSS_S_COMPLETE ) {
		return map_gsserr2ldap( ld, GSS_C_NO_OID, gss_rc, minor_status );
	}

	return LDAP_SUCCESS;
}

void ldap_int_gssapi_close( LDAP *ld, LDAPConn *lc )
{
	if ( lc && lc->lconn_gss_ctx ) {
		OM_uint32 minor_status;
		OM_uint32 ctx_flags = 0;
		gss_ctx_id_t old_gss_ctx = GSS_C_NO_CONTEXT;
		old_gss_ctx = (gss_ctx_id_t)lc->lconn_gss_ctx;

		gss_inquire_context(&minor_status,
				    old_gss_ctx,
				    NULL,
				    NULL,
				    NULL,
				    NULL,
				    &ctx_flags,
				    NULL,
				    NULL);

		if (!( ld->ld_options.ldo_gssapi_options & LDAP_GSSAPI_OPT_DO_NOT_FREE_GSS_CONTEXT )) {
			gss_delete_sec_context( &minor_status, &old_gss_ctx, GSS_C_NO_BUFFER );
		}
		lc->lconn_gss_ctx = GSS_C_NO_CONTEXT;

		if (ctx_flags & (GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG)) {
			/* remove wrapping layer */
			sb_sasl_gssapi_remove( lc->lconn_sb );
		}
	}
}

static void
ldap_int_gssapi_setup(
	LDAP *ld,
	LDAPConn *lc,
	gss_ctx_id_t gss_ctx)
{
	OM_uint32 minor_status;
	OM_uint32 ctx_flags = 0;

	ldap_int_gssapi_close( ld, lc );

	gss_inquire_context(&minor_status,
			    gss_ctx,
			    NULL,
			    NULL,
			    NULL,
			    NULL,
			    &ctx_flags,
			    NULL,
			    NULL);

	lc->lconn_gss_ctx = gss_ctx;

	if (ctx_flags & (GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG)) {
		/* setup wrapping layer */
		sb_sasl_gssapi_install( lc->lconn_sb, gss_ctx );
	}
}

#ifdef LDAP_R_COMPILE
ldap_pvt_thread_mutex_t ldap_int_gssapi_mutex;
#endif

static int
ldap_int_gss_spnego_bind_s( LDAP *ld )
{
	int rc;
	int gss_rc;
	OM_uint32 minor_status;
	char *mechlist = NULL;
	char *ldapServiceName = NULL;
	char *dnsHostName = NULL;
	gss_OID_set supported_mechs = GSS_C_NO_OID_SET;
	int spnego_support = 0;
#define	__SPNEGO_OID_LENGTH 6
#define	__SPNEGO_OID "\053\006\001\005\005\002"
	gss_OID_desc spnego_oid = {__SPNEGO_OID_LENGTH, __SPNEGO_OID};
	gss_OID req_mech = GSS_C_NO_OID;
	gss_OID ret_mech = GSS_C_NO_OID;
	gss_ctx_id_t gss_ctx = GSS_C_NO_CONTEXT;
	gss_name_t principal = GSS_C_NO_NAME;
	OM_uint32 req_flags;
	OM_uint32 ret_flags;
	gss_buffer_desc input_token, output_token = GSS_C_EMPTY_BUFFER;
	struct berval cred, *scred = NULL;

	LDAP_MUTEX_LOCK( &ldap_int_gssapi_mutex );

	/* get information from RootDSE entry */
	rc = ldap_gssapi_get_rootdse_infos ( ld, &mechlist,
					     &ldapServiceName, &dnsHostName);
	if ( rc != LDAP_SUCCESS ) {
		return rc;
	}

	/* check that the server supports GSS-SPNEGO */
	rc = check_for_gss_spnego_support( ld, mechlist );
	if ( rc != LDAP_SUCCESS ) {
		goto rc_error;
	}

	/* prepare new gss_ctx_id_t */
	rc = guess_service_principal( ld, ldapServiceName, dnsHostName, &principal );
	if ( rc != LDAP_SUCCESS ) {
		goto rc_error;
	}

	/* see if our gssapi library supports spnego */
	gss_rc = gss_indicate_mechs( &minor_status, &supported_mechs );
	if ( gss_rc != GSS_S_COMPLETE ) {
		goto gss_error;
	}
	gss_rc = gss_test_oid_set_member( &minor_status,
		&spnego_oid, supported_mechs, &spnego_support);
	gss_release_oid_set( &minor_status, &supported_mechs);
	if ( gss_rc != GSS_S_COMPLETE ) {
		goto gss_error;
	}
	if ( spnego_support != 0 ) {
		req_mech = &spnego_oid;
	}

	req_flags = ld->ld_options.ldo_gssapi_flags;
	req_flags |= GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG;

	/*
	 * loop around gss_init_sec_context() and ldap_sasl_bind_s()
	 */
	input_token.value = NULL;
	input_token.length = 0;
	gss_rc = gss_init_sec_context(&minor_status,
				      GSS_C_NO_CREDENTIAL,
				      &gss_ctx,
				      principal,
				      req_mech,
				      req_flags,
				      0,
				      NULL,
				      &input_token,
				      &ret_mech,
				      &output_token,
				      &ret_flags,
				      NULL);
	if ( gss_rc == GSS_S_COMPLETE ) {
		rc = LDAP_INAPPROPRIATE_AUTH;
		goto rc_error;
	}
	if ( gss_rc != GSS_S_CONTINUE_NEEDED ) {
		goto gss_error;
	}
	while (1) {
		cred.bv_val = (char *)output_token.value;
		cred.bv_len = output_token.length;
		rc = ldap_sasl_bind_s( ld, NULL, "GSS-SPNEGO", &cred, NULL, NULL, &scred );
		gss_release_buffer( &minor_status, &output_token );
		if ( rc != LDAP_SUCCESS && rc != LDAP_SASL_BIND_IN_PROGRESS ) {
			goto rc_error;
		}

		if ( scred ) {
			input_token.value = scred->bv_val;
			input_token.length = scred->bv_len;
		} else {
			input_token.value = NULL;
			input_token.length = 0;
		}

		gss_rc = gss_init_sec_context(&minor_status,
					      GSS_C_NO_CREDENTIAL,
					      &gss_ctx,
					      principal,
					      req_mech,
					      req_flags,
					      0,
					      NULL,
					      &input_token,
					      &ret_mech,
					      &output_token,
					      &ret_flags,
					      NULL);
		if ( scred ) {
			ber_bvfree( scred );
		}
		if ( gss_rc == GSS_S_COMPLETE ) {
			gss_release_buffer( &minor_status, &output_token );
			break;
		}

		if ( gss_rc != GSS_S_CONTINUE_NEEDED ) {
			goto gss_error;
		}
	}

 	ldap_int_gssapi_setup( ld, ld->ld_defconn, gss_ctx);
	gss_ctx = GSS_C_NO_CONTEXT;

	rc = LDAP_SUCCESS;
	goto rc_error;

gss_error:
	rc = map_gsserr2ldap( ld, 
			      (ret_mech != GSS_C_NO_OID ? ret_mech : req_mech ),
			      gss_rc, minor_status );
rc_error:
	LDAP_MUTEX_UNLOCK( &ldap_int_gssapi_mutex );
	LDAP_FREE( mechlist );
	LDAP_FREE( ldapServiceName );
	LDAP_FREE( dnsHostName );
	gss_release_buffer( &minor_status, &output_token );
	if ( gss_ctx != GSS_C_NO_CONTEXT ) {
		gss_delete_sec_context( &minor_status, &gss_ctx, GSS_C_NO_BUFFER );
	}
	if ( principal != GSS_C_NO_NAME ) {
		gss_release_name( &minor_status, &principal );
	}
	return rc;
}

int
ldap_int_gssapi_config( struct ldapoptions *lo, int option, const char *arg )
{
	int ok = 0;

	switch( option ) {
	case LDAP_OPT_SIGN:

		if (!arg) {
		} else if (strcasecmp(arg, "on") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "yes") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "true") == 0) {
			ok = 1;

		}
		if (ok) {
			lo->ldo_gssapi_flags |= GSS_C_INTEG_FLAG;
		}

		return 0;

	case LDAP_OPT_ENCRYPT:

		if (!arg) {
		} else if (strcasecmp(arg, "on") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "yes") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "true") == 0) {
			ok = 1;
		}

		if (ok) {
			lo->ldo_gssapi_flags |= GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG;
		}

		return 0;

	case LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL:

		if (!arg) {
		} else if (strcasecmp(arg, "on") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "yes") == 0) {
			ok = 1;
		} else if (strcasecmp(arg, "true") == 0) {
			ok = 1;
		}

		if (ok) {
			lo->ldo_gssapi_options |= LDAP_GSSAPI_OPT_ALLOW_REMOTE_PRINCIPAL;
		}

		return 0;
	}

	return -1;
}

int
ldap_int_gssapi_get_option( LDAP *ld, int option, void *arg )
{
	if ( ld == NULL )
		return -1;

	switch ( option ) {
	case LDAP_OPT_SSPI_FLAGS:
		* (unsigned *) arg = (unsigned) ld->ld_options.ldo_gssapi_flags;
		break;

	case LDAP_OPT_SIGN:
		if ( ld->ld_options.ldo_gssapi_flags & GSS_C_INTEG_FLAG ) {
			* (int *) arg = (int)-1;
		} else {
			* (int *) arg = (int)0;
		}
		break;

	case LDAP_OPT_ENCRYPT:
		if ( ld->ld_options.ldo_gssapi_flags & GSS_C_CONF_FLAG ) {
			* (int *) arg = (int)-1;
		} else {
			* (int *) arg = (int)0;
		}
		break;

	case LDAP_OPT_SASL_METHOD:
		* (char **) arg = LDAP_STRDUP("GSS-SPNEGO");
		break;

	case LDAP_OPT_SECURITY_CONTEXT:
		if ( ld->ld_defconn && ld->ld_defconn->lconn_gss_ctx ) {
			* (gss_ctx_id_t *) arg = (gss_ctx_id_t)ld->ld_defconn->lconn_gss_ctx;
		} else {
			* (gss_ctx_id_t *) arg = GSS_C_NO_CONTEXT;
		}
		break;

	case LDAP_OPT_X_GSSAPI_DO_NOT_FREE_CONTEXT:
		if ( ld->ld_options.ldo_gssapi_options & LDAP_GSSAPI_OPT_DO_NOT_FREE_GSS_CONTEXT ) {
			* (int *) arg = (int)-1;
		} else {
			* (int *) arg = (int)0;
		}
		break;

	case LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL:
		if ( ld->ld_options.ldo_gssapi_options & LDAP_GSSAPI_OPT_ALLOW_REMOTE_PRINCIPAL ) {
			* (int *) arg = (int)-1;
		} else {
			* (int *) arg = (int)0;
		}
		break;

	default:
		return -1;
	}

	return 0;
}

int
ldap_int_gssapi_set_option( LDAP *ld, int option, void *arg )
{
	if ( ld == NULL )
		return -1;

	switch ( option ) {
	case LDAP_OPT_SSPI_FLAGS:
		if ( arg != LDAP_OPT_OFF ) {
			ld->ld_options.ldo_gssapi_flags = * (unsigned *)arg;
		}
		break;

	case LDAP_OPT_SIGN:
		if ( arg != LDAP_OPT_OFF ) {
			ld->ld_options.ldo_gssapi_flags |= GSS_C_INTEG_FLAG;
		}
		break;

	case LDAP_OPT_ENCRYPT:
		if ( arg != LDAP_OPT_OFF ) {
			ld->ld_options.ldo_gssapi_flags |= GSS_C_INTEG_FLAG | GSS_C_CONF_FLAG;
		}
		break;

	case LDAP_OPT_SASL_METHOD:
		if ( arg != LDAP_OPT_OFF ) {
			const char *m = (const char *)arg;
			if ( strcmp( "GSS-SPNEGO", m ) != 0 ) {
				/* we currently only support GSS-SPNEGO */
				return -1;
			}
		}
		break;

	case LDAP_OPT_SECURITY_CONTEXT:
		if ( arg != LDAP_OPT_OFF && ld->ld_defconn) {
			ldap_int_gssapi_setup( ld, ld->ld_defconn,
					       (gss_ctx_id_t) arg);
		}
		break;

	case LDAP_OPT_X_GSSAPI_DO_NOT_FREE_CONTEXT:
		if ( arg != LDAP_OPT_OFF ) {
			ld->ld_options.ldo_gssapi_options |= LDAP_GSSAPI_OPT_DO_NOT_FREE_GSS_CONTEXT;
		}
		break;

	case LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL:
		if ( arg != LDAP_OPT_OFF ) {
			ld->ld_options.ldo_gssapi_options |= LDAP_GSSAPI_OPT_ALLOW_REMOTE_PRINCIPAL;
		}
		break;

	default:
		return -1;
	}

	return 0;
}

#else /* HAVE_GSSAPI */
#define ldap_int_gss_spnego_bind_s(ld) LDAP_NOT_SUPPORTED
#endif /* HAVE_GSSAPI */

int
ldap_gssapi_bind(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *creds )
{
	return LDAP_NOT_SUPPORTED;
}

int
ldap_gssapi_bind_s(
	LDAP *ld,
	LDAP_CONST char *dn,
	LDAP_CONST char *creds )
{
	if ( dn != NULL ) {
		return LDAP_NOT_SUPPORTED;
	}

	if ( creds != NULL ) {
		return LDAP_NOT_SUPPORTED;
	}

	return ldap_int_gss_spnego_bind_s(ld);
}
