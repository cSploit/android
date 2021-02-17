#ifndef _string_h_
#define _string_h_

struct mstring {
    char	*base, *ptr, *end;
    };

void msprintf(struct mstring *, const char *, ...);
int mputchar(struct mstring *, int);
struct mstring *msnew(void);
char *msdone(struct mstring *);

/* compare two strings, ignoring whitespace, except between two letters or
** digits (and treat all of these as equal) */
int strnscmp(const char *, const char *);
/* hash a string, ignoring whitespace */
unsigned int strnshash(const char *);

#define mputc(m, ch)	((m)->ptr==(m)->end?mputchar(m,ch):(*(m)->ptr++=(ch)))

#endif /* _string_h_ */
