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

#ifndef _LITE_COMDLG32_H
#define _LITE_COMDLG32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commdlg.h>

//#define COMDLG32_USE_STATIC_LIB

#ifdef COMDLG32_USE_STATIC_LIB

	//__pragma( comment( lib, "comdlg32.lib" ) )

	//#define _ChooseColorW			ChooseColorW
	//#define _ChooseFontW			ChooseFontW

	#define _GetOpenFileNameW		GetOpenFileNameW
	#define _GetSaveFileNameW		GetSaveFileNameW

#else

	#define COMDLG32_STATE_SHUTDOWN		0
	#define COMDLG32_STATE_RUNNING		1

	//typedef BOOL ( WINAPI *pChooseColorW )( LPCHOOSECOLOR lpcc );
	//typedef BOOL ( WINAPI *pChooseFontW )( LPCHOOSEFONT lpcf );

	typedef BOOL ( WINAPI *pGetOpenFileNameW )( LPOPENFILENAME lpofn );
	typedef BOOL ( WINAPI *pGetSaveFileNameW )( LPOPENFILENAME lpofn );

	//extern pChooseColorW			_ChooseColorW;
	//extern pChooseFontW				_ChooseFontW;

	extern pGetOpenFileNameW		_GetOpenFileNameW;
	extern pGetSaveFileNameW		_GetSaveFileNameW;

	extern unsigned char comdlg32_state;

	bool InitializeComDlg32();
	bool UnInitializeComDlg32();

#endif

#endif
