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
#include <stdlib.h>
#include <ensure_cleanup.h>
#include <omobus-mobile.h>

#define TIMESYNC_WAKEUP         L"timesync->wakeup"
#define TIMESYNC_WAKEUP_DEF     L"10"

#define Int32x32To64(a, b) ((LONGLONG)((LONG)(a)) * (LONGLONG)((LONG)(b)))

typedef struct _tagGPSMON {
  HANDLE hThr, hKill;
  int wakeup;
} HDAEMON;

static HINSTANCE _hInstance = NULL;

static
LPFILETIME TimeToFileTime(time_t t, LPFILETIME pft)
{
    LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD)(ll >> 32);
    return pft;
}

static
LPSYSTEMTIME TimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
    FILETIME ft; 
    TimeToFileTime(t, &ft);
    FileTimeToSystemTime(&ft, pst);
    return pst;
}

/*
static
time_t TimeFromFileTime(const FILETIME* filetime)
{
    LONGLONG longlong = filetime->dwHighDateTime;
    longlong <<= 32;
    longlong |= filetime->dwLowDateTime;
    longlong -= 116444736000000000;
    return (time_t) (longlong / 10000000);
}

static
time_t TimeFromSystemTime(const SYSTEMTIME* systemtime)
{
    FILETIME filetime;
    SystemTimeToFileTime(systemtime, &filetime);
    return TimeFromFileTime(&filetime);
}
*/

static 
void UpdateSysTime(time_t tt)
{
  SYSTEMTIME st = {0};
	SetSystemTime(TimeToSystemTime(tt, &st));
}

static
void gpsSyncTime() {
  omobus_location_t l;
  for( int i = 0; i < 40; i++ ) {
    omobus_location(&l);
    if( l.gpsmon_status == 0 || l.location_status == 0 || l.location_status == 1 )
      break;
    Sleep(500);
  }
  if( l.location_status == 1 /*1 - valid*/ ) {
    if( abs(l.utc - omobus_time()) > 40 ) {
      UpdateSysTime(l.utc);
      DEBUGMSG(TRUE, (L"Time has been synchronized via GPS.\n"));
    }
  }
}

static 
DWORD CALLBACK TimeSyncProc(LPVOID pParam) 
{
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  CEnsureCloseHandle hResume;

  hResume = CreateEvent(NULL, FALSE, TRUE, OMOBUS_TIMESYNC_NOTIFY);
  if( hResume.IsInvalid() ) {
    RETAILMSG(TRUE, (L"TimeSyncProc: Unable to create events!\n"));
    return 1;
  }

  while( 1 ) {
    HANDLE hEv[] = { h->hKill, hResume };
    if( WaitForMultipleObjects(sizeof(hEv)/sizeof(hEv[0]), hEv, FALSE, -1)
          == WAIT_OBJECT_0 + 1 ) {
      gpsSyncTime();
      if( h->wakeup != 0 ) {
        if( SetTimeoutTrigger(OMOBUS_TIMESYNC_NOTIFY, h->wakeup*60) != OMOBUS_OK ) {
          RETAILMSG(TRUE, (L"TimeSyncProc: Unable to set '%s' wakeup trigger. Timeout: %i min\n", 
            OMOBUS_TIMESYNC_NOTIFY, h->wakeup));
        }
      }
      ResetEvent(hResume);
    } else {
      DEBUGMSG(TRUE, (L"TimeSyncProc: Exiting thread\n"));
      break;
    }
  }

  RemoveTrigger(OMOBUS_TIMESYNC_NOTIFY);
  hResume = NULL;
  
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
  h->wakeup = _wtoi(conf(TIMESYNC_WAKEUP, TIMESYNC_WAKEUP_DEF));

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThr = CreateThread(NULL, 0, TimeSyncProc, h, 0, NULL);

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
      RETAILMSG(TRUE, (L"Terminating TimeSyncProc thread.\n"));
    }
    CloseHandle(h->hThr);
    h->hThr = NULL;
  }
  if( h->hKill != NULL ) {
    CloseHandle(h->hKill);
    h->hKill = NULL;
  }
  memset(h, 0, sizeof(HDAEMON));
  delete h;
}
