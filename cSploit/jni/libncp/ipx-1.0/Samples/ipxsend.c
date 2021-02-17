#include "samples.h"

int
main(int argc, char **argv)
{
	struct sockaddr_ipx sipx;
	int s;
	int result;
	char msg[100] = "Hi Mom";
	int len = sizeof(sipx);

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		perror("IPX: socket: ");
		exit(-1);
	}
	sipx.sipx_family = AF_IPX;
	sipx.sipx_network = 0;
	sipx.sipx_port = 0;
	sipx.sipx_type = 17;

	result = bind(s, (struct sockaddr *) &sipx, sizeof(sipx));
	if (result < 0)
	{
		perror("IPX: bind: ");
		exit(-1);
	}
	result = getsockname(s, (struct sockaddr *) &sipx, &len);
	sipx.sipx_port = htons(0x5000);
	result = sendto(s, msg, sizeof(msg), 0, (struct sockaddr *) &sipx,
			sizeof(sipx));
	if (result < 0)
	{
		perror("IPX: send: ");
		exit(-1);
	}
	return 0;
}
