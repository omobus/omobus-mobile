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

#include <windows.h>
#include <omobus-mobile.h>

//--------------------------------------------------------------------------
// Установка именованного события
void SetNameEvent(LPCWSTR lpName) 
{
  HANDLE h = OpenEvent(EVENT_ALL_ACCESS, FALSE, lpName);
  if( h != NULL ) {
    SetEvent(h);
    CloseHandle(h);
  }
}

//--------------------------------------------------------------------------
