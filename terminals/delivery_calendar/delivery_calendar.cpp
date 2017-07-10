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
#include <gesture.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlenc.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlwince.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atlomobus.h>
#include <atllabel.h>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-delivery_calendar-terminal-element";
static int _nHeight = 20;
static std::wstring _strTitle0, _strTitle1, _strNone;

typedef struct _tagDATE_CONF {
  wchar_t del_date[OMOBUS_MAX_DATE + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
} DATE_CONF;
typedef std::vector<DATE_CONF> calendar_t;

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontLB, hFontB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  bool focus;
  const wchar_t *date_fmt, *account_id;
  calendar_t cal_a, cal_d, *cal;
} HTERMINAL;

static void setDate(HTERMINAL *h, const wchar_t *d);
static std::wstring fmtDate(HTERMINAL *h, const wchar_t *d);

struct find_date : public std::unary_function <DATE_CONF&, bool> { 
  find_date(const wchar_t *d_) : d(d_) {
  }
  bool operator()(const DATE_CONF &v) { 
    return COMPARE_ID(d, v.del_date);
  }
  const wchar_t *d;
};

class CMainFrame : 
	public CStdDialogResizeImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  calendar_t *m_cal;
  CListViewCtrl m_list;
  
public:
  CMainFrame(HTERMINAL *h, calendar_t *cal) : m_hterm(h), m_cal(cal) {
  }

	enum { IDD = IDD_CALENDAR };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMainFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_cal->size());
    m_list.ShowWindow(m_cal->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    DATE_CONF *ptr = &((*m_cal)[lpdis->itemID]);
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFontLB);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd);
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(2);
    rect.bottom = rect.top + DRA::SCALEY(18);
    dc.DrawText(fmtDate(m_hterm, ptr->del_date).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_hterm->hFont);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK|DT_END_ELLIPSIS);

    dc.DrawLine(lpdis->rcItem.left, lpdis->rcItem.bottom - 1, 
      lpdis->rcItem.right, lpdis->rcItem.bottom - 1);

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    dc.SetTextColor(crOldTextColor);
    dc.SetBkColor(crOldBkColor);
    _LockMainMenuItems();
    return 1;
  }

  LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPMEASUREITEMSTRUCT lmi = (LPMEASUREITEMSTRUCT)lParam;
    if( lmi->CtlType != ODT_LISTVIEW )
      return 0;
    lmi->itemHeight = DRA::SCALEY(56);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _setDate(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _setDate(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
	}

  void _setDate(size_t sel) {
    if( sel < m_cal->size() ) {
      setDate(m_hterm, (*m_cal)[sel].del_date);
      EndDialog(ID_ACTION);
    }
  }
};

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  AtlLoadString(uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

// Format: %account_id%;%delivery_date%;%descr%;
static 
bool delivery_calendar(void *cookie, int line, const wchar_t **argv, int argc)
{
  HTERMINAL *ptr = NULL;
  calendar_t *c = NULL;
  DATE_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (HTERMINAL *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) ) {
    c = &ptr->cal_d;
  } else if( COMPARE_ID(argv[0], ptr->account_id) ) {
    c = &ptr->cal_a;
  }
  if( c != NULL ) {
    memset(&cnf, 0, sizeof(cnf));
    COPY_ATTR__S(cnf.del_date, argv[1]);
    COPY_ATTR__S(cnf.descr, argv[2]);
    c->push_back(cnf);
  }

  return true;
}

static
const wchar_t *getDate(HTERMINAL *h) {
  return h->get_conf(__INT_DEL_DATE, NULL);
}

static
void setDate(HTERMINAL *h, const wchar_t *d) {
  return h->put_conf(__INT_DEL_DATE, d);
}

static
std::wstring fmtDate(HTERMINAL *h, const wchar_t *d) {
  struct tm t;
  return wsftime(date2tm(d, &t), h->date_fmt);
}

static
void OnReload(HTERMINAL *h) {
  const wchar_t *d;
  calendar_t::iterator it;

  h->cal = NULL;
  h->cal_a.clear();
  h->cal_d.clear();
  h->account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  ReadOmobusManual(L"delivery_calendar", h, delivery_calendar);
  DEBUGMSG(TRUE, (L"delivery_calendar = %i:%i\n", h->cal_a.size(), h->cal_d.size()));
  if( !h->cal_a.empty() ) {
    h->cal = &h->cal_a;
  } else if( !h->cal_d.empty() ) {
    h->cal = &h->cal_d;
  }
  if( h->cal != NULL ) {
    if( (d = getDate(h)) != NULL ) {
      if( (it = std::find_if(h->cal->begin(), h->cal->end(), find_date(d))) == h->cal->end() ) {
        setDate(h, NULL);
      }
    } else {
      setDate(h, (*(h->cal))[0].del_date);
    }
  } else {
    setDate(h, NULL);
  }
}

static
void OnShow(HTERMINAL *h) {
  DEBUGMSG(1, (L"delivery_calendar: OnShown\n"));
  OnReload(h);
  SetTimer(h->hWnd, 1, 600000, NULL);
}

static
void OnHide(HTERMINAL *h) {
  KillTimer(h->hWnd, 1);
  h->cal = NULL;
  h->cal_a.clear();
  h->cal_d.clear();
  h->account_id = NULL;
  DEBUGMSG(1, (L"delivery_calendar: OnHide\n"));
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);
  const wchar_t *d = NULL;

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  if( h->cal ) {
    DrawText(hDC, _strTitle0.c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);
    SelectObject(hDC, h->hFontB);
    if( (d = getDate(h)) == NULL ) {
      if( !h->focus ) {
        SetTextColor(hDC, OMOBUS_ALLERTCOLOR);
      }
      DrawText(hDC, _strNone.c_str(), -1, &rect, DT_RIGHT|DT_SINGLELINE);
    } else {
      DrawText(hDC, fmtDate(h, d).c_str(), -1, &rect, DT_RIGHT|DT_SINGLELINE);
    }
  } else {
    if( !h->focus ) {
      SetTextColor(hDC, OMOBUS_ALLERTCOLOR);
    }
    SelectObject(hDC, h->hFontB);
    DrawText(hDC, _strTitle1.c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);
  }

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) 
{
  if( h->cal != NULL )
    CMainFrame(h, h->cal).DoModal(h->hParWnd);
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
  case WM_TIMER:
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
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus )
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
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
  pf_terminals_put_conf put_conf, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL ) {
    return NULL;
  }

  h->hWnd = NULL;
  h->focus = false;
  h->cal = NULL;
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontLB = CreateBaseFont(DRA::SCALEY(14), FW_BOLD, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, 
    get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _strTitle0 = wsfromRes(IDS_TITLE_0);
  _strTitle1 = wsfromRes(IDS_TITLE_1);
  _strNone = wsfromRes(IDS_NONE);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, wcsistrue(get_conf(__INT_SPLIT_ACTIVITY_SCREEN, NULL)) ?
      OMOBUS_SCREEN_ACTIVITY_TAB_1 : OMOBUS_SCREEN_ACTIVITY);
  }

  return h;
}

void terminal_deinit(void *ptr) {
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontLB);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  delete h;
}
