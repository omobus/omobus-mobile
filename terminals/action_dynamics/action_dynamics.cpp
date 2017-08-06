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

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-action_dynamics-terminal-element";
static int _nHeight = 20;

struct TOTAL_CONF {
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  cur_t amount_c, amount_r, markup;
};
typedef std::vector<TOTAL_CONF> totals_t;

struct PRODUCT_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID+1], descr[OMOBUS_MAX_DESCR+1];
  cur_t amount_c, amount_r, markup;
  int color, bgcolor;
};
typedef std::vector<PRODUCT_CONF> products_t;

struct DYNAMIC_CONF {
  wchar_t account_id[OMOBUS_MAX_ID+1], descr[OMOBUS_MAX_DESCR+1], address[OMOBUS_MAX_ADDR_DESCR+1];
  totals_t t;
  products_t p;
};
typedef std::vector<DYNAMIC_CONF> dynamics_t;

struct loader_cookit_t {
  const wchar_t *account_id; 
  dynamics_t dyn;
  int accounts, products;
};

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  bool focus;
  int cur_prec;
  const wchar_t *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontL, hFontLB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

class find_account :
  public std::unary_function<DYNAMIC_CONF, bool> {
private:
  const wchar_t *m_account_id;
public:
  find_account(const wchar_t *id) : m_account_id(id) {
  }
  bool operator()(DYNAMIC_CONF &v) const {
    return COMPARE_ID(m_account_id, v.account_id);
  }
};

class CDetDlg : 
	public CStdDialogResizeImpl<CDetDlg>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  DYNAMIC_CONF *m_dyn;
  CListViewCtrl m_list;

public:
  CDetDlg(HTERMINAL *hterm, DYNAMIC_CONF *dyn) : m_hterm(hterm), m_dyn(dyn) {
  }

	enum { IDD = IDD_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CDetDlg)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CDetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CDetDlg>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONE, SHCMBF_HIDESIPBUTTON);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_dyn == NULL ? 0 : m_dyn->p.size());
    m_list.ShowWindow(m_dyn == NULL || m_dyn->p.empty()?SW_HIDE:SW_NORMAL);
		return bHandled = FALSE;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    int right = rect.right;
    rect.top = GetTitleHeight();
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP0), -1,
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    // Total #1
    TOTAL_CONF *t = m_dyn == NULL || m_dyn->t.empty() ? NULL : &m_dyn->t.at(0);

    rect.right = right - scrWidth;
    rect.left = 0;
    rect.top += DRA::SCALEY(18 + 2);
    rect.bottom = rect.top + DRA::SCALEY(17);
    if( t != NULL ) {
      dc.DrawText(t->descr, wcslen(t->descr),
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(17);
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->markup, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    dc.DrawLine(0, rect.top, right, rect.top);
    dc.DrawLine(0, rect.bottom, right, rect.bottom);

    // Total #2
    t = m_dyn == NULL || m_dyn->t.size() < 1 ? NULL : &m_dyn->t.at(1);

    rect.right = right - scrWidth;
    rect.left = 0;
    rect.top += DRA::SCALEY(19);
    rect.bottom = rect.top + DRA::SCALEY(17);
    if( t != NULL ) {
      dc.DrawText(t->descr, wcslen(t->descr),
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(17);
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->markup, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    dc.DrawLine(0, rect.top, right, rect.top);
    dc.DrawLine(0, rect.bottom, right, rect.bottom);

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    DoPaintTitle();
		return FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    PRODUCT_CONF *ptr = &m_dyn->p.at(lpdis->itemID);
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFontL);
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
    rect.bottom = rect.top + DRA::SCALEY(17);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->markup, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

class CMainDlg : 
  public CStdDialogResizeImpl<CMainDlg>,
  public CUpdateUI<CMainDlg>,
  public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  dynamics_t *m_dyn;
  CListViewCtrl m_list;

public:
  CMainDlg(HTERMINAL *hterm, dynamics_t *dyn) : m_hterm(hterm), m_dyn(dyn) {
  }

	enum { IDD = IDD_MAIN };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMainDlg)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CMainDlg)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnDet)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMainDlg>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_dyn->size());
    m_list.ShowWindow(m_dyn->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    rect.top = GetTitleHeight();
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP0), -1,
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    DoPaintTitle();
		return FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    DYNAMIC_CONF *ptr = &m_dyn->at(lpdis->itemID);
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
      dc.FillRect(&lpdis->rcItem, (lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd));
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(2);
    rect.bottom = rect.top + DRA::SCALEY(15);
    dc.SelectFont(m_hterm->hFontLB);
    dc.DrawText(ptr->descr, wcslen(ptr->descr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    dc.SelectFont(m_hterm->hFontL);

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(28);
    dc.DrawText(ptr->address, wcslen(ptr->address), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK|DT_END_ELLIPSIS);

    // Total #1
    TOTAL_CONF *t = ptr->t.empty() ? NULL : &ptr->t.at(0);

    rect.right = lpdis->rcItem.right - scrWidth;
    rect.left = 0;
    rect.top += DRA::SCALEY(28 + 2);
    rect.bottom = rect.top + DRA::SCALEY(17);
    if( t != NULL ) {
      dc.DrawText(t->descr, wcslen(t->descr),
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(17);
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->markup, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    dc.DrawLine(0, rect.top, lpdis->rcItem.right, rect.top);
    dc.DrawLine(0, rect.bottom, lpdis->rcItem.right, rect.bottom);

    // Total #2
    t = ptr->t.size() < 1 ? NULL : &ptr->t.at(1);

    rect.right = lpdis->rcItem.right - scrWidth;
    rect.left = 0;
    rect.top += DRA::SCALEY(19);
    rect.bottom = rect.top + DRA::SCALEY(17);
    if( t != NULL ) {
      dc.DrawText(t->descr, wcslen(t->descr),
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(17);
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->markup, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( t != NULL ) {
      dc.SelectFont(m_hterm->hFontLB);
      dc.DrawText(ftows(t->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_hterm->hFontL);
    }

    dc.DrawLine(0, rect.top, lpdis->rcItem.right, rect.top);
    dc.DrawLine(0, rect.bottom, lpdis->rcItem.right, rect.bottom);

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
    lmi->itemHeight = DRA::SCALEY(118);
    return 1;
  }
  
  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Det(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

	LRESULT OnDet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Det(m_list.GetSelectedIndex());
		return 0;
	}

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}
  
private:
  bool _IsValid(size_t sel) {
    return sel < m_dyn->size() && !m_dyn->at(sel).p.empty();
  }

  void _Det(size_t sel) {
    if( _IsValid(sel) ) {
      CDetDlg(m_hterm, &m_dyn->at(sel)).DoModal(m_hWnd);
    }
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, _IsValid(m_list.GetSelectedIndex()));
		UIUpdateToolBar();
  }
};

// Format: %account_id%;%descr%;%amount_c%;%amount_r%;%markup%;
static 
bool __action_dynamics(void *cookie, int line, const wchar_t **argv, int count) 
{
  loader_cookit_t *ptr;
  dynamics_t::iterator it;
  TOTAL_CONF cnf;

  if( line == 0 || count < 5 || (ptr = (loader_cookit_t *) cookie) == NULL ) {
    return line == 0 ? true : false;
  }
  if( !(ptr->account_id == NULL || COMPARE_ID(ptr->account_id, argv[0])) ) {
    return true;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.descr, argv[1]);
  cnf.amount_c = wcstof(argv[2]);
  cnf.amount_r = wcstof(argv[3]);
  cnf.markup = wcstof(argv[4]);

  if( (it = std::find_if(ptr->dyn.begin(), ptr->dyn.end(), find_account(argv[0]))) == ptr->dyn.end() ) {
    DYNAMIC_CONF x;
    COPY_ATTR__S(x.account_id, argv[0]);
    x.descr[0] = '\0'; x.address[0] = '\0';
    ptr->dyn.push_back(x);
    ptr->dyn.back().t.push_back(cnf);
    ptr->accounts++;
  } else {
    it->t.push_back(cnf);
  }

  return true;
}

// Format: %account_id%;%prod_id%;%amount_c%;%amount_r%;%markup%;%color%;%bgcolor%;
static 
bool __action_dynamics_products(void *cookie, int line, const wchar_t **argv, int count) 
{
  loader_cookit_t *ptr;
  dynamics_t::iterator it;
  PRODUCT_CONF cnf;

  if( line == 0 || count < 7 || (ptr = (loader_cookit_t *)cookie) == NULL ) {
    return line == 0 ? true : false;
  }
  if( !(ptr->account_id == NULL || COMPARE_ID(ptr->account_id, argv[0])) ) {
    return true;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.prod_id, argv[1]);
  cnf.amount_c = wcstof(argv[2]);
  cnf.amount_r = wcstof(argv[3]);
  cnf.markup = wcstof(argv[4]);
  cnf.color = _wtoi(argv[5]);
  cnf.bgcolor = _wtoi(argv[6]);

  if( (it = std::find_if(ptr->dyn.begin(), ptr->dyn.end(), find_account(argv[0]))) == ptr->dyn.end() ) {
    DYNAMIC_CONF x;
    COPY_ATTR__S(x.account_id, argv[0]);
    x.descr[0] = '\0'; x.address[0] = '\0';
    ptr->dyn.push_back(x);
    ptr->dyn.back().p.push_back(cnf);
    ptr->accounts++;
  } else {
    it->p.push_back(cnf);
  }
  ptr->products++;

  return true;
}

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;...
static 
bool __accounts(void *cookie, int line, const wchar_t **argv, int count) 
{
  loader_cookit_t *ptr;
  dynamics_t::iterator it;

  if( line == 0 || count < 6 || (ptr = (loader_cookit_t *)cookie) == NULL ) {
    return line == 0 ? true : false;
  }
  if( _wtoi(argv[2]) == 0 && (it = std::find_if(ptr->dyn.begin(), ptr->dyn.end(), find_account(argv[1]))) != ptr->dyn.end() ) {
    COPY_ATTR__S(it->descr, argv[4]);
    COPY_ATTR__S(it->address, argv[5]);
    ptr->accounts--;
  }

  return ptr->accounts > 0;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;...
static 
bool __products(void *cookie, int line, const wchar_t **argv, int count) 
{
  loader_cookit_t *ptr;
  PRODUCT_CONF *prod;

  if( line == 0 || count < 6 || (ptr = (loader_cookit_t *)cookie) == NULL ) {
    return line == 0 ? true : false;
  }
  if( _wtoi(argv[2]) == 0 ) {
    for( size_t i = 0, size0 = ptr->dyn.size(); i < size0; ++i ) {
      for( size_t j = 0, size1 = ptr->dyn.at(i).p.size(); j < size1; ++j ) {
        prod = &ptr->dyn.at(i).p.at(j);
        if( COMPARE_ID(prod->prod_id, argv[1]) ) {
          COPY_ATTR__S(prod->descr, argv[5]);
          ptr->products--;
        }
      }
    }
  }

  return ptr->products > 0;
}

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  AtlLoadString(uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
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
  DrawText(hDC, wsfromRes(IDS_TITLE).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
void OnShow(HTERMINAL *h) {
}

static
void OnHide(HTERMINAL *h) {
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor wc;
  loader_cookit_t cookie;

  cookie.account_id = h->get_conf(__INT_ACCOUNT_ID, NULL);
  cookie.accounts = cookie.products = 0;

  ReadOmobusManual(L"action_dynamics", &cookie, __action_dynamics);
  ReadOmobusManual(L"action_dynamics_products", &cookie, __action_dynamics_products);
  if( cookie.account_id == NULL ) {
    ReadOmobusManual(L"accounts", &cookie, __accounts);
  }
  ReadOmobusManual(L"products", &cookie, __products);
  wc.Restore();
  if( cookie.account_id != NULL ) {
    CDetDlg(h, cookie.dyn.empty() ? NULL : &cookie.dyn.at(0)).DoModal(h->hParWnd);
  } else {
    CMainDlg(h, &cookie.dyn).DoModal(h->hParWnd);
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

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->get_conf = get_conf;
  h->hParWnd = hParWnd;
  h->cur_prec = _wtoi(get_conf(OMOBUS_CUR_PRECISION, OMOBUS_CUR_PRECISION_DEF));
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontL = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  h->hFontLB = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK|OMOBUS_SCREEN_ACTIVITY);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *)ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontL);
  chk_delete_object(h->hFontLB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
