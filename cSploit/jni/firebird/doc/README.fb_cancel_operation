fb_cancel_operation() - new API call in firebird 2.5.

   Implements capability to asynchronously cancel long running API call.

Author:
   Alex Peshkoff <peshkoff@mail.ru>

Syntax is:

   ISC_STATUS fb_cancel_operation(ISC_STATUS* status_vector,
                                  isc_db_handle* db_handle,
                                  ISC_USHORT option);

Description:

Action performed by this function depends upon value of last parameter, option. Option can be:
   - fb_cancel_raise: cancels any activity, related with db_handle. This means that firebird 
will as soon as possible (to be precise, at the nearest rescheduling point, except some special 
states of engine) try to stop running request and error-return to the user.
   - fb_cancel_disable: disable execution of fb_cancel_raise requests for given attachment. It's
useful when your program executes some critical things (for example, cleanup).
   - fb_cancel_enable: enables previously disabled cancel delivery.
   - fb_cancel_abort: forcible close client side of connection. Useful if you need to close 
connection urgently. All active transactions will be rolled back by server. Success always
returned to the application. Use with care ! 

fb_cancel_disable / fb_cancel_enable may be set many times - if engine is already in requested
state, no action is taken. Cancel is enabled by default (i.e. after attach/create database).

Pay attention to asynchronous nature of this API call. Usually fb_cancel_raise is called when 
you need to stop long-running request. This means that it's called from separate thread (calling
it from signal handler is bad idea, it's NOT async signal safe). Another side of asynchronous 
execution is that at the end of API call attachment's activity may be already cancelled, may be not
(and the second is very possible). It also means that returned status vector will almost always be
FB_SUCCESS, though some errors are possible (like error sending network packet to remote server).

Sample:

Thread A:
fb_cancel_operation(isc_status, &DB, fb_cancel_enable);
isc_dsql_execute_immediate(isc_status, &DB, &TR, 0, "long running statement", 3, NULL);
// waits for API call to finish...

		Thread B:
		fb_cancel_operation(local_status, &DB, fb_cancel_raise);

Thread A:
if (isc_status[1])
	isc_print_status(isc_status); // will print "operation was cancelled"
