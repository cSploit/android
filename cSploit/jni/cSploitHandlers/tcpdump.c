#include <string.h>
#include <regex.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "handler.h"
#include "logger.h"
#include "tcpdump.h"
#include "message.h"
#include "common_regex.h"

handler handler_info = {
  NULL,                       // next
  6,                          // handler id
  0,                          // have_stdin
  1,                          // have_stdout
  1,                          // enabled
  NULL,                       // raw_output_parser
  &tcpdump_output_parser,     // output_parser
  NULL,                       // input_parser
  "tools/tcpdump/tcpdump",    // argv[0]
  NULL,                       // workdir
  "tcpdump"                   // handler name
};

regex_t packet_pattern;

__attribute__((constructor))
void arpspoof_init() {
  int ret;
  
  if((ret = regcomp(&packet_pattern, "length ([0-9]+)\\) (" IPv4_REGEX ")\\.[0-9]+ > (" IPv4_REGEX ")\\.[0-9]+", REG_EXTENDED))) {
    print(ERROR, "regcomp(packet_pattern): %d", ret);
  }
}

__attribute__((destructor))
void arpspoof_fini() {
  regfree(&packet_pattern);
}

/**
 * @brief search for error messages and report them back if present
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *tcpdump_output_parser(char *line) {
  message *m;
  regmatch_t pmatch[4];
  struct tcpdump_packet_info *packet_info;
  
  if(regexec(&packet_pattern, line, 4, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct tcpdump_packet_info), 0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  packet_info = (struct tcpdump_packet_info *) m->data;
  
  // terminate single parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  
  packet_info->tcpdump_action = TCPDUMP_PACKET;
  packet_info->src = inet_addr(line + pmatch[2].rm_so);
  packet_info->dst = inet_addr(line + pmatch[3].rm_so);
  packet_info->len = strtoul(line + pmatch[1].rm_so, NULL, 10);
  
  return m;
}
