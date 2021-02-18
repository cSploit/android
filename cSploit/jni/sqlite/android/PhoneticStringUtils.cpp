/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>

#include "PhoneticStringUtils.h"
#include <utils/String8.h>

// We'd like 0 length string last of sorted list. So when input string is NULL
// or 0 length string, we use these instead.
#define CODEPOINT_FOR_NULL_STR 0xFFFD
#define STR_FOR_NULL_STR "\xEF\xBF\xBD"

// We assume that users will not notice strings not sorted properly when the
// first 128 characters are the same.
#define MAX_CODEPOINTS 128

namespace android {

// Get hiragana from halfwidth katakana.
static int GetHiraganaFromHalfwidthKatakana(char32_t codepoint,
                                            char32_t next_codepoint,
                                            bool *next_is_consumed) {
    if (codepoint < 0xFF66 || 0xFF9F < codepoint) {
        return codepoint;
    }

    switch (codepoint) {
        case 0xFF66: // wo
            return 0x3092;
        case 0xFF67: // xa
            return 0x3041;
        case 0xFF68: // xi
            return 0x3043;
        case 0xFF69: // xu
            return 0x3045;
        case 0xFF6A: // xe
            return 0x3047;
        case 0xFF6B: // xo
            return 0x3049;
        case 0xFF6C: // xya
            return 0x3083;
        case 0xFF6D: // xyu
            return 0x3085;
        case 0xFF6E: // xyo
            return 0x3087;
        case 0xFF6F: // xtsu
            return 0x3063;
        case 0xFF70: // -
            return 0x30FC;
        case 0xFF9C: // wa
            return 0x308F;
        case 0xFF9D: // n
            return 0x3093;
            break;
        default:   {
            if (0xFF71 <= codepoint && codepoint <= 0xFF75) {
                // a, i, u, e, o
                if (codepoint == 0xFF73 && next_codepoint == 0xFF9E) {
                    if (next_is_consumed != NULL) {
                        *next_is_consumed = true;
                    }
                    return 0x3094; // vu
                } else {
                    return 0x3042 + (codepoint - 0xFF71) * 2;
                }
            } else if (0xFF76 <= codepoint && codepoint <= 0xFF81) {
                // ka - chi
                if (next_codepoint == 0xFF9E) {
                    // "dakuten" (voiced mark)
                    if (next_is_consumed != NULL) {
                        *next_is_consumed = true;
                    }
                    return 0x304B + (codepoint - 0xFF76) * 2 + 1;
                } else {
                    return 0x304B + (codepoint - 0xFF76) * 2;
                }
            } else if (0xFF82 <= codepoint && codepoint <= 0xFF84) {
                // tsu, te, to (skip xtsu)
                if (next_codepoint == 0xFF9E) {
                    // "dakuten" (voiced mark)
                    if (next_is_consumed != NULL) {
                        *next_is_consumed = true;
                    }
                    return 0x3064 + (codepoint - 0xFF82) * 2 + 1;
                } else {
                    return 0x3064 + (codepoint - 0xFF82) * 2;
                }
            } else if (0xFF85 <= codepoint && codepoint <= 0xFF89) {
                // na, ni, nu, ne, no
                return 0x306A + (codepoint - 0xFF85);
            } else if (0xFF8A <= codepoint && codepoint <= 0xFF8E) {
                // ha, hi, hu, he, ho
                if (next_codepoint == 0xFF9E) {
                    // "dakuten" (voiced mark)
                    if (next_is_consumed != NULL) {
                        *next_is_consumed = true;
                    }
                    return 0x306F + (codepoint - 0xFF8A) * 3 + 1;
                } else if (next_codepoint == 0xFF9F) {
                    // "han-dakuten" (half voiced mark)
                    if (next_is_consumed != NULL) {
                        *next_is_consumed = true;
                    }
                    return 0x306F + (codepoint - 0xFF8A) * 3 + 2;
                } else {
                    return 0x306F + (codepoint - 0xFF8A) * 3;
                }
            } else if (0xFF8F <= codepoint && codepoint <= 0xFF93) {
                // ma, mi, mu, me, mo
                return 0x307E + (codepoint - 0xFF8F);
            } else if (0xFF94 <= codepoint && codepoint <= 0xFF96) {
                // ya, yu, yo
                return 0x3084 + (codepoint - 0xFF94) * 2;
            } else if (0xFF97 <= codepoint && codepoint <= 0xFF9B) {
                // ra, ri, ru, re, ro
                return 0x3089 + (codepoint - 0xFF97);
            }
            // Note: 0xFF9C, 0xFF9D are handled above
        } // end of default
    }

    return codepoint;
}

// Assuming input is hiragana, convert the hiragana to "normalized" hiragana.
static int GetNormalizedHiragana(int codepoint) {
    if (codepoint < 0x3040 || 0x309F < codepoint) {
        return codepoint;
    }

    // TODO: should care (semi-)voiced mark (0x3099, 0x309A).

    // Trivial kana conversions.
    // e.g. xa => a
    switch (codepoint) {
        case 0x3041:
        case 0x3043:
        case 0x3045:
        case 0x3047:
        case 0x3049:
        case 0x308E: // xwa
            return codepoint + 1;
        case 0x3095: // xka
            return 0x304B;
        case 0x3096: // xku
            return 0x304F;
        default:
            return codepoint;
    }
}

static int GetNormalizedKana(char32_t codepoint,
                             char32_t next_codepoint,
                             bool *next_is_consumed) {
    // First, convert fullwidth katakana and halfwidth katakana to hiragana.
    if (0x30A1 <= codepoint && codepoint <= 0x30F6) {
        // Make fullwidth katakana same as hiragana.
        // 96 == 0x30A1 - 0x3041c
        codepoint = codepoint - 96;
    } else {
        codepoint = GetHiraganaFromHalfwidthKatakana(
                codepoint, next_codepoint, next_is_consumed);
    }

    // Normalize Hiragana.
    return GetNormalizedHiragana(codepoint);
}

int GetNormalizedCodePoint(char32_t codepoint,
                           char32_t next_codepoint,
                           bool *next_is_consumed) {
    if (next_is_consumed != NULL) {
        *next_is_consumed = false;
    }

    if (codepoint <= 0x0020 || codepoint == 0x3000) {
        // Whitespaces. Keep it as is.
        return codepoint;
    } else if ((0x0021 <= codepoint && codepoint <= 0x007E) ||
               (0xFF01 <= codepoint && codepoint <= 0xFF5E)) {
        // Ascii and fullwidth ascii. Keep it as is
        return codepoint;
    } else if (codepoint == 0x02DC || codepoint == 0x223C) {
        // tilde
        return 0xFF5E;
    } else if (codepoint <= 0x3040 ||
               (0x3100 <= codepoint && codepoint < 0xFF00) ||
               codepoint == CODEPOINT_FOR_NULL_STR) {
        // Keep it as is.
        return codepoint;
    }

    // Below is Kana-related handling.

    return GetNormalizedKana(codepoint, next_codepoint, next_is_consumed);
}

static bool GetExpectedString(
    const char *src, char **dst, size_t *dst_len,
    int (*get_codepoint_function)(char32_t, char32_t, bool*)) {
    if (dst == NULL || dst_len == NULL) {
        return false;
    }

    if (src == NULL || *src == '\0') {
        src = STR_FOR_NULL_STR;
    }

    char32_t codepoints[MAX_CODEPOINTS]; // if array size is changed the for loop needs to be changed

    size_t src_len = utf8_length(src);
    if (src_len == 0) {
        return false;
    }
    bool next_is_consumed;
    size_t j = 0;
    for (size_t i = 0; i < src_len && j < MAX_CODEPOINTS;) {
        int32_t ret = utf32_at(src, src_len, i, &i);
        if (ret < 0) {
            // failed to parse UTF-8
            return false;
        }
        ret = get_codepoint_function(
                static_cast<char32_t>(ret),
                i + 1 < src_len ? src[i + 1] : 0,
                &next_is_consumed);
        if (ret > 0) {
            codepoints[j] = static_cast<char32_t>(ret);
            j++;
        }
        if (next_is_consumed) {
            i++;
        }
    }
    size_t length = j;

    if (length == 0) {
        // If all of codepoints are invalid, we place the string at the end of
        // the list.
        codepoints[0] = 0x10000 + CODEPOINT_FOR_NULL_STR;
        length = 1;
    }

    size_t new_len = utf8_length_from_utf32(codepoints, length);
    *dst = static_cast<char *>(malloc(new_len + 1));
    if (*dst == NULL) {
        return false;
    }

    if (utf32_to_utf8(codepoints, length, *dst, new_len + 1) != new_len) {
        free(*dst);
        *dst = NULL;
        return false;
    }

    *dst_len = new_len;
    return true;
}

bool GetNormalizedString(const char *src, char **dst, size_t *len) {
    return GetExpectedString(src, dst, len, GetNormalizedCodePoint);
}

}  // namespace android
