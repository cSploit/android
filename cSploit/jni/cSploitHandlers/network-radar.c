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

#include <string.h>
#include <regex.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "handler.h"
#include "logger.h"
#include "network-radar.h"
#include "message.h"
#include "str_array.h"

handler handler_info = {
  NULL,                                     // next
  8,                                        // handler id
  0,                                        // have_stdin
  1,                                        // have_stdout
  1,                                        // enabled
  NULL,                                     // raw_output_parser
  &nrdr_output_parser,                      // output_parser
  NULL,                                     // input_parser
  "tools/network-radar/network-radar",      // argv[0]
  NULL,                                     // workdir
  "network-radar"                           // handler name
};

regex_t host_add_edit_pattern, host_del_pattern;

__attribute__((constructor))
void nrdr_init() {
  int ret;
  
  ret = regcomp(&host_add_edit_pattern,
      "^HOST_(ADD |EDIT) { mac: ([^,]+), ip: ([^,]+), name: (.*) }", REG_EXTENDED);
  
  if(ret) {
    print(ERROR, "regcomp(host_add_edit_pattern): %d", ret);
    return;
  }
  
  ret = regcomp(&host_del_pattern,
      "^HOST_DEL  { ip: (.*) }", REG_EXTENDED);
      
  if(ret) {
    print(ERROR, "regcomp(host_del_pattern): %d", ret);
    regfree(&host_add_edit_pattern);
    return;
  }
}

__attribute__((destructor))
void nrdr_fini() {
  regfree(&host_add_edit_pattern);
  regfree(&host_del_pattern);
}

/**
 * @brief parse the ::HOST_ADD / ::HOST_EDIT event
 * @returns a ::message on success, NULL on error.
 */
static message *parse_nrdr_host_add_edit(char *line) {
  regmatch_t pmatch[5];
  struct nrdr_host_info *hinfo;
  message *m;
  
  if(regexec(&host_add_edit_pattern, line, 5, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct nrdr_host_info), 0);
  
  if(!m) {
    print( ERROR, "cannot create messages");
    return NULL;
  }
  
  // terminate parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  *(line + pmatch[4].rm_eo) = '\0';
  
  hinfo = (struct nrdr_host_info *) m->data;
  
  if (*(line + pmatch[1].rm_so) == 'A') {
    hinfo->nrdr_action = NRDR_HOST_ADD;
  } else {
    hinfo->nrdr_action = NRDR_HOST_EDIT;
  }
  
  if(sscanf(line + pmatch[2].rm_so, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
      &(hinfo->eth_addr[0]), &(hinfo->eth_addr[1]), &(hinfo->eth_addr[2]),
      &(hinfo->eth_addr[3]), &(hinfo->eth_addr[4]), &(hinfo->eth_addr[5])) != 6) {
    print( ERROR, "cannot parse Ethernet address. string='%s'",
            line + pmatch[2].rm_so);
    goto error;
  }
  
  if(inet_pton(AF_INET, line + pmatch[3].rm_so, &(hinfo->ip_addr)) != 1) {
    print( ERROR, "cannot parse IP address. string='%s'",
            line + pmatch[3].rm_so);
    goto error;
  }
  
  if( pmatch[4].rm_eo != pmatch[4].rm_so &&
      string_array_add(m, offsetof(struct nrdr_host_info, name),
          line + pmatch[4].rm_so) ) {
    print( ERROR, "cannot add string to message");
    goto error;
  }
  
  return m;
  
  error:
  free_message(m);
  return NULL;
}

/**
 * @brief parse the ::HOST_DEL event
 * @returns a ::message on success, NULL on error.
 */
static message *parse_nrdr_host_del(char *line) {
  regmatch_t pmatch[2];
  struct nrdr_host_del_info *hinfo;
  message *m;
  
  if(regexec(&host_del_pattern, line, 2, pmatch, 0))
    return NULL;
    
  m = create_message(0, sizeof(struct nrdr_host_del_info), 0);
  if(!m) {
    print( ERROR, "cannot create messages");
    return NULL;
  }
  
  *(line + pmatch[1].rm_eo) = '\0';
  
  hinfo = (struct nrdr_host_del_info *) m->data;
  
  hinfo->nrdr_action = NRDR_HOST_DEL;
  
  if(inet_pton(AF_INET, line + pmatch[1].rm_so, &(hinfo->ip_addr)) != 1) {
    print( ERROR, "cannot parse IP address. string='%s'",
            line + pmatch[1].rm_so);
    free_message(m);
    return NULL;
  }
  
  return m;
}

/**
 * @brief parse events from network-radar
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *nrdr_output_parser(char *line) {
  static message *(*parsers[2])(char *) = {
    &parse_nrdr_host_add_edit,
    &parse_nrdr_host_del
  };
  message *m;
  register int i;
  
  if(!strlen(line)) {
    return NULL;
  }
  
  print( DEBUG, "%s", line);
  
  for(m=NULL,i=0;i <2 && !m;i++) {
    m = (*parsers[i])(line);
  }
  
  return m;
}
