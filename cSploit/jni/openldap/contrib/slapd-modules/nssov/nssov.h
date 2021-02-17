/* nssov.h - NSS overlay header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 Howard Chu.
 * Portions Copyright 2013 Ted C. Cheng, Symas Corp.
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

#ifndef NSSOV_H
#define NSSOV_H

#ifndef NSLCD_PATH
#define	NSLCD_PATH	"/var/run/nslcd"
#endif

#ifndef NSLCD_SOCKET
#define NSLCD_SOCKET	NSLCD_PATH "/socket"
#endif

#include <stdio.h>

#include "nslcd.h"
#include "nslcd-prot.h"
#include "tio.h"
#include "attrs.h"

#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "portable.h"
#include "slap.h"
#include <ac/string.h>

/* selectors for different maps */
enum nssov_map_selector
{
  NM_alias,
  NM_ether,
  NM_group,
  NM_host,
  NM_netgroup,
  NM_network,
  NM_passwd,
  NM_protocol,
  NM_rpc,
  NM_service,
  NM_shadow,
  NM_NONE
};

typedef struct nssov_mapinfo {
	struct berval mi_base;
	int mi_scope;
	struct berval mi_filter0;
	struct berval mi_filter;
	struct berval *mi_attrkeys;
	AttributeName *mi_attrs;
} nssov_mapinfo;

typedef struct nssov_info
{
	/* search timelimit */
	int ni_timelimit;
	struct nssov_mapinfo ni_maps[NM_NONE];
	int ni_socket;
	Connection *ni_conn;
	BackendDB *ni_db;

	/* PAM authz support... */
	slap_mask_t ni_pam_opts;
	struct berval ni_pam_group_dn;
	AttributeDescription *ni_pam_group_ad;
	int ni_pam_min_uid;
	int ni_pam_max_uid;
	AttributeDescription *ni_pam_template_ad;
	struct berval ni_pam_template;
	struct berval ni_pam_defhost;
	struct berval *ni_pam_sessions;
	struct berval ni_pam_password_prohibit_message;
	struct berval ni_pam_pwdmgr_dn;
	struct berval ni_pam_pwdmgr_pwd;
} nssov_info;

#define NI_PAM_USERHOST		1	/* old style host checking */
#define NI_PAM_USERSVC		2	/* old style service checking */
#define NI_PAM_USERGRP		4	/* old style group checking */
#define NI_PAM_HOSTSVC		8	/* new style authz checking */
#define NI_PAM_SASL2DN		0x10	/* use sasl2dn */
#define NI_PAM_UID2DN		0x20	/* use uid2dn */

#define	NI_PAM_OLD	(NI_PAM_USERHOST|NI_PAM_USERSVC|NI_PAM_USERGRP)
#define	NI_PAM_NEW	NI_PAM_HOSTSVC

extern AttributeDescription *nssov_pam_host_ad;
extern AttributeDescription *nssov_pam_svc_ad;

/* Read the default configuration file. */
void nssov_cfg_init(nssov_info *ni,const char *fname);

/* macros for basic read and write operations, the following
   ERROR_OUT* marcos define the action taken on errors
   the stream is not closed because the caller closes the
   stream */

#define ERROR_OUT_WRITEERROR(fp) \
  Debug(LDAP_DEBUG_ANY,"nssov: error writing to client\n",0,0,0); \
  return -1;

#define ERROR_OUT_READERROR(fp) \
  Debug(LDAP_DEBUG_ANY,"nssov: error reading from client\n",0,0,0); \
  return -1;

#define ERROR_OUT_BUFERROR(fp) \
  Debug(LDAP_DEBUG_ANY,"nssov: client supplied argument too large\n",0,0,0); \
  return -1;

#define WRITE_BERVAL(fp,bv) \
  DEBUG_PRINT("WRITE_STRING: var="__STRING(bv)" string=\"%s\"",(bv)->bv_val); \
  if ((bv)==NULL) \
  { \
    WRITE_INT32(fp,0); \
  } \
  else \
  { \
    WRITE_INT32(fp,(bv)->bv_len); \
    if (tmpint32>0) \
      { WRITE(fp,(bv)->bv_val,tmpint32); } \
  }

#define WRITE_BVARRAY(fp,arr) \
  /* first determine length of array */ \
  for (tmp3int32=0;(arr)[tmp3int32].bv_val!=NULL;tmp3int32++) \
    /*nothing*/ ; \
  /* write number of strings */ \
  DEBUG_PRINT("WRITE_BVARRAY: var="__STRING(arr)" num=%d",(int)tmp3int32); \
  WRITE_TYPE(fp,tmp3int32,int32_t); \
  /* write strings */ \
  for (tmp2int32=0;tmp2int32<tmp3int32;tmp2int32++) \
  { \
    WRITE_BERVAL(fp,&(arr)[tmp2int32]); \
  }

/* This tries to get the user password attribute from the entry.
   It will try to return an encrypted password as it is used in /etc/passwd,
   /etc/group or /etc/shadow depending upon what is in the directory.
   This function will return NULL if no passwd is found and will return the
   literal value in the directory if conversion is not possible. */
void get_userpassword(struct berval *attr, struct berval *pw);

/* write out an address, parsing the addr value */
int write_address(TFILE *fp,struct berval *addr);

/* a helper macro to write out addresses and bail out on errors */
#define WRITE_ADDRESS(fp,addr) \
  if (write_address(fp,addr)) \
    return -1;

/* read an address from the stream */
int read_address(TFILE *fp,char *addr,int *addrlen,int *af);

/* helper macro to read an address from the stream */
#define READ_ADDRESS(fp,addr,len,af) \
  len=(int)sizeof(addr); \
  if (read_address(fp,addr,&(len),&(af))) \
    return -1;

/* checks to see if the specified string is a valid username */
int isvalidusername(struct berval *name);

/* transforms the DN into a uid doing an LDAP lookup if needed */
int nssov_dn2uid(Operation *op,nssov_info *ni,struct berval *dn,struct berval *uid);

/* transforms the uid into a DN by doing an LDAP lookup */
int nssov_uid2dn(Operation *op,nssov_info *ni,struct berval *uid,struct berval *dn);
int nssov_name2dn_cb(Operation *op, SlapReply *rs);

/* Escapes characters in a string for use in a search filter. */
int nssov_escape(struct berval *src,struct berval *dst);

int nssov_filter_byname(nssov_mapinfo *mi,int key,struct berval *name,struct berval *buf);
int nssov_filter_byid(nssov_mapinfo *mi,int key,struct berval *id,struct berval *buf);

void nssov_alias_init(nssov_info *ni);
void nssov_ether_init(nssov_info *ni);
void nssov_group_init(nssov_info *ni);
void nssov_host_init(nssov_info *ni);
void nssov_netgroup_init(nssov_info *ni);
void nssov_network_init(nssov_info *ni);
void nssov_passwd_init(nssov_info *ni);
void nssov_protocol_init(nssov_info *ni);
void nssov_rpc_init(nssov_info *ni);
void nssov_service_init(nssov_info *ni);
void nssov_shadow_init(nssov_info *ni);

int nssov_pam_init(void);

/* these are the different functions that handle the database
   specific actions, see nslcd.h for the action descriptions */
int nssov_alias_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_alias_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_ether_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_ether_byether(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_ether_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_group_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_group_bygid(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_group_bymember(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_group_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_host_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_host_byaddr(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_host_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_netgroup_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_network_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_network_byaddr(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_network_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_passwd_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_passwd_byuid(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_passwd_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_protocol_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_protocol_bynumber(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_protocol_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_rpc_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_rpc_bynumber(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_rpc_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_service_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_service_bynumber(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_service_all(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_shadow_byname(nssov_info *ni,TFILE *fp,Operation *op);
int nssov_shadow_all(nssov_info *ni,TFILE *fp,Operation *op);
int pam_authc(nssov_info *ni,TFILE *fp,Operation *op);
int pam_authz(nssov_info *ni,TFILE *fp,Operation *op);
int pam_sess_o(nssov_info *ni,TFILE *fp,Operation *op);
int pam_sess_c(nssov_info *ni,TFILE *fp,Operation *op);
int pam_pwmod(nssov_info *ni,TFILE *fp,Operation *op);

/* config initialization */
#define NSSOV_INIT(db) \
 void nssov_##db##_init(nssov_info *ni) \
 { \
	nssov_mapinfo *mi = &ni->ni_maps[NM_##db]; \
	int i; \
	for (i=0;!BER_BVISNULL(&db##_keys[i]);i++); \
	i++; \
	mi->mi_attrs = ch_malloc( i*sizeof(AttributeName)); \
	for (i=0;!BER_BVISNULL(&db##_keys[i]);i++) { \
		mi->mi_attrs[i].an_name = db##_keys[i]; \
		mi->mi_attrs[i].an_desc = NULL; \
	} \
	mi->mi_scope = LDAP_SCOPE_DEFAULT; \
	mi->mi_filter0 = db##_filter; \
	ber_dupbv( &mi->mi_filter, &mi->mi_filter0 ); \
	mi->mi_filter = db##_filter; \
	mi->mi_attrkeys = db##_keys; \
	BER_BVZERO(&mi->mi_base); \
 }

/* param structure for search callback */
#define NSSOV_CBPRIV(db,parms) \
  typedef struct nssov_##db##_cbp { \
  	nssov_mapinfo *mi; \
	TFILE *fp; \
	Operation *op; \
	parms \
  } nssov_##db##_cbp

/* callback for writing search results */
#define NSSOV_CB(db) \
  static int nssov_##db##_cb(Operation *op, SlapReply *rs) \
  { \
    if ( rs->sr_type == REP_SEARCH ) { \
    nssov_##db##_cbp *cbp = op->o_callback->sc_private; \
  	if (write_##db(cbp,rs->sr_entry)) return LDAP_OTHER; \
  } \
  return LDAP_SUCCESS; \
  } \

/* macro for generating service handling code */
#define NSSOV_HANDLE(db,fn,readfn,logcall,action,mkfilter) \
  int nssov_##db##_##fn(nssov_info *ni,TFILE *fp,Operation *op) \
  { \
    /* define common variables */ \
    int32_t tmpint32; \
    int rc; \
	nssov_##db##_cbp cbp; \
	slap_callback cb = {0}; \
	SlapReply rs = {REP_RESULT}; \
	cbp.mi = &ni->ni_maps[NM_##db]; \
	cbp.fp = fp; \
	cbp.op = op; \
    /* read request parameters */ \
    readfn; \
    /* log call */ \
    logcall; \
    /* write the response header */ \
    WRITE_INT32(fp,NSLCD_VERSION); \
    WRITE_INT32(fp,action); \
    /* prepare the search filter */ \
    if (mkfilter) \
    { \
      Debug(LDAP_DEBUG_ANY,"nssov_" __STRING(db) "_" __STRING(fn) "(): filter buffer too small",0,0,0); \
      return -1; \
    } \
	cb.sc_private = &cbp; \
	op->o_callback = &cb; \
	cb.sc_response = nssov_##db##_cb; \
	slap_op_time( &op->o_time, &op->o_tincr ); \
	op->o_req_dn = cbp.mi->mi_base; \
	op->o_req_ndn = cbp.mi->mi_base; \
	op->ors_scope = cbp.mi->mi_scope; \
	op->ors_filterstr = filter; \
	op->ors_filter = str2filter_x( op, filter.bv_val ); \
	op->ors_attrs = cbp.mi->mi_attrs; \
	op->ors_tlimit = SLAP_NO_LIMIT; \
	op->ors_slimit = SLAP_NO_LIMIT; \
    /* do the internal search */ \
	op->o_bd->be_search( op, &rs ); \
	filter_free_x( op, op->ors_filter, 1 ); \
	WRITE_INT32(fp,NSLCD_RESULT_END); \
    return 0; \
  }

#endif /* NSSOV_H */
