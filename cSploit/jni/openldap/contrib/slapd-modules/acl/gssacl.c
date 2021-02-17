/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2011 PADL Software Pty Ltd.
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

#include <portable.h>

#include <ac/string.h>
#include <slap.h>
#include <lutil.h>

#include <sasl/sasl.h>
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>

#define ACL_BUF_SIZE 1024

typedef struct gssattr_t {
	slap_style_t		gssattr_style;
	struct berval		gssattr_name;		/* asserted name */
	struct berval		gssattr_value;		/* asserted value */
} gssattr_t;

static int gssattr_dynacl_destroy( void *priv );

static int
regex_matches(
	struct berval	*pat,		/* pattern to expand and match against */
	char		*str,		/* string to match against pattern */
	struct berval	*dn_matches,	/* buffer with $N expansion variables from DN */
	struct berval	*val_matches,	/* buffer with $N expansion variables from val */
	AclRegexMatches	*matches	/* offsets in buffer for $N expansion variables */
);

static int
gssattr_dynacl_parse(
	const char	*fname,
	int 		lineno,
	const char	*opts,
	slap_style_t	style,
	const char	*pattern,
	void		**privp )
{
	gssattr_t	*gssattr;

	gssattr = (gssattr_t *)ch_calloc( 1, sizeof( gssattr_t ) );

	if ( opts == NULL || opts[0] == '\0' ) {
		fprintf( stderr, "%s line %d: GSS ACL: no attribute specified.\n",
			 fname, lineno );
		goto cleanup;
	}

	if ( pattern == NULL || pattern[0] == '\0' ) {
		fprintf( stderr, "%s line %d: GSS ACL: no attribute value specified.\n",
			 fname, lineno );
		goto cleanup;
	}

	gssattr->gssattr_style = style;

	switch ( gssattr->gssattr_style ) {
	case ACL_STYLE_BASE:
	case ACL_STYLE_REGEX:
	case ACL_STYLE_EXPAND:
		break;
	default:
		fprintf( stderr, "%s line %d: GSS ACL: unsupported style \"%s\".\n",
			 fname, lineno, style_strings[style] );
		goto cleanup;
		break;
	}

	ber_str2bv( opts,    0, 1, &gssattr->gssattr_name );
	ber_str2bv( pattern, 0, 1, &gssattr->gssattr_value );

	*privp = (void *)gssattr;
	return 0;

cleanup:
	(void)gssattr_dynacl_destroy( (void *)gssattr );

	return 1;
}

static int
gssattr_dynacl_unparse(
	void		*priv,
	struct berval	*bv )
{
	gssattr_t	*gssattr = (gssattr_t *)priv;
	char		*ptr;

	bv->bv_len = STRLENOF( " dynacl/gss/.expand=" ) +
		     gssattr->gssattr_name.bv_len +
		     gssattr->gssattr_value.bv_len;
	bv->bv_val = ch_malloc( bv->bv_len + 1 );

	ptr = lutil_strcopy( bv->bv_val, " dynacl/gss/" );
	ptr = lutil_strncopy( ptr, gssattr->gssattr_name.bv_val,
			      gssattr->gssattr_name.bv_len );
	switch ( gssattr->gssattr_style ) {
	case ACL_STYLE_BASE:
		ptr = lutil_strcopy( ptr, ".exact=" );
		break;
	case ACL_STYLE_REGEX:
		ptr = lutil_strcopy( ptr, ".regex=" );
		break;
	case ACL_STYLE_EXPAND:
		ptr = lutil_strcopy( ptr, ".expand=" );
		break;
	default:
		assert( 0 );
		break;
	}

	ptr = lutil_strncopy( ptr, gssattr->gssattr_value.bv_val,
			      gssattr->gssattr_value.bv_len );

	ptr[ 0 ] = '\0';

	bv->bv_len = ptr - bv->bv_val;

	return 0;
}

static int
gssattr_dynacl_mask(
	void			*priv,
	Operation		*op,
	Entry			*target,
	AttributeDescription	*desc,
	struct berval		*val,
	int			nmatch,
	regmatch_t		*matches,
	slap_access_t		*grant,
	slap_access_t		*deny )
{
	gssattr_t	*gssattr = (gssattr_t *)priv;
	sasl_conn_t	*sasl_ctx = op->o_conn->c_sasl_authctx;
	gss_name_t	gss_name = GSS_C_NO_NAME;
	OM_uint32	major, minor;
	int		more = -1;
	int		authenticated, complete;
	gss_buffer_desc	attr = GSS_C_EMPTY_BUFFER;
	int		granted = 0;

	ACL_INVALIDATE( *deny );

	if ( sasl_ctx == NULL ||
	     sasl_getprop( sasl_ctx, SASL_GSS_PEER_NAME, (const void **)&gss_name) != 0 ||
	     gss_name == GSS_C_NO_NAME ) {
		return 0;
	}

	attr.length = gssattr->gssattr_name.bv_len;
	attr.value = gssattr->gssattr_name.bv_val;

	while ( more != 0 ) {
		AclRegexMatches amatches = { 0 };
		gss_buffer_desc	gss_value = GSS_C_EMPTY_BUFFER;
		gss_buffer_desc	gss_display_value = GSS_C_EMPTY_BUFFER;
		struct berval bv_value;

		major = gss_get_name_attribute( &minor, gss_name, &attr,
						&authenticated, &complete,
						&gss_value, &gss_display_value, &more );
		if ( GSS_ERROR( major ) ) {
			break;
		} else if ( authenticated == 0 ) {
			gss_release_buffer( &minor, &gss_value );
			gss_release_buffer( &minor, &gss_display_value );
			continue;
		}

		bv_value.bv_len = gss_value.length;
		bv_value.bv_val = (char *)gss_value.value;

		if ( !ber_bvccmp( &gssattr->gssattr_value, '*' ) ) {
			if ( gssattr->gssattr_style != ACL_STYLE_BASE ) {
				amatches.dn_count = nmatch;
				AC_MEMCPY( amatches.dn_data, matches, sizeof( amatches.dn_data ) );
			}

			switch ( gssattr->gssattr_style ) {
			case ACL_STYLE_REGEX:
				/* XXX assumes value NUL terminated */
				granted = regex_matches( &gssattr->gssattr_value, bv_value.bv_val,
							  &target->e_nname, val, &amatches );
				break;
			case ACL_STYLE_EXPAND: {
				struct berval bv;
				char buf[ACL_BUF_SIZE];

				bv.bv_len = sizeof( buf ) - 1;
				bv.bv_val = buf;

				granted = ( acl_string_expand( &bv, &gssattr->gssattr_value,
							       &target->e_nname, val,
							       &amatches ) == 0 ) &&
					  ( ber_bvstrcmp( &bv, &bv_value) == 0 );
				break;
			}
			case ACL_STYLE_BASE:
				granted = ( ber_bvstrcmp( &gssattr->gssattr_value, &bv_value ) == 0 );
				break;
			default:
				assert(0);
				break;
			}
		} else {
			granted = 1;
		}

		gss_release_buffer( &minor, &gss_value );
		gss_release_buffer( &minor, &gss_display_value );

		if ( granted ) {
			break;
		}
	}

	if ( granted ) {
		ACL_LVL_ASSIGN_WRITE( *grant );
	}

	return 0;
}

static int
gssattr_dynacl_destroy(
	void		*priv )
{
	gssattr_t		*gssattr = (gssattr_t *)priv;

	if ( gssattr != NULL ) {
		if ( !BER_BVISNULL( &gssattr->gssattr_name ) ) {
			ber_memfree( gssattr->gssattr_name.bv_val );
		}
		if ( !BER_BVISNULL( &gssattr->gssattr_value ) ) {
			ber_memfree( gssattr->gssattr_value.bv_val );
		}
		ch_free( gssattr );
	}

	return 0;
}

static struct slap_dynacl_t gssattr_dynacl = {
	"gss",
	gssattr_dynacl_parse,
	gssattr_dynacl_unparse,
	gssattr_dynacl_mask,
	gssattr_dynacl_destroy
};

int
init_module( int argc, char *argv[] )
{
	return slap_dynacl_register( &gssattr_dynacl );
}


static int
regex_matches(
	struct berval	*pat,		/* pattern to expand and match against */
	char		*str,		/* string to match against pattern */
	struct berval	*dn_matches,	/* buffer with $N expansion variables from DN */
	struct berval	*val_matches,	/* buffer with $N expansion variables from val */
	AclRegexMatches	*matches	/* offsets in buffer for $N expansion variables */
)
{
	regex_t re;
	char newbuf[ACL_BUF_SIZE];
	struct berval bv;
	int	rc;

	bv.bv_len = sizeof( newbuf ) - 1;
	bv.bv_val = newbuf;

	if (str == NULL) {
		str = "";
	};

	acl_string_expand( &bv, pat, dn_matches, val_matches, matches );
	rc = regcomp( &re, newbuf, REG_EXTENDED|REG_ICASE );
	if ( rc ) {
		char error[ACL_BUF_SIZE];
		regerror( rc, &re, error, sizeof( error ) );

		Debug( LDAP_DEBUG_TRACE,
		    "compile( \"%s\", \"%s\") failed %s\n",
			pat->bv_val, str, error );
		return( 0 );
	}

	rc = regexec( &re, str, 0, NULL, 0 );
	regfree( &re );

	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: string:	 %s\n", str, 0, 0 );
	Debug( LDAP_DEBUG_TRACE,
	    "=> regex_matches: rc: %d %s\n",
		rc, !rc ? "matches" : "no matches", 0 );
	return( !rc );
}

