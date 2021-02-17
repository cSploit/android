/* callback.c -- functions to use readline as an X `callback' mechanism. */

/* Copyright (C) 1987-2009 Free Software Foundation, Inc.

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

#include "rlconf.h"

#if defined (READLINE_CALLBACKS)

#include <sys/types.h>

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif

#include <stdio.h>

/* System-specific feature definitions and include files. */
#include "rldefs.h"
#include "readline.h"
#include "rlprivate.h"
#include "xmalloc.h"

/* Private data for callback registration functions.  See comments in
   rl_callback_read_char for more details. */
_rl_callback_func_t *_rl_callback_func = 0;
_rl_callback_generic_arg *_rl_callback_data = 0;

/* **************************************************************** */
/*								    */
/*			Callback Readline Functions		 */
/*								    */
/* **************************************************************** */

/* Allow using readline in situations where a program may have multiple
   things to handle at once, and dispatches them via select().  Call
   rl_callback_handler_install() with the prompt and a function to call
   whenever a complete line of input is ready.  The user must then
   call rl_callback_read_char() every time some input is available, and 
   rl_callback_read_char() will call the user's function with the complete
   text read in at each end of line.  The terminal is kept prepped
   all the time, except during calls to the user's function.  Signal
   handlers are only installed when the application calls back into
   readline, so readline doesn't `steal' signals from the application.  */

rl_vcpfunc_t *rl_linefunc;		/* user callback function */
static int in_handler;		/* terminal_prepped and signals set? */

/* Make sure the terminal is set up, initialize readline, and prompt. */
static void
_rl_callback_newline ()
{
  rl_initialize ();

  if (in_handler == 0)
    {
      in_handler = 1;

      if (rl_prep_term_function)
	(*rl_prep_term_function) (_rl_meta_flag);
    }

  readline_internal_setup ();
  RL_CHECK_SIGNALS ();
}

/* Install a readline handler, set up the terminal, and issue the prompt. */
void
rl_callback_handler_install (prompt, linefunc)
     const char *prompt;
     rl_vcpfunc_t *linefunc;
{
  rl_set_prompt (prompt);
  RL_SETSTATE (RL_STATE_CALLBACK);
  rl_linefunc = linefunc;
  _rl_callback_newline ();
}

#if defined (HANDLE_SIGNALS)
#define CALLBACK_READ_RETURN() \
  do { \
    rl_clear_signals (); \
    return; \
  } while (0)
#else
#define CALLBACK_READ_RETURN() return
#endif

/* Read one character, and dispatch to the handler if it ends the line. */
void
rl_callback_read_char ()
{
  char *line;
  int eof, jcode;
  static procenv_t olevel;

  if (rl_linefunc == NULL)
    {
      _rl_errmsg ("readline_callback_read_char() called with no handler!");
      abort ();
    }

  memcpy ((void *)olevel, (void *)_rl_top_level, sizeof (procenv_t));
#if defined (HAVE_POSIX_SIGSETJMP)
  jcode = sigsetjmp (_rl_top_level, 0);
#else
  jcode = setjmp (_rl_top_level);
#endif
  if (jcode)
    {
      (*rl_redisplay_function) ();
      _rl_want_redisplay = 0;
      memcpy ((void *)_rl_top_level, (void *)olevel, sizeof (procenv_t));
      CALLBACK_READ_RETURN ();
    }

#if defined (HANDLE_SIGNALS)
  /* Install signal handlers only when readline has control. */
  rl_set_signals ();
#endif

  do
    {
      RL_CHECK_SIGNALS ();
      if  (RL_ISSTATE (RL_STATE_ISEARCH))
	{
	  eof = _rl_isearch_callback (_rl_iscxt);
	  if (eof == 0 && (RL_ISSTATE (RL_STATE_ISEARCH) == 0) && RL_ISSTATE (RL_STATE_INPUTPENDING))
	    rl_callback_read_char ();

	  CALLBACK_READ_RETURN ();
	}
      else if  (RL_ISSTATE (RL_STATE_NSEARCH))
	{
	  eof = _rl_nsearch_callback (_rl_nscxt);

	  CALLBACK_READ_RETURN ();
	}
#if defined (VI_MODE)
      else if (RL_ISSTATE (RL_STATE_VIMOTION))
	{
	  eof = _rl_vi_domove_callback (_rl_vimvcxt);
	  /* Should handle everything, including cleanup, numeric arguments,
	     and turning off RL_STATE_VIMOTION */
	  if (RL_ISSTATE (RL_STATE_NUMERICARG) == 0)
	    _rl_internal_char_cleanup ();

	  CALLBACK_READ_RETURN ();
	}
#endif
      else if (RL_ISSTATE (RL_STATE_NUMERICARG))
	{
	  eof = _rl_arg_callback (_rl_argcxt);
	  if (eof == 0 && (RL_ISSTATE (RL_STATE_NUMERICARG) == 0) && RL_ISSTATE (RL_STATE_INPUTPENDING))
	    rl_callback_read_char ();
	  /* XXX - this should handle _rl_last_command_was_kill better */
	  else if (RL_ISSTATE (RL_STATE_NUMERICARG) == 0)
	    _rl_internal_char_cleanup ();

	  CALLBACK_READ_RETURN ();
	}
      else if (RL_ISSTATE (RL_STATE_MULTIKEY))
	{
	  eof = _rl_dispatch_callback (_rl_kscxt);	/* For now */
	  while ((eof == -1 || eof == -2) && RL_ISSTATE (RL_STATE_MULTIKEY) && _rl_kscxt && (_rl_kscxt->flags & KSEQ_DISPATCHED))
	    eof = _rl_dispatch_callback (_rl_kscxt);
	  if (RL_ISSTATE (RL_STATE_MULTIKEY) == 0)
	    {
	      _rl_internal_char_cleanup ();
	      _rl_want_redisplay = 1;
	    }
	}
      else if (_rl_callback_func)
	{
	  /* This allows functions that simply need to read an additional
	     character (like quoted-insert) to register a function to be
	     called when input is available.  _rl_callback_data is a
	     pointer to a struct that has the argument count originally
	     passed to the registering function and space for any additional
	     parameters.  */
	  eof = (*_rl_callback_func) (_rl_callback_data);
	  /* If the function `deregisters' itself, make sure the data is
	     cleaned up. */
	  if (_rl_callback_func == 0)
	    {
	      if (_rl_callback_data) 	
		{
		  _rl_callback_data_dispose (_rl_callback_data);
		  _rl_callback_data = 0;
		}
	      _rl_internal_char_cleanup ();
	    }
	}
      else
	eof = readline_internal_char ();

      RL_CHECK_SIGNALS ();
      if (rl_done == 0 && _rl_want_redisplay)
	{
	  (*rl_redisplay_function) ();
	  _rl_want_redisplay = 0;
	}

      if (rl_done)
	{
	  line = readline_internal_teardown (eof);

	  if (rl_deprep_term_function)
	    (*rl_deprep_term_function) ();
#if defined (HANDLE_SIGNALS)
	  rl_clear_signals ();
#endif
	  in_handler = 0;
	  (*rl_linefunc) (line);

	  /* If the user did not clear out the line, do it for him. */
	  if (rl_line_buffer[0])
	    _rl_init_line_state ();

	  /* Redisplay the prompt if readline_handler_{install,remove}
	     not called. */
	  if (in_handler == 0 && rl_linefunc)
	    _rl_callback_newline ();
	}
    }
  while (rl_pending_input || _rl_pushed_input_available () || RL_ISSTATE (RL_STATE_MACROINPUT));

  CALLBACK_READ_RETURN ();
}

/* Remove the handler, and make sure the terminal is in its normal state. */
void
rl_callback_handler_remove ()
{
  rl_linefunc = NULL;
  RL_UNSETSTATE (RL_STATE_CALLBACK);
  RL_CHECK_SIGNALS ();
  if (in_handler)
    {
      in_handler = 0;
      if (rl_deprep_term_function)
	(*rl_deprep_term_function) ();
#if defined (HANDLE_SIGNALS)
      rl_clear_signals ();
#endif
    }
}

_rl_callback_generic_arg *
_rl_callback_data_alloc (count)
     int count;
{
  _rl_callback_generic_arg *arg;

  arg = (_rl_callback_generic_arg *)xmalloc (sizeof (_rl_callback_generic_arg));
  arg->count = count;

  arg->i1 = arg->i2 = 0;

  return arg;
}

void _rl_callback_data_dispose (arg)
     _rl_callback_generic_arg *arg;
{
  xfree (arg);
}

#endif
