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
#include <imaging.h>

#include <objbase.h>

bool NormalizeImageFile(const wchar_t *szFile, const wchar_t *szFileN, 
  int width, int height, const wchar_t *fmt)
{
  HRESULT hr = S_OK;
  IImagingFactory *pImgFactory = NULL;
  IImage *pImage = NULL, *pImageN = NULL;
  IBitmapImage *pBitmapImage = NULL;
  IImageEncoder *imageEncoder = NULL;
  IImageSink *imageSink = NULL;
  ImageInfo imageInfo = {0};
  ImageCodecInfo *imageCodecInfo= NULL;
  UINT imageCodecCount = 0;
  CLSID encoderClassId = {0};

  hr = CoCreateInstance(CLSID_ImagingFactory, NULL, CLSCTX_INPROC_SERVER,
          IID_IImagingFactory, (void **)&pImgFactory);
  if( FAILED(hr) ) {
    RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to create ImagingFactory. 0x%08X\n", hr));
    return false;
  }

  hr = pImgFactory->CreateImageFromFile(szFile, &pImage);
  if( FAILED(hr) || FAILED(pImage->GetImageInfo(&imageInfo)) ) {
    RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to load image file (0x%X): %s\n", hr, szFile));
    pImgFactory->Release();
    return false;
  }

  hr = pImgFactory->CreateBitmapFromImage(pImage, 
          min(imageInfo.Width, imageInfo.Width < imageInfo.Height ? min(width, height) : max(width, height)), 
          min(imageInfo.Height, imageInfo.Width < imageInfo.Height ? max(width, height) : min(width, height)), 
          PixelFormatDontCare, InterpolationHintDefault, &pBitmapImage);
  if( SUCCEEDED(hr) ) {
    hr = pBitmapImage->QueryInterface(IID_IImage, (void **)&pImageN);
    if( SUCCEEDED(hr) ) {
      pImgFactory->GetInstalledEncoders(&imageCodecCount, &imageCodecInfo);
      for( UINT i = 0; i < imageCodecCount; i++ ) {
        if( wcscmp(imageCodecInfo[i].MimeType, fmt) == 0 ) {
          encoderClassId = imageCodecInfo[i].Clsid;
          break;
        }
      }
      CoTaskMemFree(imageCodecInfo);
      hr = pImgFactory->CreateImageEncoderToFile(&encoderClassId, szFileN, &imageEncoder);
      if( SUCCEEDED(hr) ) {
        hr = imageEncoder->GetEncodeSink(&imageSink);
        if( SUCCEEDED(hr) ) {
          if( pImageN->PushIntoSink(imageSink) != S_OK ) {
            RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to execute PushIntoSink(0x%X)\n", hr));
          }
          imageSink->Release();
        } else {
          RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to get GetEncodeSink (0x%X)\n", hr));
        }
        imageEncoder->TerminateEncoder();
        imageEncoder->Release();
      } else {
        RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to CreateImageEncoderToFile (0x%X)\n", hr));
      }
      pImageN->Release();
    } else {
      RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to get IID_IImage interface (0x%X)\n", hr));
    }
    pBitmapImage->Release();
  } else {
    RETAILMSG(TRUE, (L"NormalizeImageFile: Unable to CreateBitmapFromImage (0x%X)\n", hr));
  }
  pImage->Release();
  pImgFactory->Release();

  return hr == S_OK;
}
