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

#include <winsock2.h>
#include <windows.h>
#include <pm.h>
#include <snapi.h>
#include <regext.h>
#include <notify.h>
#include <aygshell.h>
#include <bt_api.h>
#include <bthutil.h>
#include <tapi.h>
#include <extapi.h>
#include <winioctl.h>
#include <ntddndis.h>
#include <nuiouser.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

#define PHONE_ALWAYS_ON           L"phone->always_on"
#define PHONE_ALWAYS_ON_DEF       L"no"
#define BLUETOOTH_ALWAYS_OFF      L"bluetooth->always_off"
#define BLUETOOTH_ALWAYS_OFF_DEF  L"no"
#define WIFI_ALWAYS_OFF           L"wifi->always_off"
#define WIFI_ALWAYS_OFF_DEF       L"no"

#define BLUETOOTH_ON(dwStatus) (1 == (1 & (dwStatus)))

#define SN_BLUETOOTHSTATUS_PATH   L"System\\State\\Hardware"
#define SN_BLUETOOTHSTATUS_VALUE  L"Bluetooth"

typedef struct _tagMODULE {
  HANDLE hThr[3], hKill;
  const wchar_t *user_id, *caption, *pk_ext, *pk_mode,
    *working_hours, *working_days;
  bool phone_always_on, bluetooth_always_off, wifi_always_off;
} HDAEMON;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIcon = NULL;
static wchar_t wsBtnCap1[16] = L"Hide", wsBtnCap2[16] = L"Close";
// {712AC67B-CCB1-4eb5-8221-30E8C3C473AF}
static const GUID CLSID_SHNAPI_OMOBUS_PHONE_OFF = 
  { 0x712ac67b, 0xccb1, 0x4eb5, { 0x82, 0x21, 0x30, 0xe8, 0xc3, 0xc4, 0x73, 0xaf } };
// {65A5DEB9-E688-428b-BEE6-98F3D3BE8B7D}
static const GUID CLSID_SHNAPI_OMOBUS_WIFI_ON = 
{ 0x65a5deb9, 0xe688, 0x428b, { 0xbe, 0xe6, 0x98, 0xf3, 0xd3, 0xbe, 0x8b, 0x7d } };
// {71A6262E-8065-45c0-9AF4-ED7976114E5E}
static const GUID CLSID_SHNAPI_OMOBUS_BLUETOOTH_ON = 
{ 0x71a6262e, 0x8065, 0x45c0, { 0x9a, 0xf4, 0xed, 0x79, 0x76, 0x11, 0x4e, 0x5e } };

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
void StoreHardwareStatus(HDAEMON *h, const wchar_t *device, byte_t state) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_HARDWARE_STATUS, true);
  if( xml.empty() )
    return;

  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%state%", state ? L"on" : L"off");
  wsrepl(xml, L"%device%", device);
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_HARDWARE, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static
void InitWarningNotify(HDAEMON *h, UINT uID, const GUID *snId) {
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1;
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = _hIcon;
  memcpy(&sn.clsid, snId, sizeof(CLSID));
  sn.grfFlags = SHNF_TITLETIME|SHNF_DISPLAYON|SHNF_CRITICAL;
  sn.pszTitle = h->caption;
  sn.pszHTML = wsfromRes(uID).c_str();
  sn.rgskn[0].pszTitle = wsBtnCap1;
  //sn.rgskn[0].skc.wpCmd = 100;
  sn.rgskn[1].pszTitle = wsBtnCap2;
  sn.rgskn[1].skc.wpCmd = 101;
  SHNotificationAdd(&sn);
}

static
void CleanupPopupNotify(const GUID *snId) {
  SHNotificationRemove(snId, 1);
}

static
DWORD GetRegStatus(const wchar_t *path, const wchar_t *key) {
  DWORD dwStatus = 0;
  RegistryGetDWORD(HKEY_LOCAL_MACHINE, path, key, &dwStatus);
  return dwStatus;
}

static
bool IsBluetoothEnabled() {
  DWORD mode = 0;
  return BthGetMode(&mode) == ERROR_SUCCESS && BTH_POWER_OFF != mode;
}

static
void ChangeBluetoothState(bool power) {
  BthSetMode(power ? BTH_DISCOVERABLE : BTH_POWER_OFF);
}

static
void SaveBluetoothStatus(HDAEMON *h, byte_t state /*on|off*/) {
  StoreHardwareStatus(h, L"bluetooth", state);
  if( state ) {
    InitWarningNotify(h, IDS_BLUETOOTH_ON, &CLSID_SHNAPI_OMOBUS_BLUETOOTH_ON);
    if( h->bluetooth_always_off ) {
      Sleep(4000);
      ChangeBluetoothState(false);
    }
  } else {
    CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_BLUETOOTH_ON);
  }
  RETAILMSG(TRUE, (L"hardware: bluetooth='%s'.\n", state ? L"ON" : L"off"));
}

static
void BluetoothStatusChangedNotify(HREGNOTIFY /*hNotify*/, DWORD dwUserData, 
  const PBYTE pData, const UINT cbData) 
{
  HANDLE h = (HANDLE *)dwUserData;
  if( h != NULL && pData != NULL && cbData == sizeof(DWORD) ) {
    SetEvent(h);
  }
}

static 
DWORD CALLBACK BluetoothProc(LPVOID pParam) 
{
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  HREGNOTIFY hNotify = NULL;
  BTEVENT bte = { 0 };
  DWORD bytes_read = 0, flags = 0, bt_reg_status = 255, st = 0;
  HANDLE msgQ = NULL, btn = NULL, hEv[] = {h->hKill, NULL};
  MSGQUEUEOPTIONS msg = { 0 };
  msg.dwSize = sizeof( MSGQUEUEOPTIONS );
  msg.cbMaxMessage = sizeof( BTEVENT );
  msg.bReadAccess = TRUE;
  msg.dwFlags = MSGQUEUE_NOPRECOMMIT;

  if( (msgQ = CreateMsgQueue(NULL, &msg)) == NULL ) {
    RETAILMSG(TRUE, (L"BluetoothProc: Unable to open message query.\n"));
    return 1;
  }

  if( (btn = RequestBluetoothNotifications(BTE_CLASS_STACK, msgQ)) == NULL ) {

/* via regestry notifications */
    hEv[1] = CreateEvent(NULL, FALSE, TRUE, NULL);
    if( hEv[1] == NULL ) {
      RETAILMSG(TRUE, (L"BluetoothProc: %s", fmtoserr().c_str()));
    } else {
      if( RegistryNotifyCallback(HKEY_LOCAL_MACHINE, SN_BLUETOOTHSTATUS_PATH, SN_BLUETOOTHSTATUS_VALUE,
            BluetoothStatusChangedNotify, (DWORD)(hEv[1]), NULL, &hNotify) != S_OK ) {
        RETAILMSG(TRUE, (L"BluetoothProc: Unable to notify changes for the HKLM\\%s -> %s",
          SN_BLUETOOTHSTATUS_PATH, SN_BLUETOOTHSTATUS_VALUE));
      } else {
        while( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) 
          == WAIT_OBJECT_0 + 1 ) {
          st = GetRegStatus(SN_BLUETOOTHSTATUS_PATH, SN_BLUETOOTHSTATUS_VALUE);
          if( bt_reg_status != st ) {
            SaveBluetoothStatus(h, BLUETOOTH_ON(st));
            bt_reg_status = st;
          }
        }
        RegistryCloseNotification(hNotify);
      }
      CloseHandle(hEv[1]);
    }

/* via bt stack */
  } else {
    SaveBluetoothStatus(h, IsBluetoothEnabled());
    hEv[1] = msgQ;
    while( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) 
      == WAIT_OBJECT_0 + 1 ) {
        memset(&bte, 0, sizeof(bte));
        bytes_read = flags = 0;
      if( ReadMsgQueue(msgQ, &bte, sizeof(BTEVENT), &bytes_read,
            0, &flags ) && bytes_read > 0 ) {
        SaveBluetoothStatus(h, bte.dwEventId == BTE_STACK_UP);
      }
    }
    StopBluetoothNotifications(btn);
  }

  CloseMsgQueue(msgQ);

  return 0;
}

bool IsPhoneEnabled(HLINE hLine)
{
  DWORD equip_state = 0, radio_state = 0;
  if( lineGetEquipmentState(hLine, &equip_state, &radio_state ) != 0 ) {
    equip_state = 0;
  }
  return equip_state > 0 && LINEEQUIPSTATE_MINIMUM != equip_state;
}

void ChangePhoneState(HLINE hLine, bool enable) {
  if( enable ) {
    if( lineSetEquipmentState(hLine, LINEEQUIPSTATE_FULL ) == 0 ) {
      lineRegister(hLine, LINEREGMODE_AUTOMATIC, NULL, LINEOPFORMAT_NONE);
    }      
  } else {
    lineSetEquipmentState(hLine, LINEEQUIPSTATE_MINIMUM);
  }
}

static
void SavePhoneStatus(HDAEMON *h, HLINE hLine, byte_t state /*on|off*/) {
  StoreHardwareStatus(h, L"phone", state);
  if( state ) {
    CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_PHONE_OFF);
  } else {
    InitWarningNotify(h, IDS_PHONE_OFF, &CLSID_SHNAPI_OMOBUS_PHONE_OFF);
    if( h->phone_always_on ) {
      Sleep(4000);
      ChangePhoneState(hLine, true);
    }
  }
  RETAILMSG(TRUE, (L"hardware: phone='%s'.\n", state ? L"on" : L"OFF"));
}

static 
DWORD CALLBACK PhoneProc(LPVOID pParam) 
{
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  HANDLE hEv[] = {h->hKill, NULL};
  DWORD api_ver = TAPI_CURRENT_VERSION, device_count = 0;
  HLINEAPP hLineApp = NULL;
  HLINE hLine = NULL;
  LINEMESSAGE lineMsg = { 0 };
  LINEINITIALIZEEXPARAMS lineParam = { 0 };
  lineParam.dwTotalSize = sizeof(LINEINITIALIZEEXPARAMS);
  lineParam.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;

  if( lineInitializeEx(&hLineApp, NULL, NULL, NULL, &device_count, &api_ver, &lineParam) != 0 ) {
    RETAILMSG(TRUE, (L"PhoneProc: Unable to initialize TAPI.\n"));
    return 1;
  }

  if( lineOpen(hLineApp, 0, &hLine, TAPI_CURRENT_VERSION, 
        0, NULL, LINECALLPRIVILEGE_MONITOR, NULL, NULL) == 0 ) 
  {
    SavePhoneStatus(h, hLine, IsPhoneEnabled(hLine));
    hEv[1] = lineParam.Handles.hEvent;
    while( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) 
      == WAIT_OBJECT_0 + 1 ) {
      if( lineGetMessage(hLineApp, &lineMsg, 0 ) == 0 ) {
        if( lineMsg.dwMessageID == LINE_DEVSPECIFIC ) {
          if( LINE_EQUIPSTATECHANGE == lineMsg.dwParam1 ) {
            SavePhoneStatus(h, hLine, lineMsg.dwParam2 != LINEEQUIPSTATE_MINIMUM);
          }
        }
      }
    }

    lineClose(hLine);
  }

  lineShutdown(hLineApp);

  return 0;
}

static
bool IsWiFiDevice(HANDLE ndis, const wchar_t *device_name)
{
  DWORD read = 0, rc = 0, buf_len = sizeof(NDISUIO_QUERY_OID) + sizeof(NDIS_802_11_SSID);
  BYTE *buf = NULL;
  NDISUIO_QUERY_OID *query = NULL;

  if( (buf = (BYTE *) omobus_alloc(buf_len)) == NULL ) {
    return false;
  }

  query = (NDISUIO_QUERY_OID *) buf;
  query->Oid = OID_802_11_SSID;
  query->ptcDeviceName = (wchar_t *)device_name;

  if( DeviceIoControl(ndis, IOCTL_NDISUIO_QUERY_OID_VALUE, buf, buf_len, 
        buf, buf_len, &read, NULL) ) {
    rc = ERROR_SUCCESS;
  } else {
    rc = GetLastError();
  }

  omobus_free(buf);
  return ERROR_SUCCESS == rc || ERROR_GEN_FAILURE == rc;
}

static
wchar_t *GetWiFiRadioName(HANDLE ndis)
{
  BYTE *buf = NULL;
  DWORD buf_size = 0;
  IP_ADAPTER_INFO* info = NULL;
  wchar_t *rc = NULL;
  
  if( ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(NULL, &buf_size) ) {
    if( (buf = (BYTE *) omobus_alloc(buf_size)) != NULL ) {
      info = (IP_ADAPTER_INFO *)buf;
      if( GetAdaptersInfo(info, &buf_size) == ERROR_SUCCESS ) {
        // Check each adapter to see if it responds to the standard 
        // OID_802_11_SSID query. If it does, then it is an 802.11 
        // adapter.
        for( IP_ADAPTER_INFO* i = info; NULL != i; i = i->Next ) {
          // If the WiFi adapter is already on, QueryOid() will return
          // ERROR_SUCCCESS. If not, it will fail and return 
          // ERROR_GEN_FAILURE. If it weren't an 802.11 adapter, it 
          // would return ERROR_INVALID_PARAMETER. Therefore, we will 
          // consider ERROR_GEN_FAILURE a success.
          rc = mbstowcs_dup(i->AdapterName);
          if( rc != NULL ) {
            if( IsWiFiDevice(ndis, rc) ){
              break;
            } else {
              omobus_free(rc);
              rc = NULL;
            }
          }
        }
      }
      omobus_free(buf);
    }
  }

  return rc;
}

static
bool IsWiFiEnabled(const wchar_t *radio_name)
{
  CEDEVICE_POWER_STATE state = D4;
  wchar_t cmd[1024] = L"";
  _snwprintf(cmd, 1023, L"%s\\%s", PMCLASS_NDIS_MINIPORT, radio_name);
  return GetDevicePower(cmd, POWER_NAME, &state) == ERROR_SUCCESS &&
    state != D4;
}

void ChangeWiFiState(const wchar_t *radio_name, bool power)
{
  wchar_t cmd[1024] = L"";
  _snwprintf(cmd, 1023, L"%s\\%s", PMCLASS_NDIS_MINIPORT, radio_name);
  SetDevicePower(cmd, POWER_NAME, power ? D0 : D4);
}

static
void SaveWiFiStatus(HDAEMON *h, const wchar_t *radio_name, byte_t state /*on|off*/) {
  StoreHardwareStatus(h, L"wifi", state);
  if( state ) {
    InitWarningNotify(h, IDS_WIFI_ON, &CLSID_SHNAPI_OMOBUS_WIFI_ON);
    if( h->wifi_always_off ) {
      Sleep(4000);
      ChangeWiFiState(radio_name, false);
    }
  } else {
    CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_WIFI_ON);
  }
  RETAILMSG(TRUE, (L"hardware: wifi='%s'.\n", state ? L"ON" : L"off"));
}

static 
DWORD CALLBACK WiFiProc(LPVOID pParam) 
{
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  wchar_t *radio_name = NULL;
  HANDLE ndis = INVALID_HANDLE_VALUE, msgQ = NULL, hEv[] = {h->hKill, NULL};
  MSGQUEUEOPTIONS msg = { 0 };
  NDISUIO_REQUEST_NOTIFICATION rn = { 0 };
  NDISUIO_DEVICE_NOTIFICATION dn = { 0 };
  DWORD bytes_read = 0, flags = 0;

  if( (ndis = CreateFile(NDISUIO_DEVICE_NAME, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE))
          == INVALID_HANDLE_VALUE ) {
    RETAILMSG(TRUE, (L"WiFiProc: Unable to open %s.\n", NDISUIO_DEVICE_NAME));
    return 1;
  }

  if( (radio_name = GetWiFiRadioName(ndis)) == NULL ) {
    RETAILMSG(TRUE, (L"WiFiProc: Unable to find WiFi module.\n"));
  } else {
    msg.dwSize = sizeof(MSGQUEUEOPTIONS);
    msg.cbMaxMessage = sizeof(NDISUIO_DEVICE_NOTIFICATION);
    msg.bReadAccess = TRUE;
    msg.dwFlags = MSGQUEUE_NOPRECOMMIT;
    
    if( (msgQ = CreateMsgQueue(NULL, &msg)) == NULL ) {
      RETAILMSG(TRUE, (L"WiFiProc: Unable to open message query.\n"));
    } else {
      hEv[1] = msgQ;
      rn.hMsgQueue = msgQ;
      rn.dwNotificationTypes = NDISUIO_NOTIFICATION_DEVICE_POWER_DOWN | NDISUIO_NOTIFICATION_DEVICE_POWER_UP;
      DeviceIoControl(ndis, IOCTL_NDISUIO_REQUEST_NOTIFICATION, &rn, 
        sizeof(NDISUIO_REQUEST_NOTIFICATION), NULL, 0, NULL, NULL);

      SaveWiFiStatus(h, radio_name, IsWiFiEnabled(radio_name));

      while( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) 
        == WAIT_OBJECT_0 + 1 ) {
        memset(&dn, 0, sizeof(dn));
        bytes_read = flags = 0;
        if( ReadMsgQueue(msgQ, &dn, sizeof(NDISUIO_DEVICE_NOTIFICATION), &bytes_read,
              0, &flags ) && bytes_read > 0 ) {
          SaveWiFiStatus(h, radio_name,
            dn.dwNotificationType == NDISUIO_NOTIFICATION_DEVICE_POWER_UP);
        }
      }

      DeviceIoControl(ndis, IOCTL_NDISUIO_CANCEL_NOTIFICATION, NULL, 
        NULL, NULL, 0, NULL, NULL);
      CloseMsgQueue(msgQ);
    }
    omobus_free(radio_name);
  }

  CloseHandle(ndis);

  return 0;
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
  h->working_hours = conf(OMOBUS_WORKING_HOURS, NULL);
  h->working_days = conf(OMOBUS_WORKING_DAYS, NULL);
  h->phone_always_on = wcsistrue(conf(PHONE_ALWAYS_ON, PHONE_ALWAYS_ON_DEF));
  h->bluetooth_always_off = wcsistrue(conf(BLUETOOTH_ALWAYS_OFF, BLUETOOTH_ALWAYS_OFF_DEF));
  h->wifi_always_off = wcsistrue(conf(WIFI_ALWAYS_OFF, WIFI_ALWAYS_OFF_DEF));

  _hResInstance = LoadMuiModule(_hInstance, conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  wcsncpy(wsBtnCap1, wsfromRes(IDS_HIDE).c_str(), CALC_BUF_SIZE(wsBtnCap1));
  wcsncpy(wsBtnCap2, wsfromRes(IDS_CLOSE).c_str(), CALC_BUF_SIZE(wsBtnCap2));

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThr[0] = CreateThread(NULL, 0, BluetoothProc, h, 0, NULL);
  h->hThr[1] = CreateThread(NULL, 0, PhoneProc, h, 0, NULL);
  h->hThr[2] = CreateThread(NULL, 0, WiFiProc, h, 0, NULL);

  return h;
}

void stop_daemon(void *_h) 
{
  HDAEMON *h = (HDAEMON *)_h;
  if( h == NULL )
    return;
  if( h->hKill != NULL )
    SetEvent(h->hKill);
  for( int i = 0; i < sizeof(h->hThr)/sizeof(h->hThr[0]); i++ ) {
    if( h->hThr[i] != NULL ) {
      if( WaitForSingleObject(h->hThr[i], 5000) == WAIT_TIMEOUT ) {
        TerminateThread(h->hThr[i], -1);
        RETAILMSG(TRUE, (L"hardware: Terminating thread.\n"));
      }
      CloseHandle(h->hThr[i]);
      h->hThr[i] = NULL;
    }
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  h->hKill = NULL;  
  CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_PHONE_OFF);
  CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_WIFI_ON);
  CleanupPopupNotify(&CLSID_SHNAPI_OMOBUS_BLUETOOTH_ON);
  delete h;
}
