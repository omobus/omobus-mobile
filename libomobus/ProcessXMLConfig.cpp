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

#include <omobus-mobile.h>
#include <cfgmgrapi.h>

bool ProcessXMLConfigFile(const wchar_t *fname)
{
  if( fname == NULL || fname[0] == L'\0' ) {
    return false;
  }
  std::wstring cmd = L"--silent --file ";
  cmd += fname;
  return ExecRootProcess(L"wpxp.exe", cmd.c_str(), TRUE) == 0;
}
