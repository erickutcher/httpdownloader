/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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
#include "lite_uxtheme.h"

#ifndef UXTHEME_USE_STATIC_LIB

	//pOpenThemeData			_OpenThemeData;
	//pCloseThemeData			_CloseThemeData;
	//pDrawThemeBackground	_DrawThemeBackground;
	//pGetThemeColor			_GetThemeColor;
	pIsThemeActive			_IsThemeActive;
	//pEnableThemeDialogTexture	_EnableThemeDialogTexture;
	//pDrawThemeParentBackground	_DrawThemeParentBackground;

	HMODULE hModule_uxtheme = NULL;

	unsigned char uxtheme_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeUXTheme()
	{
		if ( uxtheme_state != UXTHEME_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_uxtheme = LoadLibraryDEMW( L"uxtheme.dll" );

		if ( hModule_uxtheme == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_OpenThemeData, "OpenThemeData" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_CloseThemeData, "CloseThemeData" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_DrawThemeBackground, "DrawThemeBackground" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_GetThemeColor, "GetThemeColor" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_IsThemeActive, "IsThemeActive" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_EnableThemeDialogTexture, "EnableThemeDialogTexture" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_uxtheme, ( void ** )&_DrawThemeParentBackground, "DrawThemeParentBackground" ) )

		uxtheme_state = UXTHEME_STATE_RUNNING;

		return true;
	}

	bool UnInitializeUXTheme()
	{
		if ( uxtheme_state != UXTHEME_STATE_SHUTDOWN )
		{
			uxtheme_state = UXTHEME_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_uxtheme ) == FALSE ? false : true );
		}

		return true;
	}

#endif
