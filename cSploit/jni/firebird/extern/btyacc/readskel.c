#include "defs.h"
#include <stdarg.h>

/* amount of memeory to allocate at once */
#define	CHUNK	8192

static char	*cp, *cp_end;
static char	**ap, **ap_end, **ap_start;

static void add_ptr(char *p)
{
    if (ap == ap_end) {
	unsigned int size = CHUNK;
	char **nap;
	while ((ap-ap_start) * sizeof(char *) >= size)
	    size = size * 2;
	if (!(nap = malloc(size)))
	    no_space();
	if (ap > ap_start)
	    memcpy(nap, ap_start, (ap-ap_start) * sizeof(char *));
	ap = nap + (ap - ap_start); 
	ap_start = nap;
	ap_end = nap + size/sizeof(char *); }
    *ap++ = p;
}

static void add_string(char *s)
{
int	len = strlen(s)+1;

    if (len > cp_end - cp) {
	int size = len > CHUNK ? len : CHUNK;
	if (!(cp = malloc(size)))
	    no_space();
	cp_end = cp + size; }
    memcpy(cp, s, len);
    add_ptr(cp);
    cp += len;
}

static void add_fmt(char *fmt, ...)
{
va_list	args;
char	buf[256];

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    add_string(buf);
}

static char **fin_section(void)
{
  char	**rv;

  add_ptr(0);
  rv = ap_start;
  ap_start = ap;
  return rv;
}

void read_skel(char *name)
{
char	buf[256];
int	section = -2;
int	line = 0, sline = 1, eline = 1;
int	i;
FILE	*fp;

    if (!(fp = fopen(name, "r")))
	open_error(name);
    while(fgets(buf, 255, fp)) {
	if ((sline = eline))
	    line++;
	if ((i = strlen(buf)) == 0)
	    continue;
	if (buf[i-1] == '\n') {
	    buf[--i] = 0;
	    eline = 1;
	} else {
	    buf[i++] = '\\';
	    buf[i] = 0;
	    eline = 0; 
	}
	if (sline && buf[0] == '%' && buf[1] == '%') {
	    char *p = buf+2;
	    if (section >= 0) {
	      section_list[section].ptr = fin_section();
	    }
	    section = -1;
	    while(*p && isspace(*p)) p++;
	    if (isalpha(*p)) {
	      char *e = p;
	      while(isalnum(*++e));
	      *e = 0;
	      for (i=0; section_list[i].name; i++)
		if (!strcmp(section_list[i].name, p))
		  section = i; 
	    }
	    if (section >= 0)
	      add_fmt("#line %d \"%s\"", line+1, name);
	    else if (*p)
	      error(0, buf, p, "line %d of \"%s\", bad section name",
		    line, name); 
        } else if (section >= 0) {
	    add_string(buf);
	}
    }
    if (section >= 0)
	section_list[section].ptr = fin_section();
    if (section == -2)
	error(0, 0, 0, "No sections found in skeleton file \"%s\"", name);
    fclose(fp);
}
