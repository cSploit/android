#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "handler.h"
#include "logger.h"
#include "ettercap.h"
#include "message.h"

handler handler_info = {
  NULL,                       // next
  3,                          // handler id
  0,                          // have_stdin
  1,                          // have_stdout
  1,                          // enabled
  NULL,                       // raw_output_parser
  &ettercap_output_parser,    // output_parser
  NULL,                       // input_parser
  "tools/ettercap/ettercap",  // argv[0]
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
  if((ret = regcomp(&account_pattern, "^([^ ]+).*[^0-9](([0-9]{1,3}\\.){3}[0-9]{1,3}).* -> USER: (.*)  PASS: ((.*)  [^ ]|(.*)$)", REG_EXTENDED | REG_ICASE))) {
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
  
  m->data[0] = READY;
  
  return m;
}

/**
 * @brief search for accounts in ettercap output.
 * @returns a ::message on success, NULL on error.
 */
message *parse_ettercap_account(char *line) {
  regmatch_t pmatch[7];
  struct ettercap_account_info *account_info;
  message *m;
  size_t proto_len, user_len, pswd_len;
  
  if(regexec(&account_pattern, line, 7, pmatch, 0))
    return NULL;
  
  proto_len = (pmatch[1].rm_eo - pmatch[1].rm_so);
  user_len  = (pmatch[4].rm_eo - pmatch[4].rm_so);
  pswd_len  = (pmatch[5].rm_eo - pmatch[5].rm_so);
  
  
  m = create_message(0,
        sizeof(struct ettercap_account_info) + proto_len + user_len + pswd_len + 3,
        0);
  
  if(!m) {
    print(ERROR, "cannot create messages");
    return NULL;
  }
  
  // terminate lines for parsing single parts
  *(line + pmatch[2].rm_eo) = '\0';
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[4].rm_eo) = '\0';
  *(line + pmatch[5].rm_eo) = '\0';
  
  account_info = (struct ettercap_account_info *) m->data;
  account_info->ettercap_action = ACCOUNT;
  account_info->address = inet_addr(line + pmatch[2].rm_so);
  memcpy(account_info->data, line + pmatch[1].rm_so, proto_len);
  memcpy(account_info->data + proto_len + 1, line + pmatch[4].rm_so, user_len);
  memcpy(account_info->data + proto_len + user_len + 2, line + pmatch[5].rm_so, pswd_len);
  *(account_info->data + proto_len) = 
  *(account_info->data + proto_len + user_len + 1) = 
  *(account_info->data + proto_len + user_len + pswd_len + 2) = '\0';
  
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
