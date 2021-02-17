/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#ifndef REWRITE_INT_H
#define REWRITE_INT_H

/*
 * These are required by every file of the library, so they're included here
 */
#include <ac/stdlib.h>
#include <ac/string.h>
#include <ac/syslog.h>
#include <ac/regex.h>
#include <ac/socket.h>
#include <ac/unistd.h>
#include <ac/ctype.h>

#include <lber.h>
#include <ldap.h>
#define LDAP_DEFINE_LDAP_DEBUG
#include <ldap_log.h>
#include <lutil.h>
#include <avl.h>

#include <rewrite.h>

#define malloc(x)	ber_memalloc(x)
#define calloc(x,y)	ber_memcalloc(x,y)
#define realloc(x,y)	ber_memrealloc(x,y)
#define free(x)	ber_memfree(x)
#undef strdup
#define	strdup(x)	ber_strdup(x)

/* Uncomment to use ldap pvt threads */
#define USE_REWRITE_LDAP_PVT_THREADS
#include <ldap_pvt_thread.h>

/*
 * For details, see RATIONALE.
 */

#define REWRITE_MAX_MATCH	11	/* 0: overall string; 1-9: submatches */
#define REWRITE_MAX_PASSES	100

/*
 * Submatch escape char
 */
/* the '\' conflicts with slapd.conf parsing */
/* #define REWRITE_SUBMATCH_ESCAPE			'\\' */
#define REWRITE_SUBMATCH_ESCAPE_ORIG		'%'
#define REWRITE_SUBMATCH_ESCAPE			'$'
#define IS_REWRITE_SUBMATCH_ESCAPE(c) \
	((c) == REWRITE_SUBMATCH_ESCAPE || (c) == REWRITE_SUBMATCH_ESCAPE_ORIG)

/*
 * REGEX flags
 */

#define REWRITE_FLAG_HONORCASE			'C'
#define REWRITE_FLAG_BASICREGEX			'R'

/*
 * Action flags
 */
#define REWRITE_FLAG_EXECONCE			':'
#define REWRITE_FLAG_STOP			'@'
#define REWRITE_FLAG_UNWILLING			'#'
#define REWRITE_FLAG_GOTO			'G'	/* requires an arg */
#define REWRITE_FLAG_USER			'U'	/* requires an arg */
#define REWRITE_FLAG_MAX_PASSES			'M'	/* requires an arg */
#define REWRITE_FLAG_IGNORE_ERR			'I'

/*
 * Map operators
 */
#define REWRITE_OPERATOR_SUBCONTEXT		'>'
#define REWRITE_OPERATOR_COMMAND		'|'
#define REWRITE_OPERATOR_VARIABLE_SET		'&'
#define REWRITE_OPERATOR_VARIABLE_GET		'*'
#define REWRITE_OPERATOR_PARAM_GET		'$'


/***********
 * PRIVATE *
 ***********/

/*
 * Action
 */
struct rewrite_action {
	struct rewrite_action          *la_next;
	
#define REWRITE_ACTION_STOP		0x0001
#define REWRITE_ACTION_UNWILLING	0x0002
#define REWRITE_ACTION_GOTO		0x0003
#define REWRITE_ACTION_IGNORE_ERR	0x0004
#define REWRITE_ACTION_USER		0x0005
	int                             la_type;
	void                           *la_args;
};

/*
 * Map
 */
struct rewrite_map {

	/*
	 * Legacy stuff
	 */
#define REWRITE_MAP_XFILEMAP		0x0001	/* Rough implementation! */
#define REWRITE_MAP_XPWDMAP		0x0002  /* uid -> gecos */
#define REWRITE_MAP_XLDAPMAP		0x0003	/* Not implemented yet! */

	/*
	 * Maps with args
	 */
#define REWRITE_MAP_SUBCONTEXT		0x0101
	
#define REWRITE_MAP_SET_OP_VAR		0x0102
#define REWRITE_MAP_SETW_OP_VAR		0x0103
#define REWRITE_MAP_GET_OP_VAR		0x0104
#define	REWRITE_MAP_SET_SESN_VAR	0x0105
#define REWRITE_MAP_SETW_SESN_VAR	0x0106
#define	REWRITE_MAP_GET_SESN_VAR	0x0107
#define REWRITE_MAP_GET_PARAM		0x0108
#define REWRITE_MAP_BUILTIN		0x0109
	int                             lm_type;

	char                           *lm_name;
	void                           *lm_data;

	/*
	 * Old maps store private data in _lm_args;
	 * new maps store the substitution pattern in _lm_subst
	 */
	union {		
	        void                   *_lm_args;
		struct rewrite_subst   *_lm_subst;
	} lm_union;
#define	lm_args lm_union._lm_args
#define	lm_subst lm_union._lm_subst

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_mutex_t         lm_mutex;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
};

/*
 * Builtin maps
 */
struct rewrite_builtin_map {
#define REWRITE_BUILTIN_MAP	0x0200
	int                             lb_type;
	char                           *lb_name;
	void                           *lb_private;
	const rewrite_mapper		   *lb_mapper;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_mutex_t         lb_mutex;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
};

/*
 * Submatch substitution
 */
struct rewrite_submatch {
#define REWRITE_SUBMATCH_ASIS		0x0000
#define REWRITE_SUBMATCH_XMAP		0x0001
#define REWRITE_SUBMATCH_MAP_W_ARG	0x0002
	int                             ls_type;
	struct rewrite_map             *ls_map;
	int                             ls_submatch;
	/*
	 * The first one represents the index of the submatch in case
	 * the map has single submatch as argument;
	 * the latter represents the map argument scheme in case
	 * the map has substitution string argument form
	 */
};

/*
 * Pattern substitution
 */
struct rewrite_subst {
	size_t                          lt_subs_len;
	struct berval                  *lt_subs;
	
	int                             lt_num_submatch;
	struct rewrite_submatch        *lt_submatch;
};

/*
 * Rule
 */
struct rewrite_rule {
	struct rewrite_rule            *lr_next;
	struct rewrite_rule            *lr_prev;

	char                           *lr_pattern;
	char                           *lr_subststring;
	char                           *lr_flagstring;
	regex_t				lr_regex;

	/*
	 * I was thinking about some kind of per-rule mutex, but there's
	 * probably no need, because rules after compilation are only read;
	 * however, I need to check whether regexec is reentrant ...
	 */

	struct rewrite_subst           *lr_subst;
	
#define REWRITE_REGEX_ICASE		REG_ICASE
#define REWRITE_REGEX_EXTENDED		REG_EXTENDED	
	int                             lr_flags;

#define REWRITE_RECURSE			0x0001
#define REWRITE_EXEC_ONCE          	0x0002
	int				lr_mode;
	int				lr_max_passes;

	struct rewrite_action          *lr_action;
};

/*
 * Rewrite Context (set of rules)
 */
struct rewrite_context {
	char                           *lc_name;
	struct rewrite_context         *lc_alias;
	struct rewrite_rule            *lc_rule;
};

/*
 * Session
 */
struct rewrite_session {
	void                           *ls_cookie;
	Avlnode                        *ls_vars;
#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_t          ls_vars_mutex;
	ldap_pvt_thread_mutex_t		ls_mutex;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */
	int				ls_count;
};

/*
 * Variable
 */
struct rewrite_var {
	char                           *lv_name;
	int				lv_flags;
	struct berval                   lv_value;
};

/*
 * Operation
 */
struct rewrite_op {
	int                             lo_num_passes;
	int                             lo_depth;
#if 0 /* FIXME: not used anywhere! (debug? then, why strdup?) */
	char                           *lo_string;
#endif
	char                           *lo_result;
	Avlnode                        *lo_vars;
	const void                     *lo_cookie;
};


/**********
 * PUBLIC *
 **********/

/*
 * Rewrite info
 */
struct rewrite_info {
	Avlnode                        *li_context;
	Avlnode                        *li_maps;
	/*
	 * No global mutex because maps are read only at 
	 * config time
	 */
	Avlnode                        *li_params;
	Avlnode                        *li_cookies;
	int                             li_num_cookies;

#ifdef USE_REWRITE_LDAP_PVT_THREADS
	ldap_pvt_thread_rdwr_t          li_params_mutex;
        ldap_pvt_thread_rdwr_t          li_cookies_mutex;
#endif /* USE_REWRITE_LDAP_PVT_THREADS */

	/*
	 * Default to `off';
	 * use `rewriteEngine {on|off}' directive to alter
	 */
	int				li_state;

	/*
	 * Defaults to REWRITE_MAXPASSES;
	 * use `rewriteMaxPasses numPasses' directive to alter
	 */
#define REWRITE_MAXPASSES		100
	int                             li_max_passes;
	int                             li_max_passes_per_rule;

	/*
	 * Behavior in case a NULL or non-existent context is required
	 */
	int                             li_rewrite_mode;
};

/***********
 * PRIVATE *
 ***********/

LDAP_REWRITE_V (struct rewrite_context*) rewrite_int_curr_context;

/*
 * Maps
 */

/*
 * Parses a map (also in legacy 'x' version)
 */
LDAP_REWRITE_F (struct rewrite_map *)
rewrite_map_parse(
		struct rewrite_info *info,
		const char *s,
		const char **end
);

LDAP_REWRITE_F (struct rewrite_map *)
rewrite_xmap_parse(
		struct rewrite_info *info,
		const char *s,
		const char **end
);

/*
 * Resolves key in val by means of map (also in legacy 'x' version)
 */
LDAP_REWRITE_F (int)
rewrite_map_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_map *map,
		struct berval *key,
		struct berval *val
);

LDAP_REWRITE_F (int)
rewrite_xmap_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_map *map,
		struct berval *key,
		struct berval *val
);

LDAP_REWRITE_F (int)
rewrite_map_destroy(
		struct rewrite_map **map
);

LDAP_REWRITE_F (int)
rewrite_xmap_destroy(
		struct rewrite_map **map
);

LDAP_REWRITE_F (void)
rewrite_builtin_map_free(
		void *map
);
/*
 * Submatch substitution
 */

/*
 * Compiles a substitution pattern
 */
LDAP_REWRITE_F (struct rewrite_subst *)
rewrite_subst_compile(
		struct rewrite_info *info,
		const char *result
);

/*
 * Substitutes a portion of rewritten string according to substitution
 * pattern using submatches
 */
LDAP_REWRITE_F (int)
rewrite_subst_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_subst *subst,
		const char *string,
		const regmatch_t *match,
		struct berval *val
);

LDAP_REWRITE_F (int)
rewrite_subst_destroy(
		struct rewrite_subst **subst
);


/*
 * Rules
 */

/*
 * Compiles the rule and appends it at the running context
 */
LDAP_REWRITE_F (int)
rewrite_rule_compile(
		struct rewrite_info *info,
		struct rewrite_context *context,
		const char *pattern,
		const char *result,
		const char *flagstring
);

/*
 * Rewrites string according to rule; may return:
 *      REWRITE_REGEXEC_OK:	fine; if *result != NULL rule matched
 *      			and rewrite succeeded.
 *      REWRITE_REGEXEC_STOP:   fine, rule matched; stop processing
 *      			following rules
 *      REWRITE_REGEXEC_UNWILL: rule matched; force 'unwilling to perform'
 *      REWRITE_REGEXEC_ERR:	an error occurred
 */
LDAP_REWRITE_F (int)
rewrite_rule_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_rule *rule,
		const char *string,
		char **result
);

LDAP_REWRITE_F (int)
rewrite_rule_destroy(
		struct rewrite_rule **rule
);

/*
 * Sessions
 */

/*
 * Fetches a struct rewrite_session
 */
LDAP_REWRITE_F (struct rewrite_session *)
rewrite_session_find(
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

/*
 * Gets a var with session scope
 */
LDAP_REWRITE_F (int)
rewrite_session_var_get(
                struct rewrite_info *info,
                const void *cookie,
                const char *name,
                struct berval *val
);

/*
 * Deletes a session
 */
LDAP_REWRITE_F (int)
rewrite_session_delete(
                struct rewrite_info *info,
                const void *cookie
);

/*
 * Destroys the cookie tree
 */
LDAP_REWRITE_F (int)
rewrite_session_destroy(
                struct rewrite_info *info
);


/*
 * Vars
 */

/*
 * Finds a var
 */
LDAP_REWRITE_F (struct rewrite_var *)
rewrite_var_find(
                Avlnode *tree,
                const char *name
);

/*
 * Replaces the value of a variable
 */
LDAP_REWRITE_F (int)
rewrite_var_replace(
		struct rewrite_var *var,
		const char *value,
		int flags
);

/*
 * Inserts a newly created var
 */
LDAP_REWRITE_F (struct rewrite_var *)
rewrite_var_insert_f(
                Avlnode **tree,
                const char *name,
                const char *value,
		int flags
);

#define rewrite_var_insert(tree, name, value) \
	rewrite_var_insert_f((tree), (name), (value), \
			REWRITE_VAR_UPDATE|REWRITE_VAR_COPY_NAME|REWRITE_VAR_COPY_VALUE)

/*
 * Sets/inserts a var
 */
LDAP_REWRITE_F (struct rewrite_var *)
rewrite_var_set_f(
                Avlnode **tree,
                const char *name,
                const char *value,
                int flags
);

#define rewrite_var_set(tree, name, value, insert) \
	rewrite_var_set_f((tree), (name), (value), \
			REWRITE_VAR_UPDATE|REWRITE_VAR_COPY_NAME|REWRITE_VAR_COPY_VALUE|((insert)? REWRITE_VAR_INSERT : 0))

/*
 * Deletes a var tree
 */
LDAP_REWRITE_F (int)
rewrite_var_delete(
                Avlnode *tree
);


/*
 * Contexts
 */

/*
 * Finds the context named rewriteContext in the context tree
 */
LDAP_REWRITE_F (struct rewrite_context *)
rewrite_context_find(
		struct rewrite_info *info,
		const char *rewriteContext
);

/*
 * Creates a new context called rewriteContext and stores in into the tree
 */
LDAP_REWRITE_F (struct rewrite_context *)
rewrite_context_create(
		struct rewrite_info *info,
		const char *rewriteContext
);

/*
 * Rewrites string according to context; may return:
 *      OK:     fine; if *result != NULL rule matched and rewrite succeeded.
 *      STOP:   fine, rule matched; stop processing following rules
 *      UNWILL: rule matched; force 'unwilling to perform'
 */
LDAP_REWRITE_F (int)
rewrite_context_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_context *context,
		const char *string,
		char **result
);

LDAP_REWRITE_F (int)
rewrite_context_destroy(
		struct rewrite_context **context
);

LDAP_REWRITE_F (void)
rewrite_context_free(
		void *tmp
);

#endif /* REWRITE_INT_H */

