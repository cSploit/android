%{
/* first section */
%}
%%
%{
/* second section */
%}
S : /* empty */	{ printf("S -> epsilon\n"); }
  | '(' S ')' S { printf("S -> ( S ) S\n"); }
%ifdef ABC
    /* see how preprocessor can be used */
  | '*'         { printf("S -> *\n"); }
%endif
  ;
%%
#include <stdio.h>

main() {
  printf("yyparse() = %d\n",yyparse());
}

yylex() {
  int ch;

	do { ch = getchar(); } while (ch == ' ' || ch == '\n' || ch == '\t');
	if (ch == EOF) return 0;
	return ch;
}

yyerror(s) char*s; {
  printf("%s\n",s);
}
