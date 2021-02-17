/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../dsql/PackageNodes.h"
#include "../jrd/dyn.h"
#include "../jrd/intl.h"
#include "../jrd/jrd.h"
#include "../jrd/tra.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/vio_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/pass1_proto.h"
#include "../common/StatusArg.h"
#include "../jrd/Attachment.h"


using namespace Firebird;

namespace Jrd {

using namespace Firebird;

/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [290] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 10,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 14,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 16,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 15,0, 
                        blr_parameter2, 1, 0,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 1,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 2,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 3,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter2, 1, 11,0, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 17,0, 
                        blr_parameter2, 1, 13,0, 12,0, 
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
                                    blr_parameter2, 2, 3,0, 9,0, 
                                    blr_fid, 1, 15,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 8,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 7,0, 
                                    blr_fid, 1, 13,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 6,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 5,0, 4,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 5,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_31 [326] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 12,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 16,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter2, 1, 0,0, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 1,0, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter2, 1, 2,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 0, 15,0, 
                        blr_parameter2, 1, 3,0, 10,0, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 4,0, 11,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter2, 1, 13,0, 12,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter2, 1, 15,0, 14,0, 
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
                                    blr_parameter2, 2, 4,0, 11,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 10,0, 
                                    blr_fid, 1, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 2,0, 9,0, 
                                    blr_fid, 1, 4,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 8,0, 
                                    blr_fid, 1, 15,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 7,0, 
                                    blr_fid, 1, 13,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 6,0, 5,0, 
                                    blr_fid, 1, 1,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 6,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_66 [129] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
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
               blr_rid, 42,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
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
                                    blr_fid, 1, 2,0, 
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
static const UCHAR	jrd_78 [184] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 7,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
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
               blr_rid, 15,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 10,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 13,0, 
                        blr_parameter2, 1, 0,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 1,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 6,0, 
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
                                    blr_fid, 1, 13,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 1, 14,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_96 [195] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 8,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
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
               blr_rid, 27,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 14,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 0,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 1,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 7,0, 
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
                                    blr_fid, 1, 7,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 1, 8,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_115 [143] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 1,0, 3,0, 
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
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 1, 2,0, 
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
static const UCHAR	jrd_128 [137] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 18,0, 0, 
               blr_boolean, 
                  blr_or, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 0, 7,0, 
                           blr_parameter, 0, 3,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 0, 6,0, 
                           blr_parameter, 0, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_erase, 0, 
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
static const UCHAR	jrd_140 [116] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
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
               blr_rid, 42,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
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
                           blr_erase, 0, 
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
static const UCHAR	jrd_151 [173] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 4,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_short, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 1,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter2, 1, 2,0, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
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
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 2,0, 
                                    blr_fid, 1, 1,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_168 [83] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 8,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 42,0, 0, 
         blr_begin, 
            blr_assignment, 
               blr_parameter2, 0, 0,0, 3,0, 
               blr_fid, 0, 1,0, 
            blr_assignment, 
               blr_parameter2, 0, 1,0, 4,0, 
               blr_fid, 0, 4,0, 
            blr_assignment, 
               blr_parameter2, 0, 6,0, 5,0, 
               blr_fid, 0, 5,0, 
            blr_assignment, 
               blr_parameter2, 0, 2,0, 7,0, 
               blr_fid, 0, 0,0, 
            blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_178 [407] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 38,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
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
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 27,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 14,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 1,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 0,0, 
                           blr_fid, 0, 4,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter2, 1, 0,0, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter2, 1, 1,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 2,0, 33,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter2, 1, 3,0, 34,0, 
                  blr_assignment, 
                     blr_fid, 0, 4,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 1, 27,0, 
                     blr_parameter2, 1, 10,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter2, 1, 12,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter2, 1, 14,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 1, 24,0, 
                     blr_parameter2, 1, 16,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 1, 23,0, 
                     blr_parameter2, 1, 18,0, 17,0, 
                  blr_assignment, 
                     blr_fid, 1, 17,0, 
                     blr_parameter2, 1, 20,0, 19,0, 
                  blr_assignment, 
                     blr_fid, 1, 11,0, 
                     blr_parameter2, 1, 22,0, 21,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter2, 1, 24,0, 23,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter2, 1, 26,0, 25,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter2, 1, 28,0, 27,0, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
                     blr_parameter2, 1, 30,0, 29,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter2, 1, 32,0, 31,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 35,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 36,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 37,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 6,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_221 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 16,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter2, 1, 1,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_231 [396] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 37,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
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
            blr_rse, 2, 
               blr_rid, 15,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 1, 0,0, 
                           blr_fid, 0, 12,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 0,0, 7,0, 
                  blr_assignment, 
                     blr_fid, 0, 14,0, 
                     blr_parameter2, 1, 1,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 0, 19,0, 
                     blr_parameter2, 1, 2,0, 33,0, 
                  blr_assignment, 
                     blr_fid, 0, 18,0, 
                     blr_parameter2, 1, 3,0, 34,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 1, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 11,0, 
                     blr_parameter, 1, 5,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 6,0, 
                  blr_assignment, 
                     blr_fid, 1, 27,0, 
                     blr_parameter2, 1, 10,0, 9,0, 
                  blr_assignment, 
                     blr_fid, 1, 26,0, 
                     blr_parameter2, 1, 12,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 1, 25,0, 
                     blr_parameter2, 1, 14,0, 13,0, 
                  blr_assignment, 
                     blr_fid, 1, 24,0, 
                     blr_parameter2, 1, 16,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 1, 23,0, 
                     blr_parameter2, 1, 18,0, 17,0, 
                  blr_assignment, 
                     blr_fid, 1, 17,0, 
                     blr_parameter2, 1, 20,0, 19,0, 
                  blr_assignment, 
                     blr_fid, 1, 11,0, 
                     blr_parameter2, 1, 22,0, 21,0, 
                  blr_assignment, 
                     blr_fid, 1, 10,0, 
                     blr_parameter2, 1, 24,0, 23,0, 
                  blr_assignment, 
                     blr_fid, 1, 9,0, 
                     blr_parameter2, 1, 26,0, 25,0, 
                  blr_assignment, 
                     blr_fid, 1, 8,0, 
                     blr_parameter2, 1, 28,0, 27,0, 
                  blr_assignment, 
                     blr_fid, 0, 16,0, 
                     blr_parameter2, 1, 30,0, 29,0, 
                  blr_assignment, 
                     blr_fid, 0, 15,0, 
                     blr_parameter2, 1, 32,0, 31,0, 
                  blr_assignment, 
                     blr_fid, 0, 17,0, 
                     blr_parameter, 1, 35,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 36,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 6,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_273 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 6,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
               blr_rid, 14,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter2, 1, 0,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 0, 13,0, 
                     blr_parameter2, 1, 1,0, 5,0, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 



//----------------------


namespace
{
	struct ParameterInfo
	{
		explicit ParameterInfo(MemoryPool& p)
			: type(0),
			  number(0),
			  name(p),
			  fieldSource(p),
			  fieldName(p),
			  relationName(p),
			  mechanism(0)
		{
			defaultSource.clear();
			defaultValue.clear();
		}

		ParameterInfo(MemoryPool& p, const ParameterInfo& o)
			: type(o.type),
			  number(o.number),
			  name(p, o.name),
			  fieldSource(p, o.fieldSource),
			  fieldName(p, o.fieldName),
			  relationName(p, o.relationName),
			  collationId(o.collationId),
			  nullFlag(o.nullFlag),
			  mechanism(o.mechanism),
			  fieldLength(o.fieldLength),
			  fieldScale(o.fieldScale),
			  fieldType(o.fieldType),
			  fieldSubType(o.fieldSubType),
			  fieldSegmentLength(o.fieldSegmentLength),
			  fieldNullFlag(o.fieldNullFlag),
			  fieldCharLength(o.fieldCharLength),
			  fieldCollationId(o.fieldCollationId),
			  fieldCharSetId(o.fieldCharSetId),
			  fieldPrecision(o.fieldPrecision),
			  defaultSource(o.defaultSource),
			  defaultValue(o.defaultValue)
		{
		}

		SSHORT type;
		SSHORT number;
		MetaName name;
		MetaName fieldSource;
		MetaName fieldName;
		MetaName relationName;
		Nullable<SSHORT> collationId;
		Nullable<SSHORT> nullFlag;
		SSHORT mechanism;
		Nullable<SSHORT> fieldLength;
		Nullable<SSHORT> fieldScale;
		Nullable<SSHORT> fieldType;
		Nullable<SSHORT> fieldSubType;
		Nullable<SSHORT> fieldSegmentLength;
		Nullable<SSHORT> fieldNullFlag;
		Nullable<SSHORT> fieldCharLength;
		Nullable<SSHORT> fieldCollationId;
		Nullable<SSHORT> fieldCharSetId;
		Nullable<SSHORT> fieldPrecision;

		// Not compared
		bid defaultSource;
		bid defaultValue;

		bool operator >(const ParameterInfo& o) const
		{
			return type > o.type || (type == o.type && number > o.number);
		}

		bool operator ==(const ParameterInfo& o) const
		{
			return type == o.type && number == o.number && name == o.name &&
				(fieldSource == o.fieldSource ||
					(fb_utils::implicit_domain(fieldSource.c_str()) &&
						fb_utils::implicit_domain(o.fieldSource.c_str()))) &&
				fieldName == o.fieldName && relationName == o.relationName &&
				collationId == o.collationId && nullFlag == o.nullFlag &&
				mechanism == o.mechanism && fieldLength == o.fieldLength &&
				fieldScale == o.fieldScale && fieldType == o.fieldType &&
				fieldSubType == o.fieldSubType && fieldSegmentLength == o.fieldSegmentLength &&
				fieldNullFlag == o.fieldNullFlag && fieldCharLength == o.fieldCharLength &&
				fieldCollationId == o.fieldCollationId && fieldCharSetId == o.fieldCharSetId &&
				fieldPrecision == o.fieldPrecision;
		}

		bool operator !=(const ParameterInfo& o) const
		{
			return !(*this == o);
		}
	};

	struct Signature
	{
		Signature(MemoryPool& p, const MetaName& aName)
			: name(p, aName),
			  parameters(p),
			  defined(false)
		{
		}

		explicit Signature(const MetaName& aName)
			: name(aName),
			  parameters(*getDefaultMemoryPool()),
			  defined(false)
		{
		}

		explicit Signature(MemoryPool& p)
			: name(p),
			  parameters(p),
			  defined(false)
		{
		}

		Signature(MemoryPool& p, const Signature& o)
			: name(p, o.name),
			  parameters(p),
			  defined(o.defined)
		{
			for (SortedObjectsArray<ParameterInfo>::const_iterator i = o.parameters.begin();
				 i != o.parameters.end(); ++i)
			{
				parameters.add(*i);
			}
		}

		bool operator >(const Signature& o) const
		{
			return name > o.name;
		}

		bool operator ==(const Signature& o) const
		{
			if (name != o.name || parameters.getCount() != o.parameters.getCount())
				return false;

			for (SortedObjectsArray<ParameterInfo>::const_iterator i = parameters.begin(),
																   j = o.parameters.begin();
				i != parameters.end(); ++i, ++j)
			{
				if (*i != *j)
					return false;
			}

			return true;
		}

		bool operator !=(const Signature& o) const
		{
			return !(*this == o);
		}

		MetaName name;
		SortedObjectsArray<ParameterInfo> parameters;
		bool defined;
	};


	// Return function and procedure names (in the user charset) and optionally its details for a
	// given package.
	void collectPackagedItems(thread_db* tdbb, jrd_tra* transaction, const MetaName& metaName,
		SortedObjectsArray<Signature>& functions,
		SortedObjectsArray<Signature>& procedures, bool details)
	{
	   struct {
	          bid  jrd_183;	// RDB$DEFAULT_VALUE 
	          bid  jrd_184;	// RDB$DEFAULT_SOURCE 
	          TEXT  jrd_185 [32];	// RDB$RELATION_NAME 
	          TEXT  jrd_186 [32];	// RDB$FIELD_NAME 
	          TEXT  jrd_187 [32];	// RDB$FIELD_SOURCE 
	          TEXT  jrd_188 [32];	// RDB$PARAMETER_NAME 
	          SSHORT jrd_189;	// gds__utility 
	          SSHORT jrd_190;	// gds__null_flag 
	          SSHORT jrd_191;	// gds__null_flag 
	          SSHORT jrd_192;	// gds__null_flag 
	          SSHORT jrd_193;	// RDB$FIELD_PRECISION 
	          SSHORT jrd_194;	// gds__null_flag 
	          SSHORT jrd_195;	// RDB$CHARACTER_SET_ID 
	          SSHORT jrd_196;	// gds__null_flag 
	          SSHORT jrd_197;	// RDB$COLLATION_ID 
	          SSHORT jrd_198;	// gds__null_flag 
	          SSHORT jrd_199;	// RDB$CHARACTER_LENGTH 
	          SSHORT jrd_200;	// gds__null_flag 
	          SSHORT jrd_201;	// RDB$NULL_FLAG 
	          SSHORT jrd_202;	// gds__null_flag 
	          SSHORT jrd_203;	// RDB$SEGMENT_LENGTH 
	          SSHORT jrd_204;	// gds__null_flag 
	          SSHORT jrd_205;	// RDB$FIELD_SUB_TYPE 
	          SSHORT jrd_206;	// gds__null_flag 
	          SSHORT jrd_207;	// RDB$FIELD_TYPE 
	          SSHORT jrd_208;	// gds__null_flag 
	          SSHORT jrd_209;	// RDB$FIELD_SCALE 
	          SSHORT jrd_210;	// gds__null_flag 
	          SSHORT jrd_211;	// RDB$FIELD_LENGTH 
	          SSHORT jrd_212;	// gds__null_flag 
	          SSHORT jrd_213;	// RDB$NULL_FLAG 
	          SSHORT jrd_214;	// gds__null_flag 
	          SSHORT jrd_215;	// RDB$COLLATION_ID 
	          SSHORT jrd_216;	// gds__null_flag 
	          SSHORT jrd_217;	// gds__null_flag 
	          SSHORT jrd_218;	// RDB$PARAMETER_MECHANISM 
	          SSHORT jrd_219;	// RDB$PARAMETER_NUMBER 
	          SSHORT jrd_220;	// RDB$PARAMETER_TYPE 
	   } jrd_182;
	   struct {
	          TEXT  jrd_180 [32];	// RDB$PROCEDURE_NAME 
	          TEXT  jrd_181 [32];	// RDB$PACKAGE_NAME 
	   } jrd_179;
	   struct {
	          TEXT  jrd_225 [256];	// RDB$ENTRYPOINT 
	          bid  jrd_226;	// RDB$PROCEDURE_BLR 
	          TEXT  jrd_227 [32];	// RDB$PROCEDURE_NAME 
	          SSHORT jrd_228;	// gds__utility 
	          SSHORT jrd_229;	// gds__null_flag 
	          SSHORT jrd_230;	// gds__null_flag 
	   } jrd_224;
	   struct {
	          TEXT  jrd_223 [32];	// RDB$PACKAGE_NAME 
	   } jrd_222;
	   struct {
	          bid  jrd_236;	// RDB$DEFAULT_VALUE 
	          bid  jrd_237;	// RDB$DEFAULT_SOURCE 
	          TEXT  jrd_238 [32];	// RDB$RELATION_NAME 
	          TEXT  jrd_239 [32];	// RDB$FIELD_NAME 
	          TEXT  jrd_240 [32];	// RDB$FIELD_SOURCE 
	          TEXT  jrd_241 [32];	// RDB$ARGUMENT_NAME 
	          SSHORT jrd_242;	// gds__utility 
	          SSHORT jrd_243;	// gds__null_flag 
	          SSHORT jrd_244;	// gds__null_flag 
	          SSHORT jrd_245;	// gds__null_flag 
	          SSHORT jrd_246;	// RDB$FIELD_PRECISION 
	          SSHORT jrd_247;	// gds__null_flag 
	          SSHORT jrd_248;	// RDB$CHARACTER_SET_ID 
	          SSHORT jrd_249;	// gds__null_flag 
	          SSHORT jrd_250;	// RDB$COLLATION_ID 
	          SSHORT jrd_251;	// gds__null_flag 
	          SSHORT jrd_252;	// RDB$CHARACTER_LENGTH 
	          SSHORT jrd_253;	// gds__null_flag 
	          SSHORT jrd_254;	// RDB$NULL_FLAG 
	          SSHORT jrd_255;	// gds__null_flag 
	          SSHORT jrd_256;	// RDB$SEGMENT_LENGTH 
	          SSHORT jrd_257;	// gds__null_flag 
	          SSHORT jrd_258;	// RDB$FIELD_SUB_TYPE 
	          SSHORT jrd_259;	// gds__null_flag 
	          SSHORT jrd_260;	// RDB$FIELD_TYPE 
	          SSHORT jrd_261;	// gds__null_flag 
	          SSHORT jrd_262;	// RDB$FIELD_SCALE 
	          SSHORT jrd_263;	// gds__null_flag 
	          SSHORT jrd_264;	// RDB$FIELD_LENGTH 
	          SSHORT jrd_265;	// gds__null_flag 
	          SSHORT jrd_266;	// RDB$NULL_FLAG 
	          SSHORT jrd_267;	// gds__null_flag 
	          SSHORT jrd_268;	// RDB$COLLATION_ID 
	          SSHORT jrd_269;	// gds__null_flag 
	          SSHORT jrd_270;	// gds__null_flag 
	          SSHORT jrd_271;	// RDB$ARGUMENT_MECHANISM 
	          SSHORT jrd_272;	// RDB$ARGUMENT_POSITION 
	   } jrd_235;
	   struct {
	          TEXT  jrd_233 [32];	// RDB$FUNCTION_NAME 
	          TEXT  jrd_234 [32];	// RDB$PACKAGE_NAME 
	   } jrd_232;
	   struct {
	          TEXT  jrd_277 [256];	// RDB$ENTRYPOINT 
	          bid  jrd_278;	// RDB$FUNCTION_BLR 
	          TEXT  jrd_279 [32];	// RDB$FUNCTION_NAME 
	          SSHORT jrd_280;	// gds__utility 
	          SSHORT jrd_281;	// gds__null_flag 
	          SSHORT jrd_282;	// gds__null_flag 
	   } jrd_276;
	   struct {
	          TEXT  jrd_275 [32];	// RDB$PACKAGE_NAME 
	   } jrd_274;
		AutoCacheRequest requestHandle(tdbb, drq_l_pkg_funcs, DYN_REQUESTS);
		AutoCacheRequest requestHandle2(tdbb, drq_l_pkg_func_args, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			FUN IN RDB$FUNCTIONS
			WITH FUN.RDB$PACKAGE_NAME EQ metaName.c_str()*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_273, sizeof(jrd_273));
		gds__vtov ((const char*) metaName.c_str(), (char*) jrd_274.jrd_275, 32);
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_274);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 302, (UCHAR*) &jrd_276);
		   if (!jrd_276.jrd_280) break;
		{
			Signature function(/*FUN.RDB$FUNCTION_NAME*/
					   jrd_276.jrd_279);
			function.defined = !/*FUN.RDB$FUNCTION_BLR.NULL*/
					    jrd_276.jrd_282 || !/*FUN.RDB$ENTRYPOINT.NULL*/
     jrd_276.jrd_281;

			if (details)
			{
				/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
					ARG IN RDB$FUNCTION_ARGUMENTS CROSS
					FLD IN RDB$FIELDS
					WITH ARG.RDB$PACKAGE_NAME EQ metaName.c_str() AND
						 ARG.RDB$FUNCTION_NAME EQ FUN.RDB$FUNCTION_NAME AND
						 FLD.RDB$FIELD_NAME EQ ARG.RDB$FIELD_SOURCE*/
				{
				requestHandle2.compile(tdbb, (UCHAR*) jrd_231, sizeof(jrd_231));
				gds__vtov ((const char*) jrd_276.jrd_279, (char*) jrd_232.jrd_233, 32);
				gds__vtov ((const char*) metaName.c_str(), (char*) jrd_232.jrd_234, 32);
				EXE_start (tdbb, requestHandle2, transaction);
				EXE_send (tdbb, requestHandle2, 0, 64, (UCHAR*) &jrd_232);
				while (1)
				   {
				   EXE_receive (tdbb, requestHandle2, 1, 206, (UCHAR*) &jrd_235);
				   if (!jrd_235.jrd_242) break;
				{
					ParameterInfo parameter(*getDefaultMemoryPool());

					parameter.number = /*ARG.RDB$ARGUMENT_POSITION*/
							   jrd_235.jrd_272;
					parameter.name = /*ARG.RDB$ARGUMENT_NAME*/
							 jrd_235.jrd_241;
					parameter.fieldSource = /*ARG.RDB$FIELD_SOURCE*/
								jrd_235.jrd_240;
					parameter.mechanism = /*ARG.RDB$ARGUMENT_MECHANISM*/
							      jrd_235.jrd_271;

					if (!/*ARG.RDB$FIELD_NAME.NULL*/
					     jrd_235.jrd_270)
						parameter.fieldName = /*ARG.RDB$FIELD_NAME*/
								      jrd_235.jrd_239;
					if (!/*ARG.RDB$RELATION_NAME.NULL*/
					     jrd_235.jrd_269)
						parameter.relationName = /*ARG.RDB$RELATION_NAME*/
									 jrd_235.jrd_238;
					if (!/*ARG.RDB$COLLATION_ID.NULL*/
					     jrd_235.jrd_267)
						parameter.collationId = /*ARG.RDB$COLLATION_ID*/
									jrd_235.jrd_268;
					if (!/*ARG.RDB$NULL_FLAG.NULL*/
					     jrd_235.jrd_265)
						parameter.nullFlag = /*ARG.RDB$NULL_FLAG*/
								     jrd_235.jrd_266;

					if (!/*FLD.RDB$FIELD_LENGTH.NULL*/
					     jrd_235.jrd_263)
						parameter.fieldLength = /*FLD.RDB$FIELD_LENGTH*/
									jrd_235.jrd_264;
					if (!/*FLD.RDB$FIELD_SCALE.NULL*/
					     jrd_235.jrd_261)
						parameter.fieldScale = /*FLD.RDB$FIELD_SCALE*/
								       jrd_235.jrd_262;
					if (!/*FLD.RDB$FIELD_TYPE.NULL*/
					     jrd_235.jrd_259)
						parameter.fieldType = /*FLD.RDB$FIELD_TYPE*/
								      jrd_235.jrd_260;
					if (!/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					     jrd_235.jrd_257)
						parameter.fieldSubType = /*FLD.RDB$FIELD_SUB_TYPE*/
									 jrd_235.jrd_258;
					if (!/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					     jrd_235.jrd_255)
						parameter.fieldSegmentLength = /*FLD.RDB$SEGMENT_LENGTH*/
									       jrd_235.jrd_256;
					if (!/*FLD.RDB$NULL_FLAG.NULL*/
					     jrd_235.jrd_253)
						parameter.fieldNullFlag = /*FLD.RDB$NULL_FLAG*/
									  jrd_235.jrd_254;
					if (!/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					     jrd_235.jrd_251)
						parameter.fieldCharLength = /*FLD.RDB$CHARACTER_LENGTH*/
									    jrd_235.jrd_252;
					if (!/*FLD.RDB$COLLATION_ID.NULL*/
					     jrd_235.jrd_249)
						parameter.fieldCollationId = /*FLD.RDB$COLLATION_ID*/
									     jrd_235.jrd_250;
					if (!/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					     jrd_235.jrd_247)
						parameter.fieldCharSetId = /*FLD.RDB$CHARACTER_SET_ID*/
									   jrd_235.jrd_248;
					if (!/*FLD.RDB$FIELD_PRECISION.NULL*/
					     jrd_235.jrd_245)
						parameter.fieldPrecision = /*FLD.RDB$FIELD_PRECISION*/
									   jrd_235.jrd_246;

					if (!/*ARG.RDB$DEFAULT_SOURCE.NULL*/
					     jrd_235.jrd_244)
						parameter.defaultSource = /*ARG.RDB$DEFAULT_SOURCE*/
									  jrd_235.jrd_237;
					if (!/*ARG.RDB$DEFAULT_VALUE.NULL*/
					     jrd_235.jrd_243)
						parameter.defaultValue = /*ARG.RDB$DEFAULT_VALUE*/
									 jrd_235.jrd_236;

					function.parameters.add(parameter);
				}
				/*END_FOR*/
				   }
				}
			}

			functions.add(function);
		}
		/*END_FOR*/
		   }
		}

		requestHandle.reset(tdbb, drq_l_pkg_procs, DYN_REQUESTS);
		requestHandle2.reset(tdbb, drq_l_pkg_proc_args, DYN_REQUESTS);

		/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
			PRC IN RDB$PROCEDURES
			WITH PRC.RDB$PACKAGE_NAME EQ metaName.c_str()*/
		{
		requestHandle.compile(tdbb, (UCHAR*) jrd_221, sizeof(jrd_221));
		gds__vtov ((const char*) metaName.c_str(), (char*) jrd_222.jrd_223, 32);
		EXE_start (tdbb, requestHandle, transaction);
		EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_222);
		while (1)
		   {
		   EXE_receive (tdbb, requestHandle, 1, 302, (UCHAR*) &jrd_224);
		   if (!jrd_224.jrd_228) break;
		{
			Signature procedure(/*PRC.RDB$PROCEDURE_NAME*/
					    jrd_224.jrd_227);
			procedure.defined = !/*PRC.RDB$PROCEDURE_BLR.NULL*/
					     jrd_224.jrd_230 || !/*PRC.RDB$ENTRYPOINT.NULL*/
     jrd_224.jrd_229;

			if (details)
			{
				/*FOR (REQUEST_HANDLE requestHandle2 TRANSACTION_HANDLE transaction)
					PRM IN RDB$PROCEDURE_PARAMETERS CROSS
					FLD IN RDB$FIELDS
					WITH PRM.RDB$PACKAGE_NAME EQ metaName.c_str() AND
						 PRM.RDB$PROCEDURE_NAME EQ PRC.RDB$PROCEDURE_NAME AND
						 FLD.RDB$FIELD_NAME EQ PRM.RDB$FIELD_SOURCE*/
				{
				requestHandle2.compile(tdbb, (UCHAR*) jrd_178, sizeof(jrd_178));
				gds__vtov ((const char*) jrd_224.jrd_227, (char*) jrd_179.jrd_180, 32);
				gds__vtov ((const char*) metaName.c_str(), (char*) jrd_179.jrd_181, 32);
				EXE_start (tdbb, requestHandle2, transaction);
				EXE_send (tdbb, requestHandle2, 0, 64, (UCHAR*) &jrd_179);
				while (1)
				   {
				   EXE_receive (tdbb, requestHandle2, 1, 208, (UCHAR*) &jrd_182);
				   if (!jrd_182.jrd_189) break;
				{
					ParameterInfo parameter(*getDefaultMemoryPool());
					parameter.type = /*PRM.RDB$PARAMETER_TYPE*/
							 jrd_182.jrd_220;
					parameter.number = /*PRM.RDB$PARAMETER_NUMBER*/
							   jrd_182.jrd_219;
					parameter.name = /*PRM.RDB$PARAMETER_NAME*/
							 jrd_182.jrd_188;
					parameter.fieldSource = /*PRM.RDB$FIELD_SOURCE*/
								jrd_182.jrd_187;
					parameter.mechanism = /*PRM.RDB$PARAMETER_MECHANISM*/
							      jrd_182.jrd_218;

					if (!/*PRM.RDB$FIELD_NAME.NULL*/
					     jrd_182.jrd_217)
						parameter.fieldName = /*PRM.RDB$FIELD_NAME*/
								      jrd_182.jrd_186;
					if (!/*PRM.RDB$RELATION_NAME.NULL*/
					     jrd_182.jrd_216)
						parameter.relationName = /*PRM.RDB$RELATION_NAME*/
									 jrd_182.jrd_185;
					if (!/*PRM.RDB$COLLATION_ID.NULL*/
					     jrd_182.jrd_214)
						parameter.collationId = /*PRM.RDB$COLLATION_ID*/
									jrd_182.jrd_215;
					if (!/*PRM.RDB$NULL_FLAG.NULL*/
					     jrd_182.jrd_212)
						parameter.nullFlag = /*PRM.RDB$NULL_FLAG*/
								     jrd_182.jrd_213;

					if (!/*FLD.RDB$FIELD_LENGTH.NULL*/
					     jrd_182.jrd_210)
						parameter.fieldLength = /*FLD.RDB$FIELD_LENGTH*/
									jrd_182.jrd_211;
					if (!/*FLD.RDB$FIELD_SCALE.NULL*/
					     jrd_182.jrd_208)
						parameter.fieldScale = /*FLD.RDB$FIELD_SCALE*/
								       jrd_182.jrd_209;
					if (!/*FLD.RDB$FIELD_TYPE.NULL*/
					     jrd_182.jrd_206)
						parameter.fieldType = /*FLD.RDB$FIELD_TYPE*/
								      jrd_182.jrd_207;
					if (!/*FLD.RDB$FIELD_SUB_TYPE.NULL*/
					     jrd_182.jrd_204)
						parameter.fieldSubType = /*FLD.RDB$FIELD_SUB_TYPE*/
									 jrd_182.jrd_205;
					if (!/*FLD.RDB$SEGMENT_LENGTH.NULL*/
					     jrd_182.jrd_202)
						parameter.fieldSegmentLength = /*FLD.RDB$SEGMENT_LENGTH*/
									       jrd_182.jrd_203;
					if (!/*FLD.RDB$NULL_FLAG.NULL*/
					     jrd_182.jrd_200)
						parameter.fieldNullFlag = /*FLD.RDB$NULL_FLAG*/
									  jrd_182.jrd_201;
					if (!/*FLD.RDB$CHARACTER_LENGTH.NULL*/
					     jrd_182.jrd_198)
						parameter.fieldCharLength = /*FLD.RDB$CHARACTER_LENGTH*/
									    jrd_182.jrd_199;
					if (!/*FLD.RDB$COLLATION_ID.NULL*/
					     jrd_182.jrd_196)
						parameter.fieldCollationId = /*FLD.RDB$COLLATION_ID*/
									     jrd_182.jrd_197;
					if (!/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					     jrd_182.jrd_194)
						parameter.fieldCharSetId = /*FLD.RDB$CHARACTER_SET_ID*/
									   jrd_182.jrd_195;
					if (!/*FLD.RDB$FIELD_PRECISION.NULL*/
					     jrd_182.jrd_192)
						parameter.fieldPrecision = /*FLD.RDB$FIELD_PRECISION*/
									   jrd_182.jrd_193;

					if (!/*PRM.RDB$DEFAULT_SOURCE.NULL*/
					     jrd_182.jrd_191)
						parameter.defaultSource = /*PRM.RDB$DEFAULT_SOURCE*/
									  jrd_182.jrd_184;
					if (!/*PRM.RDB$DEFAULT_VALUE.NULL*/
					     jrd_182.jrd_190)
						parameter.defaultValue = /*PRM.RDB$DEFAULT_VALUE*/
									 jrd_182.jrd_183;

					procedure.parameters.add(parameter);
				}
				/*END_FOR*/
				   }
				}
			}

			procedures.add(procedure);
		}
		/*END_FOR*/
		   }
		}
	}
}	// namespace


//----------------------


void CreateAlterPackageNode::print(string& text) const
{
	fb_assert(items);

	text.printf(
		"CreateAlterPackageNode\n"
		"  name: '%s'  create: %d  alter: %d\n"
		"  Items:\n"
		"--------\n",
		name.c_str(), create, alter);

	for (unsigned i = 0; i < items->getCount(); ++i)
	{
		string item;

		switch ((*items)[i].type)
		{
			case Item::FUNCTION:
				(*items)[i].function->print(item);
				break;

			case Item::PROCEDURE:
				(*items)[i].procedure->print(item);
				break;
		}

		text += item;
	}

	text += "--------\n";
}


DdlNode* CreateAlterPackageNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	source.ltrim("\n\r\t ");

	// items
	for (unsigned i = 0; i < items->getCount(); ++i)
	{
		DsqlCompiledStatement* itemStatement = FB_NEW(getPool()) DsqlCompiledStatement(getPool());

		DsqlCompilerScratch* itemScratch = (*items)[i].dsqlScratch =
			FB_NEW(getPool()) DsqlCompilerScratch(getPool(), dsqlScratch->getAttachment(),
				dsqlScratch->getTransaction(), itemStatement);

		itemScratch->clientDialect = dsqlScratch->clientDialect;
		itemScratch->flags |= DsqlCompilerScratch::FLAG_DDL;
		itemScratch->package = name;

		switch ((*items)[i].type)
		{
			case CreateAlterPackageNode::Item::FUNCTION:
				{
					CreateAlterFunctionNode* const fun = (*items)[i].function;

					if (functionNames.exist(fun->name))
					{
						status_exception::raise(
							Arg::Gds(isc_no_meta_update) <<
							Arg::Gds(isc_dyn_duplicate_package_item) <<
								Arg::Str("FUNCTION") << Arg::Str(fun->name));
					}

					functionNames.add(fun->name);

					fun->alter = true;
					fun->package = name;
					fun->dsqlPass(itemScratch);
				}
				break;

			case CreateAlterPackageNode::Item::PROCEDURE:
				{
					CreateAlterProcedureNode* const proc = (*items)[i].procedure;

					if (procedureNames.exist(proc->name))
					{
						status_exception::raise(
							Arg::Gds(isc_no_meta_update) <<
							Arg::Gds(isc_dyn_duplicate_package_item) <<
								Arg::Str("PROCEDURE") << Arg::Str(proc->name));
					}

					procedureNames.add(proc->name);

					proc->alter = true;
					proc->package = name;
					proc->dsqlPass(itemScratch);
				}
				break;
		}
	}

	return DdlNode::dsqlPass(dsqlScratch);
}


void CreateAlterPackageNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
	fb_assert(create || alter);

	//Database* dbb = tdbb->getDatabase();

	//dbb->checkOdsForDsql(ODS_12_0);

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	if (alter)
	{
		if (!executeAlter(tdbb, dsqlScratch, transaction))
		{
			if (create)	// create or alter
				executeCreate(tdbb, dsqlScratch, transaction);
			else
			{
				status_exception::raise(
					Arg::Gds(isc_no_meta_update) <<
					Arg::Gds(isc_dyn_package_not_found) << Arg::Str(name));
			}
		}
	}
	else
		executeCreate(tdbb, dsqlScratch, transaction);

	// items
	for (unsigned i = 0; i < items->getCount(); ++i)
	{
		switch ((*items)[i].type)
		{
			case Item::FUNCTION:
				(*items)[i].function->packageOwner = owner;
				(*items)[i].function->executeDdl(tdbb, (*items)[i].dsqlScratch, transaction);
				break;

			case Item::PROCEDURE:
				(*items)[i].procedure->packageOwner = owner;
				(*items)[i].procedure->executeDdl(tdbb, (*items)[i].dsqlScratch, transaction);
				break;
		}
	}

	savePoint.release();	// everything is ok
}


void CreateAlterPackageNode::executeCreate(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          bid  jrd_170;	// RDB$PACKAGE_HEADER_SOURCE 
          TEXT  jrd_171 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_172 [32];	// RDB$PACKAGE_NAME 
          SSHORT jrd_173;	// gds__null_flag 
          SSHORT jrd_174;	// gds__null_flag 
          SSHORT jrd_175;	// gds__null_flag 
          SSHORT jrd_176;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_177;	// gds__null_flag 
   } jrd_169;
	Attachment* attachment = transaction->getAttachment();
	const string& userName = attachment->att_user->usr_user_name;

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
		DDL_TRIGGER_CREATE_PACKAGE, name);

	AutoCacheRequest requestHandle(tdbb, drq_s_pkg, DYN_REQUESTS);

	/*STORE (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PKG IN RDB$PACKAGES USING*/
	{
	
	{
		/*PKG.RDB$PACKAGE_NAME.NULL*/
		jrd_169.jrd_177 = FALSE;
		strcpy(/*PKG.RDB$PACKAGE_NAME*/
		       jrd_169.jrd_172, name.c_str());

		/*PKG.RDB$SYSTEM_FLAG.NULL*/
		jrd_169.jrd_175 = FALSE;
		/*PKG.RDB$SYSTEM_FLAG*/
		jrd_169.jrd_176 = 0;

		/*PKG.RDB$OWNER_NAME.NULL*/
		jrd_169.jrd_174 = FALSE;
		strcpy(/*PKG.RDB$OWNER_NAME*/
		       jrd_169.jrd_171, userName.c_str());

		/*PKG.RDB$PACKAGE_HEADER_SOURCE.NULL*/
		jrd_169.jrd_173 = FALSE;
		attachment->storeMetaDataBlob(tdbb, transaction, &/*PKG.RDB$PACKAGE_HEADER_SOURCE*/
								  jrd_169.jrd_170, source);
	}
	/*END_STORE*/
	requestHandle.compile(tdbb, (UCHAR*) jrd_168, sizeof(jrd_168));
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 82, (UCHAR*) &jrd_169);
	}

	storePrivileges(tdbb, transaction, name, obj_package_header, EXEC_PRIVILEGES);

	owner = userName;

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_CREATE_PACKAGE, name);
}


bool CreateAlterPackageNode::executeAlter(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_167;	// gds__utility 
   } jrd_166;
   struct {
          bid  jrd_162;	// RDB$PACKAGE_HEADER_SOURCE 
          bid  jrd_163;	// RDB$PACKAGE_BODY_SOURCE 
          SSHORT jrd_164;	// gds__null_flag 
          SSHORT jrd_165;	// gds__null_flag 
   } jrd_161;
   struct {
          TEXT  jrd_155 [32];	// RDB$OWNER_NAME 
          bid  jrd_156;	// RDB$PACKAGE_BODY_SOURCE 
          bid  jrd_157;	// RDB$PACKAGE_HEADER_SOURCE 
          SSHORT jrd_158;	// gds__utility 
          SSHORT jrd_159;	// gds__null_flag 
          SSHORT jrd_160;	// gds__null_flag 
   } jrd_154;
   struct {
          TEXT  jrd_153 [32];	// RDB$PACKAGE_NAME 
   } jrd_152;
	Attachment* attachment = transaction->getAttachment();
	AutoCacheRequest requestHandle(tdbb, drq_m_pkg, DYN_REQUESTS);
	bool modified = false;

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PKG IN RDB$PACKAGES
		WITH PKG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_151, sizeof(jrd_151));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_152.jrd_153, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_152);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 54, (UCHAR*) &jrd_154);
	   if (!jrd_154.jrd_158) break;
	{
		modified = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_ALTER_PACKAGE, name);

		SortedObjectsArray<Signature> existingFuncs(getPool());
		SortedObjectsArray<Signature> existingProcs(getPool());
		collectPackagedItems(tdbb, transaction, name, existingFuncs, existingProcs, false);

		for (SortedObjectsArray<Signature>::iterator i = existingFuncs.begin();
			 i != existingFuncs.end(); ++i)
		{
			if (!functionNames.exist(i->name))
			{
				DropFunctionNode dropNode(getPool(), i->name);
				dropNode.package = name;
				dropNode.dsqlPass(dsqlScratch);
				dropNode.executeDdl(tdbb, dsqlScratch, transaction);
			}
		}

		for (SortedObjectsArray<Signature>::iterator i = existingProcs.begin();
			 i != existingProcs.end(); ++i)
		{
			if (!procedureNames.exist(i->name))
			{
				DropProcedureNode dropNode(getPool(), i->name);
				dropNode.package = name;
				dropNode.dsqlPass(dsqlScratch);
				dropNode.executeDdl(tdbb, dsqlScratch, transaction);
			}
		}

		/*MODIFY PKG*/
		{
		
			/*PKG.RDB$PACKAGE_HEADER_SOURCE.NULL*/
			jrd_154.jrd_160 = FALSE;
			attachment->storeMetaDataBlob(tdbb, transaction, &/*PKG.RDB$PACKAGE_HEADER_SOURCE*/
									  jrd_154.jrd_157,
				source);

			/*PKG.RDB$PACKAGE_BODY_SOURCE.NULL*/
			jrd_154.jrd_159 = TRUE;
		/*END_MODIFY*/
		jrd_161.jrd_162 = jrd_154.jrd_157;
		jrd_161.jrd_163 = jrd_154.jrd_156;
		jrd_161.jrd_164 = jrd_154.jrd_160;
		jrd_161.jrd_165 = jrd_154.jrd_159;
		EXE_send (tdbb, requestHandle, 2, 20, (UCHAR*) &jrd_161);
		}

		owner = /*PKG.RDB$OWNER_NAME*/
			jrd_154.jrd_155;
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_166);
	   }
	}

	if (modified)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_ALTER_PACKAGE, name);

	return modified;
}


//----------------------


void DropPackageNode::print(string& text) const
{
	text.printf(
		"DropPackageNode\n"
		"  name: '%s'\n",
		name.c_str());
}


void DropPackageNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_139;	// gds__utility 
   } jrd_138;
   struct {
          SSHORT jrd_137;	// gds__utility 
   } jrd_136;
   struct {
          SSHORT jrd_135;	// gds__utility 
   } jrd_134;
   struct {
          TEXT  jrd_130 [32];	// RDB$USER 
          TEXT  jrd_131 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_132;	// RDB$USER_TYPE 
          SSHORT jrd_133;	// RDB$OBJECT_TYPE 
   } jrd_129;
   struct {
          SSHORT jrd_150;	// gds__utility 
   } jrd_149;
   struct {
          SSHORT jrd_148;	// gds__utility 
   } jrd_147;
   struct {
          TEXT  jrd_144 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_145;	// gds__utility 
          SSHORT jrd_146;	// gds__null_flag 
   } jrd_143;
   struct {
          TEXT  jrd_142 [32];	// RDB$PACKAGE_NAME 
   } jrd_141;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	bool found = false;
	AutoCacheRequest requestHandle(tdbb, drq_e_pkg, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PKG IN RDB$PACKAGES
		WITH PKG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_140, sizeof(jrd_140));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_141.jrd_142, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_141);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 36, (UCHAR*) &jrd_143);
	   if (!jrd_143.jrd_145) break;
	{
		found = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_PACKAGE, name);

		/*ERASE PKG;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_147);

		if (!/*PKG.RDB$SECURITY_CLASS.NULL*/
		     jrd_143.jrd_146)
			deleteSecurityClass(tdbb, transaction, /*PKG.RDB$SECURITY_CLASS*/
							       jrd_143.jrd_144);

		dsc desc;
		desc.makeText(name.length(), ttype_metadata,
			(UCHAR*) const_cast<char*>(name.c_str()));	// safe const_cast
		DFW_post_work(transaction, dfw_drop_package_header, &desc, 0);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_149);
	   }
	}

	if (!found && !silent)
	{
		status_exception::raise(
			Arg::Gds(isc_no_meta_update) <<
			Arg::Gds(isc_dyn_package_not_found) << Arg::Str(name));
	}

	SortedObjectsArray<Signature> existingFuncs(getPool());
	SortedObjectsArray<Signature> existingProcs(getPool());
	collectPackagedItems(tdbb, transaction, name, existingFuncs, existingProcs, false);

	for (SortedObjectsArray<Signature>::iterator i = existingFuncs.begin();
		 i != existingFuncs.end(); ++i)
	{
		DropFunctionNode dropNode(getPool(), i->name);
		dropNode.package = name;
		dropNode.dsqlPass(dsqlScratch);
		dropNode.executeDdl(tdbb, dsqlScratch, transaction);
	}

	for (SortedObjectsArray<Signature>::iterator i = existingProcs.begin();
		 i != existingProcs.end(); ++i)
	{
		DropProcedureNode dropNode(getPool(), i->name);
		dropNode.package = name;
		dropNode.dsqlPass(dsqlScratch);
		dropNode.executeDdl(tdbb, dsqlScratch, transaction);
	}

	requestHandle.reset(tdbb, drq_e_pkg_prv, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRIV IN RDB$USER_PRIVILEGES
		WITH (PRIV.RDB$RELATION_NAME EQ name.c_str() AND
				PRIV.RDB$OBJECT_TYPE = obj_package_header) OR
			 (PRIV.RDB$USER EQ name.c_str() AND PRIV.RDB$USER_TYPE = obj_package_header)*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_128, sizeof(jrd_128));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_129.jrd_130, 32);
	gds__vtov ((const char*) name.c_str(), (char*) jrd_129.jrd_131, 32);
	jrd_129.jrd_132 = obj_package_header;
	jrd_129.jrd_133 = obj_package_header;
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 68, (UCHAR*) &jrd_129);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 2, (UCHAR*) &jrd_134);
	   if (!jrd_134.jrd_135) break;
	{
		/*ERASE PRIV;*/
		EXE_send (tdbb, requestHandle, 2, 2, (UCHAR*) &jrd_136);
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_138);
	   }
	}

	if (found)
		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER, DDL_TRIGGER_DROP_PACKAGE, name);

	savePoint.release();	// everything is ok
}


//----------------------


void CreatePackageBodyNode::print(string& text) const
{
	fb_assert(items);

	text.printf(
		"CreatePackageBodyNode\n"
		"  name: '%s'\n"
		"  declaredItems:\n"
		"--------\n",
		name.c_str());

	if (declaredItems)
	{
		for (unsigned i = 0; i < declaredItems->getCount(); ++i)
		{
			string item;

			switch ((*declaredItems)[i].type)
			{
				case CreateAlterPackageNode::Item::FUNCTION:
					(*declaredItems)[i].function->print(item);
					break;

				case CreateAlterPackageNode::Item::PROCEDURE:
					(*declaredItems)[i].procedure->print(item);
					break;
			}

			text += item;
		}
	}

	text +=
		"--------\n"
		"  items:\n"
		"--------\n";

	for (unsigned i = 0; i < items->getCount(); ++i)
	{
		string item;

		switch ((*items)[i].type)
		{
			case CreateAlterPackageNode::Item::FUNCTION:
				(*items)[i].function->print(item);
				break;

			case CreateAlterPackageNode::Item::PROCEDURE:
				(*items)[i].procedure->print(item);
				break;
		}

		text += item;
	}

	text += "--------\n";
}


DdlNode* CreatePackageBodyNode::dsqlPass(DsqlCompilerScratch* dsqlScratch)
{
	source.ltrim("\n\r\t ");

	// process declaredItems and items
	Array<CreateAlterPackageNode::Item>* arrays[] = {declaredItems, items};
	SortedArray<MetaName> functionNames[FB_NELEM(arrays)];
	SortedArray<MetaName> procedureNames[FB_NELEM(arrays)];

	for (unsigned i = 0; i < FB_NELEM(arrays); ++i)
	{
		if (!arrays[i])
			continue;

		for (unsigned j = 0; j < arrays[i]->getCount(); ++j)
		{
			DsqlCompiledStatement* itemStatement = FB_NEW(getPool()) DsqlCompiledStatement(getPool());

			DsqlCompilerScratch* itemScratch = (*arrays[i])[j].dsqlScratch =
				FB_NEW(getPool()) DsqlCompilerScratch(getPool(), dsqlScratch->getAttachment(),
					dsqlScratch->getTransaction(), itemStatement);

			itemScratch->clientDialect = dsqlScratch->clientDialect;
			itemScratch->flags |= DsqlCompilerScratch::FLAG_DDL;
			itemScratch->package = name;

			switch ((*arrays[i])[j].type)
			{
				case CreateAlterPackageNode::Item::FUNCTION:
					{
						CreateAlterFunctionNode* const fun = (*arrays[i])[j].function;

						if (functionNames[i].exist(fun->name))
						{
							status_exception::raise(
								Arg::Gds(isc_no_meta_update) <<
								Arg::Gds(isc_dyn_duplicate_package_item) <<
									Arg::Str("FUNCTION") << Arg::Str(fun->name));
						}

						functionNames[i].add(fun->name);

						fun->package = name;
						fun->create = true;
						if (arrays[i] == items)
							fun->alter = true;
						fun->dsqlPass(itemScratch);
					}
					break;

				case CreateAlterPackageNode::Item::PROCEDURE:
					{
						CreateAlterProcedureNode* const proc = (*arrays[i])[j].procedure;

						if (procedureNames[i].exist(proc->name))
						{
							status_exception::raise(
								Arg::Gds(isc_no_meta_update) <<
								Arg::Gds(isc_dyn_duplicate_package_item) <<
									Arg::Str("PROCEDURE") << Arg::Str(proc->name));
						}

						procedureNames[i].add(proc->name);

						proc->package = name;
						proc->create = true;
						if (arrays[i] == items)
							proc->alter = true;
						proc->dsqlPass(itemScratch);
					}
					break;
			}
		}
	}

	return DdlNode::dsqlPass(dsqlScratch);
}


void CreatePackageBodyNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_95;	// gds__utility 
   } jrd_94;
   struct {
          bid  jrd_90;	// RDB$DEFAULT_SOURCE 
          bid  jrd_91;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_92;	// gds__null_flag 
          SSHORT jrd_93;	// gds__null_flag 
   } jrd_89;
   struct {
          bid  jrd_82;	// RDB$DEFAULT_VALUE 
          bid  jrd_83;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_84 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_85;	// gds__utility 
          SSHORT jrd_86;	// gds__null_flag 
          SSHORT jrd_87;	// gds__null_flag 
          SSHORT jrd_88;	// RDB$ARGUMENT_POSITION 
   } jrd_81;
   struct {
          TEXT  jrd_80 [32];	// RDB$PACKAGE_NAME 
   } jrd_79;
   struct {
          SSHORT jrd_114;	// gds__utility 
   } jrd_113;
   struct {
          bid  jrd_109;	// RDB$DEFAULT_SOURCE 
          bid  jrd_110;	// RDB$DEFAULT_VALUE 
          SSHORT jrd_111;	// gds__null_flag 
          SSHORT jrd_112;	// gds__null_flag 
   } jrd_108;
   struct {
          bid  jrd_100;	// RDB$DEFAULT_VALUE 
          bid  jrd_101;	// RDB$DEFAULT_SOURCE 
          TEXT  jrd_102 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_103;	// gds__utility 
          SSHORT jrd_104;	// gds__null_flag 
          SSHORT jrd_105;	// gds__null_flag 
          SSHORT jrd_106;	// RDB$PARAMETER_NUMBER 
          SSHORT jrd_107;	// RDB$PARAMETER_TYPE 
   } jrd_99;
   struct {
          TEXT  jrd_98 [32];	// RDB$PACKAGE_NAME 
   } jrd_97;
   struct {
          SSHORT jrd_127;	// gds__utility 
   } jrd_126;
   struct {
          bid  jrd_124;	// RDB$PACKAGE_BODY_SOURCE 
          SSHORT jrd_125;	// gds__null_flag 
   } jrd_123;
   struct {
          TEXT  jrd_119 [32];	// RDB$OWNER_NAME 
          bid  jrd_120;	// RDB$PACKAGE_BODY_SOURCE 
          SSHORT jrd_121;	// gds__utility 
          SSHORT jrd_122;	// gds__null_flag 
   } jrd_118;
   struct {
          TEXT  jrd_117 [32];	// RDB$PACKAGE_NAME 
   } jrd_116;
	Attachment* attachment = transaction->getAttachment();

	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	AutoCacheRequest requestHandle(tdbb, drq_m_pkg_body, DYN_REQUESTS);
	bool modified = false;

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PKG IN RDB$PACKAGES
		WITH PKG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_115, sizeof(jrd_115));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_116.jrd_117, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_116);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 44, (UCHAR*) &jrd_118);
	   if (!jrd_118.jrd_121) break;
	{
		if (!/*PKG.RDB$PACKAGE_BODY_SOURCE.NULL*/
		     jrd_118.jrd_122)
		{
			status_exception::raise(
				Arg::Gds(isc_no_meta_update) <<
				Arg::Gds(isc_dyn_package_body_exists) << Arg::Str(name));
		}

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_CREATE_PACKAGE_BODY, name);

		/*MODIFY PKG*/
		{
		
			/*PKG.RDB$PACKAGE_BODY_SOURCE.NULL*/
			jrd_118.jrd_122 = FALSE;
			attachment->storeMetaDataBlob(tdbb, transaction, &/*PKG.RDB$PACKAGE_BODY_SOURCE*/
									  jrd_118.jrd_120, source);
		/*END_MODIFY*/
		jrd_123.jrd_124 = jrd_118.jrd_120;
		jrd_123.jrd_125 = jrd_118.jrd_122;
		EXE_send (tdbb, requestHandle, 2, 10, (UCHAR*) &jrd_123);
		}

		modified = true;

		owner = /*PKG.RDB$OWNER_NAME*/
			jrd_118.jrd_119;
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_126);
	   }
	}

	if (!modified)
	{
		status_exception::raise(
			Arg::Gds(isc_no_meta_update) <<
			Arg::Gds(isc_dyn_package_not_found) << Arg::Str(name));
	}

	SortedObjectsArray<Signature> headerFuncs(getPool());
	SortedObjectsArray<Signature> headerProcs(getPool());
	collectPackagedItems(tdbb, transaction, name, headerFuncs, headerProcs, false);

	SortedObjectsArray<Signature> existingFuncs(getPool());
	SortedObjectsArray<Signature> existingProcs(getPool());

	// process declaredItems and items
	Array<CreateAlterPackageNode::Item>* arrays[] = {declaredItems, items};

	for (unsigned i = 0; i < FB_NELEM(arrays); ++i)
	{
		if (!arrays[i])
			continue;

		if (arrays[i] == items)
			collectPackagedItems(tdbb, transaction, name, existingFuncs, existingProcs, true);

		for (unsigned j = 0; j < arrays[i]->getCount(); ++j)
		{
			CreateAlterPackageNode::Item& elem = (*arrays[i])[j];
			switch (elem.type)
			{
				case CreateAlterPackageNode::Item::FUNCTION:
					{
						CreateAlterFunctionNode* func = elem.function;

						if (arrays[i] == items)
							func->privateScope = !headerFuncs.exist(Signature(func->name));

						func->packageOwner = owner;
						func->executeDdl(tdbb, elem.dsqlScratch, transaction);
					}
					break;

				case CreateAlterPackageNode::Item::PROCEDURE:
					{
						CreateAlterProcedureNode* proc = elem.procedure;

						if (arrays[i] == items)
							proc->privateScope = !headerProcs.exist(Signature(proc->name));

						proc->packageOwner = owner;
						proc->executeDdl(tdbb, elem.dsqlScratch, transaction);
					}
					break;
			}
		}
	}

	SortedObjectsArray<Signature> newFuncs(getPool());
	SortedObjectsArray<Signature> newProcs(getPool());
	collectPackagedItems(tdbb, transaction, name, newFuncs, newProcs, true);

	for (SortedObjectsArray<Signature>::iterator i = existingFuncs.begin();
		 i != existingFuncs.end(); ++i)
	{
		size_t pos;
		bool found = newFuncs.find(Signature(getPool(), i->name), pos);

		if (!found || !newFuncs[pos].defined)
		{
			status_exception::raise(
				Arg::Gds(isc_dyn_funcnotdef_package) << i->name.c_str() << name.c_str());
		}
		else if (newFuncs[pos] != *i)
		{
			status_exception::raise(
				Arg::Gds(isc_dyn_funcsignat_package) << i->name.c_str() << name.c_str());
		}
	}

	for (SortedObjectsArray<Signature>::iterator i = existingProcs.begin();
		 i != existingProcs.end(); ++i)
	{
		size_t pos;
		bool found = newProcs.find(Signature(getPool(), i->name), pos);

		if (!found || !newProcs[pos].defined)
		{
			status_exception::raise(
				Arg::Gds(isc_dyn_procnotdef_package) << i->name.c_str() << name.c_str());
		}
		else if (newProcs[pos] != *i)
		{
			status_exception::raise(
				Arg::Gds(isc_dyn_procsignat_package) << i->name.c_str() << name.c_str());
		}

		for (SortedObjectsArray<ParameterInfo>::iterator j = newProcs[pos].parameters.begin();
			 j != newProcs[pos].parameters.end(); ++j)
		{
			if (!j->defaultSource.isEmpty() || !j->defaultValue.isEmpty())
			{
				status_exception::raise(
					Arg::Gds(isc_dyn_defvaldecl_package) << name.c_str() << i->name.c_str());
			}
		}
	}

	// Lets recreate default of public procedure/function parameters

	requestHandle.reset(tdbb, drq_m_pkg_prm_defs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRM IN RDB$PROCEDURE_PARAMETERS
		WITH PRM.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_96, sizeof(jrd_96));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_97.jrd_98, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_97);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 58, (UCHAR*) &jrd_99);
	   if (!jrd_99.jrd_103) break;
	{
		size_t pos;
		if (existingProcs.find(Signature(getPool(), MetaName(/*PRM.RDB$PROCEDURE_NAME*/
								     jrd_99.jrd_102)), pos))
		{
			const Signature& proc = existingProcs[pos];

			ParameterInfo paramKey(getPool());
			paramKey.type = /*PRM.RDB$PARAMETER_TYPE*/
					jrd_99.jrd_107;
			paramKey.number = /*PRM.RDB$PARAMETER_NUMBER*/
					  jrd_99.jrd_106;

			if (proc.parameters.find(paramKey, pos))
			{
				const ParameterInfo& param = proc.parameters[pos];

				/*MODIFY PRM*/
				{
				
					/*PRM.RDB$DEFAULT_SOURCE*/
					jrd_99.jrd_101 = param.defaultSource;
					/*PRM.RDB$DEFAULT_SOURCE.NULL*/
					jrd_99.jrd_105 = param.defaultSource.isEmpty();

					/*PRM.RDB$DEFAULT_VALUE*/
					jrd_99.jrd_100 = param.defaultValue;
					/*PRM.RDB$DEFAULT_VALUE.NULL*/
					jrd_99.jrd_104 = param.defaultValue.isEmpty();
				/*END_MODIFY*/
				jrd_108.jrd_109 = jrd_99.jrd_101;
				jrd_108.jrd_110 = jrd_99.jrd_100;
				jrd_108.jrd_111 = jrd_99.jrd_105;
				jrd_108.jrd_112 = jrd_99.jrd_104;
				EXE_send (tdbb, requestHandle, 2, 20, (UCHAR*) &jrd_108);
				}
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_113);
	   }
	}

	requestHandle.reset(tdbb, drq_m_pkg_arg_defs, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		ARG IN RDB$FUNCTION_ARGUMENTS
		WITH ARG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_78, sizeof(jrd_78));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_79.jrd_80, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_79);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 56, (UCHAR*) &jrd_81);
	   if (!jrd_81.jrd_85) break;
	{
		size_t pos;
		if (existingFuncs.find(Signature(getPool(), MetaName(/*ARG.RDB$FUNCTION_NAME*/
								     jrd_81.jrd_84)), pos))
		{
			const Signature& func = existingFuncs[pos];

			ParameterInfo paramKey(getPool());
			paramKey.number = /*ARG.RDB$ARGUMENT_POSITION*/
					  jrd_81.jrd_88;

			if (func.parameters.find(paramKey, pos))
			{
				const ParameterInfo& param = func.parameters[pos];

				/*MODIFY ARG*/
				{
				
					/*ARG.RDB$DEFAULT_SOURCE*/
					jrd_81.jrd_83 = param.defaultSource;
					/*ARG.RDB$DEFAULT_SOURCE.NULL*/
					jrd_81.jrd_87 = param.defaultSource.isEmpty();

					/*ARG.RDB$DEFAULT_VALUE*/
					jrd_81.jrd_82 = param.defaultValue;
					/*ARG.RDB$DEFAULT_VALUE.NULL*/
					jrd_81.jrd_86 = param.defaultValue.isEmpty();
				/*END_MODIFY*/
				jrd_89.jrd_90 = jrd_81.jrd_83;
				jrd_89.jrd_91 = jrd_81.jrd_82;
				jrd_89.jrd_92 = jrd_81.jrd_87;
				jrd_89.jrd_93 = jrd_81.jrd_86;
				EXE_send (tdbb, requestHandle, 2, 20, (UCHAR*) &jrd_89);
				}
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_94);
	   }
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_CREATE_PACKAGE_BODY, name);

	savePoint.release();	// everything is ok
}


//----------------------


void DropPackageBodyNode::print(string& text) const
{
	text.printf(
		"DropPackageBodyNode\n"
		"  name: '%s'\n",
		name.c_str());
}


void DropPackageBodyNode::execute(thread_db* tdbb, DsqlCompilerScratch* dsqlScratch,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_30;	// gds__utility 
   } jrd_29;
   struct {
          bid  jrd_19;	// RDB$PROCEDURE_BLR 
          bid  jrd_20;	// RDB$DEBUG_INFO 
          TEXT  jrd_21 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_22 [256];	// RDB$ENTRYPOINT 
          SSHORT jrd_23;	// gds__null_flag 
          SSHORT jrd_24;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_25;	// gds__null_flag 
          SSHORT jrd_26;	// gds__null_flag 
          SSHORT jrd_27;	// gds__null_flag 
          SSHORT jrd_28;	// gds__null_flag 
   } jrd_18;
   struct {
          TEXT  jrd_4 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_5 [32];	// RDB$ENGINE_NAME 
          bid  jrd_6;	// RDB$DEBUG_INFO 
          bid  jrd_7;	// RDB$PROCEDURE_BLR 
          TEXT  jrd_8 [32];	// RDB$PROCEDURE_NAME 
          SSHORT jrd_9;	// gds__utility 
          SSHORT jrd_10;	// gds__null_flag 
          SSHORT jrd_11;	// gds__null_flag 
          SSHORT jrd_12;	// gds__null_flag 
          SSHORT jrd_13;	// gds__null_flag 
          SSHORT jrd_14;	// gds__null_flag 
          SSHORT jrd_15;	// RDB$PROCEDURE_TYPE 
          SSHORT jrd_16;	// gds__null_flag 
          SSHORT jrd_17;	// RDB$PRIVATE_FLAG 
   } jrd_3;
   struct {
          TEXT  jrd_2 [32];	// RDB$PACKAGE_NAME 
   } jrd_1;
   struct {
          SSHORT jrd_65;	// gds__utility 
   } jrd_64;
   struct {
          bid  jrd_52;	// RDB$FUNCTION_BLR 
          bid  jrd_53;	// RDB$DEBUG_INFO 
          TEXT  jrd_54 [256];	// RDB$MODULE_NAME 
          TEXT  jrd_55 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_56 [256];	// RDB$ENTRYPOINT 
          SSHORT jrd_57;	// gds__null_flag 
          SSHORT jrd_58;	// RDB$FUNCTION_TYPE 
          SSHORT jrd_59;	// gds__null_flag 
          SSHORT jrd_60;	// gds__null_flag 
          SSHORT jrd_61;	// gds__null_flag 
          SSHORT jrd_62;	// gds__null_flag 
          SSHORT jrd_63;	// gds__null_flag 
   } jrd_51;
   struct {
          TEXT  jrd_35 [256];	// RDB$ENTRYPOINT 
          TEXT  jrd_36 [32];	// RDB$ENGINE_NAME 
          TEXT  jrd_37 [256];	// RDB$MODULE_NAME 
          bid  jrd_38;	// RDB$DEBUG_INFO 
          bid  jrd_39;	// RDB$FUNCTION_BLR 
          TEXT  jrd_40 [32];	// RDB$FUNCTION_NAME 
          SSHORT jrd_41;	// gds__utility 
          SSHORT jrd_42;	// gds__null_flag 
          SSHORT jrd_43;	// gds__null_flag 
          SSHORT jrd_44;	// gds__null_flag 
          SSHORT jrd_45;	// gds__null_flag 
          SSHORT jrd_46;	// gds__null_flag 
          SSHORT jrd_47;	// gds__null_flag 
          SSHORT jrd_48;	// RDB$FUNCTION_TYPE 
          SSHORT jrd_49;	// gds__null_flag 
          SSHORT jrd_50;	// RDB$PRIVATE_FLAG 
   } jrd_34;
   struct {
          TEXT  jrd_33 [32];	// RDB$PACKAGE_NAME 
   } jrd_32;
   struct {
          SSHORT jrd_77;	// gds__utility 
   } jrd_76;
   struct {
          bid  jrd_74;	// RDB$PACKAGE_BODY_SOURCE 
          SSHORT jrd_75;	// gds__null_flag 
   } jrd_73;
   struct {
          bid  jrd_70;	// RDB$PACKAGE_BODY_SOURCE 
          SSHORT jrd_71;	// gds__utility 
          SSHORT jrd_72;	// gds__null_flag 
   } jrd_69;
   struct {
          TEXT  jrd_68 [32];	// RDB$PACKAGE_NAME 
   } jrd_67;
	// run all statements under savepoint control
	AutoSavePoint savePoint(tdbb, transaction);

	bool found = false;
	AutoCacheRequest requestHandle(tdbb, drq_m_pkg_body2, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PKG IN RDB$PACKAGES
		WITH PKG.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_66, sizeof(jrd_66));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_67.jrd_68, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_67);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 12, (UCHAR*) &jrd_69);
	   if (!jrd_69.jrd_71) break;
	{
		found = true;

		executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_BEFORE,
			DDL_TRIGGER_DROP_PACKAGE_BODY, name);

		/*MODIFY PKG*/
		{
		
			/*PKG.RDB$PACKAGE_BODY_SOURCE.NULL*/
			jrd_69.jrd_72 = TRUE;

			dsc desc;
			desc.makeText(name.length(), ttype_metadata,
				(UCHAR*) const_cast<char*>(name.c_str()));	// safe const_cast
			DFW_post_work(transaction, dfw_drop_package_body, &desc, 0);
		/*END_MODIFY*/
		jrd_73.jrd_74 = jrd_69.jrd_70;
		jrd_73.jrd_75 = jrd_69.jrd_72;
		EXE_send (tdbb, requestHandle, 2, 10, (UCHAR*) &jrd_73);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_76);
	   }
	}

	if (!found)
	{
		status_exception::raise(
			Arg::Gds(isc_no_meta_update) <<
			Arg::Gds(isc_dyn_package_not_found) << Arg::Str(name));
	}

	requestHandle.reset(tdbb, drq_m_pkg_fun, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		FUN IN RDB$FUNCTIONS
		WITH FUN.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_31, sizeof(jrd_31));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_32.jrd_33, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_32);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 612, (UCHAR*) &jrd_34);
	   if (!jrd_34.jrd_41) break;
	{
		if (!/*FUN.RDB$PRIVATE_FLAG.NULL*/
		     jrd_34.jrd_49 && /*FUN.RDB$PRIVATE_FLAG*/
    jrd_34.jrd_50 != 0)
		{
			DropFunctionNode dropNode(getPool(), /*FUN.RDB$FUNCTION_NAME*/
							     jrd_34.jrd_40);
			dropNode.package = name;
			dropNode.dsqlPass(dsqlScratch);
			dropNode.executeDdl(tdbb, dsqlScratch, transaction);
		}
		else
		{
			/*MODIFY FUN*/
			{
			
				/*FUN.RDB$FUNCTION_TYPE.NULL*/
				jrd_34.jrd_47 = TRUE;
				/*FUN.RDB$FUNCTION_BLR.NULL*/
				jrd_34.jrd_46 = TRUE;
				/*FUN.RDB$DEBUG_INFO.NULL*/
				jrd_34.jrd_45 = TRUE;
				/*FUN.RDB$MODULE_NAME.NULL*/
				jrd_34.jrd_44 = TRUE;
				/*FUN.RDB$ENGINE_NAME.NULL*/
				jrd_34.jrd_43 = TRUE;
				/*FUN.RDB$ENTRYPOINT.NULL*/
				jrd_34.jrd_42 = TRUE;
			/*END_MODIFY*/
			jrd_51.jrd_52 = jrd_34.jrd_39;
			jrd_51.jrd_53 = jrd_34.jrd_38;
			gds__vtov((const char*) jrd_34.jrd_37, (char*) jrd_51.jrd_54, 256);
			gds__vtov((const char*) jrd_34.jrd_36, (char*) jrd_51.jrd_55, 32);
			gds__vtov((const char*) jrd_34.jrd_35, (char*) jrd_51.jrd_56, 256);
			jrd_51.jrd_57 = jrd_34.jrd_47;
			jrd_51.jrd_58 = jrd_34.jrd_48;
			jrd_51.jrd_59 = jrd_34.jrd_46;
			jrd_51.jrd_60 = jrd_34.jrd_45;
			jrd_51.jrd_61 = jrd_34.jrd_44;
			jrd_51.jrd_62 = jrd_34.jrd_43;
			jrd_51.jrd_63 = jrd_34.jrd_42;
			EXE_send (tdbb, requestHandle, 2, 574, (UCHAR*) &jrd_51);
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_64);
	   }
	}

	requestHandle.reset(tdbb, drq_m_pkg_prc, DYN_REQUESTS);

	/*FOR (REQUEST_HANDLE requestHandle TRANSACTION_HANDLE transaction)
		PRC IN RDB$PROCEDURES
		WITH PRC.RDB$PACKAGE_NAME EQ name.c_str()*/
	{
	requestHandle.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_1.jrd_2, 32);
	EXE_start (tdbb, requestHandle, transaction);
	EXE_send (tdbb, requestHandle, 0, 32, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, requestHandle, 1, 354, (UCHAR*) &jrd_3);
	   if (!jrd_3.jrd_9) break;
	{
		if (!/*PRC.RDB$PRIVATE_FLAG.NULL*/
		     jrd_3.jrd_16 && /*PRC.RDB$PRIVATE_FLAG*/
    jrd_3.jrd_17 != 0)
		{
			DropProcedureNode dropNode(getPool(), /*PRC.RDB$PROCEDURE_NAME*/
							      jrd_3.jrd_8);
			dropNode.package = name;
			dropNode.dsqlPass(dsqlScratch);
			dropNode.executeDdl(tdbb, dsqlScratch, transaction);
		}
		else
		{
			/*MODIFY PRC*/
			{
			
				/*PRC.RDB$PROCEDURE_TYPE.NULL*/
				jrd_3.jrd_14 = TRUE;
				/*PRC.RDB$PROCEDURE_BLR.NULL*/
				jrd_3.jrd_13 = TRUE;
				/*PRC.RDB$DEBUG_INFO.NULL*/
				jrd_3.jrd_12 = TRUE;
				/*PRC.RDB$ENGINE_NAME.NULL*/
				jrd_3.jrd_11 = TRUE;
				/*PRC.RDB$ENTRYPOINT.NULL*/
				jrd_3.jrd_10 = TRUE;
			/*END_MODIFY*/
			jrd_18.jrd_19 = jrd_3.jrd_7;
			jrd_18.jrd_20 = jrd_3.jrd_6;
			gds__vtov((const char*) jrd_3.jrd_5, (char*) jrd_18.jrd_21, 32);
			gds__vtov((const char*) jrd_3.jrd_4, (char*) jrd_18.jrd_22, 256);
			jrd_18.jrd_23 = jrd_3.jrd_14;
			jrd_18.jrd_24 = jrd_3.jrd_15;
			jrd_18.jrd_25 = jrd_3.jrd_13;
			jrd_18.jrd_26 = jrd_3.jrd_12;
			jrd_18.jrd_27 = jrd_3.jrd_11;
			jrd_18.jrd_28 = jrd_3.jrd_10;
			EXE_send (tdbb, requestHandle, 2, 316, (UCHAR*) &jrd_18);
			}
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, requestHandle, 3, 2, (UCHAR*) &jrd_29);
	   }
	}

	executeDdlTrigger(tdbb, dsqlScratch, transaction, DTW_AFTER,
		DDL_TRIGGER_DROP_PACKAGE_BODY, name);

	savePoint.release();	// everything is ok
}


}	// namespace Jrd
