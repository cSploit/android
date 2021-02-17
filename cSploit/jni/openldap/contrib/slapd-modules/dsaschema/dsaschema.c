/* dsaschema.c */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
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
#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/errno.h>
#include <ac/stdlib.h>
#include <ac/ctype.h>
#include <ac/time.h>
#include <ac/unistd.h>

#include <stdio.h>

/*
 * Schema reader that allows us to define DSA schema (including
 * operational attributes and non-user object classes)
 *
 * A kludge, at best, and in order to avoid including slapd
 * headers we use fprintf() rather than slapd's native logging,
 * which may confuse users...
 *
 */

#include <ldap.h>
#include <ldap_schema.h>

extern int at_add(LDAPAttributeType *at, const char **err);
extern int oc_add(LDAPObjectClass *oc, int user, const char **err);
extern int cr_add(LDAPContentRule *cr, int user, const char **err);

#define ARGS_STEP 512

static char *fp_getline(FILE *fp, int *lineno);
static void fp_getline_init(int *lineno);
static int fp_parse_line(int lineno, char *line);
static char *strtok_quote( char *line, char *sep );

static char **cargv = NULL;
static int cargv_size = 0;
static int cargc = 0;
static char *strtok_quote_ptr;

int init_module(int argc, char *argv[]);

static int dsaschema_parse_at(const char *fname, int lineno, char *line, char **argv)
{
	LDAPAttributeType *at;
	int code;
	const char *err;

	at = ldap_str2attributetype(line, &code, &err, LDAP_SCHEMA_ALLOW_ALL);
	if (!at) {
		fprintf(stderr, "%s: line %d: %s before %s\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	if (at->at_oid == NULL) {
		fprintf(stderr, "%s: line %d: attributeType has no OID\n",
			fname, lineno);
		return 1;
	}

	code = at_add(at, &err);
	if (code) {
		fprintf(stderr, "%s: line %d: %s: \"%s\"\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	ldap_memfree(at);

	return 0;
}

static int dsaschema_parse_oc(const char *fname, int lineno, char *line, char **argv)
{
	LDAPObjectClass *oc;
	int code;
	const char *err;

	oc = ldap_str2objectclass(line, &code, &err, LDAP_SCHEMA_ALLOW_ALL);
	if (!oc) {
		fprintf(stderr, "%s: line %d: %s before %s\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	if (oc->oc_oid == NULL) {
		fprintf(stderr,
			"%s: line %d: objectclass has no OID\n",
			fname, lineno);
		return 1;
	}

	code = oc_add(oc, 0, &err);
	if (code) {
		fprintf(stderr, "%s: line %d: %s: \"%s\"\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	ldap_memfree(oc);
	return 0;
}

static int dsaschema_parse_cr(const char *fname, int lineno, char *line, char **argv)
{
	LDAPContentRule *cr;
	int code;
	const char *err;

	cr = ldap_str2contentrule(line, &code, &err, LDAP_SCHEMA_ALLOW_ALL);
	if (!cr) {
		fprintf(stderr, "%s: line %d: %s before %s\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	if (cr->cr_oid == NULL) {
		fprintf(stderr,
			"%s: line %d: objectclass has no OID\n",
			fname, lineno);
		return 1;
	}

	code = cr_add(cr, 0, &err);
	if (code) {
		fprintf(stderr, "%s: line %d: %s: \"%s\"\n",
			fname, lineno, ldap_scherr2str(code), err);
		return 1;
	}

	ldap_memfree(cr);
	return 0;
}

static int dsaschema_read_config(const char *fname, int depth)
{
	FILE *fp;
	char *line, *savefname, *saveline;
	int savelineno, lineno;
	int rc;

	if (depth == 0) {
		cargv = calloc(ARGS_STEP + 1, sizeof(*cargv));
		if (cargv == NULL) {
			return 1;
		}
		cargv_size = ARGS_STEP + 1;
	}

	fp = fopen(fname, "r");
	if (fp == NULL) {
		fprintf(stderr, "could not open config file \"%s\": %s (%d)\n",
			fname, strerror(errno), errno);
		return 1;
	}
	fp_getline_init(&lineno);

	while ((line = fp_getline(fp, &lineno)) != NULL) {
		/* skip comments and blank lines */
		if (line[0] == '#' || line[0] == '\0') {
			continue;
		}

		saveline = strdup(line);
		if (saveline == NULL) {
			return 1;
		}

		if (fp_parse_line(lineno, line) != 0) {
			return 1;
		}

		if (cargc < 1) {
			continue;
		}

		if (strcasecmp(cargv[0], "attributetype") == 0 ||
		    strcasecmp(cargv[0], "attribute") == 0) {
			if (cargc < 2) {
				fprintf(stderr, "%s: line %d: illegal attribute type format\n",
					fname, lineno);
				return 1;
			} else if (*cargv[1] == '(' /*')'*/) {
				char *p;
	
				p = strchr(saveline, '(' /*')'*/);
				rc = dsaschema_parse_at(fname, lineno, p, cargv);
				if (rc != 0)
					return rc;
			} else {
				fprintf(stderr, "%s: line %d: old attribute type format not supported\n",
					fname, lineno);
			}
		} else if (strcasecmp(cargv[0], "ditcontentrule") == 0) {
			char *p;
			p = strchr(saveline, '(' /*')'*/);
			rc = dsaschema_parse_cr(fname, lineno, p, cargv);
			if (rc != 0)
				return rc;
		} else if (strcasecmp(cargv[0], "objectclass") == 0) {
			if (cargc < 2) {
				fprintf(stderr, "%s: line %d: illegal objectclass format\n",
					fname, lineno);
				return 1;
			} else if (*cargv[1] == '(' /*')'*/) {
				char *p;

				p = strchr(saveline, '(' /*')'*/);
				rc = dsaschema_parse_oc(fname, lineno, p, cargv);
				if (rc != 0)
					return rc;
			} else {
				fprintf(stderr, "%s: line %d: object class format not supported\n",
					fname, lineno);
			}
		} else if (strcasecmp(cargv[0], "include") == 0) {
			if (cargc < 2) {
				fprintf(stderr, "%s: line %d: missing file name in \"include <filename>\" line",
					fname, lineno);
				return 1;
			}
			savefname = strdup(cargv[1]);
			if (savefname == NULL) {
				return 1;
			}
			if (dsaschema_read_config(savefname, depth + 1) != 0) {
				return 1;
			}
			free(savefname);
			lineno = savelineno - 1;
		} else {
			fprintf(stderr, "%s: line %d: unknown directive \"%s\" (ignored)\n",
				fname, lineno, cargv[0]);
		}
	}

	fclose(fp);

	if (depth == 0)
		free(cargv);

	return 0;
}

int init_module(int argc, char *argv[])
{
	int i;
	int rc;

	for (i = 0; i < argc; i++) {
		rc = dsaschema_read_config(argv[i], 0);
		if (rc != 0) {
			break;
		}
	}

	return rc;
}


static int
fp_parse_line(
    int		lineno,
    char	*line
)
{
	char *	token;

	cargc = 0;
	token = strtok_quote( line, " \t" );

	if ( strtok_quote_ptr ) {
		*strtok_quote_ptr = ' ';
	}

	if ( strtok_quote_ptr ) {
		*strtok_quote_ptr = '\0';
	}

	for ( ; token != NULL; token = strtok_quote( NULL, " \t" ) ) {
		if ( cargc == cargv_size - 1 ) {
			char **tmp;
			tmp = realloc( cargv, (cargv_size + ARGS_STEP) *
					    sizeof(*cargv) );
			if ( tmp == NULL ) {
				return -1;
			}
			cargv = tmp;
			cargv_size += ARGS_STEP;
		}
		cargv[cargc++] = token;
	}
	cargv[cargc] = NULL;
	return 0;
}

static char *
strtok_quote( char *line, char *sep )
{
	int		inquote;
	char		*tmp;
	static char	*next;

	strtok_quote_ptr = NULL;
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
					strtok_quote_ptr = next;
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

static char	buf[BUFSIZ];
static char	*line;
static size_t lmax, lcur;

#define CATLINE( buf ) \
	do { \
		size_t len = strlen( buf ); \
		while ( lcur + len + 1 > lmax ) { \
			lmax += BUFSIZ; \
			line = (char *) realloc( line, lmax ); \
		} \
		strcpy( line + lcur, buf ); \
		lcur += len; \
	} while( 0 )

static char *
fp_getline( FILE *fp, int *lineno )
{
	char		*p;

	lcur = 0;
	CATLINE( buf );
	(*lineno)++;

	/* hack attack - keeps us from having to keep a stack of bufs... */
	if ( strncasecmp( line, "include", 7 ) == 0 ) {
		buf[0] = '\0';
		return( line );
	}

	while ( fgets( buf, sizeof(buf), fp ) != NULL ) {
		/* trim off \r\n or \n */
		if ( (p = strchr( buf, '\n' )) != NULL ) {
			if( p > buf && p[-1] == '\r' ) --p;
			*p = '\0';
		}
		
		/* trim off trailing \ and append the next line */
		if ( line[ 0 ] != '\0' 
				&& (p = line + strlen( line ) - 1)[ 0 ] == '\\'
				&& p[ -1 ] != '\\' ) {
			p[ 0 ] = '\0';
			lcur--;

		} else {
			if ( ! isspace( (unsigned char) buf[0] ) ) {
				return( line );
			}

			/* change leading whitespace to a space */
			buf[0] = ' ';
		}

		CATLINE( buf );
		(*lineno)++;
	}
	buf[0] = '\0';

	return( line[0] ? line : NULL );
}

static void
fp_getline_init( int *lineno )
{
	*lineno = -1;
	buf[0] = '\0';
}

