/*
 * tab=4
 *____________________________________________________________
 *
 *		PROGRAM:	C preprocessor
 *		MODULE:		gpre_meta_boot.cpp
 *		DESCRIPTION:	Meta data interface to system
 *
 *  The contents of this file are subject to the Interbase Public
 *  License Version 1.0 (the "License"); you may not use this file
 *  except in compliance with the License. You may obtain a copy
 *  of the License at http://www.Inprise.com/IPL.html
 *
 *  Software distributed under the License is distributed on an
 *  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 *  or implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  The Original Code was created by Inprise Corporation
 *  and its predecessors. Portions created by Inprise Corporation are
 *  Copyright (C) Inprise Corporation.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 *____________________________________________________________
 *
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/ibase.h"
#include "../gpre/gpre.h"
//#include "../jrd/license.h"
#include "../jrd/intl.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/jrdme_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "../gpre/par_proto.h"
#include "../yvalve/gds_proto.h"

static const UCHAR blr_bpb[] =
{
	isc_bpb_version1,
	isc_bpb_source_type, 1, isc_blob_blr,
	isc_bpb_target_type, 1, isc_blob_blr
};

/*____________________________________________________________
 *
 *		Lookup a field by name in a context.
 *		If found, return field block.  If not, return NULL.
 */

gpre_fld* MET_context_field( gpre_ctx* context, const char* string)
{
	SCHAR name[NAME_SIZE];

	if (context->ctx_relation) {
		return (MET_field(context->ctx_relation, string));
	}

	if (!context->ctx_procedure) {
		return NULL;
	}

	strcpy(name, string);
	gpre_prc* procedure = context->ctx_procedure;

	// At this point the procedure should have been scanned, so all
	// its fields are in the symbol table.

	gpre_fld* field = NULL;
	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_field && (field = (gpre_fld*) symbol->sym_object) &&
			field->fld_procedure == procedure)
		{
			return field;
		}
	}

	return field;
}


/*____________________________________________________________
 *
 *		Initialize meta data access to database.  If the
 *		database can't be opened, return false.
 */

bool MET_database(gpre_dbb* db, bool /*print_version*/)
{
	// Each info item requested will return
	//
	//     1 byte for the info item tag
	//     2 bytes for the length of the information that follows
	//     1 to 4 bytes of integer information
	//
	// isc_info_end will not have a 2-byte length - which gives us
	// some padding in the buffer.

	JRDMET_init(db);
	return true;
}


/*____________________________________________________________
 *
 *		Lookup a domain by name.
 *		Initialize the size of the field.
 */

bool MET_domain_lookup(gpre_req* request, gpre_fld* field, const char* string)
{
	SCHAR name[NAME_SIZE];

	strcpy(name, string);

	// Lookup domain.  If we find it in the hash table, and it is not the
	// field we a currently looking at, use it.  Else look it up from the
	// database.

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		gpre_fld* d_field;
		if ((symbol->sym_type == SYM_field) &&
			((d_field = (gpre_fld*) symbol->sym_object) && (d_field != field)))
		{
			field->fld_length = d_field->fld_length;
			field->fld_scale = d_field->fld_scale;
			field->fld_sub_type = d_field->fld_sub_type;
			field->fld_dtype = d_field->fld_dtype;
			field->fld_ttype = d_field->fld_ttype;
			field->fld_charset_id = d_field->fld_charset_id;
			field->fld_collate_id = d_field->fld_collate_id;
			field->fld_char_length = d_field->fld_char_length;
			return true;
		}
	}

	if (!request)
		return false;

	fb_assert(0);
	return false;
}


/*____________________________________________________________
 *
 *		Gets the default value for a domain of an existing table
 */

bool MET_get_domain_default(gpre_dbb* /*db*/, const TEXT* /*domain_name*/, TEXT* /*buffer*/,
	USHORT /*buff_length*/)
{
	fb_assert(0);
	return false;
}


/*____________________________________________________________
 *
 *		Gets the default value for a column of an existing table.
 *		Will check the default for the column of the table, if that is
 *		not present, will check for the default of the relevant domain
 *
 *		The default blr is returned in buffer. The blr is of the form
 *		blr_version4 blr_literal ..... blr_eoc
 *
 *		Reads the system tables RDB$FIELDS and RDB$RELATION_FIELDS.
 */

bool MET_get_column_default(const gpre_rel*, //relation,
							const TEXT*, // column_name,
							TEXT*, // buffer,
							USHORT) // buff_length)
{
	fb_assert(0);
	return false;
}


/*____________________________________________________________
 *
 *		Lookup the fields for the primary key
 *		index on a relation, returning a list
 *		of the fields.
 */

gpre_lls* MET_get_primary_key(gpre_dbb* db, const TEXT* relation_name)
{
	SCHAR name[NAME_SIZE];

	strcpy(name, relation_name);

	if (db == NULL)
		return NULL;

	if ((db->dbb_handle == 0) && !MET_database(db, false))
		CPR_exit(FINI_ERROR);

	fb_assert(db->dbb_transaction == 0);
	fb_assert(0);
	return 0;
}


/*____________________________________________________________
 *
 *		Lookup a field by name in a relation.
 *		If found, return field block.  If not, return NULL.
 */

gpre_fld* MET_field(gpre_rel* relation, const char* string)
{
	SCHAR name[NAME_SIZE];

	strcpy(name, string);
	//const SSHORT length = strlen(name);

	// Lookup field.  If we find it, nifty.  If not, look it up in the
	// database.

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_keyword && symbol->sym_keyword == (int) KW_DBKEY)
		{
			return relation->rel_dbkey;
		}

		gpre_fld* field;
		if (symbol->sym_type == SYM_field && (field = (gpre_fld*) symbol->sym_object) &&
			field->fld_relation == relation)
		{
			return field;
		}
	}

	return NULL;
}


/*____________________________________________________________
 *
 *     Return a list of the fields in a relation
 */

gpre_nod* MET_fields(gpre_ctx* context)
{
	gpre_fld* field;

	gpre_prc* procedure = context->ctx_procedure;
	if (procedure)
	{
		gpre_nod* node = MSC_node(nod_list, procedure->prc_out_count);
		//int count = 0;
		for (field = procedure->prc_outputs; field; field = field->fld_next)
		{
			ref* reference = (ref*) MSC_alloc(REF_LEN);
			reference->ref_field = field;
			reference->ref_context = context;
			gpre_nod* field_node = MSC_unary(nod_field, (gpre_nod*) reference);
			node->nod_arg[field->fld_position] = field_node;
			//count++;
		}
		return node;
	}

	gpre_rel* relation = context->ctx_relation;
	if (relation->rel_meta)
	{
		int count = 0;
		for (field = relation->rel_fields; field; field = field->fld_next) {
			count++;
		}
		gpre_nod* node = MSC_node(nod_list, count);
		//count = 0;
		for (field = relation->rel_fields; field; field = field->fld_next)
		{
			ref* reference = (ref*) MSC_alloc(REF_LEN);
			reference->ref_field = field;
			reference->ref_context = context;
			gpre_nod* field_node = MSC_unary(nod_field, (gpre_nod*) reference);
			node->nod_arg[field->fld_position] = field_node;
			//count++;
		}
		return node;
	}

	return NULL;
}


/*____________________________________________________________
 *
 *		Shutdown all attached databases.
 */

void MET_fini( gpre_dbb*)
{
	return;
}


/*____________________________________________________________
 *
 *		Lookup a generator by name.
 *		If found, return string. If not, return NULL.
 */

const SCHAR* MET_generator(const TEXT* string, const gpre_dbb* db)
{
	SCHAR name[NAME_SIZE];

	strcpy(name, string);

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if ((symbol->sym_type == SYM_generator) && (db == (gpre_dbb*) symbol->sym_object))
		{
			return symbol->sym_string;
		}

	return NULL;
}


/*____________________________________________________________
 *
 *		Compute internal datatype and length based on system relation field values.
 */

USHORT MET_get_dtype(USHORT blr_dtype, USHORT sub_type, USHORT* length)
{
	USHORT dtype;

	USHORT l = *length;

	switch (blr_dtype)
	{
	case blr_varying:
	case blr_text:
		dtype = dtype_text;
		if (gpreGlob.sw_cstring && sub_type != dsc_text_type_fixed)
		{
			++l;
			dtype = dtype_cstring;
		}
		break;

	case blr_cstring:
		dtype = dtype_cstring;
		++l;
		break;

	case blr_short:
		dtype = dtype_short;
		l = sizeof(SSHORT);
		break;

	case blr_long:
		dtype = dtype_long;
		l = sizeof(SLONG);
		break;

	case blr_quad:
		dtype = dtype_quad;
		l = sizeof(ISC_QUAD);
		break;

	case blr_float:
		dtype = dtype_real;
		l = sizeof(float);
		break;

	case blr_double:
		dtype = dtype_double;
		l = sizeof(double);
		break;

	case blr_blob:
		dtype = dtype_blob;
		l = sizeof(ISC_QUAD);
		break;

	// Begin sql date/time/timestamp
	case blr_sql_date:
		dtype = dtype_sql_date;
		l = sizeof(ISC_DATE);
		break;

	case blr_sql_time:
		dtype = dtype_sql_time;
		l = sizeof(ISC_TIME);
		break;

	case blr_timestamp:
		dtype = dtype_timestamp;
		l = sizeof(ISC_TIMESTAMP);
		break;
	// End sql date/time/timestamp

	case blr_int64:
		dtype = dtype_int64;
		l = sizeof(ISC_INT64);
		break;

	default:
		CPR_error("datatype not supported");
		return 0; // silence non initialized warning
	}

	*length = l;

	return dtype;
}


/*____________________________________________________________
 *
 *		Lookup a procedure (represented by a token) in a database.
 *		Return a procedure block (if name is found) or NULL.
 *
 *		This function has been cloned into MET_get_udf
 */

gpre_prc* MET_get_procedure(gpre_dbb* db, const TEXT* string, const TEXT* owner_name)
{
	SCHAR name[NAME_SIZE], owner[NAME_SIZE];

	strcpy(name, string);
	strcpy(owner, owner_name);
	gpre_prc* procedure = NULL;

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_procedure &&
			(procedure = (gpre_prc*) symbol->sym_object) && procedure->prc_database == db &&
			(!owner[0] || (procedure->prc_owner && !strcmp(owner, procedure->prc_owner->sym_string))))
		{
			break;
		}
	}

	if (!procedure)
		return NULL;

	if (procedure->prc_flags & PRC_scanned)
		return procedure;

	fb_assert(0);
	return NULL;
}


/*____________________________________________________________
 *
 *		Lookup a relation (represented by a token) in a database.
 *		Return a relation block (if name is found) or NULL.
 */

gpre_rel* MET_get_relation(gpre_dbb* db, const TEXT* string, const TEXT* owner_name)
{
	gpre_rel* relation;
	SCHAR name[NAME_SIZE], owner[NAME_SIZE];

	strcpy(name, string);
	strcpy(owner, owner_name);

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_relation &&
			(relation = (gpre_rel*) symbol->sym_object) && relation->rel_database == db &&
			(!owner[0] || (relation->rel_owner && !strcmp(owner, relation->rel_owner->sym_string))))
		{
			return relation;
		}
	}

	return NULL;
}


/*____________________________________________________________
 *
 */

intlsym* MET_get_text_subtype(SSHORT ttype)
{
	for (intlsym* p = gpreGlob.text_subtypes; p; p = p->intlsym_next)
	{
		if (p->intlsym_ttype == ttype)
			return p;
	}

	return NULL;
}


/*____________________________________________________________
 *
 *		Lookup a udf (represented by a token) in a database.
 *		Return a udf block (if name is found) or NULL.
 *
 *		This function was cloned from MET_get_procedure
 */

udf* MET_get_udf(gpre_dbb* db, const TEXT* string)
{
	SCHAR name[NAME_SIZE];

	strcpy(name, string);
	udf* udf_val = NULL;
	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_udf &&
			(udf_val = (udf*) symbol->sym_object) && udf_val->udf_database == db)
		{
			break;
		}
	}

	if (!udf_val)
		return NULL;

	fb_assert(0);
	return NULL;
}


/*____________________________________________________________
 *
 *		Return relation if the passed view_name represents a
 *		view with the passed relation as a base table
 *		(the relation could be an alias).
 */

gpre_rel* MET_get_view_relation(gpre_req* /*request*/,
								const char* /*view_name*/,
								const char* /*relation_or_alias*/, USHORT /*level*/)
{
	fb_assert(0);
	return NULL;
}


/*____________________________________________________________
 *
 *		Lookup an index for a database.
 *		Return an index block (if name is found) or NULL.
 */

gpre_index* MET_index(gpre_dbb* db, const TEXT* string)
{
	gpre_index* index;
	SCHAR name[NAME_SIZE];

	strcpy(name, string);
	//const SSHORT length = strlen(name);

	for (gpre_sym* symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_index &&
			(index = (gpre_index*) symbol->sym_object) && index->ind_relation->rel_database == db)
		{
			return index;
		}
	}

	return NULL;
}


/*____________________________________________________________
 *
 *		Load all of the relation names
 *       and user defined function names
 *       into the symbol (hash) table.
 */

void MET_load_hash_table( gpre_dbb*)
{
	// If this is an internal ISC access method invocation, don't do any of this stuff

	return;
}


/*____________________________________________________________
 *
 *		Make a field symbol.
 */

gpre_fld* MET_make_field(const SCHAR* name,
						SSHORT dtype,
						SSHORT length,
						bool insert_flag)
{
	gpre_fld* field = (gpre_fld*) MSC_alloc(FLD_LEN);
	field->fld_length = length;
	field->fld_dtype = dtype;
	gpre_sym* symbol = MSC_symbol(SYM_field, name, strlen(name), (gpre_ctx*) field);
	field->fld_symbol = symbol;

	if (insert_flag)
		HSH_insert(symbol);

	return field;
}


/*____________________________________________________________
 *
 *		Make an index symbol.
 */

gpre_index* MET_make_index(const SCHAR* name)
{
	gpre_index* index = (gpre_index*) MSC_alloc(IND_LEN);
	index->ind_symbol = MSC_symbol(SYM_index, name, strlen(name), (gpre_ctx*) index);

	return index;
}


/*____________________________________________________________
 *
 *		Make an relation symbol.
 */

gpre_rel* MET_make_relation(const SCHAR* name)
{
	gpre_rel* relation = (gpre_rel*) MSC_alloc(REL_LEN);
	relation->rel_symbol = MSC_symbol(SYM_relation, name, strlen(name), (gpre_ctx*) relation);

	return relation;
}


/*____________________________________________________________
 *
 *		Lookup a type name for a field.
 */

bool MET_type(gpre_fld* field, const TEXT* string, SSHORT* ptr)
{
	field_type* type;

	for (gpre_sym* symbol = HSH_lookup(string); symbol; symbol = symbol->sym_homonym)
	{
		if (symbol->sym_type == SYM_type && (type = (field_type*) symbol->sym_object) &&
			(!type->typ_field || type->typ_field == field))
		{
			*ptr = type->typ_value;
			return true;
		}
	}

	fb_assert(0);
	return false;
}


/*____________________________________________________________
 *
 *		Lookup an index for a database.
 *
 *  Return: true if the trigger exists
 *		   false otherwise
 */

bool MET_trigger_exists(gpre_dbb* /*db*/, const TEXT* /*trigger_name*/)
{
	fb_assert(0);
	return false;
}

#include "firebird/Interface.h"

using namespace Firebird;

class DummyMasterImpl : public IMaster
{
public:
	// IMaster implementation (almost dummy, for boot build)
	virtual int FB_CARG getVersion()
	{
		return FB_MASTER_VERSION;
	}

	virtual IPluginModule* FB_CARG getModule()
	{
		return NULL;
	}

	virtual IStatus* FB_CARG getStatus()
	{
		fb_assert(false);
		return NULL;
	}

	virtual IProvider* FB_CARG getDispatcher()
	{
		fb_assert(false);
		return NULL;
	}

	virtual IPluginManager* FB_CARG getPluginManager()
	{
		//fb_assert(false);
		return NULL;
	}

	virtual int FB_CARG upgradeInterface(IVersioned* /*toUpgrade*/, int /*desiredVersion*/,
										 struct UpgradeInfo* /*upInfo*/)
	{
		fb_assert(false);
		return 0;
	}

	virtual const char* FB_CARG circularAlloc(const char* s, size_t len, intptr_t /*thr*/)
	{
		char* buf = (char*) malloc(len + 1);
		memcpy(buf, s, len);
		buf[len] = 0;
		return buf;
	}

	virtual ITimerControl* FB_CARG getTimerControl()
	{
		fb_assert(false);
		return NULL;
	}

	virtual IAttachment* FB_CARG registerAttachment(IProvider* /*provider*/, IAttachment* /*attachment*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual ITransaction* FB_CARG registerTransaction(IAttachment* /*attachment*/, ITransaction* /*transaction*/)
	{
		fb_assert(false);
		return NULL;
	}

	virtual IDtc* FB_CARG getDtc()
	{
		fb_assert(false);
		return NULL;
	}

	virtual int FB_CARG same(IVersioned* /*first*/, IVersioned* /*second*/)
	{
		return 0;
	}

	virtual IMetadataBuilder* FB_CARG getMetadataBuilder(IStatus* status, unsigned fieldCount)
	{
		fb_assert(false);
		return NULL;
	}

	virtual IDebug* FB_CARG getDebug()
	{
		return NULL;
	}

	virtual int FB_CARG serverMode(int mode)
	{
		return -1;
	}

	virtual IUtl* FB_CARG getUtlInterface()
	{
		fb_assert(false);
		return NULL;
	}

	virtual IConfigManager* FB_CARG getConfigManager()
	{
		fb_assert(false);
		return NULL;
	}
};


extern "C" {

void ERR_bugcheck(int)
{
}

void ERR_post(ISC_STATUS, ...)
{
}

Firebird::IMaster* API_ROUTINE fb_get_master_interface()
{
	static DummyMasterImpl dummyMaster;
	return &dummyMaster;
}

} // extern "C"
