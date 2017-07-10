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

#include <windows.h>
#include <omobus-mobile.h>

//--------------------------------------------------------------------------
// Загрузка модуля локализации. На вход подается дескриптор
// модуля, для которого требуется загрузить модуль локализации.
HMODULE LoadMuiModule(HMODULE module, const wchar_t *postfix)
{
  if( postfix == NULL )
    return module;

  wchar_t tcBuf[1024+256];
	GetModuleFileName(module, tcBuf, 1024);
	lstrcat(tcBuf, _T("."));
  lstrcat(tcBuf, postfix);
	lstrcat(tcBuf, _T("."));
	lstrcat(tcBuf, _T("mui"));
	HMODULE ret = LoadLibrary(tcBuf);
  return ret == NULL ? module : ret;
}

//--------------------------------------------------------------------------
