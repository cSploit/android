/* //device/libs/android_runtime/PhoneNumberUtils.h
**
** Copyright 2006, The Android Open Source Project
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

#ifndef _ANDROID_PHONE_NUMBER_UTILS_H
#define _ANDROID_PHONE_NUMBER_UTILS_H

namespace android {

bool phone_number_compare_loose(const char* a, const char* b);
bool phone_number_compare_loose_with_minmatch(const char* a, const char* b, int min_match);
bool phone_number_compare_strict(const char* a, const char* b);
bool phone_number_stripped_reversed_inter(const char* in, char* out, const int len, int *outlen);

}  // namespace android

#endif
