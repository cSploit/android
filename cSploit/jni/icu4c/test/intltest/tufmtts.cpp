/********************************************************************
 * Copyright (c) 2008-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/tmunit.h"
#include "unicode/tmutamt.h"
#include "unicode/tmutfmt.h"
#include "tufmtts.h"


//TODO: put as compilation flag
//#define TUFMTTS_DEBUG 1

#ifdef TUFMTTS_DEBUG
#include <iostream>
#endif

void TimeUnitTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ ) {
    if (exec) logln("TestSuite TimeUnitTest");
    switch (index) {
        TESTCASE(0, testBasic);
        TESTCASE(1, testAPI);
        default: name = ""; break;
    }
}

/**
 * Test basic
 */
void TimeUnitTest::testBasic() {
    const char* locales[] = {"en", "sl", "fr", "zh", "ar", "ru", "zh_Hant"};
    for ( unsigned int locIndex = 0; 
          locIndex < sizeof(locales)/sizeof(locales[0]); 
          ++locIndex ) {
        UErrorCode status = U_ZERO_ERROR;
        Locale loc(locales[locIndex]);
        TimeUnitFormat** formats = new TimeUnitFormat*[2];
        formats[TimeUnitFormat::kFull] = new TimeUnitFormat(loc, status);
        if (!assertSuccess("TimeUnitFormat(full)", status, TRUE)) return;
        formats[TimeUnitFormat::kAbbreviate] = new TimeUnitFormat(loc, TimeUnitFormat::kAbbreviate, status);
        if (!assertSuccess("TimeUnitFormat(short)", status)) return;
#ifdef TUFMTTS_DEBUG
        std::cout << "locale: " << locales[locIndex] << "\n";
#endif
        for (int style = TimeUnitFormat::kFull; 
             style <= TimeUnitFormat::kAbbreviate;
             ++style) {
          for (TimeUnit::UTimeUnitFields j = TimeUnit::UTIMEUNIT_YEAR; 
             j < TimeUnit::UTIMEUNIT_FIELD_COUNT; 
             j = (TimeUnit::UTimeUnitFields)(j+1)) {
#ifdef TUFMTTS_DEBUG
            std::cout << "time unit: " << j << "\n";
#endif
            double tests[] = {0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 5, 10, 100, 101.35};
            for (unsigned int i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
#ifdef TUFMTTS_DEBUG
                std::cout << "number: " << tests[i] << "\n";
#endif
                TimeUnitAmount* source = new TimeUnitAmount(tests[i], j, status);
                if (!assertSuccess("TimeUnitAmount()", status)) return;
                UnicodeString formatted;
                Formattable formattable;
                formattable.adoptObject(source);
                formatted = ((Format*)formats[style])->format(formattable, formatted, status);
                if (!assertSuccess("format()", status)) return;
#ifdef TUFMTTS_DEBUG
                char formatResult[1000];
                formatted.extract(0, formatted.length(), formatResult, "UTF-8");
                std::cout << "format result: " << formatResult << "\n";
#endif
                Formattable result;
                ((Format*)formats[style])->parseObject(formatted, result, status);
                if (!assertSuccess("parseObject()", status)) return;
                if (result != formattable) {
                    dataerrln("No round trip: ");
                }
                // other style parsing
                Formattable result_1;
                ((Format*)formats[1-style])->parseObject(formatted, result_1, status);
                if (!assertSuccess("parseObject()", status)) return;
                if (result_1 != formattable) {
                    dataerrln("No round trip: ");
                }
            }
          }
        }
        delete formats[TimeUnitFormat::kFull];
        delete formats[TimeUnitFormat::kAbbreviate];
        delete[] formats;
    }
}


void TimeUnitTest::testAPI() {
    //================= TimeUnit =================
    UErrorCode status = U_ZERO_ERROR;

    TimeUnit* tmunit = TimeUnit::createInstance(TimeUnit::UTIMEUNIT_YEAR, status);
    if (!assertSuccess("TimeUnit::createInstance", status)) return;

    TimeUnit* another = (TimeUnit*)tmunit->clone();
    TimeUnit third(*tmunit);
    TimeUnit fourth = third;

    assertTrue("orig and clone are equal", (*tmunit == *another));
    assertTrue("copied and assigned are equal", (third == fourth));

    TimeUnit* tmunit_m = TimeUnit::createInstance(TimeUnit::UTIMEUNIT_MONTH, status);
    assertTrue("year != month", (*tmunit != *tmunit_m));

    TimeUnit::UTimeUnitFields field = tmunit_m->getTimeUnitField();
    assertTrue("field of month time unit is month", (field == TimeUnit::UTIMEUNIT_MONTH));
    
    delete tmunit;
    delete another;
    delete tmunit_m;
    //
    //================= TimeUnitAmount =================

    Formattable formattable((int32_t)2);
    TimeUnitAmount tma_long(formattable, TimeUnit::UTIMEUNIT_DAY, status);
    if (!assertSuccess("TimeUnitAmount(formattable...)", status)) return;

    formattable.setDouble(2);
    TimeUnitAmount tma_double(formattable, TimeUnit::UTIMEUNIT_DAY, status);
    if (!assertSuccess("TimeUnitAmount(formattable...)", status)) return;

    formattable.setDouble(3);
    TimeUnitAmount tma_double_3(formattable, TimeUnit::UTIMEUNIT_DAY, status);
    if (!assertSuccess("TimeUnitAmount(formattable...)", status)) return;

    TimeUnitAmount tma(2, TimeUnit::UTIMEUNIT_DAY, status);
    if (!assertSuccess("TimeUnitAmount(number...)", status)) return;

    TimeUnitAmount tma_h(2, TimeUnit::UTIMEUNIT_HOUR, status);
    if (!assertSuccess("TimeUnitAmount(number...)", status)) return;

    TimeUnitAmount second(tma);
    TimeUnitAmount third_tma = tma;
    TimeUnitAmount* fourth_tma = (TimeUnitAmount*)tma.clone();

    assertTrue("orig and copy are equal", (second == tma));
    assertTrue("clone and assigned are equal", (third_tma == *fourth_tma));
    assertTrue("different if number diff", (tma_double != tma_double_3));
    assertTrue("different if number type diff", (tma_double != tma_long));
    assertTrue("different if time unit diff", (tma != tma_h));
    assertTrue("same even different constructor", (tma_double == tma));

    assertTrue("getTimeUnitField", (tma.getTimeUnitField() == TimeUnit::UTIMEUNIT_DAY));
    delete fourth_tma;
    //
    //================= TimeUnitFormat =================
    //
    TimeUnitFormat* tmf_en = new TimeUnitFormat(Locale("en"), status);
    if (!assertSuccess("TimeUnitFormat(en...)", status, TRUE)) return;
    TimeUnitFormat tmf_fr(Locale("fr"), status);
    if (!assertSuccess("TimeUnitFormat(fr...)", status)) return;

    assertTrue("TimeUnitFormat: en and fr diff", (*tmf_en != tmf_fr));

    TimeUnitFormat tmf_assign = *tmf_en;
    assertTrue("TimeUnitFormat: orig and assign are equal", (*tmf_en == tmf_assign));

    TimeUnitFormat tmf_copy(tmf_fr);
    assertTrue("TimeUnitFormat: orig and copy are equal", (tmf_fr == tmf_copy));

    TimeUnitFormat* tmf_clone = (TimeUnitFormat*)tmf_en->clone();
    assertTrue("TimeUnitFormat: orig and clone are equal", (*tmf_en == *tmf_clone));
    delete tmf_clone;

    tmf_en->setLocale(Locale("fr"), status);
    if (!assertSuccess("setLocale(fr...)", status)) return;

    NumberFormat* numberFmt = NumberFormat::createInstance(
                                 Locale("fr"), status);
    if (!assertSuccess("NumberFormat::createInstance()", status)) return;
    tmf_en->setNumberFormat(*numberFmt, status);
    if (!assertSuccess("setNumberFormat(en...)", status)) return;
    assertTrue("TimeUnitFormat: setLocale", (*tmf_en == tmf_fr));

    delete tmf_en;

    TimeUnitFormat* en_long = new TimeUnitFormat(Locale("en"), TimeUnitFormat::kFull, status);
    if (!assertSuccess("TimeUnitFormat(en...)", status)) return;
    delete en_long;

    TimeUnitFormat* en_short = new TimeUnitFormat(Locale("en"), TimeUnitFormat::kAbbreviate, status);
    if (!assertSuccess("TimeUnitFormat(en...)", status)) return;
    delete en_short;

    TimeUnitFormat* format = new TimeUnitFormat(status);
    format->setLocale(Locale("zh"), status);
    format->setNumberFormat(*numberFmt, status);
    if (!assertSuccess("TimeUnitFormat(en...)", status)) return;
    delete numberFmt;
    delete format;
}


#endif
