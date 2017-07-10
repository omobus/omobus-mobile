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

#include "resource.h"
#include <windows.h>
#include <cfgmgrapi.h>
#include <omobus-mobile.h>
#include <argcargv.h>
#include <string>

HINSTANCE _hInstance = NULL;
CEnsureFreeLibrary _hResInstance;

inline
std::wstring &_readfile(const wchar_t *fname, std::wstring &out) {
  out.clear();
  FILE *f = _wfopen(fname, _T("rb"));
  if( f == NULL )
    return out;
  wchar_t buf[1024+1];
  while( !feof(f) ) {
    memset(buf, 0, sizeof(buf));
    size_t len = fread(buf, 2, 1024, f);
    buf[len] = L'\0';
	  out += buf;
  }
  fclose(f);
  return out;
}

inline
std::wstring _readfile(const wchar_t *fname) {
  std::wstring out; 
  return _readfile(fname, out);
}

static
HRESULT ProcessXMLConfig(const wchar_t *strConfig)
{
  int step = 0;
  LPWSTR wszOutput = NULL;
  HRESULT hr = S_OK;
  do {
    hr = DMProcessConfigXML(strConfig, CFGFLAG_PROCESS, &wszOutput);
	  delete [] wszOutput;
    if( hr == S_OK || hr == CONFIG_E_BAD_XML ) {
      break;
    }
    Sleep(1200);
    step++;
  } while(step < 5);
  if( hr != S_OK ) {
    RETAILMSG(TRUE, (L"ProcessXMLConfig: FAILED (0x%08X)\n", hr));
  }
	return hr;
}

static 
void _mui_par(void *cookie, const wchar_t *par_name, const wchar_t *par_val) 
{
  if( wcscmp(par_name, OMOBUS_MUI_NAME) == 0 )
    _hResInstance = LoadMuiModule(_hInstance, par_val);
}

static
int MessageBoxEx(UINT uMsg, UINT uCap, UINT uType)
{
  TCHAR cap[25] = _T(""), msg[1024] = _T("");
  LoadString(_hResInstance, uCap, cap, 24);
  LoadString(_hResInstance, uMsg, msg, 1023);
  return MessageBox(GetForegroundWindow(), msg, cap, uType);
}

static
UINT getError(HRESULT hr)
{
  UINT res = IDS_CONFIG_UNKNOWN;
  switch( hr ) {
  case CONFIG_E_OBJECTBUSY:
    res = IDS_CONFIG_E_OBJECTBUSY;
    break;
  case CONFIG_E_CANCELTIMEOUT:
    res = IDS_CONFIG_E_CANCELTIMEOUT;
    break;
  case CONFIG_E_ENTRYNOTFOUND:
    res = IDS_CONFIG_E_ENTRYNOTFOUND;
    break;
  case CONFIG_S_PROCESSINGCANCELED:
    res = IDS_CONFIG_S_PROCESSINGCANCELED;
    break;
  case CONFIG_E_CSPEXCEPTION:
    res = IDS_CONFIG_E_CSPEXCEPTION;
    break;
  case CONFIG_E_TRANSACTIONINGFAILURE:
    res = IDS_CONFIG_E_TRANSACTIONINGFAILURE;
    break;
  case CONFIG_E_BAD_XML:
    res = IDS_CONFIG_E_BAD_XML;
    break;
  };

  return res;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, 
  int nCmdShow)
{
	bool bQuiet = false;
  CCommandLine cmdLine(lpstrCmdLine);
  std::wstring strFileName;
  _hInstance = hInstance;

  ParseOmobusConf(NULL, NULL, _mui_par);

  for( int i = 0; i < cmdLine._Argc(); i++ )  {
    if( _tcsicmp(cmdLine._Argv()[i], _T("--silent")) == 0 || 
        _tcsicmp(cmdLine._Argv()[i], _T("--quite")) == 0 )
      bQuiet = true;
    else if( _tcsicmp(cmdLine._Argv()[i], _T("--file")) == 0 ) {
      if( cmdLine._Argc() > (i + 1) ) {
        strFileName = cmdLine._Argv()[i+1];
        i++;
      }
    }
  }

  if( strFileName.empty() ) {
    if( !bQuiet )
      MessageBoxEx(IDS_COMMANDLINE, IDS_WARNING, 0);
    return 1;
  }

  if( !bQuiet ) {
    if( MessageBoxEx(IDS_CONF_CHANGE_CONFIRM, IDS_WARNING, MB_YESNO|MB_ICONQUESTION) == IDNO )
		  return 0;
	}

  HRESULT hr =ProcessXMLConfig(_readfile(strFileName.c_str()).c_str());

	if( !bQuiet )
    MessageBoxEx(hr != S_OK ? getError(hr) : IDS_OPERATION_SUCCESSFULL, 
      hr != S_OK ? IDS_WARNING : IDS_INFORMATION, 
      MB_OK|(hr != S_OK ? MB_ICONSTOP : MB_ICONINFORMATION));

  return hr != S_OK ? 1 : 0;
}
