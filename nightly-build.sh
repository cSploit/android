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
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_DIR="${DIR}/logs/"
MAX_DAYS="15"
PREVIOUS_COMMIT=$(cat "${LOG_DIR}/last_commit")

if [ ! -d "${LOG_DIR}" ]; then
    mkdir -p $LOG_DIR
fi

if [ ! -d "${NIGHTLIES_OUT_DIR}" ]; then
    mkdir -p $NIGHTLIES_OUT_DIR
fi

cd "${DIR}" || exit 1 | tee $LOG_DIR/$DATE.log

echo -n -e "${YELLOW}Cleaning old files${RESET}\n" | tee $LOG_DIR/$DATE.log
LAST_APK=$(readlink -f "${NIGHTLIES_OUT_DIR}/dSploit-lastest.apk")
find $NIGHTLIES_OUT_DIR -type f -a -ctime -"${MAX_DAYS}" -a ! -name "${LAST_APK}" -delete &>1 | tee $LOG_DIR/$DATE.log
find $LOG_DIR -type f -a -ctime -"${MAX_DAYS}" -delete &>1 | tee $LOG_DIR/$DATE.log

echo -n -e "${CYAN}Syncing git repo...${RESET}\n" | tee $LOG_DIR/$DATE.log
git pull | tee $LOG_DIR/$DATE.log

LAST_COMMIT=$(git rev-parse HEAD)

if [ -n "${PREVIOUS_COMMIT}" -a "${PREVIOUS_COMMIT}" == "${LAST_COMMIT}" ]; then
    echo -n -e "${YELLOW}Nothing changed${RESET}" | tee $LOG_DIR/$DATE.log
    echo -n -e "${GREEN}Done" | tee $LOG_DIR/$DATE.log
    exit 0
fi

echo -n -e "${CYAN}Building dSploit...${RESET}\n" | tee $LOG_DIR/$DATE.log
rm -f dSploit/build/apk/dSploit-release.apk
./gradlew clean | tee $LOG_DIR/$DATE.log
./gradlew assembleRelease | tee $LOG_DIR/$DATE.log

echo -n -e "${GREEN}Copying signed apk to output directory${RESET}\n" | tee $LOG_DIR/$DATE.log
cp dSploit/build/apk/dSploit-release.apk $NIGHTLIES_OUT_DIR/dSploit-$LAST_COMMIT.apk &&
ln -sf "dSploit-${LAST_COMMIT}.apk" $NIGHTLIES_OUT_DIR/dSploit-lastest.apk &&
echo "${LAST_COMMIT}" > "${LOG_DIR}/last_commit" | tee $LOG_DIR/$DATE.log

echo -n -e "${GREEN}Done.${RESET}\n\n" | tee $LOG_DIR/$DATE.log
