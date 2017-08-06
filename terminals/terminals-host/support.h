/* -*- H -*- */
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

#pragma once

struct SUPPORT_CONF {
  wchar_t descr[OMOBUS_MAX_DESCR+1];
  wchar_t phone[OMOBUS_MAX_DESCR+1];
  wchar_t mobile[OMOBUS_MAX_DESCR+1];
  wchar_t email[OMOBUS_MAX_DESCR+1];
  wchar_t working_hours[OMOBUS_MAX_DESCR+1];
  wchar_t role[OMOBUS_MAX_DESCR+1];
};
typedef std::vector<SUPPORT_CONF> supports_t;

class CSupportFrame : 
	public CStdDialogImpl<CSupportFrame>,
  public CWinDataExchange<CSupportFrame>,
	public CUpdateUI<CSupportFrame>,
	public CMessageFilter
{
protected:
  hsrv_t *m_hsrv;
  supports_t m_sup;
  int m_cur;
  CLabel m_label;
  CString m_pageno;

public:
  CSupportFrame(hsrv_t *hsrv) : m_hsrv(hsrv) {
    m_cur = -1;
  }

  ~CSupportFrame() {
  }

  // Format: %descr%;%phone%;%mobile%;%email%;%working_hours%;%role%;
  static 
  bool __support(void *cookie, int line, const wchar_t **argv, int count) 
  {
    SUPPORT_CONF cnf;
    if( line == 0 ) {
      return true;
    }
    if( cookie == NULL || count < 6 ) {
      return false;
    }
    memset(&cnf, 0, sizeof(SUPPORT_CONF));
    COPY_ATTR__S(cnf.descr, argv[0]);
    COPY_ATTR__S(cnf.phone, argv[1]);
    COPY_ATTR__S(cnf.mobile, argv[2]);
    COPY_ATTR__S(cnf.email, argv[3]);
    COPY_ATTR__S(cnf.working_hours, argv[4]);
    COPY_ATTR__S(cnf.role, argv[5]);
    ((supports_t *)cookie)->push_back(cnf);
    return true;
  }
  
	enum { IDD = IDD_SUPPORTDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_DDX_MAP(CSupportFrame)
    if( m_cur >= 0 ) {
      SUPPORT_CONF *cont = &m_sup[m_cur];
      DDX_TEXT(IDC_DESCR, cont->descr)
      DDX_TEXT(IDC_PHONE, cont->phone)
      DDX_TEXT(IDC_MOBILE, cont->mobile)
      DDX_TEXT(IDC_EMAIL, cont->email)
      DDX_TEXT(IDC_WORKING_HOURS, cont->working_hours)
      DDX_TEXT(IDC_ROLE, cont->role)
    }
	END_DDX_MAP()

	BEGIN_UPDATE_UI_MAP(CSupportFrame)
    UPDATE_ELEMENT(ID_SUPPORT_NEXT, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CSupportFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_GESTURE, OnGesture)
		COMMAND_ID_HANDLER(ID_EXIT, OnExit)
    COMMAND_ID_HANDLER(IDCANCEL, OnExit)
    COMMAND_ID_HANDLER(ID_SUPPORT_CANCEL, OnExit)
    COMMAND_ID_HANDLER(ID_SUPPORT_NEXT, OnChange)
		CHAIN_MSG_MAP(CUpdateUI<CSupportFrame>)
		CHAIN_MSG_MAP(CStdDialogImpl<CSupportFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, IDM_MENU_SUPPORT, SHCMBF_HIDESIPBUTTON));
    ReadOmobusManual(L"support", &m_sup, __support);
    ATLTRACE(L"support = %i\n", m_sup.size());
    if( !m_sup.empty() )
      m_cur = 0;
    if( !(m_sup.size() > 1) ) {
      UIEnable(ID_SUPPORT_NEXT, FALSE);
	    UIUpdateToolBar();
    }
    m_label.SubclassWindow(GetDlgItem(IDC_DESCR));
    m_label.SetFontBold(TRUE);
    SetDlgItemText(IDC_FIRM, m_hsrv->user_name);
    SetDlgItemText(IDC_USER, m_hsrv->firm_name);
    DoDataExchange(DDX_LOAD);
		return bHandled = FALSE;
	}

  LRESULT OnGesture(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    LRESULT rc = 0;
    GESTUREINFO gi;
    memset(&gi, 0, sizeof(gi));
    gi.cbSize = sizeof(GESTUREINFO);
    if( TKGetGestureInfo((HGESTUREINFO)lParam, &gi) ) {
      if( wParam == GID_PAN ) {
        if( gi.dwFlags & (GF_END|GF_INERTIA) ) {
          if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_RIGHT ) {
            Next();
          } else if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_LEFT ) {
            Next(-1);
          }
          bHandled = TRUE;
          rc = 1;
        }
      }
    }
    return rc;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

  LRESULT OnChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    Next();
    return 0;
  }

  const wchar_t *GetSubTitle() {
    m_pageno = L"";
    if( m_sup.size() > 1 ) {
      m_pageno.Format(L"%i/%i", m_cur + 1, m_sup.size());
    }
    return m_pageno.GetString();
  }

private:
  void Next(int step=1) {
    RECT rTitle;
    size_t size;
    if( m_sup.empty() ) {
      m_cur = -1;
      return;
    } else {
      size = m_sup.size();
      if( size == 1 ) {
        m_cur = 0;
        return;
      }
      m_cur += step;
      if( m_cur < 0 ) {
        m_cur = size - 1;
      }
      if( m_cur >= size )
        m_cur = 0;
    }
    ::GetClientRect(m_hWnd, &rTitle);
    rTitle.bottom = nTitleHeight;
    DoDataExchange(DDX_LOAD);
    RedrawWindow(&rTitle);
  }
};
