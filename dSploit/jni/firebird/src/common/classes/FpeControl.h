/*
 *	PROGRAM:	FPE handling
 *	MODULE:		FpeControl.h
 *	DESCRIPTION:	handle state of math coprocessor when thread
 *					enters / leaves engine
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Alexander Peshkoff <peshkoff@mail.ru>,
 *					   Bill Oliver <Bill.Oliver@sas.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_FPE_CONTROL_H
#define CLASSES_FPE_CONTROL_H

#include <math.h>
#if defined(WIN_NT)
#include <float.h>
#else
#include <fenv.h>
#include <string.h>
#endif

#if defined(SOLARIS) && !defined(HAVE_FEGETENV)
// ok to remove this when Solaris 9 is no longer supported
#include <ieeefp.h>
#endif

namespace Firebird
{

// class to hold the state of the Floating Point Exception mask

// The firebird server *must* run with FP exceptions masked, as we may
// intentionally overflow and then check for infinity to raise an error.
// Most hosts run with FP exceptions masked by default, but we need
// to save the mask for cases where Firebird is used as an embedded
// database.
class FpeControl
{
public:
	// the constructor (1) saves the current floating point mask, and
	// (2) masks all floating point exceptions. Use is similar to
	// the ContextPoolHolder for memory allocation.

	// on modern systems, the default is to mask exceptions
	FpeControl() throw()
	{
		getCurrentMask(savedMask);
		if (!areExceptionsMasked(savedMask))
		{
			maskAll();
		}
	}

	~FpeControl() throw()
	{
		// change it back if necessary
		if (!areExceptionsMasked(savedMask))
		{
			restoreMask();
		}
	}

#if defined(WIN_NT)
	static void maskAll() throw()
	{
		_clearfp(); // always call _clearfp() before setting control word
		_controlfp(_CW_DEFAULT, _MCW_EM);
	}

private:
	typedef unsigned int Mask;
	Mask savedMask;

	static bool areExceptionsMasked(const Mask& m) throw()
	{
		return m == _CW_DEFAULT;
	}

	static void getCurrentMask(Mask& m) throw()
	{
		m = _controlfp(0, 0);
	}

	void restoreMask() throw()
	{
		_clearfp(); // always call _clearfp() before setting control word
		_controlfp(savedMask, _MCW_EM); // restore saved
	}

#elif defined(HAVE_FEGETENV) || defined ARM
	static void maskAll() throw()
	{
		fesetenv(FE_DFL_ENV);
	}

private:
	// Can't dereference FE_DFL_ENV, therefore need to have something to compare with
	class DefaultEnvironment
	{
	public:
		DefaultEnvironment()
		{
			fenv_t saved;
			fegetenv(&saved);
			fesetenv(FE_DFL_ENV);
			fegetenv(&clean);
			fesetenv(&saved);
		}

		fenv_t clean;
	};

	fenv_t savedMask;

	static bool areExceptionsMasked(const fenv_t& m) throw()
	{
		const static DefaultEnvironment defaultEnvironment;
		return memcmp(&defaultEnvironment.clean, &m, sizeof(fenv_t)) == 0;
	}

	static void getCurrentMask(fenv_t& m) throw()
	{
		fegetenv(&m);
	}

	void restoreMask() throw()
	{
		fesetenv(&savedMask);
	}
#elif defined(SOLARIS) && !defined(HAVE_FEGETENV)
	// ok to remove this when Solaris 9 is no longer supported
	// Solaris without fegetenv() implies Solaris 9 or older. In this case we
	// have to use the Solaris FPE routines.
	static void maskAll() throw()
	{
		fpsetmask(~(FP_X_OFL | FP_X_INV | FP_X_UFL | FP_X_DZ | FP_X_IMP));
	}

private:
	// default environment is all traps disabled, but there is no
	// constand for this setting
	class DefaultEnvironment
	{
	public:
		DefaultEnvironment()
		{
			fp_except saved;
			saved = fpgetmask();
			fpsetmask(~(FP_X_OFL | FP_X_INV | FP_X_UFL | FP_X_DZ | FP_X_IMP));
			clean = fpgetmask();
			fpsetmask(saved);
		}

		fp_except clean;
	};

	fp_except savedMask;

	static bool areExceptionsMasked(const fp_except& m) throw()
	{
		const static DefaultEnvironment defaultEnvironment;
		return memcmp(&defaultEnvironment.clean, &m, sizeof(fp_except)) == 0;
	}

	static void getCurrentMask(fp_except& m) throw()
	{
		m = fpgetmask();
	}

	void restoreMask() throw()
	{
		fpsetsticky(0); // clear exception sticky flags
		fpsetmask(savedMask);
	}

#else
#error do not know how to mask floating point exceptions on this platform!
#endif

};

} //namespace Firebird


// getting a portable isinf() is harder than you would expect
#ifdef WIN_NT
inline bool isinf(double x)
{
	return (!_finite (x) && !isnan(x));
}
#else
#ifndef isinf
template <typename F>
inline bool isinf(F x)
{
	return !isnan(x) && isnan(x - x);
}
#endif // isinf
#endif // WIN_NT

#endif //CLASSES_FPE_CONTROL_H
