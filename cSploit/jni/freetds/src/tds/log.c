/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2006-2010  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>

#if TIME_WITH_SYS_TIME
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
# include <process.h>
#endif

#include <freetds/tds.h>
#include "tds_checks.h"
#include <freetds/thread.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: log.c,v 1.21 2011-06-05 09:21:49 freddy77 Exp $");

/* for now all messages go to the log */
int tds_debug_flags = TDS_DBGFLAG_ALL | TDS_DBGFLAG_SOURCE;
int tds_g_append_mode = 0;
static char *g_dump_filename = NULL;
int tds_write_dump = 0;	/* is TDS stream debug log turned on? */
static FILE *g_dumpfile = NULL;	/* file pointer for dump log          */
static tds_mutex g_dump_mutex = TDS_MUTEX_INITIALIZER;

static FILE* tdsdump_append(void);

#ifdef TDS_ATTRIBUTE_DESTRUCTOR
static void __attribute__((destructor))
tds_util_deinit(void)
{
	tdsdump_close();
}
#endif

/**
 * Temporarily turn off logging.
 */
void
tdsdump_off(void)
{
	tds_mutex_lock(&g_dump_mutex);
	tds_write_dump = 0;
	tds_mutex_unlock(&g_dump_mutex);
}				/* tdsdump_off()  */


/**
 * Turn logging back on.  You must call tdsdump_open() before calling this routine.
 */
void
tdsdump_on(void)
{
	tds_mutex_lock(&g_dump_mutex);
	tds_write_dump = 1;
	tds_mutex_unlock(&g_dump_mutex);
}

int
tdsdump_isopen()
{
	return NULL != g_dumpfile;
}


/**
 * Create and truncate a human readable dump file for the TDS
 * traffic.  The name of the file is specified by the filename
 * parameter.  If that is given as NULL or an empty string,
 * any existing log file will be closed.
 *
 * \return  true if the file was opened, false if it couldn't be opened.
 */
int
tdsdump_open(const char *filename)
{
	int result;		/* really should be a boolean, not an int */

	tds_mutex_lock(&g_dump_mutex);

	/* same append file */
	if (tds_g_append_mode && filename != NULL && g_dump_filename != NULL && strcmp(filename, g_dump_filename) == 0) {
		tds_mutex_unlock(&g_dump_mutex);
		return 1;
	}

	/* free old one */
	if (g_dumpfile != NULL && g_dumpfile != stdout && g_dumpfile != stderr)
		fclose(g_dumpfile);
	g_dumpfile = NULL;
	if (g_dump_filename)
		TDS_ZERO_FREE(g_dump_filename);

	/* required to close just log ?? */
	if (filename == NULL || filename[0] == '\0') {
		tds_mutex_unlock(&g_dump_mutex);
		return 1;
	}

	result = 1;
	if (tds_g_append_mode) {
		g_dump_filename = strdup(filename);
		/* if mutex are available do not reopen file every time */
#ifdef TDS_HAVE_MUTEX
		g_dumpfile = tdsdump_append();
#endif
	} else if (!strcmp(filename, "stdout")) {
		g_dumpfile = stdout;
	} else if (!strcmp(filename, "stderr")) {
		g_dumpfile = stderr;
	} else if (NULL == (g_dumpfile = fopen(filename, "w"))) {
		result = 0;
	}

	if (result)
		tds_write_dump = 1;
	tds_mutex_unlock(&g_dump_mutex);

	if (result) {
		char today[64];
		struct tm *tm;
#ifdef HAVE_LOCALTIME_R
		struct tm res;
#endif
		time_t t;

		time(&t);
#ifdef HAVE_LOCALTIME_R
#if HAVE_FUNC_LOCALTIME_R_TM
		tm = localtime_r(&t, &res);
#elif HAVE_FUNC_LOCALTIME_R_INT
		tm = NULL;
		if (!localtime_r(&t, &res))
			tm = &res;
#else
#error One should be defined
#endif
#else
		tm = localtime(&t);
#endif

		strftime(today, sizeof(today), "%Y-%m-%d %H:%M:%S", tm);
		tdsdump_log(TDS_DBG_INFO1, "Starting log file for FreeTDS %s\n"
			    "\ton %s with debug flags 0x%x.\n", VERSION, today, tds_debug_flags);
	}
	return result;
}				/* tdsdump_open()  */

static FILE*
tdsdump_append(void)
{
	if (!g_dump_filename)
		return NULL;

	if (!strcmp(g_dump_filename, "stdout")) {
		return stdout;
	} else if (!strcmp(g_dump_filename, "stderr")) {
		return stderr;
	}
	return fopen(g_dump_filename, "a");
}


/**
 * Close the TDS dump log file.
 */
void
tdsdump_close(void)
{
	tds_mutex_lock(&g_dump_mutex);
	tds_write_dump = 0;
	if (g_dumpfile != NULL && g_dumpfile != stdout && g_dumpfile != stderr)
		fclose(g_dumpfile);
	g_dumpfile = NULL;
	if (g_dump_filename)
		TDS_ZERO_FREE(g_dump_filename);
	tds_mutex_unlock(&g_dump_mutex);
}				/* tdsdump_close()  */

static void
tdsdump_start(FILE *file, const char *fname, int line)
{
	char buf[128], *pbuf;
	int started = 0;

	/* write always time before log */
	if (tds_debug_flags & TDS_DBGFLAG_TIME) {
		fputs(tds_timestamp_str(buf, 127), file);
		started = 1;
	}

	pbuf = buf;
	if (tds_debug_flags & TDS_DBGFLAG_PID) {
		if (started)
			*pbuf++ = ' ';
		pbuf += sprintf(pbuf, "%d", (int) getpid());
		started = 1;
	}

	if ((tds_debug_flags & TDS_DBGFLAG_SOURCE) && fname && line) {
		const char *p;
		p = strrchr(fname, '/');
		if (p)
			fname = p + 1;
		p = strrchr(fname, '\\');
		if (p)
			fname = p + 1;
		if (started)
			pbuf += sprintf(pbuf, " (%s:%d)", fname, line);
		else
			pbuf += sprintf(pbuf, "%s:%d", fname, line);
		started = 1;
	}
	if (started)
		*pbuf++ = ':';
	*pbuf = 0;
	fputs(buf, file);
}

/**
 * Dump the contents of data into the log file in a human readable format.
 * \param file       source file name
 * \param level_line line and level combined. This and file are automatically computed by
 *                   TDS_DBG_* macros.
 * \param msg        message to print before dump
 * \param buf        buffer to dump
 * \param length     number of bytes in the buffer
 */
void
tdsdump_dump_buf(const char* file, unsigned int level_line, const char *msg, const void *buf, size_t length)
{
	size_t i, j;
#define BYTES_PER_LINE 16
	const unsigned char *data = (const unsigned char *) buf;
	const int debug_lvl = level_line & 15;
	const int line = level_line >> 4;
	char line_buf[BYTES_PER_LINE * 8 + 16], *p;
	FILE *dumpfile;

	if (((tds_debug_flags >> debug_lvl) & 1) == 0 || !tds_write_dump)
		return;

	if (!g_dumpfile && !g_dump_filename)
		return;

	tds_mutex_lock(&g_dump_mutex);

	dumpfile = g_dumpfile;
#ifdef TDS_HAVE_MUTEX
	if (tds_g_append_mode && dumpfile == NULL)
		dumpfile = g_dumpfile = tdsdump_append();
#else
	if (tds_g_append_mode)
		dumpfile = tdsdump_append();
#endif

	if (dumpfile == NULL) {
		tds_mutex_unlock(&g_dump_mutex);
		return;
	}

	tdsdump_start(dumpfile, file, line);

	fprintf(dumpfile, "%s\n", msg);

	for (i = 0; i < length; i += BYTES_PER_LINE) {
		p = line_buf;
		/*
		 * print the offset as a 4 digit hex number
		 */
		p += sprintf(p, "%04x", ((unsigned int) i) & 0xffffu);

		/*
		 * print each byte in hex
		 */
		for (j = 0; j < BYTES_PER_LINE; j++) {
			if (j == BYTES_PER_LINE / 2)
				*p++ = '-';
			else
				*p++ = ' ';
			if (j + i >= length)
				p += sprintf(p, "  ");
			else
				p += sprintf(p, "%02x", data[i + j]);
		}

		/*
		 * skip over to the ascii dump column
		 */
		p += sprintf(p, " |");

		/*
		 * print each byte in ascii
		 */
		for (j = i; j < length && (j - i) < BYTES_PER_LINE; j++) {
			if (j - i == BYTES_PER_LINE / 2)
				*p++ = ' ';
			p += sprintf(p, "%c", (isprint(data[j])) ? data[j] : '.');
		}
		strcpy(p, "|\n");
		fputs(line_buf, dumpfile);
	}
	fputs("\n", dumpfile);

	fflush(dumpfile);

#ifndef TDS_HAVE_MUTEX
	if (tds_g_append_mode) {
		if (dumpfile != stdout && dumpfile != stderr)
			fclose(dumpfile);
	}
#endif

	tds_mutex_unlock(&g_dump_mutex);

}				/* tdsdump_dump_buf()  */


#undef tdsdump_log
/**
 * Write a message to the debug log.  
 * \param file name of the log file
 * \param level_line kind of detail to be included
 * \param fmt       printf-like format string
 */
void
tdsdump_log(const char* file, unsigned int level_line, const char *fmt, ...)
{
	const int debug_lvl = level_line & 15;
	const int line = level_line >> 4;
	va_list ap;
	FILE *dumpfile;

	if (((tds_debug_flags >> debug_lvl) & 1) == 0 || !tds_write_dump)
		return;

	if (!g_dumpfile && !g_dump_filename)
		return;

	tds_mutex_lock(&g_dump_mutex);

	dumpfile = g_dumpfile;
#ifdef TDS_HAVE_MUTEX
	if (tds_g_append_mode && dumpfile == NULL)
		dumpfile = g_dumpfile = tdsdump_append();
#else
	if (tds_g_append_mode)
		dumpfile = tdsdump_append();
#endif
	
	if (dumpfile == NULL) { 
		tds_mutex_unlock(&g_dump_mutex);
		return;
	}

	tdsdump_start(dumpfile, file, line);

	va_start(ap, fmt);

	vfprintf(dumpfile, fmt, ap);
	va_end(ap);

	fflush(dumpfile);

#ifndef TDS_HAVE_MUTEX
	if (tds_g_append_mode) {
		if (dumpfile != stdout && dumpfile != stderr)
			fclose(dumpfile);
	}
#endif
	tds_mutex_unlock(&g_dump_mutex);
}				/* tdsdump_log()  */
#define tdsdump_log if (TDS_UNLIKELY(tds_write_dump)) tdsdump_log


/**
 * Write a column value to the debug log.  
 * \param col column to dump
 */
void
tdsdump_col(const TDSCOLUMN *col)
{
	const char* typename;
	char* data;
	TDS_SMALLINT type;
	
	assert(col);
	assert(col->column_data);
	
	typename = tds_prtype(col->column_type);
	type = col->column_type;
	
	switch(type) {
	case SYBINTN:
		switch(col->column_cur_size) {
		case sizeof(TDS_INT):
			type = SYBINT4;
			break;
		case sizeof(TDS_SMALLINT):
			type = SYBINT2;
			break;
		case sizeof(TDS_TINYINT):
			type = SYBINT1;
			break;
		}
		break;
	case SYBFLTN:
		switch(col->column_cur_size) {
		case sizeof(TDS_REAL):
			type = SYBREAL;
			break;
		case sizeof(TDS_FLOAT):
			type = SYBFLT8;
			break;
		}
		break;
	}
	
	switch(type) {
	case SYBCHAR: 
	case SYBVARCHAR:
		if (col->column_cur_size >= 0) {
			data = (char*) calloc(1, 1 + col->column_cur_size);
			if (!data) {
				tdsdump_log(TDS_DBG_FUNC, "no memory to log data for type %s\n", typename);
				return;
			}
			memcpy(data, col->column_data, col->column_cur_size);
			tdsdump_log(TDS_DBG_FUNC, "type %s has value \"%s\"\n", typename, data);
			free(data);
		} else {
			tdsdump_log(TDS_DBG_FUNC, "type %s has value NULL\n", typename);
		}
		break;
	case SYBINT1: 
		tdsdump_log(TDS_DBG_FUNC, "type %s has value %d\n", typename, (int)*(TDS_TINYINT*)col->column_data); 
		break;
	case SYBINT2: 
		tdsdump_log(TDS_DBG_FUNC, "type %s has value %d\n", typename, (int)*(TDS_SMALLINT*)col->column_data); 
		break;
	case SYBINT4: 
		tdsdump_log(TDS_DBG_FUNC, "type %s has value %d\n", typename, (int)*(TDS_INT*)col->column_data); 
		break;
	case SYBREAL: 
		tdsdump_log(TDS_DBG_FUNC, "type %s has value %f\n", typename, (double)*(TDS_REAL*)col->column_data); 
		break;
	case SYBFLT8: 
		tdsdump_log(TDS_DBG_FUNC, "type %s has value %f\n", typename, (double)*(TDS_FLOAT*)col->column_data); 
		break;
	default:
		tdsdump_log(TDS_DBG_FUNC, "cannot log data for type %s\n", typename);
		break;
	}
}
