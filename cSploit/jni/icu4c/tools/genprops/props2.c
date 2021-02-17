/*
*******************************************************************************
*
*   Copyright (C) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  props2.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb24
*   created by: Markus W. Scherer
*
*   Parse more Unicode Character Database files and store
*   additional Unicode character properties in bit set vectors.
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "cstring.h"
#include "cmemory.h"
#include "utrie.h"
#include "uprops.h"
#include "propsvec.h"
#include "uparse.h"
#include "writesrc.h"
#include "genprops.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* data --------------------------------------------------------------------- */

static UNewTrie *newTrie;
UPropsVectors *pv;

/* miscellaneous ------------------------------------------------------------ */

static char *
trimTerminateField(char *s, char *limit) {
    /* trim leading whitespace */
    s=(char *)u_skipWhitespace(s);

    /* trim trailing whitespace */
    while(s<limit && (*(limit-1)==' ' || *(limit-1)=='\t')) {
        --limit;
    }
    *limit=0;

    return s;
}

static void
parseTwoFieldFile(char *filename, char *basename,
                  const char *ucdFile, const char *suffix,
                  UParseLineFn *lineFn,
                  UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, 2, lineFn, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", ucdFile, u_errorName(*pErrorCode));
    }
}

static void U_CALLCONV
ageLineFn(void *context,
          char *fields[][2], int32_t fieldCount,
          UErrorCode *pErrorCode);

static void
parseMultiFieldFile(char *filename, char *basename,
                    const char *ucdFile, const char *suffix,
                    int32_t fieldCount,
                    UParseLineFn *lineFn,
                    UErrorCode *pErrorCode) {
    char *fields[20][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, fieldCount, lineFn, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", ucdFile, u_errorName(*pErrorCode));
    }
}

static void U_CALLCONV
numericLineFn(void *context,
              char *fields[][2], int32_t fieldCount,
              UErrorCode *pErrorCode);

/* parse files with single enumerated properties ---------------------------- */

struct SingleEnum {
    const char *ucdFile, *propName;
    UProperty prop;
    int32_t vecWord, vecShift;
    uint32_t vecMask;
};
typedef struct SingleEnum SingleEnum;

static void
parseSingleEnumFile(char *filename, char *basename, const char *suffix,
                    const SingleEnum *sen,
                    UErrorCode *pErrorCode);

static const SingleEnum scriptSingleEnum={ 
    "Scripts", "script", 
    UCHAR_SCRIPT, 
    0, 0, UPROPS_SCRIPT_MASK
};

static const SingleEnum blockSingleEnum={
    "Blocks", "block",
    UCHAR_BLOCK,
    0, UPROPS_BLOCK_SHIFT, UPROPS_BLOCK_MASK
};

static const SingleEnum graphemeClusterBreakSingleEnum={
    "GraphemeBreakProperty", "Grapheme_Cluster_Break",
    UCHAR_GRAPHEME_CLUSTER_BREAK,
    2, UPROPS_GCB_SHIFT, UPROPS_GCB_MASK
};

static const SingleEnum wordBreakSingleEnum={
    "WordBreakProperty", "Word_Break",
    UCHAR_WORD_BREAK,
    2, UPROPS_WB_SHIFT, UPROPS_WB_MASK
};

static const SingleEnum sentenceBreakSingleEnum={
    "SentenceBreakProperty", "Sentence_Break",
    UCHAR_SENTENCE_BREAK,
    2, UPROPS_SB_SHIFT, UPROPS_SB_MASK
};

static const SingleEnum lineBreakSingleEnum={
    "LineBreak", "line break",
    UCHAR_LINE_BREAK,
    UPROPS_LB_VWORD, UPROPS_LB_SHIFT, UPROPS_LB_MASK
};

static const SingleEnum eawSingleEnum={
    "EastAsianWidth", "east asian width",
    UCHAR_EAST_ASIAN_WIDTH,
    0, UPROPS_EA_SHIFT, UPROPS_EA_MASK
};

static void U_CALLCONV
singleEnumLineFn(void *context,
                 char *fields[][2], int32_t fieldCount,
                 UErrorCode *pErrorCode) {
    const SingleEnum *sen;
    char *s;
    uint32_t start, end, uv;
    int32_t value;

    sen=(const SingleEnum *)context;

    u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops: syntax error in %s.txt field 0 at %s\n", sen->ucdFile, fields[0][0]);
        exit(*pErrorCode);
    }

    /* parse property alias */
    s=trimTerminateField(fields[1][0], fields[1][1]);
    value=u_getPropertyValueEnum(sen->prop, s);
    if(value<0) {
        if(sen->prop==UCHAR_BLOCK) {
            if(isToken("Greek", s)) {
                value=UBLOCK_GREEK; /* Unicode 3.2 renames this to "Greek and Coptic" */
            } else if(isToken("Combining Marks for Symbols", s)) {
                value=UBLOCK_COMBINING_MARKS_FOR_SYMBOLS; /* Unicode 3.2 renames this to "Combining Diacritical Marks for Symbols" */
            } else if(isToken("Private Use", s)) {
                value=UBLOCK_PRIVATE_USE; /* Unicode 3.2 renames this to "Private Use Area" */
            }
        }
    }
    if(value<0) {
        fprintf(stderr, "genprops error: unknown %s name in %s.txt field 1 at %s\n",
                        sen->propName, sen->ucdFile, s);
        exit(U_PARSE_ERROR);
    }

    uv=(uint32_t)(value<<sen->vecShift);
    if((uv&sen->vecMask)!=uv) {
        fprintf(stderr, "genprops error: %s value overflow (0x%x) at %s\n",
                        sen->propName, (int)uv, s);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }

    if(start==0 && end==0x10ffff) {
        /* Also set bits for initialValue and errorValue. */
        end=UPVEC_MAX_CP;
    }
    upvec_setValue(pv, start, end, sen->vecWord, uv, sen->vecMask, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops error: unable to set %s code: %s\n",
                        sen->propName, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

static void
parseSingleEnumFile(char *filename, char *basename, const char *suffix,
                    const SingleEnum *sen,
                    UErrorCode *pErrorCode) {
    char *fields[2][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, sen->ucdFile, suffix);

    u_parseDelimitedFile(filename, ';', fields, 2, singleEnumLineFn, (void *)sen, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", sen->ucdFile, u_errorName(*pErrorCode));
    }
}

/* parse files with multiple binary properties ------------------------------ */

struct Binary {
    const char *propName;
    int32_t vecWord, vecShift;
};
typedef struct Binary Binary;

struct Binaries {
    const char *ucdFile;
    const Binary *binaries;
    int32_t binariesCount;
};
typedef struct Binaries Binaries;

static const Binary
propListNames[]={
    { "White_Space",                        1, UPROPS_WHITE_SPACE },
    { "Dash",                               1, UPROPS_DASH },
    { "Hyphen",                             1, UPROPS_HYPHEN },
    { "Quotation_Mark",                     1, UPROPS_QUOTATION_MARK },
    { "Terminal_Punctuation",               1, UPROPS_TERMINAL_PUNCTUATION },
    { "Hex_Digit",                          1, UPROPS_HEX_DIGIT },
    { "ASCII_Hex_Digit",                    1, UPROPS_ASCII_HEX_DIGIT },
    { "Ideographic",                        1, UPROPS_IDEOGRAPHIC },
    { "Diacritic",                          1, UPROPS_DIACRITIC },
    { "Extender",                           1, UPROPS_EXTENDER },
    { "Noncharacter_Code_Point",            1, UPROPS_NONCHARACTER_CODE_POINT },
    { "Grapheme_Link",                      1, UPROPS_GRAPHEME_LINK },
    { "IDS_Binary_Operator",                1, UPROPS_IDS_BINARY_OPERATOR },
    { "IDS_Trinary_Operator",               1, UPROPS_IDS_TRINARY_OPERATOR },
    { "Radical",                            1, UPROPS_RADICAL },
    { "Unified_Ideograph",                  1, UPROPS_UNIFIED_IDEOGRAPH },
    { "Deprecated",                         1, UPROPS_DEPRECATED },
    { "Logical_Order_Exception",            1, UPROPS_LOGICAL_ORDER_EXCEPTION },

    /* new properties in Unicode 4.0.1 */
    { "STerm",                              1, UPROPS_S_TERM },
    { "Variation_Selector",                 1, UPROPS_VARIATION_SELECTOR },

    /* new properties in Unicode 4.1 */
    { "Pattern_Syntax",                     1, UPROPS_PATTERN_SYNTAX },
    { "Pattern_White_Space",                1, UPROPS_PATTERN_WHITE_SPACE }
};

static const Binaries
propListBinaries={
    "PropList", propListNames, LENGTHOF(propListNames)
};

static const Binary
derCorePropsNames[]={
    { "XID_Start",                          1, UPROPS_XID_START },
    { "XID_Continue",                       1, UPROPS_XID_CONTINUE },

    /* before Unicode 4/ICU 2.6/format version 3.2, these used to be Other_XYZ from PropList.txt */
    { "Math",                               1, UPROPS_MATH },
    { "Alphabetic",                         1, UPROPS_ALPHABETIC },
    { "Grapheme_Extend",                    1, UPROPS_GRAPHEME_EXTEND },
    { "Default_Ignorable_Code_Point",       1, UPROPS_DEFAULT_IGNORABLE_CODE_POINT },

    /* new properties bits in ICU 2.6/format version 3.2 */
    { "ID_Start",                           1, UPROPS_ID_START },
    { "ID_Continue",                        1, UPROPS_ID_CONTINUE },
    { "Grapheme_Base",                      1, UPROPS_GRAPHEME_BASE },

    /*
     * Unicode 5/ICU 3.6 moves Grapheme_Link from PropList.txt
     * to DerivedCoreProperties.txt and deprecates it.
     */
    { "Grapheme_Link",                      1, UPROPS_GRAPHEME_LINK }
};

static const Binaries
derCorePropsBinaries={
    "DerivedCoreProperties", derCorePropsNames, LENGTHOF(derCorePropsNames)
};

static char ignoredProps[100][64];
static int32_t ignoredPropsCount;

static void
addIgnoredProp(char *s, char *limit) {
    int32_t i;

    s=trimTerminateField(s, limit);
    for(i=0; i<ignoredPropsCount; ++i) {
        if(0==uprv_strcmp(ignoredProps[i], s)) {
            return;
        }
    }
    uprv_strcpy(ignoredProps[ignoredPropsCount++], s);
}

static void U_CALLCONV
binariesLineFn(void *context,
               char *fields[][2], int32_t fieldCount,
               UErrorCode *pErrorCode) {
    const Binaries *bin;
    char *s;
    uint32_t start, end, uv;
    int32_t i;

    bin=(const Binaries *)context;

    u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops: syntax error in %s.txt field 0 at %s\n", bin->ucdFile, fields[0][0]);
        exit(*pErrorCode);
    }

    /* parse binary property name */
    s=(char *)u_skipWhitespace(fields[1][0]);
    for(i=0;; ++i) {
        if(i==bin->binariesCount) {
            /* ignore unrecognized properties */
            if(beVerbose) {
                addIgnoredProp(s, fields[1][1]);
            }
            return;
        }
        if(isToken(bin->binaries[i].propName, s)) {
            break;
        }
    }

    if(bin->binaries[i].vecShift>=32) {
        fprintf(stderr, "genprops error: shift value %d>=32 for %s %s\n",
                        (int)bin->binaries[i].vecShift, bin->ucdFile, bin->binaries[i].propName);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }
    uv=U_MASK(bin->binaries[i].vecShift);

    if(start==0 && end==0x10ffff) {
        /* Also set bits for initialValue and errorValue. */
        end=UPVEC_MAX_CP;
    }
    upvec_setValue(pv, start, end, bin->binaries[i].vecWord, uv, uv, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops error: unable to set %s code: %s\n",
                        bin->binaries[i].propName, u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

static void
parseBinariesFile(char *filename, char *basename, const char *suffix,
                  const Binaries *bin,
                  UErrorCode *pErrorCode) {
    char *fields[2][2];
    int32_t i;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    writeUCDFilename(basename, bin->ucdFile, suffix);

    ignoredPropsCount=0;

    u_parseDelimitedFile(filename, ';', fields, 2, binariesLineFn, (void *)bin, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "error parsing %s.txt: %s\n", bin->ucdFile, u_errorName(*pErrorCode));
    }

    if(beVerbose) {
        for(i=0; i<ignoredPropsCount; ++i) {
            printf("genprops: ignoring property %s in %s.txt\n", ignoredProps[i], bin->ucdFile);
        }
    }
}

/* -------------------------------------------------------------------------- */

U_CFUNC void
initAdditionalProperties() {
    UErrorCode errorCode=U_ZERO_ERROR;
    pv=upvec_open(UPROPS_VECTOR_WORDS, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "error: upvec_open() failed - %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
}

U_CFUNC void
exitAdditionalProperties() {
    utrie_close(newTrie);
    upvec_close(pv);
}

U_CFUNC void
generateAdditionalProperties(char *filename, const char *suffix, UErrorCode *pErrorCode) {
    char *basename;

    basename=filename+uprv_strlen(filename);

    /* process various UCD .txt files */

    /* add Han numeric types & values */
    parseMultiFieldFile(filename, basename, "DerivedNumericValues", suffix, 2, numericLineFn, pErrorCode);

    parseTwoFieldFile(filename, basename, "DerivedAge", suffix, ageLineFn, pErrorCode);

    /*
     * UTR 24 says:
     * Section 2:
     *   "Common - For characters that may be used
     *             within multiple scripts,
     *             or any unassigned code points."
     *
     * Section 4:
     *   "The value COMMON is the default value,
     *    given to all code points that are not
     *    explicitly mentioned in the data file."
     *
     * COMMON==USCRIPT_COMMON==0 - nothing to do
     */
    parseSingleEnumFile(filename, basename, suffix, &scriptSingleEnum, pErrorCode);

    parseSingleEnumFile(filename, basename, suffix, &blockSingleEnum, pErrorCode);

    parseBinariesFile(filename, basename, suffix, &propListBinaries, pErrorCode);

    parseBinariesFile(filename, basename, suffix, &derCorePropsBinaries, pErrorCode);

    parseSingleEnumFile(filename, basename, suffix, &graphemeClusterBreakSingleEnum, pErrorCode);

    parseSingleEnumFile(filename, basename, suffix, &wordBreakSingleEnum, pErrorCode);

    parseSingleEnumFile(filename, basename, suffix, &sentenceBreakSingleEnum, pErrorCode);

    /*
     * LineBreak-4.0.0.txt:
     *  - All code points, assigned and unassigned, that are not listed 
     *         explicitly are given the value "XX".
     *
     * XX==U_LB_UNKNOWN==0 - nothing to do
     */
    parseSingleEnumFile(filename, basename, suffix, &lineBreakSingleEnum, pErrorCode);

    /*
     * Preset East Asian Width defaults:
     *
     * http://www.unicode.org/reports/tr11/#Unassigned
     * 7.1 Unassigned and Private Use characters
     *
     * All unassigned characters are by default classified as non-East Asian neutral,
     * except for the range U+20000 to U+2FFFD,
     * since all code positions from U+20000 to U+2FFFD are intended for CJK ideographs (W).
     * All Private use characters are by default classified as ambiguous,
     * since their definition depends on context.
     *
     * N for all ==0 - nothing to do
     * A for Private Use
     * W for plane 2
     */
    *pErrorCode=U_ZERO_ERROR;
    upvec_setValue(pv, 0xe000, 0xf8ff, 0, (uint32_t)(U_EA_AMBIGUOUS<<UPROPS_EA_SHIFT), UPROPS_EA_MASK, pErrorCode);
    upvec_setValue(pv, 0xf0000, 0xffffd, 0, (uint32_t)(U_EA_AMBIGUOUS<<UPROPS_EA_SHIFT), UPROPS_EA_MASK, pErrorCode);
    upvec_setValue(pv, 0x100000, 0x10fffd, 0, (uint32_t)(U_EA_AMBIGUOUS<<UPROPS_EA_SHIFT), UPROPS_EA_MASK, pErrorCode);
    upvec_setValue(pv, 0x20000, 0x2fffd, 0, (uint32_t)(U_EA_WIDE<<UPROPS_EA_SHIFT), UPROPS_EA_MASK, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops: unable to set default East Asian Widths: %s\n", u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }

    /* parse EastAsianWidth.txt */
    parseSingleEnumFile(filename, basename, suffix, &eawSingleEnum, pErrorCode);

    {
        UPVecToUTrieContext toUTrie={ NULL, 50000 /* capacity */, 0, TRUE /* latin1Linear */ };
        upvec_compact(pv, upvec_compactToUTrieHandler, &toUTrie, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            fprintf(stderr, "genprops error: unable to build trie for additional properties: %s\n",
                    u_errorName(*pErrorCode));
            exit(*pErrorCode);
        }
        newTrie=toUTrie.newTrie;
    }
}

/* DerivedAge.txt ----------------------------------------------------------- */

static void U_CALLCONV
ageLineFn(void *context,
          char *fields[][2], int32_t fieldCount,
          UErrorCode *pErrorCode) {
    char *s, *numberLimit;
    uint32_t value, start, end, version;

    u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops: syntax error in DerivedAge.txt field 0 at %s\n", fields[0][0]);
        exit(*pErrorCode);
    }

    /* ignore "unassigned" (the default is already set to 0.0) */
    s=(char *)u_skipWhitespace(fields[1][0]);
    if(0==uprv_strncmp(s, "unassigned", 10)) {
        return;
    }

    /* parse version number */
    value=(uint32_t)uprv_strtoul(s, &numberLimit, 10);
    if(s==numberLimit || value==0 || value>15 || (*numberLimit!='.' && *numberLimit!=' ' && *numberLimit!='\t' && *numberLimit!=0)) {
        fprintf(stderr, "genprops: syntax error in DerivedAge.txt field 1 at %s\n", fields[1][0]);
        *pErrorCode=U_PARSE_ERROR;
        exit(U_PARSE_ERROR);
    }
    version=value<<4;

    /* parse minor version number */
    if(*numberLimit=='.') {
        s=(char *)u_skipWhitespace(numberLimit+1);
        value=(uint32_t)uprv_strtoul(s, &numberLimit, 10);
        if(s==numberLimit || value>15 || (*numberLimit!=' ' && *numberLimit!='\t' && *numberLimit!=0)) {
            fprintf(stderr, "genprops: syntax error in DerivedAge.txt field 1 at %s\n", fields[1][0]);
            *pErrorCode=U_PARSE_ERROR;
            exit(U_PARSE_ERROR);
        }
        version|=value;
    }

    if(start==0 && end==0x10ffff) {
        /* Also set bits for initialValue and errorValue. */
        end=UPVEC_MAX_CP;
    }
    upvec_setValue(pv, start, end, 0, version<<UPROPS_AGE_SHIFT, UPROPS_AGE_MASK, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops error: unable to set character age: %s\n", u_errorName(*pErrorCode));
        exit(*pErrorCode);
    }
}

/* DerivedNumericValues.txt ------------------------------------------------- */

static void U_CALLCONV
numericLineFn(void *context,
              char *fields[][2], int32_t fieldCount,
              UErrorCode *pErrorCode) {
    Props newProps={ 0 };
    char *s, *numberLimit;
    uint32_t start, end, value, oldProps32;
    char c;
    UBool isFraction;

    /* get the code point range */
    u_parseCodePointRange(fields[0][0], &start, &end, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        fprintf(stderr, "genprops: syntax error in DerivedNumericValues.txt field 0 at %s\n", fields[0][0]);
        exit(*pErrorCode);
    }

    /*
     * Ignore the
     * # @missing: 0000..10FFFF; NaN
     * line from Unicode 5.1's DerivedNumericValues.txt:
     * The following code cannot parse "NaN", and we don't want to overwrite
     * the numeric values for all characters after reading most
     * from UnicodeData.txt already.
     */
    if(start==0 && end==0x10ffff) {
        return;
    }

    /* check if the numeric value is a fraction (this code does not handle any) */
    isFraction=FALSE;
    s=uprv_strchr(fields[1][0], '.');
    if(s!=NULL) {
        numberLimit=s+1;
        while('0'<=(c=*numberLimit++) && c<='9') {
            if(c!='0') {
                isFraction=TRUE;
                break;
            }
        }
    }

    if(isFraction) {
        value=0;
    } else {
        /* parse numeric value */
        s=(char *)u_skipWhitespace(fields[1][0]);

        /* try large, single-significant-digit numbers, may otherwise overflow strtoul() */
        if('1'<=s[0] && s[0]<='9' && s[1]=='0' && s[2]=='0') {
            /* large integers are encoded in a special way, see store.c */
            uint8_t exp=0;

            value=s[0]-'0';
            numberLimit=s;
            while(*(++numberLimit)=='0') {
                ++exp;
            }
            newProps.exponent=exp;
        } else {
            /* normal number parsing */
            value=(uint32_t)uprv_strtoul(s, &numberLimit, 10);
        }
        if(numberLimit<=s || (*numberLimit!='.' && u_skipWhitespace(numberLimit)!=fields[1][1]) || value>=0x80000000) {
            fprintf(stderr, "genprops: syntax error in DerivedNumericValues.txt field 1 at %s\n", fields[0][0]);
            exit(U_PARSE_ERROR);
        }
    }

    /*
     * Unicode 4.0.1 removes the third column that used to list the numeric type.
     * Assume that either the data is the same as in UnicodeData.txt,
     * or else that the numeric type is "numeric".
     * This should work because we only expect to add numeric values for
     * Han characters; for those, UnicodeData.txt lists only ranges without
     * specific properties for single characters.
     */

    /* set the new numeric value */
    newProps.code=start;
    newProps.numericValue=(int32_t)value;       /* newly parsed numeric value */
    /* the exponent may have been set above */

    for(; start<=end; ++start) {
        uint32_t newProps32;
        int32_t oldNtv;
        oldProps32=getProps(start);
        oldNtv=(int32_t)GET_NUMERIC_TYPE_VALUE(oldProps32);

        if(isFraction) {
            if(UPROPS_NTV_FRACTION_START<=oldNtv && oldNtv<UPROPS_NTV_LARGE_START) {
                /* this code point was already listed with its numeric value in UnicodeData.txt */
                continue;
            } else {
                fprintf(stderr, "genprops: not prepared for new fractions in DerivedNumericValues.txt field 1 at %s\n", fields[1][0]);
                exit(U_PARSE_ERROR);
            }
        }

        /*
         * For simplicity, and because we only expect to set numeric values for Han characters,
         * for now we only allow to set these values for Lo characters.
         */
        if(oldNtv==UPROPS_NTV_NONE && GET_CATEGORY(oldProps32)!=U_OTHER_LETTER) {
            fprintf(stderr, "genprops error: new numeric value for a character other than Lo in DerivedNumericValues.txt at %s\n", fields[0][0]);
            exit(U_PARSE_ERROR);
        }

        /* verify that we do not change an existing value (fractions were excluded above) */
        if(oldNtv!=UPROPS_NTV_NONE) {
            /* the code point already has a value stored */
            newProps.numericType=UPROPS_NTV_GET_TYPE(oldNtv);
            newProps32=makeProps(&newProps);
            if(oldNtv!=GET_NUMERIC_TYPE_VALUE(newProps32)) {
                fprintf(stderr, "genprops error: new numeric value differs from old one for U+%04lx\n", (long)start);
                exit(U_PARSE_ERROR);
            }
            /* same value, continue */
        } else {
            /* the code point is getting a new numeric value */
            newProps.numericType=(uint8_t)U_NT_NUMERIC; /* assumed numeric type, see Unicode 4.0.1 comment */
            newProps32=makeProps(&newProps);
            if(beVerbose) {
                printf("adding U+%04x numeric type %d encoded-numeric-type-value 0x%03x from %s\n",
                       (int)start, U_NT_NUMERIC, (int)GET_NUMERIC_TYPE_VALUE(newProps32), fields[0][0]);
            }

            addProps(start, newProps32|GET_CATEGORY(oldProps32));
        }
    }
}

/* data serialization ------------------------------------------------------- */

U_CFUNC int32_t
writeAdditionalData(FILE *f, uint8_t *p, int32_t capacity, int32_t indexes[UPROPS_INDEX_COUNT]) {
    const uint32_t *pvArray;
    int32_t pvRows, pvCount;
    int32_t length;
    UErrorCode errorCode;

    pvArray=upvec_getArray(pv, &pvRows, NULL);
    pvCount=pvRows*UPROPS_VECTOR_WORDS;

    errorCode=U_ZERO_ERROR;
    length=utrie_serialize(newTrie, p, capacity, NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "genprops error: unable to serialize trie for additional properties: %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    if(p!=NULL) {
        if(beVerbose) {
            printf("size in bytes of additional props trie:%5u\n", (int)length);
        }
        if(f!=NULL) {
            UTrie trie={ NULL };
            UTrie2 *trie2;

            utrie_unserialize(&trie, p, length, &errorCode);
            if(U_FAILURE(errorCode)) {
                fprintf(
                    stderr,
                    "genprops error: failed to utrie_unserialize(trie for additional properties) - %s\n",
                    u_errorName(errorCode));
                exit(errorCode);
            }

            /* use UTrie2 */
            trie2=utrie2_fromUTrie(&trie, trie.initialValue, &errorCode);
            if(U_FAILURE(errorCode)) {
                fprintf(
                    stderr,
                    "genprops error: utrie2_fromUTrie() failed - %s\n",
                    u_errorName(errorCode));
                exit(errorCode);
            }
            {
                /* delete lead surrogate code unit values */
                UChar lead;
                trie2=utrie2_cloneAsThawed(trie2, &errorCode);
                for(lead=0xd800; lead<0xdc00; ++lead) {
                    utrie2_set32ForLeadSurrogateCodeUnit(trie2, lead, trie2->initialValue, &errorCode);
                }
                utrie2_freeze(trie2, UTRIE2_16_VALUE_BITS, &errorCode);
                if(U_FAILURE(errorCode)) {
                    fprintf(
                        stderr,
                        "genbidi error: deleting lead surrogate code unit values failed - %s\n",
                        u_errorName(errorCode));
                    exit(errorCode);
                }
            }

            usrc_writeUTrie2Arrays(f,
                "static const uint16_t propsVectorsTrie_index[%ld]={\n", NULL,
                trie2,
                "\n};\n\n");
            usrc_writeUTrie2Struct(f,
                "static const UTrie2 propsVectorsTrie={\n",
                trie2, "propsVectorsTrie_index", NULL,
                "};\n\n");

            utrie2_close(trie2);
        }

        p+=length;
        capacity-=length;

        /* set indexes */
        indexes[UPROPS_ADDITIONAL_VECTORS_INDEX]=
            indexes[UPROPS_ADDITIONAL_TRIE_INDEX]+length/4;
        indexes[UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX]=UPROPS_VECTOR_WORDS;
        indexes[UPROPS_RESERVED_INDEX]=
            indexes[UPROPS_ADDITIONAL_VECTORS_INDEX]+pvCount;

        indexes[UPROPS_MAX_VALUES_INDEX]=
            (((int32_t)U_EA_COUNT-1)<<UPROPS_EA_SHIFT)|
            (((int32_t)UBLOCK_COUNT-1)<<UPROPS_BLOCK_SHIFT)|
            (((int32_t)USCRIPT_CODE_LIMIT-1)&UPROPS_SCRIPT_MASK);
        indexes[UPROPS_MAX_VALUES_2_INDEX]=
            (((int32_t)U_LB_COUNT-1)<<UPROPS_LB_SHIFT)|
            (((int32_t)U_SB_COUNT-1)<<UPROPS_SB_SHIFT)|
            (((int32_t)U_WB_COUNT-1)<<UPROPS_WB_SHIFT)|
            (((int32_t)U_GCB_COUNT-1)<<UPROPS_GCB_SHIFT)|
            ((int32_t)U_DT_COUNT-1);
    }

    if(p!=NULL && (pvCount*4)<=capacity) {
        if(f!=NULL) {
            usrc_writeArray(f,
                "static const uint32_t propsVectors[%ld]={\n",
                pvArray, 32, pvCount,
                "};\n\n");
            fprintf(f, "static const int32_t countPropsVectors=%ld;\n", (long)pvCount);
            fprintf(f, "static const int32_t propsVectorsColumns=%ld;\n", (long)indexes[UPROPS_ADDITIONAL_VECTORS_COLUMNS_INDEX]);
        } else {
            uprv_memcpy(p, pvArray, pvCount*4);
        }
        if(beVerbose) {
            printf("number of additional props vectors:    %5u\n", (int)pvRows);
            printf("number of 32-bit words per vector:     %5u\n", UPROPS_VECTOR_WORDS);
        }
    }
    length+=pvCount*4;

    return length;
}
