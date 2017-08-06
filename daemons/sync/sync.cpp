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
#include <notify.h>
#include <aygshell.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

#define SYNC_WAKEUP        L"sync->wakeup"
#define SYNC_WAKEUP_DEF    L"0"
#define SYNC_SRV_PATH      L"transport->sync->path"
#define SYNC_SRV_PATH_DEF  L"sync/"

typedef struct _tagMODULE {
  HANDLE hThrTr, hKill;
  int wakeup;
  const wchar_t *user_id, *caption, *srv_path, *pk_ext, *pk_mode, *mui;
  pf_daemons_pk_recv recv;
  uint32_t packets;
} HDAEMON;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIconOK = NULL, _hIconER = NULL;
static wchar_t wsBtnCap1[16] = L"Hide", wsBtnCap2[16] = L"Close";
// {416F0CB2-D048-4d10-AA6E-BD6616965247}
static const GUID CLSID_SHNAPI_OMOBUS_SYNC = 
  { 0x416f0cb2, 0xd048, 0x4d10, { 0xaa, 0x6e, 0xbd, 0x66, 0x16, 0x96, 0x52, 0x47 } };

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void _InitNotify(HDAEMON *h, UINT uID, HICON hIcon, DWORD grfFlags) {
  std::wstring html = wsfromRes(uID);
  wsrepl(html, L"%packets%", itows(h->packets).c_str());
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1;
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = hIcon;
  sn.clsid = CLSID_SHNAPI_OMOBUS_SYNC;
  sn.grfFlags = grfFlags;
  sn.pszTitle = h->caption;
  sn.pszHTML = html.c_str();
  sn.rgskn[0].pszTitle = wsBtnCap1;
  //sn.rgskn[0].skc.wpCmd = 100;
  sn.rgskn[1].pszTitle = wsBtnCap2;
  sn.rgskn[1].skc.wpCmd = 101;
  SHNotificationAdd(&sn);
}

void CleanupPopupNotify() {
  SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_SYNC, 1);
}

inline 
void InitAlertNotify(HDAEMON *h) {
  _InitNotify(h, IDS_SYNC_ERR, _hIconER, SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|
    SHNF_CRITICAL);
}

static
void InitInfoNotify(HDAEMON *h) {
  _InitNotify(h, IDS_SYNC_OK, _hIconOK, SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|
    SHNF_SILENT);
}

static
void StoreActivity(HDAEMON *h, int status) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_USERACT_SYNC, true);
  if( xml.empty() )
    return;
  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%status%", status==OMOBUS_OK?L"success":L"failed");
  wsrepl(xml, L"%packets%", itows(h->packets).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_EXCHANGE, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static
void _write_last_sync() {
  std::wstring dt = wsftime(omobus_time(), ISO_DATETIME_FMT);
  std::wstring fname = GetOmobusProfilePath(OMOBUS_PROFILE_CACHE); 
  fname += OMOBUS_LAST_SYNC_J;
  FILE *f = _wfopen(fname.c_str(), L"wb");
  if( f != NULL ) {
    fwrite(dt.c_str(), sizeof(wchar_t), dt.size(), f);
    fclose(f);
  }
}

static
bool _is_last_sync_exist() {
  std::wstring fname = GetOmobusProfilePath(OMOBUS_PROFILE_CACHE); 
  fname += OMOBUS_LAST_SYNC_J;
  return wcsfexist(fname.c_str());
}

static 
DWORD CALLBACK SyncProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL || h->srv_path == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, TRUE, OMOBUS_SYNC_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_SYNC_NOTIFY));
    return 1;
  }

  bool exit = false;
  DWORD dwTimeout = INFINITE;
  while(!exit) {
    HANDLE hEv[] = {h->hKill, hNotify };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, dwTimeout) ) {
    case WAIT_OBJECT_0:
      exit = true; 
      break;
    case WAIT_OBJECT_0 + 1: {
        CleanupPopupNotify();
        h->packets = 0;
        int rc = h->recv(h->srv_path, GetOmobusProfilePath(OMOBUS_PROFILE_SYNC), &h->packets);
        // Сохранение даты получения пакета
        if( (rc == OMOBUS_OK && h->packets > 0) || !_is_last_sync_exist() ) {
          _write_last_sync();
        }
        if( h->packets > 0 || rc == OMOBUS_ERR ) {
          StoreActivity(h, rc);
        }
        if( rc != OMOBUS_OK )  {
          InitAlertNotify(h);
        } else if( h->packets > 0 ) {
          ReloadTerminalsData();
          InitInfoNotify(h);
        }
        // Через 180 сек. уведомление принудительно удаляется
        dwTimeout = 180000;
        h->packets = 0;
      }
      SetTimeoutTrigger(OMOBUS_SYNC_NOTIFY, h->wakeup*60);
      ResetEvent(hNotify);
      break;
    case WAIT_TIMEOUT:
      CleanupPopupNotify();
      dwTimeout = INFINITE;
      break;
    default:
      RETAILMSG(TRUE, (L"SyncProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_SYNC_NOTIFY);
  CleanupPopupNotify();

  return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    _hIconER = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_TRAY_ER));
    _hIconOK = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_TRAY_OK));
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
    _hResInstance = NULL;
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
  h->recv = tr->pk_recv;
  h->user_id = conf(OMOBUS_USER_ID, NULL);
  h->caption = conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  h->wakeup = _ttoi(conf(SYNC_WAKEUP, SYNC_WAKEUP_DEF));
  h->srv_path = conf(SYNC_SRV_PATH, SYNC_SRV_PATH_DEF);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  _hResInstance = LoadMuiModule(_hInstance, conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  wcsncpy(wsBtnCap1, wsfromRes(IDS_HIDE).c_str(), CALC_BUF_SIZE(wsBtnCap1));
  wcsncpy(wsBtnCap2, wsfromRes(IDS_CLOSE).c_str(), CALC_BUF_SIZE(wsBtnCap2));

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThrTr = CreateThread(NULL, 0, SyncProc, h, 0, NULL);

  return h;
}

void stop_daemon(void *_h) 
{
  HDAEMON *h = (HDAEMON *)_h;
  if( h == NULL )
    return;
  if( h->hKill != NULL )
    SetEvent(h->hKill);
  if( h->hThrTr != NULL ) {
    if( WaitForSingleObject(h->hThrTr, 5000) == WAIT_TIMEOUT ) {
      TerminateThread(h->hThrTr, -1);
      RETAILMSG(TRUE, (L"Terminating SyncProc thread.\n"));
    }
    CloseHandle(h->hThrTr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}
