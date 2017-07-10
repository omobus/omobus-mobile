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

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-planograms-terminal-element";
static int _nHeight = 20;
static int _nCXScreen = 0, _nCYScreen = 0;

struct BRAND_CONF {
  uint32_t _c;
  wchar_t brand_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<BRAND_CONF> brands_t;

struct PL_CONF {
  wchar_t pl_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  wchar_t brand_id[OMOBUS_MAX_ID + 1];
  wchar_t image[MAX_PATH + 1];
};
typedef std::vector<PL_CONF> pl_t;
typedef std::vector<PL_CONF *> pl_ptr_t;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  bool focus;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontN, hFontB, hFontI;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

struct _cookie {
  brands_t *br; 
  pl_t *pl;
};

struct _find_brand : public std::unary_function <BRAND_CONF&, bool> { 
  _find_brand(const wchar_t *brand_id_) : brand_id(brand_id_) {
  }
  bool operator()(const BRAND_CONF &v) { 
    return COMPARE_ID(brand_id, v.brand_id);
  }
  const wchar_t *brand_id;
};

inline bool isEmptyBrand(BRAND_CONF &v) {
  return v._c == 0;
}

class CImageFrame : 
	public CStdDialogImpl<CImageFrame>,
	public CMessageFilter
{
private:
  std::wstring m_wsFile;
  HBITMAP m_hBmp;
  BITMAP m_BitMap;

public:
  CImageFrame(HBITMAP hBmp, const wchar_t *fname) : m_hBmp(hBmp), m_wsFile(fname) {
    GetObject(hBmp, sizeof(BITMAP), &m_BitMap);
  }

	enum { IDD = IDD_IMAGE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP(CImageFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_GESTURE, OnGesture)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogImpl<CImageFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
		return bHandled = FALSE;
	}

  LRESULT OnGesture(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    LRESULT rc = 0;
    GESTUREINFO gi;
    memset(&gi, 0, sizeof(gi));
    gi.cbSize = sizeof(GESTUREINFO);
    if( TKGetGestureInfo((HGESTUREINFO)lParam, &gi) ) {
      if( wParam == GID_DOUBLESELECT ) {
        ExecRootProcess(L"viewer.exe", m_wsFile.c_str(), FALSE);
      }
    }
    return rc;
  }

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CDC dcMem;
    HBITMAP hBmpOld;
    RECT rc;

    if( m_hBmp != NULL ) {
      GetClientRect(&rc);
			dcMem.CreateCompatibleDC(dc);
			hBmpOld = dcMem.SelectBitmap(m_hBmp);
      dc.StretchBlt(0, 0, rc.right, rc.bottom, dcMem, 0, 0, 
        m_BitMap.bmWidth, m_BitMap.bmHeight, SRCCOPY);
			dcMem.SelectBitmap(hBmpOld);
    }
    
    return 0;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}
};

class CPLFrame : 
	public CStdDialogResizeImpl<CPLFrame>,
	public CUpdateUI<CPLFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  pl_t *m_pl;
  pl_ptr_t m_cur_pl;
  BRAND_CONF *m_cur_brand;
  CListViewCtrl m_list;

public:
  CPLFrame(HTERMINAL *hterm, pl_t *pl, BRAND_CONF *cur_brand) : 
      m_hterm(hterm), m_pl(pl), m_cur_brand(cur_brand) {}

	enum { IDD = IDD_PLANOGRAMS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CPLFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CPLFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CPLFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CUpdateUI<CPLFrame>)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CPLFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDW_MENU_BAR, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    _Reload();
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    PL_CONF *ptr = m_cur_pl[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    std::wstring fname;
    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(_PLFile(ptr->image, fname) ? m_hterm->hFontB : m_hterm->hFontI);
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
    rect.bottom -= DRA::SCALEY(2);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

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
    lmi->itemHeight = DRA::SCALEY(30);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Image(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Image(m_list.GetSelectedIndex());
    return 0;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
		return 0;
	}

  const wchar_t *GetSubTitle() {
    return m_cur_brand ? m_cur_brand->descr : NULL;
  }

protected:
  bool _PLFile(const wchar_t *image, std::wstring &fname) {
    if( image == NULL || image[0] == L'\0' ) {
      return false;
    }
    fname = GetOmobusProfilePath(OMOBUS_PROFILE_SYNC); 
    fname += image;
    if( wcsfexist(fname.c_str()) ) {
      return true;
    }
    fname = GetOmobusRootPath();
    fname += image;
    return wcsfexist(fname.c_str());
  }

  HBITMAP _LoadPL(const wchar_t *fname) {
    CWaitCursor _wc;
    return LoadImageFile(fname, _nCXScreen, _nCYScreen, NULL, NULL);
  }

  void _UnloadPL(HBITMAP pl) {
    if( pl != NULL )
      DeleteObject(pl);
  }

  void _Image(size_t i) {
    if( i >= m_cur_pl.size() ) {
      return;
    }
    PL_CONF *ptr = m_cur_pl[i];
    HBITMAP pl = NULL;
    std::wstring fname;
    if( !_PLFile(ptr->image, fname) ) {
      return;
    }
    if( (pl = _LoadPL(fname.c_str())) != NULL ) {
      CImageFrame(pl, fname.c_str()).DoModal(m_hWnd);
      _UnloadPL(pl);
    } else {
      MessageBoxEx(m_hWnd, IDS_EMEM, IDS_ERROR, MB_ICONSTOP);
    }
  }

  void _Reload() {
    CWaitCursor _wc;
    m_cur_pl.clear();
    m_list.SetItemCount(m_cur_pl.size());
    size_t size = m_pl->size();
    for( size_t i = 0; i < size; i++ ) {
      PL_CONF *ptr = &(*m_pl)[i];
      if( m_cur_brand == NULL || COMPARE_ID(ptr->brand_id, m_cur_brand->brand_id) ) {
        m_cur_pl.push_back(ptr);
      }
    }
    m_list.SetItemCount(m_cur_pl.size());
    m_list.ShowWindow(m_cur_pl.empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
  }

  void _LockMainMenuItems() {
    std::wstring fname;
    int i = m_list.GetSelectedIndex();
    UIEnable(ID_ACTION, i != -1 && ((size_t)i) < m_cur_pl.size() && 
      _PLFile(m_cur_pl[i]->image, fname) );
		UIUpdateToolBar();
  }
};

class CBrandFrame : 
	public CStdDialogResizeImpl<CBrandFrame>,
	public CUpdateUI<CBrandFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  pl_t *m_pl;
  brands_t *m_brands;
  CListViewCtrl m_list;
  
public:
  CBrandFrame(HTERMINAL *h, pl_t *pl, brands_t *brands) : 
      m_hterm(h), m_pl(pl), m_brands(brands) {}

	enum { IDD = IDD_BRANDS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CBrandFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CBrandFrame)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CBrandFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CBrandFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_brands->size());
    m_list.ShowWindow(m_brands->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    BRAND_CONF *ptr = &((*m_brands)[lpdis->itemID]);
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(m_hterm->hFontN);
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
    rect.bottom -= DRA::SCALEY(2);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

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
    lmi->itemHeight = DRA::SCALEY(30);
    return 1;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _PL(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _PL(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

protected:
  void _LockMainMenuItems() {
    UIEnable(ID_MENU_OK, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
	}

  void _PL(size_t sel) {
    if( sel >= m_brands->size() )
      return;
    CPLFrame(m_hterm, m_pl, &(*m_brands)[sel]).DoModal(m_hWnd);
  }
};

// Format: %id%;%descr%;
static 
bool __brands(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  BRAND_CONF cnf;
  brands_t *ptr = (brands_t *) cookie;

  if( ln == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 2 ) {
    return false;
  }
 
  memset(&cnf, 0, sizeof(BRAND_CONF));
  COPY_ATTR__S(cnf.brand_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);

  ptr->push_back(cnf);

  return true;
}

// Format: %id%;%descr%;%brand_id%;%image%;
static 
bool __planograms(void *cookie, int line, const wchar_t **argv, int argc) 
{
  PL_CONF cnf;
  _cookie *ptr = (_cookie *) cookie;
  brands_t::iterator br_it;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 4 ) {
    return false;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.pl_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  COPY_ATTR__S(cnf.brand_id, argv[2]);
  COPY_ATTR__S(cnf.image, argv[3]);
  ptr->pl->push_back(cnf);

  if( cnf.brand_id[0] != L'\0' &&
      (br_it = std::find_if(ptr->br->begin(), ptr->br->end(), _find_brand(cnf.brand_id))) != ptr->br->end() ) {
    br_it->_c++;
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
  pl_t pl;
  brands_t br;
  const wchar_t *x;
  BRAND_CONF def;
  _cookie ck = {&br, &pl};

  memset(&def, 0, sizeof(def));
  if( (x = h->get_conf(__INT_MERCH_BRAND_ID, NULL)) == NULL ) {
    ReadOmobusManual(L"brands", &br, __brands);
    ATLTRACE(L"brands = %i\n", br.size());
  } else {
    COPY_ATTR__S(def.brand_id, x);
    COPY_ATTR__S(def.descr, h->get_conf(__INT_MERCH_BRAND, L""));
    br.push_back(def);
  }

  ReadOmobusManual(L"planograms", &ck, __planograms);
  ATLTRACE(L"planograms = %i\n", pl.size());
  if( !br.empty() ) {
    br.erase(remove_if(br.begin(), br.end( ), isEmptyBrand), br.end());
    ATLTRACE(L"brands (after shrinking) = %i\n", br.size());
  }

  _wc.Restore();
  if( br.size() > 1 ) {
    CBrandFrame(h, &pl, &br).DoModal(h->hParWnd);
  } else {
    CPLFrame(h, &pl, x == NULL ? NULL : &def).DoModal(h->hParWnd);
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
    _nCYScreen = GetSystemMetrics(SM_CYSCREEN);
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
  h->get_conf = get_conf;

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontN = CreateBaseFont(DRA::SCALEY(14), FW_SEMIBOLD, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(11), FW_SEMIBOLD, FALSE);
  h->hFontI = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK | OMOBUS_SCREEN_ACTIVITY | OMOBUS_SCREEN_MERCH_TASKS);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontN);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hFontI);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
