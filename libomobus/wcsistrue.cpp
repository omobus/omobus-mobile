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

bool wcsistrue(const wchar_t *str)
{
  return str != NULL && (
    _tcsicmp(str, _T("1")) == 0 ||
    _tcsicmp(str, _T("yes")) == 0 || 
    _tcsicmp(str, _T("true")) == 0 ||
    _tcsicmp(str, _T("+")) == 0 || 
    _tcsicmp(str, _T("enabled")) == 0
    );
}
