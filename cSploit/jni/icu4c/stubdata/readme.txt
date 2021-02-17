This directory is used for building our Android ICU data file.

1. To generate ICU data files:run the icu_dat_generator.py script.
   The command is:
     ./icu_dat_generator.py  <icu version> [-v] [-h]
   For example:
     ./icu_dat_generator.py  4.4

2. To add a resource to Android ICU data file: insert an entry to
   icudtxx-<tag name>.txt under external/icu4c/stubdata directory then
   run the icu_dat_generator.py.
   For example, to add French sort to icudt44l-us.dat, you need to
   a. Add an entry, "coll/fr.res", into external/icu4c/stubdata/icudt44l-us.txt
   b. run "./icu_dat_generator.py 4.4".

3. To add a new Adnroid ICU data file: add the <tag name> to datlist[] in
   icu_dat_generator.py and add corresponding resource list to
   icudtxxl<tag name>.txt. Then run the script icu_dat_generator.py to generate
   dat files.
   For example, to add icudt44l-latin.dat, you need to
   a. Modify icu_dat_generator.py by adding "latin" into datlist.
   b. Make a new file icudt44l-latin.txt to include the resource list.
   c. run "./icu_dat_generator.py 4.4".

4. Add a new resource or modify existing ICU resource definition:
   Note: This is a rare case. You should talk to ICU team first if it is a bug
   in ICU resource or a feature enhancement before making such changes.
   If you would like to add existing ICU resource to Android, please check #2.
   a. Create or change the text format resource files under external/icu4c/data.
   b. Make a temporary directory for ICU build.
      i.e. mkdir external/icu4c/icuBuild
   c. cd to ICU build directory.
      i.e. cd external/icu4c/icuBuild
   d. Run external/icu4c/runConfigureICU with "Linux" option to generate the
      makefile.
      i.e. $ANDROID_BUILD_TOP/external/icu4c/runConfigureICU Linux
   e. make -j2
   f. The new icudtxxl.dat is under data/out/tmp and the individual resources are
      under data/out/build/icudtxxl
      For example, you can find data/out/tmp/icudt44l.dat and data/out/build/icudt44l/*.res.
   g. Copy the new icudtxxl.dat over $ANDROID_BUILD_TOP/external/icu4c/stubdata/icudtxxl-all.dat.
      i.e. cp data/out/tmp/icudt44l.dat $ANDROID_BUILD_TOP/external/icu4c/stubdata/icudt44l-all.dat.
   h. Check #1 or #2 to replace or add resource to ICU.
   i. Clean up ICU build directory.
   j. Discuss with icu-team how to include the change to public ICU.

Locale Resource Files:
- icudt44l-all.dat contains the resources in packed
  form. It includes everything that comes with
  a vanilla ICU 4.4. icu_dat_generator.py uses this file to generate
  custom build dat files.

- cnv/*.cnv are the additional encodings required by Android.
  These are
  - gsm-03.38-2000.cnv
  - iso-8859_16-2001.cnv
  - docomo-shift_jis-2007.cnv
  - kddi-jisx-208-2007.cnv
  - kddi-shift_jis-2007.cnv
  - softbank-jisx-208-2007.cnv
  - softbank-shift_jis-2007.cnv

- Text format ICU resource files are under external/icu4c/data directory.
  Binary resource files are packaged in external/icu4c/stubdata/icudtxxl-all.dat.
  If you have special need such as bug fxings or examining individual resource size,
  you can run icupkg utility to extract icudtxxl-all.dat into a temporary directory.
  For example:
  cd $ANDROID_BUILD_TOP/external/icu4c/stubdata
  cp icudt44l-all.dat icudt44l.dat
  mkdir tempDir
  $ANDROID_BUILD_TOP/prebuilt/linux-x86/icu-4.4/icupkg  icudt44l.dat -x "*" -d tempDir

Run ICU tests:
ICU tests are not part of Android build. If you change the ICU code or data,
it is highly recommended to run ICU tests.
 1. Remove the flag "-R" in external/icu4c/data/Makefile.in.
    "Reverse collation keys" tables are not included in ICU data on Android. To
    pass ICU collation tests, you need to delete the flag "-R" in Makefile.in.
    Search for " -R" under "### collation res" section in external/icu4c/data/Makefile.in,
    delete all of them.
 2. Make a temporary directory for ICU build.
    i.e. mkdir external/icu4c/icuBuild
 3. cd to ICU build directory.
    i.e. cd external/icu4c/icuBuild
 4. Run external/icu4c/runConfigureICU with "Linux" option to generate the makefile.
    i.e. $ANDROID_BUILD_TOP/external/icu4c/runConfigureICU Linux
 5. make -j2 check
 6. Check the result. Ignore the errors from tsconv.

Note:
  1. The script assumes you have done
  envsetup.sh and choosecombo before, because
  it relies on an environment variable pointing
  to the prebuilt tools.

