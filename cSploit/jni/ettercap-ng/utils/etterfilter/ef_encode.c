/*
    etterfilter -- the actual compiler

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ef_encode.c,v 1.21 2004/06/25 14:24:30 alor Exp $
*/

#include <ef.h>
#include <ef_functions.h>
#include <ec_filter.h>

#include <ctype.h>

#include <regex.h>
#ifdef HAVE_PCRE
   #include <pcre.h>
#endif

/* protos */

int encode_offset(char *string, struct filter_op *fop);
int encode_function(char *string, struct filter_op *fop);
int encode_const(char *string, struct filter_op *fop);
static char ** decode_args(char *args, int *nargs);
static char * strsep_quotes(char **stringp, const char delim);

/*******************************************/

/*
 * search an offset and fill the filter_op structure
 * return ESUCCESS on error.
 */
int encode_offset(char *string, struct filter_op *fop)
{
   char *str, *p, *q, *tok;
   int ret;

   memset(fop, 0, sizeof(struct filter_op));
   
   /* make the modifications on a copy */
   str = strdup(string);
   
   /*
    * the offset contains at least one '.'
    * we are sure because the syntax parser
    * will not have passed it here if it is not
    * in the right form.
    */
   p = ec_strtok(str, ".", &tok);
   q = ec_strtok(NULL, ".", &tok);

   /* the the virtual pointer from the table */
   ret = get_virtualpointer(p, q, &fop->op.test.level, &fop->op.test.offset, &fop->op.test.size);

   SAFE_FREE(str);

   return ret;
}

/*
 * assing the value of the const to the fop.value
 *
 * all the value are integer32 and are saved in host order
 */
int encode_const(char *string, struct filter_op *fop)
{
   char *p;
   
   memset(fop, 0, sizeof(struct filter_op));
   
   /* it is an hexadecimal value */
   if (!strncmp(string, "0x", 2) && isdigit((int)string[2])) {
      fop->op.test.value = strtoul(string, NULL, 16);
      return ESUCCESS;
      
   /* it is an integer value */
   } else if (isdigit((int)string[0])) {
      fop->op.test.value = strtoul(string, NULL, 10);
      return ESUCCESS;
      
   /* it is an ip address */
   } else if (string[0] == '\'' && string[strlen(string) - 1] == '\'') {
      struct in_addr ipaddr;
      
      /* remove the single quote */
      p = strchr(string + 1, '\'');
      *p = '\0';

      if (inet_aton(string + 1, &ipaddr) == 0)
         return -EFATAL;
      
      fop->op.test.value = ntohl(ipaddr.s_addr);
      return ESUCCESS;
      
   /* it is a string */
   } else if (string[0] == '\"' && string[strlen(string) - 1] == '\"') {
  
      /* remove the quotes */
      p = strchr(string + 1, '\"');
      *p = '\0';

      /* copy the string */
      fop->op.test.string = strdup(string + 1);
         
      /* escape it in the structure */
      fop->op.test.slen = strescape(fop->op.test.string, fop->op.test.string);
     
      return ESUCCESS;
      
   /* it is a constant */
   } else if (isalpha((int)string[0])) {
      return get_constant(string, &fop->op.test.value);
   }
   
   /* anything else is an error */
   return -ENOTFOUND;
}


/*
 * parse a function and its arguments and fill the structure
 */
int encode_function(char *string, struct filter_op *fop)
{
   char *str = strdup(string);
   int ret = -ENOTFOUND;
   char *name, *args;
   int nargs = 0, i;
   char **dec_args = NULL;
   char *tok;

   memset(fop, 0, sizeof(struct filter_op));
   
   /* get the name of the function */
   name = ec_strtok(string, "(", &tok);
   /* get all the args */
   args = name + strlen(name) + 1;

   /* analyze the arguments */
   dec_args = decode_args(args, &nargs);

   /* this fop is a function */
   fop->opcode = FOP_FUNC;

   /* check if it is a known function */
   if (!strcmp(name, "search")) {
      if (nargs == 2) {
         /* get the level (DATA or DECODED) */
         if (encode_offset(dec_args[0], fop) == ESUCCESS) {
            /* encode offset wipe the fop !! */
            fop->opcode = FOP_FUNC;
            fop->op.func.op = FFUNC_SEARCH;
            fop->op.func.string = strdup(dec_args[1]);
            fop->op.func.slen = strescape(fop->op.func.string, fop->op.func.string);
            ret = ESUCCESS;
         }
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "regex")) {
      if (nargs == 2) {
         int err;
         regex_t regex;
         char errbuf[100];
         
         /* get the level (DATA or DECODED) */
         if (encode_offset(dec_args[0], fop) == ESUCCESS) {
            /* encode offset wipe the fop !! */
            fop->opcode = FOP_FUNC;
            fop->op.func.op = FFUNC_REGEX;
            fop->op.func.string = strdup(dec_args[1]);
            fop->op.func.slen = strescape(fop->op.func.string, fop->op.func.string);
            ret = ESUCCESS;
         }

         /* check if the regex is valid */
         err = regcomp(&regex, fop->op.func.string, REG_EXTENDED | REG_NOSUB | REG_ICASE );
         if (err) {
            regerror(err, &regex, errbuf, sizeof(errbuf));
            SCRIPT_ERROR("%s", errbuf);
         } 
         
         regfree(&regex);
                        
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "pcre_regex")) {
#ifndef HAVE_PCRE
      WARNING("The script contains pcre_regex, but you don't have support for it.");
#else
      pcre *pregex;
      const char *errbuf = NULL;
      int erroff;
      
      if (nargs == 2) {
                     
         /* get the level (DATA or DECODED) */
         if (encode_offset(dec_args[0], fop) == ESUCCESS) {
            /* encode offset wipe the fop !! */
            fop->opcode = FOP_FUNC;
            fop->op.func.op = FFUNC_PCRE;
            fop->op.func.string = strdup(dec_args[1]);
            fop->op.func.slen = strlen(fop->op.func.string);
            ret = ESUCCESS;
         }

         /* check if the pcre is valid */
         pregex = pcre_compile(fop->op.func.string, 0, &errbuf, &erroff, NULL );
         if (pregex == NULL)
            SCRIPT_ERROR("%s\n", errbuf);

         pcre_free(pregex);
      } else if (nargs == 3) {
            
         fop->opcode = FOP_FUNC;
         fop->op.func.op = FFUNC_PCRE;
         /* substitution always at layer DATA */
         fop->op.func.level = 5;
         fop->op.func.string = strdup(dec_args[1]);
         fop->op.func.slen = strlen(fop->op.func.string);
         fop->op.func.replace = strdup(dec_args[2]);
         fop->op.func.rlen = strlen(fop->op.func.replace);
         ret = ESUCCESS;
         
         /* check if the pcre is valid */
         pregex = pcre_compile(fop->op.func.string, 0, &errbuf, &erroff, NULL );
         if (pregex == NULL)
            SCRIPT_ERROR("%s\n", errbuf);

         pcre_free(pregex);
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
#endif
   } else if (!strcmp(name, "replace")) {
      if (nargs == 2) {
         fop->op.func.op = FFUNC_REPLACE;
         /* replace always operate at DATA level */
         fop->op.func.level = 5;
         fop->op.func.string = strdup(dec_args[0]);
         fop->op.func.slen = strescape(fop->op.func.string, fop->op.func.string);
         fop->op.func.replace = strdup(dec_args[1]);
         fop->op.func.rlen = strescape(fop->op.func.replace, fop->op.func.replace);
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "inject")) {
      if (nargs == 1) {
         fop->op.func.op = FFUNC_INJECT;
         /* inject always operate at DATA level */
         fop->op.func.level = 5;
         fop->op.func.string = strdup(dec_args[0]);
         fop->op.func.slen = strlen(fop->op.func.string);
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "log")) {
      if (nargs == 2) {
         /* get the level (DATA or DECODED) */
         if (encode_offset(dec_args[0], fop) == ESUCCESS) {
            /* encode offset wipe the fop !! */
            fop->opcode = FOP_FUNC;
            fop->op.func.op = FFUNC_LOG;
            fop->op.func.string = strdup(dec_args[1]);
            fop->op.func.slen = strlen(fop->op.func.string);
            ret = ESUCCESS;
         }
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "drop")) {
      if (nargs == 0) {
         fop->op.func.op = FFUNC_DROP;
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "kill")) {
      if (nargs == 0) {
         fop->op.func.op = FFUNC_KILL;
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "msg")) {
      if (nargs == 1) {
         fop->op.func.op = FFUNC_MSG;
         fop->op.func.string = strdup(dec_args[0]);
         fop->op.func.slen = strescape(fop->op.func.string, fop->op.func.string);
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "exec")) {
      if (nargs == 1) {
         fop->op.func.op = FFUNC_EXEC;
         fop->op.func.string = strdup(dec_args[0]);
         fop->op.func.slen = strlen(fop->op.func.string);
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   } else if (!strcmp(name, "exit")) {
      if (nargs == 0) {
         fop->opcode = FOP_EXIT;
         ret = ESUCCESS;
      } else
         SCRIPT_ERROR("Wrong number of arguments for function \"%s\" ", name);
   }

   /* free the array */
   for (i = 0; i < nargs; i++)
      SAFE_FREE(dec_args[i]);
      
   SAFE_FREE(dec_args);
   SAFE_FREE(str);
   return ret;
}

/*
 * split the args of a function and return
 * the number of found args
 */
static char ** decode_args(char *args, int *nargs)
{
   char *p, *q, *arg;
   int i = 0;
   char **parsed;

   *nargs = 0;
  
   /* get the end */
   if ((p = strrchr(args, ')')) != NULL)
      *p = '\0';
   
   /* trim the empty spaces */
   for (; *args == ' '; args++);
   for (q = args + strlen(args) - 1; *q == ' '; q--)
      *q = '\0';

   /* there are no arguments */
   if (!strchr(args, ',') && strlen(args) == 0)
      return NULL;
  
   SAFE_CALLOC(parsed, 1, sizeof(char *));
   
   /* split the arguments */
   for (p = strsep_quotes(&args, ','), i = 1; p != NULL; p = strsep_quotes(&args, ','), i++) {
      
      /* alloc the array for the arguments */
      SAFE_REALLOC(parsed, (i + 1) * sizeof(char *));
      
      /* trim the empty spaces */
      for (arg = p; *arg == ' '; arg++);
      for (q = arg + strlen(arg) - 1; *q == ' '; q--)
         *q = '\0';
    
      /* remove the quotes (if there are) */
      if (*arg == '\"' && arg[strlen(arg) - 1] == '\"') {      
         arg[strlen(arg) - 1] = '\0';
         arg++;
      }
      /* put in in the array */
      parsed[i - 1] = strdup(arg);
      
      ef_debug(5, "ARGUMENT: %s\n", arg);
   }

   /* return the number of args */
   *nargs = i - 1;
   
   return parsed;
}



/*
 * split the string in tokens separated by 'delim'.
 * ignore 'delim' if it is between two quotes "..."
 */
static char * strsep_quotes(char **stringp, const char delim)
{
	char *s;
	int c;
	char *tok;

   /* sanity check */
	if ((s = *stringp) == NULL)
		return (NULL);

   /* parse the string */
	for (tok = s;;) {

      /* XXX - 
       * this does not parses correctly string in the form:
       *
       *  "foo, bar, "tic,tac""
       */

      /* skip string between quotes */
      if (*s == '\"')
         while(*(++s) != '\"' && *s != '\0');

      c = *s++;
     
      /* search for the delimiter */
      if ( c == delim || c == 0) {
         if (c == 0)
            s = NULL;
         else
            s[-1] = 0;
         
         *stringp = s;
         
         return (tok);
      }
	}
	/* NOTREACHED */
}

/* EOF */

// vim:ts=3:expandtab

