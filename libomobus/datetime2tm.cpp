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

#define _put_dt_v(v, b, l, p)\
  if( l > p ) \
    v = _wtoi(&b[p])

//--------------------------------------------------------------------------
// Перевод строки содержащую дату-время в структуру tm 
struct tm *datetime2tm(const wchar_t *buf, struct tm *t)
{
  if( buf == NULL || t == NULL )
    return t;

  memset(t, 0, sizeof(struct tm));
  int len = wcslen(buf);
  _put_dt_v(t->tm_year, buf, len, 0); 
  _put_dt_v(t->tm_mon,  buf, len, 5);
  _put_dt_v(t->tm_mday, buf, len, 8);
  _put_dt_v(t->tm_hour, buf, len, 11);
  _put_dt_v(t->tm_min,  buf, len, 14);
  _put_dt_v(t->tm_sec,  buf, len, 17);
  
  if( t->tm_mon > 0 )
    t->tm_mon--;
  if( t->tm_year > 0 )
    t->tm_year -= 1900;

  return t;
}

//--------------------------------------------------------------------------
