#ifndef __GETOPT_H__
#define __GETOPT_H__

extern int optind;
extern int opterr;
extern int optopt;
extern char* optarg;

#define REQUIRE_ORDER	0
#define PERMUTE		1
#define RETURN_IN_ORDER	2

struct option {
	const char* name;
	int         has_arg;
	int*        flag;
	int         val;
	      };


int getopt(int argc, char ** argv, const char* optstring);
int getopt_long(int argc, char ** argv, const char* optstring, const struct option* longopts, int* longind);

#endif	/* __GETOPT_H__ */
