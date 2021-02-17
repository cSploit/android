#include "defs.h"
#include <signal.h>
#include <stdio.h>

#if defined(WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1500
// ISO C++ conformant names
#define unlink _unlink
#define mktemp _mktemp
#define strdup _strdup
#endif

char dflag;
char lflag;
char rflag;
char tflag;
char vflag;
int Eflag = 0;

char *file_prefix = "y";
char *myname = "yacc";
#if defined(__MSDOS__) || defined(WIN32)
#define DIR_CHAR '\\'
#define DEFAULT_TMPDIR "."
#else  /* Unix */
#define DIR_CHAR '/'
#define DEFAULT_TMPDIR "/tmp"
#endif
char *temp_form = "yacc_t_XXXXXX";

int lineno;
int outline;

char *action_file_name;
char *code_file_name;
char *defines_file_name;
char *input_file_name = "";
char *output_file_name;
char *text_file_name;
char *union_file_name;
char *verbose_file_name;

FILE *action_file;	/*  a temp file, used to save actions associated    */
			/*  with rules until the parser is written	    */
FILE *code_file;	/*  y.code.c (used when the -r option is specified) */
FILE *defines_file;	/*  y.tab.h					    */
FILE *input_file;	/*  the input file				    */
FILE *output_file;	/*  y.tab.c					    */
FILE *text_file;	/*  a temp file, used to save text until all	    */
			/*  symbols have been defined			    */
FILE *union_file;	/*  a temp file, used to save the union		    */
			/*  definition until all symbol have been	    */
			/*  defined					    */
FILE *verbose_file;	/*  y.output					    */

int nitems;
int nrules;
int nsyms;
int ntokens;
int nvars;

int   start_symbol;
char  **symbol_name;
Yshort *symbol_value;
Yshort *symbol_prec;
char  *symbol_assoc;

Yshort *ritem;
Yshort *rlhs;
Yshort *rrhs;
Yshort *rprec;
char  *rassoc;
Yshort **derives;
char *nullable;


void done(int k)
{
	if (action_file) 
	{ 
		fclose(action_file); 
		unlink(action_file_name); 
	}
	if (text_file) 
	{ 
		fclose(text_file); 
		unlink(text_file_name); 
	}
	if (union_file) 
	{ 
		fclose(union_file); 
		unlink(union_file_name); 
	}
	exit(k);
}


void onintr(int sig)
{
	done(1);
}


void set_signals()
{
#ifdef SIGINT
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onintr);
#endif
#ifdef SIGTERM
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, onintr);
#endif
#ifdef SIGHUP
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onintr);
#endif
}


void usage()
{
	fprintf(stderr, "usage: %s [-dlrtv] [-b file_prefix] [-S skeleton file] "
		    "filename\n", myname);
	exit(1);
}


void no_more_options(int i, int argc, char* argv[])
{
	if (i + 1 != argc) 
		usage();

	input_file_name = argv[i];

	if (!file_prefix) 
	{
		if (input_file_name) 
		{
			char* s;
			file_prefix = strdup(input_file_name);
			if ((s = strrchr(file_prefix, '.')))
				*s = 0; 
		} 
		else {
			file_prefix = "y"; 
		}
	}
}


void getargs(int argc, char **argv)
{
	register int i;
	register char *s;

	if (argc > 0) 
		myname = argv[0];

	for (i = 1; i < argc; ++i)
	{
		s = argv[i];
		if (*s != '-') 
			break;

		switch (*++s)
		{
		case '\0':
			input_file = stdin;
			if (i + 1 < argc) 
				usage();
			return;

		case '-':
			++i;
			no_more_options(i, argc, argv);
			return;

		case 'b':
			if (*++s)
				file_prefix = s;
			else if (++i < argc)
				file_prefix = argv[i];
			else
				usage();
			continue;

		case 'd':
			dflag = 1;
			break;

		case 'D':
			/* Find the preprocessor variable */
			{
				char **ps;
				char *var_name = s + 1;
				extern char *defd_vars[];
				for (ps = &defd_vars[0]; *ps; ps++) 
				{
					if (strcmp(*ps, var_name) == 0) {
						error(lineno, 0, 0, "Preprocessor variable %s already defined", var_name);
					}
				}
				*ps = MALLOC(strlen(var_name)+1);
				strcpy(*ps, var_name);
				*++ps = NULL;
			}
			continue;
			  
		case 'E':
			Eflag = 1;
			break;

		case 'l':
			lflag = 1;
			break;

		case 'r':
			rflag = 1;
			break;

		case 't':
			tflag = 1;
			break;

		case 'v':
			vflag = 1;
			break;

		case 'S':
			if (*++s)
				read_skel(s);
			else if (++i < argc)
				read_skel(argv[i]);
			else
				usage();
			continue;

		default:
			usage();
		}

		for (;*++s;)
		{
			switch (*s)
			{
			//case '\0':
			//goto end_of_option;

			case 'd':
				dflag = 1;
				break;

			case 'l':
				lflag = 1;
				break;

			case 'r':
				rflag = 1;
				break;

			case 't':
				tflag = 1;
				break;

			case 'v':
				vflag = 1;
				break;

			default:
				usage();
			}
		}
		//end_of_option:
	}

	no_more_options(i, argc, argv);
}


char *allocate(unsigned n)
{
    register char *p = NULL;
    if (n)
    {
        /* VM: add a few bytes here, cause 
         * Linux calloc does not like sizes like 32768 */
		p = CALLOC(1, n + 10);
		if (!p) 
			no_space();
    }
    return (p);
}


void create_file_names()
{
	int i, len;

	char* tmpdir = getenv("TMPDIR");
	if (tmpdir == 0) 
		tmpdir = DEFAULT_TMPDIR;

	len = strlen(tmpdir);
	i = len + strlen(temp_form) + 1;
	if (len && tmpdir[len-1] != DIR_CHAR)
		++i;

	action_file_name = MALLOC(i);
	if (action_file_name == 0) 
		no_space();
	text_file_name = MALLOC(i);
	if (text_file_name == 0) 
		no_space();
	union_file_name = MALLOC(i);
	if (union_file_name == 0) 
		no_space();

	strcpy(action_file_name, tmpdir);
	strcpy(text_file_name, tmpdir);
	strcpy(union_file_name, tmpdir);

	if (len && tmpdir[len - 1] != DIR_CHAR)
	{
		action_file_name[len] = DIR_CHAR;
		text_file_name[len] = DIR_CHAR;
		union_file_name[len] = DIR_CHAR;
		++len;
	}

	strcpy(action_file_name + len, temp_form);
	strcpy(text_file_name + len, temp_form);
	strcpy(union_file_name + len, temp_form);

	action_file_name[len + 5] = 'a';
	text_file_name[len + 5] = 't';
	union_file_name[len + 5] = 'u';

	if(mktemp(action_file_name) == NULL) {
		fprintf(stderr, "btyacc: Cannot create temporary file\n");
		exit(1);
	}
	if(mktemp(text_file_name) == NULL) {
		fprintf(stderr, "btyacc: Cannot create temporary file\n");
		exit(1);
	}
	if(mktemp(union_file_name) == NULL) {
		fprintf(stderr, "btyacc: Cannot create temporary file\n");
		exit(1);
	}

	len = strlen(file_prefix);

	output_file_name = MALLOC(len + 7);
	if (output_file_name == 0)
		no_space();
	strcpy(output_file_name, file_prefix);
	strcpy(output_file_name + len, OUTPUT_SUFFIX);

	if (rflag)
	{
		code_file_name = MALLOC(len + 8);
		if (code_file_name == 0)
			no_space();
		strcpy(code_file_name, file_prefix);
		strcpy(code_file_name + len, CODE_SUFFIX);
	}
	else
		code_file_name = output_file_name;

	if (dflag)
	{
		defines_file_name = MALLOC(len + 7);
		if (defines_file_name == 0)
			no_space();
		strcpy(defines_file_name, file_prefix);
		strcpy(defines_file_name + len, DEFINES_SUFFIX);
	}

	if (vflag)
	{
		verbose_file_name = MALLOC(len + 8);
		if (verbose_file_name == 0)
			no_space();
		strcpy(verbose_file_name, file_prefix);
		strcpy(verbose_file_name + len, VERBOSE_SUFFIX);
	}
}


void open_files()
{
	create_file_names();

	if (input_file == 0)
	{
		input_file = fopen(input_file_name, "r");
		if (input_file == 0)
			open_error(input_file_name);
	}

	action_file = fopen(action_file_name, "w");
		if (action_file == 0)
			open_error(action_file_name);

	text_file = fopen(text_file_name, "w");
	if (text_file == 0)
		open_error(text_file_name);

	if (vflag)
	{
		verbose_file = fopen(verbose_file_name, "w");
		if (verbose_file == 0)
			open_error(verbose_file_name);
	}

	if (dflag)
	{
		defines_file = fopen(defines_file_name, "w");
		if (defines_file == 0)
			open_error(defines_file_name);
		union_file = fopen(union_file_name, "w");
		if (union_file ==  0)
			open_error(union_file_name);
	}

	output_file = fopen(output_file_name, "w");
	if (output_file == 0)
		open_error(output_file_name);

	if (rflag)
	{
		code_file = fopen(code_file_name, "w");
		if (code_file == 0)
			open_error(code_file_name);
	}
	else
		code_file = output_file;
}


int main(int argc, char **argv)
{
	set_signals();
	getargs(argc, argv);
	open_files();
	reader();
	lr0();
	lalr();
	make_parser();
	verbose();
	output();
	done(0);
	return 0;
}
