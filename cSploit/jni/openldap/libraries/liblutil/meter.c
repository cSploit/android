/* meter.c - lutil_meter meters */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright (c) 2009 by Emily Backes, Symas Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Emily Backes for inclusion
 * in OpenLDAP software.
 */

#include "portable.h"
#include "lutil_meter.h"

#include <ac/assert.h>
#include <ac/string.h>

int
lutil_time_string (
	char *dest,
	int duration,
	int max_terms)
{
	static const int time_div[] = {31556952,
				       604800,
				       86400,
				       3600,
				       60,
				       1,
				       0};
	const int * time_divp = time_div;
	static const char * time_name_ch = "ywdhms";
	const char * time_name_chp = time_name_ch;
	int term_count = 0;
	char *buf = dest;
	int time_quot;
	
	assert ( max_terms >= 2 ); /* room for "none" message */

	if ( duration < 0 ) {
		*dest = '\0';
		return 1;
	}
	if ( duration == 0 ) {
		strcpy( dest, "none" );
		return 0;
	}
	while ( term_count < max_terms && duration > 0 ) {
		if (duration > *time_divp) {
			time_quot = duration / *time_divp;
			duration %= *time_divp;
			if (time_quot > 99) {
				return 1;
			} else {
				*(buf++) = time_quot / 10 + '0';
				*(buf++) = time_quot % 10 + '0';
				*(buf++) = *time_name_chp;
				++term_count;
			}
		}
		if ( *(++time_divp) == 0) duration = 0;
		++time_name_chp;
	}
	*buf = '\0';
	return 0;
}

int
lutil_get_now (double *now)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;

	assert( now );
	gettimeofday( &tv, NULL );
	*now = ((double) tv.tv_sec) + (((double) tv.tv_usec) / 1000000.0);
	return 0;
#else
	time_t tm;

	assert( now );
	time( &tm );
	*now = (double) tm;
	return 0;
#endif
}

int
lutil_meter_open (
	lutil_meter_t *meter,
	const lutil_meter_display_t *display, 
	const lutil_meter_estimator_t *estimator,
	unsigned long goal_value)
{
	int rc;

	assert( meter != NULL );
	assert( display != NULL );
	assert( estimator != NULL );

	if (goal_value < 1) return -1;

	memset( (void*) meter, 0, sizeof( lutil_meter_t ));
	meter->display = display;
	meter->estimator = estimator;
	lutil_get_now( &meter->start_time );
	meter->last_update = meter->start_time;
	meter->goal_value = goal_value;
	meter->last_position = 0;

	rc = meter->display->display_open( &meter->display_data );
	if( rc != 0 ) return rc;
	
	rc = meter->estimator->estimator_open( &meter->estimator_data );
	if( rc != 0 ) {
		meter->display->display_close( &meter->display_data );
		return rc;
	}
	
	return 0;
}

int
lutil_meter_update (
	lutil_meter_t *meter,
	unsigned long position,
	int force)
{
	static const double display_rate = 0.5;
	double frac, cycle_length, speed, now;
	time_t remaining_time, elapsed;
	int rc;

	assert( meter != NULL );

	lutil_get_now( &now );

	if ( !force && now - meter->last_update < display_rate ) return 0;

	frac = ((double)position) / ((double) meter->goal_value);
	elapsed = now - meter->start_time;
	if (frac <= 0.0) return 0;
	if (frac >= 1.0) {
		rc = meter->display->display_update(
			&meter->display_data,
			1.0,
			0,
			(time_t) elapsed,
			((double)position) / elapsed);
	} else {
		rc = meter->estimator->estimator_update( 
			&meter->estimator_data, 
			meter->start_time,
			frac,
			&remaining_time );
		if ( rc == 0 ) {
			cycle_length = now - meter->last_update;
			speed = cycle_length > 0.0 ?
				((double)(position - meter->last_position)) 
				/ cycle_length :
				0.0;
			rc = meter->display->display_update(
				&meter->display_data,
				frac,
				remaining_time,
				(time_t) elapsed,
				speed);
			if ( rc == 0 ) {
				meter->last_update = now;
				meter->last_position = position;
			}
		}
	}

	return rc;
}

int
lutil_meter_close (lutil_meter_t *meter)
{
	meter->estimator->estimator_close( &meter->estimator_data );
	meter->display->display_close( &meter->display_data );

	return 0;
}

/* Default display and estimator */
typedef struct {
	int buffer_length;
	char * buffer;
	int need_eol;
	int phase;
	FILE *output;
} text_display_state_t;

static int
text_open (void ** display_datap)
{
	static const int default_buffer_length = 81;
	text_display_state_t *data;

	assert( display_datap != NULL );
	data = calloc( 1, sizeof( text_display_state_t ));
	assert( data != NULL );
	data->buffer_length = default_buffer_length;
	data->buffer = calloc( 1, default_buffer_length );
	assert( data->buffer != NULL );
	data->output = stderr;
	*display_datap = data;
	return 0;
}

static int
text_update ( 
	void **display_datap,
	double frac,
	time_t remaining_time,
	time_t elapsed,
	double byte_rate)
{
	text_display_state_t *data;
	char *buf, *buf_end;

	assert( display_datap != NULL );
	assert( *display_datap != NULL );
	data = (text_display_state_t*) *display_datap;

	if ( data->output == NULL ) return 1;

	buf = data->buffer;
	buf_end = buf + data->buffer_length - 1;

/* |#################### 100.00% eta  1d19h elapsed 23w 7d23h15m12s spd nnnn.n M/s */

	{
		/* spinner */
		static const int phase_mod = 8;
		static const char phase_char[] = "_.-*\"*-.";
		*buf++ = phase_char[data->phase % phase_mod];
		data->phase++;
	}

	{
		/* bar */
		static const int bar_length = 20;
		static const double bar_lengthd = 20.0;
		static const char fill_char = '#';
		static const char blank_char = ' ';
		char *bar_end = buf + bar_length;
		char *bar_pos = frac < 0.0 ? 
			buf :
			frac < 1.0 ?
			buf + (int) (bar_lengthd * frac) :
			bar_end;

		assert( (buf_end - buf) > bar_length );
		while ( buf < bar_end ) {
			*buf = buf < bar_pos ?
				fill_char : blank_char;
			++buf;
		}
	}

	{
		/* percent */
		(void) snprintf( buf, buf_end-buf, "%7.2f%%", 100.0*frac );
		buf += 8;
	}

	{
		/* eta and elapsed */
		char time_buffer[19];
		int rc;
		rc = lutil_time_string( time_buffer, remaining_time, 2);
		if (rc == 0)
			snprintf( buf, buf_end-buf, " eta %6s", time_buffer );
		buf += 5+6;
		rc = lutil_time_string( time_buffer, elapsed, 5);
		if (rc == 0)
			snprintf( buf, buf_end-buf, " elapsed %15s", 
				  time_buffer );
		buf += 9+15;
	}

	{
		/* speed */
		static const char prefixes[] = " kMGTPEZY";
		const char *prefix_chp = prefixes;

		while (*prefix_chp && byte_rate >= 1024.0) {
			byte_rate /= 1024.0;
			++prefix_chp;
		}
		if ( byte_rate >= 1024.0 ) {
			snprintf( buf, buf_end-buf, " fast!" );
			buf += 6;
		} else {
			snprintf( buf, buf_end-buf, " spd %5.1f %c/s",
				  byte_rate,
				  *prefix_chp);
			buf += 5+6+4;
		}
	}

	(void) fprintf( data->output,
			"\r%-79s", 
			data->buffer );
	data->need_eol = 1;
	return 0;
}

static int
text_close (void ** display_datap)
{
	text_display_state_t *data;

	if (display_datap) {
		if (*display_datap) {
			data = (text_display_state_t*) *display_datap;
			if (data->output && data->need_eol) 
				fputs ("\n", data->output);
			if (data->buffer)
				free( data->buffer );
			free( data );
		}
		*display_datap = NULL;
	}
	return 0;
}

static int
null_open_close (void **datap)
{
	assert( datap );
	*datap = NULL;
	return 0;
}

static int
linear_update (
	void **estimator_datap, 
	double start, 
	double frac, 
	time_t *remaining)
{
	double now;
	double elapsed;
	
	assert( estimator_datap != NULL );
	assert( *estimator_datap == NULL );
	assert( start > 0.0 );
	assert( frac >= 0.0 );
	assert( frac <= 1.0 );
	assert( remaining != NULL );
	lutil_get_now( &now );

	elapsed = now-start;
	assert( elapsed >= 0.0 );

	if ( frac == 0.0 ) {
		return 1;
	} else if ( frac >= 1.0 ) {
		*remaining = 0;
		return 0;
	} else {
		*remaining = (time_t) (elapsed/frac-elapsed+0.5);
		return 0;
	}
}

const lutil_meter_display_t lutil_meter_text_display = {
	text_open, text_update, text_close
};

const lutil_meter_estimator_t lutil_meter_linear_estimator = {
	null_open_close, linear_update, null_open_close
};
