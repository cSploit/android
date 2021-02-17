/*
 *
 * (C) Copyright IBM Corp. 1998-2010 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "MarkToBasePosnSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEGlyphID MarkToBasePositioningSubtable::findBaseGlyph(GlyphIterator *glyphIterator) const
{
    if (glyphIterator->prev()) {
        return glyphIterator->getCurrGlyphID();
    }

    return 0xFFFF;
}

le_int32 MarkToBasePositioningSubtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    LEGlyphID markGlyph = glyphIterator->getCurrGlyphID();
    le_int32 markCoverage = getGlyphCoverage((LEGlyphID) markGlyph);

    if (markCoverage < 0) {
        // markGlyph isn't a covered mark glyph
        return 0;
    }

    LEPoint markAnchor;
    const MarkArray *markArray = (const MarkArray *) ((char *) this + SWAPW(markArrayOffset));
    le_int32 markClass = markArray->getMarkClass(markGlyph, markCoverage, fontInstance, markAnchor);
    le_uint16 mcCount = SWAPW(classCount);

    if (markClass < 0 || markClass >= mcCount) {
        // markGlyph isn't in the mark array or its
        // mark class is too big. The table is mal-formed!
        return 0;
    }

    // FIXME: We probably don't want to find a base glyph before a previous ligature... 
    GlyphIterator baseIterator(*glyphIterator, (le_uint16) (lfIgnoreMarks /*| lfIgnoreLigatures*/));
    LEGlyphID baseGlyph = findBaseGlyph(&baseIterator);
    le_int32 baseCoverage = getBaseCoverage((LEGlyphID) baseGlyph);
    const BaseArray *baseArray = (const BaseArray *) ((char *) this + SWAPW(baseArrayOffset));
    le_uint16 baseCount = SWAPW(baseArray->baseRecordCount);

    if (baseCoverage < 0 || baseCoverage >= baseCount) {
        // The base glyph isn't covered, or the coverage
        // index is too big. The latter means that the
        // table is mal-formed...
        return 0;
    }

    const BaseRecord *baseRecord = &baseArray->baseRecordArray[baseCoverage * mcCount];
    Offset anchorTableOffset = SWAPW(baseRecord->baseAnchorTableOffsetArray[markClass]);
    const AnchorTable *anchorTable = (const AnchorTable *) ((char *) baseArray + anchorTableOffset);
    LEPoint baseAnchor, markAdvance, pixels;

    if (anchorTableOffset == 0) {
        // this means the table is mal-formed...
        glyphIterator->setCurrGlyphBaseOffset(baseIterator.getCurrStreamPosition());
        return 0;
    }

    anchorTable->getAnchor(baseGlyph, fontInstance, baseAnchor);

    fontInstance->getGlyphAdvance(markGlyph, pixels);
    fontInstance->pixelsToUnits(pixels, markAdvance);

    float anchorDiffX = baseAnchor.fX - markAnchor.fX;
    float anchorDiffY = baseAnchor.fY - markAnchor.fY;

    glyphIterator->setCurrGlyphBaseOffset(baseIterator.getCurrStreamPosition());

    if (glyphIterator->isRightToLeft()) {
        // FIXME: need similar patch to below; also in MarkToLigature and MarkToMark
        // (is there a better way to approach this for all the cases?)
        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX, anchorDiffY, -markAdvance.fX, -markAdvance.fY);
    } else {
        LEPoint baseAdvance;

        fontInstance->getGlyphAdvance(baseGlyph, pixels);
        
        //JK: adjustment needs to account for non-zero advance of any marks between base glyph and current mark
        GlyphIterator gi(baseIterator, (le_uint16)0); // copy of baseIterator that won't ignore marks
        gi.next(); // point beyond the base glyph
        while (gi.getCurrStreamPosition() < glyphIterator->getCurrStreamPosition()) { // for all intervening glyphs (marks)...
            LEGlyphID otherMark = gi.getCurrGlyphID();
            LEPoint px;
            fontInstance->getGlyphAdvance(otherMark, px); // get advance, in case it's non-zero
            pixels.fX += px.fX; // and add that to the base glyph's advance
            pixels.fY += px.fY;
            gi.next();
        }
        // end of JK patch
   
        fontInstance->pixelsToUnits(pixels, baseAdvance);

        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX - baseAdvance.fX, anchorDiffY - baseAdvance.fY, -markAdvance.fX, -markAdvance.fY);
    }

    return 1;
}

U_NAMESPACE_END
