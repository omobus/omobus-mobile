/* -*- C++ -*- */
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

#include <omobus-mobile.h>

static LONG _pkNum = 0;

int WriteOmobusXmlPk(profile_path_t pp, 
  const wchar_t *name, const wchar_t *user_id, 
  const wchar_t *pk_ext, const wchar_t *pk_mode, 
  const wchar_t *xml)
{
  int rc = OMOBUS_ERR;
  wchar_t pkn[1024] = L"", pknt[1024] = L"", mode[16] = L"";
  LONG pknum = 0;
  size_t size = 0;

  if( name == NULL || user_id == NULL || xml == NULL )
    return rc;

  _snwprintf(mode, CALC_BUF_SIZE(mode), L"wb%s", pk_mode == NULL ? L"" : pk_mode);
  pknum = InterlockedIncrement(&_pkNum);
  size = wcslen(xml)*sizeof(wchar_t);

  for( int step = 0; step < 2/*0 - gx/bz2/xml, 1 - xml*/; step++ ) {
    _snwprintf(pkn, CALC_BUF_SIZE(pkn), L"%s%s.%s+%s+%s+%X.%s",
      GetOmobusProfilePath(pp), name, user_id, 
      wsftime(omobus_time(), FILE_DATETIME_FMT).c_str(), 
      GetOmobusId(), pknum,
      pk_ext == NULL || pk_ext[0] == L'\0' ? L"xml" : pk_ext);
    _snwprintf(pknt, CALC_BUF_SIZE(pknt), L"%s.%s+%X.%s",
      GetOmobusProfilePath(pp), name, pknum,
      pk_ext == NULL || pk_ext[0] == L'\0' ? L"xml" : pk_ext);

    pk_file_t f = pkopen(w2a(pknt).c_str(), w2a(mode).c_str());
    if( f != NULL ) {
      if( pkwrite(f, xml, size) == size )
        rc = OMOBUS_OK;
      pkclose(f); 
      f = NULL;
    }
    if( rc == OMOBUS_ERR ) {
      pk_ext = NULL;
      DeleteFile(pknt);
      continue;
    }
    MoveFile(pknt, pkn);
    break;
  }

  return rc;
}
