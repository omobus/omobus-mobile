"C:\Program Files\Windows Mobile 6 SDK\Tools\CabWiz\makecab.exe" _setup.xml ..\contrib\_setup.cab
"C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\Bin\signtool.exe" sign /f ..\cert\omobus-mobile.pfx ..\contrib\_setup.cab
