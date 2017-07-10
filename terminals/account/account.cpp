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
#include <atldlgs.h>
#include <atllabel.h>
#include <atlomobus.h>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-account-terminal-element";
static int _nHeight = 44;
static std::wstring _wsTitle;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

typedef struct _tagCONTACT_CONF {
  wchar_t descr[OMOBUS_MAX_DESCR + 1], phone[OMOBUS_MAX_PHONE + 1];
  SIMPLE_CONF *jt;
  bool locked, new_contact;
} CONTACT_CONF;
typedef std::vector<CONTACT_CONF> contacts_t;

typedef struct _tagACCOUNT_CONF {
  wchar_t account_id[OMOBUS_MAX_ID + 1];
  wchar_t account[OMOBUS_MAX_DESCR + 1];
  wchar_t address[OMOBUS_MAX_ADDR_DESCR + 1];
  wchar_t chan[OMOBUS_MAX_DESCR + 1];
  wchar_t poten[OMOBUS_MAX_DESCR + 1];
  int outlet_size, cash_register;
  contacts_t *_contacts;
  simple_t *_job_titles, *_potens;
  wchar_t _c_date[11];
} ACCOUNT_CONF;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  bool focus;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontS, hFontL, hFontLS;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

class _find_simple :
  public std::unary_function<SIMPLE_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  _find_simple(const wchar_t *id) : m_id(id) {
  }
  bool operator()(SIMPLE_CONF &v) const {
    return COMPARE_ID(m_id, v.id);
  }
};

class CContactsFrame : 
	public CStdDialogResizeImpl<CContactsFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  contacts_t *m_contacts;
  CListViewCtrl m_list;
  CFont m_baseFont, m_detailFont, m_italicFont;
  
public:
  CContactsFrame(HTERMINAL *h, contacts_t *contacts) : m_hterm(h), m_contacts(contacts) {
    m_baseFont = CreateBaseFont(DRA::SCALEY(14), FW_SEMIBOLD, FALSE);
    m_detailFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_italicFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, TRUE);
  }

	enum { IDD = IDD_CONTACTS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CContactsFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CContactsFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CContactsFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_contacts->size());
    m_list.ShowWindow(m_contacts->empty()?SW_HIDE:SW_NORMAL);
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    CONTACT_CONF *ptr = &((*m_contacts)[lpdis->itemID]);
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(ptr->locked || ptr->new_contact ? m_italicFont : m_baseFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      if( ptr->locked ) {
        dc.SetTextColor(OMOBUS_ALLERTCOLOR);
      }
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd);
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(4);
    rect.bottom -= DRA::SCALEY(4);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(ptr->locked || ptr->new_contact ? m_italicFont : m_detailFont);
    rect.top += DRA::SCALEY(19);
    dc.DrawText(ptr->jt->descr, wcslen(ptr->jt->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.DrawText(ptr->phone, wcslen(ptr->phone),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.DrawLine(lpdis->rcItem.left, lpdis->rcItem.bottom - 1, 
      lpdis->rcItem.right, lpdis->rcItem.bottom - 1);

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    dc.SetTextColor(crOldTextColor);
    dc.SetBkColor(crOldBkColor);
    return 1;
  }

  LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPMEASUREITEMSTRUCT lmi = (LPMEASUREITEMSTRUCT)lParam;
    if( lmi->CtlType != ODT_LISTVIEW )
      return 0;
    lmi->itemHeight = DRA::SCALEY(42);
    return 1;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }
};

class CMainFrame : 
	public CStdDialogResizeImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, 
  public CWinDataExchange<CMainFrame>
{
protected:
  HTERMINAL *m_hterm;
  ACCOUNT_CONF *m_a;
  CLabel m_emptyLabel[4];

public:
  CMainFrame(HTERMINAL *hterm, ACCOUNT_CONF *a) : 
    m_hterm(hterm), m_a(a) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMainFrame)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_DDX_MAP(CMainFrame)
    DDX_TEXT(IDC_NAME, m_a->account)
		DDX_TEXT(IDC_ADDR, m_a->address)
    if( m_a->chan[0] != L'\0' )
      { DDX_TEXT(IDC_CHAN, m_a->chan) }
    if( m_a->poten[0] != L'\0' )
      { DDX_TEXT(IDC_POTEN, m_a->poten) }
    if( m_a->outlet_size )
      { DDX_INT(IDC_OUTLET_SIZE, m_a->outlet_size) }
    if( m_a->cash_register != NULL )
      { DDX_INT(IDC_CASH_REGISTER, m_a->cash_register) }
	END_DDX_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    _InitEmptyLabel(m_emptyLabel[0], IDC_CHAN, m_a->chan[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[1], IDC_POTEN, m_a->poten[0] != L'\0', TRUE);
    _InitEmptyLabel(m_emptyLabel[2], IDC_OUTLET_SIZE, m_a->outlet_size);
    _InitEmptyLabel(m_emptyLabel[3], IDC_CASH_REGISTER, m_a->cash_register);
    DoDataExchange(DDX_LOAD);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CContactsFrame(m_hterm, m_a->_contacts).DoModal(m_hWnd);
    return 0;
  }

private:
  void _InitEmptyLabel(CLabel &ctrl, UINT uID, BOOL bValid, BOOL bBold=FALSE) {
    ctrl.SubclassWindow(GetDlgItem(uID));
    if( !bValid )
      ctrl.SetFontItalic(TRUE);
    else if( bBold ) 
      ctrl.SetFontBold(TRUE);
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, !m_a->_contacts->empty());
		UIUpdateToolBar();
	}
};

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  AtlLoadString(uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;%group_price_id%;%pay_delay%;%locked%;%latitude%;%longitude%;%outlet_size%;%cash_register%;
static 
bool __accounts(void *cookie, int line, const wchar_t **argv, int count) 
{
  ACCOUNT_CONF *ptr = (ACCOUNT_CONF *)cookie;
  simple_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 15 ) {
    return false;
  }
  if( /*ftype*/ _ttoi(argv[2]) > 0 || !COMPARE_ID(argv[1], ptr->account_id) ) {
    return true;
  }

  COPY_ATTR__S(ptr->account, argv[4]);
  COPY_ATTR__S(ptr->address, argv[5]);
  COPY_ATTR__S(ptr->chan, argv[6]);
  ptr->outlet_size = _wtoi(argv[13]);
  ptr->cash_register = _wtoi(argv[14]);

  if( (it = std::find_if(ptr->_potens->begin(), ptr->_potens->end(), _find_simple(argv[7])))
        != ptr->_potens->end() ) {
    COPY_ATTR__S(ptr->poten, it->descr);
  }

  return false;
}

// Format: %contact_id%;%account_id%;%descr%;%job_title_id%;%phone%;%locked%;
static 
bool __contacts(void *cookie, int line, const wchar_t **argv, int count) 
{
  ACCOUNT_CONF *ptr = (ACCOUNT_CONF *)cookie;
  CONTACT_CONF cnf = {0};
  simple_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 6 ) {
    return false;
  }
  if( !COMPARE_ID(argv[1], ptr->account_id) ) {
    return true;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.descr, argv[2]);
  if( (it = std::find_if(ptr->_job_titles->begin(), ptr->_job_titles->end(), _find_simple(argv[3])))
        != ptr->_job_titles->end() ) {
    cnf.jt = &(*it);
  }
  COPY_ATTR__S(cnf.phone, argv[4]);
  cnf.locked = wcsistrue(argv[5]);
  ptr->_contacts->push_back(cnf);

  return true;
}

// Format: %date%;%account_id%;%doc_id%;%first_name% %last_name%;%job_title_id%;%phone%;
static 
bool __new_contacts(void *cookie, int line, const wchar_t **argv, int count) 
{
  ACCOUNT_CONF *ptr = (ACCOUNT_CONF *)cookie;
  CONTACT_CONF cnf = {0};
  simple_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 6 ) {
    return false;
  }
  if( wcsncmp(ptr->_c_date, argv[0], 10) != 0 ||
      !COMPARE_ID(argv[1], ptr->account_id) ) {
    return true;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.descr, argv[3]);
  if( (it = std::find_if(ptr->_job_titles->begin(), ptr->_job_titles->end(), _find_simple(argv[4])))
        != ptr->_job_titles->end() ) {
    cnf.jt = &(*it);
  }
  COPY_ATTR__S(cnf.phone, argv[5]);
  cnf.new_contact = true;
  ptr->_contacts->push_back(cnf);

  return true;
}

// Format: %id%;%descr%;
static 
bool __simple(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  SIMPLE_CONF cnf;
  if( ln == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 2 ) {
    return false;
  }
  memset(&cnf, 0, sizeof(SIMPLE_CONF));
  COPY_ATTR__S(cnf.id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  ((simple_t *)cookie)->push_back(cnf);
  return true;
}

static
const wchar_t *accountName(HTERMINAL *h) {
  const wchar_t *val = h->get_conf(__INT_ACCOUNT, NULL);
  return val == NULL ?_wsTitle.c_str() : val;
}

static
bool accountLocked(HTERMINAL *h) {
  const wchar_t *val = h->get_conf(__INT_ACCOUNT_LOCKED, NULL);
  return val == NULL ? false : (_wtoi(val) != 0);
}

static
const wchar_t * addrName(HTERMINAL *h) {
  return h->get_conf(__INT_ACCOUNT_ADDR, L"");
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(1), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(1) };
  bool locked = accountLocked(h);
  HFONT hOldFont = (HFONT)SelectObject(hDC, locked ? h->hFontL : h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    if( locked ) {
      SetTextColor(hDC, OMOBUS_ALLERTCOLOR);
    }
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, accountName(h), -1, &rect, DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|
    DT_WORD_ELLIPSIS);
  rect.top += DRA::SCALEY(14);
  SelectObject(hDC, locked ? h->hFontLS : h->hFontS);
  DrawText(hDC, addrName(h), -1, &rect, DT_LEFT|DT_WORDBREAK|DT_NOPREFIX|
    DT_WORD_ELLIPSIS);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) 
{
  ACCOUNT_CONF a;
  simple_t job_titles, potens;
  contacts_t contacts;

  memset(&a, 0, sizeof(a));
  COPY_ATTR__S(a.account_id, h->get_conf(__INT_ACCOUNT_ID, NULL));
  COPY_ATTR__S(a.account, accountName(h));
  COPY_ATTR__S(a.address, addrName(h));
  a._contacts = &contacts;
  a._job_titles = &job_titles;
  a._potens = &potens;
  wcsncpy(a._c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(a._c_date));

  ReadOmobusManual(L"potentials", &potens, __simple);
  ReadOmobusManual(L"job_titles", &job_titles, __simple);
  ReadOmobusJournal(OMOBUS_NEW_CONTACT, &a, __new_contacts);
  ReadOmobusManual(L"contacts", &a, __contacts);
  ReadOmobusManual(L"accounts", &a, __accounts);

  CMainFrame(h, &a).DoModal(h->hParWnd);
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
  pf_terminals_put_conf put_conf, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));

  h->get_conf = get_conf;
  h->hParWnd = hParWnd;

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontS = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  h->hFontL = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, TRUE);
  h->hFontLS = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  _wsTitle = wsfromRes(IDS_TITLE);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ACTIVITY|OMOBUS_SCREEN_ACTIVITY_TAB_1);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontL);
  chk_delete_object(h->hFontS);
  chk_delete_object(h->hFontLS);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
