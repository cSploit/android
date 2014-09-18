#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "handler.h"
#include "nmap.h"
#include "message.h"

handler handler_info = {
  NULL,                   // next
  NMAP_HANDLER_ID,        // handler id
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
    fprintf(stderr, "%s: regcomp(hop_pattern): %d\n", __func__, ret);
  }
  if((ret = regcomp(&port_pattern, "^Discovered open port ([0-9]+)/([a-z]+)", REG_EXTENDED | REG_ICASE))) {
    fprintf(stderr, "%s: regcomp(port_pattern): %d\n", __func__, ret);
  }
  if((ret = regcomp(&xml_port_pattern, "<port protocol=\"([^\"]+)\" portid=\"([^\"]+)\"><state state=\"open\"[^>]+><service(.+product=\"([^\"]+)\")?(.+version=\"([^\"]+)\")?", REG_EXTENDED | REG_ICASE))) {
    fprintf(stderr, "%s: regcomp(xml_port_pattern): %d\n", __func__, ret);
  }
  if((ret = regcomp(&xml_os_pattern, "<osclass type=\"([^\"]+)\".+osfamily=\"([^\"]+)\".+accuracy=\"([^\"]+)\"", REG_EXTENDED | REG_ICASE))) {
    fprintf(stderr, "%s: regcomp(xml_os_pattern): %d\n", __func__, ret);
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
    fprintf(stderr, "%s: cannot create messages\n", __func__);
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
    fprintf(stderr, "%s: cannot create messages\n", __func__);
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
  regmatch_t pmatch[7];
  struct nmap_service_info *service_info;
  message *m;
  uint16_t service_len, version_len;
  
  if(regexec(&xml_port_pattern, line, 7, pmatch, 0))
    return NULL;
  
  // when not found this expression is: var_len = ( (-1) - (-1) ) = 0;
  service_len = ( pmatch[4].rm_eo - pmatch[4].rm_so );
  version_len = ( pmatch[6].rm_eo - pmatch[6].rm_so );
  
  if(!service_len) { // no service found, only an open port.
    m = create_message(0, sizeof(struct nmap_port_info), 0);
  } else if(version_len) { // service and version found.
    m = create_message(0, sizeof(struct nmap_service_info) + (service_len + 1 + version_len), 0);
  } else { // only service found, no version.
    m = create_message(0, sizeof(struct nmap_service_info) + service_len, 0);
  }
  
  if(!m) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
    return NULL;
  }
  
  service_info = (struct nmap_service_info *) m->data;
  
  // terminate parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  if(service_len)
    *(line + pmatch[4].rm_eo) = '\0';
  if(version_len)
    *(line + pmatch[6].rm_eo) = '\0';
  
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
  
  if(service_len) {
    service_info->nmap_action = SERVICE;
    memcpy(service_info->service, line + pmatch[4].rm_so, service_len);
    if(version_len) {
      *(service_info->service + service_len) = STRING_SEPARATOR;
      memcpy(service_info->service + service_len + 1, line + pmatch[6].rm_so, version_len);
    }
  } else {
    service_info->nmap_action = PORT;
  }
  
  return m;
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
  uint16_t type_len, os_len;
  
  if(regexec(&xml_os_pattern, line, 7, pmatch, 0))
    return NULL;
  
  type_len = (pmatch[1].rm_eo - pmatch[1].rm_so);
  os_len   = (pmatch[2].rm_eo - pmatch[2].rm_so);
  
  m = create_message(0, sizeof(struct nmap_os_info) + type_len + os_len + 1, 0);
  
  if(!m) {
    fprintf(stderr, "%s: cannot create messages\n", __func__);
    return NULL;
  }
  
  *(line + pmatch[3].rm_eo) = '\0';
  
  os_info = (struct nmap_os_info*) m->data;
  
  os_info->nmap_action = OS;
  os_info->accuracy = strtoul(line + pmatch[3].rm_so, NULL, 10);
  memcpy(os_info->os, line + pmatch[2].rm_so, os_len);
  *(os_info->os + os_len) = STRING_SEPARATOR;
  memcpy(os_info->os + os_len + 1, line + pmatch[1].rm_so, type_len);
  
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
