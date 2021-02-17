/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		format.h
 *	DESCRIPTION:	Print formatter definitions
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

#ifndef QLI_FORMAT_H
#define QLI_FORMAT_H

/* Picture string elements:

    A		any alpha character (interpreted as X)
    B		blank character
    CR		credit (sign)
    D		day of month (digit)
    DB		debit (sign)
    E		exponent
    G		exponent or decimal format, whichever is shorter
    H		hex digit
    J		day of year (digit)
    M		alpha month
    N		month (numeric)
    P		AM/PM indicator for time
    T		time
    W		weekday (character)
    X		any character
    Y		year (digit)
    Z		digit or leading suppressed digit (blank)
    9		digit
    $		fixed or floating insertion
    *		floating insertion
    ,		literal character or inserted
    .		decimal point
    +		sign (fixed or floating), always present
    -		sign (fixed or floating), suppressed on positive, or literal
    ?		separate between real and missing components of edit string
    "string"	any string
    (( ))	insert parenthesis if negative
    /		literal insertion
    %		literal insertion
    :		literal insertion
*/

enum pic_t {
    pic_alpha = 1,
    pic_numeric,
    pic_date,
    pic_float
    // , pic_text // unused
};

// Picture string handling block

struct pics
{
    blk		pic_header;
    USHORT	pic_print_length;	// Print length of picture string
    const TEXT*	pic_string;		// Address of string
    const TEXT*	pic_pointer;		// Address of string
    USHORT	pic_flags;		// Misc. trash
    USHORT	pic_count;		// Count of repeat characters
    TEXT	pic_character;		// Last significant character
    pic_t	pic_type;		// Type of edit
    USHORT	pic_length;		// Printing columns (MAX of edit_string & missing)
    USHORT	pic_floats;		// Character of floating things
    USHORT	pic_digits;		// Digits of number
    USHORT	pic_hex_digits;		// Hexidecimal digits
    USHORT	pic_fractions;		// Digits after decimal point
    USHORT	pic_chars;		// Insertion characters (alpha)
    USHORT	pic_literals;		// Literal insertion characters
    USHORT	pic_days;		// Digits of day of month
    USHORT	pic_weekdays;		// Characters of weekday
    USHORT	pic_months;		// Characters of alpha month
    USHORT	pic_nmonths;		// Digits of numeric month
    USHORT	pic_years;		// Digits of year
    USHORT	pic_julians;		// Digits of julian days
    USHORT	pic_decimals;		// Number of decimal points (?!)
    USHORT	pic_brackets;		// Pairs of deficit brackets
    USHORT	pic_exponents;		// Exponential indicators
    USHORT	pic_float_digits;	// Floating digits
    USHORT	pic_hours;		// hours
    USHORT	pic_minutes;		// minutes
    USHORT	pic_seconds;		// seconds
    USHORT	pic_meridian;		// AM/PM indicator
    pics*	pic_missing;		// missing value edit string
};

// pic_flags
const USHORT PIC_suppress_blanks	= 1;	// Suppress leading blanks
const USHORT PIC_literal			= 2;	// We're in a quoted string
const USHORT PIC_signed				= 4;	// This numeric edit string has a sign indicator

#endif // QLI_FORMAT_H

