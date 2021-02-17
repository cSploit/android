/*
*******************************************************************************
*   Copyright (C) 2004-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*
*   file name:  udraft.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Created by: genheaders.pl, a perl script written by Ram Viswanadha
*
*  Contains data for commenting out APIs.
*  Gets included by umachine.h
*
*  THIS FILE IS MACHINE-GENERATED, DON'T PLAY WITH IT IF YOU DON'T KNOW WHAT
*  YOU ARE DOING, OTHERWISE VERY BAD THINGS WILL HAPPEN!
*/

#ifndef UDRAFT_H
#define UDRAFT_H

#ifdef U_HIDE_DRAFT_API

#    if U_DISABLE_RENAMING
#        define bamuScriptCode bamuScriptCode_DRAFT_API_DO_NOT_USE
#        define u_fadopt u_fadopt_DRAFT_API_DO_NOT_USE
#        define u_strFromJavaModifiedUTF8WithSub u_strFromJavaModifiedUTF8WithSub_DRAFT_API_DO_NOT_USE
#        define u_strToJavaModifiedUTF8 u_strToJavaModifiedUTF8_DRAFT_API_DO_NOT_USE
#        define ucal_getDayOfWeekType ucal_getDayOfWeekType_DRAFT_API_DO_NOT_USE
#        define ucal_getWeekendTransition ucal_getWeekendTransition_DRAFT_API_DO_NOT_USE
#        define ucal_isWeekend ucal_isWeekend_DRAFT_API_DO_NOT_USE
#        define udat_toCalendarDateField udat_toCalendarDateField_DRAFT_API_DO_NOT_USE
#        define udatpg_getBestPatternWithOptions udatpg_getBestPatternWithOptions_DRAFT_API_DO_NOT_USE
#        define udatpg_replaceFieldTypesWithOptions udatpg_replaceFieldTypesWithOptions_DRAFT_API_DO_NOT_USE
#        define uldn_close uldn_close_DRAFT_API_DO_NOT_USE
#        define uldn_getDialectHandling uldn_getDialectHandling_DRAFT_API_DO_NOT_USE
#        define uldn_getLocale uldn_getLocale_DRAFT_API_DO_NOT_USE
#        define uldn_keyDisplayName uldn_keyDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_keyValueDisplayName uldn_keyValueDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_languageDisplayName uldn_languageDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_localeDisplayName uldn_localeDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_open uldn_open_DRAFT_API_DO_NOT_USE
#        define uldn_regionDisplayName uldn_regionDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_scriptCodeDisplayName uldn_scriptCodeDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_scriptDisplayName uldn_scriptDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_variantDisplayName uldn_variantDisplayName_DRAFT_API_DO_NOT_USE
#        define uloc_forLanguageTag uloc_forLanguageTag_DRAFT_API_DO_NOT_USE
#        define uloc_toLanguageTag uloc_toLanguageTag_DRAFT_API_DO_NOT_USE
#        define unorm2_append unorm2_append_DRAFT_API_DO_NOT_USE
#        define unorm2_close unorm2_close_DRAFT_API_DO_NOT_USE
#        define unorm2_getInstance unorm2_getInstance_DRAFT_API_DO_NOT_USE
#        define unorm2_hasBoundaryAfter unorm2_hasBoundaryAfter_DRAFT_API_DO_NOT_USE
#        define unorm2_hasBoundaryBefore unorm2_hasBoundaryBefore_DRAFT_API_DO_NOT_USE
#        define unorm2_isInert unorm2_isInert_DRAFT_API_DO_NOT_USE
#        define unorm2_isNormalized unorm2_isNormalized_DRAFT_API_DO_NOT_USE
#        define unorm2_normalize unorm2_normalize_DRAFT_API_DO_NOT_USE
#        define unorm2_normalizeSecondAndAppend unorm2_normalizeSecondAndAppend_DRAFT_API_DO_NOT_USE
#        define unorm2_openFiltered unorm2_openFiltered_DRAFT_API_DO_NOT_USE
#        define unorm2_quickCheck unorm2_quickCheck_DRAFT_API_DO_NOT_USE
#        define unorm2_spanQuickCheckYes unorm2_spanQuickCheckYes_DRAFT_API_DO_NOT_USE
#        define unum_formatDecimal unum_formatDecimal_DRAFT_API_DO_NOT_USE
#        define unum_parseDecimal unum_parseDecimal_DRAFT_API_DO_NOT_USE
#    else
#        define bamuScriptCode_4_4 bamuScriptCode_DRAFT_API_DO_NOT_USE
#        define u_fadopt_4_4 u_fadopt_DRAFT_API_DO_NOT_USE
#        define u_strFromJavaModifiedUTF8WithSub_4_4 u_strFromJavaModifiedUTF8WithSub_DRAFT_API_DO_NOT_USE
#        define u_strToJavaModifiedUTF8_4_4 u_strToJavaModifiedUTF8_DRAFT_API_DO_NOT_USE
#        define ucal_getDayOfWeekType_4_4 ucal_getDayOfWeekType_DRAFT_API_DO_NOT_USE
#        define ucal_getWeekendTransition_4_4 ucal_getWeekendTransition_DRAFT_API_DO_NOT_USE
#        define ucal_isWeekend_4_4 ucal_isWeekend_DRAFT_API_DO_NOT_USE
#        define udat_toCalendarDateField_4_4 udat_toCalendarDateField_DRAFT_API_DO_NOT_USE
#        define udatpg_getBestPatternWithOptions_4_4 udatpg_getBestPatternWithOptions_DRAFT_API_DO_NOT_USE
#        define udatpg_replaceFieldTypesWithOptions_4_4 udatpg_replaceFieldTypesWithOptions_DRAFT_API_DO_NOT_USE
#        define uldn_close_4_4 uldn_close_DRAFT_API_DO_NOT_USE
#        define uldn_getDialectHandling_4_4 uldn_getDialectHandling_DRAFT_API_DO_NOT_USE
#        define uldn_getLocale_4_4 uldn_getLocale_DRAFT_API_DO_NOT_USE
#        define uldn_keyDisplayName_4_4 uldn_keyDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_keyValueDisplayName_4_4 uldn_keyValueDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_languageDisplayName_4_4 uldn_languageDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_localeDisplayName_4_4 uldn_localeDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_open_4_4 uldn_open_DRAFT_API_DO_NOT_USE
#        define uldn_regionDisplayName_4_4 uldn_regionDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_scriptCodeDisplayName_4_4 uldn_scriptCodeDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_scriptDisplayName_4_4 uldn_scriptDisplayName_DRAFT_API_DO_NOT_USE
#        define uldn_variantDisplayName_4_4 uldn_variantDisplayName_DRAFT_API_DO_NOT_USE
#        define uloc_forLanguageTag_4_4 uloc_forLanguageTag_DRAFT_API_DO_NOT_USE
#        define uloc_toLanguageTag_4_4 uloc_toLanguageTag_DRAFT_API_DO_NOT_USE
#        define unorm2_append_4_4 unorm2_append_DRAFT_API_DO_NOT_USE
#        define unorm2_close_4_4 unorm2_close_DRAFT_API_DO_NOT_USE
#        define unorm2_getInstance_4_4 unorm2_getInstance_DRAFT_API_DO_NOT_USE
#        define unorm2_hasBoundaryAfter_4_4 unorm2_hasBoundaryAfter_DRAFT_API_DO_NOT_USE
#        define unorm2_hasBoundaryBefore_4_4 unorm2_hasBoundaryBefore_DRAFT_API_DO_NOT_USE
#        define unorm2_isInert_4_4 unorm2_isInert_DRAFT_API_DO_NOT_USE
#        define unorm2_isNormalized_4_4 unorm2_isNormalized_DRAFT_API_DO_NOT_USE
#        define unorm2_normalizeSecondAndAppend_4_4 unorm2_normalizeSecondAndAppend_DRAFT_API_DO_NOT_USE
#        define unorm2_normalize_4_4 unorm2_normalize_DRAFT_API_DO_NOT_USE
#        define unorm2_openFiltered_4_4 unorm2_openFiltered_DRAFT_API_DO_NOT_USE
#        define unorm2_quickCheck_4_4 unorm2_quickCheck_DRAFT_API_DO_NOT_USE
#        define unorm2_spanQuickCheckYes_4_4 unorm2_spanQuickCheckYes_DRAFT_API_DO_NOT_USE
#        define unum_formatDecimal_4_4 unum_formatDecimal_DRAFT_API_DO_NOT_USE
#        define unum_parseDecimal_4_4 unum_parseDecimal_DRAFT_API_DO_NOT_USE
#    endif /* U_DISABLE_RENAMING */

#endif /* U_HIDE_DRAFT_API */
#endif /* UDRAFT_H */

