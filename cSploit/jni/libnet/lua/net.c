/*
Copyright (C) 2010 Wurldtech Security Technologies All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <time.h>

#include "dnet.h"
#include "libnet_decode.h"
#include <libnet.h>


#define LUA_LIB /* To pull in LUA_API definitions for modules */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#undef NET_DUMP

/*
Get field from arg table, errors if argt is not a table, returns
0 if field not found, otherwise pushes field value and returns
index.
*/
static int v_arg(lua_State* L, int argt, const char* field)
{
    luaL_checktype(L, argt, LUA_TTABLE);

    lua_getfield(L, argt, field);

    if(lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    return lua_gettop(L);
}

static const char* v_arg_lstring(lua_State* L, int argt, const char* field, size_t* size, const char* def)
{
    if(!v_arg(L, argt, field))
    {
        if(def) {
            lua_pushstring(L, def);
            return lua_tolstring(L, -1, size);
        } else {
            const char* msg = lua_pushfstring(L, "%s is missing", field);
            luaL_argerror(L, argt, msg);
        }
    }

    if(!lua_tostring(L, -1)) {
        const char* msg = lua_pushfstring(L, "%s is not a string", field);
        luaL_argerror(L, argt, msg);
    }

    return lua_tolstring(L, -1, size);
}

static const char* v_arg_string(lua_State* L, int argt, const char* field)
{
    return v_arg_lstring(L, argt, field, NULL, NULL);
}

static int v_arg_integer_get_(lua_State* L, int argt, const char* field)
{
    if(lua_type(L, -1) != LUA_TNUMBER) {
        const char* msg = lua_pushfstring(L, "%s is not an integer", field);
        luaL_argerror(L, argt, msg);
    }

    return lua_tointeger(L, -1);
}

static int v_arg_integer(lua_State* L, int argt, const char* field)
{
    if(!v_arg(L, argt, field))
    {
        const char* msg = lua_pushfstring(L, "%s is missing", field);
        luaL_argerror(L, argt, msg);
    }

    return v_arg_integer_get_(L, argt, field);
}

static int v_arg_integer_opt(lua_State* L, int argt, const char* field, int def)
{
    if(!v_arg(L, argt, field))
    {
        lua_pushinteger(L, def);
        return lua_tointeger(L, -1);
    }

    return v_arg_integer_get_(L, argt, field);
}

static void v_obj_metatable(lua_State* L, const char* regid, const struct luaL_reg methods[])
{
    /* metatable = { ... methods ... } */
    luaL_newmetatable(L, regid);
    luaL_register(L, NULL, methods);
    /* metatable["__index"] = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}




#define L_NET_REGID "wt.net"


static int net_error(lua_State* L, libnet_t* l)
{
    return luaL_error(L, "%s", libnet_geterror(l));
}

/* TODO I really regret not using the standard nil,emsg return convention... */
static int check_error(lua_State* L, libnet_t* l, int r)
{
    if(r < 0) {
        return net_error(L, l);
    }
    return r;
}

static int pushptag(lua_State* L, libnet_t* l, int ptag)
{
    check_error(L, l, ptag);
    lua_pushinteger(L, ptag);
    return 1;
}

static uint32_t check_ip_pton(lua_State* L, const char* p, const char* name)
{
    uint32_t n = 0;
    if(ip_pton(p, &n) < 0) {
        return luaL_error(L, "ip_pton failed on %s '%s'", name, p);
    }
    return n;
}

static eth_addr_t check_eth_pton(lua_State* L, const char* p, const char* name)
{
    eth_addr_t n;
    if(eth_pton(p, &n) < 0) {
        luaL_error(L, "eth_pton failed on %s '%s'", name, p);
    }
    return n;
}

static const char* pushistring(lua_State* L, const char* fmt, int i)
{
    char formatted[128];
    snprintf(formatted, sizeof(formatted), fmt, i);
    formatted[sizeof(formatted)-1] = '\0';
    return lua_pushfstring(L, formatted);
}

/* check ptag argument is zero, or refers to a pblock of the expected type */
static int lnet_arg_ptag(lua_State* L, libnet_t* ud, int targ, int type)
{
    int ptag = v_arg_integer_opt(L, targ, "ptag", LIBNET_PTAG_INITIALIZER);

    if(ptag) {
        libnet_pblock_t* pblock = libnet_pblock_find(ud, ptag);
        luaL_argcheck(L, pblock, targ,
                lua_pushfstring(L, "ptag %d cannot be found", ptag));
        luaL_argcheck(L, pblock->type == type, targ,
                lua_pushfstring(L, "ptag %d of type %s/%s is not %s/%s",
                ptag,
                pushistring(L, "%#x", pblock->type), libnet_diag_dump_pblock_type(pblock->type),
                pushistring(L, "%#x", type), libnet_diag_dump_pblock_type(type)
                ));
    }

    return ptag;
}

static libnet_t* checkudata(lua_State* L)
{
    libnet_t** ud = luaL_checkudata(L, 1, L_NET_REGID);

    luaL_argcheck(L, *ud, 1, "net has been destroyed");

    return *ud;
}

static libnet_pblock_t* checkpblock(lua_State* L, libnet_t* ud, int narg)
{
    int ptag = luaL_checkint(L, narg);
    libnet_pblock_t* pblock = libnet_pblock_find(ud, ptag);
    luaL_argcheck(L, pblock, narg, "ptag cannot be found");
    return pblock;
}

static const uint8_t*
checklbuffer(lua_State* L, int argt, const char* field, uint32_t* size)
{
    size_t payloadsz = 0;
    const char* payload = v_arg_lstring(L, argt, field, &payloadsz, "");

    if(payloadsz == 0) {
        payload = NULL;
    }

    *size = payloadsz;

    return (const uint8_t*) payload;
}

static const uint8_t*
checkpayload(lua_State* L, int argt, uint32_t* size)
{
  return checklbuffer(L, argt, "payload", size);
}

static int pushtagornil(lua_State* L, libnet_pblock_t* pblock)
{
    if(pblock)
        lua_pushinteger(L, pblock->ptag);
    else
        lua_pushnil(L);
    return 1;
}

static void
setintfield(lua_State* L, int tindex, const char* field, int i)
{
    lua_pushinteger(L, i);
    lua_setfield(L, tindex, field);
}
static void
setui32field(lua_State* L, int tindex, const char* field, uint32_t i)
{
    lua_pushnumber(L, i);
    lua_setfield(L, tindex, field);
}

static void
setnsintfield(lua_State* L, int tindex, const char* field, uint16_t i)
{
    setintfield(L, tindex, field, ntohs(i));
}

static void
setnlintfield(lua_State* L, int tindex, const char* field, uint32_t i)
{
    setui32field(L, tindex, field, ntohl(i));
}

static void
setlstringfield(lua_State* L, int tindex, const char* field, const void* lstr, size_t lsiz)
{
    lua_pushlstring(L, lstr, lsiz);
    lua_setfield(L, tindex, field);
}

static void
setstringfield(lua_State* L, int tindex, const char* field, const char* str)
{
    lua_pushstring(L, str);
    lua_setfield(L, tindex, field);
}

static void
setipfield(lua_State* L, int tindex, const char* field, struct in_addr addr)
{
    char addrstr[INET_ADDRSTRLEN];
    lua_pushstring(L, inet_ntop(AF_INET, &addr, addrstr, sizeof(addrstr)));
    lua_setfield(L, tindex, field);
}

static void
setethfield(lua_State* L, int tindex, const char* field, const uint8_t addr[ETHER_ADDR_LEN])
{
    char addrstr[] = "11:22:33:44:55:66";
    eth_addr_t ethaddr;

    memcpy(ethaddr.data, addr, ETHER_ADDR_LEN);

    setstringfield(L, tindex, field, eth_ntop(&ethaddr, addrstr, sizeof(addrstr)));
}

static const char* type_string(u_int8_t type)
{
    return libnet_diag_dump_pblock_type(type);
}

/*-
-- net:destroy()

Manually destroy a net object, freeing it's resources (this will happen
on garbage collection if not done explicitly).
*/
static int lnet_destroy (lua_State *L)
{
    libnet_t** ud = luaL_checkudata(L, 1, L_NET_REGID);

    if(*ud)
        libnet_destroy(*ud);

    *ud = NULL;

    return 0;
}

/*-
-- net = net:clear()

Clear the current packet and all it's protocol blocks.

Return self.
*/
static int lnet_clear(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_clear_packet(ud);
    /* TODO - these bugs are fixed in libnet head */
    ud->n_pblocks = 0;
    ud->ptag_state = 0;
    return 1;
}

/*-
-- str = net:block([ptag])

Coalesce the protocol blocks into a single chunk, and return.

If a ptag is provided, just return data of that pblock (no checksums
will be calculated).
*/
/*
FIXME - we should just have a packet function, this fails on single blocks/ptags,
and the :pblock() method is better, anyway.
*/
static int lnet_block(lua_State* L)
{
    libnet_t* ud = checkudata(L);

    u_int32_t len;
    u_int8_t *packet = NULL;
    u_int8_t *block;
    int r;

    r = libnet_pblock_coalesce(ud, &packet, &len);

    check_error(L, ud, r);

    block = packet;

    if(!lua_isnoneornil(L, 2)) {
        libnet_pblock_t* end = checkpblock(L, ud, 2);
        libnet_pblock_t* p = ud->pblock_end;
        while(p != end) {
            block += p->b_len;
            p = p->prev;
        }
        assert(p == end);
        len = p->b_len;
    }

    lua_pushlstring(L, (char*) block, len);

    libnet_adv_free_packet(ud, packet);

    return 1;
}

/*-
-- net:pbuf(ptag)
*/
static int lnet_pbuf(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    int ptag = luaL_checkint(L, 2); /* checkpblock */
    const char* pbuf = (const char*)libnet_getpbuf(ud, ptag);
    size_t pbufsz = libnet_getpbuf_size(ud, ptag);

    if(!pbuf)
      return net_error(L, ud);

    lua_pushlstring(L, pbuf, pbufsz);

    return 1;
}

/*-
-- net:write()

Write the packet (which must previously have been built up inside the context).
*/
static int lnet_write(lua_State *L)
{
    libnet_t* ud = checkudata(L);

#ifdef NET_DUMP
    lnet_dump(L);
#endif

    int r = libnet_write(ud);
    check_error(L, ud, r);
    lua_pushinteger(L, r);
    return 1;
}

/*-
-- size = net:write_link(link_pdu)

Writes link_pdu raw at the link layer, you are responsible for forming
the link-layer header.

Returns the size written on success.
*/
static int lnet_write_link (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    size_t payloadsz_ = 0;
    const char* payload_ = luaL_checklstring(L, 2, &payloadsz_);
    uint32_t payloadsz = (uint32_t) payloadsz_;
    const uint8_t* payload = (const uint8_t*) payload_;
    int size = libnet_write_link(ud, payload, payloadsz);
    check_error(L, ud, size);
    lua_pushinteger(L, size);
    return 1;
}

/*-
-- net:tag_below(ptag)

tag below ptag, or bottom tag if ptag is nil
*/
static int lnet_tag_below(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = NULL;
    
    if(lua_isnoneornil(L, 2)) {
        return pushtagornil(L, ud->pblock_end);
    }

    pblock = checkpblock(L, ud, 2);

    return pushtagornil(L, pblock->next);
}

/*-
-- net:tag_above(ptag)

tag above ptag, or top tag if ptag is nil
*/
static int lnet_tag_above(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = NULL;

    if(lua_isnoneornil(L, 2)) {
        return pushtagornil(L, ud->protocol_blocks);
    }

    pblock = checkpblock(L, ud, 2);

    return pushtagornil(L, pblock->prev);
}


/*- net:tag_type(ptag)

returns type of ptag, a string
*/

static int lnet_tag_type(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkpblock(L, ud, 2);
    lua_pushstring(L, libnet_diag_dump_pblock_type(pblock->type));
    return 1;
}

/*-
-- net:fd()

Get the fileno of the underlying file descriptor.
*/
static int lnet_getfd(lua_State* L)
{ 
    libnet_t* ud = checkudata(L);
    lua_pushinteger(L, libnet_getfd(ud));
    return 1;
}

/*-
-- net:device()

Get the device name, maybe be nil.
*/
static int lnet_getdevice(lua_State* L)
{ 
    libnet_t* ud = checkudata(L);
    const char* device = libnet_getdevice(ud);
    if(device)
      lua_pushstring(L, device);
    else
      lua_pushnil(L);

    return 1;
}

/* NOTE all these builders could return self, ptag, so that calls can be chained
 * n:udp{...}
 *  :ipv4{}
 *  :eth{}
 */

/*-
-- ptag = net:data{payload=STR, ptag=int}

Build generic data packet inside net context.

ptag is optional, defaults to creating a new protocol block
*/
static int lnet_data (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_DATA_H);

    ptag = libnet_build_data(payload, payloadsz, ud, ptag);
    check_error(L, ud, ptag);
    lua_pushinteger(L, ptag);
    return 1;
}

/*-
-- ptag = net:igmp{type=NUM, [reserved=NUM,] ip=IP, payload=STR, ptag=int}

Build IGMPv1 packet inside net context.

Reserved is optional, and should always be 0.

ptag is optional, defaults to creating a new protocol block
*/
/* TODO type should allow strings: query, report */
static int lnet_igmp (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    int type = v_arg_integer(L, 2, "type");
    int rsv = v_arg_integer_opt(L, 2, "reserved", 0);
    const char* ip = v_arg_string(L, 2, "ip");
    uint32_t ip_n = check_ip_pton(L, ip, "ip");
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int cksum = 0;
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_IGMP_H);

    ptag = libnet_build_igmp(type, rsv, cksum, ip_n, payload, payloadsz, ud, ptag);
    check_error(L, ud, ptag);
    lua_pushinteger(L, ptag);
    return 1;
}

/*-
-- ptag = net:udp{src=NUM, dst=NUM, len=NUM, payload=STR, ptag=int}

Build UDP packet inside net context.

ptag is optional, defaults to creating a new protocol block
*/
static int lnet_udp (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    int src = v_arg_integer(L, 2, "src");
    int dst = v_arg_integer(L, 2, "dst");
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int len = v_arg_integer_opt(L, 2, "len", LIBNET_UDP_H + payloadsz);
    int cksum = 0;
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_UDP_H);

    ptag = libnet_build_udp(src, dst, len, cksum, payload, payloadsz, ud, ptag);
    check_error(L, ud, ptag);
    lua_pushinteger(L, ptag);
    return 1;
}

/* TODO - add to libnet? */
static
libnet_pblock_t *
libnet_pblock_by_type(libnet_t* l, uint8_t type, libnet_pblock_t* p)
{
    /* TODO - search from end/link layer upwards? */
    if(!p)
        p = l->protocol_blocks;

    for (; p; p = p->next)
    {
        if (p->type == type)
        {
            return (p);
        }
    }
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): couldn't find protocol type\n", __func__);
    return NULL;
}

static
libnet_pblock_t*
checkptype(lua_State* L, libnet_t* l, uint8_t type)
{
    libnet_pblock_t* pblock = libnet_pblock_by_type(l, type, NULL);
    if(!pblock)
        luaL_error(L, "pblock type %s (%s) not found",
                pushistring(L, "%#x", type), libnet_diag_dump_pblock_type(type));
    return pblock;
}

/*-
-- argt = net:get_udp()

Get udp pblock argument table.

*/
/* TODO - optional ptag argument? */
static int lnet_get_udp(lua_State *L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkptype(L, ud, LIBNET_PBLOCK_UDP_H);
    struct libnet_udp_hdr* udp = (struct libnet_udp_hdr*) pblock->buf;
    uint8_t* data = pblock->buf + LIBNET_UDP_H;
    uint32_t datasz = 0;

    lua_newtable(L);
    setintfield(L, 2, "ptag", pblock->ptag);
    setnsintfield(L, 2, "src", udp->uh_sport);
    setnsintfield(L, 2, "dst", udp->uh_dport);
    setnsintfield(L, 2, "_len", udp->uh_ulen);
    setnsintfield(L, 2, "_sum", udp->uh_sum);

    if(pblock->b_len > LIBNET_UDP_H) {
        datasz = pblock->b_len - LIBNET_UDP_H;
    }

    setlstringfield(L, 2, "payload", data, (size_t)datasz);

    return 1;
}

/*-
-- ptag = n:tcp{
    -- required arguments
      src=port,
      dst=port,
      seq=int,
      flags=int,
      win=int,
    -- optional arguments
      ack=int,
      urg=int,
      ptag=int,
      payload=str,
      options=tcp_options,
  }

ptag is optional, defaults to creating a new protocol block
options is optional
*/
static int lnet_tcp (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    int src = v_arg_integer(L, 2, "src");
    int dst = v_arg_integer(L, 2, "dst");
    int seq = v_arg_integer(L, 2, "seq");
    int ack = v_arg_integer_opt(L, 2, "ack", 0);
    int flags = v_arg_integer(L, 2, "flags");
    int win = v_arg_integer(L, 2, "win");
    int urg = v_arg_integer_opt(L, 2, "urg", 0);
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_TCP_H);
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int options_ptag = 0;
    uint32_t optionsz = 0;
    const uint8_t* options = checklbuffer(L, 2, "options", &optionsz);
    int cksum = 0; /* 0 is a flag requesting libnet to fill in correct cksum */
    libnet_pblock_t* oblock = NULL;
    int len = 0; /* libnet needs len for checksum calculation */

    oblock = ptag ? libnet_pblock_find(ud, ptag)->prev : ud->pblock_end;

    if(!oblock || oblock->type != LIBNET_PBLOCK_TCPO_H)
      oblock = NULL;
    else
      options_ptag = oblock->ptag;

    /* Two initial states possible:
     *   - has prev ip options block, or not
     * Two final states desired:
     *   - has prev ip options block, or not
     */

    if(!options) {
      libnet_pblock_delete(ud, oblock);
    } else {
      options_ptag = libnet_build_tcp_options(options, optionsz, ud, options_ptag);

      check_error(L, ud, options_ptag);

      if(oblock) {
	/* we replaced an existing block that was correctly placed */
      } else if(ptag) {
	libnet_pblock_insert_before(ud, ptag, options_ptag);
      } else {
          /* we just pushed a new options block, and are about to push a new ip block */
      }
    }

    /* Rewrite len to be len of tcp pblock + previous blocks. */
    {
        libnet_pblock_t* p = ptag ? libnet_pblock_find(ud, ptag)->prev : ud->pblock_end;

        len = LIBNET_TCP_H + payloadsz;

        while(p) {
            /* don't count tcpdata pblock... we will replace it payloadsz data below */
            if(p->type != LIBNET_PBLOCK_TCPDATA)
                len += p->b_len;
            p = p->prev;
        }
    }

    ptag = libnet_build_tcp(
            src, dst, seq, ack, flags, win, cksum, urg,
            len,
            payload, payloadsz,
            ud, ptag);

    check_error(L, ud, ptag);

    lua_pushinteger(L, ptag);

    return 1;
}

/*-
-- argt = net:get_tcp()

Get tcp pblock argument table.
*/

/* TODO - optional ptag argument? */
static int lnet_get_tcp (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkptype(L, ud, LIBNET_PBLOCK_TCP_H);
    struct libnet_tcp_hdr* hdr = (struct libnet_tcp_hdr*) pblock->buf;
    libnet_pblock_t* oblock = pblock->prev;
    libnet_pblock_t* dblock = pblock->prev;

    /* We might have TCP options and/or data blocks, look for them. */
    if(oblock && oblock->type != LIBNET_PBLOCK_TCPO_H) {
        oblock = NULL;
    }

    if(dblock && dblock->type == LIBNET_PBLOCK_TCPO_H) {
        dblock = dblock->prev;
    }
    if(dblock && dblock->type != LIBNET_PBLOCK_TCPDATA) {
        dblock = NULL;
    }
    lua_newtable(L);
    setintfield(L, 2, "ptag", pblock->ptag);
    setnsintfield(L, 2, "src", hdr->th_sport); 
    setnsintfield(L, 2, "dst", hdr->th_dport);
    setnlintfield(L, 2, "seq", hdr->th_seq);
    setnlintfield(L, 2, "ack", hdr->th_ack);
    setintfield(L, 2, "_rsv", hdr->th_x2);
    setintfield(L, 2, "_off", hdr->th_off);
    setintfield(L, 2, "flags", hdr->th_flags);
    setnsintfield(L, 2, "win", hdr->th_win);
    setnsintfield(L, 2, "_sum", hdr->th_sum);
    setnsintfield(L, 2, "urg", hdr->th_urp);

    if(oblock) {
        setlstringfield(L, 2, "options", (const char*)oblock->buf, (size_t)oblock->b_len);
    } else {
        setlstringfield(L, 2, "options", "", 0);
    }

    if(dblock) {
        setlstringfield(L, 2, "payload", (const char*)dblock->buf, (size_t)dblock->b_len);
    }

    return 1;
}


/*-
-- ptag = n:ipv4{
    -- required arguments
      src=ipaddr,
      dst=ipaddr,
      protocol=int,
    -- optional arguments
      ptag=int,
      payload=str,
      options=ip_options,
      len=int, -- default is correct length
      tos=int,
      id=int,
      frag=int,
      ttl=int, -- defaults to 64
  }

ptag is optional, defaults to creating a new protocol block
options is optional
*/
static int lnet_ipv4 (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    const char* src = v_arg_string(L, 2, "src");
    const char* dst = v_arg_string(L, 2, "dst");
    uint32_t src_n = check_ip_pton(L, src, "src");
    uint32_t dst_n = check_ip_pton(L, dst, "dst");
    int protocol = v_arg_integer(L, 2, "protocol"); /* TODO make optional */
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_IPV4_H);
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int options_ptag = 0;
    uint32_t optionsz = 0;
    const uint8_t* options = checklbuffer(L, 2, "options", &optionsz);
    int len = v_arg_integer_opt(L, 2, "len", -1);
    int tos = v_arg_integer_opt(L, 2, "tos", 0);
    int id = v_arg_integer_opt(L, 2, "id", 0);
    int frag = v_arg_integer_opt(L, 2, "frag", 0);
    int ttl = v_arg_integer_opt(L, 2, "ttl", 64);
    int cksum = 0; /* 0 is a flag requesting libnet to fill in correct cksum */
    libnet_pblock_t* oblock = NULL;

#ifdef NET_DUMP
    printf("net ipv4 src %s dst %s len %d payloadsz %lu ptag %d optionsz %lu\n", src, dst, len, payloadsz, ptag, optionsz);
#endif

    oblock = ptag ? libnet_pblock_find(ud, ptag)->prev : ud->pblock_end;

    if(!oblock || oblock->type != LIBNET_PBLOCK_IPO_H)
      oblock = NULL;
    else
      options_ptag = oblock->ptag;

#ifdef NET_DUMP
    printf("  options_ptag %d optionsz from %lu to %lu\n",
            options_ptag, oblock ? oblock->b_len : 0, optionsz);
#endif

    /* Two initial states possible:
     *   - has prev ip options block, or not
     * Two final states desired:
     *   - has prev ip options block, or not
     */

    if(!options) {
      libnet_pblock_delete(ud, oblock);
    } else {
      options_ptag = libnet_build_ipv4_options(options, optionsz, ud, options_ptag);

      check_error(L, ud, options_ptag);

      if(oblock) {
	/* we replaced an existing block that was correctly placed */
      } else if(ptag) {
	libnet_pblock_insert_before(ud, ptag, options_ptag);
      } else {
          /* we just pushed a new options block, and are about to push a new ip block */
      }
    }

    /* If len unspecified, rewrite it to be len of ipv4 pblock + previous blocks. */
    /* FIXME I don't think defaulting to end is correct

-- libnet doesn't have a generic icmp construction api, see bug#1373
local function build_icmp(n, icmp)
    local typecode = string.char(assert(icmp.type), assert(icmp.code))
    local data = icmp.data or ""
    local checksum = net.checksum(typecode, "\0\0", data)
    local packet = typecode..checksum..data

    return n:ipv4{
        src      = arg.localip,
        dst      = arg.dutip,
        protocol = 1, -- ICMP is protocol 1 FIXME get from iana.ip.types.icmp
        payload  = packet,
        len      = 20 + #packet,
        ptag     = icmp.ptag
    }
end

getmetatable(n).icmp = build_icmp

-- set up the pblock stack, top to bottom
local ptag = n:icmp{type=0, code=0}
n:eth{src=arg.localmac, dst=arg.dutmac}

   n:icmp{ptag=ptag, type=type, code=code, payload=data}

print(n:dump())
print(n:get_ipv4())


~/w/wt/achilles-engine/data/Plugins/Grammar % sudo ./icmp-data-grammar-l2 dutip=1.1.1.1 localdev=lo localip=2.2.2.2 dutmac=11:11:11:11:11:11 localmac=22:22:22:22:22:22 pcap=pc.pcap
tag 2 flags 0 type ipdata/0xf buf 0x6541e0 b_len  4 h_len  4 copied 4 prev -1 next 1
tag 1 flags 1 type ipv4/0xd buf 0x6582f0 b_len 20 h_len 20 copied 20 prev 2 next 3
tag 3 flags 0 type eth/0x4 buf 0x647580 b_len 14 h_len  0 copied 14 prev 1 next -1
link_offset 14 aligner 0 total_size 38 nblocks 3

Total:1
Subtest 1: ICMP type 0 code 1 with payload size 1
tag 2 flags 0 type ipdata/0xf buf 0x6541e0 b_len  4 h_len  4 copied 4 prev -1 next 1
tag 1 flags 1 type ipv4/0xd buf 0x6582f0 b_len 20 h_len 20 copied 20 prev 2 next 3
tag 3 flags 0 type eth/0x4 buf 0x647580 b_len 14 h_len  0 copied 14 prev 1 next -1
link_offset 14 aligner 0 total_size 38 nblocks 3

{
ptag = 1, protocol = 1, _iphl = 5, id = 0, options = "", dst = "1.1.1.1", src = "2.2.2.2", _sum = 0, _ipv = 4, tos = 0, _len = 28, ttl = 64, frag = 0
}


============>> note that _len is 28, it should be 24
    
    */
    if(len < 0) {
        libnet_pblock_t* p = ptag ? libnet_pblock_find(ud, ptag)->prev : ud->pblock_end;

        len = LIBNET_IPV4_H + payloadsz;

        while(p) {
            len += p->b_len;
            p = p->prev;
        }
    }

    ptag = libnet_build_ipv4(
            len, tos, id, frag, ttl, protocol, cksum,
            src_n, dst_n,
            payload, payloadsz,
            ud, ptag);

    check_error(L, ud, ptag);

    lua_pushinteger(L, ptag);

    return 1;
}

/*-
-- argt = net:get_ipv4()

Get ipv4 pblock argument table.
*/
/* TODO - optional ptag argument? */
static int lnet_get_ipv4 (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkptype(L, ud, LIBNET_PBLOCK_IPV4_H);
    struct libnet_ipv4_hdr* ip = (struct libnet_ipv4_hdr*) pblock->buf;
    libnet_pblock_t* oblock = pblock->prev;

    if(oblock && oblock->type != LIBNET_PBLOCK_IPO_H) {
        oblock = NULL;
    }

    lua_newtable(L);
    setintfield(L, 2, "ptag", pblock->ptag);
    setintfield(L, 2, "_iphl", ip->ip_hl);
    setintfield(L, 2, "_ipv", ip->ip_v);
    setintfield(L, 2, "tos", ip->ip_tos);
    setintfield(L, 2, "_len", ntohs(ip->ip_len));
    setintfield(L, 2, "id", ntohs(ip->ip_id));
    setintfield(L, 2, "frag", ntohs(ip->ip_off));
    /* FIXME
    Only if non-zero, or flags are set:
    setintfield(L, 2, "_fragoffset", ntohs(ip->ip_off) & 0xd0);
    Only if non-zero:
    setintfield(L, 2, "_fragflags",  ntohs(ip->ip_off) & 0x1f);
                 , 2, "_fragmore",                           ;
                 , 2, "_fragdont",                           ;
                 , 2, "_fragrsv",                           ;
    */
    setintfield(L, 2, "ttl", ip->ip_ttl);
    setintfield(L, 2, "protocol", ip->ip_p);
    setintfield(L, 2, "_sum", ntohs(ip->ip_sum));
    setipfield(L, 2, "src", ip->ip_src);
    setipfield(L, 2, "dst", ip->ip_dst);

    if(oblock) {
        lua_pushlstring(L, (const char*)oblock->buf, (size_t)oblock->b_len);
    } else {
        lua_pushstring(L, "");
    }
    lua_setfield(L, 2, "options");

    /* TODO
        If we need to manipulate IPV4 payload, then we'll need to encode the
        blocks following the options, and then set and _payload field.

        Unless the payload was PBLOCK_IPDATA instead of some specific pblock,
        in which case it's easy to get it.

        Also, we'll need to change either libnet_build_ipv4() or net:ipv4{} so
        that when payload is specified, it destroys any of the preceeding pblocks
        that would normally be used to form the payload.
    */
    /* FIXME - at least deal with IPDATA, as get_tcp does. */

    return 1;
}


/*-
-- ptag = n:eth{src=ethmac, dst=ethmac, type=int, payload=str, ptag=int}

type is optional, defaults to IP
ptag is optional, defaults to creating a new protocol block
*/
static int lnet_eth (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    const char* src = v_arg_string(L, 2, "src");
    const char* dst = v_arg_string(L, 2, "dst");
    int type = v_arg_integer_opt(L, 2, "type", ETHERTYPE_IP);
    uint32_t payloadsz = 0;
    const uint8_t* payload = checkpayload(L, 2, &payloadsz);
    int ptag = lnet_arg_ptag(L, ud, 2, LIBNET_PBLOCK_ETH_H);

#ifdef NET_DUMP
    printf("net eth src %s dst %s type %d payloadsz %lu ptag %d\n", src, dst, type, payloadsz, ptag);
#endif

    {
        eth_addr_t src_n = check_eth_pton(L, src, "src");
        eth_addr_t dst_n = check_eth_pton(L, dst, "dst");
        ptag = libnet_build_ethernet(dst_n.data, src_n.data, type, payload, payloadsz, ud, ptag);
    }
    check_error(L, ud, ptag);
    lua_pushinteger(L, ptag);
    return 1;
}


/*-
-- argt = net:get_eth()

Get eth pblock argument table.
*/
static int lnet_get_eth (lua_State *L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkptype(L, ud, LIBNET_PBLOCK_ETH_H);
    struct libnet_ethernet_hdr* hdr = (struct libnet_ethernet_hdr*) pblock->buf;

    lua_newtable(L);
    setintfield(L, 2, "ptag", pblock->ptag);
    setethfield(L, 2, "src", hdr->ether_shost);
    setethfield(L, 2, "dst", hdr->ether_dhost);
    setnsintfield(L, 2, "type", hdr->ether_type);

    if(!pblock->prev) {
        /* direct payload */
        uint8_t* data = pblock->buf + LIBNET_ETH_H;
        uint32_t datasz = 0;

        if(pblock->b_len > LIBNET_ETH_H) {
            datasz = pblock->b_len - LIBNET_ETH_H;
        }

        setlstringfield(L, 2, "payload", data, (size_t)datasz);
    }

    return 1;
}


static int lnet_decode_ipv4(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    size_t pkt_s = 0;
    const char* pkt = luaL_checklstring(L, 2, &pkt_s);
    int ptag = 0;

    ptag = libnet_decode_ipv4((void*)pkt, pkt_s, ud);

    pushptag(L, ud, ptag);

    return 1;
}

static int lnet_decode_ip(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    size_t pkt_s = 0;
    const char* pkt = luaL_checklstring(L, 2, &pkt_s);
    int ptag = 0;

    ptag = libnet_decode_ip((void*)pkt, pkt_s, ud);

    pushptag(L, ud, ptag);

    return 1;
}

static int lnet_decode_eth(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    size_t pkt_s = 0;
    const char* pkt = luaL_checklstring(L, 2, &pkt_s);
    int ptag = 0;

    ptag = libnet_decode_eth((void*)pkt, pkt_s, ud);

    pushptag(L, ud, ptag);

    return 1;
}


static void pushpblock(lua_State* L, libnet_pblock_t* pblock)
{
    int tindex = lua_gettop(L) + 1;
    lua_newtable(L);
    setlstringfield(L, tindex, "buf", pblock->buf, pblock->b_len);
    setintfield(L, tindex, "b_len", pblock->b_len);
    setintfield(L, tindex, "h_len", pblock->h_len);
    setintfield(L, tindex, "copied", pblock->copied);
    setstringfield(L, tindex, "type", libnet_diag_dump_pblock_type(pblock->type));
    setintfield(L, tindex, "flags", pblock->flags);
    setintfield(L, tindex, "ptag", pblock->ptag);
    if(pblock->next)
        setintfield(L, tindex, "next", pblock->next->ptag);
    if(pblock->prev)
        setintfield(L, tindex, "prev", pblock->prev->ptag);
}

/*-
-- net:pblock(lua_State* L, int ptag)

Get a table with all the pblock info in it.
*/
static int lnet_pblock(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* pblock = checkpblock(L, ud, 2);
    pushpblock(L, pblock);
    return 1;
}

/*-
-- net:dump()

Write summary of protocol blocks to stdout.
*/
static int lnet_dump(lua_State* L)
{
    libnet_t* ud = checkudata(L);
    libnet_pblock_t* p = ud->protocol_blocks;
    int strings = 0;

    while(p) {
        /* h_len is header length for checksumming? "chksum length"? */
        char str[1024];
        sprintf(str, "tag %d flags %d type %s/%#x buf %p b_len %2u h_len %2u copied %u prev %d next %d\n",
                p->ptag, p->flags, type_string(p->type), p->type,
                p->buf, p->b_len, p->h_len, p->copied,
                p->prev ? p->prev->ptag : -1,
                p->next ? p->next->ptag : -1
                );
        lua_pushstring(L, str);
        p = p->next;
        strings++;
    }
    lua_pushfstring(L, "link_offset %d aligner %d total_size %d nblocks %d\n",
            ud->link_offset, ud->aligner, ud->total_size, ud->n_pblocks);
    strings++;

    lua_concat(L, strings);

    return 1;
}

#ifdef NET_DUMP
/* Called from inside gdb to see current state of libnet stack */
void dump(lua_State* L) {
  lnet_dump(L);
  printf("%s\n", lua_tostring(L, -1));
  lua_pop(L, 1);
}
#endif

/* TODO net.ntop() take either a number (a host u32 inet addr), or a string of bytes, and convert to presentation */

/*-
-- network = net.pton(presentation)

presentation is something like "df:33:44:12:45:54", or "1.2.3.4", or a host name

return is the binary, network byte-ordered address (you have to know what kind it was!)
*/
static int lnet_pton(lua_State *L)
{
    const char* src = luaL_checkstring(L, 1);
    struct addr dst;

    if(addr_pton(src, &dst) < 0) {
        return luaL_error(L, "pton failed on '%s'", src);
    }

    {
        int size = dst.addr_bits;
        void* addr = &dst.__addr_u;

        lua_pushlstring(L, addr, size/8);
    }

    return 1;
}

/*-
-- n = net.htons(h)

Return a network byte order encoding of number, h.
*/
static int lnet_htons(lua_State *L)
{
    int h = luaL_checkint(L, 1);
    union {
        char buf[2];
        uint16_t n;
    } u;

    u.n = htons(h);

    lua_pushlstring(L, u.buf, sizeof(u.buf));

    return 1;
}

/*-
-- sum = net.checksum(string, ...)

Checksum the series of strings passed in. sum is a string, the 16-bit checksum
encoded in network byte order.
*/
static int lnet_chksum(lua_State *L)
{
    int interm = 0;
    uint16_t chks = 0;

    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {
        size_t length = 0;
        const char* src = luaL_checklstring(L, i, &length);
        interm += libnet_in_cksum((uint16_t*)src, length);
    }

    chks = LIBNET_CKSUM_CARRY(interm);

    lua_pushlstring(L, (char *)&chks, 2); /* FIXME intel specific? should be using htons() */

    return 1;
}

#ifndef _WIN32
/* TODO see http://stackoverflow.com/questions/7827062/is-there-a-windows-equivalent-of-nanosleep */
/*-
-- remaining = net.nanosleep(seconds)

Seconds can be smaller than 1 (resolution is nanoseconds, theoretically).
Return is number of seconds not slept, or nil and an error message on failure.

remaining = assert(net.nanosleep(seconds))

*/
static int lnet_nanosleep(lua_State *L)
{
    double n = luaL_checknumber(L, 1);
    struct timespec req = { 0 };
    struct timespec rem = { 0 };

    luaL_argcheck(L, n > 0, 1, "seconds must be greater than zero");

    req.tv_sec = (time_t) n;
    req.tv_nsec = 1000000000 * (n-req.tv_sec);

    if(nanosleep(&req, &rem) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    } else {
        lua_pushnumber(L, (double) rem.tv_sec + rem.tv_nsec / 1000000000.0);
        return 1;
    }
}

/* TODO use GetSystemTime or GetSystemTimeAsFileTime */
/*-
    t = net.gettimeofday()

Returns the current time since the epoch as a decimal number, with up to 
microsecond precision.  On error, returns nil and an error message.
*/
static int lnet_gettimeofday(lua_State *L)
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    } else {
        lua_pushnumber(L, (double) tv.tv_sec + tv.tv_usec / 1000000.0);
        return 1;
    }
}

#endif

/*-
-- net.init(injection, device)

injection is one of "link", "raw", ...
device is "eth0", ...

Switch order of args? injection we can give a default (link), device we can't default.
*/
static int lnet_init(lua_State *L)
{
    static const char* injection_opt[] = {
        "none", "link", "link_adv", "raw4", "raw4_adv", "raw6", "raw6_adv", NULL
    };
    static int injection_val[] = {
        LIBNET_NONE, LIBNET_LINK, LIBNET_LINK_ADV, LIBNET_RAW4, LIBNET_RAW4_ADV, LIBNET_RAW6, LIBNET_RAW6_ADV
    };
    char errbuf[LIBNET_ERRBUF_SIZE];
    int type = injection_val[luaL_checkoption(L, 1, "none", injection_opt)];
    const char *device = luaL_optstring(L, 2, NULL);

    libnet_t** ud = lua_newuserdata(L, sizeof(*ud));
    *ud = NULL;

    luaL_getmetatable(L, L_NET_REGID);
    lua_setmetatable(L, -2);

    *ud = libnet_init(type, device, errbuf);

    if (!*ud) {
        return luaL_error(L, "%s", errbuf);
    }

    return 1;
}

static const luaL_reg net_methods[] =
{
  {"__gc", lnet_destroy},
  {"destroy", lnet_destroy},
  {"clear", lnet_clear},
  {"block", lnet_block},
  {"pbuf", lnet_pbuf},
  {"write", lnet_write},
  {"write_link", lnet_write_link},
  {"tag_below", lnet_tag_below},
  {"tag_above", lnet_tag_above},
  {"tag_type", lnet_tag_type},
  {"fd", lnet_getfd},
  {"device", lnet_getdevice},
  {"data", lnet_data},
  {"igmp", lnet_igmp},
  {"udp", lnet_udp},
  {"get_udp", lnet_get_udp},
  {"tcp", lnet_tcp},
  {"get_tcp", lnet_get_tcp},
  {"ipv4", lnet_ipv4},
  {"get_ipv4", lnet_get_ipv4},
  {"eth", lnet_eth},
  {"get_eth", lnet_get_eth},
  {"decode_ipv4", lnet_decode_ipv4},
  {"decode_ip", lnet_decode_ip},
  {"decode_eth", lnet_decode_eth},
  {"pblock", lnet_pblock},
  {"dump", lnet_dump},
  {NULL, NULL}
};

static const luaL_reg net[] =
{
  {"pton", lnet_pton},
  {"htons", lnet_htons},
  {"checksum", lnet_chksum},
#ifndef _WIN32
  {"nanosleep", lnet_nanosleep},
  {"gettimeofday", lnet_gettimeofday},
#endif
  {"init", lnet_init},
  {NULL, NULL}
};

LUALIB_API int luaopen_net (lua_State *L)
{
  v_obj_metatable(L, L_NET_REGID, net_methods);

  luaL_register(L, "net", net);

  return 1;
}

