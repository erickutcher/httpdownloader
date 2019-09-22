/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
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
#include "lite_gdi32.h"

#ifndef GDI32_USE_STATIC_LIB

	pBitBlt						_BitBlt;
	pCreateCompatibleBitmap		_CreateCompatibleBitmap;
	pCreateCompatibleDC			_CreateCompatibleDC;
	//pCreateDCW					_CreateDCW;
	pCreateFontIndirectW		_CreateFontIndirectW;
	pCreatePatternBrush			_CreatePatternBrush;
	pCreatePen					_CreatePen;
	pCreateSolidBrush			_CreateSolidBrush;
	pDeleteDC					_DeleteDC;
	pDeleteObject				_DeleteObject;
	//pExcludeClipRect			_ExcludeClipRect;
	//pGdiAlphaBlend				_GdiAlphaBlend;
	//pGdiGradientFill			_GdiGradientFill;
	//pGetDeviceCaps				_GetDeviceCaps;
	//pGetObjectW					_GetObjectW;
	pGetStockObject				_GetStockObject;
	//pGetTextExtentPoint32W		_GetTextExtentPoint32W;
	pGetTextMetricsW			_GetTextMetricsW;
	pPatBlt						_PatBlt;
	pRectangle					_Rectangle;
	pSelectObject				_SelectObject;
	//pSetBkColor					_SetBkColor;
	pSetBkMode					_SetBkMode;
	pSetBrushOrgEx				_SetBrushOrgEx;
	pSetTextColor				_SetTextColor;

	HMODULE hModule_gdi32 = NULL;

	unsigned char gdi32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeGDI32()
	{
		if ( gdi32_state != GDI32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_gdi32 = LoadLibraryDEMW( L"gdi32.dll" );

		if ( hModule_gdi32 == NULL )
		{
			return false;
		}

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_BitBlt, "BitBlt" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreateCompatibleBitmap, "CreateCompatibleBitmap" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreateCompatibleDC, "CreateCompatibleDC" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreateDCW, "CreateDCW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreateFontIndirectW, "CreateFontIndirectW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreatePatternBrush, "CreatePatternBrush" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreatePen, "CreatePen" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_CreateSolidBrush, "CreateSolidBrush" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_DeleteDC, "DeleteDC" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_DeleteObject, "DeleteObject" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_ExcludeClipRect, "ExcludeClipRect" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GdiAlphaBlend, "GdiAlphaBlend" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GdiGradientFill, "GdiGradientFill" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GetDeviceCaps, "GetDeviceCaps" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GetObjectW, "GetObjectW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GetStockObject, "GetStockObject" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GetTextExtentPoint32W, "GetTextExtentPoint32W" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_GetTextMetricsW, "GetTextMetricsW" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_PatBlt, "PatBlt" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_Rectangle, "Rectangle" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_SelectObject, "SelectObject" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_SetBkColor, "SetBkColor" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_SetBkMode, "SetBkMode" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_SetBrushOrgEx, "SetBrushOrgEx" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_gdi32, ( void ** )&_SetTextColor, "SetTextColor" ) )

		gdi32_state = GDI32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeGDI32()
	{
		if ( gdi32_state != GDI32_STATE_SHUTDOWN )
		{
			gdi32_state = GDI32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_gdi32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
