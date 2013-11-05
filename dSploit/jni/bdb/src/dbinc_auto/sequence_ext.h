/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_sequence_ext_h_
#define	_sequence_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __seq_stat __P((DB_SEQUENCE *, DB_SEQUENCE_STAT **, u_int32_t));
int __seq_stat_print __P((DB_SEQUENCE *, u_int32_t));
const FN * __db_get_seq_flags_fn __P((void));
const FN * __db_get_seq_flags_fn __P((void));
int __seq_open __P((DB_SEQUENCE *, DB_TXN *, DBT *, u_int32_t));
int __seq_initial_value  __P((DB_SEQUENCE *, db_seq_t));
int __seq_get __P((DB_SEQUENCE *, DB_TXN *, u_int32_t,  db_seq_t *, u_int32_t));
int __seq_close __P((DB_SEQUENCE *, u_int32_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_sequence_ext_h_ */
