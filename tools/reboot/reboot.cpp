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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include <windows.h>
#include <aygshell.h>
#include <omobus-mobile.h>
#include <WaitCursor.h>

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, 
  LPTSTR lpstrCmdLine, int nCmdShow)
{
  int step = 0;
  WaitCursor _wc(hInstance);
  do {
    StopDaemonsHost();
    StopTerminalsHost();
    Sleep(5*1000);
    step++;
  } while( step <= 10 && TerminalsHostStarted() && DaemonsHostStarted() );
  ExitWindowsEx(EWX_REBOOT, 0);  
  return 0;
}
