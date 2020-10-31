//------------------------------------------------------------------------
//  UTILITIES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>

#include "m_strings.h"
#include "w_rawdef.h"


//
// a case-insensitive strcmp()
//
int y_stricmp(const char *s1, const char *s2)
{
	for (;;)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(*s1) - (int)(unsigned char)(*s2);

		if (*s1 && *s2)
		{
			s1++;
			s2++;
			continue;
		}

		// both *s1 and *s2 must be zero
		return 0;
	}
}


//
// a case-insensitive strncmp()
//
int y_strnicmp(const char *s1, const char *s2, size_t len)
{
	SYS_ASSERT(len != 0);

	while (len-- > 0)
	{
		if (tolower(*s1) != tolower(*s2))
			return (int)(unsigned char)(*s1) - (int)(unsigned char)(*s2);

		if (*s1 && *s2)
		{
			s1++;
			s2++;
			continue;
		}

		// both *s1 and *s2 must be zero
		return 0;
	}

	return 0;
}


//
// upper-case a string (in situ)
//
void y_strupr(char *str)
{
	for ( ; *str ; str++)
	{
		*str = toupper(*str);
	}
}


//
// lower-case a string (in situ)
//
void y_strlowr(char *str)
{
	for ( ; *str ; str++)
	{
		*str = tolower(*str);
	}
}


char *StringNew(int length)
{
	// length does not include the trailing NUL.

	char *s = (char *) calloc(length + 1, 1);

	if (! s)
		FatalError("Out of memory (%d bytes for string)\n", length);

	return s;
}


char *StringDup(const char *orig, int limit)
{
	if (! orig)
		return NULL;

	if (limit < 0)
	{
		char *s = strdup(orig);

		if (! s)
			FatalError("Out of memory (copy string)\n");

		return s;
	}

	char * s = StringNew(limit+1);
	strncpy(s, orig, limit);
	s[limit] = 0;

	return s;
}


SString StringUpper(const SString &name)
{
	SString copy(name);
	for(char &c : copy)
		c = toupper(c);
	return copy;
}


SString StringLower(const SString &name)
{
	SString copy(name);
	for(char &c : copy)
		c = tolower(c);
	return copy;
}

//
// Non-leaking version
//
SString StringPrintf(EUR_FORMAT_STRING(const char *str), ...)
{
	// Algorithm: keep doubling the allocated buffer size
	// until the output fits. Based on code by Darren Salt.

	char *buf = NULL;
	int buf_size = 128;

	for (;;)
	{
		va_list args;
		int out_len;

		buf_size *= 2;

		buf = (char*)realloc(buf, buf_size);
		if (!buf)
			FatalError("Out of memory (formatting string)\n");

		va_start(args, str);
		out_len = vsnprintf(buf, buf_size, str, args);
		va_end(args);

		// old versions of vsnprintf() simply return -1 when
		// the output doesn't fit.
		if (out_len < 0 || out_len >= buf_size)
			continue;

		SString result(buf);
		free(buf);
		return result;
	}
}
SString StringVPrintf(const char *str, va_list ap)
{
	// Algorithm: keep doubling the allocated buffer size
	// until the output fits. Based on code by Darren Salt.

	char *buf = NULL;
	int buf_size = 128;

	for (;;)
	{
		int out_len;

		buf_size *= 2;

		buf = (char*)realloc(buf, buf_size);
		if (!buf)
			FatalError("Out of memory (formatting string)\n");

		out_len = vsnprintf(buf, buf_size, str, ap);

		// old versions of vsnprintf() simply return -1 when
		// the output doesn't fit.
		if (out_len < 0 || out_len >= buf_size)
			continue;

		SString result(buf);
		free(buf);
		return result;
	}

}

//
// Safe, cross-platform version of strncpy
//
void StringCopy(char *buffer, size_t size, const SString &source)
{
	if(!size)
		return;	// trivial
	strncpy(buffer, source.c_str(), size);
	buffer[size - 1] = 0;
}

void StringRemoveCRLF(char *str)
{
	size_t len = strlen(str);

	if (len > 0 && str[len - 1] == '\n')
		str[--len] = 0;

	if (len > 0 && str[len - 1] == '\r')
		str[--len] = 0;
}

//
// Removes the endline character or sequence from the end
//
void StringRemoveCRLF(SString &string)
{
	if(string && string.back() == '\n')
		string.pop_back();
	if(string && string.back() == '\r')
		string.pop_back();
}

//
// Removes the endline character or sequence from the end
//
void SString::removeCRLF()
{
	if(*this && back() == '\n')
		pop_back();
	if(*this && back() == '\r')
		pop_back();
}

//
// Cuts a string at position "pos", removing the respective character too
//
void SString::getCutWithSpace(size_t pos, SString *word0, SString *word1) const
{
	SYS_ASSERT(pos < length());
	if(word0)
		*word0 = SString(data.substr(0, pos));
	if(word1)
		*word1 = SString(data.substr(pos + 1));
}

//
// Trim leading spaces
//
void SString::trimLeadingSpaces()
{
	size_t i = 0;
	for(i = 0; i < length(); ++i)
		if(!isspace(data[i]))
			break;
	if(i)
		erase(0, i);
}

SString StringTidy(const SString &str, const SString &bad_chars)
{
	SString buf;
	buf.reserve(str.length() + 2);

	for(const char &c : str)
		if(isprint(c) && bad_chars.find(c) == std::string::npos)
			buf.push_back(c);

	return buf;
}


void TimeDelay(unsigned int millies)
{
	SYS_ASSERT(millies < 300000);

#ifdef WIN32
	::Sleep(millies);

#else // LINUX or MacOSX

	usleep(millies * 1000);
#endif
}


unsigned int TimeGetMillies()
{
	// Note: you *MUST* handle overflow (it *WILL* happen)

#ifdef WIN32
	return GetTickCount();
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	return ((int)tv.tv_sec * 1000 + (int)tv.tv_usec / 1000);
#endif
}


//
// sanity checks for the sizes and properties of certain types.
// useful when porting.
//

#define assert_size(type,size)            \
  do                  \
  {                 \
    if (sizeof (type) != size)            \
      FatalError("sizeof " #type " is %d (should be " #size ")\n",  \
  (int) sizeof (type));           \
  }                 \
  while (0)

#define assert_wrap(type,high,low)          \
  do                  \
  {                 \
    type n = high;              \
    if (++n != low)             \
      FatalError("Type " #type " wraps around to %lu (should be " #low ")\n",\
  (unsigned long) n);           \
  }                 \
  while (0)


void CheckTypeSizes()
{
	assert_size(u8_t,  1);
	assert_size(s8_t,  1);
	assert_size(u16_t, 2);
	assert_size(s16_t, 2);
	assert_size(u32_t, 4);
	assert_size(s32_t, 4);

	assert_size(raw_linedef_t, 14);
	assert_size(raw_sector_s,  26);
	assert_size(raw_sidedef_t, 30);
	assert_size(raw_thing_t,   10);
	assert_size(raw_vertex_t,   4);
}


//
// translate (dx, dy) into an integer angle value (0-65535)
//
unsigned int ComputeAngle(int dx, int dy)
{
	return (unsigned int) (atan2 ((double) dy, (double) dx) * 10430.37835 + 0.5);
}



//
// compute the distance from (0, 0) to (dx, dy)
//
unsigned int ComputeDist(int dx, int dy)
{
	return (unsigned int) (hypot ((double) dx, (double) dy) + 0.5);
}


double PerpDist(double x, double y,
                double x1, double y1, double x2, double y2)
{
	x  -= x1; y  -= y1;
	x2 -= x1; y2 -= y1;

	double len = sqrt(x2*x2 + y2*y2);

	SYS_ASSERT(len > 0);

	return (x * y2 - y * x2) / len;
}


double AlongDist(double x, double y,
                 double x1, double y1, double x2, double y2)
{
	x  -= x1; y  -= y1;
	x2 -= x1; y2 -= y1;

	double len = sqrt(x2*x2 + y2*y2);

	SYS_ASSERT(len > 0);

	return (x * x2 + y * y2) / len;
}


const char *Int_TmpStr(int value)
{
	static char buffer[200];

	snprintf(buffer, sizeof(buffer), "%d", value);

	return buffer;
}


//
// rounds the value _up_ to the nearest power of two.
//
int RoundPOW2(int x)
{
	if (x <= 2)
		return x;

	x--;

	for (int tmp = x >> 1 ; tmp ; tmp >>= 1)
		x |= tmp;

	return x + 1;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
