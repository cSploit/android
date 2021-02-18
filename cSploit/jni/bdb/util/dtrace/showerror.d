#!/usr/sbin/dtrace -qs
/*
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * showerror.d - Capture context about certain DB errors.
 *
 * This shows the stack when a panic or a variant of DB->err() is called.
 * At the end of execution it displays an aggregated summary showing
 * which function invoked which of the traced error routines.
 *
 * The optional integer maxcount parameter directs the script to exit when
 * that many error messages have been displayed.
 *
 * usage: showerror.d { -p <pid> | -c "<program> [<args]" } [maxcount]
 */
#pragma D option defaultargs

self int code;
self uintptr_t message; self void *arg;

dtrace:::BEGIN
{
	errorcount = 0;
	maxcount = $1 != 0 ? $1 : -1;
	printf(
	    "Display DB stack when process %d prints an error message: max %d\n",
	    $target, maxcount);
}

/*
 * At the start of these error functions the error text might not be mapped in
 * yet. They save the argument upon entry and process them when returning, so
 * that the text of the error message can be more safely included in the trace.
 */

/* __db_err(ENV *, int errorcode, char *message, ...) */
pid$target::__db_err:entry
{
	self->code = arg1;
	self->message = (uintptr_t) arg2;
	self->arg = (void *) arg3;
}

pid$target::__db_err:return
{
	this->message = copyinstr(self->message);
	printf("DB error %d message \"%s\"(%p,...) from:\n",
	    self->code, this->message, self->arg);
	ustack();
	@errors[this->message, ustack(6)] = count();
	errorcount++;
}

/* __db_errx(ENV *, char *message, ...) */
pid$target::__db_errx:entry
{
	self->code = 0;
	self->message = (uintptr_t) arg1;
	self->arg = (void *) arg2;
}

pid$target::__db_errx:return
{
	this->message = copyinstr(self->message);
	printf("DB error message \"%s\"(%p,...) from:\n", this->message,
	    self->arg);
	ustack();
	@errors[this->message, ustack(4)] = count();
	errorcount++;
}

pid$target::__env_panic:entry
{
	printf("DB panic with error code %d from:\n", arg0);
	ustack();
	errorcount++;
}

pid$target::__db_err*:return,
pid$target::__env_panic:return
/errorcount == maxcount/
{
	exit(0);
}

dtrace:::END
{
	printa("Instances of the message \"%s\"", @errors);
}
