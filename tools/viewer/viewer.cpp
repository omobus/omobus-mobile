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
//#define _WTL_NO_CSTRING
#define _ATL_NO_HOSTING
#define _ATL_USE_CSTRING_FLOAT
#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA

#include <windows.h>
#include <stdlib.h>
#include <tpcshell.h>
#include <aygshell.h>
#include <gesture.h>

#include <atlbase.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlscrl.h>
#include <atlctrlx.h>
#include <atlwince.h>

#include <omobus-mobile.h>

#include "resource.h"

CAppModule _Module;

class CImageView : 
	public CWindowImpl<CImageView>,
	public CZoomScrollImpl<CImageView>	
{
private:
	CBitmap m_bmp;
	CPoint m_ptMouse;

public:
	DECLARE_WND_CLASS(NULL)

	CImageView(const wchar_t *fname) {
    SetImageFile(fname);
  }

  BOOL PreTranslateMessage(MSG* pMsg) {
		pMsg;
		return FALSE;
	}
	
	BEGIN_MSG_MAP(CImageView)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		CHAIN_MSG_MAP(CZoomScrollImpl<CImageView>)
	END_MSG_MAP()

  LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		m_sizeClient = CSize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		CPoint ptOffset0 = m_ptOffset;
    if( AdjustScrollOffset((int&)m_ptOffset.x, (int&)m_ptOffset.y) ) {
			ScrollWindowEx(ptOffset0.x - m_ptOffset.x, ptOffset0.y - m_ptOffset.y, m_uScrollFlags);
		}
		return bHandled;
	}

  LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		m_ptMouse = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		SHRGINFO shrgi = {sizeof(SHRGINFO), m_hWnd, m_ptMouse.x, m_ptMouse.y, SHRG_NOTIFYPARENT};
		return SHRecognizeGesture(&shrgi);
	}

  LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		if( !m_bmp.IsNull()) {
			CPoint ptNew(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			SetScrollOffset(GetScrollOffset() + ((m_ptMouse - ptNew) * GetZoom()));
			m_ptMouse = ptNew;
		}
		return 0;
	}

	void DoPaint(CDCHandle dc) {
		if( !m_bmp.IsNull())
			Draw(m_bmp, dc);
	}

  bool SetImageFile(const wchar_t *fname) {
    CWaitCursor _wc;
    if( !wcsfexist(fname) ) {
      return false;
    }
    HBITMAP bmp = LoadImageFile(fname, 1200, 1600, NULL, NULL);
    if( bmp != NULL ) {
      SetImage(bmp);
    }
    return bmp != NULL;
  }

private:
	void SetImage(HBITMAP hBitmap, double fZoom = 1.0, POINT pScroll = CPoint(-1,-1))
	{
		CRect rect;
		CSize sizeImage(1, 1);

    m_bmp.Attach(hBitmap); // will delete existing if necessary

		if( !m_bmp.IsNull() ) {
			m_bmp.GetSize(sizeImage);
		}

		SetZoomScrollSize(sizeImage, fZoom, FALSE);
		SetScrollPage(m_sizeClient);
		if( CPoint(-1, -1) == pScroll )
			pScroll = (CPoint)((sizeImage - m_sizeClient) / 2);

		SetScrollOffset(pScroll, false);
		ShowScrollBars(false);

		sizeImage *= 100;
		SystemParametersInfo(SPI_GETWORKAREA, NULL, rect, FALSE);
	}

	void ShowScrollBars(bool bShow) {
		if( bShow ) {
			SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_RANGE | SIF_POS};
			si.nMax = m_sizeAll.cx - 1;
			si.nPage = m_sizeClient.cx;
			si.nPos = m_ptOffset.x;
			SetScrollInfo(SB_HORZ, &si);
			si.nMax = m_sizeAll.cy - 1;
			si.nPage = m_sizeClient.cy;
			si.nPos = m_ptOffset.y;
			SetScrollInfo(SB_VERT, &si);
		} else {
			SCROLLINFO si = { sizeof(si), SIF_RANGE, 0, 0};
			SetScrollInfo(SB_HORZ, &si);
			SetScrollInfo(SB_VERT, &si);
		}
		Invalidate();
	}

	void SetScrollOffset(POINT ptOffset, BOOL bRedraw = TRUE) {
		m_ptOffset = CPoint(CSize(ptOffset) / m_fzoom);
		AdjustScrollOffset((int&)m_ptOffset.x, (int&)m_ptOffset.y);
		if( bRedraw)
			Invalidate();
	}

	void SetZoom(double fzoom, BOOL bRedraw = TRUE) {
		CPoint ptCenter = WndtoTrue(m_sizeClient / 2 );
		m_sizeAll = m_sizeTrue / fzoom;
		m_fzoom = fzoom;
		m_ptOffset = TruetoWnd(ptCenter) + m_ptOffset - m_sizeClient/ 2;
		AdjustScrollOffset((int&)m_ptOffset.x, (int&)m_ptOffset.y);
		if( bRedraw)
			Invalidate();
	}
};

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CFullScreenFrame<CMainFrame>,
  public CAppWindow<CMainFrame>,
	public CMessageFilter
{
private:
	CImageView m_view;

public:
	DECLARE_APP_FRAME_CLASS(NULL, IDR_MAINFRAME, L"")

  CMainFrame(const wchar_t *fname) : m_view(fname) {
  }

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if( CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE; 
		return m_view.IsWindow() ? m_view.PreTranslateMessage(pMsg) : FALSE;
	}

	bool AppNewInstance(LPCTSTR lpstrCmdLine) {
		return m_view.SetImageFile(lpstrCmdLine);
	}


	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_GESTURE, OnGesture)
    CHAIN_MSG_MAP(CAppWindow<CMainFrame>)
		CHAIN_MSG_MAP(CFullScreenFrame<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
    m_hWndCECommandBar = AtlCreateEmptyMenuBar(m_hWnd, false);
    SetFullScreen(TRUE);
		m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | 
      WS_CLIPCHILDREN);

    CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);

		return 0;
	}

  LRESULT OnGesture(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    LRESULT rc = 0;
    GESTUREINFO gi;
    memset(&gi, 0, sizeof(gi));
    gi.cbSize = sizeof(GESTUREINFO);
    if( TKGetGestureInfo((HGESTUREINFO)lParam, &gi) ) {
      if( wParam == GID_DOUBLESELECT ) {
        PostMessage(WM_CLOSE);
      }
    }
    return rc;
  }
};

int Run(const wchar_t *fname) 
{
	CMainFrame wndMain(fname);
	CMessageLoop theLoop;
  int nRet = 0;

	_Module.AddMessageLoop(&theLoop);
	if(wndMain.CreateEx() == NULL) {
		ATLTRACE2(atlTraceUI, 0, _T("Main window creation failed!\n"));
		return 0;
	}
	wndMain.ShowWindow(SW_SHOWNORMAL);
	nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

  return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpstrCmdLine, int nCmdShow)
{
  int nRet = 0;
	HRESULT hRes = S_OK;
  
  hRes = CMainFrame::ActivatePreviousInstance(hInstance, lpstrCmdLine);
	if( FAILED(hRes) || S_FALSE == hRes){
		return hRes;
	}

  if( !wcsfexist(lpstrCmdLine) ) {
    return 1;
  }

  hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_DATE_CLASSES);
	SHInitExtraControls();

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	nRet = Run(lpstrCmdLine);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}

