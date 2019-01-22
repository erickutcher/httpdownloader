/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2019 Eric Kutcher

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _LITE_ZLIB1_H
#define _LITE_ZLIB1_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "zlib.h"

//#define ZLIB1_USE_STATIC_LIB

#ifdef ZLIB1_USE_STATIC_LIB

	//__pragma( comment( lib, "zdll.lib" ) )

	#define _RpcStringFreeW		RpcStringFreeW
	#define _UuidToStringW		UuidToStringW
	#define _UuidCreate			UuidCreate

	//#define _zlibVersion	zlibVersion
	//#define _uncompress		uncompress
	//#define _inflateInit_	inflateInit_
	#define _inflateInit2_	inflateInit2_
	#define _inflate		inflate
	#define _inflateEnd		inflateEnd

#else

	#define ZLIB1_STATE_SHUTDOWN	0
	#define ZLIB1_STATE_RUNNING		1

	//typedef const char * ( __cdecl *pzlibVersion )( void );
	//typedef int ( __cdecl *puncompress )( Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen );
	//typedef int ( __cdecl *pinflateInit_ )( z_streamp strm, const char *version, int stream_size );
	typedef int ( __cdecl *pinflateInit2_ )( z_streamp strm, int  windowBits, const char *version, int stream_size );
	typedef int ( __cdecl *pinflate )( z_streamp strm, int flush );
	typedef int ( __cdecl *pinflateEnd )( z_streamp strm );

	//extern pzlibVersion		_zlibVersion;
	//extern puncompress		_uncompress;
	//extern pinflateInit_	_inflateInit_;
	extern pinflateInit2_	_inflateInit2_;
	extern pinflate			_inflate;
	extern pinflateEnd		_inflateEnd;

	extern unsigned char zlib1_state;

	bool InitializeZLib1();
	bool UnInitializeZLib1();

#endif

	#define ZLIB_CHUNK 16384

	#define _inflateInit( strm ) _inflateInit_( ( strm ), ZLIB_VERSION, ( int )sizeof( z_stream ) )
	#define _inflateInit2( strm, windowBits ) _inflateInit2_( ( strm ), ( windowBits ), ZLIB_VERSION, ( int )sizeof( z_stream ) )

	void *zGlobalAlloc( void *opaque, unsigned int items, unsigned int size );
	void zGlobalFree( void *opaque, void *address );

#endif
