/* unique.c - attribute uniqueness module */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004,2006-2007 Symas Corporation.
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
/* ACKNOWLEDGEMENTS: 
 * This work was initially developed by Symas Corporation for
 * inclusion in OpenLDAP Software, with subsequent enhancements by
 * Emily Backes at Symas Corporation.  This work was sponsored by
 * Hewlett-Packard.
 */

#include "portable.h"

#ifdef SLAPD_OVER_UNIQUE

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"

#define UNIQUE_DEFAULT_URI ("ldap:///??sub")

static slap_overinst unique;

typedef struct unique_attrs_s {
	struct unique_attrs_s *next;	      /* list of attrs */
	AttributeDescription *attr;
} unique_attrs;

typedef struct unique_domain_uri_s {
	struct unique_domain_uri_s *next;
	struct berval dn;
	struct berval ndn;
	struct berval filter;
	Filter *f;
	struct unique_attrs_s *attrs;
	int scope;
} unique_domain_uri;

typedef struct unique_domain_s {
	struct unique_domain_s *next;
	struct berval domain_spec;
	struct unique_domain_uri_s *uri;
	char ignore;                          /* polarity of attributes */
	char strict;                          /* null considered unique too */
} unique_domain;

typedef struct unique_data_s {
	struct unique_domain_s *domains;
	struct unique_domain_s *legacy;
	char legacy_strict_set;
} unique_data;

typedef struct unique_counter_s {
	struct berval *ndn;
	int count;
} unique_counter;

enum {
	UNIQUE_BASE = 1,
	UNIQUE_IGNORE,
	UNIQUE_ATTR,
	UNIQUE_STRICT,
	UNIQUE_URI
};

static ConfigDriver unique_cf_base;
static ConfigDriver unique_cf_attrs;
static ConfigDriver unique_cf_strict;
static ConfigDriver unique_cf_uri;

static ConfigTable uniquecfg[] = {
	{ "unique_base", "basedn", 2, 2, 0, ARG_DN|ARG_MAGIC|UNIQUE_BASE,
	  unique_cf_base, "( OLcfgOvAt:10.1 NAME 'olcUniqueBase' "
	  "DESC 'Subtree for uniqueness searches' "
	  "EQUALITY distinguishedNameMatch "
	  "SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ "unique_ignore", "attribute...", 2, 0, 0, ARG_MAGIC|UNIQUE_IGNORE,
	  unique_cf_attrs, "( OLcfgOvAt:10.2 NAME 'olcUniqueIgnore' "
	  "DESC 'Attributes for which uniqueness shall not be enforced' "
	  "EQUALITY caseIgnoreMatch "
	  "ORDERING caseIgnoreOrderingMatch "
	  "SUBSTR caseIgnoreSubstringsMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "unique_attributes", "attribute...", 2, 0, 0, ARG_MAGIC|UNIQUE_ATTR,
	  unique_cf_attrs, "( OLcfgOvAt:10.3 NAME 'olcUniqueAttribute' "
	  "DESC 'Attributes for which uniqueness shall be enforced' "
	  "EQUALITY caseIgnoreMatch "
	  "ORDERING caseIgnoreOrderingMatch "
	  "SUBSTR caseIgnoreSubstringsMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "unique_strict", "on|off", 1, 2, 0, ARG_MAGIC|UNIQUE_STRICT,
	  unique_cf_strict, "( OLcfgOvAt:10.4 NAME 'olcUniqueStrict' "
	  "DESC 'Enforce uniqueness of null values' "
	  "EQUALITY booleanMatch "
	  "SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "unique_uri", "ldapuri", 2, 3, 0, ARG_MAGIC|UNIQUE_URI,
	  unique_cf_uri, "( OLcfgOvAt:10.5 NAME 'olcUniqueURI' "
	  "DESC 'List of keywords and LDAP URIs for a uniqueness domain' "
	  "EQUALITY caseExactMatch "
	  "ORDERING caseExactOrderingMatch "
	  "SUBSTR caseExactSubstringsMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs uniqueocs[] = {
	{ "( OLcfgOvOc:10.1 "
	  "NAME 'olcUniqueConfig' "
	  "DESC 'Attribute value uniqueness configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcUniqueBase $ olcUniqueIgnore $ "
	  "olcUniqueAttribute $ olcUniqueStrict $ "
	  "olcUniqueURI ) )",
	  Cft_Overlay, uniquecfg },
	{ NULL, 0, NULL }
};

static void
unique_free_domain_uri ( unique_domain_uri *uri )
{
	unique_domain_uri *next_uri = NULL;
	unique_attrs *attr, *next_attr = NULL;

	while ( uri ) {
		next_uri = uri->next;
		ch_free ( uri->dn.bv_val );
		ch_free ( uri->ndn.bv_val );
		ch_free ( uri->filter.bv_val );
		filter_free( uri->f );
		attr = uri->attrs;
		while ( attr ) {
			next_attr = attr->next;
			ch_free (attr);
			attr = next_attr;
		}
		ch_free ( uri );
		uri = next_uri;
	}
}

/* free an entire stack of domains */
static void
unique_free_domain ( unique_domain *domain )
{
	unique_domain *next_domain = NULL;

	while ( domain ) {
		next_domain = domain->next;
		ch_free ( domain->domain_spec.bv_val );
		unique_free_domain_uri ( domain->uri );
		ch_free ( domain );
		domain = next_domain;
	}
}

static int
unique_new_domain_uri ( unique_domain_uri **urip,
			const LDAPURLDesc *url_desc,
			ConfigArgs *c )
{
	int i, rc = LDAP_SUCCESS;
	unique_domain_uri *uri;
	struct berval bv = {0, NULL};
	BackendDB *be = (BackendDB *)c->be;
	char ** attr_str;
	AttributeDescription * ad;
	const char * text;

	uri = ch_calloc ( 1, sizeof ( unique_domain_uri ) );

	if ( url_desc->lud_host && url_desc->lud_host[0] ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			  "host <%s> not allowed in URI",
			  url_desc->lud_host );
		rc = ARG_BAD_CONF;
		goto exit;
	}

	if ( url_desc->lud_dn && url_desc->lud_dn[0] ) {
		ber_str2bv( url_desc->lud_dn, 0, 0, &bv );
		rc = dnPrettyNormal( NULL,
				     &bv,
				     &uri->dn,
				     &uri->ndn,
				     NULL );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "<%s> invalid DN %d (%s)",
				  url_desc->lud_dn, rc, ldap_err2string( rc ));
			rc = ARG_BAD_CONF;
			goto exit;
		}

		if ( be->be_nsuffix == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "suffix must be set" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			goto exit;
		}

		if ( !dnIsSuffix ( &uri->ndn, &be->be_nsuffix[0] ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "dn <%s> is not a suffix of backend base dn <%s>",
				  uri->dn.bv_val,
				  be->be_nsuffix[0].bv_val );
			rc = ARG_BAD_CONF;
			goto exit;
		}

		if ( BER_BVISNULL( &be->be_rootndn ) || BER_BVISEMPTY( &be->be_rootndn ) ) {
			Debug( LDAP_DEBUG_ANY,
				"slapo-unique needs a rootdn; "
				"backend <%s> has none, YMMV.\n",
				be->be_nsuffix[0].bv_val, 0, 0 );
		}
	}

	attr_str = url_desc->lud_attrs;
	if ( attr_str ) {
		for ( i=0; attr_str[i]; ++i ) {
			unique_attrs * attr;
			ad = NULL;
			if ( slap_str2ad ( attr_str[i], &ad, &text )
			     == LDAP_SUCCESS) {
				attr = ch_calloc ( 1,
						   sizeof ( unique_attrs ) );
				attr->attr = ad;
				attr->next = uri->attrs;
				uri->attrs = attr;
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					  "unique: attribute: %s: %s",
					  attr_str[i], text );
				rc = ARG_BAD_CONF;
				goto exit;
			}
		}
	}

	uri->scope = url_desc->lud_scope;
	if ( !uri->scope ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			  "unique: uri with base scope will always be unique");
		rc = ARG_BAD_CONF;
		goto exit;
	}

	if (url_desc->lud_filter) {
		char *ptr;
		uri->f = str2filter( url_desc->lud_filter );
		if ( !uri->f ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "unique: bad filter");
			rc = ARG_BAD_CONF;
			goto exit;
		}
		/* make sure the strfilter is in normal form (ITS#5581) */
		filter2bv( uri->f, &uri->filter );
		ptr = strstr( uri->filter.bv_val, "(?=" /*)*/ );
		if ( ptr != NULL && ptr <= ( uri->filter.bv_val - STRLENOF( "(?=" /*)*/ ) + uri->filter.bv_len ) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "unique: bad filter");
			rc = ARG_BAD_CONF;
			goto exit;
		}
	}
exit:
	uri->next = *urip;
	*urip = uri;
	if ( rc ) {
		Debug ( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s: %s\n", c->log, c->cr_msg, 0 );
		unique_free_domain_uri ( uri );
		*urip = NULL;
	}
	return rc;
}

static int
unique_new_domain_uri_basic ( unique_domain_uri **urip,
			      ConfigArgs *c )
{
	LDAPURLDesc *url_desc = NULL;
	int rc;

	rc = ldap_url_parse ( UNIQUE_DEFAULT_URI, &url_desc );
	if ( rc ) return rc;
	rc = unique_new_domain_uri ( urip, url_desc, c );
	ldap_free_urldesc ( url_desc );
	return rc;
}

/* if *domain is non-null, it's pushed down the stack.
 * note that the entire stack is freed if there is an error,
 * so build added domains in a separate stack before adding them
 *
 * domain_specs look like
 *
 * [strict ][ignore ]uri[[ uri]...]
 * e.g. "ldap:///ou=foo,o=bar?uid?sub ldap:///ou=baz,o=bar?uid?sub"
 *      "strict ldap:///ou=accounts,o=bar?uid,uidNumber?one"
 *      etc
 *
 * so finally strictness is per-domain
 * but so is ignore-state, and that would be better as a per-url thing
 */
static int
unique_new_domain ( unique_domain **domainp,
		    char *domain_spec,
		    ConfigArgs *c )
{
	char *uri_start;
	int rc = LDAP_SUCCESS;
	int uri_err = 0;
	unique_domain * domain;
	LDAPURLDesc *url_desc, *url_descs = NULL;

	Debug(LDAP_DEBUG_TRACE, "==> unique_new_domain <%s>\n",
	      domain_spec, 0, 0);

	domain = ch_calloc ( 1, sizeof (unique_domain) );
	ber_str2bv( domain_spec, 0, 1, &domain->domain_spec );

	uri_start = domain_spec;
	if ( strncasecmp ( uri_start, "ignore ",
			   STRLENOF( "ignore " ) ) == 0 ) {
		domain->ignore = 1;
		uri_start += STRLENOF( "ignore " );
	}
	if ( strncasecmp ( uri_start, "strict ",
			   STRLENOF( "strict " ) ) == 0 ) {
		domain->strict = 1;
		uri_start += STRLENOF( "strict " );
		if ( !domain->ignore
		     && strncasecmp ( uri_start, "ignore ",
				      STRLENOF( "ignore " ) ) == 0 ) {
			domain->ignore = 1;
			uri_start += STRLENOF( "ignore " );
		}
	}
	rc = ldap_url_parselist_ext ( &url_descs, uri_start, " ", 0 );
	if ( rc ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			  "<%s> invalid ldap urilist",
			  uri_start );
		rc = ARG_BAD_CONF;
		goto exit;
	}

	for ( url_desc = url_descs;
	      url_desc;
	      url_desc = url_descs->lud_next ) {
		rc = unique_new_domain_uri ( &domain->uri,
					     url_desc,
					     c );
		if ( rc ) {
			rc = ARG_BAD_CONF;
			uri_err = 1;
			goto exit;
		}
	}

exit:
	if ( url_descs ) ldap_free_urldesc ( url_descs );
	domain->next = *domainp;
	*domainp = domain;
	if ( rc ) {
		Debug ( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s: %s\n", c->log, c->cr_msg, 0 );
		unique_free_domain ( domain );
		*domainp = NULL;
	}
	return rc;
}

static int
unique_cf_base( ConfigArgs *c )
{
	BackendDB *be = (BackendDB *)c->be;
	slap_overinst *on = (slap_overinst *)c->bi;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	int rc = ARG_BAD_CONF;

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		rc = 0;
		if ( legacy && legacy->uri && legacy->uri->dn.bv_val ) {
			rc = value_add_one ( &c->rvalue_vals,
					     &legacy->uri->dn );
			if ( rc ) return rc;
			rc = value_add_one ( &c->rvalue_nvals,
					     &legacy->uri->ndn );
			if ( rc ) return rc;
		}
		break;
	case LDAP_MOD_DELETE:
		assert ( legacy && legacy->uri && legacy->uri->dn.bv_val );
		rc = 0;
		ch_free ( legacy->uri->dn.bv_val );
		ch_free ( legacy->uri->ndn.bv_val );
		BER_BVZERO( &legacy->uri->dn );
		BER_BVZERO( &legacy->uri->ndn );
		if ( !legacy->uri->attrs ) {
			unique_free_domain_uri ( legacy->uri );
			legacy->uri = NULL;
		}
		if ( !legacy->uri && !private->legacy_strict_set ) {
			unique_free_domain ( legacy );
			private->legacy = legacy = NULL;
		}
		break;
	case LDAP_MOD_ADD:
	case SLAP_CONFIG_ADD:
		if ( domains ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "cannot set legacy attrs when URIs are present" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( be->be_nsuffix == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "suffix must be set" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( !dnIsSuffix ( &c->value_ndn,
				   &be->be_nsuffix[0] ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "dn is not a suffix of backend base" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( !legacy ) {
			unique_new_domain ( &private->legacy,
					    UNIQUE_DEFAULT_URI,
					    c );
			legacy = private->legacy;
		}
		if ( !legacy->uri )
			unique_new_domain_uri_basic ( &legacy->uri, c );
		ch_free ( legacy->uri->dn.bv_val );
		ch_free ( legacy->uri->ndn.bv_val );
		legacy->uri->dn = c->value_dn;
		legacy->uri->ndn = c->value_ndn;
		rc = 0;
		break;
	default:
		abort();
	}

	if ( rc ) {
		ch_free( c->value_dn.bv_val );
		BER_BVZERO( &c->value_dn );
		ch_free( c->value_ndn.bv_val );
		BER_BVZERO( &c->value_ndn );
	}

	return rc;
}

static int
unique_cf_attrs( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	unique_attrs *new_attrs = NULL;
	unique_attrs *attr, *next_attr, *reverse_attrs;
	unique_attrs **attrp;
	int rc = ARG_BAD_CONF;
	int i;

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		if ( legacy
		     && (c->type == UNIQUE_IGNORE) == legacy->ignore
		     && legacy->uri )
			for ( attr = legacy->uri->attrs;
			      attr;
			      attr = attr->next )
				value_add_one( &c->rvalue_vals,
					       &attr->attr->ad_cname );
		rc = 0;
		break;
	case LDAP_MOD_DELETE:
		if ( legacy
		     && (c->type == UNIQUE_IGNORE) == legacy->ignore
		     && legacy->uri
		     && legacy->uri->attrs) {
			if ( c->valx < 0 ) { /* delete all */
				for ( attr = legacy->uri->attrs;
				      attr;
				      attr = next_attr ) {
					next_attr = attr->next;
					ch_free ( attr );
				}
				legacy->uri->attrs = NULL;
			} else { /* delete by index */
				attrp = &legacy->uri->attrs;
				for ( i=0; i < c->valx; ++i )
					attrp = &(*attrp)->next;
				attr = *attrp;
				*attrp = attr->next;
				ch_free (attr);
			}
			if ( !legacy->uri->attrs
			     && !legacy->uri->dn.bv_val ) {
				unique_free_domain_uri ( legacy->uri );
				legacy->uri = NULL;
			}
			if ( !legacy->uri && !private->legacy_strict_set ) {
				unique_free_domain ( legacy );
				private->legacy = legacy = NULL;
			}
		}
		rc = 0;
		break;
	case LDAP_MOD_ADD:
	case SLAP_CONFIG_ADD:
		if ( domains ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "cannot set legacy attrs when URIs are present" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( legacy
		     && legacy->uri
		     && legacy->uri->attrs
		     && (c->type == UNIQUE_IGNORE) != legacy->ignore ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "cannot set both attrs and ignore-attrs" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( !legacy ) {
			unique_new_domain ( &private->legacy,
					    UNIQUE_DEFAULT_URI,
					    c );
			legacy = private->legacy;
		}
		if ( !legacy->uri )
			unique_new_domain_uri_basic ( &legacy->uri, c );
		rc = 0;
		for ( i=1; c->argv[i]; ++i ) {
			AttributeDescription * ad = NULL;
			const char * text;
			if ( slap_str2ad ( c->argv[i], &ad, &text )
			     == LDAP_SUCCESS) {

				attr = ch_calloc ( 1,
					sizeof ( unique_attrs ) );
				attr->attr = ad;
				attr->next = new_attrs;
				new_attrs = attr;
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					  "unique: attribute: %s: %s",
					  c->argv[i], text );
				for ( attr = new_attrs;
				      attr;
				      attr=next_attr ) {
					next_attr = attr->next;
					ch_free ( attr );
				}
				rc = ARG_BAD_CONF;
				break;
			}
		}
		if ( rc ) break;

		/* (nconc legacy->uri->attrs (nreverse new_attrs)) */
		reverse_attrs = NULL;
		for ( attr = new_attrs;
		      attr;
		      attr = next_attr ) {
			next_attr = attr->next;
			attr->next = reverse_attrs;
			reverse_attrs = attr;
		}
		for ( attrp = &legacy->uri->attrs;
		      *attrp;
		      attrp = &(*attrp)->next ) ;
		*attrp = reverse_attrs;

		legacy->ignore = ( c->type == UNIQUE_IGNORE );
		break;
	default:
		abort();
	}

	if ( rc ) {
		Debug ( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s: %s\n", c->log, c->cr_msg, 0 );
	}
	return rc;
}

static int
unique_cf_strict( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	int rc = ARG_BAD_CONF;

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		/* We process the boolean manually instead of using
		 * ARG_ON_OFF so that we can three-state it;
		 * olcUniqueStrict is either TRUE, FALSE, or missing,
		 * and missing is necessary to add olcUniqueURIs...
		 */
		if ( private->legacy_strict_set ) {
			struct berval bv;
			bv.bv_val = legacy->strict ? "TRUE" : "FALSE";
			bv.bv_len = legacy->strict ?
				STRLENOF("TRUE") :
				STRLENOF("FALSE");
			value_add_one ( &c->rvalue_vals, &bv );
		}
		rc = 0;
		break;
	case LDAP_MOD_DELETE:
		if ( legacy ) {
			legacy->strict = 0;
			if ( ! legacy->uri ) {
				unique_free_domain ( legacy );
				private->legacy = NULL;
			}
		}
		private->legacy_strict_set = 0;
		rc = 0;
		break;
	case LDAP_MOD_ADD:
	case SLAP_CONFIG_ADD:
		if ( domains ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "cannot set legacy attrs when URIs are present" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( ! legacy ) {
			unique_new_domain ( &private->legacy,
					    UNIQUE_DEFAULT_URI,
					    c );
			legacy = private->legacy;
		}
		/* ... not using ARG_ON_OFF makes this necessary too */
		assert ( c->argc == 2 );
		legacy->strict = (strcasecmp ( c->argv[1], "TRUE" ) == 0);
		private->legacy_strict_set = 1;
		rc = 0;
		break;
	default:
		abort();
	}

	return rc;
}

static int
unique_cf_uri( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	unique_domain *domain = NULL, **domainp = NULL;
	int rc = ARG_BAD_CONF;
	int i;

	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		for ( domain = domains;
		      domain;
		      domain = domain->next ) {
			rc = value_add_one ( &c->rvalue_vals,
					     &domain->domain_spec );
			if ( rc ) break;
		}
		break;
	case LDAP_MOD_DELETE:
		if ( c->valx < 0 ) { /* delete them all! */
			unique_free_domain ( domains );
			private->domains = NULL;
		} else { /* delete just one */
			domainp = &private->domains;
			for ( i=0; i < c->valx && *domainp; ++i )
				domainp = &(*domainp)->next;

			/* If *domainp is null, we walked off the end
			 * of the list.  This happens when back-config
			 * and the overlay are out-of-sync, like when
			 * rejecting changes before ITS#4752 gets
			 * fixed.
			 *
			 * This should never happen, but will appear
			 * if you backport this version of
			 * slapo-unique without the config-undo fixes
			 *
			 * test024 Will hit this case in such a
			 * situation.
			 */
			assert (*domainp != NULL);

			domain = *domainp;
			*domainp = domain->next;
			domain->next = NULL;
			unique_free_domain ( domain );
		}
		rc = 0;
		break;

	case SLAP_CONFIG_ADD: /* fallthrough */
	case LDAP_MOD_ADD:
		if ( legacy ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				  "cannot set Uri when legacy attrs are present" );
			Debug ( LDAP_DEBUG_CONFIG, "unique config: %s\n",
				c->cr_msg, NULL, NULL );
			rc = ARG_BAD_CONF;
			break;
		}
		rc = 0;
		if ( c->line ) rc = unique_new_domain ( &domain, c->line, c );
		else rc = unique_new_domain ( &domain, c->argv[1], c );
		if ( rc ) break;
		assert ( domain->next == NULL );
		for ( domainp = &private->domains;
		      *domainp;
		      domainp = &(*domainp)->next ) ;
		*domainp = domain;

		break;

	default:
		abort ();
	}

	return rc;
}

/*
** allocate new unique_data;
** initialize, copy basedn;
** store in on_bi.bi_private;
**
*/

static int
unique_db_init(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	unique_data **privatep = (unique_data **) &on->on_bi.bi_private;

	Debug(LDAP_DEBUG_TRACE, "==> unique_db_init\n", 0, 0, 0);

	*privatep = ch_calloc ( 1, sizeof ( unique_data ) );

	return 0;
}

static int
unique_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	unique_data **privatep = (unique_data **) &on->on_bi.bi_private;
	unique_data *private = *privatep;

	Debug(LDAP_DEBUG_TRACE, "==> unique_db_destroy\n", 0, 0, 0);

	if ( private ) {
		unique_domain *domains = private->domains;
		unique_domain *legacy = private->legacy;

		unique_free_domain ( domains );
		unique_free_domain ( legacy );
		ch_free ( private );
		*privatep = NULL;
	}

	return 0;
}

static int
unique_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	Debug(LDAP_DEBUG_TRACE, "unique_open: overlay initialized\n", 0, 0, 0);

	return 0;
}


/*
** Leave unique_data but wipe out config
**
*/

static int
unique_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on	= (slap_overinst *) be->bd_info;
	unique_data **privatep = (unique_data **) &on->on_bi.bi_private;
	unique_data *private = *privatep;

	Debug(LDAP_DEBUG_TRACE, "==> unique_close\n", 0, 0, 0);

	if ( private ) {
		unique_domain *domains = private->domains;
		unique_domain *legacy = private->legacy;

		unique_free_domain ( domains );
		unique_free_domain ( legacy );
		memset ( private, 0, sizeof ( unique_data ) );
	}

	return ( 0 );
}


/*
** search callback
**	if this is a REP_SEARCH, count++;
**
*/

static int count_attr_cb(
	Operation *op,
	SlapReply *rs
)
{
	unique_counter *uc;

	/* because you never know */
	if(!op || !rs) return(0);

	/* Only search entries are interesting */
	if(rs->sr_type != REP_SEARCH) return(0);

	uc = op->o_callback->sc_private;

	/* Ignore the current entry */
	if ( dn_match( uc->ndn, &rs->sr_entry->e_nname )) return(0);

	Debug(LDAP_DEBUG_TRACE, "==> count_attr_cb <%s>\n",
		rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);

	uc->count++;

	return(0);
}

/* count the length of one attribute ad
 * (and all of its values b)
 * in the proposed filter
 */
static int
count_filter_len(
	unique_domain *domain,
	unique_domain_uri *uri,
	AttributeDescription *ad,
	BerVarray b
)
{
	unique_attrs *attr;
	int i;
	int ks = 0;

	while ( !is_at_operational( ad->ad_type ) ) {
		if ( uri->attrs ) {
			for ( attr = uri->attrs; attr; attr = attr->next ) {
				if ( ad == attr->attr ) {
					break;
				}
			}
			if ( ( domain->ignore && attr )
			     || (!domain->ignore && !attr )) {
				break;
			}
		}
		if ( b && b[0].bv_val ) {
			for (i = 0; b[i].bv_val; i++ ) {
				/* note: make room for filter escaping... */
				ks += ( 3 * b[i].bv_len ) + ad->ad_cname.bv_len + STRLENOF( "(=)" );
			}
		} else if ( domain->strict ) {
			ks += ad->ad_cname.bv_len + STRLENOF( "(=*)" );	/* (attr=*) */
		}
		break;
	}

	return ks;
}

static char *
build_filter(
	unique_domain *domain,
	unique_domain_uri *uri,
	AttributeDescription *ad,
	BerVarray b,
	char *kp,
	int ks,
	void *ctx
)
{
	unique_attrs *attr;
	int i;

	while ( !is_at_operational( ad->ad_type ) ) {
		if ( uri->attrs ) {
			for ( attr = uri->attrs; attr; attr = attr->next ) {
				if ( ad == attr->attr ) {
					break;
				}
			}
			if ( ( domain->ignore && attr )
			     || (!domain->ignore && !attr )) {
				break;
			}
		}
		if ( b && b[0].bv_val ) {
			for ( i = 0; b[i].bv_val; i++ ) {
				struct berval	bv;
				int len;

				ldap_bv2escaped_filter_value_x( &b[i], &bv, 1, ctx );
				if (!b[i].bv_len)
					bv.bv_val = b[i].bv_val;
				len = snprintf( kp, ks, "(%s=%s)", ad->ad_cname.bv_val, bv.bv_val );
				assert( len >= 0 && len < ks );
				kp += len;
				if ( bv.bv_val != b[i].bv_val ) {
					ber_memfree_x( bv.bv_val, ctx );
				}
			}
		} else if ( domain->strict ) {
			int len;
			len = snprintf( kp, ks, "(%s=*)", ad->ad_cname.bv_val );
			assert( len >= 0 && len < ks );
			kp += len;
		}
		break;
	}
	return kp;
}

static int
unique_search(
	Operation *op,
	Operation *nop,
	struct berval * dn,
	int scope,
	SlapReply *rs,
	struct berval *key
)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	SlapReply nrs = { REP_RESULT };
	slap_callback cb = { NULL, NULL, NULL, NULL }; /* XXX */
	unique_counter uq = { NULL, 0 };
	int rc;

	Debug(LDAP_DEBUG_TRACE, "==> unique_search %s\n", key->bv_val, 0, 0);

	nop->ors_filter = str2filter_x(nop, key->bv_val);
	if(nop->ors_filter == NULL) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, LDAP_OTHER,
			"unique_search invalid filter");
		return(rs->sr_err);
	}

	nop->ors_filterstr = *key;

	cb.sc_response	= (slap_response*)count_attr_cb;
	cb.sc_private	= &uq;
	nop->o_callback	= &cb;
	nop->o_tag	= LDAP_REQ_SEARCH;
	nop->ors_scope	= scope;
	nop->ors_deref	= LDAP_DEREF_NEVER;
	nop->ors_limit	= NULL;
	nop->ors_slimit	= SLAP_NO_LIMIT;
	nop->ors_tlimit	= SLAP_NO_LIMIT;
	nop->ors_attrs	= slap_anlist_no_attrs;
	nop->ors_attrsonly = 1;

	uq.ndn = &op->o_req_ndn;

	nop->o_req_ndn = *dn;
	nop->o_ndn = op->o_bd->be_rootndn;

	nop->o_bd = on->on_info->oi_origdb;
	rc = nop->o_bd->be_search(nop, &nrs);
	filter_free_x(nop, nop->ors_filter, 1);
	op->o_tmpfree( key->bv_val, op->o_tmpmemctx );

	if(rc != LDAP_SUCCESS && rc != LDAP_NO_SUCH_OBJECT) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, rc, "unique_search failed");
		return(rs->sr_err);
	}

	Debug(LDAP_DEBUG_TRACE, "=> unique_search found %d records\n", uq.count, 0, 0);

	if(uq.count) {
		op->o_bd->bd_info = (BackendInfo *) on->on_info;
		send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION,
			"some attributes not unique");
		return(rs->sr_err);
	}

	return(SLAP_CB_CONTINUE);
}

static int
unique_add(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	unique_domain *domain;
	Operation nop = *op;
	Attribute *a;
	char *key, *kp;
	struct berval bvkey;
	int rc = SLAP_CB_CONTINUE;

	Debug(LDAP_DEBUG_TRACE, "==> unique_add <%s>\n",
	      op->o_req_dn.bv_val, 0, 0);

	/* skip the checks if the operation has manageDsaIt control in it
	 * (for replication) */
	if ( op->o_managedsait > SLAP_CONTROL_IGNORED ) {
		Debug(LDAP_DEBUG_TRACE, "unique_add: administrative bypass, skipping\n", 0, 0, 0);
		return rc;
	}

	for ( domain = legacy ? legacy : domains;
	      domain;
	      domain = domain->next )
	{
		unique_domain_uri *uri;

		for ( uri = domain->uri;
		      uri;
		      uri = uri->next )
		{
			int len;
			int ks = 0;

			if ( uri->ndn.bv_val
			     && !dnIsSuffix( &op->o_req_ndn, &uri->ndn ))
				continue;

			if ( uri->f ) {
				if ( test_filter( NULL, op->ora_e, uri->f )
					== LDAP_COMPARE_FALSE )
				{
					Debug( LDAP_DEBUG_TRACE,
						"==> unique_add_skip<%s>\n",
						op->o_req_dn.bv_val, 0, 0 );
					continue;
				}
			}

			if(!(a = op->ora_e->e_attrs)) {
				op->o_bd->bd_info = (BackendInfo *) on->on_info;
				send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
						"unique_add() got null op.ora_e.e_attrs");
				rc = rs->sr_err;
				break;

			} else {
				for(; a; a = a->a_next) {
					ks += count_filter_len ( domain,
								 uri,
								 a->a_desc,
								 a->a_vals);
				}
			}

			/* skip this domain-uri if it isn't involved */
			if ( !ks ) continue;

			/* terminating NUL */
			ks += sizeof("(|)");

			if ( uri->filter.bv_val && uri->filter.bv_len )
				ks += uri->filter.bv_len + STRLENOF ("(&)");
			kp = key = op->o_tmpalloc(ks, op->o_tmpmemctx);

			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf (kp, ks, "(&%s", uri->filter.bv_val);
				assert( len >= 0 && len < ks );
				kp += len;
			}
			len = snprintf(kp, ks - (kp - key), "(|");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;

			for(a = op->ora_e->e_attrs; a; a = a->a_next)
				kp = build_filter(domain,
						  uri,
						  a->a_desc,
						  a->a_vals,
						  kp,
						  ks - ( kp - key ),
						  op->o_tmpmemctx);

			len = snprintf(kp, ks - (kp - key), ")");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;
			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf(kp, ks - (kp - key), ")");
				assert( len >= 0 && len < ks - (kp - key) );
				kp += len;
			}
			bvkey.bv_val = key;
			bvkey.bv_len = kp - key;

			rc = unique_search ( op,
					     &nop,
					     uri->ndn.bv_val ?
					     &uri->ndn :
					     &op->o_bd->be_nsuffix[0],
					     uri->scope,
					     rs,
					     &bvkey);

			if ( rc != SLAP_CB_CONTINUE ) break;
		}
		if ( rc != SLAP_CB_CONTINUE ) break;
	}

	return rc;
}


static int
unique_modify(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	unique_domain *domain;
	Operation nop = *op;
	Modifications *m;
	char *key, *kp;
	struct berval bvkey;
	int rc = SLAP_CB_CONTINUE;

	Debug(LDAP_DEBUG_TRACE, "==> unique_modify <%s>\n",
	      op->o_req_dn.bv_val, 0, 0);

	/* skip the checks if the operation has manageDsaIt control in it
	 * (for replication) */
	if ( op->o_managedsait > SLAP_CONTROL_IGNORED ) {
		Debug(LDAP_DEBUG_TRACE, "unique_modify: administrative bypass, skipping\n", 0, 0, 0);
		return rc;
	}

	for ( domain = legacy ? legacy : domains;
	      domain;
	      domain = domain->next )
	{
		unique_domain_uri *uri;

		for ( uri = domain->uri;
		      uri;
		      uri = uri->next )
		{
			int len;
			int ks = 0;

			if ( uri->ndn.bv_val
			     && !dnIsSuffix( &op->o_req_ndn, &uri->ndn ))
				continue;

			if ( !(m = op->orm_modlist) ) {
				op->o_bd->bd_info = (BackendInfo *) on->on_info;
				send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
						"unique_modify() got null op.orm_modlist");
				rc = rs->sr_err;
				break;

			} else
				for ( ; m; m = m->sml_next)
					if ( (m->sml_op & LDAP_MOD_OP)
					     != LDAP_MOD_DELETE )
						ks += count_filter_len
							( domain,
							  uri,
							  m->sml_desc,
							  m->sml_values);

			/* skip this domain-uri if it isn't involved */
			if ( !ks ) continue;

			/* terminating NUL */
			ks += sizeof("(|)");

			if ( uri->filter.bv_val && uri->filter.bv_len )
				ks += uri->filter.bv_len + STRLENOF ("(&)");
			kp = key = op->o_tmpalloc(ks, op->o_tmpmemctx);

			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf(kp, ks, "(&%s", uri->filter.bv_val);
				assert( len >= 0 && len < ks );
				kp += len;
			}
			len = snprintf(kp, ks - (kp - key), "(|");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;

			for(m = op->orm_modlist; m; m = m->sml_next)
				if ( (m->sml_op & LDAP_MOD_OP)
				     != LDAP_MOD_DELETE )
					kp = build_filter ( domain,
							    uri,
							    m->sml_desc,
							    m->sml_values,
							    kp,
							    ks - (kp - key),
							    op->o_tmpmemctx );

			len = snprintf(kp, ks - (kp - key), ")");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;
			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf (kp, ks - (kp - key), ")");
				assert( len >= 0 && len < ks - (kp - key) );
				kp += len;
			}
			bvkey.bv_val = key;
			bvkey.bv_len = kp - key;

			rc = unique_search ( op,
					     &nop,
					     uri->ndn.bv_val ?
					     &uri->ndn :
					     &op->o_bd->be_nsuffix[0],
					     uri->scope,
					     rs,
					     &bvkey);

			if ( rc != SLAP_CB_CONTINUE ) break;
		}
		if ( rc != SLAP_CB_CONTINUE ) break;
	}

	return rc;
}


static int
unique_modrdn(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	unique_data *private = (unique_data *) on->on_bi.bi_private;
	unique_domain *domains = private->domains;
	unique_domain *legacy = private->legacy;
	unique_domain *domain;
	Operation nop = *op;
	char *key, *kp;
	struct berval bvkey;
	LDAPRDN	newrdn;
	struct berval bv[2];
	int rc = SLAP_CB_CONTINUE;

	Debug(LDAP_DEBUG_TRACE, "==> unique_modrdn <%s> <%s>\n",
		op->o_req_dn.bv_val, op->orr_newrdn.bv_val, 0);

	/* skip the checks if the operation has manageDsaIt control in it
	 * (for replication) */
	if ( op->o_managedsait > SLAP_CONTROL_IGNORED ) {
		Debug(LDAP_DEBUG_TRACE, "unique_modrdn: administrative bypass, skipping\n", 0, 0, 0);
		return rc;
	}

	for ( domain = legacy ? legacy : domains;
	      domain;
	      domain = domain->next )
	{
		unique_domain_uri *uri;

		for ( uri = domain->uri;
		      uri;
		      uri = uri->next )
		{
			int i, len;
			int ks = 0;

			if ( uri->ndn.bv_val
			     && !dnIsSuffix( &op->o_req_ndn, &uri->ndn )
			     && (!op->orr_nnewSup
				 || !dnIsSuffix( op->orr_nnewSup, &uri->ndn )))
				continue;

			if ( ldap_bv2rdn_x ( &op->oq_modrdn.rs_newrdn,
					     &newrdn,
					     (char **)&rs->sr_text,
					     LDAP_DN_FORMAT_LDAP,
					     op->o_tmpmemctx ) ) {
				op->o_bd->bd_info = (BackendInfo *) on->on_info;
				send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
						"unknown type(s) used in RDN");
				rc = rs->sr_err;
				break;
			}

			rc = SLAP_CB_CONTINUE;
			for ( i=0; newrdn[i]; i++) {
				AttributeDescription *ad = NULL;
				if ( slap_bv2ad( &newrdn[i]->la_attr, &ad, &rs->sr_text )) {
					ldap_rdnfree_x( newrdn, op->o_tmpmemctx );
					rs->sr_err = LDAP_INVALID_SYNTAX;
					send_ldap_result( op, rs );
					rc = rs->sr_err;
					break;
				}
				newrdn[i]->la_private = ad;
			}
			if ( rc != SLAP_CB_CONTINUE ) break;

			bv[1].bv_val = NULL;
			bv[1].bv_len = 0;

			for ( i=0; newrdn[i]; i++ ) {
				bv[0] = newrdn[i]->la_value;
				ks += count_filter_len ( domain,
							 uri,
							 newrdn[i]->la_private,
							 bv);
			}

			/* skip this domain if it isn't involved */
			if ( !ks ) continue;

			/* terminating NUL */
			ks += sizeof("(|)");

			if ( uri->filter.bv_val && uri->filter.bv_len )
				ks += uri->filter.bv_len + STRLENOF ("(&)");
			kp = key = op->o_tmpalloc(ks, op->o_tmpmemctx);

			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf(kp, ks, "(&%s", uri->filter.bv_val);
				assert( len >= 0 && len < ks );
				kp += len;
			}
			len = snprintf(kp, ks - (kp - key), "(|");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;

			for ( i=0; newrdn[i]; i++) {
				bv[0] = newrdn[i]->la_value;
				kp = build_filter ( domain,
						    uri,
						    newrdn[i]->la_private,
						    bv,
						    kp,
						    ks - (kp - key ),
						    op->o_tmpmemctx);
			}

			len = snprintf(kp, ks - (kp - key), ")");
			assert( len >= 0 && len < ks - (kp - key) );
			kp += len;
			if ( uri->filter.bv_val && uri->filter.bv_len ) {
				len = snprintf (kp, ks - (kp - key), ")");
				assert( len >= 0 && len < ks - (kp - key) );
				kp += len;
			}
			bvkey.bv_val = key;
			bvkey.bv_len = kp - key;

			rc = unique_search ( op,
					     &nop,
					     uri->ndn.bv_val ?
					     &uri->ndn :
					     &op->o_bd->be_nsuffix[0],
					     uri->scope,
					     rs,
					     &bvkey);

			if ( rc != SLAP_CB_CONTINUE ) break;
		}
		if ( rc != SLAP_CB_CONTINUE ) break;
	}

	return rc;
}

/*
** init_module is last so the symbols resolve "for free" --
** it expects to be called automagically during dynamic module initialization
*/

int
unique_initialize()
{
	int rc;

	/* statically declared just after the #includes at top */
	memset (&unique, 0, sizeof(unique));

	unique.on_bi.bi_type = "unique";
	unique.on_bi.bi_db_init = unique_db_init;
	unique.on_bi.bi_db_destroy = unique_db_destroy;
	unique.on_bi.bi_db_open = unique_open;
	unique.on_bi.bi_db_close = unique_close;
	unique.on_bi.bi_op_add = unique_add;
	unique.on_bi.bi_op_modify = unique_modify;
	unique.on_bi.bi_op_modrdn = unique_modrdn;

	unique.on_bi.bi_cf_ocs = uniqueocs;
	rc = config_register_schema( uniquecfg, uniqueocs );
	if ( rc ) return rc;

	return(overlay_register(&unique));
}

#if SLAPD_OVER_UNIQUE == SLAPD_MOD_DYNAMIC && defined(PIC)
int init_module(int argc, char *argv[]) {
	return unique_initialize();
}
#endif

#endif /* SLAPD_OVER_UNIQUE */
