/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dfw.epp
 *	DESCRIPTION:	Deferred Work handler
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
 * 2001.6.25 Claudio Valderrama: Implement deferred check for udf usage
 * inside a procedure before dropping the udf and creating stub for future
 * processing of dependencies from dropped generators.
 *
 * 2001.8.12 Claudio Valderrama: find_depend_in_dfw() and other functions
 *   should respect identifiers with embedded blanks instead of chopping them
 *.
 * 2001.10.01 Claudio Valderrama: check constraints should fire AFTER the
 *   BEFORE <action> triggers; otherwise they allow invalid data to be stored.
 *   This is a quick fix for SF Bug #444463 until a more robust one is devised
 *   using trigger's rdb$flags or another mechanism.
 *
 * 2001.10.10 Ann Harrison:  Don't increment the format version unless the
 *   table is actually reformatted.  At the same time, break out some of
 *   the parts of make_version making some new subroutines with the goal
 *   of making make_version readable.
 *
 * 2001.10.18 Ann Harrison: some cleanup of trigger & constraint handling.
 *   it now appears to work correctly on new Firebird databases with lots
 *   of system types and on InterBase databases, without checking for
 *   missing source.
 *
 * 23-Feb-2002 Dmitry Yemanov - Events wildcarding
 *
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 *
 * Adriano dos Santos Fernandes
 *
 * 2008-03-16 Alex Peshkoff - avoid most of data modifications in system transaction.
 *	Problems took place when same data was modified in user transaction, and later -
 *	in system transaction. System transaction always performs updates in place,
 *	but when between RPB setup and actual modification garbage was collected (this
 *	was noticed with GC thread active, but may happen due to any read of the record),
 *	BUGCHECK(291) took place. To avoid that issue, it was decided not to modify data
 *	in system transaction. An exception is RDB$FORMATS relation, which is always modified
 *	by transaction zero. Also an aspect of 'dirty' access from system transaction was
 *	taken into an account in add_file(), make_version() and create_index().
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>

#include "../common/classes/fb_string.h"
#include "../common/classes/VaryStr.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/scl.h"
#include "../jrd/blb.h"
#include "../jrd/met.h"
#include "../jrd/lck.h"
#include "../jrd/sdw.h"
#include "../jrd/flags.h"
#include "../jrd/intl.h"
#include "../intl/charsets.h"
#include "../jrd/align.h"
#include "../common/gdsassert.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../common/dsc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/grant_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/intl_proto.h"
#include "../common/isc_f_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/pcmet_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/scl_proto.h"
#include "../jrd/sdw_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/event_proto.h"
#include "../jrd/nbak.h"
#include "../jrd/trig.h"
#include "../jrd/GarbageCollector.h"
#include "../jrd/IntlManager.h"
#include "../jrd/UserManagement.h"
#include "../jrd/Function.h"
#include "../jrd/PreparedStatement.h"
#include "../jrd/ResultSet.h"
#include "../common/utils_proto.h"
#include "../common/classes/Hash.h"
#include "../jrd/CryptoManager.h"
#include "../jrd/Mapping.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gen/iberror.h"

// Pick up system relation ids
#include "../jrd/ini.h"

// Define range of user relation ids

const int MIN_RELATION_ID	= rel_MAX;
const int MAX_RELATION_ID	= 32767;

const int COMPUTED_FLAG	= 128;
const int WAIT_PERIOD	= -1;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [197] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 5,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_or, 
                        blr_between, 
                           blr_fid, 0, 8,0, 
                           blr_parameter, 0, 4,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 8,0, 
                              blr_literal, blr_long, 0, 0,0,0,0,
                           blr_any, 
                              blr_rse, 2, 
                                 blr_rid, 24,0, 1, 
                                 blr_rid, 22,0, 2, 
                                 blr_boolean, 
                                    blr_and, 
                                       blr_eql, 
                                          blr_fid, 0, 0,0, 
                                          blr_fid, 1, 1,0, 
                                       blr_and, 
                                          blr_eql, 
                                             blr_fid, 1, 0,0, 
                                             blr_fid, 2, 0,0, 
                                          blr_or, 
                                             blr_eql, 
                                                blr_fid, 2, 1,0, 
                                                blr_parameter, 0, 2,0, 
                                             blr_eql, 
                                                blr_fid, 2, 1,0, 
                                                blr_parameter, 0, 1,0, 
                                 blr_end, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_11 [180] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 8,0, 
                           blr_literal, blr_long, 0, 0,0,0,0,
                        blr_not, 
                           blr_any, 
                              blr_rse, 2, 
                                 blr_rid, 24,0, 1, 
                                 blr_rid, 22,0, 2, 
                                 blr_boolean, 
                                    blr_and, 
                                       blr_eql, 
                                          blr_fid, 0, 0,0, 
                                          blr_fid, 1, 1,0, 
                                       blr_and, 
                                          blr_eql, 
                                             blr_fid, 1, 0,0, 
                                             blr_fid, 2, 0,0, 
                                          blr_or, 
                                             blr_eql, 
                                                blr_fid, 2, 1,0, 
                                                blr_parameter, 0, 2,0, 
                                             blr_eql, 
                                                blr_fid, 2, 1,0, 
                                                blr_parameter, 0, 1,0, 
                                 blr_end, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_20 [116] =
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
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 8,0, 
                        blr_literal, blr_long, 0, 1,0,0,0,
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_27 [71] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_begin, 
      blr_for, 
         blr_rse, 1, 
            blr_rid, 10,0, 0, 
            blr_first, 
               blr_literal, blr_long, 0, 1,0,0,0,
            blr_boolean, 
               blr_gtr, 
                  blr_fid, 0, 5,0, 
                  blr_literal, blr_long, 0, 0,0,0,0,
            blr_end, 
         blr_send, 0, 
            blr_begin, 
               blr_assignment, 
                  blr_literal, blr_long, 0, 1,0,0,0,
                  blr_parameter, 0, 0,0, 
               blr_end, 
      blr_send, 0, 
         blr_assignment, 
            blr_literal, blr_long, 0, 0,0,0,0,
            blr_parameter, 0, 0,0, 
      blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_30 [136] =
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
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 5,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
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
                                    blr_fid, 1, 10,0, 
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
static const UCHAR	jrd_42 [108] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 8,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_literal, blr_long, 0, 0,0,0,0,
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
static const UCHAR	jrd_51 [96] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
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
                     blr_fid, 1, 5,0, 
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
static const UCHAR	jrd_57 [84] =
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
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 3,0, 
                        blr_literal, blr_long, 0, 1,0,0,0,
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
static const UCHAR	jrd_62 [525] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 8,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 38,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
      blr_quad, 0, 
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
               blr_rid, 5,0, 0, 
               blr_rid, 2,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_fid, 1, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 19,0, 
                        blr_parameter2, 1, 1,0, 25,0, 
                     blr_assignment, 
                        blr_fid, 0, 14,0, 
                        blr_parameter2, 1, 2,0, 26,0, 
                     blr_assignment, 
                        blr_fid, 1, 2,0, 
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 1, 6,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 1, 12,0, 
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 8,0, 
                     blr_assignment, 
                        blr_fid, 1, 4,0, 
                        blr_parameter, 1, 9,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 10,0, 
                     blr_assignment, 
                        blr_fid, 1, 19,0, 
                        blr_parameter, 1, 11,0, 
                     blr_assignment, 
                        blr_fid, 1, 20,0, 
                        blr_parameter, 1, 12,0, 
                     blr_assignment, 
                        blr_fid, 1, 21,0, 
                        blr_parameter, 1, 13,0, 
                     blr_assignment, 
                        blr_fid, 1, 22,0, 
                        blr_parameter, 1, 14,0, 
                     blr_assignment, 
                        blr_fid, 1, 11,0, 
                        blr_parameter, 1, 15,0, 
                     blr_assignment, 
                        blr_fid, 1, 8,0, 
                        blr_parameter, 1, 16,0, 
                     blr_assignment, 
                        blr_fid, 1, 9,0, 
                        blr_parameter, 1, 17,0, 
                     blr_assignment, 
                        blr_fid, 1, 10,0, 
                        blr_parameter, 1, 18,0, 
                     blr_assignment, 
                        blr_fid, 0, 18,0, 
                        blr_parameter2, 1, 20,0, 19,0, 
                     blr_assignment, 
                        blr_fid, 1, 25,0, 
                        blr_parameter2, 1, 22,0, 21,0, 
                     blr_assignment, 
                        blr_fid, 1, 26,0, 
                        blr_parameter2, 1, 24,0, 23,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 1, 27,0, 
                     blr_assignment, 
                        blr_fid, 1, 23,0, 
                        blr_parameter2, 1, 29,0, 28,0, 
                     blr_assignment, 
                        blr_fid, 0, 16,0, 
                        blr_parameter2, 1, 31,0, 30,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 33,0, 32,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 35,0, 34,0, 
                     blr_assignment, 
                        blr_fid, 0, 9,0, 
                        blr_parameter2, 1, 37,0, 36,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 7,0, 6,0, 
                                    blr_fid, 2, 16,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 5,0, 4,0, 
                                    blr_fid, 2, 6,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 2,0, 
                                    blr_fid, 2, 8,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 0,0, 
                                    blr_fid, 2, 9,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 10,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_115 [259] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 6,0, 
      blr_quad, 0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 10,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 11,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter2, 1, 8,0, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 9,0, 
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
                                    blr_parameter, 2, 5,0, 
                                    blr_fid, 1, 5,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 1, 8,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 4,0, 
                                    blr_fid, 1, 7,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 11,0, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 3,0, 2,0, 
                                    blr_fid, 1, 6,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_138 [50] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 0, 3,0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_store, 
         blr_rid, 8,0, 0, 
         blr_begin, 
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
static const UCHAR	jrd_143 [107] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_int64, 0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 12,0, 0, 
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
                     blr_fid, 0, 5,0, 
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
static const UCHAR	jrd_151 [104] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 4,0, 
      blr_long, 0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 21,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 3,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 2,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_159 [86] =
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
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_166 [124] =
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
            blr_rse, 3, 
               blr_rid, 2,0, 0, 
               blr_rid, 5,0, 1, 
               blr_rid, 6,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_fid, 1, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_eql, 
                           blr_fid, 2, 8,0, 
                           blr_fid, 1, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 2, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_173 [82] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
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
                     blr_eql, 
                        blr_fid, 0, 3,0, 
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
static const UCHAR	jrd_178 [68] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 6,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 3,0, 
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
static const UCHAR	jrd_183 [149] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 3, 
               blr_rid, 6,0, 0, 
               blr_rid, 7,0, 1, 
               blr_rid, 5,0, 2, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 1,0, 
                        blr_fid, 0, 8,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 3,0, 
                           blr_parameter, 0, 1,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 2, 10,0, 
                              blr_fid, 1, 2,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 2, 1,0, 
                                 blr_fid, 1, 0,0, 
                              blr_eql, 
                                 blr_fid, 2, 4,0, 
                                 blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 4,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_191 [95] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_short, 0, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 8,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
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
static const UCHAR	jrd_200 [85] =
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
            blr_rse, 1, 
               blr_rid, 7,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 1,0, 
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
static const UCHAR	jrd_206 [78] =
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
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 4,0, 
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
static const UCHAR	jrd_211 [86] =
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
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_218 [126] =
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
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 2,0, 
                           blr_parameter, 0, 0,0, 
                        blr_missing, 
                           blr_fid, 1, 0,0, 
               blr_project, 2, 
                  blr_fid, 0, 1,0, 
                  blr_fid, 0, 9,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 1,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 9,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_225 [110] =
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
            blr_rse, 2, 
               blr_rid, 5,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
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
                  blr_assignment, 
                     blr_fid, 1, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_232 [86] =
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
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_239 [212] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 1, 8,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter2, 1, 2,0, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 8,0, 
                        blr_parameter2, 1, 3,0, 6,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 7,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 0,0, 1,0, 
                                    blr_fid, 2, 8,0, 
                                 blr_end, 
                        blr_end, 
               blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 4,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_257 [82] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
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
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
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
static const UCHAR	jrd_263 [120] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_or, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 2,0, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 1, 5,0, 
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
static const UCHAR	jrd_271 [205] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 4, 1,0, 
      blr_short, 0, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 6,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_quad, 0, 
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
               blr_rid, 1,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 1, 8,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 10,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 1, 5,0, 
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 1, 3,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 5,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 4, 
                           blr_modify, 0, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 4, 0,0, 
                                    blr_fid, 2, 1,0, 
                                 blr_end, 
                        blr_receive, 2, 
                           blr_modify, 1, 3, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 3, 5,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 3, 3,0, 
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
static const UCHAR	jrd_288 [86] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 1,0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 0,0, 12,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 22,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
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
static const UCHAR	jrd_294 [129] =
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
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
                                    blr_fid, 1, 2,0, 
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
static const UCHAR	jrd_306 [338] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 22,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_double, 
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
            blr_rse, 5, 
               blr_rid, 4,0, 0, 
               blr_rid, 3,0, 1, 
               blr_rid, 5,0, 2, 
               blr_rid, 2,0, 3, 
               blr_rid, 6,0, 4, 
               blr_boolean, 
                  blr_and, 
                     blr_and, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 1, 0,0, 
                              blr_fid, 0, 0,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 2, 0,0, 
                                 blr_fid, 1, 1,0, 
                              blr_eql, 
                                 blr_fid, 2, 1,0, 
                                 blr_fid, 0, 1,0, 
                        blr_eql, 
                           blr_fid, 4, 8,0, 
                           blr_fid, 2, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 3, 0,0, 
                           blr_fid, 2, 2,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 2, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 8,0, 
                     blr_parameter2, 1, 1,0, 15,0, 
                  blr_assignment, 
                     blr_fid, 0, 12,0, 
                     blr_parameter, 1, 2,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 3,0, 
                  blr_assignment, 
                     blr_fid, 3, 25,0, 
                     blr_parameter2, 1, 5,0, 4,0, 
                  blr_assignment, 
                     blr_fid, 2, 18,0, 
                     blr_parameter2, 1, 7,0, 6,0, 
                  blr_assignment, 
                     blr_fid, 3, 26,0, 
                     blr_parameter2, 1, 9,0, 8,0, 
                  blr_assignment, 
                     blr_fid, 2, 9,0, 
                     blr_parameter, 1, 10,0, 
                  blr_assignment, 
                     blr_fid, 3, 22,0, 
                     blr_parameter2, 1, 12,0, 11,0, 
                  blr_assignment, 
                     blr_fid, 3, 10,0, 
                     blr_parameter, 1, 13,0, 
                  blr_assignment, 
                     blr_fid, 1, 2,0, 
                     blr_parameter, 1, 14,0, 
                  blr_assignment, 
                     blr_fid, 0, 7,0, 
                     blr_parameter, 1, 16,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 17,0, 
                  blr_assignment, 
                     blr_fid, 0, 5,0, 
                     blr_parameter, 1, 18,0, 
                  blr_assignment, 
                     blr_fid, 0, 6,0, 
                     blr_parameter, 1, 19,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
                     blr_parameter, 1, 20,0, 
                  blr_assignment, 
                     blr_fid, 4, 3,0, 
                     blr_parameter, 1, 21,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_332 [172] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 6,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_quad, 0, 
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
               blr_rid, 4,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter2, 1, 1,0, 5,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 4,0, 3,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter2, 2, 1,0, 0,0, 
                                    blr_fid, 2, 2,0, 
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
static const UCHAR	jrd_347 [71] =
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
               blr_rid, 1,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_352 [142] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 2,0, 
                        blr_equiv, 
                           blr_fid, 0, 5,0, 
                           blr_value_if, 
                              blr_eql, 
                                 blr_parameter, 0, 0,0, 
                                 blr_literal, blr_text2, 3,0, 0,0, 
                              blr_null, 
                              blr_parameter, 0, 0,0, 
               blr_project, 1, 
                  blr_fid, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_361 [157] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 4,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 13,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 2,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 4,0, 
                           blr_parameter, 0, 3,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 1,0, 
                           blr_equiv, 
                              blr_fid, 0, 5,0, 
                              blr_value_if, 
                                 blr_eql, 
                                    blr_parameter, 0, 0,0, 
                                    blr_literal, blr_text2, 3,0, 0,0, 
                                 blr_null, 
                                 blr_parameter, 0, 0,0, 
               blr_project, 1, 
                  blr_fid, 0, 0,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_literal, blr_long, 0, 1,0,0,0,
                     blr_parameter, 1, 1,0, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 2,0, 
                  blr_end, 
         blr_send, 1, 
            blr_assignment, 
               blr_literal, blr_long, 0, 0,0,0,0,
               blr_parameter, 1, 1,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 
static const UCHAR	jrd_371 [133] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_message, 0, 3,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 13,0, 0, 
               blr_rid, 5,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 3,0, 
                           blr_parameter, 0, 2,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 4,0, 
                              blr_parameter, 0, 1,0, 
                           blr_and, 
                              blr_eql, 
                                 blr_fid, 1, 1,0, 
                                 blr_fid, 0, 1,0, 
                              blr_eql, 
                                 blr_fid, 1, 0,0, 
                                 blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_379 [141] =
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
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 7,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 0, 1,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
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
                                    blr_fid, 1, 4,0, 
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
static const UCHAR	jrd_392 [118] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_quad, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 7,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
                        blr_and, 
                           blr_eql, 
                              blr_fid, 0, 2,0, 
                              blr_parameter, 0, 1,0, 
                           blr_missing, 
                              blr_fid, 0, 5,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_400 [211] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 3,0, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 6,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 2, 
               blr_rid, 10,0, 0, 
               blr_rid, 10,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 5,0, 
                        blr_fid, 0, 5,0, 
                     blr_eql, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 1, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 1, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 1, 2,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 1, 1,0, 
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 1, 4,0, 
                        blr_parameter, 1, 4,0, 
                     blr_assignment, 
                        blr_fid, 1, 5,0, 
                        blr_parameter, 1, 5,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 1, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 2,0, 
                                    blr_fid, 2, 1,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 2, 2,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 2, 4,0, 
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
static const UCHAR	jrd_416 [149] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_message, 1, 4,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 4,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
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
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_428 [152] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_long, 0, 
      blr_long, 0, 
   blr_message, 1, 3,0, 
      blr_long, 0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 1,0, 
                     blr_eql, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 1,0, 
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
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 3,0, 
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
static const UCHAR	jrd_441 [112] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 3,0, 
      blr_long, 0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_first, 
                  blr_literal, blr_long, 0, 1,0,0,0,
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 1,0, 
               blr_sort, 1, 
                  blr_descending, 
                     blr_fid, 0, 1,0, 
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 3,0, 
                     blr_parameter, 1, 0,0, 
                  blr_assignment, 
                     blr_fid, 0, 2,0, 
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
static const UCHAR	jrd_448 [193] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 4, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_long, 0, 
      blr_short, 0, 
   blr_message, 1, 5,0, 
      blr_cstring2, 0,0, 0,1, 
      blr_long, 0, 
      blr_short, 0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 0,0, 0,1, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 10,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 0,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 1, 4,0, 
                     blr_end, 
               blr_label, 0, 
                  blr_loop, 
                     blr_select, 
                        blr_receive, 3, 
                           blr_leave, 0, 
                        blr_receive, 2, 
                           blr_modify, 0, 2, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 2, 1,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 2, 2,0, 
                                 blr_end, 
                        blr_receive, 4, 
                           blr_modify, 0, 1, 
                              blr_begin, 
                                 blr_assignment, 
                                    blr_parameter, 4, 0,0, 
                                    blr_fid, 1, 0,0, 
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
static const UCHAR	jrd_464 [141] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_double, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_double, 
      blr_short, 0, 
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
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
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
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 12,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 1,0, 
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
static const UCHAR	jrd_476 [148] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_double, 
      blr_short, 0, 
   blr_message, 1, 3,0, 
      blr_double, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 0, 1,0, 
      blr_cstring2, 3,0, 32,0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 3,0, 0, 
               blr_boolean, 
                  blr_eql, 
                     blr_fid, 0, 0,0, 
                     blr_parameter, 0, 0,0, 
               blr_sort, 1, 
                  blr_ascending, 
                     blr_fid, 0, 2,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
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
                                    blr_parameter, 2, 1,0, 
                                    blr_fid, 1, 2,0, 
                                 blr_assignment, 
                                    blr_parameter, 2, 0,0, 
                                    blr_fid, 1, 3,0, 
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
static const UCHAR	jrd_488 [133] =
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
               blr_rid, 26,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_not, 
                        blr_missing, 
                           blr_fid, 0, 6,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 0,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
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
                                    blr_fid, 1, 12,0, 
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
static const UCHAR	jrd_500 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_quad, 0, 
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
                     blr_fid, 0, 6,0, 
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
static const UCHAR	jrd_507 [133] =
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
static const UCHAR	jrd_519 [110] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_quad, 0, 
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
                     blr_fid, 0, 13,0, 
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


using namespace Jrd;
using namespace Firebird;

namespace Jrd {

typedef Hash<
	DeferredWork,
	DEFAULT_HASH_SIZE,
	DeferredWork,
	DefaultKeyValue<DeferredWork>,
	DeferredWork
> DfwHash;

// NS: This needs careful refactoring.
//
// Deferred work item:
// * Encapsulates deferred invocation of the task routine with a given set of
//   arguments.
// * Has code to maintain a doubly linked list of itself.
//
// These two functions need to be split, and linked list of custom entries can
// become generic.
//

class DeferredWork : public pool_alloc<type_dfw>,
					 public DfwHash::Entry
{
private:
	DeferredWork(const DeferredWork&);

public:
	enum dfw_t 		dfw_type;		// type of work deferred

private:
	DeferredWork***	dfw_end;
	DeferredWork**	dfw_prev;
	DeferredWork*	dfw_next;

public:
	Lock*			dfw_lock;		// relation creation lock
	Array<DeferredWork*>  dfw_args;	// arguments
	SLONG			dfw_sav_number;	// save point number
	USHORT			dfw_id;			// object id, if appropriate
	USHORT			dfw_count;		// count of block posts
	string			dfw_name;		// name of object
	MetaName		dfw_package;	// package name
	SortedArray<int> dfw_ids;		// list of identifiers (or any numbers) needed by an action

public:
	DeferredWork(MemoryPool& p, DeferredWork*** end,
				 enum dfw_t t, USHORT id, SLONG sn, const string& name,
				 const MetaName& package)
	  : dfw_type(t), dfw_end(end), dfw_prev(dfw_end ? *dfw_end : NULL),
		dfw_next(dfw_prev ? *dfw_prev : NULL), dfw_lock(NULL), dfw_args(p),
		dfw_sav_number(sn), dfw_id(id), dfw_count(1), dfw_name(p, name),
		dfw_package(p, package), dfw_ids(p)
	{
		// make previous element point to us
		if (dfw_prev)
		{
			*dfw_prev = this;
			// make next element (if present) to point to us
			if (dfw_next)
			{
				dfw_next->dfw_prev = &dfw_next;
			}
		}
	}

	~DeferredWork()
	{
		// if we are linked
		if (dfw_prev)
		{
			if (dfw_next)
			{
				// adjust previous pointer in next element ...
				dfw_next->dfw_prev = dfw_prev;
			}
			// adjust next pointer in previous element
			*dfw_prev = dfw_next;

			// Adjust end marker of the list
			if (*dfw_end == &dfw_next)
			{
				*dfw_end = dfw_prev;
			}
		}

		for (DeferredWork** itr = dfw_args.begin(); itr < dfw_args.end(); ++itr)
		{
			delete *itr;
		}

		if (dfw_lock)
		{
			LCK_release(JRD_get_thread_data(), dfw_lock);
			delete dfw_lock;
		}
	}

	DeferredWork* findArg(dfw_t type) const
	{
		for (DeferredWork* const* itr = dfw_args.begin(); itr < dfw_args.end(); ++itr)
		{
			DeferredWork* const arg = *itr;

			if (arg->dfw_type == type)
			{
				return arg;
			}
		}

		return NULL;
	}

	DeferredWork** getNextPtr()
	{
		return &dfw_next;
	}

	DeferredWork* getNext() const
	{
		return dfw_next;
	}

	// hash interface
	bool isEqual(const DeferredWork& work) const
	{
		if (dfw_type == work.dfw_type &&
			dfw_id == work.dfw_id &&
			dfw_name == work.dfw_name &&
			dfw_package == work.dfw_package &&
			dfw_sav_number == work.dfw_sav_number)
		{
			return true;
		}
		return false;
	}

	DeferredWork* get() { return this; }

	static size_t hash(const DeferredWork& work, size_t hashSize)
	{
		const int nameLimit = 32;
		char key[sizeof work.dfw_type + sizeof work.dfw_id + nameLimit];
		memset(key, 0, sizeof key);
		char* place = key;

		memcpy(place, &work.dfw_type, sizeof work.dfw_type);
		place += sizeof work.dfw_type;

		memcpy(place, &work.dfw_id, sizeof work.dfw_id);
		place += sizeof work.dfw_id;

		work.dfw_name.copyTo(place, nameLimit);	// It's good enough to have first 32 bytes

		return DefaultHash<DeferredWork>::hash(key, sizeof key, hashSize);
	}
};

class DfwSavePoint;

typedef Hash<
	DfwSavePoint,
	DEFAULT_HASH_SIZE,
	SLONG,
	DfwSavePoint
> DfwSavePointHash;

class DfwSavePoint : public DfwSavePointHash::Entry
{
	SLONG dfw_sav_number;

public:
	DfwHash hash; // Deferred work items posted under this savepoint

	explicit DfwSavePoint(SLONG number) : dfw_sav_number(number) { }

	// hash interface
	bool isEqual(const SLONG& number) const
	{
		return dfw_sav_number == number;
	}

	DfwSavePoint* get() { return this; }

	static SLONG generate(const DfwSavePoint& item)
	{
		return item.dfw_sav_number;
	}
};

// List of deferred work items (with per-savepoint break-down)
class DeferredJob
{
public:
	DfwSavePointHash hash; // Hash set of savepoints, that posted work
	DeferredWork* work;
	DeferredWork** end;

	DeferredJob() : work(NULL), end(&work) { }
};

} // namespace Jrd

/*==================================================================
 *
 * NOTE:
 *
 *	The following functions required the same number of
 *	parameters to be passed.
 *
 *==================================================================
 */
static bool add_file(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool add_shadow(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_shadow(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool compute_security(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_index(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool scan_relation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_trigger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_collation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_collation(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_exception(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool set_generator(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_generator(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool create_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool modify_field(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_global(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_parameter(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_rfr(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool make_version(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool add_difference(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool delete_difference(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool begin_backup(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool end_backup(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool check_not_null(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool store_view_context_type(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool user_management(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool drop_package_header(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool drop_package_body(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool grant_privileges(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool db_crypt(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool set_linger(thread_db*, SSHORT, DeferredWork*, jrd_tra*);
static bool clear_mapping(thread_db*, SSHORT, DeferredWork*, jrd_tra*);

// ----------------------------------------------------------------

static bool create_expression_index(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction);
static void check_computed_dependencies(thread_db* tdbb, jrd_tra* transaction,
	const Firebird::MetaName& fieldName);
static void check_dependencies(thread_db*, const TEXT*, const TEXT*, const TEXT*, int, jrd_tra*);
static void check_filename(const Firebird::string&, bool);
static bool formatsAreEqual(const Format*, const Format*);
static bool	find_depend_in_dfw(thread_db*, TEXT*, USHORT, USHORT, jrd_tra*);
static void get_array_desc(thread_db*, const TEXT*, Ods::InternalArrayDesc*);
static void get_trigger_dependencies(DeferredWork*, bool, jrd_tra*);
static void	load_trigs(thread_db*, jrd_rel*, trig_vec**);
static Format*	make_format(thread_db*, jrd_rel*, USHORT *, TemporaryField*);
static void put_summary_blob(thread_db* tdbb, blb*, enum rsr_t, bid*, jrd_tra*);
static void put_summary_record(thread_db* tdbb, blb*, enum rsr_t, const UCHAR*, USHORT);
static void	setup_array(thread_db*, blb*, const TEXT*, USHORT, TemporaryField*);
static blb*	setup_triggers(thread_db*, jrd_rel*, bool, trig_vec**, blb*);
static void	setup_trigger_details(thread_db*, jrd_rel*, blb*, trig_vec**, const TEXT*, bool);
static bool validate_text_type (thread_db*, const TemporaryField*);

static Lock* protect_relation(thread_db*, jrd_tra*, jrd_rel*, bool&);
static void release_protect_lock(thread_db*, jrd_tra*, Lock*);
static void check_partners(thread_db*, const USHORT);
static string get_string(const dsc* desc);

static ISC_STATUS getErrorCodeByObjectType(int obj_type)
{
	ISC_STATUS err_code = 0;

	switch (obj_type)
	{
	case obj_relation:
		err_code = isc_table_name;
		break;
	case obj_view:
		err_code = isc_view_name;
		break;
	case obj_procedure:
		err_code = isc_proc_name;
		break;
	case obj_collation:
		err_code = isc_collation_name;
		break;
	case obj_exception:
		err_code = isc_exception_name;
		break;
	case obj_field:
		err_code = isc_domain_name;
		break;
	case obj_generator:
		err_code = isc_generator_name;
		break;
	case obj_udf:
		err_code = isc_udf_name;
		break;
	case obj_index:
		err_code = isc_index_name;
		break;
	case obj_package_header:
	case obj_package_body:
		err_code = isc_package_name;
		break;
	default:
		fb_assert(false);
	}

	return err_code;
}

static void raiseDatabaseInUseError(bool timeout)
{
	if (timeout)
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_lock_timeout) <<
				 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
	}

	ERR_post(Arg::Gds(isc_no_meta_update) <<
			 Arg::Gds(isc_obj_in_use) << Arg::Str("DATABASE"));
}

static void raiseObjectInUseError(const string& obj_type, const string& obj_name)
{
	string name;
	name.printf("%s \"%s\"", obj_type.c_str(), obj_name.c_str());

	ERR_post(Arg::Gds(isc_no_meta_update) <<
			 Arg::Gds(isc_obj_in_use) << Arg::Str(name));
}

static void raiseRelationInUseError(const jrd_rel* relation)
{
	const string obj_type =
		relation->isView() ? "VIEW" : "TABLE";
	const string obj_name = relation->rel_name.c_str();

	raiseObjectInUseError(obj_type, obj_name);
}

static void raiseRoutineInUseError(const Routine* routine)
{
	const string obj_type =
		(routine->getObjectType() == obj_udf) ? "FUNCTION" : "PROCEDURE";
	const string obj_name = routine->getName().toString();

	raiseObjectInUseError(obj_type, obj_name);
}

static void raiseTooManyVersionsError(const int obj_type, const string& obj_name)
{
	const ISC_STATUS err_code = getErrorCodeByObjectType(obj_type);

	ERR_post(Arg::Gds(isc_no_meta_update) <<
			 Arg::Gds(err_code) << Arg::Str(obj_name) <<
			 Arg::Gds(isc_version_err));
}

static const UCHAR nonnull_validation_blr[] =
{
	blr_version5,
	blr_not,
	blr_missing,
	blr_fid, 0, 0, 0,
	blr_eoc
};

typedef bool (*dfw_task_routine) (thread_db*, SSHORT, DeferredWork*, jrd_tra*);
struct deferred_task
{
	enum dfw_t task_type;
	dfw_task_routine task_routine;
};

namespace
{
	template <typename Self, typename T, int objType,
		T* (*lookupById)(thread_db*, USHORT, bool, bool, USHORT),
		T* (*lookupByName)(Jrd::thread_db*, const QualifiedName&, bool),
		T* (*loadById)(thread_db*, USHORT, bool, USHORT)
	>
	class RoutineManager
	{
	public:
		// Create a new routine.
		static bool createRoutine(thread_db* tdbb, SSHORT phase, DeferredWork* work,
			jrd_tra* transaction)
		{
			SET_TDBB(tdbb);

			switch (phase)
			{
				case 1:
				case 2:
					return true;

				case 3:
				{
					const bool compile = !work->findArg(dfw_arg_check_blr);
					getDependencies(work, compile, transaction);

					T* routine = lookupByName(tdbb,
						QualifiedName(work->dfw_name, work->dfw_package), compile);

					if (!routine)
						return false;

					break;
				}
			}

			return false;
		}

		// Perform required actions when modifying a routine.
		static bool modifyRoutine(thread_db* tdbb, SSHORT phase, DeferredWork* work,
			jrd_tra* transaction)
		{
			SET_TDBB(tdbb);

			Jrd::Attachment* attachment = tdbb->getAttachment();
			const QualifiedName name(work->dfw_name, work->dfw_package);
			Routine* routine;

			switch (phase)
			{
				case 0:
					routine = lookupById(tdbb, work->dfw_id, false, true, 0);
					if (!routine)
						return false;

					if (routine->existenceLock)
						LCK_convert(tdbb, routine->existenceLock, LCK_SR, transaction->getLockWait());

					return false;

				case 1:
				case 2:
					return true;

				case 3:
					routine = lookupById(tdbb, work->dfw_id, false, true, 0);
					if (!routine)
						return false;

					if (routine->existenceLock)
					{
						// Let routine be deleted if only this transaction is using it

						if (!LCK_convert(tdbb, routine->existenceLock, LCK_EX,
								transaction->getLockWait()))
						{
							raiseRoutineInUseError(routine);
						}
					}

					// If we are in a multi-client server, someone else may have marked
					// routine obsolete. Unmark and we will remark it later.

					routine->flags &= ~Routine::FLAG_OBSOLETE;
					return true;

				case 4:
				{
					routine = lookupById(tdbb, work->dfw_id, false, true, 0);
					if (!routine)
						return false;

					// Do not allow to modify routine used by user requests
					if (routine->isUsed() && MET_routine_in_use(tdbb, routine))
					{
						///raiseRoutineInUseError(routine);
						gds__log("Modifying %s %s which is currently in use by active user requests",
								 Self::getTypeStr(), name.toString().c_str());

						USHORT alterCount = routine->alterCount;

						if (alterCount > Routine::MAX_ALTER_COUNT)
							raiseTooManyVersionsError(routine->getObjectType(), work->dfw_name);

						if (routine->existenceLock)
							LCK_release(tdbb, routine->existenceLock);

						Self::clearId(tdbb->getAttachment(), routine->getId());

						if (!(routine = lookupById(tdbb, work->dfw_id, false,
								true, Routine::FLAG_BEING_ALTERED)))
						{
							return false;
						}

						routine->alterCount = ++alterCount;
					}

					routine->flags |= Routine::FLAG_BEING_ALTERED;

					if (routine->getStatement())
					{
						if (routine->getStatement()->isActive())
							raiseRoutineInUseError(routine);

						// release the request

						routine->releaseStatement(tdbb);
					}

					// delete dependency lists

					if (work->dfw_package.isEmpty())
						MET_delete_dependencies(tdbb, work->dfw_name, objType, transaction);

					/* the routine has just been scanned by lookupById
					   and its Routine::FLAG_SCANNED flag is set. We are going to reread it
					   from file (create all new dependencies) and do not want this
					   flag to be set. That is why we do not add Routine::FLAG_OBSOLETE and
					   Routine::FLAG_BEING_ALTERED flags, we set only these two flags
					 */
					routine->flags = (Routine::FLAG_OBSOLETE | Routine::FLAG_BEING_ALTERED);

					if (routine->existenceLock)
						LCK_release(tdbb, routine->existenceLock);

					// remove routine from cache

					routine->remove(tdbb);

					// Now handle the new definition
					bool compile = !work->findArg(dfw_arg_check_blr);
					getDependencies(work, compile, transaction);

					routine->flags &= ~(Routine::FLAG_OBSOLETE | Routine::FLAG_BEING_ALTERED);

					return true;
				}

				case 5:
					if (work->findArg(dfw_arg_check_blr))
					{
						SSHORT validBlr = FALSE;

						MemoryPool* newPool = attachment->createPool();
						try
						{
							Jrd::ContextPoolHolder context(tdbb, newPool);

							// compile the routine to know if the BLR is still valid
							if (loadById(tdbb, work->dfw_id, false, 0))
								validBlr = TRUE;
						}
						catch (const Firebird::Exception&)
						{
							fb_utils::init_status(tdbb->tdbb_status_vector);
						}

						attachment->deletePool(newPool);

						Self::validate(tdbb, transaction, work, validBlr);
					}

					break;
			}

			return false;
		}

		// Check if it is allowed to delete a routine, and if so, clean up after it.
		static bool deleteRoutine(thread_db* tdbb, SSHORT phase, DeferredWork* work,
			jrd_tra* transaction)
		{
			SET_TDBB(tdbb);
			T* routine = NULL;

			switch (phase)
			{
				case 0:
					routine = lookupById(tdbb, work->dfw_id, false, true, 0);
					if (!routine)
						return false;

					if (routine->existenceLock)
						LCK_convert(tdbb, routine->existenceLock, LCK_SR, transaction->getLockWait());

					return false;

				case 1:
					check_dependencies(tdbb, work->dfw_name.c_str(), NULL, work->dfw_package.c_str(),
						objType, transaction);
					return true;

				case 2:
					routine = lookupById(tdbb, work->dfw_id, false, true, 0);
					if (!routine)
						return false;

					if (routine->existenceLock)
					{
						if (!LCK_convert(tdbb, routine->existenceLock, LCK_EX,
								transaction->getLockWait()))
						{
							raiseRoutineInUseError(routine);
						}
					}

					// If we are in a multi-client server, someone else may have marked
					// routine obsolete. Unmark and we will remark it later.

					routine->flags &= ~Routine::FLAG_OBSOLETE;
					return true;

				case 3:
					return true;

				case 4:
				{
					routine = lookupById(tdbb, work->dfw_id, true, true, 0);
					if (!routine)
						return false;

					const QualifiedName name(work->dfw_name, work->dfw_package);

					// Do not allow to drop routine used by user requests
					if (routine->isUsed() && MET_routine_in_use(tdbb, routine))
					{
						///raiseRoutineInUseError(routine);
						gds__log("Deleting %s %s which is currently in use by active user requests",
							Self::getTypeStr(), name.toString().c_str());

						if (work->dfw_package.isEmpty())
							MET_delete_dependencies(tdbb, work->dfw_name, objType, transaction);

						if (routine->existenceLock)
							LCK_release(tdbb, routine->existenceLock);

						Self::clearId(tdbb->getAttachment(), routine->getId());
						return false;
					}

					const USHORT old_flags = routine->flags;
					routine->flags |= Routine::FLAG_OBSOLETE;

					if (routine->getStatement())
					{
						if (routine->getStatement()->isActive())
						{
							routine->flags = old_flags;
							raiseRoutineInUseError(routine);
						}

						routine->releaseStatement(tdbb);
					}

					// delete dependency lists

					if (work->dfw_package.isEmpty())
						MET_delete_dependencies(tdbb, work->dfw_name, objType, transaction);

					if (routine->existenceLock)
						LCK_release(tdbb, routine->existenceLock);

					break;
				}
			}	// switch

			return false;
		}

	private:
		// Get relations and fields on which this routine depends, either when it's being
		// created or when it's modified.
		static void getDependencies(DeferredWork* work, bool compile, jrd_tra* transaction)
		{
			thread_db* tdbb = JRD_get_thread_data();
			Jrd::Attachment* attachment = tdbb->getAttachment();

			if (compile)
				compile = !(tdbb->getAttachment()->att_flags & ATT_gbak_attachment);

			bid blobId;
			blobId.clear();
			Routine* routine = Self::lookupBlobId(tdbb, work, blobId, compile);

#ifdef DEV_BUILD
			MET_verify_cache(tdbb);
#endif

			// get any dependencies now by parsing the blr

			if (routine && !blobId.isEmpty())
			{
				JrdStatement* statement = NULL;
				// Nickolay Samofatov: allocate statement memory pool...
				MemoryPool* new_pool = attachment->createPool();
				// block is used to ensure MET_verify_cache
				// works in not deleted context
				{
					Jrd::ContextPoolHolder context(tdbb, new_pool);

					const Firebird::MetaName depName(work->dfw_package.isEmpty() ?
						work->dfw_name : work->dfw_package);
					MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &blobId,
						(compile ? &statement : NULL),
						NULL, depName,
						(work->dfw_package.isEmpty() ? objType : obj_package_body),
						0, transaction);

					if (statement)
						statement->release(tdbb);
					else
						attachment->deletePool(new_pool);
				}

#ifdef DEV_BUILD
				MET_verify_cache(tdbb);
#endif
			}
		}
	};

	class FunctionManager : public RoutineManager<FunctionManager, Function, obj_udf,
		Function::lookup, Function::lookup, Function::loadMetadata>
	{
	public:
		static const char* const getTypeStr()
		{
			return "function";
		}

		static void clearId(Jrd::Attachment* attachment, USHORT id)
		{
			attachment->att_functions[id] = NULL;
		}

		static Routine* lookupBlobId(thread_db* tdbb, DeferredWork* work, bid& blobId, bool compile);
		static void validate(thread_db* tdbb, jrd_tra* transaction, DeferredWork* work,
			SSHORT validBlr);
	};

	class ProcedureManager : public RoutineManager<ProcedureManager, jrd_prc, obj_procedure,
		MET_lookup_procedure_id, MET_lookup_procedure, MET_procedure>
	{
	public:
		static const char* const getTypeStr()
		{
			return "procedure";
		}

		static void clearId(Jrd::Attachment* attachment, USHORT id)
		{
			attachment->att_procedures[id] = NULL;
		}

		static Routine* lookupBlobId(thread_db* tdbb, DeferredWork* work, bid& blobId, bool compile);
		static void validate(thread_db* tdbb, jrd_tra* transaction, DeferredWork* work,
			SSHORT validBlr);
	};

	// These methods cannot be defined inline, because GPRE generates wrong code.

	Routine* FunctionManager::lookupBlobId(thread_db* tdbb, DeferredWork* work, bid& blobId,
		bool compile)
	{
	   struct {
	          bid  jrd_524;	// RDB$FUNCTION_BLR 
	          SSHORT jrd_525;	// gds__utility 
	   } jrd_523;
	   struct {
	          TEXT  jrd_521 [32];	// RDB$PACKAGE_NAME 
	          TEXT  jrd_522 [32];	// RDB$FUNCTION_NAME 
	   } jrd_520;
		Jrd::Attachment* attachment = tdbb->getAttachment();
		AutoCacheRequest handle(tdbb, irq_c_fun_dpd, IRQ_REQUESTS);
		Routine* routine = NULL;

		/*FOR(REQUEST_HANDLE handle)
			X IN RDB$FUNCTIONS WITH
				X.RDB$FUNCTION_NAME EQ work->dfw_name.c_str() AND
				X.RDB$PACKAGE_NAME EQUIV NULLIF(work->dfw_package.c_str(), '')*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_519, sizeof(jrd_519));
		gds__vtov ((const char*) work->dfw_package.c_str(), (char*) jrd_520.jrd_521, 32);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_520.jrd_522, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_520);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_523);
		   if (!jrd_523.jrd_525) break;
		{
			blobId = /*X.RDB$FUNCTION_BLR*/
				 jrd_523.jrd_524;
			routine = Function::lookup(tdbb,
				QualifiedName(work->dfw_name, work->dfw_package), !compile);
		}
		/*END_FOR*/
		   }
		}

		return routine;
	}

	void FunctionManager::validate(thread_db* tdbb, jrd_tra* transaction, DeferredWork* work,
		SSHORT validBlr)
	{
	   struct {
	          SSHORT jrd_518;	// gds__utility 
	   } jrd_517;
	   struct {
	          SSHORT jrd_515;	// gds__null_flag 
	          SSHORT jrd_516;	// RDB$VALID_BLR 
	   } jrd_514;
	   struct {
	          SSHORT jrd_511;	// gds__utility 
	          SSHORT jrd_512;	// gds__null_flag 
	          SSHORT jrd_513;	// RDB$VALID_BLR 
	   } jrd_510;
	   struct {
	          SSHORT jrd_509;	// RDB$FUNCTION_ID 
	   } jrd_508;
		Jrd::Attachment* attachment = tdbb->getAttachment();
		AutoCacheRequest request(tdbb, irq_fun_validate, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			FUN IN RDB$FUNCTIONS
			WITH FUN.RDB$FUNCTION_ID EQ work->dfw_id AND
				 FUN.RDB$FUNCTION_BLR NOT MISSING*/
		{
		request.compile(tdbb, (UCHAR*) jrd_507, sizeof(jrd_507));
		jrd_508.jrd_509 = work->dfw_id;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_508);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_510);
		   if (!jrd_510.jrd_511) break;
		{
			/*MODIFY FUN USING*/
			{
			
				/*FUN.RDB$VALID_BLR*/
				jrd_510.jrd_513 = validBlr;
				/*FUN.RDB$VALID_BLR.NULL*/
				jrd_510.jrd_512 = FALSE;
			/*END_MODIFY*/
			jrd_514.jrd_515 = jrd_510.jrd_512;
			jrd_514.jrd_516 = jrd_510.jrd_513;
			EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_514);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_517);
		   }
		}
	}

	Routine* ProcedureManager::lookupBlobId(thread_db* tdbb, DeferredWork* work, bid& blobId,
		bool compile)
	{
	   struct {
	          bid  jrd_505;	// RDB$PROCEDURE_BLR 
	          SSHORT jrd_506;	// gds__utility 
	   } jrd_504;
	   struct {
	          TEXT  jrd_502 [32];	// RDB$PACKAGE_NAME 
	          TEXT  jrd_503 [32];	// RDB$PROCEDURE_NAME 
	   } jrd_501;
		Jrd::Attachment* attachment = tdbb->getAttachment();
		AutoCacheRequest handle(tdbb, irq_c_prc_dpd, IRQ_REQUESTS);
		Routine* routine = NULL;

		/*FOR(REQUEST_HANDLE handle)
			X IN RDB$PROCEDURES WITH
				X.RDB$PROCEDURE_NAME EQ work->dfw_name.c_str() AND
				X.RDB$PACKAGE_NAME EQUIV NULLIF(work->dfw_package.c_str(), '')*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_500, sizeof(jrd_500));
		gds__vtov ((const char*) work->dfw_package.c_str(), (char*) jrd_501.jrd_502, 32);
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_501.jrd_503, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 64, (UCHAR*) &jrd_501);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_504);
		   if (!jrd_504.jrd_506) break;
		{
			blobId = /*X.RDB$PROCEDURE_BLR*/
				 jrd_504.jrd_505;
			routine = MET_lookup_procedure(tdbb,
				QualifiedName(work->dfw_name, work->dfw_package), !compile);
		}
		/*END_FOR*/
		   }
		}

		return routine;
	}

	void ProcedureManager::validate(thread_db* tdbb, jrd_tra* transaction, DeferredWork* work,
		SSHORT validBlr)
	{
	   struct {
	          SSHORT jrd_499;	// gds__utility 
	   } jrd_498;
	   struct {
	          SSHORT jrd_496;	// gds__null_flag 
	          SSHORT jrd_497;	// RDB$VALID_BLR 
	   } jrd_495;
	   struct {
	          SSHORT jrd_492;	// gds__utility 
	          SSHORT jrd_493;	// gds__null_flag 
	          SSHORT jrd_494;	// RDB$VALID_BLR 
	   } jrd_491;
	   struct {
	          SSHORT jrd_490;	// RDB$PROCEDURE_ID 
	   } jrd_489;
		Jrd::Attachment* attachment = tdbb->getAttachment();
		AutoCacheRequest request(tdbb, irq_prc_validate, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			PRC IN RDB$PROCEDURES
			WITH PRC.RDB$PROCEDURE_ID EQ work->dfw_id AND
				 PRC.RDB$PROCEDURE_BLR NOT MISSING*/
		{
		request.compile(tdbb, (UCHAR*) jrd_488, sizeof(jrd_488));
		jrd_489.jrd_490 = work->dfw_id;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_489);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_491);
		   if (!jrd_491.jrd_492) break;
		{
			/*MODIFY PRC USING*/
			{
			
				/*PRC.RDB$VALID_BLR*/
				jrd_491.jrd_494 = validBlr;
				/*PRC.RDB$VALID_BLR.NULL*/
				jrd_491.jrd_493 = FALSE;
			/*END_MODIFY*/
			jrd_495.jrd_496 = jrd_491.jrd_493;
			jrd_495.jrd_497 = jrd_491.jrd_494;
			EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_495);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_498);
		   }
		}
	}
} // namespace

static const deferred_task task_table[] =
{
	{ dfw_add_file, add_file },
	{ dfw_add_shadow, add_shadow },
	{ dfw_delete_index, modify_index },
	{ dfw_delete_expression_index, modify_index },
	{ dfw_delete_rfr, delete_rfr },
	{ dfw_delete_relation, delete_relation },
	{ dfw_delete_shadow, delete_shadow },
	{ dfw_create_field, create_field },
	{ dfw_delete_field, delete_field },
	{ dfw_modify_field, modify_field },
	{ dfw_delete_global, delete_global },
	{ dfw_create_relation, create_relation },
	{ dfw_update_format, make_version },
	{ dfw_scan_relation, scan_relation },
	{ dfw_compute_security, compute_security },
	{ dfw_create_index, modify_index },
	{ dfw_create_expression_index, modify_index },
	{ dfw_grant, grant_privileges },
	{ dfw_create_trigger, create_trigger },
	{ dfw_delete_trigger, delete_trigger },
	{ dfw_modify_trigger, modify_trigger },
	{ dfw_drop_package_header, drop_package_header },	// packages should be before procedures
	{ dfw_drop_package_body, drop_package_body },		// packages should be before procedures
	{ dfw_create_procedure, ProcedureManager::createRoutine },
	{ dfw_create_function, FunctionManager::createRoutine },
	{ dfw_delete_procedure, ProcedureManager::deleteRoutine },
	{ dfw_delete_function, FunctionManager::deleteRoutine },
	{ dfw_modify_procedure, ProcedureManager::modifyRoutine },
	{ dfw_modify_function, FunctionManager::modifyRoutine },
	{ dfw_delete_prm, delete_parameter },
	{ dfw_create_collation, create_collation },
	{ dfw_delete_collation, delete_collation },
	{ dfw_delete_exception, delete_exception },
	{ dfw_set_generator, set_generator },
	{ dfw_delete_generator, delete_generator },
	{ dfw_add_difference, add_difference },
	{ dfw_delete_difference, delete_difference },
	{ dfw_begin_backup, begin_backup },
	{ dfw_end_backup, end_backup },
	{ dfw_user_management, user_management },
	{ dfw_check_not_null, check_not_null },
	{ dfw_store_view_context_type, store_view_context_type },
	{ dfw_db_crypt, db_crypt },
	{ dfw_set_linger, set_linger },
	{ dfw_clear_mapping, clear_mapping },
	{ dfw_null, NULL }
};


USHORT DFW_assign_index_type(thread_db* tdbb, const Firebird::MetaName& name, SSHORT field_type,
	SSHORT ttype)
{
/**************************************
 *
 *	D F W _ a s s i g n _ i n d e x _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Define the index segment type based
 * 	on the field's type and subtype.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (field_type == dtype_varying || field_type == dtype_cstring || field_type == dtype_text)
	{
	    switch (ttype)
	    {
		case ttype_none:
			return idx_string;
		case ttype_binary:
			return idx_byte_array;
		case ttype_metadata:
			return idx_metadata;
		case ttype_ascii:
			return idx_string;
		}

		// Dynamic text cannot occur here as this is for an on-disk
		// index, which must be bound to a text type.

		fb_assert(ttype != ttype_dynamic);

		if (INTL_defined_type(tdbb, ttype))
			return INTL_TEXT_TO_INDEX(ttype);

		ERR_post_nothrow(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_random) << Arg::Str(name));
		INTL_texttype_lookup(tdbb, ttype);	// should punt
		ERR_punt(); // if INTL_texttype_lookup hasn't punt
	}

	switch (field_type)
	{
	case dtype_timestamp:
		return idx_timestamp;
	case dtype_sql_date:
		return idx_sql_date;
	case dtype_sql_time:
		return idx_sql_time;
	// idx_numeric2 used for 64-bit Integer support
	case dtype_int64:
		return idx_numeric2;
	case dtype_boolean:
		return idx_boolean;
	default:
		return idx_numeric;
	}
}


void DFW_delete_deferred( jrd_tra* transaction, SLONG sav_number)
{
/**************************************
 *
 *	D F W _ d e l e t e _ d e f e r r e d
 *
 **************************************
 *
 * Functional description
 *	Get rid of work deferred that was to be done at
 *	COMMIT time as the statement has been rolled back.
 *
 *	if (sav_number == -1), then  remove all entries.
 *
 **************************************/

	// If there is no deferred work, just return

	if (!transaction->tra_deferred_job) {
		return;
	}

	// Remove deferred work and events which are to be rolled back

	if (sav_number == -1)
	{
		DeferredWork* work;
		while (work = transaction->tra_deferred_job->work)
		{
			delete work;
		}
		transaction->tra_flags &= ~TRA_deferred_meta;
		return;
	}

	DfwSavePoint* h = transaction->tra_deferred_job->hash.lookup(sav_number);
	if (!h)
	{
		return;
	}

	for (DfwHash::iterator i(h->hash); i.hasData();)
	{
		DeferredWork* work(i);
		++i;
		delete work;
	}
}


// Get (by reference) the array of IDs present in a DeferredWork.
SortedArray<int>& DFW_get_ids(DeferredWork* work)
{
	return work->dfw_ids;
}


void DFW_merge_work(jrd_tra* transaction, SLONG old_sav_number, SLONG new_sav_number)
{
/**************************************
 *
 *	D F W _ m e r g e _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Merge the deferred work with the previous level.  This will
 *	be called only if there is a previous level.
 *
 **************************************/

	// If there is no deferred work, just return

	DeferredJob *job = transaction->tra_deferred_job;
	if (! job)
	{
		return;
	}

	// Check to see if work is already posted

	DfwSavePoint* oldSp = job->hash.lookup(old_sav_number);
	if (!oldSp)
	{
		return;
	}

	DfwSavePoint* newSp = job->hash.lookup(new_sav_number);

	// Decrement the save point number in the deferred block
	// i.e. merge with the previous level.

	for (DfwHash::iterator itr(oldSp->hash); itr.hasData();)
	{
		if (! newSp)
		{
			newSp = FB_NEW(*transaction->tra_pool) DfwSavePoint(new_sav_number);
			job->hash.add(newSp);
		}

		DeferredWork* work(itr);
		++itr;
		oldSp->hash.remove(*work); // After ++itr
		work->dfw_sav_number = new_sav_number;

		DeferredWork* newWork = newSp->hash.lookup(*work);

		if (!newWork)
			newSp->hash.add(work);
		else
		{
			SortedArray<int>& workIds = work->dfw_ids;

			for (SortedArray<int>::iterator itr2(workIds.begin()); itr2 != workIds.end(); ++itr2)
			{
				int n = *itr2;
				if (!newWork->dfw_ids.exist(n))
					newWork->dfw_ids.add(n);
			}

			newWork->dfw_count += work->dfw_count;
			delete work;
		}
	}

	job->hash.remove(old_sav_number);
	delete oldSp;
}


void DFW_perform_system_work(thread_db* tdbb)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ s y s t e m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Flush out the work left to be done in the
 *	system transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	DFW_perform_work(tdbb, attachment->getSysTransaction());
}


void DFW_perform_work(thread_db* tdbb, jrd_tra* transaction)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Do work deferred to COMMIT time 'cause that time has
 *	come.
 *
 **************************************/

	// If no deferred work or it's all deferred event posting don't bother

	if (!transaction->tra_deferred_job || !(transaction->tra_flags & TRA_deferred_meta))
	{
		return;
	}

	SET_TDBB(tdbb);
	Jrd::ContextPoolHolder context(tdbb, transaction->tra_pool);

	/* Loop for as long as any of the deferred work routines says that it has
	more to do.  A deferred work routine should be able to deal with any
	value of phase, either to say that it wants to be called again in the
	next phase (by returning true) or that it has nothing more to do in this
	or later phases (by returning false). By convention, phase 0 has been
	designated as the cleanup phase. If any non-zero phase punts, then phase 0
	is executed for all deferred work blocks to cleanup work-in-progress. */

	bool dump_shadow = false;
	SSHORT phase = 1;
	bool more;
	ISC_STATUS_ARRAY err_status = {0};

	do
	{
		more = false;
		try {
			tdbb->tdbb_flags |= (TDBB_dont_post_dfw | TDBB_use_db_page_space);
			for (const deferred_task* task = task_table; task->task_type != dfw_null; ++task)
			{
				for (DeferredWork* work = transaction->tra_deferred_job->work; work; work = work->getNext())
				{
					if (work->dfw_type == task->task_type)
					{
						if (work->dfw_type == dfw_add_shadow)
						{
							dump_shadow = true;
						}
						if ((*task->task_routine)(tdbb, phase, work, transaction))
						{
							more = true;
						}
					}
				}
			}
			if (!phase)
			{
				Firebird::makePermanentVector(tdbb->tdbb_status_vector, err_status);
				ERR_punt();
			}
			++phase;
			tdbb->tdbb_flags &= ~(TDBB_dont_post_dfw | TDBB_use_db_page_space);
		}
		catch (const Firebird::Exception& ex)
		{
			tdbb->tdbb_flags &= ~(TDBB_dont_post_dfw | TDBB_use_db_page_space);

			// Do any necessary cleanup
			if (!phase)
			{
				ex.stuff_exception(tdbb->tdbb_status_vector);
				ERR_punt();
			}
			else
				ex.stuff_exception(err_status);

			phase = 0;
			more = true;
		}

	} while (more);

	// Remove deferred work blocks so that system transaction and
	// commit retaining transactions don't re-execute them. Leave
	// events to be posted after commit

	for (DeferredWork* itr = transaction->tra_deferred_job->work; itr;)
	{
		DeferredWork* work = itr;
		itr = itr->getNext();

		switch (work->dfw_type)
		{
		case dfw_post_event:
		case dfw_delete_shadow:
			break;

		default:
			delete work;
			break;
		}
	}

	transaction->tra_flags &= ~TRA_deferred_meta;

	if (dump_shadow) {
		SDW_dump_pages(tdbb);
	}
}


void DFW_perform_post_commit_work(jrd_tra* transaction)
{
/**************************************
 *
 *	D F W _ p e r f o r m _ p o s t _ c o m m i t _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Perform any post commit work
 *	1. Post any pending events.
 *	2. Unlink shadow files for dropped shadows
 *
 *	Then, delete it from chain of pending work.
 *
 **************************************/

	if (!transaction->tra_deferred_job)
		return;

	bool pending_events = false;

	Database* dbb = GET_DBB();

	for (DeferredWork* itr = transaction->tra_deferred_job->work; itr;)
	{
		DeferredWork* work = itr;
		itr = itr->getNext();

		switch (work->dfw_type)
		{
		case dfw_post_event:
			EventManager::init(transaction->tra_attachment);

			dbb->dbb_event_mgr->postEvent(work->dfw_name.length(), work->dfw_name.c_str(),
										  work->dfw_count);

			delete work;
			pending_events = true;
			break;
		case dfw_delete_shadow:
			unlink(work->dfw_name.c_str());
			delete work;
			break;
		default:
			break;
		}
	}

	if (pending_events)
	{
		dbb->dbb_event_mgr->deliverEvents();
	}
}


DeferredWork* DFW_post_system_work(thread_db* tdbb, enum dfw_t type, const dsc* desc, USHORT id)
{
/**************************************
 *
 *	D F W _ p o s t _ s y s t e m _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Post work to be done in the context of system transaction.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	return DFW_post_work(attachment->getSysTransaction(), type, desc, id, "");
}


DeferredWork* DFW_post_work(jrd_tra* transaction, enum dfw_t type, const dsc* desc, USHORT id,
	const MetaName& package)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Post work to be deferred to commit time.
 *
 **************************************/

	return DFW_post_work(transaction, type, get_string(desc), id, package);
}


DeferredWork* DFW_post_work(jrd_tra* transaction, enum dfw_t type, const string& name, USHORT id,
	const MetaName& package)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k
 *
 **************************************
 *
 * Functional description
 *	Post work to be deferred to commit time.
 *
 **************************************/

	// get the current save point number

	const SLONG sav_number = transaction->tra_save_point ?
		transaction->tra_save_point->sav_number : 0;

	// initialize transaction if needed

	DeferredJob *job = transaction->tra_deferred_job;
	if (! job)
	{
		transaction->tra_deferred_job = job = FB_NEW(*transaction->tra_pool) DeferredJob;
	}

	// Check to see if work is already posted

	DfwSavePoint* sp = job->hash.lookup(sav_number);
	if (! sp)
	{
		sp = FB_NEW(*transaction->tra_pool) DfwSavePoint(sav_number);
		job->hash.add(sp);
	}

	DeferredWork tmp(AutoStorage::getAutoMemoryPool(), 0, type, id, sav_number, name, package);
	DeferredWork* work = sp->hash.lookup(tmp);
	if (work)
	{
		work->dfw_count++;
		return work;
	}

	// Not already posted, so do so now.

	work = FB_NEW(*transaction->tra_pool)
		DeferredWork(*transaction->tra_pool, &(job->end), type, id, sav_number, name, package);
	job->end = work->getNextPtr();
	fb_assert(!(*job->end));
	sp->hash.add(work);

	switch (type)
	{
	case dfw_user_management:
	case dfw_set_generator:
		transaction->tra_flags |= TRA_deferred_meta;
		// fall down ...
	case dfw_post_event:
		if (transaction->tra_save_point)
			transaction->tra_save_point->sav_flags |= SAV_force_dfw;
		break;
	default:
		transaction->tra_flags |= TRA_deferred_meta;
		break;
	}

	return work;
}


DeferredWork* DFW_post_work_arg( jrd_tra* transaction, DeferredWork* work, const dsc* desc,
	USHORT id)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k _ a r g
 *
 **************************************
 *
 * Functional description
 *	Post an argument for work to be deferred to commit time.
 *
 **************************************/
	return DFW_post_work_arg(transaction, work, desc, id, work->dfw_type);
}


DeferredWork* DFW_post_work_arg( jrd_tra* transaction, DeferredWork* work, const dsc* desc,
	USHORT id, Jrd::dfw_t type)
{
/**************************************
 *
 *	D F W _ p o s t _ w o r k _ a r g
 *
 **************************************
 *
 * Functional description
 *	Post an argument for work to be deferred to commit time.
 *
 **************************************/
	const Firebird::string name = get_string(desc);

	DeferredWork* arg = work->findArg(type);

	if (! arg)
	{
		arg = FB_NEW(*transaction->tra_pool)
			DeferredWork(*transaction->tra_pool, 0, type, id, 0, name, "");
		work->dfw_args.add(arg);
	}

	return arg;
}


void DFW_update_index(const TEXT* name, USHORT id, const SelectivityList& selectivity,
	jrd_tra* transaction)
{
   struct {
          SSHORT jrd_475;	// gds__utility 
   } jrd_474;
   struct {
          double  jrd_472;	// RDB$STATISTICS 
          SSHORT jrd_473;	// RDB$INDEX_ID 
   } jrd_471;
   struct {
          double  jrd_468;	// RDB$STATISTICS 
          SSHORT jrd_469;	// gds__utility 
          SSHORT jrd_470;	// RDB$INDEX_ID 
   } jrd_467;
   struct {
          TEXT  jrd_466 [32];	// RDB$INDEX_NAME 
   } jrd_465;
   struct {
          SSHORT jrd_487;	// gds__utility 
   } jrd_486;
   struct {
          double  jrd_484;	// RDB$STATISTICS 
          SSHORT jrd_485;	// RDB$FIELD_POSITION 
   } jrd_483;
   struct {
          double  jrd_480;	// RDB$STATISTICS 
          SSHORT jrd_481;	// gds__utility 
          SSHORT jrd_482;	// RDB$FIELD_POSITION 
   } jrd_479;
   struct {
          TEXT  jrd_478 [32];	// RDB$INDEX_NAME 
   } jrd_477;
/**************************************
 *
 *	D F W _ u p d a t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Update information in the index relation after creation
 *	of the index.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	AutoCacheRequest request(tdbb, irq_m_index_seg, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		SEG IN RDB$INDEX_SEGMENTS WITH SEG.RDB$INDEX_NAME EQ name
			SORTED BY SEG.RDB$FIELD_POSITION*/
	{
	request.compile(tdbb, (UCHAR*) jrd_476, sizeof(jrd_476));
	gds__vtov ((const char*) name, (char*) jrd_477.jrd_478, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_477);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_479);
	   if (!jrd_479.jrd_481) break;
	{
		/*MODIFY SEG USING*/
		{
		
			/*SEG.RDB$STATISTICS*/
			jrd_479.jrd_480 = selectivity[/*SEG.RDB$FIELD_POSITION*/
	       jrd_479.jrd_482];
		/*END_MODIFY*/
		jrd_483.jrd_484 = jrd_479.jrd_480;
		jrd_483.jrd_485 = jrd_479.jrd_482;
		EXE_send (tdbb, request, 2, 10, (UCHAR*) &jrd_483);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_486);
	   }
	}

	request.reset(tdbb, irq_m_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
		IDX IN RDB$INDICES WITH IDX.RDB$INDEX_NAME EQ name*/
	{
	request.compile(tdbb, (UCHAR*) jrd_464, sizeof(jrd_464));
	gds__vtov ((const char*) name, (char*) jrd_465.jrd_466, 32);
	EXE_start (tdbb, request, transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_465);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_467);
	   if (!jrd_467.jrd_469) break;
	{
		/*MODIFY IDX USING*/
		{
		
			/*IDX.RDB$INDEX_ID*/
			jrd_467.jrd_470 = id + 1;
			/*IDX.RDB$STATISTICS*/
			jrd_467.jrd_468 = selectivity.back();
		/*END_MODIFY*/
		jrd_471.jrd_472 = jrd_467.jrd_468;
		jrd_471.jrd_473 = jrd_467.jrd_470;
		EXE_send (tdbb, request, 2, 10, (UCHAR*) &jrd_471);
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_474);
	   }
	}
}


static bool add_file(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_440;	// gds__utility 
   } jrd_439;
   struct {
          SLONG  jrd_437;	// RDB$FILE_LENGTH 
          SLONG  jrd_438;	// RDB$FILE_START 
   } jrd_436;
   struct {
          SLONG  jrd_433;	// RDB$FILE_START 
          SLONG  jrd_434;	// RDB$FILE_LENGTH 
          SSHORT jrd_435;	// gds__utility 
   } jrd_432;
   struct {
          SSHORT jrd_430;	// RDB$SHADOW_NUMBER 
          SSHORT jrd_431;	// RDB$FILE_SEQUENCE 
   } jrd_429;
   struct {
          SLONG  jrd_445;	// RDB$FILE_LENGTH 
          SLONG  jrd_446;	// RDB$FILE_START 
          SSHORT jrd_447;	// gds__utility 
   } jrd_444;
   struct {
          SSHORT jrd_443;	// RDB$SHADOW_NUMBER 
   } jrd_442;
   struct {
          TEXT  jrd_463 [256];	// RDB$FILE_NAME 
   } jrd_462;
   struct {
          SSHORT jrd_461;	// gds__utility 
   } jrd_460;
   struct {
          SLONG  jrd_458;	// RDB$FILE_START 
          SSHORT jrd_459;	// RDB$FILE_SEQUENCE 
   } jrd_457;
   struct {
          TEXT  jrd_452 [256];	// RDB$FILE_NAME 
          SLONG  jrd_453;	// RDB$FILE_START 
          SSHORT jrd_454;	// gds__utility 
          SSHORT jrd_455;	// RDB$FILE_SEQUENCE 
          SSHORT jrd_456;	// RDB$SHADOW_NUMBER 
   } jrd_451;
   struct {
          TEXT  jrd_450 [256];	// RDB$FILE_NAME 
   } jrd_449;
/**************************************
 *
 *	a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to a database.
 *	This file could be a regular database
 *	file or a shadow file.  Either way we
 *	require exclusive access to the database.
 *
 **************************************/
	USHORT section, shadow_number;
	SLONG start, max;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		CCH_release_exclusive(tdbb);
		return false;

	case 1:
	case 2:
		return true;

	case 3:
		if (!CCH_exclusive(tdbb, LCK_EX, WAIT_PERIOD, NULL))
			raiseDatabaseInUseError(true);
		return true;

	case 4:
		CCH_flush(tdbb, FLUSH_FINI, 0);
		max = PageSpace::maxAlloc(dbb) + 1;
		AutoRequest handle;
		AutoRequest handle2;

		// Check the file name for node name. This has already
		// been done for shadows in add_shadow()

		if (work->dfw_type != dfw_add_shadow) {
			check_filename(work->dfw_name, true);
		}

		// User transaction may be safely used instead of system, cause
		// we requested and got exclusive database access. AP-2008.

		// get any files to extend into

		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) X IN RDB$FILES
			WITH X.RDB$FILE_NAME EQ work->dfw_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_448, sizeof(jrd_448));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_449.jrd_450, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_449);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 266, (UCHAR*) &jrd_451);
		   if (!jrd_451.jrd_454) break;
			// First expand the file name This has already been done
			// for shadows in add_shadow ())
			if (work->dfw_type != dfw_add_shadow)
			{
				/*MODIFY X USING*/
				{
				
					ISC_expand_filename(/*X.RDB$FILE_NAME*/
							    jrd_451.jrd_452, 0,
						/*X.RDB$FILE_NAME*/
						jrd_451.jrd_452, sizeof(/*X.RDB$FILE_NAME*/
	 jrd_451.jrd_452), false);
				/*END_MODIFY*/
				gds__vtov((const char*) jrd_451.jrd_452, (char*) jrd_462.jrd_463, 256);
				EXE_send (tdbb, handle, 4, 256, (UCHAR*) &jrd_462);
				}
			}

			// If there is no starting position specified, or if it is
			// too low a value, make a stab at assigning one based on
			// the indicated preference for the previous file length.

			if ((start = /*X.RDB$FILE_START*/
				     jrd_451.jrd_453) < max)
			{
				/*FOR(REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
					FIRST 1 Y IN RDB$FILES
						WITH Y.RDB$SHADOW_NUMBER EQ X.RDB$SHADOW_NUMBER
						AND Y.RDB$FILE_SEQUENCE NOT MISSING
						SORTED BY DESCENDING Y.RDB$FILE_SEQUENCE*/
				{
				handle2.compile(tdbb, (UCHAR*) jrd_441, sizeof(jrd_441));
				jrd_442.jrd_443 = jrd_451.jrd_456;
				EXE_start (tdbb, handle2, transaction);
				EXE_send (tdbb, handle2, 0, 2, (UCHAR*) &jrd_442);
				while (1)
				   {
				   EXE_receive (tdbb, handle2, 1, 10, (UCHAR*) &jrd_444);
				   if (!jrd_444.jrd_447) break;
				{
					start = /*Y.RDB$FILE_START*/
						jrd_444.jrd_446 + /*Y.RDB$FILE_LENGTH*/
   jrd_444.jrd_445;
				}
				/*END_FOR*/
				   }
				}
			}

			start = MAX(max, start);
			shadow_number = /*X.RDB$SHADOW_NUMBER*/
					jrd_451.jrd_456;
			if ((shadow_number &&
				(section = SDW_add_file(tdbb, /*X.RDB$FILE_NAME*/
							      jrd_451.jrd_452, start, shadow_number))) ||
				(section = PAG_add_file(tdbb, /*X.RDB$FILE_NAME*/
							      jrd_451.jrd_452, start)))
			{
				/*MODIFY X USING*/
				{
				
					/*X.RDB$FILE_SEQUENCE*/
					jrd_451.jrd_455 = section;
					/*X.RDB$FILE_START*/
					jrd_451.jrd_453 = start;
				/*END_MODIFY*/
				jrd_457.jrd_458 = jrd_451.jrd_453;
				jrd_457.jrd_459 = jrd_451.jrd_455;
				EXE_send (tdbb, handle, 2, 6, (UCHAR*) &jrd_457);
				}
			}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_460);
		   }
		}

		if (section)
		{
			handle.reset();
			section--;
			/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) X IN RDB$FILES
				WITH X.RDB$FILE_SEQUENCE EQ section
					AND X.RDB$SHADOW_NUMBER EQ shadow_number*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_428, sizeof(jrd_428));
			jrd_429.jrd_430 = shadow_number;
			jrd_429.jrd_431 = section;
			EXE_start (tdbb, handle, transaction);
			EXE_send (tdbb, handle, 0, 4, (UCHAR*) &jrd_429);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 10, (UCHAR*) &jrd_432);
			   if (!jrd_432.jrd_435) break;
			{
				/*MODIFY X USING*/
				{
				
					/*X.RDB$FILE_LENGTH*/
					jrd_432.jrd_434 = start - /*X.RDB$FILE_START*/
	   jrd_432.jrd_433;
				/*END_MODIFY*/
				jrd_436.jrd_437 = jrd_432.jrd_434;
				jrd_436.jrd_438 = jrd_432.jrd_433;
				EXE_send (tdbb, handle, 2, 8, (UCHAR*) &jrd_436);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_439);
			   }
			}
		}

		CCH_release_exclusive(tdbb);
		break;
	}

	return false;
}



static bool add_shadow(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_415;	// gds__utility 
   } jrd_414;
   struct {
          SLONG  jrd_411;	// RDB$FILE_START 
          SSHORT jrd_412;	// RDB$FILE_FLAGS 
          SSHORT jrd_413;	// RDB$FILE_SEQUENCE 
   } jrd_410;
   struct {
          TEXT  jrd_404 [256];	// RDB$FILE_NAME 
          SLONG  jrd_405;	// RDB$FILE_START 
          SSHORT jrd_406;	// gds__utility 
          SSHORT jrd_407;	// RDB$FILE_SEQUENCE 
          SSHORT jrd_408;	// RDB$FILE_FLAGS 
          SSHORT jrd_409;	// RDB$SHADOW_NUMBER 
   } jrd_403;
   struct {
          TEXT  jrd_402 [256];	// RDB$FILE_NAME 
   } jrd_401;
   struct {
          SSHORT jrd_427;	// gds__utility 
   } jrd_426;
   struct {
          TEXT  jrd_425 [256];	// RDB$FILE_NAME 
   } jrd_424;
   struct {
          TEXT  jrd_420 [256];	// RDB$FILE_NAME 
          SSHORT jrd_421;	// gds__utility 
          SSHORT jrd_422;	// RDB$FILE_FLAGS 
          SSHORT jrd_423;	// RDB$SHADOW_NUMBER 
   } jrd_419;
   struct {
          TEXT  jrd_418 [256];	// RDB$FILE_NAME 
   } jrd_417;
/**************************************
 *
 *	a d d _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	A file or files have been added for shadowing.
 *	Get all files for this particular shadow first
 *	in order of starting page, if specified, then
 *	in sequence order.
 *
 **************************************/

	AutoRequest handle;
	Shadow* shadow;
	USHORT sequence, add_sequence;
	bool finished;
	ULONG min_page;
	Firebird::PathName expanded_fname;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		CCH_release_exclusive(tdbb);
		return false;

	case 1:
	case 2:
	case 3:
		return true;

	case 4:
		check_filename(work->dfw_name, false);

		/* could have two cases:
		   1) this shadow has already been written to, so add this file using
		   the standard routine to extend a database
		   2) this file is part of a newly added shadow which has already been
		   fetched in totem and prepared for writing to, so just ignore it
		 */

		finished = false;
		handle.reset();
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
			F IN RDB$FILES
				WITH F.RDB$FILE_NAME EQ work->dfw_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_416, sizeof(jrd_416));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_417.jrd_418, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_417);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 262, (UCHAR*) &jrd_419);
		   if (!jrd_419.jrd_421) break;

			expanded_fname = /*F.RDB$FILE_NAME*/
					 jrd_419.jrd_420;
			ISC_expand_filename(expanded_fname, false);
			/*MODIFY F USING*/
			{
			
				expanded_fname.copyTo(/*F.RDB$FILE_NAME*/
						      jrd_419.jrd_420, sizeof(/*F.RDB$FILE_NAME*/
	 jrd_419.jrd_420));
			/*END_MODIFY*/
			gds__vtov((const char*) jrd_419.jrd_420, (char*) jrd_424.jrd_425, 256);
			EXE_send (tdbb, handle, 2, 256, (UCHAR*) &jrd_424);
			}

			for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
			{
				if ((/*F.RDB$SHADOW_NUMBER*/
				     jrd_419.jrd_423 == shadow->sdw_number) && !(shadow->sdw_flags & SDW_IGNORE))
				{
					if (/*F.RDB$FILE_FLAGS*/
					    jrd_419.jrd_422 & FILE_shadow)
					{
						// This is the case of a bogus duplicate posted
						// work when we added a multi-file shadow
						finished = true;
					}
					else if (shadow->sdw_flags & (SDW_dumped))
					{
						/* Case of adding a file to a currently active
						 * shadow set.
						 * Note: as of 1995-January-31 there is
						 * no SQL syntax that supports this, but there
						 * may be GDML
						 */
						add_file(tdbb, 3, work, transaction);
						add_file(tdbb, 4, work, transaction);
						finished = true;
					}
					else
					{
						// We cannot add a file to a shadow that is still
						// in the process of being created.
						raiseDatabaseInUseError(false);
					}
					break;
				}
			}

		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_426);
		   }
		}

		if (finished)
			return false;

		// this file is part of a new shadow, so get all files for the shadow
		// in order of the starting page for the file

		// Note that for a multi-file shadow, we have several pieces of
		// work posted (one dfw_add_shadow for each file).  Rather than
		// trying to cancel the other pieces of work we ignore them
		// when they arrive in this routine.

		sequence = 0;
		min_page = 0;
		shadow = NULL;
		handle.reset();
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
			X IN RDB$FILES CROSS
				Y IN RDB$FILES
				OVER RDB$SHADOW_NUMBER
				WITH X.RDB$FILE_NAME EQ expanded_fname.c_str()
				SORTED BY Y.RDB$FILE_START*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_400, sizeof(jrd_400));
		gds__vtov ((const char*) expanded_fname.c_str(), (char*) jrd_401.jrd_402, 256);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 256, (UCHAR*) &jrd_401);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 268, (UCHAR*) &jrd_403);
		   if (!jrd_403.jrd_406) break;
		{
			// for the first file, create a brand new shadow; for secondary
			// files that have a starting page specified, add a file
			if (!sequence)
				SDW_add(tdbb, /*Y.RDB$FILE_NAME*/
					      jrd_403.jrd_404, /*Y.RDB$SHADOW_NUMBER*/
  jrd_403.jrd_409, /*Y.RDB$FILE_FLAGS*/
  jrd_403.jrd_408);
			else if (/*Y.RDB$FILE_START*/
				 jrd_403.jrd_405)
			{
				if (!shadow)
				{
					for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
					{
						if ((/*Y.RDB$SHADOW_NUMBER*/
						     jrd_403.jrd_409 == shadow->sdw_number) &&
							!(shadow->sdw_flags & SDW_IGNORE))
						{
								break;
						}
					}
				}

				if (!shadow)
					BUGCHECK(203);	// msg 203 shadow block not found for extend file

				min_page = MAX((min_page + 1), (ULONG) /*Y.RDB$FILE_START*/
								       jrd_403.jrd_405);
				add_sequence = SDW_add_file(tdbb, /*Y.RDB$FILE_NAME*/
								  jrd_403.jrd_404, min_page, /*Y.RDB$SHADOW_NUMBER*/
	    jrd_403.jrd_409);
			}

			// update the sequence number and bless the file entry as being good

			if (!sequence || (/*Y.RDB$FILE_START*/
					  jrd_403.jrd_405 && add_sequence))
			{
				/*MODIFY Y*/
				{
				
					/*Y.RDB$FILE_FLAGS*/
					jrd_403.jrd_408 |= FILE_shadow;
					/*Y.RDB$FILE_SEQUENCE*/
					jrd_403.jrd_407 = sequence;
					/*Y.RDB$FILE_START*/
					jrd_403.jrd_405 = min_page;
				/*END_MODIFY*/
				jrd_410.jrd_411 = jrd_403.jrd_405;
				jrd_410.jrd_412 = jrd_403.jrd_408;
				jrd_410.jrd_413 = jrd_403.jrd_407;
				EXE_send (tdbb, handle, 2, 8, (UCHAR*) &jrd_410);
				}
				sequence++;
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_414);
		   }
		}

		break;
	}

	return false;
}

static bool add_difference(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	a d d _ d i f f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	Add backup difference file to the database
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			BackupManager::StateReadGuard stateGuard(tdbb);
			if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_wrong_backup_state));
			}
			check_filename(work->dfw_name, true);
			dbb->dbb_backup_manager->setDifference(tdbb, work->dfw_name.c_str());
		}
		break;
	}

	return false;
}


static bool delete_difference(thread_db* tdbb, SSHORT phase, DeferredWork*, jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ d i f f e r e n c e
 *
 **************************************
 *
 * Delete backup difference file for database
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			BackupManager::StateReadGuard stateGuard(tdbb);

			if (dbb->dbb_backup_manager->getState() != nbak_state_normal)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_wrong_backup_state));
			}
			dbb->dbb_backup_manager->setDifference(tdbb, NULL);
		}
		break;
	}

	return false;
}

static bool begin_backup(thread_db* tdbb, SSHORT phase, DeferredWork*, jrd_tra*)
{
/**************************************
 *
 *	b e g i n _ b a c k u p
 *
 **************************************
 *
 * Begin backup storing changed pages in difference file
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		dbb->dbb_backup_manager->beginBackup(tdbb);
		break;
	}

	return false;
}

static bool end_backup(thread_db* tdbb, SSHORT phase, DeferredWork*, jrd_tra*)
{
/**************************************
 *
 *	e n d _ b a c k u p
 *
 **************************************
 *
 * End backup and merge difference file if neseccary
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		// End backup normally
		dbb->dbb_backup_manager->endBackup(tdbb, false);
		break;
	}

	return false;
}

static bool db_crypt(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	d b _ c r y p t
 *
 **************************************
 *
 * Encrypt database using plugin dfw_name or decrypt if dfw_name is empty.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		dbb->dbb_crypto_manager->changeCryptState(tdbb, work->dfw_name);
		break;
	}

	return false;
}

static bool set_linger(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	s e t _ l i n g e r
 *
 **************************************
 *
 * Set linger interval in Database block.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
	case 3:
		return true;

	case 4:
		dbb->dbb_linger_seconds = atoi(work->dfw_name.c_str());		// number stored as string
		break;
	}

	return false;
}

static bool clear_mapping(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	c l e a r _ m a p p i n g
 *
 **************************************
 *
 * Clear security names mapping cache
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		clearMap(dbb->dbb_filename.c_str());
		break;
	}

	return false;
}

static bool check_not_null(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	c h e c k _ n o t _ n u l l
 *
 **************************************
 *
 * Scan relation to detect NULLs in fields being changed to NOT NULL.
 *
 **************************************/

	SET_TDBB(tdbb);

	Lock* relationLock = NULL;
	bool releaseRelationLock = false;

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		try
		{
			SortedArray<int>& fields = work->dfw_ids;

			jrd_rel* relation = MET_lookup_relation(tdbb, work->dfw_name);
			if (relation->rel_view_rse || fields.isEmpty())
				break;

			// Protect relation from modification
			relationLock = protect_relation(tdbb, transaction, relation, releaseRelationLock);

			UCharBuffer blr;

			blr.add(blr_version5);
			blr.add(blr_begin);
			blr.add(blr_message);
			blr.add(1);	// message number
			blr.add(fields.getCount() & 0xFF);
			blr.add(fields.getCount() >> 8);

			for (size_t i = 0; i < fields.getCount(); ++i)
			{
				blr.add(blr_short);
				blr.add(0);
			}

			blr.add(blr_for);
			blr.add(blr_stall);
			blr.add(blr_rse);
			blr.add(1);
			blr.add(blr_rid);
			blr.add(relation->rel_id & 0xFF);
			blr.add(relation->rel_id >> 8);
			blr.add(0);	// stream
			blr.add(blr_boolean);

			for (size_t i = 0; i < fields.getCount(); ++i)
			{
				if (i != fields.getCount() - 1)
					blr.add(blr_or);

				blr.add(blr_missing);
				blr.add(blr_fid);
				blr.add(0);	// stream
				blr.add(USHORT(fields[i]) & 0xFF);
				blr.add(USHORT(fields[i]) >> 8);
			}

			blr.add(blr_end);

			blr.add(blr_send);
			blr.add(1);
			blr.add(blr_begin);

			for (size_t i = 0; i < fields.getCount(); ++i)
			{
				blr.add(blr_assignment);

				blr.add(blr_value_if);
				blr.add(blr_missing);
				blr.add(blr_fid);
				blr.add(0);	// stream
				blr.add(USHORT(fields[i]) & 0xFF);
				blr.add(USHORT(fields[i]) >> 8);

				blr.add(blr_literal);
				blr.add(blr_short);
				blr.add(0);
				blr.add(1);
				blr.add(0);

				blr.add(blr_literal);
				blr.add(blr_short);
				blr.add(0);
				blr.add(0);
				blr.add(0);

				blr.add(blr_parameter);
				blr.add(1);	// message number
				blr.add(i & 0xFF);
				blr.add(i >> 8);
			}

			blr.add(blr_end);

			blr.add(blr_send);
			blr.add(1);
			blr.add(blr_begin);

			for (size_t i = 0; i < fields.getCount(); ++i)
			{
				blr.add(blr_assignment);
				blr.add(blr_literal);
				blr.add(blr_short);
				blr.add(0);
				blr.add(0);
				blr.add(0);
				blr.add(blr_parameter);
				blr.add(1);	// message number
				blr.add(i & 0xFF);
				blr.add(i >> 8);
			}

			blr.add(blr_end);
			blr.add(blr_end);
			blr.add(blr_eoc);

			AutoRequest request;
			request.compile(tdbb, blr.begin(), blr.getCount());

			HalfStaticArray<USHORT, 5> hasRecord;

			EXE_start(tdbb, request, transaction);
			EXE_receive(tdbb, request, 1, fields.getCount() * sizeof(USHORT),
				(UCHAR*) hasRecord.getBuffer(fields.getCount()));

			Arg::Gds errs(isc_no_meta_update);
			bool hasError = false;

			for (size_t i = 0; i < fields.getCount(); ++i)
			{
				if (hasRecord[i])
				{
					hasError = true;
					errs << Arg::Gds(isc_cannot_make_not_null) <<
							(*relation->rel_fields)[fields[i]]->fld_name <<
							relation->rel_name;
				}
			}

			if (hasError)
				ERR_post(errs);

			if (relationLock && releaseRelationLock)
				release_protect_lock(tdbb, transaction, relationLock);
		}
		catch (const Exception&)
		{
			if (relationLock && releaseRelationLock)
				release_protect_lock(tdbb, transaction, relationLock);

			throw;
		}

		break;
	}

	return false;
}


// Store RDB$CONTEXT_TYPE in RDB$VIEW_RELATIONS when restoring legacy backup.
static bool store_view_context_type(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_391;	// gds__utility 
   } jrd_390;
   struct {
          SSHORT jrd_388;	// gds__null_flag 
          SSHORT jrd_389;	// RDB$CONTEXT_TYPE 
   } jrd_387;
   struct {
          SSHORT jrd_384;	// gds__utility 
          SSHORT jrd_385;	// gds__null_flag 
          SSHORT jrd_386;	// RDB$CONTEXT_TYPE 
   } jrd_383;
   struct {
          TEXT  jrd_381 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_382;	// RDB$VIEW_CONTEXT 
   } jrd_380;
   struct {
          bid  jrd_397;	// RDB$VIEW_BLR 
          SSHORT jrd_398;	// gds__utility 
          SSHORT jrd_399;	// gds__null_flag 
   } jrd_396;
   struct {
          TEXT  jrd_394 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_395;	// RDB$VIEW_CONTEXT 
   } jrd_393;
	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		{
			// If RDB$PACKAGE_NAME IS NOT NULL or no record is found in RDB$RELATIONS,
			// the context is a procedure;
			ViewContextType vct = VCT_PROCEDURE;

			AutoRequest handle1;
			/*FOR (REQUEST_HANDLE handle1 TRANSACTION_HANDLE transaction)
				VRL IN RDB$VIEW_RELATIONS CROSS
				REL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
					VRL.RDB$VIEW_NAME = work->dfw_name.c_str() AND
					VRL.RDB$VIEW_CONTEXT = work->dfw_id AND
					VRL.RDB$PACKAGE_NAME MISSING*/
			{
			handle1.compile(tdbb, (UCHAR*) jrd_392, sizeof(jrd_392));
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_393.jrd_394, 32);
			jrd_393.jrd_395 = work->dfw_id;
			EXE_start (tdbb, handle1, transaction);
			EXE_send (tdbb, handle1, 0, 34, (UCHAR*) &jrd_393);
			while (1)
			   {
			   EXE_receive (tdbb, handle1, 1, 12, (UCHAR*) &jrd_396);
			   if (!jrd_396.jrd_398) break;
			{
				vct = (/*REL.RDB$VIEW_BLR.NULL*/
				       jrd_396.jrd_399 ? VCT_TABLE : VCT_VIEW);
			}
			/*END_FOR*/
			   }
			}

			AutoRequest handle2;
			/*FOR (REQUEST_HANDLE handle2 TRANSACTION_HANDLE transaction)
				VRL IN RDB$VIEW_RELATIONS WITH
					VRL.RDB$VIEW_NAME = work->dfw_name.c_str() AND
					VRL.RDB$VIEW_CONTEXT = work->dfw_id*/
			{
			handle2.compile(tdbb, (UCHAR*) jrd_379, sizeof(jrd_379));
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_380.jrd_381, 32);
			jrd_380.jrd_382 = work->dfw_id;
			EXE_start (tdbb, handle2, transaction);
			EXE_send (tdbb, handle2, 0, 34, (UCHAR*) &jrd_380);
			while (1)
			   {
			   EXE_receive (tdbb, handle2, 1, 6, (UCHAR*) &jrd_383);
			   if (!jrd_383.jrd_384) break;
			{
				/*MODIFY VRL USING*/
				{
				
					/*VRL.RDB$CONTEXT_TYPE.NULL*/
					jrd_383.jrd_385 = FALSE;
					/*VRL.RDB$CONTEXT_TYPE*/
					jrd_383.jrd_386 = (SSHORT) vct;
				/*END_MODIFY*/
				jrd_387.jrd_388 = jrd_383.jrd_385;
				jrd_387.jrd_389 = jrd_383.jrd_386;
				EXE_send (tdbb, handle2, 2, 4, (UCHAR*) &jrd_387);
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, handle2, 3, 2, (UCHAR*) &jrd_390);
			   }
			}
		}
		break;
	}

	return false;
}


static bool user_management(thread_db* /*tdbb*/, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	u s e r _ m a n a g e m e n t
 *
 **************************************
 *
 * Commit in security database
 *
 **************************************/

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		transaction->getUserManagement()->execute(work->dfw_id);
		return true;

	case 4:
		transaction->getUserManagement()->commit();	// safe to be called multiple times
		break;
	}

	return false;
}


// Drop dependencies of a package header.
static bool drop_package_header(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_package_body, transaction);
		MET_delete_dependencies(tdbb, work->dfw_name, obj_package_header, transaction);
		break;
	}

	return false;
}


// Drop dependencies of a package body.
static bool drop_package_body(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_package_body, transaction);
		break;
	}

	return false;
}


static bool grant_privileges(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	g r a n t _ p r i v i l e g e s
 *
 **************************************
 *
 * Functional description
 *	Compute access control list from SQL privileges.
 *
 **************************************/
	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		GRANT_privileges(tdbb, work->dfw_name, work->dfw_id, transaction);
		break;

	default:
		break;
	}

	return false;
}


static bool create_expression_index(thread_db* tdbb, SSHORT phase, DeferredWork* work,
	jrd_tra* transaction)
{
/**************************************
 *
 *	c r e a t e  _ e x p r e s s i o n _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create a new expression index.
 *
 **************************************/
	switch (phase)
	{
	case 0:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_expression_index, transaction);
		return false;

	case 1:
	case 2:
		return true;

	case 3:
		PCMET_expression_index(tdbb, work->dfw_name, work->dfw_id, transaction);
		break;

	default:
		break;
	}

	return false;
}


static void check_computed_dependencies(thread_db* tdbb, jrd_tra* transaction,
	const Firebird::MetaName& fieldName)
{
   struct {
          TEXT  jrd_377 [32];	// RDB$FIELD_SOURCE 
          SSHORT jrd_378;	// gds__utility 
   } jrd_376;
   struct {
          TEXT  jrd_373 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_374;	// RDB$DEPENDED_ON_TYPE 
          SSHORT jrd_375;	// RDB$DEPENDENT_TYPE 
   } jrd_372;
/**************************************
 *
 *	c h e c k _ c o m p u t e d _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Checks if a computed field has circular dependencies.
 *
 **************************************/
	SET_TDBB(tdbb);

	bool err = false;
	Firebird::SortedObjectsArray<Firebird::MetaName> sortedNames(*tdbb->getDefaultPool());
	Firebird::ObjectsArray<Firebird::MetaName> names;

	sortedNames.add(fieldName);
	names.add(fieldName);

	for (size_t pos = 0; !err && pos < names.getCount(); ++pos)
	{
		AutoCacheRequest request(tdbb, irq_comp_circ_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			DEP IN RDB$DEPENDENCIES CROSS
			RFL IN RDB$RELATION_FIELDS WITH
				DEP.RDB$DEPENDENT_NAME EQ names[pos].c_str() AND
				DEP.RDB$DEPENDENT_TYPE EQ obj_computed AND
				DEP.RDB$DEPENDED_ON_TYPE = obj_relation AND
				RFL.RDB$RELATION_NAME = DEP.RDB$DEPENDED_ON_NAME AND
				RFL.RDB$FIELD_NAME = DEP.RDB$FIELD_NAME*/
		{
		request.compile(tdbb, (UCHAR*) jrd_371, sizeof(jrd_371));
		gds__vtov ((const char*) names[pos].c_str(), (char*) jrd_372.jrd_373, 32);
		jrd_372.jrd_374 = obj_relation;
		jrd_372.jrd_375 = obj_computed;
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 36, (UCHAR*) &jrd_372);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_376);
		   if (!jrd_376.jrd_378) break;
		{
			Firebird::MetaName fieldSource(/*RFL.RDB$FIELD_SOURCE*/
						       jrd_376.jrd_377);

			if (fieldName == fieldSource)
			{
				err = true;
				break;
			}

			if (!sortedNames.exist(fieldSource))
			{
				sortedNames.add(fieldSource);
				names.add(fieldSource);
			}
		}
		/*END_FOR*/
		   }
		}
	}

	if (err)
	{
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_circular_computed));
	}
}


static void check_dependencies(thread_db* tdbb,
							   const TEXT* dpdo_name,
							   const TEXT* field_name,
							   const TEXT* package_name,
							   int dpdo_type,
							   jrd_tra* transaction)
{
   struct {
          TEXT  jrd_358 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_359;	// gds__utility 
          SSHORT jrd_360;	// RDB$DEPENDENT_TYPE 
   } jrd_357;
   struct {
          TEXT  jrd_354 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_355 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_356;	// RDB$DEPENDED_ON_TYPE 
   } jrd_353;
   struct {
          TEXT  jrd_368 [32];	// RDB$DEPENDENT_NAME 
          SSHORT jrd_369;	// gds__utility 
          SSHORT jrd_370;	// RDB$DEPENDENT_TYPE 
   } jrd_367;
   struct {
          TEXT  jrd_363 [32];	// RDB$PACKAGE_NAME 
          TEXT  jrd_364 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_365 [32];	// RDB$DEPENDED_ON_NAME 
          SSHORT jrd_366;	// RDB$DEPENDED_ON_TYPE 
   } jrd_362;
/**************************************
 *
 *	c h e c k _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Check the dependency list for relation or relation.field
 *	before deleting such.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	const MetaName packageName(package_name);

	SLONG dep_counts[obj_type_MAX];
	for (int i = 0; i < obj_type_MAX; i++)
		dep_counts[i] = 0;

	if (field_name)
	{
		AutoCacheRequest request(tdbb, irq_ch_f_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES
				WITH DEP.RDB$DEPENDED_ON_NAME EQ dpdo_name
				AND DEP.RDB$DEPENDED_ON_TYPE = dpdo_type
				AND DEP.RDB$FIELD_NAME EQ field_name
				AND DEP.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')
				REDUCED TO DEP.RDB$DEPENDENT_NAME*/
		{
		request.compile(tdbb, (UCHAR*) jrd_361, sizeof(jrd_361));
		gds__vtov ((const char*) packageName.c_str(), (char*) jrd_362.jrd_363, 32);
		gds__vtov ((const char*) field_name, (char*) jrd_362.jrd_364, 32);
		gds__vtov ((const char*) dpdo_name, (char*) jrd_362.jrd_365, 32);
		jrd_362.jrd_366 = dpdo_type;
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 98, (UCHAR*) &jrd_362);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_367);
		   if (!jrd_367.jrd_369) break;
		{
			// If the found object is also being deleted, there's no dependency

			if (!find_depend_in_dfw(tdbb, /*DEP.RDB$DEPENDENT_NAME*/
						      jrd_367.jrd_368, /*DEP.RDB$DEPENDENT_TYPE*/
  jrd_367.jrd_370,
									0, transaction))
			{
				++dep_counts[/*DEP.RDB$DEPENDENT_TYPE*/
					     jrd_367.jrd_370];
			}
		}
		/*END_FOR*/
		   }
		}
	}
	else
	{
		AutoCacheRequest request(tdbb, irq_ch_dpd, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			DEP IN RDB$DEPENDENCIES
				WITH DEP.RDB$DEPENDED_ON_NAME EQ dpdo_name
				AND DEP.RDB$DEPENDED_ON_TYPE = dpdo_type
				AND DEP.RDB$PACKAGE_NAME EQUIV NULLIF(packageName.c_str(), '')
				REDUCED TO DEP.RDB$DEPENDENT_NAME*/
		{
		request.compile(tdbb, (UCHAR*) jrd_352, sizeof(jrd_352));
		gds__vtov ((const char*) packageName.c_str(), (char*) jrd_353.jrd_354, 32);
		gds__vtov ((const char*) dpdo_name, (char*) jrd_353.jrd_355, 32);
		jrd_353.jrd_356 = dpdo_type;
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 66, (UCHAR*) &jrd_353);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_357);
		   if (!jrd_357.jrd_359) break;
		{
			// If the found object is also being deleted, there's no dependency

			if (!find_depend_in_dfw(tdbb, /*DEP.RDB$DEPENDENT_NAME*/
						      jrd_357.jrd_358, /*DEP.RDB$DEPENDENT_TYPE*/
  jrd_357.jrd_360,
									0, transaction))
			{
				++dep_counts[/*DEP.RDB$DEPENDENT_TYPE*/
					     jrd_357.jrd_360];
			}
		}
		/*END_FOR*/
		   }
		}
	}

	SLONG total = 0;
	for (int i = 0; i < obj_type_MAX; i++)
		total += dep_counts[i];

	if (!total)
		return;

	if (field_name)
	{
		string fld_name(dpdo_name);
		fld_name.append(".");
		fld_name.append(field_name);

		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_no_delete) <<						// Msg353: can not delete
				 Arg::Gds(isc_field_name) << Arg::Str(fld_name) <<
				 Arg::Gds(isc_dependency) << Arg::Num(total));	// Msg310: there are %ld dependencies
	}
	else
	{
		const ISC_STATUS obj_type = getErrorCodeByObjectType(dpdo_type);

		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_no_delete) <<						// can not delete
				 Arg::Gds(obj_type) << Arg::Str(QualifiedName(dpdo_name, packageName).toString()) <<
				 Arg::Gds(isc_dependency) << Arg::Num(total));	// there are %ld dependencies
	}
}


static void check_filename(const Firebird::string& name, bool shareExpand)
{
/**************************************
 *
 *	c h e c k _ f i l e n a m e
 *
 **************************************
 *
 * Functional description
 *	Make sure that a file path doesn't contain an
 *	inet node name.
 *
 **************************************/
	const Firebird::PathName file_name(name.ToPathName());
	const bool valid = file_name.find("::") == Firebird::PathName::npos;

	if (!valid || ISC_check_if_remote(file_name, shareExpand)) {
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_node_name_err));
		// Msg305: A node name is not permitted in a secondary, shadow, or log file name
	}

	if (!JRD_verify_database_access(file_name)) {
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("additional database file") <<
													 Arg::Str(name));
	}
}


static bool formatsAreEqual(const Format* old_format, const Format* new_format)
{
/**************************************
 *
 * Functional description
 *	Compare two format blocks
 *
 **************************************/

	if ((old_format->fmt_length != new_format->fmt_length) ||
		(old_format->fmt_count != new_format->fmt_count))
	{
		return false;
	}

	Format::fmt_desc_const_iterator old_desc = old_format->fmt_desc.begin();
	const Format::fmt_desc_const_iterator old_end = old_format->fmt_desc.end();

	Format::fmt_desc_const_iterator new_desc = new_format->fmt_desc.begin();

	while (old_desc != old_end)
	{
		if ((old_desc->dsc_dtype != new_desc->dsc_dtype) ||
			(old_desc->dsc_scale != new_desc->dsc_scale) ||
			(old_desc->dsc_length != new_desc->dsc_length) ||
			(old_desc->dsc_sub_type != new_desc->dsc_sub_type) ||
			(old_desc->dsc_flags != new_desc->dsc_flags) ||
			(old_desc->dsc_address != new_desc->dsc_address))
		{
			return false;
		}

		++new_desc;
		++old_desc;
	}

	return true;
}


static bool compute_security(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
   struct {
          SSHORT jrd_351;	// gds__utility 
   } jrd_350;
   struct {
          TEXT  jrd_349 [32];	// RDB$SECURITY_CLASS 
   } jrd_348;
/**************************************
 *
 *	c o m p u t e _ s e c u r i t y
 *
 **************************************
 *
 * Functional description
 *	There was a change in a security class.  Recompute everything
 *	it touches.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			// Get security class.  This may return NULL if it doesn't exist

			SecurityClass* s_class = SCL_recompute_class(tdbb, work->dfw_name.c_str());

			AutoRequest handle;
			/*FOR(REQUEST_HANDLE handle) X IN RDB$DATABASE
				WITH X.RDB$SECURITY_CLASS EQ work->dfw_name.c_str()*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_347, sizeof(jrd_347));
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_348.jrd_349, 32);
			EXE_start (tdbb, handle, attachment->getSysTransaction());
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_348);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_350);
			   if (!jrd_350.jrd_351) break;
			{
				attachment->att_security_class = s_class;
			}
			/*END_FOR*/
			   }
			}
		}
		break;
	}

	return false;
}


static bool modify_index(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	m o d i f y _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create\drop an index or change the state of an index between active/inactive.
 *	If index owns by global temporary table with on commit preserve rows scope
 *	change index instance for this temporary table too. For "create index" work
 *	item create base index instance before temp index instance. For index
 *	deletion delete temp index instance first to release index usage counter
 *	before deletion of base index instance.
 **************************************/

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = transaction->getAttachment();

	bool is_create = true;
	dfw_task_routine task_routine = NULL;

	switch (work->dfw_type)
	{
		case dfw_create_index :
			task_routine = create_index;
			break;

		case dfw_create_expression_index :
			task_routine = create_expression_index;
			break;

		case dfw_delete_index :
		case dfw_delete_expression_index :
			task_routine = delete_index;
			is_create = false;
			break;
	}
	fb_assert(task_routine);

	bool more = false, more2 = false;

	if (is_create) {
		more = (*task_routine)(tdbb, phase, work, transaction);
	}

	bool gtt_preserve = false;
	jrd_rel* relation = NULL;

	if (is_create)
	{
		PreparedStatement::Builder sql;
		SLONG rdbRelationID;
		SLONG rdbRelationType;
		sql << "select"
			<< sql("rel.rdb$relation_id,", rdbRelationID)
			<< sql("rel.rdb$relation_type", rdbRelationType)
			<< "from rdb$indices idx join rdb$relations rel using (rdb$relation_name)"
			<< "where idx.rdb$index_name = " << work->dfw_name
			<< "  and rel.rdb$relation_id is not null";
		AutoPreparedStatement ps(attachment->prepareStatement(tdbb,
			attachment->getSysTransaction(), sql));
		AutoResultSet rs(ps->executeQuery(tdbb, attachment->getSysTransaction()));

		while (rs->fetch(tdbb))
		{
			gtt_preserve = (rdbRelationType == rel_global_temp_preserve);
			relation = MET_lookup_relation_id(tdbb, rdbRelationID, false);
		}
	}
	else if (work->dfw_id > 0)
	{
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		gtt_preserve = (relation) && (relation->rel_flags & REL_temp_conn);
	}

	if (gtt_preserve && relation)
	{
		tdbb->tdbb_flags &= ~TDBB_use_db_page_space;
		try {
			if (relation->getPages(tdbb, MAX_TRA_NUMBER, false)) {
				more2 = (*task_routine) (tdbb, phase, work, transaction);
			}
			tdbb->tdbb_flags |= TDBB_use_db_page_space;
		}
		catch (...)
		{
			tdbb->tdbb_flags |= TDBB_use_db_page_space;
			throw;
		}
	}

	if (!is_create) {
		more = (*task_routine)(tdbb, phase, work, transaction);
	}

	return (more || more2);
}


static bool create_index(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_293;	// gds__utility 
   } jrd_292;
   struct {
          TEXT  jrd_290 [32];	// RDB$INDEX_NAME 
          TEXT  jrd_291 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_289;
   struct {
          SSHORT jrd_305;	// gds__utility 
   } jrd_304;
   struct {
          SSHORT jrd_302;	// gds__null_flag 
          SSHORT jrd_303;	// RDB$INDEX_ID 
   } jrd_301;
   struct {
          SSHORT jrd_298;	// gds__utility 
          SSHORT jrd_299;	// gds__null_flag 
          SSHORT jrd_300;	// RDB$INDEX_ID 
   } jrd_297;
   struct {
          TEXT  jrd_296 [32];	// RDB$INDEX_NAME 
   } jrd_295;
   struct {
          TEXT  jrd_310 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_311 [32];	// RDB$FOREIGN_KEY 
          double  jrd_312;	// RDB$STATISTICS 
          SSHORT jrd_313;	// gds__utility 
          SSHORT jrd_314;	// gds__null_flag 
          SSHORT jrd_315;	// RDB$COLLATION_ID 
          SSHORT jrd_316;	// gds__null_flag 
          SSHORT jrd_317;	// RDB$COLLATION_ID 
          SSHORT jrd_318;	// gds__null_flag 
          SSHORT jrd_319;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_320;	// RDB$FIELD_ID 
          SSHORT jrd_321;	// gds__null_flag 
          SSHORT jrd_322;	// RDB$DIMENSIONS 
          SSHORT jrd_323;	// RDB$FIELD_TYPE 
          SSHORT jrd_324;	// RDB$FIELD_POSITION 
          SSHORT jrd_325;	// gds__null_flag 
          SSHORT jrd_326;	// RDB$INDEX_TYPE 
          SSHORT jrd_327;	// RDB$UNIQUE_FLAG 
          SSHORT jrd_328;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_329;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_330;	// RDB$INDEX_ID 
          SSHORT jrd_331;	// RDB$RELATION_ID 
   } jrd_309;
   struct {
          TEXT  jrd_308 [32];	// RDB$INDEX_NAME 
   } jrd_307;
   struct {
          SSHORT jrd_346;	// gds__utility 
   } jrd_345;
   struct {
          SSHORT jrd_343;	// gds__null_flag 
          SSHORT jrd_344;	// RDB$INDEX_ID 
   } jrd_342;
   struct {
          TEXT  jrd_336 [32];	// RDB$RELATION_NAME 
          bid  jrd_337;	// RDB$VIEW_BLR 
          SSHORT jrd_338;	// gds__utility 
          SSHORT jrd_339;	// gds__null_flag 
          SSHORT jrd_340;	// RDB$INDEX_ID 
          SSHORT jrd_341;	// gds__null_flag 
   } jrd_335;
   struct {
          TEXT  jrd_334 [32];	// RDB$INDEX_NAME 
   } jrd_333;
/**************************************
 *
 *	c r e a t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create a new index or change the state of an index between active/inactive.
 *
 **************************************/
	AutoCacheRequest request;
	jrd_rel* relation;
	jrd_rel* partner_relation;
	index_desc idx;
	int key_count;
	AutoRequest handle;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		handle.reset();

		// Drop those indices at clean up time.
		/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction) IDXN IN RDB$INDICES CROSS
			IREL IN RDB$RELATIONS OVER RDB$RELATION_NAME
				WITH IDXN.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_332, sizeof(jrd_332));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_333.jrd_334, 32);
		EXE_start (tdbb, handle, transaction);
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_333);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 48, (UCHAR*) &jrd_335);
		   if (!jrd_335.jrd_338) break;
		{
			// Views do not have indices
			if (/*IREL.RDB$VIEW_BLR.NULL*/
			    jrd_335.jrd_341)
			{
				relation = MET_lookup_relation(tdbb, /*IDXN.RDB$RELATION_NAME*/
								     jrd_335.jrd_336);

				RelationPages* relPages = relation->getPages(tdbb, MAX_TRA_NUMBER, false);
				if (!relPages) {
					return false;
				}

				// we need to special handle temp tables with ON PRESERVE ROWS only
				const bool isTempIndex = (relation->rel_flags & REL_temp_conn) &&
					(relPages->rel_instance_id != 0);

				// Fetch the root index page and mark MUST_WRITE, and then
				// delete the index. It will also clean the index slot. Note
				// that the previous fixed definition of MAX_IDX (64) has been
				// dropped in favor of a computed value saved in the Database
				if (relPages->rel_index_root)
				{
					if (work->dfw_id != dbb->dbb_max_idx)
					{
						WIN window(relPages->rel_pg_space_id, relPages->rel_index_root);
						CCH_FETCH(tdbb, &window, LCK_write, pag_root);
						CCH_MARK_MUST_WRITE(tdbb, &window);
						const bool tree_exists = BTR_delete_index(tdbb, &window, work->dfw_id);

						if (!isTempIndex) {
							work->dfw_id = dbb->dbb_max_idx;
						}
						else if (tree_exists)
						{
							IndexLock* idx_lock = CMP_get_index_lock(tdbb, relation, work->dfw_id);
							if (idx_lock)
							{
								if (!--idx_lock->idl_count) {
									LCK_release(tdbb, idx_lock->idl_lock);
								}
							}
						}
					}
					if (!/*IDXN.RDB$INDEX_ID.NULL*/
					     jrd_335.jrd_339)
					{
						/*MODIFY IDXN USING*/
						{
						
							/*IDXN.RDB$INDEX_ID.NULL*/
							jrd_335.jrd_339 = TRUE;
						/*END_MODIFY*/
						jrd_342.jrd_343 = jrd_335.jrd_339;
						jrd_342.jrd_344 = jrd_335.jrd_340;
						EXE_send (tdbb, handle, 2, 4, (UCHAR*) &jrd_342);
						}
					}
				}
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_345);
		   }
		}

		return false;

	case 1:
	case 2:
		return true;

	case 3:
		key_count = 0;
		relation = NULL;
		idx.idx_flags = 0;

		// Here we need dirty reads from database (first of all from
		// RDB$RELATION_FIELDS and RDB$FIELDS - tables not directly related
		// with index to be created and it's dfw_name. Missing it breaks gbak,
		// and appears can break other applications. Modification of
		// record in RDB$INDICES was moved into separate request. AP-2008.

		// Fetch the information necessary to create the index.  On the first
		// time thru, check to see if the index already exists.  If so, delete
		// it.  If the index inactive flag is set, don't create the index

		request.reset(tdbb, irq_c_index, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			IDX IN RDB$INDICES CROSS
				SEG IN RDB$INDEX_SEGMENTS OVER RDB$INDEX_NAME CROSS
				RFR IN RDB$RELATION_FIELDS OVER RDB$FIELD_NAME,
				RDB$RELATION_NAME CROSS
				FLD IN RDB$FIELDS CROSS
				REL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
					FLD.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE AND
					IDX.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_306, sizeof(jrd_306));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_307.jrd_308, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_307);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 110, (UCHAR*) &jrd_309);
		   if (!jrd_309.jrd_313) break;
		{
			if (!relation)
			{
				relation = MET_lookup_relation_id(tdbb, /*REL.RDB$RELATION_ID*/
									jrd_309.jrd_331, false);
				if (!relation)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
					// Msg308: can't create index %s
				}
				if (/*IDX.RDB$INDEX_ID*/
				    jrd_309.jrd_330 && /*IDX.RDB$STATISTICS*/
    jrd_309.jrd_312 < 0.0)
				{
					// we need to know if this relation is temporary or not
					MET_scan_relation(tdbb, relation);

					// no need to recalculate statistics for base instance of GTT
					RelationPages* relPages = relation->getPages(tdbb, MAX_TRA_NUMBER, false);
					const bool isTempInstance = relation->isTemporary() &&
						relPages && (relPages->rel_instance_id != 0);

					if (isTempInstance || !relation->isTemporary())
					{
						SelectivityList selectivity(*tdbb->getDefaultPool());
						const USHORT id = /*IDX.RDB$INDEX_ID*/
								  jrd_309.jrd_330 - 1;
						IDX_statistics(tdbb, relation, id, selectivity);
						DFW_update_index(work->dfw_name.c_str(), id, selectivity, transaction);
					}

					return false;
				}

				if (/*IDX.RDB$INDEX_ID*/
				    jrd_309.jrd_330)
				{
					IDX_delete_index(tdbb, relation, (USHORT)(/*IDX.RDB$INDEX_ID*/
										  jrd_309.jrd_330 - 1));

					AutoCacheRequest request2(tdbb, irq_c_index_m, IRQ_REQUESTS);

			        /*FOR(REQUEST_HANDLE request2 TRANSACTION_HANDLE transaction)
						IDXM IN RDB$INDICES WITH IDXM.RDB$INDEX_NAME EQ work->dfw_name.c_str()*/
				{
				request2.compile(tdbb, (UCHAR*) jrd_294, sizeof(jrd_294));
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_295.jrd_296, 32);
				EXE_start (tdbb, request2, transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_295);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 6, (UCHAR*) &jrd_297);
				   if (!jrd_297.jrd_298) break;
					{
						/*MODIFY IDXM*/
						{
						
							/*IDXM.RDB$INDEX_ID.NULL*/
							jrd_297.jrd_299 = TRUE;
						/*END_MODIFY*/
						jrd_301.jrd_302 = jrd_297.jrd_299;
						jrd_301.jrd_303 = jrd_297.jrd_300;
						EXE_send (tdbb, request2, 2, 4, (UCHAR*) &jrd_301);
						}
					}
					/*END_FOR*/
					   EXE_send (tdbb, request2, 3, 2, (UCHAR*) &jrd_304);
					   }
					}
				}

				if (/*IDX.RDB$INDEX_INACTIVE*/
				    jrd_309.jrd_329)
					return false;

				idx.idx_count = /*IDX.RDB$SEGMENT_COUNT*/
						jrd_309.jrd_328;

				if (!idx.idx_count || idx.idx_count > MAX_INDEX_SEGMENTS)
				{
					if (!idx.idx_count)
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_idx_seg_err) << Arg::Str(work->dfw_name));
						// Msg304: segment count of 0 defined for index %s
					}
					else
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_idx_key_err) << Arg::Str(work->dfw_name));
						// Msg311: too many keys defined for index %s
					}
				}
				if (/*IDX.RDB$UNIQUE_FLAG*/
				    jrd_309.jrd_327)
					idx.idx_flags |= idx_unique;
				if (/*IDX.RDB$INDEX_TYPE*/
				    jrd_309.jrd_326 == 1)
					idx.idx_flags |= idx_descending;
				if (!/*IDX.RDB$FOREIGN_KEY.NULL*/
				     jrd_309.jrd_325)
					idx.idx_flags |= idx_foreign;

				AutoRequest rc_request;

				/*FOR(REQUEST_HANDLE rc_request TRANSACTION_HANDLE transaction)
					RC IN RDB$RELATION_CONSTRAINTS WITH
					RC.RDB$INDEX_NAME EQ work->dfw_name.c_str() AND
					RC.RDB$CONSTRAINT_TYPE = PRIMARY_KEY*/
				{
				rc_request.compile(tdbb, (UCHAR*) jrd_288, sizeof(jrd_288));
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_289.jrd_290, 32);
				gds__vtov ((const char*) PRIMARY_KEY, (char*) jrd_289.jrd_291, 12);
				EXE_start (tdbb, rc_request, transaction);
				EXE_send (tdbb, rc_request, 0, 44, (UCHAR*) &jrd_289);
				while (1)
				   {
				   EXE_receive (tdbb, rc_request, 1, 2, (UCHAR*) &jrd_292);
				   if (!jrd_292.jrd_293) break;
				{
					idx.idx_flags |= idx_primary;
				}
				/*END_FOR*/
				   }
				}
			}

			if (++key_count > idx.idx_count || /*SEG.RDB$FIELD_POSITION*/
							   jrd_309.jrd_324 > idx.idx_count ||
				/*FLD.RDB$FIELD_TYPE*/
				jrd_309.jrd_323 == blr_blob || !/*FLD.RDB$DIMENSIONS.NULL*/
		 jrd_309.jrd_321)
			{
				if (key_count > idx.idx_count)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_idx_key_err) << Arg::Str(work->dfw_name));
					// Msg311: too many keys defined for index %s
				}
				else if (/*SEG.RDB$FIELD_POSITION*/
					 jrd_309.jrd_324 > idx.idx_count)
				{
					fb_utils::exact_name(/*RFR.RDB$FIELD_NAME*/
							     jrd_309.jrd_310);
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_inval_key_posn) <<
							 // Msg358: invalid key position
							 Arg::Gds(isc_field_name) << Arg::Str(/*RFR.RDB$FIELD_NAME*/
											      jrd_309.jrd_310) <<
							 Arg::Gds(isc_index_name) << Arg::Str(work->dfw_name));
				}
				else if (/*FLD.RDB$FIELD_TYPE*/
					 jrd_309.jrd_323 == blr_blob)
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_blob_idx_err) << Arg::Str(work->dfw_name));
					// Msg350: attempt to index blob column in index %s
				}
				else
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_array_idx_err) << Arg::Str(work->dfw_name));
					// Msg351: attempt to index array column in index %s
				}
			}

			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_309.jrd_324].idx_field = /*RFR.RDB$FIELD_ID*/
	      jrd_309.jrd_320;

			if (/*FLD.RDB$CHARACTER_SET_ID.NULL*/
			    jrd_309.jrd_318)
				/*FLD.RDB$CHARACTER_SET_ID*/
				jrd_309.jrd_319 = CS_NONE;

			SSHORT collate;
			if (!/*RFR.RDB$COLLATION_ID.NULL*/
			     jrd_309.jrd_316)
				collate = /*RFR.RDB$COLLATION_ID*/
					  jrd_309.jrd_317;
			else if (!/*FLD.RDB$COLLATION_ID.NULL*/
				  jrd_309.jrd_314)
				collate = /*FLD.RDB$COLLATION_ID*/
					  jrd_309.jrd_315;
			else
				collate = COLLATE_NONE;

			const SSHORT text_type = INTL_CS_COLL_TO_TTYPE(/*FLD.RDB$CHARACTER_SET_ID*/
								       jrd_309.jrd_319, collate);
			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_309.jrd_324].idx_itype =
				DFW_assign_index_type(tdbb, work->dfw_name, gds_cvt_blr_dtype[/*FLD.RDB$FIELD_TYPE*/
											      jrd_309.jrd_323], text_type);

			// Initialize selectivity to zero. Otherwise random rubbish makes its way into database
			idx.idx_rpt[/*SEG.RDB$FIELD_POSITION*/
				    jrd_309.jrd_324].idx_selectivity = 0;
		}
		/*END_FOR*/
		   }
		}

		if (key_count != idx.idx_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_key_field_err) << Arg::Str(work->dfw_name));
			// Msg352: too few key columns found for index %s (incorrect column name?)
		}

		if (!relation)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
			// Msg308: can't create index %s
		}

		// Make sure the relation info is all current

		MET_scan_relation(tdbb, relation);

		if (relation->rel_view_rse)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_idx_create_err) << Arg::Str(work->dfw_name));
			// Msg308: can't create index %s
		}

		// Actually create the index

		Lock *relationLock = NULL, *partnerLock = NULL;
		bool releaseRelationLock = false, releasePartnerLock = false;
		partner_relation = NULL;
		try
		{
			// Protect relation from modification to create consistent index
			relationLock = protect_relation(tdbb, transaction, relation, releaseRelationLock);

			if (idx.idx_flags & idx_foreign)
			{
				idx.idx_id = idx_invalid;

				if (MET_lookup_partner(tdbb, relation, &idx, work->dfw_name.c_str()))
				{
					partner_relation = MET_lookup_relation_id(tdbb, idx.idx_primary_relation, true);
				}

				if (!partner_relation)
				{
					Firebird::MetaName constraint_name;
					MET_lookup_cnstrt_for_index(tdbb, constraint_name, work->dfw_name);
					ERR_post(Arg::Gds(isc_partner_idx_not_found) << Arg::Str(constraint_name));
				}

				// Get an protected_read lock on the both relations if the index being
				// defined enforces a foreign key constraint. This will prevent
				// the constraint from being violated during index construction.

				partnerLock = protect_relation(tdbb, transaction, partner_relation, releasePartnerLock);

				int bad_segment;
				if (!IDX_check_master_types(tdbb, idx, partner_relation, bad_segment))
				{
					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_partner_idx_incompat_type) << Arg::Num(bad_segment + 1));
				}

/* hvlad: this code was never called but i preserve it for Claudio review and decision

				// CVC: Currently, the server doesn't enforce FK creation more than at DYN level.
				// If DYN is bypassed, then FK creation succeeds and operation will fail at run-time.
				// The aim is to check REFERENCES at DDL time instead of DML time and behave accordingly
				// to ANSI SQL rules for REFERENCES rights.
				// For testing purposes, I'm calling SCL_check_index, although most of the DFW ops are
				// carried using internal metadata structures that are refreshed from system tables.

				// Don't bother if the master's owner is the same than the detail's owner.
				// If both tables aren't defined in the same session, partner_relation->rel_owner_name
				// won't be loaded hence, we need to be careful about null pointers.

				if (relation->rel_owner_name.length() == 0 ||
					partner_relation->rel_owner_name.length() == 0 ||
					relation->rel_owner_name != partner_relation->rel_owner_name)
				{
					SCL_check_index(tdbb, partner_relation->rel_name,
									idx.idx_id + 1, SCL_references);
				}
*/
			}

			fb_assert(work->dfw_id <= dbb->dbb_max_idx);
			idx.idx_id = work->dfw_id;
			SelectivityList selectivity(*tdbb->getDefaultPool());
			IDX_create_index(tdbb, relation, &idx, work->dfw_name.c_str(),
							&work->dfw_id, transaction, selectivity);
			fb_assert(work->dfw_id == idx.idx_id);
			DFW_update_index(work->dfw_name.c_str(), idx.idx_id, selectivity, transaction);

			if (partner_relation)
			{
				// signal to other processes about new constraint
				relation->rel_flags |= REL_check_partners;
				LCK_lock(tdbb, relation->rel_partners_lock, LCK_EX, LCK_WAIT);
				LCK_release(tdbb, relation->rel_partners_lock);

				if (relation != partner_relation) {
					partner_relation->rel_flags |= REL_check_partners;
					LCK_lock(tdbb, partner_relation->rel_partners_lock, LCK_EX, LCK_WAIT);
					LCK_release(tdbb, partner_relation->rel_partners_lock);
				}
			}
			if (relationLock && releaseRelationLock) {
				release_protect_lock(tdbb, transaction, relationLock);
			}
			if (partnerLock && releasePartnerLock) {
				release_protect_lock(tdbb, transaction, partnerLock);
			}
		}
		catch (const Firebird::Exception&)
		{
			if (relationLock && releaseRelationLock) {
				release_protect_lock(tdbb, transaction, relationLock);
			}
			if (partnerLock && releasePartnerLock) {
				release_protect_lock(tdbb, transaction, partnerLock);
			}
			throw;
		}

		break;
	}

	return false;
}


static bool create_relation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_261;	// gds__utility 
          SSHORT jrd_262;	// RDB$RELATION_ID 
   } jrd_260;
   struct {
          TEXT  jrd_259 [32];	// RDB$RELATION_NAME 
   } jrd_258;
   struct {
          SSHORT jrd_269;	// gds__utility 
          SSHORT jrd_270;	// RDB$DBKEY_LENGTH 
   } jrd_268;
   struct {
          TEXT  jrd_265 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_266;	// RDB$CONTEXT_TYPE 
          SSHORT jrd_267;	// RDB$CONTEXT_TYPE 
   } jrd_264;
   struct {
          SSHORT jrd_287;	// RDB$RELATION_ID 
   } jrd_286;
   struct {
          SSHORT jrd_285;	// gds__utility 
   } jrd_284;
   struct {
          SSHORT jrd_282;	// RDB$RELATION_ID 
          SSHORT jrd_283;	// RDB$DBKEY_LENGTH 
   } jrd_281;
   struct {
          TEXT  jrd_275 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_276;	// RDB$VIEW_BLR 
          SSHORT jrd_277;	// gds__utility 
          SSHORT jrd_278;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_279;	// RDB$RELATION_ID 
          SSHORT jrd_280;	// RDB$RELATION_ID 
   } jrd_274;
   struct {
          TEXT  jrd_273 [32];	// RDB$RELATION_NAME 
   } jrd_272;
/**************************************
 *
 *	c r e a t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Create a new relation.
 *
 **************************************/
	AutoCacheRequest request;
	jrd_rel* relation;
	USHORT rel_id, external_flag;
	bid blob_id;
	AutoRequest handle;
	Lock* lock;

	blob_id.clear();

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	const USHORT local_min_relation_id = USER_DEF_REL_INIT_ID;

	switch (phase)
	{
	case 0:
		if (work->dfw_lock)
		{
			LCK_release(tdbb, work->dfw_lock);
			delete work->dfw_lock;
			work->dfw_lock = NULL;
		}
		break;

	case 1:
	case 2:
		return true;

	case 3:
		// Take a relation lock on rel id -1 before actually generating a relation id.

		work->dfw_lock = lock = FB_NEW_RPT(*tdbb->getDefaultPool(), sizeof(SLONG))
			Lock(tdbb, sizeof(SLONG), LCK_relation);
		lock->lck_key.lck_long = -1;

		LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT);

		/* Assign a relation ID and dbkey length to the new relation.
		   Probe the candidate relation ID returned from the system
		   relation RDB$DATABASE to make sure it isn't already assigned.
		   This can happen from nefarious manipulation of RDB$DATABASE
		   or wraparound of the next relation ID. Keep looking for a
		   usable relation ID until the search space is exhausted. */

		rel_id = 0;
		request.reset(tdbb, irq_c_relation, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
			X IN RDB$DATABASE CROSS Y IN RDB$RELATIONS WITH
				Y.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_271, sizeof(jrd_271));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_272.jrd_273, 32);
		EXE_start (tdbb, request, transaction);
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_272);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 272, (UCHAR*) &jrd_274);
		   if (!jrd_274.jrd_277) break;
		{
			blob_id = /*Y.RDB$VIEW_BLR*/
				  jrd_274.jrd_276;
			external_flag = /*Y.RDB$EXTERNAL_FILE*/
					jrd_274.jrd_275[0];

			/*MODIFY X USING*/
			{
			
				rel_id = /*X.RDB$RELATION_ID*/
					 jrd_274.jrd_280;

				if (rel_id < local_min_relation_id || rel_id > MAX_RELATION_ID)
					rel_id = /*X.RDB$RELATION_ID*/
						 jrd_274.jrd_280 = local_min_relation_id;

				while ( (relation = MET_lookup_relation_id(tdbb, rel_id++, false)) )
				{
					if (rel_id < local_min_relation_id || rel_id > MAX_RELATION_ID)
						rel_id = local_min_relation_id;

					if (rel_id == /*X.RDB$RELATION_ID*/
						      jrd_274.jrd_280)
					{
						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
								 Arg::Gds(isc_imp_exc));
					}
				}

				/*X.RDB$RELATION_ID*/
				jrd_274.jrd_280 = (rel_id > MAX_RELATION_ID) ? local_min_relation_id : rel_id;

				/*MODIFY Y USING*/
				{
				
					/*Y.RDB$RELATION_ID*/
					jrd_274.jrd_279 = --rel_id;
					if (blob_id.isEmpty())
						/*Y.RDB$DBKEY_LENGTH*/
						jrd_274.jrd_278 = 8;
					else
					{
						// update the dbkey length to include each of the base relations
						/*Y.RDB$DBKEY_LENGTH*/
						jrd_274.jrd_278 = 0;

						handle.reset();

						/*FOR(REQUEST_HANDLE handle)
							Z IN RDB$VIEW_RELATIONS CROSS
							R IN RDB$RELATIONS OVER RDB$RELATION_NAME
							WITH Z.RDB$VIEW_NAME = work->dfw_name.c_str() AND
								 (Z.RDB$CONTEXT_TYPE = VCT_TABLE OR
								  Z.RDB$CONTEXT_TYPE = VCT_VIEW)*/
						{
						handle.compile(tdbb, (UCHAR*) jrd_263, sizeof(jrd_263));
						gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_264.jrd_265, 32);
						jrd_264.jrd_266 = VCT_VIEW;
						jrd_264.jrd_267 = VCT_TABLE;
						EXE_start (tdbb, handle, attachment->getSysTransaction());
						EXE_send (tdbb, handle, 0, 36, (UCHAR*) &jrd_264);
						while (1)
						   {
						   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_268);
						   if (!jrd_268.jrd_269) break;
						{
							/*Y.RDB$DBKEY_LENGTH*/
							jrd_274.jrd_278 += /*R.RDB$DBKEY_LENGTH*/
    jrd_268.jrd_270;
						}
						/*END_FOR*/
						   }
						}
					}
				/*END_MODIFY*/
				jrd_281.jrd_282 = jrd_274.jrd_279;
				jrd_281.jrd_283 = jrd_274.jrd_278;
				EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_281);
				}
			/*END_MODIFY*/
			jrd_286.jrd_287 = jrd_274.jrd_280;
			EXE_send (tdbb, request, 4, 2, (UCHAR*) &jrd_286);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_284);
		   }
		}

		LCK_release(tdbb, lock);
		delete lock;
		work->dfw_lock = NULL;

		// if this is not a view, create the relation

		if (rel_id && blob_id.isEmpty() && !external_flag)
		{
			relation = MET_relation(tdbb, rel_id);
			DPM_create_relation(tdbb, relation);
		}

		return true;

	case 4:

		// get the relation and flag it to check for dependencies
		// in the view blr (if it exists) and any computed fields

		request.reset(tdbb, irq_c_relation2, IRQ_REQUESTS);

		/*FOR(REQUEST_HANDLE request)
			X IN RDB$RELATIONS WITH
				X.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_257, sizeof(jrd_257));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_258.jrd_259, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_258);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_260);
		   if (!jrd_260.jrd_261) break;
		{
			rel_id = /*X.RDB$RELATION_ID*/
				 jrd_260.jrd_262;
			relation = MET_relation(tdbb, rel_id);
			relation->rel_flags |= REL_get_dependencies;

			relation->rel_flags &= ~REL_scanned;
			DFW_post_work(transaction, dfw_scan_relation, NULL, rel_id);
		}
		/*END_FOR*/
		   }
		}

		break;
	}

	return false;
}


static bool create_trigger(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	c r e a t e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Perform required actions on creation of trigger.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			const bool compile = !work->findArg(dfw_arg_check_blr);
			get_trigger_dependencies(work, compile, transaction);
			return true;
		}

	case 4:
		{
			if (!work->findArg(dfw_arg_rel_name))
			{
				const DeferredWork* const arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				if (arg)
				{
					// ASF: arg->dfw_id is RDB$TRIGGER_TYPE truncated to USHORT
					if ((arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
					{
						MET_load_trigger(tdbb, NULL, work->dfw_name,
							&tdbb->getAttachment()->att_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB]);
					}
					else if ((arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DDL)
					{
						MET_load_trigger(tdbb, NULL, work->dfw_name,
							&tdbb->getAttachment()->att_ddl_triggers);
					}
				}
			}
		}
		break;
	}

	return false;
}


static bool create_collation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_256;	// gds__utility 
   } jrd_255;
   struct {
          bid  jrd_253;	// RDB$SPECIFIC_ATTRIBUTES 
          SSHORT jrd_254;	// gds__null_flag 
   } jrd_252;
   struct {
          TEXT  jrd_244 [32];	// RDB$CHARACTER_SET_NAME 
          TEXT  jrd_245 [32];	// RDB$COLLATION_NAME 
          TEXT  jrd_246 [32];	// RDB$BASE_COLLATION_NAME 
          bid  jrd_247;	// RDB$SPECIFIC_ATTRIBUTES 
          SSHORT jrd_248;	// gds__utility 
          SSHORT jrd_249;	// gds__null_flag 
          SSHORT jrd_250;	// gds__null_flag 
          SSHORT jrd_251;	// RDB$SYSTEM_FLAG 
   } jrd_243;
   struct {
          TEXT  jrd_241 [32];	// RDB$COLLATION_NAME 
          SSHORT jrd_242;	// RDB$CHARACTER_SET_ID 
   } jrd_240;
/**************************************
 *
 *	c r e a t e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Get collation id or setup specific attributes.
 *
 **************************************/

	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
		{
			const USHORT charSetId = TTYPE_TO_CHARSET(work->dfw_id);
			AutoRequest handle;

			/*FOR(REQUEST_HANDLE handle TRANSACTION_HANDLE transaction)
				COLL IN RDB$COLLATIONS
				CROSS CS IN RDB$CHARACTER_SETS
					OVER RDB$CHARACTER_SET_ID
				WITH COLL.RDB$COLLATION_NAME EQ work->dfw_name.c_str() AND
					 COLL.RDB$CHARACTER_SET_ID EQ charSetId*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_239, sizeof(jrd_239));
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_240.jrd_241, 32);
			jrd_240.jrd_242 = charSetId;
			EXE_start (tdbb, handle, transaction);
			EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_240);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 112, (UCHAR*) &jrd_243);
			   if (!jrd_243.jrd_248) break;
			{
				if (/*COLL.RDB$SYSTEM_FLAG*/
				    jrd_243.jrd_251 != 0)
				{
					SLONG length = 0;
					HalfStaticArray<UCHAR, BUFFER_SMALL> buffer;

					if (!/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
					     jrd_243.jrd_250)
					{
						blb* blob = blb::open(tdbb, transaction, &/*COLL.RDB$SPECIFIC_ATTRIBUTES*/
											  jrd_243.jrd_247);
						length = blob->blb_length + 10;
						length = blob->BLB_get_data(tdbb, buffer.getBuffer(length), length);
					}

					const string specificAttributes((const char*) buffer.begin(), length);
					string newSpecificAttributes;

					// ASF: If setupCollationAttributes fail we store the original
					// attributes. This should be what we want for new databases
					// and restores. CREATE COLLATION will fail in DYN.
					if (IntlManager::setupCollationAttributes(
							fb_utils::exact_name(/*COLL.RDB$BASE_COLLATION_NAME.NULL*/
									     jrd_243.jrd_249 ?
								/*COLL.RDB$COLLATION_NAME*/
								jrd_243.jrd_245 : /*COLL.RDB$BASE_COLLATION_NAME*/
   jrd_243.jrd_246),
							fb_utils::exact_name(/*CS.RDB$CHARACTER_SET_NAME*/
									     jrd_243.jrd_244),
							specificAttributes, newSpecificAttributes) &&
						newSpecificAttributes != specificAttributes) // if nothing changed, we do nothing
					{
						/*MODIFY COLL USING*/
						{
						
							if (newSpecificAttributes.isEmpty())
								/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
								jrd_243.jrd_250 = TRUE;
							else
							{
								/*COLL.RDB$SPECIFIC_ATTRIBUTES.NULL*/
								jrd_243.jrd_250 = FALSE;
								attachment->storeMetaDataBlob(tdbb, transaction,
									&/*COLL.RDB$SPECIFIC_ATTRIBUTES*/
									 jrd_243.jrd_247, newSpecificAttributes);
							}
						/*END_MODIFY*/
						jrd_252.jrd_253 = jrd_243.jrd_247;
						jrd_252.jrd_254 = jrd_243.jrd_250;
						EXE_send (tdbb, handle, 2, 10, (UCHAR*) &jrd_252);
						}
					}
				}
			}
			/*END_FOR*/
			   EXE_send (tdbb, handle, 3, 2, (UCHAR*) &jrd_255);
			   }
			}
		}

		break;
	}

	return false;
}


static bool delete_collation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ c o l l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a collation, and if so, clean up after it.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, NULL, obj_collation, transaction);
		return true;

	case 2:
		return true;

	case 3:
		INTL_texttype_unload(tdbb, work->dfw_id);
		break;
	}

	return false;
}


static bool delete_exception(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ e x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	an exception, and if so, clean up after it.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, NULL, obj_exception, transaction);
		break;
	}

	return false;
}


static bool set_generator(thread_db* tdbb,
						  SSHORT phase,
						  DeferredWork* work,
						  jrd_tra* transaction)
{
/**************************************
 *
 *	s e t _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Set the generator to the given value.
 *
 **************************************/
	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			const SLONG id = MET_lookup_generator(tdbb, work->dfw_name);
			if (id >= 0)
			{
				fb_assert(id == work->dfw_id);
				SINT64 value = 0;
				if (transaction->getGenIdCache()->get(id, value))
				{
					transaction->getGenIdCache()->remove(id);
					DPM_gen_id(tdbb, id, true, value);
				}
			}
#ifdef DEV_BUILD
			else // This is a test only
				status_exception::raise(Arg::Gds(isc_cant_modify_sysobj) << "generator" << work->dfw_name);
#endif
		}
		break;
	}

	return false;
}


static bool delete_generator(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ g e n e r a t o r
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a generator, and if so, clean up after it.
 * CVC: This function was modelled after delete_exception.
 *
 **************************************/

	SET_TDBB(tdbb);
	const char* gen_name = work->dfw_name.c_str();

	switch (phase)
	{
	case 1:
		check_dependencies(tdbb, gen_name, NULL, NULL, obj_generator, transaction);
		break;
	}

	return false;
}


static bool create_field(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          bid  jrd_236;	// RDB$VALIDATION_BLR 
          SSHORT jrd_237;	// gds__utility 
          SSHORT jrd_238;	// gds__null_flag 
   } jrd_235;
   struct {
          TEXT  jrd_234 [32];	// RDB$FIELD_NAME 
   } jrd_233;
/**************************************
 *
 *	c r e a t e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Store dependencies of a field.
 *
 **************************************/

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
		{
			const Firebird::MetaName depName(work->dfw_name);
			AutoRequest handle;
			bid validation;
			validation.clear();

			/*FOR(REQUEST_HANDLE handle)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ depName.c_str()*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_232, sizeof(jrd_232));
			gds__vtov ((const char*) depName.c_str(), (char*) jrd_233.jrd_234, 32);
			EXE_start (tdbb, handle, attachment->getSysTransaction());
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_233);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_235);
			   if (!jrd_235.jrd_237) break;
			{
				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_235.jrd_238)
					validation = /*FLD.RDB$VALIDATION_BLR*/
						     jrd_235.jrd_236;
			}
			/*END_FOR*/
			   }
			}

			if (!validation.isEmpty())
			{
				MemoryPool* new_pool = attachment->createPool();
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &validation,
					NULL, NULL, depName, obj_validation, 0, transaction, depName);

				attachment->deletePool(new_pool);
			}
		}
		// fall through

	case 2:
	case 3:
		return true;

	case 4:	// after scan_relation (phase 3)
		check_computed_dependencies(tdbb, transaction, work->dfw_name);
		break;
	}

	return false;
}


static bool delete_field(thread_db*	tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_229 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_230;	// gds__utility 
          SSHORT jrd_231;	// RDB$RELATION_ID 
   } jrd_228;
   struct {
          TEXT  jrd_227 [32];	// RDB$FIELD_SOURCE 
   } jrd_226;
/**************************************
 *
 *	d e l e t e _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	This whole routine exists just to
 *	return an error if someone attempts to
 *	delete a global field that is in use
 *
 **************************************/

	int field_count;
	AutoRequest handle;

	SET_TDBB(tdbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
		// Look up the field in RFR.  If we can't find the field, go ahead with the delete.

		handle.reset();
		field_count = 0;

		/*FOR(REQUEST_HANDLE handle)
			RFR IN RDB$RELATION_FIELDS CROSS
				REL IN RDB$RELATIONS
				OVER RDB$RELATION_NAME
				WITH RFR.RDB$FIELD_SOURCE EQ work->dfw_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_225, sizeof(jrd_225));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_226.jrd_227, 32);
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_226);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_228);
		   if (!jrd_228.jrd_230) break;
		{
			// If the rfr field is also being deleted, there's no dependency
			if (!find_depend_in_dfw(tdbb, /*RFR.RDB$FIELD_NAME*/
						      jrd_228.jrd_229, obj_computed, /*REL.RDB$RELATION_ID*/
		jrd_228.jrd_231,
									transaction))
			{
				field_count++;
			}
		}
		/*END_FOR*/
		   }
		}

		if (field_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) <<								// Msg353: can not delete
					 Arg::Gds(isc_domain_name) << Arg::Str(work->dfw_name) <<
					 Arg::Gds(isc_dependency) << Arg::Num(field_count)); 	// Msg310: there are %ld dependencies
		}

		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, NULL, obj_field, transaction);

	case 2:
		return true;

	case 3:
		MET_delete_dependencies(tdbb, work->dfw_name, obj_computed, transaction);
		MET_delete_dependencies(tdbb, work->dfw_name, obj_validation, transaction);
		break;
	}

	return false;
}


static bool modify_field(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          bid  jrd_215;	// RDB$VALIDATION_BLR 
          SSHORT jrd_216;	// gds__utility 
          SSHORT jrd_217;	// gds__null_flag 
   } jrd_214;
   struct {
          TEXT  jrd_213 [32];	// RDB$FIELD_NAME 
   } jrd_212;
   struct {
          TEXT  jrd_222 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_223;	// gds__utility 
          SSHORT jrd_224;	// RDB$FIELD_ID 
   } jrd_221;
   struct {
          TEXT  jrd_220 [32];	// RDB$FIELD_SOURCE 
   } jrd_219;
/**************************************
 *
 *	m o d i f y _ f i e l d
 *
 **************************************
 *
 * Functional description
 *	Handle constraint dependencies of a field.
 *
 **************************************/

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
		{
			const Firebird::MetaName depName(work->dfw_name);
			AutoRequest handle;

			// If a domain is being changed to NOT NULL, schedule validation of involved relations.
			if (work->findArg(dfw_arg_field_not_null))
			{
				/*FOR(REQUEST_HANDLE handle)
					RFL IN RDB$RELATION_FIELDS CROSS
					REL IN RDB$RELATIONS
					WITH REL.RDB$RELATION_NAME EQ RFL.RDB$RELATION_NAME AND
						 RFL.RDB$FIELD_SOURCE EQ depName.c_str() AND
						 REL.RDB$VIEW_BLR MISSING
					REDUCED TO RFL.RDB$RELATION_NAME, RFL.RDB$FIELD_ID*/
				{
				handle.compile(tdbb, (UCHAR*) jrd_218, sizeof(jrd_218));
				gds__vtov ((const char*) depName.c_str(), (char*) jrd_219.jrd_220, 32);
				EXE_start (tdbb, handle, attachment->getSysTransaction());
				EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_219);
				while (1)
				   {
				   EXE_receive (tdbb, handle, 1, 36, (UCHAR*) &jrd_221);
				   if (!jrd_221.jrd_223) break;
				{
					dsc desc;
					desc.makeText(strlen(/*RFL.RDB$RELATION_NAME*/
							     jrd_221.jrd_222), CS_METADATA,
						(UCHAR*) /*RFL.RDB$RELATION_NAME*/
							 jrd_221.jrd_222);

					DeferredWork* work = DFW_post_work(transaction, dfw_check_not_null, &desc, 0);
					SortedArray<int>& ids = DFW_get_ids(work);
					ids.add(/*RFL.RDB$FIELD_ID*/
						jrd_221.jrd_224);
				}
				/*END_FOR*/
				   }
				}
			}

			bid validation;
			validation.clear();

			handle.reset();

			/*FOR(REQUEST_HANDLE handle)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ depName.c_str()*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_211, sizeof(jrd_211));
			gds__vtov ((const char*) depName.c_str(), (char*) jrd_212.jrd_213, 32);
			EXE_start (tdbb, handle, attachment->getSysTransaction());
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_212);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 12, (UCHAR*) &jrd_214);
			   if (!jrd_214.jrd_216) break;
			{
				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_214.jrd_217)
					validation = /*FLD.RDB$VALIDATION_BLR*/
						     jrd_214.jrd_215;
			}
			/*END_FOR*/
			   }
			}

			const DeferredWork* const arg = work->findArg(dfw_arg_new_name);

			// ASF: If there are procedures depending on the domain, it can't be renamed.
			if (arg && depName != arg->dfw_name.c_str())
				check_dependencies(tdbb, depName.c_str(), NULL, NULL, obj_field, transaction);

			MET_delete_dependencies(tdbb, depName, obj_validation, transaction);

			if (!validation.isEmpty())
			{
				MemoryPool* new_pool = attachment->createPool();
				Jrd::ContextPoolHolder context(tdbb, new_pool);

				MET_get_dependencies(tdbb, NULL, NULL, 0, NULL, &validation,
					NULL, NULL, depName, obj_validation, 0, transaction, depName);

				attachment->deletePool(new_pool);
			}
		}
		// fall through

	case 2:
	case 3:
		return true;

	case 4:	// after scan_relation (phase 3)
		check_computed_dependencies(tdbb, transaction, work->dfw_name);
		break;
	}

	return false;
}


static bool delete_global(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_210;	// gds__utility 
   } jrd_209;
   struct {
          TEXT  jrd_208 [32];	// RDB$FIELD_NAME 
   } jrd_207;
/**************************************
 *
 *	d e l e t e _ g l o b a l
 *
 **************************************
 *
 * Functional description
 *	If a local field has been deleted,
 *	check to see if its global field
 *	is computed. If so, delete all its
 *	dependencies under the assumption
 *	that a global computed field has only
 *	one local field.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			AutoRequest handle;
			/*FOR(REQUEST_HANDLE handle)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ work->dfw_name.c_str() AND
					FLD.RDB$COMPUTED_BLR NOT MISSING*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_206, sizeof(jrd_206));
			gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_207.jrd_208, 32);
			EXE_start (tdbb, handle, attachment->getSysTransaction());
			EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_207);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_209);
			   if (!jrd_209.jrd_210) break;
			{
				MET_delete_dependencies(tdbb, work->dfw_name, obj_computed, transaction);
			}
			/*END_FOR*/
			   }
			}
		}
		break;
	}

	return false;
}


static void check_partners(thread_db* tdbb, const USHORT rel_id)
{
/**************************************
 *
 *	c h e c k _ p a r t n e r s
 *
 **************************************
 *
 * Functional description
 *	Signal other processes to check partners of relation rel_id
 * Used when FK index was dropped
 *
 **************************************/
	const Jrd::Attachment* att = tdbb->getAttachment();
	vec<jrd_rel*>* relations = att->att_relations;

	fb_assert(relations);
	fb_assert(rel_id < relations->count());

	jrd_rel *relation = (*relations)[rel_id];
	fb_assert(relation);

	LCK_lock(tdbb, relation->rel_partners_lock, LCK_EX, LCK_WAIT);
	LCK_release(tdbb, relation->rel_partners_lock);
	relation->rel_flags |= REL_check_partners;
}


static bool delete_index(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	IndexLock* index = NULL;

	SET_TDBB(tdbb);

	const DeferredWork* arg = work->findArg(dfw_arg_index_name);

	fb_assert(arg);
	fb_assert(arg->dfw_id > 0);
	const USHORT id = arg->dfw_id - 1;
	// Maybe a permanent check?
	//if (id == idx_invalid)
	//	ERR_post(...);

	// Look up the relation.  If we can't find the relation,
	// don't worry about the index.

	jrd_rel* relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
	if (!relation) {
		return false;
	}

	RelationPages* relPages = relation->getPages(tdbb, MAX_TRA_NUMBER, false);
	if (!relPages) {
		return false;
	}
	// we need to special handle temp tables with ON PRESERVE ROWS only
	const bool isTempIndex = (relation->rel_flags & REL_temp_conn) &&
		(relPages->rel_instance_id != 0);

	switch (phase)
	{
	case 0:
		index = CMP_get_index_lock(tdbb, relation, id);
		if (index)
		{
			if (!index->idl_count)
				LCK_release(tdbb, index->idl_lock);
		}
		return false;

	case 1:
		check_dependencies(tdbb, arg->dfw_name.c_str(), NULL, NULL, obj_index, transaction);
		return true;

	case 2:
		return true;

	case 3:
		// Make sure nobody is currently using the index

		// If we about to delete temp index instance then usage counter
		// will remains 1 and will be decremented by IDX_delete_index at
		// phase 4

		index = CMP_get_index_lock(tdbb, relation, id);
		if (index)
		{
			// take into account lock probably used by temp index instance
			bool temp_lock_released = false;
			if (isTempIndex && (index->idl_count == 1))
			{
				index_desc idx;
				if (BTR_lookup(tdbb, relation, id, &idx, relPages))
				{
					index->idl_count--;
					LCK_release(tdbb, index->idl_lock);
					temp_lock_released = true;
				}
			}

			// Try to clear trigger cache to release lock
			if (index->idl_count)
				MET_clear_cache(tdbb);

			if (!isTempIndex)
			{
				if (index->idl_count ||
					!LCK_lock(tdbb, index->idl_lock, LCK_EX, transaction->getLockWait()))
				{
					// restore lock used by temp index instance
					if (temp_lock_released)
					{
						LCK_lock(tdbb, index->idl_lock, LCK_SR, LCK_WAIT);
						index->idl_count++;
					}

					raiseObjectInUseError("INDEX", arg->dfw_name);
				}
				index->idl_count++;
			}
		}

		return true;

	case 4:
		index = CMP_get_index_lock(tdbb, relation, id);
		if (isTempIndex && index)
			index->idl_count++;
		IDX_delete_index(tdbb, relation, id);

		if (isTempIndex)
			return false;

		if (work->dfw_type == dfw_delete_expression_index)
		{
			MET_delete_dependencies(tdbb, arg->dfw_name, obj_expression_index, transaction);
		}

		// if index was bound to deleted FK constraint
		// then work->dfw_args was set in VIO_erase
		arg = work->findArg(dfw_arg_partner_rel_id);

		if (arg) {
			if (arg->dfw_id) {
				check_partners(tdbb, relation->rel_id);
				if (relation->rel_id != arg->dfw_id) {
					check_partners(tdbb, arg->dfw_id);
				}
			}
			else {
				// partner relation was not found in VIO_erase
				// we must check partners of all relations in database
				MET_update_partners(tdbb);
			}
		}

		if (index)
		{
			/* in order for us to have gotten the lock in phase 3
			* idl_count HAD to be 0, therefore after having incremented
			* it for the exclusive lock it would have to be 1.
			* IF now it is NOT 1 then someone else got a lock to
			* the index and something is seriously wrong */
			fb_assert(index->idl_count == 1);
			if (!--index->idl_count)
			{
				// Release index existence lock and memory.

				for (IndexLock** ptr = &relation->rel_index_locks; *ptr; ptr = &(*ptr)->idl_next)
				{
					if (*ptr == index)
					{
						*ptr = index->idl_next;
						break;
					}
				}
				if (index->idl_lock)
				{
					LCK_release(tdbb, index->idl_lock);
					delete index->idl_lock;
				}
				delete index;

				// Release index refresh lock and memory.

				for (IndexBlock** iptr = &relation->rel_index_blocks; *iptr; iptr = &(*iptr)->idb_next)
				{
					if ((*iptr)->idb_id == id)
					{
						IndexBlock* index_block = *iptr;
						*iptr = index_block->idb_next;

						// Lock was released in IDX_delete_index().

						delete index_block->idb_lock;
						delete index_block;
						break;
					}
				}
			}
		}
		break;
	}

	return false;
}


static bool delete_parameter(thread_db* tdbb, SSHORT phase, DeferredWork*, jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Return an error if someone attempts to
 *	delete a field from a procedure and it is
 *	used by a view or procedure.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
		/* hvlad: temporary disable procedure parameters dependency check
		  until proper solution (something like dyn_mod_parameter)
		  will be implemented. This check never worked properly
		  so no harm is done

		if (MET_lookup_procedure_id(tdbb, work->dfw_id, false, true, 0))
		{
			const DeferredWork* arg = work->dfw_args;
			fb_assert(arg && (arg->dfw_type == dfw_arg_proc_name));

			check_dependencies(tdbb, arg->dfw_name.c_str(), work->dfw_name.c_str(),
							   obj_procedure, transaction);
		}
		*/
		break;
	}

	return false;
}


static bool delete_relation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_199;	// gds__utility 
   } jrd_198;
   struct {
          SSHORT jrd_197;	// gds__utility 
   } jrd_196;
   struct {
          SSHORT jrd_195;	// gds__utility 
   } jrd_194;
   struct {
          SSHORT jrd_193;	// RDB$RELATION_ID 
   } jrd_192;
   struct {
          TEXT  jrd_204 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_205;	// gds__utility 
   } jrd_203;
   struct {
          TEXT  jrd_202 [32];	// RDB$RELATION_NAME 
   } jrd_201;
/**************************************
 *
 *	d e l e t e _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Check if it is allowable to delete
 *	a relation, and if so, clean up after it.
 *
 **************************************/
	AutoRequest request;
	jrd_rel* relation;
	Resource* rsc;
	USHORT view_count;
	bool adjusted;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	switch (phase)
	{
	case 0:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation) {
			return false;
		}

		if (relation->rel_existence_lock)
		{
			LCK_convert(tdbb, relation->rel_existence_lock, LCK_SR, transaction->getLockWait());
		}

		if (relation->rel_flags & REL_deleting)
		{
			relation->rel_flags &= ~REL_deleting;
			relation->rel_drop_mutex.leave();
		}

		return false;

	case 1:
		// check if any views use this as a base relation

		request.reset();
		view_count = 0;
		/*FOR(REQUEST_HANDLE request)
			X IN RDB$VIEW_RELATIONS WITH
			X.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		request.compile(tdbb, (UCHAR*) jrd_200, sizeof(jrd_200));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_201.jrd_202, 32);
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_201);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_203);
		   if (!jrd_203.jrd_205) break;
		{
			// If the view is also being deleted, there's no dependency
			if (!find_depend_in_dfw(tdbb, /*X.RDB$VIEW_NAME*/
						      jrd_203.jrd_204, obj_view, 0, transaction))
			{
				view_count++;
			}
		}
		/*END_FOR*/
		   }
		}

		if (view_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) << // Msg353: can not delete
					 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
					 Arg::Gds(isc_dependency) << Arg::Num(view_count));
					 // Msg310: there are %ld dependencies
		}

		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation)
			return false;

		check_dependencies(tdbb, work->dfw_name.c_str(), NULL, NULL,
			(relation->isView() ? obj_view : obj_relation), transaction);
		return true;

	case 2:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (!relation) {
			return false;
		}

		// Let relation be deleted if only this transaction is using it

		adjusted = false;
		if (relation->rel_use_count == 1)
		{
			for (rsc = transaction->tra_resources.begin(); rsc < transaction->tra_resources.end();
				rsc++)
			{
				if (rsc->rsc_rel == relation)
				{
					--relation->rel_use_count;
					adjusted = true;
					break;
				}
			}
		}

		if (relation->rel_use_count)
			MET_clear_cache(tdbb);

		if (relation->rel_use_count || (relation->rel_existence_lock &&
			!LCK_convert(tdbb, relation->rel_existence_lock, LCK_EX, transaction->getLockWait())))
		{
			if (adjusted)
				++relation->rel_use_count;

			raiseRelationInUseError(relation);
		}

		fb_assert(!relation->rel_use_count);

		// Flag relation delete in progress so that active sweep or
		// garbage collector threads working on relation can skip over it

		relation->rel_flags |= REL_deleting;
		{ // scope
			Jrd::Attachment::Checkout attCout(attachment, FB_FUNCTION);
			relation->rel_drop_mutex.enter(FB_FUNCTION);
		}

		return true;

	case 3:
		return true;

	case 4:
		relation = MET_lookup_relation_id(tdbb, work->dfw_id, true);
		if (!relation) {
			return false;
		}

		// The sweep and garbage collector threads have no more than
		// a single record latency in responding to the flagged relation
		// deletion. Nevertheless, as a defensive programming measure,
		// don't wait forever if something has gone awry and the sweep
		// count doesn't run down.

		for (int wait = 0; wait < 60; wait++)
		{
			if (!relation->rel_sweep_count) {
				break;
			}

			Jrd::Attachment::Checkout attCout(attachment, FB_FUNCTION);
			THREAD_SLEEP(1 * 1000);
		}

		if (relation->rel_sweep_count)
			raiseRelationInUseError(relation);

		// Free any memory associated with the relation's garbage collection bitmap
		if (dbb->dbb_garbage_collector) {
			dbb->dbb_garbage_collector->removeRelation(relation->rel_id);
		}

		if (relation->rel_file) {
		    EXT_fini(relation, false);
		}

		RelationPages* relPages = relation->getBasePages();
		if (relPages->rel_index_root) {
			IDX_delete_indices(tdbb, relation, relPages);
		}

		if (relPages->rel_pages) {
			DPM_delete_relation(tdbb, relation);
		}

		// if this is a view (or even if we don't know), delete dependency lists

		if (relation->rel_view_rse || !(relation->rel_flags & REL_scanned)) {
			MET_delete_dependencies(tdbb, work->dfw_name, obj_view, transaction);
		}

		// Now that the data, pointer, and index pages are gone,
		// get rid of the relation itself

		request.reset();

		/*FOR(REQUEST_HANDLE request) X IN RDB$FORMATS WITH
			X.RDB$RELATION_ID EQ relation->rel_id*/
		{
		request.compile(tdbb, (UCHAR*) jrd_191, sizeof(jrd_191));
		jrd_192.jrd_193 = relation->rel_id;
		EXE_start (tdbb, request, attachment->getSysTransaction());
		EXE_send (tdbb, request, 0, 2, (UCHAR*) &jrd_192);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_194);
		   if (!jrd_194.jrd_195) break;
		{
			/*ERASE X;*/
			EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_196);
		}
		/*END_FOR*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_198);
		   }
		}

		// Release relation locks
		if (relation->rel_existence_lock) {
			LCK_release(tdbb, relation->rel_existence_lock);
		}
		if (relation->rel_partners_lock) {
			LCK_release(tdbb, relation->rel_partners_lock);
		}
		if (relation->rel_rescan_lock) {
			LCK_release(tdbb, relation->rel_rescan_lock);
		}

		// Mark relation in the cache as dropped
		relation->rel_flags |= REL_deleted;

		if (relation->rel_flags & REL_deleting)
		{
			relation->rel_flags &= ~REL_deleting;
			relation->rel_drop_mutex.leave();
		}

		// Release relation triggers
		MET_release_triggers(tdbb, &relation->rel_pre_store);
		MET_release_triggers(tdbb, &relation->rel_post_store);
		MET_release_triggers(tdbb, &relation->rel_pre_erase);
		MET_release_triggers(tdbb, &relation->rel_post_erase);
		MET_release_triggers(tdbb, &relation->rel_pre_modify);
		MET_release_triggers(tdbb, &relation->rel_post_modify);

		break;
	}

	return false;
}


static bool delete_rfr(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_177;	// gds__utility 
   } jrd_176;
   struct {
          SSHORT jrd_175;	// RDB$RELATION_ID 
   } jrd_174;
   struct {
          SSHORT jrd_182;	// gds__utility 
   } jrd_181;
   struct {
          SSHORT jrd_180;	// RDB$RELATION_ID 
   } jrd_179;
   struct {
          TEXT  jrd_188 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_189 [32];	// RDB$VIEW_NAME 
          SSHORT jrd_190;	// gds__utility 
   } jrd_187;
   struct {
          TEXT  jrd_185 [32];	// RDB$BASE_FIELD 
          SSHORT jrd_186;	// RDB$RELATION_ID 
   } jrd_184;
/**************************************
 *
 *	d e l e t e _ r f r
 *
 **************************************
 *
 * Functional description
 *	This whole routine exists just to
 *	return an error if someone attempts to
 *	1. delete a field from a relation if the relation
 *		is used in a view and the field is referenced in
 *		the view.
 *	2. drop the last column of a table
 *
 **************************************/
	int rel_exists, field_count;
	AutoRequest handle;
	Firebird::MetaName f;
	jrd_rel* relation;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
		// first check if there are any fields used explicitly by the view

		handle.reset();
		field_count = 0;
		/*FOR(REQUEST_HANDLE handle)
			REL IN RDB$RELATIONS CROSS
				VR IN RDB$VIEW_RELATIONS OVER RDB$RELATION_NAME CROSS
				VFLD IN RDB$RELATION_FIELDS WITH
				REL.RDB$RELATION_ID EQ work->dfw_id AND
				VFLD.RDB$VIEW_CONTEXT EQ VR.RDB$VIEW_CONTEXT AND
				VFLD.RDB$RELATION_NAME EQ VR.RDB$VIEW_NAME AND
				VFLD.RDB$BASE_FIELD EQ work->dfw_name.c_str()*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_183, sizeof(jrd_183));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_184.jrd_185, 32);
		jrd_184.jrd_186 = work->dfw_id;
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 34, (UCHAR*) &jrd_184);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 66, (UCHAR*) &jrd_187);
		   if (!jrd_187.jrd_190) break;
		{
			// If the view is also being deleted, there's no dependency
			if (!find_depend_in_dfw(tdbb, /*VR.RDB$VIEW_NAME*/
						      jrd_187.jrd_189, obj_view, 0, transaction))
			{
				f = /*VFLD.RDB$BASE_FIELD*/
				    jrd_187.jrd_188;
				field_count++;
			}
		}
		/*END_FOR*/
		   }
		}

		if (field_count)
		{
			ERR_post(Arg::Gds(isc_no_meta_update) <<
					 Arg::Gds(isc_no_delete) <<	// Msg353: can not delete
					 Arg::Gds(isc_field_name) << Arg::Str(f) <<
					 Arg::Gds(isc_dependency) << Arg::Num(field_count));
					 // Msg310: there are %ld dependencies
		}

		// now check if there are any dependencies generated through the blr
		// that defines the relation

		if ( (relation = MET_lookup_relation_id(tdbb, work->dfw_id, false)) )
		{
			check_dependencies(tdbb, relation->rel_name.c_str(), work->dfw_name.c_str(), NULL,
							   (relation->isView() ? obj_view : obj_relation),
							   transaction);
		}

		// see if the relation itself is being dropped

		handle.reset();
		rel_exists = 0;
		/*FOR(REQUEST_HANDLE handle)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_ID EQ work->dfw_id*/
		{
		handle.compile(tdbb, (UCHAR*) jrd_178, sizeof(jrd_178));
		jrd_179.jrd_180 = work->dfw_id;
		EXE_start (tdbb, handle, attachment->getSysTransaction());
		EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_179);
		while (1)
		   {
		   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_181);
		   if (!jrd_181.jrd_182) break;
		{
			rel_exists++;
		}
		/*END_FOR*/
		   }
		}

		// if table exists, check if this is the last column in the table

		if (rel_exists)
		{
			field_count = 0;
			handle.reset();

			/*FOR(REQUEST_HANDLE handle)
				REL IN RDB$RELATIONS CROSS
					RFLD IN RDB$RELATION_FIELDS OVER RDB$RELATION_NAME
					WITH REL.RDB$RELATION_ID EQ work->dfw_id*/
			{
			handle.compile(tdbb, (UCHAR*) jrd_173, sizeof(jrd_173));
			jrd_174.jrd_175 = work->dfw_id;
			EXE_start (tdbb, handle, attachment->getSysTransaction());
			EXE_send (tdbb, handle, 0, 2, (UCHAR*) &jrd_174);
			while (1)
			   {
			   EXE_receive (tdbb, handle, 1, 2, (UCHAR*) &jrd_176);
			   if (!jrd_176.jrd_177) break;
				field_count++;
			/*END_FOR*/
			   }
			}

			if (!field_count)
			{
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_del_last_field));
				// Msg354: last column in a relation cannot be deleted
			}
		}

	case 2:
		return true;

	case 3:
		// Unlink field from data structures.  Don't try to actually release field and
		// friends -- somebody may be pointing to them

		relation = MET_lookup_relation_id(tdbb, work->dfw_id, false);
		if (relation)
		{
			const int id = MET_lookup_field(tdbb, relation, work->dfw_name);
			if (id >= 0)
			{
				vec<jrd_fld*>* vector = relation->rel_fields;
				if (vector && (ULONG) id < vector->count() && (*vector)[id])
				{
					(*vector)[id] = NULL;
				}
			}
		}
		break;
	}

	return false;
}


static bool delete_shadow(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	d e l e t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Provide deferred work interface to
 *	MET_delete_shadow.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		MET_delete_shadow(tdbb, work->dfw_id);
		break;
	}

	return false;
}


static bool delete_trigger(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
/**************************************
 *
 *	d e l e t e _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Cleanup after a deleted trigger.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		// get rid of dependencies
		MET_delete_dependencies(tdbb, work->dfw_name, obj_trigger, transaction);
		return true;

	case 4:
		{
			const DeferredWork* arg = work->findArg(dfw_arg_rel_name);
			if (!arg)
			{
				const DeferredWork* arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				// ASF: arg->dfw_id is RDB$TRIGGER_TYPE truncated to USHORT
				if (arg && (arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
				{
					MET_release_trigger(tdbb,
						&tdbb->getAttachment()->att_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB],
						work->dfw_name);
				}
			}
		}
		break;
	}

	return false;
}


static bool find_depend_in_dfw(thread_db*	tdbb,
							   TEXT*	object_name,
							   USHORT	dep_type,
							   USHORT	rel_id,
							   jrd_tra*		transaction)
{
   struct {
          bid  jrd_163;	// RDB$VALIDATION_BLR 
          SSHORT jrd_164;	// gds__utility 
          SSHORT jrd_165;	// gds__null_flag 
   } jrd_162;
   struct {
          TEXT  jrd_161 [32];	// RDB$FIELD_NAME 
   } jrd_160;
   struct {
          TEXT  jrd_170 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_171;	// gds__utility 
          SSHORT jrd_172;	// RDB$RELATION_ID 
   } jrd_169;
   struct {
          TEXT  jrd_168 [32];	// RDB$FIELD_NAME 
   } jrd_167;
/**************************************
 *
 *	f i n d _ d e p e n d _ i n _ d f w
 *
 **************************************
 *
 * Functional description
 *	Check the object to see if it is being
 *	deleted as part of the deferred work.
 *	Return true if it is, false otherwise.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	fb_utils::exact_name(object_name);
	enum dfw_t dfw_type;
	switch (dep_type)
	{
	case obj_view:
		dfw_type = dfw_delete_relation;
		break;
	case obj_trigger:
		dfw_type = dfw_delete_trigger;
		break;
	case obj_computed:
		dfw_type = rel_id ? dfw_delete_rfr : dfw_delete_global;
		break;
	case obj_validation:
		dfw_type = dfw_delete_global;
		break;
	case obj_procedure:
		dfw_type = dfw_delete_procedure;
		break;
	case obj_expression_index:
		dfw_type = dfw_delete_expression_index;
		break;
	case obj_package_header:
		dfw_type = dfw_drop_package_header;
		break;
	case obj_package_body:
		dfw_type = dfw_drop_package_body;
		break;
	case obj_udf:
		dfw_type = dfw_delete_function;
		break;
	default:
		fb_assert(false);
		break;
	}

	// Look to see if an object of the desired type is being deleted or modified.
	// For an object being modified we verify dependencies separately when we parse its BLR.
	for (const DeferredWork* work = transaction->tra_deferred_job->work; work; work = work->getNext())
	{
		if ((work->dfw_type == dfw_type ||
			(work->dfw_type == dfw_modify_procedure && dfw_type == dfw_delete_procedure) ||
			(work->dfw_type == dfw_modify_field && dfw_type == dfw_delete_global) ||
			(work->dfw_type == dfw_modify_trigger && dfw_type == dfw_delete_trigger) ||
			(work->dfw_type == dfw_modify_function && dfw_type == dfw_delete_function)) &&
			work->dfw_name == object_name && work->dfw_package.isEmpty() &&
			(!rel_id || rel_id == work->dfw_id))
		{
			if (work->dfw_type == dfw_modify_procedure || work->dfw_type == dfw_modify_function)
			{
				// Don't consider that routine is in DFW if we are only checking the BLR
				if (!work->findArg(dfw_arg_check_blr))
					return true;
			}
			else
			{
				return true;
			}
		}

		if (work->dfw_type == dfw_type && dfw_type == dfw_delete_expression_index)
		{
			for (size_t i = 0; i < work->dfw_args.getCount(); ++i)
			{
				const DeferredWork* arg = work->dfw_args[i];
				if (arg->dfw_type == dfw_arg_index_name &&
					arg->dfw_name == object_name)
				{
					return true;
				}
			}
		}
	}

	if (dfw_type == dfw_delete_global)
	{
		if (dep_type == obj_computed)
		{
			// Computed fields are more complicated.  If the global field isn't being
			// deleted, see if all of the fields it is the source for, are.

			AutoCacheRequest request(tdbb, irq_ch_cmp_dpd, IRQ_REQUESTS);

			/*FOR(REQUEST_HANDLE request)
				FLD IN RDB$FIELDS CROSS
					RFR IN RDB$RELATION_FIELDS CROSS
					REL IN RDB$RELATIONS
					WITH FLD.RDB$FIELD_NAME EQ RFR.RDB$FIELD_SOURCE
					AND FLD.RDB$FIELD_NAME EQ object_name
					AND REL.RDB$RELATION_NAME EQ RFR.RDB$RELATION_NAME*/
			{
			request.compile(tdbb, (UCHAR*) jrd_166, sizeof(jrd_166));
			gds__vtov ((const char*) object_name, (char*) jrd_167.jrd_168, 32);
			EXE_start (tdbb, request, attachment->getSysTransaction());
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_167);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_169);
			   if (!jrd_169.jrd_171) break;
			{
				if (!find_depend_in_dfw(tdbb, /*RFR.RDB$FIELD_NAME*/
							      jrd_169.jrd_170, obj_computed,
										/*REL.RDB$RELATION_ID*/
										jrd_169.jrd_172, transaction))
				{
					return false;
				}
			}
			/*END_FOR*/
			   }
			}

			return true;
		}

		if (dep_type == obj_validation)
		{
			// Maybe it's worth caching in the future?
			AutoRequest request;

			/*FOR(REQUEST_HANDLE request)
				FLD IN RDB$FIELDS WITH
					FLD.RDB$FIELD_NAME EQ object_name*/
			{
			request.compile(tdbb, (UCHAR*) jrd_159, sizeof(jrd_159));
			gds__vtov ((const char*) object_name, (char*) jrd_160.jrd_161, 32);
			EXE_start (tdbb, request, attachment->getSysTransaction());
			EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_160);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_162);
			   if (!jrd_162.jrd_164) break;
			{
				if (!/*FLD.RDB$VALIDATION_BLR.NULL*/
				     jrd_162.jrd_165)
					return false;
			}
			/*END_FOR*/
			   }
			}

			return true;
		}
	}

	return false;
}


static void get_array_desc(thread_db* tdbb, const TEXT* field_name, Ods::InternalArrayDesc* desc)
{
   struct {
          SLONG  jrd_155;	// RDB$UPPER_BOUND 
          SLONG  jrd_156;	// RDB$LOWER_BOUND 
          SSHORT jrd_157;	// gds__utility 
          SSHORT jrd_158;	// RDB$DIMENSION 
   } jrd_154;
   struct {
          TEXT  jrd_153 [32];	// RDB$FIELD_NAME 
   } jrd_152;
/**************************************
 *
 *	g e t _ a r r a y _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Get array descriptor for an array.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	AutoCacheRequest request(tdbb, irq_r_fld_dim, IRQ_REQUESTS);

	Ods::InternalArrayDesc::iad_repeat* ranges = 0;
	/*FOR (REQUEST_HANDLE request)
		D IN RDB$FIELD_DIMENSIONS WITH D.RDB$FIELD_NAME EQ field_name*/
	{
	request.compile(tdbb, (UCHAR*) jrd_151, sizeof(jrd_151));
	gds__vtov ((const char*) field_name, (char*) jrd_152.jrd_153, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_152);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 12, (UCHAR*) &jrd_154);
	   if (!jrd_154.jrd_157) break;
	{
		if (/*D.RDB$DIMENSION*/
		    jrd_154.jrd_158 >= 0 && /*D.RDB$DIMENSION*/
	 jrd_154.jrd_158 < desc->iad_dimensions)
		{
			ranges = desc->iad_rpt + /*D.RDB$DIMENSION*/
						 jrd_154.jrd_158;
			ranges->iad_lower = /*D.RDB$LOWER_BOUND*/
					    jrd_154.jrd_156;
			ranges->iad_upper = /*D.RDB$UPPER_BOUND*/
					    jrd_154.jrd_155;
		}
	}
	/*END_FOR*/
	   }
	}

	desc->iad_count = 1;

	for (ranges = desc->iad_rpt + desc->iad_dimensions; --ranges >= desc->iad_rpt;)
	{
		ranges->iad_length = desc->iad_count;
		desc->iad_count *= ranges->iad_upper - ranges->iad_lower + 1;
	}

	desc->iad_version = Ods::IAD_VERSION_1;
	desc->iad_length = IAD_LEN(MAX(desc->iad_struct_count, desc->iad_dimensions));
	desc->iad_element_length = desc->iad_rpt[0].iad_desc.dsc_length;
	desc->iad_total_length = desc->iad_element_length * desc->iad_count;
}


static void get_trigger_dependencies(DeferredWork* work, bool compile, jrd_tra* transaction)
{
   struct {
          TEXT  jrd_147 [32];	// RDB$RELATION_NAME 
          ISC_INT64  jrd_148;	// RDB$TRIGGER_TYPE 
          bid  jrd_149;	// RDB$TRIGGER_BLR 
          SSHORT jrd_150;	// gds__utility 
   } jrd_146;
   struct {
          TEXT  jrd_145 [32];	// RDB$TRIGGER_NAME 
   } jrd_144;
/**************************************
 *
 *	g e t _ t r i g g e r _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Get relations and fields on which this
 *	trigger depends, either when it's being
 *	created or when it's modified.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Jrd::Attachment* attachment = tdbb->getAttachment();

	if (compile)
		compile = !(tdbb->getAttachment()->att_flags & ATT_gbak_attachment);

	jrd_rel* relation = NULL;
	bid blob_id;
	blob_id.clear();

	ISC_UINT64 type = 0;

	AutoCacheRequest handle(tdbb, irq_c_trigger, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE handle)
		X IN RDB$TRIGGERS WITH
			X.RDB$TRIGGER_NAME EQ work->dfw_name.c_str()*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_143, sizeof(jrd_143));
	gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_144.jrd_145, 32);
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_144);
	while (1)
	   {
	   EXE_receive (tdbb, handle, 1, 50, (UCHAR*) &jrd_146);
	   if (!jrd_146.jrd_150) break;
	{
		blob_id = /*X.RDB$TRIGGER_BLR*/
			  jrd_146.jrd_149;
		type = (ISC_UINT64) /*X.RDB$TRIGGER_TYPE*/
				    jrd_146.jrd_148;
		relation = MET_lookup_relation(tdbb, /*X.RDB$RELATION_NAME*/
						     jrd_146.jrd_147);
	}
	/*END_FOR*/
	   }
	}

	// get any dependencies now by parsing the blr

	if ((relation || (type & TRIGGER_TYPE_MASK) != TRIGGER_TYPE_DML) && !blob_id.isEmpty())
	{
		JrdStatement* statement = NULL;
		// Nickolay Samofatov: allocate statement memory pool...
		MemoryPool* new_pool = attachment->createPool();
		USHORT par_flags;

		if ((type & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DML)
			par_flags = (USHORT) ((type & 1) ? csb_pre_trigger : csb_post_trigger);
		else
			par_flags = 0;

		Jrd::ContextPoolHolder context(tdbb, new_pool);
		const Firebird::MetaName depName(work->dfw_name);
		MET_get_dependencies(tdbb, relation, NULL, 0, NULL, &blob_id, (compile ? &statement : NULL),
							 NULL, depName, obj_trigger, par_flags, transaction);

		if (statement)
			statement->release(tdbb);
		else
			attachment->deletePool(new_pool);
	}
}


static void load_trigs(thread_db* tdbb, jrd_rel* relation, trig_vec** triggers)
{
/**************************************
 *
 *	l o a d _ t r i g s
 *
 **************************************
 *
 * Functional description
 *	We have just loaded the triggers onto the local vector
 *      triggers. Its now time to place them at their rightful
 *      place ie the relation block.
 *
 **************************************/
	trig_vec* tmp_vector;

	tmp_vector = relation->rel_pre_store;
	relation->rel_pre_store = triggers[TRIGGER_PRE_STORE];
	MET_release_triggers(tdbb, &tmp_vector);

	tmp_vector = relation->rel_post_store;
	relation->rel_post_store = triggers[TRIGGER_POST_STORE];
	MET_release_triggers(tdbb, &tmp_vector);

	tmp_vector = relation->rel_pre_erase;
	relation->rel_pre_erase = triggers[TRIGGER_PRE_ERASE];
	MET_release_triggers(tdbb, &tmp_vector);

	tmp_vector = relation->rel_post_erase;
	relation->rel_post_erase = triggers[TRIGGER_POST_ERASE];
	MET_release_triggers(tdbb, &tmp_vector);

	tmp_vector = relation->rel_pre_modify;
	relation->rel_pre_modify = triggers[TRIGGER_PRE_MODIFY];
	MET_release_triggers(tdbb, &tmp_vector);

	tmp_vector = relation->rel_post_modify;
	relation->rel_post_modify = triggers[TRIGGER_POST_MODIFY];
	MET_release_triggers(tdbb, &tmp_vector);
}


static Format* make_format(thread_db* tdbb, jrd_rel* relation, USHORT* version, TemporaryField* stack)
{
   struct {
          bid  jrd_140;	// RDB$DESCRIPTOR 
          SSHORT jrd_141;	// RDB$RELATION_ID 
          SSHORT jrd_142;	// RDB$FORMAT 
   } jrd_139;
/**************************************
 *
 *	m a k e _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Make a format block for a relation.
 *
 **************************************/
	TemporaryField* tfb;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	jrd_tra* sysTransaction = attachment->getSysTransaction();
	Database* dbb = tdbb->getDatabase();

	// Figure out the highest field id and allocate a format block

	USHORT count = 0;
	for (tfb = stack; tfb; tfb = tfb->tfb_next)
		count = MAX(count, tfb->tfb_id);

	Format* format = Format::newFormat(*dbb->dbb_permanent, count + 1);
	format->fmt_version = version ? *version : 0;

	// Fill in the format block from the temporary field blocks

	for (tfb = stack; tfb; tfb = tfb->tfb_next)
	{
		dsc* desc = &format->fmt_desc[tfb->tfb_id];
		if (tfb->tfb_flags & TFB_array)
		{
			desc->dsc_dtype = dtype_array;
			desc->dsc_length = sizeof(ISC_QUAD);
		}
		else
			*desc = tfb->tfb_desc;
		if (tfb->tfb_flags & TFB_computed)
			desc->dsc_dtype |= COMPUTED_FLAG;

		impure_value& defRef = format->fmt_defaults[tfb->tfb_id];
		defRef = tfb->tfb_default;

		if (tfb->tfb_default.vlu_string)
		{
			fb_assert(defRef.vlu_desc.dsc_dtype == dtype_text);
			defRef.vlu_desc.dsc_address = defRef.vlu_string->str_data;
		}
		else
			defRef.vlu_desc.dsc_address = (UCHAR*) &defRef.vlu_misc;
	}

	// Compute the offsets of the various fields

	ULONG offset = FLAG_BYTES(count);

	count = 0;
	for (Format::fmt_desc_iterator desc2 = format->fmt_desc.begin();
		count < format->fmt_count;
		++count, ++desc2)
	{
		if (desc2->dsc_dtype & COMPUTED_FLAG)
		{
			desc2->dsc_dtype &= ~COMPUTED_FLAG;
			continue;
		}
		if (desc2->dsc_dtype)
		{
			offset = MET_align(&(*desc2), offset);
			desc2->dsc_address = (UCHAR *) (IPTR) offset;
			offset += desc2->dsc_length;
		}
	}

	// Release the temporary field blocks

	while ( (tfb = stack) )
	{
		stack = tfb->tfb_next;
		delete tfb;
	}

	if (offset > MAX_RECORD_SIZE)
	{
		delete format;
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_rec_size_err) << Arg::Num(offset) <<
				 Arg::Gds(isc_table_name) << Arg::Str(relation->rel_name));
		// Msg361: new record size of %ld bytes is too big
	}

	format->fmt_length = offset;

	Format* old_format;
	if (format->fmt_version &&
		(old_format = MET_format(tdbb, relation, (format->fmt_version - 1))) &&
		(formatsAreEqual(old_format, format)))
	{
		delete format;
		*version = old_format->fmt_version;
		return old_format;
	}

	// Link the format block into the world

	vec<Format*>* vector = relation->rel_formats =
		vec<Format*>::newVector(*relation->rel_pool, relation->rel_formats, format->fmt_version + 1);
	(*vector)[format->fmt_version] = format;

	// Store format in system relation

	AutoCacheRequest request(tdbb, irq_format3, IRQ_REQUESTS);

	/*STORE(REQUEST_HANDLE request)
		FMTS IN RDB$FORMATS*/
	{
	
	{
		/*FMTS.RDB$FORMAT*/
		jrd_139.jrd_142 = format->fmt_version;
		/*FMTS.RDB$RELATION_ID*/
		jrd_139.jrd_141 = relation->rel_id;
		blb* blob = blb::create(tdbb, sysTransaction, &/*FMTS.RDB$DESCRIPTOR*/
							       jrd_139.jrd_140);

		// Use generic representation of formats with 32-bit offsets

		Firebird::Array<Ods::Descriptor> odsDescs;
		Ods::Descriptor* odsDesc = odsDescs.getBuffer(format->fmt_count);

		for (Format::fmt_desc_const_iterator desc = format->fmt_desc.begin();
			 desc < format->fmt_desc.end(); ++desc, ++odsDesc)
		{
			*odsDesc = *desc;
		}

		HalfStaticArray<UCHAR, BUFFER_MEDIUM> buffer;

		buffer.add(UCHAR(format->fmt_count));
		buffer.add(UCHAR(format->fmt_count >> 8));

		buffer.add((UCHAR*) odsDescs.begin(), odsDescs.getCount() * sizeof(Ods::Descriptor));

		const size_t pos = buffer.getCount();
		buffer.add(0);
		buffer.add(0);

		USHORT i = 0, dflCount = 0;
		for (Format::fmt_defaults_iterator impure = format->fmt_defaults.begin();
			 impure != format->fmt_defaults.end(); ++impure, ++i)
		{
			if (!impure->vlu_desc.isUnknown())
			{
				dsc desc = impure->vlu_desc;
				desc.dsc_address = NULL;

				Ods::Descriptor odsDflDesc = desc;

				buffer.add(UCHAR(i));
				buffer.add(UCHAR(i >> 8));
				buffer.add((UCHAR*) &odsDflDesc, sizeof(odsDflDesc));
				buffer.add(impure->vlu_desc.dsc_address, impure->vlu_desc.dsc_length);

				++dflCount;
			}
		}

		buffer[pos] = UCHAR(dflCount);
		buffer[pos + 1] = UCHAR(dflCount >> 8);

		blob->BLB_put_segment(tdbb, buffer.begin(), buffer.getCount());
		blob->BLB_close(tdbb);
	}
	/*END_STORE*/
	request.compile(tdbb, (UCHAR*) jrd_138, sizeof(jrd_138));
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 12, (UCHAR*) &jrd_139);
	}

	return format;
}


static bool make_version(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_50;	// gds__utility 
   } jrd_49;
   struct {
          SSHORT jrd_48;	// gds__utility 
   } jrd_47;
   struct {
          SSHORT jrd_46;	// gds__utility 
   } jrd_45;
   struct {
          SSHORT jrd_44;	// RDB$RELATION_ID 
   } jrd_43;
   struct {
          SSHORT jrd_55;	// gds__utility 
          SSHORT jrd_56;	// RDB$DBKEY_LENGTH 
   } jrd_54;
   struct {
          TEXT  jrd_53 [32];	// RDB$VIEW_NAME 
   } jrd_52;
   struct {
          SSHORT jrd_61;	// gds__utility 
   } jrd_60;
   struct {
          TEXT  jrd_59 [32];	// RDB$RELATION_NAME 
   } jrd_58;
   struct {
          SSHORT jrd_114;	// gds__utility 
   } jrd_113;
   struct {
          SSHORT jrd_105;	// gds__null_flag 
          SSHORT jrd_106;	// RDB$FIELD_ID 
          SSHORT jrd_107;	// gds__null_flag 
          SSHORT jrd_108;	// RDB$UPDATE_FLAG 
          SSHORT jrd_109;	// gds__null_flag 
          SSHORT jrd_110;	// RDB$FIELD_POSITION 
          SSHORT jrd_111;	// gds__null_flag 
          SSHORT jrd_112;	// RDB$NULL_FLAG 
   } jrd_104;
   struct {
          TEXT  jrd_66 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_67 [32];	// RDB$GENERATOR_NAME 
          TEXT  jrd_68 [32];	// RDB$SECURITY_CLASS 
          bid  jrd_69;	// RDB$VALIDATION_BLR 
          bid  jrd_70;	// RDB$DEFAULT_VALUE 
          bid  jrd_71;	// RDB$DEFAULT_VALUE 
          bid  jrd_72;	// RDB$MISSING_VALUE 
          TEXT  jrd_73 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_74 [32];	// RDB$FIELD_NAME 
          bid  jrd_75;	// RDB$COMPUTED_BLR 
          SSHORT jrd_76;	// gds__utility 
          SSHORT jrd_77;	// RDB$EXTERNAL_LENGTH 
          SSHORT jrd_78;	// RDB$EXTERNAL_SCALE 
          SSHORT jrd_79;	// RDB$EXTERNAL_TYPE 
          SSHORT jrd_80;	// RDB$DIMENSIONS 
          SSHORT jrd_81;	// RDB$FIELD_SUB_TYPE 
          SSHORT jrd_82;	// RDB$FIELD_LENGTH 
          SSHORT jrd_83;	// RDB$FIELD_SCALE 
          SSHORT jrd_84;	// RDB$FIELD_TYPE 
          SSHORT jrd_85;	// gds__null_flag 
          SSHORT jrd_86;	// RDB$COLLATION_ID 
          SSHORT jrd_87;	// gds__null_flag 
          SSHORT jrd_88;	// RDB$COLLATION_ID 
          SSHORT jrd_89;	// gds__null_flag 
          SSHORT jrd_90;	// RDB$CHARACTER_SET_ID 
          SSHORT jrd_91;	// gds__null_flag 
          SSHORT jrd_92;	// gds__null_flag 
          SSHORT jrd_93;	// RDB$VIEW_CONTEXT 
          SSHORT jrd_94;	// gds__null_flag 
          SSHORT jrd_95;	// RDB$NULL_FLAG 
          SSHORT jrd_96;	// gds__null_flag 
          SSHORT jrd_97;	// RDB$NULL_FLAG 
          SSHORT jrd_98;	// gds__null_flag 
          SSHORT jrd_99;	// RDB$FIELD_POSITION 
          SSHORT jrd_100;	// gds__null_flag 
          SSHORT jrd_101;	// RDB$UPDATE_FLAG 
          SSHORT jrd_102;	// gds__null_flag 
          SSHORT jrd_103;	// RDB$FIELD_ID 
   } jrd_65;
   struct {
          TEXT  jrd_64 [32];	// RDB$RELATION_NAME 
   } jrd_63;
   struct {
          SSHORT jrd_137;	// gds__utility 
   } jrd_136;
   struct {
          bid  jrd_130;	// RDB$RUNTIME 
          TEXT  jrd_131 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_132;	// gds__null_flag 
          SSHORT jrd_133;	// RDB$FORMAT 
          SSHORT jrd_134;	// RDB$FIELD_ID 
          SSHORT jrd_135;	// RDB$DBKEY_LENGTH 
   } jrd_129;
   struct {
          TEXT  jrd_119 [32];	// RDB$RELATION_NAME 
          bid  jrd_120;	// RDB$RUNTIME 
          TEXT  jrd_121 [256];	// RDB$EXTERNAL_FILE 
          bid  jrd_122;	// RDB$VIEW_BLR 
          SSHORT jrd_123;	// gds__utility 
          SSHORT jrd_124;	// RDB$DBKEY_LENGTH 
          SSHORT jrd_125;	// gds__null_flag 
          SSHORT jrd_126;	// RDB$FIELD_ID 
          SSHORT jrd_127;	// RDB$FORMAT 
          SSHORT jrd_128;	// RDB$RELATION_ID 
   } jrd_118;
   struct {
          TEXT  jrd_117 [32];	// RDB$RELATION_NAME 
   } jrd_116;
/**************************************
 *
 *	m a k e _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Make a new format version for a relation.  While we're at it, make
 *	sure all fields have id's.  If the relation is a view, make a
 *	a format anyway -- used for view updates.
 *
 *	While we're in the vicinity, also check the updatability of fields.
 *
 **************************************/
	TemporaryField* stack;
	TemporaryField* external;
	jrd_rel* relation;
	//bid blob_id;
	//blob_id.clear();

	USHORT n;
	int physical_fields = 0;
	bool external_flag = false;
	bool computed_field;
	trig_vec* triggers[TRIGGER_MAX];

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();
	bool null_view;

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		relation = NULL;
		stack = external = NULL;
		computed_field = false;

		for (n = 0; n < TRIGGER_MAX; n++) {
			triggers[n] = NULL;
		}

		AutoCacheRequest request_fmt1(tdbb, irq_format1, IRQ_REQUESTS);

		// User transaction may be safely used instead of system cause
		// all required dirty reads are performed in metadata cache. AP-2008.

		/*FOR(REQUEST_HANDLE request_fmt1 TRANSACTION_HANDLE transaction)
			REL IN RDB$RELATIONS WITH REL.RDB$RELATION_NAME EQ work->dfw_name.c_str()*/
		{
		request_fmt1.compile(tdbb, (UCHAR*) jrd_115, sizeof(jrd_115));
		gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_116.jrd_117, 32);
		EXE_start (tdbb, request_fmt1, transaction);
		EXE_send (tdbb, request_fmt1, 0, 32, (UCHAR*) &jrd_116);
		while (1)
		   {
		   EXE_receive (tdbb, request_fmt1, 1, 316, (UCHAR*) &jrd_118);
		   if (!jrd_118.jrd_123) break;
		{
			relation = MET_lookup_relation_id(tdbb, /*REL.RDB$RELATION_ID*/
								jrd_118.jrd_128, false);
			if (!relation)
				return false;

			const bid blob_id = /*REL.RDB$VIEW_BLR*/
					    jrd_118.jrd_122;
			null_view = blob_id.isEmpty();
			external_flag = /*REL.RDB$EXTERNAL_FILE*/
					jrd_118.jrd_121[0];

			if (/*REL.RDB$FORMAT*/
			    jrd_118.jrd_127 == MAX_TABLE_VERSIONS)
				raiseTooManyVersionsError(obj_relation, work->dfw_name);

			/*MODIFY REL USING*/
			{
			
				blb* blob = blb::create(tdbb, transaction, &/*REL.RDB$RUNTIME*/
									    jrd_118.jrd_120);
				AutoCacheRequest request_fmtx(tdbb, irq_format2, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request_fmtx TRANSACTION_HANDLE transaction)
					RFR IN RDB$RELATION_FIELDS CROSS
						FLD IN RDB$FIELDS WITH
						RFR.RDB$RELATION_NAME EQ work->dfw_name.c_str() AND
						RFR.RDB$FIELD_SOURCE EQ FLD.RDB$FIELD_NAME*/
				{
				request_fmtx.compile(tdbb, (UCHAR*) jrd_62, sizeof(jrd_62));
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_63.jrd_64, 32);
				EXE_start (tdbb, request_fmtx, transaction);
				EXE_send (tdbb, request_fmtx, 0, 32, (UCHAR*) &jrd_63);
				while (1)
				   {
				   EXE_receive (tdbb, request_fmtx, 1, 256, (UCHAR*) &jrd_65);
				   if (!jrd_65.jrd_76) break;
				{
					// Update RFR to reflect new fields id

					if (!/*RFR.RDB$FIELD_ID.NULL*/
					     jrd_65.jrd_102 && /*RFR.RDB$FIELD_ID*/
    jrd_65.jrd_103 >= /*REL.RDB$FIELD_ID*/
    jrd_118.jrd_126)
						/*REL.RDB$FIELD_ID*/
						jrd_118.jrd_126 = /*RFR.RDB$FIELD_ID*/
   jrd_65.jrd_103 + 1;

					// force recalculation of RDB$UPDATE_FLAG if field is calculated
					if (!/*FLD.RDB$COMPUTED_BLR*/
					     jrd_65.jrd_75.isEmpty())
					{
						/*RFR.RDB$UPDATE_FLAG.NULL*/
						jrd_65.jrd_100 = TRUE;
						computed_field = true;
					}

					if (/*RFR.RDB$FIELD_ID.NULL*/
					    jrd_65.jrd_102 || /*RFR.RDB$UPDATE_FLAG.NULL*/
    jrd_65.jrd_100)
					{
						/*MODIFY RFR USING*/
						{
						
							if (/*RFR.RDB$FIELD_ID.NULL*/
							    jrd_65.jrd_102)
							{
								if (external_flag)
								{
									/*RFR.RDB$FIELD_ID*/
									jrd_65.jrd_103 = /*RFR.RDB$FIELD_POSITION*/
   jrd_65.jrd_99;
									// RFR.RDB$FIELD_POSITION.NULL is
									// needed to be referenced in the
									// code somewhere for GPRE to include
									// this field in the structures that
									// it generates at the top of this func.
									/*RFR.RDB$FIELD_ID.NULL*/
									jrd_65.jrd_102 = /*RFR.RDB$FIELD_POSITION.NULL*/
   jrd_65.jrd_98;
								}
								else
								{
									/*RFR.RDB$FIELD_ID*/
									jrd_65.jrd_103 = /*REL.RDB$FIELD_ID*/
   jrd_118.jrd_126;
									/*RFR.RDB$FIELD_ID.NULL*/
									jrd_65.jrd_102 = FALSE;

									// If the table is being altered, check validity of NOT NULL fields.
									if (!/*REL.RDB$FORMAT.NULL*/
									     jrd_118.jrd_125)
									{
										bool notNull = (/*RFR.RDB$NULL_FLAG.NULL*/
												jrd_65.jrd_96 ?
											(/*FLD.RDB$NULL_FLAG.NULL*/
											 jrd_65.jrd_94 ? false : (bool) /*FLD.RDB$NULL_FLAG*/
		  jrd_65.jrd_95) :
											(bool) /*RFR.RDB$NULL_FLAG*/
											       jrd_65.jrd_97);

										if (notNull)
										{
											dsc desc;
											desc.makeText(strlen(/*REL.RDB$RELATION_NAME*/
													     jrd_118.jrd_119), CS_METADATA,
												(UCHAR*) /*REL.RDB$RELATION_NAME*/
													 jrd_118.jrd_119);

											DeferredWork* work = DFW_post_work(transaction,
												dfw_check_not_null, &desc, 0);
											SortedArray<int>& ids = DFW_get_ids(work);
											ids.add(/*RFR.RDB$FIELD_ID*/
												jrd_65.jrd_103);
										}
									}
								}

								/*REL.RDB$FIELD_ID*/
								jrd_118.jrd_126++;
							}
							if (/*RFR.RDB$UPDATE_FLAG.NULL*/
							    jrd_65.jrd_100)
							{
								/*RFR.RDB$UPDATE_FLAG.NULL*/
								jrd_65.jrd_100 = FALSE;
								/*RFR.RDB$UPDATE_FLAG*/
								jrd_65.jrd_101 = 1;
								if (!/*FLD.RDB$COMPUTED_BLR*/
								     jrd_65.jrd_75.isEmpty())
								{
									/*RFR.RDB$UPDATE_FLAG*/
									jrd_65.jrd_101 = 0;
								}
								if (!null_view && /*REL.RDB$DBKEY_LENGTH*/
										  jrd_118.jrd_124 > 8)
								{
									AutoRequest temp;
									/*RFR.RDB$UPDATE_FLAG*/
									jrd_65.jrd_101 = 0;
									/*FOR(REQUEST_HANDLE temp) X IN RDB$TRIGGERS WITH
										X.RDB$RELATION_NAME EQ work->dfw_name.c_str() AND
											X.RDB$TRIGGER_TYPE EQ 1*/
									{
									temp.compile(tdbb, (UCHAR*) jrd_57, sizeof(jrd_57));
									gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_58.jrd_59, 32);
									EXE_start (tdbb, temp, attachment->getSysTransaction());
									EXE_send (tdbb, temp, 0, 32, (UCHAR*) &jrd_58);
									while (1)
									   {
									   EXE_receive (tdbb, temp, 1, 2, (UCHAR*) &jrd_60);
									   if (!jrd_60.jrd_61) break;
									{
										/*RFR.RDB$UPDATE_FLAG*/
										jrd_65.jrd_101 = 1;
									}
									/*END_FOR*/
									   }
									}
								}
							}
						/*END_MODIFY*/
						jrd_104.jrd_105 = jrd_65.jrd_102;
						jrd_104.jrd_106 = jrd_65.jrd_103;
						jrd_104.jrd_107 = jrd_65.jrd_100;
						jrd_104.jrd_108 = jrd_65.jrd_101;
						jrd_104.jrd_109 = jrd_65.jrd_98;
						jrd_104.jrd_110 = jrd_65.jrd_99;
						jrd_104.jrd_111 = jrd_65.jrd_96;
						jrd_104.jrd_112 = jrd_65.jrd_97;
						EXE_send (tdbb, request_fmtx, 2, 16, (UCHAR*) &jrd_104);
						}
					}

					// Store stuff in field summary

					n = /*RFR.RDB$FIELD_ID*/
					    jrd_65.jrd_103;
					put_summary_record(tdbb, blob, RSR_field_id, (UCHAR*)&n, sizeof(n));
					put_summary_record(tdbb, blob, RSR_field_name, (UCHAR*) /*RFR.RDB$FIELD_NAME*/
												jrd_65.jrd_74,
									   fb_utils::name_length(/*RFR.RDB$FIELD_NAME*/
												 jrd_65.jrd_74));
					if (!/*FLD.RDB$COMPUTED_BLR*/
					     jrd_65.jrd_75.isEmpty() && !/*RFR.RDB$VIEW_CONTEXT*/
	       jrd_65.jrd_93)
					{
						put_summary_blob(tdbb, blob, RSR_computed_blr, &/*FLD.RDB$COMPUTED_BLR*/
												jrd_65.jrd_75, transaction);
					}
					else if (!null_view)
					{
						n = /*RFR.RDB$VIEW_CONTEXT*/
						    jrd_65.jrd_93;
						put_summary_record(tdbb, blob, RSR_view_context, (UCHAR*)&n, sizeof(n));
						put_summary_record(tdbb, blob, RSR_base_field, (UCHAR*) /*RFR.RDB$BASE_FIELD*/
													jrd_65.jrd_73,
										   fb_utils::name_length(/*RFR.RDB$BASE_FIELD*/
													 jrd_65.jrd_73));
					}
					put_summary_blob(tdbb, blob, RSR_missing_value, &/*FLD.RDB$MISSING_VALUE*/
											 jrd_65.jrd_72, transaction);

					bid* defaultValue = /*RFR.RDB$DEFAULT_VALUE*/
							    jrd_65.jrd_71.isEmpty() ?
						&/*FLD.RDB$DEFAULT_VALUE*/
						 jrd_65.jrd_70 : &/*RFR.RDB$DEFAULT_VALUE*/
    jrd_65.jrd_71;

					put_summary_blob(tdbb, blob, RSR_default_value, defaultValue, transaction);
					put_summary_blob(tdbb, blob, RSR_validation_blr, &/*FLD.RDB$VALIDATION_BLR*/
											  jrd_65.jrd_69, transaction);

					bool notNull = (/*RFR.RDB$NULL_FLAG.NULL*/
							jrd_65.jrd_96 ?
						(/*FLD.RDB$NULL_FLAG.NULL*/
						 jrd_65.jrd_94 ? false : (bool) /*FLD.RDB$NULL_FLAG*/
		  jrd_65.jrd_95) :
						(bool) /*RFR.RDB$NULL_FLAG*/
						       jrd_65.jrd_97);

					if (notNull)
					{
						put_summary_record(tdbb, blob, RSR_field_not_null,
											nonnull_validation_blr, sizeof(nonnull_validation_blr));
					}

					n = fb_utils::name_length(/*RFR.RDB$SECURITY_CLASS*/
								  jrd_65.jrd_68);
					if (!/*RFR.RDB$SECURITY_CLASS.NULL*/
					     jrd_65.jrd_92 && n)
					{
						put_summary_record(tdbb, blob, RSR_security_class,
										   (UCHAR*) /*RFR.RDB$SECURITY_CLASS*/
											    jrd_65.jrd_68, n);
					}

					n = fb_utils::name_length(/*RFR.RDB$GENERATOR_NAME*/
								  jrd_65.jrd_67);
					if (!/*RFR.RDB$GENERATOR_NAME.NULL*/
					     jrd_65.jrd_91 && n)
					{
						put_summary_record(tdbb, blob, RSR_field_generator_name,
										   (UCHAR*) /*RFR.RDB$GENERATOR_NAME*/
											    jrd_65.jrd_67, n);
					}

					// Make a temporary field block

					TemporaryField* tfb = FB_NEW(*tdbb->getDefaultPool()) TemporaryField;
					tfb->tfb_next = stack;
					stack = tfb;

					memset(&tfb->tfb_default, 0, sizeof(tfb->tfb_default));

					if (notNull && !defaultValue->isEmpty())
					{
						Jrd::ContextPoolHolder context(tdbb, attachment->createPool());
						JrdStatement* defaultStatement = NULL;
						try
						{
							ValueExprNode* defaultNode = static_cast<ValueExprNode*>(MET_parse_blob(
								tdbb, relation, defaultValue, NULL, &defaultStatement, false, false));

							jrd_req* const defaultRequest = defaultStatement->findRequest(tdbb);

							// Attention: this is scoped to the end of this "try".
							AutoSetRestore2<jrd_req*, thread_db> autoRequest(tdbb,
								&thread_db::getRequest, &thread_db::setRequest, defaultRequest);

							defaultRequest->req_timestamp.validate();

							TRA_attach_request(transaction, defaultRequest);
							dsc* result = EVL_expr(tdbb, defaultRequest, defaultNode);
							TRA_detach_request(defaultRequest);

							if (result)
							{
								dsc desc = *result;
								MoveBuffer buffer;

								if (desc.isText() || desc.isBlob())
								{
									UCHAR* ptr = NULL;
									const int len = MOV_make_string2(tdbb, &desc, CS_NONE, &ptr, buffer, true);
									fb_assert(ULONG(len) < ULONG(MAX_USHORT));
									desc.makeText(len, ttype_none, ptr);
								}

								EVL_make_value(tdbb, &desc, &tfb->tfb_default, dbb->dbb_permanent);
							}
						}
						catch (const Exception&)
						{
							if (defaultStatement)
								defaultStatement->release(tdbb);
							throw;
						}

						defaultStatement->release(tdbb);
					}

					// for text data types, grab the CHARACTER_SET and
					// COLLATION to give the type of international text

					if (/*FLD.RDB$CHARACTER_SET_ID.NULL*/
					    jrd_65.jrd_89)
						/*FLD.RDB$CHARACTER_SET_ID*/
						jrd_65.jrd_90 = CS_NONE;

					SSHORT collation = COLLATE_NONE;	// codepoint collation
					if (!/*FLD.RDB$COLLATION_ID.NULL*/
					     jrd_65.jrd_87)
						collation = /*FLD.RDB$COLLATION_ID*/
							    jrd_65.jrd_88;
					if (!/*RFR.RDB$COLLATION_ID.NULL*/
					     jrd_65.jrd_85)
						collation = /*RFR.RDB$COLLATION_ID*/
							    jrd_65.jrd_86;

					if (!DSC_make_descriptor(&tfb->tfb_desc, /*FLD.RDB$FIELD_TYPE*/
										 jrd_65.jrd_84,
											 /*FLD.RDB$FIELD_SCALE*/
											 jrd_65.jrd_83,
											 /*FLD.RDB$FIELD_LENGTH*/
											 jrd_65.jrd_82,
											 /*FLD.RDB$FIELD_SUB_TYPE*/
											 jrd_65.jrd_81,
											 /*FLD.RDB$CHARACTER_SET_ID*/
											 jrd_65.jrd_90, collation))
					{
						if (/*REL.RDB$FORMAT.NULL*/
						    jrd_118.jrd_125)
							DPM_delete_relation(tdbb, relation);

						ERR_post(Arg::Gds(isc_no_meta_update) <<
								 Arg::Gds(isc_random) << Arg::Str(work->dfw_name));
					}

					// Make sure the text type specified is implemented
					if (!validate_text_type(tdbb, tfb))
					{
						if (/*REL.RDB$FORMAT.NULL*/
						    jrd_118.jrd_125)
							DPM_delete_relation(tdbb, relation);

						ERR_post_nothrow(Arg::Gds(isc_no_meta_update) <<
										 Arg::Gds(isc_random) << Arg::Str(work->dfw_name));
						INTL_texttype_lookup(tdbb,
							(DTYPE_IS_TEXT(tfb->tfb_desc.dsc_dtype) ?
								tfb->tfb_desc.dsc_ttype() : tfb->tfb_desc.dsc_blob_ttype()));	// should punt
						ERR_punt(); // if INTL_texttype_lookup hasn't punt
					}

					// dimitr: view fields shouldn't be marked as computed
					if (null_view && !/*FLD.RDB$COMPUTED_BLR*/
							  jrd_65.jrd_75.isEmpty())
						tfb->tfb_flags |= TFB_computed;
					else
						++physical_fields;

					tfb->tfb_id = /*RFR.RDB$FIELD_ID*/
						      jrd_65.jrd_103;

					if ((n = /*FLD.RDB$DIMENSIONS*/
						 jrd_65.jrd_80))
						setup_array(tdbb, blob, /*FLD.RDB$FIELD_NAME*/
									jrd_65.jrd_66, n, tfb);

					if (external_flag)
					{
						tfb = FB_NEW(*tdbb->getDefaultPool()) TemporaryField;
						tfb->tfb_next = external;
						external = tfb;
						fb_assert(/*FLD.RDB$EXTERNAL_TYPE*/
							  jrd_65.jrd_79 <= MAX_UCHAR);
						tfb->tfb_desc.dsc_dtype = (UCHAR)/*FLD.RDB$EXTERNAL_TYPE*/
										 jrd_65.jrd_79;
						fb_assert(/*FLD.RDB$EXTERNAL_SCALE*/
							  jrd_65.jrd_78 >= MIN_SCHAR &&
							/*FLD.RDB$EXTERNAL_SCALE*/
							jrd_65.jrd_78 <= MAX_SCHAR);
						tfb->tfb_desc.dsc_scale = (SCHAR)/*FLD.RDB$EXTERNAL_SCALE*/
										 jrd_65.jrd_78;
						tfb->tfb_desc.dsc_length = /*FLD.RDB$EXTERNAL_LENGTH*/
									   jrd_65.jrd_77;
						tfb->tfb_id = /*RFR.RDB$FIELD_ID*/
							      jrd_65.jrd_103;
					}
				}
				/*END_FOR*/
				   EXE_send (tdbb, request_fmtx, 3, 2, (UCHAR*) &jrd_113);
				   }
				}

				if (null_view && !physical_fields)
				{
					if (/*REL.RDB$FORMAT.NULL*/
					    jrd_118.jrd_125)
						DPM_delete_relation(tdbb, relation);

					ERR_post(Arg::Gds(isc_no_meta_update) <<
							 Arg::Gds(isc_table_name) << Arg::Str(work->dfw_name) <<
			 				 Arg::Gds(isc_must_have_phys_field));
				}

				blob = setup_triggers(tdbb, relation, null_view, triggers, blob);

				blob->BLB_close(tdbb);
				USHORT version = /*REL.RDB$FORMAT.NULL*/
						 jrd_118.jrd_125 ? 0 : /*REL.RDB$FORMAT*/
       jrd_118.jrd_127;
				version++;
				relation->rel_current_format = make_format(tdbb, relation, &version, stack);
				/*REL.RDB$FORMAT.NULL*/
				jrd_118.jrd_125 = FALSE;
				/*REL.RDB$FORMAT*/
				jrd_118.jrd_127 = version;

				if (!null_view)
				{
					// update the dbkey length to include each of the base relations

					/*REL.RDB$DBKEY_LENGTH*/
					jrd_118.jrd_124 = 0;

					AutoRequest handle;
					/*FOR(REQUEST_HANDLE handle)
						Z IN RDB$VIEW_RELATIONS
						CROSS R IN RDB$RELATIONS OVER RDB$RELATION_NAME
						WITH Z.RDB$VIEW_NAME = work->dfw_name.c_str()*/
					{
					handle.compile(tdbb, (UCHAR*) jrd_51, sizeof(jrd_51));
					gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_52.jrd_53, 32);
					EXE_start (tdbb, handle, attachment->getSysTransaction());
					EXE_send (tdbb, handle, 0, 32, (UCHAR*) &jrd_52);
					while (1)
					   {
					   EXE_receive (tdbb, handle, 1, 4, (UCHAR*) &jrd_54);
					   if (!jrd_54.jrd_55) break;
					{
						/*REL.RDB$DBKEY_LENGTH*/
						jrd_118.jrd_124 += /*R.RDB$DBKEY_LENGTH*/
    jrd_54.jrd_56;
					}
					/*END_FOR*/
					   }
					}
				}
			/*END_MODIFY*/
			jrd_129.jrd_130 = jrd_118.jrd_120;
			gds__vtov((const char*) jrd_118.jrd_119, (char*) jrd_129.jrd_131, 32);
			jrd_129.jrd_132 = jrd_118.jrd_125;
			jrd_129.jrd_133 = jrd_118.jrd_127;
			jrd_129.jrd_134 = jrd_118.jrd_126;
			jrd_129.jrd_135 = jrd_118.jrd_124;
			EXE_send (tdbb, request_fmt1, 2, 48, (UCHAR*) &jrd_129);
			}
		}
		/*END_FOR*/
		   EXE_send (tdbb, request_fmt1, 3, 2, (UCHAR*) &jrd_136);
		   }
		}

		// If we didn't find the relation, it is probably being dropped

		if (!relation)
			return false;

		if (!(relation->rel_flags & REL_sys_trigs_being_loaded))
		    load_trigs(tdbb, relation, triggers);

		// in case somebody changed the view definition or a computed
		// field, reset the dependencies by deleting the current ones
		// and setting a flag for MET_scan_relation to find the new ones

		if (!null_view)
			MET_delete_dependencies(tdbb, work->dfw_name, obj_view, transaction);

		{ // begin scope
			const DeferredWork* arg = work->findArg(dfw_arg_force_computed);
			if (arg)
			{
				computed_field = true;
				MET_delete_dependencies(tdbb, arg->dfw_name, obj_computed, transaction);
			}
		} // end scope

		if (!null_view || computed_field)
			relation->rel_flags |= REL_get_dependencies;

		if (external_flag)
		{
			AutoRequest temp;
			/*FOR(REQUEST_HANDLE temp) FMTS IN RDB$FORMATS WITH
				FMTS.RDB$RELATION_ID EQ relation->rel_id AND
				FMTS.RDB$FORMAT EQ 0*/
			{
			temp.compile(tdbb, (UCHAR*) jrd_42, sizeof(jrd_42));
			jrd_43.jrd_44 = relation->rel_id;
			EXE_start (tdbb, temp, attachment->getSysTransaction());
			EXE_send (tdbb, temp, 0, 2, (UCHAR*) &jrd_43);
			while (1)
			   {
			   EXE_receive (tdbb, temp, 1, 2, (UCHAR*) &jrd_45);
			   if (!jrd_45.jrd_46) break;
			{
				/*ERASE FMTS;*/
				EXE_send (tdbb, temp, 2, 2, (UCHAR*) &jrd_47);
			}
			/*END_FOR*/
			   EXE_send (tdbb, temp, 3, 2, (UCHAR*) &jrd_49);
			   }
			}

			make_format(tdbb, relation, 0, external);
		}

		relation->rel_flags &= ~REL_scanned;
		DFW_post_work(transaction, dfw_scan_relation, NULL, relation->rel_id);

		// signal others about new format presence
		LCK_lock(tdbb, relation->rel_rescan_lock, LCK_EX, LCK_WAIT);
		LCK_release(tdbb, relation->rel_rescan_lock);

		break;
	}

	return false;
}


static bool modify_trigger(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_41;	// gds__utility 
   } jrd_40;
   struct {
          SSHORT jrd_38;	// gds__null_flag 
          SSHORT jrd_39;	// RDB$VALID_BLR 
   } jrd_37;
   struct {
          SSHORT jrd_34;	// gds__utility 
          SSHORT jrd_35;	// gds__null_flag 
          SSHORT jrd_36;	// RDB$VALID_BLR 
   } jrd_33;
   struct {
          TEXT  jrd_32 [32];	// RDB$TRIGGER_NAME 
   } jrd_31;
/**************************************
 *
 *	m o d i f y _ t r i g g e r
 *
 **************************************
 *
 * Functional description
 *	Perform required actions when modifying trigger.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Jrd::Attachment* attachment = tdbb->getAttachment();

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		{
			bool compile = !work->findArg(dfw_arg_check_blr);

			// get rid of old dependencies, bring in the new
			MET_delete_dependencies(tdbb, work->dfw_name, obj_trigger, transaction);
			get_trigger_dependencies(work, compile, transaction);
		}
		return true;

	case 4:
		{
			const DeferredWork* arg = work->findArg(dfw_arg_rel_name);
			if (!arg)
			{
				arg = work->findArg(dfw_arg_trg_type);
				fb_assert(arg);

				// ASF: arg->dfw_id is RDB$TRIGGER_TYPE truncated to USHORT
				if (arg && (arg->dfw_id & TRIGGER_TYPE_MASK) == TRIGGER_TYPE_DB)
				{
					MET_release_trigger(tdbb,
						&tdbb->getAttachment()->att_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB],
						work->dfw_name);

					MET_load_trigger(tdbb, NULL, work->dfw_name,
						&tdbb->getAttachment()->att_triggers[arg->dfw_id & ~TRIGGER_TYPE_DB]);
				}
			}
		}

		{ // scope
			const DeferredWork* arg = work->findArg(dfw_arg_check_blr);
			if (arg)
			{
				const Firebird::MetaName relation_name(arg->dfw_name);
				SSHORT valid_blr = FALSE;

				try
				{
					jrd_rel* relation = MET_lookup_relation(tdbb, relation_name);

					if (relation)
					{
						// remove cached triggers from relation
						relation->rel_flags &= ~REL_scanned;
						MET_scan_relation(tdbb, relation);

						trig_vec* triggers[TRIGGER_MAX];

						for (int i = 0; i < TRIGGER_MAX; ++i)
							triggers[i] = NULL;

						MemoryPool* new_pool = attachment->createPool();
						try
						{
							Jrd::ContextPoolHolder context(tdbb, new_pool);

							MET_load_trigger(tdbb, relation, work->dfw_name, triggers);

							for (int i = 0; i < TRIGGER_MAX; ++i)
							{
								if (triggers[i])
								{
									for (size_t j = 0; j < triggers[i]->getCount(); ++j)
										(*triggers[i])[j].compile(tdbb);

									MET_release_triggers(tdbb, &triggers[i]);
								}
							}

							valid_blr = TRUE;
						}
						catch (const Firebird::Exception&)
						{
							attachment->deletePool(new_pool);
							throw;
						}

						attachment->deletePool(new_pool);
					}
				}
				catch (const Firebird::Exception&)
				{
				}

				AutoCacheRequest request(tdbb, irq_trg_validate, IRQ_REQUESTS);

				/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE transaction)
					TRG IN RDB$TRIGGERS WITH
						TRG.RDB$TRIGGER_NAME EQ work->dfw_name.c_str() AND TRG.RDB$TRIGGER_BLR NOT MISSING*/
				{
				request.compile(tdbb, (UCHAR*) jrd_30, sizeof(jrd_30));
				gds__vtov ((const char*) work->dfw_name.c_str(), (char*) jrd_31.jrd_32, 32);
				EXE_start (tdbb, request, transaction);
				EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_31);
				while (1)
				   {
				   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_33);
				   if (!jrd_33.jrd_34) break;
				{
					/*MODIFY TRG USING*/
					{
					
						/*TRG.RDB$VALID_BLR*/
						jrd_33.jrd_36 = valid_blr;
						/*TRG.RDB$VALID_BLR.NULL*/
						jrd_33.jrd_35 = FALSE;
					/*END_MODIFY*/
					jrd_37.jrd_38 = jrd_33.jrd_35;
					jrd_37.jrd_39 = jrd_33.jrd_36;
					EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_37);
					}
				}
				/*END_FOR*/
				   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_40);
				   }
				}
			}
		} // scope
		break;
	}

	return false;
}


static void put_summary_blob(thread_db* tdbb, blb* blob, rsr_t type, bid* blob_id, jrd_tra* transaction)
{
/**************************************
 *
 *	p u t _ s u m m a r y _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Put an attribute record to the relation summary blob.
 *
 **************************************/

	SET_TDBB(tdbb);

	if (blob_id->isEmpty())	// If blob is null, don't bother.
		return;

	// Go ahead and open blob
	blb* blr = blb::open(tdbb, transaction, blob_id);
	fb_assert(blr->blb_length <= MAX_USHORT);

	USHORT length = (USHORT) blr->blb_length;
	HalfStaticArray<UCHAR, 128> buffer;

	length = (USHORT) blr->BLB_get_data(tdbb, buffer.getBuffer(length), (SLONG) length);
	put_summary_record(tdbb, blob, type, buffer.begin(), length);
}


static Lock* protect_relation(thread_db* tdbb, jrd_tra* transaction, jrd_rel* relation,
	bool& releaseLock)
{
/**************************************
 *
 * p r o t e c t _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Lock relation with protected_read level or raise existing relation lock
 * to this level to ensure nobody can write to this relation.
 * Used when new index is built.
 * releaseLock set to true if there was no existing lock before
 *
 **************************************/
	Lock* relLock = RLCK_transaction_relation_lock(tdbb, transaction, relation);

	releaseLock = (relLock->lck_logical == LCK_none);

	bool inUse = false;

	if (!releaseLock) {
		if ( (relLock->lck_logical < LCK_PR) &&
			!LCK_convert(tdbb, relLock, LCK_PR, transaction->getLockWait()) )
		{
			inUse = true;
		}
	}
	else {
		if (!LCK_lock(tdbb, relLock, LCK_PR, transaction->getLockWait()))
			inUse = true;
	}

	if (inUse)
		raiseRelationInUseError(relation);

	return relLock;
}


static void put_summary_record(thread_db* tdbb,
							   blb*		blob,
							   rsr_t	type,
							   const UCHAR*	data,
							   USHORT	length)
{
/**************************************
 *
 *	p u t _ s u m m a r y _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Put an attribute record to the relation summary blob.
 *
 **************************************/

	SET_TDBB(tdbb);

	fb_assert(length < MAX_USHORT); // otherwise length + 1 wraps. Or do we bugcheck???
	UCHAR temp[129];

	UCHAR* const buffer = ((size_t) (length + 1) > sizeof(temp)) ?
		FB_NEW(*getDefaultMemoryPool()) UCHAR[length + 1] : temp;

	UCHAR* p = buffer;
	*p++ = (UCHAR) type;
	memcpy(p, data, length);

	try {
		blob->BLB_put_segment(tdbb, buffer, length + 1);
	}
	catch (const Firebird::Exception&)
	{
		if (buffer != temp)
			delete[] buffer;
		throw;
	}

	if (buffer != temp)
		delete[] buffer;
}


static void release_protect_lock(thread_db* tdbb, jrd_tra*	transaction, Lock* relLock)
{
	vec<Lock*>* vector = transaction->tra_relation_locks;
	if (vector) {
		vec<Lock*>::iterator lock = vector->begin();
		for (ULONG i = 0; i < vector->count(); ++i, ++lock)
		{
			if (*lock == relLock)
			{
				LCK_release(tdbb, relLock);
				*lock = 0;
				break;
			}
		}
	}
}


static bool scan_relation(thread_db* tdbb, SSHORT phase, DeferredWork* work, jrd_tra*)
{
/**************************************
 *
 *	s c a n _ r e l a t i o n
 *
 **************************************
 *
 * Functional description
 *	Call MET_scan_relation with the appropriate
 *	relation.
 *
 **************************************/

	SET_TDBB(tdbb);

	switch (phase)
	{
	case 1:
	case 2:
		return true;

	case 3:
		// dimitr:	I suspect that nobody expects an updated format to
		//			appear at stage 3, so the logic would work reliably
		//			if this line is removed (and hence we rely on the
		//			4th stage only). But I leave it here for the time being.
		MET_scan_relation(tdbb, MET_relation(tdbb, work->dfw_id));
		return true;

	case 4:
		MET_scan_relation(tdbb, MET_relation(tdbb, work->dfw_id));
		break;
	}

	return false;
}

#ifdef NOT_USED_OR_REPLACED
static bool shadow_defined(thread_db* tdbb)
{
   struct {
          SSHORT jrd_29;	// gds__utility 
   } jrd_28;
/**************************************
 *
 *	s h a d o w _ d e f i n e d
 *
 **************************************
 *
 * Functional description
 *	Return true if any shadows have been has been defined
 *      for the database else return false.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	bool result = false;
	AutoRequest handle;

	/*FOR(REQUEST_HANDLE handle) FIRST 1 X IN RDB$FILES
		WITH X.RDB$SHADOW_NUMBER > 0*/
	{
	handle.compile(tdbb, (UCHAR*) jrd_27, sizeof(jrd_27));
	EXE_start (tdbb, handle, attachment->getSysTransaction());
	while (1)
	   {
	   EXE_receive (tdbb, handle, 0, 2, (UCHAR*) &jrd_28);
	   if (!jrd_28.jrd_29) break;
	{
		result = true;
	}
	/*END_FOR*/
	   }
	}

	return result;
}
#endif


static void setup_array(thread_db* tdbb, blb* blob, const TEXT* field_name, USHORT n,
	TemporaryField* tfb)
{
/**************************************
 *
 *	s e t u p _ a r r a y
 *
 **************************************
 *
 * Functional description
 *
 *	setup an array descriptor in a tfb
 *
 **************************************/

	SLONG stuff[256];

	put_summary_record(tdbb, blob, RSR_dimensions, (UCHAR*) &n, sizeof(n));
	tfb->tfb_flags |= TFB_array;
	Ods::InternalArrayDesc* array = reinterpret_cast<Ods::InternalArrayDesc*>(stuff);
	MOVE_CLEAR(array, (SLONG) sizeof(Ods::InternalArrayDesc));
	array->iad_dimensions = n;
	array->iad_struct_count = 1;
	array->iad_rpt[0].iad_desc = tfb->tfb_desc;
	get_array_desc(tdbb, field_name, array);
	put_summary_record(tdbb, blob, RSR_array_desc, (UCHAR*) array, array->iad_length);
}


static blb* setup_triggers(thread_db* tdbb, jrd_rel* relation, bool null_view,
	trig_vec** triggers, blb* blob)
{
   struct {
          TEXT  jrd_8 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_9;	// gds__utility 
          SSHORT jrd_10;	// RDB$TRIGGER_INACTIVE 
   } jrd_7;
   struct {
          TEXT  jrd_2 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_3 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_4 [12];	// RDB$CONSTRAINT_TYPE 
          SSHORT jrd_5;	// RDB$SYSTEM_FLAG 
          SSHORT jrd_6;	// RDB$SYSTEM_FLAG 
   } jrd_1;
   struct {
          TEXT  jrd_17 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_18;	// gds__utility 
          SSHORT jrd_19;	// RDB$TRIGGER_INACTIVE 
   } jrd_16;
   struct {
          TEXT  jrd_13 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_14 [12];	// RDB$CONSTRAINT_TYPE 
          TEXT  jrd_15 [12];	// RDB$CONSTRAINT_TYPE 
   } jrd_12;
   struct {
          TEXT  jrd_24 [32];	// RDB$TRIGGER_NAME 
          SSHORT jrd_25;	// gds__utility 
          SSHORT jrd_26;	// RDB$TRIGGER_INACTIVE 
   } jrd_23;
   struct {
          TEXT  jrd_22 [32];	// RDB$RELATION_NAME 
   } jrd_21;
/**************************************
 *
 *	s e t u p _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *
 *	Get the triggers in the right order, which appears
 *      to be system triggers first, then user triggers,
 *      then triggers that implement check constraints.
 *
 * BUG #8458: Check constraint triggers have to be loaded
 *	      (and hence executed) after the user-defined
 *	      triggers because user-defined triggers can modify
 *	      the values being inserted or updated so that
 *	      the end values stored in the database don't
 *	      fulfill the check constraint.
 *
 **************************************/
	if (!relation)
		return blob;

	Jrd::Attachment* attachment = tdbb->getAttachment();

	// system triggers

	AutoCacheRequest request_fmtx(tdbb, irq_format4, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
			AND TRG.RDB$SYSTEM_FLAG = 1
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	request_fmtx.compile(tdbb, (UCHAR*) jrd_20, sizeof(jrd_20));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_21.jrd_22, 32);
	EXE_start (tdbb, request_fmtx, attachment->getSysTransaction());
	EXE_send (tdbb, request_fmtx, 0, 32, (UCHAR*) &jrd_21);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_23);
	   if (!jrd_23.jrd_25) break;
	{
		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_23.jrd_26)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_23.jrd_24, null_view);
	}
	/*END_FOR*/
	   }
	}

	// user triggers

	request_fmtx.reset(tdbb, irq_format5, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME EQ relation->rel_name.c_str()
		AND TRG.RDB$SYSTEM_FLAG = 0
		AND (NOT ANY
			CHK IN RDB$CHECK_CONSTRAINTS CROSS
			RCN IN RDB$RELATION_CONSTRAINTS
			WITH TRG.RDB$TRIGGER_NAME EQ CHK.RDB$TRIGGER_NAME
			AND CHK.RDB$CONSTRAINT_NAME EQ RCN.RDB$CONSTRAINT_NAME
			AND (RCN.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT
			    OR RCN.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
			)
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	request_fmtx.compile(tdbb, (UCHAR*) jrd_11, sizeof(jrd_11));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_12.jrd_13, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_12.jrd_14, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_12.jrd_15, 12);
	EXE_start (tdbb, request_fmtx, attachment->getSysTransaction());
	EXE_send (tdbb, request_fmtx, 0, 56, (UCHAR*) &jrd_12);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_16);
	   if (!jrd_16.jrd_18) break;
	{
		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_16.jrd_19)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_16.jrd_17, null_view);
	}
	/*END_FOR*/
	   }
	}

	// check constraint triggers.  We're looking for triggers that belong
	// to the table and are system triggers (i.e. system flag in (3, 4, 5))
	// or a user looking trigger that's involved in a check constraint

	request_fmtx.reset(tdbb, irq_format6, IRQ_REQUESTS);

	/*FOR (REQUEST_HANDLE request_fmtx)
		TRG IN RDB$TRIGGERS
		WITH TRG.RDB$RELATION_NAME = relation->rel_name.c_str()
		AND (TRG.RDB$SYSTEM_FLAG BT fb_sysflag_check_constraint AND fb_sysflag_view_check
			OR (TRG.RDB$SYSTEM_FLAG = 0 AND ANY
			CHK IN RDB$CHECK_CONSTRAINTS CROSS
			RCN IN RDB$RELATION_CONSTRAINTS
				WITH TRG.RDB$TRIGGER_NAME EQ CHK.RDB$TRIGGER_NAME
				AND CHK.RDB$CONSTRAINT_NAME EQ RCN.RDB$CONSTRAINT_NAME
				AND (RCN.RDB$CONSTRAINT_TYPE EQ CHECK_CNSTRT
				    OR RCN.RDB$CONSTRAINT_TYPE EQ FOREIGN_KEY)
		    	)
			)
		SORTED BY TRG.RDB$TRIGGER_SEQUENCE*/
	{
	request_fmtx.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_1.jrd_2, 32);
	gds__vtov ((const char*) FOREIGN_KEY, (char*) jrd_1.jrd_3, 12);
	gds__vtov ((const char*) CHECK_CNSTRT, (char*) jrd_1.jrd_4, 12);
	jrd_1.jrd_5 = fb_sysflag_view_check;
	jrd_1.jrd_6 = fb_sysflag_check_constraint;
	EXE_start (tdbb, request_fmtx, attachment->getSysTransaction());
	EXE_send (tdbb, request_fmtx, 0, 60, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request_fmtx, 1, 36, (UCHAR*) &jrd_7);
	   if (!jrd_7.jrd_9) break;
	{
		if (!/*TRG.RDB$TRIGGER_INACTIVE*/
		     jrd_7.jrd_10)
			setup_trigger_details(tdbb, relation, blob, triggers, /*TRG.RDB$TRIGGER_NAME*/
									      jrd_7.jrd_8, null_view);
	}
	/*END_FOR*/
	   }
	}

	return blob;
}


static void setup_trigger_details(thread_db* tdbb,
								  jrd_rel* relation,
								  blb* blob,
								  trig_vec** triggers,
								  const TEXT* trigger_name,
								  bool null_view)
{
/**************************************
 *
 *	s e t u p _ t r i g g e r _ d e t a i l s
 *
 **************************************
 *
 * Functional description
 *	Stuff trigger details in places.
 *
 * for a view, load the trigger temporarily --
 * this is inefficient since it will just be reloaded
 *  in MET_scan_relation () but it needs to be done
 *  in case the view would otherwise be non-updatable
 *
 **************************************/

	put_summary_record(tdbb, blob, RSR_trigger_name,
		(const UCHAR*) trigger_name, fb_utils::name_length(trigger_name));

	if (!null_view) {
		MET_load_trigger(tdbb, relation, trigger_name, triggers);
	}
}


static bool validate_text_type(thread_db* tdbb, const TemporaryField* tfb)
{
/**************************************
 *
 *	v a l i d a t e _ t e x t _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Make sure the text type specified is implemented
 *
 **************************************/

	if ((DTYPE_IS_TEXT (tfb->tfb_desc.dsc_dtype) &&
		!INTL_defined_type(tdbb, tfb->tfb_desc.dsc_ttype())) ||
		(tfb->tfb_desc.dsc_dtype == dtype_blob && tfb->tfb_desc.dsc_sub_type == isc_blob_text &&
			!INTL_defined_type(tdbb, tfb->tfb_desc.dsc_blob_ttype())))
	{
		return false;
	}

	return true;
}


static string get_string(const dsc* desc)
{
/**************************************
 *
 *  g e t _ s t r i n g
 *
 **************************************
 *
 * Get string for a given descriptor.
 *
 **************************************/
	const char* str;
	VaryStr<MAXPATHLEN> temp;// Must hold largest metadata field or filename

	if (!desc)
	{
		return string();
	}

	// Find the actual length of the string, searching until the claimed
	// end of the string, or the terminating \0, whichever comes first.

	USHORT length = MOV_make_string(desc, ttype_metadata, &str, &temp, sizeof(temp));

	const char* p = str;
	const char* const q = str + length;
	while (p < q && *p)
	{
		++p;
	}

	// Trim trailing blanks (bug 3355)

	while (--p >= str && *p == ' ')
		;
	length = (p + 1) - str;

	return string(str, length);
}
