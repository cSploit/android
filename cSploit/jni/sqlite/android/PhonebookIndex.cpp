/*
 * Copyright 2010, The Android Open Source Project
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

#include <ctype.h>
#include <string.h>

#include <unicode/ucol.h>
#include <unicode/uiter.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

#include "PhonebookIndex.h"
#include "PhoneticStringUtils.h"

#define MIN_OUTPUT_SIZE 6       // Minimum required size for the output buffer (in bytes)

namespace android {

// IMPORTANT!  Keep the codes below SORTED. We are doing a binary search on the array
static UChar DEFAULT_CHAR_MAP[] = {
    0x00C6,    'A',       // AE
    0x00DF,    'S',       // Etzett
    0x1100, 0x3131,       // HANGUL LETTER KIYEOK
    0x1101, 0x3132,       // HANGUL LETTER SSANGKIYEOK
    0x1102, 0x3134,       // HANGUL LETTER NIEUN
    0x1103, 0x3137,       // HANGUL LETTER TIKEUT
    0x1104, 0x3138,       // HANGUL LETTER SSANGTIKEUT
    0x1105, 0x3139,       // HANGUL LETTER RIEUL
    0x1106, 0x3141,       // HANGUL LETTER MIEUM
    0x1107, 0x3142,       // HANGUL LETTER PIEUP
    0x1108, 0x3143,       // HANGUL LETTER SSANGPIEUP
    0x1109, 0x3145,       // HANGUL LETTER SIOS
    0x110A, 0x3146,       // HANGUL LETTER SSANGSIOS
    0x110B, 0x3147,       // HANGUL LETTER IEUNG
    0x110C, 0x3148,       // HANGUL LETTER CIEUC
    0x110D, 0x3149,       // HANGUL LETTER SSANGCIEUC
    0x110E, 0x314A,       // HANGUL LETTER CHIEUCH
    0x110F, 0x314B,       // HANGUL LETTER KHIEUKH
    0x1110, 0x314C,       // HANGUL LETTER THIEUTH
    0x1111, 0x314D,       // HANGUL LETTER PHIEUPH
    0x1112, 0x314E,       // HANGUL LETTER HIEUH
    0x111A, 0x3140,       // HANGUL LETTER RIEUL-HIEUH
    0x1121, 0x3144,       // HANGUL LETTER PIEUP-SIOS
    0x1161, 0x314F,       // HANGUL LETTER A
    0x1162, 0x3150,       // HANGUL LETTER AE
    0x1163, 0x3151,       // HANGUL LETTER YA
    0x1164, 0x3152,       // HANGUL LETTER YAE
    0x1165, 0x3153,       // HANGUL LETTER EO
    0x1166, 0x3154,       // HANGUL LETTER E
    0x1167, 0x3155,       // HANGUL LETTER YEO
    0x1168, 0x3156,       // HANGUL LETTER YE
    0x1169, 0x3157,       // HANGUL LETTER O
    0x116A, 0x3158,       // HANGUL LETTER WA
    0x116B, 0x3159,       // HANGUL LETTER WAE
    0x116C, 0x315A,       // HANGUL LETTER OE
    0x116D, 0x315B,       // HANGUL LETTER YO
    0x116E, 0x315C,       // HANGUL LETTER U
    0x116F, 0x315D,       // HANGUL LETTER WEO
    0x1170, 0x315E,       // HANGUL LETTER WE
    0x1171, 0x315F,       // HANGUL LETTER WI
    0x1172, 0x3160,       // HANGUL LETTER YU
    0x1173, 0x3161,       // HANGUL LETTER EU
    0x1174, 0x3162,       // HANGUL LETTER YI
    0x1175, 0x3163,       // HANGUL LETTER I
    0x11AA, 0x3133,       // HANGUL LETTER KIYEOK-SIOS
    0x11AC, 0x3135,       // HANGUL LETTER NIEUN-CIEUC
    0x11AD, 0x3136,       // HANGUL LETTER NIEUN-HIEUH
    0x11B0, 0x313A,       // HANGUL LETTER RIEUL-KIYEOK
    0x11B1, 0x313B,       // HANGUL LETTER RIEUL-MIEUM
    0x11B3, 0x313D,       // HANGUL LETTER RIEUL-SIOS
    0x11B4, 0x313E,       // HANGUL LETTER RIEUL-THIEUTH
    0x11B5, 0x313F,       // HANGUL LETTER RIEUL-PHIEUPH
};

/**
 * Binary search to map an individual character to the corresponding phone book index.
 */
static UChar map_character(UChar c, UChar * char_map, int32_t length) {
  int from = 0, to = length;
  while (from < to) {
    int m = ((to + from) >> 1) & ~0x1;    // Only consider even positions
    UChar cm = char_map[m];
    if (cm == c) {
      return char_map[m + 1];
    } else if (cm < c) {
      from = m + 2;
    } else {
      to = m;
    }
  }
  return 0;
}

/**
 * Returns TRUE if the character belongs to a Hanzi unicode block
 */
static bool is_CJK(UChar c) {
  return
       (0x4e00 <= c && c <= 0x9fff)     // CJK_UNIFIED_IDEOGRAPHS
    || (0x3400 <= c && c <= 0x4dbf)     // CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A
    || (0x3000 <= c && c <= 0x303f)     // CJK_SYMBOLS_AND_PUNCTUATION
    || (0x2e80 <= c && c <= 0x2eff)     // CJK_RADICALS_SUPPLEMENT
    || (0x3300 <= c && c <= 0x33ff)     // CJK_COMPATIBILITY
    || (0xfe30 <= c && c <= 0xfe4f)     // CJK_COMPATIBILITY_FORMS
    || (0xf900 <= c && c <= 0xfaff);    // CJK_COMPATIBILITY_IDEOGRAPHS
}

int32_t GetPhonebookIndex(UCharIterator * iter, const char * locale, UChar * out, int32_t size,
        UBool * isError)
{
  if (size < MIN_OUTPUT_SIZE) {
    *isError = TRUE;
    return 0;
  }

  *isError = FALSE;

  // Normalize the first character to remove accents using the NFD normalization
  UErrorCode errorCode = U_ZERO_ERROR;
  int32_t len = unorm_next(iter, out, size, UNORM_NFD,
          0 /* options */, TRUE /* normalize */, NULL, &errorCode);
  if (U_FAILURE(errorCode)) {
    *isError = TRUE;
    return 0;
  }

  if (len == 0) {   // Empty input string
    return 0;
  }

  UChar c = out[0];

  // We are only interested in letters
  if (!u_isalpha(c)) {
    return 0;
  }

  c = u_toupper(c);

  // Check for explicitly mapped characters
  UChar c_mapped = map_character(c, DEFAULT_CHAR_MAP, sizeof(DEFAULT_CHAR_MAP) / sizeof(UChar));
  if (c_mapped != 0) {
    out[0] = c_mapped;
    return 1;
  }

  // Convert Kanas to Hiragana
  UChar next = len > 2 ? out[1] : 0;
  c = android::GetNormalizedCodePoint(c, next, NULL);

  // Traditional grouping of Hiragana characters
  if (0x3042 <= c && c <= 0x309F) {
    if (c < 0x304B) c = 0x3042;         // a
    else if (c < 0x3055) c = 0x304B;    // ka
    else if (c < 0x305F) c = 0x3055;    // sa
    else if (c < 0x306A) c = 0x305F;    // ta
    else if (c < 0x306F) c = 0x306A;    // na
    else if (c < 0x307E) c = 0x306F;    // ha
    else if (c < 0x3084) c = 0x307E;    // ma
    else if (c < 0x3089) c = 0x3084;    // ya
    else if (c < 0x308F) c = 0x3089;    // ra
    else c = 0x308F;                    // wa
    out[0] = c;
    return 1;
  }

  if (is_CJK(c)) {
    if (strncmp(locale, "ja", 2) == 0) {
      // Japanese word meaning "misc" or "other"
      out[0] = 0x4ED6;
      return 1;
    } else {
      return 0;
    }
  }

  out[0] = c;
  return 1;
}

}  // namespace android
