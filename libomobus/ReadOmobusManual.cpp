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

int ReadOmobusManual(const wchar_t *name, void *cookie, pf_opt_line_t par)
{
  int rc = -1;
  std::wstring fname = GetOmobusProfilePath(OMOBUS_PROFILE_SYNC), filter; 
  filter = fname; filter += name; filter += L"+*.*";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      fname += FindFileData.cFileName;
      rc = ReadOmobusPlainText(fname.c_str(), cookie, par);
      break;
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
  return rc;  
}
