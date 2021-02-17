#include "firebird.h"
#include "firebird/Provider.h"
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "gen/iberror.h"
#include "../common/classes/alloc.h"
#include "../common/classes/init.h"
#include "../common/classes/array.h"
#include "../common/thd.h"
#include "../common/utils_proto.h"
#include "../common/StatusHolder.h"

namespace Firebird {

// Before using thr parameter, make sure that thread is not going to work with
// this functions itself.
// CVC: Do not let "perm" be incremented before "trans", because it may lead to serious memory errors,
// since several places in our code blindly pass the same vector twice.
void makePermanentVector(ISC_STATUS* perm, const ISC_STATUS* trans, ThreadId thr) throw()
{
	try
	{
		while (true)
		{
			const ISC_STATUS type = *perm++ = *trans++;

			switch (type)
			{
			case isc_arg_end:
				return;
			case isc_arg_cstring:
				{
					perm [-1] = isc_arg_string;
					const size_t len = *trans++;
					const char* temp = reinterpret_cast<char*>(*trans++);
					*perm++ = (ISC_STATUS)(IPTR) (MasterInterfacePtr()->circularAlloc(temp, len, (intptr_t) thr));
				}
				break;
			case isc_arg_string:
			case isc_arg_interpreted:
			case isc_arg_sql_state:
				{
					const char* temp = reinterpret_cast<char*>(*trans++);
					const size_t len = strlen(temp);
					*perm++ = (ISC_STATUS)(IPTR) (MasterInterfacePtr()->circularAlloc(temp, len, (intptr_t) thr));
				}
				break;
			default:
				*perm++ = *trans++;
				break;
			}
		}
	}
	catch (const system_call_failed& ex)
	{
		memcpy(perm, ex.value(), sizeof(ISC_STATUS_ARRAY));
	}
	catch (const BadAlloc&)
	{
		*perm++ = isc_arg_gds;
		*perm++ = isc_virmemexh;
		*perm++ = isc_arg_end;
	}
	catch (...)
	{
		*perm++ = isc_arg_gds;
		*perm++ = isc_random;
		*perm++ = isc_arg_string;
		*perm++ = (ISC_STATUS)(IPTR) "Unexpected exception in makePermanentVector()";
		*perm++ = isc_arg_end;
	}
}

void makePermanentVector(ISC_STATUS* v, ThreadId thr) throw()
{
	makePermanentVector(v, v, thr);
}

// ********************************* Exception *******************************

Exception::~Exception() throw() { }

ISC_STATUS Exception::stuff_exception(ISC_STATUS* const status_vector) const throw()
{
	LocalStatus status;
	stuffException(&status);
	const ISC_STATUS* s = status.get();
	fb_utils::copyStatus(status_vector, ISC_STATUS_LENGTH, s, fb_utils::statusLength(s));

	return status_vector[1];
}

// ********************************* status_exception *******************************

status_exception::status_exception() throw()
{
	memset(m_status_vector, 0, sizeof(m_status_vector));
}

status_exception::status_exception(const ISC_STATUS *status_vector) throw()
{
	memset(m_status_vector, 0, sizeof(m_status_vector));

	if (status_vector)
	{
		set_status(status_vector);
	}
}

void status_exception::set_status(const ISC_STATUS *new_vector) throw()
{
	fb_assert(new_vector != 0);

	makePermanentVector(m_status_vector, new_vector);
}

status_exception::~status_exception() throw()
{
}

const char* status_exception::what() const throw()
{
	return "Firebird::status_exception";
}

void status_exception::raise(const ISC_STATUS *status_vector)
{
	throw status_exception(status_vector);
}

void status_exception::raise(const Arg::StatusVector& statusVector)
{
	throw status_exception(statusVector.value());
}

ISC_STATUS status_exception::stuffException(IStatus* status) const throw()
{
	if (status)
	{
		status->set(value());
	}

	return value()[1];
}

// ********************************* BadAlloc ****************************

void BadAlloc::raise()
{
	throw BadAlloc();
}

ISC_STATUS BadAlloc::stuffException(IStatus* status) const throw()
{
	const ISC_STATUS sv[] = {isc_arg_gds, isc_virmemexh};

	if (status)
	{
		status->set(FB_NELEM(sv), sv);
	}

	return sv[1];
}

const char* BadAlloc::what() const throw()
{
	return "Firebird::BadAlloc";
}

// ********************************* LongJump ***************************

void LongJump::raise()
{
	throw LongJump();
}

ISC_STATUS LongJump::stuffException(IStatus* status) const throw()
{
	ISC_STATUS sv[] = {isc_arg_gds, isc_random, isc_arg_string, (ISC_STATUS)(IPTR) "Unexpected Firebird::LongJump"};

	if (status)
	{
		status->set(FB_NELEM(sv), sv);
	}

	return sv[1];
}

const char* LongJump::what() const throw()
{
	return "Firebird::LongJump";
}


// ********************************* system_error ***************************

system_error::system_error(const char* syscall, int error_code) :
	status_exception(), errorCode(error_code)
{
	Arg::Gds temp(isc_sys_request);
	temp << Arg::Str(syscall);
	temp << SYS_ERR(errorCode);
	set_status(temp.value());
}

void system_error::raise(const char* syscall, int error_code)
{
	throw system_error(syscall, error_code);
}

void system_error::raise(const char* syscall)
{
	throw system_error(syscall, getSystemError());
}

int system_error::getSystemError()
{
#ifdef WIN_NT
	return GetLastError();
#else
	return errno;
#endif
}

// ********************************* system_call_failed ***************************

system_call_failed::system_call_failed(const char* syscall, int error_code) :
	system_error(syscall, error_code)
{
	// NS: something unexpected has happened. Log the error to log file
	// In the future we may consider terminating the process even in PROD_BUILD
	gds__log("Operating system call %s failed. Error code %d", syscall, error_code);
#ifdef DEV_BUILD
	// raised failed system call exception in DEV_BUILD in 99.99% means
	// problems with the code - let's create memory dump now
	abort();
#endif
}

void system_call_failed::raise(const char* syscall, int error_code)
{
	throw system_call_failed(syscall, error_code);
}

void system_call_failed::raise(const char* syscall)
{
	throw system_call_failed(syscall, getSystemError());
}

// ********************************* fatal_exception *******************************

fatal_exception::fatal_exception(const char* message) :
	status_exception()
{
	const ISC_STATUS temp[] =
	{
		isc_arg_gds,
		isc_random,
		isc_arg_string,
		(ISC_STATUS)(IPTR) message,
		isc_arg_end
	};
	set_status(temp);
}

// Keep in sync with the constructor above, please; "message" becomes 4th element
// after initialization of status vector in constructor.
const char* fatal_exception::what() const throw()
{
	return reinterpret_cast<const char*>(value()[3]);
}

void fatal_exception::raise(const char* message)
{
	throw fatal_exception(message);
}

void fatal_exception::raiseFmt(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	VSNPRINTF(buffer, sizeof(buffer), format, args);
	buffer[sizeof(buffer) - 1] = 0;
	va_end(args);
	throw fatal_exception(buffer);
}

}	// namespace Firebird
