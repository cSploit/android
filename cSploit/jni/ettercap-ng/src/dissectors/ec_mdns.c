/*
    ettercap -- dissector for multicast DNS

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>
#include <ec_dissect.h>
#include <ec_resolv.h>

FUNC_DECODER(dissector_mdns);
void mdns_init(void);

static void handle_ipv4_record(char* name, char* ptr);
static void handle_srv_record(char* name, char* port_ptr, struct packet_object *po);

static const char local_tcp[] = "._tcp.local";
static const char local_udp[] = "._udp.local";

/************************************************/

void __init mdns_init(void)
{
   dissect_add("mdns", APP_LAYER_UDP, 5353, dissector_mdns);
}

FUNC_DECODER(dissector_mdns)
{
    uint16_t questions = 0;
    uint16_t records = 0;
    unsigned char* start = NULL;
    char name[NS_MAXDNAME];
    DECLARE_DISP_PTR_END(ptr, end);

    // unused param supression
    (void)buf;
    (void)buflen;
    (void)len;

    // skip packets which are not useful
    if (PACKET->DATA.len <= 12)
       return NULL;

    PACKET->PASSIVE.flags |= FP_HOST_LOCAL;
    questions = ntohs(*((uint16_t*)ptr + 4));

    hook_point(HOOK_PROTO_MDNS, PACKET);

    if (questions > 0)
    {
        //skip packets with questions for now. Makes parsing easier
        return NULL;
    }

    records += ntohs(*((uint16_t*)(ptr + 6)));
    records += ntohs(*((uint16_t*)(ptr + 8)));
    records += ntohs(*((uint16_t*)(ptr + 10)));
    if (records == 0)
    {
        // there is nothing to parse
        return NULL;
    }

    if ((ptr + 12) >= end)
    {
        // not enough data
        return NULL;
    }

    // store the start for dns records pointers
    start = ptr;
    ptr += 12;

    // loop over all the records
    for ( ; records > 0; --records)
    {
        uint16_t type = 0;
        uint16_t length = 0;
        uint16_t name_len = dn_expand(start, end, ptr, name, sizeof(name));

        if ((ptr + name_len + 10) >= end)
        {
            return NULL;
        }

        ptr += name_len;

        type = ntohs(*((uint16_t*)(ptr)));
        length = ntohs(*((uint16_t*)(ptr + 8)));

        if ((ptr + 10 + length) >= end)
        {
            return NULL;
        }

        ptr += 10;

        switch (type)
        {
            case 1: // A host IPv4
                handle_ipv4_record(name, (char*)ptr);
                break;
            case 28: // AAA (Host IPv6 Address)
                //TODO the resolve cache doesn't handle ipv6
                break;
            case 33: // Service
                handle_srv_record(name, (char*)ptr + 4, PACKET);
                break;
            default:
                //various other types should hit here: TXT, HINFO, etc.
                break;
        }

        // skip over the rest of the record
        ptr += length;
    }

    return NULL;
}

static void handle_ipv4_record(char* name, char* ptr)
{
    uint32_t addr;
    struct ip_addr ip;
    NS_GET32(addr, ptr);
    addr = htonl(addr);
    ip_addr_init(&ip, AF_INET, (u_char *)&addr);

    /* insert the answer in the resolv cache */
    resolv_cache_insert(&ip, name);
}

static void handle_srv_record(char* name, char* port_ptr, struct packet_object *po)
{
    uint16_t port;
    NS_GET16(port, port_ptr);
    port = ntohs(port);

    if (strlen(name) > sizeof(local_tcp))
    {
        int name_offset = strlen(name) - (sizeof(local_tcp) - 1);
        if (strncmp(name + name_offset, local_tcp, sizeof(local_tcp) - 1) == 0)
        {
            PACKET->DISSECTOR.advertised_proto = NL_TYPE_TCP;
        }
        else if (strncmp(name + name_offset, local_udp, sizeof(local_udp) - 1) == 0)
        {
            PACKET->DISSECTOR.advertised_proto = NL_TYPE_UDP;
        }

        PACKET->DISSECTOR.advertised_port = port;
    }
}
