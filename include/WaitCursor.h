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

class WaitCursor
{
protected:
	HCURSOR m_hWaitCursor;
	HCURSOR m_hOldCursor;
	bool m_bInUse;

public:
	WaitCursor(HINSTANCE hInstance, bool bSet = true, LPCTSTR lpstrCursor = IDC_WAIT, 
    bool bSys = true) : m_hOldCursor(NULL), m_bInUse(false) {
		m_hWaitCursor = ::LoadCursor(hInstance, lpstrCursor);
		if(bSet)
			Set();
	}

	~WaitCursor() {
		Restore();
	}

	bool Set() {
		if(m_bInUse)
			return false;
		m_hOldCursor = ::SetCursor(m_hWaitCursor);
		m_bInUse = true;
		return true;
	}

	bool Restore() {
		if(!m_bInUse)
			return false;
		::SetCursor(m_hOldCursor);
		m_bInUse = false;
		return true;
	}
};
