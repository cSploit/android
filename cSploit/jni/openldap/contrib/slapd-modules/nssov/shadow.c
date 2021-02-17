/* shadow.c - shadow account lookup routines */
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

/* ( nisSchema.2.1 NAME 'shadowAccount' SUP top AUXILIARY
 *	 DESC 'Additional attributes for shadow passwords'
 *	 MUST uid
 *	 MAY ( userPassword $ shadowLastChange $ shadowMin
 *				 shadowMax $ shadowWarning $ shadowInactive $
 *				 shadowExpire $ shadowFlag $ description ) )
 */

/* the basic search filter for searches */
static struct berval shadow_filter = BER_BVC("(objectClass=shadowAccount)");

/* the attributes to request with searches */
static struct berval shadow_keys[] = {
	BER_BVC("uid"),
	BER_BVC("userPassword"),
	BER_BVC("shadowLastChange"),
	BER_BVC("shadowMin"),
	BER_BVC("shadowMax"),
	BER_BVC("shadowWarning"),
	BER_BVC("shadowInactive"),
	BER_BVC("shadowExpire"),
	BER_BVC("shadowFlag"),
	BER_BVNULL
};

#define UID_KEY	0
#define PWD_KEY	1
#define CHG_KEY	2
#define MIN_KEY	3
#define MAX_KEY 4
#define WRN_KEY 5
#define INA_KEY 6
#define EXP_KEY 7
#define FLG_KEY 8

/* default values for attributes */
static struct berval default_shadow_userPassword		 = BER_BVC("*"); /* unmatchable */
static int default_nums[] = { 0,0,
	-1, /* LastChange */
	-1, /* Min */
	-1, /* Max */
	-1, /* Warning */
	-1, /* Inactive */
	-1, /* Expire */
	0 /* Flag */
};

NSSOV_INIT(shadow)

static long to_date(struct berval *date,AttributeDescription *attr)
{
	long value;
	char *tmp;
	/* do some special handling for date values on AD */
	if (strcasecmp(attr->ad_cname.bv_val,"pwdLastSet")==0)
	{
		char buffer[8];
		size_t l;
		/* we expect an AD 64-bit datetime value;
			 we should do date=date/864000000000-134774
			 but that causes problems on 32-bit platforms,
			 first we devide by 1000000000 by stripping the
			 last 9 digits from the string and going from there */
		l=date->bv_len-9;
		if (l<1 || l>(sizeof(buffer)-1))
			return 0; /* error */
		strncpy(buffer,date->bv_val,l);
		buffer[l]='\0';
		value=strtol(buffer,&tmp,0);
		if ((buffer[0]=='\0')||(*tmp!='\0'))
		{
			Debug(LDAP_DEBUG_ANY,"shadow entry contains non-numeric %s value\n",
				attr->ad_cname.bv_val,0,0);
			return 0;
		}
		return value/864-134774;
		/* note that AD does not have expiry dates but a lastchangeddate
			 and some value that needs to be added */
	}
	value=strtol(date->bv_val,&tmp,0);
	if ((date->bv_val[0]=='\0')||(*tmp!='\0'))
	{
		Debug(LDAP_DEBUG_ANY,"shadow entry contains non-numeric %s value\n",
			attr->ad_cname.bv_val,0,0);
		return 0;
	}
	return value;
}

#ifndef UF_DONT_EXPIRE_PASSWD
#define UF_DONT_EXPIRE_PASSWD 0x10000
#endif

#define GET_OPTIONAL_LONG(var,key) \
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[key].an_desc); \
	if ( !a || BER_BVISNULL(&a->a_vals[0])) \
		var = default_nums[key]; \
	else \
	{ \
		if (a->a_numvals > 1) \
		{ \
			Debug(LDAP_DEBUG_ANY,"shadow entry %s contains multiple %s values\n", \
				entry->e_name.bv_val, cbp->mi->mi_attrs[key].an_desc->ad_cname.bv_val,0); \
		} \
		var=strtol(a->a_vals[0].bv_val,&tmp,0); \
		if ((a->a_vals[0].bv_val[0]=='\0')||(*tmp!='\0')) \
		{ \
			Debug(LDAP_DEBUG_ANY,"shadow entry %s contains non-numeric %s value\n", \
				entry->e_name.bv_val, cbp->mi->mi_attrs[key].an_desc->ad_cname.bv_val,0); \
			return 0; \
		} \
	}

#define GET_OPTIONAL_DATE(var,key) \
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[key].an_desc); \
	if ( !a || BER_BVISNULL(&a->a_vals[0])) \
		var = default_nums[key]; \
	else \
	{ \
		if (a->a_numvals > 1) \
		{ \
			Debug(LDAP_DEBUG_ANY,"shadow entry %s contains multiple %s values\n", \
				entry->e_name.bv_val, cbp->mi->mi_attrs[key].an_desc->ad_cname.bv_val,0); \
		} \
		var=to_date(&a->a_vals[0],cbp->mi->mi_attrs[key].an_desc); \
	}

NSSOV_CBPRIV(shadow,
	char buf[256];
	struct berval name;);

static int write_shadow(nssov_shadow_cbp *cbp,Entry *entry)
{
	struct berval tmparr[2];
	struct berval *names;
	Attribute *a;
	char *tmp;
	struct berval passwd = {0};
	long lastchangedate;
	long mindays;
	long maxdays;
	long warndays;
	long inactdays;
	long expiredate;
	unsigned long flag;
	int i;
	int32_t tmpint32;
	/* get username */
	if (BER_BVISNULL(&cbp->name))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[UID_KEY].an_desc);
		if (!a)
		{
			Debug(LDAP_DEBUG_ANY,"shadow entry %s does not contain %s value\n",
				entry->e_name.bv_val, cbp->mi->mi_attrs[UID_KEY].an_desc->ad_cname.bv_val,0);
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
	/* get password */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[PWD_KEY].an_desc);
	if ( a )
		get_userpassword(&a->a_vals[0], &passwd);
	if (BER_BVISNULL(&passwd))
		passwd=default_shadow_userPassword;
	/* get lastchange date */
	GET_OPTIONAL_DATE(lastchangedate,CHG_KEY);
	/* get mindays */
	GET_OPTIONAL_LONG(mindays,MIN_KEY);
	/* get maxdays */
	GET_OPTIONAL_LONG(maxdays,MAX_KEY);
	/* get warndays */
	GET_OPTIONAL_LONG(warndays,WRN_KEY);
	/* get inactdays */
	GET_OPTIONAL_LONG(inactdays,INA_KEY);
	/* get expire date */
	GET_OPTIONAL_LONG(expiredate,EXP_KEY);
	/* get flag */
	GET_OPTIONAL_LONG(flag,FLG_KEY);
	/* if we're using AD handle the flag specially */
	if (strcasecmp(cbp->mi->mi_attrs[CHG_KEY].an_desc->ad_cname.bv_val,"pwdLastSet")==0)
	{
		if (flag&UF_DONT_EXPIRE_PASSWD)
			maxdays=99999;
		flag=0;
	}
	/* write the entries */
	for (i=0;!BER_BVISNULL(&names[i]);i++)
	{
		WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
		WRITE_BERVAL(cbp->fp,&names[i]);
		WRITE_BERVAL(cbp->fp,&passwd);
		WRITE_INT32(cbp->fp,lastchangedate);
		WRITE_INT32(cbp->fp,mindays);
		WRITE_INT32(cbp->fp,maxdays);
		WRITE_INT32(cbp->fp,warndays);
		WRITE_INT32(cbp->fp,inactdays);
		WRITE_INT32(cbp->fp,expiredate);
		WRITE_INT32(cbp->fp,flag);
	}
	return 0;
}

NSSOV_CB(shadow)

NSSOV_HANDLE(
	shadow,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);,
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;
	Debug(LDAP_DEBUG_ANY,"nssov_shadow_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_SHADOW_BYNAME,
	nssov_filter_byname(cbp.mi,UID_KEY,&cbp.name,&filter)
)

NSSOV_HANDLE(
	shadow,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.name);,
	Debug(LDAP_DEBUG_ANY,"nssov_shadow_all()\n",0,0,0);,
	NSLCD_ACTION_SHADOW_ALL,
	(filter=cbp.mi->mi_filter,0)
)
