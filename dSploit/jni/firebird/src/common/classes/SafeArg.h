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
 *  The Original Code was created by Claudio Valderrama on 3-Mar-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  The author thanks specially Fred Polizo Jr and Adriano dos Santos Fernandes
 *  for comments and corrections.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// Localized messages type-safe printing facility.

#ifndef FB_SAFEARG_H
#define FB_SAFEARG_H

// Definitions to generate fixed-length integral data types.
#ifdef _MSC_VER
typedef _int64 int64_t;
typedef unsigned _int64 uint64_t;
typedef _int32 int32_t;
typedef unsigned _int32 uint32_t;
typedef _int16 int16_t;
typedef unsigned _int16 uint16_t;
/* These macros were used to generate constants for testing.
#define c64(n) (n)
#define uc64(n) (n)
#else
#define c64(n) (n##LL)
#define uc64(n) (n##ULL)
*/
#endif

// Just an emulation of 128-bit numbers for now.
struct DoubleQuad
{
	int64_t high;
	uint64_t low;
};

typedef DoubleQuad SINT128;


namespace MsgFormat
{

// For now we allow 7 parameters; @1..@7 in MsgPrint.
const size_t SAFEARG_MAX_ARG = 7;

// This is the unit that represents one parameter in the format routines.
// The user of the routines rarely needs to be concerned with it.
// For now it contains only the data type and the value.
struct safe_cell
{
	enum arg_type
	{
		at_none,
		at_char,
		at_uchar,
		//at_int16,
		//at_int32,
		at_int64,
		at_uint64,
		at_int128,
		at_double,
		at_str,
		at_ptr
	};

	struct safe_str
	{
		const char* s_string;
	};


	arg_type type;
	union
	{
		unsigned char c_value;
		int64_t i_value;
		SINT128 i128_value;
		double d_value;
		safe_str st_value;
		void* p_value;
	};
};



class BaseStream;

// This is the main class that does the magic of receiving a chain of type-safe
// parameters. All parameters should be appended to it using the << operator.
// Only basic data types are supported.
// The allowed types are char, UCHAR, all lengths of signed/unsigned integral values,
// the SINT128 fake type (a struct, really), double, strings, UCHAR strings and the
// (non-const) void pointer. Care should be taken to not pass something by address
// (except char* and UCHAR* types) because the compiler may route it to the overload
// for the void pointer and it will be printed as an address in hex.
// An object of this class can be created, filled and passed later to the printing
// routines, cleaned with clear(), refilled and passed again to the printing routines
// or simply constructed as an anonymous object and being filled at the same time
// that it serves as input to the printing routines (the MsgPrint family).
// Using << to pass more than 7 parameters will ignore the rest of the parameters
// silently instead of throwing exceptions or crashing.
// Only the full MsgPrint is declared a friend below, so the other routines should
// work through it.
// Typically the construction doesn't have parameters, but there's an overload
// that takes an array of integers. If the array has more than 7 elements, the
// rest are ignored. Then probably no more parameters will be pushed using <<.
// A dump() method is provided to dump all the parameters to a TEXT array or up
// to the number of elements in the target array if it's smallers than the number
// of parameters in SafeArg. In case it's bigger, the elements are set to NULL.
// Finally getCount() and getCell() are provided for the rare need to loop
// retrieving individual parameters as safe_cell structures. No provision is
// made to modify parameters once pushed into a SafeArg object.
// The parameter formatting is just an indication for the future, thus the
// m_extras pointer is not used for now. No need for parameter formatting (like
// alignment, width, precision, padding, numeric base) was evident as the msg.fdb
// contains rather simple format strings.
// Usage examples appear in the MsgPrint.h file.
class SafeArg
{
public:
	SafeArg();
	SafeArg(const int val[], size_t v_size);
	SafeArg& clear();
	SafeArg& operator<<(char c);
	SafeArg& operator<<(unsigned char c);
	//SafeArg& operator<<(int16_t c);
	SafeArg& operator<<(short c);
	SafeArg& operator<<(unsigned short c);
	//SafeArg& operator<<(int32_t c);
	SafeArg& operator<<(int c);
	SafeArg& operator<<(unsigned int c);
	SafeArg& operator<<(long int c);
	SafeArg& operator<<(unsigned long int c);
	SafeArg& operator<<(SINT64 c);
	SafeArg& operator<<(FB_UINT64 c);
	//SafeArg& operator<<(long c);
	SafeArg& operator<<(SINT128 c);
	SafeArg& operator<<(double c);
	SafeArg& operator<<(const char* c);
	SafeArg& operator<<(const unsigned char* c);
	SafeArg& operator<<(void* c);
	void dump(const TEXT* target[], size_t v_size) const;
	const safe_cell& getCell(size_t index) const;
	size_t getCount() const;
private:
	size_t m_count;
	safe_cell m_arguments[SAFEARG_MAX_ARG];
	const void* m_extras; // Formatting, etc.
	friend int MsgPrint(BaseStream& out_stream, const char* format, const SafeArg& arg);
};

inline SafeArg::SafeArg()
	: m_count(0), m_extras(0)
{
}

inline SafeArg& SafeArg::clear()
{
	m_count = 0;
	m_extras = 0;
	return *this;
}

inline size_t SafeArg::getCount() const
{
	return m_count;
}

} // namespace

#endif // FB_SAFEARG_H

