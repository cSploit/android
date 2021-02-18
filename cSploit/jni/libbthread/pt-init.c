#include <pthread.h>
#include <bthread.h>
#include <pt-internal.h>
#include <signal.h>

void pthread_cancel_handler(int signum) {
	pthread_exit(0);
}

void pthread_init(void) {
	struct sigaction sa;
	struct pthread_internal_t * p = (struct pthread_internal_t *)pthread_self();
	
	if(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_HANDLER)
		return;
	
	// set thread status as pthread_create should do.
	// ASYNCROUNOUS is not set, see pthread_setcancelstate(3)
	p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_HANDLER|PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	
	sa.sa_handler = pthread_cancel_handler;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	
	sigaction(SIGRTMIN, &sa, NULL);
}