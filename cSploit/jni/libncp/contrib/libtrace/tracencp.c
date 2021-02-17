#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static void* freestack[256];
static void** RetAddrPtr = freestack;

static int infunc = 0;

static FILE* tracefile = NULL;

static void linesetup(void) {
	int x;
	
	x = RetAddrPtr - freestack;
	while (x--) {
		fprintf(tracefile, "  ");
	}
}

static void logprintf(void* calladdr, const char * message, ...) {
	va_list va;
	
	if (!tracefile) {
		char* x = getenv("NCP_TRACE_FILE");
		
		if (x) {
			tracefile = fopen(x, "wt");
		}
		if (!tracefile)
			tracefile = stderr;
	}
	if (infunc) {
		fprintf(tracefile, "...\n");
	}
	fprintf(tracefile, "%08lX: ", (unsigned long)calladdr);
	linesetup();
	va_start(va, message);
	vfprintf(tracefile, message, va);
	va_end(va);
	infunc = 1;
}

static void resprintf(const char * message, ...) {
	va_list va;
	
	if (!infunc) {
		fprintf(tracefile, "(cont'd.) ");
		linesetup();
		fprintf(tracefile, "... = ");
	}
	va_start(va, message);
	vfprintf(tracefile, message, va);
	va_end(va);
	infunc = 0;
}

#define DO_ASM_SYMBOL(SYMBOL)				\
asm(							\
".data						\n"	\
"	.align 4				\n"	\
"	.type	 .L99" #SYMBOL "_wrap,@object	\n"	\
"	.size	 .L99" #SYMBOL "_wrap,4		\n"	\
".L99" #SYMBOL "_wrap:				\n"	\
"	.long 0					\n"	\
".section	.rodata				\n"	\
".LC0" #SYMBOL ":				\n"	\
"	.string	\"" #SYMBOL "\"			\n"	\
".text						\n"	\
"	.align 4				\n"	\
".globl " #SYMBOL "				\n"	\
"	.type	 " #SYMBOL ",@function		\n"	\
#SYMBOL ":					\n"	\
"	movl $trace_in_" #SYMBOL ",%eax		\n"	\
"	testl %eax,%eax				\n"	\
"	jne .L8" #SYMBOL "			\n"	\
"	movl $trace_default_in,%eax		\n"	\
".L8" #SYMBOL ":				\n"	\
"	pushl $.LC0" #SYMBOL "			\n"	\
"	call *%eax				\n"	\
"	addl $4,%esp				\n"	\
".L3" #SYMBOL ":				\n"	\
"	cmpl $0,.L99" #SYMBOL "_wrap		\n"	\
"	jne .L4" #SYMBOL "			\n"	\
"	pushl $.LC0" #SYMBOL "			\n"	\
"	pushl $-1				\n"	\
"	call dlsym				\n"	\
"	addl $8,%esp				\n"	\
"	movl %eax,.L99" #SYMBOL "_wrap		\n"	\
"	testl %eax,%eax				\n"	\
"	jne .L4" #SYMBOL "			\n"	\
"	call __errno_location			\n"	\
"	movl $-38,(%eax)			\n"	\
"	movl $-1,%eax				\n"	\
"	jmp .L6" #SYMBOL "			\n"	\
"	.p2align 4,,7				\n"	\
".L4" #SYMBOL ":				\n"	\
"	movl RetAddrPtr,%eax			\n"	\
"	popl (%eax)				\n"	\
"	addl $4,RetAddrPtr			\n"	\
"	pushl $.L99" #SYMBOL "_continue		\n"	\
"	jmp *.L99" #SYMBOL "_wrap		\n"	\
".L99" #SYMBOL "_continue:			\n"	\
"	addl $-4,RetAddrPtr			\n"	\
"	movl RetAddrPtr,%ecx			\n"	\
"	pushl (%ecx)				\n"	\
"	movl $trace_out_" #SYMBOL ",%ecx	\n"	\
"	testl %ecx,%ecx				\n"	\
"	jne .L7" #SYMBOL "			\n"	\
"	movl $trace_default_out,%ecx		\n"	\
".L7" #SYMBOL ":				\n"	\
"	pushl %eax				\n"	\
"	call *%ecx				\n"	\
"	popl %eax				\n"	\
".L6" #SYMBOL ":				\n"	\
"	ret					\n"	\
".Lfe1" #SYMBOL ":				\n"	\
"	.size	 " #SYMBOL ",.Lfe1" #SYMBOL "-" #SYMBOL "\n"\
"	.weak	trace_out_" #SYMBOL "		\n"	\
"	.weak	trace_in_" #SYMBOL "		\n");

#define DO_ASM_SYMBOL_EASY(SYMBOL)			\
DO_ASM_SYMBOL(SYMBOL)

void trace_default_in(const char* funcname, void* calladdr) {
	logprintf(calladdr, "%s() = ", funcname);
}
void trace_default_out(int result) {
	resprintf("%d\n", result);
}

#include "allsyms.c"

void trace_out_strnwerror(const char* result) {
	resprintf("%s\n", result);
}
