#include "samples.h"

int
main(int argc, char **argv)
{
	struct sockaddr_ipx sipx;
	int s;
	int result;
	char msg[100];
	int len;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		perror("IPX: socket: ");
		exit(-1);
	}
	sipx.sipx_family = AF_IPX;
	sipx.sipx_network = 0;
	sipx.sipx_port = htons(0x5000);
	sipx.sipx_type = 17;
	len = sizeof(sipx);
	result = bind(s, (struct sockaddr *) &sipx, sizeof(sipx));
	if (result < 0)
	{
		perror("IPX: bind: ");
		exit(-1);
	}
	msg[0] = '\0';
	result = recvfrom(s, msg, sizeof(msg), 0, (struct sockaddr *) &sipx,
			  &len);
	if (result < 0)
	{
		perror("IPX: recvfrom: ");
	}
	printf("From %08X:%02X%02X%02X%02X%02X%02X:%04X\n",
	       (u_int32_t)htonl(sipx.sipx_network),
	       sipx.sipx_node[0], sipx.sipx_node[1],
	       sipx.sipx_node[2], sipx.sipx_node[3],
	       sipx.sipx_node[4], sipx.sipx_node[5],
	       htons(sipx.sipx_port));
	printf("\tGot \"%s\"\n", msg);
	return 0;
}
