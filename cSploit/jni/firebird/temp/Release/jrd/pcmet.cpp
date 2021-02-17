/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pcmet.epp
 *	DESCRIPTION:	Meta data for expression indices
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
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/irq.h"
#include "../jrd/tra.h"
#include "../jrd/val.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/met.h"
#include "../jrd/lck.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/tra_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/pcmet_proto.h"

using namespace Jrd;
using namespace Firebird;

/*DATABASE DB = FILENAME "ODS.RDB";*/
static const UCHAR	jrd_0 [102] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 1, 2,0, 
      blr_quad, 0, 
      blr_short, 0, 
   blr_message, 0, 2,0, 
      blr_cstring2, 3,0, 32,0, 
      blr_short, 0, 
   blr_receive, 0, 
      blr_begin, 
         blr_for, 
            blr_rse, 1, 
               blr_rid, 4,0, 0, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 0, 1,0, 
                        blr_parameter, 0, 0,0, 
                     blr_eql, 
                        blr_fid, 0, 2,0, 
                        blr_add, 
                           blr_parameter, 0, 1,0, 
                           blr_literal, blr_long, 0, 1,0,0,0,
               blr_end, 
            blr_send, 1, 
               blr_begin, 
                  blr_assignment, 
                     blr_fid, 0, 10,0, 
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
static const UCHAR	jrd_7 [244] =
   {	// blr string 
blr_version4,
blr_begin, 
   blr_message, 3, 1,0, 
      blr_short, 0, 
   blr_message, 2, 2,0, 
      blr_short, 0, 
      blr_short, 0, 
   blr_message, 1, 12,0, 
      blr_quad, 0, 
      blr_double, 
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
            blr_rse, 2, 
               blr_rid, 4,0, 0, 
               blr_rid, 6,0, 1, 
               blr_boolean, 
                  blr_and, 
                     blr_eql, 
                        blr_fid, 1, 8,0, 
                        blr_fid, 0, 1,0, 
                     blr_and, 
                        blr_not, 
                           blr_missing, 
                              blr_fid, 0, 10,0, 
                        blr_eql, 
                           blr_fid, 0, 0,0, 
                           blr_parameter, 0, 0,0, 
               blr_end, 
            blr_begin, 
               blr_send, 1, 
                  blr_begin, 
                     blr_assignment, 
                        blr_fid, 0, 10,0, 
                        blr_parameter2, 1, 0,0, 4,0, 
                     blr_assignment, 
                        blr_fid, 0, 12,0, 
                        blr_parameter, 1, 1,0, 
                     blr_assignment, 
                        blr_fid, 1, 8,0, 
                        blr_parameter, 1, 2,0, 
                     blr_assignment, 
                        blr_literal, blr_long, 0, 1,0,0,0,
                        blr_parameter, 1, 3,0, 
                     blr_assignment, 
                        blr_fid, 0, 7,0, 
                        blr_parameter, 1, 5,0, 
                     blr_assignment, 
                        blr_fid, 0, 3,0, 
                        blr_parameter, 1, 6,0, 
                     blr_assignment, 
                        blr_fid, 0, 5,0, 
                        blr_parameter, 1, 7,0, 
                     blr_assignment, 
                        blr_fid, 0, 6,0, 
                        blr_parameter, 1, 8,0, 
                     blr_assignment, 
                        blr_fid, 0, 2,0, 
                        blr_parameter2, 1, 10,0, 9,0, 
                     blr_assignment, 
                        blr_fid, 1, 3,0, 
                        blr_parameter, 1, 11,0, 
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
               blr_parameter, 1, 3,0, 
         blr_end, 
   blr_end, 
blr_eoc

   };	// end of blr string 



void PCMET_expression_index(thread_db* tdbb, const MetaName& name, USHORT id, jrd_tra* transaction)
{
   struct {
          SSHORT jrd_27;	// gds__utility 
   } jrd_26;
   struct {
          SSHORT jrd_24;	// gds__null_flag 
          SSHORT jrd_25;	// RDB$INDEX_ID 
   } jrd_23;
   struct {
          bid  jrd_11;	// RDB$EXPRESSION_BLR 
          double  jrd_12;	// RDB$STATISTICS 
          TEXT  jrd_13 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_14;	// gds__utility 
          SSHORT jrd_15;	// gds__null_flag 
          SSHORT jrd_16;	// RDB$INDEX_TYPE 
          SSHORT jrd_17;	// RDB$UNIQUE_FLAG 
          SSHORT jrd_18;	// RDB$SEGMENT_COUNT 
          SSHORT jrd_19;	// RDB$INDEX_INACTIVE 
          SSHORT jrd_20;	// gds__null_flag 
          SSHORT jrd_21;	// RDB$INDEX_ID 
          SSHORT jrd_22;	// RDB$RELATION_ID 
   } jrd_10;
   struct {
          TEXT  jrd_9 [32];	// RDB$INDEX_NAME 
   } jrd_8;
/**************************************
 *
 *	P C M E T _ e x p r e s s i o n _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Create a new expression index.
 *
 **************************************/
	jrd_rel* relation = NULL;
	index_desc idx;
	MemoryPool* new_pool = NULL;

	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();
#ifdef DEV_BUILD
	Database* dbb = tdbb->getDatabase();
#endif

	MOVE_CLEAR(&idx, sizeof(index_desc));

	AutoCacheRequest request(tdbb, irq_c_exp_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		IDX IN RDB$INDICES CROSS
			REL IN RDB$RELATIONS OVER RDB$RELATION_NAME WITH
			IDX.RDB$EXPRESSION_BLR NOT MISSING AND
			IDX.RDB$INDEX_NAME EQ name.c_str()*/
	{
	request.compile(tdbb, (UCHAR*) jrd_7, sizeof(jrd_7));
	gds__vtov ((const char*) name.c_str(), (char*) jrd_8.jrd_9, 32);
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_8);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 66, (UCHAR*) &jrd_10);
	   if (!jrd_10.jrd_14) break;
	{
		if (!relation)
		{
			relation = MET_relation(tdbb, /*REL.RDB$RELATION_ID*/
						      jrd_10.jrd_22);
			if (relation->rel_name.length() == 0) {
				relation->rel_name = /*REL.RDB$RELATION_NAME*/
						     jrd_10.jrd_13;
			}

			if (/*IDX.RDB$INDEX_ID*/
			    jrd_10.jrd_21 && /*IDX.RDB$STATISTICS*/
    jrd_10.jrd_12 < 0.0)
			{
				SelectivityList selectivity(*tdbb->getDefaultPool());
				const USHORT id = /*IDX.RDB$INDEX_ID*/
						  jrd_10.jrd_21 - 1;
				IDX_statistics(tdbb, relation, id, selectivity);
				DFW_update_index(name.c_str(), id, selectivity, transaction);

				return;
			}

			if (/*IDX.RDB$INDEX_ID*/
			    jrd_10.jrd_21)
			{
				IDX_delete_index(tdbb, relation, /*IDX.RDB$INDEX_ID*/
								 jrd_10.jrd_21 - 1);
				MET_delete_dependencies(tdbb, name, obj_expression_index, transaction);
				/*MODIFY IDX*/
				{
				
					/*IDX.RDB$INDEX_ID.NULL*/
					jrd_10.jrd_20 = TRUE;
				/*END_MODIFY*/
				jrd_23.jrd_24 = jrd_10.jrd_20;
				jrd_23.jrd_25 = jrd_10.jrd_21;
				EXE_send (tdbb, request, 2, 4, (UCHAR*) &jrd_23);
				}
			}

			if (/*IDX.RDB$INDEX_INACTIVE*/
			    jrd_10.jrd_19)
				return;

			if (/*IDX.RDB$SEGMENT_COUNT*/
			    jrd_10.jrd_18)
			{
				// Msg359: segments not allowed in expression index %s
				ERR_post(Arg::Gds(isc_no_meta_update) <<
						 Arg::Gds(isc_no_segments_err) << Arg::Str(name));
			}
			if (/*IDX.RDB$UNIQUE_FLAG*/
			    jrd_10.jrd_17)
				idx.idx_flags |= idx_unique;
			if (/*IDX.RDB$INDEX_TYPE*/
			    jrd_10.jrd_16 == 1)
				idx.idx_flags |= idx_descending;

			CompilerScratch* csb = 0;
			// allocate a new pool to contain the expression tree for the expression index
			new_pool = attachment->createPool();
			{ // scope
				Jrd::ContextPoolHolder context(tdbb, new_pool);
				MET_scan_relation(tdbb, relation);

				if (!/*IDX.RDB$EXPRESSION_BLR.NULL*/
				     jrd_10.jrd_15)
				{
					idx.idx_expression = static_cast<ValueExprNode*>(MET_get_dependencies(
						tdbb, relation, NULL, 0, NULL, &/*IDX.RDB$EXPRESSION_BLR*/
										jrd_10.jrd_11,
						&idx.idx_expression_statement, &csb, name, obj_expression_index, 0,
						transaction));
				}
			} // end scope

			// fake a description of the index

			idx.idx_count = 1;
			idx.idx_flags |= idx_expressn;
			idx.idx_expression->getDesc(tdbb, csb, &idx.idx_expression_desc);
			idx.idx_rpt[0].idx_itype =
				DFW_assign_index_type(tdbb, name,
									  idx.idx_expression_desc.dsc_dtype,
									  idx.idx_expression_desc.dsc_sub_type);
			idx.idx_rpt[0].idx_selectivity = 0;

			delete csb;
		}
	}
	/*END_FOR*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_26);
	   }
	}

	if (!relation)
	{
		// Msg308: can't create index %s
		ERR_post(Arg::Gds(isc_no_meta_update) <<
				 Arg::Gds(isc_idx_create_err) << Arg::Str(name));
	}

	// Actually create the index

	SelectivityList selectivity(*tdbb->getDefaultPool());

	jrd_tra* const current_transaction = tdbb->getTransaction();

	fb_assert(id <= dbb->dbb_max_idx);
	idx.idx_id = id;
	IDX_create_index(tdbb, relation, &idx, name.c_str(), &id, transaction, selectivity);

	fb_assert(id == idx.idx_id);

	tdbb->setTransaction(current_transaction);

	DFW_update_index(name.c_str(), idx.idx_id, selectivity, transaction);

	// Get rid of the pool containing the expression tree

	attachment->deletePool(new_pool);
}


void PCMET_lookup_index(thread_db* tdbb, jrd_rel* relation, index_desc* idx)
{
   struct {
          bid  jrd_5;	// RDB$EXPRESSION_BLR 
          SSHORT jrd_6;	// gds__utility 
   } jrd_4;
   struct {
          TEXT  jrd_2 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_3;	// RDB$INDEX_ID 
   } jrd_1;
/**************************************
 *
 *	P C M E T _ l o o k u p _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Lookup information about an index, in
 *	the metadata cache if possible.
 *
 **************************************/
	SET_TDBB(tdbb);
	Jrd::Attachment* attachment = tdbb->getAttachment();

	// Check the index blocks for the relation to see if we have a cached block

	IndexBlock* index_block;
	for (index_block = relation->rel_index_blocks; index_block; index_block = index_block->idb_next)
	{
		if (index_block->idb_id == idx->idx_id)
			break;
	}

	if (index_block && index_block->idb_expression)
	{
		idx->idx_expression = index_block->idb_expression;
		idx->idx_expression_statement = index_block->idb_expression_statement;
		memcpy(&idx->idx_expression_desc, &index_block->idb_expression_desc, sizeof(struct dsc));
		return;
	}

	if (!(relation->rel_flags & REL_scanned) || (relation->rel_flags & REL_being_scanned))
	{
		MET_scan_relation(tdbb, relation);
	}

	CompilerScratch* csb = NULL;
	AutoCacheRequest request(tdbb, irq_l_exp_index, IRQ_REQUESTS);

	/*FOR(REQUEST_HANDLE request)
		IDX IN RDB$INDICES WITH
			IDX.RDB$RELATION_NAME EQ relation->rel_name.c_str() AND
			IDX.RDB$INDEX_ID EQ idx->idx_id + 1*/
	{
	request.compile(tdbb, (UCHAR*) jrd_0, sizeof(jrd_0));
	gds__vtov ((const char*) relation->rel_name.c_str(), (char*) jrd_1.jrd_2, 32);
	jrd_1.jrd_3 = idx->idx_id;
	EXE_start (tdbb, request, attachment->getSysTransaction());
	EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_1);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 10, (UCHAR*) &jrd_4);
	   if (!jrd_4.jrd_6) break;
	{
		if (idx->idx_expression_statement)
		{
			idx->idx_expression_statement->release(tdbb);
			idx->idx_expression_statement = NULL;
		}

		// parse the blr, making sure to create the resulting expression
		// tree and request in its own pool so that it may be cached
		// with the index block in the "permanent" metadata cache

		{ // scope
			Jrd::ContextPoolHolder context(tdbb, attachment->createPool());
			idx->idx_expression = static_cast<ValueExprNode*>(MET_parse_blob(
				tdbb, relation, &/*IDX.RDB$EXPRESSION_BLR*/
						 jrd_4.jrd_5, &csb,
				&idx->idx_expression_statement, false, false));
		} // end scope
	}
	/*END_FOR*/
	   }
	}

	if (csb)
		idx->idx_expression->getDesc(tdbb, csb, &idx->idx_expression_desc);

	delete csb;

	// if there is no existing index block for this index, create
	// one and link it in with the index blocks for this relation

	if (!index_block)
		index_block = IDX_create_index_block(tdbb, relation, idx->idx_id);

	// if we can't get the lock, no big deal: just give up on caching the index info

	if (!LCK_lock(tdbb, index_block->idb_lock, LCK_SR, LCK_NO_WAIT))
	{
		// clear lock error from status vector
		fb_utils::init_status(tdbb->tdbb_status_vector);

		return;
	}

	// whether the index block already existed or was just created,
	// fill in the cached information about the index

	index_block->idb_expression = idx->idx_expression;
	index_block->idb_expression_statement = idx->idx_expression_statement;
	memcpy(&index_block->idb_expression_desc, &idx->idx_expression_desc, sizeof(struct dsc));
}
