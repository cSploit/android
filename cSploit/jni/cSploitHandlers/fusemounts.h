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
 * 
 * 
 */
#ifndef HANDLERS_FUSEMOUNTS_H
#define HANDLERS_FUSEMOUNTS_H

enum fusemount_action {
  FUSEMOUNT_BIND
};

struct fusemount_bind_info {
  char          fusemount_action;  ///< must be set to ::FUSEMOUNT_BIND
  /**
   * @brief string array containing mount source and destination
   * 
   * data[0] is the source path
   * data[1] is the destination path
   */
  char          data[];
};

message *fusemounts_output_parser(char *);

#endif
