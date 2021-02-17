/*
 *	PROGRAM:	InterBase Access Method
 *	MODULE:		functions.c
 *	DESCRIPTION:	External entrypoint definitions
 *
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

typedef int	(*FUN_PTR)();

typedef struct {
    char	*fn_module;
    char	*fn_entrypoint;
    FUN_PTR	fn_function;
} FN;

static		test();
extern char	*fn_lower_c();
extern char	*fn_strcat();
extern char	*fn_substr();
extern char	*fn_trim();
extern char	*fn_trunc();
extern int	fn_doy();
extern short	*fn_moy();
extern char	*fn_dow();
extern char	*fn_sysdate();
extern int	fn_add2();
extern double	fn_mul();
extern double	fn_fact();
extern double	fn_abs();
extern double	fn_max();
extern double	fn_sqrt();
extern int	fn_blob_linecount();
extern int	fn_blob_bytecount();
extern char	*fn_blob_substr();

static FN	isc_functions [] = {
    "test_module", "test_function", test,
    "FUNCLIB", "fn_lower_c", (FUN_PTR) fn_lower_c,
    "FUNCLIB", "fn_strcat", (FUN_PTR) fn_strcat,
    "FUNCLIB", "fn_substr", (FUN_PTR) fn_substr,
    "FUNCLIB", "fn_trim", (FUN_PTR) fn_trim,
    "FUNCLIB", "fn_trunc", (FUN_PTR) fn_trunc,
    "FUNCLIB", "fn_doy", (FUN_PTR) fn_doy,
    "FUNCLIB", "fn_moy", (FUN_PTR) fn_moy,
    "FUNCLIB", "fn_dow", (FUN_PTR) fn_dow,
    "FUNCLIB", "fn_sysdate", (FUN_PTR) fn_sysdate,
    "FUNCLIB", "fn_add2", (FUN_PTR) fn_add2,
    "FUNCLIB", "fn_mul", (FUN_PTR) fn_mul,
    "FUNCLIB", "fn_fact", (FUN_PTR) fn_fact,
    "FUNCLIB", "fn_abs", (FUN_PTR) fn_abs,
    "FUNCLIB", "fn_max", (FUN_PTR) fn_max,
    "FUNCLIB", "fn_sqrt", (FUN_PTR) fn_sqrt,
    "FUNCLIB", "fn_blob_linecount", (FUN_PTR) fn_blob_linecount,
    "FUNCLIB", "fn_blob_bytecount", (FUN_PTR) fn_blob_bytecount,
    "FUNCLIB", "fn_blob_substr", (FUN_PTR) fn_blob_substr,
    0, 0, 0};

#ifdef SHLIB_DEFS
#define strcmp		(*_libfun_strcmp)
#define sprintf		(*_libfun_sprintf)

extern int		strcmp();
extern int		sprintf();
#endif

FUN_PTR FUNCTIONS_entrypoint (module, entrypoint)
    char	*module;
    char	*entrypoint;
{
/**************************************
 *
 *	F U N C T I O N S _ e n t r y p o i n t
 *
 **************************************
 *
 * Functional description
 *	Lookup function in hardcoded table.  The module and
 *	entrypoint names are null terminated, but may contain
 *	insignificant trailing blanks.
 *
 **************************************/
FN	*function;
char	*p, temp [64], *ep;

p = temp;

while (*module && *module != ' ')
    *p++ = *module++;

*p++ = 0;
ep = p;

while (*entrypoint && *entrypoint != ' ')
    *p++ = *entrypoint++;

*p = 0;

for (function = isc_functions; function->fn_module; ++function)
    if (!strcmp (temp, function->fn_module) && !strcmp (ep, function->fn_entrypoint))
	return function->fn_function;

return 0;
}

static test (n, result)
    int		n;
    char	*result;
{
/**************************************
 *
 *	t e s t
 *
 **************************************
 *
 * Functional description
 *	Sample extern function.  Defined in database by:
 *
 *	define function test module_name "test_module" entry_point "test_function"
 *	    long by value,
 *	    char [20] by reference return_argument;
 *
 **************************************/
char	*end;

sprintf (result, "%d is a number", n);
end = result + 20;

while (*result)
    result++;

while (result < end)
    *result++ = ' ';
}
