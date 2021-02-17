/*
 *	PROGRAM:	Intl
 *	MODULE:		ld.h
 *	DESCRIPTION:	Language Driver data structures
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - MAC ("MAC", "MAC_AUX" and "MAC_CP" defines)
 *                          - XENIX and OS/2
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "MS_DOS" define
 *
 */

/* ------------NOTE NOTE NOTE -----------
 * This file is shared with the Language Driver kit used by PC/Windows
 * products.  This master is in the PC side kit, currently (931018) stored
 * as DBENGINE:/area6/langdrv/h/ld.h
 * with Martin Levy as module owner
 * The file has been ported from ANSI C++ to K&R C, with minimal changes
 * to support re-porting in the future.
 *   C++ comments are surrounded by C comments.
 *   Prototypes are #ifdef'ed out.
 *   Following is a bunch of remappings of InterBase data types to
 *	the type names as used on the PC
 */

#ifndef INTL_LD_H
#define INTL_LD_H

#ifdef CHAR
#undef CHAR
#endif
#define CHAR	SCHAR

#define UINT16	USHORT

#if defined(WIN_NT)
#define FB_DLL_EXPORT __declspec(dllexport)
#elif defined(DARWIN)
#define FB_DLL_EXPORT API_ROUTINE
#else
#define FB_DLL_EXPORT
#endif


/* Following this line is LD.H from Borland Language Driver Kit */


/*
//-----------------------------------------------------------------
//
// LD.H
//
// Language Driver API header file
//
// 08-17-92 MLevy LdLibraryName[ LD_LIBNAME_LEN ] changed to pLdLibraryName.
// 08-05-92 MLevy Exposed LdExit() again.
// 06-09-92 MLevy ICELAND and BorlPORTUGUESE added.
// 05-20-92 MLevy File made.
//
//-----------------------------------------------------------------
*/

//#ifndef __LD_H
//#define __LD_H

/*
//-----------------------------------------------------------------
// DATA TYPE DEFINITIONS
//-----------------------------------------------------------------
*/

#ifndef TRUE
#define           TRUE 1
#endif
#ifndef FALSE
#define           FALSE 0
#endif

/*
//-----------------------------------------------------------------
// API CONSTANT VALUES
//-----------------------------------------------------------------
*/

//#define YES                       TRUE
//#define NO                        FALSE


/*
// LdObject Windows resource type
*/
//#define LDRESOBJECT            1001
//#define LDOBJ_NAME1             101

//#define LDID                   1002

//#define LDIDRESNAME               "LDID"

/*
// Language type (file or resource)
*/
//#define LDOBJRES                  1
//#define LDOBJFILE                 2


//#define SECONDARY_DIFF          256
//#define TERTIARY_DIFF           512
//#define LENGTH_DIFF            1024

/*
//
// SORT/COLLATION TYPE PROVIDED BY LANGUAGE DRIVER
//
*/
//#define ASCII_SORT                1
//#define CODEPAGE_SORT             ASCII_SORT
//#define DICT_SORT                 2
//#define PHONETIC_SORT             3
//#define NAME_SORT                 4

/*
//
// CODE PAGE/CHARACTER SET PLATFORM TYPES
//
*/
//#define DOS_CP                    1
//#define OEM_CP                    DOS_CP
//#define WIN_CP                    2
//#define ANSI_CP                   WIN_CP

/*
// UNIX etc.
*/
//#define SUNOS_CP                  4
//#define VMS_CP                    5
//#define HPUX_CP                   6
//#define AIX_CP                    8
//#define AUX_CP                    9

/*
// SHIFT-JIS (JAPANESE DBCS)
*/
//#define SJIS_CP                 255

/*
// length of descriptive language driver name (incl zero)
*/
//#define LD_SYMNAME_LEN           20

/*
// max length of language driver signature & file name (incl zero)
*/
//#define LD_SFNAME_LEN            20

/*
// max length of language driver resource file name (incl zero)
*/
//#define LD_LIBNAME_LEN           13

//#define LDSUCCESS                 0
//#define LDFAILURE                -1

#define REVERSE					  0x01
#define EXPAND                    0x02
#define SECONDARY                 0x04
#define LOCAL_EXPAND              0x08
#define LOCAL_SECONDARY           0x10

/*
//
// TABLE INDEX CONSTANTS
//
*/

//#define CHARDEF_TBL               0
//#define UPPERCASE_TBL             1
//#define LOWERCASE_TBL             2
//#define EXPAND_TBL                3
//#define COMPRESS_TBL              4
//#define CASESORT_TBL              5
//#define NOCASESORT_TBL            6
//#define SOUNDEX_TBL               7
//#define LICSCP_TBL                8
//#define CPLICS_TBL                9
//#define PRIMALT_TBL              10
//#define ALTPRIM_TBL              11
//#define BASE_TBL                 12
//#define EOF_VAL                  13

//#define NUMBER_TBLS              EOF_VAL + 1

/*
//-----------------------------------------------------------------
*/

struct SortOrderTblEntry {

	UINT16 Primary:8;
	UINT16 Secondary:4;
	UINT16 Tertiary:2;
	UINT16 IsExpand:1;
	UINT16 IsCompress:1;

};

/*
//
// CHARACTER EXPANSION STRUCTURE
//
*/

struct ExpandChar {

	BYTE Ch;
	BYTE ExpCh1;
	BYTE ExpCh2;
};

/*
//
// CHARACTER PAIR TO SINGLE SORT WEIGHT STRUCTURE
//
*/

struct CompressPair {

	BYTE CharPair[2];
	SortOrderTblEntry CaseWeight;
	SortOrderTblEntry NoCaseWeight;
};


/*
//-----------------------------------------------------------------
*/

/*
//-----------------------------------------------------------------
// CHARACTER/SYMBOL DEFINITION VALUES
//-----------------------------------------------------------------
*/

/*
// NO SPECIAL QUALITIES
*/
//#define CH_UNDEFINED           0x00

/*
// A NUMERIC CHARACTER
*/
//#define CH_DIGIT               0x01

/*
// AN UPPERCASE CHARACTER
*/
//#define CH_UPPER               0x02

/*
// A LOWERCASE CHARACTER
*/
//#define CH_LOWER               0x04

/*
// ALPHA CHARACTER
*/
//#define CH_ALPHA               CH_LOWER + CH_UPPER

/*
// ALPHANUMERIC CHARACTER
*/
//#define CH_ALPHANUM            CH_LOWER + CH_UPPER + CH_DIGIT

/*
//-----------------------------------------------------------------
// ID CODES
//-----------------------------------------------------------------
//
// PRODUCT ID
//
*/
//#define PDOX                    0x1
//#define DBASE                   0x2
//#define IBASE                   0x4
//#define OTHER                   0x8
//#define BORLAND                0x10

/*
//
// COUNTRY/LANGUAGE ID
//
*/

/*
//#define US                        1
//#define CANADA                    2
//#define LATINAMER                 3
//#define NEDERLANDS               31
//#define BELGIUM                  32
//#define FRANCE                   33
//#define SPAIN                    34
//#define HUNGARY                  36
//#define YUGOSLAVIA               38
//#define ITALY                    39
//#define SWITZ                    41
//#define CZECH                    42
//#define UK                       44
//#define DENMARK                  45
//#define SWEDEN                   46
//#define NORWAY                   47
//#define POLAND                   48
//#define GERMANY                  49
//#define BRAZIL                   55
//#define INTL                     61
//#define PORTUGAL                351
//#define FINLAND                 358
//#define JAPAN                    81
//#define ICELAND                 354

//#define NORDAN                    DENMARK + NORWAY
//#define SWEDFIN                   SWEDEN  + FINLAND
*/

/*
//
// UNIQUE LANGUAGE DRIVER ID
//
*/

/*
// Paradox
*/
/*
//#define pxUS                      1
//#define pxINTL                    2
//#define pxJAPANESE                3
//#define pxNORDAN                  4
//#define pxNORDAN4                 5
//#define pxSWEDFIN                 6
*/
/*
// dBASE
*/
/*
//#define dbARABIC                  7
//#define dbDANISH                  8
//#define dbDUTCH                   9
//#define dbDUTCH2                 10
//#define dbFINNISH                11
//#define dbFINNISH2               12
//#define dbFRENCH                 13
//#define dbFRENCH2                14
//#define dbGERMAN                 15
//#define dbGERMAN2                16
//#define dbITALIAN                17
//#define dbITALIAN2               18
//#define dbJAPANESE               19
//#define dbSPANISH2               20
//#define dbSWEDISH                21
//#define dbSWEDISH2               22
//#define dbNORWEGIAN              23
//#define dbSPANISH                24
//#define dbUK                     25
//#define dbUK2                    26
//#define dbUS                     27
//#define dbFRENCHCAN              28
//#define dbFRENCHCAN2             29
//#define dbFRENCHCAN3             30
//#define dbCZECH                  31
//#define dbCZECH2                 32
//#define dbGREEK                  33
//#define dbHUNGARIAN              34
//#define dbPOLISH                 35
//#define dbPORTUGUESE             36
//#define dbPORTUGUESE2            37
//#define dbRUSSIAN                38
*/
/*
// Borland
*/
/*
//#define BorlDANISH               39
//#define BorlDUTCH                40
//#define BorlFINNISH              41
//#define BorlFRENCH               42
//#define BorlCANADIAN             43
//#define BorlGERMAN               44
//#define BorlICELANDIC            45
//#define BorlITALIAN              46
//#define BorlJAPANESE             47
//#define BorlNORWEGIAN            48
//#define BorlSPANISH              49
//#define BorlSPANISH2             50
//#define BorlSWEDISH              51
//#define BorlUK                   52
//#define BorlUS                   53
//#define BorlPORTUGUESE           54
//#define dbUS2                    55
*/
/*
// User Defined 201 - 254
*/
/*
//#define USERDEFINED_MIN         201
//#define USERDEFINED_MAX         255
*/

/*
//-----------------------------------------------------------------
// ERROR CODES
//-----------------------------------------------------------------
*/

//#define ERRBASE_LDAPI             0x7000

/*
// no errors at all
*/
//#define LDERR_NONE                LDSUCCESS

/*
// LD memory could be allocated
*/
//#define LDERR_NOMEMORY            ERRBASE_LDAPI + 0xA1

/*
// LD file corrupt/could not be read
*/
//#define LDERR_FILECORRUPT         ERRBASE_LDAPI + 0xA2

/*
// LD version mismatch
*/
//#define LDERR_OLDVERSION          ERRBASE_LDAPI + 0xA3

/*
// LD table not supported
*/
//#define LDERR_NOTSUPPORTED        ERRBASE_LDAPI + 0xA4

/*
// LD file not accessible
*/
//#define LDERR_FILEOPEN            ERRBASE_LDAPI + 0xA5

/*
// LD resource not accessible
*/
//#define LDERR_RESOPEN             ERRBASE_LDAPI + 0xA6

/*
// unknown
*/
//#define LDERR_UNKNOWN             ERRBASE_LDAPI + 0xA7

//#endif


#endif /* INTL_LD_H */
