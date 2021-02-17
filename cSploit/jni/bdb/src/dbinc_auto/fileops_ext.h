/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_fileops_ext_h_
#define	_fileops_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __fop_init_recover __P((ENV *, DB_DISTAB *));
int __fop_create_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_create_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_remove_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_file_60_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_file_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_42_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_file_remove_print __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_init_print __P((ENV *, DB_DISTAB *));
int __fop_create __P((ENV *, DB_TXN *, DB_FH **, const char *, const char **, APPNAME, int, u_int32_t));
int __fop_remove __P((ENV *, DB_TXN *, u_int8_t *, const char *, const char **, APPNAME, u_int32_t));
int __fop_write __P((ENV *, DB_TXN *, const char *, const char *, APPNAME, DB_FH *, u_int32_t, db_pgno_t, u_int32_t, void *, u_int32_t, u_int32_t, u_int32_t));
int __fop_write_file __P((ENV *, DB_TXN *, const char *, const char *, APPNAME, DB_FH *, off_t, void *, size_t, u_int32_t));
int __fop_rename __P((ENV *, DB_TXN *, const char *, const char *, const char **, u_int8_t *, APPNAME, int, u_int32_t));
int __fop_create_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_create_42_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_remove_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_file_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_file_60_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_42_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_write_file_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_noundo_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_42_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_rename_noundo_46_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_file_remove_recover __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
int __fop_lock_handle __P((ENV *, DB *, DB_LOCKER *, db_lockmode_t, DB_LOCK *, u_int32_t));
int __fop_file_setup __P((DB *, DB_THREAD_INFO *ip, DB_TXN *, const char *, int, u_int32_t, u_int32_t *));
int __fop_subdb_setup __P((DB *, DB_THREAD_INFO *, DB_TXN *, const char *, const char *, int, u_int32_t));
int __fop_remove_setup __P((DB *, DB_TXN *, const char *, u_int32_t));
int __fop_read_meta __P((ENV *, const char *, u_int8_t *, size_t, DB_FH *, int, size_t *));
int __fop_dummy __P((DB *, DB_TXN *, const char *, const char *, APPNAME));
int __fop_dbrename __P((DB *, const char *, const char *, APPNAME));

#if defined(__cplusplus)
}
#endif
#endif /* !_fileops_ext_h_ */
