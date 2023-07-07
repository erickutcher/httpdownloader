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
#include "lite_comdlg32.h"

#ifndef COMDLG32_USE_STATIC_LIB

	pChooseColorW			_ChooseColorW;
	pChooseFontW			_ChooseFontW;

	pGetOpenFileNameW		_GetOpenFileNameW;
	pGetSaveFileNameW		_GetSaveFileNameW;

	HMODULE hModule_comdlg32 = NULL;

	unsigned char comdlg32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeComDlg32()
	{
		if ( comdlg32_state != COMDLG32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_comdlg32 = LoadLibraryDEMW( L"comdlg32.dll" );

		if ( hModule_comdlg32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comdlg32, ( void ** )&_ChooseColorW, "ChooseColorW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comdlg32, ( void ** )&_ChooseFontW, "ChooseFontW" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comdlg32, ( void ** )&_GetOpenFileNameW, "GetOpenFileNameW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comdlg32, ( void ** )&_GetSaveFileNameW, "GetSaveFileNameW" ) )

		comdlg32_state = COMDLG32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeComDlg32()
	{
		if ( comdlg32_state != COMDLG32_STATE_SHUTDOWN )
		{
			comdlg32_state = COMDLG32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_comdlg32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
