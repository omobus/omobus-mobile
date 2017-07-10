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
// Перекодирование double в строку с учётом точности
wchar_t *ftowcs(double val, int prec, const wchar_t *thsep)
{
  wchar_t buf[256] = L"", buf2[256] = L"";
  _snwprintf(buf, 255, L"%f", val);
  NUMBERFMTW nfmt = {0};
  nfmt.lpDecimalSep = L".";
  nfmt.lpThousandSep = thsep == NULL ? L"" : thsep;
  nfmt.NumDigits = prec;
  nfmt.LeadingZero = 1;
  nfmt.NegativeOrder = 1;
  nfmt.Grouping = 3;
  GetNumberFormatW(LOCALE_USER_DEFAULT, 0, buf, &nfmt, buf2, 254);
  return wcsdup(buf2);
}

// -----------------------------------------------------------------------------
