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

#define YEAR0                   1900
#define EPOCH_YR                1970
#define SECS_DAY                (24L * 60L * 60L)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366 : 365)

const int _ytab[2][12] = 
{
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

//--------------------------------------------------------------------------
// Аналог ANSI функции gmtime_r
// http://www.jbox.dk/sanos/source/lib/time.c.html
struct tm* gmtime_r(const time_t *timer, struct tm *tmbuf)
{
  if( timer == NULL || tmbuf == NULL )
    return tmbuf;
  memset(tmbuf, 0, sizeof(tm));
  time_t time = *timer;
  unsigned long dayclock, dayno;
  int year = EPOCH_YR;

  dayclock = (unsigned long) time % SECS_DAY;
  dayno = (unsigned long) time / SECS_DAY;

  tmbuf->tm_sec = dayclock % 60;
  tmbuf->tm_min = (dayclock % 3600) / 60;
  tmbuf->tm_hour = dayclock / 3600;
  tmbuf->tm_wday = (dayno + 4) % 7; // Day 0 was a thursday
  while (dayno >= (unsigned long) YEARSIZE(year)) 
  {
    dayno -= YEARSIZE(year);
    year++;
  }
  tmbuf->tm_year = year - YEAR0;
  tmbuf->tm_yday = dayno;
  tmbuf->tm_mon = 0;
  while (dayno >= (unsigned long) _ytab[LEAPYEAR(year)][tmbuf->tm_mon]) 
  {
    dayno -= _ytab[LEAPYEAR(year)][tmbuf->tm_mon];
    tmbuf->tm_mon++;
  }
  tmbuf->tm_mday = dayno + 1;
  tmbuf->tm_isdst = 0;

  return tmbuf;
}

static struct tm st_tm;
struct tm* gmtime(const time_t* clock)
{
  return gmtime_r(clock, &st_tm);
}

//--------------------------------------------------------------------------
