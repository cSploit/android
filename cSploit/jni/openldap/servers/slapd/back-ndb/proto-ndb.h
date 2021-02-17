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

#ifndef _PROTO_NDB_H
#define _PROTO_NDB_H

LDAP_BEGIN_DECL

extern BI_init		ndb_back_initialize;

extern BI_open		ndb_back_open;
extern BI_close		ndb_back_close;
extern BI_destroy	ndb_back_destroy;

extern BI_db_init	ndb_back_db_init;
extern BI_db_destroy	ndb_back_db_destroy;

extern BI_op_bind	ndb_back_bind;
extern BI_op_unbind	ndb_back_unbind;
extern BI_op_search	ndb_back_search;
extern BI_op_compare	ndb_back_compare;
extern BI_op_modify	ndb_back_modify;
extern BI_op_modrdn	ndb_back_modrdn;
extern BI_op_add	ndb_back_add;
extern BI_op_delete	ndb_back_delete;

extern BI_operational	ndb_operational;
extern BI_has_subordinates	ndb_has_subordinates;
extern BI_entry_get_rw	ndb_entry_get;

extern BI_tool_entry_open	ndb_tool_entry_open;
extern BI_tool_entry_close	ndb_tool_entry_close;
extern BI_tool_entry_first	ndb_tool_entry_first;
extern BI_tool_entry_next	ndb_tool_entry_next;
extern BI_tool_entry_get	ndb_tool_entry_get;
extern BI_tool_entry_put	ndb_tool_entry_put;
extern BI_tool_dn2id_get	ndb_tool_dn2id_get;

extern int ndb_modify_internal(
	Operation *op,
	NdbArgs *NA,
	const char **text,
	char *textbuf,
	size_t textlen );

extern int
ndb_entry_get_data(
	Operation *op,
	NdbArgs *args,
	int update );

extern int
ndb_entry_put_data(
	BackendDB *be,
	NdbArgs *args );

extern int
ndb_entry_del_data(
	BackendDB *be,
	NdbArgs *args );

extern int
ndb_entry_put_info(
	BackendDB *be,
	NdbArgs *args,
	int update );

extern int
ndb_entry_get_info(
	Operation *op,
	NdbArgs *args,
	int update,
	struct berval *matched );

extern "C" int
ndb_entry_del_info(
	BackendDB *be,
	NdbArgs *args );

extern int
ndb_dn2rdns(
	struct berval *dn,
	NdbRdns *buf );

extern NdbAttrInfo *
ndb_ai_find( struct ndb_info *ni, AttributeType *at );

extern NdbAttrInfo *
ndb_ai_get( struct ndb_info *ni, struct berval *at );

extern int
ndb_aset_get( struct ndb_info *ni, struct berval *sname, struct berval *attrs, NdbOcInfo **ret );

extern int
ndb_aset_create( struct ndb_info *ni, NdbOcInfo *oci );

extern int
ndb_oc_read( struct ndb_info *ni, const NdbDictionary::Dictionary *dict );

extern int
ndb_oc_attrs(
	NdbTransaction *txn,
	const NdbDictionary::Table *myTable,
	Entry *e,
	NdbOcInfo *no,
	NdbAttrInfo **attrs,
	int nattrs,
	Attribute *old );

extern int
ndb_has_children(
	NdbArgs *NA,
	int *hasChildren );

extern struct berval *
ndb_str2bvarray(
	char *str,
	int len,
	char delim,
	void *ctx );

extern struct berval *
ndb_ref2oclist(
	const char *ref,
	void *ctx );

extern int
ndb_next_id(
	BackendDB *be,
	Ndb *ndb,
	ID *id );

extern int
ndb_thread_handle(
	Operation *op,
	Ndb **ndb );

extern int
ndb_back_init_cf(
	BackendInfo *bi );

extern "C" void
ndb_trans_backoff( int num_retries );

extern "C" void
ndb_check_referral( Operation *op, SlapReply *rs, NdbArgs *NA );

LDAP_END_DECL

#endif /* _PROTO_NDB_H */
