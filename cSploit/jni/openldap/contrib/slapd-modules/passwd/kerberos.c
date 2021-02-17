/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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

#include <unistd.h>

#include <lber.h>
#include <lber_pvt.h>	/* BER_BVC definition */
#include "lutil.h"
#include <ac/string.h>

#ifdef HAVE_KRB5
#include <krb5.h>
#elif defined(HAVE_KRB4)
#include <krb.h>
#endif

/* From <ldap_pvt.h> */
LDAP_F( char *) ldap_pvt_get_fqdn LDAP_P(( char * ));

static LUTIL_PASSWD_CHK_FUNC chk_kerberos;
static const struct berval scheme = BER_BVC("{KERBEROS}");

static int chk_kerberos(
	const struct berval *sc,
	const struct berval * passwd,
	const struct berval * cred,
	const char **text )
{
	unsigned int i;
	int rtn;

	for( i=0; i<cred->bv_len; i++) {
		if(cred->bv_val[i] == '\0') {
			return LUTIL_PASSWD_ERR;	/* NUL character in password */
		}
	}

	if( cred->bv_val[i] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* cred must behave like a string */
	}

	for( i=0; i<passwd->bv_len; i++) {
		if(passwd->bv_val[i] == '\0') {
			return LUTIL_PASSWD_ERR;	/* NUL character in password */
		}
	}

	if( passwd->bv_val[i] != '\0' ) {
		return LUTIL_PASSWD_ERR;	/* passwd must behave like a string */
	}

	rtn = LUTIL_PASSWD_ERR;

#ifdef HAVE_KRB5 /* HAVE_HEIMDAL_KRB5 */
	{
/* Portions:
 * Copyright (c) 1997, 1998, 1999 Kungliga Tekniska H\xf6gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

		krb5_context context;
   		krb5_error_code ret;
   		krb5_creds creds;
   		krb5_get_init_creds_opt get_options;
   		krb5_verify_init_creds_opt verify_options;
		krb5_principal client, server;
#ifdef notdef
		krb5_preauthtype pre_auth_types[] = {KRB5_PADATA_ENC_TIMESTAMP};
#endif

		ret = krb5_init_context( &context );
		if (ret) {
			return LUTIL_PASSWD_ERR;
		}

#ifdef notdef
		krb5_get_init_creds_opt_set_preauth_list(&get_options,
			pre_auth_types, 1);
#endif

   		krb5_get_init_creds_opt_init( &get_options );

		krb5_verify_init_creds_opt_init( &verify_options );
	
		ret = krb5_parse_name( context, passwd->bv_val, &client );

		if (ret) {
			krb5_free_context( context );
			return LUTIL_PASSWD_ERR;
		}

		ret = krb5_get_init_creds_password( context,
			&creds, client, cred->bv_val, NULL,
			NULL, 0, NULL, &get_options );

		if (ret) {
			krb5_free_principal( context, client );
			krb5_free_context( context );
			return LUTIL_PASSWD_ERR;
		}

		{
			char *host = ldap_pvt_get_fqdn( NULL );

			if( host == NULL ) {
				krb5_free_principal( context, client );
				krb5_free_context( context );
				return LUTIL_PASSWD_ERR;
			}

			ret = krb5_sname_to_principal( context,
				host, "ldap", KRB5_NT_SRV_HST, &server );

			ber_memfree( host );
		}

		if (ret) {
			krb5_free_principal( context, client );
			krb5_free_context( context );
			return LUTIL_PASSWD_ERR;
		}

		ret = krb5_verify_init_creds( context,
			&creds, server, NULL, NULL, &verify_options );

		krb5_free_principal( context, client );
		krb5_free_principal( context, server );
		krb5_free_cred_contents( context, &creds );
		krb5_free_context( context );

		rtn = ret ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
	}
#elif	defined(HAVE_KRB4)
	{
		/* Borrowed from Heimdal kpopper */
/* Portions:
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

		int status;
		char lrealm[REALM_SZ];
		char tkt[MAXHOSTNAMELEN];

		status = krb_get_lrealm(lrealm,1);
		if (status == KFAILURE) {
			return LUTIL_PASSWD_ERR;
		}

		snprintf(tkt, sizeof(tkt), "%s_slapd.%u",
			TKT_ROOT, (unsigned)getpid());
		krb_set_tkt_string (tkt);

		status = krb_verify_user( passwd->bv_val, "", lrealm,
			cred->bv_val, 1, "ldap");

		dest_tkt(); /* no point in keeping the tickets */

		return status == KFAILURE ? LUTIL_PASSWD_ERR : LUTIL_PASSWD_OK;
	}
#endif

	return rtn;
}

int init_module(int argc, char *argv[]) {
	return lutil_passwd_add( (struct berval *)&scheme, chk_kerberos, NULL );
}
