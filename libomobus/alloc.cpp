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

void *omobus_alloc(size_t len)
{
  return malloc(len);
}

void omobus_free(void *ptr)
{
  if( ptr != NULL )
    free(ptr);
}

// UTF-16 -> ANSI
char *wcstombs_dup(const wchar_t *str) 
{
	if( str == NULL )
		return NULL;
	size_t len = wcslen(str);
	if( len == 0 )
		return NULL;
	char *s = (char *)omobus_alloc(len+1);
	if( s == NULL )
		return NULL;
	wcstombs(s, str, len+1);
	s[len] = '\0';
	return s;
}

// ANSI -> UTF-16
wchar_t *mbstowcs_dup(const char *str) 
{
	if( str == NULL )
		return NULL;
	size_t len = strlen(str);
	if( len == 0 )
		return NULL;
	wchar_t *s = (wchar_t *)omobus_alloc((len+1)*sizeof(wchar_t));
	if( s == NULL )
		return NULL;
	mbstowcs(s, str, len+1);
	s[len] = L'\0';
	return s;
}
