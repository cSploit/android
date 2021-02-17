/*
 * Simple string replacement utility
 *
 * $Id: fixup.c,v 1.2 2005/06/14 06:07:43 chw Exp chw $
 */

#include <string.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    int i;
    char line[512];

    if (argc < 2 || argc % 2 != 1) {
	fprintf(stderr, "usage: %s search1 replace1 ..\n", argv[0]);
	exit(1);
    }
    for (i = 1 + 1; i < argc; i += 2) {
	if (argv[i][0] == '@') {
	    int len, off = 1;
	    FILE *f;

	    if (argv[i][1] == '@') {
		off++;
	    }
	    f = fopen(argv[i] + off, "r");
	    if (f == NULL) {
		fprintf(stderr, "unable to read %s\n", argv[i] + off);
		exit(1);
	    }
	    line[0] = '\0';
	    fgets(line, sizeof (line), f);
	    fclose(f);
	    len = strlen(line);
	    if (len) {
		line[len - 1] = '\0';
	    }
	    if (off > 1) {
		int x = 0, y = 0, z = 0;

		sscanf(line, "%d.%d.%d", &x, &y, &z);
		sprintf(line, "%d", x * 100000 + y * 1000 + z);
	    }
	    argv[i] = strdup(line);
	}
    }
    while (fgets(line, sizeof (line), stdin) != NULL) {
	int found = 0;

	for (i = 1; i < argc; i += 2) {
	    char *p = strstr(line, argv[i]);

	    if (p != NULL) {
		fwrite(line, p - line, 1, stdout);
		fputs(argv[i + 1], stdout);
		p += strlen(argv[i]);
		fputs(p, stdout);
		found = 1;
		break;
	    }
	}
	if (!found) {
	    fputs(line, stdout);
	}
    }
    exit(0);
}
