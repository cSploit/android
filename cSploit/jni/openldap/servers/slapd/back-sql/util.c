/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
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
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include <sys/types.h>
#include "ac/string.h"
#include "ac/ctype.h"
#include "ac/stdarg.h"

#include "slap.h"
#include "proto-sql.h"
#include "lutil.h"

#define BACKSQL_MAX(a,b) ((a)>(b)?(a):(b))
#define BACKSQL_MIN(a,b) ((a)<(b)?(a):(b))

#define BACKSQL_STR_GROW 256

const char backsql_def_oc_query[] = 
	"SELECT id,name,keytbl,keycol,create_proc,delete_proc,expect_return "
	"FROM ldap_oc_mappings";
const char backsql_def_needs_select_oc_query[] = 
	"SELECT id,name,keytbl,keycol,create_proc,create_keyval,delete_proc,"
	"expect_return FROM ldap_oc_mappings";
const char backsql_def_at_query[] = 
	"SELECT name,sel_expr,from_tbls,join_where,add_proc,delete_proc,"
	"param_order,expect_return,sel_expr_u FROM ldap_attr_mappings "
	"WHERE oc_map_id=?";
const char backsql_def_delentry_stmt[] = "DELETE FROM ldap_entries WHERE id=?";
const char backsql_def_renentry_stmt[] =
	"UPDATE ldap_entries SET dn=?,parent=?,keyval=? WHERE id=?";
const char backsql_def_insentry_stmt[] = 
	"INSERT INTO ldap_entries (dn,oc_map_id,parent,keyval) "
	"VALUES (?,?,?,?)";
const char backsql_def_delobjclasses_stmt[] = "DELETE FROM ldap_entry_objclasses "
	"WHERE entry_id=?";
const char backsql_def_subtree_cond[] = "ldap_entries.dn LIKE CONCAT('%',?)";
const char backsql_def_upper_subtree_cond[] = "(ldap_entries.dn) LIKE CONCAT('%',?)";
const char backsql_id_query[] = "SELECT id,keyval,oc_map_id,dn FROM ldap_entries WHERE ";
/* better ?||? or cast(?||? as varchar) */ 
const char backsql_def_concat_func[] = "CONCAT(?,?)";

/* TimesTen */
const char backsql_check_dn_ru_query[] = "SELECT dn_ru FROM ldap_entries";

struct berbuf *
backsql_strcat_x( struct berbuf *dest, void *memctx, ... )
{
	va_list		strs;
	ber_len_t	cdlen, cslen, grow;
	char		*cstr;

	assert( dest != NULL );
	assert( dest->bb_val.bv_val == NULL 
			|| dest->bb_val.bv_len == strlen( dest->bb_val.bv_val ) );
 
#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "==>backsql_strcat()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */

	va_start( strs, memctx );
	if ( dest->bb_val.bv_val == NULL || dest->bb_len == 0 ) {
		dest->bb_val.bv_val = (char *)ber_memalloc_x( BACKSQL_STR_GROW * sizeof( char ), memctx );
		dest->bb_val.bv_len = 0;
		dest->bb_len = BACKSQL_STR_GROW;
	}
	cdlen = dest->bb_val.bv_len;
	while ( ( cstr = va_arg( strs, char * ) ) != NULL ) {
		cslen = strlen( cstr );
		grow = BACKSQL_MAX( BACKSQL_STR_GROW, cslen );
		if ( dest->bb_len - cdlen <= cslen ) {
			char	*tmp_dest;

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, "backsql_strcat(): "
				"buflen=%d, cdlen=%d, cslen=%d "
				"-- reallocating dest\n",
				dest->bb_len, cdlen + 1, cslen );
#endif /* BACKSQL_TRACE */

			tmp_dest = (char *)ber_memrealloc_x( dest->bb_val.bv_val,
					dest->bb_len + grow * sizeof( char ), memctx );
			if ( tmp_dest == NULL ) {
				Debug( LDAP_DEBUG_ANY, "backsql_strcat(): "
					"could not reallocate string buffer.\n",
					0, 0, 0 );
				return NULL;
			}
			dest->bb_val.bv_val = tmp_dest;
			dest->bb_len += grow;

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, "backsql_strcat(): "
				"new buflen=%d, dest=%p\n",
				dest->bb_len, dest, 0 );
#endif /* BACKSQL_TRACE */
		}
		AC_MEMCPY( dest->bb_val.bv_val + cdlen, cstr, cslen + 1 );
		cdlen += cslen;
	}
	va_end( strs );

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<==backsql_strcat() (dest=\"%s\")\n", 
			dest->bb_val.bv_val, 0, 0 );
#endif /* BACKSQL_TRACE */

	dest->bb_val.bv_len = cdlen;

	return dest;
} 

struct berbuf *
backsql_strfcat_x( struct berbuf *dest, void *memctx, const char *fmt, ... )
{
	va_list		strs;
	ber_len_t	cdlen;

	assert( dest != NULL );
	assert( fmt != NULL );
	assert( dest->bb_len == 0 || dest->bb_len > dest->bb_val.bv_len );
	assert( dest->bb_val.bv_val == NULL 
			|| dest->bb_val.bv_len == strlen( dest->bb_val.bv_val ) );
 
#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "==>backsql_strfcat()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */

	va_start( strs, fmt );
	if ( dest->bb_val.bv_val == NULL || dest->bb_len == 0 ) {
		dest->bb_val.bv_val = (char *)ber_memalloc_x( BACKSQL_STR_GROW * sizeof( char ), memctx );
		dest->bb_val.bv_len = 0;
		dest->bb_len = BACKSQL_STR_GROW;
	}

	cdlen = dest->bb_val.bv_len;
	for ( ; fmt[0]; fmt++ ) {
		ber_len_t	cslen, grow;
		char		*cstr, cc[ 2 ] = { '\0', '\0' };
		struct berval	*cbv;

		switch ( fmt[ 0 ] ) {

		/* berval */
		case 'b':
			cbv = va_arg( strs, struct berval * );
			cstr = cbv->bv_val;
			cslen = cbv->bv_len;
			break;

		/* length + string */
		case 'l':
			cslen = va_arg( strs, ber_len_t );
			cstr = va_arg( strs, char * );
			break;

		/* string */
		case 's':
			cstr = va_arg( strs, char * );
			cslen = strlen( cstr );
			break;

		/* char */
		case 'c':
			/* 
			 * `char' is promoted to `int' when passed through `...'
			 */
			cc[0] = va_arg( strs, int );
			cstr = cc;
			cslen = 1;
			break;

		default:
			assert( 0 );
		}

		grow = BACKSQL_MAX( BACKSQL_STR_GROW, cslen );
		if ( dest->bb_len - cdlen <= cslen ) {
			char	*tmp_dest;

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, "backsql_strfcat(): "
				"buflen=%d, cdlen=%d, cslen=%d "
				"-- reallocating dest\n",
				dest->bb_len, cdlen + 1, cslen );
#endif /* BACKSQL_TRACE */

			tmp_dest = (char *)ber_memrealloc_x( dest->bb_val.bv_val,
					( dest->bb_len ) + grow * sizeof( char ), memctx );
			if ( tmp_dest == NULL ) {
				Debug( LDAP_DEBUG_ANY, "backsql_strfcat(): "
					"could not reallocate string buffer.\n",
					0, 0, 0 );
				return NULL;
			}
			dest->bb_val.bv_val = tmp_dest;
			dest->bb_len += grow * sizeof( char );

#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, "backsql_strfcat(): "
				"new buflen=%d, dest=%p\n", dest->bb_len, dest, 0 );
#endif /* BACKSQL_TRACE */
		}

		assert( cstr != NULL );
		
		AC_MEMCPY( dest->bb_val.bv_val + cdlen, cstr, cslen + 1 );
		cdlen += cslen;
	}

	va_end( strs );

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<==backsql_strfcat() (dest=\"%s\")\n", 
			dest->bb_val.bv_val, 0, 0 );
#endif /* BACKSQL_TRACE */

	dest->bb_val.bv_len = cdlen;

	return dest;
} 

int
backsql_entry_addattr(
	Entry			*e,
	AttributeDescription	*ad,
	struct berval		*val,
	void			*memctx )
{
	int			rc;

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "backsql_entry_addattr(\"%s\"): %s=%s\n", 
		e->e_name.bv_val, ad->ad_cname.bv_val, val->bv_val );
#endif /* BACKSQL_TRACE */

	rc = attr_merge_normalize_one( e, ad, val, memctx );

	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_entry_addattr(\"%s\"): "
			"failed to merge value \"%s\" for attribute \"%s\"\n",
			e->e_name.bv_val, val->bv_val, ad->ad_cname.bv_val );
		return rc;
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<==backsql_entry_addattr(\"%s\")\n",
		e->e_name.bv_val, 0, 0 );
#endif /* BACKSQL_TRACE */

	return LDAP_SUCCESS;
}

static char *
backsql_get_table_spec( backsql_info *bi, char **p )
{
	char		*s, *q;
	struct berbuf	res = BB_NULL;

	assert( p != NULL );
	assert( *p != NULL );

	s = *p;
	while ( **p && **p != ',' ) {
		(*p)++;
	}

	if ( **p ) {
		*(*p)++ = '\0';
	}
	
#define BACKSQL_NEXT_WORD { \
		while ( *s && isspace( (unsigned char)*s ) ) s++; \
		if ( !*s ) return res.bb_val.bv_val; \
		q = s; \
		while ( *q && !isspace( (unsigned char)*q ) ) q++; \
		if ( *q ) *q++='\0'; \
	}

	BACKSQL_NEXT_WORD;
	/* table name */
	backsql_strcat_x( &res, NULL, s, NULL );
	s = q;

	BACKSQL_NEXT_WORD;
	if ( strcasecmp( s, "AS" ) == 0 ) {
		s = q;
		BACKSQL_NEXT_WORD;
	}

	/* oracle doesn't understand "AS" :( and other RDBMSes don't need it */
	backsql_strfcat_x( &res, NULL, "lbbsb",
			STRLENOF( " " ), " ",
			&bi->sql_aliasing,
			&bi->sql_aliasing_quote,
			s,
			&bi->sql_aliasing_quote );

	return res.bb_val.bv_val;
}

int
backsql_merge_from_clause( 
	backsql_info	*bi,
	struct berbuf	*dest_from,
	struct berval	*src_from )
{
	char		*s, *p, *srcc, *pos, e;
	struct berbuf	res = BB_NULL;

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "==>backsql_merge_from_clause(): "
		"dest_from=\"%s\",src_from=\"%s\"\n",
 		dest_from ? dest_from->bb_val.bv_val : "<NULL>",
		src_from->bv_val, 0 );
#endif /* BACKSQL_TRACE */

	srcc = ch_strdup( src_from->bv_val );
	p = srcc;

	if ( dest_from != NULL ) {
		res = *dest_from;
	}
	
	while ( *p ) {
		s = backsql_get_table_spec( bi, &p );

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, "backsql_merge_from_clause(): "
			"p=\"%s\" s=\"%s\"\n", p, s, 0 );
#endif /* BACKSQL_TRACE */

		if ( BER_BVISNULL( &res.bb_val ) ) {
			backsql_strcat_x( &res, NULL, s, NULL );

		} else {
			pos = strstr( res.bb_val.bv_val, s );
			if ( pos == NULL || ( ( e = pos[ strlen( s ) ] ) != '\0' && e != ',' ) ) {
				backsql_strfcat_x( &res, NULL, "cs", ',', s );
			}
		}
		
		if ( s ) {
			ch_free( s );
		}
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<==backsql_merge_from_clause()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */

	free( srcc );
	*dest_from = res;

	return 1;
}

/*
 * splits a pattern in components separated by '?'
 * (double ?? are turned into single ? and left in the string)
 * expected contains the number of expected occurrences of '?'
 * (a negative value means parse as many as possible)
 */

int
backsql_split_pattern(
	const char	*_pattern,
	BerVarray	*split_pattern,
	int		expected )
{
	char		*pattern, *start, *end;
	struct berval	bv;
	int		rc = 0;

#define SPLIT_CHAR	'?'
	
	assert( _pattern != NULL );
	assert( split_pattern != NULL );

	pattern = ch_strdup( _pattern );

	start = pattern;
	end = strchr( start, SPLIT_CHAR );
	for ( ; start; expected-- ) {
		char		*real_end = end;
		ber_len_t	real_len;
		
		if ( real_end == NULL ) {
			real_end = start + strlen( start );

		} else if ( real_end[ 1 ] == SPLIT_CHAR ) {
			expected++;
			AC_MEMCPY( real_end, real_end + 1, strlen( real_end ) );
			end = strchr( real_end + 1, SPLIT_CHAR );
			continue;
		}

		real_len = real_end - start;
		if ( real_len == 0 ) {
			ber_str2bv( "", 0, 1, &bv );
		} else {
			ber_str2bv( start, real_len, 1, &bv );
		}

		ber_bvarray_add( split_pattern, &bv );

		if ( expected == 0 ) {
			if ( end != NULL ) {
				rc = -1;
				goto done;
			}
			break;
		}

		if ( end != NULL ) {
			start = end + 1;
			end = strchr( start, SPLIT_CHAR );
		}
	}

done:;

     	ch_free( pattern );

	return rc;
}

int
backsql_prepare_pattern(
	BerVarray	split_pattern,
	BerVarray	values,
	struct berval	*res )
{
	int		i;
	struct berbuf	bb = BB_NULL;

	assert( res != NULL );

	for ( i = 0; values[i].bv_val; i++ ) {
		if ( split_pattern[i].bv_val == NULL ) {
			ch_free( bb.bb_val.bv_val );
			return -1;
		}
		backsql_strfcat_x( &bb, NULL, "b", &split_pattern[ i ] );
		backsql_strfcat_x( &bb, NULL, "b", &values[ i ] );
	}

	if ( split_pattern[ i ].bv_val == NULL ) {
		ch_free( bb.bb_val.bv_val );
		return -1;
	}

	backsql_strfcat_x( &bb, NULL, "b", &split_pattern[ i ] );

	*res = bb.bb_val;

	return 0;
}

int
backsql_entryUUID(
	backsql_info	*bi,
	backsql_entryID	*id,
	struct berval	*entryUUID,
	void		*memctx )
{
	char		uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
	struct berval	uuid;
#ifdef BACKSQL_ARBITRARY_KEY
	int		i;
	ber_len_t	l, lmax;
#endif /* BACKSQL_ARBITRARY_KEY */

	/* entryUUID is generated as "%08x-%04x-%04x-0000-eaddrXXX"
	 * with eid_oc_id as %08x and hi and lo eid_id as %04x-%04x */
	assert( bi != NULL );
	assert( id != NULL );
	assert( entryUUID != NULL );

#ifdef BACKSQL_ARBITRARY_KEY
	snprintf( uuidbuf, sizeof( uuidbuf ),
			"%08x-0000-0000-0000-000000000000",
			( id->eid_oc_id & 0xFFFFFFFF ) );
	lmax = id->eid_keyval.bv_len < 12 ? id->eid_keyval.bv_len : 12;
	for ( l = 0, i = 9; l < lmax; l++, i += 2 ) {
		switch ( i ) {
		case STRLENOF( "00000000-0000" ):
		case STRLENOF( "00000000-0000-0000" ):
		case STRLENOF( "00000000-0000-0000-0000" ):
			uuidbuf[ i++ ] = '-';
		/* FALLTHRU */

		default:
			snprintf( &uuidbuf[ i ], 3, "%2x", id->eid_keyval.bv_val[ l ] );
			break;
		}
	}
#else /* ! BACKSQL_ARBITRARY_KEY */
	/* note: works only with 32 bit architectures... */
	snprintf( uuidbuf, sizeof( uuidbuf ),
			"%08x-%04x-%04x-0000-000000000000",
			( (unsigned)id->eid_oc_id & 0xFFFFFFFF ),
			( ( (unsigned)id->eid_keyval & 0xFFFF0000 ) >> 020 /* 16 */ ),
			( (unsigned)id->eid_keyval & 0xFFFF ) );
#endif /* ! BACKSQL_ARBITRARY_KEY */

	uuid.bv_val = uuidbuf;
	uuid.bv_len = strlen( uuidbuf );

	ber_dupbv_x( entryUUID, &uuid, memctx );

	return 0;
}

int
backsql_entryUUID_decode(
	struct berval	*entryUUID,
	unsigned long	*oc_id,
#ifdef BACKSQL_ARBITRARY_KEY
	struct berval	*keyval
#else /* ! BACKSQL_ARBITRARY_KEY */
	unsigned long	*keyval
#endif /* ! BACKSQL_ARBITRARY_KEY */
	)
{
#if 0
	fprintf( stderr, "==> backsql_entryUUID_decode()\n" );
#endif

	*oc_id = ( entryUUID->bv_val[0] << 030 /* 24 */ )
		+ ( entryUUID->bv_val[1] << 020 /* 16 */ )
		+ ( entryUUID->bv_val[2] << 010 /* 8 */ )
		+ entryUUID->bv_val[3];

#ifdef BACKSQL_ARBITRARY_KEY
	/* FIXME */
#else /* ! BACKSQL_ARBITRARY_KEY */
	*keyval = ( entryUUID->bv_val[4] << 030 /* 24 */ )
		+ ( entryUUID->bv_val[5] << 020 /* 16 */ )
		+ ( entryUUID->bv_val[6] << 010 /* 8 */ )
		+ entryUUID->bv_val[7];
#endif /* ! BACKSQL_ARBITRARY_KEY */

#if 0
	fprintf( stderr, "<== backsql_entryUUID_decode(): oc=%lu id=%lu\n",
			*oc_id, *keyval );
#endif

	return LDAP_SUCCESS;
}

