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
#include <omobus-mobile.h>

const wchar_t *wmkdir_r(const wchar_t *path)
{
  if( path == L'\0' )
    return path;

  wchar_t *copy = wcsdup(path), *tmp = copy, *tmp2 = NULL;
  if( copy != NULL ) {
    int len = wcslen(tmp);
    if( tmp[len-1] == L'\\' )
      tmp[len-1] = L'\0';
    tmp2 = tmp;
    while(1) {
      tmp2 = wcschr(tmp2+1, L'\\');
      if( tmp2 == NULL )
        break;
      *tmp2 = L'\0';
      CreateDirectoryW(tmp, NULL);
      *tmp2 = L'\\';
    }
    CreateDirectoryW(copy, NULL);
    omobus_free(copy);
  }

  return path;
}
