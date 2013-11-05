/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         quad.cpp
 *      DESCRIPTION:    Quad arithmetic simulation
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

#include "firebird.h"

using namespace Firebird;

SQUAD QUAD_add(const SQUAD*, const SQUAD*, ErrorFunction err)
{
/**************************************
 *
 *      Q U A D _ a d d
 *
 **************************************
 *
 * Functional description
 *      Add two quad numbers.
 *
 **************************************/

	err(Arg::Gds(isc_badblk));	/* not really badblk, but internal error */
/* IBERROR (224); *//* msg 224 quad word arithmetic not supported */

	SQUAD temp = { 0, 0 };
	return temp;				/* Added to remove compiler warnings */
}


SSHORT QUAD_compare(const SQUAD* arg1, const SQUAD* arg2)
{
/**************************************
 *
 *      Q U A D _ c o m p a r e
 *
 **************************************
 *
 * Functional description
 *      Compare two descriptors.  Return (-1, 0, 1) if a<b, a=b, or a>b.
 *
 **************************************/

	if (((SLONG *) arg1)[HIGH_WORD] > ((SLONG *) arg2)[HIGH_WORD])
		return 1;
	if (((SLONG *) arg1)[HIGH_WORD] < ((SLONG *) arg2)[HIGH_WORD])
		return -1;
	if (((ULONG *) arg1)[LOW_WORD] > ((ULONG *) arg2)[LOW_WORD])
		return 1;
	if (((ULONG *) arg1)[LOW_WORD] < ((ULONG *) arg2)[LOW_WORD])
		return -1;
	return 0;
}


SQUAD QUAD_from_double(const double*, ErrorFunction err)
{
/**************************************
 *
 *      Q U A D _ f r o m _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *      Convert a double to a quad.
 *
 **************************************/

	err(Arg::Gds(isc_badblk));	/* not really badblk, but internal error */
/* BUGCHECK (190); *//* msg 190 conversion not supported for */
	/* specified data types */

	SQUAD temp = { 0, 0 };

	return temp;				/* Added to remove compiler warnings */
}


SQUAD QUAD_multiply(const SQUAD*, const SQUAD*, ErrorFunction err)
{
/**************************************
 *
 *      Q U A D _ m u l t i p l y
 *
 **************************************
 *
 * Functional description
 *      Multiply two quad numbers.
 *
 **************************************/

	err(Arg::Gds(isc_badblk));	/* not really badblk, but internal error */
/* IBERROR (224); *//* msg 224 quad word arithmetic not supported */

	SQUAD temp = { 0, 0 };
	return temp;				/* Added to remove compiler warnings */
}


SQUAD QUAD_negate(const SQUAD*, ErrorFunction err)
{
/**************************************
 *
 *      Q U A D _ n e g a t e
 *
 **************************************
 *
 * Functional description
 *      Negate a quad number.
 *
 **************************************/

	err(Arg::Gds(isc_badblk));	/* not really badblk, but internal error */
/* IBERROR (224); *//* msg 224 quad word arithmetic not supported */

	SQUAD temp = { 0, 0 };
	return temp;				/* Added to remove compiler warnings */
}


SQUAD QUAD_subtract(const SQUAD*, const SQUAD*, ErrorFunction err)
{
/**************************************
 *
 *      Q U A D _ s u b t r a c t
 *
 **************************************
 *
 * Functional description
 *      Subtract two quad numbers.
 *
 **************************************/

	err(Arg::Gds(isc_badblk));	/* not really badblk, but internal error */
/* IBERROR (224); *//* msg 224 quad word arithmetic not supported */

	SQUAD temp = { 0, 0 };
	return temp;				/* Added to remove compiler warnings */
}


