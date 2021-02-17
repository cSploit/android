/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *  PROGRAM:    Dynamic SQL runtime support
 *  MODULE:     metd.epp
 *  DESCRIPTION:    Meta-data interface
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
 * 2001.11.28 Claudio Valderrama: load not only udfs but udf arguments;
 *   handle possible collisions with udf redefinitions (drop->declare).
 *   This closes SF Bug# 409769.
 * 2001.12.06 Claudio Valderrama: METD_get_charset_bpc() was added to
 *    get only the bytes per char of a field, given its charset id.
 *   This request is not cached.
 * 2001.02.23 Claudio Valderrama: Fix SF Bug #228135 with views spoiling
 *    NULLs in outer joins.
 * 2004.01.16 Vlad Horsun: make METD_get_col_default and
 *   METD_get_domain_default return actual length of default BLR
 * 2004.01.16 Vlad Horsun: added support for default parameters
 */

#include "firebird.h"
#include <string.h>
#include "../dsql/dsql.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/metd_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/errd_proto.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/thread_proto.h"
#include "../yvalve/why_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/init.h"

using namespace Jrd;
using namespace Firebird;

// NOTE: The static definition of DB and gds_trans by gpre will not
// be used by the meta data routines.  Each of those routines has
// its own local definition of these variables.

/*DATABASE DB = STATIC "yachts.lnk";*/
static const UCHAR	jrd_0 [99] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 7,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_7 [107] =
   {	// blr string 
blr_version4,
blr_begin, 
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
               blr_rid, 5,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 1,0, 4,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_16 [135] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 7,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 5,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_26 [97] =
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
               blr_rid, 11,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_33 [309] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 24,0, 
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
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 2,0, 0, 
               blr_rid, 5,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_fid, 1, 2,0, 
                     blr_eql, 
                        blr_fid, 1, 1,0, 
                        blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 1, 6,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 0,0, 18,0, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 1, 13,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 23,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 1, 16,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 25,0, 
                     blr_parameter2, 1, 9,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 1, 18,0, 
                     blr_parameter2, 1, 11,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter2, 1, 13,0, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 22,0, 
                     blr_parameter2, 1, 15,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 17,0, 
                     blr_parameter, 1, 16,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 17,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 19,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 20,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 21,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter2, 1, 23,0, 22,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_61 [144] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 9,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
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
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter2, 1, 1,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter2, 1, 8,0, 7,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_74 [97] =
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
            blr_rse, 2, 
               blr_rid, 6,0, 0, 
               blr_rid, 5,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 1,0, 
                        blr_fid, 0, 8,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 8,0, 
                           blr_parameter, 0, 0,0, 
                        blr_or, 
                           blr_missing, 
                              blr_fid, 0, 3,0, 
                           blr_missing, 
                              blr_fid, 1, 9,0, 
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
static const UCHAR	jrd_79 [392] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 29,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 27,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 0,0, 
                        blr_fid, 0, 4,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 3,0, 
                              blr_parameter, 0, 2,0, 
                           blr_equiv, 
                              blr_fid, 0, 14,0, 
                              blr_value_if, 
                                 blr_eql, 
                                    blr_parameter, 0, 0,0, 
                                    blr_literal, blr_text2, 3,0, 0,0, 
                                 blr_null, 
                                 blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_descending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter2, 1, 0,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 2,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 3,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 6,0, 26,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 1, 17,0, 
                     blr_parameter, 1, 11,0, 
                  blr_assignment, 
                     blr_fid, 1, 23,0, 
                     blr_parameter, 1, 12,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter, 1, 13,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter2, 1, 15,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter2, 1, 17,0, 16,0, 
                  blr_assignment, 
                     blr_fid, 1, 11,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter, 1, 19,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 1, 20,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 21,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 23,0, 22,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 25,0, 24,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 28,0, 27,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 7,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_114 [139] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_cstring2, 3,0, 32,0, 
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
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 1,0, 
                     blr_equiv, 
                        blr_fid, 0, 16,0, 
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
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 17,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_124 [143] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 4,0, 0, 
               blr_rid, 3,0, 1, 
               blr_rid, 22,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 1, 0,0, 
                           blr_fid, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 2, 5,0, 
                           blr_fid, 1, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 2, 2,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 2, 1,0, 
                           blr_literal, blr_text2, 3,0, 11,0, 'P','R','I','M','A','R','Y',32,'K','E','Y',
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 1, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 1,0, 
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
static const UCHAR	jrd_130 [134] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 8,0, 
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
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter2, 1, 5,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 7,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_142 [209] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 12,0, 
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
                     blr_parameter2, 1, 0,0, 11,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 6,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 9,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 10,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_159 [125] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
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
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 3,0, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_168 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
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
static const UCHAR	jrd_175 [227] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 19,0, 
      blr_quad, 0, 
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
                     blr_fid, 0, 4,0, 
                     blr_parameter2, 1, 0,0, 7,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 17,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 23,0, 
                     blr_parameter2, 1, 6,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 24,0, 
                     blr_parameter2, 1, 9,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 25,0, 
                     blr_parameter2, 1, 11,0, 10,0, 
                  blr_assignment, 
                     blr_fid, 0, 26,0, 
                     blr_parameter2, 1, 13,0, 12,0, 
                  blr_assignment, 
                     blr_fid, 0, 22,0, 
                     blr_parameter2, 1, 15,0, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 16,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 17,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter, 1, 18,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_198 [79] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 1,0, 0, 
            blr_first, 
               blr_literal, blr_long, 0, 1,0,0,0,
            blr_boolean, 
               blr_not, 
                  blr_missing, 
                     blr_fid, 0, 3,0, 
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_fid, 0, 3,0, 
                  blr_parameter, 0, 0,0, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 1,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 1,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_202 [82] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 28,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
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
static const UCHAR	jrd_208 [180] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
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
            blr_rse, 3, 
               blr_rid, 29,0, 0, 
               blr_rid, 28,0, 1, 
               blr_rid, 11,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 4,0, 
                        blr_fid, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 2, 1,0, 
                           blr_fid, 1, 4,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 2, 2,0, 
                              blr_parameter, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 2, 0,0, 
                                 blr_literal, blr_text2, 3,0, 22,0, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E',
                              blr_eql, 
                                 blr_fid, 1, 3,0, 
                                 blr_fid, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_217 [130] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_fid, 1, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 6,0, 
                     blr_parameter2, 1, 0,0, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 1,0, 4,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_227 [134] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 5,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 29,0, 0, 
               blr_rid, 28,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 4,0, 
                        blr_fid, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter2, 1, 2,0, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 4,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 0,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 


static const UCHAR blr_bpb[] =
{
	isc_bpb_version1,
	isc_bpb_source_type, 1, isc_blob_blr,
	isc_bpb_target_type, 1, isc_blob_blr
};

static void convert_dtype(TypeClause*, SSHORT);
static void free_relation(dsql_rel*);

namespace
{
	inline void validateTransaction(const jrd_tra* transaction)
	{
		if (!transaction || !transaction->checkHandle())
		{
			ERR_post(Arg::Gds(isc_bad_trans_handle));
		}
	}
}


void METD_drop_charset(jrd_tra* transaction, const MetaName& metaName)
{
/**************************************
 *
 *  M E T D _ d r o p _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Drop a character set from our metadata, and the next caller who wants it will
 *  look up the new version.
 *  Dropping will be achieved by marking the character set
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	dsql_dbb* dbb = transaction->getDsqlAttachment();
	dsql_intlsym* charSet;

	if (dbb->dbb_charsets.get(metaName, charSet))
	{
		MET_dsql_cache_use(tdbb, SYM_intlsym_charset, metaName);
		charSet->intlsym_flags |= INTLSYM_dropped;
		dbb->dbb_charsets.remove(metaName);
		dbb->dbb_charsets_by_id.remove(charSet->intlsym_charset_id);
	}
}


void METD_drop_collation(jrd_tra* transaction, const MetaName& name)
{
/**************************************
 *
 *  M E T D _ d r o p _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a collation from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the collation
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	dsql_dbb* dbb = transaction->getDsqlAttachment();

	dsql_intlsym* collation;

	if (dbb->dbb_collations.get(name, collation))
	{
		MET_dsql_cache_use(tdbb, SYM_intlsym_collation, name);
		collation->intlsym_flags |= INTLSYM_dropped;
		dbb->dbb_collations.remove(name);
	}
}


void METD_drop_function(jrd_tra* transaction, const QualifiedName& name)
{
/**************************************
 *
 *  M E T D _ d r o p _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a user defined function from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the function
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	dsql_dbb* dbb = transaction->getDsqlAttachment();

	dsql_udf* function;

	if (dbb->dbb_functions.get(name, function))
	{
		MET_dsql_cache_use(tdbb, SYM_udf, name.identifier, name.package);
		function->udf_flags |= UDF_dropped;
		dbb->dbb_functions.remove(name);
	}

}


void METD_drop_procedure(jrd_tra* transaction, const QualifiedName& name)
{
/**************************************
 *
 *  M E T D _ d r o p _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Drop a procedure from our metadata, and
 *  the next caller who wants it will
 *  look up the new version.
 *
 *  Dropping will be achieved by marking the procedure
 *  as dropped.  Anyone with current access can continue
 *  accessing it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	dsql_dbb* dbb = transaction->getDsqlAttachment();

	dsql_prc* procedure;

	if (dbb->dbb_procedures.get(name, procedure))
	{
		MET_dsql_cache_use(tdbb, SYM_procedure, name.identifier, name.package);
		procedure->prc_flags |= PRC_dropped;
		dbb->dbb_procedures.remove(name);
	}
}


void METD_drop_relation(jrd_tra* transaction, const MetaName& name)
{
/**************************************
 *
 *  M E T D _ d r o p _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Drop a relation from our metadata, and
 *  rely on the next guy who wants it to
 *  look up the new version.
 *
 *      Dropping will be achieved by marking the relation
 *      as dropped.  Anyone with current access can continue
 *      accessing it.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	dsql_dbb* dbb = transaction->getDsqlAttachment();

	dsql_rel* relation;

	if (dbb->dbb_relations.get(name, relation))
	{
		MET_dsql_cache_use(tdbb, SYM_relation, name);
		relation->rel_flags |= REL_dropped;
		dbb->dbb_relations.remove(name);
	}
}


dsql_intlsym* METD_get_collation(jrd_tra* transaction, const MetaName& name, USHORT charset_id)
{
   struct {
          SSHORT jrd_232;	// gds__utility 
          SSHORT jrd_233;	// gds__null_flag 
          SSHORT jrd_234;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_235;	// RDB$COLLATION_ID 
          SSHORT jrd_236;	// RDB$CHARACTER_SET_ID 
   } jrd_231;
   struct {
          TEXT  jrd_229 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_230;	// RDB$CHARACTER_SET_ID 
   } jrd_228;
/**************************************
 *
 *  M E T D _ g e t _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();

	// Start by seeing if symbol is already defined

	dsql_intlsym* symbol;
	if (dbb->dbb_collations.get(name, symbol) && !(symbol->intlsym_flags & INTLSYM_dropped) &&
		symbol->intlsym_charset_id == charset_id)
	{
		if (MET_dsql_cache_use(tdbb, SYM_intlsym_collation, name))
			symbol->intlsym_flags |= INTLSYM_dropped;
		else
			return symbol;
	}

	// Now see if it is in the database

	symbol = NULL;

	AutoCacheRequest handle(tdbb, irq_collation, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
	X IN RDB$COLLATIONS
		CROSS Y IN RDB$CHARACTER_SETS OVER RDB$CHARACTER_SET_ID
		WITH X.RDB$COLLATION_NAME EQ name.c_str() AND
			 X.RDB$CHARACTER_SET_ID EQ charset_id*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_227, sizeof(jrd_227));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_228.jrd_229, 32);
	jrd_228.jrd_230 = charset_id;
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_228);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_231);
	   if (!jrd_231.jrd_232) break;;
	{
		symbol = FB_NEW(dbb->dbb_pool) dsql_intlsym(dbb->dbb_pool);
		symbol->intlsym_name = name;
		symbol->intlsym_flags = 0;
		symbol->intlsym_charset_id = /*X.RDB$CHARACTER_SET_ID*/
					     jrd_231.jrd_236;
		symbol->intlsym_collate_id = /*X.RDB$COLLATION_ID*/
					     jrd_231.jrd_235;
		symbol->intlsym_ttype =
			INTL_CS_COLL_TO_TTYPE(symbol->intlsym_charset_id, symbol->intlsym_collate_id);
		symbol->intlsym_bytes_per_char =
			(/*Y.RDB$BYTES_PER_CHARACTER.NULL*/
			 jrd_231.jrd_233) ? 1 : (/*Y.RDB$BYTES_PER_CHARACTER*/
	 jrd_231.jrd_234);
	}
	/*END_FOR*/
	   }
	}

	if (!symbol)
		return NULL;

	dbb->dbb_collations.put(name, symbol);
	MET_dsql_cache_use(tdbb, SYM_intlsym_collation, name);

	return symbol;
}


USHORT METD_get_col_default(jrd_tra* transaction, const char* for_rel_name,
	const char* for_col_name, bool* has_default, UCHAR* buffer, USHORT buff_length)
{
   struct {
          bid  jrd_222;	// RDB$DEFAULT_VALUE 
          bid  jrd_223;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_224;	// gds__utility 
          SSHORT jrd_225;	// gds__null_flag 
          SSHORT jrd_226;	// gds__null_flag 
   } jrd_221;
   struct {
          TEXT  jrd_219 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_220 [32];	// RDB$RELATION_NAME 
   } jrd_218;
/*************************************************************
 *
 *  M E T D _ g e t _ c o l _ d e f a u l t
 *
 **************************************************************
 *
 * Function:
 *    Gets the default value for a column of an existing table.
 *    Will check the default for the column of the table, if that is
 *    not present, will check for the default of the relevant domain
 *
 *    The default blr is returned in buffer. The blr is of the form
 *    blr_version4 blr_literal ..... blr_eoc
 *
 *    Reads the system tables RDB$FIELDS and RDB$RELATION_FIELDS.
 *
 **************************************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();
	bid* blob_id;

	USHORT result = 0;
	blb* blob_handle = 0;

	*has_default = false;

	AutoCacheRequest handle(tdbb, irq_col_default, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		RFL IN RDB$RELATION_FIELDS CROSS
		FLD IN RDB$FIELDS WITH
		RFL.RDB$RELATION_NAME EQ for_rel_name AND
		RFL.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME AND
		RFL.RDB$FIELD_NAME EQ for_col_name*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_217, sizeof(jrd_217));
	gds__vtov ((const char*) for_col_name, (char*) jrd_218.jrd_219, 32);
	gds__vtov ((const char*) for_rel_name, (char*) jrd_218.jrd_220, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_218);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 22, (UCHAR*) &jrd_221);
	   if (!jrd_221.jrd_224) break;
	{
		if (!/*RFL.RDB$DEFAULT_VALUE.NULL*/
		     jrd_221.jrd_226)
		{
			blob_id = &/*RFL.RDB$DEFAULT_VALUE*/
				   jrd_221.jrd_223;
			*has_default = true;
		}
		else if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
			  jrd_221.jrd_225)
		{
			blob_id = &/*FLD.RDB$DEFAULT_VALUE*/
				   jrd_221.jrd_222;
			*has_default = true;
		}
		else
			*has_default = false;

		if (*has_default)
		{
			blob_handle = blb::open2(tdbb, transaction, blob_id, sizeof(blr_bpb), blr_bpb, true);

			// fetch segments. Assuming here that the buffer is big enough.
			UCHAR* ptr_in_buffer = buffer;
			while (true)
			{
				const USHORT length = blob_handle->BLB_get_segment(tdbb, ptr_in_buffer, buff_length);

				ptr_in_buffer += length;
				buff_length -= length;
				result += length;

				if (blob_handle->blb_flags & BLB_eof)
				{
					// null terminate the buffer
					*ptr_in_buffer = 0;
					break;
				}
				if (blob_handle->getFragmentSize())
					status_exception::raise(Arg::Gds(isc_segment));
				else
					continue;
			}

			try
			{
				ThreadStatusGuard status_vector(tdbb);

				blob_handle->BLB_close(tdbb);
				blob_handle = NULL;
			}
			catch (Exception&)
			{
			}

			// the default string must be of the form:
			// blr_version4 blr_literal ..... blr_eoc
			fb_assert((buffer[0] == blr_version4) || (buffer[0] == blr_version5));
			fb_assert(buffer[1] == blr_literal);
		}
		else
		{
			if (dbb->dbb_db_SQL_dialect > SQL_DIALECT_V5)
				buffer[0] = blr_version5;
			else
				buffer[0] = blr_version4;
			buffer[1] = blr_eoc;
			result = 2;
		}
	}
	/*END_FOR*/
	   }
	}

	return result;
}


dsql_intlsym* METD_get_charset(jrd_tra* transaction, USHORT length, const char* name) // UTF-8
{
   struct {
          SSHORT jrd_212;	// gds__utility 
          SSHORT jrd_213;	// gds__null_flag 
          SSHORT jrd_214;	// RDB$BYTES_PER_CHARACTER 
          SSHORT jrd_215;	// RDB$COLLATION_ID 
          SSHORT jrd_216;	// RDB$CHARACTER_SET_ID 
   } jrd_211;
   struct {
          TEXT  jrd_210 [32];	// RDB$TYPE_NAME 
   } jrd_209;
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();
	MetaName metaName(name, length);

	// Start by seeing if symbol is already defined

	dsql_intlsym* symbol;
	if (dbb->dbb_charsets.get(metaName, symbol) && !(symbol->intlsym_flags & INTLSYM_dropped))
	{
		if (MET_dsql_cache_use(tdbb, SYM_intlsym_charset, metaName))
			symbol->intlsym_flags |= INTLSYM_dropped;
		else
			return symbol;
	}

	// Now see if it is in the database

	symbol = NULL;

	AutoCacheRequest handle(tdbb, irq_charset, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
	X IN RDB$COLLATIONS
		CROSS Y IN RDB$CHARACTER_SETS OVER RDB$CHARACTER_SET_ID
		CROSS Z IN RDB$TYPES
		WITH Z.RDB$TYPE EQ Y.RDB$CHARACTER_SET_ID
		AND Z.RDB$TYPE_NAME EQ name
		AND Z.RDB$FIELD_NAME EQ "RDB$CHARACTER_SET_NAME"
		AND Y.RDB$DEFAULT_COLLATE_NAME EQ X.RDB$COLLATION_NAME*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_208, sizeof(jrd_208));
	gds__vtov ((const char*) name, (char*) jrd_209.jrd_210, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_209);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_211);
	   if (!jrd_211.jrd_212) break;;
	{
		symbol = FB_NEW(dbb->dbb_pool) dsql_intlsym(dbb->dbb_pool);
		symbol->intlsym_name = metaName;
		symbol->intlsym_flags = 0;
		symbol->intlsym_charset_id = /*X.RDB$CHARACTER_SET_ID*/
					     jrd_211.jrd_216;
		symbol->intlsym_collate_id = /*X.RDB$COLLATION_ID*/
					     jrd_211.jrd_215;
		symbol->intlsym_ttype =
			INTL_CS_COLL_TO_TTYPE(symbol->intlsym_charset_id, symbol->intlsym_collate_id);
		symbol->intlsym_bytes_per_char =
			(/*Y.RDB$BYTES_PER_CHARACTER.NULL*/
			 jrd_211.jrd_213) ? 1 : (/*Y.RDB$BYTES_PER_CHARACTER*/
	 jrd_211.jrd_214);
	}
	/*END_FOR*/
	   }
	}

	if (!symbol)
		return NULL;

	dbb->dbb_charsets.put(metaName, symbol);
	dbb->dbb_charsets_by_id.put(symbol->intlsym_charset_id, symbol);
	MET_dsql_cache_use(tdbb, SYM_intlsym_charset, metaName);

	return symbol;
}


USHORT METD_get_charset_bpc(jrd_tra* transaction, SSHORT charset_id)
{
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t _ b p c
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return NULL.
 *  Go directly to system tables & return only the
 *  number of bytes per character. Lookup by
 *  charset' id, not by name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	dsql_dbb* dbb = transaction->getDsqlAttachment();

	if (charset_id == CS_dynamic)
		charset_id = tdbb->getCharSet();

	dsql_intlsym* symbol = NULL;
	if (!dbb->dbb_charsets_by_id.get(charset_id, symbol))
	{
		const MetaName cs_name = METD_get_charset_name(transaction, charset_id);
		symbol = METD_get_charset(transaction, cs_name.length(), cs_name.c_str());
	}

	fb_assert(symbol);

	return symbol ? symbol->intlsym_bytes_per_char : 0;
}


MetaName METD_get_charset_name(jrd_tra* transaction, SSHORT charset_id)
{
   struct {
          TEXT  jrd_206 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_207;	// gds__utility 
   } jrd_205;
   struct {
          SSHORT jrd_204;	// RDB$CHARACTER_SET_ID 
   } jrd_203;
/**************************************
 *
 *  M E T D _ g e t _ c h a r s e t _ n a m e
 *
 **************************************
 *
 * Functional description
 *  Look up an international text type object.
 *  If it doesn't exist, return empty string.
 *  Go directly to system tables & return only the
 *  name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();

    if (charset_id == CS_dynamic)
		charset_id = tdbb->getCharSet();

	dsql_intlsym* sym = NULL;
	if (dbb->dbb_charsets_by_id.get(charset_id, sym))
		return sym->intlsym_name;

	MetaName name;

	AutoCacheRequest handle(tdbb, irq_cs_name, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		Y IN RDB$CHARACTER_SETS
		WITH Y.RDB$CHARACTER_SET_ID EQ charset_id*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_202, sizeof(jrd_202));
	jrd_203.jrd_204 = charset_id;
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_203);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_205);
	   if (!jrd_205.jrd_207) break;
	{
		name = /*Y.RDB$CHARACTER_SET_NAME*/
		       jrd_205.jrd_206;
	}
	/*END_FOR*/
	   }
	}

	// put new charset into hash table if needed
	METD_get_charset(transaction, name.length(), name.c_str());

	return name;
}


MetaName METD_get_default_charset(jrd_tra* transaction)
{
   struct {
          TEXT  jrd_200 [32];	// RDB$CHARACTER_SET_NAME 
          SSHORT jrd_201;	// gds__utility 
   } jrd_199;
/**************************************
 *
 *  M E T D _ g e t _ d e f a u l t _ c h a r s e t
 *
 **************************************
 *
 * Functional description
 *  Find the default character set for a database
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();
	if (dbb->dbb_no_charset)
		return NULL;

	if (dbb->dbb_dfl_charset.hasData())
		return dbb->dbb_dfl_charset;

	// Now see if it is in the database

	AutoCacheRequest handle(tdbb, irq_default_cs, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		FIRST 1 DBB IN RDB$DATABASE
		WITH DBB.RDB$CHARACTER_SET_NAME NOT MISSING*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_198, sizeof(jrd_198));
	EXE_start (tdbb, handle, transaction);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 34, (UCHAR*) &jrd_199);
	   if (!jrd_199.jrd_201) break;;
	{
		// Terminate ASCIIZ string on first trailing blank
		fb_utils::exact_name(/*DBB.RDB$CHARACTER_SET_NAME*/
				     jrd_199.jrd_200);
		const USHORT length = strlen(/*DBB.RDB$CHARACTER_SET_NAME*/
					     jrd_199.jrd_200);
		dbb->dbb_dfl_charset = /*DBB.RDB$CHARACTER_SET_NAME*/
				       jrd_199.jrd_200;
	}
	/*END_FOR*/
	   }
	}

	if (dbb->dbb_dfl_charset.isEmpty())
		dbb->dbb_no_charset = true;

	return dbb->dbb_dfl_charset;
}


bool METD_get_domain(jrd_tra* transaction, TypeClause* field, const MetaName& name) // UTF-8
{
   struct {
          bid  jrd_179;	// RDB$COMPUTED_BLR 
          SSHORT jrd_180;	// gds__utility 
          SSHORT jrd_181;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_182;	// RDB$FIELD_TYPE 
          SSHORT jrd_183;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_184;	// gds__null_flag 
          SSHORT jrd_185;	// RDB$NULL_FLAG 
          SSHORT jrd_186;	// gds__null_flag 
          SSHORT jrd_187;	// gds__null_flag 
          SSHORT jrd_188;	// RDB$CHARACTER_LENGTH 
          SSHORT jrd_189;	// gds__null_flag 
          SSHORT jrd_190;	// RDB$COLLATION_ID 
          SSHORT jrd_191;	// gds__null_flag 
          SSHORT jrd_192;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_193;	// gds__null_flag 
          SSHORT jrd_194;	// RDB$DIMENSIONS 
          SSHORT jrd_195;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_196;	// RDB$FIELD_SCALE 
          SSHORT jrd_197;	// RDB$FIELD_LENGTH 
   } jrd_178;
   struct {
          TEXT  jrd_177 [32];	// RDB$FIELD_NAME 
   } jrd_176;
/**************************************
 *
 *  M E T D _ g e t _ d o m a i n
 *
 **************************************
 *
 * Functional description
 *  Fetch domain information for field defined as 'name'
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	bool found = false;

	AutoCacheRequest handle(tdbb, irq_domain, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		FLX IN RDB$FIELDS WITH FLX.RDB$FIELD_NAME EQ name.c_str()*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_175, sizeof(jrd_175));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_176.jrd_177, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_176);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 44, (UCHAR*) &jrd_178);
	   if (!jrd_178.jrd_180) break;
	{
		found = true;
		field->length = /*FLX.RDB$FIELD_LENGTH*/
				jrd_178.jrd_197;
		field->scale = /*FLX.RDB$FIELD_SCALE*/
			       jrd_178.jrd_196;
		field->subType = /*FLX.RDB$FIELD_SUB_TYPE*/
				 jrd_178.jrd_195;
		field->dimensions = /*FLX.RDB$DIMENSIONS.NULL*/
				    jrd_178.jrd_193 ? 0 : /*FLX.RDB$DIMENSIONS*/
       jrd_178.jrd_194;

		field->charSetId = 0;
		if (!/*FLX.RDB$CHARACTER_SET_ID.NULL*/
		     jrd_178.jrd_191)
			field->charSetId = /*FLX.RDB$CHARACTER_SET_ID*/
					   jrd_178.jrd_192;
		field->collationId = 0;
		if (!/*FLX.RDB$COLLATION_ID.NULL*/
		     jrd_178.jrd_189)
			field->collationId = /*FLX.RDB$COLLATION_ID*/
					     jrd_178.jrd_190;
		field->charLength = 0;
		if (!/*FLX.RDB$CHARACTER_LENGTH.NULL*/
		     jrd_178.jrd_187)
			field->charLength = /*FLX.RDB$CHARACTER_LENGTH*/
					    jrd_178.jrd_188;

		if (!/*FLX.RDB$COMPUTED_BLR.NULL*/
		     jrd_178.jrd_186)
			field->flags |= FLD_computed;

		if (/*FLX.RDB$NULL_FLAG.NULL*/
		    jrd_178.jrd_184 || !/*FLX.RDB$NULL_FLAG*/
     jrd_178.jrd_185)
			field->flags |= FLD_nullable;

		if (/*FLX.RDB$SYSTEM_FLAG*/
		    jrd_178.jrd_183 == 1)
			field->flags |= FLD_system;

		convert_dtype(field, /*FLX.RDB$FIELD_TYPE*/
				     jrd_178.jrd_182);

		if (/*FLX.RDB$FIELD_TYPE*/
		    jrd_178.jrd_182 == blr_blob) {
			field->segLength = /*FLX.RDB$SEGMENT_LENGTH*/
					   jrd_178.jrd_181;
		}
	}
	/*END_FOR*/
	   }
	}

	return found;
}


USHORT METD_get_domain_default(jrd_tra* transaction, const MetaName& domain_name, bool* has_default,
	UCHAR* buffer, USHORT buff_length)
{
   struct {
          bid  jrd_172;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_173;	// gds__utility 
          SSHORT jrd_174;	// gds__null_flag 
   } jrd_171;
   struct {
          TEXT  jrd_170 [32];	// RDB$FIELD_NAME 
   } jrd_169;
/*************************************************************
 *
 *  M E T D _ g e t _ d o m a i n _ d e f a u l t
 *
 **************************************************************
 *
 * Function:
 *    Gets the default value for a domain of an existing table.
 *
 **************************************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	*has_default = false;

	dsql_dbb* dbb = transaction->getDsqlAttachment();
	USHORT result = 0;

	AutoCacheRequest handle(tdbb, irq_domain_2, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		FLD IN RDB$FIELDS WITH FLD.RDB$FIELD_NAME EQ domain_name.c_str()*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_168, sizeof(jrd_168));
	gds__vtov ((const char*) domain_name.c_str(), (char*) jrd_169.jrd_170, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_169);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_171);
	   if (!jrd_171.jrd_173) break;
	{
		bid* blob_id;
		if (!/*FLD.RDB$DEFAULT_VALUE.NULL*/
		     jrd_171.jrd_174)
		{
			blob_id = &/*FLD.RDB$DEFAULT_VALUE*/
				   jrd_171.jrd_172;
			*has_default = true;
		}
		else
			*has_default = false;

		if (*has_default)
		{
			blb* blob_handle = blb::open2(tdbb, transaction, blob_id, sizeof(blr_bpb), blr_bpb, true);

			// fetch segments. Assume buffer is big enough.
			UCHAR* ptr_in_buffer = buffer;
			while (true)
			{
				const USHORT length = blob_handle->BLB_get_segment(tdbb, ptr_in_buffer, buff_length);

				ptr_in_buffer += length;
				buff_length -= length;
				result += length;

				if (blob_handle->blb_flags & BLB_eof)
				{
					// null terminate the buffer
					*ptr_in_buffer = 0;
					break;
				}
				if (blob_handle->getFragmentSize())
					status_exception::raise(Arg::Gds(isc_segment));
				else
					continue;
			}

			try
			{
				ThreadStatusGuard status_vector(tdbb);

				blob_handle->BLB_close(tdbb);
				blob_handle = NULL;
			}
			catch (Exception&)
			{
			}

			// the default string must be of the form:
			// blr_version4 blr_literal ..... blr_eoc
			fb_assert((buffer[0] == blr_version4) || (buffer[0] == blr_version5));
			fb_assert(buffer[1] == blr_literal);
		}
		else
		{
			if (dbb->dbb_db_SQL_dialect > SQL_DIALECT_V5)
				buffer[0] = blr_version5;
			else
				buffer[0] = blr_version4;
			buffer[1] = blr_eoc;
			result = 2;
		}
	}
	/*END_FOR*/
	   }
	}

	return result;
}


dsql_udf* METD_get_function(jrd_tra* transaction, DsqlCompilerScratch* dsqlScratch,
	const QualifiedName& name)
{
   struct {
          SSHORT jrd_134;	// gds__utility 
          SSHORT jrd_135;	// gds__null_flag 
          SSHORT jrd_136;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_137;	// RDB$FIELD_LENGTH 
          SSHORT jrd_138;	// gds__null_flag 
          SSHORT jrd_139;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_140;	// RDB$FIELD_SCALE 
          SSHORT jrd_141;	// RDB$FIELD_TYPE 
   } jrd_133;
   struct {
          TEXT  jrd_132 [32];	// RDB$FIELD_SOURCE 
   } jrd_131;
   struct {
          TEXT  jrd_147 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_148;	// gds__utility 
          SSHORT jrd_149;	// gds__null_flag 
          SSHORT jrd_150;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_151;	// RDB$FIELD_LENGTH 
          SSHORT jrd_152;	// gds__null_flag 
          SSHORT jrd_153;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_154;	// RDB$FIELD_SCALE 
          SSHORT jrd_155;	// RDB$FIELD_TYPE 
          SSHORT jrd_156;	// RDB$MECHANISM 
          SSHORT jrd_157;	// RDB$ARGUMENT_POSITION 
          SSHORT jrd_158;	// gds__null_flag 
   } jrd_146;
   struct {
          TEXT  jrd_144 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_145 [32];	// RDB$FUNCTION_NAME 
   } jrd_143;
   struct {
          SSHORT jrd_164;	// gds__utility 
          SSHORT jrd_165;	// RDB$RETURN_ARGUMENT 
          SSHORT jrd_166;	// gds__null_flag 
          SSHORT jrd_167;	// RDB$PRIVATE_FLAG 
   } jrd_163;
   struct {
          TEXT  jrd_161 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_162 [32];	// RDB$FUNCTION_NAME 
   } jrd_160;
/**************************************
 *
 *  M E T D _ g e t _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up a user defined function.  If it doesn't exist,
 *  return NULL.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();
	QualifiedName metaName(name);

	bool maybeUnqualified = dsqlScratch->package.hasData() && metaName.package.isEmpty();
	if (maybeUnqualified)
		metaName.package = dsqlScratch->package;

	// Start by seeing if symbol is already defined

	dsql_udf* userFunc = NULL;
	if (dbb->dbb_functions.get(metaName, userFunc))
	{
		if (userFunc->udf_private && metaName.package != dsqlScratch->package)
		{
			status_exception::raise(Arg::Gds(isc_private_function) <<
				Arg::Str(metaName.identifier) << Arg::Str(metaName.package));
		}

		if (MET_dsql_cache_use(tdbb, SYM_udf, metaName.identifier, metaName.package))
			userFunc->udf_flags |= UDF_dropped;

		return userFunc;
	}

	// Now see if it is in the database

	USHORT return_arg = 0;

	while (!userFunc)
	{
		AutoCacheRequest handle1(tdbb, irq_function, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE transaction)
			X IN RDB$FUNCTIONS WITH
			X.RDB$FUNCTION_NAME EQ metaName.identifier.c_str() AND
			X.RDB$PACKAGE_NAME EQUIV NULLIF(metaName.package.c_str(), '')*/
		{
		handle1.compile(tdbb, (UCHAR*) jrd_159, sizeof(jrd_159));
		gds__vtov ((const char*) metaName.package.c_str(), (char*) jrd_160.jrd_161, 32);
		gds__vtov ((const char*) metaName.identifier.c_str(), (char*) jrd_160.jrd_162, 32);
		EXE_start (tdbb, handle1, transaction);
		EXE_send (tdbb, handle1, 0, 64, (UCHAR*) &jrd_160);
		while (1)
		   {
		   EXE_receive (tdbb, handle1, 1, 8, (UCHAR*) &jrd_163);
		   if (!jrd_163.jrd_164) break;
		{
			userFunc = FB_NEW(dbb->dbb_pool) dsql_udf(dbb->dbb_pool);
			userFunc->udf_name = metaName;
			userFunc->udf_private = !/*X.RDB$PRIVATE_FLAG.NULL*/
						 jrd_163.jrd_166 && /*X.RDB$PRIVATE_FLAG*/
    jrd_163.jrd_167 != 0;

			return_arg = /*X.RDB$RETURN_ARGUMENT*/
				     jrd_163.jrd_165;
		}
		/*END_FOR*/
		   }
		}

		if (!userFunc)
		{
			if (maybeUnqualified)
			{
				maybeUnqualified = false;
				metaName.package = "";
			}
			else
				return NULL;
		}
	}

	AutoCacheRequest handle2(tdbb, irq_func_return, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
		X IN RDB$FUNCTION_ARGUMENTS WITH
		X.RDB$FUNCTION_NAME EQ metaName.identifier.c_str() AND
		X.RDB$PACKAGE_NAME EQUIV NULLIF(metaName.package.c_str(), '')
		SORTED BY X.RDB$ARGUMENT_POSITION*/
	{
	handle2.compile(tdbb, (UCHAR*) jrd_142, sizeof(jrd_142));
	gds__vtov ((const char*) metaName.package.c_str(), (char*) jrd_143.jrd_144, 32);
	gds__vtov ((const char*) metaName.identifier.c_str(), (char*) jrd_143.jrd_145, 32);
	EXE_start (tdbb, handle2, transaction);
	EXE_send (tdbb, handle2, 0, 64, (UCHAR*) &jrd_143);
	while (1)
	   {
	   EXE_receive (tdbb, handle2, 1, 54, (UCHAR*) &jrd_146);
	   if (!jrd_146.jrd_148) break;
	{
		if (!/*X.RDB$FIELD_SOURCE.NULL*/
		     jrd_146.jrd_158)
		{
			AutoCacheRequest handle3(tdbb, irq_func_ret_fld, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE handle3 TRANSACTION_HANDLE transaction)
				F IN RDB$FIELDS WITH
				F.RDB$FIELD_NAME EQ X.RDB$FIELD_SOURCE*/
			{
			handle3.compile(tdbb, (UCHAR*) jrd_130, sizeof(jrd_130));
			gds__vtov ((const char*) jrd_146.jrd_147, (char*) jrd_131.jrd_132, 32);
			EXE_start (tdbb, handle3, transaction);
			EXE_send (tdbb, handle3, 0, 32, (UCHAR*) &jrd_131);
			while (1)
			   {
			   EXE_receive (tdbb, handle3, 1, 16, (UCHAR*) &jrd_133);
			   if (!jrd_133.jrd_134) break;
			{
				if (/*X.RDB$ARGUMENT_POSITION*/
				    jrd_146.jrd_157 == return_arg)
				{
					userFunc->udf_dtype = (/*F.RDB$FIELD_TYPE*/
							       jrd_133.jrd_141 != blr_blob) ?
						gds_cvt_blr_dtype[/*F.RDB$FIELD_TYPE*/
								  jrd_133.jrd_141] : dtype_blob;
					userFunc->udf_scale = /*F.RDB$FIELD_SCALE*/
							      jrd_133.jrd_140;

					if (!/*F.RDB$FIELD_SUB_TYPE.NULL*/
					     jrd_133.jrd_138) {
						userFunc->udf_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
									 jrd_133.jrd_139;
					}
					else {
						userFunc->udf_sub_type = 0;
					}
					// CVC: We are overcoming a bug in ddl.cpp:put_field()
					// when any field is defined: the length is not given for blobs.
					if (/*F.RDB$FIELD_TYPE*/
					    jrd_133.jrd_141 == blr_blob)
						userFunc->udf_length = sizeof(ISC_QUAD);
					else
						userFunc->udf_length = /*F.RDB$FIELD_LENGTH*/
								       jrd_133.jrd_137;

					if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
					     jrd_133.jrd_135) {
						userFunc->udf_character_set_id = /*F.RDB$CHARACTER_SET_ID*/
										 jrd_133.jrd_136;
					}
				}
				else
				{
					DSC d;
					d.dsc_dtype = (/*F.RDB$FIELD_TYPE*/
						       jrd_133.jrd_141 != blr_blob) ?
						gds_cvt_blr_dtype[/*F.RDB$FIELD_TYPE*/
								  jrd_133.jrd_141] : dtype_blob;
					// dimitr: adjust the UDF arguments for CSTRING
					if (d.dsc_dtype == dtype_cstring) {
						d.dsc_dtype = dtype_text;
					}
					d.dsc_scale = /*F.RDB$FIELD_SCALE*/
						      jrd_133.jrd_140;
					if (!/*F.RDB$FIELD_SUB_TYPE.NULL*/
					     jrd_133.jrd_138) {
						d.dsc_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
								 jrd_133.jrd_139;
					}
					else {
						d.dsc_sub_type = 0;
					}
					d.dsc_length = /*F.RDB$FIELD_LENGTH*/
						       jrd_133.jrd_137;
					if (d.dsc_dtype == dtype_varying) {
						d.dsc_length += sizeof(USHORT);
					}
					d.dsc_address = NULL;

					if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
					     jrd_133.jrd_135)
					{
						if (d.dsc_dtype != dtype_blob) {
							d.dsc_ttype() = /*F.RDB$CHARACTER_SET_ID*/
									jrd_133.jrd_136;
						}
						else {
							d.dsc_scale = /*F.RDB$CHARACTER_SET_ID*/
								      jrd_133.jrd_136;
						}
					}

					if (/*X.RDB$MECHANISM*/
					    jrd_146.jrd_156 != FUN_value && /*X.RDB$MECHANISM*/
		 jrd_146.jrd_156 != FUN_reference)
					{
						d.dsc_flags = DSC_nullable;
					}

					userFunc->udf_arguments.add(d);
				}
			}
			/*END_FOR*/
			   }
			}
		}
		else
		{
			if (/*X.RDB$ARGUMENT_POSITION*/
			    jrd_146.jrd_157 == return_arg)
			{
				userFunc->udf_dtype = (/*X.RDB$FIELD_TYPE*/
						       jrd_146.jrd_155 != blr_blob) ?
					gds_cvt_blr_dtype[/*X.RDB$FIELD_TYPE*/
							  jrd_146.jrd_155] : dtype_blob;
				userFunc->udf_scale = /*X.RDB$FIELD_SCALE*/
						      jrd_146.jrd_154;

				if (!/*X.RDB$FIELD_SUB_TYPE.NULL*/
				     jrd_146.jrd_152) {
					userFunc->udf_sub_type = /*X.RDB$FIELD_SUB_TYPE*/
								 jrd_146.jrd_153;
				}
				else {
					userFunc->udf_sub_type = 0;
				}
				// CVC: We are overcoming a bug in ddl.c:put_field()
				// when any field is defined: the length is not given for blobs.
				if (/*X.RDB$FIELD_TYPE*/
				    jrd_146.jrd_155 == blr_blob)
					userFunc->udf_length = sizeof(ISC_QUAD);
				else
					userFunc->udf_length = /*X.RDB$FIELD_LENGTH*/
							       jrd_146.jrd_151;

				if (!/*X.RDB$CHARACTER_SET_ID.NULL*/
				     jrd_146.jrd_149) {
					userFunc->udf_character_set_id = /*X.RDB$CHARACTER_SET_ID*/
									 jrd_146.jrd_150;
				}
			}
			else
			{
				DSC d;
				d.dsc_dtype = (/*X.RDB$FIELD_TYPE*/
					       jrd_146.jrd_155 != blr_blob) ?
					gds_cvt_blr_dtype[/*X.RDB$FIELD_TYPE*/
							  jrd_146.jrd_155] : dtype_blob;
				// dimitr: adjust the UDF arguments for CSTRING
				if (d.dsc_dtype == dtype_cstring) {
					d.dsc_dtype = dtype_text;
				}
				d.dsc_scale = /*X.RDB$FIELD_SCALE*/
					      jrd_146.jrd_154;
				if (!/*X.RDB$FIELD_SUB_TYPE.NULL*/
				     jrd_146.jrd_152) {
					d.dsc_sub_type = /*X.RDB$FIELD_SUB_TYPE*/
							 jrd_146.jrd_153;
				}
				else {
					d.dsc_sub_type = 0;
				}
				d.dsc_length = /*X.RDB$FIELD_LENGTH*/
					       jrd_146.jrd_151;
				if (d.dsc_dtype == dtype_varying) {
					d.dsc_length += sizeof(USHORT);
				}
				d.dsc_address = NULL;

				if (!/*X.RDB$CHARACTER_SET_ID.NULL*/
				     jrd_146.jrd_149)
				{
					if (d.dsc_dtype != dtype_blob) {
						d.dsc_ttype() = /*X.RDB$CHARACTER_SET_ID*/
								jrd_146.jrd_150;
					}
					else {
						d.dsc_scale = /*X.RDB$CHARACTER_SET_ID*/
							      jrd_146.jrd_150;
					}
				}

				if (/*X.RDB$MECHANISM*/
				    jrd_146.jrd_156 != FUN_value && /*X.RDB$MECHANISM*/
		 jrd_146.jrd_156 != FUN_reference)
				{
					d.dsc_flags = DSC_nullable;
				}

				userFunc->udf_arguments.add(d);
			}
		}
	}
	/*END_FOR*/
	   }
	}

	// Adjust the return type & length of the UDF to account for
	// cstring & varying.  While a UDF can return CSTRING, we convert it
	// to VARCHAR for manipulation as CSTRING is not a SQL type.

	if (userFunc->udf_dtype == dtype_cstring)
	{
		userFunc->udf_dtype = dtype_varying;
		userFunc->udf_length += sizeof(USHORT);
		if (userFunc->udf_length > MAX_SSHORT)
			userFunc->udf_length = MAX_SSHORT;
	}
	else if (userFunc->udf_dtype == dtype_varying)
		userFunc->udf_length += sizeof(USHORT);

	dbb->dbb_functions.put(userFunc->udf_name, userFunc);

	if (userFunc->udf_private && metaName.package != dsqlScratch->package)
	{
		status_exception::raise(Arg::Gds(isc_private_function) <<
			Arg::Str(metaName.identifier) << Arg::Str(metaName.package));
	}

	MET_dsql_cache_use(tdbb, SYM_udf, userFunc->udf_name.identifier, userFunc->udf_name.package);

	return userFunc;
}


void METD_get_primary_key(jrd_tra* transaction, const MetaName& relationName,
	Array<NestConst<FieldNode> >& fields)
{
   struct {
          TEXT  jrd_128 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_129;	// gds__utility 
   } jrd_127;
   struct {
          TEXT  jrd_126 [32];	// RDB$RELATION_NAME 
   } jrd_125;
/**************************************
 *
 *  M E T D _ g e t _ p r i m a r y _ k e y
 *
 **************************************
 *
 * Functional description
 *  Lookup the fields for the primary key
 *  index on a relation, returning a list
 *  node of the fields.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	MemoryPool& pool = *tdbb->getDefaultPool();

	validateTransaction(transaction);

	AutoCacheRequest handle(tdbb, irq_primary_key, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		X IN RDB$INDICES CROSS
		Y IN RDB$INDEX_SEGMENTS
		OVER RDB$INDEX_NAME CROSS
		Z IN RDB$RELATION_CONSTRAINTS
		OVER RDB$INDEX_NAME
		WITH Z.RDB$RELATION_NAME EQ relationName.c_str()
		AND Z.RDB$CONSTRAINT_TYPE EQ "PRIMARY KEY"
		SORTED BY Y.RDB$FIELD_POSITION*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_124, sizeof(jrd_124));
	gds__vtov ((const char*) relationName.c_str(), (char*) jrd_125.jrd_126, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_125);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 34, (UCHAR*) &jrd_127);
	   if (!jrd_127.jrd_129) break;
	{
		FieldNode* fieldNode = FB_NEW(pool) FieldNode(pool);
		fieldNode->dsqlName = /*Y.RDB$FIELD_NAME*/
				      jrd_127.jrd_128;
		fields.add(fieldNode);
	}
	/*END_FOR*/
	   }
	}
}


dsql_prc* METD_get_procedure(jrd_tra* transaction, DsqlCompilerScratch* dsqlScratch,
	const QualifiedName& name)
{
   struct {
          bid  jrd_85;	// RDB$DEFAULT_VALUE 
          TEXT  jrd_86 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_87 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_88 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_89 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_90 [32];	// RDB$PARAMETER_NAME 
          bid  jrd_91;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_92;	// gds__utility 
          SSHORT jrd_93;	// gds__null_flag 
          SSHORT jrd_94;	// gds__null_flag 
          SSHORT jrd_95;	// gds__null_flag 
          SSHORT jrd_96;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_97;	// RDB$NULL_FLAG 
          SSHORT jrd_98;	// RDB$FIELD_TYPE 
          SSHORT jrd_99;	// gds__null_flag 
          SSHORT jrd_100;	// RDB$COLLATION_ID 
          SSHORT jrd_101;	// gds__null_flag 
          SSHORT jrd_102;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_103;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_104;	// RDB$FIELD_SCALE 
          SSHORT jrd_105;	// RDB$FIELD_LENGTH 
          SSHORT jrd_106;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_107;	// gds__null_flag 
          SSHORT jrd_108;	// RDB$PARAMETER_MECHANISM 
          SSHORT jrd_109;	// gds__null_flag 
          SSHORT jrd_110;	// RDB$NULL_FLAG 
          SSHORT jrd_111;	// gds__null_flag 
          SSHORT jrd_112;	// gds__null_flag 
          SSHORT jrd_113;	// RDB$COLLATION_ID 
   } jrd_84;
   struct {
          TEXT  jrd_81 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_82 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_83;	// RDB$PARAMETER_TYPE 
   } jrd_80;
   struct {
          TEXT  jrd_119 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_120;	// gds__utility 
          SSHORT jrd_121;	// gds__null_flag 
          SSHORT jrd_122;	// RDB$PRIVATE_FLAG 
          SSHORT jrd_123;	// RDB$PROCEDURE_ID 
   } jrd_118;
   struct {
          TEXT  jrd_116 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_117 [32];	// RDB$PROCEDURE_NAME 
   } jrd_115;
/**************************************
 *
 *  M E T D _ g e t _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Look up a procedure.  If it doesn't exist, return NULL.
 *  If it does, fetch field information as well.
 *  If it is marked dropped, try to read from system tables
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();

	// ASF: I've removed the code where we verify if the procedure being looked up is the one being
	// defined (dsqlScratch->procedure). This code is totally incorrect, not considering
	// transactions and savepoints, hence being incompatible with packages).
	// Example (with autocommit off):
	//
	// SQL> create procedure p1 as begin end!
	// SQL> create procedure p2 as begin execute procedure p1; end!
	// SQL> rollback!
	// SQL> execute procedure p2!
	// Statement failed, SQLSTATE = 42000
	// Dynamic SQL Error
	// -SQL error code = -204
	// -Procedure unknown
	// -P2
	// SQL> execute procedure p1!
	// Statement failed, SQLSTATE = 42000
	// invalid request BLR at offset 5
	// -procedure P1 is not defined
	//
	// The side effect is that this occur in more cases now:
	//
	// SQL> create procedure p as begin execute procedure p; execute procedure p2; end!
	// Statement failed, SQLSTATE = 42000
	// Dynamic SQL Error
	// -SQL error code = -204
	// -Procedure unknown
	// -P2
	// SQL> execute procedure p!
	// Statement failed, SQLSTATE = 42000
	// invalid request BLR at offset 4
	// -procedure P is not defined
	//
	// I hope for a solution, involving savepoint logic.

	QualifiedName metaName(name);

	bool maybeUnqualified = dsqlScratch->package.hasData() && metaName.package.isEmpty();
	if (maybeUnqualified)
		metaName.package = dsqlScratch->package;

	// Start by seeing if symbol is already defined

	dsql_prc* procedure = NULL;
	if (dbb->dbb_procedures.get(metaName, procedure))
	{
		if (procedure->prc_private && metaName.package != dsqlScratch->package)
		{
			status_exception::raise(Arg::Gds(isc_private_procedure) <<
				Arg::Str(metaName.identifier) << Arg::Str(metaName.package));
		}

		if (MET_dsql_cache_use(tdbb, SYM_procedure, metaName.identifier, metaName.package))
			procedure->prc_flags |= PRC_dropped;

		return procedure;
	}

	// now see if it is in the database

	while (!procedure)
	{
		AutoCacheRequest handle1(tdbb, irq_procedure, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE transaction)
			X IN RDB$PROCEDURES
			WITH X.RDB$PROCEDURE_NAME EQ metaName.identifier.c_str() AND
				 X.RDB$PACKAGE_NAME EQUIV NULLIF(metaName.package.c_str(), '')*/
		{
		handle1.compile(tdbb, (UCHAR*) jrd_114, sizeof(jrd_114));
		gds__vtov ((const char*) metaName.package.c_str(), (char*) jrd_115.jrd_116, 32);
		gds__vtov ((const char*) metaName.identifier.c_str(), (char*) jrd_115.jrd_117, 32);
		EXE_start (tdbb, handle1, transaction);
		EXE_send (tdbb, handle1, 0, 64, (UCHAR*) &jrd_115);
		while (1)
		   {
		   EXE_receive (tdbb, handle1, 1, 40, (UCHAR*) &jrd_118);
		   if (!jrd_118.jrd_120) break;
		{
			fb_utils::exact_name(/*X.RDB$OWNER_NAME*/
					     jrd_118.jrd_119);

			procedure = FB_NEW(dbb->dbb_pool) dsql_prc(dbb->dbb_pool);
			procedure->prc_id = /*X.RDB$PROCEDURE_ID*/
					    jrd_118.jrd_123;
			procedure->prc_name = metaName;
			procedure->prc_owner = /*X.RDB$OWNER_NAME*/
					       jrd_118.jrd_119;
			procedure->prc_private = !/*X.RDB$PRIVATE_FLAG.NULL*/
						  jrd_118.jrd_121 && /*X.RDB$PRIVATE_FLAG*/
    jrd_118.jrd_122 != 0;
		}
		/*END_FOR*/
		   }
		}

		if (!procedure)
		{
			if (maybeUnqualified)
			{
				maybeUnqualified = false;
				metaName.package = "";
			}
			else
				return NULL;
		}
	}

	// Lookup parameter stuff

	for (int type = 0; type < 2; type++)
	{
		dsql_fld** const ptr = type ? &procedure->prc_outputs : &procedure->prc_inputs;

		SSHORT count = 0, defaults = 0;

		AutoCacheRequest handle2(tdbb, irq_parameters, IRQ_REQUESTS);

		/*FOR (REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
			PR IN RDB$PROCEDURE_PARAMETERS
			CROSS FLD IN RDB$FIELDS
			WITH FLD.RDB$FIELD_NAME EQ PR.RDB$FIELD_SOURCE AND
				 PR.RDB$PROCEDURE_NAME EQ metaName.identifier.c_str() AND
				 PR.RDB$PARAMETER_TYPE = type AND
				 PR.RDB$PACKAGE_NAME EQUIV NULLIF(metaName.package.c_str(), '')
			SORTED BY DESCENDING PR.RDB$PARAMETER_NUMBER*/
		{
		handle2.compile(tdbb, (UCHAR*) jrd_79, sizeof(jrd_79));
		gds__vtov ((const char*) metaName.package.c_str(), (char*) jrd_80.jrd_81, 32);
		gds__vtov ((const char*) metaName.identifier.c_str(), (char*) jrd_80.jrd_82, 32);
		jrd_80.jrd_83 = type;
		EXE_start (tdbb, handle2, transaction);
		EXE_send (tdbb, handle2, 0, 66, (UCHAR*) &jrd_80);
		while (1)
		   {
		   EXE_receive (tdbb, handle2, 1, 220, (UCHAR*) &jrd_84);
		   if (!jrd_84.jrd_92) break;
		{
			const SSHORT pr_collation_id_null = /*PR.RDB$COLLATION_ID.NULL*/
							    jrd_84.jrd_112;
			const SSHORT pr_collation_id = /*PR.RDB$COLLATION_ID*/
						       jrd_84.jrd_113;

			const SSHORT pr_default_value_null = /*PR.RDB$DEFAULT_VALUE.NULL*/
							     jrd_84.jrd_111;

			const SSHORT pr_null_flag_null = /*PR.RDB$NULL_FLAG.NULL*/
							 jrd_84.jrd_109;
			const SSHORT pr_null_flag = /*PR.RDB$NULL_FLAG*/
						    jrd_84.jrd_110;

			const bool pr_type_of =
				(!/*PR.RDB$PARAMETER_MECHANISM.NULL*/
				  jrd_84.jrd_107 && /*PR.RDB$PARAMETER_MECHANISM*/
    jrd_84.jrd_108 == prm_mech_type_of);

			count++;
			// allocate the field block

			fb_utils::exact_name(/*PR.RDB$PARAMETER_NAME*/
					     jrd_84.jrd_90);
			fb_utils::exact_name(/*PR.RDB$FIELD_SOURCE*/
					     jrd_84.jrd_89);

			dsql_fld* parameter = FB_NEW(dbb->dbb_pool) dsql_fld(dbb->dbb_pool);
			parameter->fld_next = *ptr;
			*ptr = parameter;

			// get parameter information

			parameter->fld_name = /*PR.RDB$PARAMETER_NAME*/
					      jrd_84.jrd_90;
			parameter->fieldSource = /*PR.RDB$FIELD_SOURCE*/
						 jrd_84.jrd_89;

			parameter->fld_id = /*PR.RDB$PARAMETER_NUMBER*/
					    jrd_84.jrd_106;
			parameter->length = /*FLD.RDB$FIELD_LENGTH*/
					    jrd_84.jrd_105;
			parameter->scale = /*FLD.RDB$FIELD_SCALE*/
					   jrd_84.jrd_104;
			parameter->subType = /*FLD.RDB$FIELD_SUB_TYPE*/
					     jrd_84.jrd_103;
			parameter->fld_procedure = procedure;

			if (!/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_84.jrd_101)
				parameter->charSetId = /*FLD.RDB$CHARACTER_SET_ID*/
						       jrd_84.jrd_102;

			if (!pr_collation_id_null)
				parameter->collationId = pr_collation_id;
			else if (!/*FLD.RDB$COLLATION_ID.NULL*/
				  jrd_84.jrd_99)
				parameter->collationId = /*FLD.RDB$COLLATION_ID*/
							 jrd_84.jrd_100;

			convert_dtype(parameter, /*FLD.RDB$FIELD_TYPE*/
						 jrd_84.jrd_98);

			if (!pr_null_flag_null)
			{
				if (!pr_null_flag)
					parameter->flags |= FLD_nullable;
			}
			else if (!/*FLD.RDB$NULL_FLAG*/
				  jrd_84.jrd_97 || pr_type_of)
				parameter->flags |= FLD_nullable;

			if (/*FLD.RDB$FIELD_TYPE*/
			    jrd_84.jrd_98 == blr_blob)
				parameter->segLength = /*FLD.RDB$SEGMENT_LENGTH*/
						       jrd_84.jrd_96;

			if (!/*PR.RDB$FIELD_NAME.NULL*/
			     jrd_84.jrd_95)
			{
				fb_utils::exact_name(/*PR.RDB$FIELD_NAME*/
						     jrd_84.jrd_88);
				parameter->typeOfName = /*PR.RDB$FIELD_NAME*/
							jrd_84.jrd_88;
			}

			if (!/*PR.RDB$RELATION_NAME.NULL*/
			     jrd_84.jrd_94)
			{
				fb_utils::exact_name(/*PR.RDB$RELATION_NAME*/
						     jrd_84.jrd_87);
				parameter->typeOfTable = /*PR.RDB$RELATION_NAME*/
							 jrd_84.jrd_87;
			}

			if (type == 0 &&
				(!pr_default_value_null ||
					(fb_utils::implicit_domain(/*FLD.RDB$FIELD_NAME*/
								   jrd_84.jrd_86) && !/*FLD.RDB$DEFAULT_VALUE.NULL*/
      jrd_84.jrd_93)))
			{
				defaults++;
			}
		}
		/*END_FOR*/
		   }
		}

		if (type)
			procedure->prc_out_count = count;
		else
		{
			procedure->prc_in_count = count;
			procedure->prc_def_count = defaults;
		}
	}

	dbb->dbb_procedures.put(procedure->prc_name, procedure);

	if (procedure->prc_private && metaName.package != dsqlScratch->package)
	{
		status_exception::raise(Arg::Gds(isc_private_procedure) <<
			Arg::Str(metaName.identifier) << Arg::Str(metaName.package));
	}

	MET_dsql_cache_use(tdbb, SYM_procedure, procedure->prc_name.identifier,
		procedure->prc_name.package);

	return procedure;
}


dsql_rel* METD_get_relation(jrd_tra* transaction, DsqlCompilerScratch* dsqlScratch,
	const MetaName& name)
{
   struct {
          bid  jrd_37;	// RDB$COMPUTED_BLR 
          TEXT  jrd_38 [32];	// RDB$FIELD_SOURCE 
          TEXT  jrd_39 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_40;	// gds__utility 
          SSHORT jrd_41;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_42;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_43;	// RDB$NULL_FLAG 
          SSHORT jrd_44;	// RDB$NULL_FLAG 
          SSHORT jrd_45;	// gds__null_flag 
          SSHORT jrd_46;	// RDB$COLLATION_ID 
          SSHORT jrd_47;	// gds__null_flag 
          SSHORT jrd_48;	// RDB$COLLATION_ID 
          SSHORT jrd_49;	// gds__null_flag 
          SSHORT jrd_50;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_51;	// gds__null_flag 
          SSHORT jrd_52;	// RDB$DIMENSIONS 
          SSHORT jrd_53;	// RDB$SEGMENT_LENGTH 
          SSHORT jrd_54;	// RDB$FIELD_TYPE 
          SSHORT jrd_55;	// gds__null_flag 
          SSHORT jrd_56;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_57;	// RDB$FIELD_SCALE 
          SSHORT jrd_58;	// RDB$FIELD_LENGTH 
          SSHORT jrd_59;	// gds__null_flag 
          SSHORT jrd_60;	// RDB$FIELD_ID 
   } jrd_36;
   struct {
          TEXT  jrd_35 [32];	// RDB$RELATION_NAME 
   } jrd_34;
   struct {
          TEXT  jrd_65 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_66;	// RDB$VIEW_BLR 
          TEXT  jrd_67 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_68;	// gds__utility 
          SSHORT jrd_69;	// gds__null_flag 
          SSHORT jrd_70;	// gds__null_flag 
          SSHORT jrd_71;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_72;	// gds__null_flag 
          SSHORT jrd_73;	// RDB$RELATION_ID 
   } jrd_64;
   struct {
          TEXT  jrd_63 [32];	// RDB$RELATION_NAME 
   } jrd_62;
   struct {
          SSHORT jrd_78;	// gds__utility 
   } jrd_77;
   struct {
          TEXT  jrd_76 [32];	// RDB$RELATION_NAME 
   } jrd_75;
/**************************************
 *
 *  M E T D _ g e t _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Look up a relation.  If it doesn't exist, return NULL.
 *  If it does, fetch field information as well.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_dbb* dbb = transaction->getDsqlAttachment();

	// See if the relation is the one currently being defined in this statement

	dsql_rel* temp = dsqlScratch->relation;
	if (temp != NULL && temp->rel_name == name)
		return temp;

	// Start by seeing if symbol is already defined

	if (dbb->dbb_relations.get(name, temp) && !(temp->rel_flags & REL_dropped))
	{
		if (MET_dsql_cache_use(tdbb, SYM_relation, name))
			temp->rel_flags |= REL_dropped;
		else
			return temp;
	}

	// If the relation id or any of the field ids have not yet been assigned,
	// and this is a type of statement which does not use ids, prepare a
	// temporary relation block to provide information without caching it

	bool permanent = true;

	AutoCacheRequest handle1(tdbb, irq_rel_ids, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE transaction)
		REL IN RDB$RELATIONS
		CROSS RFR IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
		WITH REL.RDB$RELATION_NAME EQ name.c_str()
		AND (REL.RDB$RELATION_ID MISSING OR RFR.RDB$FIELD_ID MISSING)*/
	{
	handle1.compile(tdbb, (UCHAR*) jrd_74, sizeof(jrd_74));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_75.jrd_76, 32);
	EXE_start (tdbb, handle1, transaction);
	EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_75);
	while (1)
	   {
	   EXE_receive (tdbb, handle1, 1, 2, (UCHAR*) &jrd_77);
	   if (!jrd_77.jrd_78) break;
	{
		permanent = false;
	}
	/*END_FOR*/
	   }
	}

	// Now see if it is in the database

	MemoryPool& pool = permanent ? dbb->dbb_pool : *tdbb->getDefaultPool();

	dsql_rel* relation = NULL;

	AutoCacheRequest handle2(tdbb, irq_relation, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
		X IN RDB$RELATIONS WITH X.RDB$RELATION_NAME EQ name.c_str()*/
	{
	handle2.compile(tdbb, (UCHAR*) jrd_61, sizeof(jrd_61));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_62.jrd_63, 32);
	EXE_start (tdbb, handle2, transaction);
	EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_62);
	while (1)
	   {
	   EXE_receive (tdbb, handle2, 1, 308, (UCHAR*) &jrd_64);
	   if (!jrd_64.jrd_68) break;
	{
		fb_utils::exact_name(/*X.RDB$OWNER_NAME*/
				     jrd_64.jrd_67);

		// Allocate from default or permanent pool as appropriate

		if (!/*X.RDB$RELATION_ID.NULL*/
		     jrd_64.jrd_72)
		{
			relation = FB_NEW(pool) dsql_rel(pool);
			relation->rel_id = /*X.RDB$RELATION_ID*/
					   jrd_64.jrd_73;
		}
		else if (!DDL_ids(dsqlScratch))
			relation = FB_NEW(pool) dsql_rel(pool);

		// fill out the relation information

		if (relation)
		{
			relation->rel_name = name;
			relation->rel_owner = /*X.RDB$OWNER_NAME*/
					      jrd_64.jrd_67;
			if (!(relation->rel_dbkey_length = /*X.RDB$DBKEY_LENGTH*/
							   jrd_64.jrd_71))
				relation->rel_dbkey_length = 8;
			// CVC: let's see if this is a table or a view.
			if (!/*X.RDB$VIEW_BLR.NULL*/
			     jrd_64.jrd_70)
				relation->rel_flags |= REL_view;
			if (!/*X.RDB$EXTERNAL_FILE.NULL*/
			     jrd_64.jrd_69)
				relation->rel_flags |= REL_external;
		}
	}
	/*END_FOR*/
	   }
	}

	if (!relation)
		return NULL;

	// Lookup field stuff

	dsql_fld** ptr = &relation->rel_fields;

	AutoCacheRequest handle3(tdbb, irq_fields, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle3 TRANSACTION_HANDLE transaction)
		FLX IN RDB$FIELDS CROSS
		RFR IN RDB$RELATION_FIELDS
		WITH FLX.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
		AND RFR.RDB$RELATION_NAME EQ name.c_str()
		SORTED BY RFR.RDB$FIELD_POSITION*/
	{
	handle3.compile(tdbb, (UCHAR*) jrd_33, sizeof(jrd_33));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_34.jrd_35, 32);
	EXE_start (tdbb, handle3, transaction);
	EXE_send (tdbb, handle3, 0, 32, (UCHAR*) &jrd_34);
	while (1)
	   {
	   EXE_receive (tdbb, handle3, 1, 114, (UCHAR*) &jrd_36);
	   if (!jrd_36.jrd_40) break;
	{
		// allocate the field block

		fb_utils::exact_name(/*RFR.RDB$FIELD_NAME*/
				     jrd_36.jrd_39);
		fb_utils::exact_name(/*RFR.RDB$FIELD_SOURCE*/
				     jrd_36.jrd_38);

		// Allocate from default or permanent pool as appropriate

		dsql_fld* field = NULL;

		if (!/*RFR.RDB$FIELD_ID.NULL*/
		     jrd_36.jrd_59)
		{
			field = FB_NEW(pool) dsql_fld(pool);
			field->fld_id = /*RFR.RDB$FIELD_ID*/
					jrd_36.jrd_60;
		}
		else if (!DDL_ids(dsqlScratch))
			field = FB_NEW(pool) dsql_fld(pool);

		if (field)
		{
			*ptr = field;
			ptr = &field->fld_next;

			// get field information

			field->fld_name = /*RFR.RDB$FIELD_NAME*/
					  jrd_36.jrd_39;
			field->fieldSource = /*RFR.RDB$FIELD_SOURCE*/
					     jrd_36.jrd_38;
			field->length = /*FLX.RDB$FIELD_LENGTH*/
					jrd_36.jrd_58;
			field->scale = /*FLX.RDB$FIELD_SCALE*/
				       jrd_36.jrd_57;
			field->subType = /*FLX.RDB$FIELD_SUB_TYPE*/
					 jrd_36.jrd_56;
			field->fld_relation = relation;

			if (!/*FLX.RDB$COMPUTED_BLR.NULL*/
			     jrd_36.jrd_55)
				field->flags |= FLD_computed;

			convert_dtype(field, /*FLX.RDB$FIELD_TYPE*/
					     jrd_36.jrd_54);

			if (/*FLX.RDB$FIELD_TYPE*/
			    jrd_36.jrd_54 == blr_blob) {
				field->segLength = /*FLX.RDB$SEGMENT_LENGTH*/
						   jrd_36.jrd_53;
			}

			if (!/*FLX.RDB$DIMENSIONS.NULL*/
			     jrd_36.jrd_51 && /*FLX.RDB$DIMENSIONS*/
    jrd_36.jrd_52)
			{
				field->elementDtype = field->dtype;
				field->elementLength = field->length;
				field->dtype = dtype_array;
				field->length = sizeof(ISC_QUAD);
				field->dimensions = /*FLX.RDB$DIMENSIONS*/
						    jrd_36.jrd_52;
			}

			if (!/*FLX.RDB$CHARACTER_SET_ID.NULL*/
			     jrd_36.jrd_49)
				field->charSetId = /*FLX.RDB$CHARACTER_SET_ID*/
						   jrd_36.jrd_50;

			if (!/*RFR.RDB$COLLATION_ID.NULL*/
			     jrd_36.jrd_47)
				field->collationId = /*RFR.RDB$COLLATION_ID*/
						     jrd_36.jrd_48;
			else if (!/*FLX.RDB$COLLATION_ID.NULL*/
				  jrd_36.jrd_45)
				field->collationId = /*FLX.RDB$COLLATION_ID*/
						     jrd_36.jrd_46;

			if (!(/*RFR.RDB$NULL_FLAG*/
			      jrd_36.jrd_44 || /*FLX.RDB$NULL_FLAG*/
    jrd_36.jrd_43) || (relation->rel_flags & REL_view))
			{
				field->flags |= FLD_nullable;
			}

			if (/*RFR.RDB$SYSTEM_FLAG*/
			    jrd_36.jrd_42 == 1 || /*FLX.RDB$SYSTEM_FLAG*/
	 jrd_36.jrd_41 == 1)
				field->flags |= FLD_system;
		}
	}
	/*END_FOR*/
	   }
	}

	if (dbb->dbb_relations.get(name, temp) && !(temp->rel_flags & REL_dropped))
	{
		free_relation(relation);
		return temp;
	}

	// Add relation to the list

	if (permanent)
	{
		dbb->dbb_relations.put(relation->rel_name, relation);
		MET_dsql_cache_use(tdbb, SYM_relation, relation->rel_name);
	}
	else
		relation->rel_flags |= REL_new_relation;

	return relation;
}


bool METD_get_type(jrd_tra* transaction, const MetaName& name, const char* field, SSHORT* value)
{
   struct {
          SSHORT jrd_31;	// gds__utility 
          SSHORT jrd_32;	// RDB$TYPE 
   } jrd_30;
   struct {
          TEXT  jrd_28 [32];	// RDB$TYPE_NAME 
          TEXT  jrd_29 [32];	// RDB$FIELD_NAME 
   } jrd_27;
/**************************************
 *
 *  M E T D _ g e t _ t y p e
 *
 **************************************
 *
 * Functional description
 *  Look up a symbolic name in RDB$TYPES
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	bool found = false;

	AutoCacheRequest handle(tdbb, irq_type, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		X IN RDB$TYPES WITH
		X.RDB$FIELD_NAME EQ field AND X.RDB$TYPE_NAME EQ name.c_str()*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_26, sizeof(jrd_26));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_27.jrd_28, 32);
	gds__vtov ((const char*) field, (char*) jrd_27.jrd_29, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_27);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_30);
	   if (!jrd_30.jrd_31) break;;
	{
		found = true;
		*value = /*X.RDB$TYPE*/
			 jrd_30.jrd_32;
	}
	/*END_FOR*/
	   }
	}

	return found;
}


dsql_rel* METD_get_view_base(jrd_tra* transaction, DsqlCompilerScratch* dsqlScratch,
	const char* view_name, MetaNamePairMap& fields)
{
   struct {
          TEXT  jrd_11 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_12 [32];	// RDB$BASE_FIELD 
          SSHORT jrd_13;	// gds__utility 
          SSHORT jrd_14;	// gds__null_flag 
          SSHORT jrd_15;	// gds__null_flag 
   } jrd_10;
   struct {
          TEXT  jrd_9 [32];	// RDB$VIEW_NAME 
   } jrd_8;
   struct {
          TEXT  jrd_20 [32];	// RDB$VIEW_NAME 
          TEXT  jrd_21 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_22 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_23;	// gds__utility 
          SSHORT jrd_24;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_25;	// RDB$VIEW_CONTEXT 
   } jrd_19;
   struct {
          TEXT  jrd_18 [32];	// RDB$VIEW_NAME 
   } jrd_17;
/**************************************
 *
 *  M E T D _ g e t _ v i e w _ b a s e
 *
 **************************************
 *
 * Functional description
 *  Return the base table of a view or NULL if there
 *  is more than one.
 *  If there is only one base, put in fields a map of
 *  top view field name / bottom base field name.
 *  Ignores the field in the case of a base field name
 *  appearing more than one time in a level.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_rel* relation = NULL;

	bool first = true;
	bool cont = true;
	MetaNamePairMap previousAux;

	fields.clear();

	while (cont)
	{
		AutoCacheRequest handle1(tdbb, irq_view_base, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE handle1 TRANSACTION_HANDLE transaction)
			X IN RDB$VIEW_RELATIONS
			WITH X.RDB$VIEW_NAME EQ view_name*/
		{
		handle1.compile(tdbb, (UCHAR*) jrd_16, sizeof(jrd_16));
		gds__vtov ((const char*) view_name, (char*) jrd_17.jrd_18, 32);
		EXE_start (tdbb, handle1, transaction);
		EXE_send (tdbb, handle1, 0, 32, (UCHAR*) &jrd_17);
		while (1)
		   {
		   EXE_receive (tdbb, handle1, 1, 326, (UCHAR*) &jrd_19);
		   if (!jrd_19.jrd_23) break;
		{
			// return NULL if there is more than one context
			if (/*X.RDB$VIEW_CONTEXT*/
			    jrd_19.jrd_25 != 1 || /*X.RDB$CONTEXT_TYPE*/
	 jrd_19.jrd_24 == VCT_PROCEDURE)
			{
				relation = NULL;
				cont = false;
				break;
			}

			fb_utils::exact_name(/*X.RDB$CONTEXT_NAME*/
					     jrd_19.jrd_22);
			fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
					     jrd_19.jrd_21);

			relation = METD_get_relation(transaction, dsqlScratch, /*X.RDB$RELATION_NAME*/
									       jrd_19.jrd_21);

			Array<MetaName> ambiguities;
			MetaNamePairMap currentAux;

			if (!relation)
			{
				cont = false;
				break;
			}

			AutoCacheRequest handle2(tdbb, irq_view_base_flds, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
				RFL IN RDB$RELATION_FIELDS
				WITH RFL.RDB$RELATION_NAME EQ X.RDB$VIEW_NAME*/
			{
			handle2.compile(tdbb, (UCHAR*) jrd_7, sizeof(jrd_7));
			gds__vtov ((const char*) jrd_19.jrd_20, (char*) jrd_8.jrd_9, 32);
			EXE_start (tdbb, handle2, transaction);
			EXE_send (tdbb, handle2, 0, 32, (UCHAR*) &jrd_8);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 70, (UCHAR*) &jrd_10);
			   if (!jrd_10.jrd_13) break;
			{
				if (/*RFL.RDB$BASE_FIELD.NULL*/
				    jrd_10.jrd_15 || /*RFL.RDB$FIELD_NAME.NULL*/
    jrd_10.jrd_14)
					continue;

				const MetaName baseField(/*RFL.RDB$BASE_FIELD*/
							 jrd_10.jrd_12);
				if (currentAux.exist(baseField))
					ambiguities.add(baseField);
				else
				{
					const MetaName fieldName(/*RFL.RDB$FIELD_NAME*/
								 jrd_10.jrd_11);
					if (first)
					{
						fields.put(fieldName, baseField);
						currentAux.put(baseField, fieldName);
					}
					else
					{
						MetaName field;

						if (previousAux.get(fieldName, field))
						{
							fields.put(field, baseField);
							currentAux.put(baseField, field);
						}
					}
				}
			}
			/*END_FOR*/
			   }
			}

			for (const MetaName* i = ambiguities.begin(); i != ambiguities.end(); ++i)
			{
				MetaName field;

				if (currentAux.get(*i, field))
				{
					currentAux.remove(*i);
					fields.remove(field);
				}
			}

			previousAux.takeOwnership(currentAux);

			if (relation->rel_flags & REL_view)
				view_name = /*X.RDB$RELATION_NAME*/
					    jrd_19.jrd_21;
			else
			{
				cont = false;
				break;
			}

			first = false;
		}
		/*END_FOR*/
		   }
		}
	}

	if (!relation)
		fields.clear();

	return relation;
}


dsql_rel* METD_get_view_relation(jrd_tra* transaction, DsqlCompilerScratch* dsqlScratch,
	const char* view_name, const char* relation_or_alias)
{
   struct {
          TEXT  jrd_4 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_5 [256];	// RDB$CONTEXT_NAME 
          SSHORT jrd_6;	// gds__utility 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$VIEW_NAME 
   } jrd_1;
/**************************************
 *
 *  M E T D _ g e t _ v i e w _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Return TRUE if the passed view_name represents a
 *  view with the passed relation as a base table
 *  (the relation could be an alias).
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	validateTransaction(transaction);

	dsql_rel* relation = NULL;

	AutoCacheRequest handle(tdbb, irq_view, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
		X IN RDB$VIEW_RELATIONS WITH X.RDB$VIEW_NAME EQ view_name*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	gds__vtov ((const char*) view_name, (char*) jrd_1.jrd_2, 32);
	EXE_start (tdbb, handle, transaction);
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 290, (UCHAR*) &jrd_3);
	   if (!jrd_3.jrd_6) break;
	{
		fb_utils::exact_name(/*X.RDB$CONTEXT_NAME*/
				     jrd_3.jrd_5);
		fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
				     jrd_3.jrd_4);

		if (!strcmp(/*X.RDB$RELATION_NAME*/
			    jrd_3.jrd_4, relation_or_alias) ||
			!strcmp(/*X.RDB$CONTEXT_NAME*/
				jrd_3.jrd_5, relation_or_alias))
		{
			relation = METD_get_relation(transaction, dsqlScratch, /*X.RDB$RELATION_NAME*/
									       jrd_3.jrd_4);
			return relation;
		}

		relation = METD_get_view_relation(transaction, dsqlScratch, /*X.RDB$RELATION_NAME*/
									    jrd_3.jrd_4,
			relation_or_alias);

		if (relation)
			return relation;
	}
	/*END_FOR*/
	   }
	}

	return NULL;
}


static void convert_dtype(TypeClause* field, SSHORT field_type)
{
/**************************************
 *
 *  c o n v e r t _ d t y p e
 *
 **************************************
 *
 * Functional description
 *  Convert from the blr_<type> stored in system metadata
 *  to the internal dtype_* descriptor.  Also set field
 *  length.
 *
 **************************************/

	// fill out the type descriptor
	switch (field_type)
	{
	case blr_text:
		field->dtype = dtype_text;
		break;
	case blr_varying:
		field->dtype = dtype_varying;
		field->length += sizeof(USHORT);
		break;
	case blr_blob:
		field->dtype = dtype_blob;
		field->length = type_lengths[field->dtype];
		break;
	default:
		field->dtype = gds_cvt_blr_dtype[field_type];
		field->length = type_lengths[field->dtype];

		fb_assert(field->dtype != dtype_unknown);
	}
}


#ifdef NOT_USED_OR_REPLACED
static void free_procedure(dsql_prc* procedure)
{
/**************************************
 *
 *  f r e e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *  Free memory allocated for a procedure block and params
 *
 **************************************/
	dsql_fld* param;

	// release the input & output parameter blocks

	for (param = procedure->prc_inputs; param;)
	{
		dsql_fld* temp = param;
		param = param->fld_next;
		delete temp;
	}

	for (param = procedure->prc_outputs; param;)
	{
		dsql_fld* temp = param;
		param = param->fld_next;
		delete temp;
	}

	// release the procedure & symbol blocks

	delete procedure;
}
#endif	// NOT_USED_OR_REPLACED


static void free_relation(dsql_rel* relation)
{
/**************************************
 *
 *  f r e e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *  Free memory allocated for a relation block and fields
 *
 **************************************/

	// release the field blocks

	for (dsql_fld* field = relation->rel_fields; field;)
	{
		dsql_fld* temp = field;
		field = field->fld_next;
		delete temp;
	}

	// release the relation & symbol blocks

	delete relation;
}
