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

#define DOCS_WAKEUP        L"docs->wakeup"
#define DOCS_WAKEUP_DEF    L"0"
#define DOCS_SRV_PATH      L"transport->docs->path"
#define DOCS_SRV_PATH_DEF  L"docs/"

typedef struct _tagMODULE {
  HANDLE hThrTr, hKill;
  int wakeup;
  const wchar_t *user_id, *caption, *srv_path, *pk_ext, *pk_mode;
  pf_daemons_pk_send send;
  uint32_t packets;
} HDAEMON;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIconOK = NULL, _hIconER = NULL;
static wchar_t wsBtnCap1[16] = L"Hide", wsBtnCap2[16] = L"Close";
// {44B0B21C-43F0-47cc-B9B1-78A331BC3292}
static const GUID CLSID_SHNAPI_OMOBUS_DOC = 
  { 0x44b0b21c, 0x43f0, 0x47cc, { 0xb9, 0xb1, 0x78, 0xa3, 0x31, 0xbc, 0x32, 0x92 } };

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
  sn.clsid = CLSID_SHNAPI_OMOBUS_DOC;
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
  SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_DOC, 1);
}

static
void InitAlertNotify(HDAEMON *h) {
  _InitNotify(h, IDS_DOC_ERR, _hIconER, SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|
    SHNF_CRITICAL);
}

static
void InitInfoNotify(HDAEMON *h) {
  _InitNotify(h, IDS_DOC_OK, _hIconOK, SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|
    SHNF_SILENT);
}

static
void StoreActivity(HDAEMON *h, int status) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_USERACT_DOC, true);
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
DWORD CALLBACK DocProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL || h->srv_path == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, TRUE, OMOBUS_DOCS_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_DOCS_NOTIFY));
    return 1;
  }

  bool exit = false;
  DWORD dwTimeout = INFINITE; // Таймаут
  while(!exit) {
    HANDLE hEv[] = {h->hKill, hNotify };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, dwTimeout) ) {
    case WAIT_OBJECT_0:
      exit = true; 
      break;
    case WAIT_OBJECT_0 + 1: {
        CleanupPopupNotify();
        h->packets = 0;
        // Отправка данных с подсчетом количества отправленных пакетов
        int rc = h->send(GetOmobusProfilePath(OMOBUS_PROFILE_DOCS), h->srv_path, &h->packets);
        if( h->packets > 0 || rc == OMOBUS_ERR )
          StoreActivity(h, rc);
        if( rc == OMOBUS_ERR ) 
          InitAlertNotify(h);
        else if( h->packets > 0 ) 
          InitInfoNotify(h);
        // Через 180 сек. уведомление принудительно удаляется
        dwTimeout = 180000;
        h->packets = 0;
      }
      SetTimeoutTrigger(OMOBUS_DOCS_NOTIFY, h->wakeup*60);
      ResetEvent(hNotify);
      break;
    case WAIT_TIMEOUT:
      CleanupPopupNotify();
      dwTimeout = INFINITE;
      break;
    default:
      RETAILMSG(TRUE, (L"DocProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_DOCS_NOTIFY);
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
  h->send = tr->pk_send;
  h->user_id = conf(OMOBUS_USER_ID, NULL);
  h->caption = conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  h->wakeup = _ttoi(conf(DOCS_WAKEUP, DOCS_WAKEUP_DEF));
  h->srv_path = conf(DOCS_SRV_PATH, DOCS_SRV_PATH_DEF);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  _hResInstance = LoadMuiModule(_hInstance, conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  wcsncpy(wsBtnCap1, wsfromRes(IDS_HIDE).c_str(), CALC_BUF_SIZE(wsBtnCap1));
  wcsncpy(wsBtnCap2, wsfromRes(IDS_CLOSE).c_str(), CALC_BUF_SIZE(wsBtnCap2));

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThrTr = CreateThread(NULL, 0, DocProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating DocProc thread.\n"));
    }
    CloseHandle(h->hThrTr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}
