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

#include "PhoneticStringUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utils/String8.h>

using namespace android;

class TestExecutor {
 public:
  TestExecutor() : m_total_count(0), m_success_count(0), m_success(true) {}
  bool DoAllTests();
 private:
  void DoOneTest(void (TestExecutor::*test)());

  void testUtf32At();
  void testGetUtf8FromUtf32();
  void testGetNormalizedString();
  void testLongString();

  // Note: When adding a test, do not forget to add it to DoOneTest().

  int m_total_count;
  int m_success_count;

  bool m_success;
};

#define ASSERT_EQ_VALUE(input, expected)                                \
  ({                                                                    \
    if ((expected) != (input)) {                                        \
      printf("0x%X(result) != 0x%X(expected)\n", input, expected);      \
      m_success = false;                                                \
      return;                                                           \
    }                                                                   \
  })

#define EXPECT_EQ_VALUE(input, expected)                                \
  ({                                                                    \
    if ((expected) != (input)) {                                        \
      printf("0x%X(result) != 0x%X(expected)\n", input, expected);      \
      m_success = false;                                                \
    }                                                                   \
  })


bool TestExecutor::DoAllTests() {
  DoOneTest(&TestExecutor::testUtf32At);
  DoOneTest(&TestExecutor::testGetUtf8FromUtf32);
  DoOneTest(&TestExecutor::testGetNormalizedString);
  DoOneTest(&TestExecutor::testLongString);

  printf("Test total: %d\nSuccess: %d\nFailure: %d\n",
         m_total_count, m_success_count, m_total_count - m_success_count);

  bool success = m_total_count == m_success_count;
  printf("\n%s\n", success ? "Success" : "Failure");

  return success;
}

void TestExecutor::DoOneTest(void (TestExecutor::*test)()) {
  m_success = true;

  (this->*test)();

  ++m_total_count;
  m_success_count += m_success ? 1 : 0;
}

#define TEST_GET_UTF32AT(src, index, expected_next, expected_value)     \
  ({                                                                    \
    size_t next;                                                        \
    int32_t ret = utf32_at(src, strlen(src), index, &next);   \
    if (ret < 0) {                                                      \
      printf("getUtf32At() returned negative value (src: %s, index: %d)\n", \
             (src), (index));                                           \
      m_success = false;                                                \
    } else if (next != (expected_next)) {                               \
      printf("next is unexpected value (src: %s, actual: %u, expected: %u)\n", \
             (src), next, (expected_next));                             \
    } else {                                                            \
      EXPECT_EQ_VALUE(ret, (expected_value));                           \
    }                                                                   \
   })

void TestExecutor::testUtf32At() {
  printf("testUtf32At()\n");

  TEST_GET_UTF32AT("a", 0, 1, 97);
  // Japanese hiragana "a"
  TEST_GET_UTF32AT("\xE3\x81\x82", 0, 3, 0x3042);
  // Japanese fullwidth katakana "a" with ascii a
  TEST_GET_UTF32AT("a\xE3\x82\xA2", 1, 4, 0x30A2);

  // 2 PUA
  TEST_GET_UTF32AT("\xF3\xBE\x80\x80\xF3\xBE\x80\x88", 0, 4, 0xFE000);
  TEST_GET_UTF32AT("\xF3\xBE\x80\x80\xF3\xBE\x80\x88", 4, 8, 0xFE008);
}


#define EXPECT_EQ_CODEPOINT_UTF8(codepoint, expected)                   \
  ({                                                                    \
    char32_t codepoints[1] = {codepoint};                                \
    status_t ret = string8.setTo(codepoints, 1);                        \
    if (ret != NO_ERROR) {                                              \
      printf("GetUtf8FromCodePoint() returned false at 0x%04X\n", codepoint); \
      m_success = false;                                                \
    } else {                                                            \
      const char* string = string8.string();                            \
      if (strcmp(string, expected) != 0) {                              \
        printf("Failed at codepoint 0x%04X\n", codepoint);              \
        for (const char *ch = string; *ch != '\0'; ++ch) {              \
          printf("0x%X ", *ch);                                         \
        }                                                               \
        printf("!= ");                                                  \
        for (const char *ch = expected; *ch != '\0'; ++ch) {            \
          printf("0x%X ", *ch);                                         \
        }                                                               \
        printf("\n");                                                   \
        m_success = false;                                              \
      }                                                                 \
    }                                                                   \
  })

void TestExecutor::testGetUtf8FromUtf32() {
  printf("testGetUtf8FromUtf32()\n");
  String8 string8;

  EXPECT_EQ_CODEPOINT_UTF8('a', "\x61");
  // Armenian capital letter AYB (2 bytes in UTF8)
  EXPECT_EQ_CODEPOINT_UTF8(0x0530, "\xD4\xB0");
  // Japanese 'a' (3 bytes in UTF8)
  EXPECT_EQ_CODEPOINT_UTF8(0x3042, "\xE3\x81\x82");
  // Kanji
  EXPECT_EQ_CODEPOINT_UTF8(0x65E5, "\xE6\x97\xA5");
  // PUA (4 byets in UTF8)
  EXPECT_EQ_CODEPOINT_UTF8(0xFE016, "\xF3\xBE\x80\x96");
  EXPECT_EQ_CODEPOINT_UTF8(0xFE972, "\xF3\xBE\xA5\xB2");
}

#define EXPECT_EQ_UTF8_UTF8(src, expected)                              \
  ({                                                                    \
    if (!GetNormalizedString(src, &dst, &len)) {                        \
      printf("GetNormalizedSortableString() returned false.\n");      \
      m_success = false;                                                \
    } else {                                                            \
      if (strcmp(dst, expected) != 0) {                                 \
        for (const char *ch = dst; *ch != '\0'; ++ch) {                 \
          printf("0x%X ", *ch);                                         \
        }                                                               \
        printf("!= ");                                                  \
        for (const char *ch = expected; *ch != '\0'; ++ch) {            \
          printf("0x%X ", *ch);                                         \
        }                                                               \
        printf("\n");                                                   \
        m_success = false;                                              \
      }                                                                 \
      free(dst);                                                        \
    }                                                                   \
   })

void TestExecutor::testGetNormalizedString() {
  printf("testGetNormalizedString()\n");
  char *dst;
  size_t len;

  // halfwidth alphabets/symbols -> keep it as is.
  EXPECT_EQ_UTF8_UTF8("ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%^&'()",
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%^&'()");
  EXPECT_EQ_UTF8_UTF8("abcdefghijklmnopqrstuvwxyz[]{}\\@/",
                      "abcdefghijklmnopqrstuvwxyz[]{}\\@/");

  // halfwidth/fullwidth-katakana -> hiragana
  EXPECT_EQ_UTF8_UTF8(
      "\xE3\x81\x82\xE3\x82\xA4\xE3\x81\x86\xEF\xBD\xB4\xE3\x82\xAA",
      "\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A");

  // whitespace -> keep it as is.
  EXPECT_EQ_UTF8_UTF8("    \t", "    \t");
}

void TestExecutor::testLongString() {
  printf("testLongString()\n");
  char * dst;
  size_t len;
  EXPECT_EQ_UTF8_UTF8("Qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqtttttttttttttttttttttttttttttttttttttttttttttttttgggggggggggggggggggggggggggggggggggggggbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
      "Qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqtttttttttttttttttttttttttttttttttttttttttttttttttggggggggggggggggggggggggggggggggggg");
}


int main() {
  TestExecutor executor;
  if(executor.DoAllTests()) {
    return 0;
  } else {
    return 1;
  }
}
