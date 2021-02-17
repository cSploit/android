/*************************************************************************
**
**	PROGRAM:		JRD file split utility program
**	MODULE:			spit.cpp
**	DESCRIPTION:	Command line interpreter for backup file split/join
**					utility program
**
**
**************************************************************************
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

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../common/gdsassert.h"
#include <stdarg.h>

#include <fcntl.h>
#include <ctype.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "../burp/split/spit.h"
#include "../common/classes/Switches.h"
#include "../burp/std_desc.h"
#include "../burp/burpswi.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

static const int mode_read	= O_RDONLY;
static const int mode_write	= O_WRONLY | O_CREAT;
static const int mask		= 0666;

static DESC open_platf(const char* name, int writeFlag)
{
#ifdef WIN_NT
	return CreateFile(name, (writeFlag ? GENERIC_WRITE : GENERIC_READ), 0, NULL,
		(writeFlag ? CREATE_ALWAYS : OPEN_EXISTING), FILE_ATTRIBUTE_NORMAL, 0);
#else
	return open(name, (writeFlag ? mode_write : mode_read), mask);
#endif
}

static int read_platf(DESC file, void* buf, int count)
{
#ifdef WIN_NT
	DWORD act;
	if (!ReadFile(file, buf, count, &act, NULL))
	{
		return -1;
	}
	return act;
#else
	return read(file, buf, count);
#endif
}

static int write_platf(DESC file, const void* buf, int count)
{
#ifdef WIN_NT
	DWORD act;
	if (!WriteFile(file, buf, count, &act, NULL))
	{
		return -1;
	}
	return act;
#else
	return write(file, buf, count);
#endif
}

static void close_platf(DESC file)
{
#ifdef WIN_NT
	CloseHandle(file);
#else
	close(file);
#endif
}

// Definitions for GSPLIT
enum gsplit_option
{
	IN_SW_SPIT_0	= 0,   // the unknowable switch
	IN_SW_SPIT_SP	= 30,  // splits back up files
	IN_SW_SPIT_JT	= 31  // joins back up files
};


const static Switches::in_sw_tab_t spit_in_sw_table[] =
{
	{IN_SW_SPIT_SP,	0,	"SPLIT_BK_FILE", 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_SPIT_JT,	0,	"JOIN_BK_FILE",	 0, 0, 0, false, 0, 0, NULL},
	{IN_SW_SPIT_0,	0,	NULL,			 0, 0, 0, false, 0, 0, NULL}
};


// backup files header record

struct header_rec
{
	TEXT name[18];
	TEXT date_time[30];
	TEXT text1[11];
	TEXT num[4];
	TEXT text2[4];
	TEXT total[4];
	TEXT text3[2];
	TEXT fl_name[MAX_FILE_NM_LEN];
};

const size_t header_rec_len = sizeof(header_rec);
static const char* const header_rec_name = "InterBase/gsplit, ";


// backup files structure

struct b_fil
{
	b_fil *b_fil_next;
	TEXT *b_fil_name;
	SLONG b_fil_number;
	SINT64 b_fil_size;
};

const size_t b_fil_len = sizeof(b_fil);


// local function declarations

static int conv_ntoc(SLONG, TEXT*);
static int free_file_list(b_fil*);
static int final_flush_io_buff(const UCHAR*, SLONG, FILE_DESC);
static int final_read_and_write(FILE_DESC, FILE_DESC, const TEXT*, SLONG, UCHAR**, bool*);
static int flush_io_buff(const UCHAR*, SLONG, FILE_DESC, SINT64, SLONG*, bool*);
static int get_file_name(const SCHAR*, SINT64, b_fil**);
static int get_file_size(const SCHAR*, const SCHAR*, SINT64*);
static int get_function_option(const SCHAR*, gsplit_option*, const SCHAR*, const Switches&);
static int gen_multy_bakup_files(b_fil*, FILE_DESC, SLONG);
static int set_hdr_str(TEXT*, const TEXT*, SLONG, SLONG);
static int join_multy_bakup_files(b_fil*);
static void print_clo(const TEXT*);
static int read_and_write(FILE_DESC, FILE_DESC, /*const TEXT*,*/ SLONG,
						  SINT64, UCHAR**, bool*, SINT64*, SLONG*);
static int read_and_write_for_join(FILE_DESC, const TEXT*, UCHAR*, SLONG, SLONG*);
static int write_header(const b_fil*, header_rec, FILE_DESC, TEXT*);



/*****************************************************
**
**	M A I N              P R O G R A M
**
******************************************************
*/

int main( int argc, char* argv[])
{
	const SCHAR* const prog_name = argv[0];

	if (argc < 2)
	{
		fprintf(stderr, "%s: No Command Line Option Specified\n", argv[0]);
		print_clo(prog_name);
		return FB_FAILURE;
	}

	// Fields initialization

	b_fil* file_ptr = NULL;
	b_fil* file_list = NULL;
	b_fil* prev_file = NULL;
	SINT64 file_size = -1;
	gsplit_option sw_replace = IN_SW_SPIT_0;


	// Initialize in_sw_table table.

	const Switches switches(spit_in_sw_table, FB_NELEM(spit_in_sw_table), false, false);

	// validating command line options

	SLONG ret_cd, file_num = 0;
	bool file_nm_sw = false;
	const SCHAR* string = NULL;

	const SCHAR* const* const end = argv + argc;
	++argv;
	while (argv < end)
	{
		string = *argv;
		if (*string == '-')
		{
			argv++;
			ret_cd = get_function_option(prog_name, &sw_replace, string, switches);
			if (ret_cd == FB_FAILURE)
			{
				free_file_list(file_list);
				return FB_FAILURE;
			}
		}						// end of processing (*string == '-')
		else
		{
			// processing function specific command line options

			switch (sw_replace)
			{
			case IN_SW_SPIT_SP:
				if (!file_nm_sw)
				{
					// process file name
					file_size = 0;
					file_num = file_num + 1;

					if (file_num > MAX_NUM_OF_FILES)
					{
						fprintf(stderr, "%s: maximum of files is %d\n", prog_name, MAX_NUM_OF_FILES);
						print_clo(prog_name);
						free_file_list(file_list);
						return FB_FAILURE;
					}

					if (strlen(string) > MAX_FILE_NM_LEN)
					{
						fprintf(stderr, "%s: file name %s is too long\n", prog_name, string);
						fprintf(stderr, "%s: maximum length of file name is %"SIZEFORMAT" bytes\n",
								   prog_name, MAX_FILE_NM_LEN);
						print_clo(prog_name);
						free_file_list(file_list);
						return FB_FAILURE;
					}

					ret_cd = get_file_name(string, file_size, &file_ptr);

					if (ret_cd == FB_FAILURE)
					{
						free_file_list(file_list);
						return FB_FAILURE;
					}

					file_nm_sw = true;
					file_ptr->b_fil_number = file_num;

					if (!file_list)
						file_list = prev_file = file_ptr;
					else
					{
						prev_file->b_fil_next = file_ptr;
						prev_file = file_ptr;
					}

				}				// processing file name
				else
				{
					// processing file size
					file_nm_sw = false;
					ret_cd = get_file_size(prog_name, string, &file_size);
					if (ret_cd == FB_FAILURE)
					{
						free_file_list(file_list);
						return FB_FAILURE;
					}
				}				// end of processing file size specification
				file_ptr->b_fil_size = file_size;
				break;

			case IN_SW_SPIT_JT:
				ret_cd = get_file_name(string, file_size, &file_ptr);

				if (ret_cd == FB_FAILURE)
				{
					free_file_list(file_list);
					return FB_FAILURE;
				}

				file_num = file_num + 1;
				file_ptr->b_fil_number = file_num;

				if (!file_list)
					file_list = prev_file = file_ptr;
				else
				{
					prev_file->b_fil_next = file_ptr;
					prev_file = file_ptr;
				}				// end of processing file size specification
				break;

			default:
				fprintf(stderr, "%s: invalid option '%s'\n", prog_name, string);
				print_clo(prog_name);
				free_file_list(file_list);
				return FB_FAILURE;
			}					// end of switch (sw_replace)

			argv++;
		}						// processing function specific command line options
	}							// while (argv < end)

	if (!file_list && sw_replace != IN_SW_SPIT_0)
	{
		fprintf(stderr, "%s: invalid option '%s', rest of parameters is missing\n", prog_name, string);
		print_clo(prog_name);
		free_file_list(file_list);
		return FB_FAILURE;
	}

	FILE_DESC input_file_desc;
	switch (sw_replace)
	{
	case IN_SW_SPIT_SP:
		input_file_desc = GBAK_STDIN_DESC();
		ret_cd = gen_multy_bakup_files(file_list, input_file_desc, file_num);
		if (ret_cd == FB_FAILURE)
		{
			fprintf(stderr, "%s: progam fails to generate multi-volumn back-up files\n", prog_name);
			free_file_list(file_list);
			return FB_FAILURE;
		}
		break;

	case IN_SW_SPIT_JT:
		ret_cd = join_multy_bakup_files(file_list);
		if (ret_cd == FB_FAILURE)
		{
			fprintf(stderr, "%s: progam fails to join multi-volumn back-up files\n", prog_name);
			free_file_list(file_list);
			return FB_FAILURE;
		}
		break;

	default:
		break;
	}

	// free all the storage allocated for backup files

	free_file_list(file_list);
	return FB_SUCCESS;

}								// end of main()


static int get_function_option(const SCHAR* prog_name,
							   gsplit_option* sw_replace,
							   const SCHAR* string,
							   const Switches& switches)
{
/********************************************************************
**
**	g e t _ f u n c t i o n _ o p t i o n
**
*********************************************************************
**
**	Functional description:
**
**		processing command line option and set proper function switch
**
*********************************************************************
*/

	if (strlen(string) == 1)
	{
		fprintf(stderr, "%s: invalid option '%s'\n", prog_name, string);
		print_clo(prog_name);
		return FB_FAILURE;
	}

	//gsplit_option op_specified = *sw_replace; Commented, where is it used here???
	const Switches::in_sw_tab_t* op = switches.findSwitch(string);
	if (!op)
	{
		fprintf(stderr, "%s: invalid option '%s'\n", prog_name, string);
		print_clo(prog_name);
		return FB_FAILURE;
	}

	if (*sw_replace == IN_SW_SPIT_0)
	{
		*sw_replace = (gsplit_option) op->in_sw;
		return FB_SUCCESS;
	}

	if (*sw_replace != op->in_sw)
	{
		fprintf(stderr, "%s: invalid option '%s', incompatible option\n", prog_name, string);
		print_clo(prog_name);
		return FB_FAILURE;
	}

	return FB_SUCCESS;
}


static int get_file_name( const SCHAR* string, SINT64 file_size, b_fil** file_ptr)
{
/********************************************************************
**
**	g e t _ f i l e _ n a m e
**
*********************************************************************
**
**	Functional description:
**
**		processing file name specification and returns file structure
**		pointer.
**
*********************************************************************
*/

	b_fil* const temp_ptr = (b_fil*) malloc(b_fil_len);

	*file_ptr = temp_ptr;
	temp_ptr->b_fil_name = (TEXT*) malloc(strlen(string) + 1);

	temp_ptr->b_fil_next = NULL;
	strcpy(temp_ptr->b_fil_name, string);
	temp_ptr->b_fil_number = 0;
	temp_ptr->b_fil_size = file_size;

	return FB_SUCCESS;
}


static int get_file_size(const SCHAR* prog_name, const SCHAR* string, SINT64* file_size)
{
/********************************************************************
**
**	g e t _ f i l e _ s i z e
**
*********************************************************************
**
**	Functional description:
**
**		processing file size specification and returns calculated
**		file size.
**
*********************************************************************
*/

	UCHAR c;

	for (const SCHAR* p = string; c = *p++;)
	{
		if (c == '\0')
			break;

		if (isdigit(c))
			*file_size = *file_size * 10 + (c - '0');
		else
		{
			if ((isalpha(c)) && (UPPER(c) == 'G' || UPPER(c) == 'K' || UPPER(c) == 'M'))
			{
				SLONG size_indicator = 0;
				switch (UPPER(c))
				{
				case 'K':
					size_indicator = K_BYTES;
					break;

				case 'M':
					size_indicator = M_BYTES;
					break;

				case 'G':
					size_indicator = G_BYTES;
					break;

				default:
					break;
				}				// end of switch( UPPER (c) )

				*file_size = *file_size * size_indicator;
			}
			else
			{
				// invalid size indicator

				fprintf(stderr, "%s: invalid size indicator '%s'\n", prog_name, string);
				print_clo(prog_name);
				return FB_FAILURE;
			}
		}
	}
	if (*file_size < MIN_FILE_SIZE)
	{
		// handling user specifies file size 0
		fprintf(stderr, "%s: invalid option '%s', minimum file size is 1 megabyte\n",
				   prog_name, string);
		print_clo(prog_name);
		return FB_FAILURE;
	}

	return FB_SUCCESS;
}


static int gen_multy_bakup_files(b_fil* file_list, FILE_DESC input_file_desc, SLONG file_num)
{
/********************************************************************
**
**	g e n _ m u l t y _ b a c k u p _ f i l e s
**
*********************************************************************
**
**	Functional description:
**
**		processing input data from stdin and splits the data into
**		multiple back-up files.
**
**		allocates an 16K bytes I/O buffer
**		intilializes header record common fields
**		do forever
**			walk through the backup file chain
**			intilializes header record unique fields
**			open backup file
**			writes out header record to backup file
**			points to the next backup file in the chain
**			calculates the actual file size ( minus header record length )
**			if the actual file size less than 16K bytes
**				set I/O size to actual file size
**			otherwise
**				set I/O size to 16K byte long
**			when it is the last backup file
**				reads data from standard input as much as indicated by I/O size
**					and writes it out to the last backup file until no EOF.
**				issues error message when disk space full condition is detected
**			otherwise reads and writes to backup files util EOF
**				if disk full cobdition is detected
**					flush the remaining data in the I/O buffer to subsequence
**							backup files
**				go back to normal read and write process util EOF
**
*********************************************************************
*/

	SLONG byte_write, ret_cd;
	TEXT header_str[header_rec_len], num_arr[5];
	header_rec hdr_rec;

	// CVC: there's a can of worms here. First, this function assumes it can free
	// the io_buffer's allocated memory without keeping a second copy of that pointer.
	// However, io_buffer can't be declared UCHAR* const because its address is
	// passed to final_read_and_write() and read_and_write() and both functions
	// thus suggest, by taking a UCHAR** that they can change the pointer's address;
	// but in practice they never affect it, so fixing those functions to take simply
	// UCHAR* would allow the correct declaration for io_buffer to succeed.
	//UCHAR* const io_buffer = (UCHAR *) malloc(IO_BUFFER_SIZE);
	UCHAR* io_buffer = (UCHAR *) malloc(IO_BUFFER_SIZE);

	if (!io_buffer)
	{
		fprintf(stderr, "I/O buffer allocation failed\n");
		return FB_FAILURE;
	}

	size_t pos;
	for (pos = 0; pos < header_rec_len; pos++)
		header_str[pos] = BLANK;

	pos = 0;
	ret_cd = set_hdr_str(header_str, header_rec_name, pos, sizeof(hdr_rec.name));
	size_t indx;
	for (indx = 0; indx < sizeof(hdr_rec.name); indx++)
		hdr_rec.name[indx] = BLANK;

	pos = pos + sizeof(hdr_rec.name);
	time_t clock = time(0);  // was SLONG
	ret_cd = set_hdr_str(header_str, ctime(&clock), pos, sizeof(hdr_rec.date_time));
	for (indx = 0; indx < sizeof(hdr_rec.date_time); indx++)
		hdr_rec.date_time[indx] = BLANK;

	pos = pos + sizeof(hdr_rec.date_time);
	ret_cd = set_hdr_str(header_str, ", file No. ", pos, sizeof(hdr_rec.text1));
	for (indx = 0; indx < sizeof(hdr_rec.text1); indx++)
		hdr_rec.text1[indx] = BLANK;

	for (indx = 0; indx < sizeof(hdr_rec.num); indx++)
		hdr_rec.num[indx] = BLANK;

	pos = pos + sizeof(hdr_rec.text1) + sizeof(hdr_rec.num);
	ret_cd = set_hdr_str(header_str, " of ", pos, sizeof(hdr_rec.text2));
	for (indx = 0; indx < sizeof(hdr_rec.text2); indx++)
		hdr_rec.text2[indx] = BLANK;

	ret_cd = conv_ntoc(file_num, num_arr);
	if (ret_cd == FB_FAILURE)
	{
		free(io_buffer);
		fprintf(stderr, "gsplit could not convert numeric data to character data\n");
		return FB_FAILURE;
	}

	num_arr[sizeof(num_arr) - 1] = TERMINAL;
	pos = pos + sizeof(hdr_rec.text2);
	ret_cd = set_hdr_str(header_str, num_arr, pos, sizeof(hdr_rec.total));
	for (indx = 0; indx < sizeof(hdr_rec.total); indx++)
		hdr_rec.total[indx] = BLANK;

	pos = pos + sizeof(hdr_rec.total);
	ret_cd = set_hdr_str(header_str, ", ", pos, sizeof(hdr_rec.text3));
	for (indx = 0; indx < sizeof(hdr_rec.text3); indx++)
		hdr_rec.text3[indx] = BLANK;

	for (indx = 0; indx < sizeof(hdr_rec.fl_name); indx++)
		hdr_rec.fl_name[indx] = BLANK;

	FILE_DESC output_fl_desc = 0;
	bool end_of_input = false, flush_done = false;
	const TEXT* file_name = NULL;
	SLONG io_size = 0;
	b_fil* fl_ptr = file_list;

	SINT64 byte_read = 0;
	SINT64 file_size = 0;

	while (true)
	{
		if (fl_ptr != NULL)
		{
			byte_read = 0;
			byte_write = 0;
			if (!fl_ptr->b_fil_next && (fl_ptr->b_fil_size == 0))
			{
				fl_ptr->b_fil_size = MIN_FILE_SIZE;
			}

			file_size = fl_ptr->b_fil_size - header_rec_len;
			file_name = fl_ptr->b_fil_name;

			output_fl_desc = open_platf(file_name, 1);
			if (output_fl_desc == INVALID_HANDLE_VALUE)
			{
				free(io_buffer);
				fprintf(stderr, "can not open back up file %s\n", file_name);
				return FB_FAILURE;
			}

			ret_cd = write_header(fl_ptr, hdr_rec, output_fl_desc, header_str);
			if (ret_cd == FB_FAILURE)
			{
				free(io_buffer);
				fprintf(stderr, "could not write header record to file %s\n", file_name);
				return FB_FAILURE;
			}

			fl_ptr = fl_ptr->b_fil_next;
		}

		if (file_size < IO_BUFFER_SIZE)
			io_size = (SLONG) file_size;
		else
			io_size = IO_BUFFER_SIZE;

		if (!fl_ptr)
		{
			while (!end_of_input)
			{
				ret_cd = final_read_and_write(input_file_desc, output_fl_desc,
											  file_name, io_size, &io_buffer, &end_of_input);
				if (ret_cd == FB_FAILURE)
				{
					free(io_buffer);
					return FB_FAILURE;
				}

				if (end_of_input)
				{
					free(io_buffer);
					return FB_SUCCESS;
				}
			}
		}
		else
		{
			while ((file_size > byte_read) && (fl_ptr != NULL))
			{
				ret_cd = read_and_write(input_file_desc, output_fl_desc,
										/*file_name,*/ io_size, file_size,
										&io_buffer, &end_of_input,
										&byte_read, &byte_write);
				switch (ret_cd)
				{
				case FB_FAILURE:
					free(io_buffer);
					return FB_FAILURE;

				case FILE_IS_FULL:
					{
						byte_read = 0;	// reset byte read count, prepare for next read

						const UCHAR* remaining_io = io_buffer + byte_write;
						SLONG remaining_io_len = IO_BUFFER_SIZE - byte_write;
						while (!flush_done && (fl_ptr != NULL))
						{
							if (!fl_ptr->b_fil_next && fl_ptr->b_fil_size == 0)
								fl_ptr->b_fil_size = MIN_FILE_SIZE;

							file_size = fl_ptr->b_fil_size - header_rec_len;
							file_name = fl_ptr->b_fil_name;

							output_fl_desc = open_platf(file_name, 1);
							if (output_fl_desc == INVALID_HANDLE_VALUE)
							{
								free(io_buffer);
								fprintf(stderr, "can not open back up file %s\n", file_name);
								return FB_FAILURE;
							}
							ret_cd = write_header(fl_ptr, hdr_rec, output_fl_desc, header_str);
							if (ret_cd == FB_FAILURE)
							{
								free(io_buffer);
								fprintf(stderr, "fail to write header rec to file %s\n", file_name);
								return FB_FAILURE;
							}

							fl_ptr = fl_ptr->b_fil_next;
							if (!fl_ptr)
							{
								ret_cd = final_flush_io_buff(remaining_io,
															 remaining_io_len,
															 output_fl_desc);
								if (ret_cd == FB_FAILURE)
								{
									fprintf(stderr, "gsplit could not do backup due");
									fprintf(stderr, " to lack of space or I/O problem\n");
									free(io_buffer);
									return FB_FAILURE;
								}
							}
							else
							{
								// got a lot of backup files

								ret_cd = flush_io_buff(remaining_io,
													   remaining_io_len,
													   output_fl_desc,
													   file_size,
													   &byte_write,
													   &flush_done);
								if (ret_cd == FB_FAILURE)
								{
									fprintf(stderr, "gsplit could not do backup due");
									fprintf(stderr, " I/O problem\n");
									free(io_buffer);
									return FB_FAILURE;
								}
								if (flush_done)
								{
									file_size = file_size - byte_write;
									byte_write = 0;
								}
								else
								{
									remaining_io = remaining_io + byte_write;
									remaining_io_len = remaining_io_len - byte_write;
								}
							}
						}		// end of while loop
						break;
					} // case FILE_IS_FULL

				default:
					break;
				}

				if (end_of_input)
				{
					free(io_buffer);
					return FB_SUCCESS;
				}
			}
		}
	}	// end of while ( true )
}


static int read_and_write(FILE_DESC input_file_desc,
						  FILE_DESC output_fl_desc,
						  //const TEXT* file_name,
						  SLONG io_size,
						  SINT64 file_size,
						  UCHAR** io_buffer,
						  bool* end_of_input,
						  SINT64* byte_read,
						  SLONG* byte_write)
{

/********************************************************************
**
**	r e a d _ a n d _ w r i t e
**
*********************************************************************
**
**	Functional description:
**
**		Read data from input file and write the data out to output
**		file.
**
*********************************************************************
*/

	SLONG read_cnt, last_read_size;

	// when number of byte read + number of byte goint to
	// be read is greater then file size, then calculate
	// the size for the last read and do the last read for
	// the current backup file. Otherwise read as mush data
	// as will fit in the current backup file.

	if (*byte_read + io_size > file_size)
	{
		last_read_size = (SLONG) (file_size - *byte_read);
		read_cnt = read_platf(input_file_desc, *io_buffer, last_read_size);
	}
	else
		read_cnt = read_platf(input_file_desc, *io_buffer, io_size);

	switch (read_cnt)
	{
	case 0:					// no more data to be read
		close_platf(output_fl_desc);
		*end_of_input = true;
		*byte_read = *byte_read + read_cnt;
		return FB_SUCCESS;

	case -1:					// read failed
		close_platf(output_fl_desc);
		fprintf(stderr,  "fail to read input from stdin, errno = %d\n", errno);
		return FB_FAILURE;

	default:					// read ok
		*byte_read = *byte_read + read_cnt;
		break;
	}

	const SLONG write_cnt = write_platf(output_fl_desc, *io_buffer, read_cnt);

	switch (write_cnt)
	{
	case -1:					// write failed
		close_platf(output_fl_desc);
		return FB_FAILURE;

	default:
		if (write_cnt == read_cnt)	// write ok
			return FB_SUCCESS;

		// write less data than it reads in
		close_platf(output_fl_desc);
		*byte_write = write_cnt;
		return FILE_IS_FULL;
	}
}


static int final_read_and_write(FILE_DESC input_file_desc,
								FILE_DESC output_fl_desc,
								const TEXT* file_name,
								SLONG io_size,
								UCHAR** io_buffer,
								bool* end_of_input)
{

/********************************************************************
**
**	f i n a l _ r e a d _ a n d _ w r i t e
**
*********************************************************************
**
**	Functional description:
**
**		Read all data from input file and write the data out to output
**		file.
**
*********************************************************************
*/

	const SLONG read_cnt = read_platf(input_file_desc, *io_buffer, io_size);

	switch (read_cnt)
	{
	case 0:					// no more data to be read
		close_platf(output_fl_desc);
		*end_of_input = true;
		return FB_SUCCESS;

	case -1:					// read failed
		close_platf(output_fl_desc);
		fprintf(stderr, "problem when reading input file, errno = %d\n", errno);
		return FB_FAILURE;

	default:					// read ok
		break;
	}

	const SLONG write_cnt = write_platf(output_fl_desc, *io_buffer, read_cnt);

	switch (write_cnt)
	{
	case -1:					// write failed
		close_platf(output_fl_desc);
		return FB_FAILURE;

	default:
		if (write_cnt == read_cnt)	// write ok
			return FB_SUCCESS;

		fprintf(stderr, "There is no enough space to write to back up file %s\n", file_name);
		close_platf(output_fl_desc);
		return FB_FAILURE;
	}
}


static int join_multy_bakup_files( b_fil* file_list)
{
/********************************************************************
**
**	j o i n _ m u l t y _ b a c k u p _ f i l e s
**
*********************************************************************
**
**	Functional description:
**
**		allocates I/O buffer and walks through backup files` chain.
**		calls read_and_write_for_join routine to read data from
**		backup file and to write data to standard output file
**		which later to be processed by gbak. Finally frees up
**		I/O buffer
**
*********************************************************************
*/

	FILE_DESC output_fl_desc = GBAK_STDOUT_DESC();

	// See comment near the beginning of gen_multy_bakup_files() as it
	// also applies to read_and_write_for_join().
	UCHAR* const io_buffer = (UCHAR*) malloc(IO_BUFFER_SIZE);
	//UCHAR* io_buffer = (UCHAR*) malloc(IO_BUFFER_SIZE);

	if (io_buffer == 0)
	{
		fprintf(stderr, "I/O buffer allocation failed\n");
		return FB_FAILURE;
	}

	SLONG cnt = 0, total_int = 0;
	// Why two variables to achieve this simple loop?
	const b_fil* next_fl = NULL;
	for (const b_fil* fl_ptr = file_list; fl_ptr; fl_ptr = next_fl)
	{
		cnt++;
		next_fl = fl_ptr->b_fil_next;
		const TEXT* file_name = fl_ptr->b_fil_name;

		SLONG ret_cd = read_and_write_for_join(output_fl_desc, file_name, io_buffer, cnt, &total_int);

		if (ret_cd == FB_FAILURE)
		{
			free(io_buffer);
			return FB_FAILURE;
		}

	}							// end of for loop

	free(io_buffer);
	return FB_SUCCESS;
}


static int read_and_write_for_join(FILE_DESC output_fl_desc,
								const TEXT* file_name,
								UCHAR* io_buffer,
								SLONG cnt,
								SLONG* total_int)
{
/********************************************************************
**
**	r e a d _ a n d _ w r i t e _ f o r _ j o i n
**
*********************************************************************
**
**	Functional description:
**
**		Reads data from backup files and writes to standard
**		output file.
**
*********************************************************************
*/

	TEXT num_arr[5], total_arr[5];
	header_rec hdr_rec;

	FILE_DESC input_fl_desc = open_platf(file_name, mode_read);

	if (input_fl_desc == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "can not open input file %s\n", file_name);
		return FB_FAILURE;
	}

	int read_cnt = read_platf(input_fl_desc, io_buffer, header_rec_len);
	if (read_cnt != static_cast<int>(header_rec_len))
	{
		close_platf(input_fl_desc);
		fprintf(stderr, "progam fails to read gsplit header record in back-up file%s\n", file_name);
		return FB_FAILURE;
	}

	const TEXT* char_ptr1 = reinterpret_cast<char*>(io_buffer);
	SLONG ret_cd = strncmp(char_ptr1, header_rec_name, sizeof(hdr_rec.name) - 1);
	if (ret_cd != 0)
	{
		close_platf(input_fl_desc);
		fprintf(stderr, "gsplit: expected GSPLIT description record\n");
		fprintf(stderr, "gsplit: Exiting before completion due to errors\n");
		return FB_FAILURE;
	}

	const SLONG skip_to_num = sizeof(hdr_rec.name) + sizeof(hdr_rec.date_time) + sizeof(hdr_rec.text1);
	const SLONG skip_to_total = skip_to_num + sizeof(hdr_rec.num) + sizeof(hdr_rec.text2);

	char_ptr1 = reinterpret_cast<char*>(io_buffer + skip_to_num);
	const TEXT* char_ptr2 = reinterpret_cast<char*>(io_buffer + skip_to_total);
	size_t indx;
	for (indx = 0; indx < sizeof(hdr_rec.num); indx++)
	{
		num_arr[indx] = *char_ptr1;
		char_ptr1++;
		if (cnt == 1)
		{
			total_arr[indx] = *char_ptr2;
			char_ptr2++;
		}
	}
	num_arr[indx] = '\0';
	const SLONG num_int = atoi(num_arr);
	if (cnt == 1)
	{
		total_arr[indx] = '\0';
		*total_int = atoi(total_arr);
	}

	if ((num_int != cnt) || (num_int > *total_int))
	{
		close_platf(input_fl_desc);
		fprintf(stderr, "gsplit: join backup file is out of sequence\n");
		fprintf(stderr, "gsplit: Exiting before completion due to errors\n");
		return FB_FAILURE;
	}

	read_cnt = read_platf(input_fl_desc, io_buffer, IO_BUFFER_SIZE);


	while (true)
	{
		switch (read_cnt)
		{
		case 0:				// no more data to be read
			close_platf(input_fl_desc);
			return FB_SUCCESS;

		case -1:				// read failed
			close_platf(input_fl_desc);
			return FB_FAILURE;

		default:				// this is the last read
			break;
		}

		SLONG write_cnt = write_platf(output_fl_desc, io_buffer, read_cnt);

		switch (write_cnt)
		{
		case -1:				// write failed
			close_platf(input_fl_desc);
			return FB_FAILURE;

		default:
			fb_assert(write_cnt == read_cnt);
			break;
		}

		read_cnt = read_platf(input_fl_desc, io_buffer, IO_BUFFER_SIZE);

	}	// end of while (true) loop
}


static int conv_ntoc( SLONG numeric_in, TEXT char_out[])
{
/********************************************************************
**
**	c o n v _ n t o c
**
*********************************************************************
**
**	Functional description:
**
**		Convert 4-digit numeric data to a 4-byte character array.
**
*********************************************************************
*/

	if (numeric_in <= 0)
		return FB_FAILURE;

	int i = numeric_in;
	int indx = 3;

	while (indx >= 0 && i > 0)
	{
		const int mod = i % 10;
		char_out[indx] = '0' + mod;
		--indx;
		i = i / 10;
	}

	if (i > 0) // number was too big
		return FB_FAILURE;

	for (; indx >= 0; indx--) {
		char_out[indx] = ' ';
	}
	return FB_SUCCESS;
}


static int write_header(const b_fil* fl_ptr,
						header_rec	hdr_rec,
						FILE_DESC	output_fl_desc,
						TEXT		header_str[])
{
/********************************************************************
**
**	w r i t e _ h e a d e r
**
*********************************************************************
**
**	Functional description:
**
**		writes out gsplit header record
**
**		assigns backup file name to header record file name array
**		calls conv_ntoc routine to convert numeric data to char data
**			and assigns to header record file number field
**		writes gsplit header record out to backup file
**
*********************************************************************
*/
	TEXT num_arr[5];

	SLONG ret_cd = conv_ntoc(fl_ptr->b_fil_number, num_arr);
	if (ret_cd == FB_FAILURE)
	{
		printf("gsplit could not convert numeric data to character data\n");
		return FB_FAILURE;
	}
	num_arr[sizeof(num_arr) - 1] = TERMINAL;
	SLONG pos = sizeof(hdr_rec.name) + sizeof(hdr_rec.date_time) + sizeof(hdr_rec.text1);
	ret_cd = set_hdr_str(header_str, num_arr, pos, sizeof(hdr_rec.num));

	const TEXT* file_name = fl_ptr->b_fil_name;
	pos = sizeof(hdr_rec.name) + sizeof(hdr_rec.date_time) +
		sizeof(hdr_rec.text1) + sizeof(hdr_rec.num) +
		sizeof(hdr_rec.text2) + sizeof(hdr_rec.total) + sizeof(hdr_rec.text3);
	ret_cd = set_hdr_str(header_str, file_name, pos, strlen(file_name));

	SLONG end, indx;
	SLONG write_cnt = write_platf(output_fl_desc, header_str, header_rec_len);
	switch (write_cnt)
	{
	case -1:					// write failed
		close_platf(output_fl_desc);
		return FB_FAILURE;

	default:
		end = pos + strlen(file_name);
		for (indx = pos; indx < end; indx++)
			header_str[indx] = BLANK;
		return FB_SUCCESS;
	}
}


static int flush_io_buff(const UCHAR* remaining_io,
						 SLONG		remaining_io_len,
						 FILE_DESC	output_fl_desc,
						 SINT64		file_size,
						 SLONG*		byte_write,
						 bool*		flush_done)
{
/********************************************************************
**
**	f l u s h _ i o _ b u f f
**
*********************************************************************
**
** Functional description:
**
**	flush out the remaining data in the I/O buffer
**
**	when the file_size is truly has space to write out the remaining data
**		then
**			set and returns flush_done true
**		otherwise
**			closes the output file, set and returns flush_done false.
**	otherwise
**		we can only writes out as much data as file_size indicated
**		if it was able to write out the remaining data
**			then
**				set and returns flush_done true
**			otherwise
**				closes the output file, set and returns flush_done false.
**
*********************************************************************
*/

	SLONG write_cnt;

	if (file_size > remaining_io_len)
		write_cnt = write_platf(output_fl_desc, remaining_io, remaining_io_len);
	else	// file_size <= remaining_io_len
		write_cnt = write_platf(output_fl_desc, remaining_io, (unsigned int) file_size);

	switch (write_cnt)
	{
	case -1:					// write failed
		close_platf(output_fl_desc);
		*flush_done = false;
		return FB_FAILURE;

	default:
		if (write_cnt == remaining_io_len)	// write ok
			*flush_done = true;
		else
		{
			// could not write out all remaining data
			close_platf(output_fl_desc);
			*flush_done = false;
		}
		*byte_write = write_cnt;
		return FB_SUCCESS;
	}
}


static int final_flush_io_buff(const UCHAR* remaining_io,
							   SLONG remaining_io_len,
							   FILE_DESC output_fl_desc)
{
/********************************************************************
**
**	f i n a l _ f l u s h _ i o _ b u f f
**
*********************************************************************
**
** Functional description:
**
**	flush out the remaining data in the I/O buffer
**
*********************************************************************
*/

	SLONG write_cnt = write_platf(output_fl_desc, remaining_io, remaining_io_len);
	switch (write_cnt)
	{
	case -1:					// write failed
		close_platf(output_fl_desc);
		return FB_FAILURE;

	default:
		if (write_cnt == remaining_io_len)	// write ok
			return FB_SUCCESS;

		close_platf(output_fl_desc);
		return FB_FAILURE;
	}
}


static void print_clo(const TEXT* prog_name)
{
/********************************************************************
**
**	p r i n t _ c l o
**
*********************************************************************
**
**	Functional description:
**
**		print out gsplit utility command line options
**
*********************************************************************
*/

	fprintf(stderr, "%s: Command Line Options Are:\n", prog_name);
	fprintf(stderr,
			   "  gsplit -S[PLIT_BK_FILE] <file> <size>{k|m|g} [... <file> [<size>{k|m|g}]] or\n");
	fprintf(stderr, "  gsplit -J[OINT_BK_FILE] <file> [... <file>]\n");
	fprintf(stderr, "%s: option can be abbreviated to the unparenthesized characters\n", prog_name);
	fprintf(stderr, "%s: Exiting before completion due to errors\n", prog_name);

	//return FB_SUCCESS;

}


static int set_hdr_str(TEXT header_str[], const TEXT* in_str, SLONG pos, SLONG len)
{
/********************************************************************
**
**	s e t _ h d r _ s t r
**
*********************************************************************
**
**	Functional description:
**
**		initialyze header string
**
*********************************************************************
*/
	const TEXT* t_str = in_str;
	const SLONG end = pos + len;

	for (SLONG indx = pos; indx < end; indx++)
	{
		switch (*t_str)
		{
		case NEW_LINE:
		case TERMINAL:
			header_str[indx] = ' ';
			break;
		default:
			header_str[indx] = *t_str;
			break;
		}
		++t_str;
	}
	return FB_SUCCESS;
}


static int free_file_list( b_fil* file_list)
{
/********************************************************************
**
**	f r e e _ f i l e _ l i s t
**
*********************************************************************
**
**	Functional description:
**
**		free all the storage allocated for backup files structure
**
*********************************************************************
*/

	b_fil* next_file = NULL;

	for (b_fil* file_ptr = file_list; file_ptr != NULL; file_ptr = next_file)
	{
		next_file = file_ptr->b_fil_next;
		free(file_ptr->b_fil_name);
		free(file_ptr);
	}

	return FB_SUCCESS;
}
