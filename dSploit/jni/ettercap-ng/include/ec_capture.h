
/* $Id: ec_capture.h,v 1.7 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_CAPTURE_H
#define EC_CAPTURE_H

#include <ec_threads.h>

EC_API_EXTERN void capture_init(void);
EC_API_EXTERN void capture_close(void);
EC_API_EXTERN EC_THREAD_FUNC(capture);
EC_API_EXTERN EC_THREAD_FUNC(capture_bridge);

EC_API_EXTERN void get_hw_info(void);
EC_API_EXTERN int is_pcap_file(char *file, char *errbuf);
EC_API_EXTERN void capture_getifs(void);

#define FUNC_ALIGNER(func) int func(void)
#define FUNC_ALIGNER_PTR(func) int (*func)(void)

EC_API_EXTERN void add_aligner(int dlt, int (*aligner)(void));

#endif

/* EOF */

// vim:ts=3:expandtab

