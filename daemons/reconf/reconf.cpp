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

#define OMOBUS_RECONF_WAKEUP        L"reconf->wakeup"
#define OMOBUS_RECONF_WAKEUP_DEF    L"0"
#define OMOBUS_RECONF_ONSTARTUP     L"reconf->onstartup"
#define OMOBUS_RECONF_ONSTARTUP_DEF L"yes"

typedef struct _tagMODULE {
  HANDLE hThr, hKill;
  int wakeup;
  uint32_t packets;
} HDAEMON;

static HINSTANCE _hInstance = NULL;

static
void _reconf_device(HDAEMON *h) {
  std::wstring filter = GetOmobusRootPath(); filter += L"*.wpx";
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind = FindFirstFile(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      std::wstring fn = GetOmobusRootPath(); fn += FindFileData.cFileName;
      if( !ProcessXMLConfigFile(fn.c_str()) ) {
        RETAILMSG(TRUE, (L"svc_reconf: '%s' - FAILED\n", fn.c_str()));
      } else {
        RETAILMSG(TRUE, (L"svc_reconf: '%s' - success\n", fn.c_str()));
      }
    } while( FindNextFile(hFind, &FindFileData) );
    FindClose(hFind);
  }
}

static 
DWORD CALLBACK ReconfProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, FALSE, OMOBUS_RECONF_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_RECONF_NOTIFY));
    return 1;
  }

  SetTimeoutTrigger(OMOBUS_RECONF_NOTIFY, h->wakeup*60);

  bool exit = false; // Флаг выхода их процедуры отправки данных
  while(!exit) {
    HANDLE hEv[] = {h->hKill, hNotify };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) ) {
    case WAIT_OBJECT_0:
      exit = true; 
      break;
    case WAIT_OBJECT_0 + 1:
      _reconf_device(h);
      SetTimeoutTrigger(OMOBUS_RECONF_NOTIFY, h->wakeup*60);
      ResetEvent(hNotify);
      break;
    default:
      RETAILMSG(TRUE, (L"ReconfProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_RECONF_NOTIFY);

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
  h->wakeup = _ttoi(conf(OMOBUS_RECONF_WAKEUP, OMOBUS_RECONF_WAKEUP_DEF));

  if( wcsistrue(conf(OMOBUS_RECONF_ONSTARTUP, OMOBUS_RECONF_ONSTARTUP_DEF)) )
    _reconf_device(h);

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThr = CreateThread(NULL, 0, ReconfProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating ReconfProc thread.\n"));
    }
    CloseHandle(h->hThr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  memset(h, 0, sizeof(HDAEMON));
  delete h;
}
