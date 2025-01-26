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

#ifndef _LITE_COMCTL32_H
#define _LITE_COMCTL32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

//#define COMCTL32_USE_STATIC_LIB

#ifdef COMCTL32_USE_STATIC_LIB

	//__pragma( comment( lib, "comctl32.lib" ) )

	#define _ImageList_Create		ImageList_Create
	#define _ImageList_Destroy		ImageList_Destroy
	//#define _ImageList_Add			ImageList_Add
	#define _ImageList_AddMasked	ImageList_AddMasked
	#define _ImageList_LoadImageW	ImageList_LoadImageW
	//#define _ImageList_ReplaceIcon	ImageList_ReplaceIcon
	//#define _ImageList_GetIcon			ImageList_GetIcon

	//#define _InitCommonControlsEx	InitCommonControlsEx

	#define _MakeDragList	MakeDragList
	//#define _DrawInsert		DrawInsert
	#define _LBItemFromPt	LBItemFromPt

	//#define _ImageList_BeginDrag		ImageList_BeginDrag
	//#define _ImageList_DragEnter		ImageList_DragEnter
	//#define _ImageList_DragMove			ImageList_DragMove
	//#define _ImageList_DragShowNolock	ImageList_DragShowNolock
	//#define _ImageList_EndDrag			ImageList_EndDrag

#else

	#define COMCTL32_STATE_SHUTDOWN		0
	#define COMCTL32_STATE_RUNNING		1

	typedef HIMAGELIST ( WINAPI *pImageList_Create )( int cx, int cy, UINT flags, int cInitial, int cGrow );
	typedef BOOL ( WINAPI *pImageList_Destroy )( HIMAGELIST himl );
	//typedef int ( WINAPI *pImageList_Add )( HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask );
	typedef int ( WINAPI *pImageList_AddMasked )( HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask );
	typedef HIMAGELIST ( WINAPI *pImageList_LoadImageW )( HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags );
	//typedef int ( WINAPI *pImageList_ReplaceIcon )( HIMAGELIST himl, int i, HICON hicon );
	//typedef HICON ( WINAPI *pImageList_GetIcon )( HIMAGELIST himl, int i, UINT flags );

	//typedef BOOL ( WINAPI *pInitCommonControlsEx )( const INITCOMMONCONTROLSEX *picce );

	typedef BOOL ( WINAPI *pMakeDragList )( HWND hLB );
	//typedef void ( WINAPI *pDrawInsert )( HWND handParent, HWND hLB, int nItem );
	typedef int ( WINAPI *pLBItemFromPt )( HWND hLB, POINT pt, BOOL bAutoScroll );

	//typedef BOOL ( WINAPI *pImageList_BeginDrag )( HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot );
	//typedef BOOL ( WINAPI *pImageList_DragEnter )( HWND hwndLock, int x, int y );
	//typedef BOOL ( WINAPI *pImageList_DragMove )( int x, int y );
	//typedef BOOL ( WINAPI *pImageList_DragShowNolock )( BOOL fShow );
	//typedef void ( WINAPI *pImageList_EndDrag )();

	extern pImageList_Create			_ImageList_Create;
	extern pImageList_Destroy			_ImageList_Destroy;
	//extern pImageList_Add				_ImageList_Add;
	extern pImageList_AddMasked			_ImageList_AddMasked;
	extern pImageList_LoadImageW		_ImageList_LoadImageW;
	//extern pImageList_ReplaceIcon		_ImageList_ReplaceIcon;
	//extern pImageList_GetIcon			_ImageList_GetIcon;

	//extern pInitCommonControlsEx		_InitCommonControlsEx;

	extern pMakeDragList				_MakeDragList;
	//extern pDrawInsert				_DrawInsert;
	extern pLBItemFromPt				_LBItemFromPt;

	//extern pImageList_BeginDrag			_ImageList_BeginDrag;
	//extern pImageList_DragEnter			_ImageList_DragEnter;
	//extern pImageList_DragMove			_ImageList_DragMove;
	//extern pImageList_DragShowNolock	_ImageList_DragShowNolock;
	//extern pImageList_EndDrag			_ImageList_EndDrag;

	extern unsigned char comctl32_state;

	bool InitializeComCtl32();
	bool UnInitializeComCtl32();

#endif

	//#define _ImageList_AddIcon( himl, hicon ) _ImageList_ReplaceIcon( himl, -1, hicon )

#endif
