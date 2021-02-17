/* search.cpp - tools for slap tools */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software. This work was sponsored by MySQL.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/errno.h>

#include "lutil.h"

#include "back-ndb.h"

static int
ndb_dn2bound(
	NdbIndexScanOperation *myop,
	NdbRdns *rdns
)
{
	unsigned int i;

	/* Walk thru RDNs */
	for ( i=0; i<rdns->nr_num; i++ ) {
		/* Note: RDN_COLUMN offset not needed here */
		if ( myop->setBound( i, NdbIndexScanOperation::BoundEQ, rdns->nr_buf[i] ))
			return LDAP_OTHER;
	}
	return i;
}

/* Check that all filter terms reside in the same table.
 *
 * If any of the filter terms are indexed, then only an IndexScan of the OL_index
 * will be performed. If none are indexed, but all the terms reside in a single
 * table, a Scan can be performed with the LDAP filter transformed into a ScanFilter.
 *
 * Otherwise, a full scan of the DB must be done with all filtering done by slapd.
 */
static int ndb_filter_check( struct ndb_info *ni, Filter *f,
	NdbOcInfo **oci, int *indexed, int *ocfilter )
{
	AttributeDescription *ad = NULL;
	ber_tag_t choice = f->f_choice;
	int rc = 0, undef = 0;

	if ( choice & SLAPD_FILTER_UNDEFINED ) {
		choice &= SLAPD_FILTER_MASK;
		undef = 1;
	}
	switch( choice ) {
	case LDAP_FILTER_AND:
	case LDAP_FILTER_OR:
	case LDAP_FILTER_NOT:
		for ( f = f->f_list; f; f=f->f_next ) {
			rc = ndb_filter_check( ni, f, oci, indexed, ocfilter );
			if ( rc ) return rc;
		}
		break;
	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_SUBSTRINGS:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_APPROX:
		ad = f->f_av_desc;
		break;
	default:
		break;
	}
	if ( ad && !undef ) {
		NdbAttrInfo *ai;
		/* ObjectClass filtering is in dn2id table */
		if ( ad == slap_schema.si_ad_objectClass ) {
			if ( choice == LDAP_FILTER_EQUALITY )
				(*ocfilter)++;
			return 0;
		}
		ai = ndb_ai_find( ni, ad->ad_type );
		if ( ai ) {
			if ( ai->na_flag & NDB_INFO_INDEX )
				(*indexed)++;
			if ( *oci ) {
				if ( ai->na_oi != *oci )
					rc = -1;
			} else {
				*oci = ai->na_oi;
			}
		}
	}
	return rc;
}

static int ndb_filter_set( Operation *op, struct ndb_info *ni, Filter *f, int indexed,
	NdbIndexScanOperation *scan, NdbScanFilter *sf, int *bounds )
{
	AttributeDescription *ad = NULL;
	ber_tag_t choice = f->f_choice;
	int undef = 0;

	if ( choice & SLAPD_FILTER_UNDEFINED ) {
		choice &= SLAPD_FILTER_MASK;
		undef = 1;
	}
	switch( choice ) {
	case LDAP_FILTER_NOT:
		/* no indexing for these */
		break;
	case LDAP_FILTER_OR:
		/* FIXME: these bounds aren't right. */
		if ( indexed ) {
			scan->end_of_bound( (*bounds)++ );
		}
	case LDAP_FILTER_AND:
		if ( sf ) {
			sf->begin( choice == LDAP_FILTER_OR ? NdbScanFilter::OR : NdbScanFilter::AND );
		}
		for ( f = f->f_list; f; f=f->f_next ) {
			if ( ndb_filter_set( op, ni, f, indexed, scan, sf, bounds ))
				return -1;
		}
		if ( sf ) {
			sf->end();
		}
		break;
	case LDAP_FILTER_PRESENT:
		ad = f->f_desc;
		break;
	case LDAP_FILTER_EQUALITY:
	case LDAP_FILTER_SUBSTRINGS:
	case LDAP_FILTER_GE:
	case LDAP_FILTER_LE:
	case LDAP_FILTER_APPROX:
		ad = f->f_av_desc;
		break;
	default:
		break;
	}
	if ( ad && !undef ) {
		NdbAttrInfo *ai;
		/* ObjectClass filtering is in dn2id table */
		if ( ad == slap_schema.si_ad_objectClass ) {
			return 0;
		}
		ai = ndb_ai_find( ni, ad->ad_type );
		if ( ai ) {
			int rc;
			if ( ai->na_flag & NDB_INFO_INDEX ) {
				char *buf, *ptr;
				NdbIndexScanOperation::BoundType bt;

				switch(choice) {
				case LDAP_FILTER_PRESENT:
					rc = scan->setBound( ai->na_ixcol - IDX_COLUMN,
						NdbIndexScanOperation::BoundGT, NULL );
					break;
				case LDAP_FILTER_EQUALITY:
				case LDAP_FILTER_APPROX:
					bt = NdbIndexScanOperation::BoundEQ;
					goto setit;
				case LDAP_FILTER_GE:
					bt = NdbIndexScanOperation::BoundGE;
					goto setit;
				case LDAP_FILTER_LE:
					bt = NdbIndexScanOperation::BoundLE;
				setit:
					rc = f->f_av_value.bv_len+1;
					if ( ai->na_len > 255 )
						rc++;
					buf = (char *)op->o_tmpalloc( rc, op->o_tmpmemctx );
					rc = f->f_av_value.bv_len;
					buf[0] = rc & 0xff;
					ptr = buf+1;
					if ( ai->na_len > 255 ) {
						buf[1] = (rc >> 8);
						ptr++;
					}
					memcpy( ptr, f->f_av_value.bv_val, f->f_av_value.bv_len );
					rc = scan->setBound( ai->na_ixcol - IDX_COLUMN, bt, buf );
					op->o_tmpfree( buf, op->o_tmpmemctx );
					break;
				default:
					break;
				}
			} else if ( sf ) {
				char *buf, *ptr;
				NdbScanFilter::BinaryCondition bc;

				switch(choice) {
				case LDAP_FILTER_PRESENT:
					rc = sf->isnotnull( ai->na_column );
					break;
				case LDAP_FILTER_EQUALITY:
				case LDAP_FILTER_APPROX:
					bc = NdbScanFilter::COND_EQ;
					goto setf;
				case LDAP_FILTER_GE:
					bc = NdbScanFilter::COND_GE;
					goto setf;
				case LDAP_FILTER_LE:
					bc = NdbScanFilter::COND_LE;
				setf:
					rc = sf->cmp( bc, ai->na_column, f->f_av_value.bv_val, f->f_av_value.bv_len );
					break;
				case LDAP_FILTER_SUBSTRINGS:
					rc = 0;
					if ( f->f_sub_initial.bv_val )
						rc += f->f_sub_initial.bv_len + 1;
					if ( f->f_sub_any ) {
						int i;
						if ( !rc ) rc++;
						for (i=0; f->f_sub_any[i].bv_val; i++)
							rc += f->f_sub_any[i].bv_len + 1;
					}
					if ( f->f_sub_final.bv_val ) {
						if ( !rc ) rc++;
						rc += f->f_sub_final.bv_len;
					}
					buf = (char *)op->o_tmpalloc( rc+1, op->o_tmpmemctx );
					ptr = buf;
					if ( f->f_sub_initial.bv_val ) {
						memcpy( ptr, f->f_sub_initial.bv_val, f->f_sub_initial.bv_len );
						ptr += f->f_sub_initial.bv_len;
						*ptr++ = '%';
					}
					if ( f->f_sub_any ) {
						int i;
						if ( ptr == buf )
							*ptr++ = '%';
						for (i=0; f->f_sub_any[i].bv_val; i++) {
							memcpy( ptr, f->f_sub_any[i].bv_val, f->f_sub_any[i].bv_len );
							ptr += f->f_sub_any[i].bv_len;
							*ptr++ = '%';
						}
					}
					if ( f->f_sub_final.bv_val ) {
						if ( ptr == buf )
							*ptr++ = '%';
						memcpy( ptr, f->f_sub_final.bv_val, f->f_sub_final.bv_len );
						ptr += f->f_sub_final.bv_len;
					}
					*ptr = '\0';
					rc = sf->cmp( NdbScanFilter::COND_LIKE, ai->na_column, buf, ptr - buf );
					op->o_tmpfree( buf, op->o_tmpmemctx );
					break;
				}
			}
		}
	}
	return 0;
}

static int ndb_oc_search( Operation *op, SlapReply *rs, Ndb *ndb, NdbTransaction *txn,
	NdbRdns *rbase, NdbOcInfo *oci, int indexed )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	const NdbDictionary::Dictionary *myDict = ndb->getDictionary();
	const NdbDictionary::Table *myTable;
	const NdbDictionary::Index *myIndex;
	NdbIndexScanOperation *scan;
	NdbIndexOperation *ixop;
	NdbScanFilter *sf = NULL;
	struct berval *ocs;
	NdbRecAttr *scanID, *scanOC, *scanDN[NDB_MAX_RDNS];
	char dnBuf[2048], *ptr;
	NdbRdns rdns;
	NdbArgs NA;
	char idbuf[2*sizeof(ID)];
	char ocbuf[NDB_OC_BUFLEN];
	int i, rc, bounds;
	Entry e = {0};
	Uint64 eid;
	time_t stoptime;
	int manageDSAit;

	stoptime = op->o_time + op->ors_tlimit;
	manageDSAit = get_manageDSAit( op );

	myTable = myDict->getTable( oci->no_table.bv_val );
	if ( indexed ) { 
		scan = txn->getNdbIndexScanOperation( INDEX_NAME, DN2ID_TABLE );
		if ( !scan )
			return LDAP_OTHER;
		scan->readTuples( NdbOperation::LM_CommittedRead );
	} else {
		myIndex = myDict->getIndex( "eid$unique", DN2ID_TABLE );
		if ( !myIndex ) {
			Debug( LDAP_DEBUG_ANY, DN2ID_TABLE " eid index is missing!\n", 0, 0, 0 );
			rs->sr_err = LDAP_OTHER;
			goto leave;
		}
		scan = (NdbIndexScanOperation *)txn->getNdbScanOperation( myTable );
		if ( !scan )
			return LDAP_OTHER;
		scan->readTuples( NdbOperation::LM_CommittedRead );
#if 1
		sf = new NdbScanFilter(scan);
		if ( !sf )
			return LDAP_OTHER;
		switch ( op->ors_filter->f_choice ) {
		case LDAP_FILTER_AND:
		case LDAP_FILTER_OR:
		case LDAP_FILTER_NOT:
			break;
		default:
			if ( sf->begin() < 0 ) {
				rc = LDAP_OTHER;
				goto leave;
			}
		}
#endif
	}

	bounds = 0;
	rc = ndb_filter_set( op, ni, op->ors_filter, indexed, scan, sf, &bounds );
	if ( rc )
		goto leave;
	if ( sf ) sf->end();
	
	scanID = scan->getValue( EID_COLUMN, idbuf );
	if ( indexed ) {
		scanOC = scan->getValue( OCS_COLUMN, ocbuf );
		for ( i=0; i<NDB_MAX_RDNS; i++ ) {
			rdns.nr_buf[i][0] = '\0';
			scanDN[i] = scan->getValue( RDN_COLUMN+i, rdns.nr_buf[i] );
		}
	}

	if ( txn->execute( NdbTransaction::NoCommit, NdbOperation::AbortOnError, 1 )) {
		rs->sr_err = LDAP_OTHER;
		goto leave;
	}

	e.e_name.bv_val = dnBuf;
	NA.e = &e;
	NA.ndb = ndb;
	while ( scan->nextResult( true, true ) == 0 ) {
		NdbTransaction *tx2;
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			break;
		}
		if ( slapd_shutdown ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			break;
		}
		if ( op->ors_tlimit != SLAP_NO_LIMIT &&
			slap_get_time() > stoptime ) {
			rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
			break;
		}

		eid = scanID->u_64_value();
		e.e_id = eid;
		if ( !indexed ) {
			tx2 = ndb->startTransaction( myTable );
			if ( !tx2 ) {
				rs->sr_err = LDAP_OTHER;
				goto leave;
			}

			ixop = tx2->getNdbIndexOperation( myIndex );
			if ( !ixop ) {
				tx2->close();
				rs->sr_err = LDAP_OTHER;
				goto leave;
			}
			ixop->readTuple( NdbOperation::LM_CommittedRead );
			ixop->equal( EID_COLUMN, eid );

			scanOC = ixop->getValue( OCS_COLUMN, ocbuf );
			for ( i=0; i<NDB_MAX_RDNS; i++ ) {
				rdns.nr_buf[i][0] = '\0';
				scanDN[i] = ixop->getValue( RDN_COLUMN+i, rdns.nr_buf[i] );
			}
			rc = tx2->execute( NdbTransaction::Commit, NdbOperation::AbortOnError, 1 );
			tx2->close();
			if ( rc ) {
				rs->sr_err = LDAP_OTHER;
				goto leave;
			}
		}

		ocs = ndb_ref2oclist( ocbuf, op->o_tmpmemctx );
		for ( i=0; i<NDB_MAX_RDNS; i++ ) {
			if ( scanDN[i]->isNULL() || !rdns.nr_buf[i][0] )
				break;
		}
		rdns.nr_num = i;

		/* entry must be subordinate to the base */
		if ( i < rbase->nr_num ) {
			continue;
		}

		ptr = dnBuf;
		for ( --i; i>=0; i-- ) {
			char *buf;
			int len;
			buf = rdns.nr_buf[i];
			len = *buf++;
			ptr = lutil_strncopy( ptr, buf, len );
			if ( i ) *ptr++ = ',';
		}
		*ptr = '\0';
		e.e_name.bv_len = ptr - dnBuf;

		/* More scope checks */
		/* If indexed, these can be moved into the ScanFilter */
		switch( op->ors_scope ) {
		case LDAP_SCOPE_ONELEVEL:
			if ( rdns.nr_num != rbase->nr_num+1 )
				continue;
		case LDAP_SCOPE_SUBORDINATE:
			if ( rdns.nr_num == rbase->nr_num )
				continue;
		case LDAP_SCOPE_SUBTREE:
		default:
			if ( e.e_name.bv_len <= op->o_req_dn.bv_len ) {
				if ( op->ors_scope != LDAP_SCOPE_SUBTREE ||
					strcasecmp( op->o_req_dn.bv_val, e.e_name.bv_val ))
					continue;
			} else if ( strcasecmp( op->o_req_dn.bv_val, e.e_name.bv_val +
				e.e_name.bv_len - op->o_req_dn.bv_len ))
				continue;
		}

		dnNormalize( 0, NULL, NULL, &e.e_name, &e.e_nname, op->o_tmpmemctx );
		{
#if 1	/* NDBapi was broken here but seems to work now */
			Ndb::Key_part_ptr keys[2];
			char xbuf[512];
			keys[0].ptr = &eid;
			keys[0].len = sizeof(eid);
			keys[1].ptr = NULL;
			keys[1].len = 0;
			tx2 = ndb->startTransaction( myTable, keys, xbuf, sizeof(xbuf));
#else
			tx2 = ndb->startTransaction( myTable );
#endif
			if ( !tx2 ) {
				rs->sr_err = LDAP_OTHER;
				goto leave;
			}
			NA.txn = tx2;
			NA.ocs = ocs;
			rc = ndb_entry_get_data( op, &NA, 0 );
			tx2->close();
		}
		ber_bvarray_free_x( ocs, op->o_tmpmemctx );
		if ( !manageDSAit && is_entry_referral( &e )) {
			BerVarray erefs = get_entry_referrals( op, &e );
			rs->sr_ref = referral_rewrite( erefs, &e.e_name, NULL,
				op->ors_scope == LDAP_SCOPE_ONELEVEL ?
					LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE );
			rc = send_search_reference( op, rs );
			ber_bvarray_free( rs->sr_ref );
			ber_bvarray_free( erefs );
			rs->sr_ref = NULL;
		} else if ( manageDSAit || !is_entry_glue( &e )) {
			rc = test_filter( op, &e, op->ors_filter );
			if ( rc == LDAP_COMPARE_TRUE ) {
				rs->sr_entry = &e;
				rs->sr_attrs = op->ors_attrs;
				rs->sr_flags = 0;
				rc = send_search_entry( op, rs );
				rs->sr_entry = NULL;
				rs->sr_attrs = NULL;
			} else {
				rc = 0;
			}
		}
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		op->o_tmpfree( e.e_nname.bv_val, op->o_tmpmemctx );
		if ( rc ) break;
	}
leave:
	if ( sf ) delete sf;
	return rc;
}

extern "C"
int ndb_back_search( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	NdbTransaction *txn;
	NdbIndexScanOperation *scan;
	NdbScanFilter *sf = NULL;
	Entry e = {0};
	int rc, i, ocfilter, indexed;
	struct berval matched;
	NdbRecAttr *scanID, *scanOC, *scanDN[NDB_MAX_RDNS];
	char dnBuf[2048], *ptr;
	char idbuf[2*sizeof(ID)];
	char ocbuf[NDB_OC_BUFLEN];
	NdbRdns rdns;
	NdbOcInfo *oci;
	NdbArgs NA;
	slap_mask_t mask;
	time_t stoptime;
	int manageDSAit;

	rc = ndb_thread_handle( op, &NA.ndb );
	rdns.nr_num = 0;

	manageDSAit = get_manageDSAit( op );

	txn = NA.ndb->startTransaction();
	if ( !txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_search) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto leave;
	}

	NA.txn = txn;
	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	NA.e = &e;
	NA.rdns = &rdns;
	NA.ocs = NULL;

	rs->sr_err = ndb_entry_get_info( op, &NA, 0, &matched );
	if ( rs->sr_err ) {
		if ( rs->sr_err == LDAP_NO_SUCH_OBJECT ) {
			rs->sr_matched = matched.bv_val;
			if ( NA.ocs )
				ndb_check_referral( op, rs, &NA );
		}
		goto leave;
	}

	if ( !access_allowed_mask( op, &e, slap_schema.si_ad_entry,
		NULL, ACL_SEARCH, NULL, &mask )) {
		if ( !ACL_GRANT( mask, ACL_DISCLOSE ))
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		else
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		goto leave;
	}

	rs->sr_err = ndb_entry_get_data( op, &NA, 0 );
	ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
	if ( rs->sr_err )
		goto leave;

	if ( !manageDSAit && is_entry_referral( &e )) {
		rs->sr_ref = get_entry_referrals( op, &e );
		rs->sr_err = LDAP_REFERRAL;
		if ( rs->sr_ref )
			rs->sr_flags |= REP_REF_MUSTBEFREED;
		rs->sr_matched = e.e_name.bv_val;
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		goto leave;
	}

	if ( !manageDSAit && is_entry_glue( &e )) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		goto leave;
	}

	if ( get_assert( op ) && test_filter( op, &e, (Filter *)get_assertion( op )) !=
		LDAP_COMPARE_TRUE ) {
		rs->sr_err = LDAP_ASSERTION_FAILED;
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		goto leave;
	}

	/* admin ignores tlimits */
	stoptime = op->o_time + op->ors_tlimit;

	if ( op->ors_scope == LDAP_SCOPE_BASE ) {
		rc = test_filter( op, &e, op->ors_filter );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = &e;
			rs->sr_attrs = op->ors_attrs;
			rs->sr_flags = 0;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
		}
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		rs->sr_err = LDAP_SUCCESS;
		goto leave;
	} else {
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		if ( rdns.nr_num == NDB_MAX_RDNS ) {
			if ( op->ors_scope == LDAP_SCOPE_ONELEVEL ||
				op->ors_scope == LDAP_SCOPE_CHILDREN )
			rs->sr_err = LDAP_SUCCESS;
			goto leave;
		}
	}

	/* See if we can handle the filter. Filtering on objectClass is only done
	 * in the DN2ID table scan. If all other filter terms reside in one table,
	 * then we scan the OC table instead of the DN2ID table.
	 */
	oci = NULL;
	indexed = 0;
	ocfilter = 0;
	rc = ndb_filter_check( ni, op->ors_filter, &oci, &indexed, &ocfilter );
	if ( rc ) {
		Debug( LDAP_DEBUG_TRACE, "ndb_back_search: "
			"filter attributes from multiple tables, indexing ignored\n",
			0, 0, 0 );
	} else if ( oci ) {
		rc = ndb_oc_search( op, rs, NA.ndb, txn, &rdns, oci, indexed );
		goto leave;
	}

	scan = txn->getNdbIndexScanOperation( "PRIMARY", DN2ID_TABLE );
	if ( !scan ) {
		rs->sr_err = LDAP_OTHER;
		goto leave;
	}
	scan->readTuples( NdbOperation::LM_CommittedRead );
	rc = ndb_dn2bound( scan, &rdns );

	/* TODO: if ( ocfilter ) set up scanfilter for objectclass matches
	 * column COND_LIKE "% <class> %"
	 */

	switch( op->ors_scope ) {
	case LDAP_SCOPE_ONELEVEL:
		sf = new NdbScanFilter(scan);
		if ( sf->begin() < 0 ||
			sf->cmp(NdbScanFilter::COND_NOT_LIKE, rc+3, "_%",
				STRLENOF("_%")) < 0 ||
			sf->end() < 0 ) {
			rs->sr_err = LDAP_OTHER;
			goto leave;
		}
		/* FALLTHRU */
	case LDAP_SCOPE_CHILDREN:
		/* Note: RDN_COLUMN offset not needed here */
		scan->setBound( rc, NdbIndexScanOperation::BoundLT, "\0" );
		/* FALLTHRU */
	case LDAP_SCOPE_SUBTREE:
		break;
	}
	scanID = scan->getValue( EID_COLUMN, idbuf );
	scanOC = scan->getValue( OCS_COLUMN, ocbuf );
	for ( i=0; i<NDB_MAX_RDNS; i++ ) {
		rdns.nr_buf[i][0] = '\0';
		scanDN[i] = scan->getValue( RDN_COLUMN+i, rdns.nr_buf[i] );
	}
	if ( txn->execute( NdbTransaction::NoCommit, NdbOperation::AbortOnError, 1 )) {
		rs->sr_err = LDAP_OTHER;
		goto leave;
	}

	e.e_name.bv_val = dnBuf;
	while ( scan->nextResult( true, true ) == 0 ) {
		if ( op->o_abandon ) {
			rs->sr_err = SLAPD_ABANDON;
			break;
		}
		if ( slapd_shutdown ) {
			rs->sr_err = LDAP_UNAVAILABLE;
			break;
		}
		if ( op->ors_tlimit != SLAP_NO_LIMIT &&
			slap_get_time() > stoptime ) {
			rs->sr_err = LDAP_TIMELIMIT_EXCEEDED;
			break;
		}
		e.e_id = scanID->u_64_value();
		NA.ocs = ndb_ref2oclist( ocbuf, op->o_tmpmemctx );
		for ( i=0; i<NDB_MAX_RDNS; i++ ) {
			if ( scanDN[i]->isNULL() || !rdns.nr_buf[i][0] )
				break;
		}
		ptr = dnBuf;
		rdns.nr_num = i;
		for ( --i; i>=0; i-- ) {
			char *buf;
			int len;
			buf = rdns.nr_buf[i];
			len = *buf++;
			ptr = lutil_strncopy( ptr, buf, len );
			if ( i ) *ptr++ = ',';
		}
		*ptr = '\0';
		e.e_name.bv_len = ptr - dnBuf;
		dnNormalize( 0, NULL, NULL, &e.e_name, &e.e_nname, op->o_tmpmemctx );
		NA.txn = NA.ndb->startTransaction();
		rc = ndb_entry_get_data( op, &NA, 0 );
		NA.txn->close();
		ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
		if ( !manageDSAit && is_entry_referral( &e )) {
			BerVarray erefs = get_entry_referrals( op, &e );
			rs->sr_ref = referral_rewrite( erefs, &e.e_name, NULL,
				op->ors_scope == LDAP_SCOPE_ONELEVEL ?
					LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE );
			rc = send_search_reference( op, rs );
			ber_bvarray_free( rs->sr_ref );
			ber_bvarray_free( erefs );
			rs->sr_ref = NULL;
		} else if ( manageDSAit || !is_entry_glue( &e )) {
			rc = test_filter( op, &e, op->ors_filter );
			if ( rc == LDAP_COMPARE_TRUE ) {
				rs->sr_entry = &e;
				rs->sr_attrs = op->ors_attrs;
				rs->sr_flags = 0;
				rc = send_search_entry( op, rs );
				rs->sr_entry = NULL;
			} else {
				rc = 0;
			}
		}
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
		op->o_tmpfree( e.e_nname.bv_val, op->o_tmpmemctx );
		if ( rc ) break;
	}
leave:
	if ( sf )
		delete sf;
	if ( txn )
		txn->close();
	send_ldap_result( op, rs );
	return rs->sr_err;
}

extern NdbInterpretedCode *ndb_lastrow_code;	/* init.cpp */

extern "C" int
ndb_has_children(
	NdbArgs *NA,
	int *hasChildren
)
{
	NdbIndexScanOperation *scan;
	char idbuf[2*sizeof(ID)];
	int rc;

	if ( NA->rdns->nr_num >= NDB_MAX_RDNS ) {
		*hasChildren = LDAP_COMPARE_FALSE;
		return 0;
	}

	scan = NA->txn->getNdbIndexScanOperation( "PRIMARY", DN2ID_TABLE );
	if ( !scan )
		return LDAP_OTHER;
	scan->readTuples( NdbOperation::LM_Read, 0U, 0U, 1U );
	rc = ndb_dn2bound( scan, NA->rdns );
	if ( rc < NDB_MAX_RDNS ) {
		scan->setBound( rc, NdbIndexScanOperation::BoundLT, "\0" );
	}
#if 0
	scan->interpret_exit_last_row();
#else
	scan->setInterpretedCode(ndb_lastrow_code);
#endif
	scan->getValue( EID_COLUMN, idbuf );
	if ( NA->txn->execute( NdbTransaction::NoCommit, NdbOperation::AO_IgnoreError, 1 )) {
		return LDAP_OTHER;
	}
	if (rc < NDB_MAX_RDNS && scan->nextResult( true, true ) == 0 )
		*hasChildren = LDAP_COMPARE_TRUE;
	else
		*hasChildren = LDAP_COMPARE_FALSE;
	scan->close();
	return 0;
}

extern "C" int
ndb_has_subordinates(
	Operation *op,
	Entry *e,
	int *hasSubordinates )
{
	NdbArgs NA;
	NdbRdns rdns;
	int rc;

	NA.rdns = &rdns;
	rc = ndb_dn2rdns( &e->e_nname, &rdns );

	if ( rc == 0 ) {
		rc = ndb_thread_handle( op, &NA.ndb );
		NA.txn = NA.ndb->startTransaction();
		if ( NA.txn ) {
			rc = ndb_has_children( &NA, hasSubordinates );
			NA.txn->close();
		}
	}

	return rc;
}

/*
 * sets the supported operational attributes (if required)
 */
extern "C" int
ndb_operational(
	Operation	*op,
	SlapReply	*rs )
{
	Attribute	**ap;

	assert( rs->sr_entry != NULL );

	for ( ap = &rs->sr_operational_attrs; *ap; ap = &(*ap)->a_next ) {
		if ( (*ap)->a_desc == slap_schema.si_ad_hasSubordinates ) {
			break;
		}
	}

	if ( *ap == NULL &&
		attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_hasSubordinates ) == NULL &&
		( SLAP_OPATTRS( rs->sr_attr_flags ) ||
			ad_inlist( slap_schema.si_ad_hasSubordinates, rs->sr_attrs ) ) )
	{
		int	hasSubordinates, rc;

		rc = ndb_has_subordinates( op, rs->sr_entry, &hasSubordinates );
		if ( rc == LDAP_SUCCESS ) {
			*ap = slap_operational_hasSubordinate( hasSubordinates == LDAP_COMPARE_TRUE );
			assert( *ap != NULL );

			ap = &(*ap)->a_next;
		}
	}

	return LDAP_SUCCESS;
}

