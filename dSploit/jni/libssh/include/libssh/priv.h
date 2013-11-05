/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/*
 * priv.h file
 * This include file contains everything you shouldn't deal with in
 * user programs. Consider that anything in this file might change
 * without notice; libssh.h file will keep backward compatibility
 * on binary & source
 */

#ifndef _LIBSSH_PRIV_H
#define _LIBSSH_PRIV_H

#include "config.h"

#ifdef _WIN32

/* Imitate define of inttypes.h */
# ifndef PRIdS
#  define PRIdS "Id"
# endif

# ifdef _MSC_VER
#  include <stdio.h>

/* On Microsoft compilers define inline to __inline on all others use inline */
#  undef inline
#  define inline __inline

#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define strtoull _strtoui64
#  define isblank(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r')

#  define usleep(X) Sleep(((X)+1000)/1000)

#  undef strtok_r
#  define strtok_r strtok_s

#  if defined(HAVE__SNPRINTF_S)
#   undef snprintf
#   define snprintf(d, n, ...) _snprintf_s((d), (n), _TRUNCATE, __VA_ARGS__)
#  else /* HAVE__SNPRINTF_S */
#   if defined(HAVE__SNPRINTF)
#     undef snprintf
#     define snprintf _snprintf
#   else /* HAVE__SNPRINTF */
#    if !defined(HAVE_SNPRINTF)
#     error "no snprintf compatible function found"
#    endif /* HAVE_SNPRINTF */
#   endif /* HAVE__SNPRINTF */
#  endif /* HAVE__SNPRINTF_S */

#  if defined(HAVE__VSNPRINTF_S)
#   undef vsnprintf
#   define vsnprintf(s, n, f, v) _vsnprintf_s((s), (n), _TRUNCATE, (f), (v))
#  else /* HAVE__VSNPRINTF_S */
#   if defined(HAVE__VSNPRINTF)
#    undef vsnprintf
#    define vsnprintf _vsnprintf
#   else
#    if !defined(HAVE_VSNPRINTF)
#     error "No vsnprintf compatible function found"
#    endif /* HAVE_VSNPRINTF */
#   endif /* HAVE__VSNPRINTF */
#  endif /* HAVE__VSNPRINTF_S */

# endif /* _MSC_VER */

#else /* _WIN32 */

#include <unistd.h>
#define PRIdS "zd"

#endif /* _WIN32 */

#include "libssh/libssh.h"
#include "libssh/callbacks.h"
#include "libssh/crypto.h"

/* some constants */
#define MAX_PACKET_LEN 262144
#define ERROR_BUFFERLEN 1024
#define CLIENTBANNER1 "SSH-1.5-libssh-" SSH_STRINGIFY(LIBSSH_VERSION)
#define CLIENTBANNER2 "SSH-2.0-libssh-" SSH_STRINGIFY(LIBSSH_VERSION)
#define KBDINT_MAX_PROMPT 256 /* more than openssh's :) */

#ifdef __cplusplus
extern "C" {
#endif


#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

typedef struct kex_struct {
	unsigned char cookie[16];
	char **methods;
} KEX;

struct error_struct {
/* error handling */
    int error_code;
    char error_buffer[ERROR_BUFFERLEN];
};

/* TODO: remove that include */
#include "libssh/wrapper.h"

struct ssh_keys_struct {
  const char *privatekey;
  const char *publickey;
};

struct ssh_message_struct;
struct ssh_common_struct;

/* server data */


SSH_PACKET_CALLBACK(ssh_packet_disconnect_callback);
SSH_PACKET_CALLBACK(ssh_packet_ignore_callback);

/* client.c */

int ssh_send_banner(ssh_session session, int is_server);
SSH_PACKET_CALLBACK(ssh_packet_dh_reply);
SSH_PACKET_CALLBACK(ssh_packet_newkeys);
SSH_PACKET_CALLBACK(ssh_packet_service_accept);

/* config.c */
int ssh_config_parse_file(ssh_session session, const char *filename);

/* errors.c */
void ssh_set_error(void *error, int code, const char *descr, ...) PRINTF_ATTRIBUTE(3, 4);
void ssh_set_error_oom(void *);
void ssh_set_error_invalid(void *, const char *);

/* in crypt.c */
uint32_t packet_decrypt_len(ssh_session session,char *crypted);
int packet_decrypt(ssh_session session, void *packet,unsigned int len);
unsigned char *packet_encrypt(ssh_session session,void *packet,unsigned int len);
 /* it returns the hmac buffer if exists*/
struct ssh_poll_handle_struct;

int packet_hmac_verify(ssh_session session,ssh_buffer buffer,unsigned char *mac);

struct ssh_socket_struct;

int ssh_packet_socket_callback(const void *data, size_t len, void *user);
void ssh_packet_register_socket_callback(ssh_session session, struct ssh_socket_struct *s);
void ssh_packet_set_callbacks(ssh_session session, ssh_packet_callbacks callbacks);
void ssh_packet_set_default_callbacks(ssh_session session);
void ssh_packet_process(ssh_session session, uint8_t type);
/* connect.c */
socket_t ssh_connect_host(ssh_session session, const char *host,const char
        *bind_addr, int port, long timeout, long usec);
socket_t ssh_connect_host_nonblocking(ssh_session session, const char *host,
		const char *bind_addr, int port);
void ssh_sock_set_nonblocking(socket_t sock);
void ssh_sock_set_blocking(socket_t sock);

/* in kex.c */
extern const char *ssh_kex_nums[];
int ssh_send_kex(ssh_session session, int server_kex);
void ssh_list_kex(ssh_session session, KEX *kex);
int set_kex(ssh_session session);
int verify_existing_algo(int algo, const char *name);
char **space_tokenize(const char *chain);
int ssh_get_kex1(ssh_session session);
char *ssh_find_matching(const char *in_d, const char *what_d);


/* in base64.c */
ssh_buffer base64_to_bin(const char *source);
unsigned char *bin_to_base64(const unsigned char *source, int len);

/* gzip.c */
int compress_buffer(ssh_session session,ssh_buffer buf);
int decompress_buffer(ssh_session session,ssh_buffer buf, size_t maxlen);

/* crc32.c */
uint32_t ssh_crc32(const char *buf, uint32_t len);


/* match.c */
int match_hostname(const char *host, const char *pattern, unsigned int len);

int message_handle(ssh_session session, void *user, uint8_t type, ssh_buffer packet);
/* log.c */

void ssh_log_common(struct ssh_common_struct *common, int verbosity,
    const char *format, ...) PRINTF_ATTRIBUTE(3, 4);

/* misc.c */
#ifdef _WIN32
int gettimeofday(struct timeval *__p, void *__t);
#endif /* _WIN32 */

#ifndef __FUNCTION__
#if defined(__SUNPRO_C)
#define __FUNCTION__ __func__
#endif
#endif

#define _enter_function(sess) \
	do {\
		if((sess)->common.log_verbosity >= SSH_LOG_FUNCTIONS){ \
			ssh_log((sess),SSH_LOG_FUNCTIONS,"entering function %s line %d in " __FILE__ , __FUNCTION__,__LINE__);\
			(sess)->common.log_indent++; \
		} \
	} while(0)

#define _leave_function(sess) \
	do { \
		if((sess)->common.log_verbosity >= SSH_LOG_FUNCTIONS){ \
			(sess)->common.log_indent--; \
			ssh_log((sess),SSH_LOG_FUNCTIONS,"leaving function %s line %d in " __FILE__ , __FUNCTION__,__LINE__);\
		}\
	} while(0)

#ifdef DEBUG_CALLTRACE
#define enter_function() _enter_function(session)
#define leave_function() _leave_function(session)
#else
#define enter_function() (void)session
#define leave_function() (void)session
#endif

/* options.c  */

int ssh_options_set_algo(ssh_session session, int algo, const char *list);
int ssh_options_apply(ssh_session session);

/* server.c */
SSH_PACKET_CALLBACK(ssh_packet_kexdh_init);

/** Free memory space */
#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)

/** Zero a structure */
#define ZERO_STRUCT(x) memset((char *)&(x), 0, sizeof(x))

/** Zero a structure given a pointer to the structure */
#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

/** Get the size of an array */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/** Overwrite the complete string with 'X' */
#define BURN_STRING(x) do { if ((x) != NULL) memset((x), 'X', strlen((x))); } while(0)

#ifdef HAVE_LIBGCRYPT
/* gcrypt_missing.c */
int my_gcry_dec2bn(bignum *bn, const char *data);
char *my_gcry_bn2dec(bignum bn);
#endif /* !HAVE_LIBGCRYPT */

#ifdef __cplusplus
}
#endif

#endif /* _LIBSSH_PRIV_H */
/* vim: set ts=2 sw=2 et cindent: */
