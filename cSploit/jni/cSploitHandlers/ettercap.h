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
#ifndef HANDLERS_ETTERCAP_H
#define HANDLERS_ETTERCAP_H

#include <arpa/inet.h>

enum ettercap_action {
  ETTERCAP_READY,
  ACCOUNT
};

struct ettercap_ready_info {
  char          ettercap_action;  ///< must be set to ::ETTERCAP_READY
};

struct ettercap_account_info {
  char          ettercap_action;  ///< must be set to ::ACCOUNT
  in_addr_t     address;          ///< the address that sent these credentials
  /**
   * @brief array of null-terminated strings
   * data[0] is the portocol
   * data[1] is the username
   * data[2] is the password
   */
  char          data[];
};

message *ettercap_output_parser(char *);

#endif
