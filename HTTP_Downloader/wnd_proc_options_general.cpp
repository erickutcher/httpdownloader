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

#include "options.h"
#include "lite_comdlg32.h"
#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_winmm.h"
#include "utilities.h"

#include "system_tray.h"
#include "drop_window.h"

#define BTN_TRAY_ICON					1000
#define BTN_MINIMIZE_TO_TRAY			1001
#define BTN_CLOSE_TO_TRAY				1002
#define BTN_START_IN_TRAY				1003
#define BTN_SHOW_NOTIFICATION			1004
#define BTN_SHOW_TRAY_PROGRESS			1005

#define BTN_ALWAYS_ON_TOP				1006

#define BTN_CHECK_FOR_UPDATES_STARTUP	1007

#define BTN_ENABLE_DROP_WINDOW			1008
#define EDIT_DROP_WINDOW_TRANSPARENCY	1009
#define BTN_SHOW_DROP_WINDOW_PROGRESS	1010

#define BTN_PLAY_SOUND					1011
#define EDIT_SOUND_FILE					1012
#define BTN_LOAD_SOUND_FILE				1013
#define BTN_PLAY_SOUND_FILE				1014

#define BTN_PLAY_SOUND_FAIL				1015
#define EDIT_SOUND_FAIL_FILE			1016
#define BTN_LOAD_SOUND_FAIL_FILE		1017
#define BTN_PLAY_SOUND_FAIL_FILE		1018

// General Tab
HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize_to_tray = NULL;
HWND g_hWnd_chk_close_to_tray = NULL;
HWND g_hWnd_chk_start_in_tray = NULL;
HWND g_hWnd_chk_show_notification = NULL;

HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_chk_check_for_updates_startup = NULL;
HWND g_hWnd_static_drop_window_transparency = NULL;
HWND g_hWnd_drop_window_transparency = NULL;
HWND g_hWnd_ud_drop_window_transparency = NULL;
HWND g_hWnd_chk_enable_drop_window = NULL;

HWND g_hWnd_static_general_hoz1 = NULL;

HWND g_hWnd_static_play_sound = NULL;

HWND g_hWnd_chk_play_sound = NULL;
HWND g_hWnd_sound_file = NULL;
HWND g_hWnd_load_sound_file = NULL;
HWND g_hWnd_play_sound_file = NULL;

HWND g_hWnd_chk_play_sound_fail = NULL;
HWND g_hWnd_sound_fail_file = NULL;
HWND g_hWnd_load_sound_fail_file = NULL;
HWND g_hWnd_play_sound_fail_file = NULL;

HWND g_hWnd_chk_show_tray_progress = NULL;
HWND g_hWnd_chk_show_drop_window_progress = NULL;

wchar_t *t_sound_file_path = NULL;
wchar_t *t_sound_fail_file_path = NULL;

bool g_t_tray_icon = false;	// Let's the Web Server options know we can display notifications.

int general_spinner_width = 0;
int general_spinner_height = 0;

LRESULT CALLBACK GeneralTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_tray_icon = _CreateWindowW( WC_BUTTON, ST_V_Enable_System_Tray_icon_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TRAY_ICON, NULL, NULL );
			g_hWnd_chk_minimize_to_tray = _CreateWindowW( WC_BUTTON, ST_V_Minimize_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_MINIMIZE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_close_to_tray = _CreateWindowW( WC_BUTTON, ST_V_Close_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CLOSE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_start_in_tray = _CreateWindowW( WC_BUTTON, ST_V_Start_in_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_START_IN_TRAY, NULL, NULL );
			g_hWnd_chk_show_notification = _CreateWindowW( WC_BUTTON, ST_V_Show_notification_when_downloads_finish, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SHOW_NOTIFICATION, NULL, NULL );
			g_hWnd_chk_show_tray_progress = _CreateWindowW( WC_BUTTON, ST_V_Show_progress_bar, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SHOW_TRAY_PROGRESS, NULL, NULL );

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_V_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );
			g_hWnd_chk_check_for_updates_startup = _CreateWindowW( WC_BUTTON, ST_V_Check_for_updates_upon_startup, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CHECK_FOR_UPDATES_STARTUP, NULL, NULL );

			g_hWnd_chk_enable_drop_window = _CreateWindowW( WC_BUTTON, ST_V_Enable_URL_drop_window_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ENABLE_DROP_WINDOW, NULL, NULL );

			g_hWnd_static_drop_window_transparency = _CreateWindowW( WC_STATIC, ST_V_Transparency_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that the spinner control can size itself.
			g_hWnd_drop_window_transparency = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 60, 23, hWnd, ( HMENU )EDIT_DROP_WINDOW_TRANSPARENCY, NULL, NULL );

			g_hWnd_ud_drop_window_transparency = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_drop_window_transparency, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETBUDDY, ( WPARAM )g_hWnd_drop_window_transparency, 0 );
			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_drop_window_transparency, UDM_SETRANGE32, 0, 255 );

			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_drop_window_transparency, &rc_spinner );
			general_spinner_width = rc_spinner.right - rc_spinner.left;
			general_spinner_height = rc_spinner.bottom - rc_spinner.top;

			g_hWnd_chk_show_drop_window_progress = _CreateWindowW( WC_BUTTON, ST_V_Show_progress_bar, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SHOW_DROP_WINDOW_PROGRESS, NULL, NULL );

			g_hWnd_static_general_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_play_sound = _CreateWindowW( WC_BUTTON, ST_V_Play_sound_when_downloads_finish, BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_play_sound = _CreateWindowW( WC_BUTTON, ST_V_Success_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_PLAY_SOUND, NULL, NULL );
			g_hWnd_sound_file = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_sound_file_path, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_load_sound_file = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_LOAD_SOUND_FILE, NULL, NULL );
			g_hWnd_play_sound_file = _CreateWindowW( WC_BUTTON, ST_V_PLAY, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_PLAY_SOUND_FILE, NULL, NULL );

			g_hWnd_chk_play_sound_fail = _CreateWindowW( WC_BUTTON, ST_V_Failure_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_PLAY_SOUND_FAIL, NULL, NULL );
			g_hWnd_sound_fail_file = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_sound_fail_file_path, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_load_sound_fail_file = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_LOAD_SOUND_FAIL_FILE, NULL, NULL );
			g_hWnd_play_sound_fail_file = _CreateWindowW( WC_BUTTON, ST_V_PLAY, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_PLAY_SOUND_FAIL_FILE, NULL, NULL );


			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_minimize_to_tray, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_close_to_tray, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_start_in_tray, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_show_notification, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_show_tray_progress, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_check_for_updates_startup, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_enable_drop_window, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_drop_window_transparency, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_drop_window_transparency, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_show_drop_window_progress, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_play_sound, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_play_sound, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_sound_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_load_sound_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_play_sound_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_play_sound_fail, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_sound_fail_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_load_sound_fail_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_play_sound_fail_file, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			// Set settings.

			if ( cfg_tray_icon )
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize_to_tray, TRUE );
				_EnableWindow( g_hWnd_chk_close_to_tray, TRUE );
				_EnableWindow( g_hWnd_chk_start_in_tray, TRUE );
				_EnableWindow( g_hWnd_chk_show_notification, TRUE );
				_EnableWindow( g_hWnd_chk_show_tray_progress, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize_to_tray, FALSE );
				_EnableWindow( g_hWnd_chk_close_to_tray, FALSE );
				_EnableWindow( g_hWnd_chk_start_in_tray, FALSE );
				_EnableWindow( g_hWnd_chk_show_notification, FALSE );
				_EnableWindow( g_hWnd_chk_show_tray_progress, FALSE );
			}
			g_t_tray_icon = cfg_tray_icon;

			_SendMessageW( g_hWnd_chk_minimize_to_tray, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_close_to_tray, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_start_in_tray, BM_SETCHECK, ( cfg_start_in_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_show_notification, BM_SETCHECK, ( cfg_show_notification ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_show_tray_progress, BM_SETCHECK, ( cfg_show_tray_progress ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_check_for_updates_startup, BM_SETCHECK, ( cfg_check_for_updates ? BST_CHECKED : BST_UNCHECKED ), 0 );

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
				_EnableWindow( g_hWnd_play_sound_file, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_play_sound, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_sound_file, FALSE );
				_EnableWindow( g_hWnd_load_sound_file, FALSE );
				_EnableWindow( g_hWnd_play_sound_file, FALSE );
			}

			if ( cfg_play_sound_fail )
			{
				_SendMessageW( g_hWnd_chk_play_sound_fail, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_sound_fail_file, TRUE );
				_EnableWindow( g_hWnd_load_sound_fail_file, TRUE );
				_EnableWindow( g_hWnd_play_sound_fail_file, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_play_sound_fail, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_sound_fail_file, FALSE );
				_EnableWindow( g_hWnd_load_sound_fail_file, FALSE );
				_EnableWindow( g_hWnd_play_sound_fail_file, FALSE );
			}

			if ( cfg_sound_file_path != NULL )
			{
				t_sound_file_path = GlobalStrDupW( cfg_sound_file_path );
			}

			if ( cfg_sound_fail_file_path != NULL )
			{
				t_sound_fail_file_path = GlobalStrDupW( cfg_sound_fail_file_path );
			}

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			int spinner_width = _SCALE_O_( general_spinner_width );
			int spinner_height = _SCALE_O_( general_spinner_height );

			HDWP hdwp = _BeginDeferWindowPos( 23 );
			_DeferWindowPos( hdwp, g_hWnd_chk_tray_icon, HWND_TOP, 0, 0, rc.right, _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_minimize_to_tray, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 20 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_close_to_tray, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 40 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_start_in_tray, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 60 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_show_notification, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 80 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_show_tray_progress, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 100 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_always_on_top, HWND_TOP, 0, _SCALE_O_( 120 ), rc.right, _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_check_for_updates_startup, HWND_TOP, 0, _SCALE_O_( 140 ), rc.right, _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_enable_drop_window, HWND_TOP, 0, _SCALE_O_( 160 ), rc.right, _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_drop_window_transparency, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 184 ), _SCALE_O_( 100 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_drop_window_transparency, HWND_TOP, _SCALE_O_( 115 ), _SCALE_O_( 180 ), _SCALE_O_( 60 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_drop_window_transparency, HWND_TOP, _SCALE_O_( 175 ), _SCALE_O_( 180 ), spinner_width, spinner_height, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_show_drop_window_progress, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 203 ), rc.right - _SCALE_O_( 15 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_general_hoz1, HWND_TOP, 0, _SCALE_O_( 229 ), rc.right, _SCALE_O_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_play_sound, HWND_TOP, 0, _SCALE_O_( 237 ), rc.right, _SCALE_O_( 119 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_play_sound, HWND_TOP, _SCALE_O_( 11 ), _SCALE_O_( 254 ), rc.right - _SCALE_O_( 22 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sound_file, HWND_TOP, _SCALE_O_( 11 ), _SCALE_O_( 274 ), rc.right - _SCALE_O_( 101 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_load_sound_file, HWND_TOP, rc.right - _SCALE_O_( 85 ), _SCALE_O_( 274 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_play_sound_file, HWND_TOP, rc.right - _SCALE_O_( 45 ), _SCALE_O_( 274 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_play_sound_fail, HWND_TOP, _SCALE_O_( 11 ), _SCALE_O_( 302 ), rc.right - _SCALE_O_( 22 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_sound_fail_file, HWND_TOP, _SCALE_O_( 11 ), _SCALE_O_( 322 ), rc.right - _SCALE_O_( 101 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_load_sound_fail_file, HWND_TOP, rc.right - _SCALE_O_( 85 ), _SCALE_O_( 322 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_play_sound_fail_file, HWND_TOP, rc.right - _SCALE_O_( 45 ), _SCALE_O_( 322 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_options;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_TRAY_ICON:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_minimize_to_tray, enable );
					_EnableWindow( g_hWnd_chk_close_to_tray, enable );
					_EnableWindow( g_hWnd_chk_start_in_tray, enable );
					_EnableWindow( g_hWnd_chk_show_notification, enable );
					_EnableWindow( g_hWnd_chk_show_tray_progress, enable );

					g_t_tray_icon = ( enable != FALSE ? true : false );

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_ENABLE_DROP_WINDOW:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_enable_drop_window, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_static_drop_window_transparency, enable );
					_EnableWindow( g_hWnd_drop_window_transparency, enable );
					_EnableWindow( g_hWnd_ud_drop_window_transparency, enable );
					_EnableWindow( g_hWnd_chk_show_drop_window_progress, enable );

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case EDIT_DROP_WINDOW_TRANSPARENCY:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

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
							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
					}
				}
				break;

				case BTN_MINIMIZE_TO_TRAY:
				case BTN_CLOSE_TO_TRAY:
				case BTN_START_IN_TRAY:
				case BTN_SHOW_NOTIFICATION:
				case BTN_SHOW_TRAY_PROGRESS:
				case BTN_ALWAYS_ON_TOP:
				case BTN_CHECK_FOR_UPDATES_STARTUP:
				case BTN_SHOW_DROP_WINDOW_PROGRESS:
				{
					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_PLAY_SOUND:
				case BTN_PLAY_SOUND_FAIL:
				{
					bool show_dialog = false;

					if ( LOWORD( wParam ) == BTN_PLAY_SOUND )
					{
						if ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						{
							if ( _SendMessageW( g_hWnd_sound_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
							{
								show_dialog = true;
							}

							_EnableWindow( g_hWnd_sound_file, TRUE );
							_EnableWindow( g_hWnd_load_sound_file, TRUE );
							_EnableWindow( g_hWnd_play_sound_file, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_sound_file, FALSE );
							_EnableWindow( g_hWnd_load_sound_file, FALSE );
							_EnableWindow( g_hWnd_play_sound_file, FALSE );
						}
					}
					else// if ( LOWORD( wParam ) == BTN_PLAY_SOUND_FAIL )
					{
						if ( _SendMessageW( g_hWnd_chk_play_sound_fail, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
						{
							if ( _SendMessageW( g_hWnd_sound_fail_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
							{
								show_dialog = true;
							}

							_EnableWindow( g_hWnd_sound_fail_file, TRUE );
							_EnableWindow( g_hWnd_load_sound_fail_file, TRUE );
							_EnableWindow( g_hWnd_play_sound_fail_file, TRUE );
						}
						else
						{
							_EnableWindow( g_hWnd_sound_fail_file, FALSE );
							_EnableWindow( g_hWnd_load_sound_fail_file, FALSE );
							_EnableWindow( g_hWnd_play_sound_fail_file, FALSE );
						}
					}

					// Fall through if we haven't chosen a file yet.
					if ( !show_dialog )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );

						break;
					}
				}

				case BTN_LOAD_SOUND_FILE:
				case BTN_LOAD_SOUND_FAIL_FILE:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					wchar_t filter[ 256 ];
					int filter_length = min( ST_L_WAV, ( 256 - 41 ) );
					_wmemcpy_s( filter, 256, ST_V_WAV, filter_length );
					_wmemcpy_s( filter + filter_length, 256 - filter_length, L" (*.WAV;*.WAVE)\0*.wav;*.wave\0", 29 );
					int filter_offset = filter_length + 29;
					filter_length = min( ST_L_All_Files, ( 256 - 12 - filter_offset ) );
					_wmemcpy_s( filter + filter_offset, 256 - filter_offset, ST_V_All_Files, filter_length );
					_wmemcpy_s( filter + filter_offset + filter_length, 256 - filter_offset - filter_length, L" (*.*)\0*.*\0\0", 12 );

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = filter;
					ofn.lpstrTitle = ST_V_Load_Download_Finish_Sound_File;
					ofn.lpstrFile = file_name;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY;

					if ( _GetOpenFileNameW( &ofn ) )
					{
						if ( LOWORD( wParam ) == BTN_LOAD_SOUND_FILE || LOWORD( wParam ) == BTN_PLAY_SOUND )
						{
							if ( t_sound_file_path != NULL )
							{
								GlobalFree( t_sound_file_path );
							}

							t_sound_file_path = file_name;

							_SendMessageW( g_hWnd_sound_file, WM_SETTEXT, 0, ( LPARAM )t_sound_file_path );
						}
						else// if ( LOWORD( wParam ) == BTN_LOAD_SOUND_FAIL_FILE )
						{
							if ( t_sound_fail_file_path != NULL )
							{
								GlobalFree( t_sound_fail_file_path );
							}

							t_sound_fail_file_path = file_name;

							_SendMessageW( g_hWnd_sound_fail_file, WM_SETTEXT, 0, ( LPARAM )t_sound_fail_file_path );
						}

						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
					else
					{
						GlobalFree( file_name );

						if ( LOWORD( wParam ) == BTN_LOAD_SOUND_FILE || LOWORD( wParam ) == BTN_PLAY_SOUND )
						{
							if ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED &&
								 _SendMessageW( g_hWnd_sound_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
							{
								_SendMessageW( g_hWnd_chk_play_sound, BM_SETCHECK, BST_UNCHECKED, 0 );
								_EnableWindow( g_hWnd_sound_file, FALSE );
								_EnableWindow( g_hWnd_load_sound_file, FALSE );
								_EnableWindow( g_hWnd_play_sound_file, FALSE );
							}
						}
						else// if ( LOWORD( wParam ) == BTN_LOAD_SOUND_FAIL_FILE )
						{
							if ( _SendMessageW( g_hWnd_chk_play_sound_fail, BM_GETCHECK, 0, 0 ) == BST_CHECKED &&
								 _SendMessageW( g_hWnd_sound_fail_file, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
							{
								_SendMessageW( g_hWnd_chk_play_sound_fail, BM_SETCHECK, BST_UNCHECKED, 0 );
								_EnableWindow( g_hWnd_sound_fail_file, FALSE );
								_EnableWindow( g_hWnd_load_sound_fail_file, FALSE );
								_EnableWindow( g_hWnd_play_sound_fail_file, FALSE );
							}
						}
					}
				}
				break;

				case BTN_PLAY_SOUND_FILE:
				case BTN_PLAY_SOUND_FAIL_FILE:
				{
					wchar_t *sound_file = NULL;

					if ( LOWORD( wParam ) == BTN_PLAY_SOUND_FILE )
					{
						sound_file = t_sound_file_path;
					}
					else// if ( LOWORD( wParam ) == BTN_PLAY_SOUND_FAIL_FILE )
					{
						sound_file = t_sound_fail_file_path;
					}

					if ( sound_file != NULL )
					{
						bool play = true;
						#ifndef WINMM_USE_STATIC_LIB
							if ( winmm_state == WINMM_STATE_SHUTDOWN )
							{
								play = InitializeWinMM();
							}
						#endif

						if ( play )
						{
							_PlaySoundW( sound_file, NULL, SND_ASYNC | SND_FILENAME );
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_SAVE_OPTIONS:
		{
			if ( cfg_sound_file_path != NULL )
			{
				GlobalFree( cfg_sound_file_path );
			}

			cfg_sound_file_path = GlobalStrDupW( t_sound_file_path );

			if ( cfg_sound_fail_file_path != NULL )
			{
				GlobalFree( cfg_sound_fail_file_path );
			}

			cfg_sound_fail_file_path = GlobalStrDupW( t_sound_fail_file_path );

			cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize_to_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close_to_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_start_in_tray = ( _SendMessageW( g_hWnd_chk_start_in_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_show_notification = ( _SendMessageW( g_hWnd_chk_show_notification, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			cfg_show_tray_progress = ( _SendMessageW( g_hWnd_chk_show_tray_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			// Add the tray icon if it was not previously enabled.
			if ( g_t_tray_icon )
			{
				if ( !cfg_tray_icon )
				{
					InitializeSystemTray( g_hWnd_main );
				}
				else	// Tray icon is still enabled.
				{
					if ( !cfg_show_tray_progress )
					{
						g_nid.uFlags &= ~NIF_INFO;
						g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
						g_nid.hIcon = g_default_tray_icon;
						_Shell_NotifyIconW( NIM_MODIFY, &g_nid );

						UninitializeIconValues();
					}
				}

				if ( cfg_show_tray_progress )
				{
					InitializeIconValues( g_hWnd_main );
				}
			}
			else if ( !g_t_tray_icon && cfg_tray_icon )	// Remove the tray icon if it was previously enabled.
			{
				// Make sure that the main window is not hidden before we delete the tray icon.
				if ( _IsWindowVisible( g_hWnd_main ) == FALSE )
				{
					_ShowWindow( g_hWnd_main, SW_SHOWNOACTIVATE );
				}

				_Shell_NotifyIconW( NIM_DELETE, &g_nid );

				UninitializeIconValues();
			}

			cfg_tray_icon = g_t_tray_icon;// = tray_icon;

			bool always_on_top = ( _SendMessageW( g_hWnd_chk_always_on_top, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			// Set any active windows if we've changed the extended style.
			if ( always_on_top != cfg_always_on_top )
			{
				cfg_always_on_top = always_on_top;

				if ( g_hWnd_main != NULL ){ _SetWindowPos( g_hWnd_main, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_add_urls != NULL ){ _SetWindowPos( g_hWnd_add_urls, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_update_download != NULL ){ _SetWindowPos( g_hWnd_update_download, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_search != NULL ){ _SetWindowPos( g_hWnd_search, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_download_speed_limit != NULL ){ _SetWindowPos( g_hWnd_download_speed_limit, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_add_category != NULL ){ _SetWindowPos( g_hWnd_add_category, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_check_for_updates != NULL ){ _SetWindowPos( g_hWnd_check_for_updates, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_site_manager != NULL ){ _SetWindowPos( g_hWnd_site_manager, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
				if ( g_hWnd_options != NULL ){ _SetWindowPos( g_hWnd_options, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
			}

			cfg_check_for_updates = ( _SendMessageW( g_hWnd_chk_check_for_updates_startup, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			cfg_enable_drop_window = ( _SendMessageW( g_hWnd_chk_enable_drop_window, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			char value[ 4 ];

			_SendMessageA( g_hWnd_drop_window_transparency, WM_GETTEXT, 4, ( LPARAM )value );
			cfg_drop_window_transparency = ( unsigned char )_strtoul( value, NULL, 10 );

			cfg_show_drop_window_progress = ( _SendMessageW( g_hWnd_chk_show_drop_window_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			if ( cfg_enable_drop_window )
			{
				if ( g_hWnd_url_drop_window == NULL )
				{
					g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"class_url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP, 0, 0, DW_WIDTH, DW_HEIGHT, NULL, NULL, NULL, NULL );
					_SetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
					_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

					// Prevents it from stealing focus.
					_SetWindowPos( g_hWnd_url_drop_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER );
				}
				else
				{
					_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

					if ( !cfg_show_drop_window_progress )
					{
						UpdateDropWindow( 0, 0, 0, 0, false );
					}
				}
			}
			else
			{
				if ( g_hWnd_url_drop_window != NULL )
				{
					_DestroyWindow( g_hWnd_url_drop_window );
				}
			}

			cfg_play_sound = ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_play_sound_fail = ( _SendMessageW( g_hWnd_chk_play_sound_fail, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			if ( cfg_play_sound || cfg_play_sound_fail )
			{
				#ifndef WINMM_USE_STATIC_LIB
					if ( winmm_state == WINMM_STATE_SHUTDOWN )
					{
						InitializeWinMM();
					}
				#endif
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

			if ( t_sound_fail_file_path != NULL )
			{
				GlobalFree( t_sound_fail_file_path );
				t_sound_fail_file_path = NULL;
			}

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
