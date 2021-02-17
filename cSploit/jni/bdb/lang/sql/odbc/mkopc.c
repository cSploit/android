/*
 * Make opcodes.[ch] from standard input.
 */

#include <string.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    FILE *c_file, *h_file;
    char line[512];
    int count = 0;

    c_file = fopen("opcodes.c", "w");
    h_file = fopen("opcodes.h", "w");
    if (c_file == NULL || h_file == NULL) {
        fprintf(stderr, "unable to open output file(s)\n");
	exit(1);
    }
    fprintf(c_file, "/* Automatically generated file. Do not edit */\n");
    fprintf(c_file, "char *sqliteOpcodeNames[] = { \"???\",\n");
    fprintf(h_file, "/* Automatically generated file. Do not edit */\n");
    while (fgets(line, sizeof (line), stdin) != NULL) {
        if (strncmp(line, "case OP_", 7) == 0) {
	    char *p, *q;

	    p = line + 8;
	    q = strchr(p, ':');
	    if (q == NULL) {
	        continue;
	    }
	    *q = '\0';
	    fprintf(c_file, "  \"%s\",\n", p);
	    fprintf(h_file, "#define %-30s %3d\n", p - 3, ++count);
	}
    }
    fprintf(c_file, "};\n");
    fclose(h_file);
    fclose(c_file);
    return 0;
}
