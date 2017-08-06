/* -*- C++ -*- */
/* This file is a part of the omobus-mobile project.
 * Copyright (c) 2006 - 2013 ak-obs, Ltd. <info@omobus.net>.
 * Author: Igor Artemov <i_artemov@ak-obs.ru>.
 *
 * This program is a free software. Redistribution and use in source and binary
 * forms, with or without modification, are permitted provided under the GPLv2 license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <windows.h>
#include <omobus-mobile.h>

//--------------------------------------------------------------------------
// Аналог ANSI функции localtime_r
struct tm* localtime_r(const time_t *timer, struct tm *tmbuf)
{
  if( timer == NULL || tmbuf == NULL )
    return tmbuf;

  TIME_ZONE_INFORMATION tzi;
  memset(&tzi, 0, sizeof(TIME_ZONE_INFORMATION));
	DWORD tziResult = GetTimeZoneInformation(&tzi);

  time_t t;
  if (tziResult == TIME_ZONE_ID_UNKNOWN)
		t = *timer - /*_timezone*/60*(tzi.Bias);
	else if (tziResult == TIME_ZONE_ID_STANDARD)
		t = *timer - /*_timezone*/60*(tzi.Bias + tzi.StandardBias);
	else if (tziResult == TIME_ZONE_ID_DAYLIGHT)
		t = *timer - /*_timezone*/60*(tzi.Bias + tzi.DaylightBias);
  gmtime_r(&t, tmbuf);

  if (tziResult == TIME_ZONE_ID_UNKNOWN)
		tmbuf->tm_isdst = -1;
	else if (tziResult == TIME_ZONE_ID_STANDARD)
		tmbuf->tm_isdst = 0;
	else if (tziResult == TIME_ZONE_ID_DAYLIGHT)
		tmbuf->tm_isdst = 1;

  return tmbuf;
}

//--------------------------------------------------------------------------
