/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

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