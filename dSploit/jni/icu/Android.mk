# THANKS: http://thebugfreeblog.blogspot.it/2013/05/cross-building-icu-for-applications-on.html

LOCAL_PATH := $(call my-dir)

#original path: androidbuild/stubdata/libicudata.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= 

LOCAL_C_INCLUDES:= \
	icu/source/common
LOCAL_SRC_FILES:= \
	source/stubdata/stubdata.c
LOCAL_MODULE := libicudata

include $(BUILD_STATIC_LIBRARY)


#original path: androidbuild/lib/libicuuc.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= \
-DU_COMMON_IMPLEMENTATION 
LOCAL_CPPFLAGS:= -frtti -fexceptions 

LOCAL_C_INCLUDES:= \
	icu/source/common
LOCAL_SRC_FILES:= \
	source/common/errorcode.cpp\
	source/common/putil.cpp\
	source/common/umath.c\
	source/common/utypes.c\
	source/common/uinvchar.c\
	source/common/umutex.cpp\
	source/common/ucln_cmn.c\
	source/common/uinit.cpp\
	source/common/uobject.cpp\
	source/common/cmemory.c\
	source/common/charstr.cpp\
	source/common/udata.cpp\
	source/common/ucmndata.c\
	source/common/udatamem.c\
	source/common/umapfile.c\
	source/common/udataswp.c\
	source/common/ucol_swp.cpp\
	source/common/utrace.c\
	source/common/uhash.c\
	source/common/uhash_us.cpp\
	source/common/uenum.c\
	source/common/ustrenum.cpp\
	source/common/uvector.cpp\
	source/common/ustack.cpp\
	source/common/uvectr32.cpp\
	source/common/uvectr64.cpp\
	source/common/ucnv.c\
	source/common/ucnv_bld.cpp\
	source/common/ucnv_cnv.c\
	source/common/ucnv_io.cpp\
	source/common/ucnv_cb.c\
	source/common/ucnv_err.c\
	source/common/ucnvlat1.c\
	source/common/ucnv_u7.c\
	source/common/ucnv_u8.c\
	source/common/ucnv_u16.c\
	source/common/ucnv_u32.c\
	source/common/ucnvscsu.c\
	source/common/ucnvbocu.cpp\
	source/common/ucnv_ext.cpp\
	source/common/ucnvmbcs.c\
	source/common/ucnv2022.cpp\
	source/common/ucnvhz.c\
	source/common/ucnv_lmb.c\
	source/common/ucnvisci.c\
	source/common/ucnvdisp.c\
	source/common/ucnv_set.c\
	source/common/ucnv_ct.c\
	source/common/uresbund.cpp\
	source/common/ures_cnv.c\
	source/common/uresdata.c\
	source/common/resbund.cpp\
	source/common/resbund_cnv.cpp\
	source/common/messagepattern.cpp\
	source/common/ucat.c\
	source/common/locmap.c\
	source/common/uloc.cpp\
	source/common/locid.cpp\
	source/common/locutil.cpp\
	source/common/locavailable.cpp\
	source/common/locdispnames.cpp\
	source/common/loclikely.cpp\
	source/common/locresdata.cpp\
	source/common/bytestream.cpp\
	source/common/stringpiece.cpp\
	source/common/stringtriebuilder.cpp\
	source/common/bytestriebuilder.cpp\
	source/common/bytestrie.cpp\
	source/common/bytestrieiterator.cpp\
	source/common/ucharstrie.cpp\
	source/common/ucharstriebuilder.cpp\
	source/common/ucharstrieiterator.cpp\
	source/common/dictionarydata.cpp\
	source/common/appendable.cpp\
	source/common/ustr_cnv.c\
	source/common/unistr_cnv.cpp\
	source/common/unistr.cpp\
	source/common/unistr_case.cpp\
	source/common/unistr_props.cpp\
	source/common/utf_impl.c\
	source/common/ustring.cpp\
	source/common/ustrcase.cpp\
	source/common/ucasemap.cpp\
	source/common/ucasemap_titlecase_brkiter.cpp\
	source/common/cstring.c\
	source/common/ustrfmt.c\
	source/common/ustrtrns.cpp\
	source/common/ustr_wcs.cpp\
	source/common/utext.cpp\
	source/common/unistr_case_locale.cpp\
	source/common/ustrcase_locale.cpp\
	source/common/unistr_titlecase_brkiter.cpp\
	source/common/ustr_titlecase_brkiter.cpp\
	source/common/normalizer2impl.cpp\
	source/common/normalizer2.cpp\
	source/common/filterednormalizer2.cpp\
	source/common/normlzr.cpp\
	source/common/unorm.cpp\
	source/common/unormcmp.cpp\
	source/common/unorm_it.c\
	source/common/chariter.cpp\
	source/common/schriter.cpp\
	source/common/uchriter.cpp\
	source/common/uiter.cpp\
	source/common/patternprops.cpp\
	source/common/uchar.c\
	source/common/uprops.cpp\
	source/common/ucase.cpp\
	source/common/propname.cpp\
	source/common/ubidi_props.c\
	source/common/ubidi.c\
	source/common/ubidiwrt.c\
	source/common/ubidiln.c\
	source/common/ushape.cpp\
	source/common/uscript.c\
	source/common/uscript_props.cpp\
	source/common/usc_impl.c\
	source/common/unames.cpp\
	source/common/utrie.cpp\
	source/common/utrie2.cpp\
	source/common/utrie2_builder.cpp\
	source/common/bmpset.cpp\
	source/common/unisetspan.cpp\
	source/common/uset_props.cpp\
	source/common/uniset_props.cpp\
	source/common/uniset_closure.cpp\
	source/common/uset.cpp\
	source/common/uniset.cpp\
	source/common/usetiter.cpp\
	source/common/ruleiter.cpp\
	source/common/caniter.cpp\
	source/common/unifilt.cpp\
	source/common/unifunct.cpp\
	source/common/uarrsort.c\
	source/common/brkiter.cpp\
	source/common/ubrk.cpp\
	source/common/brkeng.cpp\
	source/common/dictbe.cpp\
	source/common/rbbi.cpp\
	source/common/rbbidata.cpp\
	source/common/rbbinode.cpp\
	source/common/rbbirb.cpp\
	source/common/rbbiscan.cpp\
	source/common/rbbisetb.cpp\
	source/common/rbbistbl.cpp\
	source/common/rbbitblb.cpp\
	source/common/serv.cpp\
	source/common/servnotf.cpp\
	source/common/servls.cpp\
	source/common/servlk.cpp\
	source/common/servlkf.cpp\
	source/common/servrbf.cpp\
	source/common/servslkf.cpp\
	source/common/uidna.cpp\
	source/common/usprep.cpp\
	source/common/uts46.cpp\
	source/common/punycode.cpp\
	source/common/util.cpp\
	source/common/util_props.cpp\
	source/common/parsepos.cpp\
	source/common/locbased.cpp\
	source/common/cwchar.c\
	source/common/wintz.c\
	source/common/dtintrv.cpp\
	source/common/ucnvsel.cpp\
	source/common/propsvec.c\
	source/common/ulist.c\
	source/common/uloc_tag.c\
	source/common/icudataver.c\
	source/common/icuplug.c\
	source/common/listformatter.cpp
LOCAL_MODULE := libicuuc

include $(BUILD_STATIC_LIBRARY)


#original path: androidbuild/lib/libicui18n.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= \
-DU_I18N_IMPLEMENTATION 
LOCAL_CPPFLAGS:= -frtti -fexceptions 

LOCAL_C_INCLUDES:= \
	icu/source/i18n\
	icu/source/common
LOCAL_SRC_FILES:= \
	source/i18n/ucln_in.c\
	source/i18n/fmtable.cpp\
	source/i18n/format.cpp\
	source/i18n/msgfmt.cpp\
	source/i18n/umsg.cpp\
	source/i18n/numfmt.cpp\
	source/i18n/unum.cpp\
	source/i18n/decimfmt.cpp\
	source/i18n/dcfmtsym.cpp\
	source/i18n/ucurr.cpp\
	source/i18n/digitlst.cpp\
	source/i18n/fmtable_cnv.cpp\
	source/i18n/choicfmt.cpp\
	source/i18n/datefmt.cpp\
	source/i18n/smpdtfmt.cpp\
	source/i18n/reldtfmt.cpp\
	source/i18n/dtfmtsym.cpp\
	source/i18n/udat.cpp\
	source/i18n/dtptngen.cpp\
	source/i18n/udatpg.cpp\
	source/i18n/nfrs.cpp\
	source/i18n/nfrule.cpp\
	source/i18n/nfsubs.cpp\
	source/i18n/rbnf.cpp\
	source/i18n/numsys.cpp\
	source/i18n/unumsys.cpp\
	source/i18n/ucsdet.cpp\
	source/i18n/ucal.cpp\
	source/i18n/calendar.cpp\
	source/i18n/gregocal.cpp\
	source/i18n/timezone.cpp\
	source/i18n/simpletz.cpp\
	source/i18n/olsontz.cpp\
	source/i18n/astro.cpp\
	source/i18n/taiwncal.cpp\
	source/i18n/buddhcal.cpp\
	source/i18n/persncal.cpp\
	source/i18n/islamcal.cpp\
	source/i18n/japancal.cpp\
	source/i18n/gregoimp.cpp\
	source/i18n/hebrwcal.cpp\
	source/i18n/indiancal.cpp\
	source/i18n/chnsecal.cpp\
	source/i18n/cecal.cpp\
	source/i18n/coptccal.cpp\
	source/i18n/dangical.cpp\
	source/i18n/ethpccal.cpp\
	source/i18n/coleitr.cpp\
	source/i18n/coll.cpp\
	source/i18n/tblcoll.cpp\
	source/i18n/sortkey.cpp\
	source/i18n/bocsu.cpp\
	source/i18n/ucoleitr.cpp\
	source/i18n/ucol.cpp\
	source/i18n/ucol_res.cpp\
	source/i18n/ucol_bld.cpp\
	source/i18n/ucol_sit.cpp\
	source/i18n/ucol_tok.cpp\
	source/i18n/ucol_wgt.cpp\
	source/i18n/ucol_cnt.cpp\
	source/i18n/ucol_elm.cpp\
	source/i18n/strmatch.cpp\
	source/i18n/usearch.cpp\
	source/i18n/search.cpp\
	source/i18n/stsearch.cpp\
	source/i18n/translit.cpp\
	source/i18n/utrans.cpp\
	source/i18n/esctrn.cpp\
	source/i18n/unesctrn.cpp\
	source/i18n/funcrepl.cpp\
	source/i18n/strrepl.cpp\
	source/i18n/tridpars.cpp\
	source/i18n/cpdtrans.cpp\
	source/i18n/rbt.cpp\
	source/i18n/rbt_data.cpp\
	source/i18n/rbt_pars.cpp\
	source/i18n/rbt_rule.cpp\
	source/i18n/rbt_set.cpp\
	source/i18n/nultrans.cpp\
	source/i18n/remtrans.cpp\
	source/i18n/casetrn.cpp\
	source/i18n/titletrn.cpp\
	source/i18n/tolowtrn.cpp\
	source/i18n/toupptrn.cpp\
	source/i18n/anytrans.cpp\
	source/i18n/name2uni.cpp\
	source/i18n/uni2name.cpp\
	source/i18n/nortrans.cpp\
	source/i18n/quant.cpp\
	source/i18n/transreg.cpp\
	source/i18n/brktrans.cpp\
	source/i18n/regexcmp.cpp\
	source/i18n/rematch.cpp\
	source/i18n/repattrn.cpp\
	source/i18n/regexst.cpp\
	source/i18n/regextxt.cpp\
	source/i18n/regeximp.cpp\
	source/i18n/uregex.cpp\
	source/i18n/uregexc.cpp\
	source/i18n/ulocdata.c\
	source/i18n/measfmt.cpp\
	source/i18n/currfmt.cpp\
	source/i18n/curramt.cpp\
	source/i18n/currunit.cpp\
	source/i18n/measure.cpp\
	source/i18n/utmscale.c\
	source/i18n/csdetect.cpp\
	source/i18n/csmatch.cpp\
	source/i18n/csr2022.cpp\
	source/i18n/csrecog.cpp\
	source/i18n/csrmbcs.cpp\
	source/i18n/csrsbcs.cpp\
	source/i18n/csrucode.cpp\
	source/i18n/csrutf8.cpp\
	source/i18n/inputext.cpp\
	source/i18n/wintzimpl.cpp\
	source/i18n/windtfmt.cpp\
	source/i18n/winnmfmt.cpp\
	source/i18n/basictz.cpp\
	source/i18n/dtrule.cpp\
	source/i18n/rbtz.cpp\
	source/i18n/tzrule.cpp\
	source/i18n/tztrans.cpp\
	source/i18n/vtzone.cpp\
	source/i18n/zonemeta.cpp\
	source/i18n/upluralrules.cpp\
	source/i18n/plurrule.cpp\
	source/i18n/plurfmt.cpp\
	source/i18n/selfmt.cpp\
	source/i18n/dtitvfmt.cpp\
	source/i18n/dtitvinf.cpp\
	source/i18n/udateintervalformat.cpp\
	source/i18n/tmunit.cpp\
	source/i18n/tmutamt.cpp\
	source/i18n/tmutfmt.cpp\
	source/i18n/currpinf.cpp\
	source/i18n/uspoof.cpp\
	source/i18n/uspoof_impl.cpp\
	source/i18n/uspoof_build.cpp\
	source/i18n/uspoof_conf.cpp\
	source/i18n/uspoof_wsconf.cpp\
	source/i18n/decfmtst.cpp\
	source/i18n/smpdtfst.cpp\
	source/i18n/ztrans.cpp\
	source/i18n/zrule.cpp\
	source/i18n/vzone.cpp\
	source/i18n/fphdlimp.cpp\
	source/i18n/fpositer.cpp\
	source/i18n/locdspnm.cpp\
	source/i18n/decNumber.c\
	source/i18n/decContext.c\
	source/i18n/alphaindex.cpp\
	source/i18n/tznames.cpp\
	source/i18n/tznames_impl.cpp\
	source/i18n/tzgnames.cpp\
	source/i18n/tzfmt.cpp\
	source/i18n/compactdecimalformat.cpp\
	source/i18n/gender.cpp\
	source/i18n/region.cpp\
	source/i18n/scriptset.cpp\
	source/i18n/identifier_info.cpp\
	source/i18n/uregion.cpp
LOCAL_MODULE := libicui18n

include $(BUILD_STATIC_LIBRARY)


#original path: androidbuild/lib/libicule.a
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= \
-DU_LAYOUT_IMPLEMENTATION -frtti -fexceptions 

LOCAL_C_INCLUDES:= \
	icu/source/layout\
	icu/source\
	icu/source/common
LOCAL_SRC_FILES:= \
	source/layout/LEFontInstance.cpp\
	source/layout/OpenTypeLayoutEngine.cpp\
	source/layout/ThaiLayoutEngine.cpp\
	source/layout/ThaiShaping.cpp\
	source/layout/ThaiStateTables.cpp\
	source/layout/ArabicLayoutEngine.cpp\
	source/layout/GXLayoutEngine.cpp\
	source/layout/HanLayoutEngine.cpp\
	source/layout/IndicLayoutEngine.cpp\
	source/layout/LayoutEngine.cpp\
	source/layout/ContextualGlyphSubstProc.cpp\
	source/layout/IndicRearrangementProcessor.cpp\
	source/layout/LigatureSubstProc.cpp\
	source/layout/LookupTables.cpp\
	source/layout/MorphTables.cpp\
	source/layout/NonContextualGlyphSubstProc.cpp\
	source/layout/SegmentArrayProcessor.cpp\
	source/layout/SegmentSingleProcessor.cpp\
	source/layout/SimpleArrayProcessor.cpp\
	source/layout/SingleTableProcessor.cpp\
	source/layout/StateTableProcessor.cpp\
	source/layout/SubtableProcessor.cpp\
	source/layout/TrimmedArrayProcessor.cpp\
	source/layout/AlternateSubstSubtables.cpp\
	source/layout/AnchorTables.cpp\
	source/layout/ArabicShaping.cpp\
	source/layout/CanonData.cpp\
	source/layout/CanonShaping.cpp\
	source/layout/ClassDefinitionTables.cpp\
	source/layout/ContextualSubstSubtables.cpp\
	source/layout/CoverageTables.cpp\
	source/layout/CursiveAttachmentSubtables.cpp\
	source/layout/DeviceTables.cpp\
	source/layout/ExtensionSubtables.cpp\
	source/layout/Features.cpp\
	source/layout/GDEFMarkFilter.cpp\
	source/layout/GlyphDefinitionTables.cpp\
	source/layout/GlyphIterator.cpp\
	source/layout/GlyphLookupTables.cpp\
	source/layout/GlyphPosnLookupProc.cpp\
	source/layout/GlyphPositionAdjustments.cpp\
	source/layout/GlyphPositioningTables.cpp\
	source/layout/GlyphSubstLookupProc.cpp\
	source/layout/GlyphSubstitutionTables.cpp\
	source/layout/IndicClassTables.cpp\
	source/layout/IndicReordering.cpp\
	source/layout/LEInsertionList.cpp\
	source/layout/LEGlyphStorage.cpp\
	source/layout/LigatureSubstSubtables.cpp\
	source/layout/LookupProcessor.cpp\
	source/layout/Lookups.cpp\
	source/layout/MarkArrays.cpp\
	source/layout/MarkToBasePosnSubtables.cpp\
	source/layout/MarkToLigaturePosnSubtables.cpp\
	source/layout/MarkToMarkPosnSubtables.cpp\
	source/layout/MirroredCharData.cpp\
	source/layout/MPreFixups.cpp\
	source/layout/MultipleSubstSubtables.cpp\
	source/layout/OpenTypeUtilities.cpp\
	source/layout/PairPositioningSubtables.cpp\
	source/layout/ScriptAndLanguage.cpp\
	source/layout/ScriptAndLanguageTags.cpp\
	source/layout/ShapingTypeData.cpp\
	source/layout/SinglePositioningSubtables.cpp\
	source/layout/SingleSubstitutionSubtables.cpp\
	source/layout/SubstitutionLookups.cpp\
	source/layout/ValueRecords.cpp\
	source/layout/KhmerLayoutEngine.cpp\
	source/layout/KhmerReordering.cpp\
	source/layout/TibetanLayoutEngine.cpp\
	source/layout/TibetanReordering.cpp\
	source/layout/HangulLayoutEngine.cpp\
	source/layout/KernTable.cpp\
	source/layout/loengine.cpp\
	source/layout/ContextualGlyphInsertionProc2.cpp\
	source/layout/ContextualGlyphSubstProc2.cpp\
	source/layout/GXLayoutEngine2.cpp\
	source/layout/IndicRearrangementProcessor2.cpp\
	source/layout/LigatureSubstProc2.cpp\
	source/layout/MorphTables2.cpp\
	source/layout/NonContextualGlyphSubstProc2.cpp\
	source/layout/SegmentArrayProcessor2.cpp\
	source/layout/SegmentSingleProcessor2.cpp\
	source/layout/SimpleArrayProcessor2.cpp\
	source/layout/SingleTableProcessor2.cpp\
	source/layout/StateTableProcessor2.cpp\
	source/layout/SubtableProcessor2.cpp\
	source/layout/TrimmedArrayProcessor2.cpp
LOCAL_MODULE := libicule

include $(BUILD_STATIC_LIBRARY)


#original path: androidbuild/lib/libiculx.a
include $(CLEAR_VARS)

LOCAL_CPPFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= \
-DU_LAYOUTEX_IMPLEMENTATION -frtti -fexceptions 

LOCAL_C_INCLUDES:= \
	icu/source/layoutex\
	icu/source\
	icu/source/common
LOCAL_SRC_FILES:= \
	source/layoutex/ParagraphLayout.cpp\
	source/layoutex/RunArrays.cpp\
	source/layoutex/LXUtilities.cpp\
	source/layoutex/playout.cpp\
	source/layoutex/plruns.cpp
LOCAL_MODULE := libiculx

include $(BUILD_STATIC_LIBRARY)


#original path: androidbuild/lib/libicuio.a
include $(CLEAR_VARS)

LOCAL_CFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
-DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
-DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
-DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
-DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
-ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
-DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= \
-DU_IO_IMPLEMENTATION 
LOCAL_CPPFLAGS:= -frtti -fexceptions 

LOCAL_C_INCLUDES:= \
	icu/source/common\
	icu/source/i18n
LOCAL_SRC_FILES:= \
	source/io/locbund.cpp\
	source/io/ufile.c\
	source/io/ufmt_cmn.c\
	source/io/uprintf.c\
	source/io/uprntf_p.c\
	source/io/uscanf.c\
	source/io/uscanf_p.c\
	source/io/ustdio.c\
	source/io/sprintf.c\
	source/io/sscanf.c\
	source/io/ustream.cpp\
	source/io/ucln_io.c
LOCAL_MODULE := libicuio

include $(BUILD_STATIC_LIBRARY)

# #original path: androidbuild/lib/libicudata.a
# include $(CLEAR_VARS)
# 
# LOCAL_CFLAGS:= -fno-short-wchar -DU_USING_ICU_NAMESPACE=1 -fno-short-enums \
# -DU_HAVE_NL_LANGINFO_CODESET=0 -D__STDC_INT64__ -DU_TIMEZONE=0 \
# -DUCONFIG_NO_LEGACY_CONVERSION=1 -DUCONFIG_NO_BREAK_ITERATION=1 \
# -DUCONFIG_NO_COLLATION=1 -DUCONFIG_NO_FORMATTING=1 \
# -DUCONFIG_NO_TRANSLITERATION=0 -DUCONFIG_NO_REGULAR_EXPRESSIONS=1 \
# -ffunction-sections -fdata-sections -D_REENTRANT -DU_HAVE_ELF_H=1 \
# -DU_ENABLE_DYLOAD=0 -DU_HAVE_ATOMIC=1 -DU_ATTRIBUTE_DEPRECATED= -DPIC \
# -fPIC 
# 
# LOCAL_C_INCLUDES:= \
# 	icu/source/common\
# 	icu/androidbuild/common
# LOCAL_SRC_FILES:= \
# 	androidbuild/data/out/tmp/icudt52l_dat.s
# LOCAL_MODULE := libicudata
# 
# include $(BUILD_STATIC_LIBRARY)