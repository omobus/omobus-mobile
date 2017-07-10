/* -*- H -*- */
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

#pragma once

#include <winsock2.h>
#include <windows.h>
#include <aygshell.h>
#include <connmgr.h>
#include <regext.h>
#include <objbase.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <ensure_cleanup.h>
#include <critical_section.h>
#include <omobus-mobile.h>
#include <WaitCursor.h>
#include <curl/curl.h>

typedef std::vector<std::wstring> slist_t;

std::wstring wsfromRes(UINT uID, bool sys);
const wchar_t *GetUserId();
const wchar_t *GetUserKey();
const wchar_t *GetValidServer(const wchar_t *net, slist_t &servers, uint32_t chk_server_timeout);
void CloseConn();
bool IsSendPk(const wchar_t *spath);
int FtpSendPk(const wchar_t *spath, const wchar_t *dpath, uint32_t *sent);
int FtpRecvPk(const wchar_t *spath, const wchar_t *dpath, uint32_t *recived);
