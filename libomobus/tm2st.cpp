/* -*- C++ -*- */
/* This file is a part of the omobus-mobile project.
 * Copyright (c) 2006 - 2013 ak-obs, Ltd. <info@omobus.net>.
 * Author: Igor Artemov <i_artemov@ak-obs.ru>.
 *
 * This program is commerce software; you can't redistribute it and/or
 * modify it.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <windows.h>
#include <omobus-mobile.h>

// -----------------------------------------------------------------------------
// Перевод tm в SYSTEMTIME
SYSTEMTIME *tm2st(const struct tm *t, SYSTEMTIME *st)
{
  if( t == NULL || st == NULL )
    return st;
  memset(st, 0, sizeof(SYSTEMTIME));
  st->wDay = t->tm_mday;
  st->wDayOfWeek = t->tm_wday;
  st->wHour = t->tm_hour;
  st->wMilliseconds = 0;
  st->wMinute = t->tm_min;
  st->wMonth = t->tm_mon + 1;
  st->wSecond = t->tm_sec;
  st->wYear = t->tm_year + 1900;
  return st;
}

// -----------------------------------------------------------------------------
