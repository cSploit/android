#include "getopt.h"
#include <stdio.h>
#include <string.h>

int optind=1;
int opterr=1;
int optopt='?';
char* optarg=NULL;

int getopt_long(int argc, char **argv, const char* optstring,
    const struct option *longopts, int* longind) {
  int c;
  const char* ctrl;
  static const char* inlist;
  int mode;

  switch (*optstring) {
    case '+':mode=REQUIRE_ORDER; optstring++; break;
    case '-':mode=RETURN_IN_ORDER; optstring++; break;
    default: mode=PERMUTE; break;
  }
  optarg=NULL;
  if (inlist) {
    c=*inlist++;
  } else {
Q2:;
    if (argc<=optind) return -1;
    inlist=argv[optind];
    if (!inlist) return -1;
    c=*inlist++;
    if ((c!='-') && (c!='/')) {
      goto Q1;
    }
    c=*inlist++;
    if (!c) {
Q1:;
      if (mode == RETURN_IN_ORDER) {
	optarg=argv[optind++];
	inlist=NULL;
	return 1;
      } else if (mode == PERMUTE) {
	int myoptind = optind+1;

	while (myoptind < argc) {
	  inlist=argv[myoptind];
	  if (!inlist) return -1;
	  c=*inlist++;
	  if ((c=='-') || (c=='/')) {
	    c=*inlist++;
	    if (c) {
	      inlist=argv[myoptind];
	      memmove(argv+optind+1, argv+optind, sizeof(char*)*(myoptind-optind));
	      argv[optind]=inlist;
	      inlist=NULL;
	      goto Q2;
	    }
	  }
	  myoptind++;
	}
      }
      inlist=NULL;
      return -1;
    }
    if (c=='-') {
      c=*inlist;
      if (!c) {
	optind++;
	inlist=NULL;
	return -1;
      }
      if (longopts) {
	const char* q;
	int ln;
	int i;

	optind++;

	q=strchr(inlist, '=');
	if (q) {
	  ln=q-inlist;
	  q++;
	} else {
	  ln=strlen(inlist);
	}
	for (i=0; longopts[i].name; i++) {
	  int ln2;

	  ln2=strlen(longopts[i].name);
	  if (ln==ln2) if (!memcmp(inlist, longopts[i].name, ln2)) {
	    if (longind) *longind=i;
	    switch (longopts[i].has_arg) {
	      case 0: if (q) {
			if (opterr)
			  fprintf(stderr, "%s: option `--%s' doesn't allow an argument\n", argv[0], longopts[i].name);
			inlist=NULL;
			return '?';
		      }
		      break;
	      case 2: break;
	      default:if (!q) {
			if (optind>=argc) {
			  if (opterr)
			    fprintf(stderr, "%s: option `--%s' requires an argument\n", argv[0], longopts[i].name);
			  inlist=NULL;
			  return ':';
			}
			q=argv[optind++];
		      }
		      break;
	    }
	    optarg=q;
	    c=longopts[i].val;
	    if (longopts[i].flag) {*longopts[i].flag=c; c=0; }
	    inlist=NULL;
	    return c;
	  }
	}
	if (opterr)
	  fprintf(stderr, "%s: unrecognized option `%s'\n", argv[0], inlist);
	inlist=NULL;
	return '?';
      }
    }
  }
  if (c==':') {
    ctrl=(optstring[0]==':')?optstring:NULL;
  } else {
    ctrl=strchr(optstring, c);
  }
  if (!*inlist) {
    inlist=NULL;
    optind++;
  }
  if (!ctrl) {
    if (opterr) {
      fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
    }
    optopt=c;
    return '?';
  }
  if (ctrl[1]==':') {
    if (!inlist) {
      if ((argc<=optind) || !(inlist=argv[optind])) {
	if (opterr) {
	  fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], c);
	}
	optopt=c;
	return ':';
      }
    }
    optarg=inlist;
    optind++;
    inlist=NULL;
  }
  return c;
}

int getopt(int argc, char ** argv, const char* optstring) {
  return getopt_long(argc, argv, optstring, NULL, NULL);
}
