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

#pragma once

#include <atlstr.h>
#include <atlmisc.h>
#include <atlctrlx.h>
#include "omobus-mobile.h"

#ifndef _CSTRING_NS
# pragma message("Please, define the following macros _CSTRING_NS !!!")
#endif //_CSTRING_NS

inline CString LoadStringEx(UINT uid) {
  CString msg; msg.LoadString(uid);
  return msg;
}

inline int MessageBoxEx(HWND hWnd, const wchar_t *sMsg, UINT uCap, UINT uType) {
  return ::MessageBox(hWnd, sMsg, LoadStringEx(uCap), uType);
}

inline int MessageBoxEx(HWND hWnd, UINT uMsg, UINT uCap, UINT uType) {
  return MessageBoxEx(hWnd, LoadStringEx(uMsg), uCap, uType);
}

inline int MessageBoxEx(UINT uMsg, UINT uCap, UINT uType) {
  return MessageBoxEx(::GetActiveWindow(), uMsg, uCap, uType);
}

template <class t_HyperButtonCallback>
class CHyperButtonTempl : 
  public CHyperLinkImpl< CHyperButtonTempl<t_HyperButtonCallback> >
{
private:
	UINT m_nID;
  t_HyperButtonCallback *m_p;

public:
  void SetParent(t_HyperButtonCallback *p, UINT nID) {
    m_p = p;
    m_nID = nID; 
  }

  bool SetTitle(LPCTSTR body) {
    CString t = L"["; t += body; t += L"]";
    return SetLabel(t);
  }

	bool Navigate() {	
		bool f = m_p->OnNavigate(m_hWnd, m_nID);
		if( f )	{	
      m_bVisited = false; 
      Invalidate(); 
    }
		return f;
	}
};

class CDimEdit : public CWindowImpl< CDimEdit, CEdit >
{
public:
  DECLARE_WND_CLASS(_T("WTL_DimEdit"))

  CDimEdit( ) : CWindowImpl< CDimEdit, CEdit >( ),
    m_isDim( true ), m_isFlat( false ), 
    m_dimText( _T("<Click here to enter value>") ),
    m_dimColor( RGB( 128, 128, 128 ) ) {
  }

  virtual ~CDimEdit() {
    if( m_hWnd != NULL && IsWindow() ) {
      DestroyWindow();
    }
    m_hWnd = NULL;
  }

  BOOL SubclassWindow( HWND hWnd )
  {
    return ( CWindowImpl< CDimEdit, CEdit >::SubclassWindow( hWnd ) );
  }

  void SetDimText( const UINT uID )
  {
    m_dimText.LoadString(uID);
  }

  void SetDimText( const CString& dimText )
  {
    m_dimText = dimText;
  }

  void SetDimColor( const COLORREF dimColor )
  {
    m_dimColor = dimColor;
  }

  void SetFlatMode(bool isFlat=true) 
  {
    m_isFlat = isFlat;
  }

  void DoPaint(CDCHandle dc) 
  {
    RECT rect; 
    LOGFONT lf = {0};
    CBrushHandle brush;
    CFont font(HFONT(GetStockObject(DEFAULT_GUI_FONT)));
    font.GetLogFont(&lf);
    font.DeleteObject();
    lf.lfItalic = TRUE;
    font.CreateFontIndirect(&lf);
    GetClientRect(&rect);

    brush.CreateSysColorBrush(COLOR_WINDOW);
    dc.FillRect(&rect, brush);
    dc.SelectFont(font);

    dc.SetTextColor(m_dimColor);
    dc.SetTextAlign(TA_LEFT|(m_isFlat?TA_TOP:0));
    dc.ExtTextOut(m_isFlat ? 0 : DRA::SCALEX(2), m_isFlat ? 0 : DRA::SCALEY(2), 
      0, NULL, m_dimText);
  }

  BEGIN_MSG_MAP( CDimEdit )
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
  END_MSG_MAP( )


  LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
  {
    if( m_hWnd == NULL || !IsWindow() ) {
      return 0;
    }
    if( !m_isDim ) {
      DefWindowProc( );
      return 0;
    }

    if(wParam != NULL)
		{
			DoPaint((HDC)wParam);
		}
		else
		{
			CPaintDC dc(m_hWnd);
			DoPaint(dc.m_hDC);
		}

    return 0;
  }

  LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
  {
    if( m_hWnd == NULL || !IsWindow() ) {
      return 0;
    }

    DefWindowProc( );

    if( GetWindowTextLength( ) == 0 )
    {
      m_isDim = false;
      Invalidate( );
    }

    return 0;
  }

  LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
  {
    if( m_hWnd == NULL || !IsWindow() ) {
      return 0;
    }

    DefWindowProc( );

    if( GetWindowTextLength( ) == 0 )
    {
      m_isDim = true;
      Invalidate( );
    }

    return 0;
  }

private:
  bool m_isDim, m_isFlat;
  CString m_dimText;
  DWORD m_dimColor;
};

template<UINT t_IDD, UINT t_ListID, class t_list, class t_conf>
class CListFrame : 
	public CStdDialogResizeImpl< CListFrame<t_IDD, t_ListID, t_list, t_conf> >,
	public CUpdateUI< CListFrame<t_IDD, t_ListID, t_list, t_conf> >,
	public CMessageFilter
{
  typedef CListFrame<t_IDD, t_ListID, t_list, t_conf> baseClass;

protected:
  HBRUSH m_hbrHL, m_hbrN, m_hbrOdd;
  HPEN m_hpBord;
  t_list *m_l;
  t_conf **m_cur;
  CListViewCtrl m_list;
  CFont m_baseFont;

public:
  CListFrame(t_list *l, t_conf **cur, HBRUSH hbrHL, HBRUSH hbrN, HBRUSH hbrOdd, HPEN hpBord) : 
      m_l(l), m_cur(cur), m_hbrHL(hbrHL), m_hbrN(hbrN), m_hbrOdd(hbrOdd), m_hpBord(hpBord)
  {
    m_baseFont = CreateBaseFont(DRA::SCALEY(14), FW_SEMIBOLD, FALSE);
  }

	enum { IDD = t_IDD };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(baseClass)
    DLGRESIZE_CONTROL(t_ListID, DLSZ_SIZE_Y|DLSZ_SIZE_X)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(baseClass)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(baseClass)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
    MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
    NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnClick)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<baseClass>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, ATL_IDM_MENU_DONECANCEL, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    m_list = GetDlgItem(t_ListID);
    m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_NOHSCROLL|LVS_EX_ONECLICKACTIVATE);
    m_list.InsertColumn(0, NULL, 0, DRA::SCALEY(240));
    m_list.SetItemCount(m_l->size());
    m_list.ShowWindow(m_l->empty()?SW_HIDE:SW_NORMAL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    if( lpdis->CtlType != ODT_LISTVIEW )
      return 0;

    t_conf *ptr = &((*m_l)[lpdis->itemID]);
    if( ptr == NULL )
      return 0;

    CDCHandle dc = lpdis->hDC;
    COLORREF crOldTextColor = GetTextColor(dc), crOldBkColor = GetBkColor(dc);
    CPen oldPen = dc.SelectPen(m_hpBord);
    CFont oldFont = dc.SelectFont(m_baseFont);
    int scrWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT rect = lpdis->rcItem;

    if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) ) {
      dc.SetTextColor(OMOBUS_HLCOLOR);
      dc.FillRect(&lpdis->rcItem, m_hbrHL);
    }
    else {
      dc.FillRect(&lpdis->rcItem, lpdis->itemID%2==0?m_hbrN:m_hbrOdd);
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
    _Next(((LPNMLISTVIEW)pnmh)->iItem);
    return 0;
  }

  LRESULT OnAction(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    _Next(m_list.GetSelectedIndex());
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

  void _Next(size_t sel) {
    if( sel >= m_l->size() )
      return;
    *m_cur = &((*m_l)[sel]);
    EndDialog(ID_ACTION);
  }
};
