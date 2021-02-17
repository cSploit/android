

#ifndef EC_SNIFF_BRIDGE_H
#define EC_SNIFF_BRIDGE_H

#include <ec_packet.h>

/* exported functions */

EC_API_EXTERN void start_bridge_sniff(void);
EC_API_EXTERN void stop_bridge_sniff(void);
EC_API_EXTERN void forward_bridge_sniff(struct packet_object *po);
EC_API_EXTERN void bridge_check_forwarded(struct packet_object *po);
EC_API_EXTERN void bridge_set_forwardable(struct packet_object *po);

#endif

/* EOF */

// vim:ts=3:expandtab

