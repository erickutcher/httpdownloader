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

#include "options.h"
#include "utilities.h"

#define EDIT_MAX_DOWNLOADS				1000
#define EDIT_RETRY_DOWNLOADS_COUNT		1001
#define EDIT_DEFAULT_DOWNLOAD_PARTS		1002
#define EDIT_RETRY_PARTS_COUNT			1003
#define EDIT_TIMEOUT					1004
#define EDIT_MAX_REDIRECTS				1005
#define EDIT_DEFAULT_SPEED_LIMIT		1006
#define CB_DEFAULT_SSL_VERSION			1007
#define BTN_REALLOCATE_PARTS			1008

// Connection Tab
HWND g_hWnd_max_downloads = NULL;
HWND g_hWnd_ud_max_downloads = NULL;

HWND g_hWnd_retry_downloads_count = NULL;
HWND g_hWnd_ud_retry_downloads_count = NULL;
HWND g_hWnd_retry_parts_count = NULL;
HWND g_hWnd_ud_retry_parts_count = NULL;

HWND g_hWnd_timeout = NULL;
HWND g_hWnd_ud_timeout = NULL;

HWND g_hWnd_max_redirects = NULL;
HWND g_hWnd_ud_max_redirects = NULL;

HWND g_hWnd_default_speed_limit = NULL;

HWND g_hWnd_default_ssl_version = NULL;
HWND g_hWnd_default_download_parts = NULL;
HWND g_hWnd_default_ud_download_parts = NULL;

HWND g_hWnd_chk_reallocate_parts = NULL;

wchar_t default_limit_tooltip_text[ 32 ];
HWND g_hWnd_default_limit_tooltip = NULL;

LRESULT CALLBACK ConnectionTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_max_downloads = _CreateWindowW( WC_STATIC, ST_V_Active_download_limit_, WS_CHILD | WS_VISIBLE, 0, 4, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_downloads = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 0, 100, 23, hWnd, ( HMENU )EDIT_MAX_DOWNLOADS, NULL, NULL );

			g_hWnd_ud_max_downloads = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_downloads, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_max_downloads, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_downloads, 0 );
			_SendMessageW( g_hWnd_ud_max_downloads, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_max_downloads, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( g_hWnd_ud_max_downloads, UDM_SETPOS, 0, cfg_max_downloads );

			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_max_downloads, &rc_spinner );
			int spinner_width = rc_spinner.right - rc_spinner.left;

			_SetWindowPos( g_hWnd_max_downloads, HWND_TOP, rc.right - ( 100 + spinner_width ), 0, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_max_downloads, HWND_TOP, rc.right - spinner_width, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE );


			HWND hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_V_Default_download_parts_, WS_CHILD | WS_VISIBLE, 0, 32, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 28, 100, 23, hWnd, ( HMENU )EDIT_DEFAULT_DOWNLOAD_PARTS, NULL, NULL );

			g_hWnd_default_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_default_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_default_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_default_download_parts, 0 );
			_SendMessageW( g_hWnd_default_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_default_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_default_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );
			_SetWindowPos( g_hWnd_default_download_parts, HWND_TOP, rc.right - ( 100 + spinner_width ), 28, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_default_ud_download_parts, HWND_TOP, rc.right - spinner_width, 28, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//


			HWND hWnd_static_retry_downloads_count = _CreateWindowW( WC_STATIC, ST_V_Retry_incomplete_downloads_, WS_CHILD | WS_VISIBLE, 0, 60, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_downloads_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 56, 100, 23, hWnd, ( HMENU )EDIT_RETRY_DOWNLOADS_COUNT, NULL, NULL );

			g_hWnd_ud_retry_downloads_count = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_downloads_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_retry_downloads_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_downloads_count, 0 );
			_SendMessageW( g_hWnd_ud_retry_downloads_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_retry_downloads_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( g_hWnd_ud_retry_downloads_count, UDM_SETPOS, 0, cfg_retry_downloads_count );
			_SetWindowPos( g_hWnd_retry_downloads_count, HWND_TOP, rc.right - ( 100 + spinner_width ), 56, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_retry_downloads_count, HWND_TOP, rc.right - spinner_width, 56, 0, 0, SWP_NOZORDER | SWP_NOSIZE );


			HWND hWnd_static_retry_parts_count = _CreateWindowW( WC_STATIC, ST_V_Retry_incomplete_parts_, WS_CHILD | WS_VISIBLE, 0, 88, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_parts_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 84, 100, 23, hWnd, ( HMENU )EDIT_RETRY_PARTS_COUNT, NULL, NULL );

			g_hWnd_ud_retry_parts_count = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_parts_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_retry_parts_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_parts_count, 0 );
			_SendMessageW( g_hWnd_ud_retry_parts_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_retry_parts_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( g_hWnd_ud_retry_parts_count, UDM_SETPOS, 0, cfg_retry_parts_count );
			_SetWindowPos( g_hWnd_retry_parts_count, HWND_TOP, rc.right - ( 100 + spinner_width ), 84, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_retry_parts_count, HWND_TOP, rc.right - spinner_width, 84, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//

			HWND hWnd_static_timeout = _CreateWindowW( WC_STATIC, ST_V_Timeout__seconds__, WS_CHILD | WS_VISIBLE, 0, 116, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_timeout = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 112, 100, 23, hWnd, ( HMENU )EDIT_TIMEOUT, NULL, NULL );

			g_hWnd_ud_timeout = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_timeout, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETBUDDY, ( WPARAM )g_hWnd_timeout, 0 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETRANGE32, 10, 300 );
			_SendMessageW( g_hWnd_ud_timeout, UDM_SETPOS, 0, cfg_timeout );
			if ( cfg_timeout == 0 )
			{
				_SendMessageW( g_hWnd_timeout, WM_SETTEXT, 0, ( LPARAM )L"0" );
			}
			_SetWindowPos( g_hWnd_timeout, HWND_TOP, rc.right - ( 100 + spinner_width ), 112, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_timeout, HWND_TOP, rc.right - spinner_width, 112, 0, 0, SWP_NOZORDER | SWP_NOSIZE );


			HWND hWnd_static_max_redirects = _CreateWindowW( WC_STATIC, ST_V_Maximum_redirects_, WS_CHILD | WS_VISIBLE, 0, 144, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_redirects = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 140, 100, 23, hWnd, ( HMENU )EDIT_MAX_REDIRECTS, NULL, NULL );


			g_hWnd_ud_max_redirects = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_redirects, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_max_redirects, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_redirects, 0 );
			_SendMessageW( g_hWnd_ud_max_redirects, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_max_redirects, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( g_hWnd_ud_max_redirects, UDM_SETPOS, 0, cfg_max_redirects );
			_SetWindowPos( g_hWnd_max_redirects, HWND_TOP, rc.right - ( 100 + spinner_width ), 140, 100, 23, SWP_NOZORDER );
			_SetWindowPos( g_hWnd_ud_max_redirects, HWND_TOP, rc.right - spinner_width, 140, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

			//

			HWND hWnd_static_default_speed_limit = _CreateWindowW( WC_STATIC, ST_V_Default_download_speed_limit_, WS_CHILD | WS_VISIBLE, 0, 172, 320, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_default_speed_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 100, 168, 100, 23, hWnd, ( HMENU )EDIT_DEFAULT_SPEED_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_default_speed_limit, EM_LIMITTEXT, 20, 0 );

			g_hWnd_default_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			default_limit_tooltip_text[ 0 ] = 0;

			TOOLINFO ti;
			_memzero( &ti, sizeof( TOOLINFO ) );
			ti.cbSize = sizeof( TOOLINFO );
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = g_hWnd_default_speed_limit;
			ti.lpszText = default_limit_tooltip_text;

			_GetClientRect( hWnd, &ti.rect );
			_SendMessageW( g_hWnd_default_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&ti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_default_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

			//

			HWND hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_V_Default_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 200, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, rc.right - 100, 196, 100, 23, hWnd, ( HMENU )CB_DEFAULT_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_default_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			//

			g_hWnd_chk_reallocate_parts = _CreateWindowW( WC_BUTTON, ST_V_Reallocate_parts_to_maximize_connections, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 228, rc.right, 20, hWnd, ( HMENU )BTN_REALLOCATE_PARTS, NULL, NULL );

			_SendMessageW( g_hWnd_chk_reallocate_parts, BM_SETCHECK, ( cfg_reallocate_parts ? BST_CHECKED : BST_UNCHECKED ), 0 );
			
			//

			_SendMessageW( hWnd_static_max_downloads, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_max_downloads, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_retry_downloads_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_retry_downloads_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_retry_parts_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_retry_parts_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_default_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_default_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_timeout, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_timeout, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_max_redirects, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_max_redirects, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_default_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_default_speed_limit, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_reallocate_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case EDIT_MAX_DOWNLOADS:
				case EDIT_MAX_REDIRECTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						/*else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}*/

						if ( ( LOWORD( wParam ) == EDIT_MAX_DOWNLOADS && num != cfg_max_downloads ) ||
							 ( LOWORD( wParam ) == EDIT_MAX_REDIRECTS && num != cfg_max_redirects ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_RETRY_DOWNLOADS_COUNT:
				case EDIT_RETRY_PARTS_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( ( LOWORD( wParam ) == EDIT_RETRY_DOWNLOADS_COUNT && num != cfg_retry_downloads_count ) ||
							 ( LOWORD( wParam ) == EDIT_RETRY_PARTS_COUNT && num != cfg_retry_parts_count ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_TIMEOUT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 300 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"300" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 0 && num < 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case CB_DEFAULT_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_options_apply, TRUE );
					}
				}
				break;

				case EDIT_DEFAULT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_default_download_parts )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_DEFAULT_SPEED_LIMIT:
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
							unsigned int length = FormatSizes( default_limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
							default_limit_tooltip_text[ length++ ] = L'/';
							default_limit_tooltip_text[ length++ ] = L's';
							default_limit_tooltip_text[ length ] = 0;
						}
						else
						{
							_wmemcpy_s( default_limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_default_speed_limit;
						ti.lpszText = default_limit_tooltip_text;
						_SendMessageW( g_hWnd_default_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );

						if ( num != cfg_default_speed_limit )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case BTN_REALLOCATE_PARTS:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;
			}

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
