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

#include "daemons-host.h"
#include "resource.h"
#include "../../version"

#define REG_ALIVE_NAME            L"alive"
#define DAEMONS_LIST              L"daemons"
#define CONNECTION                L"transport->connection"
#define SERVERS                   L"transport->servers"
#define CHK_SERVER_TIMEOUT        L"transport->chk_server_timeout"
#define CHK_SERVER_TIMEOUT_DEF    L"10"
#define FTP_EPSV                  L"transport->ftp->epsv"
#define FTP_EPSV_DEF              L"yes"
#define FTP_SKIP_PASV_IP          L"transport->ftp->skip_pasv_ip"
#define FTP_SKIP_PASV_IP_DEF      L"yes"
#define FTP_RESPONSE_TIMEOUT      L"transport->ftp->response_timeout"
#define FTP_RESPONSE_TIMEOUT_DEF  L"0"
#define TIMEOUT                   L"transport->timeout"
#define TIMEOUT_DEF               L"120"
#define CONNECT_TIMEOUT           L"transport->connect_timeout"
#define CONNECT_TIMEOUT_DEF       L"90"

typedef struct _tag_daemons_conf {
  omobus_dll_t hLib;
  pf_start_daemon start;
  pf_stop_daemon stop;
  void *hDaemon;
} daemons_conf_t;

typedef std::vector<daemons_conf_t> daemons_t;
typedef std::map<std::wstring, std::wstring> conf_t;

static daemons_transport_t _tr_func;
static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIcon[3] = {NULL, NULL, NULL}; 
static short _hCurIcon = 0;
// {D59AFF71-4865-413c-8044-E1FE24206DD2}
static const GUID CLSID_SHNAPI_OMOBUS_TRANSPORT = 
  { 0xd59aff71, 0x4865, 0x413c, { 0x80, 0x44, 0xe1, 0xfe, 0x24, 0x20, 0x6d, 0xd2 } };
static const wchar_t _wakeup_uid[] = 
  L"AC3BCB70-A868-414d-B67D-8FE7F09FE77C";
static critical_section _stayup_cs, _conf_cs;
static conf_t _conf;
static int _stayup = 0; // Счетчик удержания устройства от сна
static slist_t _servers;
static uint32_t _chk_server_timeout = 0;
uint32_t _ftp_skip_pasv_ip = 0, _ftp_epsv = 0, _ftp_response_timeout = 0, 
  _timeout = 0, _connect_timeout = 0;
static const wchar_t *_net = NULL, *_user_id = NULL, *_key = NULL, 
  *_pk_ext = NULL, *_pk_mode = NULL, *_caption = NULL;

std::wstring wsfromRes(UINT uID, bool sys) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

const wchar_t *GetUserId() {
  return _user_id;
}

const wchar_t *GetUserKey() {
  return _key;
}

static
slist_t &_parce_slist(const wchar_t *s, slist_t &sl) {
  sl.clear();
  if( s == NULL )
    return sl;
  std::wstring buf;
  for( int i = 0; s[i] != L'\0'; i++ ) {
    if( s[i] == L' ' ) {
      if( !buf.empty() )
        sl.push_back(buf);
      buf = L"";
    } else {
      buf += s[i];
    }
  }
  if( !buf.empty() )
    sl.push_back(buf);
  return sl;
}

static
void _wcsfconf_par(void *cookie, const wchar_t *par_name, const wchar_t *par_val) {
  conf_t *ptr = (conf_t *)cookie;
  if( ptr == NULL )
    return;
  if( (*ptr).find(par_name) == (*ptr).end() )
    (*ptr).insert(conf_t::value_type(par_name, par_val));
  else
    (*ptr)[par_name] = par_val;
}

static
void _stayup_increment() {
  s_guard _al(_stayup_cs);
  _stayup++;
  DEBUGMSG(_stayup == 1, (L"_stayup: START\n"));
}

static
void _stayup_decrement() {
  s_guard _al(_stayup_cs);
  if( _stayup > 0 )
    _stayup--;
  if( _stayup == 0 )
    CloseConn();
  DEBUGMSG(_stayup == 0, (L"_stayup: STOP\n"));
}

static
void _increment_notify_icon() {
  _hCurIcon++;
  if( _hCurIcon >= 3 )
    _hCurIcon = 0;
}

static
void _DisableSuspend() {
  // Сброс таймера перехода в режим ожидания
  SystemIdleTimerReset();
  // VK_F24 удерживает от перехода в спящий режим только при включенном экране.
  keybd_event(VK_F24, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
  // Обеспечиваем периодическое пробуждение устройства.
  if( SetTimeoutTrigger(_wakeup_uid, 15) != OMOBUS_OK ) {
    RETAILMSG(TRUE, (L"Unable to set ['%s'/15 sec] trigger!\n", _wakeup_uid));
  }
}

static
void _EnableSuspend() {
  RemoveTrigger(_wakeup_uid);
}

static
void _InitPopupNotify()
{
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1; // Код уведомления
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = _hIcon[_hCurIcon];
  sn.clsid = CLSID_SHNAPI_OMOBUS_TRANSPORT;
  sn.grfFlags = SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|SHNF_SILENT;
  sn.pszTitle = _caption;
  sn.pszHTML = wsfromRes(IDS_TRANSPORT, false).c_str();
  SHNotificationAdd(&sn);
}

static
void _CleanupPopupNotify()
{
  SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_TRANSPORT, 1);
}

static
void _UpdatePopupNotify()
{
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1; // Код уведомления
  sn.clsid = CLSID_SHNAPI_OMOBUS_TRANSPORT;
  sn.hicon = _hIcon[_hCurIcon];
  SHNotificationUpdate(SHNUM_ICON, &sn);
}

// Периодически сбрасываем в реестр информацию о том, что модуль
// работает (сбрасываем дату). При корректном завершении модуля,
// эта информация удаляется. При некорректном завершении (например,
// выключение устройства или извлечение ак/б) на основании этой
// даты формируется активность - остановка процесса 
// (см. StoreIncorrectStopActivity).
static
void _SetAliveValue(time_t tt = omobus_time()) {
  RegistrySetDWORD(HKEY_LOCAL_MACHINE, OMOBUS_CODE, REG_ALIVE_NAME, tt);
}

static
void _DeleteAliveValue() {
  RegistryDeleteValue(HKEY_LOCAL_MACHINE, OMOBUS_CODE, REG_ALIVE_NAME);
}

static
time_t _GetAliveValue() {
  DWORD dwVal = 0;
  return 
    RegistryGetDWORD(HKEY_LOCAL_MACHINE, OMOBUS_CODE, REG_ALIVE_NAME, &dwVal) != S_OK 
      ?
        0
      :
        dwVal;
}

static
void _StoreProcActivity(bool start, uint32_t pid, const wchar_t *image_name, const wchar_t *args) 
{
  if( _user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_X_SYSACT_PROC, true);
  if( xml.empty() )
    return;

  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", _user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%state%", start?L"start":L"stop");
  wsrepl(xml, L"%pid%", itows(pid).c_str());
  wsrepl(xml, L"%image_name%", image_name?image_name:L"");
  wsrepl(xml, L"%args%", args?args:L"");

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_PROC, _user_id, _pk_ext, 
    _pk_mode, xml.c_str());
}

static
void _StoreIncorrectStopActivity(const wchar_t *image_name) 
{
  if( _user_id == NULL ) {
    return;
  }
  std::wstring xml = wsfromRes(IDS_X_SYSACT_PROC, true);
  if( xml.empty() ) {
    return;
  }
  time_t tt = _GetAliveValue();
  if( tt == 0 ) {
    return;
  }

  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", _user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(tt).c_str());
  wsrepl(xml, L"%satellite_dt%", L"0000-00-00 00:00:00");
  wsrepl(xml, L"%latitude%", L"0");
  wsrepl(xml, L"%longitude%",L"0");
  wsrepl(xml, L"%state%", L"stop");
  wsrepl(xml, L"%pid%", L"0");
  wsrepl(xml, L"%image_name%", image_name);
  wsrepl(xml, L"%args%", L"");

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_PROC, _user_id, _pk_ext, 
    _pk_mode, xml.c_str());
}

static
const wchar_t *_daemons_conf(const wchar_t *par_name, const wchar_t *def_val) {
  if( par_name == NULL )
    return NULL;
  s_guard _al(_conf_cs);
  if( _conf.find(par_name) == _conf.end() ) {
    if( def_val == NULL )
      return def_val;
    _conf[par_name] = def_val;
  }
  return _conf[par_name].c_str();
}

static
int _daemons_pk_send(const wchar_t *spath, const wchar_t *dpath, uint32_t *sent) 
{
  if( spath == NULL || dpath == NULL || sent == NULL )
    return OMOBUS_ERR;
  
  std::wstring uri;
  int rc = OMOBUS_ERR;
  const wchar_t *server = NULL;
  *sent = 0;

  if( !IsSendPk(spath) )
    return OMOBUS_OK;

  _stayup_increment();
  if( (server = GetValidServer(_net, _servers, _chk_server_timeout)) != NULL ) {
    uri = server; uri += dpath;
    rc = FtpSendPk(spath, uri.c_str(), sent);
  }
  _stayup_decrement();

  return rc;
}

static
int _daemons_pk_recv(const wchar_t *spath, const wchar_t *dpath, uint32_t *recived) 
{
  if( spath == NULL || dpath == NULL || recived == NULL )
    return OMOBUS_ERR;
  
  std::wstring uri;
  int rc = OMOBUS_ERR;
  const wchar_t *server = NULL;
  (*recived) = 0;

  _stayup_increment();
  if( (server = GetValidServer(_net, _servers, _chk_server_timeout)) != NULL ) {
    uri = server; uri += spath;
    rc = FtpRecvPk(uri.c_str(), dpath, recived);
  }
  _stayup_decrement();

  return rc;
}

static
const std::wstring _plugin_name(const wchar_t *name) {
  std::wstring rc = GetOmobusRootPath(); 
  rc += L"svc_";
  rc += name; 
  rc += L".dll";
  return rc;
}

static
int _start(slist_t &names, daemons_t &daemons) {
  for( size_t i = 0; i < names.size(); i++ ) {
    daemons_conf_t dc;
    dc.hLib = dlopen(_plugin_name(names[i].c_str()).c_str());
    if( dc.hLib == NULL )
      continue;
    dc.start = (pf_start_daemon) dlsym(dc.hLib, L"start_daemon");
    dc.stop = (pf_stop_daemon) dlsym(dc.hLib, L"stop_daemon");
    if( dc.start == NULL || dc.stop == NULL ) {
      chk_dlclose(dc.hLib);
      continue;
    }
    dc.hDaemon = dc.start(_daemons_conf, &_tr_func);
    if( dc.hDaemon == NULL ) {
      chk_dlclose(dc.hLib);
      continue;
    }
    daemons.push_back(dc);
  }
  return daemons.empty() ? OMOBUS_ERR : OMOBUS_OK;
}

static
void _stop(daemons_t &daemons) {
  for( size_t i = 0; i < daemons.size(); i++ ) {
    daemons_conf_t *dc = &daemons[i];
    dc->stop(dc->hDaemon);
    chk_dlclose(dc->hLib);
  }
  daemons.clear();
}

static
void _wait(HANDLE hStop) {
  bool notify = false;
  ResetEvent(hStop);
  while( WaitForSingleObject(hStop, 2000) == WAIT_TIMEOUT ) {
    _SetAliveValue();
    s_guard _al(_stayup_cs);
    if( _stayup > 0 ) {
      _DisableSuspend();
      if( !notify ) {
        _InitPopupNotify();
        notify = true;
      } else {
        _UpdatePopupNotify();
      }
      _increment_notify_icon();
    } else {
      if( notify ) {
        _CleanupPopupNotify();
        _EnableSuspend();
        notify = false;
      }
    }
  }
  _CleanupPopupNotify();
  _EnableSuspend();
}

int _tmain(int argc, _TCHAR* argv[])
{
  CEnsureCloseHandle hOnline = CreateEvent(NULL, TRUE, FALSE, OMOBUS_SRV_ONLINE);
  if( hOnline == NULL || GetLastError() == ERROR_ALREADY_EXISTS )
    return 1;

  // Since this is application is launched through the registry HKLM\Init 
  // we need to call SignalStarted passing in the command line parameter.
  // See 'Create a Windows CE Image That Boots to Kiosk Mode':
  //  http://msdn.microsoft.com/en-us/library/aa446914.aspx
  if( argc == 2 )
    SignalStarted(_wtol(argv[1]));

  _hInstance = (HINSTANCE)GetModuleHandle(NULL);
  _hIcon[0] = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1));
  _hIcon[1] = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON2));
  _hIcon[2] = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON3));

  daemons_t daemons;
  slist_t daemon_names;
  wchar_t locale[255];
  const wchar_t *mui = NULL;
  WaitCursor _wc_startup(_hInstance);
  
  memset(locale, 0, sizeof(locale));
  memset(&_tr_func, 0, sizeof(_tr_func));
  _tr_func.pk_recv = _daemons_pk_recv;
  _tr_func.pk_send = _daemons_pk_send;

  ParseOmobusConf(NULL, &_conf, _wcsfconf_par);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Загрузка локализационных данных
  mui = _daemons_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF);
  _hResInstance = LoadMuiModule(_hInstance, mui);
  _snwprintf(locale, 254, L"locale.%s", mui);
  ParseOmobusConf(locale, &_conf, _wcsfconf_par);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  if( _conf.find(DAEMONS_LIST) == _conf.end() )
    return -1;
  _parce_slist(_conf[DAEMONS_LIST].c_str(), daemon_names);
  _parce_slist(_daemons_conf(SERVERS, NULL), _servers);

  _user_id = _daemons_conf(OMOBUS_USER_ID, NULL);
  _key = _daemons_conf(OMOBUS_USER_KEY, L"");
  _caption = _daemons_conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  _net = _daemons_conf(CONNECTION, NULL);
  _chk_server_timeout = _wtoi(_daemons_conf(CHK_SERVER_TIMEOUT, CHK_SERVER_TIMEOUT_DEF));
  _ftp_skip_pasv_ip = wcsistrue(_daemons_conf(FTP_SKIP_PASV_IP, FTP_SKIP_PASV_IP_DEF));
  _ftp_epsv = wcsistrue(_daemons_conf(FTP_EPSV, FTP_EPSV_DEF));
  _ftp_response_timeout = _wtoi(_daemons_conf(FTP_RESPONSE_TIMEOUT, FTP_RESPONSE_TIMEOUT_DEF));
  _timeout = _wtoi(_daemons_conf(TIMEOUT, TIMEOUT_DEF));
  _connect_timeout = _wtoi(_daemons_conf(CONNECT_TIMEOUT, CONNECT_TIMEOUT_DEF));
  _pk_ext = _daemons_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  _pk_mode = _daemons_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);

  curl_global_init(CURL_GLOBAL_ALL);
  // Фиксируем факт некорректной остановки (если он был). Некорректная
  // остановка возможна только при выключении устройства, извлечении ак/б 
  // или при падении процесса.
  _StoreIncorrectStopActivity(argv[0]);
  _StoreProcActivity(true, GetCurrentProcessId(), argv[0], GetCommandLine());
  _SetAliveValue();

  if( _start(daemon_names, daemons) == OMOBUS_OK ) {
    _wc_startup.Restore();
    _wait(hOnline);
  }
  _stop(daemons);

  _StoreProcActivity(false, GetCurrentProcessId(), argv[0], GetCommandLine());
  _DeleteAliveValue();
  curl_global_cleanup();
  CloseConn();

  return 0;
}
