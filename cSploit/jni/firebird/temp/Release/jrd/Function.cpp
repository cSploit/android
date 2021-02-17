/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
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
 */

#include "firebird.h"
#include "../common/gdsassert.h"
#include "../jrd/flags.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../jrd/lck.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/blb.h"
#include "../jrd/met.h"
#include "../jrd/align.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/flu_proto.h"
#include "../jrd/fun_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/utils_proto.h"
#include "../jrd/DebugInterface.h"

#include "../jrd/Function.h"

using namespace Firebird;
using namespace Jrd;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [133] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 12,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 13,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 2,0, 1,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 0,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_12 [166] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 10,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 25,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 9,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_26 [279] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 20,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 10,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 0,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 1,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 2,0, 17,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 12,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 0, 17,0, 
                     blr_parameter2, 1, 14,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 16,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 19,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_51 [89] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 42,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 1, 0,0, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_58 [258] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 23,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 0,0, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 1,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 2,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter2, 1, 3,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 4,0, 17,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter2, 1, 5,0, 16,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 6,0, 21,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 7,0, 22,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 11,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 19,0, 
                     blr_parameter2, 1, 19,0, 18,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 20,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 9,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_85 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 9,0, 
                        blr_value_if, 
                           blr_eql, 
                              blr_parameter, 0, 0,0, 
                              blr_literal, blr_text2, 3,0, 0,0, 
                           blr_null, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_92 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 1, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


const char* const Function::EXCEPTION_MESSAGE = "The user defined function: \t%s\n\t   referencing"
	" entrypoint: \t%s\n\t                in module: \t%s\n\tcaused the fatal exception:";

Function* Function::lookup(thread_db* tdbb, USHORT id, bool return_deleted, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_96;	// gds__utility 
          SSHORT jrd_97;	// RDB$FUNCTION_ID 
   } jrd_95;
   struct {
          SSHORT jrd_94;	// RDB$FUNCTION_ID 
   } jrd_93;
	Jrd::Attachment* attachment = tdbb->getAttachment();
	Function* check_function = NULL;

	Function* function = (id < attachment->att_functions.getCount()) ? attachment->att_functions[id] : NULL;

	if (function && function->getId() == id &&
		!(function->flags & Routine::FLAG_BEING_SCANNED) &&
		((function->flags & Routine::FLAG_SCANNED) || noscan) &&
		!(function->flags & Routine::FLAG_BEING_ALTERED) &&
		(!(function->flags & Routine::FLAG_OBSOLETE) || return_deleted))
	{
		if (!(function->flags & Routine::FLAG_CHECK_EXISTENCE))
		{
			return function;
		}

		check_function = function;
		LCK_lock(tdbb, check_function->existenceLock, LCK_SR, LCK_WAIT);
	}

	// We need to look up the function in RDB$FUNCTIONS

	function = NULL;

	AutoCacheRequest request(tdbb, irq_l_fun_id, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FUNCTIONS WITH X.RDB$FUNCTION_ID EQ id*/
	{
	request.compile(tdbb, (UCHAR*) jrd_92, sizeof(jrd_92));
	jrd_93.jrd_94 = id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_93);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_95);
	   if (!jrd_95.jrd_96) break;
	{
		function = loadMetadata(tdbb, /*X.RDB$FUNCTION_ID*/
					      jrd_95.jrd_97, noscan, flags);
	}
	/*END_FOR*/
	   }
	}

	if (check_function)
	{
		check_function->flags &= ~Routine::FLAG_CHECK_EXISTENCE;
		if (check_function != function)
		{
			LCK_release(tdbb, check_function->existenceLock);
			check_function->flags |= Routine::FLAG_OBSOLETE;
		}
	}

	return function;
}

Function* Function::lookup(thread_db* tdbb, const QualifiedName& name, bool noscan)
{
   struct {
          SSHORT jrd_90;	// gds__utility 
          SSHORT jrd_91;	// RDB$FUNCTION_ID 
   } jrd_89;
   struct {
          TEXT  jrd_87 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_88 [32];	// RDB$FUNCTION_NAME 
   } jrd_86;
	Jrd::Attachment* attachment = tdbb->getAttachment();

	Function* check_function = NULL;

	// See if we already know the function by name

	for (Function** iter = attachment->att_functions.begin(); iter < attachment->att_functions.end(); ++iter)
	{
		Function* const function = *iter;

		if (function && !(function->flags & Routine::FLAG_OBSOLETE) &&
			((function->flags & Routine::FLAG_SCANNED) || noscan) &&
			!(function->flags & Routine::FLAG_BEING_SCANNED) &&
			!(function->flags & Routine::FLAG_BEING_ALTERED))
		{
			if (function->getName() == name)
			{
				if (function->flags & Routine::FLAG_CHECK_EXISTENCE)
				{
					check_function = function;
					LCK_lock(tdbb, check_function->existenceLock, LCK_SR, LCK_WAIT);
					break;
				}

				return function;
			}
		}
	}

	// We need to look up the function in RDB$FUNCTIONS

	Function* function = NULL;

	AutoCacheRequest request(tdbb, irq_l_fun_name, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$FUNCTIONS
		WITH X.RDB$FUNCTION_NAME EQ name.identifier.c_str() AND
			 X.RDB$PACKAGE_NAME EQUIV NULLIF(name.package.c_str(), '')*/
	{
	request.compile(tdbb, (UCHAR*) jrd_85, sizeof(jrd_85));
	gds__vtov ((const char*) name.package.c_str(), (char*) jrd_86.jrd_87, 32);
	gds__vtov ((const char*) name.identifier.c_str(), (char*) jrd_86.jrd_88, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_86);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_89);
	   if (!jrd_89.jrd_90) break;
	{
		function = loadMetadata(tdbb, /*X.RDB$FUNCTION_ID*/
					      jrd_89.jrd_91, noscan, 0);
	}
	/*END_FOR*/
	   }
	}

	if (check_function)
	{
		check_function->flags &= ~Routine::FLAG_CHECK_EXISTENCE;
		if (check_function != function)
		{
			LCK_release(tdbb, check_function->existenceLock);
			check_function->flags |= Routine::FLAG_OBSOLETE;
		}
	}

	return function;
}

Function* Function::loadMetadata(thread_db* tdbb, USHORT id, bool noscan, USHORT flags)
{
   struct {
          SSHORT jrd_11;	// gds__utility 
   } jrd_10;
   struct {
          SSHORT jrd_8;	// gds__null_flag 
          SSHORT jrd_9;	// RDB$VALID_BLR 
   } jrd_7;
   struct {
          SSHORT jrd_4;	// gds__utility 
          SSHORT jrd_5;	// gds__null_flag 
          SSHORT jrd_6;	// RDB$VALID_BLR 
   } jrd_3;
   struct {
          SSHORT jrd_2;	// RDB$FUNCTION_ID 
   } jrd_1;
   struct {
          bid  jrd_16;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_17 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_18;	// gds__utility 
          SSHORT jrd_19;	// gds__null_flag 
          SSHORT jrd_20;	// RDB$COLLATION_ID 
          SSHORT jrd_21;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_22;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_23;	// RDB$FIELD_LENGTH 
          SSHORT jrd_24;	// RDB$FIELD_SCALE 
          SSHORT jrd_25;	// RDB$FIELD_TYPE 
   } jrd_15;
   struct {
          TEXT  jrd_14 [32];	// RDB$FIELD_SOURCE 
   } jrd_13;
   struct {
          TEXT  jrd_31 [32];	// RDB$FIELD_SOURCE 
          bid  jrd_32;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_33 [32];	// RDB$ARGUMENT_NAME 
          SSHORT jrd_34;	// gds__utility 
          SSHORT jrd_35;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_36;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_37;	// RDB$FIELD_LENGTH 
          SSHORT jrd_38;	// RDB$FIELD_SCALE 
          SSHORT jrd_39;	// RDB$FIELD_TYPE 
          SSHORT jrd_40;	// gds__null_flag 
          SSHORT jrd_41;	// gds__null_flag 
          SSHORT jrd_42;	// gds__null_flag 
          SSHORT jrd_43;	// RDB$COLLATION_ID 
          SSHORT jrd_44;	// gds__null_flag 
          SSHORT jrd_45;	// RDB$ARGUMENT_MECHANISM 
          SSHORT jrd_46;	// gds__null_flag 
          SSHORT jrd_47;	// RDB$NULL_FLAG 
          SSHORT jrd_48;	// gds__null_flag 
          SSHORT jrd_49;	// RDB$MECHANISM 
          SSHORT jrd_50;	// RDB$ARGUMENT_POSITION 
   } jrd_30;
   struct {
          TEXT  jrd_28 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_29 [32];	// RDB$FUNCTION_NAME 
   } jrd_27;
   struct {
          TEXT  jrd_55 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_56;	// gds__utility 
          SSHORT jrd_57;	// gds__null_flag 
   } jrd_54;
   struct {
          TEXT  jrd_53 [32];	// RDB$PACKAGE_NAME 
   } jrd_52;
   struct {
          bid  jrd_62;	// RDB$DEBUG_INFO 
          bid  jrd_63;	// RDB$FUNCTION_SOURCE 
          bid  jrd_64;	// RDB$FUNCTION_BLR 
          TEXT  jrd_65 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_66 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_67 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_68 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_69 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_70 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_71;	// gds__utility 
          SSHORT jrd_72;	// gds__null_flag 
          SSHORT jrd_73;	// RDB$VALID_BLR 
          SSHORT jrd_74;	// gds__null_flag 
          SSHORT jrd_75;	// gds__null_flag 
          SSHORT jrd_76;	// gds__null_flag 
          SSHORT jrd_77;	// gds__null_flag 
          SSHORT jrd_78;	// gds__null_flag 
          SSHORT jrd_79;	// gds__null_flag 
          SSHORT jrd_80;	// gds__null_flag 
          SSHORT jrd_81;	// RDB$DETERMINISTIC_FLAG 
          SSHORT jrd_82;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_83;	// gds__null_flag 
          SSHORT jrd_84;	// gds__null_flag 
   } jrd_61;
   struct {
          SSHORT jrd_60;	// RDB$FUNCTION_ID 
   } jrd_59;
	Jrd::Attachment* attachment = tdbb->getAttachment();
	jrd_tra* sysTransaction = attachment->getSysTransaction();
	Database* const dbb = tdbb->getDatabase();

	if (id >= attachment->att_functions.getCount())
		attachment->att_functions.grow(id + 1);

	Function* function = attachment->att_functions[id];

	if (function && !(function->flags & Routine::FLAG_OBSOLETE))
	{
		// Make sure Routine::FLAG_BEING_SCANNED and Routine::FLAG_SCANNED are not set at the same time
		fb_assert(!(function->flags & Routine::FLAG_BEING_SCANNED) ||
			!(function->flags & Routine::FLAG_SCANNED));

		if ((function->flags & Routine::FLAG_BEING_SCANNED) ||
			(function->flags & Routine::FLAG_SCANNED))
		{
			return function;
		}
	}

	if (!function)
		function = FB_NEW(*attachment->att_pool) Function(*attachment->att_pool);

	try
	{
	function->flags |= (Routine::FLAG_BEING_SCANNED | flags);
	function->flags &= ~Routine::FLAG_OBSOLETE;

	function->setId(id);
	attachment->att_functions[id] = function;

	if (!function->existenceLock)
	{
		Lock* const lock = FB_NEW_RPT(*attachment->att_pool, 0)
			Lock(tdbb, sizeof(SLONG), LCK_fun_exist, function, blockingAst);
		function->existenceLock = lock;
		lock->lck_key.lck_long = function->getId();
	}

	LCK_lock(tdbb, function->existenceLock, LCK_SR, LCK_WAIT);

	if (!noscan)
	{
		bool valid_blr = true;

		AutoCacheRequest request_fun(tdbb, irq_l_functions, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request_fun)
			X IN RDB$FUNCTIONS
			WITH X.RDB$FUNCTION_ID EQ id*/
		{
		request_fun.compile(tdbb, (UCHAR*) jrd_58, sizeof(jrd_58));
		jrd_59.jrd_60 = id;
		EXE_start (tdbb, request_fun, attachment->getSysTransaction());
		EXE_send (tdbb, request_fun, 0, 2, (UCHAR*) &jrd_59);
		while (1)
		   {
		   EXE_receive (tdbb, request_fun, 1, 692, (UCHAR*) &jrd_61);
		   if (!jrd_61.jrd_71) break;
		{
			function->setName(QualifiedName(/*X.RDB$FUNCTION_NAME*/
							jrd_61.jrd_70,
				(/*X.RDB$PACKAGE_NAME.NULL*/
				 jrd_61.jrd_84 ? NULL : /*X.RDB$PACKAGE_NAME*/
	  jrd_61.jrd_69)));

			if (!/*X.RDB$SECURITY_CLASS.NULL*/
			     jrd_61.jrd_83)
			{
				function->setSecurityName(/*X.RDB$SECURITY_CLASS*/
							  jrd_61.jrd_68);
			}
			else if (!/*X.RDB$PACKAGE_NAME.NULL*/
				  jrd_61.jrd_84)
			{
				AutoCacheRequest requestHandle(tdbb, irq_l_procedure_pkg_class, IRQ_REQUESTS);

				/*FOR (REQUEST_HANDLE requestHandle)
					PKG IN RDB$PACKAGES
					WITH PKG.RDB$PACKAGE_NAME EQ X.RDB$PACKAGE_NAME*/
				{
				requestHandle.compile(tdbb, (UCHAR*) jrd_51, sizeof(jrd_51));
				gds__vtov ((const char*) jrd_61.jrd_69, (char*) jrd_52.jrd_53, 32);
				EXE_start (tdbb, requestHandle, attachment->getSysTransaction());
				EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_52);
				while (1)
				   {
				   EXE_receive (tdbb, requestHandle, 1, 36, (UCHAR*) &jrd_54);
				   if (!jrd_54.jrd_56) break;

					if (!/*PKG.RDB$SECURITY_CLASS.NULL*/
					     jrd_54.jrd_57)
					{
						function->setSecurityName(/*PKG.RDB$SECURITY_CLASS*/
									  jrd_54.jrd_55);
					}

				/*END_FOR*/
				   }
				}
			}

			size_t count = 0;
			ULONG length = 0;

			function->fun_inputs = 0;
			function->setDefaultCount(0);

			function->getInputFields().clear();
			function->getOutputFields().clear();

			AutoCacheRequest request_arg(tdbb, irq_l_args, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request_arg)
				Y IN RDB$FUNCTION_ARGUMENTS
				WITH Y.RDB$FUNCTION_NAME EQ function->getName().identifier.c_str() AND
					 Y.RDB$PACKAGE_NAME EQUIV NULLIF(function->getName().package.c_str(), '')
				SORTED BY Y.RDB$ARGUMENT_POSITION*/
			{
			request_arg.compile(tdbb, (UCHAR*) jrd_26, sizeof(jrd_26));
			gds__vtov ((const char*) function->getName().package.c_str(), (char*) jrd_27.jrd_28, 32);
			gds__vtov ((const char*) function->getName().identifier.c_str(), (char*) jrd_27.jrd_29, 32);
			EXE_start (tdbb, request_arg, attachment->getSysTransaction());
			EXE_send (tdbb, request_arg, 0, 64, (UCHAR*) &jrd_27);
			while (1)
			   {
			   EXE_receive (tdbb, request_arg, 1, 106, (UCHAR*) &jrd_30);
			   if (!jrd_30.jrd_34) break;
			{
				Parameter* parameter = FB_NEW(function->getPool()) Parameter(function->getPool());

				if (/*Y.RDB$ARGUMENT_POSITION*/
				    jrd_30.jrd_50 != /*X.RDB$RETURN_ARGUMENT*/
    jrd_61.jrd_82)
				{
					function->fun_inputs++;
					int newCount = /*Y.RDB$ARGUMENT_POSITION*/
						       jrd_30.jrd_50 - function->getOutputFields().getCount();
					fb_assert(newCount >= 0);

					function->getInputFields().resize(newCount + 1);
					function->getInputFields()[newCount] = parameter;
				}
				else
				{
					fb_assert(function->getOutputFields().isEmpty());
					function->getOutputFields().add(parameter);
				}

				parameter->prm_fun_mechanism = (FUN_T) /*Y.RDB$MECHANISM*/
								       jrd_30.jrd_49;
				parameter->prm_number = /*Y.RDB$ARGUMENT_POSITION*/
							jrd_30.jrd_50;
				parameter->prm_name = /*Y.RDB$ARGUMENT_NAME.NULL*/
						      jrd_30.jrd_48 ? "" : /*Y.RDB$ARGUMENT_NAME*/
	jrd_30.jrd_33;
				parameter->prm_nullable = /*Y.RDB$NULL_FLAG.NULL*/
							  jrd_30.jrd_46 || /*Y.RDB$NULL_FLAG*/
    jrd_30.jrd_47 == 0;
				parameter->prm_mechanism = /*Y.RDB$ARGUMENT_MECHANISM.NULL*/
							   jrd_30.jrd_44 ?
					prm_mech_normal : (prm_mech_t) /*Y.RDB$ARGUMENT_MECHANISM*/
								       jrd_30.jrd_45;

				const SSHORT collation_id_null = /*Y.RDB$COLLATION_ID.NULL*/
								 jrd_30.jrd_42;
				const SSHORT collation_id = /*Y.RDB$COLLATION_ID*/
							    jrd_30.jrd_43;

				SSHORT default_value_null = /*Y.RDB$DEFAULT_VALUE.NULL*/
							    jrd_30.jrd_41;
				bid default_value = /*Y.RDB$DEFAULT_VALUE*/
						    jrd_30.jrd_32;

				if (!/*Y.RDB$FIELD_SOURCE.NULL*/
				     jrd_30.jrd_40)
				{
					parameter->prm_field_source = /*Y.RDB$FIELD_SOURCE*/
								      jrd_30.jrd_31;

					AutoCacheRequest request_arg_fld(tdbb, irq_l_arg_fld, IRQ_REQUESTS);

					/*FOR(REQUEST_HANDLE request_arg_fld)
						F IN RDB$FIELDS
						WITH F.RDB$FIELD_NAME = Y.RDB$FIELD_SOURCE*/
					{
					request_arg_fld.compile(tdbb, (UCHAR*) jrd_12, sizeof(jrd_12));
					gds__vtov ((const char*) jrd_30.jrd_31, (char*) jrd_13.jrd_14, 32);
					EXE_start (tdbb, request_arg_fld, attachment->getSysTransaction());
					EXE_send (tdbb, request_arg_fld, 0, 32, (UCHAR*) &jrd_13);
					while (1)
					   {
					   EXE_receive (tdbb, request_arg_fld, 1, 56, (UCHAR*) &jrd_15);
					   if (!jrd_15.jrd_18) break;
					{
						DSC_make_descriptor(&parameter->prm_desc, /*F.RDB$FIELD_TYPE*/
											  jrd_15.jrd_25,
											/*F.RDB$FIELD_SCALE*/
											jrd_15.jrd_24, /*F.RDB$FIELD_LENGTH*/
  jrd_15.jrd_23,
											/*F.RDB$FIELD_SUB_TYPE*/
											jrd_15.jrd_22, /*F.RDB$CHARACTER_SET_ID*/
  jrd_15.jrd_21,
											(collation_id_null ? /*F.RDB$COLLATION_ID*/
													     jrd_15.jrd_20 : collation_id));

						if (default_value_null && fb_utils::implicit_domain(/*F.RDB$FIELD_NAME*/
												    jrd_15.jrd_17))
						{
							default_value_null = /*F.RDB$DEFAULT_VALUE.NULL*/
									     jrd_15.jrd_19;
							default_value = /*F.RDB$DEFAULT_VALUE*/
									jrd_15.jrd_16;
						}
					}
					/*END_FOR*/
					   }
					}
				}
				else
				{
					DSC_make_descriptor(&parameter->prm_desc, /*Y.RDB$FIELD_TYPE*/
										  jrd_30.jrd_39,
										/*Y.RDB$FIELD_SCALE*/
										jrd_30.jrd_38, /*Y.RDB$FIELD_LENGTH*/
  jrd_30.jrd_37,
										/*Y.RDB$FIELD_SUB_TYPE*/
										jrd_30.jrd_36, /*Y.RDB$CHARACTER_SET_ID*/
  jrd_30.jrd_35,
										(collation_id_null ? 0 : collation_id));
				}

				if (/*Y.RDB$ARGUMENT_POSITION*/
				    jrd_30.jrd_50 != /*X.RDB$RETURN_ARGUMENT*/
    jrd_61.jrd_82 && !default_value_null)
				{
					function->setDefaultCount(function->getDefaultCount() + 1);

					MemoryPool* const csb_pool = attachment->createPool();
					Jrd::ContextPoolHolder context(tdbb, csb_pool);

					try
					{
						parameter->prm_default_value = static_cast<ValueExprNode*>(MET_parse_blob(
							tdbb, NULL, &default_value, NULL, NULL, false, false));
					}
					catch (const Exception&)
					{
						attachment->deletePool(csb_pool);
						throw; // an explicit error message would be better
					}
				}

				if (parameter->prm_desc.dsc_dtype == dtype_cstring)
					parameter->prm_desc.dsc_length++;

				length += (parameter->prm_desc.dsc_dtype == dtype_blob) ?
					sizeof(udf_blob) : FB_ALIGN(parameter->prm_desc.dsc_length, FB_DOUBLE_ALIGN);

				count = MAX(count, size_t(/*Y.RDB$ARGUMENT_POSITION*/
							  jrd_30.jrd_50));
			}
			/*END_FOR*/
			   }
			}

			for (int i = (int) function->getInputFields().getCount() - 1; i >= 0; --i)
			{
				if (!function->getInputFields()[i])
					function->getInputFields().remove(i);
			}

			function->fun_return_arg = /*X.RDB$RETURN_ARGUMENT*/
						   jrd_61.jrd_82;
			function->fun_temp_length = length;

			// Prepare the exception message to be used in case this function ever
			// causes an exception.  This is done at this time to save us from preparing
			// (thus allocating) this message every time the function is called.
			function->fun_exception_message.printf(EXCEPTION_MESSAGE,
				function->getName().toString().c_str(), /*X.RDB$ENTRYPOINT*/
									jrd_61.jrd_67, /*X.RDB$MODULE_NAME*/
  jrd_61.jrd_66);

			if (!/*X.RDB$DETERMINISTIC_FLAG.NULL*/
			     jrd_61.jrd_80)
				function->fun_deterministic = (/*X.RDB$DETERMINISTIC_FLAG*/
							       jrd_61.jrd_81 != 0);

			function->setImplemented(true);
			function->setDefined(true);

			function->fun_entrypoint = NULL;
			function->fun_external = NULL;
			function->setStatement(NULL);

			if (!/*X.RDB$MODULE_NAME.NULL*/
			     jrd_61.jrd_79 && !/*X.RDB$ENTRYPOINT.NULL*/
     jrd_61.jrd_78)
			{
				function->fun_entrypoint =
					Module::lookup(/*X.RDB$MODULE_NAME*/
						       jrd_61.jrd_66, /*X.RDB$ENTRYPOINT*/
  jrd_61.jrd_67, dbb->dbb_modules);

				// Could not find a function with given MODULE, ENTRYPOINT.
				// Try the list of internally implemented functions.
				if (!function->fun_entrypoint)
				{
					function->fun_entrypoint =
						BUILTIN_entrypoint(/*X.RDB$MODULE_NAME*/
								   jrd_61.jrd_66, /*X.RDB$ENTRYPOINT*/
  jrd_61.jrd_67);
				}

				if (!function->fun_entrypoint)
					function->setDefined(false);
			}
			else if (!/*X.RDB$ENGINE_NAME.NULL*/
				  jrd_61.jrd_77 || !/*X.RDB$FUNCTION_BLR.NULL*/
     jrd_61.jrd_76)
			{
				MemoryPool* const csb_pool = attachment->createPool();
				Jrd::ContextPoolHolder context(tdbb, csb_pool);

				try
				{
					AutoPtr<CompilerScratch> csb(CompilerScratch::newCsb(*csb_pool, 5));

					if (!/*X.RDB$ENGINE_NAME.NULL*/
					     jrd_61.jrd_77)
					{
						HalfStaticArray<UCHAR, 512> body;

						if (!/*X.RDB$FUNCTION_SOURCE.NULL*/
						     jrd_61.jrd_75)
						{
							blb* const blob = blb::open(tdbb, sysTransaction, &/*X.RDB$FUNCTION_SOURCE*/
													   jrd_61.jrd_63);
							const ULONG len = blob->BLB_get_data(tdbb,
								body.getBuffer(blob->blb_length + 1), blob->blb_length + 1);
							body[MIN(blob->blb_length, len)] = 0;
						}
						else
							body.getBuffer(1)[0] = 0;

						dbb->dbb_extManager.makeFunction(tdbb, csb, function, /*X.RDB$ENGINE_NAME*/
												      jrd_61.jrd_65,
							(/*X.RDB$ENTRYPOINT.NULL*/
							 jrd_61.jrd_78 ? "" : /*X.RDB$ENTRYPOINT*/
	jrd_61.jrd_67), (char*) body.begin());

						if (!function->fun_external)
							function->setDefined(false);
					}
					else if (!/*X.RDB$FUNCTION_BLR.NULL*/
						  jrd_61.jrd_76)
					{
						if (!/*X.RDB$DEBUG_INFO.NULL*/
						     jrd_61.jrd_74)
							DBG_parse_debug_info(tdbb, &/*X.RDB$DEBUG_INFO*/
										    jrd_61.jrd_62, *csb->csb_dbg_info);

						try
						{
							function->parseBlr(tdbb, csb, &/*X.RDB$FUNCTION_BLR*/
										       jrd_61.jrd_64);
						}
						catch (const Exception& ex)
						{
							ISC_STATUS_ARRAY temp_status;
							ex.stuff_exception(temp_status);
							const string name = function->getName().toString();
							(Arg::Gds(isc_bad_fun_BLR) << Arg::Str(name)
								<< Arg::StatusVector(temp_status)).raise();
						}
					}
				}
				catch (const Exception&)
				{
					attachment->deletePool(csb_pool);
					throw;
				}

				function->getStatement()->function = function;
			}
			else
			{
				RefPtr<MsgMetadata> inputMetadata(REF_NO_INCR, createMetadata(function->getInputFields()));
				function->setInputFormat(createFormat(function->getPool(), inputMetadata, false));

				RefPtr<MsgMetadata> outputMetadata(REF_NO_INCR, createMetadata(function->getOutputFields()));
				function->setOutputFormat(createFormat(function->getPool(), outputMetadata, true));

				function->setImplemented(false);
			}

			if (/*X.RDB$VALID_BLR.NULL*/
			    jrd_61.jrd_72 || /*X.RDB$VALID_BLR*/
    jrd_61.jrd_73 == FALSE)
				valid_blr = false;

			function->flags |= Routine::FLAG_SCANNED;
		}
		/*END_FOR*/
		   }
		}

		if (!dbb->readOnly() && !valid_blr)
		{
			// if the BLR was marked as invalid but the function was compiled,
			// mark the BLR as valid

			AutoRequest request_set_valid;

			/*FOR(REQUEST_HANDLE request_set_valid)
				F IN RDB$FUNCTIONS WITH
					F.RDB$FUNCTION_ID EQ function->getId() AND F.RDB$FUNCTION_BLR NOT MISSING*/
			{
			request_set_valid.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
			jrd_1.jrd_2 = function->getId();
			EXE_start (tdbb, request_set_valid, attachment->getSysTransaction());
			EXE_send (tdbb, request_set_valid, 0, 2, (UCHAR*) &jrd_1);
			while (1)
			   {
			   EXE_receive (tdbb, request_set_valid, 1, 6, (UCHAR*) &jrd_3);
			   if (!jrd_3.jrd_4) break;
			{
				/*MODIFY F USING*/
				{
				
					/*F.RDB$VALID_BLR*/
					jrd_3.jrd_6 = TRUE;
					/*F.RDB$VALID_BLR.NULL*/
					jrd_3.jrd_5 = FALSE;
				/*END_MODIFY*/
				jrd_7.jrd_8 = jrd_3.jrd_5;
				jrd_7.jrd_9 = jrd_3.jrd_6;
				EXE_send (tdbb, request_set_valid, 2, 4, (UCHAR*) &jrd_7);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, request_set_valid, 3, 2, (UCHAR*) &jrd_10);
			   }
			}
		}
	}

	// Make sure that it is really being scanned
	fb_assert(function->flags & Routine::FLAG_BEING_SCANNED);

	function->flags &= ~Routine::FLAG_BEING_SCANNED;

	}	// try
	catch (const Exception&)
	{
		function->flags &= ~(Routine::FLAG_BEING_SCANNED | Routine::FLAG_SCANNED);

		if (function->existenceLock)
		{
			LCK_release(tdbb, function->existenceLock);
			delete function->existenceLock;
			function->existenceLock = NULL;
		}

		throw;
	}

	return function;
}

int Function::blockingAst(void* ast_object)
{
	Function* const function = static_cast<Function*>(ast_object);

	try
	{
		Database* const dbb = function->existenceLock->lck_dbb;

		AsyncContextHolder tdbb(dbb, FB_FUNCTION, function->existenceLock);

		LCK_release(tdbb, function->existenceLock);
		function->flags |= Routine::FLAG_OBSOLETE;
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}

void Function::releaseLocks(thread_db* tdbb)
{
	if (existenceLock)
	{
		LCK_release(tdbb, existenceLock);
		flags |= Routine::FLAG_CHECK_EXISTENCE;
		useCount = 0;
	}
}

bool Function::checkCache(thread_db* tdbb) const
{
	return tdbb->getAttachment()->att_functions[getId()] == this;
}

void Function::clearCache(thread_db* tdbb)
{
	tdbb->getAttachment()->att_functions[getId()] = NULL;
}
