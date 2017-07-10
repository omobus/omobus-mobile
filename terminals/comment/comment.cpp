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

#define WINVER 0x0420
#define _WTL_NO_CSTRING
#define _WTL_CE_NO_ZOOMSCROLL
#define _WTL_CE_NO_FULLSCREEN
#define _ATL_NO_HOSTING
#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA

#include <windows.h>
#include <stdlib.h>
#include <tpcshell.h>
#include <aygshell.h>
#include <gesture.h>

#include <atlbase.h>
#include <atlstr.h>
#include <atlenc.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlwince.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atlomobus.h>
#include <atllabel.h>

#include <string>
#include <vector>
#include <omobus-mobile.h>

#include "resource.h"
#include "../../version"

#define IMAGE_TYPE                  L"comment->image"
#define IMAGE_WIDTH                 L"comment->image->width"
#define IMAGE_WIDTH_DEF             L"800"
#define IMAGE_HEIGHT                L"comment->image->height"
#define IMAGE_HEIGHT_DEF            L"480"
#define CAMERA_WIDTH                L"comment->camera->width"
#define CAMERA_WIDTH_DEF            L"1600"
#define CAMERA_HEIGHT               L"comment->camera->height"
#define CAMERA_HEIGHT_DEF           L"1200"
#define CAMERA_MIN_TEXT_LENGTH      L"comment->min_text_length"
#define CAMERA_MIN_TEXT_LENGTH_DEF  L"0"

static HINSTANCE _hInstance = NULL;
static CAppModule _Module;
static TCHAR _szWindowClass[] = L"omobus-comment-terminal-element";
static int _nHeight = 20;
static std::wstring _wsTitle;
static int _nCXScreen = 0, _nCYScreen = 0;
static wchar_t _tempPath[MAX_PATH + 1] = L"";
static const wchar_t _imageFilter[] = L"*.photo";
static const wchar_t _imageExt[] = L"photo";
static int _imageNo = 0;

struct SIMPLE_CONF {
  wchar_t id[OMOBUS_MAX_ID + 1];
  wchar_t descr[OMOBUS_MAX_DESCR + 1];
};
typedef std::vector<SIMPLE_CONF> simple_t;

typedef std::vector<std::wstring> slist_t;

typedef struct _tagCOMMENT {
  const wchar_t *account_id, *activity_type_id, *w_cookie, *a_cookie,
    *brand_id, *brand_descr;
  omobus_location_t posCre;
  time_t ttCre;
  SIMPLE_CONF *type;
  wchar_t note[OMOBUS_MAX_NOTE + 1], photo[MAX_PATH + 1];
  simple_t *_types;
} COMMENT;

typedef struct _tagHTERMINAL {
  pf_terminals_get_conf get_conf;
  pf_terminals_put_conf put_conf;
  const wchar_t *user_id, *pk_ext, *pk_mode, *image_fmt;
  bool focus;
  int image_width, image_height, camera_width, camera_height,
    min_text_length;
  HWND hWnd, hParWnd;
  HFONT hFont;
  HBRUSH hbrHL, hbrN, hbrOdd;
  HPEN hpBord;
} HTERMINAL;

static
slist_t &flist(const wchar_t *path, const wchar_t *filter, slist_t &fl);

class CTypesFrame : 
  public CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF> 
{
public:
  CTypesFrame(HTERMINAL *hterm, simple_t *l, SIMPLE_CONF **cur) :
    CListFrame<IDD_TYPES, IDC_LIST, simple_t, SIMPLE_CONF>(
      l, cur, hterm->hbrHL, hterm->hbrN, hterm->hbrOdd, hterm->hpBord)
  {
  }
};

class CImageFrame : 
	public CStdDialogResizeImpl<CImageFrame>,
	public CUpdateUI<CImageFrame>,
	public CMessageFilter
{
private:
  HTERMINAL *m_hterm;
  std::wstring m_wsFile;
  CBitmap m_bm;
  int m_imageWidth, m_imageHeight;
  slist_t m_imageList;
  size_t m_listPos;
  CFont m_infoFont, m_baseFont;
  CString m_templ[4], m_tmp;

public:
  CImageFrame(HTERMINAL *hterm, const wchar_t *szFile) : m_hterm(hterm) {
    m_imageWidth = m_imageHeight = 0;
    m_infoFont = CreateBaseFont(DRA::SCALEY(17), FW_SEMIBOLD, FALSE);
    m_baseFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
    m_templ[0].LoadString(IDS_TEMPL0);
    m_templ[1].LoadString(IDS_TEMPL1);
    m_templ[2].LoadString(IDS_STR5);
    m_templ[3].LoadString(IDS_STR6);
    LoadImageList(szFile);    
  }

	enum { IDD = IDD_IMAGE };

	virtual BOOL PreTranslateMessage(MSG* pMsg) 	{
		return CWindow::IsDialogMessage(pMsg);
	}

  BEGIN_DLGRESIZE_MAP(CImageFrame)
  END_DLGRESIZE_MAP()

	BEGIN_UPDATE_UI_MAP(CImageFrame)
    UPDATE_ELEMENT(ID_MENU_OK, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CImageFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_GESTURE, OnGesture)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
		COMMAND_ID_HANDLER(ID_MENU_CAMERA, OnCamera)
		COMMAND_ID_HANDLER(ID_MENU_OK, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CImageFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{
		UIAddToolBar(AtlCreateMenuBar(m_hWnd, IDD_IMAGE, SHCMBF_HIDESIPBUTTON));
		UIAddChildWindowContainer(m_hWnd);
    if( m_imageList.empty() ) {
      DoImageCapture();
    } else {
      RedrawImage();
    }
		return bHandled = FALSE;
	}

  LRESULT OnGesture(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL &bHandled) {
    LRESULT rc = 0;
    GESTUREINFO gi;
    memset(&gi, 0, sizeof(gi));
    gi.cbSize = sizeof(GESTUREINFO);
    if( TKGetGestureInfo((HGESTUREINFO)lParam, &gi) ) {
      if( wParam == GID_PAN ) {
        if( gi.dwFlags & (GF_END|GF_INERTIA) ) {
          if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_RIGHT ) {
            NextImage();
          } else if( GID_SCROLL_DIRECTION(gi.ullArguments) == ARG_SCROLL_LEFT ) {
            PrevImage();
          }
          bHandled = TRUE;
          rc = 1;
        }
      } else if( wParam == GID_DOUBLESELECT ) {
        ExecRootProcess(L"viewer.exe", m_wsFile.c_str(), FALSE);
      }
    }
    return rc;
  }

	LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = TRUE;
    CDCHandle dc((HDC)wParam);
    RECT rc = {0};
    if( m_bm.IsNull() ) {
      ::GetClientRect(m_hWnd, &rc);
      dc.FillRect(&rc, COLOR_WINDOW);
    }
		return 1;
  }

  LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		CPaintDC dc(m_hWnd);
    CDC dcMem;
    HBITMAP hBmpOld;
    BITMAP info = {0};
    RECT rc = {0};
    size_t len = m_imageList.size();

    ::GetClientRect(m_hWnd, &rc);
    if( !m_bm.IsNull() ) {
      m_bm.GetBitmap(&info);
			dcMem.CreateCompatibleDC(dc);
			hBmpOld = dcMem.SelectBitmap(m_bm);
      dc.StretchBlt(0, 0, rc.right, rc.bottom, dcMem, 0, 0, info.bmWidth, info.bmHeight, SRCCOPY);
			dcMem.SelectBitmap(hBmpOld);
    } else {
      dc.SelectFont(m_baseFont);
      dc.DrawText(m_templ[len?3:2], m_templ[len?3:2].GetLength(), &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }

    rc.left = DRA::SCALEX(4); rc.right = _nCXScreen - DRA::SCALEX(4);
    rc.top = DRA::SCALEY(10); rc.bottom = rc.top + DRA::SCALEY(20);
    dc.SetTextColor(RGB(0,0,80));
    dc.SelectFont(m_infoFont);
    dc.SetBkMode(TRANSPARENT);
    if( !m_bm.IsNull() ) {
      m_tmp.Format(m_templ[0], GetFileSizeKb(m_wsFile.c_str()), m_imageWidth, m_imageHeight);
      dc.DrawText(m_tmp, m_tmp.GetLength(), &rc, DT_LEFT|DT_TOP|DT_SINGLELINE);
    }
    if( len > 0 ) {
      m_tmp.Format(m_templ[1], m_listPos + 1, len);
      dc.DrawText(m_tmp, m_tmp.GetLength(), &rc, DT_RIGHT|DT_TOP|DT_SINGLELINE);
    }

    return 0;
	}

  LRESULT OnCamera(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    DoImageCapture();
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(ID_MENU_OK != wID || m_bm.IsNull() ? IDCANCEL : wID);
		return 0;
	}

  const wchar_t *GetImageFile() {
    return m_wsFile.c_str();
  }

  int GetImageSize() {
    return GetFileSizeKb(m_wsFile.c_str());
  }

  int GetImageWidth() {
    return m_imageWidth;
  }

  int GetImageHeight() {
    return m_imageHeight;
  }

protected:
  void LockMainMenuItems() {
    UIEnable(ID_MENU_OK, !m_bm.IsNull()&&!m_imageList.empty());
		UIUpdateToolBar();
	}

  void DoImageCapture() {
    //CWaitCursor _wc;
    if( StartCamera(m_hterm, m_hWnd, _tempPath, m_wsFile) ) {
      m_bm = LoadImageFile(m_wsFile.c_str(), _nCXScreen, _nCYScreen, &m_imageWidth, &m_imageHeight);
      m_imageList.push_back(m_wsFile);
      m_listPos = m_imageList.size() - 1;
    }
    LockMainMenuItems();
  }

  void LoadImageList(const wchar_t *szFile) {
    flist(_tempPath, _imageFilter, m_imageList);
    m_listPos = 0;
    if( szFile != NULL ) {
      for( size_t i = 0; i < m_imageList.size(); i++ ) {
        if( wcscmp(szFile, m_imageList[i].c_str()) == 0 ) {
          m_listPos = i;
          break;
        }
      }
    }
  }

  void NextImage() {
    if( m_imageList.empty() ) {
      return;
    }
    m_listPos++;
    if( m_listPos >= m_imageList.size() ) {
      m_listPos = 0;
    }
    RedrawImage();
    LockMainMenuItems();
  }

  void PrevImage() {
    if( m_imageList.empty() ) {
      return;
    }
    if( m_listPos == 0 ) {
      m_listPos = m_imageList.size() - 1;
    } else {
      m_listPos--;
    }
    RedrawImage();
    LockMainMenuItems();
  }

  void RedrawImage() {
    CWaitCursor _wc;
    m_wsFile = m_imageList[m_listPos];
    m_bm = LoadImageFile(m_wsFile.c_str(), _nCXScreen, _nCYScreen, &m_imageWidth, &m_imageHeight);
    Invalidate();
  }

  bool StartCamera(HTERMINAL *h, HWND hWnd, const wchar_t *szDir, std::wstring &wsFile) 
  {
    HRESULT hr = S_OK;
    bool rc = false;
    wchar_t szFileN[MAX_PATH + 1] = L"\0";
    SHCAMERACAPTURE shcc = {0};
    shcc.cbSize = sizeof(SHCAMERACAPTURE);
    shcc.hwndOwner = hWnd;
    shcc.pszInitialDir = wmkdir_r(szDir);
    shcc.Mode = CAMERACAPTURE_MODE_STILL;
    shcc.StillQuality = CAMERACAPTURE_STILLQUALITY_NORMAL;
    shcc.VideoTypes = CAMERACAPTURE_VIDEOTYPE_ALL;
    shcc.nResolutionWidth = h->camera_width;
    shcc.nResolutionHeight = h->camera_height;
    //shcc.nVideoTimeLimit = 1;
    hr = SHCameraCapture(&shcc);
{
  // В Garmin&Asus Nuvifon M10 с WM 6.5.3 SHCameraCapture вызывает
  // CoUnilinialize лишний раз. Данный код устраняет эту проблему.
  if( CoInitializeEx(NULL, COINIT_MULTITHREADED) == S_FALSE ) {
    CoUninitialize();
  }
}
    switch( hr ) {
    case S_OK:
      _snwprintf(szFileN, CALC_BUF_SIZE(szFileN), L"%s%08X.%s", _tempPath, _imageNo++, _imageExt);
      DeleteFile(szFileN);
      if( NormalizeImageFile(shcc.szFile, szFileN, h->image_width, h->image_height, 
            h->image_fmt) ) {
        wsFile = szFileN;
        rc = true;
      } else {
        DeleteFile(szFileN);
      }
      DeleteFile(shcc.szFile);
      break;
    case S_FALSE:
      // The user canceled the Camera Capture dialog box.
      break;
    case E_INVALIDARG:
      RETAILMSG(TRUE, (L"comment: StartCamera() - An invalid argument was specified.\n"));
      break;
    case E_OUTOFMEMORY:
      RETAILMSG(TRUE, (L"comment: StartCamera() - There is not enough memory to save the image or video.\n"));
      break;
    case HRESULT_FROM_WIN32(ERROR_RESOURCE_DISABLED):
      RETAILMSG(TRUE, (L"comment: StartCamera() - The camera is disabled.\n"));
      break;
    default:
      RETAILMSG(TRUE, (L"comment: StartCamera() - An unknown error occurred.\n"));
      break;
    };
    return rc;
  }

  int GetFileSizeKb(const wchar_t *szFile) {
    int rv = 0;
    HANDLE hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if( hFile != INVALID_HANDLE_VALUE ) {
      rv = GetFileSize(hFile, NULL)/1024;
      CloseHandle(hFile);
    }
    return rv;
  }
};

class CCommentFrame : 
	public CStdDialogResizeImpl<CCommentFrame>,
	public CUpdateUI<CCommentFrame>,
	public CMessageFilter
{
protected:
  CDimEdit m_note;
  CHyperButtonTempl<CCommentFrame> m_type, m_photo;
  HTERMINAL *m_hterm;
  COMMENT *m_doc;

public:
  CCommentFrame(HTERMINAL *hterm, COMMENT *r) : m_hterm(hterm), m_doc(r) {
  }

	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		return CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_UPDATE_UI_MAP(CCommentFrame)
    UPDATE_ELEMENT(ID_CLOSE, UPDUI_MENUPOPUP|UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

  BEGIN_DLGRESIZE_MAP(CCommentFrame)
  END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CCommentFrame)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_CLOSE, OnClose)
		COMMAND_ID_HANDLER(ID_MENU_CANCEL, OnExit)
		COMMAND_ID_HANDLER(IDOK, OnExit)
		COMMAND_ID_HANDLER(IDCANCEL, OnExit)
		COMMAND_CODE_HANDLER(EN_CHANGE, OnEdit)
		CHAIN_MSG_MAP(CStdDialogResizeImpl<CCommentFrame>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UIAddToolBar(CreateMenuBar(IDR_MAINFRAME));
		UIAddChildWindowContainer(m_hWnd);
    m_note.SubclassWindow(GetDlgItem(IDC_EDIT_0));
    m_note.SetLimitText(CALC_BUF_SIZE(m_doc->note));
    m_note.SetDimText(IDC_EDIT_0);
    m_type.SubclassWindow(GetDlgItem(IDC_TYPE));
    m_type.SetParent(this, IDC_TYPE);
    m_type.EnableWindow(!m_doc->_types->empty());
    m_photo.SubclassWindow(GetDlgItem(IDC_PHOTO));
    m_photo.SetParent(this, IDC_PHOTO);
    m_photo.EnableWindow(m_hterm->image_fmt != NULL);
    _LockMainMenuItems();
		return bHandled = FALSE;
	}

  LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    memset(m_doc->note, 0, sizeof(m_doc->note));
    if( m_note.IsWindow() ) {
      m_note.GetWindowText(m_doc->note, CALC_BUF_SIZE(m_doc->note));
    }
    _LockMainMenuItems();
    return 0;
  }

  LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( _IsDataValid() ) {
      if( MessageBoxEx(m_hWnd, IDS_STR1, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES ) {
        if( _Close() ) {
          MessageBoxEx(m_hWnd, IDS_STR2, IDS_INFO, MB_ICONINFORMATION);
          EndDialog(wID);
        } else {
          MessageBoxEx(m_hWnd, IDS_STR3, IDS_ERROR, MB_ICONSTOP);
        }
      }
    }
    return 0;
  }

  LRESULT OnExit(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if( !_IsDataValid() || MessageBoxEx(m_hWnd, IDS_STR4, IDS_INFO, MB_YESNO|MB_ICONQUESTION) == IDYES )
      EndDialog(wID);
    return 0;
  }

  bool OnNavigate(HWND hWnd, UINT nID) {
    if( nID == IDC_TYPE ) {
      if( CTypesFrame(m_hterm, m_doc->_types, &m_doc->type).DoModal(m_hWnd) == ID_ACTION ) {
        _SetLabel(m_type, m_doc->type);
      }
    } else if( nID == IDC_PHOTO ) {
      CString lb;
      CImageFrame dlg(m_hterm, m_doc->photo[0] == L'\0' ? NULL : m_doc->photo);
      if( dlg.DoModal(m_hWnd) == ID_MENU_OK ) {
        wcsncpy(m_doc->photo, dlg.GetImageFile(), CALC_BUF_SIZE(m_doc->photo));
        lb.Format(LoadStringEx(IDS_STR7), dlg.GetImageWidth(), dlg.GetImageHeight(), 
          dlg.GetImageSize());
        _SetLabel(m_photo, lb);
      }
    }
    _LockMainMenuItems();
    return true;
  }

  const wchar_t *GetSubTitle() {
    return m_doc->brand_descr;
  }

protected:
  bool _IsDataValid() {
    return m_doc->type != NULL || 
      (m_doc->note[0] != L'\0' && wcslen(m_doc->note) >= m_hterm->min_text_length) ||
      m_doc->photo[0] != L'\0';
  }

  void _LockMainMenuItems() {
    UIEnable(ID_CLOSE, _IsDataValid());
		UIUpdateToolBar();
	}

  void _SetLabel(CHyperButtonTempl<CCommentFrame> &ctrl, SIMPLE_CONF *conf) {
    if( conf == NULL )
      return;
    _SetLabel(ctrl, conf->descr);
  }

  void _SetLabel(CHyperButtonTempl<CCommentFrame> &ctrl, const wchar_t *lb) {
    if( lb == NULL ) 
      return;
    CString txt;
    txt.Format(L"[%s]", lb);
    ctrl.SetLabel(txt);
  }

  bool _Close();
};

static
std::wstring wsfromRes(UINT uID, bool sys=false) {
  wchar_t buf[4095+1] = L"";
  LoadString(sys?_Module.GetModuleInstance():_Module.GetResourceInstance(), 
    uID, buf, 4095);
  buf[4095] = L'\0';
  return std::wstring(buf);
}

static
wchar_t *fencode(const wchar_t *szFile) {
  HANDLE hFile, hMap;
  LPVOID lpMem, lpRes = NULL;
  int dwSize, dwResSize = 0;
  wchar_t *rv = NULL;
  if( (hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) 
        != INVALID_HANDLE_VALUE ) {
    if( (hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL ) {
      if( (lpMem = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL ) {
        dwSize = (int)GetFileSize(hFile, NULL);
        dwResSize = Base64EncodeGetRequiredLength(dwSize);
        if( dwResSize > 0 ) {
          dwResSize += 2;
          lpRes = VirtualAlloc(NULL, dwResSize, MEM_COMMIT, PAGE_READWRITE);
          if( lpRes != NULL ) {
            memset(lpRes, 0, dwResSize);
            Base64Encode((BYTE*)lpMem, dwSize, (LPSTR)lpRes, &dwResSize);
            rv = mbstowcs_dup((LPSTR)lpRes);
            VirtualFree(lpRes, 0, MEM_RELEASE);
          }
        }
        UnmapViewOfFile(lpMem);
      }
      CloseHandle(hMap);
    }
    CloseHandle(hFile);
  }
  return rv;
}

static
slist_t &flist(const wchar_t *path, const wchar_t *filter, slist_t &fl) {
  fl.clear();
  std::wstring fn, ftempl = path; ftempl += filter;
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = FindFirstFileW(ftempl.c_str(), &ffd); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
            ffd.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF ||
            ffd.cFileName[0] == L'.'
            ) ) {
        fn = path; fn += ffd.cFileName;
        fl.push_back(fn);
      }
      memset(&ffd, 0, sizeof(ffd));
    } while( FindNextFileW(hFind, &ffd) );
    FindClose(hFind);
  }
  return fl;
}

static
void funlink(const wchar_t *path, const wchar_t *filter=L"*") {
  std::wstring fn, ftempl = path; ftempl += filter;
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = FindFirstFileW(ftempl.c_str(), &ffd); 
  if( hFind != INVALID_HANDLE_VALUE ) {
    do {
      if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
            ffd.dwFileAttributes & FILE_ATTRIBUTE_INROM || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMMODULE || 
            ffd.dwFileAttributes & FILE_ATTRIBUTE_ROMSTATICREF ||
            ffd.cFileName[0] == L'.'
            ) ) {
        fn = path; fn += ffd.cFileName;
        DeleteFile(fn.c_str());
      }
      memset(&ffd, 0, sizeof(ffd));
    } while( FindNextFileW(hFind, &ffd) );
    FindClose(hFind);
  }
  RemoveDirectory(path);
}

// Format: %id%;%descr%;%mask%;
static 
bool __comment_types(void *cookie, int line, const wchar_t **argv, int count) 
{
  COMMENT *ptr = (COMMENT*)cookie;
  SIMPLE_CONF cnf;

  if( line == 0 ) {
    return true;
  }
  if( cookie == NULL || count < 3 ) {
    return false;
  }
  if( argv[2][0] != L'\0' ) {
    if( ptr->brand_id == NULL ) {
      if( !COMPARE_ID(argv[2], L"activity") ) {
        return true;
      }
    } else {
      if( !COMPARE_ID(argv[2], L"merch") ) {
        return true;
      }
    }
  }

  memset(&cnf, 0, sizeof(SIMPLE_CONF));
  COPY_ATTR__S(cnf.id, argv[0]);
  COPY_ATTR__S(cnf.descr, argv[1]);
  ptr->_types->push_back(cnf);

  return true;
}

static
bool WriteDocPk(HTERMINAL *h, COMMENT *r, uint64_t &doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_COMMENT, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%fix_dt%", wsftime(omobus_time()).c_str());
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%doc_note%", fix_xml(r->note).c_str());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%w_cookie%", r->w_cookie);
  wsrepl(xml, L"%a_cookie%", r->a_cookie);
  wsrepl(xml, L"%account_id%", fix_xml(r->account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(r->activity_type_id).c_str());
  wsrepl(xml, L"%created_dt%", wsftime(r->ttCre).c_str());
  wsrepl(xml, L"%closed_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%brand_id%", r->brand_id == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(r->brand_id).c_str());
  wsrepl(xml, L"%comment_type_id%", r->type == NULL ? OMOBUS_UNKNOWN_ID : fix_xml(r->type->id).c_str());
  wsrepl(xml, L"%created_gps_dt%", wsftime(r->posCre.utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%created_gps_la%", ftows(r->posCre.lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%created_gps_lo%", ftows(r->posCre.lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%closed_gps_la%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%closed_gps_lo%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());

  if( r->photo[0] == L'\0' ) {
    wsrepl(xml, L"%photo%", L"");
  } else {
    wchar_t *b64;
    if( (b64 = fencode(r->photo)) == NULL ) {
      return false;
    }
    wsrepl(xml, L"%photo%", b64);
    omobus_free(b64);
  }

  return WriteOmobusXmlPk(OMOBUS_PROFILE_DOCS, OMOBUS_COMMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
bool WriteActPk(HTERMINAL *h, COMMENT *r, uint64_t &doc_id, omobus_location_t *posClo, 
  time_t ttClo) 
{
  std::wstring xml = wsfromRes(IDS_X_USER_DOCUMENT, true);
  if( xml.empty() ) {
    return false;
  }
  wsrepl(xml, L"%vstamp%", OMOBUS_VERSIONW);
  wsrepl(xml, L"%dev_id%", GetOmobusId());
  wsrepl(xml, L"%user_id%", h->user_id);
  wsrepl(xml, L"%fix_dt%", wsftime(ttClo).c_str());
  wsrepl(xml, L"%doc_type%", OMOBUS_COMMENT);
  wsrepl(xml, L"%doc_id%", itows(doc_id).c_str());
  wsrepl(xml, L"%account_id%", fix_xml(r->account_id).c_str());
  wsrepl(xml, L"%activity_type_id%", fix_xml(r->activity_type_id).c_str());
  wsrepl(xml, L"%satellite_dt%", wsftime(posClo->utc, ISO_DATETIME_FMT, true).c_str());
  wsrepl(xml, L"%latitude%", ftows(posClo->lat, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%longitude%", ftows(posClo->lon, OMOBUS_GPS_PREC).c_str());
  wsrepl(xml, L"%w_cookie%", r->w_cookie);
  wsrepl(xml, L"%a_cookie%", r->a_cookie);

  return WriteOmobusXmlPk(OMOBUS_PROFILE_ACTS, OMOBUS_USER_DOCUMENT, 
    h->user_id, h->pk_ext, h->pk_mode, xml.c_str()) == OMOBUS_OK;
}

static
void IncrementDocumentCounter(HTERMINAL *h) {
  uint32_t count = _wtoi(h->get_conf(__INT_ACTIVITY_DOCS, L"0"));
  ATLTRACE(__INT_ACTIVITY_DOCS L" = %i\n", count + 1);
  h->put_conf(__INT_ACTIVITY_DOCS, itows(count + 1).c_str());
}

static
bool CloseDoc(HTERMINAL *h, COMMENT *r) 
{
  CWaitCursor _wc;
  omobus_location_t posClo; 
  omobus_location(&posClo);
  uint64_t doc_id = GetDocId();
  time_t ttClo = omobus_time();
  if( !WriteDocPk(h, r, doc_id, &posClo, ttClo) ) {
    return false;
  }
  WriteActPk(h, r, doc_id, &posClo, ttClo);

  return true;
}

bool CCommentFrame :: _Close() 
{
  CWaitCursor _wc;
  return CloseDoc(m_hterm, m_doc);
}

static
void OnPaint(HTERMINAL *h, HDC hDC, RECT *rcDraw) 
{
  RECT rect = { 
    rcDraw->left + DRA::SCALEX(2), rcDraw->top + DRA::SCALEY(2), 
    rcDraw->right - DRA::SCALEX(2), rcDraw->bottom - DRA::SCALEY(2) };
  HFONT hOldFont = (HFONT)SelectObject(hDC, h->hFont);
  COLORREF crOldTextColor = GetTextColor(hDC), crOldBkColor = GetBkColor(hDC);

  if( h->focus ) {
    SetTextColor(hDC, OMOBUS_HLCOLOR);
    SetBkColor(hDC, OMOBUS_HLBGCOLOR);
    FillRect(hDC, rcDraw, h->hbrHL);
  } else {
    FillRect(hDC, rcDraw, h->hbrN);
  }

  SetBkMode(hDC, TRANSPARENT);
  DrawText(hDC, _wsTitle.c_str(), _wsTitle.size(), &rect, DT_LEFT|DT_SINGLELINE);

  SelectObject(hDC, hOldFont);
  SetTextColor(hDC, crOldTextColor);
  SetBkColor(hDC, crOldBkColor);
}

static 
void OnAction(HTERMINAL *h) {
  CWaitCursor _wc;
  simple_t types;
  COMMENT doc;
  memset(&doc, 0, sizeof(COMMENT));
  doc.ttCre = omobus_time();
  omobus_location(&doc.posCre);
  doc.w_cookie = h->get_conf(__INT_W_COOKIE, NULL);
  doc.a_cookie = h->get_conf(__INT_A_COOKIE, NULL);
  doc.activity_type_id = h->get_conf(__INT_ACTIVITY_TYPE_ID, OMOBUS_UNKNOWN_ID);
  doc.account_id = h->get_conf(__INT_ACCOUNT_ID, OMOBUS_UNKNOWN_ID);
  doc.brand_id = h->get_conf(__INT_MERCH_BRAND_ID, NULL);
  doc.brand_descr = h->get_conf(__INT_MERCH_BRAND, NULL);
  doc._types = &types;

  funlink(_tempPath);
  ReadOmobusManual(L"comment_types", &doc, __comment_types);
  ATLTRACE(L"comment_types = %i\n", types.size());
  _wc.Restore();

  if( CCommentFrame(h, &doc).DoModal(h->hParWnd) == ID_CLOSE ) {
    IncrementDocumentCounter(h);
  }
  funlink(_tempPath);
}

static
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch( uMsg ) {
	case WM_CREATE:
    SetWindowLong(hWnd, GWL_USERDATA, 
      (LONG)(HTERMINAL *)(((CREATESTRUCT *)lParam)->lpCreateParams));
		break;
	case WM_DESTROY:
    return 0;
	case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
      RECT rcDraw; GetClientRect(hWnd, &rcDraw);
      OnPaint((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA), hDC, &rcDraw);
			EndPaint(hWnd, &ps);
		}
		return TRUE;
	case OMOBUS_SSM_PLUGIN_SHOW:
    return 0;
	case OMOBUS_SSM_PLUGIN_HIDE:
    return 0;
	case OMOBUS_SSM_PLUGIN_SELECT:
    ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus = 
      ((BOOL)wParam)==TRUE;
    InvalidateRect(hWnd, NULL, FALSE);
		return 0;
	case OMOBUS_SSM_PLUGIN_ACTION:
  case WM_LBUTTONUP:
  	if( ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->focus )
    	OnAction((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA));
		return 0;
  case WM_LBUTTONDOWN:
    SendMessage(
      ((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hParWnd, 
      OMOBUS_SSM_PLUGIN_SELECT, 0, 
      (LPARAM)((HTERMINAL *)GetWindowLong(hWnd, GWL_USERDATA))->hWnd);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static
HWND CreatePluginWindow(HTERMINAL *h, const TCHAR *szWindowClass, WNDPROC lpfnWndProc) 
{
	WNDCLASS wc; 
  memset(&wc, 0, sizeof(WNDCLASS));
	wc.hInstance = _hInstance;
	wc.lpszClassName = szWindowClass;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
  wc.lpfnWndProc = lpfnWndProc;
	RegisterClass(&wc);
  return CreateWindowEx(0, szWindowClass, szWindowClass, WS_CHILD, 0, 0, 
    GetSystemMetrics(SM_CXSCREEN), 0, h->hParWnd, NULL, _hInstance, h);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
    _hInstance = (HINSTANCE)hModule;
    _Module.Init(NULL, _hInstance);
    DisableThreadLibraryCalls(_hInstance);
    _nCXScreen = GetSystemMetrics(SM_CXSCREEN);
    _nCYScreen = GetSystemMetrics(SM_CYSCREEN);
		break;
	case DLL_PROCESS_DETACH:
    chk_dlclose(_Module.m_hInstResource);
    _Module.Term();
		break;
	}

	return TRUE;
}

void *terminal_init(HWND hParWnd, pf_terminals_get_conf get_conf, 
  pf_terminals_put_conf put_conf, pf_register_start_item rsi )
{
  HTERMINAL *h;

  if( (h = new HTERMINAL) == NULL )
    return NULL;
  memset(h, 0, sizeof(HTERMINAL));
  h->get_conf = get_conf;
  h->put_conf = put_conf;
  h->hParWnd = hParWnd;
  h->user_id = get_conf(OMOBUS_USER_ID, NULL);
  h->pk_ext = get_conf(OMOBUS_PK_MODE, OMOBUS_PK_MODE_DEF);
  h->pk_mode = get_conf(OMOBUS_PK_LEVEL, OMOBUS_PK_LEVEL_DEF);
  h->image_fmt = get_conf(IMAGE_TYPE, NULL);
  h->image_width = _wtoi(get_conf(IMAGE_WIDTH, IMAGE_WIDTH_DEF));
  h->image_height = _wtoi(get_conf(IMAGE_HEIGHT, IMAGE_HEIGHT_DEF));
  h->camera_width = _wtoi(get_conf(CAMERA_WIDTH, CAMERA_WIDTH_DEF));
  h->camera_height = _wtoi(get_conf(CAMERA_HEIGHT, CAMERA_HEIGHT_DEF));
  h->min_text_length = _wtoi(get_conf(CAMERA_MIN_TEXT_LENGTH, CAMERA_MIN_TEXT_LENGTH_DEF));

  if( h->user_id == NULL ) {
    RETAILMSG(TRUE, (L"comment: >>> INCORRECT USER_ID <<<\n"));
    memset(h, 0, sizeof(HTERMINAL));
    delete h;
    return NULL;
  }

  _snwprintf(_tempPath, CALC_BUF_SIZE(_tempPath), L"%s.t$c$\\", GetOmobusRootPath());

  _Module.m_hInstResource = LoadMuiModule(_hInstance, get_conf(OMOBUS_MUI_NAME, OMOBUS_MUI_NAME_DEF));
  _wsTitle = wsfromRes(IDS_TITLE);

  h->hFont = CreateBaseFont(DRA::SCALEY(12), FW_NORMAL, FALSE);
  h->hbrHL = CreateSolidBrush(OMOBUS_HLBGCOLOR);
  h->hbrN = CreateSolidBrush(OMOBUS_BGCOLOR);
  h->hbrOdd = CreateSolidBrush(OMOBUS_ODDBGCOLOR);
  h->hpBord = ::CreatePen(PS_DASH, 1, OMOBUS_BORDERCOLOR);

  if( (h->hWnd = CreatePluginWindow(h, _szWindowClass, WndProc)) != NULL ) {
    rsi(h->hWnd, _nHeight, (wcsistrue(get_conf(__INT_SPLIT_ACTIVITY_SCREEN, NULL)) ?
      OMOBUS_SCREEN_ACTIVITY_TAB_1 : OMOBUS_SCREEN_ACTIVITY) | OMOBUS_SCREEN_MERCH_TASKS);
  }

  return h;
}

void terminal_deinit(void *ptr) 
{
  HTERMINAL *h = (HTERMINAL *) ptr;
  if( h == NULL )
    return;
  chk_delete_object(h->hFont);
  chk_delete_object(h->hbrHL);
  chk_delete_object(h->hbrN);
  chk_delete_object(h->hbrOdd);
  chk_delete_object(h->hpBord);
  chk_destroy_window(h->hWnd);
  memset(h, 0, sizeof(HTERMINAL));
  delete h;
}
