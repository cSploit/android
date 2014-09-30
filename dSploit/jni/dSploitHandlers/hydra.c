#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "handler.h"
#include "logger.h"
#include "hydra.h"
#include "message.h"

handler handler_info = {
  NULL,                   // next
  4,                      // handler id
  0,                      // have_stdin
  1,                      // have_stdout
  1,                      // enabled
  NULL,                   // raw_output_parser
  &hydra_output_parser,   // output_parser
  NULL,                   // input_parser
  "tools/hydra/hydra",    // argv[0]
  NULL,                   // workdir
  "hydra"                 // handler name
};

regex_t status_pattern, alert_pattern, login_pattern;

__attribute__((constructor))
void hydra_init() {
  int ret;
  
  if((ret = regcomp(&status_pattern, "^\\[STATUS\\] .*, ([0-9]+) tries in [^,]+, ([0-9]+) todo", REG_EXTENDED))) {
    print( ERROR, "regcomp(status_pattern): %d", ret);
  }
  if((ret = regcomp(&alert_pattern, "^\\[(ERROR|WARNING)\\] ", REG_EXTENDED | REG_ICASE))) {
    print( ERROR, "regcomp(alert_pattern): %d", ret);
  }
  if((ret = regcomp(&login_pattern, "^\\[([0-9]+)\\]\\[[^]]+\\] (host: ([^ ]+)( +|$))?(login: ([^ ]+)( +|$))?(password: ([^ ]+)( +|$))?", REG_EXTENDED))) {
    print( ERROR, "regcomp(login_pattern): %d", ret);
  }
}

__attribute__((destructor))
void hydra_fini() {
  regfree(&status_pattern);
  regfree(&alert_pattern);
  regfree(&login_pattern);
}

/**
 * @brief search for HOP in an nmap trace scan.
 * @returns a ::message on success, NULL on error.
 */
message *parse_hydra_status(char *line) {
  regmatch_t pmatch[3];
  struct hydra_status_info *status_info;
  message *m;
  
  if(regexec(&status_pattern, line, 3, pmatch, 0))
    return NULL;
  
  m = create_message(0, sizeof(struct hydra_status_info), 0);
  if(!m) {
    print( ERROR, "cannot create messages");
    return NULL;
  }
  
  // terminate lines for parsing single parts
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[2].rm_eo) = '\0';
  
  
  status_info = (struct hydra_status_info *) m->data;
  status_info->hydra_action = HYDRA_STATUS;
  status_info->sent = strtoul(line + pmatch[1].rm_so, NULL, 10);
  status_info->left = strtoul(line + pmatch[2].rm_so, NULL, 10);
  
  return m;
}

/**
 * @brief search for an error or warning message in hydra output
 * @param line the line to parse
 * @returns a ::message on success, NULL on error.
 */
message *parse_hydra_alert(char *line) {
  regmatch_t pmatch[1];
  struct hydra_warning_info *alert_info;  // warning and error info differ only by hydra_action
  message *m;
  size_t len;
  
  if(regexec(&alert_pattern, line, 1, pmatch, 0))
    return NULL;
  
  len = strlen(line + pmatch[0].rm_eo);
  
  m = create_message(0, sizeof(struct hydra_warning_info) + len + 1, 0);
  
  if(!m) {
    print( ERROR, "cannot craete messages");
    return NULL;
  }
  
  alert_info = (struct hydra_warning_info *) m->data;
  
  if(line[1] == 'E') // ERROR
    alert_info->hydra_action = HYDRA_ERROR;
  else
    alert_info->hydra_action = HYDRA_WARNING;
  
  memcpy(alert_info->text, line + pmatch[0].rm_eo, len);
  *(alert_info->text + len) = '\0';
  
  return m;
}

/**
 * @brief search for login in hydra output
 * @param line the line to parse
 * @returns a ::message on success, NULL on error.
 */
message *parse_hydra_login(char *line) {
  regmatch_t pmatch[11];
  struct hydra_login_info *login_info;
  message *m;
  size_t host_len, login_len, pswd_len;
  uint8_t num_sep;
  char *pswd_ptr;
  
  if(regexec(&login_pattern, line, 11, pmatch, 0))
    return NULL;
  
  host_len  = (pmatch[3].rm_eo - pmatch[3].rm_so);
  login_len = (pmatch[6].rm_eo - pmatch[6].rm_so);
  pswd_len  = (pmatch[9].rm_eo - pmatch[9].rm_so);
  
  num_sep = 0;
  
  if(login_len) {
    num_sep++;
  }
  if(pswd_len) {
    num_sep++;
  }
  
  m = create_message(0,
        sizeof(struct hydra_login_info) + (num_sep) + (login_len + pswd_len)
        , 0);
  
  if(!m) {
    print( ERROR, "cannot create messages");
    return NULL;
  }
  
  // terminate substrings
  *(line + pmatch[1].rm_eo) = '\0';
  *(line + pmatch[3].rm_eo) = '\0';
  
  login_info = (struct hydra_login_info *)m->data;
  login_info->hydra_action = HYDRA_LOGIN;
  login_info->port = strtoul(line + pmatch[1].rm_so, NULL, 10);
  login_info->contents = 0;
  
  if(host_len) {
    login_info->contents |= HAVE_ADDRESS;
    login_info->address = inet_addr(line + pmatch[3].rm_so + 6);
  }
  
  if(login_len) {
    login_info->contents |= HAVE_LOGIN;
    memcpy(login_info->data, line + pmatch[6].rm_so, login_len);
    *(login_info->data + login_len) = '\0';
    pswd_ptr = login_info->data + login_len + 1;
  } else {
    pswd_ptr = login_info->data;
  }
  
  if(pswd_len) {
    login_info->contents |= HAVE_PASSWORD;
    memcpy(pswd_ptr, line + pmatch[9].rm_so, pswd_len);
    *(pswd_ptr + pswd_len) = '\0';
  }
  
  return m;
}

/**
 * @brief function that call all hydra output parsers on @p line
 * @param line the line to parse
 * @returns a message to send or NULL
 */
message *hydra_output_parser(char *line) {
  static message *(*parsers[3])(char *) = {
    &parse_hydra_status,
    &parse_hydra_alert,
    &parse_hydra_login
  };
  message *m;
  register int i;
    
  if(!strlen(line)) {
    return NULL;
  }
  
  for(m=NULL,i=0;i <3 && !m;i++) {
    m = (*parsers[i])(line);
  }
  
  return m;
}
