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

#include "omcore.h"
#include <tchar.h>

//---------------------------------------------------------------------------------------------
/* Перевод строки содержащую дату-время в структуру tm */
struct tm *omcore_datetime2tm(const wchar_t *buf, struct tm *t)
{
  if( buf == NULL || t == NULL )
    return t;

  memset(t, 0, sizeof(struct tm));
  
  t->tm_year = _wtoi(&buf[0]); t->tm_year -= 1900;
  t->tm_mon  = _wtoi(&buf[5]); t->tm_mon--;
  t->tm_mday = _wtoi(&buf[8]);
  t->tm_hour = _wtoi(&buf[11]);
  t->tm_min  = _wtoi(&buf[14]);
  t->tm_sec  = _wtoi(&buf[17]);
  
  return t;
}

//---------------------------------------------------------------------------------------------
/* Перевод строки содержащую дату в структуру tm */
struct tm *omcore_date2tm(const wchar_t *buf, struct tm *t)
{
  if( buf == NULL || t == NULL )
    return t;

  memset(t, 0, sizeof(struct tm));

  t->tm_year = _wtoi(&buf[0]); t->tm_year -= 1900;
  t->tm_mon  = _wtoi(&buf[5]); t->tm_mon--;
  t->tm_mday = _wtoi(&buf[8]);
  
  return t;
}

//---------------------------------------------------------------------------------------------
/* Перевод строки содержащую дату-время в структуру tm */
struct tm *omcore_time2tm(const wchar_t *buf, struct tm *t)
{
  if( buf == NULL || t == NULL )
    return t;

  memset(t, 0, sizeof(struct tm));

  t->tm_hour = _wtoi(&buf[0]);
  t->tm_min  = _wtoi(&buf[3]);
  t->tm_sec  = _wtoi(&buf[6]);
  
  return t;
}

//---------------------------------------------------------------------------------------------
