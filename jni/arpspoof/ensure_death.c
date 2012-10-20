/*    ensure_death.c provides a mechanism for killing a program when its' stdin recieves input
    Copyright (C) 2011 Robbie Clemons <robclemons@gmail.com> */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


void *blocking_input()
{
    char string[10];
    fgets(string, 8, stdin);
    raise(SIGINT);

} 

void ensure_death()
{
    pthread_t thread_id;
    pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, blocking_input, NULL);
}
