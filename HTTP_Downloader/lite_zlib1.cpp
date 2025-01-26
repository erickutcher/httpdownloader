/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2025 Eric Kutcher

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

#include "lite_dlls.h"
#include "lite_zlib1.h"
#include "lite_ntdll.h"

#ifndef ZLIB1_USE_STATIC_LIB

	//pzlibVersion	_zlibVersion;
	//puncompress		_uncompress;
	//pinflateInit_	_inflateInit_;
	pinflateInit2_	_inflateInit2_;
	pinflate		_inflate;
	pinflateEnd		_inflateEnd;

	HMODULE hModule_zlib1 = NULL;

	unsigned char zlib1_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeZLib1()
	{
		if ( zlib1_state != ZLIB1_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_zlib1 = LoadLibraryDEMW( L"zlib1.dll" );

		if ( hModule_zlib1 == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_zlibVersion, "zlibVersion" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_uncompress, "uncompress" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_inflateInit_, "inflateInit_" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_inflateInit2_, "inflateInit2_" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_inflate, "inflate" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_zlib1, ( void ** )&_inflateEnd, "inflateEnd" ) )

		zlib1_state = ZLIB1_STATE_RUNNING;

		return true;
	}

	bool UnInitializeZLib1()
	{
		if ( zlib1_state != ZLIB1_STATE_SHUTDOWN )
		{
			zlib1_state = ZLIB1_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_zlib1 ) == FALSE ? false : true );
		}

		return true;
	}

#endif

void *zGlobalAlloc( void *opaque, unsigned int items, unsigned int size )
{
	opaque = Z_NULL;
	return GlobalAlloc( GMEM_FIXED, size * items );
}

void zGlobalFree( void *opaque, void *address )
{
	opaque = Z_NULL;
	GlobalFree( address );
}
