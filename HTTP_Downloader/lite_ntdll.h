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

#ifndef _LITE_NTDLL_H
#define _LITE_NTDLL_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//#define NTDLL_USE_STATIC_LIB

#ifdef NTDLL_USE_STATIC_LIB

	//__pragma( comment( lib, "msvcrt.lib" ) )

	#include <stdio.h>

	//#define _RtlLargeIntegerDivide	RtlLargeIntegerDivide

	//#define __allmul		_allmul
	//#define __aulldiv		_aulldiv
	//#define __alldiv		_alldiv
	//#define __allshr		_allshr
	//#define __ftol2		_ftol2
	
	#define _abs			abs

	#define _memcpy			memcpy
	#define _memset			memset
	#define _memcmp			memcmp
	#define _memmove		memmove

	//#define _wcsncpy		wcsncpy

	//#define _strcmp		strcmp
	//#define _strncmp		strncmp
	//#define __stricmp		_stricmp
	//#define __strnicmp	_strnicmp

	//#define _wcscmp		wcscmp
	//#define _wcsncmp		wcsncmp
	//#define __wcsicmp		_wcsicmp
	//#define __wcsnicmp	_wcsnicmp

	//#define _strchr		strchr
	//#define _strstr		strstr

	//#define __atoi64		_atoi64
	#define _strtoul		strtoul
	#define _wcstoul		wcstoul
	//#define __snprintf	_snprintf
	#define __snprintf		sprintf_s
	//#define __snwprintf	_snwprintf
	#define __snwprintf		swprintf_s

	#define _memcpy_s		memcpy_s
	#define _wmemcpy_s( dest, size, src, count )	memcpy_s( dest, sizeof( wchar_t ) * ( size ), src, sizeof( wchar_t ) * ( count ) )

	#define THREAD_RETURN	unsigned int WINAPI

	#define _CreateThread	_beginthreadex
	#define _ExitThread		_endthreadex

#else

	#define NTDLL_STATE_SHUTDOWN		0
	#define NTDLL_STATE_RUNNING			1

	//typedef LARGE_INTEGER ( WINAPIV *pRtlLargeIntegerDivide )( LARGE_INTEGER Dividend, LARGE_INTEGER Divisor, PLARGE_INTEGER Remainder );

	//typedef __int64 ( WINAPIV *p_allmul )( __int64 a, __int64 b );
	//typedef unsigned __int64 ( WINAPIV *p_aulldiv )( unsigned __int64 a, unsigned __int64 b );
	//typedef __int64 ( WINAPIV *p_alldiv )( __int64 a, __int64 b );
	//typedef __int64 ( WINAPIV *p_allshr )( __int64 v, unsigned char s );
	//typedef long ( WINAPIV *p_ftol2 )( float v );
	
	typedef int ( WINAPIV *pabs )( int n );

	typedef void * ( WINAPIV *pmemcpy )( void *dest, const void *src, size_t count );
	typedef void * ( WINAPIV *pmemset )( void *dest, int c, size_t count );
	typedef int ( WINAPIV *pmemcmp )( const void *buf1, const void *buf2, size_t count );
	typedef void * ( WINAPIV *pmemmove )( void *dest, const void *src, size_t count );

	//typedef wchar_t * ( WINAPIV *pwcsncpy )( wchar_t *strDest, const wchar_t *strSource, size_t count );

	//typedef int ( WINAPIV *pstrcmp )( const char *string1, const char *string2 );
	//typedef int ( WINAPIV *pstrncmp )( const char *string1, const char *string2, size_t count );
	//typedef int ( WINAPIV *p_stricmp )( const char *string1, const char *string2 );
	//typedef int ( WINAPIV *p_strnicmp )( const char *string1, const char *string2, size_t count );

	//typedef int ( WINAPIV *pwcscmp )( const wchar_t *string1, const wchar_t *string2 );
	//typedef int ( WINAPIV *pwcsncmp )( const wchar_t *string1, const wchar_t *string2, size_t count );
	//typedef int ( WINAPIV *p_wcsicmp )( const wchar_t *string1, const wchar_t *string2 );
	//typedef int ( WINAPIV *p_wcsnicmp )( const wchar_t *string1, const wchar_t *string2, size_t count );

	//typedef char * ( WINAPIV *pstrchr )( const char *str, int c );
	//typedef char * ( WINAPIV *pstrstr )( const char *str, const char *strSearch );

	//typedef __int64 ( WINAPIV *p_atoi64 )( const char *str );
	typedef unsigned long ( WINAPIV *pstrtoul )( const char *nptr, char **endptr, int base );
	typedef unsigned long ( WINAPIV *pwcstoul )( const wchar_t *nptr, wchar_t **endptr, int base );
	typedef int ( WINAPIV *p_snprintf )( char *buffer, size_t count, const char *format, ... );
	typedef int ( WINAPIV *p_snwprintf )( wchar_t *buffer, size_t count, const wchar_t *format, ... );

	//extern pRtlLargeIntegerDivide	_RtlLargeIntegerDivide;

	//extern p_allmul		__allmul;
	//extern p_aulldiv		__aulldiv;
	//extern p_alldiv		__alldiv;
	//extern p_allshr		__allshr;
	//extern p_ftol2		__ftol2;
	
	extern pabs				_abs;

	extern pmemcpy			_memcpy;
	extern pmemset			_memset;
	extern pmemcmp			_memcmp;
	extern pmemmove			_memmove;

	//extern pwcsncpy		_wcsncpy;

	//extern pstrcmp		_strcmp;
	//extern pstrncmp		_strncmp;
	//extern p_stricmp		__stricmp;
	//extern p_strnicmp		__strnicmp;

	//extern pwcscmp		_wcscmp;
	//extern pwcsncmp		_wcsncmp;
	//extern p_wcsicmp		__wcsicmp;
	//extern p_wcsnicmp		__wcsnicmp;

	//extern pstrchr		_strchr;
	//extern pstrstr		_strstr;

	//extern p_atoi64		__atoi64;
	extern pstrtoul			_strtoul;
	extern pwcstoul			_wcstoul;
	extern p_snprintf		__snprintf;
	extern p_snwprintf		__snwprintf;

	extern unsigned char ntdll_state;

	bool InitializeNTDLL();
	bool UnInitializeNTDLL();

	void * WINAPIV _memcpy_s( void *dest, size_t size, const void *src, size_t count );
	void * WINAPIV _wmemcpy_s( void *dest, size_t size, const void *src, size_t count );

	#define THREAD_RETURN	DWORD WINAPI

	#define _CreateThread	CreateThread
	#define _ExitThread		ExitThread

#endif

//wchar_t * WINAPIV _wcsncpy_s( wchar_t *strDest, size_t size, const wchar_t *strSource, size_t count );
wchar_t * WINAPI _wcsncpy_s( wchar_t *strDest, int size, const wchar_t *strSource, int count );

#define _memzero( dest, count ) _memset( dest, 0, count )

#endif
