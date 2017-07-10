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
#include <atlomobus.h>
#include <atllabel.h>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-route_history-terminal-element";
static int _nHeight = 20;
static CString _fmt0, _fmt1, _fmt2, _fmt3;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

struct ACCOUNT_CONF {
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  wchar_t address[OMOBUS_MAX_ADDR_DESCR+1];
  wchar_t chan[OMOBUS_MAX_DESCR + 1];
  SIMPLE_CONF *poten;
};
typedef std::vector<ACCOUNT_CONF*> accounts_t;

struct USER_CONF {
  wchar_t user_id[OMOBUS_MAX_ID + 1], pid[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  bool ftype;
  byte_t level;
};
typedef std::vector<USER_CONF*> users_t;

struct PRODUCT_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  SIMPLE_CONF *brand;
};
typedef std::vector<PRODUCT_CONF*> products_t;

struct COMMENT_CONF {
  USER_CONF *user;
  wchar_t a_cookie[OMOBUS_MAX_ID + 1];
  SIMPLE_CONF *brand, *comment_type;
  wchar_t note[OMOBUS_MAX_NOTE + 1];
};
typedef std::vector<COMMENT_CONF*> comments_t;

struct PRESENCE_CONF {
  USER_CONF *user;
  wchar_t a_cookie[OMOBUS_MAX_ID + 1];
  PRODUCT_CONF *prod;
  int32_t facing, stock;
};
typedef std::vector<PRESENCE_CONF*> presence_t;

struct ROUTE_CONF {
  wchar_t a_cookie[OMOBUS_MAX_ID + 1];
  bool closed, warn, canceled;
  int32_t dist, duration, color, bgcolor;
  tm route_date, b_dt, e_dt;
  USER_CONF *user;
  ACCOUNT_CONF *account;
  SIMPLE_CONF *activity_type;
};
typedef std::vector<ROUTE_CONF*> routes_t;

struct USER_ROUTES_CONF {
  USER_CONF *user;
  int32_t total, closed, warn;
  routes_t routes;
};
typedef std::vector<USER_ROUTES_CONF*> user_routes_t;

struct DATE_ROUTES_CONF {
  tm route_date;
  int32_t total, closed, warn;
  user_routes_t user_routes;
};
typedef std::vector<DATE_ROUTES_CONF*> date_routes_t;

typedef struct _tagHTERMINAL {
  bool focus;
  const wchar_t *date_fmt, *datetime_fmt, *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

typedef struct _tag_cookie {
  HANDLE heap;
  date_routes_t routes;
  simple_t potens, activity_types, brands, comment_types;
  accounts_t accounts;
  users_t users;
  products_t products;
  comments_t comments;
  presence_t presence;
} cookie_t;

inline UINT getStatusResID(const ROUTE_CONF *route) {
  UINT rc = -1;
  if( route->closed ) {
    if( route->warn ) {
      rc = IDS_STATUS_WARN;
    } else {
      rc = IDS_STATUS_CLOSED;
    }
  } else if( route->canceled ) {
    rc = IDS_STATUS_CANCELED;
  }
  return rc;
}

inline bool isClosed(ROUTE_CONF *v) {
  return v->closed || v->canceled;
}

inline bool isWarn(ROUTE_CONF *v) {
  return v->closed && v->warn;
}

inline bool checkClosed(const wchar_t *v) {
  return wcscmp(v, L"closed") == 0 || wcscmp(v, L"1") == 0;
}

inline bool checkCanceled(const wchar_t *v) {
  return wcscmp(v, L"canceled") == 0 || wcscmp(v, L"2") == 0;
}

class find_simple :
  public std::unary_function<SIMPLE_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  find_simple(const wchar_t *id) : m_id(id) {
  }
  bool operator()(SIMPLE_CONF &v) const {
    return COMPARE_ID(m_id, v.id);
  }
};

class find_account :
  public std::unary_function<ACCOUNT_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  find_account(const wchar_t *id) : m_id(id) {
  }
  bool operator()(ACCOUNT_CONF *v) const {
    return COMPARE_ID(m_id, v->account_id);
  }
};

class find_user :
  public std::unary_function<USER_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  find_user(const wchar_t *id) : m_id(id) {
  }
  bool operator()(USER_CONF *v) const {
    return COMPARE_ID(m_id, v->user_id);
  }
};

class find_product :
  public std::unary_function<PRODUCT_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  find_product(const wchar_t *id) : m_id(id) {
  }
  bool operator()(PRODUCT_CONF *v) const {
    return COMPARE_ID(m_id, v->prod_id);
  }
};

class find_date_route :
  public std::unary_function<DATE_ROUTES_CONF, bool> {
private:
  const tm *m_route_date;
public:
  find_date_route(const tm *route_date) : m_route_date(route_date) {
  }
  bool operator()(DATE_ROUTES_CONF *v) const {
    return memcmp(m_route_date, &v->route_date, sizeof(tm)) == 0;
  }
};

class find_user_route :
  public std::unary_function<USER_ROUTES_CONF, bool> {
private:
  const USER_CONF *m_user;
public:
  find_user_route(const USER_CONF *user) : m_user(user) {
  }
  bool operator()(USER_ROUTES_CONF *v) const {
    return COMPARE_ID(m_user->user_id, v->user->user_id);
  }
};

template <class t_Vector, class t_Class>
class copy_data {
private:
  t_Vector *m_result;
  const ROUTE_CONF *m_route;
public:
  copy_data<t_Vector, t_Class>(t_Vector *result, const ROUTE_CONF *route) : 
      m_route(route), m_result(result) {
  }
  void operator()(t_Class *v) const {
    if( COMPARE_ID(m_route->user->user_id, v->user->user_id) &&
        COMPARE_ID(m_route->a_cookie, v->a_cookie) ) {
      m_result->push_back(v);
    }
  }
};

class CCommentsFrame : 
	public CStdDialogResizeImpl<CCommentsFrame>,
  public CUpdateUI<CCommentsFrame>,
	public CMessageFilter
{
private:
  const comments_t *m_comments;
  CLabel m_brand, m_type, m_note;
  size_t m_cur;
  CString m_msg[2], m_pageno;

public:
  CCommentsFrame(const comments_t *comments) : m_comments(comments) {
    m_cur = 0;
  }

	enum { IDD = IDD_COMMENTS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CRouteFrame)
    UPDATE_ELEMENT(ID_NEXT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CCommentsFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CCommentsFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_GESTURE, OnGesture)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_NEXT, OnAction)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CCommentsFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, IDM_MENU_COMMENT, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_brand.SubclassWindow(GetDlgItem(IDC_BRAND));
    m_type.SubclassWindow(GetDlgItem(IDC_TYPE));
    m_note.SubclassWindow(GetDlgItem(IDC_NOTE));
    m_brand.GetWindowText(m_msg[0]);
    m_type.GetWindowText(m_msg[1]);
    DrawData();
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnGesture(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    LRESULT rc = 0;
    GESTUREINFO gi;
    memset(&gi, 0, sizeof(gi));
    gi.cbSize = sizeof(GESTUREINFO);
    if( TKGetGestureInfo((HGESTUREINFO)lParam, &gi) ) {
      if( wParam == GID_PAN ) {
        if( gi.dwFlags & (GF_END|GF_INERTIA) ) {
          if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_RIGHT ) {
            Next();
          } else if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_LEFT ) {
            Next(-1);
          }
          bHandled = TRUE;
          rc = 1;
        }
      }
    }
    return rc;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
		return 0;
	}

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    Next();
		return 0;
	}

  const wchar_t *GetSubTitle() {
    m_pageno = L"";
    if( m_comments->size() > 1 ) {
      m_pageno.Format(L"%i/%i", m_cur + 1, m_comments->size());
    }
    return m_pageno.GetString();
  }


private:
  void _LockMainMenuItems() {
    UIEnable(ID_NEXT, m_comments->size() > 1);
		UIUpdateToolBar();
	}

  COMMENT_CONF *GetCur() {
    return m_cur >= m_comments->size() ? NULL : (*m_comments)[m_cur];
  }

  void DrawData() {
    COMMENT_CONF *cur;
    if( (cur = GetCur()) != NULL ) {
      m_brand.SetText(cur->brand ? cur->brand->descr : m_msg[0]);
      m_brand.SetFontItalic(!cur->brand);
      m_brand.SetFontBold(cur->brand!=NULL);
      m_type.SetText(cur->comment_type ? cur->comment_type->descr : m_msg[1]);
      m_type.SetFontItalic(!cur->comment_type);
      m_note.SetText(cur->note);
    }
    Invalidate();
  }

  void Next(int step=1) {
    RECT rTitle;
    size_t size;
    if( m_comments->empty() ) {
      m_cur = -1;
      return;
    } else {
      size = m_comments->size();
      if( size == 1 ) {
        m_cur = 0;
        return;
      }
      m_cur += step;
      if( m_cur < 0 ) {
        m_cur = size - 1;
      }
      if( m_cur >= size )
        m_cur = 0;
    }
    ::GetClientRect(m_hWnd, &rTitle);
    rTitle.bottom = nTitleHeight;
    DrawData();
    RedrawWindow(&rTitle);
  }
};

class CPresenceFrame : 
  public CStdDialogResizeImpl<CPresenceFrame>,
  public CMessageFilter
{
private:
  const HTERMINAL *m_hterm;
  const presence_t *m_presence;
  CListViewCtrl m_list;
  CFont m_capFont, m_baseFont, m_boldFont;

public:
  CPresenceFrame(const HTERMINAL *hterm, const presence_t *presence) : 
      m_hterm(hterm), m_presence(presence) 
  {
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_PRESENCE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CPresenceFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CPresenceFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CPresenceFrame>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_presence->size());
    m_list.ShowWindow(m_presence->empty()?SW_HIDE:SW_NORMAL);
		return bHandled = FALSE;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_capFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    rect.top = GetTitleHeight();
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(45);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PRESENCE_CAP2), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(45);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PRESENCE_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(100);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    /*dc.DrawText(LoadStringEx(IDS_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);*/

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP0), -1,
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    DoPaintTitle();
		return FALSE;
	}
  
  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    PRESENCE_CONF *ptr = (*m_presence)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_baseFont);
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
    rect.bottom = rect.top + DRA::SCALEY(17);
    dc.DrawText(ptr->prod->descr, wcslen(ptr->prod->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_boldFont);
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(45);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(itows(ptr->stock).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(45);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->facing > 0 ) {
      dc.DrawText(itows(ptr->facing).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    }
    dc.SelectFont(m_baseFont);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(100);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    /*dc.DrawText(wsftime(&pres->date, m_hterm->date_fmt).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);*/

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(itows(lpdis->itemID+1).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    dc.DrawLine(lpdis->rcItem.left, rect.top, 
      lpdis->rcItem.right, rect.top);
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
    lmi->itemHeight = DRA::SCALEY(35);
    return 1;
  }
  
	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}
};

class CDetFrame : 
	public CStdDialogResizeImpl<CDetFrame>,
	public CMessageFilter
{
private:
  const HTERMINAL *m_hterm;
  const ROUTE_CONF *m_route;
  comments_t m_comments;
  presence_t m_presence;
  CLabel m_emptyLabel[9];
  CHyperButtonTempl<CDetFrame> m_hl[2];
  CString m_tmp;

public:
  CDetFrame(const HTERMINAL *hterm, const ROUTE_CONF *route, const comments_t *comments,
    const presence_t *presence) : m_hterm(hterm), m_route(route) 
  {
    std::for_each(comments->begin(), comments->end(), 
      copy_data<comments_t, COMMENT_CONF>(&m_comments, route));
    std::for_each(presence->begin(), presence->end(), 
      copy_data<presence_t, PRESENCE_CONF>(&m_presence, route));
  }

	enum { IDD = IDD_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CDetFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CDetFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CDetFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
    SetWindowText(m_route->activity_type->descr);
    SetDlgItemText(IDC_ACCOUNT, m_route->account->descr);
    SetDlgItemText(IDC_DEL_ADDR, m_route->account->address);
    SetDlgItemText(IDC_USER, m_route->user->descr);
    if( m_route->account->chan[0] != L'\0' ) {
      SetDlgItemText(IDC_CHAN, m_route->account->chan);
    }
    if( m_route->account->poten && m_route->account->poten->descr[0] != L'\0' ) {
      SetDlgItemText(IDC_POTEN, m_route->account->poten->descr);
    }
    if( m_route->duration ) {
      m_tmp.Format(_fmt1, m_route->duration);
      SetDlgItemText(IDC_DURATION, m_tmp);
    }
    if( tmvalid(&m_route->b_dt) ) {
      SetDlgItemText(IDC_BEGIN, wsftime(&m_route->b_dt, m_hterm->datetime_fmt).c_str());
    }
    if( tmvalid(&m_route->e_dt) ) {
      SetDlgItemText(IDC_END, wsftime(&m_route->e_dt, m_hterm->datetime_fmt).c_str());
    }
    if( m_route->dist ) {
      m_tmp.Format(_fmt2, ftows(m_route->dist, 0, m_hterm->thousand_sep).c_str());
      SetDlgItemText(IDC_DIST, m_tmp);
    }
    if( !m_presence.empty() ) {
      SetDlgItemInt(IDC_PRESENCE, m_presence.size(), FALSE);
      _InitLink(m_hl[0], IDC_PRESENCE_DET);
    }
    if( !m_comments.empty() ) {
      SetDlgItemInt(IDC_COMMENTS, m_comments.size(), FALSE);
      _InitLink(m_hl[1], IDC_COMMENTS_DET);
    }
    _InitLabel(m_emptyLabel[0], IDC_CHAN, m_route->account->chan[0] != L'\0');
    _InitLabel(m_emptyLabel[1], IDC_POTEN, m_route->account->poten && m_route->account->poten->descr[0] != L'\0');
    _InitLabel(m_emptyLabel[2], IDC_STATE, m_route->closed || m_route->canceled, TRUE, 
        getStatusResID(m_route));
    _InitLabel(m_emptyLabel[3], IDC_DURATION, m_route->duration, TRUE);
    _InitLabel(m_emptyLabel[4], IDC_DIST, m_route->dist, TRUE);
    _InitLabel(m_emptyLabel[5], IDC_BEGIN, tmvalid(&m_route->b_dt));
    _InitLabel(m_emptyLabel[6], IDC_END, tmvalid(&m_route->e_dt));
    _InitLabel(m_emptyLabel[7], IDC_PRESENCE, !m_presence.empty(), TRUE);
    _InitLabel(m_emptyLabel[8], IDC_COMMENTS, !m_comments.empty(), TRUE);
    _ShowLabel(IDC_DURATION, m_route->closed);
    _ShowLabel(IDC_DURATION_S, m_route->closed);
    _ShowLabel(IDC_BEGIN, m_route->closed);
    _ShowLabel(IDC_BEGIN_S, m_route->closed);
    _ShowLabel(IDC_END, m_route->closed);
    _ShowLabel(IDC_END_S, m_route->closed);
    _ShowLabel(IDC_DIST, m_route->closed);
    _ShowLabel(IDC_DIST_S, m_route->closed);
    _ShowLabel(IDC_PRESENCE, m_route->closed);
    _ShowLabel(IDC_PRESENCE_S, m_route->closed);
    _ShowLabel(IDC_COMMENTS, m_route->closed);
    _ShowLabel(IDC_COMMENTS_S, m_route->closed);
		return bHandled = FALSE;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
		return 0;
	}

  const wchar_t *GetSubTitle() {
    return wsftime(&m_route->route_date, m_hterm->date_fmt).c_str();
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( nID == IDC_PRESENCE_DET ) {
      if( !m_presence.empty() ) {
        CPresenceFrame(m_hterm, &m_presence).DoModal(m_hWnd);
      }
    } else if( nID == IDC_COMMENTS_DET ) {
      if( !m_comments.empty() ) {
        CCommentsFrame(&m_comments).DoModal(m_hWnd);
      }
    }
    return true;
  }

private:
  void _InitLabel(CLabel &ctrl, UINT uID, BOOL bValid, BOOL bBold=FALSE, UINT tID=-1) {
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

  void _InitLink(CHyperButtonTempl<CDetFrame> &ctrl, UINT uID) {
    ctrl.SubclassWindow(GetDlgItem(uID));
    ctrl.SetParent(this, uID);
    ctrl.ShowWindow(SW_SHOWNORMAL);
  }

  void _ShowLabel(UINT uID, bool isVisible = true) {
    HWND hWnd = NULL;
    if( (hWnd = GetDlgItem(uID)) != NULL ) {
      ::ShowWindow(hWnd, isVisible ? SW_SHOWNORMAL : SW_HIDE);
    }
  }
};

class CRouteFrame : 
	public CStdDialogResizeImpl<CRouteFrame>,
  public CUpdateUI<CRouteFrame>,
	public CMessageFilter
{
private:
  const HTERMINAL *m_hterm;
  const tm *m_route_date;
  const routes_t *m_routes;
  const comments_t *m_comments;
  const presence_t *m_presence;
  CListViewCtrl m_list;
  CFont m_baseFont, m_boldFont;

public:
  CRouteFrame(const HTERMINAL *hterm, const tm *route_date, const routes_t *routes, 
    const comments_t *comments, const presence_t *presence) : 
      m_hterm(hterm), m_route_date(route_date), m_routes(routes), 
      m_comments(comments), m_presence(presence)
  {
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CRouteFrame)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CRouteFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CRouteFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
    COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CRouteFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_routes->size());
    m_list.ShowWindow(m_routes->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    ROUTE_CONF *ptr = (*m_routes)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_boldFont);
    CBrush hbrCur = ptr->bgcolor ? CreateSolidBrush(ptr->bgcolor) : NULL;
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      if( ptr->color != 0 )
        dc.SetTextColor(ptr->color);
      dc.FillRect(&lpdis->rcItem, !hbrCur.IsNull() ? hbrCur.m_hBrush : 
        (lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd));
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(2);
    rect.bottom = rect.top + DRA::SCALEY(15);
    dc.DrawText(ptr->account->descr, wcslen(ptr->account->descr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);
    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(28);
    dc.DrawText(ptr->account->address, wcslen(ptr->account->address), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK|DT_END_ELLIPSIS);

    rect.top = rect.bottom + DRA::SCALEY(2);
    rect.bottom = lpdis->rcItem.bottom;
    rect.right = DRA::SCALEX(160);
    dc.DrawText(ptr->account->chan, wcslen(ptr->account->chan), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.left = rect.right + DRA::SCALEX(10);
    rect.right = rect.left + DRA::SCALEX(35);
    if( ptr->account->poten ) {
      dc.DrawText(ptr->account->poten->descr, wcslen(ptr->account->poten->descr), 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE/*|DT_END_ELLIPSIS*/);
    }

    if( ptr->closed ) {
      rect.left = rect.right;
      rect.right = lpdis->rcItem.right - DRA::SCALEX(2) - scrWidth;
      dc.SelectFont(m_boldFont);
      dc.DrawText(L"+", 1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE/*|DT_END_ELLIPSIS*/);
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

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Det(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Det(m_list.GetSelectedIndex());
    return 0;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}

  const wchar_t *GetSubTitle() {
    return wsftime(m_route_date, m_hterm->date_fmt).c_str();
  }

private:
  void _LockMainMenuItems() {
    UIEnable(ID_MENU_OK, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
	}

  void _Det(size_t sel) {
    ROUTE_CONF *ptr;
    if( !(sel >= m_routes->size() || (ptr = (*m_routes)[sel]) == NULL) ) {
      CDetFrame(m_hterm, ptr, m_comments, m_presence).DoModal(m_hWnd);
    }
  }
};

class CUsersFrame : 
	public CStdDialogResizeImpl<CUsersFrame>,
	public CUpdateUI<CUsersFrame>,
	public CMessageFilter
{
protected:
  const HTERMINAL *m_hterm;
  const user_routes_t *m_routes;
  const tm *m_route_date;
  const comments_t *m_comments;
  const presence_t *m_presence;
  CListViewCtrl m_list;
  CFont m_baseFont, m_boldFont;
  CString m_tmp;
  
public:
  CUsersFrame(const HTERMINAL *h, const tm *route_date, const user_routes_t *routes, 
    const comments_t *comments, const presence_t *presence) : 
      m_hterm(h), m_route_date(route_date), m_routes(routes),
      m_comments(comments), m_presence(presence)
  {
    m_boldFont = CreateBaseFont(DRA::SCALEY(14), FW_SEMIBOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CUsersFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CUsersFrame)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CUsersFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CUsersFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_routes->size());
    m_list.ShowWindow(m_routes->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    USER_ROUTES_CONF *ptr = (*m_routes)[lpdis->itemID];
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
    rect.bottom = rect.top + DRA::SCALEY(18);
    dc.DrawText(ptr->user->descr, -1,
      rect.left + DRA::SCALEX(2 + (6 * (ptr->user->level <= 1 ? 0 : ptr->user->level - 1))), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);
    if( ptr->warn ) {
      m_tmp.Format(_fmt3, ptr->total, ptr->closed, ptr->warn);
    } else {
      m_tmp.Format(_fmt0, ptr->total, ptr->closed);
    }
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.DrawText(m_tmp, m_tmp.GetLength(), 
      rect.left + DRA::SCALEX(2 + (6 * (ptr->user->level <= 1 ? 0 : ptr->user->level - 1))), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

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
    lmi->itemHeight = DRA::SCALEY(38);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Det(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Det(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

  const wchar_t *GetSubTitle() {
    return wsftime(m_route_date, m_hterm->date_fmt).c_str();
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_MENU_OK, m_list.GetSelectedIndex() != -1 && _Sel(m_list.GetSelectedIndex()));
		UIUpdateToolBar();
	}

  void _Det(size_t sel) {
    USER_ROUTES_CONF *ptr;
    if( (ptr = _Sel(sel)) ) {
      CRouteFrame(m_hterm, m_route_date, &ptr->routes, m_comments, m_presence).DoModal(m_hWnd);
    }
  }

  USER_ROUTES_CONF *_Sel(size_t sel) {
    USER_ROUTES_CONF *ptr;
    return !(sel >= m_routes->size() || (ptr = (*m_routes)[sel]) == NULL || ptr->routes.empty())
      ? ptr : NULL;
  }
};

class CMainFrame : 
	public CStdDialogResizeImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter
{
protected:
  const HTERMINAL *m_hterm;
  const date_routes_t *m_routes;
  const comments_t *m_comments;
  const presence_t *m_presence;
  CListViewCtrl m_list;
  CFont m_baseFont, m_boldFont;
  CString m_tmp;
  
public:
  CMainFrame(HTERMINAL *h, date_routes_t *routes, const comments_t *comments, 
    const presence_t *presence) : m_hterm(h), m_routes(routes), m_comments(comments),
      m_presence(presence)
  {
    m_boldFont = CreateBaseFont(DRA::SCALEY(14), FW_SEMIBOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMainFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
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
    m_list.SetItemCount(m_routes->size());
    m_list.ShowWindow(m_routes->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    DATE_ROUTES_CONF *ptr = (*m_routes)[lpdis->itemID];
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
    rect.bottom = rect.top + DRA::SCALEY(18);
    dc.DrawText(wsftime(&ptr->route_date, m_hterm->date_fmt).c_str(), -1,
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);
    if( ptr->warn ) {
      m_tmp.Format(_fmt3, ptr->total, ptr->closed, ptr->warn);
    } else {
      m_tmp.Format(_fmt0, ptr->total, ptr->closed);
    }
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.DrawText(m_tmp, m_tmp.GetLength(), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

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
    lmi->itemHeight = DRA::SCALEY(38);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Det(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Det(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_MENU_OK, m_list.GetSelectedIndex() != -1 && _Sel(m_list.GetSelectedIndex()));
		UIUpdateToolBar();
	}

  void _Det(size_t sel) {
    DATE_ROUTES_CONF *ptr;
    if( (ptr = _Sel(sel)) ) {
      if( ptr->user_routes.size() > 1 ) {
        CUsersFrame(m_hterm, &ptr->route_date, &ptr->user_routes, m_comments, m_presence).
          DoModal(m_hWnd);
      } else {
        CRouteFrame(m_hterm, &ptr->route_date, &ptr->user_routes[0]->routes, m_comments, m_presence).
          DoModal(m_hWnd);
      }
    }
  }

  DATE_ROUTES_CONF *_Sel(size_t sel) {
    DATE_ROUTES_CONF *ptr;
    return !(sel >= m_routes->size() || (ptr = (*m_routes)[sel]) == NULL || ptr->user_routes.empty())
      ? ptr : NULL;
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

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;...
static 
bool __accounts(void *cookie, int line, const wchar_t **argv, int argc) 
{
  cookie_t *ptr;
  ACCOUNT_CONF *cnf;
  simple_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 8 || (ptr = (cookie_t *)cookie) == NULL ) {
    return false;
  }
  if( /*ftype*/_wtoi(argv[2]) != 0 ) {
    return true;
  }
  if( (cnf = (ACCOUNT_CONF *)HeapAlloc(ptr->heap, 0, sizeof(ACCOUNT_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(ACCOUNT_CONF));
  COPY_ATTR__S(cnf->account_id, argv[1])
  COPY_ATTR__S(cnf->descr, argv[4]);
  COPY_ATTR__S(cnf->address, argv[5]);
  COPY_ATTR__S(cnf->chan, argv[6]);
  if( (it = std::find_if(ptr->potens.begin(), ptr->potens.end(), find_simple(argv[7])))
        != ptr->potens.end() ) {
    cnf->poten = &(*it);
  }
  ptr->accounts.push_back(cnf);

  return true;
}

// Format: %pid%;%user_id%;%ftype%;%descr%;
static 
bool __users(void *cookie, int line, const wchar_t **argv, int count) 
{
  cookie_t *ptr;
  USER_CONF *cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (cookie_t *)cookie) == NULL || count < 4 ) {
    return false;
  }
  if( (cnf = (USER_CONF *)HeapAlloc(ptr->heap, 0, sizeof(USER_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(USER_CONF));
  COPY_ATTR__S(cnf->pid, argv[0]);
  COPY_ATTR__S(cnf->user_id, argv[1]);
  cnf->ftype = wcsistrue(argv[2]);
  COPY_ATTR__S(cnf->descr, argv[3]);
  ptr->users.push_back(cnf);

  return true;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;%manuf_id%;%shelf_life%;%brand_id%;...
static 
bool __products(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  cookie_t *ptr;
  PRODUCT_CONF *cnf;
  simple_t::iterator it;

  if( ln == 0 ) {
    return true;
  }  
  if( (ptr = (cookie_t *)cookie) == NULL || argc < 9 ) {
    return false;
  }
  if( /*ftype*/_wtoi(argv[2]) != 0 ) {
    return true;
  }
  if( (cnf = (PRODUCT_CONF *)HeapAlloc(ptr->heap, 0, sizeof(PRODUCT_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(PRODUCT_CONF));
  COPY_ATTR__S(cnf->prod_id, argv[1])
  COPY_ATTR__S(cnf->descr, argv[5]);
  if( (it = std::find_if(ptr->brands.begin(), ptr->brands.end(), find_simple(argv[8])))
        != ptr->brands.end() ) {
    cnf->brand = &(*it);
  }
  ptr->products.push_back(cnf);

  return true;
}

// Format: %route_date%;%account_id%;%activity_type_id%;%user_id%;%a_cookie%;%closed%;%warn%;%dist%;%b_dt%;%e_dt%;%color%;%bgcolor%;
static 
bool __r_route_history(void *cookie, int line, const wchar_t **argv, int argc) 
{
  ROUTE_CONF *cnf;
  USER_ROUTES_CONF *ur;
  DATE_ROUTES_CONF *dr;
  cookie_t *ptr;
  simple_t::iterator s_it;
  users_t::iterator u_it;
  accounts_t::iterator a_it;
  date_routes_t::iterator dr_it;
  user_routes_t::iterator ur_it;
  size_t i, size;
  time_t tt_e, tt_b;
  
  if( line == 0 ) {
    return true;
  }
  if( argc < 12 || (ptr = (cookie_t*)cookie) == NULL ) {
    return false;
  }
  if( argv[0][0] == L'\0' || wcslen(argv[0]) < 10 ) {
    return true;
  }
  if( (a_it = std::find_if(ptr->accounts.begin(), ptr->accounts.end(), find_account(argv[1])))
        == ptr->accounts.end() ) {
    ATLTRACE(L"route_history: unable to find account '%s'.\n", argv[1]);
    return true;
  }
  if( (u_it = std::find_if(ptr->users.begin(), ptr->users.end(), find_user(argv[3])))
        == ptr->users.end() ) {
    ATLTRACE(L"route_history: unable to find user '%s'.\n", argv[3]);
    return true;
  }
  if( (s_it = std::find_if(ptr->activity_types.begin(), ptr->activity_types.end(), find_simple(argv[2])))
        == ptr->activity_types.end() ) {
    ATLTRACE(L"route_history: unable to find activity_type '%s'.\n", argv[2]);
    return true;
  }
  if( (cnf = (ROUTE_CONF *)HeapAlloc(ptr->heap, 0, sizeof(ROUTE_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(ROUTE_CONF));
  date2tm(argv[0], &cnf->route_date);
  cnf->account = (*a_it);
  cnf->activity_type = &(*s_it);
  cnf->user = (*u_it);
  COPY_ATTR__S(cnf->a_cookie, argv[4]);
  cnf->closed = checkClosed(argv[5]);
  cnf->canceled = checkCanceled(argv[5]);
  cnf->warn = wcsistrue(argv[6]);
  cnf->dist = _wtoi(argv[7]);
  datetime2tm(argv[8], &cnf->b_dt);
  datetime2tm(argv[9], &cnf->e_dt);
  if( tmvalid(&cnf->b_dt) && tmvalid(&cnf->e_dt) ) {
    if( (tt_e = omobus_utc_mktime(&cnf->e_dt)) >= (tt_b = omobus_utc_mktime(&cnf->b_dt)) ) {
      cnf->duration = (tt_e - tt_b)/60;
    } else {
      cnf->duration = ((int)((tt_b - tt_e)/60))*(-1);
    }
  }
  cnf->color = _wtoi(argv[10]);
  cnf->bgcolor = _wtoi(argv[11]);

  if( (dr_it = std::find_if(ptr->routes.begin(), ptr->routes.end(), find_date_route(&cnf->route_date)))
        == ptr->routes.end() ) {
    if( (dr = (DATE_ROUTES_CONF *)HeapAlloc(ptr->heap, 0, sizeof(DATE_ROUTES_CONF))) == NULL ) {
      ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
      return false;
    }
    memcpy(&dr->route_date, &cnf->route_date, sizeof(tm));
    dr->total = 1;
    dr->closed = isClosed(cnf) ? 1 : 0;
    dr->warn = isWarn(cnf) ? 1 : 0;
    size = ptr->users.size();
    for( i = 0; i < size; i++ ) {
      if( (ur = (USER_ROUTES_CONF *)HeapAlloc(ptr->heap, 0, sizeof(USER_ROUTES_CONF))) == NULL ) {
        ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
        return false;
      }
      ur->total = ur->closed = ur->warn = 0;
      ur->user = ptr->users[i];
      if( COMPARE_ID(cnf->user->user_id, ur->user->user_id) ) {
        ur->total++;
        ur->closed += isClosed(cnf) ? 1 : 0;
        ur->warn += isWarn(cnf) ? 1 : 0;
        ur->routes.push_back(cnf);
      }
      dr->user_routes.push_back(ur);
    }
    ptr->routes.push_back(dr);
  } else {
    if( (ur_it = std::find_if((*dr_it)->user_routes.begin(), (*dr_it)->user_routes.end(), find_user_route(cnf->user)))
          != (*dr_it)->user_routes.end() ) {
      (*ur_it)->total++;
      (*ur_it)->closed += isClosed(cnf) ? 1 : 0;
      (*ur_it)->warn += isWarn(cnf) ? 1 : 0;
      (*ur_it)->routes.push_back(cnf);
      (*dr_it)->total++;
      (*dr_it)->closed += isClosed(cnf) ? 1 : 0;
      (*dr_it)->warn += isWarn(cnf) ? 1 : 0;
    }
  }

  return true;
}

// Format: %user_id%;%a_cookie%;%doc_no%;%brand_id%;%comment_type_id%;%note%;
static 
bool __r_route_history_comments(void *cookie, int line, const wchar_t **argv, int argc) 
{
  COMMENT_CONF *cnf;
  cookie_t *ptr;
  users_t::iterator u_it;
  simple_t::iterator s_it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 6 || (ptr = (cookie_t*)cookie) == NULL ) {
    return false;
  }
  if( (u_it = std::find_if(ptr->users.begin(), ptr->users.end(), find_user(argv[0])))
        == ptr->users.end() ) {
    ATLTRACE(L"route_history: unable to find user '%s'.\n", argv[3]);
    return true;
  }
  if( (cnf = (COMMENT_CONF *)HeapAlloc(ptr->heap, 0, sizeof(COMMENT_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(COMMENT_CONF));
  cnf->user = (*u_it);
  COPY_ATTR__S(cnf->a_cookie, argv[1]);
  //COPY_ATTR__S(cnf->doc_no, argv[2]);
  if( (s_it = std::find_if(ptr->brands.begin(), ptr->brands.end(), find_simple(argv[3])))
        != ptr->brands.end() ) {
    cnf->brand = &(*s_it);
  }
  if( (s_it = std::find_if(ptr->comment_types.begin(), ptr->comment_types.end(), find_simple(argv[4])))
        != ptr->comment_types.end() ) {
    cnf->comment_type = &(*s_it);
  }
  COPY_ATTR__S(cnf->note, argv[5]);

  ptr->comments.push_back(cnf);

  return true;
}

// Format: %user_id%;%a_cookie%;%doc_no%;%prod_id%;%facing%;%stock%;
static 
bool __r_route_history_presence(void *cookie, int line, const wchar_t **argv, int argc) 
{
  PRESENCE_CONF *cnf;
  cookie_t *ptr;
  users_t::iterator u_it;
  products_t::iterator p_it;

  if( line == 0 ) {
    return true;
  }
  if( argc < 6 || (ptr = (cookie_t*)cookie) == NULL ) {
    return false;
  }
  if( (u_it = std::find_if(ptr->users.begin(), ptr->users.end(), find_user(argv[0])))
        == ptr->users.end() ) {
    ATLTRACE(L"route_history: unable to find user '%s'.\n", argv[3]);
    return true;
  }
  if( (cnf = (PRESENCE_CONF *)HeapAlloc(ptr->heap, 0, sizeof(PRESENCE_CONF))) == NULL ) {
    ATLTRACE(L"route_history: unable to allocate memory: %s\n", fmtoserr().c_str());
    return false;
  }

  memset(cnf, 0, sizeof(PRESENCE_CONF));
  cnf->user = (*u_it);
  COPY_ATTR__S(cnf->a_cookie, argv[1]);
  //COPY_ATTR__S(cnf->doc_no, argv[2]);
  if( (p_it = std::find_if(ptr->products.begin(), ptr->products.end(), find_product(argv[3])))
        != ptr->products.end() ) {
    cnf->prod = (*p_it);
  }
  cnf->facing = _wtoi(argv[4]);
  cnf->stock = _wtoi(argv[5]);

  ptr->presence.push_back(cnf);

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
void build_user_hierarchy(const users_t *o_users, users_t *r_users, const wchar_t *pid, byte_t l) 
{
  USER_CONF *ptr = NULL;
  size_t i = 0, size = o_users->size();
  for( ; i < size; i++ ) {
    ptr = (*o_users)[i];
    if( COMPARE_ID(ptr->pid, pid) ) {
      ptr->level = l;
      r_users->push_back(ptr);
      if( ptr->ftype && !COMPARE_ID(ptr->user_id, ptr->pid) ) {
        build_user_hierarchy(o_users, r_users, ptr->user_id, l + 1);
      }
    }
  }
}

static 
void build_user_hierarchy(cookie_t *ck) {
  users_t tmp;
  build_user_hierarchy(&ck->users, &tmp, L"", 0);
  ck->users.clear();
  ck->users = tmp;
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
  DrawText(hDC, wsfromRes(IDS_TITLE).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor _wc;
  cookie_t ck;

  if( (ck.heap = HeapCreate(0, 10*1024*1024, 0)) == NULL ) {
    ATLTRACE(L"route_history: unable to create heap: %s",
      fmtoserr().c_str());
    return;
  }

  ReadOmobusManual(L"potentials", &ck.potens, __simple);
  ATLTRACE(L"potentials = %i\n", ck.potens.size());
  ReadOmobusManual(L"activity_types", &ck.activity_types, __simple);
  ATLTRACE(L"activity_types = %i\n", ck.activity_types.size());
  ReadOmobusManual(L"brands", &ck.brands, __simple);
  ATLTRACE(L"brands = %i\n", ck.brands.size());
  ReadOmobusManual(L"products", &ck, __products);
  ATLTRACE(L"products = %i (size=%iKb)\n", ck.products.size(), 
    sizeof(PRODUCT_CONF)*ck.products.size()/1024);
  ReadOmobusManual(L"comment_types", &ck.comment_types, __simple);
  ATLTRACE(L"comment_types = %i\n", ck.comment_types.size());
  ReadOmobusManual(L"users", &ck, __users);
  ATLTRACE(L"users = %i\n", ck.users.size());
  build_user_hierarchy(&ck);
  ATLTRACE(L"users [after build_user_hierarchy] = %i\n", ck.users.size());
  ReadOmobusManual(L"accounts", &ck, __accounts);
  ATLTRACE(L"accounts = %i (size=%iKb)\n", ck.accounts.size(), 
    sizeof(ACCOUNT_CONF)*ck.accounts.size()/1024);
  ReadOmobusManual(L"route_history", &ck, __r_route_history);
  ATLTRACE(L"route_history = %i\n", ck.routes.size());
  ReadOmobusManual(L"route_history_comments", &ck, __r_route_history_comments);
  ATLTRACE(L"route_history_comments = %i\n", ck.comments.size());
  ReadOmobusManual(L"route_history_presence", &ck, __r_route_history_presence);
  ATLTRACE(L"route_history_presence = %i\n", ck.presence.size());
  _wc.Restore();

  CMainFrame(h, &ck.routes, &ck.comments, &ck.presence).DoModal(h->hParWnd);

  HeapDestroy(ck.heap);
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

  if( (h = new HTERMINAL) == NULL ) {
    return NULL;
  }
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME, ISO_TIME_FMT);
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _fmt0.LoadStringW(IDS_FMT0);
  _fmt1.LoadStringW(IDS_FMT1);
  _fmt2.LoadStringW(IDS_FMT2);
  _fmt3.LoadStringW(IDS_FMT3);

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
