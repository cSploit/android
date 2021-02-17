/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "StateTables.h"
#include "MorphStateTables.h"
#include "SubtableProcessor.h"
#include "StateTableProcessor.h"
#include "LigatureSubstProc.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

#define ExtendedComplement(m) ((le_int32) (~((le_uint32) (m))))
#define SignBit(m) ((ExtendedComplement(m) >> 1) & (le_int32)(m))
#define SignExtend(v,m) (((v) & SignBit(m))? ((v) | ExtendedComplement(m)): (v))

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(LigatureSubstitutionProcessor)

LigatureSubstitutionProcessor::LigatureSubstitutionProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : StateTableProcessor(morphSubtableHeader)
{
    ligatureSubstitutionHeader = (const LigatureSubstitutionHeader *) morphSubtableHeader;
    ligatureActionTableOffset = SWAPW(ligatureSubstitutionHeader->ligatureActionTableOffset);
    componentTableOffset = SWAPW(ligatureSubstitutionHeader->componentTableOffset);
    ligatureTableOffset = SWAPW(ligatureSubstitutionHeader->ligatureTableOffset);

    entryTable = (const LigatureSubstitutionStateEntry *) ((char *) &stateTableHeader->stHeader + entryTableOffset);
}

LigatureSubstitutionProcessor::~LigatureSubstitutionProcessor()
{
}

void LigatureSubstitutionProcessor::beginStateTable()
{
    m = -1;
}

ByteOffset LigatureSubstitutionProcessor::processStateEntry(LEGlyphStorage &glyphStorage, le_int32 &currGlyph, EntryTableIndex index)
{
    const LigatureSubstitutionStateEntry *entry = &entryTable[index];
    ByteOffset newState = SWAPW(entry->newStateOffset);
    le_int16 flags = SWAPW(entry->flags);

    if (flags & lsfSetComponent) {
        if (++m >= nComponents) {
            m = 0;
        }

        componentStack[m] = currGlyph;
    }

    ByteOffset actionOffset = flags & lsfActionOffsetMask;

    if (actionOffset != 0) {
        const LigatureActionEntry *ap = (const LigatureActionEntry *) ((char *) &ligatureSubstitutionHeader->stHeader + actionOffset);
        LigatureActionEntry action;
        le_int32 offset, i = 0;
        le_int32 stack[nComponents];
        le_int16 mm = -1;

        do {
            le_uint32 componentGlyph = componentStack[m--];

            action = SWAPL(*ap++);

            if (m < 0) {
                m = nComponents - 1;
            }

            offset = action & lafComponentOffsetMask;
            if (offset != 0) {
                const le_int16 *offsetTable = (const le_int16 *)((char *) &ligatureSubstitutionHeader->stHeader + 2 * SignExtend(offset, lafComponentOffsetMask));

                i += SWAPW(offsetTable[LE_GET_GLYPH(glyphStorage[componentGlyph])]);

                if (action & (lafLast | lafStore))  {
                    const TTGlyphID *ligatureOffset = (const TTGlyphID *) ((char *) &ligatureSubstitutionHeader->stHeader + i);
                    TTGlyphID ligatureGlyph = SWAPW(*ligatureOffset);

                    glyphStorage[componentGlyph] = LE_SET_GLYPH(glyphStorage[componentGlyph], ligatureGlyph);
                    stack[++mm] = componentGlyph;
                    i = 0;
                } else {
                    glyphStorage[componentGlyph] = LE_SET_GLYPH(glyphStorage[componentGlyph], 0xFFFF);
                }
            }
        } while (!(action & lafLast));

        while (mm >= 0) {
            if (++m >= nComponents) {
                m = 0;
            }

            componentStack[m] = stack[mm--];
        }
    }

    if (!(flags & lsfDontAdvance)) {
        // should handle reverse too!
        currGlyph += 1;
    }

    return newState;
}

void LigatureSubstitutionProcessor::endStateTable()
{
}

U_NAMESPACE_END
