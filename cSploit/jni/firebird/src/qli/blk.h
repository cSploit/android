/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		blk.h
 *	DESCRIPTION:	Block type definitions
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

BLKDEF (type_frb, qli_frb, 0)
BLKDEF (type_hnk, qli_hnk, 0)
BLKDEF (type_plb, qli_plb, 0)
BLKDEF (type_vec, qli_vec, sizeof (((qli_vec*) NULL)->vec_object[0]))
BLKDEF (type_dbb, qli_dbb, 1)
BLKDEF (type_rel, qli_rel, 0)
BLKDEF (type_fld, qli_fld, 1)
BLKDEF (type_vcl, qli_vcl, sizeof (((qli_vcl*) NULL)->vcl_long[0]))
BLKDEF (type_req, qli_req, 0)			// Request block
BLKDEF (type_nod, qli_nod, sizeof(qli_nod*)) // sizeof (((qli_nod*) NULL)->nod_arg[0]))
BLKDEF (type_syn, qli_nod, sizeof (((qli_syntax*) NULL)->syn_arg[0]))
BLKDEF (type_lls, qli_lls, 0)			// linked list stack
BLKDEF (type_str, qli_str, 1)			// random string block
BLKDEF (type_tok, qli_tok, 1)			// token block
BLKDEF (type_sym, qli_symbol, 1)		// symbol block
BLKDEF (type_msg, qli_msg, 0)			// Message block
BLKDEF (type_nam, qli_name, 1)			// Name node
BLKDEF (type_ctx, qli_ctx, 0)			// Context block
BLKDEF (type_con, qli_const, 1)			// Constant block
BLKDEF (type_itm, qli_print_item, 0)	// Print item
BLKDEF (type_par, qli_par, 0)			// Parameter block
BLKDEF (type_line, qli_line, 1)			// Input line block
BLKDEF (type_brk, qli_brk, 0)
BLKDEF (type_rpt, qli_rpt, 0)
BLKDEF (type_pic, pics, 0)
BLKDEF (type_prt, qli_prt, 0)
BLKDEF (type_map, qli_map, 0)
BLKDEF (type_qpr, qli_proc, 0)
BLKDEF (type_qfn, qli_func, 0)
BLKDEF (type_qfl, qli_filter, 0)
BLKDEF (type_fun, qli_fun, 0) 			//, sizeof(dsc)) // sizeof (((FUN) NULL)->fun_arg[0]))
BLKDEF (type_rlb, qli_rlb, 0)			// Request language block
