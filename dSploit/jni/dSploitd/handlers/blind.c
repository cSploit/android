/* LICENSE
 * 
 */

#include "handler.h"
#include "blind.h"

handler handler_info = {
  NULL,                   // next
  BLIND_HANDLER_ID,       // handler id
  0,                      // have_stdin
  0,                      // have_stdout
  1,                      // enabled
  NULL,                   // raw_output_parser
  NULL,                   // output_parser
  NULL,                   // input_parser
  NULL,                   // argv[0]
  NULL,                   // workdir
  "blind"                 // handler name
};