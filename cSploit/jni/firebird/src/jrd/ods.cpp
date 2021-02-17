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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2005 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/ods.h"
#include "../jrd/ods_proto.h"

namespace Ods {

bool isSupported(USHORT majorVersion, USHORT minorVersion)
{
	const bool isFirebird = (majorVersion & ODS_FIREBIRD_FLAG);
	majorVersion &= ~ODS_FIREBIRD_FLAG;

	if (!isFirebird)
		return false;

#ifdef ODS_8_TO_CURRENT
	// Obsolete: Support InterBase major ODS numbers from 8 to 10
	/*
	if (!isFirebird)
	{
		return (majorVersion >= ODS_VERSION8 &&
				majorVersion <= ODS_VERSION10);
	}
	*/

	// This is for future ODS versions
	if (majorVersion > ODS_VERSION10 &&
		majorVersion < ODS_VERSION)
	{
		return true;
	}
#endif

	// Support current ODS of the engine
	if (majorVersion == ODS_VERSION &&
		minorVersion >= ODS_RELEASED &&
		minorVersion <= ODS_CURRENT)
	{
		return true;
	}

	// Do not support anything else
	return false;
}

size_t bytesBitPIP(size_t page_size)
{
	return page_size - OFFSETA(page_inv_page*, pip_bits);
}

size_t pagesPerPIP(size_t page_size)
{
	return bytesBitPIP(page_size) * 8;
}

size_t pagesPerSCN(size_t page_size)
{
	return pagesPerPIP(page_size) / BITS_PER_LONG;
}

// We must ensure that pagesPerSCN items can fit into scns_page::scn_pages array.
// We can't use fb_assert() here in ods.h so it is placed at pag.cpp

size_t maxPagesPerSCN(size_t page_size)
{
	return (page_size - OFFSETA(scns_page*, scn_pages)) / sizeof(((scns_page*)NULL)->scn_pages);
}

size_t transPerTIP(size_t page_size)
{
	return (page_size - OFFSETA(tx_inv_page*, tip_transactions)) * 4;
}

size_t gensPerPage(size_t page_size)
{
	return (page_size - OFFSETA(generator_page*, gpg_values)) /
		sizeof(((generator_page*) NULL)->gpg_values);
}

size_t dataPagesPerPP(size_t page_size)
{
	// Compute the number of data pages per pointer page.  Each data page
	// requires a 32 bit pointer and a 4 bit control field.

	return (page_size - OFFSETA(pointer_page*, ppg_page)) * 8 / (BITS_PER_LONG + PPG_DP_BITS_NUM);
}

size_t maxRecsPerDP(size_t page_size)
{
	// Compute the number of records that can fit on a page using the
	// size of the record index (dpb_repeat) and a record header.  This
	// gives an artificially high number, reducing the density of db_keys.

	size_t max_records =
		(page_size - sizeof(data_page)) / (sizeof(data_page::dpg_repeat) + OFFSETA(rhd*, rhd_data));

	// Artificially reduce density of records to test high bits of record number
	// max_records = 32000;

	// Optimize record numbers for new 64-bit sparse bitmap implementation
	// We need to measure if it is beneficial from performance point of view.
	// Price is slightly reduced density of record numbers, but for
	// ODS11 it doesn't matter because record numbers are 40-bit.
	// Benefit is ~1.5 times smaller sparse bitmaps on average and faster bitmap iteration.

	//max_records = FB_ALIGN(max_records, 64);

	return max_records;
}

size_t maxIndices(size_t page_size)
{
	// Compute the number of index roots that will fit on an index root page,
	// assuming that each index has only one key

	return (page_size - OFFSETA(index_root_page*, irt_rpt)) /
		(sizeof(index_root_page::irt_repeat) + sizeof(irtd));
}

Firebird::string pagtype(UCHAR type)
{
	// Print pretty name for database page type

	const char* nameArray[pag_max + 1] = {
		"purposely undefined",
		"database header",
		"page inventory",
		"transaction inventory",
		"pointer",
		"data",
		"index root",
		"index B-tree",
		"blob",
		"generators",
		"SCN inventory"
	};

	Firebird::string rc;
	if (type < FB_NELEM(nameArray))
		rc = nameArray[type];
	else
		rc.printf("unknown (%d)", type);

	return rc;
}

} // namespace
