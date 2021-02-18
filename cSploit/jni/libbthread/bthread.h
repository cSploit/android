# define PTHREAD_CANCEL_ENABLE    	 0x00000010
# define PTHREAD_CANCEL_DISABLE   	 0x00000000

# define PTHREAD_CANCEL_ASYNCHRONOUS 0x00000020
# define PTHREAD_CANCEL_DEFERRED     0x00000000

#define PTHREAD_CANCELED ((void *) -1)

int pthread_setcancelstate (int , int *);
int pthread_setcanceltype (int , int *);
void pthread_testcancel (void);
int pthread_cancel (pthread_t t);