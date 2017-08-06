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
#include <regext.h>
#include <notify.h>
#include <aygshell.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

typedef struct _tagMODULE {
  HREGNOTIFY rn[20];
  const wchar_t *user_id, *caption, *pk_ext, *pk_mode;
} HDAEMON;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIcon = NULL;
static wchar_t wsBtnCap1[16] = L"Hide", wsBtnCap2[16] = L"Close";
// {A50115F0-F172-4f22-A71D-1F57615600C8}
static const GUID CLSID_SHNAPI_OMOBUS_GPSCONF = 
{ 0xa50115f0, 0xf172, 0x4f22, { 0xa7, 0x1d, 0x1f, 0x57, 0x61, 0x56, 0x0, 0xc8 } };


static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void StoreGpsDevState(HDAEMON *h) 
{
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_GPSMON_ACT_GPS_STATE, true);
  if( xml.empty() )
    return;
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_GPS_STATE, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static
void InitWarningNotify(HDAEMON *h) {
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1;
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = _hIcon;
  sn.clsid = CLSID_SHNAPI_OMOBUS_GPSCONF;
  sn.grfFlags = SHNF_TITLETIME|SHNF_DISPLAYON|SHNF_CRITICAL;
  sn.pszTitle = h->caption;
  sn.pszHTML = wsfromRes(IDS_GPS_RECONFIG).c_str();
  sn.rgskn[0].pszTitle = wsBtnCap1;
  //sn.rgskn[0].skc.wpCmd = 100;
  sn.rgskn[1].pszTitle = wsBtnCap2;
  sn.rgskn[1].skc.wpCmd = 101;
  SHNotificationAdd(&sn);
}

void CleanupPopupNotify() {
  SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_GPSCONF, 1);
}

static
void AddRegNotify(HDAEMON *h, HREGNOTIFY hrn) {
  int len = sizeof(h->rn)/sizeof(h->rn[0]);
  for( int i = 0; i < len; i++ ) {
    if( h->rn[i] == NULL )
      h->rn[i] = hrn;
  }
}

static
void ClearRegNotify(HDAEMON *h) {
  int len = sizeof(h->rn)/sizeof(h->rn[0]);
  for( int i = 0; i < len; i++ ) {
    if( h->rn[i] != NULL ) {
      RegistryCloseNotification(h->rn[i]);
      h->rn[i] = NULL;
    }
  }
}

static
void AdjustGPSConf(HREGNOTIFY /*hNotify*/, DWORD dwUserData, 
  const PBYTE /*pData*/, const UINT /*cbData*/) {
  HDAEMON *h = (HDAEMON *)dwUserData;
  if( h != NULL ) {
    StoreGpsDevState(h);
    InitWarningNotify(h);
  }
  RETAILMSG(TRUE, (L"gpsconf: GPS settings have been changed!\n"));
}

static
void SetRegNotifyDWORD(HDAEMON *h, wchar_t *wcRootKey, wchar_t *wcValName, 
  DWORD *dwVal) 
{
  HKEY hKey = NULL;
  HREGNOTIFY hNotify = NULL;
  NOTIFICATIONCONDITION nc = {REG_CT_NOT_EQUAL, 1, NULL};

  if( !(RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcRootKey, 0, 0, &hKey) 
          != ERROR_SUCCESS || hKey == NULL) ) {
    RegistryGetDWORD(hKey, NULL, wcValName, dwVal);
    nc.TargetValue.dw = *dwVal;
    if( RegistryNotifyCallback(hKey, NULL, wcValName, 
          AdjustGPSConf, (DWORD)h, &nc, &hNotify) != S_OK ) {
      RETAILMSG(TRUE, (L"gpsconf: Unable to notify changes for the HKLM\\%s -> %s",
        wcRootKey, wcValName));
    }
  }

  RegCloseKey(hKey);
  AddRegNotify(h, hNotify);
}

static
void SetRegNotifyString(HDAEMON *h, wchar_t *wcRootKey, wchar_t *wcValName, 
  wchar_t *wcVal, UINT cchData) 
{
  HKEY hKey = NULL;
  HREGNOTIFY hNotify = NULL;
  NOTIFICATIONCONDITION nc = {REG_CT_NOT_EQUAL, 0, NULL};

  if( !(RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcRootKey, 0, 0, &hKey) 
          != ERROR_SUCCESS || hKey == NULL) ) {
    if( RegistryGetString(hKey, NULL, wcValName, wcVal, cchData) == S_OK ||
            wcVal[0] != L'\0' ) {
      nc.TargetValue.psz = wcVal;
      if( RegistryNotifyCallback(hKey, NULL, wcValName, 
            AdjustGPSConf, (DWORD)h, &nc, &hNotify) != S_OK ) {
        RETAILMSG(TRUE, (L"gpsconf: Unable to notify changes for the HKLM\\%s%s%s -> %s",
          wcRootKey, wcValName));
      }
    }
  }

  RegCloseKey(hKey);
  AddRegNotify(h, hNotify);
}

static
void SetRegNotifyKey(HDAEMON *h, wchar_t *wcRootKey, wchar_t *wcKeyName) 
{
  HKEY hKey = NULL, hKey2 = NULL;
  HREGNOTIFY hNotify = NULL;
  NOTIFICATIONCONDITION nc;
  wchar_t wcValName[255];
  BYTE bData[1024];
  DWORD dwIndex = 0, cchValName = 0, dwType = 0, cbData = 0;

  if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcRootKey, 0, 0, &hKey2) 
        != ERROR_SUCCESS || hKey2 == NULL )
    return;
  
  RegOpenKeyEx(hKey2, wcKeyName, 0, 0, &hKey);
  RegCloseKey(hKey2); hKey2 = NULL;
  if( hKey == NULL )
    return;
  
  while( 1 ) {
    cchValName = CALC_BUF_SIZE(wcValName);
    cbData = CALC_BUF_SIZE(bData);
    memset(wcValName, 0, sizeof(wcValName));
    memset(bData, 0, sizeof(bData));
    memset(&nc, 0, sizeof(nc));

    if( RegEnumValue(hKey, dwIndex, wcValName, &cchValName, 0,
          &dwType, bData, &cbData) != ERROR_SUCCESS )
      break;

    nc.ctComparisonType = REG_CT_NOT_EQUAL;
    if( REG_DWORD == dwType ) {
      nc.TargetValue.dw = *((DWORD *)bData);
      nc.dwMask = -1;
    } else if( REG_SZ == dwType ) {
      nc.TargetValue.psz = (LPTSTR)bData;
      nc.dwMask = 0;
    } else
      continue;
    hNotify = NULL;
    if( RegistryNotifyCallback(hKey, NULL, wcValName, 
          AdjustGPSConf, (DWORD)h, &nc, &hNotify) != S_OK ) {
      RETAILMSG(TRUE, (L"gpsconf: Unable to notify changes for the HKLM\\%s\\%s -> %s",
        wcRootKey, wcKeyName, wcValName));
    }
    AddRegNotify(h, hNotify);
    dwIndex++;
  }

  RegCloseKey(hKey);
}

static
void RegisterGPSConfTrigger(HDAEMON *h) 
{
  DWORD IsEnabled = 1;
  wchar_t CurrentDriver[255] = L"";
  
  SetRegNotifyDWORD(h, 
    L"System\\CurrentControlSet\\GPS Intermediate Driver",
    L"IsEnabled", &IsEnabled);

  SetRegNotifyKey(h, 
    L"System\\CurrentControlSet\\GPS Intermediate Driver",
    L"Multiplexer");

  SetRegNotifyString(h, 
    L"System\\CurrentControlSet\\GPS Intermediate Driver\\Drivers",
    L"CurrentDriver", CurrentDriver, CALC_BUF_SIZE(CurrentDriver));

  if( CurrentDriver[0] != L'\0' ) {
    SetRegNotifyKey(h, 
      L"System\\CurrentControlSet\\GPS Intermediate Driver\\Drivers",
      CurrentDriver);
  }
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    _hIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_TRAY));
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
  h->user_id = conf(OMOBUS_USER_ID, NULL);
  h->caption = conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  _hResInstance = LoadMuiModule(_hInstance, conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  wcsncpy(wsBtnCap1, wsfromRes(IDS_HIDE).c_str(), CALC_BUF_SIZE(wsBtnCap1));
  wcsncpy(wsBtnCap2, wsfromRes(IDS_CLOSE).c_str(), CALC_BUF_SIZE(wsBtnCap2));

  RegisterGPSConfTrigger(h);
  return h;
}

void stop_daemon(void *_h) 
{
  HDAEMON *h = (HDAEMON *)_h;
  if( h == NULL )
    return;
  ClearRegNotify(h);
  CleanupPopupNotify();
  delete h;
}
