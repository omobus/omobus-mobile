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

#define WINVER 0x0420
#define _WTL_NO_CSTRING
#define _WTL_CE_NO_ZOOMSCROLL
#define _WTL_CE_NO_FULLSCREEN
#define _ATL_NO_HOSTING
#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA

#include <windows.h>
#include <stdlib.h>
#include <tpcshell.h>
#include <aygshell.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlwince.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <atlomobus.h>

#include <string>
#include <critical_section.h>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"

typedef struct _tagMODULE {
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN;
  bool focus;
  int gps_lifetime, max_dop, wakeup, read_timeout;
  const wchar_t *port, *datetime_fmt, *working_hours, *working_days;
  uint32_t fixedPos_Empty, fixedPos_Valid;
} HTERMINAL;

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-gps_status-terminal-element";
static int _nHeight = 20;

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  AtlLoadString(uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

static
UINT gpsMonStatus(omobus_location_t *gpsInfo) {
  if( gpsInfo->gpsmon_status == -1 ) return IDS_STATE_NONE;
  if( gpsInfo->gpsmon_status == 1 ) return IDS_STATE_ON;
  if( gpsInfo->gpsmon_status == 2 ) return IDS_STATE_OFF;
  if( gpsInfo->gpsmon_status == 3 ) return IDS_STATE_FAILD_PORT;
  if( gpsInfo->gpsmon_status == 4 ) return IDS_STATE_FAILD_DATA;
  return IDS_STATE_UNKNOWN;
}

static
UINT gpsPosStatus(omobus_location_t *gpsInfo) {
  if( gpsInfo->location_status == 1 ) return IDS_POS_YES;
  if( gpsInfo->location_status == 2 ) return IDS_POS_OLD;
  return IDS_POS_NO;
}

UINT gpsPosStatusDet(omobus_location_t *l) {
  if( l->location_status == 1 ) return IDS_LOCATION_ONLINE;
  if( l->location_status == 2 ) return IDS_LOCATION_CACHED;
  return IDS_LOCATION_INVALID;
}

typedef struct _fixed_pos_t {
  wchar_t cur[11];
  uint32_t *fixedPos_Empty, *fixedPos_Valid;
} fixed_pos_t;

// Format: %fix_dt%;%satellite_dt%;%latitude%;%longitude%;%trace%;%seconds%;%pk%;
static 
bool __fixed_pos(void *cookie, int line, const wchar_t **argv, int count) 
{
  fixed_pos_t *ptr = (fixed_pos_t *) cookie;
  double La, Lo;
  
  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 4 ) {
    return false;
  }
  if( wcsncmp(ptr->cur, argv[0], CALC_BUF_SIZE(ptr->cur)) != 0 ) {
    return true;
  }
  if( (La = wcstof(argv[2])) == 0.0 && (Lo = wcstof(argv[3])) == 0.0 ) {
    (*ptr->fixedPos_Empty)++;
  } else {
    (*ptr->fixedPos_Valid)++;
  }
  return true;
}

static
void getFixedPos(HTERMINAL *h) {
  fixed_pos_t ck;
  memset(&ck, 0, sizeof(ck));
  h->fixedPos_Empty = 0;
  h->fixedPos_Valid = 0;
  wcsncpy(ck.cur, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), 
    CALC_BUF_SIZE(ck.cur));
  ck.fixedPos_Empty = &h->fixedPos_Empty;
  ck.fixedPos_Valid = &h->fixedPos_Valid;
  ReadOmobusJournal(OMOBUS_GPS_POS, &ck, __fixed_pos);
  ATLTRACE(OMOBUS_GPS_POS L" = %i:%i\n", h->fixedPos_Valid,
    h->fixedPos_Empty);
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
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);
  omobus_location_t gpsInfo; omobus_location(&gpsInfo);
  std::wstring gpsMon = wsfromRes(IDS_STR0); gpsMon += wsfromRes(gpsMonStatus(&gpsInfo));
  std::wstring gpsPos = wsfromRes(IDS_STR1); gpsPos += wsfromRes(gpsPosStatus(&gpsInfo));

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, gpsMon.c_str(), gpsMon.size(), &rect, DT_LEFT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);
  DrawText(hDC, gpsPos.c_str(), gpsPos.size(), &rect, DT_RIGHT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

class CGpsInfoFrame : 
	public CStdDialogImpl<CGpsInfoFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  CString m_Fmt[13], m_Tmp, m_Empty;

public:
  CGpsInfoFrame(HTERMINAL *h) : m_hterm(h) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP(CGpsInfoFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(ID_EXIT, OnExit)
		CHAIN_MSG_MAP(CStdDialogImpl<CGpsInfoFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    HWND hMenuBar = AtlCreateMenuBar(m_hWnd, ID_MENU_GPSINFO, SHCMBF_HIDESIPBUTTON);
    for( int i = 0; i < 13; i++ )
      GetDlgItemText(2000+i, m_Fmt[i]);
    m_Empty.LoadString(IDS_EMPTY);
    getFixedPos(m_hterm); 
    Load();
    SetTimer(1, 5000, NULL);
		return bHandled = FALSE;
	}

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    // VK_F24 удерживает от перехода в спящий режим только при включенном экране.
    keybd_event(VK_F24, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
    Load();
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    KillTimer(1);
    PostMessage(WM_CLOSE);
    return 0;
  }

protected:
  void Load() {
    CWaitCursor wc;
    int i = 0;
    // Получаем информацию о состояние GPS-устройства
    omobus_location_t gpsInfo; omobus_location(&gpsInfo);
    // Состояние (Вкл/Выкл) GPS-монитора
    m_Tmp.Format(m_Fmt[i], LoadStringEx(gpsMonStatus(&gpsInfo)).GetString(), 
      m_hterm->port == NULL ? L"gpsapi" : m_hterm->port);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Интервал жизни GPS-данных
    m_Tmp.Format(m_Fmt[i], m_hterm->gps_lifetime);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Количество зафиксированных позиций
    m_Tmp.Format(m_Fmt[i], m_hterm->fixedPos_Empty + m_hterm->fixedPos_Valid, 
      m_hterm->fixedPos_Empty);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Качество GPS данных
    m_Tmp.Format(m_Fmt[i], wsfromRes(gpsPosStatusDet(&gpsInfo)).c_str());
    SetDlgItemText(2000+(i++), m_Tmp);
    // Доступно спутников
    m_Tmp.Format(m_Fmt[i], gpsInfo.sat_inuse, gpsInfo.sat_inview);
    SetDlgItemText(2000+(i++), m_Tmp);
    // UTC время со спутника
    if( gpsInfo.utc > 0 )
      m_Tmp.Format(m_Fmt[i], wsftime(gpsInfo.utc, m_hterm->datetime_fmt, true).c_str());
    else 
      m_Tmp = m_Empty;
    SetDlgItemText(2000+(i++), m_Tmp);
    // Позиция
    m_Tmp.Format(m_Fmt[i], 
      ftows(gpsInfo.lat, OMOBUS_GPS_PREC).c_str(),
      ftows(gpsInfo.lon, OMOBUS_GPS_PREC).c_str());
    SetDlgItemText(2000+(i++), m_Tmp);
    // Скорость
    m_Tmp.Format(m_Fmt[i], gpsInfo.speed);
    SetDlgItemText(2000+(i++), m_Tmp);
    // DOP (http://ru.wikipedia.org/wiki/DOP_(GPS))
    m_Tmp.Format(m_Fmt[i], gpsInfo.hdop, gpsInfo.vdop, gpsInfo.pdop);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Высота над уровнем моря
    m_Tmp.Format(m_Fmt[i], gpsInfo.elv);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Период фиксации
    m_Tmp.Format(m_Fmt[i], m_hterm->wakeup);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Фильтр по DOP
    m_Tmp.Format(m_Fmt[i], m_hterm->read_timeout, m_hterm->max_dop);
    SetDlgItemText(2000+(i++), m_Tmp);
    // Рабочее время
    m_Tmp.Format(m_Fmt[i], m_hterm->working_hours == NULL ? m_Empty.GetString() : m_hterm->working_hours);
    SetDlgItemText(2000+(i++), m_Tmp);
  }
};

static 
void OnAction(HTERMINAL *h) {
  CGpsInfoFrame(h).DoModal(h->hParWnd);
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
    _Module.Init(NULL, _hInstance);
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
    chk_dlclose(_Module.m_hInstResource);
    _Module.Term();
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
  h->gps_lifetime = _wtoi(get_conf(OMOBUS_GPSMON_LIFETIME, OMOBUS_GPSMON_LIFETIME_DEF));
  h->port = get_conf(OMOBUS_GPSMON_PORT, NULL);
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME_F, ISO_DATETIME_FMT);
  h->max_dop = _wtoi(get_conf(OMOBUS_GPSMON_MAX_DOP, OMOBUS_GPSMON_MAX_DOP_DEF));
  h->wakeup = _wtoi(get_conf(OMOBUS_GPSMON_WAKEUP, OMOBUS_GPSMON_WAKEUP_DEF));
  h->read_timeout = _wtoi(get_conf(OMOBUS_GPSMON_READ_TIMEOUT, OMOBUS_GPSMON_READ_TIMEOUT_DEF));
  h->working_hours = get_conf(OMOBUS_WORKING_HOURS, NULL);
  h->working_days = get_conf(OMOBUS_WORKING_DAYS, NULL);

  if( h->port != NULL ) {
    if( h->port[0] == L'\0' || wcsicmp(h->port, L"gpsapi") == 0 || wcsicmp(h->port, L"gpsapi:") == 0 )
      h->port = NULL;
  }

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ANY);
  }

  return h;
}

void terminal_deinit(void *ptr) {
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
