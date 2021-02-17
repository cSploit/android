/*
 *	Test tool for Language Drivers.
 *
 *	This tool loads up a language driver using the dynamic link
 *	(or shared object) method and interrogates it's ID function.
 *	This tool is used to quickly verify the dynamic load ability
 *	of a newly created language driver.
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

/*  define LIKE_JRD to have the lookup follow exactly the syntax
 *  used by intl.c in JRD.
 *  Set the static full_debug to 1 to turn on printf debugging.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "Apollo" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#define DEBUG

static int full_debug = 0;
#define	FULL_DEBUG	if (full_debug) printf



typedef unsigned short SHORT;

#include "../jrd/intl.h"

/* Following defines are duplicates of those in intl.c */
/* Name of module that implements text-type (n) */

#ifndef INTL_MODULE
#define	INTL_MODULE "IBLD_%03d"
#endif

#ifndef INTL_INIT_ENTRY
#define INTL_INIT_ENTRY "ld_init"
#endif


void try_fc(char* c, FPTR_INT f)
{
	unsigned char buffer[200];
	const int res = (*f) (strlen(c), c, sizeof(buffer), buffer);
	printf("%s => ", c);
	for (int i = 0; i < res; i++)
		printf("%d ", buffer[i]);
	printf("\n");
}

int main(int argc, char** argv)
{
	char buffer[200];
	struct texttype this_textobj;

	if (argc <= 1)
	{
		printf("usage: dtest Intl_module_name\n");
		return (1);
	}
	char** vector = argv;

	FPTR_INT func = 0;

	for (int i = 1; i < argc; i++)
	{

#ifdef LIKE_JRD
		{
			char module[200];
			Firebird::PathName path;
			char entry[200];
			const int t_type = atoi(vector[i]);
			sprintf(module, INTL_MODULE, t_type);
			path = fb_utils::getPrefix(Firebird::DirType::FB_DIR_LIB, module);
			sprintf(entry, INTL_INIT_ENTRY, t_type);
			printf("path=%s entry=%s\n", path.c_str(), entry);
			func = (FPTR_INT) ISC_lookup_entrypoint(path.c_str(), entry, NULL);
		}
#else
		if (strcmp(vector[i], "ask") == 0)
		{
			gets(buffer);
			func = (FPTR_INT) ISC_lookup_entrypoint(buffer, "ld_init", NULL);
		}
		else
			func = (FPTR_INT) ISC_lookup_entrypoint(vector[i], "ld_init", NULL);
#endif
		if (func == NULL)
			printf("Cannot find %s.init\n", vector[i]);
		else
		{
			FULL_DEBUG("This testobj %ld\n", &this_textobj);
			FULL_DEBUG("size of %d\n", sizeof(this_textobj));
			if ((*func) (99, &this_textobj) != 99)
				printf("%s.Init returned bad result\n", vector[i]);
			else
			{
				FULL_DEBUG("Called init ok\n");
				FULL_DEBUG("ld_init is %ld %ld\n",
						   this_textobj.
						   texttype_functions[(int) intl_fn_init], func);
				func = this_textobj.texttype_functions[intl_fn_NULL];
				FULL_DEBUG("ld_id is %ld %ld\n",
						   this_textobj.
						   texttype_functions[(int) intl_fn_NULL], func);
				if (func == NULL)
					printf("%s.Init OK can't find ID\n", vector[i]);
				else
				{
					FULL_DEBUG("About to call ID fn\n");
					(*func) (sizeof(buffer), buffer);
					FULL_DEBUG("Back from ID fn \n");
					printf("%s.id => %s\n", vector[i], buffer);
				}

				func = this_textobj.texttype_functions[intl_fn_string_to_key];

				FULL_DEBUG("ld_to_key is %ld\n", func);
				if (func == NULL)
					printf("%s: Can't find str_to_key\n", vector[i]);
				else
				{
					try_fc("cote", func);
					try_fc("COTE", func);
					try_fc("co-te", func);
					try_fc("CO-TE", func);
				}
			}
		}
	}
	return (0);
}

