/* rpc.c - rpc lookup routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>. 
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 by Howard Chu, Symas Corp.
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
 * This code references portions of the nss-ldapd package
 * written by Arthur de Jong. The nss-ldapd code was forked
 * from the nss-ldap library written by Luke Howard.
 */

#include "nssov.h"

/* ( nisSchema.2.5 NAME 'oncRpc' SUP top STRUCTURAL
 *	 DESC 'Abstraction of an Open Network Computing (ONC)
 *				 [RFC1057] Remote Procedure Call (RPC) binding.
 *				 This class maps an ONC RPC number to a name.
 *				 The distinguished value of the cn attribute denotes
 *				 the RPC service's canonical name'
 *	 MUST ( cn $ oncRpcNumber )
 *	 MAY description )
 */

/* the basic search filter for searches */
static struct berval rpc_filter = BER_BVC("(objectClass=oncRpc)");

/* the attributes to request with searches */
static struct berval rpc_keys[] = {
	BER_BVC("cn"),
	BER_BVC("oncRpcNumber"),
	BER_BVNULL
};

NSSOV_INIT(rpc)

NSSOV_CBPRIV(rpc,
	char buf[256];
	struct berval name;
	struct berval numb;);

/* write a single rpc entry to the stream */
static int write_rpc(nssov_rpc_cbp *cbp,Entry *entry)
{
	int32_t tmpint32,tmp2int32,tmp3int32;
	int i,numname,dupname,number;
	struct berval name,*names;
	Attribute *a;
	char *tmp;

	/* get the most canonical name */
	nssov_find_rdnval( &entry->e_nname, cbp->mi->mi_attrs[0].an_desc, &name );
	/* get the other names for the rpc */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[0].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"rpc entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[0].an_desc->ad_cname.bv_val, 0 );
		return 0;
	}
	names = a->a_vals;
	numname = a->a_numvals;
	/* if the name is not yet found, get the first entry from names */
	if (BER_BVISNULL(&name)) {
		name=names[0];
		dupname = 0;
	} else {
		dupname = -1;
		for (i=0; i<numname; i++) {
			if ( bvmatch(&name, &a->a_nvals[i])) {
				dupname = i;
				break;
			}
		}
	}
	/* get the rpc number */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[1].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"rpc entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
		return 0;
	} else if ( a->a_numvals > 1 ) {
		Debug(LDAP_DEBUG_ANY,"rpc entry %s contains multiple %s values\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
	}
	number=(int)strtol(a->a_vals[0].bv_val,&tmp,0);
	if (*tmp)
	{
		Debug(LDAP_DEBUG_ANY,"rpc entry %s contains non-numeric %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
		return 0;
	}
	/* write the entry */
	WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
	WRITE_BERVAL(cbp->fp,&name);
	if ( dupname >= 0 ) {
		WRITE_INT32(cbp->fp,numname-1);
	} else {
		WRITE_INT32(cbp->fp,numname);
	}
	for (i=0;i<numname;i++) {
		if (i == dupname) continue;
		WRITE_BERVAL(cbp->fp,&names[i]);
	}
	WRITE_INT32(cbp->fp,number);
	return 0;
}

NSSOV_CB(rpc)

NSSOV_HANDLE(
	rpc,byname,
	char fbuf[1024];
    struct berval filter = {sizeof(fbuf)};
    filter.bv_val = fbuf;
    BER_BVZERO(&cbp.numb);
    READ_STRING(fp,cbp.buf);
    cbp.name.bv_len = tmpint32;
    cbp.name.bv_val = cbp.buf;,
	Debug(LDAP_DEBUG_TRACE,"nssov_rpc_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_RPC_BYNAME,
	nssov_filter_byname(cbp.mi,0,&cbp.name,&filter)
)

NSSOV_HANDLE(
	rpc,bynumber,
	int number;
	char fbuf[1024];
    struct berval filter = {sizeof(fbuf)};
    filter.bv_val = fbuf;
	READ_INT32(fp,number);
	cbp.numb.bv_val = cbp.buf;
	cbp.numb.bv_len = snprintf(cbp.buf,sizeof(cbp.buf),"%d",number);
	BER_BVZERO(&cbp.name);,
	Debug(LDAP_DEBUG_TRACE,"nssov_rpc_bynumber(%s)\n",cbp.numb.bv_val,0,0);,
	NSLCD_ACTION_RPC_BYNUMBER,
	nssov_filter_byid(cbp.mi,1,&cbp.numb,&filter)
)

NSSOV_HANDLE(
	rpc,all,
	struct berval filter;
	/* no parameters to read */,
	Debug(LDAP_DEBUG_TRACE,"nssov_rpc_all()\n",0,0,0);,
	NSLCD_ACTION_RPC_ALL,
	(filter=cbp.mi->mi_filter,0)
)
