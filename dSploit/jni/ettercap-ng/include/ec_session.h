
/* $Id: ec_session.h,v 1.8 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_SESSION_H
#define EC_SESSION_H


struct ec_session {
   void *ident;
   size_t ident_len;
   void *data;
   size_t data_len;
   /* Used to trace headers for injection */
   struct ec_session *prev_session;
   int (*match)(void *id_sess, void *id);
   void (*free)(void *data, size_t data_len);
};

EC_API_EXTERN void session_put(struct ec_session *s);
EC_API_EXTERN int session_get(struct ec_session **s, void *ident, size_t ident_len);
EC_API_EXTERN int session_del(void *ident, size_t ident_len);
EC_API_EXTERN int session_get_and_del(struct ec_session **s, void *ident, size_t ident_len);
EC_API_EXTERN void session_free(struct ec_session *s);
   

#endif

/* EOF */

// vim:ts=3:expandtab

