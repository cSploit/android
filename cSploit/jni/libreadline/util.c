/* util.c -- readline utility functions */

/* Copyright (C) 1987-2012 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.      

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#define READLINE_LIBRARY

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#include "posixjmp.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>           /* for _POSIX_VERSION */
#endif /* HAVE_UNISTD_H */

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#include <stdio.h>
#include <ctype.h>

/* System-specific feature definitions and include files. */
#include "rldefs.h"
#include "rlmbutil.h"

#if defined (TIOCSTAT_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
#endif /* TIOCSTAT_IN_SYS_IOCTL */

/* Some standard library routines. */
#include "readline.h"

#include "rlprivate.h"
#include "xmalloc.h"

/* **************************************************************** */
/*								    */
/*			Utility Functions			    */
/*								    */
/* **************************************************************** */

/* Return 0 if C is not a member of the class of characters that belong
   in words, or 1 if it is. */

int _rl_allow_pathname_alphabetic_chars = 0;
static const char * const pathname_alphabetic_chars = "/-_=~.#$";

int
rl_alphabetic (c)
     int c;
{
  if (ALPHABETIC (c))
    return (1);

  return (_rl_allow_pathname_alphabetic_chars &&
	    strchr (pathname_alphabetic_chars, c) != NULL);
}

#if defined (HANDLE_MULTIBYTE)
int
_rl_walphabetic (wchar_t wc)
{
  int c;

  if (iswalnum (wc))
    return (1);     

  c = wc & 0177;
  return (_rl_allow_pathname_alphabetic_chars &&
	    strchr (pathname_alphabetic_chars, c) != NULL);
}
#endif

/* How to abort things. */
int
_rl_abort_internal ()
{
  rl_ding ();
  rl_clear_message ();
  _rl_reset_argument ();
  rl_clear_pending_input ();

  RL_UNSETSTATE (RL_STATE_MACRODEF);
  while (rl_executing_macro)
    _rl_pop_executing_macro ();

  rl_last_func = (rl_command_func_t *)NULL;
#if defined (HAVE_POSIX_SIGSETJMP)
  siglongjmp (_rl_top_level, 1);
#else
  longjmp (_rl_top_level, 1);
#endif
  return (0);
}

int
rl_abort (count, key)
     int count, key;
{
  return (_rl_abort_internal ());
}

int
_rl_null_function (count, key)
     int count, key;
{
  return 0;
}

int
rl_tty_status (count, key)
     int count, key;
{
#if defined (TIOCSTAT)
  ioctl (1, TIOCSTAT, (char *)0);
  rl_refresh_line (count, key);
#else
  rl_ding ();
#endif
  return 0;
}

/* Return a copy of the string between FROM and TO.
   FROM is inclusive, TO is not. */
char *
rl_copy_text (from, to)
     int from, to;
{
  register int length;
  char *copy;

  /* Fix it if the caller is confused. */
  if (from > to)
    SWAP (from, to);

  length = to - from;
  copy = (char *)xmalloc (1 + length);
  strncpy (copy, rl_line_buffer + from, length);
  copy[length] = '\0';
  return (copy);
}

/* Increase the size of RL_LINE_BUFFER until it has enough space to hold
   LEN characters. */
void
rl_extend_line_buffer (len)
     int len;
{
  while (len >= rl_line_buffer_len)
    {
      rl_line_buffer_len += DEFAULT_BUFFER_SIZE;
      rl_line_buffer = (char *)xrealloc (rl_line_buffer, rl_line_buffer_len);
    }

  _rl_set_the_line ();
}


/* A function for simple tilde expansion. */
int
rl_tilde_expand (ignore, key)
     int ignore, key;
{
  register int start, end;
  char *homedir, *temp;
  int len;

  end = rl_point;
  start = end - 1;

  if (rl_point == rl_end && rl_line_buffer[rl_point] == '~')
    {
      homedir = tilde_expand ("~");
      _rl_replace_text (homedir, start, end);
      xfree (homedir);
      return (0);
    }
  else if (rl_line_buffer[start] != '~')
    {
      for (; !whitespace (rl_line_buffer[start]) && start >= 0; start--)
        ;
      start++;
    }

  end = start;
  do
    end++;
  while (whitespace (rl_line_buffer[end]) == 0 && end < rl_end);

  if (whitespace (rl_line_buffer[end]) || end >= rl_end)
    end--;

  /* If the first character of the current word is a tilde, perform
     tilde expansion and insert the result.  If not a tilde, do
     nothing. */
  if (rl_line_buffer[start] == '~')
    {
      len = end - start + 1;
      temp = (char *)xmalloc (len + 1);
      strncpy (temp, rl_line_buffer + start, len);
      temp[len] = '\0';
      homedir = tilde_expand (temp);
      xfree (temp);

      _rl_replace_text (homedir, start, end);
      xfree (homedir);
    }

  return (0);
}

#if defined (USE_VARARGS)
void
#if defined (PREFER_STDARG)
_rl_ttymsg (const char *format, ...)
#else
_rl_ttymsg (va_alist)
     va_dcl
#endif
{
  va_list args;
#if defined (PREFER_VARARGS)
  char *format;
#endif

#if defined (PREFER_STDARG)
  va_start (args, format);
#else
  va_start (args);
  format = va_arg (args, char *);
#endif

  fprintf (stderr, "readline: ");
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");
  fflush (stderr);

  va_end (args);

  rl_forced_update_display ();
}

void
#if defined (PREFER_STDARG)
_rl_errmsg (const char *format, ...)
#else
_rl_errmsg (va_alist)
     va_dcl
#endif
{
  va_list args;
#if defined (PREFER_VARARGS)
  char *format;
#endif

#if defined (PREFER_STDARG)
  va_start (args, format);
#else
  va_start (args);
  format = va_arg (args, char *);
#endif

  fprintf (stderr, "readline: ");
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");
  fflush (stderr);

  va_end (args);
}

#else /* !USE_VARARGS */
void
_rl_ttymsg (format, arg1, arg2)
     char *format;
{
  fprintf (stderr, "readline: ");
  fprintf (stderr, format, arg1, arg2);
  fprintf (stderr, "\n");

  rl_forced_update_display ();
}

void
_rl_errmsg (format, arg1, arg2)
     char *format;
{
  fprintf (stderr, "readline: ");
  fprintf (stderr, format, arg1, arg2);
  fprintf (stderr, "\n");
}
#endif /* !USE_VARARGS */

/* **************************************************************** */
/*								    */
/*			String Utility Functions		    */
/*								    */
/* **************************************************************** */

/* Determine if s2 occurs in s1.  If so, return a pointer to the
   match in s1.  The compare is case insensitive. */
char *
_rl_strindex (s1, s2)
     register const char *s1, *s2;
{
  register int i, l, len;

  for (i = 0, l = strlen (s2), len = strlen (s1); (len - i) >= l; i++)
    if (_rl_strnicmp (s1 + i, s2, l) == 0)
      return ((char *) (s1 + i));
  return ((char *)NULL);
}

#ifndef HAVE_STRPBRK
/* Find the first occurrence in STRING1 of any character from STRING2.
   Return a pointer to the character in STRING1. */
char *
_rl_strpbrk (string1, string2)
     const char *string1, *string2;
{
  register const char *scan;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  register int i, v;

  memset (&ps, 0, sizeof (mbstate_t));
#endif

  for (; *string1; string1++)
    {
      for (scan = string2; *scan; scan++)
	{
	  if (*string1 == *scan)
	    return ((char *)string1);
	}
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
	{
	  v = _rl_get_char_len (string1, &ps);
	  if (v > 1)
	    string1 += v - 1;	/* -1 to account for auto-increment in loop */
	}
#endif
    }
  return ((char *)NULL);
}
#endif

#if !defined (HAVE_STRCASECMP)
/* Compare at most COUNT characters from string1 to string2.  Case
   doesn't matter (strncasecmp). */
int
_rl_strnicmp (string1, string2, count)
     const char *string1;
     const char *string2;
     int count;
{
  register const char *s1;
  register const char *s2;
  register int d;

  if (count <= 0 || (string1 == string2))
    return 0;

  s1 = string1;
  s2 = string2;
  do
    {
      d = _rl_to_lower (*s1) - _rl_to_lower (*s2);	/* XXX - cast to unsigned char? */
      if (d != 0)
	return d;
      if (*s1++ == '\0')
        break;
      s2++;
    }
  while (--count != 0);

  return (0);
}

/* strcmp (), but caseless (strcasecmp). */
int
_rl_stricmp (string1, string2)
     const char *string1;
     const char *string2;
{
  register const char *s1;
  register const char *s2;
  register int d;

  s1 = string1;
  s2 = string2;

  if (s1 == s2)
    return 0;

  while ((d = _rl_to_lower (*s1) - _rl_to_lower (*s2)) == 0)
    {
      if (*s1++ == '\0')
        return 0;
      s2++;
    }

  return (d);
}
#endif /* !HAVE_STRCASECMP */

/* Stupid comparison routine for qsort () ing strings. */
int
_rl_qsort_string_compare (s1, s2)
  char **s1, **s2;
{
#if defined (HAVE_STRCOLL)
  return (strcoll (*s1, *s2));
#else
  int result;

  result = **s1 - **s2;
  if (result == 0)
    result = strcmp (*s1, *s2);

  return result;
#endif
}

/* Function equivalents for the macros defined in chardefs.h. */
#define FUNCTION_FOR_MACRO(f)	int (f) (c) int c; { return f (c); }

FUNCTION_FOR_MACRO (_rl_digit_p)
FUNCTION_FOR_MACRO (_rl_digit_value)
FUNCTION_FOR_MACRO (_rl_lowercase_p)
FUNCTION_FOR_MACRO (_rl_pure_alphabetic)
FUNCTION_FOR_MACRO (_rl_to_lower)
FUNCTION_FOR_MACRO (_rl_to_upper)
FUNCTION_FOR_MACRO (_rl_uppercase_p)

/* A convenience function, to force memory deallocation to be performed
   by readline.  DLLs on Windows apparently require this. */
void
rl_free (mem)
     void *mem;
{
  if (mem)
    free (mem);
}

/* Backwards compatibility, now that savestring has been removed from
   all `public' readline header files. */
#undef _rl_savestring
char *
_rl_savestring (s)
     const char *s;
{
  return (strcpy ((char *)xmalloc (1 + (int)strlen (s)), (s)));
}

#if defined (DEBUG)
#if defined (USE_VARARGS)
static FILE *_rl_tracefp;

void
#if defined (PREFER_STDARG)
_rl_trace (const char *format, ...)
#else
_rl_trace (va_alist)
     va_dcl
#endif
{
  va_list args;
#if defined (PREFER_VARARGS)
  char *format;
#endif

#if defined (PREFER_STDARG)
  va_start (args, format);
#else
  va_start (args);
  format = va_arg (args, char *);
#endif

  if (_rl_tracefp == 0)
    _rl_tropen ();
  vfprintf (_rl_tracefp, format, args);
  fprintf (_rl_tracefp, "\n");
  fflush (_rl_tracefp);

  va_end (args);
}

int
_rl_tropen ()
{
  char fnbuf[128];

  if (_rl_tracefp)
    fclose (_rl_tracefp);
  sprintf (fnbuf, "/var/tmp/rltrace.%ld", (long)getpid());
  unlink(fnbuf);
  _rl_tracefp = fopen (fnbuf, "w+");
  return _rl_tracefp != 0;
}

int
_rl_trclose ()
{
  int r;

  r = fclose (_rl_tracefp);
  _rl_tracefp = 0;
  return r;
}

void
_rl_settracefp (fp)
     FILE *fp;
{
  _rl_tracefp = fp;
}
#endif
#endif /* DEBUG */


#if HAVE_DECL_AUDIT_USER_TTY && defined (ENABLE_TTY_AUDIT_SUPPORT)
#include <sys/socket.h>
#include <linux/audit.h>
#include <linux/netlink.h>

/* Report STRING to the audit system. */
void
_rl_audit_tty (string)
     char *string;
{
  struct sockaddr_nl addr;
  struct msghdr msg;
  struct nlmsghdr nlm;
  struct iovec iov[2];
  size_t size;
  int fd;

  fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_AUDIT);
  if (fd < 0)
    return;
  size = strlen (string) + 1;

  nlm.nlmsg_len = NLMSG_LENGTH (size);
  nlm.nlmsg_type = AUDIT_USER_TTY;
  nlm.nlmsg_flags = NLM_F_REQUEST;
  nlm.nlmsg_seq = 0;
  nlm.nlmsg_pid = 0;

  iov[0].iov_base = &nlm;
  iov[0].iov_len = sizeof (nlm);
  iov[1].iov_base = string;
  iov[1].iov_len = size;

  addr.nl_family = AF_NETLINK;
  addr.nl_pid = 0;
  addr.nl_groups = 0;

  msg.msg_name = &addr;
  msg.msg_namelen = sizeof (addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 2;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  (void)sendmsg (fd, &msg, 0);
  close (fd);
}
#endif
