
/* $Id: ec_decode.h,v 1.17 2004/09/02 13:17:41 alor Exp $ */

#ifndef EC_DECODE_H
#define EC_DECODE_H

#include <ec_proto.h>
#include <ec_packet.h>
#include <ec_hook.h>
#include <pcap.h>

/* layer canonical name */

enum {
   IFACE_LAYER  = 1,
   LINK_LAYER   = 2,
   NET_LAYER    = 3,
   PROTO_LAYER  = 4,
   APP_LAYER    = 5,       /* special case for the middleware decoder. don't use it */
   APP_LAYER_TCP = 51,
   APP_LAYER_UDP = 52,
};

#define FUNC_DECODER(func) void * func(u_char *buf, u_int16 buflen, int *len, struct packet_object *po)
#define FUNC_DECODER_PTR(func) void * (*func)(u_char *buf, u_int16 buflen, int *len, struct packet_object *po)

#define DECODE_DATALEN   buflen
#define DECODE_DATA      buf
#define DECODED_LEN      *len
#define PACKET           po

#define EXECUTE_DECODER(x) do{ \
   if (x) \
      x(DECODE_DATA+DECODED_LEN, DECODE_DATALEN-DECODED_LEN, len, PACKET); \
}while(0)

#define DECLARE_REAL_PTR_END(x,y) u_char *x = po->DATA.data; \
                                  u_char *y = x + po->DATA.len

#define DECLARE_DISP_PTR_END(x,y) u_char *x = po->DATA.disp_data; \
                                  u_char *y = x + po->DATA.disp_len
                                  
#define DISPLAY_DATA    po->disp_data
#define DISPLAY_LEN     po->disp_len                             

/* exported functions */

EC_API_EXTERN void ec_decode(u_char *u, const struct pcap_pkthdr *pkthdr, const u_char *pkt);
EC_API_EXTERN void add_decoder(u_int8 level, u_int32 type, FUNC_DECODER_PTR(decoder));
EC_API_EXTERN void del_decoder(u_int8 level, u_int32 type);
EC_API_EXTERN void * get_decoder(u_int8 level, u_int32 type);


#endif

/* EOF */

// vim:ts=3:expandtab

