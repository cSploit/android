/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SCANNER_H
#define SCANNER_H


/** time to scan all private networks ( in ms ) */
#define FULL_SCAN_MS 600000 // 10 minutes

/** time to scal all hosts in the local subnet ( in ms ) */
#define LOCAL_SCAN_MS 10000 // 10 seconds

/** number of hosts in private networks */
#define PRIVATE_NETWORKS_HOSTS 17891328

void full_scan(void);
void local_scan(void);

#endif