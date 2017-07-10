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
#include <GetDeviceUniqueId.h>
#include <omobus-mobile.h>
#include <string>
#include <map>

#define OMOBUS_UNIQUE_PREFIX      L"@^!omobus-modile!^@"

std::wstring _dev_id, _rpath;
std::map<profile_path_t, std::wstring> _profile_paths;

static
void _InitOmobusId()
{
  _dev_id.clear();

  BYTE rgDeviceId[GETDEVICEUNIQUEID_V1_OUTPUT];
  DWORD cbDeviceId = sizeof(rgDeviceId);
  memset(rgDeviceId, 0, cbDeviceId);
  GetDeviceUniqueID((LPBYTE)OMOBUS_UNIQUE_PREFIX, sizeof(OMOBUS_UNIQUE_PREFIX),
    GETDEVICEUNIQUEID_V1, rgDeviceId, &cbDeviceId);

  for( DWORD i = 0; i < cbDeviceId; i++ ) {
    wchar_t b[3] = L""; _snwprintf(b, 2, L"%02X", (int)(rgDeviceId[i]));
    _dev_id += b;
  }
}

static
void _InitOmobusProfilePath(profile_path_t pp, const wchar_t *subfolder) 
{
  std::wstring path = _rpath; path += subfolder;
  _profile_paths[pp] = path;
}

static
void _InitOmobusPaths(HINSTANCE hInstance) 
{
  wchar_t rp[1024] = L"", *tmp = NULL;
  if( GetModuleFileName(hInstance, rp, CALC_BUF_SIZE(rp)) == 0 ) {
    _snwprintf(rp, CALC_BUF_SIZE(rp), L"\\OMOBUS\\");
  } else {
    if( (tmp = wcsrchr(rp, L'\\')) != NULL ) {
      tmp++; *tmp = L'\0';
    }
  }
  _rpath = rp;
  _InitOmobusProfilePath(OMOBUS_PROFILE_UPD,  L"profile\\upd\\");
  _InitOmobusProfilePath(OMOBUS_PROFILE_SYNC, L"profile\\sync\\");
  _InitOmobusProfilePath(OMOBUS_PROFILE_DOCS, L"profile\\docs\\");
  _InitOmobusProfilePath(OMOBUS_PROFILE_ACTS, L"profile\\acts\\");
  _InitOmobusProfilePath(OMOBUS_PROFILE_CACHE,L"profile\\cache\\");
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _InitOmobusId();
    _InitOmobusPaths((HINSTANCE)hModule);
    if( _dev_id.empty() || _rpath.empty() ) {
      ExitProcess(-1);
    }
    DisableThreadLibraryCalls((HINSTANCE)hModule);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

#define DEFINE_GUID_EX(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID_EX(IID_IImagingFactory, 0x327abda7,0x072b,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
DEFINE_GUID_EX(IID_IImage, 0x327abda9,0x072b,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
DEFINE_GUID_EX(IID_IBasicBitmapOps, 0x327abdaf,0x072b,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
DEFINE_GUID_EX(CLSID_ImagingFactory, 0x327abda8,0x072b,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
DEFINE_GUID_EX(ImageFormatJPEG, 0xb96b3cae,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
