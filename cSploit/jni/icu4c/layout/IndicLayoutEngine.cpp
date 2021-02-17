
/*
 *
 * (C) Copyright IBM Corp. 1998-2009 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LayoutEngine.h"
#include "OpenTypeLayoutEngine.h"
#include "IndicLayoutEngine.h"
#include "ScriptAndLanguageTags.h"

#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"

#include "GDEFMarkFilter.h"
#include "LEGlyphStorage.h"

#include "IndicReordering.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IndicOpenTypeLayoutEngine)

IndicOpenTypeLayoutEngine::IndicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                    le_int32 typoFlags, le_bool version2, const GlyphSubstitutionTableHeader *gsubTable, LEErrorCode &success)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success), fMPreFixups(NULL)
{
	if ( version2 ) {
		fFeatureMap = IndicReordering::getv2FeatureMap(fFeatureMapCount);
	} else {
        fFeatureMap = IndicReordering::getFeatureMap(fFeatureMapCount);
	}
	fFeatureOrder = TRUE;
    fVersion2 = version2;
    fFilterZeroWidth = IndicReordering::getFilterZeroWidth(fScriptCode);
}

IndicOpenTypeLayoutEngine::IndicOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags, LEErrorCode &success)
    : OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success), fMPreFixups(NULL)
{
    fFeatureMap = IndicReordering::getFeatureMap(fFeatureMapCount);
    fFeatureOrder = TRUE;
	fVersion2 =  FALSE;
}

IndicOpenTypeLayoutEngine::~IndicOpenTypeLayoutEngine()
{
    // nothing to do
}

// Input: characters, tags
// Output: glyphs, char indices
le_int32 IndicOpenTypeLayoutEngine::glyphProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
                    LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    le_int32 retCount = OpenTypeLayoutEngine::glyphProcessing(chars, offset, count, max, rightToLeft, glyphStorage, success);

    if (LE_FAILURE(success)) {
        return 0;
    }

    if (fVersion2) {
        IndicReordering::finalReordering(glyphStorage,retCount);
        IndicReordering::applyPresentationForms(glyphStorage,retCount);
        OpenTypeLayoutEngine::glyphSubstitution(count,max, rightToLeft, glyphStorage, success);
    } else {
        IndicReordering::adjustMPres(fMPreFixups, glyphStorage, success);
    }
    return retCount;
}

// Input: characters
// Output: characters, char indices, tags
// Returns: output character count
le_int32 IndicOpenTypeLayoutEngine::characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    le_int32 worstCase = count * IndicReordering::getWorstCaseExpansion(fScriptCode);

    outChars = LE_NEW_ARRAY(LEUnicode, worstCase);

    if (outChars == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphStorage.allocateGlyphArray(worstCase, rightToLeft, success);
    glyphStorage.allocateAuxData(success);

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        return 0;
    }

    // NOTE: assumes this allocates featureTags...
    // (probably better than doing the worst case stuff here...)

    le_int32 outCharCount;
    if (fVersion2) {
        outCharCount = IndicReordering::v2process(&chars[offset], count, fScriptCode, outChars, glyphStorage);
    } else {
        outCharCount = IndicReordering::reorder(&chars[offset], count, fScriptCode, outChars, glyphStorage, &fMPreFixups, success);
    }

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        return 0;
    }

    glyphStorage.adoptGlyphCount(outCharCount);
    return outCharCount;
}

U_NAMESPACE_END
