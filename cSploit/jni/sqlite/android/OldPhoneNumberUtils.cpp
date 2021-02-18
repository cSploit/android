/*
 *
 * Copyright 2006, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Old implementation for phone_number_compare(), which has used in cupcake, but once replaced with
// the new, more strict version, and reverted again.

#include <string.h>

namespace android {

static int MIN_MATCH = 7;

/** True if c is ISO-LATIN characters 0-9 */
static bool isISODigit (char c)
{
    return c >= '0' && c <= '9';
}

/** True if c is ISO-LATIN characters 0-9, *, # , +  */
static bool isNonSeparator(char c)
{
    return (c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+';
}

/**
 * Phone numbers are stored in "lookup" form in the database
 * as reversed strings to allow for caller ID lookup
 *
 * This method takes a phone number and makes a valid SQL "LIKE"
 * string that will match the lookup form
 *
 */
/** all of a up to len must be an international prefix or
 *  separators/non-dialing digits
 */
static bool matchIntlPrefix(const char* a, int len)
{
    /* '([^0-9*#+]\+[^0-9*#+] | [^0-9*#+]0(0|11)[^0-9*#+] )$' */
    /*        0    1                     2 3 45               */

    int state = 0;
    for (int i = 0 ; i < len ; i++) {
        char c = a[i];

        switch (state) {
            case 0:
                if      (c == '+') state = 1;
                else if (c == '0') state = 2;
                else if (isNonSeparator(c)) return false;
            break;

            case 2:
                if      (c == '0') state = 3;
                else if (c == '1') state = 4;
                else if (isNonSeparator(c)) return false;
            break;

            case 4:
                if      (c == '1') state = 5;
                else if (isNonSeparator(c)) return false;
            break;

            default:
                if (isNonSeparator(c)) return false;
            break;

        }
    }

    return state == 1 || state == 3 || state == 5;
}

/** all of 'a' up to len must match non-US trunk prefix ('0') */
static bool matchTrunkPrefix(const char* a, int len)
{
    bool found;

    found = false;

    for (int i = 0 ; i < len ; i++) {
        char c = a[i];

        if (c == '0' && !found) {
            found = true;
        } else if (isNonSeparator(c)) {
            return false;
        }
    }

    return found;
}

/** all of 'a' up to len must be a (+|00|011)country code)
 *  We're fast and loose with the country code. Any \d{1,3} matches */
static bool matchIntlPrefixAndCC(const char* a, int len)
{
    /*  [^0-9*#+]*(\+|0(0|11)\d\d?\d? [^0-9*#+] $ */
    /*      0       1 2 3 45  6 7  8              */

    int state = 0;
    for (int i = 0 ; i < len ; i++ ) {
        char c = a[i];

        switch (state) {
            case 0:
                if      (c == '+') state = 1;
                else if (c == '0') state = 2;
                else if (isNonSeparator(c)) return false;
            break;

            case 2:
                if      (c == '0') state = 3;
                else if (c == '1') state = 4;
                else if (isNonSeparator(c)) return false;
            break;

            case 4:
                if      (c == '1') state = 5;
                else if (isNonSeparator(c)) return false;
            break;

            case 1:
            case 3:
            case 5:
                if      (isISODigit(c)) state = 6;
                else if (isNonSeparator(c)) return false;
            break;

            case 6:
            case 7:
                if      (isISODigit(c)) state++;
                else if (isNonSeparator(c)) return false;
            break;

            default:
                if (isNonSeparator(c)) return false;
        }
    }

    return state == 6 || state == 7 || state == 8;
}

/** or -1 if both are negative */
static int minPositive(int a, int b)
{
    if (a >= 0 && b >= 0) {
        return (a < b) ? a : b;
    } else if (a >= 0) { /* && b < 0 */
        return a;
    } else if (b >= 0) { /* && a < 0 */
        return b;
    } else { /* a < 0 && b < 0 */
        return -1;
    }
}

/**
 * Return the offset into a of the first appearance of b, or -1 if there
 * is no such character in a.
 */
static int indexOf(const char *a, char b) {
    const char *ix = strchr(a, b);

    if (ix == NULL)
        return -1;
    else
        return ix - a;
}

/**
 * Compare phone numbers a and b, return true if they're identical
 * enough for caller ID purposes.
 *
 * - Compares from right to left
 * - requires MIN_MATCH (7) characters to match
 * - handles common trunk prefixes and international prefixes
 *   (basically, everything except the Russian trunk prefix)
 *
 * Tolerates nulls
 */
bool phone_number_compare_loose(const char* a, const char* b)
{
    int ia, ib;
    int matched;
    int numSeparatorCharsInA = 0;
    int numSeparatorCharsInB = 0;

    if (a == NULL || b == NULL) {
        return false;
    }

    ia = strlen(a);
    ib = strlen(b);
    if (ia == 0 || ib == 0) {
        return false;
    }

    // Compare from right to left
    ia--;
    ib--;

    matched = 0;

    while (ia >= 0 && ib >=0) {
        char ca, cb;
        bool skipCmp = false;

        ca = a[ia];

        if (!isNonSeparator(ca)) {
            ia--;
            skipCmp = true;
            numSeparatorCharsInA++;
        }

        cb = b[ib];

        if (!isNonSeparator(cb)) {
            ib--;
            skipCmp = true;
            numSeparatorCharsInB++;
        }

        if (!skipCmp) {
            if (cb != ca) {
                break;
            }
            ia--; ib--; matched++;
        }
    }

    if (matched < MIN_MATCH) {
        const int effectiveALen = strlen(a) - numSeparatorCharsInA;
        const int effectiveBLen = strlen(b) - numSeparatorCharsInB;

        // if the number of dialable chars in a and b match, but the matched chars < MIN_MATCH,
        // treat them as equal (i.e. 404-04 and 40404)
        if (effectiveALen == effectiveBLen && effectiveALen == matched) {
            return true;
        }

        return false;
    }

    // At least one string has matched completely;
    if (matched >= MIN_MATCH && (ia < 0 || ib < 0)) {
        return true;
    }

    /*
     * Now, what remains must be one of the following for a
     * match:
     *
     *  - a '+' on one and a '00' or a '011' on the other
     *  - a '0' on one and a (+,00)<country code> on the other
     *     (for this, a '0' and a '00' prefix would have succeeded above)
     */

    if (matchIntlPrefix(a, ia + 1) && matchIntlPrefix(b, ib +1)) {
        return true;
    }

    if (matchTrunkPrefix(a, ia + 1) && matchIntlPrefixAndCC(b, ib +1)) {
        return true;
    }

    if (matchTrunkPrefix(b, ib + 1) && matchIntlPrefixAndCC(a, ia +1)) {
        return true;
    }

    /*
     * Last resort: if the number of unmatched characters on both sides is less than or equal
     * to the length of the longest country code and only one number starts with a + accept
     * the match. This is because some countries like France and Russia have an extra prefix
     * digit that is used when dialing locally in country that does not show up when you dial
     * the number using the country code. In France this prefix digit is used to determine
     * which land line carrier to route the call over.
     */
    bool aPlusFirst = (*a == '+');
    bool bPlusFirst = (*b == '+');
    if (ia < 4 && ib < 4 && (aPlusFirst || bPlusFirst) && !(aPlusFirst && bPlusFirst)) {
        return true;
    }

    return false;
}

}  // namespace android
