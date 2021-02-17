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

//
// Note that similar (or almost same) tests exist in Java side (See
// DatabaseGeneralTest.java in AndroidTests). The differences are:
// - this test is quite easy to do (You can do it in your Unix PC)
// - this test is not automatically executed by build servers
//
// You should also execute the test before submitting this.
//

#include "PhoneNumberUtils.h"

#include <stdio.h>
#include <string.h>

#include <gtest/gtest.h>

using namespace android;


TEST(PhoneNumberUtils, compareStrictNullOrEmpty) {
    EXPECT_TRUE(phone_number_compare_strict(NULL, NULL));
    EXPECT_TRUE(phone_number_compare_strict("", NULL));
    EXPECT_TRUE(phone_number_compare_strict(NULL, ""));
    EXPECT_TRUE(phone_number_compare_strict("", ""));
}

TEST(PhoneNumberUtils, compareStrictDigitsSame) {
    EXPECT_TRUE(phone_number_compare_strict("999", "999"));
    EXPECT_TRUE(phone_number_compare_strict("119", "119"));
}

TEST(PhoneNumberUtils, compareStrictDigitsDifferent) {
    EXPECT_FALSE(phone_number_compare_strict("123456789", "923456789"));
    EXPECT_FALSE(phone_number_compare_strict("123456789", "123456781"));
    EXPECT_FALSE(phone_number_compare_strict("123456789", "1234567890"));
    EXPECT_FALSE(phone_number_compare_strict("123456789", "0123456789"));
}

TEST(PhoneNumberUtils, compareStrictGoogle) {
    EXPECT_TRUE(phone_number_compare_strict("650-253-0000", "6502530000"));
    EXPECT_TRUE(phone_number_compare_strict("650-253-0000", "650 253 0000"));
    EXPECT_TRUE(phone_number_compare_strict("650 253 0000", "6502530000"));
}

TEST(PhoneNumberUtils, compareStrictTrunkPrefixUs) {
    // trunk (NDD) prefix must be properly handled in US
    EXPECT_TRUE(phone_number_compare_strict("650-253-0000", "1-650-253-0000"));
    EXPECT_TRUE(phone_number_compare_strict("650-253-0000", "   1-650-253-0000"));
    EXPECT_FALSE(phone_number_compare_strict("650-253-0000", "11-650-253-0000"));
    EXPECT_FALSE(phone_number_compare_strict("650-253-0000", "0-650-253-0000"));
    EXPECT_FALSE(phone_number_compare_strict("555-4141", "+1-700-555-4141"));

    EXPECT_TRUE(phone_number_compare_strict("+1 650-253-0000", "6502530000"));
    EXPECT_TRUE(phone_number_compare_strict("001 650-253-0000", "6502530000"));
    EXPECT_TRUE(phone_number_compare_strict("0111 650-253-0000", "6502530000"));
}

TEST(PhoneNumberUtils, compareStrictDifferentCountryCode) {
    EXPECT_FALSE(phone_number_compare_strict("+19012345678", "+819012345678"));
}

TEST(PhoneNumberUtils, compareStrictTrunkJapan) {
    EXPECT_TRUE(phone_number_compare_strict("+31771234567", "0771234567"));
    EXPECT_TRUE(phone_number_compare_strict("090-1234-5678", "+819012345678"));
    EXPECT_TRUE(phone_number_compare_strict("090(1234)5678", "+819012345678"));
    EXPECT_TRUE(phone_number_compare_strict("090-1234-5678", "+81-90-1234-5678"));

    EXPECT_TRUE(phone_number_compare_strict("+819012345678", "090-1234-5678"));
    EXPECT_TRUE(phone_number_compare_strict("+819012345678", "090(1234)5678"));
    EXPECT_TRUE(phone_number_compare_strict("+81-90-1234-5678", "090-1234-5678"));
}

TEST(PhoneNumberUtils, compareStrictTrunkRussia) {
    EXPECT_TRUE(phone_number_compare_strict("+79161234567", "89161234567"));
}

TEST(PhoneNumberUtils, compareStrictTrunkFrance) {
    EXPECT_TRUE(phone_number_compare_strict("+33123456789", "0123456789"));
}

TEST(PhoneNumberUtils, compareStrictTrunkNetherlandsCities) {
    EXPECT_TRUE(phone_number_compare_strict("+31771234567", "0771234567"));
}

// TODO: Two digit trunk prefixes are not handled
//TEST(PhoneNumberUtils, compareStrictTrunkHungary) {
//  EXPECT_TRUE(phone_number_compare_strict("+36 1 234 5678", "06 1234-5678"));
//}

// TODO: Two digit trunk prefixes are not handled
//TEST(PhoneNumberUtils, compareStrictTrunkMexico) {
//  EXPECT_TRUE(phone_number_compare_strict("+52 55 1234 5678", "01 55 1234 5678"));
//}

// TODO: Two digit trunk prefixes are not handled
//TEST(PhoneNumberUtils, compareStrictTrunkMongolia) {
//  EXPECT_TRUE(phone_number_compare_strict("+976 1 123 4567", "01 1 23 4567"));
//  EXPECT_TRUE(phone_number_compare_strict("+976 2 234 5678", "02 2 34 5678"));
//}

TEST(PhoneNumberUtils, compareStrictTrunkIgnoreJapan) {
    // Trunk prefix must not be ignored in Japan
    EXPECT_FALSE(phone_number_compare_strict("090-1234-5678", "90-1234-5678"));
    EXPECT_FALSE(phone_number_compare_strict("090-1234-5678", "080-1234-5678"));
    EXPECT_FALSE(phone_number_compare_strict("090-1234-5678", "190-1234-5678"));
    EXPECT_FALSE(phone_number_compare_strict("090-1234-5678", "890-1234-5678"));
    EXPECT_FALSE(phone_number_compare_strict("080-1234-5678", "+819012345678"));
    EXPECT_FALSE(phone_number_compare_strict("+81-90-1234-5678", "+81-090-1234-5678"));
}

TEST(PhoneNumberUtils, compareStrictInternationalNational) {
    EXPECT_TRUE(phone_number_compare_strict("+593(800)123-1234", "8001231234"));
}

TEST(PhoneNumberUtils, compareStrictTwoContinuousZeros) {
    // Two continuous 0 at the begining of the phone string should not be
    // treated as trunk prefix.
    EXPECT_FALSE(phone_number_compare_strict("008001231234", "8001231234"));
}

TEST(PhoneNumberUtils, compareStrictCallerIdThailandUs) {
    // Test broken caller ID seen on call from Thailand to the US
    EXPECT_TRUE(phone_number_compare_strict("+66811234567", "166811234567"));
    // Confirm that the bug found before does not re-appear.
    EXPECT_FALSE(phone_number_compare_strict("080-1234-5678", "+819012345678"));
    EXPECT_TRUE(phone_number_compare_strict("650-000-3456", "16500003456"));
    EXPECT_TRUE(phone_number_compare_strict("011 1 7005554141", "+17005554141"));
    EXPECT_FALSE(phone_number_compare_strict("011 11 7005554141", "+17005554141"));
    EXPECT_FALSE(phone_number_compare_strict("+44 207 792 3490", "00 207 792 3490"));
}

TEST(PhoneNumberUtils, compareStrictNamp1661) {
    // This is not related to Thailand case. NAMP "1" + region code "661".
    EXPECT_TRUE(phone_number_compare_strict("16610001234", "6610001234"));
}

TEST(PhoneNumberUtils, compareStrictAlphaDifferent) {
    // We also need to compare two alpha addresses to make sure two different strings
    // aren't treated as the same addresses. This is relevant to SMS as SMS sender may
    // contain all alpha chars.
    EXPECT_FALSE(phone_number_compare_strict("abcd", "bcde"));
}

TEST(PhoneNumberUtils, compareStrictAlphaNumericSame) {
    // in the U.S. people often use alpha in the phone number to easily remember it
    // (e.g. 800-flowers would be dialed as 800-356-9377). Since we accept this form of
    // phone number in Contacts and others, we should make sure the comparison method
    // handle them.
    EXPECT_TRUE(phone_number_compare_strict("1-800-flowers", "800-flowers"));
}

TEST(PhoneNumberUtils, compareStrictAlphaNumericDifferent) {
    EXPECT_FALSE(phone_number_compare_strict("1-800-flowers", "1-800-abcdefg"));
}

// TODO: we currently do not support this comparison.
// TODO: It maybe be nice to support this in the future.
//TEST(PhoneNumberUtils, compareStrictLettersToDigits) {
//  EXPECT_TRUE("1-800-flowers", "1-800-356-9377")
//}

TEST(PhoneNumberUtils, compareStrictWrongPrefix) {
    // Currently we cannot get this test through (Japanese trunk prefix is 0,
    // but there is no sensible way to know it now (as of 2009-6-12)...
    // EXPECT_FALSE(phone_number_compare_strict("290-1234-5678", "+819012345678"));
    // EXPECT_FALSE(phone_number_compare_strict("+819012345678", "290-1234-5678"));

    // USA
    EXPECT_FALSE(phone_number_compare_strict("550-450-3605", "+14504503605"));

    EXPECT_FALSE(phone_number_compare_strict("550-450-3605", "+15404503605"));
    EXPECT_FALSE(phone_number_compare_strict("550-450-3605", "+15514503605"));
    EXPECT_FALSE(phone_number_compare_strict("5504503605", "+14504503605"));

    EXPECT_FALSE(phone_number_compare_strict("+14504503605", "550-450-3605"));
    EXPECT_FALSE(phone_number_compare_strict("+15404503605", "550-450-3605"));
    EXPECT_FALSE(phone_number_compare_strict("+15514503605", "550-450-3605"));
    EXPECT_FALSE(phone_number_compare_strict("+14504503605", "5504503605"));
}

TEST(PhoneNumberUtils, compareStrict_phone_number_stripped_reversed_inter) {
    char out[6];
    int outlen;

#define ASSERT_STRIPPED_REVERSE(input, expected) \
    phone_number_stripped_reversed_inter((input), out, sizeof(out), &outlen); \
    out[outlen] = 0; \
    ASSERT_STREQ((expected), (out)); \

    ASSERT_STRIPPED_REVERSE("", "");

    ASSERT_STRIPPED_REVERSE("123", "321");
    ASSERT_STRIPPED_REVERSE("123*N#", "#N*321");

    // Buffer overflow
    ASSERT_STRIPPED_REVERSE("1234567890", "098765");

    // Only one plus is copied
    ASSERT_STRIPPED_REVERSE("1+2+", "+21");

    // Pause/wait in the phone number
    ASSERT_STRIPPED_REVERSE("12;34", "21");

    // Ignoring non-dialable
    ASSERT_STRIPPED_REVERSE("1A2 3?4", "4321");
}

TEST(PhoneNumberUtils, compareStrictRussianNumbers) {
    EXPECT_FALSE(phone_number_compare_strict("84951234567", "+84951234567"));

    EXPECT_FALSE(phone_number_compare_strict("88001234567", "+88001234567"));
}
