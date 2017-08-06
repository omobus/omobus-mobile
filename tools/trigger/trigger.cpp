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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include <windows.h>
#include <tchar.h>

int _tmain(int argc, _TCHAR* argv[])
{
  TCHAR lpEventName[MAX_PATH+1] = _T("");
  for( int i = 0; i < argc; i++ )  {
    if( _tcsicmp(argv[i], _T("--event")) == 0 && argc > (i + 1) ) {
      _tcsncpy(lpEventName, argv[i+1], MAX_PATH);
      i++;
    }
  }

  if( lpEventName[0] == _T('\0') )
    return -1;

  HANDLE h = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpEventName);
  if( h != NULL ) {
    SetEvent(h);
    CloseHandle(h);
  }

	return 0;
}
