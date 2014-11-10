/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_ETTERCAP_H
#define HANDLERS_ETTERCAP_H

#include <arpa/inet.h>

enum ettercap_action {
  READY,
  ACCOUNT
};

struct ettercap_ready_info {
  char          ettercap_action;  ///< must be set to ::READY
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
