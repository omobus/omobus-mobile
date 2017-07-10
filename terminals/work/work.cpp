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
#include <WaitCursor.h>
#include <stdlib.h>
#include <string>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"
#include "../../version"

#define OMOBUS_WORK_DAY_BEGIN         L"work->begin"
#define OMOBUS_WORK_DAY_BEGIN_DEF     L""
#define OMOBUS_WORK_DAY_END           L"work->end"
#define OMOBUS_WORK_DAY_END_DEF       L"act"
#define OMOBUS_WORK_DAY_W_POS         L"work->pos_needed"
#define OMOBUS_WORK_DAY_W_POS_DEF     L"no"

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf; 
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN;
  bool focus;
  const wchar_t *user_id, *wd_begin, *wd_end, *pk_ext, *pk_mode;
  wchar_t w_cookie[OMOBUS_MAX_ID + 1];
  bool w_pos;
} HTERMINAL;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static TCHAR _szWindowClass[] = L"omobus-work-terminal-element";
static int _nHeight = 20;

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void _exec_oper(const wchar_t *oper) {
  if( wcscmp(oper, L"sync") == 0 ) {
    SetNameEvent(OMOBUS_SYNC_NOTIFY);
  } else if( wcscmp(oper, L"upd") == 0 ) {
    SetNameEvent(OMOBUS_UPD_NOTIFY);
  } else if( wcscmp(oper, L"docs") == 0 ) {
    SetNameEvent(OMOBUS_DOCS_NOTIFY);
  } else if( wcscmp(oper, L"acts") == 0 ) {
    SetNameEvent(OMOBUS_ACTS_NOTIFY);
  } else if( wcscmp(oper, L"gpsmon") == 0 ) {
    SetNameEvent(OMOBUS_GPSMON_NOTIFY);
  }
}

static
void _exec_slist(const wchar_t *s) {
  if( s == NULL )
    return;
  std::wstring buf;
  for( int i = 0; s[i] != L'\0'; i++ ) {
    if( s[i] == L' ' ) {
      if( !buf.empty() )
        _exec_oper(buf.c_str());
      buf = L"";
    } else {
      buf += s[i];
    }
  }
  if( !buf.empty() )
    _exec_oper(buf.c_str());
}

static
void initCookie(HTERMINAL *h) {
  _snwprintf(h->w_cookie, OMOBUS_MAX_ID, L"w_%x", omobus_time());
  h->put_conf(__INT_W_COOKIE, h->w_cookie);
  DEBUGMSG(TRUE, (L"w_cookie/begin: %s\n", h->w_cookie));
}

static
void deinitCookie(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"w_cookie/end: %s\n", h->w_cookie));
  h->put_conf(__INT_W_COOKIE, NULL);
  memset(h->w_cookie, 0, sizeof(h->w_cookie));
}

static
void StoreActivity(HTERMINAL *h, const wchar_t *state) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_X_USER_WORK, true);
  if( xml.empty() )
    return;
  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%state%", state);
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  
  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_WORK, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static
int getStatus(HTERMINAL *h) {
  const wchar_t *ptr = h->get_conf(__INT_CURRENT_SCREEN, NULL);
  return ptr == NULL ? OMOBUS_SCREEN_ROOT : _wtoi(ptr);
}

static
void putStatus(HTERMINAL *h, int status) {
  wchar_t buf[11]; wsprintf(buf, L"%i", status);
  h->put_conf(__INT_CURRENT_SCREEN, buf);
}

static
std::wstring getMsg(int status) {
  return wsfromRes(status & OMOBUS_SCREEN_ROOT ? IDS_STR0 : IDS_STR1);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);
  std::wstring msg = getMsg(getStatus(h));

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, msg.c_str(), msg.size(), &rect, DT_RIGHT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
bool OnStartWorkDay(HTERMINAL *h) {
  if( h->w_pos ) {
    omobus_location_t l; omobus_location(&l);
    if( !l.location_status ) {
      if( MessageBox(h->hWnd, wsfromRes(IDS_STR4).c_str(), wsfromRes(IDS_ERROR).c_str(),
            MB_ICONWARNING|MB_YESNO) == IDYES ) {
        return false;
      }
    }
  }
  WaitCursor _wc(_hResInstance);
  _exec_slist(h->wd_begin);
  initCookie(h);
  StoreActivity(h, L"begin");
  return true;
}

static
bool OnStopWorkDay(HTERMINAL *h) {
  if( MessageBox(h->hWnd, wsfromRes(IDS_STR3).c_str(), wsfromRes(IDS_INFO).c_str(),
    MB_ICONQUESTION|MB_YESNO) == IDYES ) {
    WaitCursor _wc(_hResInstance);
    StoreActivity(h, L"end");
    deinitCookie(h);
    _exec_slist(h->wd_end);
    return true;
  }
  return false;
}

static 
void OnAction(HTERMINAL *h) {
  if( getStatus(h) & OMOBUS_SCREEN_ROOT ) {
    if( OnStartWorkDay(h) )
      putStatus(h, OMOBUS_SCREEN_WORK);
  } else {
    if( OnStopWorkDay(h) )
      putStatus(h, OMOBUS_SCREEN_ROOT);
  }
}

static
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch( uMsg ) {
	case WM_CREATE:
    SetWindowLong(hWnd, GWL_USERDATA, 
      (LONG)(HTERMINAL *)(((CREATESTRUCT *)lParam)->lpCreateParams));
		break;
	case WM_DESTROY:
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
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd);
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
  pf_terminals_put_conf put_conf, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->wd_begin = get_conf(OMOBUS_WORK_DAY_BEGIN, OMOBUS_WORK_DAY_BEGIN_DEF);
  h->wd_end = get_conf(OMOBUS_WORK_DAY_END, OMOBUS_WORK_DAY_END_DEF);
  h->w_pos = wcsistrue(get_conf(OMOBUS_WORK_DAY_W_POS, OMOBUS_WORK_DAY_W_POS_DEF));

  _hResInstance = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ROOT|OMOBUS_SCREEN_WORK);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_destroy_window(h->hWnd);
  delete h;
}
