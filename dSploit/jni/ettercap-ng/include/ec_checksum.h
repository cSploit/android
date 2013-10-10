
/* $Id: ec_checksum.h,v 1.6 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_CHECKSUM_H
#define EC_CHECKSUM_H

#include <ec_packet.h>

EC_API_EXTERN u_int16 L3_checksum(u_char *buf, size_t len);
EC_API_EXTERN u_int16 L4_checksum(struct packet_object *po);
#define CSUM_INIT    0
#define CSUM_RESULT  0

EC_API_EXTERN u_int32 CRC_checksum(u_char *buf, size_t len, u_int32 init);
#define CRC_INIT_ZERO   0x0
#define CRC_INIT        0xffffffff
#define CRC_RESULT      0xdebb20e3

EC_API_EXTERN u_int16 checksum_shouldbe(u_int16 sum, u_int16 computed_sum);

#endif

/* EOF */

// vim:ts=3:expandtab

