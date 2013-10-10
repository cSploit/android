
/* $Id: ec_stats.h,v 1.8 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_STATS_H
#define EC_STATS_H

/*
 * this struct contains all field to collect 
 * statistics about packet and byte rate
 * for the bottom and top half
 */

struct half_stats {
   u_int64 pck_recv;
   u_int64 pck_size;
   struct timeval ttot;
   struct timeval tpar;
   struct timeval ts;
   struct timeval te;
   u_int64 tmp_size;
   u_int32 rate_adv;
   u_int32 rate_worst;
   u_int32 thru_adv;
   u_int32 thru_worst;
};

/* 
 * global statistics: bottom and top half + queue
 */

struct gbl_stats {
   u_int64 ps_recv;
   u_int64 ps_recv_delta;
   u_int64 ps_drop;
   u_int64 ps_drop_delta;
   u_int64 ps_ifdrop;
   u_int64 ps_sent;
   u_int64 ps_sent_delta;
   u_int64 bs_sent;
   u_int64 bs_sent_delta;
   struct half_stats bh;
   struct half_stats th;
   u_int32 queue_max;
   u_int32 queue_curr;
};

#define time_sub(a, b, result) do {                  \
   (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
   (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;  \
   if ((result)->tv_usec < 0) {                      \
      --(result)->tv_sec;                            \
      (result)->tv_usec += 1000000;                  \
   }                                                 \
} while (0)

#define time_add(a, b, result) do {                  \
   (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;     \
   (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;  \
   if ((result)->tv_usec >= 1000000) {               \
      ++(result)->tv_sec;                            \
      (result)->tv_usec -= 1000000;                  \
   }                                                 \
} while (0)


/* exports */

EC_API_EXTERN void stats_wipe(void);
EC_API_EXTERN void stats_update(void);

EC_API_EXTERN u_int32 stats_queue_add(void);
EC_API_EXTERN u_int32 stats_queue_del(void);

EC_API_EXTERN void stats_half_start(struct half_stats *hs);
EC_API_EXTERN void stats_half_end(struct half_stats *hs, u_int32 len);


#endif

/* EOF */

// vim:ts=3:expandtab

