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

#include <portable.h>

#include "rewrite-int.h"

/*
 * Compares two struct rewrite_context based on the name;
 * used by avl stuff
 */
static int
rewrite_context_cmp(
		const void *c1,
		const void *c2
)
{
	const struct rewrite_context *lc1, *lc2;
	
	lc1 = (const struct rewrite_context *)c1;
	lc2 = (const struct rewrite_context *)c2;
	
	assert( c1 != NULL );
	assert( c2 != NULL );
	assert( lc1->lc_name != NULL );
	assert( lc2->lc_name != NULL );
	
	return strcasecmp( lc1->lc_name, lc2->lc_name );
}

/*
 * Returns -1 in case a duplicate struct rewrite_context
 * has been inserted; used by avl stuff
 */
static int
rewrite_context_dup(
		void *c1,
		void *c2
		)
{
	struct rewrite_context *lc1, *lc2;
	
	lc1 = (struct rewrite_context *)c1;
	lc2 = (struct rewrite_context *)c2;
	
	assert( c1 != NULL );
	assert( c2 != NULL );
	assert( lc1->lc_name != NULL );
	assert( lc2->lc_name != NULL );
	
	return( strcasecmp( lc1->lc_name, lc2->lc_name) == 0 ? -1 : 0 );
}

/*
 * Finds the context named rewriteContext in the context tree
 */
struct rewrite_context *
rewrite_context_find(
		struct rewrite_info *info,
		const char *rewriteContext
)
{
	struct rewrite_context *context, c;

	assert( info != NULL );
	assert( rewriteContext != NULL );

	/*
	 * Fetches the required rewrite context
	 */
	c.lc_name = (char *)rewriteContext;
	context = (struct rewrite_context *)avl_find( info->li_context, 
			(caddr_t)&c, rewrite_context_cmp );
	if ( context == NULL ) {
		return NULL;
	}

	/*
	 * De-aliases the context if required
	 */
	if ( context->lc_alias ) {
		return context->lc_alias;
	}

	return context;
}

/*
 * Creates a new context called rewriteContext and stores in into the tree
 */
struct rewrite_context *
rewrite_context_create(
		struct rewrite_info *info,
		const char *rewriteContext
)
{
	struct rewrite_context *context;
	int rc;

	assert( info != NULL );
	assert( rewriteContext != NULL );
	
	context = calloc( sizeof( struct rewrite_context ), 1 );
	if ( context == NULL ) {
		return NULL;
	}
	
	/*
	 * Context name
	 */
	context->lc_name = strdup( rewriteContext );
	if ( context->lc_name == NULL ) {
		free( context );
		return NULL;
	}

	/*
	 * The first, empty rule
	 */
	context->lc_rule = calloc( sizeof( struct rewrite_rule ), 1 );
	if ( context->lc_rule == NULL ) {
		free( context->lc_name );
		free( context );
		return NULL;
	}
	memset( context->lc_rule, 0, sizeof( struct rewrite_rule ) );
	
	/*
	 * Add context to tree
	 */
	rc = avl_insert( &info->li_context, (caddr_t)context,
			rewrite_context_cmp, rewrite_context_dup );
	if ( rc == -1 ) {
		free( context->lc_rule );
		free( context->lc_name );
		free( context );
		return NULL;
	}

	return context;
}

/*
 * Finds the next rule according to a goto action statement,
 * or null in case of error.
 * Helper for rewrite_context_apply.
 */
static struct rewrite_rule *
rewrite_action_goto(
		struct rewrite_action *action,
		struct rewrite_rule *rule
)
{
	int n;
	
	assert( action != NULL );
	assert( action->la_args != NULL );
	assert( rule != NULL );
	
	n = ((int *)action->la_args)[ 0 ];
	
	if ( n > 0 ) {
		for ( ; n > 1 && rule != NULL ; n-- ) {
			rule = rule->lr_next;
		}
	} else if ( n <= 0 ) {
		for ( ; n < 1 && rule != NULL ; n++ ) {
			rule = rule->lr_prev;
		}
	}

	return rule;
}

/*
 * Rewrites string according to context; may return:
 *      OK:     fine; if *result != NULL rule matched and rewrite succeeded.
 *      STOP:   fine, rule matched; stop processing following rules
 *      UNWILL: rule matched; force 'unwilling to perform'
 */
int
rewrite_context_apply(
		struct rewrite_info *info,
		struct rewrite_op *op,
		struct rewrite_context *context,
		const char *string,
		char **result
)
{
	struct rewrite_rule *rule;
	char *s, *res = NULL;
	int return_code = REWRITE_REGEXEC_OK;
	
	assert( info != NULL );
	assert( op != NULL );
	assert( context != NULL );
	assert( context->lc_rule != NULL );
	assert( string != NULL );
	assert( result != NULL );

	op->lo_depth++;

	Debug( LDAP_DEBUG_TRACE, "==> rewrite_context_apply"
			" [depth=%d] string='%s'\n",
			op->lo_depth, string, 0 );
	assert( op->lo_depth > 0 );
	
	s = (char *)string;
	
	for ( rule = context->lc_rule->lr_next;
			rule != NULL && op->lo_num_passes < info->li_max_passes;
			rule = rule->lr_next, op->lo_num_passes++ ) {
		int rc;
		
		/*
		 * Apply a single rule
		 */
		rc = rewrite_rule_apply( info, op, rule, s, &res );
		
		/*
		 * A rule may return:
		 * 	OK 		with result != NULL if matched
		 * 	ERR		if anything was wrong
		 * 	UNWILLING	if the server should drop the request
		 * the latter case in honored immediately;
		 * the other two may require some special actions to take
		 * place.
		 */
		switch ( rc ) {
			
		case REWRITE_REGEXEC_ERR:
			Debug( LDAP_DEBUG_ANY, "==> rewrite_context_apply"
					" error ...\n", 0, 0, 0);

			/*
			 * Checks for special actions to be taken
			 * in case of error ...
			 */
			if ( rule->lr_action != NULL ) {
				struct rewrite_action *action;
				int do_continue = 0;
				
				for ( action = rule->lr_action;
						action != NULL;
						action = action->la_next ) {
					switch ( action->la_type ) {
					
					/*
					 * This action takes precedence
					 * over the others in case of failure
					 */
					case REWRITE_ACTION_IGNORE_ERR:
						Debug( LDAP_DEBUG_ANY,
					"==> rewrite_context_apply"
					" ignoring error ...\n", 0, 0, 0 );
						do_continue = 1;
						break;

					/*
					 * Goto is honored only if it comes
					 * after ignore error
					 */
					case REWRITE_ACTION_GOTO:
						if ( do_continue ) {
							rule = rewrite_action_goto( action, rule );
							if ( rule == NULL ) {
								return_code = REWRITE_REGEXEC_ERR;
								goto rc_end_of_context;
							}
						}
						break;

					/*
					 * Other actions are ignored
					 */
					default:
						break;
					}
				}

				if ( do_continue ) {
					if ( rule->lr_next == NULL ) {
						res = s;
					}
					goto rc_continue;
				}
			}

			/* 
			 * Default behavior is to bail out ...
			 */
			return_code = REWRITE_REGEXEC_ERR;
			goto rc_end_of_context;
		
		/*
		 * OK means there were no errors or special return codes;
		 * if res is defined, it means the rule matched and we
		 * got a sucessful rewriting
		 */
		case REWRITE_REGEXEC_OK:

			/*
			 * It matched! Check for actions ...
			 */
			if ( res != NULL ) {
				struct rewrite_action *action;
				
				if ( s != string && s != res ) {
					free( s );
				}
				s = res;

				for ( action = rule->lr_action;
						action != NULL;
						action = action->la_next ) {

					switch ( action->la_type ) {

					/*
					 * This ends the rewrite context
					 * successfully
					 */
					case REWRITE_ACTION_STOP:
						goto rc_end_of_context;
					
					/*
					 * This instructs the server to return
					 * an `unwilling to perform' error
					 * message
					 */
					case REWRITE_ACTION_UNWILLING:
						return_code = REWRITE_REGEXEC_UNWILLING;
						goto rc_end_of_context;
					
					/*
					 * This causes the processing to
					 * jump n rules back and forth
					 */
					case REWRITE_ACTION_GOTO:
						rule = rewrite_action_goto( action, rule );
						if ( rule == NULL ) {
							return_code = REWRITE_REGEXEC_ERR;
							goto rc_end_of_context;
						}
						break;

					/*
					 * This ends the rewrite context
					 * and returns a user-defined
					 * error code
					 */
					case REWRITE_ACTION_USER:
						return_code = ((int *)action->la_args)[ 0 ];
						goto rc_end_of_context;
					
					default:
						/* ... */
						break;
					}
				}

			/*
			 * If result was OK and string didn't match,
			 * in case of last rule we need to set the
			 * result back to the string
			 */
			} else if ( rule->lr_next == NULL ) {
				res = s;
			}
			
			break;

		/*
		 * A STOP has propagated ...
		 */
		case REWRITE_REGEXEC_STOP:
			goto rc_end_of_context;

		/*
		 * This will instruct the server to return
		 * an `unwilling to perform' error message
		 */
		case REWRITE_REGEXEC_UNWILLING:
			return_code = REWRITE_REGEXEC_UNWILLING;
			goto rc_end_of_context;

		/*
		 * A user-defined error code has propagated ...
		 */
		default:
			assert( rc >= REWRITE_REGEXEC_USER );
			goto rc_end_of_context;

		}
		
rc_continue:;	/* sent here by actions that require to continue */

	}

rc_end_of_context:;
	*result = res;

	Debug( LDAP_DEBUG_TRACE, "==> rewrite_context_apply"
			" [depth=%d] res={%d,'%s'}\n",
			op->lo_depth, return_code, ( res ? res : "NULL" ) );

	assert( op->lo_depth > 0 );
	op->lo_depth--;

	return return_code;
}

void
rewrite_context_free(
		void *tmp
)
{
	struct rewrite_context *context = (struct rewrite_context *)tmp;

	assert( tmp != NULL );

	rewrite_context_destroy( &context );
}

int
rewrite_context_destroy(
		struct rewrite_context **pcontext
)
{
	struct rewrite_context *context;
	struct rewrite_rule *r;

	assert( pcontext != NULL );
	assert( *pcontext != NULL );

	context = *pcontext;

	assert( context->lc_rule != NULL );

	for ( r = context->lc_rule->lr_next; r; ) {
		struct rewrite_rule *cr = r;

		r = r->lr_next;
		rewrite_rule_destroy( &cr );
	}

	free( context->lc_rule );
	context->lc_rule = NULL;

	assert( context->lc_name != NULL );
	free( context->lc_name );
	context->lc_name = NULL;

	free( context );
	*pcontext = NULL;
	
	return 0;
}
