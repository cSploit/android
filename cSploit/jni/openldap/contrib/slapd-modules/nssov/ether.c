/* ether.c - ethernet address lookup routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Copyright 2008 by Howard Chu, Symas Corp.
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

struct ether_addr {
  uint8_t ether_addr_octet[6];
};

/* ( nisSchema.2.11 NAME 'ieee802Device' SUP top AUXILIARY
 *   DESC 'A device with a MAC address; device SHOULD be
 *         used as a structural class'
 *   MAY macAddress )
 */

/* the basic search filter for searches */
static struct berval ether_filter = BER_BVC("(objectClass=ieee802Device)");

/* the attributes to request with searches */
static struct berval ether_keys[] = {
	BER_BVC("cn"),
	BER_BVC("macAddress"),
	BER_BVNULL
};

NSSOV_INIT(ether)

NSSOV_CBPRIV(ether,
	char buf[256];
	struct berval name;
	struct berval addr;);

#define WRITE_ETHER(fp,addr) \
  {int ao[6]; \
  sscanf(addr.bv_val,"%02x:%02x:%02x:%02x:%02x:%02x", \
  	&ao[0], &ao[1], &ao[2], &ao[3], &ao[4], &ao[5] );\
	tmpaddr.ether_addr_octet[0] = ao[0]; \
	tmpaddr.ether_addr_octet[1] = ao[1]; \
	tmpaddr.ether_addr_octet[2] = ao[2]; \
	tmpaddr.ether_addr_octet[3] = ao[3]; \
	tmpaddr.ether_addr_octet[4] = ao[4]; \
	tmpaddr.ether_addr_octet[5] = ao[5]; } \
  WRITE_TYPE(fp,tmpaddr,uint8_t[6]);

static int write_ether(nssov_ether_cbp *cbp,Entry *entry)
{
	int32_t tmpint32;
	struct ether_addr tmpaddr;
	struct berval tmparr[2], empty;
	struct berval *names,*ethers;
	Attribute *a;
	int i,j;

	/* get the name of the ether entry */
	if (BER_BVISNULL(&cbp->name))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[0].an_desc);
		if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"ether entry %s does not contain %s value\n",
							entry->e_name.bv_val,cbp->mi->mi_attrs[0].an_desc->ad_cname.bv_val,0 );
			return 0;
		}
		names = a->a_vals;
	}
	else
	{
		names=tmparr;
		names[0]=cbp->name;
		BER_BVZERO(&names[1]);
	}
	/* get the addresses */
	if (BER_BVISNULL(&cbp->addr))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[1].an_desc);
		if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"ether entry %s does not contain %s value\n",
							entry->e_name.bv_val,cbp->mi->mi_attrs[1].an_desc->ad_cname.bv_val,0 );
			return 0;
		}
		ethers = a->a_vals;
		/* TODO: move parsing of addresses up here */
	}
	else
	{
		ethers=tmparr;
		ethers[0]=cbp->addr;
		BER_BVZERO(&ethers[1]);
	}
	/* write entries for all names and addresses */
	for (i=0;!BER_BVISNULL(&names[i]);i++)
		for (j=0;!BER_BVISNULL(&ethers[j]);j++)
		{
			WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
			WRITE_BERVAL(cbp->fp,&names[i]);
			WRITE_ETHER(cbp->fp,ethers[j]);
		}
	return 0;
}

NSSOV_CB(ether)

NSSOV_HANDLE(
	ether,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	BER_BVZERO(&cbp.addr);
	READ_STRING(fp,cbp.buf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;,
	Debug(LDAP_DEBUG_TRACE,"nssov_ether_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_ETHER_BYNAME,
	nssov_filter_byname(cbp.mi,0,&cbp.name,&filter)
)

NSSOV_HANDLE(
	ether,byether,
	struct ether_addr addr;
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	BER_BVZERO(&cbp.name);
	READ_TYPE(fp,addr,uint8_t[6]);
	cbp.addr.bv_len = snprintf(cbp.buf,sizeof(cbp.buf), "%x:%x:%x:%x:%x:%x",
		addr.ether_addr_octet[0],
		addr.ether_addr_octet[1],
		addr.ether_addr_octet[2],
		addr.ether_addr_octet[3],
		addr.ether_addr_octet[4],
		addr.ether_addr_octet[5]);
	cbp.addr.bv_val = cbp.buf;,
	Debug(LDAP_DEBUG_TRACE,"nssov_ether_byether(%s)\n",cbp.addr.bv_val,0,0);,
	NSLCD_ACTION_ETHER_BYETHER,
	nssov_filter_byid(cbp.mi,1,&cbp.addr,&filter)
)

NSSOV_HANDLE(
	ether,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.addr);,
	Debug(LDAP_DEBUG_TRACE,"nssov_ether_all()\n",0,0,0);,
	NSLCD_ACTION_ETHER_ALL,
	(filter=cbp.mi->mi_filter,0)
)
