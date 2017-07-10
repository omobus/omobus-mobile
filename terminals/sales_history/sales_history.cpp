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
#include <omobus-mobile.h>
#include "resource.h"

#define RECLAMATIONS      L"sales_history->reclamations"
#define RECLAMATIONS_DEF  L"yes"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-sales_history-terminal-element";
static int _nHeight = 20;

struct SALES_H {
  cur_t amount_c, amount_r; // Сумма продажи и возврата
  qty_t qty_c, qty_r; // Колличество продажи и возврата
  int32_t color, bgcolor;
};
struct SALES_H_CONF : public SALES_H {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  tm date;
};
typedef std::vector<SALES_H_CONF> sales_h_t;

struct PRODUCT_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  wchar_t code[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  SALES_H tsh; // Итог продаж и возвратов по продукту
};
typedef std::vector<PRODUCT_CONF> products_t;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  bool focus, recls;
  int cur_prec, qty_prec;
  const wchar_t *datetime_fmt, *date_fmt, *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB, hFontL, hFontLB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  const wchar_t *account_id; 
  products_t *prods; 
  sales_h_t *sales_h;
  cur_t sales;
  bool r_sales_valid;
} HTERMINAL;

class _total_sales_h
{
public:
  _total_sales_h(const wchar_t *prod_id, SALES_H *sh) : 
      m_prod_id(prod_id), m_sh(sh) {
    m_sh->color = m_sh->bgcolor = 0;
  }
  void operator()(SALES_H_CONF &el) {
    if( m_prod_id == NULL || COMPARE_ID(m_prod_id, el.prod_id) ) {
      m_sh->amount_c += el.amount_c; m_sh->amount_r += el.amount_r;
      m_sh->qty_c += el.qty_c; m_sh->qty_r += el.qty_r;
    }
  }

  const wchar_t *m_prod_id;
  SALES_H *m_sh;
};

class _prod_sales_h
{
public:
  _prod_sales_h(const wchar_t *prod_id, sales_h_t *psh) : 
      m_prod_id(prod_id), m_psh(psh) {
  }
  void operator()(SALES_H_CONF &el) const {
    if( m_prod_id != NULL && COMPARE_ID(m_prod_id, el.prod_id) )
      m_psh->push_back(el);
  }

  const wchar_t *m_prod_id;
  sales_h_t *m_psh;
};

class CSaHDetDlg : 
  public CStdDialogResizeImpl<CSaHDetDlg>,
  public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  CListViewCtrl m_list;
  PRODUCT_CONF *m_prod;
  sales_h_t *m_prod_sh;

public:
  CSaHDetDlg(HTERMINAL *hterm, PRODUCT_CONF *prod, sales_h_t *prod_sh) : 
      m_hterm(hterm), m_prod(prod), m_prod_sh(prod_sh) {
  }

	enum { IDD = IDD_SH_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CSaHDetDlg)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CSaHDetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CSaHDetDlg>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONE, SHCMBF_HIDESIPBUTTON);
    SetDlgItemText(IDC_PROD, m_prod->descr);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_prod_sh->size());
    m_list.ShowWindow(m_prod_sh->empty()?SW_HIDE:SW_NORMAL);
		return bHandled = FALSE;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    rect.top = GetTitleHeight() + DRA::SCALEY(40);
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP3), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP4), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

    SALES_H_CONF *ptr = &(*m_prod_sh)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFontLB);
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
    dc.DrawText(wsftime(&ptr->date, m_hterm->date_fmt).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(ftows(ptr->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }

    dc.SelectFont(m_hterm->hFontL);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->qty_c, m_hterm->qty_prec).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2),  
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(ftows(ptr->qty_r, m_hterm->qty_prec).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(20);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }

    if( m_hterm->recls ) {
      dc.DrawLine(rect.left, rect.top + DRA::SCALEY(18), 
        lpdis->rcItem.right, rect.top + DRA::SCALEY(18));
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(itows(lpdis->itemID+1).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top + DRA::SCALEY(2),  
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);

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
    lmi->itemHeight = DRA::SCALEY(m_hterm->recls ? 55 : 37);
    return 1;
  }
};

class CSaHDlg : 
  public CStdDialogResizeImpl<CSaHDlg>,
  public CUpdateUI<CSaHDlg>,
  public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  CListViewCtrl m_list;
  products_t *m_prod;
  sales_h_t *m_sah;

public:
  CSaHDlg(HTERMINAL *hterm, products_t *prod, sales_h_t *sales_h) : 
      m_hterm(hterm), m_prod(prod), m_sah(sales_h) {
  }

	enum { IDD = IDD_SH };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CSaHDlg)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CSaHDlg)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CSaHDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnDet)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CSaHDlg>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_prod->size());
    m_list.ShowWindow(m_prod->empty()?SW_HIDE:SW_NORMAL);
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
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP3), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP4), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

    PRODUCT_CONF *ptr = &(*m_prod)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFontL);
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
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_hterm->hFontLB);
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->tsh.amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(ftows(ptr->tsh.amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }
    dc.SelectFont(m_hterm->hFontL);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->tsh.qty_c, m_hterm->qty_prec).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(ftows(ptr->tsh.qty_r, m_hterm->qty_prec).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(20);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    if( m_hterm->recls ) {
      dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
        rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(20), 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    }

    if( m_hterm->recls ) {
      dc.DrawLine(rect.left, rect.top + DRA::SCALEY(18), 
        lpdis->rcItem.right, rect.top + DRA::SCALEY(18));
    }

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(itows(lpdis->itemID+1).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top + DRA::SCALEY(2), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);

    dc.DrawLine(lpdis->rcItem.left, rect.top, 
      lpdis->rcItem.right, rect.top);
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
    lmi->itemHeight = DRA::SCALEY(m_hterm->recls ? 55 : 37);
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
  void _Det(size_t sel) {
    if( sel < (*m_prod).size() ) {
      PRODUCT_CONF *ptr = &(*m_prod)[sel];
      sales_h_t prod_sh;
      std::for_each(m_sah->begin(), m_sah->end(), _prod_sales_h(ptr->prod_id, &prod_sh));
      if( !prod_sh.empty() )
        CSaHDetDlg(m_hterm, ptr, &prod_sh).DoModal(m_hWnd);
    }
  }

  void _LockMainMenuItems() {
    size_t sel = m_list.GetSelectedIndex();
    UIEnable(ID_ACTION, sel < (*m_prod).size());
		UIUpdateToolBar();
  }
};

// Format: %account_id%;%prod_id%;%descr%;%amount_c%;%pack_c_id%;%qty_c%;%amount_r%;%pack_r_id%;%qty_r%;%color%;%bgcolor%;
static 
bool __sales_h(void *cookie, int line, const wchar_t **argv, int count)
{
  HTERMINAL *ptr = (HTERMINAL *)cookie;
  SALES_H_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 11 ) {
    return false;
  }
  if( !COMPARE_ID(ptr->account_id, argv[0]) ) {
    return true;
  }

  memset(&cnf, 0, sizeof(SALES_H_CONF));
  COPY_ATTR__S(cnf.prod_id, argv[1]);
  date2tm(argv[2], &cnf.date);
  cnf.amount_c = wcstof(argv[3]);
  cnf.qty_c = wcstof(argv[5]);
  cnf.amount_r = wcstof(argv[6]);
  cnf.qty_r = wcstof(argv[8]);
  cnf.color = _wtoi(argv[9]);
  cnf.bgcolor = _wtoi(argv[10]);

  ptr->sales_h->push_back(cnf);

  return true;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;(игнорируется)
static 
bool __product(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *ptr = (HTERMINAL *)cookie;
  byte_t ftype;
  const wchar_t *prod_id;
  PRODUCT_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 6 ) {
    return false;
  }
  if( (ftype = (byte_t)_wtoi(argv[2])) != 0 ) {
    return true;
  }

  prod_id = argv[1];
  memset(&cnf, 0, sizeof(PRODUCT_CONF));
  COPY_ATTR__S(cnf.prod_id, prod_id);
  COPY_ATTR__S(cnf.code, argv[3]);
  COPY_ATTR__S(cnf.descr, argv[5]);
  std::for_each(ptr->sales_h->begin(), ptr->sales_h->end(), _total_sales_h(prod_id, &cnf.tsh));
  if( cnf.tsh.amount_c > 0 || cnf.tsh.amount_r > 0 )
    ptr->prods->push_back(cnf);

  return true;
}

// Format: %account_id%;%amount%;
static 
bool __sales(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *ptr = (HTERMINAL *)cookie;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 2 ) {
    return false;
  }
  if( !COMPARE_ID(ptr->account_id, argv[0]) ) {
    return true;
  }

  ptr->sales = wcstof(argv[1]);

  return false;
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
  if( h->r_sales_valid ) {
    DrawText(hDC, wsfromRes(IDS_TITLE_0).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);
    SelectObject(hDC, h->hFontB);
    DrawText(hDC, ftows(h->sales, h->cur_prec, h->thousand_sep).c_str(), -1, &rect, 
      DT_RIGHT|DT_SINGLELINE);
  } else {
    DrawText(hDC, wsfromRes(IDS_TITLE_1).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);
  }

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
void OnShow(HTERMINAL *h) {
  h->account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  h->sales = 0.0;
  h->r_sales_valid = ReadOmobusManual(L"sales", h, __sales) >= 0 /*(!=OMOBUS_ERR)*/;
}

static
void OnHide(HTERMINAL *h) {
  h->account_id = NULL;
  h->sales = 0.0;
  h->sales_h = NULL;
  h->prods = NULL;
  h->r_sales_valid = false;
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor _wc;
  products_t prod; 
  sales_h_t sales_h;
  h->prods = &prod; 
  h->sales_h = &sales_h;

  if( ReadOmobusManual(L"sales_history", h, __sales_h) != OMOBUS_ERR ) {
    ATLTRACE(L"sales_history = %i/%i (size=%i Kb)\n", sizeof(SALES_H_CONF), sales_h.size(), sizeof(SALES_H_CONF)*sales_h.size()/1024);
    ReadOmobusManual(L"products", h, __product);
    ATLTRACE(L"products = %i/%i (size=%i Kb)\n", sizeof(PRODUCT_CONF), prod.size(), sizeof(PRODUCT_CONF)*prod.size()/1024);
    _wc.Restore();
    CSaHDlg(h, &prod, &sales_h).DoModal(h->hParWnd);
  }

  h->prods = NULL; 
  h->sales_h = NULL;
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

  if( (h = new HTERMINAL) == NULL ) {
    return NULL;
  }

  memset(h, 0, sizeof(HTERMINAL));
  h->get_conf = get_conf;
  h->hParWnd = hParWnd;
  h->cur_prec = _wtoi(get_conf(OMOBUS_CUR_PRECISION, OMOBUS_CUR_PRECISION_DEF));
  h->qty_prec = _wtoi(get_conf(OMOBUS_QTY_PRECISION, OMOBUS_QTY_PRECISION_DEF));
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME, ISO_DATETIME_FMT);
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);
  h->recls = wcsistrue(get_conf(RECLAMATIONS, RECLAMATIONS_DEF));

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hFontL = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  h->hFontLB = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_ACTIVITY);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontB);
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
