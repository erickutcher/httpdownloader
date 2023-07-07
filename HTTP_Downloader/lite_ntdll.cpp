/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2023 Eric Kutcher

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
#include "lite_ntdll.h"

#ifndef NTDLL_USE_STATIC_LIB

	#ifdef __cplusplus
		extern "C"
		{
	#endif
			int _fltused = 1;	// Dummy value so that the linker shuts up.
	#ifdef __cplusplus
		}
	#endif

	//pRtlLargeIntegerDivide	_RtlLargeIntegerDivide;

	//p_allmul		__allmul;
	//p_aulldiv		__aulldiv;
	//p_alldiv		__alldiv;
	//p_allshr		__allshr;
	//p_ftol2		__ftol2;

	pabs			_abs;
	pceil			_ceil;

	pmemcpy			_memcpy;
	pmemset			_memset;
	pmemcmp			_memcmp;
	pmemmove		_memmove;

	//pwcsncpy		_wcsncpy;

	//pstrcmp			_strcmp;
	//pstrncmp		_strncmp;
	//p_stricmp		__stricmp;
	//p_strnicmp		__strnicmp;

	//pwcscmp			_wcscmp;
	//pwcsncmp		_wcsncmp;
	//p_wcsicmp		__wcsicmp;
	//p_wcsnicmp		__wcsnicmp;

	//pstrchr			_strchr;
	//pstrstr			_strstr;

	//p_atoi64		__atoi64;
	pstrtoul		_strtoul;
	pwcstoul		_wcstoul;
	p_snprintf		__snprintf;
	p_snwprintf		__snwprintf;

	HMODULE hModule_ntdll = NULL;

	unsigned char ntdll_state = NTDLL_STATE_SHUTDOWN;	// 0 = Not running, 1 = running.

	bool InitializeNTDLL()
	{
		if ( ntdll_state != NTDLL_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_ntdll = LoadLibraryDEMW( L"ntdll.dll" );

		if ( hModule_ntdll == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_RtlLargeIntegerDivide, "RtlLargeIntegerDivide" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__allmul, "_allmul" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__aulldiv, "_aulldiv" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__alldiv, "_alldiv" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__allshr, "_allshr" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__ftol2, "_ftol" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_abs, "abs" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_ceil, "ceil" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_memcpy, "memcpy" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_memset, "memset" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_memcmp, "memcmp" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_memmove, "memmove" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_wcsncpy, "wcsncpy" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_strcmp, "strcmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_strncmp, "strncmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__stricmp, "_stricmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__strnicmp, "_strnicmp" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_wcscmp, "wcscmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_wcsncmp, "wcsncmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__wcsicmp, "_wcsicmp" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__wcsnicmp, "_wcsnicmp" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_strchr, "strchr" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_strstr, "strstr" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__atoi64, "_atoi64" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_strtoul, "strtoul" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&_wcstoul, "wcstoul" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__snprintf, "_snprintf" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_ntdll, ( void ** )&__snwprintf, "_snwprintf" ) )

		ntdll_state = NTDLL_STATE_RUNNING;

		return true;
	}

	bool UnInitializeNTDLL()
	{
		if ( ntdll_state != NTDLL_STATE_SHUTDOWN )
		{
			ntdll_state = NTDLL_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_ntdll ) == FALSE ? false : true );
		}

		return true;
	}

	void * WINAPIV _memcpy_s( void *dest, size_t size, const void *src, size_t count )
	{
		if ( src == NULL || size < count )
		{
			_memzero( dest, size );
			return dest;
		}

		return _memcpy( dest, src, count );
	}

	void * WINAPIV _wmemcpy_s( void *dest, size_t size, const void *src, size_t count )
	{
		size_t wsize = sizeof( wchar_t ) * size;
		size_t wcount = sizeof( wchar_t ) * count;

		if ( src == NULL || wsize < wcount )
		{
			_memzero( dest, wsize );
			return dest;
		}

		return _memcpy( dest, src, wcount );
	}

#endif

//wchar_t * WINAPIV _wcsncpy_s( wchar_t *strDest, size_t size, const wchar_t *strSource, size_t count )
wchar_t * WINAPIV _wcsncpy_s( wchar_t *strDest, int size, const wchar_t *strSource, int count )
{
	if ( size < count )
	{
		return NULL;
	}

	//return _wcsncpy( strDest, strSource, count );
	return lstrcpynW( strDest, strSource, count );
}
