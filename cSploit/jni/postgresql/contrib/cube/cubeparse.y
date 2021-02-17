%{
/* contrib/cube/cubeparse.y */

/* NdBox = [(lowerleft),(upperright)] */
/* [(xLL(1)...xLL(N)),(xUR(1)...xUR(n))] */

#define YYSTYPE char *
#define YYDEBUG 1

#include "postgres.h"

#include "cubedata.h"

/*
 * Bison doesn't allocate anything that needs to live across parser calls,
 * so we can easily have it use palloc instead of malloc.  This prevents
 * memory leaks if we error out during parsing.  Note this only works with
 * bison >= 2.0.  However, in bison 1.875 the default is to use alloca()
 * if possible, so there's not really much problem anyhow, at least if
 * you're building with gcc.
 */
#define YYMALLOC palloc
#define YYFREE   pfree

extern int cube_yylex(void);

static char *scanbuf;
static int	scanbuflen;

extern int	cube_yyparse(NDBOX **result);
extern void cube_yyerror(NDBOX **result, const char *message);

static int delim_count(char *s, char delim);
static NDBOX * write_box(unsigned int dim, char *str1, char *str2);
static NDBOX * write_point_as_box(char *s, int dim);

%}

/* BISON Declarations */
%parse-param {NDBOX **result}
%expect 0
%name-prefix="cube_yy"

%token CUBEFLOAT O_PAREN C_PAREN O_BRACKET C_BRACKET COMMA
%start box

/* Grammar follows */
%%

box:
          O_BRACKET paren_list COMMA paren_list C_BRACKET {

	    int dim;

	    dim = delim_count($2, ',') + 1;
	    if ( (delim_count($4, ',') + 1) != dim ) {
          ereport(ERROR,
                  (errcode(ERRCODE_SYNTAX_ERROR),
                   errmsg("bad cube representation"),
                   errdetail("Different point dimensions in (%s) and (%s).",
                             $2, $4)));
	      YYABORT;
	    }
	    if (dim > CUBE_MAX_DIM) {
              ereport(ERROR,
                      (errcode(ERRCODE_SYNTAX_ERROR),
                       errmsg("bad cube representation"),
                       errdetail("A cube cannot have more than %d dimensions.",
								 CUBE_MAX_DIM)));
              YYABORT;
            }

	    *result = write_box( dim, $2, $4 );

          }
      |
          paren_list COMMA paren_list {
	    int dim;

	    dim = delim_count($1, ',') + 1;

	    if ( (delim_count($3, ',') + 1) != dim ) {
          ereport(ERROR,
                  (errcode(ERRCODE_SYNTAX_ERROR),
                   errmsg("bad cube representation"),
                   errdetail("Different point dimensions in (%s) and (%s).",
                             $1, $3)));
	      YYABORT;
	    }
	    if (dim > CUBE_MAX_DIM) {
              ereport(ERROR,
                      (errcode(ERRCODE_SYNTAX_ERROR),
                       errmsg("bad cube representation"),
                       errdetail("A cube cannot have more than %d dimensions.",
                                 CUBE_MAX_DIM)));
              YYABORT;
            }

	    *result = write_box( dim, $1, $3 );
          }
      |

          paren_list {
            int dim;

            dim = delim_count($1, ',') + 1;
	    if (dim > CUBE_MAX_DIM) {
              ereport(ERROR,
                      (errcode(ERRCODE_SYNTAX_ERROR),
                       errmsg("bad cube representation"),
                       errdetail("A cube cannot have more than %d dimensions.",
                                 CUBE_MAX_DIM)));
              YYABORT;
            }

	    *result = write_point_as_box($1, dim);
          }

      |

          list {
            int dim;

            dim = delim_count($1, ',') + 1;
	    if (dim > CUBE_MAX_DIM) {
              ereport(ERROR,
                      (errcode(ERRCODE_SYNTAX_ERROR),
                       errmsg("bad cube representation"),
                       errdetail("A cube cannot have more than %d dimensions.",
                                 CUBE_MAX_DIM)));
              YYABORT;
            }
	    *result = write_point_as_box($1, dim);
          }
      ;

paren_list:
          O_PAREN list C_PAREN {
             $$ = $2;
	  }
      ;

list:
          CUBEFLOAT {
			 /* alloc enough space to be sure whole list will fit */
             $$ = palloc(scanbuflen + 1);
			 strcpy($$, $1);
	  }
      |
	  list COMMA CUBEFLOAT {
             $$ = $1;
	     strcat($$, ",");
	     strcat($$, $3);
	  }
      ;

%%

static int
delim_count(char *s, char delim)
{
      int        ndelim = 0;

      while ((s = strchr(s, delim)) != NULL)
      {
        ndelim++;
        s++;
      }
      return (ndelim);
}

static NDBOX *
write_box(unsigned int dim, char *str1, char *str2)
{
  NDBOX * bp;
  char * s;
  int i;
  int size = offsetof(NDBOX, x[0]) + sizeof(double) * dim * 2;

  bp = palloc0(size);
  SET_VARSIZE(bp, size);
  bp->dim = dim;

  s = str1;
  bp->x[i=0] = strtod(s, NULL);
  while ((s = strchr(s, ',')) != NULL) {
    s++; i++;
    bp->x[i] = strtod(s, NULL);
  }

  s = str2;
  bp->x[i=dim] = strtod(s, NULL);
  while ((s = strchr(s, ',')) != NULL) {
    s++; i++;
    bp->x[i] = strtod(s, NULL);
  }

  return(bp);
}


static NDBOX *
write_point_as_box(char *str, int dim)
{
  NDBOX * bp;
  int i, size;
  double x;
  char * s = str;

  size = offsetof(NDBOX, x[0]) + sizeof(double) * dim * 2;

  bp = palloc0(size);
  SET_VARSIZE(bp, size);
  bp->dim = dim;

  i = 0;
  x = strtod(s, NULL);
  bp->x[0] = x;
  bp->x[dim] = x;
  while ((s = strchr(s, ',')) != NULL) {
    s++; i++;
    x = strtod(s, NULL);
    bp->x[i] = x;
    bp->x[i+dim] = x;
  }

  return(bp);
}

#include "cubescan.c"
