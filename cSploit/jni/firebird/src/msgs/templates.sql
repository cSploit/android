INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('C', 'gds.h',
'/*
 *	PROGRAM:	C preprocessor
 *	MODULE:		gds.h
 *	DESCRIPTION:	BLR constants
 *
 * copyright (c) 1984, 1990 by Interbase Software Corporation
 */

#define gds_version3

#define GDS_$TRUE	1
#define GDS_$FALSE      0

#ifndef apollo
#define GDS_VAL(val)	val
#define GDS_REF(val)	&val
#define GDS_TYPE	long

#else
#define GDS_VAL(val)	(*val)
#define GDS_REF(val)	val
#define GDS_TYPE	std_$call
#endif

GDS_TYPE gds_$attach_database ();
GDS_TYPE gds_$blob_info();
GDS_TYPE gds_$cancel_blob();
GDS_TYPE gds_$close_blob();
GDS_TYPE gds_$commit_transaction ();
GDS_TYPE gds_$compile_request ();
GDS_TYPE gds_$compile_request2 ();
GDS_TYPE gds_$create_blob ();
GDS_TYPE gds_$create_blob2 ();
GDS_TYPE gds_$create_database ();
GDS_TYPE gds_$database_info ();
GDS_TYPE gds_$detach_database ();
GDS_TYPE gds_$get_segment ();
GDS_TYPE gds_$open_blob ();
GDS_TYPE gds_$open_blob2 ();
GDS_TYPE gds_$prepare_transaction ();
GDS_TYPE gds_$prepare_transaction2 ();
GDS_TYPE gds_$put_segment ();
GDS_TYPE gds_$receive ();
GDS_TYPE gds_$reconnect_transaction ();
GDS_TYPE gds_$request_info ();
GDS_TYPE gds_$release_request ();
GDS_TYPE gds_$rollback_transaction ();
GDS_TYPE gds_$seek_blob ();
GDS_TYPE gds_$send ();
GDS_TYPE gds_$set_debug ();
GDS_TYPE gds_$start_and_send ();
GDS_TYPE gds_$start_multiple ();
GDS_TYPE gds_$start_request ();
GDS_TYPE gds_$start_transaction ();
GDS_TYPE gds_$transaction_info ();
GDS_TYPE gds_$unwind_request ();
GDS_TYPE gds_$ftof ();
GDS_TYPE gds_$print_status ();
GDS_TYPE gds_$sqlcode ();
GDS_TYPE gds_$ddl();
GDS_TYPE gds_$commit_retaining();
GDS_TYPE gds_$que_events();
GDS_TYPE gds_$cancel_events();
GDS_TYPE gds_$event_wait();
GDS_TYPE gds_$event_counts();
GDS_TYPE gds_$event_block();
GDS_TYPE gds_$event_block2();
GDS_TYPE gds_$get_slice();
GDS_TYPE gds_$put_slice();
GDS_TYPE gds_$seek_blob();

void	  gds_$vtof ();


typedef struct {
    long		gds_quad_high;
    unsigned long	gds_quad_low;
} GDS_$QUAD;

#define blr_word(n) (n % 256), (n / 256)

$ SYMBOLS BLR DTYPE "#define %-34s %d\n

$ SYMBOLS BLR JOIN "#define %-34s %d\n

$ SYMBOLS BLR MECH "#define %-34s %d\n

$ SYMBOLS BLR STATEMENTS "#define %-34s %d\n

$ SYMBOLS BLR VALUES "#define %-34s %d\n

$ SYMBOLS BLR BOOLEANS "#define %-34s %d\n

$ SYMBOLS BLR RSE "#define %-34s %d\n

$ SYMBOLS BLR AGGREGATE "#define %-34s %d\n

$ SYMBOLS BLR NEW "#define %-34s %d\n

/* Database parameter block stuff */

$ SYMBOLS DPB ITEMS "#define %-34s %d\n

$ SYMBOLS DPB BITS "#define %-34s %d\n

/* Bit assignments in RDB$SYSTEM_FLAG */

$ SYMBOLS RDB FLAG "#define %-34s %d\n

/* Transaction parameter blob stuff */

$ SYMBOLS TPB ITEMS "#define %-34s %d\n

/* Blob Parameter Block */

$ SYMBOLS BPB ITEMS "#define %-34s %d\n

/* Blob stream stuff */ 

typedef struct bstream {
    int		*bstr_blob;		/* Blob handle */
    char	*bstr_buffer;		/* Address of buffer */
    char	*bstr_ptr;		/* Next character */
    short	bstr_length;		/* Length of buffer */
    short	bstr_cnt;		/* Characters in buffer */
    char	bstr_mode;		/* (mode) ? OUTPUT : INPUT */
} BSTREAM, * FB_BLOB_STREAM;

#define getb(p)	(--(p)->bstr_cnt >= 0 ? *(p)->bstr_ptr++ & 0377: BLOB_get (p))
#define putb(x,p) ((x == '\n' || (!(--(p)->bstr_cnt))) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))
#define putbx(x,p) ((!(--(p)->bstr_cnt)) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))

FB_BLOB_STREAM	Bopen(), BLOB_open();

/* Information call declarations */

/* Common, structural codes */

$ SYMBOLS INFO MECH "#define %-34s %d\n

/* Database information items */

$ SYMBOLS INFO DB "#define %-34s %d\n

/* Database Info Return Values */

$ SYMBOLS INFO VALUES "#define %-34s %d\n

/* Request information items */

$ SYMBOLS INFO REQUEST "#define %-34s %d\n

/* Blob information items */

$ SYMBOLS INFO BLOB "#define %-34s %d\n

/* Transaction information items */

$ SYMBOLS INFO TRANSACTION "#define %-34s %d\n

/* Error codes */

$ SYMBOLS ERROR MECH "#define %-34s %d\n

$ ERROR MAJOR "#define %-34s %d\n

/* Minor codes subject to change */

$ ERROR MINOR "#define %-34s %d\n

#define gds_err_max   143

/* Dynamic Data Definition Language operators */

/* Version number */

$ SYMBOLS DYN MECH "#define %-34s %d\n

/* Operations (may be nested) */

$ SYMBOLS DYN OPERATIONS "#define %-34s %d\n

/* View specific stuff */

$ SYMBOLS DYN VIEW "#define %-34s %d\n

/* Generic attributes */

$ SYMBOLS DYN GENERIC "#define %-34s %d\n

/* Relation specific attributes */

$ SYMBOLS DYN RELATION "#define %-34s %d\n

/* Global field specific attributes */

$ SYMBOLS DYN GLOBAL "#define %-34s %d\n

/* Local field specific attributes */

$ SYMBOLS DYN FIELD "#define %-34s %d\n

/* Index specific attributes */

$ SYMBOLS DYN INDEX "#define %-34s %d\n

/* Trigger specific attributes */

$ SYMBOLS DYN TRIGGER "#define %-34s %d\n

/* Security Class specific attributes */

$ SYMBOLS DYN SECURITY "#define %-34s %d\n

/* Dimension specific information */

$ SYMBOLS DYN ARRAY "#define %-34s %d\n

/* File specific attributes */

$ SYMBOLS DYN FILES "#define %-34s %d\n

/* Function specific attributes */

$ SYMBOLS DYN FUNCTIONS "#define %-34s %d\n

/* Generator specific attributes */

$ SYMBOLS DYN GENERATOR "#define %-34s %d\n

/* Array slice description language (SDL) */

$ SYMBOLS SDL SDL "#define %-34s %d\n

/* Dynamic SQL definitions */

typedef struct {
    short	sqltype;
    short	sqllen;
    char	*sqldata;
    short	*sqlind;
    short	sqlname_length;
    char	sqlname [30];
} SQLVAR;

typedef struct {
    char	sqldaid [8];
    long	sqldabc;
    short	sqln;
    short	sqld;
    SQLVAR	sqlvar[1];
} SQLDA;

#define SQLDA_LENGTH(n)		(sizeof (SQLDA) + (n-1) * sizeof (SQLVAR))

$ SYMBOLS SQL DTYPE "#define %-34s %d\n

/* Forms Package definitions */

/* Map definition block definitions */

$ SYMBOLS PYXIS MAP  "#define %-34s %d\n

/* Field option flags for display options */

$ SYMBOLS PYXIS DISPLAY  "#define %-34s %d\n

/* Field option values following display */

$ SYMBOLS PYXIS VALUE  "#define %-34s %d\n

/* Pseudo key definitions */

$ SYMBOLS PYXIS KEY  "#define %-34s %d\n

/* Menu definition stuff */

$ SYMBOLS PYXIS MENU  "#define %-34s %d\n');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('C++', 'gds.hxx',
'/*
 *	PROGRAM:	C preprocessor
 *	MODULE:		gds.hxx
 *	DESCRIPTION:	BLR constants for C++
 *
 * copyright (c) 1984, 1990 by Interbase Software Corporation
 */
 
#define gds_version3
 
#define GDS_TRUE	1
#define GDS_FALSE	0

typedef long	gds_db_handle;
typedef long	gds_req_handle;
typedef long	gds_tr_handle;
typedef long	gds_blob_handle;
typedef long	gds_win_handle;
typedef long	gds_form_handle;
typedef void	(*gds_callback)();

typedef struct {
    long		gds_quad_high;
    unsigned long	gds_quad_low;
} GDS_QUAD;

/* Dynamic SQL definitions */
 
typedef struct {
    short	sqltype;
    short	sqllen;
    char	*sqldata;
    short	*sqlind;
    short	sqlname_length;
    char	sqlname [30];
} SQLVAR;
 
typedef struct {
    char	sqldaid [8];
    long	sqldabc;
    short	sqln;
    short	sqld;
    SQLVAR	sqlvar[1];
} SQLDA;
 
#define SQLDA_LENGTH(n)		(sizeof (SQLDA) + (n-1) * sizeof (SQLVAR))

extern long 
    isc_attach_database (long*, short, char*, gds_db_handle*, short, char*),
    isc_blob_info (long*, gds_blob_handle*, short, char*, short, char*),
    isc_cancel_blob (long*, gds_blob_handle*),
    isc_close_blob (long*, gds_blob_handle*),
    isc_commit_transaction (long*, gds_tr_handle*),
    isc_compile_request  (long*, gds_db_handle*, gds_req_handle*, short, char*),
    isc_compile_request2 (long*, gds_db_handle*, gds_req_handle*, short, char*),
    isc_create_blob  (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*),
    isc_create_blob2  (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*, short, char*),
    isc_create_database  (long*, short, char*, gds_db_handle*, short, char*),
    isc_database_info  (long*, gds_db_handle*, short, char*, short, char*),
    isc_detach_database (long*, gds_db_handle*),
    isc_get_segment (long*, gds_blob_handle*, unsigned short*, short, char*),
    isc_open_blob (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*),
    isc_open_blob2 (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*, short, char*),
    isc_prepare_transaction  (long*, gds_tr_handle*),
    isc_prepare_transaction2  (long*, gds_tr_handle*, short, char*),
    isc_put_segment  (long*, gds_blob_handle*, short, char*),
    isc_receive (long*, gds_req_handle*, short, short, void*, short),
    isc_reconnect_transaction  (long*, gds_db_handle*, gds_tr_handle*, short, char*),
    isc_request_info  (long*, gds_req_handle*, short, char*, short, char*, short),
    isc_release_request  (long*, gds_req_handle*),
    isc_rollback_transaction  (long*, gds_tr_handle*),
    isc_send (long*, gds_req_handle*, short, short, void*, short),
    isc_start_and_send (long*, gds_req_handle*, gds_tr_handle*, short, short, void*, short),
    isc_start_multiple  (long*, gds_tr_handle*, short, void*),
    isc_start_request (long*, gds_req_handle*, gds_tr_handle*, short),
    isc_start_transaction (long*, gds_tr_handle*, short, gds_db_handle*, short, char*, ...),
    isc_transaction_info  (long*, gds_tr_handle*, short, char*, short, char*),
    isc_unwind_request  (long*, gds_tr_handle*, short),
    isc_print_status  (long*),
    isc_sqlcode  (long*),
    isc_ddl (long*, gds_db_handle*, gds_tr_handle*, short, char*),
    isc_commit_retaining (long*, gds_tr_handle*),
    isc_que_events (long*, gds_db_handle*, long*, short, char*, gds_callback, long*),
    isc_cancel_events (long*, gds_db_handle*, long*),
    isc_event_wait (long*, gds_db_handle*, short, char*, char*),
    isc_event_counts (long*, short, char*, char*),
    isc_event_block (char**, char**, short, ...),
    isc_get_slice (long*, gds_db_handle*, gds_tr_handle*, GDS_QUAD*, short, char*, short, long*,
		    long, void*, long*),
    isc_put_slice (long*, gds_db_handle*, gds_tr_handle*, GDS_QUAD*, short, char*, short, long*,
		    long, void*);

extern long 
    isc_prepare (long*, gds_db_handle*, gds_tr_handle*, char*, short*, char*, SQLDA*),
    isc_declare (long*, char*, char*),
    isc_open (long*, gds_tr_handle*, char*, SQLDA*),
    isc_fetch (long*, char*, SQLDA*),
    isc_close (long*, char*),
    isc_describe (long*, char*, SQLDA*),
    isc_execute (long*, gds_tr_handle*, char*, SQLDA*),
    isc_execute_immediate (long*, gds_db_handle*, gds_tr_handle*, short*, char*),
    isc_ddl (long*, gds_db_handle*, gds_tr_handle*, short, char*);

extern long
    BLOB_edit (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);

extern long 
    isc_initialize_menu (long*, gds_req_handle*),
    isc_put_entree (long*, gds_req_handle*, short*, char*, long*),
    isc_drive_menu (long*, gds_win_handle*, gds_req_handle*, short*, char*, short*, char*, short*,
		       short*, char*, long*),
    isc_drive_form (long*, gds_db_handle*, gds_tr_handle*, gds_win_handle*, gds_req_handle*,
		void*, void*),
    isc_load_form (long*, gds_db_handle*, gds_tr_handle*, gds_form_handle*, short*, char*),
    isc_compile_map (long*, gds_form_handle*, gds_req_handle*, short*, char*),
    isc_compile_sub_map (long*, gds_win_handle*, gds_req_handle*, short*, char*),
    isc_form_fetch (long*, gds_db_handle*, gds_tr_handle*, gds_req_handle*, void*),
    isc_reset_form (long*, gds_req_handle*),
    isc_pop_window (long*, gds_win_handle*),
    isc_create_window (long*, gds_win_handle*, short*, char*, short*, short*),
    isc_menu (long*, gds_win_handle*, gds_req_handle*, short*, char*);

extern void 
    isc_ftof  (char*, short, char*, short),
    isc_vtof (char*, char*),
    isc_vtov (char*, char*, short),
    isc_version (gds_db_handle*, gds_callback, long);

#define blr_word(n) (n % 256), (n / 256)
 
$ SET SANS_DOLLAR
$ SYMBOLS BLR DTYPE "const char %-32s= %d;\n
 
$ SYMBOLS BLR JOIN "const char %-32s= %d;\n
 
$ SYMBOLS BLR MECH "const char %-32s= %d;\n
 
$ SYMBOLS BLR STATEMENTS "const char %-32s= %d;\n
 
$ SYMBOLS BLR VALUES "const char %-32s= %d;\n
 
$ SYMBOLS BLR BOOLEANS "const char %-32s= %d;\n
 
$ SYMBOLS BLR RSE "const char %-32s= %d;\n
 
$ SYMBOLS BLR AGGREGATE "const char %-32s= %d;\n
 
$ SYMBOLS BLR NEW "const char %-32s= %d;\n
 
/* Database parameter block stuff */
 
$ SYMBOLS DPB ITEMS "const char %-32s= %d;\n
 
$ SYMBOLS DPB BITS "const char %-32s= %d;\n
 
/* Bit assignments in RDB$SYSTEM_FLAG */
 
$ SYMBOLS RDB FLAG "const char %-32s= %d;\n
 
/* Transaction parameter blob stuff */
 
$ SYMBOLS TPB ITEMS "const char %-32s= %d;\n
 
/* Blob Parameter Block */
 
$ SYMBOLS BPB ITEMS "const char %-32s= %d;\n
 
/* Blob stream stuff */ 
 
typedef struct bstream {
    int		*bstr_blob;		/* Blob handle */
    char	*bstr_buffer;		/* Address of buffer */
    char	*bstr_ptr;		/* Next character */
    short	bstr_length;		/* Length of buffer */
    short	bstr_cnt;		/* Characters in buffer */
    char	bstr_mode;		/* (mode) ? OUTPUT : INPUT */
} BSTREAM, * FB_BLOB_STREAM;
 
#define getb(p)	(--(p)->bstr_cnt >= 0 ? *(p)->bstr_ptr++ & 0377: BLOB_get (p))
#define putb(x,p) ((x == '\n' || (!(--(p)->bstr_cnt))) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))
#define putbx(x,p) ((!(--(p)->bstr_cnt)) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))
 
FB_BLOB_STREAM	Bopen(), BLOB_open();
 
/* Information call declarations */
 
/* Common, structural codes */
 
$ SYMBOLS INFO MECH "const char %-32s= %d;\n
 
/* Database information items */
 
$ SYMBOLS INFO DB "const char %-32s= %d;\n
 
/* Database Info Return Values */
 
$ SYMBOLS INFO VALUES "const char %-32s= %d;\n
 
/* Request information items */
 
$ SYMBOLS INFO REQUEST "const char %-32s= %d;\n
 
/* Blob information items */
 
$ SYMBOLS INFO BLOB "const char %-32s= %d;\n
 
/* Transaction information items */
 
$ SYMBOLS INFO TRANSACTION "const char %-32s= %d;\n
 
/* Error codes */
 
$ SYMBOLS ERROR MECH "const long %-32s= %d;\n
 
$ ERROR MAJOR "const long %-32s= %d;\n
 
/* Minor codes subject to change */
 
$ ERROR MINOR "const long %-32s= %d;\n
 
/* Dynamic Data Definition Language operators */
 
/* Version number */
 
$ SYMBOLS DYN MECH "const char %-32s= %d;\n
 
/* Operations (may be nested) */
 
$ SYMBOLS DYN OPERATIONS "const char %-32s= %d;\n
 
/* View specific stuff */
 
$ SYMBOLS DYN VIEW "const char %-32s= %d;\n
 
/* Generic attributes */
 
$ SYMBOLS DYN GENERIC "const char %-32s= %d;\n
 
/* Relation specific attributes */
 
$ SYMBOLS DYN RELATION "const char %-32s= %d;\n
 
/* Global field specific attributes */
 
$ SYMBOLS DYN GLOBAL "const char %-32s= %d;\n
 
/* Local field specific attributes */
 
$ SYMBOLS DYN FIELD "const char %-32s= %d;\n
 
/* Index specific attributes */
 
$ SYMBOLS DYN INDEX "const char %-32s= %d;\n
 
/* Trigger specific attributes */
 
$ SYMBOLS DYN TRIGGER "const char %-32s= %d;\n
 
/* Security Class specific attributes */
 
$ SYMBOLS DYN SECURITY "const char %-32s= %d;\n
 
/* Dimension specific information */
 
$ SYMBOLS DYN ARRAY "const char %-32s= %d;\n
 
/* File specific attributes */

$ SYMBOLS DYN FILES "const char %-32s= %d;\n

/* Function specific attributes */

$ SYMBOLS DYN FUNCTIONS "const char %-32s= %d;\n

/* Generator specific attributes */

$ SYMBOLS DYN GENERATOR "const char %-32s= %d;\n

/* Array slice description language (SDL) */
 
$ SYMBOLS SDL SDL "const char %-32s= %d;\n
 
$ SYMBOLS SQL DTYPE "const short %-32s= %d;\n
 
/* Forms Package definitions */
 
/* Map definition block definitions */
 
$ SYMBOLS PYXIS MAP  "const char %-32s= %d;\n
 
/* Field option flags for display options */
 
$ SYMBOLS PYXIS DISPLAY  "const char %-32s= %d;\n
 
/* Field option values following display */
 
$ SYMBOLS PYXIS VALUE  "const char %-32s= %d;\n
 
/* Pseudo key definitions */
 
$ SYMBOLS PYXIS KEY  "const char %-32s= %d;\n
 
/* Menu definition stuff */
 
$ SYMBOLS PYXIS MENU  "const char %-32s= %d;\n');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('VPASCAL', 'gds.vpas',
'(*
 *	PROGRAM:	Preprocessor
 *	MODULE:		sql.pas
 *	DESCRIPTION:	SQL constants, procedures, etc.
 *
 * copyright (c) 1985, 1990 by Interbase Software Corporation
 *)

type
    gds_$short = [word] -32768..32767;
    gds_$handle	= ^integer;
    gds_$string	= array [1..65535] of char;
    gds_$ptr_type = [UNSAFE] ^gds_$string;
    gds_$status_vector	= array [1..20] of integer;
    gds_$quad		= array [1..2] of integer;
    gds_$teb_t		= record
			dbb_ptr	: gds_$ptr_type;
			tpb_len	: integer;
			tpb_ptr	: gds_$ptr_type;
                      end;
    gds_$tm		= record
			    tm_sec	: integer;
			    tm_min	: integer;
			    tm_hour	: integer;
			    tm_mday	: integer;
			    tm_mon	: integer;
			    tm_year	: integer;
			    tm_wday	: integer;
			    tm_yday	: integer;
			    tm_isdst	: integer;
			end;
    sqlvar	= record
		    sqltype		: gds_$short;
		    sqllen		: gds_$short;
		    sqldata		: gds_$ptr_type;
		    sqlind		: gds_$ptr_type;
		    sqlname_length	: gds_$short;
		    sqlname		: array [1..30] of char;
		end;
    sqlda	= record
		    sqldaid		: array [1..8] of char;
		    sqldabc		: integer;
		    sqln		: gds_$short;
		    sqld		: gds_$short;
		    sqlvars		: array [1..100] of sqlvar;
		end;


procedure GDS_$SET_DEBUG (
		debug_val	: [IMMEDIATE] gds_$short
        ); extern;

procedure gds_$ATTACH_DATABASE (
	var	stat		: gds_$status_vector; 
		name_length	: [IMMEDIATE] gds_$short;
		file_name	: gds_$ptr_type; 
	var	db_handle	: gds_$handle;
		dpb_length	: [IMMEDIATE] gds_$short;
		dpb		: gds_$ptr_type
	); extern;

procedure gds_$ATTACH_DATABASE_D (
	var	stat		: gds_$status_vector; 
		file_name	: gds_$ptr_type; 
	var	db_handle	: gds_$handle;
		dpb_length	: [IMMEDIATE] gds_$short;
		dpb		: gds_$ptr_type
	); extern;

procedure gds_$CANCEL_BLOB (
	var 	stat		: gds_$status_vector; 
	var	blob_handle	: gds_$handle
	); extern;

procedure GDS_$CANCEL_EVENTS (
        var     stat            : gds_$status_vector;
        var     db_handle       : gds_$handle;
                id              : gds_$handle
        ); extern;

procedure gds_$CLOSE_BLOB (
	var 	stat		: gds_$status_vector; 
	var	blob_handle	: gds_$handle
	); extern;

procedure GDS_$COMMIT_RETAINING (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle
	); extern;

procedure gds_$COMMIT_TRANSACTION (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle
	); extern;

procedure gds_$COMPILE_REQUEST (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
	var	request_handle	: gds_$handle;
		blr_length	: [IMMEDIATE] gds_$short;
		blr		: gds_$ptr_type
	); extern;

procedure gds_$COMPILE_REQUEST2 (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
	var	request_handle	: gds_$handle;
		blr_length	: [IMMEDIATE] gds_$short;
		blr		: gds_$ptr_type
	); extern;

procedure gds_$CREATE_BLOB (
	var 	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		tra_handle	: gds_$handle;
	var	blob_handle	: gds_$handle;
	var	blob_id		: gds_$quad
	); extern;

procedure GDS_$CREATE_BLOB2 (
        var     stat            : gds_$status_vector;
                db_handle       : gds_$handle;
                tra_handle      : gds_$handle;
        var     blob_handle     : gds_$handle;
        var     blob_id         : gds_$quad;
                bpb_length      : integer;
                bpb             : gds_$ptr_type
        ); extern;

procedure GDS_$CREATE_DATABASE (
	var 	stat		: gds_$status_vector; 
		name_length	: [IMMEDIATE] gds_$short;
		file_name	: gds_$ptr_type;
	var	db_handle	: gds_$handle;
		dpb_length	: [IMMEDIATE] gds_$short;
		dpb		: gds_$ptr_type;
		dpb_type	: [IMMEDIATE] gds_$short
	); extern;

procedure GDS_$DDL (
	var 	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		tra_handle	: gds_$handle;
		mblr_length	: [IMMEDIATE] gds_$short;
		mblr		: gds_$ptr_type
	); extern;


procedure gds_$DETACH_DATABASE (
	var	stat		: gds_$status_vector; 
	var	db_handle	: gds_$handle
	); extern;

procedure gds_$DROP_DATABASE (
	var 	stat		: gds_$status_vector; 
		name_length	: [IMMEDIATE] gds_$short;
		file_name	: gds_$ptr_type; 
		db_type		: [IMMEDIATE] gds_$short
	); extern;

procedure GDS_$EVENT_WAIT (
        var     stat            : gds_$status_vector;
                db_handle       : gds_$handle;
                length          : [IMMEDIATE] gds_$short;
        var     events          : gds_$ptr_type;
        var     buffer          : gds_$ptr_type
        ); extern;

procedure GDS_$GET_SLICE (
	var 	stat		: gds_$status_vector; 
                db_handle       : gds_$handle;
                tra_handle      : gds_$handle;
		blob_id         : gds_$quad;
                sdl_length      : [IMMEDIATE] gds_$short;
                sdl             : gds_$ptr_type;
                param_length    : [IMMEDIATE] gds_$short;
                param           : gds_$ptr_type;
                slice_length    : integer;
                slice           : gds_$ptr_type;
        var     return_length   : integer
	); extern;

function gds_$GET_SEGMENT (
	var 	stat		: gds_$status_vector; 
		blob_handle	: gds_$handle;
	var	length		: gds_$short;
		buffer_length	: [IMMEDIATE] gds_$short;
	var	buffer		: gds_$ptr_type
	): integer; extern;

procedure gds_$OPEN_BLOB (
	var 	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		tra_handle	: gds_$handle;
	var	blob_handle	: gds_$handle;
		blob_id		: gds_$quad
	); extern;

procedure GDS_$OPEN_BLOB2 (
	var 	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		tra_handle	: gds_$handle;
	var	blob_handle	: gds_$handle;
		blob_id		: gds_$quad;
                bpb_length      : integer;
                bpb             : gds_$ptr_type
	); extern;

procedure gds_$PREPARE_TRANSACTION (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle
	); extern;

procedure gds_$STATUS_AND_STOP (
	var	stat		: gds_$status_vector
	); extern;

function gds_$PUT_SEGMENT (
	var 	stat		: gds_$status_vector; 
		blob_handle	: gds_$handle;
		length		: [IMMEDIATE] gds_$short;
		buffer		: gds_$ptr_type
	): integer; extern;

procedure GDS_$PUT_SLICE (
	var 	stat		: gds_$status_vector; 
                db_handle       : gds_$handle;
                tra_handle      : gds_$handle;
		blob_id         : gds_$quad;
                sdl_length      : [IMMEDIATE] gds_$short;
                sdl             : gds_$ptr_type;
                param_length    : [IMMEDIATE] gds_$short;
                param           : gds_$ptr_type;
                slice_length    : integer;
                slice           : gds_$ptr_type
	); extern;

procedure GDS_$QUE_EVENTS (
        var     stat            : gds_$status_vector;
                db_handle       : gds_$handle;
                id              : gds_$quad;
                length          : [IMMEDIATE] gds_$short;
                events          : gds_$ptr_type;
                ast             : gds_$handle;
                arg             : gds_$quad
        ); extern;

procedure gds_$RECEIVE (
	var	stat		: gds_$status_vector; 
		request_handle	: gds_$handle;
		message_type	: [IMMEDIATE] gds_$short;
		message_length	: [IMMEDIATE] gds_$short;
	var	message		: gds_$ptr_type;
		instantiation	: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$RELEASE_REQUEST (
	var	stat		: gds_$status_vector; 
	var	request_handle	: gds_$handle
	); extern;

procedure gds_$ROLLBACK_TRANSACTION (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle
	); extern;

procedure gds_$SEND (
	var	stat		: gds_$status_vector; 
		request_handle	: gds_$handle;
		message_type	: [IMMEDIATE] gds_$short;
		message_length	: [IMMEDIATE] gds_$short;
		message		: gds_$ptr_type;
		instantiation	: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$START_AND_SEND (
	var	stat		: gds_$status_vector; 
		request_handle	: gds_$handle;
		tra_handle	: gds_$handle;
		message_type	: [IMMEDIATE] gds_$short;
		message_length	: [IMMEDIATE] gds_$short;
		message		: gds_$ptr_type;
		instantiation	: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$START_REQUEST (
	var	stat		: gds_$status_vector; 
		request_handle	: gds_$handle;
		tra_handle	: gds_$handle;
		instantiation	: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$START_MULTIPLE (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle;
		tra_count	: [IMMEDIATE] gds_$short;
		teb		: gds_$ptr_type
	); extern;

procedure gds_$START_TRANSACTION (
	var	stat		: gds_$status_vector; 
	var	tra_handle	: gds_$handle;
		tra_count	: [IMMEDIATE] gds_$short;
		db_handle	: gds_$handle;
		tpb_length	: [IMMEDIATE] gds_$short;
		tpb		: gds_$ptr_type
	); extern;

procedure gds_$UNWIND_REQUEST (
	var	stat		: gds_$status_vector; 
		request_handle	: gds_$handle;
		instantiation	: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$ftof (
		string1		: [REFERENCE] gds_$ptr_type;
                length1		: [IMMEDIATE] gds_$short;
		string2		: [REFERENCE] gds_$ptr_type;
                length2		: [IMMEDIATE] gds_$short
	); extern;

procedure gds_$print_status (
		stat		: gds_$status_vector
	); extern;

function gds_$sqlcode (
		stat		: gds_$status_vector
	) : integer; extern;


procedure gds_$encode_date (
		times		: gds_$tm;
	var	date 		: gds_$quad
    ); extern;

procedure gds_$decode_date (
		date 		: gds_$quad;
	var	times		: gds_$tm 
    ); extern;

const
    gds_$true	= 1;
    gds_$false  = 0;

$ SYMBOLS BLR DTYPE BYTE "\t%-32s= chr(%d);\n
$ SYMBOLS BLR MECH	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR STATEMENTS"\t%-32s= chr(%d);\n
$ SYMBOLS BLR VALUES	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR BOOLEANS	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR RSE	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR JOIN	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR AGGREGATE "\t%-32s= chr(%d);\n
$ SYMBOLS BLR NEW	"\t%-32s= chr(%d);\n
 
(* Database parameter block stuff *)

$ SYMBOLS DPB ITEMS	"\t%-32s= chr(%d);\n
 
$ SYMBOLS DPB BITS	"\t%-32s= chr(%d);\n

(* Transaction parameter block stuff *)

$ SYMBOLS TPB ITEMS	"\t%-32s= chr(%d);\n

(* Blob parameter block stuff *)

$ SYMBOLS BPB ITEMS	"\t%-32s= chr(%d);\n

(* Blob routine declarations *)

type
    gds_$field_name	= array [1..31] of char;
    gds_$file_name	= array [1..128] of char;

procedure blob_$display (
		blob_id		: gds_$quad;
	 	db_handle	: gds_$handle;
	 	tra_handle	: gds_$handle;
        	field_name	: gds_$ptr_type;
                name_length	: [IMMEDIATE] integer
	); extern;

procedure blob_$dump (
		blob_id		: gds_$quad;
	 	db_handle	: gds_$handle;
	 	tra_handle	: gds_$handle;
        	file_name	: gds_$ptr_type;
        	name_length	: [IMMEDIATE] integer
	); extern;

procedure blob_$edit (
		blob_id		: gds_$quad;
	 	db_handle	: gds_$handle;
	 	tra_handle	: gds_$handle;
        	field_name	: gds_$ptr_type;
        	name_length	: [IMMEDIATE] integer
	); extern;

procedure blob_$load (
		blob_id		: gds_$quad;
	 	db_handle	: gds_$handle;
	 	tra_handle	: gds_$handle;
        	file_name	: gds_$ptr_type;
        	name_length	: [IMMEDIATE] integer
	); extern;

(* Dynamic SQL procedures *)

procedure gds_$close (
	var	stat		: gds_$status_vector; 
		cursor_name	: gds_$ptr_type
	); extern;

procedure gds_$declare (
	var	stat		: gds_$status_vector; 
		statement_name	: gds_$ptr_type;
		cursor_name	: gds_$ptr_type
	); extern;

procedure gds_$describe (
	var	stat		: gds_$status_vector; 
		statement_name	: gds_$ptr_type;
	var	descriptor	: gds_$ptr_type
	); extern;

procedure gds_$dsql_finish (
		db_handle	: gds_$handle
	); extern;

procedure gds_$execute (
	var	stat		: gds_$status_vector; 
		trans_handle	: gds_$handle;
		statement_name	: gds_$ptr_type;
	var	descriptor	: gds_$ptr_type
	); extern;

procedure gds_$execute_immediate (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
		string_length	: gds_$short;
		string		: gds_$ptr_type
	); extern;

function gds_$fetch (
	var	stat		: gds_$status_vector; 
		cursor_name	: gds_$ptr_type;
	var	descriptor	: gds_$ptr_type
	) : integer; extern;

procedure gds_$open (
	var	stat		: gds_$status_vector; 
		trans_handle	: gds_$handle;
		cursor_name	: gds_$ptr_type;
	var	descriptor	: gds_$ptr_type
	); extern;

procedure gds_$prepare (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
		statement_name	: gds_$ptr_type;
		string_length	: gds_$short;
		string		: gds_$ptr_type; 
	var	descriptor	: gds_$ptr_type
	); extern;


(* Information parameters *)

(* Common, structural codes *)
const
$ SYMBOLS INFO MECH	"\t%-32s= chr(%d);\n

(* Database information items *)

$ SYMBOLS INFO DB	"\t%-32s= chr(%d);\n

(* Request information items *)

$ SYMBOLS INFO REQUEST	"\t%-32s= chr(%d);\n

(* Blob information items *)

$ SYMBOLS INFO BLOB	"\t%-32s= chr(%d);\n

(* Transaction information items *)

$ SYMBOLS INFO TRANSACTION	"\t%-32s= chr(%d);\n

(* Error codes *)

CONST
$ SYMBOLS ERROR MECH	"\t%-26s= %d;\n
 
$ ERROR MAJOR		"\t%-26s= %d;\n

(* Minor codes subject to change *)

$ ERROR MINOR		"\t%-26s= %d;\n

(* Dynamic Data Definition Language operators *)

(* Version number *)

$ SYMBOLS DYN MECH "\t%-32s= chr(%d);\n

(* Operations (may be nested) *)

$ SYMBOLS DYN OPERATIONS "\t%-32s= chr(%d);\n

(* View specific stuff *)

$ SYMBOLS DYN VIEW "\t%-32s= chr(%d);\n

(* Generic attributes *)

$ SYMBOLS DYN GENERIC "\t%-32s= chr(%d);\n

(* Relation specific attributes *)

$ SYMBOLS DYN RELATION "\t%-32s= chr(%d);\n

(* Global field specific attributes *)

$ SYMBOLS DYN GLOBAL "\t%-32s= chr(%d);\n

(* Local field specific attributes *)

$ SYMBOLS DYN FIELD "\t%-32s= chr(%d);\n

(* Index specific attributes *)

$ SYMBOLS DYN INDEX "\t%-32s= chr(%d);\n

(* Trigger specific attributes *)

$ SYMBOLS DYN TRIGGER "\t%-32s= chr(%d);\n

(* Security Class specific attributes *)

$ SYMBOLS DYN SECURITY "\t%-32s= chr(%d);\n

(* File specific attributes *)

$ SYMBOLS DYN FILES "\t%-32s= chr(%d);\n

(* Function specific attributes *)

$ SYMBOLS DYN FUNCTIONS "\t%-32s= chr(%d);\n

(* Generator specific attributes *)

$ SYMBOLS DYN GENERATOR "\t%-32s= chr(%d);\n

(* Array slice description language (SDL) *)

$ SYMBOLS SDL SDL BYTE "\t%-32s= chr(%d);\n

(* Dynamic SQL datatypes *)

CONST
$ SYMBOLS SQL DTYPE "\t%-32s= %d;\n

(* Forms Package definitions *)

(* Map definition block definitions *)

CONST
$ SYMBOLS PYXIS MAP BYTE  "\t%-32s= chr(%d);\n

(* Field option flags for display options *)

CONST
$ SYMBOLS PYXIS DISPLAY  "\t%-32s= %d;\n

(* Field option values following display *)

CONST
$ SYMBOLS PYXIS VALUE  "\t%-32s= %d;\n

(* Pseudo key definitions *)

CONST
$ SYMBOLS PYXIS KEY  "\t%-32s= %d;\n

(* Menu definition stuff *)

CONST
$ SYMBOLS PYXIS MENU BYTE "\t%-32s= chr(%d);\n

procedure pyxis_$compile_map (
	var	stat		: gds_$status_vector; 
	 	form_handle	: gds_$handle;
	var 	map_handle	: gds_$handle;
		length		: gds_$short;
		map		: gds_$ptr_type
	); extern;

procedure pyxis_$compile_sub_map (
	var	stat		: gds_$status_vector; 
	 	form_handle	: gds_$handle;
	var 	map_handle	: gds_$handle;
		length		: gds_$short;
		map		: gds_$ptr_type
	); extern;

procedure pyxis_$create_window (
	var	window_handle	: gds_$handle;
		name_length	: gds_$short;
		file_name	: gds_$ptr_type;
	var	width		: gds_$short;
	var	height		: gds_$short
	); extern;

procedure pyxis_$delete_window (
	var	window_handle	: gds_$handle
	); extern;

procedure pyxis_$drive_form (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
	var	window_handle	: gds_$handle;
	 	map_handle	: gds_$handle;
		input		: gds_$ptr_type;
		output		: gds_$ptr_type
	); extern;

procedure pyxis_$fetch (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
	 	map_handle	: gds_$handle;
		output		: gds_$ptr_type
	); extern;

procedure pyxis_$insert (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
	 	map_handle	: gds_$handle;
		input		: gds_$ptr_type
	); extern;

procedure pyxis_$load_form (
	var	stat		: gds_$status_vector; 
		db_handle	: gds_$handle;
		trans_handle	: gds_$handle;
	var	form_handle	: gds_$handle;
		name_length	: gds_$short;
		form_name	: gds_$ptr_type
	); extern;

function pyxis_$menu (
	var	window_handle	: gds_$handle;
	var	menu_handle	: gds_$handle;
		length		: gds_$short;
		menu		: gds_$ptr_type
	) : gds_$short; extern;

procedure pyxis_$pop_window (
	 	window_handle	: gds_$handle
	); extern;

procedure pyxis_$reset_form (
	var	stat		: gds_$status_vector; 
		window_handle	: gds_$handle
	); extern;

procedure pyxis_$drive_menu (
	var	window_handle	: gds_$handle;
	var	menu_handle	: gds_$handle;
		blr_length	: gds_$short;
		blr_source	: gds_$ptr_type;
		title_length	: gds_$short;
		title		: gds_$ptr_type;
	var	terminator	: gds_$short;
	var	entree_length	: gds_$short;
		entree_text	: gds_$ptr_type;
	var	entree_value	: integer
	); extern;


procedure pyxis_$get_entree (
		menu_handle	: gds_$handle;
	var	entree_length	: gds_$short;
		entree_text	: gds_$ptr_type;
	var	entree_value	: integer;
	var	entree_end	: gds_$short
	); extern;

procedure pyxis_$initialize_menu (
	var	menu_handle	: gds_$handle
	); extern;

procedure pyxis_$put_entree (
		menu_handle	: gds_$handle;
		entree_length	: gds_$short;
		entree_text	: gds_$ptr_type;
		entree_value	: integer
	); extern;


[HIDDEN] PROCEDURE LIB$SCOPY_DXDX; EXTERNAL;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('PASCAL', 'gds.pas',
'(*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gds.pas
 *	DESCRIPTION:	GDS constants, procedures, etc.
 *
 * copyright (c) 1985, 1990 by Interbase Software Corporation
 *)

type
    gds_$handle	= ^integer32;
    gds_$string	= array [1..65535] of char;
    gds_$status_vector	= array [1..20] of integer32;
    gds_$quad		= array [1..2] of integer32;
    gds_$teb_t	= record
		    dbb_ptr	: ^gds_$handle;
		    tpb_len	: integer32;
		    tpb_ptr	: UNIV_PTR;                       
                 end;
    gds_$tm	= record
		    tm_sec	: integer32;
		    tm_min	: integer32;
		    tm_hour	: integer32;
		    tm_mday	: integer32;
		    tm_mon	: integer32;
		    tm_year	: integer32;
		    tm_wday	: integer32;
		    tm_yday	: integer32;
		    tm_isdst	: integer32;
		end;
    sqlvar	= record                    
		    sqltype		: integer16;
		    sqllen		: integer16;
		    sqldata		: UNIV_PTR;
		    sqlind		: UNIV_PTR;
		    sqlname_length	: integer16;
		    sqlname		: array [1..30] of char;
		end;
    sqlda	= record
		    sqldaid		: array [1..8] of char;
		    sqldabc		: integer32;
		    sqln		: integer16;
		    sqld		: integer16;
		    sqlvars		: array [1..100] of sqlvar;
		end;

procedure GDS_$SET_DEBUG (
        in	debug_val	: integer32
        ); extern;

procedure GDS_$ATTACH_DATABASE (
	out	stat		: gds_$status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds_$string; 
	in out	db_handle	: gds_$handle;
	in	dpb_length	: integer16;
	in	dpb		: UNIV gds_$string
	); extern;

procedure GDS_$CANCEL_BLOB (
	out 	stat		: gds_$status_vector; 
	in out	blob_handle	: gds_$handle
	); extern;

procedure GDS_$CANCEL_EVENTS (
        out     stat            : gds_$status_vector;
        in out  db_handle       : gds_$handle;
        in      id              : gds_$handle
        ); extern;

procedure GDS_$CLOSE_BLOB (
	out 	stat		: gds_$status_vector; 
	in out	blob_handle	: gds_$handle
	); extern;

procedure GDS_$COMMIT_RETAINING (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle
	); extern;

procedure GDS_$COMMIT_TRANSACTION (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle
	); extern;

procedure GDS_$COMPILE_REQUEST (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in out	request_handle	: gds_$handle;
	in	blr_length	: integer16;
	in	blr		: UNIV gds_$string
	); extern;

procedure GDS_$COMPILE_REQUEST2 (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in out	request_handle	: gds_$handle;
	in	blr_length	: integer16;
	in	blr		: UNIV gds_$string
	); extern;

procedure GDS_$CREATE_BLOB (
	out 	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in out	blob_handle	: gds_$handle;
	out	blob_id		: gds_$quad
	); extern;

procedure GDS_$CREATE_BLOB2 (
        out     stat            : gds_$status_vector;
        in      db_handle       : gds_$handle;
        in      tra_handle      : gds_$handle;
        in out  blob_handle     : gds_$handle;
        out     blob_id         : gds_$quad;
        in      bpb_length      : integer16;
        in      bpb             : UNIV gds_$string
        ); extern;

procedure GDS_$CREATE_DATABASE (
	out 	stat		: gds_$status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds_$string; 
	in out	db_handle	: gds_$handle;
	in	dpb_length	: integer16;
	in	dpb		: UNIV gds_$string;
	in	dpb_type	: integer16
	); extern;

procedure GDS_$DDL (
	out 	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in	mblr_length	: integer16;
	in	mblr		: UNIV gds_$string
	); extern;

procedure GDS_$DETACH_DATABASE (
	out	stat		: gds_$status_vector; 
	in out	db_handle	: gds_$handle
	); extern;

procedure GDS_$DROP_DATABASE (
	out 	stat		: gds_$status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds_$string; 
	in	dbtype		: integer16
	); extern;

procedure GDS_$EVENT_WAIT (
        out     stat            : gds_$status_vector;
        in      db_handle       : gds_$handle;
        in      length          : integer16;
        in out  events          : UNIV gds_$string;
        in out  buffer          : UNIV gds_$string
        ); extern;

procedure GDS_$GET_SLICE (
	out 	stat		: gds_$status_vector; 
        in      db_handle       : gds_$handle;
        in      tra_handle      : gds_$handle;
	in	blob_id         : gds_$quad;
        in      sdl_length      : integer16;
        in      sdl             : UNIV gds_$string;
        in      param_length    : integer16;
        in      param           : UNIV gds_$string;
        in      slice_length    : integer32;
        in      slice           : UNIV gds_$string;
        in out  return_length   : integer32
	); extern;

function GDS_$GET_SEGMENT (
	out 	stat		: gds_$status_vector; 
	in	blob_handle	: gds_$handle;
	out	length		: integer16;
	in	buffer_length	: integer16;
	out	buffer		: UNIV gds_$string
	): integer32; extern;

procedure GDS_$OPEN_BLOB (
	out 	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in out	blob_handle	: gds_$handle;
	in	blob_id		: gds_$quad
	); extern;

procedure GDS_$OPEN_BLOB2 (
	out 	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in out	blob_handle	: gds_$handle;
	in	blob_id		: gds_$quad;
        in      bpb_length      : integer16;
        in      bpb             : UNIV gds_$string
	); extern;

procedure GDS_$PREPARE_TRANSACTION (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle
	); extern;

procedure GDS_$PREPARE_TRANSACTION2 (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle;
        in      msg_length      : integer16;
        in      msg             : UNIV gds_$string
	); extern;

function GDS_$PUT_SEGMENT (
	out 	stat		: gds_$status_vector; 
	in	blob_handle	: gds_$handle;
	in	length		: integer16;
	out	buffer		: UNIV gds_$string
	): integer32; extern;

procedure GDS_$PUT_SLICE (
	out 	stat		: gds_$status_vector; 
        in      db_handle       : gds_$handle;
        in      tra_handle      : gds_$handle;
	in	blob_id         : gds_$quad;
        in      sdl_length      : integer16;
        in      sdl             : UNIV gds_$string;
        in      param_length    : integer16;
        in      param           : UNIV gds_$string;
        in      slice_length    : integer32;
        in      slice           : UNIV gds_$string
	); extern;

procedure GDS_$QUE_EVENTS (
        out     stat            : gds_$status_vector;
        in      db_handle       : gds_$handle;
        in      id              : gds_$quad;
        in      length          : integer16;
        in      events          : UNIV gds_$string;
        in      ast             : gds_$handle;
        in      arg             : gds_$quad
        ); extern;

procedure GDS_$RECEIVE (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in out	message		: UNIV gds_$string;
	in	instantiation	: integer16
	); extern;

procedure GDS_$RELEASE_REQUEST (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle
	); extern;

procedure GDS_$ROLLBACK_TRANSACTION (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle
	); extern;

procedure GDS_$SEND (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in	message		: UNIV gds_$string;
	in	instantiation	: integer16
	); extern;

procedure GDS_$START_AND_SEND (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in	message		: UNIV gds_$string;
	in	instantiation	: integer16
	); extern;

procedure GDS_$START_REQUEST (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle;
	in	tra_handle	: gds_$handle;
	in	instantiation	: integer16
	); extern;

procedure GDS_$START_MULTIPLE (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle;
	in	tra_count	: integer16;
	in	teb		: UNIV gds_$string
	); extern;

procedure GDS_$START_TRANSACTION (
	out	stat		: gds_$status_vector; 
	in out	tra_handle	: gds_$handle;
	in	tra_count	: integer16;
	in	db_handle	: gds_$handle;
	in	tpb_length	: integer16;
	in	tpb		: UNIV gds_$string
	); extern;

procedure GDS_$UNWIND_REQUEST (
	out	stat		: gds_$status_vector; 
	in	request_handle	: gds_$handle;
	in	instantiation	: integer16
	); extern;

procedure gds_$ftof (
	in	string1		: UNIV gds_$string;
	in	length1		: integer16;
	in	string2		: UNIV gds_$string;
	in	length2		: integer16
	); extern;

procedure gds_$print_status (
	in	stat		: gds_$status_vector
	); extern;

function gds_$sqlcode (
	in	stat		: gds_$status_vector
	) : integer32; extern;


procedure gds_$encode_date (
	in	times		: gds_$tm;
	out	date 		: gds_$quad
    ); extern;

procedure gds_$decode_date (
	in	date 		: gds_$quad;
	out	times		: gds_$tm 
    ); extern;

const
    gds_$true	= 1;
    gds_$false  = 0;

$ SYMBOLS BLR DTYPE BYTE	"\t%-32s= chr(%d);\n
$ SYMBOLS BLR MECH	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR STATEMENTS"\t%-32s= chr(%d);\n
$ SYMBOLS BLR VALUES	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR BOOLEANS	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR RSE	"\t%-32s= chr(%d);\n

$ SYMBOLS BLR JOIN	"\t%-32s= chr(%d);\n
 
$ SYMBOLS BLR AGGREGATE "\t%-32s= chr(%d);\n
$ SYMBOLS BLR NEW	"\t%-32s= chr(%d);\n

(* Database parameter block stuff *)

$ SYMBOLS DPB ITEMS	"\t%-32s= chr(%d);\n
 
$ SYMBOLS DPB BITS	"\t%-32s= chr(%d);\n


(* Transaction parameter block stuff *)

$ SYMBOLS TPB ITEMS	"\t%-32s= chr(%d);\n

(* Blob parameter block stuff *)

$ SYMBOLS BPB ITEMS	"\t%-32s= chr(%d);\n

(* Blob routine declarations *)

type
    gds_$field_name	= array [1..31] of char;
    gds_$file_name	= array [1..128] of char;

procedure blob_$display (
	in	blob_id		: gds_$quad;
	in 	db_handle	: gds_$handle;
	in 	tra_handle	: gds_$handle;
        in	field_name	: univ gds_$field_name;
        in      name_length	: integer16
	); extern;

procedure blob_$dump (
	in	blob_id		: gds_$quad;
	in 	db_handle	: gds_$handle;
	in 	tra_handle	: gds_$handle;
        in	file_name	: univ gds_$file_name;
        in      name_length	: integer16
	); extern;

procedure blob_$edit (
	in	blob_id		: gds_$quad;
	in 	db_handle	: gds_$handle;
	in 	tra_handle	: gds_$handle;
        in	field_name	: univ gds_$field_name;
        in      name_length	: integer16
	); extern;

procedure blob_$load (
	in	blob_id		: gds_$quad;
	in 	db_handle	: gds_$handle;
	in 	tra_handle	: gds_$handle;
        in	file_name	: univ gds_$file_name;
        in      name_length	: integer16
	); extern;

(* Dynamic SQL procedures *)

procedure gds_$close (
	out	stat		: gds_$status_vector; 
	in	cursor_name	: UNIV gds_$string
	); extern;

procedure gds_$declare (
	out	stat		: gds_$status_vector; 
	in	statement_name	: UNIV gds_$string;
	in	cursor_name	: UNIV gds_$string
	); extern;

procedure gds_$describe (
	out	stat		: gds_$status_vector; 
	in	statement_name	: UNIV gds_$string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds_$dsql_finish (
	in	db_handle	: gds_$handle
	); extern;

procedure gds_$execute (
	out	stat		: gds_$status_vector; 
	in	trans_handle	: gds_$handle;
	in	statement_name	: UNIV gds_$string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds_$execute_immediate (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	trans_handle	: gds_$handle;
	in	string_length	: integer;
	in	string		: UNIV gds_$string
	); extern;

function gds_$fetch (
	out	stat		: gds_$status_vector; 
	in	cursor_name	: UNIV gds_$string;
	in out	descriptor	: UNIV sqlda
	) : integer32; extern;

procedure gds_$open (
	out	stat		: gds_$status_vector; 
	in	trans_handle	: gds_$handle;
	in	cursor_name	: UNIV gds_$string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds_$prepare (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	trans_handle	: gds_$handle;
	in	statement_name	: UNIV gds_$string;
	in	string_length	: integer;
	in	string		: UNIV gds_$string;
	in out	descriptor	: UNIV sqlda
	); extern;


(* Information parameters *)

(* Common, structural codes *)
const
$ SYMBOLS INFO MECH	"\t%-32s= chr(%d);\n

(* Database information items *)

$ SYMBOLS INFO DB	"\t%-32s= chr(%d);\n

(* Request information items *)

$ SYMBOLS INFO REQUEST	"\t%-32s= chr(%d);\n

(* Blob information items *)

$ SYMBOLS INFO BLOB	"\t%-32s= chr(%d);\n

(* Transaction information items *)

$ SYMBOLS INFO TRANSACTION	"\t%-32s= chr(%d);\n

(* Error codes *)

CONST
$ SYMBOLS ERROR MECH	"\t%-26s= %d;\n

$ ERROR MAJOR		"\t%-26s= %d;\n

(* Minor codes subject to change *)

$ ERROR MINOR		"\t%-26s= %d;\n

(* Dynamic Data Definition Language operators *)

(* Version number *)

$ SYMBOLS DYN MECH "\t%-32s= chr(%d);\n

(* Operations (may be nested) *)

$ SYMBOLS DYN OPERATIONS "\t%-32s= chr(%d);\n

(* View specific stuff *)

$ SYMBOLS DYN VIEW "\t%-32s= chr(%d);\n

(* Generic attributes *)

$ SYMBOLS DYN GENERIC "\t%-32s= chr(%d);\n

(* Relation specific attributes *)

$ SYMBOLS DYN RELATION "\t%-32s= chr(%d);\n

(* Global field specific attributes *)

$ SYMBOLS DYN GLOBAL "\t%-32s= chr(%d);\n

(* Local field specific attributes *)

$ SYMBOLS DYN FIELD "\t%-32s= chr(%d);\n

(* Index specific attributes *)

$ SYMBOLS DYN INDEX "\t%-32s= chr(%d);\n

(* Trigger specific attributes *)

$ SYMBOLS DYN TRIGGER "\t%-32s= chr(%d);\n

(* Security Class specific attributes *)

$ SYMBOLS DYN SECURITY "\t%-32s= chr(%d);\n

(* dimension specific stuff *)

$ SYMBOLS DYN ARRAY "\t%-32s= chr(%d);\n

(* File specific attributes *)

$ SYMBOLS DYN FILES "\t%-32s= chr(%d);\n

(* Function specific attributes *)

$ SYMBOLS DYN FUNCTIONS "\t%-32s= chr(%d);\n

(* Generator specific attributes *)

$ SYMBOLS DYN GENERATOR "\t%-32s= chr(%d);\n

(* Array slice description language (SDL) *)

$ SYMBOLS SDL SDL BYTE	"\t%-32s= chr(%d);\n

(* Dynamic SQL datatypes *)

CONST
$ SYMBOLS SQL DTYPE "\t%-32s= %d;\n

(* Forms Package definitions *)

(* Map definition block definitions *)

CONST
$ SYMBOLS PYXIS MAP BYTE  "\t%-32s= chr(%d);\n

(* Field option flags for display options *)

CONST
$ SYMBOLS PYXIS DISPLAY  "\t%-32s= %d;\n

(* Field option values following display *)

CONST
$ SYMBOLS PYXIS VALUE  "\t%-32s= %d;\n

(* Pseudo key definitions *)

CONST
$ SYMBOLS PYXIS KEY  "\t%-32s= %d;\n

(* Menu definition stuff *)

CONST
$ SYMBOLS PYXIS MENU BYTE  "\t%-32s= chr(%d);\n

procedure pyxis_$compile_map (
	out	stat		: gds_$status_vector; 
	in 	form_handle	: gds_$handle;
	out 	map_handle	: gds_$handle;
	in	length		: integer16;
	in	map		: UNIV gds_$string
	); extern;

procedure pyxis_$compile_sub_map (
	out	stat		: gds_$status_vector; 
	in 	form_handle	: gds_$handle;
	out 	map_handle	: gds_$handle;
	in	length		: integer16;
	in	map		: UNIV gds_$string
	); extern;

procedure pyxis_$create_window (
	in out	window_handle	: gds_$handle;
	in	name_length	: integer16;
	in	file_name	: UNIV gds_$string; 
	in out	width		: integer16;
	in out	height		: integer16
	); extern;

procedure pyxis_$delete_window (
	in out	window_handle	: gds_$handle
	); extern;

procedure pyxis_$drive_form (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	trans_handle	: gds_$handle;
	in out	window_handle	: gds_$handle;
	in 	map_handle	: gds_$handle;
	in	input		: UNIV gds_$string;
	in	output		: UNIV gds_$string
	); extern;

procedure pyxis_$fetch (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	trans_handle	: gds_$handle;
	in 	map_handle	: gds_$handle;
	in	output		: UNIV gds_$string
	); extern;

procedure pyxis_$insert (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in	trans_handle	: gds_$handle;
	in 	map_handle	: gds_$handle;
	in	input		: UNIV gds_$string
	); extern;

procedure pyxis_$load_form (
	out	stat		: gds_$status_vector; 
	in	db_handle	: gds_$handle;
	in 	trans_handle	: gds_$handle;
	in out	form_handle	: gds_$handle;
	in	name_length	: integer16;
	in	form_name	: UNIV gds_$string 
	); extern;

function pyxis_$menu (
	in out	window_handle	: gds_$handle;
	in out	menu_handle	: gds_$handle;
	in	length		: integer16;
	in	menu		: UNIV gds_$string
	) : integer16; extern;

procedure pyxis_$pop_window (
	in 	window_handle	: gds_$handle
	); extern;

procedure pyxis_$reset_form (
	out	stat		: gds_$status_vector; 
	in	window_handle	: gds_$handle
	); extern;

procedure pyxis_$drive_menu (
	in out	window_handle	: gds_$handle;
	in out	menu_handle	: gds_$handle;
	in	blr_length	: integer16;
	in	blr_source	: UNIV gds_$string;
	in	title_length	: integer16;
	in	title		: UNIV gds_$string;
	out	terminator	: integer16;
	out	entree_length	: integer16;
	out	entree_text	: UNIV gds_$string;
	out	entree_value	: integer32
	); extern;

procedure pyxis_$get_entree (
	in	menu_handle	: gds_$handle;
	out	entree_length	: integer16;
	out	entree_text	: UNIV gds_$string;
	out	entree_value	: integer32;
	out	entree_end	: integer16
	); extern;

procedure pyxis_$initialize_menu (
	in out	menu_handle	: gds_$handle
	); extern;

procedure pyxis_$put_entree (
	in	menu_handle	: gds_$handle;
	in	entree_length	: integer16;
	in	entree_text	: UNIV gds_$string;
	in	entree_value	: integer32
	); extern;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('APOLLO_ADA', 'interbase.a',
'WITH SYSTEM;

PACKAGE interbase IS

TYPE status_vector IS ARRAY (0..19) OF INTEGER;
TYPE quad IS ARRAY (0..1) OF INTEGER;
TYPE chars is array (integer range <>) of character;
TYPE blr IS ARRAY (integer range <>) of tiny_integer;
TYPE dpb IS ARRAY (integer range <>) of tiny_integer;
TYPE tpb IS ARRAY (integer range <>) of tiny_integer;

SUBTYPE isc_short		IS short_integer;
SUBTYPE isc_long		IS integer;
SUBTYPE isc_float		IS short_float;
SUBTYPE isc_double		IS float;
SUBTYPE database_handle		IS integer;
SUBTYPE request_handle		IS integer;
SUBTYPE transaction_handle	IS integer;
SUBTYPE blob_handle		IS integer;
SUBTYPE form_handle		IS integer;
SUBTYPE map_handle		IS integer;
SUBTYPE window_handle		IS integer;
SUBTYPE menu_handle		IS integer;

TYPE gds_teb_t IS RECORD
    dbb_ptr	: SYSTEM.ADDRESS;
    tpb_len	: integer;
    tpb_ptr	: SYSTEM.ADDRESS;                       
END RECORD;

TYPE gds_tm IS RECORD
    tm_sec	: integer;
    tm_min	: integer;
    tm_hour	: integer;
    tm_mday	: integer;
    tm_mon	: integer;
    tm_year	: integer;
    tm_wday	: integer;
    tm_yday	: integer;
    tm_isdst	: integer;
END RECORD;

TYPE sqlvar is RECORD
	sqltype		: short_integer;
	sqllen		: short_integer;
	sqldata		: SYSTEM.ADDRESS;
	sqlind		: SYSTEM.ADDRESS;
	sqlname_length	: short_integer;
	sqlname		: chars (1..30);
END RECORD;

FOR sqlvar use record at mod 2;
	sqltype		at 0 range 0..15;
	sqllen		at 2 range 0..15;
	sqldata		at 4 range 0..31;
	sqlind		at 8 range 0..31;
	sqlname_length	at 12 range 0..15;
	sqlname		at 14 range 0..239;
END RECORD;

TYPE sqlvar_array IS ARRAY (short_integer range <>) of sqlvar;

--- Constants

    gds_true	: CONSTANT integer := 1;
    gds_false  : CONSTANT integer := 0;

--- BLR

$ SET SANS_DOLLAR

$ FORMAT BLR_FORMAT	"    %s : CONSTANT tiny_integer := %d;\n
$ FORMAT DYN_FORMAT	"    %-32s: CONSTANT tiny_integer := %d;\n
$ FORMAT SDL_FORMAT	"    %-32s: CONSTANT tiny_integer := %d;\n
$ FORMAT ERROR_FORMAT	"%-25s: CONSTANT integer := %d;\n
$ FORMAT PB_FORMAT	"    %-29s: CONSTANT tiny_integer := %d;\n
$ FORMAT PYXIS_FORMAT	"%-29s: CONSTANT tiny_integer := %d;\n
$ FORMAT OPT_FORMAT	"%-32s: CONSTANT integer := %d;\n
$ FORMAT SQL_FORMAT	"%-16s: CONSTANT short_integer := %d;\n

$ SYMBOLS BLR DTYPE BLR_FORMAT
$ SYMBOLS BLR MECH SIGNED BLR_FORMAT
 
$ SYMBOLS BLR STATEMENTS BLR_FORMAT
$ SYMBOLS BLR VALUES BLR_FORMAT
 
$ SYMBOLS BLR BOOLEANS BLR_FORMAT
 
$ SYMBOLS BLR RSE BLR_FORMAT
 
$ SYMBOLS BLR JOIN BLR_FORMAT
 
$ SYMBOLS BLR AGGREGATE BLR_FORMAT
$ SYMBOLS BLR NEW BLR_FORMAT

--- Dynamic Data Definition Language operators

--- Version number

$ SYMBOLS DYN MECH SIGNED DYN_FORMAT

--- Operations (may be nested)

$ SYMBOLS DYN OPERATIONS SIGNED DYN_FORMAT

--- View specific stuff

$ SYMBOLS DYN VIEW SIGNED DYN_FORMAT

--- Generic attributes

$ SYMBOLS DYN GENERIC SIGNED DYN_FORMAT

--- Relation specific attributes

$ SYMBOLS DYN RELATION SIGNED DYN_FORMAT

--- Global field specific attributes

$ SYMBOLS DYN GLOBAL SIGNED DYN_FORMAT

--- Local field specific attributes

$ SYMBOLS DYN FIELD SIGNED DYN_FORMAT

--- Index specific attributes

$ SYMBOLS DYN INDEX SIGNED DYN_FORMAT

--- Trigger specific attributes

$ SYMBOLS DYN TRIGGER SIGNED DYN_FORMAT

--- Security Class specific attributes

$ SYMBOLS DYN SECURITY SIGNED DYN_FORMAT

--- Dimension attributes

$ SYMBOLS DYN ARRAY SIGNED DYN_FORMAT

--- File specific attributes

$ SYMBOLS DYN FILES DYN_FORMAT

--- Function specific attributes

$ SYMBOLS DYN FUNCTIONS DYN_FORMAT

--- Generator specific attributes

$ SYMBOLS DYN GENERATOR DYN_FORMAT

--- Array slice description language (SDL)

$ SYMBOLS SDL SDL SIGNED SDL_FORMAT

--- Database parameter block stuff 

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

--- Transaction parameter block stuff 

$ SYMBOLS TPB ITEMS	PB_FORMAT

--- Blob parameter block stuff

$ SYMBOLS BPB ITEMS	PB_FORMAT

--- Common, structural codes

$ SYMBOLS INFO MECH	PB_FORMAT

--- Database information items 

$ SYMBOLS INFO DB	PB_FORMAT

--- Request information items 

$ SYMBOLS INFO REQUEST	PB_FORMAT

--- Blob information items

$ SYMBOLS INFO BLOB	PB_FORMAT

--- Transaction information items 

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

--- Error codes 

$ SYMBOLS ERROR MECH	ERROR_FORMAT

$ ERROR MAJOR		ERROR_FORMAT

--- Minor codes subject to change 

$ ERROR MINOR		ERROR_FORMAT

--- Dynamic SQL datatypes 

$ SYMBOLS SQL DTYPE SQL_FORMAT

--- Forms Package definitions 

--- Map definition block definitions

$ SYMBOLS PYXIS MAP SIGNED  PYXIS_FORMAT

--- Field option flags for display options 

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

--- Field option values following display 

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

--- Pseudo key definitions 

$ SYMBOLS PYXIS KEY  OPT_FORMAT

--- Menu definition stuff 

$ SYMBOLS PYXIS MENU SIGNED PYXIS_FORMAT

database_error		: EXCEPTION;
null_blob		: CONSTANT quad := (0, 0);
null_tpb		: tpb (0..0);
null_dpb		: dpb (0..0);

PROCEDURE attach_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb);
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: short_integer);

PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: short_integer);

PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE create_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb;
    db_type		: short_integer);

PROCEDURE detach_database (
    user_status		: OUT status_vector;
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE decode_date (
    date                : quad;
    times               : IN OUT gds_tm);
                                  
PROCEDURE encode_date (
    times               : IN OUT gds_tm;
    date                : quad);

PROCEDURE event_wait (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    length              : short_integer;
    events              : string;
    buffer              : string);

PROCEDURE get_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    actual_length	: OUT short_integer;
    buffer_length	: short_integer;
    buffer		: string);

PROCEDURE get_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT integer);

PROCEDURE open_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    msg_length          : short_integer;
    msg                 : blr);

PROCEDURE put_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    length		: short_integer;
    buffer		: string);

PROCEDURE put_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE queue_events (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    id                  : quad;
    length              : short_integer;
    events              : string;
    ast                 : integer;
    arg                 : quad);

PROCEDURE receive (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE set_debug (
    debug_val           : integer);

PROCEDURE start_and_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE start_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: short_integer);

PROCEDURE start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    db_handle		: database_handle;
    tpb_length		: short_integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    level		: short_integer);

PROCEDURE print_status (
    user_status		: status_vector);

PROCEDURE print_version (
    db_handle   :  database_handle);

FUNCTION sqlcode (
    user_status		: status_vector) RETURN integer;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
    user_status		: OUT status_vector;
    cursor_name		: string);

PROCEDURE dsql_declare (
    user_status		: OUT status_vector;
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    user_status		: OUT status_vector;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: short_integer;
    command		: string);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT integer;
    cursor_name		: string);

PROCEDURE dsql_finish (
    db_handle		: database_handle);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE create_window (
    win_handle		: OUT window_handle;
    name_length		: short_integer;
    file_name		: string;
    width		: IN OUT short_integer;
    height		: IN OUT short_integer);

PROCEDURE delete_window (
    win_handle		: IN OUT window_handle);

PROCEDURE drive_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: short_integer;
    form_name		: string);

FUNCTION menu (
    win_handle		: window_handle;
    men_handle		: menu_handle;
    menu_length		: short_integer;
    menu		: blr) RETURN short_integer;

PROCEDURE pop_window (
    win_handle		: window_handle);

PROCEDURE reset_form (
    user_status		: OUT status_vector;
    win_handle		: window_handle);

PROCEDURE drive_menu (
    win_handle		: IN OUT window_handle;
    men_handle		: IN OUT menu_handle;
    blr_length		: short_integer;
    blr_source		: blr;
    title_length	: short_integer;
    title		: string;
    terminator		: OUT short_integer;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer);

PROCEDURE get_entree (
    men_handle		: menu_handle;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer;
    entree_end		: OUT short_integer);

PROCEDURE initialize_menu (
    men_handle		: IN OUT menu_handle);

PROCEDURE put_entree (
    men_handle		: menu_handle;
    entree_length	: short_integer;
    entree_text		: string;
    entree_value	: integer);



---
--- Same routines, but with automatic error checking
---

PROCEDURE attach_database (
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb);
    
PROCEDURE cancel_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE create_database (
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb;
    db_type		: short_integer);

PROCEDURE detach_database (
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE get_segment (
    completion_code	: OUT integer;
    blb_handle		: blob_handle;
    actual_length	: OUT short_integer;
    buffer_length	: short_integer;
    buffer		: string);

PROCEDURE get_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT integer);

PROCEDURE open_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    trans_handle	: IN OUT transaction_handle;
    msg_length          : short_integer;
    msg                 : blr);

PROCEDURE put_segment (
    blb_handle		: blob_handle;
    length		: short_integer;
    buffer		: string);

PROCEDURE put_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE queue_events (
    db_handle           : database_handle;
    id                  : quad;
    length              : short_integer;
    events              : string;
    ast                 : integer;
    arg                 : quad);

PROCEDURE receive (
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE rollback_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE start_and_send (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level			: short_integer);

PROCEDURE start_request (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: short_integer);

PROCEDURE start_multiple (
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    teb			: SYSTEM.ADDRESS);

PROCEDURE start_transaction (
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    db_handle		: database_handle;
    tpb_length		: short_integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    req_handle		: request_handle;
    level		: short_integer);

PROCEDURE dsql_close (
    cursor_name		: string);

PROCEDURE dsql_declare (
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: short_integer;
    command		: string);

PROCEDURE dsql_fetch (
    sqlcode		: OUT integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    sqlcode		: OUT integer;
    cursor_name		: string);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE compile_sub_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE drive_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: short_integer;
    form_name		: string);

PROCEDURE reset_form (
    win_handle		: window_handle);

END interbase;

PACKAGE BODY interbase IS

---
--- Actual Interbase entrypoints
---

PROCEDURE gds_attach_database (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    file_length		: SYSTEM.ADDRESS; -- short_integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    handle		: SYSTEM.ADDRESS; -- IN OUT database_handle;
    dpb_length		: SYSTEM.ADDRESS; -- short_integer;
    dpb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_blob_display (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        field_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- short_integer;

PROCEDURE gds_blob_dump (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- short_integer;

PROCEDURE gds_blob_edit (
	blob_id		: SYSTEM.ADDRESS; -- IN OUT quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        field_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- short_integer;

PROCEDURE gds_blob_load (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- short_integer;

PROCEDURE gds_cancel_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); -- IN OUT blob_handle);

PROCEDURE gds_cancel_events (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    blb_handle          : SYSTEM.ADDRESS); -- IN OUT blob_handle);

PROCEDURE gds_close_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); --IN OUT blob_handle);

PROCEDURE gds_commit_retaining (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_commit_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_compile_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: SYSTEM.ADDRESS; -- short_integer;
    blr			: SYSTEM.ADDRESS);

PROCEDURE gds_compile_request2 (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: SYSTEM.ADDRESS; -- short_integer;
    blr_arg		: SYSTEM.ADDRESS); -- blr;

PROCEDURE gds_create_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS); --OUT quad);

PROCEDURE gds_create_blob2 (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS; --OUT quad;
    bpb_length          : SYSTEM.ADDRESS; --short_integer;
    bpb                 : SYSTEM.ADDRESS); --blr;

PROCEDURE gds_create_database (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    file_length		: SYSTEM.ADDRESS; --short_integer;
    file_name		: SYSTEM.ADDRESS; --string;
    db_handle		: SYSTEM.ADDRESS; --IN OUT database_handle;
    dpb_length		: SYSTEM.ADDRESS; --short_integer;
    dpb_arg		: SYSTEM.ADDRESS; --dpb;
    db_type		: SYSTEM.ADDRESS); --integer);

PROCEDURE gds_ddl (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    msg_length		: SYSTEM.ADDRESS; -- short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE gds_decode_date (
    date                : SYSTEM.ADDRESS; -- quad;
    times               : SYSTEM.ADDRESS); -- gds_tm);

PROCEDURE gds_detach_database (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    handle		: SYSTEM.ADDRESS); -- IN OUT database_handle);

PROCEDURE gds_encode_date (
    times               : SYSTEM.ADDRESS; -- gds_tm;
    date                : SYSTEM.ADDRESS); -- OUT quad);

PROCEDURE gds_event_wait (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    length              : SYSTEM.ADDRESS; -- short_integer;
    events              : SYSTEM.ADDRESS; -- string;
    buffer              : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_get_segment (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; --blob_handle;
    actual_length	: SYSTEM.ADDRESS; --OUT integer;
    buffer_length	: SYSTEM.ADDRESS; --integer;
    buffer		: SYSTEM.ADDRESS); --string;

PROCEDURE gds_get_slice (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : SYSTEM.ADDRESS; -- quad;
    sdl_length          : SYSTEM.ADDRESS; -- short_integer;
    sdl                 : SYSTEM.ADDRESS; -- blr;
    param_length        : SYSTEM.ADDRESS; -- short_integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : SYSTEM.ADDRESS; -- integer;
    slice               : SYSTEM.ADDRESS; -- string;
    return_length       : SYSTEM.ADDRESS); -- OUT integer;

PROCEDURE gds_open_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS); --quad;

PROCEDURE gds_open_blob2 (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS; --quad;
    bpb_length          : SYSTEM.ADDRESS; --short_integer;
    bpb                 : SYSTEM.ADDRESS); --blr;

PROCEDURE gds_prepare_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_prepare_transaction2 (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    msg_length          : SYSTEM.ADDRESS; -- short_integer;
    msg                 : SYSTEM.ADDRESS); -- blr;

PROCEDURE gds_put_segment (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; --blob_handle;
    length		: SYSTEM.ADDRESS; --integer;
    buffer		: SYSTEM.ADDRESS); --string;

PROCEDURE gds_put_slice (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : SYSTEM.ADDRESS; -- quad;
    sdl_length          : SYSTEM.ADDRESS; -- short_integer;
    sdl                 : SYSTEM.ADDRESS; -- blr;
    param_length        : SYSTEM.ADDRESS; -- short_integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : SYSTEM.ADDRESS; -- integer;
    slice               : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_queue_events (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    id                  : SYSTEM.ADDRESS; -- quad;
    length              : SYSTEM.ADDRESS; -- short_integer;
    events              : SYSTEM.ADDRESS; -- string;
    ast                 : SYSTEM.ADDRESS; -- integer;
    arg                 : SYSTEM.ADDRESS); -- quad;

PROCEDURE gds_receive (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: SYSTEM.ADDRESS; -- short_integer;
    msg_length		: SYSTEM.ADDRESS; -- short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); -- short_integer;

PROCEDURE gds_rollback_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_send (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: SYSTEM.ADDRESS; -- short_integer;
    msg_length		: SYSTEM.ADDRESS; -- short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); -- short_integer);

PROCEDURE gds_set_debug (
    debug_val           : SYSTEM.ADDRESS); -- short_integer);

PROCEDURE gds_start_and_send (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    msg_type		: SYSTEM.ADDRESS; -- short_integer;
    msg_length		: SYSTEM.ADDRESS; -- short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); -- short_integer);

PROCEDURE gds_start_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    level		: SYSTEM.ADDRESS); -- short_integer);

PROCEDURE gds_start_multiple (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: SYSTEM.ADDRESS; -- short_integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_start_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: SYSTEM.ADDRESS; -- short_integer;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    dpb_length		: SYSTEM.ADDRESS; -- short_integer;
    dpb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_unwind_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    level		: SYSTEM.ADDRESS); -- short_integer);

PROCEDURE gds_print_status (
    user_status		: SYSTEM.ADDRESS); -- status_vector;

PROCEDURE gds_version (
    db_handle    : SYSTEM.ADDRESS;
    ptr1	:  integer;
    ptr2	:  SYSTEM.ADDRESS); 

FUNCTION gds_sqlcode (
    user_status		: SYSTEM.ADDRESS) RETURN integer;

--- Dynamic SQL procedures 

PROCEDURE gds_close (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_declare (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_describe (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_dsql_finish (
    user_status		: SYSTEM.ADDRESS); -- database handle;

PROCEDURE gds_execute (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_execute_immediate (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    command_length	: SYSTEM.ADDRESS; -- short_integer;
    command		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_fetch (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    sqlcode		: SYSTEM.ADDRESS; -- OUT integer;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_open (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_prepare (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    command_length	: SYSTEM.ADDRESS; -- short_integer;
    command		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

--- PYXIS procedures

PROCEDURE pyxis_compile_map (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- short_integer;
    map			: SYSTEM.ADDRESS); -- blr);

PROCEDURE pyxis_compile_sub_map (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- short_integer;
    map			: SYSTEM.ADDRESS); -- blr);

PROCEDURE pyxis_create_window (
    win_handle		: SYSTEM.ADDRESS; -- OUT window_handle;
    name_length		: SYSTEM.ADDRESS; -- short_integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    width		: SYSTEM.ADDRESS; -- IN OUT short_integer;
    height		: SYSTEM.ADDRESS); -- IN OUT short_integer);

PROCEDURE pyxis_delete_window (
    win_handle		: SYSTEM.ADDRESS); -- IN OUT window_handle);

PROCEDURE pyxis_drive_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS; -- SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_fetch (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    output		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_insert (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_load_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    frm_handle		: SYSTEM.ADDRESS; -- IN OUT form_handle;
    name_length		: SYSTEM.ADDRESS; -- short_integer;
    form_name		: SYSTEM.ADDRESS); -- string);

FUNCTION pyxis_menu (
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    menu_length		: SYSTEM.ADDRESS; -- short_integer;
    menu		: SYSTEM.ADDRESS) RETURN short_integer; -- blr) ;

PROCEDURE pyxis_pop_window (
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_reset_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_drive_menu (
    win_handle		: SYSTEM.ADDRESS; -- IN OUT window_handle;
    men_handle		: SYSTEM.ADDRESS; -- IN OUT menu_handle;
    blr_length		: SYSTEM.ADDRESS; -- short_integer;
    blr_source		: SYSTEM.ADDRESS; -- blr;
    title_length	: SYSTEM.ADDRESS; -- short_integer;
    title		: SYSTEM.ADDRESS; -- string;
    terminator		: SYSTEM.ADDRESS; -- OUT short_integer;
    entree_length	: SYSTEM.ADDRESS; -- OUT short_integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS); -- OUT integer);

PROCEDURE pyxis_get_entree (
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    entree_length	: SYSTEM.ADDRESS; -- OUT short_integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS; -- OUT integer;
    entree_end		: SYSTEM.ADDRESS); -- OUT short_integer);

PROCEDURE pyxis_initialize_menu (
    men_handle		: SYSTEM.ADDRESS); -- IN OUT menu_handle);

PROCEDURE pyxis_put_entree (
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    entree_length	: SYSTEM.ADDRESS; -- short_integer;
    entree_text		: SYSTEM.ADDRESS; -- string;
    entree_value	: SYSTEM.ADDRESS); -- integer);





pragma interface (C, gds_attach_database, "gds_$attach_database");

pragma interface (C, gds_blob_display, "blob_$display");
pragma interface (C, gds_blob_dump, "blob_$dump");
pragma interface (C, gds_blob_edit, "blob_$edit");
pragma interface (C, gds_blob_load, "blob_$load");

pragma interface (C, gds_cancel_blob, "gds_$cancel_blob");
pragma interface (C, gds_cancel_events, "gds_$cancel_events");
pragma interface (C, gds_close, "gds_$close");
pragma interface (C, gds_close_blob, "gds_$close_blob");
pragma interface (C, gds_commit_retaining, "gds_$commit_retaining");
pragma interface (C, gds_commit_transaction, "gds_$commit_transaction");
pragma interface (C, gds_compile_request, "gds_$compile_request");
pragma interface (C, gds_compile_request2, "gds_$compile_request2");
pragma interface (C, gds_create_blob, "gds_$create_blob");
pragma interface (C, gds_create_blob2, "gds_$create_blob2");
pragma interface (C, gds_create_database, "gds_$create_database");

pragma interface (C, gds_ddl, "gds_$ddl");
pragma interface (C, gds_declare, "gds_$declare");
pragma interface (C, gds_decode_date, "gds_$decode_date");
pragma interface (C, gds_describe, "gds_$describe");
pragma interface (C, gds_detach_database, "gds_$detach_database");
pragma interface (C, gds_dsql_finish, "gds_$dsql_finish");

pragma interface (C, gds_encode_date, "gds_$encode_date");
pragma interface (C, gds_event_wait, "gds_$event_wait");
pragma interface (C, gds_execute, "gds_$execute");
pragma interface (C, gds_execute_immediate, "gds_$execute_immediate");

pragma interface (C, gds_fetch, "gds_$fetch_a");

pragma interface (C, gds_get_segment, "gds_$get_segment");
pragma interface (C, gds_get_slice, "gds_$get_slice");

pragma interface (C, gds_open, "gds_$open");
pragma interface (C, gds_open_blob, "gds_$open_blob");
pragma interface (C, gds_open_blob2, "gds_$open_blob2");

pragma interface (C, gds_prepare, "gds_$prepare");
pragma interface (C, gds_prepare_transaction, "gds_$prepare_transaction");
pragma interface (C, gds_prepare_transaction2, "gds_$prepare_transaction2");
pragma interface (C, gds_print_status, "gds_$print_status");
pragma interface (C, gds_put_segment, "gds_$put_segment");
pragma interface (C, gds_put_slice, "gds_$put_slice");

pragma interface (C, gds_queue_events, "gds_$que_events");

pragma interface (C, gds_receive, "gds_$receive");
pragma interface (C, gds_rollback_transaction, "gds_$rollback_transaction");

pragma interface (C, gds_send, "gds_$send");
pragma interface (C, gds_set_debug, "gds_$set_debug");
pragma interface (C, gds_sqlcode, "gds_$sqlcode");
pragma interface (C, gds_start_and_send, "gds_$start_and_send");
pragma interface (C, gds_start_multiple, "gds_$start_multiple");
pragma interface (C, gds_start_request, "gds_$start_request");
pragma interface (C, gds_start_transaction, "gds_$start_transaction");

pragma interface (C, gds_unwind_request, "gds_$unwind_request");

pragma interface (C, gds_version, "gds_$version");

pragma interface (C, pyxis_compile_map, "pyxis_$compile_map");
pragma interface (C, pyxis_compile_sub_map, "pyxis_$compile_sub_map");
pragma interface (C, pyxis_create_window, "pyxis_$create_window");
pragma interface (C, pyxis_delete_window, "pyxis_$delete_window");
pragma interface (C, pyxis_drive_form, "pyxis_$drive_form");
pragma interface (C, pyxis_drive_menu, "pyxis_$drive_menu");
pragma interface (C, pyxis_fetch, "pyxis_$fetch");
pragma interface (C, pyxis_get_entree, "pyxis_$get_entree");
pragma interface (C, pyxis_initialize_menu, "pyxis_$initialize_menu");
pragma interface (C, pyxis_insert, "pyxis_$insert");
pragma interface (C, pyxis_load_form, "pyxis_$load_form");
pragma interface (C, pyxis_menu, "pyxis_$menu");
pragma interface (C, pyxis_pop_window, "pyxis_$pop_window");
pragma interface (C, pyxis_put_entree, "pyxis_$put_entree");
pragma interface (C, pyxis_reset_form, "pyxis_$reset_form");


---
--- Interface routines that return status vector
---

PROCEDURE attach_database (
	    user_status		: OUT status_vector;
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb) IS
    BEGIN
	gds_attach_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address);
    END attach_database;
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_display (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            field_name'address,
            name_length'address);
    END blob_display;


PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_dump (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_dump;


PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_edit (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            field_name'address,
            name_length'address);
    END blob_edit;


PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_load (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_load;


PROCEDURE cancel_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_cancel_blob (
	    user_status'address,
	    blb_handle'address);
    END cancel_blob;

PROCEDURE cancel_events (
        user_status         : OUT status_vector;
        blb_handle          : IN OUT blob_handle) IS
    BEGIN
        gds_cancel_events (
            user_status'address,
            blb_handle'address);
    END cancel_events;

PROCEDURE close_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_close_blob (
	    user_status'address,
	    blb_handle'address);
    END close_blob;

PROCEDURE commit_retaining (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle) IS
    BEGIN
        gds_commit_retaining (
            user_status'address,
            trans_handle'address);
    END commit_retaining;

PROCEDURE commit_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_commit_transaction (
	    user_status'address,
	    trans_handle'address);
    END commit_transaction;


PROCEDURE compile_request (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
    END compile_request;


PROCEDURE compile_request2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request2 (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
    END compile_request2;


PROCEDURE create_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS
    BEGIN
	gds_create_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
    END create_blob;

PROCEDURE create_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS
    BEGIN
	gds_create_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
    END create_blob2;

PROCEDURE create_database (
	    user_status		: OUT status_vector;
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb;
	    db_type		: short_integer) IS
    BEGIN
	gds_create_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address,
	    db_type'address);
    END create_database;

PROCEDURE ddl (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_ddl (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    msg_length'address,
	    msg);
    END ddl;

PROCEDURE decode_date (
            date                : quad;
            times               : IN OUT gds_tm) IS
    BEGIN
        gds_decode_date (
            date'address,
            times'address);
    END decode_date;


PROCEDURE detach_database (
	    user_status		: OUT status_vector;
	    db_handle		: IN OUT database_handle) IS
    BEGIN
	gds_detach_database (
	    user_status'address,
	    db_handle'address);
    END detach_database;

PROCEDURE encode_date (
            times               : IN OUT gds_tm;
            date                : quad) IS
    BEGIN
        gds_encode_date (
            times'address,
            date'address);
    END encode_date;


PROCEDURE event_wait (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            length              : short_integer;
            events              : string;
            buffer              : string) IS
    BEGIN
        gds_event_wait (
            user_status'address,
            db_handle'address,
            length'address,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    actual_length	: OUT short_integer;
	    buffer_length	: short_integer;
	    buffer		: string) IS
    BEGIN
	gds_get_segment (
	    user_status'address,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length'address,
	    buffer'address);
    END get_segment;

PROCEDURE get_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT integer) IS
    BEGIN
        gds_get_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice,
            return_length'address);
    END get_slice;

PROCEDURE open_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS
    BEGIN
	gds_open_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
    END open_blob;

PROCEDURE open_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS
    BEGIN
	gds_open_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
    END open_blob2;

PROCEDURE prepare_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_prepare_transaction (
	    user_status'address,
	    trans_handle'address);
    END prepare_transaction;

PROCEDURE prepare_transaction2 (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle;
            msg_length          : short_integer;
            msg                 : blr) IS
    BEGIN
	gds_prepare_transaction2 (
	    user_status'address,
	    trans_handle'address,
            msg_length'address,
            msg'address);
    END prepare_transaction2;

PROCEDURE put_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    length		: short_integer;
	    buffer		: string) IS
    BEGIN
	gds_put_segment (
	    user_status'address,
	    blb_handle'address,
	    length'address,
	    buffer'address);
    END put_segment;

PROCEDURE put_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS) IS
    BEGIN
        gds_put_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice);
    END put_slice;

PROCEDURE queue_events (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            id                  : quad;
            length              : short_integer;
            events              : string;
            ast                 : integer;
            arg                 : quad) IS
    BEGIN
        gds_queue_events (
            user_status'address,
            db_handle'address,
            id'address,
            length'address,
            events'address,
            ast'address,
            arg'address);
    END queue_events;

PROCEDURE receive (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_receive (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END receive;

PROCEDURE rollback_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_rollback_transaction (
	    user_status'address,
	    trans_handle'address);
    END rollback_transaction;


PROCEDURE send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_send (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END send;

PROCEDURE set_debug (
            debug_val           : integer) IS
    BEGIN
        gds_set_debug (
            debug_val'address);
    END set_debug;

PROCEDURE start_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: short_integer) IS
    BEGIN
	gds_start_request (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    level'address);
    END start_request;


PROCEDURE start_and_send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_start_and_send (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END start_and_send;

PROCEDURE start_multiple (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    teb			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_start_multiple (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    teb);
    END start_multiple;

    
PROCEDURE start_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    db_handle		: database_handle;
	    tpb_length		: short_integer;
	    tpb_arg		: tpb) IS
    BEGIN
	gds_start_transaction (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    db_handle'address,
	    tpb_length'address,
	    tpb_arg'address);
    END start_transaction;

    
PROCEDURE unwind_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    level		: short_integer) IS
    BEGIN
	gds_unwind_request (
	    user_status'address,
	    req_handle'address,
	    level'address);
    END unwind_request;


PROCEDURE print_status (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (
	    user_status'address);
    END print_status;


PROCEDURE print_version (
            db_handle: database_handle) IS

    ptr	: integer := 0;
    BEGIN
        gds_version (
            db_handle'address,
	    0,
	    ptr'address);
    END print_version;

FUNCTION sqlcode (
	user_status		: status_vector) RETURN integer IS
    BEGIN
	return gds_sqlcode (user_status'address);
    end sqlcode;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	user_status	: OUT status_vector;
	cursor_name	: string) IS
    BEGIN
	gds_close (
		user_status'address,
		cursor_name'address);
    END dsql_close;

PROCEDURE dsql_declare (
	user_status	: OUT status_vector;
	statement_name	: string;
	cursor_name	: string) IS
    BEGIN
	gds_declare (
		user_status'address,
		statement_name'address,
		cursor_name'address);
    END dsql_declare;

PROCEDURE dsql_describe (
	user_status	: OUT status_vector;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_describe (
		user_status'address,
		statement_name'address,
		descriptor);
    END dsql_describe;

PROCEDURE dsql_finish (
	db_handle	: database_handle) IS
    BEGIN
	gds_dsql_finish (
	    db_handle'address);
    END dsql_finish;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		descriptor);
    END dsql_execute;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string) IS
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: short_integer;
	command		: string) IS
    BEGIN
	gds_execute_immediate (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		descriptor);
    END dsql_fetch;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT integer;
	cursor_name	: string) IS
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
    END dsql_fetch;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle	: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		descriptor);
    END dsql_open;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle	: transaction_handle;
	cursor_name		: string) IS
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
    END dsql_open;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
    END dsql_prepare;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string) IS
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
    END compile_map;

PROCEDURE compile_sub_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_sub_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
    END compile_sub_map;

PROCEDURE create_window (
	win_handle	: OUT window_handle;
	name_length	: short_integer;
	file_name	: string;
	width		: IN OUT short_integer;
	height		: IN OUT short_integer) IS
    BEGIN
	pyxis_create_window (
		win_handle'address,
		name_length'address,
		file_name'address,
		width'address,
		height'address);
    END create_window;

PROCEDURE delete_window (
	win_handle	: IN OUT window_handle) IS
    BEGIN
	pyxis_delete_window (
		win_handle'address);
    END delete_window;

PROCEDURE drive_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_drive_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
    END drive_form;

PROCEDURE fetch_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_fetch (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_insert (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
    END insert_sub_form;

PROCEDURE load_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: short_integer;
	form_name	: string) IS
    BEGIN
	pyxis_load_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
    END load_form;

FUNCTION menu (
	win_handle	: window_handle;
	men_handle	: menu_handle;
	menu_length	: short_integer;
	menu		: blr) RETURN short_integer IS
    BEGIN
	return pyxis_menu (
		win_handle'address,
		men_handle'address,
		menu_length'address,
		menu'address);
    END menu;

PROCEDURE pop_window (
	win_handle	: window_handle) IS
    BEGIN
	pyxis_pop_window (
		win_handle'address);
    END pop_window;

PROCEDURE reset_form (
	user_status	: OUT status_vector;
	win_handle	: window_handle) IS
    BEGIN
	pyxis_reset_form (
		user_status'address,
		win_handle'address);
    END reset_form;

PROCEDURE drive_menu (
	win_handle		: IN OUT window_handle;
	men_handle		: IN OUT menu_handle;
	blr_length		: short_integer;
	blr_source		: blr;
	title_length		: short_integer;
	title			: string;
	terminator		: OUT short_integer;
	entree_length		: OUT short_integer;
	entree_text		: OUT string;
	entree_value		: OUT integer) IS
    BEGIN
	pyxis_drive_menu (
	 	win_handle'address,
		men_handle'address,
		blr_length'address,
		blr_source'address,
		title_length'address,
		title'address,
		terminator'address,
		entree_length'address,
		entree_text'address,
		entree_value'address);
    END drive_menu;

PROCEDURE get_entree (
	men_handle		: menu_handle;
	entree_length		: OUT short_integer;
	entree_text		: OUT string;
	entree_value		: OUT integer;
	entree_end		: OUT short_integer) IS
    BEGIN
	pyxis_get_entree (
		men_handle'address,
		entree_length'address,
		entree_text'address,
		entree_value'address,
		entree_end'address);
    END get_entree;

PROCEDURE initialize_menu (
	men_handle		: IN OUT menu_handle) IS
    BEGIN
	pyxis_initialize_menu (
		men_handle'address);
    END initialize_menu;

PROCEDURE put_entree (
	men_handle		: menu_handle;
	entree_length		: short_integer;
	entree_text		: string;
	entree_value		: integer) IS
    BEGIN
	pyxis_put_entree (
		men_handle'address,
		entree_length'address,
		entree_text'address,
		entree_value'address);
    END put_entree;


---
--- Internal routines
---

PROCEDURE error (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status'address);
	RAISE database_error;
    END;


---
--- Routines sans explicit status vector
---

PROCEDURE attach_database (
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_attach_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END attach_database;
    
PROCEDURE cancel_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_cancel_blob (
	    user_status'address,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END cancel_blob;

PROCEDURE cancel_events (
            blb_handle          : IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_cancel_events (
            user_status'address,
            blb_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END cancel_events;

PROCEDURE close_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_close_blob (
	    user_status'address,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END close_blob;

PROCEDURE commit_retaining (
            trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_commit_retaining (
            user_status'address,
            trans_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END commit_retaining;

PROCEDURE commit_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_commit_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END commit_transaction;


PROCEDURE compile_request (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request;


PROCEDURE compile_request2 (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request2 (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request2;


PROCEDURE create_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob;

PROCEDURE create_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob2;

PROCEDURE create_database (
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb;
	    db_type		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address,
	    db_type'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_database;

PROCEDURE ddl (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_ddl (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    msg_length'address,
	    msg);
	if user_status (1) /= 0 then error (user_status); end if;
    END ddl;

PROCEDURE detach_database (
	    db_handle		: IN OUT database_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_detach_database (
	    user_status'address,
	    db_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END detach_database;

    
PROCEDURE event_wait (
            db_handle           : database_handle;
            length              : short_integer;
            events              : string;
            buffer              : string) IS

	user_status 		: status_vector;
    BEGIN
        gds_event_wait (
            user_status'address,
            db_handle'address,
            length'address,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    completion_code	: OUT integer;
	    blb_handle		: blob_handle;
	    actual_length	: OUT short_integer;
	    buffer_length	: short_integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_get_segment (
	    user_status'address,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length'address,
	    buffer'address);
	if user_status (1) /= 0 and user_status (1) /= gds_segment and
           user_status (1) /= gds_segstr_eof then 
	    error (user_status); 
	end if;
        completion_code := user_status(1);
    END get_segment;

PROCEDURE get_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT integer) IS

	user_status 		: status_vector;
    BEGIN
        gds_get_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice,
            return_length'address);
    END get_slice;

    
PROCEDURE open_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob;

PROCEDURE open_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob2;

PROCEDURE prepare_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END prepare_transaction;

PROCEDURE prepare_transaction2 (
            trans_handle	: IN OUT transaction_handle;
            msg_length          : short_integer;
            msg                 : blr) IS

        user_status             : status_vector;
    BEGIN
	gds_prepare_transaction2 (
	    user_status'address,
	    trans_handle'address,
            msg_length'address,
            msg'address);
    END prepare_transaction2;

PROCEDURE put_segment (
	    blb_handle		: blob_handle;
	    length		: short_integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_put_segment (
	    user_status'address,
	    blb_handle'address,
	    length'address,
	    buffer'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END put_segment;

PROCEDURE put_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS) IS

            user_status         : status_vector;
    BEGIN
        gds_put_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice);
    END put_slice;

PROCEDURE queue_events (
            db_handle           : database_handle;
            id                  : quad;
            length              : short_integer;
            events              : string;
            ast                 : integer;
            arg                 : quad) IS

            user_status         : status_vector;
    BEGIN
        gds_queue_events (
            user_status'address,
            db_handle'address,
            id'address,
            length'address,
            events'address,
            ast'address,
            arg'address);
    END queue_events;

PROCEDURE receive (
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_receive (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END receive;

PROCEDURE rollback_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_rollback_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END rollback_transaction;


PROCEDURE send (
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_send (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END send;

PROCEDURE start_request (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_request (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_request;


PROCEDURE start_and_send (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 	: status_vector;
    BEGIN
	gds_start_and_send (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_and_send;

PROCEDURE start_multiple (
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    teb			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_multiple (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    teb);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_multiple;

    
PROCEDURE start_transaction (
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    db_handle		: database_handle;
	    tpb_length		: short_integer;
	    tpb_arg		: tpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_transaction (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    db_handle'address,
	    tpb_length'address,
	    tpb_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_transaction;

    
PROCEDURE unwind_request (
	    req_handle		: request_handle;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_unwind_request (
	    user_status'address,
	    req_handle'address,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END unwind_request;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_close (
		user_status'address,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_close;

PROCEDURE dsql_declare (
	statement_name	: string;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_declare (
		user_status'address,
		statement_name'address,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_declare;

PROCEDURE dsql_describe (
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_describe (
		user_status'address,
		statement_name'address,
		descriptor'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_describe;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: short_integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute_immediate (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	sqlcode		: OUT integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_fetch (
	sqlcode		: OUT integer;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_map;

PROCEDURE compile_sub_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_sub_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_sub_map;

PROCEDURE drive_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_drive_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END drive_form;

PROCEDURE fetch_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_fetch (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_insert (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
---	if user_status (1) /= 0 then error (user_status); end if;
    END insert_sub_form;

PROCEDURE load_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: short_integer;
	form_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_load_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END load_form;

PROCEDURE reset_form (
	win_handle	: window_handle) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_reset_form (
		user_status'address,
		win_handle'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END reset_form;


END interbase;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('ALSYS_ADA', 'interbase_alsys.a',
'WITH SYSTEM;

PACKAGE interbase IS

TYPE status_vector IS ARRAY (0..19) OF long_integer;
TYPE quad IS ARRAY (0..1) OF long_integer;
TYPE chars IS ARRAY (integer range <>) of character;
TYPE blr IS ARRAY (integer range <>) of short_integer;
TYPE dpb IS ARRAY (integer range <>) of short_integer;
TYPE tpb IS ARRAY (integer range <>) of short_integer;

SUBTYPE isc_short		IS integer;
SUBTYPE isc_long		IS long_integer;
SUBTYPE isc_float		IS float;
SUBTYPE isc_double		IS long_float;
SUBTYPE database_handle		IS long_integer;
SUBTYPE request_handle		IS long_integer;
SUBTYPE transaction_handle	IS long_integer;
SUBTYPE blob_handle		IS long_integer;
SUBTYPE form_handle		IS long_integer;
SUBTYPE map_handle		IS long_integer;
SUBTYPE window_handle		IS long_integer;
SUBTYPE menu_handle		IS long_integer;

TYPE gds_teb_t IS RECORD
    dbb_ptr	: SYSTEM.ADDRESS;
    tpb_len	: long_integer;
    tpb_ptr	: SYSTEM.ADDRESS;                       
END RECORD;

TYPE gds_tm IS RECORD
    tm_sec	: long_integer;
    tm_min	: long_integer;
    tm_hour	: long_integer;
    tm_mday	: long_integer;
    tm_mon	: long_integer;
    tm_year	: long_integer;
    tm_wday	: long_integer;
    tm_yday	: long_integer;
    tm_isdst	: long_integer;
END RECORD;

TYPE sqlvar is RECORD
	sqltype		: integer;
	sqllen		: integer;
	sqldata		: SYSTEM.ADDRESS;
	sqlind		: SYSTEM.ADDRESS;
	sqlname_length	: integer;
	sqlname		: chars (1..30);
END RECORD;

FOR sqlvar use record at mod 2;
	sqltype		at 0 range 0..15;
	sqllen		at 2 range 0..15;
	sqldata		at 4 range 0..31;
	sqlind		at 8 range 0..31;
	sqlname_length	at 12 range 0..15;
	sqlname		at 14 range 0..239;
END RECORD;

TYPE sqlvar_array IS ARRAY (short_integer range <>) of sqlvar;

--- Constants

    gds_true	: CONSTANT long_integer := 1;
    gds_false  : CONSTANT long_integer := 0;

$ SET SANS_DOLLAR

$ FORMAT BLR_FORMAT	"    %s : CONSTANT short_integer := %d;\n
$ FORMAT DYN_FORMAT	"    %-32s: CONSTANT short_integer := %d;\n
$ FORMAT SDL_FORMAT	"    %-32s: CONSTANT short_integer := %d;\n
$ FORMAT ERROR_FORMAT	"%-25s: CONSTANT long_integer := %d;\n
$ FORMAT PB_FORMAT	"    %-29s: CONSTANT short_integer := %d;\n
$ FORMAT PYXIS_FORMAT	"%-29s: CONSTANT short_integer := %d;\n
$ FORMAT OPT_FORMAT	"%-32s: CONSTANT long_integer := %d;\n
$ FORMAT SQL_FORMAT	"%-16s: CONSTANT integer := %d;\n

--- BLR

$ SYMBOLS BLR DTYPE BLR_FORMAT
$ SYMBOLS BLR MECH SIGNED BLR_FORMAT
 
$ SYMBOLS BLR STATEMENTS BLR_FORMAT
$ SYMBOLS BLR VALUES BLR_FORMAT
 
$ SYMBOLS BLR BOOLEANS BLR_FORMAT
 
$ SYMBOLS BLR RSE BLR_FORMAT
 
$ SYMBOLS BLR JOIN BLR_FORMAT
 
$ SYMBOLS BLR AGGREGATE BLR_FORMAT
$ SYMBOLS BLR NEW BLR_FORMAT

--- Dynamic Data Definition Language operators

--- Version number

$ SYMBOLS DYN MECH SIGNED DYN_FORMAT

--- Operations (may be nested)

$ SYMBOLS DYN OPERATIONS SIGNED DYN_FORMAT

--- View specific stuff

$ SYMBOLS DYN VIEW SIGNED DYN_FORMAT

--- Generic attributes

$ SYMBOLS DYN GENERIC SIGNED DYN_FORMAT

--- Relation specific attributes

$ SYMBOLS DYN RELATION SIGNED DYN_FORMAT

--- Global field specific attributes

$ SYMBOLS DYN GLOBAL SIGNED DYN_FORMAT

--- Local field specific attributes

$ SYMBOLS DYN FIELD SIGNED DYN_FORMAT

--- Index specific attributes

$ SYMBOLS DYN INDEX SIGNED DYN_FORMAT

--- Trigger specific attributes

$ SYMBOLS DYN TRIGGER SIGNED DYN_FORMAT

--- Security Class specific attributes

$ SYMBOLS DYN SECURITY SIGNED DYN_FORMAT

--- Dimension attributes

$ SYMBOLS DYN ARRAY SIGNED DYN_FORMAT

--- File specific attributes

$ SYMBOLS DYN FILES DYN_FORMAT

--- Function specific attributes

$ SYMBOLS DYN FUNCTIONS DYN_FORMAT

--- Generator specific attributes

$ SYMBOLS DYN GENERATOR DYN_FORMAT

--- Array slice description language (SDL)

$ SYMBOLS SDL SDL SIGNED SDL_FORMAT

--- Database parameter block stuff 

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

--- Transaction parameter block stuff 

$ SYMBOLS TPB ITEMS	PB_FORMAT

--- Blob parameter block stuff

$ SYMBOLS BPB ITEMS	PB_FORMAT

--- Common, structural codes

$ SYMBOLS INFO MECH	PB_FORMAT

--- Database information items 

$ SYMBOLS INFO DB	PB_FORMAT

--- Request information items 

$ SYMBOLS INFO REQUEST	PB_FORMAT

--- Blob information items

$ SYMBOLS INFO BLOB	PB_FORMAT

--- Transaction information items 

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

--- Error codes 

$ SYMBOLS ERROR MECH	ERROR_FORMAT

$ ERROR MAJOR		ERROR_FORMAT

--- Minor codes subject to change 

$ ERROR MINOR		ERROR_FORMAT

--- Dynamic SQL datatypes 

$ SYMBOLS SQL DTYPE SQL_FORMAT

--- Forms Package definitions 

--- Map definition block definitions

$ SYMBOLS PYXIS MAP SIGNED  PYXIS_FORMAT

--- Field option flags for display options 

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

--- Field option values following display 

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

--- Pseudo key definitions 

$ SYMBOLS PYXIS KEY  OPT_FORMAT

--- Menu definition stuff 

$ SYMBOLS PYXIS MENU SIGNED PYXIS_FORMAT

database_error		: EXCEPTION;
null_blob		: CONSTANT quad := (0, 0);
null_tpb		: tpb (0..0);
null_dpb		: dpb (0..0);

PROCEDURE attach_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb);
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: integer);

PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: integer);

PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: integer);

PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: integer);

PROCEDURE cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE create_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb;
    db_type		: integer);

PROCEDURE detach_database (
    user_status		: OUT status_vector;
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE decode_date (
    date                : quad;
    times               : IN OUT gds_tm);
                                  
PROCEDURE encode_date (
    times               : IN OUT gds_tm;
    date                : quad);

PROCEDURE event_wait (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    length              : integer;
    events              : string;
    buffer              : string);

PROCEDURE get_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    actual_length	: OUT integer;
    buffer_length	: integer;
    buffer		: string);

PROCEDURE get_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT long_integer);

PROCEDURE open_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    msg_length          : integer;
    msg                 : blr);

PROCEDURE put_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    length		: integer;
    buffer		: string);

PROCEDURE put_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE queue_events (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    id                  : quad;
    length              : integer;
    events              : string;
    ast                 : long_integer;
    arg                 : quad);

PROCEDURE receive (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE set_debug (
    debug_val           : long_integer);

PROCEDURE start_and_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: integer);

PROCEDURE start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    db_handle		: database_handle;
    tpb_length		: integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    level		: integer);

PROCEDURE print_status (
    user_status		: status_vector);

PROCEDURE print_version (
    db_handle   :  database_handle);

FUNCTION sqlcode (
    user_status		: status_vector) RETURN long_integer;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
    user_status		: OUT status_vector;
    cursor_name		: string);

PROCEDURE dsql_declare (
    user_status		: OUT status_vector;
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    user_status		: OUT status_vector;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: integer;
    command		: string);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT long_integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT long_integer;
    cursor_name		: string);

PROCEDURE dsql_finish (
    db_handle		: database_handle);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE create_window (
    win_handle		: OUT window_handle;
    name_length		: integer;
    file_name		: string;
    width		: IN OUT integer;
    height		: IN OUT integer);

PROCEDURE delete_window (
    win_handle		: IN OUT window_handle);

PROCEDURE drive_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: integer;
    form_name		: string);

FUNCTION menu (
    win_handle		: window_handle;
    men_handle		: menu_handle;
    menu_length		: integer;
    menu		: blr) RETURN integer;

PROCEDURE pop_window (
    win_handle		: window_handle);

PROCEDURE reset_form (
    user_status		: OUT status_vector;
    win_handle		: window_handle);

PROCEDURE drive_menu (
    win_handle		: IN OUT window_handle;
    men_handle		: IN OUT menu_handle;
    blr_length		: integer;
    blr_source		: blr;
    title_length	: integer;
    title		: string;
    terminator		: OUT integer;
    entree_length	: OUT integer;
    entree_text		: OUT string;
    entree_value	: OUT long_integer);

PROCEDURE get_entree (
    men_handle		: menu_handle;
    entree_length	: OUT integer;
    entree_text		: OUT string;
    entree_value	: OUT long_integer;
    entree_end		: OUT integer);

PROCEDURE initialize_menu (
    men_handle		: IN OUT menu_handle);

PROCEDURE put_entree (
    men_handle		: menu_handle;
    entree_length	: integer;
    entree_text		: string;
    entree_value	: long_integer);



---
--- Same routines, but with automatic error checking
---

PROCEDURE attach_database (
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb);
    
PROCEDURE cancel_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE create_database (
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb;
    db_type		: integer);

PROCEDURE detach_database (
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE get_segment (
    completion_code	: OUT long_integer;
    blb_handle		: blob_handle;
    actual_length	: OUT integer;
    buffer_length	: integer;
    buffer		: string);

PROCEDURE get_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT long_integer);

PROCEDURE open_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    trans_handle	: IN OUT transaction_handle;
    msg_length          : integer;
    msg                 : blr);

PROCEDURE put_segment (
    blb_handle		: blob_handle;
    length		: integer;
    buffer		: string);

PROCEDURE put_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE queue_events (
    db_handle           : database_handle;
    id                  : quad;
    length              : integer;
    events              : string;
    ast                 : long_integer;
    arg                 : quad);

PROCEDURE receive (
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE rollback_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_and_send (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_request (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: integer);

PROCEDURE start_multiple (
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    teb			: SYSTEM.ADDRESS);

PROCEDURE start_transaction (
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    db_handle		: database_handle;
    tpb_length		: integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    req_handle		: request_handle;
    level		: integer);

PROCEDURE dsql_close (
    cursor_name		: string);

PROCEDURE dsql_declare (
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: integer;
    command		: string);

PROCEDURE dsql_fetch (
    sqlcode		: OUT long_integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    sqlcode		: OUT long_integer;
    cursor_name		: string);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE compile_sub_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE drive_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: integer;
    form_name		: string);

PROCEDURE reset_form (
    win_handle		: window_handle);

END interbase;

PACKAGE BODY interbase IS

---
--- Actual Interbase entrypoints
---

PROCEDURE gds_attach_database (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    file_length		: SYSTEM.ADDRESS; -- integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    handle		: SYSTEM.ADDRESS; -- IN OUT database_handle;
    dpb_length		: SYSTEM.ADDRESS; -- integer;
    dpb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_blob_display (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        field_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer;

PROCEDURE gds_blob_dump (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer;

PROCEDURE gds_blob_edit (
	blob_id		: SYSTEM.ADDRESS; -- IN OUT quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        field_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer;

PROCEDURE gds_blob_load (
	blob_id		: SYSTEM.ADDRESS; -- quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer;

PROCEDURE gds_cancel_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); -- IN OUT blob_handle);

PROCEDURE gds_cancel_events (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    blb_handle          : SYSTEM.ADDRESS); -- IN OUT blob_handle);

PROCEDURE gds_close_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); --IN OUT blob_handle);

PROCEDURE gds_commit_retaining (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_commit_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_compile_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: SYSTEM.ADDRESS; -- integer;
    blr			: SYSTEM.ADDRESS);

PROCEDURE gds_compile_request2 (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: SYSTEM.ADDRESS; -- integer;
    blr_arg		: SYSTEM.ADDRESS); -- blr;

PROCEDURE gds_create_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS); --OUT quad);

PROCEDURE gds_create_blob2 (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS; --OUT quad;
    bpb_length          : SYSTEM.ADDRESS; --integer;
    bpb                 : SYSTEM.ADDRESS); --blr;

PROCEDURE gds_create_database (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    file_length		: SYSTEM.ADDRESS; --integer;
    file_name		: SYSTEM.ADDRESS; --string;
    db_handle		: SYSTEM.ADDRESS; --IN OUT database_handle;
    dpb_length		: SYSTEM.ADDRESS; --integer;
    dpb_arg		: SYSTEM.ADDRESS; --dpb;
    db_type		: SYSTEM.ADDRESS); --integer);

PROCEDURE gds_ddl (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    msg_length		: SYSTEM.ADDRESS; -- integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE gds_decode_date (
    date                : SYSTEM.ADDRESS; -- quad;
    times               : SYSTEM.ADDRESS); -- gds_tm);

PROCEDURE gds_detach_database (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    handle		: SYSTEM.ADDRESS); -- IN OUT database_handle);

PROCEDURE gds_encode_date (
    times               : SYSTEM.ADDRESS; -- gds_tm;
    date                : SYSTEM.ADDRESS); -- OUT quad);

PROCEDURE gds_event_wait (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    length              : SYSTEM.ADDRESS; -- integer;
    events              : SYSTEM.ADDRESS; -- string;
    buffer              : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_get_segment (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; --blob_handle;
    actual_length	: SYSTEM.ADDRESS; --OUT long_integer;
    buffer_length	: SYSTEM.ADDRESS; --long_integer;
    buffer		: SYSTEM.ADDRESS); --string;

PROCEDURE gds_get_slice (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : SYSTEM.ADDRESS; -- quad;
    sdl_length          : SYSTEM.ADDRESS; -- integer;
    sdl                 : SYSTEM.ADDRESS; -- blr;
    param_length        : SYSTEM.ADDRESS; -- integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : SYSTEM.ADDRESS; -- long_integer;
    slice               : SYSTEM.ADDRESS; -- string;
    return_length       : SYSTEM.ADDRESS); -- OUT long_integer;

PROCEDURE gds_open_blob (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS); --quad;

PROCEDURE gds_open_blob2 (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: SYSTEM.ADDRESS; --quad;
    bpb_length          : SYSTEM.ADDRESS; --integer;
    bpb                 : SYSTEM.ADDRESS); --blr;

PROCEDURE gds_prepare_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_prepare_transaction2 (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    msg_length          : SYSTEM.ADDRESS; -- integer;
    msg                 : SYSTEM.ADDRESS); -- blr;

PROCEDURE gds_put_segment (
    user_status		: SYSTEM.ADDRESS; --OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; --blob_handle;
    length		: SYSTEM.ADDRESS; --long_integer;
    buffer		: SYSTEM.ADDRESS); --string;

PROCEDURE gds_put_slice (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : SYSTEM.ADDRESS; -- quad;
    sdl_length          : SYSTEM.ADDRESS; -- integer;
    sdl                 : SYSTEM.ADDRESS; -- blr;
    param_length        : SYSTEM.ADDRESS; -- integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : SYSTEM.ADDRESS; -- long_integer;
    slice               : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_queue_events (
    user_status         : SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    id                  : SYSTEM.ADDRESS; -- quad;
    length              : SYSTEM.ADDRESS; -- integer;
    events              : SYSTEM.ADDRESS; -- string;
    ast                 : SYSTEM.ADDRESS; -- long_integer;
    arg                 : SYSTEM.ADDRESS); -- quad;

PROCEDURE gds_receive (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: SYSTEM.ADDRESS; -- integer;
    msg_length		: SYSTEM.ADDRESS; -- integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); --integer;

PROCEDURE gds_rollback_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_send (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: SYSTEM.ADDRESS; -- integer;
    msg_length		: SYSTEM.ADDRESS; -- integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_set_debug (
    debug_val           : SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_start_and_send (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    msg_type		: SYSTEM.ADDRESS; -- integer;
    msg_length		: SYSTEM.ADDRESS; -- integer;
    msg			: SYSTEM.ADDRESS;
    level		: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_start_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    level		: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_start_multiple (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: SYSTEM.ADDRESS; -- integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_start_transaction (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: SYSTEM.ADDRESS; -- integer;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    dpb_length		: SYSTEM.ADDRESS; -- integer;
    dpb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_unwind_request (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    level		: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_print_status (
    user_status		: SYSTEM.ADDRESS); -- status_vector;

PROCEDURE gds_version (
    db_handle    : SYSTEM.ADDRESS;
    ptr1	:  long_integer;
    ptr2	:  SYSTEM.ADDRESS); 

FUNCTION gds_sqlcode (
    user_status		: SYSTEM.ADDRESS) RETURN long_integer;

--- Dynamic SQL procedures 

PROCEDURE gds_close (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_declare (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_describe (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_dsql_finish (
    user_status		: SYSTEM.ADDRESS); -- database handle;

PROCEDURE gds_execute (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_execute_immediate (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    command_length	: SYSTEM.ADDRESS; -- integer;
    command		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_fetch (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    sqlcode		: SYSTEM.ADDRESS; -- OUT integer;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_open (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE gds_prepare (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    command_length	: SYSTEM.ADDRESS; -- integer;
    command		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

--- PYXIS procedures

PROCEDURE pyxis_compile_map (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- integer;
    map			: SYSTEM.ADDRESS); -- blr);

PROCEDURE pyxis_compile_sub_map (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- integer;
    map			: SYSTEM.ADDRESS); -- blr);

PROCEDURE pyxis_create_window (
    win_handle		: SYSTEM.ADDRESS; -- OUT window_handle;
    name_length		: SYSTEM.ADDRESS; -- integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    width		: SYSTEM.ADDRESS; -- IN OUT integer;
    height		: SYSTEM.ADDRESS); -- IN OUT integer);

PROCEDURE pyxis_delete_window (
    win_handle		: SYSTEM.ADDRESS); -- IN OUT window_handle);

PROCEDURE pyxis_drive_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS; -- SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_fetch (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    output		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_insert (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS); -- SYSTEM.ADDRESS);

PROCEDURE pyxis_load_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    frm_handle		: SYSTEM.ADDRESS; -- IN OUT form_handle;
    name_length		: SYSTEM.ADDRESS; -- integer;
    form_name		: SYSTEM.ADDRESS); -- string);

FUNCTION pyxis_menu (
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    menu_length		: SYSTEM.ADDRESS; -- integer;
    menu		: SYSTEM.ADDRESS) RETURN integer; -- blr) ;

PROCEDURE pyxis_pop_window (
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_reset_form (
    user_status		: SYSTEM.ADDRESS; -- OUT status_vector;
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_drive_menu (
    win_handle		: SYSTEM.ADDRESS; -- IN OUT window_handle;
    men_handle		: SYSTEM.ADDRESS; -- IN OUT menu_handle;
    blr_length		: SYSTEM.ADDRESS; -- integer;
    blr_source		: SYSTEM.ADDRESS; -- blr;
    title_length	: SYSTEM.ADDRESS; -- integer;
    title		: SYSTEM.ADDRESS; -- string;
    terminator		: SYSTEM.ADDRESS; -- OUT integer;
    entree_length	: SYSTEM.ADDRESS; -- OUT integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS); -- OUT long_integer);

PROCEDURE pyxis_get_entree (
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    entree_length	: SYSTEM.ADDRESS; -- OUT integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS; -- OUT integer;
    entree_end		: SYSTEM.ADDRESS); -- OUT integer);

PROCEDURE pyxis_initialize_menu (
    men_handle		: SYSTEM.ADDRESS); -- IN OUT menu_handle);

PROCEDURE pyxis_put_entree (
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    entree_length	: SYSTEM.ADDRESS; -- integer;
    entree_text		: SYSTEM.ADDRESS; -- string;
    entree_value	: SYSTEM.ADDRESS); -- long_integer);





pragma interface (C, gds_attach_database);
pragma interface_name (gds_attach_database, "gds_$attach_database");

pragma interface (C, gds_blob_display);
pragma interface_name (gds_blob_display, "blob_$display");
pragma interface (C, gds_blob_dump);
pragma interface_name (gds_blob_dump, "blob_$dump");
pragma interface (C, gds_blob_edit);
pragma interface_name (gds_blob_edit, "blob_$edit");
pragma interface (C, gds_blob_load);
pragma interface_name (gds_blob_load, "blob_$load");

pragma interface (C, gds_cancel_blob);
pragma interface_name (gds_cancel_blob, "gds_$cancel_blob");
pragma interface (C, gds_cancel_events);
pragma interface_name (gds_cancel_events, "gds_$cancel_events");
pragma interface (C, gds_close);
pragma interface_name (gds_close, "gds_$close");
pragma interface (C, gds_close_blob);
pragma interface_name (gds_close_blob, "gds_$close_blob");
pragma interface (C, gds_commit_retaining);
pragma interface_name (gds_commit_retaining, "gds_$commit_retaining");
pragma interface (C, gds_commit_transaction);
pragma interface_name (gds_commit_transaction, "gds_$commit_transaction");
pragma interface (C, gds_compile_request);
pragma interface_name (gds_compile_request, "gds_$compile_request");
pragma interface (C, gds_compile_request2);
pragma interface_name (gds_compile_request2, "gds_$compile_request2");
pragma interface (C, gds_create_blob);
pragma interface_name (gds_create_blob, "gds_$create_blob");
pragma interface (C, gds_create_blob2);
pragma interface_name (gds_create_blob2, "gds_$create_blob2");
pragma interface (C, gds_create_database);
pragma interface_name (gds_create_database, "gds_$create_database");

pragma interface (C, gds_ddl);
pragma interface_name (gds_ddl, "gds_$ddl");
pragma interface (C, gds_declare);
pragma interface_name (gds_declare, "gds_$declare");
pragma interface (C, gds_decode_date);
pragma interface_name (gds_decode_date, "gds_$decode_date");
pragma interface (C, gds_describe);
pragma interface_name (gds_describe, "gds_$describe");
pragma interface (C, gds_detach_database);
pragma interface_name (gds_detach_database, "gds_$detach_database");
pragma interface (C, gds_dsql_finish);
pragma interface_name (gds_dsql_finish, "gds_$dsql_finish");

pragma interface (C, gds_encode_date);
pragma interface_name (gds_encode_date, "gds_$encode_date");
pragma interface (C, gds_event_wait);
pragma interface_name (gds_event_wait, "gds_$event_wait");
pragma interface (C, gds_execute);
pragma interface_name (gds_execute, "gds_$execute");
pragma interface (C, gds_execute_immediate);
pragma interface_name (gds_execute_immediate, "gds_$execute_immediate");

pragma interface (C, gds_fetch);
pragma interface_name (gds_fetch, "gds_$fetch_a");

pragma interface (C, gds_get_segment);
pragma interface_name (gds_get_segment, "gds_$get_segment");
pragma interface (C, gds_get_slice);
pragma interface_name (gds_get_slice, "gds_$get_slice");

pragma interface (C, gds_open);
pragma interface_name (gds_open, "gds_$open");
pragma interface (C, gds_open_blob);
pragma interface_name (gds_open_blob, "gds_$open_blob");
pragma interface (C, gds_open_blob2);
pragma interface_name (gds_open_blob2, "gds_$open_blob2");

pragma interface (C, gds_prepare);
pragma interface_name (gds_prepare, "gds_$prepare");
pragma interface (C, gds_prepare_transaction);
pragma interface_name (gds_prepare_transaction, "gds_$prepare_transaction");
pragma interface (C, gds_prepare_transaction2);
pragma interface_name (gds_prepare_transaction2, "gds_$prepare_transaction2");
pragma interface (C, gds_print_status);
pragma interface_name (gds_print_status, "gds_$print_status");
pragma interface (C, gds_put_segment);
pragma interface_name (gds_put_segment, "gds_$put_segment");
pragma interface (C, gds_put_slice);
pragma interface_name (gds_put_slice, "gds_$put_slice");

pragma interface (C, gds_queue_events);
pragma interface_name (gds_queue_events, "gds_$que_events");

pragma interface (C, gds_receive);
pragma interface_name (gds_receive, "gds_$receive");
pragma interface (C, gds_rollback_transaction);
pragma interface_name (gds_rollback_transaction, "gds_$rollback_transaction");

pragma interface (C, gds_send);
pragma interface_name (gds_send, "gds_$send");
pragma interface (C, gds_set_debug);
pragma interface_name (gds_set_debug, "gds_$set_debug");
pragma interface (C, gds_sqlcode);
pragma interface_name (gds_sqlcode, "gds_$sqlcode");
pragma interface (C, gds_start_and_send);
pragma interface_name (gds_start_and_send, "gds_$start_and_send");
pragma interface (C, gds_start_multiple);
pragma interface_name (gds_start_multiple, "gds_$start_multiple");
pragma interface (C, gds_start_request);
pragma interface_name (gds_start_request, "gds_$start_request");
pragma interface (C, gds_start_transaction);
pragma interface_name (gds_start_transaction, "gds_$start_transaction");

pragma interface (C, gds_unwind_request);
pragma interface_name (gds_unwind_request, "gds_$unwind_request");

pragma interface (C, gds_version);
pragma interface_name (gds_version, "gds_$version");

pragma interface (C, pyxis_compile_map);
pragma interface_name (pyxis_compile_map, "pyxis_$compile_map");
pragma interface (C, pyxis_compile_sub_map);
pragma interface_name (pyxis_compile_sub_map, "pyxis_$compile_sub_map");
pragma interface (C, pyxis_create_window);
pragma interface_name (pyxis_create_window, "pyxis_$create_window");
pragma interface (C, pyxis_delete_window);
pragma interface_name (pyxis_delete_window, "pyxis_$delete_window");
pragma interface (C, pyxis_drive_form);
pragma interface_name (pyxis_drive_form, "pyxis_$drive_form");
pragma interface (C, pyxis_drive_menu);
pragma interface_name (pyxis_drive_menu, "pyxis_$drive_menu");
pragma interface (C, pyxis_fetch);
pragma interface_name (pyxis_fetch, "pyxis_$fetch");
pragma interface (C, pyxis_get_entree);
pragma interface_name (pyxis_get_entree, "pyxis_$get_entree");
pragma interface (C, pyxis_initialize_menu);
pragma interface_name (pyxis_initialize_menu, "pyxis_$initialize_menu");
pragma interface (C, pyxis_insert);
pragma interface_name (pyxis_insert, "pyxis_$insert");
pragma interface (C, pyxis_load_form);
pragma interface_name (pyxis_load_form, "pyxis_$load_form");
pragma interface (C, pyxis_menu);
pragma interface_name (pyxis_menu, "pyxis_$menu");
pragma interface (C, pyxis_pop_window);
pragma interface_name (pyxis_pop_window, "pyxis_$pop_window");
pragma interface (C, pyxis_put_entree);
pragma interface_name (pyxis_put_entree, "pyxis_$put_entree");
pragma interface (C, pyxis_reset_form);
pragma interface_name (pyxis_reset_form, "pyxis_$reset_form");


---
--- Interface routines that return status vector
---

PROCEDURE attach_database (
	    user_status		: OUT status_vector;
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb) IS
    BEGIN
	gds_attach_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address);
    END attach_database;
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_display (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            field_name'address,
            name_length'address);
    END blob_display;


PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_dump (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_dump;


PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_edit (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            field_name'address,
            name_length'address);
    END blob_edit;


PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_load (
            blob_id'address,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_load;


PROCEDURE cancel_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_cancel_blob (
	    user_status'address,
	    blb_handle'address);
    END cancel_blob;

PROCEDURE cancel_events (
        user_status         : OUT status_vector;
        blb_handle          : IN OUT blob_handle) IS
    BEGIN
        gds_cancel_events (
            user_status'address,
            blb_handle'address);
    END cancel_events;

PROCEDURE close_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_close_blob (
	    user_status'address,
	    blb_handle'address);
    END close_blob;

PROCEDURE commit_retaining (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle) IS
    BEGIN
        gds_commit_retaining (
            user_status'address,
            trans_handle'address);
    END commit_retaining;

PROCEDURE commit_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_commit_transaction (
	    user_status'address,
	    trans_handle'address);
    END commit_transaction;


PROCEDURE compile_request (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
    END compile_request;


PROCEDURE compile_request2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request2 (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
    END compile_request2;


PROCEDURE create_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS
    BEGIN
	gds_create_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
    END create_blob;

PROCEDURE create_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : integer;
            bpb                 : blr) IS
    BEGIN
	gds_create_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
    END create_blob2;

PROCEDURE create_database (
	    user_status		: OUT status_vector;
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb;
	    db_type		: integer) IS
    BEGIN
	gds_create_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address,
	    db_type'address);
    END create_database;

PROCEDURE ddl (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_ddl (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    msg_length'address,
	    msg);
    END ddl;

PROCEDURE decode_date (
            date                : quad;
            times               : IN OUT gds_tm) IS
    BEGIN
        gds_decode_date (
            date'address,
            times'address);
    END decode_date;


PROCEDURE detach_database (
	    user_status		: OUT status_vector;
	    db_handle		: IN OUT database_handle) IS
    BEGIN
	gds_detach_database (
	    user_status'address,
	    db_handle'address);
    END detach_database;

PROCEDURE encode_date (
            times               : IN OUT gds_tm;
            date                : quad) IS
    BEGIN
        gds_encode_date (
            times'address,
            date'address);
    END encode_date;


PROCEDURE event_wait (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            length              : integer;
            events              : string;
            buffer              : string) IS
    BEGIN
        gds_event_wait (
            user_status'address,
            db_handle'address,
            length'address,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    actual_length	: OUT integer;
	    buffer_length	: integer;
	    buffer		: string) IS
    BEGIN
	gds_get_segment (
	    user_status'address,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length'address,
	    buffer'address);
    END get_segment;

PROCEDURE get_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT long_integer) IS
    BEGIN
        gds_get_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice,
            return_length'address);
    END get_slice;

PROCEDURE open_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS
    BEGIN
	gds_open_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
    END open_blob;

PROCEDURE open_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : integer;
            bpb                 : blr) IS
    BEGIN
	gds_open_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
    END open_blob2;

PROCEDURE prepare_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_prepare_transaction (
	    user_status'address,
	    trans_handle'address);
    END prepare_transaction;

PROCEDURE prepare_transaction2 (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle;
            msg_length          : integer;
            msg                 : blr) IS
    BEGIN
	gds_prepare_transaction2 (
	    user_status'address,
	    trans_handle'address,
            msg_length'address,
            msg'address);
    END prepare_transaction2;

PROCEDURE put_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    length		: integer;
	    buffer		: string) IS
    BEGIN
	gds_put_segment (
	    user_status'address,
	    blb_handle'address,
	    length'address,
	    buffer'address);
    END put_segment;

PROCEDURE put_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : SYSTEM.ADDRESS) IS
    BEGIN
        gds_put_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice);
    END put_slice;

PROCEDURE queue_events (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            id                  : quad;
            length              : integer;
            events              : string;
            ast                 : long_integer;
            arg                 : quad) IS
    BEGIN
        gds_queue_events (
            user_status'address,
            db_handle'address,
            id'address,
            length'address,
            events'address,
            ast'address,
            arg'address);
    END queue_events;

PROCEDURE receive (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_receive (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END receive;

PROCEDURE rollback_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_rollback_transaction (
	    user_status'address,
	    trans_handle'address);
    END rollback_transaction;


PROCEDURE send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_send (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END send;

PROCEDURE set_debug (
            debug_val           : long_integer) IS
    BEGIN
        gds_set_debug (
            debug_val'address);
    END set_debug;

PROCEDURE start_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: integer) IS
    BEGIN
	gds_start_request (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    level'address);
    END start_request;


PROCEDURE start_and_send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_start_and_send (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
    END start_and_send;

PROCEDURE start_multiple (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    teb			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_start_multiple (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    teb);
    END start_multiple;

    
PROCEDURE start_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    db_handle		: database_handle;
	    tpb_length		: integer;
	    tpb_arg		: tpb) IS
    BEGIN
	gds_start_transaction (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    db_handle'address,
	    tpb_length'address,
	    tpb_arg'address);
    END start_transaction;

    
PROCEDURE unwind_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    level		: integer) IS
    BEGIN
	gds_unwind_request (
	    user_status'address,
	    req_handle'address,
	    level'address);
    END unwind_request;


PROCEDURE print_status (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (
	    user_status'address);
    END print_status;


PROCEDURE print_version (
            db_handle: interbase.database_handle) IS

    ptr	: long_integer := 0;
    BEGIN
        gds_version (
            db_handle'address,
	    0,
	    ptr'address);
    END print_version;

FUNCTION sqlcode (
	user_status		: status_vector) RETURN long_integer IS
    BEGIN
	return gds_sqlcode (user_status'address);
    end sqlcode;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	user_status	: OUT status_vector;
	cursor_name	: string) IS
    BEGIN
	gds_close (
		user_status'address,
		cursor_name'address);
    END dsql_close;

PROCEDURE dsql_declare (
	user_status	: OUT status_vector;
	statement_name	: string;
	cursor_name	: string) IS
    BEGIN
	gds_declare (
		user_status'address,
		statement_name'address,
		cursor_name'address);
    END dsql_declare;

PROCEDURE dsql_describe (
	user_status	: OUT status_vector;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_describe (
		user_status'address,
		statement_name'address,
		descriptor);
    END dsql_describe;

PROCEDURE dsql_finish (
	db_handle	: database_handle) IS
    BEGIN
	gds_dsql_finish (
	    db_handle'address);
    END dsql_finish;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		descriptor);
    END dsql_execute;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string) IS
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: integer;
	command		: string) IS
    BEGIN
	gds_execute_immediate (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT long_integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		descriptor);
    END dsql_fetch;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT long_integer;
	cursor_name	: string) IS
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
    END dsql_fetch;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle		: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		descriptor);
    END dsql_open;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle		: transaction_handle;
	cursor_name		: string) IS
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
    END dsql_open;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
    END dsql_prepare;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string) IS
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
    END compile_map;

PROCEDURE compile_sub_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_sub_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
    END compile_sub_map;

PROCEDURE create_window (
	win_handle	: OUT window_handle;
	name_length	: integer;
	file_name	: string;
	width		: IN OUT integer;
	height		: IN OUT integer) IS
    BEGIN
	pyxis_create_window (
		win_handle'address,
		name_length'address,
		file_name'address,
		width'address,
		height'address);
    END create_window;

PROCEDURE delete_window (
	win_handle	: IN OUT window_handle) IS
    BEGIN
	pyxis_delete_window (
		win_handle'address);
    END delete_window;

PROCEDURE drive_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_drive_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
    END drive_form;

PROCEDURE fetch_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_fetch (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_insert (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
    END insert_sub_form;

PROCEDURE load_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: integer;
	form_name	: string) IS
    BEGIN
	pyxis_load_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
    END load_form;

FUNCTION menu (
	win_handle	: window_handle;
	men_handle	: menu_handle;
	menu_length	: integer;
	menu		: blr) RETURN integer IS
    BEGIN
	return pyxis_menu (
		win_handle'address,
		men_handle'address,
		menu_length'address,
		menu'address);
    END menu;

PROCEDURE pop_window (
	win_handle	: window_handle) IS
    BEGIN
	pyxis_pop_window (
		win_handle'address);
    END pop_window;

PROCEDURE reset_form (
	user_status	: OUT status_vector;
	win_handle	: window_handle) IS
    BEGIN
	pyxis_reset_form (
		user_status'address,
		win_handle'address);
    END reset_form;

PROCEDURE drive_menu (
	win_handle		: IN OUT window_handle;
	men_handle		: IN OUT menu_handle;
	blr_length		: integer;
	blr_source		: blr;
	title_length		: integer;
	title			: string;
	terminator		: OUT integer;
	entree_length		: OUT integer;
	entree_text		: OUT string;
	entree_value		: OUT long_integer) IS
    BEGIN
	pyxis_drive_menu (
	 	win_handle'address,
		men_handle'address,
		blr_length'address,
		blr_source'address,
		title_length'address,
		title'address,
		terminator'address,
		entree_length'address,
		entree_text'address,
		entree_value'address);
    END drive_menu;

PROCEDURE get_entree (
	men_handle		: menu_handle;
	entree_length		: OUT integer;
	entree_text		: OUT string;
	entree_value		: OUT long_integer;
	entree_end		: OUT integer) IS
    BEGIN
	pyxis_get_entree (
		men_handle'address,
		entree_length'address,
		entree_text'address,
		entree_value'address,
		entree_end'address);
    END get_entree;

PROCEDURE initialize_menu (
	men_handle		: IN OUT menu_handle) IS
    BEGIN
	pyxis_initialize_menu (
		men_handle'address);
    END initialize_menu;

PROCEDURE put_entree (
	men_handle		: menu_handle;
	entree_length		: integer;
	entree_text		: string;
	entree_value		: long_integer) IS
    BEGIN
	pyxis_put_entree (
		men_handle'address,
		entree_length'address,
		entree_text'address,
		entree_value'address);
    END put_entree;


---
--- Internal routines
---

PROCEDURE error (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status'address);
	RAISE database_error;
    END;


---
--- Routines sans explicit status vector
---

PROCEDURE attach_database (
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_attach_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END attach_database;
    
PROCEDURE cancel_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_cancel_blob (
	    user_status'address,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END cancel_blob;

PROCEDURE cancel_events (
            blb_handle          : IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_cancel_events (
            user_status'address,
            blb_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END cancel_events;

PROCEDURE close_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_close_blob (
	    user_status'address,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END close_blob;

PROCEDURE commit_retaining (
            trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_commit_retaining (
            user_status'address,
            trans_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END commit_retaining;

PROCEDURE commit_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_commit_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END commit_transaction;


PROCEDURE compile_request (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request;


PROCEDURE compile_request2 (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request2 (
	    user_status'address,
	    db_handle'address,
	    req_handle'address,
	    blr_length'address,
	    blr_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request2;


PROCEDURE create_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob;

PROCEDURE create_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob2;

PROCEDURE create_database (
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb;
	    db_type		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_database (
	    user_status'address,
	    file_length'address,
	    file_name'address,
	    db_handle'address,
	    dpb_length'address,
	    dpb_arg'address,
	    db_type'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_database;

PROCEDURE ddl (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_ddl (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    msg_length'address,
	    msg);
	if user_status (1) /= 0 then error (user_status); end if;
    END ddl;

PROCEDURE detach_database (
	    db_handle		: IN OUT database_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_detach_database (
	    user_status'address,
	    db_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END detach_database;

    
PROCEDURE event_wait (
            db_handle           : database_handle;
            length              : integer;
            events              : string;
            buffer              : string) IS

	user_status 		: status_vector;
    BEGIN
        gds_event_wait (
            user_status'address,
            db_handle'address,
            length'address,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    completion_code	: OUT long_integer;
	    blb_handle		: blob_handle;
	    actual_length	: OUT integer;
	    buffer_length	: integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_get_segment (
	    user_status'address,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length'address,
	    buffer'address);
	if user_status (1) /= 0 and user_status (1) /= gds_segment and
           user_status (1) /= gds_segstr_eof then 
	    error (user_status); 
	end if;
        completion_code := user_status(1);
    END get_segment;

PROCEDURE get_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT long_integer) IS

	user_status 		: status_vector;
    BEGIN
        gds_get_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice,
            return_length'address);
    END get_slice;

    
PROCEDURE open_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob;

PROCEDURE open_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob2 (
	    user_status'address,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id'address,
            bpb_length'address,
            bpb'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob2;

PROCEDURE prepare_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END prepare_transaction;

PROCEDURE prepare_transaction2 (
            trans_handle	: IN OUT transaction_handle;
            msg_length          : integer;
            msg                 : blr) IS

        user_status             : status_vector;
    BEGIN
	gds_prepare_transaction2 (
	    user_status'address,
	    trans_handle'address,
            msg_length'address,
            msg'address);
    END prepare_transaction2;

PROCEDURE put_segment (
	    blb_handle		: blob_handle;
	    length		: integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_put_segment (
	    user_status'address,
	    blb_handle'address,
	    length'address,
	    buffer'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END put_segment;

PROCEDURE put_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : SYSTEM.ADDRESS) IS

            user_status         : status_vector;
    BEGIN
        gds_put_slice (
            user_status'address,
            db_handle'address,
            trans_handle'address,
            blob_id'address,
            sdl_length'address,
            sdl'address,
            param_length'address,
            param'address,
            slice_length'address,
            slice);
    END put_slice;

PROCEDURE queue_events (
            db_handle           : database_handle;
            id                  : quad;
            length              : integer;
            events              : string;
            ast                 : long_integer;
            arg                 : quad) IS

            user_status         : status_vector;
    BEGIN
        gds_queue_events (
            user_status'address,
            db_handle'address,
            id'address,
            length'address,
            events'address,
            ast'address,
            arg'address);
    END queue_events;

PROCEDURE receive (
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_receive (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END receive;

PROCEDURE rollback_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_rollback_transaction (
	    user_status'address,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END rollback_transaction;


PROCEDURE send (
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_send (
	    user_status'address,
	    req_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END send;

PROCEDURE start_request (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_request (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_request;


PROCEDURE start_and_send (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 	: status_vector;
    BEGIN
	gds_start_and_send (
	    user_status'address,
	    req_handle'address,
	    trans_handle'address,
	    msg_type'address,
	    msg_length'address,
	    msg,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_and_send;

PROCEDURE start_multiple (
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    teb			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_multiple (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    teb);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_multiple;

    
PROCEDURE start_transaction (
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    db_handle		: database_handle;
	    tpb_length		: integer;
	    tpb_arg		: tpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_transaction (
	    user_status'address,
	    trans_handle'address,
	    count'address,
	    db_handle'address,
	    tpb_length'address,
	    tpb_arg'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_transaction;

    
PROCEDURE unwind_request (
	    req_handle		: request_handle;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_unwind_request (
	    user_status'address,
	    req_handle'address,
	    level'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END unwind_request;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_close (
		user_status'address,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_close;

PROCEDURE dsql_declare (
	statement_name	: string;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_declare (
		user_status'address,
		statement_name'address,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_declare;

PROCEDURE dsql_describe (
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_describe (
		user_status'address,
		statement_name'address,
		descriptor'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_describe;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status'address,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute_immediate (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	sqlcode		: OUT long_integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_fetch (
	sqlcode		: OUT long_integer;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status'address,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_open (
	trans_handle		: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_open (
	trans_handle		: transaction_handle;
	cursor_name		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status'address,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_map;

PROCEDURE compile_sub_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_sub_map (
		user_status'address,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_sub_map;

PROCEDURE drive_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_drive_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END drive_form;

PROCEDURE fetch_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_fetch (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_insert (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
---	if user_status (1) /= 0 then error (user_status); end if;
    END insert_sub_form;

PROCEDURE load_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: integer;
	form_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_load_form (
		user_status'address,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END load_form;

PROCEDURE reset_form (
	win_handle	: window_handle) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_reset_form (
		user_status'address,
		win_handle'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END reset_form;


END interbase;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('VMS_ADA', 'interbase_vms.a',
'WITH SYSTEM;

PACKAGE interbase IS

TYPE status_vector IS ARRAY (0..19) OF INTEGER;
TYPE quad IS ARRAY (0..1) OF INTEGER;
TYPE chars is array (integer range <>) of character;
TYPE blr IS ARRAY (integer range <>) of short_short_integer;
TYPE dpb IS ARRAY (integer range <>) of short_short_integer;
TYPE tpb IS ARRAY (integer range <>) of short_short_integer;

SUBTYPE isc_short		IS short_integer;
SUBTYPE isc_long		IS integer;
SUBTYPE isc_float		IS float;
SUBTYPE isc_double		IS system.g_float;
SUBTYPE double			IS system.g_float;
SUBTYPE database_handle		IS integer;
SUBTYPE request_handle		IS integer;
SUBTYPE transaction_handle	IS integer;
SUBTYPE blob_handle		IS integer;
SUBTYPE form_handle		IS integer;
SUBTYPE map_handle		IS integer;
SUBTYPE window_handle		IS integer;
SUBTYPE menu_handle		IS integer;

TYPE gds_teb_t IS RECORD
    dbb_ptr	: SYSTEM.ADDRESS;
    tpb_len	: integer;
    tpb_ptr	: SYSTEM.ADDRESS;
END RECORD;

TYPE gds_tm IS RECORD
    tm_sec	: integer;
    tm_min	: integer;
    tm_hour	: integer;
    tm_mday	: integer;
    tm_mon	: integer;
    tm_year	: integer;
    tm_wday	: integer;
    tm_yday	: integer;
    tm_isdst	: integer;
END RECORD;

TYPE sqlvar is RECORD
	sqltype		: short_integer;
	sqllen		: short_integer;
	sqldata		: SYSTEM.ADDRESS;
	sqlind		: SYSTEM.ADDRESS;
	sqlname_length	: short_integer;
	sqlname		: chars (1..30);
END RECORD;

TYPE sqlvar_array IS ARRAY (short_integer range <>) of sqlvar;

TYPE sqlda (size: short_integer) IS RECORD
	sqldaid		: chars (1..8);
	sqldabc		: integer;
	sqln		: short_integer := size;
	sqld		: short_integer;
	sqlvars		: sqlvar_array (1..size);
END RECORD;

--- Constants

    gds_true	: CONSTANT integer := 1;
    gds_false  : CONSTANT integer := 0;

--- BLR

$ SET SANS_DOLLAR

$ FORMAT BLR_FORMAT	"    %s : CONSTANT short_short_integer := %d;\n
$ FORMAT DYN_FORMAT	"    %-32s: CONSTANT short_short_integer := %d;\n
$ FORMAT SDL_FORMAT	"    %-32s: CONSTANT short_short_integer := %d;\n
$ FORMAT ERROR_FORMAT	"%-25s: CONSTANT integer := %d;\n
$ FORMAT PB_FORMAT	"    %-29s: CONSTANT short_short_integer := %d;\n
$ FORMAT PYXIS_FORMAT	"%-29s: CONSTANT short_short_integer := %d;\n
$ FORMAT OPT_FORMAT	"%-32s: CONSTANT integer := %d;\n
$ FORMAT SQL_FORMAT	"%-16s: CONSTANT short_integer := %d;\n

$ SYMBOLS BLR DTYPE BLR_FORMAT
$ SYMBOLS BLR MECH SIGNED BLR_FORMAT
 
$ SYMBOLS BLR STATEMENTS BLR_FORMAT
$ SYMBOLS BLR VALUES BLR_FORMAT
 
$ SYMBOLS BLR BOOLEANS BLR_FORMAT
 
$ SYMBOLS BLR RSE BLR_FORMAT
 
$ SYMBOLS BLR JOIN BLR_FORMAT
 
$ SYMBOLS BLR AGGREGATE BLR_FORMAT
$ SYMBOLS BLR NEW BLR_FORMAT

--- Dynamic Data Definition Language operators

--- Version number

$ SYMBOLS DYN MECH SIGNED DYN_FORMAT

--- Operations (may be nested)

$ SYMBOLS DYN OPERATIONS SIGNED DYN_FORMAT

--- View specific stuff

$ SYMBOLS DYN VIEW SIGNED DYN_FORMAT

--- Generic attributes

$ SYMBOLS DYN GENERIC SIGNED DYN_FORMAT

--- Relation specific attributes

$ SYMBOLS DYN RELATION SIGNED DYN_FORMAT

--- Global field specific attributes

$ SYMBOLS DYN GLOBAL SIGNED DYN_FORMAT

--- Local field specific attributes

$ SYMBOLS DYN FIELD SIGNED DYN_FORMAT

--- Index specific attributes

$ SYMBOLS DYN INDEX SIGNED DYN_FORMAT

--- Trigger specific attributes

$ SYMBOLS DYN TRIGGER SIGNED DYN_FORMAT

--- Security Class specific attributes

$ SYMBOLS DYN SECURITY SIGNED DYN_FORMAT

--- Dimension attributes

$ SYMBOLS DYN ARRAY SIGNED DYN_FORMAT

--- File specific attributes

$ SYMBOLS DYN FILES DYN_FORMAT

--- Function specific attributes

$ SYMBOLS DYN FUNCTIONS DYN_FORMAT

--- Generator specific attributes

$ SYMBOLS DYN GENERATOR DYN_FORMAT

--- Array slice description language (SDL)

$ SYMBOLS SDL SDL SIGNED SDL_FORMAT

--- Database parameter block stuff 

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

--- Transaction parameter block stuff 

$ SYMBOLS TPB ITEMS	PB_FORMAT

--- Blob parameter block stuff

$ SYMBOLS BPB ITEMS	PB_FORMAT

--- Common, structural codes

$ SYMBOLS INFO MECH	PB_FORMAT

--- Database information items 

$ SYMBOLS INFO DB	PB_FORMAT

--- Request information items 

$ SYMBOLS INFO REQUEST	PB_FORMAT

--- Blob information items

$ SYMBOLS INFO BLOB	PB_FORMAT

--- Transaction information items 

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

--- Error codes 

$ SYMBOLS ERROR MECH	ERROR_FORMAT

$ ERROR MAJOR		ERROR_FORMAT

--- Minor codes subject to change 

$ ERROR MINOR		ERROR_FORMAT

--- Dynamic SQL datatypes 

$ SYMBOLS SQL DTYPE SQL_FORMAT

--- Forms Package definitions 

--- Map definition block definitions

$ SYMBOLS PYXIS MAP SIGNED  PYXIS_FORMAT

--- Field option flags for display options 

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

--- Field option values following display 

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

--- Pseudo key definitions 

$ SYMBOLS PYXIS KEY  OPT_FORMAT

--- Menu definition stuff 

$ SYMBOLS PYXIS MENU SIGNED PYXIS_FORMAT    blr_text : CONSTANT short_short_integer := 14;                                                

database_error		: EXCEPTION;
null_blob		: CONSTANT quad := (0, 0);
null_tpb		: tpb (0..0);
null_dpb		: dpb (0..0);

PROCEDURE attach_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb);
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: short_integer);

PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: short_integer);

PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : IN OUT blob_handle;
    id                  : integer);

PROCEDURE close_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE create_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb;
    db_type		: short_integer);

PROCEDURE detach_database (
    user_status		: OUT status_vector;
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE encode_date (
    times               : gds_tm;
    date                : IN OUT quad);

PROCEDURE decode_date (
    date                : quad;
    times               : IN OUT gds_tm);

PROCEDURE event_wait (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    length              : short_integer;
    events              : string;
    buffer              : string);

PROCEDURE get_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    actual_length	: OUT short_integer;
    buffer_length	: short_integer;
    buffer		: string);

PROCEDURE get_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT integer);

PROCEDURE open_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    msg_length          : short_integer;
    msg                 : blr);

PROCEDURE print_status (
    user_status		: status_vector);

PROCEDURE put_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    length		: short_integer;
    buffer		: string);

PROCEDURE put_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE que_events (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    id                  : quad;
    length              : short_integer;
    events              : string;
    ast                 : integer;
    arg                 : quad);

PROCEDURE receive (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE print_version (
    db_handle   :  database_handle);

FUNCTION sqlcode (
    user_status		: status_vector) RETURN integer;

PROCEDURE start_and_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE start_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: short_integer);

PROCEDURE start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    db_handle		: database_handle;
    tpb_length		: short_integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    level		: short_integer);

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
    user_status		: OUT status_vector;
    cursor_name		: string);

PROCEDURE dsql_declare (
    user_status		: OUT status_vector;
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    user_status		: OUT status_vector;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: short_integer;
    command		: string);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT integer;
    cursor_name		: string);

PROCEDURE dsql_finish (
    db_handle		: database_handle);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE create_window (
    win_handle		: OUT window_handle;
    name_length		: short_integer;
    file_name		: string;
    width		: IN OUT short_integer;
    height		: IN OUT short_integer);

PROCEDURE delete_window (
    win_handle		: IN OUT window_handle);

PROCEDURE drive_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: short_integer;
    form_name		: string);

FUNCTION menu (
    win_handle		: window_handle;
    men_handle		: menu_handle;
    menu_length		: short_integer;
    menu		: blr) RETURN short_integer;

PROCEDURE pop_window (
    win_handle		: window_handle);

PROCEDURE reset_form (
    user_status		: OUT status_vector;
    win_handle		: window_handle);

PROCEDURE drive_menu (
    win_handle		: IN OUT window_handle;
    men_handle		: IN OUT menu_handle;
    blr_length		: short_integer;
    blr_source		: blr;
    title_length	: short_integer;
    title		: string;
    terminator		: OUT short_integer;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer);

PROCEDURE get_entree (
    men_handle		: menu_handle;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer;
    entree_end		: OUT short_integer);

PROCEDURE initialize_menu (
    men_handle		: IN OUT menu_handle);

PROCEDURE put_entree (
    men_handle		: menu_handle;
    entree_length	: short_integer;
    entree_text		: string;
    entree_value	: integer);




---
--- Same routines, but with automatic error checking
---

PROCEDURE attach_database (
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb);
    
PROCEDURE cancel_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    blb_handle          : IN OUT blob_handle;
    id                  : integer);

PROCEDURE close_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE create_database (
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb;
    db_type		: short_integer);

PROCEDURE detach_database (
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE get_segment (
    completion_code	: OUT integer;
    blb_handle		: blob_handle;
    actual_length	: OUT short_integer;
    buffer_length	: short_integer;
    buffer		: string);

PROCEDURE get_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT integer);

PROCEDURE open_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    trans_handle	: IN OUT transaction_handle;
    msg_length          : short_integer;
    msg                 : blr);

PROCEDURE put_segment (
    blb_handle		: blob_handle;
    length		: short_integer;
    buffer		: string);

PROCEDURE put_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE que_events (
    db_handle           : database_handle;
    id                  : quad;
    length              : short_integer;
    events              : string;
    ast                 : integer;
    arg                 : quad);

PROCEDURE receive (
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE rollback_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE start_and_send (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level			: short_integer);

PROCEDURE start_request (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: short_integer);

PROCEDURE start_multiple (
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    db_handle		: database_handle;
    tpb_length		: short_integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    req_handle		: request_handle;
    level		: short_integer);

PROCEDURE dsql_close (
    cursor_name		: string);

PROCEDURE dsql_declare (
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: short_integer;
    command		: string);

PROCEDURE dsql_fetch (
    sqlcode		: OUT integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    sqlcode		: OUT integer;
    cursor_name		: string);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE compile_sub_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE drive_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: short_integer;
    form_name		: string);

PROCEDURE reset_form (
    win_handle		: window_handle);

END interbase;

PACKAGE BODY interbase IS

---
--- Actual Interbase entrypoints
---

PROCEDURE gds_attach_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb);
    
PROCEDURE gds_blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: short_integer);

PROCEDURE gds_blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE gds_blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: short_integer);

PROCEDURE gds_blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: short_integer);

PROCEDURE gds_cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE gds_cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : IN OUT blob_handle;
    id                  : integer);

PROCEDURE gds_close_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE gds_commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE gds_commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE gds_compile_request (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE gds_compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: short_integer;
    blr_arg		: blr);

PROCEDURE gds_create_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE gds_create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE gds_create_database (
    user_status		: OUT status_vector;
    file_length		: short_integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: short_integer;
    dpb_arg		: dpb;
    db_type		: short_integer);

PROCEDURE gds_ddl (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS);

PROCEDURE gds_decode_date (
    date                : quad;
    times               : IN OUT gds_tm);

PROCEDURE gds_detach_database (
    user_status		: OUT status_vector;
    db_handle		: IN OUT database_handle);

PROCEDURE gds_encode_date (
    times               : gds_tm;
    date                : IN OUT quad);

PROCEDURE gds_event_wait (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    length              : short_integer;
    events              : string;
    buffer              : string);

PROCEDURE gds_get_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    actual_length	: OUT short_integer;
    buffer_length	: short_integer;
    buffer		: string);

PROCEDURE gds_get_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS;
    return_length       : OUT integer);

PROCEDURE gds_open_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE gds_open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : short_integer;
    bpb                 : blr);

PROCEDURE gds_prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE gds_prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    msg_length          : short_integer;
    msg                 : blr);

PROCEDURE gds_print_status (
    user_status		:  status_vector);

PROCEDURE gds_put_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    length		: short_integer;
    buffer		: string);

PROCEDURE gds_put_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : short_integer;
    sdl                 : blr;
    param_length        : short_integer;
    param               : string;
    slice_length        : integer;
    slice               : SYSTEM.ADDRESS);

PROCEDURE gds_que_events (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    id                  : quad;
    length              : short_integer;
    events              : string;
    ast                 : integer;
    arg                 : quad);

PROCEDURE gds_receive (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE gds_rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);
                                                 
PROCEDURE gds_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE gds_version (
    db_handle    : database_handle;
    ptr1	:  integer;
    ptr2	:  SYSTEM.ADDRESS); 

FUNCTION gds_sqlcode (
    user_status		: status_vector) RETURN integer;

PROCEDURE gds_start_and_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: short_integer;
    msg_length		: short_integer;
    msg			: SYSTEM.ADDRESS;
    level		: short_integer);

PROCEDURE gds_start_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: short_integer);

PROCEDURE gds_start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: short_integer;
    db_handle		: database_handle;
    tpb_length		: short_integer;
    tpb_arg		: tpb);
    
PROCEDURE gds_unwind_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    level		: short_integer);

--- Dynamic SQL procedures 

PROCEDURE gds_close (
    user_status		: OUT status_vector;
    cursor_name		: string);

PROCEDURE gds_declare (
    user_status		: OUT status_vector;
    statement_name	: string;
    cursor_name		: string);

PROCEDURE gds_describe (
    user_status		: OUT status_vector;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_dsql_finish (
    db_handle		: database_handle);

PROCEDURE gds_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: short_integer;
    command		: string);

PROCEDURE gds_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: short_integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

--- PYXIS procedures

PROCEDURE pyxis_compile_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE pyxis_compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: short_integer;
    map			: blr);

PROCEDURE pyxis_create_window (
    win_handle		: OUT window_handle;
    name_length		: short_integer;
    file_name		: string;
    width		: IN OUT short_integer;
    height		: IN OUT short_integer);

PROCEDURE pyxis_delete_window (
    win_handle		: IN OUT window_handle);

PROCEDURE pyxis_drive_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE pyxis_fetch (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE pyxis_insert (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE pyxis_load_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: short_integer;
    form_name		: string);

FUNCTION pyxis_menu (
    win_handle		: window_handle;
    men_handle		: menu_handle;
    menu_length		: short_integer;
    menu		: blr) RETURN short_integer;

PROCEDURE pyxis_pop_window (
    win_handle		: window_handle);

PROCEDURE pyxis_reset_form (
    user_status		: OUT status_vector;
    win_handle		: window_handle);

PROCEDURE pyxis_drive_menu (
    win_handle		: IN OUT window_handle;
    men_handle		: IN OUT menu_handle;
    blr_length		: short_integer;
    blr_source		: blr;
    title_length	: short_integer;
    title		: string;
    terminator		: OUT short_integer;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer);

PROCEDURE pyxis_get_entree (
    men_handle		: menu_handle;
    entree_length	: OUT short_integer;
    entree_text		: OUT string;
    entree_value	: OUT integer;
    entree_end		: OUT short_integer);

PROCEDURE pyxis_initialize_menu (
    men_handle		: IN OUT menu_handle);

PROCEDURE pyxis_put_entree (
    men_handle		: menu_handle;
    entree_length	: short_integer;
    entree_text		: string;
    entree_value	: integer);




pragma interface (C, gds_attach_database);

pragma interface (C, gds_blob_display);
pragma interface (C, gds_blob_dump);
pragma interface (C, gds_blob_edit);
pragma interface (C, gds_blob_load);

pragma interface (C, gds_cancel_blob);
pragma interface (C, gds_cancel_events);
pragma interface (C, gds_close);
pragma interface (C, gds_close_blob);
pragma interface (C, gds_commit_retaining);
pragma interface (C, gds_commit_transaction);
pragma interface (C, gds_compile_request);
pragma interface (C, gds_compile_request2);
pragma interface (C, gds_create_blob);
pragma interface (C, gds_create_blob2);
pragma interface (C, gds_create_database);

pragma interface (C, gds_ddl);
pragma interface (C, gds_declare);
pragma interface (C, gds_decode_date);
pragma interface (C, gds_describe);
pragma interface (C, gds_detach_database);
pragma interface (C, gds_dsql_finish);

pragma interface (C, gds_encode_date);
pragma interface (C, gds_event_wait);
pragma interface (C, gds_execute);
pragma interface (C, gds_execute_immediate);

pragma interface (C, gds_fetch);

pragma interface (C, gds_get_segment);
pragma interface (C, gds_get_slice);

pragma interface (C, gds_open);
pragma interface (C, gds_open_blob);
pragma interface (C, gds_open_blob2);

pragma interface (C, gds_prepare);
pragma interface (C, gds_prepare_transaction);
pragma interface (C, gds_prepare_transaction2);
pragma interface (C, gds_print_status);
pragma interface (C, gds_put_segment);
pragma interface (C, gds_put_slice);

pragma interface (C, gds_que_events);

pragma interface (C, gds_receive);
pragma interface (C, gds_rollback_transaction);

pragma interface (C, gds_send);
pragma interface (C, gds_sqlcode);
pragma interface (C, gds_start_and_send);
pragma interface (C, gds_start_multiple);
pragma interface (C, gds_start_request);
pragma interface (C, gds_start_transaction);

pragma interface (C, gds_unwind_request);

pragma interface (C, gds_version);

pragma interface (C, pyxis_compile_map);
pragma interface (C, pyxis_compile_sub_map);
pragma interface (C, pyxis_create_window);

pragma interface (C, pyxis_delete_window);
pragma interface (C, pyxis_drive_form);
pragma interface (C, pyxis_drive_menu);

pragma interface (C, pyxis_fetch);

pragma interface (C, pyxis_get_entree);

pragma interface (C, pyxis_initialize_menu);
pragma interface (C, pyxis_insert);

pragma interface (C, pyxis_load_form);

pragma interface (C, pyxis_menu);

pragma interface (C, pyxis_pop_window);
pragma interface (C, pyxis_put_entree);

pragma interface (C, pyxis_reset_form);



pragma import_procedure (gds_attach_database, "gds_$attach_database",
	mechanism => (reference, value, reference, reference, value, reference));

pragma import_procedure (gds_blob_display, "blob_$display",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (gds_blob_dump, "blob_$dump",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (gds_blob_edit, "blob_$edit",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (gds_blob_load, "blob_$load",
	mechanism => (reference, reference, reference, reference, reference));

pragma import_procedure (gds_cancel_blob, "gds_$cancel_blob",
	mechanism => (reference, reference));

pragma import_procedure (gds_cancel_events, "gds_$cancel_events",
        mechanism => (reference, reference, reference));
pragma import_procedure (gds_close, "gds_$close",
	mechanism => (reference, reference));
pragma import_procedure (gds_close_blob, "gds_$close_blob",
	mechanism => (reference, reference));
pragma import_procedure (gds_commit_retaining, "gds_$commit_retaining",
	mechanism => (reference, reference));
pragma import_procedure (gds_commit_transaction, "gds_$commit_transaction",
	mechanism => (reference, reference));
pragma import_procedure (gds_compile_request, "gds_$compile_request",
	mechanism => (reference, reference, reference, value, reference));
pragma import_procedure (gds_compile_request2, "gds_$compile_request2",
	mechanism => (reference, reference, reference, value, reference));
pragma import_procedure (gds_create_blob, "gds_$create_blob",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (gds_create_blob2, "gds_$create_blob2",
	mechanism => (reference, reference, reference, reference, reference, value, reference));
pragma import_procedure (gds_create_database, "gds_$create_database",
	mechanism => (reference, value, reference, reference, value, reference, value));

pragma import_procedure (gds_ddl, "gds_$ddl",
	mechanism => (reference, reference,  reference, value, value));
pragma import_procedure (gds_declare, "gds_$declare",
	mechanism => (reference, reference, reference));
pragma import_procedure (gds_decode_date, "gds_$decode_date",
        mechanism => (reference, reference));
pragma import_procedure (gds_describe, "gds_$describe",
	mechanism => (reference, reference, value));
pragma import_procedure (gds_detach_database, "gds_$detach_database",
	mechanism => (reference, reference));
pragma import_procedure (gds_dsql_finish, "gds_$dsql_finish",
	mechanism => (reference));

pragma import_procedure (gds_encode_date, "gds_$encode_date",
        mechanism => (reference, reference));
pragma import_procedure (gds_event_wait, "gds_$event_wait",
        mechanism => (reference, reference, value, reference, reference));
pragma import_procedure (gds_execute, "gds_$execute",
	mechanism => (reference, reference, reference, value));
pragma import_procedure (gds_execute_immediate, "gds_$execute_immediate",
	mechanism => (reference, reference, reference, reference, reference));

pragma import_procedure (gds_fetch, "gds_$fetch_a",
	mechanism => (reference, reference, reference, value));

pragma import_procedure (gds_get_segment, "gds_$get_segment",
	mechanism => (reference, reference, reference, value, reference));
pragma import_procedure (gds_get_slice, "gds_$get_slice",
        mechanism => (reference, reference, reference, reference, value, reference,
                      value, reference, value, value, reference));

pragma import_procedure (gds_open, "gds_$open",
	mechanism => (reference, reference, reference, value));
pragma import_procedure (gds_open_blob, "gds_$open_blob",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (gds_open_blob2, "gds_$open_blob2",
	mechanism => (reference, reference, reference, reference, reference, value, reference));

pragma import_procedure (gds_prepare, "gds_$prepare",
	mechanism => (reference, reference, reference, reference, reference, reference, value));
pragma import_procedure (gds_prepare_transaction, "gds_$prepare_transaction",
	mechanism => (reference, reference));
pragma import_procedure (gds_prepare_transaction2, "gds_$prepare_transaction2",
	mechanism => (reference, reference, value, reference));
pragma import_procedure (gds_print_status, "gds_$print_status",
	mechanism => (reference));
pragma import_procedure (gds_put_segment, "gds_$put_segment",
	mechanism => (reference, reference, value, reference));
pragma import_procedure (gds_put_slice, "gds_$put_slice",
        mechanism => (reference, reference, reference, reference, value,
                      reference, value, reference, value, value));

pragma import_procedure (gds_que_events, "gds_$que_events",
	mechanism => (reference, reference, reference, value, reference,
		      value, reference));

pragma import_procedure (gds_receive, "gds_$receive",
	mechanism => (reference, reference, value, value, value, value));
pragma import_procedure (gds_rollback_transaction, "gds_$rollback_transaction",
	mechanism => (reference, reference));

pragma import_function (gds_sqlcode, "gds_$sqlcode",
        mechanism => (reference));
pragma import_procedure (gds_send, "gds_$send",
	mechanism => (reference, reference, value, value, value, value));
pragma import_procedure (gds_start_and_send, "gds_$start_and_send",
	mechanism => (reference, reference, reference, value, value, value, value));
pragma import_procedure (gds_start_multiple, "gds_$start_multiple",
        mechanism => (reference, reference, value, value));
pragma import_procedure (gds_start_request, "gds_$start_request",
	mechanism => (reference, reference, reference, value));
pragma import_procedure (gds_start_transaction, "gds_$start_transaction",
	mechanism => (reference, reference, value, reference, value, reference));

pragma import_procedure (gds_unwind_request, "gds_$unwind_request",
	mechanism => (reference, reference, value));

pragma import_procedure (gds_version, "gds_$version",
	mechanism => (reference, value, reference));

pragma import_procedure (pyxis_compile_map, "pyxis_$compile_map",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (pyxis_compile_sub_map, "pyxis_$compile_sub_map",
	mechanism => (reference, reference, reference, reference, reference));
pragma import_procedure (pyxis_create_window, "pyxis_$create_window",
	mechanism => (reference, reference, reference, reference, reference));

pragma import_procedure (pyxis_delete_window, "pyxis_$delete_window",
	mechanism => (reference));
pragma import_procedure (pyxis_drive_form, "pyxis_$drive_form",
	mechanism => (reference, reference, reference, reference, reference, value, value));
pragma import_procedure (pyxis_drive_menu, "pyxis_$drive_menu",
	mechanism => (reference, reference, reference, reference, reference, reference,
			 reference, reference, reference, reference));

pragma import_procedure (pyxis_fetch, "pyxis_$fetch",
	mechanism => (reference, reference, reference, reference, value));

pragma import_procedure (pyxis_get_entree, "pyxis_$get_entree",
	mechanism => (reference, reference, reference, reference, reference));

pragma import_procedure (pyxis_initialize_menu, "pyxis_$initialize_menu",
	mechanism => (reference));
pragma import_procedure (pyxis_insert, "pyxis_$insert",
	mechanism => (reference, reference, reference, reference, value));

pragma import_procedure (pyxis_load_form, "pyxis_$load_form",
	mechanism => (reference, reference, reference, reference, reference, reference));

pragma import_function (pyxis_menu, "pyxis_$menu",
	mechanism => (reference, reference, reference, reference));

pragma import_procedure (pyxis_pop_window, "pyxis_$pop_window",
	mechanism => (reference));
pragma import_procedure (pyxis_put_entree, "pyxis_$put_entree",
	mechanism => (reference, reference, reference, reference));

pragma import_procedure (pyxis_reset_form, "pyxis_$reset_form",
	mechanism => (reference, reference));



---
--- Interface routines that return status vector
---

PROCEDURE attach_database (
	    user_status		: OUT status_vector;
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb) IS
    BEGIN
	gds_attach_database (
	    user_status,
	    file_length,
	    file_name,
	    db_handle,
	    dpb_length,
	    dpb_arg);
    END attach_database;
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_display (
            blob_id,
            db_handle,
            tra_handle,
            field_name,
            name_length);
    END blob_display;


PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_dump (
            blob_id,
            db_handle,
            tra_handle,
            file_name,
            name_length);
    END blob_dump;


PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_edit (
            blob_id,
            db_handle,
            tra_handle,
            field_name,
            name_length);
    END blob_edit;


PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: short_integer) IS
    BEGIN
        gds_blob_load (
            blob_id,
            db_handle,
            tra_handle,
            file_name,
            name_length);
    END blob_load;


PROCEDURE cancel_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_cancel_blob (
	    user_status,
	    blb_handle);
    END cancel_blob;


PROCEDURE cancel_events (
        user_status         : OUT status_vector;
        blb_handle          : IN OUT blob_handle;
        id                  : integer) IS
    BEGIN
        gds_cancel_events (
            user_status,
            blb_handle,
            id);
    END cancel_events;

PROCEDURE close_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_close_blob (
	    user_status,
	    blb_handle);
    END close_blob;

PROCEDURE commit_retaining (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle) IS
    BEGIN
        gds_commit_retaining (
            user_status,
            trans_handle);
    END commit_retaining;

PROCEDURE commit_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_commit_transaction (
	    user_status,
	    trans_handle);
    END commit_transaction;


PROCEDURE compile_request (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request (
	    user_status,
	    db_handle,
	    req_handle,
	    blr_length,
	    blr_arg);
    END compile_request;


PROCEDURE compile_request2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request2 (
	    user_status,
            db_handle,
	    req_handle,
	    blr_length,
	    blr_arg);
    END compile_request2;


PROCEDURE create_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS
    BEGIN
	gds_create_blob (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id);
    END create_blob;

PROCEDURE create_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS
    BEGIN
	gds_create_blob2 (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id,
            bpb_length,
            bpb);
    END create_blob2;

PROCEDURE create_database (
	    user_status		: OUT status_vector;
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb;
	    db_type		: short_integer) IS
    BEGIN
	gds_create_database (
	    user_status,
	    file_length,
	    file_name,
	    db_handle,
	    dpb_length,
	    dpb_arg,
	    db_type);
    END create_database;


PROCEDURE ddl (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_ddl (
	    user_status,
	    db_handle,
	    trans_handle,
	    msg_length,
	    msg);
    END ddl;


PROCEDURE decode_date (
            date                : quad;
            times               : IN OUT gds_tm) IS
    BEGIN
        gds_decode_date (
            date,
            times);
    END decode_date;


PROCEDURE detach_database (
	    user_status		: OUT status_vector;
	    db_handle		: IN OUT database_handle) IS
    BEGIN
	gds_detach_database (
	    user_status,
	    db_handle);
    END detach_database;

    
PROCEDURE encode_date (
            times               : gds_tm;
            date                : IN OUT quad) IS
    BEGIN
        gds_encode_date (
            times,
            date);
    END encode_date;


PROCEDURE event_wait (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            length              : short_integer;
            events              : string;
            buffer              : string) IS
    BEGIN
        gds_event_wait (
            user_status,
            db_handle,
            length,
            events,
            buffer);
    END event_wait;

PROCEDURE get_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    actual_length	: OUT short_integer;
	    buffer_length	: short_integer;
	    buffer		: string) IS
    BEGIN
	gds_get_segment (
	    user_status,
	    blb_handle,
	    actual_length,
	    buffer_length,
	    buffer);
    END get_segment;


PROCEDURE get_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT integer) IS
    BEGIN
        gds_get_slice (
            user_status,
            db_handle,
            trans_handle,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param,
            slice_length,
            slice,
            return_length);
    END get_slice;

PROCEDURE open_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS
    BEGIN
	gds_open_blob (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id);
    END open_blob;


PROCEDURE open_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS
    BEGIN
	gds_open_blob2 (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id,
            bpb_length,
            bpb);
    END open_blob2;


PROCEDURE prepare_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_prepare_transaction (
	    user_status,
	    trans_handle);
    END prepare_transaction;


PROCEDURE prepare_transaction2 (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle;
            msg_length          : short_integer;
            msg                 : blr) IS
    BEGIN
	gds_prepare_transaction2 (
	    user_status,
	    trans_handle,
            msg_length,
            msg);
    END prepare_transaction2;


PROCEDURE put_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    length		: short_integer;
	    buffer		: string) IS
    BEGIN
	gds_put_segment (
	    user_status,
	    blb_handle,
	    length,
	    buffer);
    END put_segment;


PROCEDURE put_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS) IS
    BEGIN
        gds_put_slice (
            user_status,
            db_handle,
            trans_handle,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param,
            slice_length,
            slice);
    END put_slice;


PROCEDURE print_status (
	user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status);
    end print_status;


PROCEDURE que_events (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            id                  : quad;
            length              : short_integer;
            events              : string;
            ast                 : integer;
            arg                 : quad) IS
    BEGIN
        gds_que_events (
            user_status,
            db_handle,
            id,
            length,
            events,
            ast,
            arg);
    END que_events;


PROCEDURE receive (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_receive (
	    user_status,
	    req_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END receive;

PROCEDURE rollback_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_rollback_transaction (
	    user_status,
	    trans_handle);
    END rollback_transaction;


PROCEDURE print_version (
            db_handle: database_handle) IS

    ptr	: integer := 0;
    BEGIN
        gds_version (
            db_handle,
	    0,
	    ptr'address);
    END print_version;

FUNCTION sqlcode (
	user_status		: status_vector) RETURN integer IS
    BEGIN
	return gds_sqlcode (user_status);
    end sqlcode;


PROCEDURE send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_send (
	    user_status,
	    req_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END send;

PROCEDURE start_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: short_integer) IS
    BEGIN
	gds_start_request (
	    user_status,
	    req_handle,
	    trans_handle,
	    level);
    END start_request;


PROCEDURE start_and_send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS
    BEGIN
	gds_start_and_send (
	    user_status,
	    req_handle,
	    trans_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END start_and_send;

PROCEDURE start_multiple (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    teb			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_start_multiple (
	    user_status,
	    trans_handle,
	    count,
	    teb);
    END start_multiple;

    
PROCEDURE start_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    db_handle		: database_handle;
	    tpb_length		: short_integer;
	    tpb_arg		: tpb) IS
    BEGIN
	gds_start_transaction (
	    user_status,
	    trans_handle,
	    count,
	    db_handle,
	    tpb_length,
	    tpb_arg);
    END start_transaction;

    
PROCEDURE unwind_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    level		: short_integer) IS
    BEGIN
	gds_unwind_request (
	    user_status,
	    req_handle,
	    level);
    END unwind_request;


--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	user_status	: OUT status_vector;
	cursor_name	: string) IS
    BEGIN
	gds_close (
		user_status,
		cursor_name);
    END dsql_close;

PROCEDURE dsql_declare (
	user_status	: OUT status_vector;
	statement_name	: string;
	cursor_name	: string) IS
    BEGIN
	gds_declare (
		user_status,
		statement_name,
		cursor_name);
    END dsql_declare;

PROCEDURE dsql_describe (
	user_status	: OUT status_vector;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_describe (
		user_status,
		statement_name,
		descriptor);
    END dsql_describe;

PROCEDURE dsql_finish (
	db_handle	: database_handle) IS
    BEGIN
	gds_dsql_finish (
		db_handle);
    END dsql_finish;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_execute (
		user_status,
		trans_handle,
		statement_name,
		descriptor);
    END dsql_execute;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string) IS
    BEGIN
	gds_execute (
		user_status,
		trans_handle,
		statement_name,
		null_blob'address);
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: short_integer;
	command		: string) IS
    BEGIN
	gds_execute_immediate (
		user_status,
		db_handle,
		trans_handle,
		command_length,
		command);
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_fetch (
		user_status,
		sqlcode,
		cursor_name,
		descriptor);
    END dsql_fetch;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT integer;
	cursor_name	: string) IS
    BEGIN
	gds_fetch (
		user_status,
		sqlcode,
		cursor_name,
		null_blob'address);
    END dsql_fetch;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle	: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS
    BEGIN
	gds_open (
		user_status,
		trans_handle,
		cursor_name,
		descriptor);
    END dsql_open;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle	: transaction_handle;
	cursor_name		: string) IS
    BEGIN
	gds_open (
		user_status,
		trans_handle,
		cursor_name,
		null_blob'address);
    END dsql_open;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_prepare (
		user_status,
		db_handle,
		trans_handle,
		statement_name,
		command_length,
		command,
		descriptor);
    END dsql_prepare;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string) IS
    BEGIN
	gds_prepare (
		user_status,
		db_handle,
		trans_handle,
		statement_name,
		command_length,
		command,
		null_blob'address);
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_map (
		user_status,
		frm_handle,
		mp_handle,
		map_length,
		map);
    END compile_map;

PROCEDURE compile_sub_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_sub_map (
		user_status,
		frm_handle,
		mp_handle,
		map_length,
		map);
    END compile_sub_map;

PROCEDURE create_window (
	win_handle	: OUT window_handle;
	name_length	: short_integer;
	file_name	: string;
	width		: IN OUT short_integer;
	height		: IN OUT short_integer) IS
    BEGIN
	pyxis_create_window (
		win_handle,
		name_length,
		file_name,
		width,
		height);
    END create_window;

PROCEDURE delete_window (
	win_handle	: IN OUT window_handle) IS
    BEGIN
	pyxis_delete_window (
		win_handle);
    END delete_window;

PROCEDURE drive_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_drive_form (
		user_status,
		db_handle,
		trans_handle,
		win_handle,
		mp_handle,
		input,
		output);
    END drive_form;

PROCEDURE fetch_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_fetch (
		user_status,
		db_handle,
		trans_handle,
		mp_handle,
		output);
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_insert (
		user_status,
		db_handle,
		trans_handle,
		mp_handle,
		input);
    END insert_sub_form;

PROCEDURE load_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: short_integer;
	form_name	: string) IS
    BEGIN
	pyxis_load_form (
		user_status,
		db_handle,
		trans_handle,
		frm_handle,
		name_length,
		form_name);
    END load_form;

FUNCTION menu (
	win_handle	: window_handle;
	men_handle	: menu_handle;
	menu_length	: short_integer;
	menu		: blr) RETURN short_integer IS
    BEGIN
	return pyxis_menu (
		win_handle,
		men_handle,
		menu_length,
		menu);
    END menu;

PROCEDURE pop_window (
	win_handle	: window_handle) IS
    BEGIN
	pyxis_pop_window (
		win_handle);
    END pop_window;

PROCEDURE reset_form (
	user_status	: OUT status_vector;
	win_handle	: window_handle) IS
    BEGIN
	pyxis_reset_form (
		user_status,
		win_handle);
    END reset_form;

PROCEDURE drive_menu (
	win_handle		: IN OUT window_handle;
	men_handle		: IN OUT menu_handle;
	blr_length		: short_integer;
	blr_source		: blr;
	title_length		: short_integer;
	title			: string;
	terminator		: OUT short_integer;
	entree_length		: OUT short_integer;
	entree_text		: OUT string;
	entree_value		: OUT integer) IS
    BEGIN
	pyxis_drive_menu (
	 	win_handle,
		men_handle,
		blr_length,
		blr_source,
		title_length,
		title,
		terminator,
		entree_length,
		entree_text,
		entree_value);
    END drive_menu;

PROCEDURE get_entree (
	men_handle		: menu_handle;
	entree_length		: OUT short_integer;
	entree_text		: OUT string;
	entree_value		: OUT integer;
	entree_end		: OUT short_integer) IS
    BEGIN
	pyxis_get_entree (
		men_handle,
		entree_length,
		entree_text,
		entree_value,
		entree_end);
    END get_entree;

PROCEDURE initialize_menu (
	men_handle		: IN OUT menu_handle) IS
    BEGIN
	pyxis_initialize_menu (
		men_handle);
    END initialize_menu;

PROCEDURE put_entree (
	men_handle		: menu_handle;
	entree_length		: short_integer;
	entree_text		: string;
	entree_value		: integer) IS
    BEGIN
	pyxis_put_entree (
		men_handle,
		entree_length,
		entree_text,
		entree_value);
    END put_entree;


---
--- Internal routines
---

PROCEDURE error (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status);
	RAISE database_error;
    END;


---
--- Routines sans explicit status vector
---

PROCEDURE attach_database (
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_attach_database (
	    user_status,
	    file_length,
	    file_name,
	    db_handle,
	    dpb_length,
	    dpb_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END attach_database;
    
PROCEDURE cancel_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_cancel_blob (
	    user_status,
	    blb_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END cancel_blob;

PROCEDURE cancel_events (
            blb_handle          : IN OUT blob_handle;
            id                  : integer) IS

	user_status 		: status_vector;
    BEGIN
        gds_cancel_events (
            user_status,
            blb_handle,
            id);
        if user_status (1) /= 0 then error (user_status); end if;
    END cancel_events;

PROCEDURE close_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_close_blob (
	    user_status,
	    blb_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END close_blob;

PROCEDURE commit_retaining (
            trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_commit_retaining (
            user_status,
            trans_handle);
        if user_status (1) /= 0 then error (user_status); end if;
    END commit_retaining;

PROCEDURE commit_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_commit_transaction (
	    user_status,
	    trans_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END commit_transaction;


PROCEDURE compile_request (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request (
	    user_status,
	    db_handle,
	    req_handle,
	    blr_length,
	    blr_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request;


PROCEDURE compile_request2 (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: short_integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request2 (
	    user_status,
	    db_handle,
	    req_handle,
	    blr_length,
	    blr_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request2;


PROCEDURE create_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob;


PROCEDURE create_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob2 (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id,
            bpb_length,
            bpb);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob2;


PROCEDURE create_database (
	    file_length		: short_integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: short_integer;
	    dpb_arg		: dpb;
	    db_type		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_database (
	    user_status,
	    file_length,
	    file_name,
	    db_handle,
	    dpb_length,
	    dpb_arg,
	    db_type);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_database;

PROCEDURE ddl (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_ddl (
	    user_status,
	    db_handle,
	    trans_handle,
	    msg_length,
	    msg);
	if user_status (1) /= 0 then error (user_status); end if;
    END ddl;


PROCEDURE detach_database (
	    db_handle		: IN OUT database_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_detach_database (
	    user_status,
	    db_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END detach_database;


PROCEDURE event_wait (
            db_handle           : database_handle;
            length              : short_integer;
            events              : string;
            buffer              : string) IS

	user_status 		: status_vector;
    BEGIN
        gds_event_wait (
            user_status,
            db_handle,
            length,
            events,
            buffer);
    END event_wait;


PROCEDURE get_segment (
	    completion_code	: OUT integer;
	    blb_handle		: blob_handle;
	    actual_length	: OUT short_integer;
	    buffer_length	: short_integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_get_segment (
	    user_status,
	    blb_handle,
	    actual_length,
	    buffer_length,
	    buffer);
	if user_status (1) /= 0 and user_status (1) /= gds_segment and
           user_status (1) /= gds_segstr_eof then 
	    error (user_status); 
	end if;
        completion_code := user_status(1);
    END get_segment;

    
PROCEDURE get_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS;
            return_length       : OUT integer) IS

	user_status 		: status_vector;
    BEGIN
        gds_get_slice (
            user_status,
            db_handle,
            trans_handle,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param,
            slice_length,
            slice,
            return_length);
    END get_slice;

    
PROCEDURE open_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob;


PROCEDURE open_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : short_integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob2 (
	    user_status,
	    db_handle,
	    trans_handle,
	    blb_handle,
	    blob_id,
            bpb_length,
            bpb);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob2;


PROCEDURE prepare_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare_transaction (
	    user_status,
	    trans_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END prepare_transaction;


PROCEDURE prepare_transaction2 (
            trans_handle	: IN OUT transaction_handle;
            msg_length          : short_integer;
            msg                 : blr) IS

        user_status             : status_vector;
    BEGIN
	gds_prepare_transaction2 (
	    user_status,
	    trans_handle,
            msg_length,
            msg);
    END prepare_transaction2;


PROCEDURE put_segment (
	    blb_handle		: blob_handle;
	    length		: short_integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_put_segment (
	    user_status,
	    blb_handle,
	    length,
	    buffer);
	if user_status (1) /= 0 then error (user_status); end if;
    END put_segment;


PROCEDURE put_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : short_integer;
            sdl                 : blr;
            param_length        : short_integer;
            param               : string;
            slice_length        : integer;
            slice               : SYSTEM.ADDRESS) IS

            user_status         : status_vector;
    BEGIN
        gds_put_slice (
            user_status,
            db_handle,
            trans_handle,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param,
            slice_length,
            slice);
    END put_slice;

PROCEDURE que_events (
            db_handle           : database_handle;
            id                  : quad;
            length              : short_integer;
            events              : string;
            ast                 : integer;
            arg                 : quad) IS

            user_status         : status_vector;
    BEGIN
        gds_que_events (
            user_status,
            db_handle,
            id,
            length,
            events,
            ast,
            arg);
    END que_events;

PROCEDURE receive (
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_receive (
	    user_status,
	    req_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END receive;

PROCEDURE rollback_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_rollback_transaction (
	    user_status,
	    trans_handle);
	if user_status (1) /= 0 then error (user_status); end if;
    END rollback_transaction;


PROCEDURE send (
	    req_handle		: request_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_send (
	    user_status,
	    req_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END send;

PROCEDURE start_request (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_request (
	    user_status,
	    req_handle,
	    trans_handle,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_request;


PROCEDURE start_and_send (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: short_integer;
	    msg_length		: short_integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: short_integer) IS

	user_status 	: status_vector;
    BEGIN
	gds_start_and_send (
	    user_status,
	    req_handle,
	    trans_handle,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_and_send;

PROCEDURE start_multiple (
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    teb			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_multiple (
	    user_status,
	    trans_handle,
	    count,
	    teb);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_multiple;

    
PROCEDURE start_transaction (
	    trans_handle	: IN OUT transaction_handle;
	    count		: short_integer;
	    db_handle		: database_handle;
	    tpb_length		: short_integer;
	    tpb_arg		: tpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_transaction (
	    user_status,
	    trans_handle,
	    count,
	    db_handle,
	    tpb_length,
	    tpb_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_transaction;

    
PROCEDURE unwind_request (
	    req_handle		: request_handle;
	    level		: short_integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_unwind_request (
	    user_status,
	    req_handle,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END unwind_request;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_close (
		user_status,
		cursor_name);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_close;

PROCEDURE dsql_declare (
	statement_name	: string;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_declare (
		user_status,
		statement_name,
		cursor_name);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_declare;

PROCEDURE dsql_describe (
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_describe (
		user_status,
		statement_name,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_describe;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status,
		trans_handle,
		statement_name,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status,
		trans_handle,
		statement_name,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: short_integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute_immediate (
		user_status,
		db_handle,
		trans_handle,
		command_length,
		command);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	sqlcode		: OUT integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status,
		sqlcode,
		cursor_name,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_fetch (
	sqlcode		: OUT integer;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status,
		sqlcode,
		cursor_name,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status,
		trans_handle,
		cursor_name,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status,
		trans_handle,
		cursor_name,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status,
		db_handle,
		trans_handle,
		statement_name,
		command_length,
		command,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: short_integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status,
		db_handle,
		trans_handle,
		statement_name,
		command_length,
		command,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_map (
		user_status,
		frm_handle,
		mp_handle,
		map_length,
		map);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_map;

PROCEDURE compile_sub_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: short_integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_sub_map (
		user_status,
		frm_handle,
		mp_handle,
		map_length,
		map);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_sub_map;

PROCEDURE drive_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_drive_form (
		user_status,
		db_handle,
		trans_handle,
		win_handle,
		mp_handle,
		input,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END drive_form;

PROCEDURE fetch_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_fetch (
		user_status,
		db_handle,
		trans_handle,
		mp_handle,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_insert (
		user_status,
		db_handle,
		trans_handle,
		mp_handle,
		input);
---	if user_status (1) /= 0 then error (user_status); end if;
    END insert_sub_form;

PROCEDURE load_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: short_integer;
	form_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_load_form (
		user_status,
		db_handle,
		trans_handle,
		frm_handle,
		name_length,
		form_name);
	if user_status (1) /= 0 then error (user_status); end if;
    END load_form;

PROCEDURE reset_form (
	win_handle	: window_handle) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_reset_form (
		user_status,
		win_handle);
---	if user_status (1) /= 0 then error (user_status); end if;
    END reset_form;


END interbase;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('SUN_ADA', 'interbase_sun.a',
'WITH SYSTEM;

PACKAGE interbase IS

TYPE status_vector IS ARRAY (0..19) of long_integer;
TYPE quad IS ARRAY (0..1) of long_integer;
SUBTYPE chars is string;
TYPE blr IS ARRAY (integer RANGE <>) of long_integer;
TYPE dpb IS ARRAY (integer RANGE <>) of long_integer;
TYPE tpb IS ARRAY (integer RANGE <>) of long_integer;

SUBTYPE database_handle		IS long_integer;
SUBTYPE request_handle		IS long_integer;
SUBTYPE transaction_handle	IS long_integer;
SUBTYPE blob_handle		IS long_integer;
SUBTYPE form_handle		IS long_integer;
SUBTYPE map_handle		IS long_integer;
SUBTYPE window_handle		IS long_integer;
SUBTYPE menu_handle		IS long_integer;

TYPE gds_teb_t IS RECORD
    dbb_ptr	: SYSTEM.ADDRESS;
    tpb_len	: long_integer;
    tpb_ptr	: SYSTEM.ADDRESS;                       
END RECORD;

TYPE gds_tm IS RECORD
    tm_sec	: integer;
    tm_min	: integer;
    tm_hour	: integer;
    tm_mday	: integer;
    tm_mon	: integer;
    tm_year	: integer;
    tm_wday	: integer;
    tm_yday	: integer;
    tm_isdst	: integer;
END RECORD;

TYPE sqlvar is RECORD
	sqltype		: integer;
	sqllen		: integer;
	sqldata		: SYSTEM.ADDRESS;
	sqlind		: SYSTEM.ADDRESS;
	sqlname_length	: integer;
	sqlname		: chars (1..30);
END RECORD;

FOR sqlvar use record at mod 2;
	sqltype		at 0 range 0..15;
	sqllen		at 2 range 0..15;
	sqldata		at 4 range 0..31;
	sqlind		at 8 range 0..31;
	sqlname_length	at 12 range 0..15;
	sqlname		at 14 range 0..239;
END RECORD;

TYPE sqlvar_array IS ARRAY (integer range <>) of sqlvar;

--- Constants

    gds_true	: CONSTANT integer := 1;
    gds_false	: CONSTANT integer := 0;

--- BLR

$ SET SANS_DOLLAR

$ FORMAT BLR_FORMAT	"    %s : CONSTANT integer := %d;\n
$ FORMAT DYN_FORMAT	"    %-32s: CONSTANT integer := %d;\n
$ FORMAT SDL_FORMAT	"    %-32s: CONSTANT integer := %d;\n
$ FORMAT ERROR_FORMAT	"%-25s: CONSTANT long_integer := %d;\n
$ FORMAT PB_FORMAT	"    %-29s: CONSTANT integer := %d;\n
$ FORMAT PYXIS_FORMAT	"%-29s: CONSTANT integer := %d;\n
$ FORMAT OPT_FORMAT	"%-32s: CONSTANT integer := %d;\n
$ FORMAT SQL_FORMAT	"%-16s: CONSTANT integer := %d;\n

$ SYMBOLS BLR DTYPE BLR_FORMAT
$ SYMBOLS BLR MECH BYTE BLR_FORMAT
 
$ SYMBOLS BLR STATEMENTS BLR_FORMAT
$ SYMBOLS BLR VALUES BLR_FORMAT
 
$ SYMBOLS BLR BOOLEANS BLR_FORMAT
 
$ SYMBOLS BLR RSE BLR_FORMAT
 
$ SYMBOLS BLR JOIN BLR_FORMAT
 
$ SYMBOLS BLR AGGREGATE BLR_FORMAT
$ SYMBOLS BLR NEW BLR_FORMAT

--- Dynamic Data Definition Language operators

--- Version number

$ SYMBOLS DYN MECH BYTE DYN_FORMAT

--- Operations (may be nested)

$ SYMBOLS DYN OPERATIONS BYTE DYN_FORMAT

--- View specific stuff

$ SYMBOLS DYN VIEW BYTE DYN_FORMAT

--- Generic attributes

$ SYMBOLS DYN GENERIC BYTE DYN_FORMAT

--- Relation specific attributes

$ SYMBOLS DYN RELATION BYTE DYN_FORMAT

--- Global field specific attributes

$ SYMBOLS DYN GLOBAL BYTE DYN_FORMAT

--- Local field specific attributes

$ SYMBOLS DYN FIELD BYTE DYN_FORMAT

--- Index specific attributes

$ SYMBOLS DYN INDEX BYTE DYN_FORMAT

--- Trigger specific attributes

$ SYMBOLS DYN TRIGGER BYTE DYN_FORMAT

--- Security Class specific attributes

$ SYMBOLS DYN SECURITY BYTE DYN_FORMAT

--- Dimension attributes

$ SYMBOLS DYN ARRAY BYTE DYN_FORMAT

--- File specific attributes

$ SYMBOLS DYN FILES DYN_FORMAT

--- Function specific attributes

$ SYMBOLS DYN FUNCTIONS DYN_FORMAT

--- Generator specific attributes

$ SYMBOLS DYN GENERATOR DYN_FORMAT

--- Array slice description language (SDL)

$ SYMBOLS SDL SDL BYTE SDL_FORMAT

--- Database parameter block stuff 

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

--- Transaction parameter block stuff 

$ SYMBOLS TPB ITEMS	PB_FORMAT

--- Blob parameter block stuff

$ SYMBOLS BPB ITEMS	PB_FORMAT

--- Common, structural codes

$ SYMBOLS INFO MECH	PB_FORMAT

--- Database information items 

$ SYMBOLS INFO DB	PB_FORMAT

--- Request information items 

$ SYMBOLS INFO REQUEST	PB_FORMAT

--- Blob information items

$ SYMBOLS INFO BLOB	PB_FORMAT

--- Transaction information items 

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

--- Error codes 

$ SYMBOLS ERROR MECH	ERROR_FORMAT

$ ERROR MAJOR		ERROR_FORMAT

--- Minor codes subject to change 

$ ERROR MINOR		ERROR_FORMAT

--- Dynamic SQL datatypes 

$ SYMBOLS SQL DTYPE SQL_FORMAT

--- Forms Package definitions 

--- Map definition block definitions

$ SYMBOLS PYXIS MAP BYTE  PYXIS_FORMAT

--- Field option flags for display options 

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

--- Field option values following display 

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

--- Pseudo key definitions 

$ SYMBOLS PYXIS KEY  OPT_FORMAT

--- Menu definition stuff 

$ SYMBOLS PYXIS MENU SIGNED PYXIS_FORMAT    blr_text		: CONSTANT integer := 14;

database_error		: EXCEPTION;
null_blob		: CONSTANT quad := (0, 0);
null_tpb		: CONSTANT tpb (1..2) := (0, 0);
null_dpb		: CONSTANT dpb (1..2) := (0, 0);

PROCEDURE attach_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb);
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
        name_length	: integer);

PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: integer);

PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: integer);

PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
        name_length	: integer);

PROCEDURE cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    user_status		: OUT status_vector;
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE create_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb;
    db_type		: integer);

PROCEDURE detach_database (
    user_status		: OUT status_vector;
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: integer;
    msg			: blr);

PROCEDURE encode_date (
    times               : gds_tm;
    date                : IN OUT quad);

PROCEDURE decode_date (
    date                : quad;
    times               : IN OUT gds_tm);

PROCEDURE event_wait (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    length              : integer;
    events              : string;
    buffer              : string);

PROCEDURE get_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    actual_length	: OUT integer;
    buffer_length	: integer;
    buffer		: string);

PROCEDURE get_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : string;
    return_length       : long_integer);

PROCEDURE open_blob (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    msg_length          : integer;
    msg                 : blr);

PROCEDURE put_segment (
    user_status		: OUT status_vector;
    blb_handle		: blob_handle;
    length		: integer;
    buffer		: string);

PROCEDURE put_slice (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : string);

PROCEDURE queue_events (
    user_status         : OUT status_vector;
    db_handle           : database_handle;
    id                  : quad;
    length              : integer;
    events              : string;
    ast                 : long_integer;
    arg                 : quad);

PROCEDURE receive (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_and_send (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: integer);

PROCEDURE start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    db_handle		: database_handle;
    tpb_length		: integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    user_status		: OUT status_vector;
    req_handle		: request_handle;
    level		: integer);

PROCEDURE print_status (
    user_status		: status_vector);

PROCEDURE print_version (
    db_handle   :  database_handle);

FUNCTION sqlcode (
    user_status		: status_vector) RETURN long_integer;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
    user_status		: OUT status_vector;
    cursor_name		: string);

PROCEDURE dsql_declare (
    user_status		: OUT status_vector;
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    user_status		: OUT status_vector;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: integer;
    command		: string);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT long_integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    user_status		: OUT status_vector;
    sqlcode		: OUT long_integer;
    cursor_name		: string);

PROCEDURE dsql_finish (
    db_handle		: database_handle);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    user_status		: OUT status_vector;
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE create_window (
    win_handle		: OUT window_handle;
    name_length		: integer;
    file_name		: string;
    width		: IN OUT integer;
    height		: IN OUT integer);

PROCEDURE delete_window (
    win_handle		: IN OUT window_handle);

PROCEDURE drive_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    user_status		: OUT status_vector;
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: integer;
    form_name		: string);

FUNCTION menu (
    win_handle		: window_handle;
    men_handle		: menu_handle;
    menu_length		: integer;
    menu		: blr) RETURN integer;

PROCEDURE pop_window (
    win_handle		: window_handle);

PROCEDURE reset_form (
    user_status		: OUT status_vector;
    win_handle		: window_handle);

PROCEDURE drive_menu (
    win_handle		: IN OUT window_handle;
    men_handle		: IN OUT menu_handle;
    blr_length		: integer;
    blr_source		: blr;
    title_length	: integer;
    title		: string;
    terminator		: OUT integer;
    entree_length	: OUT integer;
    entree_text		: OUT string;
    entree_value	: OUT long_integer);

PROCEDURE get_entree (
    men_handle		: menu_handle;
    entree_length	: OUT integer;
    entree_text		: OUT string;
    entree_value	: OUT long_integer;
    entree_end		: OUT integer);

PROCEDURE initialize_menu (
    men_handle		: IN OUT menu_handle);

PROCEDURE put_entree (
    men_handle		: menu_handle;
    entree_length	: integer;
    entree_text		: string;
    entree_value	: long_integer);


---
--- Same routines, but with automatic error checking
---

PROCEDURE attach_database (
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb);
    
PROCEDURE cancel_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE cancel_events (
    blb_handle          : IN OUT blob_handle);

PROCEDURE close_blob (
    blb_handle		: IN OUT blob_handle);

PROCEDURE commit_retaining (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE commit_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE compile_request (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE compile_request2 (
    db_handle		: database_handle;
    req_handle		: IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE create_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE create_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE create_database (
    file_length		: integer;
    file_name		: string;
    db_handle		: IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb;
    db_type		: integer);

PROCEDURE detach_database (
    db_handle		: IN OUT database_handle);
    
PROCEDURE ddl (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    msg_length		: integer;
    msg			: blr);

PROCEDURE get_segment (
    completion_code	: OUT long_integer;
    blb_handle		: blob_handle;
    actual_length	: OUT integer;
    buffer_length	: integer;
    buffer		: string);

PROCEDURE get_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : string;
    return_length       : long_integer);

PROCEDURE open_blob (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE open_blob2 (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    blb_handle		: IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE prepare_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE prepare_transaction2 (
    trans_handle	: IN OUT transaction_handle;
    msg_length          : integer;
    msg                 : blr);

PROCEDURE put_segment (
    blb_handle		: blob_handle;
    length		: integer;
    buffer		: string);

PROCEDURE put_slice (
    db_handle           : database_handle;
    trans_handle        : transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : string;
    slice_length        : long_integer;
    slice               : string);

PROCEDURE queue_events (
    db_handle           : database_handle;
    id                  : quad;
    length              : integer;
    events              : string;
    ast                 : long_integer;
    arg                 : quad);

PROCEDURE receive (
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE rollback_transaction (
    trans_handle	: IN OUT transaction_handle);

PROCEDURE send (
    req_handle		: request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE start_and_send (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level			: integer);

PROCEDURE start_request (
    req_handle		: request_handle;
    trans_handle	: transaction_handle;
    level		: integer);

PROCEDURE start_multiple (
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE start_transaction (
    trans_handle	: IN OUT transaction_handle;
    count		: integer;
    db_handle		: database_handle;
    tpb_length		: integer;
    tpb_arg		: tpb);
    
PROCEDURE unwind_request (
    req_handle		: request_handle;
    level		: integer);

PROCEDURE dsql_close (
    cursor_name		: string);

PROCEDURE dsql_declare (
    statement_name	: string;
    cursor_name		: string);

PROCEDURE dsql_describe (
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_execute (
    trans_handle	: transaction_handle;
    statement_name	: string);

PROCEDURE dsql_execute_immediate (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    command_length	: integer;
    command		: string);

PROCEDURE dsql_fetch (
    sqlcode		: OUT long_integer;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_fetch (
    sqlcode		: OUT long_integer;
    cursor_name		: string);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_open (
    trans_handle	: transaction_handle;
    cursor_name		: string);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE dsql_prepare (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    statement_name	: string;
    command_length	: integer;
    command		: string);

--- PYXIS procedures

PROCEDURE compile_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE compile_sub_map (
    frm_handle		: form_handle;
    mp_handle		: OUT map_handle;
    map_length		: integer;
    map			: blr);

PROCEDURE drive_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    win_handle		: window_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE fetch_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE insert_sub_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    mp_handle		: map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE load_form (
    db_handle		: database_handle;
    trans_handle	: transaction_handle;
    frm_handle		: IN OUT form_handle;
    name_length		: integer;
    form_name		: string);

PROCEDURE reset_form (
    win_handle		: window_handle);

END interbase;

PACKAGE BODY interbase IS

---
--- Actual Interbase entrypoints
---

PROCEDURE gds_attach_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    db_handle		: SYSTEM.ADDRESS; -- IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb);
    
PROCEDURE gds_blob_display (
	blob_id		: quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        field_name	: SYSTEM.ADDRESS; -- string;
	name_length	: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_blob_dump (
	blob_id		: quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_blob_load (
	blob_id		: IN OUT quad;
	db_handle	: SYSTEM.ADDRESS; -- database_handle;
	tra_handle	: SYSTEM.ADDRESS; -- transaction_handle;
        file_name	: SYSTEM.ADDRESS; -- string;
        name_length	: SYSTEM.ADDRESS); -- integer);

PROCEDURE gds_cancel_blob (
    user_status		: OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); -- IN OUT blob_handle;

PROCEDURE gds_cancel_events (
    user_status         : OUT status_vector;
    blb_handle          : SYSTEM.ADDRESS); -- IN OUT blob_handle;

PROCEDURE gds_close_blob (
    user_status		: OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS); -- IN OUT blob_handle;

PROCEDURE gds_commit_retaining (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_commit_transaction (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle;

PROCEDURE gds_compile_request (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr);

PROCEDURE gds_compile_request2 (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    req_handle		: SYSTEM.ADDRESS; -- IN OUT request_handle;
    blr_length		: integer;
    blr_arg		: blr); -- blr;

PROCEDURE gds_create_blob (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; -- IN OUT blob_handle;
    blob_id		: OUT quad);

PROCEDURE gds_create_blob2 (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: OUT quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE gds_create_database (
    user_status		: OUT status_vector;
    file_length		: integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    db_handle		: SYSTEM.ADDRESS; -- IN OUT database_handle;
    dpb_length		: integer;
    dpb_arg		: dpb;
    db_type		: integer);

PROCEDURE gds_detach_database (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS); -- IN OUT database_handle);
    
PROCEDURE gds_ddl (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    msg_length		: integer;
    msg			: blr);

PROCEDURE gds_decode_date (
    date                : quad; -- quad;
    times               : SYSTEM.ADDRESS); -- IN OUT gds_tm);

PROCEDURE gds_encode_date (
    times               : SYSTEM.ADDRESS; -- gds_tm;
    date                : IN OUT quad); -- IN OUT quad);

PROCEDURE gds_event_wait (
    user_status         : OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    length              : integer;
    events              : SYSTEM.ADDRESS; -- string;
    buffer              : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_get_segment (
    user_status		: OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; -- blob_handle;
    actual_length	: SYSTEM.ADDRESS; -- OUT integer;
    buffer_length	: integer;
    buffer		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_get_slice (
    user_status         : OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS; -- string;
    return_length       : long_integer); -- OUT long_integer;

PROCEDURE gds_open_blob (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; -- IN OUT blob_handle;
    blob_id		: quad);

PROCEDURE gds_open_blob2 (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; --database_handle;
    trans_handle	: SYSTEM.ADDRESS; --transaction_handle;
    blb_handle		: SYSTEM.ADDRESS; --IN OUT blob_handle;
    blob_id		: quad;
    bpb_length          : integer;
    bpb                 : blr);

PROCEDURE gds_prepare_transaction (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_prepare_transaction2 (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    msg_length          : integer;
    msg                 : blr); -- blr;

PROCEDURE gds_put_segment (
    user_status		: OUT status_vector;
    blb_handle		: SYSTEM.ADDRESS; -- blob_handle;
    length		: integer;
    buffer		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_put_slice (
    user_status         : OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    trans_handle        : SYSTEM.ADDRESS; -- transaction_handle;
    blob_id             : IN OUT quad;
    sdl_length          : integer;
    sdl                 : blr;
    param_length        : integer;
    param               : SYSTEM.ADDRESS; -- string;
    slice_length        : long_integer;
    slice               : SYSTEM.ADDRESS); -- string;

PROCEDURE gds_queue_events (
    user_status         : OUT status_vector;
    db_handle           : SYSTEM.ADDRESS; -- database_handle;
    id                  : quad;
    length              : integer;
    events              : SYSTEM.ADDRESS; -- string;
    ast                 : long_integer;
    arg                 : quad); -- quad;

PROCEDURE gds_receive (
    user_status		: OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE gds_rollback_transaction (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS); -- IN OUT transaction_handle);

PROCEDURE gds_send (
    user_status		: OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE gds_start_and_send (
    user_status		: OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    msg_type		: integer;
    msg_length		: integer;
    msg			: SYSTEM.ADDRESS;
    level		: integer);

PROCEDURE gds_start_request (
    user_status		: OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    level		: integer);

PROCEDURE gds_start_multiple (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: integer;
    teb			: SYSTEM.ADDRESS);
    
PROCEDURE gds_start_transaction (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- IN OUT transaction_handle;
    count		: integer;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    tpb_length		: integer;
    tpb_arg		: tpb);
    
PROCEDURE gds_unwind_request (
    user_status		: OUT status_vector;
    req_handle		: SYSTEM.ADDRESS; -- request_handle;
    level		: integer);

PROCEDURE gds_print_status (
    user_status		:  status_vector);

PROCEDURE gds_version (
    db_handle    : SYSTEM.ADDRESS;
    ptr1	:  long_integer;
    ptr2	:  SYSTEM.ADDRESS); 

FUNCTION gds_sqlcode (
    user_status		: status_vector) RETURN long_integer;

--- Dynamic SQL procedures 

PROCEDURE gds_close (
    user_status		: OUT status_vector;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_declare (
    user_status		: OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    cursor_name		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_describe (
    user_status		: OUT status_vector;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_dsql_finish (
    db_handle		: SYSTEM.ADDRESS); -- database_handle);

PROCEDURE gds_execute (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_execute_immediate (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    command_length	: SYSTEM.ADDRESS; -- integer;
    command		: SYSTEM.ADDRESS); -- string);

PROCEDURE gds_fetch (
    user_status		: OUT status_vector;
    sqlcode		: SYSTEM.ADDRESS; -- OUT long_integer;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_open (
    user_status		: OUT status_vector;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    cursor_name		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS);

PROCEDURE gds_prepare (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    statement_name	: SYSTEM.ADDRESS; -- string;
    command_length	: SYSTEM.ADDRESS; -- integer;
    command		: SYSTEM.ADDRESS; -- string;
    descriptor		: SYSTEM.ADDRESS);

--- PYXIS procedures

PROCEDURE pyxis_compile_map (
    user_status		: OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- integer;
    map			: blr);

PROCEDURE pyxis_compile_sub_map (
    user_status		: OUT status_vector;
    frm_handle		: SYSTEM.ADDRESS; -- form_handle;
    mp_handle		: SYSTEM.ADDRESS; -- OUT map_handle;
    map_length		: SYSTEM.ADDRESS; -- integer;
    map			: blr);

PROCEDURE pyxis_create_window (
    win_handle		: SYSTEM.ADDRESS; -- OUT window_handle;
    name_length		: SYSTEM.ADDRESS; -- integer;
    file_name		: SYSTEM.ADDRESS; -- string;
    width		: SYSTEM.ADDRESS; -- IN OUT integer;
    height		: SYSTEM.ADDRESS); -- IN OUT integer);

PROCEDURE pyxis_delete_window (
    win_handle		: SYSTEM.ADDRESS); -- IN OUT window_handle);

PROCEDURE pyxis_drive_form (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS;
    output		: SYSTEM.ADDRESS);

PROCEDURE pyxis_fetch (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    output		: SYSTEM.ADDRESS);

PROCEDURE pyxis_insert (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    mp_handle		: SYSTEM.ADDRESS; -- map_handle;
    input		: SYSTEM.ADDRESS);

PROCEDURE pyxis_load_form (
    user_status		: OUT status_vector;
    db_handle		: SYSTEM.ADDRESS; -- database_handle;
    trans_handle	: SYSTEM.ADDRESS; -- transaction_handle;
    frm_handle		: SYSTEM.ADDRESS; -- IN OUT form_handle;
    name_length		: SYSTEM.ADDRESS; -- integer;
    form_name		: SYSTEM.ADDRESS); -- string);

FUNCTION pyxis_menu (
    win_handle		: SYSTEM.ADDRESS; -- window_handle;
    men_handle		: SYSTEM.ADDRESS; -- menu_handle;
    menu_length		: SYSTEM.ADDRESS; -- integer;
    menu		: blr) RETURN integer;

PROCEDURE pyxis_pop_window (
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_reset_form (
    user_status		: OUT status_vector;
    win_handle		: SYSTEM.ADDRESS); -- window_handle);

PROCEDURE pyxis_drive_menu (
    win_handle		: SYSTEM.ADDRESS; -- IN OUT window_handle;
    men_handle		: SYSTEM.ADDRESS; -- IN OUT menu_handle;
    blr_length		: integer;
    blr_source		: blr;
    title_length	: integer;
    title		: SYSTEM.ADDRESS; -- string;
    terminator		: SYSTEM.ADDRESS; -- OUT integer;
    entree_length	: SYSTEM.ADDRESS; -- OUT integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS); -- OUT integer);

PROCEDURE pyxis_get_entree (
    men_handle		: menu_handle;
    entree_length	: SYSTEM.ADDRESS; -- OUT integer;
    entree_text		: SYSTEM.ADDRESS; -- OUT string;
    entree_value	: SYSTEM.ADDRESS; -- OUT long_integer;
    entree_end		: SYSTEM.ADDRESS); -- OUT integer);

PROCEDURE pyxis_initialize_menu (
    men_handle		: SYSTEM.ADDRESS); -- IN OUT menu_handle);

PROCEDURE pyxis_put_entree (
    men_handle		: menu_handle;
    entree_length	: integer;
    entree_text		: SYSTEM.ADDRESS; -- string;
    entree_value	: long_integer);

pragma interface (C, gds_attach_database);
pragma linkname (gds_attach_database, "gds_$attach_database");

pragma interface (C, gds_blob_display);
pragma linkname (gds_blob_display, "blob_$display");
pragma interface (C, gds_blob_dump);
pragma linkname (gds_blob_dump, "blob_$dump");
pragma interface (C, gds_blob_edit);
pragma linkname (gds_blob_edit, "blob_$edit");
pragma interface (C, gds_blob_load);
pragma linkname (gds_blob_load, "blob_$load");

pragma interface (C, gds_cancel_blob);
pragma linkname (gds_cancel_blob, "gds_$cancel_blob");
pragma interface (C, gds_cancel_events);
pragma linkname (gds_cancel_events, "gds_$cancel_events");
pragma interface (C, gds_close);
pragma linkname (gds_close, "gds_$close");
pragma interface (C, gds_close_blob);
pragma linkname (gds_close_blob, "gds_$close_blob");
pragma interface (C, gds_commit_retaining);
pragma linkname (gds_commit_retaining, "gds_$commit_retaining");
pragma interface (C, gds_commit_transaction);
pragma linkname (gds_commit_transaction, "gds_$commit_transaction");
pragma interface (C, gds_compile_request);
pragma linkname (gds_compile_request, "gds_$compile_request");
pragma interface (C, gds_compile_request2);
pragma linkname (gds_compile_request2, "gds_$compile_request2");
pragma interface (C, gds_create_blob);
pragma linkname (gds_create_blob, "gds_$create_blob");
pragma interface (C, gds_create_blob2);
pragma linkname (gds_create_blob2, "gds_$create_blob2");
pragma interface (C, gds_create_database);
pragma linkname (gds_create_database, "gds_$create_database");

pragma interface (C, gds_ddl);
pragma linkname (gds_ddl, "gds_$ddl");
pragma interface (C, gds_declare);
pragma linkname (gds_declare, "gds_$declare");
pragma interface (C, gds_decode_date);
pragma linkname (gds_decode_date, "gds_$decode_date");
pragma interface (C, gds_describe);
pragma linkname (gds_describe, "gds_$describe");
pragma interface (C, gds_detach_database);
pragma linkname (gds_detach_database, "gds_$detach_database");
pragma interface (C, gds_dsql_finish);
pragma linkname (gds_dsql_finish, "gds_$dsql_finish");

pragma interface (C, gds_encode_date);
pragma linkname (gds_encode_date, "gds_$encode_date");
pragma interface (C, gds_event_wait);
pragma linkname (gds_event_wait, "gds_$event_wait");
pragma interface (C, gds_execute);
pragma linkname (gds_execute, "gds_$execute");
pragma interface (C, gds_execute_immediate);
pragma linkname (gds_execute_immediate, "gds_$execute_immediate");

pragma interface (C, gds_fetch);
pragma linkname (gds_fetch, "gds_$fetch_a");

pragma interface (C, gds_get_segment);
pragma linkname (gds_get_segment, "gds_$get_segment");
pragma interface (C, gds_get_slice);
pragma linkname (gds_get_slice, "gds_$get_slice");

pragma interface (C, gds_open);
pragma linkname (gds_open, "gds_$open");
pragma interface (C, gds_open_blob);
pragma linkname (gds_open_blob, "gds_$open_blob");
pragma interface (C, gds_open_blob2);
pragma linkname (gds_open_blob2, "gds_$open_blob2");

pragma interface (C, gds_prepare);
pragma linkname (gds_prepare, "gds_$prepare");
pragma interface (C, gds_prepare_transaction);
pragma linkname (gds_prepare_transaction, "gds_$prepare_transaction");
pragma interface (C, gds_prepare_transaction2);
pragma linkname (gds_prepare_transaction2, "gds_$prepare_transaction2");
pragma interface (C, gds_print_status);
pragma linkname (gds_print_status, "gds_$print_status");
pragma interface (C, gds_put_segment);
pragma linkname (gds_put_segment, "gds_$put_segment");
pragma interface (C, gds_put_slice);
pragma linkname (gds_put_slice, "gds_$put_slice");

pragma interface (C, gds_queue_events);
pragma linkname (gds_queue_events, "gds_$que_events");

pragma interface (C, gds_receive);
pragma linkname (gds_receive, "gds_$receive");
pragma interface (C, gds_rollback_transaction);
pragma linkname (gds_rollback_transaction, "gds_$rollback_transaction");

pragma interface (C, gds_send);
pragma linkname (gds_send, "gds_$send");
pragma interface (C, gds_sqlcode);
pragma linkname (gds_sqlcode, "gds_$sqlcode");
pragma interface (C, gds_start_and_send);
pragma linkname (gds_start_and_send, "gds_$start_and_send");
pragma interface (C, gds_start_multiple);
pragma linkname (gds_start_multiple, "gds_$start_multiple");
pragma interface (C, gds_start_request);
pragma linkname (gds_start_request, "gds_$start_request");
pragma interface (C, gds_start_transaction);
pragma linkname (gds_start_transaction, "gds_$start_transaction");

pragma interface (C, gds_unwind_request);
pragma linkname (gds_unwind_request, "gds_$unwind_request");

pragma interface (C, gds_version);
pragma linkname (gds_version, "gds_$version");

pragma interface (C, pyxis_compile_map);
pragma linkname (pyxis_compile_map, "pyxis_$compile_map");
pragma interface (C, pyxis_compile_sub_map);
pragma linkname (pyxis_compile_sub_map, "pyxis_$compile_sub_map");
pragma interface (C, pyxis_create_window);
pragma linkname (pyxis_create_window, "pyxis_$create_window");
pragma interface (C, pyxis_delete_window);
pragma linkname (pyxis_delete_window, "pyxis_$delete_window");
pragma interface (C, pyxis_drive_form);
pragma linkname (pyxis_drive_form, "pyxis_$drive_form");
pragma interface (C, pyxis_drive_menu);
pragma linkname (pyxis_drive_menu, "pyxis_$drive_menu");
pragma interface (C, pyxis_fetch);
pragma linkname (pyxis_fetch, "pyxis_$fetch");
pragma interface (C, pyxis_get_entree);
pragma linkname (pyxis_get_entree, "pyxis_$get_entree");
pragma interface (C, pyxis_initialize_menu);
pragma linkname (pyxis_initialize_menu, "pyxis_$initialize_menu");
pragma interface (C, pyxis_insert);
pragma linkname (pyxis_insert, "pyxis_$insert");
pragma interface (C, pyxis_load_form);
pragma linkname (pyxis_load_form, "pyxis_$load_form");
pragma interface (C, pyxis_menu);
pragma linkname (pyxis_menu, "pyxis_$menu");
pragma interface (C, pyxis_pop_window);
pragma linkname (pyxis_pop_window, "pyxis_$pop_window");
pragma interface (C, pyxis_put_entree);
pragma linkname (pyxis_put_entree, "pyxis_$put_entree");
pragma interface (C, pyxis_reset_form);
pragma linkname (pyxis_reset_form, "pyxis_$reset_form");


---
--- Interface routines that return status vector
---

PROCEDURE attach_database (
	    user_status		: OUT status_vector;
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb) IS
    BEGIN
	gds_attach_database (
	    user_status,
	    file_length,
	    file_name'address,
	    db_handle'address,
	    dpb_length,
	    dpb_arg);
    END attach_database;
    
PROCEDURE blob_display (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        field_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_display (
            blob_id,
            db_handle'address,
            tra_handle'address,
            field_name'address,
            name_length'address);
    END blob_display;


PROCEDURE blob_dump (
	blob_id		: quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_dump (
            blob_id,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_dump;


PROCEDURE blob_edit (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_edit (
            blob_id,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_edit;


PROCEDURE blob_load (
	blob_id		: IN OUT quad;
	db_handle	: database_handle;
	tra_handle	: transaction_handle;
        file_name	: string;
	name_length	: integer) IS
    BEGIN
        gds_blob_load (
            blob_id,
            db_handle'address,
            tra_handle'address,
            file_name'address,
            name_length'address);
    END blob_load;


PROCEDURE cancel_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_cancel_blob (
	    user_status,
	    blb_handle'address);
    END cancel_blob;

PROCEDURE cancel_events (
        user_status         : OUT status_vector;
        blb_handle          : IN OUT blob_handle) IS
    BEGIN
        gds_cancel_events (
            user_status,
            blb_handle'address);
    END cancel_events;

PROCEDURE close_blob (
	    user_status		: OUT status_vector;
	    blb_handle		: IN OUT blob_handle) IS
    BEGIN
	gds_close_blob (
	    user_status,
	    blb_handle'address);
    END close_blob;

PROCEDURE commit_retaining (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle) IS
    BEGIN
        gds_commit_retaining (
            user_status,
            trans_handle'address);
    END commit_retaining;

PROCEDURE commit_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_commit_transaction (
	    user_status,
	    trans_handle'address);
    END commit_transaction;


PROCEDURE compile_request (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request (
	    user_status,
	    db_handle'address,
	    req_handle'address,
	    blr_length,
	    blr_arg);
    END compile_request;


PROCEDURE compile_request2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS
    BEGIN
	gds_compile_request2 (
	    user_status,
	    db_handle'address,
	    req_handle'address,
	    blr_length,
	    blr_arg);
    END compile_request2;


PROCEDURE create_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS
    BEGIN
	gds_create_blob (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id);
    END create_blob;

PROCEDURE create_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : integer;
            bpb                 : blr) IS
    BEGIN
	gds_create_blob2 (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id,
            bpb_length,
            bpb);
    END create_blob2;

PROCEDURE create_database (
	    user_status		: OUT status_vector;
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb;
	    db_type		: integer) IS
    BEGIN
	gds_create_database (
	    user_status,
	    file_length,
	    file_name'address,
	    db_handle'address,
	    dpb_length,
	    dpb_arg,
	    db_type);
    END create_database;

PROCEDURE ddl (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: integer;
	    msg			: blr) IS
    BEGIN
	gds_ddl (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    msg_length,
	    msg);
    END ddl;

PROCEDURE decode_date (
            date                : quad;
            times               : IN OUT gds_tm) IS
    BEGIN
        gds_decode_date (
            date,
            times'address);
    END decode_date;


PROCEDURE detach_database (
	    user_status		: OUT status_vector;
	    db_handle		: IN OUT database_handle) IS
    BEGIN
	gds_detach_database (
	    user_status,
	    db_handle'address);
    END detach_database;

PROCEDURE encode_date (
            times               : gds_tm;
            date                : IN OUT quad) IS
    BEGIN
        gds_encode_date (
            times'address,
            date);
    END encode_date;


PROCEDURE event_wait (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            length              : integer;
            events              : string;
            buffer              : string) IS
    BEGIN
        gds_event_wait (
            user_status,
            db_handle'address,
            length,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    actual_length	: OUT integer;
	    buffer_length	: integer;
	    buffer		: string) IS
    BEGIN
	gds_get_segment (
	    user_status,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length,
	    buffer'address);
    END get_segment;

PROCEDURE get_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : string;
            return_length       : long_integer) IS
    BEGIN
        gds_get_slice (
            user_status,
            db_handle'address,
            trans_handle'address,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param'address,
            slice_length,
            slice'address,
            return_length);
    END get_slice;

PROCEDURE open_blob (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS
    BEGIN
	gds_open_blob (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id);
    END open_blob;

PROCEDURE open_blob2 (
	    user_status		: OUT status_vector;
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : integer;
            bpb                 : blr) IS
    BEGIN
	gds_open_blob2 (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id,
            bpb_length,
            bpb);
    END open_blob2;

PROCEDURE prepare_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_prepare_transaction (
	    user_status,
	    trans_handle'address);
    END prepare_transaction;

PROCEDURE prepare_transaction2 (
            user_status		: OUT status_vector;
            trans_handle	: IN OUT transaction_handle;
            msg_length          : integer;
            msg                 : blr) IS
    BEGIN
	gds_prepare_transaction2 (
	    user_status,
	    trans_handle'address,
            msg_length,
            msg);
    END prepare_transaction2;

PROCEDURE put_segment (
	    user_status		: OUT status_vector;
	    blb_handle		: blob_handle;
	    length		: integer;
	    buffer		: string) IS
    BEGIN
	gds_put_segment (
	    user_status,
	    blb_handle'address,
	    length,
	    buffer'address);
    END put_segment;

PROCEDURE put_slice (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : string) IS
    BEGIN
        gds_put_slice (
            user_status,
            db_handle'address,
            trans_handle'address,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param'address,
            slice_length,
            slice'address);
    END put_slice;

PROCEDURE queue_events (
            user_status         : OUT status_vector;
            db_handle           : database_handle;
            id                  : quad;
            length              : integer;
            events              : string;
            ast                 : long_integer;
            arg                 : quad) IS
    BEGIN
        gds_queue_events (
            user_status,
            db_handle'address,
            id,
            length,
            events'address,
            ast,
            arg);
    END queue_events;

PROCEDURE receive (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_receive (
	    user_status,
	    req_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END receive;

PROCEDURE rollback_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle) IS
    BEGIN
	gds_rollback_transaction (
	    user_status,
	    trans_handle'address);
    END rollback_transaction;


PROCEDURE send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_send (
	    user_status,
	    req_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END send;

PROCEDURE start_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: integer) IS
    BEGIN
	gds_start_request (
	    user_status,
	    req_handle'address,
	    trans_handle'address,
	    level);
    END start_request;


PROCEDURE start_and_send (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS
    BEGIN
	gds_start_and_send (
	    user_status,
	    req_handle'address,
	    trans_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
    END start_and_send;

PROCEDURE start_multiple (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    teb			: SYSTEM.ADDRESS) IS
    BEGIN
	gds_start_multiple (
	    user_status,
	    trans_handle'address,
	    count,
	    teb);
    END start_multiple;

    
PROCEDURE start_transaction (
	    user_status		: OUT status_vector;
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    db_handle		: database_handle;
	    tpb_length		: integer;
	    tpb_arg		: tpb) IS
    BEGIN
	gds_start_transaction (
	    user_status,
	    trans_handle'address,
	    count,
	    db_handle'address,
	    tpb_length,
	    tpb_arg);
    END start_transaction;

    
PROCEDURE unwind_request (
	    user_status		: OUT status_vector;
	    req_handle		: request_handle;
	    level		: integer) IS
    BEGIN
	gds_unwind_request (
	    user_status,
	    req_handle'address,
	    level);
    END unwind_request;

PROCEDURE print_status (
	user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status);
    end print_status;

PROCEDURE print_version (
            db_handle: database_handle) IS

    ptr	: long_integer := 0;
    BEGIN
        gds_version (
            db_handle'address,
	    0,
	    ptr'address);
    END print_version;

FUNCTION sqlcode (
	user_status		: status_vector) RETURN long_integer IS
    BEGIN
	return gds_sqlcode (user_status);
    end sqlcode;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	user_status	: OUT status_vector;
	cursor_name	: string) IS
    BEGIN
	gds_close (
		user_status,
		cursor_name'address);
    END dsql_close;

PROCEDURE dsql_declare (
	user_status	: OUT status_vector;
	statement_name	: string;
	cursor_name	: string) IS
    BEGIN
	gds_declare (
		user_status,
		statement_name'address,
		cursor_name'address);
    END dsql_declare;

PROCEDURE dsql_describe (
	user_status	: OUT status_vector;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_describe (
		user_status,
		statement_name'address,
		descriptor);
    END dsql_describe;

PROCEDURE dsql_finish (
	db_handle	: database_handle) IS
    BEGIN
	gds_dsql_finish (
		db_handle'address);
    END dsql_finish;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_execute (
		user_status,
		trans_handle'address,
		statement_name'address,
		descriptor);
    END dsql_execute;

PROCEDURE dsql_execute (
	user_status	: OUT status_vector;
	trans_handle	: transaction_handle;
	statement_name	: string) IS
    BEGIN
	gds_execute (
		user_status,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: integer;
	command		: string) IS
    BEGIN
	gds_execute_immediate (
		user_status,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT long_integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_fetch (
		user_status,
		sqlcode'address,
		cursor_name'address,
		descriptor);
    END dsql_fetch;

PROCEDURE dsql_fetch (
	user_status	: OUT status_vector;
	sqlcode		: OUT long_integer;
	cursor_name	: string) IS
    BEGIN
	gds_fetch (
		user_status,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
    END dsql_fetch;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle		: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS
    BEGIN
	gds_open (
		user_status,
		trans_handle'address,
		cursor_name'address,
		descriptor);
    END dsql_open;

PROCEDURE dsql_open (
	user_status		: OUT status_vector;
	trans_handle		: transaction_handle;
	cursor_name		: string) IS
    BEGIN
	gds_open (
		user_status,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
    END dsql_open;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS
    BEGIN
	gds_prepare (
		user_status,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
    END dsql_prepare;

PROCEDURE dsql_prepare (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string) IS
    BEGIN
	gds_prepare (
		user_status,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_map  (
		user_status,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map);
    END compile_map;

PROCEDURE compile_sub_map (
	user_status	: OUT status_vector;
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS
    BEGIN
	pyxis_compile_sub_map (
		user_status,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map);
    END compile_sub_map;

PROCEDURE create_window (
	win_handle	: OUT window_handle;
	name_length	: integer;
	file_name	: string;
	width		: IN OUT integer;
	height		: IN OUT integer) IS
    BEGIN
	pyxis_create_window (
		win_handle'address,
		name_length'address,
		file_name'address,
		width'address,
		height'address);
    END create_window;

PROCEDURE delete_window (
	win_handle	: IN OUT window_handle) IS
    BEGIN
	pyxis_delete_window (
		win_handle'address);
    END delete_window;

PROCEDURE drive_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_drive_form (
		user_status,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
    END drive_form;

PROCEDURE fetch_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_fetch (
		user_status,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS
    BEGIN
	pyxis_insert (
		user_status,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
    END insert_sub_form;

PROCEDURE load_form (
	user_status	: OUT status_vector;
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: integer;
	form_name	: string) IS
    BEGIN
	pyxis_load_form (
		user_status,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
    END load_form;

FUNCTION menu (
	win_handle	: window_handle;
	men_handle	: menu_handle;
	menu_length	: integer;
	menu		: blr) RETURN integer IS
    BEGIN
	return pyxis_menu (
		win_handle'address,
		men_handle'address,
		menu_length'address,
		menu);
    END menu;

PROCEDURE pop_window (
	win_handle	: window_handle) IS
    BEGIN
	pyxis_pop_window (
		win_handle'address);
    END pop_window;

PROCEDURE reset_form (
	user_status	: OUT status_vector;
	win_handle	: window_handle) IS
    BEGIN
	pyxis_reset_form (
		user_status,
		win_handle'address);
    END reset_form;

PROCEDURE drive_menu (
	win_handle		: IN OUT window_handle;
	men_handle		: IN OUT menu_handle;
	blr_length		: integer;
	blr_source		: blr;
	title_length		: integer;
	title			: string;
	terminator		: OUT integer;
	entree_length		: OUT integer;
	entree_text		: OUT string;
	entree_value		: OUT long_integer) IS
    BEGIN
	pyxis_drive_menu (
	 	win_handle'address,
		men_handle'address,
		blr_length,
		blr_source,
		title_length,
		title'address,
		terminator'address,
		entree_length'address,
		entree_text'address,
		entree_value'address);
    END drive_menu;

PROCEDURE get_entree (
	men_handle		: menu_handle;
	entree_length		: OUT integer;
	entree_text		: OUT string;
	entree_value		: OUT long_integer;
	entree_end		: OUT integer) IS
    BEGIN
	pyxis_get_entree (
		men_handle,
		entree_length'address,
		entree_text'address,
		entree_value'address,
		entree_end'address);
    END get_entree;

PROCEDURE initialize_menu (
	men_handle		: IN OUT menu_handle) IS
    BEGIN
	pyxis_initialize_menu (
		men_handle'address);
    END initialize_menu;

PROCEDURE put_entree (
	men_handle		: menu_handle;
	entree_length		: integer;
	entree_text		: string;
	entree_value		: long_integer) IS
    BEGIN
	pyxis_put_entree (
		men_handle,
		entree_length,
		entree_text'address,
		entree_value);
    END put_entree;

---
--- Internal routines
---

PROCEDURE error (
	    user_status		: status_vector) IS
    BEGIN
	gds_print_status (user_status);
	RAISE database_error;
    END;

---
--- Routines sans explicit status vector
---

PROCEDURE attach_database (
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_attach_database (
	    user_status,
	    file_length,
	    file_name'address,
	    db_handle'address,
	    dpb_length,
	    dpb_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END attach_database;
    
PROCEDURE cancel_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_cancel_blob (
	    user_status,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END cancel_blob;

PROCEDURE cancel_events (
            blb_handle          : IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_cancel_events (
            user_status,
            blb_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END cancel_events;

PROCEDURE close_blob (
	    blb_handle		: IN OUT blob_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_close_blob (
	    user_status,
	    blb_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END close_blob;

PROCEDURE commit_retaining (
            trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
        gds_commit_retaining (
            user_status,
            trans_handle'address);
        if user_status (1) /= 0 then error (user_status); end if;
    END commit_retaining;

PROCEDURE commit_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_commit_transaction (
	    user_status,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END commit_transaction;


PROCEDURE compile_request (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request (
	    user_status,
	    db_handle'address,
	    req_handle'address,
	    blr_length,
	    blr_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request;


PROCEDURE compile_request2 (
	    db_handle		: database_handle;
	    req_handle		: IN OUT request_handle;
	    blr_length		: integer;
	    blr_arg		: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_compile_request2 (
	    user_status,
	    db_handle'address,
	    req_handle'address,
	    blr_length,
	    blr_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END compile_request2;


PROCEDURE create_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob;

PROCEDURE create_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: OUT quad;
            bpb_length          : integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_blob2 (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id,
            bpb_length,
            bpb);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_blob2;

PROCEDURE create_database (
	    file_length		: integer;
	    file_name		: string;
	    db_handle		: IN OUT database_handle;
	    dpb_length		: integer;
	    dpb_arg		: dpb;
	    db_type		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_create_database (
	    user_status,
	    file_length,
	    file_name'address,
	    db_handle'address,
	    dpb_length,
	    dpb_arg,
	    db_type);
	if user_status (1) /= 0 then error (user_status); end if;
    END create_database;

PROCEDURE ddl (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    msg_length		: integer;
	    msg			: blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_ddl (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    msg_length,
	    msg);
	if user_status (1) /= 0 then error (user_status); end if;
    END ddl;

PROCEDURE detach_database (
	    db_handle		: IN OUT database_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_detach_database (
	    user_status,
	    db_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END detach_database;

    
PROCEDURE event_wait (
            db_handle           : database_handle;
            length              : integer;
            events              : string;
            buffer              : string) IS

	user_status 		: status_vector;
    BEGIN
        gds_event_wait (
            user_status,
            db_handle'address,
            length,
            events'address,
            buffer'address);
    END event_wait;

PROCEDURE get_segment (
	    completion_code	: OUT long_integer;
	    blb_handle		: blob_handle;
	    actual_length	: OUT integer;
	    buffer_length	: integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_get_segment (
	    user_status,
	    blb_handle'address,
	    actual_length'address,
	    buffer_length,
	    buffer'address);
	if user_status (1) /= 0 and user_status (1) /= gds_segment and
           user_status (1) /= gds_segstr_eof then 
	    error (user_status); 
	end if;
        completion_code := user_status(1);
    END get_segment;

    
PROCEDURE get_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : string;
            return_length       : long_integer) IS

	user_status 		: status_vector;
    BEGIN
        gds_get_slice (
            user_status,
            db_handle'address,
            trans_handle'address,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param'address,
            slice_length,
            slice'address,
            return_length);
    END get_slice;

    
PROCEDURE open_blob (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob;

PROCEDURE open_blob2 (
	    db_handle		: database_handle;
	    trans_handle	: transaction_handle;
	    blb_handle		: IN OUT blob_handle;
	    blob_id		: quad;
            bpb_length          : integer;
            bpb                 : blr) IS

	user_status 		: status_vector;
    BEGIN
	gds_open_blob2 (
	    user_status,
	    db_handle'address,
	    trans_handle'address,
	    blb_handle'address,
	    blob_id,
            bpb_length,
            bpb);
	if user_status (1) /= 0 then error (user_status); end if;
    END open_blob2;

PROCEDURE prepare_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare_transaction (
	    user_status,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END prepare_transaction;


PROCEDURE prepare_transaction2 (
            trans_handle	: IN OUT transaction_handle;
            msg_length          : integer;
            msg                 : blr) IS

        user_status             : status_vector;
    BEGIN
	gds_prepare_transaction2 (
	    user_status,
	    trans_handle'address,
            msg_length,
            msg);
    END prepare_transaction2;

PROCEDURE put_segment (
	    blb_handle		: blob_handle;
	    length		: integer;
	    buffer		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_put_segment (
	    user_status,
	    blb_handle'address,
	    length,
	    buffer'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END put_segment;

PROCEDURE put_slice (
            db_handle           : database_handle;
            trans_handle        : transaction_handle;
            blob_id             : IN OUT quad;
            sdl_length          : integer;
            sdl                 : blr;
            param_length        : integer;
            param               : string;
            slice_length        : long_integer;
            slice               : string) IS

            user_status         : status_vector;
    BEGIN
        gds_put_slice (
            user_status,
            db_handle'address,
            trans_handle'address,
            blob_id,
            sdl_length,
            sdl,
            param_length,
            param'address,
            slice_length,
            slice'address);
    END put_slice;

PROCEDURE queue_events (
            db_handle           : database_handle;
            id                  : quad;
            length              : integer;
            events              : string;
            ast                 : long_integer;
            arg                 : quad) IS

            user_status         : status_vector;
    BEGIN
        gds_queue_events (
            user_status,
            db_handle'address,
            id,
            length,
            events'address,
            ast,
            arg);
    END queue_events;

PROCEDURE receive (
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_receive (
	    user_status,
	    req_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END receive;

PROCEDURE rollback_transaction (
	    trans_handle	: IN OUT transaction_handle) IS

	user_status 		: status_vector;
    BEGIN
	gds_rollback_transaction (
	    user_status,
	    trans_handle'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END rollback_transaction;


PROCEDURE send (
	    req_handle		: request_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_send (
	    user_status,
	    req_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END send;

PROCEDURE start_request (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_request (
	    user_status,
	    req_handle'address,
	    trans_handle'address,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_request;


PROCEDURE start_and_send (
	    req_handle		: request_handle;
	    trans_handle	: transaction_handle;
	    msg_type		: integer;
	    msg_length		: integer;
	    msg			: SYSTEM.ADDRESS;
	    level		: integer) IS

	user_status 	: status_vector;
    BEGIN
	gds_start_and_send (
	    user_status,
	    req_handle'address,
	    trans_handle'address,
	    msg_type,
	    msg_length,
	    msg,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_and_send;

PROCEDURE start_multiple (
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    teb			: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_multiple (
	    user_status,
	    trans_handle'address,
	    count,
	    teb);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_multiple;

    
PROCEDURE start_transaction (
	    trans_handle	: IN OUT transaction_handle;
	    count		: integer;
	    db_handle		: database_handle;
	    tpb_length		: integer;
	    tpb_arg		: tpb) IS

	user_status 		: status_vector;
    BEGIN
	gds_start_transaction (
	    user_status,
	    trans_handle'address,
	    count,
	    db_handle'address,
	    tpb_length,
	    tpb_arg);
	if user_status (1) /= 0 then error (user_status); end if;
    END start_transaction;

    
PROCEDURE unwind_request (
	    req_handle		: request_handle;
	    level		: integer) IS

	user_status 		: status_vector;
    BEGIN
	gds_unwind_request (
	    user_status,
	    req_handle'address,
	    level);
	if user_status (1) /= 0 then error (user_status); end if;
    END unwind_request;

--- Dynamic SQL procedures 

PROCEDURE dsql_close (
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_close (
		user_status,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_close;

PROCEDURE dsql_declare (
	statement_name	: string;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_declare (
		user_status,
		statement_name'address,
		cursor_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_declare;

PROCEDURE dsql_describe (
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_describe (
		user_status,
		statement_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_describe;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status,
		trans_handle'address,
		statement_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute (
	trans_handle	: transaction_handle;
	statement_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute (
		user_status,
		trans_handle'address,
		statement_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute;

PROCEDURE dsql_execute_immediate (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	command_length	: integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_execute_immediate (
		user_status,
		db_handle'address,
		trans_handle'address,
		command_length'address,
		command'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_execute_immediate;

PROCEDURE dsql_fetch (
	sqlcode		: OUT long_integer;
	cursor_name	: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status,
		sqlcode'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_fetch (
	sqlcode		: OUT long_integer;
	cursor_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_fetch (
		user_status,
		sqlcode'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_fetch;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string;
	descriptor		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status,
		trans_handle'address,
		cursor_name'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_open (
	trans_handle	: transaction_handle;
	cursor_name		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_open (
		user_status,
		trans_handle'address,
		cursor_name'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_open;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string;
	descriptor	: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		descriptor);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

PROCEDURE dsql_prepare (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	statement_name	: string;
	command_length	: integer;
	command		: string) IS

	user_status 		: status_vector;
    BEGIN
	gds_prepare (
		user_status,
		db_handle'address,
		trans_handle'address,
		statement_name'address,
		command_length'address,
		command'address,
		null_blob'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END dsql_prepare;

--- PYXIS procedures

PROCEDURE compile_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_map (
		user_status,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_map;

PROCEDURE compile_sub_map (
	frm_handle	: form_handle;
	mp_handle	: OUT map_handle;
	map_length	: integer;
	map		: blr) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_compile_sub_map (
		user_status,
		frm_handle'address,
		mp_handle'address,
		map_length'address,
		map);
---	if user_status (1) /= 0 then error (user_status); end if;
    END compile_sub_map;

PROCEDURE drive_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	win_handle	: window_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_drive_form (
		user_status,
		db_handle'address,
		trans_handle'address,
		win_handle'address,
		mp_handle'address,
		input,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END drive_form;

PROCEDURE fetch_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	output		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_fetch (
		user_status,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		output);
---	if user_status (1) /= 0 then error (user_status); end if;
    END fetch_sub_form;

PROCEDURE insert_sub_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	mp_handle	: map_handle;
	input		: SYSTEM.ADDRESS) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_insert (
		user_status,
		db_handle'address,
		trans_handle'address,
		mp_handle'address,
		input);
---	if user_status (1) /= 0 then error (user_status); end if;
    END insert_sub_form;

PROCEDURE load_form (
	db_handle	: database_handle;
	trans_handle	: transaction_handle;
	frm_handle	: IN OUT form_handle;
	name_length	: integer;
	form_name	: string) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_load_form (
		user_status,
		db_handle'address,
		trans_handle'address,
		frm_handle'address,
		name_length'address,
		form_name'address);
	if user_status (1) /= 0 then error (user_status); end if;
    END load_form;

PROCEDURE reset_form (
	win_handle	: window_handle) IS

	user_status 		: status_vector;
    BEGIN
	pyxis_reset_form (
		user_status,
		win_handle'address);
---	if user_status (1) /= 0 then error (user_status); end if;
    END reset_form;


END interbase;');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('PLI', 'gds.pli',
'/*
 *	PROGRAM:	PLI preprocessor
 *	MODULE:		gds.pli
 *	DESCRIPTION:	BLR constants
 *
 * copyright (c) 1984, 1990 by Interbase Software Corporation
 */

%REPLACE GDS_$TRUE BY 1;
%REPLACE GDS_$FALSE BY 0;

$ FORMAT BLR_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT DYN_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT SDL_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT ERROR_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT PB_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT PYXIS_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT OPT_FORMAT	"%%REPLACE %-32s BY %d;\n
$ FORMAT SQL_FORMAT	"%%REPLACE %-32s BY %%d;\n

$ SYMBOLS BLR DTYPE BLR_FORMAT
$ SYMBOLS BLR MECH SIGNED BLR_FORMAT
 
$ SYMBOLS BLR STATEMENTS BLR_FORMAT
$ SYMBOLS BLR VALUES BLR_FORMAT
 
$ SYMBOLS BLR BOOLEANS BLR_FORMAT
 
$ SYMBOLS BLR RSE BLR_FORMAT
 
$ SYMBOLS BLR JOIN BLR_FORMAT
 
$ SYMBOLS BLR AGGREGATE BLR_FORMAT
$ SYMBOLS BLR NEW BLR_FORMAT

/* Array slice description language (SDL) */

$ SYMBOLS SDL SDL SDL_FORMAT

%REPLACE dsc$k_dtype_t			BY 14;
%REPLACE dsc$k_dtype_vt			BY 37;
%REPLACE dsc$k_dtype_w			BY 7;
%REPLACE dsc$k_dtype_l			BY 8;
%REPLACE dsc$k_dtype_q			BY 9;
%REPLACE dsc$k_dtype_f			BY 10;
%REPLACE dsc$k_dtype_g			BY 27;
%REPLACE dsc$k_dtype_adt		BY 35;

%REPLACE rdb$k_dpb_cdd_pathname		BY 1;
%REPLACE rdb$k_dpb_allocation		BY 2;
%REPLACE rdb$k_dpb_aij			BY 3;
%REPLACE rdb$k_dpb_page_size		BY 4;
%REPLACE rdb$k_dpb_num_buff		BY 5;
%REPLACE rdb$k_dpb_buffer_length	BY 6;
%REPLACE rdb$k_dpb_debug		BY 7;
%REPLACE rdb$k_dpb_garbage_collect	BY 8;
%REPLACE rdb$k_dpb_verify		BY 9;
%REPLACE rdb$k_dpb_sweep		BY 10;
%REPLACE rdb$k_dpb_enable_journal	BY 11;
%REPLACE rdb$k_dpb_disable_journal	BY 12;
%REPLACE rdb$k_dpb_dbkey_scope		BY 13;
%REPLACE rdb$k_dpb_number_of_users	BY 14;

%REPLACE rdb$k_db_type_default		BY 0;
%REPLACE rdb$k_db_type_vms		BY 1;
%REPLACE rdb$k_db_type_eln		BY 2;
%REPLACE rdb$k_db_type_gds		BY -1;

/* Database parameter block stuff */

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

/* Bit assignments in RDB$SYSTEM_FLAG */

$ SYMBOLS RDB FLAG	PB_FORMAT

/* Transaction parameter blob stuff */

$ SYMBOLS TPB ITEMS	PB_FORMAT

/* Blob parameter block stuff */

$ SYMBOLS BPB ITEMS	PB_FORMAT

/* Information call declarations */

/* Common, structural codes */

$ SYMBOLS INFO MECH	PB_FORMAT

/* Database information items */

$ SYMBOLS INFO DB	PB_FORMAT

/* Request information items */

$ SYMBOLS INFO REQUEST	PB_FORMAT

/* Blob information items */

$ SYMBOLS INFO BLOB	PB_FORMAT

/* Transaction information items */

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

/* Error codes */

$ SYMBOLS ERROR MECH	ERROR_FORMAT

$ ERROR MAJOR		ERROR_FORMAT

/* Minor codes subject to change */

$ ERROR MINOR		ERROR_FORMAT

DECLARE GDS_$SQLCODE ENTRY(
	(20) FIXED)	/* status vector */
	RETURNS (FIXED);

DECLARE GDS_$ATTACH_DATABASE ENTRY(
	ANY VALUE,			/* status vector */
	FIXED BINARY (15) VALUE,	/* file name length */
	ANY,				/* file name */
	ANY,				/* database handle */
	FIXED BINARY (15) VALUE,	/* dpb length */
	ANY VALUE);			/* dpb */

DECLARE GDS_$CANCEL_BLOB ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* blob handle */

DECLARE GDS_$CANCEL_EVENTS ENTRY(
        ANY VALUE,       /* status vector */
        ANY,             /* database handle */
        ANY);            /* id */

DECLARE GDS_$CLOSE_BLOB ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* blob handle */

DECLARE GDS_$COMMIT_RETAINING ENTRY(
        ANY VALUE,      /* status vector */
        ANY);           /* transaction handle */

DECLARE GDS_$COMMIT_TRANSACTION ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* transaction handle */

DECLARE GDS_$COMPILE_REQUEST ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* database handle */
	ANY,		/* request handle */
	FIXED BINARY (15) VALUE,	/* blr length */
	ANY);		/* blr */

DECLARE GDS_$COMPILE_REQUEST2 ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* database handle */
	ANY,		/* request handle */
	FIXED BINARY (15) VALUE,	/* blr length */
	ANY);		/* blr */

DECLARE GDS_$CREATE_BLOB ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* blob handle */
	(2) FIXED);		/* blob id */

DECLARE GDS_$CREATE_BLOB2 ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* blob handle */
	(2) FIXED,	/* blob id */
        FIXED BINARY (15) VALUE,        /* blob parameter block length */
        ANY);           /* blob parameter block */

DECLARE GDS_$CREATE_DATABASE ENTRY(
	ANY VALUE,			/* status vector */
	FIXED BINARY (15) VALUE,	/* file name length */
	ANY,				/* file name */
	ANY,				/* database handle */
	FIXED BINARY (15) VALUE,	/* dpb length */
	ANY  VALUE,			/* dpb */
	FIXED BINARY (15) VALUE);	/* database type */

DECLARE GDS_$DDL ENTRY(
	ANY VALUE,			/* status vector */
	ANY,				/* database handle */
	ANY,				/* transaction handle */
	FIXED BINARY (15) VALUE,	/* mblr length */
	ANY);				/* mblr */

DECLARE GDS_$DETACH_DATABASE ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* database handle */

DECLARE GDS_$DROP_DATABASE ENTRY(
	ANY VALUE,			/* status vector */
	FIXED BINARY (15) VALUE,	/* file name length */
	ANY,				/* file name */
	FIXED BINARY (15) VALUE);	/* database type */

DECLARE GDS_$EVENT_WAIT ENTRY(
        ANY VALUE,      /* status vector */
        ANY,            /* database handle */
        ANY,            /* length */
        ANY,            /* events */
        ANY);           /* buffer */

DECLARE GDS_$GET_SEGMENT ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* blob handle */
	ANY,	/* segment length */
	FIXED BINARY (15) VALUE,	/* buffer length */
	ANY)		/* buffer */
	RETURNS (FIXED);

DECLARE GDS_$GET_SLICE ENTRY(
        ANY VALUE,      /* status vector */
        ANY,            /* database handle */
        ANY,            /* transaction handle */
        ANY,            /* array (blob) id */
        FIXED BINARY (15),  /* sdl length */
        ANY,            /* sdl */
        FIXED BINARY (15),  /* parameter length */
        ANY,            /* parameter */
        FIXED BINARY (31),  /* slice length */
        ANY,            /* slice */
        FIXED BINARY (31));  /* return length */

DECLARE GDS_$OPEN_BLOB ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* blob handle */
	(2) FIXED);		/* blob id */

DECLARE GDS_$OPEN_BLOB2 ENTRY(
	ANY VALUE,	   /* status vector */
	ANY,		   /* database handle */
	ANY,		   /* transaction handle */
	ANY,		   /* blob handle */
	(2) FIXED,	   /* blob id */
        FIXED BINARY (15), /* blob parameter block length */
        ANY);              /* blob parameter block */

DECLARE GDS_$PREPARE_TRANSACTION ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* transaction handle */

DECLARE GDS_$PREPARE_TRANSACTION2 ENTRY(
	ANY VALUE,	   /* status vector */
	ANY,		   /* transaction handle */
        FIXED BINARY (15),
        ANY);

DECLARE GDS_$PUT_SEGMENT ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* blob handle */
	FIXED BINARY (15) VALUE,	/* buffer length */
	ANY)		/* buffer */
	RETURNS (FIXED);

DECLARE GDS_$PUT_SLICE ENTRY(
        ANY VALUE,      /* status vector */
        ANY,            /* database handle */
        ANY,            /* transaction handle */
        ANY,            /* array (blob) id */
        FIXED BINARY (15),  /* sdl length */
        ANY,            /* sdl */
        FIXED BINARY (15),  /* parameter length */
        ANY,            /* parameter */
        FIXED BINARY (31),  /* slice length */
        ANY);            /* slice */

DECLARE GDS_$QUE_EVENTS ENTRY(
        ANY VALUE,      /* status vector */
        ANY,            /* database handle */
        ANY,            /* id */
        ANY,            /* length */
        ANY,            /* events */
        ANY,            /* ast */
        ANY);           /* arg */

DECLARE GDS_$RECEIVE ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* request handle */
	FIXED BINARY (15) VALUE,	/* message type */
	FIXED BINARY (15) VALUE,	/* message length */
	PTR VALUE,		/* message */
	FIXED BINARY (15) VALUE);	/* instantiation */

DECLARE GDS_$RELEASE_REQUEST ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* request handle */

DECLARE GDS_$ROLLBACK_TRANSACTION ENTRY(
	ANY VALUE,	/* status vector */
	ANY);		/* transaction handle */

DECLARE GDS_$SEND ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* request handle */
	FIXED BINARY (15) VALUE,	/* message type */
	FIXED BINARY (15) VALUE,	/* message length */
	ANY,		/* message */
	FIXED BINARY (15) VALUE);	/* instantiation */

DECLARE GDS_$SET_DEBUG ENTRY(
        FIXED BINARY (15) VALUE);

DECLARE GDS_$START_AND_SEND ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* request handle */
	ANY,		/* transaction handle */
	FIXED BINARY (15) VALUE,	/* message type */
	FIXED BINARY (15) VALUE,	/* message length */
	ANY,		/* message */
	FIXED BINARY (15) VALUE);	/* instantiation */

DECLARE GDS_$START_REQUEST ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* request handle */
	ANY,		/* transaction handle */
	FIXED BINARY (15) VALUE);	/* instantiation */

DECLARE GDS_$START_MULTIPLE ENTRY(
	ANY VALUE,	/* status vector */
	ANY,		/* transaction handle */
	FIXED BINARY (15) VALUE,	/* transaction count */
	ANY);		/* teb */

DECLARE GDS_$START_TRANSACTION ENTRY(
	ANY VALUE,			/* status vector */
	ANY,				/* transaction handle */
	FIXED BINARY (15) VALUE,	/* transaction count */
	ANY VALUE,			/* database handle */
	FIXED BINARY (15) VALUE,	/* tpb length */
	ANY VALUE) 			/* tpb */
	OPTIONS (VARIABLE);

DECLARE GDS_$STATUS_AND_STOP ENTRY(
	ANY VALUE);		/* status vector */

DECLARE GDS_$UNWIND_REQUEST ENTRY(
	ANY VALUE,			/* status vector */
	ANY,				/* request handle */
	FIXED BINARY (15) VALUE);	/* instantiation */

DECLARE gds_$print_status ENTRY(
	(20) FIXED);		/* status vector */


DECLARE gds_$encode_date ENTRY(
	ANY,		/* time record */
	(2) FIXED);		/* date */

DECLARE gds_$decode_date ENTRY(
	(2) FIXED,		/* date */
	ANY);		/* time record */


DECLARE blob_$display ENTRY (
		(2) FIXED,	/* blob_id*/
		ANY,		/* database handle */
		ANY,		/* transaction handle */
		ANY,		/* field name */
                ANY);		/* name_length */

DECLARE blob_$dump ENTRY (
		(2) FIXED,	/* blob_id*/
		ANY,		/* database handle */
		ANY,		/* transaction handle */
		ANY,		/* file name */
                ANY);		/* name_length */

DECLARE blob_$edit ENTRY (
		(2) FIXED,	/* blob_id*/
		ANY,		/* database handle */
		ANY,		/* transaction handle */
		ANY,		/* field name */
                ANY);		/* name_length */

DECLARE blob_$load ENTRY (
		(2) FIXED,	/* blob_id*/
		ANY,		/* database handle */
		ANY,		/* transaction handle */
		ANY,		/* file name */
                ANY);		/* name_length */

/* Dynamic SQL datatypes */

%REPLACE  SQL_TEXT	BY 452;
%REPLACE  SQL_VARYING	BY 448;
%REPLACE  SQL_SHORT	BY 500;
%REPLACE  SQL_LONG	BY 496;
%REPLACE  SQL_FLOAT	BY 482;
%REPLACE  SQL_DOUBLE	BY 480;
%REPLACE  SQL_DATE	BY 510;
%REPLACE  SQL_BLOB	BY 520;

/* Dynamic SQL DECLAREs */

DECLARE gds_$close ENTRY (
	ANY VALUE,		/* status vector */
	ANY		/* cursor_name */
	);

DECLARE gds_$declare ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* statement_name */
	ANY		/* cursor_name */
	);

DECLARE gds_$describe ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* statement_name */
	ANY		/* descriptor */
	);

DECLARE gds_$dsql_finish ENTRY (
	ANY		/* db_handle */
	);

DECLARE gds_$execute ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* trans_handle	*/
	ANY,		/* statement_name */
	ANY		/* descriptor */
	);

DECLARE gds_$execute_immediate ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* db_handle */
	ANY,		/* trans_handle */
	ANY,		/* string_length */
	ANY		/* string */
	);

DECLARE gds_$fetch ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* cursor_name */
	ANY		/* descriptor */
	) RETURNS (FIXED);

DECLARE gds_$open ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* trans_handle */
	ANY,		/* cursor_name */
	ANY		/* descriptor */
	);

DECLARE gds_$prepare ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* db_handle */
	ANY,		/* trans_handle */
	ANY,		/* statement_name */
	ANY,		/* string_length */
	ANY,		/* string */
	ANY		/* descriptor */
	);

/* Forms Package definitions */

/* Map definition block definitions */

$ SYMBOLS PYXIS MAP PYXIS_FORMAT

/* Field option flags for display options */

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

/* Field option values following display */

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

/* Pseudo key definitions */

$ SYMBOLS PYXIS KEY  OPT_FORMAT

/* Menu definition stuff */

$ SYMBOLS PYXIS MENU PYXIS_FORMAT

DECLARE pyxis_$compile_map ENTRY (
	ANY VALUE,		/* status vector */
	ANY,			/* form handle */
	ANY,			/* map handle */
	FIXED BINARY (15),	/* length */
	ANY			/* map handle */
	);

DECLARE pyxis_$compile_sub_map ENTRY (
	ANY VALUE,		/* status vector */
	ANY,			/* form handle */
	ANY,			/* map handle */
	FIXED BINARY (15),	/* length */
	ANY			/* map */
	);

DECLARE pyxis_$create_window ENTRY (
	ANY,			/* window handle */
	FIXED BINARY (15),	/* length */
	ANY,			/* file_name */
	FIXED BINARY (15),	/* width */
	FIXED BINARY (15)	/* height */
	);

DECLARE pyxis_$delete_window ENTRY (
	ANY		/* window handle */
	);

DECLARE pyxis_$drive_form ENTRY (
	ANY VALUE,		/* status vector */
	ANY,			/* database handle */
	ANY,			/* transaction handle */
	ANY,			/* window handle */
	ANY,			/* map handle */
	ANY,			/* input */
	ANY			/* output */
	);

DECLARE pyxis_$fetch ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* map handle */
	ANY		/* output */
	);

DECLARE pyxis_$insert ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* map handle */
	ANY		/* intput */
	);

DECLARE pyxis_$load_form ENTRY (
	ANY VALUE,		/* status vector */
	ANY,		/* database handle */
	ANY,		/* transaction handle */
	ANY,		/* form handle */
	ANY,		/* map handle */
	ANY		/* form_name */
	);

DECLARE pyxis_$menu ENTRY (
	ANY,			/* window handle */
	ANY,			/* menu handle */
	FIXED BINARY (15),	/* length */
	ANY			/* menu */
	) RETURNS (FIXED BINARY (15));

DECLARE pyxis_$pop_window ENTRY (
	ANY		/* window handle */
	);

DECLARE pyxis_$reset_form ENTRY (
	ANY VALUE,		/* status vector */
	ANY		/* window handle */
	);

DECLARE pyxis_$drive_menu ENTRY (
	ANY,			/* window_handle */
	ANY,			/* menu_handle */
	FIXED BINARY (15),	/* blr_length */
	ANY,			/* blr_source */
	FIXED BINARY (15),	/* title_length */
	ANY,			/* title */
	FIXED BINARY (15),	/* terminator */
	FIXED BINARY (15),	/* entree_length */
	ANY,			/* entree_text */
	ANY			/* entree_value */
	);
        
DECLARE pyxis_$get_entree ENTRY (
	ANY,			/* menu_handle */
	FIXED BINARY (15),	/* entree_length */
	ANY,			/* entree_text */
	ANY,			/* entree_value */
	FIXED BINARY (15)	/* entree_end */
	);

DECLARE pyxis_$initialize_menu ENTRY (
	ANY			/* menu_handle */
	);

DECLARE pyxis_$put_entree ENTRY (
	ANY,			/* menu_handle */
	FIXED BINARY (15),	/* entree_length */
	ANY,			/* entree_text */
	ANY			/* entree_value */
	);
');
INSERT INTO TEMPLATES (LANGUAGE, "FILE", TEMPLATE) VALUES ('BASIC', 'gds.bas',
'
	!*
	!*	PROGRAM:	BASIC preprocessor
	!*	MODULE:		gds.bas
	!*	DESCRIPTION:	BLR constants
	!*
	!* copyright (c) 1984, 1990 by Interbase Software Corporation
	!*
	
! Error codes

$ FORMAT ERROR_FORMAT	"	DECLARE LONG CONSTANT %-32s= %d\n
$ FORMAT PB_FORMAT	"	DECLARE BYTE CONSTANT %-32s= %d\n
$ FORMAT OPT_FORMAT	"	DECLARE WORD CONSTANT %-32s= %d\n
$ SYMBOLS ERROR MECH ERROR_FORMAT

$ ERROR MAJOR ERROR_FORMAT

! Minor codes subject to change

$ ERROR MINOR ERROR_FORMAT

! Database parameter block stuff 

$ SYMBOLS DPB ITEMS	PB_FORMAT
 
$ SYMBOLS DPB BITS	PB_FORMAT

! Bit assignments in RDB$SYSTEM_FLAG 

$ SYMBOLS RDB FLAG	PB_FORMAT

! Transaction parameter blob stuff 

$ SYMBOLS TPB ITEMS	PB_FORMAT

! Blob Parameter Block

$ SYMBOLS BPB ITEMS	PB_FORMAT

! Information call declarations 

! Common, structural codes 

$ SYMBOLS INFO MECH	PB_FORMAT

! Database information items

$ SYMBOLS INFO DB	PB_FORMAT

! Request information items

$ SYMBOLS INFO REQUEST	PB_FORMAT

! Blob information items

$ SYMBOLS INFO BLOB	PB_FORMAT

! Transaction information items

$ SYMBOLS INFO TRANSACTION	PB_FORMAT

! Dynamic SQL datatypes 

$ SYMBOLS SQL DTYPE OPT_FORMAT

! Forms Package definitions 

! Map definition block definitions 

$ SYMBOLS PYXIS MAP PB_FORMAT

! Field option flags for display options 

$ SYMBOLS PYXIS DISPLAY  OPT_FORMAT

! Field option values following display 

$ SYMBOLS PYXIS VALUE  OPT_FORMAT

! Pseudo key definitions 

$ SYMBOLS PYXIS KEY  OPT_FORMAT

! Menu definition stuff 

$ SYMBOLS PYXIS MENU PB_FORMAT

	EXTERNAL LONG FUNCTION GDS_$SQLCODE BY REF
	EXTERNAL SUB GDS_$ATTACH_DATABASE BY REF
	EXTERNAL SUB GDS_$CANCEL_BLOB BY REF
        EXTERNAL SUB GDS_$CANCEL_EVENTS BY REF
	EXTERNAL SUB GDS_$CLOSE_BLOB BY REF
        EXTERNAL SUB GDS_$COMMIT_RETAINING BY REF
	EXTERNAL SUB GDS_$COMMIT_TRANSACTION BY REF
	EXTERNAL SUB GDS_$COMPILE_REQUEST BY REF
	EXTERNAL SUB GDS_$CREATE_BLOB BY REF
        EXTERNAL SUB GDS_$CREATE_BLOB2 BY REF
	EXTERNAL SUB GDS_$CREATE_DATABASE BY REF
	EXTERNAL SUB GDS_$DETACH_DATABASE BY REF
        EXTERNAL SUB GDS_EVENT_WAIT BY REF
	EXTERNAL LONG FUNCTION GDS_$GET_SEGMENT BY REF
	EXTERNAL SUB GDS_$OPEN_BLOB BY REF
        EXTERNAL SUB GDS_$OPEN_BLOB2 BY REF
	EXTERNAL SUB GDS_$PREPARE_TRANSACTION BY REF
        EXTERNAL SUB GDS_$PREPARE_TRANSACTION2 BY REF
	EXTERNAL LONG FUNCTION GDS_$PUT_SEGMENT BY REF
        EXTERNAL SUB GDS_$QUE_EVENTS BY REF
	EXTERNAL SUB GDS_$RECEIVE BY REF
	EXTERNAL SUB GDS_$RELEASE_REQUEST BY REF
	EXTERNAL SUB GDS_$ROLLBACK_TRANSACTION BY REF
	EXTERNAL SUB GDS_$SEND BY REF
        EXTERNAL SUB gds_$set_debug BY REF
	EXTERNAL SUB GDS_$START_AND_SEND BY REF
	EXTERNAL SUB GDS_$START_REQUEST BY REF
	EXTERNAL SUB GDS_$START_MULTIPLE BY REF
	EXTERNAL SUB GDS_$START_TRANSACTION BY REF
	EXTERNAL SUB GDS_$UNWIND_REQUEST BY REF
	EXTERNAL SUB gds_$print_status BY REF
	EXTERNAL SUB gds_$encode_date BY REF
	EXTERNAL SUB gds_$decode_date BY REF
	EXTERNAL SUB blob_$display BY REF
	EXTERNAL SUB blob_$dump BY REF
	EXTERNAL SUB blob_$edit BY REF
	EXTERNAL SUB blob_$load BY REF
	EXTERNAL SUB gds_$close BY REF
	EXTERNAL SUB gds_$declare BY REF
	EXTERNAL SUB gds_$describe BY REF
	EXTERNAL SUB gds_$dsql_finish BY REF
	EXTERNAL SUB gds_$execute BY REF
	EXTERNAL SUB gds_$execute_immediate BY REF
	EXTERNAL LONG FUNCTION gds_$fetch BY REF
	EXTERNAL SUB gds_$open BY REF
	EXTERNAL SUB gds_$prepare BY REF
        EXTERNAL SUB gds_$event_counts BY REF
        EXTERNAL SUB gds_$event_block BY REF
        EXTERNAL SUB gds_$get_slice BY REF
        EXTERNAL SUB gds_$put_slice BY REF
        EXTERNAL SUB gds_$seek_blob BY REF
        EXTERNAL SUB gds_$ddl BY REF
	EXTERNAL SUB pyxis_$compile_map BY REF
	EXTERNAL SUB pyxis_$compile_sub_map BY REF
	EXTERNAL SUB pyxis_$create_window BY REF
	EXTERNAL SUB pyxis_$delete_window BY REF
	EXTERNAL SUB pyxis_$drive_form BY REF
	EXTERNAL SUB pyxis_$fetch BY REF
	EXTERNAL SUB pyxis_$insert BY REF
	EXTERNAL SUB pyxis_$load_form BY REF
	EXTERNAL WORD FUNCTION pyxis_$menu BY REF
	EXTERNAL SUB pyxis_$pop_window BY REF
	EXTERNAL SUB pyxis_$reset_form BY REF
	EXTERNAL SUB pyxis_$drive_menu BY REF
	EXTERNAL SUB pyxis_$initialize_menu BY REF
	EXTERNAL SUB pyxis_$get_entree BY REF
	EXTERNAL SUB pyxis_$put_entree BY REF
');

COMMIT WORK;
