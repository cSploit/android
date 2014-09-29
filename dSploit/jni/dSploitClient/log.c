/* LICENSE
 * 
 */
#include <stdlib.h>

#include "log.h"
#include "message.h"

void android_dump_message(message *m) {
  char *str;
  
  str = message_to_string(m);
  
  LOGD("%s", str);
  
  if(str)
    free(str);
}
  