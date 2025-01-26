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

#ifndef _LITE_WINMM_H
#define _LITE_WINMM_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma warning( push )
#pragma warning( disable : 4201 )	// nonstandard extension used: nameless struct/union
#include <mmsystem.h>
#pragma warning( pop )

//#define WINMM_USE_STATIC_LIB

#ifdef WINMM_USE_STATIC_LIB

	//__pragma( comment( lib, "winmm.lib" ) )

	#define _PlaySoundW	PlaySoundW

#else

	#define WINMM_STATE_SHUTDOWN	0
	#define WINMM_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pPlaySoundW )( LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound );

	extern pPlaySoundW	_PlaySoundW;

	extern unsigned char winmm_state;

	bool InitializeWinMM();
	bool UnInitializeWinMM();

#endif

#endif
