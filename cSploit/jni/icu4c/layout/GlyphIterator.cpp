/*
 *
 * (C) Copyright IBM Corp. 1998-2008 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositionAdjustments.h"
#include "GlyphIterator.h"
#include "LEGlyphStorage.h"
#include "Lookups.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

GlyphIterator::GlyphIterator(LEGlyphStorage &theGlyphStorage, GlyphPositionAdjustments *theGlyphPositionAdjustments, le_bool rightToLeft, le_uint16 theLookupFlags,
                             FeatureMask theFeatureMask, const GlyphDefinitionTableHeader *theGlyphDefinitionTableHeader)
  : direction(1), position(-1), nextLimit(-1), prevLimit(-1),
    glyphStorage(theGlyphStorage), glyphPositionAdjustments(theGlyphPositionAdjustments),
    srcIndex(-1), destIndex(-1), lookupFlags(theLookupFlags), featureMask(theFeatureMask), glyphGroup(0),
    glyphClassDefinitionTable(NULL), markAttachClassDefinitionTable(NULL)

{
    le_int32 glyphCount = glyphStorage.getGlyphCount();

    if (theGlyphDefinitionTableHeader != NULL) {
        glyphClassDefinitionTable = theGlyphDefinitionTableHeader->getGlyphClassDefinitionTable();
        markAttachClassDefinitionTable = theGlyphDefinitionTableHeader->getMarkAttachClassDefinitionTable();
    }

    nextLimit = glyphCount;

    if (rightToLeft) {
        direction = -1;
        position = glyphCount;
        nextLimit = -1;
        prevLimit = glyphCount;
    }
}

GlyphIterator::GlyphIterator(GlyphIterator &that)
  : glyphStorage(that.glyphStorage)
{
    direction    = that.direction;
    position     = that.position;
    nextLimit    = that.nextLimit;
    prevLimit    = that.prevLimit;

    glyphPositionAdjustments = that.glyphPositionAdjustments;
    srcIndex = that.srcIndex;
    destIndex = that.destIndex;
    lookupFlags = that.lookupFlags;
    featureMask = that.featureMask;
    glyphGroup  = that.glyphGroup;
    glyphClassDefinitionTable = that.glyphClassDefinitionTable;
    markAttachClassDefinitionTable = that.markAttachClassDefinitionTable;
}

GlyphIterator::GlyphIterator(GlyphIterator &that, FeatureMask newFeatureMask)
  : glyphStorage(that.glyphStorage)
{
    direction    = that.direction;
    position     = that.position;
    nextLimit    = that.nextLimit;
    prevLimit    = that.prevLimit;

    glyphPositionAdjustments = that.glyphPositionAdjustments;
    srcIndex = that.srcIndex;
    destIndex = that.destIndex;
    lookupFlags = that.lookupFlags;
    featureMask = newFeatureMask;
    glyphGroup  = 0;
    glyphClassDefinitionTable = that.glyphClassDefinitionTable;
    markAttachClassDefinitionTable = that.markAttachClassDefinitionTable;
}

GlyphIterator::GlyphIterator(GlyphIterator &that, le_uint16 newLookupFlags)
  : glyphStorage(that.glyphStorage)
{
    direction    = that.direction;
    position     = that.position;
    nextLimit    = that.nextLimit;
    prevLimit    = that.prevLimit;

    glyphPositionAdjustments = that.glyphPositionAdjustments;
    srcIndex = that.srcIndex;
    destIndex = that.destIndex;
    lookupFlags = newLookupFlags;
    featureMask = that.featureMask;
    glyphGroup  = that.glyphGroup;
    glyphClassDefinitionTable = that.glyphClassDefinitionTable;
    markAttachClassDefinitionTable = that.markAttachClassDefinitionTable;
}

GlyphIterator::~GlyphIterator()
{
    // nothing to do, right?
}

void GlyphIterator::reset(le_uint16 newLookupFlags, FeatureMask newFeatureMask)
{
    position     = prevLimit;
    featureMask  = newFeatureMask;
    glyphGroup   = 0;
    lookupFlags  = newLookupFlags;
}

LEGlyphID *GlyphIterator::insertGlyphs(le_int32 count, LEErrorCode& success)
{
    return glyphStorage.insertGlyphs(position, count, success);
}

le_int32 GlyphIterator::applyInsertions()
{
    le_int32 newGlyphCount = glyphStorage.applyInsertions();

    if (direction < 0) {
        prevLimit = newGlyphCount;
    } else {
        nextLimit = newGlyphCount;
    }

    return newGlyphCount;
}

le_int32 GlyphIterator::getCurrStreamPosition() const
{
    return position;
}

le_bool GlyphIterator::isRightToLeft() const
{
    return direction < 0;
}

le_bool GlyphIterator::ignoresMarks() const
{
    return (lookupFlags & lfIgnoreMarks) != 0;
}

le_bool GlyphIterator::baselineIsLogicalEnd() const
{
    return (lookupFlags & lfBaselineIsLogicalEnd) != 0;
}

LEGlyphID GlyphIterator::getCurrGlyphID() const
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return 0xFFFF;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return 0xFFFF;
        }
    }

    return glyphStorage[position];
}

void GlyphIterator::getCursiveEntryPoint(LEPoint &entryPoint) const
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->getEntryPoint(position, entryPoint);
}

void GlyphIterator::getCursiveExitPoint(LEPoint &exitPoint) const
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->getExitPoint(position, exitPoint);
}

void GlyphIterator::setCurrGlyphID(TTGlyphID glyphID)
{
    LEGlyphID glyph = glyphStorage[position];

    glyphStorage[position] = LE_SET_GLYPH(glyph, glyphID);
}

void GlyphIterator::setCurrStreamPosition(le_int32 newPosition)
{
    if (direction < 0) {
        if (newPosition >= prevLimit) {
            position = prevLimit;
            return;
        }

        if (newPosition <= nextLimit) {
            position = nextLimit;
            return;
        }
    } else {
        if (newPosition <= prevLimit) {
            position = prevLimit;
            return;
        }

        if (newPosition >= nextLimit) {
            position = nextLimit;
            return;
        }
    }

    position = newPosition - direction;
    next();
}

void GlyphIterator::setCurrGlyphBaseOffset(le_int32 baseOffset)
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->setBaseOffset(position, baseOffset);
}

void GlyphIterator::adjustCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                                      float xAdvanceAdjust, float yAdvanceAdjust)
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->adjustXPlacement(position, xPlacementAdjust);
    glyphPositionAdjustments->adjustYPlacement(position, yPlacementAdjust);
    glyphPositionAdjustments->adjustXAdvance(position, xAdvanceAdjust);
    glyphPositionAdjustments->adjustYAdvance(position, yAdvanceAdjust);
}

void GlyphIterator::setCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                                      float xAdvanceAdjust, float yAdvanceAdjust)
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->setXPlacement(position, xPlacementAdjust);
    glyphPositionAdjustments->setYPlacement(position, yPlacementAdjust);
    glyphPositionAdjustments->setXAdvance(position, xAdvanceAdjust);
    glyphPositionAdjustments->setYAdvance(position, yAdvanceAdjust);
}

void GlyphIterator::clearCursiveEntryPoint()
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->clearEntryPoint(position);
}

void GlyphIterator::clearCursiveExitPoint()
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->clearExitPoint(position);
}

void GlyphIterator::setCursiveEntryPoint(LEPoint &entryPoint)
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->setEntryPoint(position, entryPoint, baselineIsLogicalEnd());
}

void GlyphIterator::setCursiveExitPoint(LEPoint &exitPoint)
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->setExitPoint(position, exitPoint, baselineIsLogicalEnd());
}

void GlyphIterator::setCursiveGlyph()
{
    if (direction < 0) {
        if (position <= nextLimit || position >= prevLimit) {
            return;
        }
    } else {
        if (position <= prevLimit || position >= nextLimit) {
            return;
        }
    }

    glyphPositionAdjustments->setCursiveGlyph(position, baselineIsLogicalEnd());
}

le_bool GlyphIterator::filterGlyph(le_uint32 index) const
{
    LEGlyphID glyphID = glyphStorage[index];
    le_int32 glyphClass = gcdNoGlyphClass;

    if (LE_GET_GLYPH(glyphID) >= 0xFFFE) {
        return TRUE;
    }

    if (glyphClassDefinitionTable != NULL) {
        glyphClass = glyphClassDefinitionTable->getGlyphClass(glyphID);
    }

    switch (glyphClass)
    {
    case gcdNoGlyphClass:
        return FALSE;

    case gcdSimpleGlyph:
        return (lookupFlags & lfIgnoreBaseGlyphs) != 0;

    case gcdLigatureGlyph:
        return (lookupFlags & lfIgnoreLigatures) != 0;

    case gcdMarkGlyph:
    {
        if ((lookupFlags & lfIgnoreMarks) != 0) {
            return TRUE;
        }

        le_uint16 markAttachType = (lookupFlags & lfMarkAttachTypeMask) >> lfMarkAttachTypeShift;

        if ((markAttachType != 0) && (markAttachClassDefinitionTable != NULL)) {
            return markAttachClassDefinitionTable->getGlyphClass(glyphID) != markAttachType;
        }

        return FALSE;
    }

    case gcdComponentGlyph:
        return (lookupFlags & lfIgnoreBaseGlyphs) != 0;

    default:
        return FALSE;
    }
}

le_bool GlyphIterator::hasFeatureTag(le_bool matchGroup) const
{
    if (featureMask == 0) {
        return TRUE;
    }

    LEErrorCode success = LE_NO_ERROR;
    FeatureMask fm = glyphStorage.getAuxData(position, success);

    return ((fm & featureMask) == featureMask) && (!matchGroup || (le_int32)(fm & LE_GLYPH_GROUP_MASK) == glyphGroup);
}

le_bool GlyphIterator::findFeatureTag()
{
  //glyphGroup = 0;

    while (nextInternal()) {
        if (hasFeatureTag(FALSE)) {
            LEErrorCode success = LE_NO_ERROR;

            glyphGroup = (glyphStorage.getAuxData(position, success) & LE_GLYPH_GROUP_MASK);
            return TRUE;
        }
    }

    return FALSE;
}


le_bool GlyphIterator::nextInternal(le_uint32 delta)
{
    le_int32 newPosition = position;

    while (newPosition != nextLimit && delta > 0) {
        do {
            newPosition += direction;
        } while (newPosition != nextLimit && filterGlyph(newPosition));

        delta -= 1;
    }

    position = newPosition;

    return position != nextLimit;
}

le_bool GlyphIterator::next(le_uint32 delta)
{
    return nextInternal(delta) && hasFeatureTag(TRUE);
}

le_bool GlyphIterator::prevInternal(le_uint32 delta)
{
    le_int32 newPosition = position;

    while (newPosition != prevLimit && delta > 0) {
        do {
            newPosition -= direction;
        } while (newPosition != prevLimit && filterGlyph(newPosition));

        delta -= 1;
    }

    position = newPosition;

    return position != prevLimit;
}

le_bool GlyphIterator::prev(le_uint32 delta)
{
    return prevInternal(delta) && hasFeatureTag(TRUE);
}

le_int32 GlyphIterator::getMarkComponent(le_int32 markPosition) const
{
    le_int32 component = 0;
    le_int32 posn;

    for (posn = position; posn != markPosition; posn += direction) {
        if (glyphStorage[posn] == 0xFFFE) {
            component += 1;
        }
    }

    return component;
}

// This is basically prevInternal except that it
// doesn't take a delta argument, and it doesn't
// filter out 0xFFFE glyphs.
le_bool GlyphIterator::findMark2Glyph()
{
    le_int32 newPosition = position;

    do {
        newPosition -= direction;
    } while (newPosition != prevLimit && glyphStorage[newPosition] != 0xFFFE && filterGlyph(newPosition));

    position = newPosition;

    return position != prevLimit;
}

U_NAMESPACE_END
