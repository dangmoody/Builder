/*
===========================================================================

Core

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#ifdef _WIN64

#include <date_and_time.h>
#include <debug.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*
================================================================================================

	DateAndTime

================================================================================================
*/

void date_and_time_get( DateAndTime* out_date_and_time ) {
	assertf( out_date_and_time, "out_date_and_time MUST be non-NULL!" );

	SYSTEMTIME system_time = {};
	GetSystemTime( &system_time );

	out_date_and_time->second = system_time.wSecond;
	out_date_and_time->minute = system_time.wMinute;
	out_date_and_time->hour = system_time.wHour;

	out_date_and_time->day_of_month = system_time.wDay;
	out_date_and_time->month = system_time.wMonth;
	out_date_and_time->year = system_time.wYear;
}

#endif // _WIN64