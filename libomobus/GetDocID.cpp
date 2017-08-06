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

#define _DOC_ID_NAME    L"doc_id"

//--------------------------------------------------------------------------
// Получение уникального номера документа. Счетчик автоматически увеличивает
// значение номера документа. Значение равное 0 говорит об ошибке.
unsigned __int64 GetDocId()
{
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // ! Возможно потребуется защитить весь код от
  // ! многопроцессного доступа с момощью мьютекса
  // ! CreateMutex(..,..,L"mutex-name")
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  HKEY hKey = NULL;
  DWORD dwDisposition = 0;
  __int64 doc_id = 0;
  DWORD size = sizeof(doc_id);

  if( RegCreateKeyExW(HKEY_LOCAL_MACHINE, OMOBUS_CODE, NULL, L"", 
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, 
        &dwDisposition) != ERROR_SUCCESS )
    return 0;

  if( RegQueryValueExW(hKey, _DOC_ID_NAME, NULL, NULL, (LPBYTE)&doc_id, 
        &size) == ERROR_SUCCESS && size == sizeof(doc_id) && doc_id > 0 ) {
    doc_id++;
  } else {
    doc_id = 1;
  }
  
  RegSetValueEx(hKey, _DOC_ID_NAME, NULL, REG_BINARY, (LPBYTE)&doc_id, 
    sizeof(doc_id));

  RegCloseKey(hKey);

  return doc_id;
}

//--------------------------------------------------------------------------
