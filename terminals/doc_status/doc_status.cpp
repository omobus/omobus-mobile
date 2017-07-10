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
#include <DeviceResolutionAware.h>
#include <stdlib.h>
#include <string>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"

typedef struct _tagMODULE {
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB;
  HBRUSH hbrHL, hbrN;
  bool focus;
} HTERMINAL;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static TCHAR _szWindowClass[] = L"omobus-doc_status-terminal-element";
static int _nHeight = 20;

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  LoadString(_hResInstance, uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

static
int getDocCount() {
  int count = 0;
  std::wstring filter = GetOmobusProfilePath(OMOBUS_PROFILE_DOCS); 
  filter += L"*.*+*.*";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      count++;
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
  return count;  
}

static
void OnCreate(HWND hWnd, HTERMINAL *h) {
  SetTimer(hWnd, 1, 60000, NULL);
}

static
void OnDestroy(HTERMINAL *h) {
  KillTimer(h->hWnd, 1);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2) };
  int count = getDocCount();
  HFONT hOldFont = (HFONT)SelectObject(hDC, count > 0 ? h->hFontB : h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);
  std::wstring msg = wsfromRes(count > 0 ? IDS_STR0 : IDS_STR1);
  if( count > 0 )
    msg += itows(count);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    if( count > 0 )
      SetTextColor(hDC, OMOBUS_ALLERTCOLOR);
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, msg.c_str(), msg.size(), &rect, DT_LEFT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) {
  if( getDocCount() > 0 ) {
    if( MessageBox(h->hWnd, wsfromRes(IDS_STR2).c_str(), 
          wsfromRes(IDS_INFO).c_str(), MB_ICONQUESTION|MB_YESNO) == IDYES ) {
      SetNameEvent(OMOBUS_DOCS_NOTIFY);
    }
  }
}

static
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch( uMsg ) {
	case WM_CREATE:
    OnCreate(hWnd, (HTERMINAL *)(((CREATESTRUCT *)lParam)->lpCreateParams));
    SetWindowLong(hWnd, GWL_USERDATA, 
      (LONG)(HTERMINAL *)(((CREATESTRUCT *)lParam)->lpCreateParams));
		break;
	case WM_DESTROY:
    OnDestroy((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    return 0;
	case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
      RECT rcDraw; GetClientRect(hWnd, &rcDraw);
      OnPaint((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), hDC, &rcDraw);
			EndPaint(hWnd, &ps);
		}
		return TRUE;
	case OMOBUS_SSM_PLUGIN_SELECT:
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus )
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd);
		return 0;
  case WM_TIMER:
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
HWND CreatePluginWindow(HTERMINAL *h, const TCHAR *szWindowClass, WNDPROC lpfnWndProc) 
{
	WNDCLASS wc; 
  memset(&wc, 0, sizeof(WNDCLASS));
	wc.hInstance = _hInstance;
	wc.lpszClassName = szWindowClass;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpfnWndProc = lpfnWndProc;
	RegisterClass(&wc);
  return CreateWindowEx(0, szWindowClass, szWindowClass, WS_CHILD, 0, 0, 
    GetSystemMetrics(SM_CXSCREEN), 0, h->hParWnd, NULL, _hInstance, h);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
    _hResInstance = NULL;
		break;
	}

	return TRUE;
}

void *terminal_init(HWND hParWnd, pf_terminals_get_conf get_conf, 
  pf_terminals_put_conf /*put_conf*/, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  
  _hResInstance = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ROOT|OMOBUS_SCREEN_WORK);
  }

  return h;
}

void terminal_deinit(void *ptr) {
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
