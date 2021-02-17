/* alias.c - mail alias lookup routines */
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

/* Vendor-specific attributes and object classes.
 * (Mainly from Sun.)
 * ( 1.3.6.1.4.1.42.2.27.1.2.5 NAME 'nisMailAlias' SUP top STRUCTURAL
 *   DESC 'NIS mail alias'
 *   MUST cn
 *   MAY rfc822MailMember )
 */

/* the basic search filter for searches */
static struct berval alias_filter = BER_BVC("(objectClass=nisMailAlias)");

/* the attributes to request with searches */
static struct berval alias_keys[] = {
	BER_BVC("cn"),
	BER_BVC("rfc822MailMember"),
	BER_BVNULL
};

NSSOV_INIT(alias)

NSSOV_CBPRIV(alias,
	struct berval name;
	char buf[256];);

static int write_alias(nssov_alias_cbp *cbp,Entry *entry)
{
	int32_t tmpint32,tmp2int32,tmp3int32;
	struct berval tmparr[2], empty;
	struct berval *names, *members;
	Attribute *a;
	int i;

	/* get the name of the alias */
	if (BER_BVISNULL(&cbp->name))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[0].an_desc);
		if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"alias entry %s does not contain %s value\n",
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
	/* get the members of the alias */
 	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[1].an_desc);
	if ( !a ) {
		BER_BVZERO( &empty );
		members = &empty;
	} else {
		members = a->a_vals;
	}
	/* for each name, write an entry */
	for (i=0;!BER_BVISNULL(&names[i]);i++)
	{
		WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
		WRITE_BERVAL(cbp->fp,&names[i]);
		WRITE_BVARRAY(cbp->fp,members);
	}
	return 0;
}

NSSOV_CB(alias)

NSSOV_HANDLE(
	alias,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;,
	Debug(LDAP_DEBUG_TRACE,"nssov_alias_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_ALIAS_BYNAME,
	nssov_filter_byname(cbp.mi,0,&cbp.name,&filter)
)

NSSOV_HANDLE(
	alias,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.name);,
	Debug(LDAP_DEBUG,"nssov_alias_all()\n",0,0,0);,
	NSLCD_ACTION_ALIAS_ALL,
	(filter=cbp.mi->mi_filter,0)
)
