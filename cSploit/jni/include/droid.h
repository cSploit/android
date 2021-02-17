// Adds all the needed header data that android is missing

#ifndef _DROID_H_
#define _DROID_H_

//from ubuntu's netinet/in.h
//typedef uint32_t in_addr_t;

//from ubuntu's net/ethernet.h
struct ether_addr
{
  u_int8_t ether_addr_octet[ETH_ALEN];
} __attribute__ ((__packed__));


#endif
