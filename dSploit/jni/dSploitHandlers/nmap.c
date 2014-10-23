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
#include "nmap.h"
#include "message.h"
#include "str_array.h"

handler handler_info = {
  NULL,                   // next
  2,                      // handler id
  0,                      // have_stdin
  1,                      // have_stdout
  1,                      // enabled
  NULL,                   // raw_output_parser
  &nmap_output_parser,    // output_parser
  NULL,                   // input_parser
  "tools/nmap/nmap",      // argv[0]
  NULL,                   // workdir
  "nmap"                  // handler name
};

regex_t hop_pattern, port_pattern, xml_port_pattern, xml_os_pattern;

__attribute__((constructor))
void nmap_init() {
  int ret;
  
  if((ret = regcomp(&hop_pattern, "^([0-9]+) +(\\.\\.\\.|[0-9\\.]+ m?s) +([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|[0-9]+)", REG_EXTENDED | REG_ICASE))) {
    print( ERROR, "%s: regcomp(hop_pattern): %d", ret);
  }
  if((ret = regcomp(&port_pattern, "^Discovered open port ([0-9]+)/([a-z]+)", REG_EXTENDED | REG_ICASE))) {
    print( ERROR, "%s: regcomp(port_pattern): %d", ret);
  }
  if((ret = regcomp(&xml_port_pattern, "<port protocol=\"([^\"]+)\" portid=\"([^\"]+)\"><state state=\"open\"[^>]+><service(.+(product|name)=\"([^\"]+)\")?(.+version=\"([^\"]+)\")?", REG_EXTENDED | REG_ICASE))) {
    print( ERROR, "%s: regcomp(xml_port_pattern): %d", ret);
  }
  if((ret = regcomp(&xml_os_pattern, "<osclass type=\"([^\"]+)\".+osfamily=\"([^\"]+)\".+accuracy=\"([^\"]+)\"", REG_EXTENDED | REG_ICASE))) {
    print( ERROR, "%s: regcomp(xml_os_pattern): %d", ret);
  }
}

__attribute__((destructor))
void nmap_fini() {
  regfree(&hop_pattern);
  regfree(&port_pattern);
  regfree(&xml_port_pattern);
  regfree(&xml_os_pattern);
}

/**
 * @brief search for HOP in an nmap trace scan.
 * @returns a ::message on success, NULL on error.
 */
message *parse_nmap_hop(char *line) {
  regmatch_t pmatch[4];
  struct nmap_hop_info *hop_info;
  unsigned long tousec;
  float time;
  message *m;
  
  if(regexec(&hop_pattern, line, 4, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct nmap_hop_info), 0);
  if(!m) {
    print( ERROR, "cannot create messages" );
    return NULL;
  }
  
  // terminate lines for parsing single parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  
  if(*(line + pmatch[2].rm_eo - 2) == 'm') { // millisconds
    tousec = 1000;
    *(line + pmatch[2].rm_eo - 3) = '\0';
  } else if(*(line + pmatch[2].rm_eo - 1) == 's'){ // seconds
    tousec = 1000000;
    *(line + pmatch[2].rm_eo -2) = '\0';
  } else { // ...
    tousec = 0;
  }
  
  hop_info = (struct nmap_hop_info *) m->data;
  hop_info->nmap_action = HOP;
  hop_info->hop = atoi(line);
  if(strncmp(line + pmatch[2].rm_so, "...", 3)) {
    sscanf(line + pmatch[2].rm_so, "%f", &(time));
    hop_info->usec = (uint32_t)(tousec * time);
  }
  hop_info->address = inet_addr(line + pmatch[3].rm_so);
  
  return m;
}

/**
 * @brief search for discovered open ports after a SYN scan.
 * @param line the line to parse
 * @returns a ::message on success, NULL on error.
 */
message *parse_nmap_port(char *line) {
  regmatch_t pmatch[3];
  struct nmap_port_info *port_info;
  message *m;
  
  if(regexec(&port_pattern, line, 3, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct nmap_port_info), 0);
  
  if(!m) {
    print( ERROR, "cannot create messages" );
    return NULL;
  }
  
  // terminate substrings
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  
  port_info = (struct nmap_port_info *)m->data;
  port_info->nmap_action = PORT;
  port_info->port = strtoul(line + pmatch[1].rm_so, NULL, 10);
  
  if(!strncmp(line + pmatch[2].rm_so, "tcp", 4)) {
    port_info->proto = TCP;
  } else if(!strncmp(line + pmatch[2].rm_so, "udp", 4)) {
    port_info->proto = UDP;
  } else if(!strncmp(line + pmatch[2].rm_so, "icmp", 5)) {
    port_info->proto = ICMP;
  } else if(!strncmp(line + pmatch[2].rm_so, "igmp", 5)) {
    port_info->proto = IGMP;
  } else {
    port_info->proto = UNKNOWN;
  }
  
  return m;
}

/**
 * @brief search for service informations in nmap XML output.
 * @returns a ::message on success, NULL on error.
 */
message *parse_nmap_xml_port(char *line) {
  regmatch_t pmatch[8];
  struct nmap_service_info *service_info;
  message *m;
  
  if(regexec(&xml_port_pattern, line, 8, pmatch, 0))
    return NULL;
    
  print( DEBUG, "%s", line);
  
  m = create_message(0, sizeof(struct nmap_port_info), 0);
  
  if(!m) {
    print( ERROR, "%s: cannot create messages" );
    return NULL;
  }
  
  service_info = (struct nmap_service_info *) m->data;
  
  // terminate parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  if(pmatch[5].rm_eo >= 0)
    *(line + pmatch[5].rm_eo) = '\0';
  if(pmatch[7].rm_eo >= 0)
    *(line + pmatch[7].rm_eo) = '\0';
  
  // debug
  print( DEBUG, "proto:   '%s'", (line + pmatch[1].rm_so));
  print( DEBUG, "port:    '%s'", (line + pmatch[2].rm_so));
  if(pmatch[5].rm_eo >= 0) print( DEBUG, "service: '%s'", (line + pmatch[5].rm_so));
  if(pmatch[7].rm_eo >= 0) print( DEBUG, "version: '%s'", (line + pmatch[7].rm_so));
  
  if(!strncmp(line + pmatch[1].rm_so, "tcp", 4)) {
    service_info->proto = TCP;
  } else if(!strncmp(line + pmatch[1].rm_so, "udp", 4)) {
    service_info->proto = UDP;
  } else if(!strncmp(line + pmatch[1].rm_so, "icmp", 5)) {
    service_info->proto = ICMP;
  } else if(!strncmp(line + pmatch[1].rm_so, "igmp", 5)) {
    service_info->proto = IGMP;
  } else {
    service_info->proto = UNKNOWN;
  }
  
  service_info->port = strtoul(line + pmatch[2].rm_so, NULL, 10);
  
  if(pmatch[5].rm_eo >= 0) {
    
    service_info->nmap_action = SERVICE;
    if(string_array_add(m, offsetof(struct nmap_service_info, service), (line + pmatch[5].rm_so)) ||
      (pmatch[7].rm_eo >= 0 && string_array_add(m, offsetof(struct nmap_service_info, service), (line + pmatch[7].rm_so)))) {
      print( ERROR, "cannot add string to message");
      goto error;
    }
  } else {
    service_info->nmap_action = PORT;
  }
  
  return m;
  
  error:
  free_message(m);
  return NULL;
}

/**
 * @brief search for os info in nmap xml output
 * @param line the line to scan
 * @returns a ::message to send or NULL if not found.
 */
message *parse_nmap_xml_os(char *line) {
  regmatch_t pmatch[4];
  struct nmap_os_info *os_info;
  message *m;
  
  if(regexec(&xml_os_pattern, line, 4, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct nmap_os_info), 0);
  
  if(!m) {
    print( ERROR, "cannot create messages" );
    return NULL;
  }
  
  // terminate strings
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  
  os_info = (struct nmap_os_info*) m->data;
  
  os_info->nmap_action = OS;
  os_info->accuracy = strtoul(line + pmatch[3].rm_so, NULL, 10);
  if(string_array_add(m, offsetof(struct nmap_os_info, os), line + pmatch[2].rm_so) ||
      string_array_add(m, offsetof(struct nmap_os_info, os), line + pmatch[1].rm_so)) {
    print( ERROR, "cannot add string to message");
    free_message(m);
    return NULL;
  }
  return m;
}

/**
 * @brief function that call all nmap output parsers on @p line
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *nmap_output_parser(char *line) {
  static message *(*parsers[4])(char *) = {
    &parse_nmap_hop,
    &parse_nmap_port,
    &parse_nmap_xml_port,
    &parse_nmap_xml_os
  };
  message *m;
  register int i;
    
  if(!strlen(line)) {
    return NULL;
  }
  
  for(m=NULL,i=0;i <4 && !m;i++) {
    m = (*parsers[i])(line);
  }
  
  return m;
}
