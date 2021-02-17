#include "samples.h"

struct rip_data
{
	u_int32_t	rip_net __attribute__((packed));
	u_int16_t	rip_hops __attribute__((packed));
	u_int16_t	rip_ticks __attribute__((packed));
};

int
main(int argc, char **argv)
{
	struct sockaddr_ipx sipx;
	int result;
	int s;
	char msg[1024];
	int len;
	char *bptr;
	struct rip_data *rp;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		perror("IPX: socket: ");
		exit(-1);
	}
	sipx.sipx_family = AF_IPX;
	sipx.sipx_network = 0;
	sipx.sipx_port = htons(0x453);
	sipx.sipx_type = 17;
	result = bind(s, (struct sockaddr *) &sipx, sizeof(sipx));
	if (result < 0)
	{
		perror("IPX: bind: ");
		exit(-1);
	}
	while (1)
	{
		len = sizeof(sipx);
		result = recvfrom(s, msg, sizeof(msg), 0,
				  (struct sockaddr *) &sipx, &len);
		if (result < 0)
		{
			perror("IPX: recvfrom");
			exit(-1);
		}
		bptr = msg;
		result -= 2;
		printf("RIP packet from: %08X:%02X%02X%02X%02X%02X%02X\n",
		       (u_int32_t)htonl(sipx.sipx_network),
		       sipx.sipx_node[0], sipx.sipx_node[1],
		       sipx.sipx_node[2], sipx.sipx_node[3],
		       sipx.sipx_node[6], sipx.sipx_node[5]);
		bptr += 2;
		rp = (struct rip_data *) bptr;
		while (result >= sizeof(struct rip_data))
		{
			printf("\tNET: %08X HOPS: %d\n", (u_int32_t)ntohl(rp->rip_net),
			       ntohs(rp->rip_hops));
			result -= sizeof(struct rip_data);
			rp++;
		}
	}
}
