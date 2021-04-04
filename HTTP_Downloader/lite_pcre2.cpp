/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2021 Eric Kutcher

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
#include "lite_pcre2.h"

#ifndef PCRE2_USE_STATIC_LIB

	ppcre2_compile_16 _pcre2_compile_16;
	ppcre2_match_16 _pcre2_match_16;
	ppcre2_match_data_create_from_pattern_16 _pcre2_match_data_create_from_pattern_16;
	ppcre2_match_data_free_16 _pcre2_match_data_free_16;
	ppcre2_code_free_16 _pcre2_code_free_16;
	//ppcre2_get_ovector_pointer_16 _pcre2_get_ovector_pointer_16;

	HMODULE hModule_pcre2 = NULL;

	unsigned char pcre2_state = 0;	// 0 = Not running, 1 = running.

	bool InitializePCRE2()
	{
		if ( pcre2_state != PCRE2_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_pcre2 = LoadLibraryDEMW( L"libpcre2-16-0.dll" );

		if ( hModule_pcre2 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_compile_16, "pcre2_compile_16" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_match_16, "pcre2_match_16" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_match_data_create_from_pattern_16, "pcre2_match_data_create_from_pattern_16" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_match_data_free_16, "pcre2_match_data_free_16" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_code_free_16, "pcre2_code_free_16" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_pcre2, ( void ** )&_pcre2_get_ovector_pointer_16, "pcre2_get_ovector_pointer_16" ) )

		pcre2_state = PCRE2_STATE_RUNNING;

		return true;
	}

	bool UnInitializePCRE2()
	{
		if ( pcre2_state != PCRE2_STATE_SHUTDOWN )
		{
			pcre2_state = PCRE2_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_pcre2 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
