/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ini.h
 *	DESCRIPTION:	Declarations for metadata initialization
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

/* Note that this file is used by jrdmet.cpp in gpre
   as well as by ini.epp in JRD.  Make sure that any
   changes are compatible in both places. */

#include "../jrd/intlobj_new.h"
#include "../jrd/intl.h"
#include "../intl/country_codes.h"
#include "../intl/charsets.h"
#include "../jrd/obj.h"
#include "../jrd/dflt.h"
#include "../jrd/constants.h"

//******************************
// names.h
//******************************

/* Define name ids */

#define NAME(name, id) id,

enum name_ids { nam_MIN,
#include "../jrd/names.h"
nam_MAX};

#undef NAME

/* Define name strings */

#define NAME(name, id) name,

static const TEXT* const names[] =
{
	0,
#include "../jrd/names.h"
};
#undef NAME

//******************************
// fields.h
//******************************
const USHORT BLOB_SIZE		= 8;
const USHORT TIMESTAMP_SIZE	= 8;

/* Pick up global ids */


#define FIELD(type, name, dtype, length, sub_type, ods, dflt_blr)	type,
enum gflds {
#include "../jrd/fields.h"
gfld_MAX};
#undef FIELD

typedef gflds GFLDS;

/* Pick up actual global fields */

#ifndef GPRE
#define FIELD(type, name, dtype, length, sub_type, ods, dflt_blr)	\
	{ (int) type, (int) name, dtype, length, sub_type, ods, dflt_blr, sizeof(dflt_blr) },
#else
#define FIELD(type, name, dtype, length, sub_type, ods, dflt_blr)	\
	{ (int) type, (int) name, dtype, length, sub_type, ods, NULL, 0 },
#endif

struct gfld
{
	int gfld_type;
	int gfld_name;
	UCHAR gfld_dtype;
	USHORT gfld_length;
	UCHAR gfld_sub_type;	// mismatch; dsc2.h uses SSHORT.
	UCHAR gfld_minor;
	const UCHAR *gfld_dflt_blr;
	USHORT gfld_dflt_len;
};

static const struct gfld gfields[] = {
#include "../jrd/fields.h"
	{ 0, 0, dtype_unknown, 0, 0, 0, NULL, 0 }
};
#undef FIELD

//******************************
// relations.h
//******************************

/* Pick up relation ids */

#define RELATION(name, id, ods, type) id,
#define FIELD(symbol, name, id, update, ods, upd_id, upd_ods)
#define END_RELATION
enum rids {
#include "../jrd/relations.h"
rel_MAX};
#undef RELATION
#undef FIELD
#undef END_RELATION

typedef rids RIDS;

/* Pick up relations themselves */

#define RELATION(name, id, ods, type)	(int) name, (int) id, ods, type,
#define FIELD(symbol, name, id, update, ods, upd_id, upd_ods)\
				(int) name, (int) id, update, ods, (int) upd_id, upd_ods,
#define END_RELATION		0,

const int RFLD_R_NAME	= 0;
const int RFLD_R_ID		= 1;
const int RFLD_R_ODS	= 2;
const int RFLD_R_TYPE	= 3;
const int RFLD_RPT		= 4;

const int RFLD_F_NAME	= 0;
const int RFLD_F_ID		= 1;
const int RFLD_F_UPDATE	= 2;
const int RFLD_F_MINOR	= 3;
const int RFLD_F_UPD_ID	= 4;
const int RFLD_F_UPD_MINOR	= 5;
const int RFLD_F_LENGTH	= 6;

static const int relfields[] =
{
#include "../jrd/relations.h"
	0
};

#undef RELATION
#undef FIELD
#undef END_RELATION

//******************************
// types.h
//******************************

/* obtain field types */

struct rtyp {
	const TEXT* rtyp_name;
	SSHORT rtyp_value;
	int rtyp_field;
};

#define TYPE(text, type, field)	{ text, type, field },

static const rtyp types[] = {
#include "../jrd/types.h"
	{NULL, 0, 0}
};

#undef TYPE
