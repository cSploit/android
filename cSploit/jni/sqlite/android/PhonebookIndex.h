/*
**
** Copyright 2010, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _ANDROID_PHONEBOOK_INDEX_H
#define _ANDROID_PHONEBOOK_INDEX_H

#include <unicode/uiter.h>
#include <unicode/utypes.h>

namespace android {

/**
 * A character converter that takes a UNICODE character and produces the
 * phone book index for it in the specified locale. For example, "a" becomes "A"
 * and so does A with accents. Conversion rules differ from locale
 * locale, which is why this function takes locale as an argument.
 *
 * @param iter iterator if input characters
 * @param locale the string representation of the current locale, e.g. "ja"
 * @param out output buffer
 * @param size size of the output buffer in bytes. The buffer should be large enough
 *        to hold the longest phone book index (e.g. a three-char word in Japan).
 * @param isError will be set to TRUE if an error occurs
 *
 * @return number of characters returned
 */
int32_t GetPhonebookIndex(UCharIterator * iter, const char * locale, UChar * out, int32_t size,
        UBool * isError);

}  // namespace android

#endif
