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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include <windows.h>
#include <notify.h>
#include <stdlib.h>
#include <string>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>

#define ACTS_WAKEUP        L"acts->wakeup"
#define ACTS_WAKEUP_DEF    L"0"
#define ACTS_SRV_PATH      L"transport->acts->path"
#define ACTS_SRV_PATH_DEF  L"acts/"

typedef struct _tagMODULE {
  HANDLE hThrTr, hKill;
  int wakeup;
  const wchar_t *srv_path;
  pf_daemons_pk_send send;
  uint32_t packets;
} HDAEMON;

static HINSTANCE _hInstance = NULL;

static 
DWORD CALLBACK ActProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL || h->srv_path == NULL )
    return 1;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, TRUE, TRUE, OMOBUS_ACTS_NOTIFY);
  if( hNotify.IsInvalid() ) {
    RETAILMSG(TRUE, (L"Unable to create event '%s'!\n", OMOBUS_ACTS_NOTIFY));
    return 1;
  }

  bool exit = false;
  while(!exit) {
    HANDLE hEv[] = {h->hKill, hNotify };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) ) {
    case WAIT_OBJECT_0:
      exit = true; 
      break;
    case WAIT_OBJECT_0 + 1:
      h->send(GetOmobusProfilePath(OMOBUS_PROFILE_ACTS), h->srv_path, &h->packets);
      SetTimeoutTrigger(OMOBUS_ACTS_NOTIFY, h->wakeup*60);
      ResetEvent(hNotify);
      break;
    default:
      RETAILMSG(TRUE, (L"ActProc: WaitForMultipleObjects = Error(%i)\n", GetLastError()));
      exit = true;
    };
  }

  RemoveTrigger(OMOBUS_ACTS_NOTIFY);

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
  h->send = tr->pk_send;
  h->wakeup = _ttoi(conf(ACTS_WAKEUP, ACTS_WAKEUP_DEF));
  h->srv_path = conf(ACTS_SRV_PATH, ACTS_SRV_PATH_DEF);

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThrTr = CreateThread(NULL, 0, ActProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating ActProc thread.\n"));
    }
    CloseHandle(h->hThrTr);
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  delete h;
}
