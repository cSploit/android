/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gpreswi.h
 *	DESCRIPTION:	Gpre Command Switch definitions
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
 * Revision 1.2  2000/11/16 15:54:29  fsg
 * Added new switch -verbose to gpre that will dump
 * parsed lines to stderr
 *
 * Fixed gpre bug in handling row names in WHERE clauses
 * that are reserved words now (DATE etc)
 * (this caused gpre to dump core when parsing tan.e)
 *
 * Fixed gpre bug in handling lower case table aliases
 * in WHERE clauses for sql dialect 2 and 3.
 * (cause a core dump in a test case from C.R. Zamana)
 *
 * Added support for RM/Cobol
 * SWB 31.Aug.2006
 *
 * Added support for user specifiable date format for Cobol programs
 * SWB 15.Sep.2006
 *
 * Added -noqli switch to suppress parsing of QLI keywords.  This was
 * causing problems with Cobol programs since the QLI and Cobol reserved word
 * lists intersect.
 * Stephen W. Boyd 21.Mar.2007
 */


// Switch handling constants.  Note that IN_SW_COUNT must always be
// one larger than the largest switch value

enum gpre_cmd_switch
{
	IN_SW_GPRE_0,		// not a known switch
	IN_SW_GPRE_C,		// source is C
	IN_SW_GPRE_D,		// SQL database declaration
	IN_SW_GPRE_E,		// accept either case
	IN_SW_GPRE_F,		// source is FORTRAN
	IN_SW_GPRE_G,		// internal GDS module
	IN_SW_GPRE_GXX,		// internal GDS C++ module
	IN_SW_GPRE_I,		// use ID's rather than names
	IN_SW_GPRE_M,		// don't generate READY/START_TRANS
	IN_SW_GPRE_N,		// don't generate debug lines
	IN_SW_GPRE_O,		// write output to standard out
	IN_SW_GPRE_P,		// source is PASCAL
	IN_SW_GPRE_R,		// generate raw BLR
	IN_SW_GPRE_S,		// generate normal, rather than C strings
	IN_SW_GPRE_T,		// print trace messages
	IN_SW_GPRE_X,		// database is EXTERNAL (used with /DATABASE only)
	IN_SW_GPRE_COB,		// source is (shudder) cobol
	IN_SW_GPRE_ANSI, 	// source is (worse and worse!) ansi cobol format
	IN_SW_GPRE_ADA,		// source is ada
	IN_SW_GPRE_HANDLES,	// ada handles package
	IN_SW_GPRE_Z, 		// print software version
	IN_SW_GPRE_D_FLOAT, // use blr_d_float for doubles
	IN_SW_GPRE_CXX, 	// source is C++
	IN_SW_GPRE_SCXX, 	// source is C++ with Saber extension
	IN_SW_GPRE_OCXX,	// C++ with object API
	IN_SW_GPRE_SQLDA, 	// use old or new SQLDA
	IN_SW_GPRE_USER, 	// default username to use when attaching database
	IN_SW_GPRE_PASSWORD, // default password to use in attaching database
	IN_SW_GPRE_INTERP,	// default character set/collation

	IN_SW_GPRE_CPLUSPLUS,	// source is platform dependant C++
	IN_SW_GPRE_SQLDIALECT,	// SQL dialect passed

	// Added this to let gpre dump the lines that it is parsing to stderr
	// FSG 14.Nov.2000

	IN_SW_GPRE_VERBOSE,

	// Added this to allow boot version of gpre to only resolve queries
	// against internal metadata

	IN_SW_GPRE_LANG_INTERNAL,

	// Added this to allow in builds to specify the compiletime database directory
	// without the need to edit the epp source

	IN_SW_GPRE_BASE,

	// Added to allow generation of RM/Cobol compatible code
	// SWB 31.Aug.2006

	IN_SW_GPRE_RMCOBOL,

	// Added to allow specification of a Cobol date format, other than ISC_QUAD, to be used to
	// deliver dates to Cobol programs

	IN_SW_GPRE_DATE_FMT,

	// Added to allow QLI processing to be suppressed.  QLI and COBOL reserved word lists
	// intersect and since QLI reserved words are recognized no matter where they occur in the
	// source stream this was causing bogus errors while parsing COBOL programs.

	IN_SW_NO_QLI,
#ifdef TRUSTED_AUTH

	// Added to allow trusted authentication even with ISC_USER set in environment

	IN_SW_GPRE_TRUSTED,
#endif
	IN_SW_GPRE_FETCH_PASS, // fetch password to use from file

	// As mentioned above: This should always be one larger than the largest
	// switch value.
	// FSG 14.Nov.2000


	IN_SW_GPRE_COUNT 	// number of IN_SW values

};	// enum gpre_cmd_switch


// Dynamic switch table type declaration.  Used to indicate that a
// switch was set.  Allocated in MAIN.


struct sw_tab_t
{
	gpre_cmd_switch sw_in_sw;
};


const static Switches::in_sw_tab_t gpre_in_sw_table[] =
{
#ifdef GPRE_ADA
	{IN_SW_GPRE_ADA		, 0, "ADA"			, 0, 0, 0, false, 0, 0, "\t\textended ADA program"},
	{IN_SW_GPRE_HANDLES	, 0, "HANDLES"		, 0, 0, 0, false, 0, 0, "\t\tADA handle package requires handle package name"},
#endif
	{IN_SW_GPRE_C		, 0, "C"			, 0, 0, 0, false, 0, 0, "\t\textended C program"},
	{IN_SW_GPRE_CXX		, 0, "CXX"			, 0, 0, 0, false, 0, 0, "\t\textended C++ program"},
	{IN_SW_GPRE_CPLUSPLUS, 0, "CPLUSPLUS"	, 0, 0, 0, false, 0, 0, "\textended C++ program"},
	{IN_SW_GPRE_D		, 0, "DATABASE"		, 0, 0, 0, false, 0, 0, "\tdatabase declaration requires database name"},

	{IN_SW_GPRE_D_FLOAT	, 0, "D_FLOAT"		, 0, 0, 0, false, 0, 0, "\t\tgenerate blr_d_float for doubles"},
	{IN_SW_GPRE_E		, 0, "EITHER_CASE"	, 0, 0, 0, false, 0, 0, "\taccept upper or lower case DML in C"},
#ifdef GPRE_FORTRAN
	{IN_SW_GPRE_F		, 0, "FORTRAN"		, 0, 0, 0, false, 0, 0, "\t\textended FORTRAN program"},
#endif
	{IN_SW_GPRE_FETCH_PASS, 0, "FETCH_PASSWORD", 0, 0, 0, false, 0, 0, "\tfetch password from file"},
	{IN_SW_GPRE_G		, 0, "GDS"			, 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_GXX		, 0, "GDS_CXX"		, 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_I		, 0, "IDENTIFIERS"	, 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_I		, 0, "IDS"			, 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_INTERP	, 0, "CHARSET"		, 0, 0, 0, false, 0, 0, "\t\tDefault character set & format"},
	{IN_SW_GPRE_INTERP	, 0, "INTERPRETATION", 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_LANG_INTERNAL , 0, "LANG_INTERNAL"	, 0, 0, 0, false, 0, 0, "\tinternal language only"},
	{IN_SW_GPRE_M		, 0, "MANUAL"		, 0, 0, 0, false, 0, 0, "\t\tdo not automatically ATTACH to a database"},
	{IN_SW_GPRE_N		, 0, "NO_LINES"		, 0, 0, 0, false, 0, 0, "\tdo not generate C debug lines"},
	{IN_SW_GPRE_O		, 0, "OUTPUT"		, 0, 0, 0, false, 0, 0, "\t\tsend output to standard out"},
	{IN_SW_GPRE_OCXX	, 0, "OCXX"			, 0, 0, 0, false, 0, 0, "\t\textended C++ program with objects API"},
#ifdef GPRE_PASCAL
	{IN_SW_GPRE_P		, 0, "PASCAL"		, 0, 0, 0, false, 0, 0, "\t\textended PASCAL program"},
#endif
	{IN_SW_GPRE_PASSWORD, 0, "PASSWORD"		, 0, 0, 0, false, 0, 0, "\tdefault password"},
	{IN_SW_GPRE_R		, 0, "RAW"			, 0, 0, 0, false, 0, 0, "\t\tgenerate unformatted binary BLR"},
	{IN_SW_GPRE_SQLDIALECT, 0, "SQL_DIALECT", 0, 0, 0, false, 0, 0, "\tSQL dialect to use"},
	{IN_SW_GPRE_S		, 0, "STRINGS"		, 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_GPRE_SQLDA	, 0, "SQLDA"		, 0, 0, 0, false, 0, 0, "\t\t***** Deprecated feature. ********"},
	{IN_SW_GPRE_T		, 0, "TRACE"		, 0, 0, 0, false, 0, 0, NULL},
#ifdef TRUSTED_AUTH
	{IN_SW_GPRE_TRUSTED	, 0, "TRUSTED"		, 0, 0, 0, false, 0, 0, "\t\tuse trusted authentication"},
#endif
	{IN_SW_GPRE_USER	, 0, "USER"			, 0, 0, 0, false, 0, 0, "\t\tdefault user name"},
	// FSG 14.Nov.2000
	{IN_SW_GPRE_VERBOSE	, 0, "VERBOSE"		, 0, 0, 0, false, 0, 0, "\t\tVerbose Output to stderr"},
	{IN_SW_GPRE_X		, 0, "X"			, 0, 0, 0, false, 0, 0, "\t\tEXTERNAL database (used with -DATABASE)"},
#ifdef GPRE_COBOL
	{IN_SW_GPRE_COB		, 0, "COB"			, 0, 0, 0, false, 0, 0, "\t\textended COBOL program"},
	{IN_SW_GPRE_ANSI	, 0, "ANSI"			, 0, 0, 0, false, 0, 0, "\t\tgenerate ANSI85 compatible COBOL"},
	{IN_SW_GPRE_RMCOBOL	, 0, "RMC"			, 0, 0, 0, false, 0, 0, "\t\tRM/Cobol"},
#endif
	{IN_SW_GPRE_Z		, 0, "Z"			, 0, 0, 0, false, 0, 0, "\t\tprint software version"},
	{IN_SW_GPRE_BASE	, 0, "BASE"			, 0, 0, 0, false, 0, 0, "\t\tbase directory for compiletime DB"},
#ifdef GPRE_COBOL
	{IN_SW_GPRE_DATE_FMT, 0, "DFM"			, 0, 0, 0, false, 0, 0, "\t\tCobol date format"},
#endif
	{IN_SW_NO_QLI		, 0, "NOQLI"		, 0, 0, 0, false, 0, 0, "\t\tsupress QLI syntax"},
	{IN_SW_GPRE_0		, 0, NULL			, 0, 0, 0, false, 0, 0, NULL}
};
