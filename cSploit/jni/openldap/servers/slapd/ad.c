/* ad.c - routines for dealing with attribute descriptions */
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
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"

static struct berval bv_no_attrs = BER_BVC( LDAP_NO_ATTRS );
static struct berval bv_all_user_attrs = BER_BVC( "*" );
static struct berval bv_all_operational_attrs = BER_BVC( "+" );

static AttributeName anlist_no_attrs[] = {
	{ BER_BVC( LDAP_NO_ATTRS ), NULL, 0, NULL },
	{ BER_BVNULL, NULL, 0, NULL }
};

static AttributeName anlist_all_user_attributes[] = {
	{ BER_BVC( LDAP_ALL_USER_ATTRIBUTES ), NULL, 0, NULL },
	{ BER_BVNULL, NULL, 0, NULL }
};

static AttributeName anlist_all_operational_attributes[] = {
	{ BER_BVC( LDAP_ALL_OPERATIONAL_ATTRIBUTES ), NULL, 0, NULL },
	{ BER_BVNULL, NULL, 0, NULL }
};

static AttributeName anlist_all_attributes[] = {
	{ BER_BVC( LDAP_ALL_USER_ATTRIBUTES ), NULL, 0, NULL },
	{ BER_BVC( LDAP_ALL_OPERATIONAL_ATTRIBUTES ), NULL, 0, NULL },
	{ BER_BVNULL, NULL, 0, NULL }
};

AttributeName *slap_anlist_no_attrs = anlist_no_attrs;
AttributeName *slap_anlist_all_user_attributes = anlist_all_user_attributes;
AttributeName *slap_anlist_all_operational_attributes = anlist_all_operational_attributes;
AttributeName *slap_anlist_all_attributes = anlist_all_attributes;

struct berval * slap_bv_no_attrs = &bv_no_attrs;
struct berval * slap_bv_all_user_attrs = &bv_all_user_attrs;
struct berval * slap_bv_all_operational_attrs = &bv_all_operational_attrs;

typedef struct Attr_option {
	struct berval name;	/* option name or prefix */
	int           prefix;	/* NAME is a tag and range prefix */
} Attr_option;

static Attr_option lang_option = { BER_BVC("lang-"), 1 };

/* Options sorted by name, and number of options */
static Attr_option *options = &lang_option;
static int option_count = 1;

static int msad_range_hack = 0;

static int ad_count;

static Attr_option *ad_find_option_definition( const char *opt, int optlen );

int ad_keystring(
	struct berval *bv )
{
	ber_len_t i;

	if( !AD_LEADCHAR( bv->bv_val[0] ) ) {
		return 1;
	}

	for( i=1; i<bv->bv_len; i++ ) {
		if( !AD_CHAR( bv->bv_val[i] )) {
			if ( msad_range_hack && bv->bv_val[i] == '=' )
				continue;
			return 1;
		}
	}
	return 0;
}

void ad_destroy( AttributeDescription *ad )
{
	AttributeDescription *n;

	for (; ad != NULL; ad = n) {
		n = ad->ad_next;
		ldap_memfree( ad );
	}
}

/* Is there an AttributeDescription for this type that uses these tags? */
AttributeDescription * ad_find_tags(
	AttributeType *type,
	struct berval *tags )
{
	AttributeDescription *ad;

	ldap_pvt_thread_mutex_lock( &type->sat_ad_mutex );
	for (ad = type->sat_ad; ad; ad=ad->ad_next)
	{
		if (ad->ad_tags.bv_len == tags->bv_len &&
			!strcasecmp(ad->ad_tags.bv_val, tags->bv_val))
			break;
	}
	ldap_pvt_thread_mutex_unlock( &type->sat_ad_mutex );
	return ad;
}

int slap_str2ad(
	const char *str,
	AttributeDescription **ad,
	const char **text )
{
	struct berval bv;
	bv.bv_val = (char *) str;
	bv.bv_len = strlen( str );

	return slap_bv2ad( &bv, ad, text );
}

static char *strchrlen(
	const char *beg, 
	const char *end,
	const char ch, 
	int *len )
{
	const char *p;

	for( p=beg; *p && p < end; p++ ) {
		if( *p == ch ) {
			*len = p - beg;
			return (char *) p;
		}
	}

	*len = p - beg;
	return NULL;
}

int slap_bv2ad(
	struct berval *bv,
	AttributeDescription **ad,
	const char **text )
{
	int rtn = LDAP_UNDEFINED_TYPE;
	AttributeDescription desc, *d2;
	char *name, *options, *optn;
	char *opt, *next;
	int ntags;
	int tagslen;

	/* hardcoded limits for speed */
#define MAX_TAGGING_OPTIONS 128
	struct berval tags[MAX_TAGGING_OPTIONS+1];
#define MAX_TAGS_LEN 1024
	char tagbuf[MAX_TAGS_LEN];

	assert( ad != NULL );
	assert( *ad == NULL ); /* temporary */

	if( bv == NULL || BER_BVISNULL( bv ) || BER_BVISEMPTY( bv ) ) {
		*text = "empty AttributeDescription";
		return rtn;
	}

	/* make sure description is IA5 */
	if( ad_keystring( bv ) ) {
		*text = "AttributeDescription contains inappropriate characters";
		return rtn;
	}

	/* find valid base attribute type; parse in place */
	desc.ad_cname = *bv;
	desc.ad_flags = 0;
	BER_BVZERO( &desc.ad_tags );
	name = bv->bv_val;
	options = ber_bvchr( bv, ';' );
	if ( options != NULL && (unsigned) ( options - name ) < bv->bv_len ) {
		/* don't go past the end of the berval! */
		desc.ad_cname.bv_len = options - name;
	} else {
		options = NULL;
	}
	desc.ad_type = at_bvfind( &desc.ad_cname );
	if( desc.ad_type == NULL ) {
		*text = "attribute type undefined";
		return rtn;
	}

	if( is_at_operational( desc.ad_type ) && options != NULL ) {
		*text = "operational attribute with options undefined";
		return rtn;
	}

	/*
	 * parse options in place
	 */
	ntags = 0;
	tagslen = 0;
	optn = bv->bv_val + bv->bv_len;

	for( opt=options; opt != NULL; opt=next ) {
		int optlen;
		opt++; 
		next = strchrlen( opt, optn, ';', &optlen );

		if( optlen == 0 ) {
			*text = "zero length option is invalid";
			return rtn;
		
		} else if ( optlen == STRLENOF("binary") &&
			strncasecmp( opt, "binary", STRLENOF("binary") ) == 0 )
		{
			/* binary option */
			if( slap_ad_is_binary( &desc ) ) {
				*text = "option \"binary\" specified multiple times";
				return rtn;
			}

			if( !slap_syntax_is_binary( desc.ad_type->sat_syntax )) {
				/* not stored in binary, disallow option */
				*text = "option \"binary\" not supported with type";
				return rtn;
			}

			desc.ad_flags |= SLAP_DESC_BINARY;
			continue;

		} else if ( ad_find_option_definition( opt, optlen ) ) {
			int i;

			if( opt[optlen-1] == '-' ||
				( opt[optlen-1] == '=' && msad_range_hack )) {
				desc.ad_flags |= SLAP_DESC_TAG_RANGE;
			}

			if( ntags >= MAX_TAGGING_OPTIONS ) {
				*text = "too many tagging options";
				return rtn;
			}

			/*
			 * tags should be presented in sorted order,
			 * so run the array in reverse.
			 */
			for( i=ntags-1; i>=0; i-- ) {
				int rc;

				rc = strncasecmp( opt, tags[i].bv_val,
					(unsigned) optlen < tags[i].bv_len
						? (unsigned) optlen : tags[i].bv_len );

				if( rc == 0 && (unsigned)optlen == tags[i].bv_len ) {
					/* duplicate (ignore) */
					goto done;

				} else if ( rc > 0 ||
					( rc == 0 && (unsigned)optlen > tags[i].bv_len ))
				{
					AC_MEMCPY( &tags[i+2], &tags[i+1],
						(ntags-i-1)*sizeof(struct berval) );
					tags[i+1].bv_val = opt;
					tags[i+1].bv_len = optlen;
					goto done;
				}
			}

			if( ntags ) {
				AC_MEMCPY( &tags[1], &tags[0],
					ntags*sizeof(struct berval) );
			}
			tags[0].bv_val = opt;
			tags[0].bv_len = optlen;

done:;
			tagslen += optlen + 1;
			ntags++;

		} else {
			*text = "unrecognized option";
			return rtn;
		}
	}

	if( ntags > 0 ) {
		int i;

		if( tagslen > MAX_TAGS_LEN ) {
			*text = "tagging options too long";
			return rtn;
		}

		desc.ad_tags.bv_val = tagbuf;
		tagslen = 0;

		for( i=0; i<ntags; i++ ) {
			AC_MEMCPY( &desc.ad_tags.bv_val[tagslen],
				tags[i].bv_val, tags[i].bv_len );

			tagslen += tags[i].bv_len;
			desc.ad_tags.bv_val[tagslen++] = ';';
		}

		desc.ad_tags.bv_val[--tagslen] = '\0';
		desc.ad_tags.bv_len = tagslen;
	}

	/* see if a matching description is already cached */
	for (d2 = desc.ad_type->sat_ad; d2; d2=d2->ad_next) {
		if( d2->ad_flags != desc.ad_flags ) {
			continue;
		}
		if( d2->ad_tags.bv_len != desc.ad_tags.bv_len ) {
			continue;
		}
		if( d2->ad_tags.bv_len == 0 ) {
			break;
		}
		if( strncasecmp( d2->ad_tags.bv_val, desc.ad_tags.bv_val,
			desc.ad_tags.bv_len ) == 0 )
		{
			break;
		}
	}

	/* Not found, add new one */
	while (d2 == NULL) {
		size_t dlen = 0;
		ldap_pvt_thread_mutex_lock( &desc.ad_type->sat_ad_mutex );
		/* check again now that we've locked */
		for (d2 = desc.ad_type->sat_ad; d2; d2=d2->ad_next) {
			if (d2->ad_flags != desc.ad_flags)
				continue;
			if (d2->ad_tags.bv_len != desc.ad_tags.bv_len)
				continue;
			if (d2->ad_tags.bv_len == 0)
				break;
			if (strncasecmp(d2->ad_tags.bv_val, desc.ad_tags.bv_val,
				desc.ad_tags.bv_len) == 0)
				break;
		}
		if (d2) {
			ldap_pvt_thread_mutex_unlock( &desc.ad_type->sat_ad_mutex );
			break;
		}

		/* Allocate a single contiguous block. If there are no
		 * options, we just need space for the AttrDesc structure.
		 * Otherwise, we need to tack on the full name length +
		 * options length, + maybe tagging options length again.
		 */
		if (desc.ad_tags.bv_len || desc.ad_flags != SLAP_DESC_NONE) {
			dlen = desc.ad_type->sat_cname.bv_len + 1;
			if (desc.ad_tags.bv_len) {
				dlen += 1 + desc.ad_tags.bv_len;
			}
			if ( slap_ad_is_binary( &desc ) ) {
				dlen += 1 + STRLENOF(";binary") + desc.ad_tags.bv_len;
			}
		}

		d2 = ch_malloc(sizeof(AttributeDescription) + dlen);
		d2->ad_next = NULL;
		d2->ad_type = desc.ad_type;
		d2->ad_flags = desc.ad_flags;
		d2->ad_cname.bv_len = desc.ad_type->sat_cname.bv_len;
		d2->ad_tags.bv_len = desc.ad_tags.bv_len;
		ldap_pvt_thread_mutex_lock( &ad_index_mutex );
		d2->ad_index = ++ad_count;
		ldap_pvt_thread_mutex_unlock( &ad_index_mutex );

		if (dlen == 0) {
			d2->ad_cname.bv_val = d2->ad_type->sat_cname.bv_val;
			d2->ad_tags.bv_val = NULL;
		} else {
			char *cp, *op, *lp;
			int j;
			d2->ad_cname.bv_val = (char *)(d2+1);
			strcpy(d2->ad_cname.bv_val, d2->ad_type->sat_cname.bv_val);
			cp = d2->ad_cname.bv_val + d2->ad_cname.bv_len;
			if( slap_ad_is_binary( &desc ) ) {
				op = cp;
				lp = NULL;
				if( desc.ad_tags.bv_len ) {
					lp = desc.ad_tags.bv_val;
					while( strncasecmp(lp, "binary", STRLENOF("binary")) < 0
					       && (lp = strchr( lp, ';' )) != NULL )
						++lp;
					if( lp != desc.ad_tags.bv_val ) {
						*cp++ = ';';
						j = (lp
						     ? (unsigned) (lp - desc.ad_tags.bv_val - 1)
						     : strlen( desc.ad_tags.bv_val ));
						cp = lutil_strncopy(cp, desc.ad_tags.bv_val, j);
					}
				}
				cp = lutil_strcopy(cp, ";binary");
				if( lp != NULL ) {
					*cp++ = ';';
					cp = lutil_strcopy(cp, lp);
				}
				d2->ad_cname.bv_len = cp - d2->ad_cname.bv_val;
				if( desc.ad_tags.bv_len )
					ldap_pvt_str2lower(op);
				j = 1;
			} else {
				j = 0;
			}
			if( desc.ad_tags.bv_len ) {
				lp = d2->ad_cname.bv_val + d2->ad_cname.bv_len + j;
				if ( j == 0 )
					*lp++ = ';';
				d2->ad_tags.bv_val = lp;
				strcpy(lp, desc.ad_tags.bv_val);
				ldap_pvt_str2lower(lp);
				if( j == 0 )
					d2->ad_cname.bv_len += 1 + desc.ad_tags.bv_len;
			}
		}
		/* Add new desc to list. We always want the bare Desc with
		 * no options to stay at the head of the list, assuming
		 * that one will be used most frequently.
		 */
		if (desc.ad_type->sat_ad == NULL || dlen == 0) {
			d2->ad_next = desc.ad_type->sat_ad;
			desc.ad_type->sat_ad = d2;
		} else {
			d2->ad_next = desc.ad_type->sat_ad->ad_next;
			desc.ad_type->sat_ad->ad_next = d2;
		}
		ldap_pvt_thread_mutex_unlock( &desc.ad_type->sat_ad_mutex );
	}

	if( *ad == NULL ) {
		*ad = d2;
	} else {
		**ad = *d2;
	}

	return LDAP_SUCCESS;
}

static int is_ad_subtags(
	struct berval *subtagsbv, 
	struct berval *suptagsbv )
{
	const char *suptags, *supp, *supdelimp, *supn;
	const char *subtags, *subp, *subdelimp, *subn;
	int  suplen, sublen;

	subtags =subtagsbv->bv_val;
	suptags =suptagsbv->bv_val;
	subn = subtags + subtagsbv->bv_len;
	supn = suptags + suptagsbv->bv_len;

	for( supp=suptags ; supp; supp=supdelimp ) {
		supdelimp = strchrlen( supp, supn, ';', &suplen );
		if( supdelimp ) supdelimp++;

		for( subp=subtags ; subp; subp=subdelimp ) {
			subdelimp = strchrlen( subp, subn, ';', &sublen );
			if( subdelimp ) subdelimp++;

			if ( suplen > sublen
				 ? ( suplen-1 == sublen && supp[suplen-1] == '-'
					 && strncmp( supp, subp, sublen ) == 0 )
				 : ( ( suplen == sublen || supp[suplen-1] == '-' )
					 && strncmp( supp, subp, suplen ) == 0 ) )
			{
				goto match;
			}
		}

		return 0;
match:;
	}
	return 1;
}

int is_ad_subtype(
	AttributeDescription *sub,
	AttributeDescription *super
)
{
	AttributeType *a;
	int lr;

	for ( a = sub->ad_type; a; a=a->sat_sup ) {
		if ( a == super->ad_type ) break;
	}
	if( !a ) {
		return 0;
	}

	/* ensure sub does support all flags of super */
	lr = sub->ad_tags.bv_len ? SLAP_DESC_TAG_RANGE : 0;
	if(( super->ad_flags & ( sub->ad_flags | lr )) != super->ad_flags ) {
		return 0;
	}

	/* check for tagging options */
	if ( super->ad_tags.bv_len == 0 )
		return 1;
	if ( sub->ad_tags.bv_len == 0 )
		return 0;

	return is_ad_subtags( &sub->ad_tags, &super->ad_tags );
}

int ad_inlist(
	AttributeDescription *desc,
	AttributeName *attrs )
{
	if (! attrs ) return 0;

	for( ; attrs->an_name.bv_val; attrs++ ) {
		AttributeType *a;
		ObjectClass *oc;
		
		if ( attrs->an_desc ) {
			int lr;

			if ( desc == attrs->an_desc ) {
				return 1;
			}

			/*
			 * EXTENSION: if requested description is preceeded by
			 * a '-' character, do not match on subtypes.
			 */
			if ( attrs->an_name.bv_val[0] == '-' ) {
				continue;
			}
			
			/* Is this a subtype of the requested attr? */
			for (a = desc->ad_type; a; a=a->sat_sup) {
				if ( a == attrs->an_desc->ad_type )
					break;
			}
			if ( !a ) {
				continue;
			}
			/* Does desc support all the requested flags? */
			lr = desc->ad_tags.bv_len ? SLAP_DESC_TAG_RANGE : 0;
			if(( attrs->an_desc->ad_flags & (desc->ad_flags | lr))
				!= attrs->an_desc->ad_flags ) {
				continue;
			}
			/* Do the descs have compatible tags? */
			if ( attrs->an_desc->ad_tags.bv_len == 0 ) {
				return 1;
			}
			if ( desc->ad_tags.bv_len == 0) {
				continue;
			}
			if ( is_ad_subtags( &desc->ad_tags,
				&attrs->an_desc->ad_tags ) ) {
				return 1;
			}
			continue;
		}

		if ( ber_bvccmp( &attrs->an_name, '*' ) ) {
			if ( !is_at_operational( desc->ad_type ) ) {
				return 1;
			}
			continue;
		}

		if ( ber_bvccmp( &attrs->an_name, '+' ) ) {
			if ( is_at_operational( desc->ad_type ) ) {
				return 1;
			}
			continue;
		}

		/*
		 * EXTENSION: see if requested description is @objectClass
		 * if so, return attributes which the class requires/allows
		 * else if requested description is !objectClass, return
		 * attributes which the class does not require/allow
		 */
		if ( !( attrs->an_flags & SLAP_AN_OCINITED )) {
			if( attrs->an_name.bv_val ) {
				switch( attrs->an_name.bv_val[0] ) {
				case '@': /* @objectClass */
				case '+': /* +objectClass (deprecated) */
				case '!': { /* exclude */
						struct berval ocname;
						ocname.bv_len = attrs->an_name.bv_len - 1;
						ocname.bv_val = &attrs->an_name.bv_val[1];
						oc = oc_bvfind( &ocname );
						if ( oc && attrs->an_name.bv_val[0] == '!' ) {
							attrs->an_flags |= SLAP_AN_OCEXCLUDE;
						} else {
							attrs->an_flags &= ~SLAP_AN_OCEXCLUDE;
						}
					} break;

				default: /* old (deprecated) way */
					oc = oc_bvfind( &attrs->an_name );
				}
				attrs->an_oc = oc;
			}
			attrs->an_flags |= SLAP_AN_OCINITED;
		}
		oc = attrs->an_oc;
		if( oc != NULL ) {
			if ( attrs->an_flags & SLAP_AN_OCEXCLUDE ) {
				if ( oc == slap_schema.si_oc_extensibleObject ) {
					/* extensibleObject allows the return of anything */
					return 0;
				}

				if( oc->soc_required ) {
					/* allow return of required attributes */
					int i;
				
   					for ( i = 0; oc->soc_required[i] != NULL; i++ ) {
						for (a = desc->ad_type; a; a=a->sat_sup) {
							if ( a == oc->soc_required[i] ) {
								return 0;
							}
						}
					}
				}

				if( oc->soc_allowed ) {
					/* allow return of allowed attributes */
					int i;
   					for ( i = 0; oc->soc_allowed[i] != NULL; i++ ) {
						for (a = desc->ad_type; a; a=a->sat_sup) {
							if ( a == oc->soc_allowed[i] ) {
								return 0;
							}
						}
					}
				}

				return 1;
			}
			
			if ( oc == slap_schema.si_oc_extensibleObject ) {
				/* extensibleObject allows the return of anything */
				return 1;
			}

			if( oc->soc_required ) {
				/* allow return of required attributes */
				int i;
				
   				for ( i = 0; oc->soc_required[i] != NULL; i++ ) {
					for (a = desc->ad_type; a; a=a->sat_sup) {
						if ( a == oc->soc_required[i] ) {
							return 1;
						}
					}
				}
			}

			if( oc->soc_allowed ) {
				/* allow return of allowed attributes */
				int i;
   				for ( i = 0; oc->soc_allowed[i] != NULL; i++ ) {
					for (a = desc->ad_type; a; a=a->sat_sup) {
						if ( a == oc->soc_allowed[i] ) {
							return 1;
						}
					}
				}
			}

		} else {
			const char      *text;

			/* give it a chance of being retrieved by a proxy... */
			(void)slap_bv2undef_ad( &attrs->an_name,
				&attrs->an_desc, &text,
				SLAP_AD_PROXIED|SLAP_AD_NOINSERT );
		}
	}

	return 0;
}


int slap_str2undef_ad(
	const char *str,
	AttributeDescription **ad,
	const char **text,
	unsigned flags )
{
	struct berval bv;
	bv.bv_val = (char *) str;
	bv.bv_len = strlen( str );

	return slap_bv2undef_ad( &bv, ad, text, flags );
}

int slap_bv2undef_ad(
	struct berval *bv,
	AttributeDescription **ad,
	const char **text,
	unsigned flags )
{
	AttributeDescription *desc;
	AttributeType *at;

	assert( ad != NULL );

	if( bv == NULL || bv->bv_len == 0 ) {
		*text = "empty AttributeDescription";
		return LDAP_UNDEFINED_TYPE;
	}

	/* make sure description is IA5 */
	if( ad_keystring( bv ) ) {
		*text = "AttributeDescription contains inappropriate characters";
		return LDAP_UNDEFINED_TYPE;
	}

	/* use the appropriate type */
	if ( flags & SLAP_AD_PROXIED ) {
		at = slap_schema.si_at_proxied;

	} else {
		at = slap_schema.si_at_undefined;
	}

	for( desc = at->sat_ad; desc; desc=desc->ad_next ) {
		if( desc->ad_cname.bv_len == bv->bv_len &&
		    !strcasecmp( desc->ad_cname.bv_val, bv->bv_val ) )
		{
		    	break;
		}
	}

	if( !desc ) {
		if ( flags & SLAP_AD_NOINSERT ) {
			*text = NULL;
			return LDAP_UNDEFINED_TYPE;
		}
	
		desc = ch_malloc(sizeof(AttributeDescription) + 1 +
			bv->bv_len);
		
		desc->ad_flags = SLAP_DESC_NONE;
		BER_BVZERO( &desc->ad_tags );

		desc->ad_cname.bv_len = bv->bv_len;
		desc->ad_cname.bv_val = (char *)(desc+1);
		strcpy(desc->ad_cname.bv_val, bv->bv_val);

		/* canonical to upper case */
		ldap_pvt_str2upper( desc->ad_cname.bv_val );

		/* shouldn't we protect this for concurrency? */
		desc->ad_type = at;
		desc->ad_index = 0;
		ldap_pvt_thread_mutex_lock( &ad_undef_mutex );
		desc->ad_next = desc->ad_type->sat_ad;
		desc->ad_type->sat_ad = desc;
		ldap_pvt_thread_mutex_unlock( &ad_undef_mutex );

		Debug( LDAP_DEBUG_ANY,
			"%s attributeDescription \"%s\" inserted.\n",
			( flags & SLAP_AD_PROXIED ) ? "PROXIED" : "UNKNOWN",
			desc->ad_cname.bv_val, 0 );
	}

	if( !*ad ) {
		*ad = desc;
	} else {
		**ad = *desc;
	}

	return LDAP_SUCCESS;
}

AttributeDescription *
slap_bv2tmp_ad(
	struct berval *bv,
	void *memctx )
{
	AttributeDescription *ad =
		 slap_sl_mfuncs.bmf_malloc( sizeof(AttributeDescription) +
			bv->bv_len + 1, memctx );

	ad->ad_cname.bv_val = (char *)(ad+1);
	strncpy( ad->ad_cname.bv_val, bv->bv_val, bv->bv_len+1 );
	ad->ad_cname.bv_len = bv->bv_len;
	ad->ad_flags = SLAP_DESC_TEMPORARY;
	ad->ad_type = slap_schema.si_at_undefined;

	return ad;
}

static int
undef_promote(
	AttributeType	*at,
	char		*name,
	AttributeType	*nat )
{
	AttributeDescription	**u_ad, **n_ad;

	/* Get to last ad on the new type */
	for ( n_ad = &nat->sat_ad; *n_ad; n_ad = &(*n_ad)->ad_next ) ;

	for ( u_ad = &at->sat_ad; *u_ad; ) {
		struct berval	bv;

		ber_str2bv( name, 0, 0, &bv );

		/* remove iff undef == name or undef == name;tag */
		if ( (*u_ad)->ad_cname.bv_len >= bv.bv_len
			&& strncasecmp( (*u_ad)->ad_cname.bv_val, bv.bv_val, bv.bv_len ) == 0
			&& ( (*u_ad)->ad_cname.bv_val[ bv.bv_len ] == '\0'
				|| (*u_ad)->ad_cname.bv_val[ bv.bv_len ] == ';' ) )
		{
			AttributeDescription	*tmp = *u_ad;

			*u_ad = (*u_ad)->ad_next;

			tmp->ad_type = nat;
			tmp->ad_next = NULL;
			/* ad_cname was contiguous, no leak here */
			tmp->ad_cname = nat->sat_cname;
			ldap_pvt_thread_mutex_lock( &ad_index_mutex );
			tmp->ad_index = ++ad_count;
			ldap_pvt_thread_mutex_unlock( &ad_index_mutex );
			*n_ad = tmp;
			n_ad = &tmp->ad_next;
		} else {
			u_ad = &(*u_ad)->ad_next;
		}
	}

	return 0;
}

int
slap_ad_undef_promote(
	char *name,
	AttributeType *at )
{
	int	rc;

	ldap_pvt_thread_mutex_lock( &ad_undef_mutex );

	rc = undef_promote( slap_schema.si_at_undefined, name, at );
	if ( rc == 0 ) {
		rc = undef_promote( slap_schema.si_at_proxied, name, at );
	}

	ldap_pvt_thread_mutex_unlock( &ad_undef_mutex );

	return rc;
}

int
an_find(
    AttributeName *a,
    struct berval *s
)
{
	if( a == NULL ) return 0;

	for ( ; a->an_name.bv_val; a++ ) {
		if ( a->an_name.bv_len != s->bv_len) continue;
		if ( strcasecmp( s->bv_val, a->an_name.bv_val ) == 0 ) {
			return( 1 );
		}
	}

	return( 0 );
}

/*
 * Convert a delimited string into a list of AttributeNames; add
 * on to an existing list if it was given.  If the string is not
 * a valid attribute name, if a '-' is prepended it is skipped
 * and the remaining name is tried again; if a '@' (or '+') is
 * prepended, an objectclass name is searched instead; if a '!'
 * is prepended, the objectclass name is negated.
 * 
 * NOTE: currently, if a valid attribute name is not found, the
 * same string is also checked as valid objectclass name; however,
 * this behavior is deprecated.
 */
AttributeName *
str2anlist( AttributeName *an, char *in, const char *brkstr )
{
	char	*str;
	char	*s;
	char	*lasts;
	int	i, j;
	const char *text;
	AttributeName *anew;

	/* find last element in list */
	i = 0;
	if ( an != NULL ) {
		for ( i = 0; !BER_BVISNULL( &an[ i ].an_name ) ; i++)
			;
	}
	
	/* protect the input string from strtok */
	str = ch_strdup( in );

	/* Count words in string */
	j = 1;
	for ( s = str; *s; s++ ) {
		if ( strchr( brkstr, *s ) != NULL ) {
			j++;
		}
	}

	an = ch_realloc( an, ( i + j + 1 ) * sizeof( AttributeName ) );
	anew = an + i;
	for ( s = ldap_pvt_strtok( str, brkstr, &lasts );
		s != NULL;
		s = ldap_pvt_strtok( NULL, brkstr, &lasts ) )
	{
		/* put a stop mark */
		BER_BVZERO( &anew[1].an_name );

		anew->an_desc = NULL;
		anew->an_oc = NULL;
		anew->an_flags = 0;
		ber_str2bv(s, 0, 1, &anew->an_name);
		slap_bv2ad(&anew->an_name, &anew->an_desc, &text);
		if ( !anew->an_desc ) {
			switch( anew->an_name.bv_val[0] ) {
			case '-': {
					struct berval adname;
					adname.bv_len = anew->an_name.bv_len - 1;
					adname.bv_val = &anew->an_name.bv_val[1];
					slap_bv2ad(&adname, &anew->an_desc, &text);
					if ( !anew->an_desc ) {
						goto reterr;
					}
				} break;

			case '@':
			case '+': /* (deprecated) */
			case '!': {
					struct berval ocname;
					ocname.bv_len = anew->an_name.bv_len - 1;
					ocname.bv_val = &anew->an_name.bv_val[1];
					anew->an_oc = oc_bvfind( &ocname );
					if ( !anew->an_oc ) {
						goto reterr;
					}

					if ( anew->an_name.bv_val[0] == '!' ) {
						anew->an_flags |= SLAP_AN_OCEXCLUDE;
					}
				} break;

			default:
				/* old (deprecated) way */
				anew->an_oc = oc_bvfind( &anew->an_name );
				if ( !anew->an_oc ) {
					goto reterr;
				}
			}
		}
		anew->an_flags |= SLAP_AN_OCINITED;
		anew++;
	}

	BER_BVZERO( &anew->an_name );
	free( str );
	return( an );

reterr:
	anlist_free( an, 1, NULL );

	/*
	 * overwrites input string
	 * on error!
	 */
	strcpy( in, s );
	free( str );
	return NULL;
}

void
anlist_free( AttributeName *an, int freename, void *ctx )
{
	if ( an == NULL ) {
		return;
	}

	if ( freename ) {
		int	i;

		for ( i = 0; an[i].an_name.bv_val; i++ ) {
			ber_memfree_x( an[i].an_name.bv_val, ctx );
		}
	}

	ber_memfree_x( an, ctx );
}

char **anlist2charray_x( AttributeName *an, int dup, void *ctx )
{
    char **attrs;
    int i;
                                                                                
    if ( an != NULL ) {
        for ( i = 0; !BER_BVISNULL( &an[i].an_name ); i++ )
            ;
		attrs = (char **) slap_sl_malloc( (i + 1) * sizeof(char *), ctx );
        for ( i = 0; !BER_BVISNULL( &an[i].an_name ); i++ ) {
			if ( dup )
	            attrs[i] = ch_strdup( an[i].an_name.bv_val );
			else
	            attrs[i] = an[i].an_name.bv_val;
        }
        attrs[i] = NULL;
    } else {
        attrs = NULL;
    }
                                                                                
    return attrs;
}

char **anlist2charray( AttributeName *an, int dup )
{
	return anlist2charray_x( an, dup, NULL );
}

char**
anlist2attrs( AttributeName * anlist )
{
	int i, j, k = 0;
	int n;
	char **attrs;
	ObjectClass *oc;

	if ( anlist == NULL )
		return NULL;

	for ( i = 0; anlist[i].an_name.bv_val; i++ ) {
		if ( ( oc = anlist[i].an_oc ) ) {
			for ( j = 0; oc->soc_required && oc->soc_required[j]; j++ ) ;
			k += j;
			for ( j = 0; oc->soc_allowed && oc->soc_allowed[j]; j++ ) ;
			k += j;
		}
	}

	if ( i == 0 )
		return NULL;
                                                                                
	attrs = anlist2charray( anlist, 1 );
                                                                                
	n = i;
                                                                                
	if ( k )
		attrs = (char **) ch_realloc( attrs, (i + k + 1) * sizeof( char * ));

   	for ( i = 0; anlist[i].an_name.bv_val; i++ ) {
		if ( ( oc = anlist[i].an_oc ) ) {
			for ( j = 0; oc->soc_required && oc->soc_required[j]; j++ ) {
				attrs[n++] = ch_strdup(
								oc->soc_required[j]->sat_cname.bv_val );
			}
			for ( j = 0; oc->soc_allowed && oc->soc_allowed[j]; j++ ) {
				attrs[n++] = ch_strdup(
								oc->soc_allowed[j]->sat_cname.bv_val );
			}
		}
	}
	
	if ( attrs )
		attrs[n] = NULL;

	i = 0;
	while ( attrs && attrs[i] ) {
		if ( *attrs[i] == '@' ) {
			ch_free( attrs[i] );
			for ( j = i; attrs[j]; j++ ) {
				attrs[j] = attrs[j+1];
			}
		} else {
			i++;
		}
	}

	for ( i = 0; attrs && attrs[i]; i++ ) {
		j = i + 1;
		while ( attrs && attrs[j] ) {
			if ( !strcmp( attrs[i], attrs[j] )) {
				ch_free( attrs[j] );
				for ( k = j; attrs && attrs[k]; k++ ) {
					attrs[k] = attrs[k+1];
				}
			} else {
				j++;
			}
		}
	}

	if ( i != n )
		attrs = (char **) ch_realloc( attrs, (i+1) * sizeof( char * ));

	return attrs;
}

#define LBUFSIZ	80
AttributeName*
file2anlist( AttributeName *an, const char *fname, const char *brkstr )
{
	FILE	*fp;
	char	*line = NULL;
	char	*lcur = NULL;
	char	*c;
	size_t	lmax = LBUFSIZ;

	fp = fopen( fname, "r" );
	if ( fp == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"get_attrs_from_file: failed to open attribute list file "
			"\"%s\": %s\n", fname, strerror(errno), 0 );
		return NULL;
	}

	lcur = line = (char *) ch_malloc( lmax );
	if ( !line ) {
		Debug( LDAP_DEBUG_ANY,
			"get_attrs_from_file: could not allocate memory\n",
			0, 0, 0 );
		fclose(fp);
		return NULL;
	}

	while ( fgets( lcur, LBUFSIZ, fp ) != NULL ) {
		if ( ( c = strchr( lcur, '\n' ) ) ) {
			if ( c == line ) {
				*c = '\0';
			} else if ( *(c-1) == '\r' ) {
				*(c-1) = '\0';
			} else {
				*c = '\0';
			}
		} else {
			lmax += LBUFSIZ;
			line = (char *) ch_realloc( line, lmax );
			if ( !line ) {
				Debug( LDAP_DEBUG_ANY,
					"get_attrs_from_file: could not allocate memory\n",
					0, 0, 0 );
				fclose(fp);
				return NULL;
			}
			lcur = line + strlen( line );
			continue;
		}
		an = str2anlist( an, line, brkstr );
		if ( an == NULL )
			break;
		lcur = line;
	}
	ch_free( line );
	fclose(fp);
	return an;
}
#undef LBUFSIZ

/* Define an attribute option. */
int
ad_define_option( const char *name, const char *fname, int lineno )
{
	int i;
	unsigned int optlen;

	if ( options == &lang_option ) {
		options = NULL;
		option_count = 0;
	}
	if ( name == NULL )
		return 0;

	optlen = 0;
	do {
		if ( !DESC_CHAR( name[optlen] ) ) {
			/* allow trailing '=', same as '-' */
			if ( name[optlen] == '=' && !name[optlen+1] ) {
				msad_range_hack = 1;
				continue;
			}
			Debug( LDAP_DEBUG_ANY,
			       "%s: line %d: illegal option name \"%s\"\n",
				    fname, lineno, name );
			return 1;
		}
	} while ( name[++optlen] );

	options = ch_realloc( options,
		(option_count+1) * sizeof(Attr_option) );

	if ( strcasecmp( name, "binary" ) == 0
	     || ad_find_option_definition( name, optlen ) ) {
		Debug( LDAP_DEBUG_ANY,
		       "%s: line %d: option \"%s\" is already defined\n",
		       fname, lineno, name );
		return 1;
	}

	for ( i = option_count; i; --i ) {
		if ( strcasecmp( name, options[i-1].name.bv_val ) >= 0 )
			break;
		options[i] = options[i-1];
	}

	options[i].name.bv_val = ch_strdup( name );
	options[i].name.bv_len = optlen;
	options[i].prefix = (name[optlen-1] == '-') ||
 		(name[optlen-1] == '=');

	if ( i != option_count &&
	     options[i].prefix &&
	     optlen < options[i+1].name.bv_len &&
	     strncasecmp( name, options[i+1].name.bv_val, optlen ) == 0 ) {
			Debug( LDAP_DEBUG_ANY,
			       "%s: line %d: option \"%s\" overrides previous option\n",
				    fname, lineno, name );
			return 1;
	}

	option_count++;
	return 0;
}

void
ad_unparse_options( BerVarray *res )
{
	int i;
	for ( i = 0; i < option_count; i++ ) {
		value_add_one( res, &options[i].name );
	}
}

/* Find the definition of the option name or prefix matching the arguments */
static Attr_option *
ad_find_option_definition( const char *opt, int optlen )
{
	int top = 0, bot = option_count;
	while ( top < bot ) {
		int mid = (top + bot) / 2;
		int mlen = options[mid].name.bv_len;
		char *mname = options[mid].name.bv_val;
		int j;
		if ( optlen < mlen ) {
			j = strncasecmp( opt, mname, optlen ) - 1;
		} else {
			j = strncasecmp( opt, mname, mlen );
			if ( j==0 && (optlen==mlen || options[mid].prefix) )
				return &options[mid];
		}
		if ( j < 0 )
			bot = mid;
		else
			top = mid + 1;
	}
	return NULL;
}

MatchingRule *ad_mr(
	AttributeDescription *ad,
	unsigned usage )
{
	switch( usage & SLAP_MR_TYPE_MASK ) {
	case SLAP_MR_NONE:
	case SLAP_MR_EQUALITY:
		return ad->ad_type->sat_equality;
		break;
	case SLAP_MR_ORDERING:
		return ad->ad_type->sat_ordering;
		break;
	case SLAP_MR_SUBSTR:
		return ad->ad_type->sat_substr;
		break;
	case SLAP_MR_EXT:
	default:
		assert( 0 /* ad_mr: bad usage */);
	}
	return NULL;
}
