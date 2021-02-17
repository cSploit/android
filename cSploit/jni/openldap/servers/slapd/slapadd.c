/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 2003 IBM Corporation.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Kurt Zeilenga for inclusion
 * in OpenLDAP Software.  Additional signficant contributors include
 *    Jong Hyuk Choi
 *    Pierangelo Masarati
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lber.h>
#include <ldif.h>
#include <lutil.h>
#include <lutil_meter.h>
#include <sys/stat.h>

#include "slapcommon.h"

extern int slap_DN_strict;	/* dn.c */

static char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];

typedef struct Erec {
	Entry *e;
	unsigned long lineno;
	unsigned long nextline;
} Erec;

typedef struct Trec {
	Entry *e;
	unsigned long lineno;
	unsigned long nextline;
	int rc;
	int ready;
} Trec;

static Trec trec;
static unsigned long sid = SLAP_SYNC_SID_MAX + 1;
static int checkvals;
static int enable_meter;
static lutil_meter_t meter;
static const char *progname = "slapadd";
static OperationBuffer opbuf;
static char *buf;
static int lmax;

static ldap_pvt_thread_mutex_t add_mutex;
static ldap_pvt_thread_cond_t add_cond;
static int add_stop;

/* returns:
 *	1: got a record
 *	0: EOF
 * -1: read failure
 * -2: parse failure
 */
static int
getrec0(Erec *erec)
{
	const char *text;
	int ldifrc;
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;
	struct berval csn;
	Operation *op = &opbuf.ob_op;
	op->o_hdr = &opbuf.ob_hdr;

again:
	erec->lineno = erec->nextline+1;
	/* nextline is the line number of the end of the current entry */
	ldifrc = ldif_read_record( ldiffp, &erec->nextline, &buf, &lmax );
	if (ldifrc < 1)
		return ldifrc < 0 ? -1 : 0;
	{
		BackendDB *bd;
		Entry *e;
		int prev_DN_strict;

		if ( erec->lineno < jumpline )
			goto again;

		if ( !dbnum ) {
			prev_DN_strict = slap_DN_strict;
			slap_DN_strict = 0;
		}
		e = str2entry2( buf, checkvals );
		if ( !dbnum ) {
			slap_DN_strict = prev_DN_strict;
		}

		if ( enable_meter )
			lutil_meter_update( &meter,
					 ftell( ldiffp->fp ),
					 0);

		if( e == NULL ) {
			fprintf( stderr, "%s: could not parse entry (line=%lu)\n",
				progname, erec->lineno );
			return -2;
		}

		/* make sure the DN is not empty */
		if( BER_BVISEMPTY( &e->e_nname ) &&
			!BER_BVISEMPTY( be->be_nsuffix ))
		{
			fprintf( stderr, "%s: line %lu: "
				"cannot add entry with empty dn=\"%s\"",
				progname, erec->lineno, e->e_dn );
			bd = select_backend( &e->e_nname, nosubordinates );
			if ( bd ) {
				BackendDB *bdtmp;
				int dbidx = 0;
				LDAP_STAILQ_FOREACH( bdtmp, &backendDB, be_next ) {
					if ( bdtmp == bd ) break;
					dbidx++;
				}

				assert( bdtmp != NULL );
				
				fprintf( stderr, "; did you mean to use database #%d (%s)?",
					dbidx,
					bd->be_suffix[0].bv_val );

			}
			fprintf( stderr, "\n" );
			entry_free( e );
			return -2;
		}

		/* check backend */
		bd = select_backend( &e->e_nname, nosubordinates );
		if ( bd != be ) {
			fprintf( stderr, "%s: line %lu: "
				"database #%d (%s) not configured to hold \"%s\"",
				progname, erec->lineno,
				dbnum,
				be->be_suffix[0].bv_val,
				e->e_dn );
			if ( bd ) {
				BackendDB *bdtmp;
				int dbidx = 0;
				LDAP_STAILQ_FOREACH( bdtmp, &backendDB, be_next ) {
					if ( bdtmp == bd ) break;
					dbidx++;
				}

				assert( bdtmp != NULL );
				
				fprintf( stderr, "; did you mean to use database #%d (%s)?",
					dbidx,
					bd->be_suffix[0].bv_val );

			} else {
				fprintf( stderr, "; no database configured for that naming context" );
			}
			fprintf( stderr, "\n" );
			entry_free( e );
			return -2;
		}

		if ( slap_tool_entry_check( progname, op, e, erec->lineno, &text, textbuf, textlen ) !=
			LDAP_SUCCESS ) {
			entry_free( e );
			return -2;
		}

		if ( SLAP_LASTMOD(be) ) {
			time_t now = slap_get_time();
			char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];
			struct berval vals[ 2 ];

			struct berval name, timestamp;

			struct berval nvals[ 2 ];
			struct berval nname;
			char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];

			enum {
				GOT_NONE = 0x0,
				GOT_CSN = 0x1,
				GOT_UUID = 0x2,
				GOT_ALL = (GOT_CSN|GOT_UUID)
			} got = GOT_ALL;

			vals[1].bv_len = 0;
			vals[1].bv_val = NULL;

			nvals[1].bv_len = 0;
			nvals[1].bv_val = NULL;

			csn.bv_len = ldap_pvt_csnstr( csnbuf, sizeof( csnbuf ), csnsid, 0 );
			csn.bv_val = csnbuf;

			timestamp.bv_val = timebuf;
			timestamp.bv_len = sizeof(timebuf);

			slap_timestamp( &now, &timestamp );

			if ( BER_BVISEMPTY( &be->be_rootndn ) ) {
				BER_BVSTR( &name, SLAPD_ANONYMOUS );
				nname = name;
			} else {
				name = be->be_rootdn;
				nname = be->be_rootndn;
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryUUID )
				== NULL )
			{
				got &= ~GOT_UUID;
				vals[0].bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ) );
				vals[0].bv_val = uuidbuf;
				attr_merge_normalize_one( e, slap_schema.si_ad_entryUUID, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_creatorsName )
				== NULL )
			{
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_creatorsName, vals, nvals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_createTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_createTimestamp, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_entryCSN )
				== NULL )
			{
				got &= ~GOT_CSN;
				vals[0] = csn;
				attr_merge( e, slap_schema.si_ad_entryCSN, vals, NULL );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifiersName )
				== NULL )
			{
				vals[0] = name;
				nvals[0] = nname;
				attr_merge( e, slap_schema.si_ad_modifiersName, vals, nvals );
			}

			if( attr_find( e->e_attrs, slap_schema.si_ad_modifyTimestamp )
				== NULL )
			{
				vals[0] = timestamp;
				attr_merge( e, slap_schema.si_ad_modifyTimestamp, vals, NULL );
			}

			if ( SLAP_SINGLE_SHADOW(be) && got != GOT_ALL ) {
				char buf[SLAP_TEXT_BUFLEN];

				snprintf( buf, sizeof(buf),
					"%s%s%s",
					( !(got & GOT_UUID) ? slap_schema.si_ad_entryUUID->ad_cname.bv_val : "" ),
					( !(got & GOT_CSN) ? "," : "" ),
					( !(got & GOT_CSN) ? slap_schema.si_ad_entryCSN->ad_cname.bv_val : "" ) );

				Debug( LDAP_DEBUG_ANY, "%s: warning, missing attrs %s from entry dn=\"%s\"\n",
					progname, buf, e->e_name.bv_val );
			}

			sid = slap_tool_update_ctxcsn_check( progname, e );
		}
		erec->e = e;
	}
	return 1;
}

static void *
getrec_thr(void *ctx)
{
	ldap_pvt_thread_mutex_lock( &add_mutex );
	while (!add_stop) {
		trec.rc = getrec0((Erec *)&trec);
		trec.ready = 1;
		while (trec.ready)
			ldap_pvt_thread_cond_wait( &add_cond, &add_mutex );
		/* eof or read failure */
		if ( trec.rc == 0 || trec.rc == -1 )
			break;
	}
	ldap_pvt_thread_mutex_unlock( &add_mutex );
	return NULL;
}

static int ldif_threaded;

static int
getrec(Erec *erec)
{
	int rc;
	if ( !ldif_threaded )
		return getrec0(erec);

	while (!trec.ready)
		ldap_pvt_thread_yield();
	erec->e = trec.e;
	erec->lineno = trec.lineno;
	erec->nextline = trec.nextline;
	trec.ready = 0;
	rc = trec.rc;
	ldap_pvt_thread_mutex_lock( &add_mutex );
	ldap_pvt_thread_mutex_unlock( &add_mutex );
	ldap_pvt_thread_cond_signal( &add_cond );
	return rc;
}

int
slapadd( int argc, char **argv )
{
	char textbuf[SLAP_TEXT_BUFLEN] = { '\0' };
	size_t textlen = sizeof textbuf;
	Erec erec;
	struct berval bvtext;
	ldap_pvt_thread_t thr;
	ID id;
	Entry *prev = NULL;

	int ldifrc;
	int rc = EXIT_SUCCESS;

	struct stat stat_buf;

	/* default "000" */
	csnsid = 0;

	if ( isatty (2) ) enable_meter = 1;
	slap_tool_init( progname, SLAPADD, argc, argv );

	if( !be->be_entry_open ||
		!be->be_entry_close ||
		!be->be_entry_put ||
		(update_ctxcsn &&
		 (!be->be_dn2id_get ||
		  !be->be_entry_get ||
		  !be->be_entry_modify)) )
	{
		fprintf( stderr, "%s: database doesn't support necessary operations.\n",
			progname );
		if ( dryrun ) {
			fprintf( stderr, "\t(dry) continuing...\n" );

		} else {
			exit( EXIT_FAILURE );
		}
	}

	checkvals = (slapMode & SLAP_TOOL_QUICK) ? 0 : 1;

	/* do not check values in quick mode */
	if ( slapMode & SLAP_TOOL_QUICK ) {
		if ( slapMode & SLAP_TOOL_VALUE_CHECK ) {
			fprintf( stderr, "%s: value-check incompatible with quick mode; disabled.\n", progname );
			slapMode &= ~SLAP_TOOL_VALUE_CHECK;
		}
	}

	/* enforce schema checking unless not disabled */
	if ( (slapMode & SLAP_TOOL_NO_SCHEMA_CHECK) == 0) {
		SLAP_DBFLAGS(be) &= ~(SLAP_DBFLAG_NO_SCHEMA_CHECK);
	}

	if( !dryrun && be->be_entry_open( be, 1 ) != 0 ) {
		fprintf( stderr, "%s: could not open database.\n",
			progname );
		exit( EXIT_FAILURE );
	}

	(void)slap_tool_update_ctxcsn_init();

	if ( enable_meter
#ifdef LDAP_DEBUG
		/* tools default to "none" */
		&& slap_debug == LDAP_DEBUG_NONE
#endif
		&& !fstat ( fileno ( ldiffp->fp ), &stat_buf )
		&& S_ISREG(stat_buf.st_mode) ) {
		enable_meter = !lutil_meter_open(
			&meter,
			&lutil_meter_text_display,
			&lutil_meter_linear_estimator,
			stat_buf.st_size);
	} else {
		enable_meter = 0;
	}

	if ( slap_tool_thread_max > 1 ) {
		ldap_pvt_thread_mutex_init( &add_mutex );
		ldap_pvt_thread_cond_init( &add_cond );
		ldap_pvt_thread_create( &thr, 0, getrec_thr, NULL );
		ldif_threaded = 1;
	}

	erec.nextline = 0;
	erec.e = NULL;

	for (;;) {
		ldifrc = getrec( &erec );
		if ( ldifrc < 1 ) {
			if ( ldifrc == -2 && continuemode )
				continue;
			break;
		}

		if ( !dryrun ) {
			/*
			 * Initialize text buffer
			 */
			bvtext.bv_len = textlen;
			bvtext.bv_val = textbuf;
			bvtext.bv_val[0] = '\0';

			id = be->be_entry_put( be, erec.e, &bvtext );
			if( id == NOID ) {
				fprintf( stderr, "%s: could not add entry dn=\"%s\" "
								 "(line=%lu): %s\n", progname, erec.e->e_dn,
								 erec.lineno, bvtext.bv_val );
				rc = EXIT_FAILURE;
				if( continuemode ) {
					if ( prev ) entry_free( prev );
					prev = erec.e;
					continue;
				}
				break;
			}
			if ( verbose )
				fprintf( stderr, "added: \"%s\" (%08lx)\n",
					erec.e->e_dn, (long) id );
		} else {
			if ( verbose )
				fprintf( stderr, "added: \"%s\"\n",
					erec.e->e_dn );
		}

		if ( prev ) entry_free( prev );
		prev = erec.e;
	}

	if ( ldif_threaded ) {
		ldap_pvt_thread_mutex_lock( &add_mutex );
		add_stop = 1;
		trec.ready = 0;
		ldap_pvt_thread_cond_signal( &add_cond );
		ldap_pvt_thread_mutex_unlock( &add_mutex );
		ldap_pvt_thread_join( thr, NULL );
	}
	if ( erec.e ) entry_free( erec.e );

	if ( ldifrc < 0 )
		rc = EXIT_FAILURE;

	bvtext.bv_len = textlen;
	bvtext.bv_val = textbuf;
	bvtext.bv_val[0] = '\0';

	if ( enable_meter ) {
		lutil_meter_update( &meter, ftell( ldiffp->fp ), 1);
		lutil_meter_close( &meter );
	}

	if ( rc == EXIT_SUCCESS ) {
		rc = slap_tool_update_ctxcsn( progname, sid, &bvtext );
	}

	ch_free( buf );

	if ( !dryrun ) {
		if ( enable_meter ) {
			fprintf( stderr, "Closing DB..." );
		}
		if( be->be_entry_close( be ) ) {
			rc = EXIT_FAILURE;
		}

		if( be->be_sync ) {
			be->be_sync( be );
		}
		if ( enable_meter ) {
			fprintf( stderr, "\n" );
		}
	}

	if ( slap_tool_destroy())
		rc = EXIT_FAILURE;

	return rc;
}

