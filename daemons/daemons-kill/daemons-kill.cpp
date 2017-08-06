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

// Программа работает только в режиме Unicode
#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include "resource.h"
#include <windows.h>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include <WaitCursor.h>

HINSTANCE _hInstance = NULL;
CEnsureFreeLibrary _hResInstance;

static 
void _mui_par(void *cookie, const wchar_t *par_name, const wchar_t *par_val) 
{
  if( wcscmp(par_name, OMOBUS_MUI_NAME) == 0 )
    _hResInstance = LoadMuiModule(_hInstance, par_val);
}

int MessageBoxEx(UINT uMsg, UINT uCap, UINT uType)
{
  TCHAR cap[25] = _T(""), msg[1024] = _T("");
  LoadString(_hResInstance, uCap, cap, 24);
  LoadString(_hResInstance, uMsg, msg, 1023);
  return MessageBox(GetForegroundWindow(), msg, cap, uType);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, 
  int nCmdShow)
{
  WaitCursor _wc(hInstance, false);
  _hInstance = hInstance;

  ParseOmobusConf(NULL, NULL, _mui_par);

  if( !DaemonsHostStarted() ) {
    MessageBoxEx(IDS_MSG0, IDS_ERROR, MB_ICONSTOP);
    return 1;
  }

  if( MessageBoxEx(IDS_MSG1, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDNO )
	  return 0;

  int step = 0;
  _wc.Set();
  do {
    StopDaemonsHost();
    Sleep(5*1000);
    step++;
  } while( step <= 10 && DaemonsHostStarted() );
  _wc.Restore();

  if( DaemonsHostStarted() )
    MessageBoxEx(IDS_MSG3, IDS_ERROR, MB_ICONWARNING);
  else
    MessageBoxEx(IDS_MSG2, IDS_INFORM, MB_ICONINFORMATION);

  return 0;
}
