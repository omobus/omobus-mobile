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

// -----------------------------------------------------------------------------
// Удаление символов ch в начале и в конце строки
wchar_t *wcstrim(wchar_t *s, wchar_t ch)
{
  return wcsrtrim(wcsltrim(s, ch), ch);
}

// -----------------------------------------------------------------------------
// Удаление символов ch с конца строки
wchar_t* wcsrtrim(wchar_t *s, wchar_t ch)
{
  if( s == NULL || s[0] == L'\0' )
    return s;
  
	wchar_t *t, *tt;
	for( tt = t = s; *t != L'\0'; ++t ) {
		if( *t != ch )
			tt = t+1;
  }
	*tt = L'\0';

	return s;
}

// -----------------------------------------------------------------------------
// Удаление символов ch из начала строки
wchar_t* wcsltrim(wchar_t *s, wchar_t ch)
{
  if( s == NULL || s[0] == L'\0' )
    return s;
  
	wchar_t *t;
	for( t = s; *t == ch; ++t )
		continue;
	memmove(s, t, (wcslen(t)+1)*sizeof(wchar_t));	/* +1 so that '\0' is moved too */
	return s;
}

// -----------------------------------------------------------------------------
