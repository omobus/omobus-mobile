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

#define OMOBUS_ACTMGR_RADIUS           L"activity->radius"
#define OMOBUS_ACTMGR_RADIUS_DEF       L"500"
#define OMOBUS_ACTMGR_BEGIN            L"activity->begin"
#define OMOBUS_ACTMGR_BEGIN_DEF        L""
#define OMOBUS_ACTMGR_END              L"activity->end"
#define OMOBUS_ACTMGR_END_DEF          L""

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

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"
#include "../../version"

static HINSTANCE _hInstance = NULL;
static TCHAR _szWindowClass[] = L"omobus-activity-terminal-element";
static CAppModule _Module;
static int _nHeight = 20;
static CWaitCursor _WaitCursor(false);

typedef std::vector<std::wstring> slist_t;
typedef std::vector<std::wstring> my_t;

static bool IsEqual(const wchar_t *src, slist_t &l);
static bool IsEqual2(const wchar_t *src1, const wchar_t *src2, slist_t &l);
static slist_t &_parce_slist(const wchar_t *s, slist_t &sl);


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
  byte_t pos; // Открывать только при наличие актуальной позиции.
  int32_t exec_limit; // Лимит выполненных активностей.
};
typedef std::vector<ACTIVITY_TYPE_CONF> activity_types_t;

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
typedef std::map<ACCOUNT_CONF*, accounts_t*> vf_gr_t;

struct USER_ACTIVITY_CONF {
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t activity_type_id[OMOBUS_MAX_ID+1];
  int32_t freq_c;
};
typedef std::vector<USER_ACTIVITY_CONF> user_activities_t;

struct P_ROUTE_CONF {
  wchar_t account_id[OMOBUS_MAX_ID+1];
};
typedef std::vector<P_ROUTE_CONF> routes_t;

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf; 
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  bool focus;
  const wchar_t *user_id, *pk_ext, *pk_mode, *act_begin, *act_end, 
    *datetime_fmt, *w_cookie;
  size_t radius;
  wchar_t a_cookie[OMOBUS_MAX_ID + 1];
  ACTIVITY_TYPE_CONF at_conf;
  ACCOUNT_CONF ac_conf;
  time_t ttBegin;
  omobus_location_t posBegin; 
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

class find_vf_gr :
  public std::unary_function<vf_gr_t::value_type, bool> {
private:
  const wchar_t *m_account_id;
public:
  find_vf_gr(const wchar_t *account_id) : m_account_id(account_id) {
  }
  bool operator()(vf_gr_t::value_type &v) const {
    return COMPARE_ID(m_account_id, v.first->account_id);
  }
};

struct __find_activity_type : public std::unary_function<ACTIVITY_TYPE_CONF, bool> {
    const wchar_t *m_activity_type_id;
    __find_activity_type(const wchar_t *activity_type_id) : m_activity_type_id(activity_type_id) {
    }
    bool operator()(ACTIVITY_TYPE_CONF &v) const {
      return COMPARE_ID(m_activity_type_id, v.activity_type_id);
    }
  };

struct find_account : public std::unary_function<ACCOUNT_CONF, bool> {
  const wchar_t *m_account_id;
  find_account(const wchar_t *account_id) : m_account_id(account_id) {
  }
  bool operator()(ACCOUNT_CONF *v) const {
    return COMPARE_ID(m_account_id, v->account_id);
  }
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

struct _find_routes : public std::unary_function <P_ROUTE_CONF, bool> { 
    const wchar_t *account_id;
    _find_routes(const wchar_t *account_id_) : account_id(account_id_) {
    }
    bool operator()(const P_ROUTE_CONF &v) { 
      return COMPARE_ID(account_id, v.account_id);
    }
  };

struct __account_cookie 
  { accounts_t *l; my_t *my; user_activities_t *ua; routes_t *pr; simple_t *potens;
    omobus_location_t *gpsPos; int32_t radius; };

struct __user_activity_cookie 
  { const wchar_t *c_date; user_activities_t *l; };

struct __routes_cookie 
  { const wchar_t *c_date; routes_t *l; };

typedef struct _vf_accounts_cookie_t {
  vf_gr_t *vf_gr;
  accounts_t *accounts;
} vf_accounts_cookie_t;

static inline
bool sort_route(ACCOUNT_CONF *elem1, ACCOUNT_CONF *elem2) {
  return elem1->route < elem2->route;
}

class CTypesFrame : 
	public CStdDialogResizeImpl<CTypesFrame>,
	public CUpdateUI<CTypesFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  activity_types_t *m_at;
  user_activities_t *m_ua;
  CListViewCtrl m_list;
  CLabel m_pos;
  CStatic m_time;
  CFont m_boldFont, m_baseFont;

public:
  CTypesFrame(HTERMINAL *hterm, activity_types_t *at, user_activities_t *ua) : 
      m_hterm(hterm), m_at(at), m_ua(ua) {
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
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    ACTIVITY_TYPE_CONF *ptr = &((*m_at)[lpdis->itemID]);
    if( ptr == NULL )
      return 0;

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
    memcpy(&m_hterm->at_conf, at, sizeof(ACTIVITY_TYPE_CONF));
    EndDialog(ID_ACTION);
  }

  bool _CheckPos(ACTIVITY_TYPE_CONF *at) {
    m_hterm->ttBegin = omobus_time();
    omobus_location(&m_hterm->posBegin);
    if( !at->pos || m_hterm->posBegin.location_status )
      return true;
    if( MessageBoxEx(m_hWnd, IDS_STR8, IDS_ERROR, MB_ICONWARNING|MB_YESNO) == IDNO )
      return true;
    return false;
  }

  bool _ChekLimit(ACTIVITY_TYPE_CONF *at) {
    user_activities_t::iterator it;
    if( m_ua->end() != (it = std::find_if(m_ua->begin(), m_ua->end(), 
          _find_user_activity(m_hterm->ac_conf.account_id, at->activity_type_id))) ) {
      if( it->freq_c >= at->exec_limit ) {
        MessageBoxEx(m_hWnd, IDS_STR10, IDS_ERROR, MB_ICONSTOP);
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
    _Next(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Next(m_list.GetSelectedIndex());
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
  void _Next(size_t cur) {
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
  vf_gr_t *m_vf_gr;
  activity_types_t *m_at;
  user_activities_t *m_ua;
  bool m_closely, m_my, m_route;
  ACCOUNT_CONF *m_parent, m_vf[4];
  CHyperButtonTempl<CAccountsFrame> m_vfctrl;
  CListViewCtrl m_list;
  CDimEdit m_name_ff, m_code_ff;
  CFont m_baseFont, m_boldFont, m_lockedFont;
  CString m_subcap;

public:
  CAccountsFrame(HTERMINAL *hterm, accounts_t *ac, vf_gr_t *vf_gr, activity_types_t *at, 
      user_activities_t *ua, bool my, bool route) : m_hterm(hterm), m_vf_gr(vf_gr), m_ac(ac), 
        m_at(at), m_ua(ua), m_my(my), m_route(route) {
    m_parent = &m_vf[0];
    memset(&m_vf, 0, sizeof(m_vf));
    m_vf[0]._vf = m_vf[1]._vf = m_vf[2]._vf = m_vf[3]._vf = 1;
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
    COPY_ATTR__S(m_vf[3].descr, LoadStringEx(IDS_GROUP_ROUTE));
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
    vf_gr_t::iterator vf_it;
    size_t size, i;
    ACCOUNT_CONF *ptr;
    m_cur_ac.clear();
    m_list.SetItemCount(m_cur_ac.size());
    if( m_parent != NULL ) {
      if( m_parent->_vf && (vf_it = m_vf_gr->find(m_parent)) != m_vf_gr->end() ) {
        size = vf_it->second == NULL ? 0 : vf_it->second->size();
        for( i = 0; i < size; i++ ) {
          if( (ptr = (*(vf_it->second))[i]) == NULL || ptr->ftype ) {
            continue;
          }
          if( !( code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
            continue;
          }
          if( !( name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) ) {
            continue;
          }
          m_cur_ac.push_back(ptr);
        }
      } else {
        size = m_ac->size();
        for( i = 0; i < size; i++ ) {
          if( (ptr = (*m_ac)[i]) == NULL || ptr->ftype ) {
            continue;
          }
          if( !( code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
            continue;
          }
          if( !( name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) ) {
            continue;
          }
          if( m_parent == &m_vf[0] ) { // ALL
            // any client
          } else if( m_parent == &m_vf[1] ) { // OWN
            if( !ptr->my )
              continue;
          } else if( m_parent == &m_vf[2] ) { // CLOSELY
            if( !ptr->closely )
              continue;
          } else if( m_parent == &m_vf[3] ) { // ROUTE
            if( !ptr->route )
              continue;
        } else { // NATIVE FOLDER
          if( !COMPARE_ID(m_parent->account_id, ptr->pid) )
            continue;
          }
          m_cur_ac.push_back(ptr);
        }
      }
    }
    // Restore route order
    if( m_parent == &m_vf[3] ) {
      std::sort(m_cur_ac.begin(), m_cur_ac.end(), sort_route);
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
    vf_gr_t::iterator vf_it;
    size_t size, i;
    ACCOUNT_CONF *ptr;

    a->_i = 0;
    if( a->_vf && (vf_it = m_vf_gr->find(a)) != m_vf_gr->end() ) {
      size = vf_it->second == NULL ? 0 : vf_it->second->size();
      for( i = 0; i < size; i++ ) {
        if( (ptr = (*(vf_it->second))[i]) == NULL || ptr->ftype ) {
          continue;
        }
        if( !(code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
          continue;
        }
        if( !(name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) ) {
          continue;
        }
        a->_i++;
      }
    } else {
      size = (*m_ac).size();
      for( size_t i = 0; i < size; i++ ) {
        if( (ptr = (*m_ac)[i]) == NULL || ptr->ftype ) {
          continue;
        }
        if( !(code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
          continue;
        }
        if( !(name_ff.empty() || IsEqual2(ptr->descr, ptr->address, name_ff)) ) {
          continue;
        }
        if( a == &m_vf[0] ) { // ALL
          if( !ptr->ftype )
            a->_i++;
        } else if( a == &m_vf[1] ) { // OWN
          if( !ptr->ftype && ptr->my )
            a->_i++;
        } else if( a == &m_vf[2] ) { // CLOSELY
          if( !ptr->ftype && ptr->closely )
            a->_i++;
        } else if( a == &m_vf[3] ) { // ROUTE
          if( !ptr->ftype && ptr->route )
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
    if( cur >= m_cur_ac.size() ) {
      return;
    }
    if( m_cur_ac[cur]->ftype ) {
      m_parent = m_cur_ac[cur];
      _Reload();
    } else {
      if( m_cur_ac[cur]->locked && 
          MessageBoxEx(m_hWnd, IDS_STR4, IDS_INFO, MB_ICONQUESTION|MB_YESNO) != IDYES ) {
        return;
      }
      ShowWindow(SW_HIDE);
      if( CTypesFrame(m_hterm, m_at, m_ua).DoModal(m_hWnd) == ID_ACTION ) {
        memcpy(&m_hterm->ac_conf, m_cur_ac[cur], sizeof(ACCOUNT_CONF));
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
    vf_gr_t::iterator vf_it;

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
    if( m_route ) {
      _Calc(&m_vf[3], code_ff, name_ff);
      ac_tree.push_back(&m_vf[3]);
    }
    for( vf_it = m_vf_gr->begin(); vf_it != m_vf_gr->end(); vf_it++ ) {
      if( vf_it->second != NULL && !vf_it->second->empty() ) {
        _Calc(vf_it->first, code_ff, name_ff);
        ac_tree.push_back(vf_it->first);
      }
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

inline
double dist(double startLat, double startLong, double endLat, double endLong) {
  return 1000 * 111.2 * acos(sin(startLat) * sin(endLat) + cos(startLat) * 
    cos(endLat) * cos(startLong - endLong));
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

struct vf_gr_cleanup {
  vf_gr_cleanup() {
  }
  void operator()(vf_gr_t::value_type &v) {
    if( v.first != NULL ) {
      delete v.first;
    }
    if( v.second != NULL ) {
      delete v.second;
    }
  }
};

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

// Format: %id%;%descr%;%note%;%docs%;%pos%;%exec_limit%;
static 
bool __act_type(void *cookie, int line, const wchar_t **argv, int count) 
{
  ACTIVITY_TYPE_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 6 ) {
    return false;
  }

  memset(&cnf, 0, sizeof(ACTIVITY_TYPE_CONF));
  COPY_ATTR__S(cnf.activity_type_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  COPY_ATTR__S(cnf.note, argv[2]);
  cnf.docs = wcsistrue(argv[3]);
  cnf.pos = wcsistrue(argv[4]);
  cnf.exec_limit = _wtoi(argv[5]);
  ((activity_types_t *)cookie)->push_back(cnf);

  return true;
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

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;%group_price_id%;%pay_delay%;%locked%;%latitude%;%longitude%;
static 
bool __account(void *cookie, int line, const wchar_t **argv, int count) 
{
  __account_cookie *ptr = (__account_cookie *)cookie;
  ACCOUNT_CONF *cnf;
  double la, lo;
  simple_t::iterator it;
  routes_t::iterator it_route;

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
    if( (it_route = std::find_if(ptr->pr->begin(), ptr->pr->end(), 
          _find_routes(cnf->account_id))) != ptr->pr->end() ) {
      cnf->route = (it_route - ptr->pr->begin()) + 1;
    }

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

// Format: %p_date%;%account_id%;%activity_type_id%;
static 
bool __routes(void *cookie, int line, const wchar_t **argv, int argc) 
{
  P_ROUTE_CONF val;
  __routes_cookie *ptr;

  if( line == 0 ) {
    return true;
  }
  if( argc < 3 || (ptr = (__routes_cookie*)cookie) == NULL ) {
    return false;
  }
  if( !(argv[0][0] == L'\0' || wcsncmp(ptr->c_date, argv[0], 10) == 0) ) {
    return true;
  }

  memset(&val, 0, sizeof(val));
  COPY_ATTR__S(val.account_id, argv[1]);

  ptr->l->push_back(val);

  return true;
}

// Format: %vf_id%;%descr%;
static 
bool __vf_names(void *cookie, int line, const wchar_t **argv, int argc)
{
  vf_gr_t *ptr;
  ACCOUNT_CONF *cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (vf_gr_t *)cookie) == NULL || argc < 2 ) {
    return false;
  }
  if( (cnf = new ACCOUNT_CONF()) == NULL ) {
    return false;
  }
  memset(cnf, 0, sizeof(ACCOUNT_CONF));
  cnf->_vf = 1;
  COPY_ATTR__S(cnf->account_id, argv[0]);
  COPY_ATTR__S(cnf->descr, argv[1]);
  ptr->insert(vf_gr_t::value_type(cnf, NULL));
  
  return true;
}

// Format: %vf_id%;%account_id%
static 
bool __vf_accounts(void *cookie, int line, const wchar_t **argv, int argc)
{
  vf_accounts_cookie_t *ptr;
  vf_gr_t::iterator vf_it;
  accounts_t::iterator account_it;
  accounts_t *l;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (vf_accounts_cookie_t *)cookie) == NULL || argc < 2 ) {
    return false;
  }
  if( (vf_it = std::find_if(ptr->vf_gr->begin(), ptr->vf_gr->end(), find_vf_gr(argv[0]))) 
              != ptr->vf_gr->end() ) {
    if( (l = vf_it->second) == NULL ) {
      if( (l = new accounts_t()) == NULL ) {
        return false;
      } else {
        vf_it->second = l;
      }
    }
    if( (account_it = std::find_if(ptr->accounts->begin(), ptr->accounts->end(), find_account(argv[1])))
            != ptr->accounts->end() && !(*account_it)->ftype ) {
      l->push_back(*account_it);
    }
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
void initWorkCookie(HTERMINAL *h) {
  h->w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  _snwprintf(h->a_cookie, OMOBUS_MAX_ID, L"a_%x", omobus_time());
  h->put_conf(__INT_A_COOKIE, h->a_cookie);
  DEBUGMSG(TRUE, (L"a_cookie/begin: %s\n", h->a_cookie));
}

static
void deinitWorkCookie(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"a_cookie/end: %s\n", h->a_cookie));
  h->put_conf(__INT_A_COOKIE, NULL);
  memset(h->a_cookie, 0, sizeof(h->a_cookie));
  h->w_cookie = NULL;
}

static
bool WriteJournal(HTERMINAL *h, time_t tt) 
{
  std::wstring txt, templ;

  txt = wsfromRes(IDS_J_USER_ACTIVITY, true);
  templ = txt;
  if( txt.empty() ) {
    return false;
  }

  wsrepl(txt, L"%begin%", wsftime(h->ttBegin).c_str());
  wsrepl(txt, L"%end%", wsftime(tt).c_str());
  wsrepl(txt, L"%activity_type_id%", h->at_conf.activity_type_id);
  wsrepl(txt, L"%account_id%", h->ac_conf.account_id);
  
  return WriteOmobusJournal(OMOBUS_USER_ACTIVITY, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
bool _WritePk(HTERMINAL *h, omobus_location_t *pos, time_t tt, const wchar_t *state) 
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
  wsrepl(xml, L"%account_id%", fix_xml(h->ac_conf.account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(h->at_conf.activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(pos->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%state%", state);
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  wsrepl(xml, L"%a_cookie%", h->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_ACTIVITY, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteBeginActivity(HTERMINAL *h, omobus_location_t *pos, time_t tt) 
{
  return _WritePk(h, pos, tt, L"begin");
}

static
bool WriteEndActivity(HTERMINAL *h, omobus_location_t *pos, time_t tt) 
{
  return _WritePk(h, pos, tt, L"end");
}

static
bool WriteActPk(HTERMINAL *h, uint64_t &doc_id, omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml;
  
  xml = wsfromRes(IDS_X_USER_DOCUMENT, true);
  if( xml.empty() ) {
    return false;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(h->ac_conf.account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(h->at_conf.activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", h->w_cookie);
  wsrepl(xml, L"%a_cookie%", h->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
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
std::wstring GetTitle(HTERMINAL *h) {
  int status = getStatus(h);
  std::wstring msg = wsfromRes(status & OMOBUS_SCREEN_WORK ? IDS_TITLE0 : IDS_TITLE1);
  if( !(status & OMOBUS_SCREEN_WORK) ) {
    msg += wcsupr(h->at_conf.descr);
  }
  return msg;
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) 
{
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(3), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
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
  DrawText(hDC, GetTitle(h).c_str(), -1, &rect, DT_RIGHT|DT_WORD_ELLIPSIS|DT_SINGLELINE);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
int MainDialog(HTERMINAL *h) 
{
  int rc;
  activity_types_t at;
  accounts_t ac;
  vf_gr_t vf_gr;
  my_t my; 
  user_activities_t ua;
  routes_t pr;
  simple_t potens;
  omobus_location_t gpsPos = {0};
  wchar_t c_date[11] = L"";
  __account_cookie ck_ac = { &ac, &my, &ua, &pr, &potens, &gpsPos, h->radius };
  __user_activity_cookie ck_ua = { c_date, &ua };
  __routes_cookie ck_pr = { c_date, &pr };
  vf_accounts_cookie_t vf_cookie = { &vf_gr, &ac };

  _WaitCursor.Set();
  omobus_location(&gpsPos);
  wcsncpy(c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(c_date));
  ATLTRACE(L"c_date = %s\n", c_date);
  ReadOmobusJournal(OMOBUS_USER_ACTIVITY, &ck_ua, __user_activity);
  ATLTRACE(OMOBUS_USER_ACTIVITY L" = %i\n", ua.size());
  ReadOmobusManual(L"my_routes", &ck_pr, __routes);
  ATLTRACE(L"my_routes = %i\n", pr.size());
  ReadOmobusManual(L"my_accounts", &my, __my);
  ATLTRACE(L"my_accounts = %i\n", my.size());
  ReadOmobusManual(L"activity_types", &at, __act_type);
  ATLTRACE(L"activity_types = %i\n", at.size());
  ReadOmobusManual(L"potentials", &potens, __simple);
  ATLTRACE(L"potentials = %i\n", potens.size());
  ReadOmobusManual(L"accounts", &ck_ac, __account);
  ATLTRACE(L"accounts = %i/%i (size=%i Kb)\n", sizeof(ACCOUNT_CONF), ac.size(), sizeof(ACCOUNT_CONF)*ac.size()/1024);
  ReadOmobusManual(L"vf_names", &vf_gr, __vf_names);
  ReadOmobusManual(L"vf_accounts", &vf_cookie, __vf_accounts);
  ATLTRACE(L"vf_names = %i\n", vf_gr.size());
  _WaitCursor.Restore();

  rc = CAccountsFrame(h, &ac, &vf_gr, &at, &ua, !my.empty(), !pr.empty()).DoModal(h->hParWnd);
  _man_cleanup<accounts_t>(ac);
  std::for_each(vf_gr.begin(), vf_gr.end(), vf_gr_cleanup());

  return rc;
}

static
bool OnStart(HTERMINAL *h) 
{
  h->ttBegin = 0;
  memset(&h->posBegin, 0, sizeof(h->posBegin));

  if( MainDialog(h) != ID_ACTION ) {
    return false;
  }

  _WaitCursor.Set();
  initWorkCookie(h);
  if( !h->posBegin.location_status ) {
    omobus_location(&h->posBegin);
    h->ttBegin = omobus_time();
  }

  h->put_conf(__INT_ACTIVITY_TYPE_ID, h->at_conf.activity_type_id);
  h->put_conf(__INT_ACCOUNT_ID, h->ac_conf.account_id);
  h->put_conf(__INT_ACCOUNT, h->ac_conf.descr);
  h->put_conf(__INT_ACCOUNT_ADDR, h->ac_conf.address);
  h->put_conf(__INT_ACCOUNT_GR_PRICE_ID, h->ac_conf.group_price_id);
  h->put_conf(__INT_ACCOUNT_PAY_DELAY, itows(h->ac_conf.pay_delay).c_str());
  h->put_conf(__INT_ACCOUNT_LOCKED, itows(h->ac_conf.locked).c_str());

  WriteBeginActivity(h, &h->posBegin, h->ttBegin);
  _exec_slist(h->act_begin);
  _WaitCursor.Restore();

  return true;
}

static
bool OnStop(HTERMINAL *h) 
{
  time_t ttEnd;
  omobus_location_t posEnd;

  if( MessageBoxEx(h->hParWnd, IDS_STR2, IDS_INFO, MB_ICONQUESTION|MB_YESNO) == IDNO ) {
    return false;
  }
  if( h->at_conf.docs > (byte_t)_wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0")) ) {
    MessageBoxEx(h->hParWnd, IDS_STR3, IDS_ERROR, MB_ICONSTOP);
    return false;
  }

  _WaitCursor.Set();
  omobus_location(&posEnd);
  ttEnd = omobus_time();

  WriteEndActivity(h, &posEnd, ttEnd);
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

  memset(&h->at_conf, 0, sizeof(h->at_conf));
  memset(&h->ac_conf, 0, sizeof(h->ac_conf));
  h->ttBegin = 0;
  memset(&h->posBegin, 0, sizeof(h->posBegin));

  _exec_slist(h->act_end);
  _WaitCursor.Restore();

  return true;
}

static 
void OnAction(HTERMINAL *h) {
  if( getStatus(h) & OMOBUS_SCREEN_WORK ) {
    if( OnStart(h) ) {
      putStatus(h, OMOBUS_SCREEN_ACTIVITY);
    }
  } else {
    if( OnStop(h) )
      putStatus(h, OMOBUS_SCREEN_WORK);
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
  
  if( ( h= new HTERMINAL) == NULL ) {
    return NULL;
  }
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->radius = _wtoi(get_conf(OMOBUS_ACTMGR_RADIUS, OMOBUS_ACTMGR_RADIUS_DEF));
  h->act_begin = get_conf(OMOBUS_ACTMGR_BEGIN, OMOBUS_ACTMGR_BEGIN_DEF);
  h->act_end = get_conf(OMOBUS_ACTMGR_END, OMOBUS_ACTMGR_END_DEF);
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME, ISO_DATETIME_FMT);
  
  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  
  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK|OMOBUS_SCREEN_ACTIVITY|
      OMOBUS_SCREEN_ACTIVITY_TAB_1);
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
