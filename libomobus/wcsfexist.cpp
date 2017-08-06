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
// Проверка существования файла
bool wcsfexist(const wchar_t *fname) 
{
  WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(fname, &FindFileData);
	if( hFind == INVALID_HANDLE_VALUE )
		return false;
	FindClose(hFind);
  return true;
}

// -----------------------------------------------------------------------------
