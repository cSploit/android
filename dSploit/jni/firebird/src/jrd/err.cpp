/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		err.cpp
 *	DESCRIPTION:	Bug check routine
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../jrd/common.h"
#include "gen/iberror.h"
#include <errno.h>
#include "../jrd/jrd.h"
#include "../jrd/os/pio.h"
#include "../jrd/val.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/rse.h"
#include "../jrd/tra.h"
#include "../jrd/cch_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/dbg_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"

using namespace Jrd;
using namespace Firebird;

//#define JRD_FAILURE_SPACE	2048
//#define JRD_FAILURE_UNKNOWN	"<UNKNOWN>"	/* Used when buffer fails */


static void internal_error(ISC_STATUS status, int number, const TEXT* file = NULL, int line = 0);
static void internal_post(const ISC_STATUS* status_vector);


void ERR_bugcheck(int number, const TEXT* file, int line)
{
/**************************************
 *
 *	E R R _ b u g c h e c k
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	Database* dbb = GET_DBB();
	dbb->dbb_flags |= DBB_bugcheck;

	CCH_shutdown_database(dbb);

	internal_error(isc_bug_check, number, file, line);
}


void ERR_bugcheck_msg(const TEXT* msg)
{
/**************************************
 *
 *	E R R _ b u g c h e c k _ m s g
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	Database* dbb = GET_DBB();

	dbb->dbb_flags |= DBB_bugcheck;
	DEBUG;
	CCH_shutdown_database(dbb);

	ERR_post(Arg::Gds(isc_bug_check) << Arg::Str(msg));
}


void ERR_corrupt(int number)
{
/**************************************
 *
 *	E R R _ c o r r u p t
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/

	internal_error(isc_db_corrupt, number);
}


void ERR_duplicate_error(idx_e code, const jrd_rel* relation, USHORT index_number, const TEXT* idx_name)
{
/**************************************
 *
 *	E R R _ d u p l i c a t e _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Duplicate error during index update.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	const Arg::StatusVector org_status(tdbb->tdbb_status_vector);

	MetaName index, constraint;
	if (!idx_name)
		MET_lookup_index(tdbb, index, relation->rel_name, index_number + 1);
	else
		index = idx_name;

	bool haveConstraint = true;
	if (index.hasData())
	{
		MET_lookup_cnstrt_for_index(tdbb, constraint, index);
		if (constraint.isEmpty())
		{
			haveConstraint = false;
			constraint = "***unknown***";
		}
	}
	else
	{
		haveConstraint = false;
		index = constraint = "***unknown***";
	}

	switch (code)
	{
	case idx_e_keytoobig:
		ERR_post(Arg::Gds(isc_imp_exc) <<
				 Arg::Gds(isc_keytoobig) << Arg::Str(index));
		break;

	case idx_e_conversion:
		org_status.raise();
		break;

	case idx_e_foreign_target_doesnt_exist:
		ERR_post(Arg::Gds(isc_foreign_key) << Arg::Str(constraint) <<
											  Arg::Str(relation->rel_name) <<
			 	 Arg::Gds(isc_foreign_key_target_doesnt_exist));
		break;

	case idx_e_foreign_references_present:
		ERR_post(Arg::Gds(isc_foreign_key) << Arg::Str(constraint) <<
											  Arg::Str(relation->rel_name) <<
			 	 Arg::Gds(isc_foreign_key_references_present));
		break;

	case idx_e_duplicate:
		if (haveConstraint)
		{
			ERR_post(Arg::Gds(isc_unique_key_violation) << Arg::Str(constraint) <<
														   Arg::Str(relation->rel_name));
		}
		else
			ERR_post(Arg::Gds(isc_no_dup) << Arg::Str(index));

	default:
		fb_assert(false);
	}
}


void ERR_error(int number)
{
/**************************************
 *
 *	E R R _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Post a user-level error.  This is a temporary mechanism
 *	that will eventually disappear.
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	DEBUG;
	if (gds__msg_lookup(0, JRD_BUGCHK, number, sizeof(errmsg), errmsg, NULL) < 1)
		sprintf(errmsg, "error code %d", number);

	ERR_post(Arg::Gds(isc_random) << Arg::Str(errmsg));
}


void ERR_error_msg(const TEXT* msg)
{
/**************************************
 *
 *	E R R _ e r r o r _ m s g
 *
 **************************************
 *
 * Functional description
 *	Post a user-level error.  This is a temporary mechanism
 *	that will eventually disappear.
 *
 **************************************/

	DEBUG;
	ERR_post(Arg::Gds(isc_random) << Arg::Str(msg));
}


void ERR_log(int facility, int number, const TEXT* message)
{
/**************************************
 *
 *	E R R _ l o g
 *
 **************************************
 *
 * Functional description
 *	Log a message to the firebird.log
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];
	thread_db* tdbb = JRD_get_thread_data();

	DEBUG;
	if (message)
	{
		strncpy(errmsg, message, sizeof(errmsg));
		errmsg[sizeof(errmsg) - 1] = 0;
	}
	else if (gds__msg_lookup(0, facility, number, sizeof(errmsg), errmsg, NULL) < 1)
		strcpy(errmsg, "Internal error code");

	const size_t len = strlen(errmsg);
	fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len, " (%d)", number);

	gds__log("Database: %s\n\t%s", (tdbb && tdbb->getAttachment()) ?
		tdbb->getAttachment()->att_filename.c_str() : "", errmsg);
}


bool ERR_post_warning(const Arg::StatusVector& v)
{
/**************************************
 *
 *      E R R _ p o s t _ w a r n i n g
 *
 **************************************
 *
 * Functional description
 *	Post a warning to the current status vector.
 *
 **************************************/
	fb_assert(v.value()[0] == isc_arg_warning);

	int indx = 0, warning_indx = 0;
	ISC_STATUS* const status_vector = JRD_get_thread_data()->tdbb_status_vector;

	if (status_vector[0] != isc_arg_gds ||
		(status_vector[0] == isc_arg_gds && status_vector[1] == 0 &&
			status_vector[2] != isc_arg_warning))
	{
		/* this is a blank status vector */
		fb_utils::init_status(status_vector);
		indx = 2;
	}
	else
	{
		/* find end of a status vector */
		PARSE_STATUS(status_vector, indx, warning_indx);
		if (indx)
			--indx;
	}

	/* stuff the warning */
	if (indx + v.length() + 1 < ISC_STATUS_LENGTH)
	{
		memcpy(&status_vector[indx], v.value(), sizeof(ISC_STATUS) * (v.length() + 1));
        ERR_make_permanent(&status_vector[indx]);
		return true;
	}

	/* not enough free space */
	return false;
}


void ERR_post_nothrow(const Arg::StatusVector& v)
/**************************************
 *
 *	E R R _ p o s t _ n o t h r o w
 *
 **************************************
 *
 * Functional description
 *	Create a status vector.
 *
 **************************************/
{
    fb_assert(v.value()[0] == isc_arg_gds);
	ISC_STATUS_ARRAY vector;
	v.copyTo(vector);
	ERR_make_permanent(vector);
	internal_post(vector);
}


void ERR_make_permanent(ISC_STATUS* s)
/**************************************
 *
 *	E R R _ m a k e _ p e r m a n e n t
 *
 **************************************
 *
 * Functional description
 *	Make strings in vector permanent
 *
 **************************************/
{
	makePermanentVector(s);
}


void ERR_make_permanent(Arg::StatusVector& v)
/**************************************
 *
 *	E R R _ m a k e _ p e r m a n e n t
 *
 **************************************
 *
 * Functional description
 *	Make strings in vector permanent
 *
 **************************************/
{
	ERR_make_permanent(const_cast<ISC_STATUS*>(v.value()));
}


void ERR_post(const Arg::StatusVector& v)
/**************************************
 *
 *	E R R _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Create a status vector and return to the user.
 *
 **************************************/
{
	ERR_post_nothrow(v);

	DEBUG;
	ERR_punt();
}


static void internal_post(const ISC_STATUS* tmp_status)
{
/**************************************
 *
 *	i n t e r n a l _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Append status vector with new values.
 *
 **************************************/

	/* calculate length of the status */
	int tmp_status_len = 0, warning_indx = 0;
	PARSE_STATUS(tmp_status, tmp_status_len, warning_indx);
	fb_assert(warning_indx == 0);

	ISC_STATUS* const status_vector = JRD_get_thread_data()->tdbb_status_vector;

	if (status_vector[0] != isc_arg_gds ||
		(status_vector[0] == isc_arg_gds && status_vector[1] == 0 &&
			status_vector[2] != isc_arg_warning))
	{
		/* this is a blank status vector just stuff the status */
		memcpy(status_vector, tmp_status, sizeof(ISC_STATUS) * tmp_status_len);
		return;
	}

	int status_len = 0;
	PARSE_STATUS(status_vector, status_len, warning_indx);
	if (status_len)
		--status_len;

	/* check for duplicated error code */
	int i;
	for (i = 0; i < ISC_STATUS_LENGTH; i++)
	{
		if (status_vector[i] == isc_arg_end && i == status_len)
			break;				/* end of argument list */

		if (i && i == warning_indx)
			break;				/* vector has no more errors */

		if (status_vector[i] == tmp_status[1] && i && status_vector[i - 1] != isc_arg_warning &&
			i + tmp_status_len - 2 < ISC_STATUS_LENGTH &&
			(memcmp(&status_vector[i], &tmp_status[1], sizeof(ISC_STATUS) * (tmp_status_len - 2)) == 0))
		{
			/* duplicate found */
			return;
		}
	}

/* if the status_vector has only warnings then adjust err_status_len */
	int err_status_len = i;
	if (err_status_len == 2 && warning_indx)
		err_status_len = 0;

	ISC_STATUS_ARRAY warning_status;
	int warning_count = 0;
	if (warning_indx)
	{
		/* copy current warning(s) to a temp buffer */
		MOVE_CLEAR(warning_status, sizeof(warning_status));
		memcpy(warning_status, &status_vector[warning_indx],
					sizeof(ISC_STATUS) * (ISC_STATUS_LENGTH - warning_indx));
		PARSE_STATUS(warning_status, warning_count, warning_indx);
	}

/* add the status into a real buffer right in between last error
   and first warning */

	if ((i = err_status_len + tmp_status_len) < ISC_STATUS_LENGTH)
	{
		memcpy(&status_vector[err_status_len], tmp_status, sizeof(ISC_STATUS) * tmp_status_len);
		/* copy current warning(s) to the status_vector */
		if (warning_count && i + warning_count - 1 < ISC_STATUS_LENGTH) {
			memcpy(&status_vector[i - 1], warning_status, sizeof(ISC_STATUS) * warning_count);
		}
	}
	return;
}


void ERR_punt()
{
/**************************************
 *
 *	E R R _ p u n t
 *
 **************************************
 *
 * Functional description
 *	Error stuff has been copied to status vector.  Now punt.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb = tdbb->getDatabase();

	if (dbb && (dbb->dbb_flags & DBB_bugcheck))
	{
		gds__log_status(dbb->dbb_filename.hasData() ? dbb->dbb_filename.c_str() : NULL,
			tdbb->tdbb_status_vector);
 		if (Config::getBugcheckAbort())
		{
			abort();
		}
	}

	ERR_make_permanent(tdbb->tdbb_status_vector);
	status_exception::raise(tdbb->tdbb_status_vector);
}


void ERR_warning(const Arg::StatusVector& v)
{
/**************************************
 *
 *	E R R _ w a r n i n g
 *
 **************************************
 *
 * Functional description
 *	Write an error out to the status vector but
 *	don't throw an exception.  This allows
 *	sending a warning message back to the user
 *	without stopping execution of a request.  Note
 *	that subsequent errors can supersede this one.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	ISC_STATUS* s = tdbb->tdbb_status_vector;

	v.copyTo(s);
	ERR_make_permanent(s);
	DEBUG;
	tdbb->getRequest()->req_flags |= req_warning;
}


void ERR_append_status(ISC_STATUS* status_vector, const Arg::StatusVector& v)
{
/**************************************
 *
 *	E R R _ a p p e n d _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Append the given status vector with the passed arguments.
 *
 **************************************/
	// First build a status vector with the passed one
	Arg::StatusVector passed(status_vector);

	// Now append the newly vector to the passed one
	passed.append(v);

	// Return the result
	passed.copyTo(status_vector);
	ERR_make_permanent(status_vector);
}


void ERR_build_status(ISC_STATUS* status_vector, const Arg::StatusVector& v)
{
/**************************************
 *
 *	E R R _  a p p e n d _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Append the given status vector with the passed arguments.
 *
 **************************************/
	v.copyTo(status_vector);
	ERR_make_permanent(status_vector);
}


static void internal_error(ISC_STATUS status, int number, const TEXT* file, int line)
{
/**************************************
 *
 *	i n t e r n a l _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Things seem to be going poorly today.
 *
 **************************************/
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	DEBUG;
	if (gds__msg_lookup(0, JRD_BUGCHK, number, sizeof(errmsg), errmsg, NULL) < 1)
		strcpy(errmsg, "Internal error code");

	const size_t len = strlen(errmsg);

	if (file)
	{
		// Remove path information
		const TEXT* ptr = file + strlen(file);
		for (; ptr > file; ptr--)
		{
			if ((*ptr == '/') || (*ptr == '\\'))
			{
				ptr++;
				break;
			}
		}
		fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len,
			" (%d), file: %s line: %d", number, ptr, line);
	}
	else {
		fb_utils::snprintf(errmsg + len, sizeof(errmsg) - len, " (%d)", number);
	}

	ERR_post(Arg::Gds(status) << Arg::Str(errmsg));
}
