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

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>, 
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, 
  public CIdleHandler
{
private:
  hsrv_t *m_hsrv;
  HWND m_cur_el;

public:
  CMainFrame(hsrv_t *hsrv) : m_hsrv(hsrv) {
    m_cur_el = NULL;
  }

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE; 
		return FALSE;
	}

  virtual BOOL OnIdle() {
    UIEnable(ID_ACTION, FindCurPos() != -1);
		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
    UPDATE_ELEMENT(ID_ACTION, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(OMOBUS_SSM_PLUGIN_SELECT, OnSelectPlugin)
    MESSAGE_HANDLER(OMOBUS_SSM_PLUGIN_RELOAD, OnReloadPlugin)
		COMMAND_ID_HANDLER(ID_ACTION, OnAction)
		COMMAND_ID_HANDLER(ID_MENU_SUPPORT, OnSupport)
		COMMAND_ID_HANDLER(ID_MENU_ABOUT, OnAbout)
		COMMAND_ID_HANDLER(ID_MENU_UPD, OnUpd)
		COMMAND_ID_HANDLER(ID_MENU_SYNC, OnSync)
		COMMAND_ID_HANDLER(ID_MENU_ACT, OnAct)
		COMMAND_ID_HANDLER(ID_MENU_REBOOT, OnReboot)
		COMMAND_ID_HANDLER(ID_MENU_SUSPEND, OnSuspend)
		COMMAND_ID_HANDLER(ID_MENU_RECONFIG, OnReconf)
		COMMAND_ID_HANDLER(ID_MENU_NETMGR, OnNetMgr)
		COMMAND_ID_HANDLER(ID_EXIT, OnExit)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)	{
		CreateSimpleCEMenuBar(ATL_IDW_MENU_BAR, SHCMBF_HIDESIPBUTTON);
		UIAddToolBar(m_hWndCECommandBar);
    SetWindowText(m_hsrv->caption);
		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);
    OnIdle();
		return 0;
	}

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = TRUE;
    CDCHandle dc((HDC)wParam);
		RECT rc; GetClientRect(&rc);
    dc.FillRect(&rc, COLOR_WINDOW);
    HPEN hDash = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR),
         hOldPen =  dc.SelectPen(hDash);
    int top = 0, width = GetSystemMetrics(SM_CXSCREEN);
    for( size_t i = 0; i < m_hsrv->elcur->size(); i++ ) {
      top += (*m_hsrv->elcur)[i].nHeight;
      dc.MoveTo(0, top); dc.LineTo(width, top);
      top += DRA::SCALEY(1);
    }
    dc.SelectPen(hOldPen);
    DeleteObject(hDash);
		return 1;
  }

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    int cur_pos = FindCurPos(), old = cur_pos;
    if( wParam == VK_UP ) {
      if( cur_pos != -1 )
        cur_pos -= 1;
    } else if( wParam == VK_DOWN ) {
      cur_pos++;
      if( m_hsrv->elcur->size() <= cur_pos )
        cur_pos = m_hsrv->elcur->size() - 1;
    } else if( wParam == VK_RETURN ) {
      if( cur_pos != -1 )
        CWindow((*m_hsrv->elcur)[cur_pos].hWnd).SendMessage(OMOBUS_SSM_PLUGIN_ACTION);
    }
    if( old == cur_pos )
      return 0;
    if( old != -1 )
      CWindow((*m_hsrv->elcur)[old].hWnd).SendMessage(OMOBUS_SSM_PLUGIN_SELECT, FALSE, 0);
    if( cur_pos != -1 ) {
      CWindow((*m_hsrv->elcur)[cur_pos].hWnd).SendMessage(OMOBUS_SSM_PLUGIN_SELECT, TRUE, 0);
      m_cur_el = (*m_hsrv->elcur)[cur_pos].hWnd;
    } else {
      m_cur_el = NULL;
    }
    OnIdle();
		return 0;
  }

	LRESULT OnSelectPlugin(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    for( size_t i = 0; i < m_hsrv->elcur->size(); i++ ) {
      if( (*m_hsrv->elcur)[i].hWnd == (HWND)lParam ) {
        if( m_cur_el != NULL )
          CWindow(m_cur_el).SendMessage(OMOBUS_SSM_PLUGIN_SELECT, FALSE, 0);
        m_cur_el = (*m_hsrv->elcur)[i].hWnd;
        CWindow(m_cur_el).SendMessage(OMOBUS_SSM_PLUGIN_SELECT, TRUE, 0);
        break;
      }
    }
    OnIdle();
		return 0;
  }

	LRESULT OnReloadPlugin(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    for( size_t i = 0; i < m_hsrv->elcur->size(); i++ ) {
      CWindow((*m_hsrv->elcur)[i].hWnd).SendMessage(OMOBUS_SSM_PLUGIN_RELOAD);
    }
		return 0;
  }

	LRESULT OnExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ShowWindow(SW_HIDE);
		return 0;
	}

	LRESULT OnAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CAboutFrame().DoModal(m_hWnd);
    return 0;
	}

	LRESULT OnSupport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CSupportFrame(m_hsrv).DoModal(m_hWnd);
    return 0;
	}
  
  LRESULT OnUpd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_UPD, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES )
      SetNameEvent(OMOBUS_UPD_NOTIFY);
    return 0;
  }
  
  LRESULT OnAct(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_ACT, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
      SetNameEvent(OMOBUS_ACTS_NOTIFY);
      SetNameEvent(OMOBUS_DOCS_NOTIFY);
    }
    return 0;
  }
  
  LRESULT OnSync(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_SYNC, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES )
      SetNameEvent(OMOBUS_SYNC_NOTIFY);
    return 0;
  }
  
  LRESULT OnReboot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ExecRootProcess(L"reboot.exe", NULL, FALSE);
    return 0;
  }
  
  LRESULT OnSuspend(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    ExecRootProcess(L"suspend.exe", NULL, FALSE);
    return 0;
  }
  
  LRESULT OnReconf(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( MessageBoxEx(m_hWnd, IDS_RECONF, IDS_INFORM, MB_YESNO|MB_ICONQUESTION) == IDYES )
      SetNameEvent(OMOBUS_RECONF_NOTIFY);
    return 0;
  }
  
  LRESULT OnNetMgr(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CWaitCursor _wc;
    ExecProcess(m_hsrv->netmgr, NULL, FALSE);
    return 0;
  }

	LRESULT OnAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( m_cur_el != NULL )
      CWindow(m_cur_el).SendMessage(OMOBUS_SSM_PLUGIN_ACTION);
		return 0;
	}

protected:
  int FindCurPos() {
    int cur_pos = -1;
    if( m_hsrv == NULL || m_hsrv->elcur == NULL || m_cur_el == NULL )
      return cur_pos;
    for( int i = 0; i < m_hsrv->elcur->size(); i++ ) {
      if( m_cur_el == (*m_hsrv->elcur)[i].hWnd ) {
        cur_pos = i;
        break;
      }
    }
    return cur_pos;
  }
};

