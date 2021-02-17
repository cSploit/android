/* config.c - configuration file handling routines */
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

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISREG
#define	S_ISREG(m)	(((m) & _S_IFMT) == _S_IFREG)
#endif

#include "slap.h"
#ifdef LDAP_SLAPI
#include "slapi/slapi.h"
#endif
#include "lutil.h"
#include "lutil_ldap.h"
#include "config.h"

#define ARGS_STEP	512

/*
 * defaults for various global variables
 */
slap_mask_t		global_allows = 0;
slap_mask_t		global_disallows = 0;
int		global_gentlehup = 0;
int		global_idletimeout = 0;
int		global_writetimeout = 0;
char	*global_host = NULL;
struct berval global_host_bv = BER_BVNULL;
char	*global_realm = NULL;
char	*sasl_host = NULL;
char		**default_passwd_hash = NULL;
struct berval default_search_base = BER_BVNULL;
struct berval default_search_nbase = BER_BVNULL;

ber_len_t sockbuf_max_incoming = SLAP_SB_MAX_INCOMING_DEFAULT;
ber_len_t sockbuf_max_incoming_auth= SLAP_SB_MAX_INCOMING_AUTH;

int	slap_conn_max_pending = SLAP_CONN_MAX_PENDING_DEFAULT;
int	slap_conn_max_pending_auth = SLAP_CONN_MAX_PENDING_AUTH;

char   *slapd_pid_file  = NULL;
char   *slapd_args_file = NULL;

int use_reverse_lookup = 0;

#ifdef LDAP_SLAPI
int slapi_plugins_used = 0;
#endif

static int fp_getline(FILE *fp, ConfigArgs *c);
static void fp_getline_init(ConfigArgs *c);

static char	*strtok_quote(char *line, char *sep, char **quote_ptr);
static char *strtok_quote_ldif(char **line);

ConfigArgs *
new_config_args( BackendDB *be, const char *fname, int lineno, int argc, char **argv )
{
	ConfigArgs *c;
	c = ch_calloc( 1, sizeof( ConfigArgs ) );
	if ( c == NULL ) return(NULL);
	c->be     = be; 
	c->fname  = fname;
	c->argc   = argc;
	c->argv   = argv; 
	c->lineno = lineno;
	snprintf( c->log, sizeof( c->log ), "%s: line %d", fname, lineno );
	return(c);
}

void
init_config_argv( ConfigArgs *c )
{
	c->argv = ch_calloc( ARGS_STEP + 1, sizeof( *c->argv ) );
	c->argv_size = ARGS_STEP + 1;
}

ConfigTable *config_find_keyword(ConfigTable *Conf, ConfigArgs *c) {
	int i;

	for(i = 0; Conf[i].name; i++)
		if( (Conf[i].length && (!strncasecmp(c->argv[0], Conf[i].name, Conf[i].length))) ||
			(!strcasecmp(c->argv[0], Conf[i].name)) ) break;
	if ( !Conf[i].name ) return NULL;
	return Conf+i;
}

int config_check_vals(ConfigTable *Conf, ConfigArgs *c, int check_only ) {
	int rc, arg_user, arg_type, arg_syn, iarg;
	unsigned uiarg;
	long larg;
	unsigned long ularg;
	ber_len_t barg;
	
	if(Conf->arg_type == ARG_IGNORED) {
		Debug(LDAP_DEBUG_CONFIG, "%s: keyword <%s> ignored\n",
			c->log, Conf->name, 0);
		return(0);
	}
	arg_type = Conf->arg_type & ARGS_TYPES;
	arg_user = Conf->arg_type & ARGS_USERLAND;
	arg_syn = Conf->arg_type & ARGS_SYNTAX;

	if((arg_type == ARG_DN) && c->argc == 1) {
		c->argc = 2;
		c->argv[1] = "";
	}
	if(Conf->min_args && (c->argc < Conf->min_args)) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> missing <%s> argument",
			c->argv[0], Conf->what ? Conf->what : "" );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: keyword %s\n", c->log, c->cr_msg, 0 );
		return(ARG_BAD_CONF);
	}
	if(Conf->max_args && (c->argc > Conf->max_args)) {
		char	*ignored = " ignored";

		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> extra cruft after <%s>",
			c->argv[0], Conf->what );

		ignored = "";
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s%s.\n",
				c->log, c->cr_msg, ignored );
		return(ARG_BAD_CONF);
	}
	if((arg_syn & ARG_DB) && !c->be) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> only allowed within database declaration",
			c->argv[0] );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: keyword %s\n",
			c->log, c->cr_msg, 0);
		return(ARG_BAD_CONF);
	}
	if((arg_syn & ARG_PRE_BI) && c->bi) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> must occur before any backend %sdeclaration",
			c->argv[0], (arg_syn & ARG_PRE_DB) ? "or database " : "" );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: keyword %s\n",
			c->log, c->cr_msg, 0 );
		return(ARG_BAD_CONF);
	}
	if((arg_syn & ARG_PRE_DB) && c->be && c->be != frontendDB) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> must occur before any database declaration",
			c->argv[0] );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: keyword %s\n",
			c->log, c->cr_msg, 0);
		return(ARG_BAD_CONF);
	}
	if((arg_syn & ARG_PAREN) && *c->argv[1] != '(' /*')'*/) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> old format not supported", c->argv[0] );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(ARG_BAD_CONF);
	}
	if(arg_type && !Conf->arg_item && !(arg_syn & ARG_OFFSET)) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid config_table, arg_item is NULL",
			c->argv[0] );
		Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
			c->log, c->cr_msg, 0);
		return(ARG_BAD_CONF);
	}
	c->type = arg_user;
	memset(&c->values, 0, sizeof(c->values));
	if(arg_type == ARG_STRING) {
		assert( c->argc == 2 );
		if ( !check_only )
			c->value_string = ch_strdup(c->argv[1]);
	} else if(arg_type == ARG_BERVAL) {
		assert( c->argc == 2 );
		if ( !check_only )
			ber_str2bv( c->argv[1], 0, 1, &c->value_bv );
	} else if(arg_type == ARG_DN) {
		struct berval bv;
		assert( c->argc == 2 );
		ber_str2bv( c->argv[1], 0, 0, &bv );
		rc = dnPrettyNormal( NULL, &bv, &c->value_dn, &c->value_ndn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid DN %d (%s)",
				c->argv[0], rc, ldap_err2string( rc ));
			Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n" , c->log, c->cr_msg, 0);
			return(ARG_BAD_CONF);
		}
		if ( check_only ) {
			ch_free( c->value_ndn.bv_val );
			ch_free( c->value_dn.bv_val );
		}
	} else if(arg_type == ARG_ATDESC) {
		const char *text = NULL;
		assert( c->argc == 2 );
		c->value_ad = NULL;
		rc = slap_str2ad( c->argv[1], &c->value_ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid AttributeDescription %d (%s)",
				c->argv[0], rc, text );
			Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n" , c->log, c->cr_msg, 0);
			return(ARG_BAD_CONF);
		}
	} else {	/* all numeric */
		int j;
		iarg = 0; larg = 0; barg = 0;
		switch(arg_type) {
			case ARG_INT:
				assert( c->argc == 2 );
				if ( lutil_atoix( &iarg, c->argv[1], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> unable to parse \"%s\" as int",
						c->argv[0], c->argv[1] );
					Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0);
					return(ARG_BAD_CONF);
				}
				break;
			case ARG_UINT:
				assert( c->argc == 2 );
				if ( lutil_atoux( &uiarg, c->argv[1], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> unable to parse \"%s\" as unsigned int",
						c->argv[0], c->argv[1] );
					Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0);
					return(ARG_BAD_CONF);
				}
				break;
			case ARG_LONG:
				assert( c->argc == 2 );
				if ( lutil_atolx( &larg, c->argv[1], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> unable to parse \"%s\" as long",
						c->argv[0], c->argv[1] );
					Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0);
					return(ARG_BAD_CONF);
				}
				break;
			case ARG_ULONG:
				assert( c->argc == 2 );
				if ( lutil_atoulx( &ularg, c->argv[1], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> unable to parse \"%s\" as unsigned long",
						c->argv[0], c->argv[1] );
					Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0);
					return(ARG_BAD_CONF);
				}
				break;
			case ARG_BER_LEN_T: {
				unsigned long	l;
				assert( c->argc == 2 );
				if ( lutil_atoulx( &l, c->argv[1], 0 ) != 0 ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"<%s> unable to parse \"%s\" as ber_len_t",
						c->argv[0], c->argv[1] );
					Debug(LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0);
					return(ARG_BAD_CONF);
				}
				barg = (ber_len_t)l;
				} break;
			case ARG_ON_OFF:
				/* note: this is an explicit exception
				 * to the "need exactly 2 args" rule */
				if (c->argc == 1) {
					iarg = 1;
				} else if ( !strcasecmp(c->argv[1], "on") ||
					!strcasecmp(c->argv[1], "true") ||
					!strcasecmp(c->argv[1], "yes") )
				{
					iarg = 1;
				} else if ( !strcasecmp(c->argv[1], "off") ||
					!strcasecmp(c->argv[1], "false") ||
					!strcasecmp(c->argv[1], "no") )
				{
					iarg = 0;
				} else {
					snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid value",
						c->argv[0] );
					Debug(LDAP_DEBUG_ANY|LDAP_DEBUG_NONE, "%s: %s\n",
						c->log, c->cr_msg, 0 );
					return(ARG_BAD_CONF);
				}
				break;
		}
		j = (arg_type & ARG_NONZERO) ? 1 : 0;
		if(iarg < j && larg < j && barg < (unsigned)j ) {
			larg = larg ? larg : (barg ? (long)barg : iarg);
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> invalid value",
				c->argv[0] );
			Debug(LDAP_DEBUG_ANY|LDAP_DEBUG_NONE, "%s: %s\n",
				c->log, c->cr_msg, 0 );
			return(ARG_BAD_CONF);
		}
		switch(arg_type) {
			case ARG_ON_OFF:
			case ARG_INT:		c->value_int = iarg;		break;
			case ARG_UINT:		c->value_uint = uiarg;		break;
			case ARG_LONG:		c->value_long = larg;		break;
			case ARG_ULONG:		c->value_ulong = ularg;		break;
			case ARG_BER_LEN_T:	c->value_ber_t = barg;		break;
		}
	}
	return 0;
}

int config_set_vals(ConfigTable *Conf, ConfigArgs *c) {
	int rc, arg_type;
	void *ptr = NULL;

	arg_type = Conf->arg_type;
	if(arg_type & ARG_MAGIC) {
		if(!c->be) c->be = frontendDB;
		c->cr_msg[0] = '\0';
		rc = (*((ConfigDriver*)Conf->arg_item))(c);
#if 0
		if(c->be == frontendDB) c->be = NULL;
#endif
		if(rc) {
			if ( !c->cr_msg[0] ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> handler exited with %d",
					c->argv[0], rc );
				Debug(LDAP_DEBUG_CONFIG, "%s: %s!\n",
					c->log, c->cr_msg, 0 );
			}
			return(ARG_BAD_CONF);
		}
		return(0);
	}
	if(arg_type & ARG_OFFSET) {
		if (c->be && c->table == Cft_Database)
			ptr = c->be->be_private;
		else if (c->bi)
			ptr = c->bi->bi_private;
		else {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> offset is missing base pointer",
				c->argv[0] );
			Debug(LDAP_DEBUG_CONFIG, "%s: %s!\n",
				c->log, c->cr_msg, 0);
			return(ARG_BAD_CONF);
		}
		ptr = (void *)((char *)ptr + (long)Conf->arg_item);
	} else if (arg_type & ARGS_TYPES) {
		ptr = Conf->arg_item;
	}
	if(arg_type & ARGS_TYPES)
		switch(arg_type & ARGS_TYPES) {
			case ARG_ON_OFF:
			case ARG_INT: 		*(int*)ptr = c->value_int;			break;
			case ARG_UINT: 		*(unsigned*)ptr = c->value_uint;			break;
			case ARG_LONG:  	*(long*)ptr = c->value_long;			break;
			case ARG_ULONG:  	*(unsigned long*)ptr = c->value_ulong;			break;
			case ARG_BER_LEN_T: 	*(ber_len_t*)ptr = c->value_ber_t;			break;
			case ARG_STRING: {
				char *cc = *(char**)ptr;
				if(cc) {
					if ((arg_type & ARG_UNIQUE) && c->op == SLAP_CONFIG_ADD ) {
						Debug(LDAP_DEBUG_CONFIG, "%s: already set %s!\n",
							c->log, Conf->name, 0 );
						return(ARG_BAD_CONF);
					}
					ch_free(cc);
				}
				*(char **)ptr = c->value_string;
				break;
				}
			case ARG_BERVAL:
				*(struct berval *)ptr = c->value_bv;
				break;
			case ARG_ATDESC:
				*(AttributeDescription **)ptr = c->value_ad;
				break;
		}
	return(0);
}

int config_add_vals(ConfigTable *Conf, ConfigArgs *c) {
	int rc, arg_type;

	arg_type = Conf->arg_type;
	if(arg_type == ARG_IGNORED) {
		Debug(LDAP_DEBUG_CONFIG, "%s: keyword <%s> ignored\n",
			c->log, Conf->name, 0);
		return(0);
	}
	rc = config_check_vals( Conf, c, 0 );
	if ( rc ) return rc;
	return config_set_vals( Conf, c );
}

int
config_del_vals(ConfigTable *cf, ConfigArgs *c)
{
	int rc = 0;

	/* If there is no handler, just ignore it */
	if ( cf->arg_type & ARG_MAGIC ) {
		c->argv[0] = cf->ad->ad_cname.bv_val;
		c->op = LDAP_MOD_DELETE;
		c->type = cf->arg_type & ARGS_USERLAND;
		rc = (*((ConfigDriver*)cf->arg_item))(c);
	}
	return rc;
}

int
config_get_vals(ConfigTable *cf, ConfigArgs *c)
{
	int rc = 0;
	struct berval bv;
	void *ptr;

	if ( cf->arg_type & ARG_IGNORED ) {
		return 1;
	}

	memset(&c->values, 0, sizeof(c->values));
	c->rvalue_vals = NULL;
	c->rvalue_nvals = NULL;
	c->op = SLAP_CONFIG_EMIT;
	c->type = cf->arg_type & ARGS_USERLAND;

	if ( cf->arg_type & ARG_MAGIC ) {
		rc = (*((ConfigDriver*)cf->arg_item))(c);
		if ( rc ) return rc;
	} else {
		if ( cf->arg_type & ARG_OFFSET ) {
			if (c->be && c->table == Cft_Database)
				ptr = c->be->be_private;
			else if ( c->bi )
				ptr = c->bi->bi_private;
			else
				return 1;
			ptr = (void *)((char *)ptr + (long)cf->arg_item);
		} else {
			ptr = cf->arg_item;
		}
		
		switch(cf->arg_type & ARGS_TYPES) {
		case ARG_ON_OFF:
		case ARG_INT:	c->value_int = *(int *)ptr; break;
		case ARG_UINT:	c->value_uint = *(unsigned *)ptr; break;
		case ARG_LONG:	c->value_long = *(long *)ptr; break;
		case ARG_ULONG:	c->value_ulong = *(unsigned long *)ptr; break;
		case ARG_BER_LEN_T:	c->value_ber_t = *(ber_len_t *)ptr; break;
		case ARG_STRING:
			if ( *(char **)ptr )
				c->value_string = ch_strdup(*(char **)ptr);
			break;
		case ARG_BERVAL:
			c->value_bv = *((struct berval *)ptr); break;
		case ARG_ATDESC:
			c->value_ad = *(AttributeDescription **)ptr; break;
		}
	}
	if ( cf->arg_type & ARGS_TYPES) {
		bv.bv_len = 0;
		bv.bv_val = c->log;
		switch(cf->arg_type & ARGS_TYPES) {
		case ARG_INT: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%d", c->value_int); break;
		case ARG_UINT: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%u", c->value_uint); break;
		case ARG_LONG: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%ld", c->value_long); break;
		case ARG_ULONG: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%lu", c->value_ulong); break;
		case ARG_BER_LEN_T: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%ld", c->value_ber_t); break;
		case ARG_ON_OFF: bv.bv_len = snprintf(bv.bv_val, sizeof( c->log ), "%s",
			c->value_int ? "TRUE" : "FALSE"); break;
		case ARG_STRING:
			if ( c->value_string && c->value_string[0]) {
				ber_str2bv( c->value_string, 0, 0, &bv);
			} else {
				return 1;
			}
			break;
		case ARG_BERVAL:
			if ( !BER_BVISEMPTY( &c->value_bv )) {
				bv = c->value_bv;
			} else {
				return 1;
			}
			break;
		case ARG_ATDESC:
			if ( c->value_ad ) {
				bv = c->value_ad->ad_cname;
			} else {
				return 1;
			}
			break;
		default:
			bv.bv_val = NULL;
			break;
		}
		if (bv.bv_val == c->log && bv.bv_len >= sizeof( c->log ) ) {
			return 1;
		}
		if (( cf->arg_type & ARGS_TYPES ) == ARG_STRING ) {
			ber_bvarray_add(&c->rvalue_vals, &bv);
		} else if ( !BER_BVISNULL( &bv ) ) {
			value_add_one(&c->rvalue_vals, &bv);
		}
		/* else: maybe c->rvalue_vals already set? */
	}
	return rc;
}

int
init_config_attrs(ConfigTable *ct) {
	int i, code;

	for (i=0; ct[i].name; i++ ) {
		if ( !ct[i].attribute ) continue;
		code = register_at( ct[i].attribute, &ct[i].ad, 1 );
		if ( code ) {
			fprintf( stderr, "init_config_attrs: register_at failed\n" );
			return code;
		}
	}

	return 0;
}

int
init_config_ocs( ConfigOCs *ocs ) {
	int i, code;

	for (i=0;ocs[i].co_def;i++) {
		code = register_oc( ocs[i].co_def, &ocs[i].co_oc, 1 );
		if ( code ) {
			fprintf( stderr, "init_config_ocs: register_oc failed\n" );
			return code;
		}
	}
	return 0;
}

/* Split an LDIF line into space-separated tokens. Words may be grouped
 * by quotes. A quoted string may begin in the middle of a word, but must
 * end at the end of the word (be followed by whitespace or EOS). Any other
 * quotes are passed through unchanged. All other characters are passed
 * through unchanged.
 */
static char *
strtok_quote_ldif( char **line )
{
	char *beg, *ptr, *quote=NULL;
	int inquote=0;

	ptr = *line;

	if ( !ptr || !*ptr )
		return NULL;

	while( isspace( (unsigned char) *ptr )) ptr++;

	if ( *ptr == '"' ) {
		inquote = 1;
		ptr++;
	}

	beg = ptr;

	for (;*ptr;ptr++) {
		if ( *ptr == '"' ) {
			if ( inquote && ( !ptr[1] || isspace((unsigned char) ptr[1]))) {
				*ptr++ = '\0';
				break;
			}
			inquote = 1;
			quote = ptr;
			continue;
		}
		if ( inquote )
			continue;
		if ( isspace( (unsigned char) *ptr )) {
			*ptr++ = '\0';
			break;
		}
	}
	if ( quote ) {
		while ( quote < ptr ) {
			*quote = quote[1];
			quote++;
		}
	}
	if ( !*ptr ) {
		*line = NULL;
	} else {
		while ( isspace( (unsigned char) *ptr )) ptr++;
		*line = ptr;
	}
	return beg;
}

static void
config_parse_ldif( ConfigArgs *c )
{
	char *next;
	c->tline = ch_strdup(c->line);
	next = c->tline;

	while ((c->argv[c->argc] = strtok_quote_ldif( &next )) != NULL) {
		c->argc++;
		if ( c->argc >= c->argv_size ) {
			char **tmp = ch_realloc( c->argv, (c->argv_size + ARGS_STEP) *
				sizeof( *c->argv ));
			c->argv = tmp;
			c->argv_size += ARGS_STEP;
		}
	}
	c->argv[c->argc] = NULL;
}

int
config_parse_vals(ConfigTable *ct, ConfigArgs *c, int valx)
{
	int 	rc = 0;

	snprintf( c->log, sizeof( c->log ), "%s: value #%d",
		ct->ad->ad_cname.bv_val, valx );
	c->argc = 1;
	c->argv[0] = ct->ad->ad_cname.bv_val;

	if ( ( ct->arg_type & ARG_QUOTE ) && c->line[ 0 ] != '"' ) {
		c->argv[c->argc] = c->line;
		c->argc++;
		c->argv[c->argc] = NULL;
		c->tline = NULL;
	} else {
		config_parse_ldif( c );
	}
	rc = config_check_vals( ct, c, 1 );
	ch_free( c->tline );
	c->tline = NULL;

	if ( rc )
		rc = LDAP_CONSTRAINT_VIOLATION;

	return rc;
}

int
config_parse_add(ConfigTable *ct, ConfigArgs *c, int valx)
{
	int	rc = 0;

	snprintf( c->log, sizeof( c->log ), "%s: value #%d",
		ct->ad->ad_cname.bv_val, valx );
	c->argc = 1;
	c->argv[0] = ct->ad->ad_cname.bv_val;

	if ( ( ct->arg_type & ARG_QUOTE ) && c->line[ 0 ] != '"' ) {
		c->argv[c->argc] = c->line;
		c->argc++;
		c->argv[c->argc] = NULL;
		c->tline = NULL;
	} else {
		config_parse_ldif( c );
	}
	c->op = LDAP_MOD_ADD;
	rc = config_add_vals( ct, c );
	ch_free( c->tline );

	return rc;
}

int
read_config_file(const char *fname, int depth, ConfigArgs *cf, ConfigTable *cft)
{
	FILE *fp;
	ConfigTable *ct;
	ConfigArgs *c;
	int rc;
	struct stat s;

	c = ch_calloc( 1, sizeof( ConfigArgs ) );
	if ( c == NULL ) {
		return 1;
	}

	if ( depth ) {
		memcpy( c, cf, sizeof( ConfigArgs ) );
	} else {
		c->depth = depth; /* XXX */
		c->bi = NULL;
		c->be = NULL;
	}

	c->valx = -1;
	c->fname = fname;
	init_config_argv( c );

	if ( stat( fname, &s ) != 0 ) {
		ldap_syslog = 1;
		Debug(LDAP_DEBUG_ANY,
		    "could not stat config file \"%s\": %s (%d)\n",
		    fname, strerror(errno), errno);
		ch_free( c );
		return(1);
	}

	if ( !S_ISREG( s.st_mode ) ) {
		ldap_syslog = 1;
		Debug(LDAP_DEBUG_ANY,
		    "regular file expected, got \"%s\"\n",
		    fname, 0, 0 );
		ch_free( c );
		return(1);
	}

	fp = fopen( fname, "r" );
	if ( fp == NULL ) {
		ldap_syslog = 1;
		Debug(LDAP_DEBUG_ANY,
		    "could not open config file \"%s\": %s (%d)\n",
		    fname, strerror(errno), errno);
		ch_free( c );
		return(1);
	}

	Debug(LDAP_DEBUG_CONFIG, "reading config file %s\n", fname, 0, 0);

	fp_getline_init(c);

	c->tline = NULL;

	while ( fp_getline( fp, c ) ) {
		/* skip comments and blank lines */
		if ( c->line[0] == '#' || c->line[0] == '\0' ) {
			continue;
		}

		snprintf( c->log, sizeof( c->log ), "%s: line %d",
				c->fname, c->lineno );

		c->argc = 0;
		ch_free( c->tline );
		if ( config_fp_parse_line( c ) ) {
			rc = 1;
			goto done;
		}

		if ( c->argc < 1 ) {
			Debug( LDAP_DEBUG_ANY, "%s: bad config line.\n",
				c->log, 0, 0);
			rc = 1;
			goto done;
		}

		c->op = SLAP_CONFIG_ADD;

		ct = config_find_keyword( cft, c );
		if ( ct ) {
			c->table = Cft_Global;
			rc = config_add_vals( ct, c );
			if ( !rc ) continue;

			if ( rc & ARGS_USERLAND ) {
				/* XXX a usertype would be opaque here */
				Debug(LDAP_DEBUG_CONFIG, "%s: unknown user type <%s>\n",
					c->log, c->argv[0], 0);
				rc = 1;
				goto done;

			} else if ( rc == ARG_BAD_CONF ) {
				rc = 1;
				goto done;
			}
			
		} else if ( c->bi && !c->be ) {
			rc = SLAP_CONF_UNKNOWN;
			if ( c->bi->bi_cf_ocs ) {
				ct = config_find_keyword( c->bi->bi_cf_ocs->co_table, c );
				if ( ct ) {
					c->table = c->bi->bi_cf_ocs->co_type;
					rc = config_add_vals( ct, c );
				}
			}
			if ( c->bi->bi_config && rc == SLAP_CONF_UNKNOWN ) {
				rc = (*c->bi->bi_config)(c->bi, c->fname, c->lineno,
					c->argc, c->argv);
			}
			if ( rc ) {
				switch(rc) {
				case SLAP_CONF_UNKNOWN:
					Debug( LDAP_DEBUG_ANY, "%s: unknown directive "
						"<%s> inside backend info definition.\n",
						c->log, *c->argv, 0);
				default:
					rc = 1;
					goto done;
				}
			}

		} else if ( c->be && c->be != frontendDB ) {
			rc = SLAP_CONF_UNKNOWN;
			if ( c->be->be_cf_ocs ) {
				ct = config_find_keyword( c->be->be_cf_ocs->co_table, c );
				if ( ct ) {
					c->table = c->be->be_cf_ocs->co_type;
					rc = config_add_vals( ct, c );
				}
			}
			if ( c->be->be_config && rc == SLAP_CONF_UNKNOWN ) {
				rc = (*c->be->be_config)(c->be, c->fname, c->lineno,
					c->argc, c->argv);
			}
			if ( rc == SLAP_CONF_UNKNOWN && SLAP_ISGLOBALOVERLAY( frontendDB ) )
			{
				/* global overlays may need 
				 * definitions inside other databases...
				 */
				rc = (*frontendDB->be_config)( frontendDB,
					c->fname, (int)c->lineno, c->argc, c->argv );
			}

			switch ( rc ) {
			case 0:
				break;

			case SLAP_CONF_UNKNOWN:
				Debug( LDAP_DEBUG_ANY, "%s: unknown directive "
					"<%s> inside backend database definition.\n",
					c->log, *c->argv, 0);
				
			default:
				rc = 1;
				goto done;
			}

		} else if ( frontendDB->be_config ) {
			rc = (*frontendDB->be_config)( frontendDB,
				c->fname, (int)c->lineno, c->argc, c->argv);
			if ( rc ) {
				switch(rc) {
				case SLAP_CONF_UNKNOWN:
					Debug( LDAP_DEBUG_ANY, "%s: unknown directive "
						"<%s> inside global database definition.\n",
						c->log, *c->argv, 0);

				default:
					rc = 1;
					goto done;
				}
			}
			
		} else {
			Debug( LDAP_DEBUG_ANY, "%s: unknown directive "
				"<%s> outside backend info and database definitions.\n",
				c->log, *c->argv, 0);
			rc = 1;
			goto done;
		}
	}

	rc = 0;

done:
	if ( cf ) {
		cf->be = c->be;
		cf->bi = c->bi;
	}
	ch_free(c->tline);
	fclose(fp);
	ch_free(c->argv);
	ch_free(c);
	return(rc);
}

/* restrictops, allows, disallows, requires, loglevel */

int
bverb_to_mask(struct berval *bword, slap_verbmasks *v) {
	int i;
	for(i = 0; !BER_BVISNULL(&v[i].word); i++) {
		if(!ber_bvstrcasecmp(bword, &v[i].word)) break;
	}
	return(i);
}

int
verb_to_mask(const char *word, slap_verbmasks *v) {
	struct berval	bword;
	ber_str2bv( word, 0, 0, &bword );
	return bverb_to_mask( &bword, v );
}

int
verbs_to_mask(int argc, char *argv[], slap_verbmasks *v, slap_mask_t *m) {
	int i, j;
	for(i = 1; i < argc; i++) {
		j = verb_to_mask(argv[i], v);
		if(BER_BVISNULL(&v[j].word)) return i;
		while (!v[j].mask) j--;
		*m |= v[j].mask;
	}
	return(0);
}

/* Mask keywords that represent multiple bits should occur before single
 * bit keywords in the verbmasks array.
 */
int
mask_to_verbs(slap_verbmasks *v, slap_mask_t m, BerVarray *bva) {
	int i, rc = 1;

	if (m) {
		for (i=0; !BER_BVISNULL(&v[i].word); i++) {
			if (!v[i].mask) continue;
			if (( m & v[i].mask ) == v[i].mask ) {
				value_add_one( bva, &v[i].word );
				rc = 0;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
	}
	return rc;
}

/* Return the verbs as a single string, separated by delim */
int
mask_to_verbstring(slap_verbmasks *v, slap_mask_t m0, char delim, struct berval *bv)
{
	int i, rc = 1;

	BER_BVZERO( bv );
	if (m0) {
		slap_mask_t m = m0;
		char *ptr;
		for (i=0; !BER_BVISNULL(&v[i].word); i++) {
			if (!v[i].mask) continue;
			if (( m & v[i].mask ) == v[i].mask ) {
				bv->bv_len += v[i].word.bv_len + 1;
				rc = 0;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
		bv->bv_val = ch_malloc(bv->bv_len);
		bv->bv_len--;
		ptr = bv->bv_val;
		m = m0;
		for (i=0; !BER_BVISNULL(&v[i].word); i++) {
			if (!v[i].mask) continue;
			if (( m & v[i].mask ) == v[i].mask ) {
				ptr = lutil_strcopy(ptr, v[i].word.bv_val);
				*ptr++ = delim;
				m ^= v[i].mask;
				if ( !m ) break;
			}
		}
		ptr[-1] = '\0';
	}
	return rc;
}

/* Parse a verbstring */
int
verbstring_to_mask(slap_verbmasks *v, char *str, char delim, slap_mask_t *m) {
	int j;
	char *d;
	struct berval bv;

	do {
		bv.bv_val = str;
		d = strchr( str, delim );
		if ( d )
			bv.bv_len = d - str;
		else
			bv.bv_len = strlen( str );
		j = bverb_to_mask( &bv, v );
		if(BER_BVISNULL(&v[j].word)) return 1;
		while (!v[j].mask) j--;
		*m |= v[j].mask;
		str += bv.bv_len + 1;
	} while ( d );
	return(0);
}

int
slap_verbmasks_init( slap_verbmasks **vp, slap_verbmasks *v )
{
	int		i;

	assert( *vp == NULL );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) /* EMPTY */;

	*vp = ch_calloc( i + 1, sizeof( slap_verbmasks ) );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) {
		ber_dupbv( &(*vp)[ i ].word, &v[ i ].word );
		*((slap_mask_t *)&(*vp)[ i ].mask) = v[ i ].mask;
	}

	BER_BVZERO( &(*vp)[ i ].word );

	return 0;		
}

int
slap_verbmasks_destroy( slap_verbmasks *v )
{
	int		i;

	assert( v != NULL );

	for ( i = 0; !BER_BVISNULL( &v[ i ].word ); i++ ) {
		ch_free( v[ i ].word.bv_val );
	}

	ch_free( v );

	return 0;
}

int
slap_verbmasks_append(
	slap_verbmasks	**vp,
	slap_mask_t	m,
	struct berval	*v,
	slap_mask_t	*ignore )
{
	int	i;

	if ( !m ) {
		return LDAP_OPERATIONS_ERROR;
	}

	for ( i = 0; !BER_BVISNULL( &(*vp)[ i ].word ); i++ ) {
		if ( !(*vp)[ i ].mask ) continue;

		if ( ignore != NULL ) {
			int	j;

			for ( j = 0; ignore[ j ] != 0; j++ ) {
				if ( (*vp)[ i ].mask == ignore[ j ] ) {
					goto check_next;
				}
			}
		}

		if ( ( m & (*vp)[ i ].mask ) == (*vp)[ i ].mask ) {
			if ( ber_bvstrcasecmp( v, &(*vp)[ i ].word ) == 0 ) {
				/* already set; ignore */
				return LDAP_SUCCESS;
			}
			/* conflicts */
			return LDAP_TYPE_OR_VALUE_EXISTS;
		}

		if ( m & (*vp)[ i ].mask ) {
			/* conflicts */
			return LDAP_CONSTRAINT_VIOLATION;
		}
check_next:;
	}

	*vp = ch_realloc( *vp, sizeof( slap_verbmasks ) * ( i + 2 ) );
	ber_dupbv( &(*vp)[ i ].word, v );
	*((slap_mask_t *)&(*vp)[ i ].mask) = m;
	BER_BVZERO( &(*vp)[ i + 1 ].word );

	return LDAP_SUCCESS;
}

int
enum_to_verb(slap_verbmasks *v, slap_mask_t m, struct berval *bv) {
	int i;

	for (i=0; !BER_BVISNULL(&v[i].word); i++) {
		if ( m == v[i].mask ) {
			if ( bv != NULL ) {
				*bv = v[i].word;
			}
			return i;
		}
	}
	return -1;
}

/* register a new verbmask */
static int
slap_verbmask_register( slap_verbmasks *vm_, slap_verbmasks **vmp, struct berval *bv, int mask )
{
	slap_verbmasks	*vm = *vmp;
	int		i;

	/* check for duplicate word */
	/* NOTE: we accept duplicate codes; the first occurrence will be used
	 * when mapping from mask to verb */
	i = verb_to_mask( bv->bv_val, vm );
	if ( !BER_BVISNULL( &vm[ i ].word ) ) {
		return -1;
	}

	for ( i = 0; !BER_BVISNULL( &vm[ i ].word ); i++ )
		;

	if ( vm == vm_ ) {
		/* first time: duplicate array */
		vm = ch_calloc( i + 2, sizeof( slap_verbmasks ) );
		for ( i = 0; !BER_BVISNULL( &vm_[ i ].word ); i++ )
		{
			ber_dupbv( &vm[ i ].word, &vm_[ i ].word );
			*((slap_mask_t*)&vm[ i ].mask) = vm_[ i ].mask;
		}

	} else {
		vm = ch_realloc( vm, (i + 2) * sizeof( slap_verbmasks ) );
	}

	ber_dupbv( &vm[ i ].word, bv );
	*((slap_mask_t*)&vm[ i ].mask) = mask;

	BER_BVZERO( &vm[ i+1 ].word );

	*vmp = vm;

	return i;
}

static slap_verbmasks slap_ldap_response_code_[] = {
	{ BER_BVC("success"),				LDAP_SUCCESS },

	{ BER_BVC("operationsError"),			LDAP_OPERATIONS_ERROR },
	{ BER_BVC("protocolError"),			LDAP_PROTOCOL_ERROR },
	{ BER_BVC("timelimitExceeded"),			LDAP_TIMELIMIT_EXCEEDED },
	{ BER_BVC("sizelimitExceeded"),			LDAP_SIZELIMIT_EXCEEDED },
	{ BER_BVC("compareFalse"),			LDAP_COMPARE_FALSE },
	{ BER_BVC("compareTrue"),			LDAP_COMPARE_TRUE },

	{ BER_BVC("authMethodNotSupported"),		LDAP_AUTH_METHOD_NOT_SUPPORTED },
	{ BER_BVC("strongAuthNotSupported"),		LDAP_STRONG_AUTH_NOT_SUPPORTED },
	{ BER_BVC("strongAuthRequired"),		LDAP_STRONG_AUTH_REQUIRED },
	{ BER_BVC("strongerAuthRequired"),		LDAP_STRONGER_AUTH_REQUIRED },
#if 0 /* not LDAPv3 */
	{ BER_BVC("partialResults"),			LDAP_PARTIAL_RESULTS },
#endif

	{ BER_BVC("referral"),				LDAP_REFERRAL },
	{ BER_BVC("adminlimitExceeded"),		LDAP_ADMINLIMIT_EXCEEDED },
	{ BER_BVC("unavailableCriticalExtension"),	LDAP_UNAVAILABLE_CRITICAL_EXTENSION },
	{ BER_BVC("confidentialityRequired"),		LDAP_CONFIDENTIALITY_REQUIRED },
	{ BER_BVC("saslBindInProgress"),		LDAP_SASL_BIND_IN_PROGRESS },

	{ BER_BVC("noSuchAttribute"),			LDAP_NO_SUCH_ATTRIBUTE },
	{ BER_BVC("undefinedType"),			LDAP_UNDEFINED_TYPE },
	{ BER_BVC("inappropriateMatching"),		LDAP_INAPPROPRIATE_MATCHING },
	{ BER_BVC("constraintViolation"),		LDAP_CONSTRAINT_VIOLATION },
	{ BER_BVC("typeOrValueExists"),			LDAP_TYPE_OR_VALUE_EXISTS },
	{ BER_BVC("invalidSyntax"),			LDAP_INVALID_SYNTAX },

	{ BER_BVC("noSuchObject"),			LDAP_NO_SUCH_OBJECT },
	{ BER_BVC("aliasProblem"),			LDAP_ALIAS_PROBLEM },
	{ BER_BVC("invalidDnSyntax"),			LDAP_INVALID_DN_SYNTAX },
#if 0 /* not LDAPv3 */
	{ BER_BVC("isLeaf"),				LDAP_IS_LEAF },
#endif
	{ BER_BVC("aliasDerefProblem"),			LDAP_ALIAS_DEREF_PROBLEM },

	{ BER_BVC("proxyAuthzFailure"),			LDAP_X_PROXY_AUTHZ_FAILURE },
	{ BER_BVC("inappropriateAuth"),			LDAP_INAPPROPRIATE_AUTH },
	{ BER_BVC("invalidCredentials"),		LDAP_INVALID_CREDENTIALS },
	{ BER_BVC("insufficientAccess"),		LDAP_INSUFFICIENT_ACCESS },

	{ BER_BVC("busy"),				LDAP_BUSY },
	{ BER_BVC("unavailable"),			LDAP_UNAVAILABLE },
	{ BER_BVC("unwillingToPerform"),		LDAP_UNWILLING_TO_PERFORM },
	{ BER_BVC("loopDetect"),			LDAP_LOOP_DETECT },

	{ BER_BVC("namingViolation"),			LDAP_NAMING_VIOLATION },
	{ BER_BVC("objectClassViolation"),		LDAP_OBJECT_CLASS_VIOLATION },
	{ BER_BVC("notAllowedOnNonleaf"),		LDAP_NOT_ALLOWED_ON_NONLEAF },
	{ BER_BVC("notAllowedOnRdn"),			LDAP_NOT_ALLOWED_ON_RDN },
	{ BER_BVC("alreadyExists"),			LDAP_ALREADY_EXISTS },
	{ BER_BVC("noObjectClassMods"),			LDAP_NO_OBJECT_CLASS_MODS },
	{ BER_BVC("resultsTooLarge"),			LDAP_RESULTS_TOO_LARGE },
	{ BER_BVC("affectsMultipleDsas"),		LDAP_AFFECTS_MULTIPLE_DSAS },

	{ BER_BVC("other"),				LDAP_OTHER },

	/* extension-specific */

	{ BER_BVC("cupResourcesExhausted"),		LDAP_CUP_RESOURCES_EXHAUSTED },
	{ BER_BVC("cupSecurityViolation"),		LDAP_CUP_SECURITY_VIOLATION },
	{ BER_BVC("cupInvalidData"),			LDAP_CUP_INVALID_DATA },
	{ BER_BVC("cupUnsupportedScheme"),		LDAP_CUP_UNSUPPORTED_SCHEME },
	{ BER_BVC("cupReloadRequired"),			LDAP_CUP_RELOAD_REQUIRED },

	{ BER_BVC("cancelled"),				LDAP_CANCELLED },
	{ BER_BVC("noSuchOperation"),			LDAP_NO_SUCH_OPERATION },
	{ BER_BVC("tooLate"),				LDAP_TOO_LATE },
	{ BER_BVC("cannotCancel"),			LDAP_CANNOT_CANCEL },

	{ BER_BVC("assertionFailed"),			LDAP_ASSERTION_FAILED },

	{ BER_BVC("proxiedAuthorizationDenied"),	LDAP_PROXIED_AUTHORIZATION_DENIED },

	{ BER_BVC("syncRefreshRequired"),		LDAP_SYNC_REFRESH_REQUIRED },

	{ BER_BVC("noOperation"),			LDAP_X_NO_OPERATION },

	{ BER_BVNULL,				0 }
};

slap_verbmasks *slap_ldap_response_code = slap_ldap_response_code_;

int
slap_ldap_response_code_register( struct berval *bv, int err )
{
	return slap_verbmask_register( slap_ldap_response_code_,
		&slap_ldap_response_code, bv, err );
}

#ifdef HAVE_TLS
static slap_verbmasks tlskey[] = {
	{ BER_BVC("no"),	SB_TLS_OFF },
	{ BER_BVC("yes"),	SB_TLS_ON },
	{ BER_BVC("critical"),	SB_TLS_CRITICAL },
	{ BER_BVNULL, 0 }
};

static slap_verbmasks crlkeys[] = {
		{ BER_BVC("none"),	LDAP_OPT_X_TLS_CRL_NONE },
		{ BER_BVC("peer"),	LDAP_OPT_X_TLS_CRL_PEER },
		{ BER_BVC("all"),	LDAP_OPT_X_TLS_CRL_ALL },
		{ BER_BVNULL, 0 }
	};

static slap_verbmasks vfykeys[] = {
		{ BER_BVC("never"),	LDAP_OPT_X_TLS_NEVER },
		{ BER_BVC("allow"),	LDAP_OPT_X_TLS_ALLOW },
		{ BER_BVC("try"),	LDAP_OPT_X_TLS_TRY },
		{ BER_BVC("demand"),	LDAP_OPT_X_TLS_DEMAND },
		{ BER_BVC("hard"),	LDAP_OPT_X_TLS_HARD },
		{ BER_BVC("true"),	LDAP_OPT_X_TLS_HARD },
		{ BER_BVNULL, 0 }
	};
#endif

static slap_verbmasks methkey[] = {
	{ BER_BVC("none"),	LDAP_AUTH_NONE },
	{ BER_BVC("simple"),	LDAP_AUTH_SIMPLE },
#ifdef HAVE_CYRUS_SASL
	{ BER_BVC("sasl"),	LDAP_AUTH_SASL },
#endif
	{ BER_BVNULL, 0 }
};

static slap_verbmasks versionkey[] = {
	{ BER_BVC("2"),		LDAP_VERSION2 },
	{ BER_BVC("3"),		LDAP_VERSION3 },
	{ BER_BVNULL, 0 }
};

int
slap_keepalive_parse(
	struct berval *val,
	void *bc,
	slap_cf_aux_table *tab0,
	const char *tabmsg,
	int unparse )
{
	if ( unparse ) {
		slap_keepalive *sk = (slap_keepalive *)bc;
		int rc = snprintf( val->bv_val, val->bv_len, "%d:%d:%d",
			sk->sk_idle, sk->sk_probes, sk->sk_interval );
		if ( rc < 0 ) {
			return -1;
		}

		if ( (unsigned)rc >= val->bv_len ) {
			return -1;
		}

		val->bv_len = rc;

	} else {
		char *s = val->bv_val;
		char *next;
		slap_keepalive *sk = (slap_keepalive *)bc;
		slap_keepalive sk2;

		if ( s[0] == ':' ) {
			sk2.sk_idle = 0;
			s++;
			
		} else {
			sk2.sk_idle = strtol( s, &next, 10 );
			if ( next == s || next[0] != ':' ) {
				return -1;
			}

			if ( sk2.sk_idle < 0 ) {
				return -1;
			}

			s = ++next;
		}

		if ( s[0] == ':' ) {
			sk2.sk_probes = 0;
			s++;

		} else {
			sk2.sk_probes = strtol( s, &next, 10 );
			if ( next == s || next[0] != ':' ) {
				return -1;
			}

			if ( sk2.sk_probes < 0 ) {
				return -1;
			}

			s = ++next;
		}

		if ( s == '\0' ) {
			sk2.sk_interval = 0;
			s++;

		} else {
			sk2.sk_interval = strtol( s, &next, 10 );
			if ( next == s || next[0] != '\0' ) {
				return -1;
			}

			if ( sk2.sk_interval < 0 ) {
				return -1;
			}
		}

		*sk = sk2;

		ber_memfree( val->bv_val );
		BER_BVZERO( val );
	}

	return 0;
}

static int
slap_sb_uri(
	struct berval *val,
	void *bcp,
	slap_cf_aux_table *tab0,
	const char *tabmsg,
	int unparse )
{
	slap_bindconf *bc = bcp;
	if ( unparse ) {
		if ( bc->sb_uri.bv_len >= val->bv_len )
			return -1;
		val->bv_len = bc->sb_uri.bv_len;
		AC_MEMCPY( val->bv_val, bc->sb_uri.bv_val, val->bv_len );
	} else {
		bc->sb_uri = *val;
#ifdef HAVE_TLS
		if ( ldap_is_ldaps_url( val->bv_val ))
			bc->sb_tls_do_init = 1;
#endif
	}
	return 0;
}

static slap_cf_aux_table bindkey[] = {
	{ BER_BVC("uri="), 0, 'x', 1, slap_sb_uri },
	{ BER_BVC("version="), offsetof(slap_bindconf, sb_version), 'i', 0, versionkey },
	{ BER_BVC("bindmethod="), offsetof(slap_bindconf, sb_method), 'i', 0, methkey },
	{ BER_BVC("timeout="), offsetof(slap_bindconf, sb_timeout_api), 'i', 0, NULL },
	{ BER_BVC("network-timeout="), offsetof(slap_bindconf, sb_timeout_net), 'i', 0, NULL },
	{ BER_BVC("binddn="), offsetof(slap_bindconf, sb_binddn), 'b', 1, (slap_verbmasks *)dnNormalize },
	{ BER_BVC("credentials="), offsetof(slap_bindconf, sb_cred), 'b', 1, NULL },
	{ BER_BVC("saslmech="), offsetof(slap_bindconf, sb_saslmech), 'b', 0, NULL },
	{ BER_BVC("secprops="), offsetof(slap_bindconf, sb_secprops), 's', 0, NULL },
	{ BER_BVC("realm="), offsetof(slap_bindconf, sb_realm), 'b', 0, NULL },
	{ BER_BVC("authcID="), offsetof(slap_bindconf, sb_authcId), 'b', 1, NULL },
	{ BER_BVC("authzID="), offsetof(slap_bindconf, sb_authzId), 'b', 1, (slap_verbmasks *)authzNormalize },
	{ BER_BVC("keepalive="), offsetof(slap_bindconf, sb_keepalive), 'x', 0, (slap_verbmasks *)slap_keepalive_parse },
#ifdef HAVE_TLS
	/* NOTE: replace "13" with the actual index
	 * of the first TLS-related line */
#define aux_TLS (bindkey+13)	/* beginning of TLS keywords */

	{ BER_BVC("starttls="), offsetof(slap_bindconf, sb_tls), 'i', 0, tlskey },
	{ BER_BVC("tls_cert="), offsetof(slap_bindconf, sb_tls_cert), 's', 1, NULL },
	{ BER_BVC("tls_key="), offsetof(slap_bindconf, sb_tls_key), 's', 1, NULL },
	{ BER_BVC("tls_cacert="), offsetof(slap_bindconf, sb_tls_cacert), 's', 1, NULL },
	{ BER_BVC("tls_cacertdir="), offsetof(slap_bindconf, sb_tls_cacertdir), 's', 1, NULL },
	{ BER_BVC("tls_reqcert="), offsetof(slap_bindconf, sb_tls_reqcert), 's', 0, NULL },
	{ BER_BVC("tls_cipher_suite="), offsetof(slap_bindconf, sb_tls_cipher_suite), 's', 0, NULL },
	{ BER_BVC("tls_protocol_min="), offsetof(slap_bindconf, sb_tls_protocol_min), 's', 0, NULL },
#ifdef HAVE_OPENSSL_CRL
	{ BER_BVC("tls_crlcheck="), offsetof(slap_bindconf, sb_tls_crlcheck), 's', 0, NULL },
#endif
#endif
	{ BER_BVNULL, 0, 0, 0, NULL }
};

/*
 * 's':	char *
 * 'b':	struct berval; if !NULL, normalize using ((slap_mr_normalize_func *)aux)
 * 'i':	int; if !NULL, compute using ((slap_verbmasks *)aux)
 * 'u':	unsigned
 * 'I':	long
 * 'U':	unsigned long
 */

int
slap_cf_aux_table_parse( const char *word, void *dst, slap_cf_aux_table *tab0, LDAP_CONST char *tabmsg )
{
	int rc = SLAP_CONF_UNKNOWN;
	slap_cf_aux_table *tab;

	for ( tab = tab0; !BER_BVISNULL( &tab->key ); tab++ ) {
		if ( !strncasecmp( word, tab->key.bv_val, tab->key.bv_len ) ) {
			char **cptr;
			int *iptr, j;
			unsigned *uptr;
			long *lptr;
			unsigned long *ulptr;
			struct berval *bptr;
			const char *val = word + tab->key.bv_len;

			switch ( tab->type ) {
			case 's':
				cptr = (char **)((char *)dst + tab->off);
				*cptr = ch_strdup( val );
				rc = 0;
				break;

			case 'b':
				bptr = (struct berval *)((char *)dst + tab->off);
				if ( tab->aux != NULL ) {
					struct berval	dn;
					slap_mr_normalize_func *normalize = (slap_mr_normalize_func *)tab->aux;

					ber_str2bv( val, 0, 0, &dn );
					rc = normalize( 0, NULL, NULL, &dn, bptr, NULL );

				} else {
					ber_str2bv( val, 0, 1, bptr );
					rc = 0;
				}
				break;

			case 'i':
				iptr = (int *)((char *)dst + tab->off);

				if ( tab->aux != NULL ) {
					slap_verbmasks *aux = (slap_verbmasks *)tab->aux;

					assert( aux != NULL );

					rc = 1;
					for ( j = 0; !BER_BVISNULL( &aux[j].word ); j++ ) {
						if ( !strcasecmp( val, aux[j].word.bv_val ) ) {
							*iptr = aux[j].mask;
							rc = 0;
							break;
						}
					}

				} else {
					rc = lutil_atoix( iptr, val, 0 );
				}
				break;

			case 'u':
				uptr = (unsigned *)((char *)dst + tab->off);

				rc = lutil_atoux( uptr, val, 0 );
				break;

			case 'I':
				lptr = (long *)((char *)dst + tab->off);

				rc = lutil_atolx( lptr, val, 0 );
				break;

			case 'U':
				ulptr = (unsigned long *)((char *)dst + tab->off);

				rc = lutil_atoulx( ulptr, val, 0 );
				break;

			case 'x':
				if ( tab->aux != NULL ) {
					struct berval value;
					slap_cf_aux_table_parse_x *func = (slap_cf_aux_table_parse_x *)tab->aux;

					ber_str2bv( val, 0, 1, &value );

					rc = func( &value, (void *)((char *)dst + tab->off), tab, tabmsg, 0 );

				} else {
					rc = 1;
				}
				break;
			}

			if ( rc ) {
				Debug( LDAP_DEBUG_ANY, "invalid %s value %s\n",
					tabmsg, word, 0 );
			}
			
			return rc;
		}
	}

	return rc;
}

int
slap_cf_aux_table_unparse( void *src, struct berval *bv, slap_cf_aux_table *tab0 )
{
	char buf[AC_LINE_MAX], *ptr;
	slap_cf_aux_table *tab;
	struct berval tmp;

	ptr = buf;
	for (tab = tab0; !BER_BVISNULL(&tab->key); tab++ ) {
		char **cptr;
		int *iptr, i;
		unsigned *uptr;
		long *lptr;
		unsigned long *ulptr;
		struct berval *bptr;

		cptr = (char **)((char *)src + tab->off);

		switch ( tab->type ) {
		case 'b':
			bptr = (struct berval *)((char *)src + tab->off);
			cptr = &bptr->bv_val;

		case 's':
			if ( *cptr ) {
				*ptr++ = ' ';
				ptr = lutil_strcopy( ptr, tab->key.bv_val );
				if ( tab->quote ) *ptr++ = '"';
				ptr = lutil_strcopy( ptr, *cptr );
				if ( tab->quote ) *ptr++ = '"';
			}
			break;

		case 'i':
			iptr = (int *)((char *)src + tab->off);

			if ( tab->aux != NULL ) {
				slap_verbmasks *aux = (slap_verbmasks *)tab->aux;

				for ( i = 0; !BER_BVISNULL( &aux[i].word ); i++ ) {
					if ( *iptr == aux[i].mask ) {
						*ptr++ = ' ';
						ptr = lutil_strcopy( ptr, tab->key.bv_val );
						ptr = lutil_strcopy( ptr, aux[i].word.bv_val );
						break;
					}
				}

			} else {
				*ptr++ = ' ';
				ptr = lutil_strcopy( ptr, tab->key.bv_val );
				ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ), "%d", *iptr );
			}
			break;

		case 'u':
			uptr = (unsigned *)((char *)src + tab->off);
			*ptr++ = ' ';
			ptr = lutil_strcopy( ptr, tab->key.bv_val );
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ), "%u", *uptr );
			break;

		case 'I':
			lptr = (long *)((char *)src + tab->off);
			*ptr++ = ' ';
			ptr = lutil_strcopy( ptr, tab->key.bv_val );
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ), "%ld", *lptr );
			break;

		case 'U':
			ulptr = (unsigned long *)((char *)src + tab->off);
			*ptr++ = ' ';
			ptr = lutil_strcopy( ptr, tab->key.bv_val );
			ptr += snprintf( ptr, sizeof( buf ) - ( ptr - buf ), "%lu", *ulptr );
			break;

		case 'x':
			{
				char *saveptr=ptr;
				*ptr++ = ' ';
				ptr = lutil_strcopy( ptr, tab->key.bv_val );
				if ( tab->quote ) *ptr++ = '"';
				if ( tab->aux != NULL ) {
					struct berval value;
					slap_cf_aux_table_parse_x *func = (slap_cf_aux_table_parse_x *)tab->aux;
					int rc;

					value.bv_val = ptr;
					value.bv_len = buf + sizeof( buf ) - ptr;

					rc = func( &value, (void *)((char *)src + tab->off), tab, "(unparse)", 1 );
					if ( rc == 0 ) {
						if (value.bv_len) {
							ptr += value.bv_len;
						} else {
							ptr = saveptr;
							break;
						}
					}
				}
				if ( tab->quote ) *ptr++ = '"';
			}
			break;

		default:
			assert( 0 );
		}
	}
	tmp.bv_val = buf;
	tmp.bv_len = ptr - buf;
	ber_dupbv( bv, &tmp );
	return 0;
}

int
slap_tls_get_config( LDAP *ld, int opt, char **val )
{
#ifdef HAVE_TLS
	slap_verbmasks *keys;
	int i, ival;

	*val = NULL;
	switch( opt ) {
	case LDAP_OPT_X_TLS_CRLCHECK:
		keys = crlkeys;
		break;
	case LDAP_OPT_X_TLS_REQUIRE_CERT:
		keys = vfykeys;
		break;
	case LDAP_OPT_X_TLS_PROTOCOL_MIN: {
		char buf[8];
		ldap_pvt_tls_get_option( ld, opt, &ival );
		snprintf( buf, sizeof( buf ), "%d.%d",
			( ival >> 8 ) & 0xff, ival & 0xff );
		*val = ch_strdup( buf );
		return 0;
		}
	default:
		return -1;
	}
	ldap_pvt_tls_get_option( ld, opt, &ival );
	for (i=0; !BER_BVISNULL(&keys[i].word); i++) {
		if (keys[i].mask == ival) {
			*val = ch_strdup( keys[i].word.bv_val );
			return 0;
		}
	}
#endif
	return -1;
}

int
bindconf_tls_parse( const char *word, slap_bindconf *bc )
{
#ifdef HAVE_TLS
	if ( slap_cf_aux_table_parse( word, bc, aux_TLS, "tls config" ) == 0 ) {
		bc->sb_tls_do_init = 1;
		return 0;
	}
#endif
	return -1;
}

int
bindconf_tls_unparse( slap_bindconf *bc, struct berval *bv )
{
#ifdef HAVE_TLS
	return slap_cf_aux_table_unparse( bc, bv, aux_TLS );
#endif
	return -1;
}

int
bindconf_parse( const char *word, slap_bindconf *bc )
{
#ifdef HAVE_TLS
	/* Detect TLS config changes explicitly */
	if ( bindconf_tls_parse( word, bc ) == 0 ) {
		return 0;
	}
#endif
	return slap_cf_aux_table_parse( word, bc, bindkey, "bind config" );
}

int
bindconf_unparse( slap_bindconf *bc, struct berval *bv )
{
	return slap_cf_aux_table_unparse( bc, bv, bindkey );
}

void bindconf_free( slap_bindconf *bc ) {
	if ( !BER_BVISNULL( &bc->sb_uri ) ) {
		ch_free( bc->sb_uri.bv_val );
		BER_BVZERO( &bc->sb_uri );
	}
	if ( !BER_BVISNULL( &bc->sb_binddn ) ) {
		ch_free( bc->sb_binddn.bv_val );
		BER_BVZERO( &bc->sb_binddn );
	}
	if ( !BER_BVISNULL( &bc->sb_cred ) ) {
		ch_free( bc->sb_cred.bv_val );
		BER_BVZERO( &bc->sb_cred );
	}
	if ( !BER_BVISNULL( &bc->sb_saslmech ) ) {
		ch_free( bc->sb_saslmech.bv_val );
		BER_BVZERO( &bc->sb_saslmech );
	}
	if ( bc->sb_secprops ) {
		ch_free( bc->sb_secprops );
		bc->sb_secprops = NULL;
	}
	if ( !BER_BVISNULL( &bc->sb_realm ) ) {
		ch_free( bc->sb_realm.bv_val );
		BER_BVZERO( &bc->sb_realm );
	}
	if ( !BER_BVISNULL( &bc->sb_authcId ) ) {
		ch_free( bc->sb_authcId.bv_val );
		BER_BVZERO( &bc->sb_authcId );
	}
	if ( !BER_BVISNULL( &bc->sb_authzId ) ) {
		ch_free( bc->sb_authzId.bv_val );
		BER_BVZERO( &bc->sb_authzId );
	}
#ifdef HAVE_TLS
	if ( bc->sb_tls_cert ) {
		ch_free( bc->sb_tls_cert );
		bc->sb_tls_cert = NULL;
	}
	if ( bc->sb_tls_key ) {
		ch_free( bc->sb_tls_key );
		bc->sb_tls_key = NULL;
	}
	if ( bc->sb_tls_cacert ) {
		ch_free( bc->sb_tls_cacert );
		bc->sb_tls_cacert = NULL;
	}
	if ( bc->sb_tls_cacertdir ) {
		ch_free( bc->sb_tls_cacertdir );
		bc->sb_tls_cacertdir = NULL;
	}
	if ( bc->sb_tls_reqcert ) {
		ch_free( bc->sb_tls_reqcert );
		bc->sb_tls_reqcert = NULL;
	}
	if ( bc->sb_tls_cipher_suite ) {
		ch_free( bc->sb_tls_cipher_suite );
		bc->sb_tls_cipher_suite = NULL;
	}
	if ( bc->sb_tls_protocol_min ) {
		ch_free( bc->sb_tls_protocol_min );
		bc->sb_tls_protocol_min = NULL;
	}
#ifdef HAVE_OPENSSL_CRL
	if ( bc->sb_tls_crlcheck ) {
		ch_free( bc->sb_tls_crlcheck );
		bc->sb_tls_crlcheck = NULL;
	}
#endif
	if ( bc->sb_tls_ctx ) {
		ldap_pvt_tls_ctx_free( bc->sb_tls_ctx );
		bc->sb_tls_ctx = NULL;
	}
#endif
}

void
bindconf_tls_defaults( slap_bindconf *bc )
{
#ifdef HAVE_TLS
	if ( bc->sb_tls_do_init ) {
		if ( !bc->sb_tls_cacert )
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_CACERTFILE,
				&bc->sb_tls_cacert );
		if ( !bc->sb_tls_cacertdir )
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_CACERTDIR,
				&bc->sb_tls_cacertdir );
		if ( !bc->sb_tls_cert )
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_CERTFILE,
				&bc->sb_tls_cert );
		if ( !bc->sb_tls_key )
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_KEYFILE,
				&bc->sb_tls_key );
		if ( !bc->sb_tls_cipher_suite )
			ldap_pvt_tls_get_option( slap_tls_ld, LDAP_OPT_X_TLS_CIPHER_SUITE,
				&bc->sb_tls_cipher_suite );
		if ( !bc->sb_tls_reqcert )
			bc->sb_tls_reqcert = ch_strdup("demand");
#ifdef HAVE_OPENSSL_CRL
		if ( !bc->sb_tls_crlcheck )
			slap_tls_get_config( slap_tls_ld, LDAP_OPT_X_TLS_CRLCHECK,
				&bc->sb_tls_crlcheck );
#endif
	}
#endif
}

#ifdef HAVE_TLS
static struct {
	const char *key;
	size_t offset;
	int opt;
} bindtlsopts[] = {
	{ "tls_cert", offsetof(slap_bindconf, sb_tls_cert), LDAP_OPT_X_TLS_CERTFILE },
	{ "tls_key", offsetof(slap_bindconf, sb_tls_key), LDAP_OPT_X_TLS_KEYFILE },
	{ "tls_cacert", offsetof(slap_bindconf, sb_tls_cacert), LDAP_OPT_X_TLS_CACERTFILE },
	{ "tls_cacertdir", offsetof(slap_bindconf, sb_tls_cacertdir), LDAP_OPT_X_TLS_CACERTDIR },
	{ "tls_cipher_suite", offsetof(slap_bindconf, sb_tls_cipher_suite), LDAP_OPT_X_TLS_CIPHER_SUITE },
	{ "tls_protocol_min", offsetof(slap_bindconf, sb_tls_protocol_min), LDAP_OPT_X_TLS_PROTOCOL_MIN },
	{0, 0}
};

int bindconf_tls_set( slap_bindconf *bc, LDAP *ld )
{
	int i, rc, newctx = 0, res = 0;
	char *ptr = (char *)bc, **word;

	bc->sb_tls_do_init = 0;

	for (i=0; bindtlsopts[i].opt; i++) {
		word = (char **)(ptr + bindtlsopts[i].offset);
		if ( *word ) {
			rc = ldap_set_option( ld, bindtlsopts[i].opt, *word );
			if ( rc ) {
				Debug( LDAP_DEBUG_ANY,
					"bindconf_tls_set: failed to set %s to %s\n",
						bindtlsopts[i].key, *word, 0 );
				res = -1;
			} else
				newctx = 1;
		}
	}
	if ( bc->sb_tls_reqcert ) {
		rc = ldap_pvt_tls_config( ld, LDAP_OPT_X_TLS_REQUIRE_CERT,
			bc->sb_tls_reqcert );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				"bindconf_tls_set: failed to set tls_reqcert to %s\n",
					bc->sb_tls_reqcert, 0, 0 );
			res = -1;
		} else
			newctx = 1;
	}
	if ( bc->sb_tls_protocol_min ) {
		rc = ldap_pvt_tls_config( ld, LDAP_OPT_X_TLS_PROTOCOL_MIN,
			bc->sb_tls_protocol_min );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				"bindconf_tls_set: failed to set tls_protocol_min to %s\n",
					bc->sb_tls_protocol_min, 0, 0 );
			res = -1;
		} else
			newctx = 1;
	}
#ifdef HAVE_OPENSSL_CRL
	if ( bc->sb_tls_crlcheck ) {
		rc = ldap_pvt_tls_config( ld, LDAP_OPT_X_TLS_CRLCHECK,
			bc->sb_tls_crlcheck );
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				"bindconf_tls_set: failed to set tls_crlcheck to %s\n",
					bc->sb_tls_crlcheck, 0, 0 );
			res = -1;
		} else
			newctx = 1;
	}
#endif
	if ( newctx ) {
		int opt = 0;

		if ( bc->sb_tls_ctx ) {
			ldap_pvt_tls_ctx_free( bc->sb_tls_ctx );
			bc->sb_tls_ctx = NULL;
		}
		rc = ldap_set_option( ld, LDAP_OPT_X_TLS_NEWCTX, &opt );
		if ( rc )
			res = rc;
		else
			ldap_get_option( ld, LDAP_OPT_X_TLS_CTX, &bc->sb_tls_ctx );
	}
	
	return res;
}
#endif

/*
 * set connection keepalive options
 */
void
slap_client_keepalive(LDAP *ld, slap_keepalive *sk)
{
	if (!sk) return;

	if ( sk->sk_idle ) {
		ldap_set_option( ld, LDAP_OPT_X_KEEPALIVE_IDLE, &sk->sk_idle );
	}

	if ( sk->sk_probes ) {
		ldap_set_option( ld, LDAP_OPT_X_KEEPALIVE_PROBES, &sk->sk_probes );
	}

	if ( sk->sk_interval ) {
		ldap_set_option( ld, LDAP_OPT_X_KEEPALIVE_INTERVAL, &sk->sk_interval );
	}

	return;
}

/*
 * connect to a client using the bindconf data
 * note: should move "version" into bindconf...
 */
int
slap_client_connect( LDAP **ldp, slap_bindconf *sb )
{
	LDAP		*ld = NULL;
	int		rc;
	struct timeval tv;

	/* Init connection to master */
	rc = ldap_initialize( &ld, sb->sb_uri.bv_val );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"slap_client_connect: "
			"ldap_initialize(%s) failed (%d)\n",
			sb->sb_uri.bv_val, rc, 0 );
		return rc;
	}

	if ( sb->sb_version != 0 ) {
		ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION,
			(const void *)&sb->sb_version );
	}

	if ( sb->sb_timeout_api ) {
		tv.tv_sec = sb->sb_timeout_api;
		tv.tv_usec = 0;
		ldap_set_option( ld, LDAP_OPT_TIMEOUT, &tv );
	}

	if ( sb->sb_timeout_net ) {
		tv.tv_sec = sb->sb_timeout_net;
		tv.tv_usec = 0;
		ldap_set_option( ld, LDAP_OPT_NETWORK_TIMEOUT, &tv );
	}

	/* setting network keepalive options */
	slap_client_keepalive(ld, &sb->sb_keepalive);

#ifdef HAVE_TLS
	if ( sb->sb_tls_do_init ) {
		rc = bindconf_tls_set( sb, ld );

	} else if ( sb->sb_tls_ctx ) {
		rc = ldap_set_option( ld, LDAP_OPT_X_TLS_CTX,
			sb->sb_tls_ctx );
	}

	if ( rc ) {
		Debug( LDAP_DEBUG_ANY,
			"slap_client_connect: "
			"URI=%s TLS context initialization failed (%d)\n",
			sb->sb_uri.bv_val, rc, 0 );
		return rc;
	}
#endif

	/* Bind */
	if ( sb->sb_tls ) {
		rc = ldap_start_tls_s( ld, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"slap_client_connect: URI=%s "
				"%s, ldap_start_tls failed (%d)\n",
				sb->sb_uri.bv_val,
				sb->sb_tls == SB_TLS_CRITICAL ?
					"Error" : "Warning",
				rc );
			if ( sb->sb_tls == SB_TLS_CRITICAL ) {
				goto done;
			}
		}
	}

	if ( sb->sb_method == LDAP_AUTH_SASL ) {
#ifdef HAVE_CYRUS_SASL
		void *defaults;

		if ( sb->sb_secprops != NULL ) {
			rc = ldap_set_option( ld,
				LDAP_OPT_X_SASL_SECPROPS, sb->sb_secprops);

			if( rc != LDAP_OPT_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
					"slap_client_connect: "
					"error, ldap_set_option "
					"(%s,SECPROPS,\"%s\") failed!\n",
					sb->sb_uri.bv_val, sb->sb_secprops, 0 );
				goto done;
			}
		}

		defaults = lutil_sasl_defaults( ld,
			sb->sb_saslmech.bv_val,
			sb->sb_realm.bv_val,
			sb->sb_authcId.bv_val,
			sb->sb_cred.bv_val,
			sb->sb_authzId.bv_val );
		if ( defaults == NULL ) {
			rc = LDAP_OTHER;
			goto done;
		}

		rc = ldap_sasl_interactive_bind_s( ld,
				sb->sb_binddn.bv_val,
				sb->sb_saslmech.bv_val,
				NULL, NULL,
				LDAP_SASL_QUIET,
				lutil_sasl_interact,
				defaults );

		lutil_sasl_freedefs( defaults );

		/* FIXME: different error behaviors according to
		 *	1) return code
		 *	2) on err policy : exit, retry, backoff ...
		 */
		if ( rc != LDAP_SUCCESS ) {
			static struct berval bv_GSSAPI = BER_BVC( "GSSAPI" );

			Debug( LDAP_DEBUG_ANY, "slap_client_connect: URI=%s "
				"ldap_sasl_interactive_bind_s failed (%d)\n",
				sb->sb_uri.bv_val, rc, 0 );

			/* FIXME (see above comment) */
			/* if Kerberos credentials cache is not active, retry */
			if ( ber_bvcmp( &sb->sb_saslmech, &bv_GSSAPI ) == 0 &&
				rc == LDAP_LOCAL_ERROR )
			{
				rc = LDAP_SERVER_DOWN;
			}

			goto done;
		}
#else /* HAVE_CYRUS_SASL */
		/* Should never get here, we trapped this at config time */
		assert(0);
		Debug( LDAP_DEBUG_SYNC, "not compiled with SASL support\n", 0, 0, 0 );
		rc = LDAP_OTHER;
		goto done;
#endif

	} else if ( sb->sb_method == LDAP_AUTH_SIMPLE ) {
		rc = ldap_sasl_bind_s( ld,
			sb->sb_binddn.bv_val, LDAP_SASL_SIMPLE,
			&sb->sb_cred, NULL, NULL, NULL );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "slap_client_connect: "
				"URI=%s DN=\"%s\" "
				"ldap_sasl_bind_s failed (%d)\n",
				sb->sb_uri.bv_val, sb->sb_binddn.bv_val, rc );
			goto done;
		}
	}

done:;
	if ( rc ) {
		if ( ld ) {
			ldap_unbind_ext( ld, NULL, NULL );
			*ldp = NULL;
		}

	} else {
		*ldp = ld;
	}

	return rc;
}

/* -------------------------------------- */


static char *
strtok_quote( char *line, char *sep, char **quote_ptr )
{
	int		inquote;
	char		*tmp;
	static char	*next;

	*quote_ptr = NULL;
	if ( line != NULL ) {
		next = line;
	}
	while ( *next && strchr( sep, *next ) ) {
		next++;
	}

	if ( *next == '\0' ) {
		next = NULL;
		return( NULL );
	}
	tmp = next;

	for ( inquote = 0; *next; ) {
		switch ( *next ) {
		case '"':
			if ( inquote ) {
				inquote = 0;
			} else {
				inquote = 1;
			}
			AC_MEMCPY( next, next + 1, strlen( next + 1 ) + 1 );
			break;

		case '\\':
			if ( next[1] )
				AC_MEMCPY( next,
					    next + 1, strlen( next + 1 ) + 1 );
			next++;		/* dont parse the escaped character */
			break;

		default:
			if ( ! inquote ) {
				if ( strchr( sep, *next ) != NULL ) {
					*quote_ptr = next;
					*next++ = '\0';
					return( tmp );
				}
			}
			next++;
			break;
		}
	}

	return( tmp );
}

static char	buf[AC_LINE_MAX];
static char	*line;
static size_t lmax, lcur;

#define CATLINE( buf ) \
	do { \
		size_t len = strlen( buf ); \
		while ( lcur + len + 1 > lmax ) { \
			lmax += AC_LINE_MAX; \
			line = (char *) ch_realloc( line, lmax ); \
		} \
		strcpy( line + lcur, buf ); \
		lcur += len; \
	} while( 0 )

static void
fp_getline_init(ConfigArgs *c) {
	c->lineno = -1;
	buf[0] = '\0';
}

static int
fp_getline( FILE *fp, ConfigArgs *c )
{
	char	*p;

	lcur = 0;
	CATLINE(buf);
	c->lineno++;

	/* avoid stack of bufs */
	if ( strncasecmp( line, "include", STRLENOF( "include" ) ) == 0 ) {
		buf[0] = '\0';
		c->line = line;
		return(1);
	}

	while ( fgets( buf, sizeof( buf ), fp ) ) {
		p = strchr( buf, '\n' );
		if ( p ) {
			if ( p > buf && p[-1] == '\r' ) {
				--p;
			}
			*p = '\0';
		}
		/* XXX ugly */
		c->line = line;
		if ( line[0]
				&& ( p = line + strlen( line ) - 1 )[0] == '\\'
				&& p[-1] != '\\' )
		{
			p[0] = '\0';
			lcur--;
			
		} else {
			if ( !isspace( (unsigned char)buf[0] ) ) {
				return(1);
			}
			buf[0] = ' ';
		}
		CATLINE(buf);
		c->lineno++;
	}

	buf[0] = '\0';
	c->line = line;
	return(line[0] ? 1 : 0);
}

int
config_fp_parse_line(ConfigArgs *c)
{
	char *token;
	static char *const hide[] = {
		"rootpw", "replica", "syncrepl",  /* in slapd */
		"acl-bind", "acl-method", "idassert-bind",  /* in back-ldap */
		"acl-passwd", "bindpw",  /* in back-<ldap/meta> */
		"pseudorootpw",  /* in back-meta */
		"dbpasswd",  /* in back-sql */
		NULL
	};
	char *quote_ptr;
	int i = (int)(sizeof(hide)/sizeof(hide[0])) - 1;

	c->tline = ch_strdup(c->line);
	token = strtok_quote(c->tline, " \t", &quote_ptr);

	if(token) for(i = 0; hide[i]; i++) if(!strcasecmp(token, hide[i])) break;
	if(quote_ptr) *quote_ptr = ' ';
	Debug(LDAP_DEBUG_CONFIG, "line %d (%s%s)\n", c->lineno,
		hide[i] ? hide[i] : c->line, hide[i] ? " ***" : "");
	if(quote_ptr) *quote_ptr = '\0';

	for(;; token = strtok_quote(NULL, " \t", &quote_ptr)) {
		if(c->argc >= c->argv_size) {
			char **tmp;
			tmp = ch_realloc(c->argv, (c->argv_size + ARGS_STEP) * sizeof(*c->argv));
			if(!tmp) {
				Debug(LDAP_DEBUG_ANY, "line %d: out of memory\n", c->lineno, 0, 0);
				return -1;
			}
			c->argv = tmp;
			c->argv_size += ARGS_STEP;
		}
		if(token == NULL)
			break;
		c->argv[c->argc++] = token;
	}
	c->argv[c->argc] = NULL;
	return(0);
}

void
config_destroy( )
{
	ucdata_unload( UCDATA_ALL );
	if ( frontendDB ) {
		/* NOTE: in case of early exit, frontendDB can be NULL */
		if ( frontendDB->be_schemandn.bv_val )
			free( frontendDB->be_schemandn.bv_val );
		if ( frontendDB->be_schemadn.bv_val )
			free( frontendDB->be_schemadn.bv_val );
		if ( frontendDB->be_acl )
			acl_destroy( frontendDB->be_acl );
	}
	free( line );
	if ( slapd_args_file )
		free ( slapd_args_file );
	if ( slapd_pid_file )
		free ( slapd_pid_file );
	if ( default_passwd_hash )
		ldap_charray_free( default_passwd_hash );
}

char **
slap_str2clist( char ***out, char *in, const char *brkstr )
{
	char	*str;
	char	*s;
	char	*lasts;
	int	i, j;
	char	**new;

	/* find last element in list */
	for (i = 0; *out && (*out)[i]; i++);

	/* protect the input string from strtok */
	str = ch_strdup( in );

	if ( *str == '\0' ) {
		free( str );
		return( *out );
	}

	/* Count words in string */
	j=1;
	for ( s = str; *s; s++ ) {
		if ( strchr( brkstr, *s ) != NULL ) {
			j++;
		}
	}

	*out = ch_realloc( *out, ( i + j + 1 ) * sizeof( char * ) );
	new = *out + i;
	for ( s = ldap_pvt_strtok( str, brkstr, &lasts );
		s != NULL;
		s = ldap_pvt_strtok( NULL, brkstr, &lasts ) )
	{
		*new = ch_strdup( s );
		new++;
	}

	*new = NULL;
	free( str );
	return( *out );
}

int config_generic_wrapper( Backend *be, const char *fname, int lineno,
	int argc, char **argv )
{
	ConfigArgs c = { 0 };
	ConfigTable *ct;
	int rc;

	c.be = be;
	c.fname = fname;
	c.lineno = lineno;
	c.argc = argc;
	c.argv = argv;
	c.valx = -1;
	c.line = line;
	c.op = SLAP_CONFIG_ADD;
	snprintf( c.log, sizeof( c.log ), "%s: line %d", fname, lineno );

	rc = SLAP_CONF_UNKNOWN;
	ct = config_find_keyword( be->be_cf_ocs->co_table, &c );
	if ( ct ) {
		c.table = be->be_cf_ocs->co_type;
		rc = config_add_vals( ct, &c );
	}
	return rc;
}

/* See if the given URL (in plain and parsed form) matches
 * any of the server's listener addresses. Return matching
 * Listener or NULL for no match.
 */
Listener *config_check_my_url( const char *url, LDAPURLDesc *lud )
{
	Listener **l = slapd_get_listeners();
	int i, isMe;

	/* Try a straight compare with Listener strings */
	for ( i=0; l && l[i]; i++ ) {
		if ( !strcasecmp( url, l[i]->sl_url.bv_val )) {
			return l[i];
		}
	}

	isMe = 0;
	/* If hostname is empty, or is localhost, or matches
	 * our hostname, this url refers to this host.
	 * Compare it against listeners and ports.
	 */
	if ( !lud->lud_host || !lud->lud_host[0] ||
		!strncasecmp("localhost", lud->lud_host,
			STRLENOF("localhost")) ||
		!strcasecmp( global_host, lud->lud_host )) {

		for ( i=0; l && l[i]; i++ ) {
			LDAPURLDesc *lu2;
			ldap_url_parse( l[i]->sl_url.bv_val, &lu2 );
			do {
				if ( strcasecmp( lud->lud_scheme,
					lu2->lud_scheme ))
					break;
				if ( lud->lud_port != lu2->lud_port )
					break;
				/* Listener on ANY address */
				if ( !lu2->lud_host || !lu2->lud_host[0] ) {
					isMe = 1;
					break;
				}
				/* URL on ANY address */
				if ( !lud->lud_host || !lud->lud_host[0] ) {
					isMe = 1;
					break;
				}
				/* Listener has specific host, must
				 * match it
				 */
				if ( !strcasecmp( lud->lud_host,
					lu2->lud_host )) {
					isMe = 1;
					break;
				}
			} while(0);
			ldap_free_urldesc( lu2 );
			if ( isMe ) {
				return l[i];
			}
		}
	}
	return NULL;
}
