

#ifndef EL_FUNCTIONS_H
#define EL_FUNCTIONS_H

#include <ec_log.h>
#include <ec_profiles.h>

/* el_parser */
EL_API_EXTERN void parse_options(int argc, char **argv);
EL_API_EXTERN void expand_token(char *s, u_int max, void (*func)(void *t, int n), void *t );

/* el_analyze */
EL_API_EXTERN void analyze(void);
EL_API_EXTERN void create_hosts_list(void);

/* el_main */
EL_API_EXTERN void progress(int value, int max);
EL_API_EXTERN void set_color(int color);
EL_API_EXTERN void reset_color(void);

/* el_log */
EL_API_EXTERN void open_log(char *file);
EL_API_EXTERN int get_header(struct log_global_header *hdr);
EL_API_EXTERN int get_packet(struct log_header_packet *pck, u_char **buf);
EL_API_EXTERN int get_info(struct log_header_info *inf, struct dissector_info *buf);
EL_API_EXTERN void concatenate(int argc, char **argv);

/* el_display */
EL_API_EXTERN void display(void);
EL_API_EXTERN void set_display_regex(char *regex);

/* el_conn */
EL_API_EXTERN void conn_table_create(void);
EL_API_EXTERN void conn_table_display(void);
EL_API_EXTERN void conn_decode(void);
EL_API_EXTERN void filcon_compile(char *conn);
EL_API_EXTERN int is_conn(struct log_header_packet *pck, int *versus);
#define VERSUS_SOURCE   0
#define VERSUS_DEST     1 

/* el_target */
EL_API_EXTERN void target_compile(char *target);
EL_API_EXTERN int is_target_pck(struct log_header_packet *pck);
EL_API_EXTERN int is_target_info(struct host_profile *hst);
EL_API_EXTERN int find_user(struct host_profile *hst, char *user);

/* el_profiles */
EL_API_EXTERN int profile_add_info(struct log_header_info *inf, struct dissector_info *buf);
EL_API_EXTERN void *get_host_list_ptr(void);

/* el_stream */
struct so_list {
   int side;
   struct packet_object po;
   TAILQ_ENTRY(so_list) next;
};

struct so_offset {
   struct so_list *so_curr;
   size_t po_off;
};

struct stream_object {
   TAILQ_HEAD (so_list_head, so_list) so_head;
   struct so_offset side1;
   struct so_offset side2;
};

EL_API_EXTERN void stream_init(struct stream_object *so);
EL_API_EXTERN int stream_add(struct stream_object *so, struct log_header_packet *pck, char *buf);
EL_API_EXTERN struct so_list * stream_search(struct stream_object *so, const char *buf, size_t buflen, int mode);
EL_API_EXTERN int stream_read(struct stream_object *so, u_char *buf, size_t size, int mode);
   #define STREAM_SIDE1 0
   #define STREAM_SIDE2 ~0
EL_API_EXTERN int stream_move(struct stream_object *so, size_t offset, int whence, int mode);

/* el_decode */

enum {
   APP_LAYER_TCP = 51,
   APP_LAYER_UDP = 52,
};

#define FUNC_EXTRACTOR(func) int func(struct stream_object *so)
#define FUNC_EXTRACTOR_PTR(func) int (*func)(struct stream_object *so)
#define EXECUTE_EXTRACTOR(x, so, ret) do{ \
   if (x) \
      ret += x(so); \
}while(0)

#define STREAM so

EL_API_EXTERN int decode_stream(struct stream_object *so);
   #define STREAM_SKIPPED  0
   #define STREAM_DECODED  1
EL_API_EXTERN void add_extractor(u_int8 level, u_int32 type, FUNC_EXTRACTOR_PTR(extractor));
EL_API_EXTERN void * get_extractor(u_int8 level, u_int32 type);
EL_API_EXTERN int decode_to_file(char *host, char *proto, char *file);

#endif

/* EOF */

// vim:ts=3:expandtab

