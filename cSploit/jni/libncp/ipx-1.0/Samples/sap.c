#include "samples.h"

struct sap_data
{
	u_int16_t	sap_type __attribute__((packed));
	char sap_name[48] __attribute__((packed));
	u_int32_t	sap_net __attribute__((packed));
	u_int8_t	sap_node[6] __attribute__((packed));
	u_int16_t	sap_sock __attribute__((packed));
	u_int16_t	sap_hops __attribute__((packed));
};

int
main(int argc, char **argv)
{
	int s;
	int result;
	struct sockaddr_ipx sipx;
	char msg[1024];
	long val = 0;
	int len;
	char *bptr;
	struct sap_data *sp;

	s = socket(AF_IPX, SOCK_DGRAM, AF_IPX);
	if (s < 0)
	{
		perror("IPX: socket: ");
		exit(-1);
	}
	result = setsockopt(s, SOL_SOCKET, SO_DEBUG, &val, 4);
	if (result < 0)
	{
		perror("IPX: setsockopt: ");
		exit(-1);
	}
	sipx.sipx_family = PF_IPX;
	sipx.sipx_network = 0L;
	sipx.sipx_port = htons(0x452);
	sipx.sipx_type = 17;

	result = bind(s, (struct sockaddr *) &sipx, sizeof(sipx));
	if (result < 0)
	{
		perror("IPX: bind: ");
		exit(-1);
	}
	while (1)
	{
		len = 1024;
		result = recvfrom(s, msg, sizeof(msg), 0,
				  (struct sockaddr *) &sipx, &len);
		if (result < 0)
		{
			perror("IPX: recvfrom: ");
			exit(-1);
		}
		bptr = msg;
		result -= 2;
		printf("SAP: OP is %x %x\n", bptr[0], bptr[1]);
		printf("Length is %d\n", result);
		if (bptr[1] != 2)
			continue;

		bptr += 2;
		sp = (struct sap_data *) bptr;
		while (result >= sizeof(struct sap_data))
		{
			int i;

			sp->sap_name[32] = '\0';
			for (i = 31; (i > 0) && (sp->sap_name[i] == '_'); i--);
			i++;
			sp->sap_name[i] = '\0';
			printf("NAME: %s TYPE: %x HOPS: %x\n", sp->sap_name,
			       ntohs(sp->sap_type), ntohs(sp->sap_hops));
			printf("%x:%x %x %x %x %x %x: %x\n",
			       (u_int32_t)ntohl(sp->sap_net),
			       sp->sap_node[0],
			       sp->sap_node[1],
			       sp->sap_node[2],
			       sp->sap_node[3],
			       sp->sap_node[4],
			       sp->sap_node[5],
			       ntohs(sp->sap_sock));
			result -= sizeof(struct sap_data);
			sp++;
		}
	}
}
