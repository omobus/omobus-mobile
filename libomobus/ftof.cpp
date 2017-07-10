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
// Перекодирование double в double с учетом точности
double ftof(double val, int prec)
{
  wchar_t *s = ftowcs(val, prec, NULL);
  if( s == NULL )
    return val;
  double rv = wcstof(s);
  omobus_free(s);
  return rv;
}

// -----------------------------------------------------------------------------
