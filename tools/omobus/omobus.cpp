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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include <windows.h>
#include <omobus-mobile.h>
#include <WaitCursor.h>
#include <ensure_cleanup.h>

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, 
  int nCmdShow)
{
  CEnsureCloseHandle hSingle = CreateEvent(NULL, FALSE, FALSE,  
    L"FCE5507E-3DBB-4730-96CD-166642AB7752");
  if( hSingle == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    return 1;

  WaitCursor _wc(hInstance);

  if( !DaemonsHostStarted() )
    ExecRootProcess(L"daemons-host.exe", NULL, FALSE);

  if( !TerminalsHostStarted() )
    ExecRootProcess(L"terminals-host.exe", NULL, FALSE);
  else
    ActivateTerminalsHost();

  return 0;
}
