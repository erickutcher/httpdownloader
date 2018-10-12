/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#ifndef _LITE_COMCTL32_H
#define _LITE_COMCTL32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

//#define COMCTL32_USE_STATIC_LIB

#ifdef COMCTL32_USE_STATIC_LIB

	//__pragma( comment( lib, "comctl32.lib" ) )

	//#define _ImageList_Create		ImageList_Create
	#define _ImageList_Destroy		ImageList_Destroy
	//#define _ImageList_Add			ImageList_Add
	#define _ImageList_LoadImageW	ImageList_LoadImageW
	//#define _ImageList_ReplaceIcon	ImageList_ReplaceIcon

#else

	#define COMCTL32_STATE_SHUTDOWN		0
	#define COMCTL32_STATE_RUNNING		1

	//typedef HIMAGELIST ( WINAPI *pImageList_Create )( int cx, int cy, UINT flags, int cInitial, int cGrow );
	typedef BOOL ( WINAPI *pImageList_Destroy )( HIMAGELIST himl );
	//typedef int ( WINAPI *pImageList_Add )( HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask );
	typedef HIMAGELIST ( WINAPI *pImageList_LoadImageW )( HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags );
	//typedef int ( WINAPI *pImageList_ReplaceIcon )( HIMAGELIST himl, int i, HICON hicon );

	//extern pImageList_Create		_ImageList_Create;
	extern pImageList_Destroy		_ImageList_Destroy;
	//extern pImageList_Add			_ImageList_Add;
	extern pImageList_LoadImageW	_ImageList_LoadImageW;
	//extern pImageList_ReplaceIcon	_ImageList_ReplaceIcon;

	extern unsigned char comctl32_state;

	bool InitializeComCtl32();
	bool UnInitializeComCtl32();

#endif

	//#define _ImageList_AddIcon( himl, hicon ) _ImageList_ReplaceIcon( himl, -1, hicon )

#endif
