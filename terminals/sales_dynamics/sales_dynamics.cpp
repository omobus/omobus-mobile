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

#define CELL_WIDTH      28
#define HEADER_HEIGHT   18

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
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-sales_dynamics-terminal-element";
static int _nHeight = 20;

typedef struct _tagHTERMINAL {
  int cur_prec;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  bool focus;
  const wchar_t *thousand_sep;
} HTERMINAL;

typedef struct _tag_sales_dynamics_value {
  wchar_t account_id[OMOBUS_MAX_ID+1];
  wchar_t account[OMOBUS_MAX_DESCR+1];
  wchar_t addr[OMOBUS_MAX_ADDR_DESCR+1];
  double amount[8];
} sales_dynamics_value_t;

typedef std::vector<sales_dynamics_value_t> sales_dynamics_t;

typedef struct _tag_sales_dynamics_names_value {
  wchar_t name[8][6];
  wchar_t descr[8][21];
} sales_dynamics_names_t;

struct _find_account_id : 
  public std::unary_function <sales_dynamics_value_t&, bool> { 
  _find_account_id(const wchar_t *account_id_) : account_id(account_id_) {
  }
  bool operator()(const sales_dynamics_value_t &v) { 
    return COMPARE_ID(account_id, v.account_id);
  }
  const wchar_t *account_id;
};

static 
bool __sales_dynamics(void *cookie, int line, const wchar_t **argv, int count) 
{
  sales_dynamics_t *ptr = (sales_dynamics_t *)cookie;
  sales_dynamics_value_t val;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 9 ) {
    return false;
  }

  memset(&val, 0, sizeof(val));
  COPY_ATTR__S(val.account_id, argv[0]);
  for( int i = 0; i < 8; i++ )
    val.amount[i] = wcstof(argv[1+i]);
  ptr->push_back(val);
  
  return true;
}

// Format: %pid%;%account_id%;%ftype%;%code%;%descr%;%address%;%chan%;%poten_id%;%group_price_id%;%pay_delay%;%locked%;%latitude%;%longitude%;
static 
bool __account(void *cookie, int line, const wchar_t **argv, int count) 
{
  sales_dynamics_t::iterator it;
  sales_dynamics_t *ptr = (sales_dynamics_t *)cookie;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 13 ) {
    return false;
  }
  if( /*ftype*/ _ttoi(argv[2]) > 0 ) {
    return true;
  }
  if( (it = std::find_if((*ptr).begin(), (*ptr).end(), _find_account_id(argv[1]))) == (*ptr).end() ) {
    return true;
  }
  COPY_ATTR__S(it->account, argv[4]);
  COPY_ATTR__S(it->addr, argv[5]);
  return true;
}

static 
bool __sales_dynamics_names(void *cookie, int line, const wchar_t **argv, int count) 
{
  sales_dynamics_names_t *ptr = (sales_dynamics_names_t *)cookie;

  if( line == 0 ) {
    return true;
  }
  if( ptr == NULL || count < 8 ) {
    return false;
  }
  if( line == 1 ) {
    for( int i = 0; i < 8; i++ )
      COPY_ATTR__S(ptr->name[i], argv[i]);
  } else if( line == 2 ) {
    for( int i = 0; i < 8; i++ )
      COPY_ATTR__S(ptr->descr[i], argv[i]);
  } else {
    return false;
  }
  return true;
}

static
void LoadSalesDynamics(HTERMINAL *h, sales_dynamics_names_t &sdn, sales_dynamics_t &sd) {
  memset(&sdn, 0, sizeof(sales_dynamics_names_t));
  sd.clear();
  ReadOmobusManual(L"sales_dynamics", &sd, __sales_dynamics);
  ATLTRACE(L"sales_dynamics = %i\n", sd.size());
  ReadOmobusManual(L"accounts", &sd, __account);
  ReadOmobusManual(L"sales_dynamics_names", &sdn, __sales_dynamics_names);
}

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  AtlLoadString(uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

class CDetDlg : 
  public CStdDialogImpl<CDetDlg>
{
private:
  HTERMINAL *m_hterm;
  CListViewCtrl m_list;
  sales_dynamics_t *m_sales;
  sales_dynamics_names_t *m_names;
  sales_dynamics_value_t *m_amount;
  double m_total_amount[8], m_total[2];
  CFont m_capFont, m_baseFont;

public:
  CDetDlg(HTERMINAL *hterm, 
          sales_dynamics_names_t *names, 
          sales_dynamics_t *sales, 
          sales_dynamics_value_t *amount) :
              m_hterm(hterm),
              m_names(names), 
              m_sales(sales), 
              m_amount(amount) { 
    memset(m_total_amount, 0, sizeof(m_total_amount));
    memset(m_total, 0, sizeof(m_total));
  }

	enum { IDD = IDD_DET_FORM };

	BEGIN_MSG_MAP(CDetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    CHAIN_MSG_MAP(CStdDialogImpl<CDetDlg>)
	END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONE, SHCMBF_HIDESIPBUTTON);
    _InitList();
    _CalcTotal();
    _InitFonts();
    _DoDataExchange();
    return bHandled = FALSE;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_capFont);
    rect.top = GetTitleHeight() + DRA::SCALEY(57);
    rect.bottom = rect.top + DRA::SCALEY(24/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_COL3), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_COL2), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_COL0), -1,
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

    size_t curPos = lpdis->itemID;
    if( curPos > 7 )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_baseFont);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd);
    }

    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(m_total_amount[curPos] != 0.0 ? ftows(m_total_amount[curPos], m_hterm->cur_prec, m_hterm->thousand_sep).c_str() : L"", -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(m_amount->amount[curPos] != 0.0 ? ftows(m_amount->amount[curPos], m_hterm->cur_prec, m_hterm->thousand_sep).c_str() : L"", -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(m_names->descr[curPos], -1, 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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
    lmi->itemHeight = DRA::SCALEY(24);
    return 1;
  }

  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled) {
    EndDialog(wID);
    return 0;
  }

private:
  void _CalcTotal() {
    for( size_t i = 0; i < (*m_sales).size(); i++ ) {
      for( size_t j = 0; j < 8; j++ )
        m_total_amount[j] += (*m_sales)[i].amount[j];
    }
    for( size_t j = 0; j < 8; j++ ) {
      m_total[0] += m_amount->amount[j];
      m_total[1] += m_total_amount[j];
    }
  }

  void _DoDataExchange() {
    SetDlgItemText(IDC_ACCOUNT, m_amount->account);
		SetDlgItemText(IDC_DEL_ADDR, m_amount->addr);
    SetDlgItemText(IDC_TOTAL1, 
      ftows(m_total[0], m_hterm->cur_prec, m_hterm->thousand_sep).c_str());
    SetDlgItemText(IDC_TOTAL2, 
      ftows(m_total[1], m_hterm->cur_prec, m_hterm->thousand_sep).c_str());
  }

  void _InitList() {
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(8);
  }

  void _InitFonts() {
    m_baseFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  }
};

class CSalesDynamicsFrame : 
	public CStdDialogResizeImpl<CSalesDynamicsFrame>,
	public CUpdateUI<CSalesDynamicsFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  CListViewCtrl m_list;
  sales_dynamics_t m_sales;
  sales_dynamics_names_t m_names;
  CFont m_capFont, m_baseFont, m_boldFont, m_plusFont;

public:
  CSalesDynamicsFrame(HTERMINAL *hterm) : m_hterm(hterm) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CSalesDynamicsFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CSalesDynamicsFrame)
    UPDATE_ELEMENT(ID_DETAILS, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CSalesDynamicsFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
    NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnIdle)
		COMMAND_ID_HANDLER(ID_DETAILS, OnAction)
		COMMAND_ID_HANDLER(ID_EXIT, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CUpdateUI<CSalesDynamicsFrame>)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CSalesDynamicsFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    _InitList();
    _Reload();
    _InitFonts();
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

	LRESULT OnIdle(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _LockMainMenuItems();
		return 0;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rc;
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_capFont);
    int top = GetTitleHeight(), bottom = top + DRA::SCALEY(HEADER_HEIGHT);
    for( int i = 0; i < 8; i++ ) {
      int left = DRA::SCALEX(i*CELL_WIDTH), right = left + DRA::SCALEX(CELL_WIDTH);
      dc.DrawLine(right, top, right, bottom);
      dc.DrawText(m_names.name[i], wcslen(m_names.name[i]), 
        left, top, right, bottom, 
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);    
    }
    dc.DrawLine(0, bottom, DRA::SCALEY(240), bottom);
    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    DoPaintTitle();
		return 0;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    sales_dynamics_value_t *data = &m_sales[lpdis->itemID];
    if( data == NULL )
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
    rect.bottom = rect.top + DRA::SCALEY(14);
    dc.DrawText(data->account, wcslen(data->account),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(30);
    dc.SelectFont(m_baseFont);
    dc.DrawText(data->addr, wcslen(data->addr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_WORDBREAK);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    dc.SelectFont(m_plusFont);
    for( int i = 0; i < 8; i++ ) {
      int left = DRA::SCALEX(i*CELL_WIDTH),
        right = DRA::SCALEX((i+1)*CELL_WIDTH);
      dc.DrawLine(right, rect.top, right, rect.bottom);
      if( data->amount[i] != 0 ) {
        dc.DrawText(_T("+"), 1, 
          left + DRA::SCALEX(2), rect.top, 
          right - DRA::SCALEX(2), rect.bottom,
          DT_CENTER|DT_VCENTER);
      }
    }

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
    lmi->itemHeight = DRA::SCALEY(60);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    LPNMLISTVIEW lv = (LPNMLISTVIEW)pnmh;
    if( lv->iItem >= 0 )
      _Det(lv->iItem);
    return 0;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}

	LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    LVITEM lvi = {0};
    if( m_list.GetSelectedItem(&lvi) )
      _Det(lvi.iItem);
		return 0;
	}

private:
  void _Reload() {
    CWaitCursor _wc;
    LoadSalesDynamics(m_hterm, m_names, m_sales);
    m_list.SetItemCount(m_sales.size());
    m_list.ShowWindow(m_sales.empty()?SW_HIDE:SW_NORMAL);
  }

  void _Det(int cur) {
    if( cur < 0 || m_sales.empty() || cur >= m_sales.size() )
      return;
    CDetDlg dlg(m_hterm, &m_names, &m_sales, &m_sales[cur]);
    dlg.DoModal();
    m_list.SetFocus();
  }

  void _LockMainMenuItems() {
    UIEnable(ID_DETAILS, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
	}

  void _InitList() {
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
  }

  void _InitFonts() {
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_plusFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  }
};

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
  DrawText(hDC, wsfromRes(IDS_TITLE).c_str(), -1, &rect, DT_LEFT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
}

static 
void OnAction(HTERMINAL *h) {
  CSalesDynamicsFrame(h).DoModal(h->hParWnd);
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
  h->cur_prec = 0/*_wtoi(get_conf(OMOBUS_CUR_PRECISION, OMOBUS_CUR_PRECISION_DEF))*/;
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

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
