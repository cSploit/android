/*
    ncplib.h
    Copyright (C) 1995, 1996 by Volker Lendecke
    Copyright (C) 1997-2001  Petr Vandrovec

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:

	0.00  1995			Volker Lendecke
		Initial revision.

	0.01  1995-1999			Petr Vandrovec <vandrove@vc.cvut.cz>
					Dave Woodhouse <dave@imladris.demon.co.uk>
					Roumen Petrov <rpetrov@usa.net>
					Arne de Bruijn <arne@knoware.nl>
		New APIs
		Big-endian support
		Glibc2 support
		Glibc2.1 support

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
		
	1.01  2000, May 17		Bruce Richardson <brichardson@lineone.net>
		Added ncp_perms_to_str and ncp_str_to_perms.
		
	1.02  2000, May 24		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWLogoutFromFileServer.

	1.03  2001, September 15	Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixes for SWIG. Unwind nested structs so that names are defined
		here and not by SWIG.

	1.04  2001, November 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Next set of SWIG fixes.

	1.05  2001, December 12		Hans Grobler <grobh@sun.ac.za>
		Added NCP_PERM_ALL, ncp_ns_delete_entry and full NET_ADDRESS_TYPE
		definition.

 */

#ifndef _NCPLIB_H
#define _NCPLIB_H

#include <ncp/ncp.h>
#include <ncp/ext/socket.h>
#include <sys/param.h>
#include <stdio.h>
#include <time.h>

#include <ncp/ipxlib.h>

typedef u_int8_t byte;
typedef u_int16_t word;
typedef u_int32_t dword;

typedef int32_t NWCCODE;

typedef enum NET_ADDRESS_TYPE {
	NT_UNKNOWN	       = -1,
	NT_IPX                 = 0,
	NT_IP                  = 1,
	NT_SDLC                = 2,
	NT_TOKENRING_ETHERNET  = 3,
	NT_OSI                 = 4,
	NT_APPLETALK           = 5,
	NT_NETBEUI             = 6,
	NT_SOCKADDR            = 7,
	NT_UDP                 = 8,
	NT_TCP                 = 9,
	NT_UDP6                = 10,
	NT_TCP6                = 11,
	NT_INTERNAL            = 12,
	NT_URL                 = 13,
	NT_COUNT               = 14
} NET_ADDRESS_TYPE;

#ifdef SWIG
/* ncp_off64_t is defined to double by Perl interface */
#else
typedef u_int64_t ncp_off64_t;
#endif

#define BVAL(buf,pos) (((const u_int8_t *)(buf))[pos])
#define BWVAL(buf,pos) (((u_int8_t*)(buf))[pos])
#define PVAL(buf,pos) ((unsigned int)BVAL(buf,pos))
#define BSET(buf,pos,val) (BWVAL(buf,pos) = (val))

#ifdef __cplusplus
extern "C" {
#endif

/* we know that the 386 can handle misalignment and has the "right" 
   byteorder */
#if defined(__i386__)

static inline word
WVAL_LH(const void * buf, int pos)
{
	return *((const word *) (((const u_int8_t*)buf) + pos));
}

static inline dword
DVAL_LH(const void * buf, int pos)
{
	return *((const dword *) (((const u_int8_t*)buf) + pos));
}

static inline u_int64_t
QVAL_LH(const void * buf, int pos)
{
	return *((const u_int64_t *) (((const u_int8_t*)buf) + pos));
}

static inline void
WSET_LH(void * buf, int pos, word val)
{
	*((word *) (((u_int8_t*)buf) + pos)) = val;
}

static inline void
DSET_LH(void * buf, int pos, dword val)
{
	*((dword *) (((u_int8_t*)buf) + pos)) = val;
}

static inline void
QSET_LH(void * buf, int pos, u_int64_t val)
{
	*((u_int64_t *) (((u_int8_t*)buf) + pos)) = val;
}

static inline word
WVAL_HL(const void * buf, int pos)
{
	return ntohs(WVAL_LH(buf, pos));
}

static inline dword
DVAL_HL(const void * buf, int pos)
{
	return ntohl(DVAL_LH(buf, pos));
}

static inline void
WSET_HL(void * buf, int pos, word val)
{
	WSET_LH(buf, pos, htons(val));
}

static inline void
DSET_HL(void * buf, int pos, dword val)
{
	DSET_LH(buf, pos, htonl(val));
}

static inline __attribute__((always_inline)) void
QSET_HL(void * buf, int pos, u_int64_t val) {
	DSET_HL(buf, pos, val >> 32);
	DSET_HL(buf, pos + 4, val);
}

#else

static inline word
WVAL_LH(const void * buf, int pos)
{
	return PVAL(buf, pos) | PVAL(buf, pos + 1) << 8;
}

static inline dword
DVAL_LH(const void * buf, int pos)
{
	return WVAL_LH(buf, pos) | WVAL_LH(buf, pos + 2) << 16;
}

static inline u_int64_t
QVAL_LH(const void * buf, int pos)
{
	return DVAL_LH(buf, pos) | (((u_int64_t)DVAL_LH(buf, pos + 4)) << 32);
}

static inline void
WSET_LH(void * buf, int pos, word val)
{
	BSET(buf, pos, val & 0xff);
	BSET(buf, pos + 1, val >> 8);
}

static inline void
DSET_LH(void * buf, int pos, dword val)
{
	WSET_LH(buf, pos, val & 0xffff);
	WSET_LH(buf, pos + 2, val >> 16);
}

static inline void
QSET_LH(void * buf, int pos, u_int64_t val)
{
	DSET_LH(buf, pos, val);
	DSET_LH(buf, pos + 4, val >> 32);
}

static inline word
WVAL_HL(const void * buf, int pos)
{
	return PVAL(buf, pos) << 8 | PVAL(buf, pos + 1);
}

static inline dword
DVAL_HL(const void * buf, int pos)
{
	return WVAL_HL(buf, pos) << 16 | WVAL_HL(buf, pos + 2);
}

static inline void
WSET_HL(void * buf, int pos, word val)
{
	BSET(buf, pos, val >> 8);
	BSET(buf, pos + 1, val & 0xff);
}

static inline void
DSET_HL(void * buf, int pos, dword val)
{
	WSET_HL(buf, pos, val >> 16);
	WSET_HL(buf, pos + 2, val & 0xffff);
}

static inline void
QSET_HL(void * buf, int pos, u_int64_t val) {
	DSET_HL(buf, pos, val >> 32);
	DSET_HL(buf, pos + 4, val);
}

#endif

static inline u_int64_t
SVAL_LH(const void * buf, int pos)
{
	return DVAL_LH(buf, pos) | (((u_int64_t)WVAL_LH(buf, pos + 4)) << 32);
}

void
str_upper(char *name);

enum connect_state
{
	NOT_CONNECTED = 0,
	CONN_PERMANENT,
	CONN_TEMPORARY,
	CONN_KERNELBASED
};

#define NCP_CONN_INVALID	0
#define NCP_CONN_PERMANENT	1
#define NCP_CONN_TEMPORARY	2
#define NCP_CONN_KERNELBASED	3

#define NCPFS_MAX_CFG_USERNAME	256

/* This is abstract type now. Use NWCCGetConnInfo instead */
struct ncp_conn;

typedef struct ncp_conn * NWCONN_HANDLE;

int ncp_get_fid(NWCONN_HANDLE);

#ifdef SWIG
struct ncp_conn_spec
{
	fixedCharArray server[NCP_BINDERY_NAME_LEN];
	fixedCharArray user[NCPFS_MAX_CFG_USERNAME];
	int uid;
	int login_type;
	fixedCharArray password[NCP_BINDERY_NAME_LEN];
};
#else
struct ncp_conn_spec
{
	char server[NCP_BINDERY_NAME_LEN];
	char user[NCPFS_MAX_CFG_USERNAME];
	uid_t uid;
	int login_type;		/* NCP_BINDERY_USER / NCP_BINDERY_PSERVER */
	char password[NCP_BINDERY_NAME_LEN];
};
#endif

struct ncp_search_seq
{
	struct nw_search_sequence s;
	int name_space;		/* RYP: namespace is reserved word for new C++ compilers (gcc 2.7x, egcc 2.9x, borland cbuilder) */
};

#ifdef SWIG
struct ncp_property_info
{
	fixedCharArray property_name[16];
	u_int8_t property_flags;
	u_int8_t property_security;
	u_int32_t search_instance;
	u_int8_t value_available_flag;
	u_int8_t more_properties_flag;
};
#else
struct ncp_property_info
{
	u_int8_t property_name[16];
	u_int8_t property_flags;
	u_int8_t property_security;
	u_int32_t search_instance;
	u_int8_t value_available_flag;
	u_int8_t more_properties_flag;
};
#endif

/* ncp_initialize is the main entry point for user programs which want
   to connect to a NetWare Server. It looks for -S, -U, -P and -n in
   the argument list, opens the connection and removes the arguments
   from the list. It was designed after the X Windows init
   functions. */
NWCONN_HANDLE ncp_initialize(int *argc, char **argv, int login_necessary, long *err);

/* You can login as another object by this procedure. As a first use
   pserver comes to mind. */
NWCONN_HANDLE ncp_initialize_as(int *argc, char **argv,
		   int login_necessary, int login_type, long *err);

/* You can login as another object by this procedure. As a first use
   pserver comes to mind. If required = 0 and none of -S,-U,-P is
   specified, NULL is returned regardless of configuration files */
NWCONN_HANDLE ncp_initialize_2(int *argc, char **argv, int login_necessary, 
		  int login_type, long *err, int required);


NWCCODE ncp_login_conn(NWCONN_HANDLE conn, const char *username, NWObjectType object_type, const char *password);

/* Open a connection */
NWCONN_HANDLE ncp_open(const struct ncp_conn_spec *spec, long *err);

/* Open a connection on an existing fd - it accepts only ncpfs fd now.
   In future, it can accept also connected IPX, IP and NCP socket (I'll see) */
int
ncp_open_fd(int fd, NWCONN_HANDLE *conn);

/* Open a connection on an existing mount point */
int ncp_open_mount(const char *mount_point, NWCONN_HANDLE * conn);

/* Find a permanent connection that fits the spec, return NULL if
 * there is none. */
char *
ncp_find_permanent(const struct ncp_conn_spec *spec);

/* Find the address of a file server */
long
ncp_find_fileserver(const char *server_name, struct sockaddr* addr, socklen_t addrlen);

/* Find the address of a server */
long
ncp_find_server(const char **server_name, int type, struct sockaddr* addr, socklen_t addrlen);

#ifdef MAKE_NCPLIB
/* Find the address of a server */
NWCCODE
ncp_find_server_addr(const char **server_name, int type, struct sockaddr* addr, socklen_t addrlen, unsigned int transport);
#endif

/* Detach from a permanent connection or destroy a temporary
   connection */
long ncp_close(NWCONN_HANDLE conn);

/* like getmntent, ncp_get_conn_ent scans /etc/mtab for usable
   connections */

#ifdef SWIG
struct ncp_conn_ent
{
	fixedCharArray server[NCP_BINDERY_NAME_LEN];
	char* user;
	uid_t uid;
	fixedCharArray mount_point[MAXPATHLEN];
};
#else
struct ncp_conn_ent
{
	char server[NCP_BINDERY_NAME_LEN];
	char* user;
	uid_t uid;
	char mount_point[MAXPATHLEN];
};
#endif

struct ncp_conn_ent *
ncp_get_conn_ent(FILE * filep);

#ifdef SWIG
#define NWCLIENT ".nwclient"
#define NWC_NOPASSWORD "-"
#else
#define NWCLIENT (".nwclient")
#define NWC_NOPASSWORD ("-")
#endif

/* find an appropriate connection */

struct ncp_conn_spec *
 ncp_find_conn_spec(const char *server, const char *user, const char *password,
		    int login_necessary, uid_t uid, long *err);

struct ncp_conn_spec *
 ncp_find_conn_spec2(const char *server, const char *user, const char *password,
		    int login_necessary, uid_t uid, int allow_multiple_conns, 
		    long *err);

NWCCODE
ncp_find_conn_spec3(const char *server, const char *user, const char *password,
		    int login_necessary, uid_t uid, int allow_multiple_conns, 
		    struct ncp_conn_spec *spec);

long ncp_get_file_server_description_strings(NWCONN_HANDLE conn, char descstring[512]);

long ncp_get_file_server_time(NWCONN_HANDLE conn, time_t * target);
long ncp_set_file_server_time(NWCONN_HANDLE conn, time_t * source);

#ifdef SWIG
struct ncp_file_server_info
{
	fixedCharArray ServerName[48];
	int FileServiceVersion;
	int FileServiceSubVersion;
	int MaximumServiceConnections;
	int ConnectionsInUse;
	int NumberMountedVolumes;
	int Revision;
	int SFTLevel;
	int TTSLevel;
	int MaxConnectionsEverUsed;
	int AccountVersion;
	int VAPVersion;
	int QueueVersion;
	int PrintVersion;
	int VirtualConsoleVersion;
	int RestrictionLevel;
	int InternetBridge;
	fixedArray Reserved[60];
};
#else
struct ncp_file_server_info
{
	u_int8_t ServerName[48];
	u_int8_t FileServiceVersion;
	u_int8_t FileServiceSubVersion;
	u_int16_t MaximumServiceConnections __attribute__((packed));
	u_int16_t ConnectionsInUse __attribute__((packed));
	u_int16_t NumberMountedVolumes __attribute__((packed));
	u_int8_t Revision;
	u_int8_t SFTLevel;
	u_int8_t TTSLevel;
	u_int16_t MaxConnectionsEverUsed __attribute__((packed));
	u_int8_t AccountVersion;
	u_int8_t VAPVersion;
	u_int8_t QueueVersion;
	u_int8_t PrintVersion;
	u_int8_t VirtualConsoleVersion;
	u_int8_t RestrictionLevel;
	u_int8_t InternetBridge;
	u_int8_t Reserved[60];
};
#endif

struct ncp_file_server_info_2 {
#ifdef SWIG
	fixedArray ServerName[49];
#else
	u_int8_t ServerName[49];
#endif	
	u_int8_t FileServiceVersion;
	u_int8_t FileServiceSubVersion;
	u_int16_t MaximumServiceConnections;
	u_int16_t ConnectionsInUse;
	u_int16_t NumberMountedVolumes;
	u_int8_t Revision;
	u_int8_t SFTLevel;
	u_int8_t TTSLevel;
	u_int16_t MaxConnectionsEverUsed;
	u_int8_t AccountVersion;
	u_int8_t VAPVersion;
	u_int8_t QueueVersion;
	u_int8_t PrintVersion;
	u_int8_t VirtualConsoleVersion;
	u_int8_t RestrictionLevel;
	u_int8_t InternetBridge;
	u_int8_t MixedModePathFlag;
	u_int8_t LocalLoginInfoCcode;
	u_int16_t ProductMajorVersion;
	u_int16_t ProductMinorVersion;
	u_int16_t ProductRevisionVersion;
	u_int8_t OSLanguageID;
	u_int8_t _64BitOffsetsSupportedFlag;
};

long ncp_get_file_server_information(NWCONN_HANDLE conn,
				struct ncp_file_server_info *target);

NWCCODE ncp_get_file_server_information_2(NWCONN_HANDLE conn,
				struct ncp_file_server_info_2 *target, size_t tsize);

long ncp_get_connlist(NWCONN_HANDLE conn,
		  u_int16_t object_type, const char *object_name,
		  int *returned_no, u_int8_t conn_numbers[256]);

long
 ncp_get_stations_logged_info(NWCONN_HANDLE conn,
			      u_int32_t connection,
			      struct ncp_bindery_object *target,
			      time_t * login_time);

long
 ncp_get_internet_address(NWCONN_HANDLE conn,
			  u_int32_t connection,
			  struct sockaddr *station_addr,
			  u_int8_t * conn_type);

long
 ncp_send_broadcast(NWCONN_HANDLE conn,
		    u_int8_t no_conn, const u_int8_t * connections,
		    const char *message);

long
 ncp_send_broadcast2(NWCONN_HANDLE conn,
		     unsigned int no_conn, const unsigned int* connections,
		     const char* message);

long
 ncp_get_encryption_key(NWCONN_HANDLE conn,
			char *encryption_key);
long
 ncp_get_bindery_object_id(NWCONN_HANDLE conn,
			   NWObjectType object_type,
			   const char *object_name,
			   struct ncp_bindery_object *target);
long
 ncp_get_bindery_object_name(NWCONN_HANDLE conn,
			     NWObjectID object_id,
			     struct ncp_bindery_object *target);
NWCCODE NWScanObject(NWCONN_HANDLE conn, const char *searchName,
		     NWObjectType searchType, NWObjectID *objID,
		     char objName[NCP_BINDERY_NAME_LEN + 1], NWObjectType *objType,
		     u_int8_t *hasPropertiesFlag, u_int8_t *objFlags,
		     u_int8_t *objSecurity);

long
 ncp_scan_bindery_object(NWCONN_HANDLE conn,
			 NWObjectID last_id, NWObjectType object_type,
			 const char *search_string,
			 struct ncp_bindery_object *target);
long
 ncp_create_bindery_object(NWCONN_HANDLE conn,
			   NWObjectType object_type,
			   const char *object_name,
			   u_int8_t object_security,
			   u_int8_t object_status);
long
 ncp_delete_bindery_object(NWCONN_HANDLE conn,
			   NWObjectType object_type,
			   const char *object_name);

long
 ncp_change_object_security(NWCONN_HANDLE conn,
			    u_int16_t object_type,
			    const char *object_name,
			    u_int8_t security);

struct ncp_station_addr
{
	u_int32_t NetWork __attribute__((packed));
#ifdef SWIG
	fixedArray Node[6];
#else
	u_int8_t Node[6];
#endif
	u_int16_t Socket __attribute__((packed));
};

struct ncp_prop_login_control
{
#ifdef SWIG
	fixedArray AccountExpireDate[3];
#else
	u_int8_t AccountExpireDate[3];
#endif	
	u_int8_t Disabled;
#ifdef SWIG
	fixedArray PasswordExpireDate[3];
#else	
	u_int8_t PasswordExpireDate[3];
#endif	
	u_int8_t GraceLogins;
	u_int16_t PasswordExpireInterval __attribute__((packed));
	u_int8_t MaxGraceLogins;
	u_int8_t MinPasswordLength;
	u_int16_t MaxConnections __attribute__((packed));
#ifdef SWIG
	fixedArray ConnectionTimeMask[42] __attribute__((packed));
	fixedArray LastLogin[6] __attribute__((packed));
#else	
	u_int8_t ConnectionTimeMask[42];
	u_int8_t LastLogin[6];
#endif	
	u_int8_t RestrictionMask;
	u_int8_t reserved;
	u_int32_t MaxDiskUsage __attribute__((packed));
	u_int16_t BadLoginCount __attribute__((packed));
	u_int32_t BadLoginCountDown __attribute__((packed));
	struct ncp_station_addr LastIntruder;
};

NWCCODE NWReadPropertyValue(NWCONN_HANDLE conn, const char *objName,
			    NWObjectType objType, const char *propertyName,
			    unsigned int segmentNum, u_int8_t *segmentData,
			    u_int8_t *moreSegments, u_int8_t *flags);

long
 ncp_read_property_value(NWCONN_HANDLE conn,
			 NWObjectType object_type, const char *object_name,
			 int segment, const char *prop_name,
			 struct nw_property *target);
long
 ncp_scan_property(NWCONN_HANDLE conn,
		   NWObjectType object_type, const char *object_name,
		   NWObjectID last_id, const char *search_string,
		   struct ncp_property_info *target);
long
 ncp_add_object_to_set(NWCONN_HANDLE conn,
		       NWObjectType object_type, const char *object_name,
		       const char *property_name,
		       NWObjectType member_type,
		       const char *member_name);
long
 ncp_change_property_security(NWCONN_HANDLE conn,
			      NWObjectType object_type, const char *object_name,
			      const char *property_name,
			      u_int8_t property_security);
long
 ncp_create_property(NWCONN_HANDLE conn,
		     NWObjectType object_type, const char *object_name,
		     const char *property_name,
		     u_int8_t property_flags,
		     u_int8_t property_security);
long
 ncp_delete_object_from_set(NWCONN_HANDLE conn,
			    NWObjectType object_type, const char *object_name,
			    const char *property_name,
			    NWObjectType member_type,
			    const char *member_name);
long
 ncp_delete_property(NWCONN_HANDLE conn,
		     NWObjectType object_type, const char *object_name,
		     const char *property_name);
long
 ncp_write_property_value(NWCONN_HANDLE conn,
			  NWObjectType object_type, const char *object_name,
			  const char *property_name,
			  u_int8_t segment,
			  const struct nw_property *property_value);

/* Bit masks for security flag */
#define NCP_SEC_CHECKSUMMING_REQUESTED        (1)
#define NCP_SEC_SIGNATURE_REQUESTED           (2)
#define NCP_SEC_COMPLETE_SIGNATURES_REQUESTED (4)
#define NCP_SEC_ENCRYPTION_REQUESTED          (8)
#define NCP_SEC_LIP_DISABLED                (128)

long
 ncp_get_big_ncp_max_packet_size(NWCONN_HANDLE conn,
				 u_int16_t proposed_max_size,
				 u_int8_t proposed_security_flag,
				 u_int16_t * accepted_max_size,
				 u_int16_t * echo_socket,
				 u_int8_t * accepted_security_flag);

long
 ncp_login_encrypted(NWCONN_HANDLE conn,
		     const struct ncp_bindery_object *object,
		     const unsigned char *key,
		     const unsigned char *passwd);

long
 ncp_login_unencrypted(NWCONN_HANDLE conn,
		       NWObjectType object_type, const char *object_name,
		       const unsigned char *passwd);

long
 ncp_change_login_passwd(NWCONN_HANDLE conn,
			 const struct ncp_bindery_object *object,
			 const unsigned char *key,
			 const unsigned char *oldpasswd,
			 const unsigned char *newpasswd);

#define NCP_GRACE_PERIOD (0xdf)

#define NCPLIB_ERROR			(0x8700)
/* ~/.nwclient is group/world writeable or readable */
#define NCPLIB_INVALID_MODE		(NCPLIB_ERROR | 0x01)
/* ncp_get_namespace_info_element called with itemid not in nsrim */
#define NCPLIB_INFORMATION_NOT_KNOWN	(NCPLIB_ERROR | 0x02)
/* neither fixed nor variable bit requested */
#define NCPLIB_NSFORMAT_INVALID		(NCPLIB_ERROR | 0x03)
/* referrals disabled, but referral is needed ... */
#define NCPLIB_REFERRAL_NEEDED		(NCPLIB_ERROR | 0x04)
/* ncpd refused to do its work... */
#define NCPLIB_NCPD_DEAD		(NCPLIB_ERROR | 0x05)
#define NCPLIB_PASSWORD_REQUIRED	(NCPLIB_ERROR | 0x06)

#define NWE_REQUESTER_ERROR		(0x8800)
#define NWE_REQ_TOO_MANY_REQ_FRAGS	(NWE_REQUESTER_ERROR | 0x0C)
#define NWE_BUFFER_OVERFLOW		(NWE_REQUESTER_ERROR | 0x0E)
#define NWE_SERVER_NO_CONN		(NWE_REQUESTER_ERROR | 0x0F)
#define NWE_SCAN_COMPLETE		(NWE_REQUESTER_ERROR | 0x12)
#define NWE_UNSUPPORTED_NAME_FORMAT_TYP	(NWE_REQUESTER_ERROR | 0x13)
#define NWE_INVALID_NCP_PACKET_LENGTH	(NWE_REQUESTER_ERROR | 0x16)
#define NWE_BUFFER_INVALID_LEN		(NWE_REQUESTER_ERROR | 0x33)	/* buffer underflow */
#define NWE_USER_NO_NAME		(NWE_REQUESTER_ERROR | 0x34)
#define NWE_PARAM_INVALID		(NWE_REQUESTER_ERROR | 0x36)
#define NWE_SERVER_NOT_FOUND		(NWE_REQUESTER_ERROR | 0x47)
#define NWE_SIGNATURE_LEVEL_CONFLICT	(NWE_REQUESTER_ERROR | 0x61)
#define NWE_INVALID_LEVEL		(NWE_REQUESTER_ERROR | 0x6B)
#define NWE_UNSUPPORTED_TRAN_TYPE	(NWE_REQUESTER_ERROR | 0x70)
#define NWE_UNSUPPORTED_AUTHENTICATOR	(NWE_REQUESTER_ERROR | 0x73)
#define NWE_REQUESTER_FAILURE		(NWE_REQUESTER_ERROR | 0xFF)

#define NWE_SERVER_ERROR		(0x8900)
#define NWE_VOL_INVALID			(NWE_SERVER_ERROR | 0x98)
#define NWE_DIRHANDLE_INVALID		(NWE_SERVER_ERROR | 0x9B)
#define NWE_LOGIN_LOCKOUT		(NWE_SERVER_ERROR | 0xC5)
#define NWE_Q_NO_RIGHTS			(NWE_SERVER_ERROR | 0xD3)
#define NWE_Q_NO_JOB			(NWE_SERVER_ERROR | 0xD5)
#define NWE_Q_NO_JOB_RIGHTS		(NWE_SERVER_ERROR | 0xD6)
#define NWE_PASSWORD_UNENCRYPTED	(NWE_SERVER_ERROR | 0xD6)
#define NWE_PASSWORD_NOT_UNIQUE		(NWE_SERVER_ERROR | 0xD7)
#define NWE_PASSWORD_TOO_SHORT		(NWE_SERVER_ERROR | 0xD8)
#define NWE_LOGIN_MAX_EXCEEDED		(NWE_SERVER_ERROR | 0xD9)
#define NWE_LOGIN_UNAUTHORIZED_TIME	(NWE_SERVER_ERROR | 0xDA)
#define NWE_LOGIN_UNAUTHORIZED_STATION	(NWE_SERVER_ERROR | 0xDB)
#define NWE_ACCT_DISABLED		(NWE_SERVER_ERROR | 0xDC)
#define NWE_PASSWORD_INVALID		(NWE_SERVER_ERROR | 0xDE)
#define NWE_PASSWORD_EXPIRED		(NWE_SERVER_ERROR | 0xDF)
#define NWE_BIND_MEMBER_ALREADY_EXISTS	(NWE_SERVER_ERROR | 0xE9)
#define NWE_NCP_NOT_SUPPORTED		(NWE_SERVER_ERROR | 0xFB)
#define NWE_SERVER_UNKNOWN		(NWE_SERVER_ERROR | 0xFC)
#define NWE_CONN_NUM_INVALID		(NWE_SERVER_ERROR | 0xFD)
#define NWE_SERVER_FAILURE		(NWE_SERVER_ERROR | 0xFF)

const char* strnwerror(int err);

long
 ncp_login_user(NWCONN_HANDLE conn,
		const unsigned char *username,
		const unsigned char *password);

long
 ncp_get_volume_info_with_number(NWCONN_HANDLE conn, int n,
				 struct ncp_volume_info *target);

long
 ncp_get_volume_number(NWCONN_HANDLE conn, const char *name,
		       int *target);

long
 ncp_file_search_init(NWCONN_HANDLE conn,
		      int dir_handle, const char *path,
		      struct ncp_filesearch_info *target);

long
 ncp_file_search_continue(NWCONN_HANDLE conn,
			  struct ncp_filesearch_info *fsinfo,
			  int attributes, const char *path,
			  struct ncp_file_info *target);

long
 ncp_get_finfo(NWCONN_HANDLE conn,
	       int dir_handle, const char *path, const char *name,
	       struct ncp_file_info *target);

long
 ncp_open_file(NWCONN_HANDLE conn,
	       int dir_handle, const char *path,
	       int attr, int accessm,
	       struct ncp_file_info *target);

long ncp_close_file(NWCONN_HANDLE conn, const char fileHandle[6]);

long
 ncp_create_newfile(NWCONN_HANDLE conn,
		    int dir_handle, const char *path,
		    int attr,
		    struct ncp_file_info *target);

long
 ncp_create_file(NWCONN_HANDLE conn,
		 int dir_handle, const char *path,
		 int attr,
		 struct ncp_file_info *target);

long
 ncp_erase_file(NWCONN_HANDLE conn,
		int dir_handle, const char *path,
		int attr);

long
 ncp_rename_file(NWCONN_HANDLE conn,
		 int old_handle, const char *old_path,
		 int attr,
		 int new_handle, const char *new_path);

long
 ncp_create_directory(NWCONN_HANDLE conn,
		      int dir_handle, const char *path,
		      int inherit_mask);

long
 ncp_delete_directory(NWCONN_HANDLE conn,
		      int dir_handle, const char *path);

long
 ncp_rename_directory(NWCONN_HANDLE conn,
		      int dir_handle,
		      const char *old_path, const char *new_path);

#ifdef SWIG
long
 ncp_get_trustee(NWCONN_HANDLE conn, NWObjectID object_id,
		 u_int8_t vol, fixedCharArray OUTPUT[256],
		 u_int16_t * OUTPUT, u_int16_t * REFERENCE);
#else
long
 ncp_get_trustee(NWCONN_HANDLE conn, NWObjectID object_id,
		 u_int8_t vol, char *path,
		 u_int16_t * trustee, u_int16_t * contin);
#endif

long
 ncp_add_trustee(NWCONN_HANDLE conn,
		 int dir_handle, const char *path,
		 NWObjectID object_id, u_int8_t rights);

long
 ncp_delete_trustee(NWCONN_HANDLE conn,
		    int dir_handle, const char *path,
		    NWObjectID object_id);

#ifdef SWIG
long
 ncp_read(NWCONN_HANDLE conn, const char fileHandle[6],
	  off_t offset, size_t count, char *RETBUFFER_LENPREV);

long
 ncp_write(NWCONN_HANDLE conn, const char fileHandle[6],
	   off_t offset, size_t IGNORE, const char *STRING_LENPREV);

long
 ncp_copy_file(NWCONN_HANDLE conn,
	       const fixedArray source_file[6],
	       const fixedArray target_file[6],
	       u_int32_t source_offset,
	       u_int32_t target_offset,
	       u_int32_t count,
	       u_int32_t * OUTPUT);
#else
long
 ncp_read(NWCONN_HANDLE conn, const char fileHandle[6],
	  off_t offset, size_t count, char *target);

long
 ncp_write(NWCONN_HANDLE conn, const char fileHandle[6],
	   off_t offset, size_t count, const char *source);

NWCCODE ncp_read64(NWCONN_HANDLE conn, const char fileHandle[6],
		ncp_off64_t offset, size_t count, void *target, size_t *bytesread);

NWCCODE ncp_write64(NWCONN_HANDLE conn, const char fileHandle[6],
		ncp_off64_t offset, size_t count, const void *source, size_t *byteswritten);

long
 ncp_copy_file(NWCONN_HANDLE conn,
	       const char source_file[6],
	       const char target_file[6],
	       u_int32_t source_offset,
	       u_int32_t target_offset,
	       u_int32_t count,
	       u_int32_t * copied_count);
#endif

#define SA_NORMAL	(0x0000)
#define SA_HIDDEN	(0x0002)
#define SA_SYSTEM	(0x0004)
#define SA_SUBDIR_ONLY	(0x0010)
#define SA_SUBDIR_FILES	(0x8000)
#define SA_ALL		(SA_SUBDIR_FILES | SA_SYSTEM | SA_HIDDEN)
#define SA_SUBDIR_ALL	(SA_SUBDIR_ONLY | SA_SYSTEM | SA_HIDDEN)
#define SA_FILES_ALL	(SA_NORMAL | SA_SYSTEM | SA_HIDDEN)

#define NCP_DIRSTYLE_HANDLE     0x00
#define NCP_DIRSTYLE_DIRBASE    0x01
#define NCP_DIRSTYLE_NOHANDLE   0xFF

#define NCP_PATH_STD		-1

long
 ncp_obtain_file_or_subdir_info(NWCONN_HANDLE conn,
				u_int8_t source_ns, u_int8_t target_ns,
				u_int16_t search_attribs, u_int32_t rim,
				u_int8_t vol, u_int32_t dirent, 
				const char *path,
				struct nw_info_struct *target);

#define NCP_PERM_READ   (0x001)
#define NCP_PERM_WRITE  (0x002)
#define NCP_PERM_OPEN   (0x004)
#define NCP_PERM_CREATE (0x008)
#define NCP_PERM_DELETE (0x010)
#define NCP_PERM_OWNER  (0x020)
#define NCP_PERM_SEARCH (0x040)
#define NCP_PERM_MODIFY (0x080)
#define NCP_PERM_SUPER  (0x100)
#define NCP_PERM_ALL    (0x1fb)

long
 ncp_get_eff_directory_rights(NWCONN_HANDLE conn,
			      u_int8_t source_ns,
			      u_int8_t target_ns,
			      u_int16_t search_attribs,
			      u_int8_t vol, u_int32_t dirent, const char *path,
			      u_int16_t * my_effective_rights);

long
ncp_do_lookup2(NWCONN_HANDLE conn,
	       u_int8_t _source_ns,
	       const struct nw_info_struct *dir,
	       const char *path,       /* may only be one component */
	       u_int8_t _target_ns,
	       struct nw_info_struct *target);

long
ncp_do_lookup(NWCONN_HANDLE conn,
	      const struct nw_info_struct *dir,
	      const char *path,	/* may only be one component */
	      struct nw_info_struct *target);

long
 ncp_modify_file_or_subdir_dos_info(NWCONN_HANDLE conn,
				    const struct nw_info_struct *file,
				    u_int32_t info_mask,
				    const struct nw_modify_dos_info *info);

long
 ncp_del_file_or_subdir(NWCONN_HANDLE conn,
			const struct nw_info_struct *dir, const char *name);


long
 ncp_open_create_file_or_subdir(NWCONN_HANDLE conn,
				const struct nw_info_struct *dir,
				const char *name,
				int open_create_mode,
				u_int32_t create_attributes,
				int desired_acc_rights,
				struct nw_file_info *target);

long
 ncp_initialize_search(NWCONN_HANDLE conn,
		       const struct nw_info_struct *dir,
		       int name_space,
		       struct ncp_search_seq *target);

long
 ncp_initialize_search2(NWCONN_HANDLE conn,
		        const struct nw_info_struct *dir,
		        int name_space,
		        const unsigned char *enc_subpath, int subpathlen,
		        struct ncp_search_seq *target);

long
ncp_search_for_file_or_subdir2(NWCONN_HANDLE conn,
			       int search_attributes,
			       u_int32_t RIM,
			       struct ncp_search_seq *seq,
			       struct nw_info_struct *target);

long
 ncp_search_for_file_or_subdir(NWCONN_HANDLE conn,
			       struct ncp_search_seq *seq,
			       struct nw_info_struct *target);

long
 ncp_ren_or_mov_file_or_subdir(NWCONN_HANDLE conn,
			       const struct nw_info_struct *old_dir, const char *old_name,
			       const struct nw_info_struct *new_dir, const char *new_name);

long
 ncp_create_queue_job_and_file(NWCONN_HANDLE conn,
			       NWObjectID queue_id,
			       struct queue_job *job);

long
 ncp_get_queue_length(NWCONN_HANDLE conn,
                      NWObjectID queue_id,
                      u_int32_t *queue_length);

long 
 ncp_get_queue_job_ids(NWCONN_HANDLE conn,
                       NWObjectID queue_id,
                       u_int32_t queue_section,
                       u_int32_t *length1,
                       u_int32_t *length2,
                       u_int32_t ids[]);
long 
 ncp_get_queue_job_info(NWCONN_HANDLE conn,
                        NWObjectID queue_id,
                        u_int32_t job_id,
                        struct nw_queue_job_entry *target);

long
NWRemoveJobFromQueue2(NWCONN_HANDLE conn, NWObjectID queue_id,
			u_int32_t job_id);
                        
long
 ncp_close_file_and_start_job(NWCONN_HANDLE conn,
			      NWObjectID queue_id,
			      const struct queue_job *job);

long
 ncp_attach_to_queue(NWCONN_HANDLE conn, NWObjectID queue_id);

long
 ncp_detach_from_queue(NWCONN_HANDLE conn, NWObjectID queue_id);

long
 ncp_service_queue_job(NWCONN_HANDLE conn, NWObjectID queue_id, 
 		       u_int16_t job_type, struct queue_job *target);

long
 ncp_finish_servicing_job(NWCONN_HANDLE conn, NWObjectID queue_id,
			  u_int32_t job_number, u_int32_t charge_info);
			  
long
 ncp_change_job_position(NWCONN_HANDLE conn, NWObjectID queue_id,
			  u_int32_t job_number, unsigned int position);

long
 ncp_abort_servicing_job(NWCONN_HANDLE conn, NWObjectID queue_id,
			 u_int32_t job_number);

NWCCODE NWChangeQueueJobEntry(NWCONN_HANDLE conn, NWObjectID queue_id,
			      const struct nw_queue_job_entry *jobdata);

long
 ncp_get_broadcast_message(NWCONN_HANDLE conn, char message[256]);

long
 ncp_dealloc_dir_handle(NWCONN_HANDLE conn, u_int8_t dir_handle);

#define NCP_ALLOC_PERMANENT (0x0000)
#define NCP_ALLOC_TEMPORARY (0x0001)
#define NCP_ALLOC_SPECIAL   (0x0002)

long
 ncp_alloc_short_dir_handle2(NWCONN_HANDLE conn,
			     u_int8_t _namespace,
			     const struct nw_info_struct *dir,
			     u_int16_t alloc_mode,
			     u_int8_t * target);

long
 ncp_alloc_short_dir_handle(NWCONN_HANDLE conn,
			    const struct nw_info_struct *dir,
			    u_int16_t alloc_mode,
			    u_int8_t * target);

long
 ncp_get_effective_dir_rights(NWCONN_HANDLE conn,
			      const struct nw_info_struct *file,
			      u_int16_t * target);

struct ncp_trustee_struct
{
	NWObjectID object_id;
	u_int16_t rights;
};

long
ncp_add_trustee_set(NWCONN_HANDLE conn,
		     u_int8_t volume_number, u_int32_t dir_entry,
		     u_int16_t rights_mask,
		     int object_count, 
		     const struct ncp_trustee_struct *rights);

struct ncp_deleted_file
{
	int32_t		seq;
	u_int32_t	vol;
	u_int32_t	base;
};

long
ncp_ns_scan_salvageable_file(NWCONN_HANDLE conn, u_int8_t src_ns,
			     int dirstyle, u_int8_t vol_num, 
			     u_int32_t dir_base,
			     const unsigned char* encpath, int pathlen,
			     struct ncp_deleted_file* finfo,
			     char* retname, int retname_maxlen);

long
ncp_ns_purge_file(NWCONN_HANDLE conn, const struct ncp_deleted_file* finfo);

long
ncp_ns_get_full_name(NWCONN_HANDLE conn, u_int8_t src_ns, u_int8_t dst_ns,
	             int dirstyle, u_int8_t vol_num, u_int32_t dir_base,
		     const unsigned char* encpath, size_t pathlen,
	             char* retname, size_t retname_maxlen);

int
ncp_get_conn_type(NWCONN_HANDLE conn);

int
ncp_get_conn_number(NWCONN_HANDLE conn);

NWCCODE
ncp_get_dentry_ttl(NWCONN_HANDLE conn, unsigned int* ttl);

NWCCODE
ncp_set_dentry_ttl(NWCONN_HANDLE conn, unsigned int ttl);

/* What to do with them?! Simply do not use them if you compiled libncp
   without NDS support */
long
ncp_send_nds_frag(NWCONN_HANDLE conn,
    int ndsverb,
    const char *inbuf, size_t inbuflen,
    char *outbuf, size_t outbufsize, size_t *outbuflen);

long
ncp_send_nds(NWCONN_HANDLE conn, int fn,
 const char *data_in, size_t data_in_len, 
 char *data_out, size_t data_out_max, size_t *data_out_len);

long
ncp_change_conn_state(NWCONN_HANDLE conn, int new_state);
/* end of NDS specific... */

NWCONN_HANDLE 
ncp_open_addr(const struct sockaddr *target, long *err);

int
ncp_path_to_NW_format(const char* path, unsigned char* encbuff, int encbuffsize);

long
ncp_obtain_file_or_subdir_info2(NWCONN_HANDLE conn, u_int8_t source_ns,
			u_int8_t target_ns, u_int16_t search_attribs, 
			u_int32_t rim, int dir_style, u_int8_t vol, 
			u_int32_t dirent, 
			const unsigned char* encpath, int pathlen,
			struct nw_info_struct* target);

typedef unsigned int NWVOL_NUM;
typedef unsigned int NWDIR_HANDLE;
typedef u_int32_t NWDIR_ENTRY;

struct NSI_Name {
#ifdef SWIG
%pragma(swig) readonly
	size_t				NameLength;
%pragma(swig) readwrite
	size_tLenPrefixCharArray	Name[256];
#else
	size_t		NameLength;
	char		Name[256];
#endif
};
	
struct NSI_Attributes {
	u_int32_t	Attributes;
	u_int16_t	Flags;
};

struct NSI_TotalSize {
	u_int32_t	TotalAllocated;
	size_t		Datastreams;
};

struct NSI_ExtAttrInfo {
	u_int32_t	DataSize;
	u_int32_t	Count;
	u_int32_t	KeySize;
};

struct NSI_Change {
	u_int16_t	Date;
	u_int16_t	Time;
	NWObjectID	ID;
};

struct NSI_Directory {
	NWDIR_ENTRY	dirEntNum;
	NWDIR_ENTRY	DosDirNum;
	NWVOL_NUM	volNumber;
};

struct NSI_DatastreamFATInfo {
	u_int32_t	Number;
	u_int32_t	FATBlockSize;
};

struct NSI_DatastreamSizes {
	size_t	NumberOfDatastreams;
	struct NSI_DatastreamFATInfo ds[0];
};

struct NSI_DatastreamInfo {
	u_int32_t	Number;
	ncp_off64_t	Size;	/* !! was 32bit in ncpfs-2.2.0.18, but
	                           this type was not used in API !! */
};

struct NSI_DatastreamLogicals {
	size_t	NumberOfDatastreams;
	struct NSI_DatastreamInfo ds[0];
};

struct NSI_DOSName {
#ifdef SWIG
%pragma(swig) readonly
	size_t				NameLength;
%pragma(swig) readwrite
	size_tLenPrefixCharArray	Name[16];
#else
	size_t		NameLength;	/* 0..14 */
	char		Name[16];	/* align... why not... */
#endif
};

struct NSI_MacTimes {
	int32_t CreateTime;
	int32_t BackupTime;
};

struct NSI_Modify {
	struct NSI_Change Modify;
	struct NSI_Change LastAccess;
};

struct NSI_LastAccess {
	u_int16_t	Date;
	u_int16_t	Time;	/* NW5.00+ */
};

#ifdef SWIG
struct nw_info_struct2 {
	ncp_off64_t	SpaceAllocated;
	struct NSI_Attributes Attributes;
	ncp_off64_t	DataSize;
	struct NSI_TotalSize TotalSize;
	struct NSI_ExtAttrInfo ExtAttrInfo;
	struct NSI_Change Archive;
	struct NSI_Change Modify;
	struct NSI_Change Creation;
	struct NSI_LastAccess LastAccess;
	u_int32_t	OwningNamespace;
	struct NSI_Directory Directory;
	u_int16_t	Rights;	/* inherited */
			
	u_int16_t	ReferenceID;
	u_int32_t	NSAttributes;
	/* datastream actual cannot be retrieved this way... */
	/* datastream logical ... dtto ... */
	int32_t		UpdateTime; /* seconds, relative to year 2000... */ /* NW4.11 */
	struct NSI_DOSName DOSName;	/* NW4.11 */
	u_int32_t	FlushTime;	/* NW4.11 */
	u_int32_t	ParentBaseID;	/* NW4.11 */
	fixedArray	MacFinderInfo[32]; /* NW4.11 */
	u_int32_t	SiblingCount;	/* NW4.11 */
	u_int32_t	EffectiveRights; /* NW4.11 */
	struct NSI_MacTimes MacTimes;	/* NW4.11 */
	u_int16_t	Unknown25;
	u_int16_t	reserved3;
	u_int32_t	reserved1[7];	/* future expansion... */
	struct NSI_Name	Name;
	fixedArray	reserved2[512];	/* future expansion... (unicode name?) */
};
#else
struct nw_info_struct2 {
	ncp_off64_t	SpaceAllocated;
	struct NSI_Attributes Attributes;
	ncp_off64_t	DataSize;
	struct NSI_TotalSize TotalSize;
	struct NSI_ExtAttrInfo ExtAttrInfo;
	struct NSI_Change Archive;
	struct NSI_Change Modify;
	struct NSI_Change Creation;
	struct NSI_LastAccess LastAccess;
	u_int32_t	OwningNamespace;
	struct NSI_Directory Directory;
	u_int16_t	Rights;	/* inherited */
			
	u_int16_t	ReferenceID;
	u_int32_t	NSAttributes;
	/* datastream actual cannot be retrieved this way... */
	/* datastream logical ... dtto ... */
	int32_t		UpdateTime; /* seconds, relative to year 2000... */ /* NW4.11 */
	struct NSI_DOSName DOSName;	/* NW4.11 */
	u_int32_t	FlushTime;	/* NW4.11 */
	u_int32_t	ParentBaseID;	/* NW4.11 */
	u_int8_t	MacFinderInfo[32]; /* NW4.11 */
	u_int32_t	SiblingCount;	/* NW4.11 */
	u_int32_t	EffectiveRights; /* NW4.11 */
	struct NSI_MacTimes MacTimes;	/* NW4.11 */
	u_int16_t	Unknown25;
	u_int16_t	reserved3;
	u_int32_t	reserved1[7];	/* future expansion... */
	struct NSI_Name	Name;
	u_int8_t	reserved2[512];	/* future expansion... (unicode name?) */
};
#endif

struct nw_info_struct3 {
	size_t		len;
	void*		data;
};

struct ncp_dos_info_rights {
	u_int16_t	Grant;
	u_int16_t	Revoke;
};

struct ncp_dos_info {
	u_int32_t	Attributes;
	struct NSI_Change Creation;
	struct NSI_Change Modify;
	struct NSI_Change Archive;
	struct NSI_LastAccess LastAccess;	/* Time field ignored */
	struct ncp_dos_info_rights Rights;
	u_int32_t	MaximumSpace;	/* 64bit? It is in 4KB blocks... And NSS does not support this... */
};

struct ncp_namespace_format_BitMask {
	u_int32_t	fixed;
	u_int32_t	variable;
	u_int32_t	huge;
};

struct ncp_namespace_format_BitsDefined {
	size_t		fixed;
	size_t		variable;
	size_t		huge;
};

struct ncp_namespace_format {
	unsigned int	Version;	/* used only by library */
	struct ncp_namespace_format_BitMask BitMask;
	struct ncp_namespace_format_BitsDefined BitsDefined;
	size_t		FieldsLength[32];
};

NWCCODE
ncp_ns_open_create_entry(NWCONN_HANDLE conn,
				/* entry info */
				unsigned int ns,
				unsigned int search_attributes,
				int dirstyle,
				unsigned int vol,
				u_int32_t dirent,
				const unsigned char* encpath, size_t pathlen,
				/*  what to do with entry */
				int datastream,
				int open_create_mode,
				u_int32_t create_attributes,
				u_int16_t desired_access_rights,
				/* what to return */
				u_int32_t rim,
				/* returned */
				/* struct nw_info_struct2 */ void* target, size_t sizeoftarget,
				u_int8_t* oc_action,
				u_int8_t* oc_callback,
				char fileHandle[6]	/* ?? u_int32_t* or NW_FILE_HANDLE* ?? */
				);

NWCCODE
ncp_ns_obtain_entry_info(NWCONN_HANDLE conn, 
				/* entry info */
				unsigned int source_ns,
				unsigned int search_attributes, 
				int dirstyle, 
				unsigned int vol, 
				u_int32_t dirent, 
				const unsigned char* encpath, size_t pathlen,
				/* what to do with entry */
				unsigned int target_ns,
				/* what to return */
				u_int32_t rim,
				/* returned */
				/* struct nw_info_struct2 */ void* target, size_t sizeoftarget);

NWCCODE
ncp_ns_modify_entry_dos_info(NWCONN_HANDLE conn,
				    /* entry info */
				    unsigned int ns,
				    unsigned int search_attributes,
				    int dirstyle,
				    unsigned int vol,
				    u_int32_t dirent,
				    const unsigned char* encpath, size_t pathlen,
				    /* what to do with entry */
				    u_int32_t mim,
				    const struct ncp_dos_info* info
				    /* nothing to return */
				    /* nothing returned */
				    );

NWCCODE
ncp_ns_obtain_namespace_info_format(NWCONN_HANDLE conn,
				  /* entry info */
				  unsigned int vol,
				  /* what to do with entry */
				  unsigned int target_ns,
				  /* returned */
				  struct ncp_namespace_format* format, size_t sizeofformat);

NWCCODE
ncp_ns_obtain_entry_namespace_info(NWCONN_HANDLE conn,
					  /* entry info */
					  unsigned int source_ns,
					  unsigned int vol,
					  u_int32_t dirent,
					  /* what to do with entry */
					  unsigned int target_ns,
					  /* rim */
					  u_int32_t nsrim,
					  /* returned */
					  void* nsibuf, size_t* nsibuflen, size_t nsimaxbuflen);
					  
NWCCODE
ncp_ns_modify_entry_namespace_info(NWCONN_HANDLE conn,
					  /* entry info */
					  unsigned int source_ns,
					  unsigned int vol,
					  u_int32_t dirent,
					  /* what to do with entry */
					  unsigned int target_ns,
					  /* rim */
					  u_int32_t nsrim,
					  /* info */
					  const void* mnsbuf, size_t mnsbuflen);
					  
NWCCODE
ncp_ns_get_namespace_info_element(const struct ncp_namespace_format* nsformat,
			       u_int32_t nsrim, 
			       const void* nsibuf,
			       size_t nsibuflen,
			       unsigned int itemid,
			       void* itembuf, size_t* itembuflen, size_t itemmaxbuflen);

NWCCODE
ncp_ns_alloc_short_dir_handle(NWCONN_HANDLE conn,
			      /* input */
			      unsigned int ns,
			      int dirstyle,
			      unsigned int vol,
			      u_int32_t dirent,
			      const unsigned char* encpath, size_t pathlen,
			      /* alloc short dir handle specific */
			      unsigned int allocate_mode,
			      /* output */
			      NWDIR_HANDLE* dirhandle,
			      NWVOL_NUM* ovol);
			 
#define NSIF_NAME		0x0000	/* NSI_Name */
#define NSIF_SPACE_ALLOCATED	0x0001	/* ncp_off64_t */
#define NSIF_ATTRIBUTES		0x0002	/* NSI_Attributes */
#define NSIF_DATA_SIZE		0x0003	/* ncp_off64_t */
#define NSIF_TOTAL_SIZE		0x0004	/* NSI_TotalSize */
#define NSIF_EXT_ATTR_INFO	0x0005	/* NSI_ExtAttrInfo */
#define NSIF_ARCHIVE		0x0006	/* NSI_Change */
#define NSIF_MODIFY		0x0007	/* NSI_Modify */
#define NSIF_CREATION		0x0008	/* NSI_Change */
#define NSIF_OWNING_NAMESPACE	0x0009	/* u_int32_t */
#define NSIF_DIRECTORY		0x000A	/* NSI_Directory */
#define NSIF_RIGHTS		0x000B	/* u_int16_t */
#define NSIF_REFERENCE_ID	0x000C	/* u_int16_t */
#define NSIF_NS_ATTRIBUTES	0x000D	/* u_int32_t */
#define NSIF_DATASTREAM_SIZES	0x000E	/* NSI_DatastreamSizes */
#define NSIF_DATASTREAM_LOGICALS 0x000F	/* NSI_DatastreamLogicals */
#define NSIF_UPDATE_TIME	0x0010	/* int32_t */
#define NSIF_DOS_NAME		0x0011	/* NSI_Name */
#define NSIF_FLUSH_TIME		0x0012	/* u_int32_t */
#define NSIF_PARENT_BASE_ID	0x0013	/* u_int32_t */
#define NSIF_MAC_FINDER_INFO	0x0014	/* 32 bytes */
#define NSIF_SIBLING_COUNT	0x0015	/* u_int32_t */
#define NSIF_EFFECTIVE_RIGHTS	0x0016	/* u_int32_t */
#define NSIF_MAC_TIMES		0x0017	/* NSI_MacTimes */
#define NSIF_LAST_ACCESS_TIME	0x0018	/* NSI_Modify (alias to NSIF_MODIFY) */
#ifdef MAKE_NCPLIB
#define NSIF_UNKNOWN25		0x0019
#endif
#define NSIF_SIZE64		0x001A	/* ncp_off64_t */

NWCCODE
ncp_ns_extract_info_field_size(
		   const struct nw_info_struct3* rq,
		   u_int32_t field,
		   size_t* destlen);
NWCCODE
ncp_ns_extract_info_field(
		   const struct nw_info_struct3* rq,
		   u_int32_t field,
		   void* dest, size_t destlen);

typedef struct ncp_directory_list_handle * NWDIRLIST_HANDLE;

NWCCODE
ncp_ns_search_init(NWCONN_HANDLE conn,
		   /* input */
		   unsigned int ns,
		   unsigned int search_attributes,
		   unsigned int dirstyle,
		   unsigned int vol,
		   NWDIR_ENTRY dirent,
		   const unsigned char* encpath, size_t enclen,
		   /* search specific */
		   int datastream,
		   const unsigned char* pattern, size_t patlen,
		   u_int32_t rim,
		   /* handle */
		   NWDIRLIST_HANDLE* handle);
		   
NWCCODE
ncp_ns_search_next(NWDIRLIST_HANDLE handle,
		   /* struct nw_info_struct2 */ void* target, size_t sizeoftarget);

NWCCODE
ncp_ns_search_end(NWDIRLIST_HANDLE handle);

typedef struct {
	NWObjectID	objectID;
	u_int16_t	objectRights;
} TRUSTEE_INFO;

NWCCODE
ncp_ns_trustee_add(NWCONN_HANDLE conn,
		   /* input */
		   unsigned int ns,
		   unsigned int search_attributes,
		   int dirstyle,
		   unsigned int vol,
		   u_int32_t dirent,
		   const unsigned char* encpath, size_t pathlen,
		   /* trustee add specific */
		   const TRUSTEE_INFO* trustees,
		   unsigned int object_count,
		   u_int16_t rights_mask);

NWCCODE
ncp_ns_trustee_del(NWCONN_HANDLE conn,
		   /* input */
		   unsigned int ns,
		   int dirstyle,
		   unsigned int vol,
		   u_int32_t dirent,
		   const unsigned char* encpath, size_t pathlen,
		   /* trustee del specific */
		   const TRUSTEE_INFO* trustees,
		   unsigned int object_count);

NWCCODE
ncp_ns_trustee_scan(NWCONN_HANDLE conn,
		    /* input */
		    unsigned int ns,
		    unsigned int search_attributes,
		    int dirstyle,
		    unsigned int vol,
		    u_int32_t dirent,
		    const unsigned char* encpath, size_t pathlen,
		    /* trustee scan specific */
		    u_int32_t* iter,
		    TRUSTEE_INFO* trustees,
		    unsigned int* object_count);

NWCCODE
ncp_ns_delete_entry(NWCONN_HANDLE conn,
		    /* input */
		    unsigned int ns,
		    unsigned int search_attributes,
		    int dirstyle,
		    unsigned int vol,
		    u_int32_t dirent,
		    const unsigned char* encpath, size_t pathlen);

typedef struct ncp_volume_list_handle * NWVOL_HANDLE;

NWCCODE
ncp_volume_list_init(NWCONN_HANDLE conn,
		     /* input */
		     unsigned int ns,
		     unsigned int reqflags,
		     /* output */
		     NWVOL_HANDLE* handle);
		     
NWCCODE
ncp_volume_list_next(NWVOL_HANDLE handle,
		     unsigned int *volnum,
		     char* retname, size_t retname_maxlen);
		     
NWCCODE
ncp_volume_list_end(NWVOL_HANDLE handle);
		     
//NWCCODE ncp_extract_file_info2(u_int32_t rim, const u_int8_t*, size_t,
//		struct nw_info_struct2*);

NWCCODE
ncp_get_file_size(NWCONN_HANDLE conn,
		  /* input */
		  const char fileHandle[6],
		  /* output */
		  ncp_off64_t* fileSize);

int
ncp_get_mount_uid(int fid, uid_t* uid);

#ifndef SWIG
void
com_err(const char* program, int error, const char* msg, ...);
#endif

NWCCODE ncp_renegotiate_siglevel(NWCONN_HANDLE conn, size_t buffsize, int siglevel);

char* ncp_perms_to_str(char r[11], const u_int16_t rights);

int ncp_str_to_perms(const char *r, u_int16_t *rights);

const char* ncp_namespace_to_str(char r[5], unsigned int ns);

#define NCP_PHYSREC_LOG	0x00
#define NCP_PHYSREC_EX	0x01
#define NCP_PHYSREC_SH	0x03

NWCCODE ncp_log_physical_record(NWCONN_HANDLE conn, const char fileHandle[6],
		ncp_off64_t startOffset, u_int64_t length, unsigned int flags,
		unsigned int timeout);

NWCCODE ncp_clear_physical_record(NWCONN_HANDLE conn, const char fileHandle[6],
		ncp_off64_t startOffset, u_int64_t length);

NWCCODE ncp_release_physical_record(NWCONN_HANDLE conn, const char fileHandle[6],
		ncp_off64_t startOffset, u_int64_t length);

#ifdef __cplusplus
}
#endif

#ifdef NCP_OBSOLETE
#include <ncp/obsolete/o_ncplib.h>
#endif

#endif	/* _NCPLIB_H */
