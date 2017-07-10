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
// Поиск подстроки без учёта регистра
const wchar_t *wcsstri(const wchar_t *str1, const wchar_t *str2)
{
  wchar_t *buf1 = _wcslwr(_wcsdup(str1)), 
    *buf2 = _wcslwr(_wcsdup(str2)),
    *result = wcsstr(buf1, buf2);
  result = result == NULL ? NULL : (wchar_t*)str1 + (result-buf1);
  free(buf1); free(buf2);
  return result;
}

// -----------------------------------------------------------------------------
