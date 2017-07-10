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

#include "resource.h"
#include "TodayWindow.h"
#include "WaitCursor.h"

class COmobusToday : public CTodayWindow
{
public:
  COmobusToday(HINSTANCE hInstance) : 
      CTodayWindow(hInstance, _T("COmobusTodayClass"), _T("COmobusTodayWnd"), 
        IDI_TODAY_ICON) {
  }

  ~COmobusToday() {
  }

protected:
  virtual void OnTodayAction() {
    WaitCursor _wc(m_hInstance);
    wchar_t startup[255+1] = L"";
    LoadInfo(L"Startup", startup, 255);
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));
    if( ::CreateProcess(startup, NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi) ) {
      ::CloseHandle(pi.hProcess);
      ::CloseHandle(pi.hThread);
    }
  }

  virtual void OnLButtonUp(UINT nFlags, POINT point) {
    OnTodayAction();
  }

  virtual void OnPaint(HDC hDC) {
    SIZE size = {0};
    RECT rect = {0};
    wchar_t title[32+1] = L"OMOBUS for Windows Mobile";
    LoadInfo(L"Title", title, 32);

    HFONT hFontOld = (HFONT)SelectObject(hDC, m_hBoldTodayFont);
	  GetTextExtentPoint32(hDC, title, wcslen(title), &size);
	  rect.left = DRA::SCALEX(28); rect.top = DRA::SCALEY(3); 
    rect.right = rect.left + size.cx; rect.bottom = rect.top + size.cy;
	  DrawText(hDC, title, -1, &rect, DT_LEFT);

	  SelectObject(hDC, hFontOld);
  }

  void LoadInfo(const wchar_t *title, wchar_t *data, int len) {
    HKEY hKey = NULL;
    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Today\\Items\\OMOBUS",
          0, 0, &hKey) == ERROR_SUCCESS ) {
      DWORD dwLen = sizeof(wchar_t)*len, dwType = REG_SZ;
      if( RegQueryValueEx(hKey, title, NULL, &dwType, (LPBYTE)data, &dwLen)
        == ERROR_SUCCESS ) {
        data[dwLen>=len?0:dwLen] = L'\0';
      }
      RegCloseKey(hKey);
    }
  }
};

COmobusToday *hydraToday = NULL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    DisableThreadLibraryCalls((HINSTANCE)hModule);
		hydraToday = new COmobusToday((HINSTANCE)hModule);
    if( hydraToday != NULL )
		  hydraToday->UnregisterTodayClass();
		break;
	case DLL_PROCESS_DETACH:
    if( hydraToday != NULL )
		  delete hydraToday;
    hydraToday = NULL;
		break;
	}

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if( hydraToday == NULL )
    return 0;
	return hydraToday->TodayWndProc(uMsg, wParam, lParam);
}

HWND InitializeCustomItem(TODAYLISTITEM *ptli, HWND hWndParent)
{
  if( hydraToday == NULL )
    return NULL;

  hydraToday->RegisterTodayClass((WNDPROC)WndProc);
  hydraToday->Create(hWndParent, 20, WS_VISIBLE | WS_CHILD);
  hydraToday->RefreshWindow(TRUE);

  return hydraToday->GetHWND();
}
