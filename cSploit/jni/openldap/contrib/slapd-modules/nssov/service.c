/* service.c - service lookup routines */
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

/* ( nisSchema.2.3 NAME 'ipService' SUP top STRUCTURAL
 *	 DESC 'Abstraction an Internet Protocol service.
 *				 Maps an IP port and protocol (such as tcp or udp)
 *				 to one or more names; the distinguished value of
 *				 the cn attribute denotes the service's canonical
 *				 name'
 *	 MUST ( cn $ ipServicePort $ ipServiceProtocol )
 *	 MAY ( description ) )
 */

/* the basic search filter for searches */
static struct berval service_filter = BER_BVC("(objectClass=ipService)");

/* the attributes to request with searches */
static struct berval service_keys[] = {
	BER_BVC("cn"),
	BER_BVC("ipServicePort"),
	BER_BVC("ipServiceProtocol"),
	BER_BVNULL
};

static int mkfilter_service_byname(nssov_mapinfo *mi,struct berval *name,
								 struct berval *protocol,struct berval *buf)
{
	char buf2[1024],buf3[1024];
	struct berval bv2 = {sizeof(buf2),buf2};
	struct berval bv3 = {sizeof(buf3),buf3};

	/* escape attributes */
	if (nssov_escape(name,&bv2))
		return -1;
	if (!BER_BVISNULL(protocol)) {
		if (nssov_escape(protocol,&bv3))
			return -1;
		if (bv2.bv_len + mi->mi_filter.bv_len + mi->mi_attrs[0].an_desc->ad_cname.bv_len +
			bv3.bv_len + mi->mi_attrs[2].an_desc->ad_cname.bv_len + 9 > buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s)(%s=%s))",
			mi->mi_filter.bv_val,
			mi->mi_attrs[0].an_desc->ad_cname.bv_val, bv2.bv_val,
			mi->mi_attrs[2].an_desc->ad_cname.bv_val, bv3.bv_val );
	} else {
		if (bv2.bv_len + mi->mi_filter.bv_len + mi->mi_attrs[0].an_desc->ad_cname.bv_len + 6 >
			buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s))",
			mi->mi_filter.bv_val, mi->mi_attrs[0].an_desc->ad_cname.bv_val,
			bv2.bv_val );
	}
	return 0;
}

static int mkfilter_service_bynumber(nssov_mapinfo *mi,struct berval *numb,
								 struct berval *protocol,struct berval *buf)
{
	char buf2[1024];
	struct berval bv2 = {sizeof(buf2),buf2};

	/* escape attribute */
	if (!BER_BVISNULL(protocol)) {
		if (nssov_escape(protocol,&bv2))
			return -1;
		if (numb->bv_len + mi->mi_filter.bv_len + mi->mi_attrs[1].an_desc->ad_cname.bv_len +
			bv2.bv_len + mi->mi_attrs[2].an_desc->ad_cname.bv_len + 9 > buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s)(%s=%s))",
			mi->mi_filter.bv_val,
			mi->mi_attrs[1].an_desc->ad_cname.bv_val, numb->bv_val,
			mi->mi_attrs[2].an_desc->ad_cname.bv_val, bv2.bv_val );
	} else {
		if (numb->bv_len + mi->mi_filter.bv_len + mi->mi_attrs[1].an_desc->ad_cname.bv_len + 6 >
			buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s))",
			mi->mi_filter.bv_val, mi->mi_attrs[1].an_desc->ad_cname.bv_val,
			numb->bv_val );
	}
	return 0;
}

NSSOV_INIT(service)

NSSOV_CBPRIV(service,
	char nbuf[256];
	char pbuf[256];
	struct berval name;
	struct berval prot;);

static int write_service(nssov_service_cbp *cbp,Entry *entry)
{
	int32_t tmpint32,tmp2int32,tmp3int32;
	struct berval name,*names,*ports,*protos;
	struct berval tmparr[2];
	Attribute *a;
	char *tmp;
	int port;
	int i,numname,dupname,numprot;

	/* get the most canonical name */
	nssov_find_rdnval( &entry->e_nname, cbp->mi->mi_attrs[0].an_desc, &name );
	/* get the other names for the rpc */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[0].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"service entry %s does not contain %s value\n",
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
	/* get the service number */
	a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[1].an_desc );
	if ( !a || !a->a_vals )
	{
		Debug(LDAP_DEBUG_ANY,"service entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
		return 0;
	} else if ( a->a_numvals > 1 ) {
		Debug(LDAP_DEBUG_ANY,"service entry %s contains multiple %s values\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
	}
	port=(int)strtol(a->a_vals[0].bv_val,&tmp,0);
	if (*tmp)
	{
		Debug(LDAP_DEBUG_ANY,"service entry %s contains non-numeric %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val, 0 );
		return 0;
	}
	/* get protocols */
	if (BER_BVISNULL(&cbp->prot))
	{
		a = attr_find( entry->e_attrs, cbp->mi->mi_attrs[2].an_desc );
		if ( !a || !a->a_vals )
		{
			Debug(LDAP_DEBUG_ANY,"service entry %s does not contain %s value\n",
				entry->e_name.bv_val, cbp->mi->mi_attrs[2].an_desc->ad_cname.bv_val, 0 );
			return 0;
		}
		protos = a->a_vals;
		numprot = a->a_numvals;
	}
	else
	{
		protos=tmparr;
		protos[0]=cbp->prot;
		BER_BVZERO(&protos[1]);
		numprot = 1;
	}
	/* write the entries */
	for (i=0;i<numprot;i++)
	{
		int j;
		WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
		WRITE_BERVAL(cbp->fp,&name);
		if ( dupname >= 0 ) {
			WRITE_INT32(cbp->fp,numname-1);
		} else {
			WRITE_INT32(cbp->fp,numname);
		}
		for (j=0;j<numname;j++) {
			if (j == dupname) continue;
			WRITE_BERVAL(cbp->fp,&names[j]);
		}
		WRITE_INT32(cbp->fp,port);
		WRITE_BERVAL(cbp->fp,&protos[i]);
	}
	return 0;
}

NSSOV_CB(service)

NSSOV_HANDLE(
	service,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.nbuf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.nbuf;
	READ_STRING(fp,cbp.pbuf);
	cbp.prot.bv_len = tmpint32;
	cbp.prot.bv_val = tmpint32 ? cbp.pbuf : NULL;,
	Debug(LDAP_DEBUG_TRACE,"nssov_service_byname(%s,%s)\n",cbp.name.bv_val,cbp.prot.bv_val ? cbp.prot.bv_val : "",0);,
	NSLCD_ACTION_SERVICE_BYNAME,
	mkfilter_service_byname(cbp.mi,&cbp.name,&cbp.prot,&filter)
)

NSSOV_HANDLE(
	service,bynumber,
	int number;
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_INT32(fp,number);
	cbp.name.bv_val = cbp.nbuf;
	cbp.name.bv_len = snprintf(cbp.nbuf,sizeof(cbp.nbuf),"%d",number);
	READ_STRING(fp,cbp.pbuf);
	cbp.prot.bv_len = tmpint32;
	cbp.prot.bv_val = tmpint32 ? cbp.pbuf : NULL;,
	Debug(LDAP_DEBUG_TRACE,"nssov_service_bynumber(%s,%s)\n",cbp.name.bv_val,cbp.prot.bv_val,0);,
	NSLCD_ACTION_SERVICE_BYNUMBER,
	mkfilter_service_bynumber(cbp.mi,&cbp.name,&cbp.prot,&filter)
)

NSSOV_HANDLE(
	service,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.prot);,
	Debug(LDAP_DEBUG_TRACE,"nssov_service_all()\n",0,0,0);,
	NSLCD_ACTION_SERVICE_ALL,
	(filter=cbp.mi->mi_filter,0)
)
