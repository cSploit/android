/* entry.c - routines for dealing with entries */
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

#include <ac/ctype.h>
#include <ac/errno.h>
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"
#include "ldif.h"

static char		*ebuf;	/* buf returned by entry2str		 */
static char		*ecur;	/* pointer to end of currently used ebuf */
static int		emaxsize;/* max size of ebuf			 */

/*
 * Empty root entry
 */
const Entry slap_entry_root = {
	NOID, { 0, "" }, { 0, "" }, NULL, 0, { 0, "" }, NULL
};

/*
 * these mutexes must be used when calling the entry2str()
 * routine since it returns a pointer to static data.
 */
ldap_pvt_thread_mutex_t	entry2str_mutex;

static const struct berval dn_bv = BER_BVC("dn");

/*
 * Entry free list
 *
 * Allocate in chunks, minimum of 1000 at a time.
 */
#define	CHUNK_SIZE	1000
typedef struct slap_list {
	struct slap_list *next;
} slap_list;
static slap_list *entry_chunks;
static Entry *entry_list;
static ldap_pvt_thread_mutex_t entry_mutex;

int entry_destroy(void)
{
	slap_list *e;
	if ( ebuf ) free( ebuf );
	ebuf = NULL;
	ecur = NULL;
	emaxsize = 0;

	for ( e=entry_chunks; e; e=entry_chunks ) {
		entry_chunks = e->next;
		free( e );
	}

	ldap_pvt_thread_mutex_destroy( &entry_mutex );
	ldap_pvt_thread_mutex_destroy( &entry2str_mutex );
	return attr_destroy();
}

int
entry_init(void)
{
	ldap_pvt_thread_mutex_init( &entry2str_mutex );
	ldap_pvt_thread_mutex_init( &entry_mutex );
	return attr_init();
}

Entry *
str2entry( char *s )
{
	return str2entry2( s, 1 );
}

#define bvcasematch(bv1, bv2)	(ber_bvstrcasecmp(bv1, bv2) == 0)

Entry *
str2entry2( char *s, int checkvals )
{
	int rc;
	Entry		*e;
	struct berval	*type, *vals, *nvals;
	char 	*freeval;
	AttributeDescription *ad, *ad_prev;
	const char *text;
	char	*next;
	int		attr_cnt;
	int		i, lines;
	Attribute	ahead, *atail;

	/*
	 * LDIF is used as the string format.
	 * An entry looks like this:
	 *
	 *	dn: <dn>\n
	 *	[<attr>:[:] <value>\n]
	 *	[<tab><continuedvalue>\n]*
	 *	...
	 *
	 * If a double colon is used after a type, it means the
	 * following value is encoded as a base 64 string.  This
	 * happens if the value contains a non-printing character
	 * or newline.
	 */

	Debug( LDAP_DEBUG_TRACE, "=> str2entry: \"%s\"\n",
		s ? s : "NULL", 0, 0 );

	e = entry_alloc();

	if( e == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"<= str2entry NULL (entry allocation failed)\n",
			0, 0, 0 );
		return( NULL );
	}

	/* initialize entry */
	e->e_id = NOID;

	/* dn + attributes */
	atail = &ahead;
	ahead.a_next = NULL;
	ad = NULL;
	ad_prev = NULL;
	attr_cnt = 0;
	next = s;

	lines = ldif_countlines( s );
	type = ch_calloc( 1, (lines+1)*3*sizeof(struct berval)+lines );
	vals = type+lines+1;
	nvals = vals+lines+1;
	freeval = (char *)(nvals+lines+1);
	i = -1;

	/* parse into individual values, record DN */
	while ( (s = ldif_getline( &next )) != NULL ) {
		int freev;
		if ( *s == '\n' || *s == '\0' ) {
			break;
		}
		i++;
		if (i >= lines) {
			Debug( LDAP_DEBUG_TRACE,
				"<= str2entry ran past end of entry\n", 0, 0, 0 );
			goto fail;
		}

		rc = ldif_parse_line2( s, type+i, vals+i, &freev );
		freeval[i] = freev;
		if ( rc ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= str2entry NULL (parse_line)\n", 0, 0, 0 );
			continue;
		}

		if ( bvcasematch( &type[i], &dn_bv ) ) {
			if ( e->e_dn != NULL ) {
				Debug( LDAP_DEBUG_ANY, "str2entry: "
					"entry %ld has multiple DNs \"%s\" and \"%s\"\n",
					(long) e->e_id, e->e_dn, vals[i].bv_val );
				goto fail;
			}

			rc = dnPrettyNormal( NULL, &vals[i], &e->e_name, &e->e_nname, NULL );
			if( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "str2entry: "
					"entry %ld has invalid DN \"%s\"\n",
					(long) e->e_id, vals[i].bv_val, 0 );
				goto fail;
			}
			if ( freeval[i] ) free( vals[i].bv_val );
			vals[i].bv_val = NULL;
			i--;
			continue;
		}
	}
	lines = i+1;

	/* check to make sure there was a dn: line */
	if ( BER_BVISNULL( &e->e_name )) {
		Debug( LDAP_DEBUG_ANY, "str2entry: entry %ld has no dn\n",
			(long) e->e_id, 0, 0 );
		goto fail;
	}

	/* Make sure all attributes with multiple values are contiguous */
	if ( checkvals ) {
		int j, k;
		struct berval bv;
		int fv;

		for (i=0; i<lines; i++) {
			for ( j=i+1; j<lines; j++ ) {
				if ( bvcasematch( type+i, type+j )) {
					/* out of order, move intervening attributes down */
					if ( j != i+1 ) {
						bv = vals[j];
						fv = freeval[j];
						for ( k=j; k>i; k-- ) {
							type[k] = type[k-1];
							vals[k] = vals[k-1];
							freeval[k] = freeval[k-1];
						}
						k++;
						type[k] = type[i];
						vals[k] = bv;
						freeval[k] = fv;
					}
					i++;
				}
			}
		}
	}

	if ( lines > 0 ) {
		for ( i=0; i<=lines; i++ ) {
			ad_prev = ad;
			if ( !ad || ( i<lines && !bvcasematch( type+i, &ad->ad_cname ))) {
				ad = NULL;
				rc = slap_bv2ad( type+i, &ad, &text );
	
				if( rc != LDAP_SUCCESS ) {
					Debug( slapMode & SLAP_TOOL_MODE
						? LDAP_DEBUG_ANY : LDAP_DEBUG_TRACE,
						"<= str2entry: str2ad(%s): %s\n", type[i].bv_val, text, 0 );
					if( slapMode & SLAP_TOOL_MODE ) {
						goto fail;
					}
	
					rc = slap_bv2undef_ad( type+i, &ad, &text, 0 );
					if( rc != LDAP_SUCCESS ) {
						Debug( LDAP_DEBUG_ANY,
							"<= str2entry: slap_str2undef_ad(%s): %s\n",
								type[i].bv_val, text, 0 );
						goto fail;
					}
				}
	
				/* require ';binary' when appropriate (ITS#5071) */
				if ( slap_syntax_is_binary( ad->ad_type->sat_syntax ) && !slap_ad_is_binary( ad ) ) {
					Debug( LDAP_DEBUG_ANY,
						"str2entry: attributeType %s #%d: "
						"needs ';binary' transfer as per syntax %s\n", 
						ad->ad_cname.bv_val, 0,
						ad->ad_type->sat_syntax->ssyn_oid );
					goto fail;
				}
			}
	
			if (( ad_prev && ad != ad_prev ) || ( i == lines )) {
				int j, k;
				/* FIXME: we only need this when migrating from an unsorted DB */
				if ( atail != &ahead && atail->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL ) {
					rc = slap_sort_vals( (Modifications *)atail, &text, &j, NULL );
					if ( rc == LDAP_SUCCESS ) {
						atail->a_flags |= SLAP_ATTR_SORTED_VALS;
					} else if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
						Debug( LDAP_DEBUG_ANY,
							"str2entry: attributeType %s value #%d provided more than once\n",
							atail->a_desc->ad_cname.bv_val, j, 0 );
						goto fail;
					}
				}
				atail->a_next = attr_alloc( NULL );
				atail = atail->a_next;
				atail->a_flags = 0;
				atail->a_numvals = attr_cnt;
				atail->a_desc = ad_prev;
				atail->a_vals = ch_malloc( (attr_cnt + 1) * sizeof(struct berval));
				if( ad_prev->ad_type->sat_equality &&
					ad_prev->ad_type->sat_equality->smr_normalize )
					atail->a_nvals = ch_malloc( (attr_cnt + 1) * sizeof(struct berval));
				else
					atail->a_nvals = NULL;
				k = i - attr_cnt;
				for ( j=0; j<attr_cnt; j++ ) {
					if ( freeval[k] )
						atail->a_vals[j] = vals[k];
					else
						ber_dupbv( atail->a_vals+j, &vals[k] );
					vals[k].bv_val = NULL;
					if ( atail->a_nvals ) {
						atail->a_nvals[j] = nvals[k];
						nvals[k].bv_val = NULL;
					}
					k++;
				}
				BER_BVZERO( &atail->a_vals[j] );
				if ( atail->a_nvals ) {
					BER_BVZERO( &atail->a_nvals[j] );
				} else {
					atail->a_nvals = atail->a_vals;
				}
				attr_cnt = 0;
				if ( i == lines ) break;
			}
	
			if ( BER_BVISNULL( &vals[i] ) ) {
				Debug( LDAP_DEBUG_ANY,
					"str2entry: attributeType %s #%d: "
					"no value\n", 
					ad->ad_cname.bv_val, attr_cnt, 0 );
				goto fail;
			}
	
			if( slapMode & SLAP_TOOL_MODE ) {
				struct berval pval;
				slap_syntax_validate_func *validate =
					ad->ad_type->sat_syntax->ssyn_validate;
				slap_syntax_transform_func *pretty =
					ad->ad_type->sat_syntax->ssyn_pretty;
	
				if ( pretty ) {
					rc = ordered_value_pretty( ad,
						&vals[i], &pval, NULL );
	
				} else if ( validate ) {
					/*
				 	 * validate value per syntax
				 	 */
					rc = ordered_value_validate( ad, &vals[i], LDAP_MOD_ADD );
	
				} else {
					Debug( LDAP_DEBUG_ANY,
						"str2entry: attributeType %s #%d: "
						"no validator for syntax %s\n", 
						ad->ad_cname.bv_val, attr_cnt,
						ad->ad_type->sat_syntax->ssyn_oid );
					goto fail;
				}
	
				if( rc != 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"str2entry: invalid value "
						"for attributeType %s #%d (syntax %s)\n",
						ad->ad_cname.bv_val, attr_cnt,
						ad->ad_type->sat_syntax->ssyn_oid );
					goto fail;
				}
	
				if( pretty ) {
					if ( freeval[i] ) free( vals[i].bv_val );
					vals[i] = pval;
					freeval[i] = 1;
				}
			}
	
			if ( ad->ad_type->sat_equality &&
				ad->ad_type->sat_equality->smr_normalize )
			{
				rc = ordered_value_normalize(
					SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
					ad,
					ad->ad_type->sat_equality,
					&vals[i], &nvals[i], NULL );
	
				if ( rc ) {
					Debug( LDAP_DEBUG_ANY,
				   		"<= str2entry NULL (smr_normalize %s %d)\n", ad->ad_cname.bv_val, rc, 0 );
					goto fail;
				}
			}
	
			attr_cnt++;
		}
	}

	free( type );
	atail->a_next = NULL;
	e->e_attrs = ahead.a_next;

	Debug(LDAP_DEBUG_TRACE, "<= str2entry(%s) -> 0x%lx\n",
		e->e_dn, (unsigned long) e, 0 );
	return( e );

fail:
	for ( i=0; i<lines; i++ ) {
		if ( freeval[i] ) free( vals[i].bv_val );
		free( nvals[i].bv_val );
	}
	free( type );
	entry_free( e );
	return NULL;
}


#define GRABSIZE	BUFSIZ

#define MAKE_SPACE( n )	{ \
		while ( ecur + (n) > ebuf + emaxsize ) { \
			ptrdiff_t	offset; \
			offset = (int) (ecur - ebuf); \
			ebuf = ch_realloc( ebuf, \
				emaxsize + GRABSIZE ); \
			emaxsize += GRABSIZE; \
			ecur = ebuf + offset; \
		} \
	}

/* NOTE: only preserved for binary compatibility */
char *
entry2str(
	Entry	*e,
	int		*len )
{
	return entry2str_wrap( e, len, LDIF_LINE_WIDTH );
}

char *
entry2str_wrap(
	Entry		*e,
	int			*len,
	ber_len_t	wrap )
{
	Attribute	*a;
	struct berval	*bv;
	int		i;
	ber_len_t tmplen;

	assert( e != NULL );

	/*
	 * In string format, an entry looks like this:
	 *	dn: <dn>\n
	 *	[<attr>: <value>\n]*
	 */

	ecur = ebuf;

	/* put the dn */
	if ( e->e_dn != NULL ) {
		/* put "dn: <dn>" */
		tmplen = e->e_name.bv_len;
		MAKE_SPACE( LDIF_SIZE_NEEDED( 2, tmplen ));
		ldif_sput_wrap( &ecur, LDIF_PUT_VALUE, "dn", e->e_dn, tmplen, wrap );
	}

	/* put the attributes */
	for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
		/* put "<type>:[:] <value>" line for each value */
		for ( i = 0; a->a_vals[i].bv_val != NULL; i++ ) {
			bv = &a->a_vals[i];
			tmplen = a->a_desc->ad_cname.bv_len;
			MAKE_SPACE( LDIF_SIZE_NEEDED( tmplen, bv->bv_len ));
			ldif_sput_wrap( &ecur, LDIF_PUT_VALUE,
				a->a_desc->ad_cname.bv_val,
				bv->bv_val, bv->bv_len, wrap );
		}
	}
	MAKE_SPACE( 1 );
	*ecur = '\0';
	*len = ecur - ebuf;

	return( ebuf );
}

void
entry_clean( Entry *e )
{
	/* free an entry structure */
	assert( e != NULL );

	/* e_private must be freed by the caller */
	assert( e->e_private == NULL );

	e->e_id = 0;

	/* free DNs */
	if ( !BER_BVISNULL( &e->e_name ) ) {
		free( e->e_name.bv_val );
		BER_BVZERO( &e->e_name );
	}
	if ( !BER_BVISNULL( &e->e_nname ) ) {
		free( e->e_nname.bv_val );
		BER_BVZERO( &e->e_nname );
	}

	if ( !BER_BVISNULL( &e->e_bv ) ) {
		free( e->e_bv.bv_val );
		BER_BVZERO( &e->e_bv );
	}

	/* free attributes */
	if ( e->e_attrs ) {
		attrs_free( e->e_attrs );
		e->e_attrs = NULL;
	}

	e->e_ocflags = 0;
}

void
entry_free( Entry *e )
{
	entry_clean( e );

	ldap_pvt_thread_mutex_lock( &entry_mutex );
	e->e_private = entry_list;
	entry_list = e;
	ldap_pvt_thread_mutex_unlock( &entry_mutex );
}

/* These parameters work well on AMD64 */
#if 0
#define	STRIDE 8
#define	STRIPE 5
#else
#define	STRIDE 1
#define	STRIPE 1
#endif
#define	STRIDE_FACTOR (STRIDE*STRIPE)

int
entry_prealloc( int num )
{
	Entry *e, **prev, *tmp;
	slap_list *s;
	int i, j;

	if (!num) return 0;

#if STRIDE_FACTOR > 1
	/* Round up to our stride factor */
	num += STRIDE_FACTOR-1;
	num /= STRIDE_FACTOR;
	num *= STRIDE_FACTOR;
#endif

	s = ch_calloc( 1, sizeof(slap_list) + num * sizeof(Entry));
	s->next = entry_chunks;
	entry_chunks = s;

	prev = &tmp;
	for (i=0; i<STRIPE; i++) {
		e = (Entry *)(s+1);
		e += i;
		for (j=i; j<num; j+= STRIDE) {
			*prev = e;
			prev = (Entry **)&e->e_private;
			e += STRIDE;
		}
	}
	*prev = entry_list;
	entry_list = (Entry *)(s+1);

	return 0;
}

Entry *
entry_alloc( void )
{
	Entry *e;

	ldap_pvt_thread_mutex_lock( &entry_mutex );
	if ( !entry_list )
		entry_prealloc( CHUNK_SIZE );
	e = entry_list;
	entry_list = e->e_private;
	e->e_private = NULL;
	ldap_pvt_thread_mutex_unlock( &entry_mutex );

	return e;
}


/*
 * These routines are used only by Backend.
 *
 * the Entry has three entry points (ways to find things):
 *
 *	by entry	e.g., if you already have an entry from the cache
 *			and want to delete it. (really by entry ptr)
 *	by dn		e.g., when looking for the base object of a search
 *	by id		e.g., for search candidates
 *
 * these correspond to three different avl trees that are maintained.
 */

int
entry_cmp( Entry *e1, Entry *e2 )
{
	return SLAP_PTRCMP( e1, e2 );
}

int
entry_dn_cmp( const void *v_e1, const void *v_e2 )
{
	/* compare their normalized UPPERCASED dn's */
	const Entry *e1 = v_e1, *e2 = v_e2;

	return ber_bvcmp( &e1->e_nname, &e2->e_nname );
}

int
entry_id_cmp( const void *v_e1, const void *v_e2 )
{
	const Entry *e1 = v_e1, *e2 = v_e2;
	return( e1->e_id < e2->e_id ? -1 : (e1->e_id > e2->e_id ? 1 : 0) );
}

/* This is like a ber_len */
#define entry_lenlen(l)	(((l) < 0x80) ? 1 : ((l) < 0x100) ? 2 : \
	((l) < 0x10000) ? 3 : ((l) < 0x1000000) ? 4 : 5)

static void
entry_putlen(unsigned char **buf, ber_len_t len)
{
	ber_len_t lenlen = entry_lenlen(len);

	if (lenlen == 1) {
		**buf = (unsigned char) len;
	} else {
		int i;
		**buf = 0x80 | ((unsigned char) lenlen - 1);
		for (i=lenlen-1; i>0; i--) {
			(*buf)[i] = (unsigned char) len;
			len >>= 8;
		}
	}
	*buf += lenlen;
}

static ber_len_t
entry_getlen(unsigned char **buf)
{
	ber_len_t len;
	int i;

	len = *(*buf)++;
	if (len <= 0x7f)
		return len;
	i = len & 0x7f;
	len = 0;
	for (;i > 0; i--) {
		len <<= 8;
		len |= *(*buf)++;
	}
	return len;
}

/* Count up the sizes of the components of an entry */
void entry_partsize(Entry *e, ber_len_t *plen,
	int *pnattrs, int *pnvals, int norm)
{
	ber_len_t len, dnlen, ndnlen;
	int i, nat = 0, nval = 0;
	Attribute *a;

	dnlen = e->e_name.bv_len;
	len = dnlen + 1;	/* trailing NUL byte */
	len += entry_lenlen(dnlen);
	if (norm) {
		ndnlen = e->e_nname.bv_len;
		len += ndnlen + 1;
		len += entry_lenlen(ndnlen);
	}
	for (a=e->e_attrs; a; a=a->a_next) {
		/* For AttributeDesc, we only store the attr name */
		nat++;
		len += a->a_desc->ad_cname.bv_len+1;
		len += entry_lenlen(a->a_desc->ad_cname.bv_len);
		for (i=0; a->a_vals[i].bv_val; i++) {
			nval++;
			len += a->a_vals[i].bv_len + 1;
			len += entry_lenlen(a->a_vals[i].bv_len);
		}
		len += entry_lenlen(i);
		nval++;	/* empty berval at end */
		if (norm && a->a_nvals != a->a_vals) {
			for (i=0; a->a_nvals[i].bv_val; i++) {
				nval++;
				len += a->a_nvals[i].bv_len + 1;
				len += entry_lenlen(a->a_nvals[i].bv_len);
			}
			len += entry_lenlen(i);	/* i nvals */
			nval++;
		} else {
			len += entry_lenlen(0);	/* 0 nvals */
		}
	}
	len += entry_lenlen(nat);
	len += entry_lenlen(nval);
	*plen = len;
	*pnattrs = nat;
	*pnvals = nval;
}

/* Add up the size of the entry for a flattened buffer */
ber_len_t entry_flatsize(Entry *e, int norm)
{
	ber_len_t len;
	int nattrs, nvals;

	entry_partsize(e, &len, &nattrs, &nvals, norm);
	len += sizeof(Entry) + (nattrs * sizeof(Attribute)) +
		(nvals * sizeof(struct berval));
	return len;
}

/* Flatten an Entry into a buffer. The buffer is filled with just the
 * strings/bervals of all the entry components. Each field is preceded
 * by its length, encoded the way ber_put_len works. Every field is NUL
 * terminated.  The entire buffer size is precomputed so that a single
 * malloc can be performed. The entry size is also recorded,
 * to aid in entry_decode.
 */
int entry_encode(Entry *e, struct berval *bv)
{
	ber_len_t len, dnlen, ndnlen, i;
	int nattrs, nvals;
	Attribute *a;
	unsigned char *ptr;

	Debug( LDAP_DEBUG_TRACE, "=> entry_encode(0x%08lx): %s\n",
		(long) e->e_id, e->e_dn, 0 );

	dnlen = e->e_name.bv_len;
	ndnlen = e->e_nname.bv_len;

	entry_partsize( e, &len, &nattrs, &nvals, 1 );

	bv->bv_len = len;
	bv->bv_val = ch_malloc(len);
	ptr = (unsigned char *)bv->bv_val;
	entry_putlen(&ptr, nattrs);
	entry_putlen(&ptr, nvals);
	entry_putlen(&ptr, dnlen);
	AC_MEMCPY(ptr, e->e_dn, dnlen);
	ptr += dnlen;
	*ptr++ = '\0';
	entry_putlen(&ptr, ndnlen);
	AC_MEMCPY(ptr, e->e_ndn, ndnlen);
	ptr += ndnlen;
	*ptr++ = '\0';

	for (a=e->e_attrs; a; a=a->a_next) {
		entry_putlen(&ptr, a->a_desc->ad_cname.bv_len);
		AC_MEMCPY(ptr, a->a_desc->ad_cname.bv_val,
			a->a_desc->ad_cname.bv_len);
		ptr += a->a_desc->ad_cname.bv_len;
		*ptr++ = '\0';
		if (a->a_vals) {
			for (i=0; a->a_vals[i].bv_val; i++);
			assert( i == a->a_numvals );
			entry_putlen(&ptr, i);
			for (i=0; a->a_vals[i].bv_val; i++) {
				entry_putlen(&ptr, a->a_vals[i].bv_len);
				AC_MEMCPY(ptr, a->a_vals[i].bv_val,
					a->a_vals[i].bv_len);
				ptr += a->a_vals[i].bv_len;
				*ptr++ = '\0';
			}
			if (a->a_nvals != a->a_vals) {
				entry_putlen(&ptr, i);
				for (i=0; a->a_nvals[i].bv_val; i++) {
					entry_putlen(&ptr, a->a_nvals[i].bv_len);
					AC_MEMCPY(ptr, a->a_nvals[i].bv_val,
					a->a_nvals[i].bv_len);
					ptr += a->a_nvals[i].bv_len;
					*ptr++ = '\0';
				}
			} else {
				entry_putlen(&ptr, 0);
			}
		}
	}

	Debug( LDAP_DEBUG_TRACE, "<= entry_encode(0x%08lx): %s\n",
		(long) e->e_id, e->e_dn, 0 );

	return 0;
}

/* Retrieve an Entry that was stored using entry_encode above.
 * First entry_header must be called to decode the size of the entry.
 * Then a single block of memory must be malloc'd to accomodate the
 * bervals and the bulk data. Next the bulk data is retrieved from
 * the DB and parsed by entry_decode.
 *
 * Note: everything is stored in a single contiguous block, so
 * you can not free individual attributes or names from this
 * structure. Attempting to do so will likely corrupt memory.
 */
int entry_header(EntryHeader *eh)
{
	unsigned char *ptr = (unsigned char *)eh->bv.bv_val;

	/* Some overlays can create empty entries
	 * so don't check for zeros here.
	 */
	eh->nattrs = entry_getlen(&ptr);
	eh->nvals = entry_getlen(&ptr);
	eh->data = (char *)ptr;
	return LDAP_SUCCESS;
}

int
entry_decode_dn( EntryHeader *eh, struct berval *dn, struct berval *ndn )
{
	int i;
	unsigned char *ptr = (unsigned char *)eh->bv.bv_val;

	assert( dn != NULL || ndn != NULL );

	ptr = (unsigned char *)eh->data;
	i = entry_getlen(&ptr);
	if ( dn != NULL ) {
		dn->bv_val = (char *) ptr;
		dn->bv_len = i;
	}

	if ( ndn != NULL ) {
		ptr += i + 1;
		i = entry_getlen(&ptr);
		ndn->bv_val = (char *) ptr;
		ndn->bv_len = i;
	}

	Debug( LDAP_DEBUG_TRACE,
		"entry_decode_dn: \"%s\"\n",
		dn ? dn->bv_val : ndn->bv_val, 0, 0 );

	return 0;
}

#ifdef SLAP_ZONE_ALLOC
int entry_decode(EntryHeader *eh, Entry **e, void *ctx)
#else
int entry_decode(EntryHeader *eh, Entry **e)
#endif
{
	int i, j, nattrs, nvals;
	int rc;
	Attribute *a;
	Entry *x;
	const char *text;
	AttributeDescription *ad;
	unsigned char *ptr = (unsigned char *)eh->bv.bv_val;
	BerVarray bptr;

	nattrs = eh->nattrs;
	nvals = eh->nvals;
	x = entry_alloc();
	x->e_attrs = attrs_alloc( nattrs );
	ptr = (unsigned char *)eh->data;
	i = entry_getlen(&ptr);
	x->e_name.bv_val = (char *) ptr;
	x->e_name.bv_len = i;
	ptr += i+1;
	i = entry_getlen(&ptr);
	x->e_nname.bv_val = (char *) ptr;
	x->e_nname.bv_len = i;
	ptr += i+1;
	Debug( LDAP_DEBUG_TRACE,
		"entry_decode: \"%s\"\n",
		x->e_dn, 0, 0 );
	x->e_bv = eh->bv;

	a = x->e_attrs;
	bptr = (BerVarray)eh->bv.bv_val;

	while ((i = entry_getlen(&ptr))) {
		struct berval bv;
		bv.bv_len = i;
		bv.bv_val = (char *) ptr;
		ad = NULL;
		rc = slap_bv2ad( &bv, &ad, &text );

		if( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"<= entry_decode: str2ad(%s): %s\n", ptr, text, 0 );
			rc = slap_bv2undef_ad( &bv, &ad, &text, 0 );

			if( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY,
					"<= entry_decode: slap_str2undef_ad(%s): %s\n",
						ptr, text, 0 );
				return rc;
			}
		}
		ptr += i + 1;
		a->a_desc = ad;
		a->a_flags = SLAP_ATTR_DONT_FREE_DATA | SLAP_ATTR_DONT_FREE_VALS;
		j = entry_getlen(&ptr);
		a->a_numvals = j;
		a->a_vals = bptr;

		while (j) {
			i = entry_getlen(&ptr);
			bptr->bv_len = i;
			bptr->bv_val = (char *)ptr;
			ptr += i+1;
			bptr++;
			j--;
		}
		bptr->bv_val = NULL;
		bptr->bv_len = 0;
		bptr++;

		j = entry_getlen(&ptr);
		if (j) {
			a->a_nvals = bptr;
			while (j) {
				i = entry_getlen(&ptr);
				bptr->bv_len = i;
				bptr->bv_val = (char *)ptr;
				ptr += i+1;
				bptr++;
				j--;
			}
			bptr->bv_val = NULL;
			bptr->bv_len = 0;
			bptr++;
		} else {
			a->a_nvals = a->a_vals;
		}
		/* FIXME: This is redundant once a sorted entry is saved into the DB */
		if ( a->a_desc->ad_type->sat_flags & SLAP_AT_SORTED_VAL ) {
			rc = slap_sort_vals( (Modifications *)a, &text, &j, NULL );
			if ( rc == LDAP_SUCCESS ) {
				a->a_flags |= SLAP_ATTR_SORTED_VALS;
			} else if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
				/* should never happen */
				Debug( LDAP_DEBUG_ANY,
					"entry_decode: attributeType %s value #%d provided more than once\n",
					a->a_desc->ad_cname.bv_val, j, 0 );
				return rc;
			}
		}
		a = a->a_next;
		nattrs--;
		if ( !nattrs )
			break;
	}

	Debug(LDAP_DEBUG_TRACE, "<= entry_decode(%s)\n",
		x->e_dn, 0, 0 );
	*e = x;
	return 0;
}

Entry *
entry_dup2( Entry *dest, Entry *source )
{
	assert( dest != NULL );
	assert( source != NULL );

	assert( dest->e_private == NULL );

	dest->e_id = source->e_id;
	ber_dupbv( &dest->e_name, &source->e_name );
	ber_dupbv( &dest->e_nname, &source->e_nname );
	dest->e_attrs = attrs_dup( source->e_attrs );
	dest->e_ocflags = source->e_ocflags;

	return dest;
}

Entry *
entry_dup( Entry *e )
{
	return entry_dup2( entry_alloc(), e );
}

#if 1
/* Duplicates an entry using a single malloc. Saves CPU time, increases
 * heap usage because a single large malloc is harder to satisfy than
 * lots of small ones, and the freed space isn't as easily reusable.
 *
 * Probably not worth using this function.
 */
Entry *entry_dup_bv( Entry *e )
{
	ber_len_t len;
	int nattrs, nvals;
	Entry *ret;
	struct berval *bvl;
	char *ptr;
	Attribute *src, *dst;

	ret = entry_alloc();

	entry_partsize(e, &len, &nattrs, &nvals, 1);
	ret->e_id = e->e_id;
	ret->e_attrs = attrs_alloc( nattrs );
	ret->e_ocflags = e->e_ocflags;
	ret->e_bv.bv_len = len + nvals * sizeof(struct berval);
	ret->e_bv.bv_val = ch_malloc( ret->e_bv.bv_len );

	bvl = (struct berval *)ret->e_bv.bv_val;
	ptr = (char *)(bvl + nvals);

	ret->e_name.bv_len = e->e_name.bv_len;
	ret->e_name.bv_val = ptr;
	AC_MEMCPY( ptr, e->e_name.bv_val, e->e_name.bv_len );
	ptr += e->e_name.bv_len;
	*ptr++ = '\0';

	ret->e_nname.bv_len = e->e_nname.bv_len;
	ret->e_nname.bv_val = ptr;
	AC_MEMCPY( ptr, e->e_nname.bv_val, e->e_nname.bv_len );
	ptr += e->e_name.bv_len;
	*ptr++ = '\0';

	dst = ret->e_attrs;
	for (src = e->e_attrs; src; src=src->a_next,dst=dst->a_next ) {
		int i;
		dst->a_desc = src->a_desc;
		dst->a_flags = SLAP_ATTR_DONT_FREE_DATA | SLAP_ATTR_DONT_FREE_VALS;
		dst->a_vals = bvl;
		dst->a_numvals = src->a_numvals;
		for ( i=0; src->a_vals[i].bv_val; i++ ) {
			bvl->bv_len = src->a_vals[i].bv_len;
			bvl->bv_val = ptr;
			AC_MEMCPY( ptr, src->a_vals[i].bv_val, bvl->bv_len );
			ptr += bvl->bv_len;
			*ptr++ = '\0';
			bvl++;
		}
		BER_BVZERO(bvl);
		bvl++;
		if ( src->a_vals != src->a_nvals ) {
			dst->a_nvals = bvl;
			for ( i=0; src->a_nvals[i].bv_val; i++ ) {
				bvl->bv_len = src->a_nvals[i].bv_len;
				bvl->bv_val = ptr;
				AC_MEMCPY( ptr, src->a_nvals[i].bv_val, bvl->bv_len );
				ptr += bvl->bv_len;
				*ptr++ = '\0';
				bvl++;
			}
			BER_BVZERO(bvl);
			bvl++;
		}
	}
	return ret;
}
#endif
