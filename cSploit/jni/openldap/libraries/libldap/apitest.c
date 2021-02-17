/* apitest.c -- OpenLDAP API Test Program */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
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
 * This program was orignally developed by Kurt D. Zeilenga for inclusion in
 * OpenLDAP Software.
 */
#include "portable.h"

#include <ac/stdlib.h>

#include <stdio.h>

#include <ldap.h>

int
main(int argc, char **argv)
{
	LDAPAPIInfo api;
	int ival;
	char *sval;

	printf("Compile time API Information\n");

#ifdef LDAP_API_INFO_VERSION
	api.ldapai_info_version = LDAP_API_INFO_VERSION;
	printf("  API Info version:  %d\n", (int) api.ldapai_info_version);
#else
	api.ldapai_info_version = 1;
	printf("  API Info version:  unknown\n");
#endif

#ifdef LDAP_FEATURE_INFO_VERSION
	printf("  Feature Info version:  %d\n", (int) LDAP_FEATURE_INFO_VERSION);
#else
	printf("  Feature Info version:  unknown\n");
	api.ldapai_info_version = 1;
#endif

#ifdef LDAP_API_VERSION
	printf("  API version:       %d\n", (int) LDAP_API_VERSION);
#else
	printf("  API version:       unknown\n");
#endif

#ifdef LDAP_VERSION
	printf("  Protocol Version:  %d\n", (int) LDAP_VERSION);
#else
	printf("  Protocol Version:  unknown\n");
#endif
#ifdef LDAP_VERSION_MIN
	printf("  Protocol Min:      %d\n", (int) LDAP_VERSION_MIN);
#else
	printf("  Protocol Min:      unknown\n");
#endif
#ifdef LDAP_VERSION_MAX
	printf("  Protocol Max:      %d\n", (int) LDAP_VERSION_MAX);
#else
	printf("  Protocol Max:      unknown\n");
#endif
#ifdef LDAP_VENDOR_NAME
	printf("  Vendor Name:       %s\n", LDAP_VENDOR_NAME);
#else
	printf("  Vendor Name:       unknown\n");
#endif
#ifdef LDAP_VENDOR_VERSION
	printf("  Vendor Version:    %d\n", (int) LDAP_VENDOR_VERSION);
#else
	printf("  Vendor Version:    unknown\n");
#endif

	if(ldap_get_option(NULL, LDAP_OPT_API_INFO, &api) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(API_INFO) failed\n", argv[0]);
		return EXIT_FAILURE;
	}

	printf("\nExecution time API Information\n");
	printf("  API Info version:  %d\n", api.ldapai_info_version);

	if (api.ldapai_info_version != LDAP_API_INFO_VERSION) {
		printf(" API INFO version mismatch: got %d, expected %d\n",
			api.ldapai_info_version, LDAP_API_INFO_VERSION);
		return EXIT_FAILURE;
	}

	printf("  API Version:       %d\n", api.ldapai_api_version);
	printf("  Protocol Max:      %d\n", api.ldapai_protocol_version);

	if(api.ldapai_extensions == NULL) {
		printf("  Extensions:        none\n");

	} else {
		int i;
		for(i=0; api.ldapai_extensions[i] != NULL; i++) /* empty */;
		printf("  Extensions:        %d\n", i);
		for(i=0; api.ldapai_extensions[i] != NULL; i++) {
#ifdef LDAP_OPT_API_FEATURE_INFO
			LDAPAPIFeatureInfo fi;
			fi.ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
			fi.ldapaif_name = api.ldapai_extensions[i];
			fi.ldapaif_version = 0;

			if( ldap_get_option(NULL, LDAP_OPT_API_FEATURE_INFO, &fi) == LDAP_SUCCESS ) {
				if(fi.ldapaif_info_version != LDAP_FEATURE_INFO_VERSION) {
					printf("                     %s feature info mismatch: got %d, expected %d\n",
						api.ldapai_extensions[i],
						LDAP_FEATURE_INFO_VERSION,
						fi.ldapaif_info_version);

				} else {
					printf("                     %s: version %d\n",
						fi.ldapaif_name,
						fi.ldapaif_version);
				}

			} else {
				printf("                     %s (NO FEATURE INFO)\n",
					api.ldapai_extensions[i]);
			}

#else
			printf("                     %s\n",
				api.ldapai_extensions[i]);
#endif

			ldap_memfree(api.ldapai_extensions[i]);
		}
		ldap_memfree(api.ldapai_extensions);
	}

	printf("  Vendor Name:       %s\n", api.ldapai_vendor_name);
	ldap_memfree(api.ldapai_vendor_name);

	printf("  Vendor Version:    %d\n", api.ldapai_vendor_version);

	printf("\nExecution time Default Options\n");

	if(ldap_get_option(NULL, LDAP_OPT_DEREF, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(api) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  DEREF:             %d\n", ival);

	if(ldap_get_option(NULL, LDAP_OPT_SIZELIMIT, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(sizelimit) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  SIZELIMIT:         %d\n", ival);

	if(ldap_get_option(NULL, LDAP_OPT_TIMELIMIT, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(timelimit) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  TIMELIMIT:         %d\n", ival);

	if(ldap_get_option(NULL, LDAP_OPT_REFERRALS, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(referrals) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  REFERRALS:         %s\n", ival ? "on" : "off");

	if(ldap_get_option(NULL, LDAP_OPT_RESTART, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(restart) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  RESTART:           %s\n", ival ? "on" : "off");

	if(ldap_get_option(NULL, LDAP_OPT_PROTOCOL_VERSION, &ival) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(protocol version) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("  PROTOCOL VERSION:  %d\n", ival);

	if(ldap_get_option(NULL, LDAP_OPT_HOST_NAME, &sval) != LDAP_SUCCESS) {
		fprintf(stderr, "%s: ldap_get_option(host name) failed\n", argv[0]);
		return EXIT_FAILURE;
	}
	if( sval != NULL ) {
		printf("  HOST NAME:         %s\n", sval);
		ldap_memfree(sval);
	} else {
		puts("  HOST NAME:         <not set>");
	}

#if 0
	/* API tests */
	{	/* bindless unbind */
		LDAP *ld;
		int rc;

		ld = ldap_init( "localhost", 389 );
		if( ld == NULL ) {
			perror("ldap_init");
			return EXIT_FAILURE;
		}

		rc = ldap_unbind( ld );
		if( rc != LDAP_SUCCESS ) {
			perror("ldap_unbind");
			return EXIT_FAILURE;
		}
	}
	{	/* bindless unbind */
		LDAP *ld;
		int rc;

		ld = ldap_init( "localhost", 389 );
		if( ld == NULL ) {
			perror("ldap_init");
			return EXIT_FAILURE;
		}

		rc = ldap_abandon_ext( ld, 0, NULL, NULL );
		if( rc != LDAP_SERVER_DOWN ) {
			ldap_perror( ld, "ldap_abandon");
			return EXIT_FAILURE;
		}

		rc = ldap_unbind( ld );
		if( rc != LDAP_SUCCESS ) {
			perror("ldap_unbind");
			return EXIT_FAILURE;
		}
	}
#endif

	return EXIT_SUCCESS;
}
