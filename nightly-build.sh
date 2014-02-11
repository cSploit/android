#!/bin/bash

# Copyleft (C) 2014 The dSploit Project
#
# Author: Louis Teboul (a.k.a Androguide.fr) 
#         admin@androguide.fr
#
# Licensed under the GNU GENERAL PUBLIC LICENSE version 3 'or later'
# The GNU General Public License is a free, copyleft license for software and other kinds of works.
# see the LICENSE file distributed with this work for a full version of the License.

CYAN="\\033[1;36m"
GREEN="\\033[1;32m"
YELLOW="\\E[33;44m"
RED="\\033[1;31m"
RESET="\\e[0m"
DATE=`date +%Y-%m-%d`
LOG_DIR="${DIR}/logs/"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ ! -d "${LOG_DIR}" ]; then
    mkdir -p $LOG_DIR
fi

if [ ! -d "${NIGHTLIES_OUT_DIR}" ]; then
    mkdir -p $NIGHTLIES_OUT_DIR
fi

echo -n -e "${CYAN}Building dSploit...${RESET}\n" | tee $LOG_DIR/$DATE.log
./gradlew clean | tee $LOG_DIR/$DATE.log
./gradlew assembleRelease | tee $LOG_DIR/$DATE.log

echo -n -e "${GREEN}Copying signed apk to output directory${RESET} ('NIGHTLIES_OUT_DIR' environment variable)\n" | tee $LOG_DIR/$DATE.log
cp $DIR/dSploit/build/apk/dSploit-release.apk $NIGHTLIES_OUT_DIR/dSploit-$DATE-nightly.apk | tee $LOG_DIR/$DATE.log

echo -n -e "${GREEN}Done.${RESET}\n\n" | tee $LOG_DIR/$DATE.log

