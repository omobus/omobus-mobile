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

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-new_contact-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

typedef struct _tagNEW_CONTACT {
  const wchar_t *account_id, *activity_type_id, *w_cookie, *a_cookie;
  omobus_location_t posCre;
  time_t ttCre;
  SIMPLE_CONF *job_title;
  wchar_t first_name[OMOBUS_MAX_DESCR + 1], last_name[OMOBUS_MAX_DESCR + 1],
    middle_name[OMOBUS_MAX_DESCR + 1], note[OMOBUS_MAX_NOTE + 1], 
    phone[OMOBUS_MAX_PHONE + 1];
  simple_t *_job_titles;
} NEW_CONTACT;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode;
  bool focus;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

class CJobTitlesFrame : 
  public CListFrame<IDD_JOB_TITLES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CJobTitlesFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_JOB_TITLES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CNewContactFrame : 
	public CStdDialogResizeImpl<CNewContactFrame>,
	public CUpdateUI<CNewContactFrame>,
	public CMessageFilter
{
protected:
  CDimEdit m_first_name, m_last_name, m_middle_name, m_note, m_phone;
  CHyperButtonTempl<CNewContactFrame> m_job_title;
  CLabel m_phone_s;
  HTERMINAL *m_hterm;
  NEW_CONTACT *m_doc;

public:
  CNewContactFrame(HTERMINAL *hterm, NEW_CONTACT *r) : m_hterm(hterm), m_doc(r) {
    if( !r->_job_titles->empty() ) {
      r->job_title = &(*r->_job_titles)[0];
    }
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CNewContactFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CNewContactFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CNewContactFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEdit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CNewContactFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(CreateMenuBar(IDR_MAINFRAME));
		UIAddChildWindowContainer(m_hWnd);
    m_first_name.SubclassWindow(GetDlgItem(IDC_F_NAME));
    m_first_name.SetLimitText(CALC_BUF_SIZE(m_doc->first_name));
    m_first_name.SetDimText(IDC_F_NAME);
    m_last_name.SubclassWindow(GetDlgItem(IDC_L_NAME));
    m_last_name.SetLimitText(CALC_BUF_SIZE(m_doc->last_name));
    m_last_name.SetDimText(IDC_L_NAME);
    m_middle_name.SubclassWindow(GetDlgItem(IDC_M_NAME));
    m_middle_name.SetLimitText(CALC_BUF_SIZE(m_doc->middle_name));
    m_middle_name.SetDimText(IDC_M_NAME);
    m_note.SubclassWindow(GetDlgItem(IDC_NOTE));
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc->note));
    m_note.SetDimText(IDC_NOTE);
    m_phone.SubclassWindow(GetDlgItem(IDC_PHONE));
    m_phone.SetLimitText(CALC_BUF_SIZE(m_doc->phone));
    m_phone.SetDimText(IDC_PHONE);
    m_phone_s.SubclassWindow(GetDlgItem(IDC_PHONE_S));
    m_phone_s.SetFontItalic(TRUE);
    m_job_title.SubclassWindow(GetDlgItem(IDC_JOB_TITLE));
    m_job_title.SetParent(this, IDC_JOB_TITLE);
    m_job_title.EnableWindow(m_doc->_job_titles->size() > 1);
    _SetLabel(m_job_title, m_doc->job_title);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    memset(m_doc->first_name, 0, sizeof(m_doc->first_name));
    memset(m_doc->last_name, 0, sizeof(m_doc->last_name));
    memset(m_doc->middle_name, 0, sizeof(m_doc->middle_name));
    memset(m_doc->note, 0, sizeof(m_doc->note));
    memset(m_doc->phone, 0, sizeof(m_doc->phone));
    if( m_first_name.IsWindow() ) {
      m_first_name.GetWindowText(m_doc->first_name, CALC_BUF_SIZE(m_doc->first_name));
    }
    if( m_last_name.IsWindow() ) {
      m_last_name.GetWindowText(m_doc->last_name, CALC_BUF_SIZE(m_doc->last_name));
    }
    if( m_middle_name.IsWindow() ) {
      m_middle_name.GetWindowText(m_doc->middle_name, CALC_BUF_SIZE(m_doc->middle_name));
    }
    if( m_note.IsWindow() ) {
      m_note.GetWindowText(m_doc->note, CALC_BUF_SIZE(m_doc->note));
    }
    if( m_phone.IsWindow() ) {
      m_phone.GetWindowText(m_doc->phone, CALC_BUF_SIZE(m_doc->phone));
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
    if( nID == IDC_JOB_TITLE ) {
      if( CJobTitlesFrame(m_hterm, m_doc->_job_titles, &m_doc->job_title).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_job_title, m_doc->job_title);
      }
    }
    _LockMainMenuItems();
    return true;
  }

protected:
  bool _IsDataValid() {
    return m_doc->first_name[0] != L'\0' && m_doc->last_name[0] != L'\0' && 
      m_doc->job_title != NULL;
  }

  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, _IsDataValid());
		UIUpdateToolBar();
	}

  void _SetLabel(CHyperButtonTempl<CNewContactFrame> &ctrl, SIMPLE_CONF *conf) {
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
bool WriteDocPk(HTERMINAL *h, NEW_CONTACT *r, uint64_t &doc_id, omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_NEW_CONTACT, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", r->w_cookie);
  wsrepl(xml, L"%a_cookie%", r->a_cookie);
  wsrepl(xml, L"%account_id%", fix_xml(r->account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(r->activity_type_id).c_str());
  wsrepl(xml, L"%created_dt%", wsftime(r->ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%first_name%", fix_xml(r->first_name).c_str());
  wsrepl(xml, L"%last_name%", fix_xml(r->last_name).c_str());
  wsrepl(xml, L"%middle_name%", fix_xml(r->middle_name).c_str());
  wsrepl(xml, L"%job_title_id%", fix_xml(r->job_title->id).c_str());
  wsrepl(xml, L"%phone%", fix_xml(r->phone).c_str());
  wsrepl(xml, L"%doc_note%", fix_xml(r->note).c_str());
  wsrepl(xml, L"%created_gps_dt%", wsftime(r->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(r->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(r->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_NEW_CONTACT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, NEW_CONTACT *r, uint64_t &doc_id, omobus_location_t *posClo, 
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
  wsrepl(xml, L"%doc_type%", OMOBUS_NEW_CONTACT);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(r->account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(r->activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", r->w_cookie);
  wsrepl(xml, L"%a_cookie%", r->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteJournal(HTERMINAL *h, NEW_CONTACT *doc, uint64_t &doc_id, time_t ttClo) 
{
  std::wstring txt = wsfromRes(IDS_J_NEW_CONTACT, true), templ;
  if( txt.empty() ) {
    return false;
  }
  templ = txt;
  wsrepl(txt, L"%date%", wsftime(ttClo).c_str());
  wsrepl(txt, L"%account_id%", doc->account_id);
  wsrepl(txt, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(txt, L"%first_name%", doc->first_name);
  wsrepl(txt, L"%last_name%", doc->last_name);
  wsrepl(txt, L"%job_title_id%", doc->job_title->id);
  wsrepl(txt, L"%phone%", doc->phone);
  return WriteOmobusJournal(OMOBUS_NEW_CONTACT, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
void IncrementDocumentCounter(HTERMINAL *h) {
  uint32_t count = _wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0"));
  ATLTRACE(__INT_ACTIVITY_DOCS L" = %i\n", count + 1);
  h->put_conf(__INT_ACTIVITY_DOCS, itows(count + 1).c_str());
}

static
bool CloseDoc(HTERMINAL *h, NEW_CONTACT *r) 
{
  CWaitCursor _wc;
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

bool CNewContactFrame :: _Close() 
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
  simple_t job_titles;
  NEW_CONTACT doc;
  memset(&doc, 0, sizeof(NEW_CONTACT));
  doc.ttCre = omobus_time();
  omobus_location(&doc.posCre);
  doc.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  doc.a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  doc.activity_type_id = h->get_conf(__INT_ACTIVITY_TYPE_ID, OMOBUS_UNKNOWN_ID);
  doc.account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  doc._job_titles = &job_titles;

  ReadOmobusManual(L"job_titles", &job_titles, __simple);
  ATLTRACE(L"job_titles = %i\n", job_titles.size());
  _wc.Restore();

  if( CNewContactFrame(h, &doc).DoModal(h->hParWnd) == ID_CLOSE ) {
    IncrementDocumentCounter(h);
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
  h->put_conf = put_conf;
  h->hParWnd = hParWnd;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  if( h->user_id == NULL ) {
    RETAILMSG(TRUE, (L"new_contact: >>> INCORRECT USER_ID <<<\n"));
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
    rsi(h->hWnd, _nHeight, wcsistrue(get_conf(__INT_SPLIT_ACTIVITY_SCREEN, NULL)) ?
      OMOBUS_SCREEN_ACTIVITY_TAB_1 : OMOBUS_SCREEN_ACTIVITY);
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
