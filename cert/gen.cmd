"C:\Program Files\Microsoft Visual Studio 9.0\SmartDevices\SDK\SDKTools\makecert.exe" -r -sv omobus-mobile.pvk -n "CN=OMOBUS" -b 01/01/2009 -e 01/01/2099 omobus-mobile.cer
"C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin\pvk2pfx.exe" -pvk omobus-mobile.pvk -spc omobus-mobile.cer -pfx omobus-mobile.pfx
"X:\AK-OBS\#cert#\openssl.exe" sha1 omobus-mobile.cer > omobus-mobile.sha1.txt
"X:\AK-OBS\#cert#\openssl.exe" base64 -in omobus-mobile.cer > omobus-mobile.base64.txt
