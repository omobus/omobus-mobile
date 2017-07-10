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

#ifndef _UNICODE
# pragma message ("This module should build under Unicode layout!")
#endif //_UNICODE

#include "daemons-host.h"
#include "resource.h"

extern uint32_t _ftp_skip_pasv_ip, _ftp_epsv, _ftp_response_timeout, 
  _timeout, _connect_timeout;

static
slist_t &_flist(const wchar_t *path, slist_t &fl) {
  fl.clear();
  std::wstring ftempl = path; ftempl += L"*";
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = FindFirstFileW(ftempl.c_str(), &ffd); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
            ffd.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF ||
            wcslen(ffd.cFileName) <= 6 ||
            ffd.cFileName[0] == L'.'
            ) )
          fl.push_back(ffd.cFileName);
        memset(&ffd, 0, sizeof(ffd));
    } while( FindNextFileW(hFind, &ffd) );
    FindClose(hFind);
  }
  return fl;
}

static
bool _transport_journal(bool success, bool download, const wchar_t * name, 
  int32_t size, const wchar_t *msg) 
{
  std::wstring txt = wsfromRes(IDS_J_TRANSPORT, true), templ = txt;
  if( txt.empty() )
    return false;

  wsrepl(txt, L"%date%", wsftime(omobus_time()).c_str());
  wsrepl(txt, L"%ecode%", success?L"+":L"E");
  wsrepl(txt, L"%state%", download?L"D":L"U");
  wsrepl(txt, L"%name%", name);
  wsrepl(txt, L"%size%", itows(size).c_str());
  wsrepl(txt, L"%msg%", msg);
  
  return WriteOmobusJournal(OMOBUS_TRANSPORT_J, templ.c_str(), txt.c_str()) 
    == OMOBUS_OK;
}

// Загрузка файла на сервер
static
int _ftpmove(CURL *curl, const wchar_t *spath, const wchar_t *dpath, const wchar_t *name) 
{
  char errbuf[CURL_ERROR_SIZE+1] = "";
  int rc = OMOBUS_OK;
  double size = 0.0;
  std::string url = w2a(dpath);
  std::wstring fname = spath;
  fname += name;
  url += w2a(name);

  FILE *f = _wfopen(fname.c_str(), L"rb");
  if( f == NULL ) {
    RETAILMSG(TRUE, (L"Unable to open '%s'\n", fname.c_str()));
    return OMOBUS_ERR;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, fread);
  curl_easy_setopt(curl, CURLOPT_READDATA, f);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

  if( curl_easy_perform(curl) != CURLE_OK ) {
    RETAILMSG(TRUE, (L"Unable to upload file ('%s') to the server: %s\n",
      name, a2w(errbuf).c_str()));
    rc = OMOBUS_ERR;
  }

  chk_fclose(f);

  if( rc != OMOBUS_ERR) {
    DeleteFile(fname.c_str());
  }

  _transport_journal(rc == OMOBUS_OK, false, name, 
    (int32_t)(curl_easy_getinfo(curl, CURLINFO_SIZE_UPLOAD, &size) != CURLE_OK ? -1 : size),
    rc == OMOBUS_OK ? L"" : a2w(errbuf).c_str());

  return rc;
}

// Отправка списка файлов на сервер.
static
int _fsend(CURL *curl, const wchar_t *spath, const wchar_t *dpath, slist_t &fl, uint32_t *sent) 
{
  int rc = OMOBUS_OK;

  curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_STDERR, NULL);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_USERNAME, w2a(GetUserId()).c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, w2a(GetUserKey()).c_str());
  if( _timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, _timeout); // продолжительность загрузки данных с сервера
  if( _connect_timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connect_timeout); // продолжительность создания подключения
  curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, _ftp_epsv ? 1 : 0);
  curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, _ftp_skip_pasv_ip ? 1 : 0);
  if( _ftp_response_timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, _ftp_response_timeout);
  /*
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_CAINFO, _cafile_a.c_str());
  curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS);
  curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
  */

  for( size_t i = 0; i < fl.size() && rc != OMOBUS_ERR; i++ ) {
    rc = _ftpmove(curl, spath, dpath, fl[i].c_str());
    if( rc != OMOBUS_ERR ) {
      if( sent != NULL )
        (*sent)++;
    }
  }

  return rc;
}

// Сохранение списка файлов на ftp-сервере
static 
size_t _ftp_buf(void *ptr, size_t size, size_t nmemb, void *stream) 
{
	std::string *buf = (std::string *)stream;
	char *line = (char *)ptr;
	if( line != NULL && buf != NULL && nmemb > 0 ) {
		for(int i = 0; line[i] != 0; i++ ) {
			if( !(line[i] == '\r' || line[i] == '\t') )
				(*buf) += line[i];
		}
	}	
  return nmemb;
}

// Разбор буфера со списком файлов. true возвращается только в случае
// наличия файла определенного макросом FF_UNLOCKED.
static 
bool _ftp_parser(const char *ptr, slist_t *lst) 
{
  if( lst == NULL || ptr == NULL )
    return false;
  size_t nmemb = strlen(ptr);
  if( nmemb < 5 )
  	return false;
  char *str = _strdup(ptr);
  if( str == NULL )
    return false;
  bool rc = false;
  char *xtmp = str, *xtmp2 = str;
  for( size_t i = 0; i < nmemb; i++ ) {
    if( xtmp2[i] == '\n' ) {
      xtmp2[i] = 0; 
      /* Проверяем наличие файла-флага разблокировки. Его отсутствие 
       * говорит о том, что в настоящий момент ресурс занят. 
       */
      if( strcmp(xtmp, "__unlocked__") == 0 )
        rc = true;
      else {
        char *ext = strrchr(xtmp, '.');
        if( ext != NULL && strlen(xtmp) > 6 &&
      		  (_stricmp(ext, ".gz") == 0 || _stricmp(ext, ".bz2") == 0 ) ) {
	        lst->push_back(a2w(xtmp));
	      }
      }
      xtmp = &xtmp2[++i];
    }
  }
  free(str);
  return rc;
}

// Загрузка файла с FTP.
static
int _ftpcopy(CURL *curl, const char *ftpf, const char *locf, const wchar_t *name) 
{
  int rc = OMOBUS_OK;
  CURLcode res = CURLE_OK;
  double size = 0.0;
	char errbuf[CURL_ERROR_SIZE+1];
	memset(errbuf, 0, sizeof(errbuf));

  FILE *f = fopen(locf, "wb");
  if( f == NULL ) {
    RETAILMSG(TRUE, (L"Unable to create file '%s'\n", locf));
    return OMOBUS_ERR;
  }

  curl_easy_setopt(curl, CURLOPT_URL, ftpf);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_FTPLISTONLY, 0);

  res = curl_easy_perform(curl);
	if( res != CURLE_OK ) {
    RETAILMSG(TRUE, (L"Unable to download file (%s) from the FTP server: %s\n", 
      name, a2w(errbuf).c_str()));
  	rc = OMOBUS_ERR;
  }

  chk_fclose(f);
  _transport_journal(rc == OMOBUS_OK, true, name, 
    (int32_t)(curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &size) != CURLE_OK ? -1 : size),
    rc == OMOBUS_OK ? L"" : a2w(errbuf).c_str());
  
  return rc;
}

// Получение списка файлов на ftp
static
int _ftplist(CURL *curl, const char *ftp, slist_t *flist)
{
	int rc = OMOBUS_OK;
	std::string buflist;
	char errbuf[CURL_ERROR_SIZE+1];

  memset(errbuf, 0, sizeof(errbuf));

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
  curl_easy_setopt(curl, CURLOPT_URL, ftp);
  curl_easy_setopt(curl, CURLOPT_FTPLISTONLY, 1);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _ftp_buf);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buflist);

  if( curl_easy_perform(curl) != CURLE_OK ) {
    RETAILMSG(TRUE, (L"Unable to load file list from the ftp: %s.\n", 
      a2w(errbuf).c_str()));
    rc = OMOBUS_ERR;
  } else {
    rc = _ftp_parser(buflist.c_str(), flist) ? OMOBUS_OK : OMOBUS_ERR;
  }

	return rc;
}

// Загрузка локального списка файлов
static
void _loclist(const wchar_t *path, slist_t *fl) {
  std::wstring filter = path; filter += L"\\*";
  WIN32_FIND_DATAW FindFileData;
  HANDLE hFind = FindFirstFileW(filter.c_str(), &FindFileData); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        continue;
      if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
          FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF )
        continue;
      fl->push_back(FindFileData.cFileName);
    } while( FindNextFileW(hFind, &FindFileData) );
    FindClose(hFind);
  }
}

// Получение файлов с сервера. В случае ошибки информация сбрасывается в 
// журнальный файл.
static
int _frecv(CURL *curl, const wchar_t *spath, const wchar_t *dpath, uint32_t *recived) {
  char errbuf[CURL_ERROR_SIZE+1] = "";
  struct curl_slist *headerlist = NULL;
  std::string url = w2a(dpath);
  int rc = OMOBUS_OK;
  slist_t ftpl, locl;

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
  curl_easy_setopt(curl, CURLOPT_STDERR, NULL);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_USERNAME, w2a(GetUserId()).c_str());
  curl_easy_setopt(curl, CURLOPT_PASSWORD, w2a(GetUserKey()).c_str());
  if( _timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, _timeout); // продолжительность загрузки данных с сервера
  if( _connect_timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connect_timeout); // продолжительность создания подключения
  curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, CURLFTP_CREATE_DIR_RETRY);
  curl_easy_setopt(curl, CURLOPT_FTP_USE_EPSV, _ftp_epsv ? 1 : 0);
  curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, _ftp_skip_pasv_ip ? 1 : 0);
  if( _ftp_response_timeout > 0 )
    curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, _ftp_response_timeout);
  /*
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl, CURLOPT_CAINFO, _cafile_a.c_str());
  curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
  curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS);
  curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
  */

  _loclist(dpath, &locl);
  rc = _ftplist(curl, w2a(spath).c_str(), &ftpl);
  if( rc != OMOBUS_OK )
    return rc;

  // Загружаем новые файлы с ftp. Копирование производится во временный
  // файл с префиксом t#. Затем, этот временный файл перемещается в предписанный
  // файл, предварительно, предписанный файл удаляется или переименовывается,
  // если удаление невозможно.
  size_t size = ftpl.size(), i = 0;
  for( i = 0 ; i < size && rc == OMOBUS_OK; i++ ) {
    if( std::find(locl.begin(), locl.end(), ftpl[i]) == locl.end() ) {
      // Формирование имени файла на ftp
      std::wstring ftpf = spath; ftpf += ftpl[i];
      // Формирование временного локального имени
      std::wstring fname = dpath; fname += L"t#"; fname += ftpl[i];
      // Формирование конечного имени
      std::wstring fname2 = dpath; fname2 += ftpl[i];
      // Формирование резервного имени
      std::wstring fname3 = dpath; fname3 += L"t0#"; fname3 += ftpl[i];
      // Получение файла с ftp
      rc = _ftpcopy(curl, w2a(ftpf.c_str()).c_str(), w2a(fname.c_str()).c_str(),
        ftpl[i].c_str());
      if( rc == OMOBUS_OK ) {
        // Удаление или перемешение текущего файла
        if( !(DeleteFile(fname2.c_str()) || ERROR_FILE_NOT_FOUND == GetLastError()) ) {
          // Удаление временной копии
          DeleteFile(fname3.c_str()); MoveFile(fname2.c_str(), fname3.c_str());
        }
        MoveFile(fname.c_str(), fname2.c_str());
        // Увеличиваем счетчик успешно принятых пакетов
        if( recived != NULL )
          (*recived)++;
      } else {
        // Удаляем файл, если произошла ошибка
        DeleteFile(fname.c_str());
      }
    }
  }

  // Удаляем устаревшие файлы из локальной папки
  size = locl.size();
  for( i = 0 ; i < size && rc == OMOBUS_OK; i++ ) {
    if( std::find(ftpl.begin(), ftpl.end(), locl[i]) == ftpl.end() ) {
      // Формирование полного имени удаляемго файла
      std::wstring fname2 = dpath; fname2 += locl[i];
      // Формирование резервного имени
      std::wstring fname3 = dpath; fname3 += L"t0#"; fname3 += locl[i];
      // Удаление файла
      if( !DeleteFile(fname2.c_str()) ) {
        // Перемещение файла, если удаление не сработало
        DeleteFile(fname3.c_str()); MoveFile(fname2.c_str(), fname3.c_str());
      }
    }
  }

  return rc;
}

bool IsSendPk(const wchar_t *spath)
{
  slist_t fl;
  return _flist(spath, fl).empty() ? false : true;
}

int FtpSendPk(const wchar_t *spath, const wchar_t *dpath, uint32_t *sent) 
{
  slist_t fl;
  int rc = OMOBUS_ERR;
  CURL *curl = curl_easy_init();
  if( curl == NULL ) {
    RETAILMSG(TRUE, (L">>> PANIC <<< Unable to prepare curl handle!\n"));
  } else {
    rc = _fsend(curl, spath, dpath, _flist(spath, fl), sent);
  }
  curl_easy_cleanup(curl);
  return rc;
}

int FtpRecvPk(const wchar_t *spath, const wchar_t *dpath, uint32_t *recived)
{
  int rc = OMOBUS_ERR;
  CURL *curl = curl_easy_init();
  if( curl == NULL ) {
    RETAILMSG(TRUE, (L">>> PANIC <<< Unable to prepare curl handle!\n"));
  } else {
    rc = _frecv(curl, spath, dpath, recived);
  }
  curl_easy_cleanup(curl);
  return rc;
}
