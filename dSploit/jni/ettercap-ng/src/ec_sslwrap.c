/*
    ettercap -- SSL support

    Copyright (C) ALoR & NaGA
    
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

    $Id: ec_sslwrap.c,v 1.55 2004/09/14 07:58:17 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_session.h>
#include <ec_hook.h>
#include <ec_dissect.h>
#include <ec_threads.h>
#include <ec_sslwrap.h>
#include <ec_file.h>
#include <ec_version.h>
#include <ec_socket.h>

#include <sys/types.h>
#ifndef OS_WINDOWS
   #include <sys/wait.h>
#endif
#include <fcntl.h>

#ifdef HAVE_OPENSSL

// XXX - check if we have poll.h
#ifdef HAVE_SYS_POLL_H
   #include <sys/poll.h>
#endif

/* don't include kerberos. RH sux !! */
#define OPENSSL_NO_KRB5 1
#include <openssl/ssl.h>

#define BREAK_ON_ERROR(x,y,z) do {  \
   if (x == -EINVALID) {            \
      SAFE_FREE(z.DATA.disp_data);  \
      sslw_initialize_po(&z, z.DATA.data); \
      z.len = 64;                   \
      z.L4.flags = TH_RST;          \
      packet_disp_data(&z, z.DATA.data, z.DATA.len); \
      sslw_parse_packet(y, SSL_SERVER, &z); \
      sslw_wipe_connection(y);      \
      SAFE_FREE(z.DATA.data);       \
      SAFE_FREE(z.DATA.disp_data);  \
      ec_thread_exit();             \
   }                                \
} while(0)

#endif /* HAVE_OPENSSL */

/* globals */

static LIST_HEAD (, listen_entry) listen_ports;

struct listen_entry {
   int fd;
   u_int16 sslw_port;   /* Port where we want to wrap SSL */
   u_int16 redir_port;  /* Port where accepts connections */
   u_char status;       /* Use directly SSL or not */
   char *name;
   LIST_ENTRY (listen_entry) next;
};


#ifdef HAVE_OPENSSL

struct accepted_entry {
   int32 fd[2];   /* 0->Client, 1->Server */
   u_int16 port[2];
   struct ip_addr ip[2];
   SSL *ssl[2];
   u_char status;
   X509 *cert;
   #define SSL_CLIENT 0
   #define SSL_SERVER 1
};

/* Session identifier 
 * It has to be even-lenghted for session hash matching */
struct sslw_ident {
   u_int32 magic;
      #define SSLW_MAGIC  0x0501e77e
   struct ip_addr L3_src;
   u_int16 L4_src;
   u_int16 L4_dst;
};
#define SSLW_IDENT_LEN sizeof(struct sslw_ident)

#define SSLW_RETRY 5
#define SSLW_WAIT 10000

#define TSLEEP (50*1000) /* 50 milliseconds */

static SSL_CTX *ssl_ctx_client, *ssl_ctx_server;
static EVP_PKEY *global_pk;
static u_int16 number_of_services;
static struct pollfd *poll_fd = NULL;

#endif /* HAVE_OPENSSL */

/* protos */

void sslw_dissect_add(char *name, u_int32 port, FUNC_DECODER_PTR(decoder), u_char status);
void sslw_dissect_move(char *name, u_int16 port);
EC_THREAD_FUNC(sslw_start);
void ssl_wrap_init(void);

#ifdef HAVE_OPENSSL

static EC_THREAD_FUNC(sslw_child);
static int sslw_is_ssl(struct packet_object *po);
static int sslw_connect_server(struct accepted_entry *ae);
static int sslw_sync_conn(struct accepted_entry *ae);
static int sslw_get_peer(struct accepted_entry *ae);
static void sslw_bind_wrapper(void);
static int sslw_read_data(struct accepted_entry *ae, u_int32 direction, struct packet_object *po);
static int sslw_write_data(struct accepted_entry *ae, u_int32 direction, struct packet_object *po);
static void sslw_wipe_connection(struct accepted_entry *ae);
static void sslw_init(void);
static void sslw_initialize_po(struct packet_object *po, u_char *p_data);
static int sslw_match(void *id_sess, void *id_curr);
static void sslw_create_session(struct ec_session **s, struct packet_object *po);
static size_t sslw_create_ident(void **i, struct packet_object *po);            
static void sslw_hook_handled(struct packet_object *po);
static X509 *sslw_create_selfsigned(X509 *serv_cert);
static int sslw_insert_redirect(u_int16 sport, u_int16 dport);
static int sslw_remove_redirect(u_int16 sport, u_int16 dport);
static void ssl_wrap_fini(void);
static int sslw_ssl_connect(SSL *ssl_sk);
static int sslw_ssl_accept(SSL *ssl_sk);

#endif /* HAVE_OPENSSL */

/*******************************************/

/* 
 * Register a new ssl wrapper 
 */
void sslw_dissect_add(char *name, u_int32 port, FUNC_DECODER_PTR(decoder), u_char status)
{
   struct listen_entry *le;
   
   SAFE_CALLOC(le, 1, sizeof(struct listen_entry));
 
   le->sslw_port = port;
   le->status = status;
   le->name = name;

   /* Insert it in the port list where listen for connections */ 
   LIST_INSERT_HEAD(&listen_ports, le, next);    

   dissect_add(name, APP_LAYER_TCP, port, decoder); 
}

/* 
 * Move a ssl_wrapper on another port
 */
void sslw_dissect_move(char *name, u_int16 port)
{
   struct listen_entry *le, *tmp;

   LIST_FOREACH_SAFE(le, &listen_ports, next, tmp) 
      if(!strcmp(name, le->name)) {
         DEBUG_MSG("sslw_dissect_move: %s [%u]", name, port);
         le->sslw_port = port;
	 
      /* Move to zero means disable */
      if (port == 0) {
         LIST_REMOVE(le, next);
         SAFE_FREE(le);
      }
   }
}

/* 
 * Initialize the ssl wrappers
 */
void ssl_wrap_init(void)
{
   struct listen_entry *le;

#ifndef HAVE_OPENSSL
   /* avoid gcc warning about unused variable */
   (void)le;
   
   DEBUG_MSG("ssl_wrap_init: not supported");
   return;
#else
   /* disable if the aggressive flag is not set */
   if (!GBL_CONF->aggressive_dissectors) {
      DEBUG_MSG("ssl_wrap_init: not aggressive");
      return;
   }
   
   /* a valid script for the redirection must be set */
   if (!GBL_CONF->redir_command_on) {
      DEBUG_MSG("ssl_wrap_init: no redirect script");
      USER_MSG("SSL dissection needs a valid 'redir_command_on' script in the etter.conf file\n");
      return;
   }

   DEBUG_MSG("ssl_wrap_init");
   sslw_init();
   sslw_bind_wrapper();
   
   /* Add the hook to block real ssl packet going to top half */
   hook_add(HOOK_HANDLED, &sslw_hook_handled);

   number_of_services = 0;
   LIST_FOREACH(le, &listen_ports, next) 
      number_of_services++;
   
   SAFE_CALLOC(poll_fd, 1, sizeof(struct pollfd) * number_of_services);

   atexit(ssl_wrap_fini);
#endif
}


#ifdef HAVE_OPENSSL
static void ssl_wrap_fini(void)
{
   struct listen_entry *le;

   /* remove every redirect rule */   
   LIST_FOREACH(le, &listen_ports, next)
      sslw_remove_redirect(le->sslw_port, le->redir_port);
}
#endif

/* 
 * SSL thread main function.
 */
EC_THREAD_FUNC(sslw_start)
{
#ifdef HAVE_OPENSSL
   struct listen_entry *le;
   struct accepted_entry *ae;
   u_int len = sizeof(struct sockaddr_in), i;
   struct sockaddr_in client_sin;
#endif

   ec_thread_init();

#ifndef HAVE_OPENSSL
   DEBUG_MSG("sslw_start: openssl support not compiled in");
   return NULL;
#else
   
   /* disabled if not accressive */
   if (!GBL_CONF->aggressive_dissectors)
      return NULL;
   
   /* a valid script for the redirection must be set */
   if (!GBL_CONF->redir_command_on)
      return NULL;
   
   DEBUG_MSG("sslw_start: initialized and ready");
   
   /* Set the polling on all registered ssl services */
   i=0;
   LIST_FOREACH(le, &listen_ports, next) {
      poll_fd[i].fd = le->fd;
      poll_fd[i++].events = POLLIN;
   }

   LOOP {
      poll(poll_fd, number_of_services, -1);
      
      /* Check which port received connection */
      for(i=0; i<number_of_services; i++) 
         if (poll_fd[i].revents & POLLIN) {
	 
            LIST_FOREACH(le, &listen_ports, next) 
               if (poll_fd[i].fd == le->fd)
                  break;
	    
            DEBUG_MSG("ssl_wrapper -- got a connection on port %d [%d]", le->redir_port, le->sslw_port);
            SAFE_CALLOC(ae, 1, sizeof(struct accepted_entry));
	    
            ae->fd[SSL_CLIENT] = accept(poll_fd[i].fd, (struct sockaddr *)&client_sin, &len);
            
            /* Error checking */
            if (ae->fd[SSL_CLIENT] == -1) {
               SAFE_FREE(ae);
               continue;
            }
	    
            /* Set the server original port for protocol dissection */
            ae->port[SSL_SERVER] = htons(le->sslw_port);
            
            /* Check if we have to enter SSL status */
            ae->status = le->status;
	       
            /* Set the peer (client) in the connection list entry */
            ae->port[SSL_CLIENT] = client_sin.sin_port;
            ip_addr_init(&(ae->ip[SSL_CLIENT]), AF_INET, (char *)&(client_sin.sin_addr.s_addr));
	    
            ec_thread_new("sslw_child", "ssl child", &sslw_child, ae);
         }
   }

   return NULL;
#endif /* HAVE_OPENSSL */
   
}	 

#ifdef HAVE_OPENSSL

/* 
 * Filter SSL related packets and create NAT sessions.
 * It hooks HOOK_HANDLED.
 */
static void sslw_hook_handled(struct packet_object *po)
{
   struct ec_session *s = NULL;

   /* We have nothing to do with this packet */
   if (!sslw_is_ssl(po))
      return;
      
   /* If it's an ssl packet don't forward */
   po->flags |= PO_DROPPED;
   
   /* If it's a new connection */
   if ( (po->flags & PO_FORWARDABLE) && 
        (po->L4.flags & TH_SYN) &&
        !(po->L4.flags & TH_ACK) ) {
	
      sslw_create_session(&s, PACKET);

      /* Remember the real destination IP */
      memcpy(s->data, &po->L3.dst, sizeof(struct ip_addr));
      session_put(s);
   } else /* Pass only the SYN for conntrack */
      po->flags |= PO_IGNORE;
}

/*
 * execute the script to add the redirection
 */
static int sslw_insert_redirect(u_int16 sport, u_int16 dport)
{
   char asc_sport[16];
   char asc_dport[16];
   int ret_val, i = 0;
   char *command, *p;
   char **param = NULL;
 
   /* the script is not defined */
   if (GBL_CONF->redir_command_on == NULL)
      return -EFATAL;
   
   sprintf(asc_sport, "%u", sport);
   sprintf(asc_dport, "%u", dport);

   /* make the substitutions in the script */
   command = strdup(GBL_CONF->redir_command_on);
   str_replace(&command, "%iface", GBL_OPTIONS->iface);
   str_replace(&command, "%port", asc_sport);
   str_replace(&command, "%rport", asc_dport);
   
   DEBUG_MSG("sslw_insert_redirect: [%s]", command);
   
   /* split the string in the parameter array */
   for (p = strsep(&command, " "); p != NULL; p = strsep(&command, " ")) {
      /* allocate the array */
      SAFE_REALLOC(param, (i + 1) * sizeof(char *));
                        
      /* copy the tokens in the array */
      param[i++] = strdup(p);
   }
   /* NULL terminate the array */
   SAFE_REALLOC(param, (i + 1) * sizeof(char *));
               
   param[i] = NULL;
               
   /* execute the script */ 
   switch (fork()) {
      case 0:
         execvp(param[0], param);
         exit(EINVALID);
      case -1:
         SAFE_FREE(param);
         return -EINVALID;
      default:
         SAFE_FREE(param);
         wait(&ret_val);
         if (ret_val == EINVALID)
            return -EINVALID;
   }    
   
   return ESUCCESS;
}

/*
 * execute the script to remove the redirection
 */
static int sslw_remove_redirect(u_int16 sport, u_int16 dport)
{
   char asc_sport[16];
   char asc_dport[16];
   int ret_val, i = 0;
   char *command, *p;
   char **param = NULL;
 
   /* the script is not defined */
   if (GBL_CONF->redir_command_off == NULL)
      return -EFATAL;
   
   sprintf(asc_sport, "%u", sport);
   sprintf(asc_dport, "%u", dport);

   /* make the substitutions in the script */
   command = strdup(GBL_CONF->redir_command_off);
   str_replace(&command, "%iface", GBL_OPTIONS->iface);
   str_replace(&command, "%port", asc_sport);
   str_replace(&command, "%rport", asc_dport);
   
   DEBUG_MSG("sslw_remove_redirect: [%s]", command);
   
   /* split the string in the parameter array */
   for (p = strsep(&command, " "); p != NULL; p = strsep(&command, " ")) {
      /* allocate the array */
      SAFE_REALLOC(param, (i + 1) * sizeof(char *));
                        
      /* copy the tokens in the array */
      param[i++] = strdup(p);
   }
   /* NULL terminate the array */
   SAFE_REALLOC(param, (i + 1) * sizeof(char *));
               
   param[i] = NULL;
               
   /* execute the script */ 
   switch (fork()) {
      case 0:
         execvp(param[0], param);
         exit(EINVALID);
      case -1:
         return -EINVALID;
      default:
         wait(&ret_val);
         if (ret_val == EINVALID)
            return -EINVALID;
   }    
   
   return ESUCCESS;
}


/* 
 * Check if this packet is for ssl wrappers 
 */
static int sslw_is_ssl(struct packet_object *po)
{
   struct listen_entry *le;
   
   /* If it's already coming from ssl wrapper 
    * or the connection is not TCP */ 
   if (po->flags & PO_FROMSSL || po->L4.proto != NL_TYPE_TCP) 
      return 0;

   LIST_FOREACH(le, &listen_ports, next) {
      if (ntohs(po->L4.dst) == le->sslw_port ||
          ntohs(po->L4.src) == le->sslw_port)
         return 1;
   }
   return 0;
}


/*
 * Bind all registered wrappers to free ports 
 * and isnert redirects.
 */ 
static void sslw_bind_wrapper(void)
{
   u_int16 bind_port = EC_MAGIC_16; 
   struct listen_entry *le;
   struct sockaddr_in sa_in;

   LIST_FOREACH(le, &listen_ports, next) { 
   
      le->fd = socket(AF_INET, SOCK_STREAM, 0);

      memset(&sa_in, 0, sizeof(sa_in));
      sa_in.sin_family = AF_INET;
      sa_in.sin_addr.s_addr = INADDR_ANY;
   
      do {
         bind_port++;
         sa_in.sin_port = htons(bind_port);
         le->redir_port = bind_port;
      } while ( bind(le->fd, (struct sockaddr *)&sa_in, sizeof(sa_in)) != 0);

      DEBUG_MSG("sslw - bind %d on %d", le->sslw_port, le->redir_port);
      listen(le->fd, 100);      
      if (sslw_insert_redirect(le->sslw_port, le->redir_port) != ESUCCESS)
        FATAL_ERROR("Can't insert firewall redirects");
   }
}

/* 
 * Create TCP a connection to the real SSL server 
 */
static int sslw_sync_conn(struct accepted_entry *ae)
{      
   if(sslw_get_peer(ae) != ESUCCESS)
         return -EINVALID;
	 
   if(sslw_connect_server(ae) != ESUCCESS)
         return -EINVALID;

   /* set nonbloking socket */
   set_blocking(ae->fd[SSL_CLIENT], 0);
   set_blocking(ae->fd[SSL_SERVER], 0);

   return ESUCCESS;
}


/* 
 * Perform a blocking SSL_connect with a
 * configurable timeout on a non-blocing socket 
 */
static int sslw_ssl_connect(SSL *ssl_sk)
{ 
   int loops = (GBL_CONF->connect_timeout * 10e5) / TSLEEP;
   int ret, ssl_err;
   
   do {
      /* connect to the server */
      if ( (ret = SSL_connect(ssl_sk)) == 1)
         return ESUCCESS;

      ssl_err = SSL_get_error(ssl_sk, ret);
      
      /* there was an error... */
      if (ssl_err != SSL_ERROR_WANT_READ && ssl_err != SSL_ERROR_WANT_WRITE) 
         return -EINVALID;
      
      /* sleep a quirk of time... */
      usleep(TSLEEP);
   } while(loops--);

   return -EINVALID;
}


/* 
 * Perform a blocking SSL_accept with a
 * configurable timeout on a non-blocing socket 
 */
static int sslw_ssl_accept(SSL *ssl_sk)
{ 
   int loops = (GBL_CONF->connect_timeout * 10e5) / TSLEEP;
   int ret, ssl_err;
   
   do {
      /* accept the ssl connection */
      if ( (ret = SSL_accept(ssl_sk)) == 1)
         return ESUCCESS;

      ssl_err = SSL_get_error(ssl_sk, ret);
      
      /* there was an error... */
      if (ssl_err != SSL_ERROR_WANT_READ && ssl_err != SSL_ERROR_WANT_WRITE) 
         return -EINVALID;
      
      /* sleep a quirk of time... */
      usleep(TSLEEP);
   } while(loops--);

   return -EINVALID;
}


/* 
 * Create an SSL connection to the real server.
 * Grab server certificate and create a fake one
 * for the poor client.
 * Then accept the SSL connection from the client.
 */   
static int sslw_sync_ssl(struct accepted_entry *ae) 
{   
   X509 *server_cert;
   
   ae->ssl[SSL_SERVER] = SSL_new(ssl_ctx_server);
   SSL_set_connect_state(ae->ssl[SSL_SERVER]);
   SSL_set_fd(ae->ssl[SSL_SERVER], ae->fd[SSL_SERVER]);
   ae->ssl[SSL_CLIENT] = SSL_new(ssl_ctx_client);
   SSL_set_fd(ae->ssl[SSL_CLIENT], ae->fd[SSL_CLIENT]);
    
   if (sslw_ssl_connect(ae->ssl[SSL_SERVER]) != ESUCCESS) 
      return -EINVALID;

   /* XXX - NULL cypher can give no certificate */
   if ( (server_cert = SSL_get_peer_certificate(ae->ssl[SSL_SERVER])) == NULL) {
      DEBUG_MSG("Can't get peer certificate");
      return -EINVALID;
   }

   /* Create the fake certificate */
   ae->cert = sslw_create_selfsigned(server_cert);  
   X509_free(server_cert);

   if (ae->cert == NULL)
      return -EINVALID;
   
   SSL_use_certificate(ae->ssl[SSL_CLIENT], ae->cert);
   
   if (sslw_ssl_accept(ae->ssl[SSL_CLIENT]) != ESUCCESS) 
      return -EINVALID;

   return ESUCCESS;   
}


/* 
 * Take the IP address of the server 
 * that the client wants to talk to.
 */
static int sslw_get_peer(struct accepted_entry *ae)
{
   struct ec_session *s = NULL;
   struct packet_object po;
   void *ident = NULL;
   int i;
 
   /* Take the server IP address from the NAT sessions */
   memcpy(&po.L3.src, &ae->ip[SSL_CLIENT], sizeof(struct ip_addr));
   po.L4.src = ae->port[SSL_CLIENT];
   po.L4.dst = ae->port[SSL_SERVER];
   
   sslw_create_ident(&ident, &po);
   
   /* 
    * A little waiting loop because the sniffing thread , 
    * which creates the session, may be slower than this
    */
   for (i=0; i<SSLW_RETRY && session_get_and_del(&s, ident, SSLW_IDENT_LEN)!=ESUCCESS; i++)
      usleep(SSLW_WAIT);

   if (i==SSLW_RETRY) {
      SAFE_FREE(ident);
      return -EINVALID;
   }
   
   /* Remember the server IP address in the sessions */
   memcpy(&ae->ip[SSL_SERVER], s->data, sizeof(struct ip_addr));
   
   SAFE_FREE(s->data);
   SAFE_FREE(s);
   SAFE_FREE(ident);

   return ESUCCESS;
}


/* 
 * Take the other peer (server) from ssl-decoders' sessions
 * and contact it. 
 * Check if we have to enter SSL state.
 */
static int sslw_connect_server(struct accepted_entry *ae)
{
   char *dest_ip;
   
   /* 
    * XXX - int_ntoa is not thread-safe. 
    * strdup it to avoid race conditions.
    * Btw int_ntoa is not so used in the code.
    */
   dest_ip = strdup(int_ntoa(ip_addr_to_int32(ae->ip[SSL_SERVER].addr)));
 
   /* Standard connection to the server */
   if (!dest_ip || (ae->fd[SSL_SERVER] = open_socket(dest_ip, ntohs(ae->port[SSL_SERVER]))) < 0) {
      SAFE_FREE(dest_ip);   
      return -EINVALID;
   }
   
   SAFE_FREE(dest_ip);	       
   return ESUCCESS;   
}


/* 
 * Read the data from an accepted connection. 
 * Check if it already entered SSL state.
 */ 
static int sslw_read_data(struct accepted_entry *ae, u_int32 direction, struct packet_object *po)
{
   int len, ret_err;
   
   if (ae->status & SSL_ENABLED)
      len = SSL_read(ae->ssl[direction], po->DATA.data, 1024);
   else       
      //len = socket_recv(ae->fd[direction], po->DATA.data, 1024);
      len = read(ae->fd[direction], po->DATA.data, 1024);

   /* XXX - Check when it returns 0 (it was a <)*/
   if (len <= 0 && (ae->status & SSL_ENABLED)) {
      ret_err = SSL_get_error(ae->ssl[direction], len);
      
      /* XXX - Is it necessary? */
      if (len == 0)
         return -EINVALID;
	       
      if (ret_err == SSL_ERROR_WANT_READ || ret_err == SSL_ERROR_WANT_WRITE)
         return -ENOTHANDLED;
      else
         return -EINVALID;
   }

   /* Only if no ssl */
   if (len < 0) {
      int err = GET_SOCK_ERRNO();

      if (err == EINTR || err == EAGAIN)
         return -ENOTHANDLED;
      else
         return -EINVALID;
   }      

   /* XXX - On standard reads, close is 0? (EOF)*/
   if (len == 0) 
      return -EINVALID;

   po->len = len;
   po->DATA.len = len;
   po->L4.flags |= TH_PSH;

   /* NULL terminate the data buffer */
   po->DATA.data[po->DATA.len] = 0;
 
   /* create the buffer to be displayed */
   packet_destroy_object(po);
   packet_disp_data(po, po->DATA.data, po->DATA.len);
  
   return ESUCCESS;
}


/* 
 * Write the data into an accepted connection. 
 * Check if it already entered SSL state.
 */ 
static int sslw_write_data(struct accepted_entry *ae, u_int32 direction, struct packet_object *po)
{
   int32 len, packet_len, not_written, ret_err;
   u_char *p_data;

   packet_len = (int32)(po->DATA.len + po->DATA.inject_len);
   p_data = po->DATA.data;
   
   if (packet_len == 0)
      return ESUCCESS;

   do {
      not_written = 0;
      /* Write packet data */
      if (ae->status & SSL_ENABLED)
         len = SSL_write(ae->ssl[direction], p_data, packet_len);
      else       
         //len = socket_send(ae->fd[direction], p_data, packet_len);
         len = write(ae->fd[direction], p_data, packet_len);

      if (len <= 0 && (ae->status & SSL_ENABLED)) {
         ret_err = SSL_get_error(ae->ssl[direction], len);
         if (ret_err == SSL_ERROR_WANT_READ || ret_err == SSL_ERROR_WANT_WRITE)
            not_written = 1;
         else
            return -EINVALID;
      }

      if (len < 0 && !(ae->status & SSL_ENABLED)) {
         int err = GET_SOCK_ERRNO();

         if (err == EINTR || err == EAGAIN)
            not_written = 1;
         else
            return -EINVALID;
      }      

      /* XXX - does some OS use partial writes for SSL? */
      if (len < packet_len && !not_written ) {
         DEBUG_MSG("SSL-Wrapper partial writes: to be implemented...");
         packet_len -= len;
         p_data += len;
         not_written = 1;
      }
      
      /* XXX - Set a proper sleep time */
      if (not_written)
         usleep(1000);
	 	 
   } while (not_written);
         
   return ESUCCESS;
}


/* 
 * Fill the packet object and put it in 
 * the dissector stack (above protocols decoders)
 */
static void sslw_parse_packet(struct accepted_entry *ae, u_int32 direction, struct packet_object *po)
{
   FUNC_DECODER_PTR(start_decoder);
   int len;

   /* 
    * ssl childs keep the connection alive even if the sniffing thread
    * was stopped. But don't add packets to top-half queue.
    */
   if (!GBL_SNIFF->active)
      return;

   memcpy(&po->L3.src, &ae->ip[direction], sizeof(struct ip_addr));
   memcpy(&po->L3.dst, &ae->ip[!direction], sizeof(struct ip_addr));
   
   po->L4.src = ae->port[direction];
   po->L4.dst = ae->port[!direction];
   
   po->flags |= PO_FROMSSL;
      
   /* get current time */
   gettimeofday(&po->ts, NULL);

   /* calculate if the dest is local or not */
   switch (ip_addr_is_local(&PACKET->L3.src)) {
      case ESUCCESS:
         PACKET->PASSIVE.flags &= ~FP_HOST_NONLOCAL;
         PACKET->PASSIVE.flags |= FP_HOST_LOCAL;
         break;
      case -ENOTFOUND:
         PACKET->PASSIVE.flags &= ~FP_HOST_LOCAL;
         PACKET->PASSIVE.flags |= FP_HOST_NONLOCAL;
         break;
      case -EINVALID:
         PACKET->PASSIVE.flags = FP_UNKNOWN;
         break;
   }

   /* Let's start from the last stage of decoder chain */
   start_decoder = get_decoder(APP_LAYER, PL_DEFAULT);
   start_decoder(po->DATA.data, po->DATA.len, &len, po);
}


/* 
 * Remove the connection from the accepted 
 * list and close both sockets.
 */
static void sslw_wipe_connection(struct accepted_entry *ae)
{
   if (ae->ssl[SSL_CLIENT]) 
      SSL_free(ae->ssl[SSL_CLIENT]);

   if (ae->ssl[SSL_SERVER]) 
      SSL_free(ae->ssl[SSL_SERVER]);
 
   close_socket(ae->fd[SSL_CLIENT]);
   close_socket(ae->fd[SSL_SERVER]);

   if (ae->cert)
      X509_free(ae->cert);

   SAFE_FREE(ae);
}

/* 
 * Initialize a fake PO to be passed to top half
 */
static void sslw_initialize_po(struct packet_object *po, u_char *p_data)
{
   /* 
    * Allocate the data buffer and initialize 
    * fake headers. Headers len is set to 0.
    * XXX - Be sure to not modify these len.
    */
   memset(po, 0, sizeof(struct packet_object));
   if (p_data == NULL)
      SAFE_CALLOC(po->DATA.data, 1, UINT16_MAX);
   else
      po->DATA.data = p_data;
      
   po->L2.header  = po->DATA.data; 
   po->L3.header  = po->DATA.data;
   po->L3.options = po->DATA.data;
   po->L4.header  = po->DATA.data;
   po->L4.options = po->DATA.data;
   po->fwd_packet = po->DATA.data;
   po->packet     = po->DATA.data;
   
   po->L3.proto = htons(LL_TYPE_IP);
   po->L3.ttl = 64;
   po->L4.proto = NL_TYPE_TCP;
}


/* 
 * Create a self-signed certificate
 */
static X509 *sslw_create_selfsigned(X509 *server_cert)
{   
   X509 *out_cert;
//   X509_EXTENSION *ext;
//   int index = 0;
   
   if ((out_cert = X509_new()) == NULL)
      return NULL;
      
   /* Set out public key, real server name... */
   X509_set_version(out_cert, 0x2);
   X509_set_serialNumber(out_cert, X509_get_serialNumber(server_cert));   
   X509_set_notBefore(out_cert, X509_get_notBefore(server_cert));
   X509_set_notAfter(out_cert, X509_get_notAfter(server_cert));
   X509_set_pubkey(out_cert, global_pk);
   X509_set_subject_name(out_cert, X509_get_subject_name(server_cert));
   X509_set_issuer_name(out_cert, X509_get_issuer_name(server_cert));  
   
   /* Modify the issuer a little bit */ 
   X509_NAME_add_entry_by_txt(X509_get_issuer_name(out_cert), "L", MBSTRING_ASC, " ", -1, -1, 0);

/*
   index = X509_get_ext_by_NID(server_cert, NID_authority_key_identifier, -1);
   if (index >=0) {
      ext = X509_get_ext(server_cert, index);
      if (ext) {
         ext->value->data[7] = 0xe7;
         ext->value->data[8] = 0x7e;
         X509_add_ext(out_cert, ext, -1);
      }
   }
*/

   /* Self-sign our certificate */
   if (!X509_sign(out_cert, global_pk, EVP_sha1())) {
      X509_free(out_cert);
      DEBUG_MSG("Error self-signing X509");
      return NULL;
   }
     
   return out_cert;
}


/* 
 * Initialize SSL stuff 
 */
static void sslw_init(void)
{
   SSL *dummy_ssl=NULL;

   SSL_library_init();

   /* Create the two global CTX */
   ssl_ctx_client = SSL_CTX_new(SSLv23_server_method());
   ssl_ctx_server = SSL_CTX_new(SSLv23_client_method());

   /* Get our private key from our cert file */
   if (SSL_CTX_use_PrivateKey_file(ssl_ctx_client, INSTALL_DATADIR "/" EC_PROGRAM "/" CERT_FILE, SSL_FILETYPE_PEM) == 0) {
      DEBUG_MSG("sslw -- SSL_CTX_use_PrivateKey_file -- trying ./share/%s",  CERT_FILE);

      if (SSL_CTX_use_PrivateKey_file(ssl_ctx_client, "./share/" CERT_FILE, SSL_FILETYPE_PEM) == 0)
         FATAL_ERROR("Can't open \"./share/%s\" file : %s", CERT_FILE, strerror(errno));
   }

   dummy_ssl = SSL_new(ssl_ctx_client);
   if ( (global_pk = SSL_get_privatekey(dummy_ssl)) == NULL ) 
      FATAL_ERROR("Can't get private key from file");

   SSL_free(dummy_ssl);   
}


/* 
 * SSL thread child function.
 */
EC_THREAD_FUNC(sslw_child)
{
   struct packet_object po;
   int direction, ret_val, data_read;
   struct accepted_entry *ae;

   ae = (struct accepted_entry *)args;
   ec_thread_init();
 
   /* Contact the real server */
   if (sslw_sync_conn(ae) == -EINVALID) {
      close_socket(ae->fd[SSL_CLIENT]);
      SAFE_FREE(ae);
      ec_thread_exit();
   }	    
	    
   if ((ae->status & SSL_ENABLED) && 
      sslw_sync_ssl(ae) == -EINVALID) {
      sslw_wipe_connection(ae);
      ec_thread_exit();
   }

   /* A fake SYN ACK for profiles */
   sslw_initialize_po(&po, NULL);
   po.len = 64;
   po.L4.flags = (TH_SYN | TH_ACK);
   packet_disp_data(&po, po.DATA.data, po.DATA.len);
   sslw_parse_packet(ae, SSL_SERVER, &po);
   sslw_initialize_po(&po, po.DATA.data);
   
   LOOP {
      data_read = 0;
      for(direction=0; direction<2; direction++) {
         ret_val = sslw_read_data(ae, direction, &po);
         BREAK_ON_ERROR(ret_val,ae,po);
	 
         /* if we have data to read */
         if (ret_val == ESUCCESS) {
            data_read = 1;
            sslw_parse_packet(ae, direction, &po);
            if (po.flags & PO_DROPPED)
               continue;

            ret_val = sslw_write_data(ae, !direction, &po);
            BREAK_ON_ERROR(ret_val,ae,po);
	    
            if ((po.flags & PO_SSLSTART) && !(ae->status & SSL_ENABLED)) {
               ae->status |= SSL_ENABLED; 
               ret_val = sslw_sync_ssl(ae);
               BREAK_ON_ERROR(ret_val,ae,po);
            }
	    
            sslw_initialize_po(&po, po.DATA.data);
         }  
      }

      /* XXX - Set a proper sleep time */
      if (!data_read)
         usleep(3000);
   }

   return NULL;
}


/*******************************************/
/* Sessions' stuff for ssl packets */

static size_t sslw_create_ident(void **i, struct packet_object *po)
{
   struct sslw_ident *ident;

   /* allocate the ident for that session */
   SAFE_CALLOC(ident, 1, sizeof(struct sslw_ident));

   /* the magic */
   ident->magic = SSLW_MAGIC;
      
   /* prepare the ident */
   memcpy(&ident->L3_src, &po->L3.src, sizeof(struct ip_addr));

   ident->L4_src = po->L4.src;
   ident->L4_dst = po->L4.dst;

   /* return the ident */
   *i = ident;

   /* return the lenght of the ident */
   return sizeof(struct sslw_ident);
}


static int sslw_match(void *id_sess, void *id_curr)
{
   struct sslw_ident *ids = id_sess;
   struct sslw_ident *id = id_curr;

   /* sanity check */
   BUG_IF(ids == NULL);
   BUG_IF(id == NULL);
  
   /* 
    * is this ident from our level ?
    * check the magic !
    */
   if (ids->magic != id->magic)
      return 0;

   if (ids->L4_src == id->L4_src &&
       ids->L4_dst == id->L4_dst &&
       !ip_addr_cmp(&ids->L3_src, &id->L3_src)) 
      return 1;
   
   return 0;
}


static void sslw_create_session(struct ec_session **s, struct packet_object *po)
{
   void *ident;

   DEBUG_MSG("sslw_create_session");

   /* allocate the session */
   SAFE_CALLOC(*s, 1, sizeof(struct ec_session));
   
   /* create the ident */
   (*s)->ident_len = sslw_create_ident(&ident, po);
   
   /* link to the session */
   (*s)->ident = ident;

   /* the matching function */
   (*s)->match = &sslw_match;

   /* alloc of data elements */
   SAFE_CALLOC((*s)->data, 1, sizeof(struct ip_addr));
}

#endif /* HAVE_OPENSSL */

/* EOF */

// vim:ts=3:expandtab

