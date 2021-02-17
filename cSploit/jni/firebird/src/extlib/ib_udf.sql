/*
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
 * Revision 1.2  2000/11/28 06:47:52  fsg
 * Changed declaration of ascii_char in ib_udf.sql
 * to get correct result as proposed by Claudio Valderrama
 * 2001.5.19 Claudio Valderrama, add the declaration of alternative
 * substrlen function to handle string,start,length instead.
 *
 */
/*****************************************
 *
 *	a b s
 *
 *****************************************
 *
 * Functional description:
 * 	Returns the absolute value of a
 * 	number.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION abs
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_abs' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a c o s
 *
 *****************************************
 *
 * Functional description:
 *	Returns the arccosine of a number
 *	between -1 and 1, if the number is
 *	out of bounds it returns NaN, as handled
 *	by the _matherr routine.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION acos
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_acos' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a s c i i _ c h a r
 *
 *****************************************
 *
 * Functional description:
 *	Returns the ASCII character corresponding
 *	with the value passed in.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION ascii_char
	INTEGER
	RETURNS CSTRING(1) FREE_IT
	ENTRY_POINT 'IB_UDF_ascii_char' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a s c i i _ v a l
 *
 *****************************************
 *
 * Functional description:
 *	Returns the ascii value of the character
 * 	passed in.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION ascii_val
	CHAR(1)
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_ascii_val' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a s i n
 *
 *****************************************
 *
 * Functional description:
 *	Returns the arcsin of a number between
 *	-1 and 1, if the number is out of
 *	range NaN is returned.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION asin
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_asin' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a t a n
 *
 *****************************************
 *
 * Functional description:
 *	Returns the arctangent of a number.
 *
 *
 *****************************************
DECLARE EXTERNAL FUNCTION atan
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_atan' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	a t a n 2
 *
 *****************************************
 *
 * Functional description:
 * 	Returns the arctangent of the
 *	first param / the second param.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION atan2
	DOUBLE PRECISION, DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_atan2' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	b i n _ a n d
 *
 *****************************************
 *
 * Functional description:
 *	Returns the result of a binary AND
 *	operation performed on the two numbers.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION bin_and
	INTEGER, INTEGER
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_bin_and' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	b i n _ o r
 *
 *****************************************
 *
 * Functional description:
 *	Returns the result of a binary OR
 *	operation performed on the two numbers.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION bin_or
	INTEGER, INTEGER
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_bin_or' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	b i n _ x o r
 *
 *****************************************
 *
 * Functional description:
 *	Returns the result of a binary XOR
 *	operation performed on the two numbers.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION bin_xor
	INTEGER, INTEGER
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_bin_xor' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	c e i l i n g
 *
 *****************************************
 *
 * Functional description:
 *	Returns a double value representing
 *	the smallest integer that is greater
 *	than or equal to the input value.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION ceiling
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_ceiling' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	c o s
 *
 *****************************************
 *
 * Functional description:
 *	The cos function returns the cosine
 *	of x. If x is greater than or equal
 *	to 263, or less than or equal to -263,
 *	a loss of significance in the result
 *	of a call to cos occurs, in which case
 *	the function generates a _TLOSS error
 *	and returns an indefinite (same as a
 *	quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION cos
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_cos' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	c o s h
 *
 *****************************************
 *
 * Functional description:
 *	The cosh function returns the hyperbolic cosine
 *	of x. If x is greater than or equal
 *	to 263, or less than or equal to -263,
 *	a loss of significance in the result
 *	of a call to cos occurs, in which case
 *	the function generates a _TLOSS error
 *	and returns an indefinite (same as a
 *	quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION cosh
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_cosh' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	c o t
 *
 *****************************************
 *
 * Functional description:
 *	Returns 1 over the tangent of the
 *	input parameter.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION cot
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_cot' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	d i v
 *
 *****************************************
 *
 * Functional description:
 *	Returns the quotient part of the division
 *	of the two input parameters.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION div
	INTEGER, INTEGER
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_div' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	f l o o r
 *
 *****************************************
 *
 * Functional description:
 * 	Returns a floating-point value
 * 	representing the largest integer that
 *	is less than or equal to x.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION floor
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_floor' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	u d f _ f r a c
 *
 *****************************************
 *
 * Functional description:
 * 	Returns a floating-point value
 * 	representing the fractional part of x.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION udf_frac
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_frac' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	l n
 *
 *****************************************
 *
 * Functional description:
 *	Returns the natural log of a number.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION ln
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_ln' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	l o g
 *
 *****************************************
 *
 * Functional description:
 *	log (x,y) returns the logarithm
 *	base x of y.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION log
	DOUBLE PRECISION, DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_log' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	l o g 1 0
 *
 *****************************************
 *
 * Functional description:
 *	Returns the logarithm base 10 of the
 *	input parameter.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION log10
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_log10' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	l o w e r
 *
 *****************************************
 *
 * Functional description:
 *	Returns the input string into lower
 *	case characters.  Note: This function
 *	will not work with international and
 *	non-ascii characters.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION lower
	CSTRING(255)
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_lower' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	l p a d
 *
 *****************************************
 *
 * Functional description:
 *	Appends the given character to beginning
 *	of the input string until length of the result
 *	string becomes equal to the given number.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION lpad
	CSTRING(255), INTEGER, CSTRING(1)
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_lpad' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	l t r i m
 *
 *****************************************
 *
 * Functional description:
 *	Removes leading spaces from the input
 *	string.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION ltrim
	CSTRING(255)
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_ltrim' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	m o d
 *
 *****************************************
 *
 * Functional description:
 *	Returns the remainder part of the
 *	division of the two input parameters.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION mod
	INTEGER, INTEGER
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_mod' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	p i
 *
 *****************************************
 *
 * Functional description:
 *	Returns the value of pi = 3.14159...
 *
 *****************************************
DECLARE EXTERNAL FUNCTION pi
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_pi' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	r a n d
 *
 *****************************************
 *
 * Functional description:
 *	Returns a random number between 0
 *	and 1.  Note the random number
 *	generator is seeded using the current
 *	time.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION rand
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_rand' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	r p a d
 *
 *****************************************
 *
 * Functional description:
 *	Appends the given character to end
 *	of the input string until length of the result
 *	string becomes equal to the given number.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION rpad
	CSTRING(255), INTEGER, CSTRING(1)
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_rpad' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	r t r i m
 *
 *****************************************
 *
 * Functional description:
 *	Removes trailing spaces from the input
 *	string.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION rtrim
	CSTRING(255)
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_rtrim' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	s i g n
 *
 *****************************************
 *
 * Functional description:
 *	Returns 1, 0, or -1 depending on whether
 * 	the input value is positive, zero or
 *	negative, respectively.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION sign
	DOUBLE PRECISION
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_sign' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	s i n
 *
 *****************************************
 *
 * Functional description:
 *	Returns the sine of x. If x is greater
 *	than or equal to 263, or less than or
 *	equal to -263, a loss of significance
 *	in the result occurs, in which case the
 *	function generates a _TLOSS error and
 *	returns an indefinite (same as a quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION sin
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_sin' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	s i n h
 *
 *****************************************
 *
 * Functional description:
 *	Returns the hyperbolic sine of x. If x is greater
 *	than or equal to 263, or less than or
 *	equal to -263, a loss of significance
 *	in the result occurs, in which case the
 *	function generates a _TLOSS error and
 *	returns an indefinite (same as a quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION sinh
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_sinh' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	s q r t
 *
 *****************************************
 *
 * Functional description:
 *	Returns the square root of a number.
 *
 *****************************************
DECLARE EXTERNAL FUNCTION sqrt
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_sqrt' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	s u b s t r
 *
 *****************************************
 *
 * Functional description:
 *	substr(s,m,n) returns the substring
 *	of s which starts at position m and
 *	ending at position n.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *      Change by Claudio Valderrama: when n>length(s),
 *      the result will be the original string instead
 *      of NULL as it was originally designed.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION substr
	CSTRING(255), SMALLINT, SMALLINT
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_substr' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	s u b s t r l e n
 *
 *****************************************
 *
 * Functional description:
 *	substrlen(s,i,l) returns the substring
 *	of s which starts at position i and
 *	ends at position i+l-1, being l the length.
 *	Note: This function is NOT limited to
 *	receiving and returning only 255 characters,
 *	rather, it can use as long as 32767
 * 	characters which is the limit on an
 *	INTERBASE character string.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION substrlen
	CSTRING(255), SMALLINT, SMALLINT
	RETURNS CSTRING(255) FREE_IT
	ENTRY_POINT 'IB_UDF_substrlen' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	s t r l e n
 *
 *****************************************
 *
 * Functional description:
 *	Returns the length of a given string.
 *
 *****************************************/
DECLARE EXTERNAL FUNCTION strlen
	CSTRING(32767)
	RETURNS INTEGER BY VALUE
	ENTRY_POINT 'IB_UDF_strlen' MODULE_NAME 'ib_udf';

/*****************************************
 *
 *	t a n
 *
 *****************************************
 *
 * Functional description:
 * 	Returns the tangent of x. If x is
 *	greater than or equal to 263, or less
 *	than or equal to -263, a loss of
 *	significance in the result occurs, in
 *	which case the function generates a
 *	_TLOSS error and returns an indefinite
 *	(same as a quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION tan
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_tan' MODULE_NAME 'ib_udf'; */

/*****************************************
 *
 *	t a n h
 *
 *****************************************
 *
 * Functional description:
 * 	Returns the tangent of x. If x is
 *	greater than or equal to 263, or less
 *	than or equal to -263, a loss of
 *	significance in the result occurs, in
 *	which case the function generates a
 *	_TLOSS error and returns an indefinite
 *	(same as a quiet NaN).
 *
 *****************************************
DECLARE EXTERNAL FUNCTION tanh
	DOUBLE PRECISION
	RETURNS DOUBLE PRECISION BY VALUE
	ENTRY_POINT 'IB_UDF_tanh' MODULE_NAME 'ib_udf'; */

