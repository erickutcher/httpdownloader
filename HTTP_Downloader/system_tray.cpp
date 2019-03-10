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

#include "system_tray.h"
#include "globals.h"
#include "resource.h"
#include "lite_ntdll.h"
#include "lite_user32.h"
#include "lite_gdi32.h"

#include "string_tables.h"

NOTIFYICONDATA g_nid;	// Tray icon information.
HICON g_default_tray_icon = NULL;

ICONINFO g_icon_info;
HDC g_icon_hdcmem = NULL;
HBITMAP g_icon_ohbm = NULL;
HICON g_tray_icon = NULL;

int g_last_percent = 0;
COLORREF last_border_color = 0;
COLORREF last_progress_color = 0;

bool is_icon_initialized = false;

void InitializeSystemTray( HWND hWnd )
{
	_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
	g_nid.cbSize = NOTIFYICONDATA_V2_SIZE;	// 5.0 (Windows 2000) and newer.
	g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_nid.hWnd = hWnd;
	g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
	g_nid.uID = 1000;
	g_nid.hIcon = g_default_tray_icon;
	g_nid.dwInfoFlags = NIIF_INFO;
	unsigned char info_size = ( unsigned char )( ST_L_Downloads_Have_Finished > ( ( sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ) ) - 1 ) ? ( ( sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ) ) - 1 ) : ST_L_Downloads_Have_Finished );
	_wmemcpy_s( g_nid.szInfoTitle, sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ), ST_V_Downloads_Have_Finished, info_size );
	g_nid.szInfoTitle[ info_size ] = 0;	// Sanity.
	_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ), PROGRAM_CAPTION, 16 );
	g_nid.szTip[ 15 ] = 0;	// Sanity.
	_Shell_NotifyIconW( NIM_ADD, &g_nid );
}

void InitializeIconValues( HWND hWnd )
{
	if ( !is_icon_initialized )
	{
		_GetIconInfo( ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON_TRAY ), IMAGE_ICON, 16, 16, LR_SHARED ), &g_icon_info );

		HDC hDC = _GetDC( hWnd );
		g_icon_hdcmem = _CreateCompatibleDC( hDC );

		g_icon_ohbm = ( HBITMAP )_SelectObject( g_icon_hdcmem, g_icon_info.hbmColor );

		_ReleaseDC( hWnd, hDC );

		is_icon_initialized = true;
	}
}

void UninitializeIconValues()
{
	if ( is_icon_initialized )
	{
		if ( g_tray_icon != NULL )
		{
			_DestroyIcon( g_tray_icon );
		}

		_DeleteObject( g_icon_ohbm );

		_DeleteDC( g_icon_hdcmem );

		_DeleteObject( g_icon_info.hbmMask );
		_DeleteObject( g_icon_info.hbmColor );

		is_icon_initialized = false;
	}
}

HICON CreateSystemTrayIcon( unsigned long long start, unsigned long long end, COLORREF border_color, COLORREF progress_color )
{
	int i_percentage = 0;

	if ( end > 0 )
	{
	#ifdef _WIN64
		i_percentage = ( int )( 14.0f * ( ( float )start / ( float )end ) );
	#else
		float f_percentage = 14.0f * ( ( float )start / ( float )end );
		i_percentage = 0;
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
	#endif

		if ( i_percentage == g_last_percent &&
			 border_color == last_border_color &&
			 progress_color == last_progress_color &&
			 g_tray_icon != NULL )
		{
			return g_tray_icon;
		}

		g_last_percent = i_percentage;
	}

	last_border_color = border_color;
	last_progress_color = progress_color;

	if ( g_tray_icon != NULL )
	{
		_DestroyIcon( g_tray_icon );
	}

	RECT icon_rc;
	icon_rc.top = 12;
	icon_rc.left = 0;
	icon_rc.right = 16;
	icon_rc.bottom = 16;

	HBRUSH color = _CreateSolidBrush( border_color );
	_FillRect( g_icon_hdcmem, &icon_rc, color );
	_DeleteObject( color );

	icon_rc.top = 13;
	icon_rc.left = 1;
	icon_rc.right = 1 + g_last_percent;
	icon_rc.bottom = 15;

	color = _CreateSolidBrush( progress_color );
	_FillRect( g_icon_hdcmem, &icon_rc, color );
	_DeleteObject( color );

	g_tray_icon = CreateIconIndirect( &g_icon_info );

	return g_tray_icon;
}
