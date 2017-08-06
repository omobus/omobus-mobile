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
static TCHAR _szWindowClass[] = L"omobus-mutual_history-terminal-element";
static int _nHeight = 20;

struct DET_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID+1], prod[OMOBUS_MAX_DESCR+1];
  wchar_t pack_id[OMOBUS_MAX_ID+1], pack[OMOBUS_MAX_DESCR+1];
  qty_t qty;
  cur_t amount;
};
typedef std::vector<DET_CONF> det_t;

struct MUTUAL_CONF {
  wchar_t doc_id[OMOBUS_MAX_ID+1], doc_code[OMOBUS_MAX_DESCR+1];
  tm doc_date;
  cur_t amount;
  int32_t color, bgcolor;
  det_t det;
};
typedef std::vector<MUTUAL_CONF> mutual_t;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  bool focus;
  int cur_prec, qty_prec;
  const wchar_t *datetime_fmt, *date_fmt, *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB, hFontL, hFontLB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  const wchar_t *account_id; 
  mutual_t *muh;
  cur_t mutual;
  bool r_mutuals_valid;
} HTERMINAL;

class find_doc :
  public std::unary_function<MUTUAL_CONF, bool> {
private:
  const wchar_t *m_id;
public:
  find_doc(const wchar_t *id) : m_id(id) {
  }
  bool operator()(MUTUAL_CONF &v) const {
    return COMPARE_ID(m_id, v.doc_id);
  }
};

class CDetDlg : 
	public CStdDialogResizeImpl<CDetDlg>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  det_t *m_det;
  CListViewCtrl m_list;
  const wchar_t *m_doc_code;

public:
  CDetDlg(HTERMINAL *hterm, const wchar_t *doc_code, det_t *l) : 
      m_hterm(hterm), m_doc_code(doc_code), m_det(l) {
  }

	enum { IDD = IDD_DOCDET };

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
    if( m_doc_code != NULL && m_doc_code[0] != L'\0' ) {
      SetWindowText(m_doc_code);
    }
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_det->size());
    m_list.ShowWindow(m_det->empty()?SW_HIDE:SW_NORMAL);
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
    dc.DrawText(LoadStringEx(IDS_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(54);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(60);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP3), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

    DET_CONF *ptr = &(*m_det)[lpdis->itemID];
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
    dc.DrawText(ptr->prod, wcslen(ptr->prod),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->amount > 0.0 ) {
      dc.DrawText(ftows(ptr->amount, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(54);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.SelectFont(m_hterm->hFontLB);
    dc.DrawText(ftows(ptr->qty, m_hterm->qty_prec).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    dc.SelectFont(m_hterm->hFontL);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(60);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ptr->pack, wcslen(ptr->pack), 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

class CMuHDlg : 
  public CStdDialogResizeImpl<CMuHDlg>,
  public CUpdateUI<CMuHDlg>,
  public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  mutual_t *m_muh;
  CListViewCtrl m_list;

public:
  CMuHDlg(HTERMINAL *hterm, mutual_t *muh) : m_hterm(hterm), m_muh(muh) {
  }

	enum { IDD = IDD_MUH };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMuHDlg)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CMuHDlg)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMuHDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnDet)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CMuHDlg>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_muh->size());
    m_list.ShowWindow(m_muh->empty()?SW_HIDE:SW_NORMAL);
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
    dc.DrawText(LoadStringEx(IDS_CAP1), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(110);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP4), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.right = rect.left;
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

    MUTUAL_CONF *ptr = &(*m_muh)[lpdis->itemID];
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
    rect.bottom = rect.top + DRA::SCALEY(16);
    dc.DrawText(ptr->doc_code, wcslen(ptr->doc_code), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_hterm->hFontL);
    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->amount, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(110);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(wsftime(&ptr->doc_date, m_hterm->datetime_fmt).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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
    return sel < m_muh->size() && !(*m_muh)[sel].det.empty();
  }

  void _Det(size_t sel) {
    if( _IsValid(sel) ) {
      MUTUAL_CONF *ptr = &(*m_muh)[sel];
      CDetDlg(m_hterm, ptr->doc_code, &ptr->det).DoModal(m_hWnd);
    }
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, _IsValid(m_list.GetSelectedIndex()));
		UIUpdateToolBar();
  }
};

// Format: %pack_id%;%prod_id%;%descr%;...
static 
bool __packs(void *cookie, int line, const wchar_t **argv, int count) 
{
  mutual_t *ptr = (mutual_t *)cookie;
  mutual_t::iterator it;
  MUTUAL_CONF *h = NULL;
  DET_CONF *d = NULL;
  size_t size = ptr->size(), i = 0, j = 0, size2 = 0;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 3 ) {
    return false;
  }
  for( i = 0; i < size; i++ ) {
    h = &(*ptr)[i];
    size2 = h->det.size();
    for( j = 0; j < size2; j++ ) {
      d = &(*h).det[j];
      if( COMPARE_ID(d->pack_id, argv[0]) && COMPARE_ID(d->prod_id, argv[1]) ) {
        COPY_ATTR__S(d->pack, argv[2]);
      }
    }
  }

  return true;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;...
static 
bool __products(void *cookie, int line, const wchar_t **argv, int count) 
{
  mutual_t *ptr = (mutual_t *)cookie;
  mutual_t::iterator it;
  MUTUAL_CONF *h = NULL;
  DET_CONF *d = NULL;
  size_t size = ptr->size(), i = 0, j = 0, size2 = 0;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 6 ) {
    return false;
  }
  for( i = 0; i < size; i++ ) {
    h = &(*ptr)[i];
    size2 = h->det.size();
    for( j = 0; j < size2; j++ ) {
      d = &(*h).det[j];
      if( COMPARE_ID(d->prod_id, argv[1]) ) {
        COPY_ATTR__S(d->prod, argv[5]);
      }
    }
  }

  return true;
}

// Format: %doc_id%;%prod_id%;%pack_id%;%qty%;%amount%;
static 
bool __mutual_h_det(void *cookie, int line, const wchar_t **argv, int count) 
{
  mutual_t *ptr = (mutual_t *)cookie;
  mutual_t::iterator it;
  DET_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 5 ) {
    return false;
  }
  if( (it = std::find_if(ptr->begin(), ptr->end(), find_doc(argv[0]))) == ptr->end() ) {
    return true;
  }
  
  memset(&cnf, 0, sizeof(DET_CONF));
  COPY_ATTR__S(cnf.prod_id, argv[1]);
  COPY_ATTR__S(cnf.pack_id, argv[2]);
  cnf.qty = wcstof(argv[3]);
  cnf.amount = wcstof(argv[4]);
  
  it->det.push_back(cnf);

  return true;
}

// Format: %account_id%;%doc_id%;%doc_code%;%doc_date%;%amount%;%color%;%bgcolor%;
static 
bool __mutual_h(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *ptr = (HTERMINAL *)cookie;
  MUTUAL_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 7 ) {
    return false;
  }
  if( !COMPARE_ID(ptr->account_id, argv[0]) ) {
    return true;
  }
  
  COPY_ATTR__S(cnf.doc_id, argv[1]);
  COPY_ATTR__S(cnf.doc_code, argv[2]);
  datetime2tm(argv[3], &cnf.doc_date);
  cnf.amount = wcstof(argv[4]);
  cnf.color = _wtoi(argv[5]);
  cnf.bgcolor = _wtoi(argv[6]);
  
  ptr->muh->push_back(cnf);

  return true;
}

// Format: %account_id%;%mutual%;
static 
bool __mutual(void *cookie, int line, const wchar_t **argv, int count) 
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

  ptr->mutual = wcstof(argv[1]);

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
  if( h->r_mutuals_valid ) {
    DrawText(hDC, wsfromRes(IDS_TITLE_0).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE);
    SelectObject(hDC, h->hFontB);
    DrawText(hDC, ftows(h->mutual, h->cur_prec, h->thousand_sep).c_str(), -1, &rect, DT_RIGHT|DT_SINGLELINE);
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
  h->mutual = 0.0;
  h->r_mutuals_valid = ReadOmobusManual(L"mutuals", h, __mutual) >= 0 /*(!=OMOBUS_ERR)*/;
}

static
void OnHide(HTERMINAL *h) {
  h->account_id = NULL;
  h->mutual = 0.0;
  h->muh = NULL;
  h->r_mutuals_valid = false;
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor _wc;
  mutual_t muh;
  h->muh = &muh;
  if( ReadOmobusManual(L"mutual_history", h, __mutual_h) != OMOBUS_ERR ) {
    ReadOmobusManual(L"mutual_history_products", &muh, __mutual_h_det);
    ReadOmobusManual(L"packs", &muh, __packs);
    ReadOmobusManual(L"products", &muh, __products);
    ATLTRACE(L"mutual_history = %i\n", muh.size());
    _wc.Restore();
    CMuHDlg(h, &muh).DoModal(h->hParWnd);
    h->muh = NULL;
  }
  h->muh = NULL;
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
  h->qty_prec = _wtoi(get_conf(OMOBUS_QTY_PRECISION, OMOBUS_QTY_PRECISION_DEF));
  h->datetime_fmt = get_conf(OMOBUS_MUI_DATETIME, ISO_DATETIME_FMT);
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

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
  HTERMINAL *h = (HTERMINAL *)ptr;
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
