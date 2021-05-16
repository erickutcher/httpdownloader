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
#include "lite_comctl32.h"

#ifndef COMCTL32_USE_STATIC_LIB

	//pImageList_Create			_ImageList_Create;
	pImageList_Destroy			_ImageList_Destroy;
	//pImageList_Add			_ImageList_Add;
	pImageList_LoadImageW		_ImageList_LoadImageW;
	//pImageList_ReplaceIcon	_ImageList_ReplaceIcon;
	//pImageList_GetIcon		_ImageList_GetIcon;

	//pInitCommonControlsEx		_InitCommonControlsEx;

	pMakeDragList				_MakeDragList;
	//pDrawInsert				_DrawInsert;
	pLBItemFromPt				_LBItemFromPt;

	//pImageList_BeginDrag		_ImageList_BeginDrag;
	//pImageList_DragEnter		_ImageList_DragEnter;
	//pImageList_DragMove			_ImageList_DragMove;
	//pImageList_DragShowNolock	_ImageList_DragShowNolock;
	//pImageList_EndDrag			_ImageList_EndDrag;

	HMODULE hModule_comctl32 = NULL;

	unsigned char comctl32_state = 0;	// 0 = Not running, 1 = running.

	bool InitializeComCtl32()
	{
		if ( comctl32_state != COMCTL32_STATE_SHUTDOWN )
		{
			return true;
		}

		hModule_comctl32 = LoadLibraryDEMW( L"comctl32.dll" );

		if ( hModule_comctl32 == NULL )
		{
			return false;
		}

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_Create, "ImageList_Create" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_Destroy, "ImageList_Destroy" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_Add, "ImageList_Add" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_LoadImageW, "ImageList_LoadImageW" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_ReplaceIcon, "ImageList_ReplaceIcon" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_GetIcon, "ImageList_GetIcon" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_InitCommonControlsEx, "InitCommonControlsEx" ) )

		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_MakeDragList, "MakeDragList" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_DrawInsert, "DrawInsert" ) )
		VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_LBItemFromPt, "LBItemFromPt" ) )

		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_BeginDrag, "ImageList_BeginDrag" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_DragEnter, "ImageList_DragEnter" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_DragMove, "ImageList_DragMove" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_DragShowNolock, "ImageList_DragShowNolock" ) )
		//VALIDATE_FUNCTION_POINTER( SetFunctionPointer( hModule_comctl32, ( void ** )&_ImageList_EndDrag, "ImageList_EndDrag" ) )

		comctl32_state = COMCTL32_STATE_RUNNING;

		return true;
	}

	bool UnInitializeComCtl32()
	{
		if ( comctl32_state != COMCTL32_STATE_SHUTDOWN )
		{
			comctl32_state = COMCTL32_STATE_SHUTDOWN;

			return ( FreeLibrary( hModule_comctl32 ) == FALSE ? false : true );
		}

		return true;
	}

#endif
