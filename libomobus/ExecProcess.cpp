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

DWORD ExecProcess(LPCWSTR pszImageName, LPCWSTR pszCmdLine, BOOL bWait) 
{
  DEBUGMSG(TRUE, (L"libomobus->ExecProcess: %s\n", pszCmdLine));
  DWORD eCode = -1;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(pi));
  if( ::CreateProcess(pszImageName, pszCmdLine, NULL, NULL, FALSE, 0, 
          NULL, NULL, NULL, &pi) ) {
    if( bWait ) {
      ::WaitForSingleObject(pi.hProcess, INFINITE);
      ::GetExitCodeProcess(pi.hProcess, &eCode);
    }
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
  } else {
    RETAILMSG(TRUE, (L"libomobus (ExecProcess): '%s %s' - FAILED\n",
      pszImageName, pszCmdLine));
  }
  return eCode;
}

DWORD ExecRootProcess(LPCWSTR pszName, LPCWSTR pszCmdLine, BOOL bWait) 
{
  std::wstring fn = GetOmobusRootPath(); fn += pszName;
  return ExecProcess(fn.c_str(), pszCmdLine, bWait);
}
