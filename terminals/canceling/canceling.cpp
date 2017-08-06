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
#include <atlddx.h>
#include <atlomobus.h>
#include <atllabel.h>

#include <string>
#include <vector>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

#define CALENDAR_LEFT                    L"canceling->calendar->left"
#define CALENDAR_LEFT_DEF                L"1"
#define CALENDAR_RIGHT                   L"canceling->calendar->right"
#define CALENDAR_RIGHT_DEF               L"60"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-canceling-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

typedef struct _tagCANCELING {
  const wchar_t *w_cookie;
  omobus_location_t posCre;
  time_t ttCre;
  SIMPLE_CONF *type, *b_date, *e_date;
  wchar_t note[OMOBUS_MAX_NOTE + 1];
  simple_t *_types, *_calendar;
} CANCELING;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode, *date_fmt;
  bool focus;
  int calendar_left, calendar_right;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

class CTypesFrame : 
  public CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CTypesFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CCalendarFrame : 
  public CListFrame<IDD_CALENDAR, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CCalendarFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_CALENDAR, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CCancelingFrame : 
	public CStdDialogResizeImpl<CCancelingFrame>,
	public CUpdateUI<CCancelingFrame>,
	public CMessageFilter
{
protected:
  CDimEdit m_note;
  CHyperButtonTempl<CCancelingFrame> m_type, m_b_date, m_e_date;
  HTERMINAL *m_hterm;
  CANCELING *m_doc;

public:
  CCancelingFrame(HTERMINAL *hterm, CANCELING *r) : m_hterm(hterm), m_doc(r) {
    if( !r->_types->empty() ) {
      r->type = &(*r->_types)[0];
    }
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CCancelingFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CCancelingFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CCancelingFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEdit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CCancelingFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(CreateMenuBar(IDR_MAINFRAME));
		UIAddChildWindowContainer(m_hWnd);
    m_note.SubclassWindow(GetDlgItem(IDC_EDIT_0));
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc->note));
    m_note.SetDimText(IDC_EDIT_0);
    m_type.SubclassWindow(GetDlgItem(IDC_TYPE));
    m_type.SetParent(this, IDC_TYPE);
    m_type.EnableWindow(m_doc->_types->size() > 1);
    m_b_date.SubclassWindow(GetDlgItem(IDC_B_DATE));
    m_b_date.SetParent(this, IDC_B_DATE);
    m_b_date.EnableWindow(!m_doc->_calendar->empty());
    m_e_date.SubclassWindow(GetDlgItem(IDC_E_DATE));
    m_e_date.SetParent(this, IDC_E_DATE);
    m_e_date.EnableWindow(!m_doc->_calendar->empty());
    _SetLabel(m_type, m_doc->type);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    memset(m_doc->note, 0, sizeof(m_doc->note));
    if( m_note.IsWindow() ) {
      m_note.GetWindowText(m_doc->note, CALC_BUF_SIZE(m_doc->note));
    }
    _LockMainMenuItems();
    return 0;
  }

  LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( _IsDataValid() ) {
      if( MessageBoxEx(m_hWnd, IDS_STR1, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
        if( _Close() ) {
          MessageBoxEx(m_hWnd, IDS_STR2, IDS_INFO, MB_ICONINFORMATION);
          EndDialog(wID);
        } else {
          MessageBoxEx(m_hWnd, IDS_STR3, IDS_ERROR, MB_ICONSTOP);
        }
      }
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( !_IsDataValid() || MessageBoxEx(m_hWnd, IDS_STR4, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES )
      EndDialog(wID);
    return 0;
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( nID == IDC_TYPE ) {
      if( CTypesFrame(m_hterm, m_doc->_types, &m_doc->type).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_type, m_doc->type);
      }
    } else if( nID == IDC_B_DATE ) {
      if( CCalendarFrame(m_hterm, m_doc->_calendar, &m_doc->b_date).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_b_date, m_doc->b_date);
        /*if( m_doc->e_date == NULL || wcscmp(m_doc->b_date->id, m_doc->e_date->id) > 0 ) {
          m_doc->e_date = m_doc->b_date;
          _SetLabel(m_e_date, m_doc->e_date);
        }*/
      }
    } else if( nID == IDC_E_DATE ) {
      if( CCalendarFrame(m_hterm, m_doc->_calendar, &m_doc->e_date).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_e_date, m_doc->e_date);
        /*if( m_doc->b_date == NULL || wcscmp(m_doc->b_date->id, m_doc->e_date->id) > 0 ) {
          m_doc->b_date = m_doc->e_date;
          _SetLabel(m_b_date, m_doc->b_date);
        }*/
      }
    }
    _LockMainMenuItems();
    return true;
  }

protected:
  bool _IsDataValid() {
    return (m_doc->type != NULL || m_doc->note[0] != L'\0') && 
      m_doc->b_date != NULL && m_doc->e_date != NULL &&
      wcscmp(m_doc->b_date->id, m_doc->e_date->id) <= 0;
  }

  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, _IsDataValid());
		UIUpdateToolBar();
	}

  void _SetLabel(CHyperButtonTempl<CCancelingFrame> &ctrl, SIMPLE_CONF *conf) {
    if( conf == NULL )
      return;
    CString t = L"["; t += conf->descr; t += L"]";
    ctrl.SetLabel(t);
  }

  bool _Close();
};

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

// Format: %id%;%descr%;
static 
bool __simple(void *cookie, int line, const wchar_t **argv, int count) 
{
  SIMPLE_CONF cnf;
  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 2 ) {
    return false;
  }
  memset(&cnf, 0, sizeof(SIMPLE_CONF));
  COPY_ATTR__S(cnf.id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  ((simple_t *)cookie)->push_back(cnf);
  return true;
}

static
bool WriteDocPk(HTERMINAL *h, CANCELING *r, uint64_t &doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_CANCELING, true);
  if( xml.empty() || r->b_date == NULL || r->e_date == NULL ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%doc_note%", fix_xml(r->note).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", r->w_cookie);
  wsrepl(xml, L"%created_dt%", wsftime(r->ttCre).c_str());
  wsrepl(xml, L"%created_gps_dt%", wsftime(r->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(r->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(r->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%canceling_type_id%", r->type ? fix_xml(r->type->id).c_str() : OMOBUS_UNKNOWN_ID);
  wsrepl(xml, L"%b_date%", fix_xml(r->b_date->id).c_str());
  wsrepl(xml, L"%e_date%", fix_xml(r->e_date->id).c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_CANCELING, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, CANCELING *r, uint64_t &doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_USER_DOCUMENT, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%doc_type%", OMOBUS_CANCELING);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", r->w_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool CloseDoc(HTERMINAL *h, CANCELING *r) 
{
  omobus_location_t posClo; 
  omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( !WriteDocPk(h, r, doc_id, &posClo, ttClo) ) {
    return false;
  }
  WriteActPk(h, r, doc_id, &posClo, ttClo);

  return true;
}

bool CCancelingFrame :: _Close() 
{
  CWaitCursor _wc;
  return CloseDoc(m_hterm, m_doc);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) 
{
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2) };
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, _wsTitle.c_str(), _wsTitle.size(), &rect, DT_LEFT|DT_SINGLELINE);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) {
  CWaitCursor _wc;
  SIMPLE_CONF cnf;
  time_t tt; tm t;
  simple_t types, calendar;
  CANCELING doc;

  memset(&doc, 0, sizeof(CANCELING));
  doc.ttCre = omobus_time();
  omobus_location(&doc.posCre);
  doc.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  doc._types = &types;
  doc._calendar = &calendar;

  for( int i = h->calendar_left; i <= h->calendar_right; i++ ) {
    tt = doc.ttCre + 86400*i;
    memset(&t, 0, sizeof(t));
    memset(&cnf, 0, sizeof(cnf));
    localtime_r(&tt, &t);
    wcsftime(cnf.id, CALC_BUF_SIZE(cnf.id), ISO_DATE_FMT, &t);
    wcsftime(cnf.descr, CALC_BUF_SIZE(cnf.descr), h->date_fmt, &t);
    calendar.push_back(cnf);
  }
  ATLTRACE(L"calendar = %i\n", calendar.size());

  ReadOmobusManual(L"canceling_types", &types, __simple);
  ATLTRACE(L"canceling_types = %i\n", types.size());
  _wc.Restore();

  CCancelingFrame(h, &doc).DoModal(h->hParWnd);
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
	case OMOBUS_SSM_PLUGIN_SHOW:
    return 0;
	case OMOBUS_SSM_PLUGIN_HIDE:
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

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->get_conf = get_conf;
  h->hParWnd = hParWnd;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->calendar_left = _wtoi(get_conf(CALENDAR_LEFT, CALENDAR_LEFT_DEF));
  h->calendar_right = _wtoi(get_conf(CALENDAR_RIGHT, CALENDAR_RIGHT_DEF));
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);

  if( h->user_id == NULL ) {
    RETAILMSG(TRUE, (L"canceling: >>> INCORRECT USER_ID <<<\n"));
    memset(h, 0, sizeof(HTERMINAL));
    delete h;
    return NULL;
  }

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _wsTitle = wsfromRes(IDS_TITLE);

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK);
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
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
