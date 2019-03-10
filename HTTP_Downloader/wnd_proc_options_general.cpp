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

#include "options.h"
#include "lite_comdlg32.h"
#include "lite_ole32.h"
#include "utilities.h"

#define BTN_TRAY_ICON					1000
#define BTN_MINIMIZE_TO_TRAY			1001
#define BTN_CLOSE_TO_TRAY				1002
#define BTN_SHOW_NOTIFICATION			1003
#define BTN_SHOW_TRAY_PROGRESS			1004

#define BTN_ALWAYS_ON_TOP				1005

#define BTN_ENABLE_DROP_WINDOW			1006
#define EDIT_DROP_WINDOW_TRANSPARENCY	1007
#define BTN_SHOW_DROP_WINDOW_PROGRESS	1008

#define BTN_PLAY_SOUND					1009
#define EDIT_SOUND_FILE					1010
#define BTN_LOAD_SOUND_FILE				1011

// General Tab
HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize = NULL;
HWND g_hWnd_chk_close = NULL;
HWND g_hWnd_chk_show_notification = NULL;

HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_static_drop_window_transparency = NULL;
HWND g_hWnd_drop_window_transparency = NULL;
HWND g_hWnd_ud_drop_window_transparency = NULL;
HWND g_hWnd_chk_enable_drop_window = NULL;

HWND g_hWnd_chk_play_sound = NULL;
HWND g_hWnd_sound_file = NULL;
HWND g_hWnd_load_sound_file = NULL;

HWND g_hWnd_chk_show_tray_progress = NULL;
HWND g_hWnd_chk_show_drop_window_progress = NULL;

wchar_t *t_sound_file_path = NULL;

LRESULT CALLBACK GeneralTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_tray_icon = _CreateWindowW( WC_BUTTON, ST_V_Enable_System_Tray_icon_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, rc.right, 20, hWnd, ( HMENU )BTN_TRAY_ICON, NULL, NULL );
			g_hWnd_chk_minimize = _CreateWindowW( WC_BUTTON, ST_V_Minimize_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 20, rc.right - 15, 20, hWnd, ( HMENU )BTN_MINIMIZE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_close = _CreateWindowW( WC_BUTTON, ST_V_Close_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 40, rc.right - 15, 20, hWnd, ( HMENU )BTN_CLOSE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_show_notification = _CreateWindowW( WC_BUTTON, ST_V_Show_notification_when_downloads_finish, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 60, rc.right - 15, 20, hWnd, ( HMENU )BTN_SHOW_NOTIFICATION, NULL, NULL );
			g_hWnd_chk_show_tray_progress = _CreateWindowW( WC_BUTTON, ST_V_Show_progress_bar, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 80, rc.right - 15, 20, hWnd, ( HMENU )BTN_SHOW_TRAY_PROGRESS, NULL, NULL );

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_V_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 100, rc.right, 20, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );

			g_hWnd_chk_enable_drop_window = _CreateWindowW( WC_BUTTON, ST_V_Enable_URL_drop_window_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, rc.right, 20, hWnd, ( HMENU )BTN_ENABLE_DROP_WINDOW, NULL, NULL );

			g_hWnd_static_drop_window_transparency = _CreateWindowW( WC_STATIC, ST_V_Transparency_, WS_CHILD | WS_VISIBLE, 15, 142, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_drop_window_transparency = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 115, 140, 60, 20, hWnd, ( HMENU )EDIT_DROP_WINDOW_TRANSPARENCY, NULL, NULL );
			// Keep this unattached. Looks ugly inside the text box.
			g_hWnd_ud_drop_window_transparency = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 175, 139, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_drop_window_transparency, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETBUDDY, ( WPARAM )g_hWnd_drop_window_transparency, 0 );
            _SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETRANGE32, 0, 255 );

			g_hWnd_chk_show_drop_window_progress = _CreateWindowW( WC_BUTTON, ST_V_Show_progress_bar, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 160, rc.right - 15, 20, hWnd, ( HMENU )BTN_SHOW_DROP_WINDOW_PROGRESS, NULL, NULL );

			g_hWnd_chk_play_sound = _CreateWindowW( WC_BUTTON, ST_V_Play_sound_when_downloads_finish_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 180, rc.right, 20, hWnd, ( HMENU )BTN_PLAY_SOUND, NULL, NULL );
			g_hWnd_sound_file = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_sound_file_path, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 200, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_load_sound_file = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 200, 35, 20, hWnd, ( HMENU )BTN_LOAD_SOUND_FILE, NULL, NULL );


			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_minimize, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_close, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_notification, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_tray_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_enable_drop_window, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_drop_window_transparency, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_drop_window_transparency, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_show_drop_window_progress, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_play_sound, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_sound_file, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_load_sound_file, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			
			// Set settings.

			if ( cfg_tray_icon )
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, TRUE );
				_EnableWindow( g_hWnd_chk_close, TRUE );
				_EnableWindow( g_hWnd_chk_show_notification, TRUE );
				_EnableWindow( g_hWnd_chk_show_tray_progress, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, FALSE );
				_EnableWindow( g_hWnd_chk_close, FALSE );
				_EnableWindow( g_hWnd_chk_show_notification, FALSE );
				_EnableWindow( g_hWnd_chk_show_tray_progress, FALSE );
			}

			_SendMessageW( g_hWnd_chk_minimize, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_close, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_show_notification, BM_SETCHECK, ( cfg_show_notification ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_show_tray_progress, BM_SETCHECK, ( cfg_show_tray_progress ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );

			if ( cfg_enable_drop_window )
			{
				_SendMessageW( g_hWnd_chk_enable_drop_window, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_static_drop_window_transparency, TRUE );
				_EnableWindow( g_hWnd_drop_window_transparency, TRUE );
				_EnableWindow( g_hWnd_ud_drop_window_transparency, TRUE );
				_EnableWindow( g_hWnd_chk_show_drop_window_progress, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_enable_drop_window, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_static_drop_window_transparency, FALSE );
				_EnableWindow( g_hWnd_drop_window_transparency, FALSE );
				_EnableWindow( g_hWnd_ud_drop_window_transparency, FALSE );
				_EnableWindow( g_hWnd_chk_show_drop_window_progress, FALSE );
			}

			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETPOS, 0, cfg_drop_window_transparency );
			_SendMessageW( g_hWnd_chk_show_drop_window_progress, BM_SETCHECK, ( cfg_show_drop_window_progress ? BST_CHECKED : BST_UNCHECKED ), 0 );

			if ( cfg_play_sound )
			{
				_SendMessageW( g_hWnd_chk_play_sound, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_sound_file, TRUE );
				_EnableWindow( g_hWnd_load_sound_file, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_play_sound, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_sound_file, FALSE );
				_EnableWindow( g_hWnd_load_sound_file, FALSE );
			}

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
			switch ( LOWORD( wParam ) )
			{
				case BTN_TRAY_ICON:
				{
					if ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_minimize, TRUE );
						_EnableWindow( g_hWnd_chk_close, TRUE );
						_EnableWindow( g_hWnd_chk_show_notification, TRUE );
						_EnableWindow( g_hWnd_chk_show_tray_progress, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_minimize, FALSE );
						_EnableWindow( g_hWnd_chk_close, FALSE );
						_EnableWindow( g_hWnd_chk_show_notification, FALSE );
						_EnableWindow( g_hWnd_chk_show_tray_progress, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_ENABLE_DROP_WINDOW:
				{
					if ( _SendMessageW( g_hWnd_chk_enable_drop_window, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_static_drop_window_transparency, TRUE );
						_EnableWindow( g_hWnd_drop_window_transparency, TRUE );
						_EnableWindow( g_hWnd_ud_drop_window_transparency, TRUE );
						_EnableWindow( g_hWnd_chk_show_drop_window_progress, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_static_drop_window_transparency, FALSE );
						_EnableWindow( g_hWnd_drop_window_transparency, FALSE );
						_EnableWindow( g_hWnd_ud_drop_window_transparency, FALSE );
						_EnableWindow( g_hWnd_chk_show_drop_window_progress, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_DROP_WINDOW_TRANSPARENCY:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 255 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"255" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_drop_window_transparency )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_CLOSE_TO_TRAY:
				case BTN_MINIMIZE_TO_TRAY:
				case BTN_ALWAYS_ON_TOP:
				case BTN_SHOW_NOTIFICATION:
				case BTN_SHOW_TRAY_PROGRESS:
				case BTN_SHOW_DROP_WINDOW_PROGRESS:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_PLAY_SOUND:
				{
					bool show_dialog = false;

					if ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						if ( _SendMessageW( g_hWnd_sound_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
						{
							show_dialog = true;
						}

						_EnableWindow( g_hWnd_sound_file, TRUE );
						_EnableWindow( g_hWnd_load_sound_file, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_sound_file, FALSE );
						_EnableWindow( g_hWnd_load_sound_file, FALSE );
					}

					// Fall through if we haven't chosen a file yet.
					if ( !show_dialog )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );

						break;
					}
				}

				case BTN_LOAD_SOUND_FILE:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"WAV (*.WAV;*.WAVE)\0*.wav;*.wave\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = ST_V_Load_Download_Finish_Sound_File;
					ofn.lpstrFile = file_name;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY;

					if ( _GetOpenFileNameW( &ofn ) )
					{
						if ( t_sound_file_path != NULL )
						{
							GlobalFree( t_sound_file_path );
						}

						t_sound_file_path = file_name;

						_SendMessageW( g_hWnd_sound_file, WM_SETTEXT, 0, ( LPARAM )t_sound_file_path );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
					else
					{
						GlobalFree( file_name );

						if ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED &&
							 _SendMessageW( g_hWnd_sound_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
						{
							_SendMessageW( g_hWnd_chk_play_sound, BM_SETCHECK, BST_UNCHECKED, 0 );
							_EnableWindow( g_hWnd_sound_file, FALSE );
							_EnableWindow( g_hWnd_load_sound_file, FALSE );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( t_sound_file_path != NULL )
			{
				GlobalFree( t_sound_file_path );
				t_sound_file_path = NULL;
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
