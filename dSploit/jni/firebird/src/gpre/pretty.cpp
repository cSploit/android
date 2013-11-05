//____________________________________________________________
//
//		PROGRAM:	BLR Pretty Printer
//		MODULE:		pretty.cpp
//		DESCRIPTION:	BLR Pretty Printer
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings
//
//
//____________________________________________________________
//
//

#include "firebird.h"
#include <stdio.h>
#include "../jrd/common.h"
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../jrd/constants.h"
#include "../gpre/prett_proto.h"
#include "../jrd/gds_proto.h"

static inline void ADVANCE_PTR(TEXT*& ptr)
{
	while (*ptr)
		ptr++;
}

//#define PRINT_VERB 	if (print_verb (control, level)) return -1
#define PRINT_DYN_VERB 	if (print_dyn_verb (control, level)) return -1
#define PRINT_SDL_VERB 	if (print_sdl_verb (control, level)) return -1
#define BLR_BYTE	*(control->ctl_blr)++
#define PUT_BYTE(byte)	*(control->ctl_ptr)++ = byte
#define NEXT_BYTE	*(control->ctl_blr)


struct ctl
{
	UCHAR *ctl_blr;				// Running blr string
	UCHAR *ctl_blr_start;		// Original start of blr string
	FPTR_PRINT_CALLBACK ctl_routine;	// Call back
	void *ctl_user_arg;		// User argument
	TEXT *ctl_ptr;
	SSHORT ctl_language;
	SSHORT ctl_level;
	TEXT ctl_buffer[PRETTY_BUFFER_SIZE];
};


static int blr_format(ctl*, const char *, ...);
static int error(ctl*, SSHORT, const TEXT *, int);
static int indent(ctl*, SSHORT);
static int print_blr_dtype(ctl*, bool);
static void print_blr_line(void*, SSHORT, const char*);
static int print_byte(ctl*);
static int print_char(ctl*, SSHORT);
static int print_dyn_verb(ctl*, SSHORT);
static int print_line(ctl*, SSHORT);
static SLONG print_long(ctl*);
static int print_sdl_verb(ctl*, SSHORT);
static int print_string(ctl*, SSHORT);
static int print_word(ctl*);


static inline void CHECK_BUFFER(ctl* control, SSHORT offset)
{
	if (control->ctl_ptr > control->ctl_buffer + sizeof(control->ctl_buffer) - 20)
		print_line(control, offset);
}


const char *dyn_table[] =
{
#include "../gpre/dyntable.h"
	NULL
};

const char *cdb_table[] =
{
#include "../gpre/cdbtable.h"
	NULL
};

const char *sdl_table[] =
{
#include "../gpre/sdltable.h"
	NULL
};

const char *map_strings[] =
{
	"FIELD2",
	"FIELD1",
	"MESSAGE",
	"TERMINATOR",
	"TERMINATING_FIELD",
	"OPAQUE",
	"TRANSPARENT",
	"TAG",
	"SUB_FORM",
	"ITEM_INDEX",
	"SUB_FIELD"
};

//____________________________________________________________
//
//		Pretty print create database parameter buffer thru callback routine.
//

int PRETTY_print_cdb( UCHAR* blr, FPTR_PRINT_CALLBACK routine, void* user_arg, SSHORT language)
{

	ctl ctl_buffer;
	ctl* control = &ctl_buffer;

	if (!routine)
	{
		routine = gds__default_printer;
		user_arg = NULL;
	}

	control->ctl_routine = routine;
	control->ctl_user_arg = user_arg;
	control->ctl_blr = control->ctl_blr_start = blr;
	control->ctl_ptr = control->ctl_buffer;
	control->ctl_language = language;

	SSHORT level = 0;
	indent(control, level);
	const SSHORT i = BLR_BYTE;

	SCHAR temp[32];
	if (*control->ctl_blr)
		sprintf(temp, "gds__dpb_version%d, ", i);
	else
		sprintf(temp, "gds__dpb_version%d", i);
	blr_format(control, temp);

	SSHORT offset = 0;
	print_line(control, offset);

	SSHORT parameter;
	while (parameter = BLR_BYTE)
	{
		const char* p;
		if (parameter > FB_NELEM(cdb_table) || !(p = cdb_table[parameter]))
		{
			return error(control, 0, "*** cdb parameter %d is undefined ***\n", parameter);
		}
		indent(control, level);
		blr_format(control, p);
		PUT_BYTE(',');
		int length = print_byte(control);
		if (length)
		{
			do {
				print_char(control, offset);
			} while (--length);
		}
		print_line(control, offset);
	}

	return 0;
}


int PRETTY_print_dyn(UCHAR* blr, FPTR_PRINT_CALLBACK routine, void* user_arg, SSHORT language)
//____________________________________________________________
//
//		Pretty print dynamic DDL thru callback routine.
//

{
	ctl ctl_buffer;
	ctl* control = &ctl_buffer;

	if (!routine)
	{
		routine = gds__default_printer;
		user_arg = NULL;
	}

	control->ctl_routine = routine;
	control->ctl_user_arg = user_arg;
	control->ctl_blr = control->ctl_blr_start = blr;
	control->ctl_ptr = control->ctl_buffer;
	control->ctl_language = language;

	const SSHORT version = BLR_BYTE;

	SSHORT offset = 0;
	if (version != isc_dyn_version_1)
		return error(control, offset, "*** dyn version %d is not supported ***\n", version);

	blr_format(control, "gds__dyn_version_1, ");
	print_line(control, offset);
	SSHORT level = 1;
	PRINT_DYN_VERB;

	if (BLR_BYTE != isc_dyn_eoc)
		return error(control, offset, "*** expected dyn end-of-command  ***\n", 0);

	blr_format(control, "gds__dyn_eoc");
	print_line(control, offset);

	return 0;
}


int PRETTY_print_sdl(UCHAR* blr, FPTR_PRINT_CALLBACK routine, void *user_arg, SSHORT language)
//____________________________________________________________
//
//		Pretty print slice description language.
//
{
	ctl ctl_buffer;
	ctl* control = &ctl_buffer;

	if (!routine)
	{
		routine = gds__default_printer;
		user_arg = NULL;
	}

	control->ctl_routine = routine;
	control->ctl_user_arg = user_arg;
	control->ctl_blr = control->ctl_blr_start = blr;
	control->ctl_ptr = control->ctl_buffer;
	control->ctl_language = language;

	const SSHORT version = BLR_BYTE;

	SSHORT offset = 0;
	if (version != isc_sdl_version1)
		return error(control, offset, "*** sdl version %d is not supported ***\n", version);

	blr_format(control, "gds__sdl_version1, ");
	print_line(control, offset);
	SSHORT level = 1;

	while (NEXT_BYTE != isc_sdl_eoc)
		PRINT_SDL_VERB;

	offset = control->ctl_blr - control->ctl_blr_start;
	blr_format(control, "gds__sdl_eoc");
	print_line(control, offset);

	return 0;
}


//____________________________________________________________
//
//		Format an utterance.
//

static int blr_format(ctl* control, const char *string, ...)
{
	va_list ptr;

	va_start(ptr, string);
	vsprintf(control->ctl_ptr, string, ptr);
	va_end(ptr);
	while (*control->ctl_ptr)
		control->ctl_ptr++;

	return 0;
}


//____________________________________________________________
//
//		Put out an error msg and punt.
//

static int error( ctl* control, SSHORT offset, const TEXT* string, int arg)
{

	print_line(control, offset);
	sprintf(control->ctl_ptr, string, arg);
	fprintf(stderr, "%s", control->ctl_ptr);
	ADVANCE_PTR(control->ctl_ptr);
	print_line(control, offset);

	return -1;
}


//____________________________________________________________
//
//		Indent for pretty printing.
//

static int indent( ctl* control, SSHORT level)
{

	level *= 3;
	while (--level >= 0)
		PUT_BYTE(' ');
	return 0;
}


//____________________________________________________________
//
//		Print a datatype sequence and return the length of the
//		data described.
//

static int print_blr_dtype(ctl* control, bool print_object)
{
	const char* string = NULL;
	SSHORT length = -1;

	const USHORT dtype = BLR_BYTE;

	// Special case blob (261) to keep down the size of the
	// jump table

	switch (dtype)
	{
	case blr_short:
		string = "short";
		length = 2;
		break;

	case blr_long:
		string = "long";
		length = 4;
		break;

	case blr_quad:
		string = "quad";
		length = 8;
		break;

	// Begin date/time/timestamp
	case blr_sql_date:
		string = "sql_date";
		length = sizeof(ISC_DATE);
		break;

	case blr_sql_time:
		string = "sql_time";
		length = sizeof(ISC_TIME);
		break;

	case blr_timestamp:
		string = "timestamp";
		length = sizeof(ISC_TIMESTAMP);
		break;
	// End date/time/timestamp

	case blr_int64:
		string = "int64";
		length = sizeof(ISC_INT64);
		break;

	case blr_float:
		string = "float";
		length = 4;
		break;

	case blr_double:
		string = "double";
		length = 8;
		break;

	case blr_d_float:
		string = "d_float";
		length = 8;
		break;

	case blr_text:
		string = "text";
		break;

	case blr_cstring:
		string = "cstring";
		break;

	case blr_varying:
		string = "varying";
		break;

	case blr_text2:
		string = "text2";
		break;

	case blr_cstring2:
		string = "cstring2";
		break;

	case blr_varying2:
		string = "varying2";
		break;

	case blr_blob_id:
		string = "blob_id";
		length = 8;
		break;

	default:
		error(control, 0, "*** invalid data type ***", 0);
	}

	blr_format(control, "blr_%s, ", string);

	if (!print_object)
		return length;

	switch (dtype)
	{
	case blr_text:
		length = print_word(control);
		break;

	case blr_varying:
		length = print_word(control) + 2;
		break;

	case blr_text2:
		print_word(control);
		length = print_word(control);
		break;

	case blr_varying2:
		print_word(control);
		length = print_word(control) + 2;
		break;

	case blr_short:
	case blr_long:
	case blr_int64:
	case blr_quad:
		print_byte(control);
		break;

	case blr_blob_id:
		print_word(control);
		break;

	default:
		if (dtype == blr_cstring)
			length = print_word(control);
		else if (dtype == blr_cstring2)
		{
			print_word(control);
			length = print_word(control);
		}
		break;
	}

	return length;
}



//____________________________________________________________
//
//		Print a line of pretty-printed BLR.
//

static void print_blr_line(void* arg, SSHORT offset, const char* line)
{
	ctl* control = static_cast<ctl*>(arg);
	bool comma = false;
	char c;

	indent(control, control->ctl_level);

	while (c = *line++)
	{
		PUT_BYTE(c);
		if (c == ',')
			comma = true;
		else if (c != ' ')
			comma = false;
	}

	if (!comma)
		PUT_BYTE(',');

	print_line(control, offset);
}


//____________________________________________________________
//
//		Print a byte as a numeric value and return same.
//

static int print_byte( ctl* control)
{
	const UCHAR v = BLR_BYTE;
	sprintf(control->ctl_ptr, control->ctl_language ? "chr(%d), " : "%d, ", v);
	ADVANCE_PTR(control->ctl_ptr);

	return v;
}


//____________________________________________________________
//
//		Print a byte as a numeric value and return same.
//

static int print_char( ctl* control, SSHORT offset)
{
	const UCHAR c = BLR_BYTE;
	const bool printable = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') || c == '$' || c == '_';

	sprintf(control->ctl_ptr, printable ? "'%c'," : control->ctl_language ? "chr(%d)," : "%d,", c);
	ADVANCE_PTR(control->ctl_ptr);

	CHECK_BUFFER(control, offset);

	return c;
}


//____________________________________________________________
//
//		Primary recursive routine to print dynamic DDL.
//

static int print_dyn_verb( ctl* control, SSHORT level)
{
	const SSHORT offset = control->ctl_blr - control->ctl_blr_start;
	const UCHAR dyn_operator = BLR_BYTE;

    const char* p;
	const int size = FB_NELEM(dyn_table);
	if (dyn_operator > size || dyn_operator <= 0 || !(p = dyn_table[dyn_operator])) {
		return error(control, offset, "*** dyn operator %d is undefined ***\n", (int) dyn_operator);
	}

	indent(control, level);
	blr_format(control, p);
	PUT_BYTE(',');
	PUT_BYTE(' ');
	++level;

	int length;

	switch (dyn_operator)
	{
	case isc_dyn_drop_difference:
	case isc_dyn_begin_backup:
	case isc_dyn_end_backup:
		return 0;
	case isc_dyn_begin:
	case isc_dyn_mod_database:
		print_line(control, offset);
		while (NEXT_BYTE != isc_dyn_end)
			PRINT_DYN_VERB;
		PRINT_DYN_VERB;
		return 0;

	case isc_dyn_view_blr:
	case isc_dyn_fld_validation_blr:
	case isc_dyn_fld_computed_blr:
	case isc_dyn_trg_blr:
	case isc_dyn_fld_missing_value:
	case isc_dyn_prc_blr:
	case isc_dyn_fld_default_value:
		length = print_word(control);
		print_line(control, offset);
		if (length)
		{
			control->ctl_level = level;
			fb_print_blr(control->ctl_blr, length, print_blr_line, control, control->ctl_language);
			control->ctl_blr += length;
		}
		return 0;

	case isc_dyn_scl_acl:
//	case isc_dyn_log_check_point_length:
//	case isc_dyn_log_num_of_buffers:
//	case isc_dyn_log_buffer_size:
//	case isc_dyn_log_group_commit_wait:
	case isc_dyn_idx_inactive:
		length = print_word(control);
		while (length--)
			print_byte(control);
		print_line(control, offset);
		return 0;

	case isc_dyn_view_source:
	case isc_dyn_fld_validation_source:
	case isc_dyn_fld_computed_source:
	case isc_dyn_description:
	case isc_dyn_prc_source:
	case isc_dyn_fld_default_source:
		length = print_word(control);
		while (length--)
			print_char(control, offset);
		print_line(control, offset);
		return 0;

	case isc_dyn_del_exception:
		if (length = print_word(control))
			do {
				print_char(control, offset);
			} while (--length);
		return 0;

	case isc_dyn_fld_not_null:
	case isc_dyn_sql_object:
//	case isc_dyn_drop_log:
//	case isc_dyn_drop_cache:
//	case isc_dyn_def_default_log:
//	case isc_dyn_log_file_serial:
//	case isc_dyn_log_file_raw:
//	case isc_dyn_log_file_overflow:
	case isc_dyn_single_validation:
	case isc_dyn_del_computed:
	case isc_dyn_del_default:
	case isc_dyn_del_validation:
	case isc_dyn_idx_statistic:
	case isc_dyn_foreign_key_delete:
	case isc_dyn_foreign_key_update:
	case isc_dyn_foreign_key_cascade:
	case isc_dyn_foreign_key_default:
	case isc_dyn_foreign_key_null:
	case isc_dyn_foreign_key_none:

		print_line(control, offset);
		return 0;

	case isc_dyn_end:
		print_line(control, offset);
		return 0;
	}

	if (length = print_word(control))
		do {
			print_char(control, offset);
		} while (--length);

	print_line(control, offset);

	switch (dyn_operator)
	{
	case isc_dyn_def_database:
	case isc_dyn_def_dimension:
	case isc_dyn_def_exception:
	case isc_dyn_def_file:
//	case isc_dyn_def_log_file:
//	case isc_dyn_def_cache_file:
	case isc_dyn_def_filter:
	case isc_dyn_def_function:
	case isc_dyn_def_function_arg:
	case isc_dyn_def_generator:
	case isc_dyn_def_global_fld:
	case isc_dyn_def_idx:
	case isc_dyn_def_local_fld:
	case isc_dyn_def_rel:
	case isc_dyn_def_procedure:
	case isc_dyn_def_parameter:
	case isc_dyn_def_security_class:
	case isc_dyn_def_shadow:
	case isc_dyn_def_sql_fld:
	case isc_dyn_def_trigger:
	case isc_dyn_def_trigger_msg:
	case isc_dyn_def_view:
	case isc_dyn_delete_database:
	case isc_dyn_delete_dimensions:
	case isc_dyn_delete_filter:
	case isc_dyn_delete_function:
	case isc_dyn_delete_global_fld:
	case isc_dyn_delete_idx:
	case isc_dyn_delete_local_fld:
	case isc_dyn_delete_rel:
	case isc_dyn_delete_procedure:
	case isc_dyn_delete_parameter:
	case isc_dyn_delete_security_class:
	case isc_dyn_delete_trigger:
	case isc_dyn_delete_trigger_msg:
	case isc_dyn_delete_shadow:
	case isc_dyn_mod_exception:
	case isc_dyn_mod_global_fld:
	case isc_dyn_mod_idx:
	case isc_dyn_mod_local_fld:
	case isc_dyn_mod_procedure:
	case isc_dyn_mod_rel:
	case isc_dyn_mod_security_class:
	case isc_dyn_mod_trigger:
	case isc_dyn_mod_trigger_msg:
	case isc_dyn_rel_constraint:
	case isc_dyn_mod_view:
	case isc_dyn_grant:
	case isc_dyn_revoke:
	case isc_dyn_view_relation:
	case isc_dyn_def_sql_role:
		while (NEXT_BYTE != isc_dyn_end)
			PRINT_DYN_VERB;
		PRINT_DYN_VERB;
		return 0;
	}

	return 0;
}


//____________________________________________________________
//
//		Invoke callback routine to print (or do something with) a line.
//

static int print_line( ctl* control, SSHORT offset)
{

	*control->ctl_ptr = 0;
	(*control->ctl_routine) (control->ctl_user_arg, offset, control->ctl_buffer);
	control->ctl_ptr = control->ctl_buffer;
	return 0;
}


//____________________________________________________________
//
//		Print a VAX word as a numeric value an return same.
//

static SLONG print_long( ctl* control)
{
	const UCHAR v1 = BLR_BYTE;
	const UCHAR v2 = BLR_BYTE;
	const UCHAR v3 = BLR_BYTE;
	const UCHAR v4 = BLR_BYTE;
	sprintf(control->ctl_ptr, control->ctl_language ?
			"chr(%d),chr(%d),chr(%d),chr(%d) " : "%d,%d,%d,%d, ",
			v1, v2, v3, v4);
	ADVANCE_PTR(control->ctl_ptr);

	return v1 | (v2 << 8) | (v3 << 16) | (v4 << 24);
}


//____________________________________________________________
//
//		Primary recursive routine to print slice description language.
//

static int print_sdl_verb( ctl* control, SSHORT level)
{
	const char* p;

	SSHORT offset = control->ctl_blr - control->ctl_blr_start;
	const UCHAR sdl_operator = BLR_BYTE;

	if (sdl_operator > FB_NELEM(sdl_table) || sdl_operator <= 0 || !(p = sdl_table[sdl_operator]))
	{
		return error(control, offset, "*** SDL operator %d is undefined ***\n", (int) sdl_operator);
	}

	indent(control, level);
	blr_format(control, p);
	PUT_BYTE(',');
	PUT_BYTE(' ');
	++level;
	int n = 0;

	switch (sdl_operator)
	{
	case isc_sdl_begin:
		print_line(control, offset);
		while (NEXT_BYTE != isc_sdl_end)
			PRINT_SDL_VERB;
		PRINT_SDL_VERB;
		return 0;

	case isc_sdl_struct:
		n = print_byte(control);
		while (n--)
		{
			print_line(control, offset);
			indent(control, level + 1);
			offset = control->ctl_blr - control->ctl_blr_start;
			print_blr_dtype(control, true);
		}
		break;

	case isc_sdl_scalar:
		print_byte(control);

	case isc_sdl_element:
		n = print_byte(control);
		print_line(control, offset);
		while (n--)
			PRINT_SDL_VERB;
		return 0;

	case isc_sdl_field:
	case isc_sdl_relation:
		print_string(control, offset);
		break;

	case isc_sdl_fid:
	case isc_sdl_rid:
	case isc_sdl_short_integer:
		print_word(control);
		break;

	case isc_sdl_variable:
	case isc_sdl_tiny_integer:
		print_byte(control);
		break;

	case isc_sdl_long_integer:
		print_long(control);
		break;

	case isc_sdl_add:
	case isc_sdl_subtract:
	case isc_sdl_multiply:
	case isc_sdl_divide:
		print_line(control, offset);
		PRINT_SDL_VERB;
		PRINT_SDL_VERB;
		return 0;

	case isc_sdl_negate:
		print_line(control, offset);
		PRINT_SDL_VERB;
		return 0;

	case isc_sdl_do3:
		n++;
	case isc_sdl_do2:
		n++;
	case isc_sdl_do1:
		n += 2;
		print_byte(control);
		print_line(control, offset);
		while (n--)
			PRINT_SDL_VERB;
		return 0;
	}

	print_line(control, offset);

	return 0;
}


//____________________________________________________________
//
//		Print a byte-counted string.
//

static int print_string( ctl* control, SSHORT offset)
{
	SSHORT n = print_byte(control);
	while (--n >= 0)
		print_char(control, offset);

	PUT_BYTE(' ');
	return 0;
}


//____________________________________________________________
//
//		Print a VAX word as a numeric value an return same.
//

static int print_word( ctl* control)
{
	const UCHAR v1 = BLR_BYTE;
	const UCHAR v2 = BLR_BYTE;
	sprintf(control->ctl_ptr, control->ctl_language ? "chr(%d),chr(%d), " : "%d,%d, ", v1, v2);
	ADVANCE_PTR(control->ctl_ptr);

	return (v2 << 8) | v1;
}

