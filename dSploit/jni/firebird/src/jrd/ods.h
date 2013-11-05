/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ods.h
 *	DESCRIPTION:	On disk structure definitions
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
 * 2000.11.29 Patrick J. P. Griffin: fixed bug SF #116733
 *	Add typedef struct gpg to properly document the layout of the generator page
 * 2002.08.26 Dmitry Yemanov: minor ODS change (new indices on system tables)
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_ODS_H
#define JRD_ODS_H

#include "../jrd/RecordNumber.h"

// This macro enables the ability of the engine to connect to databases
// from ODS 8 up to the latest.  If this macro is undefined, the engine
// only opens a database of the current ODS major version.

#define ODS_8_TO_CURRENT

/**********************************************************************
**
** NOTE:
**
**   ODS 5 was shipped with version 3.3 but no longer supported
**   ODS 6 and ODS 7 never went out the door
**   ODS 8 was shipped with version 4.0
**   ODS 9 was going to be shipped with version 4.5 but never was released,
**         thus it became v5.0's ODS.
**   ODS 10 was shipped with IB version 6.0
**   Here the Firebird history begins:
**   ODS 10.0 is for FB1.0 and ODS 10.1 is for FB1.5.
**   ODS 11.0 is for FB2.0, ODS11.1 is for FB2.1 and ODS11.2 is for FB2.5.
**
***********************************************************************/

// ODS major version -- major versions are not compatible

//const USHORT ODS_VERSION6	= 6;		// on-disk structure as of v3.0
//const USHORT ODS_VERSION7	= 7;		// new on disk structure for fixing index bug
const USHORT ODS_VERSION8	= 8;		// new btree structure to support pc semantics
const USHORT ODS_VERSION9	= 9;		// btree leaf pages are always propagated up
const USHORT ODS_VERSION10	= 10;		// V6.0 features. SQL delimited idetifier,
										// SQLDATE, and 64-bit exact numeric type
const USHORT ODS_VERSION11	= 11;		// Firebird 2.x features

// ODS minor version -- minor versions ARE compatible, but may be
// increasingly functional.  Add new minor versions, but leave previous
// names intact

// Minor versions for ODS 6

//const USHORT ODS_GRANT6		= 1;	// adds fields for field level grant
//const USHORT ODS_INTEGRITY6	= 2;	// adds fields for referential integrity
//const USHORT ODS_FUNCTIONS6	= 3;	// modifies type of RDB$MODULE_NAME field
//const USHORT ODS_SQLNAMES6	= 4;	// permits SQL security on > 27 SCHAR names
//const USHORT ODS_CURRENT6		= 4;

// Minor versions for ODS 7

//const USHORT ODS_FUNCTIONS7	= 1;	// modifies type of RDB$MODULE_NAME field
//const USHORT ODS_SQLNAMES7	= 2;	// permits SQL security on > 27 SCHAR names
//const USHORT ODS_CURRENT7		= 2;

// Minor versions for ODS 8

//const USHORT ODS_CASCADE_RI8	= 1;	// permits cascading referential integrity
										// ODS 8.2 is the same as ODS 8.1
//const USHORT ODS_CURRENT8		= 2;

// Minor versions for ODS 9

//const USHORT ODS_CURRENT_9_0	= 0;	// SQL roles & Index garbage collection
//const USHORT ODS_SYSINDEX9	= 1;	// Index on RDB$CHECK_CONSTRAINTS (RDB$TRIGGER_NAME)
//const USHORT ODS_CURRENT9		= 1;

// Minor versions for ODS 10

//const USHORT ODS_CURRENT10_0	= 0;	// V6.0 features. SQL delimited identifier,
										// SQLDATE, and 64-bit exact numeric type
//const USHORT ODS_SYSINDEX10	= 1;	// New system indices
//const USHORT ODS_CURRENT10	= 1;

// Minor versions for ODS 11

const USHORT ODS_CURRENT11_0	= 0;	// Firebird 2.0 features
const USHORT ODS_CURRENT11_1	= 1;	// Firebird 2.1 features
const USHORT ODS_CURRENT11_2	= 2;	// Firebird 2.5 features
const USHORT ODS_CURRENT11		= 2;

// useful ODS macros. These are currently used to flag the version of the
// system triggers and system indices in ini.epp

inline USHORT ENCODE_ODS(USHORT major, USHORT minor)
{
	return ((major << 4) | minor);
}

const USHORT ODS_8_0		= ENCODE_ODS(ODS_VERSION8, 0);
const USHORT ODS_8_1		= ENCODE_ODS(ODS_VERSION8, 1);
const USHORT ODS_9_0		= ENCODE_ODS(ODS_VERSION9, 0);
const USHORT ODS_9_1		= ENCODE_ODS(ODS_VERSION9, 1);
const USHORT ODS_10_0		= ENCODE_ODS(ODS_VERSION10, 0);
const USHORT ODS_10_1		= ENCODE_ODS(ODS_VERSION10, 1);
const USHORT ODS_11_0		= ENCODE_ODS(ODS_VERSION11, 0);
const USHORT ODS_11_1		= ENCODE_ODS(ODS_VERSION11, 1);
const USHORT ODS_11_2		= ENCODE_ODS(ODS_VERSION11, 2);

const USHORT ODS_FIREBIRD_FLAG = 0x8000;

// Decode ODS version to Major and Minor parts. The 4 LSB's are minor and
// the next 11 bits are major version number. The highest significant bit
// is the Firebird database flag.
inline USHORT DECODE_ODS_MAJOR(USHORT ods_version)
{
	return ((ods_version & 0x7FF0) >> 4);
}

inline USHORT DECODE_ODS_MINOR(USHORT ods_version)
{
	return (ods_version & 0x000F);
}

// Set current ODS major and minor version

const USHORT ODS_VERSION = ODS_VERSION11;		// Current ODS major version -- always
												// the highest.

const USHORT ODS_RELEASED = ODS_CURRENT11_0;	// The lowest stable minor version
												// number for this ODS_VERSION!

const USHORT ODS_CURRENT = ODS_CURRENT11;		// The highest defined minor version
												// number for this ODS_VERSION!

const USHORT ODS_CURRENT_VERSION = ODS_11_2;	// Current ODS version in use which includes
												// both major and minor ODS versions!


const USHORT USER_REL_INIT_ID_ODS8		= 31;	// ODS < 9 ( <= 8.2)
const USHORT USER_DEF_REL_INIT_ID		= 128;	// ODS >= 9


// Page types

const SCHAR pag_undefined		= 0;
const SCHAR pag_header			= 1;		// Database header page
const SCHAR pag_pages			= 2;		// Page inventory page
const SCHAR pag_transactions	= 3;		// Transaction inventory page
const SCHAR pag_pointer			= 4;		// Pointer page
const SCHAR pag_data			= 5;		// Data page
const SCHAR pag_root			= 6;		// Index root page
const SCHAR pag_index			= 7;		// Index (B-tree) page
const SCHAR pag_blob			= 8;		// Blob data page
const SCHAR pag_ids				= 9;		// Gen-ids
const SCHAR pag_log				= 10;		// Write ahead log information DEPRECATED
const SCHAR pag_max				= 10;		// Max page type

// Pre-defined page numbers

const SLONG HEADER_PAGE		= 0;
const SLONG LOG_PAGE		= 2;

// Page size limits

const USHORT MIN_PAGE_SIZE		= 1024;
const USHORT MAX_PAGE_SIZE		= 16384;
const USHORT DEFAULT_PAGE_SIZE	= 4096;
const USHORT MIN_NEW_PAGE_SIZE	= 4096;

namespace Ods {

// Basic page header

struct pag
{
	SCHAR pag_type;
	UCHAR pag_flags;
	USHORT pag_checksum;
	ULONG pag_generation;
	// We renamed pag_seqno for SCN number usage to avoid major ODS version bump
	ULONG pag_scn;			// WAL seqno of last update, now used by nbackup
	ULONG reserved;			// Was used for WAL
};

typedef pag* PAG;


// Blob page

struct blob_page
{
	pag blp_header;
	SLONG blp_lead_page;		// First page of blob (for redundancy only)
	SLONG blp_sequence;			// Sequence within blob
	USHORT blp_length;			// Bytes on page
	USHORT blp_pad;				// Unused
	SLONG blp_page[1];			// Page number if level 1
};

#define BLP_SIZE	OFFSETA(Ods::blob_page*, blp_page)

// pag_flags
const UCHAR blp_pointers	= 1;		// Blob pointer page, not data page


// B-tree node

struct btree_nod
{
	UCHAR btn_prefix;			// size of compressed prefix
	UCHAR btn_length;			// length of data in node
	UCHAR btn_number[4];		// page or record number
	UCHAR btn_data[1];
};

const int BTN_SIZE	= 6;

// B-tree page ("bucket")
struct btree_page
{
	pag btr_header;
	SLONG btr_sibling;			// right sibling page
	SLONG btr_left_sibling;		// left sibling page
	SLONG btr_prefix_total;		// sum of all prefixes on page
	USHORT btr_relation;		// relation id for consistency
	USHORT btr_length;			// length of data in bucket
	UCHAR btr_id;				// index id for consistency
	UCHAR btr_level;			// index level (0 = leaf)
	btree_nod btr_nodes[1];
};

// Firebird B-tree nodes
const USHORT BTN_LEAF_SIZE	= 6;
const USHORT BTN_PAGE_SIZE	= 10;

struct IndexNode
{
	UCHAR* nodePointer;			// pointer to where this node can be read from the page
	USHORT prefix;				// size of compressed prefix
	USHORT length;				// length of data in node
	SLONG pageNumber;			// page number
	UCHAR* data;				// Data can be read from here
	RecordNumber recordNumber;	// record number
	bool isEndBucket;
	bool isEndLevel;
};

struct IndexJumpNode
{
	UCHAR* nodePointer;	// pointer to where this node can be read from the page
	USHORT prefix;		// length of prefix against previous jump node
	USHORT length;		// length of data in jump node (together with prefix this is prefix for pointing node)
	USHORT offset;		// offset to node in page
	UCHAR* data;		// Data can be read from here
};

struct IndexJumpInfo
{
	USHORT firstNodeOffset;		// offset to node in page
	USHORT jumpAreaSize;		// size area before a new jumpnode is made
	UCHAR  jumpers;				// nr of jump-nodes in page, with a maximum of 255
};

// pag_flags
const UCHAR btr_dont_gc				= 1;	// Don't garbage-collect this page
//const UCHAR btr_not_propagated	= 2;	// page is not propagated upward
const UCHAR btr_descending			= 8;	// Page/bucket is part of a descending index
const UCHAR btr_all_record_number	= 16;	// Non-leaf-nodes will contain record number information
const UCHAR btr_large_keys			= 32;	// AB: 2003-index-structure enhancement
const UCHAR btr_jump_info			= 64;	// AB: 2003-index-structure enhancement
const UCHAR btr_released			= 128;	// Page was released from b-tree

const UCHAR BTR_FLAG_COPY_MASK = (btr_descending | btr_all_record_number | btr_large_keys | btr_jump_info);

// Data Page

struct data_page
{
	pag dpg_header;
	SLONG dpg_sequence;			// Sequence number in relation
	USHORT dpg_relation;		// Relation id
	USHORT dpg_count;			// Number of record segments on page
	struct dpg_repeat
	{
		USHORT dpg_offset;		// Offset of record fragment
		USHORT dpg_length;		// Length of record fragment
	} dpg_rpt[1];
};

#define DPG_SIZE	(sizeof (Ods::data_page) - sizeof (Ods::data_page::dpg_repeat))

// pag_flags
const UCHAR dpg_orphan	= 1;		// Data page is NOT in pointer page
const UCHAR dpg_full	= 2;		// Pointer page is marked FULL
const UCHAR dpg_large	= 4;		// Large object is on page


// Index root page

struct index_root_page
{
	pag irt_header;
	USHORT irt_relation;			// relation id (for consistency)
	USHORT irt_count;				// Number of indices
	struct irt_repeat
	{
		SLONG irt_root;				// page number of index root
		union {
			float irt_selectivity;	// selectivity of index - NOT USED since ODS11
			SLONG irt_transaction;	// transaction in progress
		} irt_stuff;
		USHORT irt_desc;			// offset to key descriptions
		UCHAR irt_keys;				// number of keys in index
		UCHAR irt_flags;
	} irt_rpt[1];
};

// key descriptor

struct irtd_ods10
{
	USHORT irtd_field;
	USHORT irtd_itype;
};

struct irtd : public irtd_ods10
{
	float irtd_selectivity;
};

// irtd_itype
const USHORT irt_unique			= 1;
const USHORT irt_descending		= 2;
const USHORT irt_in_progress	= 4;
const USHORT irt_foreign		= 8;
const USHORT irt_primary		= 16;
const USHORT irt_expression		= 32;
const USHORT irt_complete_segs	= 64;

const int STUFF_COUNT		= 4;
const SLONG END_LEVEL		= -1;
const SLONG END_BUCKET		= -2;

// Header page

struct header_page
{
	pag hdr_header;
	USHORT hdr_page_size;			// Page size of database
	USHORT hdr_ods_version;			// Version of on-disk structure
	SLONG hdr_PAGES;				// Page number of PAGES relation
	ULONG hdr_next_page;			// Page number of next hdr page
	SLONG hdr_oldest_transaction;	// Oldest interesting transaction
	SLONG hdr_oldest_active;		// Oldest transaction thought active
	SLONG hdr_next_transaction;		// Next transaction id
	USHORT hdr_sequence;			// sequence number of file
	USHORT hdr_flags;				// Flag settings, see below
	SLONG hdr_creation_date[2];		// Date/time of creation
	SLONG hdr_attachment_id;		// Next attachment id
	SLONG hdr_shadow_count;			// Event count for shadow synchronization
	SSHORT hdr_implementation;		// Implementation number
	USHORT hdr_ods_minor;			// Update version of ODS
	USHORT hdr_ods_minor_original;	// Update version of ODS at creation
	USHORT hdr_end;					// offset of HDR_end in page
	ULONG hdr_page_buffers;			// Page buffers for database cache
	SLONG hdr_bumped_transaction;	// Bumped transaction id for log optimization
	SLONG hdr_oldest_snapshot;		// Oldest snapshot of active transactions
	SLONG hdr_backup_pages; 		// The amount of pages in files locked for backup
	SLONG hdr_misc[3];				// Stuff to be named later
	UCHAR hdr_data[1];				// Misc data
};

#define HDR_SIZE	OFFSETA (Ods::header_page*, hdr_data)

// Header page clumplets

// Data items have the format
//
//	<type_byte> <length_byte> <data...>

const UCHAR HDR_end					= 0;
const UCHAR HDR_root_file_name		= 1;	// Original name of root file
//const UCHAR HDR_journal_server	= 2;	// Name of journal server
const UCHAR HDR_file				= 3;	// Secondary file
const UCHAR HDR_last_page			= 4;	// Last logical page number of file
//const UCHAR HDR_unlicensed		= 5;	// Count of unlicensed activity
const UCHAR HDR_sweep_interval		= 6;	// Transactions between sweeps
const UCHAR HDR_log_name			= 7;	// replay log name
//const UCHAR HDR_journal_file		= 8;	// Intermediate journal file
const UCHAR HDR_password_file_key	= 9;	// Key to compare to password db
//const UCHAR HDR_backup_info		= 10;	// WAL backup information
//const UCHAR HDR_cache_file		= 11;	// Shared cache file
const UCHAR HDR_difference_file		= 12;	// Delta file that is used during backup lock
const UCHAR HDR_backup_guid			= 13;	// UID generated on each switch into backup mode
const UCHAR HDR_max					= 14;	// Maximum HDR_clump value

// Header page flags

const USHORT hdr_active_shadow		= 0x1;		// 1    file is an active shadow file
const USHORT hdr_force_write		= 0x2;		// 2    database is forced write
//const USHORT hdr_short_journal	= 0x4;		// 4    short-term journalling
//const USHORT hdr_long_journal		= 0x8;		// 8    long-term journalling
const USHORT hdr_no_checksums		= 0x10;		// 16   don't calculate checksums
const USHORT hdr_no_reserve			= 0x20;		// 32   don't reserve space for versions
//const USHORT hdr_disable_cache	= 0x40;		// 64   disable using shared cache file
//const USHORT hdr_shutdown			= 0x80;		// 128  database is shutdown
const USHORT hdr_SQL_dialect_3		= 0x100;	// 256  database SQL dialect 3
const USHORT hdr_read_only			= 0x200;	// 512  Database in ReadOnly. If not set, DB is RW
// backup status mask - see bit values in nbak.h
const USHORT hdr_backup_mask		= 0xC00;
const USHORT hdr_shutdown_mask		= 0x1080;

// Values for shutdown mask
const USHORT hdr_shutdown_none		= 0x0;
const USHORT hdr_shutdown_multi		= 0x80;
const USHORT hdr_shutdown_full		= 0x1000;
const USHORT hdr_shutdown_single	= 0x1080;

/*
struct sfd
{
	SLONG sfd_min_page;			// Minimum page number
	SLONG sfd_max_page;			// Maximum page number
	UCHAR sfd_index;			// Sequence of secondary file
	UCHAR sfd_file[1];			// Given file name
};
typedef sfd SFD;
*/

// Page Inventory Page

struct page_inv_page
{
	pag pip_header;
	SLONG pip_min;				// Lowest (possible) free page
	UCHAR pip_bits[1];
};


// Pointer Page

struct pointer_page
{
	pag ppg_header;
	SLONG ppg_sequence;			// Sequence number in relation
	SLONG ppg_next;				// Next pointer page in relation
	USHORT ppg_count;			// Number of slots active
	USHORT ppg_relation;		// Relation id
	USHORT ppg_min_space;		// Lowest slot with space available
	USHORT ppg_max_space;		// Highest slot with space available
	SLONG ppg_page[1];			// Data page vector
};

// pag_flags
const UCHAR ppg_eof		= 1;	// Last pointer page in relation

// Transaction Inventory Page

struct tx_inv_page
{
	pag tip_header;
	SLONG tip_next;				// Next transaction inventory page
	UCHAR tip_transactions[1];
};


// Generator Page

struct generator_page
{
	pag gpg_header;
	SLONG gpg_sequence;			// Sequence number
	SLONG gpg_waste1;			// overhead carried for backward compatibility
	USHORT gpg_waste2;			// overhead carried for backward compatibility
	USHORT gpg_waste3;			// overhead carried for backward compatibility
	USHORT gpg_waste4;			// overhead carried for backward compatibility
	USHORT gpg_waste5;			// overhead carried for backward compatibility
	SINT64 gpg_values[1];		// Generator vector
};


// Record header

struct rhd {
	SLONG rhd_transaction;		// transaction id
	SLONG rhd_b_page;			// back pointer
	USHORT rhd_b_line;			// back line
	USHORT rhd_flags;			// flags, etc
	UCHAR rhd_format;			// format version
	UCHAR rhd_data[1];
};

#define RHD_SIZE	OFFSETA (Ods::rhd*, rhd_data)

// Record header for fragmented record

struct rhdf
{
	SLONG rhdf_transaction;		// transaction id
	SLONG rhdf_b_page;			// back pointer
	USHORT rhdf_b_line;			// back line
	USHORT rhdf_flags;			// flags, etc
	UCHAR rhdf_format;			// format version    // until here, same than rhd
	SLONG rhdf_f_page;			// next fragment page
	USHORT rhdf_f_line;			// next fragment line
	UCHAR rhdf_data[1];			// Blob data
};

#define RHDF_SIZE	OFFSETA (Ods::rhdf*, rhdf_data)


// Record header for blob header

struct blh
{
	SLONG blh_lead_page;		// First data page number
	SLONG blh_max_sequence;		// Number of data pages
	USHORT blh_max_segment;		// Longest segment
	USHORT blh_flags;			// flags, etc
	UCHAR blh_level;			// Number of address levels
	SLONG blh_count;			// Total number of segments
	SLONG blh_length;			// Total length of data
	USHORT blh_sub_type;		// Blob sub-type
	UCHAR blh_charset;			// Blob charset (since ODS 11.1)
	UCHAR blh_unused;
	SLONG blh_page[1];			// Page vector for blob pages
};

#define BLH_SIZE	OFFSETA (Ods::blh*, blh_page)
// rhd_flags, rhdf_flags and blh_flags

const USHORT rhd_deleted		= 1;		// record is logically deleted
const USHORT rhd_chain			= 2;		// record is an old version
const USHORT rhd_fragment		= 4;		// record is a fragment
const USHORT rhd_incomplete		= 8;		// record is incomplete
const USHORT rhd_blob			= 16;		// isn't a record but a blob
const USHORT rhd_stream_blob	= 32;		// blob is a stream mode blob
const USHORT rhd_delta			= 32;		// prior version is differences only
const USHORT rhd_large			= 64;		// object is large
const USHORT rhd_damaged		= 128;		// object is known to be damaged
const USHORT rhd_gc_active		= 256;		// garbage collecting dead record version
const USHORT rhd_uk_modified	= 512;		// record key field values are changed


// Log page

struct ctrl_pt
{
	SLONG cp_seqno;
	SLONG cp_offset;
	SLONG cp_p_offset;
	SSHORT cp_fn_length;
};

struct log_info_page
{
	pag log_header;
	SLONG log_flags;			// flags, OBSOLETE
	ctrl_pt log_cp_1;			// control point 1
	ctrl_pt log_cp_2;			// control point 2
	ctrl_pt log_file;			// current file
	SLONG log_next_page;		// Next log page
	SLONG log_mod_tip;			// tip of modify transaction
	SLONG log_mod_tid;			// transaction id of modify process
	SLONG log_creation_date[2];	// Date/time of log creation
	SLONG log_free[4];			// some extra space for later use
	USHORT log_end;				// similar to hdr_end
	UCHAR log_data[1];
};

#define LIP_SIZE	OFFSETA (Ods::log_info_page*, log_data)

// additions for write ahead log, almost obsolete.

//const int CTRL_FILE_LEN			= 255;	// Pre allocated size of file name
//const USHORT CLUMP_ADD			= 0;
//const USHORT CLUMP_REPLACE		= 1;
//const USHORT CLUMP_REPLACE_ONLY	= 2;

// Log Clumplet types

const UCHAR LOG_end				= HDR_end;
//const int LOG_ctrl_file1		= 1;	// file name of 2nd last control pt
//const int LOG_ctrl_file2		= 2;	// file name of last ctrl pt
//const int LOG_logfile			= 3;	// Primary WAL file name
//const int LOG_backup_info		= 4;	// Journal backup directory
//const int LOG_chkpt_len		= 5;	// checkpoint length
//const int LOG_num_bufs		= 6;	// Number of log buffers
//const int LOG_bufsize			= 7;	// Buffer size
//const int LOG_grp_cmt_wait	= 8;	// Group commit wait time
//const int LOG_max				= 8;	// Maximum LOG_clump value

// This (not exact) copy of class DSC is used to store descriptors on disk.
// Hopefully its binary layout is common for 32/64 bit CPUs.
struct Descriptor
{
	UCHAR	dsc_dtype;
	SCHAR	dsc_scale;
	USHORT	dsc_length;
	SSHORT	dsc_sub_type;
	USHORT	dsc_flags;
	ULONG	dsc_offset;
};

// Array description, "internal side" used by the engine.
// And stored on the disk, in the relation summary blob.

struct InternalArrayDesc
{
	UCHAR iad_version;			// Array descriptor version number
	UCHAR iad_dimensions;		// Dimensions of array
	USHORT iad_struct_count;	// Number of struct elements
	USHORT iad_element_length;	// Length of array element
	USHORT iad_length;			// Length of array descriptor
	SLONG iad_count;			// Total number of elements
	SLONG iad_total_length;		// Total length of array
	struct iad_repeat
	{
		Descriptor iad_desc;	// Element descriptor
		SLONG iad_length;		// Length of "vector" element
		SLONG iad_lower;		// Lower bound
		SLONG iad_upper;		// Upper bound
	};
	iad_repeat iad_rpt[1];
};

const UCHAR IAD_VERSION_1		= 1;

/*
inline int IAD_LEN(int count)
{
	if (!count)
		count = 1;
	return sizeof (InternalArrayDesc) +
		(count - 1) * sizeof (InternalArrayDesc::iad_repeat);
}
*/
#define IAD_LEN(count)	(sizeof (Ods::InternalArrayDesc) + \
	(count ? count - 1: count) * sizeof (Ods::InternalArrayDesc::iad_repeat))

} //namespace Ods

#endif // JRD_ODS_H
