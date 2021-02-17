/* Convert text in given files from the specified from-set to the to-set.
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <argp.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <iconv.h>
#include <locale.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _POSIX_MAPPED_FILES
# include <sys/mman.h>
#endif
//#include <gconv_int.h>

/* Get libc version number.  */
#include "version.h"

#define PACKAGE _libc_intl_domainname


/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

#define OPT_VERBOSE	1000
#define OPT_LIST	1001

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, N_("Input/Output format specification:") },
  { "from-code", 'f', "NAME", 0, N_("encoding of original text") },
  { "to-code", 't', "NAME", 0, N_("encoding for output") },
  { NULL, 0, NULL, 0, N_("Information:") },
  { "list", OPT_LIST, NULL, 0, N_("list all known coded character sets") },
  { NULL, 0, NULL, 0, N_("Output control:") },
  { "output", 'o', "FILE", 0, N_("output file") },
  { "verbose", OPT_VERBOSE, NULL, 0, N_("print progress information") },
  { NULL, 0, NULL, 0, NULL }
};

/* Short description of program.  */
static const char doc[] = N_("\
Convert encoding of given files from one encoding to another.");

/* Strings for arguments in help texts.  */
static const char args_doc[] = N_("[FILE...]");

/* Prototype for option handler.  */
static error_t parse_opt __P ((int key, char *arg, struct argp_state *state));

/* Function to print some extra text in the help message.  */
static char *more_help __P ((int key, const char *text, void *input));

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, args_doc, doc, NULL, more_help
};

/* Code sets to convert from and to respectively.  */
static const char *from_code;
static const char *to_code;

/* File to write output to.  If NULL write to stdout.  */
static const char *output_file;

/* Nonzero if verbose ouput is wanted.  */
static int verbose;

/* Nonzero if list of all coded character sets is wanted.  */
static int list;

/* Prototypes for the functions doing the actual work.  */
static int process_block (iconv_t cd, const char *addr, size_t len,
			  FILE *output);
static int process_fd (iconv_t cd, int fd, FILE *output);
static int process_file (iconv_t cd, FILE *input, FILE *output);
static void print_known_names (void) internal_function;


int
main (int argc, char *argv[])
{
  int status = EXIT_SUCCESS;
  int remaining;
  FILE *output;
  iconv_t cd;

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  textdomain (_libc_intl_domainname);

  /* Parse and process arguments.  */
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  /* List all coded character sets if wanted.  */
  if (list)
    {
      print_known_names ();
      exit (EXIT_SUCCESS);
    }

  /* If either the from- or to-code is not specified this is an error
     since we do not know what to do.  */
  if (from_code == NULL && to_code == NULL)
    error (EXIT_FAILURE, 0,
	   _("neither original nor target encoding specified"));
  if (from_code == NULL)
    error (EXIT_FAILURE, 0, _("original encoding not specified using `-f'"));
  if (to_code == NULL)
    error (EXIT_FAILURE, 0, _("target encoding not specified using `-t'"));

  /* Let's see whether we have these coded character sets.  */
  cd = iconv_open (to_code, from_code);
  if (cd == (iconv_t) -1)
    {
      if (errno == EINVAL)
	error (EXIT_FAILURE, 0, _("conversion from `%s' to `%s' not supported"),
	       from_code, to_code);
      else
	error (EXIT_FAILURE, errno, _("failed to start conversion processing"));
    }

  /* Determine output file.  */
  if (output_file != NULL)
    {
      output = fopen (output_file, "w");
      if (output == NULL)
	error (EXIT_FAILURE, errno, _("cannot open output file"));
    }
  else
    output = stdout;

  /* Now process the remaining files.  Write them to stdout or the file
     specified with the `-o' parameter.  If we have no file given as
     the parameter process all from stdin.  */
  if (remaining == argc)
    process_file (cd, stdin, output);
  else
    do
      {
	struct stat st;
	const char *addr;
	int fd = open (argv[remaining], O_RDONLY);

	if (verbose)
	  printf ("%s:\n", argv[remaining]);

	if (fd == -1)
	  {
	    error (0, errno, _("cannot open input file `%s'"),
		   argv[remaining]);
	    status = EXIT_FAILURE;
	    continue;
	  }

#ifdef _POSIX_MAPPED_FILES
	/* We have possibilities for reading the input file.  First try
	   to mmap() it since this will provide the fastest solution.  */
	if (fstat (fd, &st) == 0
	    && ((addr = mmap (NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0))
		!= MAP_FAILED))
	  {
	    /* Yes, we can use mmap().  The descriptor is not needed
               anymore.  */
	    if (close (fd) != 0)
	      error (EXIT_FAILURE, errno, _("error while closing input `%s'"),
		     argv[remaining]);

	    if (process_block (cd, addr, st.st_size, output) < 0)
	      {
		/* Something went wrong.  */
		status = EXIT_FAILURE;

		/* We don't need the input data anymore.  */
		munmap ((void *) addr, st.st_size);

		/* We cannot go on with producing output since it might
		   lead to problem because the last output might leave
		   the output stream in an undefined state.  */
		break;
	      }

	    /* We don't need the input data anymore.  */
	    munmap ((void *) addr, st.st_size);
	  }
	else
#endif	/* _POSIX_MAPPED_FILES */
	  {
	    /* Read the file in pieces.  */
	    if (process_fd (cd, fd, output) != 0)
	      {
		/* Something went wrong.  */
		status = EXIT_FAILURE;

		/* We don't need the input file anymore.  */
		close (fd);

		/* We cannot go on with producing output since it might
		   lead to problem because the last output might leave
		   the output stream in an undefined state.  */
		break;
	      }

	    /* Now close the file.  */
	    close (fd);
	  }
      }
    while (++remaining < argc);

  /* Close the output file now.  */
  if (fclose (output))
    error (EXIT_FAILURE, errno, _("error while closing output file"));

  return status;
}


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      from_code = arg;
      break;
    case 't':
      to_code = arg;
      break;
    case 'o':
      output_file = arg;
      break;
    case OPT_VERBOSE:
      verbose = 1;
      break;
    case OPT_LIST:
      list = 1;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


static char *
more_help (int key, const char *text, void *input)
{
  switch (key)
    {
    case ARGP_KEY_HELP_EXTRA:
      /* We print some extra information.  */
      return strdup (gettext ("\
Report bugs using the `glibcbug' script to <bugs@gnu.org>.\n"));
    default:
      break;
    }
  return (char *) text;
}


/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "iconv (GNU %s) %s\n", PACKAGE, VERSION);
  fprintf (stream, gettext ("\
Copyright (C) %s Free Software Foundation, Inc.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
"), "1999");
  fprintf (stream, gettext ("Written by %s.\n"), "Ulrich Drepper");
}


static int
process_block (iconv_t cd, const char *addr, size_t len, FILE *output)
{
#define OUTBUF_SIZE	32768
  const char *start = addr;
  char outbuf[OUTBUF_SIZE];
  char *outptr;
  size_t outlen;
  size_t n;

  while (len > 0)
    {
      outptr = outbuf;
      outlen = OUTBUF_SIZE;
      n = iconv (cd, &addr, &len, &outptr, &outlen);

      if (outptr != outbuf)
	{
	  /* We have something to write out.  */
	  if (fwrite (outbuf, 1, outptr - outbuf, output) < outptr - outbuf
	      || ferror (output))
	    {
	      /* Error occurred while printing the result.  */
	      error (0, 0, _("\
conversion stopped due to problem in writing the output"));
	      return -1;
	    }
	}

      if (n != (size_t) -1)
	/* Everything is processed.  */
	break;

      if (errno != E2BIG)
	{
	  /* iconv() ran into a problem.  */
	  switch (errno)
	    {
	    case EILSEQ:
	      error (0, 0, _("illegal input sequence at position %ld"),
		     addr - start);
	      addr++;
	      len--;
	      goto cont;
	      break;
	    case EINVAL:
	      error (0, 0, _("\
incomplete character or shift sequence at end of buffer"));
	      break;
	    case EBADF:
	      error (0, 0, _("internal error (illegal descriptor)"));
	      break;
	    default:
	      error (0, 0, _("unknown iconv() error %d"), errno);
	      break;
	    }

	  return -1;
	}
cont:;	
    }

  return 0;
}


static int
process_fd (iconv_t cd, int fd, FILE *output)
{
  /* we have a problem with reading from a desriptor since we must not
     provide the iconv() function an incomplete character or shift
     sequence at the end of the buffer.  Since we have to deal with
     arbitrary encodings we must read the whole text in a buffer and
     process it in one step.  */
  static char *inbuf = NULL;
  static size_t maxlen = 0;
  char *inptr = NULL;
  size_t actlen = 0;

  while (actlen < maxlen)
    {
      size_t n = read (fd, inptr, maxlen - actlen);

      if (n == 0)
	/* No more text to read.  */
	break;

      if (n == -1)
	{
	  /* Error while reading.  */
	  error (0, errno, _("error while reading the input"));
	  return -1;
	}

      inptr += n;
      actlen += n;
    }

  if (actlen == maxlen)
    while (1)
      {
	size_t n;

	/* Increase the buffer.  */
	maxlen += 32768;
	inbuf = realloc (inbuf, maxlen);
	if (inbuf == NULL)
	  error (0, errno, _("unable to allocate buffer for input"));
	inptr = inbuf + actlen;

	do
	  {
	    n = read (fd, inptr, maxlen - actlen);

	    if (n == 0)
	      /* No more text to read.  */
	      break;

	    if (n == -1)
	      {
		/* Error while reading.  */
		error (0, errno, _("error while reading the input"));
		return -1;
	      }

	    inptr += n;
	    actlen += n;
	  }
	while (actlen < maxlen);

	if (n == 0)
	  /* Break again so we leave both loops.  */
	  break;
      }

  /* Now we have all the input in the buffer.  Process it in one run.  */
  return process_block (cd, inbuf, actlen, output);
}


static int
process_file (iconv_t cd, FILE *input, FILE *output)
{
  /* This should be safe since we use this function only for `stdin' and
     we haven't read anything so far.  */
  return process_fd (cd, fileno (input), output);
}


/* Print all known character sets/encodings.  */
static void *printlist;
static size_t column;
static int not_first;

static void
insert_print_list (const void *nodep, VISIT value, int level)
{
  if (value == leaf || value == postorder)
    {
      const struct gconv_alias *s = *(const struct gconv_alias **) nodep;
      tsearch (s->fromname, &printlist, (__compar_fn_t) strverscmp);
    }
}

static void
do_print  (const void *nodep, VISIT value, int level)
{
  if (value == leaf || value == postorder)
    {
      const char *s = *(const char **) nodep;
      size_t len = strlen (s);
      size_t cnt;

      while (len > 0 && s[len - 1] == '/')
	--len;

      for (cnt = 0; cnt < len; ++cnt)
	if (isalnum (s[cnt]))
	  break;
      if (cnt == len)
	return;

      if (not_first)
	{
	  putchar (',');
	  ++column;

	  if (column > 2 && column + len > 77)
	    {
	      fputs ("\n  ", stdout);
	      column = 2;
	    }
	  else
	    {
	      putchar (' ');
	      ++column;
	    }
	}
      else
	not_first = 1;

      fwrite (s, len, 1, stdout);
      column += len;
    }
}

static void
internal_function
add_known_names (struct gconv_module *node)
{
  if (node->left != NULL)
    add_known_names (node->left);
  if (node->right != NULL)
    add_known_names (node->right);
  if (node->same != NULL)
    add_known_names (node->same);
  do
    {
      if (node->from_pattern == NULL)
	{
	  if (strcmp (node->from_constpfx, "INTERNAL"))
	    tsearch (node->from_constpfx, &printlist,
		     (__compar_fn_t) strverscmp);
	  if (strcmp (node->to_string, "INTERNAL"))
	    tsearch (node->to_string, &printlist, (__compar_fn_t) strverscmp);
	}
      else
	if (strcmp (node->from_pattern, "INTERNAL"))
	  tsearch (node->from_pattern, &printlist, (__compar_fn_t) strverscmp);

      node = node->matching;
    }
  while (node != NULL);
}

static void
internal_function
print_known_names (void)
{
  iconv_t h;

  /* We must initialize the internal databases first.  */
  h = iconv_open ("L1", "L1");
  iconv_close (h);

  /* First add the aliases.  */
  twalk (__gconv_alias_db, insert_print_list);

  /* Add the from- and to-names from the known modules.  */
  add_known_names (__gconv_modules_db);

  fputs (_("\
The following list contain all the coded character sets known.  This does\n\
not necessarily mean that all combinations of these names can be used for\n\
the FROM and TO command line parameters.  One coded character set can be\n\
listed with several different names (aliases).\n\
  Some of the names are no plain strings but instead regular expressions and\n\
they match a variety of names which can be given as parameters to the\n\
program.\n\n  "), stdout);

  /* Now print the collected names.  */
  column = 2;
  twalk (printlist, do_print);

  if (column != 0)
    puts ("");
}
