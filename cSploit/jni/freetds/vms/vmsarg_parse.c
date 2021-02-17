/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2010  Craig A. Berry	craigberry@mac.com
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

/*
 * Program to Parse VMS style arguments.  The calling program passes
 * argc, argv into this routine, and we translate the VMS qualifiers into
 * Unix style arguments according to a user specified translation table.
 * It then puts these into a new argv and returns that address to the
 * caller.  The calling program then happily proceeds with the Unix style
 * qualifiers that it was written to deal with.
 *
 * Based on VMSARG Version 2.0 by Tom Wade <t.wade@vms.eurokom.ei>
 *
 * Extensively revised for inclusion in FreeTDS by Craig A. Berry.
 *
 * From the VMSARG 2.0 documentation:
 *
 * The product is aimed at . . . people who are porting a package from 
 * Unix to VMS. This software is made freely available for inclusion in 
 * such products, whether they are freeware, public domain or commercial. 
 * No licensing is required.
 */


#include "vargdefs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <climsgdef.h>
#include <descrip.h>
#include <libdef.h>
#include <ssdef.h>

#include <cli$routines.h>
#include <lbr$routines.h>
#include <lib$routines.h>
#include <starlet.h>

/*	Increment arg counter and crash out if we have too many
*/

#define Increment(x) x = x + 1;\
                     if (x > MAX_ARGS) sys$exit(SS$_OVRMAXARG);

/*	Declare internal static routines
*/

static int varg_get_index(), varg_set_index(), varg_convert_date(), varg_do_help();
static int varg_protect_string(), varg_set_symbol(), varg_do_command();
static int varg_do_exit();

parse_vms_args(int *argc, char **argv[])
{
        static char command[S_LENGTH], value_template[S_LENGTH];
        static char qual_template[S_LENGTH];
        char verb[] = "VARG ";
        char *vms_arg[MAX_ARGS];
        char *unix_arg[MAX_ARGS];
        char *unix_narg[MAX_ARGS];
        char *vms_key[MAX_ARGS];
        char *unix_key[MAX_ARGS];
        char *separator[MAX_ARGS];
        char *pattern[MAX_ARGS];
        char *outverb, *arg_symbol, *image_name;
        char foreign_image[S_LENGTH];
        int flags[MAX_ARGS], action_flags;
        int  nvargs, u_length, l_element, index;
        char arg_string[S_LENGTH], arg_element[S_LENGTH];
        static char value[S_LENGTH];
        char *special_chars;
        char default_special[] = "!@#^&()=+\'\"/?,";
        $DESCRIPTOR(command_desc, command);
        $DESCRIPTOR(qual_desc, "");
        $DESCRIPTOR(value_desc, value);
        int i, length, status, i_length, arg_count, q_length;
        static char command_args[S_LENGTH];
        $DESCRIPTOR(command_arg_desc, command_args);
        extern VARG_CLD;
        static	char *newargv[MAX_ARGS];
        int affirm_flag, negative_flag, keywords_flag, separator_flag;
        int date_flag, append_flag, present, more_flag, multivalue, help_flag;
        int vmsarg_mapping();

        /*	Get the VMS -> Unix mapping definitions from the routine generated
        	by the GEN_MAPPING utility.
        */

        vmsarg_mapping(&nvargs, vms_arg, unix_arg, unix_narg, vms_key, unix_key,
                       separator, flags, pattern, &outverb, &action_flags,
                       &arg_symbol, &image_name );

        /*	Get the original command line from the foreign command utility.
        */

        length = 0;					/* RTL only writes word */
        lib$get_foreign(&command_arg_desc, 0, &length);
        command_args[length] = 0;			/* add NULL terminator */

        /*	Check to see if any of the exising arguments start with a "-"
        	character.  if so, assume he's already using Unix style arguments so
        	we don't have to do anything.  Only do this if the Unix_Arg bit is
        	set in the action flags.
        */

        if ((action_flags & VARGACT_M_UNIXARG)) {
                for (i = 1; i <= *argc - 1; i++)
                        if ((*argv) [i][0] == '-') {
                                /* He's using Unix style args. */
                                if ((action_flags & VARGACT_M_RETURN)) {
                                        return 1;		/* back out quietly	*/
                                } else {
                                        status = varg_do_exit(action_flags, arg_symbol, command_args,
                                                          image_name, outverb);
                                        sys$exit(status);	/* exit with DCL stuff done  */
                                }
                        }
        }

        /*	Catenate verb and arguments into the command string for DCL
        	to parse.
        */

        strcpy(command, verb);
        strcat(command, command_args);
        command_desc.dsc$w_length = strlen(verb) + strlen(command_args);

        /*	Pass it to the DCL parser for its approval.
        */

        status = cli$dcl_parse(&command_desc, &VARG_CLD);
        if ((status & 1) != 1) sys$exit (status);

        /*	Now that DCL is happy, we go through all the defined VMS qualifiers
        	and parameters to check if they are present or defaulted.
        */

        arg_count = 0;		/* so far so good */

        for (i = 1; i <= nvargs; i++) {

                /*	Set The various flags for this qualifier.
                */

                affirm_flag = (VARG_M_AFFIRM & flags[i]) != 0;
                negative_flag = (VARG_M_NEGATIVE & flags[i]) != 0;
                keywords_flag = (VARG_M_KEYWORDS & flags[i]) != 0;
                separator_flag = (VARG_M_SEPARATOR & flags[i]) != 0;
                date_flag = (VARG_M_DATE & flags[i]) != 0;
                append_flag = (VARG_M_APPEND & flags[i]) != 0;
                help_flag = (VARG_M_HELP & flags[i]) != 0;

                /*	Check if the qualifier or parameter is present
                */

                qual_desc.dsc$w_length = strlen(vms_arg[i]);
                qual_desc.dsc$a_pointer = vms_arg[i];
                present = cli$present(&qual_desc) & 1;

                /*	if the qualifier is present, and there is a positive translation,
                	then replace the VMS qualifier by its Unix equivalent.  Note that
                	if the Unix equivalent starts with a '$', then it is a parameter and
                	we omit the qualifier name, only putting in the value.
                */

                qual_template[0] = 0;			/* zero string initially */

                if (present && affirm_flag) {
                        if (unix_arg[i][0] != '$') strcpy(qual_template, unix_arg[i]);
                }

                if (!(present & 1) && negative_flag)		/* NOT present AND
						   negative_flag */
                {
                        strcpy(qual_template, unix_narg[i]);
                }

                /*	Get the first value (if any).  We may have to catenate this with the
                	qualifier keyword, or build up multiple values in a single argv
                	element.
                */

                if (present) {
                        value_desc.dsc$w_length = QUAL_LENGTH;
                        status = cli$get_value(&qual_desc, &value_desc, &length);
                        if ((status & 1) != 1) length = 0;
                        value[length] = 0;			/* NULL terminator */
                        more_flag = status == CLI$_COMMA;
                } else {
                        length = 0;
                }

                /*	if the /APPEND qualifier was used, then the qualifier and its value
                	are in the same argv element.
                */

                if (append_flag) {
                        strcpy(value_template, qual_template);
                } else {
                        /*	Allocate a fresh argv element and load the Unix keyword into it.
                        */
                        q_length = strlen(qual_template);

                        if (q_length != 0) {
                                Increment(arg_count);
                                newargv[arg_count] = malloc(q_length + 1);
                                if (newargv[arg_count] == NULL) sys$exit(LIB$_INSVIRMEM);
                                strcpy(newargv[arg_count], qual_template);
                        }
                }

                multivalue = more_flag;

                /*	Get all the various values and either append them with appropriate
                	separators to the current template, or load them in successive argv
                	elements.
                */

                while (affirm_flag && (length != 0)) {

                        /*	if a keyword list is involved, get the corresponding field from the
                        	Unix equivalent.
                        */

                        if (keywords_flag) {
                                index = varg_get_index(value, vms_key[i]);
                                varg_set_index(value, unix_key[i], index);
                        }

                        /*	if the /DATE qualifier was present, we interpret the value as a
                        	VMS date/time specification, and convert it to the equivalent Unix
                        	type string.
                        */

                        if (date_flag) varg_convert_date(value, pattern[i]);

                        /*	Add the current value to the template so far.  Add a separator
                        	character if we need one.
                        */

                        strcat(value_template, value);

                        if (separator_flag) {
                                if (more_flag)
                                        strcat(value_template, separator[i]);
                        } else {

                                /*	Each value should have its own argv element.
                                */

                                Increment(arg_count);
                                newargv[arg_count] = malloc(strlen(value_template) + 1);
                                if (newargv[arg_count] == NULL) sys$exit(LIB$_INSVIRMEM);
                                strcpy(newargv[arg_count], value_template);
                                value_template[0] = 0;
                        }

                        /*	Get the next value from the DCL parser.
                        */

                        value_desc.dsc$w_length = QUAL_LENGTH;		/* reset string */
                        status = cli$get_value(&qual_desc, &value_desc, &length);
                        if ((status & 1) != 1) length = 0;
                        value[length] = 0;
                        more_flag = status == CLI$_COMMA;
                }

                /*	All the values are in for this qualifier - if there were being stacked
                	up then load them all into an argv element.
                */

                if (separator_flag & multivalue) {
                        Increment(arg_count);
                        newargv[arg_count] = malloc(strlen(value_template) + 1);
                        if (newargv[arg_count] == NULL) sys$exit(LIB$_INSVIRMEM);
                        strcpy(newargv[arg_count], value_template);
                        value_template[0] = 0;
                }

                /*	Check to see if this is a HELP directive.  if so, then the help
                	library name is stored in pattern[i] and the initial help string
                	in unix_arg[i]
                */

                if ((present &1) && help_flag) {
                        varg_do_help(pattern[i], unix_arg[i]);
                }

                /*	And around for the next qualifier.
                */

        }

        /*	We've built the argument vector.  Now lets find out what we're
        	supposed to do with it.
        */

        if ((action_flags & VARGACT_M_RETURN)) {

                /*	We should return the result to the caller. Set the corresponding argc
                	& argv variables in the caller to point at these.  Make sure the new
                	argv contains the same [0] element as the old one (program name).
                */

                *argc = arg_count + 1;
                newargv[0] = (*argv)[0];
                *argv = newargv;
                return 1;
        }

        /*	We must convert the argv elements into an arg string.
        */

        arg_string[0] = 0;
        special_chars = default_special;

        for (i = 1; i <= arg_count; i++) {
                varg_protect_string(newargv[i], arg_element, action_flags,
                                special_chars);
                strcat(arg_string, arg_element);
                if (i != arg_count) strcat(arg_string, " ");
        }

        status = varg_do_exit(action_flags, arg_symbol, arg_string, image_name, outverb);

        /*	if we got this far then exit to DCL.
        */

        sys$exit(status);
}

/*****************************************************************************|
|	Routine to scan the string 'string' to see if it matches any of       |
|	the fields in 'keywords' (fields delimited by "|" characters). if     |
|	so, return the field number (1=first).  if it doesn't match, return   |
|	0.                                                                    |
\*****************************************************************************/

static int varg_get_index(char *string, char *keywords)
{
        int i;
        char temp[S_LENGTH];
        char *field;

        i = 0;
        strcpy(temp, keywords);		/* don't muck up caller's string.  */

        /*	Get successive fields using STRTOK function and compare.
        */

        field = strtok(temp, "|");

        while (field != NULL) {
                i = i + 1;
                if (strcmp(field, string) == 0) return i;
                field = strtok(NULL, "|");
        }

        return 0;
}

/*****************************************************************************|
|	Routine to return the 'index' numbered field of 'keywords' in         |
|	'string'.  Keywords are delimited by "|" characters.  if 'index'      |
|	is 0 do nothing.                                                      |
\*****************************************************************************/

static int varg_set_index(char *string, char *keywords, int index)
{
        int i;
        char temp[S_LENGTH];
        char *field;

        if (index == 0) return 1;
        strcpy(temp, keywords);		/* don't screw up caller's string. */
        field = strtok(temp, "|");

        for (i = 1; i <= index - 1; i++) {
                field = strtok(NULL, "|");
        }

        strcpy(string, field);
        return 1;
}

/*****************************************************************************|
|	Invoke a help dialog with the user using the supplied help library    |
|	and initial help command.                                             |
\*****************************************************************************/

static int varg_do_help(char *help_library, char *help_command)
{
        $DESCRIPTOR(lib_desc, "");
        $DESCRIPTOR(command_desc, "");
        int status;

        lib_desc.dsc$w_length = strlen(help_library);
        lib_desc.dsc$a_pointer = help_library;
        command_desc.dsc$w_length = strlen(help_command);
        command_desc.dsc$a_pointer = help_command;

        status = lbr$output_help(&lib$put_output, 0, &command_desc, &lib_desc, 0,
                                 &lib$get_input);
        return status;
}

/*****************************************************************************|
|	Routine to create a DCL symbol called 'symbol' with value 'value'.    |
\*****************************************************************************/

static int varg_set_symbol(char *symbol, char *value)
{
        $DESCRIPTOR(symbol_desc, "");
        $DESCRIPTOR(value_desc, "");
        int status;

        symbol_desc.dsc$w_length = strlen(symbol);
        symbol_desc.dsc$a_pointer = symbol;
        value_desc.dsc$w_length = strlen(value);
        value_desc.dsc$a_pointer = value;
        status = lib$set_symbol(&symbol_desc, &value_desc);
        return status;
}

/*****************************************************************************|
|	Routine to exit the program and pass 'command' to DCL for execution.  |
\*****************************************************************************/

static int varg_do_command(char *command)
{
        $DESCRIPTOR(command_desc, "");
        int status;

        command_desc.dsc$w_length = strlen(command);
        command_desc.dsc$a_pointer = command;
        status = lib$do_command(&command_desc);
        return status;
}

/*****************************************************************************|
|	Routine to rebuild 'date_string' converting from a VMS standard       |
|	date/time specifier to the Unix format specified in 'pattern'.        |
|                                                                             |
\*****************************************************************************/

static int varg_convert_date(char *date_string, char *pattern)
{
        char date_text[30];
        int bin_time[2], flags, length, status, i, from, to;
        char year_4[5], year_2[4], month_name[4];
        char month_2[2], month_1[2], date_2[3], date_1[2];
        char hour_2[3], hour_1[2], minute[3], second[3];
        char *months[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                         };
        $DESCRIPTOR(date_desc, "");
        static int context;


        /*	First convert the date string into internal VMS format
        */

        date_desc.dsc$w_length = strlen(date_string);
        date_desc.dsc$a_pointer = date_string;
        context = 0;
        flags = 0X7F;
        status = lib$convert_date_string(&date_desc, &bin_time, &context, &flags);
        if ((status & 1) != 1) return status;

        /*	Now convert it back to string form.  This ensures that delta times
        	and keywords like TODAY get translated into a consistent format.
        */

        date_desc.dsc$w_length = 30;
        /* date_desc.dsc$a_pointer = &date_text;*/
        date_desc.dsc$a_pointer = date_text;
        length = 0;
        status = sys$asctim(&length, &date_desc, &bin_time, 0);
        if ((status & 1) != 1) return status;

        date_text[length] = 0;		/* add NULL terminator */
        date_string[0] = 0;			/* start with empty string */

        /*	Break the date/time string up into the constituent fields.  Fields
        	such as date have two instances - one with leading zero and one without
        	e.g. "05" and "5".
        */

        if (date_text[0] == ' ') {
                date_2[0] = '0';
                date_2[1] = date_text[1];
                date_2[2] = 0;
                date_1[0] = date_text[1];
                date_1[1] = 0;
        } else {
                date_2[0] = date_text[0];
                date_2[1] = date_text[1];
                date_2[2] = 0;
                strcpy(date_1, date_2);
        }

        /*	Extract the month name and lowercase the 2nd and 3rd characters.
        */

        month_name[0] = date_text [3];
        month_name[1] = tolower(date_text [4]);
        month_name[2] = tolower(date_text [5]);
        month_name[3] = 0;

        /*	Calculate the month number by looking up the month name in a
        	character array.
        */

        for (i = 1; i <= 12; i++) {
                if (strcmp(month_name, months[i]) == 0) {
                        sprintf(month_2, "%02d", i);
                        sprintf(month_1, "%d", i);
                        break;
                }
        }

        /*	Extract the year field (both 4 digit and 2 digit forms).
        */

        year_2[0] = date_text[9];
        year_2[1] = date_text[10];
        year_2[3] = 0;

        for (i = 0; i <= 3; i++) {
                year_4[i] = date_text[i+7];
        }

        year_4[4] = 0;

        /*	Extract hour field.
        */

        hour_2[0] = date_text[12];
        hour_2[1] = date_text[13];
        hour_2[2] = 0;

        if (date_text[12] == '0') {
                hour_1[0] = date_text[13];
                hour_1[1] = 0;
        } else strcpy(hour_1, hour_2);

        /*	Lastly copy the minutes and seconds.
        */

        for (i = 0; i <= 1; i++) {
                minute[i] = date_text[i+15];
                second[i] = date_text[i+18];
        }

        minute[2] = 0;
        second[2] = 0;

        /*	Now start building up the output string by copying characters from
        	'pattern' to 'date_string' substituting the various fields when we
        	encounter an escape pair.
        */

        from = 0;
        to = 0;

        while (pattern[from] != 0) {
                if (pattern[from] == '$') {
                        switch (pattern[from+1]) {
                        case 'Y':
                                strcat(date_string, year_4);
                                to = to + 4;
                                break;

                        case 'y':
                                strcat(date_string, year_2);
                                to = to + 2;
                                break;

                        case 'M':
                                strcat(date_string, month_name);
                                to = to + 3;
                                break;

                        case 'N':
                                strcat(date_string, month_2);
                                to = to + 2;
                                break;

                        case 'n':
                                strcat(date_string, month_1);
                                to = to + strlen(month_1);
                                break;

                        case 'D':
                                strcat(date_string, date_2);
                                to = to + 2;
                                break;

                        case 'd':
                                strcat(date_string, date_1);
                                to = to + strlen(date_1);
                                break;

                        case 'H':
                                strcat(date_string, hour_2);
                                to = to + 2;
                                break;

                        case 'h':
                                strcat(date_string, hour_1);
                                to = to + strlen(hour_1);
                                break;

                        case 'm':
                                strcat(date_string, minute);
                                to = to + 2;
                                break;

                        case 'S':
                        case 's':
                                strcat(date_string, second);
                                to = to + 2;
                                break;

                        case '$':
                                date_string[to] = '$';
                                date_string[to+1] = 0;
                                to = to + 1;
                                break;
                        }

                        from = from + 2;
                } else {
                        date_string[to] = pattern[from];
                        date_string[to+1] = 0;
                        from = from + 1;
                        to = to + 1;
                }
        }
}

/*****************************************************************************|
|	Routine to decide if a string requires to be enclosed in quotation    |
|	marks, and if so, put them in.  The various bits in 'flags' tells     |
|	us what characters require protecting.  'Quote' contains the actual   |
|	quote character (which may require doubling or escaping with "\"      |
|	if it occurs in the string).  'Special' contains a list of characters |
|	which require protecting.                                             |
\*****************************************************************************/

static int varg_protect_string(char *instring, char *outstring,
                           int flags, char *special)
{
        int protection_needed, quote_present;
        int length, i, j, special_length;
        char ch, quote = '\"';
        char quote_string[1];

        quote_string[0] = quote;
        quote_string[1] = 0;
        length = strlen(instring);
        special_length = strlen(special);
        protection_needed = 0;
        quote_present = 0;

        /*	Check to see if we actually need the quotes.
        */

        if ((flags & VARGACT_M_PROTMASK) != 0) {
                for (i = 0; i <= length-1; i++) {
                        ch = instring[i];

                        if ((flags & VARGACT_M_UPPER) && isupper (ch)) {
                                protection_needed = 1;
                                continue;
                        }

                        if ((flags & VARGACT_M_LOWER) && islower (ch)) {
                                protection_needed = 1;
                                continue;
                        }

                        if ((flags & VARGACT_M_SPECIAL)) {
                                for (j = 0; j <= special_length-1; j++) {
                                        if (ch == special[j]) {
                                                protection_needed = 1;
                                                continue;
                                        }
                                }

                                if (protection_needed) continue;
                        }

                        if (((flags & VARGACT_M_ESCAPE) ||
                             (flags & VARGACT_M_DOUBLE)) &&
                            ch == quote) {
                                protection_needed = 1;
                                quote_present = 1;
                                continue;
                        }
                }
        }

        /*	if protection is not needed, just copy the input string to the
        	output string.
        */

        if (!protection_needed) {
                strcpy(outstring, instring);
                return 1;
        }

        /*	Protection is needed.  if no quotes are present, then we simply
        	enclose the string in quote marks, otherwise we have to step through
        	the string.
        */

        if (quote_present) {
                outstring[0] = quote;
                j = 1;

                for (i = 0; i <= length-1; i++) {
                        ch = instring[i];

                        if (ch == quote) {
                                if ((flags & VARGACT_M_ESCAPE)) {
                                        outstring[j] = '\\';
                                        j = j + 1;
                                }

                                if ((flags & VARGACT_M_DOUBLE)) {
                                        outstring[j] = quote;
                                        j = j + 1;
                                }
                        }

                        outstring[j] = ch;
                        j = j + 1;
                }

                outstring[j] = 0;
        } else {			/* just append the output string + quote */
                outstring[0] = 0;
                strcat(outstring, quote_string);
                strcat(outstring, instring);
                strcat(outstring, quote_string);
        }

        return 1;
}

/*****************************************************************************|
|	Routine to define any DCL symbols that are required, and if the       |
|	appropriate flag bit is set, exit to DCL using a LIB$DO_COMMAND call  |
|	which will activate the real application image.                       |
\*****************************************************************************/

static int varg_do_exit(int flags, char *arg_symbol,
                    char *arg_string, char *image_name, char *verb)
{
        char *foreign_image;
        char *command;
        int status;

        /*	Should we set up a symbol containing this arg string ?
        */

        if ((flags & VARGACT_M_SYMBOL))
                varg_set_symbol(arg_symbol, arg_string);

        /*	Should we define an image name in the verb symbol ?
        */

        if ((flags & VARGACT_M_IMAGE)) {
                foreign_image = malloc(strlen(image_name) + 2);
                if (foreign_image == NULL) return LIB$_INSVIRMEM;
                strcpy(foreign_image, "$");
                strcat(foreign_image, image_name);
                varg_set_symbol(verb, foreign_image);
        }

        /*	Should we exit the program and and activate another image or
        	command procedure ?
        */

        if ((flags & VARGACT_M_COMMAND)) {
                command = malloc(strlen(verb) + strlen(arg_string) + 2);
                if (command == NULL) return LIB$_INSVIRMEM;
                strcpy(command, verb);
                strcat(command, " ");
                strcat(command, arg_string);
                status = varg_do_command(command);
                return status;
        }

        return 1;
}
