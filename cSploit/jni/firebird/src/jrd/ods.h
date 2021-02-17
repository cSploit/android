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
#include "../common/classes/fb_string.h"

// This macro enables the ability of the engine to connect to databases
// from ODS 8 up to the latest.  If this macro is undefined, the engine
// only opens a database of the current ODS major version.

//#define ODS_8_TO_CURRENT

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
**   ODS 12.0 is for FB3.
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
const USHORT ODS_VERSION12	= 12;		// Firebird 3.x features

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

//const USHORT ODS_CURRENT11_0	= 0;	// Firebird 2.0 features
//const USHORT ODS_CURRENT11_1	= 1;	// Firebird 2.1 features
//const USHORT ODS_CURRENT11_2	= 2;	// Firebird 2.5 features
//const USHORT ODS_CURRENT11	= 2;

// Minor versions for ODS 12

const USHORT ODS_CURRENT12_0	= 0;	// Firebird 3.0 features
const USHORT ODS_CURRENT12		= 0;

// useful ODS macros. These are currently used to flag the version of the
// system triggers and system indices in ini.e

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
const USHORT ODS_12_0		= ENCODE_ODS(ODS_VERSION12, 0);

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

const USHORT ODS_VERSION = ODS_VERSION12;		// Current ODS major version -- always
												// the highest.

const USHORT ODS_RELEASED = ODS_CURRENT12_0;	// The lowest stable minor version
												// number for this ODS_VERSION!

const USHORT ODS_CURRENT = ODS_CURRENT12;		// The highest defined minor version
												// number for this ODS_VERSION!

const USHORT ODS_CURRENT_VERSION = ODS_12_0;	// Current ODS version in use which includes
												// both major and minor ODS versions!


//const USHORT USER_REL_INIT_ID_ODS8	= 31;	// ODS < 9 ( <= 8.2)
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
const SCHAR pag_scns			= 10;		// SCN's inventory page
const SCHAR pag_max				= 10;		// Max page type

// Pre-defined page numbers

const ULONG HEADER_PAGE		= 0;
const ULONG FIRST_PIP_PAGE	= 1;
const ULONG FIRST_SCN_PAGE	= 2;

// Page size limits

const USHORT MIN_PAGE_SIZE		= 1024;
const USHORT MAX_PAGE_SIZE		= 16384;
const USHORT DEFAULT_PAGE_SIZE	= 4096;
const USHORT MIN_NEW_PAGE_SIZE	= 4096;

namespace Ods {

// Crypt page by type

const bool pag_crypt_page[pag_max + 1] = {false, false, false,
										  false, false, true,	// data
										  false, true, true,	// index, blob
										  false, false};

// pag_flags for any page type

const UCHAR crypted_page	= 0x80;		// Page encrypted

// Basic page header

struct pag
{
	UCHAR pag_type;
	UCHAR pag_flags;
	USHORT pag_reserved;		// not used but anyway present because of alignment rules
	ULONG pag_generation;
	ULONG pag_scn;
	ULONG pag_pageno;			// for validation
};

typedef pag* PAG;


// Blob page

struct blob_page
{
	pag blp_header;
	ULONG blp_lead_page;		// First page of blob (for redundancy only)
	ULONG blp_sequence;			// Sequence within blob
	USHORT blp_length;			// Bytes on page
	USHORT blp_pad;				// Unused
	ULONG blp_page[1];			// Page number if level 1
};

#define BLP_SIZE	OFFSETA(Ods::blob_page*, blp_page)

// pag_flags
const UCHAR blp_pointers	= 0x01;		// Blob pointer page, not data page


// B-tree page ("bucket")
struct btree_page
{
	pag btr_header;
	ULONG btr_sibling;			// right sibling page
	ULONG btr_left_sibling;		// left sibling page
	SLONG btr_prefix_total;		// sum of all prefixes on page
	USHORT btr_relation;		// relation id for consistency
	USHORT btr_length;			// length of data in bucket
	UCHAR btr_id;				// index id for consistency
	UCHAR btr_level;			// index level (0 = leaf)
	USHORT btr_jump_interval;	// interval between jump nodes
	USHORT btr_jump_size;		// size of the jump table
	UCHAR btr_jump_count;		// number of jump nodes
	UCHAR btr_nodes[1];
};

#define BTR_SIZE	OFFSETA(Ods::btree_page*, btr_nodes)

// pag_flags
//const UCHAR btr_dont_gc			= 1;	// Don't garbage-collect this page
//const UCHAR btr_descending		= 2;	// Page/bucket is part of a descending index
//const UCHAR btr_jump_info			= 16;	// AB: 2003-index-structure enhancement
const UCHAR btr_released			= 32;	// Page was released from b-tree

// Data Page

struct data_page
{
	pag dpg_header;
	ULONG dpg_sequence;			// Sequence number in relation
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
const UCHAR dpg_swept	= 8;		// Sweep has nothing to do on this page
const UCHAR dpg_secondary	= 16;	// Primary record versions not stored on this page
									// Set in dpm.epp's extend_relation() but never tested.


// Index root page

struct index_root_page
{
	pag irt_header;
	USHORT irt_relation;			// relation id (for consistency)
	USHORT irt_count;				// Number of indices
	struct irt_repeat
	{
		ULONG irt_root;				// page number of index root
		TraNumber irt_transaction;	// transaction in progress
		USHORT irt_desc;			// offset to key descriptions
		UCHAR irt_keys;				// number of keys in index
		UCHAR irt_flags;
	} irt_rpt[1];
};

// key descriptor

struct irtd
{
	USHORT irtd_field;
	USHORT irtd_itype;
	float irtd_selectivity;
};

// irt_flags, must match the idx_flags (see btr.h)
const USHORT irt_unique			= 1;
const USHORT irt_descending		= 2;
const USHORT irt_in_progress	= 4;
const USHORT irt_foreign		= 8;
const USHORT irt_primary		= 16;
const USHORT irt_expression		= 32;

const int STUFF_COUNT		= 4;

const ULONG END_LEVEL		= ~0;
const ULONG END_BUCKET		= (~0u) << 1;

// Header page

struct header_page
{
	pag hdr_header;
	USHORT hdr_page_size;			// Page size of database
	USHORT hdr_ods_version;			// Version of on-disk structure
	ULONG hdr_PAGES;				// Page number of PAGES relation
	ULONG hdr_next_page;			// Page number of next hdr page
	TraNumber hdr_oldest_transaction;	// Oldest interesting transaction
	TraNumber hdr_oldest_active;		// Oldest transaction thought active
	TraNumber hdr_next_transaction;		// Next transaction id
	USHORT hdr_sequence;			// sequence number of file
	USHORT hdr_flags;				// Flag settings, see below
	SLONG hdr_creation_date[2];		// Date/time of creation
	SLONG hdr_attachment_id;		// Next attachment id
	SLONG hdr_shadow_count;			// Event count for shadow synchronization
	UCHAR hdr_cpu;					// CPU database was created on
	UCHAR hdr_os;					// OS database was created under
	UCHAR hdr_cc;					// Compiler of engine on which database was created
	UCHAR hdr_compatibility_flags;	// Cross-platform database transfer compatibility flags
	USHORT hdr_ods_minor;			// Update version of ODS
	USHORT hdr_end;					// offset of HDR_end in page
	ULONG hdr_page_buffers;			// Page buffers for database cache
	TraNumber hdr_oldest_snapshot;		// Oldest snapshot of active transactions
	SLONG hdr_backup_pages; 		// The amount of pages in files locked for backup
	ULONG hdr_crypt_page;			// Page at which processing is in progress
	ULONG hdr_top_crypt;			// Last page to crypt
	TEXT hdr_crypt_plugin[32];		// Name of plugin used to crypt this DB
	SLONG hdr_misc[3];				// Stuff to be named later - reserved for minor changes
	UCHAR hdr_data[1];				// Misc data
};

#define HDR_SIZE	OFFSETA (Ods::header_page*, hdr_data)

// Header page clumplets

// Data items have the format
//
//	<type_byte> <length_byte> <data...>

const UCHAR HDR_end					= 0;
const UCHAR HDR_root_file_name		= 1;	// Original name of root file
const UCHAR HDR_file				= 2;	// Secondary file
const UCHAR HDR_last_page			= 3;	// Last logical page number of file
const UCHAR HDR_sweep_interval		= 4;	// Transactions between sweeps
const UCHAR HDR_password_file_key	= 5;	// Key to compare to password db
const UCHAR HDR_difference_file		= 6;	// Delta file that is used during backup lock
const UCHAR HDR_backup_guid			= 7;	// UID generated on each switch into backup mode
const UCHAR HDR_max					= 8;	// Maximum HDR_clump value

// Header page flags

const USHORT hdr_active_shadow		= 0x1;		// 1	file is an active shadow file
const USHORT hdr_force_write		= 0x2;		// 2	database is forced write
// const USHORT hdr_no_checksums	= 0x4;		// 4	don't calculate checksums, not used since ODS 12
const USHORT hdr_crypt_process		= 0x4;		// 4	Encryption status is changing now
const USHORT hdr_no_reserve			= 0x8;		// 8	don't reserve space for versions
const USHORT hdr_SQL_dialect_3		= 0x10;		// 16	database SQL dialect 3
const USHORT hdr_read_only			= 0x20;		// 32	Database is ReadOnly. If not set, DB is RW
const USHORT hdr_encrypted			= 0x40;		// 64	Database is encrypted
// backup status mask - see bit values in nbak.h
const USHORT hdr_backup_mask		= 0xC00;
const USHORT hdr_shutdown_mask		= 0x1080;

// Values for shutdown mask
const USHORT hdr_shutdown_none		= 0x0;
const USHORT hdr_shutdown_multi		= 0x80;
const USHORT hdr_shutdown_full		= 0x1000;
const USHORT hdr_shutdown_single	= 0x1080;


// Page Inventory Page

struct page_inv_page
{
	pag pip_header;
	ULONG pip_min;				// Lowest (possible) free page
	ULONG pip_used;				// Number of pages allocated from this PIP page
	UCHAR pip_bits[1];
};


// SCN's Page

struct scns_page
{
	pag scn_header;
	ULONG scn_sequence;			// Sequence number in page space
	ULONG scn_pages[1];			// SCN's vector
};

// Important note !
// pagesPerPIP value must be multiply of pagesPerSCN value !
//
// Nth PIP page number is : pagesPerPIP * N - 1
// Nth SCN page number is : pagesPerSCN * N
// Numbers of first PIP and SCN pages (N = 0) is fixed and not interesting here.
//
// Generally speaking it is possible that exists N and M that
//   pagesPerSCN * N == pagesPerPIP * M - 1,
// i.e. we can't guarantee that some SCN page will not have the same number as
// some PIP page. We can implement checks for this case and put corresponding
// SCN page at the next position but it will complicate code a lot.
//
// The much more easy solution is to make pagesPerPIP multiply of pagesPerSCN.
// The fact that page_inv_page::pip_bits array is LONG aligned and occupy less
// size (in bytes) than scns_page::scn_pages array allow us to use very simple
// formula for pagesPerSCN : pagesPerSCN = pagesPerPIP / BITS_PER_LONG.
// Please, consider above when changing page_inv_page or scns_page definition.
//
// Table below show numbers for different page sizes using current (ODS12)
// definitions of page_inv_page and scns_page
//
// PageSize  pagesPerPIP  maxPagesPerSCN    pagesPerSCN
//     4096        32576            1019           1018
//     8192        65344            2043           2042
//    16384       130880            4091           4090
//    32768       261952            8187           8186
//    65536       524096           16379          16378


// Pointer Page

struct pointer_page
{
	pag ppg_header;
	ULONG ppg_sequence;			// Sequence number in relation
	ULONG ppg_next;				// Next pointer page in relation
	USHORT ppg_count;			// Number of slots active
	USHORT ppg_relation;		// Relation id
	USHORT ppg_min_space;		// Lowest slot with space available
	ULONG ppg_page[1];			// Data page vector
};

// pag_flags
const UCHAR ppg_eof		= 1;	// Last pointer page in relation

// After array of physical page numbers (ppg_page) there is also array of bit
// flags per every data page. These flags describes state of corresponding data
// page. Definitions below used to deal with these bits.
const int PPG_DP_BITS_NUM	= 4;		// Number of additional flag bits per data page

const UCHAR ppg_dp_full		= 1;		// Data page is FULL
const UCHAR ppg_dp_large	= 2;		// Large object is on data page
const UCHAR ppg_dp_swept	= 4;		// Sweep has nothing to do on data page
const UCHAR ppg_dp_secondary = 8;		// Primary record versions not stored on data page

const UCHAR PPG_DP_ALL_BITS	= (1 << PPG_DP_BITS_NUM) - 1;

#define PPG_DP_BIT_MASK(slot, bit)		((bit) << (((slot) & 1) << 2))
#define PPG_DP_BITS_BYTE(bits, slot)	((bits)[(slot) >> 1])

#define PPG_DP_BIT_TEST(flags, slot, bit)	(PPG_DP_BITS_BYTE((flags), (slot)) & PPG_DP_BIT_MASK((slot), (bit)))
#define PPG_DP_BIT_SET(flags, slot, bit)	(PPG_DP_BITS_BYTE((flags), (slot)) |= PPG_DP_BIT_MASK((slot), (bit)))
#define PPG_DP_BIT_CLEAR(flags, slot, bit)	(PPG_DP_BITS_BYTE((flags), (slot)) &= ~PPG_DP_BIT_MASK((slot), (bit)))


// Transaction Inventory Page

struct tx_inv_page
{
	pag tip_header;
	ULONG tip_next;				// Next transaction inventory page
	UCHAR tip_transactions[1];
};


// Generator Page

struct generator_page
{
	pag gpg_header;
	ULONG gpg_sequence;			// Sequence number
	SINT64 gpg_values[1];		// Generator vector
};


// Record header

struct rhd
{
	TraNumber rhd_transaction;	// transaction id
	ULONG rhd_b_page;			// back pointer
	USHORT rhd_b_line;			// back line
	USHORT rhd_flags;			// flags, etc
	UCHAR rhd_format;			// format version
	UCHAR rhd_data[1];
};

#define RHD_SIZE	OFFSETA (Ods::rhd*, rhd_data)

// Record header for fragmented record

struct rhdf
{
	TraNumber rhdf_transaction;	// transaction id
	ULONG rhdf_b_page;			// back pointer
	USHORT rhdf_b_line;			// back line
	USHORT rhdf_flags;			// flags, etc
	UCHAR rhdf_format;			// format version    // until here, same than rhd
	ULONG rhdf_f_page;			// next fragment page
	USHORT rhdf_f_line;			// next fragment line
	UCHAR rhdf_data[1];			// Blob data
};

#define RHDF_SIZE	OFFSETA (Ods::rhdf*, rhdf_data)


// Record header for blob header

struct blh
{
	ULONG blh_lead_page;		// First data page number
	ULONG blh_max_sequence;		// Number of data pages
	USHORT blh_max_segment;		// Longest segment
	USHORT blh_flags;			// flags, etc
	UCHAR blh_level;			// Number of address levels, see blb_level in blb.h
	ULONG blh_count;			// Total number of segments
	ULONG blh_length;			// Total length of data
	USHORT blh_sub_type;		// Blob sub-type
	UCHAR blh_charset;			// Blob charset (since ODS 11.1)
	UCHAR blh_unused;
	ULONG blh_page[1];			// Page vector for blob pages
};

#define BLH_SIZE	OFFSETA (Ods::blh*, blh_page)
// rhd_flags, rhdf_flags and blh_flags

// record_param flags in req.h must be an exact replica of ODS record header flags

const USHORT rhd_deleted		= 1;		// record is logically deleted
const USHORT rhd_chain			= 2;		// record is an old version
const USHORT rhd_fragment		= 4;		// record is a fragment
const USHORT rhd_incomplete		= 8;		// record is incomplete
const USHORT rhd_blob			= 16;		// isn't a record but a blob
const USHORT rhd_stream_blob	= 32;		// blob is a stream mode blob
const USHORT rhd_delta			= 32;		// prior version is differences only
											// Tested in validation.cpp's walk_chain but never set
const USHORT rhd_large			= 64;		// object is large
const USHORT rhd_damaged		= 128;		// object is known to be damaged
const USHORT rhd_gc_active		= 256;		// garbage collecting dead record version
//const USHORT rhd_uk_modified	= 512;		// record key field values are changed



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
	ULONG iad_count;			// Total number of elements
	ULONG iad_total_length;		// Total length of array
	struct iad_repeat
	{
		Descriptor iad_desc;	// Element descriptor
		ULONG iad_length;		// Length of "vector" element
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

Firebird::string pagtype(UCHAR type);

} //namespace Ods

#endif // JRD_ODS_H
