## threads

  - prober: send NB or ARP requests to all hosts of the network
  - pkt_sniffer: sniff using pcap [ filter: (((arp or rarp) and arp[6:2] & 1 == 0) or ( udp and port 137)) ]
  - resolver: perform async reverse DNS resolutions
  - notifier: print events to stdout

## workflow

### prober

  1. if host is !known goto 4
  2. if host has timedout, delete host and goto 4
  3. send ARP request; end
  4. send NB request

### pkt_sniffer

  1. name = ( pkt is NB ? ((NB)pkt).name : NULL )
  2. mac = pkt.mac
  3. ip = ( pkt is ARP ? ((ARP)pkt).ip : ((UDP)pkt).ip )
  4. add new host(mac, ip, name)

### resolver

  1. if host.name or host.dns_sent end
  2. ares_gethostbyaddr(host.ip)

### resolver.callback

  1. if host.name end
  2. host.name = DNS_reply.name
  
## internals

### Memory vs CPU

there is many points where we have to check if the current host has been found.
this is why i choosen an offset access table rater then an associative one.
the offset is the gap between current host IP address and the base IP address.
that array will contain only pointers to a struct. a NULL pointer is an unknown host.

### delay

we must delay a bit between every round.

### synchronization

a `pthread_cond_t` global variable is used to notify that an host has been found.
another `pthread_cond_t` global variable is used to notify that an event has been added.
