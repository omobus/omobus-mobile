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

class CAboutFrame : 
	public CStdDialogImpl<CAboutFrame>,
	public CMessageFilter
{
public:
  CAboutFrame() {
  }

  ~CAboutFrame() {
  }

	enum { IDD = IDD_ABOUTDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP(CAboutFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_EXIT, OnExit)
    COMMAND_ID_HANDLER(IDCANCEL, OnExit)
    COMMAND_ID_HANDLER(ID_ABOUT_CANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogImpl<CAboutFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
    AtlCreateMenuBar(m_hWnd, IDM_MENU_ABOUT, SHCMBF_HIDESIPBUTTON);
    CString templ, version;
    GetDlgItemText(IDC_VERSION, templ);
    version.Format(templ, OMOBUS_VERSION_MAJOR, OMOBUS_VERSION_MINOR, 
      OMOBUS_VERSION_REV, OMOBUS_VERSION_BUILD);
    SetDlgItemText(IDC_VERSION, version);
		return bHandled = FALSE;
	}

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }
};

