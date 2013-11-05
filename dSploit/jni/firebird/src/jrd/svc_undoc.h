/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		svc_undoc.h
 *	DESCRIPTION:	Undocumented service items
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

#ifndef SVC_UNDOC_H
#define SVC_UNDOC_H

/* Service Action Flags */
const int isc_action_svc_lock_stats		= 13;	/* Retrieves lock manager statistics */
const int isc_action_svc_set_config		= 14;	/* Sets configuration file values */
const int isc_action_svc_default_config	= 15;	/* Sets the configuration options to their default values */
const int isc_action_svc_set_env		= 16;	/* Sets the value for $FIREBIRD */
const int isc_action_svc_set_env_lock	= 17;	/* Sets the value for $FIREBIRD_LOCK */
const int isc_action_svc_set_env_msg	= 18;	/* Sets the value for $FIREBIRD_MSG */


/* Service Info Flags */
const int isc_info_svc_total_length		= 69;
const int isc_info_svc_response			= 70;
const int isc_info_svc_response_more	= 71;
const int isc_info_svc_message			= 72;
const int isc_info_svc_svr_online		= 73;
const int isc_info_svc_svr_offline		= 74;
const int isc_info_svc_set_config		= 75;
const int isc_info_svc_default_config	= 76;
const int isc_info_svc_dump_pool_info	= 77;

// BRS: Is not strange the following parameters are undocumented
// it are also unused in the code
/********************************************
 * Parameters for isc_action_svc_set_config *
 * isc_info_svc_get_config                  *
 *                                          *
 *             UNDOCUMENTED                 *
 ********************************************/
//#define isc_spb_ibc_lock_mem_size     5
//#define isc_spb_ibc_lock_sem_count    6
//#define isc_spb_ibc_lock_signal       7
//#define isc_spb_ibc_event_mem_size    8
//#define isc_spb_ibc_db_cache_pages    9
//#define isc_spb_ibc_priority_class    10
//#define isc_spb_ibc_svr_map_size      11
//#define isc_spb_ibc_svr_min_work_set  12
//#define isc_spb_ibc_svr_max_work_set  13
//#define isc_spb_ibc_lock_grant_order  14
//#define isc_spb_ibc_lockhash          15
//#define isc_spb_ibc_deadlock          16
//#define isc_spb_ibc_lockspin          17
//#define isc_spb_ibc_conn_timeout      18
//#define isc_spb_ibc_dummy_intrvl      19
//#define isc_spb_ibc_trace_pools       20
//#define isc_spb_ibc_temp_dir_name     21
//#define isc_spb_ibc_temp_dir_size     22

/*******************************************
 * Parameters for isc_action_svc_set_env,  *
 * isc_action_svc_set_env_lock,            *
 * isc_action_svc_set_env_msg              *
 *                                         *
 *             UNDOCUMENTED                *
 *******************************************/

//#define isc_spb_env_path              5

/*********************************************
 * Parameters for isc_action_svc_lock_stats  *
 *********************************************/

//#define isc_spb_lck_sample		5
//#define isc_spb_lck_secs		6
//#define isc_spb_lck_contents		0x01
//#define isc_spb_lck_summary		0x02
//#define isc_spb_lck_wait		0x04
//#define isc_spb_lck_stats		0x08

#endif	/* SVC_UNDOC_H */

