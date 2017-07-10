echo off

del /S /Q *.exp
del /S /Q *.aps
del /S /Q *.ncb
del /S /Q *.vcproj.vspscc
rem ## del /S /Q *.suo /A:H
rem ## del /S /Q *.suo
rem ## del /S /Q *.sln
del /S /Q *.plg
del /S /Q *.opt
del /S /Q *.ilk
del /S /Q dlldata.c
del /S /Q *_p.c
del /S /Q *.~*
del /S /Q *.user
del /S /Q *.ilk
rem ## del /S /Q *.pdb
del /S /Q *.ncb

mkdir .build-arm4i
cd .build-arm4i
del /S /Q *.*
cd ..
