#!/bin/bash

# build coverage output
# require lcov installed

set -e
set -o pipefail

# go to main distro dir
trap 'echo Error at line $LINENO' ERR
DIR=`dirname $0`
cd "$DIR/.."
DIR=`pwd -P`
cd "$DIR"

# cleanup
rm -rf covtmp coverage
mkdir covtmp
mkdir coverage

# move required files in covtmp
find include/ src/ \( -name \*.\[ch\] -o -name \*.da -o -name \*.bb\* -o -name \*.gc\* \) | tar cf -  -T - | (cd covtmp && exec tar xf -)

# prepare
cd covtmp
find -name \*.bb\* | grep .libs | xargs rm -f || true
find -name \*.da -o -name \*.gc\* | grep '/.libs/' | xargs -imao -n1 sh -c 'mv mao $(echo mao | sed s,/.libs/,/,g)'

# save
cd ..
tar zcvf $HOME/coverage-$(date +%Y%m%d%H%M).tgz covtmp
cd covtmp

# generate coverage
geninfo . -o out0.info -t 'Test'
perl -ne '$skip = 1 if (m(^SF:/usr/include/)); print if(!$skip); $skip = 0 if (/^end_of_record$/);' < out0.info > out.info
genhtml out.info -t 'FreeTDS coverage' -o ../coverage -p "$DIR/covtmp"

echo Success !!
