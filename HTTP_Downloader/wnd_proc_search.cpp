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

#include "globals.h"
#include "connection.h"
#include "list_operations.h"
#include "string_tables.h"
#include "lite_gdi32.h"
#include "lite_pcre2.h"
#include "utilities.h"

#include "dark_mode.h"

#define SEARCH_WINDOW_WIDTH		400
#define SEARCH_CLIENT_HEIGHT	183

#define BTN_SEARCH_ALL			1000
#define BTN_SEARCH				1001
#define BTN_SEARCH_CANCEL		1002

#define EDIT_SEARCH_FOR			1003

#define BTN_TYPE_FILENAME		1004
#define BTN_TYPE_URL			1005
#define BTN_TYPE_COMMENTS		1006

#define BTN_MATCH_CASE			1007
#define BTN_MATCH_WHOLE_WORD	1008
#define BTN_REGULAR_EXPRESSION	1009

HWND g_hWnd_search = NULL;

HWND g_hWnd_static_search_for = NULL;
HWND g_hWnd_search_for = NULL;
HWND g_hWnd_btn_search_type = NULL;
HWND g_hWnd_chk_type_filename = NULL;
HWND g_hWnd_chk_type_url = NULL;
HWND g_hWnd_chk_type_comments = NULL;
HWND g_hWnd_chk_match_case = NULL;
HWND g_hWnd_chk_match_whole_word = NULL;
HWND g_hWnd_chk_regular_expression = NULL;
HWND g_hWnd_btn_search_all = NULL;
HWND g_hWnd_btn_search = NULL;
HWND g_hWnd_search_cancel = NULL;

UINT current_dpi_search = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_search = 0;
HFONT hFont_search = NULL;

#define _SCALE_S_( x )						_SCALE_( ( x ), dpi_search )

LRESULT CALLBACK SearchWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			current_dpi_search = __GetDpiForWindow( hWnd );
			last_dpi_search = ( current_dpi_search == current_dpi_main ? current_dpi_search : 0 );
			hFont_search = UpdateFont( current_dpi_search );

			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_static_search_for = _CreateWindowW( WC_STATIC, ST_V_Search_for_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_search_for = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SEARCH_FOR, NULL, NULL );

			g_hWnd_btn_search_type = _CreateWindowW( WC_BUTTON, ST_V_Search_Type, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_chk_type_filename = _CreateWindowW( WC_BUTTON, ST_V_Filename, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_FILENAME, NULL, NULL );
			g_hWnd_chk_type_url = _CreateWindowW( WC_BUTTON, ST_V_URL, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_URL, NULL, NULL );
			g_hWnd_chk_type_comments = _CreateWindowW( WC_BUTTON, ST_V_Comments, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_COMMENTS, NULL, NULL );

			g_hWnd_chk_match_case = _CreateWindowW( WC_BUTTON, ST_V_Match_case, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_MATCH_CASE, NULL, NULL );
			g_hWnd_chk_match_whole_word = _CreateWindowW( WC_BUTTON, ST_V_Match_whole_word, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_MATCH_WHOLE_WORD, NULL, NULL );
			g_hWnd_chk_regular_expression = _CreateWindowW( WC_BUTTON, ST_V_Regular_expression, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE | ( pcre2_state == PCRE2_STATE_RUNNING ? 0 : WS_DISABLED ), 0, 0, 0, 0, hWnd, ( HMENU )BTN_REGULAR_EXPRESSION, NULL, NULL );

			g_hWnd_btn_search_all = _CreateWindowW( WC_BUTTON, ST_V_Search_All, WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SEARCH_ALL, NULL, NULL );
			g_hWnd_btn_search = _CreateWindowW( WC_BUTTON, ST_V_Search_Next, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SEARCH, NULL, NULL );
			g_hWnd_search_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SEARCH_CANCEL, NULL, NULL );

			_SendMessageW( g_hWnd_static_search_for, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_search_for, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_btn_search_type, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_type_filename, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_type_url, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_type_comments, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_match_case, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_match_whole_word, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_chk_regular_expression, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_btn_search_all, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_btn_search, WM_SETFONT, ( WPARAM )hFont_search, 0 );
			_SendMessageW( g_hWnd_search_cancel, WM_SETFONT, ( WPARAM )hFont_search, 0 );

			_SendMessageW( g_hWnd_chk_type_filename, BM_SETCHECK, BST_CHECKED, 0 );

			_SetFocus( g_hWnd_search_for );

			int width = _SCALE_S_( SEARCH_WINDOW_WIDTH );

			// Accounts for differing title bar heights.
			CREATESTRUCTW *cs = ( CREATESTRUCTW * )lParam;
			int height = ( cs->cy - ( rc.bottom - rc.top ) ) + _SCALE_S_( SEARCH_CLIENT_HEIGHT );	// Bottom of last window object + 10.

			HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );

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
				case BTN_SEARCH:
				case BTN_SEARCH_ALL:
				{
					// Prevents the user from holding down the button and queuing up a bunch of threads.
					if ( in_worker_thread )
					{
						break;
					}

					SEARCH_INFO *si = ( SEARCH_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SEARCH_INFO ) );
					if ( si != NULL )
					{
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

						if ( _SendMessageW( g_hWnd_chk_type_comments, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						{
							si->type = 2;
						}
						else
						{
							si->type = ( _SendMessageW( g_hWnd_chk_type_url, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );
						}

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
					else
					{
						_EnableWindow( g_hWnd_btn_search_all, TRUE );
						_EnableWindow( g_hWnd_btn_search, TRUE );
					}
				}
				break;

				case EDIT_SEARCH_FOR:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						if ( _SendMessageW( ( HWND )lParam, WM_GETTEXTLENGTH, 0, 0 ) > 0 )
						{
							_EnableWindow( g_hWnd_btn_search_all, TRUE );
							_EnableWindow( g_hWnd_btn_search, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_btn_search_all, FALSE );
							_EnableWindow( g_hWnd_btn_search, FALSE );
						}
					}
				}
				break;

				case BTN_REGULAR_EXPRESSION:
				{
					if ( _SendMessageW( g_hWnd_chk_regular_expression, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_match_case, FALSE );
						_EnableWindow( g_hWnd_chk_match_whole_word, FALSE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_match_case, TRUE );
						_EnableWindow( g_hWnd_chk_match_whole_word, TRUE );
					}
				}
				break;

				case BTN_SEARCH_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HDWP hdwp = _BeginDeferWindowPos( 12 );
			_DeferWindowPos( hdwp, g_hWnd_static_search_for, HWND_TOP, _SCALE_S_( 10 ), _SCALE_S_( 10 ), rc.right - _SCALE_S_( 20 ), _SCALE_S_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_search_for, HWND_TOP, _SCALE_S_( 10 ), _SCALE_S_( 28 ), rc.right - _SCALE_S_( 20 ), _SCALE_S_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_search_type, HWND_TOP, _SCALE_S_( 10 ), _SCALE_S_( 58 ), _SCALE_S_( 112 ), _SCALE_S_( 83 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_type_filename, HWND_TOP, _SCALE_S_( 21 ), _SCALE_S_( 75 ), _SCALE_S_( 90 ), _SCALE_S_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_type_url, HWND_TOP, _SCALE_S_( 21 ), _SCALE_S_( 95 ), _SCALE_S_( 90 ), _SCALE_S_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_type_comments, HWND_TOP, _SCALE_S_( 21 ), _SCALE_S_( 115 ), _SCALE_S_( 90 ), _SCALE_S_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_match_case, HWND_TOP, _SCALE_S_( 135 ), _SCALE_S_( 62 ), rc.right - _SCALE_S_( 145 ), _SCALE_S_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_match_whole_word, HWND_TOP, _SCALE_S_( 135 ), _SCALE_S_( 82 ), rc.right - _SCALE_S_( 145 ), _SCALE_S_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_regular_expression, HWND_TOP, _SCALE_S_( 135 ), _SCALE_S_( 102 ), rc.right - _SCALE_S_( 145 ), _SCALE_S_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_btn_search_all, HWND_TOP, rc.right - _SCALE_S_( 285 ), _SCALE_S_( 150 ), _SCALE_S_( 85 ), _SCALE_S_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_search, HWND_TOP, rc.right - _SCALE_S_( 195 ), _SCALE_S_( 150 ), _SCALE_S_( 100 ), _SCALE_S_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_search_cancel, HWND_TOP, rc.right - _SCALE_S_( 90 ), _SCALE_S_( 150 ), _SCALE_S_( 80 ), _SCALE_S_( 23 ), SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_search;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_search;
			current_dpi_search = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_search );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_search );
			hFont_search = hFont;

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_search == 0 )
			{
				HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
				MONITORINFO mi;
				mi.cbSize = sizeof( MONITORINFO );
				_GetMonitorInfoW( hMon, &mi );
				_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );
			}
			else
			{
				_SetWindowPos( hWnd, NULL, rc->left, rc->top, width, height, SWP_NOZORDER | SWP_NOACTIVATE );
			}

			last_dpi_search = last_dpi;

			return 0;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == 1 )
			{
				_EnableWindow( g_hWnd_btn_search_all, TRUE );
				_EnableWindow( g_hWnd_btn_search, TRUE );
			}
			else
			{
				_ShowWindow( hWnd, SW_SHOWNORMAL );
				_SetForegroundWindow( hWnd );

				_SetFocus( g_hWnd_search_for );
			}

			return TRUE;
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
			_ShowWindow( hWnd, SW_HIDE );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			// Delete our font.
			_DeleteObject( hFont_search );

			g_hWnd_search = NULL;

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				CleanupButtonGlyphs( hWnd );
			}
#endif

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
