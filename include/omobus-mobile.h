/* -*- H -*- */
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

#ifndef __libomobus_h__
#define __libomobus_h__

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <string>

#define ISO_DATETIME_FMT          L"%Y-%m-%d %H:%M:%S"
#define ISO_DATE_FMT              L"%Y-%m-%d"
#define ISO_TIME_FMT              L"%H:%M:%S"
#define FILE_DATETIME_FMT         L"%Y-%m-%dT%H-%M-%S"

#define ISO_DATETIME_EMPTY        L"0000-00-00 00:00:00"
#define POS_LAT_EMPTY             L"0"
#define POS_LON_EMPTY             L"0"

#define OMOBUS_OK                 0
#define OMOBUS_ERR                -1

#define OMOBUS_CODE               L"OMOBUS"

#define OMOBUS_MD5_BYTES 					16
#define OMOBUS_GPS_PREC           6
#define OMOBUS_WEIGHT_PREC        3
#define OMOBUS_VOLUME_PREC        3

#define OMOBUS_MAX_ID             38
#define OMOBUS_MAX_DESCR          64
#define OMOBUS_MAX_DATE           10
#define OMOBUS_MAX_SHORT_DESCR    16
#define OMOBUS_MAX_ADDR_DESCR     128
#define OMOBUS_MAX_NOTE           256
#define OMOBUS_MAX_PHONE          32
#define OMOBUS_UNKNOWN_ID         L""
#define OMOBUS_BORDERCOLOR        RGB(180, 180, 180)
#define OMOBUS_BGCOLOR					  GetSysColor(COLOR_WINDOW)
#define OMOBUS_HLCOLOR					  GetSysColor(COLOR_HIGHLIGHTTEXT)
#define OMOBUS_HLBGCOLOR					GetSysColor(COLOR_HIGHLIGHT)
#define OMOBUS_ODDBGCOLOR			    RGB(240, 240, 255)
#define OMOBUS_ALLERTCOLOR		    RGB(200, 0, 0)

#define OMOBUS_SRV_ONLINE         OMOBUS_CODE L"-daemons-host-online"
#define OMOBUS_TERM_ONLINE        OMOBUS_CODE L"-terminals-host-online"
#define OMOBUS_TERM_ACTIVATE      OMOBUS_CODE L"-terminals-host-activate"
#define OMOBUS_TERM_RELOAD        OMOBUS_CODE L"-terminals-host-reload"
#define OMOBUS_PC_SYNC_NOTIFY     OMOBUS_CODE L"-pc-sync"
#define OMOBUS_POWER_NOTIFY       OMOBUS_CODE L"-power"
#define OMOBUS_ACTS_NOTIFY        OMOBUS_CODE L"-acts"
#define OMOBUS_DOCS_NOTIFY        OMOBUS_CODE L"-docs"
#define OMOBUS_SYNC_NOTIFY        OMOBUS_CODE L"-sync"
#define OMOBUS_UPD_NOTIFY         OMOBUS_CODE L"-upd"
#define OMOBUS_RECONF_NOTIFY      OMOBUS_CODE L"-reconf"
#define OMOBUS_TIMESYNC_NOTIFY    OMOBUS_CODE L"-timesync"
#define OMOBUS_GPSMON_NOTIFY      OMOBUS_CODE L"-gpsmon"
#define OMOBUS_GPS_POS_SHMEM_NAME OMOBUS_CODE L"-gps-pos-shmem"
#define OMOBUS_GPS_POS_LOCK_NAME  OMOBUS_CODE L"-gps-pos-lock"

#define OMOBUS_TRANSPORT_J        L"transport"
#define OMOBUS_GPSLOG_J           L"gpslog"
#define OMOBUS_LAST_SYNC_J        L"sync"

#define OMOBUS_SYS_POWER          L"sys_power"
#define OMOBUS_SYS_PC_SYNC        L"sys_pc_sync"
#define OMOBUS_SYS_PROC           L"sys_proc"
#define OMOBUS_SYS_HARDWARE       L"sys_hardware"
#define OMOBUS_EXCHANGE           L"exchange"
#define OMOBUS_USER_WORK          L"user_work"
#define OMOBUS_USER_ACTIVITY      L"user_activity"
#define OMOBUS_USER_DOCUMENT      L"user_document"
#define OMOBUS_GPS_STATE          L"gps_state"
#define OMOBUS_GPS_POS            L"gps_pos"
#define OMOBUS_NEW_ACCOUNT        L"new_account"
#define OMOBUS_ORDER              L"order"
#define OMOBUS_ORDER_T            L"order_t"
#define OMOBUS_RECLAMATION        L"reclamation"
#define OMOBUS_RECLAMATION_T      L"reclamation_t"
#define OMOBUS_PRESENCE           L"presence"
#define OMOBUS_COMMENT            L"comment"
#define OMOBUS_LOCATION           L"location"
#define OMOBUS_PENDING            L"pending"
#define OMOBUS_UNSCHED            L"unsched"
#define OMOBUS_CANCELING          L"canceling"
#define OMOBUS_NEW_CONTACT        L"new_contact"

#define OMOBUS_MUI_NAME           L"mui"
#define OMOBUS_MUI_NAME_DEF       L"RU"
#define OMOBUS_MUI_DATETIME       L"datetime"   // �������� ����/����� (��� ���.)
#define OMOBUS_MUI_DATETIME_F     L"datetime_f" // ������ ����/����� (� ���.)
#define OMOBUS_MUI_DATE           L"date"
#define OMOBUS_MUI_TIME           L"time"       // �������� ����� (��:��)
#define OMOBUS_MUI_TIME_F         L"time_f"     // ������ ����� (��:��:��)
#define OMOBUS_MUI_THOUSAND_SEP   L"thousand_sep"

#define OMOBUS_USER_ID            L"user_id"
#define OMOBUS_USER_KEY           L"user_key"
#define OMOBUS_USER_NAME          L"user_name"
#define OMOBUS_FIRM_NAME          L"firm_name"
#define OMOBUS_WORKING_HOURS      L"working_hours"
#define OMOBUS_WORKING_DAYS       L"working_days"
#define OMOBUS_PK_MODE            L"pk->mode"
#define OMOBUS_PK_MODE_DEF        L"gz"
#define OMOBUS_PK_LEVEL           L"pk->compression->level"
#define OMOBUS_PK_LEVEL_DEF       L"9"
#define OMOBUS_CAPTION            L"caption"
#define OMOBUS_CAPTION_DEF        OMOBUS_CODE
#define OMOBUS_CUR_PRECISION      L"currency_precision"
#define OMOBUS_CUR_PRECISION_DEF  L"2"
#define OMOBUS_QTY_PRECISION      L"quantity_precision"
#define OMOBUS_QTY_PRECISION_DEF  L"0"

#define OMOBUS_GPSMON_LIFETIME              L"gpsmon->lifetime"
#define OMOBUS_GPSMON_LIFETIME_DEF          L"20"
#define OMOBUS_GPSMON_WAKEUP                L"gpsmon->wakeup"
#define OMOBUS_GPSMON_WAKEUP_DEF            L"0"
#define OMOBUS_GPSMON_MAX_DOP               L"gpsmon->max_dop"
#define OMOBUS_GPSMON_MAX_DOP_DEF           L"9"
#define OMOBUS_GPSMON_PORT                  L"gpsmon->port"
#define OMOBUS_GPSMON_BAUD                  L"gpsmon->baud"
#define OMOBUS_GPSMON_BAUD_DEF              L"0"
#define OMOBUS_GPSMON_READ_TIMEOUT          L"gpsmon->read_timeout"
#define OMOBUS_GPSMON_READ_TIMEOUT_DEF      L"2"
#define OMOBUS_GPSMON_EMPTY_PKG             L"gpsmon->empty_pkg"
#define OMOBUS_GPSMON_EMPTY_PKG_DEF         L"no"
#define OMOBUS_GPSMON_GPSLOG                L"gpsmon->gpslog"
#define OMOBUS_GPSMON_GPSLOG_DEF            L"no"
#define OMOBUS_GPSMON_TRACE                 L"gpsmon->trace"
#define OMOBUS_GPSMON_TRACE_DEF             L"no"

// ������� �����
#define __INT_CURRENT_SCREEN            L"__int.current_screen"
// ����� �������� ��� � ����������.
#define __INT_W_COOKIE                  L"__int.w_cookie" 
#define __INT_A_COOKIE                  L"__int.a_cookie" 
// ���������� ���������� ��������� � ������ ����������. ������ ��������
// ������������� �������� ���������� ���������, �������� � ������������ 
// � 0 ������� act-mgr.
#define __INT_ACTIVITY_DOCS             L"__int.activity_docs" 
// ��������� ������� ����������
#define __INT_ACTIVITY_TYPE_ID          L"__int.activity_type_id" 
// ��������� ������ ������� ������ ���������� �� �������
#define __INT_SPLIT_ACTIVITY_SCREEN     L"__int.split_activity_screen"
// ��������� ���������� �������
#define __INT_ACCOUNT_ID                L"__int.account_id" 
#define __INT_ACCOUNT                   L"__int.account" 
#define __INT_ACCOUNT_ADDR              L"__int.account_addr" 
#define __INT_ACCOUNT_GR_PRICE_ID       L"__int.account_gr_price_id" 
#define __INT_ACCOUNT_PAY_DELAY         L"__int.account_pay_delay" 
#define __INT_ACCOUNT_LOCKED            L"__int.account_locked" 
#define __INT_DEL_DATE                  L"__int.delivery_date"
// ��������� ������
#define __INT_MERCH_BRAND_ID            L"__int.merch.brand_id"
#define __INT_MERCH_BRAND               L"__int.merch.brand_descr"

// ��������� �������� (����) ���� Startup Screen
#define OMOBUS_SCREEN_ROOT            0x01 /* ��������� ���a�, �� ��� ��� ���� �� ����� ������� ���� */
#define OMOBUS_SCREEN_WORK            0x02 /* ����� ������� ���� */
#define OMOBUS_SCREEN_ROUTE           0x04 /* ������������ ������� */
//==>#define OMOBUS_SCREEN_REPORTS         0x08 /* ������ ��������� � �������� */
#define OMOBUS_SCREEN_ACTIVITY        0x10 /* ���������� ��������� � �������� */
#define OMOBUS_SCREEN_ACTIVITY_TAB_1  0x20 /* �������������� ������� ��� OMOBUS_SCREEN_ACTIVITY */
#define OMOBUS_SCREEN_MERCH           0x40 /* ������������� */
#define OMOBUS_SCREEN_MERCH_TASKS     0x80 /* �������� ����������� � ������ �������������� */
#define OMOBUS_SCREEN_ANY             0xFF /* ����� */

// ��������� ���������� ��������� Startup Screen
#define OMOBUS_SSM_PLUGIN_SELECT    (WM_USER + 600)
#define OMOBUS_SSM_PLUGIN_ACTION    (WM_USER + 601)
#define OMOBUS_SSM_PLUGIN_SHOW      (WM_USER + 602)
#define OMOBUS_SSM_PLUGIN_HIDE      (WM_USER + 603)
#define OMOBUS_SSM_PLUGIN_RELOAD    (WM_USER + 604)

// ������������ ������ � ������������ ���������
#define chk_omobus_free(ptr)    if( ptr != NULL ) omobus_free(ptr); ptr = NULL;  
// �������� ����� � ������������ ���������
#define chk_fclose(ptr)         if( ptr != NULL ) fclose(ptr); ptr = NULL;
// �������� DLL � ������������ ���������
#define chk_dlclose(ptr)        if( ptr != NULL ) dlclose(ptr); ptr = NULL;
// ������������ ������ � ��������������� ���������
#define chk_strdup(in_str)      (in_str==NULL?NULL:strdup(in_str))
// ������� ����������� GUI ��������
#define chk_delete_object(x)    if( x != NULL ) DeleteObject(x); x = NULL;
// ����������� ����
#define chk_destroy_window(x)   if( x != NULL ) DestroyWindow(x); x = NULL;
// ������� �����������
#define chk_close_handle(x, y)  if( x != y ) CloseHandle(x); x = y;
// ������� ������� ������
#define CALC_BUF_SIZE(buf)      (sizeof(buf)/sizeof((buf)[0]) - 1)
// ���������� ����������� 
#define COPY_ATTR__S(to, from) wcsncpy((to),(from),CALC_BUF_SIZE(to));
// ��������� ���� ���������������, ���� ��� ����� - ������������ true, ���� ��� - false
#define COMPARE_ID(i1, i2)     (i1!=NULL && i2!=NULL && wcscmp(i1,i2) == 0)

typedef __int64 int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef wchar_t tchar_t;
typedef unsigned char byte_t;
typedef void *omobus_dll_t; // ���������� ������������� ������
typedef void *omobus_sym_t; // ��������� �� ������� � ������������ ������
typedef double qty_t;
typedef double cur_t;
typedef void *pk_file_t;

typedef enum OMOBUS_PROFILE_PATH {
  OMOBUS_PROFILE_UPD = 0,
  OMOBUS_PROFILE_SYNC = 1,
  OMOBUS_PROFILE_DOCS = 2,
  OMOBUS_PROFILE_ACTS = 3,
  OMOBUS_PROFILE_CACHE = 4
} profile_path_t;;

// �������� ��������� ������� MD5
struct omobus_MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
};
typedef struct omobus_MD5Context omobus_MD5_CTX;

// �������� ������� �������������� ��������� ����������������� �����
// ���������:
//    cookie    - ������ �������
//    par_name  - ��� ��������������� ���������
//    par_val   - �������� ��������������� ���������
// ������������ ��������:
//  ���
typedef void (*pf_conf_par_t)(void *cookie, const wchar_t *par_name, 
  const wchar_t *par_val);

// ���������� ������ omobus-plain-text
// ���������:
//    cookie ������ �������
//    ln ������� ������
//    argv ������
//    argc ���������� ��������� � argv
// ������������ ��������:
//  ���������� ��������� ������ ����: false - ���������, true - ����������
typedef bool (*pf_opt_line_t)(void *cookie, int ln, const wchar_t **argv, int argc);

// ������ � ���������������� ����������
typedef const wchar_t* (*pf_daemons_conf)(const wchar_t *par_val, const wchar_t *def_val);

// �������� �������
typedef int (*pf_daemons_pk_send)(const wchar_t *spath, const wchar_t *dpath, uint32_t *send);

// �������� �������
typedef int (*pf_daemons_pk_recv)(const wchar_t *spath, const wchar_t *dpath, uint32_t *recived);

// ������������ �������, ��������������� daemons_host
typedef struct _daemons_transport {
  pf_daemons_pk_send pk_send;
  pf_daemons_pk_recv pk_recv;
} daemons_transport_t;

// ������ ������
typedef void* (*pf_start_daemon)(pf_daemons_conf conf, daemons_transport_t *tr);

// ��������� ������
typedef void (*pf_stop_daemon)(void *h);

// ������ � ���������������� ����������
typedef const wchar_t* (*pf_terminals_get_conf)(const wchar_t *par_name, const wchar_t *def_val);

// ��������� ���������
typedef void (*pf_terminals_put_conf)(const wchar_t *par_name, const wchar_t *par_val);

// ����������� �������� StartupScreen
typedef void (*pf_register_start_item)(HWND hWnd, int nHeight, byte_t startup_screen_code);

// ������������� ���������
typedef void* (*pf_terminal_init)(HWND hParWnd, pf_terminals_get_conf get_conf, 
  pf_terminals_put_conf put_conf, pf_register_start_item rsi );

// ��������������� ���������
typedef void (*pf_terminal_deinit)(void *h);

// ���������� � ������� ���������.
typedef struct _omobus_location_t {
  /* ��������� ������ gpsmon.d: 
   *   0 - ����������, 
   *   1 - ���������� �������,
   *   2 - ���������� ��������,
   *   3 - ����������� GPS ����,
   *   4 - GPS �� ���������� ������ �� �������� ����. */
  byte_t gpsmon_status;
  /* ��������� ���������� � �������:
   *   0 - ����������,
   *   1 - ������ ��������� (���������� ������),
   *   2 - ������ ������� �� ����. */
  byte_t location_status;
  /* ������ � ���������. */
  double lat, lon, elv, speed;
  time_t cur, utc;
  byte_t sat_inview, sat_inuse;
  double pdop, hdop, vdop;
} omobus_location_t;


#ifdef __cplusplus
extern "C" {
#endif

// ��������� ������� ������
void *omobus_alloc(size_t len);

// ������������ ������� ������
void omobus_free(void *ptr);

// �������� ������. �������� mode ����� ��������� �� ������ ������ ��������
//	 "wb" ��� "rb", �� � ������� ������ �� 0 �� 9, ��������, "wb9". ����� 
//	 ������������� ���������� ����� � � ����������� �� ����� ����������
//	 ����� ������ ������ (bz2 - bzip2, gz - deflate, � ��������� �������
//	 ������ �������� ��� ����).
pk_file_t pkopen(const char *path, const char *mode);

// ������ ��������� ���������� �������� ������. pkread ���������� ����������
//	 ����������� ����, 0 � ������ ���� ��������� ����� ����� ��� -1 � ������ 
//	 ������.
int pkread(pk_file_t file, void *buf, int len);

// ������ ���������� ���������� �������� ����. pkwrite ���������� ����������
//	 ���������� �������� ����.
int pkwrite(pk_file_t file, const void *buf, int len);

// ����� ������ �� ����.
int pkflush(pk_file_t file);

// �������� �����.
int pkclose(pk_file_t file);

// UTF-16 -> ANSI
char *wcstombs_dup(const wchar_t *str);

// ANSI -> UTF-16
wchar_t *mbstowcs_dup(const char *str);

// �������� �������� ch �� ������ ������
wchar_t* wcsltrim(wchar_t *s, wchar_t ch);

// �������� �������� ch � ����� ������
wchar_t* wcsrtrim(wchar_t *s, wchar_t ch);

// �������� �������� ch � ������ � � ����� ������
wchar_t *wcstrim(wchar_t *s, wchar_t ch);

// ���������� 1 ���� ������� ������ 'true|yes|+|enabled|1', � ��������� ������ - 0.
bool wcsistrue(const wchar_t *str);

// ��������������� double � ������ � ������ ��������
wchar_t *ftowcs(double val, int prec, const wchar_t *thsep);

// ��������������� double � double � ������ ��������
double ftof(double val, int prec);

// ������� ������ � double
double wcstof(const wchar_t *val);

// ����� ��������� ��� ����� ��������
const wchar_t *wcsstri(const wchar_t *str1, const wchar_t *str2);

// ���������� ���������� ������������� ����������, �� ������� ��������
// ������ OMOBUS ��� Windows Moblie.
const wchar_t *GetOmobusId();

// ��������� ����������� ������ ���������. ������� ������������� �����������
// �������� ������ ���������. �������� ������ 0 ������� �� ������.
unsigned __int64 GetDocId();

// ��������� XML-���� ���������������� ���������� (� ������� wpx)
bool ProcessXMLConfigFile(const wchar_t *fname);

// ��������� �������� �����.
const wchar_t *GetOmobusRootPath();

// ��������� ��������� ����� �������
const wchar_t *GetOmobusProfilePath(profile_path_t pp);

// ��������� ����������������� ����� � ��������� UTF-16, ������ �����
// �������� � �������: par_name = par_value
void ParseOmobusConf(const wchar_t *conf_name, void *cookie, pf_conf_par_t par);

// �������� ������ � ������� omobus-plain-text. ������������ ���������� 
// ������� ������������ �����.
int ReadOmobusPlainText(const wchar_t *fname, void *cookie, pf_opt_line_t par, 
  int buf_size=2048);

// ������ ������ omobus-plain-text � ��������� utf-16le. 
int WriteOmobusPlainText(const wchar_t *fname, const wchar_t *templ, 
  const wchar_t *buf);

// ��������� ������������ �������
void SetNameEvent(LPCWSTR lpName);

// ���������� ��������
DWORD ExecProcess(LPCWSTR pszImageName, LPCWSTR pszCmdLine, BOOL bWait);
DWORD ExecRootProcess(LPCWSTR pszName, LPCWSTR pszCmdLine, BOOL bWait);

// ���������� ��������� �������� �� ���� ������
wchar_t *FormatSystemError(DWORD e);

// ��������� ������ DLL
omobus_dll_t dlopen(const wchar_t* path);

// ��������� ������ DLL 
void dlclose(omobus_dll_t h);

// �������� ����� ������� �� ����� 
omobus_sym_t dlsym(omobus_dll_t handle, const wchar_t* addr);

// ��������� �������� ��������� ������ ��������� ��� ������ � DLL
wchar_t *dlerror();

// ������ ANSI ������� localtime_r
struct tm* localtime_r(const time_t *timer, struct tm *tmbuf);

// ������ ANSI ������� gmtime_r
struct tm* gmtime_r(const time_t *timer, struct tm *tmbuf);
struct tm* gmtime(const time_t *clock);

// ������ ANSI ������� time
time_t omobus_time();

// ������ mktime �� ��� �����������, ��� �� ���� ���������� utc-����.
time_t omobus_utc_mktime(struct tm *tmbuf);

// ������� tm � SYSTEMTIME
SYSTEMTIME *tm2st(const struct tm *t, SYSTEMTIME *st);

// ������� SYSTEMTIME � tm
struct tm *st2tm(const SYSTEMTIME *st, struct tm *t);

// ������� ������ ���������� ����-����� � ��������� tm 
struct tm *datetime2tm(const wchar_t *buf, struct tm *t);

// ������� ������ ���������� ���� � ��������� tm
struct tm *date2tm(const wchar_t *buf, struct tm *t);

// ������� ������ ���������� ����-����� � ��������� tm 
struct tm *time2tm(const wchar_t *buf, struct tm *t);

// �������� ������������ ������ �������� � tm
bool tmvalid(const struct tm *tmbuf);

// �������� ����� � ���� �������� (_r = [R]ecursive)
const wchar_t *wmkdir_r(const wchar_t *path);

// �������� ������������� �����
bool wcsfexist(const wchar_t *fname);

// ������������� ��������� ��������� ������� MD5.
void omobus_MD5Init(struct omobus_MD5Context *context);

// ���������� ��������� ��������� ������� MD5.
void omobus_MD5Update(struct omobus_MD5Context *context, unsigned char const *buf, size_t len);

// ������� MD5 �� ��������� ������������ ���������.
void omobus_MD5Final(unsigned char digest[OMOBUS_MD5_BYTES], struct omobus_MD5Context *context);

// ��������� �������� �� ������������ �����
int SetTimeoutTrigger(const wchar_t *name, uint32_t sec);

// ��������� �������� �� ��������� �������
int SetEventTrigger(const wchar_t *name, DWORD ev);

// �������� ����� �������������� ��������
void RemoveTrigger(const wchar_t *name);

// �������� ������ �����������. �� ���� �������� ����������
// ������, ��� �������� ��������� ��������� ������ �����������.
HMODULE LoadMuiModule(HMODULE module, const wchar_t *postfix);

// ��������� daemons-host
void StopDaemonsHost();

// �������� ��������� daemons-host
bool DaemonsHostStarted();

// �������� ��������� daemons-host
bool TerminalsHostStarted();

// ����������� terminals-host
void ActivateTerminalsHost();

// ��������� terminals-host
void StopTerminalsHost();

// �������� ������, ����������� �����������
void ReloadTerminalsData();

// ��������� ������ � �������������� ���������.
void omobus_location(omobus_location_t *info);

// ��������� ������
int WriteOmobusXmlPk(profile_path_t pp, 
  const wchar_t *name, const wchar_t *user_id, 
  const wchar_t *pk_ext, const wchar_t *pk_mode, 
  const wchar_t *xml);

// ������ ������ �����������.
// ������������ ���������� ������� ������������ �����, ��� -1 � ������ ������.
int ReadOmobusManual(const wchar_t *name, void *cookie, pf_opt_line_t par);

// ������ ������ �� �������.
// ������������ ���������� ������� ������������ �����, ��� -1 � ������ ������.
int ReadOmobusJournal(const wchar_t *name, void *cookie, pf_opt_line_t par);

// ������ ������ � ������.
int WriteOmobusJournal(const wchar_t *name, const wchar_t *templ, const wchar_t *buf);

// �������� �������� ������
HFONT CreateBaseFont(LONG lfHeight, LONG lfWeight, BYTE lfItalic);

// �������� ����������� � ����������� ��� � ������������� �������
HBITMAP LoadImageFile(const wchar_t *szFile, int width, int height, 
  int *orig_width, int *orig_height);

// ������������ (���������� � ���������� �������) �����������.
bool NormalizeImageFile(const wchar_t *szFile, const wchar_t *szFileN, int width, 
  int height, const wchar_t *fmt);

#ifdef __cplusplus
}
#endif

// ������� double � ������ � ������ ��������
inline 
std::wstring ftows(double val, int prec, const wchar_t *thsep=NULL) {
  std::wstring rv; wchar_t *s = NULL;
  if( (s = ftowcs(val, prec, thsep)) != NULL ) {
    rv = s; omobus_free(s);
  }
  return rv;
}

// �������������� byte_t � ������
inline
std::wstring b2ws(byte_t b) {
  wchar_t buf[5] = L"";
  _snwprintf(buf, 4, L"%i", (int)b);
  return std::wstring(buf);
}

// �������������� int32_t � ������
inline
std::wstring itows(int32_t b) {
  wchar_t buf[32] = L"";
  _snwprintf(buf, 31, L"%i", b);
  return std::wstring(buf);
}
// �������������� uint32_t � ������
inline
std::wstring itows(uint32_t b) {
  wchar_t buf[32] = L"";
  _snwprintf(buf, 31, L"%u", b);
  return std::wstring(buf);
}

// �������������� int64_t � ������
inline
std::wstring itows(int64_t b) {
  wchar_t buf[32] = L"";
  _snwprintf(buf, 31, L"%I64i", b);
  return std::wstring(buf);
}

// �������������� uint64_t � ������
inline
std::wstring itows(uint64_t b) {
  wchar_t buf[32] = L"";
  _snwprintf(buf, 31, L"%I64u", b);
  return std::wstring(buf);
}

// ������������ ������ � ����� � ������ �������
inline
std::wstring wsftime(const struct tm *t, const wchar_t *fmt=ISO_DATETIME_FMT) {
  wchar_t buf[255] = ISO_DATETIME_EMPTY;
  if( t != NULL && fmt != NULL ) {
    wcsftime(buf, 254, fmt, t);
    buf[254] = L'\0';
  }
  return std::wstring(buf);
}

inline
std::wstring wsftime(time_t tt, const wchar_t *fmt=ISO_DATETIME_FMT, bool utc=false) {
  tm t; wchar_t buf[255] = ISO_DATETIME_EMPTY;
  if( tt > 0 && fmt != NULL ) {
    wcsftime(buf, 254, fmt, 
      utc ? gmtime_r(&tt, &t) : localtime_r(&tt, &t));
    buf[254] = L'\0';
  }
  return std::wstring(buf);
}

// ����� ������
inline
std::wstring fmtoserr(DWORD e = GetLastError()) {
  wchar_t *msg = FormatSystemError(e);
  std::wstring rc;
  if( msg == NULL ) {
    rc = L"#";
    rc += itows((uint32_t)e);
  } else {
    rc = msg;
    omobus_free(msg);
  }
  return rc;
}

// ������ ���������
inline
std::wstring &wsrepl(std::wstring &s, const wchar_t *from, const wchar_t *to) {
  if( from == NULL || from[0] == '\0' )
    return s;
  if( to == NULL )
    to = L"";
  size_t len = wcslen(from), len2 = wcslen(to), pos = 0;
  if( len == len2 && wcscmp(from, to) == 0 )
    return s;
  while( (pos = s.find(from, pos)) != std::wstring::npos ) {
    s.replace(pos, len, to);
    pos += len2;
  }
  return s; 
}

// �������� �������� � ����� ������
inline
std::wstring &wsrtrim(std::wstring &s, const wchar_t c = L' ') {
  size_t len;
  while( (len = s.size()) > 0 && s[--len] == c )
    s.erase(len, 1);
  return s;
}

// �������� �������� � ������ ������
inline
std::wstring &wsltrim(std::wstring &s, const wchar_t c = L' ') {
  size_t len = s.size(), pos = 0;
  while( pos < len && s[pos] == c )
    pos++;
  return (s = s.substr(pos, len - pos));
}

// �������� �������� � ������ ����� ������
inline
std::wstring &_wstrim(std::wstring &s, const wchar_t c = L' ') {
  return wsltrim(wsrtrim(s, c), c);
}

// �������� ������������ XML
inline
std::wstring fix_xml(const wchar_t *text) {
  std::wstring xml;
  if( text == NULL )
    return xml;
  xml = text;
  wsrepl(xml, L"&",  L"&amp;");
  wsrepl(xml, L"\"", L"&quot;");
  wsrepl(xml, L"<",  L"&lt;");
  wsrepl(xml, L">",  L"&gt;");
  wsrepl(xml, L"\n", L" ");
  wsrepl(xml, L"\t", L" ");
  wsrepl(xml, L"\r", L" ");
  return xml;
}

// ��������������� UTF16 � ANSI
inline
std::string w2a(const wchar_t *s) {
  std::string rv;
  if( s == NULL || s[0] == L'\0' )
    return rv;
  char *a = wcstombs_dup(s);
  if( a == NULL )
    return rv;
  rv = a;
  omobus_free(a);
  return rv;
}

// ��������������� ANSI � UTF16
inline
std::wstring a2w(const char *s) {
  std::wstring rv;
  if( s == NULL || s[0] == '\0' )
    return rv;
  wchar_t *a = mbstowcs_dup(s);
  if( a == NULL )
    return rv;
  rv = a;
  omobus_free(a);
  return rv;
}

#endif //__libomobus_h__
