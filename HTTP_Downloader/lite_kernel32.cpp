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
#include "lite_kernel32.h"

#ifndef KERNEL32_USE_STATIC_LIB

	pSetFileInformationByHandle		_SetFileInformationByHandle;
	pGetUserDefaultLocaleName		_GetUserDefaultLocaleName;

	HMODULE hModule_kernel32 = NULL;

	unsigned char kernel32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeKernel32()
	{
		if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_kernel32 = LoadLibraryDEMW( L"kernel32.dll" );

		if ( hModule_kernel32 == NULL )
		{
			return false;
		}

		// Both of these functions are for Windows Vista and newer.
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_SetFileInformationByHandle, "SetFileInformationByHandle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_kernel32, ( void ** )&_GetUserDefaultLocaleName, "GetUserDefaultLocaleName" ) )

		kernel32_state = KERNEL32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeKernel32()
	{
		if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
		{
			kernel32_state = KERNEL32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_kernel32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
