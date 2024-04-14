/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2024 Eric Kutcher

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

#include "globals.h"
#include "lite_comctl32.h"
#include "utilities.h"
#include "string_tables.h"

#include "dark_mode.h"

#define EDIT_SPEED_LIMIT		1000
#define BTN_SPEED_LIMIT_SET		1001
#define BTN_SPEED_LIMIT_CANCEL	1002

HWND g_hWnd_download_speed_limit = NULL;

HWND g_hWnd_speed_limit = NULL;
HWND g_hWnd_speed_limit_set = NULL;

wchar_t limit_tooltip_text[ 32 ];
HWND g_hWnd_limit_tooltip = NULL;

LRESULT CALLBACK DownloadSpeedLimitWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_speed_limit = _CreateWindowW( WC_STATIC, ST_V_Global_download_speed_limit_bytes_, WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_speed_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, 28, rc.right - 20, 23, hWnd, ( HMENU )EDIT_SPEED_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_speed_limit, EM_LIMITTEXT, 20, 0 );

			g_hWnd_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			limit_tooltip_text[ 0 ] = 0;

			TOOLINFO ti;
			_memzero( &ti, sizeof( TOOLINFO ) );
			ti.cbSize = sizeof( TOOLINFO );
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = g_hWnd_speed_limit;
			ti.lpszText = limit_tooltip_text;

			_GetClientRect( hWnd, &ti.rect );
			_SendMessageW( g_hWnd_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&ti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_download_speed_limit );
			_SendMessageA( g_hWnd_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageA( g_hWnd_speed_limit, EM_SETSEL, 0, -1 );


			g_hWnd_speed_limit_set = _CreateWindowW( WC_BUTTON, ST_V_Set, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, rc.right - 175, 61, 80, 23, hWnd, ( HMENU )BTN_SPEED_LIMIT_SET, NULL, NULL );
			HWND hWnd_speed_limit_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 90, 61, 80, 23, hWnd, ( HMENU )BTN_SPEED_LIMIT_CANCEL, NULL, NULL );

			_SendMessageW( hWnd_static_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_speed_limit_set, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_speed_limit_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SetFocus( g_hWnd_speed_limit );

			// Accounts for differing title bar heights.
			CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
			int height = ( cs->cy - ( rc.bottom - rc.top ) ) + 94;	// Bottom of last window object + 10.

			HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - 330 ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), 330, height, 0 );

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				_EnumChildWindows( hWnd, EnumChildProc, NULL );
				_EnumThreadWindows( GetCurrentThreadId(), EnumTLWProc, NULL );
			}
#endif

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_SPEED_LIMIT_SET:
				{
					char value[ 21 ];
					_SendMessageA( g_hWnd_speed_limit, WM_GETTEXT, 21, ( LPARAM )value );
					cfg_download_speed_limit = strtoull( value );

					wchar_t status_bar_buf[ 128 ];
					unsigned char buf_length;

					buf_length = ( unsigned char )( ST_L_Global_download_speed_limit_ > 102 ? 102 : ST_L_Global_download_speed_limit_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
					_wmemcpy_s( status_bar_buf, 128, ST_V_Global_download_speed_limit_, buf_length );
					status_bar_buf[ buf_length++ ] = ' ';

					if ( cfg_download_speed_limit == 0 )
					{
						_wmemcpy_s( status_bar_buf + buf_length, 128 - buf_length, ST_V_Unlimited, ST_L_Unlimited + 1 );
					}
					else
					{
						// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
						unsigned int length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_speed_limit, cfg_download_speed_limit ) + buf_length;
						status_bar_buf[ length++ ] = L'/';
						status_bar_buf[ length++ ] = L's';
						status_bar_buf[ length ] = 0;
					}

					_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 3, ( LPARAM )status_bar_buf );
					_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 3, 0 ), ( LPARAM )status_bar_buf );

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case EDIT_SPEED_LIMIT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 21 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 21, ( LPARAM )value );
						unsigned long long num = strtoull( value );

						if ( num == 0xFFFFFFFFFFFFFFFF )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"18446744073709551615" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num > 0 )
						{
							unsigned int length = FormatSizes( limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
							limit_tooltip_text[ length++ ] = L'/';
							limit_tooltip_text[ length++ ] = L's';
							limit_tooltip_text[ length ] = 0;
						}
						else
						{
							_wmemcpy_s( limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_speed_limit;
						ti.lpszText = limit_tooltip_text;
						_SendMessageW( g_hWnd_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );

						if ( num != cfg_download_speed_limit )
						{
							_EnableWindow( g_hWnd_speed_limit_set, TRUE );
						}
					}
				}
				break;

				case BTN_SPEED_LIMIT_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

			return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			g_hWnd_download_speed_limit = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	//return TRUE;
}
