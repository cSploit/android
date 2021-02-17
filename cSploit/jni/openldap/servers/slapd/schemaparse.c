/* schemaparse.c - routines to parse config file objectclass definitions */
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

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "ldap_schema.h"
#include "config.h"

static void		oc_usage(void); 
static void		at_usage(void);

static char *const err2text[] = {
	"Success",
	"Out of memory",
	"ObjectClass not found",
	"user-defined ObjectClass includes operational attributes",
	"user-defined ObjectClass has inappropriate SUPerior",
	"Duplicate objectClass",
	"Inconsistent duplicate objectClass",
	"AttributeType not found",
	"AttributeType inappropriate matching rule",
	"AttributeType inappropriate USAGE",
	"AttributeType inappropriate SUPerior",
	"AttributeType SYNTAX or SUPerior required",
	"Duplicate attributeType",
	"Inconsistent duplicate attributeType",
	"MatchingRule not found",
	"MatchingRule incomplete",
	"Duplicate matchingRule",
	"Syntax not found",
	"Duplicate ldapSyntax",
	"Superior syntax not found",
	"Substitute syntax not specified",
	"Substitute syntax not found",
	"OID or name required",
	"Qualifier not supported",
	"Invalid NAME",
	"OID could not be expanded",
	"Duplicate Content Rule",
	"Content Rule not for STRUCTURAL object class",
	"Content Rule AUX contains inappropriate object class",
	"Content Rule attribute type list contains duplicate",
	NULL
};

char *
scherr2str(int code)
{
	if ( code < 0 || SLAP_SCHERR_LAST <= code ) {
		return "Unknown error";
	} else {
		return err2text[code];
	}
}

/* check schema descr validity */
int slap_valid_descr( const char *descr )
{
	int i=0;

	if( !DESC_LEADCHAR( descr[i] ) ) {
		return 0;
	}

	while( descr[++i] ) {
		if( !DESC_CHAR( descr[i] ) ) {
			return 0;
		}
	}

	return 1;
}


/* OID Macros */

/* String compare with delimiter check. Return 0 if not
 * matched, otherwise return length matched.
 */
int
dscompare(const char *s1, const char *s2, char delim)
{
	const char *orig = s1;
	while (*s1++ == *s2++)
		if (!s1[-1]) break;
	--s1;
	--s2;
	if (!*s1 && (!*s2 || *s2 == delim))
		return s1 - orig;
	return 0;
}

static void
cr_usage( void )
{
	fprintf( stderr,
		"DITContentRuleDescription = \"(\" whsp\n"
		"  numericoid whsp       ; StructuralObjectClass identifier\n"
		"  [ \"NAME\" qdescrs ]\n"
		"  [ \"DESC\" qdstring ]\n"
		"  [ \"OBSOLETE\" whsp ]\n"
		"  [ \"AUX\" oids ]      ; Auxiliary ObjectClasses\n"
		"  [ \"MUST\" oids ]     ; AttributeTypes\n"
		"  [ \"MAY\" oids ]      ; AttributeTypes\n"
		"  [ \"NOT\" oids ]      ; AttributeTypes\n"
		"  whsp \")\"\n" );
}

int
parse_cr(
	struct config_args_s *c,
	ContentRule	**scr )
{
	LDAPContentRule *cr;
	int		code;
	const char	*err;
	char *line = strchr( c->line, '(' );

	cr = ldap_str2contentrule( line, &code, &err, LDAP_SCHEMA_ALLOW_ALL );
	if ( !cr ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s before %s",
			c->argv[0], ldap_scherr2str( code ), err );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		cr_usage();
		return 1;
	}

	if ( cr->cr_oid == NULL ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: OID is missing",
			c->argv[0] );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		cr_usage();
		code = 1;
		goto done;
	}

	code = cr_add( cr, 1, scr, &err );
	if ( code ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s: \"%s\"",
			c->argv[0], scherr2str(code), err);
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		code = 1;
		goto done;
	}

done:;
	if ( code ) {
		ldap_contentrule_free( cr );

	} else {
		ldap_memfree( cr );
	}

	return code;
}

int
parse_oc(
	struct config_args_s *c,
	ObjectClass	**soc,
	ObjectClass *prev )
{
	LDAPObjectClass *oc;
	int		code;
	const char	*err;
	char *line = strchr( c->line, '(' );

	oc = ldap_str2objectclass(line, &code, &err, LDAP_SCHEMA_ALLOW_ALL );
	if ( !oc ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s before %s",
			c->argv[0], ldap_scherr2str( code ), err );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		oc_usage();
		return 1;
	}

	if ( oc->oc_oid == NULL ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: OID is missing",
			c->argv[0] );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		oc_usage();
		code = 1;
		goto done;
	}

	code = oc_add( oc, 1, soc, prev, &err );
	if ( code ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s: \"%s\"",
			c->argv[0], scherr2str(code), err);
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		code = 1;
		goto done;
	}

done:;
	if ( code ) {
		ldap_objectclass_free( oc );

	} else {
		ldap_memfree( oc );
	}

	return code;
}

static void
oc_usage( void )
{
	fprintf( stderr,
		"ObjectClassDescription = \"(\" whsp\n"
		"  numericoid whsp                 ; ObjectClass identifier\n"
		"  [ \"NAME\" qdescrs ]\n"
		"  [ \"DESC\" qdstring ]\n"
		"  [ \"OBSOLETE\" whsp ]\n"
		"  [ \"SUP\" oids ]                ; Superior ObjectClasses\n"
		"  [ ( \"ABSTRACT\" / \"STRUCTURAL\" / \"AUXILIARY\" ) whsp ]\n"
		"                                  ; default structural\n"
		"  [ \"MUST\" oids ]               ; AttributeTypes\n"
		"  [ \"MAY\" oids ]                ; AttributeTypes\n"
		"  whsp \")\"\n" );
}

static void
at_usage( void )
{
	fprintf( stderr, "%s%s%s",
		"AttributeTypeDescription = \"(\" whsp\n"
		"  numericoid whsp      ; AttributeType identifier\n"
		"  [ \"NAME\" qdescrs ]             ; name used in AttributeType\n"
		"  [ \"DESC\" qdstring ]            ; description\n"
		"  [ \"OBSOLETE\" whsp ]\n"
		"  [ \"SUP\" woid ]                 ; derived from this other\n"
		"                                   ; AttributeType\n",
		"  [ \"EQUALITY\" woid ]            ; Matching Rule name\n"
		"  [ \"ORDERING\" woid ]            ; Matching Rule name\n"
		"  [ \"SUBSTR\" woid ]              ; Matching Rule name\n"
		"  [ \"SYNTAX\" whsp noidlen whsp ] ; see section 4.3\n"
		"  [ \"SINGLE-VALUE\" whsp ]        ; default multi-valued\n"
		"  [ \"COLLECTIVE\" whsp ]          ; default not collective\n",
		"  [ \"NO-USER-MODIFICATION\" whsp ]; default user modifiable\n"
		"  [ \"USAGE\" whsp AttributeUsage ]; default userApplications\n"
		"                                   ; userApplications\n"
		"                                   ; directoryOperation\n"
		"                                   ; distributedOperation\n"
		"                                   ; dSAOperation\n"
		"  whsp \")\"\n");
}

int
parse_at(
	struct config_args_s *c,
	AttributeType	**sat,
	AttributeType	*prev )
{
	LDAPAttributeType *at;
	int		code;
	const char	*err;
	char *line = strchr( c->line, '(' );

	at = ldap_str2attributetype( line, &code, &err, LDAP_SCHEMA_ALLOW_ALL );
	if ( !at ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s before %s",
			c->argv[0], ldap_scherr2str(code), err );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		at_usage();
		return 1;
	}

	if ( at->at_oid == NULL ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: OID is missing",
			c->argv[0] );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		at_usage();
		code = 1;
		goto done;
	}

	/* operational attributes should be defined internally */
	if ( at->at_usage ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: \"%s\" is operational",
			c->argv[0], at->at_oid );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		code = 1;
		goto done;
	}

	code = at_add( at, 1, sat, prev, &err);
	if ( code ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s: \"%s\"",
			c->argv[0], scherr2str(code), err);
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		code = 1;
		goto done;
	}

done:;
	if ( code ) {
		ldap_attributetype_free( at );

	} else {
		ldap_memfree( at );
	}

	return code;
}

static void
syn_usage( void )
{
	fprintf( stderr, "%s",
		"SyntaxDescription = \"(\" whsp\n"
		"  numericoid whsp                  ; object identifier\n"
		"  [ whsp \"DESC\" whsp qdstring ]  ; description\n"
		"  extensions whsp \")\"            ; extensions\n"
		"  whsp \")\"\n");
}

int
parse_syn(
	struct config_args_s *c,
	Syntax **ssyn,
	Syntax *prev )
{
	LDAPSyntax		*syn;
	slap_syntax_defs_rec	def = { 0 };
	int			code;
	const char		*err;
	char			*line = strchr( c->line, '(' );

	syn = ldap_str2syntax( line, &code, &err, LDAP_SCHEMA_ALLOW_ALL );
	if ( !syn ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s before %s",
			c->argv[0], ldap_scherr2str(code), err );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		syn_usage();
		return 1;
	}

	if ( syn->syn_oid == NULL ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: OID is missing",
			c->argv[0] );
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		syn_usage();
		code = 1;
		goto done;
	}

	code = syn_add( syn, 1, &def, ssyn, prev, &err );
	if ( code ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: %s: \"%s\"",
			c->argv[0], scherr2str(code), err);
		Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
			"%s %s\n", c->log, c->cr_msg, 0 );
		code = 1;
		goto done;
	}

done:;
	if ( code ) {
		ldap_syntax_free( syn );

	} else {
		ldap_memfree( syn );
	}

	return code;
}

