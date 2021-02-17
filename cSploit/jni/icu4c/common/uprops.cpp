/*
*******************************************************************************
*
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uprops.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002feb24
*   created by: Markus W. Scherer
*
*   Implementations for mostly non-core Unicode character properties
*   stored in uprops.icu.
*
*   With the APIs implemented here, almost all properties files and
*   their associated implementation files are used from this file,
*   including those for normalization and case mappings.
*/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "normalizer2impl.h"
#include "ucln_cmn.h"
#include "umutex.h"
#include "unormimp.h"
#include "ubidi_props.h"
#include "uprops.h"
#include "ucase.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

U_NAMESPACE_USE

/* cleanup ------------------------------------------------------------------ */

static const UBiDiProps *gBdp=NULL;

static UBool U_CALLCONV uprops_cleanup(void) {
    gBdp=NULL;
    return TRUE;
}

/* bidi/shaping properties API ---------------------------------------------- */

/* get the UBiDiProps singleton, or else its dummy, once and for all */
static const UBiDiProps *
getBiDiProps() {
    /*
     * This lazy intialization with double-checked locking (without mutex protection for
     * the initial check) is transiently unsafe under certain circumstances.
     * Check the readme and use u_init() if necessary.
     */

    /* the initial check is performed by the GET_BIDI_PROPS() macro */
    const UBiDiProps *bdp;
    UErrorCode errorCode=U_ZERO_ERROR;

    bdp=ubidi_getSingleton(&errorCode);
#if !UBIDI_HARDCODE_DATA
    if(U_FAILURE(errorCode)) {
        errorCode=U_ZERO_ERROR;
        bdp=ubidi_getDummy(&errorCode);
        if(U_FAILURE(errorCode)) {
            return NULL;
        }
    }
#endif

    umtx_lock(NULL);
    if(gBdp==NULL) {
        gBdp=bdp;
        ucln_common_registerCleanup(UCLN_COMMON_UPROPS, uprops_cleanup);
    }
    umtx_unlock(NULL);

    return gBdp;
}

/* see comment for GET_CASE_PROPS() */
#define GET_BIDI_PROPS() (gBdp!=NULL ? gBdp : getBiDiProps())

/* general properties API functions ----------------------------------------- */

static const struct {
    int32_t column;
    uint32_t mask;
} binProps[UCHAR_BINARY_LIMIT]={
    /*
     * column and mask values for binary properties from u_getUnicodeProperties().
     * Must be in order of corresponding UProperty,
     * and there must be exactly one entry per binary UProperty.
     *
     * Properties with mask 0 are handled in code.
     * For them, column is the UPropertySource value.
     */
    {  1,               U_MASK(UPROPS_ALPHABETIC) },
    {  1,               U_MASK(UPROPS_ASCII_HEX_DIGIT) },
    { UPROPS_SRC_BIDI,  0 },                                    /* UCHAR_BIDI_CONTROL */
    { UPROPS_SRC_BIDI,  0 },                                    /* UCHAR_BIDI_MIRRORED */
    {  1,               U_MASK(UPROPS_DASH) },
    {  1,               U_MASK(UPROPS_DEFAULT_IGNORABLE_CODE_POINT) },
    {  1,               U_MASK(UPROPS_DEPRECATED) },
    {  1,               U_MASK(UPROPS_DIACRITIC) },
    {  1,               U_MASK(UPROPS_EXTENDER) },
    { UPROPS_SRC_NFC,  0 },                                     /* UCHAR_FULL_COMPOSITION_EXCLUSION */
    {  1,               U_MASK(UPROPS_GRAPHEME_BASE) },
    {  1,               U_MASK(UPROPS_GRAPHEME_EXTEND) },
    {  1,               U_MASK(UPROPS_GRAPHEME_LINK) },
    {  1,               U_MASK(UPROPS_HEX_DIGIT) },
    {  1,               U_MASK(UPROPS_HYPHEN) },
    {  1,               U_MASK(UPROPS_ID_CONTINUE) },
    {  1,               U_MASK(UPROPS_ID_START) },
    {  1,               U_MASK(UPROPS_IDEOGRAPHIC) },
    {  1,               U_MASK(UPROPS_IDS_BINARY_OPERATOR) },
    {  1,               U_MASK(UPROPS_IDS_TRINARY_OPERATOR) },
    { UPROPS_SRC_BIDI,  0 },                                    /* UCHAR_JOIN_CONTROL */
    {  1,               U_MASK(UPROPS_LOGICAL_ORDER_EXCEPTION) },
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_LOWERCASE */
    {  1,               U_MASK(UPROPS_MATH) },
    {  1,               U_MASK(UPROPS_NONCHARACTER_CODE_POINT) },
    {  1,               U_MASK(UPROPS_QUOTATION_MARK) },
    {  1,               U_MASK(UPROPS_RADICAL) },
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_SOFT_DOTTED */
    {  1,               U_MASK(UPROPS_TERMINAL_PUNCTUATION) },
    {  1,               U_MASK(UPROPS_UNIFIED_IDEOGRAPH) },
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_UPPERCASE */
    {  1,               U_MASK(UPROPS_WHITE_SPACE) },
    {  1,               U_MASK(UPROPS_XID_CONTINUE) },
    {  1,               U_MASK(UPROPS_XID_START) },
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CASE_SENSITIVE */
    {  1,               U_MASK(UPROPS_S_TERM) },
    {  1,               U_MASK(UPROPS_VARIATION_SELECTOR) },
    { UPROPS_SRC_NFC,   0 },                                    /* UCHAR_NFD_INERT */
    { UPROPS_SRC_NFKC,  0 },                                    /* UCHAR_NFKD_INERT */
    { UPROPS_SRC_NFC,   0 },                                    /* UCHAR_NFC_INERT */
    { UPROPS_SRC_NFKC,  0 },                                    /* UCHAR_NFKC_INERT */
    { UPROPS_SRC_NORM,  0 },                                    /* UCHAR_SEGMENT_STARTER */
    {  1,               U_MASK(UPROPS_PATTERN_SYNTAX) },
    {  1,               U_MASK(UPROPS_PATTERN_WHITE_SPACE) },
    { UPROPS_SRC_CHAR_AND_PROPSVEC,  0 },                       /* UCHAR_POSIX_ALNUM */
    { UPROPS_SRC_CHAR,  0 },                                    /* UCHAR_POSIX_BLANK */
    { UPROPS_SRC_CHAR,  0 },                                    /* UCHAR_POSIX_GRAPH */
    { UPROPS_SRC_CHAR,  0 },                                    /* UCHAR_POSIX_PRINT */
    { UPROPS_SRC_CHAR,  0 },                                    /* UCHAR_POSIX_XDIGIT */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CASED */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CASE_IGNORABLE */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CHANGES_WHEN_LOWERCASED */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CHANGES_WHEN_UPPERCASED */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CHANGES_WHEN_TITLECASED */
    { UPROPS_SRC_CASE_AND_NORM,  0 },                           /* UCHAR_CHANGES_WHEN_CASEFOLDED */
    { UPROPS_SRC_CASE,  0 },                                    /* UCHAR_CHANGES_WHEN_CASEMAPPED */
    { UPROPS_SRC_NFKC_CF, 0 }                                   /* UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED */
};

U_CAPI UBool U_EXPORT2
u_hasBinaryProperty(UChar32 c, UProperty which) {
    /* c is range-checked in the functions that are called from here */
    if(which<UCHAR_BINARY_START || UCHAR_BINARY_LIMIT<=which) {
        /* not a known binary property */
    } else {
        uint32_t mask=binProps[which].mask;
        int32_t column=binProps[which].column;
        if(mask!=0) {
            /* systematic, directly stored properties */
            return (u_getUnicodeProperties(c, column)&mask)!=0;
        } else {
            if(column==UPROPS_SRC_CASE) {
                return ucase_hasBinaryProperty(c, which);
            } else if(column==UPROPS_SRC_NORM) {
#if !UCONFIG_NO_NORMALIZATION
                /* normalization properties from unorm.icu */
                switch(which) {
                case UCHAR_SEGMENT_STARTER:
                    return unorm_isCanonSafeStart(c);
                default:
                    break;
                }
#endif
            } else if(column==UPROPS_SRC_NFC) {
#if !UCONFIG_NO_NORMALIZATION
                UErrorCode errorCode=U_ZERO_ERROR;
                switch(which) {
                case UCHAR_FULL_COMPOSITION_EXCLUSION: {
                    // By definition, Full_Composition_Exclusion is the same as NFC_QC=No.
                    const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(errorCode);
                    return U_SUCCESS(errorCode) && impl->isCompNo(impl->getNorm16(c));
                    break;
                }
                default: {
                    // UCHAR_NF[CD]_INERT properties
                    const Normalizer2 *norm2=Normalizer2Factory::getInstance(
                        (UNormalizationMode)(which-UCHAR_NFD_INERT+UNORM_NFD), errorCode);
                    return U_SUCCESS(errorCode) && norm2->isInert(c);
                }
                }
#endif
            } else if(column==UPROPS_SRC_NFKC) {
#if !UCONFIG_NO_NORMALIZATION
                // UCHAR_NFK[CD]_INERT properties
                UErrorCode errorCode=U_ZERO_ERROR;
                const Normalizer2 *norm2=Normalizer2Factory::getInstance(
                    (UNormalizationMode)(which-UCHAR_NFD_INERT+UNORM_NFD), errorCode);
                return U_SUCCESS(errorCode) && norm2->isInert(c);
#endif
            } else if(column==UPROPS_SRC_NFKC_CF) {
                // currently only for UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED
#if !UCONFIG_NO_NORMALIZATION
                UErrorCode errorCode=U_ZERO_ERROR;
                const Normalizer2Impl *kcf=Normalizer2Factory::getNFKC_CFImpl(errorCode);
                if(U_SUCCESS(errorCode)) {
                    UnicodeString src(c);
                    UnicodeString dest;
                    {
                        // The ReorderingBuffer must be in a block because its destructor
                        // needs to release dest's buffer before we look at its contents.
                        ReorderingBuffer buffer(*kcf, dest);
                        // Small destCapacity for NFKC_CF(c).
                        if(buffer.init(5, errorCode)) {
                            const UChar *srcArray=src.getBuffer();
                            kcf->compose(srcArray, srcArray+src.length(), FALSE,
                                         TRUE, buffer, errorCode);
                        }
                    }
                    return U_SUCCESS(errorCode) && dest!=src;
                }
#endif
            } else if(column==UPROPS_SRC_BIDI) {
                /* bidi/shaping properties */
                const UBiDiProps *bdp=GET_BIDI_PROPS();
                if(bdp!=NULL) {
                    switch(which) {
                    case UCHAR_BIDI_MIRRORED:
                        return ubidi_isMirrored(bdp, c);
                    case UCHAR_BIDI_CONTROL:
                        return ubidi_isBidiControl(bdp, c);
                    case UCHAR_JOIN_CONTROL:
                        return ubidi_isJoinControl(bdp, c);
                    default:
                        break;
                    }
                }
                /* else return FALSE below */
            } else if(column==UPROPS_SRC_CHAR) {
                switch(which) {
                case UCHAR_POSIX_BLANK:
                    return u_isblank(c);
                case UCHAR_POSIX_GRAPH:
                    return u_isgraphPOSIX(c);
                case UCHAR_POSIX_PRINT:
                    return u_isprintPOSIX(c);
                case UCHAR_POSIX_XDIGIT:
                    return u_isxdigit(c);
                default:
                    break;
                }
            } else if(column==UPROPS_SRC_CHAR_AND_PROPSVEC) {
                switch(which) {
                case UCHAR_POSIX_ALNUM:
                    return u_isalnumPOSIX(c);
                default:
                    break;
                }
            } else if(column==UPROPS_SRC_CASE_AND_NORM) {
#if !UCONFIG_NO_NORMALIZATION
                UChar nfdBuffer[4];
                const UChar *nfd;
                int32_t nfdLength;
                UErrorCode errorCode=U_ZERO_ERROR;
                const Normalizer2Impl *nfcImpl=Normalizer2Factory::getNFCImpl(errorCode);
                if(U_FAILURE(errorCode)) {
                    return FALSE;
                }
                switch(which) {
                case UCHAR_CHANGES_WHEN_CASEFOLDED:
                    nfd=nfcImpl->getDecomposition(c, nfdBuffer, nfdLength);
                    if(nfd!=NULL) {
                        /* c has a decomposition */
                        if(nfdLength==1) {
                            c=nfd[0];  /* single BMP code point */
                        } else if(nfdLength<=U16_MAX_LENGTH) {
                            int32_t i=0;
                            U16_NEXT(nfd, i, nfdLength, c);
                            if(i==nfdLength) {
                                /* single supplementary code point */
                            } else {
                                c=U_SENTINEL;
                            }
                        } else {
                            c=U_SENTINEL;
                        }
                    } else if(c<0) {
                        return FALSE;  /* protect against bad input */
                    }
                    errorCode=U_ZERO_ERROR;
                    if(c>=0) {
                        /* single code point */
                        const UCaseProps *csp=ucase_getSingleton(&errorCode);
                        const UChar *resultString;
                        return (UBool)(ucase_toFullFolding(csp, c, &resultString, U_FOLD_CASE_DEFAULT)>=0);
                    } else {
                        /* guess some large but stack-friendly capacity */
                        UChar dest[2*UCASE_MAX_STRING_LENGTH];
                        int32_t destLength;
                        destLength=u_strFoldCase(dest, LENGTHOF(dest), nfd, nfdLength, U_FOLD_CASE_DEFAULT, &errorCode);
                        return (UBool)(U_SUCCESS(errorCode) && 0!=u_strCompare(nfd, nfdLength, dest, destLength, FALSE));
                    }
                default:
                    break;
                }
#endif
            }
        }
    }
    return FALSE;
}

#if !UCONFIG_NO_NORMALIZATION

U_CAPI uint8_t U_EXPORT2
u_getCombiningClass(UChar32 c) {
    UErrorCode errorCode=U_ZERO_ERROR;
    const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(errorCode);
    if(U_SUCCESS(errorCode)) {
        return impl->getCC(impl->getNorm16(c));
    } else {
        return 0;
    }
}

static uint16_t
getFCD16(UChar32 c) {
    UErrorCode errorCode=U_ZERO_ERROR;
    const UTrie2 *trie=Normalizer2Factory::getFCDTrie(errorCode);
    if(U_SUCCESS(errorCode)) {
        return UTRIE2_GET16(trie, c);
    } else {
        return 0;
    }
}

#endif

/*
 * Map some of the Grapheme Cluster Break values to Hangul Syllable Types.
 * Hangul_Syllable_Type is fully redundant with a subset of Grapheme_Cluster_Break.
 */
static const UHangulSyllableType gcbToHst[]={
    U_HST_NOT_APPLICABLE,   /* U_GCB_OTHER */
    U_HST_NOT_APPLICABLE,   /* U_GCB_CONTROL */
    U_HST_NOT_APPLICABLE,   /* U_GCB_CR */
    U_HST_NOT_APPLICABLE,   /* U_GCB_EXTEND */
    U_HST_LEADING_JAMO,     /* U_GCB_L */
    U_HST_NOT_APPLICABLE,   /* U_GCB_LF */
    U_HST_LV_SYLLABLE,      /* U_GCB_LV */
    U_HST_LVT_SYLLABLE,     /* U_GCB_LVT */
    U_HST_TRAILING_JAMO,    /* U_GCB_T */
    U_HST_VOWEL_JAMO        /* U_GCB_V */
    /*
     * Omit GCB values beyond what we need for hst.
     * The code below checks for the array length.
     */
};

U_CAPI int32_t U_EXPORT2
u_getIntPropertyValue(UChar32 c, UProperty which) {
    UErrorCode errorCode;

    if(which<UCHAR_BINARY_START) {
        return 0; /* undefined */
    } else if(which<UCHAR_BINARY_LIMIT) {
        return (int32_t)u_hasBinaryProperty(c, which);
    } else if(which<UCHAR_INT_START) {
        return 0; /* undefined */
    } else if(which<UCHAR_INT_LIMIT) {
        switch(which) {
        case UCHAR_BIDI_CLASS:
            return (int32_t)u_charDirection(c);
        case UCHAR_BLOCK:
            return (int32_t)ublock_getCode(c);
#if !UCONFIG_NO_NORMALIZATION
        case UCHAR_CANONICAL_COMBINING_CLASS:
            return u_getCombiningClass(c);
#endif
        case UCHAR_DECOMPOSITION_TYPE:
            return (int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_DT_MASK);
        case UCHAR_EAST_ASIAN_WIDTH:
            return (int32_t)(u_getUnicodeProperties(c, 0)&UPROPS_EA_MASK)>>UPROPS_EA_SHIFT;
        case UCHAR_GENERAL_CATEGORY:
            return (int32_t)u_charType(c);
        case UCHAR_JOINING_GROUP:
            return ubidi_getJoiningGroup(GET_BIDI_PROPS(), c);
        case UCHAR_JOINING_TYPE:
            return ubidi_getJoiningType(GET_BIDI_PROPS(), c);
        case UCHAR_LINE_BREAK:
            return (int32_t)(u_getUnicodeProperties(c, UPROPS_LB_VWORD)&UPROPS_LB_MASK)>>UPROPS_LB_SHIFT;
        case UCHAR_NUMERIC_TYPE: {
            int32_t ntv=(int32_t)GET_NUMERIC_TYPE_VALUE(u_getUnicodeProperties(c, -1));
            return UPROPS_NTV_GET_TYPE(ntv);
        }
        case UCHAR_SCRIPT:
            errorCode=U_ZERO_ERROR;
            return (int32_t)uscript_getScript(c, &errorCode);
        case UCHAR_HANGUL_SYLLABLE_TYPE: {
            /* see comments on gcbToHst[] above */
            int32_t gcb=(int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_GCB_MASK)>>UPROPS_GCB_SHIFT;
            if(gcb<LENGTHOF(gcbToHst)) {
                return gcbToHst[gcb];
            } else {
                return U_HST_NOT_APPLICABLE;
            }
        }
#if !UCONFIG_NO_NORMALIZATION
        case UCHAR_NFD_QUICK_CHECK:
        case UCHAR_NFKD_QUICK_CHECK:
        case UCHAR_NFC_QUICK_CHECK:
        case UCHAR_NFKC_QUICK_CHECK:
            return (int32_t)unorm_getQuickCheck(c, (UNormalizationMode)(which-UCHAR_NFD_QUICK_CHECK+UNORM_NFD));
        case UCHAR_LEAD_CANONICAL_COMBINING_CLASS:
            return getFCD16(c)>>8;
        case UCHAR_TRAIL_CANONICAL_COMBINING_CLASS:
            return getFCD16(c)&0xff;
#endif
        case UCHAR_GRAPHEME_CLUSTER_BREAK:
            return (int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_GCB_MASK)>>UPROPS_GCB_SHIFT;
        case UCHAR_SENTENCE_BREAK:
            return (int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_SB_MASK)>>UPROPS_SB_SHIFT;
        case UCHAR_WORD_BREAK:
            return (int32_t)(u_getUnicodeProperties(c, 2)&UPROPS_WB_MASK)>>UPROPS_WB_SHIFT;
        default:
            return 0; /* undefined */
        }
    } else if(which==UCHAR_GENERAL_CATEGORY_MASK) {
        return U_MASK(u_charType(c));
    } else {
        return 0; /* undefined */
    }
}

U_CAPI int32_t U_EXPORT2
u_getIntPropertyMinValue(UProperty which) {
    return 0; /* all binary/enum/int properties have a minimum value of 0 */
}

U_CAPI int32_t U_EXPORT2
u_getIntPropertyMaxValue(UProperty which) {
    if(which<UCHAR_BINARY_START) {
        return -1; /* undefined */
    } else if(which<UCHAR_BINARY_LIMIT) {
        return 1; /* maximum TRUE for all binary properties */
    } else if(which<UCHAR_INT_START) {
        return -1; /* undefined */
    } else if(which<UCHAR_INT_LIMIT) {
        switch(which) {
        case UCHAR_BIDI_CLASS:
        case UCHAR_JOINING_GROUP:
        case UCHAR_JOINING_TYPE:
            return ubidi_getMaxValue(GET_BIDI_PROPS(), which);
        case UCHAR_BLOCK:
            return (uprv_getMaxValues(0)&UPROPS_BLOCK_MASK)>>UPROPS_BLOCK_SHIFT;
        case UCHAR_CANONICAL_COMBINING_CLASS:
        case UCHAR_LEAD_CANONICAL_COMBINING_CLASS:
        case UCHAR_TRAIL_CANONICAL_COMBINING_CLASS:
            return 0xff; /* TODO do we need to be more precise, getting the actual maximum? */
        case UCHAR_DECOMPOSITION_TYPE:
            return uprv_getMaxValues(2)&UPROPS_DT_MASK;
        case UCHAR_EAST_ASIAN_WIDTH:
            return (uprv_getMaxValues(0)&UPROPS_EA_MASK)>>UPROPS_EA_SHIFT;
        case UCHAR_GENERAL_CATEGORY:
            return (int32_t)U_CHAR_CATEGORY_COUNT-1;
        case UCHAR_LINE_BREAK:
            return (uprv_getMaxValues(UPROPS_LB_VWORD)&UPROPS_LB_MASK)>>UPROPS_LB_SHIFT;
        case UCHAR_NUMERIC_TYPE:
            return (int32_t)U_NT_COUNT-1;
        case UCHAR_SCRIPT:
            return uprv_getMaxValues(0)&UPROPS_SCRIPT_MASK;
        case UCHAR_HANGUL_SYLLABLE_TYPE:
            return (int32_t)U_HST_COUNT-1;
#if !UCONFIG_NO_NORMALIZATION
        case UCHAR_NFD_QUICK_CHECK:
        case UCHAR_NFKD_QUICK_CHECK:
            return (int32_t)UNORM_YES; /* these are never "maybe", only "no" or "yes" */
        case UCHAR_NFC_QUICK_CHECK:
        case UCHAR_NFKC_QUICK_CHECK:
            return (int32_t)UNORM_MAYBE;
#endif
        case UCHAR_GRAPHEME_CLUSTER_BREAK:
            return (uprv_getMaxValues(2)&UPROPS_GCB_MASK)>>UPROPS_GCB_SHIFT;
        case UCHAR_SENTENCE_BREAK:
            return (uprv_getMaxValues(2)&UPROPS_SB_MASK)>>UPROPS_SB_SHIFT;
        case UCHAR_WORD_BREAK:
            return (uprv_getMaxValues(2)&UPROPS_WB_MASK)>>UPROPS_WB_SHIFT;
        default:
            return -1; /* undefined */
        }
    } else {
        return -1; /* undefined */
    }
}

/*
 * TODO: Simplify, similar to binProps[].
 * Use an array of column/source, mask, shift values to drive returning simple
 * properties and their sources.
 *
 * TODO: Split the single propsvec into one per column, and have
 * upropsvec_addPropertyStarts() pass a trie value function that gets the
 * desired column's values.
 */
U_CFUNC UPropertySource U_EXPORT2
uprops_getSource(UProperty which) {
    if(which<UCHAR_BINARY_START) {
        return UPROPS_SRC_NONE; /* undefined */
    } else if(which<UCHAR_BINARY_LIMIT) {
        if(binProps[which].mask!=0) {
            return UPROPS_SRC_PROPSVEC;
        } else {
            return (UPropertySource)binProps[which].column;
        }
    } else if(which<UCHAR_INT_START) {
        return UPROPS_SRC_NONE; /* undefined */
    } else if(which<UCHAR_INT_LIMIT) {
        switch(which) {
        case UCHAR_GENERAL_CATEGORY:
        case UCHAR_NUMERIC_TYPE:
            return UPROPS_SRC_CHAR;

        case UCHAR_CANONICAL_COMBINING_CLASS:
        case UCHAR_NFD_QUICK_CHECK:
        case UCHAR_NFC_QUICK_CHECK:
        case UCHAR_LEAD_CANONICAL_COMBINING_CLASS:
        case UCHAR_TRAIL_CANONICAL_COMBINING_CLASS:
            return UPROPS_SRC_NFC;
        case UCHAR_NFKD_QUICK_CHECK:
        case UCHAR_NFKC_QUICK_CHECK:
            return UPROPS_SRC_NFKC;

        case UCHAR_BIDI_CLASS:
        case UCHAR_JOINING_GROUP:
        case UCHAR_JOINING_TYPE:
            return UPROPS_SRC_BIDI;

        default:
            return UPROPS_SRC_PROPSVEC;
        }
    } else if(which<UCHAR_STRING_START) {
        switch(which) {
        case UCHAR_GENERAL_CATEGORY_MASK:
        case UCHAR_NUMERIC_VALUE:
            return UPROPS_SRC_CHAR;

        default:
            return UPROPS_SRC_NONE;
        }
    } else if(which<UCHAR_STRING_LIMIT) {
        switch(which) {
        case UCHAR_AGE:
            return UPROPS_SRC_PROPSVEC;

        case UCHAR_BIDI_MIRRORING_GLYPH:
            return UPROPS_SRC_BIDI;

        case UCHAR_CASE_FOLDING:
        case UCHAR_LOWERCASE_MAPPING:
        case UCHAR_SIMPLE_CASE_FOLDING:
        case UCHAR_SIMPLE_LOWERCASE_MAPPING:
        case UCHAR_SIMPLE_TITLECASE_MAPPING:
        case UCHAR_SIMPLE_UPPERCASE_MAPPING:
        case UCHAR_TITLECASE_MAPPING:
        case UCHAR_UPPERCASE_MAPPING:
            return UPROPS_SRC_CASE;

        case UCHAR_ISO_COMMENT:
        case UCHAR_NAME:
        case UCHAR_UNICODE_1_NAME:
            return UPROPS_SRC_NAMES;

        default:
            return UPROPS_SRC_NONE;
        }
    } else {
        return UPROPS_SRC_NONE; /* undefined */
    }
}

#if !UCONFIG_NO_NORMALIZATION

U_CAPI int32_t U_EXPORT2
u_getFC_NFKC_Closure(UChar32 c, UChar *dest, int32_t destCapacity, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(destCapacity<0 || (dest==NULL && destCapacity>0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    // Compute the FC_NFKC_Closure on the fly:
    // We have the API for complete coverage of Unicode properties, although
    // this value by itself is not useful via API.
    // (What could be useful is a custom normalization table that combines
    // case folding and NFKC.)
    // For the derivation, see Unicode's DerivedNormalizationProps.txt.
    const Normalizer2 *nfkc=Normalizer2Factory::getNFKCInstance(*pErrorCode);
    const UCaseProps *csp=ucase_getSingleton(pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    // first: b = NFKC(Fold(a))
    UnicodeString folded1String;
    const UChar *folded1;
    int32_t folded1Length=ucase_toFullFolding(csp, c, &folded1, U_FOLD_CASE_DEFAULT);
    if(folded1Length<0) {
        const Normalizer2Impl *nfkcImpl=Normalizer2Factory::getImpl(nfkc);
        if(nfkcImpl->getCompQuickCheck(nfkcImpl->getNorm16(c))!=UNORM_NO) {
            return u_terminateUChars(dest, destCapacity, 0, pErrorCode);  // c does not change at all under CaseFolding+NFKC
        }
        folded1String.setTo(c);
    } else {
        if(folded1Length>UCASE_MAX_STRING_LENGTH) {
            folded1String.setTo(folded1Length);
        } else {
            folded1String.setTo(FALSE, folded1, folded1Length);
        }
    }
    UnicodeString kc1=nfkc->normalize(folded1String, *pErrorCode);
    // second: c = NFKC(Fold(b))
    UnicodeString folded2String(kc1);
    UnicodeString kc2=nfkc->normalize(folded2String.foldCase(), *pErrorCode);
    // if (c != b) add the mapping from a to c
    if(U_FAILURE(*pErrorCode) || kc1==kc2) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    } else {
        return kc2.extract(dest, destCapacity, *pErrorCode);
    }
}
#endif

/*----------------------------------------------------------------
 * Inclusions list
 *----------------------------------------------------------------*/

/*
 * Return a set of characters for property enumeration.
 * The set implicitly contains 0x110000 as well, which is one more than the highest
 * Unicode code point.
 *
 * This set is used as an ordered list - its code points are ordered, and
 * consecutive code points (in Unicode code point order) in the set define a range.
 * For each two consecutive characters (start, limit) in the set,
 * all of the UCD/normalization and related properties for
 * all code points start..limit-1 are all the same,
 * except for character names and ISO comments.
 *
 * All Unicode code points U+0000..U+10ffff are covered by these ranges.
 * The ranges define a partition of the Unicode code space.
 * ICU uses the inclusions set to enumerate properties for generating
 * UnicodeSets containing all code points that have a certain property value.
 *
 * The Inclusion List is generated from the UCD. It is generated
 * by enumerating the data tries, and code points for hardcoded properties
 * are added as well.
 *
 * --------------------------------------------------------------------------
 *
 * The following are ideas for getting properties-unique code point ranges,
 * with possible optimizations beyond the current implementation.
 * These optimizations would require more code and be more fragile.
 * The current implementation generates one single list (set) for all properties.
 *
 * To enumerate properties efficiently, one needs to know ranges of
 * repetitive values, so that the value of only each start code point
 * can be applied to the whole range.
 * This information is in principle available in the uprops.icu/unorm.icu data.
 *
 * There are two obstacles:
 *
 * 1. Some properties are computed from multiple data structures,
 *    making it necessary to get repetitive ranges by intersecting
 *    ranges from multiple tries.
 *
 * 2. It is not economical to write code for getting repetitive ranges
 *    that are precise for each of some 50 properties.
 *
 * Compromise ideas:
 *
 * - Get ranges per trie, not per individual property.
 *   Each range contains the same values for a whole group of properties.
 *   This would generate currently five range sets, two for uprops.icu tries
 *   and three for unorm.icu tries.
 *
 * - Combine sets of ranges for multiple tries to get sufficient sets
 *   for properties, e.g., the uprops.icu main and auxiliary tries
 *   for all non-normalization properties.
 *
 * Ideas for representing ranges and combining them:
 *
 * - A UnicodeSet could hold just the start code points of ranges.
 *   Multiple sets are easily combined by or-ing them together.
 *
 * - Alternatively, a UnicodeSet could hold each even-numbered range.
 *   All ranges could be enumerated by using each start code point
 *   (for the even-numbered ranges) as well as each limit (end+1) code point
 *   (for the odd-numbered ranges).
 *   It should be possible to combine two such sets by xor-ing them,
 *   but no more than two.
 *
 * The second way to represent ranges may(?!) yield smaller UnicodeSet arrays,
 * but the first one is certainly simpler and applicable for combining more than
 * two range sets.
 *
 * It is possible to combine all range sets for all uprops/unorm tries into one
 * set that can be used for all properties.
 * As an optimization, there could be less-combined range sets for certain
 * groups of properties.
 * The relationship of which less-combined range set to use for which property
 * depends on the implementation of the properties and must be hardcoded
 * - somewhat error-prone and higher maintenance but can be tested easily
 * by building property sets "the simple way" in test code.
 *
 * ---
 *
 * Do not use a UnicodeSet pattern because that causes infinite recursion;
 * UnicodeSet depends on the inclusions set.
 *
 * ---
 *
 * uprv_getInclusions() is commented out starting 2004-sep-13 because
 * uniset_props.cpp now calls the uxyz_addPropertyStarts() directly,
 * and only for the relevant property source.
 */
#if 0

U_CAPI void U_EXPORT2
uprv_getInclusions(const USetAdder *sa, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

#if !UCONFIG_NO_NORMALIZATION
    unorm_addPropertyStarts(sa, pErrorCode);
#endif
    uchar_addPropertyStarts(sa, pErrorCode);
    ucase_addPropertyStarts(ucase_getSingleton(pErrorCode), sa, pErrorCode);
    ubidi_addPropertyStarts(ubidi_getSingleton(pErrorCode), sa, pErrorCode);
}

#endif
