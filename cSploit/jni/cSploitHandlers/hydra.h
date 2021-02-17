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
#ifndef HANDLERS_HYDRA_H
#define HANDLERS_HYDRA_H

#include <arpa/inet.h>

enum hydra_action {
  HYDRA_ATTEMPTS,
  HYDRA_WARNING,
  HYDRA_ERROR,
  HYDRA_LOGIN
};

/// hydra attempts info
struct hydra_attempts_info {
  char                hydra_action;   ///< must be set to ::HYDRA_ATTEMPTS
  unsigned long int   sent;           ///< # of sent logins
  unsigned long int   left;           ///< # of logins left to try
  unsigned int        elapsed;        ///< elapsed time in minutes
  unsigned int        eta;            ///< ETA in minutes
};

/// hydra warning info
struct hydra_warning_info {
  char                hydra_action;    ///< must be set to ::HYDRA_WARNING
  char                text[];
};

/// hydra error info
struct hydra_error_info {
  char                hydra_action;    ///< must be set to ::HYDRA_ERROR
  char                text[];
};

#define HAVE_ADDRESS  1               ///< ::hydra_account_info contains an address
#define HAVE_LOGIN    2               ///< ::hydra_account_info contains a login
#define HAVE_PASSWORD 4               ///< ::hydra_account_info contains a password

/// hydra login info
struct hydra_login_info {
  char                hydra_action;   ///< must be set to ::LOGIN
  uint16_t            port;           ///< port where we found this login.
  uint8_t             contents;       ///< 0 OR bitmask of ::HAVE_ADDRESS, ::HAVE_LOGIN, ::HAVE_PASSWORD
  in_addr_t           address;        ///< address where we found this login, if ( ::hydra_login_info.contents & ::HAVE_ADDRESS )
  /**
   * @brief found credentials
   * 
   * this string array contains the found credentials in this order:
   * login, password
   * 
   * check ::hydra_account_info.contents for their existence.
   */
  char                data[];
};

message *hydra_output_parser(char *);

#endif
