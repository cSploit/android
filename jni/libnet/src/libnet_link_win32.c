/*
 *  $Id: libnet_link_win32.c,v 1.16 2004/02/18 18:19:00 mike Exp $
 *
 *  libnet
 *  libnet_link_win32.c - low-level win32 libwpcap routines
 *
 *  Copyright (c) 2001 - 2002 Don Bowman <don@sandvine.com>
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  Copyright (c) 2002 Roberto Larcher <roberto.larcher@libero.it>
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

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#include <winsock2.h>
#include <iphlpapi.h> /* From the Microsoft Platform SDK */
#include "../include/win32/libnet.h"
#include <assert.h>
#include <Packet32.h>
#include <Ntddndis.h>
#include "iprtrmib.h"

int
libnet_open_link(libnet_t *l)
{
    DWORD dwErrorCode;
    NetType IFType;

    if (l == NULL)
    { 
        return (-1);
    } 

    if (l->device == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): NULL device\n", __func__);
        return (-1);
    }

    l->lpAdapter = 0;

    /* open adapter */
	l->lpAdapter = PacketOpenAdapter(l->device);
    if (!l->lpAdapter || (l->lpAdapter->hFile == INVALID_HANDLE_VALUE))
    {
        dwErrorCode=GetLastError();
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): unable to open the driver, error Code : %lx\n",
                __func__, dwErrorCode); 
        return (-1);
    }
	
    /* increase the send buffer */
    PacketSetBuff(l->lpAdapter, 512000);

    /*
     *  Assign link type and offset.
     */
    if (PacketGetNetType(l->lpAdapter, &IFType))
    {
        switch(IFType.LinkType)
        {
            case NdisMedium802_3:
				l->link_type = DLT_EN10MB;
				l->link_offset = LIBNET_ETH_H;
                break;
			case NdisMedium802_5:
				l->link_type = DLT_IEEE802;
				l->link_offset = LIBNET_TOKEN_RING_H;
				break;
			case NdisMediumFddi:
				l->link_type = DLT_FDDI;
				l->link_offset = 0x15;
				break;
			case NdisMediumWan:
				snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s():, WinPcap has disabled support for Network type (%d)\n",
                 __func__, IFType.LinkType);
				return (-1);
				break;
			case NdisMediumAtm:
				l->link_type = DLT_ATM_RFC1483;
				break;
			case NdisMediumArcnet878_2:
				l->link_type = DLT_ARCNET;
				break;
			default:
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                         "%s(): network type (%d) is not supported\n",
                         __func__, IFType.LinkType);
                return (-1);
                break;
        }
    }
    else
    {
        dwErrorCode=GetLastError();
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): unable to determine the network type, error Code : %lx\n",
                __func__, dwErrorCode);
        return (-1);
    }
    return (1);
}

int
libnet_close_link_interface(libnet_t *l)
{
    if (l->lpAdapter)
    {
        PacketSetHwFilter(l->lpAdapter, NDIS_PACKET_TYPE_ALL_LOCAL);
        PacketCloseAdapter(l->lpAdapter);
    }
    return (1);
}

int
libnet_write_link(libnet_t *l, const uint8_t *packet, uint32_t size)
{
    LPPACKET   lpPacket;
    DWORD      BytesTransfered;	

    BytesTransfered = -1;

    if ((lpPacket = PacketAllocatePacket()) == NULL)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): failed to allocate the LPPACKET structure\n", __func__);
		return (-1);
    }
    PacketInitPacket(lpPacket, packet, size);

    /* PacketSendPacket returns a BOOLEAN */
    if(PacketSendPacket(l->lpAdapter, lpPacket, TRUE))
    {
	    BytesTransfered = size;
    }
	
    PacketFreePacket(lpPacket);
    return (BytesTransfered);
 }

struct libnet_ether_addr *
libnet_get_hwaddr(libnet_t *l)
{
    /* This implementation is not-reentrant. */
    static struct libnet_ether_addr *mac;
    
    ULONG IoCtlBufferLength = (sizeof(PACKET_OID_DATA) + sizeof(ULONG) - 1);
	PPACKET_OID_DATA OidData;
	
	int i = 0;

	if (l == NULL)
    { 
        return (NULL);
    } 

	if (l->device == NULL)
    {           
        if (libnet_select_device(l) == -1)
        {   
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): can't figure out a device to use\n", __func__);
            return (NULL);
        }
    }

    mac = (struct libnet_ether_addr *)calloc(1,sizeof(struct libnet_ether_addr));
	if (mac == NULL)
	{
		snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): calloc error\n", __func__);
		return (NULL);
	}

    OidData = (struct _PACKET_OID_DATA *) malloc(IoCtlBufferLength);
	
	if (OidData == NULL)
	{
	     snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
				 "%s(): OidData is NULL\n", __func__);
	    return(NULL);
	}

	if (l->link_type == DLT_IEEE802)
	{
		OidData->Oid = OID_802_5_CURRENT_ADDRESS;	
	}
	else
	{
		OidData->Oid = OID_802_3_CURRENT_ADDRESS;	
	}
	
	OidData->Length = 6;
	if((PacketRequest(l->lpAdapter, FALSE, OidData)) == FALSE)
	{
		memset(mac, 0, 6);
	}
	else
	{
		for (i = 0; i < 6; i++)
		{
			mac->ether_addr_octet[i] = OidData->Data[i];
		}
	}
        free(OidData);
	return(mac);
}

struct hostent *gethostbyname2(const int8_t *name, int af) 
{
   /* XXX not implemented */
   return(NULL);
}

BYTE *
libnet_win32_get_remote_mac(libnet_t *l, DWORD DestIP)
{
	HRESULT hr;
    ULONG   pulMac[6];
    ULONG   ulLen = 6;
	static PBYTE pbHexMac;
	PIP_ADAPTER_INFO pinfo = NULL;
	DWORD dwSize = 0;
	struct sockaddr_in sin;
	static BYTE bcastmac[]= {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

	BYTE *MAC = libnet_win32_read_arp_table(DestIP);
	
	if (MAC==NULL)
	{
		memset(pulMac, 0xff, sizeof (pulMac));
		memset(&sin, 0, sizeof(sin));
	    
		if((hr = SendARP (DestIP, 0, pulMac, &ulLen)) != NO_ERROR)
		{
			*(int32_t *)&sin.sin_addr = DestIP;
			GetAdaptersInfo(NULL, &dwSize);
			pinfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, dwSize);
			GetAdaptersInfo(pinfo, &dwSize);
			if(pinfo != NULL)
			{
				DestIP = inet_addr(pinfo->GatewayList.IpAddress.String);
				memset (pulMac, 0xff, sizeof (pulMac));
				ulLen = 6;
				if((hr = SendARP (DestIP, 0, pulMac, &ulLen)) != NO_ERROR)
				{
					GlobalFree(pinfo);
					return(bcastmac);
				}
			}
			else
			{
				GlobalFree(pinfo);
				return(bcastmac); /* ff:ff:ff:ff:ff:ff */
			}
		}
	  	
		pbHexMac = (PBYTE) pulMac;

		return (pbHexMac);
	}
	else
	{
		return (MAC);
	}
}

BYTE * libnet_win32_read_arp_table(DWORD DestIP)
{
	static BYTE buffMAC[6];
    BOOL fOrder=TRUE;
	BYTE *MAC=NULL;
	DWORD status, i, ci;
    
    PMIB_IPNETTABLE pIpNetTable = NULL;
	DWORD Size = 0;
	
	memset(buffMAC, 0, sizeof(buffMAC));

    if((status = GetIpNetTable(pIpNetTable, &Size, fOrder)) == ERROR_INSUFFICIENT_BUFFER)
    {
        pIpNetTable = (PMIB_IPNETTABLE) malloc(Size);
        assert(pIpNetTable);        
        status = GetIpNetTable(pIpNetTable, &Size, fOrder);
    }

	if(status == NO_ERROR)
	{
		/* set current interface */
		ci = pIpNetTable->table[0].dwIndex;

		for (i = 0; i < pIpNetTable->dwNumEntries; ++i)
		{
			if (pIpNetTable->table[i].dwIndex != ci)
			    ci = pIpNetTable->table[i].dwIndex;

			if(pIpNetTable->table[i].dwAddr == DestIP) /* found IP in arp cache */
			{
				memcpy(buffMAC, pIpNetTable->table[i].bPhysAddr, sizeof(buffMAC));
				free(pIpNetTable);
				return buffMAC;
			}        
		}
		  
		if (pIpNetTable)
            free (pIpNetTable);
		return(NULL);
	}
    else
    {
        if (pIpNetTable)
        {
            free (pIpNetTable);
        }
        MAC=NULL;
    }
    return(NULL);
}

/* EOF */
