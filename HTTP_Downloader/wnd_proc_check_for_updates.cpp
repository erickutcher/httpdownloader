/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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
#include "connection.h"
#include "string_tables.h"

#define BTN_DOWNLOAD_UPDATE				1000
#define BTN_VIEW_CHANGELOG				1001
#define BTN_CHECK_UPDATES_CANCEL		1002
#define BTN_VISIT_HOME_PAGE				1003

HWND g_hWnd_check_for_updates = NULL;

HWND g_hWnd_static_check_for_updates = NULL;
HWND g_hWnd_static_current_version = NULL;
HWND g_hWnd_static_latest_version = NULL;

HWND g_hWnd_visit_home_page = NULL;
HWND g_hWnd_download_update = NULL;
HWND g_hWnd_view_changelog = NULL;
HWND g_hWnd_check_updates_cancel = NULL;

LRESULT CALLBACK CheckForUpdatesWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_static_check_for_updates = _CreateWindowW( WC_STATIC, ST_V_Checking_for_updates___, WS_CHILD | WS_VISIBLE, 10, 10, rc.right - 20, 15, hWnd, NULL, NULL, NULL );

			wchar_t current_version[ 128 ];
			__snwprintf( current_version, 128, L"%s %lu.%lu.%lu.%lu", ST_V_Current_version_, CURRENT_VERSION_A, CURRENT_VERSION_B, CURRENT_VERSION_C, CURRENT_VERSION_D );

			g_hWnd_static_current_version = _CreateWindowW( WC_STATIC, current_version, WS_CHILD | WS_VISIBLE, 10, 30, rc.right - 20, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_static_latest_version = _CreateWindowW( WC_STATIC, ST_V_Latest_version_, WS_CHILD | WS_VISIBLE, 10, 45, rc.right - 20, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_visit_home_page = _CreateWindowW( WC_BUTTON, ST_V_Visit_Home_Page, WS_CHILD | WS_TABSTOP, 10, rc.bottom - 32, 125, 23, hWnd, ( HMENU )BTN_VISIT_HOME_PAGE, NULL, NULL );
			g_hWnd_download_update = _CreateWindowW( WC_BUTTON, ST_V_Download_Update, WS_CHILD | WS_TABSTOP, 10, rc.bottom - 32, 125, 23, hWnd, ( HMENU )BTN_DOWNLOAD_UPDATE, NULL, NULL );
			g_hWnd_view_changelog = _CreateWindowW( WC_BUTTON, ST_V_View_Changelog, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 215, rc.bottom - 32, 120, 23, hWnd, ( HMENU )BTN_VIEW_CHANGELOG, NULL, NULL );
			g_hWnd_check_updates_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CHECK_UPDATES_CANCEL, NULL, NULL );

			_SendMessageW( g_hWnd_static_check_for_updates, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_current_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_latest_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_visit_home_page, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_download_update, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_view_changelog, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_check_updates_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				/*case IDOK:
				case BTN_SEARCH:
				case BTN_SEARCH_ALL:
				{
					// Prevents the user from holding down the button and queuing up a bunch of threads.
					if ( in_worker_thread )
					{
						break;
					}

					SEARCH_INFO *si = ( SEARCH_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SEARCH_INFO ) );
					si->search_all = ( LOWORD( wParam ) == BTN_SEARCH_ALL ? true : false );

					if ( _SendMessageW( g_hWnd_chk_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						si->search_flag = 0x04;	// Use regular expression.
					}
					else
					{
						si->search_flag = ( _SendMessageW( g_hWnd_chk_match_case, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x01 : 0x00 );
						si->search_flag |= ( _SendMessageW( g_hWnd_chk_match_whole_word, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 0x02 : 0x00 );
					}

					si->type = ( _SendMessageW( g_hWnd_chk_type_url, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int text_length = ( unsigned int )_SendMessageW( g_hWnd_search_for, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					si->text = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * text_length );
					_SendMessageW( g_hWnd_search_for, WM_GETTEXT, text_length, ( LPARAM )si->text );

					// Enabled, when our thread completes.
					_EnableWindow( g_hWnd_btn_search_all, FALSE );
					_EnableWindow( g_hWnd_btn_search, FALSE );

					// si is freed in search_list.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, search_list, ( void * )si, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						_EnableWindow( g_hWnd_btn_search_all, TRUE );
						_EnableWindow( g_hWnd_btn_search, TRUE );

						GlobalFree( si->text );
						GlobalFree( si );
					}
				}
				break;*/

				case BTN_DOWNLOAD_UPDATE:
				{
					_SendMessageW( ( g_hWnd_add_urls != NULL ? g_hWnd_add_urls : g_hWnd_main ), WM_PROPAGATE, CF_TEXT, ( LPARAM )g_new_version_url );

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_VISIT_HOME_PAGE:
				{
					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
					}

					_ShellExecuteW( NULL, L"open", HOME_PAGE, NULL, NULL, SW_SHOWNORMAL );

					if ( destroy )
					{
						_CoUninitialize();
					}
				}
				break;

				case BTN_VIEW_CHANGELOG:
				{
					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
					}

					_ShellExecuteW( NULL, L"open", CHANGELOG, NULL, NULL, SW_SHOWNORMAL );

					if ( destroy )
					{
						_CoUninitialize();
					}
				}
				break;

				case BTN_CHECK_UPDATES_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			wchar_t latest_version[ 128 ];

			if ( wParam == 1 )
			{
				_SendMessageW( g_hWnd_static_check_for_updates, WM_SETTEXT, NULL, ( LPARAM )ST_V_A_new_version_is_available_ );

				unsigned long version_a = g_new_version >> 24;
				unsigned long version_b = ( g_new_version & 0x00FF0000 ) >> 16;
				unsigned long version_c = ( g_new_version & 0x0000FF00 ) >> 8;
				unsigned long version_d = ( g_new_version & 0x000000FF );

				__snwprintf( latest_version, 128, L"%s %lu.%lu.%lu.%lu", ST_V_Latest_version_, version_a, version_b, version_c, version_d );
				_SendMessageW( g_hWnd_static_latest_version, WM_SETTEXT, NULL, ( LPARAM )latest_version );

				_EnableWindow( g_hWnd_download_update, TRUE );
				_ShowWindow( g_hWnd_download_update, SW_SHOW );
			}
			else if ( wParam == 2 )
			{
				_SendMessageW( g_hWnd_static_check_for_updates, WM_SETTEXT, NULL, ( LPARAM )ST_V_HTTP_Downloader_is_up_to_date_ );

				__snwprintf( latest_version, 128, L"%s %lu.%lu.%lu.%lu", ST_V_Latest_version_, CURRENT_VERSION_A, CURRENT_VERSION_B, CURRENT_VERSION_C, CURRENT_VERSION_D );
				_SendMessageW( g_hWnd_static_latest_version, WM_SETTEXT, NULL, ( LPARAM )latest_version );
			}
			else if ( wParam == 3 )
			{
				_SendMessageW( g_hWnd_static_check_for_updates, WM_SETTEXT, NULL, ( LPARAM )ST_V_The_update_check_has_failed_ );

				_EnableWindow( g_hWnd_visit_home_page, TRUE );
				_ShowWindow( g_hWnd_visit_home_page, SW_SHOW );
			}
			else if ( wParam == 4 )	// Automatic updated succeeded and we have the latest version. Destory the window.
			{
				_DestroyWindow( hWnd );

				break;
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );
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
			/*_ShowWindow( hWnd, SW_HIDE );

			_SendMessageW( g_hWnd_static_check_for_updates, WM_SETTEXT, NULL, ( LPARAM )L"Checking for updates..." );

			_EnableWindow( g_hWnd_download_update, FALSE );
			_ShowWindow( g_hWnd_download_update, SW_HIDE );

			_EnableWindow( g_hWnd_visit_home_page, FALSE );
			_ShowWindow( g_hWnd_visit_home_page, SW_HIDE );*/

			// Release the semaphore to complete the update check.
			if ( g_update_semaphore != NULL )
			{
				ReleaseSemaphore( g_update_semaphore, 1, NULL );
			}

			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			g_new_version = 0;

			if ( g_new_version_url != NULL )
			{
				GlobalFree( g_new_version_url );
				g_new_version_url = NULL;
			}

			g_hWnd_check_for_updates = NULL;

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
