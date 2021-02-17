#!/bin/sh -ex
#
# A bootstrapping script that can be used to generate the autoconf,
# automake and libtool-related scripts of the build process.

set -e

autoreconf --install

