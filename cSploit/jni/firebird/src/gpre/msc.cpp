//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		msc.cpp
//		DESCRIPTION:	Miscellaneous little stuff
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings
//
//
//____________________________________________________________
//
//
//
//

// ***************************************************
// THIS MODULE HAS SEVERAL KISSING COUSINS; IF YOU
// SHOULD CHANGE ONE OF THE MODULES IN THE FOLLOWING
// LIST, PLEASE BE SURE TO CHECK THE OTHERS FOR
// SIMILAR CHANGES:
//
//                  qli/all.cpp
//                  gpre/msc.cpp
//
// - THANK YOU
// **************************************************

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../gpre/gpre.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/msc_proto.h"
#include "../yvalve/gds_proto.h"


struct gpre_space
{
	gpre_space* spc_next;
	SLONG spc_remaining;
};

static gpre_space* space;
static gpre_space* permanent_space;
static gpre_lls* free_lls;


//____________________________________________________________
//
//		Make an action and link it to a request.
//

act* MSC_action( gpre_req* request, act_t type)
{
	act* action = (act*) MSC_alloc(ACT_LEN);
	action->act_type = type;

	if (request)
	{
		action->act_next = request->req_actions;
		request->req_actions = action;
		action->act_request = request;
	}

	return action;
}


//____________________________________________________________
//
//

UCHAR* MSC_alloc(int size)
{
	size = FB_ALIGN(size, FB_ALIGNMENT);

	if (!space || size > space->spc_remaining)
	{
		const int n = MAX(size, 4096);
		gpre_space* next = (gpre_space*) gds__alloc((SLONG) (n + sizeof(gpre_space)));
		if (!next)
			CPR_error("virtual memory exhausted");
#ifdef DEBUG_GDS_ALLOC
		// For V4.0 we don't care about gpre specific memory leaks
		gds_alloc_flag_unfreed(next);
#endif
		next->spc_next = space;
		next->spc_remaining = n;
		space = next;
	}

	space->spc_remaining -= size;
	UCHAR* blk = ((UCHAR*) space + sizeof(gpre_space) + space->spc_remaining);
	memset(blk, 0, size);

	return blk;
}


//____________________________________________________________
//
//		Allocate a block in permanent memory.
//

UCHAR* MSC_alloc_permanent(int size)
{
	size = FB_ALIGN(size, FB_ALIGNMENT);

	if (!permanent_space || size > permanent_space->spc_remaining)
	{
		const int n = MAX(size, 4096);
		gpre_space* next = (gpre_space*) gds__alloc((SLONG) (n + sizeof(gpre_space)));
		if (!next)
			CPR_error("virtual memory exhausted");
#ifdef DEBUG_GDS_ALLOC
		// For V4.0 we don't care about gpre specific memory leaks
		gds_alloc_flag_unfreed(next);
#endif
		next->spc_next = permanent_space;
		next->spc_remaining = n;
		permanent_space = next;
	}

	permanent_space->spc_remaining -= size;
	UCHAR* blk = (UCHAR*) permanent_space + sizeof(gpre_space) + permanent_space->spc_remaining;
	memset(blk, 0, size);

	return blk;
}


//____________________________________________________________
//
//		Make a binary node.
//

gpre_nod* MSC_binary(nod_t type, gpre_nod* arg1, gpre_nod* arg2)
{
	gpre_nod* node = MSC_node(type, 2);
	node->nod_arg[0] = arg1;
	node->nod_arg[1] = arg2;

	return node;
}


//____________________________________________________________
//
//		Make a new context for a request and link it up to the request.
//

gpre_ctx* MSC_context(gpre_req* request)
{
	// allocate and initialize

	gpre_ctx* context = (gpre_ctx*) MSC_alloc(CTX_LEN);
	context->ctx_request = request;
	context->ctx_internal = request->req_internal++;
	context->ctx_scope_level = request->req_scope_level;

	// link in with the request block

	context->ctx_next = request->req_contexts;
	request->req_contexts = context;

	return context;
}


//____________________________________________________________
//
//		Copy one string into another.
//

void MSC_copy(const char* from, int length, char* to)
{

	if (length)
		memcpy(to, from, length);

	to[length] = 0;
}

//____________________________________________________________
//
//		Copy two strings into another.
//

void MSC_copy_cat(const char* from1, int length1, const char* from2, int length2, char* to)
{

	if (length1)
		memcpy(to, from1, length1);
	if (length2)
		memcpy(to + length1, from2, length2);

	to[length1 + length2] = 0;
}

//____________________________________________________________
//
//		Find a symbol of a particular type.
//

gpre_sym* MSC_find_symbol(gpre_sym* symbol, sym_t type)
{

	for (; symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == type)
			return symbol;

	return NULL;
}


//____________________________________________________________
//
//		Free a block.
//

void MSC_free(void*)
{
}


//____________________________________________________________
//
//		Get rid of an erroroneously allocated request block.
//

void MSC_free_request( gpre_req* request)
{

	gpreGlob.requests = request->req_next;
	gpreGlob.cur_routine->act_object = (ref*) request->req_routine;
	MSC_free(request);
}


//____________________________________________________________
//
//		Initialize (or more properly, re-initialize) the memory
//		allocator.
//

void MSC_init()
{
	free_lls = NULL;

    gpre_space* stuff;
	while (stuff = space)
	{
		space = space->spc_next;
		gds__free(stuff);
	}
}


//____________________________________________________________
//
//		Match the current token against a keyword.  If successful,
//		advance the token stream and return true.  Otherwise return
//		false.
//

bool MSC_match(const kwwords_t keyword)
{

	if (gpreGlob.token_global.tok_keyword == KW_none && gpreGlob.token_global.tok_symbol)
	{
		gpre_sym* symbol;
		for (symbol = gpreGlob.token_global.tok_symbol->sym_collision; symbol;
			symbol = symbol->sym_collision)
		{
			if ((strcmp(symbol->sym_string, gpreGlob.token_global.tok_string) == 0) &&
				symbol->sym_keyword != KW_none)
			{
				gpreGlob.token_global.tok_symbol = symbol;
				gpreGlob.token_global.tok_keyword = static_cast<kwwords_t>(symbol->sym_keyword);
			}
		}
	}

	if (gpreGlob.token_global.tok_keyword != keyword)
		return false;

	CPR_token();

	return true;
}

#ifdef NOT_USED_OR_REPLACED
//____________________________________________________________
//
//		Determinate where a specific object is
//		represented on a linked list stack.
//

bool MSC_member(const gpre_nod* object, const gpre_lls* stack)
{

	for (; stack; stack = stack->lls_next)
		if (stack->lls_object == object)
			return true;

	return false;
}
#endif

//____________________________________________________________
//
//		Allocate and initialize a syntax node.
//

gpre_nod* MSC_node(nod_t type, SSHORT count)
{
	gpre_nod* node = (gpre_nod*) MSC_alloc(NOD_LEN(count));
	node->nod_count = count;
	node->nod_type = type;

	return node;
}


//____________________________________________________________
//
//		Pop an item off a linked list stack.  Free the stack node.
//

gpre_nod* MSC_pop(gpre_lls** pointer)
{
	gpre_lls* stack = *pointer;
	gpre_nod* node = stack->lls_object;
	*pointer = stack->lls_next;

	stack->lls_next = free_lls;
	free_lls = stack;

	return node;
}


//____________________________________________________________
//
//       Allocate a new privilege (grant/revoke) block.
//

PRV MSC_privilege_block()
{
	PRV privilege_block = (PRV) MSC_alloc(PRV_LEN);
	privilege_block->prv_privileges = PRV_no_privs;
	privilege_block->prv_username = 0;
	privilege_block->prv_relation = 0;
	privilege_block->prv_fields = 0;
	privilege_block->prv_next = 0;

	return privilege_block;
}


//____________________________________________________________
//
//		Push an arbitrary object onto a linked list stack.
//

void MSC_push( gpre_nod* object, gpre_lls** pointer)
{
	gpre_lls* stack = free_lls;
	if (stack)
		free_lls = stack->lls_next;
	else
		stack = (gpre_lls*) MSC_alloc(LLS_LEN);

	stack->lls_object = object;
	stack->lls_next = *pointer;
	*pointer = stack;
}


//____________________________________________________________
//
//		Generate a reference and possibly link the reference into
//		a linked list.
//

ref* MSC_reference(ref** link)
{
	ref* reference = (ref*) MSC_alloc(REF_LEN);

	if (link)
	{
		reference->ref_next = *link;
		*link = reference;
	}

	return reference;
}


//____________________________________________________________
//
//		Set up for a new request.  Make request and action
//		blocks, all linked up and ready to go.
//

gpre_req* MSC_request(req_t type)
{
	gpre_req* request = (gpre_req*) MSC_alloc(REQ_LEN);
	request->req_type = type;
	request->req_next = gpreGlob.requests;
	gpreGlob.requests = request;

	request->req_routine = (gpre_req*) gpreGlob.cur_routine->act_object;
	gpreGlob.cur_routine->act_object = (ref*) request;

	if (!(gpreGlob.cur_routine->act_flags & ACT_main))
		request->req_flags |= REQ_local;
	if (gpreGlob.sw_sql_dialect <= SQL_DIALECT_V5)
		request->req_flags |= REQ_blr_version4;

	return request;
}


//____________________________________________________________
//
//		Copy a string into a permanent block.
//

SCHAR* MSC_string(const TEXT* input)
{
	TEXT* string = (TEXT*) MSC_alloc(strlen(input) + 1);
	strcpy(string, input);

	return string;
}


//____________________________________________________________
//
//		Allocate and initialize a symbol block.
//

gpre_sym* MSC_symbol(sym_t type, const TEXT* string, USHORT length, gpre_ctx* object)
{
	gpre_sym* symbol = (gpre_sym*) MSC_alloc(SYM_LEN + length);
	symbol->sym_type = type;
	symbol->sym_object = object;
	TEXT* p = symbol->sym_name;
	symbol->sym_string = p;

	if (length)
		memcpy(p, string, length);

	return symbol;
}


//____________________________________________________________
//
//		Make a unary node.
//

gpre_nod* MSC_unary(nod_t type, gpre_nod* arg)
{
	gpre_nod* node = MSC_node(type, 1);
	node->nod_arg[0] = arg;

	return node;
}


//____________________________________________________________
//
//		Set up for a new username.
//

gpre_usn* MSC_username(const SCHAR* name, USHORT name_dyn)
{
	gpre_usn* username = (gpre_usn*) MSC_alloc(USN_LEN);
	char* newname = (char*) MSC_alloc(strlen(name) + 1);
	username->usn_name = newname;
	strcpy(newname, name);
	username->usn_dyn = name_dyn;

	username->usn_next = 0;

	return username;
}

