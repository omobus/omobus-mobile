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
// Перевод SYSTEMTIME в tm
struct tm *st2tm(const SYSTEMTIME *st, struct tm *t)
{
  if( t == NULL || st == NULL )
    return t;
  memset(t, 0, sizeof(struct tm));
  t->tm_mday = st->wDay;
  t->tm_wday = st->wDayOfWeek;
  t->tm_hour = st->wHour;
  t->tm_min = st->wMinute;
  t->tm_mon = st->wMonth - 1;
  t->tm_sec = st->wSecond;
  t->tm_year = st->wYear - 1900;
  return t;
}

// -----------------------------------------------------------------------------
