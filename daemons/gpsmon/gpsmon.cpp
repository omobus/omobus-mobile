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
#include <stdlib.h>
#include <notify.h>
#include <aygshell.h>
#include <gpsapi.h>
#include <pm.h>
#include <string>
#include <vector>
#include <nmea/nmea.h>
#include <ensure_cleanup.h>
#include <critical_section.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

typedef struct _tagGPSMON {
  HANDLE hThrGps, hKill, hGpsPort, hGpsDev;
  bool empty_pkg, gpslog, trace;
  int gps_lifetime, wakeup, baud, read_timeout, max_dop;
  const wchar_t *user_id, *caption, *pk_ext, *pk_mode,
    *working_hours, *working_days, *port;
  char nmea_buf[1024*5];
  FILE *fgpslog;
  nmeaPARSER parser;
} HDAEMON;

typedef std::vector<omobus_location_t> trace_t;

static HINSTANCE _hInstance = NULL;
static const wchar_t *_days_abbrev[] = {
  L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
static const wchar_t _resume_uid[] = 
  L"F423B8E4-A269-4bac-B21F-B6B030D00517";

static 
time_t nmeaTIME_2_tt(const nmeaTIME *nmea) {
  tm t; memset(&t, 0, sizeof(t));
  t.tm_year = nmea->year;
  t.tm_mon = nmea->mon;
  t.tm_mday = nmea->day;
  t.tm_hour = nmea->hour;
  t.tm_min = nmea->min;
  t.tm_sec = nmea->sec;
  return omobus_utc_mktime(&t);
}

static
void nmeaError(const char *str, int str_size) {
  RETAILMSG(TRUE, (L"NMEA error: %hs\n", str));
}

static
std::wstring gpslogName() {
  std::wstring fname;
  fname  = GetOmobusProfilePath(OMOBUS_PROFILE_CACHE);
  fname += OMOBUS_GPSLOG_J;
  return fname;
}

static
std::wstring wsfromRes(UINT uID) {
  wchar_t buf[4095+1] = L"";
  LoadString(_hInstance, uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
bool IsWorkingTime(const wchar_t *working_days, const wchar_t *working_time, 
  time_t tt) {
  tm lt = {0}; localtime_r(&tt, &lt);
  // Используем фильтр по дням
  if( !(working_days == NULL || working_days[0] == L'\0') &&
      lt.tm_wday <= 6 &&
      wcsstri(working_days, _days_abbrev[lt.tm_wday]) == NULL )
    return false;
  // Используем фильтр по времени
  if( !(working_time == NULL || wcslen(working_time) != 11) ) {
    // Вытаскиваем время начала и завершения рабочего дня. Причем должнеы
    // быть заданы оба значения. 
    wchar_t buf[6] = L""; _snwprintf(buf, 5, L"%02i:%02i", lt.tm_hour, lt.tm_min);
    if( !(wcsncmp(buf, &working_time[0], 5) > 0 && 
          wcsncmp(buf, &working_time[6], 5) < 0) )
      return false;
  }
  return true;
}

static
int GetResumeTimeout(HDAEMON *h, bool wtime) {
  return wtime ? 15 : 120;
}

static
void StoreGpsDevState(HDAEMON *h, const wchar_t *state, const wchar_t *msg=NULL) 
{
  if( h->user_id == NULL )
    return;
  std::wstring xml = wsfromRes(IDS_GPSMON_ACT_GPS_STATE);
  if( xml.empty() )
    return;
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%state%", state);
  wsrepl(xml, L"%port%", h->port != NULL ? h->port : L"");
  wsrepl(xml, L"%msg%", msg != NULL ? msg : L"");

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_GPS_STATE, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str());
}

static
bool WritePosPk(HDAEMON *h, time_t tt, int seconds, omobus_location_t *l, 
  trace_t *trace) 
{
  std::wstring 
    xml_b = wsfromRes(IDS_GPSMON_ACT_GPS_POS_B),
    xml_t = wsfromRes(IDS_GPSMON_ACT_GPS_POS_T),
    xml_e = wsfromRes(IDS_GPSMON_ACT_GPS_POS_E);
  if( xml_b.empty() || xml_t.empty() || xml_e.empty() )
    return false;

  wsrepl(xml_b, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml_b, L"%dev_id%", GetOmobusId());
  wsrepl(xml_b, L"%user_id%", h->user_id);
  wsrepl(xml_b, L"%seconds%", itows(seconds).c_str());
  wsrepl(xml_b, L"%fix_dt%", wsftime(tt).c_str());
  wsrepl(xml_b, L"%satellite_dt%", wsftime(l->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml_b, L"%latitude%", ftows(l->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml_b, L"%longitude%",ftows(l->lon, OMOBUS_GPS_PREC).c_str());
  if( h->trace && !trace->empty() ) {
    for( size_t i = 0; i < trace->size(); i++ ) {
      std::wstring tmp_xml = xml_t;
      omobus_location_t *tmp_l = &(*trace)[i];
      wsrepl(tmp_xml, L"%trace_no%", itows(i).c_str());
      wsrepl(tmp_xml, L"%fix_dt%", wsftime(tmp_l->cur).c_str());
      wsrepl(tmp_xml, L"%satellite_dt%", wsftime(tmp_l->utc, ISO_DATETIME_FMT, true).c_str());
      wsrepl(tmp_xml, L"%latitude%", ftows(tmp_l->lat, OMOBUS_GPS_PREC).c_str());
      wsrepl(tmp_xml, L"%longitude%", ftows(tmp_l->lon, OMOBUS_GPS_PREC).c_str());
      xml_b += tmp_xml;
    }
  }
  xml_b += xml_e;

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_GPS_POS, 
    h->user_id, h->pk_ext, h->pk_mode, xml_b.c_str()) == OMOBUS_OK;
}

static
bool WritePosJournal(HDAEMON *h, time_t tt, int seconds, omobus_location_t *l, 
  trace_t *trace, bool pk) 
{
  std::wstring txt = wsfromRes(IDS_J_GPSMON_ACT_GPS_POS), templ = txt;
  if( txt.empty() )
    return false;

  wsrepl(txt, L"%fix_dt%", wsftime(tt).c_str());
  wsrepl(txt, L"%seconds%", itows(seconds).c_str());
  wsrepl(txt, L"%trace%", itows(trace->size()).c_str());
  wsrepl(txt, L"%pk%", pk?L"yes":L"no");
  wsrepl(txt, L"%satellite_dt%", wsftime(l->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(txt, L"%latitude%", ftows(l->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(txt, L"%longitude%", ftows(l->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(txt, L"%pdop%", ftows(l->pdop, 2).c_str());
  wsrepl(txt, L"%hdop%", ftows(l->hdop, 2).c_str());
  wsrepl(txt, L"%vdop%", ftows(l->vdop, 2).c_str());
  
  return WriteOmobusJournal(OMOBUS_GPS_POS, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

static
void StoreGpsPos(HDAEMON *h, omobus_location_t *l, trace_t *trace) 
{
  if( h->user_id != NULL ) {
    int seconds = 0;
    time_t tt = omobus_time();
    omobus_location_t ll = {0};
    if( l->location_status == 1 /*1 - valid*/ ) {
      memcpy(&ll, l, sizeof(omobus_location_t));
    } else if( !trace->empty() ) {
      memcpy(&ll, &trace->back(), sizeof(omobus_location_t));
      seconds = tt - trace->back().cur;
    } else {
      memset(&ll, 0, sizeof(omobus_location_t));
      if( !h->empty_pkg )
        return;
    }
    WritePosJournal(h, tt, seconds, &ll, trace, WritePosPk(h, tt, seconds, &ll, trace));
  }
  trace->clear();
}

static
bool gpsapi_OpenGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsDev != NULL ) {
    return true;
  }
  if( (h->hGpsDev = GPSOpenDevice(NULL, NULL, NULL, NULL)) == NULL ) {
    if( l->gpsmon_status != 3 )
      StoreGpsDevState(h, L"failed", fmtoserr().c_str());
    l->gpsmon_status = 3;
    l->location_status = 0;
    l->lat = l->lon = l->elv = l->speed = 0.0;
    l->cur = l->utc = 0;
    l->sat_inview = l->sat_inuse = 0;
    l->pdop = l->hdop = l->vdop = 0.0;
  } else {
    StoreGpsDevState(h, L"on");
    l->gpsmon_status = 1;
  }

  return h->hGpsDev != NULL;
}

static
bool comport_OpenGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsPort != INVALID_HANDLE_VALUE )
    return true;

  DCB dcb;
  COMMTIMEOUTS t;
  DWORD rd = 0;
  char buf[10] = "";
  bool success = false;
  memset(&dcb, 0, sizeof(dcb));
  memset(&t, 0, sizeof(COMMTIMEOUTS));
  dcb.DCBlength = sizeof(dcb);

  h->hGpsPort = CreateFile(h->port, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
    FILE_ATTRIBUTE_NORMAL, NULL);
  if( h->hGpsPort != INVALID_HANDLE_VALUE ) {
    if( GetCommState(h->hGpsPort, &dcb) == TRUE && 
        GetCommTimeouts(h->hGpsPort, &t) == TRUE ) {
      dcb.BaudRate = h->baud > 0 ? h->baud : dcb.BaudRate;
      t.ReadIntervalTimeout = h->read_timeout * 2;
      success = 
        SetCommState(h->hGpsPort, &dcb) &&
        SetCommTimeouts(h->hGpsPort, &t) &&
        ReadFile(h->hGpsPort, buf, 10, &rd, NULL);
    }
    if( success ) {
      StoreGpsDevState(h, L"on");
      l->gpsmon_status = 1;
    } else {
      if( l->gpsmon_status != 3 )
        StoreGpsDevState(h, L"failed", fmtoserr().c_str());
      CloseHandle(h->hGpsPort);
      h->hGpsPort = INVALID_HANDLE_VALUE;
      l->gpsmon_status = 3;
      l->location_status = 0;
      l->lat = l->lon = l->elv = l->speed = 0.0;
      l->cur = l->utc = 0;
      l->sat_inview = l->sat_inuse = 0;
      l->pdop = l->hdop = l->vdop = 0.0;
    }
  }

  return h->hGpsPort != INVALID_HANDLE_VALUE;
}

static
bool OpenGps(HDAEMON *h, omobus_location_t *l) {
  return h->port == NULL ? gpsapi_OpenGps(h, l) :
    comport_OpenGps(h, l);
}

static
void gpsapi_CloseGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsDev == NULL )
    return;
  GPSCloseDevice(h->hGpsDev);
  h->hGpsDev = NULL;
  StoreGpsDevState(h, L"off");
  l->gpsmon_status = 2;
  l->location_status = 0;
  l->lat = l->lon = l->elv = l->speed = 0.0;
  l->cur = l->utc = 0;
  l->sat_inview = l->sat_inuse = 0;
  l->pdop = l->hdop = l->vdop = 0.0;
}

static
void comport_CloseGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsPort == INVALID_HANDLE_VALUE )
    return;
  CloseHandle(h->hGpsPort);
  h->hGpsPort = INVALID_HANDLE_VALUE;
  StoreGpsDevState(h, L"off");
  l->gpsmon_status = 2;
  l->location_status = 0;
  l->lat = l->lon = l->elv = l->speed = 0.0;
  l->cur = l->utc = 0;
  l->sat_inview = l->sat_inuse = 0;
  l->pdop = l->hdop = l->vdop = 0.0;
}

static
void CloseGps(HDAEMON *h, omobus_location_t *l) {
  h->port == NULL ? gpsapi_CloseGps(h, l) :
    comport_CloseGps(h, l);
}

static
bool gpsapi_ReadGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsDev == NULL ) {
    return false;
  }

  DWORD dwErr = 0;
  tm t = {0};
  GPS_POSITION GPSPos;
  memset(&GPSPos, 0, sizeof(GPSPos));
  GPSPos.dwVersion = GPS_VERSION_1;
  GPSPos.dwSize = sizeof(GPSPos);
  
  if( (dwErr = GPSGetPosition(h->hGpsDev, &GPSPos, h->read_timeout*1000, 0))
          != ERROR_SUCCESS ) {
    if( l->gpsmon_status != 4 )
      StoreGpsDevState(h, L"failed", fmtoserr().c_str());
    gpsapi_CloseGps(h, l);
    l->gpsmon_status = 4;
    return false;
  }

  l->gpsmon_status = 1;
  if( (GPSPos.dwValidFields & GPS_VALID_UTC_TIME) &&
      (GPSPos.dwValidFields & GPS_VALID_LATITUDE) && 
      (GPSPos.dwValidFields & GPS_VALID_LONGITUDE) &&
      GPSPos.dwFlags != GPS_DATA_FLAGS_HARDWARE_OFF && 
      GPSPos.FixQuality > GPS_FIX_QUALITY_UNKNOWN &&
      GPSPos.FixType > GPS_FIX_UNKNOWN && 
      GPSPos.flPositionDilutionOfPrecision <= (float)h->max_dop && 
      GPSPos.flHorizontalDilutionOfPrecision <= (float)h->max_dop && 
      GPSPos.flVerticalDilutionOfPrecision <= (float)h->max_dop ) {
    l->location_status = 1;
    l->cur = omobus_time();
    l->utc = omobus_utc_mktime(st2tm(&GPSPos.stUTCTime, &t));
    l->lat = GPSPos.dblLatitude;
    l->lon = GPSPos.dblLongitude;
    l->elv = GPSPos.flAltitudeWRTSeaLevel;
    l->speed = GPSPos.flSpeed;
    l->sat_inview = (byte_t)GPSPos.dwSatellitesInView;
    l->sat_inuse = (byte_t)GPSPos.dwSatelliteCount;
    l->pdop = GPSPos.flPositionDilutionOfPrecision;
    l->hdop = GPSPos.flHorizontalDilutionOfPrecision;
    l->vdop = GPSPos.flVerticalDilutionOfPrecision;
  } else if( l->cur > 0 && l->utc > 0 && 
             (((time_t)h->gps_lifetime*60) >= (omobus_time() - l->cur)) ) {
    l->location_status = 2;
  } else {
    l->location_status = 0;
    l->lat = l->lon = l->elv = l->speed = 0.0;
    l->cur = l->utc = 0;
    l->sat_inview = l->sat_inuse = 0;
    l->pdop = l->hdop = l->vdop = 0.0;
  }

  return true;
}

static
bool comport_ReadGps(HDAEMON *h, omobus_location_t *l) {
  if( h->hGpsPort == INVALID_HANDLE_VALUE )
    return false;

  DWORD rd = 0;
  BOOL r = FALSE;
  nmeaINFO info;
  nmeaPOS dpos;

  memset(h->nmea_buf, 0, sizeof(h->nmea_buf));
  memset(&dpos, 0, sizeof(dpos));
  nmea_zero_INFO(&info);

  if( (r = ReadFile(h->hGpsPort, h->nmea_buf, sizeof(h->nmea_buf) - 1, &rd, NULL))
              == FALSE ) {
    if( l->gpsmon_status != 4 )
      StoreGpsDevState(h, L"failed", fmtoserr().c_str());
    comport_CloseGps(h, l);
    l->gpsmon_status = 4;
    return false;
  }

  l->gpsmon_status = 1;
  if( rd > 0 ) {
    h->nmea_buf[rd] = '\0';
    if( h->fgpslog != NULL ) {
      fwrite(h->nmea_buf, 1, rd, h->fgpslog);
      fflush(h->fgpslog);
    }
    nmea_parse(&h->parser, h->nmea_buf, rd, &info);
  }
  if( /*info.smask ???? */
      info.sig > NMEA_SIG_BAD && info.sig <= NMEA_SIG_HIGH &&
      info.fix > NMEA_FIX_BAD && info.fix <= NMEA_FIX_3D &&
      info.utc.day > 0 && 
      info.PDOP <= (double)h->max_dop && 
      info.HDOP <= (double)h->max_dop && 
      info.VDOP <= (double)h->max_dop ) {
    l->location_status = 1;
    l->cur = omobus_time();
    l->utc = nmeaTIME_2_tt(&info.utc);
    nmea_info2pos(&info, &dpos);
    l->lat = dpos.lat*180/3.14159265;
    l->lon = dpos.lon*180/3.14159265;
    l->elv = info.elv;
    l->speed = info.speed;
    l->sat_inview = info.satinfo.inview;
    l->sat_inuse = info.satinfo.inuse;
    l->pdop = info.PDOP;
    l->hdop = info.HDOP;
    l->vdop = info.VDOP;
  } else if( l->cur > 0 && l->utc > 0 && 
             (((time_t)h->gps_lifetime*60) >= (omobus_time() - l->cur)) ) {
    l->location_status = 2;
  } else {
    l->location_status = 0;
    l->lat = l->lon = l->elv = l->speed = 0.0;
    l->cur = l->utc = 0;
    l->sat_inview = l->sat_inuse = 0;
    l->pdop = l->hdop = l->vdop = 0.0;
  }

  return true;
}

static
bool ReadGps(HDAEMON *h, omobus_location_t *l) {
  return h->port == NULL ? gpsapi_ReadGps(h, l) :
    comport_ReadGps(h, l);
}

static 
DWORD CALLBACK GpsProc(LPVOID pParam) {
  HDAEMON *h = (HDAEMON *)pParam;
  if( h == NULL )
    return 1;

  trace_t trace; trace.reserve(500);
  omobus_location_t l = {0}; memset(&l, 0, sizeof(l));
  bool pk = false, wtime = false;
  DWORD dwWaitStatus = 0;

  CEnsureCloseHandle hNotify = CreateEvent(NULL, FALSE, TRUE, OMOBUS_GPSMON_NOTIFY),
    hLock = CreateEvent(NULL, FALSE, TRUE, OMOBUS_GPS_POS_LOCK_NAME),
    hResume = CreateEvent(NULL, FALSE, TRUE, _resume_uid);
  if( hLock.IsInvalid() || hNotify.IsInvalid() || hResume.IsInvalid() ) {
    RETAILMSG(TRUE, (L"GpsProc: Unable to create events!\n"));
    return 1;
  }

  CEnsureCloseHandle hMap = CreateFileMapping(INVALID_HANDLE_VALUE, 
    NULL, PAGE_READWRITE, 0, sizeof(omobus_location_t), OMOBUS_GPS_POS_SHMEM_NAME);
  if( hMap.IsInvalid() ) {
    RETAILMSG(TRUE, (L"GpsProc: Unable to create shared memory object!\n"));
    return 1;
  }

  CEnsureUnmapViewOfFile pMap = MapViewOfFile(hMap, FILE_MAP_READ|FILE_MAP_WRITE, 
    0, 0, 0);
  if( pMap.IsInvalid() ) {
    RETAILMSG(TRUE, (L"GpsProc: Unable to create shared memory pointer!\n"));
    return 1;
  }

  while( 1 ) {
    HANDLE hEv[] = { h->hKill, hNotify, hResume };
    dwWaitStatus = WaitForMultipleObjects(sizeof(hEv)/sizeof(hEv[0]), hEv, FALSE, 
      h->read_timeout*1000);
    wtime = IsWorkingTime(h->working_days, h->working_hours, omobus_time());
    // Инициация процедуры генерации пакета.
    if( dwWaitStatus == WAIT_OBJECT_0 + 1 ) {
      if( SetTimeoutTrigger(OMOBUS_GPSMON_NOTIFY, h->wakeup*60) != OMOBUS_OK ) {
        RETAILMSG(TRUE, (L"Unable to set '%s' wakeup trigger.\n", 
          OMOBUS_GPSMON_NOTIFY));
      }
      pk = wtime && h->wakeup > 0;
      DEBUGMSG(TRUE, (L"GpsProc: Notify.\n"));
    // Удержание устройства от сна
    } else if( dwWaitStatus == WAIT_OBJECT_0 + 2 ) {
      if( SetTimeoutTrigger(_resume_uid, GetResumeTimeout(h, wtime)) != OMOBUS_OK ) {
        RETAILMSG(TRUE, (L"Unable to set '%s' wakeup trigger.\n", 
          _resume_uid));
      }
      //SystemIdleTimerReset();
      //keybd_event(VK_F24, 0, KEYEVENTF_KEYUP | KEYEVENTF_SILENT, 0);
      DEBUGMSG(TRUE, (L"GpsProc: Wakeup/[%i sec].\n", GetResumeTimeout(h, wtime)));
    // Получение данных с GPS-ресивера.
    } else if( dwWaitStatus == WAIT_TIMEOUT ) {
      if( wtime ) {
        if( !(OpenGps(h, &l) && ReadGps(h, &l)) ) {
          DEBUGMSG(TRUE, (L"GpsProc: Unable to open/read GPS-port.\n"));
        } else {
          if( l.location_status == 1 /*1 - valid*/ ) {
            if( h->wakeup > 0 ) {
              trace.push_back(l);
            }
            DEBUGMSG(TRUE, (L"GpsProc: Pos: la=%s, lo=%s, utc='%s', cur='%s'\n",
              ftows(l.lat, OMOBUS_GPS_PREC).c_str(), 
              ftows(l.lon, OMOBUS_GPS_PREC).c_str(),
              wsftime(l.utc, ISO_DATETIME_FMT, true).c_str(),
              wsftime(l.cur).c_str() ));
          }
        }
        // Формирование пакета
        if( pk ) {
          StoreGpsPos(h, &l, &trace);
          pk = false;
          DEBUGMSG(TRUE, (L"GpsProc: StoreGpsPos\n"));
        }
      } else {
        pk = false;
        CloseGps(h, &l);
      }
      // *************************************************
      // Запись данных для внешних модулей
      if( WaitForSingleObject(hLock, 1500) == WAIT_TIMEOUT ) {
        RETAILMSG(TRUE, (L"GpsProc: >> PANIC << Unable to lock shared region.\n"));
      } else {
        memcpy((LPVOID)pMap, &l, sizeof(l));
        SetEvent(hLock);
      }
      // *************************************************
    } else {
      DEBUGMSG(TRUE, (L"GpsProc: Exiting thread\n"));
      break;
    }
  }

  RemoveTrigger(OMOBUS_GPSMON_NOTIFY);
  RemoveTrigger(_resume_uid);
  CloseGps(h, &l);
  pMap = NULL;
  hMap = NULL;
  hLock = NULL;
  
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
  h->hGpsPort = INVALID_HANDLE_VALUE;
  h->user_id = conf(OMOBUS_USER_ID, NULL);
  h->caption = conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  h->gps_lifetime = _wtoi(conf(OMOBUS_GPSMON_LIFETIME, OMOBUS_GPSMON_LIFETIME_DEF));
  h->wakeup = _wtoi(conf(OMOBUS_GPSMON_WAKEUP, OMOBUS_GPSMON_WAKEUP_DEF));
  h->working_hours = conf(OMOBUS_WORKING_HOURS, NULL);
  h->working_days = conf(OMOBUS_WORKING_DAYS, NULL);
  h->pk_ext = conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->port = conf(OMOBUS_GPSMON_PORT, NULL);
  h->baud = _wtoi(conf(OMOBUS_GPSMON_BAUD, OMOBUS_GPSMON_BAUD_DEF));
  h->empty_pkg = wcsistrue(conf(OMOBUS_GPSMON_EMPTY_PKG, OMOBUS_GPSMON_EMPTY_PKG_DEF));
  h->read_timeout = _wtoi(conf(OMOBUS_GPSMON_READ_TIMEOUT, OMOBUS_GPSMON_READ_TIMEOUT_DEF));
  h->max_dop = _wtoi(conf(OMOBUS_GPSMON_MAX_DOP, OMOBUS_GPSMON_MAX_DOP_DEF));
  h->gpslog = wcsistrue(conf(OMOBUS_GPSMON_GPSLOG, OMOBUS_GPSMON_GPSLOG_DEF));
  h->trace = wcsistrue(conf(OMOBUS_GPSMON_TRACE, OMOBUS_GPSMON_TRACE_DEF));

  if( h->read_timeout < 1 )
    h->read_timeout = 2;

  if( h->gpslog ) {
    h->fgpslog = _wfopen(gpslogName().c_str(), L"ab");
    if( h->fgpslog == NULL ) {
      RETAILMSG(TRUE, (L"GpsProc: Unable to open GPS-journal.\n"));
    }
  }

  if( h->port != NULL ) {
    if( h->port[0] == L'\0' || wcsicmp(h->port, L"gpsapi") == 0 || wcsicmp(h->port, L"gpsapi:") == 0 )
      h->port = NULL;
  }

  if( h->port != NULL ) {
    //DEBUG: nmea_property()->error_func = &nmeaError;
    nmea_parser_init(&h->parser);
  }

  h->hKill = CreateEvent(NULL, TRUE, FALSE, NULL);
  h->hThrGps = CreateThread(NULL, 0, GpsProc, h, 0, NULL);

  return h;
}

void stop_daemon(void *_h) 
{
  HDAEMON *h = (HDAEMON *)_h;
  if( h == NULL )
    return;
  if( h->hKill != NULL )
    SetEvent(h->hKill);
  if( h->hThrGps != NULL ) {
    if( WaitForSingleObject(h->hThrGps, 5000) == WAIT_TIMEOUT ) {
      TerminateThread(h->hThrGps, -1);
      RETAILMSG(TRUE, (L"Terminating GpsProc thread.\n"));
    }
    CloseHandle(h->hThrGps);
    h->hThrGps = NULL;
  }
  if( h->hKill != NULL )
    CloseHandle(h->hKill);
  h->hKill = NULL;  
  chk_fclose(h->fgpslog);
  nmea_parser_destroy(&h->parser);
  delete h;
}
