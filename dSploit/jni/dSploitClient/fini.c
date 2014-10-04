/* LICENSE
 * 
 */

#include "control.h"
#include "cache.h"
#include "child.h"
#include "reader.h"
#include "handler.h"
#include "auth.h"

void destroy_controls() {
  control_destroy(&(children.control));
  control_destroy(&(incoming_messages.control));
  control_destroy(&(handlers.control));
  control_destroy(&(logged.control));
}

__attribute__((destructor))
void client_destructor() {
  destroy_controls();
  free_cache();
}