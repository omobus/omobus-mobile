# 1. Dependencies

First of all your should have the following software:

    * Microsoft Visual Studio 2005 Standard or Professional Edition
    * Windows Mobile 6 SDK
    * Windows Mobile 6.5.3 DTK

You may download SDK from the official Microsoft site: http://www.microsoft.com.

# 2. Building omobus-droid

Open sln files and build the in the following order:

    * zlib
    * bzip2
    * libcurl
    * nmealib
    * tools
    * daemons
    * terminals

Result files will be stored in the contrib/ folder.

# 3. LIMITATIONS

Windows Mobile imposes the following restrictions:

    * maximum adress space is 32 Mb;
    * maximum photo size is 960X1600;
    * ActiveSync does not support non-blocking sockets;
    * bzip2 needs ~9Mb for compess/decompress data.

# 4. LICENSE

GPLv2
