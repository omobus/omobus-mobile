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

// Загрузка строки
static
wchar_t *_pkgets(pk_file_t f, wchar_t *buf, int len) {
  wchar_t *b = buf;
  if( buf == NULL || len <= 0) 
    return buf;
  while( --len > 0 && pkread(f, buf, 2) == 2 && *buf++ != L'\n' );
  *buf = L'\0';
  return b == buf && len > 0 ? NULL : b;
}

// Загрузка данных в формате omobus-plain-text в кодировке utf-16le. 
// Возвращается количество успешно обработанных строк, или -1 в случае ошибки.
int ReadOmobusPlainText(const wchar_t *fname, void *cookie, pf_opt_line_t par, 
  int buf_size)
{
  if( fname == NULL || par == NULL )
    return -1;

  pk_file_t f = pkopen(w2a(fname).c_str(), "rb");
  if( f == NULL )
    return -1;

  int count = 0, pos = 0;
  wchar_t *buf = (wchar_t *)omobus_alloc(buf_size*sizeof(wchar_t)), 
    *all_data_in_one[100];
  if( buf == NULL ) {
    pkclose(f);
    return -1;
  }

  memset(buf, 0, buf_size*sizeof(wchar_t));
  while( _pkgets(f, buf, buf_size - 1) != NULL ) {
    if( buf[0] != L'#' ) {
      memset(all_data_in_one, 0, sizeof(wchar_t*)*100);
      pos = 0;
      wchar_t *data = buf;
      for( int i = 0; buf[i] != 0; i++ ) {
        if( buf[i] == L';' ) {
          buf[i] = 0;
          all_data_in_one[pos] = data;
          pos++;
          data = &buf[i+1];
        }
      }
      if( par(cookie, count, (const wchar_t**)all_data_in_one, pos) != true )
        break;
      count++;
    }
    memset(buf, 0, buf_size*sizeof(wchar_t));
  }

  chk_omobus_free(buf);
  pkclose(f);

  return count;
}
