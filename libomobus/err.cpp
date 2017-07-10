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
// Возвращает текстовое описание по коду ошибки
wchar_t* FormatSystemError(DWORD Err)
{
  wchar_t Msg[1024] = L"";
  DWORD dwSym = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
    NULL, Err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Msg, 1023, NULL);
  for( DWORD x = 0; x < dwSym; x++ ) {
    if( Msg[x] == L'\r' || Msg[x] == L'\n' || Msg[x] == L'\t' )
      Msg[x] = L' ';
  }
  return wcsdup(Msg);
}

// -----------------------------------------------------------------------------
