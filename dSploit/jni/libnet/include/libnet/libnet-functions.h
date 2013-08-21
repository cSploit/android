/*
 *  $Id: libnet-functions.h,v 1.43 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet-functions.h - function prototypes
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __LIBNET_FUNCTIONS_H
#define __LIBNET_FUNCTIONS_H
/**
 * @file libnet-functions.h
 * @brief libnet exported function prototypes
 */

/**
 * Creates the libnet environment. It initializes the library and returns a 
 * libnet context. If the injection_type is LIBNET_LINK or LIBNET_LINK_ADV, the
 * function initializes the injection primitives for the link-layer interface
 * enabling the application programmer to build packets starting at the
 * data-link layer (which also provides more granular control over the IP
 * layer). If libnet uses the link-layer and the device argument is non-NULL,
 * the function attempts to use the specified network device for packet
 * injection. This is either a canonical string that references the device
 * (such as "eth0" for a 100MB Ethernet card on Linux or "fxp0" for a 100MB
 * Ethernet card on OpenBSD) or the dots and decimals representation of the
 * device's IP address (192.168.0.1). If device is NULL, libnet attempts to
 * find a suitable device to use. If the injection_type is LIBNET_RAW4 or
 * LIBNET_RAW4_ADV, the function initializes the injection primitives for the
 * IPv4 raw socket interface. The final argument, err_buf, should be a buffer
 * of size LIBNET_ERRBUF_SIZE and holds an error message if the function fails.
 * This function requires root privileges to execute successfully. Upon
 * success, the function returns a valid libnet context for use in later
 * function calls; upon failure, the function returns NULL.
 * @param injection_type packet injection type (LIBNET_LINK, LIBNET_LINK_ADV, LIBNET_RAW4, LIBNET_RAW4_ADV, LIBNET_RAW6, LIBNET_RAW6_ADV)
 * @param device the interface to use (NULL and libnet will choose one)
 * @param err_buf will contain an error message on failure
 * @return libnet context ready for use or NULL on error.
 */
libnet_t *
libnet_init(int injection_type, const char *device, char *err_buf);

/**
 * Shuts down the libnet session referenced by l. It closes the network 
 * interface and frees all internal memory structures associated with l.  
 * @param l pointer to a libnet context
 */
void
libnet_destroy(libnet_t *l);

/**
 * Clears the current packet referenced and frees all pblocks. Should be
 * called when the programmer want to send a completely new packet of
 * a different type using the same context.
 * @param l pointer to a libnet context
 */
void
libnet_clear_packet(libnet_t *l);

/**
 * Fills in a libnet_stats structure with packet injection statistics
 * (packets written, bytes written, packet sending errors).
 * @param l pointer to a libnet context
 * @param ls pointer to a libnet statistics structure
 */
void
libnet_stats(libnet_t *l, struct libnet_stats *ls);

/**
 * Returns the FILENO of the file descriptor used for packet injection.
 * @param l pointer to a libnet context
 * @return the file number of the file descriptor used for packet injection
 */
int 
libnet_getfd(libnet_t *l);

/**
 * Returns the canonical name of the device used for packet injection.
 * @param l pointer to a libnet context
 * @return the canonical name of the device used for packet injection. Note 
 * it can be NULL without being an error.
 */
const char *
libnet_getdevice(libnet_t *l);

/**
 * Returns the pblock buffer contents for the specified ptag; a
 * subsequent call to libnet_getpbuf_size() should be made to determine the
 * size of the buffer.
 * @param l pointer to a libnet context
 * @param ptag the ptag reference number
 * @return a pointer to the pblock buffer or NULL on error
 */
uint8_t *
libnet_getpbuf(libnet_t *l, libnet_ptag_t ptag);

/**
 * Returns the pblock buffer size for the specified ptag; a
 * previous call to libnet_getpbuf() should be made to pull the actual buffer
 * contents.
 * @param l pointer to a libnet context
 * @param ptag the ptag reference number
 * @return the size of the pblock buffer
 */ 
uint32_t
libnet_getpbuf_size(libnet_t *l, libnet_ptag_t ptag);

/**
 * Returns the last error set inside of the referenced libnet context. This
 * function should be called anytime a function fails or an error condition
 * is detected inside of libnet.
 * @param l pointer to a libnet context
 * @return an error string or NULL if no error has occured
 */ 
char *
libnet_geterror(libnet_t *l);

/**
 * Returns the sum of the size of all of the pblocks inside of l (this should
 * be the resuling packet size).
 * @param l pointer to a libnet context
 * @return the size of the packet in l
 */ 
uint32_t
libnet_getpacket_size(libnet_t *l);

/**
 * Seeds the psuedo-random number generator.
 * @param l pointer to a libnet context
 * @return 1 on success, -1 on failure
 */
int
libnet_seed_prand(libnet_t *l);

/**
 * Generates an unsigned psuedo-random value within the range specified by
 * mod.
 * LIBNET_PR2    0 - 1
 * LIBNET_PR8    0 - 255
 * LIBNET_PR16   0 - 32767
 * LIBNET_PRu16  0 - 65535
 * LIBNET_PR32   0 - 2147483647
 * LIBNET_PRu32  0 - 4294967295
 *
 * @param mod one the of LIBNET_PR* constants
 * @return 1 on success, -1 on failure
 */
uint32_t
libnet_get_prand(int mod);

/**
 * If a given protocol header is built with the checksum field set to "0", by
 * default libnet will calculate the header checksum prior to injection. If the
 * header is set to any other value, by default libnet will not calculate the
 * header checksum. To over-ride this behavior, use libnet_toggle_checksum().
 * Switches auto-checksumming on or off for the specified ptag. If mode is set
 * to LIBNET_ON, libnet will mark the specificed ptag to calculate a checksum 
 * for the ptag prior to injection. This assumes that the ptag refers to a 
 * protocol that has a checksum field. If mode is set to LIBNET_OFF, libnet
 * will clear the checksum flag and no checksum will be computed prior to 
 * injection. This assumes that the programmer will assign a value (zero or
 * otherwise) to the checksum field.  Often times this is useful if a
 * precomputed checksum or some other predefined value is going to be used.
 * Note that when libnet is initialized with LIBNET_RAW4, the IPv4 header
 * checksum will always be computed by the kernel prior to injection, 
 * regardless of what the programmer sets.
 * @param l pointer to a libnet context
 * @param ptag the ptag reference number
 * @param mode LIBNET_ON or LIBNET_OFF
 * @return 1 on success, -1 on failure
 */
int
libnet_toggle_checksum(libnet_t *l, libnet_ptag_t ptag, int mode);

/**
 * Takes a network byte ordered IPv4 address and returns a pointer to either a 
 * canonical DNS name (if it has one) or a string of dotted decimals. This may
 * incur a DNS lookup if the hostname and mode is set to LIBNET_RESOLVE. If
 * mode is set to LIBNET_DONT_RESOLVE, no DNS lookup will be performed and
 * the function will return a pointer to a dotted decimal string. The function
 * cannot fail -- if no canonical name exists, it will fall back on returning
 * a dotted decimal string. This function is non-reentrant.
 * @param in network byte ordered IPv4 address
 * @param use_name LIBNET_RESOLVE or LIBNET_DONT_RESOLVE
 * @return a pointer to presentation format string
 */
char *
libnet_addr2name4(uint32_t in, uint8_t use_name);

/**
 * Takes a dotted decimal string or a canonical DNS name and returns a 
 * network byte ordered IPv4 address. This may incur a DNS lookup if mode is
 * set to LIBNET_RESOLVE and host_name refers to a canonical DNS name. If mode
 * is set to LIBNET_DONT_RESOLVE no DNS lookup will occur. The function can
 * fail if DNS lookup fails or if mode is set to LIBNET_DONT_RESOLVE and
 * host_name refers to a canonical DNS name.
 * @param l pointer to a libnet context
 * @param host_name pointer to a string containing a presentation format host
 * name
 * @param use_name LIBNET_RESOLVE or LIBNET_DONT_RESOLVE
 * @return network byte ordered IPv4 address or -1 (2^32 - 1) on error 
 */
uint32_t
libnet_name2addr4(libnet_t *l, char *host_name, uint8_t use_name);

extern const struct libnet_in6_addr in6addr_error;

/**
 * Takes a dotted decimal string or a canonical DNS name and returns a 
 * network byte ordered IPv6 address. This may incur a DNS lookup if mode is
 * set to LIBNET_RESOLVE and host_name refers to a canonical DNS name. If mode
 * is set to LIBNET_DONT_RESOLVE no DNS lookup will occur. The function can
 * fail if DNS lookup fails or if mode is set to LIBNET_DONT_RESOLVE and
 * host_name refers to a canonical DNS name.
 * @param l pointer to a libnet context
 * @param host_name pointer to a string containing a presentation format host
 * name
 * @param use_name LIBNET_RESOLVE or LIBNET_DONT_RESOLVE
 * @return network byte ordered IPv6 address structure 
 */
struct libnet_in6_addr
libnet_name2addr6(libnet_t *l, char *host_name, uint8_t use_name);

/**
 * Should document this baby right here.
 */
void
libnet_addr2name6_r(struct libnet_in6_addr addr, uint8_t use_name,
char *host_name, int host_name_len);

/**
 * Creates a new port list. Port list chains are useful for TCP and UDP-based
 * applications that need to send packets to a range of ports (contiguous or
 * otherwise). The port list chain, which token_list points to, should contain
 * a series of int8_tacters from the following list: "0123456789,-" of the
 * general format "x - y, z", where "xyz" are port numbers between 0 and 
 * 65,535. plist points to the front of the port list chain list for use in 
 * further libnet_plist_chain() functions. Upon success, the function returns
 * 1. Upon failure, the function returns -1 and libnet_geterror() can tell you
 * why.
 * @param l pointer to a libnet context
 * @param plist if successful, will refer to the portlist, if not, NULL
 * @param token_list string containing the port list primitive
 * @return 1 on success, -1 on failure
 */
int
libnet_plist_chain_new(libnet_t *l, libnet_plist_t **plist, char *token_list);

/**
 * Returns the next port list chain pair from the port list chain plist. bport
 * and eport contain the starting port number and ending port number, 
 * respectively. Upon success, the function returns 1 and fills in the port
 * variables; however, if the list is empty, the function returns 0 and sets 
 * both port variables to 0. Upon failure, the function returns -1.
 * @param plist previously created portlist
 * @param bport will contain the beginning port number or 0
 * @param eport will contain the ending port number or 0
 * @return 1 on success, 0 if empty, -1 on failure
 */
int
libnet_plist_chain_next_pair(libnet_plist_t *plist, uint16_t *bport, 
uint16_t *eport); 

/**
 * Runs through the port list and prints the contents of the port list chain
 * list to stdout.
 * @param plist previously created portlist
 * @return 1 on success, -1 on failure
 */
int
libnet_plist_chain_dump(libnet_plist_t *plist);

/**
 * Runs through the port list and prints the contents of the port list chain
 * list to string. This function uses strdup and is not re-entrant.  It also
 * has a memory leak and should not really be used.
 * @param plist previously created portlist
 * @return a printable string containing the port list contents on success
 * NULL on error
 */
char *
libnet_plist_chain_dump_string(libnet_plist_t *plist);

/**
 * Frees all memory associated with port list chain.
 * @param plist previously created portlist
 * @return 1 on success, -1 on failure
 */
int
libnet_plist_chain_free(libnet_plist_t *plist);

/**
 * @section PBF Packet Builder Functions
 *
 * The core of libnet is the platform-independent packet-building 
 * functionality. These functions enable an application programmer to build 
 * protocol headers (and data) in a simple and consistent manner without having
 * to worry (too much) about low-level network odds and ends. Each 
 * libnet_build() function builds a piece of a packet (generally a protocol 
 * header). While it is perfectly possible to build an entire, 
 * ready-to-transmit packet with a single call to a libnet_build() function, 
 * generally more than one builder-class function call is required to construct
 * a full packet. A complete wire-ready packet generally consists of more than 
 * one piece.
 * Every function that builds a protocol header takes a series of arguments 
 * roughly corresponding to the header values as they appear on the wire. This 
 * process is intuitive but often makes for functions with huge prototypes and 
 * large stack frames.
 * One important thing to note is that you must call these functions in order, 
 * corresponding to how they should appear on the wire (from the highest 
 * protocol layer on down). This building process is intuitive; it approximates
 * what happens in an operating system kernel. In other words, to build a 
 * Network Time Protocol (NTP) packet by using the link-layer interface, the 
 * application programmer would call the libnet_build() functions in the 
 * following order:
 * 1. libnet_build_ntp()
 * 2. libnet_build_udp()
 * 3. libnet_build_ipv4()
 * 4. libnet_build_ethernet()
 * This ordering is essential for libnet 1.1.x to properly link together the 
 * packet internally (previous libnet versions did not have the requirement).
 *
 * @subsection TPI The Payload Interface
 *
 * The payload interface specifies an optional way to include data directly 
 * after the protocol header in question. You can use this function for a 
 * variety of purposes, including the following:
 * - Including additional or arbitrary protocol header information that is not 
 *   available from a libnet interface
 * - Including a packet payload (data segment)
 * - Building another protocol header that is not available from a libnet 
 *   interface
 * To employ the interface, the application programmer should construct the i
 * payload data and pass a const uint8_t * to this data and its size to the desired
 * libnet_build() function. Libnet handles the rest.
 *
 * It is important to note that some functions (notably the IPv6 builders) do
 * use the payload interface to specify variable length but ostensibly 
 * non-optional data. See the individual libnet_build_ipv6*() functions for
 * more information.
 * 
 * @subsection PT Protocol Tags and Packet Builder Return Values
 *
 * Libnet uses the protocol tag (ptag) to identify individual pieces of a 
 * packet after being created. A new ptag results every time a libnet_build() 
 * function with an empty (0) ptag argument completes successfully. This new 
 * ptag now refers to the packet piece just created. The application 
 * programmer's responsibility is to save this value if he or she plans to 
 * modify this particular portion later on in the program. If the application 
 * programmer needs to modify some portion of that particular packet piece 
 * again, he or she calls the same libnet_build() function specifying the 
 * saved ptag argument. Libnet then searches for that packet piece and modifies
 * it rather than creating a new one. Upon failure for any reason, 
 * libnet_build() functions return -1; libnet_geterror() tells you why.
 */

/**
 * Builds an IEEE 802.1q VLAN tagging header. Depending on the value of
 * len_proto, the function wraps the 802.1q header inside either an IEEE 802.3
 * header or an RFC 894 Ethernet II (DIX) header (both resulting in an 18-byte
 * frame). If len is 1500 or less, most receiving protocol stacks parse the
 * frame as an IEEE 802.3 encapsulated frame. If len is one of the Ethernet type
 * values, most protocol stacks parse the frame as an RFC 894 Ethernet II
 * encapsulated frame. Note the length value is calculated without the 802.1q
 * header of 18 bytes.
 * @param dst pointer to a six byte source ethernet address
 * @param src pointer to a six byte destination ethernet address
 * @param tpi tag protocol identifier
 * @param priority priority
 * @param cfi canonical format indicator
 * @param vlan_id vlan identifier
 * @param len_proto length (802.3) protocol (Ethernet II) 
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_802_1q(const uint8_t *dst, const uint8_t *src, uint16_t tpi,
uint8_t priority, uint8_t cfi, uint16_t vlan_id, uint16_t len_proto,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IEEE 802.1x extended authentication protocol header.
 * @param eap_ver the EAP version
 * @param eap_type the EAP type
 * @param length frame length
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_802_1x(uint8_t eap_ver, uint8_t eap_type, uint16_t length, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IEEE 802.2 LLC header.
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param control control field
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_802_2(uint8_t dsap, uint8_t ssap, uint8_t control,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IEEE 802.2 LLC SNAP header.
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param control control field
 * @param oui Organizationally Unique Identifier
 * @param type upper layer protocol
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_802_2snap(uint8_t dsap, uint8_t ssap, uint8_t control, 
uint8_t *oui, uint16_t type, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag); 

/**
 * Builds an IEEE 802.3 header. The 802.3 header is almost identical to the 
 * RFC 894 Ethernet II header, the exception being that the field immediately
 * following the source address holds the frame's length (as opposed to the
 * layer 3 protocol). You should only use this function when libnet is
 * initialized with the LIBNET_LINK interface.
 * @param dst destination ethernet address
 * @param src source ethernet address
 * @param len frame length sans header
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_802_3(const uint8_t *dst, const uint8_t *src, uint16_t len, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an Ethernet header. The RFC 894 Ethernet II header is almost 
 * identical to the IEEE 802.3 header, with the exception that the field 
 * immediately following the source address holds the layer 3 protocol (as
 * opposed to frame's length). You should only use this function when 
 * libnet is initialized with the LIBNET_LINK interface. 
 * @param dst destination ethernet address
 * @param src source ethernet address
 * @param type upper layer protocol type
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ethernet(const uint8_t *dst, const uint8_t *src, uint16_t type, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Autobuilds an Ethernet header. The RFC 894 Ethernet II header is almost 
 * identical to the IEEE 802.3 header, with the exception that the field 
 * immediately following the source address holds the layer 3 protocol (as
 * opposed to frame's length). You should only use this function when 
 * libnet is initialized with the LIBNET_LINK interface. 
 * @param dst destination ethernet address
 * @param type upper layer protocol type
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_ethernet(const uint8_t *dst, uint16_t type, libnet_t *l);

/**
 * Builds a Fiber Distributed Data Interface (FDDI) header.
 * @param fc class format and priority
 * @param dst destination fddi address
 * @param src source fddi address
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param cf cf
 * @param oui 3 byte IEEE organizational code
 * @param type upper layer protocol 
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_fddi(uint8_t fc, const uint8_t *dst, const uint8_t *src, uint8_t dsap,
uint8_t ssap, uint8_t cf, const uint8_t *oui, uint16_t type, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Autobuilds a Fiber Distributed Data Interface (FDDI) header.
 * @param fc class format and priority
 * @param dst destination fddi address
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param cf cf
 * @param oui IEEE organizational code
 * @param type upper layer protocol 
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_fddi(uint8_t fc, const uint8_t *dst, uint8_t dsap, uint8_t ssap,
uint8_t cf, const uint8_t *oui, uint16_t type, libnet_t *l);

/**
 * Builds an Address Resolution Protocol (ARP) header.  Depending on the op 
 * value, the function builds one of several different types of RFC 826 or
 * RFC 903 RARP packets.
 * @param hrd hardware address format
 * @param pro protocol address format
 * @param hln hardware address length
 * @param pln protocol address length
 * @param op ARP operation type
 * @param sha sender's hardware address
 * @param spa sender's protocol address
 * @param tha target hardware address
 * @param tpa targer protocol address
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_arp(uint16_t hrd, uint16_t pro, uint8_t hln, uint8_t pln,
uint16_t op, const uint8_t *sha, const uint8_t *spa, const uint8_t *tha, const uint8_t *tpa,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Autouilds an Address Resolution Protocol (ARP) header.  Depending on the op 
 * value, the function builds one of several different types of RFC 826 or
 * RFC 903 RARP packets.
 * @param op ARP operation type
 * @param sha sender's hardware address
 * @param spa sender's protocol address
 * @param tha target hardware address
 * @param tpa targer protocol address
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_arp(uint16_t op, const uint8_t *sha, const uint8_t *spa, const uint8_t *tha,
uint8_t *tpa, libnet_t *l);

/**
 * Builds an RFC 793 Transmission Control Protocol (TCP) header.
 * @param sp source port
 * @param dp destination port
 * @param seq sequence number
 * @param ack acknowledgement number
 * @param control control flags
 * @param win window size
 * @param sum checksum (0 for libnet to autofill)
 * @param urg urgent pointer
 * @param len total length of the TCP packet (for checksum calculation)
 * @param payload
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_tcp(uint16_t sp, uint16_t dp, uint32_t seq, uint32_t ack,
uint8_t control, uint16_t win, uint16_t sum, uint16_t urg, uint16_t len, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 793 Transmission Control Protocol (TCP) options header.
 * The function expects options to be a valid TCP options string of size 
 * options_s, which is no larger than 40 bytes (the maximum size of an 
 * options string). The function checks to ensure that the packet consists of 
 * a TCP header preceded by an IPv4 header, and that the addition of the
 * options string would not result in a packet larger than 65,535 bytes
 * (IPMAXPACKET). The function counts up the number of 32-bit words in the
 * options string and adjusts the TCP header length value as necessary.
 * @param options byte string of TCP options
 * @param options_s length of options string
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_tcp_options(const uint8_t *options, uint32_t options_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds an RFC 768 User Datagram Protocol (UDP) header.
 * @param sp source port
 * @param dp destination port
 * @param len total length of the UDP packet
 * @param sum checksum (0 for libnet to autofill)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_udp(uint16_t sp, uint16_t dp, uint16_t len, uint16_t sum,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a Cisco Discovery Protocol (CDP) header. Cisco Systems designed CDP
 * to aid in the network management of adjacent Cisco devices. The CDP protocol
 * specifies data by using a type/length/value (TLV) setup. The first TLV can
 * specified by using the functions type, length, and value arguments. To
 * specify additional TLVs, the programmer could either use the payload 
 * interface or libnet_build_data() to construct them.
 * @param version CDP version
 * @param ttl time to live (time information should be cached by recipient)
 * @param sum checksum (0 for libnet to autofill)
 * @param type type of data contained in value
 * @param value_s length of value argument
 * @param value the CDP information string
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_cdp(uint8_t version, uint8_t ttl, uint16_t sum, uint16_t type,
uint16_t value_s, const uint8_t *value, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Control Message Protocol (ICMP)
 * echo request/reply header
 * @param type type of ICMP packet (should be ICMP_ECHOREPLY or ICMP_ECHO)
 * @param code code of ICMP packet (should be 0)
 * @param sum checksum (0 for libnet to autofill)
 * @param id identification number
 * @param seq packet sequence number
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_echo(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Control Message Protocol (ICMP)
 * IP netmask request/reply header.
 * @param type type of ICMP packet (should be ICMP_MASKREQ or ICMP_MASKREPLY)
 * @param code code of ICMP packet (should be 0)
 * @param sum checksum (0 for libnet to autofill)
 * @param id identification number
 * @param seq packet sequence number
 * @param mask subnet mask
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_mask(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, uint32_t mask, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Control Message Protocol (ICMP)
 * unreachable header. The IP header that caused the error message should be 
 * built by a previous call to libnet_build_ipv4().
 * @param type type of ICMP packet (should be ICMP_UNREACH)
 * @param code code of ICMP packet (should be one of the 16 unreachable codes)
 * @param sum checksum (0 for libnet to autofill)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_unreach(uint8_t type, uint8_t code, uint16_t sum,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Message Control Protocol (ICMP) 
 * redirect header.  The IP header that caused the error message should be 
 * built by a previous call to libnet_build_ipv4().
 * @param type type of ICMP packet (should be ICMP_REDIRECT)
 * @param code code of ICMP packet (should be one of the four redirect codes)
 * @param sum checksum (0 for libnet to autofill)
 * @param gateway
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_redirect(uint8_t type, uint8_t code, uint16_t sum,
uint32_t gateway, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Control Message Protocol (ICMP) time
 * exceeded header.  The IP header that caused the error message should be 
 * built by a previous call to libnet_build_ipv4().
 * @param type type of ICMP packet (should be ICMP_TIMXCEED)
 * @param code code of ICMP packet (ICMP_TIMXCEED_INTRANS / ICMP_TIMXCEED_REASS)
 * @param sum checksum (0 for libnet to autofill)
 * @param payload optional payload or NULL
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_timeexceed(uint8_t type, uint8_t code, uint16_t sum,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 4 RFC 792 Internet Control Message Protocol (ICMP)
 * timestamp request/reply header.
 * @param type type of ICMP packet (should be ICMP_TSTAMP or ICMP_TSTAMPREPLY)
 * @param code code of ICMP packet (should be 0)
 * @param sum checksum (0 for libnet to autofill)
 * @param id identification number
 * @param seq sequence number
 * @param otime originate timestamp
 * @param rtime receive timestamp
 * @param ttime transmit timestamp
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv4_timestamp(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, uint32_t otime, uint32_t rtime, uint32_t ttime,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IP version 6 RFC 4443 Internet Control Message Protocol (ICMP)
 * unreachable header. The IP header that caused the error message should be 
 * built by a previous call to libnet_build_ipv6().
 * @param type type of ICMP packet (should be ICMP6_UNREACH)
 * @param code code of ICMP packet (should be one of the 5 unreachable codes)
 * @param sum checksum (0 for libnet to autofill)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_icmpv6_unreach(u_int8_t type, u_int8_t code, u_int16_t sum,
u_int8_t *payload, u_int32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1112 Internet Group Memebership Protocol (IGMP) header.
 * @param type packet type
 * @param code packet code (should be 0)
 * @param sum checksum (0 for libnet to autofill)
 * @param ip IPv4 address
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_igmp(uint8_t type, uint8_t code, uint16_t sum, uint32_t ip,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a version 4 RFC 791 Internet Protocol (IP) header.
 *
 * @param ip_len total length of the IP packet including all subsequent data (subsequent
 *   data includes any IP options and IP options padding)
 * @param tos type of service bits
 * @param id IP identification number
 * @param frag fragmentation bits and offset
 * @param ttl time to live in the network
 * @param prot upper layer protocol
 * @param sum checksum (0 for libnet to autofill)
 * @param src source IPv4 address (little endian)
 * @param dst destination IPv4 address (little endian)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t 
libnet_build_ipv4(uint16_t ip_len, uint8_t tos, uint16_t id, uint16_t frag,
uint8_t ttl, uint8_t prot, uint16_t sum, uint32_t src, uint32_t dst,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an version 4 Internet Protocol (IP) options header. The function 
 * expects options to be a valid IP options string of size options_s, no larger
 * than 40 bytes (the maximum size of an options string).
 *
 * When building a chain, the options must be built, then the IPv4 header.
 *
 * When updating a chain, if the block following the options is an IPv4 header,
 * it's total length and header length will be updated if the options block
 * size changes.
 *
 * @param options byte string of IP options (it will be padded up to be an integral
 *   multiple of 32-bit words).
 * @param options_s length of options string
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t 
libnet_build_ipv4_options(const uint8_t *options, uint32_t options_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Autobuilds a version 4 Internet Protocol (IP) header. The function is useful
 * to build an IP header quickly when you do not need a granular level of
 * control. The function takes the same len, prot, and dst arguments as 
 * libnet_build_ipv4(). The function does not accept a ptag argument, but it
 * does return a ptag. In other words, you can use it to build a new IP header
 * but not to modify an existing one.
 * @param len total length of the IP packet including all subsequent data
 * @param prot upper layer protocol
 * @param dst destination IPv4 address (little endian)
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_ipv4(uint16_t len, uint8_t prot, uint32_t dst, libnet_t *l);

/**
 * Builds a version 6 RFC 2460 Internet Protocol (IP) header.
 * @param tc traffic class
 * @param fl flow label
 * @param len total length of the IP packet
 * @param nh next header
 * @param hl hop limit
 * @param src source IPv6 address
 * @param dst destination IPv6 address
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipv6(uint8_t tc, uint32_t fl, uint16_t len, uint8_t nh,
uint8_t hl, struct libnet_in6_addr src, struct libnet_in6_addr dst, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a version 6 RFC 2460 Internet Protocol (IP) fragmentation header.
 * @param nh next header
 * @param reserved unused value... OR IS IT!
 * @param frag fragmentation bits (ala ipv4)
 * @param id packet identification
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipv6_frag(uint8_t nh, uint8_t reserved, uint16_t frag,
uint32_t id, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds a version 6 RFC 2460 Internet Protocol (IP) routing header. This
 * function is special in that it uses the payload interface to include the 
 * "type-specific data"; that is the routing information. Most often this will
 * be a number of 128-bit IPv6 addresses. The application programmer will build
 * a byte string of IPv6 address and pass them to the function using the
 * payload interface.
 * @param nh next header
 * @param len length of the header in 8-byte octets not including the first 8 octets
 * @param rtype routing header type
 * @param segments number of routing segments that follow
 * @param payload optional payload of routing information
 * @param payload_s payload length
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipv6_routing(uint8_t nh, uint8_t len, uint8_t rtype,
uint8_t segments, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds a version 6 RFC 2460 Internet Protocol (IP) destination options
 * header. This function is special in that it uses the payload interface to
 * include the options data. The application programmer will build an IPv6 
 * options byte string and pass it to the function using the payload interface.
 * @param nh next header
 * @param len length of the header in 8-byte octets not including the first 8 octets
 * @param payload options payload
 * @param payload_s payload length
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipv6_destopts(uint8_t nh, uint8_t len, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a version 6 RFC 2460 Internet Protocol (IP) hop by hop options
 * header. This function is special in that it uses the payload interface to
 * include the options data. The application programmer will build an IPv6
 * hop by hop options byte string and pass it to the function using the payload
 * interface.
 * @param nh next header
 * @param len length of the header in 8-byte octets not including the first 8 octets
 * @param payload options payload
 * @param payload_s payload length
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipv6_hbhopts(uint8_t nh, uint8_t len, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * This function is not yet implement and is a NOOP.
 * @param len length
 * @param nh next header
 * @param dst destination IPv6 address
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_ipv6(uint16_t len, uint8_t nh, struct libnet_in6_addr dst,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a Cisco Inter-Switch Link (ISL) header.
 * @param dhost destination address (should be 01:00:0c:00:00)
 * @param type type of frame
 * @param user user defined data
 * @param shost source mac address
 * @param len total length of the encapuslated packet less 18 bytes
 * @param snap SNAP information (0xaaaa03 + vendor code)
 * @param vid 15 bit VLAN ID, 1 bit BPDU or CDP indicator
 * @param portindex port index
 * @param reserved used for FDDI and token ring
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_isl(uint8_t *dhost, uint8_t type, uint8_t user,
uint8_t *shost, uint16_t len, const uint8_t *snap, uint16_t vid,
uint16_t portindex, uint16_t reserved, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an Internet Protocol Security Encapsulating Security Payload header.
 * @param spi security parameter index
 * @param seq ESP sequence number
 * @param iv initialization vector
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipsec_esp_hdr(uint32_t spi, uint32_t seq, uint32_t iv,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an Internet Protocol Security Encapsulating Security Payload footer.
 * @param len padding length
 * @param nh next header
 * @param auth authentication data
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipsec_esp_ftr(uint8_t len, uint8_t nh, int8_t *auth,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an Internet Protocol Security Authentication header.
 * @param nh next header
 * @param len payload length
 * @param res reserved
 * @param spi security parameter index
 * @param seq sequence number
 * @param auth authentication data
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ipsec_ah(uint8_t nh, uint8_t len, uint16_t res,
uint32_t spi, uint32_t seq, uint32_t auth, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1035 version 4 DNS header. Additional DNS payload information
 * should be specified using the payload interface.
 * @param h_len
 * @param id DNS packet id
 * @param flags control flags
 * @param num_q number of questions
 * @param num_anws_rr number of answer resource records
 * @param num_auth_rr number of authority resource records
 * @param num_addi_rr number of additional resource records
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_dnsv4(uint16_t h_len, uint16_t id, uint16_t flags,
uint16_t num_q, uint16_t num_anws_rr, uint16_t num_auth_rr,
uint16_t num_addi_rr, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds a Routing Information Protocol header (RFCs 1058 and 2453).
 * @param cmd command
 * @param version protocol version
 * @param rd version one: 0, version two: routing domain
 * @param af address family
 * @param rt version one: 0, version two: route tag
 * @param addr IPv4 address
 * @param mask version one: 0, version two: subnet mask
 * @param next_hop version one: 0, version two: next hop address
 * @param metric routing metric
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_rip(uint8_t cmd, uint8_t version, uint16_t rd, uint16_t af,
uint16_t rt, uint32_t addr, uint32_t mask, uint32_t next_hop,
uint32_t metric, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds an Remote Procedure Call (Version 2) Call message header as
 * specified in RFC 1831. This builder provides the option for
 * specifying the record marking which is required when used with
 * streaming protocols (TCP).
 * @param rm record marking indicating the position in a stream, 0 otherwise
 * @param xid transaction identifier used to link calls and replies
 * @param prog_num remote program specification typically between 0 - 1fffffff
 * @param prog_vers remote program version specification
 * @param procedure procedure to be performed by remote program
 * @param cflavor authentication credential type
 * @param clength credential length (should be 0)
 * @param cdata opaque credential data (currently unused)
 * @param vflavor authentication verifier type
 * @param vlength verifier length (should be 0)
 * @param vdata opaque verifier data (currently unused)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_rpc_call(uint32_t rm, uint32_t xid, uint32_t prog_num,
uint32_t prog_vers, uint32_t procedure, uint32_t cflavor, uint32_t clength,
uint8_t *cdata, uint32_t vflavor, uint32_t vlength, const uint8_t *vdata,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IEEE 802.1d Spanning Tree Protocol (STP) configuration header.
 * STP frames are usually encapsulated inside of an 802.2 + 802.3 frame 
 * combination.
 * @param id protocol id
 * @param version protocol version
 * @param bpdu_type bridge protocol data unit type
 * @param flags flags
 * @param root_id root id
 * @param root_pc root path cost
 * @param bridge_id bridge id
 * @param port_id port id
 * @param message_age message age
 * @param max_age max age
 * @param hello_time hello time
 * @param f_delay forward delay
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_stp_conf(uint16_t id, uint8_t version, uint8_t bpdu_type,
uint8_t flags, const uint8_t *root_id, uint32_t root_pc, const uint8_t *bridge_id,
uint16_t port_id, uint16_t message_age, uint16_t max_age, 
uint16_t hello_time, uint16_t f_delay, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an IEEE 802.1d Spanning Tree Protocol (STP) topology change
 * notification header. STP frames are usually encapsulated inside of an
 * 802.2 + 802.3 frame combination.
 * @param id protocol id
 * @param version protocol version
 * @param bpdu_type bridge protocol data unit type
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_stp_tcn(uint16_t id, uint8_t version, uint8_t bpdu_type,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a token ring header.
 * @param ac access control
 * @param fc frame control
 * @param dst destination address
 * @param src source address
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param cf control field
 * @param oui Organizationally Unique Identifier
 * @param type upper layer protocol type
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_token_ring(uint8_t ac, uint8_t fc, const uint8_t *dst, const uint8_t *src,
uint8_t dsap, uint8_t ssap, uint8_t cf, const uint8_t *oui, uint16_t type,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Auto-builds a token ring header.
 * @param ac access control
 * @param fc frame control
 * @param dst destination address
 * @param dsap destination service access point
 * @param ssap source service access point
 * @param cf control field
 * @param oui Organizationally Unique Identifier
 * @param type upper layer protocol type
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_token_ring(uint8_t ac, uint8_t fc, const uint8_t *dst, 
uint8_t dsap, uint8_t ssap, uint8_t cf, const uint8_t *oui, uint16_t type,
libnet_t *l);

/**
 * Builds an RFC 2338 Virtual Router Redundacy Protool (VRRP) header. Use the
 * payload interface to specify address and autthentication information. To
 * build a "legal" packet, the destination IPv4 address should be the multicast  * address 224.0.0.18, the IP TTL should be set to 255, and the IP protocol
 * should be set to 112.
 * @param version VRRP version (should be 2)
 * @param type VRRP packet type (should be 1 -- ADVERTISEMENT)
 * @param vrouter_id virtual router identification
 * @param priority priority (higher numbers indicate higher priority)
 * @param ip_count number of IPv4 addresses contained in this advertisement
 * @param auth_type type of authentication (0, 1, 2 -- see RFC)
 * @param advert_int interval between advertisements
 * @param sum checksum (0 for libnet to autofill)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_vrrp(uint8_t version, uint8_t type, uint8_t vrouter_id,
uint8_t priority, uint8_t ip_count, uint8_t auth_type, uint8_t advert_int,
uint16_t sum, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds an RFC 3032 Multi-Protocol Label Switching (MPLS) header.
 * @param label 20-bit label value
 * @param experimental 3-bit reserved field
 * @param bos 1-bit bottom of stack identifier
 * @param ttl time to live
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_mpls(uint32_t label, uint8_t experimental, uint8_t bos,
uint8_t ttl, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds an RFC 958 Network Time Protocol (NTP) header.
 * @param leap_indicator the leap indicator
 * @param version NTP protocol version
 * @param mode NTP mode
 * @param stratum stratum
 * @param poll polling interval
 * @param precision precision
 * @param delay_int delay interval
 * @param delay_frac delay fraction
 * @param dispersion_int dispersion interval
 * @param dispersion_frac dispersion fraction
 * @param reference_id reference id
 * @param ref_ts_int reference timestamp integer
 * @param ref_ts_frac reference timestamp fraction
 * @param orig_ts_int original timestamp integer
 * @param orig_ts_frac original timestamp fraction
 * @param rec_ts_int receiver timestamp integer
 * @param rec_ts_frac reciever timestamp fraction
 * @param xmt_ts_int transmit timestamp integer
 * @param xmt_ts_frac transmit timestamp integer
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ntp(uint8_t leap_indicator, uint8_t version, uint8_t mode,
uint8_t stratum, uint8_t poll, uint8_t precision, uint16_t delay_int,
uint16_t delay_frac, uint16_t dispersion_int, uint16_t dispersion_frac,
uint32_t reference_id, uint32_t ref_ts_int, uint32_t ref_ts_frac,
uint32_t orig_ts_int, uint32_t orig_ts_frac, uint32_t rec_ts_int,
uint32_t rec_ts_frac, uint32_t xmt_ts_int, uint32_t xmt_ts_frac,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * @param len
 * @param type
 * @param rtr_id
 * @param area_id
 * @param sum
 * @param autype
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2(uint16_t len, uint8_t type, uint32_t rtr_id,
uint32_t area_id, uint16_t sum, uint16_t autype, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * @param netmask
 * @param interval
 * @param opts
 * @param priority
 * @param dead_int
 * @param des_rtr
 * @param bkup_rtr
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_hello(uint32_t netmask, uint16_t interval, uint8_t opts,
uint8_t priority, uint dead_int, uint32_t des_rtr, uint32_t bkup_rtr,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param dgram_len
 * @param opts
 * @param type
 * @param seqnum
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_dbd(uint16_t dgram_len, uint8_t opts, uint8_t type,
uint seqnum, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);
 
/**
 * @param type
 * @param lsid
 * @param advrtr
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsr(uint type, uint lsid, uint32_t advrtr,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param num
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsu(uint num, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * @param age
 * @param opts
 * @param type
 * @param lsid
 * @param advrtr
 * @param seqnum
 * @param sum
 * @param len
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsa(uint16_t age, uint8_t opts, uint8_t type,
uint lsid, uint32_t advrtr, uint seqnum, uint16_t sum, uint16_t len,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param flags
 * @param num
 * @param id
 * @param data
 * @param type
 * @param tos
 * @param metric
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsa_rtr(uint16_t flags, uint16_t num, uint id,
uint data, uint8_t type, uint8_t tos, uint16_t metric, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param nmask
 * @param rtrid
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsa_net(uint32_t nmask, uint rtrid, const uint8_t* payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param nmask
 * @param metric
 * @param tos
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsa_sum(uint32_t nmask, uint metric, uint tos,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);
 
/**
 * @param nmask
 * @param metric
 * @param fwdaddr
 * @param tag
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_ospfv2_lsa_as(uint32_t nmask, uint metric, uint32_t fwdaddr,
uint tag, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * Builds a generic libnet protocol header. This is useful for including an
 * optional payload to a packet that might need to change repeatedly inside
 * of a loop. This won't work for TCP or IP payload, they have special types
 * (this is probably a bug).
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_data(const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * @param opcode
 * @param htype
 * @param hlen
 * @param hopcount
 * @param xid
 * @param secs
 * @param flags
 * @param cip
 * @param yip
 * @param sip
 * @param gip
 * @param chaddr
 * @param sname
 * @param file
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_dhcpv4(uint8_t opcode, uint8_t htype, uint8_t hlen,
uint8_t hopcount, uint32_t xid, uint16_t secs, uint16_t flags,
uint32_t cip, uint32_t yip,  uint32_t sip, uint32_t gip, const uint8_t *chaddr,
uint8_t *sname, const uint8_t *file, const uint8_t* payload, uint32_t payload_s, 
libnet_t *l, libnet_ptag_t ptag);

/**
 * @param opcode
 * @param htype
 * @param hlen
 * @param hopcount
 * @param xid
 * @param secs
 * @param flags
 * @param cip
 * @param yip
 * @param sip
 * @param gip
 * @param chaddr
 * @param sname
 * @param file
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_bootpv4(uint8_t opcode, uint8_t htype, uint8_t hlen,
uint8_t hopcount, uint32_t xid, uint16_t secs, uint16_t flags,
uint32_t cip, uint32_t yip,  uint32_t sip, uint32_t gip, const uint8_t *chaddr,
uint8_t *sname, const uint8_t *file, const uint8_t* payload, uint32_t payload_s, 
libnet_t *l, libnet_ptag_t ptag);

/**
 * @param fv see libnet_build_gre().
 * @return size, see libnet_build_gre().
 */
uint32_t
libnet_getgre_length(uint16_t fv);

/**
 * Generic Routing Encapsulation (GRE - RFC 1701) is used to encapsulate any
 * protocol. Hence, the IP part of the packet is usually referred as "delivery
 * header". It is then followed by the GRE header and finally the encapsulated
 * packet (IP or whatever).
 * As GRE is very modular, the first GRE header describes the structure of the
 * header, using bits and flag to specify which fields will be present in the
 * header.
 * @param fv the 16 0 to 7: which fields are included in the header (checksum,
 *   seq. number, key, ...), bits 8 to 12: flag, bits 13 to 15: version.
 * @param type which protocol is encapsulated (PPP, IP, ...)
 * @param sum checksum (0 for libnet to autofill).
 * @param offset byte offset from the start of the routing field to the first byte of the SRE
 * @param key inserted by the encapsulator to authenticate the source
 * @param seq sequence number used by the receiver to sort the packets
 * @param len size of the GRE packet
 * @param payload
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_gre(uint16_t fv, uint16_t type, uint16_t sum,
uint16_t offset, uint32_t key, uint32_t seq, uint16_t len,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Generic Routing Encapsulation (GRE - RFC 1701) is used to encapsulate any
 * protocol. Hence, the IP part of the packet is usually referred as "delivery
 * header". It is then followed by the GRE header and finally the encapsulated
 * packet (IP or whatever).
 * As GRE is very modular, the first GRE header describes the structure of the
 * header, using bits and flag to specify which fields will be present in the
 * header.
 * @param fv the 16 0 to 7: which fields are included in the header (checksum, seq. number, key, ...), bits 8 to 12: flag, bits 13 to 15: version.
 * @param type which protocol is encapsulated (PPP, IP, ...)
 * @param sum checksum (0 for libnet to autofill).
 * @param offset byte offset from the start of the routing field to the first byte of the SRE
 * @param key inserted by the encapsulator to authenticate the source
 * @param seq sequence number used by the receiver to sort the packets
 * @param len size of the GRE packet
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_egre(uint16_t fv, uint16_t type, uint16_t sum,
uint16_t offset, uint32_t key, uint32_t seq, uint16_t len,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * @param af
 * @param offset
 * @param length
 * @param routing
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_gre_sre(uint16_t af, uint8_t offset, uint8_t length,
uint8_t *routing, const uint8_t* payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag);

/**
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_gre_last_sre(libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1771 Border Gateway Protocol 4 (BGP-4) header. The primary
 * function of a BGP speaking system is to exchange network reachability
 * information with other BGP systems. This network reachability information
 * includes information on the list of  Autonomous Systems (ASs) that
 * reachability information traverses.  This information is sufficient to
 * construct a graph of AS connectivity from which routing loops may be pruned
 * and some policy decisions at the AS level may be enforced.
 * This function builds the base BGP header which is used as a preamble before
 * any other BGP header. For example, a BGP KEEPALIVE message may be built with
 * only this function, while an error notification requires a subsequent call
 * to libnet_build_bgp4_notification.
 * @param marker a value the receiver can predict (if the message type is not BGP OPEN, or no authentication is used, these 16 bytes are normally set as all ones)
 * @param len total length of the BGP message, including the header
 * @param type type code of the message (OPEN, UPDATE, NOTIFICATION or KEEPALIVE)
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_bgp4_header(uint8_t marker[LIBNET_BGP4_MARKER_SIZE],
uint16_t len, uint8_t type, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1771 Border Gateway Protocol 4 (BGP-4) OPEN header. This is
 * the first message sent by each side of a BGP connection. The optional
 * parameters options should be constructed using the payload interface (see
 * RFC 1771 for the options structures).
 * @param version protocol version (should be set to 4)
 * @param src_as Autonomous System of the sender
 * @param hold_time used to compute the maximum allowed time between the receipt of KEEPALIVE, and/or UPDATE messages by the sender
 * @param bgp_id BGP identifier of the sender
 * @param opt_len total length of the  optional parameters field in bytes
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_bgp4_open(uint8_t version, uint16_t src_as, uint16_t hold_time,
uint32_t bgp_id, uint8_t opt_len, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1771 Border Gateway Protocol 4 (BGP-4) update header. Update
 * messages are used to transfer routing information between BGP peers.
 * @param unfeasible_rt_len indicates the length of the (next) "withdrawn routes" field in bytes
 * @param withdrawn_rt list of IP addresses prefixes for the routes that are being withdrawn; each IP address prefix is built as a 2-tuple <length (1 byte), prefix (variable)>
 * @param total_path_attr_len indicates the length of the (next) "path attributes" field in bytes
 * @param path_attributes each attribute is a 3-tuple <type (2 bytes), length, value>
 * @param info_len indicates the length of the (next) "network layer reachability information" field in bytes (needed for internal memory size calculation)
 * @param reachability_info 2-tuples <length (1 byte), prefix (variable)>.
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_bgp4_update(uint16_t unfeasible_rt_len, const uint8_t *withdrawn_rt,
uint16_t total_path_attr_len, const uint8_t *path_attributes, uint16_t info_len,
uint8_t *reachability_info, const uint8_t* payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds an RFC 1771 Border Gateway Protocol 4 (BGP-4) notification header.
 * A NOTIFICATION message is sent when an error condition is detected. Specific
 * error information may be passed through the payload interface.
 * @param err_code type of notification
 * @param err_subcode more specific information about the reported error.
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_bgp4_notification(uint8_t err_code, uint8_t err_subcode,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a Sebek header. The Sebek protocol was designed by the Honeynet
 * Project as a transport mechanism for post-intrusion forensic data. More
 * information may be found here: http://www.honeynet.org/papers/sebek.pdf.
 * @param magic identify packets that should be hidden 
 * @param version protocol version, currently 1 
 * @param type type of record (read data is type 0, write data is type 1) 
 * @param counter PDU counter used to identify when packet are lost 
 * @param time_sec seconds since EPOCH according to the honeypot 
 * @param time_usec residual microseconds 
 * @param pid PID 
 * @param uid UID 
 * @param fd FD 
 * @param cmd 12 first characters of the command 
 * @param length length in bytes of the PDU's body 
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_sebek(uint32_t magic, uint16_t version, uint16_t type, 
uint32_t counter, uint32_t time_sec, uint32_t time_usec, uint32_t pid,
uint32_t uid, uint32_t fd, uint8_t cmd[SEBEK_CMD_LENGTH], uint32_t length, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a HSRP header. HSRP is a Cisco propietary protocol defined in
 * RFC 2281
 * @param version version of the HSRP messages
 * @param opcode type of message
 * @param state current state of the router
 * @param hello_time period in seconds between hello messages
 * @param hold_time seconds that the current hello message is valid
 * @param priority priority for the election proccess
 * @param group standby group
 * @param reserved reserved field
 * @param authdata password
 * @param virtual_ip virtual ip address
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_hsrp(uint8_t version, uint8_t opcode, uint8_t state, 
uint8_t hello_time, uint8_t hold_time, uint8_t priority, uint8_t group,
uint8_t reserved, uint8_t authdata[HSRP_AUTHDATA_LENGTH], uint32_t virtual_ip,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Builds a link layer header for an initialized l. The function
 * determines the proper link layer header format from how l was initialized.
 * The function current supports Ethernet and Token Ring link layers.
 * @param dst the destination MAC address
 * @param src the source MAC address
 * @param oui Organizationally Unique Identifier (unused for Ethernet)
 * @param type the upper layer protocol type
 * @param payload optional payload or NULL
 * @param payload_s payload length or 0
 * @param l pointer to a libnet context
 * @param ptag protocol tag to modify an existing header, 0 to build a new one
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_build_link(const uint8_t *dst, const uint8_t *src, const uint8_t *oui, uint16_t type, 
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag);

/**
 * Automatically builds a link layer header for an initialized l. The function
 * determines the proper link layer header format from how l was initialized.
 * The function current supports Ethernet and Token Ring link layers.
 * @param dst the destination MAC address
 * @param oui Organizationally Unique Identifier (unused for Ethernet)
 * @param type the upper layer protocol type
 * @param l pointer to a libnet context
 * @return protocol tag value on success, -1 on error
 */
libnet_ptag_t
libnet_autobuild_link(const uint8_t *dst, const uint8_t *oui, uint16_t type,
libnet_t *l);

/**
 * Writes a prebuilt packet to the network. The function assumes that l was
 * previously initialized (via a call to libnet_init()) and that a 
 * previously constructed packet has been built inside this context (via one or
 * more calls to the libnet_build* family of functions) and is ready to go.
 * Depending on how libnet was initialized, the function will write the packet
 * to the wire either via the raw or link layer interface. The function will
 * also bump up the internal libnet stat counters which are retrievable via
 * libnet_stats().
 * @param l pointer to a libnet context
 * @return the number of bytes written, -1 on error
 */
int
libnet_write(libnet_t *l);

/**
 * Returns the IP address for the device libnet was initialized with. If
 * libnet was initialized without a device (in raw socket mode) the function
 * will attempt to find one. If the function fails and returns -1 a call to 
 * libnet_geterrror() will tell you why.
 * @param l pointer to a libnet context
 * @return a big endian IP address suitable for use in a libnet_build function or -1
 */

uint32_t
libnet_get_ipaddr4(libnet_t *l);

/**
 * This function is not yet implemented under IPv6.
 * @param l pointer to a libnet context
 * @return well, nothing yet
 */
struct libnet_in6_addr
libnet_get_ipaddr6(libnet_t *l);

/**
 * Returns the MAC address for the device libnet was initialized with. If
 * libnet was initialized without a device the function will attempt to find
 * one. If the function fails and returns NULL a call to libnet_geterror() will
 * tell you why.
 * @param l pointer to a libnet context
 * @return a pointer to the MAC address or NULL
 */
struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l);

/**
 * Takes a colon separated hexidecimal address (from the command line) and
 * returns a bytestring suitable for use in a libnet_build function. Note this
 * function performs an implicit malloc and the return value should be freed
 * after its use.
 * @param s the string to be parsed
 * @param len the resulting size of the returned byte string
 * @return a byte string or NULL on failure
 */
uint8_t *
libnet_hex_aton(const char *s, int *len);

/**
 * Returns the version of libnet.
 * @return the libnet version
 */
const char *
libnet_version(void);

/**
 * [Advanced Interface]
 * Yanks a prebuilt, wire-ready packet from the given libnet context. If
 * libnet was configured to do so (which it is by default) the packet will have
 * all checksums written in. This function is part of the advanced interface
 * and is only available when libnet is initialized in advanced mode. It is
 * important to note that the function performs an implicit malloc() and a
 * corresponding call to libnet_adv_free_packet() should be made to free the
 * memory packet occupies. If the function fails libnet_geterror() can tell you
 * why.
 * @param l pointer to a libnet context
 * @param packet will contain the wire-ready packet
 * @param packet_s will contain the packet size
 * @return 1 on success, -1 on failure  
 */
int
libnet_adv_cull_packet(libnet_t *l, uint8_t **packet, uint32_t *packet_s);

/**
 * [Advanced Interface] 
 * Pulls the header from the specified ptag from the given libnet context. This
 * function is part of the advanced interface and is only available when libnet
 * is initialized in advanced mode. If the function fails libnet_geterror() can
 * tell you why.
 * @param l pointer to a libnet context
 * @param ptag the ptag referencing the header to pull
 * @param header will contain the header
 * @param header_s will contain the header size
 * @return 1 on success, -1 on failure  
 */
int
libnet_adv_cull_header(libnet_t *l, libnet_ptag_t ptag, uint8_t **header,
uint32_t *header_s);

/**
 * [Advanced Interface] 
 * Writes a packet the network at the link layer. This function is useful to
 * write a packet that has been constructed by hand by the application
 * programmer or, more commonly, to write a packet that has been returned by
 * a call to libnet_adv_cull_packet(). This function is part of the advanced
 * interface and is only available when libnet is initialized in advanced mode.
 * If the function fails libnet_geterror() can tell you why.
 * @param l pointer to a libnet context
 * @param packet a pointer to the packet to inject
 * @param packet_s the size of the packet
 * @return the number of bytes written, or -1 on failure
 */
int
libnet_adv_write_link(libnet_t *l, const uint8_t *packet, uint32_t packet_s);

/**
 * [Advanced Interface] 
 * Writes a packet the network at the raw socket layer. This function is useful
 * to write a packet that has been constructed by hand by the application
 * programmer or, more commonly, to write a packet that has been returned by
 * a call to libnet_adv_cull_packet(). This function is part of the advanced
 * interface and is only available when libnet is initialized in advanced mode.
 * If the function fails libnet_geterror() can tell you why.
 * @param l pointer to a libnet context
 * @param packet a pointer to the packet to inject
 * @param packet_s the size of the packet
 * @return the number of bytes written, or -1 on failure
 */
int
libnet_adv_write_raw_ipv4(libnet_t *l, const uint8_t *packet, uint32_t packet_s);

/**
 * [Advanced Interface] 
 * Frees the memory allocated when libnet_adv_cull_packet() is called.
 * @param l pointer to a libnet context
 * @param packet a pointer to the packet to free
 */
void
libnet_adv_free_packet(libnet_t *l, uint8_t *packet);

/**
 * [Context Queue] 
 * Adds a new context to the libnet context queue. If no queue exists, this
 * function will create the queue and add the specified libnet context as the
 * first entry on the list. The functions checks to ensure niether l nor label
 * are NULL, and that label doesn't refer to an existing context already in the
 * queue. Additionally, l should refer to a libnet context previously
 * initialized with a call to libnet_init(). If the context queue in write
 * locked, this function will fail.
 * @param l pointer to a libnet context
 * @param label a canonical name given to recognize the new context, no longer than LIBNET_LABEL_SIZE
 * @return 1 on success, -1 on failure
*/
int 
libnet_cq_add(libnet_t *l, char *label);

/**
 * [Context Queue] 
 * Removes a specified context from the libnet context queue by specifying the
 * libnet context pointer. Note the function will remove the specified context
 * from the context queue and cleanup internal memory from the queue, it is up
 * to the application programmer to free the returned libnet context with a
 * call to libnet_destroy(). Also, as it is not necessary to keep the libnet
 * context pointer when initially adding it to the context queue, most
 * application programmers will prefer to refer to entries on the context
 * queue by canonical name and would use libnet_cq_remove_by_label(). If the
 * context queue is write locked, this function will fail.
 * @param l pointer to a libnet context
 * @return the pointer to the removed libnet context, NULL on failure
 */
libnet_t *
libnet_cq_remove(libnet_t *l);

/** 
 * [Context Queue] 
 * Removes a specified context from the libnet context queue by specifying the
 * canonical name. Note the function will remove the specified context from
 * the context queue and cleanup internal memory from the queue, it is up to 
 * the application programmer to free the returned libnet context with a call
 * to libnet_destroy(). If the context queue is write locked, this function
 * will fail.
 * @param label canonical name of the context to remove
 * @return the pointer to the removed libnet context, NULL on failure
 */   
libnet_t *
libnet_cq_remove_by_label(char *label);
 
/**
 * [Context Queue] 
 * Returns the canonical label associated with the context.
 * @param l pointer to a libnet context
 * @return pointer to the libnet context's label
 */   
const char *
libnet_cq_getlabel(libnet_t *l);
 
/**
 * [Context Queue] 
 * Locates a libnet context from the queue, indexed by a canonical label.
 * @param label canonical label of the libnet context to retrieve
 * @return the expected libnet context, NULL on failure
 */
libnet_t *
libnet_cq_find_by_label(char *label);
  
/**
 * [Context Queue] 
 * Destroys the entire context queue, calling libnet_destroy() on each
 * member context.
 */
void
libnet_cq_destroy(void);

/**
 * [Context Queue] 
 * Intiailizes the interator interface and set a write lock on the entire
 * queue. This function is intended to be called just prior to interating
 * through the entire list of contexts (with the probable intent of inject a
 * series of packets in rapid succession). This function is often used as
 * per the following:
 *
 *    for (l = libnet_cq_head(); libnet_cq_last(); l = libnet_cq_next())
 *    {
 *         ...
 *    }
 *
 * Much of the time, the application programmer will use the iterator as it is
 * written above; as such, libnet provides a macro to do exactly that,
 * for_each_context_in_cq(l). Warning: do not call the iterator more than once
 * in a single loop.
 * @return the head of the context queue
 */
libnet_t *
libnet_cq_head(void);

/**
 * [Context Queue] 
 * Check whether the iterator is at the last context in the queue.
 * @return 1 if at the end of the context queue, 0 otherwise
 */
int
libnet_cq_last(void);

/**
 * [Context Queue] 
 * Get next context from the context queue.
 * @return the next context from the context queue
 */
libnet_t *
libnet_cq_next(void);

/**
 * [Context Queue] 
 * Function returns the number of libnet contexts that are in the queue.
 * @return the number of libnet contexts currently in the queue
 */
uint32_t
libnet_cq_size(void);

/**
 * [Context Queue]
 */
uint32_t
libnet_cq_end_loop(void);

/**
 * [Diagnostic] 
 * Prints the contents of the given context.
 * @param l pointer to a libnet context
 */
void
libnet_diag_dump_context(libnet_t *l);

/**
 * [Diagnostic] 
 * Prints the contents of every pblock.
 * @param l pointer to a libnet context
 */
void
libnet_diag_dump_pblock(libnet_t *l);

/**
 * [Diagnostic] 
 * Returns the canonical name of the pblock type.
 * @param type pblock type
 * @return a string representing the pblock type type or "unknown" for an unknown value
 */
char *
libnet_diag_dump_pblock_type(uint8_t type);

/**
 * [Diagnostic] 
 * Function prints the contents of the supplied buffer to the supplied
 * stream pointer. Will swap endianness based disposition of mode variable.
 * Useful to be used in conjunction with the advanced interface and a culled
 * packet.
 * @param packet the packet to print
 * @param len length of the packet in bytes
 * @param swap 1 to swap byte order, 0 to not.
 *   Counter-intuitively, it is necessary to swap in order to see the byte
 *   order as it is on the wire (this may be a bug).
 * @param stream a stream pointer to print to
 */
void
libnet_diag_dump_hex(const uint8_t *packet, uint32_t len, int swap, FILE *stream);

/*
 * [Internal] 
 */
int
libnet_write_raw_ipv4(libnet_t *l, const uint8_t *packet, uint32_t size);

/*
 * [Internal] 
 */
int
libnet_write_raw_ipv6(libnet_t *l, const uint8_t *packet, uint32_t size);

/*
 * [Internal] 
 */
int
libnet_write_link(libnet_t *l, const uint8_t *packet, uint32_t size);

#if ((__WIN32__) && !(__CYGWIN__))
/*
 * [Internal] 
 */
SOCKET
libnet_open_raw4(libnet_t *l);
#else
/*
 * [Internal] 
 */
int
libnet_open_raw4(libnet_t *l);
#endif

/*
 * [Internal] 
 */
int
libnet_close_raw4(libnet_t *l);

/*
 * [Internal] 
 */
int
libnet_open_raw6(libnet_t *l);
       
/*
 * [Internal] 
 */
int
libnet_close_raw6(libnet_t *l);

/*
 * [Internal] 
 */
int
libnet_select_device(libnet_t *l);

/*
 * [Internal] 
 */
int
libnet_open_link(libnet_t *l);

/*
 * [Internal] 
 */
int
libnet_close_link(libnet_t *l);

/*
 * [Internal]
 *   THIS FUNCTION IS BROKEN. IT WILL SEGFAULT OR CORRUPT MEMORY, OR JUST SILENTLY DO THE
 *   WRONG THING IF NOT CALLED CORRECTLY, AND CALLING IT CORRECTLY IS UNDOCUMENTED, AND
 *   ALMOST IMPOSSIBLE. YOU HAVE BEEN WARNED.
 */
int
libnet_do_checksum(libnet_t *l, uint8_t *iphdr, int protocol, int h_len);

/* Calculate internet checksums.
 *
 * IP (TCP, UDP, IGMP, ICMP, etc...) checksums usually need information from
 * the IP header to construct the "pseudo header", this function takes a
 * pointer to that header, the buffer boundaries, the "h_len" (see pblock_t for
 * a description, it is increasinly unused, though, and I'm trying to remove it
 * altogether), and the protocol number for the protocol that is to be
 * checksummed.
 *
 * Finding that protocol requires that the IP header be well-formed... so this
 * won't work well for invalid packets. But then, what is the valid checksum
 * for a valid packet, anyhow?
 *
 * This doesn't work well for non-inet checksums, sorry, that's an original design
 * flaw. pblock_t needs a pointer in it, to a packet assembly function that can be
 * called at runtime to do assembly and checksumming.
 */
int
libnet_inet_checksum(libnet_t *l, uint8_t *iphdr, int protocol, int h_len, const uint8_t *beg, const uint8_t * end);

/*
 * [Internal] 
 */
uint32_t
libnet_compute_crc(uint8_t *buf, uint32_t len);

/*
 * [Internal] 
 */
uint16_t
libnet_ip_check(uint16_t *addr, int len);

/*
 * [Internal] 
 */
int
libnet_in_cksum(uint16_t *addr, int len);

/*
 * [Internal] 
 * If ptag is 0, function will create a pblock for the protocol unit type,
 * append it to the list and return a pointer to it.  If ptag is not 0,
 * function will search the pblock list for the specified protocol block 
 * and return a pointer to it.
 */
libnet_pblock_t *
libnet_pblock_probe(libnet_t *l, libnet_ptag_t ptag, uint32_t b_len, 
uint8_t type);

/*
 * [Internal] 
 * Function creates the pblock list if l->protocol_blocks == NULL or appends
 * an entry to the doubly linked list.
 */
libnet_pblock_t *
libnet_pblock_new(libnet_t *l, uint32_t b_len);

/*
 * [Internal] 
 * Function swaps two pblocks in memory.
 */
int
libnet_pblock_swap(libnet_t *l, libnet_ptag_t ptag1, libnet_ptag_t ptag2);

/*
 * [Internal] 
 * Function inserts ptag2 before ptag1 in the doubly linked list.
 */
int
libnet_pblock_insert_before(libnet_t *l, libnet_ptag_t ptag1,
libnet_ptag_t ptag2);

/*
 * [Internal] 
 * Function removes a pblock from context 
 */
void
libnet_pblock_delete(libnet_t *l, libnet_pblock_t *p);

/*
 * [Internal] 
 * Function updates the pblock meta-inforation.  Internally it updates the
 * ptag with a monotonically increasing variable kept in l.  This way each
 * pblock has a succesively increasing ptag identifier.
 */
libnet_ptag_t
libnet_pblock_update(libnet_t *l, libnet_pblock_t *p, uint32_t h, uint8_t type);


/*
 * [Internal] 
 * Function locates a given block by it's ptag. 
 */
libnet_pblock_t *
libnet_pblock_find(libnet_t *l, libnet_ptag_t ptag);

/*
 * [Internal] 
 * Function copies protocol block data over.
 */
int
libnet_pblock_append(libnet_t *l, libnet_pblock_t *p, const uint8_t *buf,
uint32_t len);

/*
 * [Internal] 
 * Function sets pblock flags.
 */
void
libnet_pblock_setflags(libnet_pblock_t *p, uint8_t flags);

/*
 * [Internal] 
 * Function returns the protocol number for the protocol block type.  If
 * the type is unknown, the function defaults to returning IPPROTO_IP.
 */
int
libnet_pblock_p2p(uint8_t type);

/*
 * [Internal] 
 * Function assembles the protocol blocks into a packet, checksums are
 * calculated if that was requested.
 */
int
libnet_pblock_coalesce(libnet_t *l, uint8_t **packet, uint32_t *size);

#if !(__WIN32__)
/*
 * [Internal] 
 * By testing if we can retrieve the FLAGS of an iface
 * we can know if it exists or not and if it is up.
 */
int
libnet_check_iface(libnet_t *l);
#endif

#if defined(__WIN32__)
/*
 * [Internal] 
 */
BYTE *
libnet_win32_get_remote_mac(libnet_t *l, DWORD IP);

/*
 * [Internal] 
 */
int
libnet_close_link_interface(libnet_t *l);

/*
 * [Internal] 
 */
BYTE * 
libnet_win32_read_arp_table(DWORD IP);
#endif
#endif  /* __LIBNET_FUNCTIONS_H */

/* EOF */
