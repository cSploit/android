/* vi_mode.c -- A vi emulation mode for Bash.
   Derived from code written by Jeff Sparkes (jsparkes@bnr.ca).  */

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

/* **************************************************************** */
/*								    */
/*			VI Emulation Mode			    */
/*								    */
/* **************************************************************** */
#include "rlconf.h"

#if defined (VI_MODE)

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <sys/types.h>

#if defined (HAVE_STDLIB_H)
#  include <stdlib.h>
#else
#  include "ansi_stdlib.h"
#endif /* HAVE_STDLIB_H */

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <stdio.h>

/* Some standard library routines. */
#include "rldefs.h"
#include "rlmbutil.h"

#include "readline.h"
#include "history.h"

#include "rlprivate.h"
#include "xmalloc.h"

#ifndef member
#define member(c, s) ((c) ? (char *)strchr ((s), (c)) != (char *)NULL : 0)
#endif

int _rl_vi_last_command = 'i';	/* default `.' puts you in insert mode */

_rl_vimotion_cxt *_rl_vimvcxt = 0;

/* Non-zero means enter insertion mode. */
static int _rl_vi_doing_insert;

/* Command keys which do movement for xxx_to commands. */
static const char * const vi_motion = " hl^$0ftFT;,%wbeWBE|`";

/* Keymap used for vi replace characters.  Created dynamically since
   rarely used. */
static Keymap vi_replace_map;

/* The number of characters inserted in the last replace operation. */
static int vi_replace_count;

/* If non-zero, we have text inserted after a c[motion] command that put
   us implicitly into insert mode.  Some people want this text to be
   attached to the command so that it is `redoable' with `.'. */
static int vi_continued_command;
static char *vi_insert_buffer;
static int vi_insert_buffer_size;

static int _rl_vi_last_repeat = 1;
static int _rl_vi_last_arg_sign = 1;
static int _rl_vi_last_motion;
#if defined (HANDLE_MULTIBYTE)
static char _rl_vi_last_search_mbchar[MB_LEN_MAX];
static int _rl_vi_last_search_mblen;
#else
static int _rl_vi_last_search_char;
#endif
static int _rl_vi_last_replacement;

static int _rl_vi_last_key_before_insert;

static int vi_redoing;

/* Text modification commands.  These are the `redoable' commands. */
static const char * const vi_textmod = "_*\\AaIiCcDdPpYyRrSsXx~";

/* Arrays for the saved marks. */
static int vi_mark_chars['z' - 'a' + 1];

static void _rl_vi_replace_insert PARAMS((int));
static void _rl_vi_save_replace PARAMS((void));
static void _rl_vi_stuff_insert PARAMS((int));
static void _rl_vi_save_insert PARAMS((UNDO_LIST *));

static void vi_save_insert_buffer PARAMS ((int, int));

static void _rl_vi_backup PARAMS((void));

static int _rl_vi_arg_dispatch PARAMS((int));
static int rl_digit_loop1 PARAMS((void));

static int _rl_vi_set_mark PARAMS((void));
static int _rl_vi_goto_mark PARAMS((void));

static void _rl_vi_append_forward PARAMS((int));

static int _rl_vi_callback_getchar PARAMS((char *, int));

#if defined (READLINE_CALLBACKS)
static int _rl_vi_callback_set_mark PARAMS((_rl_callback_generic_arg *));
static int _rl_vi_callback_goto_mark PARAMS((_rl_callback_generic_arg *));
static int _rl_vi_callback_change_char PARAMS((_rl_callback_generic_arg *));
static int _rl_vi_callback_char_search PARAMS((_rl_callback_generic_arg *));
#endif

static int rl_domove_read_callback PARAMS((_rl_vimotion_cxt *));
static int rl_domove_motion_callback PARAMS((_rl_vimotion_cxt *));
static int rl_vi_domove_getchar PARAMS((_rl_vimotion_cxt *));

static int vi_change_dispatch PARAMS((_rl_vimotion_cxt *));
static int vi_delete_dispatch PARAMS((_rl_vimotion_cxt *));
static int vi_yank_dispatch PARAMS((_rl_vimotion_cxt *));

static int vidomove_dispatch PARAMS((_rl_vimotion_cxt *));

void
_rl_vi_initialize_line ()
{
  register int i, n;

  n = sizeof (vi_mark_chars) / sizeof (vi_mark_chars[0]);
  for (i = 0; i < n; i++)
    vi_mark_chars[i] = -1;

  RL_UNSETSTATE(RL_STATE_VICMDONCE);
}

void
_rl_vi_reset_last ()
{
  _rl_vi_last_command = 'i';
  _rl_vi_last_repeat = 1;
  _rl_vi_last_arg_sign = 1;
  _rl_vi_last_motion = 0;
}

void
_rl_vi_set_last (key, repeat, sign)
     int key, repeat, sign;
{
  _rl_vi_last_command = key;
  _rl_vi_last_repeat = repeat;
  _rl_vi_last_arg_sign = sign;
}

/* A convenience function that calls _rl_vi_set_last to save the last command
   information and enters insertion mode. */
void
rl_vi_start_inserting (key, repeat, sign)
     int key, repeat, sign;
{
  _rl_vi_set_last (key, repeat, sign);
  rl_vi_insertion_mode (1, key);
}

/* Is the command C a VI mode text modification command? */
int
_rl_vi_textmod_command (c)
     int c;
{
  return (member (c, vi_textmod));
}

static void
_rl_vi_replace_insert (count)
     int count;
{
  int nchars;

  nchars = strlen (vi_insert_buffer);

  rl_begin_undo_group ();
  while (count--)
    /* nchars-1 to compensate for _rl_replace_text using `end+1' in call
       to rl_delete_text */
    _rl_replace_text (vi_insert_buffer, rl_point, rl_point+nchars-1);
  rl_end_undo_group ();
}

static void
_rl_vi_stuff_insert (count)
     int count;
{
  rl_begin_undo_group ();
  while (count--)
    rl_insert_text (vi_insert_buffer);
  rl_end_undo_group ();
}

/* Bound to `.'.  Called from command mode, so we know that we have to
   redo a text modification command.  The default for _rl_vi_last_command
   puts you back into insert mode. */
int
rl_vi_redo (count, c)
     int count, c;
{
  int r;

  if (rl_explicit_arg == 0)
    {
      rl_numeric_arg = _rl_vi_last_repeat;
      rl_arg_sign = _rl_vi_last_arg_sign;
    }

  r = 0;
  vi_redoing = 1;
  /* If we're redoing an insert with `i', stuff in the inserted text
     and do not go into insertion mode. */
  if (_rl_vi_last_command == 'i' && vi_insert_buffer && *vi_insert_buffer)
    {
      _rl_vi_stuff_insert (count);
      /* And back up point over the last character inserted. */
      if (rl_point > 0)
	_rl_vi_backup ();
    }
  else if (_rl_vi_last_command == 'R' && vi_insert_buffer && *vi_insert_buffer)
    {
      _rl_vi_replace_insert (count);
      /* And back up point over the last character inserted. */
      if (rl_point > 0)
	_rl_vi_backup ();
    }
  /* Ditto for redoing an insert with `I', but move to the beginning of the
     line like the `I' command does. */
  else if (_rl_vi_last_command == 'I' && vi_insert_buffer && *vi_insert_buffer)
    {
      rl_beg_of_line (1, 'I');
      _rl_vi_stuff_insert (count);
      if (rl_point > 0)
	_rl_vi_backup ();
    }
  /* Ditto for redoing an insert with `a', but move forward a character first
     like the `a' command does. */
  else if (_rl_vi_last_command == 'a' && vi_insert_buffer && *vi_insert_buffer)
    {
      _rl_vi_append_forward ('a');
      _rl_vi_stuff_insert (count);
      if (rl_point > 0)
	_rl_vi_backup ();
    }
  /* Ditto for redoing an insert with `A', but move to the end of the line
     like the `A' command does. */
  else if (_rl_vi_last_command == 'A' && vi_insert_buffer && *vi_insert_buffer)
    {
      rl_end_of_line (1, 'A');
      _rl_vi_stuff_insert (count);
      if (rl_point > 0)
	_rl_vi_backup ();
    }
  else
    r = _rl_dispatch (_rl_vi_last_command, _rl_keymap);
  vi_redoing = 0;

  return (r);
}

/* A placeholder for further expansion. */
int
rl_vi_undo (count, key)
     int count, key;
{
  return (rl_undo_command (count, key));
}
    
/* Yank the nth arg from the previous line into this line at point. */
int
rl_vi_yank_arg (count, key)
     int count, key;
{
  /* Readline thinks that the first word on a line is the 0th, while vi
     thinks the first word on a line is the 1st.  Compensate. */
  if (rl_explicit_arg)
    rl_yank_nth_arg (count - 1, 0);
  else
    rl_yank_nth_arg ('$', 0);

  return (0);
}

/* With an argument, move back that many history lines, else move to the
   beginning of history. */
int
rl_vi_fetch_history (count, c)
     int count, c;
{
  int wanted;

  /* Giving an argument of n means we want the nth command in the history
     file.  The command number is interpreted the same way that the bash
     `history' command does it -- that is, giving an argument count of 450
     to this command would get the command listed as number 450 in the
     output of `history'. */
  if (rl_explicit_arg)
    {
      wanted = history_base + where_history () - count;
      if (wanted <= 0)
        rl_beginning_of_history (0, 0);
      else
        rl_get_previous_history (wanted, c);
    }
  else
    rl_beginning_of_history (count, 0);
  return (0);
}

/* Search again for the last thing searched for. */
int
rl_vi_search_again (count, key)
     int count, key;
{
  switch (key)
    {
    case 'n':
      rl_noninc_reverse_search_again (count, key);
      break;

    case 'N':
      rl_noninc_forward_search_again (count, key);
      break;
    }
  return (0);
}

/* Do a vi style search. */
int
rl_vi_search (count, key)
     int count, key;
{
  switch (key)
    {
    case '?':
      _rl_free_saved_history_line ();
      rl_noninc_forward_search (count, key);
      break;

    case '/':
      _rl_free_saved_history_line ();
      rl_noninc_reverse_search (count, key);
      break;

    default:
      rl_ding ();
      break;
    }
  return (0);
}

/* Completion, from vi's point of view. */
int
rl_vi_complete (ignore, key)
     int ignore, key;
{
  if ((rl_point < rl_end) && (!whitespace (rl_line_buffer[rl_point])))
    {
      if (!whitespace (rl_line_buffer[rl_point + 1]))
	rl_vi_end_word (1, 'E');
      rl_point++;
    }

  if (key == '*')
    rl_complete_internal ('*');	/* Expansion and replacement. */
  else if (key == '=')
    rl_complete_internal ('?');	/* List possible completions. */
  else if (key == '\\')
    rl_complete_internal (TAB);	/* Standard Readline completion. */
  else
    rl_complete (0, key);

  if (key == '*' || key == '\\')
    rl_vi_start_inserting (key, 1, rl_arg_sign);

  return (0);
}

/* Tilde expansion for vi mode. */
int
rl_vi_tilde_expand (ignore, key)
     int ignore, key;
{
  rl_tilde_expand (0, key);
  rl_vi_start_inserting (key, 1, rl_arg_sign);
  return (0);
}

/* Previous word in vi mode. */
int
rl_vi_prev_word (count, key)
     int count, key;
{
  if (count < 0)
    return (rl_vi_next_word (-count, key));

  if (rl_point == 0)
    {
      rl_ding ();
      return (0);
    }

  if (_rl_uppercase_p (key))
    rl_vi_bWord (count, key);
  else
    rl_vi_bword (count, key);

  return (0);
}

/* Next word in vi mode. */
int
rl_vi_next_word (count, key)
     int count, key;
{
  if (count < 0)
    return (rl_vi_prev_word (-count, key));

  if (rl_point >= (rl_end - 1))
    {
      rl_ding ();
      return (0);
    }

  if (_rl_uppercase_p (key))
    rl_vi_fWord (count, key);
  else
    rl_vi_fword (count, key);
  return (0);
}

/* Move to the end of the ?next? word. */
int
rl_vi_end_word (count, key)
     int count, key;
{
  if (count < 0)
    {
      rl_ding ();
      return -1;
    }

  if (_rl_uppercase_p (key))
    rl_vi_eWord (count, key);
  else
    rl_vi_eword (count, key);
  return (0);
}

/* Move forward a word the way that 'W' does. */
int
rl_vi_fWord (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point < (rl_end - 1))
    {
      /* Skip until whitespace. */
      while (!whitespace (rl_line_buffer[rl_point]) && rl_point < rl_end)
	rl_point++;

      /* Now skip whitespace. */
      while (whitespace (rl_line_buffer[rl_point]) && rl_point < rl_end)
	rl_point++;
    }
  return (0);
}

int
rl_vi_bWord (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point > 0)
    {
      /* If we are at the start of a word, move back to whitespace so
	 we will go back to the start of the previous word. */
      if (!whitespace (rl_line_buffer[rl_point]) &&
	  whitespace (rl_line_buffer[rl_point - 1]))
	rl_point--;

      while (rl_point > 0 && whitespace (rl_line_buffer[rl_point]))
	rl_point--;

      if (rl_point > 0)
	{
	  while (--rl_point >= 0 && !whitespace (rl_line_buffer[rl_point]));
	  rl_point++;
	}
    }
  return (0);
}

int
rl_vi_eWord (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point < (rl_end - 1))
    {
      if (!whitespace (rl_line_buffer[rl_point]))
	rl_point++;

      /* Move to the next non-whitespace character (to the start of the
	 next word). */
      while (rl_point < rl_end && whitespace (rl_line_buffer[rl_point]))
	rl_point++;

      if (rl_point && rl_point < rl_end)
	{
	  /* Skip whitespace. */
	  while (rl_point < rl_end && whitespace (rl_line_buffer[rl_point]))
	    rl_point++;

	  /* Skip until whitespace. */
	  while (rl_point < rl_end && !whitespace (rl_line_buffer[rl_point]))
	    rl_point++;

	  /* Move back to the last character of the word. */
	  rl_point--;
	}
    }
  return (0);
}

int
rl_vi_fword (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point < (rl_end - 1))
    {
      /* Move to white space (really non-identifer). */
      if (_rl_isident (rl_line_buffer[rl_point]))
	{
	  while (_rl_isident (rl_line_buffer[rl_point]) && rl_point < rl_end)
	    rl_point++;
	}
      else /* if (!whitespace (rl_line_buffer[rl_point])) */
	{
	  while (!_rl_isident (rl_line_buffer[rl_point]) &&
		 !whitespace (rl_line_buffer[rl_point]) && rl_point < rl_end)
	    rl_point++;
	}

      /* Move past whitespace. */
      while (whitespace (rl_line_buffer[rl_point]) && rl_point < rl_end)
	rl_point++;
    }
  return (0);
}

int
rl_vi_bword (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point > 0)
    {
      int last_is_ident;

      /* If we are at the start of a word, move back to whitespace
	 so we will go back to the start of the previous word. */
      if (!whitespace (rl_line_buffer[rl_point]) &&
	  whitespace (rl_line_buffer[rl_point - 1]))
	rl_point--;

      /* If this character and the previous character are `opposite', move
	 back so we don't get messed up by the rl_point++ down there in
	 the while loop.  Without this code, words like `l;' screw up the
	 function. */
      last_is_ident = _rl_isident (rl_line_buffer[rl_point - 1]);
      if ((_rl_isident (rl_line_buffer[rl_point]) && !last_is_ident) ||
	  (!_rl_isident (rl_line_buffer[rl_point]) && last_is_ident))
	rl_point--;

      while (rl_point > 0 && whitespace (rl_line_buffer[rl_point]))
	rl_point--;

      if (rl_point > 0)
	{
	  if (_rl_isident (rl_line_buffer[rl_point]))
	    while (--rl_point >= 0 && _rl_isident (rl_line_buffer[rl_point]));
	  else
	    while (--rl_point >= 0 && !_rl_isident (rl_line_buffer[rl_point]) &&
		   !whitespace (rl_line_buffer[rl_point]));
	  rl_point++;
	}
    }
  return (0);
}

int
rl_vi_eword (count, ignore)
     int count, ignore;
{
  while (count-- && rl_point < rl_end - 1)
    {
      if (!whitespace (rl_line_buffer[rl_point]))
	rl_point++;

      while (rl_point < rl_end && whitespace (rl_line_buffer[rl_point]))
	rl_point++;

      if (rl_point < rl_end)
	{
	  if (_rl_isident (rl_line_buffer[rl_point]))
	    while (++rl_point < rl_end && _rl_isident (rl_line_buffer[rl_point]));
	  else
	    while (++rl_point < rl_end && !_rl_isident (rl_line_buffer[rl_point])
		   && !whitespace (rl_line_buffer[rl_point]));
	}
      rl_point--;
    }
  return (0);
}

int
rl_vi_insert_beg (count, key)
     int count, key;
{
  rl_beg_of_line (1, key);
  rl_vi_insert_mode (1, key);
  return (0);
}

static void
_rl_vi_append_forward (key)
     int key;
{
  int point;

  if (rl_point < rl_end)
    {
      if (MB_CUR_MAX == 1 || rl_byte_oriented)
	rl_point++;
      else
	{
	  point = rl_point;
#if 0
	  rl_forward_char (1, key);
#else
	  rl_point = _rl_forward_char_internal (1);
#endif
	  if (point == rl_point)
	    rl_point = rl_end;
	}
    }
}

int
rl_vi_append_mode (count, key)
     int count, key;
{
  _rl_vi_append_forward (key);
  rl_vi_start_inserting (key, 1, rl_arg_sign);
  return (0);
}

int
rl_vi_append_eol (count, key)
     int count, key;
{
  rl_end_of_line (1, key);
  rl_vi_append_mode (1, key);
  return (0);
}

/* What to do in the case of C-d. */
int
rl_vi_eof_maybe (count, c)
     int count, c;
{
  return (rl_newline (1, '\n'));
}

/* Insertion mode stuff. */

/* Switching from one mode to the other really just involves
   switching keymaps. */
int
rl_vi_insertion_mode (count, key)
     int count, key;
{
  _rl_keymap = vi_insertion_keymap;
  _rl_vi_last_key_before_insert = key;
  if (_rl_show_mode_in_prompt)
    _rl_reset_prompt ();
  return (0);
}

int
rl_vi_insert_mode (count, key)
     int count, key;
{
  rl_vi_start_inserting (key, 1, rl_arg_sign);
  return (0);
}

static void
vi_save_insert_buffer (start, len)
     int start, len;
{
  /* Same code as _rl_vi_save_insert below */
  if (len >= vi_insert_buffer_size)
    {
      vi_insert_buffer_size += (len + 32) - (len % 32);
      vi_insert_buffer = (char *)xrealloc (vi_insert_buffer, vi_insert_buffer_size);
    }
  strncpy (vi_insert_buffer, rl_line_buffer + start, len - 1);
  vi_insert_buffer[len-1] = '\0';
}

static void
_rl_vi_save_replace ()
{
  int len, start, end;
  UNDO_LIST *up;

  up = rl_undo_list;
  if (up == 0 || up->what != UNDO_END || vi_replace_count <= 0)
    {
      if (vi_insert_buffer_size >= 1)
	vi_insert_buffer[0] = '\0';
      return;
    }
  /* Let's try it the quick and easy way for now.  This should essentially
     accommodate every UNDO_INSERT and save the inserted text to
     vi_insert_buffer */
  end = rl_point;
  start = end - vi_replace_count + 1;
  len = vi_replace_count + 1;

  vi_save_insert_buffer (start, len);  
}

static void
_rl_vi_save_insert (up)
      UNDO_LIST *up;
{
  int len, start, end;

  if (up == 0 || up->what != UNDO_INSERT)
    {
      if (vi_insert_buffer_size >= 1)
	vi_insert_buffer[0] = '\0';
      return;
    }

  start = up->start;
  end = up->end;
  len = end - start + 1;

  vi_save_insert_buffer (start, len);
}
    
void
_rl_vi_done_inserting ()
{
  if (_rl_vi_doing_insert)
    {
      /* The `C', `s', and `S' commands set this. */
      rl_end_undo_group ();
      /* Now, the text between rl_undo_list->next->start and
	 rl_undo_list->next->end is what was inserted while in insert
	 mode.  It gets copied to VI_INSERT_BUFFER because it depends
	 on absolute indices into the line which may change (though they
	 probably will not). */
      _rl_vi_doing_insert = 0;
      if (_rl_vi_last_key_before_insert == 'R')
	_rl_vi_save_replace ();		/* Half the battle */
      else
	_rl_vi_save_insert (rl_undo_list->next);
      vi_continued_command = 1;
    }
  else
    {
      if (rl_undo_list && (_rl_vi_last_key_before_insert == 'i' ||
			   _rl_vi_last_key_before_insert == 'a' ||
			   _rl_vi_last_key_before_insert == 'I' ||
			   _rl_vi_last_key_before_insert == 'A'))
	_rl_vi_save_insert (rl_undo_list);
      /* XXX - Other keys probably need to be checked. */
      else if (_rl_vi_last_key_before_insert == 'C')
	rl_end_undo_group ();
      while (_rl_undo_group_level > 0)
	rl_end_undo_group ();
      vi_continued_command = 0;
    }
}

int
rl_vi_movement_mode (count, key)
     int count, key;
{
  if (rl_point > 0)
    rl_backward_char (1, key);

  _rl_keymap = vi_movement_keymap;
  _rl_vi_done_inserting ();

  /* This is how POSIX.2 says `U' should behave -- everything up until the
     first time you go into command mode should not be undone. */
  if (RL_ISSTATE (RL_STATE_VICMDONCE) == 0)
    rl_free_undo_list ();

  if (_rl_show_mode_in_prompt)
    _rl_reset_prompt ();

  RL_SETSTATE (RL_STATE_VICMDONCE);
  return (0);
}

int
rl_vi_arg_digit (count, c)
     int count, c;
{
  if (c == '0' && rl_numeric_arg == 1 && !rl_explicit_arg)
    return (rl_beg_of_line (1, c));
  else
    return (rl_digit_argument (count, c));
}

/* Change the case of the next COUNT characters. */
#if defined (HANDLE_MULTIBYTE)
static int
_rl_vi_change_mbchar_case (count)
     int count;
{
  wchar_t wc;
  char mb[MB_LEN_MAX+1];
  int mlen, p;
  size_t m;
  mbstate_t ps;

  memset (&ps, 0, sizeof (mbstate_t));
  if (_rl_adjust_point (rl_line_buffer, rl_point, &ps) > 0)
    count--;
  while (count-- && rl_point < rl_end)
    {
      m = mbrtowc (&wc, rl_line_buffer + rl_point, rl_end - rl_point, &ps);
      if (MB_INVALIDCH (m))
	wc = (wchar_t)rl_line_buffer[rl_point];
      else if (MB_NULLWCH (m))
	wc = L'\0';
      if (iswupper (wc))
	wc = towlower (wc);
      else if (iswlower (wc))
	wc = towupper (wc);
      else
	{
	  /* Just skip over chars neither upper nor lower case */
	  rl_forward_char (1, 0);
	  continue;
	}

      /* Vi is kind of strange here. */
      if (wc)
	{
	  p = rl_point;
	  mlen = wcrtomb (mb, wc, &ps);
	  if (mlen >= 0)
	    mb[mlen] = '\0';
	  rl_begin_undo_group ();
	  rl_vi_delete (1, 0);
	  if (rl_point < p)	/* Did we retreat at EOL? */
	    rl_point++;	/* XXX - should we advance more than 1 for mbchar? */
	  rl_insert_text (mb);
	  rl_end_undo_group ();
	  rl_vi_check ();
	}
      else
	rl_forward_char (1, 0);
    }

  return 0;
}
#endif

int
rl_vi_change_case (count, ignore)
     int count, ignore;
{
  int c, p;

  /* Don't try this on an empty line. */
  if (rl_point >= rl_end)
    return (0);

  c = 0;
#if defined (HANDLE_MULTIBYTE)
  if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    return (_rl_vi_change_mbchar_case (count));
#endif

  while (count-- && rl_point < rl_end)
    {
      if (_rl_uppercase_p (rl_line_buffer[rl_point]))
	c = _rl_to_lower (rl_line_buffer[rl_point]);
      else if (_rl_lowercase_p (rl_line_buffer[rl_point]))
	c = _rl_to_upper (rl_line_buffer[rl_point]);
      else
	{
	  /* Just skip over characters neither upper nor lower case. */
	  rl_forward_char (1, c);
	  continue;
	}

      /* Vi is kind of strange here. */
      if (c)
	{
	  p = rl_point;
	  rl_begin_undo_group ();
	  rl_vi_delete (1, c);
	  if (rl_point < p)	/* Did we retreat at EOL? */
	    rl_point++;
	  _rl_insert_char (1, c);
	  rl_end_undo_group ();
	  rl_vi_check ();
	}
      else
	rl_forward_char (1, c);
    }
  return (0);
}

int
rl_vi_put (count, key)
     int count, key;
{
  if (!_rl_uppercase_p (key) && (rl_point + 1 <= rl_end))
    rl_point = _rl_find_next_mbchar (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);

  while (count--)
    rl_yank (1, key);

  rl_backward_char (1, key);
  return (0);
}

static void
_rl_vi_backup ()
{
  if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    rl_point = _rl_find_prev_mbchar (rl_line_buffer, rl_point, MB_FIND_NONZERO);
  else
    rl_point--;
}

int
rl_vi_check ()
{
  if (rl_point && rl_point == rl_end)
    {
      if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
	rl_point = _rl_find_prev_mbchar (rl_line_buffer, rl_point, MB_FIND_NONZERO);
      else
	rl_point--;
    }
  return (0);
}

int
rl_vi_column (count, key)
     int count, key;
{
  if (count > rl_end)
    rl_end_of_line (1, key);
  else
    rl_point = count - 1;
  return (0);
}

/* Process C as part of the current numeric argument.  Return -1 if the
   argument should be aborted, 0 if we should not read any more chars, and
   1 if we should continue to read chars. */
static int
_rl_vi_arg_dispatch (c)
     int c;
{
  int key;

  key = c;
  if (c >= 0 && _rl_keymap[c].type == ISFUNC && _rl_keymap[c].function == rl_universal_argument)
    {
      rl_numeric_arg *= 4;
      return 1;
    }

  c = UNMETA (c);

  if (_rl_digit_p (c))
    {
      if (rl_explicit_arg)
	rl_numeric_arg = (rl_numeric_arg * 10) + _rl_digit_value (c);
      else
	rl_numeric_arg = _rl_digit_value (c);
      rl_explicit_arg = 1;
      return 1;		/* keep going */
    }
  else
    {
      rl_clear_message ();
      rl_stuff_char (key);
      return 0;		/* done */
    }
}

/* A simplified loop for vi. Don't dispatch key at end.
   Don't recognize minus sign?
   Should this do rl_save_prompt/rl_restore_prompt? */
static int
rl_digit_loop1 ()
{
  int c, r;

  while (1)
    {
      if (_rl_arg_overflow ())
	return 1;

      c = _rl_arg_getchar ();

      r = _rl_vi_arg_dispatch (c);
      if (r <= 0)
	break;
    }

  RL_UNSETSTATE(RL_STATE_NUMERICARG);
  return (0);
}

static void
_rl_mvcxt_init (m, op, key)
     _rl_vimotion_cxt *m;
     int op, key;
{
  m->op = op;
  m->state = m->flags = 0;
  m->ncxt = 0;
  m->numeric_arg = -1;
  m->start = rl_point;
  m->end = rl_end;
  m->key = key;
  m->motion = -1;
}

static _rl_vimotion_cxt *
_rl_mvcxt_alloc (op, key)
     int op, key;
{
  _rl_vimotion_cxt *m;

  m = xmalloc (sizeof (_rl_vimotion_cxt));
  _rl_mvcxt_init (m, op, key);
  return m;
}

static void
_rl_mvcxt_dispose (m)
     _rl_vimotion_cxt *m;
{
  xfree (m);
}

static int
rl_domove_motion_callback (m)
     _rl_vimotion_cxt *m;
{
  int c, save, r;
  int old_end;

  _rl_vi_last_motion = c = m->motion;

  /* Append a blank character temporarily so that the motion routines
     work right at the end of the line. */
  old_end = rl_end;
  rl_line_buffer[rl_end++] = ' ';
  rl_line_buffer[rl_end] = '\0';

  _rl_dispatch (c, _rl_keymap);

  /* Remove the blank that we added. */
  rl_end = old_end;
  rl_line_buffer[rl_end] = '\0';
  if (rl_point > rl_end)
    rl_point = rl_end;

  /* No change in position means the command failed. */
  if (rl_mark == rl_point)
    return (-1);

  /* rl_vi_f[wW]ord () leaves the cursor on the first character of the next
     word.  If we are not at the end of the line, and we are on a
     non-whitespace character, move back one (presumably to whitespace). */
  if ((_rl_to_upper (c) == 'W') && rl_point < rl_end && rl_point > rl_mark &&
      !whitespace (rl_line_buffer[rl_point]))
    rl_point--;

  /* If cw or cW, back up to the end of a word, so the behaviour of ce
     or cE is the actual result.  Brute-force, no subtlety. */
  if (m->key == 'c' && rl_point >= rl_mark && (_rl_to_upper (c) == 'W'))
    {
      /* Don't move farther back than where we started. */
      while (rl_point > rl_mark && whitespace (rl_line_buffer[rl_point]))
	rl_point--;

      /* Posix.2 says that if cw or cW moves the cursor towards the end of
	 the line, the character under the cursor should be deleted. */
      if (rl_point == rl_mark)
	rl_point++;
      else
	{
	  /* Move past the end of the word so that the kill doesn't
	     remove the last letter of the previous word.  Only do this
	     if we are not at the end of the line. */
	  if (rl_point >= 0 && rl_point < (rl_end - 1) && !whitespace (rl_line_buffer[rl_point]))
	    rl_point++;
	}
    }

  if (rl_mark < rl_point)
    SWAP (rl_point, rl_mark);

#if defined (READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    (*rl_redisplay_function)();		/* make sure motion is displayed */
#endif

  r = vidomove_dispatch (m);

  return (r);
}

#define RL_VIMOVENUMARG()	(RL_ISSTATE (RL_STATE_VIMOTION) && RL_ISSTATE (RL_STATE_NUMERICARG))

static int
rl_domove_read_callback (m)
     _rl_vimotion_cxt *m;
{
  int c, save;

  c = m->motion;

  if (member (c, vi_motion))
    {
#if defined (READLINE_CALLBACKS)
      /* If we just read a vi-mode motion command numeric argument, turn off
	 the `reading numeric arg' state */
      if (RL_ISSTATE (RL_STATE_CALLBACK) && RL_VIMOVENUMARG())
	RL_UNSETSTATE (RL_STATE_NUMERICARG);
#endif
      /* Should do everything, including turning off RL_STATE_VIMOTION */
      return (rl_domove_motion_callback (m));
    }
  else if (m->key == c && (m->key == 'd' || m->key == 'y' || m->key == 'c'))
    {
      rl_mark = rl_end;
      rl_beg_of_line (1, c);
      _rl_vi_last_motion = c;
      RL_UNSETSTATE (RL_STATE_VIMOTION);
      return (vidomove_dispatch (m));
    }
#if defined (READLINE_CALLBACKS)
  /* XXX - these need to handle rl_universal_argument bindings */
  /* Reading vi motion char continuing numeric argument */
  else if (_rl_digit_p (c) && RL_ISSTATE (RL_STATE_CALLBACK) && RL_VIMOVENUMARG())
    {
      return (_rl_vi_arg_dispatch (c));
    }
  /* Readine vi motion char starting numeric argument */
  else if (_rl_digit_p (c) && RL_ISSTATE (RL_STATE_CALLBACK) && RL_ISSTATE (RL_STATE_VIMOTION) && (RL_ISSTATE (RL_STATE_NUMERICARG) == 0))
    {
      RL_SETSTATE (RL_STATE_NUMERICARG);
      return (_rl_vi_arg_dispatch (c));
    }
#endif
  else if (_rl_digit_p (c))
    {
      /* This code path taken when not in callback mode */
      save = rl_numeric_arg;
      rl_numeric_arg = _rl_digit_value (c);
      rl_explicit_arg = 1;
      RL_SETSTATE (RL_STATE_NUMERICARG);
      rl_digit_loop1 ();
      rl_numeric_arg *= save;
      c = rl_vi_domove_getchar (m);
      if (c < 0)
	{
	  m->motion = 0;
	  return -1;
	}
      m->motion = c;
      return (rl_domove_motion_callback (m));
    }
  else
    {
      RL_UNSETSTATE (RL_STATE_VIMOTION);
      RL_UNSETSTATE (RL_STATE_NUMERICARG);
      return (1);
    }
}

static int
rl_vi_domove_getchar (m)
     _rl_vimotion_cxt *m;
{
  int c;

  RL_SETSTATE(RL_STATE_MOREINPUT);
  c = rl_read_key ();
  RL_UNSETSTATE(RL_STATE_MOREINPUT);

  return c;
}

#if defined (READLINE_CALLBACKS)
int
_rl_vi_domove_callback (m)
     _rl_vimotion_cxt *m;
{
  int c, r;

  m->motion = c = rl_vi_domove_getchar (m);
  /* XXX - what to do if this returns -1?  Should we return 1 for eof to
     callback code? */
  r = rl_domove_read_callback (m);

  return ((r == 0) ? r : 1);	/* normalize return values */
}
#endif

/* This code path taken when not in callback mode. */
int
rl_vi_domove (x, ignore)
     int x, *ignore;
{
  int r;
  _rl_vimotion_cxt *m;

  m = _rl_vimvcxt;
  *ignore = m->motion = rl_vi_domove_getchar (m);

  if (m->motion < 0)
    {
      m->motion = 0;
      return -1;
    }

  return (rl_domove_read_callback (m));
}

static int
vi_delete_dispatch (m)
     _rl_vimotion_cxt *m;
{
  /* These are the motion commands that do not require adjusting the
     mark. */
  if (((strchr (" l|h^0bBFT`", m->motion) == 0) && (rl_point >= m->start)) &&
      (rl_mark < rl_end))
    rl_mark++;

  rl_kill_text (rl_point, rl_mark);
  return (0);
}

int
rl_vi_delete_to (count, key)
     int count, key;
{
  int c, r;

  _rl_vimvcxt = _rl_mvcxt_alloc (VIM_DELETE, key);
  _rl_vimvcxt->start = rl_point;

  rl_mark = rl_point;
  if (_rl_uppercase_p (key))
    {
      _rl_vimvcxt->motion = '$';
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing && _rl_vi_last_motion != 'd')	/* `dd' is special */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing)		/* handle redoing `dd' here */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      rl_mark = rl_end;
      rl_beg_of_line (1, key);
      RL_UNSETSTATE (RL_STATE_VIMOTION);
      r = vidomove_dispatch (_rl_vimvcxt);
    }
#if defined (READLINE_CALLBACKS)
  else if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      RL_SETSTATE (RL_STATE_VIMOTION);
      return (0);
    }
#endif
  else
    r = rl_vi_domove (key, &c);

  if (r < 0)
    {
      rl_ding ();
      r = -1;
    }

  _rl_mvcxt_dispose (_rl_vimvcxt);
  _rl_vimvcxt = 0;

  return r;
}

static int
vi_change_dispatch (m)
     _rl_vimotion_cxt *m;
{
  /* These are the motion commands that do not require adjusting the
     mark.  c[wW] are handled by special-case code in rl_vi_domove(),
     and already leave the mark at the correct location. */
  if (((strchr (" l|hwW^0bBFT`", m->motion) == 0) && (rl_point >= m->start)) &&
      (rl_mark < rl_end))
    rl_mark++;

  /* The cursor never moves with c[wW]. */
  if ((_rl_to_upper (m->motion) == 'W') && rl_point < m->start)
    rl_point = m->start;

  if (vi_redoing)
    {
      if (vi_insert_buffer && *vi_insert_buffer)
	rl_begin_undo_group ();
      rl_delete_text (rl_point, rl_mark);
      if (vi_insert_buffer && *vi_insert_buffer)
	{
	  rl_insert_text (vi_insert_buffer);
	  rl_end_undo_group ();
	}
    }
  else
    {
      rl_begin_undo_group ();		/* to make the `u' command work */
      rl_kill_text (rl_point, rl_mark);
      /* `C' does not save the text inserted for undoing or redoing. */
      if (_rl_uppercase_p (m->key) == 0)
	_rl_vi_doing_insert = 1;
      /* XXX -- TODO -- use m->numericarg? */
      rl_vi_start_inserting (m->key, rl_numeric_arg, rl_arg_sign);
    }

  return (0);
}

int
rl_vi_change_to (count, key)
     int count, key;
{
  int c, r;

  _rl_vimvcxt = _rl_mvcxt_alloc (VIM_CHANGE, key);
  _rl_vimvcxt->start = rl_point;

  rl_mark = rl_point;
  if (_rl_uppercase_p (key))
    {
      _rl_vimvcxt->motion = '$';
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing && _rl_vi_last_motion != 'c')	/* `cc' is special */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing)		/* handle redoing `cc' here */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      rl_mark = rl_end;
      rl_beg_of_line (1, key);
      RL_UNSETSTATE (RL_STATE_VIMOTION);
      r = vidomove_dispatch (_rl_vimvcxt);
    }
#if defined (READLINE_CALLBACKS)
  else if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      RL_SETSTATE (RL_STATE_VIMOTION);
      return (0);
    }
#endif
  else
    r = rl_vi_domove (key, &c);

  if (r < 0)
    {
      rl_ding ();
      r = -1;	/* normalize return value */
    }

  _rl_mvcxt_dispose (_rl_vimvcxt);
  _rl_vimvcxt = 0;

  return r;
}

static int
vi_yank_dispatch (m)
     _rl_vimotion_cxt *m;
{
  /* These are the motion commands that do not require adjusting the
     mark. */
  if (((strchr (" l|h^0%bBFT`", m->motion) == 0) && (rl_point >= m->start)) &&
      (rl_mark < rl_end))
    rl_mark++;

  rl_begin_undo_group ();
  rl_kill_text (rl_point, rl_mark);
  rl_end_undo_group ();
  rl_do_undo ();
  rl_point = m->start;

  return (0);
}

int
rl_vi_yank_to (count, key)
     int count, key;
{
  int c, r;

  _rl_vimvcxt = _rl_mvcxt_alloc (VIM_YANK, key);
  _rl_vimvcxt->start = rl_point;

  rl_mark = rl_point;
  if (_rl_uppercase_p (key))
    {
      _rl_vimvcxt->motion = '$';
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing && _rl_vi_last_motion != 'y')	/* `yy' is special */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      r = rl_domove_motion_callback (_rl_vimvcxt);
    }
  else if (vi_redoing)			/* handle redoing `yy' here */
    {
      _rl_vimvcxt->motion = _rl_vi_last_motion;
      rl_mark = rl_end;
      rl_beg_of_line (1, key);
      RL_UNSETSTATE (RL_STATE_VIMOTION);
      r = vidomove_dispatch (_rl_vimvcxt);
    }
#if defined (READLINE_CALLBACKS)
  else if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      RL_SETSTATE (RL_STATE_VIMOTION);
      return (0);
    }
#endif
  else
    r = rl_vi_domove (key, &c);

  if (r < 0)
    {
      rl_ding ();
      r = -1;
    }

  _rl_mvcxt_dispose (_rl_vimvcxt);
  _rl_vimvcxt = 0;

  return r;
}

static int
vidomove_dispatch (m)
     _rl_vimotion_cxt *m;
{
  int r;

  switch (m->op)
    {
    case VIM_DELETE:
      r = vi_delete_dispatch (m);
      break;
    case VIM_CHANGE:
      r = vi_change_dispatch (m);
      break;
    case VIM_YANK:
      r = vi_yank_dispatch (m);
      break;
    default:
      _rl_errmsg ("vidomove_dispatch: unknown operator %d", m->op);
      r = 1;
      break;
    }

  RL_UNSETSTATE (RL_STATE_VIMOTION);
  return r;
}

int
rl_vi_rubout (count, key)
     int count, key;
{
  int opoint;

  if (count < 0)
    return (rl_vi_delete (-count, key));

  if (rl_point == 0)
    {
      rl_ding ();
      return -1;
    }

  opoint = rl_point;
  if (count > 1 && MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    rl_backward_char (count, key);
  else if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    rl_point = _rl_find_prev_mbchar (rl_line_buffer, rl_point, MB_FIND_NONZERO);
  else
    rl_point -= count;

  if (rl_point < 0)
    rl_point = 0;

  rl_kill_text (rl_point, opoint);
  
  return (0);
}

int
rl_vi_delete (count, key)
     int count, key;
{
  int end;

  if (count < 0)
    return (rl_vi_rubout (-count, key));

  if (rl_end == 0)
    {
      rl_ding ();
      return -1;
    }

  if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    end = _rl_find_next_mbchar (rl_line_buffer, rl_point, count, MB_FIND_NONZERO);
  else
    end = rl_point + count;

  if (end >= rl_end)
    end = rl_end;

  rl_kill_text (rl_point, end);
  
  if (rl_point > 0 && rl_point == rl_end)
    rl_backward_char (1, key);

  return (0);
}

int
rl_vi_back_to_indent (count, key)
     int count, key;
{
  rl_beg_of_line (1, key);
  while (rl_point < rl_end && whitespace (rl_line_buffer[rl_point]))
    rl_point++;
  return (0);
}

int
rl_vi_first_print (count, key)
     int count, key;
{
  return (rl_vi_back_to_indent (1, key));
}

static int _rl_cs_dir, _rl_cs_orig_dir;

#if defined (READLINE_CALLBACKS)
static int
_rl_vi_callback_char_search (data)
     _rl_callback_generic_arg *data;
{
  int c;
#if defined (HANDLE_MULTIBYTE)
  c = _rl_vi_last_search_mblen = _rl_read_mbchar (_rl_vi_last_search_mbchar, MB_LEN_MAX);
#else
  RL_SETSTATE(RL_STATE_MOREINPUT);
  c = rl_read_key ();
  RL_UNSETSTATE(RL_STATE_MOREINPUT);
#endif

  if (c <= 0)
    return -1;

#if !defined (HANDLE_MULTIBYTE)
  _rl_vi_last_search_char = c;
#endif

  _rl_callback_func = 0;
  _rl_want_redisplay = 1;

#if defined (HANDLE_MULTIBYTE)
  return (_rl_char_search_internal (data->count, _rl_cs_dir, _rl_vi_last_search_mbchar, _rl_vi_last_search_mblen));
#else
  return (_rl_char_search_internal (data->count, _rl_cs_dir, _rl_vi_last_search_char));
#endif  
}
#endif

int
rl_vi_char_search (count, key)
     int count, key;
{
  int c;
#if defined (HANDLE_MULTIBYTE)
  static char *target;
  static int tlen;
#else
  static char target;
#endif

  if (key == ';' || key == ',')
    {
      if (_rl_cs_orig_dir == 0)
	return -1;
#if defined (HANDLE_MULTIBYTE)
      if (_rl_vi_last_search_mblen == 0)
	return -1;
#else
      if (_rl_vi_last_search_char == 0)
	return -1;
#endif
      _rl_cs_dir = (key == ';') ? _rl_cs_orig_dir : -_rl_cs_orig_dir;
    }
  else
    {
      switch (key)
	{
	case 't':
	  _rl_cs_orig_dir = _rl_cs_dir = FTO;
	  break;

	case 'T':
	  _rl_cs_orig_dir = _rl_cs_dir = BTO;
	  break;

	case 'f':
	  _rl_cs_orig_dir = _rl_cs_dir = FFIND;
	  break;

	case 'F':
	  _rl_cs_orig_dir = _rl_cs_dir = BFIND;
	  break;
	}

      if (vi_redoing)
	{
	  /* set target and tlen below */
	}
#if defined (READLINE_CALLBACKS)
      else if (RL_ISSTATE (RL_STATE_CALLBACK))
	{
	  _rl_callback_data = _rl_callback_data_alloc (count);
	  _rl_callback_data->i1 = _rl_cs_dir;
	  _rl_callback_func = _rl_vi_callback_char_search;
	  return (0);
	}
#endif
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  c = _rl_read_mbchar (_rl_vi_last_search_mbchar, MB_LEN_MAX);
	  if (c <= 0)
	    return -1;
	  _rl_vi_last_search_mblen = c;
#else
	  RL_SETSTATE(RL_STATE_MOREINPUT);
	  c = rl_read_key ();
	  RL_UNSETSTATE(RL_STATE_MOREINPUT);
	  if (c < 0)
	    return -1;
	  _rl_vi_last_search_char = c;
#endif
	}
    }

#if defined (HANDLE_MULTIBYTE)
  target = _rl_vi_last_search_mbchar;
  tlen = _rl_vi_last_search_mblen;
#else
  target = _rl_vi_last_search_char;
#endif

#if defined (HANDLE_MULTIBYTE)
  return (_rl_char_search_internal (count, _rl_cs_dir, target, tlen));
#else
  return (_rl_char_search_internal (count, _rl_cs_dir, target));
#endif
}

/* Match brackets */
int
rl_vi_match (ignore, key)
     int ignore, key;
{
  int count = 1, brack, pos, tmp, pre;

  pos = rl_point;
  if ((brack = rl_vi_bracktype (rl_line_buffer[rl_point])) == 0)
    {
      if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
	{
	  while ((brack = rl_vi_bracktype (rl_line_buffer[rl_point])) == 0)
	    {
	      pre = rl_point;
	      rl_forward_char (1, key);
	      if (pre == rl_point)
		break;
	    }
	}
      else
	while ((brack = rl_vi_bracktype (rl_line_buffer[rl_point])) == 0 &&
		rl_point < rl_end - 1)
	  rl_forward_char (1, key);

      if (brack <= 0)
	{
	  rl_point = pos;
	  rl_ding ();
	  return -1;
	}
    }

  pos = rl_point;

  if (brack < 0)
    {
      while (count)
	{
	  tmp = pos;
	  if (MB_CUR_MAX == 1 || rl_byte_oriented)
	    pos--;
	  else
	    {
	      pos = _rl_find_prev_mbchar (rl_line_buffer, pos, MB_FIND_ANY);
	      if (tmp == pos)
		pos--;
	    }
	  if (pos >= 0)
	    {
	      int b = rl_vi_bracktype (rl_line_buffer[pos]);
	      if (b == -brack)
		count--;
	      else if (b == brack)
		count++;
	    }
	  else
	    {
	      rl_ding ();
	      return -1;
	    }
	}
    }
  else
    {			/* brack > 0 */
      while (count)
	{
	  if (MB_CUR_MAX == 1 || rl_byte_oriented)
	    pos++;
	  else
	    pos = _rl_find_next_mbchar (rl_line_buffer, pos, 1, MB_FIND_ANY);

	  if (pos < rl_end)
	    {
	      int b = rl_vi_bracktype (rl_line_buffer[pos]);
	      if (b == -brack)
		count--;
	      else if (b == brack)
		count++;
	    }
	  else
	    {
	      rl_ding ();
	      return -1;
	    }
	}
    }
  rl_point = pos;
  return (0);
}

int
rl_vi_bracktype (c)
     int c;
{
  switch (c)
    {
    case '(': return  1;
    case ')': return -1;
    case '[': return  2;
    case ']': return -2;
    case '{': return  3;
    case '}': return -3;
    default:  return  0;
    }
}

static int
_rl_vi_change_char (count, c, mb)
     int count, c;
     char *mb;
{
  int p;

  if (c == '\033' || c == CTRL ('C'))
    return -1;

  rl_begin_undo_group ();
  while (count-- && rl_point < rl_end)
    {
      p = rl_point;
      rl_vi_delete (1, c);
      if (rl_point < p)		/* Did we retreat at EOL? */
	rl_point++;
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
	rl_insert_text (mb);
      else
#endif
	_rl_insert_char (1, c);
    }

  /* The cursor shall be left on the last character changed. */
  rl_backward_char (1, c);

  rl_end_undo_group ();

  return (0);
}

static int
_rl_vi_callback_getchar (mb, mlen)
     char *mb;
     int mlen;
{
  int c;

  RL_SETSTATE(RL_STATE_MOREINPUT);
  c = rl_read_key ();
  RL_UNSETSTATE(RL_STATE_MOREINPUT);

  if (c < 0)
    return -1;

#if defined (HANDLE_MULTIBYTE)
  if (MB_CUR_MAX > 1 && rl_byte_oriented == 0)
    c = _rl_read_mbstring (c, mb, mlen);
#endif

  return c;
}

#if defined (READLINE_CALLBACKS)
static int
_rl_vi_callback_change_char (data)
     _rl_callback_generic_arg *data;
{
  int c;
  char mb[MB_LEN_MAX];

  _rl_vi_last_replacement = c = _rl_vi_callback_getchar (mb, MB_LEN_MAX);

  if (c < 0)
    return -1;

  _rl_callback_func = 0;
  _rl_want_redisplay = 1;

  return (_rl_vi_change_char (data->count, c, mb));
}
#endif

int
rl_vi_change_char (count, key)
     int count, key;
{
  int c;
  char mb[MB_LEN_MAX];

  if (vi_redoing)
    {
      c = _rl_vi_last_replacement;
      mb[0] = c;
      mb[1] = '\0';
    }
#if defined (READLINE_CALLBACKS)
  else if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = _rl_callback_data_alloc (count);
      _rl_callback_func = _rl_vi_callback_change_char;
      return (0);
    }
#endif
  else
    _rl_vi_last_replacement = c = _rl_vi_callback_getchar (mb, MB_LEN_MAX);

  if (c < 0)
    return -1;

  return (_rl_vi_change_char (count, c, mb));
}

int
rl_vi_subst (count, key)
     int count, key;
{
  /* If we are redoing, rl_vi_change_to will stuff the last motion char */
  if (vi_redoing == 0)
    rl_stuff_char ((key == 'S') ? 'c' : 'l');	/* `S' == `cc', `s' == `cl' */

  return (rl_vi_change_to (count, 'c'));
}

int
rl_vi_overstrike (count, key)
     int count, key;
{
  if (_rl_vi_doing_insert == 0)
    {
      _rl_vi_doing_insert = 1;
      rl_begin_undo_group ();
    }

  if (count > 0)
    {
      _rl_overwrite_char (count, key);
      vi_replace_count += count;
    }

  return (0);
}

int
rl_vi_overstrike_delete (count, key)
     int count, key;
{
  int i, s;

  for (i = 0; i < count; i++)
    {
      if (vi_replace_count == 0)
	{
	  rl_ding ();
	  break;
	}
      s = rl_point;

      if (rl_do_undo ())
	vi_replace_count--;

      if (rl_point == s)
	rl_backward_char (1, key);
    }

  if (vi_replace_count == 0 && _rl_vi_doing_insert)
    {
      rl_end_undo_group ();
      rl_do_undo ();
      _rl_vi_doing_insert = 0;
    }
  return (0);
}

int
rl_vi_replace (count, key)
     int count, key;
{
  int i;

  vi_replace_count = 0;

  if (vi_replace_map == 0)
    {
      vi_replace_map = rl_make_bare_keymap ();

      for (i = 0; i < ' '; i++)
	if (vi_insertion_keymap[i].type == ISFUNC)
	  vi_replace_map[i].function = vi_insertion_keymap[i].function;

      for (i = ' '; i < KEYMAP_SIZE; i++)
	vi_replace_map[i].function = rl_vi_overstrike;

      vi_replace_map[RUBOUT].function = rl_vi_overstrike_delete;

      /* Make sure these are what we want. */
      vi_replace_map[ESC].function = rl_vi_movement_mode;
      vi_replace_map[RETURN].function = rl_newline;
      vi_replace_map[NEWLINE].function = rl_newline;

      /* If the normal vi insertion keymap has ^H bound to erase, do the
	 same here.  Probably should remove the assignment to RUBOUT up
	 there, but I don't think it will make a difference in real life. */
      if (vi_insertion_keymap[CTRL ('H')].type == ISFUNC &&
	  vi_insertion_keymap[CTRL ('H')].function == rl_rubout)
	vi_replace_map[CTRL ('H')].function = rl_vi_overstrike_delete;

    }

  rl_vi_start_inserting (key, 1, rl_arg_sign);

  _rl_vi_last_key_before_insert = key;
  _rl_keymap = vi_replace_map;

  return (0);
}

#if 0
/* Try to complete the word we are standing on or the word that ends with
   the previous character.  A space matches everything.  Word delimiters are
   space and ;. */
int
rl_vi_possible_completions()
{
  int save_pos = rl_point;

  if (rl_line_buffer[rl_point] != ' ' && rl_line_buffer[rl_point] != ';')
    {
      while (rl_point < rl_end && rl_line_buffer[rl_point] != ' ' &&
	     rl_line_buffer[rl_point] != ';')
	rl_point++;
    }
  else if (rl_line_buffer[rl_point - 1] == ';')
    {
      rl_ding ();
      return (0);
    }

  rl_possible_completions ();
  rl_point = save_pos;

  return (0);
}
#endif

/* Functions to save and restore marks. */
static int
_rl_vi_set_mark ()
{
  int ch;

  RL_SETSTATE(RL_STATE_MOREINPUT);
  ch = rl_read_key ();
  RL_UNSETSTATE(RL_STATE_MOREINPUT);

  if (ch < 0 || ch < 'a' || ch > 'z')	/* make test against 0 explicit */
    {
      rl_ding ();
      return -1;
    }
  ch -= 'a';
  vi_mark_chars[ch] = rl_point;
  return 0;
}

#if defined (READLINE_CALLBACKS)
static int
_rl_vi_callback_set_mark (data)
     _rl_callback_generic_arg *data;
{
  _rl_callback_func = 0;
  _rl_want_redisplay = 1;

  return (_rl_vi_set_mark ());
}
#endif

int
rl_vi_set_mark (count, key)
     int count, key;
{
#if defined (READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = 0;
      _rl_callback_func = _rl_vi_callback_set_mark;
      return (0);
    }
#endif

  return (_rl_vi_set_mark ());
}

static int
_rl_vi_goto_mark ()
{
  int ch;

  RL_SETSTATE(RL_STATE_MOREINPUT);
  ch = rl_read_key ();
  RL_UNSETSTATE(RL_STATE_MOREINPUT);

  if (ch == '`')
    {
      rl_point = rl_mark;
      return 0;
    }
  else if (ch < 0 || ch < 'a' || ch > 'z')	/* make test against 0 explicit */
    {
      rl_ding ();
      return -1;
    }

  ch -= 'a';
  if (vi_mark_chars[ch] == -1)
    {
      rl_ding ();
      return -1;
    }
  rl_point = vi_mark_chars[ch];
  return 0;
}

#if defined (READLINE_CALLBACKS)
static int
_rl_vi_callback_goto_mark (data)
     _rl_callback_generic_arg *data;
{
  _rl_callback_func = 0;
  _rl_want_redisplay = 1;

  return (_rl_vi_goto_mark ());
}
#endif

int
rl_vi_goto_mark (count, key)
     int count, key;
{
#if defined (READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = 0;
      _rl_callback_func = _rl_vi_callback_goto_mark;
      return (0);
    }
#endif

  return (_rl_vi_goto_mark ());
}
#endif /* VI_MODE */
