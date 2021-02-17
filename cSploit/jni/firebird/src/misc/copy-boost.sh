#!/bin/sh
# This script is based on a git-svn tree.

BOOST=$1

mkdir -p src/include/firebird/impl
bcp --boost=$BOOST --namespace=FirebirdImpl preprocessor/seq src/include/firebird/impl
find src/include/firebird/impl/boost -type f -exec sed -i 's/BOOST_/FB_BOOST_/g' {} \;
find src/include/firebird/impl/boost -type f -exec sed -i 's/<boost\//<firebird\/impl\/boost\//g' {} \;

g++ -ggdb -Isrc/include/gen -Isrc/include -E src/include/firebird/Message.h | sed -n -e 's/.*"\(.*impl.*\)".*/\1/p' | sort -u > gen/boost
for line in `cat gen/boost`; do git add $line; done
git add src/include/firebird/impl/boost/preprocessor/control
git add src/include/firebird/impl/boost/preprocessor/detail
git add src/include/firebird/impl/boost/preprocessor/repetition/detail
rm gen/boost

echo Now run this:
echo git commit src/include/firebird/impl/boost
echo rm -rf src/include/firebird/impl/boost
echo git checkout -- src/include/firebird
