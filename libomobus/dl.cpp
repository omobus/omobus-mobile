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
// ��������� ������ DLL
omobus_dll_t dlopen(const wchar_t* path)
{
  return (omobus_dll_t)LoadLibrary(path);
}

// -----------------------------------------------------------------------------
// ��������� ������ DLL 
void dlclose(omobus_dll_t h)
{
  FreeLibrary((HMODULE)h);
}

// -----------------------------------------------------------------------------
// �������� ����� ������� �� ����� 
omobus_sym_t dlsym(omobus_dll_t handle, const wchar_t* addr)
{
  return (omobus_sym_t)GetProcAddress((HMODULE)handle, addr);
}

// -----------------------------------------------------------------------------
// ��������� �������� ��������� ������ ��������� ��� ������ � DLL
wchar_t *dlerror()
{
  return FormatSystemError(GetLastError());
}

// -----------------------------------------------------------------------------
