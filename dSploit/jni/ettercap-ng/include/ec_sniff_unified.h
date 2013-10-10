
/* $Id: ec_sniff_unified.h,v 1.5 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_SNIFF_UNIFIED_H
#define EC_SNIFF_UNIFIED_H

/* exported functions */

EC_API_EXTERN void start_unified_sniff(void);
EC_API_EXTERN void stop_unified_sniff(void);
EC_API_EXTERN void forward_unified_sniff(struct packet_object *po);
EC_API_EXTERN void unified_check_forwarded(struct packet_object *po);
EC_API_EXTERN void unified_set_forwardable(struct packet_object *po);

#endif

/* EOF */

// vim:ts=3:expandtab

