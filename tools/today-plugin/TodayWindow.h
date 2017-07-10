/* -*- H -*- */
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

#ifndef __today_window_h__
#define __today_window_h__

#include <windows.h>
#include <todaycmn.h>
#include <aygshell.h>
#include <DeviceResolutionAware.h>

class CTodayWindow  
{
public:
  CTodayWindow() {
	  m_hWnd = NULL;
    m_hIcon = NULL;
    m_hNormalTodayFont = NULL;
	  m_hBoldTodayFont = NULL;
    m_uMetricChangeMsg2 = 0;
    m_nInitHeight = m_nHeight = 20;
  }

  CTodayWindow(HINSTANCE hInstance, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT uiIconId);
  virtual ~CTodayWindow();

	// Main Create method
  BOOL Create(HWND hWndParent, int nHeight = 20, DWORD dwStyle = WS_VISIBLE | WS_CHILD);

	// Register/Unregister TodayWindow
	void RegisterTodayClass(WNDPROC wndProc);
  void UnregisterTodayClass() {
	  if( m_hInstance )
		  UnregisterClass(m_lpszClassName, m_hInstance);
  }

	// Update Window
  void RefreshWindow(BOOL bShow = FALSE) {
	  if( m_hWnd ) {
		  if( bShow )
			  ShowWindow(m_hWnd, SW_SHOWNORMAL);
		  UpdateWindow(m_hWnd);
	  }
  }

  void SetClassInfo(LPCTSTR lpszClassName, LPCTSTR lpszWindowName) {
	  m_lpszClassName = lpszClassName;
	  m_lpszWindowName = lpszWindowName;
  }

  void SetInstance(HINSTANCE hInstance) {
    m_hInstance = hInstance;
  }

	// Get Methods
  HWND GetHWND() {
    return m_hWnd;
  }

  HINSTANCE GetInstance() {
    return m_hInstance;
  }
	
	// TodayWndProc - main message loop
	virtual LRESULT CALLBACK TodayWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	// Message handlers
  virtual int OnCreate(LPCREATESTRUCT lpCreateStruct) {
    return 0;
  }

  virtual void OnDestroy() {
  }

  virtual void OnPaint(HDC hDC) {
  }

  virtual void OnLButtonUp(UINT nFlags, POINT point) {
  }

  virtual void OnTodayAction() {
  }

  virtual void OnSettingChange(UINT nFlags, LPCTSTR lpszSection) {
  }

  virtual LRESULT OnNotify(UINT nID, NMHDR* pNMHDR) {
    return 0;
  }
  
  virtual LRESULT OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
  }

  void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr) {
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
  }

  void SetItemState(HWND hwnd, LONG lState, BOOL fSet) {
    LONG lData = GetWindowLong(hwnd, GWL_USERDATA);
    if (fSet)
      lData |= lState;
    else
      lData &= ~lState;
    SetWindowLong(hwnd, GWL_USERDATA, lData);
  }

  BOOL IsItemState(HWND hwnd, LONG lState) {
    return ((GetWindowLong(hwnd, GWL_USERDATA) & lState) == lState);
  }

protected:
	HWND m_hWnd;
	HWND m_hParentHwnd;
	HINSTANCE m_hInstance;
	LPCTSTR m_lpszClassName;
	LPCTSTR m_lpszWindowName;
	HICON m_hIcon;
	HFONT m_hNormalTodayFont;
	HFONT m_hBoldTodayFont;
	int m_nHeight, m_nInitHeight;
  uint m_uMetricChangeMsg2;
};

#endif //__today_window_h__
