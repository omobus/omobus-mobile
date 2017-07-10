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

#include <windows.h>
#include <DeviceResolutionAware.h>
#include <WaitCursor.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <omobus-mobile.h>
#include <ensure_cleanup.h>

#include "resource.h"

#define OMOBUS_HEIGHT                  L"merch->height"
#define OMOBUS_HEIGHT_DEF              L"263"

struct BRAND_CONF {
  wchar_t brand_id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<BRAND_CONF> brands_t;

typedef struct _tagMODULE {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf; 
  HWND hWnd0, hWnd1, hWndL, hParWnd;
  HFONT hFont, hFontN;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
  bool focus0, focus1;
  BRAND_CONF *brand;
  brands_t *_brands;
  wchar_t _sel_brand_id[OMOBUS_MAX_ID + 1];
} HTERMINAL;

struct _find_brand_id : public std::unary_function <BRAND_CONF&, bool> { 
  _find_brand_id(const wchar_t *brand_id_) : brand_id(brand_id_) {
  }
  bool operator()(const BRAND_CONF &v) { 
    return COMPARE_ID(brand_id, v.brand_id);
  }
private:
  const wchar_t *brand_id;
};


static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static TCHAR _szWindowClass0[] = L"omobus-merch0-terminal-element";
static TCHAR _szWindowClass1[] = L"omobus-merch1-terminal-element";
static int _nHeight0 = 20, _nHeight1 = 0;
static LVCOLUMN _lvc = { 0 };

// Format: %id%;%descr%;
static 
bool __brands(void *cookie, int line, const wchar_t **argv, int count) 
{
  BRAND_CONF cnf;
  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 2 ) {
    return false;
  }
  memset(&cnf, 0, sizeof(BRAND_CONF));
  COPY_ATTR__S(cnf.brand_id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  ((brands_t *)cookie)->push_back(cnf);
  return true;
}

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
int getStatus(HTERMINAL *h) {
  const wchar_t *ptr = h->get_conf(__INT_CURRENT_SCREEN, NULL);
  return ptr == NULL ? OMOBUS_SCREEN_ROOT : _wtoi(ptr);
}

static
void putStatus(HTERMINAL *h, int status) {
  wchar_t buf[11]; wsprintf(buf, L"%i", status);
  h->put_conf(__INT_CURRENT_SCREEN, buf);
}

static
std::wstring getMsg(int status, const wchar_t *b_name) {
  std::wstring msg = wsfromRes(
    (status & OMOBUS_SCREEN_ACTIVITY || status & OMOBUS_SCREEN_ACTIVITY_TAB_1) ? IDS_TITLE0 : 
    (status & OMOBUS_SCREEN_MERCH ? IDS_TITLE1 : IDS_TITLE2) );
  if( status & OMOBUS_SCREEN_MERCH_TASKS ) {
    msg += b_name;
  }
  std::transform(msg.begin(), msg.end(), msg.begin(), ::towupper);
  return msg;
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
                rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus0 ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, getMsg(getStatus(h), h->brand->descr).c_str(), -1, &rect, DT_RIGHT|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_VCENTER);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
bool OnStartMerch(HTERMINAL *h) {
  COPY_ATTR__S(h->_sel_brand_id, OMOBUS_UNKNOWN_ID);
  return true;
}

static
bool OnStopMerch(HTERMINAL *h) {
  COPY_ATTR__S(h->_sel_brand_id, OMOBUS_UNKNOWN_ID);
  return true;
}

static
bool OnStartMerchTasks(HTERMINAL *h, BRAND_CONF *brand) {
  h->put_conf(__INT_MERCH_BRAND_ID, brand->brand_id);
  h->put_conf(__INT_MERCH_BRAND, brand->descr);
  h->brand = brand;
  return true;
}

static
bool OnStopMerchTasks(HTERMINAL *h) {
  COPY_ATTR__S(h->_sel_brand_id, h->brand == NULL ? OMOBUS_UNKNOWN_ID : h->brand->brand_id);
  h->put_conf(__INT_MERCH_BRAND_ID, NULL);
  h->put_conf(__INT_MERCH_BRAND, NULL);
  h->brand = NULL;
  return true;
}

static 
void OnAction(HTERMINAL *h) {
  switch( getStatus(h) ) {
  case OMOBUS_SCREEN_ACTIVITY:
  case OMOBUS_SCREEN_ACTIVITY_TAB_1:
    if( OnStartMerch(h) ) {
      putStatus(h, OMOBUS_SCREEN_MERCH);
    }
    break;
  case OMOBUS_SCREEN_MERCH:
    if( OnStopMerch(h) ) {
      putStatus(h, OMOBUS_SCREEN_ACTIVITY);
    }
    break;
  case OMOBUS_SCREEN_MERCH_TASKS:
    if( OnStopMerchTasks(h) ) {
      putStatus(h, OMOBUS_SCREEN_MERCH);
    }
    break;
  };
}

static
HWND InitListView(HWND hWnd, RECT *rcDraw) {
  HWND hWndL = NULL;
  hWndL = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL |
     LVS_OWNERDRAWFIXED | LVS_ALIGNLEFT | LVS_OWNERDATA | LVS_NOCOLUMNHEADER | WS_TABSTOP, 
     rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top, 
     hWnd, (HMENU)NULL, _hInstance, NULL);
  if( hWndL == NULL ) {
    RETAILMSG(TRUE, (L"merch: Unable to create ListView.\n"));
    return hWndL;
  }
  _lvc.mask = LVCF_WIDTH;
  _lvc.cx = DRA::SCALEY(240);
  ListView_InsertColumn(hWndL, 0, &_lvc);
  ListView_SetExtendedListViewStyle(hWndL, LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
  return hWndL;
}

static
void DrawLine(HDC hDc, int left, int top, int right, int bottom) {
  MoveToEx(hDc, left, top, NULL); LineTo(hDc, right, bottom);
}

static
int GetLastSelectionMark(HTERMINAL *h) {
  brands_t::iterator it;
  return 
    h->_sel_brand_id[0] == L'\0' 
      ||
    (it = std::find_if(h->_brands->begin(), h->_brands->end(), _find_brand_id(h->_sel_brand_id)))
      == h->_brands->end() ? -1 : (int) (it - h->_brands->begin());
}

static
void OnDrawItemL(HTERMINAL *h, LPDRAWITEMSTRUCT dis) 
{
  BRAND_CONF *ptr;
  HDC hDC;
  HFONT hOldFont;
  HPEN hOldPen;
  COLORREF crOldTextColor, crOldBkColor;
  RECT rc;
  int scrWidth;
  
  if( dis->CtlType != ODT_LISTVIEW || dis->itemID >= h->_brands->size() || 
      (ptr = &((*h->_brands)[dis->itemID])) == NULL || (hDC = dis->hDC) == NULL ) {
    return;
  }

  scrWidth = GetSystemMetrics(SM_CXVSCROLL);
  hOldFont = (HFONT)SelectObject(hDC, h->hFontN);
  hOldPen = (HPEN)SelectObject(hDC, h->hpBord);
  crOldTextColor = GetTextColor(hDC);
  crOldBkColor = GetBkColor(hDC);
  memcpy(&rc, &dis->rcItem, sizeof(RECT));

  if( h->focus1 && (dis->itemAction | ODA_SELECT) && (dis->itemState & ODS_SELECTED) ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, &rc, h->hbrHL);
  } else {
    FillRect(hDC, &rc, dis->itemID%2==0?h->hbrN:h->hbrOdd);
  }

  SetBkMode(hDC, TRANSPARENT);

  rc.left += DRA::SCALEX(2);
  rc.right -= (scrWidth + DRA::SCALEX(2));
  rc.top += DRA::SCALEY(2);
  rc.bottom -= DRA::SCALEY(2);
  DrawText(hDC, ptr->descr, -1, &rc, DT_LEFT|DT_VCENTER|DT_NOPREFIX|
    DT_SINGLELINE|DT_WORD_ELLIPSIS);

  DrawLine(hDC, dis->rcItem.left, dis->rcItem.bottom - 1, 
    dis->rcItem.right, dis->rcItem.bottom - 1);

  SelectObject(hDC, hOldFont);
  SelectObject(hDC, hOldPen);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static
void OnMeasureItemL(HWND hWnd, LPMEASUREITEMSTRUCT mis) {
  if( mis->CtlType == ODT_LISTVIEW )
    mis->itemHeight = DRA::SCALEY(25);
}

static
void OnShowL(HTERMINAL *h, RECT *rcDraw) {
  WaitCursor wc(_hInstance);
  int selMark = -1;
  h->_brands->clear();
  ReadOmobusManual(L"brands", h->_brands, __brands);
  DEBUGMSG(TRUE, (L"brands = %i\n", h->_brands->size()));
  if( !h->_brands->empty() ) {
    if( h->hWndL == NULL ) {
      h->hWndL = InitListView(h->hWnd1, rcDraw);
    }
  } else {
    chk_destroy_window(h->hWndL);
  }
  if( h->hWndL != NULL ) {
    ListView_SetItemCount(h->hWndL, h->_brands->size());
    if( (selMark = GetLastSelectionMark(h)) != -1 ) {
      ListView_SetItemState(h->hWndL, selMark, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
      ListView_EnsureVisible(h->hWndL, selMark, FALSE);
      SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
    }
    ShowWindow(h->hWndL, SW_SHOWNORMAL);
  }
  DEBUGMSG(TRUE, (L"merch::OnShow(): selMark=%i\n", selMark));
}

static
void OnHideL(HTERMINAL *h) {
  DEBUGMSG(TRUE, (L"merch::OnHide\n"));
  chk_destroy_window(h->hWndL);
  h->_brands->clear();
}

static 
void OnActionL(HTERMINAL *h, size_t item) {
  if( item >= h->_brands->size() )
    return;
  if( OnStartMerchTasks(h, &((*h->_brands)[item])) ) {
    putStatus(h, OMOBUS_SCREEN_MERCH_TASKS);
  }
}

static
void OnPaintL(HTERMINAL *h, HDC hDC, RECT *rcDraw) {
  RECT rect = { rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(20), 
                rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2)};
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFontN);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  SetTextColor(hDC, RGB(150, 150, 150));
  FillRect(hDC, rcDraw, h->hbrN);

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, wsfromRes(IDS_TITLE3).c_str(), -1, &rect, DT_CENTER|DT_SINGLELINE|
    DT_NOPREFIX|DT_WORD_ELLIPSIS|DT_TOP);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
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
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus0 = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus0 ) {
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    	InvalidateRect(hWnd, NULL, FALSE);
    }
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
LRESULT CALLBACK WndProcL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  HTERMINAL *h;
  RECT rcDraw;
  int l;
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
      GetClientRect(hWnd, &rcDraw);
      OnPaintL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), hDC, &rcDraw);
			EndPaint(hWnd, &ps);
		}
		return TRUE;
	case OMOBUS_SSM_PLUGIN_SHOW:
	case OMOBUS_SSM_PLUGIN_RELOAD:
    GetClientRect(hWnd, &rcDraw);
    OnShowL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), &rcDraw);
    return 0;
	case OMOBUS_SSM_PLUGIN_HIDE:
    OnHideL((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    return 0;
	case OMOBUS_SSM_PLUGIN_SELECT:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    h->focus1 = ((BOOL)wParam)==TRUE;
    l = ListView_GetSelectionMark(h->hWndL);
    ListView_RedrawItems(h->hWndL, l, l);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    l = ListView_GetSelectionMark(h->hWndL);
    if( h->focus1 ) {
    	OnActionL(h, l);
    }
		return 0;
  case WM_LBUTTONDOWN:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
		return 0;
  case WM_DRAWITEM:
    OnDrawItemL(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA)), 
      (LPDRAWITEMSTRUCT)lParam);
    return TRUE;
  case WM_MEASUREITEM:
    h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
    OnMeasureItemL(h->hParWnd, (LPMEASUREITEMSTRUCT)lParam);
    return TRUE;
  case WM_NOTIFY: {
      LPNMHDR pnmh = (LPNMHDR) lParam;
      h = ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
      if( pnmh->code == LVN_ITEMACTIVATE || pnmh->code ==  LVN_ITEMCHANGED ) {
        if( !h->focus1 ) {
          SendMessage(h->hParWnd, OMOBUS_SSM_PLUGIN_SELECT, 0, (LPARAM)h->hWnd1);
        }
        if( pnmh->code == LVN_ITEMACTIVATE ) {
          OnActionL(h, ((NMLISTVIEW *)pnmh)->iItem);
        }
      }
    }
    break;
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
  if( (h->_brands = new brands_t) == NULL ) {
    delete h;
    return NULL;
  }
  h->hParWnd = hParWnd;
  h->get_conf = get_conf;
  h->put_conf = put_conf;

  _hResInstance = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _nHeight1 = _wtoi(get_conf(OMOBUS_HEIGHT, OMOBUS_HEIGHT_DEF));

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_BOLD, FALSE);
  h->hFontN = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd0 = CreatePluginWindow(h, _szWindowClass0, WndProc)) != NULL ) {
    rsi(h->hWnd0, _nHeight0, (wcsistrue(get_conf(__INT_SPLIT_ACTIVITY_SCREEN, NULL)) ?
      OMOBUS_SCREEN_ACTIVITY_TAB_1 : OMOBUS_SCREEN_ACTIVITY) | 
      OMOBUS_SCREEN_MERCH | OMOBUS_SCREEN_MERCH_TASKS);
  }
  if( (h->hWnd1 = CreatePluginWindow(h, _szWindowClass1, WndProcL)) != NULL ) {
    rsi(h->hWnd1, _nHeight1, OMOBUS_SCREEN_MERCH);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  delete h->_brands;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hFontN);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd0);
  chk_destroy_window(h->hWnd1);
  chk_destroy_window(h->hWndL);
  delete h;
}
