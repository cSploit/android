To construct a revised data file, copy it to this directory, changing its
name to icudt42l.dat, and then run the following command in this directory:

    icupkg --add add.txt icudt42l.dat

Then copy the new data file back to its source location.

Note: If you have built but not installed ICU on the Mac, you will need
to do something like this to execute the command:

    ICU_SOURCE=/Users/danfuzz/down/icu/source
    DYLD_LIBRARY_PATH=$ICU_SOURCE/lib $ICU_SOURCE/bin/icupkg
