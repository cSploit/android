#include "handler.h"

handler handler_info = {
  NULL,   // next
  0,      // handler id
  0,      // have_stdin
  0,      // have_stdout
  1,      // enabled
  NULL,   // raw_output_parser
  NULL,   // output_parser
  NULL,   // input_parser
  NULL,   // argv[0]
  NULL,   // workdir
  "blind" // handler name
};