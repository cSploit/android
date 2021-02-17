/*
 * (C) Copyright IBM Corp. 1998-2008 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "OpenTypeUtilities.h"
#include "ScriptAndLanguage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

const LangSysTable *ScriptTable::findLanguage(LETag languageTag, le_bool exactMatch) const
{
    le_uint16 count = SWAPW(langSysCount);
    Offset langSysTableOffset = exactMatch? 0 : SWAPW(defaultLangSysTableOffset);

    if (count > 0) {
        Offset foundOffset =
            OpenTypeUtilities::getTagOffset(languageTag, langSysRecordArray, count);

        if (foundOffset != 0) {
            langSysTableOffset = foundOffset;
        }
    }

    if (langSysTableOffset != 0) {
        return (const LangSysTable *) ((char *)this + langSysTableOffset);
    }

    return NULL;
}

const ScriptTable *ScriptListTable::findScript(LETag scriptTag) const
{
    /*
     * There are some fonts that have a large, bogus value for scriptCount. To try
     * and protect against this, we use the offset in the first scriptRecord,
     * which we know has to be past the end of the scriptRecordArray, to compute
     * a value which is greater than or equal to the actual script count.
     *
     * Note: normally, the first offset will point to just after the scriptRecordArray,
     * but there's no guarantee of this, only that it's *after* the scriptRecordArray.
     * Because of this, a binary serach isn't safe, because the new count may include
     * data that's not actually in the scriptRecordArray and hence the array will appear
     * to be unsorted.
     */
    le_uint16 count = SWAPW(scriptCount);
    le_uint16 limit = ((SWAPW(scriptRecordArray[0].offset) - sizeof(ScriptListTable)) / sizeof(scriptRecordArray)) + ANY_NUMBER;
    Offset scriptTableOffset = 0;

    if (count > limit) {
        // the scriptCount value is bogus; do a linear search
        // because limit may still be too large.
        for(le_int32 s = 0; s < limit; s += 1) {
            if (SWAPT(scriptRecordArray[s].tag) == scriptTag) {
                scriptTableOffset = SWAPW(scriptRecordArray[s].offset);
                break;
            }
        }
    } else {
        scriptTableOffset = OpenTypeUtilities::getTagOffset(scriptTag, scriptRecordArray, count);
    }

    if (scriptTableOffset != 0) {
        return (const ScriptTable *) ((char *)this + scriptTableOffset);
    }

    return NULL;
}

const LangSysTable *ScriptListTable::findLanguage(LETag scriptTag, LETag languageTag, le_bool exactMatch) const
{
    const ScriptTable *scriptTable = findScript(scriptTag);

    if (scriptTable == 0) {
        return NULL;
    }

    return scriptTable->findLanguage(languageTag, exactMatch);
}

U_NAMESPACE_END
