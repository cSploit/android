fb_shutdown(), fb_shutdown_callback() - new API call in Firebird 2.5.

   Implements smart shutdown of engine. Primarily used when working with embedded Firebird.

Author:
   Alex Peshkoff <peshkoff@mail.ru>

Syntax is:

   typedef int (*FB_SHUTDOWN_CALLBACK)(const int reason, const int mask, void* arg);

   int fb_shutdown(unsigned int timeout, 
                   const int reason);

   ISC_STATUS fb_shutdown_callback(ISC_STATUS* status_vector,
                                   FB_SHUTDOWN_CALLBACK callback_function,
                                   const int mask,
                                   void* arg);

Description:

fb_shutdown() performs smart shutdown of various Firebird subsystems (yValve, engine, redirector).
It DOES NOT perform shutdown of remote servers, to which you are currently attached - just 
terminates any Firebird activity in the current process. fb_shutdown() was primarily designed 
to be used by engine itself, but also can be used in user applications - for example, if you want
to close all opened handles at once, fb_shutdown() may be used for it. Normally it should not be 
used, because Firebird libraries (both kinds - embedded or pure client) do call it automatically
at exit(). To make fb_shutdown() be called at exit, you should attach at least one database (or 
service).

fb_shutdown() accepts 2 parameters - timeout in milliseconds and reason of shutdown. Engine uses
negative reason codes (they are listed in ibase.h, see constants starting with fb_shutrsn), if
you need to call fb_shutdown() from your program, you must use positive value for reason. This
value is passed to callback_function, passed as an argument to fb_shutdown_callback(), and you can
take appropriate actions when writing callback function.

Zero return value of fb_shutdown() means shutdown is successfull, non-zero means some errors took 
place during shutdown. You should consult firebird.log for more information.

fb_shutdown_callback() setups callback function, which will be called during shutdown. It has 4 
parameters - status vector, pointer to callback function, call mask and argumnet to be passed 
to callback function. Call mask can have the following values:
fb_shut_confirmation - callback function may return non-zero value to abort shutdown
fb_shut_preproviders - callback function will be called before shutting down engine
fb_shut_postproviders - callback function will be called after shutting down engine
fb_shut_finish - final step, callback function may wait for some activity to be terminated
or ORed combination of them (to make same function be called in required cases).

Callback function has 3 parameters - reason of shutdown, actual value of mask with which it was 
called and argument passed by user to fb_shutdown_callback(). There are 2 specially interesting 
shutdown reasons: 
fb_shutrsn_exit_called - Firebird is closing due to exit() or unloaded client/embedded library
fb_shutrsn_signal - signal SIGINT or SIGTERM was caught (posix only)
Second parameter (actual value of mask) helps to distinguish if callback was invoked before or after
engine shutdown. 
First and second parameters help you decide what action to be taken in your callback. Third can 
be used for any purporse you like and may be NULL.

Zero return value of callback function means it performed it's job OK, non-zero is interpreted
depending upon call mask. For fb_shut_confirmation non-zero means that shutdown will not be
performed. It's bad idea to return non-zero if shutdown is due to exit() called. In all other cases
it means some errors took place, and non-zero value will be returned from fb_shutdown(). It's 
callback function's responsibility to notify the world about exact reasons of error return.

fb_shutdown_callback() almost always returns successfully, though in some cases (out of memory
for example) it can return error.

Sample (it will make your program do not terminate on ctrl-C pressed after attaching databases):

#include <ibase.h>

// callback function for shutdown
static int ignoreCtrlC(const int reason, const int, void*)
{
	return reason == fb_shutrsn_signal ? 1 : 0;
}

int main(int argc, char *argv[])
{
	ISC_STATUS_ARRAY status;
	if (fb_shutdown_callback(status, ignoreCtrlC, fb_shut_confirmation, 0))
	{
		isc_print_status(status);
		return 1;
	}
	// your code continues ...
}
