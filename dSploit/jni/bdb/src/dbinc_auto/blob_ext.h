/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_blob_ext_h_
#define	_blob_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __blob_file_create __P ((DBC *, DB_FH **, uintmax_t *));
int  __blob_file_close __P ((DBC *, DB_FH *, u_int32_t));
int __blob_file_delete __P((DBC *, uintmax_t));
int __blob_file_open __P((DB *, DB_FH **, uintmax_t, u_int32_t));
int __blob_file_read __P((ENV *, DB_FH *, DBT *, off_t, u_int32_t));
int __blob_file_write __P((DBC *, DB_FH *, DBT *, off_t, uintmax_t, off_t *, u_int32_t));
int __blob_bulk __P((DBC *, u_int32_t, uintmax_t, u_int8_t *));
int __blob_get __P((DBC *, DBT *, uintmax_t, off_t, void **, u_int32_t *));
int __blob_put __P(( DBC *, DBT *, uintmax_t *, off_t *size, DB_LSN *));
int __blob_repl __P((DBC *, DBT *, uintmax_t, uintmax_t *,off_t *));
int __blob_del __P((DBC *, uintmax_t));
int __db_stream_init __P((DBC *, DB_STREAM **, u_int32_t));
int __db_stream_close_int __P ((DB_STREAM *));
int __blob_make_sub_dir __P((ENV *, char **, uintmax_t, uintmax_t));
int __blob_make_meta_fname __P((ENV *, DB *, char **));
int __blob_get_dir __P((DB *, char **));
int __blob_generate_dir_ids __P((DB *, DB_TXN *, uintmax_t *));
int __blob_generate_id __P((DB *, DB_TXN *, uintmax_t *));
int __blob_id_to_path __P((ENV *, const char *, uintmax_t, char **));
int __blob_salvage __P((ENV *, uintmax_t, off_t, size_t, uintmax_t, uintmax_t, DBT *));
int __blob_vrfy __P((ENV *, uintmax_t, off_t, uintmax_t, uintmax_t, db_pgno_t, u_int32_t));
int __blob_del_all __P((DB *, DB_TXN *, int));
int __blob_copy_all __P((DB*, const char *));

#if defined(__cplusplus)
}
#endif
#endif /* !_blob_ext_h_ */
