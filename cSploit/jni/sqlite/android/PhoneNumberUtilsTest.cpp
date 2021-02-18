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
/*
 * Note that similar (or almost same) tests exist in Java side (See
 * DatabaseGeneralTest.java in AndroidTests). The differences are:
 * - this test is quite easy to do (You can do it in your Unix PC)
 * - this test is not automatically executed by build servers
 *
 * You should also execute the test before submitting this.
 */

#include "PhoneNumberUtils.h"

#include <stdio.h>
#include <string.h>

using namespace android;

#define EXPECT(function, input1, input2, expected, total, error)        \
    ({                                                                  \
        const char *i1_cache = input1;                                  \
        const char *i2_cache = input2;                                  \
        (total)++;                                                      \
        if ((expected) != (function)((i1_cache), (i2_cache))) {         \
            if (expected) {                                             \
                printf("%s != %s while we expect %s == %s\n",           \
                       (i1_cache), (i2_cache), (i1_cache), (i2_cache)); \
            } else {                                                    \
                printf("%s == %s while we expect %s != %s\n",           \
                       (i1_cache), (i2_cache), (i1_cache), (i2_cache)); \
            }                                                           \
            (error)++;                                                  \
        }                                                               \
    })

#define EXPECT_EQ(input1, input2)                                       \
    EXPECT(phone_number_compare_strict, (input1), (input2), true,              \
           (total), (error))


#define EXPECT_NE(input1, input2)                                       \
    EXPECT(phone_number_compare_strict, (input1), (input2), false,             \
           (total), (error))

int main() {
    int total = 0;
    int error = 0;

    EXPECT_EQ(NULL, NULL);
    EXPECT_EQ("", NULL);
    EXPECT_EQ(NULL, "");
    EXPECT_EQ("", "");

    EXPECT_EQ("999", "999");
    EXPECT_EQ("119", "119");

    EXPECT_NE("123456789", "923456789");
    EXPECT_NE("123456789", "123456781");
    EXPECT_NE("123456789", "1234567890");
    EXPECT_NE("123456789", "0123456789");

    // Google, Inc.
    EXPECT_EQ("650-253-0000", "6502530000");
    EXPECT_EQ("650-253-0000", "650 253 0000");
    EXPECT_EQ("650 253 0000", "6502530000");

    // trunk (NDD) prefix must be properly handled in US
    EXPECT_EQ("650-253-0000", "1-650-253-0000");
    EXPECT_EQ("650-253-0000", "   1-650-253-0000");
    EXPECT_NE("650-253-0000", "11-650-253-0000");
    EXPECT_NE("650-253-0000", "0-650-253-0000");
    EXPECT_NE("555-4141", "+1-700-555-4141");

    EXPECT_EQ("+1 650-253-0000", "6502530000");
    EXPECT_EQ("001 650-253-0000", "6502530000");
    EXPECT_EQ("0111 650-253-0000", "6502530000");

    // Country code is different.
    EXPECT_NE("+19012345678", "+819012345678");

    // Russian trunk digit
    EXPECT_EQ("+79161234567", "89161234567");

    // French trunk digit
    EXPECT_EQ("+33123456789", "0123456789");

    // Trunk digit for city codes in the Netherlands
    EXPECT_EQ("+31771234567", "0771234567");

    // Japanese dial
    EXPECT_EQ("090-1234-5678", "+819012345678");
    EXPECT_EQ("090(1234)5678", "+819012345678");
    EXPECT_EQ("090-1234-5678", "+81-90-1234-5678");

    // Trunk prefix must not be ignored in Japan
    EXPECT_NE("090-1234-5678", "90-1234-5678");

    EXPECT_NE("090-1234-5678", "080-1234-5678");
    EXPECT_NE("090-1234-5678", "190-1234-5678");
    EXPECT_NE("090-1234-5678", "890-1234-5678");
    EXPECT_NE("+81-90-1234-5678", "+81-090-1234-5678");

    EXPECT_EQ("+593(800)123-1234", "8001231234");

    // Two continuous 0 at the beginieng of the phone string should not be
    // treated as trunk prefix.
    EXPECT_NE("008001231234", "8001231234");

    // Test broken caller ID seen on call from Thailand to the US
    EXPECT_EQ("+66811234567", "166811234567");

    // Confirm that the bug found before does not re-appear.
    EXPECT_NE("080-1234-5678", "+819012345678");
    EXPECT_EQ("650-000-3456", "16500003456");
    EXPECT_EQ("011 1 7005554141", "+17005554141");
    EXPECT_NE("011 11 7005554141", "+17005554141");
    EXPECT_NE("+44 207 792 3490", "00 207 792 3490");
    // This is not related to Thailand case. NAMP "1" + region code "661".
    EXPECT_EQ("16610001234", "6610001234");

    // We also need to compare two alpha addresses to make sure two different strings
    // aren't treated as the same addresses. This is relevant to SMS as SMS sender may
    // contain all alpha chars.
    EXPECT_NE("abcd", "bcde");

    // in the U.S. people often use alpha in the phone number to easily remember it
    // (e.g. 800-flowers would be dialed as 800-356-9377). Since we accept this form of
    // phone number in Contacts and others, we should make sure the comparison method
    // handle them.
    EXPECT_EQ("1-800-flowers", "800-flowers");

    // TODO: we currently do not support this comparison. It maybe nice to support this
    // TODO: in the future.
    // EXPECT_EQ("1-800-flowers", "1-800-356-9377")

    EXPECT_NE("1-800-flowers", "1-800-abcdefg");

    // Currently we cannot get this test through (Japanese trunk prefix is 0,
    // but there is no sensible way to know it now (as of 2009-6-12)...
    // EXPECT_NE("290-1234-5678", "+819012345678");

    printf("total: %d, error: %d\n\n", total, error);
    if (error == 0) {
        printf("Success!\n");
    } else {
        printf("Failure... :(\n");
    }
}
