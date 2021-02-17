#include <stdio.h>
#include <string.h>
#include <errno.h>
//#ifdef HAVE_UNISTD_H
#include <unistd.h>
//#endif



void ProcessFile(FILE *in, bool stripFirstComment)
{
	char s[256];
	bool striping = false;
	while (fgets(s, sizeof(s), in))
	{
		if (stripFirstComment)
		{
			char *x = strstr(s, "/*");
			if (x)
			{
				striping = true;
				stripFirstComment = false;
				continue;
			}
		}
		if (striping)
		{
			char *x = strstr(s, "*/");
			if (x) {
				striping = false;
			}
			continue;
		}
		const char* include = "#include";
		if (memcmp(s, include, strlen(include)))
		{
			fputs(s, stdout);
			continue;
		}
		char *p = strchr(s, '<');
		if (p)
		{
			fputs(s, stdout);
			continue;
		}
		p = strchr(s, '"');
		if (! p) {
			throw "#include misses \" - start of filename";
		}
		++p;
		char *p2 = strchr(p, '"');
		if (! p2) {
			throw "#include misses \" - end of filename";
		}
		*p2 = 0;
		p2 = strrchr(p, '/');	// always open files in current directory
		if (p2) {
			p = p2 + 1;
		}
		FILE *newIn = fopen(p, "rt");
		if (! newIn) {
			continue;
		}
		ProcessFile(newIn, true);
		fclose(newIn);
		// clean after Makefile's cp
		unlink(p);
	}
}


int main()
{
	try {
		ProcessFile(stdin, false);
	}
	catch (const char* x)
	{
		fprintf(stderr, "%s\n", x);
		return 1;
	}
	return 0;
}
