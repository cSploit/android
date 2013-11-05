/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RecordNumber.h
 *	DESCRIPTION:	Handler class for table record number
 *
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 *
 */

#ifndef JRD_RECORDNUMBER_H
#define JRD_RECORDNUMBER_H

const SINT64 EMPTY_NUMBER = QUADCONST(0);
const SINT64 BOF_NUMBER = QUADCONST(-1);

// This class is to be used everywhere you may need to handle record numbers. We
// deliberately not define implicit conversions to and from integer to allow
// compiler check errors on our migration path from 32-bit to 64-bit record
// numbers.
class RecordNumber
{
public:
	// Packed record number represents common layout of RDB$DB_KEY and BLOB ID.
	// To ensure binary compatibility with old (pre-ODS11) databases it differs
	// for big- and little-endian machines.
	class Packed
	{
#ifdef WORDS_BIGENDIAN
	private:
		UCHAR bid_number_up;				// Upper byte of 40-bit record number
		UCHAR bid_reserved_for_relation;	// Reserved for future
	public:
		USHORT bid_relation_id;				// Relation id (or null)
	private:
		ULONG bid_number;					// Lower bytes of 40-bit record number
											// or 32-bit temporary ID of blob or array
#else
	public:
		USHORT bid_relation_id;		/* Relation id (or null) */

	private:
		UCHAR bid_reserved_for_relation;	/* Reserved for future expansion of relation space. */
		UCHAR bid_number_up;				// Upper byte of 40-bit record number
		ULONG bid_number;					// Lower bytes of 40-bit record number
											// or 32-bit temporary ID of blob or array
#endif
	public:
		ULONG& bid_temp_id()
		{
			return bid_number;
		}

		ULONG bid_temp_id() const
		{
			return bid_number;
		}

		// Handle encoding of record number for RDB$DB_KEY and BLOB ID structure.
		// BLOB ID is stored in database thus we do encode large record numbers
		// in a manner which preserves backward compatibility with older ODS.
		// The same applies to bid_decode routine below.
		inline void bid_encode(SINT64 value)
		{
			// Use explicit casts to suppress 64-bit truncation warnings
			// Store lower 32 bits of number
			bid_number = static_cast<ULONG>(value);
			// Store high 8 bits of number
			bid_number_up = static_cast<UCHAR>(value >> 32);
		}

		inline SINT64 bid_decode() const
		{
			return bid_number + (static_cast<FB_UINT64>(bid_number_up) << 32);
		}
	};

	// Default constructor.
	inline RecordNumber() : value(EMPTY_NUMBER), valid(false) {}

	// Copy constructor
	inline RecordNumber(const RecordNumber& from) : value(from.value), valid(from.valid) {}

	// Explicit constructor from 64-bit record number value
	inline explicit RecordNumber(SINT64 number) : value(number), valid(true) {}

	// Assignment operator
	inline RecordNumber& operator =(const RecordNumber& from)
	{
		value = from.value;
		valid = from.valid;
		return *this;
	}

	inline bool operator ==(const RecordNumber& other) const
	{
		return value == other.value;
	}

	inline bool operator !=(const RecordNumber& other) const
	{
		return value != other.value;
	}

	inline bool operator > (const RecordNumber& other) const
	{
		return value > other.value;
	}

	inline bool operator < (const RecordNumber& other) const
	{
		return value < other.value;
	}

	inline bool operator >= (const RecordNumber& other) const
	{
		return value >= other.value;
	}

	inline bool operator <= (const RecordNumber& other) const
	{
		return value <= other.value;
	}

	inline void decrement() { value--; }

	inline void increment() { value++; }

	inline SINT64 getValue() const { return value; }

	inline void setValue(SINT64 avalue) { value = avalue; }

	bool isBof() const { return value == BOF_NUMBER; }

	bool isValid() const { return valid; }

	inline bool checkNumber(USHORT records_per_page, // ~400 (8k page)
							USHORT data_pages_per_pointer_page) const  // ~2000 (8k page)
	{
		// We limit record number value to 40 bits and make sure decomposed value
		// fits into 3 USHORTs. This all makes practical table size limit (not
		// counting allocation threshold and overhead) roughtly equal to:
		// 16k page - 20000 GB
		// 8k page -  10000 GB
		// 4k page -   2500 GB
		// 2k page -    600 GB
		// 1k page -    150 GB
		// Large page size values are recommended for large databases because
		// page allocators are generally linear.
		return value < QUADCONST(0x10000000000) &&
			value < (SINT64) MAX_USHORT * records_per_page * data_pages_per_pointer_page;
	}

	inline void decompose(USHORT records_per_page, // ~400 (8k page)
						  USHORT data_pages_per_pointer_page,  // ~2000 (8k page)
						  SSHORT& line,
						  SSHORT& slot,
						  USHORT& pp_sequence) const
	{
		// Use explicit casts to suppress 64-bit truncation warnings
		line = static_cast<SSHORT>(value % records_per_page);
		const ULONG sequence = static_cast<ULONG>(value / records_per_page);
		slot = sequence % data_pages_per_pointer_page;
		pp_sequence = sequence / data_pages_per_pointer_page;
	}

	inline void compose(USHORT records_per_page, // ~400 (8k page)
						USHORT data_pages_per_pointer_page,  // ~2000 (8k page)
						SSHORT line,
						SSHORT slot,
						USHORT pp_sequence)
	{
		value = (((SINT64) pp_sequence) * data_pages_per_pointer_page + slot) * records_per_page + line;
	}

	// Handle encoding of record number for RDB$DB_KEY and BLOB ID structure.
	inline void bid_encode(Packed* recno) const
	{
		recno->bid_encode(value);
	}

	inline void bid_decode(const Packed* recno)
	{
		value = recno->bid_decode();
	}

	inline void setValid(bool to_value)
	{
		valid = to_value;
	}

private:
	// Use signed value because negative values are widely used as flags in the
	// engine. Since page number is the signed 32-bit integer and it is also may
	// be stored in this structure we want sign extension to take place.
	SINT64 value;
	bool valid;
};

#endif // JRD_RECORDNUMBER_H
