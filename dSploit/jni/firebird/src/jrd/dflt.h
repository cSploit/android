/*
 *      PROGRAM:        JRD Access Method
 *      MODULE:         dflt.h
 *      DESCRIPTION:    Default values for fields in system relations.
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

#ifndef JRD_DFLT_H
#define JRD_DFLT_H

/* This file contains the blr for the default values for fields
   in system relations.  The GDEF source for these fields is in
   DFLT.GDL in the JRD component.  When modifying a system field default value,
   check out DFLT.GDL, modify the field definition source, to generate the
   proper blr, replace the blr in DFLT.H, and check both files back in.
   PLEASE NOTE: Since default values cannot be specified in GDML, the
   MISSING_VALUE clause is used to generate the BLR for DEFAULT value.  */

/* default value of "NO" for RDB$DEFERRABLE, RDB$INITIALLY_DEFERRED in
    RDB$RELATION_CONSTRAINTS.                                           */

/* transform int's into network byte format */
#define TWOBYTES(x)	((x) % 256), ((x) / 256)

static const UCHAR dflt_no[] =
{
	blr_version5,
	blr_literal, blr_text2, TWOBYTES(ttype_ascii), 2, 0, 'N', 'O',
	blr_eoc
};

/* default value of "FULL" for RDB$MATCH_OPTION in RDB$REF_CONSTRAINTS   */

static const UCHAR dflt_full[] =
{
	blr_version5,
	blr_literal, blr_text2, TWOBYTES(ttype_ascii), 4, 0, 'F', 'U', 'L', 'L',
	blr_eoc
};

/* default value of "RESTRICT" for  RDB$UPDATE_RULE, RDB$DELETE_RULE in
   RDB$REF_CONSTRAINTS                                                  */

static const UCHAR dflt_restrict[] =
{
	blr_version5,
	blr_literal, blr_text2, TWOBYTES(ttype_ascii), 8, 0, 'R', 'E', 'S', 'T',
		'R', 'I', 'C', 'T',
	blr_eoc
};

#endif // JRD_DFLT_H

