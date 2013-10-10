
/* $Id: ec_parser.h,v 1.8 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_PARSER_H
#define EC_PARSER_H


EC_API_EXTERN void parse_options(int argc, char **argv);

EC_API_EXTERN int expand_token(char *s, u_int max, void (*func)(void *t, u_int n), void *t );
EC_API_EXTERN int set_regex(char *regex);


#endif

/* EOF */

// vim:ts=3:expandtab

