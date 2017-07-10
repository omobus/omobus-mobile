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
#include <notify.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

typedef struct _tagMODULE {
  HANDLE hThr, hKill;
  const wchar_t *user_id, *pk_ext, *pk_mode;
} HDAEMON;

static HINSTANCE _hInstance = NULL;

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(_hInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void StoreSyncEnd(HDAEMON *h) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_SYSACT_PC_SYNC, true);
  if( xml.empty() )
    return;
  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_PC_SYNC, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static 
DWORD CALLBACK PcSyncProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  CEnsureCloseHandle hNotifySync = CreateEvent(NULL, TRUE, FALSE, OMOBUS_PC_SYNC_NOTIFY);
  if( hNotifySync.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_PC_SYNC_NOTIFY));
    return 1;
  }

  SetEventTrigger(OMOBUS_PC_SYNC_NOTIFY, NOTIFICATION_EVENT_SYNC_END);

  bool exit = false;
  while( !exit ) {
    HANDLE hEv[] = {h->hKill, hNotifySync};
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) ) {
    case WAIT_OBJECT_0:
      exit = true;
      break;
    case WAIT_OBJECT_0 + 1:
      StoreSyncEnd(h);
      // “.к. событие окончани€ синхронизации приходит 2 раза, что нам не нужно,
      // пытаемс€ использовать только первое событие. ƒл€ этого используем сбрасываемое
      // вручную событи и ожидание в 5 сек. перед его сбросом.
      if( WaitForSingleObject(h->hKill, 5000) == WAIT_TIMEOUT )
        ResetEvent(hNotifySync);
      break;
    default:
      RETAILMSG(TRUE, (L"PcSyncProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_PC_SYNC_NOTIFY);

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

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThr = CreateThread(NULL, 0, PcSyncProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating PcSyncProc thread.\n"));
    }
    CloseHandle(h->hThr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}
