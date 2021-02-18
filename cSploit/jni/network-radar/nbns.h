/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NBNS_H
#define NBNS_H

#include <stdint.h>

/// query
#define NBNS_OP_QUERY 0
/// registration
#define NBNS_OP_REGISTRATION 5
/// release
#define NBNS_OP_RELEASE 6
/// wack
#define NBNS_OP_WACK 7
/// refresh
#define NBNS_OP_REFRESH 8

/**
 * @brief Broadcast Flag.
 * 
 * 1: packet was broadcast or multicast
 * 0: unicast
 */
#define NBNS_FLAG_BROADCAST 0x01
/**
 * @brief Recursion Available Flag.
 * 
 * Only valid in responses from a NetBIOS Name
 * Server -- must be zero in all other
 * responses.
 *
 * If one (1) then the NBNS supports recursive
 * query, registration, and release.
 *
 * If zero (0) then the end-node must iterate
 * for query and challenge for registration. 
 */
#define NBNS_FLAG_RA 0x08

/**
 * @brief Recursion Desired Flag.
 * 
 * May only be set on a request to a NetBIOS
 * Name Server.
 *
 * The NBNS will copy its state into the
 * response packet.
 *
 * If one (1) the NBNS will iterate on the
 * query, registration, or release.
 */
#define NBNS_FLAG_RD 0x10

/**
 * @brief Truncation Flag.
 * 
 * Set if this message was truncated because the
 * datagram carrying it would be greater than
 * 576 bytes in length.  Use TCP to get the
 * information from the NetBIOS Name Server.
 */
#define NBNS_FLAG_TC 0x20

/**
 * @brief Authoritative Answer flag.
 * 
 * Must be zero (0) if ::nbnshdr.response is zero (0).
 *
 * If R flag is one (1) then if AA is one (1)
 * then the node responding is an authority for
 * the domain name.
 *
 * End nodes responding to queries always set
 * this bit in responses.
 */
#define NBNS_FLAG_AA 0x40

/**
 * @brief Format Error.
 * 
 * Request was invalidly formatted.
 */
#define NBNS_RCODE_FMT_ERR 0x1

/**
 * @brief Server failure.
 * 
 * Problem with NBNS, cannot process name.
 */
#define NBNS_RCODE_SRV_ERR 0x2
                       
/**
 * @brief Unsupported request error.
 * 
 * Allowable only for challenging NBNS when gets an Update type
 * registration request.
 */
#define NBNS_RCODE_IMP_ERR 0x4

/**
 * @brief Refused error.
 * 
 * For policy reasons server will not
 * register this name from this host.
 */
#define NBNS_RCODE_RFS_ERR 0x5
                       
/**
 * @brief Active error.
 * 
 * Name is owned by another node.
 */
#define NBNS_RCODE_ACT_ERR 0x6
/**
 * @brief Name in conflict error.
 * 
 * A UNIQUE name is owned by more than one node.
 */
#define NBNS_RCODE_CFT_ERR 0x7

/// NetBIOS Header ( RFC 1002, Paragraph 4.2.1.1 )
struct nbnshdr {
  
  /**
   * @brief Transaction ID for Name Service Transaction.
   * 
   *  Requestor places a unique value for each active
   *  transaction.  Responder puts NAME_TRN_ID value
   *  from request packet in response packet.
   */
  uint16_t trans_id;
  
  /**
   * @brief RESPONSE flag
   * 
   * if response == 0 then request packet
   * if response == 1 then response packet.
   */
  uint16_t response:1;
  
  /**
   * @brief Operation specifier
   * 
   * @see ::NBNS_OP_QUERY
   * @see ::NBNS_OP_REGISTRATION
   * @see ::NBNS_OP_RELEASE
   * @see ::NBNS_OP_WACK
   * @see ::NBNS_OP_REFRESH
   */
  uint16_t opcode:4;
  
  /**
   * @brief Flags for operation
   * 
   * @see ::NBNS_FLAG_BROADCAST
   * @see ::NBNS_FLAG_RA
   * @see ::NBNS_FLAG_RD
   * @see ::NBNS_FLAG_TC
   * @see ::NBNS_FLAG_AA
   */
  uint16_t flags:7;
  
  /**
   * @brief Response error code.
   * 
   * @see ::NBNS_RCODE_FMT_ERR
   * @see ::NBNS_RCODE_SRV_ERR
   * @see ::NBNS_RCODE_IMP_ERR
   * @see ::NBNS_RCODE_RFS_ERR
   * @see ::NBNS_RCODE_ACT_ERR
   * @see ::NBNS_RCODE_CFT_ERR
   */
  uint16_t rcode:4;
  
  /**
   * @brief Unsigned 16 bit integer specifying the number of 
   * entries in the question section of a Name.
   * 
   * Service packet.  Always zero (0) for responses.
   * Must be non-zero for all NetBIOS Name requests.
   */
  uint16_t qdcount;
  
  /**
   * @brief Unsigned 16 bit integer specifying the number of 
   * resource records in the answer section of a Name
   * Service packet.
   */
  uint16_t ancount;
  
  /**
   * @brief Unsigned 16 bit integer specifying the number of
   * resource records in the authority section of a
   * Name Service packet.
   */
  uint16_t nscount;
  
  /**
   * @brief Unsigned 16 bit integer specifying the number of
   * resource records in the additional records
   * section of a Name Service packet.
   */
  uint16_t arcount;
} __attribute__((__packed__));

/// Workstation Service (aka Machine Name or Client Service).
#define NBNS_NAME_PURPOSE_WORKSTATION 0x00
/// Messenger Service.
#define NBNS_NAME_PURPOSE_MSG 0x03
/// File Server Service.
#define NBNS_NAME_PURPOSE_FILE 0x20
/// Domain Master Browser.
#define NBNS_NAME_PURPOSE_DMB 0x1B

#define NBNS_NAME_SIZE 15

struct nbns_name {
  uint8_t name[NBNS_NAME_SIZE];
  uint8_t purpose;
} __attribute__((__packed__));

#define NBNS_NAME_L1_MAX_SIZE 63

struct nbns_name_L1 {
  uint8_t enc_name[NBNS_NAME_L1_MAX_SIZE];
};

struct nbns_L2 {
  /**
   * @brief flag that specify if this struct is a name or a pointer
   * 00 => ::nbns_name_L2
   * 11 => ::nbns_pointer_L2
   */
  uint8_t flag:2;
};

struct nbns_name_L2 {
  uint8_t flag:2; ///< must be set to 0x0
  uint8_t length:6; ///< ::nbns_name_L2.l1_name length
  struct nbns_name_L1 l1_name;
} __attribute__((__packed__));

struct nbns_pointer_L2 {
  uint16_t flag:2; ///< must be set to 0x3
  uint16_t pointer:14; ///< pointer to a ::nbns_name_L2 struct
} __attribute__((__packed__));

/*
 * NOTE: we cannot specify records as structs
 *       because they have a variable length 
 *       char array at the start,
 *       we can only define their tail.
 */

/// NetBIOS general Name Service Resource Record.
#define NBNS_QUESTION_TYPE_NB     0x0020
/// NetBIOS NODE STATUS Resource Record.
#define NBNS_QUESTION_TYPE_NBSTAT 0x0021
/// Internet class.
#define NBNS_QUESTION_CLASS_IN 0x0001

/// info at the end of a question record
struct nbns_question_tail {
  
  /**
   * @brief The type of request.
   * 
   * @see ::NBNS_QUESTION_TYPE_NB
   * @see ::NBNS_QUESTION_TYPE_NBSTAT
   */
  uint16_t type;
  
  /**
   * @brief The class of the request.
   * 
   * @see ::NBNS_QUESTION_CLASS_IN
   */
  uint16_t class;
} __attribute__((__packed__));


/// IP address Resource Record.
#define NBNS_RESOURCE_TYPE_A 0x0001
/// Name Server Resource Record
#define NBNS_RESOURCE_TYPE_NS 0x0002
/// NULL Resource Record.
#define NBNS_RESOURCE_TYPE_NULL 0x000A
/// NetBIOS general Name Service Resource Record.
#define NBNS_RESOURCE_TYPE_NB 0x0020
/// NetBIOS NODE STATUS Resource Record.
#define NBNS_RESOURCE_TYPE_NBSTAT 0x0021


/// Inetrnet class.
#define NBNS_RESOURCE_CLASS_IN 0x0001

/// info at the end of a resource record
struct nbns_resource_tail {
  /**
   * @brief Resource record type code.
   * 
   * @see ::NBNS_RESOURCE_TYPE_A
   * @see ::NBNS_RESOURCE_TYPE_NS
   * @see ::NBNS_RESOURCE_TYPE_NULL
   * @see ::NBNS_RESOURCE_TYPE_NB
   * @see ::NBNS_RESOURCE_TYPE_NBSTAT
   */
  uint16_t type;
  
  /**
   * @brief Resource record class code.
   * 
   * @see ::NBNS_RESOURCE_CLASS_IN
   */
  uint16_t class;
  
  /// The Time To Live of a the resource record's name.
  uint32_t ttl;
  
  /// ::nbns_resource_tail.rdat length
  uint16_t rdlength;
  
  /**
   * @brief ::nbns_resource_tail.rr_class and ::nbns_resource_tail.rr_type dependent field.
   * 
   * Contains the resource information for the NetBIOS name.
   */
  uint8_t rdata[];
} __attribute__((__packed__));

/// B node
#define NBNS_NAME_NODE_B 0x0

/// P node
#define NBNS_NAME_NODE_P 0x1

/// M node
#define NBNS_NAME_NODE_M 0x2

/// flags after a name node entry ( RFC 1002 4.2.18 )
struct nbns_node_name_tail {
  /**
   * @brief Group Name Flag.
   * 
   * If one (1) then the name is a GROUP NetBIOS name.
   * If zero (0) then it is a UNIQUE NetBIOS name.
   */
  uint16_t group:1;
  
  /**
   * @brief Owner Node Type
   * 
   * @see ::NBNS_NAME_FLAG_B_NODE
   * @see ::NBNS_NAME_FLAG_P_NODE
   * @see ::NBNS_NAME_FLAG_M_NODE
   */
  uint16_t node:2;
  
  /**
   * @brief Deregister Flag.
   * If one (1) then this name is in the process of being deleted.
   */
  uint16_t deregister:1;
  
  /**
   * @brief Conflict Flag.
   * If one (1) then name on this node is in conflict.
   */
  uint16_t conflict:1;

  /**
   * @brief Active Name Flag.
   * 
   * All entries have this flag set to one (1).
   */  
  uint16_t active:1;
  
  /**
   * @brief Permanent Name Flag.
   * 
   * If one (1) then entry is for the permanent node name.
   * Flag is zero (0) for all other names.
   */
  uint16_t permanent:1;

  uint16_t reserved:9;
} __attribute__((__packed__));

#define NBNS_NBSTATREQ_LEN 50

extern uint8_t nbns_nbstat_request[NBNS_NBSTATREQ_LEN];

/* ===== functions =====*/

char *nbns_get_status_name(struct nbnshdr *);

#endif
