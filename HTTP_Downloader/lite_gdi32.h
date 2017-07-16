/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2017 Eric Kutcher

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

#ifndef _LITE_GDI32_H
#define _LITE_GDI32_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//#define GDI32_USE_STATIC_LIB

#ifdef GDI32_USE_STATIC_LIB

	//__pragma( comment( lib, "gdi32.lib" ) )

	#define _BitBlt						BitBlt
	#define _CreateCompatibleBitmap		CreateCompatibleBitmap
	#define _CreateCompatibleDC			CreateCompatibleDC
	#define _CreateFontIndirectW		CreateFontIndirectW
	#define _CreatePen					CreatePen
	#define _CreateSolidBrush			CreateSolidBrush
	#define _DeleteDC					DeleteDC
	#define _DeleteObject				DeleteObject
	//#define _ExcludeClipRect			ExcludeClipRect
	//#define _GdiAlphaBlend				GdiAlphaBlend
	//#define _GdiGradientFill			GdiGradientFill
	//#define _GetDeviceCaps				GetDeviceCaps
	#define _GetObjectW					GetObjectW
	#define _GetStockObject				GetStockObject
	//#define _GetTextExtentPoint32W	GetTextExtentPoint32W
	#define _GetTextMetricsW			GetTextMetricsW
	#define _PatBlt						PatBlt
	#define _Rectangle					Rectangle
	#define _SelectObject				SelectObject
	//#define _SetBkColor					SetBkColor
	#define _SetBkMode					SetBkMode
	#define _SetTextColor				SetTextColor

#else

	#define GDI32_STATE_SHUTDOWN	0
	#define GDI32_STATE_RUNNING		1

	typedef BOOL ( WINAPI *pBitBlt )( HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop );
	typedef HBITMAP ( WINAPI *pCreateCompatibleBitmap )( HDC hdc, int nWidth, int nHeight );
	typedef HDC ( WINAPI *pCreateCompatibleDC )( HDC hdc );
	typedef HFONT ( WINAPI *pCreateFontIndirectW )( const LOGFONT *lplf );
	typedef HPEN ( WINAPI *pCreatePen )( int fnPenStyle, int nWidth, COLORREF crColor );
	typedef HBRUSH ( WINAPI *pCreateSolidBrush )( COLORREF crColor );
	typedef BOOL ( WINAPI *pDeleteDC )( HDC hdc );
	typedef BOOL ( WINAPI *pDeleteObject )( HGDIOBJ hObject );
	//typedef int ( WINAPI *pExcludeClipRect )( HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect );
	//typedef BOOL ( WINAPI *pGdiAlphaBlend )( HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn );
	//typedef BOOL ( WINAPI *pGdiGradientFill )( HDC hdc, PTRIVERTEX pVertex, ULONG dwNumVertex, PVOID pMesh, ULONG dwNumMesh, ULONG dwMode );
	//typedef int ( WINAPI *pGetDeviceCaps )( HDC hdc, int nIndex );
	typedef int ( WINAPI *pGetObjectW )( HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject );
	typedef HGDIOBJ ( WINAPI *pGetStockObject )( int fnObject );
	//typedef BOOL ( WINAPI *pGetTextExtentPoint32W )( HDC hdc, LPCTSTR lpString, int c, LPSIZE lpSize );
	typedef BOOL ( WINAPI *pGetTextMetricsW )( HDC hdc, LPTEXTMETRIC lptm );
	typedef BOOL ( WINAPI *pPatBlt )( HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, DWORD dwRop );
	typedef BOOL ( WINAPI *pRectangle )( HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect );
	typedef HGDIOBJ ( WINAPI *pSelectObject )( HDC hdc, HGDIOBJ hgdiobj );
	//typedef COLORREF ( WINAPI *pSetBkColor )( HDC hdc, COLORREF crColor );
	typedef int ( WINAPI *pSetBkMode )( HDC hdc, int iBkMode );
	typedef COLORREF ( WINAPI *pSetTextColor )( HDC hdc, COLORREF crColor );

	extern pBitBlt						_BitBlt;
	extern pCreateCompatibleBitmap		_CreateCompatibleBitmap;
	extern pCreateCompatibleDC			_CreateCompatibleDC;
	extern pCreateFontIndirectW			_CreateFontIndirectW;
	extern pCreatePen					_CreatePen;
	extern pCreateSolidBrush			_CreateSolidBrush;
	extern pDeleteDC					_DeleteDC;
	extern pDeleteObject				_DeleteObject;
	//extern pExcludeClipRect				_ExcludeClipRect;
	//extern pGdiAlphaBlend				_GdiAlphaBlend;
	//extern pGdiGradientFill				_GdiGradientFill;
	//extern pGetDeviceCaps				_GetDeviceCaps;
	extern pGetObjectW					_GetObjectW;
	extern pGetStockObject				_GetStockObject;
	//extern pGetTextExtentPoint32W		_GetTextExtentPoint32W;
	extern pGetTextMetricsW				_GetTextMetricsW;
	//extern pPatBlt						_PatBlt;
	extern pRectangle					_Rectangle;
	extern pSelectObject				_SelectObject;
	//extern pSetBkColor					_SetBkColor;
	extern pSetBkMode					_SetBkMode;
	extern pSetTextColor				_SetTextColor;

	extern unsigned char gdi32_state;

	bool InitializeGDI32();
	bool UnInitializeGDI32();

#endif

#endif











