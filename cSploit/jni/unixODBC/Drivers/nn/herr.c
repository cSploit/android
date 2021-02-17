/**
    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>
	Enhanced for unixODBC (1999) by Peter Harvey <pharvey@codebydesign.com>
	
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/

#include <config.h>
#include	<nnconfig.h>

#include	<herr.h>
#include	"herr.ci"


typedef struct {
	int	code;	/* sqlstat(if msg == null) or native code */
	char*	msg;
} err_t;

# define	ERRSTACK_DEPTH	3

typedef struct {
	err_t	stack[ERRSTACK_DEPTH];
	int	top;	/* top free slot */
} err_stack_t;

void*	nnodbc_pusherr(void* stack, int code, char* msg)
{
	err_stack_t*	err_stack = stack;

	if( ! err_stack )
	{
		err_stack = (err_stack_t*)MEM_ALLOC( sizeof(err_stack_t));

		if( ! err_stack )
			return 0;

		err_stack->top = 0;
	}

	if( err_stack->top < ERRSTACK_DEPTH - 1)
		err_stack->top ++;

	(err_stack->stack)[err_stack->top - 1].code = code;
	(err_stack->stack)[err_stack->top - 1].msg  = msg;

	return err_stack;
}

int	nnodbc_errstkempty(void* stack)
{
	err_stack_t*	err_stack = stack;

	if( ! err_stack
	 || ! err_stack->top )
		return 1;

	return 0;
}

void	nnodbc_errstkunset(void* stack)
{
	if( stack )
		((err_stack_t*)stack)->top = 0;
}

void	nnodbc_poperr(void* stack )
{
	err_stack_t*	err_stack = stack;

	if( ! nnodbc_errstkempty( err_stack ) )
		err_stack->top --;

	return;
}

static	int	is_sqlerr(err_t* err)
{
	return !(err->msg);
}

int	nnodbc_getsqlstatcode(void* stack)
{
	err_stack_t*	err_stack = stack;
	err_t*		err;

	err = err_stack->stack + err_stack->top - 1;

	if( ! is_sqlerr(err) )
		return 0;

	return err->code;
}

char*	nnodbc_getsqlstatstr(void* stack)
{
	err_stack_t*	err_stack = stack;
	err_t*		err;
	int		i;

	err = err_stack->stack + err_stack->top - 1;

	if( ! is_sqlerr(err) )
		return 0;

	for(i=0;sqlerrmsg_tab[i].stat;i++)
	{
		if( sqlerrmsg_tab[i].code == err->code )
			return sqlerrmsg_tab[i].stat;
	}

	return 0;
}

char*	nnodbc_getsqlstatmsg(void* stack)
{
	err_stack_t*	err_stack = stack;
	err_t*		err;
	int		i;

	err = err_stack->stack + err_stack->top - 1;

	if( ! is_sqlerr(err) )
		return 0;

	for(i=0;sqlerrmsg_tab[i].stat;i++)
	{
		if( sqlerrmsg_tab[i].code == err->code )
			return sqlerrmsg_tab[i].msg;
	}

	return 0;
}

int	nnodbc_getnativcode(void* stack)
{
	err_stack_t*	err_stack = stack;
	err_t*		err;

	err = err_stack->stack + err_stack->top - 1;

	if( is_sqlerr(err) )
		return 0;

	return err->code;
}

char*	nnodbc_getnativemsg(void* stack)
{
	err_stack_t*	err_stack = stack;
	err_t*		err;

	err = err_stack->stack + err_stack->top - 1;

	return err->msg;
}

void*	nnodbc_clearerr(void* stack)
{
	MEM_FREE(stack);

	return 0;
}
