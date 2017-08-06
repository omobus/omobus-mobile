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

#include <windows.h>
#include <string>
#include <omobus-mobile.h>

int WriteOmobusPlainText(const wchar_t *fname, const wchar_t *templ, 
  const wchar_t *buf)
{
  if( fname == NULL || templ == NULL || buf == NULL )
    return OMOBUS_ERR;
  bool fexist = wcsfexist(fname);
  pk_file_t f = pkopen(w2a(fname).c_str(), "ab");
  if( f == NULL )
    return 0;
  if( !fexist ) {
    std::wstring wh =
      L"# omobus-plain-text\r\n"
      L"# ts=";
    wh += 
      wsftime(omobus_time());
    wh += 
      L"\r\n";
    wh += templ;
    pkwrite(f, wh.c_str(), wh.size()*sizeof(wchar_t));
  }
  pkwrite(f, buf, wcslen(buf)*sizeof(wchar_t));
  pkclose(f);
  return OMOBUS_OK;
}
