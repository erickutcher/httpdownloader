/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
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
#include "lite_winmm.h"

#ifndef WINMM_USE_STATIC_LIB

	pPlaySoundW	_PlaySoundW;

	HMODULE hModule_winmm = NULL;

	unsigned char winmm_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeWinMM()
	{
		if ( winmm_state != WINMM_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_winmm = LoadLibraryDEMW( L"winmm.dll" );

		if ( hModule_winmm == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_winmm, ( void ** )&_PlaySoundW, "PlaySoundW" ) )

		winmm_state = WINMM_STATE_RUNNING;

		return true;
	}

	bool UnInitializeWinMM()
	{
		if ( winmm_state != WINMM_STATE_SHUTDOWN )
		{
			winmm_state = WINMM_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_winmm ) == FALSE ? false : true );
		}

		return true;
	}

#endif
