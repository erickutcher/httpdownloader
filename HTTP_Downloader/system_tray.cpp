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
HICON g_tray_icon = NULL;

int g_last_percent = 0;
COLORREF last_border_color = 0;
COLORREF last_progress_color = 0;

bool is_system_tray_initialized = false;

wchar_t *g_notification_title = NULL;

void InitializeSystemTray( HWND hWnd )
{
	if ( !is_system_tray_initialized )
	{
		_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
		g_nid.cbSize = NOTIFYICONDATA_V2_SIZE;	// 5.0 (Windows 2000) and newer.
		g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		g_nid.hWnd = hWnd;
		g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
		g_nid.uID = 1000;
		g_nid.hIcon = g_default_tray_icon;
		g_nid.dwInfoFlags = NIIF_INFO;
		_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ), PROGRAM_CAPTION, 16 );
		g_nid.szTip[ 15 ] = 0;	// Sanity.

		is_system_tray_initialized = true;
	}

	_Shell_NotifyIconW( NIM_ADD, &g_nid );
}

void InitializeIconValues( HWND hWnd )
{
	UINT current_dpi_main = ( UINT )_SendMessageW( hWnd, WM_GET_DPI, 0, 0 );

	UninitializeIconValues();

	HICON hIcon;

	_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\tray.ico\0", 10 );
	if ( GetFileAttributesW( g_program_directory ) != INVALID_FILE_ATTRIBUTES )
	{
		hIcon = ( HICON )_LoadImageW( NULL, g_program_directory, IMAGE_ICON, 0, 0, LR_LOADFROMFILE );
	}
	else
	{
		hIcon = ( HICON )_LoadImageW( GetModuleHandleW( NULL ), MAKEINTRESOURCE( IDI_ICON_TRAY ), IMAGE_ICON, 0, 0, LR_SHARED );
	}

	_GetIconInfo( hIcon, &g_icon_info );

	int res_height = _SCALE_( 16, dpi_main );
	int res_width = _SCALE_( 16, dpi_main );

	//

	HDC hDC = _GetDC( hWnd );

	HDC hdcMem_bmp = _CreateCompatibleDC( hDC );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem_bmp, g_icon_info.hbmColor );
	_DeleteObject( ohbm );

	HBITMAP hBmp_scaled = _CreateCompatibleBitmap( hDC, res_width, res_height );

	HDC hdcMem = _CreateCompatibleDC( hDC );
	ohbm = ( HBITMAP )_SelectObject( hdcMem, hBmp_scaled );
	_DeleteObject( ohbm );

	_SetStretchBltMode( hdcMem, COLORONCOLOR );
	_StretchBlt( hdcMem, 0, 0, res_width, res_height, hdcMem_bmp, 0, 0, g_icon_info.xHotspot * 2, g_icon_info.yHotspot * 2, SRCCOPY );

	//

	g_icon_hdcmem = hdcMem;

	_DeleteObject( g_icon_info.hbmColor );
	g_icon_info.hbmColor = hBmp_scaled;

	//

	// The last object in hdcMem_bmp was g_icon_info.hbmColor before we deleted it above.
	// It will already have been deleted.
	ohbm = ( HBITMAP )_SelectObject( hdcMem_bmp, g_icon_info.hbmMask );

	hBmp_scaled = _CreateCompatibleBitmap( hDC, res_width, res_height );

	hdcMem = _CreateCompatibleDC( hDC );
	ohbm = ( HBITMAP )_SelectObject( hdcMem, hBmp_scaled );
	_DeleteObject( ohbm );

	_SetStretchBltMode( hdcMem, COLORONCOLOR );
	_StretchBlt( hdcMem, 0, 0, res_width, res_height, hdcMem_bmp, 0, 0, g_icon_info.xHotspot * 2, g_icon_info.yHotspot * 2, SRCCOPY );

	//

	_DeleteDC( hdcMem );

	_DeleteObject( g_icon_info.hbmMask );
	g_icon_info.hbmMask = hBmp_scaled;

	//

	g_icon_info.xHotspot = g_icon_info.yHotspot = res_width / 2;

	_DeleteDC( hdcMem_bmp );
	_ReleaseDC( hWnd, hDC );
}

void UninitializeIconValues()
{
	if ( g_tray_icon != NULL )
	{
		_DestroyIcon( g_tray_icon );
		g_tray_icon = NULL;
	}

	if ( g_icon_hdcmem != NULL )
	{
		_DeleteDC( g_icon_hdcmem );
		g_icon_hdcmem = NULL;
	}

	if ( g_icon_info.hbmMask != NULL )
	{
		_DeleteObject( g_icon_info.hbmMask );
		g_icon_info.hbmMask = NULL;
	}

	if ( g_icon_info.hbmColor != NULL )
	{
		_DeleteObject( g_icon_info.hbmColor );
		g_icon_info.hbmColor = NULL;
	}
}

HICON CreateSystemTrayIcon( unsigned long long start, unsigned long long end, COLORREF border_color, COLORREF progress_color )
{
	UINT current_dpi_main = ( UINT )_SendMessageW( g_hWnd_main, WM_GET_DPI, 0, 0 );

	int i_percentage;

	if ( end > 0 )
	{
	#ifdef _WIN64
		i_percentage = ( int )( 14.0 * ( ( double )start / ( double )end ) );
	#else
		double f_percentage = 14.0 * ( ( double )start / ( double )end );
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
	#endif

		i_percentage = _SCALE_( i_percentage, dpi_main );

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

	RECT icon_rc;
	icon_rc.top = _SCALE_( 12, dpi_main );
	icon_rc.left = 0;
	icon_rc.right = _SCALE_( 16, dpi_main );
	icon_rc.bottom = _SCALE_( 16, dpi_main );

	HBRUSH color = _CreateSolidBrush( border_color );
	_FillRect( g_icon_hdcmem, &icon_rc, color );
	_DeleteObject( color );

	icon_rc.top = icon_rc.top + 1;//13;
	icon_rc.left = 1;
	icon_rc.right = 1 + g_last_percent;
	icon_rc.bottom = icon_rc.bottom - 1;//15;

	color = _CreateSolidBrush( progress_color );
	_FillRect( g_icon_hdcmem, &icon_rc, color );
	_DeleteObject( color );

	if ( g_tray_icon != NULL )
	{
		_DestroyIcon( g_tray_icon );
	}

	g_tray_icon = CreateIconIndirect( &g_icon_info );

	return g_tray_icon;
}

void SetNotificationTitle( wchar_t *title, unsigned short title_length )
{
	if ( title != g_notification_title )
	{
		unsigned char info_size = ( unsigned char )( title_length > ( ( sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ) ) - 1 ) ? ( ( sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ) ) - 1 ) : title_length );
		_wmemcpy_s( g_nid.szInfoTitle, sizeof( g_nid.szInfoTitle ) / sizeof( g_nid.szInfoTitle[ 0 ] ), title, info_size );
		g_nid.szInfoTitle[ info_size ] = 0;	// Sanity.

		g_notification_title = title;
	}
}
