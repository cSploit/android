/* 
 * These are the canonical names for character sets accepted by GNU iconv.  
 * See its documentation for the standard it follows.  
 * 
 * GNU iconv accepts other character set names, too, and your favorite operating system
 * very likely uses still other names to represent the _same_ character set.  
 * 
 * Alternative character set names are mapped to these canonical ones in 
 * alternative_character_sets.h and are accessed with canonical_charset();
 */
		  {"ISO-8859-1",	1, 1}
		, {"UTF-8", 		1, 4}
		, {"UCS-2LE", 		2, 2}
		, {"UCS-2BE", 		2, 2}
		, {"UCS-2",		2, 2}
		, {"US-ASCII",		1, 1}
		, {"UCS-4",		4, 4}
		, {"UCS-4BE", 		4, 4}
		, {"UCS-4LE", 		4, 4}
		, {"UTF-16", 		2, 4}
		, {"UTF-16BE", 		2, 4}
		, {"UTF-16LE", 		2, 4}
		, {"UTF-32", 		4, 4}
		, {"UTF-32BE", 		4, 4}
		, {"UTF-32LE", 		4, 4}
		, {"UTF-7",		1, 4}
		, {"UCS-2-INTERNAL", 	2, 2}
		, {"UCS-2-SWAPPED", 	2, 2}
		, {"UCS-4-INTERNAL", 	4, 4}
		, {"UCS-4-SWAPPED", 	4, 4}
		, {"C99", 		1, 1}
		, {"JAVA", 		1, 1}
		, {"ISO-8859-2", 	1, 1}
		, {"ISO-8859-3", 	1, 1}
		, {"ISO-8859-4", 	1, 1}
		, {"ISO-8859-5",	1, 1}
		, {"ISO-8859-6",	1, 1}
		, {"ISO-8859-7", 	1, 1}
		, {"ISO-8859-8",	1, 1}
		, {"ISO-8859-9", 	1, 1}
		, {"ISO-8859-10", 	1, 1}
		, {"ISO-8859-13", 	1, 1}
		, {"ISO-8859-14", 	1, 1}
		, {"ISO-8859-15", 	1, 1}
		, {"ISO-8859-16", 	1, 1}
		, {"KOI8-R", 		1, 1}
		, {"KOI8-U", 		1, 1}
		, {"KOI8-RU", 		1, 1}
		, {"CP1250", 		1, 1}
		, {"CP1251", 		1, 1}
		, {"CP1252", 		1, 1}
		, {"CP1253", 		1, 1}
		, {"CP1254", 		1, 1}
		, {"CP1255", 		1, 1}
		, {"CP1256", 		1, 1}
		, {"CP1257", 		1, 1}
		, {"CP1258", 		1, 1}
		, {"CP850", 		1, 1}
		, {"CP862", 		1, 1}
		, {"CP866", 		1, 1}
		, {"CP437", 		1, 1}
		, {"MAC", 		1, 1}
		, {"MACCENTRALEUROPE", 	1, 1}
		, {"MACICELAND", 	1, 1}
		, {"MACCROATIAN", 	1, 1}
		, {"MACROMANIA", 	1, 1}
		, {"MACCYRILLIC", 	1, 1}
		, {"MACUKRAINE", 	1, 1}
		, {"MACGREEK", 		1, 1}
		, {"MACTURKISH", 	1, 1}
		, {"MACHEBREW", 	1, 1}
		, {"MACARABIC", 	1, 1}
		, {"MACTHAI", 		1, 1}
		, {"ROMAN8",		1, 1}
		, {"NEXTSTEP", 		1, 1}
		, {"ARMSCII-8", 	1, 1}
		, {"GEORGIAN-ACADEMY", 	1, 1}
		, {"GEORGIAN-PS", 	1, 1}
		, {"KOI8-T", 		1, 1}
		, {"MULELAO-1", 	1, 1}
		, {"CP1133", 		1, 1}
		, {"ISO-IR-166", 	1, 1}
		, {"CP874", 		1, 1}
		, {"CP936", 		1, 2}
		, {"CN", 		1, 1}
		, {"CP932", 		1, 2}
		, {"CN-GB", 		1, 2}
		, {"CP950", 		1, 2}
		, {"CP949", 		1, 2}
		, {"CP1361", 		1, 2}
		, {"BIG-5", 		1, 2}
		, {"BIG5-HKSCS", 	1, 2}
		, {"SJIS", 		1, 2}
		, {"EUC-KR", 		1, 2}
		, {"VISCII", 		1, 1}
		, {"ISO-IR-14", 	1, 1}
		, {"EUC-JP", 		1, 3}
		, {"EUC-TW", 		1, 4}
		, {"ISO-2022-JP", 	1, 1}
		, {"ISO-2022-KR", 	1, 2}
		, {"ISO-2022-CN", 	1, 4}
		, {"ISO-2022-CN-EXT", 	1, 4}
		, {"ISO-2022-JP-2", 	1, 1}
		, {"GB18030", 		1, 4}
		/*
		 * Beyond this point, I don't know the right answers.  
		 * If you can provide the correct min/max (byte/char) values, please
		 * correct them if necessary and move them above the stopper row.
		 * Will the person vetting the last unknown row please turn off the lights?  
		 * --jkl April 2003
		 */
		, {"",	 		0, 0}	/* stopper row */
		
		, {"TCVN", 		1, 1}
		, {"JISX0201-1976", 	1, 1}
		, {"ISO-IR-87", 	1, 1}
		, {"ISO-IR-159", 	1, 1}
		, {"CHINESE", 		1, 1}
		, {"CN-GB-ISOIR165", 	1, 1}
		, {"ISO-IR-149", 	1, 1}
		, {"ISO-2022-JP-1", 	1, 1}
		, {"HZ", 		1, 1}
		/* stopper row */
		, {"",	 		0, 0}
		
