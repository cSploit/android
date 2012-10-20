/*
 * arp.h
 *
 * ARP cache routines.
 * 
 * Copyright (c) 1999 Dug Song <dugsong@monkey.org>
 *
 * $Id: arp.h,v 1.1 2001/03/15 08:27:08 dugsong Exp $
 */

#ifndef _ARP_H_
#define _ARP_H_

int	arp_cache_lookup(in_addr_t ip, struct ether_addr *ether, const char* linf);

#endif /* _ARP_H_ */
