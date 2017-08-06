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
#include <omobus-mobile.h>
#include <string>

#define OMOBUS_CONF L"omobus-mobile"

// Обработка конфигурационного файла в кодировке UTF-16, данные файлы
// хранятся в формате: par_name = par_value
static
void wcsfconf_parser(FILE *f, void *cookie, pf_conf_par_t par)
{
  if( f == NULL || par == NULL )
    return;
  int count = 0;
  const int buf_size = 4*1024;
  wchar_t *buf = (wchar_t *)omobus_alloc(sizeof(wchar_t)*buf_size);
  while( buf != NULL && fgetws(buf, buf_size-1, f) != NULL ) {
    if( L'A' > buf[0] || buf[0] > L'z' )
      continue;
    wchar_t *data = NULL;
    for( int i = 0; buf[i] != L'\0'; i++ ) {
      if( buf[i] == L'=' ) {
        buf[i] = L'\0';
        i++;
        if( buf[i] == L'\0' )
          break;
        data = &buf[i];
        break;
      }
    }
    if( data == NULL )
      continue;
    // Удаляем символы \t,\r,\n с из строки с данными
    for( int i = 0; data[i] != L'\0'; i++ ) {
      if( data[i] == L'\t' || data[i] == L'\r' ||  data[i] == L'\n' )
        data[i] = L' ';
    }
    // Подготовка параметров
    wcstrim(buf, L' '); wcstrim(data, L' ');
    // Удаляем ' т.к. символ ' вставляется для сохранения пробелов
    wcstrim(data, L'\'');
    // Анализ параметров
    par(cookie, buf, data);

    memset(buf, 0, sizeof(wchar_t)*buf_size);
    count++;
  }
  chk_omobus_free(buf);
}

// Обработка конфигурационного файла в кодировке UTF-16, данные файлы
// хранятся в формате: par_name = par_value
void ParseOmobusConf(const wchar_t *conf_name, void *cookie, pf_conf_par_t par)
{
  std::wstring conf = GetOmobusRootPath();
  conf += conf_name == NULL ? OMOBUS_CONF : conf_name; 
  conf += L".conf";

  if( conf_name == NULL || wcsicmp(conf_name, OMOBUS_CONF) == 0 ) {
    // Простановка параметров используемых по-умолчанию
    par(cookie, L"caption", L"OMOBUS");
    par(cookie, L"currency_precision", L"2");
    par(cookie, L"qty_precision", L"0");
    par(cookie, L"help_viewer", L"\\Windows\\peghelp.exe");
    par(cookie, L"transport->protocol", L"ftp");
    par(cookie, L"transport->port", L"21021");
  }

  // Загрузка конфигурационных параметров
  FILE *f = _wfopen(conf.c_str(), L"rb");
  if( f != NULL )
    wcsfconf_parser(f, cookie, par);
  chk_fclose(f);
}
