/*
 *  $Id: libnet_link_dlpi.c,v 1.8 2004/01/28 19:45:00 mike Exp $
 *
 *  libnet
 *  libnet_dlpi.c - dlpi routines
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * This code contributed by Atanu Ghosh (atanu@cs.ucl.ac.uk),
 * University College London.
 */


#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_BUFMOD_H
#include <sys/bufmod.h>
#endif
#include <sys/dlpi.h>
#ifdef HAVE_HPUX9
#include <sys/socket.h>
#endif
#ifdef DL_HP_PPA_ACK_OBS
#include <sys/stat.h>
#endif
#include <sys/stream.h>
#if defined(HAVE_SOLARIS) && defined(HAVE_SYS_BUFMOD_H)
#include <sys/systeminfo.h>
#endif

#ifdef HAVE_SYS_DLPI_EXT_H
#include <sys/dlpi_ext.h>
#endif

#ifdef HAVE_HPUX9
#include <net/if.h>
#endif

#include <ctype.h>
#ifdef HAVE_HPUX9
#include <nlist.h>
#include <dlpi_ext.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>

#include "../include/libnet.h"
#include "../include/bpf.h"

#include "../include/gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "../include/os-proto.h"
#endif

#ifndef DLPI_DEV_PREFIX
#define DLPI_DEV_PREFIX "/dev"
#endif

#define	MAXDLBUF 8192

/* Forwards */
static int dlattachreq(int, bpf_u_int32, int8_t *);
static int dlbindack(int, int8_t *, int8_t *);
static int dlbindreq(int, bpf_u_int32, int8_t *);
static int dlinfoack(int, int8_t *, int8_t *);
static int dlinforeq(int, int8_t *);
static int dlokack(int, const int8_t *, int8_t *, int8_t *);
static int recv_ack(int, int, const int8_t *, int8_t *, int8_t *);
static int send_request(int, int8_t *, int, int8_t *, int8_t *, int);
#ifdef HAVE_SYS_BUFMOD_H
static int strioctl(int, int, int, int8_t *);
#endif
#ifdef HAVE_HPUX9
static int dlpi_kread(int, off_t, void *, uint, int8_t *);
#endif
#ifdef HAVE_DEV_DLPI
static int get_dlpi_ppa(int, const int8_t *, int, int8_t *);
#endif

/* XXX Needed by HP-UX (at least) */
static bpf_u_int32 ctlbuf[MAXDLBUF];


int
libnet_open_link(libnet_t *l)
{
    register int8_t *cp;
    int8_t *eos;
    register int ppa;
    register dl_info_ack_t *infop;
    bpf_u_int32 buf[MAXDLBUF];
    int8_t dname[100];
#ifndef HAVE_DEV_DLPI
    int8_t dname2[100];
#endif

    if (l == NULL)
    { 
        return (-1);
    } 

    /*
     *  Determine device and ppa
     */
    cp = strpbrk(l->device, "0123456789");
    if (cp == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %s is missing unit number\n", __func__, l->device);
        goto bad;
    }
    ppa = strtol(cp, &eos, 10);
    if (*eos != '\0')
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): %s bad unit number\n", __func__, l->device);
        goto bad;
    }

    if (*(l->device) == '/')
    {
        memset(&dname, 0, sizeof(dname));
        strncpy(dname, l->device, sizeof(dname) - 1);
        dname[sizeof(dname) - 1] = '\0';
    }
    else
    {
        sprintf(dname, "%s/%s", DLPI_DEV_PREFIX, l->device);
    }
#ifdef HAVE_DEV_DLPI
    /*
     *  Map network device to /dev/dlpi unit
     */
    cp = "/dev/dlpi";

    l->fd = open(cp, O_RDWR);
    if (l->fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): open(): %s: %s\n",
                __func__, cp, strerror(errno));
        goto bad;
    }

    /*
     *  Map network interface to /dev/dlpi unit
     */
    ppa = get_dlpi_ppa(l->fd, dname, ppa, l->err_buf);
    if (ppa < 0)
    {
        goto bad;
    }
#else
    /*
     *  Try device without unit number
     */
    strcpy(dname2, dname);
    cp = strchr(dname, *cp);
    *cp = '\0';

    l->fd = open(dname, O_RDWR);
    if (l->fd == -1)
    {
        if (errno != ENOENT)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): open(): %s: %s\n",
                    __func__, dname, strerror(errno));
            goto bad;
        }

        /*
         *  Try again with unit number
         */
        l->fd = open(dname2, O_RDWR);
        if (l->fd == -1)
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, "%s(): open(): %s: %s\n",
                    __func__, dname2, strerror(errno));
            goto bad;
        }

        cp = dname2;
        while (*cp && !isdigit((int)*cp))
        {
            cp++;
        }
        if (*cp)
        {
            ppa = atoi(cp);
        }
        else
        /*
         *  XXX Assume unit zero
         */
        ppa = 0;
    }
#endif
    /*
     *  Attach if "style 2" provider
     */
    if (dlinforeq(l->fd, l->err_buf) < 0 ||
            dlinfoack(l->fd, (int8_t *)buf, l->err_buf) < 0)
    {
        goto bad;
    }
    infop = &((union DL_primitives *)buf)->info_ack;
    if (infop->dl_provider_style == DL_STYLE2 &&
            (dlattachreq(l->fd, ppa, l->err_buf)
            < 0 || dlokack(l->fd, "attach", (int8_t *)buf, l->err_buf) < 0))
    {
        goto bad;
    }

    /*
     *  Bind HP-UX 9 and HP-UX 10.20
     */
#if defined(HAVE_HPUX9) || defined(HAVE_HPUX10_20) || defined(HAVE_HPUX11) || defined(HAVE_SOLARIS)
    if (dlbindreq(l->fd, 0, l->err_buf) < 0 ||
            dlbindack(l->fd, (int8_t *)buf, l->err_buf) < 0)
    {
        goto bad;
    }
#endif

    /*
     *  Determine link type
     */
    if (dlinforeq(l->fd, l->err_buf) < 0 ||
            dlinfoack(l->fd, (int8_t *)buf, l->err_buf) < 0)
    {
        goto bad;
    }

    infop = &((union DL_primitives *)buf)->info_ack;
    switch (infop->dl_mac_type)
    {
        case DL_CSMACD:
        case DL_ETHER:
            l->link_type    = DLT_EN10MB;
            l->link_offset  = 0xe;
            break;
        case DL_FDDI:
            l->link_type    = DLT_FDDI;
            l->link_offset  = 0x15;
            break;
        case DL_TPR:
            l->link_type    = DLT_PRONET;
            l->link_offset  = 0x16;
            break;
        default:
            sprintf(l->err_buf, "%s(): unknown mac type 0x%lu\n", __func__,
                    (uint32_t) infop->dl_mac_type);
            goto bad;
    }

#ifdef	DLIOCRAW
    /*
     *  This is a non standard SunOS hack to get the ethernet header.
     */
    if (strioctl(l->fd, DLIOCRAW, 0, NULL) < 0)
    {
        sprintf(l->err_buf, "%s(): DLIOCRAW: %s\n", __func__, strerror(errno));
        goto bad;
    }
#endif

    return (1);
bad:
    if (l->fd > 0)
    {
        close(l->fd);      /* this can fail ok */
    }
    return (-1);
}


static int
send_request(int fd, int8_t *ptr, int len, int8_t *what, int8_t *ebuf,
int flags)
{
    struct strbuf ctl;

    ctl.maxlen = 0;
    ctl.len = len;
    ctl.buf = ptr;

    if (putmsg(fd, &ctl, (struct strbuf *) NULL, flags) < 0)
    {
        sprintf(ebuf, "%s(): putmsg \"%s\": %s\n", __func__, what,
                strerror(errno));
        return (-1);
    }
    return (0);
}

static int
recv_ack(int fd, int size, const int8_t *what, int8_t *bufp, int8_t *ebuf)
{
    union DL_primitives *dlp;
    struct strbuf ctl;
    int flags;

    ctl.maxlen = MAXDLBUF;
    ctl.len = 0;
    ctl.buf = bufp;

    flags = 0;
    if (getmsg(fd, &ctl, (struct strbuf*)NULL, &flags) < 0)
    {
        sprintf(ebuf, "%s(): %s getmsg: %s\n", __func__, what, strerror(errno));
        return (-1);
    }

    dlp = (union DL_primitives *)ctl.buf;
    switch (dlp->dl_primitive)
    {
        case DL_INFO_ACK:
        case DL_PHYS_ADDR_ACK:
        case DL_BIND_ACK:
        case DL_OK_ACK:
#ifdef DL_HP_PPA_ACK
        case DL_HP_PPA_ACK:
#endif
        /*
         *  These are OK
         */
        break;

        case DL_ERROR_ACK:
            switch (dlp->error_ack.dl_errno)
            {
                case DL_BADPPA:
                    sprintf(ebuf, "recv_ack: %s bad ppa (device unit)", what);
                    break;
                case DL_SYSERR:
                    sprintf(ebuf, "recv_ack: %s: %s",
                        what, strerror(dlp->error_ack.dl_unix_errno));
                    break;
                case DL_UNSUPPORTED:
                    sprintf(ebuf,
                        "recv_ack: %s: Service not supplied by provider", what);
                    break;
                default:
                    sprintf(ebuf, "recv_ack: %s error 0x%x", what,
                        (bpf_u_int32)dlp->error_ack.dl_errno);
                    break;
            }
            return (-1);

        default:
            sprintf(ebuf, "recv_ack: %s unexpected primitive ack 0x%x ",
                what, (bpf_u_int32)dlp->dl_primitive);
            return (-1);
    }

    if (ctl.len < size)
    {
        sprintf(ebuf, "recv_ack: %s ack too small (%d < %d)",
            what, ctl.len, size);
        return (-1);
    }
    return (ctl.len);
}

static int
dlattachreq(int fd, bpf_u_int32 ppa, int8_t *ebuf)
{
    dl_attach_req_t req;

    req.dl_primitive = DL_ATTACH_REQ;
    req.dl_ppa       = ppa;

    return (send_request(fd, (int8_t *)&req, sizeof(req), "attach", ebuf, 0));
}

static int
dlbindreq(int fd, bpf_u_int32 sap, int8_t *ebuf)
{

    dl_bind_req_t	req;

    memset((int8_t *)&req, 0, sizeof(req));
    req.dl_primitive = DL_BIND_REQ;
#ifdef DL_HP_RAWDLS
    req.dl_max_conind = 1;  /* XXX magic number */
    /*
     *  22 is INSAP as per the HP-UX DLPI Programmer's Guide
     */
    req.dl_sap = 22;
    req.dl_service_mode = DL_HP_RAWDLS;
#else
    req.dl_sap = sap;
#ifdef DL_CLDLS
    req.dl_service_mode = DL_CLDLS;
#endif
#endif
    return (send_request(fd, (int8_t *)&req, sizeof(req), "bind", ebuf, 0));
}

static int
dlbindack(int fd, int8_t *bufp, int8_t *ebuf)
{
    return (recv_ack(fd, DL_BIND_ACK_SIZE, "bind", bufp, ebuf));
}

static int
dlokack(int fd, const int8_t *what, int8_t *bufp, int8_t *ebuf)
{
    return (recv_ack(fd, DL_OK_ACK_SIZE, what, bufp, ebuf));
}

static int
dlinforeq(int fd, int8_t *ebuf)
{
    dl_info_req_t req;

    req.dl_primitive = DL_INFO_REQ;

    return (send_request(fd, (int8_t *)&req, sizeof(req), "info", ebuf, 
            RS_HIPRI));
}

static int
dlinfoack(int fd, int8_t *bufp, int8_t *ebuf)
{
    return (recv_ack(fd, DL_INFO_ACK_SIZE, "info", bufp, ebuf));
}


#ifdef HAVE_SYS_BUFMOD_H
static int
strioctl(int fd, int cmd, int len, int8_t *dp)
{
    struct strioctl str;
    int rc;

    str.ic_cmd    = cmd;
    str.ic_timout = -1;
    str.ic_len    = len;
    str.ic_dp     = dp;
    
    rc = ioctl(fd, I_STR, &str);
    if (rc < 0)
    {
        return (rc);
    }
    else
    {
        return (str.ic_len);
    }
}
#endif


#if (defined(DL_HP_PPA_ACK_OBS) && !defined(HAVE_HPUX11))
/*
 * Under HP-UX 10, we can ask for the ppa
 */
static int
get_dlpi_ppa(register int fd, register const int8_t *device, register int unit,
register int8_t *ebuf)
{
    register dl_hp_ppa_ack_t *ap;
    register dl_hp_ppa_info_t *ip;
    register int i;
    register uint32_t majdev;
    dl_hp_ppa_req_t	req;
    struct stat statbuf;
    bpf_u_int32 buf[MAXDLBUF];

    if (stat(device, &statbuf) < 0)
    {
        sprintf(ebuf, "stat: %s: %s", device, strerror(errno));
        return (-1);
    }
    majdev = major(statbuf.st_rdev);

    memset((int8_t *)&req, 0, sizeof(req));
    req.dl_primitive = DL_HP_PPA_REQ;

    memset((int8_t *)buf, 0, sizeof(buf));
    if (send_request(fd, (int8_t *)&req, sizeof(req), "hpppa", ebuf, 0) < 0 ||
        recv_ack(fd, DL_HP_PPA_ACK_SIZE, "hpppa", (int8_t *)buf, ebuf) < 0)
    {
        return (-1);
    }

    ap = (dl_hp_ppa_ack_t *)buf;
    ip = (dl_hp_ppa_info_t *)((uint8_t *)ap + ap->dl_offset);

    for (i = 0; i < ap->dl_count; i++)
    {
        if (ip->dl_mjr_num == majdev && ip->dl_instance_num == unit)
        break;

        ip = (dl_hp_ppa_info_t *)((uint8_t *)ip + ip->dl_next_offset);
    }

    if (i == ap->dl_count)
    {
        sprintf(ebuf, "can't find PPA for %s", device);
        return (-1);
    }

    if (ip->dl_hdw_state == HDW_DEAD)
    {
        sprintf(ebuf, "%s: hardware state: DOWN\n", device);
        return (-1);
    }
    return ((int)ip->dl_ppa);
}
#endif

#ifdef HAVE_HPUX9
/*
 * Under HP-UX 9, there is no good way to determine the ppa.
 * So punt and read it from /dev/kmem.
 */
static struct nlist nl[] =
{
#define NL_IFNET 0
    { "ifnet" },
    { "" }
};

static int8_t path_vmunix[] = "/hp-ux";

/*
 *  Determine ppa number that specifies ifname
 */
static int
get_dlpi_ppa(register int fd, register const int8_t *ifname, register int unit,
    register int8_t *ebuf)
{
    register const int8_t *cp;
    register int kd;
    void *addr;
    struct ifnet ifnet;
    int8_t if_name[sizeof(ifnet.if_name)], tifname[32];

    cp = strrchr(ifname, '/');
    if (cp != NULL)
    {
        ifname = cp + 1;
    }
    if (nlist(path_vmunix, &nl) < 0)
    {
        sprintf(ebuf, "nlist %s failed", path_vmunix);
        return (-1);
    }

    if (nl[NL_IFNET].n_value == 0)
    {
        sprintf(ebuf, "could't find %s kernel symbol", nl[NL_IFNET].n_name);
        return (-1);
    }

    kd = open("/dev/kmem", O_RDONLY);
    if (kd < 0)
    {
        sprintf(ebuf, "kmem open: %s", strerror(errno));
        return (-1);
    }

    if (dlpi_kread(kd, nl[NL_IFNET].n_value, &addr, sizeof(addr), ebuf) < 0)
    {
        close(kd);
        return (-1);
    }
    for (; addr != NULL; addr = ifnet.if_next)
    {
        if (dlpi_kread(kd, (off_t)addr, &ifnet, sizeof(ifnet), ebuf) < 0 ||
            dlpi_kread(kd, (off_t)ifnet.if_name,
            if_name, sizeof(if_name), ebuf) < 0)
            {
                close(kd);
                return (-1);
            }
            sprintf(tifname, "%.*s%d",
                (int)sizeof(if_name), if_name, ifnet.if_unit);
            if (strcmp(tifname, ifname) == 0)
            {
                return (ifnet.if_index);
            }
    }

    sprintf(ebuf, "Can't find %s", ifname);
    return (-1);
}

static int
dlpi_kread(register int fd, register off_t addr, register void *buf,
register uint len, register int8_t *ebuf)
{
    register int cc;

    if (lseek(fd, addr, SEEK_SET) < 0)
    {
        sprintf(ebuf, "lseek: %s", strerror(errno));
        return (-1);
    }
    cc = read(fd, buf, len);
    if (cc < 0)
    {
        sprintf(ebuf, "read: %s", strerror(errno));
        return (-1);
    }
    else if (cc != len)
    {
        sprintf(ebuf, "int16_t read (%d != %d)", cc, len);
        return (-1);
    }
    return (cc);
}
#endif

#define ETHERADDRL 6
struct  EnetHeaderInfo
{
    struct libnet_ether_addr   DestEtherAddr;
    uint16_t EtherFrameType;
};


int
libnet_close_link(libnet_t *l)
{
    if (close(l->fd) == 0)
    {
        return (1);
    }
    else
    {
        return (-1);
    }
}

#ifdef HAVE_HPUX11
int 
libnet_write_link(libnet_t *l, const uint8_t *packet, uint32_t size)   
{                                                             
    struct strbuf data, ctl;
    dl_hp_rawdata_req_t *rdata;
    int c;                     
                          
    if (l == NULL)
    {             
        return (-1);           
    }       
            
    rdata = (dl_hp_rawdata_req_t *)ctlbuf;
    rdata->dl_primitive = DL_HP_RAWDATA_REQ; 

    /* send it */                                  
    ctl.len = sizeof(dl_hp_rawdata_req_t);                         
    ctl.maxlen = sizeof(dl_hp_rawdata_req_t);                      
    ctl.buf = (int8_t *)rdata;

    data.maxlen = size;
    data.len    = size;                                
    data.buf    = packet;  

    c = putmsg(l->fd, &ctl, &data, 0);
    if (c == -1)                      
    {                    
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_link(): %d bytes written (%s)\n", c,
                strerror(errno));
        return (-1);
    }
    else
    {
        return (size);
    }                
}   
#else
int
libnet_write_link(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    int c;
    struct strbuf data;

    if (l == NULL)
    { 
        return (-1);
    } 

    data.maxlen = size;
    data.len    = size;
    data.buf    = packet;

    c = putmsg(l->fd, NULL, &data, 0);
    if (c == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "libnet_write_link: %d bytes written (%s)\n", c,
                strerror(errno));
        return (-1);
    }
    else
    {
        return (size);
    }
}
#endif

struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l)
{
    /* This implementation is not-reentrant. */
    static int8_t buf[2048];
    union DL_primitives *dlp;
    struct libnet_ether_addr *eap;

    if (l == NULL)
    { 
        return (NULL);
    }

    dlp = (union DL_primitives *)buf;

    dlp->physaddr_req.dl_primitive = DL_PHYS_ADDR_REQ;
    dlp->physaddr_req.dl_addr_type = DL_CURR_PHYS_ADDR;

    if (send_request(l->fd, (int8_t *)dlp, DL_PHYS_ADDR_REQ_SIZE, "physaddr",
            l->err_buf, 0) < 0)
    {
        return (NULL);
    }
    if (recv_ack(l->fd, DL_PHYS_ADDR_ACK_SIZE, "physaddr", (int8_t *)dlp,
            l->err_buf) < 0)
    {
        return (NULL);
    }

    eap = (struct libnet_ether_addr *)
            ((int8_t *) dlp + dlp->physaddr_ack.dl_addr_offset);
    return (eap);
}   

/* EOF */
