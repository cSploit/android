/* $OpenLDAP$
 */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#ifndef REWRITE_H
#define REWRITE_H

/*
 * Default rewrite context
 */
#define REWRITE_DEFAULT_CONTEXT		"default"

/*
 * Rewrite engine states
 */
#define REWRITE_OFF			0x0000
#define REWRITE_ON			0x0001
#define REWRITE_DEFAULT			REWRITE_OFF

/*
 * Rewrite internal status returns
 */
#define REWRITE_SUCCESS			LDAP_SUCCESS
#define REWRITE_ERR			LDAP_OTHER

/*
 * Rewrite modes (input values for rewrite_info_init); determine the
 * behavior in case a null or non existent context is required:
 *
 * 	REWRITE_MODE_ERR		error
 * 	REWRITE_MODE_OK			no error but no rewrite
 * 	REWRITE_MODE_COPY_INPUT		a copy of the input is returned
 * 	REWRITE_MODE_USE_DEFAULT	the default context is used.
 */
#define REWRITE_MODE_ERR		0x0010
#define REWRITE_MODE_OK			0x0011
#define REWRITE_MODE_COPY_INPUT		0x0012
#define REWRITE_MODE_USE_DEFAULT	0x0013

/*
 * Rewrite status returns
 *
 * 	REWRITE_REGEXEC_OK		success (result may be empty in case
 * 					of no match)
 * 	REWRITE_REGEXEC_ERR		error (internal error,
 * 					misconfiguration, map not working ...)
 * 	REWRITE_REGEXEC_STOP		internal use; never returned
 * 	REWRITE_REGEXEC_UNWILLING	the server should issue an 'unwilling
 * 					to perform' error
 */
#define REWRITE_REGEXEC_OK              (0)
#define REWRITE_REGEXEC_ERR             (-1)
#define REWRITE_REGEXEC_STOP            (-2)
#define REWRITE_REGEXEC_UNWILLING       (-3)
#define REWRITE_REGEXEC_USER		(1)	/* and above: LDAP errors */

/*
 * Rewrite variable flags
 *	REWRITE_VAR_INSERT		insert mode (default) when adding
 *					a variable; if not set during value
 *					update, the variable is not inserted
 *					if not present
 *	REWRITE_VAR_UPDATE		update mode (default) when updating
 *					a variable; if not set during insert,
 *					the value is not updated if the
 *					variable already exists
 *	REWRITE_VAR_COPY_NAME		copy the variable name; if not set,
 *					the name is not copied; be sure the
 *					referenced string is available for
 *					the entire life scope of the variable.
 *	REWRITE_VAR_COPY_VALUE		copy the variable value; if not set,
 *					the value is not copied; be sure the
 *					referenced string is available for
 *					the entire life scope of the variable.
 */
#define REWRITE_VAR_NONE		0x0000
#define REWRITE_VAR_INSERT		0x0001
#define REWRITE_VAR_UPDATE		0x0002
#define REWRITE_VAR_COPY_NAME		0x0004
#define REWRITE_VAR_COPY_VALUE		0x0008

/*
 * Rewrite info
 */
struct rewrite_info;

struct berval; /* avoid include */

LDAP_BEGIN_DECL

/*
 * Inits the info
 */
LDAP_REWRITE_F (struct rewrite_info *)
rewrite_info_init(
		int mode
);

/*
 * Cleans up the info structure
 */
LDAP_REWRITE_F (int)
rewrite_info_delete(
                struct rewrite_info **info
);


/*
 * Parses a config line and takes actions to fit content in rewrite structure;
 * lines handled are of the form:
 *
 *      rewriteEngine 		{on|off}
 *      rewriteMaxPasses	numPasses
 *      rewriteContext 		contextName [alias aliasedRewriteContex]
 *      rewriteRule 		pattern substPattern [ruleFlags]
 *      rewriteMap 		mapType mapName [mapArgs]
 *      rewriteParam		paramName paramValue
 */
LDAP_REWRITE_F (int)
rewrite_parse(
		struct rewrite_info *info,
                const char *fname,
                int lineno,
                int argc,
                char **argv
);

/*
 * process a config file that was already opened. Uses rewrite_parse.
 */
LDAP_REWRITE_F (int)
rewrite_read(
		FILE *fin,
		struct rewrite_info *info
);

/*
 * Rewrites a string according to context.
 * If the engine is off, OK is returned, but the return string will be NULL.
 * In case of 'unwilling to perform', UNWILLING is returned, and the
 * return string will also be null. The same in case of error.
 * Otherwise, OK is returned, and result will hold a newly allocated string
 * with the rewriting.
 *
 * What to do in case of non-existing rewrite context is still an issue.
 * Four possibilities:
 *      - error,
 *      - ok with NULL result,
 *      - ok with copy of string as result,
 *      - use the default rewrite context.
 */
LDAP_REWRITE_F (int)
rewrite(
		struct rewrite_info *info,
		const char *rewriteContext,
		const char *string,
		char **result
);

/*
 * Same as above; the cookie relates the rewrite to a session
 */
LDAP_REWRITE_F (int)
rewrite_session(
		struct rewrite_info *info,
		const char *rewriteContext,
		const char *string,
		const void *cookie,
		char **result
);

/*
 * Inits a session
 */
LDAP_REWRITE_F (struct rewrite_session *)
rewrite_session_init(
                struct rewrite_info *info,
                const void *cookie
);

/*
 * Defines and inits a variable with session scope
 */
LDAP_REWRITE_F (int)
rewrite_session_var_set_f(
		struct rewrite_info *info,
		const void *cookie,
		const char *name,
		const char *value,
		int flags
);

#define rewrite_session_var_set(info, cookie, name, value) \
	rewrite_session_var_set_f((info), (cookie), (name), (value), \
			REWRITE_VAR_INSERT|REWRITE_VAR_UPDATE|REWRITE_VAR_COPY_NAME|REWRITE_VAR_COPY_VALUE)

/*
 * Deletes a session
 */
LDAP_REWRITE_F (int)
rewrite_session_delete(
		struct rewrite_info *info,
		const void *cookie
);


/*
 * Params
 */

/*
 * Defines and inits a variable with global scope
 */
LDAP_REWRITE_F (int)
rewrite_param_set(
                struct rewrite_info *info,
                const char *name,
                const char *value
);

/*
 * Gets a var with global scope
 */
LDAP_REWRITE_F (int)
rewrite_param_get(
                struct rewrite_info *info,
                const char *name,
                struct berval *value
);

/*
 * Destroys the parameter tree
 */
LDAP_REWRITE_F (int)
rewrite_param_destroy(
                struct rewrite_info *info
);

/*
 * Mapping implementations
 */

struct rewrite_mapper;

typedef void * (rewrite_mapper_config)(
	const char *fname,
	int lineno,
	int argc,
	char **argv );

typedef int (rewrite_mapper_apply)(
	void *ctx,
	const char *arg,
	struct berval *retval );

typedef int (rewrite_mapper_destroy)(
	void *ctx );

typedef struct rewrite_mapper {
	char *rm_name;
	rewrite_mapper_config *rm_config;
	rewrite_mapper_apply *rm_apply;
	rewrite_mapper_destroy *rm_destroy;
} rewrite_mapper;

/* For dynamic loading and unloading of mappers */
LDAP_REWRITE_F (int)
rewrite_mapper_register(
	const rewrite_mapper *map );

LDAP_REWRITE_F (int)
rewrite_mapper_unregister(
	const rewrite_mapper *map );

LDAP_REWRITE_F (const rewrite_mapper *)
rewrite_mapper_find(
	const char *name );

LDAP_END_DECL

#endif /* REWRITE_H */
