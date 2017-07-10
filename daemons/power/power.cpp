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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include <windows.h>
#include <notify.h>
#include <Msgqueue.h>
#include <Pm.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

#define OMOBUS_POWER_WAKEUP      L"power->wakeup"
#define OMOBUS_POWER_WAKEUP_DEF  L"0"

static HINSTANCE _hInstance = NULL;
static const wchar_t *_days_abbrev[] = {
  L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };


typedef struct _tagMODULE {
  HANDLE hThr, hKill;
  const wchar_t *user_id, *pk_ext, *pk_mode,
    *working_hours, *working_days;
  int wakeup;
} HDAEMON;

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(_hInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
bool IsWorkingTime(const wchar_t *working_days, const wchar_t *working_time, 
  time_t tt) {
  tm lt = {0}; localtime_r(&tt, &lt);
  // Используем фильтр по дням
  if( !(working_days == NULL || working_days[0] == L'\0') &&
      lt.tm_wday <= 6 &&
      wcsstri(working_days, _days_abbrev[lt.tm_wday]) == NULL )
    return false;
  // Используем фильтр по времени
  if( !(working_time == NULL || wcslen(working_time) != 11) ) {
    // Вытаскиваем время начала и завершения рабочего дня. Причем должнеы
    // быть заданы оба значения. 
    wchar_t buf[6] = L""; _snwprintf(buf, 5, L"%02i:%02i", lt.tm_hour, lt.tm_min);
    if( !(wcsncmp(buf, &working_time[0], 5) > 0 && 
          wcsncmp(buf, &working_time[6], 5) < 0) )
      return false;
  }
  return true;
}

static
void StoreAcPower(HDAEMON *h, BYTE bACLineStatus, BYTE bBatteryLifePercent) 
{
  std::wstring xml;
  omobus_location_t pos;

  if( h->user_id == NULL ) {
    return;
  }

  xml = wsfromRes(IDS_SYSACT_POWER, true);
  if( xml.empty() ) {
    return;
  }

  omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%state%", bACLineStatus == AC_LINE_ONLINE ? L"on" : 
    (bACLineStatus != AC_LINE_UNKNOWN ? L"off" : L"unknown"));
  wsrepl(xml, L"%battery_life%", b2ws(bBatteryLifePercent).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_POWER, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static 
DWORD CALLBACK PowerProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, FALSE, OMOBUS_POWER_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_POWER_NOTIFY));
    return 1;
  }

  DWORD cbPowerMsgSize = sizeof POWER_BROADCAST + (MAX_PATH * sizeof TCHAR);
  MSGQUEUEOPTIONS mqo;
  mqo.dwSize = sizeof(MSGQUEUEOPTIONS);
  mqo.dwFlags = MSGQUEUE_NOPRECOMMIT;
  mqo.dwMaxMessages = 4;
  mqo.cbMaxMessage = cbPowerMsgSize;
  mqo.bReadAccess = TRUE;

  CEnsureCloseMsgQueue hPowerMsgQ = CreateMsgQueue(NULL, &mqo);
  if( hPowerMsgQ.IsInvalid() ) {
    RETAILMSG(TRUE, (L"PowerProc: Unable to create a message queue!\n"));
    return 1;
  }

  CEnsureStopPowerNotificationse hPowerNotifications = 
    RequestPowerNotifications(hPowerMsgQ, PBT_POWERINFOCHANGE);
  if( hPowerNotifications.IsInvalid() ) {
    RETAILMSG(TRUE, (L"PowerProc: Unable to request power notifications!\n"));
    return 1;
  }

  POWER_BROADCAST *ppb = (POWER_BROADCAST*) omobus_alloc(cbPowerMsgSize);
  if( ppb == NULL ) {
    RETAILMSG(TRUE, (L"PowerProc: Unable to allocate buffer!\n"));
    return 1;
  }

  SetTimeoutTrigger(OMOBUS_POWER_NOTIFY, h->wakeup*60);

  byte_t ac_state = AC_LINE_UNKNOWN;
  bool exit = false;
  while( !exit ) {
    HANDLE hEv[] = {h->hKill, hNotify, hPowerMsgQ};
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) ) {
    case WAIT_OBJECT_0:
      exit = true;
      break;
    case WAIT_OBJECT_0 + 1: 
      SetTimeoutTrigger(OMOBUS_POWER_NOTIFY, h->wakeup*60);
      if( IsWorkingTime(h->working_days, h->working_hours, omobus_time()) ) {
        SYSTEM_POWER_STATUS_EX sps;
        memset(&sps, 0, sizeof(SYSTEM_POWER_STATUS_EX));
        if( GetSystemPowerStatusEx(&sps, FALSE) == FALSE ) {
          sps.ACLineStatus = AC_LINE_UNKNOWN;
        }
        StoreAcPower(h, sps.ACLineStatus, sps.BatteryLifePercent);
        ac_state = sps.ACLineStatus;
      }
      ResetEvent(hNotify);
      break;
    case WAIT_OBJECT_0 + 2: {
        DWORD cbRead = 0, dwFlags = 0;
        while(ReadMsgQueue(hPowerMsgQ, ppb, cbPowerMsgSize, &cbRead, 0, &dwFlags)) {
          if( ppb->Message == PBT_POWERINFOCHANGE ) {
            // PBT_POWERINFOCHANGE message embeds a POWER_BROADCAST_POWER_INFO 
            // structure into the SystemPowerState field
            PPOWER_BROADCAST_POWER_INFO ppbpi = (PPOWER_BROADCAST_POWER_INFO) ppb->SystemPowerState;
            if( ppbpi && ppbpi->bACLineStatus != AC_LINE_UNKNOWN && 
                ppbpi->bBatteryLifePercent != BATTERY_FLAG_UNKNOWN ) {
              if( ac_state != ppbpi->bACLineStatus ) {
                StoreAcPower(h, ppbpi->bACLineStatus, ppbpi->bBatteryLifePercent);
                ac_state = ppbpi->bACLineStatus;
              }
            }
          }
        }
      }
      break;
    default:
      RETAILMSG(TRUE, (L"PowerProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_POWER_NOTIFY);
  chk_omobus_free(ppb);

  return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

void *start_daemon(pf_daemons_conf conf, daemons_transport_t *tr) 
{
  HDAEMON *h = new HDAEMON;
  if( h == NULL )
    return NULL;
  memset(h, 0, sizeof(HDAEMON));
  h->user_id = conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->wakeup = _ttoi(conf(OMOBUS_POWER_WAKEUP, OMOBUS_POWER_WAKEUP_DEF));
  h->working_hours = conf(OMOBUS_WORKING_HOURS, NULL);
  h->working_days = conf(OMOBUS_WORKING_DAYS, NULL);

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThr = CreateThread(NULL, 0, PowerProc, h, 0, NULL);

  return h;
}

void stop_daemon(void *_h) 
{
  HDAEMON *h = (HDAEMON *)_h;
  if( h == NULL )
    return;
  if( h->hKill != NULL )
    SetEvent(h->hKill);
  if( h->hThr != NULL ) {
    if( WaitForSingleObject(h->hThr, 5000) == WAIT_TIMEOUT ) {
      TerminateThread(h->hThr, -1);
      RETAILMSG(TRUE, (L"Terminating PowerProc thread.\n"));
    }
    CloseHandle(h->hThr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}
