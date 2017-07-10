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
#include <aygshell.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

#define REBOOT_EVENT_NAME L"OMOBUS-reboot-event"
#define UPD_WAKEUP        L"upd->wakeup"
#define UPD_WAKEUP_DEF    L"0"
#define UPD_SRV_PATH      L"transport->upd->path"
#define UPD_SRV_PATH_DEF  L"upd/"
#define UPD_REBOOT        L"upd->reboot"
#define UPD_REBOOT_DEF    L"5"
#define UPD_JUNK          L"upd->junk"
#define UPD_JUNK_DEF      L"yes"
#define UPD_HIDDEN        L"upd->hidden"

typedef struct _tagMODULE {
  HANDLE hThrTr, hKill;
  int wakeup, reboot;
  const wchar_t *user_id, *caption, *srv_path, *pk_ext, *pk_mode, 
    *hidden, *time_fmt;
  bool junk;
  pf_daemons_pk_recv recv;
  uint32_t packets;
} HDAEMON;

typedef std::vector<std::wstring> slist_t;

static HINSTANCE _hInstance = NULL;
static CEnsureFreeLibrary _hResInstance;
static HICON _hIcon = NULL, _hIcon1 = NULL;
static wchar_t wsBtnCap1[16] = L"Hide", wsBtnCap2[16] = L"Close", 
  wsBtnCap3[16] = L"Reboot", wsRebootTask[255] = L"";
// {86925669-2626-4957-BE06-F8DD14822095}
static const GUID CLSID_SHNAPI_OMOBUS_UPD = 
  { 0x86925669, 0x2626, 0x4957, { 0xbe, 0x6, 0xf8, 0xdd, 0x14, 0x82, 0x20, 0x95 } };

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_hInstance:_hResInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
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

void CleanupPopupNotify(bool cleanReboot) {
  if( cleanReboot ) {
    SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_UPD, 1);
  }
  SHNotificationRemove(&CLSID_SHNAPI_OMOBUS_UPD, 2);
}

static
std::wstring rebootMsg(HDAEMON *h) {
  time_t tt_reb = omobus_time() + h->reboot*60;
  std::wstring html = wsfromRes(IDS_UPD_REBOOT);
  wsrepl(html, L"%reboot_datetime%", wsftime(tt_reb, h->time_fmt).c_str());
  return html;
}

static
void InitRebootNotify(HDAEMON *h) {
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 1;
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = _hIcon;
  sn.clsid = CLSID_SHNAPI_OMOBUS_UPD;
  sn.grfFlags = SHNF_TITLETIME|SHNF_DISPLAYON|SHNF_CRITICAL;
  sn.pszTitle = h->caption;
  sn.pszHTML = rebootMsg(h).c_str();
  sn.pszTodaySK = wsBtnCap3;
  sn.pszTodayExec = wsRebootTask;
  SHNotificationAdd(&sn);
}

static
void InitWarningNotify(HDAEMON *h) {
  SHNOTIFICATIONDATA sn;
  memset(&sn, 0, sizeof(SHNOTIFICATIONDATA));
  sn.cbStruct = sizeof(sn);
  sn.dwID = 2;
  sn.npPriority = SHNP_INFORM;
  sn.csDuration = -1;
  sn.hicon = _hIcon1;
  sn.clsid = CLSID_SHNAPI_OMOBUS_UPD;
  sn.grfFlags = SHNF_TITLETIME|SHNF_STRAIGHTTOTRAY|SHNF_CRITICAL;
  sn.pszTitle = h->caption;
  sn.pszHTML = wsfromRes(IDS_UPD_ERROR).c_str();
  sn.rgskn[0].pszTitle = wsBtnCap1;
  sn.rgskn[0].skc.wpCmd = 100;
  sn.rgskn[0].skc.grfFlags = NOTIF_SOFTKEY_FLAGS_HIDE;
  sn.rgskn[1].pszTitle = wsBtnCap2;
  sn.rgskn[1].skc.wpCmd = 101;
  sn.rgskn[1].skc.grfFlags = NOTIF_SOFTKEY_FLAGS_DISMISS;
  SHNotificationAdd(&sn);
}

static
std::wstring _fmd5(const wchar_t *fn) {
	std::wstring md5;
  FILE *f = _wfopen(fn, L"rb");
  if( f != NULL ) {
	  struct omobus_MD5Context context;
	  unsigned char digest[OMOBUS_MD5_BYTES];
	  wchar_t key[8]; char buf[1024];
	  omobus_MD5Init(&context);
    while( !feof(f) ) {
      memset(buf, 0, sizeof(buf));
      size_t size = fread(buf, 1, sizeof(buf), f);
      if( size > 0 )
        omobus_MD5Update(&context, (const unsigned char *)buf, size);
    }
	  omobus_MD5Final(digest, &context);
	  for( int i = 0; i < OMOBUS_MD5_BYTES; i++ ) {
      memset(key, 0, sizeof(key));
		  _snwprintf(key, CALC_BUF_SIZE(key), L"%.2x", digest[i]); 
      md5 += key;
	  }
    fclose(f);
  }
	return md5;	
}

static
bool _unpack(const wchar_t *pkname, const wchar_t *modname) {
  FILE *f = _wfopen(modname, L"wb");
  if( f == NULL )
    return false;
  pk_file_t pk = pkopen(w2a(pkname).c_str(), "rb");
  if( pk == NULL ) {
    fclose(f);
    return false;
  }
  char buf[1024];
  while( 1 ) {
    memset(buf, 0, sizeof(buf));
    int size = pkread(pk, buf, sizeof(buf));
    if( size <= 0 || size > sizeof(buf) )
      break;
    fwrite(buf, 1, size, f);
  }
  pkclose(pk);
  fclose(f);
  return true;
}

static
uint32_t _upd_install(HDAEMON *h, const wchar_t *pk_path, bool &reboot) {
  // Сравниваем MD5 загруженных пакетов и уже установленных модулей.
  // Если MD5 отличается, переименовываем текущий модуль и распаковываем 
  // на его место модуль из пакета. Если модуль имеет расширение wpx, то 
  // он сразу выполняется.

  reboot = false;
  uint32_t upd = 0;
  std::wstring filter = pk_path; filter += L"*+*.*";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      // Находим последннее вхождение '+' и '.'
      wchar_t *md5 = wcsrchr(FindFileData.cFileName, L'+'), 
        *ext = wcsrchr(FindFileData.cFileName, L'.');
      if( md5 == NULL || ext == NULL || md5 > ext )
        continue;
      // Формирование полного имени пакета
      std::wstring pk = pk_path; pk += FindFileData.cFileName;
      // Делим имя на части
      *md5 = L'\0'; md5++; *ext = L'\0';
      // Формирование полного имени модуля
      std::wstring fn = GetOmobusRootPath(); fn += FindFileData.cFileName;
      // Подсчет MD5 модуля и сравнение его с MD5 пакета
      if( wcsicmp(_fmd5(fn.c_str()).c_str(), md5) != 0 ) {
        if( wcsfexist(fn.c_str()) ) {
          // Переименование текущего модуля (добавляем префикс ~ДАТА~
          // к имени модуля.)
          std::wstring bkp = GetOmobusRootPath(); bkp += L"~"; 
          bkp += wsftime(omobus_time(), FILE_DATETIME_FMT); bkp += L"~"; 
          bkp += FindFileData.cFileName;
          MoveFile(fn.c_str(), bkp.c_str());
        }
        // Распаковываем пакет
        if( !_unpack(pk.c_str(), fn.c_str()) ) {
          RETAILMSG(TRUE, (L"Unable to unpack file from '%s' to '%s': %s\n", 
            pk.c_str(), fn.c_str(), fmtoserr().c_str()));
        } else {
          // Определяем расширение файла
          ext = wcsrchr(fn.c_str(), L'.');
          if( ext != NULL ) {
            ext++;
            // Выполняем WPX
            if( wcsicmp(ext, L"wpx") == 0 ) {
              if( !ProcessXMLConfigFile(fn.c_str()) ) {
                RETAILMSG(TRUE, (L"svc_reconf: '%s' - FAILED\n", fn.c_str()));
              } else {
                RETAILMSG(TRUE, (L"svc_reconf: '%s' - success\n", fn.c_str()));
              }
              reboot = true;
            } else if( wcsicmp(ext, L"jpg") == 0 ) {
              // jpg не требует перезагрузки
            } else {
              reboot = true;
            }
          } else {
            reboot = true;
          }
          upd++;
        }
      }
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }

  return upd;
}

// Поиск файла в папке /profile/pkgs/upd по его первой составляющей до '+'
static
bool isfexist(const wchar_t *pk_path, const wchar_t *ff) {
  bool exist = false;
  std::wstring filter = pk_path; filter += ff; filter += L"+*.*";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    exist = true;
    FindClose(hFind);
  }
  return exist;
}

// Поддержание корневой папки в состоянии идентичном папке /profile/pkgs/upd
static
void _upd_junk_control(HDAEMON *h, const wchar_t *pk_path) {
  if( !h->junk ) // Контроль запрещен настройками
    return;
  std::wstring filter = GetOmobusRootPath(); filter += L"*.*";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      if( FindFileData.cFileName[0] == L'~' )
        continue;
      // Ищем файл в папке /profile/pkgs/upd. Если файл отсутствует,
      // удаляем текущий файл.
      if( !isfexist(pk_path, FindFileData.cFileName) ) {
        std::wstring fn = GetOmobusRootPath(); fn += FindFileData.cFileName;
        // Переименование текущего модуля (добавляем префикс ~ДАТА~ к имени
        // модуля.)
        std::wstring bkp = GetOmobusRootPath(); bkp += L"~"; 
        bkp += wsftime(omobus_time(), FILE_DATETIME_FMT); bkp += L"~";
        bkp += FindFileData.cFileName;
        MoveFile(fn.c_str(), bkp.c_str());
      }
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
}

// Пометка файлов как "скрытые"
static
void _upd_hidden(HDAEMON *h, const wchar_t *mask) {
  std::wstring filter = GetOmobusRootPath(); filter += mask;
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      std::wstring fname = GetOmobusRootPath(); fname += FindFileData.cFileName;
      DWORD attrs = GetFileAttributes(fname.c_str());
      if( !(attrs & FILE_ATTRIBUTE_HIDDEN) )
        SetFileAttributes(fname.c_str(),  attrs | FILE_ATTRIBUTE_HIDDEN);
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
}

static
void _upd_hidden(HDAEMON *h) {
  if( h->hidden == NULL )
    return;
  slist_t ml; _parce_slist(h->hidden, ml);
  for( size_t i = 0; i < ml.size(); i++ )
    _upd_hidden(h, ml[i].c_str());
}

// Удаление старых версий
static
void _upd_cleanup(HDAEMON *h) {
  std::wstring filter = GetOmobusRootPath(); filter += L"~*~*";
  // Удаляем все файлы подпадающие под фильтр
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      std::wstring del = GetOmobusRootPath(); del += FindFileData.cFileName;
      // Сбрасываем атрибуты удаляемого файла
      SetFileAttributes(del.c_str(), FILE_ATTRIBUTE_NORMAL);
      // Удаляем файл
      if( DeleteFile(del.c_str()) == FALSE ) {
        RETAILMSG(TRUE, (L"Unable to delete '%s': %s\n",
            del.c_str(), fmtoserr().c_str()));
      }
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
}

// Сохранение активности "Обновление справочников"
static
void StoreActivity(HDAEMON *h, int status) {
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_USERACT_UPD, true);
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

// Поток автономной отправки данных
static 
DWORD CALLBACK UpdProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL || h->srv_path == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, TRUE, OMOBUS_UPD_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_UPD_NOTIFY));
    return 1;
  }

  CEnsureCloseHandle hReboot = CreateEvent(NULL, FALSE, FALSE, REBOOT_EVENT_NAME);
  if( hReboot.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", REBOOT_EVENT_NAME));
    return 1;
  }

  _upd_cleanup(h);
  _upd_hidden(h);
  RemoveTrigger(REBOOT_EVENT_NAME);

  bool exit = false;
  DWORD dwTimeout = INFINITE;
  while(!exit) {
    HANDLE hEv[] = { h->hKill, hNotify, hReboot };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, dwTimeout) ) {
    case WAIT_OBJECT_0:
      exit = true; 
      break;
    case WAIT_OBJECT_0 + 1: {
        CleanupPopupNotify(false);
        h->packets = 0;
        bool reboot = false;
        int installed = 0, rc = OMOBUS_OK;
        const wchar_t *path = GetOmobusProfilePath(OMOBUS_PROFILE_UPD);
        if( (rc = h->recv(h->srv_path, path, &h->packets)) == OMOBUS_OK ) {
          if( (installed = _upd_install(h, path, reboot)) > 0 ) {
            if( reboot ) {
              InitRebootNotify(h);
              SetTimeoutTrigger(REBOOT_EVENT_NAME, h->reboot*60);
            }
          }
          _upd_junk_control(h, path);
        } else {
          InitWarningNotify(h);
          // Через 180 сек. уведомление принудительно удаляется
          dwTimeout = 180000;
        }
        _upd_cleanup(h);
        _upd_hidden(h);
        if( h->packets > 0 || rc == OMOBUS_ERR || installed > 0 )
          StoreActivity(h, rc);
        h->packets = 0;
      }
      SetTimeoutTrigger(OMOBUS_UPD_NOTIFY, h->wakeup*60);
      ResetEvent(hNotify);
      break;
    case WAIT_OBJECT_0 + 2:
      RETAILMSG(TRUE, (L">>> REBOOTING DEVICE <<<\n"));
      ExecProcess(wsRebootTask, NULL, FALSE);
      break;
    case WAIT_TIMEOUT:
      CleanupPopupNotify(false);
      dwTimeout = INFINITE;
      break;
    default:
      RETAILMSG(TRUE, (L"UpdProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_UPD_NOTIFY);
  RemoveTrigger(REBOOT_EVENT_NAME);
  CleanupPopupNotify(true);

  return 0;
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
  h->wakeup = _ttoi(conf(UPD_WAKEUP, UPD_WAKEUP_DEF));
  h->srv_path = conf(UPD_SRV_PATH, UPD_SRV_PATH_DEF);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->reboot = _ttoi(conf(UPD_REBOOT, UPD_REBOOT_DEF));
  h->junk = wcsistrue(conf(UPD_JUNK, UPD_JUNK_DEF));
  h->hidden = conf(UPD_HIDDEN, NULL);
  h->time_fmt = conf(OMOBUS_MUI_TIME, ISO_TIME_FMT);

  _hResInstance = LoadMuiModule(_hInstance, conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  wcsncpy(wsBtnCap1, wsfromRes(IDS_HIDE).c_str(), CALC_BUF_SIZE(wsBtnCap1));
  wcsncpy(wsBtnCap2, wsfromRes(IDS_CLOSE).c_str(), CALC_BUF_SIZE(wsBtnCap2));
  wcsncpy(wsBtnCap3, wsfromRes(IDS_REBOOT).c_str(), CALC_BUF_SIZE(wsBtnCap3));
  _snwprintf(wsRebootTask, CALC_BUF_SIZE(wsRebootTask), L"%sreboot.exe", GetOmobusRootPath());

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThrTr = CreateThread(NULL, 0, UpdProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating UpdProc thread.\n"));
    }
    CloseHandle(h->hThrTr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    _hIcon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_TRAY));
    _hIcon1 = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ERR));
    DisableThreadLibraryCalls(_hInstance);
		break;
	case DLL_PROCESS_DETACH:
    _hResInstance = NULL;
		break;
	}

	return TRUE;
}
