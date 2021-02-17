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

#include "firebird.h"
#include <stdio.h>

static int full_debug = 0;
#define	FULL_DEBUG	if (full_debug) printf


#define DEBUG_INTL
#include "../jrd/intl.c"


/*
void try_fc(char* c, FUN_PTR f)
{
	unsigned char buffer[200];
	const int res = (*f) (strlen(c), c, sizeof(buffer), buffer);
	printf("%s => ", c);
	for (int i = 0; i < res; i++)
		printf("%d ", buffer[i]);
	printf("\n");
}
*/

void my_err()
{
	printf("Error routine called!\n");
}

int main(int argc, char** argv)
{
	if (argc <= 1) {
		printf("usage: dtest Intl_module_name\n");
		return (1);
	}
	char** vector = argv;

	struct texttype this_textobj;
	for (int i = 1; i < argc; i++) {
		const int t_type = atoi(vector[i]);
		INTL_fn_init(t_type, &this_textobj, my_err);
	}
	return (0);
}

