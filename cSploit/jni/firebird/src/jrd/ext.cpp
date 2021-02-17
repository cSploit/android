/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ext.cpp
 *	DESCRIPTION:	External file access
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
 *
 * 26-Sept-2001 Paul Beach - Windows External File Directory Config. Parameter
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, second attempt
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "../jrd/rse.h"
#include "../jrd/ext.h"
#include "../jrd/tra.h"
#include "../dsql/ExprNodes.h"
#include "gen/iberror.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/ext_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/config/config.h"
#include "../common/config/dir_list.h"
#include "../common/os/path_utils.h"
#include "../common/classes/init.h"
#include "../common/isc_f_proto.h"

#if defined _MSC_VER && _MSC_VER < 1400
// NS: in VS2003 these only work with static CRT
extern "C" {
int __cdecl _fseeki64(FILE*, __int64, int);
__int64 __cdecl _ftelli64(FILE*);
}
#endif

#ifdef WIN_NT
#define FTELL64 _ftelli64
#define FSEEK64 _fseeki64
#else
#define FTELL64 ftello
#define FSEEK64 fseeko
#endif

using namespace Firebird;

namespace Jrd
{
	class ExternalFileDirectoryList : public DirectoryList
	{
	private:
		const PathName getConfigString() const
		{
			return PathName(config->getExternalFileAccess());
		}

	public:
		explicit ExternalFileDirectoryList(const Database* dbb)
			: DirectoryList(*dbb->dbb_permanent), config(dbb->dbb_config)
		{
			initialize();
		}

		static void create(Database* dbb)
		{
			if (!dbb->dbb_external_file_directory_list)
			{
				dbb->dbb_external_file_directory_list =
					FB_NEW(*dbb->dbb_permanent) ExternalFileDirectoryList(dbb);
			}
		}

	private:
		RefPtr<Config> config;
	};
}

using namespace Jrd;

namespace {

#ifdef WIN_NT
	static const char* const FOPEN_TYPE			= "a+b";
#else
	static const char* const FOPEN_TYPE			= "a+";
#endif
	static const char* const FOPEN_READ_ONLY	= "rb";

	FILE *ext_fopen(Database* dbb, ExternalFile* ext_file)
	{
		const char* file_name = ext_file->ext_filename;

		ExternalFileDirectoryList::create(dbb);
		if (!dbb->dbb_external_file_directory_list->isPathInList(file_name))
		{
			ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("external file") <<
														 Arg::Str(file_name));
		}

		// If the database is updateable, then try opening the external files in
		// RW mode. If the DB is ReadOnly, then open the external files only in
		// ReadOnly mode, thus being consistent.
		if (!dbb->readOnly())
			ext_file->ext_ifi = fopen(file_name, FOPEN_TYPE);

		if (!ext_file->ext_ifi)
		{
			// could not open the file as read write attempt as read only
			if (!(ext_file->ext_ifi = fopen(file_name, FOPEN_READ_ONLY)))
			{
				ERR_post(Arg::Gds(isc_io_error) << Arg::Str("fopen") << Arg::Str(file_name) <<
						 Arg::Gds(isc_io_open_err) << SYS_ERR(errno));
			}
			else {
				ext_file->ext_flags |= EXT_readonly;
			}
		}

		return ext_file->ext_ifi;
	}
} // namespace


double EXT_cardinality(thread_db* tdbb, jrd_rel* relation)
{
/**************************************
 *
 *	E X T _ c a r d i n a l i t y
 *
 **************************************
 *
 * Functional description
 *	Return cardinality for the external file.
 *
 **************************************/
	ExternalFile* const file = relation->rel_file;
	fb_assert(file);

	try
	{
		bool must_close = false;
		if (!file->ext_ifi)
		{
			ext_fopen(tdbb->getDatabase(), file);
			must_close = true;
		}

		FB_UINT64 file_size = 0;

#ifdef WIN_NT
		struct __stat64 statistics;
		if (!_fstat64(_fileno(file->ext_ifi), &statistics))
#else
		struct stat statistics;
		if (!fstat(fileno(file->ext_ifi), &statistics))
#endif
		{
			file_size = statistics.st_size;
		}

		if (must_close)
		{
			fclose(file->ext_ifi);
			file->ext_ifi = NULL;
		}

		const Format* const format = MET_current(tdbb, relation);
		fb_assert(format && format->fmt_length);
		const USHORT offset = (USHORT)(IPTR) format->fmt_desc[0].dsc_address;
		const ULONG record_length = format->fmt_length - offset;

		return (double) file_size / record_length;
	}
	catch (const Exception&)
	{
		fb_utils::init_status(tdbb->tdbb_status_vector);
	}

	return 10000; // just a wild guess
}


void EXT_erase(record_param*, jrd_tra*)
{
/**************************************
 *
 *	E X T _ e r a s e
 *
 **************************************
 *
 * Functional description
 *	Update an external file.
 *
 **************************************/

	ERR_post(Arg::Gds(isc_ext_file_delete));
}


// Third param is unused.
ExternalFile* EXT_file(jrd_rel* relation, const TEXT* file_name) //, bid* description)
{
/**************************************
 *
 *	E X T _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Create a file block for external file access.
 *
 **************************************/
	Database* dbb = GET_DBB();
	CHECK_DBB(dbb);

	// if we already have a external file associated with this relation just
	// return the file structure
	if (relation->rel_file) {
		EXT_fini(relation, false);
	}

#ifdef WIN_NT
	// Default number of file handles stdio.h on Windows is 512, use this
	// call to increase and set to the maximum
	_setmaxstdio(2048);
#endif

	// If file_name is relative expand it in ExternalFilesPath.
	PathName newName, name(file_name);
	if (PathUtils::isRelative(name))
	{
		ExternalFileDirectoryList::create(dbb);
		if (!(dbb->dbb_external_file_directory_list->expandFileName(newName, name)))
		{
			if (!dbb->dbb_external_file_directory_list->defaultName(newName, name))
			{
				ISC_expand_filename(newName, false);
			}
		}
		file_name = newName.c_str();
		name = newName;
	}

	// Create missing path components
	ObjectsArray<PathName> paths;

	for (;;)
	{
		PathName path, file;
		PathUtils::splitLastComponent(path, file, name);
		if (path.isEmpty())
			break;

		int rc = PathUtils::makeDir(path.c_str());
		if (rc == 0 || rc == EEXIST)
			break;
		paths.push(path);
		name = path;
	}

	while (paths.hasData())
	{
		PathName path(paths.pop());
		if (PathUtils::makeDir(path.c_str()) != 0)
			break;
	}

	paths.clear();

	ExternalFile* file = FB_NEW_RPT(*relation->rel_pool, (strlen(file_name) + 1)) ExternalFile();
	relation->rel_file = file;
	strcpy(file->ext_filename, file_name);
	file->ext_flags = 0;
	file->ext_ifi = NULL;

	return file;
}


void EXT_fini(jrd_rel* relation, bool close_only)
{
/**************************************
 *
 *	E X T _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Close the file associated with a relation.
 *
 **************************************/
	if (relation->rel_file)
	{
		ExternalFile* file = relation->rel_file;
		if (file->ext_ifi)
		{
			fclose(file->ext_ifi);
			file->ext_ifi = NULL;
		}

		// before zeroing out the rel_file we need to deallocate the memory
		if (!close_only)
		{
			delete file;
			relation->rel_file = NULL;
		}
	}
}


bool EXT_get(thread_db* /*tdbb*/, record_param* rpb, FB_UINT64& position)
{
/**************************************
 *
 *	E X T _ g e t
 *
 **************************************
 *
 * Functional description
 *	Get a record from an external file.
 *
 **************************************/
	jrd_rel* const relation = rpb->rpb_relation;
	ExternalFile* const file = relation->rel_file;
	fb_assert(file->ext_ifi);

	Record* const record = rpb->rpb_record;
	const Format* const format = record->rec_format;

	const USHORT offset = (USHORT) (IPTR) format->fmt_desc[0].dsc_address;
	UCHAR* p = record->rec_data + offset;
	const ULONG l = record->rec_length - offset;

	if (file->ext_ifi == NULL)
	{
		ERR_post(Arg::Gds(isc_io_error) << "fseek" << Arg::Str(file->ext_filename) <<
				 Arg::Gds(isc_io_open_err) << Arg::Unix(EBADF) <<
				 Arg::Gds(isc_random) << "File not opened");
	}

	// hvlad: fseek will flush file buffer and degrade performance, so don't
	// call it if it is not necessary. Note that we must flush file buffer if we
	// do read after write

	bool doSeek = false;
	if (!(file->ext_flags & EXT_last_read))
	{
		doSeek = true;
	}
	else
	{
		SINT64 offset = FTELL64(file->ext_ifi);
		if (offset < 0)
		{
			ERR_post(Arg::Gds(isc_io_error) << STRINGIZE(FTELL64) << Arg::Str(file->ext_filename) <<
					 Arg::Gds(isc_io_read_err) << SYS_ERR(errno));
		}
		doSeek = (static_cast<FB_UINT64>(offset) != position);
	}

	// reset both flags cause we are going to move the file pointer
	file->ext_flags &= ~(EXT_last_write | EXT_last_read);

	if (doSeek)
	{
		if (FSEEK64(file->ext_ifi, position, SEEK_SET) != 0)
		{
			ERR_post(Arg::Gds(isc_io_error) << STRINGIZE(FSEEK64) << Arg::Str(file->ext_filename) <<
					 Arg::Gds(isc_io_open_err) << SYS_ERR(errno));
		}
	}

	if (!fread(p, l, 1, file->ext_ifi))
	{
		return false;
	}

	position += l;
	file->ext_flags |= EXT_last_read;

	// Loop thru fields setting missing fields to either blanks/zeros or the missing value

	dsc desc;
	Format::fmt_desc_const_iterator desc_ptr = format->fmt_desc.begin();

    SSHORT i = 0;
	for (vec<jrd_fld*>::iterator itr = relation->rel_fields->begin();
		i < format->fmt_count; ++i, ++itr, ++desc_ptr)
	{
	    const jrd_fld* field = *itr;

		record->setNull(i);

		if (!desc_ptr->dsc_length || !field)
			continue;

		const LiteralNode* literal = ExprNode::as<LiteralNode>(field->fld_missing_value);

		if (literal)
		{
			desc = *desc_ptr;
			desc.dsc_address = record->rec_data + (IPTR) desc.dsc_address;

			if (!MOV_compare(&literal->litDesc, &desc))
				continue;
		}

		record->clearNull(i);
	}

	return true;
}


void EXT_modify(record_param* /*old_rpb*/, record_param* /*new_rpb*/, jrd_tra* /*transaction*/)
{
/**************************************
 *
 *	E X T _ m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Update an external file.
 *
 **************************************/

	ERR_post(Arg::Gds(isc_ext_file_modify));
}


void EXT_open(Database* dbb, ExternalFile* file)
{
/**************************************
 *
 *	E X T _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a record stream for an external file.
 *
 **************************************/
	if (!file->ext_ifi) {
		ext_fopen(dbb, file);
	}
}


void EXT_store(thread_db* tdbb, record_param* rpb)
{
/**************************************
 *
 *	E X T _ s t o r e
 *
 **************************************
 *
 * Functional description
 *	Update an external file.
 *
 **************************************/
	jrd_rel* relation = rpb->rpb_relation;
	ExternalFile* file = relation->rel_file;
	Record* record = rpb->rpb_record;
	const Format* format = record->rec_format;

	if (!file->ext_ifi) {
		ext_fopen(tdbb->getDatabase(), file);
	}

	// Loop thru fields setting missing fields to either blanks/zeros or the missing value

	// check if file is read only if read only then post error we cannot write to this file
	if (file->ext_flags & EXT_readonly)
	{
		Database* dbb = tdbb->getDatabase();
		CHECK_DBB(dbb);
		// Distinguish error message for a ReadOnly database
		if (dbb->readOnly())
			ERR_post(Arg::Gds(isc_read_only_database));
		else
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("insert") << Arg::Str(file->ext_filename) <<
					 Arg::Gds(isc_io_write_err) <<
					 Arg::Gds(isc_ext_readonly_err));
		}
	}

	dsc desc;
	vec<jrd_fld*>::iterator field_ptr = relation->rel_fields->begin();
	Format::fmt_desc_const_iterator desc_ptr = format->fmt_desc.begin();

	for (USHORT i = 0; i < format->fmt_count; ++i, ++field_ptr, ++desc_ptr)
	{
		const jrd_fld* field = *field_ptr;
		if (field && !field->fld_computation && desc_ptr->dsc_length && record->isNull(i))
		{
			UCHAR* p = record->rec_data + (IPTR) desc_ptr->dsc_address;
			LiteralNode* literal = ExprNode::as<LiteralNode>(field->fld_missing_value);

			if (literal)
			{
				desc = *desc_ptr;
				desc.dsc_address = p;
				MOV_move(tdbb, &literal->litDesc, &desc);
			}
			else
			{
				const UCHAR pad = (desc_ptr->dsc_dtype == dtype_text) ? ' ' : 0;
				memset(p, pad, desc_ptr->dsc_length);
			}
		}
	}

	const USHORT offset = (USHORT) (IPTR) format->fmt_desc[0].dsc_address;
	const UCHAR* p = record->rec_data + offset;
	const ULONG l = record->rec_length - offset;

	// hvlad: fseek will flush file buffer and degrade performance, so don't
	// call it if it is not necessary.	Note that we must flush file buffer if we
	// do write after read
	file->ext_flags &= ~EXT_last_read;
	if (file->ext_ifi == NULL ||
		(!(file->ext_flags & EXT_last_write) && FSEEK64(file->ext_ifi, (SINT64) 0, SEEK_END) != 0) )
	{
		file->ext_flags &= ~EXT_last_write;
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("fseek") << Arg::Str(file->ext_filename) <<
				 Arg::Gds(isc_io_open_err) << SYS_ERR(errno));
	}

	if (!fwrite(p, l, 1, file->ext_ifi))
	{
		file->ext_flags &= ~EXT_last_write;
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("fwrite") << Arg::Str(file->ext_filename) <<
				 Arg::Gds(isc_io_open_err) << SYS_ERR(errno));
	}

	// fflush(file->ext_ifi);
	file->ext_flags |= EXT_last_write;
}


void EXT_tra_attach(ExternalFile* file, jrd_tra*)
{
/**************************************
 *
 *	E X T _ t r a _ a t t a c h
 *
 **************************************
 *
 * Functional description
 *	Transaction going to use external table.
 *  Increment transactions use count.
 *
 **************************************/

	file->ext_tra_cnt++;
}

void EXT_tra_detach(ExternalFile* file, jrd_tra*)
{
/**************************************
 *
 *	E X T _ t r a _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Transaction used external table is finished.
 *  Decrement transactions use count and close
 *  external file if count is zero.
 *
 **************************************/

	file->ext_tra_cnt--;
	if (!file->ext_tra_cnt && file->ext_ifi)
	{
		fclose(file->ext_ifi);
		file->ext_ifi = NULL;
	}
}
