/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "core_types.h"

struct DateAndTime {
	u32	second;
	u32	minute;
	u32	hour;

	u32	day_of_month;
	u32	month;
	u32	year;
};

extern  void	date_and_time_get( DateAndTime* out_date_and_time );