/* netgroup.c - netgroup lookup routines */
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
#include <ac/ctype.h>

/* ( nisSchema.2.8 NAME 'nisNetgroup' SUP top STRUCTURAL
 *   DESC 'Abstraction of a netgroup. May refer to other netgroups'
 *   MUST cn
 *   MAY ( nisNetgroupTriple $ memberNisNetgroup $ description ) )
 */

/* the basic search filter for searches */
static struct berval netgroup_filter = BER_BVC("(objectClass=nisNetgroup)");

/* the attributes to request with searches */
static struct berval netgroup_keys[] = {
	BER_BVC("cn"),
	BER_BVC("nisNetgroupTriple"),
	BER_BVC("memberNisNetgroup"),
	BER_BVNULL
};

NSSOV_INIT(netgroup)

NSSOV_CBPRIV(netgroup,
	char buf[256];
	struct berval name;);

static int write_string_stripspace_len(TFILE *fp,const char *str,int len)
{
	int32_t tmpint32;
	int i,j;
	DEBUG_PRINT("WRITE_STRING: var="__STRING(str)" string=\"%s\"",str);
	if (str==NULL)
	{
		WRITE_INT32(fp,0);
	}
	else
	{
		/* skip leading spaces */
		for (i=0;(str[i]!='\0')&&(isspace(str[i]));i++)
			/* nothing else to do */ ;
		/* skip trailing spaces */
		for (j=len;(j>i)&&(isspace(str[j-1]));j--)
			/* nothing else to do */ ;
		/* write length of string */
		WRITE_INT32(fp,j-i);
		/* write string itself */
		if (j>i)
		{
			WRITE(fp,str+i,j-i);
		}
	}
	/* we're done */
	return 0;
}

#define WRITE_STRING_STRIPSPACE_LEN(fp,str,len) \
	if (write_string_stripspace_len(fp,str,len)) \
		return -1;

#define WRITE_STRING_STRIPSPACE(fp,str) \
	WRITE_STRING_STRIPSPACE_LEN(fp,str,strlen(str))

static int write_netgroup_triple(TFILE *fp,const char *triple)
{
	int32_t tmpint32;
	int i;
	int hostb,hoste,userb,usere,domainb,domaine;
	/* skip leading spaces */
	for (i=0;(triple[i]!='\0')&&(isspace(triple[i]));i++)
		/* nothing else to do */ ;
	/* we should have a bracket now */
	if (triple[i]!='(')
	{
		Debug(LDAP_DEBUG_ANY,"write_netgroup_triple(): entry does not begin with '(' (entry skipped)\n",0,0,0);
		return 0;
	}
	i++;
	hostb=i;
	/* find comma (end of host string) */
	for (;(triple[i]!='\0')&&(triple[i]!=',');i++)
		/* nothing else to do */ ;
	if (triple[i]!=',')
	{
		Debug(LDAP_DEBUG_ANY,"write_netgroup_triple(): missing ',' (entry skipped)\n",0,0,0);
		return 0;
	}
	hoste=i;
	i++;
	userb=i;
	/* find comma (end of user string) */
	for (;(triple[i]!='\0')&&(triple[i]!=',');i++)
		/* nothing else to do */ ;
	if (triple[i]!=',')
	{
		Debug(LDAP_DEBUG_ANY,"write_netgroup_triple(): missing ',' (entry skipped)\n",0,0,0);
		return 0;
	}
	usere=i;
	i++;
	domainb=i;
	/* find closing bracket (end of domain string) */
	for (;(triple[i]!='\0')&&(triple[i]!=')');i++)
		/* nothing else to do */ ;
	if (triple[i]!=')')
	{
		Debug(LDAP_DEBUG_ANY,"write_netgroup_triple(): missing ')' (entry skipped)\n",0,0,0);
		return 0;
	}
	domaine=i;
	i++;
	/* skip trailing spaces */
	for (;(triple[i]!='\0')&&(isspace(triple[i]));i++)
		/* nothing else to do */ ;
	/* if anything is left in the string we have a problem */
	if (triple[i]!='\0')
	{
		Debug(LDAP_DEBUG_ANY,"write_netgroup_triple(): string contains trailing data (entry skipped)\n",0,0,0);
		return 0;
	}
	/* write strings */
	WRITE_INT32(fp,NSLCD_RESULT_BEGIN);
	WRITE_INT32(fp,NSLCD_NETGROUP_TYPE_TRIPLE);
	WRITE_STRING_STRIPSPACE_LEN(fp,triple+hostb,hoste-hostb)
	WRITE_STRING_STRIPSPACE_LEN(fp,triple+userb,usere-userb)
	WRITE_STRING_STRIPSPACE_LEN(fp,triple+domainb,domaine-domainb)
	/* we're done */
	return 0;
}

static int write_netgroup(nssov_netgroup_cbp *cbp,Entry *entry)
{
	int32_t tmpint32;
	int i;
	Attribute *a;

	/* get the netgroup triples and member */
	a = attr_find(entry->e_attrs,cbp->mi->mi_attrs[1].an_desc);
	if ( a ) {
	/* write the netgroup triples */
		for (i=0;i<a->a_numvals;i++)
		{
			if (write_netgroup_triple(cbp->fp, a->a_vals[i].bv_val))
				return -1;
		}
	}
	a = attr_find(entry->e_attrs,cbp->mi->mi_attrs[2].an_desc);
	if ( a ) {
	/* write netgroup members */
		for (i=0;i<a->a_numvals;i++)
		{
			/* write the result code */
			WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
			/* write triple indicator */
			WRITE_INT32(cbp->fp,NSLCD_NETGROUP_TYPE_NETGROUP);
			/* write netgroup name */
			if (write_string_stripspace_len(cbp->fp,a->a_vals[i].bv_val,a->a_vals[i].bv_len))
				return -1;
		}
	}
	/* we're done */
	return 0;
}

NSSOV_CB(netgroup)

NSSOV_HANDLE(
	netgroup,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);,
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;
	Debug(LDAP_DEBUG_TRACE,"nssov_netgroup_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_NETGROUP_BYNAME,
	nssov_filter_byname(cbp.mi,0,&cbp.name,&filter)
)
