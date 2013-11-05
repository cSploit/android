#ifndef PCAP_H_
#define PCAP_H_

#include "config.h"
#include "libssh/libssh.h"

#ifdef WITH_PCAP
typedef struct ssh_pcap_context_struct* ssh_pcap_context;

int ssh_pcap_file_write_packet(ssh_pcap_file pcap, ssh_buffer packet, uint32_t original_len);

ssh_pcap_context ssh_pcap_context_new(ssh_session session);
void ssh_pcap_context_free(ssh_pcap_context ctx);

enum ssh_pcap_direction{
	SSH_PCAP_DIR_IN,
	SSH_PCAP_DIR_OUT
};
void ssh_pcap_context_set_file(ssh_pcap_context, ssh_pcap_file);
int ssh_pcap_context_write(ssh_pcap_context,enum ssh_pcap_direction direction, void *data,
		uint32_t len, uint32_t origlen);


#endif /* WITH_PCAP */
#endif /* PCAP_H_ */
/* vim: set ts=2 sw=2 et cindent: */
