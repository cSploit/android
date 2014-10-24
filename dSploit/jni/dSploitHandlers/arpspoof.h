/* LICENSE
 * 
 * 
 */
#ifndef HANDLERS_ARPSPOOF_H
#define HANDLERS_ARPSPOOF_H

enum arpspoof_action {
  ARPSPOOF_ERROR
};

struct arpspoof_error_message {
  char          arpspoof_action;  ///< must be set to ::ARPSPOOF_ERROR
  char          text[];           ///< the error message
};

message *arpspoof_output_parser(char *);

#endif
