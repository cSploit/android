/*====================================================================
  File Name:	udflib.c
  Description:
  This module contains some user defined functions (UDF).  
  The test suite for UDF will use udf.sql
  to define UDF to the database using SQL statements.
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
====================================================================== */

#include <stdlib.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <string.h>
#include <math.h>
#include <ibase.h>
#include "example.h"
#include "ib_util.h"

#define BADVAL -9999L
#define MYBUF_LEN 15		/* number of chars to get for */

typedef struct blob {
	short	(*blob_get_segment) ();
	void	*blob_handle;
	int	blob_number_segments;
	int	blob_max_segment;
	int	blob_total_length;
	void	(*blob_put_segment) ();
} *BLOB;

time_t	time();
char	*ctime();

/* variable to return values in.  Needs to be static so it doesn't go 
   away as soon as the function invocation is finished */

char	buffer[256];
char	buffer2[512];	/* for string concatenation */
char	datebuf[12];	/* for date string */

int	r_long;
double	r_double;
float	r_float;
short	r_short;

struct	tm *tbuf;
int	time_sec;

ISC_QUAD newdate;


/*===============================================================
 fn_lower_c() - Puts its argument longo lower case, for C programs
 Input is of VARCHAR, output is of CSTRING.
 Not international or non-ascii friendly.
================================================================= */
char* EXPORT fn_lower_c(char* s) /* VARCHAR input */
{
	char *buf;
	short length = 0;

	char *buffer = (char *)ib_util_malloc(256);

	length = (short)*s;
	s += 2;
	buf = buffer;
	while (*s)
		if (*s >= 'A' && *s <= 'Z')
			*buf++ = *s++ - 'A' + 'a';
		else
			*buf++ = *s++;

	*buf = '\0';
	buffer [length] = '\0';

	return buffer;
}

/*===============================================================
 fn_strcat(s1, s2) - Returns concatenated string. 
	s1 and s2 are varchar to get a length count
================================================================= */

char* EXPORT fn_strcat(char* s1, char* s2)
{
    short j = 0;
	short length1, length2;
	char *p;

	char *buffer2 = (char *)ib_util_malloc(512);
	
	length1 = (short)*s1;
	length2 = (short)*s2;

	s1 += 2;
	s2 += 2;

	/* strip trailing blanks of s1 */
	p = s1 + length1 - 1;
	while (*p  && *p == ' ')
		p--;
	p++;
	*p = '\0';

	p = buffer2;
	while (*s1)
		*p++ = *s1++;

	for (j = 0; j < length2; j++)
		if (*s2)
			*p++ = *s2++;

	*p = '\0';
        return buffer2; 
}

/*===============================================================
 fn_substr(s, m, n) - Returns the substr starting m ending n in s. 
================================================================= */
char* EXPORT fn_substr(char* s,
	short* m, /* starting position */
	short* n) /* ending position */
{
    short i = 0;
	short j = 0;

	char *buffer = (char *)ib_util_malloc(256);

        if (*m > *n || *m < 1 || *n < 1) return "Bad parameters in substring"; 

        while (*s && i++ < *m-1) /* skip */
                s++;

        while (*s && i++ <= *n)  /* copy */
                buffer[j++] = *s++;
        buffer[j] = '\0';

        return buffer;
}

/*===============================================================
 fn_trim(s) - Returns string that has leading blanks trimmed. 
================================================================= */
char* EXPORT fn_trim(char* s)
{
	short j = 0;

	char *buffer = (char *)ib_util_malloc(256);

        while (*s == ' ')       /* skip leading blanks */
                s++;

        while (*s)		/* copy the rest */
                buffer[j++] = *s++;

        buffer[j] = '\0';

        return buffer;
}

/*===============================================================
 fn_trunc(s, m) - Returns the string truncated at position m; 
 Input is of CSTRING, output is of VARCHAR.
================================================================= */
char* EXPORT fn_trunc(char* s, short* m)
{
	short j = 2;	/* leave 1st 2 bytes for VARCHAR output */

	char *buffer = (char *)ib_util_malloc(256);

	while (*s && j < *m + 2)	/* need to add 2 */
		buffer[j++] = *s++;

	buffer[j] = '\0';

	buffer[0] = (unsigned short) strlen(s) + 2;
	buffer[1] = ' ';	/* anything other than \0 */

/*
	*((unsigned short *)buffer) = (unsigned short) (strlen(buffer) + 2);
*/

        return buffer;		/* VARCHAR output */
}


/* ==============================================================
   fn_doy() return the nth day of the year, by value.
   ============================================================== */
int EXPORT fn_doy()
{
	char buf[4];	/* for day */
	int i;

        time (&time_sec);
        tbuf = localtime(&time_sec);

	i = strftime(buf, 4, "%j", tbuf);
	return atoi (buf);
}

/* ==============================================================
   fn_moy() return the nth month of the year. e.g. 1,2,3,...12.
   Return by reference.
   ============================================================== */
short* EXPORT fn_moy()
{
        time (&time_sec);
        tbuf = localtime(&time_sec);
	r_short = (short) tbuf->tm_mon + 1;

	return &r_short;
}

/* ==============================================================
   fn_dow() return the day of today. e.g., Monday, Friday. ...
   ============================================================== */
char* EXPORT fn_dow()
{
        time (&time_sec);
        tbuf = localtime(&time_sec);

        switch (tbuf->tm_wday) {
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        case 7: return "Sunday";
        default: return "Error in Date";
        }
}
 
/*===============================================================
 fn_sysdate() - Returns the system date in the "MMM-DD-YYYY" format.
================================================================= */
char* EXPORT fn_sysdate()
{
        short len, i, j = 0;
 
        char *time_str;

	char *datebuf = (char *)ib_util_malloc(12);
 
        time (&time_sec);
        time_str = ctime (&time_sec);
        len = strlen(time_str);

        for (i = 4; i <= 10; i++) {
                if (*(time_str + i) != ' ')
                        datebuf[j++] = *(time_str+i);
                else if (i == 7 || i == 10)
                        datebuf[j++] = '-';
                else
                        datebuf[j++] = '0';
        }
        for (i = 20; i < len-1 ; i++)
                datebuf[j++] = *(time_str+i);
 
        datebuf[j] = '\0';
        return datebuf;
}
 


/* ==============================================
  fn_add2 (a, b) - returns a + b
  =============================================== */

int EXPORT fn_add2(int* a, int* b)
{
	return (*a + *b);
}


/* ==================================================
  fn_mul (a, b) - returns a * b
 =================================================== */

double EXPORT fn_mul(double* a, double* b)
{
	return (*a * *b);
}


/* ==============================================
  fn_fact (n) - return factorial of n
 ================================================ */

double EXPORT fn_fact(double* n)
{
	double k;

	if (*n > 100) return BADVAL;
        if (*n < 0) return BADVAL;   

        if (*n == 0) return 1L;

        else {
		k = *n - 1L;
		return (*n * fn_fact(&k));
	}
}

/*===============================================================
 fn_abs() - returns the absolute value of its argument.
================================================================= */
double EXPORT fn_abs(double* x)
{
	return (*x < 0.0) ? -*x : *x;
}


/*===============================================================
 fn_max() - Returns the greater of its two arguments
================================================================ */
double EXPORT fn_max(double* a, double* b)
{
	return  (*a > *b) ? *a : *b;
}



/*===============================================================
 fn_sqrt() - Returns square root of n
================================================================ */
double* EXPORT fn_sqrt(double* n)
{
	r_double = sqrt(*n);
	return &r_double;
} 




/*=============================================================
 fn_blob_linecount() returns the number of lines in a blob 
  =============================================================*/

int EXPORT fn_blob_linecount(BLOB b)
{
    char *buf, *p;
	short length, actual_length;

	/* Null values */
	if (!b->blob_handle)
		return 0L;

	length = b->blob_max_segment + 1L;
	buf = (char *) malloc (length); 
 
	r_long = 0;
        while ((*b->blob_get_segment) (b->blob_handle, buf, length, &actual_length)) 
		{
		buf [actual_length] = 0;
		p = buf;
                while (*p)  
			if (*p++  == '\n')
				r_long++;
		}

        free (buf); 
        return r_long;  
}

/*=============================================================
 fn_blob_bytecount() returns the number of bytes in a blob 
 do not count newlines, so get rid of the newlines. 
 ==============================================================*/

int EXPORT fn_blob_bytecount(BLOB b)
{
	/* Null values */
	if (!b->blob_handle)
		return 0L;

        return (b->blob_total_length - fn_blob_linecount(b));  
}


 
/*=============================================================
 fn_substr_blob() returns portion of TEXT blob beginning at m th
 character and ended at n th character. 
 Newlines are eliminated to make for better printing.
  =============================================================*/
 
char* EXPORT fn_blob_substr(BLOB b, int* m, int* n)
{
	char *buf, *p, *q;
	int i = 0;
	int curr_bytecount = 0;
        int begin, end; 
	short length, actual_length;
 
	char *buffer = (char *)malloc(256);

	if (!b->blob_handle)
		return "<null>";
	length = b->blob_max_segment + 1L;
	buf = (char *) malloc (length); 


        if (*m > *n || *m < 1L || *n < 1L) 
		return "";
	if (b->blob_total_length < (int)*m) 
		return "";

	begin = *m;				/* beginning position */

	if (b->blob_total_length < (int)*n) 
		end = b->blob_total_length;	/* ending position */
	else
		end = *n;

	/* Limit the return string to 255 bytes */
	if (end - begin + 1L > 255L)
		end = begin + 254L;
	q = buffer;

        while ((*b->blob_get_segment) (b->blob_handle, buf, length, 
		&actual_length))
		{    
		buf [actual_length] = 0;

		p = buf;
		while (*p && (curr_bytecount <= end))
			{
			curr_bytecount++;
			if (*p == '\n')
				*p = ' ';
			if (curr_bytecount >= begin)
				*q++ = *p;
			p++;
			}
		if (curr_bytecount >= end) 
			{
			*q = 0;
			break;
			}
		}
 
	free (buf);
        return buffer;
}

