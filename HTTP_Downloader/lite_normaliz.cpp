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

#include "lite_dlls.h"
#include "lite_normaliz.h"

#ifndef NORMALIZ_USE_STATIC_LIB

	pIdnToAscii		_IdnToAscii;
	//pIdnToUnicode	_IdnToUnicode;

	HMODULE hModule_normaliz = NULL;

	unsigned char normaliz_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeNormaliz()
	{
		if ( normaliz_state != NORMALIZ_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_normaliz = LoadLibraryDEMW( L"normaliz.dll" );

		if ( hModule_normaliz == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_normaliz, ( void ** )&_IdnToAscii, "IdnToAscii" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_normaliz, ( void ** )&_IdnToUnicode, "IdnToUnicode" ) )

		normaliz_state = NORMALIZ_STATE_RUNNING;

		return true;
	}

	bool UnInitializeNormaliz()
	{
		if ( normaliz_state != NORMALIZ_STATE_SHUTDOWN )
		{
			normaliz_state = NORMALIZ_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_normaliz ) == FALSE ? false : true );
		}

		return true;
	}

#endif
