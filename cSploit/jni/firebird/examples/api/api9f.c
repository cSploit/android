/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include <stdio.h>
#include <ibase.h>
#include "example.h"

#ifndef CHAR
#define CHAR        unsigned char
#endif

#ifndef SHORT
#define SHORT       unsigned short
#endif

static void set_statistics (char*, ISC_BLOB_CTL);
static int read_text (SHORT, ISC_BLOB_CTL);
static int dump_text (SHORT, ISC_BLOB_CTL);
static int caller (SHORT, ISC_BLOB_CTL, SHORT, CHAR*, SHORT*);

#define	ACTION_open 0
#define	ACTION_get_segment 1
#define	ACTION_close 2
#define	ACTION_put_segment 3
#define	ACTION_create 4

#define FB_SUCCESS			0
#define FB_FAILURE			1

#define BUFFER_LENGTH		512

#ifdef SHLIB_DEFS
#define system		(*_libfun_system)
#define fopen		(*_libfun_fopen)
#define unlink		(*_libfun_unlink)
#define fprintf		(*_libfun_fprintf)
#define fclose		(*_libfun_fclose)
#define fgetc		(*_libfun_fgetc)

extern int	system();
extern FILE	*fopen();
extern int	unlink();
extern int	fprintf();
extern int	fclose();
extern int	fgetc();
#endif

static char prevbuf[BUFFER_LENGTH + 1];
static int width = 40;

int EXPORT desc_filter (SHORT action, ISC_BLOB_CTL control)
{
/**************************************
 *
 *	d e s c _  f i l t e r 
 *
 **************************************
 *
 * Functional description
 *	Format a blob into 40-character-wide
 *  paragraphs.
 *  Read the blob into a file and process
 *	it on open, then read it back line by
 *	line in the get_segment loop.
 *
 **************************************/
int	status;
FILE	*text_file;
char	*out_buffer;

switch (action)
    {
	case ACTION_open:
		prevbuf[0] = '\0';
		if (status = dump_text (action, control))
			return status;
		set_statistics("desc.txt", control);  /* set up stats in ctl struct */
		break;

	case ACTION_get_segment:
		/* open the file first time through and save the file pointer */
		if (!control->ctl_data [0])
		{
			text_file = fopen ("desc.txt", "r");
			control->ctl_data[0] = (long) text_file;
		}
		if (status = read_text (action, control))
			return status;
		break;

	case ACTION_close:
		/* don't need the temp files any more, so clean them up */
		/*unlink("desc.txt");*/
		break;

	case ACTION_create:
	case ACTION_put_segment:
		return isc_uns_ext;
    }

	return FB_SUCCESS;
}

static int caller (SHORT action, ISC_BLOB_CTL control, SHORT buffer_length,
    CHAR *buffer, SHORT *return_length)
{
/**************************************
 *
 *        c a l l e r
 *
 **************************************
 *
 * Functional description
 *        Call next source filter.  This 
 *        is a useful service routine for
 *        all blob filters.
 *
 **************************************/
int        status;
ISC_BLOB_CTL        source;

source = control->ctl_source_handle;
source->ctl_status = control->ctl_status;
source->ctl_buffer = buffer;
source->ctl_buffer_length = buffer_length;

status = (*source->ctl_source) (action, source);

if (return_length)
    *return_length = source->ctl_segment_length;

return status;
}


static int dump_text (SHORT action, ISC_BLOB_CTL control)
{
/**************************************
 *
 *	d u m p _ t e x t 
 *
 **************************************
 *
 * Functional description
 *	Open a blob and write the
 *	contents to a file
 *
 **************************************/
	FILE	*text_file;
	CHAR	buffer [BUFFER_LENGTH + 1];
	SHORT	length;
	int		status;
	int		i, j;
	CHAR	tbuf [BUFFER_LENGTH + 1];

 
	if (!(text_file = fopen ("desc.txt", "w")))
		return FB_FAILURE;

	while (!(status = caller (ACTION_get_segment, control, sizeof(buffer) - 1,
		buffer, &length)))
    {
		buffer[length] = 0; 
		sprintf(tbuf, "%s%s", prevbuf, buffer);

		length = strlen(tbuf);

		/* replace any new-lines with space */
        for (i = 0; i < length; i++) {
            if (tbuf[i] == '\n')    
                tbuf[i] = ' '; 
        } 
 
		i = 0;
		/* break the line up into width-length pieces */
		for (; i < length; i++)
		{
			/* save the remainder */
            if (strlen(&tbuf[i]) <= 40) {
                sprintf(prevbuf, "%s ", &tbuf[i]);
                break;
            }
 
			/* find end-of-word to break the line at */
            for (j = width - 1; j >= 0; j--) {
                if (isspace(tbuf[i + j]))
                    break;
            }
            if (j < 0)
                j = width;
            fprintf (text_file, "%*.*s\n", j, j, &tbuf[i]);
 
            i = i + j;
		}
    }
	/* print the remainder */
	fprintf(text_file, "%s\n", prevbuf);

	fclose (text_file);

	if (status != isc_segstr_eof)
		return status;

	return FB_SUCCESS;
}

static void set_statistics (char* filename, ISC_BLOB_CTL control)
{
/*************************************
 *
 *	s e t _ s t a t i s t i c s
 *
 *************************************
 *
 * Functional description
 *	Sets up the statistical fields
 *	in the passed in ctl structure.
 *	These fields are:
 *	  ctl_max_segment - length of
 *	     longest seg in blob (in
 *	     bytes)
 *	  ctl_number_segments - # of
 *	     segments in blob
 *	  ctl_total_length - total length
 *	     of blob in bytes.
 *	we should reset the
 *	ctl structure, so that 
 *	blob_info calls get the right
 *	values.
 *
 *************************************/
int	max_seg_length;
int	length;
int	num_segs;
int	cur_length;
FILE		*file;
char		*p;
char		c;

num_segs = 0;
length = 0;
max_seg_length = 0;
cur_length = 0;

file = fopen ("desc.txt", "r");

while (1)
    {
    c = fgetc (file);
    if (feof (file))
	break;
    length++;
    cur_length++;

    if (c == '\n')    /* that means we are at end of seg */
	{
	if (cur_length > max_seg_length)
	    max_seg_length = cur_length;
	num_segs++;
	cur_length = 0;
	}
    }

control->ctl_max_segment = max_seg_length;
control->ctl_number_segments = num_segs;
control->ctl_total_length = length;
}

static int read_text (SHORT action, ISC_BLOB_CTL control)
{
/**************************************
 *
 *	r e a d _ t e x t 
 *
 **************************************
 *
 * Functional description
 *	Reads a file one line at a time 
 *	and puts the data out as if it 
 *	were coming from a blob. 
 *
 **************************************/
CHAR	*p;
FILE	*file;
SHORT	length;
int	c, status;


p = control->ctl_buffer;
length = control->ctl_buffer_length;
file = (FILE *)control->ctl_data [0];

for (;;)
{
	c = fgetc (file);
	if (feof (file))
		break;
	*p++ = c;
	if ((c == '\n')  || p >= control->ctl_buffer + length)
	{
		control->ctl_segment_length = p - control->ctl_buffer; 
		if (c == '\n')
			return FB_SUCCESS; 
		else
			return isc_segment;
	}
}

return isc_segstr_eof;
}

