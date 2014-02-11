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
RESET="\\e[0m"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -f "${DIR}/cronjob.txt" ]; then
    rm -f $DIR/cronjob.txt
fi

echo -n -e "${CYAN}Generating cronjob file...${RESET}"
echo "00 00 * * * /bin/bash ${DIR}/nightly-build.sh" > $DIR/cronjob.txt

echo -n -e "${CYAN}Adding job to cron...${RESET}"
crontab cronjob.txt

echo -n -e "${GREEN}Done.${RESET}"
