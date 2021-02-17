/* ldif-filter -- clean up LDIF testdata from stdin */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/ctype.h>
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/unistd.h>

#define DEFAULT_SPECS "ndb=a,null=n"

typedef struct { char   *val; size_t len, alloc; } String;
typedef struct { String	*val; size_t len, alloc; } Strings;

/* Flags and corresponding program options */
enum { SORT_ATTRS = 1, SORT_ENTRIES = 2, NO_OUTPUT = 4, DUMMY_FLAG = 8 };
static const char spec_options[] = "aen"; /* option index = log2(enum flag) */

static const char *progname = "ldif-filter";
static const String null_string = { NULL, 0, 0 };

static void
usage( void )
{
	fprintf( stderr, "\
Usage: %s [-b backend] [-s spec[,spec]...]\n\
Filter standard input by first <spec> matching '[<backend>]=[a][e][n]':\n\
  - Remove LDIF comments.\n\
  - 'a': Sort attributes in entries.\n\
  - 'e': Sort any entries separated by just one empty line.\n\
  - 'n': Output nothing.\n\
<backend> defaults to the $BACKEND environment variable.\n\
Use specs '%s' if no spec on the command line applies.\n",
		progname, DEFAULT_SPECS );
	exit( EXIT_FAILURE );
}

/* Return flags from "backend=flags" in spec; nonzero if backend found */
static unsigned
get_flags( const char *backend, const char *spec )
{
	size_t len = strlen( backend );
	unsigned flags = DUMMY_FLAG;
	const char *tmp;

	while ( '=' != *(spec += strncmp( spec, backend, len ) ? 0 : len) ) {
		if ( (spec = strchr( spec, ',' )) == NULL ) {
			return 0;
		}
		++spec;
	}
	while ( *++spec && *spec != ',' ) {
		if ( (tmp = strchr( spec_options, *spec )) == NULL ) {
			usage();
		}
		flags |= 1U << (tmp - spec_options);
	}
	return flags;
}

#define APPEND(s /* String or Strings */, data, count, isString) do { \
	size_t slen = (s)->len, salloc = (s)->alloc, sz = sizeof *(s)->val; \
	if ( salloc <= slen + (count) ) { \
		(s)->alloc = salloc += salloc + ((count)|7) + 1; \
		(s)->val   = xrealloc( (s)->val, sz * salloc ); \
	} \
	memcpy( (s)->val + slen, data, sz * ((count) + !!(isString)) ); \
	(s)->len = slen + (count); \
} while (0)

static void *
xrealloc( void *ptr, size_t len )
{
	if ( (ptr = realloc( ptr, len )) == NULL ) {
		perror( progname );
		exit( EXIT_FAILURE );
	}
	return ptr;
}

static int
cmp( const void *s, const void *t )
{
	return strcmp( ((const String *) s)->val, ((const String *) t)->val );
}

static void
sort_strings( Strings *ss, size_t offset )
{
	qsort( ss->val + offset, ss->len - offset, sizeof(*ss->val), cmp );
}

/* Build entry ss[n] from attrs ss[n...], and free the attrs */
static void
build_entry( Strings *ss, size_t n, unsigned flags, size_t new_len )
{
	String *vals = ss->val, *e = &vals[n];
	size_t end = ss->len;
	char *ptr;

	if ( flags & SORT_ATTRS ) {
		sort_strings( ss, n + 1 );
	}
	e->val = xrealloc( e->val, e->alloc = new_len + 1 );
	ptr = e->val + e->len;
	e->len = new_len;
	ss->len = ++n;
	for ( ; n < end; free( vals[n++].val )) {
		ptr = strcpy( ptr, vals[n].val ) + vals[n].len;
	}
	assert( ptr == e->val + new_len );
}

/* Flush entries to stdout and free them */
static void
flush_entries( Strings *ss, const char *sep, unsigned flags )
{
	size_t i, end = ss->len;
	const char *prefix = "";

	if ( flags & SORT_ENTRIES ) {
		sort_strings( ss, 0 );
	}
	for ( i = 0; i < end; i++, prefix = sep ) {
		if ( printf( "%s%s", prefix, ss->val[i].val ) < 0 ) {
			perror( progname );
			exit( EXIT_FAILURE );
		}
		free( ss->val[i].val );
	}
	ss->len = 0;
}

static void
filter_stdin( unsigned flags )
{
	char line[256];
	Strings ss = { NULL, 0, 0 };	/* entries + attrs of partial entry */
	size_t entries = 0, attrs_totlen = 0, line_len;
	const char *entry_sep = "\n", *sep = "";
	int comment = 0, eof = 0, eol, prev_eol = 1;	/* flags */
	String *s;

	/* LDIF = Entries ss[..entries-1] + sep + attrs ss[entries..] + line */
	for ( ; !eof || ss.len || *sep; prev_eol = eol ) {
		if ( eof || (eof = !fgets( line, sizeof(line), stdin ))) {
			strcpy( line, prev_eol ? "" : *sep ? sep : "\n" );
		}
		line_len = strlen( line );
		eol = (line_len == 0 || line[line_len - 1] == '\n');

		if ( *line == ' ' ) {		/* continuation line? */
			prev_eol = 0;
		} else if ( prev_eol ) {	/* start of logical line? */
			comment = (*line == '#');
		}
		if ( comment || (flags & NO_OUTPUT) ) {
			continue;
		}

		/* Collect attrs for partial entry in ss[entries...] */
		if ( !prev_eol && attrs_totlen != 0 ) {
			goto grow_attr;
		} else if ( line_len > (*line == '\r' ? 2 : 1) ) {
			APPEND( &ss, &null_string, 1, 0 ); /* new attr */
		grow_attr:
			s = &ss.val[ss.len - 1];
			APPEND( s, line, line_len, 1 ); /* strcat to attr */
			attrs_totlen += line_len;
			continue;
		}

		/* Empty line - consume sep+attrs or entries+sep */
		if ( attrs_totlen != 0 ) {
			entry_sep = sep;
			if ( entries == 0 )
				fputs( sep, stdout );
			build_entry( &ss, entries++, flags, attrs_totlen );
			attrs_totlen = 0;
		} else {
			flush_entries( &ss, entry_sep, flags );
			fputs( sep, stdout );
			entries = 0;
		}
		sep = "\r\n" + 2 - line_len;	/* sep = copy(line) */
	}

	free( ss.val );
}

int
main( int argc, char **argv )
{
	const char *backend = getenv( "BACKEND" ), *specs = "", *tmp;
	unsigned flags;
	int i;

	if ( argc > 0 ) {
		progname = (tmp = strrchr( argv[0], '/' )) ? tmp+1 : argv[0];
	}

	while ( (i = getopt( argc, argv, "b:s:" )) != EOF ) {
		switch ( i ) {
		case 'b':
			backend = optarg;
			break;
		case 's':
			specs = optarg;
			break;
		default:
			usage();
		}
	}
	if ( optind < argc ) {
		usage();
	}
	if ( backend == NULL ) {
		backend = "";
	}

	flags = get_flags( backend, specs );
	filter_stdin( flags ? flags : get_flags( backend, DEFAULT_SPECS ));
	if ( fclose( stdout ) == EOF ) {
		perror( progname );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
