/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn_util.epp
 *	DESCRIPTION:	Dynamic data definition - utility functions
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
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 *
 */

#include "firebird.h"
#include "dyn_consts.h"
#include <stdio.h>
#include <string.h>

#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/scl.h"
#include "../jrd/drq.h"
#include "../jrd/flags.h"
#include "../jrd/ibase.h"
#include "../jrd/lls.h"
#include "../jrd/met.h"
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/dyn.h"
#include "../jrd/ods.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dyn_ut_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../common/isc_f_proto.h"
#include "../jrd/vio_proto.h"
#include "../common/utils_proto.h"

using MsgFormat::SafeArg;

using namespace Firebird;
using namespace Jrd;

/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [45] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 24,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_4 [161] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 27,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_fid, 1, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 3,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 0, 4,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_equiv, 
                                    blr_fid, 1, 14,0, 
                                    blr_fid, 0, 5,0, 
                                 blr_and, 
                                    blr_eql, 
                                       blr_fid, 1, 3,0, 
                                       blr_literal, blr_long, 0, 1,0,0,0,
                                    blr_eql, 
                                       blr_fid, 1, 0,0, 
                                       blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 4,0, 
                     blr_parameter, 1, 0,0, 
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
static const UCHAR	jrd_13 [150] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 5,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 1,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 4,0, 
                           blr_and, 
                              blr_between, 
                                 blr_fid, 0, 4,0, 
                                 blr_parameter, 0, 3,0, 
                                 blr_parameter, 0, 2,0, 
                              blr_and, 
                                 blr_missing, 
                                    blr_fid, 0, 5,0, 
                                 blr_eql, 
                                    blr_fid, 1, 0,0, 
                                    blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 0,0, 
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
static const UCHAR	jrd_23 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_28 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 20,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_33 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 4,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_38 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_45 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 2,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_50 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_55 [77] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
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
                        blr_parameter, 0, 0,0, 
                     blr_missing, 
                        blr_fid, 0, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_60 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_65 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 30,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_70 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 4,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_75 [77] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_missing, 
                        blr_fid, 0, 16,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_80 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


static const UCHAR gen_id_blr1[] =
{
	blr_version5,
	blr_begin,
	blr_message, 0, 1, 0,
	blr_int64, 0,
	blr_begin,
	blr_send, 0,
	blr_begin,
	blr_assignment,
	blr_gen_id
};

static const UCHAR gen_id_blr2[] =
{
	blr_literal, blr_long, 0, 1, 0, 0, 0,
		blr_parameter, 0, 0, 0, blr_end, blr_end, blr_end, blr_eoc
};

void DYN_UTIL_check_unique_name(thread_db* tdbb, jrd_tra* transaction,
	const Firebird::MetaName& object_name, int object_type)
{
   struct {
          SSHORT jrd_59;	// gds__utility 
   } jrd_58;
   struct {
          TEXT  jrd_57 [32];	// RDB$FUNCTION_NAME 
   } jrd_56;
   struct {
          SSHORT jrd_64;	// gds__utility 
   } jrd_63;
   struct {
          TEXT  jrd_62 [32];	// RDB$GENERATOR_NAME 
   } jrd_61;
   struct {
          SSHORT jrd_69;	// gds__utility 
   } jrd_68;
   struct {
          TEXT  jrd_67 [32];	// RDB$EXCEPTION_NAME 
   } jrd_66;
   struct {
          SSHORT jrd_74;	// gds__utility 
   } jrd_73;
   struct {
          TEXT  jrd_72 [32];	// RDB$INDEX_NAME 
   } jrd_71;
   struct {
          SSHORT jrd_79;	// gds__utility 
   } jrd_78;
   struct {
          TEXT  jrd_77 [32];	// RDB$PROCEDURE_NAME 
   } jrd_76;
   struct {
          SSHORT jrd_84;	// gds__utility 
   } jrd_83;
   struct {
          TEXT  jrd_82 [32];	// RDB$RELATION_NAME 
   } jrd_81;
/**************************************
 *
 *	D Y N _ U T I L _ c h e c k _ u n i q u e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Check if an object already exists.
 *	If yes then return error.
 *
 **************************************/
	SET_TDBB(tdbb);

	USHORT error_code = 0;
	AutoCacheRequest request;

	switch (object_type)
	{
		case obj_relation:
		case obj_procedure:
			request.reset(tdbb, drq_l_rel_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				EREL IN RDB$RELATIONS WITH EREL.RDB$RELATION_NAME EQ object_name.c_str()*/
			{
			request.compile(tdbb, (UCHAR*) jrd_80, sizeof(jrd_80));
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_81.jrd_82, 32);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_81);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_83);
			   if (!jrd_83.jrd_84) break;
			{
				error_code = 132;
			}
			/*END_FOR*/
			   }
			}

			if (!error_code)
			{
				request.reset(tdbb, drq_l_prc_name, DYN_REQUESTS);

				/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					EPRC IN RDB$PROCEDURES
					WITH EPRC.RDB$PROCEDURE_NAME EQ object_name.c_str() AND
						 EPRC.RDB$PACKAGE_NAME MISSING*/
				{
				request.compile(tdbb, (UCHAR*) jrd_75, sizeof(jrd_75));
				gds__vtov ((const char*) object_name.c_str(), (char*) jrd_76.jrd_77, 32);
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_76);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_78);
				   if (!jrd_78.jrd_79) break;
				{
					error_code = 135;
				}
				/*END_FOR*/
				   }
				}
			}
			break;

		case obj_index:
			request.reset(tdbb, drq_l_idx_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				EIDX IN RDB$INDICES WITH EIDX.RDB$INDEX_NAME EQ object_name.c_str()*/
			{
			request.compile(tdbb, (UCHAR*) jrd_70, sizeof(jrd_70));
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_71.jrd_72, 32);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_71);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_73);
			   if (!jrd_73.jrd_74) break;
			{
				error_code = 251;
			}
			/*END_FOR*/
			   }
			}

			break;

		case obj_exception:
			request.reset(tdbb, drq_l_xcp_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				EXCP IN RDB$EXCEPTIONS WITH EXCP.RDB$EXCEPTION_NAME EQ object_name.c_str()*/
			{
			request.compile(tdbb, (UCHAR*) jrd_65, sizeof(jrd_65));
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_66.jrd_67, 32);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_66);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_68);
			   if (!jrd_68.jrd_69) break;
			{
				error_code = 253;
			}
			/*END_FOR*/
			   }
			}

			break;

		case obj_generator:
			request.reset(tdbb, drq_l_gen_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				EGEN IN RDB$GENERATORS WITH EGEN.RDB$GENERATOR_NAME EQ object_name.c_str()*/
			{
			request.compile(tdbb, (UCHAR*) jrd_60, sizeof(jrd_60));
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_61.jrd_62, 32);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_61);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_63);
			   if (!jrd_63.jrd_64) break;
			{
				error_code = 254;
			}
			/*END_FOR*/
			   }
			}

			break;

		case obj_udf:
			request.reset(tdbb, drq_l_fun_name, DYN_REQUESTS);

			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
				EFUN IN RDB$FUNCTIONS
				WITH EFUN.RDB$FUNCTION_NAME EQ object_name.c_str() AND
					 EFUN.RDB$PACKAGE_NAME MISSING*/
			{
			request.compile(tdbb, (UCHAR*) jrd_55, sizeof(jrd_55));
			gds__vtov ((const char*) object_name.c_str(), (char*) jrd_56.jrd_57, 32);
			EXE_start (tdbb, request, transaction);
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_56);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_58);
			   if (!jrd_58.jrd_59) break;
			{
				error_code = 268;
			}
			/*END_FOR*/
			   }
			}

			break;

		default:
			fb_assert(false);
	}

	if (error_code)
		status_exception::raise(Arg::PrivateDyn(error_code) << object_name.c_str());
}


SINT64 DYN_UTIL_gen_unique_id(thread_db* tdbb, SSHORT id, const char* generator_name)
{
/**************************************
 *
 *	D Y N _ U T I L _ g e n _ u n i q u e _ i d
 *
 **************************************
 *
 * Functional description
 *	Generate a unique id using a generator.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, id, DYN_REQUESTS);
	SINT64 value = 0;

	if (!request)
	{
		const size_t name_length = strlen(generator_name);
		fb_assert(name_length < MAX_SQL_IDENTIFIER_SIZE);
		const size_t blr_size = sizeof(gen_id_blr1) + sizeof(gen_id_blr2) + 1 + name_length;

		Firebird::UCharBuffer blr;
		UCHAR* p = blr.getBuffer(blr_size);

		memcpy(p, gen_id_blr1, sizeof(gen_id_blr1));
		p += sizeof(gen_id_blr1);
		*p++ = name_length;
		memcpy(p, generator_name, name_length);
		p += name_length;
		memcpy(p, gen_id_blr2, sizeof(gen_id_blr2));
		p += sizeof(gen_id_blr2);
		fb_assert(size_t(p - blr.begin()) == blr_size);

		request.compile(tdbb, blr.begin(), (ULONG) blr.getCount());
	}

	EXE_start(tdbb, request, attachment->getSysTransaction());
	EXE_receive(tdbb, request, 0, sizeof(value), (UCHAR*) &value);

	return value;
}


void DYN_UTIL_generate_constraint_name(thread_db* tdbb, MetaName& buffer)
{
   struct {
          SSHORT jrd_54;	// gds__utility 
   } jrd_53;
   struct {
          TEXT  jrd_52 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_51;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ c o n s t r a i n t _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$RELATION_CONSTRAINTS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	bool found = false;

	do
	{
		buffer.printf("INTEG_%" SQUADFORMAT,
				DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_con, "RDB$CONSTRAINT_NAME"));

		AutoCacheRequest request(tdbb, drq_f_nxt_con, DYN_REQUESTS);

		found = false;
		/*FOR(REQUEST_HANDLE request)
			FIRST 1 X IN RDB$RELATION_CONSTRAINTS
			WITH X.RDB$CONSTRAINT_NAME EQ buffer.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_50, sizeof(jrd_50));
		gds__vtov ((const char*) buffer.c_str(), (char*) jrd_51.jrd_52, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_51);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_53);
		   if (!jrd_53.jrd_54) break;
		{
			found = true;
		}
		/*END_FOR*/
		   }
		}
	} while (found);
}


void DYN_UTIL_generate_field_name(thread_db* tdbb, TEXT* buffer)
{
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Stub to make it work with char* too.
 *
 **************************************/
	Firebird::MetaName temp;
	DYN_UTIL_generate_field_name(tdbb, temp);
	strcpy(buffer, temp.c_str());
}


void DYN_UTIL_generate_field_name(thread_db* tdbb, Firebird::MetaName& buffer)
{
   struct {
          SSHORT jrd_49;	// gds__utility 
   } jrd_48;
   struct {
          TEXT  jrd_47 [32];	// RDB$FIELD_NAME 
   } jrd_46;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$FIELDS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	bool found = false;

	do
	{
		buffer.printf("RDB$%" SQUADFORMAT,
				DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_fld, "RDB$FIELD_NAME"));

		AutoCacheRequest request(tdbb, drq_f_nxt_fld, DYN_REQUESTS);

		found = false;
		/*FOR(REQUEST_HANDLE request)
			FIRST 1 X IN RDB$FIELDS
			WITH X.RDB$FIELD_NAME EQ buffer.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_45, sizeof(jrd_45));
		gds__vtov ((const char*) buffer.c_str(), (char*) jrd_46.jrd_47, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_46);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_48);
		   if (!jrd_48.jrd_49) break;
		{
			found = true;
		}
		/*END_FOR*/
		   }
		}
	} while (found);
}


void DYN_UTIL_generate_field_position(thread_db* tdbb, const MetaName& relation_name,
	SLONG* field_pos)
{
   struct {
          SSHORT jrd_42;	// gds__utility 
          SSHORT jrd_43;	// gds__null_flag 
          SSHORT jrd_44;	// RDB$FIELD_POSITION 
   } jrd_41;
   struct {
          TEXT  jrd_40 [32];	// RDB$RELATION_NAME 
   } jrd_39;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ f i e l d _ p o s i t i o n
 *
 **************************************
 *
 * Functional description
 *	Generate a field position if not specified
 *
 **************************************/
	SLONG field_position = -1;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, drq_l_fld_pos, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		X IN RDB$RELATION_FIELDS
		WITH X.RDB$RELATION_NAME EQ relation_name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_38, sizeof(jrd_38));
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_39.jrd_40, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_39);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_41);
	   if (!jrd_41.jrd_42) break;
	{
		if (/*X.RDB$FIELD_POSITION.NULL*/
		    jrd_41.jrd_43)
			continue;

		field_position = MAX(/*X.RDB$FIELD_POSITION*/
				     jrd_41.jrd_44, field_position);
	}
	/*END_FOR*/
	   }
	}

	*field_pos = field_position;
}


void DYN_UTIL_generate_index_name(thread_db* tdbb, jrd_tra* /*transaction*/,
								  Firebird::MetaName& buffer, UCHAR verb)
{
   struct {
          SSHORT jrd_37;	// gds__utility 
   } jrd_36;
   struct {
          TEXT  jrd_35 [32];	// RDB$INDEX_NAME 
   } jrd_34;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ i n d e x _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$INDICES.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	bool found = false;

	do
	{
		const SCHAR* format;
		switch (verb)
		{
			case isc_dyn_def_primary_key:
				format = "RDB$PRIMARY%" SQUADFORMAT;
				break;
			case isc_dyn_def_foreign_key:
				format = "RDB$FOREIGN%" SQUADFORMAT;
				break;
			default:
				format = "RDB$%" SQUADFORMAT;
		}

		buffer.printf(format,
			DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_idx, "RDB$INDEX_NAME"));

		AutoCacheRequest request(tdbb, drq_f_nxt_idx, DYN_REQUESTS);

		found = false;
		/*FOR(REQUEST_HANDLE request)
			FIRST 1 X IN RDB$INDICES
			WITH X.RDB$INDEX_NAME EQ buffer.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_33, sizeof(jrd_33));
		gds__vtov ((const char*) buffer.c_str(), (char*) jrd_34.jrd_35, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_34);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_36);
		   if (!jrd_36.jrd_37) break;
		{
			found = true;
		}
		/*END_FOR*/
		   }
		}
	} while (found);
}


// Generate a name unique to RDB$GENERATORS.
void DYN_UTIL_generate_generator_name(thread_db* tdbb, Firebird::MetaName& buffer)
{
   struct {
          SSHORT jrd_32;	// gds__utility 
   } jrd_31;
   struct {
          TEXT  jrd_30 [32];	// RDB$GENERATOR_NAME 
   } jrd_29;
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, drq_f_nxt_gen, DYN_REQUESTS);
	bool found = false;

	do
	{
		buffer.printf("RDB$%" SQUADFORMAT,
			DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_gen, "RDB$GENERATOR_NAME"));

		found = false;

		/*FOR (REQUEST_HANDLE request)
			FIRST 1 X IN RDB$GENERATORS WITH X.RDB$GENERATOR_NAME EQ buffer.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_28, sizeof(jrd_28));
		gds__vtov ((const char*) buffer.c_str(), (char*) jrd_29.jrd_30, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_29);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_31);
		   if (!jrd_31.jrd_32) break;
		{
			found = true;
		}
		/*END_FOR*/
		   }
		}
	} while (found);
}


void DYN_UTIL_generate_trigger_name(thread_db* tdbb, jrd_tra* /*transaction*/, MetaName& buffer)
{
   struct {
          SSHORT jrd_27;	// gds__utility 
   } jrd_26;
   struct {
          TEXT  jrd_25 [32];	// RDB$TRIGGER_NAME 
   } jrd_24;
/**************************************
 *
 *	D Y N _ U T I L _ g e n e r a t e _ t r i g g e r _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Generate a name unique to RDB$TRIGGERS.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	bool found = false;

	do
	{
		buffer.printf("CHECK_%" SQUADFORMAT,
			DYN_UTIL_gen_unique_id(tdbb, drq_g_nxt_trg, "RDB$TRIGGER_NAME"));

		AutoCacheRequest request(tdbb, drq_f_nxt_trg, DYN_REQUESTS);

		found = false;
		/*FOR(REQUEST_HANDLE request)
			FIRST 1 X IN RDB$TRIGGERS
			WITH X.RDB$TRIGGER_NAME EQ buffer.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_23, sizeof(jrd_23));
		gds__vtov ((const char*) buffer.c_str(), (char*) jrd_24.jrd_25, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_24);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_26);
		   if (!jrd_26.jrd_27) break;
		{
			found = true;
		}
		/*END_FOR*/
		   }
		}
	} while (found);
}


bool DYN_UTIL_find_field_source(thread_db* tdbb,
								jrd_tra* transaction,
								const Firebird::MetaName& view_name,
								USHORT context,
								const TEXT* local_name,
								TEXT* output_field_name)
{
   struct {
          TEXT  jrd_11 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_12;	// gds__utility 
   } jrd_10;
   struct {
          TEXT  jrd_6 [32];	// RDB$PARAMETER_NAME 
          TEXT  jrd_7 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_8;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_9;	// RDB$VIEW_CONTEXT 
   } jrd_5;
   struct {
          TEXT  jrd_21 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_22;	// gds__utility 
   } jrd_20;
   struct {
          TEXT  jrd_15 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_16 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_17;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_18;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_19;	// RDB$VIEW_CONTEXT 
   } jrd_14;
/**************************************
 *
 *	D Y N _ U T I L _ f i n d _ f i e l d _ s o u r c e
 *
 **************************************
 *
 * Functional description
 *	Find the original source for a view field.
 *
 **************************************/
	SET_TDBB(tdbb);

	AutoCacheRequest request(tdbb, drq_l_fld_src2, DYN_REQUESTS);
	bool found = false;

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		VRL IN RDB$VIEW_RELATIONS CROSS
			RFR IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
			WITH VRL.RDB$VIEW_NAME EQ view_name.c_str() AND
			VRL.RDB$VIEW_CONTEXT EQ context AND
			VRL.RDB$CONTEXT_TYPE BETWEEN VCT_TABLE AND VCT_VIEW AND
			VRL.RDB$PACKAGE_NAME MISSING AND
			RFR.RDB$FIELD_NAME EQ local_name*/
	{
	request.compile(tdbb, (UCHAR*) jrd_13, sizeof(jrd_13));
	gds__vtov ((const char*) local_name, (char*) jrd_14.jrd_15, 32);
	gds__vtov ((const char*) view_name.c_str(), (char*) jrd_14.jrd_16, 32);
	jrd_14.jrd_17 = VCT_VIEW;
	jrd_14.jrd_18 = VCT_TABLE;
	jrd_14.jrd_19 = context;
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 70, (UCHAR*) &jrd_14);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_20);
	   if (!jrd_20.jrd_22) break;
	{
		found = true;
		fb_utils::exact_name_limit(/*RFR.RDB$FIELD_SOURCE*/
					   jrd_20.jrd_21, sizeof(/*RFR.RDB$FIELD_SOURCE*/
	 jrd_20.jrd_21));
		strcpy(output_field_name, /*RFR.RDB$FIELD_SOURCE*/
					  jrd_20.jrd_21);
	}
	/*END_FOR*/
	   }
	}

	if (!found)
	{
		request.reset(tdbb, drq_l_fld_src3, DYN_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			VRL IN RDB$VIEW_RELATIONS CROSS
				PPR IN RDB$PROCEDURE_PARAMETERS
				WITH VRL.RDB$RELATION_NAME EQ PPR.RDB$PROCEDURE_NAME AND
				VRL.RDB$VIEW_NAME EQ view_name.c_str() AND
				VRL.RDB$VIEW_CONTEXT EQ context AND
				VRL.RDB$CONTEXT_TYPE EQ VCT_PROCEDURE AND
				PPR.RDB$PACKAGE_NAME EQUIV VRL.RDB$PACKAGE_NAME AND
				PPR.RDB$PARAMETER_TYPE = 1 AND // output
				PPR.RDB$PARAMETER_NAME EQ local_name*/
		{
		request.compile(tdbb, (UCHAR*) jrd_4, sizeof(jrd_4));
		gds__vtov ((const char*) local_name, (char*) jrd_5.jrd_6, 32);
		gds__vtov ((const char*) view_name.c_str(), (char*) jrd_5.jrd_7, 32);
		jrd_5.jrd_8 = VCT_PROCEDURE;
		jrd_5.jrd_9 = context;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_5);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_10);
		   if (!jrd_10.jrd_12) break;
		{
			found = true;
			fb_utils::exact_name_limit(/*PPR.RDB$FIELD_SOURCE*/
						   jrd_10.jrd_11, sizeof(/*PPR.RDB$FIELD_SOURCE*/
	 jrd_10.jrd_11));
			strcpy(output_field_name, /*PPR.RDB$FIELD_SOURCE*/
						  jrd_10.jrd_11);
		}
		/*END_FOR*/
		   }
		}
	}

	return found;
}


void DYN_UTIL_store_check_constraints(thread_db* tdbb, jrd_tra* transaction,
	const MetaName& constraint_name, const MetaName& trigger_name)
{
   struct {
          TEXT  jrd_2 [32];	// RDB$TRIGGER_NAME 
          TEXT  jrd_3 [32];	// RDB$CONSTRAINT_NAME 
   } jrd_1;
/**************************************
 *
 *	D Y N _ U T I L _ s t o r e _ c h e c k _ c o n s t r a i n t s
 *
 **************************************
 *
 * Functional description
 *	Fill in rdb$check_constraints the association between a check name and the
 *	system defined trigger that implements that check.
 *
 **************************************/
	SET_TDBB(tdbb);

	AutoCacheRequest request(tdbb, drq_s_chk_con, DYN_REQUESTS);

	/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		CHK IN RDB$CHECK_CONSTRAINTS*/
	{
	
	{
        strcpy(/*CHK.RDB$CONSTRAINT_NAME*/
	       jrd_1.jrd_3, constraint_name.c_str());
		strcpy(/*CHK.RDB$TRIGGER_NAME*/
		       jrd_1.jrd_2, trigger_name.c_str());
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_1);
	}
}
