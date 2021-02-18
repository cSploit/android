/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code
 * distributions retain the above copyright notice and this paragraph
 * in its entirety, and (2) distributions including binary code include
 * the above copyright notice and this paragraph in its entirety in
 * the documentation or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * Original code by Hannes Gredler (hannes@juniper.net)
 * Support for LMP service discovery extensions (defined by UNI 1.0) added
 * by Manu Pathak (mapathak@cisco.com), May 2005
 */

#ifndef lint
static const char rcsid[] _U_ =
    "@(#) $Header: /tcpdump/master/tcpdump/print-lmp.c,v 1.5.2.4 2006/06/23 02:07:27 hannes Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tcpdump-stdinc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "extract.h"
#include "addrtoname.h"
#include "gmpls.h"

/*
 * LMP common header
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Vers  |      (Reserved)       |    Flags      |    Msg Type   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |          LMP Length           |          (Reserved)           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

struct lmp_common_header {
    u_int8_t version_res[2];
    u_int8_t flags;   
    u_int8_t msg_type;
    u_int8_t length[2];
    u_int8_t reserved[2];
};

#define LMP_VERSION            1
#define	LMP_EXTRACT_VERSION(x) (((x)&0xf0)>>4) 

static const struct tok lmp_header_flag_values[] = {
    { 0x01, "Control Channel Down"},
    { 0x02, "LMP restart"},
    { 0, NULL}
};

static const struct tok lmp_obj_te_link_flag_values[] = {
    { 0x01, "Fault Management Supported"},
    { 0x02, "Link Verification Supported"},
    { 0, NULL}
};

static const struct tok lmp_obj_data_link_flag_values[] = {
    { 0x01, "Data Link Port"},
    { 0x02, "Allocated for user traffic"},
    { 0x04, "Failed link"},
    { 0, NULL}
};

static const struct tok lmp_obj_channel_status_values[] = {
    { 1, "Signal Okay"},
    { 2, "Signal Degraded"},
    { 3, "Signal Fail"},
    { 0, NULL}
};

static const struct tok lmp_obj_begin_verify_flag_values[] = {
    { 0x0001, "Verify all links"},
    { 0x0002, "Data link type"},
    { 0, NULL}
};

static const struct tok lmp_obj_begin_verify_error_values[] = {
    { 0x01, "\n\t\tLink Verification Procedure Not supported"},
    { 0x02, "\n\t\tUnwilling to verify"},
    { 0x04, "\n\t\tUnsupported verification transport mechanism"},
    { 0x08, "\n\t\tLink_Id configuration error"},
    { 0x10, "\n\t\tUnknown object c-type"},
    { 0, NULL}
};

static const struct tok lmp_obj_link_summary_error_values[] = {
    { 0x01, "\n\t\tUnacceptable non-negotiable LINK_SUMMARY parameters"},
    { 0x02, "\n\t\tRenegotiate LINK_SUMMARY parameters"},
    { 0x04, "\n\t\tInvalid TE-LINK Object"},
    { 0x08, "\n\t\tInvalid DATA-LINK Object"},
    { 0x10, "\n\t\tUnknown TE-LINK Object c-type"},
    { 0x20, "\n\t\tUnknown DATA-LINK Object c-type"},
    { 0, NULL}
};

/* Service Config Supported Protocols Flags */
static const struct tok lmp_obj_service_config_sp_flag_values[] = {
    { 0x01, "RSVP Supported"},
    { 0x02, "LDP Supported"},
    { 0, NULL}
};

/* Service Config Client Port Service Attribute Transparency Flags */
static const struct tok lmp_obj_service_config_cpsa_tp_flag_values[] = {
    { 0x01, "Path/VC Overhead Transparency Supported"},
    { 0x02, "Line/MS Overhead Transparency Supported"},
    { 0x04, "Section/RS Overhead Transparency Supported"},
    { 0, NULL}
};

/* Service Config Client Port Service Attribute Contiguous Concatenation Types Flags */
static const struct tok lmp_obj_service_config_cpsa_cct_flag_values[] = {
    { 0x01, "Contiguous Concatenation Types Supported"},
    { 0, NULL}
};

/* Service Config Network Service Attributes Transparency Flags */
static const struct tok lmp_obj_service_config_nsa_transparency_flag_values[] = {
    { 0x01, "Standard SOH/RSOH Transparency Supported"},
    { 0x02, "Standard LOH/MSOH Transparency Supported"},
    { 0, NULL}
};

/* Service Config Network Service Attributes TCM Monitoring Flags */
static const struct tok lmp_obj_service_config_nsa_tcm_flag_values[] = {
    { 0x01, "Transparent Tandem Connection Monitoring Supported"},
    { 0, NULL}
};

/* Network Service Attributes Network Diversity Flags */
static const struct tok lmp_obj_service_config_nsa_network_diversity_flag_values[] = {
    { 0x01, "Node Diversity Supported"},
    { 0x02, "Link Diversity Supported"},
    { 0x04, "SRLG Diversity Supported"},
    { 0, NULL}
};

#define	LMP_MSGTYPE_CONFIG                 1
#define	LMP_MSGTYPE_CONFIG_ACK             2
#define	LMP_MSGTYPE_CONFIG_NACK            3
#define	LMP_MSGTYPE_HELLO                  4
#define	LMP_MSGTYPE_VERIFY_BEGIN           5
#define	LMP_MSGTYPE_VERIFY_BEGIN_ACK       6
#define	LMP_MSGTYPE_VERIFY_BEGIN_NACK      7
#define LMP_MSGTYPE_VERIFY_END             8
#define LMP_MSGTYPE_VERIFY_END_ACK         9
#define LMP_MSGTYPE_TEST                  10
#define LMP_MSGTYPE_TEST_STATUS_SUCCESS   11
#define	LMP_MSGTYPE_TEST_STATUS_FAILURE   12
#define	LMP_MSGTYPE_TEST_STATUS_ACK       13
#define	LMP_MSGTYPE_LINK_SUMMARY          14
#define	LMP_MSGTYPE_LINK_SUMMARY_ACK      15
#define	LMP_MSGTYPE_LINK_SUMMARY_NACK     16
#define	LMP_MSGTYPE_CHANNEL_STATUS        17
#define	LMP_MSGTYPE_CHANNEL_STATUS_ACK    18
#define	LMP_MSGTYPE_CHANNEL_STATUS_REQ    19
#define	LMP_MSGTYPE_CHANNEL_STATUS_RESP   20
/* LMP Service Discovery message types defined by UNI 1.0 */
#define LMP_MSGTYPE_SERVICE_CONFIG        50
#define LMP_MSGTYPE_SERVICE_CONFIG_ACK    51
#define LMP_MSGTYPE_SERVICE_CONFIG_NACK   52

static const struct tok lmp_msg_type_values[] = {
    { LMP_MSGTYPE_CONFIG, "Config"},
    { LMP_MSGTYPE_CONFIG_ACK, "Config ACK"},
    { LMP_MSGTYPE_CONFIG_NACK, "Config NACK"},
    { LMP_MSGTYPE_HELLO, "Hello"},
    { LMP_MSGTYPE_VERIFY_BEGIN, "Begin Verify"},
    { LMP_MSGTYPE_VERIFY_BEGIN_ACK, "Begin Verify ACK"},
    { LMP_MSGTYPE_VERIFY_BEGIN_NACK, "Begin Verify NACK"},
    { LMP_MSGTYPE_VERIFY_END, "End Verify"},
    { LMP_MSGTYPE_VERIFY_END_ACK, "End Verify ACK"},
    { LMP_MSGTYPE_TEST, "Test"},
    { LMP_MSGTYPE_TEST_STATUS_SUCCESS, "Test Status Success"},
    { LMP_MSGTYPE_TEST_STATUS_FAILURE, "Test Status Failure"},
    { LMP_MSGTYPE_TEST_STATUS_ACK, "Test Status ACK"},
    { LMP_MSGTYPE_LINK_SUMMARY, "Link Summary"},
    { LMP_MSGTYPE_LINK_SUMMARY_ACK, "Link Summary ACK"},
    { LMP_MSGTYPE_LINK_SUMMARY_NACK, "Link Summary NACK"},
    { LMP_MSGTYPE_CHANNEL_STATUS, "Channel Status"},
    { LMP_MSGTYPE_CHANNEL_STATUS_ACK, "Channel Status ACK"},
    { LMP_MSGTYPE_CHANNEL_STATUS_REQ, "Channel Status Request"},
    { LMP_MSGTYPE_CHANNEL_STATUS_RESP, "Channel Status Response"},
    { LMP_MSGTYPE_SERVICE_CONFIG, "Service Config"},
    { LMP_MSGTYPE_SERVICE_CONFIG_ACK, "Service Config ACK"},
    { LMP_MSGTYPE_SERVICE_CONFIG_NACK, "Service Config NACK"},
    { 0, NULL}
};

/* 
 * LMP object header
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |N|   C-Type    |     Class     |            Length             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * //                       (object contents)                     //
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 */

struct lmp_object_header {
    u_int8_t ctype;
    u_int8_t class_num;
    u_int8_t length[2];
};

#define	LMP_OBJ_CC_ID                 1
#define	LMP_OBJ_NODE_ID               2
#define	LMP_OBJ_LINK_ID               3
#define	LMP_OBJ_INTERFACE_ID          4
#define	LMP_OBJ_MESSAGE_ID            5 
#define	LMP_OBJ_CONFIG                6
#define	LMP_OBJ_HELLO                 7
#define	LMP_OBJ_VERIFY_BEGIN          8
#define LMP_OBJ_VERIFY_BEGIN_ACK      9
#define LMP_OBJ_VERIFY_ID            10
#define LMP_OBJ_TE_LINK              11
#define LMP_OBJ_DATA_LINK            12
#define LMP_OBJ_CHANNEL_STATUS       13
#define LMP_OBJ_CHANNEL_STATUS_REQ   14
#define LMP_OBJ_ERROR_CODE           20

#define LMP_OBJ_SERVICE_CONFIG       51 /* defined in UNI 1.0 */

static const struct tok lmp_obj_values[] = {
    { LMP_OBJ_CC_ID, "Control Channel ID" },
    { LMP_OBJ_NODE_ID, "Node ID" },
    { LMP_OBJ_LINK_ID, "Link ID" },
    { LMP_OBJ_INTERFACE_ID, "Interface ID" },
    { LMP_OBJ_MESSAGE_ID, "Message ID" },
    { LMP_OBJ_CONFIG, "Configuration" },
    { LMP_OBJ_HELLO, "Hello" },
    { LMP_OBJ_VERIFY_BEGIN, "Verify Begin" },
    { LMP_OBJ_VERIFY_BEGIN_ACK, "Verify Begin ACK" },
    { LMP_OBJ_VERIFY_ID, "Verify ID" },
    { LMP_OBJ_TE_LINK, "TE Link" },
    { LMP_OBJ_DATA_LINK, "Data Link" },
    { LMP_OBJ_CHANNEL_STATUS, "Channel Status" },
    { LMP_OBJ_CHANNEL_STATUS_REQ, "Channel Status Request" },
    { LMP_OBJ_ERROR_CODE, "Error Code" },
    { LMP_OBJ_SERVICE_CONFIG, "Service Config" },

    { 0, NULL}
};

#define INT_SWITCHING_TYPE_SUBOBJ 1
#define WAVELENGTH_SUBOBJ         2

static const struct tok lmp_data_link_subobj[] = {
    { INT_SWITCHING_TYPE_SUBOBJ, "Interface Switching Type" },
    { WAVELENGTH_SUBOBJ        , "Wavelength" },
    { 0, NULL}
};

#define	LMP_CTYPE_IPV4       1
#define	LMP_CTYPE_IPV6       2

#define	LMP_CTYPE_LOC        1
#define	LMP_CTYPE_RMT        2
#define	LMP_CTYPE_UNMD       3
 
#define	LMP_CTYPE_IPV4_LOC   1
#define	LMP_CTYPE_IPV4_RMT   2
#define	LMP_CTYPE_IPV6_LOC   3
#define	LMP_CTYPE_IPV6_RMT   4
#define	LMP_CTYPE_UNMD_LOC   5  
#define	LMP_CTYPE_UNMD_RMT   6 

#define	LMP_CTYPE_1          1
#define	LMP_CTYPE_2          2

#define LMP_CTYPE_HELLO_CONFIG  1
#define LMP_CTYPE_HELLO         1

#define LMP_CTYPE_BEGIN_VERIFY_ERROR 1
#define LMP_CTYPE_LINK_SUMMARY_ERROR 2

/* C-Types for Service Config Object */
#define LMP_CTYPE_SERVICE_CONFIG_SP                   1
#define LMP_CTYPE_SERVICE_CONFIG_CPSA                 2
#define LMP_CTYPE_SERVICE_CONFIG_TRANSPARENCY_TCM     3
#define LMP_CTYPE_SERVICE_CONFIG_NETWORK_DIVERSITY    4

/* 
 * Different link types allowed in the Client Port Service Attributes
 * subobject defined for LMP Service Discovery in the UNI 1.0 spec
 */
#define LMP_SD_SERVICE_CONFIG_CPSA_LINK_TYPE_SDH     5 /* UNI 1.0 Sec 9.4.2 */
#define LMP_SD_SERVICE_CONFIG_CPSA_LINK_TYPE_SONET   6 /* UNI 1.0 Sec 9.4.2 */

/*
 * the ctypes are not globally unique so for
 * translating it to strings we build a table based
 * on objects offsetted by the ctype
 */

static const struct tok lmp_ctype_values[] = {
    { 256*LMP_OBJ_CC_ID+LMP_CTYPE_LOC, "Local" },
    { 256*LMP_OBJ_CC_ID+LMP_CTYPE_RMT, "Remote" },
    { 256*LMP_OBJ_NODE_ID+LMP_CTYPE_LOC, "Local" },
    { 256*LMP_OBJ_NODE_ID+LMP_CTYPE_RMT, "Remote" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_IPV4_LOC, "IPv4 Local" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_IPV4_RMT, "IPv4 Remote" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_IPV6_LOC, "IPv6 Local" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_IPV6_RMT, "IPv6 Remote" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_UNMD_LOC, "Unnumbered Local" },
    { 256*LMP_OBJ_LINK_ID+LMP_CTYPE_UNMD_RMT, "Unnumbered Remote" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_IPV4_LOC, "IPv4 Local" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_IPV4_RMT, "IPv4 Remote" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_IPV6_LOC, "IPv6 Local" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_IPV6_RMT, "IPv6 Remote" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_UNMD_LOC, "Unnumbered Local" },
    { 256*LMP_OBJ_INTERFACE_ID+LMP_CTYPE_UNMD_RMT, "Unnumbered Remote" },
    { 256*LMP_OBJ_MESSAGE_ID+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_MESSAGE_ID+LMP_CTYPE_2, "2" },
    { 256*LMP_OBJ_CONFIG+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_HELLO+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_VERIFY_BEGIN+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_VERIFY_BEGIN_ACK+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_VERIFY_ID+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_TE_LINK+LMP_CTYPE_IPV4, "IPv4" },
    { 256*LMP_OBJ_TE_LINK+LMP_CTYPE_IPV6, "IPv6" },
    { 256*LMP_OBJ_TE_LINK+LMP_CTYPE_UNMD, "Unnumbered" },
    { 256*LMP_OBJ_DATA_LINK+LMP_CTYPE_IPV4, "IPv4" },
    { 256*LMP_OBJ_DATA_LINK+LMP_CTYPE_IPV6, "IPv6" },
    { 256*LMP_OBJ_DATA_LINK+LMP_CTYPE_UNMD, "Unnumbered" },
    { 256*LMP_OBJ_CHANNEL_STATUS+LMP_CTYPE_IPV4, "IPv4" },
    { 256*LMP_OBJ_CHANNEL_STATUS+LMP_CTYPE_IPV6, "IPv6" },
    { 256*LMP_OBJ_CHANNEL_STATUS+LMP_CTYPE_UNMD, "Unnumbered" },
    { 256*LMP_OBJ_CHANNEL_STATUS_REQ+LMP_CTYPE_IPV4, "IPv4" },
    { 256*LMP_OBJ_CHANNEL_STATUS_REQ+LMP_CTYPE_IPV6, "IPv6" },
    { 256*LMP_OBJ_CHANNEL_STATUS_REQ+LMP_CTYPE_UNMD, "Unnumbered" },
    { 256*LMP_OBJ_ERROR_CODE+LMP_CTYPE_1, "1" },
    { 256*LMP_OBJ_ERROR_CODE+LMP_CTYPE_2, "2" },
    { 256*LMP_OBJ_SERVICE_CONFIG+LMP_CTYPE_SERVICE_CONFIG_SP, "1" },
    { 256*LMP_OBJ_SERVICE_CONFIG+LMP_CTYPE_SERVICE_CONFIG_CPSA, "2" },
    { 256*LMP_OBJ_SERVICE_CONFIG+LMP_CTYPE_SERVICE_CONFIG_TRANSPARENCY_TCM, "3" },
    { 256*LMP_OBJ_SERVICE_CONFIG+LMP_CTYPE_SERVICE_CONFIG_NETWORK_DIVERSITY, "4" },
    { 0, NULL}
};

void
lmp_print(register const u_char *pptr, register u_int len) {

    const struct lmp_common_header *lmp_com_header;
    const struct lmp_object_header *lmp_obj_header;
    const u_char *tptr,*obj_tptr;
    int tlen,lmp_obj_len,lmp_obj_ctype,obj_tlen;
    int hexdump;
    int offset,subobj_type,subobj_len,total_subobj_len;
    int link_type;

    union { /* int to float conversion buffer */
        float f; 
        u_int32_t i;
    } bw;

    tptr=pptr;
    lmp_com_header = (const struct lmp_common_header *)pptr;
    TCHECK(*lmp_com_header);

    /*
     * Sanity checking of the header.
     */
    if (LMP_EXTRACT_VERSION(lmp_com_header->version_res[0]) != LMP_VERSION) {
	printf("LMP version %u packet not supported",
               LMP_EXTRACT_VERSION(lmp_com_header->version_res[0]));
	return;
    }

    /* in non-verbose mode just lets print the basic Message Type*/
    if (vflag < 1) {
        printf("LMPv%u %s Message, length: %u",
               LMP_EXTRACT_VERSION(lmp_com_header->version_res[0]),
               tok2str(lmp_msg_type_values, "unknown (%u)",lmp_com_header->msg_type),
               len);
        return;
    }

    /* ok they seem to want to know everything - lets fully decode it */

    tlen=EXTRACT_16BITS(lmp_com_header->length);

    printf("\n\tLMPv%u, msg-type: %s, Flags: [%s], length: %u",
           LMP_EXTRACT_VERSION(lmp_com_header->version_res[0]),
           tok2str(lmp_msg_type_values, "unknown, type: %u",lmp_com_header->msg_type),
           bittok2str(lmp_header_flag_values,"none",lmp_com_header->flags),
           tlen);

    tptr+=sizeof(const struct lmp_common_header);
    tlen-=sizeof(const struct lmp_common_header);

    while(tlen>0) {
        /* did we capture enough for fully decoding the object header ? */
        if (!TTEST2(*tptr, sizeof(struct lmp_object_header)))
            goto trunc;

        lmp_obj_header = (const struct lmp_object_header *)tptr;
        lmp_obj_len=EXTRACT_16BITS(lmp_obj_header->length);
        lmp_obj_ctype=(lmp_obj_header->ctype)&0x7f;

        if(lmp_obj_len % 4 || lmp_obj_len < 4)
            return;

        printf("\n\t  %s Object (%u), Class-Type: %s (%u) Flags: [%snegotiable], length: %u",
               tok2str(lmp_obj_values,
                       "Unknown",
                       lmp_obj_header->class_num),
               lmp_obj_header->class_num,
               tok2str(lmp_ctype_values,
                       "Unknown",
                       ((lmp_obj_header->class_num)<<8)+lmp_obj_ctype),
               lmp_obj_ctype,
               (lmp_obj_header->ctype)&0x80 ? "" : "non-",
               lmp_obj_len);

        obj_tptr=tptr+sizeof(struct lmp_object_header);
        obj_tlen=lmp_obj_len-sizeof(struct lmp_object_header);

        /* did we capture enough for fully decoding the object ? */
        if (!TTEST2(*tptr, lmp_obj_len))
            goto trunc;
        hexdump=FALSE;

        switch(lmp_obj_header->class_num) {

        case LMP_OBJ_CC_ID:
            switch(lmp_obj_ctype) {
            case LMP_CTYPE_LOC:
            case LMP_CTYPE_RMT:
                printf("\n\t    Control Channel ID: %u (0x%08x)",
                       EXTRACT_32BITS(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;

            default:
                hexdump=TRUE;
            }
            break;

        case LMP_OBJ_LINK_ID:
        case LMP_OBJ_INTERFACE_ID:
            switch(lmp_obj_ctype) {
            case LMP_CTYPE_IPV4_LOC:
            case LMP_CTYPE_IPV4_RMT:
                printf("\n\t    IPv4 Link ID: %s (0x%08x)",
                       ipaddr_string(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;
#ifdef INET6
            case LMP_CTYPE_IPV6_LOC:
            case LMP_CTYPE_IPV6_RMT:
                printf("\n\t    IPv6 Link ID: %s (0x%08x)",
                       ip6addr_string(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;
#endif
            case LMP_CTYPE_UNMD_LOC:
            case LMP_CTYPE_UNMD_RMT:
                printf("\n\t    Link ID: %u (0x%08x)",
                       EXTRACT_32BITS(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;
            default:
                hexdump=TRUE;
            }
            break;

        case LMP_OBJ_MESSAGE_ID:
            switch(lmp_obj_ctype) {
            case LMP_CTYPE_1:
                printf("\n\t    Message ID: %u (0x%08x)",
                       EXTRACT_32BITS(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;
            case LMP_CTYPE_2:
                printf("\n\t    Message ID Ack: %u (0x%08x)",
                       EXTRACT_32BITS(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;
            default:
                hexdump=TRUE;
            }
            break;

        case LMP_OBJ_NODE_ID:
            switch(lmp_obj_ctype) {
            case LMP_CTYPE_LOC:
            case LMP_CTYPE_RMT:
                printf("\n\t    Node ID: %s (0x%08x)",
                       ipaddr_string(obj_tptr),
                       EXTRACT_32BITS(obj_tptr));
                break;

            default:
                hexdump=TRUE;
            }
            break;

        case LMP_OBJ_CONFIG:
            switch(lmp_obj_ctype) {
            case LMP_CTYPE_HELLO_CONFIG:
                printf("\n\t    Hello Interval: %u\n\t    Hello Dead Interval: %u",
                       EXTRACT_16BITS(obj_tptr),
                       EXTRACT_16BITS(obj_tptr+2));
                break;

            default:
                hexdump=TRUE;
            }
            break;
	
        case LMP_OBJ_HELLO:
            switch(lmp_obj_ctype) {
	    case LMP_CTYPE_HELLO:
                printf("\n\t    TxSeqNum: %u\n\t    RcvSeqNum: %u",
                       EXTRACT_32BITS(obj_tptr),
                       EXTRACT_32BITS(obj_tptr+4));
                break;

            default:
                hexdump=TRUE;
            }
            break;      
	    
        case LMP_OBJ_TE_LINK:
		printf("\n\t    Flags: [%s]",
		bittok2str(lmp_obj_te_link_flag_values,
			"none",
			EXTRACT_16BITS(obj_tptr)>>8));
            
	    switch(lmp_obj_ctype) {
	    case LMP_CTYPE_IPV4:
		printf("\n\t    Local Link-ID: %s (0x%08x) \
			\n\t    Remote Link-ID: %s (0x%08x)",
                       ipaddr_string(obj_tptr+4),
                       EXTRACT_32BITS(obj_tptr+4),
                       ipaddr_string(obj_tptr+8),
                       EXTRACT_32BITS(obj_tptr+8));
		break;
		
#ifdef INET6
	    case LMP_CTYPE_IPV6:
#endif
	    case LMP_CTYPE_UNMD:
            default:
                hexdump=TRUE;
            }
            break;
	
        case LMP_OBJ_DATA_LINK:
		printf("\n\t    Flags: [%s]",
		bittok2str(lmp_obj_data_link_flag_values,
			"none",
			EXTRACT_16BITS(obj_tptr)>>8));
            
	    switch(lmp_obj_ctype) {
	    case LMP_CTYPE_IPV4:
	    case LMP_CTYPE_UNMD:
                printf("\n\t    Local Interface ID: %s (0x%08x) \
			\n\t    Remote Interface ID: %s (0x%08x)",
                       ipaddr_string(obj_tptr+4),
                       EXTRACT_32BITS(obj_tptr+4),
                       ipaddr_string(obj_tptr+8),
                       EXTRACT_32BITS(obj_tptr+8));
		
		total_subobj_len = lmp_obj_len - 16;	 
		offset = 12;
		while (total_subobj_len > 0 && hexdump == FALSE ) {
			subobj_type = EXTRACT_16BITS(obj_tptr+offset)>>8;
			subobj_len  = EXTRACT_16BITS(obj_tptr+offset)&0x00FF;
			printf("\n\t    Subobject, Type: %s (%u), Length: %u",
				tok2str(lmp_data_link_subobj,
					"Unknown",
					subobj_type),
					subobj_type,
					subobj_len);
			switch(subobj_type) {
			case INT_SWITCHING_TYPE_SUBOBJ:
				printf("\n\t      Switching Type: %s (%u)",
					tok2str(gmpls_switch_cap_values, 
						"Unknown", 
						EXTRACT_16BITS(obj_tptr+offset+2)>>8),
					EXTRACT_16BITS(obj_tptr+offset+2)>>8);
				printf("\n\t      Encoding Type: %s (%u)",
					tok2str(gmpls_encoding_values, 
						"Unknown", 
						EXTRACT_16BITS(obj_tptr+offset+2)&0x00FF),
					EXTRACT_16BITS(obj_tptr+offset+2)&0x00FF);
				bw.i = EXTRACT_32BITS(obj_tptr+offset+4);
				printf("\n\t      Min Reservable Bandwidth: %.3f Mbps",
                                       bw.f*8/1000000);
				bw.i = EXTRACT_32BITS(obj_tptr+offset+8);
				printf("\n\t      Max Reservable Bandwidth: %.3f Mbps",
                                       bw.f*8/1000000);
				break;	
			case WAVELENGTH_SUBOBJ:
				printf("\n\t      Wavelength: %u",
					EXTRACT_32BITS(obj_tptr+offset+4));
				break;
			default:
				/* Any Unknown Subobject ==> Exit loop */
				hexdump=TRUE;
				break;
			}
			total_subobj_len-=subobj_len;
			offset+=subobj_len;
		}
		
		break;
#ifdef INET6   
	    case LMP_CTYPE_IPV6:
#endif
            default:
                hexdump=TRUE;
            }
            break;      
	    
        case LMP_OBJ_VERIFY_BEGIN:
	    switch(lmp_obj_ctype) {
            case LMP_CTYPE_1:
		printf("\n\t    Flags: %s",
		bittok2str(lmp_obj_begin_verify_flag_values,
			"none",
			EXTRACT_16BITS(obj_tptr)));
		printf("\n\t    Verify Interval: %u",
			EXTRACT_16BITS(obj_tptr+2));
		printf("\n\t    Data links: %u",
			EXTRACT_32BITS(obj_tptr+4));
                printf("\n\t    Encoding type: %s",
			tok2str(gmpls_encoding_values, "Unknown", *(obj_tptr+8)));
                printf("\n\t    Verify Tranport Mechanism: %u (0x%x) %s",
			EXTRACT_16BITS(obj_tptr+10),
			EXTRACT_16BITS(obj_tptr+10),
			EXTRACT_16BITS(obj_tptr+10)&8000 ? "(Payload test messages capable)" : "");
                bw.i = EXTRACT_32BITS(obj_tptr+12);
		printf("\n\t    Transmission Rate: %.3f Mbps",bw.f*8/1000000);
		printf("\n\t    Wavelength: %u",
			EXTRACT_32BITS(obj_tptr+16));
		break;
		
            default:
                hexdump=TRUE;
            }
            break;      
	
        case LMP_OBJ_VERIFY_BEGIN_ACK:
	    switch(lmp_obj_ctype) {
            case LMP_CTYPE_1:
                printf("\n\t    Verify Dead Interval: %u 	\
			\n\t    Verify Transport Response: %u",
                       EXTRACT_16BITS(obj_tptr),
                       EXTRACT_16BITS(obj_tptr+2));
                break;
		
            default:
                hexdump=TRUE;
            }
            break;      
        
	case LMP_OBJ_VERIFY_ID:
	    switch(lmp_obj_ctype) {
            case LMP_CTYPE_1:
                printf("\n\t    Verify ID: %u",
                       EXTRACT_32BITS(obj_tptr));
                break;
		
            default:
                hexdump=TRUE;
            }
            break;      
        
	case LMP_OBJ_CHANNEL_STATUS:
            switch(lmp_obj_ctype) {
	    case LMP_CTYPE_IPV4:
	    case LMP_CTYPE_UNMD:
		offset = 0;
		/* Decode pairs: <Interface_ID (4 bytes), Channel_status (4 bytes)> */
		while (offset < (lmp_obj_len-(int)sizeof(struct lmp_object_header)) ) {
			printf("\n\t    Interface ID: %s (0x%08x)",
			ipaddr_string(obj_tptr+offset),
			EXTRACT_32BITS(obj_tptr+offset));
			
			printf("\n\t\t    Active: %s (%u)", 		(EXTRACT_32BITS(obj_tptr+offset+4)>>31) ? 
						"Allocated" : "Non-allocated",
				(EXTRACT_32BITS(obj_tptr+offset+4)>>31));
			
			printf("\n\t\t    Direction: %s (%u)", (EXTRACT_32BITS(obj_tptr+offset+4)>>30)&0x1 ? 
						"Transmit" : "Receive",
				(EXTRACT_32BITS(obj_tptr+offset+4)>>30)&0x1);	
						
			printf("\n\t\t    Channel Status: %s (%u)",
					tok2str(lmp_obj_channel_status_values,
			 		"Unknown",
					EXTRACT_32BITS(obj_tptr+offset+4)&0x3FFFFFF),
			EXTRACT_32BITS(obj_tptr+offset+4)&0x3FFFFFF);
			offset+=8;
		}
                break;
#ifdef INET6       
	    case LMP_CTYPE_IPV6:
#endif
            default:
                hexdump=TRUE;
            }
            break;      
        
	case LMP_OBJ_CHANNEL_STATUS_REQ:
            switch(lmp_obj_ctype) {
	    case LMP_CTYPE_IPV4:
	    case LMP_CTYPE_UNMD:
		offset = 0;
		while (offset < (lmp_obj_len-(int)sizeof(struct lmp_object_header)) ) {
			printf("\n\t    Interface ID: %s (0x%08x)",
			ipaddr_string(obj_tptr+offset),
			EXTRACT_32BITS(obj_tptr+offset));
			offset+=4;
		}
                break;
#ifdef INET6       
	    case LMP_CTYPE_IPV6:
#endif
	    default:
                hexdump=TRUE;
            }
            break;      
	
        case LMP_OBJ_ERROR_CODE:
	    switch(lmp_obj_ctype) {
            case LMP_CTYPE_BEGIN_VERIFY_ERROR:
		printf("\n\t    Error Code: %s",
		bittok2str(lmp_obj_begin_verify_error_values,
			"none",
			EXTRACT_32BITS(obj_tptr)));
                break;
		
            case LMP_CTYPE_LINK_SUMMARY_ERROR:
		printf("\n\t    Error Code: %s",
		bittok2str(lmp_obj_link_summary_error_values,
			"none",
			EXTRACT_32BITS(obj_tptr)));
                break;
            default:
                hexdump=TRUE;
            }
            break;      

	case LMP_OBJ_SERVICE_CONFIG:
	    switch (lmp_obj_ctype) {
	    case LMP_CTYPE_SERVICE_CONFIG_SP:
		
		printf("\n\t Flags: %s",
		       bittok2str(lmp_obj_service_config_sp_flag_values,
				  "none", 
				  EXTRACT_16BITS(obj_tptr)>>8));

		printf("\n\t  UNI Version: %u",
		       EXTRACT_16BITS(obj_tptr) & 0x00FF);

		break;
		
            case LMP_CTYPE_SERVICE_CONFIG_CPSA:
		
		link_type = EXTRACT_16BITS(obj_tptr)>>8;
		
		printf("\n\t Link Type: %s (%u)",
		       tok2str(lmp_sd_service_config_cpsa_link_type_values,
			       "Unknown", link_type),
		       link_type);
		
		if (link_type == LMP_SD_SERVICE_CONFIG_CPSA_LINK_TYPE_SDH) {
		    printf("\n\t Signal Type: %s (%u)",
			   tok2str(lmp_sd_service_config_cpsa_signal_type_sdh_values,
				   "Unknown",
				   EXTRACT_16BITS(obj_tptr) & 0x00FF),
			   EXTRACT_16BITS(obj_tptr) & 0x00FF);
		}
		
		if (link_type == LMP_SD_SERVICE_CONFIG_CPSA_LINK_TYPE_SONET) {
		    printf("\n\t Signal Type: %s (%u)",
			   tok2str(lmp_sd_service_config_cpsa_signal_type_sonet_values,
				   "Unknown",
				   EXTRACT_16BITS(obj_tptr) & 0x00FF),
			   EXTRACT_16BITS(obj_tptr) & 0x00FF);
		}
		
		printf("\n\t Transparency: %s",
		       bittok2str(lmp_obj_service_config_cpsa_tp_flag_values,
				  "none",
				  EXTRACT_16BITS(obj_tptr+2)>>8));
		
		printf("\n\t Contiguous Concatenation Types: %s",
		       bittok2str(lmp_obj_service_config_cpsa_cct_flag_values,
				  "none",
				  EXTRACT_16BITS(obj_tptr+2)>>8 & 0x00FF));
		
		printf("\n\t Minimum NCC: %u",
		       EXTRACT_16BITS(obj_tptr+4));
		
		printf("\n\t Maximum NCC: %u",
		       EXTRACT_16BITS(obj_tptr+6));
		
		printf("\n\t Minimum NVC:%u",
		       EXTRACT_16BITS(obj_tptr+8));
		
		printf("\n\t Maximum NVC:%u",
		       EXTRACT_16BITS(obj_tptr+10));
		
		printf("\n\t    Local Interface ID: %s (0x%08x)",
		       ipaddr_string(obj_tptr+12),
		       EXTRACT_32BITS(obj_tptr+12));
		
		break;
		
	    case LMP_CTYPE_SERVICE_CONFIG_TRANSPARENCY_TCM:
		
		printf("\n\t Transparency Flags: %s",
		       bittok2str(
			   lmp_obj_service_config_nsa_transparency_flag_values,
			   "none",
			   EXTRACT_32BITS(obj_tptr)));

		printf("\n\t TCM Monitoring Flags: %s",
		       bittok2str(
			   lmp_obj_service_config_nsa_tcm_flag_values,
			   "none",
			   EXTRACT_16BITS(obj_tptr+6) & 0x00FF));
		
		break;
		
	    case LMP_CTYPE_SERVICE_CONFIG_NETWORK_DIVERSITY:
		
		printf("\n\t Diversity: Flags: %s",
		       bittok2str(
			   lmp_obj_service_config_nsa_network_diversity_flag_values,
			   "none",
			   EXTRACT_16BITS(obj_tptr+2) & 0x00FF));
		break;

	    default:
		hexdump = TRUE;
	    };

	break;

        default:
            if (vflag <= 1)
                print_unknown_data(obj_tptr,"\n\t    ",obj_tlen);
            break;
        }
        /* do we want to see an additionally hexdump ? */
        if (vflag > 1 || hexdump==TRUE)
            print_unknown_data(tptr+sizeof(sizeof(struct lmp_object_header)),"\n\t    ",
                               lmp_obj_len-sizeof(struct lmp_object_header));

        tptr+=lmp_obj_len;
        tlen-=lmp_obj_len;
    }
    return;
trunc:
    printf("\n\t\t packet exceeded snapshot");
}
