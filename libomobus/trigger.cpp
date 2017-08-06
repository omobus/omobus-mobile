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
#include <Notify.h>

// -----------------------------------------------------------------------------
// Удаление триггера
static
void _DeleteTrigger(const wchar_t *app, const wchar_t *arg) {
  DWORD dwHowMany = 0;
  CeGetUserNotificationHandles(NULL, 0, &dwHowMany);
  if( dwHowMany <= 0 )
    return;
  HANDLE *hList = (HANDLE *) omobus_alloc(sizeof(HANDLE)*dwHowMany);
  if( hList == NULL )
    return;
  memset(hList, 0, sizeof(HANDLE)*dwHowMany);
  if( CeGetUserNotificationHandles(hList, dwHowMany, &dwHowMany) == TRUE ) {
    for( DWORD i = 0; i < dwHowMany; i++ ) {
      DWORD cBufferSize = 0;
      CeGetUserNotification(hList[i], 0, &cBufferSize, NULL);
      if( cBufferSize > 0 ) {
        BYTE *pBuffer = (BYTE *) omobus_alloc(sizeof(BYTE)*cBufferSize);
        if( pBuffer != NULL ) {
          memset(pBuffer, 0, sizeof(BYTE)*cBufferSize);
          if( CeGetUserNotification(hList[i], cBufferSize, &cBufferSize, pBuffer) == TRUE ) {
            CE_NOTIFICATION_INFO_HEADER *nih = (CE_NOTIFICATION_INFO_HEADER *)pBuffer;
            if( nih->pcent != NULL && 
                nih->pcent->lpszApplication != NULL &&
                wcsstri(nih->pcent->lpszApplication, app) != NULL &&
                (
                  arg == NULL || 
                    (
                      nih->pcent->lpszArguments != NULL && 
                      wcsstri(nih->pcent->lpszArguments, arg) != NULL
                    )
                ) ) {
              CeClearUserNotification(nih->hNotification);
            }
          }
          omobus_free(pBuffer);
        }
      }
    }
  }
  omobus_free(hList);
}

// -----------------------------------------------------------------------------
// Установка триггера на определенное время
int SetTimeoutTrigger(const wchar_t *name, uint32_t sec)
{
  if( name == NULL )
    return OMOBUS_ERR;

  _DeleteTrigger(name, NULL);

  if( sec <= 10 /*см. коммент к CeSetUserNotificationEx в msdn*/ )
    return OMOBUS_ERR;

  wchar_t evname[_MAX_PATH + 1] = L"";
  _snwprintf(evname, CALC_BUF_SIZE(evname), 
    L"\\\\.\\Notifications\\NamedEvents\\%s", name);
  time_t tt = omobus_time() + sec;
  tm lt = {0}; localtime_r(&tt, &lt);
  //if( sec > 120 ) // Для интервалов > 2 минут сбрасываем секунды
  //  lt.tm_sec = 0;

  CE_NOTIFICATION_TRIGGER trig;
  memset(&trig, 0, sizeof(CE_NOTIFICATION_TRIGGER));
  trig.dwSize = sizeof(CE_NOTIFICATION_TRIGGER);
  trig.dwType = CNT_TIME;
  trig.lpszApplication = evname;
  trig.lpszArguments = NULL;
  tm2st(&lt, &trig.stStartTime);
  HANDLE hNotify = CeSetUserNotificationEx(NULL, &trig, NULL);
  if( hNotify == NULL )
    return OMOBUS_ERR;
  
  CloseHandle(hNotify);
  DEBUGMSG(TRUE, (L"SetTimeoutTrigger: %s = %04i-%02i-%02i %02i:%02i:%02i\n",
    name, trig.stStartTime.wYear, trig.stStartTime.wMonth,
    trig.stStartTime.wDay, trig.stStartTime.wHour, 
    trig.stStartTime.wMinute, trig.stStartTime.wSecond));
  return OMOBUS_OK;
}

// -----------------------------------------------------------------------------
// Установка триггера на системное событие
int SetEventTrigger(const wchar_t *name, DWORD ev)
{
  if( name == NULL )
    return OMOBUS_ERR;

  _DeleteTrigger(name, NULL);

  wchar_t evname[_MAX_PATH + 1] = L"";
  _snwprintf(evname, CALC_BUF_SIZE(evname), 
    L"\\\\.\\Notifications\\NamedEvents\\%s", name);

  CE_NOTIFICATION_TRIGGER trig;
  memset(&trig, 0, sizeof(CE_NOTIFICATION_TRIGGER));
  trig.dwSize = sizeof(CE_NOTIFICATION_TRIGGER);
  trig.dwType = CNT_EVENT;
  trig.lpszApplication = evname;
  trig.lpszArguments = NULL;
  trig.dwEvent = ev;

  HANDLE hNotify = CeSetUserNotificationEx(NULL, &trig, NULL);
  if( hNotify == NULL )
    return OMOBUS_ERR;

  CloseHandle(hNotify);
  return OMOBUS_OK;
}

// -----------------------------------------------------------------------------
// Удаление ранее установленного триггера
void RemoveTrigger(const wchar_t *name)
{
  _DeleteTrigger(name, NULL);
}

// -----------------------------------------------------------------------------
