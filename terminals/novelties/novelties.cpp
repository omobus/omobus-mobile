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
#include <atllabel.h>
#include <atlomobus.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-novelties-terminal-element";
static int _nHeight = 20;

typedef std::map<std::wstring, cur_t> novelties_t;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

struct PACK_CONF {
  wchar_t pack_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_SHORT_DESCR + 1];
  qty_t pack;
  double weight;
  double volume;
};
typedef std::vector<PACK_CONF> packs_t;
typedef std::map<std::wstring, packs_t> product_packs_t;

struct PRODUCT_CONF {
  wchar_t code[OMOBUS_MAX_ID + 1];
  wchar_t art[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  SIMPLE_CONF *manuf, *categ;
  PACK_CONF *base_pack, *def_pack;
  packs_t *packs;
  wchar_t shelf_life[OMOBUS_MAX_DESCR + 1];
  byte_t fractional;
  cur_t price;
};
typedef std::vector<PRODUCT_CONF> products_t;

typedef struct _tagHTERMINAL {
  bool focus;
  int cur_prec;
  const wchar_t *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  int novelty_count;
} HTERMINAL;

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

class find_pack :
  public std::unary_function<PACK_CONF, bool> {
private:
  const wchar_t *m_pack_id;
public:
  find_pack(const wchar_t *pack_id) : m_pack_id(pack_id) {
  }
  bool operator()(PACK_CONF &v) const {
    return COMPARE_ID(m_pack_id, v.pack_id);
  }
};

class find_base_pack :
  public std::unary_function<PACK_CONF, bool> {
public:
  find_base_pack() {
  }
  bool operator()(PACK_CONF &v) const {
    return v.pack == 1.0;
  }
};

struct __int_product_cookie {
  products_t *prods;
  simple_t *mf, *categ;
  novelties_t *novs;
  product_packs_t *packs;
};

class CProdDet : 
	public CStdDialogResizeImpl<CProdDet>,
	public CMessageFilter, 
  public CWinDataExchange<CProdDet>
{
protected:
  HTERMINAL *m_hterm;
  PRODUCT_CONF *m_prod;
  CLabel m_emptyLabel[9];

public:
  CProdDet(HTERMINAL *hterm, PRODUCT_CONF *prod) : 
    m_hterm(hterm), m_prod(prod) {
  }

	enum { IDD = IDD_PROD_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CProdDet)
  END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP(CProdDet)
		DDX_TEXT(IDC_NAME, m_prod->descr)
    if( m_prod->categ != NULL && m_prod->categ->descr[0] != L'\0' )
      { DDX_TEXT(IDC_CATEG, m_prod->categ->descr) }
    if( m_prod->manuf != NULL && m_prod->manuf->descr[0] != L'\0' )
      { DDX_TEXT(IDC_MANUF, m_prod->manuf->descr) }
    if( m_prod->shelf_life[0] != L'\0' )
      { DDX_TEXT(IDC_SHELF_LIFE, m_prod->shelf_life) }
    if( m_prod->code[0] != L'\0' )
      { DDX_TEXT(IDC_CODE, m_prod->code) }
    if( m_prod->art[0] != L'\0' )
      { DDX_TEXT(IDC_ART, m_prod->art) }
    if( m_prod->price > 0.0 )
      { DDX_CUR(IDC_PRICE, m_prod->price, m_hterm->cur_prec) }
    if( m_prod->def_pack != NULL && m_prod->def_pack->weight != 0.0 )
      { DDX_CUR(IDC_WEIGHT, m_prod->def_pack->weight, OMOBUS_WEIGHT_PREC) }
    if( m_prod->def_pack != NULL && m_prod->def_pack->volume != 0.0 )
      { DDX_CUR(IDC_VOLUME, m_prod->def_pack->volume, OMOBUS_VOLUME_PREC) }
    if( m_prod->def_pack != NULL ) 
      { DDX_TEXT(IDC_PACK, m_prod->def_pack->descr) }
	END_DDX_MAP()

	BEGIN_MSG_MAP(CProdDet)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CProdDet>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
    _InitEmptyLabel(m_emptyLabel[0], IDC_CATEG, m_prod->categ != NULL && m_prod->categ->descr[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[1], IDC_MANUF, m_prod->manuf != NULL && m_prod->manuf->descr[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[2], IDC_SHELF_LIFE, m_prod->shelf_life[0] != L'\0', TRUE);
    _InitEmptyLabel(m_emptyLabel[3], IDC_CODE, m_prod->code[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[4], IDC_ART, m_prod->art[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[5], IDC_PRICE, m_prod->price > 0.0, TRUE);
    _InitEmptyLabel(m_emptyLabel[6], IDC_WEIGHT, m_prod->def_pack != NULL && m_prod->def_pack->weight != 0.0);
    _InitEmptyLabel(m_emptyLabel[7], IDC_VOLUME, m_prod->def_pack != NULL && m_prod->def_pack->volume != 0.0);
    _InitEmptyLabel(m_emptyLabel[8], IDC_PACK, m_prod->def_pack != NULL);
    DoDataExchange(DDX_LOAD);
		return bHandled = FALSE;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
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
};

class CNoveltyFrame : 
	public CStdDialogResizeImpl<CNoveltyFrame>,
	public CUpdateUI<CNoveltyFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  products_t *m_prods;
  CListViewCtrl m_list;
  CFont m_capFont, m_baseFont, m_boldFont;

public:
  CNoveltyFrame(HTERMINAL *hterm, products_t *prods) : 
      m_hterm(hterm), m_prods(prods) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CNoveltyFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CNoveltyFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CNoveltyFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CNoveltyFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    _InitList();
    _InitFonts();
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &bHandled) {
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
    rect.left = rect.right - DRA::SCALEX(74);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP3), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(64);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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

    PRODUCT_CONF *ptr = &((*m_prods)[lpdis->itemID]);
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
    rect.bottom = rect.top + DRA::SCALEY(14);
    dc.DrawText(ptr->descr, wcslen(ptr->descr),
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = rect.top + DRA::SCALEY(16);
    if( ptr->categ != NULL ) {
      dc.DrawText(ptr->categ->descr, wcslen(ptr->categ->descr),
        rect.left + DRA::SCALEX(2), rect.top, 
        rect.right - DRA::SCALEX(2), rect.bottom,
        DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);
    }

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(74);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->price > 0.0 ) {
      dc.SelectFont(m_boldFont);
      dc.DrawText(ftows(ptr->price, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(64);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ptr->def_pack != NULL ? ptr->def_pack->descr : L"", -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ptr->code, wcslen(ptr->code), 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

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
    lmi->itemHeight = DRA::SCALEY(49);
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
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
	}

  void _InitList() {
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_prods->size());
  }

  void _InitFonts() {
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  }

  void _Det(size_t sel) {
    if( sel < m_prods->size() )
      CProdDet(m_hterm, &((*m_prods)[sel])).DoModal(m_hWnd);
  }
};

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

// Format: %prod_id%;%price%;
static 
bool __novelty_count(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  HTERMINAL *ptr = (HTERMINAL *)cookie;
  if( ptr == NULL ) {
    return false;
  }
  if( ln == 0 ) {
    return true;
  }
  if( argc > 1 ) {
    ptr->novelty_count++;
  }
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

// Format: %prod_id%;%price%;
static 
bool __r_novelties(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  novelties_t *novs = (novelties_t *)cookie;
  if( ln == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 2 ) {
    return false;
  }
  if( (*novs).find(argv[1]) == (*novs).end() ) {
    (*novs).insert(novelties_t::value_type(argv[0], wcstof(argv[1])));
  } else {
    (*novs)[argv[0]] = wcstof(argv[1]);
  }
  return true;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;%manuf_id%;%shelf_life%;%brand_id%;%fractional%;%categ_id%;
static 
bool __product(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  if( ln == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 11 ) {
    return false;
  }

  __int_product_cookie *ptr = (__int_product_cookie *)cookie;
  const wchar_t *prod_id = argv[1];
  const wchar_t *manuf_id = argv[6];
  const wchar_t *categ_id = argv[10];
  novelties_t::iterator nov_it;
  simple_t::iterator mf_it, categ_it;
  product_packs_t::iterator pack_it;
  packs_t::iterator base_pack_it, def_pack_it;
  PRODUCT_CONF cnf;

  if( _wtoi(argv[2]) ) { //folder?
    return true;
  }
  if( (nov_it = ptr->novs->find(prod_id)) == ptr->novs->end() ) {
    return true;
  }

  memset(&cnf, 0, sizeof(PRODUCT_CONF));
  COPY_ATTR__S(cnf.code, argv[3]);
  COPY_ATTR__S(cnf.art, argv[4]);
  COPY_ATTR__S(cnf.descr, argv[5]);
  COPY_ATTR__S(cnf.shelf_life, argv[7]);
  cnf.fractional = (byte_t)_wtoi(argv[9]);
  cnf.price = nov_it->second;
  // Категория
  categ_it = std::find_if(ptr->categ->begin(), ptr->categ->end(), find_simple(categ_id));
  cnf.categ = categ_it != ptr->categ->end() ? &(*categ_it) : NULL;
  // Производитель
  mf_it = std::find_if(ptr->mf->begin(), ptr->mf->end(), find_simple(manuf_id));
  cnf.manuf = mf_it != ptr->mf->end() ? &(*mf_it) : NULL;
  // Определяем наличие упаковок.
  if( (pack_it = ptr->packs->find(prod_id)) == ptr->packs->end() || pack_it->second.empty() ) {
    ATLTRACE(L"__product: У продукта нет упаковок (prod_id='%s')!\n", prod_id);
  } else {
    cnf.packs = &pack_it->second;
    // Ищем базовую упаковку. Базовая упаковка обязательна, если её нет,
    // то все остальные упаковки игнорируем.
      packs_t::iterator pack_it = std::find_if(cnf.packs->begin(),
        cnf.packs->end(), find_base_pack());
      if( pack_it != cnf.packs->end() ) {
        cnf.def_pack = cnf.base_pack = &(*pack_it);
      }
  }
  
  ptr->prods->push_back(cnf);

  return true;
}

// Format: %pack_id%;%prod_id%;%descr%;%pack%;%weight%;%volume%;
static 
bool __pack(void *cookie, int ln, const wchar_t **argv, int argc)
{
  if( ln == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 6 ) {
    return false;
  }

  product_packs_t *ptr = (product_packs_t *)cookie;
  const wchar_t *prod_id = argv[1];
  PACK_CONF cnf; memset(&cnf, 0, sizeof(PACK_CONF));
  COPY_ATTR__S(cnf.pack_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[2]);
  cnf.pack = wcstof(argv[3]);
  cnf.weight = wcstof(argv[4]);
  cnf.volume = wcstof(argv[5]);
  if( cnf.pack <= 0.0 )
    return true;

  product_packs_t::iterator it = ptr->find(prod_id);
  if( it == ptr->end() ) {
    packs_t l; l.push_back(cnf);
    ptr->insert(product_packs_t::value_type(prod_id, l));
  } else {
    it->second.push_back(cnf);
  }

  return true;
}

std::wstring GetMsg(HTERMINAL *h) {
  std::wstring msg = wsfromRes(h->novelty_count > 0 ? IDS_STR1 : IDS_STR0);
  if( h->novelty_count > 0 )
    msg += itows(h->novelty_count);
  return msg;
}

static
void OnReload(HTERMINAL *h) {
  h->novelty_count = 0;
  ReadOmobusManual(L"novelties", h, __novelty_count);
  ATLTRACE(L"novelties(count) = %i\n", h->novelty_count);
}

static
void OnShow(HTERMINAL *h) {
  ATLTRACE(L"novelties: OnShown\n");
  OnReload(h);
  SetTimer(h->hWnd, 1, 600000, NULL);
}

static
void OnHide(HTERMINAL *h) {
  KillTimer(h->hWnd, 1);
  h->novelty_count = 0;
  ATLTRACE(L"novelties: OnHide\n");
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) 
{
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2) 
  };
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->novelty_count ? h->hFontB : h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, GetMsg(h).c_str(), -1, &rect, DT_LEFT);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor _wc;
  simple_t mf, categ;
  novelties_t novs;
  products_t prods;
  product_packs_t prod_packs;
  __int_product_cookie ck_prod = { &prods, &mf, &categ, &novs, &prod_packs };

  h->novelty_count = 0;
  ReadOmobusManual(L"novelties", &novs, __r_novelties);
  ATLTRACE(L"novelties = %i\n", novs.size());
  if( novs.empty() ) {
    return;
  }
  h->novelty_count = novs.size();
  ReadOmobusManual(L"manufacturers", &mf, __simple);
  ATLTRACE(L"manufacturers = %i\n", mf.size());
  ReadOmobusManual(L"categories", &categ, __simple);
  ATLTRACE(L"categories = %i\n", categ.size());
  ReadOmobusManual(L"packs", &prod_packs, __pack);
  ATLTRACE(L"packs = %i\n", prod_packs.size());
  ReadOmobusManual(L"products", &ck_prod, __product);
  ATLTRACE(L"products = %i/%i (size=%i Kb)\n", sizeof(PRODUCT_CONF), prods.size(), sizeof(PRODUCT_CONF)*prods.size()/1024);

  _wc.Restore();
  CNoveltyFrame(h, &prods).DoModal(h->hParWnd);
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
  case WM_TIMER:
  case OMOBUS_SSM_PLUGIN_RELOAD:
    OnReload((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
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
  pf_terminals_put_conf /*put_conf*/, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->hParWnd = hParWnd;
  h->cur_prec = _wtoi(get_conf(OMOBUS_CUR_PRECISION, OMOBUS_CUR_PRECISION_DEF));
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK);
  }

  return h;
}

void terminal_deinit(void *p) 
{
  ATLTRACE(L"novelties: terminal_deinit\n");
  HTERMINAL *h = (HTERMINAL *)p;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
