/* LICENSE
 * 
 */
#ifndef CONTROLLER_H
#define CONTROLLER_H

struct conn_node;
struct message;

int on_control_request(struct conn_node *, struct message *);

#endif