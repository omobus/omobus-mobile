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
#include <atlomobus.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

#define QTY_STD_SHIDIF SHIDIF_SIZEDLGFULLSCREEN|SHIDIF_CANCELBUTTON/*SHIDIF_DONEBUTTON*/

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-presence-terminal-element";
static int _nHeight = 20, _nCXScreen = 0;

struct T_PRESENCE {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  wchar_t priority[6];
  int32_t color, bgcolor;
  cur_t sales;
  int32_t facing_L, stock_L; // Last data from presence_products.
  int32_t facing_J, stock_J; // Last data from loacl journal cache/presence.
  int32_t facing, stock;
};
typedef std::vector<T_PRESENCE> docT_t;

struct H_PRESENCE {
  const wchar_t *account_id, *activity_type_id, *w_cookie, *a_cookie,
    *brand_id, *brand_descr;
  omobus_location_t posCre;
  time_t ttCre;
  docT_t *_t;
  wchar_t _c_date[OMOBUS_MAX_DATE + 1];
};

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

class find_prod :
  public std::unary_function<T_PRESENCE, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_prod(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(T_PRESENCE &v) const {
    return COMPARE_ID(m_prod_id, v.prod_id);
  }
};

inline bool isValidItem(T_PRESENCE &v) {
  return v.stock > 0;
}

static
bool CloseDoc(HTERMINAL *h, H_PRESENCE *doc);

class CQtyDlg : 
	public CStdDialogResizeImpl<CQtyDlg, QTY_STD_SHIDIF>,
	public CMessageFilter
{
  typedef CStdDialogResizeImpl<CQtyDlg, QTY_STD_SHIDIF> baseClass;

protected:
  CEdit m_facing, m_stock;
  HTERMINAL *m_hterm;
  T_PRESENCE *m_tpart;

public:
  CQtyDlg(HTERMINAL *hterm, T_PRESENCE *tpart) : m_hterm(hterm), m_tpart(tpart) {
  }

	enum { IDD = IDD_QTY };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CQtyDlg)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CQtyDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL);
    SetDlgItemText(IDC_NAME, m_tpart->descr);
    m_facing = GetDlgItem(IDC_FACING);
    m_facing.SetLimitText(3);
    m_stock = GetDlgItem(IDC_STOCK);
    m_stock.SetLimitText(3);
    if( m_tpart->facing ) {
      m_facing.SetWindowText(itows(m_tpart->facing).c_str());
    }
    if( m_tpart->stock ) {
      m_stock.SetWindowText(itows(m_tpart->stock).c_str());
    }
    m_facing.SetFocus(); 
    m_facing.SetSelAll();
		return bHandled = FALSE;
	}

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    m_tpart->facing = _GetEditInt(m_facing);
    m_tpart->stock = _GetEditInt(m_stock);
    if( m_tpart->facing > m_tpart->stock ) {
      m_tpart->stock = m_tpart->facing;
    }
    SHSipPreference(m_hWnd, SIP_DOWN);
    EndDialog(ID_ACTION);
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    SHSipPreference(m_hWnd, SIP_DOWN);
    EndDialog(wID);
    return 0;
  }

private:
  int32_t _GetEditInt(CEdit &ctrl) {
    CString tmp; ctrl.GetWindowText(tmp);
    return _wtoi(tmp);
  }
};

class CPresenceFrame : 
	public CStdDialogResizeImpl<CPresenceFrame>,
	public CUpdateUI<CPresenceFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  H_PRESENCE *m_doc;
  CListViewCtrl m_list;
  CFont m_capFont, m_boldFont, m_italicFont, m_baseFont;

public:
  CPresenceFrame(HTERMINAL *hterm, H_PRESENCE *doc) : m_hterm(hterm), m_doc(doc)
  {
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_italicFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CPresenceFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CPresenceFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CPresenceFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CUpdateUI<CPresenceFrame>)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CPresenceFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDW_MENU_BAR, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_doc->_t->size());
    m_list.ShowWindow(m_doc->_t->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CRect rect; GetClientRect(&rect);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_capFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL), itemWidth = (_nCXScreen - scrWidth)/5;
    rect.top = GetTitleHeight();
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right;

    for( int i = 0; i < 5; i++ ) {
      rect.right = rect.left;
      rect.left = rect.right - itemWidth;
      dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
      dc.DrawText(LoadStringEx(IDS_CAP4 - i), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    }

    dc.SelectPen(oldPen);
    dc.SelectFont(oldFont);
    DoPaintTitle();
		return FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    T_PRESENCE *ptr = &(*m_doc->_t)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    HPEN oldPen = dc.SelectPen(m_hterm->hpBord);
    HFONT oldFont = dc.SelectFont(m_baseFont);
    CBrush hbrCur = ptr->bgcolor ? CreateSolidBrush(ptr->bgcolor) : NULL;
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL), itemWidth = (_nCXScreen - scrWidth)/5;
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
    rect.top += DRA::SCALEY(3);
    rect.bottom = rect.top + DRA::SCALEY(16);
    dc.DrawText(ptr->descr, wcslen(ptr->descr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - itemWidth;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->stock || ptr->stock_J ) {
      dc.SelectFont(ptr->stock ? m_boldFont : m_italicFont);
      dc.DrawText(itows(ptr->stock ? ptr->stock : ptr->stock_J).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - itemWidth;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->facing || ptr->facing_J ) {
      if( !(ptr->stock && !ptr->facing) ) {
        dc.SelectFont(ptr->facing ? m_boldFont : m_italicFont);
        dc.DrawText(itows(ptr->facing ? ptr->facing : ptr->facing_J).c_str(), -1, 
          rect.left + DRA::SCALEX(4), rect.top, 
          rect.right - DRA::SCALEX(4), rect.bottom,
          DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
        dc.SelectFont(m_baseFont);
      }
    }

    rect.right = rect.left;
    rect.left = rect.right - itemWidth;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->sales ) {
      dc.SelectFont(m_boldFont);
      dc.DrawText(ftows(ptr->sales, 0).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - itemWidth;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->stock_L ) {
      dc.SelectFont(m_boldFont);
      dc.DrawText(ftows(ptr->stock_L, 0).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - itemWidth;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->facing_L ) {
      dc.SelectFont(m_boldFont);
      dc.DrawText(ftows(ptr->facing_L, 0).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
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
    lmi->itemHeight = DRA::SCALEY(35);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Count(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

	LRESULT OnClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_STR1, IDS_INFORM, MB_ICONQUESTION|MB_YESNO) == IDYES ) {
      if( CloseDoc(m_hterm, m_doc) ) {
        MessageBoxEx(m_hWnd, IDS_STR8, IDS_INFORM, MB_ICONINFORMATION);
        EndDialog(ID_CLOSE);
      } else {
        MessageBoxEx(m_hWnd, IDS_STR9, IDS_ERROR, MB_ICONSTOP);
      }
    }
		return 0;
	}

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( std::count_if(m_doc->_t->begin(), m_doc->_t->end(), isValidItem) == 0 ||
        MessageBoxEx(m_hWnd, IDS_STR7, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
      EndDialog(wID);
    }
		return 0;
	}

  const wchar_t *GetSubTitle() {
    return m_doc->brand_descr;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, std::count_if(m_doc->_t->begin(), m_doc->_t->end(), isValidItem));
		UIUpdateToolBar();
	}

  void _Count(size_t i) {
    if( i >= m_doc->_t->size() ) {
      return;
    }
    T_PRESENCE *tpart = &(*m_doc->_t)[i];
    CQtyDlg(m_hterm, tpart).DoModal(m_hWnd);
    _LockMainMenuItems();
  }
};

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;%manuf_id%;%shelf_life%;%brand_id%;...
static 
bool __products(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  H_PRESENCE *ptr = (H_PRESENCE *) cookie;
  docT_t::iterator it;

  if( ln == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 9 ) {
    return false;
  }
  if( /*ftype*/_wtoi(argv[2]) != 0 ) {
    return true;
  }
  if( (it = std::find_if(ptr->_t->begin(), ptr->_t->end(), find_prod(argv[1]))) == ptr->_t->end() ) {
    return true;
  }
  if( !COMPARE_ID(ptr->brand_id, argv[8]) ) {
    ptr->_t->erase(it);
  } else {
    if( it->priority[0] == L'\0' ) {
      COPY_ATTR__S(it->descr, argv[5]);
    } else {
      _snwprintf(it->descr, OMOBUS_MAX_DESCR, L"%s %s", it->priority, argv[5]);
    }
  }

  return true;
}

// Format: %account_id%;%prod_id%;%color%;%bgcolor%;%sales%;%facing%;%stock%;%priority%;
static 
bool __presence_products(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  H_PRESENCE *ptr;
  T_PRESENCE cnf;
  docT_t::iterator it;
  
  if( ln == 0 ) {
    return true;
  }
  if( (ptr = (H_PRESENCE *) cookie) == NULL || argc < 8 ) {
    return false;
  }
  if( !(COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(ptr->account_id, argv[0])) ) {
    return true;
  }
  if( (it = std::find_if(ptr->_t->begin(), ptr->_t->end(), find_prod(argv[1]))) == ptr->_t->end() ) {
    memset(&cnf, 0, sizeof(cnf));
    COPY_ATTR__S(cnf.prod_id, argv[1]);
    cnf.color = _wtoi(argv[2]);
    cnf.bgcolor = _wtoi(argv[3]);
    cnf.sales = wcstof(argv[4]);
    cnf.facing_L = _wtoi(argv[5]);
    cnf.stock_L = _wtoi(argv[6]);
    COPY_ATTR__S(cnf.priority, argv[7]);
    ptr->_t->push_back(cnf);
  } else {
    it->color = _wtoi(argv[2]);
    it->bgcolor = _wtoi(argv[3]);
    it->sales = wcstof(argv[4]);
    it->facing_L = _wtoi(argv[5]);
    it->stock_L = _wtoi(argv[6]);
    COPY_ATTR__S(it->priority, argv[7]);
  }

  return true;
}

// Format: %date%;%account_id%;%prod_id%;%facing%;%stock%;
static 
bool __j_presence(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  H_PRESENCE *ptr = (H_PRESENCE *) cookie;
  docT_t::iterator it;
  
  if( ln == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 5 ) {
    return false;
  }
  if( wcsncmp(ptr->_c_date, argv[0], OMOBUS_MAX_DATE) != 0 ) {
    return true;
  }
  if( !COMPARE_ID(ptr->account_id, argv[1]) ) {
    return true;
  }
  if( (it = std::find_if(ptr->_t->begin(), ptr->_t->end(), find_prod(argv[2]))) == ptr->_t->end() ) {
    return true;
  }

  it->facing_J = _wtoi(argv[3]);
  it->stock_J = _wtoi(argv[4]);

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
bool WriteDocPk(HTERMINAL *h, H_PRESENCE *doc, uint64_t &doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml, xml_t, rows, cur_row;
  size_t i, qty_pos;
  T_PRESENCE *tpart;

  xml = wsfromRes(IDS_X_DOC_PRESENCE_H, true);
  xml_t = wsfromRes(IDS_X_DOC_PRESENCE_T, true);
  if( xml.empty() || xml_t.empty() ) {
    return false;
  }

  qty_pos = 0;
  cur_row = xml_t;
  for( i = 0; i < doc->_t->size(); i++ ) {
    tpart = &(*doc->_t)[i];
    if( !isValidItem(*tpart) ) {
      continue;
    }
    wsrepl(cur_row, L"%pos_id%", itows(qty_pos).c_str());
    wsrepl(cur_row, L"%prod_id%", fix_xml(tpart->prod_id).c_str());
    wsrepl(cur_row, L"%facing%", itows(tpart->facing).c_str());
    wsrepl(cur_row, L"%stock%", itows(tpart->stock).c_str());
    qty_pos++;
    rows += cur_row; 
    cur_row = xml_t;
  }

  if( qty_pos == 0 ) {
    ATLTRACE(L"presence: WriteDocPk - попытка закрытия документа без табличной части.\n");
    return false;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", doc->w_cookie);
  wsrepl(xml, L"%a_cookie%", doc->a_cookie);
  wsrepl(xml, L"%activity_type_id%", fix_xml(doc->activity_type_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(doc->account_id).c_str());
  wsrepl(xml, L"%created_dt%", wsftime(doc->ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%qty_pos%", itows(qty_pos).c_str());
  wsrepl(xml, L"%created_gps_dt%", wsftime(doc->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(doc->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(doc->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%rows%", rows.c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_PRESENCE, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, H_PRESENCE *doc, uint64_t &doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
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
  wsrepl(xml, L"%doc_type%", OMOBUS_PRESENCE);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(doc->activity_type_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(doc->account_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", doc->w_cookie);
  wsrepl(xml, L"%a_cookie%", doc->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteJournal(HTERMINAL *h, H_PRESENCE *doc, time_t ttClo) 
{
  std::wstring txt, templ, rows;
  size_t size, i;
  T_PRESENCE *tpart;

  txt = wsfromRes(IDS_J_PRESENCE, true);
  if( txt.empty() ) {
    return false;
  }

  templ = txt;
  size = doc->_t->size();
  for( i = 0 ; i < size; i++ ) {
    tpart = &(*doc->_t)[i];
    if( !isValidItem(*tpart) ) {
      continue;
    }
    wsrepl(txt, L"%date%", wsftime(ttClo).c_str());
    wsrepl(txt, L"%account_id%", fix_xml(doc->account_id).c_str());
    wsrepl(txt, L"%prod_id%", fix_xml(tpart->prod_id).c_str());
    wsrepl(txt, L"%facing%", itows(tpart->facing).c_str());
    wsrepl(txt, L"%stock%", itows(tpart->stock).c_str());
    rows += txt; 
    txt = templ;
  }
  
  return WriteOmobusJournal(OMOBUS_PRESENCE, templ.c_str(), rows.c_str()) 
    == OMOBUS_OK;
}

static
bool CloseDoc(HTERMINAL *h, H_PRESENCE *doc)
{
  if( h->user_id == NULL )
    return false;
  CWaitCursor _wc;
  omobus_location_t posClo; omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( !WriteDocPk(h, doc, doc_id, &posClo, ttClo) )
    return false;
  WriteActPk(h, doc, doc_id, &posClo, ttClo);
  WriteJournal(h, doc, ttClo);
  return true;
}

static
void IncrementDocumentCounter(HTERMINAL *h) {
  uint32_t count = _wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0"));
  ATLTRACE(__INT_ACTIVITY_DOCS L" = %i\n", count + 1);
  h->put_conf(__INT_ACTIVITY_DOCS, itows(count + 1).c_str());
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
  H_PRESENCE doc;
  docT_t docT;

  memset(&doc, 0, sizeof(H_PRESENCE));
  doc.ttCre = omobus_time();
  omobus_location(&doc.posCre);
  doc.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  doc.a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  doc.activity_type_id = h->get_conf(__INT_ACTIVITY_TYPE_ID, OMOBUS_UNKNOWN_ID);
  doc.account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  doc.brand_id = h->get_conf(__INT_MERCH_BRAND_ID, NULL);
  doc.brand_descr = h->get_conf(__INT_MERCH_BRAND, NULL);
  doc._t = &docT;
  wcsncpy(doc._c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(doc._c_date));

  ReadOmobusManual(L"presence_products", &doc, __presence_products);
  ATLTRACE(L"presence_products = %i\n", docT.size());
  if( !docT.empty() ) {
    ReadOmobusManual(L"products", &doc, __products);
    ATLTRACE(L"products = %i\n", docT.size());
    ReadOmobusJournal(OMOBUS_PRESENCE, &doc, __j_presence);
  }
  _wc.Restore();

  if( CPresenceFrame(h, &doc).DoModal(h->hParWnd) == ID_CLOSE ) {
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
    _nCXScreen = GetSystemMetrics(SM_CXSCREEN);
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
  h->put_conf = put_conf;
  h->hParWnd = hParWnd;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_MERCH_TASKS);
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
