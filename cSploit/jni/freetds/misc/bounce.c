#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

/* $Id: bounce.c,v 1.2 2010-12-30 12:39:11 freddy77 Exp $ */

/* This small application make man-in-the-middle with a crypted SQL Server
 * to be able to see decrypted login
 * It's just a small utility, at end of login it close connection and don't
 * handle a lot of cases. Works only with mssql2k or later
 * Based on GnuTLS echo example
 * compile with:
 *    gcc -O2 -Wall -o bounce bounce.c -lgnutls
 */

/* path to certificate, can be created with 
 * openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout mycert.pem -out mycert.pem
 */
#define CERTFILE "mycert.pem"

/* port to listen to, you should connect to this port */
#define LISTEN_PORT 1433
/* server and port to connect to, the real server you want to tunnel */
#define SERVER_IP   "10.20.30.40"
#define SERVER_PORT 1433

#define SA struct sockaddr
#define SOCKET_ERR(err,s) if(err==-1) {perror(s);return(1);}
#define MAX_BUF 1024
#define DH_BITS 1024

/* These are global */
gnutls_certificate_credentials_t x509_cred;

static void put_packet(int sd);
static void get_packet(int sd);

typedef enum
{
	prelogin,
	auth,
	in_tls
} State;
static State state;

static int server_sd = -1;
static int client_sd = -1;

static unsigned char packet[4096 + 192];
static int packet_len;
static int to_send = 0;
static unsigned char packet_type = 0x12;
static int pos = 0;

static ssize_t
tds_pull_func(gnutls_transport_ptr ptr, void *data, size_t len)
{
	fprintf(stderr, "in tds_pull_func\n");

	/* if we have some data send it */
	if (to_send && packet_len >= 8) {
		packet[1] = 1;
		put_packet(client_sd);
		packet_len = 0;
		to_send = 0;
	}

	if (state == in_tls) {
		return recv(client_sd, data, len, 0);
	}
	/* read from packet */
	if (!packet_len || pos >= packet_len) {
		get_packet(client_sd);
		pos = 8;
	}
	if (!packet_len)
		exit(1);
	if (len > (packet_len - pos))
		len = packet_len - pos;
	memcpy(data, packet + pos, len);
	pos += len;
	printf("read %d bytes\n", (int) len);
	return len;
}

static ssize_t
tds_push_func(gnutls_transport_ptr ptr, const void *data, size_t len)
{
	int left;

	if (state == in_tls)
		return send(server_sd, data, len, 0);

	/* write to packet */
	if (!to_send)
		packet_len = 8;
	to_send = 1;
	packet[0] = packet_type;
	left = 4096 - packet_len;
	if (left <= 0) {
		packet[1] = 0;	/* not last */
		put_packet(server_sd);
		packet_len = 8;
		left = 4096 - packet_len;
	}
	packet[1] = 1;		/* last */
	if (len > left)
		len = left;
	memcpy(packet + packet_len, data, len);
	packet_len += len;
	packet[2] = packet_len >> 8;
	packet[3] = packet_len;
	return len;
}

static gnutls_session_t
initialize_tls_session()
{
	gnutls_session_t session;
	static const int cipher_priority[] = {
		GNUTLS_CIPHER_ARCFOUR_40,
		GNUTLS_CIPHER_DES_CBC,
		GNUTLS_CIPHER_AES_256_CBC, GNUTLS_CIPHER_AES_128_CBC,
		GNUTLS_CIPHER_3DES_CBC, GNUTLS_CIPHER_ARCFOUR_128,
		0
	};


	gnutls_init(&session, GNUTLS_SERVER);
	gnutls_transport_set_pull_function(session, tds_pull_func);
	gnutls_transport_set_push_function(session, tds_push_func);

	/* avoid calling all the priority functions, since the defaults
	 * are adequate.
	 */
	gnutls_set_default_priority(session);
	gnutls_cipher_set_priority(session, cipher_priority);

	gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, x509_cred);

	gnutls_dh_set_prime_bits(session, DH_BITS);

	return session;
}

static gnutls_dh_params_t dh_params;

static int
generate_dh_params(void)
{

	/* Generate Diffie Hellman parameters - for use with DHE
	 * kx algorithms. These should be discarded and regenerated
	 * once a day, once a week or once a month. Depending on the
	 * security requirements.
	 */
	gnutls_dh_params_init(&dh_params);
	gnutls_dh_params_generate2(dh_params, DH_BITS);

	return 0;
}

static void
tds_tls_log(int level, const char *s)
{
	printf("(%d) %s", level, s);
}

static int
tcp_connect(void)
{
	int err, sd;
	struct sockaddr_in sa;

	/* connects to server 
	 */
	sd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&sa, '\0', sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_IP, &sa.sin_addr);

	err = connect(sd, (SA *) & sa, sizeof(sa));
	if (sd < 0 || err < 0) {
		fprintf(stderr, "Connect error\n");
		exit(1);
	}

	return sd;
}

static void
get_packet(int sd)
{
	int full_len, l;

	printf("get_packet\n");
	packet_len = 0;
	for (;;) {
		full_len = 4;
		if (packet_len >= 4)
			full_len = (((int) packet[2]) << 8) + packet[3];

		l = recv(sd, packet + packet_len, full_len - packet_len, 0);
		if (l <= 0) {
			fprintf(stderr, "error recv\n");
			exit(1);
		}
		packet_len += l;

		if (full_len >= 8 && packet_len == full_len)
			break;
	}
}

static void
put_packet(int sd)
{
	int sent = 0;

	printf("put_packet\n");
	for (; sent < packet_len;) {
		int l = send(sd, packet + sent, packet_len - sent, 0);

		if (l <= 0) {
			fprintf(stderr, "error send\n");
			exit(1);
		}
		sent += l;
	}
	to_send = 0;
}

static void
hexdump(char *buffer, int len)
{
	int i;
	char hex[16 * 3 + 2], chars[20];

	hex[0] = 0;
	for (i = 0; len > 0 && i < len; ++i) {
		if ((i & 15) == 0) {
			hex[0] = 0;
			chars[0] = 0;
		}
		sprintf(strchr(hex, 0), "%02x ", (unsigned char) buffer[i]);
		chars[i & 15] = buffer[i] >= 32 && buffer[i] < 126 ? buffer[i] : ' ';
		chars[(i & 15) + 1] = 0;
		if ((i & 15) == 15)
			printf("%04x: %-48s %s\n", i & 0xfff0, hex, chars);
	}
	if ((i & 15) != 0)
		printf("%04x: %-48s %s\n", i & 0xfff0, hex, chars);
}

int
main()
{
	int err, listen_sd, i;
	int sd, ret;
	struct sockaddr_in sa_serv;
	struct sockaddr_in sa_cli;
	socklen_t client_len;
	char topbuf[512];
	gnutls_session_t session;
	char buffer[MAX_BUF + 1];
	int optval = 1;

	/* this must be called once in the program
	 */
	gnutls_global_init();
	gnutls_global_set_log_level(11);
	gnutls_global_set_log_function(tds_tls_log);


	gnutls_certificate_allocate_credentials(&x509_cred);

	ret = gnutls_certificate_set_x509_key_file(x509_cred, /* CERTFILE */ CERTFILE, /* KEYFILE */ CERTFILE,
						   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fprintf(stderr, "certificate failed (%s)\n", gnutls_strerror(ret));
		exit(1);
	}

	generate_dh_params();

	gnutls_certificate_set_dh_params(x509_cred, dh_params);

	/* Socket operations
	 */
	listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	SOCKET_ERR(listen_sd, "socket");

	memset(&sa_serv, '\0', sizeof(sa_serv));
	sa_serv.sin_family = AF_INET;
	sa_serv.sin_addr.s_addr = INADDR_ANY;
	sa_serv.sin_port = htons(LISTEN_PORT);	/* Server Port number */

	setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

	err = bind(listen_sd, (SA *) & sa_serv, sizeof(sa_serv));
	SOCKET_ERR(err, "bind");
	err = listen(listen_sd, 1024);
	SOCKET_ERR(err, "listen");

	printf("Server ready. Listening to port '%d'.\n\n", LISTEN_PORT);

	client_len = sizeof(sa_cli);
	for (;;) {
		session = initialize_tls_session();

		client_sd = sd = accept(listen_sd, (SA *) & sa_cli, &client_len);

		printf("- connection from %s, port %d\n",
		       inet_ntop(AF_INET, &sa_cli.sin_addr, topbuf, sizeof(topbuf)), ntohs(sa_cli.sin_port));

		/* now do prelogin */
		/* connect to real peer */
		printf("connect to real peer\n");
		server_sd = tcp_connect();

		/* get prelogin packet from client */
		printf("get prelogin packet from client\n");
		get_packet(client_sd);

		/* send prelogin packet to server */
		printf("send prelogin packet to server\n");
		put_packet(server_sd);

		/* get prelogin reply from server */
		printf("get prelogin reply from server\n");
		get_packet(server_sd);

		/* reply with same prelogin packet */
		printf("reply with same prelogin packet\n");
		put_packet(client_sd);

		/* now we must do authentication with client and with server */
		state = auth;
		packet_len = 0;

		/* do with client */
		gnutls_transport_set_ptr(session, (gnutls_transport_ptr_t) (((char*)0)+sd));
		ret = gnutls_handshake(session);
		if (ret < 0) {
			close(sd);
			gnutls_deinit(session);
			fprintf(stderr, "*** Handshake has failed (%s)\n\n", gnutls_strerror(ret));
			continue;
		}
		printf("- Handshake was completed\n");

		/* flush last packet */
		packet[0] = 4;
		packet[1] = 1;
		put_packet(client_sd);

		/* on, reset all */
		state = in_tls;
		to_send = 0;
		packet_len = 0;

		/* do with server */

		/* now log and do man-in-the-middle to see decrypted data !!! */
		for (;;) {
			/* wait all packet */
			sleep(2);

			/* get client */
			ret = gnutls_record_recv(session, buffer, MAX_BUF);
			if (ret > 0) {
				hexdump(buffer, ret);

				gnutls_record_send(session, buffer, ret);

				ret = gnutls_record_recv(session, buffer, MAX_BUF);
				if (ret > 0)
					hexdump(buffer, ret);
			}
			/* for the moment.. */
			exit(1);

			/* send to server */
		}

		/* see the Getting peer's information example */
		/* print_info(session); */

		i = 0;
		for (;;) {
			bzero(buffer, MAX_BUF + 1);
			ret = gnutls_record_recv(session, buffer, MAX_BUF);

			if (ret == 0) {
				printf("\n- Peer has closed the GNUTLS connection\n");
				break;
			} else if (ret < 0) {
				fprintf(stderr, "\n*** Received corrupted " "data(%d). Closing the connection.\n\n", ret);
				break;
			} else if (ret > 0) {
				/* echo data back to the client */
				hexdump(buffer, ret);
				gnutls_record_send(session, buffer, ret);
			}
		}
		printf("\n");
		/* do not wait for the peer to close the connection. */
		gnutls_bye(session, GNUTLS_SHUT_WR);

		close(sd);
		gnutls_deinit(session);

	}
	close(listen_sd);

	gnutls_certificate_free_credentials(x509_cred);

	gnutls_global_deinit();

	return 0;
}

