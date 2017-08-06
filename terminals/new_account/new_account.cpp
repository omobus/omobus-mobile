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
#include <atllabel.h>
#include <atlomobus.h>

#include <string>
#include <vector>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-new_account-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle, _wsNone;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode, *w_cookie;
  bool focus;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  omobus_location_t m_posCre;
  time_t m_ttCre;
} HTERMINAL;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

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

class CNewAccountFrame : 
	public CStdDialogResizeImpl<CNewAccountFrame>,
	public CUpdateUI<CNewAccountFrame>,
	public CMessageFilter
{
protected:
  CDimEdit m_edit[5];
  CHyperButtonTempl<CNewAccountFrame> m_hl;
  HTERMINAL *m_hterm;
  simple_t *m_types;
  SIMPLE_CONF *m_cur_type;

public:
  CNewAccountFrame(HTERMINAL *hterm, simple_t *types) : 
      m_hterm(hterm), m_types(types) {
    m_cur_type = types->empty() ? NULL : &((*m_types)[0]);
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CNewAccountFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CNewAccountFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CNewAccountFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnIdle)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CNewAccountFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(CreateMenuBar(IDR_MAINFRAME));
		UIAddChildWindowContainer(m_hWnd);
    for( int i = 0; i < 5; i++ ) {
      // !!!! Предполагается, что IDC_EDIT_0, IDC_EDIT_2, ... имеют последовательные id !!!
      m_edit[i].SubclassWindow(GetDlgItem(IDC_EDIT_0 + i));
      m_edit[i].SetDimText(IDC_EDIT_0 + i);
    }
    m_hl.SubclassWindow(GetDlgItem(IDC_TYPE));
    m_hl.SetParent(this, IDC_TYPE);
    m_hl.EnableWindow(m_types->size() > 1 && m_cur_type != NULL );
    _SetHlLabel();
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnIdle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _LockMainMenuItems();
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( !_Compleat() )
      return 0;
    if( MessageBoxEx(m_hWnd, IDS_STR1, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
      if( _Close() ) {
        EndDialog(wID);
      } else {
        MessageBoxEx(m_hWnd, IDS_STR3, IDS_ERROR, MB_ICONSTOP);
      }
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_STR4, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES )
      EndDialog(wID);
    return 0;
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( CTypesFrame(m_hterm, m_types, &m_cur_type).DoModal(m_hWnd) == ID_ACTION ) {
      _SetHlLabel();
    }
    return true;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, _Compleat() == true);
		UIUpdateToolBar();
	}

  CString GetEditText(int num) {
    CString t; m_edit[num].GetWindowText(t);
    return t;
  }

  void _SetHlLabel() {
    if( m_cur_type == NULL )
      return;
    m_hl.SetTitle(m_cur_type->descr);
  }

  bool _Compleat() {
    return 
      !( GetEditText(0).IsEmpty() ||
         GetEditText(1).IsEmpty() ||
         GetEditText(2).IsEmpty() ||
         GetEditText(3).IsEmpty() ||
         GetEditText(4).IsEmpty() ||
         m_cur_type == NULL
       );
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
  if( line == 0 )
    return true;
  if( cookie == NULL || count < 2 )
    return false;
  memset(&cnf, 0, sizeof(SIMPLE_CONF));
  COPY_ATTR__S(cnf.id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  ((simple_t *)cookie)->push_back(cnf);
  return true;
}

static
bool WriteDocPk(HTERMINAL *h, const wchar_t *descr, const wchar_t *address,
  const wchar_t *delivery_address, const wchar_t *note, const wchar_t *number, 
  const wchar_t *type_id, uint64_t doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_DOC_NEW_ACCOUNT, true);
  if( xml.empty() )
    return false;

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  wsrepl(xml, L"%created_dt%", wsftime(h->m_ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%descr%", fix_xml(descr).c_str());
  wsrepl(xml, L"%address%", fix_xml(address).c_str());
  wsrepl(xml, L"%delivery_address%", fix_xml(delivery_address).c_str());
  wsrepl(xml, L"%note%", fix_xml(note).c_str());
  wsrepl(xml, L"%number%", number);
  wsrepl(xml, L"%new_account_type_id%", type_id);
  wsrepl(xml, L"%created_gps_dt%", wsftime(h->m_posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(h->m_posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(h->m_posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_NEW_ACCOUNT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, uint64_t doc_id, omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_USER_DOCUMENT, true);
  if( xml.empty() )
    return false;

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%doc_type%", OMOBUS_NEW_ACCOUNT);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", h->w_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool CloseDoc(HTERMINAL *h, const wchar_t *descr, const wchar_t *address,
  const wchar_t *delivery_address, const wchar_t *note, const wchar_t *number, 
  const wchar_t *type_id, const wchar_t *type_descr) 
{
  omobus_location_t posClo; omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( !WriteDocPk(h, descr, address, delivery_address, note, number, type_id, doc_id, 
          &posClo, ttClo) )
    return false;
  WriteActPk(h, doc_id, &posClo, ttClo);
  return true;
}

bool CNewAccountFrame :: _Close() {
  CWaitCursor _wc;
  return 
    m_cur_type == NULL 
      ? 
        false 
      :
        CloseDoc(m_hterm, 
          GetEditText(0), 
          GetEditText(2), 
          GetEditText(1), 
          GetEditText(3), 
          GetEditText(4), 
          m_cur_type->id,
          m_cur_type->descr);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
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
  DrawText(hDC, _wsTitle.c_str(), _wsTitle.size(), &rect, DT_LEFT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) {
  CWaitCursor _wc;
  simple_t types;
  h->w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  h->m_ttCre = omobus_time();
  omobus_location(&h->m_posCre);
  ReadOmobusManual(L"new_account_types", &types, __simple);
  ATLTRACE(L"new_account_types = %i\n", types.size());
  _wc.Restore();
  CNewAccountFrame(h, &types).DoModal(h->hParWnd);
  h->w_cookie = NULL;
  h->m_ttCre = 0;
  memset(&h->m_posCre, 0, sizeof(h->m_posCre));
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
  pf_terminals_put_conf /*put_conf*/, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _wsTitle = wsfromRes(IDS_TITLE);
  _wsNone = wsfromRes(IDS_NONE);

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
