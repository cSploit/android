#include <pt-internal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_request_lock = PTHREAD_MUTEX_INITIALIZER;
int print_request,want_exit;

void print_pthread_internal(struct pthread_internal_t * p) {
	pthread_mutex_lock(&print_lock);
	
	printf("\n\n%s called by %p\n", __func__, (struct pthread_internal_t *)pthread_self());
	printf("p=%p\n",p);
	printf("p->next=%p\n", p->next);
	printf("p->prev=%p\n", p->prev);
	printf("p->tid=%d\n", p->tid);
	printf("p->attr.flags=%04x\n", p->attr.flags);
	printf("p->attr.stack_base=%p\n", p->attr.stack_base);
	printf("p->attr.stack_size=%d\n", p->attr.stack_size);
	printf("p->attr.guard_size=%d\n", p->attr.guard_size);
	printf("p->attr.sched_policy=%d\n", p->attr.sched_policy);
	printf("p->attr.sched_priority=%d\n", p->attr.sched_priority);
	printf("p->cleanup_stack=%p\n", p->cleanup_stack);
	printf("p->start_routine=%p\n", p->start_routine);
	printf("p->return_value=%p\n", p->return_value);
	printf("p->alternate_signal_stack=%p\n", p->alternate_signal_stack);
	
	pthread_mutex_unlock(&print_lock);
}


void *child(void *arg) {
	
	pthread_t id = pthread_self();
	struct pthread_internal_t * p = (struct pthread_internal_t *)id;
	int print,run;
	
	printf("child=%p\n", p);
	
	
	for(run=1;run;) {
		pthread_mutex_lock(&print_request_lock);
		if(print_request) {
			print_pthread_internal(p);
			print_request = 0;
		}
		run=!want_exit;
		pthread_mutex_unlock(&print_request_lock);
		
		usleep(100);
	}
}

void set_print(void) {
	pthread_mutex_lock(&print_request_lock);
	print_request = 1;
	pthread_mutex_unlock(&print_request_lock);
}

void wait_print(void) {
	int has_printed;
	do {
		usleep(100);
		pthread_mutex_lock(&print_request_lock);
		has_printed = (print_request==0);
		pthread_mutex_unlock(&print_request_lock);
	}while(!has_printed);
}

int main(int argc, char **argv) {
	
	pthread_t id,child_id;
	struct pthread_internal_t *p, *child_p;
	
	p=(struct pthread_internal_t *)(id=pthread_self());
	
	printf("main=%p\n", p);
	
	print_request = 1;
	want_exit = 0;
	
	pthread_create(&child_id, NULL, &child, NULL);
	
	child_p  = (struct pthread_internal_t *)child_id;
	
	wait_print();
	
	printf("setting cancel enable on %p\n", child_p);
	child_p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	print_pthread_internal(child_p);
	
	set_print();
	wait_print();
	
	printf("setting cancel async on %p\n", child_p);
	child_p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	print_pthread_internal(child_p);
	
	set_print();
	wait_print();
	
	printf("setting cancel pending on %p\n", child_p);
	child_p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_PENDING;
	print_pthread_internal(child_p);
	
	pthread_mutex_lock(&print_request_lock);
	print_request=want_exit=1;
	pthread_mutex_unlock(&print_request_lock);
	
	pthread_join(child_id, NULL); 
	
	return EXIT_SUCCESS;
}