/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2024 Eric Kutcher

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

#ifndef _LITE_NORMALIZ_H
#define _LITE_NORMALIZ_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winnls.h>

//#define NORMALIZ_USE_STATIC_LIB

#ifdef NORMALIZ_USE_STATIC_LIB

	//__pragma( comment( lib, "normaliz.lib" ) )

	#define _IdnToAscii		IdnToAscii
	//#define _IdnToUnicode	IdnToUnicode

#else

	#define NORMALIZ_STATE_SHUTDOWN	0
	#define NORMALIZ_STATE_RUNNING	1

	typedef int ( WINAPI *pIdnToAscii )( DWORD dwFlags, LPCWSTR lpUnicodeCharStr, int cchUnicodeChar, LPWSTR lpASCIICharStr, int cchASCIIChar );
	//typedef int ( WINAPI *pIdnToUnicode )( DWORD dwFlags, LPCWSTR lpASCIICharStr, int cchASCIIChar, LPWSTR lpUnicodeCharStr, int cchUnicodeChar );

	extern pIdnToAscii		_IdnToAscii;
	//extern pIdnToUnicode	_IdnToUnicode;

	extern unsigned char normaliz_state;

	bool InitializeNormaliz();
	bool UnInitializeNormaliz();

#endif

#endif
