/*
 * 
 * LICENSE
 */

#ifndef COMMAND_H
#define COMMAND_H

struct message;
struct conn_node;
struct child_node;

int on_command_request(struct conn_node *, struct message *);
int on_cmd_stderr(struct child_node *, char *);
int on_child_done(struct child_node *, int );

#endif
