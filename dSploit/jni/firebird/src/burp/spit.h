/*
 **************************************************************************
 *
 *	PROGRAM:		JRD file split utility program
 *	MODULE:			spit.h
 *	DESCRIPTION:	file split utility program main header file
 *
 *
 **************************************************************************
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

#include "../jrd/common.h"

const char BLANK				= ' ';
const int FILE_IS_FULL			= -9;
//const int FILE_NM_ARR_LEN		= 20;
//const char* GSPLIT_HDR_REC_NM	= "InterBase/Gsplit";

const int K_BYTES				= 1024;
const int IO_BUFFER_SIZE		= (16 * K_BYTES);

const int M_BYTES				= (K_BYTES * K_BYTES);
const int G_BYTES				= (K_BYTES * M_BYTES);
const size_t MAX_FILE_NM_LEN	= 27;	// size of header_rec.fl_name
const int MAX_NUM_OF_FILES		= 9999;
const int MIN_FILE_SIZE			= M_BYTES;
const char NEW_LINE				= '\n';
const char TERMINAL				= '\0';
