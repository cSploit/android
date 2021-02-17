/* A front-end using readline to "cook" input lines.
 *
 * Copyright (C) 2004, 1999  Per Bothner
 * 
 * This front-end program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Some code from Johnson & Troan: "Linux Application Development"
 * (Addison-Wesley, 1998) was used directly or for inspiration.
 *
 * 2003-11-07 Wolfgang Taeuber <wolfgang_taeuber@agilent.com>
 * Specify a history file and the size of the history file with command
 * line options; use EDITOR/VISUAL to set vi/emacs preference.
 */

/* PROBLEMS/TODO:
 *
 * Only tested under GNU/Linux and Mac OS 10.x;  needs to be ported.
 *
 * Switching between line-editing-mode vs raw-char-mode depending on
 * what tcgetattr returns is inherently not robust, plus it doesn't
 * work when ssh/telnetting in.  A better solution is possible if the
 * tty system can send in-line escape sequences indicating the current
 * mode, echo'd input, etc.  That would also allow a user preference
 * to set different colors for prompt, input, stdout, and stderr.
 *
 * When running mc -c under the Linux console, mc does not recognize
 * mouse clicks, which mc does when not running under rlfe.
 *
 * Pasting selected text containing tabs is like hitting the tab character,
 * which invokes readline completion.  We don't want this.  I don't know
 * if this is fixable without integrating rlfe into a terminal emulator.
 *
 * Echo suppression is a kludge, but can only be avoided with better kernel
 * support: We need a tty mode to disable "real" echoing, while still
 * letting the inferior think its tty driver to doing echoing.
 * Stevens's book claims SCR$ and BSD4.3+ have TIOCREMOTE.
 *
 * The latest readline may have some hooks we can use to avoid having
 * to back up the prompt. (See HAVE_ALREADY_PROMPTED.)
 *
 * Desirable readline feature:  When in cooked no-echo mode (e.g. password),
 * echo characters are they are types with '*', but remove them when done.
 *
 * Asynchronous output while we're editing an input line should be
 * inserted in the output view *before* the input line, so that the
 * lines being edited (with the prompt) float at the end of the input.
 *
 * A "page mode" option to emulate more/less behavior:  At each page of
 * output, pause for a user command.  This required parsing the output
 * to keep track of line lengths.  It also requires remembering the
 * output, if we want an option to scroll back, which suggests that
 * this should be integrated with a terminal emulator like xterm.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "config.h"
#include "extern.h"

#if defined (HAVE_SYS_WAIT_H)
#  include <sys/wait.h>
#endif

#ifdef READLINE_LIBRARY
#  include "readline.h"
#  include "history.h"
#else
#  include <readline/readline.h>
#  include <readline/history.h>
#endif

#ifndef COMMAND
#define COMMAND "/bin/bash"
#endif
#ifndef COMMAND_ARGS
#define COMMAND_ARGS COMMAND
#endif

#ifndef ALT_COMMAND
#define ALT_COMMAND "/bin/sh"
#endif
#ifndef ALT_COMMAND_ARGS
#define ALT_COMMAND_ARGS ALT_COMMAND
#endif

#ifndef HAVE_MEMMOVE
#  if __GNUC__ > 1
#    define memmove(d, s, n)	__builtin_memcpy(d, s, n)
#  else
#    define memmove(d, s, n)	memcpy(d, s, n)
#  endif
#else
#  define memmove(d, s, n)	memcpy(d, s, n)
#endif

#define APPLICATION_NAME "rlfe"

static int in_from_inferior_fd;
static int out_to_inferior_fd;
static void set_edit_mode ();
static void usage_exit ();
static char *hist_file = 0;
static int  hist_size = 0;

/* Unfortunately, we cannot safely display echo from the inferior process.
   The reason is that the echo bit in the pty is "owned" by the inferior,
   and if we try to turn it off, we could confuse the inferior.
   Thus, when echoing, we get echo twice:  First readline echoes while
   we're actually editing. Then we send the line to the inferior, and the
   terminal driver send back an extra echo.
   The work-around is to remember the input lines, and when we see that
   line come back, we supress the output.
   A better solution (supposedly available on SVR4) would be a smarter
   terminal driver, with more flags ... */
#define ECHO_SUPPRESS_MAX 1024
char echo_suppress_buffer[ECHO_SUPPRESS_MAX];
int echo_suppress_start = 0;
int echo_suppress_limit = 0;

/*#define DEBUG*/

#ifdef DEBUG
FILE *logfile = NULL;
#define DPRINT0(FMT) (fprintf(logfile, FMT), fflush(logfile))
#define DPRINT1(FMT, V1) (fprintf(logfile, FMT, V1), fflush(logfile))
#define DPRINT2(FMT, V1, V2) (fprintf(logfile, FMT, V1, V2), fflush(logfile))
#else
#define DPRINT0(FMT) ((void) 0) /* Do nothing */
#define DPRINT1(FMT, V1) ((void) 0) /* Do nothing */
#define DPRINT2(FMT, V1, V2) ((void) 0) /* Do nothing */
#endif

struct termios orig_term;

/* Pid of child process. */
static pid_t child = -1;

static void
sig_child (int signo)
{
  int status;
  wait (&status);
  if (hist_file != 0)
    {
      write_history (hist_file);
      if (hist_size)
	history_truncate_file (hist_file, hist_size);
    }
  DPRINT0 ("(Child process died.)\n");
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
  exit (0);
}

volatile int propagate_sigwinch = 0;

/* sigwinch_handler
 * propagate window size changes from input file descriptor to
 * master side of pty.
 */
void sigwinch_handler(int signal) { 
   propagate_sigwinch = 1;
}


/* get_slave_pty() returns an integer file descriptor.
 * If it returns < 0, an error has occurred.
 * Otherwise, it has returned the slave file descriptor.
 */

int get_slave_pty(char *name) { 
   struct group *gptr;
   gid_t gid;
   int slave = -1;

   /* chown/chmod the corresponding pty, if possible.
    * This will only work if the process has root permissions.
    * Alternatively, write and exec a small setuid program that
    * does just this.
    */
   if ((gptr = getgrnam("tty")) != 0) {
      gid = gptr->gr_gid;
   } else {
      /* if the tty group does not exist, don't change the
       * group on the slave pty, only the owner
       */
      gid = -1;
   }

   /* Note that we do not check for errors here.  If this is code
    * where these actions are critical, check for errors!
    */
   chown(name, getuid(), gid);
   /* This code only makes the slave read/writeable for the user.
    * If this is for an interactive shell that will want to
    * receive "write" and "wall" messages, OR S_IWGRP into the
    * second argument below.
    */
   chmod(name, S_IRUSR|S_IWUSR);

   /* open the corresponding slave pty */
   slave = open(name, O_RDWR);
   return (slave);
}

/* Certain special characters, such as ctrl/C, we want to pass directly
   to the inferior, rather than letting readline handle them. */

static char special_chars[20];
static int special_chars_count;

static void
add_special_char(int ch)
{
  if (ch != 0)
    special_chars[special_chars_count++] = ch;
}

static int eof_char;

static int
is_special_char(int ch)
{
  int i;
#if 0
  if (ch == eof_char && rl_point == rl_end)
    return 1;
#endif
  for (i = special_chars_count;  --i >= 0; )
    if (special_chars[i] == ch)
      return 1;
  return 0;
}

static char buf[1024];
/* buf[0 .. buf_count-1] is the what has been emitted on the current line.
   It is used as the readline prompt. */
static int buf_count = 0;

int do_emphasize_input = 1;
int current_emphasize_input;

char *start_input_mode = "\033[1m";
char *end_input_mode = "\033[0m";

int num_keys = 0;

static void maybe_emphasize_input (int on)
{
  if (on == current_emphasize_input
      || (on && ! do_emphasize_input))
    return;
  fprintf (rl_outstream, on ? start_input_mode : end_input_mode);
  fflush (rl_outstream);
  current_emphasize_input = on;
}

static void
null_prep_terminal (int meta)
{
}

static void
null_deprep_terminal ()
{
  maybe_emphasize_input (0);
}

static int
pre_input_change_mode (void)
{
  return 0;
}

char pending_special_char;

static void
line_handler (char *line)
{
  if (line == NULL)
    {
      char buf[1];
      DPRINT0("saw eof!\n");
      buf[0] = '\004'; /* ctrl/d */
      write (out_to_inferior_fd, buf, 1);
    }
  else
    {
      static char enter[] = "\r";
      /*  Send line to inferior: */
      int length = strlen (line);
      if (length > ECHO_SUPPRESS_MAX-2)
	{
	  echo_suppress_start = 0;
	  echo_suppress_limit = 0;
	}
      else
	{
	  if (echo_suppress_limit + length > ECHO_SUPPRESS_MAX - 2)
	    {
	      if (echo_suppress_limit - echo_suppress_start + length
		  <= ECHO_SUPPRESS_MAX - 2)
		{
		  memmove (echo_suppress_buffer,
			   echo_suppress_buffer + echo_suppress_start,
			   echo_suppress_limit - echo_suppress_start);
		  echo_suppress_limit -= echo_suppress_start;
		  echo_suppress_start = 0;
		}
	      else
		{
		  echo_suppress_limit = 0;
		}
	      echo_suppress_start = 0;
	    }
	  memcpy (echo_suppress_buffer + echo_suppress_limit,
		  line, length);
	  echo_suppress_limit += length;
	  echo_suppress_buffer[echo_suppress_limit++] = '\r';
	  echo_suppress_buffer[echo_suppress_limit++] = '\n';
	}
      write (out_to_inferior_fd, line, length);
      if (pending_special_char == 0)
        {
          write (out_to_inferior_fd, enter, sizeof(enter)-1);
          if (*line)
            add_history (line);
        }
      free (line);
    }
  rl_callback_handler_remove ();
  buf_count = 0;
  num_keys = 0;
  if (pending_special_char != 0)
    {
      write (out_to_inferior_fd, &pending_special_char, 1);
      pending_special_char = 0;
    }
}

/* Value of rl_getc_function.
   Use this because readline should read from stdin, not rl_instream,
   points to the pty (so readline has monitor its terminal modes). */

int
my_rl_getc (FILE *dummy)
{
  int ch = rl_getc (stdin);
  if (is_special_char (ch))
    {
      pending_special_char = ch;
      return '\r';
    }
  return ch;
}

int
main(int argc, char** argv)
{
  char *path;
  int i;
  int master;
  char *name;
  int in_from_tty_fd;
  struct sigaction act;
  struct winsize ws;
  struct termios t;
  int maxfd;
  fd_set in_set;
  static char empty_string[1] = "";
  char *prompt = empty_string;
  int ioctl_err = 0;
  int arg_base = 1;

#ifdef DEBUG
  logfile = fopen("/tmp/rlfe.log", "w");
#endif

  while (arg_base<argc)
    {
      if (argv[arg_base][0] != '-')
	break;
      if (arg_base+1 >= argc )
	usage_exit();
      switch(argv[arg_base][1])
	{
	case 'h':
	  arg_base++;
	  hist_file = argv[arg_base];
	  break;
	case 's':
	  arg_base++;
	  hist_size = atoi(argv[arg_base]);
	  if (hist_size<0)
	    usage_exit();
	  break;
	default:
	  usage_exit();
	}
      arg_base++;
    }
  if (hist_file)
    read_history (hist_file);

  set_edit_mode ();

  rl_readline_name = APPLICATION_NAME;
  
  if ((master = OpenPTY (&name)) < 0)
    {
      perror("ptypair: could not open master pty");
      exit(1);
    }

  DPRINT1("pty name: '%s'\n", name);

  /* set up SIGWINCH handler */
  act.sa_handler = sigwinch_handler;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = 0;
  if (sigaction(SIGWINCH, &act, NULL) < 0)
    {
      perror("ptypair: could not handle SIGWINCH ");
      exit(1);
    }

  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
    {
      perror("ptypair: could not get window size");
      exit(1);
    }

  if ((child = fork()) < 0)
    {
      perror("cannot fork");
      exit(1);
    }

  if (child == 0)
    { 
      int slave;  /* file descriptor for slave pty */

      /* We are in the child process */
      close(master);

#ifdef TIOCSCTTY
      if ((slave = get_slave_pty(name)) < 0)
	{
	  perror("ptypair: could not open slave pty");
	  exit(1);
	}
#endif

      /* We need to make this process a session group leader, because
       * it is on a new PTY, and things like job control simply will
       * not work correctly unless there is a session group leader
       * and process group leader (which a session group leader
       * automatically is). This also disassociates us from our old
       * controlling tty. 
       */
      if (setsid() < 0)
	{
	  perror("could not set session leader");
	}

      /* Tie us to our new controlling tty. */
#ifdef TIOCSCTTY
      if (ioctl(slave, TIOCSCTTY, NULL))
	{
	  perror("could not set new controlling tty");
	}
#else
      if ((slave = get_slave_pty(name)) < 0)
	{
	  perror("ptypair: could not open slave pty");
	  exit(1);
	}
#endif

      /* make slave pty be standard in, out, and error */
      dup2(slave, STDIN_FILENO);
      dup2(slave, STDOUT_FILENO);
      dup2(slave, STDERR_FILENO);

      /* at this point the slave pty should be standard input */
      if (slave > 2)
	{
	  close(slave);
	}

      /* Try to restore window size; failure isn't critical */
      if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws) < 0)
	{
	  perror("could not restore window size");
	}

      /* now start the shell */
      {
	static char* command_args[] = { COMMAND_ARGS, NULL };
	static char* alt_command_args[] = { ALT_COMMAND_ARGS, NULL };
	if (argc <= 1)
	  {
	    execvp (COMMAND, command_args);
	    execvp (ALT_COMMAND, alt_command_args);
	  }
	else
	  execvp (argv[arg_base], &argv[arg_base]);
      }

      /* should never be reached */
      exit(1);
    }

  /* parent */
  signal (SIGCHLD, sig_child);

  /* Note that we only set termios settings for standard input;
   * the master side of a pty is NOT a tty.
   */
  tcgetattr(STDIN_FILENO, &orig_term);

  t = orig_term;
  eof_char = t.c_cc[VEOF];
  /*  add_special_char(t.c_cc[VEOF]);*/
  add_special_char(t.c_cc[VINTR]);
  add_special_char(t.c_cc[VQUIT]);
  add_special_char(t.c_cc[VSUSP]);
#if defined (VDISCARD)
  add_special_char(t.c_cc[VDISCARD]);
#endif

  t.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOCTL | ECHOE | \
		 ECHOK | ECHONL
#if defined (ECHOKE)
		| ECHOKE
#endif
#if defined (ECHOPRT)
		| ECHOPRT
#endif
		);
  t.c_iflag &= ~ICRNL;
  t.c_iflag |= IGNBRK;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
  in_from_inferior_fd = master;
  out_to_inferior_fd = master;
  rl_instream = fdopen (master, "r");
  rl_getc_function = my_rl_getc;

  rl_prep_term_function = null_prep_terminal; 
  rl_deprep_term_function = null_deprep_terminal;
  rl_pre_input_hook = pre_input_change_mode;
  rl_callback_handler_install (prompt, line_handler);

  in_from_tty_fd = STDIN_FILENO;
  FD_ZERO (&in_set);
  maxfd = in_from_inferior_fd > in_from_tty_fd ? in_from_inferior_fd
    : in_from_tty_fd;
  for (;;)
    {
      int num;
      FD_SET (in_from_inferior_fd, &in_set);
      FD_SET (in_from_tty_fd, &in_set);

      num = select(maxfd+1, &in_set, NULL, NULL, NULL);

      if (propagate_sigwinch)
	{
	  struct winsize ws;
	  if (ioctl (STDIN_FILENO, TIOCGWINSZ, &ws) >= 0)
	    {
	      ioctl (master, TIOCSWINSZ, &ws);
	    }
	  propagate_sigwinch = 0;
	  continue;
	}

      if (num <= 0)
	{
	  perror ("select");
	  exit (-1);
	}
      if (FD_ISSET (in_from_tty_fd, &in_set))
	{
	  extern int _rl_echoing_p;
	  struct termios term_master;
	  int do_canon = 1;
	  int do_icrnl = 1;
	  int ioctl_ret;

	  DPRINT1("[tty avail num_keys:%d]\n", num_keys);

	  /* If we can't get tty modes for the master side of the pty, we
	     can't handle non-canonical-mode programs.  Always assume the
	     master is in canonical echo mode if we can't tell. */
	  ioctl_ret = tcgetattr(master, &term_master);

	  if (ioctl_ret >= 0)
	    {
	      do_canon = (term_master.c_lflag & ICANON) != 0;
	      do_icrnl = (term_master.c_lflag & ICRNL) != 0;
	      _rl_echoing_p = (term_master.c_lflag & ECHO) != 0;
	      DPRINT1 ("echo,canon,crnl:%03d\n",
		       100 * _rl_echoing_p
		       + 10 * do_canon
		       + 1 * do_icrnl);
	    }
	  else
	    {
	      if (ioctl_err == 0)
		DPRINT1("tcgetattr on master fd failed: errno = %d\n", errno);
	      ioctl_err = 1;
	    }

	  if (do_canon == 0 && num_keys == 0)
	    {
	      char ch[10];
	      int count = read (STDIN_FILENO, ch, sizeof(ch));
	      DPRINT1("[read %d chars from stdin: ", count);
	      DPRINT2(" \"%.*s\"]\n", count, ch);
	      if (do_icrnl)
		{
		  int i = count;
		  while (--i >= 0)
		    {
		      if (ch[i] == '\r')
			ch[i] = '\n';
		    }
		}
	      maybe_emphasize_input (1);
	      write (out_to_inferior_fd, ch, count);
	    }
	  else
	    {
	      if (num_keys == 0)
		{
		  int i;
		  /* Re-install callback handler for new prompt. */
		  if (prompt != empty_string)
		    free (prompt);
		  if (prompt == NULL)
		    {
		      DPRINT0("New empty prompt\n");
		      prompt = empty_string;
		    }
		  else
		    {
		      if (do_emphasize_input && buf_count > 0)
			{
			  prompt = malloc (buf_count + strlen (end_input_mode)
					   + strlen (start_input_mode) + 5);
			  sprintf (prompt, "\001%s\002%.*s\001%s\002",
				   end_input_mode,
				   buf_count, buf,
				   start_input_mode);
			}
		      else
			{
			  prompt = malloc (buf_count + 1);
			  memcpy (prompt, buf, buf_count);
			  prompt[buf_count] = '\0';
			}
		      DPRINT1("New prompt '%s'\n", prompt);
#if 0 /* ifdef HAVE_RL_ALREADY_PROMPTED */
		      /* Doesn't quite work when do_emphasize_input is 1. */
		      rl_already_prompted = buf_count > 0;
#else
		      if (buf_count > 0)
			write (1, "\r", 1);
#endif
		    }

		  rl_callback_handler_install (prompt, line_handler);
		}
	      num_keys++;
	      maybe_emphasize_input (1);
	      rl_callback_read_char ();
	    }
	}
      else /* output from inferior. */
	{
	  int i;
	  int count;
	  int old_count;
	  if (buf_count > (sizeof(buf) >> 2))
	    buf_count = 0;
	  count = read (in_from_inferior_fd, buf+buf_count,
			sizeof(buf) - buf_count);
          DPRINT2("read %d from inferior, buf_count=%d", count, buf_count);
	  DPRINT2(": \"%.*s\"", count, buf+buf_count);
	  maybe_emphasize_input (0);
	  if (count <= 0)
	    {
	      DPRINT0 ("(Connection closed by foreign host.)\n");
	      tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
	      exit (0);
	    }
	  old_count = buf_count;

          /* Look for any pending echo that we need to suppress. */
	  while (echo_suppress_start < echo_suppress_limit
		 && count > 0
		 && buf[buf_count] == echo_suppress_buffer[echo_suppress_start])
	    {
	      count--;
	      buf_count++;
	      echo_suppress_start++;
	    }
	  DPRINT1("suppressed %d characters of echo.\n", buf_count-old_count);

          /* Write to the terminal anything that was not suppressed. */
          if (count > 0)
            write (1, buf + buf_count, count);

          /* Finally, look for a prompt candidate.
           * When we get around to going input (from the keyboard),
           * we will consider the prompt to be anything since the last
           * line terminator.  So we need to save that text in the
           * initial part of buf.  However, anything before the
           * most recent end-of-line is not interesting. */
	  buf_count += count;
#if 1
	  for (i = buf_count;  --i >= old_count; )
#else
	  for (i = buf_count - 1;  i-- >= buf_count - count; )
#endif
	    {
	      if (buf[i] == '\n' || buf[i] == '\r')
		{
		  i++;
		  memmove (buf, buf+i, buf_count - i);
		  buf_count -= i;
		  break;
		}
	    }
	  DPRINT2("-> i: %d, buf_count: %d\n", i, buf_count);
	}
    }
}

static void set_edit_mode ()
{
  int vi = 0;
  char *shellopts;

  shellopts = getenv ("SHELLOPTS");
  while (shellopts != 0)
    {
      if (strncmp ("vi", shellopts, 2) == 0)
	{
	  vi = 1;
	  break;
	}
      shellopts = strchr (shellopts + 1, ':');
    }

  if (!vi)
    {
      if (getenv ("EDITOR") != 0)
	vi |= strcmp (getenv ("EDITOR"), "vi") == 0;
    }

  if (vi)
    rl_variable_bind ("editing-mode", "vi");
  else
    rl_variable_bind ("editing-mode", "emacs");
}


static void usage_exit ()
{
  fprintf (stderr, "Usage: rlfe [-h histfile] [-s size] cmd [arg1] [arg2] ...\n\n");
  exit (1);
}
