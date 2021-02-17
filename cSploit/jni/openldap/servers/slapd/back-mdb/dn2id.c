/* dn2id.c - routines to deal with the dn2id index */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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
#include <ac/string.h>

#include "back-mdb.h"
#include "idl.h"
#include "lutil.h"

/* Management routines for a hierarchically structured database.
 *
 * Instead of a ldbm-style dn2id database, we use a hierarchical one. Each
 * entry in this database is a struct diskNode, keyed by entryID and with
 * the data containing the RDN and entryID of the node's children. We use
 * a B-Tree with sorted duplicates to store all the children of a node under
 * the same key. Also, the first item under the key contains the entry's own
 * rdn and the ID of the node's parent, to allow bottom-up tree traversal as
 * well as top-down. To keep this info first in the list, the high bit of all
 * subsequent nrdnlen's is always set. This means we can only accomodate
 * RDNs up to length 32767, but that's fine since full DNs are already
 * restricted to 8192.
 *
 * Also each child node contains a count of the number of entries in
 * its subtree, appended after its entryID.
 *
 * The diskNode is a variable length structure. This definition is not
 * directly usable for in-memory manipulation.
 */
typedef struct diskNode {
	unsigned char nrdnlen[2];
	char nrdn[1];
	char rdn[1];                        /* variable placement */
	unsigned char entryID[sizeof(ID)];  /* variable placement */
	/* unsigned char nsubs[sizeof(ID)];	in child nodes only */
} diskNode;

/* Sort function for the sorted duplicate data items of a dn2id key.
 * Sorts based on normalized RDN, in length order.
 */
int
mdb_dup_compare(
	const MDB_val *usrkey,
	const MDB_val *curkey
)
{
	diskNode *un, *cn;
	int rc, nrlen;

	un = (diskNode *)usrkey->mv_data;
	cn = (diskNode *)curkey->mv_data;

	/* data is not aligned, cannot compare directly */
	rc = un->nrdnlen[0] - cn->nrdnlen[0];
	if ( rc ) return rc;
	rc = un->nrdnlen[1] - cn->nrdnlen[1];
	if ( rc ) return rc;

	nrlen = ((un->nrdnlen[0] & 0x7f) << 8) | un->nrdnlen[1];
	return strncmp( un->nrdn, cn->nrdn, nrlen );
}

/* We add two elements to the DN2ID database - a data item under the parent's
 * entryID containing the child's RDN and entryID, and an item under the
 * child's entryID containing the parent's entryID.
 */
int
mdb_dn2id_add(
	Operation	*op,
	MDB_cursor	*mcp,
	MDB_cursor	*mcd,
	ID pid,
	ID nsubs,
	int upsub,
	Entry		*e )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_val		key, data;
	ID		nid;
	int		rc, rlen, nrlen;
	diskNode *d;
	char *ptr;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_dn2id_add 0x%lx: \"%s\"\n",
		e->e_id, e->e_ndn ? e->e_ndn : "", 0 );

	nrlen = dn_rdnlen( op->o_bd, &e->e_nname );
	if (nrlen) {
		rlen = dn_rdnlen( op->o_bd, &e->e_name );
	} else {
		nrlen = e->e_nname.bv_len;
		rlen = e->e_name.bv_len;
	}

	d = op->o_tmpalloc(sizeof(diskNode) + rlen + nrlen + sizeof(ID), op->o_tmpmemctx);
	d->nrdnlen[1] = nrlen & 0xff;
	d->nrdnlen[0] = (nrlen >> 8) | 0x80;
	ptr = lutil_strncopy( d->nrdn, e->e_nname.bv_val, nrlen );
	*ptr++ = '\0';
	ptr = lutil_strncopy( ptr, e->e_name.bv_val, rlen );
	*ptr++ = '\0';
	memcpy( ptr, &e->e_id, sizeof( ID ));
	ptr += sizeof( ID );
	memcpy( ptr, &nsubs, sizeof( ID ));

	key.mv_size = sizeof(ID);
	key.mv_data = &nid;

	nid = pid;

	/* Need to make dummy root node once. Subsequent attempts
	 * will fail harmlessly.
	 */
	if ( pid == 0 ) {
		diskNode dummy = {{0, 0}, "", "", ""};
		data.mv_data = &dummy;
		data.mv_size = sizeof(diskNode);

		mdb_cursor_put( mcp, &key, &data, MDB_NODUPDATA );
	}

	data.mv_data = d;
	data.mv_size = sizeof(diskNode) + rlen + nrlen + sizeof( ID );

	/* Add our child node under parent's key */
	rc = mdb_cursor_put( mcp, &key, &data, MDB_NODUPDATA );

	/* Add our own node */
	if (rc == 0) {
		int flag = MDB_NODUPDATA;
		nid = e->e_id;
		/* drop subtree count */
		data.mv_size -= sizeof( ID );
		ptr -= sizeof( ID );
		memcpy( ptr, &pid, sizeof( ID ));
		d->nrdnlen[0] ^= 0x80;

		if ((slapMode & SLAP_TOOL_MODE) || (e->e_id == mdb->mi_nextid))
			flag |= MDB_APPEND;
		rc = mdb_cursor_put( mcd, &key, &data, flag );
	}
	op->o_tmpfree( d, op->o_tmpmemctx );

	/* Add our subtree count to all superiors */
	if ( rc == 0 && upsub && pid ) {
		ID subs;
		nid = pid;
		do {
			/* Get parent's RDN */
			rc = mdb_cursor_get( mcp, &key, &data, MDB_SET );
			if ( !rc ) {
				char *p2;
				ptr = (char *)data.mv_data + data.mv_size - sizeof( ID );
				memcpy( &nid, ptr, sizeof( ID ));
				/* Get parent's node under grandparent */
				d = data.mv_data;
				rlen = ( d->nrdnlen[0] << 8 ) | d->nrdnlen[1];
				p2 = op->o_tmpalloc( rlen + 2, op->o_tmpmemctx );
				memcpy( p2, data.mv_data, rlen+2 );
				*p2 ^= 0x80;
				data.mv_data = p2;
				rc = mdb_cursor_get( mcp, &key, &data, MDB_GET_BOTH );
				op->o_tmpfree( p2, op->o_tmpmemctx );
				if ( !rc ) {
					/* Get parent's subtree count */
					ptr = (char *)data.mv_data + data.mv_size - sizeof( ID );
					memcpy( &subs, ptr, sizeof( ID ));
					subs += nsubs;
					p2 = op->o_tmpalloc( data.mv_size, op->o_tmpmemctx );
					memcpy( p2, data.mv_data, data.mv_size - sizeof( ID ));
					memcpy( p2+data.mv_size - sizeof( ID ), &subs, sizeof( ID ));
					data.mv_data = p2;
					rc = mdb_cursor_put( mcp, &key, &data, MDB_CURRENT );
					op->o_tmpfree( p2, op->o_tmpmemctx );
				}
			}
			if ( rc )
				break;
		} while ( nid );
	}

	Debug( LDAP_DEBUG_TRACE, "<= mdb_dn2id_add 0x%lx: %d\n", e->e_id, rc, 0 );

	return rc;
}

/* mc must have been set by mdb_dn2id */
int
mdb_dn2id_delete(
	Operation	*op,
	MDB_cursor *mc,
	ID id,
	ID nsubs )
{
	ID nid;
	char *ptr;
	int rc;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_dn2id_delete 0x%lx\n",
		id, 0, 0 );

	/* Delete our ID from the parent's list */
	rc = mdb_cursor_del( mc, 0 );

	/* Delete our ID from the tree. With sorted duplicates, this
	 * will leave any child nodes still hanging around. This is OK
	 * for modrdn, which will add our info back in later.
	 */
	if ( rc == 0 ) {
		MDB_val	key, data;
		if ( nsubs ) {
			mdb_cursor_get( mc, &key, NULL, MDB_GET_CURRENT );
			memcpy( &nid, key.mv_data, sizeof( ID ));
		}
		key.mv_size = sizeof(ID);
		key.mv_data = &id;
		rc = mdb_cursor_get( mc, &key, &data, MDB_SET );
		if ( rc == 0 )
			rc = mdb_cursor_del( mc, 0 );
	}

	/* Delete our subtree count from all superiors */
	if ( rc == 0 && nsubs && nid ) {
		MDB_val key, data;
		ID subs;
		key.mv_data = &nid;
		key.mv_size = sizeof( ID );
		do {
			rc = mdb_cursor_get( mc, &key, &data, MDB_SET );
			if ( !rc ) {
				char *p2;
				diskNode *d;
				int rlen;
				ptr = (char *)data.mv_data + data.mv_size - sizeof( ID );
				memcpy( &nid, ptr, sizeof( ID ));
				/* Get parent's node under grandparent */
				d = data.mv_data;
				rlen = ( d->nrdnlen[0] << 8 ) | d->nrdnlen[1];
				p2 = op->o_tmpalloc( rlen + 2, op->o_tmpmemctx );
				memcpy( p2, data.mv_data, rlen+2 );
				*p2 ^= 0x80;
				data.mv_data = p2;
				rc = mdb_cursor_get( mc, &key, &data, MDB_GET_BOTH );
				op->o_tmpfree( p2, op->o_tmpmemctx );
				if ( !rc ) {
					/* Get parent's subtree count */
					ptr = (char *)data.mv_data + data.mv_size - sizeof( ID );
					memcpy( &subs, ptr, sizeof( ID ));
					subs -= nsubs;
					p2 = op->o_tmpalloc( data.mv_size, op->o_tmpmemctx );
					memcpy( p2, data.mv_data, data.mv_size - sizeof( ID ));
					memcpy( p2+data.mv_size - sizeof( ID ), &subs, sizeof( ID ));
					data.mv_data = p2;
					rc = mdb_cursor_put( mc, &key, &data, MDB_CURRENT );
					op->o_tmpfree( p2, op->o_tmpmemctx );
				}

			}
			if ( rc )
				break;
		} while ( nid );
	}

	Debug( LDAP_DEBUG_TRACE, "<= mdb_dn2id_delete 0x%lx: %d\n", id, rc, 0 );
	return rc;
}

/* return last found ID in *id if no match
 * If mc is provided, it will be left pointing to the RDN's
 * record under the parent's ID. If nsubs is provided, return
 * the number of entries in this entry's subtree.
 */
int
mdb_dn2id(
	Operation	*op,
	MDB_txn *txn,
	MDB_cursor	*mc,
	struct berval	*in,
	ID	*id,
	ID	*nsubs,
	struct berval	*matched,
	struct berval	*nmatched )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_cursor *cursor;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	int		rc = 0, nrlen;
	diskNode *d;
	char	*ptr;
	char dn[SLAP_LDAPDN_MAXLEN];
	ID pid, nid;
	struct berval tmp;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_dn2id(\"%s\")\n", in->bv_val ? in->bv_val : "", 0, 0 );

	if ( matched ) {
		matched->bv_val = dn + sizeof(dn) - 1;
		matched->bv_len = 0;
		*matched->bv_val-- = '\0';
	}
	if ( nmatched ) {
		nmatched->bv_len = 0;
		nmatched->bv_val = 0;
	}

	if ( !in->bv_len ) {
		*id = 0;
		nid = 0;
		goto done;
	}

	tmp = *in;

	if ( op->o_bd->be_nsuffix[0].bv_len ) {
		nrlen = tmp.bv_len - op->o_bd->be_nsuffix[0].bv_len;
		tmp.bv_val += nrlen;
		tmp.bv_len = op->o_bd->be_nsuffix[0].bv_len;
	} else {
		for ( ptr = tmp.bv_val + tmp.bv_len - 1; ptr >= tmp.bv_val; ptr-- )
			if (DN_SEPARATOR(*ptr))
				break;
		ptr++;
		tmp.bv_len -= ptr - tmp.bv_val;
		tmp.bv_val = ptr;
	}
	nid = 0;
	key.mv_size = sizeof(ID);

	if ( mc ) {
		cursor = mc;
	} else {
		rc = mdb_cursor_open( txn, dbi, &cursor );
		if ( rc ) return rc;
	}

	for (;;) {
		key.mv_data = &pid;
		pid = nid;

		data.mv_size = sizeof(diskNode) + tmp.bv_len;
		d = op->o_tmpalloc( data.mv_size, op->o_tmpmemctx );
		d->nrdnlen[1] = tmp.bv_len & 0xff;
		d->nrdnlen[0] = (tmp.bv_len >> 8) | 0x80;
		ptr = lutil_strncopy( d->nrdn, tmp.bv_val, tmp.bv_len );
		*ptr = '\0';
		data.mv_data = d;
		rc = mdb_cursor_get( cursor, &key, &data, MDB_GET_BOTH );
		op->o_tmpfree( d, op->o_tmpmemctx );
		if ( rc )
			break;
		ptr = (char *) data.mv_data + data.mv_size - 2*sizeof(ID);
		memcpy( &nid, ptr, sizeof(ID));

		/* grab the non-normalized RDN */
		if ( matched ) {
			int rlen;
			d = data.mv_data;
			rlen = data.mv_size - sizeof(diskNode) - tmp.bv_len - sizeof(ID);
			matched->bv_len += rlen;
			matched->bv_val -= rlen + 1;
			ptr = lutil_strcopy( matched->bv_val, d->rdn + tmp.bv_len );
			if ( pid ) {
				*ptr = ',';
				matched->bv_len++;
			}
		}
		if ( nmatched ) {
			nmatched->bv_val = tmp.bv_val;
		}

		if ( tmp.bv_val > in->bv_val ) {
			for (ptr = tmp.bv_val - 2; ptr > in->bv_val &&
				!DN_SEPARATOR(*ptr); ptr--)	/* empty */;
			if ( ptr >= in->bv_val ) {
				if (DN_SEPARATOR(*ptr)) ptr++;
				tmp.bv_len = tmp.bv_val - ptr - 1;
				tmp.bv_val = ptr;
			}
		} else {
			break;
		}
	}
	*id = nid; 
	/* return subtree count if requested */
	if ( !rc && nsubs ) {
		ptr = (char *)data.mv_data + data.mv_size - sizeof(ID);
		memcpy( nsubs, ptr, sizeof( ID ));
	}
	if ( !mc )
		mdb_cursor_close( cursor );
done:
	if ( matched ) {
		if ( matched->bv_len ) {
			ptr = op->o_tmpalloc( matched->bv_len+1, op->o_tmpmemctx );
			strcpy( ptr, matched->bv_val );
			matched->bv_val = ptr;
		} else {
			if ( BER_BVISEMPTY( &op->o_bd->be_nsuffix[0] ) && !nid ) {
				ber_dupbv( matched, (struct berval *)&slap_empty_bv );
			} else {
				matched->bv_val = NULL;
			}
		}
	}
	if ( nmatched ) {
		if ( nmatched->bv_val ) {
			nmatched->bv_len = in->bv_len - (nmatched->bv_val - in->bv_val);
		} else {
			*nmatched = slap_empty_bv;
		}
	}

	if( rc != 0 ) {
		Debug( LDAP_DEBUG_TRACE, "<= mdb_dn2id: get failed: %s (%d)\n",
			mdb_strerror( rc ), rc, 0 );
	} else {
		Debug( LDAP_DEBUG_TRACE, "<= mdb_dn2id: got id=0x%lx\n",
			nid, 0, 0 );
	}

	return rc;
}

/* return IDs from root to parent of DN */
int
mdb_dn2sups(
	Operation	*op,
	MDB_txn *txn,
	struct berval	*in,
	ID	*ids )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_cursor *cursor;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	int		rc = 0, nrlen;
	diskNode *d;
	char	*ptr;
	ID pid, nid;
	struct berval tmp;

	Debug( LDAP_DEBUG_TRACE, "=> mdb_dn2sups(\"%s\")\n", in->bv_val, 0, 0 );

	if ( !in->bv_len ) {
		goto done;
	}

	tmp = *in;

	nrlen = tmp.bv_len - op->o_bd->be_nsuffix[0].bv_len;
	tmp.bv_val += nrlen;
	tmp.bv_len = op->o_bd->be_nsuffix[0].bv_len;
	nid = 0;
	key.mv_size = sizeof(ID);

	rc = mdb_cursor_open( txn, dbi, &cursor );
	if ( rc ) return rc;

	for (;;) {
		key.mv_data = &pid;
		pid = nid;

		data.mv_size = sizeof(diskNode) + tmp.bv_len;
		d = op->o_tmpalloc( data.mv_size, op->o_tmpmemctx );
		d->nrdnlen[1] = tmp.bv_len & 0xff;
		d->nrdnlen[0] = (tmp.bv_len >> 8) | 0x80;
		ptr = lutil_strncopy( d->nrdn, tmp.bv_val, tmp.bv_len );
		*ptr = '\0';
		data.mv_data = d;
		rc = mdb_cursor_get( cursor, &key, &data, MDB_GET_BOTH );
		op->o_tmpfree( d, op->o_tmpmemctx );
		if ( rc ) {
			mdb_cursor_close( cursor );
			break;
		}
		ptr = (char *) data.mv_data + data.mv_size - 2*sizeof(ID);
		memcpy( &nid, ptr, sizeof(ID));

		if ( pid )
			mdb_idl_insert( ids, pid );

		if ( tmp.bv_val > in->bv_val ) {
			for (ptr = tmp.bv_val - 2; ptr > in->bv_val &&
				!DN_SEPARATOR(*ptr); ptr--)	/* empty */;
			if ( ptr >= in->bv_val ) {
				if (DN_SEPARATOR(*ptr)) ptr++;
				tmp.bv_len = tmp.bv_val - ptr - 1;
				tmp.bv_val = ptr;
			}
		} else {
			break;
		}
	}

done:
	if( rc != 0 ) {
		Debug( LDAP_DEBUG_TRACE, "<= mdb_dn2sups: get failed: %s (%d)\n",
			mdb_strerror( rc ), rc, 0 );
	}

	return rc;
}

int
mdb_dn2id_children(
	Operation *op,
	MDB_txn *txn,
	Entry *e )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	MDB_cursor	*cursor;
	int		rc;
	ID		id;

	key.mv_size = sizeof(ID);
	key.mv_data = &id;
	id = e->e_id;

	rc = mdb_cursor_open( txn, dbi, &cursor );
	if ( rc ) return rc;

	rc = mdb_cursor_get( cursor, &key, &data, MDB_SET );
	if ( rc == 0 ) {
		size_t dkids;
		rc = mdb_cursor_count( cursor, &dkids );
		if ( rc == 0 ) {
			if ( dkids < 2 ) rc = MDB_NOTFOUND;
		}
	}
	mdb_cursor_close( cursor );
	return rc;
}

int
mdb_id2name(
	Operation *op,
	MDB_txn *txn,
	MDB_cursor **cursp,
	ID id,
	struct berval *name,
	struct berval *nname )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	MDB_cursor	*cursor;
	int		rc, len, nlen;
	char dn[SLAP_LDAPDN_MAXLEN], ndn[SLAP_LDAPDN_MAXLEN], *ptr;
	char *dptr, *nptr;
	diskNode *d;

	key.mv_size = sizeof(ID);

	if ( !*cursp ) {
		rc = mdb_cursor_open( txn, dbi, cursp );
		if ( rc ) return rc;
	}
	cursor = *cursp;

	len = 0;
	nlen = 0;
	dptr = dn;
	nptr = ndn;
	while (id) {
		unsigned int nrlen, rlen;
		key.mv_data = &id;
		data.mv_size = 0;
		data.mv_data = "";
		rc = mdb_cursor_get( cursor, &key, &data, MDB_SET );
		if ( rc ) break;
		ptr = data.mv_data;
		ptr += data.mv_size - sizeof(ID);
		memcpy( &id, ptr, sizeof(ID) );
		d = data.mv_data;
		nrlen = (d->nrdnlen[0] << 8) | d->nrdnlen[1];
		rlen = data.mv_size - sizeof(diskNode) - nrlen;
		assert( nrlen < 1024 && rlen < 1024 );	/* FIXME: Sanity check */
		if (nptr > ndn) {
			*nptr++ = ',';
			*dptr++ = ',';
		}
		/* copy name and trailing NUL */
		memcpy( nptr, d->nrdn, nrlen+1 );
		memcpy( dptr, d->nrdn+nrlen+1, rlen+1 );
		nptr += nrlen;
		dptr += rlen;
	}
	if ( rc == 0 ) {
		name->bv_len = dptr - dn;
		nname->bv_len = nptr - ndn;
		name->bv_val = op->o_tmpalloc( name->bv_len + 1, op->o_tmpmemctx );
		nname->bv_val = op->o_tmpalloc( nname->bv_len + 1, op->o_tmpmemctx );
		memcpy( name->bv_val, dn, name->bv_len );
		name->bv_val[name->bv_len] = '\0';
		memcpy( nname->bv_val, ndn, nname->bv_len );
		nname->bv_val[nname->bv_len] = '\0';
	}
	return rc;
}

/* Find each id in ids that is a child of base and move it to res.
 */
int
mdb_idscope(
	Operation *op,
	MDB_txn *txn,
	ID base,
	ID *ids,
	ID *res )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	MDB_cursor	*cursor;
	ID ida, id, cid = 0, ci0 = 0, idc = 0;
	char	*ptr;
	int		rc, copy;

	key.mv_size = sizeof(ID);

	MDB_IDL_ZERO( res );

	rc = mdb_cursor_open( txn, dbi, &cursor );
	if ( rc ) return rc;

	ida = mdb_idl_first( ids, &cid );

	/* Don't bother moving out of ids if it's a range */
	if (!MDB_IDL_IS_RANGE(ids)) {
		idc = ids[0];
		ci0 = cid;
	}

	while (ida != NOID) {
		copy = 1;
		id = ida;
		while (id) {
			key.mv_data = &id;
			rc = mdb_cursor_get( cursor, &key, &data, MDB_SET );
			if ( rc ) {
				/* not found, drop this from ids */
				copy = 0;
				break;
			}
			ptr = data.mv_data;
			ptr += data.mv_size - sizeof(ID);
			memcpy( &id, ptr, sizeof(ID) );
			if ( id == base ) {
				res[0]++;
				res[res[0]] = ida;
				copy = 0;
				break;
			}
			if ( op->ors_scope == LDAP_SCOPE_ONELEVEL )
				break;
		}
		if (idc) {
			if (copy) {
				if (ci0 != cid)
					ids[ci0] = ids[cid];
				ci0++;
			} else
				idc--;
		}
		ida = mdb_idl_next( ids, &cid );
	}
	if (!MDB_IDL_IS_RANGE( ids ))
		ids[0] = idc;

	mdb_cursor_close( cursor );
	return rc;
}

/* See if base is a child of any of the scopes
 */
int
mdb_idscopes(
	Operation *op,
	IdScopes *isc )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	MDB_dbi dbi = mdb->mi_dn2id;
	MDB_val		key, data;
	ID id;
	ID2 id2;
	char	*ptr;
	int		rc = 0;
	unsigned int x;
	unsigned int nrlen, rlen;
	diskNode *d;

	key.mv_size = sizeof(ID);

	if ( !isc->mc ) {
		rc = mdb_cursor_open( isc->mt, dbi, &isc->mc );
		if ( rc ) return rc;
	}

	id = isc->id;

	/* Catch entries from deref'd aliases */
	x = mdb_id2l_search( isc->scopes, id );
	if ( x <= isc->scopes[0].mid && isc->scopes[x].mid == id ) {
		isc->nscope = x;
		return MDB_SUCCESS;
	}

	while (id) {
		if ( !rc ) {
			key.mv_data = &id;
			rc = mdb_cursor_get( isc->mc, &key, &data, MDB_SET );
			if ( rc )
				return rc;

			/* save RDN info */
		}
		d = data.mv_data;
		nrlen = (d->nrdnlen[0] << 8) | d->nrdnlen[1];
		rlen = data.mv_size - sizeof(diskNode) - nrlen;
		isc->nrdns[isc->numrdns].bv_len = nrlen;
		isc->nrdns[isc->numrdns].bv_val = d->nrdn;
		isc->rdns[isc->numrdns].bv_len = rlen;
		isc->rdns[isc->numrdns].bv_val = d->nrdn+nrlen+1;
		isc->numrdns++;

		if (!rc && id != isc->id) {
			id2.mid = id;
			id2.mval = data;
			mdb_id2l_insert( isc->scopes, &id2 );
		}
		ptr = data.mv_data;
		ptr += data.mv_size - sizeof(ID);
		memcpy( &id, ptr, sizeof(ID) );
		x = mdb_id2l_search( isc->scopes, id );
		if ( x <= isc->scopes[0].mid && isc->scopes[x].mid == id ) {
			if ( !isc->scopes[x].mval.mv_data ) {
				isc->nscope = x;
				return MDB_SUCCESS;
			}
			data = isc->scopes[x].mval;
			rc = 1;
		}
		if ( op->ors_scope == LDAP_SCOPE_ONELEVEL )
			break;
	}
	return MDB_SUCCESS;
}

int
mdb_dn2id_walk(
	Operation *op,
	IdScopes *isc
)
{
	MDB_val key, data;
	diskNode *d;
	char *ptr;
	int rc, n;
	ID nsubs;

	if ( !isc->numrdns ) {
		key.mv_data = &isc->id;
		key.mv_size = sizeof(ID);
		rc = mdb_cursor_get( isc->mc, &key, &data, MDB_SET );
		isc->scopes[0].mid = isc->id;
		isc->numrdns++;
		isc->nscope = 0;
		/* skip base if not a subtree walk */
		if ( isc->oscope == LDAP_SCOPE_SUBTREE ||
			isc->oscope == LDAP_SCOPE_BASE )
			return rc;
	}
	if ( isc->oscope == LDAP_SCOPE_BASE )
		return MDB_NOTFOUND;

	for (;;) {
		/* Get next sibling */
		rc = mdb_cursor_get( isc->mc, &key, &data, MDB_NEXT_DUP );
		if ( !rc ) {
			ptr = (char *)data.mv_data + data.mv_size - 2*sizeof(ID);
			d = data.mv_data;
			memcpy( &isc->id, ptr, sizeof(ID));

			/* If we're pushing down, see if there's any children to find */
			if ( isc->nscope ) {
				ptr += sizeof(ID);
				memcpy( &nsubs, ptr, sizeof(ID));
				/* No children, go to next sibling */
				if ( nsubs < 2 )
					continue;
			}
			n = isc->numrdns;
			isc->scopes[n].mid = isc->id;
			n--;
			isc->nrdns[n].bv_len = ((d->nrdnlen[0] & 0x7f) << 8) | d->nrdnlen[1];
			isc->nrdns[n].bv_val = d->nrdn;
			isc->rdns[n].bv_val = d->nrdn+isc->nrdns[n].bv_len+1;
			isc->rdns[n].bv_len = data.mv_size - sizeof(diskNode) - isc->nrdns[n].bv_len - sizeof(ID);
			/* return this ID to caller */
			if ( !isc->nscope )
				break;

			/* push down to child */
			key.mv_data = &isc->id;
			mdb_cursor_get( isc->mc, &key, &data, MDB_SET );
			isc->nscope = 0;
			isc->numrdns++;
			continue;

		} else if ( rc == MDB_NOTFOUND ) {
			if ( !isc->nscope && isc->oscope != LDAP_SCOPE_ONELEVEL ) {
				/* reset to first dup */
				mdb_cursor_get( isc->mc, &key, NULL, MDB_GET_CURRENT );
				mdb_cursor_get( isc->mc, &key, &data, MDB_SET );
				isc->nscope = 1;
				continue;
			} else {
				isc->numrdns--;
				/* stack is empty? */
				if ( !isc->numrdns )
					break;
				/* pop up to prev node */
				n = isc->numrdns - 1;
				key.mv_data = &isc->scopes[n].mid;
				key.mv_size = sizeof(ID);
				data.mv_data = isc->nrdns[n].bv_val - 2;
				data.mv_size = 1;	/* just needs to be non-zero, mdb_dup_compare doesn't care */
				mdb_cursor_get( isc->mc, &key, &data, MDB_GET_BOTH );
				continue;
			}
		} else {
			break;
		}
	}
	return rc;
}
