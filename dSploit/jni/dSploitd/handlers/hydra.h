/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_HYDRA_H
#define HANDLERS_HYDRA_H

#include <arpa/inet.h>

#define HYDRA_HANDLER_ID 4

enum hydra_action {
  STATUS,
  WARNING,
  ERROR,
  ACCOUNT
};

/// hydra status info
struct hydra_status_info {
  char                hydra_action;   ///< must be set to ::STATUS
  unsigned long int   sent;           ///< # of sent logins
  unsigned long int   left;           ///< # of logins left to try
};

/// hydra warning info
struct hydra_warning_info {
  char                hydra_action;    ///< must be set to ::WARNING
  char                text[];
};

/// hydra error info
struct hydra_error_info {
  char                hydra_action;    ///< must be set to ::ERROR
  char                text[];
};

#define HAVE_ADDRESS  1               ///< ::hydra_account_info contains an address
#define HAVE_LOGIN    2               ///< ::hydra_account_info contains a login
#define HAVE_PASSWORD 4               ///< ::hydra_account_info contains a password

/// hydra account info
struct hydra_account_info {
  char                hydra_action;   ///< must be set to ::ACCOUNT
  uint16_t            port;           ///< port where we found this account.
  uint8_t             contents;       ///< 0 OR bitmask of ::HAVE_ADDRESS, ::HAVE_LOGIN, ::HAVE_PASSWORD
  in_addr_t           address;        ///< address where we found this account, if ( ::hydra_account_info.contents & ::HAVE_ADDRESS )
  /**
   * @brief found credentials
   * 
   * this string array contains the found credentials in this order:
   * login, password
   * 
   * check ::hydra_account_info.contents for their existence.
   * 
   * they are separed by ::STRING_SEPARATOR if both exists.
   */
  char                data[];
};

message *hydra_output_parser(char *);

#endif
