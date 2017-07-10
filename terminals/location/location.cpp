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
#include <WaitCursor.h>
#include "resource.h"
#include "../../version"

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static TCHAR _szWindowClass[] = L"omobus-location-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle0, _wsTitle1, _wsTitleM;

typedef struct _tagMODULE {
  int _c;
  wchar_t _mark[32];
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode;
  int timeout;
  bool focus;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB;
  HBRUSH hbrHL, hbrN;
  const wchar_t *w_cookie, *a_cookie, *account_id, *activity_type_id;
  bool enabled;
} HTERMINAL;

// Format: %account_id%;
static 
bool __locations(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *h = (HTERMINAL *) cookie;
  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 1 ) {
    return false;
  }
  if( COMPARE_ID(h->account_id, argv[0]) ) {
    h->enabled = true;
    return false;
  }
  return true;
}

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void IncrementDocumentCounter(HTERMINAL *h) {
  uint32_t count = _wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0"));
  DEBUGMSG(TRUE, (__INT_ACTIVITY_DOCS L" = %i\n", count + 1));
  h->put_conf(__INT_ACTIVITY_DOCS, itows(count + 1).c_str());
}

static
bool writeDocument(const wchar_t *user_id, const wchar_t *pk_ext, const wchar_t *pk_mode,
  const wchar_t *w_cookie, const wchar_t *a_cookie, const wchar_t *account_id, 
  const wchar_t *activity_type_id, const omobus_location_t *l, time_t tt)
{
  std::wstring xml = wsfromRes(IDS_X_LOCATION, true);
  if( xml.empty() ) {
    RETAILMSG(TRUE, (L"location: Empty document template.\n"));
    return false;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%doc_id%", itows(GetDocId()).c_str());
  wsrepl(xml, L"%user_id%", user_id);
  wsrepl(xml, L"%w_cookie%", w_cookie);
  wsrepl(xml, L"%a_cookie%", a_cookie);
  wsrepl(xml, L"%account_id%", fix_xml(account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(activity_type_id).c_str());
  wsrepl(xml, L"%fix_dt%", wsftime(tt).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(l->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(l->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(l->lon, OMOBUS_GPS_PREC).c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_LOCATION, 
    user_id, pk_ext, pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
void Close(HTERMINAL *h)
{
  if( !h->enabled || 
      MessageBox(h->hWnd, wsfromRes(IDS_STR0).c_str(), 
        wsfromRes(IDS_INFO).c_str(), MB_ICONQUESTION|MB_YESNO) != IDYES )
    return;

  WaitCursor _wc(_hInstance);
  omobus_location_t l;

  for( int i = 0; i < 40; i++ ) {
    keybd_event(VK_F24, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
    omobus_location(&l);
    if( l.gpsmon_status == 0 || l.location_status == 0 || l.location_status == 1 )
      break;
    Sleep(500/**h->timeout*/);
  }
  _wc.Restore();

  if( l.location_status == 1 ) {
    if( writeDocument(h->user_id, h->pk_ext, h->pk_mode, h->w_cookie, h->a_cookie,
          h->account_id, h->activity_type_id, &l, omobus_time()) ) {
      h->_c++;
      IncrementDocumentCounter(h);
      MessageBox(h->hWnd, wsfromRes(IDS_STR1).c_str(), 
        wsfromRes(IDS_INFO).c_str(), MB_ICONINFORMATION);
    } else {
      MessageBox(h->hWnd, wsfromRes(IDS_STR3).c_str(), 
          wsfromRes(IDS_ERROR).c_str(), MB_ICONSTOP);
    }
  } else {
    MessageBox(h->hWnd, wsfromRes(IDS_STR2).c_str(), 
        wsfromRes(IDS_ERROR).c_str(), MB_ICONSTOP);
  }
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) 
{
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2) };
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->enabled ? h->hFontB : h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  if( h->enabled ) {
    DrawText(hDC, _wsTitle0.c_str(), _wsTitle0.size(), &rect, DT_LEFT|DT_SINGLELINE);
    if( h->_c ) {
      SelectObject(hDC, h->hFont);
      DrawText(hDC, _wsTitleM.c_str(), _wsTitleM.size(), &rect, DT_RIGHT|DT_SINGLELINE);
    }
  } else {
    DrawText(hDC, _wsTitle1.c_str(), _wsTitle1.size(), &rect, DT_LEFT|DT_SINGLELINE);
  }

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
void OnReload(HTERMINAL *h) {
  ReadOmobusManual(L"locations", h, __locations);
}

static
void OnShow(HTERMINAL *h) {
  const wchar_t *a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  if( wcscmp(h->_mark, a_cookie) != 0 ) {
    wcsncpy(h->_mark, a_cookie, CALC_BUF_SIZE(h->_mark));
    h->_c = 0;
  }
  h->enabled = false;
  h->w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  h->a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  h->activity_type_id = h->get_conf(__INT_ACTIVITY_TYPE_ID, OMOBUS_UNKNOWN_ID);
  h->account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  OnReload(h);
  DEBUGMSG(TRUE, (L"location::OnShow - _c=%i\n", h->_c));
}

static
void OnHide(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"location::OnHide - _c=%i\n", h->_c));
  h->enabled = false;
  h->activity_type_id = NULL;
  h->account_id = NULL;
}

static 
void OnAction(HTERMINAL *h) {
  EnableWindow(h->hParWnd, FALSE);
  Close(h);
  EnableWindow(h->hParWnd, TRUE);
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
  case OMOBUS_SSM_PLUGIN_RELOAD:
    OnReload((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
  case OMOBUS_SSM_PLUGIN_SHOW:
    OnShow((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    return 0;
	case OMOBUS_SSM_PLUGIN_HIDE:
    OnHide((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    return 0;
	case OMOBUS_SSM_PLUGIN_SELECT:
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus ) {
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    	InvalidateRect(hWnd, NULL, FALSE);
  	}
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
  h->timeout = _wtoi(get_conf(OMOBUS_GPSMON_READ_TIMEOUT, OMOBUS_GPSMON_READ_TIMEOUT_DEF));

  _hResInstance = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _wsTitle0 = wsfromRes(IDS_TITLE_0);
  _wsTitle1 = wsfromRes(IDS_TITLE_1);
  _wsTitleM = wsfromRes(IDS_TITLE_M);

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ACTIVITY);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
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
