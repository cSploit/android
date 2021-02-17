
/*
 *
 * (C) Copyright IBM Corp. 1998-2009 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEScripts.h"
#include "LELanguages.h"

#include "LayoutEngine.h"
#include "ArabicLayoutEngine.h"
#include "CanonShaping.h"
#include "HanLayoutEngine.h"
#include "HangulLayoutEngine.h"
#include "IndicLayoutEngine.h"
#include "KhmerLayoutEngine.h"
#include "ThaiLayoutEngine.h"
#include "TibetanLayoutEngine.h"
#include "GXLayoutEngine.h"
#include "ScriptAndLanguageTags.h"
#include "CharSubstitutionFilter.h"

#include "LEGlyphStorage.h"

#include "OpenTypeUtilities.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "MorphTables.h"

#include "DefaultCharMapper.h"

#include "KernTable.h"

U_NAMESPACE_BEGIN

/* Leave this copyright notice here! It needs to go somewhere in this library. */
static const char copyright[] = U_COPYRIGHT_STRING;

const LEUnicode32 DefaultCharMapper::controlChars[] = {
    0x0009, 0x000A, 0x000D,
    /*0x200C, 0x200D,*/ 0x200E, 0x200F,
    0x2028, 0x2029, 0x202A, 0x202B, 0x202C, 0x202D, 0x202E,
    0x206A, 0x206B, 0x206C, 0x206D, 0x206E, 0x206F
};

const le_int32 DefaultCharMapper::controlCharsCount = LE_ARRAY_SIZE(controlChars);

LEUnicode32 DefaultCharMapper::mapChar(LEUnicode32 ch) const
{
    if (fFilterControls) {
        le_int32 index = OpenTypeUtilities::search((le_uint32)ch, (le_uint32 *)controlChars, controlCharsCount);

        if (controlChars[index] == ch) {
            return 0xFFFF;
        }
    }

    if (fMirror) {
        le_int32 index = OpenTypeUtilities::search((le_uint32) ch, (le_uint32 *)DefaultCharMapper::mirroredChars, DefaultCharMapper::mirroredCharsCount);

        if (mirroredChars[index] == ch) {
            return DefaultCharMapper::srahCderorrim[index];
        }
    }

    return ch;
}

// This is here to get it out of LEGlyphFilter.h.
// No particular reason to put it here, other than
// this is a good central location...
LEGlyphFilter::~LEGlyphFilter()
{
    // nothing to do
}

CharSubstitutionFilter::CharSubstitutionFilter(const LEFontInstance *fontInstance)
  : fFontInstance(fontInstance)
{
    // nothing to do
}

CharSubstitutionFilter::~CharSubstitutionFilter()
{
    // nothing to do
}

class CanonMarkFilter : public UMemory, public LEGlyphFilter
{
private:
    const GlyphClassDefinitionTable *classDefTable;

    CanonMarkFilter(const CanonMarkFilter &other); // forbid copying of this class
    CanonMarkFilter &operator=(const CanonMarkFilter &other); // forbid copying of this class

public:
    CanonMarkFilter(const GlyphDefinitionTableHeader *gdefTable);
    virtual ~CanonMarkFilter();

    virtual le_bool accept(LEGlyphID glyph) const;
};

CanonMarkFilter::CanonMarkFilter(const GlyphDefinitionTableHeader *gdefTable)
{
    classDefTable = gdefTable->getMarkAttachClassDefinitionTable();
}

CanonMarkFilter::~CanonMarkFilter()
{
    // nothing to do?
}

le_bool CanonMarkFilter::accept(LEGlyphID glyph) const
{
    le_int32 glyphClass = classDefTable->getGlyphClass(glyph);

    return glyphClass != 0;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LayoutEngine)

#define ccmpFeatureTag  LE_CCMP_FEATURE_TAG

#define ccmpFeatureMask 0x80000000UL

#define canonFeatures (ccmpFeatureMask)

static const FeatureMap canonFeatureMap[] =
{
    {ccmpFeatureTag, ccmpFeatureMask}
};

static const le_int32 canonFeatureMapCount = LE_ARRAY_SIZE(canonFeatureMap);

LayoutEngine::LayoutEngine(const LEFontInstance *fontInstance, 
                           le_int32 scriptCode, 
                           le_int32 languageCode, 
                           le_int32 typoFlags,
                           LEErrorCode &success)
  : fGlyphStorage(NULL), fFontInstance(fontInstance), fScriptCode(scriptCode), fLanguageCode(languageCode),
    fTypoFlags(typoFlags), fFilterZeroWidth(TRUE)
{
    if (LE_FAILURE(success)) {
        return;
    } 

    fGlyphStorage = new LEGlyphStorage();
    if (fGlyphStorage == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
    }
}

le_int32 LayoutEngine::getGlyphCount() const
{
    return fGlyphStorage->getGlyphCount();
}

void LayoutEngine::getCharIndices(le_int32 charIndices[], le_int32 indexBase, LEErrorCode &success) const
{
    fGlyphStorage->getCharIndices(charIndices, indexBase, success);
}

void LayoutEngine::getCharIndices(le_int32 charIndices[], LEErrorCode &success) const
{
    fGlyphStorage->getCharIndices(charIndices, success);
}

// Copy the glyphs into caller's (32-bit) glyph array, OR in extraBits
void LayoutEngine::getGlyphs(le_uint32 glyphs[], le_uint32 extraBits, LEErrorCode &success) const
{
    fGlyphStorage->getGlyphs(glyphs, extraBits, success);
}

void LayoutEngine::getGlyphs(LEGlyphID glyphs[], LEErrorCode &success) const
{
    fGlyphStorage->getGlyphs(glyphs, success);
}


void LayoutEngine::getGlyphPositions(float positions[], LEErrorCode &success) const
{
    fGlyphStorage->getGlyphPositions(positions, success);
}

void LayoutEngine::getGlyphPosition(le_int32 glyphIndex, float &x, float &y, LEErrorCode &success) const
{
    fGlyphStorage->getGlyphPosition(glyphIndex, x, y, success);
}

le_int32 LayoutEngine::characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
                LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    const GlyphSubstitutionTableHeader *canonGSUBTable = (GlyphSubstitutionTableHeader *) CanonShaping::glyphSubstitutionTable;
    LETag scriptTag  = OpenTypeLayoutEngine::getScriptTag(fScriptCode);
    LETag langSysTag = OpenTypeLayoutEngine::getLangSysTag(fLanguageCode);
    le_int32 i, dir = 1, out = 0, outCharCount = count;

    if (canonGSUBTable->coversScript(scriptTag)) {
        CharSubstitutionFilter *substitutionFilter = new CharSubstitutionFilter(fFontInstance);
        if (substitutionFilter == NULL) { 
            success = LE_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

		const LEUnicode *inChars = &chars[offset];
		LEUnicode *reordered = NULL;
        LEGlyphStorage fakeGlyphStorage;

        fakeGlyphStorage.allocateGlyphArray(count, rightToLeft, success);

        if (LE_FAILURE(success)) {
            delete substitutionFilter;
            return 0;
        }

		// This is the cheapest way to get mark reordering only for Hebrew.
		// We could just do the mark reordering for all scripts, but most
		// of them probably don't need it...
		if (fScriptCode == hebrScriptCode) {
			reordered = LE_NEW_ARRAY(LEUnicode, count);

			if (reordered == NULL) {
                delete substitutionFilter;
				success = LE_MEMORY_ALLOCATION_ERROR;
				return 0;
			}

			CanonShaping::reorderMarks(&chars[offset], count, rightToLeft, reordered, fakeGlyphStorage);
			inChars = reordered;
        }

        fakeGlyphStorage.allocateAuxData(success);

        if (LE_FAILURE(success)) {
            delete substitutionFilter;
            return 0;
        }

        if (rightToLeft) {
            out = count - 1;
            dir = -1;
        }

        for (i = 0; i < count; i += 1, out += dir) {
            fakeGlyphStorage[out] = (LEGlyphID) inChars[i];
            fakeGlyphStorage.setAuxData(out, canonFeatures, success);
        }

		if (reordered != NULL) {
			LE_DELETE_ARRAY(reordered);
		}

        outCharCount = canonGSUBTable->process(fakeGlyphStorage, rightToLeft, scriptTag, langSysTag, NULL, substitutionFilter, canonFeatureMap, canonFeatureMapCount, FALSE, success);

        if (LE_FAILURE(success)) {
            delete substitutionFilter;
            return 0;
        }

        out = (rightToLeft? outCharCount - 1 : 0);

        /*
         * The char indices array in fakeGlyphStorage has the correct mapping
         * back to the original input characters. Save it in glyphStorage. The
         * subsequent call to glyphStoratge.allocateGlyphArray will keep this
         * array rather than allocating and initializing a new one.
         */
        glyphStorage.adoptCharIndicesArray(fakeGlyphStorage);

        outChars = LE_NEW_ARRAY(LEUnicode, outCharCount);

        if (outChars == NULL) {
            delete substitutionFilter;
            success = LE_MEMORY_ALLOCATION_ERROR;
            return 0;
        }

        for (i = 0; i < outCharCount; i += 1, out += dir) {
            outChars[out] = (LEUnicode) LE_GET_GLYPH(fakeGlyphStorage[i]);
        }

        delete substitutionFilter;
    }

    return outCharCount;
}

le_int32 LayoutEngine::computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
                                            LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    LEUnicode *outChars = NULL;
    le_int32 outCharCount = characterProcessing(chars, offset, count, max, rightToLeft, outChars, glyphStorage, success);

    if (outChars != NULL) {
        mapCharsToGlyphs(outChars, 0, outCharCount, rightToLeft, rightToLeft, glyphStorage, success);
        LE_DELETE_ARRAY(outChars); // FIXME: a subclass may have allocated this, in which case this delete might not work...
    } else {
        mapCharsToGlyphs(chars, offset, count, rightToLeft, rightToLeft, glyphStorage, success);
    }

    return glyphStorage.getGlyphCount();
}

// Input: glyphs
// Output: positions
void LayoutEngine::positionGlyphs(LEGlyphStorage &glyphStorage, float x, float y, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return;
    }

    glyphStorage.allocatePositions(success);

    if (LE_FAILURE(success)) {
        return;
    }

    le_int32 i, glyphCount = glyphStorage.getGlyphCount();

    for (i = 0; i < glyphCount; i += 1) {
        LEPoint advance;

        glyphStorage.setPosition(i, x, y, success);

        fFontInstance->getGlyphAdvance(glyphStorage[i], advance);
        x += advance.fX;
        y += advance.fY;
    }

    glyphStorage.setPosition(glyphCount, x, y, success);
}

void LayoutEngine::adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool  reverse,
                                        LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return;
    }

    if (chars == NULL || offset < 0 || count < 0) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    GlyphDefinitionTableHeader *gdefTable = (GlyphDefinitionTableHeader *) CanonShaping::glyphDefinitionTable;
    CanonMarkFilter filter(gdefTable);

    adjustMarkGlyphs(&chars[offset], count, reverse, glyphStorage, &filter, success);

    if (fTypoFlags & 0x1) { /* kerning enabled */
      static const le_uint32 kernTableTag = LE_KERN_TABLE_TAG;

      KernTable kt(fFontInstance, getFontTable(kernTableTag));
      kt.process(glyphStorage);
    }

    // default is no adjustments
    return;
}

void LayoutEngine::adjustMarkGlyphs(LEGlyphStorage &glyphStorage, LEGlyphFilter *markFilter, LEErrorCode &success)
{
    float xAdjust = 0;
    le_int32 p, glyphCount = glyphStorage.getGlyphCount();

    if (LE_FAILURE(success)) {
        return;
    }

    if (markFilter == NULL) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    float ignore, prev;

    glyphStorage.getGlyphPosition(0, prev, ignore, success);

    for (p = 0; p < glyphCount; p += 1) {
        float next, xAdvance;
        
        glyphStorage.getGlyphPosition(p + 1, next, ignore, success);

        xAdvance = next - prev;
        glyphStorage.adjustPosition(p, xAdjust, 0, success);

        if (markFilter->accept(glyphStorage[p])) {
            xAdjust -= xAdvance;
        }

        prev = next;
    }

    glyphStorage.adjustPosition(glyphCount, xAdjust, 0, success);
}

void LayoutEngine::adjustMarkGlyphs(const LEUnicode chars[], le_int32 charCount, le_bool reverse, LEGlyphStorage &glyphStorage, LEGlyphFilter *markFilter, LEErrorCode &success)
{
    float xAdjust = 0;
    le_int32 c = 0, direction = 1, p;
    le_int32 glyphCount = glyphStorage.getGlyphCount();

    if (LE_FAILURE(success)) {
        return;
    }

    if (markFilter == NULL) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if (reverse) {
        c = glyphCount - 1;
        direction = -1;
    }

    float ignore, prev;

    glyphStorage.getGlyphPosition(0, prev, ignore, success);

    for (p = 0; p < charCount; p += 1, c += direction) {
        float next, xAdvance;
        
        glyphStorage.getGlyphPosition(p + 1, next, ignore, success);

        xAdvance = next - prev;
        glyphStorage.adjustPosition(p, xAdjust, 0, success);

        if (markFilter->accept(chars[c])) {
            xAdjust -= xAdvance;
        }

        prev = next;
    }

    glyphStorage.adjustPosition(glyphCount, xAdjust, 0, success);
}

const void *LayoutEngine::getFontTable(LETag tableTag) const
{
    return fFontInstance->getFontTable(tableTag);
}

void LayoutEngine::mapCharsToGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse, le_bool mirror,
                                    LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return;
    }

    glyphStorage.allocateGlyphArray(count, reverse, success);

    DefaultCharMapper charMapper(TRUE, mirror);

    fFontInstance->mapCharsToGlyphs(chars, offset, count, reverse, &charMapper, fFilterZeroWidth, glyphStorage);
}

// Input: characters, font?
// Output: glyphs, positions, char indices
// Returns: number of glyphs
le_int32 LayoutEngine::layoutChars(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
                              float x, float y, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    le_int32 glyphCount;

    if (fGlyphStorage->getGlyphCount() > 0) {
        fGlyphStorage->reset();
    }
    
    glyphCount = computeGlyphs(chars, offset, count, max, rightToLeft, *fGlyphStorage, success);
    positionGlyphs(*fGlyphStorage, x, y, success);
    adjustGlyphPositions(chars, offset, count, rightToLeft, *fGlyphStorage, success);

    return glyphCount;
}

void LayoutEngine::reset()
{
    fGlyphStorage->reset();
}
    
LayoutEngine *LayoutEngine::layoutEngineFactory(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, LEErrorCode &success)
{
  // 3 -> kerning and ligatures
  return LayoutEngine::layoutEngineFactory(fontInstance, scriptCode, languageCode, 3, success);
}
    
LayoutEngine *LayoutEngine::layoutEngineFactory(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags, LEErrorCode &success)
{
    static const le_uint32 gsubTableTag = LE_GSUB_TABLE_TAG;
    static const le_uint32 mortTableTag = LE_MORT_TABLE_TAG;

    if (LE_FAILURE(success)) {
        return NULL;
    }

    const GlyphSubstitutionTableHeader *gsubTable = (const GlyphSubstitutionTableHeader *) fontInstance->getFontTable(gsubTableTag);
    LayoutEngine *result = NULL;
    LETag scriptTag   = 0x00000000;
    LETag languageTag = 0x00000000;
	LETag v2ScriptTag = OpenTypeLayoutEngine::getV2ScriptTag(scriptCode);

    // Right now, only invoke V2 processing for Devanagari.  TODO: Allow more V2 scripts as they are
    // properly tested.

	if ( v2ScriptTag == dev2ScriptTag && gsubTable != NULL && gsubTable->coversScript( v2ScriptTag )) {
		result = new IndicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, TRUE, gsubTable, success);
	}
    else if (gsubTable != NULL && gsubTable->coversScript(scriptTag = OpenTypeLayoutEngine::getScriptTag(scriptCode))) {
        switch (scriptCode) {
        case bengScriptCode:
        case devaScriptCode:
        case gujrScriptCode:
        case kndaScriptCode:
        case mlymScriptCode:
        case oryaScriptCode:
        case guruScriptCode:
        case tamlScriptCode:
        case teluScriptCode:
        case sinhScriptCode:
            result = new IndicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, FALSE, gsubTable, success);
            break;

        case arabScriptCode:
            result = new ArabicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
            break;

        case hangScriptCode:
            result = new HangulOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
            break;

        case haniScriptCode:
            languageTag = OpenTypeLayoutEngine::getLangSysTag(languageCode);

            switch (languageCode) {
            case korLanguageCode:
            case janLanguageCode:
            case zhtLanguageCode:
            case zhsLanguageCode:
                if (gsubTable->coversScriptAndLanguage(scriptTag, languageTag, TRUE)) {
                    result = new HanOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
                    break;
                }

                // note: falling through to default case.
            default:
                result = new OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
                break;
            }

            break;

        case tibtScriptCode:
            result = new TibetanOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
            break;

        case khmrScriptCode:
            result = new KhmerOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
            break;

        default:
            result = new OpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, gsubTable, success);
            break;
        }
    } else {
        const MorphTableHeader *morphTable = (MorphTableHeader *) fontInstance->getFontTable(mortTableTag);

        if (morphTable != NULL) {
            result = new GXLayoutEngine(fontInstance, scriptCode, languageCode, morphTable, success);
        } else {
            switch (scriptCode) {
            case bengScriptCode:
            case devaScriptCode:
            case gujrScriptCode:
            case kndaScriptCode:
            case mlymScriptCode:
            case oryaScriptCode:
            case guruScriptCode:
            case tamlScriptCode:
            case teluScriptCode:
            case sinhScriptCode:
            {
                result = new IndicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success);
                break;
            }

            case arabScriptCode:
            //case hebrScriptCode:
                result = new UnicodeArabicOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success);
                break;

            //case hebrScriptCode:
            //    return new HebrewOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags);

            case thaiScriptCode:
                result = new ThaiLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success);
                break;

            case hangScriptCode:
                result = new HangulOpenTypeLayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success);
                break;

            default:
                result = new LayoutEngine(fontInstance, scriptCode, languageCode, typoFlags, success);
                break;
            }
        }
    }

    if (result && LE_FAILURE(success)) {
		delete result;
		result = NULL;
	}

    if (result == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
    }

    return result;
}

LayoutEngine::~LayoutEngine() {
    delete fGlyphStorage;
}

U_NAMESPACE_END
