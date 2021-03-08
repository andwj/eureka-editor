//------------------------------------------------------------------------
//  Debugging support
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2013 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __SYS_DEBUG_H__
#define __SYS_DEBUG_H__

#include <stdio.h>
#include "PrintfMacros.h"
#include <vector>

#define MSG_BUF_LEN  1024

class SString;

namespace global
{
    extern bool Quiet;
    extern bool Debugging;
    extern bool in_fatal_error;
}

void LogOpenFile(const char *filename);
void LogOpenWindow(void);
void LogClose(void);

void LogSaveTo(FILE *dest_fp);

void LogPrintf(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(1, 2);

void DebugPrintf(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(1, 2);

//
// Log controller
//
class Log
{
public:
	typedef void (*WindowAddCallback)(const SString &text, void *userData);

	bool openFile(const SString &filename);
	void openWindow();
	void close();
	void printf(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(2, 3);
	void debugPrintf(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(2, 3);
	void saveTo(FILE *dest_fp) const;

	//
	// Callback setter
	//
	void setWindowAddCallback(WindowAddCallback callback, void *userData)
	{
		windowAdd = callback;
		windowAddUserData = userData;
	}

	//
	// Mark the error
	//
	void markFatalError()
	{
		inFatalError = true;
	}
private:
	WindowAddCallback windowAdd = nullptr;
	void *windowAddUserData = nullptr;

	bool inFatalError = false;

	bool log_window_open = false;
	FILE *log_fp = nullptr;
	std::vector<SString> kept_messages;
};


// -------- assertion macros --------

#ifdef NDEBUG
#define SYS_ASSERT(cond)  ((void) 0)

#elif defined(__GNUC__)
#define SYS_ASSERT(cond)  ((cond) ? (void)0 :  \
        BugError("Assertion (%s) failed\nIn function %s (%s:%d)\n", #cond , __func__, __FILE__, __LINE__))

#else
#define SYS_ASSERT(cond)  ((cond) ? (void)0 :  \
        BugError("Assertion (%s) failed\nIn file %s:%d\n", #cond , __FILE__, __LINE__))

#endif  // NDEBUG

#define SYS_NULL_CHECK(ptr)    SYS_ASSERT((ptr) != NULL)
#define SYS_ZERO_CHECK(value)  SYS_ASSERT((value) != 0)

#endif /* __SYS_DEBUG_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
