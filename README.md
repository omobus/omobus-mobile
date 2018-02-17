# 1. Dependencies

First of all your should have the following software:

    * Microsoft Visual Studio 2005 Standard or Professional Edition
    * Windows Mobile 6 SDK
    * Windows Mobile 6.5.3 DTK

You may download SDK from the official Microsoft site: http://www.microsoft.com.

# 2. Building omobus-droid

Open sln files and build then in the following order:

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

Copyright (c) 2006 - 2017 ak-obs, Ltd. <info@omobus.net>
All rights reserved.

This program is a free software. Redistribution and use in source
and binary forms, with or without modification, are permitted provided 
under the [GPLv2](http://www.gnu.org/licenses/gpl-2.0.html) license.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
