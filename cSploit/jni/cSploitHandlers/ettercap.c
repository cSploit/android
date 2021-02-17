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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "handler.h"
#include "logger.h"
#include "ettercap.h"
#include "message.h"
#include "str_array.h"
#include "common_regex.h"

handler handler_info = {
  NULL,                       // next
  3,                          // handler id
  0,                          // have_stdin
  1,                          // have_stdout
  1,                          // enabled
  NULL,                       // raw_output_parser
  &ettercap_output_parser,    // output_parser
  NULL,                       // input_parser
  "./ettercap",               // argv[0]
  "tools/ettercap",           // workdir
  "ettercap"                  // handler name
};

regex_t ready_pattern, account_pattern;

__attribute__((constructor))
void ettercap_init() {
  int ret;
  
  if((ret = regcomp(&ready_pattern, "for inline help", REG_ICASE))) {
    print(ERROR, "regcomp(ready_pattern): %d", ret);
  }
  //TODO: search for dissectors that does not match this line and create a specific regex ( or change thier sources )
  if((ret = regcomp(&account_pattern, "^([^ ]+).*[^0-9](" IPv4_REGEX ").* -> USER: (.*)  PASS: ((.*)  [^ ]|(.*)$)", REG_EXTENDED | REG_ICASE))) {
    print(ERROR, "regcomp(account_pattern): %d", ret);
  }
}

__attribute__((destructor))
void ettercap_fini() {
  regfree(&ready_pattern);
  regfree(&account_pattern);
}


/**
 * @brief search for the ready message in ettercap output.
 * @returns a ::message on success, NULL on error.
 */
message *parse_ettercap_ready(char *line) {
  message *m;
  
  if(regexec(&ready_pattern, line, 0, NULL, 0))
    return NULL;
  
  m = create_message(0, 1, 0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  m->data[0] = ETTERCAP_READY;
  
  return m;
}

/**
 * @brief search for accounts in ettercap output.
 * @returns a ::message on success, NULL on error.
 */
message *parse_ettercap_account(char *line) {
  regmatch_t pmatch[6];
  struct ettercap_account_info *account_info;
  message *m;
  int array_start;
  
  if(regexec(&account_pattern, line, 6, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct ettercap_account_info), 0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  // terminate lines for parsing single parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  *(line + pmatch[4].rm_eo) = '\0';
  
  array_start = offsetof(struct ettercap_account_info, data);
  
  account_info = (struct ettercap_account_info *) m->data;
  account_info->ettercap_action = ACCOUNT;
  account_info->address = inet_addr(line + pmatch[2].rm_so);
  
  if(string_array_add(m, array_start, line + pmatch[1].rm_so) ||
     string_array_add(m, array_start, line + pmatch[3].rm_so) ||
     string_array_add(m, array_start, line + pmatch[4].rm_so)) {
     print( ERROR, "cannot add string to message");
     free_message(m);
     return NULL;
   }
  
  return m;
}

/**
 * @brief function that call all ettercap output parsers on @p line
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *ettercap_output_parser(char *line) {
  static message *(*parsers[2])(char *) = {
    &parse_ettercap_ready,
    &parse_ettercap_account
  };
  message *m;
  register int i;
    
  if(!strlen(line)) {
    return NULL;
  }
  
  for(m=NULL,i=0;i <2 && !m;i++) {
    m = (*parsers[i])(line);
  }
  
  return m;
}
