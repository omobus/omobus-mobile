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

#define SHOW_UNAVAILABLE_PRODUCTS        L"order->show_unavailable_products"
#define SHOW_UNAVAILABLE_PRODUCTS_DEF    L"no"
#define SELECT_UNAVAILABLE_PRODUCTS      L"order->select_unavailable_products"
#define SELECT_UNAVAILABLE_PRODUCTS_DEF  L"no"
#define SHOW_UNKNOWN_PRICE_PRODUTS       L"order->show_unknown_price_produts"
#define SHOW_UNKNOWN_PRICE_PRODUTS_DEF   L"no"
#define SELECT_UNKNOWN_PRICE_PRODUTS     L"order->select_unknown_price_produts"
#define SELECT_UNKNOWN_PRICE_PRODUTS_DEF L"no"
#define PAY_DELAY                        L"order->pay_delay"
#define PAY_DELAY_DEF                    L"no"
#define ALLOW_CHANGING_PACK              L"order->allow_changing_pack"
#define ALLOW_CHANGING_PACK_DEF          L"no"

#define BEGIN_NAV_MAP() \
  bool OnNavigate(HWND hWnd, UINT nID) { \
    if( FALSE ) {}

#define NAV_ELEMENT(uID, cl, hlb) \
    else if( nID == uID ) {\
      if( cl(m_hterm, hlb.l, &hlb.cur).DoModal(m_hWnd) == ID_ACTION ) {\
        _SetHyperLabel(hlb);\
      }\
    }

#define END_NAV_MAP() \
    return true;\
  }

#define QTY_STD_SHIDIF SHIDIF_SIZEDLGFULLSCREEN|SHIDIF_CANCELBUTTON/*SHIDIF_DONEBUTTON*/

#define _DEF_PACK_MENUITEM_BEGIN  65000
#define _DEF_PACK_MENUITEM_END    65015

#define ALLOW_CHANGING_PACK__NO       0
#define ALLOW_CHANGING_PACK__YES      1
#define ALLOW_CHANGING_PACK__AUTO     2

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
#include <atlminihtml.h>

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
static TCHAR _szWindowClass[] = L"omobus-order-terminal-element";
static int _nHeight = 20;

struct PRICE_CONF {
  wchar_t pack_id[OMOBUS_MAX_ID + 1];
  cur_t price;
};
typedef std::map<std::wstring, PRICE_CONF> prices_t;

typedef std::map<std::wstring, qty_t> wareh_stocks_t;
typedef std::vector<std::wstring> slist_t;
typedef std::map<std::wstring, std::wstring> recl_percentage_t;

struct LOCK_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  bool soft_lock;
};
typedef std::vector<LOCK_CONF> black_list_t;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

struct WARN_CONF {
  std::wstring caption, body;
};
typedef std::vector<WARN_CONF> warn_t;

struct SALES_H {
  cur_t amount_c, amount_r;
  qty_t qty_c, qty_r;
  int32_t color, bgcolor;
};
struct SALES_H_CONF : public SALES_H {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  tm date;
};
typedef std::vector<SALES_H_CONF *> sales_h_t;

struct ACTION_CONF {
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  wchar_t pack_id[OMOBUS_MAX_ID + 1];
};
typedef std::vector<ACTION_CONF> actions_t;

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
  int _i, _l, _vf;
  wchar_t pid[OMOBUS_MAX_ID + 1];
  wchar_t prod_id[OMOBUS_MAX_ID + 1];
  byte_t ftype;
  wchar_t code[OMOBUS_MAX_ID + 1];
  wchar_t art[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
  const wchar_t *recl_percentage;
  SIMPLE_CONF *manuf;
  PACK_CONF *base_pack, *def_pack, *doc_pack;
  packs_t *packs;
  wchar_t shelf_life[OMOBUS_MAX_DESCR + 1];
  bool fractional;
  PRICE_CONF *price;
  qty_t wareh_stock;
  SALES_H tsh;
  bool locked;
  int32_t color, bgcolor;
  ACTION_CONF *action;
  //  Limits:
  PACK_CONF *min_pack, *max_pack;
  int min_qty, max_qty;
};
typedef std::vector<PRODUCT_CONF*> products_t;
typedef std::map<PRODUCT_CONF*, products_t*> vf_gr_t;

struct H_ORDER {
  SIMPLE_CONF *spn_conf, *wareh_conf, *type_conf;
  double weight, volume;
  cur_t amount;
  int qty_pos, pay_delay;
  wchar_t note[OMOBUS_MAX_NOTE + 1];
  const wchar_t *account_id, *gr_price_id, *activity_type_id, 
  	*w_cookie, *a_cookie, *del_date;
  omobus_location_t posCre;
  time_t ttCre;
};

struct T_ORDER {
  PRODUCT_CONF *pr_conf;
  bool predefined;
  PACK_CONF *pack;
  qty_t qty;
};
typedef std::vector<T_ORDER> T_ORDER_V;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  int cur_prec, qty_prec, allow_changing_pack;
  bool show_unavailable_products, show_unknown_price_produts,
    select_unavailable_products, select_unknown_price_produts,
    pay_delay, focus;
  const wchar_t *user_id, *pk_ext, *pk_mode, *date_fmt, 
    *thousand_sep;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

template< class T >
void man_cleanup(T &v) {
  size_t size = v.size(), i = 0;
  for( ; i < size; i++ ) {
    if( v[i] != NULL ) {
      delete v[i];
      v[i] = NULL;
    }
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

class find_order_product :
  public std::unary_function<T_ORDER, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_order_product(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(T_ORDER &v) const {
    return COMPARE_ID(m_prod_id, v.pr_conf->prod_id);
  }
};

class find_product :
  public std::unary_function<PRODUCT_CONF, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_product(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(PRODUCT_CONF *v) const {
    return COMPARE_ID(m_prod_id, v->prod_id);
  }
};

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

class find_vf_gr :
  public std::unary_function<vf_gr_t::value_type, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_vf_gr(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(vf_gr_t::value_type &v) const {
    return COMPARE_ID(m_prod_id, v.first->prod_id);
  }
};

class find_action :
  public std::unary_function<ACTION_CONF, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_action(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(ACTION_CONF &v) const {
    return COMPARE_ID(m_prod_id, v.prod_id);
  }
};

class total_sales_h
{
public:
  total_sales_h(const wchar_t *prod_id, SALES_H *sh) : 
      m_prod_id(prod_id), m_sh(sh) {
    m_sh->color = m_sh->bgcolor = 0;
  }
  void operator()(SALES_H_CONF *el) {
    if( m_prod_id == NULL || COMPARE_ID(m_prod_id, el->prod_id) ) {
      m_sh->amount_c += el->amount_c; m_sh->amount_r += el->amount_r;
      m_sh->qty_c += el->qty_c; m_sh->qty_r += el->qty_r;
    }
  }

  const wchar_t *m_prod_id;
  SALES_H *m_sh;
};

class prod_sales_h
{
public:
  prod_sales_h(const wchar_t *prod_id, sales_h_t *psh) : 
      m_prod_id(prod_id), m_psh(psh) {
  }
  void operator()(SALES_H_CONF *el) const {
    if( m_prod_id != NULL && COMPARE_ID(m_prod_id, el->prod_id) )
      m_psh->push_back(el);
  }

  const wchar_t *m_prod_id;
  sales_h_t *m_psh;
};

class find_lock :
  public std::unary_function<LOCK_CONF, bool> {
private:
  const wchar_t *m_prod_id;
public:
  find_lock(const wchar_t *prod_id) : m_prod_id(prod_id) {
  }
  bool operator()(LOCK_CONF &v) const {
    return COMPARE_ID(m_prod_id, v.prod_id);
  }
};

struct product_cookie {
  products_t *prods;
  prices_t *prices;
  wareh_stocks_t *wst;
  simple_t *mf;
  black_list_t *black_list;
  sales_h_t *sh;
  product_packs_t *packs;
  recl_percentage_t *recl_percentage;
  actions_t *actions;
  bool show_unavailable_products, show_unknown_price_produts;
};

struct p_order_cookie { 
  T_ORDER_V *t; products_t *prod;
  const wchar_t *account_id; 
};

struct highlight_cookie { 
  products_t *prod;
  const wchar_t *account_id; 
};

struct price_cookie {
  const wchar_t *price_id, *price_date;
  prices_t *prices;
};

struct wareh_stocks_cookie {
  const wchar_t *wareh_id;
  wareh_stocks_t *wst;
};

struct sales_h_cookie {
  const wchar_t *account_id; 
  sales_h_t *sales_h;
};

typedef struct _black_list_cookie_t {
  const wchar_t *account_id; 
  black_list_t *l;
} black_list_cookie_t;

struct warn_cookie {
  const wchar_t *account_id; 
  warn_t *warn;
};

typedef struct _recl_percentage_cookie_t {
  const wchar_t *account_id; 
  recl_percentage_t *l;
} recl_percentage_cookie_t;

typedef struct _vf_products_cookie_t {
  const wchar_t *account_id;
  vf_gr_t *vf_gr;
  products_t *prods;
} vf_products_cookie_t;

inline
cur_t row_unit_price(const T_ORDER *t) {
  return t->pr_conf->price == NULL ? 0.0 :
    t->pack->pack/t->pr_conf->def_pack->pack*t->pr_conf->price->price;
}

inline
cur_t row_amount(const T_ORDER *t) {
  return t->qty*row_unit_price(t);
}

inline
double row_weight(const T_ORDER *t) {
  return t->qty*t->pack->weight;
}

inline
double row_volume(const T_ORDER *t) {
  return t->qty*t->pack->volume;
}

static bool CloseDoc(HTERMINAL *h, H_ORDER *doc_h, T_ORDER_V *doc_t);
static slist_t &parce_slist(const wchar_t *s, slist_t &sl);
static bool IsEqual(const wchar_t *src, slist_t &l);
static std::wstring fmtDate(HTERMINAL *h, const wchar_t *d);

class CTypeFrame : 
  public CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CTypeFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CWarehFrame : 
  public CListFrame<IDD_WAREHS, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CWarehFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_WAREHS, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CPriceFrame : 
  public CListFrame<IDD_PRICES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CPriceFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_PRICES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CPackFrame : 
  public CListFrame<IDD_PACKS, IDC_LIST, packs_t, PACK_CONF> 
{
public:
  CPackFrame(HTERMINAL *hterm, packs_t *l, PACK_CONF **cur) :
    CListFrame<IDD_PACKS, IDC_LIST, packs_t, PACK_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CNotifyFrame : 
	public CStdDialogResizeImpl<CNotifyFrame>,
	public CMessageFilter
{
private:
  const wchar_t *m_caption, *m_body;
  CMiniHtmlCtrl m_ctrlLabel;

public:
  CNotifyFrame(const wchar_t *caption, const wchar_t *body) : 
      m_caption(caption), m_body(body) {
  }

	enum { IDD = IDD_WARN };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CNotifyFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CNotifyFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CNotifyFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON);
    if( m_caption != NULL ) {
      SetWindowText(m_caption);
    }
    m_ctrlLabel.SubclassWindow(GetDlgItem(IDC_BODY));
    m_ctrlLabel.SetWordWrap(TRUE);
    if( m_body != NULL ) {
      m_ctrlLabel.SetWindowText(m_body);
    }
		return bHandled = FALSE;
	}

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(ID_ACTION);
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
    return 0;
  }
};

class CConfirmFrame : 
	public CStdDialogResizeImpl<CConfirmFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  H_ORDER *m_doc_h;
  CLabel m_del_date, m_amount;

public:
  CConfirmFrame(HTERMINAL *hterm, H_ORDER *doc_h) : m_hterm(hterm), m_doc_h(doc_h) {
  }

	enum { IDD = IDD_CONFIRM };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CConfirmFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CConfirmFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CConfirmFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON);
    _InitLabel(m_del_date, IDC_DEL_DATE, m_doc_h->del_date == NULL ? NULL : 
      fmtDate(m_hterm, m_doc_h->del_date).c_str());
    _InitLabel(m_amount, IDC_AMOUNT, ftows(m_doc_h->amount, m_hterm->cur_prec, 
      m_hterm->thousand_sep).c_str());
		return bHandled = FALSE;
	}

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(ID_ACTION);
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
    return 0;
  }

private:
  void _InitLabel(CLabel &l, UINT uID, const wchar_t *s) {
    l.SubclassWindow(GetDlgItem(uID));
    if( s != NULL && s[0] != L'\0' ) {
      l.SetFontBold(TRUE);
      l.SetText(s);
    } else {
      l.SetFontItalic(TRUE);
    }
  }
};

class CDocDet : 
	public CStdDialogResizeImpl<CDocDet>,
	public CMessageFilter
{
  struct hyper_button_t {
    simple_t *l; SIMPLE_CONF *cur;
    CHyperButtonTempl<CDocDet> ctrl;
  };

protected:
  HTERMINAL *m_hterm;
  H_ORDER *m_doc;
  hyper_button_t m_type;
  CEdit m_note, m_p_delay;

public:
  CDocDet(HTERMINAL *hterm, H_ORDER *doc, simple_t *type) : m_hterm(hterm), m_doc(doc) {
    m_type.l = type;
    m_type.cur = m_doc->type_conf; 
  }

  enum { IDD = IDD_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CDocDet)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CDocDet)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnEditColor)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_MENU_SAVE, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnAction)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CDocDet>)
	END_MSG_MAP()

  BEGIN_NAV_MAP()
    NAV_ELEMENT(IDC_DOC_TYPE, CTypeFrame, m_type)
  END_NAV_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		AtlCreateMenuBar(m_hWnd, IDR_DET_MENU);
    _InitEdit(m_note, IDC_DOC_NOTE, m_doc->note);
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc->note));
    _InitEdit(m_p_delay, IDC_PAY_DELAY, itows(m_doc->pay_delay).c_str(), m_hterm->pay_delay?TRUE:FALSE);
    _InitHyperButton(m_type, IDC_DOC_TYPE);
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

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    m_note.GetWindowText(m_doc->note, CALC_BUF_SIZE(m_doc->note));
    m_doc->pay_delay = _GetEditInt(m_p_delay);
    m_doc->type_conf = m_type.cur; 
    EndDialog(IDOK);
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(IDCANCEL);
    return 0;
  }

private:
  void _InitEdit(CEdit &ctrl, UINT uID, const wchar_t *def, BOOL enabled=TRUE) {
    ctrl = GetDlgItem(uID);
    ctrl.SetWindowText(def);
    ctrl.EnableWindow(enabled);
  }

  void _InitHyperButton(hyper_button_t &hlb, UINT uID) {
    hlb.ctrl.SubclassWindow(GetDlgItem(uID));
    hlb.ctrl.SetParent(this, uID);
    hlb.ctrl.EnableWindow(hlb.l->size() > 1 );
    _SetHyperLabel(hlb);
  }

  void _SetHyperLabel(hyper_button_t &hlb) {
    if( hlb.l == NULL || hlb.l->empty() || hlb.cur == NULL )
      return;
    hlb.ctrl.SetTitle(hlb.cur->descr);
  }

  int _GetEditInt(CEdit &ctrl) {
    CString tmp; ctrl.GetWindowText(tmp);
    return _wtoi(tmp);
  }
};

class CProdDet : 
	public CStdDialogResizeImpl<CProdDet>,
	public CMessageFilter, 
  public CWinDataExchange<CProdDet>
{
protected:
  HTERMINAL *m_hterm;
  PRODUCT_CONF *m_prod;
  CLabel m_emptyLabel[10];
  CString m_sales_c, m_sales_r;

public:
  CProdDet(HTERMINAL *hterm, PRODUCT_CONF *prod) : 
    m_hterm(hterm), m_prod(prod) {
    if( prod->tsh.amount_c != 0.0 || prod->tsh.qty_c != 0.0 ) {
      m_sales_c.Format(L"%s (%s)", ftows(prod->tsh.amount_c, hterm->cur_prec, m_hterm->thousand_sep).c_str(),
        ftows(prod->tsh.qty_c, hterm->qty_prec).c_str());
    }
    if( prod->tsh.amount_r != 0.0 || prod->tsh.qty_r != 0.0 ) {
      m_sales_r.Format(L"%s (%s)", ftows(prod->tsh.amount_r, hterm->cur_prec, m_hterm->thousand_sep).c_str(),
        ftows(prod->tsh.qty_r, hterm->qty_prec).c_str());
    }
  }

	enum { IDD = IDD_PROD_DET };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CProdDet)
  END_DLGRESIZE_MAP()

	BEGIN_DDX_MAP(CProdDet)
		DDX_TEXT(IDC_NAME, m_prod->descr)
    if( m_prod->manuf != NULL && m_prod->manuf->descr[0] != L'\0' )
      { DDX_TEXT(IDC_MANUF, m_prod->manuf->descr) }
    if( m_prod->shelf_life[0] != L'\0' )
      { DDX_TEXT(IDC_SHELF_LIFE, m_prod->shelf_life) }
    if( m_prod->code[0] != L'\0' )
      { DDX_TEXT(IDC_CODE, m_prod->code) }
    if( m_prod->art[0] != L'\0' )
      { DDX_TEXT(IDC_ART, m_prod->art) }
    if( m_prod->price != NULL )
      { DDX_CUR(IDC_PRICE, m_prod->price->price, m_hterm->cur_prec) }
    if( m_prod->def_pack != NULL && m_prod->def_pack->weight != 0.0 )
      { DDX_CUR(IDC_WEIGHT, m_prod->def_pack->weight, OMOBUS_WEIGHT_PREC) }
    if( m_prod->def_pack != NULL && m_prod->def_pack->volume != 0.0 )
      { DDX_CUR(IDC_VOLUME, m_prod->def_pack->volume, OMOBUS_VOLUME_PREC) }
    if( m_prod->def_pack != NULL ) 
      { DDX_TEXT(IDC_PACK, m_prod->def_pack->descr) }
    if( !m_sales_c.IsEmpty() )
      { DDX_TEXT(IDC_SH_C, m_sales_c) } 
    if( !m_sales_r.IsEmpty() )
      { DDX_TEXT(IDC_SH_R, m_sales_r) }
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
    _InitEmptyLabel(m_emptyLabel[0], IDC_MANUF, m_prod->manuf != NULL && m_prod->manuf->descr[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[1], IDC_SHELF_LIFE, m_prod->shelf_life[0] != L'\0', TRUE);
    _InitEmptyLabel(m_emptyLabel[2], IDC_CODE, m_prod->code[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[3], IDC_ART, m_prod->art[0] != L'\0');
    _InitEmptyLabel(m_emptyLabel[4], IDC_PRICE, m_prod->price != NULL, TRUE);
    _InitEmptyLabel(m_emptyLabel[5], IDC_WEIGHT, m_prod->def_pack != NULL && m_prod->def_pack->weight != 0.0);
    _InitEmptyLabel(m_emptyLabel[6], IDC_VOLUME, m_prod->def_pack != NULL && m_prod->def_pack->volume != 0.0);
    _InitEmptyLabel(m_emptyLabel[7], IDC_PACK, m_prod->def_pack != NULL);
    _InitEmptyLabel(m_emptyLabel[8], IDC_SH_C, !m_sales_c.IsEmpty(), TRUE);
    _InitEmptyLabel(m_emptyLabel[9], IDC_SH_R, !m_sales_r.IsEmpty(), TRUE);
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

class CSaHDetDlg : 
  public CStdDialogResizeImpl<CSaHDetDlg>,
  public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  CListViewCtrl m_list;
  PRODUCT_CONF *m_prod;
  sales_h_t *m_prod_sh;
  CFont m_capFont, m_baseFont, m_boldFont;

public:
  CSaHDetDlg(HTERMINAL *hterm, PRODUCT_CONF *prod, sales_h_t *prod_sh) : 
      m_hterm(hterm), m_prod(prod), m_prod_sh(prod_sh) {
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
  }

	enum { IDD = IDD_PROD_SH_DET };

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
    AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_CANCEL, SHCMBF_HIDESIPBUTTON);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_prod_sh->size());
    m_list.ShowWindow(m_prod_sh->empty()?SW_HIDE:SW_NORMAL);
    SetDlgItemText(IDC_PROD, m_prod->descr);
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
    CFont oldFont = dc.SelectFont(m_capFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    rect.top = GetTitleHeight() + DRA::SCALEY(40);
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1,
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

    SALES_H_CONF *ptr = (*m_prod_sh)[lpdis->itemID];
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
    rect.bottom = rect.top + DRA::SCALEY(17);
    dc.DrawText(wsftime(&ptr->date, m_hterm->date_fmt).c_str(), -1, 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    dc.SelectFont(m_baseFont);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(80);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->amount_c, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    dc.DrawText(ftows(ptr->amount_r, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1,
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(14), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(50);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ftows(ptr->qty_c, m_hterm->qty_prec).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    dc.DrawText(ftows(ptr->qty_r, m_hterm->qty_prec).c_str(), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(14), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(20);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP4), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);
    dc.DrawText(LoadStringEx(IDS_CAP5), -1, 
      rect.left + DRA::SCALEX(4), rect.top + DRA::SCALEY(14), 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE);

    dc.DrawLine(rect.left, rect.top + DRA::SCALEY(14), 
      lpdis->rcItem.right, rect.top + DRA::SCALEY(14));

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
    lmi->itemHeight = DRA::SCALEY(48);
    return 1;
  }
};

class CQtyDlg : 
	public CStdDialogResizeImpl<CQtyDlg, QTY_STD_SHIDIF>,
	public CMessageFilter
{
  typedef CStdDialogResizeImpl<CQtyDlg, QTY_STD_SHIDIF> baseClass;

protected:
  CEdit m_qty_e;
  CHyperButtonTempl<CQtyDlg> m_pack;
  HTERMINAL *m_hterm;
  T_ORDER *m_tpart;
  PACK_CONF *m_cur_pack;
  qty_t m_qty;

public:
  CQtyDlg(HTERMINAL *hterm, T_ORDER *tpart) : m_hterm(hterm), m_tpart(tpart) {
    _InitPack();
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
    SetDlgItemText(IDC_NAME, m_tpart->pr_conf->descr);
    //SetDlgItemText(IDC_MANUF, m_tpart->pr_conf->manuf == NULL ? L"" : m_tpart->pr_conf->manuf->descr);
    SetDlgItemText(IDC_CODE, m_tpart->pr_conf->code);
    if( m_tpart->pr_conf->recl_percentage != NULL ) {
      SetDlgItemText(IDC_RECL_PERCENTAGE, m_tpart->pr_conf->recl_percentage);
    }
    _InitPackCtrl();
    _InitQtyCtrl();
    _Recalc();
		return bHandled = FALSE;
	}

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString s, msg; 
    m_qty_e.GetWindowText(s); s.Replace(L",", L".");
    qty_t cur_qty = ftof(wcstof(s), m_hterm->qty_prec);
    if( m_tpart->pr_conf->min_pack != NULL && m_tpart->pr_conf->min_qty > 0.0 && m_cur_pack->pack*cur_qty < m_tpart->pr_conf->min_pack->pack*m_tpart->pr_conf->min_qty ) {
        msg.Format(IDS_STR13, m_tpart->pr_conf->min_qty, m_tpart->pr_conf->min_pack->descr);
        MessageBoxEx(m_hWnd, msg, IDS_WARN, MB_ICONSTOP);
    } else if( m_tpart->pr_conf->max_pack != NULL && m_tpart->pr_conf->max_qty > 0.0 && m_cur_pack->pack*cur_qty > m_tpart->pr_conf->max_pack->pack*m_tpart->pr_conf->max_qty ) {
        msg.Format(IDS_STR14, m_tpart->pr_conf->max_qty, m_tpart->pr_conf->max_pack->descr);
        MessageBoxEx(m_hWnd, msg, IDS_WARN, MB_ICONSTOP);
    } else {
      _change_doc_pack(); // Смена упаковки в Т.Ч., если разрешено настройками.
      m_tpart->qty = _qty();
      SHSipPreference(m_hWnd, SIP_DOWN);
      EndDialog(ID_ACTION);
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    SHSipPreference(m_hWnd, SIP_DOWN);
    EndDialog(wID);
    return 0;
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( CPackFrame(m_hterm, m_tpart->pr_conf->packs, &m_cur_pack).DoModal(m_hWnd) == ID_ACTION ) {
      _SetPackLabel();
      _Recalc();
      SHSipPreference(m_hWnd, SIP_DOWN);
    }
    return true;
  }

private:
  void _InitPackCtrl() {
    m_pack.SubclassWindow(GetDlgItem(IDC_PACK));
    m_pack.SetParent(this, IDC_PACK);
    m_pack.EnableWindow(m_tpart->pr_conf->packs->size() > 1);
    _SetPackLabel();
  }

  void _InitQtyCtrl() {
    m_qty_e = GetDlgItem(IDC_QTY);
    m_qty_e.ModifyStyle(m_tpart->pr_conf->fractional ? ES_NUMBER : 0, 
      m_tpart->pr_conf->fractional ? 0 : ES_NUMBER);
    m_qty_e.SetWindowText(ftows(m_qty, 
      m_tpart->pr_conf->fractional ? m_hterm->qty_prec : 0).c_str());
    m_qty_e.SetFocus(); 
    m_qty_e.SetSelAll();
  }

  void _SetPackLabel() {
    if( m_tpart->pr_conf->packs->empty() || m_cur_pack == NULL )
      return;
    m_pack.SetTitle(m_cur_pack->descr);
  }

  void _InitPack() {
    m_qty = 1; // Количество по-умолчанию
    m_cur_pack = NULL;
    // Для весовых продуктов используем всегда упаковку заданную
    // в табличной части документа.
    if( m_tpart->pr_conf->fractional ) {
      m_qty = m_tpart->qty==0?1:m_tpart->qty;
      m_cur_pack = m_tpart->pack;
    } else {
    // Для невесовых продуктов выполняем подбор наиболее подходящей
    // упаковки с учетом того, что для невесовых продуктов задание
    // дробных количеств запрещено.
      if( m_tpart->qty == 0 ) { // Новый продукт
        m_qty = 1;
        m_cur_pack = m_tpart->pack;
      } else if( m_tpart->qty == (qty_t)((uint32_t)m_tpart->qty) ) {
        m_qty = m_tpart->qty;
        m_cur_pack = m_tpart->pack;
      } else {
        // Используем базовую упаковку т.к. её делить нельзя.
        m_qty = ftof(m_tpart->qty*m_tpart->pack->pack/m_tpart->pr_conf->base_pack->pack, 0);
        m_cur_pack = m_tpart->pr_conf->base_pack;
      }
    }
  }

  void _Recalc() {
    qty_t pack = _def_pack();
    if( m_tpart->pr_conf->wareh_stock > 0 )
      SetDlgItemText(IDC_WST, ftows(m_tpart->pr_conf->wareh_stock/pack, m_hterm->qty_prec).c_str());
    if( m_tpart->pr_conf->price != NULL )
      SetDlgItemText(IDC_PRICE, ftows(m_tpart->pr_conf->price->price*pack, m_hterm->cur_prec).c_str());
  }

  qty_t _cur_pack() {
    return m_cur_pack == NULL ? 1 : m_cur_pack->pack;
  }

  qty_t _doc_pack() {
    return _cur_pack()/m_tpart->pack->pack;
  }

  qty_t _def_pack() {
    return _cur_pack()/m_tpart->pr_conf->def_pack->pack;
  }

  qty_t _qty() {
    CString s; m_qty_e.GetWindowText(s); s.Replace(L",", L".");
    return ftof(wcstof(s)*_doc_pack(), m_hterm->qty_prec);
  }

  void _change_doc_pack() {
    if( m_hterm->allow_changing_pack != ALLOW_CHANGING_PACK__AUTO )
      return;
    if( m_cur_pack == NULL ) 
      return;
    for( size_t i = 0; i < m_tpart->pr_conf->packs->size(); i++ ) {
      if( COMPARE_ID(m_cur_pack->pack_id, (*m_tpart->pr_conf->packs)[i].pack_id) ) {
        m_tpart->pack = &(*m_tpart->pr_conf->packs)[i];
        break;
      }
    }
  }
};

class CProductTreeFrame : 
	public CStdDialogResizeImpl<CProductTreeFrame>,
  public CUpdateUI<CProductTreeFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  products_t *m_cur_pr;
  PRODUCT_CONF *m_cur_sel;
  CString m_fmt0, m_tmp;
  CListViewCtrl m_list;
  CFont m_baseFont, m_folderFont;
  CBrush m_hbrSel, m_hbrOdd;
  CPen m_dashPen;

public:
  CProductTreeFrame(HTERMINAL *h, products_t *cur_ac, PRODUCT_CONF *cur_sel) : 
    m_hterm(h), m_cur_pr(cur_ac), m_cur_sel(cur_sel) {
    m_folderFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
  }

	enum { IDD = IDD_TREE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CProductTreeFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CProductTreeFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CProductTreeFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CProductTreeFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_fmt0.LoadStringW(IDS_FMT0);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_cur_pr->size());
    m_list.SelectItem(_GetSel(m_cur_sel));
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    PRODUCT_CONF *ptr = (*m_cur_pr)[lpdis->itemID];
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
    EndDialog(wID);
    return 0;
  }

  PRODUCT_CONF *GetSelElem() {
    return m_cur_sel;
  }

protected:
  void _Ret(size_t cur) {
    if( cur >= m_cur_pr->size() )
      return;
    m_cur_sel = (*m_cur_pr)[cur];
    EndDialog(ID_ACTION);
  }

  int _GetSel(PRODUCT_CONF *p) {
    size_t size = m_cur_pr->size(), i = 0;
    for( i = 0; i < size; i++ ) {
      if( p == (*m_cur_pr)[i] )
        return i;
    }
    return -1;
  }

  void _LockMainMenuItems() {
    UIEnable(ID_ACTION, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
  }
};

class CProductsFrame : 
	public CStdDialogResizeImpl<CProductsFrame>,
	public CUpdateUI<CProductsFrame>,
	public CMessageFilter
{
protected:
  HTERMINAL *m_hterm;
  products_t *m_pr, m_cur_pr;
  vf_gr_t *m_vf_gr;
  sales_h_t *m_sh;
  T_ORDER_V *m_doc_t;
  PRODUCT_CONF *m_parent, m_vf[2];
  CHyperButtonTempl<CProductsFrame> m_vfctrl;
  CListViewCtrl m_list;
  CDimEdit m_name_ff, m_code_ff;
  CMenu m_ctxmenu;
  CFont m_capFont, m_baseFont, m_boldFont, m_italicFont;
  CString m_amount;

public:
  CProductsFrame(HTERMINAL *hterm, products_t *pr, vf_gr_t *vf_gr, sales_h_t *sh, 
      T_ORDER_V *doc_t) : m_hterm(hterm), m_pr(pr), m_vf_gr(vf_gr), m_sh(sh), m_doc_t(doc_t) {
    m_parent = &m_vf[0];
    memset(&m_vf, 0, sizeof(m_vf));
    m_vf[0]._vf = m_vf[1]._vf = 1;
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_boldFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_italicFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  }
  
	enum { IDD = IDD_PRODUCTS };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CMainFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CProductsFrame)
    UPDATE_ELEMENT(ID_PRODUCTS_ADD, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CProductsFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnEditColor)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(ID_PRODUCTS_ADD, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_PROD_DET, OnProdDet)
		COMMAND_ID_HANDLER(ID_PROD_SH, OnProdSH)
    COMMAND_HANDLER(IDC_NAME_FF, EN_CHANGE, OnFilter)
    COMMAND_HANDLER(IDC_CODE_FF, EN_CHANGE, OnFilter)
    NOTIFY_HANDLER(IDC_LIST, LVN_ODFINDITEM, OnFindItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CProductsFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(CreateMenuBar(IDR_PRODUCTS_MENU));
		UIAddChildWindowContainer(m_hWnd);
    COPY_ATTR__S(m_vf[0].descr, LoadStringEx(IDS_GROUP_ALL));
    COPY_ATTR__S(m_vf[1].descr, LoadStringEx(IDS_GROUP_SALES_H));
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_NOHSCROLL | LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_ctxmenu.LoadMenu(IDR_POPUP_MENU);
    m_vfctrl.SubclassWindow(GetDlgItem(IDC_VF));
    m_vfctrl.SetParent(this, IDC_VF);
    if( m_sh && !m_sh->empty() ) {
      m_parent = &m_vf[1];
    } 
    if( !m_vf_gr->empty() ) {
      for( vf_gr_t::iterator i = m_vf_gr->begin(); i != m_vf_gr->end(); i++ ) {
        if( i->second != NULL && !i->second->empty() ) {
          m_parent = i->first;
          break;
        }
      }
    }
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
    CFont oldFont = dc.SelectFont(m_capFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    rect.top = GetTitleHeight() + DRA::SCALEY(26);
    rect.bottom = rect.top + DRA::SCALEY(18/*HEADER*/);

    dc.DrawLine(rect.left, rect.top, rect.right, rect.top);
    dc.DrawLine(rect.left, rect.bottom, rect.right, rect.bottom);
    dc.DrawLine(rect.left, rect.bottom + DRA::SCALEY(21), 
      rect.right, rect.bottom + DRA::SCALEY(21));

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PROD_CAP3), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PROD_CAP2), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PROD_CAP1), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = 0;
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_PROD_CAP0), -1,
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

    PRODUCT_CONF *ptr = m_cur_pr[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(ptr->locked ? m_italicFont : m_baseFont);
    CBrush hbrCur = ptr->bgcolor ? CreateSolidBrush(ptr->bgcolor) : NULL;
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      if( ptr->locked || ptr->color != 0 )
        dc.SetTextColor(ptr->locked ? OMOBUS_ALLERTCOLOR : ptr->color);
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
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->price != NULL ) {    
      dc.SelectFont(m_boldFont);
      dc.DrawText(ftows(ptr->price->price, m_hterm->cur_prec).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(ptr->locked ? m_italicFont : m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->def_pack != NULL ) {
      dc.DrawText(ptr->def_pack->descr, -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_CENTER|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
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
    lmi->itemHeight = DRA::SCALEY(35);
    return 1;
  }

  LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_cur_pr.size() ) {
      CMenuHandle ctxmenu1 = m_ctxmenu.GetSubMenu(1);
      PRODUCT_CONF *pr = m_cur_pr[sel];
      ctxmenu1.EnableMenuItem(ID_PROD_SH, MF_BYCOMMAND | 
        ((pr->tsh.amount_c > 0.0 || pr->tsh.amount_r > 0.0) ? MF_ENABLED : MF_GRAYED));
      ctxmenu1.TrackPopupMenu(TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN, 
        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), m_hWnd);
    }
    return 0;
  }

  LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    KillTimer(1);
    if( wParam == 1 )
      _Reload();
		return 0;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Add(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnFindItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    LPNMLVFINDITEM fi = (LPNMLVFINDITEM)pnmh;
    if( fi->lvfi.flags & LVFI_STRING ) {
      size_t size = m_cur_pr.size(), i = fi->iStart;
      for( ; i < size; i++ ) {
        PRODUCT_CONF *ptr = m_cur_pr[i];
        if( _wcsnicmp(ptr->descr, fi->lvfi.psz, wcslen(fi->lvfi.psz)) == 0 )
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
    _Add(m_list.GetSelectedIndex());
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    KillTimer(1);
    EndDialog(wID);
    return 0;
  }

  LRESULT OnProdDet(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_cur_pr.size() )
      CProdDet(m_hterm, m_cur_pr[sel]).DoModal(m_hWnd);
    return 0;
  }

	LRESULT OnProdSH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_cur_pr.size() ) {
      sales_h_t prod_sh; 
      PRODUCT_CONF *pr = m_cur_pr[sel];
      std::for_each(m_sh->begin(), m_sh->end(), prod_sales_h(pr->prod_id, &prod_sh));
      CSaHDetDlg(m_hterm, pr, &prod_sh).DoModal(m_hWnd);
    }
    return 0;
	}

  bool OnNavigate(HWND /*hWnd*/, UINT /*nID*/) {
    KillTimer(1);
    _Hierarchy();
    return true;
  }

  const wchar_t *GetSubTitle() {
    size_t size = m_doc_t->size(), i = 0;
    cur_t amount = 0.0;
    for( ; i < size; i++ )
      amount += row_amount(&(*m_doc_t)[i]);
    m_amount = amount == 0.0 ? L"" : ftows(amount, m_hterm->cur_prec, m_hterm->thousand_sep).c_str();
    return m_amount.GetBuffer();
  }

protected:
  void _Reload() {
    _Reload(_CodeFF(), _NameFF());
  }

  void _Reload(slist_t &code_ff, slist_t &name_ff) {
    CWaitCursor _wc;
    vf_gr_t::iterator vf_it;
    size_t size, i;
    PRODUCT_CONF *ptr;
    m_cur_pr.clear();
    m_list.SetItemCount(m_cur_pr.size());
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
          if( !( name_ff.empty() || IsEqual(ptr->descr, name_ff)) ) {
            continue;
          }
          m_cur_pr.push_back(ptr);
        }
      } else {
        size = (*m_pr).size();
        for( i = 0; i < size; i++ ) {
          if( (ptr = (*m_pr)[i]) == NULL || ptr->ftype ) {
            continue;
          }
          if( !( code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
            continue;
          }
          if( !( name_ff.empty() || IsEqual(ptr->descr, name_ff)) ) {
            continue;
          }
          if( m_parent == &m_vf[0] ) { // ALL
            // any client
          } else if( m_parent == &m_vf[1] ) { // SALES_H
            if( !(!m_sh || ptr->tsh.amount_c != 0 || ptr->tsh.amount_r != 0) )
              continue;
          } else { // NATIVE FOLDER
            if( !COMPARE_ID(m_parent->prod_id, ptr->pid) )
              continue;
          }
          m_cur_pr.push_back(ptr);
        }
      }
    }
    m_list.SetItemCount(m_cur_pr.size());
    m_list.ShowWindow(m_cur_pr.empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
  }

  void _Tree(products_t &pr_tree, PRODUCT_CONF *parent, int level, slist_t &code_ff, slist_t &name_ff) {
    size_t size = m_pr->size(), i = 0;
    for( i = 0; i < size; i++ ) {
      PRODUCT_CONF *ptr = (*m_pr)[i];
      if( !ptr->ftype )
        continue;
      if( parent == NULL ) {
        if( !COMPARE_ID(OMOBUS_UNKNOWN_ID, ptr->pid) )
          continue;
      } else {
        if( !COMPARE_ID(parent->prod_id, ptr->pid) )
          continue;
      }
      _Calc(ptr, code_ff, name_ff);
      if( ptr->_i == 0 )
        continue;
      ptr->_l = level;
      pr_tree.push_back(ptr);
      _Tree(pr_tree, ptr, level + 1, code_ff, name_ff);
    }    
  }

  void _Calc(PRODUCT_CONF *a, slist_t &code_ff, slist_t &name_ff) {
    vf_gr_t::iterator vf_it;
    size_t size, i;
    PRODUCT_CONF *ptr;

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
        if( !(name_ff.empty() || IsEqual(ptr->descr, name_ff)) ) {
          continue;
        }
        a->_i++;
      }
    } else {
      size = (*m_pr).size();
      for( size_t i = 0; i < size; i++ ) {
        if( (ptr = (*m_pr)[i]) == NULL || ptr->ftype ) {
          continue;
        }
        if( !(code_ff.empty() || IsEqual(ptr->code, code_ff)) ) {
          continue;
        }
        if( !(name_ff.empty() || IsEqual(ptr->descr, name_ff)) ) {
          continue;
        }
        if( a == &m_vf[0] ) { // ALL
          if( !ptr->ftype )
            a->_i++;
        } else if( a == &m_vf[1] ) { // SALES_H
          if( !m_sh || ptr->tsh.amount_c != 0 || ptr->tsh.amount_r != 0 )
            a->_i++;
        } else { // NATIVE FOLDER
          if( COMPARE_ID(a->prod_id, ptr->pid) ) {
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
    return parce_slist(ff, sl);
  }

  slist_t _NameFF() {
    slist_t sl; CString ff; 
    m_name_ff.GetWindowText(ff);
    return parce_slist(ff, sl);
  }

  void _Add(size_t sel) {
    if( sel >= m_cur_pr.size() ) {
      return;
    }
    PRODUCT_CONF *prod = m_cur_pr[sel];
    // Определяем наличие упаковок. Если список допустимых упаковок пуст,
    // подбор продукта или изменение его количесва невозможно.
    if( prod->packs == NULL || prod->packs->empty() ) {
      MessageBoxEx(m_hWnd, IDS_STR12, IDS_WARN, MB_ICONSTOP);
      return;
    }
    // Ищем, добавлен ли уже продукт, если добавлен, то изменяем
    // уже добавленное количество
    size_t size = (*m_doc_t).size(), i = 0;
    for( ; i < size; i++ ) {
      if( COMPARE_ID((*m_doc_t)[i].pr_conf->prod_id, prod->prod_id) ) {
        CQtyDlg(m_hterm, &(*m_doc_t)[i]).DoModal(m_hWnd);
        return;
      }
    }
    // Проверяем разрешение добавлять продукты с неопределенной ценой
    if( !(m_hterm->select_unknown_price_produts || prod->price != NULL) ) {
      MessageBoxEx(m_hWnd, IDS_STR1, IDS_WARN, MB_ICONSTOP);
      return;
    }
    // Проверяем разрешение добавлять продукты, отсутствующие на складе
    if( !(m_hterm->select_unavailable_products || prod->wareh_stock > 0) ) {
      MessageBoxEx(m_hWnd, IDS_STR2, IDS_WARN, MB_ICONSTOP);
      return;
    }
    // Заблокированные продукты запрещено добавлять
    if( prod->locked ) {
      MessageBoxEx(m_hWnd, IDS_STR6, IDS_WARN, MB_ICONSTOP);
      return;
    }
    // У продукта должна быть определена упаковка подбора. Если её нет,
    // продукт добавлять запрещено.
    if( prod->doc_pack == NULL ) {
      MessageBoxEx(m_hWnd, IDS_STR11, IDS_WARN, MB_ICONSTOP);
      return;
    }
    // Создаем новую запись.
    T_ORDER doc_t; memset(&doc_t, 0, sizeof(doc_t));
    doc_t.pr_conf = prod;
    doc_t.pack = prod->doc_pack;
    if( CQtyDlg(m_hterm, &doc_t).DoModal(m_hWnd) == ID_ACTION )
      (*m_doc_t).push_back(doc_t);
  }

  void _Hierarchy() {
    CWaitCursor _wc;
    products_t pr_tree;
    slist_t code_ff = _CodeFF(), name_ff = _NameFF();
    vf_gr_t::iterator vf_it;

    _Calc(&m_vf[0], code_ff, name_ff);
    pr_tree.push_back(&m_vf[0]);
    if( m_sh && !m_sh->empty() ) {
      _Calc(&m_vf[1], code_ff, name_ff);
      pr_tree.push_back(&m_vf[1]);
    }
    for( vf_it = m_vf_gr->begin(); vf_it != m_vf_gr->end(); vf_it++ ) {
      if( vf_it->second != NULL && !vf_it->second->empty() ) {
        _Calc(vf_it->first, code_ff, name_ff);
        pr_tree.push_back(vf_it->first);
      }
    }
    _Tree(pr_tree, NULL, 0, code_ff, name_ff);
    _wc.Restore();
    if( !pr_tree.empty() ) {
      CProductTreeFrame dlg(m_hterm, &pr_tree, m_parent);
      if( dlg.DoModal(m_hWnd) == ID_ACTION ) {
        m_parent = dlg.GetSelElem();
        _Reload(code_ff, name_ff);
        _InitVfSelector();
      }
    }
  }

  void _LockMainMenuItems() {
    UIEnable(ID_PRODUCTS_ADD, m_list.GetSelectedIndex() != -1);
		UIUpdateToolBar();
  }

  void _InitVfSelector() {
    m_vfctrl.SetTitle(m_parent != NULL ? m_parent->descr : L"        ");
  }
};

class COrderFrame : 
	public CStdDialogResizeImpl<COrderFrame>,
	public CUpdateUI<COrderFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  H_ORDER *m_doc_h;
  T_ORDER_V *m_doc_t;
  simple_t *m_ct, *m_mf;
  sales_h_t *m_sh;
  products_t *m_pr;
  vf_gr_t *m_vf_gr;
  CListViewCtrl m_list;
  CHyperButtonTempl<COrderFrame> m_detctrl, m_addctrl;
  CLabel m_totalctrl;
  CMenu m_ctxmenu;
  CFont m_capFont, m_baseFont, m_emptyFont, m_qtyFont;
  CString m_total;

public:
  COrderFrame(HTERMINAL *hterm, 
    H_ORDER *doc_h, 
    T_ORDER_V *doc_t, 
    simple_t *ct, 
    sales_h_t *sh, 
    products_t *pr,
    vf_gr_t *vf_gr,
    simple_t *mf) : 
        m_hterm(hterm), 
        m_doc_h(doc_h), 
        m_doc_t(doc_t), 
        m_ct(ct), 
        m_sh(sh), 
        m_pr(pr), 
        m_vf_gr(vf_gr),
        m_mf(mf) {
    m_capFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_qtyFont = CreateBaseFont(DRA::SCALEY(11), FW_BOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, FALSE);
    m_emptyFont = CreateBaseFont(DRA::SCALEY(11), FW_NORMAL, TRUE);
  }

	enum { IDD = IDD_ORDER };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(COrderFrame)
    DLGRESIZE_CONTROL(IDC_LIST, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(COrderFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(COrderFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER(ID_PROD_DET, OnProdDet)
		COMMAND_ID_HANDLER(ID_PROD_SH, OnProdSH)
		COMMAND_ID_HANDLER(ID_PROD_QTY, OnProdQty)
		COMMAND_ID_HANDLER(ID_PROD_DEL, OnProdDel)
		COMMAND_ID_HANDLER(ID_PROD_PACK_DEF, OnProdPack)
		COMMAND_RANGE_HANDLER(_DEF_PACK_MENUITEM_BEGIN, _DEF_PACK_MENUITEM_END, OnProdPack)
		CHAIN_MSG_MAP(CUpdateUI<COrderFrame>)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<COrderFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDW_MENU_BAR, SHCMBF_HIDESIPBUTTON));
    UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(IDC_LIST);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_ctxmenu.LoadMenu(IDR_POPUP_MENU);
    m_detctrl.SubclassWindow(GetDlgItem(IDC_DET));
    m_detctrl.SetParent(this, IDC_DET);
    m_addctrl.SubclassWindow(GetDlgItem(IDC_ADD));
    m_addctrl.SetParent(this, IDC_ADD);
    m_totalctrl.SubclassWindow(GetDlgItem(IDC_TOTAL));
    m_totalctrl.SetFontBold(TRUE);
    _Total();
    _Reload();
    _LockMainMenuItems();
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
    dc.DrawLine(rect.left, rect.bottom + DRA::SCALEY(21), 
      rect.right, rect.bottom + DRA::SCALEY(21));

    rect.right -= scrWidth;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP2), -1, 
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP1), -1,
      rect.left + DRA::SCALEX(4), rect.top, 
      rect.right - DRA::SCALEX(4), rect.bottom,
      DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(60);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(LoadStringEx(IDS_CAP6), -1,
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

    T_ORDER *ptr = &(*m_doc_t)[lpdis->itemID];
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hterm->hpBord);
    CFont oldFont = dc.SelectFont(ptr->qty > 0 ? m_baseFont : m_emptyFont);
    CBrush hbrCur = ptr->pr_conf->bgcolor ? CreateSolidBrush(ptr->pr_conf->bgcolor) : NULL;
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hterm->hbrHL);
    }
    else {
      if( ptr->pr_conf->color != 0 )
        dc.SetTextColor(ptr->pr_conf->color);
      dc.FillRect(&lpdis->rcItem, !hbrCur.IsNull() ? hbrCur.m_hBrush : 
        (lpdis->itemID%2==0?m_hterm->hbrN:m_hterm->hbrOdd));
    }

    rect.right -= scrWidth;
    rect.top += DRA::SCALEY(3);
    rect.bottom = rect.top + DRA::SCALEY(16);
    dc.DrawText(ptr->pr_conf->descr, wcslen(ptr->pr_conf->descr), 
      rect.left + DRA::SCALEX(2), rect.top, 
      rect.right - DRA::SCALEX(2), rect.bottom,
      DT_LEFT|DT_TOP|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

    rect.top = rect.bottom;
    rect.bottom = lpdis->rcItem.bottom;
    rect.left = rect.right - DRA::SCALEX(70);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->pr_conf->price != NULL && ptr->qty > 0 ) {    
      dc.DrawText(ftows(row_amount(ptr), m_hterm->cur_prec, m_hterm->thousand_sep).c_str(), -1, 
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(55);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    if( ptr->qty > 0 ) {
      dc.SelectFont(m_qtyFont);
      dc.DrawText(ftows(ptr->qty, m_hterm->qty_prec).c_str(), -1,
        rect.left + DRA::SCALEX(4), rect.top, 
        rect.right - DRA::SCALEX(4), rect.bottom,
        DT_RIGHT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE);
      dc.SelectFont(m_baseFont);
    }

    rect.right = rect.left;
    rect.left = rect.right - DRA::SCALEX(60);
    dc.DrawLine(rect.right, rect.top, rect.right, rect.bottom);
    dc.DrawText(ptr->pack->descr, wcslen(ptr->pack->descr), 
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

  LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel >= m_doc_t->size() ) {
      return 0;
    }
    T_ORDER *tpart = &(*m_doc_t)[sel];
    CMenuHandle ctxmenu0 = m_ctxmenu.GetSubMenu(0);
    CMenuHandle ctxmenu3 = ctxmenu0.GetSubMenu(2);
    while( ctxmenu3.DeleteMenu(2, MF_BYPOSITION) == TRUE );
    if( tpart->pr_conf->packs != NULL ) {
      for( size_t i = 0; i < tpart->pr_conf->packs->size(); i++ )
        ctxmenu3.InsertMenu(-1, MF_BYPOSITION|MF_STRING|
          (tpart->pack==&((*tpart->pr_conf->packs)[i])?MF_CHECKED:MF_UNCHECKED)|
          (m_hterm->allow_changing_pack==ALLOW_CHANGING_PACK__YES?MF_ENABLED:MF_GRAYED), 
          (UINT_PTR) _DEF_PACK_MENUITEM_BEGIN + i, 
          (*tpart->pr_conf->packs)[i].descr);
    }
    ctxmenu0.EnableMenuItem(ID_PROD_SH, MF_BYCOMMAND | 
      ((tpart->pr_conf->tsh.amount_c > 0.0 || 
        tpart->pr_conf->tsh.amount_r > 0.0) ? MF_ENABLED : MF_GRAYED));
    ctxmenu0.EnableMenuItem(ID_PROD_DEL, MF_BYCOMMAND | 
      (!tpart->predefined ? MF_ENABLED : MF_GRAYED));
    ctxmenu0.TrackPopupMenu(TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN, 
      GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), m_hWnd);

    return 0;
  }

  LRESULT OnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    _Count(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( m_doc_t->empty() || m_doc_h->qty_pos == 0 ||
        MessageBoxEx(m_hWnd, IDS_STR7, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES )
      EndDialog(wID);
		return 0;
	}

	LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Add();
		return 0;
	}

	LRESULT OnClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( m_doc_t->empty() || m_doc_h->amount == 0.0  ) {
      MessageBoxEx(m_hWnd, IDS_STR5, IDS_WARN, MB_ICONWARNING);
      return 0;
    }
    if( CConfirmFrame(m_hterm, m_doc_h).DoModal(m_hWnd) == ID_ACTION ) {
      if( CloseDoc(m_hterm, m_doc_h, m_doc_t) ) {
        MessageBoxEx(m_hWnd, IDS_STR8, IDS_INFORM, MB_ICONINFORMATION);
        EndDialog(ID_CLOSE);
      } else {
        MessageBoxEx(m_hWnd, IDS_STR9, IDS_ERROR, MB_ICONSTOP);
      }
    }
		return 0;
	}

	LRESULT OnProdDet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_doc_t->size() )
      CProdDet(m_hterm, ((*m_doc_t)[sel]).pr_conf).DoModal(m_hWnd);
		return 0;
	}

	LRESULT OnProdSH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_doc_t->size() ) {
      sales_h_t prod_sh; 
      PRODUCT_CONF *pr = (*m_doc_t)[sel].pr_conf;
      std::for_each(m_sh->begin(), m_sh->end(), prod_sales_h(pr->prod_id, &prod_sh));
      CSaHDetDlg(m_hterm, pr, &prod_sh).DoModal(m_hWnd);
    }
		return 0;
	}

	LRESULT OnProdDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_doc_t->size() ) {
      if( (*m_doc_t)[sel].predefined ) {
        MessageBoxEx(m_hWnd, IDS_STR4, IDS_WARN, MB_ICONSTOP);
        return 0;
      }
      CString msg; msg.Format(IDS_STR3, (*m_doc_t)[sel].pr_conf->descr);
      if( MessageBoxEx(m_hWnd, msg, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
        m_doc_t->erase(m_doc_t->begin() + sel);
        _Total();
        _Reload();
      }
    }
		return 0;
	}

	LRESULT OnProdQty(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Count(m_list.GetSelectedIndex());
		return 0;
	}

	LRESULT OnProdPack(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    size_t sel = m_list.GetSelectedIndex();
    if( sel < m_doc_t->size() ) {
      T_ORDER *tpart = &(*m_doc_t)[sel];
      PACK_CONF *new_pack = tpart->pr_conf->doc_pack; // По-умолчанию
      if( wID != ID_PROD_PACK_DEF ) {
        size_t pack_num = wID - _DEF_PACK_MENUITEM_BEGIN;
        if( tpart->pr_conf->packs != NULL && tpart->pr_conf->packs->size() > pack_num )
          new_pack = &((*tpart->pr_conf->packs)[pack_num]);
      }
      tpart->qty *= tpart->pack->pack/new_pack->pack;
      // Дробное количество в базовой упаковке разрешено только для
      // весовых продуктов, для невесовых продуктов количество в базовой
      // упаковке не должно быть дробным.
      tpart->qty = ftof(tpart->qty, 
          tpart->pr_conf->fractional||tpart->pr_conf->base_pack!=new_pack
            ?
              m_hterm->qty_prec
            :
              0);
      tpart->pack = new_pack;
      _Total();
    }
		return 0;
	}

  bool OnNavigate(HWND /*hWnd*/, UINT nID) {
    if( nID == IDC_DET ) {
      _Det();
    } else if( nID == IDC_ADD ) {
      _Add();
    }
    return true;
  }

protected:
  void _Reload() {
    m_list.SetItemCount(m_doc_t->size());
    m_list.ShowWindow(m_doc_t->empty() ? SW_HIDE : SW_NORMAL);
    _LockMainMenuItems();
  }

  void _Total() {
    size_t size = m_doc_t->size(), i = 0;
    m_doc_h->weight = 0.0; m_doc_h->volume = 0.0;
    m_doc_h->amount = 0.0; m_doc_h->qty_pos = 0;
    for( ; i < size; i++ ) {
      m_doc_h->qty_pos += (*m_doc_t)[i].qty>0.0?1:0;
      m_doc_h->amount += row_amount(&(*m_doc_t)[i]);
      m_doc_h->weight += row_weight(&(*m_doc_t)[i]);
      m_doc_h->volume += row_volume(&(*m_doc_t)[i]);
    }
    m_total.Format(L"%s (%i)", 
      ftows(m_doc_h->amount, m_hterm->cur_prec, m_hterm->thousand_sep).c_str(),
      m_doc_h->qty_pos);
    m_totalctrl.SetText(m_total);
  }

  void _Count(size_t i) {
    if( i >= m_doc_t->size() ) {
      return;
    }
    T_ORDER *tpart = &((*m_doc_t)[i]);
    // Определяем наличие упаковок. Если список допустимых упаковок пуст,
    // подбор продукта или изменение его количесва невозможно.
    if( tpart->pr_conf->packs == NULL || tpart->pr_conf->packs->empty() ) {
      MessageBoxEx(m_hWnd, IDS_STR12, IDS_WARN, MB_ICONSTOP);
      return;
    }
    CQtyDlg(m_hterm, tpart).DoModal(m_hWnd);
    _Total();
    _LockMainMenuItems();
  }

  void _Add() {
    CProductsFrame(m_hterm, m_pr, m_vf_gr, m_sh, m_doc_t).DoModal(m_hWnd);
    _Total();
    _Reload();
  }

  void _Det() {
    CDocDet(m_hterm, m_doc_h, m_ct).DoModal(m_hWnd);
  }

  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, !(m_doc_t->empty() || m_doc_h->amount == 0.0));
		UIUpdateToolBar();
  }
};

//Format: %id%;%descr%;
static 
bool __simple(void *cookie, int line, const wchar_t **argv, int argc) 
{
  SIMPLE_CONF cnf;
  
  if( line == 0 ) {
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

//Format: %action_id%;%descr%;%b_date%;%e_date%;%prod_id%;%pack_id%;%qty%;
static 
bool __actions(void *cookie, int ln, const wchar_t **argv, int argc) 
{
  ACTION_CONF cnf;
  actions_t *ptr = (actions_t *) cookie;

  if( ln == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 7 ) {
    return false;
  }

  memset(&cnf, 0, sizeof(cnf));
  COPY_ATTR__S(cnf.prod_id, argv[4]);
  COPY_ATTR__S(cnf.pack_id, argv[5]);
  ptr->push_back(cnf);

  return true;
}

// Format: %pid%;%prod_id%;%ftype%;%code%;%art%;%descr%;%manuf_id%;%shelf_life%;%brand_id%;%fractional%;%categ_id%;
static 
bool __product(void *cookie, int line, const wchar_t **argv, int argc) 
{
  if( line == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 11 ) {
    return false;
  }

  product_cookie *ptr = (product_cookie *)cookie;
  byte_t ftype = (byte_t)_wtoi(argv[2]);
  const wchar_t *prod_id = argv[1];
  const wchar_t *manuf_id = argv[6];
  PRICE_CONF *price = NULL;
  qty_t wareh_stock = 0;
  bool locked = false;
  black_list_t::iterator lock_it;
  prices_t::iterator price_it;
  wareh_stocks_t::iterator wst_it;

  if( ftype == 0 ) {
    if( (lock_it = std::find_if(ptr->black_list->begin(), ptr->black_list->end(), find_lock(prod_id)))
            != ptr->black_list->end() ) {
      if( lock_it->soft_lock ) {
        locked = true;
      } else {
        return true;
      }
    }
    
    price_it = ptr->prices->find(prod_id);
    if( !(ptr->show_unknown_price_produts || price_it != ptr->prices->end()) ) {
      return true;
    }
    if( price_it != ptr->prices->end() ) {
      price = &price_it->second;
    }
    
    wst_it = ptr->wst->find(prod_id);
    if( !(ptr->show_unavailable_products || wst_it != ptr->wst->end()) ) {
      return true;
    }
    if( wst_it != ptr->wst->end() ) {
      wareh_stock = wst_it->second;
    }
    if( !(ptr->show_unavailable_products || wareh_stock > 0) ) {
      return true;
    }
  }

  PRODUCT_CONF *cnf = new PRODUCT_CONF;
  if( cnf == NULL )
    return false;
  memset(cnf, 0, sizeof(PRODUCT_CONF));
  COPY_ATTR__S(cnf->pid, argv[0]);
  COPY_ATTR__S(cnf->prod_id, prod_id);
  cnf->ftype = ftype;
  COPY_ATTR__S(cnf->code, argv[3]);
  COPY_ATTR__S(cnf->art, argv[4]);
  COPY_ATTR__S(cnf->descr, argv[5]);
  COPY_ATTR__S(cnf->shelf_life, argv[7]);
  cnf->fractional = wcsistrue(argv[9]);
  cnf->price = price;
  cnf->wareh_stock = wareh_stock;
  cnf->locked = locked;

  if( ftype == 0 ) {
    // % возврата
    recl_percentage_t::iterator recl_p_it = ptr->recl_percentage->find(prod_id);
    cnf->recl_percentage = recl_p_it == ptr->recl_percentage->end() ? NULL :
        recl_p_it->second.c_str();
    // Акция
    actions_t::iterator a_it = std::find_if(ptr->actions->begin(), ptr->actions->end(), 
      find_action(prod_id));
    cnf->action = a_it != ptr->actions->end() ? &(*a_it) : NULL;
    // Производитель
    simple_t::iterator mf_it = std::find_if(ptr->mf->begin(), ptr->mf->end(), 
      find_simple(manuf_id));
    cnf->manuf = mf_it != ptr->mf->end() ? &(*mf_it) : NULL;
    // Подсчет итогов продаж и возвратов
    std::for_each(ptr->sh->begin(), ptr->sh->end(), 
      total_sales_h(prod_id, &cnf->tsh));
    // Определяем наличие упаковок.
    product_packs_t::iterator it = ptr->packs->find(prod_id);
    if( it == ptr->packs->end() || it->second.empty() ) {
      ATLTRACE(L"__product: У продукта нет упаковок (prod_id='%s')!\n", prod_id);
    } else {
      cnf->packs = &it->second;
      // Ищем базовую упаковку. Базовая упаковка обязательна, если её нет,
      // то все остальные упаковки игнорируем.
      packs_t::iterator pack_it = std::find_if(cnf->packs->begin(),
        cnf->packs->end(), find_base_pack());
      if( pack_it != cnf->packs->end() ) {
        cnf->base_pack = &(*pack_it);
        if( cnf->price != NULL && !COMPARE_ID(cnf->price->pack_id, OMOBUS_UNKNOWN_ID) ) {
          pack_it = std::find_if(cnf->packs->begin(), cnf->packs->end(), 
            find_pack(cnf->price->pack_id));
          cnf->doc_pack = cnf->def_pack = 
            pack_it == cnf->packs->end() ? NULL : &(*pack_it);
          if( cnf->action != NULL ) {
            pack_it = std::find_if(cnf->packs->begin(),
              cnf->packs->end(), find_pack(cnf->action->pack_id));
            if( pack_it != cnf->packs->end() ) {
              cnf->doc_pack = &(*pack_it);
            }
          }
        }
      }
    }
  }
  
  ptr->prods->push_back(cnf);

  return true;
}

// Format: %account_id%;%prod_id%;%qty%;
static 
bool __preorder(void *cookie, int line, const wchar_t **argv, int argc) 
{
  p_order_cookie *ptr;
  products_t::iterator prod_it;
  T_ORDER_V::iterator order_it;
  T_ORDER cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (p_order_cookie *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( !(COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[0], ptr->account_id)) ) {
    return true;
  }
  if( (prod_it = std::find_if(ptr->prod->begin(), ptr->prod->end(), find_product(argv[1]))) 
          == ptr->prod->end() || 
      (*prod_it)->doc_pack == NULL || 
      (*prod_it)->locked ) {
    return true;
  }
  if( (order_it = std::find_if(ptr->t->begin(), ptr->t->end(), find_order_product(argv[1]))) 
          == ptr->t->end() ) {
    memset(&cnf, 0, sizeof(T_ORDER));
    cnf.pr_conf = *prod_it;
    cnf.pack = cnf.pr_conf->doc_pack;
    cnf.qty = wcstof(argv[2]);
    if( cnf.qty > 0 ) {
      cnf.qty *= (*prod_it)->base_pack->pack/(*prod_it)->doc_pack->pack;
    }
    cnf.predefined = true;
    ptr->t->push_back(cnf);
  } else {
    order_it->qty = wcstof(argv[2]);
    if( order_it->qty > 0 ) {
      order_it->qty *= (*prod_it)->base_pack->pack/(*prod_it)->doc_pack->pack;
    }
  }

  return true;
}

// Format: %account_id%;%prod_id%;%color%;%bgcolor%;
static 
bool __highlights(void *cookie, int line, const wchar_t **argv, int argc) 
{
  highlight_cookie *ptr;
  products_t::iterator prod_it;

  if( line == 0 || (ptr = (highlight_cookie *)cookie) == NULL || argc < 4 ) {
    return line == 0 ? true : false;
  }
  if( !(COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[0], ptr->account_id)) ) {
    return true;
  }
  if( (prod_it = std::find_if(ptr->prod->begin(), ptr->prod->end(), find_product(argv[1]))) 
          == ptr->prod->end() ) {
    return true;
  }
  (*prod_it)->color = _wtoi(argv[2]);
  (*prod_it)->bgcolor = _wtoi(argv[3]);

  return true;
}

static 
bool __prices(void *cookie, int line, const wchar_t **argv, int argc)
{
  price_cookie *ptr;
  const wchar_t *prod_id;
  PRICE_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (price_cookie *)cookie) == NULL || argc < 5 ) {
    return false;
  }
  if( ptr->price_id == NULL || ptr->prices == NULL || ptr->price_id[0] == L'\0' ) {
    return false;
  }
  if( COMPARE_ID(ptr->price_id, argv[0]) 
        &&
      ((argv[3][0] == L'\0') || (ptr->price_date != NULL && COMPARE_ID(ptr->price_date, argv[3])))
    ) {
    prod_id = argv[1];
    COPY_ATTR__S(cnf.pack_id, argv[2]);
    cnf.price = wcstof(argv[4]);
    if( (*ptr->prices).find(argv[1]) == (*ptr->prices).end() ) {
      (*ptr->prices).insert(prices_t::value_type(prod_id, cnf));
    } else {
      (*ptr->prices)[prod_id] = cnf;
    }
  }
  return true;
}

static 
bool __wareh_stocks(void *cookie, int line, const wchar_t **argv, int argc)
{
  if( line == 0 )
    return true;
  if( cookie == NULL || argc < 3 )
    return false;
  wareh_stocks_cookie *ptr = (wareh_stocks_cookie *)cookie;
  if( ptr->wareh_id == NULL || ptr->wst == NULL )
    return false;
  if( COMPARE_ID(ptr->wareh_id, argv[0]) ) {
    (*ptr->wst).insert(wareh_stocks_t::value_type(argv[1], 
      _wtoi(argv[2])));
  }
  return true;
}

// Format: %account_id%;%prod_id%;%descr%;%amount_c%;%pack_c_id%;%qty_c%;%amount_r%;%pack_r_id%;%qty_r%;%color%;%bgcolor%;
static 
bool __sales_h(void *cookie, int line, const wchar_t **argv, int argc)
{
  sales_h_cookie *ptr = (sales_h_cookie *)cookie;

  if( line == 0 ) {
    return true;
  }  
  if( cookie == NULL || argc < 11 ) {
    return false;
  }
  if( !COMPARE_ID(ptr->account_id, argv[0]) ) {
    return true;
  }

  SALES_H_CONF *cnf = new SALES_H_CONF;
  if( cnf == NULL )
    return false;
  memset(cnf, 0, sizeof(SALES_H_CONF));
  COPY_ATTR__S(cnf->prod_id, argv[1]);
  date2tm(argv[2], &cnf->date);  
  cnf->amount_c = wcstof(argv[3]);
  cnf->qty_c = wcstof(argv[5]);
  cnf->amount_r = wcstof(argv[6]);
  cnf->qty_r = wcstof(argv[8]);
  cnf->color = _wtoi(argv[9]);
  cnf->bgcolor = _wtoi(argv[10]);

  ptr->sales_h->push_back(cnf);

  return true;
}

// Format: %pack_id%;%prod_id%;%descr%;%pack%;%weight%;%volume%;
static 
bool __pack(void *cookie, int line, const wchar_t **argv, int argc)
{
  if( line == 0 ) {
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

// Format: %account_id%;%prod_id%;%soft_lock%;
static 
bool __black_list(void *cookie, int line, const wchar_t **argv, int argc)
{
  black_list_cookie_t *ptr;
  LOCK_CONF cnf;
  black_list_t::iterator it;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (black_list_cookie_t *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[0], ptr->account_id) ) {
    if( (it = std::find_if(ptr->l->begin(), ptr->l->end(), find_lock(argv[1]))) == ptr->l->end() ) {
      memset(&cnf, 0, sizeof(cnf));
      COPY_ATTR__S(cnf.prod_id, argv[1]);
      cnf.soft_lock = wcsistrue(argv[2]);
      ptr->l->push_back(cnf);
    } else {
      it->soft_lock = wcsistrue(argv[2]);
    }
  }

  return true;
}

// Format: %warn_id%;%account_id%;%caption%;%body%;
static 
bool __warn(void *cookie, int line, const wchar_t **argv, int argc)
{
  warn_cookie *ptr = (warn_cookie *)cookie;
  WARN_CONF conf;
  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || argc < 4 ) {
    return false;
  }
  if( !(COMPARE_ID(argv[1], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[1], ptr->account_id)) ) {
    return true;
  }
  conf.caption = argv[2];
  conf.body = argv[3];
  ptr->warn->push_back(conf);
  return true;
}

// Format: %account_id%;%prod_id%;%descr%;
static 
bool __recl_percentage(void *cookie, int line, const wchar_t **argv, int argc)
{
  recl_percentage_cookie_t *ptr;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (recl_percentage_cookie_t *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( COMPARE_ID(argv[0], ptr->account_id) ) {
    (*(ptr->l))[argv[1]] = argv[2];
  }

  return true;
}

// Format: %vf_id%;%descr%;
static 
bool __vf_names(void *cookie, int line, const wchar_t **argv, int argc)
{
  vf_gr_t *ptr;
  PRODUCT_CONF *cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (vf_gr_t *)cookie) == NULL || argc < 2 ) {
    return false;
  }
  if( (cnf = new PRODUCT_CONF()) == NULL ) {
    return false;
  }
  memset(cnf, 0, sizeof(PRODUCT_CONF));
  cnf->_vf = 1;
  COPY_ATTR__S(cnf->prod_id, argv[0]);
  COPY_ATTR__S(cnf->descr, argv[1]);
  ptr->insert(vf_gr_t::value_type(cnf, NULL));
  
  return true;
}

// Format: %vf_id%;%account_id%;%prod_id%;
static 
bool __vf_products(void *cookie, int line, const wchar_t **argv, int argc)
{
  vf_products_cookie_t *ptr;
  vf_gr_t::iterator vf_it;
  products_t::iterator prod_it;
  products_t *l;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (vf_products_cookie_t *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( (vf_it = std::find_if(ptr->vf_gr->begin(), ptr->vf_gr->end(), find_vf_gr(argv[0]))) 
              != ptr->vf_gr->end() 
          &&
      (COMPARE_ID(argv[1], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[1], ptr->account_id))
    ) {
    if( (l = vf_it->second) == NULL ) {
      if( (l = new products_t()) == NULL ) {
        return false;
      } else {
        vf_it->second = l;
      }
    }
    if( (prod_it = std::find_if(ptr->prods->begin(), ptr->prods->end(), find_product(argv[2])))
            != ptr->prods->end() && !(*prod_it)->ftype ) {
      l->push_back(*prod_it);
    }
  }
  
  return true;
}

// Format: %account_id%;%prod_id%;%pack_id%;%min_qty%;%max_qty%;
static 
bool __restrictions(void *cookie, int line, const wchar_t **argv, int argc) 
{
  highlight_cookie *ptr;
  products_t::iterator prod_it;
  packs_t::iterator pack_it;

  if( line == 0 || (ptr = (highlight_cookie *)cookie) == NULL || argc < 5 ) {
    return line == 0 ? true : false;
  }
  if( !(COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[0], ptr->account_id)) ) {
    return true;
  }
  if( (prod_it = std::find_if(ptr->prod->begin(), ptr->prod->end(), find_product(argv[1]))) 
          == ptr->prod->end() ) {
    return true;
  }
  if( (*prod_it)->packs == NULL ) {
    return true;
  }
  if( (pack_it = std::find_if((*prod_it)->packs->begin(), (*prod_it)->packs->end(), find_pack(argv[2])))
          == (*prod_it)->packs->end() ) {
    return true;
  }

  (*prod_it)->min_pack = (*prod_it)->max_pack = &(*pack_it);
  (*prod_it)->min_qty = _wtoi(argv[3]);
  (*prod_it)->max_qty = _wtoi(argv[4]);

  return true;
}

/*
static 
bool __preorder(void *cookie, int line, const wchar_t **argv, int argc) 
{
  p_order_cookie *ptr;
  products_t::iterator prod_it;
  T_ORDER_V::iterator order_it;
  T_ORDER cnf;

  if( line == 0 ) {
    return true;
  }
  if( (ptr = (p_order_cookie *)cookie) == NULL || argc < 3 ) {
    return false;
  }
  if( !(COMPARE_ID(argv[0], OMOBUS_UNKNOWN_ID) || COMPARE_ID(argv[0], ptr->account_id)) ) {
    return true;
  }
  if( (prod_it = std::find_if(ptr->prod->begin(), ptr->prod->end(), find_product(argv[1]))) 
          == ptr->prod->end() || 
      (*prod_it)->doc_pack == NULL || 
      (*prod_it)->locked ) {
    return true;
  }
  if( (order_it = std::find_if(ptr->t->begin(), ptr->t->end(), find_order_product(argv[1]))) 
          == ptr->t->end() ) {
    memset(&cnf, 0, sizeof(T_ORDER));
    cnf.pr_conf = *prod_it;
    cnf.pack = cnf.pr_conf->doc_pack;
    cnf.qty = wcstof(argv[2]);
    if( cnf.qty > 0 ) {
      cnf.qty *= (*prod_it)->base_pack->pack/(*prod_it)->doc_pack->pack;
    }
    cnf.predefined = true;
    ptr->t->push_back(cnf);
  } else {
    order_it->qty = wcstof(argv[2]);
    if( order_it->qty > 0 ) {
      order_it->qty *= (*prod_it)->base_pack->pack/(*prod_it)->doc_pack->pack;
    }
  }

  return true;
}


*/

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
slist_t &parce_slist(const wchar_t *s, slist_t &sl) {
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
std::wstring fmtDate(HTERMINAL *h, const wchar_t *d) {
  struct tm t;
  return wsftime(date2tm(d, &t), h->date_fmt);
}

static
bool WriteDocPk(HTERMINAL *h, H_ORDER *doc_h, T_ORDER_V *doc_t, uint64_t &doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_DOC_ORDER_H, true), 
    xml_t = wsfromRes(IDS_X_DOC_ORDER_T, true);
  if( xml.empty() || xml_t.empty() )
    return false;

  std::wstring rows, cur_row = xml_t;
  size_t size = doc_t->size(), i = 0, pos_id = 0;
  for( ; i < size; i++ ) {
    T_ORDER *tpart = &(*doc_t)[i];
    if( tpart->qty <= 0.0 )
      continue;
    wsrepl(cur_row, L"%pos_id%", itows(pos_id).c_str());
    wsrepl(cur_row, L"%prod_id%", fix_xml(tpart->pr_conf->prod_id).c_str());
    wsrepl(cur_row, L"%pack_id%", fix_xml(tpart->pack->pack_id).c_str());
    wsrepl(cur_row, L"%pack%", ftows(tpart->pack->pack, h->qty_prec).c_str());
    wsrepl(cur_row, L"%qty%", ftows(tpart->qty, h->qty_prec).c_str());
    wsrepl(cur_row, L"%unit_price%", ftows(row_unit_price(tpart), h->cur_prec).c_str());
    wsrepl(cur_row, L"%amount%", ftows(row_amount(tpart), h->cur_prec).c_str());
    wsrepl(cur_row, L"%weight%", ftows(row_weight(tpart), OMOBUS_WEIGHT_PREC).c_str());
    wsrepl(cur_row, L"%volume%", ftows(row_volume(tpart), OMOBUS_VOLUME_PREC).c_str());
    rows += cur_row; cur_row = xml_t;
    pos_id++;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%doc_note%", fix_xml(doc_h->note).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", doc_h->w_cookie);
  wsrepl(xml, L"%a_cookie%", doc_h->a_cookie);
  wsrepl(xml, L"%activity_type_id%", fix_xml(doc_h->activity_type_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(doc_h->account_id).c_str());
  wsrepl(xml, L"%created_dt%", wsftime(doc_h->ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%wareh_id%", doc_h->wareh_conf == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(doc_h->wareh_conf->id).c_str());
  wsrepl(xml, L"%order_type_id%", doc_h->type_conf == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(doc_h->type_conf->id).c_str());
  wsrepl(xml, L"%std_price_id%", doc_h->spn_conf == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(doc_h->spn_conf->id).c_str());
  wsrepl(xml, L"%group_price_id%", doc_h->gr_price_id == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(doc_h->gr_price_id).c_str());
  wsrepl(xml, L"%pay_delay%", itows(doc_h->pay_delay).c_str());
  wsrepl(xml, L"%del_date%", fix_xml(doc_h->del_date).c_str());
  wsrepl(xml, L"%amount%", ftows(doc_h->amount, h->cur_prec).c_str());
  wsrepl(xml, L"%qty_pos%", itows(doc_h->qty_pos).c_str());
  wsrepl(xml, L"%weight%", ftows(doc_h->weight, OMOBUS_WEIGHT_PREC).c_str());
  wsrepl(xml, L"%volume%", ftows(doc_h->volume, OMOBUS_VOLUME_PREC).c_str());
  wsrepl(xml, L"%created_gps_dt%", wsftime(doc_h->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(doc_h->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(doc_h->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%rows%", rows.c_str());

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_ORDER, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, H_ORDER *doc_h, uint64_t &doc_id, 
  omobus_location_t *posClo, time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_USER_DOCUMENT, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%doc_type%", OMOBUS_ORDER);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(doc_h->activity_type_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(doc_h->account_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", doc_h->w_cookie);
  wsrepl(xml, L"%a_cookie%", doc_h->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool _doc_h_journal(HTERMINAL *h, H_ORDER *doc_h, uint64_t &doc_id, 
  time_t ttClo) {
  std::wstring txt = wsfromRes(IDS_J_DOC_ORDER_H, true), templ = txt;
  if( txt.empty() )
    return false;

  wsrepl(txt, L"%date%", wsftime(ttClo).c_str());
  wsrepl(txt, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(txt, L"%account_id%", doc_h->account_id);
  wsrepl(txt, L"%amount%", ftows(doc_h->amount, h->cur_prec).c_str());
  wsrepl(txt, L"%qty_pos%", itows(doc_h->qty_pos).c_str());
  
  return WriteOmobusJournal(OMOBUS_ORDER, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
bool _doc_t_journal(HTERMINAL *h, H_ORDER *doc_h, T_ORDER_V *doc_t, 
  uint64_t &doc_id) {
  std::wstring txt = wsfromRes(IDS_J_DOC_ORDER_T, true), templ = txt, rows;
  if( txt.empty() )
    return false;

  size_t size = doc_t->size(), i = 0;
  for( ; i < size; i++ ) {
    T_ORDER *tpart = &(*doc_t)[i];
    if( tpart->qty <= 0.0 )
      continue;
    wsrepl(txt, L"%doc_id%", itows(doc_id).c_str());
    wsrepl(txt, L"%prod%", tpart->pr_conf->descr);
    wsrepl(txt, L"%pack%", tpart->pack->descr);
    wsrepl(txt, L"%qty%", ftows(tpart->qty, h->qty_prec).c_str());
    wsrepl(txt, L"%amount%", ftows(row_amount(tpart), h->cur_prec).c_str());
    rows += txt; txt = templ;
  }
  
  return WriteOmobusJournal(OMOBUS_ORDER_T, templ.c_str(), rows.c_str()) 
    == OMOBUS_OK;
}

static
int _wcsstate(const wchar_t *str) {
  if( wcsicmp(str, _T("yes")) == 0 || wcsicmp(str, _T("true")) == 0 ||
      wcsicmp(str, _T("+")) == 0 || wcsicmp(str, _T("enabled")) == 0 )
    return 1;
  if( wcsicmp(str, _T("auto")) == 0 )
    return 2;

  return 0;
}

static
bool CloseDoc(HTERMINAL *h, H_ORDER *doc_h, T_ORDER_V *doc_t)
{
  if( h->user_id == NULL )
    return false;

  CWaitCursor _wc;
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  omobus_location_t posClo; omobus_location(&posClo);
  if( !WriteDocPk(h, doc_h, doc_t, doc_id, &posClo, ttClo) ) {
    return false;
  }
  WriteActPk(h, doc_h, doc_id, &posClo, ttClo);
  _doc_h_journal(h, doc_h, doc_id, ttClo);
  _doc_t_journal(h, doc_h, doc_t, doc_id);
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
}

static 
void OnAction(HTERMINAL *h) 
{
  CWaitCursor _wc;
  simple_t spn, wareh, type, mf;
  black_list_t black_list;
  actions_t actions;
  products_t prod;
  prices_t prices;
  wareh_stocks_t wst;
  sales_h_t sales_h;
  product_packs_t prod_packs;
  warn_t warn;
  recl_percentage_t recl_p;
  H_ORDER doc_h;
  T_ORDER_V doc_t;
  recl_percentage_cookie_t recl_p_cookie;
  vf_gr_t vf_gr;
  vf_products_cookie_t vf_cookie;
  black_list_cookie_t bl_cookie;

  memset(&doc_h, 0, sizeof(H_ORDER));
  doc_h.ttCre = omobus_time();
  omobus_location(&doc_h.posCre);
  doc_h.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  doc_h.a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  doc_h.activity_type_id = h->get_conf(__INT_ACTIVITY_TYPE_ID, OMOBUS_UNKNOWN_ID);
  doc_h.account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  doc_h.gr_price_id = h->get_conf(__INT_ACCOUNT_GR_PRICE_ID, NULL);
  doc_h.pay_delay = _wtoi(h->get_conf(__INT_ACCOUNT_PAY_DELAY, L"0"));

  if( (doc_h.del_date = h->get_conf(__INT_DEL_DATE, NULL)) == NULL ) {
    MessageBoxEx(h->hParWnd, IDS_STR10, IDS_WARN, MB_ICONSTOP);
    return;
  }

  product_cookie ck_prod = {
    &prod, &prices, &wst, &mf, &black_list, &sales_h, &prod_packs, &recl_p, &actions,
    h->show_unavailable_products, h->show_unknown_price_produts
  };

  p_order_cookie ck_pprod = {
    &doc_t, &prod, doc_h.account_id     
  };

  highlight_cookie ck_highlight = {
    &prod, doc_h.account_id     
  };

  wareh_stocks_cookie ck_wareh_st = {
    NULL, NULL
  };

  price_cookie ck_price = {
    NULL, NULL, &prices
  };

  sales_h_cookie ck_sales_h = {
    doc_h.account_id, &sales_h
  };

  warn_cookie ck_warn = {
    doc_h.account_id, &warn
  };

  bl_cookie.account_id = doc_h.account_id;
  bl_cookie.l = &black_list;
  recl_p_cookie.account_id = doc_h.account_id;
  recl_p_cookie.l = &recl_p;
  vf_cookie.account_id = doc_h.account_id;
  vf_cookie.prods = &prod;
  vf_cookie.vf_gr = &vf_gr;

  // (1) Показ уведомлений
  ReadOmobusManual(L"order_warnings", &ck_warn, __warn);
  ATLTRACE(L"order_warnings = %i\n", warn.size());
  _wc.Restore();
  for( size_t n = 0; n < warn.size(); n++ ) {
    WARN_CONF *nconf = &warn[n];
    if( CNotifyFrame(nconf->caption.c_str(), nconf->body.c_str()).DoModal(h->hParWnd) != ID_ACTION )
      return;
  }

  // (2) Выбор базовой цены
  _wc.Set();
  ReadOmobusManual(L"std_price_names", &spn, __simple);
  ATLTRACE(L"std_price_names = %i\n", spn.size());
  _wc.Restore();
  if( !spn.empty() ) {
    if( spn.size() > 1 ) {
      if( CPriceFrame(h, &spn, &doc_h.spn_conf).DoModal(h->hParWnd) != ID_ACTION )
        return;
    } else {
      doc_h.spn_conf = &spn[0];
    }
  }

  // (3) Выбор склада
  _wc.Set();
  ReadOmobusManual(L"warehouses", &wareh, __simple);
  ATLTRACE(L"warehouses = %i\n", wareh.size());
  _wc.Restore();
  if( !wareh.empty() ) {
    if( wareh.size() > 1 ) {
      if( CWarehFrame(h, &wareh, &doc_h.wareh_conf).DoModal(h->hParWnd) != ID_ACTION )
        return;
    } else {
      doc_h.wareh_conf = &wareh[0];
    }
  }

  // (4) Загрузка справочников
  _wc.Set();
  ReadOmobusManual(L"actions", &actions, __actions);
  ATLTRACE(L"actions = %i\n", actions.size());
  ReadOmobusManual(L"order_black_list", &bl_cookie, __black_list);
  ATLTRACE(L"black_list = %i\n", black_list.size());
  ReadOmobusManual(L"reclamation_percentage", &recl_p_cookie, __recl_percentage);
  ATLTRACE(L"reclamation_percentage = %i\n", recl_p.size());

  ck_wareh_st.wst = &wst;
  ck_wareh_st.wareh_id = doc_h.wareh_conf == NULL ? OMOBUS_UNKNOWN_ID : doc_h.wareh_conf->id;
  ReadOmobusManual(L"wareh_stocks", &ck_wareh_st, __wareh_stocks);
  ATLTRACE(L"wareh_stocks = %i\n", wst.size());

  if( doc_h.spn_conf != NULL ) {
    ck_price.price_id = doc_h.spn_conf->id;
    ck_price.price_date = doc_h.del_date;
    ReadOmobusManual(L"std_prices", &ck_price, __prices);
  }
  ATLTRACE(L"(1/std_prices) price_id=%s, prices=%i\n", ck_price.price_id, prices.size());
  if( doc_h.gr_price_id != NULL ) {
    ck_price.price_id = doc_h.gr_price_id;
    ck_price.price_date = doc_h.del_date;
    ReadOmobusManual(L"group_prices", &ck_price, __prices);
  }
  ATLTRACE(L"(2/group_prices) group_price_id=%s, prices=%i\n", ck_price.price_id, prices.size());
  if( doc_h.account_id != NULL ) {
    ck_price.price_id = ck_pprod.account_id;
    ck_price.price_date = doc_h.del_date;
    ReadOmobusManual(L"account_prices", &ck_price, __prices);
  }
  ATLTRACE(L"(3/account_prices) account_id=%s, prices=%i\n", ck_price.price_id, prices.size());

  ReadOmobusManual(L"manufacturers", &mf, __simple);
  ATLTRACE(L"manufacturers = %i\n", mf.size());
  ReadOmobusManual(L"sales_history", &ck_sales_h, __sales_h);
  ATLTRACE(L"sales_history = %i/%i (size=%i Kb)\n", sizeof(SALES_H_CONF), sales_h.size(), sizeof(SALES_H_CONF)*sales_h.size()/1024);
  ReadOmobusManual(L"packs", &prod_packs, __pack);
  ATLTRACE(L"packs = %i\n", prod_packs.size());
  ReadOmobusManual(L"products", &ck_prod, __product);
  ATLTRACE(L"products = %i/%i (size=%i Kb)\n", sizeof(PRODUCT_CONF), prod.size(), sizeof(PRODUCT_CONF)*prod.size()/1024);
  ReadOmobusManual(L"order_products", &ck_pprod, __preorder);
  ATLTRACE(L"order_products = %i\n", doc_t.size());
  ReadOmobusManual(L"order_vf_names", &vf_gr, __vf_names);
  ReadOmobusManual(L"order_vf_products", &vf_cookie, __vf_products);
  ATLTRACE(L"order_vf_names = %i\n", vf_gr.size());
  ReadOmobusManual(L"order_types", &type, __simple);
  ATLTRACE(L"order_types = %i\n", type.size());
  ReadOmobusManual(L"order_highlights", &ck_highlight, __highlights);
  ReadOmobusManual(L"order_restrictions", &ck_highlight, __restrictions);
  _wc.Restore();

  if( !type.empty() ) {
    doc_h.type_conf = &type[0];
  }
  if( COrderFrame(h, &doc_h, &doc_t, &type, &sales_h, &prod, &vf_gr, &mf).
        DoModal(h->hParWnd) == ID_CLOSE ) {
    IncrementDocumentCounter(h);
  }

  man_cleanup<products_t>(prod);
  man_cleanup<sales_h_t>(sales_h);
  std::for_each(vf_gr.begin(), vf_gr.end(), vf_gr_cleanup());
  vf_gr.clear();
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
  
  if( ( h = new HTERMINAL) == NULL ) {
    return NULL;
  }
  memset(h, 0, sizeof(HTERMINAL));
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->hParWnd = hParWnd;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->cur_prec = _wtoi(get_conf(OMOBUS_CUR_PRECISION, OMOBUS_CUR_PRECISION_DEF));
  h->qty_prec = _wtoi(get_conf(OMOBUS_QTY_PRECISION, OMOBUS_QTY_PRECISION_DEF));
  h->show_unavailable_products = wcsistrue(get_conf(SHOW_UNAVAILABLE_PRODUCTS, SHOW_UNAVAILABLE_PRODUCTS_DEF));
  h->select_unavailable_products = wcsistrue(get_conf(SELECT_UNAVAILABLE_PRODUCTS, SELECT_UNAVAILABLE_PRODUCTS_DEF));
  h->show_unknown_price_produts = wcsistrue(get_conf(SHOW_UNKNOWN_PRICE_PRODUTS, SHOW_UNKNOWN_PRICE_PRODUTS_DEF));
  h->select_unknown_price_produts = wcsistrue(get_conf(SELECT_UNKNOWN_PRICE_PRODUTS, SELECT_UNKNOWN_PRICE_PRODUTS_DEF));
  h->pay_delay = wcsistrue(get_conf(PAY_DELAY, PAY_DELAY_DEF));
  h->allow_changing_pack = _wcsstate(get_conf(ALLOW_CHANGING_PACK, ALLOW_CHANGING_PACK_DEF));
  h->date_fmt = get_conf(OMOBUS_MUI_DATE, ISO_DATE_FMT);
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, wcsistrue(get_conf(__INT_SPLIT_ACTIVITY_SCREEN, NULL)) ?
      OMOBUS_SCREEN_ACTIVITY_TAB_1 : OMOBUS_SCREEN_ACTIVITY);
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
