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

#include "issue1.h"

#define NUM_ELEM(a) (sizeof(a) / sizeof(a[0]))

typedef void (*issue_fn)();

issue_fn known_issues[] = {
  issue1
};

int main() {
  int i,n;
  
  n = NUM_ELEM(known_issues);
  
  for(i=0;i<n;i++) {
    known_issues[i]();
  }
  return 0;
}