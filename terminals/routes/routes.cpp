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
#include <atldlgs.h>
#include <atllabel.h>
#include <atlomobus.h>
#include <atllabel.h>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>

#include "resource.h"
#include "../../version"

#define OMOBUS_ACTMGR_RADIUS           L"activity->radius"
#define OMOBUS_ACTMGR_RADIUS_DEF       L"500"
#define OMOBUS_ACTMGR_BEGIN            L"activity->begin"
#define OMOBUS_ACTMGR_BEGIN_DEF        L""
#define OMOBUS_ACTMGR_END              L"activity->end"
#define OMOBUS_ACTMGR_END_DEF          L""
#define OMOBUS_HEIGHT                  L"routes->height"
#define OMOBUS_HEIGHT_DEF              L"243"

typedef std::vector<std::wstring> slist_t;
typedef std::vector<std::wstring> my_t;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

struct ACTIVITY_TYPE_CONF {
  wchar_t activity_type_id[OMOBUS_MAX_ID+1];
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  wchar_t note[OMOBUS_MAX_NOTE+1];
  byte_t docs; // Обязательно создание документа.
  byte_t pos; // Открывать только при наличии актуальной позиции.
  int32_t exec_limit; // Лимит выполненных активностей.
  int32_t pending_limit; // Лимит отложенных активностей
};
typedef std::vector<ACTIVITY_TYPE_CONF> activity_types_t;

struct ROUTE_CONF {
  int32_t freq_c, color, bgcolor;
  bool allow_pending, pending;
  wchar_t route_date[OMOBUS_MAX_DATE + 1];
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  wchar_t address[OMOBUS_MAX_ADDR_DESCR+1];
  wchar_t chan[OMOBUS_MAX_DESCR + 1];
  wchar_t poten[OMOBUS_MAX_DESCR + 1];
  short pay_delay;
  wchar_t group_price_id[OMOBUS_MAX_ID+1];
  byte_t locked;
  ACTIVITY_TYPE_CONF *at;
};
typedef std::vector<ROUTE_CONF> routes_t;

typedef struct _tagPENDING {
  const wchar_t *account_id, *activity_type_id;
  omobus_location_t posCre;
  time_t ttCre;
  SIMPLE_CONF type;
  wchar_t note[OMOBUS_MAX_NOTE + 1];
  wchar_t route_date[OMOBUS_MAX_DATE + 1];
} PENDING;

struct ACCOUNT_CONF {
  int _i, _l, _vf;
  wchar_t pid[OMOBUS_MAX_ID+1];
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t code[OMOBUS_MAX_ID+1];
  byte_t ftype;
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  wchar_t address[OMOBUS_MAX_ADDR_DESCR+1];
  wchar_t chan[OMOBUS_MAX_DESCR + 1];
  wchar_t poten[OMOBUS_MAX_DESCR + 1];
  short pay_delay;
  wchar_t group_price_id[OMOBUS_MAX_ID+1];
  byte_t locked;
  byte_t my;
  byte_t closely;
  byte_t closed;
  byte_t route;
};
typedef std::vector<ACCOUNT_CONF*> accounts_t;

struct USER_ACTIVITY_CONF {
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t activity_type_id[OMOBUS_MAX_ID+1];
  int32_t freq_c;
};
typedef std::vector<USER_ACTIVITY_CONF> user_activities_t;

struct CURRENT_ACTIVITY_CONF {
  wchar_t a_cookie[OMOBUS_MAX_ID + 1], a_name[OMOBUS_MAX_DESCR + 1];
  wchar_t account_id[OMOBUS_MAX_ID+1], activity_type_id[OMOBUS_MAX_ID+1];
  wchar_t descr[OMOBUS_MAX_DESCR+1], address[OMOBUS_MAX_ADDR_DESCR+1];
  short pay_delay;
  wchar_t group_price_id[OMOBUS_MAX_ID+1];
  byte_t locked;
  time_t ttBegin;
  omobus_location_t posBegin;
  wchar_t route_date[OMOBUS_MAX_DATE + 1];
  byte_t docs;
};

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf; 
  HWND hWnd0, hWnd1, hWnd2, hWndL, hParWnd;
  HFONT hFont, hFontE, hFontN, hFontB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  bool focus0, focus1, focus2;
  const wchar_t *user_id, *rt_begin, *rt_end, *pk_ext, *pk_mode,
    *act_begin, *act_end, *w_cookie, *date_fmt, *datetime_fmt;
  size_t radius;
  CURRENT_ACTIVITY_CONF cur_activity;
} HTERMINAL;

struct _find_simple : public std::unary_function<SIMPLE_CONF, bool> {
  _find_simple(const wchar_t *id) : m_id(id) {
  }
  bool operator()(SIMPLE_CONF &v) const {
    return COMPARE_ID(m_id, v.id);
  }
private:
  const wchar_t *m_id;
};

struct _find_activity_type : public std::unary_function <ACTIVITY_TYPE_CONF&, bool> { 
  _find_activity_type(const wchar_t *at_id_) : at_id(at_id_) {
  }
  bool operator()(const ACTIVITY_TYPE_CONF &v) { 
    return COMPARE_ID(at_id, v.activity_type_id);
  }
private:
  const wchar_t *at_id;
};

struct _find_account_id : public std::unary_function <ROUTE_CONF&, bool> { 
  _find_account_id(const wchar_t *account_id_) : account_id(account_id_) {
  }
  bool operator()(const ROUTE_CONF &v) { 
    return COMPARE_ID(account_id, v.account_id);
  }
private:
  const wchar_t *account_id;
};

struct _find_route : public std::unary_function <ROUTE_CONF&, bool> { 
  _find_route(const wchar_t *account_id_, const wchar_t *activity_type_id_) : 
    account_id(account_id_), activity_type_id(activity_type_id_) {
  }
  bool operator()(const ROUTE_CONF &v) { 
    return COMPARE_ID(account_id, v.account_id) &&
      v.at != NULL && COMPARE_ID(activity_type_id, v.at->activity_type_id);
  }
private:
  const wchar_t *account_id, *activity_type_id;
};

struct _find_user_activity : public std::unary_function <USER_ACTIVITY_CONF, bool> { 
  const wchar_t *account_id, *activity_type_id;
  _find_user_activity(const wchar_t *account_id_, const wchar_t *activity_type_id_) : 
    account_id(account_id_), activity_type_id(activity_type_id_) {
  }
  bool operator()(const USER_ACTIVITY_CONF &v) { 
    return 
      COMPARE_ID(account_id, v.account_id) &&
      (activity_type_id == NULL || COMPARE_ID(activity_type_id, v.activity_type_id));
  }
};

struct __account_cookie { 
  accounts_t *l; my_t *my; user_activities_t *ua; simple_t *potens;
  omobus_location_t *gpsPos; int32_t radius; 
};

struct __user_activity_cookie { 
  const wchar_t *c_date; user_activities_t *l; 
};

inline bool IsPending(ROUTE_CONF &r) {
  return r.pending;
}

inline double dist(double startLat, double startLong, double endLat, double endLong) {
  return 1000 * 111.2 * acos(sin(startLat) * sin(endLat) + cos(startLat) * 
    cos(endLat) * cos(startLong - endLong));
}

static bool IsEqual(const wchar_t *src, slist_t &l);
static bool IsEqual2(const wchar_t *src1, const wchar_t *src2, slist_t &l);
static slist_t &_parce_slist(const wchar_t *s, slist_t &sl);


static CAppModule _Module;
static TCHAR _szWindowClass0[] = L"omobus-routes0-terminal-element",
  _szWindowClass1[] = L"omobus-routes1-terminal-element",
  _szWindowClass2[] = L"omobus-routes2-terminal-element";
static int _nHeight0 = 20, _nHeight1 = 0, _nHeight2 = 20;
static LVCOLUMN _lvc = { 0 };
static routes_t _route;
static activity_types_t _at;
static simple_t _potens;
static wchar_t _c_date[OMOBUS_MAX_DATE + 1] = L"", _closed[3] = L"";
static wchar_t _sel_account_id[OMOBUS_MAX_ID+1] = L"";

class CPendingTypesFrame : 
  public CListFrame<IDD_PENDING_TYPES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CPendingTypesFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_PENDING_TYPES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CPendingDlg : 
	public CStdDialogResizeImpl<CPendingDlg>,
  public CUpdateUI<CPendingDlg>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  ROUTE_CONF *m_route;
  CLabel m_status;
  CDimEdit m_note;
  CHyperButtonTempl<CPendingDlg> m_type;
  simple_t m_t;
  SIMPLE_CONF *m_cur_t;
  PENDING m_doc;

public:
  CPendingDlg(HTERMINAL *hterm, ROUTE_CONF *route) : 
      m_hterm(hterm), m_route(route), m_cur_t(NULL) {
    memset(&m_doc, 0, sizeof(PENDING));
    wcsncpy(m_doc.route_date, route->route_date, CALC_BUF_SIZE(m_doc.route_date));
    m_doc.account_id = route->account_id;
    m_doc.activity_type_id = route->at->activity_type_id;
    _Load();
  }

	enum { IDD = IDD_PENDINGDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CPendingDlg)
    UPDATE_ELEMENT(ID_MENU_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CPendingDlg)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CPendingDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_CLOSE, OnClose)
    COMMAND_ID_HANDLER(ID_MENU_BACK, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnIdle)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CPendingDlg>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    CString m_cap;
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ID_PENDING_MENU));
    UIAddChildWindowContainer(m_hWnd);
    GetWindowText(m_cap); 
    m_cap += CString(m_route->at->descr).MakeLower();
    SetWindowText(m_cap);
    SetDlgItemText(IDC_ACCOUNT, m_route->descr);
    SetDlgItemText(IDC_DEL_ADDR, m_route->address);
    m_note.SubclassWindow(GetDlgItem(IDC_EDIT_0));
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc.note));
    m_note.SetDimText(IDC_EDIT_0);
    m_type.SubclassWindow(GetDlgItem(IDC_TYPE));
    m_type.SetParent(this, IDC_TYPE);
    m_type.EnableWindow(!m_t.empty());
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
      if( MessageBoxEx(m_hWnd, IDS_STR8, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
        if( _Close() ) {
          MessageBoxEx(m_hWnd, IDS_STR9, IDS_INFO, MB_ICONINFORMATION);
          EndDialog(ID_MENU_CLOSE);
        } else {
          MessageBoxEx(m_hWnd, IDS_STR10, IDS_ERROR, MB_ICONSTOP);
        }
      }
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( !_IsDataValid() || MessageBoxEx(m_hWnd, IDS_STR11, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES )
      EndDialog(ID_MENU_BACK);
    return 0;
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( nID == IDC_TYPE ) {
      if( CPendingTypesFrame(m_hterm, &m_t, &m_cur_t).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_type, m_cur_t);
      }
    }
    _DoDataExchange();
    _LockMainMenuItems();
    return true;
  }

private:
  bool _IsDataValid() {
    return m_cur_t != NULL || m_doc.note[0] != L'\0';
  }

  void _LockMainMenuItems() {
    UIEnable(ID_MENU_CLOSE, _IsDataValid());
    UIUpdateToolBar();
	}

  void _SetLabel(CHyperButtonTempl<CPendingDlg> &ctrl, SIMPLE_CONF *conf) {
    if( conf == NULL )
      return;
    CString t = L"["; t += conf->descr; t += L"]";
    ctrl.SetLabel(t);
  }

  void _DoDataExchange() {
    memset(m_doc.note, 0, sizeof(m_doc.note));
    m_cur_t != NULL ? memcpy(&m_doc.type, m_cur_t, sizeof(SIMPLE_CONF)) : memset(&m_doc.type, 0, sizeof(SIMPLE_CONF));
    if( m_note.IsWindow() ) {
      m_note.GetWindowText(m_doc.note, CALC_BUF_SIZE(m_doc.note));
    }
  }

  bool _Close();
  void _Load();
};

class CMainDlg : 
	public CStdDialogResizeImpl<CMainDlg>,
  public CUpdateUI<CMainDlg>,
	public CMessageFilter,
  public CWinDataExchange<CMainDlg>
{
private:
  HTERMINAL *m_hterm;
  ROUTE_CONF *m_route;
  CLabel m_emptyLabel[3];
  wchar_t m_route_date_L[16];

public:
  CMainDlg(HTERMINAL *hterm, ROUTE_CONF *route) : m_hterm(hterm), m_route(route) {
    memset(m_route_date_L, 0, sizeof(m_route_date_L));
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
    UPDATE_ELEMENT(ID_MENU_START, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
    UPDATE_ELEMENT(ID_MENU_PENDING, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CMainDlg)
  END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP(CMainDlg)
		DDX_TEXT(IDC_ACCOUNT, m_route->descr)
    DDX_TEXT(IDC_DEL_ADDR, m_route->address)
    if( m_route->chan[0] != L'\0' )
      { DDX_TEXT(IDC_CHAN, m_route->chan) }
    if( m_route->poten[0] != L'\0' )
      { DDX_TEXT(IDC_POTEN, m_route->poten) }
    DDX_TEXT(IDC_PENDING, m_route_date_L)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_START, OnStart)
    COMMAND_ID_HANDLER(ID_MENU_PENDING, OnPending)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMainDlg>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    tm t;
    wcsftime(m_route_date_L, CALC_BUF_SIZE(m_route_date_L), m_hterm->date_fmt,
      date2tm(m_route->route_date, &t));
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ID_MAINDLG_MENU, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    SetWindowText(m_route->at->descr);
    _InitEmptyLabel(m_emptyLabel[0], IDC_CHAN, m_route->chan[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[1], IDC_POTEN, m_route->poten[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[2], IDC_STATE, m_route->freq_c || m_route->pending, 
        TRUE, m_route->freq_c ? IDS_STR6 : IDS_STR7);
    DoDataExchange(DDX_LOAD);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
		return 0;
	}

  LRESULT OnStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( m_route->freq_c >=  m_route->at->exec_limit ) {
      MessageBoxEx(m_hWnd, IDS_STR4, IDS_WARN, MB_ICONSTOP);
      return 0;
    }
    if( m_route->locked ) {
      if( MessageBoxEx(m_hWnd, IDS_STR5, IDS_WARN, MB_ICONWARNING|MB_YESNO) == IDNO ) {
        return 0;
      }
    }
    EndDialog(IDOK);
		return 0;
	}

  LRESULT OnPending(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ShowWindow(SW_HIDE);
    if( CPendingDlg(m_hterm, m_route).DoModal(m_hWnd) == ID_MENU_BACK ) {
      ShowWindow(SW_NORMAL);
    } else {
      m_route->pending = true;
      EndDialog(IDCANCEL);
    }
		return 0;
	}

private:
  void _LockMainMenuItems() {
    BOOL b = m_route->freq_c >= m_route->at->exec_limit;
    UIEnable(ID_MENU_START, !b);
    UIEnable(ID_MENU_PENDING, !b && m_route->allow_pending && m_route->freq_c == 0 && !m_route->pending && 
      std::count_if(_route.begin(), _route.end(), IsPending) < m_route->at->pending_limit);
    UIUpdateToolBar();
	}

  void _InitEmptyLabel(CLabel &ctrl, UINT uID, BOOL bValid, BOOL bBold=FALSE, UINT tID=-1) {
    ctrl.SubclassWindow(GetDlgItem(uID));
    if( !bValid ) {
      ctrl.SetFontItalic(TRUE);
    } else if( bBold ) {
      ctrl.SetFontBold(TRUE);
      if( tID != (UINT)-1 ) {
        ctrl.SetText(LoadStringEx(tID));
      }
    }
  }
};

class CTypesFrame : 
	public CStdDialogResizeImpl<CTypesFrame>,
	public CUpdateUI<CTypesFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  ACCOUNT_CONF *m_ac;
  activity_types_t *m_at;
  user_activities_t *m_ua;
  CListViewCtrl m_list;
  CLabel m_pos;
  CStatic m_time;
  CFont m_boldFont, m_baseFont;

public:
  CTypesFrame(HTERMINAL *hterm, ACCOUNT_CONF *ac, activity_types_t *at, user_activities_t *ua) : 
      m_hterm(hterm), m_ac(ac), m_at(at), m_ua(ua) {
    m_baseFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(14), FW_BOLD, FALSE);
  }

	enum { IDD = IDD_TYPES };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CTypesFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CTypesFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CTypesFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CTypesFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_time = GetDlgItem(IDC_TIME);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_at->size());
    m_list.ShowWindow(m_at->empty()?SW_HIDE:SW_NORMAL);
    m_pos.SubclassWindow(GetDlgItem(IDC_POS));
    m_pos.SetFontBold(TRUE);
    _LockMainMenuItems();
    _RedrawStatus();
    SetTimer(1, 5000, NULL);
		return bHandled = FALSE;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    rect.top = GetTitleHeight() + DRA::SCALEY(30);
    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.SelectPen(oldPen);
    DoPaintTitle();
		return FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW ) {
      return 0;
    }

    ACTIVITY_TYPE_CONF *ptr = &((*m_at)[lpdis->itemID]);
    if( ptr == NULL ) {
      return 0;
    }

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_boldFont);
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
    rect.bottom = rect.top + DRA::SCALEY(16);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(24), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    if( ptr->pos ) {
      dc.DrawText(LoadStringEx(IDS_STR7), -1,
        rect.right - DRA::SCALEX(24), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    dc.SelectFont(m_baseFont);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.DrawText(ptr->note, wcslen(ptr->note),
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(15), rect.bottom,
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
    _Next(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Next(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    _RedrawStatus();
    return 0;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
  }

  void _Next(size_t sel) {
    if( sel >= m_at->size() ) {
      return;
    }
    ACTIVITY_TYPE_CONF *at = &((*m_at)[sel]);
    if( !_CheckPos(at) ) {
      return;
    }
    if( !_ChekLimit(at) ) {
      return;
    }
    COPY_ATTR__S(m_hterm->cur_activity.activity_type_id, at->activity_type_id);
    COPY_ATTR__S(m_hterm->cur_activity.a_name, at->descr);
    m_hterm->cur_activity.docs = at->docs;
    EndDialog(ID_ACTION);
  }

  bool _CheckPos(ACTIVITY_TYPE_CONF *at) {
    m_hterm->cur_activity.ttBegin = omobus_time();
    omobus_location(&m_hterm->cur_activity.posBegin);
    if( !at->pos || m_hterm->cur_activity.posBegin.location_status ) {
      return true;
    }
    if( MessageBoxEx(m_hWnd, IDS_STR8, IDS_ERROR, MB_ICONWARNING|MB_YESNO) == IDNO ) {
      return true;
    }
    return false;
  }

  bool _ChekLimit(ACTIVITY_TYPE_CONF *at) {
    user_activities_t::iterator it;
    if( m_ua->end() != (it = std::find_if(m_ua->begin(), m_ua->end(), 
          _find_user_activity(m_ac->account_id, at->activity_type_id))) ) {
      if( it->freq_c >= at->exec_limit ) {
        MessageBoxEx(m_hWnd, IDS_STR4, IDS_ERROR, MB_ICONSTOP);
        return false;
      }
    }
    return true;
  }

  void _RedrawStatus() {
    omobus_location_t l; omobus_location(&l);
    m_pos.SetText(LoadStringEx(
      l.location_status == 1 ? IDS_POS_YES :
        (l.location_status == 2 ? IDS_POS_OLD : IDS_POS_NO)
      ));
    m_time.SetWindowText(wsftime(omobus_time(), m_hterm->datetime_fmt).c_str());
  }
};

class CTreeFrame : 
	public CStdDialogResizeImpl<CTreeFrame>,
  public CUpdateUI<CTreeFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  accounts_t *m_cur_ac;
  ACCOUNT_CONF *m_cur_sel;
  CString m_fmt0, m_tmp;
  CListViewCtrl m_list;
  CFont m_baseFont, m_folderFont;

public:
  CTreeFrame(HTERMINAL *h, accounts_t *cur_ac, ACCOUNT_CONF *cur_sel) : 
    m_hterm(h), m_cur_ac(cur_ac), m_cur_sel(cur_sel) {
    m_folderFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_TREE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CTreeFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CTreeFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CTreeFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CTreeFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_fmt0.LoadStringW(IDS_FMT0);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_cur_ac->size());
    m_list.SelectItem(_GetSel(m_cur_sel));
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    ACCOUNT_CONF *ptr = (*m_cur_ac)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_folderFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      dc.SetTextColor(ptr->_vf ? RGB(0, 0, 140) : crOldTextColor);
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd);
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(2);
    rect.bottom = rect.top + DRA::SCALEY(18);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2+8*ptr->_l), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);
    m_tmp.Format(m_fmt0, ptr->_i);
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.DrawText(m_tmp, m_tmp.GetLength(), 
      rect.left + DRA::SCALEX(2+8*ptr->_l), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
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
    lmi->itemHeight = DRA::SCALEY(35);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Ret(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Ret(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _CloseDialog(wID);
    return 0;
  }

  ACCOUNT_CONF *GetSelElem() {
    return m_cur_sel;
  }

protected:
  void _Ret(size_t cur) {
    if( cur >= m_cur_ac->size() )
      return;
    m_cur_sel = (*m_cur_ac)[cur];
    _CloseDialog(ID_ACTION);
  }

  int _GetSel(ACCOUNT_CONF *p) {
    size_t size = m_cur_ac->size(), i = 0;
    for( i = 0; i < size; i++ ) {
      if( m_cur_sel == (*m_cur_ac)[i] )
        return i;
    }
    return -1;
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
  }

  void _CloseDialog(WORD wID) {
    EndDialog(wID);
  }
};

class CAccountsFrame : 
	public CStdDialogResizeImpl<CAccountsFrame>,
  public CUpdateUI<CAccountsFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  accounts_t *m_ac, m_cur_ac;
  activity_types_t *m_at;
  user_activities_t *m_ua;
  bool m_closely, m_my;
  ACCOUNT_CONF *m_parent, m_vf[3];
  CHyperButtonTempl<CAccountsFrame> m_vfctrl;
  CListViewCtrl m_list;
  CDimEdit m_name_ff, m_code_ff;
  CFont m_baseFont, m_boldFont, m_lockedFont;
  CString m_subcap;

public:
  CAccountsFrame(HTERMINAL *hterm, accounts_t *ac, activity_types_t *at, user_activities_t *ua, 
      bool my) : m_hterm(hterm), m_ac(ac), m_at(at), m_ua(ua), m_my(my) {
    m_parent = &m_vf[0];
    memset(&m_vf, 0, sizeof(m_vf));
    m_vf[0]._vf = m_vf[1]._vf = m_vf[2]._vf = 1;
    m_closely = _IsClosely();
    m_parent = m_closely ? &m_vf[2] : (m_my ? &m_vf[1] : &m_vf[0]);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_lockedFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  }

	enum { IDD = IDD_ACCOUNTS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CAccountsFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CAccountsFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CAccountsFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnEditColor)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
    COMMAND_HANDLER(IDC_CODE_FF, EN_CHANGE, OnFilter)
    COMMAND_HANDLER(IDC_NAME_FF, EN_CHANGE, OnFilter)
    NOTIFY_HANDLER(IDC_LIST, LVN_ODFINDITEM, OnFindItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CAccountsFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL));
		UIAddChildWindowContainer(m_hWnd);
    COPY_ATTR__S(m_vf[0].descr, LoadStringEx(IDS_GROUP_ALL));
    COPY_ATTR__S(m_vf[1].descr, LoadStringEx(IDS_GROUP_OWN));
    COPY_ATTR__S(m_vf[2].descr, LoadStringEx(IDS_GROUP_CLOSELY));
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_vfctrl.SubclassWindow(GetDlgItem(IDC_VF));
    m_vfctrl.SetParent(this, IDC_VF);
    m_code_ff.SubclassWindow(GetDlgItem(IDC_CODE_FF));
    m_code_ff.SetDimText(IDS_CODE_FF);
    m_code_ff.SetFlatMode();
    m_name_ff.SubclassWindow(GetDlgItem(IDC_NAME_FF));
    m_name_ff.SetDimText(IDS_NAME_FF);
    m_name_ff.SetFlatMode();
    _Reload();
    _InitVfSelector();
		return bHandled = FALSE;
	}

  LRESULT OnEditColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
    CDCHandle dc((HDC)wParam);
    if( dc.IsNull() ) {
      bHandled = FALSE;
      return 0;
    }
    dc.SetTextColor( (COLORREF) RGB(0, 0, 200) );
    dc.SetBkMode(TRANSPARENT);
    bHandled = TRUE;
    return (LRESULT)m_hterm->hbrN;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    rect.top = GetTitleHeight() + DRA::SCALEY(26);
    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    rect.top += DRA::SCALEY(22);
    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.SelectPen(oldPen);
    DoPaintTitle();
		return FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    ACCOUNT_CONF *ptr = m_cur_ac[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(ptr->locked ? m_lockedFont : m_boldFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      dc.SetTextColor(ptr->closed ? RGB(150, 150, 150) : 
        (ptr->locked ? OMOBUS_ALLERTCOLOR : crOldTextColor));
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd);
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(2);
    rect.bottom = rect.top + DRA::SCALEY(15);
    dc.DrawText(ptr->descr, wcslen(ptr->descr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);
    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(28);
    dc.DrawText(ptr->address, wcslen(ptr->address), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK|DT_END_ELLIPSIS);

    rect.top = rect.bottom + DRA::SCALEY(2);
    rect.bottom = lpdis->rcItem.bottom;
    rect.right = DRA::SCALEX(160);
    dc.DrawText(ptr->chan, wcslen(ptr->chan), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.left = rect.right + DRA::SCALEX(10);
    rect.right = rect.left + DRA::SCALEX(35);
    dc.DrawText(ptr->poten, wcslen(ptr->poten), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    if( ptr->closed ) {
      rect.left = rect.right;
      rect.right = lpdis->rcItem.right - DRA::SCALEX(2) - scrWidth;
      dc.SelectFont(m_boldFont);
      dc.DrawText(L"+", 1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
      dc.SelectFont(m_baseFont);
    }

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
    lmi->itemHeight = DRA::SCALEY(65);
    return 1;
  }

  LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    KillTimer(1);
    if( wParam == 1 )
      _Reload();
		return 0;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Next(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    LPNMLVFINDITEM fi = (LPNMLVFINDITEM)pnmh;
    if( fi->lvfi.flags & LVFI_STRING ) {
      size_t size = m_cur_ac.size(), i = fi->iStart;
      for( ; i < size; i++ ) {
        if( _wcsnicmp(m_cur_ac[i]->descr, fi->lvfi.psz, wcslen(fi->lvfi.psz)) == 0 )
          return i;
      }
    }
    return -1;
  }

  LRESULT OnFilter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    KillTimer(1);
    SetTimer(1, 1200);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Next(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    KillTimer(1);
    EndDialog(wID);
    return 0;
  }

  bool OnNavigate(HWND /*hWnd*/, UINT /*nID*/) {
    KillTimer(1);
    _Hierarchy();
    return true;
  }

  const wchar_t *GetSubTitle() {
    m_subcap.Format(IDS_FMT1, m_cur_ac.size());
    return m_subcap;
  }

protected:
  void _Reload() {
    _Reload(_CodeFF(), _NameFF());
    Invalidate(FALSE);
  }

  void _Reload(slist_t &code_ff, slist_t &name_ff) {
    CWaitCursor _wc;
    m_cur_ac.clear();
    m_list.SetItemCount(m_cur_ac.size());
    size_t size = m_ac->size();
    for( size_t i = 0; i < size; i++ ) {
      ACCOUNT_CONF *ptr = (*m_ac)[i];
      if( ptr->ftype || m_parent == NULL )
        continue;
      if( !( code_ff.empty() || IsEqual(ptr->code, code_ff)) )
        continue;
      if( !( name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) )
        continue;
      if( m_parent == &m_vf[0] ) { // ALL
        // any client
      } else if( m_parent == &m_vf[1] ) { // OWN
        if( !ptr->my )
          continue;
      } else if( m_parent == &m_vf[2] ) { // CLOSELY
        if( !ptr->closely )
          continue;
      } else { // NATIVE FOLDER
        if( !COMPARE_ID(m_parent->account_id, ptr->pid) )
          continue;
      }
      m_cur_ac.push_back(ptr);
    }
    m_list.SetItemCount(m_cur_ac.size());
    m_list.ShowWindow(m_cur_ac.empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
  }

  void _Tree(accounts_t &ac_tree, ACCOUNT_CONF *parent, int level, slist_t &code_ff, slist_t &name_ff) {
    size_t size = m_ac->size(), i = 0;
    for( i = 0; i < size; i++ ) {
      ACCOUNT_CONF *ptr = (*m_ac)[i];
      if( !ptr->ftype )
        continue;
      if( parent == NULL ) {
        if( !COMPARE_ID(OMOBUS_UNKNOWN_ID, ptr->pid) )
          continue;
      } else {
        if( !COMPARE_ID(parent->account_id, ptr->pid) )
          continue;
      }
      _Calc(ptr, code_ff, name_ff);
      if( ptr->_i == 0 )
        continue;
      ptr->_l = level;
      ac_tree.push_back(ptr);
      _Tree(ac_tree, ptr, level + 1, code_ff, name_ff);
    }    
  }

  void _Calc(ACCOUNT_CONF *a, slist_t &code_ff, slist_t &name_ff) {
    a->_i = 0;
    size_t size = m_ac->size();
    for( size_t i = 0; i < size; i++ ) {
      ACCOUNT_CONF *ptr = (*m_ac)[i];
      if( !( ptr->ftype || code_ff.empty() || IsEqual(ptr->code, code_ff)) )
        continue;
      if( !( ptr->ftype || name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) )
        continue;
      if( a == &m_vf[0] ) { // ALL
        if( !ptr->ftype )
          a->_i++;
      } else if( a == &m_vf[1] ) { // OWN
        if( !ptr->ftype && ptr->my )
          a->_i++;
      } else if( a == &m_vf[2] ) { // CLOSELY
        if( !ptr->ftype && ptr->closely )
          a->_i++;
      } else { // NATIVE FOLDER
        if( COMPARE_ID(a->account_id, ptr->pid) ) {
          if( ptr->ftype ) {
            _Calc(ptr, code_ff, name_ff);
            a->_i += ptr->_i;
          } else {
            a->_i++;
          }
        }
      }
    }
  }

  slist_t _CodeFF() {
    slist_t sl; CString ff;
    m_code_ff.GetWindowText(ff);
    return _parce_slist(ff, sl);
  }

  slist_t _NameFF() {
    slist_t sl; CString ff; 
    m_name_ff.GetWindowText(ff);
    return _parce_slist(ff, sl);
  }

  void _Next(size_t cur) {
    ACCOUNT_CONF *ac = NULL;
    if( cur >= m_cur_ac.size() || (ac = m_cur_ac[cur]) == NULL ) {
      return;
    }
    if( ac->ftype ) {
      m_parent = m_cur_ac[cur];
      _Reload();
    } else {
      if( ac->locked && 
          MessageBoxEx(m_hWnd, IDS_STR5, IDS_INFO, MB_ICONQUESTION|MB_YESNO) != IDYES ) {
        return;
      }
      ShowWindow(SW_HIDE);
      if( CTypesFrame(m_hterm, ac, m_at, m_ua).DoModal(m_hWnd) == ID_ACTION ) {
        COPY_ATTR__S(m_hterm->cur_activity.account_id, ac->account_id);
        COPY_ATTR__S(m_hterm->cur_activity.descr, ac->descr);
        COPY_ATTR__S(m_hterm->cur_activity.address, ac->address);
        COPY_ATTR__S(m_hterm->cur_activity.group_price_id, ac->group_price_id);
        m_hterm->cur_activity.locked = ac->locked;
        m_hterm->cur_activity.pay_delay = ac->pay_delay;
        KillTimer(1);
        EndDialog(ID_ACTION);
      } else {
        ShowWindow(SW_SHOWNORMAL);
      }
    }
  }

  void _Hierarchy() {
    CWaitCursor _wc;
    accounts_t ac_tree;
    slist_t code_ff = _CodeFF(), name_ff = _NameFF();
    _Calc(&m_vf[0], code_ff, name_ff);
    ac_tree.push_back(&m_vf[0]);
    if( m_my ) {
      _Calc(&m_vf[1], code_ff, name_ff);
      ac_tree.push_back(&m_vf[1]);
    }
    if( m_closely ) {
      _Calc(&m_vf[2], code_ff, name_ff);
      ac_tree.push_back(&m_vf[2]);
    }
    _Tree(ac_tree, NULL, 0, code_ff, name_ff);
    _wc.Restore();
    if( !ac_tree.empty() ) {
      CTreeFrame dlg(m_hterm, &ac_tree, m_parent);
      if( dlg.DoModal(m_hWnd) == ID_ACTION ) {
        m_parent = dlg.GetSelElem();
        _Reload(code_ff, name_ff);
        _InitVfSelector();
      }
    }
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
  }

  void _InitVfSelector() {
    m_vfctrl.SetTitle(m_parent != NULL ? m_parent->descr : L"        ");
  }

  bool _IsClosely() {
    for( accounts_t::iterator it = m_ac->begin(); it != m_ac->end(); it++ ) {
      if( (*it)->closely ) {
        return true;
      }
    }
    return false;
  }
};

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

// Format: %id%;%descr%;%note%;%docs%;%pos%;%exec_limit%;%pending_limit%;
static 
bool __act_type(void *, int line, const wchar_t **argv, int count) 
{
  ACTIVITY_TYPE_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( count < 7 ) {
    return false;
  }

  memset(&cnf, 0, sizeof(ACTIVITY_TYPE_CONF));
  COPY_ATTR__S(cnf.activity_type_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  COPY_ATTR__S(cnf.note, argv[2]);
  cnf.docs = wcsistrue(argv[3]);
  cnf.pos = wcsistrue(argv[4]);
  cnf.exec_limit = _wtoi(argv[5]);
  cnf.pending_limit = _wtoi(argv[6]);
  _at.push_back(cnf);

  return true;
}

// Format: %p_date%;%account_id%;%activity_type_id%;%allow_pending%;%pending_date%;%color%;%bgcolor%;
static 
bool __routes(void *, int line, const wchar_t **argv, int argc) 
{
  ROUTE_CONF val;
  activity_types_t::iterator at_it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 7 ) {
    return false;
  }
  if( !(argv[0][0] == L'\0' || wcsncmp(_c_date, argv[0], 10) == 0) ) {
    return true;
  }
  if( (at_it = std::find_if(_at.begin(), _at.end(), _find_activity_type(argv[2]))) == _at.end() ) {
    return true;
  }

  memset(&val, 0, sizeof(val));
  val.at = &(*at_it);
  COPY_ATTR__S(val.account_id, argv[1]);
  val.allow_pending = wcsistrue(argv[3]);
  COPY_ATTR__S(val.route_date, wcslen(argv[4]) == 10 ? argv[4] : _c_date);
  val.color = (int32_t) _wtoi(argv[5]);
  val.bgcolor = (int32_t) _wtoi(argv[6]);

  _route.push_back(val);

  return true;
}

// Format: %begin%;%end%;%account_id%;%activity_type_id%;
static 
bool __user_activity_r(void *, int line, const wchar_t **argv, int argc) 
{
  routes_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 4 ) {
    return false;
  }
  if( wcsncmp(_c_date, argv[0], OMOBUS_MAX_DATE) != 0 ) {
    return true;
  }
  if( (it = std::find_if(_route.begin(), _route.end(), _find_route(argv[2], argv[3]))) 
              == _route.end() ) {
    return true;
  }

  it->freq_c++;

  return true;
}

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;%group_price_id%;%pay_delay%;%locked%;%latitude%;%longitude%;
static 
bool __account_r(void *cookie, int line, const wchar_t **argv, int count) 
{
  routes_t::iterator it;
  simple_t::iterator it_s;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 13 ) {
    return false;
  }
  if( /*ftype*/ _ttoi(argv[2]) > 0 ) {
    return true;
  }
  if( (it = std::find_if(_route.begin(), _route.end(), _find_account_id(argv[1]))) 
        == _route.end() ) {
    return true;
  }
  COPY_ATTR__S(it->descr, argv[4]);
  COPY_ATTR__S(it->address, argv[5]);
  COPY_ATTR__S(it->chan, argv[6]);
  COPY_ATTR__S(it->group_price_id, argv[8]);
  it->pay_delay = (short)_wtoi(argv[9]);
  it->locked = (byte_t)_wtoi(argv[10]);

  if( (it_s = std::find_if(_potens.begin(), _potens.end(), _find_simple(argv[7])))
        != _potens.end() ) {
    COPY_ATTR__S(it->poten, it_s->descr);
  }

  return true;
}

// Format: %date%;%account_id%;%activity_type_id%;route_date%;...
static 
bool __pending(void *, int line, const wchar_t **argv, int argc) 
{
  routes_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 4 ) {
    return false;
  }
  if( wcsncmp(_c_date, argv[0], OMOBUS_MAX_DATE) != 0 ) {
    return true;
  }
  if( (it = std::find_if(_route.begin(), _route.end(), _find_route(argv[1], argv[2]))) 
              == _route.end() ) {
    return true;
  }

  it->pending = true;

  return true;
}

static 
bool __my(void *cookie, int line, const wchar_t **argv, int count) 
{
  if( line == 0 )
    return true;
  if( cookie == NULL || count < 1 )
    return false;
  my_t *ptr = (my_t *)cookie;
  ptr->push_back(argv[0]);
  return true;
}

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;%group_price_id%;%pay_delay%;%locked%;%latitude%;%longitude%;
static 
bool __account(void *cookie, int line, const wchar_t **argv, int count) 
{
  __account_cookie *ptr = (__account_cookie *)cookie;
  ACCOUNT_CONF *cnf;
  double la, lo;
  simple_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 13 ) {
    return false;
  }
  if( (cnf = new ACCOUNT_CONF) == NULL ) {
    return false;
  }

  memset(cnf, 0, sizeof(ACCOUNT_CONF));
  COPY_ATTR__S(cnf->pid, argv[0]);
  COPY_ATTR__S(cnf->account_id, argv[1]);
  cnf->ftype = (byte_t)_wtoi(argv[2]);
  COPY_ATTR__S(cnf->code, argv[3]);
  COPY_ATTR__S(cnf->descr, argv[4]);
  COPY_ATTR__S(cnf->address, argv[5]);
  COPY_ATTR__S(cnf->chan, argv[6]);
  COPY_ATTR__S(cnf->group_price_id, argv[8]);
  cnf->pay_delay = (short)_wtoi(argv[9]);
  cnf->locked = (byte_t)_wtoi(argv[10]);

  if( !cnf->ftype ) {
    cnf->my = std::find(ptr->my->begin(), ptr->my->end(), cnf->account_id) 
      != ptr->my->end();
    cnf->closed = std::find_if(ptr->ua->begin(), ptr->ua->end(), 
      _find_user_activity(cnf->account_id, NULL)) != ptr->ua->end();

    if( (it = std::find_if(ptr->potens->begin(), ptr->potens->end(), _find_simple(argv[7])))
          != ptr->potens->end() ) {
      COPY_ATTR__S(cnf->poten, it->descr);
    }

    if( ptr->radius > 0 &&
        (ptr->gpsPos->lat != 0 && ptr->gpsPos->lon != 0) &&
        ((la =  wcstof(argv[11])) != 0 && (lo = wcstof(argv[12])) != 0) &&
        ptr->radius >= dist(ptr->gpsPos->lat, ptr->gpsPos->lon, la, lo) ) {
      cnf->closely = 1;
    }
  }

  ptr->l->push_back(cnf);

  return true;
}

// Format: %begin%;%end%;%account_id%;%activity_type_id%;
static 
bool __user_activity(void *cookie, int line, const wchar_t **argv, int argc) 
{
  __user_activity_cookie *ptr = (__user_activity_cookie *) cookie;
  user_activities_t::iterator it;
  USER_ACTIVITY_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 4 ) {
    return false;
  }
  if( wcsncmp(ptr->c_date, argv[0], 10) != 0 ) {
    return true;
  }
  if( (it = std::find_if(ptr->l->begin(), ptr->l->end(), _find_user_activity(argv[2], argv[3]))) 
              == ptr->l->end() ) {
    memset(&cnf, 0, sizeof(cnf));
    COPY_ATTR__S(cnf.account_id, argv[2]);
    COPY_ATTR__S(cnf.activity_type_id, argv[3]);
    cnf.freq_c = 1; 
    ptr->l->push_back(cnf);
  } else {
    it->freq_c++;
  }

  return true;
}

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
slist_t &_parce_slist(const wchar_t *s, slist_t &sl) {
  sl.clear();
  if( s == NULL )
    return sl;
  std::wstring buf;
  for( int i = 0; s[i] != L'\0'; i++ ) {
    if( s[i] == L' ' ) {
      if( !buf.empty() )
        sl.push_back(buf);
      buf = L"";
    } else {
      buf += s[i];
    }
  }
  if( !buf.empty() )
    sl.push_back(buf);
  return sl;
}

static
bool IsEqual(const wchar_t *src, slist_t &l) {
  for( size_t i = 0; i < l.size(); i++ ) {
    if( wcsstri(src, l[i].c_str()) == NULL )
      return false;
  }
  return true;
}

static
bool IsEqual2(const wchar_t *src1, const wchar_t *src2, slist_t &l) {
  for( size_t i = 0; i < l.size(); i++ ) {
    if( wcsstri(src1, l[i].c_str()) == NULL && wcsstri(src2, l[i].c_str()) == NULL )
      return false;
  }
  return true;
}

template< class T >
void _man_cleanup(T &v) {
  size_t size = v.size(), i = 0;
  for( ; i < size; i++ ) {
    if( v[i] != NULL )
      delete v[i];
  }
  v.clear();
}

static
void initCookie(HTERMINAL *h) {
  _snwprintf(h->cur_activity.a_cookie, OMOBUS_MAX_ID, L"r_%x", omobus_time());
  h->put_conf(__INT_A_COOKIE, h->cur_activity.a_cookie);
  DEBUGMSG(TRUE, (L"r_cookie/begin: %s\n", h->cur_activity.a_cookie));
}

static
void deinitCookie(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"r_cookie/end: %s\n", h->cur_activity.a_cookie));
  h->put_conf(__INT_A_COOKIE, NULL);
  memset(h->cur_activity.a_cookie, 0, sizeof(h->cur_activity.a_cookie));
}

static
bool WriteJournal(HTERMINAL *h, time_t ttEnd) 
{
  std::wstring txt, templ;

  templ = txt = wsfromRes(IDS_J_USER_ACTIVITY, true);
  if( txt.empty() ) {
    return false;
  }

  wsrepl(txt, L"%begin%", wsftime(h->cur_activity.ttBegin).c_str());
  wsrepl(txt, L"%end%", wsftime(ttEnd).c_str());
  wsrepl(txt, L"%activity_type_id%", h->cur_activity.activity_type_id);
  wsrepl(txt, L"%account_id%", h->cur_activity.account_id);
  
  return WriteOmobusJournal(OMOBUS_USER_ACTIVITY, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
bool WriteActivity(HTERMINAL *h, omobus_location_t *pos, time_t tt, const wchar_t *state) 
{
  std::wstring xml;
  
  xml = wsfromRes(IDS_X_USER_ACTIVITY, true);
  if( xml.empty() ) {
    return false;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(tt).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(h->cur_activity.account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(h->cur_activity.activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(pos->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%state%", state);
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  wsrepl(xml, L"%a_cookie%", h->cur_activity.a_cookie);
  wsrepl(xml, L"%route_date%", h->cur_activity.route_date);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_ACTIVITY, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk_Doc(HTERMINAL *h, uint64_t doc_id, const wchar_t *doc_type,
  const wchar_t *account, const wchar_t *activity_type_id, omobus_location_t *posClo, 
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
  wsrepl(xml, L"%doc_type%", doc_type);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(account).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", h->w_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteDocPk_Pending(HTERMINAL *h, PENDING *r, uint64_t doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_PENDING, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%doc_note%", fix_xml(r->note).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  wsrepl(xml, L"%account_id%", fix_xml(r->account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(r->activity_type_id).c_str());
  wsrepl(xml, L"%created_dt%", wsftime(r->ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%pending_type_id%", fix_xml(r->type.id).c_str());
  wsrepl(xml, L"%route_date%", r->route_date);
  wsrepl(xml, L"%created_gps_dt%", wsftime(r->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(r->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(r->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_PENDING, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk_Pending(HTERMINAL *h, PENDING *r, uint64_t doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
{
  return WriteActPk_Doc(h, doc_id, OMOBUS_PENDING, r->account_id,
    r->activity_type_id, posClo, ttClo);
}

static
bool WriteJournal_Pending(HTERMINAL *h, PENDING *r, uint64_t doc_id, 
  time_t tt) 
{
  std::wstring txt, templ;

  templ = txt = wsfromRes(IDS_J_PENDING, true);
  if( txt.empty() ) {
    return false;
  }

  wsrepl(txt, L"%date%", wsftime(tt, ISO_DATETIME_FMT).c_str());
  wsrepl(txt, L"%route_date%", r->route_date);
  wsrepl(txt, L"%activity_type_id%", r->activity_type_id);
  wsrepl(txt, L"%account_id%", r->account_id);
  wsrepl(txt, L"%pending_type%", r->type.descr);
  wsrepl(txt, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(txt, L"%doc_note%", r->note);
  
  return WriteOmobusJournal(OMOBUS_PENDING, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
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
void exec_oper(const wchar_t *oper) {
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
void exec_slist(const wchar_t *s) {
  if( s == NULL )
    return;
  std::wstring buf;
  for( int i = 0; s[i] != L'\0'; i++ ) {
    if( s[i] == L' ' ) {
      if( !buf.empty() )
        exec_oper(buf.c_str());
      buf = L"";
    } else {
      buf += s[i];
    }
  }
  if( !buf.empty() )
    exec_oper(buf.c_str());
}

static
std::wstring getMsg(int status, const wchar_t *a_name) {
  std::wstring msg = wsfromRes(status & OMOBUS_SCREEN_WORK ? IDS_TITLE0 : 
    (status & OMOBUS_SCREEN_ROUTE ? IDS_TITLE1 : IDS_TITLE2) );
  if( status & OMOBUS_SCREEN_ACTIVITY || status & OMOBUS_SCREEN_ACTIVITY_TAB_1 ) {
    msg += a_name;
  }
  return msg;
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
                rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus0 ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, getMsg(getStatus(h), h->cur_activity.a_name).c_str(), -1, &rect, 
    DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
bool OnStartRoute(HTERMINAL *h) {
  h->w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  return true;
}

static
bool OnStopRoute(HTERMINAL *h) {
  h->w_cookie = NULL;
  return true;
}

static
void OnStartActivity(HTERMINAL *h) 
{
  CWaitCursor _wc;
  initCookie(h);
  wcsupr(h->cur_activity.a_name);
  omobus_location(&h->cur_activity.posBegin);
  h->cur_activity.ttBegin = omobus_time();
  h->put_conf(__INT_ACTIVITY_TYPE_ID, h->cur_activity.activity_type_id);
  h->put_conf(__INT_ACCOUNT_ID, h->cur_activity.account_id);
  h->put_conf(__INT_ACCOUNT, h->cur_activity.descr);
  h->put_conf(__INT_ACCOUNT_ADDR, h->cur_activity.address);
  h->put_conf(__INT_ACCOUNT_GR_PRICE_ID, h->cur_activity.group_price_id);
  h->put_conf(__INT_ACCOUNT_PAY_DELAY, itows(h->cur_activity.pay_delay).c_str());
  h->put_conf(__INT_ACCOUNT_LOCKED, itows(h->cur_activity.locked).c_str());
  WriteActivity(h, &h->cur_activity.posBegin, h->cur_activity.ttBegin, L"begin");
  exec_slist(h->act_begin);
}

static
bool OnStartActivityR(HTERMINAL *h, ROUTE_CONF *route) 
{
  if( CMainDlg(h, route).DoModal(h->hParWnd) == IDCANCEL ) {
    return false;
  }
  memset(&h->cur_activity, 0, sizeof(h->cur_activity));
  COPY_ATTR__S(h->cur_activity.activity_type_id, route->at->activity_type_id);
  COPY_ATTR__S(h->cur_activity.a_name, route->at->descr);
  h->cur_activity.docs = route->at->docs;
  COPY_ATTR__S(h->cur_activity.account_id, route->account_id);
  COPY_ATTR__S(h->cur_activity.descr, route->descr);
  COPY_ATTR__S(h->cur_activity.address, route->address);
  COPY_ATTR__S(h->cur_activity.group_price_id, route->group_price_id);
  COPY_ATTR__S(h->cur_activity.route_date, route->route_date);
  h->cur_activity.locked = route->locked;
  h->cur_activity.pay_delay = route->pay_delay;
  OnStartActivity(h);
  return true;
}

static
bool OnStartActivityA(HTERMINAL *h) {
  CWaitCursor _wc;
  bool rc;
  accounts_t ac;
  my_t my; 
  user_activities_t ua;
  simple_t potens;
  routes_t::iterator r_it;
  omobus_location_t gpsPos = {0};
  wchar_t c_date[11] = L"";
  __account_cookie ck_ac = { &ac, &my, &ua, &potens, &gpsPos, h->radius };
  __user_activity_cookie ck_ua = { c_date, &ua };

  memset(&h->cur_activity, 0, sizeof(h->cur_activity));
  omobus_location(&gpsPos);
  wcsncpy(c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(c_date));
  ATLTRACE(L"c_date = %s\n", c_date);
  ReadOmobusJournal(OMOBUS_USER_ACTIVITY, &ck_ua, __user_activity);
  ATLTRACE(OMOBUS_USER_ACTIVITY L" = %i\n", ua.size());
  ReadOmobusManual(L"my_accounts", &my, __my);
  ATLTRACE(L"my_accounts = %i\n", my.size());
  ReadOmobusManual(L"potentials", &potens, __simple);
  ATLTRACE(L"potentials = %i\n", potens.size());
  ReadOmobusManual(L"accounts", &ck_ac, __account);
  ATLTRACE(L"accounts = %i/%i (size=%i Kb)\n", sizeof(ACCOUNT_CONF), ac.size(), 
    sizeof(ACCOUNT_CONF)*ac.size()/1024);
  _wc.Restore();

  if( (rc = CAccountsFrame(h, &ac, &_at, &ua, !my.empty()).DoModal(h->hParWnd) == ID_ACTION) ) {
    if( _route.end() != (r_it = std::find_if(_route.begin(), _route.end(), 
          _find_route(h->cur_activity.account_id, h->cur_activity.activity_type_id))) ) {
      COPY_ATTR__S(h->cur_activity.route_date, r_it->route_date);
    }
    OnStartActivity(h);
  }
  _man_cleanup<accounts_t>(ac);

  return rc;
}

static
bool OnStopActivity(HTERMINAL *h) 
{
  if( MessageBoxEx(h->hParWnd, IDS_STR3, IDS_INFO, MB_ICONQUESTION|MB_YESNO) == IDNO ) {
    return false;
  }
  if( h->cur_activity.docs > (byte_t)_wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0")) ) {
    MessageBoxEx(h->hParWnd, IDS_STR12, IDS_ERROR, MB_ICONSTOP);
    return false;
  }

  time_t ttEnd;
  omobus_location_t posEnd;
  CWaitCursor _wc;
  omobus_location(&posEnd);
  ttEnd = omobus_time();

  WriteActivity(h, &posEnd, ttEnd, L"end");
  WriteJournal(h, ttEnd);

  h->put_conf(__INT_ACTIVITY_TYPE_ID, NULL);
  h->put_conf(__INT_ACCOUNT_ID, NULL);
  h->put_conf(__INT_ACCOUNT, NULL);
  h->put_conf(__INT_ACCOUNT_ADDR, NULL);
  h->put_conf(__INT_ACCOUNT_GR_PRICE_ID, NULL);
  h->put_conf(__INT_ACCOUNT_PAY_DELAY, NULL);
  h->put_conf(__INT_ACCOUNT_LOCKED, NULL);
  h->put_conf(__INT_ACTIVITY_DOCS, NULL);
  h->put_conf(__INT_DEL_DATE, NULL);

  memset(&h->cur_activity, 0, sizeof(h->cur_activity));
  deinitCookie(h);
  exec_slist(h->act_end);

  return true;
}

static 
void OnAction(HTERMINAL *h) {
  int s;
  if( (s = getStatus(h)) & OMOBUS_SCREEN_WORK ) {
    if( OnStartRoute(h) ) {
      putStatus(h, OMOBUS_SCREEN_ROUTE);
    }
  } else if( s & OMOBUS_SCREEN_ROUTE ) {
    if( OnStopRoute(h) ) {
      putStatus(h, OMOBUS_SCREEN_WORK);
    }
  } else if( s & OMOBUS_SCREEN_ACTIVITY || s & OMOBUS_SCREEN_ACTIVITY_TAB_1) {
    if( OnStopActivity(h) ) {
      putStatus(h, OMOBUS_SCREEN_ROUTE);
    }
  }
}

static
HWND InitListView(HWND hWnd, RECT *rcDraw) {
  HWND hWndL = NULL;
  hWndL = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
     LVS_OWNERDRAWFIXED | LVS_ALIGNLEFT | LVS_OWNERDATA | LVS_NOCOLUMNHEADER | WS_TABSTOP, 
     rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top, 
     hWnd, (HMENU)NULL, _Module.GetModuleInstance(), NULL);
  if( hWndL == NULL ) {
    RETAILMSG(TRUE, (L"routes: Unable to create ListView.\n"));
    return hWndL;
  }
  _lvc.mask = LVCF_WIDTH;
  _lvc.cx = DRA::SCALEY(240);
  ListView_InsertColumn(hWndL, 0, &_lvc);
  ListView_SetExtendedListViewStyle(hWndL, LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
  return hWndL;
}

inline
void DrawLine(HDC hDc, int left, int top, int right, int bottom) {
  MoveToEx(hDc, left, top, NULL); LineTo(hDc, right, bottom);
}

static
int GetLastSelectionMark() {
  routes_t::iterator it;
  return 
    _sel_account_id[0] == L'\0' 
      ||
    (it = std::find_if(_route.begin(), _route.end(), _find_account_id(_sel_account_id)))
      == _route.end() ? -1 : (int) (it - _route.begin());
}

static
void OnDrawItemL(HTERMINAL *h, LPDRAWITEMSTRUCT dis) 
{
  ROUTE_CONF *ptr;
  HDC hDC;
  HFONT hOldFont;
  HPEN hOldPen;
  HBRUSH hbrCur;
  COLORREF crOldTextColor, crOldBkColor;
  RECT rc;
  int scrWidth;
  
  if( dis->CtlType != ODT_LISTVIEW || dis->itemID >= _route.size() || 
      (ptr = &(_route[dis->itemID])) == NULL || (hDC = dis->hDC) == NULL ) {
    return;
  }

  scrWidth = GetSystemMetrics(SM_CXVSCROLL);
  hOldFont = (HFONT)SelectObject(hDC, h->hFontB);
  hOldPen = (HPEN)SelectObject(hDC, h->hpBord);
  hbrCur = ptr->bgcolor ? CreateSolidBrush(ptr->bgcolor) : NULL;
  crOldTextColor = GetTextColor(hDC);
  crOldBkColor = GetBkColor(hDC);
  memcpy(&rc, &dis->rcItem, sizeof(RECT));

  if( h->focus1 && (dis->itemAction | ODA_SELECT) && (dis->itemState & ODS_SELECTED) ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, &rc, h->hbrHL);
  } else {
    if( ptr->freq_c || ptr->pending ) {
      SetTextColor(hDC, RGB(150, 150, 150));
    } else if( ptr->color != 0 ) {
      SetTextColor(hDC, ptr->color);
    }
    FillRect(hDC, &rc, hbrCur != NULL ? hbrCur : (dis->itemID%2==0?h->hbrN:h->hbrOdd));
  }

  SetBkMode(hDC, TRANSPARENT);

  rc.left += DRA::SCALEX(2);
  rc.right -= (scrWidth + DRA::SCALEX(2));
  rc.top += DRA::SCALEY(2);
  rc.bottom = rc.top + DRA::SCALEY(15);
  DrawText(hDC, ptr->descr, -1, &rc, DT_LEFT|DT_TOP|DT_NOPREFIX|
    DT_SINGLELINE|DT_WORD_ELLIPSIS);

  SelectObject(hDC, h->hFontN);
  rc.top = rc.bottom;
  rc.bottom += DRA::SCALEY(28);
  DrawText(hDC, ptr->address, -1, &rc, DT_LEFT|DT_TOP|DT_NOPREFIX|
    DT_WORDBREAK|DT_WORD_ELLIPSIS);

  rc.top = rc.bottom + DRA::SCALEY(2);
  rc.bottom = dis->rcItem.bottom;
  rc.right = DRA::SCALEX(160);
  DrawText(hDC, ptr->chan, -1, &rc, DT_LEFT|DT_TOP|DT_NOPREFIX|
    DT_SINGLELINE|DT_WORD_ELLIPSIS);

  rc.left = rc.right + DRA::SCALEX(10);
  rc.right = rc.left + DRA::SCALEX(35);
  DrawText(hDC, ptr->poten, -1, &rc, DT_LEFT|DT_TOP|DT_NOPREFIX|
    DT_SINGLELINE/*|DT_WORD_ELLIPSIS*/);

  if( ptr->freq_c ) {
    rc.left = rc.right;
    rc.right = dis->rcItem.right - (scrWidth + DRA::SCALEX(2));
    SelectObject(hDC, h->hFontB);
    DrawText(hDC, L"+", 1, &rc, DT_RIGHT|DT_TOP|DT_NOPREFIX|
      DT_SINGLELINE/*|DT_WORD_ELLIPSIS*/);
  }

  DrawLine(hDC, dis->rcItem.left, dis->rcItem.bottom - 1, 
    dis->rcItem.right, dis->rcItem.bottom - 1);

  SelectObject(hDC, hOldFont);
  SelectObject(hDC, hOldPen);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
  if( hbrCur != NULL ) {
    DeleteObject(hbrCur);
  }
}

static
void OnMeasureItemL(HWND hWnd, LPMEASUREITEMSTRUCT mis) {
  if( mis->CtlType == ODT_LISTVIEW )
    mis->itemHeight = DRA::SCALEY(65);
}

static
void OnShowL(HTERMINAL *h, RECT *rcDraw) {
  CWaitCursor wc;
  int selMark = -1;
  _route.clear();
  _potens.clear();
  _at.clear();
  wcsncpy(_c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(_c_date));
  ReadOmobusManual(L"potentials", &_potens, __simple);
  DEBUGMSG(TRUE, (L"potentials = %i\n", _potens.size()));
  ReadOmobusManual(L"activity_types", h, __act_type);
  DEBUGMSG(TRUE, (L"activity_types = %i\n", _at.size()));
  ReadOmobusManual(L"my_routes", h, __routes);
  DEBUGMSG(TRUE, (L"my_routes(%s) = %i\n", _c_date, _route.size()));
  if( !_route.empty() ) {
    ReadOmobusJournal(OMOBUS_USER_ACTIVITY, h, __user_activity_r);
    ReadOmobusJournal(OMOBUS_PENDING, h, __pending);
    ReadOmobusManual(L"accounts", h, __account_r);
    if( h->hWndL == NULL ) {
      h->hWndL = InitListView(h->hWnd1, rcDraw);
    }
  } else {
    chk_destroy_window(h->hWndL);
  }
  if( h->hWndL != NULL ) {
    ListView_SetItemCount(h->hWndL, _route.size());
    if( (selMark = GetLastSelectionMark()) != -1 ) {
      ListView_SetItemState(h->hWndL, selMark, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
      ListView_EnsureVisible(h->hWndL, selMark, FALSE);
      SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
    }
    ShowWindow(h->hWndL, SW_SHOWNORMAL);
  }
  DEBUGMSG(TRUE, (L"routes::OnShow(): selMark=%i\n", selMark));
}

static
void OnHideL(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"routes::OnHide\n"));
  chk_destroy_window(h->hWndL);
  _route.clear();
  _potens.clear();
  _at.clear();
}

static 
void OnActionL(HTERMINAL *h, size_t item) {
  ROUTE_CONF *route = NULL;
  if( item >= _route.size() || (route = &_route[item]) == NULL ) {
    return;
  }
  if( OnStartActivityR(h, route) ) {
    putStatus(h, OMOBUS_SCREEN_ACTIVITY);
  }
  COPY_ATTR__S(_sel_account_id, route->account_id);
}

static
void OnPaintL(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(20), 
                rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFontE);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  SetTextColor(hDC, RGB(150, 150, 150));
  FillRect(hDC, rcDraw, h->hbrN);

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, wsfromRes(IDS_TITLE3).c_str(), -1, &rect, DT_CENTER|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_TOP);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnActionU(HTERMINAL *h) {
  if( OnStartActivityA(h) ) {
    putStatus(h, OMOBUS_SCREEN_ACTIVITY);
  }
}

static
void OnPaintU(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
                rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus2 ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, wsfromRes(IDS_TITLE4).c_str(), -1, &rect, DT_RIGHT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

void CPendingDlg::_Load() {
  m_doc.ttCre = omobus_time();
  omobus_location(&m_doc.posCre);
  ReadOmobusManual(L"pending_types", &m_t, __simple);
  ATLTRACE(L"pending_types = %i\n", m_t.size());
}

bool CPendingDlg::_Close() {
  CWaitCursor _wc;
  bool rc = false;
  omobus_location_t posClo; 
  omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( (rc = WriteDocPk_Pending(m_hterm, &m_doc, doc_id, &posClo, ttClo)) ) {
    WriteActPk_Pending(m_hterm, &m_doc, doc_id, &posClo, ttClo);
    WriteJournal_Pending(m_hterm, &m_doc, doc_id, ttClo);
  }
  return rc;
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
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus0 = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus0 ) {
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    	InvalidateRect(hWnd, NULL, FALSE);
    }
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
LRESULT CALLBACK WndProcU(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
      OnPaintU((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), hDC, &rcDraw);
			EndPaint(hWnd, &ps);
		}
		return TRUE;
	case OMOBUS_SSM_PLUGIN_SELECT:
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus2 = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus2 ) {
    	OnActionU((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    	InvalidateRect(hWnd, NULL, FALSE);
    }
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd2);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
LRESULT CALLBACK WndProcL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  HTERMINAL *h;
  RECT rcDraw;
  int l;
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
      GetClientRect(hWnd, &rcDraw);
      OnPaintL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), hDC, &rcDraw);
			EndPaint(hWnd, &ps);
		}
		return TRUE;
	case OMOBUS_SSM_PLUGIN_SHOW:
	case OMOBUS_SSM_PLUGIN_RELOAD:
    GetClientRect(hWnd, &rcDraw);
    OnShowL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), &rcDraw);
    return 0;
	case OMOBUS_SSM_PLUGIN_HIDE:
    OnHideL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    return 0;
	case OMOBUS_SSM_PLUGIN_SELECT:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    h->focus1 = ((BOOL)wParam)==TRUE;
    l = ListView_GetSelectionMark(h->hWndL);
    ListView_RedrawItems(h->hWndL, l, l);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    l = ListView_GetSelectionMark(h->hWndL);
    if( h->focus1 ) {
    	OnActionL(h, l);
    }
		return 0;
  case WM_LBUTTONDOWN:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
		return 0;
  case WM_DRAWITEM:
    OnDrawItemL(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA)), 
      (LPDRAWITEMSTRUCT)lParam);
    return TRUE;
  case WM_MEASUREITEM:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    OnMeasureItemL(h->hParWnd, (LPMEASUREITEMSTRUCT)lParam);
    return TRUE;
  case WM_NOTIFY: {
      LPNMHDR pnmh = (LPNMHDR) lParam;
      h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
      if( pnmh->code == LVN_ITEMACTIVATE || pnmh->code ==  LVN_ITEMCHANGED ) {
        if( !h->focus1 ) {
          SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
        }
        if( pnmh->code == LVN_ITEMACTIVATE ) {
          OnActionL(h, ((NMLISTVIEW *)pnmh)->iItem);
        }
      }
    }
    break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
HWND CreatePluginWindow(HTERMINAL *h, const TCHAR *szWindowClass, WNDPROC lpfnWndProc) 
{
	WNDCLASS wc; 
  memset(&wc, 0, sizeof(WNDCLASS));
	wc.hInstance = _Module.GetModuleInstance();
	wc.lpszClassName = szWindowClass;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpfnWndProc = lpfnWndProc;
	RegisterClass(&wc);
  return CreateWindowEx(0, szWindowClass, szWindowClass, WS_CHILD, 0, 0, 
    GetSystemMetrics(SM_CXSCREEN), 0, h->hParWnd, NULL, _Module.GetModuleInstance(), h);

}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _Module.Init(NULL, (HINSTANCE)hModule);
    DisableThreadLibraryCalls((HINSTANCE)hModule);
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

  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->act_begin = get_conf(OMOBUS_ACTMGR_BEGIN, OMOBUS_ACTMGR_BEGIN_DEF);
  h->act_end = get_conf(OMOBUS_ACTMGR_END, OMOBUS_ACTMGR_END_DEF);
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME, ISO_DATETIME_FMT);
  h->radius = _wtoi(get_conf(OMOBUS_ACTMGR_RADIUS, OMOBUS_ACTMGR_RADIUS_DEF));

  if( h->user_id == NULL ) {
    RETAILMSG(TRUE, (L"routes: >>> INCORRECT USER_ID <<<\n"));
    memset(h, 0, sizeof(HTERMINAL));
    delete h;
    return NULL;
  }

  _Module.m_hInstResource = LoadMuiModule(_Module.GetModuleInstance(), 
    get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _nHeight1 = _wtoi(get_conf(OMOBUS_HEIGHT, OMOBUS_HEIGHT_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hFontE = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontN = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd0 = CreatePluginWindow(h, _szWindowClass0, WndProc)) != NULL ) {
    rsi(h->hWnd0, _nHeight0, OMOBUS_SCREEN_WORK|OMOBUS_SCREEN_ROUTE|OMOBUS_SCREEN_ACTIVITY|
      OMOBUS_SCREEN_ACTIVITY_TAB_1);
  }
  if( (h->hWnd2 = CreatePluginWindow(h, _szWindowClass2, WndProcU)) != NULL ) {
    rsi(h->hWnd2, _nHeight2, OMOBUS_SCREEN_ROUTE);
  }
  if( (h->hWnd1 = CreatePluginWindow(h, _szWindowClass1, WndProcL)) != NULL ) {
    rsi(h->hWnd1, _nHeight1, OMOBUS_SCREEN_ROUTE);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontE);
  chk_delete_object(h->hFontN);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd0);
  chk_destroy_window(h->hWnd1);
  chk_destroy_window(h->hWnd2);
  chk_destroy_window(h->hWndL);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
