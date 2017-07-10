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

#include <omobus-mobile.h>

int WriteOmobusJournal(const wchar_t *name, const wchar_t *templ, const wchar_t *buf)
{
  std::wstring fname = GetOmobusProfilePath(OMOBUS_PROFILE_CACHE);
  fname += name;
  return WriteOmobusPlainText(fname.c_str(), templ, buf);
}
