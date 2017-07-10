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
#include <map>
#include <string>

extern std::wstring _rpath;
extern std::map<profile_path_t, std::wstring> _profile_paths;

const wchar_t *GetOmobusProfilePath(profile_path_t pp)
{
  std::map<profile_path_t, std::wstring>::iterator it =
    _profile_paths.find(pp);
  return wmkdir_r(it == _profile_paths.end() ? _rpath.c_str() : it->second.c_str());
}
