#include "handler.h"

handler handler_info = {
  NULL,   // next
  1,      // handler id
  1,      // have_stdin
  1,      // have_stdout
  1,      // enabled
  NULL,   // raw_output_parser
  NULL,   // output_parser
  NULL,   // input_parser
  NULL,   // argv[0]
  NULL,   // workdir
  "raw"   // handler name
};