

#ifndef EC_CAPTURE_H
#define EC_CAPTURE_H

#include <ec.h>
#include <ec_threads.h>

EC_API_EXTERN void capture_start(struct iface_env *);
EC_API_EXTERN void capture_stop(struct iface_env *);

EC_API_EXTERN EC_THREAD_FUNC(capture);

EC_API_EXTERN int is_pcap_file(char *file, char *pcap_errbuf);
EC_API_EXTERN void capture_getifs(void);

#define FUNC_ALIGNER(func) int func(void)
#define FUNC_ALIGNER_PTR(func) int (*func)(void)

EC_API_EXTERN u_int8 get_alignment(int dlt);
EC_API_EXTERN void add_aligner(int dlt, int (*aligner)(void));

#endif

/* EOF */

// vim:ts=3:expandtab

