# -*- coding: ISO-8859-1 -*-

Firebird INTL
=============

Author: Adriano dos Santos Fernandes <adrianosf at uol.com.br>


Architecture
------------

Firebird allow you to specify character sets and collations in every field/variable declaration.
You can also specify the default character set at database create time and every CHAR/VARCHAR
declaration that omits the character set will use it.

At attachment time you can specify the character set that the client want to read all the strings.
If you don't specify one, NONE is assumed.

There are two specials character sets: NONE and OCTETS.
Both can be used in declarations but OCTETS can't be used in attachment.
They are very similar with the exception that space of NONE is ASCII 0x20 and space of OCTETS
is 0x00.
They are specials because they don't follow the rule of others character sets regarding conversions.
With others character sets conversion is performed with CHARSET1->UNICODE->CHARSET2. With
NONE/OCTETS the bytes are just copied: NONE/OCTETS->CHARSET2 and CHARSET1->NONE/OCTETS.


Enhancements
------------


	Well-formedness checks
	----------------------

	Some character sets (specially multi-byte) don't accept everything.
	Now, the engine verifies if strings are wellformed when assigning from NONE/OCTETS and strings
	sent by the client (the statement string and parameters).


	Uppercase
	---------
	
	In FB 1.5.X only ASCII characters are uppercased in character sets default collation order
	(without collation specified). Ex:
	
	isql -q -ch dos850
	SQL> create database 'test.fdb';
	SQL> create table t (c char(1) character set dos850);
	SQL> insert into t values ('a');
	SQL> insert into t values ('e');
	SQL> insert into t values ('á');
	SQL> insert into t values ('é');
	SQL> 
	SQL> select c, upper(c) from t;

	C      UPPER
	====== ======
	a      A
	e      E
	á      á
	é      é

	In FB 2.0 the result is:
	
	C      UPPER
	====== ======
	a      A
	e      E
	á      Á
	é      É


	Maximum string length
	---------------------

	In FB 1.5.X the engine doesn't verify logical length of MBCS strings.
	Hence a UNICODE_FSS field can accept three (maximum length of one UNICODE_FSS character) times more characters than what's declared in the field size.
	For compatibility purpose this was maintained for legacy character sets but new character sets (UTF8, for example) don't suffer from this problem.


	NONE as attachment character set
	--------------------------------
	
	When NONE is used as attachment character set, the sqlsubtype member of XSQLVAR has the character set number of the read field, instead of always 0 as in previous versions.
	
	
	BLOBs and collations
	--------------------
	
	Allow usage of DML COLLATE clause with BLOBs. Ex:
	select blob_column from table where blob_column collate unicode = 'foo';
	
	
New character sets and collations
---------------------------------


	UTF8 character set
	------------------

	The UNICODE_FSS character set has a number of problems: it's an old version of UTF8, accepts
	malformed strings and doesn't enforce correct maximum string length. In FB 1.5.X UTF8 it's an
	alias to UNICODE_FSS.
	Now UTF8 is a new character set, without these problems of UNICODE_FSS.


	UNICODE collations (for UTF8)
	-----------------------------
	
	UCS_BASIC works identical as UTF8 without collation specified (sorts in UNICODE code-point order).
	UNICODE sorts using UCA (Unicode Collation Algorithm).
	Sort order sample:
	
	isql -q -ch dos850
	SQL> create database 'test.fdb';
	SQL> create table t (c char(1) character set utf8);
	SQL> insert into t values ('a');
	SQL> insert into t values ('A');
	SQL> insert into t values ('á');
	SQL> insert into t values ('b');
	SQL> insert into t values ('B');
	SQL> select * from t order by c collate ucs_basic;
	
	C
	======
	A
	B
	a
	b
	á
	
	SQL> select * from t order by c collate unicode;
	
	C
	======
	a
	A
	á
	b
	B
	
	
	Brazilian collations
	--------------------

	Two case-insensitive/accent-insensitive collations were created for Brazil: PT_BR/WIN_PTBR
	(for WIN1252) and PT_BR (for ISO8859_1).
	Sort order and equality sample:

	isql -q -ch dos850
	SQL> create database 'test.fdb';
	SQL> create table t (c char(1) character set iso8859_1 collate pt_br);
	SQL> insert into t values ('a');
	SQL> insert into t values ('A');
	SQL> insert into t values ('á');
	SQL> insert into t values ('b');
	SQL> select * from t order by c;
	
	C
	======
	A
	a
	á
	b
	
	SQL> select * from t where c = 'â';
	
	C
	======
	a
	A
	á


Drivers
-------

New character sets and collations are implemented through dynamic libraries and installed in the server with a manifest file in intl subdirectory. For an example see fbintl.conf.
Not all implemented character sets and collations need to be listed in the manifest file. Only those listed are available and duplications are not loaded.

After installed in the server, they should be registered in the database's system tables (rdb$character_sets and rdb$collations).
One script file with stored procedures to register/unregister is provided in misc/intl.sql.

In FB 2.1, don't use misc/intl.sql for collations anymore, now a DDL command exists for this task.

Syntax:
	CREATE COLLATION <name>
		FOR <charset>
		[ FROM <base> | FROM EXTERNAL ('<name>') ]
		[ NO PAD | PAD SPACE ]
		[ CASE SENSITIVE | CASE INSENSITIVE ]
		[ ACCENT SENSITIVE | ACCENT INSENSITIVE ]
		[ '<specific-attributes>' ]

Notes:
	1) Specific attributes should be separated by semicolon and are case sensitive.

Examples:
	1) CREATE COLLATION UNICODE_ENUS_CI
			FOR UTF8
			FROM UNICODE
			CASE INSENSITIVE
			'LOCALE=en_US';

	2) CREATE COLLATION NEW_COLLATION
			FOR WIN1252
			PAD SPACE;
		-- NEW_COLLATION should be declared in .conf file in $root/intl directory


ICU character sets
------------------

All non-wide and ascii-based character sets present in ICU can be used by Firebird.
But for small distribution kit, we customize ICU to include only essential character sets or the
ones we had feature request.
If the character set you need is not included in it, you can replace ICU libraries by a complete
one found in our site or installed in your OS.

To use, you first need to register it in intl/fbintl.conf as follow:
	<charset NAME>
		intl_module	fbintl
		collation	NAME [REAL-NAME]
	</charset>

And register it in the databases using sp_register_character_set (found in misc/intl.sql).
You need to know how much bytes a single character can occupy in the encoding.

Example:
	<charset GB>
		intl_module	fbintl
		collation	GB GB18030
	</charset>

	execute procedure sp_register_character_set ('GB', 4);


UNICODE collation
-----------------

You can use unicode collation (case sensitive and case insensitive) in all character sets present
in fbintl.
They're already registerd in fbintl.conf, but you need to register in the databases you want with
the attributes you desire.
They should use this name convention: charset_collation. Ex:
	create collation win1252_unicode
		for win1252;
	create collation win1252_unicode_ci
		for win1252
		from win1252_unicode
		case insensitive;

Note the name should be as in fbintl.conf (i.e. ISO8859_1 instead of ISO88591, for example).


Specific attributes
-------------------

Note that some attributes may not report error but not work with some collations.

DISABLE-COMPRESSIONS: Disable compressions (aka contractions) changing the order of a group of characters.
Valid for: collations of narrow character sets.
Format: DISABLE-COMPRESSIONS={0 | 1}
Example: DISABLE-COMPRESSIONS=1

DISABLE-EXPANSIONS: Disable expansions changing the order of a character to sort as a group of characters.
Valid for: collations of narrow character sets.
Format: DISABLE-EXPANSIONS={0 | 1}
Example: DISABLE-EXPANSIONS=1

ICU-VERSION: Specify what version of ICU library will be used. Valid values are the ones defined in
the config file (intl/fbintl.conf) in entry intl_module/icu_versions.
Valid for: UNICODE collations.
Format: ICU-VERSION={default | major.minor}
Example: ICU-VERSION=3.0

LOCALE: Specify the collation locale.
Valid for: UNICODE collations.
Requires: Complete version of ICU libraries.
Format: LOCALE=xx_XX
Example: LOCALE=en_US

MULTI-LEVEL: Uses more than one level for ordering purposes.
Valid for: collations of narrow character sets.
Format: MULTI-LEVEL={0 | 1}
Example: MULTI-LEVEL=1

SPECIALS-FIRST: Order specials characters (spaces, symbols, etc) before alphanumeric characters.
Valid for: collations of narrow character sets.
Format: SPECIALS-FIRST={0 | 1}
Example: SPECIALS-FIRST=1


NUMERIC-SORT: Specify how numbers are sorted.
Valid for: UNICODE collations.
Format: NUMERIC-SORT={0 | 1} (0 is the default)

NUMERIC-SORT=0 sorts number in alphabetic order. Example:
1
10
100
2
20

NUMERIC-SORT=1 sorts number in numeric order. Example:
1
2
10
20
100


Collation changes
-----------------

ES_ES (as well as new ES_ES_CI_AI) collation automatically uses attributes DISABLE-COMPRESSIONS=1;SPECIALS-FIRST=1.
The attributes are stored at database creation time, so databases with ODS < 11.1 do not use it.


Appendix
--------

Narrow character sets: CYRL, DOS437, DOS737, DOS775, DOS850, DOS852, DOS857, DOS858, DOS860, DOS861, DOS862, DOS863, DOS864, DOS865,
DOS866, DOS869, ISO8859_1, ISO8859_13, ISO8859_2, ISO8859_3, ISO8859_4, ISO8859_5, ISO8859_6, ISO8859_7, ISO8859_8, ISO8859_9, KOI8R,
KOI8U, NEXT, TIS620, WIN1250, WIN1251, WIN1252, WIN1253, WIN1254, WIN1255, WIN1256, WIN1257 and WIN1258.

ICU character sets: 
UTF-8 ibm-1208 ibm-1209 ibm-5304 ibm-5305 windows-65001 cp1208 
UTF-16 ISO-10646-UCS-2 unicode csUnicode ucs-2 
UTF-16BE x-utf-16be ibm-1200 ibm-1201 ibm-5297 ibm-13488 ibm-17584 windows-1201 cp1200 cp1201 UTF16_BigEndian 
UTF-16LE x-utf-16le ibm-1202 ibm-13490 ibm-17586 UTF16_LittleEndian windows-1200 
UTF-32 ISO-10646-UCS-4 csUCS4 ucs-4 
UTF-32BE UTF32_BigEndian ibm-1232 ibm-1233 
UTF-32LE UTF32_LittleEndian ibm-1234 
UTF16_PlatformEndian 
UTF16_OppositeEndian 
UTF32_PlatformEndian 
UTF32_OppositeEndian 
UTF-7 windows-65000 
IMAP-mailbox-name 
SCSU 
BOCU-1 csBOCU-1 
CESU-8 
ISO-8859-1 ibm-819 IBM819 cp819 latin1 8859_1 csISOLatin1 iso-ir-100 ISO_8859-1:1987 l1 819 
US-ASCII ASCII ANSI_X3.4-1968 ANSI_X3.4-1986 ISO_646.irv:1991 iso_646.irv:1983 ISO646-US us csASCII iso-ir-6 cp367 ascii7 646 windows-20127 
ISO_2022,locale=ja,version=0 ISO-2022-JP csISO2022JP 
ISO_2022,locale=ja,version=1 ISO-2022-JP-1 JIS JIS_Encoding 
ISO_2022,locale=ja,version=2 ISO-2022-JP-2 csISO2022JP2 
ISO_2022,locale=ja,version=3 JIS7 csJISEncoding 
ISO_2022,locale=ja,version=4 JIS8 
ISO_2022,locale=ko,version=0 ISO-2022-KR csISO2022KR 
ISO_2022,locale=ko,version=1 ibm-25546 
ISO_2022,locale=zh,version=0 ISO-2022-CN 
ISO_2022,locale=zh,version=1 ISO-2022-CN-EXT 
HZ HZ-GB-2312 
ISCII,version=0 x-iscii-de windows-57002 iscii-dev 
ISCII,version=1 x-iscii-be windows-57003 iscii-bng windows-57006 x-iscii-as 
ISCII,version=2 x-iscii-pa windows-57011 iscii-gur 
ISCII,version=3 x-iscii-gu windows-57010 iscii-guj 
ISCII,version=4 x-iscii-or windows-57007 iscii-ori 
ISCII,version=5 x-iscii-ta windows-57004 iscii-tml 
ISCII,version=6 x-iscii-te windows-57005 iscii-tlg 
ISCII,version=7 x-iscii-ka windows-57008 iscii-knd 
ISCII,version=8 x-iscii-ma windows-57009 iscii-mlm 
gb18030 ibm-1392 windows-54936 
LMBCS-1 lmbcs 
LMBCS-2 
LMBCS-3 
LMBCS-4 
LMBCS-5 
LMBCS-6 
LMBCS-8 
LMBCS-11 
LMBCS-16 
LMBCS-17 
LMBCS-18 
LMBCS-19 
ibm-367_P100-1995 ibm-367 IBM367 
ibm-912_P100-1995 ibm-912 iso-8859-2 ISO_8859-2:1987 latin2 csISOLatin2 iso-ir-101 l2 8859_2 cp912 912 windows-28592 
ibm-913_P100-2000 ibm-913 iso-8859-3 ISO_8859-3:1988 latin3 csISOLatin3 iso-ir-109 l3 8859_3 cp913 913 windows-28593 
ibm-914_P100-1995 ibm-914 iso-8859-4 latin4 csISOLatin4 iso-ir-110 ISO_8859-4:1988 l4 8859_4 cp914 914 windows-28594 
ibm-915_P100-1995 ibm-915 iso-8859-5 cyrillic csISOLatinCyrillic iso-ir-144 ISO_8859-5:1988 8859_5 cp915 915 windows-28595 
ibm-1089_P100-1995 ibm-1089 iso-8859-6 arabic csISOLatinArabic iso-ir-127 ISO_8859-6:1987 ECMA-114 ASMO-708 8859_6 cp1089 1089 windows-28596 ISO-8859-6-I ISO-8859-6-E 
ibm-813_P100-1995 ibm-813 iso-8859-7 greek greek8 ELOT_928 ECMA-118 csISOLatinGreek iso-ir-126 ISO_8859-7:1987 8859_7 cp813 813 windows-28597 
ibm-916_P100-1995 ibm-916 iso-8859-8 hebrew csISOLatinHebrew iso-ir-138 ISO_8859-8:1988 ISO-8859-8-I ISO-8859-8-E 8859_8 cp916 916 windows-28598 
ibm-920_P100-1995 ibm-920 iso-8859-9 latin5 csISOLatin5 iso-ir-148 ISO_8859-9:1989 l5 8859_9 cp920 920 windows-28599 ECMA-128 
ibm-921_P100-1995 ibm-921 iso-8859-13 8859_13 cp921 921 
ibm-923_P100-1998 ibm-923 iso-8859-15 Latin-9 l9 8859_15 latin0 csisolatin0 csisolatin9 iso8859_15_fdis cp923 923 windows-28605 
ibm-942_P12A-1999 ibm-942 ibm-932 cp932 shift_jis78 sjis78 ibm-942_VSUB_VPUA ibm-932_VSUB_VPUA 
ibm-943_P15A-2003 ibm-943 Shift_JIS MS_Kanji csShiftJIS windows-31j csWindows31J x-sjis x-ms-cp932 cp932 windows-932 cp943c IBM-943C ms932 pck sjis ibm-943_VSUB_VPUA 
ibm-943_P130-1999 ibm-943 Shift_JIS cp943 943 ibm-943_VASCII_VSUB_VPUA 
ibm-33722_P12A-1999 ibm-33722 ibm-5050 EUC-JP Extended_UNIX_Code_Packed_Format_for_Japanese csEUCPkdFmtJapanese X-EUC-JP eucjis windows-51932 ibm-33722_VPUA IBM-eucJP 
ibm-33722_P120-1999 ibm-33722 ibm-5050 cp33722 33722 ibm-33722_VASCII_VPUA 
ibm-954_P101-2000 ibm-954 EUC-JP 
ibm-1373_P100-2002 ibm-1373 windows-950 
windows-950-2000 Big5 csBig5 windows-950 x-big5 
ibm-950_P110-1999 ibm-950 cp950 950 
macos-2566-10.2 Big5-HKSCS big5hk HKSCS-BIG5 
ibm-1375_P100-2003 ibm-1375 Big5-HKSCS 
ibm-1386_P100-2002 ibm-1386 cp1386 windows-936 ibm-1386_VSUB_VPUA 
windows-936-2000 GBK CP936 MS936 windows-936 
ibm-1383_P110-1999 ibm-1383 GB2312 csGB2312 EUC-CN ibm-eucCN hp15CN cp1383 1383 ibm-1383_VPUA 
ibm-5478_P100-1995 ibm-5478 GB_2312-80 chinese iso-ir-58 csISO58GB231280 gb2312-1980 GB2312.1980-0 
ibm-964_P110-1999 ibm-964 EUC-TW ibm-eucTW cns11643 cp964 964 ibm-964_VPUA 
ibm-949_P110-1999 ibm-949 cp949 949 ibm-949_VASCII_VSUB_VPUA 
ibm-949_P11A-1999 ibm-949 cp949c ibm-949_VSUB_VPUA 
ibm-970_P110-1995 ibm-970 EUC-KR KS_C_5601-1987 windows-51949 csEUCKR ibm-eucKR KSC_5601 5601 ibm-970_VPUA 
ibm-971_P100-1995 ibm-971 ibm-971_VPUA 
ibm-1363_P11B-1998 ibm-1363 KS_C_5601-1987 KS_C_5601-1989 KSC_5601 csKSC56011987 korean iso-ir-149 5601 cp1363 ksc windows-949 ibm-1363_VSUB_VPUA 
ibm-1363_P110-1997 ibm-1363 ibm-1363_VASCII_VSUB_VPUA 
windows-949-2000 windows-949 KS_C_5601-1987 KS_C_5601-1989 KSC_5601 csKSC56011987 korean iso-ir-149 ms949 
ibm-1162_P100-1999 ibm-1162 
ibm-874_P100-1995 ibm-874 ibm-9066 cp874 TIS-620 tis620.2533 eucTH cp9066 
windows-874-2000 TIS-620 windows-874 MS874 
ibm-437_P100-1995 ibm-437 IBM437 cp437 437 csPC8CodePage437 windows-437 
ibm-850_P100-1995 ibm-850 IBM850 cp850 850 csPC850Multilingual windows-850 
ibm-851_P100-1995 ibm-851 IBM851 cp851 851 csPC851 
ibm-852_P100-1995 ibm-852 IBM852 cp852 852 csPCp852 windows-852 
ibm-855_P100-1995 ibm-855 IBM855 cp855 855 csIBM855 csPCp855 
ibm-856_P100-1995 ibm-856 cp856 856 
ibm-857_P100-1995 ibm-857 IBM857 cp857 857 csIBM857 windows-857 
ibm-858_P100-1997 ibm-858 IBM00858 CCSID00858 CP00858 PC-Multilingual-850+euro cp858 
ibm-860_P100-1995 ibm-860 IBM860 cp860 860 csIBM860 
ibm-861_P100-1995 ibm-861 IBM861 cp861 861 cp-is csIBM861 windows-861 
ibm-862_P100-1995 ibm-862 IBM862 cp862 862 csPC862LatinHebrew DOS-862 windows-862 
ibm-863_P100-1995 ibm-863 IBM863 cp863 863 csIBM863 
ibm-864_X110-1999 ibm-864 IBM864 cp864 csIBM864 
ibm-865_P100-1995 ibm-865 IBM865 cp865 865 csIBM865 
ibm-866_P100-1995 ibm-866 IBM866 cp866 866 csIBM866 windows-866 
ibm-867_P100-1998 ibm-867 cp867 
ibm-868_P100-1995 ibm-868 IBM868 CP868 868 csIBM868 cp-ar 
ibm-869_P100-1995 ibm-869 IBM869 cp869 869 cp-gr csIBM869 windows-869 
ibm-878_P100-1996 ibm-878 KOI8-R koi8 csKOI8R cp878 
ibm-901_P100-1999 ibm-901 
ibm-902_P100-1999 ibm-902 
ibm-922_P100-1999 ibm-922 cp922 922 
ibm-4909_P100-1999 ibm-4909 
ibm-5346_P100-1998 ibm-5346 windows-1250 cp1250 
ibm-5347_P100-1998 ibm-5347 windows-1251 cp1251 
ibm-5348_P100-1997 ibm-5348 windows-1252 cp1252 
ibm-5349_P100-1998 ibm-5349 windows-1253 cp1253 
ibm-5350_P100-1998 ibm-5350 windows-1254 cp1254 
ibm-9447_P100-2002 ibm-9447 windows-1255 cp1255 
windows-1256-2000 windows-1256 cp1256 
ibm-9449_P100-2002 ibm-9449 windows-1257 cp1257 
ibm-5354_P100-1998 ibm-5354 windows-1258 cp1258 
ibm-1250_P100-1995 ibm-1250 windows-1250 
ibm-1251_P100-1995 ibm-1251 windows-1251 
ibm-1252_P100-2000 ibm-1252 windows-1252 
ibm-1253_P100-1995 ibm-1253 windows-1253 
ibm-1254_P100-1995 ibm-1254 windows-1254 
ibm-1255_P100-1995 ibm-1255 
ibm-5351_P100-1998 ibm-5351 windows-1255 
ibm-1256_P110-1997 ibm-1256 
ibm-5352_P100-1998 ibm-5352 windows-1256 
ibm-1257_P100-1995 ibm-1257 
ibm-5353_P100-1998 ibm-5353 windows-1257 
ibm-1258_P100-1997 ibm-1258 windows-1258 
macos-0_2-10.2 macintosh mac csMacintosh windows-10000 
macos-6-10.2 x-mac-greek windows-10006 macgr 
macos-7_3-10.2 x-mac-cyrillic windows-10007 maccy 
macos-29-10.2 x-mac-centraleurroman windows-10029 x-mac-ce macce 
macos-35-10.2 x-mac-turkish windows-10081 mactr 
ibm-1051_P100-1995 ibm-1051 hp-roman8 roman8 r8 csHPRoman8 
ibm-1276_P100-1995 ibm-1276 Adobe-Standard-Encoding csAdobeStandardEncoding 
ibm-1277_P100-1995 ibm-1277 Adobe-Latin1-Encoding 
ibm-1006_P100-1995 ibm-1006 cp1006 1006 
ibm-1098_P100-1995 ibm-1098 cp1098 1098 
ibm-1124_P100-1996 ibm-1124 cp1124 1124 
ibm-1125_P100-1997 ibm-1125 cp1125 
ibm-1129_P100-1997 ibm-1129 
ibm-1131_P100-1997 ibm-1131 cp1131 
ibm-1133_P100-1997 ibm-1133 
ibm-1381_P110-1999 ibm-1381 cp1381 1381 
ibm-37_P100-1995 ibm-37 IBM037 ibm-037 ebcdic-cp-us ebcdic-cp-ca ebcdic-cp-wt ebcdic-cp-nl csIBM037 cp037 037 cpibm37 cp37 
ibm-273_P100-1995 ibm-273 IBM273 CP273 csIBM273 ebcdic-de cpibm273 273 
ibm-277_P100-1995 ibm-277 IBM277 cp277 EBCDIC-CP-DK EBCDIC-CP-NO csIBM277 ebcdic-dk cpibm277 277 
ibm-278_P100-1995 ibm-278 IBM278 cp278 ebcdic-cp-fi ebcdic-cp-se csIBM278 ebcdic-sv cpibm278 278 
ibm-280_P100-1995 ibm-280 IBM280 CP280 ebcdic-cp-it csIBM280 cpibm280 280 
ibm-284_P100-1995 ibm-284 IBM284 CP284 ebcdic-cp-es csIBM284 cpibm284 284 
ibm-285_P100-1995 ibm-285 IBM285 CP285 ebcdic-cp-gb csIBM285 ebcdic-gb cpibm285 285 
ibm-290_P100-1995 ibm-290 IBM290 cp290 EBCDIC-JP-kana csIBM290 
ibm-297_P100-1995 ibm-297 IBM297 cp297 ebcdic-cp-fr csIBM297 cpibm297 297 
ibm-420_X120-1999 ibm-420 IBM420 cp420 ebcdic-cp-ar1 csIBM420 420 
ibm-424_P100-1995 ibm-424 IBM424 cp424 ebcdic-cp-he csIBM424 424 
ibm-500_P100-1995 ibm-500 IBM500 CP500 ebcdic-cp-be csIBM500 ebcdic-cp-ch cpibm500 500 
ibm-803_P100-1999 ibm-803 cp803 
ibm-838_P100-1995 ibm-838 IBM-Thai csIBMThai cp838 838 ibm-9030 
ibm-870_P100-1995 ibm-870 IBM870 CP870 ebcdic-cp-roece ebcdic-cp-yu csIBM870 
ibm-871_P100-1995 ibm-871 IBM871 ebcdic-cp-is csIBM871 CP871 ebcdic-is cpibm871 871 
ibm-875_P100-1995 ibm-875 IBM875 cp875 875 
ibm-918_P100-1995 ibm-918 IBM918 CP918 ebcdic-cp-ar2 csIBM918 
ibm-930_P120-1999 ibm-930 ibm-5026 cp930 cpibm930 930 
ibm-933_P110-1995 ibm-933 cp933 cpibm933 933 
ibm-935_P110-1999 ibm-935 cp935 cpibm935 935 
ibm-937_P110-1999 ibm-937 cp937 cpibm937 937 
ibm-939_P120-1999 ibm-939 ibm-931 ibm-5035 cp939 939 
ibm-1025_P100-1995 ibm-1025 cp1025 1025 
ibm-1026_P100-1995 ibm-1026 IBM1026 CP1026 csIBM1026 1026 
ibm-1047_P100-1995 ibm-1047 IBM1047 cpibm1047 
ibm-1097_P100-1995 ibm-1097 cp1097 1097 
ibm-1112_P100-1995 ibm-1112 cp1112 1112 
ibm-1122_P100-1999 ibm-1122 cp1122 1122 
ibm-1123_P100-1995 ibm-1123 cp1123 1123 cpibm1123 
ibm-1130_P100-1997 ibm-1130 
ibm-1132_P100-1998 ibm-1132 
ibm-1140_P100-1997 ibm-1140 IBM01140 CCSID01140 CP01140 cp1140 cpibm1140 ebcdic-us-37+euro 
ibm-1141_P100-1997 ibm-1141 IBM01141 CCSID01141 CP01141 cp1141 cpibm1141 ebcdic-de-273+euro 
ibm-1142_P100-1997 ibm-1142 IBM01142 CCSID01142 CP01142 cp1142 cpibm1142 ebcdic-dk-277+euro ebcdic-no-277+euro 
ibm-1143_P100-1997 ibm-1143 IBM01143 CCSID01143 CP01143 cp1143 cpibm1143 ebcdic-fi-278+euro ebcdic-se-278+euro 
ibm-1144_P100-1997 ibm-1144 IBM01144 CCSID01144 CP01144 cp1144 cpibm1144 ebcdic-it-280+euro 
ibm-1145_P100-1997 ibm-1145 IBM01145 CCSID01145 CP01145 cp1145 cpibm1145 ebcdic-es-284+euro 
ibm-1146_P100-1997 ibm-1146 IBM01146 CCSID01146 CP01146 cp1146 cpibm1146 ebcdic-gb-285+euro 
ibm-1147_P100-1997 ibm-1147 IBM01147 CCSID01147 CP01147 cp1147 cpibm1147 ebcdic-fr-297+euro 
ibm-1148_P100-1997 ibm-1148 IBM01148 CCSID01148 CP01148 cp1148 cpibm1148 ebcdic-international-500+euro 
ibm-1149_P100-1997 ibm-1149 IBM01149 CCSID01149 CP01149 cp1149 cpibm1149 ebcdic-is-871+euro 
ibm-1153_P100-1999 ibm-1153 cpibm1153 
ibm-1154_P100-1999 ibm-1154 cpibm1154 
ibm-1155_P100-1999 ibm-1155 cpibm1155 
ibm-1156_P100-1999 ibm-1156 cpibm1156 
ibm-1157_P100-1999 ibm-1157 cpibm1157 
ibm-1158_P100-1999 ibm-1158 cpibm1158 
ibm-1160_P100-1999 ibm-1160 cpibm1160 
ibm-1164_P100-1999 ibm-1164 cpibm1164 
ibm-1364_P110-1997 ibm-1364 cp1364 
ibm-1371_P100-1999 ibm-1371 cpibm1371 
ibm-1388_P103-2001 ibm-1388 ibm-9580 
ibm-1390_P110-2003 ibm-1390 cpibm1390 
ibm-1399_P110-2003 ibm-1399 
ibm-16684_P110-2003 ibm-16684 
ibm-4899_P100-1998 ibm-4899 cpibm4899 
ibm-4971_P100-1999 ibm-4971 cpibm4971 
ibm-12712_P100-1998 ibm-12712 cpibm12712 ebcdic-he 
ibm-16804_X110-1999 ibm-16804 cpibm16804 ebcdic-ar 
ibm-1137_P100-1999 ibm-1137 
ibm-5123_P100-1999 ibm-5123 
ibm-8482_P100-1999 ibm-8482 
ibm-37_P100-1995,swaplfnl ibm-37-s390 ibm037-s390 
ibm-1047_P100-1995,swaplfnl ibm-1047-s390 
ibm-1140_P100-1997,swaplfnl ibm-1140-s390 
ibm-1142_P100-1997,swaplfnl ibm-1142-s390 
ibm-1143_P100-1997,swaplfnl ibm-1143-s390 
ibm-1144_P100-1997,swaplfnl ibm-1144-s390 
ibm-1145_P100-1997,swaplfnl ibm-1145-s390 
ibm-1146_P100-1997,swaplfnl ibm-1146-s390 
ibm-1147_P100-1997,swaplfnl ibm-1147-s390 
ibm-1148_P100-1997,swaplfnl ibm-1148-s390 
ibm-1149_P100-1997,swaplfnl ibm-1149-s390 
ibm-1153_P100-1999,swaplfnl ibm-1153-s390 
ibm-12712_P100-1998,swaplfnl ibm-12712-s390 
ibm-16804_X110-1999,swaplfnl ibm-16804-s390 
ebcdic-xml-us 
