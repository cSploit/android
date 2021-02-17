/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		command.cpp
 *	DESCRIPTION:	Interprete commands
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../qli/compile.h"
#include "../qli/exe.h"
//#include "../jrd/license.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/exe_proto.h"
#include "../qli/meta_proto.h"
#include "../qli/proc_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/utils_proto.h"

using MsgFormat::SafeArg;


static void dump_procedure(qli_dbb*, FILE*, const TEXT*, USHORT, FB_API_HANDLE);
static void extract_procedure(void*, const TEXT*, USHORT, qli_dbb*, ISC_QUAD&);

#ifdef NOT_USED_OR_REPLACED
static SCHAR db_items[] = { gds_info_page_size, gds_info_allocation, gds_info_end };
#endif

bool CMD_check_ready()
{
/**************************************
 *
 *	C M D _ c h e c k _ r e a d y
 *
 **************************************
 *
 * Functional description
 *	Make sure at least one database is ready.  If not, give a
 *	message.
 *
 **************************************/

	if (QLI_databases)
		return false;

	ERRQ_msg_put(95);	// Msg95 No databases are currently ready

	return true;
}


void CMD_copy_procedure( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ c o p y  _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Copy one procedure to another, possibly
 *	across databases
 *
 **************************************/
	qli_proc* old_proc = (qli_proc*) node->syn_arg[0];
	qli_proc* new_proc = (qli_proc*) node->syn_arg[1];

	PRO_copy_procedure(old_proc->qpr_database, old_proc->qpr_name->nam_string,
					   new_proc->qpr_database, new_proc->qpr_name->nam_string);
}


void CMD_define_procedure( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ d e f i n e  _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Define a procedure in the named database
 *	or in the most recently readied database.
 *
 **************************************/
	qli_proc* proc = (qli_proc*) node->syn_arg[0];

	if (!(proc->qpr_database))
		proc->qpr_database = QLI_databases;

	PRO_create(proc->qpr_database, proc->qpr_name->nam_string);
}


void CMD_delete_proc( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ d e l e t e _ p r o c
 *
 **************************************
 *
 * Functional description
 *	Delete a procedure in the named database
 *	or in the most recently readied database.
 *
 **************************************/
	qli_proc* proc = (qli_proc*) node->syn_arg[0];

	if (!proc->qpr_database)
		proc->qpr_database = QLI_databases;

	if (PRO_delete_procedure(proc->qpr_database, proc->qpr_name->nam_string))
		return;

	ERRQ_msg_put(88, SafeArg() << proc->qpr_name->nam_string <<	// Msg88 Procedure %s not found in database %s
				 proc->qpr_database->dbb_symbol->sym_string);
}


void CMD_edit_proc( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ e d i t _ p r o c
 *
 **************************************
 *
 * Functional description
 *	Edit a procedure in the specified database.
 *
 **************************************/
	qli_proc* proc = (qli_proc*) node->syn_arg[0];
	if (!proc->qpr_database)
		proc->qpr_database = QLI_databases;

	PRO_edit_procedure(proc->qpr_database, proc->qpr_name->nam_string);
}


void CMD_extract( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ e x t r a c t
 *
 **************************************
 *
 * Functional description
 *	Extract a series of procedures.
 *
 **************************************/
	FILE* file = (FILE*) EXEC_open_output((qli_nod*) node->syn_arg[1]);

	qli_syntax* list = node->syn_arg[0];
	if (list)
	{
		qli_syntax** ptr = list->syn_arg;
		for (const qli_syntax* const* const end = ptr + list->syn_count; ptr < end; ptr++)
		{
			qli_proc* proc = (qli_proc*) *ptr;
			qli_dbb* database = proc->qpr_database;
			if (!database)
				database = QLI_databases;

			const qli_name* name = proc->qpr_name;
			FB_API_HANDLE blob = PRO_fetch_procedure(database, name->nam_string);
			if (!blob)
			{
				ERRQ_msg_put(89, SafeArg() << name->nam_string << database->dbb_symbol->sym_string);
				// Msg89 Procedure %s not found in database %s
				continue;
			}
			dump_procedure(database, file, name->nam_string, name->nam_length, blob);
		}
	}
	else
	{
		CMD_check_ready();
		for (qli_dbb* database = QLI_databases; database; database = database->dbb_next)
		{
			PRO_scan(database, extract_procedure, file);
		}
	}

#ifdef WIN_NT
	if (((qli_nod*) node->syn_arg[1])->nod_arg[e_out_pipe])
		_pclose(file);
	else
#endif
		fclose(file);
}


void CMD_finish( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ f i n i s h
 *
 **************************************
 *
 * Functional description
 *	Perform FINISH.  Either finish listed databases or everything.
 *
 **************************************/
	if (node->syn_count == 0)
	{
		while (QLI_databases)
			MET_finish(QLI_databases);
		return;
	}

	for (USHORT i = 0; i < node->syn_count; i++)
		MET_finish((qli_dbb*) node->syn_arg[i]);
}


void CMD_rename_proc( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ r e n a m e _ p r o c
 *
 **************************************
 *
 * Functional description
 *	Rename a procedure in the named database,
 *	or the most recently readied database.
 *
 **************************************/
	qli_proc* old_proc = (qli_proc*) node->syn_arg[0];
	qli_proc* new_proc = (qli_proc*) node->syn_arg[1];

	qli_dbb* database = old_proc->qpr_database;
	if (!database)
		database = QLI_databases;

	if (new_proc->qpr_database && (new_proc->qpr_database != database))
		IBERROR(84);			// Msg84 Procedures can not be renamed across databases. Try COPY
	const qli_name* old_name = old_proc->qpr_name;
	const qli_name* new_name = new_proc->qpr_name;

	if (PRO_rename_procedure(database, old_name->nam_string, new_name->nam_string))
	{
		return;
	}

	ERRQ_error(85, SafeArg() << old_name->nam_string << database->dbb_symbol->sym_string);
	// Msg85 Procedure %s not found in database %s
}


void CMD_set( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ s e t
 *
 **************************************
 *
 * Functional description
 *	Set various options.
 *
 **************************************/
	USHORT length;
	const qli_const* string;

	const qli_syntax* const* ptr = node->syn_arg;

	for (USHORT i = 0; i < node->syn_count; i++)
	{
		const USHORT foo = (USHORT)(IPTR) *ptr++;
		const enum set_t sw = (enum set_t) foo;
		const qli_syntax* value = *ptr++;
		switch (sw)
		{
		case set_blr:
			QLI_blr = (bool)(IPTR) value;
			break;

		case set_statistics:
			QLI_statistics = (bool)(IPTR) value;
			break;

		case set_columns:
			QLI_name_columns = QLI_columns = (USHORT)(IPTR) value;
			break;

		case set_lines:
			QLI_lines = (USHORT)(IPTR) value;
			break;

		case set_semi:
			QLI_semi = (bool)(IPTR) value;
			break;

		case set_echo:
			QLI_echo = (bool)(IPTR) value;
			break;

		//case set_form:
		//	IBERROR(484);		// FORMs not supported
		//	break;

		case set_password:
			string = (qli_const*) value;
			length = MIN(string->con_desc.dsc_length + 1u, sizeof(QLI_default_password));
			fb_utils::copy_terminate(QLI_default_password, (char*) string->con_data, length);
			break;

		case set_prompt:
			string = (qli_const*) value;
			if (string->con_desc.dsc_length >= sizeof(QLI_prompt_string))
				ERRQ_error(86);	// Msg86 substitute prompt string too long
			fb_utils::copy_terminate(QLI_prompt_string, (char*) string->con_data, string->con_desc.dsc_length + 1);
			break;

		case set_continuation:
			string = (qli_const*) value;
			if (string->con_desc.dsc_length >= sizeof(QLI_cont_string))
				ERRQ_error(87);	// Msg87 substitute prompt string too long
			fb_utils::copy_terminate(QLI_cont_string, (char*) string->con_data, string->con_desc.dsc_length + 1);
			break;

		case set_matching_language:
			if (QLI_matching_language)
				ALLQ_release((qli_frb*) QLI_matching_language);
			string = (qli_const*) value;
			if (!string)
				QLI_matching_language = NULL;
			else
			{
				const USHORT len = string->con_desc.dsc_length;
				QLI_matching_language = (qli_const*) ALLOCPV(type_con, len);
				fb_utils::copy_terminate((char*) QLI_matching_language->con_data,
										 (char*) string->con_data, len + 1);
				dsc& lang = QLI_matching_language->con_desc;
				lang.dsc_dtype = dtype_text;
				lang.dsc_address = QLI_matching_language->con_data;
				lang.dsc_length = len;
			}
			break;

		case set_user:
			string = (qli_const*) value;
			length = MIN(string->con_desc.dsc_length + 1u, sizeof(QLI_default_user));
			fb_utils::copy_terminate(QLI_default_user, (char*) string->con_data, length);
			break;

			break;

		case set_count:
			QLI_count = (USHORT)(IPTR) value;
			break;

		case set_charset:
			{
				if (!value)
				{
					QLI_charset[0] = 0;
					break;
				}
				const TEXT* name = ((qli_name*) value)->nam_string;
				fb_utils::copy_terminate(QLI_charset, name, sizeof(QLI_charset));
				break;
			}

#ifdef DEV_BUILD
		case set_explain:
			QLI_explain = (bool)(IPTR) value;
			break;

		case set_hex_output:
			QLI_hex_output = (bool)(IPTR) value;
			break;
#endif

		default:
			ERRQ_bugcheck(6);		// Msg6 set option not implemented
		}
	}
}


void CMD_shell( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ s h e l l
 *
 **************************************
 *
 * Functional description
 *	Invoke operating system shell.
 *
 **************************************/
	TEXT buffer[256];

	// Copy command, inserting extra blank at end.

	TEXT* p = buffer;
	const qli_const* constant = (qli_const*) node->syn_arg[0];
	if (constant)
	{
		const USHORT l = constant->con_desc.dsc_length;
		if (l)
			memcpy(p, constant->con_data, l);

		p += l;
		*p++ = ' ';
		*p = 0;
	}
	else
	{
#ifndef WIN_NT
		strcpy(buffer, "$SHELL");
#else
		strcpy(buffer, "%ComSpec%");
#endif
	}

	FB_UNUSED(system(buffer));
}


void CMD_transaction( qli_syntax* node)
{
/**************************************
 *
 *	C M D _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Perform COMMIT, ROLLBACK or PREPARE
 *      on listed databases or everything.
 *
 **************************************/

	// If there aren't any open databases then obviously
	// there isn't anything to commit.

	if (node->syn_count == 0 && !QLI_databases)
		return;

	if (node->syn_type == nod_commit)
	{
		if ((node->syn_count > 1) || (node->syn_count == 0 && QLI_databases->dbb_next))
		{
			node->syn_type = nod_prepare;
			CMD_transaction(node);
			node->syn_type = nod_commit;
		}
		else if (node->syn_count == 1)
		{
			qli_dbb* tmp_db = (qli_dbb*) node->syn_arg[0];
			tmp_db->dbb_flags |= DBB_prepared;
		}
		else
			QLI_databases->dbb_flags |= DBB_prepared;
	}


	if (node->syn_count == 0)
	{
		for (qli_dbb* db_iter = QLI_databases; db_iter; db_iter = db_iter->dbb_next)
		{
			if ((node->syn_type == nod_commit) && !(db_iter->dbb_flags & DBB_prepared))
			{
				ERRQ_msg_put(465, db_iter->dbb_symbol->sym_string);
			}
			else if (node->syn_type == nod_prepare)
				db_iter->dbb_flags |= DBB_prepared;
			if (db_iter->dbb_transaction)
				MET_transaction(node->syn_type, db_iter);
			if (db_iter->dbb_meta_trans)
				MET_meta_commit(db_iter);
			if (db_iter->dbb_proc_trans)
				PRO_commit(db_iter);
		}
		return;
	}

	qli_syntax** ptr = node->syn_arg;
	for (const qli_syntax* const* const end = ptr + node->syn_count; ptr < end; ptr++)
	{
		qli_dbb* database = (qli_dbb*) *ptr;
		if ((node->syn_type == nod_commit) && !(database->dbb_flags & DBB_prepared))
		{
				ERRQ_msg_put(465, database->dbb_symbol->sym_string);
		}
		else if (node->syn_type == nod_prepare)
			database->dbb_flags |= DBB_prepared;
		if (database->dbb_transaction)
			MET_transaction(node->syn_type, database);
	}
}


static void dump_procedure(qli_dbb* database,
						   FILE* file,
						   const TEXT* name, USHORT length, FB_API_HANDLE blob)
{
/**************************************
 *
 *	d u m p _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Extract a procedure from a database.
 *
 **************************************/
	TEXT buffer[256];

	fprintf(file, "DELETE PROCEDURE %.*s;\n", length, name);
	fprintf(file, "DEFINE PROCEDURE %.*s\n", length, name);

	while (PRO_get_line(blob, buffer, sizeof(buffer)))
		fputs(buffer, file);

	PRO_close(database, blob);
	fprintf(file, "END_PROCEDURE\n\n");
}


static void extract_procedure(void* file,
							  const TEXT* name,
							  USHORT length, qli_dbb* database, ISC_QUAD& blob_id)
{
/**************************************
 *
 *	e x t r a c t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Extract a procedure from a database.
 *
 **************************************/
	FB_API_HANDLE blob = PRO_open_blob(database, blob_id);
	dump_procedure(database, static_cast<FILE*>(file), name, length, blob);
}


