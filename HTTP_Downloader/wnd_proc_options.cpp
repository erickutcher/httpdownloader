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

#include "globals.h"

#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_normaliz.h"

#include "file_operations.h"

#include "utilities.h"

#include "string_tables.h"

#define BTN_OK					1000
#define BTN_CANCEL				1001
#define BTN_APPLY				1002

#define BTN_TRAY_ICON			1003
#define BTN_MINIMIZE_TO_TRAY	1004
#define BTN_CLOSE_TO_TRAY		1005

#define BTN_ALWAYS_ON_TOP		1006

#define BTN_DOWNLOAD_HISTORY	1007

#define BTN_QUICK_ALLOCATION	1008

#define EDIT_MAX_DOWNLOADS		1009

#define EDIT_RETRY_DOWNLOADS_COUNT	1010
#define EDIT_RETRY_PARTS_COUNT	1011

#define EDIT_TIMEOUT			1012

#define EDIT_THREAD_COUNT		1013

#define CB_DEFAULT_SSL_VERSION	1014

#define EDIT_DEFAULT_DOWNLOAD_PARTS	1015
#define BTN_DEFAULT_DOWNLOAD_DIRECTORY	1016

#define BTN_PROXY				1017

#define BTN_TYPE_HOST			1018
#define BTN_TYPE_IP_ADDRESS		1019
#define EDIT_HOST				1020
#define EDIT_IP_ADDRESS			1021
#define EDIT_PORT				1022

#define BTN_PROXY_S				1023

#define BTN_TYPE_HOST_S			1024
#define BTN_TYPE_IP_ADDRESS_S	1025
#define EDIT_HOST_S				1026
#define EDIT_IP_ADDRESS_S		1027
#define EDIT_PORT_S				1028

#define EDIT_AUTH_USERNAME		1029
#define EDIT_AUTH_PASSWORD		1030
#define EDIT_AUTH_USERNAME_S	1031
#define EDIT_AUTH_PASSWORD_S	1032



HWND g_hWnd_options = NULL;


bool options_state_changed = false;

wchar_t *t_default_download_directory = NULL;

HFONT hFont_copy = NULL;


// Options Window
HWND g_hWnd_options_tab = NULL;
HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_download_tab = NULL;
HWND g_hWnd_proxy_tab = NULL;

HWND g_hWnd_apply = NULL;


// Connection Tab
// HTTP proxy
HWND g_hWnd_chk_proxy = NULL;

HWND g_hWnd_ip_address = NULL;
HWND g_hWnd_hostname = NULL;
HWND g_hWnd_port = NULL;

HWND g_hWnd_static_port = NULL;
HWND g_hWnd_static_colon = NULL;

HWND g_hWnd_chk_type_hostname = NULL;
HWND g_hWnd_chk_type_ip_address = NULL;

HWND g_hWnd_static_auth_username = NULL;
HWND g_hWnd_auth_username = NULL;
HWND g_hWnd_static_auth_password = NULL;
HWND g_hWnd_auth_password = NULL;

// HTTPS proxy
HWND g_hWnd_chk_proxy_s = NULL;

HWND g_hWnd_ip_address_s = NULL;
HWND g_hWnd_hostname_s = NULL;
HWND g_hWnd_port_s = NULL;

HWND g_hWnd_static_port_s = NULL;
HWND g_hWnd_static_colon_s = NULL;

HWND g_hWnd_chk_type_hostname_s = NULL;
HWND g_hWnd_chk_type_ip_address_s = NULL;

HWND g_hWnd_static_auth_username_s = NULL;
HWND g_hWnd_auth_username_s = NULL;
HWND g_hWnd_static_auth_password_s = NULL;
HWND g_hWnd_auth_password_s = NULL;

//

HWND g_hWnd_max_downloads = NULL;

HWND g_hWnd_retry_downloads_count = NULL;
HWND g_hWnd_retry_parts_count = NULL;

HWND g_hWnd_timeout = NULL;

HWND g_hWnd_default_ssl_version = NULL;
HWND g_hWnd_default_download_parts = NULL;


// General Tab
HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize = NULL;
HWND g_hWnd_chk_close = NULL;

HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_chk_download_history = NULL;
HWND g_hWnd_chk_quick_allocation = NULL;

HWND g_hWnd_thread_count = NULL;

HWND g_hWnd_default_download_directory = NULL;
HWND g_hWnd_btn_default_download_directory = NULL;



LRESULT CALLBACK GeneralTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_tray_icon = _CreateWindowW( WC_BUTTON, ST_Enable_System_Tray_icon_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_TRAY_ICON, NULL, NULL );
			g_hWnd_chk_minimize = _CreateWindowW( WC_BUTTON, ST_Minimize_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 20, 200, 20, hWnd, ( HMENU )BTN_MINIMIZE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_close = _CreateWindowW( WC_BUTTON, ST_Close_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 40, 200, 20, hWnd, ( HMENU )BTN_CLOSE_TO_TRAY, NULL, NULL );

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 65, 200, 20, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );

			g_hWnd_chk_download_history = _CreateWindowW( WC_BUTTON, ST_Enable_download_history, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 90, 200, 20, hWnd, ( HMENU )BTN_DOWNLOAD_HISTORY, NULL, NULL );

			g_hWnd_chk_quick_allocation = _CreateWindowW( WC_BUTTON, ST_Enable_quick_file_allocation, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 115, 325, 20, hWnd, ( HMENU )BTN_QUICK_ALLOCATION, NULL, NULL );
			
			HWND hWnd_static_default_download_directory = _CreateWindowW( WC_STATIC, ST_Default_download_directory_, WS_CHILD | WS_VISIBLE, 0, 140, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 155, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_default_download_directory = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 155, 35, 20, hWnd, ( HMENU )BTN_DEFAULT_DOWNLOAD_DIRECTORY, NULL, NULL );

			HWND hWnd_static_thread_count = _CreateWindowW( WC_STATIC, ST_Thread_pool_count_, WS_CHILD | WS_VISIBLE, 0, 180, 130, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_thread_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 195, 85, 20, hWnd, ( HMENU )EDIT_THREAD_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_thread_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 194, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_thread_count, EM_LIMITTEXT, 10, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_thread_count, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETRANGE32, 1, g_max_threads );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETPOS, 0, cfg_thread_count );



			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_minimize, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_close, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_download_history, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_quick_allocation, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_thread_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_thread_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_TRAY_ICON:
				{
					if ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_minimize, TRUE );
						_EnableWindow( g_hWnd_chk_close, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_minimize, FALSE );
						_EnableWindow( g_hWnd_chk_close, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_CLOSE_TO_TRAY:
				case BTN_MINIMIZE_TO_TRAY:
				case BTN_ALWAYS_ON_TOP:
				case BTN_DOWNLOAD_HISTORY:
				case BTN_QUICK_ALLOCATION:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_THREAD_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > g_max_threads )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							__snprintf( value, 11, "%lu", g_max_threads );
							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_thread_count )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_DEFAULT_DOWNLOAD_DIRECTORY:
				{
					// Open a browse for folder dialog box.
					BROWSEINFO bi;
					_memzero( &bi, sizeof( BROWSEINFO ) );
					bi.hwndOwner = hWnd;
					bi.lpszTitle = ST_Select_the_default_download_directory;
					bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_VALIDATE;

					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						// OleInitialize calls CoInitializeEx
						_OleInitialize( NULL );
					}

					LPITEMIDLIST lpiidl = _SHBrowseForFolderW( &bi );
					if ( lpiidl )
					{
						wchar_t *directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
						_memzero( directory, sizeof( wchar_t ) * MAX_PATH );

						// Get the directory path from the id list.
						_SHGetPathFromIDListW( lpiidl, ( LPTSTR )directory );

						if ( directory[ 0 ] != NULL )
						{
							if ( t_default_download_directory != NULL )
							{
								GlobalFree( t_default_download_directory );
							}

							t_default_download_directory = directory;

							_SendMessageW( g_hWnd_default_download_directory, WM_SETTEXT, 0, ( LPARAM )t_default_download_directory );

							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
						else
						{
							GlobalFree( directory );
						}

						if ( destroy )
						{
							_CoTaskMemFree( lpiidl );
						}
						else	// Warn of leak if we can't free.
						{
							_MessageBoxW( NULL, L"Item ID List was not freed.", PROGRAM_CAPTION, 0 );
						}
					}

					if ( destroy )
					{
						_OleUninitialize();
					}
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
	return TRUE;
}

LRESULT CALLBACK DownloadTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_max_downloads = _CreateWindowW( WC_STATIC, ST_Active_download_limit_, WS_CHILD | WS_VISIBLE, 0, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_downloads = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 15, 85, 20, hWnd, ( HMENU )EDIT_MAX_DOWNLOADS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_max_downloads = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 14, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_downloads, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_downloads, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETPOS, 0, cfg_max_downloads );


			HWND hWnd_static_retry_downloads_count = _CreateWindowW( WC_STATIC, ST_Retry_incomplete_downloads_, WS_CHILD | WS_VISIBLE, 150, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_downloads_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 15, 85, 20, hWnd, ( HMENU )EDIT_RETRY_DOWNLOADS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_downloads_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 235, 14, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_downloads_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_downloads_count, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETPOS, 0, cfg_retry_downloads_count );

			//

			HWND hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_Default_download_parts_, WS_CHILD | WS_VISIBLE, 0, 45, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 85, 20, hWnd, ( HMENU )EDIT_DEFAULT_DOWNLOAD_PARTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 59, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_default_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_default_download_parts, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );


			HWND hWnd_static_retry_parts_count = _CreateWindowW( WC_STATIC, ST_Retry_incomplete_parts_, WS_CHILD | WS_VISIBLE, 150, 45, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_parts_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 60, 85, 20, hWnd, ( HMENU )EDIT_RETRY_PARTS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_parts_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 235, 59, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_parts_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_parts_count, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETPOS, 0, cfg_retry_parts_count );

			//

			HWND hWnd_static_timeout = _CreateWindowW( WC_STATIC, ST_Timeout__seconds__, WS_CHILD | WS_VISIBLE, 0, 90, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_timeout = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 105, 85, 20, hWnd, ( HMENU )EDIT_TIMEOUT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_timeout = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 104, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_timeout, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBUDDY, ( WPARAM )g_hWnd_timeout, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETRANGE32, 10, 300 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETPOS, 0, cfg_timeout );
			if ( cfg_timeout == 0 )
			{
				_SendMessageW( g_hWnd_timeout, WM_SETTEXT, 0, ( LPARAM )L"0" );
			}


			HWND hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_Default_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 150, 90, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 150, 105, 100, 20, hWnd, ( HMENU )CB_DEFAULT_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_2_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_3_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_1 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_2 );

			_SendMessageW( g_hWnd_default_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			//








			_SendMessageW( hWnd_static_max_downloads, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_max_downloads, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_retry_downloads_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_retry_downloads_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_retry_parts_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_retry_parts_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_ud_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_ud_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case EDIT_MAX_DOWNLOADS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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

						if ( num != cfg_max_downloads )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_RETRY_DOWNLOADS_COUNT:
				case EDIT_RETRY_PARTS_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_TIMEOUT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						DWORD sel_start = 0;

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
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case CB_DEFAULT_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_DEFAULT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
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
	return TRUE;
}

LRESULT CALLBACK ProxyTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	 switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			//

			g_hWnd_chk_proxy = _CreateWindowW( WC_BUTTON, ST_Enable_HTTP_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_PROXY, NULL, NULL );


			g_hWnd_chk_type_hostname = _CreateWindowW( WC_BUTTON, ST_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 20, 150, 20, hWnd, ( HMENU )BTN_TYPE_HOST, NULL, NULL );
			g_hWnd_chk_type_ip_address = _CreateWindowW( WC_BUTTON, ST_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 20, 95, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS, NULL, NULL );

			g_hWnd_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 235, 20, hWnd, ( HMENU )EDIT_HOST, NULL, NULL );
			g_hWnd_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 40, 235, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_colon = _CreateWindowW( WC_STATIC, ST__, WS_CHILD | WS_VISIBLE, 239, 42, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port = _CreateWindowW( WC_STATIC, ST_Port_, WS_CHILD | WS_VISIBLE, 245, 25, 60, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 245, 40, 60, 20, hWnd, ( HMENU )EDIT_PORT, NULL, NULL );


			g_hWnd_static_auth_username = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 0, 65, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_auth_password = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 110, 65, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 80, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD, NULL, NULL );


			_SendMessageW( g_hWnd_hostname, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port, EM_LIMITTEXT, 5, 0 );


			//

			HWND hWnd_static_hoz = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 110, rc.right, 5, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_chk_proxy_s = _CreateWindowW( WC_BUTTON, ST_Enable_HTTPS_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 200, 20, hWnd, ( HMENU )BTN_PROXY_S, NULL, NULL );


			g_hWnd_chk_type_hostname_s = _CreateWindowW( WC_BUTTON, ST_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 140, 150, 20, hWnd, ( HMENU )BTN_TYPE_HOST_S, NULL, NULL );
			g_hWnd_chk_type_ip_address_s = _CreateWindowW( WC_BUTTON, ST_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 140, 95, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS_S, NULL, NULL );

			g_hWnd_hostname_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 160, 235, 20, hWnd, ( HMENU )EDIT_HOST_S, NULL, NULL );
			g_hWnd_ip_address_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 160, 235, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS_S, NULL, NULL );


			g_hWnd_static_colon_s = _CreateWindowW( WC_STATIC, ST__, WS_CHILD | WS_VISIBLE, 239, 162, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port_s = _CreateWindowW( WC_STATIC, ST_Port_, WS_CHILD | WS_VISIBLE, 245, 145, 60, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 245, 160, 60, 20, hWnd, ( HMENU )EDIT_PORT_S, NULL, NULL );


			g_hWnd_static_auth_username_s = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 0, 185, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 200, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME_S, NULL, NULL );

			g_hWnd_static_auth_password_s = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 110, 185, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 200, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD_S, NULL, NULL );


			_SendMessageW( g_hWnd_hostname_s, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port_s, EM_LIMITTEXT, 5, 0 );


			//


			_SendMessageW( g_hWnd_chk_proxy, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_colon, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_port, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_port, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_username, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_password, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_proxy_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_hostname_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_colon_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_port_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_port_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_username_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_password_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			LOGFONT lf;
			_memzero( &lf, sizeof( LOGFONT ) );
			_GetObjectW( hFont, sizeof( LOGFONT ), &lf );
			hFont_copy = _CreateFontIndirectW( &lf );
			_SendMessageW( g_hWnd_ip_address, WM_SETFONT, ( WPARAM )hFont_copy, 0 );

			_SendMessageW( g_hWnd_ip_address_s, WM_SETFONT, ( WPARAM )hFont_copy, 0 );

			//
			// HTTP proxy.
			//
			_SendMessageW( g_hWnd_chk_proxy, BM_SETCHECK, ( cfg_enable_proxy ? BST_CHECKED : BST_UNCHECKED ), 0 );

			BOOL enable = ( cfg_enable_proxy ? TRUE : FALSE );

			_EnableWindow( g_hWnd_chk_type_hostname, enable );
			_EnableWindow( g_hWnd_chk_type_ip_address, enable );
			_EnableWindow( g_hWnd_hostname, enable );
			_EnableWindow( g_hWnd_ip_address, enable );
			_EnableWindow( g_hWnd_static_colon, enable );
			_EnableWindow( g_hWnd_static_port, enable );
			_EnableWindow( g_hWnd_port, enable );
			_EnableWindow( g_hWnd_static_auth_username, enable );
			_EnableWindow( g_hWnd_auth_username, enable );
			_EnableWindow( g_hWnd_static_auth_password, enable );
			_EnableWindow( g_hWnd_auth_password, enable );

			if ( cfg_address_type == 1 )
			{
				_SendMessageW( g_hWnd_chk_type_ip_address, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_hostname, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_hostname, SW_HIDE );
				_ShowWindow( g_hWnd_ip_address, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_hostname, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_ip_address, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_ip_address, SW_HIDE );
				_ShowWindow( g_hWnd_hostname, SW_SHOW );	
			}

			if ( cfg_hostname == NULL )
			{
				_SendMessageW( g_hWnd_hostname, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
			}
			else
			{
				_SendMessageW( g_hWnd_hostname, WM_SETTEXT, 0, ( LPARAM )cfg_hostname );
			}

			_SendMessageW( g_hWnd_ip_address, IPM_SETADDRESS, 0, cfg_ip_address );

			char value[ 6 ];
			_memzero( value, sizeof( char ) * 6 );
			__snprintf( value, 6, "%hu", cfg_port );
			_SendMessageA( g_hWnd_port, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_auth_username, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_username );
			_SendMessageW( g_hWnd_auth_password, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_password );

			//
			// HTTPS proxy.
			//
			_SendMessageW( g_hWnd_chk_proxy_s, BM_SETCHECK, ( cfg_enable_proxy_s ? BST_CHECKED : BST_UNCHECKED ), 0 );

			enable = ( cfg_enable_proxy_s ? TRUE : FALSE );

			_EnableWindow( g_hWnd_chk_type_hostname_s, enable );
			_EnableWindow( g_hWnd_chk_type_ip_address_s, enable );
			_EnableWindow( g_hWnd_hostname_s, enable );
			_EnableWindow( g_hWnd_ip_address_s, enable );
			_EnableWindow( g_hWnd_static_colon_s, enable );
			_EnableWindow( g_hWnd_static_port_s, enable );
			_EnableWindow( g_hWnd_port_s, enable );
			_EnableWindow( g_hWnd_static_auth_username_s, enable );
			_EnableWindow( g_hWnd_auth_username_s, enable );
			_EnableWindow( g_hWnd_static_auth_password_s, enable );
			_EnableWindow( g_hWnd_auth_password_s, enable );

			if ( cfg_address_type_s == 1 )
			{
				_SendMessageW( g_hWnd_chk_type_ip_address_s, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_hostname_s, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_hostname_s, SW_HIDE );
				_ShowWindow( g_hWnd_ip_address_s, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_hostname_s, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_ip_address_s, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_ip_address_s, SW_HIDE );
				_ShowWindow( g_hWnd_hostname_s, SW_SHOW );	
			}

			if ( cfg_hostname_s == NULL )
			{
				_SendMessageW( g_hWnd_hostname_s, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
			}
			else
			{
				_SendMessageW( g_hWnd_hostname_s, WM_SETTEXT, 0, ( LPARAM )cfg_hostname_s );
			}

			_SendMessageW( g_hWnd_ip_address_s, IPM_SETADDRESS, 0, cfg_ip_address_s );

			_memzero( value, sizeof( char ) * 6 );
			__snprintf( value, 6, "%hu", cfg_port_s );
			_SendMessageA( g_hWnd_port_s, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_auth_username_s, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_username_s );
			_SendMessageW( g_hWnd_auth_password_s, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_password_s );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_PROXY:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_type_hostname, enable );
					_EnableWindow( g_hWnd_chk_type_ip_address, enable );
					_EnableWindow( g_hWnd_hostname, enable );
					_EnableWindow( g_hWnd_ip_address, enable );
					_EnableWindow( g_hWnd_static_colon, enable );
					_EnableWindow( g_hWnd_static_port, enable );
					_EnableWindow( g_hWnd_port, enable );
					_EnableWindow( g_hWnd_static_auth_username, enable );
					_EnableWindow( g_hWnd_auth_username, enable );
					_EnableWindow( g_hWnd_static_auth_password, enable );
					_EnableWindow( g_hWnd_auth_password, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_PROXY_S:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_proxy_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_type_hostname_s, enable );
					_EnableWindow( g_hWnd_chk_type_ip_address_s, enable );
					_EnableWindow( g_hWnd_hostname_s, enable );
					_EnableWindow( g_hWnd_ip_address_s, enable );
					_EnableWindow( g_hWnd_static_colon_s, enable );
					_EnableWindow( g_hWnd_static_port_s, enable );
					_EnableWindow( g_hWnd_port_s, enable );
					_EnableWindow( g_hWnd_static_auth_username_s, enable );
					_EnableWindow( g_hWnd_auth_username_s, enable );
					_EnableWindow( g_hWnd_static_auth_password_s, enable );
					_EnableWindow( g_hWnd_auth_password_s, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_PORT:
				case EDIT_PORT_S:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 6, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 65535 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"65535" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( ( LOWORD( wParam ) == EDIT_PORT	&& num != cfg_port ) ||
							 ( LOWORD( wParam ) == EDIT_PORT_S	&& num != cfg_port_s ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_HOST:
				case EDIT_HOST_S:
				case EDIT_AUTH_USERNAME:
				case EDIT_AUTH_PASSWORD:
				case EDIT_AUTH_USERNAME_S:
				case EDIT_AUTH_PASSWORD_S:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_IP_ADDRESS:
				case EDIT_IP_ADDRESS_S:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case BTN_TYPE_HOST:
				{
					if ( _SendMessageW( g_hWnd_chk_type_hostname, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_ip_address, SW_HIDE );
						_ShowWindow( g_hWnd_hostname, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_HOST_S:
				{
					if ( _SendMessageW( g_hWnd_chk_type_hostname_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_ip_address_s, SW_HIDE );
						_ShowWindow( g_hWnd_hostname_s, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_IP_ADDRESS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_hostname, SW_HIDE );
						_ShowWindow( g_hWnd_ip_address, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_IP_ADDRESS_S:
				{
					if ( _SendMessageW( g_hWnd_chk_type_ip_address_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_hostname_s, SW_HIDE );
						_ShowWindow( g_hWnd_ip_address_s, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_DESTROY:
		{
			_DeleteObject( hFont_copy );
			hFont_copy = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	return TRUE;
}

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_General;
			ti.lParam = ( LPARAM )&g_hWnd_general_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Download;
			ti.lParam = ( LPARAM )&g_hWnd_download_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Proxy;
			ti.lParam = ( LPARAM )&g_hWnd_proxy_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.


			HWND g_hWnd_ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			HWND g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_apply = _CreateWindowW( WC_BUTTON, ST_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );


			_SendMessageW( g_hWnd_options_tab, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Set the tab control's font so we can get the correct tab height to adjust its children.
			RECT rc_tab;
			_GetClientRect( g_hWnd_options_tab, &rc );

			_SendMessageW( g_hWnd_options_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_download_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"download_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_proxy_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"proxy_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );



			_SendMessageW( g_hWnd_ok, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_apply, WM_SETFONT, ( WPARAM )hFont, 0 );



			// Set settings.

			if ( cfg_tray_icon )
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, TRUE );
				_EnableWindow( g_hWnd_chk_close, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, FALSE );
				_EnableWindow( g_hWnd_chk_close, FALSE );
			}

			_SendMessageW( g_hWnd_chk_minimize, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_close, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_download_history, BM_SETCHECK, ( cfg_enable_download_history ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_quick_allocation, BM_SETCHECK, ( cfg_enable_quick_allocation ? BST_CHECKED : BST_UNCHECKED ), 0 );

			/*char value[ 11 ];

			__snprintf( value, 11, "%lu", cfg_max_downloads );
			_SendMessageA( g_hWnd_max_downloads, WM_SETTEXT, 0, ( LPARAM )value );

			__snprintf( value, 11, "%lu", cfg_thread_count );
			_SendMessageA( g_hWnd_thread_count, WM_SETTEXT, 0, ( LPARAM )value );*/

			if ( cfg_default_download_directory != NULL )
			{
				t_default_download_directory = GlobalStrDupW( cfg_default_download_directory );
			}


			options_state_changed = false;
			_EnableWindow( g_hWnd_apply, FALSE );

			return 0;
		}
		break;
/*
		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc, frame_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_MENU ) );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			frame_rc = client_rc;
			frame_rc.left += 10;
			frame_rc.right -= 10;
			frame_rc.top += 10;
			frame_rc.bottom -= 40;

			// Fill the frame.
			color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &frame_rc, color );
			_DeleteObject( color );

			// Draw the frame's border.
			_DrawEdge( hdcMem, &frame_rc, EDGE_ETCHED, BF_RECT );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;
*/

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_HIDE );
					}

					return FALSE;
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						SetFocus( GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )tie.lParam ) );
					}

					return FALSE;
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_OK:
				case BTN_APPLY:
				{
					if ( !options_state_changed )
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
						break;
					}

					if ( cfg_default_download_directory != NULL )
					{
						GlobalFree( cfg_default_download_directory );
					}

					g_default_download_directory_length = lstrlenW( t_default_download_directory );
					cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ) );
					//_memcpy_s( cfg_default_download_directory, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ), t_default_download_directory, sizeof( wchar_t ) * g_default_download_directory_length );
					_wmemcpy_s( cfg_default_download_directory, g_default_download_directory_length + 1, t_default_download_directory, g_default_download_directory_length );
					*( cfg_default_download_directory + g_default_download_directory_length ) = 0;	// Sanity.

					cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool tray_icon = ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// Add the tray icon if it was not previously enabled.
					if ( tray_icon && !cfg_tray_icon )
					{
						g_nid.cbSize = sizeof( g_nid );
						g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
						g_nid.hWnd = g_hWnd_main;
						g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
						g_nid.uID = 1000;
						g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
						_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), PROGRAM_CAPTION, 15 );
						g_nid.szTip[ 15 ] = 0;	// Sanity.
						_Shell_NotifyIconW( NIM_ADD, &g_nid );
					}
					else if ( !tray_icon && cfg_tray_icon )	// Remove the tray icon if it was previously enabled.
					{
						// Make sure that the main window is not hidden before we delete the tray icon.
						if ( _IsWindowVisible( g_hWnd_main ) == FALSE )
						{
							_ShowWindow( g_hWnd_main, SW_SHOWNOACTIVATE );
						}

						_Shell_NotifyIconW( NIM_DELETE, &g_nid );
					}

					cfg_tray_icon = tray_icon;

					bool always_on_top = ( _SendMessageW( g_hWnd_chk_always_on_top, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// Set any active windows if we've changed the extended style.
					if ( always_on_top != cfg_always_on_top )
					{
						cfg_always_on_top = always_on_top;

						if ( g_hWnd_main != NULL ){ _SetWindowPos( g_hWnd_main, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_add_urls != NULL ){ _SetWindowPos( g_hWnd_add_urls, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_options != NULL ){ _SetWindowPos( g_hWnd_options, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
					}

					cfg_enable_download_history = ( _SendMessageW( g_hWnd_chk_download_history, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool enable_quick_allocation = ( _SendMessageW( g_hWnd_chk_quick_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					unsigned char display_notice = 0;
					if ( enable_quick_allocation != cfg_enable_quick_allocation )
					{
						cfg_enable_quick_allocation = enable_quick_allocation;

						display_notice += 1;
					}

					char value[ 11 ];
					_SendMessageA( g_hWnd_max_downloads, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_max_downloads = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_retry_downloads_count, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_retry_downloads_count = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_retry_parts_count, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_retry_parts_count = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_timeout, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned short timeout = ( unsigned short )_strtoul( value, NULL, 10 );

					if ( timeout != cfg_timeout )
					{
						if ( cfg_timeout == 0 )
						{
							cfg_timeout = timeout;	// New value will not be 0 and the timeout thread will poll every 1 second.

							// Trigger the timeout thread to poll.
							if ( g_timeout_semaphore != NULL )
							{
								ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
							}
						}
						else
						{
							cfg_timeout = timeout;
						}
					}

					_SendMessageA( g_hWnd_default_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_default_download_parts = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_thread_count, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned long thread_count = _strtoul( value, NULL, 10 );

					cfg_default_ssl_version = ( unsigned char )_SendMessageW( g_hWnd_default_ssl_version, CB_GETCURSEL, 0, 0 );

					if ( thread_count != cfg_thread_count )
					{
						cfg_thread_count = thread_count;

						display_notice += 2;
					}

					//
					// HTTP proxy.
					//
					cfg_enable_proxy = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type = ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int hostname_length = _SendMessageW( g_hWnd_hostname, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname != NULL )
					{
						GlobalFree( cfg_hostname );
					}
					cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname );

					_SendMessageW( g_hWnd_ip_address, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address );

					_SendMessageA( g_hWnd_port, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port = ( unsigned short )_strtoul( value, NULL, 10 );

					unsigned int auth_length = _SendMessageW( g_hWnd_auth_username, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username != NULL )
					{
						GlobalFree( cfg_proxy_auth_username );
					}
					cfg_proxy_auth_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username );

					auth_length = _SendMessageW( g_hWnd_auth_password, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_password != NULL )
					{
						GlobalFree( cfg_proxy_auth_password );
					}
					cfg_proxy_auth_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_password, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_password );

					if ( normaliz_state == NORMALIZ_STATE_RUNNING )
					{
						if ( cfg_address_type == 0 )
						{
							if ( g_punycode_hostname != NULL )
							{
								GlobalFree( g_punycode_hostname );
								g_punycode_hostname = NULL;
							}

							int punycode_length = _IdnToAscii( 0, cfg_hostname, hostname_length, NULL, 0 );

							if ( punycode_length > ( int )hostname_length )
							{
								g_punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, cfg_hostname, hostname_length, g_punycode_hostname, punycode_length );
							}
						}
					}

					//
					// HTTPS proxy.
					//
					cfg_enable_proxy_s = ( _SendMessageW( g_hWnd_chk_proxy_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type_s = ( _SendMessageW( g_hWnd_chk_type_ip_address_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					hostname_length = _SendMessageW( g_hWnd_hostname_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname_s != NULL )
					{
						GlobalFree( cfg_hostname_s );
					}
					cfg_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname_s, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname_s );

					_SendMessageW( g_hWnd_ip_address_s, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address_s );

					_SendMessageA( g_hWnd_port_s, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port_s = ( unsigned short )_strtoul( value, NULL, 10 );

					auth_length = _SendMessageW( g_hWnd_auth_username_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username_s != NULL )
					{
						GlobalFree( cfg_proxy_auth_username_s );
					}
					cfg_proxy_auth_username_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username_s, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username_s );

					auth_length = _SendMessageW( g_hWnd_auth_password_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_password_s != NULL )
					{
						GlobalFree( cfg_proxy_auth_password_s );
					}
					cfg_proxy_auth_password_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_password_s, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_password_s );

					if ( normaliz_state == NORMALIZ_STATE_RUNNING )
					{
						if ( cfg_address_type_s == 0 )
						{
							if ( g_punycode_hostname_s != NULL )
							{
								GlobalFree( g_punycode_hostname_s );
								g_punycode_hostname_s = NULL;
							}

							int punycode_length = _IdnToAscii( 0, cfg_hostname_s, hostname_length, NULL, 0 );

							if ( punycode_length > ( int )hostname_length )
							{
								g_punycode_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, cfg_hostname_s, hostname_length, g_punycode_hostname_s, punycode_length );
							}
						}
					}

					//
					//
					//

					save_config();

					int auth_username_length = 0, auth_password_length = 0;

					g_proxy_auth_key_length = 0;
					if ( g_proxy_auth_key != NULL )
					{
						GlobalFree( g_proxy_auth_key );
						g_proxy_auth_key = NULL;
					}

					if ( cfg_proxy_auth_username != NULL && cfg_proxy_auth_password != NULL )
					{
						if ( g_proxy_auth_username != NULL )
						{
							GlobalFree( g_proxy_auth_username );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, g_proxy_auth_username, auth_username_length, NULL, NULL ) - 1;

						if ( g_proxy_auth_password != NULL )
						{
							GlobalFree( g_proxy_auth_password );
						}
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, g_proxy_auth_password, auth_password_length, NULL, NULL ) - 1;

						CreateBasicAuthorizationKey( g_proxy_auth_username, auth_username_length, g_proxy_auth_password, auth_password_length, &g_proxy_auth_key, &g_proxy_auth_key_length );
					}

					g_proxy_auth_key_length_s = 0;
					if ( g_proxy_auth_key_s != NULL )
					{
						GlobalFree( g_proxy_auth_key_s );
						g_proxy_auth_key_s = NULL;
					}

					if ( cfg_proxy_auth_username_s != NULL && cfg_proxy_auth_password_s != NULL )
					{
						if ( g_proxy_auth_username_s != NULL )
						{
							GlobalFree( g_proxy_auth_username_s );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_username_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, g_proxy_auth_username_s, auth_username_length, NULL, NULL ) - 1;

						if ( g_proxy_auth_password_s != NULL )
						{
							GlobalFree( g_proxy_auth_password_s );
						}
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_password_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, g_proxy_auth_password_s, auth_password_length, NULL, NULL ) - 1;

						CreateBasicAuthorizationKey( g_proxy_auth_username_s, auth_username_length, g_proxy_auth_password_s, auth_password_length, &g_proxy_auth_key_s, &g_proxy_auth_key_length_s );
					}

					if ( display_notice == 1 )
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required_allocation, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice == 2 )
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required_threads, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice > 2 )	// Multiple settings changed.
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}

					options_state_changed = false;

					if ( LOWORD( wParam ) == BTN_APPLY )
					{
						_EnableWindow( g_hWnd_apply, FALSE );
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
					}
				}
				break;

				case BTN_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;	
			}

			return 0;
		}

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
			if ( t_default_download_directory != NULL )
			{
				GlobalFree( t_default_download_directory );
				t_default_download_directory = NULL;
			}

			g_hWnd_options = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
