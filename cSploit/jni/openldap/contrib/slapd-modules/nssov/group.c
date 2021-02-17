/* group.c - group lookup routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>. 
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008-2009 by Howard Chu, Symas Corp.
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

/* for gid_t */
#include <grp.h>

/* ( nisSchema.2.2 NAME 'posixGroup' SUP top STRUCTURAL
 *   DESC 'Abstraction of a group of accounts'
 *   MUST ( cn $ gidNumber )
 *   MAY ( userPassword $ memberUid $ description ) )
 *
 * apart from that the above the uniqueMember attributes may be
 * supported in a coming release (they map to DNs, which is an extra
 * lookup step)
 *
 * using nested groups (groups that are member of a group) is currently
 * not supported, this may be added in a later release
 */

/* the basic search filter for searches */
static struct berval group_filter = BER_BVC("(objectClass=posixGroup)");

/* the attributes to request with searches */
static struct berval group_keys[] = {
	BER_BVC("cn"),
	BER_BVC("userPassword"),
	BER_BVC("gidNumber"),
	BER_BVC("memberUid"),
	BER_BVC("uniqueMember"),
	BER_BVNULL
};

#define	CN_KEY	0
#define	PWD_KEY	1
#define	GID_KEY	2
#define	UID_KEY	3
#define	MEM_KEY	4

/* default values for attributes */
static struct berval default_group_userPassword     = BER_BVC("*"); /* unmatchable */

NSSOV_CBPRIV(group,
	nssov_info *ni;
	char buf[256];
	struct berval name;
	struct berval gidnum;
	struct berval user;
	int wantmembers;);

/* create a search filter for searching a group entry
	 by member uid, return -1 on errors */
static int mkfilter_group_bymember(nssov_group_cbp *cbp,struct berval *buf)
{
	struct berval dn;
	/* try to translate uid to DN */
	nssov_uid2dn(cbp->op,cbp->ni,&cbp->user,&dn);
	if (BER_BVISNULL(&dn)) {
		if (cbp->user.bv_len + cbp->mi->mi_filter.bv_len + cbp->mi->mi_attrs[UID_KEY].an_desc->ad_cname.bv_len + 6 >
			buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(%s=%s))",
			cbp->mi->mi_filter.bv_val, cbp->mi->mi_attrs[UID_KEY].an_desc->ad_cname.bv_val,
			cbp->user.bv_val );
	} else { /* also lookup using user DN */
		if (cbp->user.bv_len + cbp->mi->mi_filter.bv_len + cbp->mi->mi_attrs[UID_KEY].an_desc->ad_cname.bv_len +
			dn.bv_len + cbp->mi->mi_attrs[MEM_KEY].an_desc->ad_cname.bv_len + 12 > buf->bv_len )
			return -1;
		buf->bv_len = snprintf(buf->bv_val, buf->bv_len, "(&%s(|(%s=%s)(%s=%s)))",
			cbp->mi->mi_filter.bv_val,
			cbp->mi->mi_attrs[UID_KEY].an_desc->ad_cname.bv_val, cbp->user.bv_val,
			cbp->mi->mi_attrs[MEM_KEY].an_desc->ad_cname.bv_val, dn.bv_val );
	}
	return 0;
}

NSSOV_INIT(group)

/*
	 Checks to see if the specified name is a valid group name.

	 This test is based on the definition from POSIX (IEEE Std 1003.1, 2004,
	 3.189 Group Name and 3.276 Portable Filename Character Set):
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_189
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_276

	 The standard defines group names valid if they only contain characters from
	 the set [A-Za-z0-9._-] where the hyphen should not be used as first
	 character.
*/
static int isvalidgroupname(struct berval *name)
{
	int i;

	if ( !name->bv_val || !name->bv_len )
		return 0;
	/* check first character */
	if ( ! ( (name->bv_val[0]>='A' && name->bv_val[0] <= 'Z') ||
					 (name->bv_val[0]>='a' && name->bv_val[0] <= 'z') ||
					 (name->bv_val[0]>='0' && name->bv_val[0] <= '9') ||
					 name->bv_val[0]=='.' || name->bv_val[0]=='_' ) )
		return 0;
	/* check other characters */
	for (i=1;i<name->bv_len;i++)
	{
#ifndef STRICT_GROUPS
		/* allow spaces too */
		if (name->bv_val[i] == ' ') continue;
#endif
		if ( ! ( (name->bv_val[i]>='A' && name->bv_val[i] <= 'Z') ||
						 (name->bv_val[i]>='a' && name->bv_val[i] <= 'z') ||
						 (name->bv_val[i]>='0' && name->bv_val[i] <= '9') ||
						 name->bv_val[i]=='.' || name->bv_val[i]=='_' || name->bv_val[i]=='-') )
			return 0;
	}
	/* no test failed so it must be good */
	return -1;
}

static int write_group(nssov_group_cbp *cbp,Entry *entry)
{
	struct berval tmparr[2], tmpgid[2];
	struct berval *names,*gids,*members;
	struct berval passwd = {0};
	Attribute *a;
	int i,j,nummembers,rc = 0;

	/* get group name (cn) */
	if (BER_BVISNULL(&cbp->name))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[CN_KEY].an_desc);
		if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"group entry %s does not contain %s value\n",
					entry->e_name.bv_val, cbp->mi->mi_attrs[CN_KEY].an_desc->ad_cname.bv_val,0);
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
	/* get the group id(s) */
	if (BER_BVISNULL(&cbp->gidnum))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[GID_KEY].an_desc);
		if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"group entry %s does not contain %s value\n",
					entry->e_name.bv_val, cbp->mi->mi_attrs[GID_KEY].an_desc->ad_cname.bv_val,0);
			return 0;
		}
		gids = a->a_vals;
	}
	else
	{
		gids=tmpgid;
		gids[0]=cbp->gidnum;
		BER_BVZERO(&gids[1]);
	}
	/* get group passwd (userPassword) (use only first entry) */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[PWD_KEY].an_desc);
	if (a)
		get_userpassword(&a->a_vals[0], &passwd);
	if (BER_BVISNULL(&passwd))
		passwd=default_group_userPassword;
	/* get group members (memberUid&uniqueMember) */
	if (cbp->wantmembers) {
		Attribute *b;
		i = 0; j = 0;
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[UID_KEY].an_desc);
		b = attr_find(entry->e_attrs, cbp->mi->mi_attrs[MEM_KEY].an_desc);
		if ( a )
			i += a->a_numvals;
		if ( b )
			i += b->a_numvals;
		if ( i ) {
			members = cbp->op->o_tmpalloc( (i+1) * sizeof(struct berval), cbp->op->o_tmpmemctx );
			
			if ( a ) {
				for (i=0; i<a->a_numvals; i++) {
					if (isvalidusername(&a->a_vals[i])) {
						ber_dupbv_x(&members[j],&a->a_vals[i],cbp->op->o_tmpmemctx);
						j++;
					}
				}
			}
			a = b;
			if ( a ) {
				for (i=0; i<a->a_numvals; i++) {
					if (nssov_dn2uid(cbp->op,cbp->ni,&a->a_nvals[i],&members[j]))
						j++;
				}
			}
			nummembers = j;
			BER_BVZERO(&members[j]);
		} else {
			members=NULL;
			nummembers = 0;
		}

	} else {
		members=NULL;
		nummembers = 0;
	}
	/* write entries for all names and gids */
	for (i=0;!BER_BVISNULL(&names[i]);i++)
	{
		if (!isvalidgroupname(&names[i]))
		{
			Debug(LDAP_DEBUG_ANY,"nssov: group entry %s contains invalid group name: \"%s\"\n",
													entry->e_name.bv_val,names[i].bv_val,0);
		}
		else
		{
			for (j=0;!BER_BVISNULL(&gids[j]);j++)
			{
				char *tmp;
				int tmpint32;
				gid_t gid;
				gid = strtol(gids[j].bv_val, &tmp, 0);
				if ( *tmp ) {
					Debug(LDAP_DEBUG_ANY,"nssov: group entry %s contains non-numeric %s value: \"%s\"\n",
						entry->e_name.bv_val, cbp->mi->mi_attrs[GID_KEY].an_desc->ad_cname.bv_val,
						names[i].bv_val);
					continue;
				}
				WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
				WRITE_BERVAL(cbp->fp,&names[i]);
				WRITE_BERVAL(cbp->fp,&passwd);
				WRITE_TYPE(cbp->fp,gid,gid_t);
				/* write a list of values */
				WRITE_INT32(cbp->fp,nummembers);
				if (nummembers)
				{
					int k;
					for (k=0;k<nummembers;k++) {
						WRITE_BERVAL(cbp->fp,&members[k]);
					}
				}
			}
		}
	}
	/* free and return */
	if (members!=NULL)
		ber_bvarray_free_x( members, cbp->op->o_tmpmemctx );
	return rc;
}

NSSOV_CB(group)

NSSOV_HANDLE(
	group,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;
	if (!isvalidgroupname(&cbp.name)) {
		Debug(LDAP_DEBUG_ANY,"nssov_group_byname(%s): invalid group name\n",cbp.name.bv_val,0,0);
		return -1;
	}
	cbp.wantmembers = 1;
	cbp.ni = ni;
	BER_BVZERO(&cbp.gidnum);
	BER_BVZERO(&cbp.user);,
	Debug(LDAP_DEBUG_TRACE,"nslcd_group_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_GROUP_BYNAME,
	nssov_filter_byname(cbp.mi,CN_KEY,&cbp.name,&filter)
)

NSSOV_HANDLE(
	group,bygid,
	gid_t gid;
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_TYPE(fp,gid,gid_t);
	cbp.gidnum.bv_val = cbp.buf;
	cbp.gidnum.bv_len = snprintf(cbp.buf,sizeof(cbp.buf),"%d",gid);
	cbp.wantmembers = 1;
	cbp.ni = ni;
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.user);,
	Debug(LDAP_DEBUG_TRACE,"nssov_group_bygid(%s)\n",cbp.gidnum.bv_val,0,0);,
	NSLCD_ACTION_GROUP_BYGID,
	nssov_filter_byid(cbp.mi,GID_KEY,&cbp.gidnum,&filter)
)

NSSOV_HANDLE(
	group,bymember,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);
	cbp.user.bv_len = tmpint32;
	cbp.user.bv_val = cbp.buf;
	if (!isvalidusername(&cbp.user)) {
		Debug(LDAP_DEBUG_ANY,"nssov_group_bymember(%s): invalid user name\n",cbp.user.bv_val,0,0);
		return -1;
	}
	cbp.wantmembers = 0;
	cbp.ni = ni;
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.gidnum);,
	Debug(LDAP_DEBUG_TRACE,"nssov_group_bymember(%s)\n",cbp.user.bv_val,0,0);,
	NSLCD_ACTION_GROUP_BYMEMBER,
	mkfilter_group_bymember(&cbp,&filter)
)

NSSOV_HANDLE(
	group,all,
	struct berval filter;
	/* no parameters to read */
	cbp.wantmembers = 1;
	cbp.ni = ni;
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.gidnum);,
	Debug(LDAP_DEBUG_TRACE,"nssov_group_all()\n",0,0,0);,
	NSLCD_ACTION_GROUP_ALL,
	(filter=cbp.mi->mi_filter,0)
)
