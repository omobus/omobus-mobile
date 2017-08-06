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
#include <imaging.h>

static
HBITMAP _IImage2HBITMAP(IImage *pImage, int width, int height) 
{
  RECT rc = {0};
  HDC hDC = NULL, hBtmDC = NULL;
  HGDIOBJ hOldObj = NULL;
  HBITMAP hResult = NULL;
  if( pImage == NULL ) {
    return NULL;
  }
  if( (hDC = ::GetWindowDC(NULL)) != NULL ) {
    if( (hBtmDC = ::CreateCompatibleDC(hDC)) != NULL ) {
      if( (hResult = ::CreateCompatibleBitmap(hDC, width, height)) != NULL ) {
        hOldObj = ::SelectObject(hBtmDC, hResult);
        rc.left = rc.top = 0; rc.right = width; rc.bottom = height;
        pImage->Draw(hBtmDC, &rc, NULL);
        ::SelectObject(hDC, hOldObj);
      }
      ::DeleteDC(hBtmDC);
    }
    ::ReleaseDC(NULL, hDC);
  }
  pImage->Release();
  return hResult;
}

HBITMAP LoadImageFile(const wchar_t *szFile, int max_width, int max_height, 
  int *orig_width, int *orig_height)
{
  HRESULT hr = S_OK;
  IImagingFactory *pImgFactory = NULL;
  IImage *pImage = NULL, *pImageN = NULL;
  IBitmapImage *pBitmapImage = NULL, *pBitmapImageR = NULL;
  IBasicBitmapOps *pBasicBitmapOps = NULL;
  ImageInfo imageInfo = {0};
  UINT width = 0, height = 0;

  hr = CoCreateInstance(CLSID_ImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        IID_IImagingFactory, (void **)&pImgFactory);
  if( FAILED(hr) ) {
    RETAILMSG(TRUE, (L"LoadImageFile: Unable to create ImagingFactory. 0x%08X\n", hr));
    return NULL;
  }

  hr = pImgFactory->CreateImageFromFile(szFile, &pImage);
  if( SUCCEEDED(hr) && SUCCEEDED(pImage->GetImageInfo(&imageInfo)) ) {
    if( imageInfo.Width > imageInfo.Height ) {
      width = min(imageInfo.Height, (UINT)max_height);
      height = min(imageInfo.Width, (UINT)max_width);
    } else {
      width = min(imageInfo.Width, (UINT)max_width);
      height = min(imageInfo.Height, (UINT)max_height);
    }
    hr = pImgFactory->CreateBitmapFromImage(pImage, width, height,
      PixelFormatDontCare, InterpolationHintDefault, &pBitmapImage);
    if( SUCCEEDED(hr) ) {
      if( imageInfo.Width > imageInfo.Height ) {
        hr = pBitmapImage->QueryInterface(IID_IBasicBitmapOps, (void **)&pBasicBitmapOps);
        if( SUCCEEDED(hr) ) {
          hr = pBasicBitmapOps->Rotate(90, InterpolationHintDefault, &pBitmapImageR);
          if( SUCCEEDED(hr) ) {
            pBitmapImageR->QueryInterface(IID_IImage, (void **)&pImageN);
            pBitmapImageR->Release();
          } else {
            pBitmapImage->QueryInterface(IID_IImage, (void **)&pImageN);
          }
          pBasicBitmapOps->Release();
        } else {
          RETAILMSG(TRUE, (L"LoadImageFile: Unable to get IBasicBitmapOps (0x%X)\n", hr));
        }
      } else {
        pBitmapImage->QueryInterface(IID_IImage, (void **)&pImageN);
      }
      pBitmapImage->Release();
    } else {
      RETAILMSG(TRUE, (L"LoadImageFile: Unable to create bitmap (0x%X)\n", hr));
    }
    pImage->Release();
  } else {
    RETAILMSG(TRUE, (L"LoadImageFile: Unable to load image file (0x%X): %s\n", hr, szFile));
  }
  pImgFactory->Release();

  if( orig_width != NULL ) {
    *orig_width = (int)imageInfo.Width;
  }
  if( orig_height != NULL ) {
    *orig_height = (int)imageInfo.Height;
  }

  return _IImage2HBITMAP(pImageN, width, height);
}

