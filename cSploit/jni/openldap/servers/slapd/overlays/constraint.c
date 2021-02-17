/* $OpenLDAP$ */
/* constraint.c - Overlay to constrain attributes to certain values */
/* 
 * Copyright 2003-2004 Hewlett-Packard Company
 * Copyright 2007 Emmanuel Dreyfus
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
/*
 * Authors: Neil Dunbar <neil.dunbar@hp.com>
 *			Emmannuel Dreyfus <manu@netbsd.org>
 */
#include "portable.h"

#ifdef SLAPD_OVER_CONSTRAINT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/regex.h>

#include "lutil.h"
#include "slap.h"
#include "config.h"

/*
 * This overlay limits the values which can be placed into an
 * attribute, over and above the limits placed by the schema.
 *
 * It traps only LDAP adds and modify commands (and only seeks to
 * control the add and modify value mods of a modify)
 */

#define REGEX_STR "regex"
#define URI_STR "uri"
#define SET_STR "set"
#define SIZE_STR "size"
#define COUNT_STR "count"

/*
 * Linked list of attribute constraints which we should enforce.
 * This is probably a sub optimal structure - some form of sorted
 * array would be better if the number of attributes contrained is
 * likely to be much bigger than 4 or 5. We stick with a list for
 * the moment.
 */

typedef struct constraint {
	struct constraint *ap_next;
	AttributeDescription **ap;

	LDAPURLDesc *restrict_lud;
	struct berval restrict_ndn;
	Filter *restrict_filter;
	struct berval restrict_val;

	regex_t *re;
	LDAPURLDesc *lud;
	int set;
	size_t size;
	size_t count;
	AttributeDescription **attrs;
	struct berval val; /* constraint value */
	struct berval dn;
	struct berval filter;
} constraint;

enum {
	CONSTRAINT_ATTRIBUTE = 1
};

static ConfigDriver constraint_cf_gen;

static ConfigTable constraintcfg[] = {
	{ "constraint_attribute", "attribute[list]> (regex|uri|set|size|count) <value> [<restrict URI>]",
	  4, 0, 0, ARG_MAGIC | CONSTRAINT_ATTRIBUTE, constraint_cf_gen,
	  "( OLcfgOvAt:13.1 NAME 'olcConstraintAttribute' "
	  "DESC 'constraint for list of attributes' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs constraintocs[] = {
	{ "( OLcfgOvOc:13.1 "
	  "NAME 'olcConstraintConfig' "
	  "DESC 'Constraint overlay configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcConstraintAttribute ) )",
	  Cft_Overlay, constraintcfg },
	{ NULL, 0, NULL }
};

static void
constraint_free( constraint *cp, int freeme )
{
	if (cp->restrict_lud)
		ldap_free_urldesc(cp->restrict_lud);
	if (!BER_BVISNULL(&cp->restrict_ndn))
		ch_free(cp->restrict_ndn.bv_val);
	if (cp->restrict_filter != NULL && cp->restrict_filter != slap_filter_objectClass_pres)
		filter_free(cp->restrict_filter);
	if (!BER_BVISNULL(&cp->restrict_val))
		ch_free(cp->restrict_val.bv_val);
	if (cp->re) {
		regfree(cp->re);
		ch_free(cp->re);
	}
	if (!BER_BVISNULL(&cp->val))
		ch_free(cp->val.bv_val);
	if (cp->lud)
		ldap_free_urldesc(cp->lud);
	if (cp->attrs)
		ch_free(cp->attrs);
	if (cp->ap)
		ch_free(cp->ap);
	if (freeme)
		ch_free(cp);
}

static int
constraint_cf_gen( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)(c->bi);
	constraint *cn = on->on_bi.bi_private, *cp;
	struct berval bv;
	int i, rc = 0;
	constraint ap = { NULL };
	const char *text = NULL;
	
	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE:
			for (cp=cn; cp; cp=cp->ap_next) {
				char *s;
				char *tstr = NULL;
				int quotes = 0;
				int j;
				size_t val;
				char val_buf[SLAP_TEXT_BUFLEN] = { '\0' };

				bv.bv_len = STRLENOF("  ");
				for (j = 0; cp->ap[j]; j++) {
					bv.bv_len += cp->ap[j]->ad_cname.bv_len;
				}

				/* room for commas */
				bv.bv_len += j - 1;

				if (cp->re) {
					tstr = REGEX_STR;
					quotes = 1;
				} else if (cp->lud) {
					tstr = URI_STR;
					quotes = 1;
				} else if (cp->set) {
					tstr = SET_STR;
					quotes = 1;
				} else if (cp->size) {
					tstr = SIZE_STR;
					val = cp->size;
				} else if (cp->count) {
					tstr = COUNT_STR;
					val = cp->count;
				}

				bv.bv_len += strlen(tstr);
				bv.bv_len += cp->val.bv_len + 2*quotes;

				if (cp->restrict_lud != NULL) {
					bv.bv_len += cp->restrict_val.bv_len + STRLENOF(" restrict=\"\"");
				}

				if (cp->count || cp->size) {
					int len = snprintf(val_buf, sizeof(val_buf), "%zd", val);
					if (len <= 0) {
						/* error */
						return -1;
					}
					bv.bv_len += len;
				}

				s = bv.bv_val = ch_malloc(bv.bv_len + 1);

				s = lutil_strncopy( s, cp->ap[0]->ad_cname.bv_val, cp->ap[0]->ad_cname.bv_len );
				for (j = 1; cp->ap[j]; j++) {
					*s++ = ',';
					s = lutil_strncopy( s, cp->ap[j]->ad_cname.bv_val, cp->ap[j]->ad_cname.bv_len );
				}
				*s++ = ' ';
				s = lutil_strcopy( s, tstr );
				*s++ = ' ';
				if (cp->count || cp->size) {
					s = lutil_strcopy( s, val_buf );
				} else {
					if ( quotes ) *s++ = '"';
					s = lutil_strncopy( s, cp->val.bv_val, cp->val.bv_len );
					if ( quotes ) *s++ = '"';
				}
				if (cp->restrict_lud != NULL) {
					s = lutil_strcopy( s, " restrict=\"" );
					s = lutil_strncopy( s, cp->restrict_val.bv_val, cp->restrict_val.bv_len );
					*s++ = '"';
				}
				*s = '\0';

				rc = value_add_one( &c->rvalue_vals, &bv );
				if (rc == LDAP_SUCCESS)
					rc = value_add_one( &c->rvalue_nvals, &bv );
				ch_free(bv.bv_val);
				if (rc) return rc;
			}
			break;
		default:
			abort();
			break;
		}
		break;
	case LDAP_MOD_DELETE:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE:
			if (!cn) break; /* nothing to do */
					
			if (c->valx < 0) {
				/* zap all constraints */
				while (cn) {
					cp = cn->ap_next;
					constraint_free( cn, 1 );
					cn = cp;
				}
						
				on->on_bi.bi_private = NULL;
			} else {
				constraint **cpp;
						
				/* zap constraint numbered 'valx' */
				for(i=0, cp = cn, cpp = &cn;
					(cp) && (i<c->valx);
					i++, cpp = &cp->ap_next, cp = *cpp);

				if (cp) {
					/* zap cp, and join cpp to cp->ap_next */
					*cpp = cp->ap_next;
					constraint_free( cp, 1 );
				}
				on->on_bi.bi_private = cn;
			}
			break;

		default:
			abort();
			break;
		}
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE: {
			int j;
			char **attrs = ldap_str2charray( c->argv[1], "," );

			for ( j = 0; attrs[j]; j++)
				/* just count */ ;
			ap.ap = ch_calloc( sizeof(AttributeDescription*), j + 1 );
			for ( j = 0; attrs[j]; j++) {
				if ( slap_str2ad( attrs[j], &ap.ap[j], &text ) ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s <%s>: %s\n", c->argv[0], attrs[j], text );
					rc = ARG_BAD_CONF;
					goto done;
				}
			}

			if ( strcasecmp( c->argv[2], REGEX_STR ) == 0) {
				int err;
			
				ap.re = ch_malloc( sizeof(regex_t) );
				if ((err = regcomp( ap.re,
					c->argv[3], REG_EXTENDED )) != 0) {
					char errmsg[1024];
							
					regerror( err, ap.re, errmsg, sizeof(errmsg) );
					ch_free(ap.re);
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s %s: Illegal regular expression \"%s\": Error %s",
						c->argv[0], c->argv[1], c->argv[3], errmsg);
					ap.re = NULL;
					rc = ARG_BAD_CONF;
					goto done;
				}
				ber_str2bv( c->argv[3], 0, 1, &ap.val );
			} else if ( strcasecmp( c->argv[2], SIZE_STR ) == 0 ) {
				size_t size;

				if ( ( size = atoi(c->argv[3]) ) != 0 )
					ap.size = size;	
			} else if ( strcasecmp( c->argv[2], COUNT_STR ) == 0 ) {
				size_t count;

				if ( ( count = atoi(c->argv[3]) ) != 0 )
					ap.count = count;	
			} else if ( strcasecmp( c->argv[2], URI_STR ) == 0 ) {
				int err;
			
				err = ldap_url_parse(c->argv[3], &ap.lud);
				if ( err != LDAP_URL_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s %s: Invalid URI \"%s\"",
						c->argv[0], c->argv[1], c->argv[3]);
					rc = ARG_BAD_CONF;
					goto done;
				}

				if (ap.lud->lud_host != NULL) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s %s: unsupported hostname in URI \"%s\"",
						c->argv[0], c->argv[1], c->argv[3]);
					ldap_free_urldesc(ap.lud);
					rc = ARG_BAD_CONF;
					goto done;
				}

				for ( i=0; ap.lud->lud_attrs[i]; i++);
				/* FIXME: This is worthless without at least one attr */
				if ( i ) {
					ap.attrs = ch_malloc( (i+1)*sizeof(AttributeDescription *));
					for ( i=0; ap.lud->lud_attrs[i]; i++) {
						ap.attrs[i] = NULL;
						if ( slap_str2ad( ap.lud->lud_attrs[i], &ap.attrs[i], &text ) ) {
							ch_free( ap.attrs );
							snprintf( c->cr_msg, sizeof( c->cr_msg ),
								"%s <%s>: %s\n", c->argv[0], ap.lud->lud_attrs[i], text );
							rc = ARG_BAD_CONF;
							goto done;
						}
					}
					ap.attrs[i] = NULL;
				}

				if (ap.lud->lud_dn == NULL) {
					ap.lud->lud_dn = ch_strdup("");
				} else {
					struct berval dn, ndn;

					ber_str2bv( ap.lud->lud_dn, 0, 0, &dn );
					if (dnNormalize( 0, NULL, NULL, &dn, &ndn, NULL ) ) {
						/* cleanup */
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"%s %s: URI %s DN normalization failed",
							c->argv[0], c->argv[1], c->argv[3] );
						Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
							   "%s: %s\n", c->log, c->cr_msg, 0 );
						rc = ARG_BAD_CONF;
						goto done;
					}
					ldap_memfree( ap.lud->lud_dn );
					ap.lud->lud_dn = ndn.bv_val;
				}

				if (ap.lud->lud_filter == NULL) {
					ap.lud->lud_filter = ch_strdup("objectClass=*");
				} else if ( ap.lud->lud_filter[0] == '(' ) {
					ber_len_t len = strlen( ap.lud->lud_filter );
					if ( ap.lud->lud_filter[len - 1] != ')' ) {
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"%s %s: invalid URI filter: %s",
							c->argv[0], c->argv[1], ap.lud->lud_filter );
						rc = ARG_BAD_CONF;
						goto done;
					}
					AC_MEMCPY( &ap.lud->lud_filter[0], &ap.lud->lud_filter[1], len - 2 );
					ap.lud->lud_filter[len - 2] = '\0';
				}

				ber_str2bv( c->argv[3], 0, 1, &ap.val );

			} else if ( strcasecmp( c->argv[2], SET_STR ) == 0 ) {
				ap.set = 1;
				ber_str2bv( c->argv[3], 0, 1, &ap.val );

			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%s %s: Unknown constraint type: %s",
					c->argv[0], c->argv[1], c->argv[2] );
				rc = ARG_BAD_CONF;
				goto done;
			}

			if ( c->argc > 4 ) {
				int argidx;

				for ( argidx = 4; argidx < c->argc; argidx++ ) {
					if ( strncasecmp( c->argv[argidx], "restrict=", STRLENOF("restrict=") ) == 0 ) {
						int err;
						char *arg = c->argv[argidx] + STRLENOF("restrict=");

						err = ldap_url_parse(arg, &ap.restrict_lud);
						if ( err != LDAP_URL_SUCCESS ) {
							snprintf( c->cr_msg, sizeof( c->cr_msg ),
								"%s %s: Invalid restrict URI \"%s\"",
								c->argv[0], c->argv[1], arg);
							rc = ARG_BAD_CONF;
							goto done;
						}

						if (ap.restrict_lud->lud_host != NULL) {
							snprintf( c->cr_msg, sizeof( c->cr_msg ),
								"%s %s: unsupported hostname in restrict URI \"%s\"",
								c->argv[0], c->argv[1], arg);
							rc = ARG_BAD_CONF;
							goto done;
						}

						if ( ap.restrict_lud->lud_attrs != NULL ) {
							if ( ap.restrict_lud->lud_attrs[0] != '\0' ) {
								snprintf( c->cr_msg, sizeof( c->cr_msg ),
									"%s %s: attrs not allowed in restrict URI %s\n",
									c->argv[0], c->argv[1], arg);
								rc = ARG_BAD_CONF;
								goto done;
							}
							ldap_memvfree((void *)ap.restrict_lud->lud_attrs);
							ap.restrict_lud->lud_attrs = NULL;
						}

						if (ap.restrict_lud->lud_dn != NULL) {
							if (ap.restrict_lud->lud_dn[0] == '\0') {
								ldap_memfree(ap.restrict_lud->lud_dn);
								ap.restrict_lud->lud_dn = NULL;

							} else {
								struct berval dn, ndn;
								int j;

								ber_str2bv(ap.restrict_lud->lud_dn, 0, 0, &dn);
								if (dnNormalize(0, NULL, NULL, &dn, &ndn, NULL)) {
									/* cleanup */
									snprintf( c->cr_msg, sizeof( c->cr_msg ),
										"%s %s: restrict URI %s DN normalization failed",
										c->argv[0], c->argv[1], arg );
									rc = ARG_BAD_CONF;
									goto done;
								}

								assert(c->be != NULL);
								if (c->be->be_nsuffix == NULL) {
									snprintf( c->cr_msg, sizeof( c->cr_msg ),
										"%s %s: restrict URI requires suffix",
										c->argv[0], c->argv[1] );
									rc = ARG_BAD_CONF;
									goto done;
								}

								for ( j = 0; !BER_BVISNULL(&c->be->be_nsuffix[j]); j++) {
									if (dnIsSuffix(&ndn, &c->be->be_nsuffix[j])) break;
								}

								if (BER_BVISNULL(&c->be->be_nsuffix[j])) {
									/* error */
									snprintf( c->cr_msg, sizeof( c->cr_msg ),
										"%s %s: restrict URI DN %s not within database naming context(s)",
										c->argv[0], c->argv[1], dn.bv_val );
									rc = ARG_BAD_CONF;
									goto done;
								}

								ap.restrict_ndn = ndn;
							}
						}

						if (ap.restrict_lud->lud_filter != NULL) {
							ap.restrict_filter = str2filter(ap.restrict_lud->lud_filter);
							if (ap.restrict_filter == NULL) {
								/* error */
								snprintf( c->cr_msg, sizeof( c->cr_msg ),
									"%s %s: restrict URI filter %s invalid",
									c->argv[0], c->argv[1], ap.restrict_lud->lud_filter );
								rc = ARG_BAD_CONF;
								goto done;
							}
						}

						ber_str2bv(c->argv[argidx] + STRLENOF("restrict="), 0, 1, &ap.restrict_val);

					} else {
						/* cleanup */
						snprintf( c->cr_msg, sizeof( c->cr_msg ),
							"%s %s: unrecognized arg #%d (%s)",
							c->argv[0], c->argv[1], argidx, c->argv[argidx] );
						rc = ARG_BAD_CONF;
						goto done;
					}
				}
			}

done:;
			if ( rc == LDAP_SUCCESS ) {
				constraint *a2 = ch_calloc( sizeof(constraint), 1 );
				a2->ap_next = on->on_bi.bi_private;
				a2->ap = ap.ap;
				a2->re = ap.re;
				a2->val = ap.val;
				a2->lud = ap.lud;
				a2->set = ap.set;
				a2->size = ap.size;
				a2->count = ap.count;
				if ( a2->lud ) {
					ber_str2bv(a2->lud->lud_dn, 0, 0, &a2->dn);
					ber_str2bv(a2->lud->lud_filter, 0, 0, &a2->filter);
				}
				a2->attrs = ap.attrs;
				a2->restrict_lud = ap.restrict_lud;
				a2->restrict_ndn = ap.restrict_ndn;
				a2->restrict_filter = ap.restrict_filter;
				a2->restrict_val = ap.restrict_val;
				on->on_bi.bi_private = a2;

			} else {
				Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
					   "%s: %s\n", c->log, c->cr_msg, 0 );
				constraint_free( &ap, 0 );
			}

			ldap_memvfree((void**)attrs);
			} break;
		default:
			abort();
			break;
		}
		break;
	default:
		abort();
	}

	return rc;
}

static int
constraint_uri_cb( Operation *op, SlapReply *rs ) 
{
	if(rs->sr_type == REP_SEARCH) {
		int *foundp = op->o_callback->sc_private;

		*foundp = 1;

		Debug(LDAP_DEBUG_TRACE, "==> constraint_uri_cb <%s>\n",
			rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);
	}
	return 0;
}

static int
constraint_violation( constraint *c, struct berval *bv, Operation *op )
{
	if ((!c) || (!bv)) return LDAP_SUCCESS;
	
	if ((c->re) &&
		(regexec(c->re, bv->bv_val, 0, NULL, 0) == REG_NOMATCH))
		return LDAP_CONSTRAINT_VIOLATION; /* regular expression violation */

	if ((c->size) && (bv->bv_len > c->size))
		return LDAP_CONSTRAINT_VIOLATION; /* size violation */

	if (c->lud) {
		Operation nop = *op;
		slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
		slap_callback cb;
		int i;
		int found = 0;
		int rc;
		size_t len;
		struct berval filterstr;
		char *ptr;

		cb.sc_next = NULL;
		cb.sc_response = constraint_uri_cb;
		cb.sc_cleanup = NULL;
		cb.sc_private = &found;

		nop.o_protocol = LDAP_VERSION3;
		nop.o_tag = LDAP_REQ_SEARCH;
		nop.o_time = slap_get_time();
		if (c->lud->lud_dn) {
			struct berval dn;

			ber_str2bv(c->lud->lud_dn, 0, 0, &dn);
			nop.o_req_dn = dn;
			nop.o_req_ndn = dn;
			nop.o_bd = select_backend(&nop.o_req_ndn, 1 );
			if (!nop.o_bd) {
				return LDAP_NO_SUCH_OBJECT; /* unexpected error */
			}
			if (!nop.o_bd->be_search) {
				return LDAP_OTHER; /* unexpected error */
			}
		} else {
			nop.o_req_dn = nop.o_bd->be_nsuffix[0];
			nop.o_req_ndn = nop.o_bd->be_nsuffix[0];
			nop.o_bd = on->on_info->oi_origdb;
		}
		nop.o_do_not_cache = 1;
		nop.o_callback = &cb;

		nop.ors_scope = c->lud->lud_scope;
		nop.ors_deref = LDAP_DEREF_NEVER;
		nop.ors_slimit = SLAP_NO_LIMIT;
		nop.ors_tlimit = SLAP_NO_LIMIT;
		nop.ors_limit = NULL;

		nop.ors_attrsonly = 0;
		nop.ors_attrs = slap_anlist_no_attrs;

		len = STRLENOF("(&(") + 
			  c->filter.bv_len +
			  STRLENOF(")(|");

		for (i = 0; c->attrs[i]; i++) {
			len += STRLENOF("(") +
				   c->attrs[i]->ad_cname.bv_len +
				   STRLENOF("=") + 
				   bv->bv_len +
				   STRLENOF(")");
		}

		len += STRLENOF("))");
		filterstr.bv_len = len;
		filterstr.bv_val = op->o_tmpalloc(len + 1, op->o_tmpmemctx);

		ptr = filterstr.bv_val +
			snprintf(filterstr.bv_val, len, "(&(%s)(|", c->lud->lud_filter);
		for (i = 0; c->attrs[i]; i++) {
			*ptr++ = '(';
			ptr = lutil_strcopy( ptr, c->attrs[i]->ad_cname.bv_val );
			*ptr++ = '=';
			ptr = lutil_strcopy( ptr, bv->bv_val );
			*ptr++ = ')';
		}
		*ptr++ = ')';
		*ptr++ = ')';
		*ptr++ = '\0';

		nop.ors_filterstr = filterstr;
		nop.ors_filter = str2filter_x(&nop, filterstr.bv_val);
		if ( nop.ors_filter == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"%s constraint_violation uri filter=\"%s\" invalid\n",
				op->o_log_prefix, filterstr.bv_val, 0 );
			rc = LDAP_OTHER;

		} else {
			SlapReply nrs = { REP_RESULT };

			Debug(LDAP_DEBUG_TRACE, 
				"==> constraint_violation uri filter = %s\n",
				filterstr.bv_val, 0, 0);

			rc = nop.o_bd->be_search( &nop, &nrs );
		
			Debug(LDAP_DEBUG_TRACE, 
				"==> constraint_violation uri rc = %d, found = %d\n",
				rc, found, 0);
		}
		op->o_tmpfree(filterstr.bv_val, op->o_tmpmemctx);

		if ((rc != LDAP_SUCCESS) && (rc != LDAP_NO_SUCH_OBJECT)) {
			return rc; /* unexpected error */
		}

		if (!found)
			return LDAP_CONSTRAINT_VIOLATION; /* constraint violation */
	}

	return LDAP_SUCCESS;
}

static char *
print_message( struct berval *errtext, AttributeDescription *a )
{
	char *ret;
	int sz;
	
	sz = errtext->bv_len + sizeof(" on ") + a->ad_cname.bv_len;
	ret = ch_malloc(sz);
	snprintf( ret, sz, "%s on %s", errtext->bv_val, a->ad_cname.bv_val );
	return ret;
}

static unsigned
constraint_count_attr(Entry *e, AttributeDescription *ad)
{
	struct Attribute *a;

	if ((a = attr_find(e->e_attrs, ad)) != NULL)
		return a->a_numvals;
	return 0;
}

static int
constraint_check_restrict( Operation *op, constraint *c, Entry *e )
{
	assert( c->restrict_lud != NULL );

	if ( c->restrict_lud->lud_dn != NULL ) {
		int diff = e->e_nname.bv_len - c->restrict_ndn.bv_len;

		if ( diff < 0 ) {
			return 0;
		}

		if ( c->restrict_lud->lud_scope == LDAP_SCOPE_BASE ) {
			return bvmatch( &e->e_nname, &c->restrict_ndn );
		}

		if ( !dnIsSuffix( &e->e_nname, &c->restrict_ndn ) ) {
			return 0;
		}

		if ( c->restrict_lud->lud_scope != LDAP_SCOPE_SUBTREE ) {
			struct berval pdn;

			if ( diff == 0 ) {
				return 0;
			}

			dnParent( &e->e_nname, &pdn );

			if ( c->restrict_lud->lud_scope == LDAP_SCOPE_ONELEVEL
				&& pdn.bv_len != c->restrict_ndn.bv_len )
			{
				return 0;
			}
		}
	}

	if ( c->restrict_filter != NULL ) {
		int rc;
		struct berval save_dn = op->o_dn, save_ndn = op->o_ndn;

		op->o_dn = op->o_bd->be_rootdn;
		op->o_ndn = op->o_bd->be_rootndn;
		rc = test_filter( op, e, c->restrict_filter );
		op->o_dn = save_dn;
		op->o_ndn = save_ndn;

		if ( rc != LDAP_COMPARE_TRUE ) {
			return 0;
		}
	}

	return 1;
}

static int
constraint_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Attribute *a;
	constraint *c = on->on_bi.bi_private, *cp;
	BerVarray b = NULL;
	int i;
	struct berval rsv = BER_BVC("add breaks constraint");
	int rc;
	char *msg = NULL;

	if (get_relax(op) || SLAPD_SYNC_IS_SYNCCONN( op->o_connid )) {
		return SLAP_CB_CONTINUE;
	}

	if ((a = op->ora_e->e_attrs) == NULL) {
		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
			"constraint_add: no attrs");
		return(rs->sr_err);
	}

	for(; a; a = a->a_next ) {
		/* we don't constrain operational attributes */
		if (is_at_operational(a->a_desc->ad_type)) continue;

		for(cp = c; cp; cp = cp->ap_next) {
			int j;
			for (j = 0; cp->ap[j]; j++) {
				if (cp->ap[j] == a->a_desc) break;
			}
			if (cp->ap[j] == NULL) continue;
			if ((b = a->a_vals) == NULL) continue;

			if (cp->restrict_lud != NULL && constraint_check_restrict(op, cp, op->ora_e) == 0) {
				continue;
			}

			Debug(LDAP_DEBUG_TRACE, 
				"==> constraint_add, "
				"a->a_numvals = %u, cp->count = %lu\n",
				a->a_numvals, (unsigned long) cp->count, 0);

			if ((cp->count != 0) && (a->a_numvals > cp->count)) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto add_violation;
			}

			for ( i = 0; b[i].bv_val; i++ ) {
				rc = constraint_violation( cp, &b[i], op );
				if ( rc ) {
					goto add_violation;
				}
			}

			if (cp->set && acl_match_set(&cp->val, op, op->ora_e, NULL) == 0) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto add_violation; /* constraint violation */
			}

		}
	}

	/* Default is to just fall through to the normal processing */
	return SLAP_CB_CONTINUE;

add_violation:
	op->o_bd->bd_info = (BackendInfo *)(on->on_info);
	if (rc == LDAP_CONSTRAINT_VIOLATION ) {
		msg = print_message( &rsv, a->a_desc );
	}
	send_ldap_error(op, rs, rc, msg );
	ch_free(msg);
	return (rs->sr_err);
}


static int
constraint_check_count_violation( Modifications *m, Entry *target_entry, constraint *cp )
{
	BerVarray b = NULL;
	unsigned ce = 0;
	unsigned ca;
	int j;

	for ( j = 0; cp->ap[j]; j++ ) {
		/* Get this attribute count */
		if ( target_entry )
			ce = constraint_count_attr( target_entry, cp->ap[j] );

		for( ; m; m = m->sml_next ) {
			if ( cp->ap[j] == m->sml_desc ) {
				ca = m->sml_numvals;
				switch ( m->sml_op ) {
				case LDAP_MOD_DELETE:
				case SLAP_MOD_SOFTDEL:
					if ( !ca || ca > ce ) {
						ce = 0;
					} else {
						/* No need to check for values' validity. Invalid values
						 * cause the whole transaction to die anyway. */
						ce -= ca;
					}
					break;

				case LDAP_MOD_ADD:
				case SLAP_MOD_SOFTADD:
					ce += ca;
					break;

				case LDAP_MOD_REPLACE:
					ce = ca;
					break;

#if 0
				/* TODO */
				case handle SLAP_MOD_ADD_IF_NOT_PRESENT:
#endif

				default:
					/* impossible! assert? */
					return 1;
				}

				Debug(LDAP_DEBUG_TRACE,
					"==> constraint_check_count_violation ce = %u, "
					"ca = %u, cp->count = %lu\n",
					ce, ca, (unsigned long) cp->count);
			}
		}
	}

	return ( ce > cp->count );
}

static int
constraint_update( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Backend *be = op->o_bd;
	constraint *c = on->on_bi.bi_private, *cp;
	Entry *target_entry = NULL, *target_entry_copy = NULL;
	Modifications *modlist, *m;
	BerVarray b = NULL;
	int i;
	struct berval rsv = BER_BVC("modify breaks constraint");
	int rc;
	char *msg = NULL;
	int is_v;

	if (get_relax(op) || SLAPD_SYNC_IS_SYNCCONN( op->o_connid )) {
		return SLAP_CB_CONTINUE;
	}

	switch ( op->o_tag ) {
	case LDAP_REQ_MODIFY:
		modlist = op->orm_modlist;
		break;

	case LDAP_REQ_MODRDN:
		modlist = op->orr_modlist;
		break;

	default:
		/* impossible! assert? */
		return LDAP_OTHER;
	}
	
	Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "constraint_update()\n", 0,0,0);
	if ((m = modlist) == NULL) {
		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
						"constraint_update() got null modlist");
		return(rs->sr_err);
	}

	op->o_bd = on->on_info->oi_origdb;
	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &target_entry );
	op->o_bd = be;

	/* let the backend send the error */
	if ( target_entry == NULL )
		return SLAP_CB_CONTINUE;

	/* Do we need to count attributes? */
	for(cp = c; cp; cp = cp->ap_next) {
		if (cp->count != 0) {
			if (rc != 0 || target_entry == NULL) {
				Debug(LDAP_DEBUG_TRACE, 
					"==> constraint_update rc = %d DN=\"%s\"%s\n",
					rc, op->o_req_ndn.bv_val,
					target_entry ? "" : " not found" );
				if ( rc == 0 ) 
					rc = LDAP_CONSTRAINT_VIOLATION;
				goto mod_violation;
			}

			if (cp->restrict_lud && constraint_check_restrict(op, cp, target_entry) == 0) {
				continue;
			}

			is_v = constraint_check_count_violation(m, target_entry, cp);

			Debug(LDAP_DEBUG_TRACE,
				"==> constraint_update is_v: %d\n", is_v, 0, 0);

			if (is_v) {
				rc = LDAP_CONSTRAINT_VIOLATION;
				goto mod_violation;
			}
		}
	}

	rc = LDAP_CONSTRAINT_VIOLATION;
	for(;m; m = m->sml_next) {
		unsigned ce = 0;

		if (is_at_operational( m->sml_desc->ad_type )) continue;

		if ((( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_ADD) &&
			(( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_REPLACE) &&
			(( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_DELETE))
			continue;
		/* we only care about ADD and REPLACE modifications */
		/* and DELETE are used to track attribute count */
		if ((( b = m->sml_values ) == NULL ) || (b[0].bv_val == NULL))
			continue;

		for(cp = c; cp; cp = cp->ap_next) {
			int j;
			for (j = 0; cp->ap[j]; j++) {
				if (cp->ap[j] == m->sml_desc) {
					break;
				}
			}
			if (cp->ap[j] == NULL) continue;

			if (cp->restrict_lud != NULL && constraint_check_restrict(op, cp, target_entry) == 0) {
				continue;
			}

			/* DELETE are to be ignored beyond this point */
			if (( m->sml_op & LDAP_MOD_OP ) == LDAP_MOD_DELETE)
				continue;

			for ( i = 0; b[i].bv_val; i++ ) {
				rc = constraint_violation( cp, &b[i], op );
				if ( rc ) {
					goto mod_violation;
				}
			}

			if (cp->set && target_entry) {
				if (target_entry_copy == NULL) {
					Modifications *ml;

					target_entry_copy = entry_dup(target_entry);

					/* if rename, set the new entry's name
					 * (in normalized form only) */
					if ( op->o_tag == LDAP_REQ_MODRDN ) {
						struct berval pdn, ndn = BER_BVNULL;

						if ( op->orr_nnewSup ) {
							pdn = *op->orr_nnewSup;

						} else {
							dnParent( &target_entry_copy->e_nname, &pdn );
						}

						build_new_dn( &ndn, &pdn, &op->orr_nnewrdn, NULL ); 

						ber_memfree( target_entry_copy->e_nname.bv_val );
						target_entry_copy->e_nname = ndn;
						ber_bvreplace( &target_entry_copy->e_name, &ndn );
					}

					/* apply modifications, in an attempt
					 * to estimate what the entry would
					 * look like in case all modifications
					 * pass */
					for ( ml = modlist; ml; ml = ml->sml_next ) {
						Modification *mod = &ml->sml_mod;
						const char *text;
						char textbuf[SLAP_TEXT_BUFLEN];
						size_t textlen = sizeof(textbuf);
						int err;

						switch ( mod->sm_op ) {
						case LDAP_MOD_ADD:
							err = modify_add_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
							break;

						case LDAP_MOD_DELETE:
							err = modify_delete_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
							break;

						case LDAP_MOD_REPLACE:
							err = modify_replace_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
							break;

						case LDAP_MOD_INCREMENT:
							err = modify_increment_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
							break;

						case SLAP_MOD_SOFTADD:
 							mod->sm_op = LDAP_MOD_ADD;
							err = modify_add_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
 							mod->sm_op = SLAP_MOD_SOFTADD;
 							if ( err == LDAP_TYPE_OR_VALUE_EXISTS ) {
 								err = LDAP_SUCCESS;
 							}
							break;

						case SLAP_MOD_SOFTDEL:
 							mod->sm_op = LDAP_MOD_ADD;
							err = modify_delete_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
 							mod->sm_op = SLAP_MOD_SOFTDEL;
 							if ( err == LDAP_NO_SUCH_ATTRIBUTE ) {
 								err = LDAP_SUCCESS;
 							}
							break;

						case SLAP_MOD_ADD_IF_NOT_PRESENT:
							if ( attr_find( target_entry_copy->e_attrs, mod->sm_desc ) ) {
								err = LDAP_SUCCESS;
								break;
							}
 							mod->sm_op = LDAP_MOD_ADD;
							err = modify_add_values( target_entry_copy,
								mod, get_permissiveModify(op),
								&text, textbuf, textlen );
 							mod->sm_op = SLAP_MOD_ADD_IF_NOT_PRESENT;
							break;

						default:
							err = LDAP_OTHER;
							break;
						}

						if ( err != LDAP_SUCCESS ) {
							rc = err;
							goto mod_violation;
						}
					}
				}

				if ( acl_match_set(&cp->val, op, target_entry_copy, NULL) == 0) {
					rc = LDAP_CONSTRAINT_VIOLATION;
					goto mod_violation;
				}
			}
		}
	}

	if (target_entry) {
		op->o_bd = on->on_info->oi_origdb;
		be_entry_release_r(op, target_entry);
		op->o_bd = be;
	}

	if (target_entry_copy) {
		entry_free(target_entry_copy);
	}

	return SLAP_CB_CONTINUE;

mod_violation:
	/* violation */
	if (target_entry) {
		op->o_bd = on->on_info->oi_origdb;
		be_entry_release_r(op, target_entry);
		op->o_bd = be;
	}

	if (target_entry_copy) {
		entry_free(target_entry_copy);
	}

	op->o_bd->bd_info = (BackendInfo *)(on->on_info);
	if ( rc == LDAP_CONSTRAINT_VIOLATION ) {
		msg = print_message( &rsv, m->sml_desc );
	}
	send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION, msg );
	ch_free(msg);
	return (rs->sr_err);
}

static int
constraint_close(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	constraint *ap, *a2;

	for ( ap = on->on_bi.bi_private; ap; ap = a2 ) {
		a2 = ap->ap_next;
		constraint_free( ap, 1 );
	}

	return 0;
}

static slap_overinst constraint_ovl;

#if SLAPD_OVER_CONSTRAINT == SLAPD_MOD_DYNAMIC
static
#endif
int
constraint_initialize( void ) {
	int rc;

	constraint_ovl.on_bi.bi_type = "constraint";
	constraint_ovl.on_bi.bi_db_close = constraint_close;
	constraint_ovl.on_bi.bi_op_add = constraint_add;
	constraint_ovl.on_bi.bi_op_modify = constraint_update;
	constraint_ovl.on_bi.bi_op_modrdn = constraint_update;

	constraint_ovl.on_bi.bi_private = NULL;
	
	constraint_ovl.on_bi.bi_cf_ocs = constraintocs;
	rc = config_register_schema( constraintcfg, constraintocs );
	if (rc) return rc;
	
	return overlay_register( &constraint_ovl );
}

#if SLAPD_OVER_CONSTRAINT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return constraint_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_CONSTRAINT) */

