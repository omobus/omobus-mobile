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

void StopDaemonsHost() 
{
  SetNameEvent(OMOBUS_SRV_ONLINE);
}

bool DaemonsHostStarted() 
{
  HANDLE h = OpenEvent(EVENT_ALL_ACCESS, FALSE, OMOBUS_SRV_ONLINE);
  if( h != NULL ) {
    CloseHandle(h);
    return true;
  }
  return false;
}

void omobus_location(omobus_location_t *info)
{
  if( info == NULL ) {
    RETAILMSG(TRUE, (L"omobus_location: >> PANIC << Incorrect input parameter. info == NULL."));
    return;
  }

  memset(info, 0, sizeof(omobus_location_t));
  HANDLE h = OpenEvent(EVENT_ALL_ACCESS, FALSE, OMOBUS_GPS_POS_LOCK_NAME);
  if( h != NULL ) {
    HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 
      0, sizeof(omobus_location_t), OMOBUS_GPS_POS_SHMEM_NAME);
    if( hMap != NULL && GetLastError() == ERROR_ALREADY_EXISTS ) {
      PVOID pMap = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
      if( pMap != NULL ) {
        if( WaitForSingleObject(h, 1000) == WAIT_TIMEOUT ) {
          RETAILMSG(TRUE, (L"omobus_location: >> PANIC << Unable to lock shared region\n"));
        } else {
          memcpy(info, pMap, sizeof(omobus_location_t));
          SetEvent(h);
        }
        UnmapViewOfFile(pMap);
      }
    }
    if( hMap != NULL )
      CloseHandle(hMap);
    CloseHandle(h);
  }
}
