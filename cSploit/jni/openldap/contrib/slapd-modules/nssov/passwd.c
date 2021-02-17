/* passwd.c - password lookup routines */
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

/* ( nisSchema.2.0 NAME 'posixAccount' SUP top AUXILIARY
 *	 DESC 'Abstraction of an account with POSIX attributes'
 *	 MUST ( cn $ uid $ uidNumber $ gidNumber $ homeDirectory )
 *	 MAY ( userPassword $ loginShell $ gecos $ description ) )
 */

/* the basic search filter for searches */
static struct berval passwd_filter = BER_BVC("(objectClass=posixAccount)");

/* the attributes used in searches */
static struct berval passwd_keys[] = {
	BER_BVC("uid"),
	BER_BVC("userPassword"),
	BER_BVC("uidNumber"),
	BER_BVC("gidNumber"),
	BER_BVC("gecos"),
	BER_BVC("cn"),
	BER_BVC("homeDirectory"),
	BER_BVC("loginShell"),
	BER_BVC("objectClass"),
	BER_BVNULL
};

#define UID_KEY	0
#define	PWD_KEY	1
#define UIDN_KEY	2
#define GIDN_KEY	3
#define GEC_KEY	4
#define CN_KEY	5
#define DIR_KEY	6
#define SHL_KEY	7

/* default values for attributes */
static struct berval default_passwd_userPassword	= BER_BVC("*"); /* unmatchable */
static struct berval default_passwd_homeDirectory	= BER_BVC("");
static struct berval default_passwd_loginShell		= BER_BVC("");

static struct berval shadow_passwd = BER_BVC("x");

NSSOV_INIT(passwd)

/*
	 Checks to see if the specified name is a valid user name.

	 This test is based on the definition from POSIX (IEEE Std 1003.1, 2004, 3.426 User Name
	 and 3.276 Portable Filename Character Set):
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_426
	 http://www.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap03.html#tag_03_276

	 The standard defines user names valid if they contain characters from
	 the set [A-Za-z0-9._-] where the hyphen should not be used as first
	 character. As an extension this test allows the dolar '$' sign as the last
	 character to support Samba special accounts.
*/
int isvalidusername(struct berval *bv)
{
	int i;
	char *name = bv->bv_val;
	if ((name==NULL)||(name[0]=='\0'))
		return 0;
	/* check first character */
	if ( ! ( (name[0]>='A' && name[0] <= 'Z') ||
					 (name[0]>='a' && name[0] <= 'z') ||
					 (name[0]>='0' && name[0] <= '9') ||
					 name[0]=='.' || name[0]=='_' ) )
		return 0;
	/* check other characters */
	for (i=1;i<bv->bv_len;i++)
	{
		if ( name[i]=='$' )
		{
			/* if the char is $ we require it to be the last char */
			if (name[i+1]!='\0')
				return 0;
		}
		else if ( ! ( (name[i]>='A' && name[i] <= 'Z') ||
									(name[i]>='a' && name[i] <= 'z') ||
									(name[i]>='0' && name[i] <= '9') ||
									name[i]=='.' || name[i]=='_'	|| name[i]=='-') )
			return 0;
	}
	/* no test failed so it must be good */
	return -1;
}

/* return 1 on success */
int nssov_dn2uid(Operation *op,nssov_info *ni,struct berval *dn,struct berval *uid)
{
	nssov_mapinfo *mi = &ni->ni_maps[NM_passwd];
	AttributeDescription *ad = mi->mi_attrs[UID_KEY].an_desc;
	Entry *e;

	/* check for empty string */
	if (!dn->bv_len)
		return 0;
	/* try to look up uid within DN string */
	if (!strncmp(dn->bv_val,ad->ad_cname.bv_val,ad->ad_cname.bv_len) &&
		dn->bv_val[ad->ad_cname.bv_len] == '=')
	{
		struct berval bv, rdn;
		dnRdn(dn, &rdn);
		/* check if it is valid */
		bv.bv_val = dn->bv_val + ad->ad_cname.bv_len + 1;
		bv.bv_len = rdn.bv_len - ad->ad_cname.bv_len - 1;
		if (!isvalidusername(&bv))
			return 0;
		ber_dupbv_x( uid, &bv, op->o_tmpmemctx );
		return 1;
	}
	/* look up the uid from the entry itself */
	if (be_entry_get_rw( op, dn, NULL, ad, 0, &e) == LDAP_SUCCESS)
	{
		Attribute *a = attr_find(e->e_attrs, ad);
		if (a) {
			ber_dupbv_x(uid, &a->a_vals[0], op->o_tmpmemctx);
		}
		be_entry_release_r(op, e);
		if (a)
			return 1;
	}
	return 0;
}

int nssov_name2dn_cb(Operation *op,SlapReply *rs)
{
	if ( rs->sr_type == REP_SEARCH )
	{
		struct berval *bv = op->o_callback->sc_private;
		if ( !BER_BVISNULL(bv)) {
			op->o_tmpfree( bv->bv_val, op->o_tmpmemctx );
			BER_BVZERO(bv);
			return LDAP_ALREADY_EXISTS;
		}
		ber_dupbv_x(bv, &rs->sr_entry->e_name, op->o_tmpmemctx);
	}
	return LDAP_SUCCESS;
}

int nssov_uid2dn(Operation *op,nssov_info *ni,struct berval *uid,struct berval *dn)
{
	nssov_mapinfo *mi = &ni->ni_maps[NM_passwd];
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf),fbuf};
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};
	Operation op2;
	int rc;

	/* if it isn't a valid username, just bail out now */
	if (!isvalidusername(uid))
		return 0;
	/* we have to look up the entry */
	nssov_filter_byid(mi,UID_KEY,uid,&filter);
	BER_BVZERO(dn);
	cb.sc_private = dn;
	cb.sc_response = nssov_name2dn_cb;
	op2 = *op;
	op2.o_callback = &cb;
	op2.o_req_dn = mi->mi_base;
	op2.o_req_ndn = mi->mi_base;
	op2.ors_scope = mi->mi_scope;
	op2.ors_filterstr = filter;
	op2.ors_filter = str2filter_x( op, filter.bv_val );
	op2.ors_attrs = slap_anlist_no_attrs;
	op2.ors_tlimit = SLAP_NO_LIMIT;
	op2.ors_slimit = SLAP_NO_LIMIT;
	rc = op2.o_bd->be_search( &op2, &rs );
	filter_free_x( op, op2.ors_filter, 1 );
	return rc == LDAP_SUCCESS && !BER_BVISNULL(dn);
}

/* the maximum number of uidNumber attributes per entry */
#define MAXUIDS_PER_ENTRY 5

NSSOV_CBPRIV(passwd,
	char buf[256];
	struct berval name;
	struct berval id;);

static struct berval shadowclass = BER_BVC("shadowAccount");

static int write_passwd(nssov_passwd_cbp *cbp,Entry *entry)
{
	int32_t tmpint32;
	struct berval tmparr[2], tmpuid[2];
	const char **tmpvalues;
	char *tmp;
	struct berval *names;
	struct berval *uids;
	struct berval passwd = {0};
	gid_t gid;
	struct berval gecos;
	struct berval homedir;
	struct berval shell;
	Attribute *a;
	int i,j;
	int use_shadow = 0;
	/* get the usernames for this entry */
	if (BER_BVISNULL(&cbp->name))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[UID_KEY].an_desc);
		if (!a)
		{
			Debug(LDAP_DEBUG_ANY,"passwd entry %s does not contain %s value\n",
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
	/* get the password for this entry */
	a = attr_find(entry->e_attrs, slap_schema.si_ad_objectClass);
	if ( a ) {
		for ( i=0; i<a->a_numvals; i++) {
			if ( bvmatch( &shadowclass, &a->a_nvals[i] )) {
				use_shadow = 1;
				break;
			}
		}
	}
	if ( use_shadow )
	{
		/* if the entry has a shadowAccount entry, point to that instead */
		passwd = shadow_passwd;
	}
	else
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[PWD_KEY].an_desc);
		if (a)
			get_userpassword(&a->a_vals[0], &passwd);
		if (BER_BVISNULL(&passwd))
			passwd=default_passwd_userPassword;
	}
	/* get the uids for this entry */
	if (BER_BVISNULL(&cbp->id))
	{
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[UIDN_KEY].an_desc);
        if ( !a )
		{
			Debug(LDAP_DEBUG_ANY,"passwd entry %s does not contain %s value\n",
				entry->e_name.bv_val, cbp->mi->mi_attrs[UIDN_KEY].an_desc->ad_cname.bv_val,0);
			return 0;
		}
		uids = a->a_vals;
	}
	else
	{
		uids = tmpuid;
		uids[0] = cbp->id;
		BER_BVZERO(&uids[1]);
	}
	/* get the gid for this entry */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[GIDN_KEY].an_desc);
	if (!a)
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[GIDN_KEY].an_desc->ad_cname.bv_val,0);
		return 0;
	}
	else if (a->a_numvals != 1)
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s contains multiple %s values\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[GIDN_KEY].an_desc->ad_cname.bv_val,0);
	}
	gid=(gid_t)strtol(a->a_vals[0].bv_val,&tmp,0);
	if ((a->a_vals[0].bv_val[0]=='\0')||(*tmp!='\0'))
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s contains non-numeric %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[GIDN_KEY].an_desc->ad_cname.bv_val,0);
		return 0;
	}
	/* get the gecos for this entry (fall back to cn) */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[GEC_KEY].an_desc);
	if (!a)
		a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[CN_KEY].an_desc);
	if (!a || !a->a_numvals)
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s does not contain %s or %s value\n",
			entry->e_name.bv_val,
			cbp->mi->mi_attrs[GEC_KEY].an_desc->ad_cname.bv_val,
			cbp->mi->mi_attrs[CN_KEY].an_desc->ad_cname.bv_val);
		return 0;
	}
	else if (a->a_numvals > 1)
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s contains multiple %s or %s values\n",
			entry->e_name.bv_val,
			cbp->mi->mi_attrs[GEC_KEY].an_desc->ad_cname.bv_val,
			cbp->mi->mi_attrs[CN_KEY].an_desc->ad_cname.bv_val);
	}
	gecos=a->a_vals[0];
	/* get the home directory for this entry */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[DIR_KEY].an_desc);
	if (!a)
	{
		Debug(LDAP_DEBUG_ANY,"passwd entry %s does not contain %s value\n",
			entry->e_name.bv_val, cbp->mi->mi_attrs[DIR_KEY].an_desc->ad_cname.bv_val,0);
		homedir=default_passwd_homeDirectory;
	}
	else
	{
		if (a->a_numvals > 1)
		{
			Debug(LDAP_DEBUG_ANY,"passwd entry %s contains multiple %s values\n",
				entry->e_name.bv_val, cbp->mi->mi_attrs[DIR_KEY].an_desc->ad_cname.bv_val,0);
		}
		homedir=a->a_vals[0];
		if (homedir.bv_val[0]=='\0')
			homedir=default_passwd_homeDirectory;
	}
	/* get the shell for this entry */
	a = attr_find(entry->e_attrs, cbp->mi->mi_attrs[SHL_KEY].an_desc);
	if (!a)
	{
		shell=default_passwd_loginShell;
	}
	else
	{
		if (a->a_numvals > 1)
		{
			Debug(LDAP_DEBUG_ANY,"passwd entry %s contains multiple %s values\n",
				entry->e_name.bv_val, cbp->mi->mi_attrs[SHL_KEY].an_desc->ad_cname.bv_val,0);
		}
		shell=a->a_vals[0];
		if (shell.bv_val[0]=='\0')
			shell=default_passwd_loginShell;
	}
	/* write the entries */
	for (i=0;!BER_BVISNULL(&names[i]);i++)
	{
		if (!isvalidusername(&names[i]))
		{
			Debug(LDAP_DEBUG_ANY,"nssov: passwd entry %s contains invalid user name: \"%s\"\n",
				entry->e_name.bv_val,names[i].bv_val,0);
		}
		else
		{
			for (j=0;!BER_BVISNULL(&uids[j]);j++)
			{
				char *tmp;
				uid_t uid;
				uid = strtol(uids[j].bv_val, &tmp, 0);
				if ( *tmp ) {
					Debug(LDAP_DEBUG_ANY,"nssov: passwd entry %s contains non-numeric %s value: \"%s\"\n",
						entry->e_name.bv_val, cbp->mi->mi_attrs[UIDN_KEY].an_desc->ad_cname.bv_val,
						names[i].bv_val);
					continue;
				}
				WRITE_INT32(cbp->fp,NSLCD_RESULT_BEGIN);
				WRITE_BERVAL(cbp->fp,&names[i]);
				WRITE_BERVAL(cbp->fp,&passwd);
				WRITE_TYPE(cbp->fp,uid,uid_t);
				WRITE_TYPE(cbp->fp,gid,gid_t);
				WRITE_BERVAL(cbp->fp,&gecos);
				WRITE_BERVAL(cbp->fp,&homedir);
				WRITE_BERVAL(cbp->fp,&shell);
			}
		}
	}
	return 0;
}

NSSOV_CB(passwd)

NSSOV_HANDLE(
	passwd,byname,
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_STRING(fp,cbp.buf);
	cbp.name.bv_len = tmpint32;
	cbp.name.bv_val = cbp.buf;
	if (!isvalidusername(&cbp.name)) {
		Debug(LDAP_DEBUG_ANY,"nssov_passwd_byname(%s): invalid user name\n",cbp.name.bv_val,0,0);
		return -1;
	}
	BER_BVZERO(&cbp.id); ,
	Debug(LDAP_DEBUG_TRACE,"nssov_passwd_byname(%s)\n",cbp.name.bv_val,0,0);,
	NSLCD_ACTION_PASSWD_BYNAME,
	nssov_filter_byname(cbp.mi,UID_KEY,&cbp.name,&filter)
)

NSSOV_HANDLE(
	passwd,byuid,
	uid_t uid;
	char fbuf[1024];
	struct berval filter = {sizeof(fbuf)};
	filter.bv_val = fbuf;
	READ_TYPE(fp,uid,uid_t);
	cbp.id.bv_val = cbp.buf;
	cbp.id.bv_len = snprintf(cbp.buf,sizeof(cbp.buf),"%d",uid);
	BER_BVZERO(&cbp.name);,
	Debug(LDAP_DEBUG_TRACE,"nssov_passwd_byuid(%s)\n",cbp.id.bv_val,0,0);,
	NSLCD_ACTION_PASSWD_BYUID,
	nssov_filter_byid(cbp.mi,UIDN_KEY,&cbp.id,&filter)
)

NSSOV_HANDLE(
	passwd,all,
	struct berval filter;
	/* no parameters to read */
	BER_BVZERO(&cbp.name);
	BER_BVZERO(&cbp.id);,
	Debug(LDAP_DEBUG_TRACE,"nssov_passwd_all()\n",0,0,0);,
	NSLCD_ACTION_PASSWD_ALL,
	(filter=cbp.mi->mi_filter,0)
)
