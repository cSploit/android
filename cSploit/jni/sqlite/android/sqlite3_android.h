/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef SQLITE3_ANDROID_H
#define SQLITE3_ANDROID_H

#include <sqlite3.h>

#ifdef __cplusplus
extern "C" {
#endif

int register_android_functions(sqlite3 * handle, int uit16Storage);

int register_localized_collators(sqlite3* handle, const char* systemLocale, int utf16Storage);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
