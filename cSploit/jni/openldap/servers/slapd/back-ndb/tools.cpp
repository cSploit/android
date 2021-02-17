/* tools.cpp - tools for slap tools */
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

typedef struct dn_id {
	ID id;
	struct berval dn;
} dn_id;

#define	HOLE_SIZE	4096
static dn_id hbuf[HOLE_SIZE], *holes = hbuf;
static unsigned nhmax = HOLE_SIZE;
static unsigned nholes;
static Avlnode *myParents;

static Ndb *myNdb;
static NdbTransaction *myScanTxn;
static NdbIndexScanOperation *myScanOp;

static NdbRecAttr *myScanID, *myScanOC;
static NdbRecAttr *myScanDN[NDB_MAX_RDNS];
static char myDNbuf[2048];
static char myIdbuf[2*sizeof(ID)];
static char myOcbuf[NDB_OC_BUFLEN];
static NdbRdns myRdns;

static NdbTransaction *myPutTxn;
static int myPutCnt;

static struct berval *myOcList;
static struct berval myDn;

extern "C"
int ndb_tool_entry_open(
	BackendDB *be, int mode )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;

	myNdb = new Ndb( ni->ni_cluster[0], ni->ni_dbname );
	return myNdb->init(1024);
}

extern "C"
int ndb_tool_entry_close(
	BackendDB *be )
{
	if ( myPutTxn ) {
		int rc = myPutTxn->execute(NdbTransaction::Commit);
		if( rc != 0 ) {
			char text[1024];
			snprintf( text, sizeof(text),
					"txn_commit failed: %s (%d)",
					myPutTxn->getNdbError().message, myPutTxn->getNdbError().code );
			Debug( LDAP_DEBUG_ANY,
				"=> " LDAP_XSTRING(ndb_tool_entry_put) ": %s\n",
				text, 0, 0 );
		}
		myPutTxn->close();
		myPutTxn = NULL;
	}
	myPutCnt = 0;

	if( nholes ) {
		unsigned i;
		fprintf( stderr, "Error, entries missing!\n");
		for (i=0; i<nholes; i++) {
			fprintf(stderr, "  entry %ld: %s\n",
				holes[i].id, holes[i].dn.bv_val);
		}
		return -1;
	}

	return 0;
}

extern "C"
ID ndb_tool_entry_next(
	BackendDB *be )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	char *ptr;
	ID id;
	int i;

	assert( be != NULL );
	assert( slapMode & SLAP_TOOL_MODE );

	if ( myScanOp->nextResult() ) {
		myScanOp->close();
		myScanOp = NULL;
		myScanTxn->close();
		myScanTxn = NULL;
		return NOID;
	}
	id = myScanID->u_64_value();

	if ( myOcList ) {
		ber_bvarray_free( myOcList );
	}
	myOcList = ndb_ref2oclist( myOcbuf, NULL );
	for ( i=0; i<NDB_MAX_RDNS; i++ ) {
		if ( myScanDN[i]->isNULL() || !myRdns.nr_buf[i][0] )
			break;
	}
	myRdns.nr_num = i;
	ptr = myDNbuf;
	for ( --i; i>=0; i-- ) {
		char *buf;
		int len;
		buf = myRdns.nr_buf[i];
		len = *buf++;
		ptr = lutil_strncopy( ptr, buf, len );
		if ( i )
			*ptr++ = ',';
	}
	*ptr = '\0';
	myDn.bv_val = myDNbuf;
	myDn.bv_len = ptr - myDNbuf;

	return id;
}

extern "C"
ID ndb_tool_entry_first(
	BackendDB *be )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	int i;

	myScanTxn = myNdb->startTransaction();
	if ( !myScanTxn )
		return NOID;

	myScanOp = myScanTxn->getNdbIndexScanOperation( "PRIMARY", DN2ID_TABLE );
	if ( !myScanOp )
		return NOID;

	if ( myScanOp->readTuples( NdbOperation::LM_CommittedRead, NdbScanOperation::SF_KeyInfo ))
		return NOID;

	myScanID = myScanOp->getValue( EID_COLUMN, myIdbuf );
	myScanOC = myScanOp->getValue( OCS_COLUMN, myOcbuf );
	for ( i=0; i<NDB_MAX_RDNS; i++ ) {
		myScanDN[i] = myScanOp->getValue( i+RDN_COLUMN, myRdns.nr_buf[i] );
	}
	if ( myScanTxn->execute( NdbTransaction::NoCommit, NdbOperation::AbortOnError, 1 ))
		return NOID;

	return ndb_tool_entry_next( be );
}

extern "C"
ID ndb_tool_dn2id_get(
	Backend *be,
	struct berval *dn
)
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	NdbArgs NA;
	NdbRdns rdns;
	Entry e;
	char text[1024];
	Operation op = {0};
	Opheader ohdr = {0};
	int rc;

	if ( BER_BVISEMPTY(dn) )
		return 0;

	NA.ndb = myNdb;
	NA.txn = myNdb->startTransaction();
	if ( !NA.txn ) {
		snprintf( text, sizeof(text),
			"startTransaction failed: %s (%d)",
			myNdb->getNdbError().message, myNdb->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> " LDAP_XSTRING(ndb_tool_dn2id_get) ": %s\n",
			 text, 0, 0 );
		return NOID;
	}
	if ( myOcList ) {
		ber_bvarray_free( myOcList );
		myOcList = NULL;
	}
	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	NA.e = &e;
	e.e_name = *dn;
	NA.rdns = &rdns;
	NA.ocs = NULL;
	rc = ndb_entry_get_info( &op, &NA, 0, NULL );
	myOcList = NA.ocs;
	NA.txn->close();
	if ( rc )
		return NOID;
	
	myDn = *dn;

	return e.e_id;
}

extern "C"
Entry* ndb_tool_entry_get( BackendDB *be, ID id )
{
	NdbArgs NA;
	int rc;
	char text[1024];
	Operation op = {0};
	Opheader ohdr = {0};

	assert( be != NULL );
	assert( slapMode & SLAP_TOOL_MODE );

	NA.txn = myNdb->startTransaction();
	if ( !NA.txn ) {
		snprintf( text, sizeof(text),
			"start_transaction failed: %s (%d)",
			myNdb->getNdbError().message, myNdb->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> " LDAP_XSTRING(ndb_tool_entry_get) ": %s\n",
			 text, 0, 0 );
		return NULL;
	}

	NA.e = entry_alloc();
	NA.e->e_id = id;
	ber_dupbv( &NA.e->e_name, &myDn );
	dnNormalize( 0, NULL, NULL, &NA.e->e_name, &NA.e->e_nname, NULL );

	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	NA.ndb = myNdb;
	NA.ocs = myOcList;
	rc = ndb_entry_get_data( &op, &NA, 0 );

	if ( rc ) {
		entry_free( NA.e );
		NA.e = NULL;
	}
	NA.txn->close();

	return NA.e;
}

static struct berval glueval[] = {
	BER_BVC("glue"),
	BER_BVNULL
};

static int ndb_dnid_cmp( const void *v1, const void *v2 )
{
	struct dn_id *dn1 = (struct dn_id *)v1,
		*dn2 = (struct dn_id *)v2;
	return ber_bvcmp( &dn1->dn, &dn2->dn );
}

static int ndb_tool_next_id(
	Operation *op,
	NdbArgs *NA,
	struct berval *text,
	int hole )
{
	struct berval ndn = NA->e->e_nname;
	int rc;

	if (ndn.bv_len == 0) {
		NA->e->e_id = 0;
		return 0;
	}

	rc = ndb_entry_get_info( op, NA, 0, NULL );
	if ( rc ) {
		Attribute *a, tmp = {0};
		if ( !be_issuffix( op->o_bd, &ndn ) ) {
			struct dn_id *dptr;
			struct berval npdn;
			dnParent( &ndn, &npdn );
			NA->e->e_nname = npdn;
			NA->rdns->nr_num--;
			rc = ndb_tool_next_id( op, NA, text, 1 );
			NA->e->e_nname = ndn;
			NA->rdns->nr_num++;
			if ( rc ) {
				return rc;
			}
			/* If parent didn't exist, it was created just now
			 * and its ID is now in e->e_id.
			 */
			dptr = (struct dn_id *)ch_malloc( sizeof( struct dn_id ) + npdn.bv_len + 1);
			dptr->id = NA->e->e_id;
			dptr->dn.bv_val = (char *)(dptr+1);
			strcpy(dptr->dn.bv_val, npdn.bv_val );
			dptr->dn.bv_len = npdn.bv_len;
			if ( avl_insert( &myParents, dptr, ndb_dnid_cmp, avl_dup_error )) {
				ch_free( dptr );
			}
		}
		rc = ndb_next_id( op->o_bd, myNdb, &NA->e->e_id );
		if ( rc ) {
			snprintf( text->bv_val, text->bv_len,
				"next_id failed: %s (%d)",
				myNdb->getNdbError().message, myNdb->getNdbError().code );
			Debug( LDAP_DEBUG_ANY,
				"=> ndb_tool_next_id: %s\n", text->bv_val, 0, 0 );
			return rc;
		}
		if ( hole ) {
			a = NA->e->e_attrs;
			NA->e->e_attrs = &tmp;
			tmp.a_desc = slap_schema.si_ad_objectClass;
			tmp.a_vals = glueval;
			tmp.a_nvals = tmp.a_vals;
			tmp.a_numvals = 1;
		}
		rc = ndb_entry_put_info( op->o_bd, NA, 0 );
		if ( hole ) {
			NA->e->e_attrs = a;
		}
		if ( rc ) {
			snprintf( text->bv_val, text->bv_len, 
				"ndb_entry_put_info failed: %s (%d)",
				myNdb->getNdbError().message, myNdb->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> ndb_tool_next_id: %s\n", text->bv_val, 0, 0 );
		} else if ( hole ) {
			if ( nholes == nhmax - 1 ) {
				if ( holes == hbuf ) {
					holes = (dn_id *)ch_malloc( nhmax * sizeof(dn_id) * 2 );
					AC_MEMCPY( holes, hbuf, sizeof(hbuf) );
				} else {
					holes = (dn_id *)ch_realloc( holes, nhmax * sizeof(dn_id) * 2 );
				}
				nhmax *= 2;
			}
			ber_dupbv( &holes[nholes].dn, &ndn );
			holes[nholes++].id = NA->e->e_id;
		}
	} else if ( !hole ) {
		unsigned i;

		for ( i=0; i<nholes; i++) {
			if ( holes[i].id == NA->e->e_id ) {
				int j;
				free(holes[i].dn.bv_val);
				for (j=i;j<nholes;j++) holes[j] = holes[j+1];
				holes[j].id = 0;
				nholes--;
				rc = ndb_entry_put_info( op->o_bd, NA, 1 );
				break;
			} else if ( holes[i].id > NA->e->e_id ) {
				break;
			}
		}
	}
	return rc;
}

extern "C"
ID ndb_tool_entry_put(
	BackendDB *be,
	Entry *e,
	struct berval *text )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	struct dn_id dtmp, *dptr;
	NdbArgs NA;
	NdbRdns rdns;
	int rc, slow = 0;
	Operation op = {0};
	Opheader ohdr = {0};

	assert( be != NULL );
	assert( slapMode & SLAP_TOOL_MODE );

	assert( text != NULL );
	assert( text->bv_val != NULL );
	assert( text->bv_val[0] == '\0' );	/* overconservative? */

	Debug( LDAP_DEBUG_TRACE, "=> " LDAP_XSTRING(ndb_tool_entry_put)
		"( %ld, \"%s\" )\n", (long) e->e_id, e->e_dn, 0 );

	if ( !be_issuffix( be, &e->e_nname )) {
		dnParent( &e->e_nname, &dtmp.dn );
		dptr = (struct dn_id *)avl_find( myParents, &dtmp, ndb_dnid_cmp );
		if ( !dptr )
			slow = 1;
	}

	rdns.nr_num = 0;

	op.o_hdr = &ohdr;
	op.o_bd = be;
	op.o_tmpmemctx = NULL;
	op.o_tmpmfuncs = &ch_mfuncs;

	if ( !slow ) {
		rc = ndb_next_id( be, myNdb, &e->e_id );
		if ( rc ) {
			snprintf( text->bv_val, text->bv_len,
				"next_id failed: %s (%d)",
				myNdb->getNdbError().message, myNdb->getNdbError().code );
			Debug( LDAP_DEBUG_ANY,
				"=> ndb_tool_next_id: %s\n", text->bv_val, 0, 0 );
			return rc;
		}
	}

	if ( !myPutTxn )
		myPutTxn = myNdb->startTransaction();
	if ( !myPutTxn ) {
		snprintf( text->bv_val, text->bv_len,
			"start_transaction failed: %s (%d)",
			myNdb->getNdbError().message, myNdb->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> " LDAP_XSTRING(ndb_tool_entry_put) ": %s\n",
			 text->bv_val, 0, 0 );
		return NOID;
	}

	/* add dn2id indices */
	ndb_dn2rdns( &e->e_name, &rdns );
	NA.rdns = &rdns;
	NA.e = e;
	NA.ndb = myNdb;
	NA.txn = myPutTxn;
	if ( slow ) {
		rc = ndb_tool_next_id( &op, &NA, text, 0 );
		if( rc != 0 ) {
			goto done;
		}
	} else {
		rc = ndb_entry_put_info( be, &NA, 0 );
		if ( rc != 0 ) {
			goto done;
		}
	}

	/* id2entry index */
	rc = ndb_entry_put_data( be, &NA );
	if( rc != 0 ) {
		snprintf( text->bv_val, text->bv_len,
				"ndb_entry_put_data failed: %s (%d)",
				myNdb->getNdbError().message, myNdb->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> " LDAP_XSTRING(ndb_tool_entry_put) ": %s\n",
			text->bv_val, 0, 0 );
		goto done;
	}

done:
	if( rc == 0 ) {
		myPutCnt++;
		if ( !( myPutCnt & 0x0f )) {
			rc = myPutTxn->execute(NdbTransaction::Commit);
			if( rc != 0 ) {
				snprintf( text->bv_val, text->bv_len,
					"txn_commit failed: %s (%d)",
					myPutTxn->getNdbError().message, myPutTxn->getNdbError().code );
				Debug( LDAP_DEBUG_ANY,
					"=> " LDAP_XSTRING(ndb_tool_entry_put) ": %s\n",
					text->bv_val, 0, 0 );
				e->e_id = NOID;
			}
			myPutTxn->close();
			myPutTxn = NULL;
		}
	} else {
		snprintf( text->bv_val, text->bv_len,
			"txn_aborted! %s (%d)",
			myPutTxn->getNdbError().message, myPutTxn->getNdbError().code );
		Debug( LDAP_DEBUG_ANY,
			"=> " LDAP_XSTRING(ndb_tool_entry_put) ": %s\n",
			text->bv_val, 0, 0 );
		e->e_id = NOID;
		myPutTxn->close();
	}

	return e->e_id;
}

extern "C"
int ndb_tool_entry_reindex(
	BackendDB *be,
	ID id,
	AttributeDescription **adv )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;

	Debug( LDAP_DEBUG_ARGS,
		"=> " LDAP_XSTRING(ndb_tool_entry_reindex) "( %ld )\n",
		(long) id, 0, 0 );

	return 0;
}

extern "C"
ID ndb_tool_entry_modify(
	BackendDB *be,
	Entry *e,
	struct berval *text )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	int rc;

	Debug( LDAP_DEBUG_TRACE,
		"=> " LDAP_XSTRING(ndb_tool_entry_modify) "( %ld, \"%s\" )\n",
		(long) e->e_id, e->e_dn, 0 );

done:
	return e->e_id;
}

