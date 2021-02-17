/* host.c - host lookup routines */
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

/* ( nisSchema.2.6 NAME 'ipHost' SUP top AUXILIARY
 *	 DESC 'Abstraction of a host, an IP device. The distinguished
 *				 value of the cn attribute denotes the host's canonical
 *				 name. Device SHOULD be used as a structural class'
 *	 MUST ( cn $ ipHostNumber )
 *	 MAY ( l $ description $ manager ) )
 */

/* the basic search filter for searches */
static struct berval host_filter = BER_BVC("(objectClass=ipHost)");

/* the attributes to request with searches */
static struct berval host_keys[] = {
	BER_BVC("cn"),
	BER_BVC("ipHostNumber"),
	BER_BVNULL
};

NSSOV_INIT(host)

NSSOV_CBPRIV(host,
	char buf[1024];
	struct berval name;
	struct berval addr;);

/* write a single host entry to the stream */
static int write_host(nssov_host_cbp *cbp,Entry *entry)
{
	int32_t tmpint32,tmp2int32,tmp3int32;
	int numaddr,i,numname,dupname;
	struct berval name,*names,*addrs;
	Attribute *a;

	/* get the most canonical name */
	nssov_find_rdnval( &entry->e_nname, cbp->mi->mi_attrs[0].an_desc, &name );
	/* get the other names for the host */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[0].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"host entry %s does not contain %s value\n",
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
	/* get the addresses */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[1].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"host entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
		return 0;
	}
	addrs = a->a_vals;
	numaddr = a->a_numvals;
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
	WRITE_INT32(cbp->fp,numaddr);
	for (i=0;i<numaddr;i++)
	{
		WRITE_ADDRESS(cbp->fp,&addrs[i]);
	}
	return 0;
}

NSSOV_CB(host)

NSSOV_HANDLE(
	host,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	BER_BVZERO(&cbp.addr);
	READ_STRING(fp,cbp.buf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;,
	Debug(LDAP_DEBUG_TRACE,"nssov_host_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_HOST_BYNAME,
	nssov_filter_byname(cbp.mi,0,&cbp.name,&filter)
)

NSSOV_HANDLE(
	host,byaddr,
	int af;
	char addr[64];
	int len=sizeof(addr);
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	BER_BVZERO(&cbp.name);
	READ_ADDRESS(fp,addr,len,af);
	/* translate the address to a string */
	if (inet_ntop(af,addr,cbp.buf,sizeof(cbp.buf))==NULL)
	{
		Debug(LDAP_DEBUG_ANY,"nssov: unable to convert address to string\n",0,0,0);
		return -1;
	}
	cbp.addr.bv_val = cbp.buf;
	cbp.addr.bv_len = strlen(cbp.buf);,
	Debug(LDAP_DEBUG_TRACE,"nssov_host_byaddr(%s)\n",cbp.addr.bv_val,0,0);,
	NSLCD_ACTION_HOST_BYADDR,
	nssov_filter_byid(cbp.mi,1,&cbp.addr,&filter)
)

NSSOV_HANDLE(
	host,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.addr);,
	Debug(LDAP_DEBUG_TRACE,"nssov_host_all()\n",0,0,0);,
	NSLCD_ACTION_HOST_ALL,
	(filter=cbp.mi->mi_filter,0)
)
