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

#define WINVER 0x0420
#define _WTL_NO_CSTRING
#define _WTL_CE_NO_ZOOMSCROLL
#define _WTL_CE_NO_FULLSCREEN
#define _ATL_NO_HOSTING
#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA

#define OMOBUS_TERMINALS_LIST       L"terminals"
#define OMOBUS_FONT_SMOOTHING       L"font_smoothing"
#define OMOBUS_FONT_SMOOTHING_DEF   L"cleartype"
#define OMOBUS_NETWORK_MANAGER      L"network_manager"
#define OMOBUS_NETWORK_MANAGER_DEF  L"\\Windows\\remnet.exe"

#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <tpcshell.h>
#include <aygshell.h>
#include <gesture.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlwince.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atllabel.h>
#include <atlomobus.h>

#include <string>
#include <map>
#include <vector>
#include <ensure_cleanup.h>
#include <critical_section.h>
#include <omobus-mobile.h>
#include "resource.h"
#include "../../version"

typedef struct _tag_terminal_conf {
  omobus_dll_t hLib;
  pf_terminal_init init;
  pf_terminal_deinit deinit;
  void *hTerm;
} terminal_conf_t;

typedef struct _tag_startup_element {
  HWND hWnd;
  int nHeight;
} startup_element_t;

typedef std::vector<std::wstring> slist_t;
typedef std::map<std::wstring, std::wstring> conf_t;
typedef std::vector<terminal_conf_t> terminals_t;
typedef std::vector<startup_element_t> elems_t;

typedef struct _tag_hsrv {
  conf_t conf, p_conf;
  elems_t *elcur, el[8];
  HWND hWnd;
  const wchar_t *terms, *user_id, *pk_ext, *pk_mode, *caption,
    *smoothing, *netmgr, *user_name, *firm_name;
  critical_section conf_cs;
} hsrv_t;


static CAppModule _Module;
static hsrv_t _srv;

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
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
void hide_startup_screen(elems_t *el) {
  for( size_t i = 0; i < el->size(); i++ ) {
    startup_element_t *e = &(*el)[i];
    ShowWindow(e->hWnd, SW_HIDE);
    SendMessage(e->hWnd, OMOBUS_SSM_PLUGIN_HIDE, (WPARAM)0, (LPARAM)0);
  }
}

static
void show_startup_screen(elems_t *el, bool without_notify=false) {
  int top = 0, width = GetSystemMetrics(SM_CXSCREEN);
  for( size_t i = 0; i < el->size(); i++ ) {
    startup_element_t *e = &(*el)[i];
    MoveWindow(e->hWnd, 0, top, width, e->nHeight, TRUE); 
    ShowWindow(e->hWnd, SW_NORMAL);
    SendMessage(e->hWnd, OMOBUS_SSM_PLUGIN_SHOW, (WPARAM)0, (LPARAM)0);
    top += (e->nHeight + DRA::SCALEY(1));
  }
}

// Получение конфигурационного параметра. Вначале выполняется
// поиск по карте параметров, загруженных из conf-файла, а затем
// по карте параметро подключаемых модулей. Это дает гарантию,
// что подключаемые модули не перезапишут параметры заданные 
// conf-файлами.
static
const wchar_t *_terminals_get_conf(const wchar_t *par_name, const wchar_t *def_val) 
{
  s_guard _al(_srv.conf_cs);
  if( _srv.conf.find(par_name) == _srv.conf.end() ) {
    if( _srv.p_conf.find(par_name) == _srv.p_conf.end() ) {
      return def_val;
    }
    return _srv.p_conf[par_name].c_str();
  }
  return _srv.conf[par_name].c_str();
}

static
void _terminals_put_conf(const wchar_t *par_name, const wchar_t *par_val) 
{
  s_guard _al(_srv.conf_cs);
  if( par_val ) {
    if( wcscmp(par_name, __INT_CURRENT_SCREEN) == 0 ) {
      byte_t new_screen = (byte_t) _wtoi(par_val);
      elems_t *old_el = _srv.elcur;
      if( new_screen == OMOBUS_SCREEN_ROOT )
        _srv.elcur = &_srv.el[0];
      else if( new_screen == OMOBUS_SCREEN_WORK )
        _srv.elcur = &_srv.el[1];
      else if( new_screen == OMOBUS_SCREEN_ROUTE )
        _srv.elcur = &_srv.el[2];
      else if( new_screen == OMOBUS_SCREEN_ACTIVITY)
        _srv.elcur = &_srv.el[4];
      else if( new_screen == OMOBUS_SCREEN_ACTIVITY_TAB_1 )
        _srv.elcur = &_srv.el[5];
      else if( new_screen == OMOBUS_SCREEN_MERCH )
        _srv.elcur = &_srv.el[6];
      else if( new_screen == OMOBUS_SCREEN_MERCH_TASKS )
        _srv.elcur = &_srv.el[7];
      if( _srv.elcur != old_el ) {
        hide_startup_screen(old_el);
        InvalidateRect(_srv.hWnd, NULL, TRUE);
        show_startup_screen(_srv.elcur);
      }
    }
    _srv.p_conf[par_name] = par_val;
  } else {
    conf_t::iterator it = _srv.p_conf.find(par_name);
    if( it != _srv.p_conf.end() )
      _srv.p_conf.erase(it);
  }
}

static
void _terminals_register_start_item(HWND hWnd, int nHeight, byte_t type) 
{
  startup_element_t se = {hWnd, DRA::SCALEY(nHeight)};
  if( type & OMOBUS_SCREEN_ROOT )
    _srv.el[0].push_back(se);
  if( type & OMOBUS_SCREEN_WORK )
    _srv.el[1].push_back(se);
  if( type & OMOBUS_SCREEN_ROUTE )
    _srv.el[2].push_back(se);
  if( type & OMOBUS_SCREEN_ACTIVITY )
    _srv.el[4].push_back(se);
  if( type & OMOBUS_SCREEN_ACTIVITY_TAB_1 )
    _srv.el[5].push_back(se);
  if( type & OMOBUS_SCREEN_MERCH )
    _srv.el[6].push_back(se);
  if( type & OMOBUS_SCREEN_MERCH_TASKS )
    _srv.el[7].push_back(se);
}

static
const std::wstring terminal_lib_name(const wchar_t *name) {
  std::wstring rc;
  rc  = GetOmobusRootPath(); 
  rc += L"term_";
  rc += name; 
  rc += L".dll";
  return rc;
}

static
int init(slist_t &names, terminals_t &terminals) {
  for( size_t i = 0; i < names.size(); i++ ) {
    terminal_conf_t dc;
    dc.hLib = dlopen(terminal_lib_name(names[i].c_str()).c_str());
    if( dc.hLib == NULL )
      continue;
    dc.init = (pf_terminal_init) dlsym(dc.hLib, L"terminal_init");
    dc.deinit = (pf_terminal_deinit) dlsym(dc.hLib, L"terminal_deinit");
    if( dc.init == NULL || dc.deinit == NULL ) {
      chk_dlclose(dc.hLib);
      continue;
    }
    dc.hTerm = dc.init(_srv.hWnd, _terminals_get_conf, _terminals_put_conf, 
      _terminals_register_start_item);
    if( dc.hTerm == NULL ) {
      chk_dlclose(dc.hLib);
      continue;
    }
    terminals.push_back(dc);
  }
  return terminals.empty() ? OMOBUS_ERR : OMOBUS_OK;
}

static
void deinit(terminals_t &terminals) {
  for( size_t i = 0; i < terminals.size(); i++ ) {
    terminal_conf_t *dc = &terminals[i];
    dc->deinit(dc->hTerm);
    chk_dlclose(dc->hLib);
  }
  terminals.clear();
}

// Поток отвечающий за обработку уведомлений полученных от внешних
// процессов
typedef struct wait_proc_t {
  HWND hWnd;
  HANDLE hStop;
} wait_proc_t;

static 
DWORD CALLBACK WaitProc(LPVOID pParam) {
  wait_proc_t *h = (wait_proc_t *) pParam;
  if( h == NULL || h->hWnd == NULL || h->hStop == NULL ) {
    DEBUGMSG(TRUE, (L"WaitProc: Incorrect input parameters\r\n"));
    return 1;
  }
  CEnsureCloseHandle hActive = CreateEvent(NULL, FALSE, FALSE, OMOBUS_TERM_ACTIVATE),
    hReload = CreateEvent(NULL, FALSE, FALSE, OMOBUS_TERM_RELOAD);
  bool exit = false;
  ResetEvent(h->hStop);
  while( !exit ) {
    HANDLE hEv[] = { h->hStop, hActive, hReload };
    switch( WaitForMultipleObjects(sizeof(hEv)/sizeof(HANDLE), hEv, FALSE, INFINITE) ) {
    case WAIT_OBJECT_0:
      DEBUGMSG(TRUE, (L"WaitProc: Catching the stop signal\r\n"));
      PostMessage(h->hWnd, WM_QUIT, 0, 0);
      exit = true;
      break;
    case WAIT_OBJECT_0 + 1:
      DEBUGMSG(TRUE, (L"WaitProc: Catching the activate signal\r\n"));
      SetForegroundWindow(reinterpret_cast<HWND>(reinterpret_cast<ULONG>(h->hWnd) | 0x1));
      break;
    case WAIT_OBJECT_0 + 2:
      DEBUGMSG(TRUE, (L"WaitProc: Catching the reload signal\r\n"));
      PostMessage(h->hWnd, OMOBUS_SSM_PLUGIN_RELOAD, 0, 0);
      break;
    default:
      exit = true;
    };
  };
  delete h;
  return 0;
}

static
void start_wait_thr(HWND hWnd, HANDLE hStop) {
  wait_proc_t *h = new wait_proc_t;
  if( h != NULL ) {
    h->hStop = hStop;
    h->hWnd = hWnd;
    HANDLE hThr = CreateThread(NULL, 0, WaitProc, h, 0, NULL);
    if( hThr != NULL )
      CloseHandle(hThr);
  }
}

static
void StoreProcActivity(bool start, uint32_t pid, const wchar_t *image_name, const wchar_t *args) {
  std::wstring xml = wsfromRes(IDS_X_SYSACT_PROC, true);
  if( xml.empty() )
    return;

  omobus_location_t pos; omobus_location(&pos);
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", _srv.user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(pos.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(pos.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%",ftows(pos.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%state%", start?L"start":L"stop");
  wsrepl(xml, L"%pid%", itows(pid).c_str());
  wsrepl(xml, L"%image_name%", image_name?image_name:L"");
  wsrepl(xml, L"%args%", args?args:L"");

  WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_SYS_PROC, 
    _srv.user_id, _srv.pk_ext, _srv.pk_mode, xml.c_str());
}

static
void ConfigureSystem() {
  if( wcsicmp(_srv.smoothing, L"off") == 0 ) {
    SystemParametersInfo(SPI_SETFONTSMOOTHING, FALSE, NULL, FALSE);
  } else if( wcsicmp(_srv.smoothing, L"cleartype") == 0 ) {
    SystemParametersInfo(SPI_SETFONTSMOOTHING, TRUE, NULL, FALSE);
  }
}

#include "support.h"
#include "about.h"
#include "mainframe.h"

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, 
  LPTSTR lpstrCmdLine, int nCmdShow)
{
  // Предотвращение повторного запуска
  CEnsureCloseHandle hOnline = CreateEvent(NULL, TRUE, FALSE, OMOBUS_TERM_ONLINE);
  if( hOnline == NULL || GetLastError() == ERROR_ALREADY_EXISTS ) {
    ActivateTerminalsHost();
    return 1;
  }

  // Since this is application is launched through the registry HKLM\Init 
  // we need to call SignalStarted passing in the command line parameter.
  // See 'Create a Windows CE Image That Boots to Kiosk Mode':
  //  http://msdn.microsoft.com/en-us/library/aa446914.aspx
  if( wcslen(lpstrCmdLine) > 0 )
    SignalStarted(_wtol(lpstrCmdLine));

  _srv.elcur = &_srv.el[0];
  terminals_t terminals;
  slist_t terminal_names;
  wchar_t locale[255], proc_name[2048];
  const wchar_t *mui = NULL, *terms = NULL;
  CWaitCursor _wcstartup;
  
  memset(locale, 0, sizeof(locale));
  memset(proc_name, 0, sizeof(proc_name));
  GetModuleFileName(hInstance, proc_name, CALC_BUF_SIZE(proc_name));

  // Загрузка конфигурационных параметров.
  ParseOmobusConf(NULL, &_srv.conf, _wcsfconf_par);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  // Загрузка локализационных данных
  mui = _terminals_get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF);
  _snwprintf(locale, CALC_BUF_SIZE(locale), L"locale.%s", mui);
  ParseOmobusConf(locale, &_srv.conf, _wcsfconf_par);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  // Определяем список загружаемых терминалов, если он не задан,
  // то дальнейшая работа не имеет смысла.
  if( _srv.conf.find(OMOBUS_TERMINALS_LIST) == _srv.conf.end() )
    return -1;
  // Загрузка используемых параметров
  _srv.terms = _srv.conf[OMOBUS_TERMINALS_LIST].c_str();
  _srv.user_id = _terminals_get_conf(OMOBUS_USER_ID, NULL);
  _srv.user_name = _terminals_get_conf(OMOBUS_FIRM_NAME, L"");
  _srv.firm_name = _terminals_get_conf(OMOBUS_USER_NAME, L"");
  _srv.pk_ext = _terminals_get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  _srv.pk_mode = _terminals_get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  _srv.caption = _terminals_get_conf(OMOBUS_CAPTION, OMOBUS_CAPTION_DEF);
  _srv.netmgr = _terminals_get_conf(OMOBUS_NETWORK_MANAGER, OMOBUS_NETWORK_MANAGER_DEF);
  _srv.smoothing = _terminals_get_conf(OMOBUS_FONT_SMOOTHING, OMOBUS_FONT_SMOOTHING_DEF);
  // Задание предопределенных параметров системы
  ConfigureSystem();
  // Инициализация COM
	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
  // Инициализация библиотеки базовых элементов управления
	AtlInitCommonControls(ICC_DATE_CLASSES);
	SHInitExtraControls();
  // Инициализация модуля WTL
  if( SUCCEEDED(_Module.Init(NULL, hInstance)) ) {
    //Загружаем ресурсы с требуемым языком
	  _Module.m_hInstResource = LoadMuiModule(hInstance, mui);
    // Инициализация обработчика оконных сообщений
    CMessageLoop theLoop;
		_Module.AddMessageLoop(&theLoop);
    // Создание главного окна
    CMainFrame wndMain(&_srv);
		if( wndMain.CreateEx(NULL, NULL, 0, 0, lpstrCmdLine) == NULL ) {
			ATLTRACE2(atlTraceUI, 0, _T("Main window creation failed!\n"));
    } else {
      _srv.hWnd = wndMain;
      // Разбор строки со списком загружаемых терминалов
      _parce_slist(_srv.terms, terminal_names);
      // Инициализация терминалов
      if( init(terminal_names, terminals) == OMOBUS_OK ) {
        // Отображение элементов Startup Screen
        show_startup_screen(_srv.elcur);
        // Показ главного окна
        wndMain.ShowWindow(nCmdShow);
        // Запуск процедуры ожидания уведомлений из внешних источников
        start_wait_thr(wndMain, hOnline);
        // Уничтожение курсора ожидания запуска
        _wcstartup.Restore();
        // Сбрасываем активность запуска процесса
        StoreProcActivity(true, GetCurrentProcessId(), proc_name, lpstrCmdLine);
        // Запуск цикла обработки сообщений
        theLoop.Run();
        // Сбрасываем активность остановки процесса
        StoreProcActivity(false, GetCurrentProcessId(), proc_name, lpstrCmdLine);
      }
      // Деинициализация терминалов
      deinit(terminals);
      // Удаление цикла обработки сообщений
      _Module.RemoveMessageLoop();
      // Уничтожение главного окна
      wndMain.DestroyWindow();
    }
    // Деинициализация модуля WTL
    _Module.Term();
  }
  // Деинициализация COM
	::CoUninitialize();

  return 0;
}

