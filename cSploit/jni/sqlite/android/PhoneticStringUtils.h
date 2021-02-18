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

#ifndef _ANDROID_PHONETIC_STRING_UTILS_H
#define _ANDROID_PHONETIC_STRING_UTILS_H

#include <string.h>  // For size_t.
#include <utils/String8.h>

namespace android {

// Returns codepoint which is "normalized", whose definition depends on each
// Locale. Note that currently this function normalizes only Japanese; the
// other characters are remained as is.
// The variable "next_is_consumed" is set to true if "next_codepoint"
// is "consumed" (e.g. Japanese halfwidth katakana's voiced mark is consumed
// when previous "codepoint" is appropriate, like half-width "ka").
//
// In Japanese, "normalized" means that half-width and full-width katakana is
// appropriately converted to hiragana.
int GetNormalizedCodePoint(char32_t codepoint,
                           char32_t next_codepoint,
                           bool *next_is_consumed);

// Pushes Utf8 expression of "codepoint" to "dst". Returns true when successful.
// If input is invalid or the length of the destination is not enough,
// returns false.
bool GetUtf8FromCodePoint(int codepoint, char *dst, size_t len, size_t *index);

// Creates a "phonetically sortable" Utf8 string and push it into "dst".
// *dst must be freed after being used outside.
// If "src" is NULL or its length is 0, "dst" is set to \uFFFF.
//
// Note that currently this function considers only Japanese.
bool GetPhoneticallySortableString(const char *src, char **dst, size_t *len);

// Creates a "normalized" Utf8 string and push it into "dst". *dst must be
// freed after being used outside.
// If "src" is NULL or its length is 0, "dst" is set to \uFFFF.
//
// Note that currently this function considers only Japanese.
bool GetNormalizedString(const char *src, char **dst, size_t *len);

}  // namespace android

#endif
