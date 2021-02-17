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
 *  The Original Code was created by Claudio Valderrama on 10-Jul-2009
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// Centralized code to handle command line arguments.

#ifndef CLASSES_SWITCHES
#define CLASSES_SWITCHES

#include "firebird.h"
#include "../common/classes/fb_string.h"

class Switches
{
public:
	// switch name and state table.  This structure should be used in all
	// command line tools to facilitate parsing options.
	struct in_sw_tab_t
	{
		int in_sw;							// key of the entry; never zero for valid items
		int in_spb_sw;						// value of service SPB item, if applicable
		const TEXT* in_sw_name;				// name of the option to check against an argument
		SINT64 in_sw_value;					// alice specific field
		SINT64 in_sw_requires;				// alice specific field
		SINT64 in_sw_incompatibilities;		// alice specific field
		bool in_sw_state;					// burp specific field: was the item found in the arguments?
		USHORT in_sw_msg;					// msg # in the msg db for the respective facility, if any
		USHORT in_sw_min_length;			// minimal length of an option; set for a whole table
											// or leave as zero for the whole table (one char matches)
		const TEXT* in_sw_text;				// dudley and gpre specific field: hardcoded explanation
											// for the option; prefer in_sw_msg for localized strings.
		int in_sw_optype;					// some utilities have options that fit into categories
											// to be shown in the built-in help for more clarity.
		// Some compilers may produce warnings because I only initialized this field in gbak and nbackup
	};

	// Take a switches table, the number of elements (assume the last elem is a terminator),
	// whether to copy the table to a modifiable internal table and if there's a minimal length for
	// each option (in other case it checks the first letter of an option).
	// Additionally it precalculates the options' lengths and checks that their minimal lengths
	// are valid (cannot be bigger than the whole string).
	Switches(const in_sw_tab_t* table, size_t count, bool copy, bool minLength);
	~Switches();

	// Given an argument, try to locate it in the table's in_sw_name data member. Empty strings
	// and strings that don't start by an hyphen are discarded silently (function returns NULL).
	// Strings prefixed by an hyphen that are not found cause invalidSwitchInd to be activated
	// if the pointer is provided.
	const in_sw_tab_t* findSwitch(Firebird::string sw, bool* invalidSwitchInd = 0) const;

	// Same as the previous, but it returns a modifiable item. Beware it throws system_call_failed
	// if "copy" was false in the constructor (no modifiable table was created).
	in_sw_tab_t* findSwitchMod(Firebird::string& sw, bool* invalidSwitchInd = 0);

	// Get the same unmodifiable table that was passed as parameter to the constructor.
	const in_sw_tab_t* getTable() const;

	// Retrieve the modifiable copy of the table that was passed as parameter to the constructor.
	// It throws system_call_failed if "copy" was false in the constructor.
	in_sw_tab_t* getTableMod();

	// Looks for an item by key (data member in_sw) and sets its in_sw_state to true.
	// It throws system_call_failed if "copy" was false in the constructor or if the key is
	// not found or it's negative or zero.
	void activate(const int in_sw);

	// Checks if an item by key exists in a list of strings that represent the set of command-line
	// arguments. This is more eficient than finding the key for each parameter and then checking
	// if it's the one we want (we reduce loop from Nkeys*Nargs to Nkeys+Nargs in the worst case).
	// It throws system_call_failed if the key is not found or it's negative or zero.
	bool exists(const int in_sw, const char* const* argv, const int start, const int stop) const;

	// Returns the switch's default text given the switch's numeric value (tag).
	// (There may be more than one spelling for the same switch.)
	// Will throw system_call_failed if tag is not found.
	const char* findNameByTag(const int in_sw) const;

private:
	// Auxiliar function for findSwitch, findSwitchMod and exists().
	static bool matchSwitch(const Firebird::string& sw, const char* target, size_t n);
	// Auxiliar function for exists().
	const in_sw_tab_t* findByTag(const int in_sw, size_t* pos = 0, bool rejectAmbiguity = true) const;
	// Shortcut to raise exceptions
	static void complain(const char* msg);

	const in_sw_tab_t* const m_base;	// pointer to the original table
	const size_t	m_count;			// count of elements (terminator element included)
	const bool		m_copy;				// was m_base copied into m_table for modifications?
	const bool		m_minLength;		// is the field in_sw_min_length meaningful?
	in_sw_tab_t*	m_table;			// modifiable copy
	size_t*			m_opLengths;		// array of in_sw_name's lengths to avoid recalculation
};

#endif // CLASSES_SWITCHES
