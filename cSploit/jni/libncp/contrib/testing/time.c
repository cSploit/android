#include <time.h>
#include <stdio.h>
#include "private/libintl.h"

time_t clk;

static void dotimes(void) {
	struct tm* tm;
	char x[1000];
	char src[1000];
	int p;
	
	puts(ctime(&clk));
	tm = localtime(&clk);
	for (p = 'A'; p <= 'z'; p++) {
		if (p == '[')
			p = 'a';
		if (p == 'E' || p == 'O')
			continue;
		sprintf(src, "%c=%%%c E%c=%%E%c O%c=%%O%c", p, p, p, p, p, p);
		strftime(x, sizeof(x), src, tm);
		puts(x);
	}
}

int main(void) {
	time(&clk);
	dotimes();
	setlocale(LC_ALL, "");
	dotimes();
	return 0;
}
