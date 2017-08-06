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

#include <windows.h>
#include <DeviceResolutionAware.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>
#include "resource.h"
#include "../../version"

#define _ROW_HEIGHT         14
#define _TEXT_TOP_OFFSET    1
#define _TEXT_LEFT_OFFSET   2
#define _TEXT_RIGHT_OFFSET  4

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static TCHAR _szWindowClass[] = L"omobus-sales_plans-terminal-element";
static int _nHeight = 4*(_ROW_HEIGHT + _TEXT_TOP_OFFSET);

typedef struct _tagP_SALES {
  wchar_t account_id[OMOBUS_MAX_ID];
  int32_t amount_f, amount_r, amount, qty;
} P_SALES;

typedef std::vector<P_SALES> sales_plans_list;

typedef struct _tagORDER {
  wchar_t account_id[OMOBUS_MAX_ID];
  cur_t amount;
} ORDER;

typedef std::vector<ORDER> orders_list;

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  HWND hWnd, hParWnd;
  HFONT hFont, hFontB;
  HBRUSH hbrHL, hbrN;
  HPEN hDash;
  bool focus;
  wchar_t c_date[11];
  const wchar_t *thousand_sep;
  sales_plans_list *sales, *sales0, *sales1;
  orders_list *orders;
} HTERMINAL;

struct _p_total {
  bool m_valid; // Есть данные?
  const wchar_t *m_account_id;
  int32_t m_amount_f, m_amount_r, m_amount, m_qty;
  _p_total(const wchar_t *account_id) : 
    m_account_id(account_id), m_amount_f(0),
    m_amount_r(0), m_amount(0), m_qty(0) {
    m_valid = false;
  }
  void operator () (const P_SALES &cnf) {
    if( m_account_id != NULL && 
        !COMPARE_ID(m_account_id, OMOBUS_UNKNOWN_ID) && 
        !COMPARE_ID(m_account_id, cnf.account_id) )
      return;
    
    m_amount_f += cnf.amount_f;
    m_amount_r += cnf.amount_r;
    m_amount += cnf.amount;
    m_qty += cnf.qty;
    m_valid = true;
  }
};

struct _o_total {
  const wchar_t *m_account_id;
  cur_t m_amount;
  uint32_t m_qty;
  _o_total(const wchar_t *account_id) : 
    m_account_id(account_id), m_amount(0), m_qty(0) {
  }
  void operator () (const ORDER &cnf) {
    if( m_account_id != NULL && 
        !COMPARE_ID(m_account_id, OMOBUS_UNKNOWN_ID) && 
        !COMPARE_ID(m_account_id, cnf.account_id) )
      return;
    
    m_amount += cnf.amount;
    m_qty++;
  }
};

// Format: %account_id%;%amount_f%;%amount_r%;%amount%;%qty%;
static 
bool __my_sales(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *ptr = (HTERMINAL *) cookie;
  
  if( line == 0 ) {
    return true;
  }
  if( ptr == NULL || count < 5 ) {
    return false;
  }

  P_SALES p; memset(&p, 0, sizeof(p));
  COPY_ATTR__S(p.account_id, argv[0]);
  p.amount_f = _wtoi(argv[1]);
  p.amount_r = _wtoi(argv[2]);
  p.amount = _wtoi(argv[3]);
  p.qty = _wtoi(argv[4]);
  ptr->sales0->push_back(p);

  return true;
}

// Format: %date%;%account_id%;%doc_id%;%qty_pos%;%amount%;
static 
bool __f_orders(void *cookie, int line, const wchar_t **argv, int count) 
{
  HTERMINAL *ptr = (HTERMINAL *) cookie;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 5 ) {
    return false;
  }
  if( wcsncmp(ptr->c_date, argv[0], 10) != 0 ) {
    return true;
  }

  ORDER o; memset(&o, 0, sizeof(o));
  COPY_ATTR__S(o.account_id, argv[1]);
  o.amount = wcstof(argv[4]);
  ptr->orders->push_back(o);

  return true;
}

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[1023+1] = L"";
  LoadString(_hResInstance, uID, buf, 1023);
  buf[1023] = L'\0';
  return std::wstring(buf);
}

static
bool IsActivity(HTERMINAL *h) {
  const wchar_t *ptr = h->get_conf(__INT_CURRENT_SCREEN, NULL);
  return ptr == NULL ? false : _wtoi(ptr) != OMOBUS_SCREEN_WORK;
}

static
void DrawRightText(HDC hDC, int top, int right, const wchar_t *txt) {
  SIZE size = {0};
  RECT rect = {0};
  int len = wcslen(txt);
  GetTextExtentPoint32(hDC, txt, len, &size);
  rect.left = right - size.cx; rect.top = top; 
  rect.right = right; rect.bottom = top + size.cy;
  DrawText(hDC, txt, len, &rect, DT_RIGHT|DT_SINGLELINE);
}

static
void DrawLeftText(HDC hDC, int top, int left, const wchar_t *txt) {
  SIZE size = {0};
  RECT rect = {0};
  int len = wcslen(txt);
  GetTextExtentPoint32(hDC, txt, len, &size);
  rect.left = left; rect.top = top; 
  rect.right = left + size.cx; rect.bottom = top + size.cy;
  DrawText(hDC, txt, len, &rect, DT_LEFT|DT_SINGLELINE);
}

static
std::wstring getPersent(const _p_total &p_total, const _o_total &o_total) {
  int32_t persent = p_total.m_amount_f == 0 ? 100 : 
    100-((100*(p_total.m_amount-o_total.m_amount))/p_total.m_amount_f);
  std::wstring rc = itows(p_total.m_valid ? persent : 0); rc += L"%";
  return rc;
}

static
std::wstring getOrderInfo(HTERMINAL *h, const _o_total &o_total) {
  std::wstring rc = ftows(o_total.m_amount, 0, h->thousand_sep);
  rc += L" ("; rc += itows(o_total.m_qty); rc += L")";
  return rc;
}

static
void OnReload(HTERMINAL *h) {
  wcsncpy(h->c_date, wsftime(omobus_time(), ISO_DATE_FMT).c_str(), CALC_BUF_SIZE(h->c_date));
  h->sales = NULL;
  h->sales0->clear();
  h->sales1->clear();
  h->orders->clear();

  ReadOmobusManual(L"my_sales", h, __my_sales);
  DEBUGMSG(1, (L"my_sales = %i/%i [size= %i/%i Kb]\n", 
    h->sales0->size(), 
    h->sales1->size(), 
    h->sales0->size()*sizeof(P_SALES)/1024, 
    h->sales1->size()*sizeof(P_SALES)/1024));
  h->sales = h->sales0->empty() ? h->sales1 : h->sales0;

  ReadOmobusJournal(OMOBUS_ORDER, h, __f_orders);
  DEBUGMSG(1, (L"orders = %i\n", h->orders->size()));
}

static
void OnShow(HTERMINAL *h) {
  DEBUGMSG(1, (L"sales_plans: OnShown\n"));
  OnReload(h);
  SetTimer(h->hWnd, 1, 600000, NULL);
}

static
void OnHide(HTERMINAL *h) {
  KillTimer(h->hWnd, 1);
  memset(h->c_date, 0, sizeof(h->c_date));
  h->sales = NULL;
  h->sales0->clear();
  h->sales1->clear();
  h->orders->clear();
  DEBUGMSG(1, (L"sales_plans: OnHide\n"));
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  _p_total p_total = std::for_each(h->sales->begin(), h->sales->end(), 
    _p_total(h->get_conf(__INT_ACCOUNT_ID, NULL)));
  _o_total o_total = std::for_each(h->orders->begin(), h->orders->end(),
    _o_total(p_total.m_account_id));

  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);
  HPEN hOldPen = (HPEN)SelectObject(hDC, h->hDash);
  bool act = IsActivity(h);
  std::wstring none = wsfromRes(IDS_STR6);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);

  DrawLeftText(hDC, 
    rcDraw->top + DRA::SCALEY(_TEXT_TOP_OFFSET),
    DRA::SCALEX(_TEXT_LEFT_OFFSET), 
    wsfromRes(IDS_STR0).c_str());

  DrawLeftText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT + _TEXT_TOP_OFFSET),
    DRA::SCALEX(_TEXT_LEFT_OFFSET), 
    wsfromRes(IDS_STR2).c_str());

  DrawLeftText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT*2 + _TEXT_TOP_OFFSET),
    DRA::SCALEX(_TEXT_LEFT_OFFSET), 
    wsfromRes(act ? IDS_STR4 : IDS_STR5).c_str());

  DrawLeftText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT*3 + _TEXT_TOP_OFFSET),
    DRA::SCALEX(_TEXT_LEFT_OFFSET), 
    wsfromRes(act ? IDS_STR1 : IDS_STR3).c_str());

  SelectObject(hDC, h->hFontB);

  DrawRightText(hDC, 
    rcDraw->top + DRA::SCALEY(_TEXT_TOP_OFFSET),
    rcDraw->right - DRA::SCALEX(_TEXT_RIGHT_OFFSET), 
    p_total.m_valid 
      ? 
        ftows(p_total.m_amount_f, 0, h->thousand_sep).c_str() 
      : 
        none.c_str());

  DrawRightText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT + _TEXT_TOP_OFFSET),
    rcDraw->right - DRA::SCALEX(_TEXT_RIGHT_OFFSET), 
    p_total.m_valid 
      ? 
        ftows(p_total.m_amount, 0, h->thousand_sep).c_str() 
      : 
        none.c_str());

  DrawRightText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT*2 + _TEXT_TOP_OFFSET),
    rcDraw->right - DRA::SCALEX(_TEXT_RIGHT_OFFSET), 
    act 
      ?
        (
          p_total.m_valid 
            ? 
              ftows(p_total.m_amount_r, 0, h->thousand_sep).c_str() 
            : 
              none.c_str()
        )
      :
        getOrderInfo(h, o_total).c_str());

  DrawRightText(hDC, 
    rcDraw->top + DRA::SCALEY(_ROW_HEIGHT*3 + _TEXT_TOP_OFFSET),
    rcDraw->right - DRA::SCALEX(_TEXT_RIGHT_OFFSET), 
    act 
      ?
        (
          p_total.m_valid 
            ? 
              itows(p_total.m_qty).c_str() 
            : 
              none.c_str()
        )
      :
        getPersent(p_total, o_total).c_str());

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
  SelectObject(hDC, hOldPen);
}

static 
void OnAction(HTERMINAL *h) {
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
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
    _hResInstance = NULL;
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
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->orders = new orders_list;
  h->sales0 = new sales_plans_list;
  h->sales1 = new sales_plans_list;
  h->thousand_sep = get_conf(OMOBUS_MUI_THOUSAND_SEP, NULL);

  _hResInstance = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hFontB = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hDash = CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, OMOBUS_SCREEN_WORK|OMOBUS_SCREEN_ACTIVITY);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  if( h->orders != NULL )
    delete h->orders;
  if( h->sales0 != NULL )
    delete h->sales0;
  if( h->sales1 != NULL )
    delete h->sales1;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontB);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hDash);
  chk_destroy_window(h->hWnd);
  delete h;
}
