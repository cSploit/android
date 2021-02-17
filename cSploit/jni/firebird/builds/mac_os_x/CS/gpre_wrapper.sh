#!/bin/sh

make -C ../../../gen/$2 -f `pwd`/Helpers.make PWD_CURR=`pwd` $1
