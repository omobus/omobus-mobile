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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include "daemons-host.h"

static critical_section _tran_cs;
static HANDLE _hConnection = NULL;

static
int _init_saddr(struct sockaddr_in &saddr, const char *host, u_short port, short family) 
{
  memset(&saddr, 0, sizeof(sockaddr_in));
  saddr.sin_family = family;
  saddr.sin_port = htons(port);
  if( (saddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE ) {
    struct hostent *hp = gethostbyname(host);
    if( hp == NULL )
      return WSAGetLastError();
    memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
  }
  return 0;
}

static
int _chk_host(const char *host, u_short port, uint32_t timeout) 
{
  if( host == NULL || host[0] == L'\0' )
    return OMOBUS_ERR;

  int rc = OMOBUS_ERR, err = 0;
  char buf[4] = "";
  SOCKET s = 0;
  sockaddr_in saddr = {0};
  timeval tval = {timeout, 0};
  u_long ul = 1;
  fd_set rset;

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if( INVALID_SOCKET == s ) {
    return OMOBUS_ERR;
  }

  if( ioctlsocket(s, FIONBIO, &ul) != SOCKET_ERROR &&
      _init_saddr(saddr, host, port, AF_INET) == 0  ) {
    connect(s, (struct sockaddr*)&saddr, sizeof(saddr));
    if( (err = WSAGetLastError()) == WSAEWOULDBLOCK ) {
      FD_ZERO(&rset); FD_SET(s, &rset);
      rc = select(0, NULL, &rset, NULL, &tval) > 0 && FD_ISSET(s, &rset) ?
        OMOBUS_OK : OMOBUS_ERR;
    } else if( err == ERROR_SUCCESS ) {
      rc = OMOBUS_OK;
    }
    if( rc == OMOBUS_OK ) {
      rc = OMOBUS_ERR;
      memset(buf, 0, sizeof(buf));
      if( (err = recv(s, buf, 3, 0)) > 0 ) {
        if( strncmp(buf, "220", 3) == 0 ) {
          rc = OMOBUS_OK;
        }
      } else if( err < 0 ) { // = error, 0 = connection closed, 
        if( (err = WSAGetLastError()) == WSAEWOULDBLOCK ) {
          FD_ZERO(&rset); FD_SET(s, &rset);
          if( select(0, &rset, NULL, NULL, &tval) > 0 && FD_ISSET(s, &rset) ) {
            if( (err = recv(s, buf, 3, 0)) > 0 && strncmp(buf, "220", 3) == 0 ) {
              rc = OMOBUS_OK;
            }
          }
        }
      }
    }
  }

  shutdown(s, SD_BOTH);
  closesocket(s);

  return rc;
}

static
void _split_uri(const wchar_t *uri, wchar_t schema[16], wchar_t host[64], int *port) {
  wchar_t *t = wcsdup(uri);
  if( t != NULL ) {
    for( int i = 0; t[i] != L'\0'; i++ ) {
      if( t[i] == L':' )
        t[i] = L' ';
    }
    swscanf(t, L"%15s //%63s %i/", schema, host, port);
    free(t);
  }
}

static
const wchar_t *_find_host(slist_t &servers, uint32_t chk_server_timeout) 
{
  for( size_t i = 0; i < servers.size(); i++ ) {
    wchar_t schema[16] = L"", host[64] = L""; int port = 0;
    _split_uri(servers[i].c_str(), schema, host, &port);
    if( _chk_host(w2a(host).c_str(), port, chk_server_timeout) == OMOBUS_OK ) {
      RETAILMSG(TRUE, (L"Valid server '%s'\n", servers[i].c_str()));
      return servers[i].c_str();
    }
    RETAILMSG(TRUE, (L"Unable to find server using following parameters '%s'\n",
      servers[i].c_str()));
  }
  return NULL;
}

static
HRESULT _NotifyConnectingError(DWORD dwStatus) 
{
  SHELLEXECUTEINFO sei = {0};
  TCHAR szExec[MAX_PATH];
  wsprintf(szExec, TEXT("-CMERROR 0x%x -report"), dwStatus);
  sei.cbSize = sizeof(sei);
  sei.hwnd = NULL;
  sei.lpFile = TEXT(":MSREMNET");
  sei.lpParameters = szExec;
  sei.nShow = SW_SHOWNORMAL;
  ShellExecuteEx( &sei );
  return E_FAIL;
}

static
int _IsConnMgrReady() 
{
  HANDLE hConnMgr = ConnMgrApiReadyEvent();
  if( hConnMgr == NULL )
    return OMOBUS_ERR;
  DWORD dwResult = WaitForSingleObject(hConnMgr, 10000);
  CloseHandle(hConnMgr);
  return dwResult == WAIT_OBJECT_0 ? OMOBUS_OK : OMOBUS_ERR;
}

static
void _CloseConn() 
{
  if( _hConnection == NULL )
    return;
  ConnMgrReleaseConnection(_hConnection, TRUE);
  _hConnection = NULL;
  RETAILMSG(TRUE, (L"Connection has been closed.\n"));
}

static
int _OpenConn(const wchar_t *net) 
{
  int rc = OMOBUS_OK;
  DWORD dwStatus = 0;
  HRESULT hr = S_OK;
  CONNMGR_CONNECTIONINFO ci;
  GUID nilGuid;
  memset(&ci, 0, sizeof(CONNMGR_CONNECTIONINFO));
  memset(&nilGuid, 0, sizeof(GUID));
  if( net == NULL || (hr = CLSIDFromString((LPOLESTR)net, &ci.guidDestNet)) != NOERROR )
    return OMOBUS_ERR;

  if( memcmp(&ci.guidDestNet, &nilGuid, sizeof(GUID)) == 0 )
    return OMOBUS_OK;

  if( _hConnection != NULL && 
      (hr = ConnMgrConnectionStatus(_hConnection, &dwStatus)) == S_OK &&
        dwStatus == CONNMGR_STATUS_CONNECTED ) {
    RETAILMSG(TRUE, (L"Reuse existing connection.\n"));
    return OMOBUS_OK;
  }
  
  _CloseConn();

  ci.cbSize = sizeof(CONNMGR_CONNECTIONINFO);
  ci.dwPriority = CONNMGR_PRIORITY_HIPRIBKGND;
  ci.bExclusive = ci.bDisabled = FALSE;
  ci.dwParams = CONNMGR_PARAM_GUIDDESTNET;
  if( !(ConnMgrEstablishConnectionSync(&ci, &_hConnection, 60000, &dwStatus) == S_OK || 
        dwStatus == CONNMGR_STATUS_CONNECTED || dwStatus == CONNMGR_STATUS_PHONEOFF) ) {
    RETAILMSG(TRUE, (L"Unable to establish connection '%s': 0x%x\n",
      net, dwStatus));
    _NotifyConnectingError(dwStatus);
    rc = OMOBUS_ERR;
  } else {
    RETAILMSG(TRUE, (L"Connection has been established.\n"));
  }
  
  return rc;
}

const wchar_t *GetValidServer(const wchar_t *net, slist_t &servers, uint32_t chk_server_timeout)
{
  s_guard _al(_tran_cs);
  if( net != NULL ) {
    if( _IsConnMgrReady() != OMOBUS_OK )
      return NULL;
    if( _OpenConn(net) != OMOBUS_OK )
      return NULL;
  }
  return _find_host(servers, chk_server_timeout);
}

void CloseConn()
{
  s_guard _al(_tran_cs);
  _CloseConn();
}
