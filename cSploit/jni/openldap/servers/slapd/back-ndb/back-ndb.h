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

#ifndef SLAPD_NDB_H
#define SLAPD_NDB_H

#include "slap.h"

#include <mysql.h>
#include <NdbApi.hpp>

LDAP_BEGIN_DECL

/* The general design is to use one relational table per objectclass. This is
 * complicated by objectclass inheritance and auxiliary classes though.
 *
 * Attributes must only occur in a single table. For objectclasses that inherit
 * from other classes, attributes defined in the superior class are only stored
 * in the superior class' table. When multiple unrelated classes define the same
 * attributes, an attributeSet should be defined instead, containing all of the
 * common attributes.
 *
 * The no_set table lists which other attributeSets apply to the current
 * objectClass. The no_attrs table lists all of the non-inherited attributes of
 * the class, including those residing in an attributeSet.
 *
 * Usually the table is named identically to the objectClass, but it can also
 * be explicitly named something else if needed.
 */
#define NDB_MAX_OCSETS	8

struct ndb_attrinfo;

typedef struct ndb_ocinfo {
	struct berval no_name;	/* objectclass cname */
	struct berval no_table;
	ObjectClass *no_oc;
	struct ndb_ocinfo *no_sets[NDB_MAX_OCSETS];
	struct ndb_attrinfo **no_attrs;
	int no_flag;
	int no_nsets;
	int no_nattrs;
} NdbOcInfo;

#define	NDB_INFO_ATLEN	0x01
#define	NDB_INFO_ATSET	0x02
#define	NDB_INFO_INDEX	0x04
#define	NDB_INFO_ATBLOB	0x08

typedef struct ndb_attrinfo {
	struct berval na_name;	/* attribute cname */
	AttributeDescription *na_desc;
	AttributeType *na_attr;
	NdbOcInfo *na_oi;
	int na_flag;
	int na_len;
	int na_column;
	int na_ixcol;
} NdbAttrInfo;

typedef struct ListNode {
	struct ListNode *ln_next;
	void *ln_data;
} ListNode;

#define	NDB_IS_OPEN(ni)	(ni->ni_cluster != NULL)

struct ndb_info {
	/* NDB connection */
	char *ni_connectstr;
	char *ni_dbname;
	Ndb_cluster_connection **ni_cluster;

	/* MySQL connection parameters */
	MYSQL ni_sql;
	char *ni_hostname;
	char *ni_username;
	char *ni_password;
	char *ni_socket;
	unsigned long ni_clflag;
	unsigned int ni_port;

	/* Search filter processing */
	int ni_search_stack_depth;
	void *ni_search_stack;

#define	DEFAULT_SEARCH_STACK_DEPTH	16
#define	MINIMUM_SEARCH_STACK_DEPTH	8

	/* Schema config */
	NdbOcInfo *ni_opattrs;
	ListNode *ni_attridxs;
	ListNode *ni_attrlens;
	ListNode *ni_attrsets;
	ListNode *ni_attrblobs;
	ldap_pvt_thread_rdwr_t ni_ai_rwlock;
	Avlnode *ni_ai_tree;
	ldap_pvt_thread_rdwr_t ni_oc_rwlock;
	Avlnode *ni_oc_tree;
	int ni_nconns;	/* number of connections to open */
	int ni_nextconn;	/* next conn to use */
	ldap_pvt_thread_mutex_t ni_conn_mutex;
};

#define	NDB_MAX_RDNS	16
#define	NDB_RDN_LEN	128
#define	NDB_MAX_OCS	64

#define	DN2ID_TABLE	"OL_dn2id"
#define	EID_COLUMN	0U
#define	VID_COLUMN	1U
#define	OCS_COLUMN	1U
#define	RDN_COLUMN	2U
#define	IDX_COLUMN	(2U+NDB_MAX_RDNS)

#define	NEXTID_TABLE	"OL_nextid"

#define	NDB_OC_BUFLEN	1026	/* 1024 data plus 2 len bytes */

#define	INDEX_NAME	"OL_index"

typedef struct NdbRdns {
	short nr_num;
	char nr_buf[NDB_MAX_RDNS][NDB_RDN_LEN+1];
} NdbRdns;

typedef struct NdbOcs {
	int no_ninfo;
	int no_ntext;
	int no_nitext;	/* number of implicit classes */
	NdbOcInfo *no_info[NDB_MAX_OCS];
	struct berval no_text[NDB_MAX_OCS];
	struct berval no_itext[NDB_MAX_OCS];	/* implicit classes */
} NdbOcs;

typedef struct NdbArgs {
	Ndb *ndb;
	NdbTransaction *txn;
	Entry *e;
	NdbRdns *rdns;
	struct berval *ocs;
	int erdns;
} NdbArgs;

#define	NDB_NO_SUCH_OBJECT	626
#define	NDB_ALREADY_EXISTS	630

LDAP_END_DECL

#include "proto-ndb.h"

#endif
