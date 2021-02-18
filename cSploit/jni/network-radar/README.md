network-radar
=============

## what it is

`network-radar` is the native replacement for the old dSploit `NetworkDiscovery` .
it's completely written in C, using threads for faster execution and libcares for asynchronous name resolving.

## how it works

  - send a NetBIOS request to all hosts of your subnet
  - listen for ARP and NetBIOS responses
  - analyze those packets to learn alive hosts
  - search a name for those hosts that do not have one
  - monitor found hosts by periodically sending them an ARP request
  
## output format

corrently the output format is very simple. when an host is found the following line is printed.

```
HOST_ADD  { mac: 00:00:00:00:00:00, ip: 0.0.0.0, name: HOSTNAME }
```

where:
  - `00:00:00:00:00:00` is the Ethernet MAC address of the found host ( or 00:00:00:00:00:00 if unknown )
  - `0.0.0.0` is the IP address of the found host
  - `HOSTNAME` is the name for this host ( or an empty string if unknown )
  
when an host is modified the output is the same except for the first work that changes from `HOST_ADD  ` to `HOST_EDIT `.

when an host disappear from the radar the following line get printed:

```
HOST_DEL  { ip: 0.0.0.0 }
```

where:
  - `0.0.0.0` is the IP address of the disconnected host

## TODO

  - perform a traceroute to find all gateway between you and internet.
  - send NetBIOS requests to all hosts of the found private subnets ( outside your local one )
  - monitor found foreign hosts by sending a ping or some other packet
  - create a configure script to check libpcap and network headers

