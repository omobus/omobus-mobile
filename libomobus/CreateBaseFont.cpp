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

HFONT CreateBaseFont(LONG lfHeight, LONG lfWeight, BYTE lfItalic)
{
  LOGFONT lf;
  memset(&lf, 0, sizeof(lf));
  _tcscpy(lf.lfFaceName, _T("Tahoma"));
  lf.lfHeight = -1*lfHeight;
  lf.lfWeight = lfWeight;
  lf.lfItalic = lfItalic;
  return CreateFontIndirect(&lf);
}
