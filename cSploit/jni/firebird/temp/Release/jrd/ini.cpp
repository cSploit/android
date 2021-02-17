/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ini.epp
 *	DESCRIPTION:	Metadata initialization / population
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
#include <string.h>
#include "../jrd/flags.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/ibase.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "gen/ids.h"
#include "../jrd/intl.h"
#include "../jrd/tra.h"
#include "../jrd/trig.h"
#include "../jrd/intl.h"
#include "../jrd/dflt.h"
#include "../jrd/ini.h"
#include "../jrd/idx.h"
#include "../common/gdsassert.h"
#include "../dsql/dsql.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/ini_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/obj.h"
#include "../jrd/acl.h"
#include "../jrd/dyn.h"
#include "../jrd/irq.h"
#include "../jrd/IntlManager.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/constants.h"

using namespace Firebird;
using namespace Jrd;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_quad, 0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 12,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 6,0, 5,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_10 [107] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 5,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 4,0, 
               blr_fid, 0, 13,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_20 [56] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 0,0, 0,4, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 17,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_25 [130] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 12,0, 
      blr_quad, 0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 29,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 4,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 6,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 8,0, 7,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 9,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 10,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 11,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_39 [100] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 28,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 3,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 4,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 6,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_49 [202] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 22,0, 
      blr_quad, 0, 
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
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 2,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 4,0, 3,0, 
               blr_fid, 0, 23,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 10,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 6,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 8,0, 7,0, 
               blr_fid, 0, 24,0, 
            blr_assignment, 
               blr_parameter2, 0, 10,0, 9,0, 
               blr_fid, 0, 17,0, 
            blr_assignment, 
               blr_parameter2, 0, 12,0, 11,0, 
               blr_fid, 0, 25,0, 
            blr_assignment, 
               blr_parameter2, 0, 14,0, 13,0, 
               blr_fid, 0, 26,0, 
            blr_assignment, 
               blr_parameter2, 0, 16,0, 15,0, 
               blr_fid, 0, 11,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 17,0, 
               blr_fid, 0, 29,0, 
            blr_assignment, 
               blr_parameter2, 0, 19,0, 18,0, 
               blr_fid, 0, 15,0, 
            blr_assignment, 
               blr_parameter, 0, 20,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter, 0, 21,0, 
               blr_fid, 0, 8,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_73 [116] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 11,0, 
      blr_quad, 0, 
      blr_int64, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 20,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 5,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 6,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 7,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 9,0, 8,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 10,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_86 [92] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 18,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_94 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 20,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_106 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 30,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_118 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 29,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 9,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_130 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 28,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 9,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_142 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
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
               blr_rid, 2,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 28,0, 
                        blr_parameter2, 1, 0,0, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 28,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_154 [42] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 9,0, 0, 
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
static const UCHAR	jrd_158 [124] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 9,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_cstring2, 0,0, 7,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 18,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 6,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 7,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 8,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_169 [171] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 0,0, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 1,0, 4,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
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
                                    blr_parameter2, 2, 1,0, 3,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 1, 9,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_185 [42] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 9,0, 0, 
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
static const UCHAR	jrd_189 [42] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 9,0, 0, 
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
static const UCHAR	jrd_193 [101] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 4,0, 
      blr_cstring2, 0,0, 4,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 22,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 5,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_201 [56] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 3,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 2,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_206 [119] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 10,0, 
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
   blr_receive, 0, 
      blr_store, 
         blr_rid, 4,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter, 0, 2,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 3,0, 
               blr_fid, 0, 6,0, 
            blr_assignment, 
               blr_parameter2, 0, 5,0, 4,0, 
               blr_fid, 0, 9,0, 
            blr_assignment, 
               blr_parameter2, 0, 7,0, 6,0, 
               blr_fid, 0, 7,0, 
            blr_assignment, 
               blr_parameter, 0, 8,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter, 0, 9,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 0,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 1,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_218 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 11,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_225 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 11,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_232 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 11,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 2,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter, 0, 4,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter, 0, 0,0, 
               blr_fid, 0, 2,0, 
            blr_assignment, 
               blr_parameter, 0, 1,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_239 [68] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 31,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 3,0, 2,0, 
               blr_fid, 0, 3,0, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 4,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 5,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


#define PAD(string, field) jrd_vtof ((char*)(string), field, sizeof (field))
const int FB_MAX_ACL_SIZE	= 4096;


static void add_index_set(thread_db*);
static void add_security_to_sys_obj(thread_db*, const MetaName&, USHORT, const MetaName&,
	USHORT = 0, const UCHAR* = NULL);
static void add_security_to_sys_rel(thread_db*, const MetaName&,
	const TEXT*, const USHORT, const UCHAR*);
static void store_generator(thread_db*, const gen*, AutoRequest&, const MetaName&);
static void store_global_field(thread_db*, const gfld*, AutoRequest&, const MetaName&);
static void store_intlnames(thread_db*, const MetaName&);
static void store_message(thread_db*, const trigger_msg*, AutoRequest&);
static void store_relation_field(thread_db*, const int*, const int*, int, AutoRequest&);
static void store_trigger(thread_db*, const jrd_trg*, AutoRequest&);


// This is the table used in defining triggers; note that
// RDB$TRIGGER_0 was first changed to RDB$TRIGGER_7 to make it easier to
// upgrade a database to support field-level grant.  It has since been
// changed to RDB$TRIGGER_9 to handle SQL security on relations whose
// name is > 27 characters

static const jrd_trg triggers[] =
{
	{ "RDB$TRIGGER_1", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger3), trigger3,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_8", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger2), trigger2,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_9", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger1), trigger1,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_2", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger4), trigger4,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_3", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger4), trigger4,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_26", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger26), trigger26,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_25", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger25),
		trigger25, 0, ODS_8_0 },
	{ "RDB$TRIGGER_10", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger10), trigger10,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_11", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger11),
		trigger11, 0, ODS_8_0 },
	{ "RDB$TRIGGER_12", (UCHAR) nam_ref_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger12), trigger12,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_13", (UCHAR) nam_ref_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger13),
		trigger13, 0, ODS_8_0 },
	{ "RDB$TRIGGER_14", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger14),
		trigger14, 0, ODS_8_0 },
	{ "RDB$TRIGGER_15", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger15), trigger15,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_16", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger16),
		trigger16, 0, ODS_8_0 },
	{ "RDB$TRIGGER_17", (UCHAR) nam_i_segments,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger17), trigger17,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_18", (UCHAR) nam_i_segments,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger18),
		trigger18, 0, ODS_8_0 },
	{ "RDB$TRIGGER_19", (UCHAR) nam_indices,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger19), trigger19,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_20", (UCHAR) nam_indices,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger20),
		trigger20, 0, ODS_8_0 },
	{ "RDB$TRIGGER_21", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger21), trigger21,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_22", (UCHAR) nam_trgs,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger22),
		trigger22, 0, ODS_8_0 },
	{ "RDB$TRIGGER_23", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger23), trigger23,
		0, ODS_8_0 },
	{ "RDB$TRIGGER_24", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger24),
		trigger24, 0, ODS_8_0 },
	{ "RDB$TRIGGER_27", (UCHAR) nam_r_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger27),
		trigger27, 0, ODS_8_0 },
	{ "RDB$TRIGGER_31", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger31),
		trigger31, 0, ODS_8_1 },
	{ "RDB$TRIGGER_32", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_ERASE*/
		5, sizeof(trigger31), trigger31,
		0, ODS_8_1 },
	{ "RDB$TRIGGER_33", (UCHAR) nam_user_privileges,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_STORE*/
		1, sizeof(trigger31), trigger31,
		0, ODS_8_1 },
	{ "RDB$TRIGGER_34", (UCHAR) nam_rel_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger34),
		trigger34, TRG_ignore_perm, ODS_8_1 },
	{ "RDB$TRIGGER_35", (UCHAR) nam_chk_constr,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.POST_ERASE*/
		6, sizeof(trigger35),
		trigger35, TRG_ignore_perm, ODS_8_1 },
	{ "RDB$TRIGGER_36", (UCHAR) nam_fields,
		/*RDB$TRIGGERS.RDB$TRIGGER_TYPE.PRE_MODIFY*/
		3, sizeof(trigger36),
		trigger36, 0, ODS_11_0 },
	{ 0, 0, 0, 0, 0, 0 }
};


// this table is used in defining messages for system triggers

static const trigger_msg trigger_messages[] =
{
	{ "RDB$TRIGGER_9", 0, "grant_obj_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_9", 1, "grant_fld_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_9", 2, "grant_nopriv", ODS_8_0 },
	{ "RDB$TRIGGER_9", 3, "nonsql_security_rel", ODS_8_0 },
	{ "RDB$TRIGGER_9", 4, "nonsql_security_fld", ODS_8_0 },
	{ "RDB$TRIGGER_9", 5, "grant_nopriv_on_base", ODS_8_0 },
	{ "RDB$TRIGGER_1", 0, "existing_priv_mod", ODS_8_0 },
	{ "RDB$TRIGGER_2", 0, "systrig_update", ODS_8_0 },
	{ "RDB$TRIGGER_3", 0, "systrig_update", ODS_8_0 },
	{ "RDB$TRIGGER_24", 1, "cnstrnt_fld_rename", ODS_8_0 },
	{ "RDB$TRIGGER_23", 1, "cnstrnt_fld_del", ODS_8_0 },
	{ "RDB$TRIGGER_22", 1, "check_trig_update", ODS_8_0 },
	{ "RDB$TRIGGER_21", 1, "check_trig_del", ODS_8_0 },
	{ "RDB$TRIGGER_20", 1, "integ_index_mod", ODS_8_0 },
	{ "RDB$TRIGGER_20", 2, "integ_index_deactivate", ODS_8_0 },
	{ "RDB$TRIGGER_20", 3, "integ_deactivate_primary", ODS_8_0 },
	{ "RDB$TRIGGER_19", 1, "integ_index_del", ODS_8_0 },
	{ "RDB$TRIGGER_18", 1, "integ_index_seg_mod", ODS_8_0 },
	{ "RDB$TRIGGER_17", 1, "integ_index_seg_del", ODS_8_0 },
	{ "RDB$TRIGGER_15", 1, "check_cnstrnt_del", ODS_8_0 },
	{ "RDB$TRIGGER_14", 1, "check_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_13", 1, "ref_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_12", 1, "ref_cnstrnt_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_12", 2, "foreign_key_notfound", ODS_8_0 },
	{ "RDB$TRIGGER_10", 1, "primary_key_ref", ODS_8_0 },
	{ "RDB$TRIGGER_10", 2, "primary_key_notnull", ODS_8_0 },
	{ "RDB$TRIGGER_25", 1, "rel_cnstrnt_update", ODS_8_0 },
	{ "RDB$TRIGGER_26", 1, "constaint_on_view", ODS_8_0 },
	{ "RDB$TRIGGER_26", 2, "invld_cnstrnt_type", ODS_8_0 },
	{ "RDB$TRIGGER_26", 3, "primary_key_exists", ODS_8_0 },
	{ "RDB$TRIGGER_31", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_32", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_33", 0, "no_write_user_priv", ODS_8_1 },
	{ "RDB$TRIGGER_24", 2, "integ_index_seg_mod", ODS_11_0 },
	{ "RDB$TRIGGER_36", 1, "integ_index_seg_mod", ODS_11_0 },
	{ 0, 0, 0, 0 }
};



void INI_format(const MetaName& owner, const MetaName& charset)
{
   struct {
          TEXT  jrd_220 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_221 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_222;	// gds__null_flag 
          SSHORT jrd_223;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_224;	// RDB$TYPE 
   } jrd_219;
   struct {
          TEXT  jrd_227 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_228 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_229;	// gds__null_flag 
          SSHORT jrd_230;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_231;	// RDB$TYPE 
   } jrd_226;
   struct {
          TEXT  jrd_234 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_235 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_236;	// gds__null_flag 
          SSHORT jrd_237;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_238;	// RDB$TYPE 
   } jrd_233;
   struct {
          TEXT  jrd_241 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_242 [32];	// RDB$ROLE_NAME 
          SSHORT jrd_243;	// gds__null_flag 
          SSHORT jrd_244;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_245;	// gds__null_flag 
          SSHORT jrd_246;	// gds__null_flag 
   } jrd_240;
/**************************************
 *
 *	I N I _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Initialize system relations in the database.
 *	The full complement of metadata should be
 *	stored here.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Jrd::Attachment* attachment = tdbb->getAttachment();

	fb_assert(owner.hasData());

	// Uppercase owner name
	MetaName ownerName(owner);
	ownerName.upper7();

	// Uppercase charset name
	MetaName rdbCharSetName(charset.hasData() ? charset.c_str() : DEFAULT_DB_CHARACTER_SET_NAME);
	rdbCharSetName.upper7();

	const int* fld;

	// Make sure relations exist already

	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		if (relfld[RFLD_R_TYPE] == rel_persistent)
		{
			DPM_create_relation(tdbb, MET_relation(tdbb, relfld[RFLD_R_ID]));
		}

		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
			;
	}

	jrd_tra* transaction = attachment->getSysTransaction();

	// Store RELATIONS and RELATION_FIELDS

	SLONG rdbRelationId;
	MetaName rdbRelationName;
	SLONG rdbFieldId;
	SLONG rdbSystemFlag = RDB_system;
	SLONG rdbRelationType;

	PreparedStatement::Builder sql;
	sql << "insert into rdb$relations (rdb$relation_id, rdb$relation_name, rdb$field_id,"
		<< "rdb$format, rdb$system_flag, rdb$dbkey_length, rdb$owner_name, rdb$relation_type)"
		<< "values (" << rdbRelationId << ", " << rdbRelationName << ", " << rdbFieldId << ", 0,"
		<< rdbSystemFlag << ", 8, " << ownerName << ", " << rdbRelationType << ")";

	AutoPreparedStatement ps(attachment->prepareStatement(tdbb, transaction, sql));

	AutoRequest handle1;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		for (rdbFieldId = 0, fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
		{
			const int* pFld = fld;
			const int* pRelFld = relfld;
			store_relation_field(tdbb, pFld, pRelFld, rdbFieldId, handle1);
			++rdbFieldId;
		}

		rdbRelationId = relfld[RFLD_R_ID];
		rdbRelationName = names[relfld[RFLD_R_NAME]];
		rdbRelationType = relfld[RFLD_R_TYPE];
		ps->execute(tdbb, transaction);
	}

	handle1.reset();

	// Store global FIELDS

	for (const gfld* gfield = gfields; gfield->gfld_name; gfield++)
		store_global_field(tdbb, gfield, handle1, ownerName);

	// Store DATABASE record

	sql = PreparedStatement::Builder();
	sql << "insert into rdb$database (rdb$relation_id, rdb$character_set_name) values ("
		<< rdbRelationId << ", " << rdbCharSetName << ")";

	ps.reset(attachment->prepareStatement(tdbb, transaction, sql));

	rdbRelationId = USER_DEF_REL_INIT_ID;
	ps->execute(tdbb, transaction);

	// Store ADMIN_ROLE record

	handle1.reset();

	/*STORE(REQUEST_HANDLE handle1) X IN RDB$ROLES*/
	{
	
	{
		PAD (ADMIN_ROLE, /*X.RDB$ROLE_NAME*/
				 jrd_240.jrd_242);
		/*X.RDB$ROLE_NAME.NULL*/
		jrd_240.jrd_246 = FALSE;
		if (ownerName.hasData())
			PAD(ownerName.c_str(), /*X.RDB$OWNER_NAME*/
					       jrd_240.jrd_241);
		else
			PAD(SYSDBA_USER_NAME, /*X.RDB$OWNER_NAME*/
					      jrd_240.jrd_241);
		/*X.RDB$OWNER_NAME.NULL*/
		jrd_240.jrd_245 = FALSE;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_240.jrd_244 = ROLE_FLAG_DBO;	// Mark it as privileged role
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_240.jrd_243 = FALSE;
	}
	/*END_STORE*/
	handle1.compile(tdbb, (UCHAR*) jrd_239, sizeof(jrd_239));
	EXE_start (tdbb, handle1, attachment->getSysTransaction());
	EXE_send (tdbb, handle1, 0, 72, (UCHAR*) &jrd_240);
	}

	// Create indices for system relations
	add_index_set(tdbb);

	// Create parameter types

	handle1.reset();

	for (const rtyp* type = types; type->rtyp_name; ++type)
	{
		// this STORE should be compatible with two below,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
		{
			PAD(names[type->rtyp_field], /*X.RDB$FIELD_NAME*/
						     jrd_233.jrd_235);
			PAD(type->rtyp_name, /*X.RDB$TYPE_NAME*/
					     jrd_233.jrd_234);
			/*X.RDB$TYPE*/
			jrd_233.jrd_238 = type->rtyp_value;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_233.jrd_237 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_233.jrd_236 = FALSE;
		}
		/*END_STORE*/
		handle1.compile(tdbb, (UCHAR*) jrd_232, sizeof(jrd_232));
		EXE_start (tdbb, handle1, attachment->getSysTransaction());
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_233);
		}
	}

	for (const IntlManager::CharSetDefinition* charSet = IntlManager::defaultCharSets;
		 charSet->name; ++charSet)
	{
		// this STORE should be compatible with one above and below,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
			PAD(names[nam_charset_name], /*X.RDB$FIELD_NAME*/
						     jrd_226.jrd_228);
			PAD(charSet->name, /*X.RDB$TYPE_NAME*/
					   jrd_226.jrd_227);
			/*X.RDB$TYPE*/
			jrd_226.jrd_231 = charSet->id;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_226.jrd_230 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_226.jrd_229 = FALSE;
		/*END_STORE;*/
		handle1.compile(tdbb, (UCHAR*) jrd_225, sizeof(jrd_225));
		EXE_start (tdbb, handle1, attachment->getSysTransaction());
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_226);
		}
	}

	for (const IntlManager::CharSetAliasDefinition* alias = IntlManager::defaultCharSetAliases;
		alias->name; ++alias)
	{
		// this STORE should be compatible with two above,
		// as they use the same compiled request
		/*STORE(REQUEST_HANDLE handle1) X IN RDB$TYPES*/
		{
		
			PAD(names[nam_charset_name], /*X.RDB$FIELD_NAME*/
						     jrd_219.jrd_221);
			PAD(alias->name, /*X.RDB$TYPE_NAME*/
					 jrd_219.jrd_220);
			/*X.RDB$TYPE*/
			jrd_219.jrd_224 = alias->charSetId;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_219.jrd_223 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_219.jrd_222 = FALSE;
		/*END_STORE;*/
		handle1.compile(tdbb, (UCHAR*) jrd_218, sizeof(jrd_218));
		EXE_start (tdbb, handle1, attachment->getSysTransaction());
		EXE_send (tdbb, handle1, 0, 70, (UCHAR*) &jrd_219);
		}
	}

	// Store symbols for international character sets & collations
	store_intlnames(tdbb, ownerName);

	// Create generators to be used by system triggers

	handle1.reset();

	for (const gen* generator = generators; generator->gen_name; generator++)
		store_generator(tdbb, generator, handle1, ownerName);

	// Adjust the value of the hidden generator RDB$GENERATORS
	DPM_gen_id(tdbb, 0, true, FB_NELEM(generators) - 1);

	// store system-defined triggers

	handle1.reset();

	for (const jrd_trg* trigger = triggers; trigger->trg_relation; ++trigger)
		store_trigger(tdbb, trigger, handle1);

	// store trigger messages to go with triggers

	handle1.reset();

	for (const trigger_msg* message = trigger_messages; message->trigmsg_name; ++message)
		store_message(tdbb, message, handle1);

	DFW_perform_system_work(tdbb);

	const size_t ownerNameLength = ownerName.length();
	fb_assert(ownerNameLength <= MAX_UCHAR);

	// Add security for the non-relation system metadata objects

	const UCHAR NON_REL_OWNER_ACL[] =
		{ACL_priv_list, priv_control, priv_alter, priv_drop, priv_usage, ACL_end};

	const UCHAR NON_REL_PUBLIC_ACL[] =
		{ACL_priv_list, priv_usage, ACL_end};

	UCHAR buffer[FB_MAX_ACL_SIZE];
	UCHAR* acl = buffer;
	*acl++ = ACL_version;
	*acl++ = ACL_id_list;
	*acl++ = id_person;

	*acl++ = (UCHAR) ownerNameLength;
	memcpy(acl, ownerName.c_str(), ownerNameLength);
	acl += ownerNameLength;

	*acl++ = ACL_end;

	memcpy(acl, NON_REL_OWNER_ACL, sizeof(NON_REL_OWNER_ACL));
	acl += sizeof(NON_REL_OWNER_ACL);

	*acl++ = ACL_id_list;
	*acl++ = ACL_end;
	memcpy(acl, NON_REL_PUBLIC_ACL, sizeof(NON_REL_PUBLIC_ACL));
	acl += sizeof(NON_REL_PUBLIC_ACL);
	*acl++ = ACL_end; // Put an extra terminator to avoid scl.epp:walk_acl() missing the end.

	USHORT length = acl - buffer;

	for (const gfld* gfield = gfields; gfield->gfld_name; gfield++)
	{
		add_security_to_sys_obj(tdbb, ownerName, obj_field,
			names[(USHORT) gfield->gfld_name], length, buffer);
	}

	for (const gen* generator = generators; generator->gen_name; generator++)
	{
		add_security_to_sys_obj(tdbb, ownerName, obj_generator,
			generator->gen_name, length, buffer);
	}

	for (const IntlManager::CharSetDefinition* charset = IntlManager::defaultCharSets;
		 charset->name;
		 ++charset)
	{
		add_security_to_sys_obj(tdbb, ownerName, obj_charset, charset->name, length, buffer);
	}

	for (const IntlManager::CollationDefinition* collation = IntlManager::defaultCollations;
		 collation->name;
		 ++collation)
	{
		add_security_to_sys_obj(tdbb, ownerName, obj_collation, collation->name, length, buffer);
	}

	// Add security on system tables

	const UCHAR REL_OWNER_ACL[] =
		{ACL_priv_list, priv_control, priv_alter, priv_drop,
		 priv_select, priv_insert, priv_update, priv_delete, ACL_end};

	const UCHAR REL_PUBLIC_ACL[] =
		{ACL_priv_list, priv_select, ACL_end};

	acl = buffer;
	*acl++ = ACL_version;
	*acl++ = ACL_id_list;
	*acl++ = id_person;

	*acl++ = (UCHAR) ownerNameLength;
	memcpy(acl, ownerName.c_str(), ownerNameLength);
	acl += ownerNameLength;

	*acl++ = ACL_end;

	memcpy(acl, REL_OWNER_ACL, sizeof(REL_OWNER_ACL));
	acl += sizeof(REL_OWNER_ACL);

	*acl++ = ACL_id_list;
	*acl++ = ACL_end;
	memcpy(acl, REL_PUBLIC_ACL, sizeof(REL_PUBLIC_ACL));
	acl += sizeof(REL_PUBLIC_ACL);
	*acl++ = ACL_end; // Put an extra terminator to avoid scl.epp:walk_acl() missing the end.

	length = acl - buffer;

	add_security_to_sys_rel(tdbb, ownerName, "RDB$ROLES", length, buffer);
	add_security_to_sys_rel(tdbb, ownerName, "RDB$PAGES", length, buffer);
	// DFW writes here
	add_security_to_sys_rel(tdbb, ownerName, "RDB$FORMATS", length, buffer);
}


USHORT INI_get_trig_flags(const TEXT* trig_name)
{
/*********************************************
 *
 *      I N I _ g e t _ t r i g _ f l a g s
 *
 *********************************************
 *
 * Functional description
 *      Return the trigger flags for a system trigger.
 *
 **************************************/
	for (const jrd_trg* trig = triggers; trig->trg_length > 0; trig++)
	{
		if (!strcmp(trig->trg_name, trig_name))
		{
			return (trig->trg_flags);
		}
	}

	return (0);
}


void INI_init(thread_db* tdbb)
{
/**************************************
 *
 *	I N I _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize in memory meta data.  Assume that all meta data
 *	fields exist in the database even if this is not the case.
 *	Do not fill in the format length or the address in each
 *	format descriptor.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	Jrd::Attachment* attachment = tdbb->getAttachment();
	MemoryPool* pool = attachment->att_pool;

	const int* fld;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		jrd_rel* relation = MET_relation(tdbb, relfld[RFLD_R_ID]);
		relation->rel_flags |= REL_system;
		relation->rel_flags |= MET_get_rel_flags_from_TYPE(relfld[RFLD_R_TYPE]);
		relation->rel_name = names[relfld[RFLD_R_NAME]];
		int n = 0;
		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
		{
			n++;
		}

		// Set a flag if their is a trigger on the relation.  Later we may need to compile it.

		for (const jrd_trg* trigger = triggers; trigger->trg_relation; trigger++)
		{
			if (relation->rel_name == names[trigger->trg_relation])
			{
				relation->rel_flags |= REL_sys_triggers;
				break;
			}
		}

		vec<jrd_fld*>* fields = vec<jrd_fld*>::newVector(*pool, n);
		relation->rel_fields = fields;
		vec<jrd_fld*>::iterator itr = fields->begin();
		Format* format = Format::newFormat(*pool, n);
		relation->rel_current_format = format;
		vec<Format*>* formats = vec<Format*>::newVector(*pool, 1);
		relation->rel_formats = formats;
		(*formats)[0] = format;
		Format::fmt_desc_iterator desc = format->fmt_desc.begin();

		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH, ++desc, ++itr)
		{
			const gfld* gfield = &gfields[fld[RFLD_F_ID]];
			desc->dsc_length = gfield->gfld_length;
			if (gfield->gfld_dtype == dtype_varying)
			{
				fb_assert(desc->dsc_length <= MAX_USHORT - sizeof(USHORT));
				desc->dsc_length += sizeof(USHORT);
			}

			desc->dsc_dtype = gfield->gfld_dtype;

			if (desc->isText())
			{
				if (gfield->gfld_sub_type & dsc_text_type_metadata)
					desc->dsc_sub_type = CS_METADATA;
				else
					desc->dsc_sub_type = CS_NONE;
			}
			else
				desc->dsc_sub_type = gfield->gfld_sub_type;

			if (desc->dsc_dtype == dtype_blob && desc->dsc_sub_type == isc_blob_text)
				desc->dsc_scale = CS_METADATA;	// blob charset

			jrd_fld* field = FB_NEW(*pool) jrd_fld(*pool);
			*itr = field;
			field->fld_name = names[fld[RFLD_F_NAME]];
		}
	}
}


void INI_init2(thread_db* tdbb)
{
/**************************************
 *
 *	I N I _ i n i t 2
 *
 **************************************
 *
 * Functional description
 *	Re-initialize in memory meta data.  Fill in
 *	format 0 based on the minor ODS version of
 *	the database when it was created.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_version = dbb->dbb_minor_version;
	vec<jrd_rel*>* vector = tdbb->getAttachment()->att_relations;

	const int* fld;
	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		if (relfld[RFLD_R_ODS] > ENCODE_ODS(major_version, minor_version))
		{
			// free the space allocated for RDB$ROLES

			const USHORT id = relfld[RFLD_R_ID];
			jrd_rel* relation = (*vector)[id];
			delete relation->rel_current_format;
			delete relation->rel_formats;
			delete relation->rel_fields;
			delete relation;
			(*vector)[id] = NULL;
			fld = relfld + RFLD_RPT;
			while (fld[RFLD_F_NAME])
			{
				fld += RFLD_F_LENGTH;
			}
		}
		else
		{
			jrd_rel* relation = MET_relation(tdbb, relfld[RFLD_R_ID]);
			Format* format = relation->rel_current_format;

			int n = 0;
			for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
			{
				if (ENCODE_ODS(major_version, minor_version) >= fld[RFLD_F_ODS])
					n++;
			}

			relation->rel_fields->resize(n);
			format->fmt_count = n; // We are using less than the allocated members.
			format->fmt_length = FLAG_BYTES(n);
			Format::fmt_desc_iterator desc = format->fmt_desc.begin();
			for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH, ++desc)
			{
				if (n-- > 0)
				{
					format->fmt_length = MET_align(&(*desc), format->fmt_length);
					desc->dsc_address = (UCHAR*)(IPTR) format->fmt_length;
					format->fmt_length += desc->dsc_length;
				}
			}
		}
	}
}


// Load system objects into DSQL metadata cache.
void INI_init_dsql(thread_db* tdbb, dsql_dbb* database)
{
	SET_TDBB(tdbb);

	Database* const dbb = tdbb->getDatabase();
	const USHORT majorVersion = dbb->dbb_ods_version;
	const USHORT minorVersion = dbb->dbb_minor_version;
	const int* fld;

	// Load relation and fields.

	for (const int* relfld = relfields; relfld[RFLD_R_NAME]; relfld = fld + 1)
	{
		if (relfld[RFLD_R_ODS] > ENCODE_ODS(majorVersion, minorVersion))
			continue;

		dsql_rel* relation = FB_NEW(database->dbb_pool) dsql_rel(database->dbb_pool);

		relation->rel_id = relfld[RFLD_R_ID];
		relation->rel_name = names[relfld[RFLD_R_NAME]];
		relation->rel_owner = SYSDBA_USER_NAME;
		relation->rel_dbkey_length = 8;

		dsql_fld** ptr = &relation->rel_fields;
		int n = 0;

		for (fld = relfld + RFLD_RPT; fld[RFLD_F_NAME]; fld += RFLD_F_LENGTH)
		{
			if (ENCODE_ODS(majorVersion, minorVersion) < fld[RFLD_F_ODS])
				continue;

			dsql_fld* field = FB_NEW(database->dbb_pool) dsql_fld(database->dbb_pool);
			field->fld_id = n++;

			*ptr = field;
			ptr = &field->fld_next;

			// get field information

			const gfld* gfield = &gfields[fld[RFLD_F_ID]];

			field->fld_name = names[fld[RFLD_F_NAME]];
			field->fieldSource = names[gfield->gfld_name];
			field->length = gfield->gfld_length;
			field->scale = 0;
			field->subType = gfield->gfld_sub_type;
			field->fld_relation = relation;

			field->dtype = gfield->gfld_dtype;

			if (field->dtype == dtype_varying)
				field->length += sizeof(USHORT);
			else if (field->dtype == dtype_blob)
			{
				field->segLength = 80;
				if (gfield->gfld_sub_type == isc_blob_text)
					field->charSetId = CS_METADATA;
			}

			if (DTYPE_IS_TEXT(gfield->gfld_dtype))
			{
				switch (gfield->gfld_sub_type)
				{
					case dsc_text_type_metadata:
						field->charSetId = CS_METADATA;
						break;
					case dsc_text_type_ascii:
						field->charSetId = CS_ASCII;
						break;
					case dsc_text_type_fixed:
						field->charSetId = CS_BINARY;
						break;
				}
			}

			if (gfield->gfld_nullable)
				field->flags |= FLD_nullable;

			field->flags |= FLD_system;
		}

		database->dbb_relations.put(relation->rel_name, relation);
		MET_dsql_cache_use(tdbb, SYM_relation, relation->rel_name);
	}

	// Load internal character sets and collations, necessary for engine operation.

	for (const IntlManager::CharSetDefinition* csDef = IntlManager::defaultCharSets;
		 csDef->name; ++csDef)
	{
		if (csDef->id > ttype_last_internal)
			continue;

		dsql_intlsym* csSymbol = FB_NEW(database->dbb_pool) dsql_intlsym(database->dbb_pool);
		csSymbol->intlsym_name = csDef->name;
		csSymbol->intlsym_charset_id = csDef->id;
		csSymbol->intlsym_collate_id = 0;
		csSymbol->intlsym_ttype =
			INTL_CS_COLL_TO_TTYPE(csSymbol->intlsym_charset_id, csSymbol->intlsym_collate_id);
		csSymbol->intlsym_bytes_per_char = csDef->maxBytes;

		// Mark the charset as invalid to reload it ASAP. This is done because we cannot know here
		// if the user has altered its default collation.
		csSymbol->intlsym_flags = INTLSYM_dropped;

		database->dbb_charsets.put(csDef->name, csSymbol);
		database->dbb_charsets_by_id.put(csSymbol->intlsym_charset_id, csSymbol);
		MET_dsql_cache_use(tdbb, SYM_intlsym_charset, csDef->name);

		for (const IntlManager::CollationDefinition* colDef = IntlManager::defaultCollations;
			 colDef->name; ++colDef)
		{
			if (colDef->charSetId != csDef->id)
				continue;

			dsql_intlsym* colSymbol = FB_NEW(database->dbb_pool) dsql_intlsym(database->dbb_pool);
			colSymbol->intlsym_name = colDef->name;
			colSymbol->intlsym_flags = 0;
			colSymbol->intlsym_charset_id = csDef->id;
			colSymbol->intlsym_collate_id = colDef->collationId;
			colSymbol->intlsym_ttype =
				INTL_CS_COLL_TO_TTYPE(colSymbol->intlsym_charset_id, colSymbol->intlsym_collate_id);
			colSymbol->intlsym_bytes_per_char = csDef->maxBytes;

			database->dbb_collations.put(colDef->name, colSymbol);
			MET_dsql_cache_use(tdbb, SYM_intlsym_collation, colDef->name);
		}
	}
}


static void add_index_set(thread_db* tdbb)
{
   struct {
          TEXT  jrd_195 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_196 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_197 [32];	// RDB$CONSTRAINT_NAME 
          TEXT  jrd_198 [4];	// RDB$INITIALLY_DEFERRED 
          TEXT  jrd_199 [4];	// RDB$DEFERRABLE 
          TEXT  jrd_200 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_194;
   struct {
          TEXT  jrd_203 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_204 [32];	// RDB$INDEX_NAME 
          SSHORT jrd_205;	// RDB$FIELD_POSITION 
   } jrd_202;
   struct {
          TEXT  jrd_208 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_209 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_210;	// RDB$INDEX_ID 
          SSHORT jrd_211;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_212;	// gds__null_flag 
          SSHORT jrd_213;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_214;	// gds__null_flag 
          SSHORT jrd_215;	// RDB$INDEX_TYPE 
          SSHORT jrd_216;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_217;	// RDB$UNIQUE_FLAG 
   } jrd_207;
/**************************************
 *
 *	a d d _ i n d e x _ s e t
 *
 **************************************
 *
 * Functional description
 *	Add system indices.  If update_ods is true we are performing
 *	an ODS update, and only add the indices marked as newer than
 *	ODS (major_version,minor_version).
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	index_desc idx;

	AutoRequest handle1, handle2, handle3;

	for (int n = 0; n < SYSTEM_INDEX_COUNT; n++)
	{
		const ini_idx_t* index = &indices[n];
		jrd_rel* relation = MET_relation(tdbb, index->ini_idx_relid);

		Firebird::MetaName indexName;
		indexName.printf("RDB$INDEX_%d", index->ini_idx_index_id);

		/*STORE(REQUEST_HANDLE handle1) X IN RDB$INDICES*/
		{
		
			PAD(relation->rel_name.c_str(), /*X.RDB$RELATION_NAME*/
							jrd_207.jrd_209);
			PAD(indexName.c_str(), /*X.RDB$INDEX_NAME*/
					       jrd_207.jrd_208);
			/*X.RDB$UNIQUE_FLAG*/
			jrd_207.jrd_217 = index->ini_idx_flags & idx_unique;
			/*X.RDB$SEGMENT_COUNT*/
			jrd_207.jrd_216 = index->ini_idx_segment_count;

			if (index->ini_idx_flags & idx_descending)
			{
				/*X.RDB$INDEX_TYPE.NULL*/
				jrd_207.jrd_214 = FALSE;
				/*X.RDB$INDEX_TYPE*/
				jrd_207.jrd_215 = 1;
			}
			else
				/*X.RDB$INDEX_TYPE.NULL*/
				jrd_207.jrd_214 = TRUE;

			/*X.RDB$SYSTEM_FLAG*/
			jrd_207.jrd_213 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_207.jrd_212 = FALSE;
			/*X.RDB$INDEX_INACTIVE*/
			jrd_207.jrd_211 = 0;

			// Store each segment for the index
			index_desc::idx_repeat* tail = idx.idx_rpt;
			for (USHORT position = 0; position < index->ini_idx_segment_count; position++, tail++)
			{
				const ini_idx_t::ini_idx_segment_t* segment = &index->ini_idx_segment[position];
				/*STORE(REQUEST_HANDLE handle2) Y IN RDB$INDEX_SEGMENTS*/
				{
				
					jrd_fld* field = (*relation->rel_fields)[segment->ini_idx_rfld_id];

					/*Y.RDB$FIELD_POSITION*/
					jrd_202.jrd_205 = position;
					PAD(/*X.RDB$INDEX_NAME*/
					    jrd_207.jrd_208, /*Y.RDB$INDEX_NAME*/
  jrd_202.jrd_204);
					PAD(field->fld_name.c_str(), /*Y.RDB$FIELD_NAME*/
								     jrd_202.jrd_203);
					tail->idx_field = segment->ini_idx_rfld_id;
					tail->idx_itype = segment->ini_idx_type;
				    tail->idx_selectivity = 0;
				/*END_STORE*/
				handle2.compile(tdbb, (UCHAR*) jrd_201, sizeof(jrd_201));
				EXE_start (tdbb, handle2, attachment->getSysTransaction());
				EXE_send (tdbb, handle2, 0, 66, (UCHAR*) &jrd_202);
				}
			}

			idx.idx_count = index->ini_idx_segment_count;
			idx.idx_flags = index->ini_idx_flags;
			SelectivityList selectivity(*tdbb->getDefaultPool());

			IDX_create_index(tdbb, relation, &idx, indexName.c_str(), NULL,
				attachment->getSysTransaction(), selectivity);

			/*X.RDB$INDEX_ID*/
			jrd_207.jrd_210 = idx.idx_id + 1;
		/*END_STORE*/
		handle1.compile(tdbb, (UCHAR*) jrd_206, sizeof(jrd_206));
		EXE_start (tdbb, handle1, attachment->getSysTransaction());
		EXE_send (tdbb, handle1, 0, 80, (UCHAR*) &jrd_207);
		}

		if (index->ini_idx_flags & idx_unique)
		{
			/*STORE(REQUEST_HANDLE handle3) RC IN RDB$RELATION_CONSTRAINTS*/
			{
			
				PAD(indexName.c_str(), /*RC.RDB$CONSTRAINT_NAME*/
						       jrd_194.jrd_197);
				PAD(indexName.c_str(), /*RC.RDB$INDEX_NAME*/
						       jrd_194.jrd_196);
				PAD(relation->rel_name.c_str(), /*RC.RDB$RELATION_NAME*/
								jrd_194.jrd_195);
				strcpy(/*RC.RDB$CONSTRAINT_TYPE*/
				       jrd_194.jrd_200, UNIQUE_CNSTRT);
				strcpy(/*RC.RDB$DEFERRABLE*/
				       jrd_194.jrd_199, "NO");
				strcpy(/*RC.RDB$INITIALLY_DEFERRED*/
				       jrd_194.jrd_198, "NO");
			/*END_STORE*/
			handle3.compile(tdbb, (UCHAR*) jrd_193, sizeof(jrd_193));
			EXE_start (tdbb, handle3, attachment->getSysTransaction());
			EXE_send (tdbb, handle3, 0, 116, (UCHAR*) &jrd_194);
			}
		}
	}
}


// The caller used an UCHAR* to store the acl, it was converted to TEXT* to
// be passed to this function, only to be converted to UCHAR* to be passed
// to BLB_put_segment. Therefore, "acl" was changed to UCHAR* as param.
static void add_security_to_sys_rel(thread_db*	tdbb,
									const Firebird::MetaName&	user_name,
									const TEXT*	rel_name,
									const USHORT	acl_length,
									const UCHAR*	acl)
{
   struct {
          TEXT  jrd_160 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_161 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_162 [32];	// RDB$GRANTOR 
          TEXT  jrd_163 [32];	// RDB$USER 
          SSHORT jrd_164;	// RDB$OBJECT_TYPE 
          SSHORT jrd_165;	// RDB$USER_TYPE 
          SSHORT jrd_166;	// gds__null_flag 
          SSHORT jrd_167;	// RDB$GRANT_OPTION 
          TEXT  jrd_168 [7];	// RDB$PRIVILEGE 
   } jrd_159;
   struct {
          SSHORT jrd_184;	// gds__utility 
   } jrd_183;
   struct {
          TEXT  jrd_179 [32];	// RDB$SECURITY_CLASS 
          TEXT  jrd_180 [32];	// RDB$DEFAULT_CLASS 
          SSHORT jrd_181;	// gds__null_flag 
          SSHORT jrd_182;	// gds__null_flag 
   } jrd_178;
   struct {
          TEXT  jrd_173 [32];	// RDB$DEFAULT_CLASS 
          TEXT  jrd_174 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_175;	// gds__utility 
          SSHORT jrd_176;	// gds__null_flag 
          SSHORT jrd_177;	// gds__null_flag 
   } jrd_172;
   struct {
          TEXT  jrd_171 [32];	// RDB$RELATION_NAME 
   } jrd_170;
   struct {
          bid  jrd_187;	// RDB$ACL 
          TEXT  jrd_188 [32];	// RDB$SECURITY_CLASS 
   } jrd_186;
   struct {
          bid  jrd_191;	// RDB$ACL 
          TEXT  jrd_192 [32];	// RDB$SECURITY_CLASS 
   } jrd_190;
/**************************************
 *
 *      a d d _ s e c u r i t y _ t o _ s y s _ r e l
 *
 **************************************
 *
 * Functional description
 *
 *      Add security to system relations. Only the owner of the
 *      database has SELECT/INSERT/UPDATE/DELETE privileges on
 *      any system relations. Any other users only has SELECT
 *      privilege.
 *
 **************************************/
	Firebird::MetaName security_class, default_class;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	bid blob_id_1;
	attachment->storeBinaryBlob(tdbb, attachment->getSysTransaction(), &blob_id_1,
		ByteChunk(acl, acl_length));

	bid blob_id_2;
	attachment->storeBinaryBlob(tdbb, attachment->getSysTransaction(), &blob_id_2,
		ByteChunk(acl, acl_length));

	security_class.printf("%s%" SQUADFORMAT, SQL_SECCLASS_PREFIX,
						  DPM_gen_id(tdbb, MET_lookup_generator(tdbb, SQL_SECCLASS_GENERATOR), false, 1));

	default_class.printf("%s%" SQUADFORMAT, DEFAULT_CLASS,
						 DPM_gen_id(tdbb, MET_lookup_generator(tdbb, DEFAULT_CLASS), false, 1));

	AutoRequest handle1;

	/*STORE(REQUEST_HANDLE handle1)
		CLS IN RDB$SECURITY_CLASSES*/
	{
	
	{
		PAD(security_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
					    jrd_190.jrd_192);
		/*CLS.RDB$ACL*/
		jrd_190.jrd_191 = blob_id_1;
	}
	/*END_STORE*/
	handle1.compile(tdbb, (UCHAR*) jrd_189, sizeof(jrd_189));
	EXE_start (tdbb, handle1, attachment->getSysTransaction());
	EXE_send (tdbb, handle1, 0, 40, (UCHAR*) &jrd_190);
	}

	handle1.reset();

	/*STORE(REQUEST_HANDLE handle1)
		CLS IN RDB$SECURITY_CLASSES*/
	{
	
	{
		PAD(default_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
					   jrd_186.jrd_188);
		/*CLS.RDB$ACL*/
		jrd_186.jrd_187 = blob_id_2;
	}
	/*END_STORE*/
	handle1.compile(tdbb, (UCHAR*) jrd_185, sizeof(jrd_185));
	EXE_start (tdbb, handle1, attachment->getSysTransaction());
	EXE_send (tdbb, handle1, 0, 40, (UCHAR*) &jrd_186);
	}

	handle1.reset();

	/*FOR(REQUEST_HANDLE handle1) REL IN RDB$RELATIONS
		WITH REL.RDB$RELATION_NAME EQ rel_name*/
	{
	handle1.compile(tdbb, (UCHAR*) jrd_169, sizeof(jrd_169));
	gds__vtov ((const char*) rel_name, (char*) jrd_170.jrd_171, 32);
	EXE_start (tdbb, handle1, attachment->getSysTransaction());
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_170);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 70, (UCHAR*) &jrd_172);
	   if (!jrd_172.jrd_175) break;
	{
		/*MODIFY REL USING*/
		{
		
			/*REL.RDB$SECURITY_CLASS.NULL*/
			jrd_172.jrd_177 = FALSE;
			PAD(security_class.c_str(), /*REL.RDB$SECURITY_CLASS*/
						    jrd_172.jrd_174);

			/*REL.RDB$DEFAULT_CLASS.NULL*/
			jrd_172.jrd_176 = FALSE;
			PAD(default_class.c_str(), /*REL.RDB$DEFAULT_CLASS*/
						   jrd_172.jrd_173);
		/*END_MODIFY*/
		gds__vtov((const char*) jrd_172.jrd_174, (char*) jrd_178.jrd_179, 32);
		gds__vtov((const char*) jrd_172.jrd_173, (char*) jrd_178.jrd_180, 32);
		jrd_178.jrd_181 = jrd_172.jrd_177;
		jrd_178.jrd_182 = jrd_172.jrd_176;
		EXE_send (tdbb, handle1, 2, 68, (UCHAR*) &jrd_178);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, handle1, 3, 2, (UCHAR*) &jrd_183);
	   }
	}

	handle1.reset();

	for (int cnt = 0; cnt < 6; cnt++)
	{
		/*STORE(REQUEST_HANDLE handle1) PRIV IN RDB$USER_PRIVILEGES*/
		{
		
			switch (cnt)
			{
			case 0:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'S';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 1;
				break;
			case 1:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'I';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 1;
				break;
			case 2:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'U';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 1;
				break;
			case 3:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'D';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 1;
				break;
			case 4:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, user_name.c_str());
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'R';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 1;
				break;
			default:
				strcpy(/*PRIV.RDB$USER*/
				       jrd_159.jrd_163, "PUBLIC");
				/*PRIV.RDB$PRIVILEGE*/
				jrd_159.jrd_168[0] = 'S';
				/*PRIV.RDB$GRANT_OPTION*/
				jrd_159.jrd_167 = 0;
				break;
			}
			strcpy(/*PRIV.RDB$GRANTOR*/
			       jrd_159.jrd_162, user_name.c_str());
			/*PRIV.RDB$PRIVILEGE*/
			jrd_159.jrd_168[1] = 0;
			strcpy(/*PRIV.RDB$RELATION_NAME*/
			       jrd_159.jrd_161, rel_name);
			/*PRIV.RDB$FIELD_NAME.NULL*/
			jrd_159.jrd_166 = TRUE;
			/*PRIV.RDB$USER_TYPE*/
			jrd_159.jrd_165 = obj_user;
			/*PRIV.RDB$OBJECT_TYPE*/
			jrd_159.jrd_164 = obj_relation;
		/*END_STORE*/
		handle1.compile(tdbb, (UCHAR*) jrd_158, sizeof(jrd_158));
		EXE_start (tdbb, handle1, attachment->getSysTransaction());
		EXE_send (tdbb, handle1, 0, 143, (UCHAR*) &jrd_159);
		}
	}
}


// Add security to system objects.
static void add_security_to_sys_obj(thread_db* tdbb,
									const MetaName& user_name,
									USHORT obj_type,
									const MetaName& obj_name,
									USHORT acl_length,
									const UCHAR* acl)
{
   struct {
          TEXT  jrd_88 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_89 [32];	// RDB$USER 
          SSHORT jrd_90;	// RDB$OBJECT_TYPE 
          SSHORT jrd_91;	// RDB$USER_TYPE 
          SSHORT jrd_92;	// RDB$GRANT_OPTION 
          TEXT  jrd_93 [7];	// RDB$PRIVILEGE 
   } jrd_87;
   struct {
          SSHORT jrd_105;	// gds__utility 
   } jrd_104;
   struct {
          TEXT  jrd_102 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_103;	// gds__null_flag 
   } jrd_101;
   struct {
          TEXT  jrd_98 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_99;	// gds__utility 
          SSHORT jrd_100;	// gds__null_flag 
   } jrd_97;
   struct {
          TEXT  jrd_96 [32];	// RDB$GENERATOR_NAME 
   } jrd_95;
   struct {
          SSHORT jrd_117;	// gds__utility 
   } jrd_116;
   struct {
          TEXT  jrd_114 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_115;	// gds__null_flag 
   } jrd_113;
   struct {
          TEXT  jrd_110 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_111;	// gds__utility 
          SSHORT jrd_112;	// gds__null_flag 
   } jrd_109;
   struct {
          TEXT  jrd_108 [32];	// RDB$EXCEPTION_NAME 
   } jrd_107;
   struct {
          SSHORT jrd_129;	// gds__utility 
   } jrd_128;
   struct {
          TEXT  jrd_126 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_127;	// gds__null_flag 
   } jrd_125;
   struct {
          TEXT  jrd_122 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_123;	// gds__utility 
          SSHORT jrd_124;	// gds__null_flag 
   } jrd_121;
   struct {
          TEXT  jrd_120 [32];	// RDB$COLLATION_NAME 
   } jrd_119;
   struct {
          SSHORT jrd_141;	// gds__utility 
   } jrd_140;
   struct {
          TEXT  jrd_138 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_139;	// gds__null_flag 
   } jrd_137;
   struct {
          TEXT  jrd_134 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_135;	// gds__utility 
          SSHORT jrd_136;	// gds__null_flag 
   } jrd_133;
   struct {
          TEXT  jrd_132 [32];	// RDB$CHARACTER_SET_NAME 
   } jrd_131;
   struct {
          SSHORT jrd_153;	// gds__utility 
   } jrd_152;
   struct {
          TEXT  jrd_150 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_151;	// gds__null_flag 
   } jrd_149;
   struct {
          TEXT  jrd_146 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_147;	// gds__utility 
          SSHORT jrd_148;	// gds__null_flag 
   } jrd_145;
   struct {
          TEXT  jrd_144 [32];	// RDB$FIELD_NAME 
   } jrd_143;
   struct {
          bid  jrd_156;	// RDB$ACL 
          TEXT  jrd_157 [32];	// RDB$SECURITY_CLASS 
   } jrd_155;
	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	bid blob_id;
	attachment->storeBinaryBlob(tdbb, attachment->getSysTransaction(), &blob_id,
								ByteChunk(acl, acl_length));

	Firebird::MetaName security_class;
	security_class.printf("%s%"SQUADFORMAT, SQL_SECCLASS_PREFIX,
		DPM_gen_id(tdbb, MET_lookup_generator(tdbb, SQL_SECCLASS_GENERATOR), false, 1));

	AutoRequest handle;

	/*STORE(REQUEST_HANDLE handle)
		CLS IN RDB$SECURITY_CLASSES*/
	{
	
	{
		PAD(security_class.c_str(), /*CLS.RDB$SECURITY_CLASS*/
					    jrd_155.jrd_157);
		/*CLS.RDB$ACL*/
		jrd_155.jrd_156 = blob_id;
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_154, sizeof(jrd_154));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 40, (UCHAR*) &jrd_155);
	}

	handle.reset();

	if (obj_type == obj_field)
	{
		/*FOR(REQUEST_HANDLE handle) FLD IN RDB$FIELDS
			WITH FLD.RDB$FIELD_NAME EQ obj_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_142, sizeof(jrd_142));
		gds__vtov ((const char*) obj_name.c_str(), (char*) jrd_143.jrd_144, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_143);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_145);
		   if (!jrd_145.jrd_147) break;
		{
			/*MODIFY FLD USING*/
			{
			
				/*FLD.RDB$SECURITY_CLASS.NULL*/
				jrd_145.jrd_148 = FALSE;
				PAD(security_class.c_str(), /*FLD.RDB$SECURITY_CLASS*/
							    jrd_145.jrd_146);
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_145.jrd_146, (char*) jrd_149.jrd_150, 32);
			jrd_149.jrd_151 = jrd_145.jrd_148;
			EXE_send (tdbb, handle, 2, 34, (UCHAR*) &jrd_149);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_152);
		   }
		}
	}
	else if (obj_type == obj_charset)
	{
		/*FOR(REQUEST_HANDLE handle) CS IN RDB$CHARACTER_SETS
			WITH CS.RDB$CHARACTER_SET_NAME EQ obj_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_130, sizeof(jrd_130));
		gds__vtov ((const char*) obj_name.c_str(), (char*) jrd_131.jrd_132, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_131);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_133);
		   if (!jrd_133.jrd_135) break;
		{
			/*MODIFY CS USING*/
			{
			
				/*CS.RDB$SECURITY_CLASS.NULL*/
				jrd_133.jrd_136 = FALSE;
				PAD(security_class.c_str(), /*CS.RDB$SECURITY_CLASS*/
							    jrd_133.jrd_134);
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_133.jrd_134, (char*) jrd_137.jrd_138, 32);
			jrd_137.jrd_139 = jrd_133.jrd_136;
			EXE_send (tdbb, handle, 2, 34, (UCHAR*) &jrd_137);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_140);
		   }
		}
	}
	else if (obj_type == obj_collation)
	{
		/*FOR(REQUEST_HANDLE handle) COLL IN RDB$COLLATIONS
			WITH COLL.RDB$COLLATION_NAME EQ obj_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_118, sizeof(jrd_118));
		gds__vtov ((const char*) obj_name.c_str(), (char*) jrd_119.jrd_120, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_119);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_121);
		   if (!jrd_121.jrd_123) break;
		{
			/*MODIFY COLL USING*/
			{
			
				/*COLL.RDB$SECURITY_CLASS.NULL*/
				jrd_121.jrd_124 = FALSE;
				PAD(security_class.c_str(), /*COLL.RDB$SECURITY_CLASS*/
							    jrd_121.jrd_122);
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_121.jrd_122, (char*) jrd_125.jrd_126, 32);
			jrd_125.jrd_127 = jrd_121.jrd_124;
			EXE_send (tdbb, handle, 2, 34, (UCHAR*) &jrd_125);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_128);
		   }
		}
	}
	else if (obj_type == obj_exception)
	{
		/*FOR(REQUEST_HANDLE handle) XCP IN RDB$EXCEPTIONS
			WITH XCP.RDB$EXCEPTION_NAME EQ obj_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_106, sizeof(jrd_106));
		gds__vtov ((const char*) obj_name.c_str(), (char*) jrd_107.jrd_108, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_107);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_109);
		   if (!jrd_109.jrd_111) break;
		{
			/*MODIFY XCP USING*/
			{
			
				/*XCP.RDB$SECURITY_CLASS.NULL*/
				jrd_109.jrd_112 = FALSE;
				PAD(security_class.c_str(), /*XCP.RDB$SECURITY_CLASS*/
							    jrd_109.jrd_110);
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_109.jrd_110, (char*) jrd_113.jrd_114, 32);
			jrd_113.jrd_115 = jrd_109.jrd_112;
			EXE_send (tdbb, handle, 2, 34, (UCHAR*) &jrd_113);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_116);
		   }
		}
	}
	else if (obj_type == obj_generator)
	{
		/*FOR(REQUEST_HANDLE handle) GEN IN RDB$GENERATORS
			WITH GEN.RDB$GENERATOR_NAME EQ obj_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_94, sizeof(jrd_94));
		gds__vtov ((const char*) obj_name.c_str(), (char*) jrd_95.jrd_96, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_95);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_97);
		   if (!jrd_97.jrd_99) break;
		{
			/*MODIFY GEN USING*/
			{
			
				/*GEN.RDB$SECURITY_CLASS.NULL*/
				jrd_97.jrd_100 = FALSE;
				PAD(security_class.c_str(), /*GEN.RDB$SECURITY_CLASS*/
							    jrd_97.jrd_98);
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_97.jrd_98, (char*) jrd_101.jrd_102, 32);
			jrd_101.jrd_103 = jrd_97.jrd_100;
			EXE_send (tdbb, handle, 2, 34, (UCHAR*) &jrd_101);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_104);
		   }
		}
	}

	handle.reset();

	for (const char* p = USAGE_PRIVILEGES; *p; ++p)
	{
		/*STORE(REQUEST_HANDLE handle) PRIV IN RDB$USER_PRIVILEGES*/
		{
		
			PAD(user_name.c_str(), /*PRIV.RDB$USER*/
					       jrd_87.jrd_89);
			PAD(obj_name.c_str(), /*PRIV.RDB$RELATION_NAME*/
					      jrd_87.jrd_88);
			/*PRIV.RDB$PRIVILEGE*/
			jrd_87.jrd_93[0] = *p;
			/*PRIV.RDB$PRIVILEGE*/
			jrd_87.jrd_93[1] = 0;
			/*PRIV.RDB$GRANT_OPTION*/
			jrd_87.jrd_92 = 1;
			/*PRIV.RDB$USER_TYPE*/
			jrd_87.jrd_91 = obj_user;
			/*PRIV.RDB$OBJECT_TYPE*/
			jrd_87.jrd_90 = obj_type;
		/*END_STORE*/
		handle.compile(tdbb, (UCHAR*) jrd_86, sizeof(jrd_86));
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 77, (UCHAR*) &jrd_87);
		}
	}
}


static void store_generator(thread_db* tdbb, const gen* generator, AutoRequest& handle,
	const MetaName& owner)
{
   struct {
          bid  jrd_75;	// RDB$DESCRIPTION 
          ISC_INT64  jrd_76;	// RDB$INITIAL_VALUE 
          TEXT  jrd_77 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_78 [32];	// RDB$GENERATOR_NAME 
          SLONG  jrd_79;	// RDB$GENERATOR_INCREMENT 
          SSHORT jrd_80;	// gds__null_flag 
          SSHORT jrd_81;	// gds__null_flag 
          SSHORT jrd_82;	// gds__null_flag 
          SSHORT jrd_83;	// gds__null_flag 
          SSHORT jrd_84;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_85;	// RDB$GENERATOR_ID 
   } jrd_74;
/**************************************
 *
 *	s t o r e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Store the passed generator according to
 *	the information in the generator block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	/*STORE(REQUEST_HANDLE handle) X IN RDB$GENERATORS*/
	{
	
	{
		PAD(generator->gen_name, /*X.RDB$GENERATOR_NAME*/
					 jrd_74.jrd_78);
		/*X.RDB$GENERATOR_ID*/
		jrd_74.jrd_85 = generator->gen_id;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_74.jrd_84 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_74.jrd_83 = FALSE;
		PAD(owner.c_str(), /*X.RDB$OWNER_NAME*/
				   jrd_74.jrd_77);
		/*X.RDB$OWNER_NAME.NULL*/
		jrd_74.jrd_82 = FALSE;
		/*X.RDB$INITIAL_VALUE*/
		jrd_74.jrd_76 = 0;
		/*X.RDB$INITIAL_VALUE.NULL*/
		jrd_74.jrd_81 = FALSE;
		if (generator->gen_description)
		{
		    attachment->storeMetaDataBlob(tdbb, attachment->getSysTransaction(), &/*X.RDB$DESCRIPTION*/
											  jrd_74.jrd_75,
				generator->gen_description);
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_74.jrd_80 = FALSE;
		}
		else
			/*X.RDB$DESCRIPTION.NULL*/
			jrd_74.jrd_80 = TRUE;

		/*X.RDB$GENERATOR_INCREMENT*/
		jrd_74.jrd_79 = 0; // only sys gens have zero default increment
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_73, sizeof(jrd_73));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 96, (UCHAR*) &jrd_74);
	}
}


static void store_global_field(thread_db* tdbb, const gfld* gfield, AutoRequest& handle,
	const MetaName& owner)
{
   struct {
          bid  jrd_51;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_52 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_53 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_54;	// gds__null_flag 
          SSHORT jrd_55;	// RDB$NULL_FLAG 
          SSHORT jrd_56;	// RDB$FIELD_TYPE 
          SSHORT jrd_57;	// gds__null_flag 
          SSHORT jrd_58;	// gds__null_flag 
          SSHORT jrd_59;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_60;	// gds__null_flag 
          SSHORT jrd_61;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_62;	// gds__null_flag 
          SSHORT jrd_63;	// RDB$COLLATION_ID 
          SSHORT jrd_64;	// gds__null_flag 
          SSHORT jrd_65;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_66;	// gds__null_flag 
          SSHORT jrd_67;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_68;	// gds__null_flag 
          SSHORT jrd_69;	// gds__null_flag 
          SSHORT jrd_70;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_71;	// RDB$FIELD_SCALE 
          SSHORT jrd_72;	// RDB$FIELD_LENGTH 
   } jrd_50;
/**************************************
 *
 *	s t o r e _ g l o b a l _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store a global field according to the
 *	passed information.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Jrd::Attachment* attachment = tdbb->getAttachment();

	/*STORE(REQUEST_HANDLE handle) X IN RDB$FIELDS*/
	{
	
	{
		PAD(names[(USHORT)gfield->gfld_name], /*X.RDB$FIELD_NAME*/
						      jrd_50.jrd_53);
		/*X.RDB$FIELD_LENGTH*/
		jrd_50.jrd_72 = gfield->gfld_length;
		/*X.RDB$FIELD_SCALE*/
		jrd_50.jrd_71 = 0;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_50.jrd_70 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_50.jrd_69 = FALSE;
		PAD(owner.c_str(), /*X.RDB$OWNER_NAME*/
				   jrd_50.jrd_52);
		/*X.RDB$OWNER_NAME.NULL*/
		jrd_50.jrd_68 = FALSE;
		/*X.RDB$FIELD_SUB_TYPE.NULL*/
		jrd_50.jrd_66 = TRUE;
		/*X.RDB$CHARACTER_SET_ID.NULL*/
		jrd_50.jrd_64 = TRUE;
		/*X.RDB$COLLATION_ID.NULL*/
		jrd_50.jrd_62 = TRUE;
		/*X.RDB$SEGMENT_LENGTH.NULL*/
		jrd_50.jrd_60 = TRUE;
		/*X.RDB$CHARACTER_LENGTH.NULL*/
		jrd_50.jrd_58 = TRUE;
		if (gfield->gfld_dflt_blr)
		{
		    attachment->storeBinaryBlob(tdbb, attachment->getSysTransaction(), &/*X.RDB$DEFAULT_VALUE*/
											jrd_50.jrd_51,
				ByteChunk(gfield->gfld_dflt_blr, gfield->gfld_dflt_len));
			/*X.RDB$DEFAULT_VALUE.NULL*/
			jrd_50.jrd_57 = FALSE;
		}
		else
			/*X.RDB$DEFAULT_VALUE.NULL*/
			jrd_50.jrd_57 = TRUE;

		switch (gfield->gfld_dtype)
		{
		case dtype_timestamp:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_timestamp;
			break;

		case dtype_sql_time:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_sql_time;
			break;

		case dtype_sql_date:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_sql_date;
			break;

		case dtype_short:
		case dtype_long:
		case dtype_int64:
			if (gfield->gfld_dtype == dtype_short)
				/*X.RDB$FIELD_TYPE*/
				jrd_50.jrd_56 = (int) blr_short;
			else if (gfield->gfld_dtype == dtype_long)
				/*X.RDB$FIELD_TYPE*/
				jrd_50.jrd_56 = (int) blr_long;
			else
				/*X.RDB$FIELD_TYPE*/
				jrd_50.jrd_56 = (int) blr_int64;

			if ((gfield->gfld_sub_type == dsc_num_type_numeric) ||
				(gfield->gfld_sub_type == dsc_num_type_decimal))
			{
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_50.jrd_66 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_50.jrd_67 = gfield->gfld_sub_type;
			}
			break;

		case dtype_double:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_double;
			break;

		case dtype_text:
		case dtype_varying:
			if (gfield->gfld_dtype == dtype_text)
			{
				/*X.RDB$FIELD_TYPE*/
				jrd_50.jrd_56 = (int) blr_text;
			}
			else
			{
				/*X.RDB$FIELD_TYPE*/
				jrd_50.jrd_56 = (int) blr_varying;
			}
			switch (gfield->gfld_sub_type)
			{
			case dsc_text_type_metadata:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_50.jrd_64 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_50.jrd_65 = CS_METADATA;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_50.jrd_62 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_50.jrd_63 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_50.jrd_66 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_50.jrd_67 = gfield->gfld_sub_type;
				/*X.RDB$CHARACTER_LENGTH.NULL*/
				jrd_50.jrd_58 = FALSE;
				/*X.RDB$CHARACTER_LENGTH*/
				jrd_50.jrd_59 = /*X.RDB$FIELD_LENGTH*/
   jrd_50.jrd_72 / METADATA_BYTES_PER_CHAR;
				break;
			case dsc_text_type_ascii:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_50.jrd_64 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_50.jrd_65 = CS_ASCII;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_50.jrd_62 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_50.jrd_63 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_50.jrd_66 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_50.jrd_67 = gfield->gfld_sub_type;
				break;
			case dsc_text_type_fixed:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_50.jrd_64 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_50.jrd_65 = CS_BINARY;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_50.jrd_62 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_50.jrd_63 = COLLATE_NONE;
				/*X.RDB$FIELD_SUB_TYPE.NULL*/
				jrd_50.jrd_66 = FALSE;
				/*X.RDB$FIELD_SUB_TYPE*/
				jrd_50.jrd_67 = gfield->gfld_sub_type;
				break;
			default:
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_50.jrd_64 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_50.jrd_65 = CS_NONE;
				/*X.RDB$COLLATION_ID.NULL*/
				jrd_50.jrd_62 = FALSE;
				/*X.RDB$COLLATION_ID*/
				jrd_50.jrd_63 = COLLATE_NONE;
				break;
			}
			break;

		case dtype_blob:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_blob;
			/*X.RDB$FIELD_SUB_TYPE.NULL*/
			jrd_50.jrd_66 = FALSE;
			/*X.RDB$SEGMENT_LENGTH.NULL*/
			jrd_50.jrd_60 = FALSE;
			/*X.RDB$FIELD_SUB_TYPE*/
			jrd_50.jrd_67 = gfield->gfld_sub_type;
			/*X.RDB$SEGMENT_LENGTH*/
			jrd_50.jrd_61 = 80;
			if (gfield->gfld_sub_type == isc_blob_text)
			{
				/*X.RDB$CHARACTER_SET_ID.NULL*/
				jrd_50.jrd_64 = FALSE;
				/*X.RDB$CHARACTER_SET_ID*/
				jrd_50.jrd_65 = CS_METADATA;
			}
			break;

		case dtype_boolean:
			/*X.RDB$FIELD_TYPE*/
			jrd_50.jrd_56 = (int) blr_bool;
			break;

		default:
			fb_assert(FALSE);
			break;
		}

		// Acknowledge not NULL sys fields
		/*X.RDB$NULL_FLAG.NULL*/
		jrd_50.jrd_54 = FALSE;
		/*X.RDB$NULL_FLAG*/
		jrd_50.jrd_55 = !gfield->gfld_nullable;
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_49, sizeof(jrd_49));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 110, (UCHAR*) &jrd_50);
	}
}


static void store_intlnames(thread_db* tdbb, const MetaName& owner)
{
   struct {
          bid  jrd_27;	// RDB$SPECIFIC_ATTRIBUTES 
          TEXT  jrd_28 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_29 [32];	// RDB$BASE_COLLATION_NAME 
          TEXT  jrd_30 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_31;	// gds__null_flag 
          SSHORT jrd_32;	// RDB$COLLATION_ATTRIBUTES 
          SSHORT jrd_33;	// gds__null_flag 
          SSHORT jrd_34;	// gds__null_flag 
          SSHORT jrd_35;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_36;	// RDB$COLLATION_ID 
          SSHORT jrd_37;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_38;	// gds__null_flag 
   } jrd_26;
   struct {
          TEXT  jrd_41 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_42 [32];	// RDB$DEFAULT_COLLATE_NAME 
          TEXT  jrd_43 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_44;	// gds__null_flag 
          SSHORT jrd_45;	// gds__null_flag 
          SSHORT jrd_46;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_47;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_48;	// RDB$CHARACTER_SET_ID 
   } jrd_40;
/**************************************
 *
 *	s t o r e _ i n t l n a m e s
 *
 **************************************
 *
 * Functional description
 *	Store symbolic names & information for international
 *	character sets & collations.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	AutoRequest handle;

	for (const IntlManager::CharSetDefinition* charSet = IntlManager::defaultCharSets;
		 charSet->name; ++charSet)
	{
		/*STORE(REQUEST_HANDLE handle) X IN RDB$CHARACTER_SETS USING*/
		{
		
		{
			PAD(charSet->name, /*X.RDB$CHARACTER_SET_NAME*/
					   jrd_40.jrd_43);
			PAD(charSet->name, /*X.RDB$DEFAULT_COLLATE_NAME*/
					   jrd_40.jrd_42);
			/*X.RDB$CHARACTER_SET_ID*/
			jrd_40.jrd_48 = charSet->id;
			/*X.RDB$BYTES_PER_CHARACTER*/
			jrd_40.jrd_47 = charSet->maxBytes;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_40.jrd_46 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_40.jrd_45 = FALSE;
			PAD(owner.c_str(), /*X.RDB$OWNER_NAME*/
					   jrd_40.jrd_41);
			/*X.RDB$OWNER_NAME.NULL*/
			jrd_40.jrd_44 = FALSE;
		}
		/*END_STORE*/
		handle.compile(tdbb, (UCHAR*) jrd_39, sizeof(jrd_39));
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 106, (UCHAR*) &jrd_40);
		}
	}

	handle.reset();

	for (const IntlManager::CollationDefinition* collation = IntlManager::defaultCollations;
		collation->name; ++collation)
	{
		/*STORE(REQUEST_HANDLE handle) X IN RDB$COLLATIONS USING*/
		{
		
		{
			PAD(collation->name, /*X.RDB$COLLATION_NAME*/
					     jrd_26.jrd_30);

			if (collation->baseName)
			{
				/*X.RDB$BASE_COLLATION_NAME.NULL*/
				jrd_26.jrd_38 = false;
				PAD(collation->baseName, /*X.RDB$BASE_COLLATION_NAME*/
							 jrd_26.jrd_29);
			}
			else
				/*X.RDB$BASE_COLLATION_NAME.NULL*/
				jrd_26.jrd_38 = true;

			/*X.RDB$CHARACTER_SET_ID*/
			jrd_26.jrd_37 = collation->charSetId;
			/*X.RDB$COLLATION_ID*/
			jrd_26.jrd_36 = collation->collationId;
			/*X.RDB$SYSTEM_FLAG*/
			jrd_26.jrd_35 = RDB_system;
			/*X.RDB$SYSTEM_FLAG.NULL*/
			jrd_26.jrd_34 = FALSE;
			PAD(owner.c_str(), /*X.RDB$OWNER_NAME*/
					   jrd_26.jrd_28);
			/*X.RDB$OWNER_NAME.NULL*/
			jrd_26.jrd_33 = FALSE;
			/*X.RDB$COLLATION_ATTRIBUTES*/
			jrd_26.jrd_32 = collation->attributes;

			if (collation->specificAttributes)
			{
				attachment->storeMetaDataBlob(tdbb, attachment->getSysTransaction(),
					&/*X.RDB$SPECIFIC_ATTRIBUTES*/
					 jrd_26.jrd_27, collation->specificAttributes);
				/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
				jrd_26.jrd_31 = FALSE;
			}
			else
				/*X.RDB$SPECIFIC_ATTRIBUTES.NULL*/
				jrd_26.jrd_31 = TRUE;
		}
		/*END_STORE*/
		handle.compile(tdbb, (UCHAR*) jrd_25, sizeof(jrd_25));
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 120, (UCHAR*) &jrd_26);
		}
	}
}


static void store_message(thread_db* tdbb, const trigger_msg* message, AutoRequest& handle)
{
   struct {
          TEXT  jrd_22 [1024];	// RDB$MESSAGE 
          TEXT  jrd_23 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_24;	// RDB$MESSAGE_NUMBER 
   } jrd_21;
/**************************************
 *
 *	s t o r e _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Store system trigger messages.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	// store the trigger

	/*STORE(REQUEST_HANDLE handle) X IN RDB$TRIGGER_MESSAGES*/
	{
	
	{
		PAD(message->trigmsg_name, /*X.RDB$TRIGGER_NAME*/
					   jrd_21.jrd_23);
		/*X.RDB$MESSAGE_NUMBER*/
		jrd_21.jrd_24 = message->trigmsg_number;
		PAD(message->trigmsg_text, /*X.RDB$MESSAGE*/
					   jrd_21.jrd_22);
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_20, sizeof(jrd_20));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 1058, (UCHAR*) &jrd_21);
	}
}


static void store_relation_field(thread_db*		tdbb,
								 const int*		fld,
								 const int*		relfld,
								 int			field_id,
								 AutoRequest&	handle)
{
   struct {
          TEXT  jrd_12 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_13 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_14 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_15;	// RDB$UPDATE_FLAG 
          SSHORT jrd_16;	// gds__null_flag 
          SSHORT jrd_17;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_18;	// RDB$FIELD_ID 
          SSHORT jrd_19;	// RDB$FIELD_POSITION 
   } jrd_11;
/**************************************
 *
 *	s t o r e _ r e l a t i o n _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store a local field according to the
 *	passed information.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	/*STORE(REQUEST_HANDLE handle) X IN RDB$RELATION_FIELDS*/
	{
	
	{
		const gfld* gfield = &gfields[fld[RFLD_F_ID]];
		PAD(names[relfld[RFLD_R_NAME]], /*X.RDB$RELATION_NAME*/
						jrd_11.jrd_14);
		PAD(names[fld[RFLD_F_NAME]], /*X.RDB$FIELD_NAME*/
					     jrd_11.jrd_13);
		PAD(names[gfield->gfld_name], /*X.RDB$FIELD_SOURCE*/
					      jrd_11.jrd_12);
		/*X.RDB$FIELD_POSITION*/
		jrd_11.jrd_19 = field_id;
		/*X.RDB$FIELD_ID*/
		jrd_11.jrd_18 = field_id;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_11.jrd_17 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_11.jrd_16 = FALSE;
		/*X.RDB$UPDATE_FLAG*/
		jrd_11.jrd_15 = fld[RFLD_F_UPDATE];
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_10, sizeof(jrd_10));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 106, (UCHAR*) &jrd_11);
	}
}


static void store_trigger(thread_db* tdbb, const jrd_trg* trigger, AutoRequest& handle)
{
   struct {
          bid  jrd_2;	// RDB$TRIGGER_BLR 
          ISC_INT64  jrd_3;	// RDB$TRIGGER_TYPE 
          TEXT  jrd_4 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_5 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_6;	// RDB$FLAGS 
          SSHORT jrd_7;	// gds__null_flag 
          SSHORT jrd_8;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_9;	// RDB$TRIGGER_SEQUENCE 
   } jrd_1;
/**************************************
 *
 *	s t o r e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Store the trigger according to the
 *	information in the trigger block.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	// indicate that the relation format needs revising
	dsc desc;
	desc.makeText(strlen(names[trigger->trg_relation]), CS_METADATA,
		(UCHAR*) names[trigger->trg_relation]);
	DFW_post_system_work(tdbb, dfw_update_format, &desc, 0);

	// store the trigger

	/*STORE(REQUEST_HANDLE handle) X IN RDB$TRIGGERS*/
	{
	
	{
		PAD(trigger->trg_name, /*X.RDB$TRIGGER_NAME*/
				       jrd_1.jrd_5);
		PAD(names[trigger->trg_relation], /*X.RDB$RELATION_NAME*/
						  jrd_1.jrd_4);
		/*X.RDB$TRIGGER_SEQUENCE*/
		jrd_1.jrd_9 = 0;
		/*X.RDB$SYSTEM_FLAG*/
		jrd_1.jrd_8 = RDB_system;
		/*X.RDB$SYSTEM_FLAG.NULL*/
		jrd_1.jrd_7 = FALSE;
		/*X.RDB$TRIGGER_TYPE*/
		jrd_1.jrd_3 = trigger->trg_type;
		/*X.RDB$FLAGS*/
		jrd_1.jrd_6 = trigger->trg_flags;
		attachment->storeBinaryBlob(tdbb, attachment->getSysTransaction(), &/*X.RDB$TRIGGER_BLR*/
										    jrd_1.jrd_2,
			ByteChunk(trigger->trg_blr, trigger->trg_length));
	}
	/*END_STORE*/
	handle.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 88, (UCHAR*) &jrd_1);
	}
}
