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

#define UNSCHED_MAX_LIMIT         L"unsched->max_limit"
#define UNSCHED_MAX_LIMIT_DEF     L"255"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-unsched-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle;

typedef struct _tagUNSCHED {
  const wchar_t *w_cookie;
  wchar_t c_date[OMOBUS_MAX_DATE + 1];
  int count;
  omobus_location_t posCre;
  time_t ttCre;
  wchar_t note[OMOBUS_MAX_NOTE + 1];
} UNSCHED;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode, *date_fmt;
  bool focus;
  int max_limit;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

class CMainFrame : 
	public CStdDialogResizeImpl<CMainFrame>,
  public CUpdateUI<CMainFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  CDimEdit m_note;
  UNSCHED *m_doc;

public:
  CMainFrame(HTERMINAL *hterm, UNSCHED *doc) : m_hterm(hterm), m_doc(doc) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CMainFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
    COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnIdle)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, IDR_MAINFRAME));
    UIAddChildWindowContainer(m_hWnd);
    m_note.SubclassWindow(GetDlgItem(IDC_EDIT_0));
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc->note));
    m_note.SetDimText(IDC_EDIT_0);
    _LockMainMenuItems();
    return bHandled = FALSE;
	}

  LRESULT OnIdle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _DoDataExchange();
    _LockMainMenuItems();
    return 0;
  }

  LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( _IsDataValid() ) {
      if( MessageBoxEx(m_hWnd, IDS_STR1, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
        if( _Close() ) {
          MessageBoxEx(m_hWnd, IDS_STR2, IDS_INFO, MB_ICONINFORMATION);
          EndDialog(ID_CLOSE);
        } else {
          MessageBoxEx(m_hWnd, IDS_STR3, IDS_ERROR, MB_ICONSTOP);
        }
      }
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( !_IsDataValid() || MessageBoxEx(m_hWnd, IDS_STR4, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
      EndDialog(ID_MENU_CANCEL);
    }
    return 0;
  }

private:
  bool _IsDataValid() {
    return m_doc->note[0] != L'\0';
  }

  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, _IsDataValid());
    UIUpdateToolBar();
	}

  void _DoDataExchange() {
    memset(m_doc->note, 0, sizeof(m_doc->note));
    if( m_note.IsWindow() ) {
      m_note.GetWindowText(m_doc->note, CALC_BUF_SIZE(m_doc->note));
      wcstrim(m_doc->note, ' ');
    }
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

// Format: %date%;...
static 
bool __unsched(void *cookie, int line, const wchar_t **argv, int argc) 
{
  UNSCHED *ptr = (UNSCHED *) cookie;

  if( line == 0 ) {
    return true;
  }
  if( ptr == NULL || argc < 1 ) {
    return false;
  }
  if( wcsncmp(ptr->c_date, argv[0], OMOBUS_MAX_DATE) != 0 ) {
    return true;
  }
  ptr->count++;

  return true;
}

static
bool WriteDocPk(HTERMINAL *h, UNSCHED *r, uint64_t &doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_UNSCHED, true);
  if( xml.empty() ) {
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

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_UNSCHED, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, UNSCHED *r, uint64_t &doc_id, omobus_location_t *posClo, 
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
  wsrepl(xml, L"%doc_type%", OMOBUS_UNSCHED);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", r->w_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteJournal(HTERMINAL *h, UNSCHED *r, uint64_t doc_id, 
  time_t tt) 
{
  std::wstring txt, templ;

  templ = txt = wsfromRes(IDS_J_UNSCHED, true);
  if( txt.empty() ) {
    return false;
  }

  wsrepl(txt, L"%date%", wsftime(tt, ISO_DATETIME_FMT).c_str());
  wsrepl(txt, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(txt, L"%doc_note%", r->note);
  
  return WriteOmobusJournal(OMOBUS_UNSCHED, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
bool CloseDoc(HTERMINAL *h, UNSCHED *r) 
{
  omobus_location_t posClo; 
  omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( !WriteDocPk(h, r, doc_id, &posClo, ttClo) ) {
    return false;
  }
  WriteActPk(h, r, doc_id, &posClo, ttClo);
  WriteJournal(h, r, doc_id, ttClo);
  return true;
}

bool CMainFrame :: _Close() 
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
  UNSCHED doc;

  memset(&doc, 0, sizeof(UNSCHED));
  doc.ttCre = omobus_time();
  omobus_location(&doc.posCre);
  doc.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  wcsncpy(doc.c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(doc.c_date));
  ReadOmobusJournal(OMOBUS_UNSCHED, &doc, __unsched);
  _wc.Restore();

  if( doc.count >= h->max_limit ) {
    MessageBoxEx(h->hParWnd, IDS_STR5, IDS_WARN, MB_ICONWARNING);
  } else {
    CMainFrame(h, &doc).DoModal(h->hParWnd);
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
  h->max_limit = _wtoi(get_conf(UNSCHED_MAX_LIMIT, UNSCHED_MAX_LIMIT_DEF));

  if( h->user_id == NULL ) {
    RETAILMSG(TRUE, (L"unsched: >>> INCORRECT USER_ID <<<\n"));
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
