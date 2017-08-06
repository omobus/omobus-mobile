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

#include "TodayWindow.h"

#ifndef WS_EX_TRANSPARENT
# define WS_EX_TRANSPARENT 0x00000020L
#endif

#define PLUGIN_SELECTED     0x00000001
#define PLUGIN_TEXTRESIZE   0x00000002

CTodayWindow::CTodayWindow(HINSTANCE hInstance, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, 
  UINT uiIconId) 
{
  m_lpszClassName = lpszClassName;
  m_lpszWindowName = lpszWindowName;
  m_hInstance = hInstance;
  CTodayWindow();
  m_uMetricChangeMsg2 = RegisterWindowMessage(SH_UIMETRIC_CHANGE);
  m_hIcon = (HICON)LoadImage(m_hInstance, MAKEINTRESOURCE(uiIconId), IMAGE_ICON, 
    DRA::SCALEX(16), DRA::SCALEY(16), LR_DEFAULTCOLOR);
  LOGFONT lf;
  HFONT hSysFont = (HFONT)GetStockObject(SYSTEM_FONT);
  GetObject(hSysFont, sizeof(LOGFONT), &lf);
  _tcscpy(lf.lfFaceName, _T("Tahoma"));
  lf.lfHeight = -1*DRA::SCALEY(11);
  lf.lfWeight = FW_NORMAL;
  m_hNormalTodayFont = CreateFontIndirect(&lf);
  lf.lfWeight = FW_SEMIBOLD;//FW_BOLD;
  m_hBoldTodayFont = CreateFontIndirect(&lf);
}

CTodayWindow::~CTodayWindow() 
{
  if( m_hIcon )
	  DestroyIcon(m_hIcon);

  if( m_hNormalTodayFont )
	  DeleteObject(m_hNormalTodayFont);

  if( m_hBoldTodayFont )
	  DeleteObject(m_hBoldTodayFont);
}

BOOL CTodayWindow::Create(HWND hWndParent, int nHeight, DWORD dwStyle) 
{
  m_nInitHeight = m_nHeight = nHeight;
  m_hParentHwnd = hWndParent;
  m_hWnd = CreateWindowEx(WS_EX_TRANSPARENT, m_lpszClassName, m_lpszWindowName, dwStyle, 0, 0, 
    GetSystemMetrics(SM_CXSCREEN), 0, m_hParentHwnd, NULL, m_hInstance, NULL);

  return (m_hWnd != NULL);
}

void CTodayWindow::RegisterTodayClass(WNDPROC wndProc)
{
	WNDCLASS wndClass;

	memset(&wndClass, 0, sizeof(wndClass));
	wndClass.hCursor = 0;
	wndClass.hIcon = 0;
	wndClass.hInstance = m_hInstance;
	wndClass.lpszClassName = m_lpszClassName;
	wndClass.lpszMenuName = NULL;
	wndClass.style = CS_VREDRAW | CS_HREDRAW;
	wndClass.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
	wndClass.lpfnWndProc = wndProc;

	RegisterClass(&wndClass);
}

LRESULT CTodayWindow::TodayWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch( uMsg ) {
	case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
      SetWindowLong(m_hWnd, GWL_USERDATA, 0);
			if (OnCreate(lpcs) == -1)
				return -1;
			break;
	  }
	case WM_DESTROY: {
			OnDestroy();
			return 0;
		}
	case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(m_hWnd, &ps);
      RECT rcDraw; GetClientRect(m_hWnd, &rcDraw);
      COLORREF crText, crHighlightedText, crOld;
      int nBkMode;
      TODAYDRAWWATERMARKINFO dwi = {0};
      dwi.rc = rcDraw;
      dwi.hwnd = m_hWnd;
      dwi.hdc = hDC;

      if( IsItemState(m_hWnd, PLUGIN_SELECTED) ) {
        COLORREF crHighlight = SendMessage(m_hParentHwnd, TODAYM_GETCOLOR, TODAYCOLOR_HIGHLIGHT, 0);
        nBkMode = SetBkMode(hDC, OPAQUE);
        FillRectClr(hDC, &rcDraw, crHighlight);
        SetBkMode(hDC, nBkMode);
      }
      else {
        if( !SendMessage(m_hParentHwnd, TODAYM_DRAWWATERMARK, 0, (LPARAM) &dwi) )
          FillRectClr(hDC, &rcDraw, GetSysColor(COLOR_WINDOW));
      }
      
      crHighlightedText = SendMessage(m_hParentHwnd, TODAYM_GETCOLOR, TODAYCOLOR_HIGHLIGHTEDTEXT, 0);
      crText = SendMessage(m_hParentHwnd, TODAYM_GETCOLOR, TODAYCOLOR_TEXT, 0);
      nBkMode = SetBkMode(hDC, TRANSPARENT);
      crOld = SetTextColor(hDC, IsItemState(m_hWnd, PLUGIN_SELECTED) ? crHighlightedText : crText);

		  ::DrawIcon(hDC, DRA::SCALEX(2), DRA::SCALEY(2), m_hIcon);
			OnPaint(hDC);
      
      crText = SetTextColor(hDC, crOld);
      SetBkMode(hDC, nBkMode);

			EndPaint(m_hWnd, &ps);
			return TRUE;
		}
	case WM_LBUTTONDOWN: {
      PostMessage(GetParent(m_hWnd), TODAYM_TOOKSELECTION, (WPARAM)m_hWnd, 0);
      SetItemState(m_hWnd, PLUGIN_SELECTED, TRUE);
			return 0;
		}
  case WM_ERASEBKGND: {
    return TRUE;
  }
	case WM_TODAYCUSTOM_QUERYREFRESHCACHE: {
			BOOL bResult = FALSE;
			TODAYLISTITEM *ptli = (TODAYLISTITEM *)wParam;
      if( ptli->cyp != DRA::SCALEX(m_nHeight) ) {
        // If the heights are not equal, return true so we tell the Today screen our
        // height has changed.
        ptli->cyp = DRA::SCALEX(m_nHeight);
        InvalidateRect(m_hWnd, NULL, FALSE);
        bResult = TRUE;
      } else if( IsItemState(m_hWnd, PLUGIN_TEXTRESIZE) ) {
        if( ptli->cyp > DRA::SCALEX(m_nInitHeight) ) {
          m_nHeight = m_nInitHeight;
          ptli->cyp = DRA::SCALEX(m_nHeight);
        } else {
          m_nHeight = m_nInitHeight + 20;
          ptli->cyp = DRA::SCALEX(m_nHeight);
        }
        SetItemState(m_hWnd, PLUGIN_TEXTRESIZE, FALSE);
        InvalidateRect(m_hWnd, NULL, FALSE);
        bResult = TRUE;   
      } else {
        bResult = FALSE;
      }
			return (LRESULT)bResult;
		}
  // http://msdn.microsoft.com/en-us/library/ms839449.aspx
	case WM_TODAYCUSTOM_RECEIVEDSELECTION: {
      BOOL bResult = FALSE;
      if( wParam == VK_UP || wParam == VK_DOWN ) {
        SetItemState(m_hWnd, PLUGIN_SELECTED, TRUE);
        bResult = TRUE;
      }
			return (LRESULT)bResult;
    }
	case WM_TODAYCUSTOM_LOSTSELECTION: {
      SetItemState(m_hWnd, PLUGIN_SELECTED, FALSE);
      InvalidateRect(m_hWnd, NULL, FALSE);
      return FALSE;
    }
	case WM_LBUTTONUP: {
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			OnLButtonUp(wParam, point);
			return 0;
		}
	case WM_TODAYCUSTOM_ACTION: {
      OnTodayAction();
			return 0;
		}
	case WM_SETTINGCHANGE: {
			OnSettingChange(wParam, (LPCTSTR)lParam);
			return 0;
		}
	case WM_NOTIFY: {
			NMHDR* pNMHDR = (NMHDR*)lParam;
			return OnNotify(wParam, pNMHDR);
		}
  case WM_TODAYCUSTOM_CLEARCACHE:
  case WM_TODAYCUSTOM_USERNAVIGATION:
  default: {
      //user changed the Text Size.  we should resize this plugin in the next 
      // WM_TODAYCUSTOM_QUERYREFRESHCACHE
      if (uMsg == m_uMetricChangeMsg2)
        SetItemState(m_hWnd, PLUGIN_TEXTRESIZE, TRUE);
		  return OnMessage(uMsg, wParam, lParam);
    }
	}

	return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
