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
#include "utilities.h"
#include "folder_browser.h"
#include "lite_gdi32.h"

#include "cmessagebox.h"

#define BTN_DOWNLOAD_HISTORY		1000
#define BTN_QUICK_ALLOCATION		1001
#define BTN_SPARSE_FILE_ALLOCATION	1002
#define BTN_SET_FILETIME			1003
#define BTN_UPDATE_REDIRECTED		1004
#define BTN_APPLY_INITIAL_PROXY		1005
#define BTN_DOWNLOAD_NON_200_206	1006
#define BTN_USE_ONE_INSTANCE		1007
#define BTN_PREVENT_STANDBY			1008
#define BTN_RESUME_DOWNLOADS		1009
#define BTN_MOVE_TO_TRASH			1010
#define BTN_OVERRIDE_LIST_PROMPTS	1011
#define BTN_CATEGORY_MOVE			1012

#define CB_DRAG_AND_DROP_ACTION		1013

#define CB_PROMPT_LAST_MODIFIED		1014
#define CB_PROMPT_RENAME			1015
#define CB_PROMPT_FILE_SIZE			1016
#define EDIT_MAX_FILE_SIZE			1017

#define CB_SHUTDOWN_ACTION			1018

#define BTN_DEFAULT_DOWNLOAD_DIRECTORY	1019

#define BTN_USE_TEMP_DOWNLOAD_DIRECTORY	1020
#define BTN_TEMP_DOWNLOAD_DIRECTORY		1021

#define EDIT_THREAD_COUNT			1022

// Advanced Tab
HWND g_hWnd_static_drag_and_drop_action = NULL;
HWND g_hWnd_static_advanced_hoz1 = NULL;
HWND g_hWnd_static_prompt_last_modified = NULL;
HWND g_hWnd_static_prompt_rename = NULL;
HWND g_hWnd_static_prompt_file_size = NULL;
HWND g_hWnd_static_shutdown_action = NULL;
HWND g_hWnd_static_advanced_hoz2 = NULL;
HWND g_hWnd_static_default_download_directory = NULL;
HWND g_hWnd_static_advanced_hoz3 = NULL;
HWND g_hWnd_static_thread_count = NULL;

HWND g_hWnd_chk_download_history = NULL;
HWND g_hWnd_chk_quick_allocation = NULL;
HWND g_hWnd_chk_sparse_file_allocation = NULL;
HWND g_hWnd_chk_set_filetime = NULL;
HWND g_hWnd_chk_update_redirected = NULL;
HWND g_hWnd_chk_apply_initial_proxy = NULL;
HWND g_hWnd_chk_download_non_200_206 = NULL;
HWND g_hWnd_chk_use_one_instance = NULL;
HWND g_hWnd_chk_prevent_standby = NULL;
HWND g_hWnd_chk_resume_downloads = NULL;
HWND g_hWnd_chk_move_to_trash = NULL;
HWND g_hWnd_chk_override_list_prompts = NULL;
HWND g_hWnd_chk_category_move = NULL;

HWND g_hWnd_drag_and_drop_action = NULL;

HWND g_hWnd_prompt_last_modified = NULL;
HWND g_hWnd_prompt_rename = NULL;
HWND g_hWnd_max_file_size = NULL;
HWND g_hWnd_prompt_file_size = NULL;

HWND g_hWnd_shutdown_action = NULL;

HWND g_hWnd_default_download_directory = NULL;
HWND g_hWnd_btn_default_download_directory = NULL;

HWND g_hWnd_chk_temp_download_directory = NULL;
HWND g_hWnd_temp_download_directory = NULL;
HWND g_hWnd_btn_temp_download_directory = NULL;

HWND g_hWnd_thread_count = NULL;
HWND g_hWnd_ud_thread_count = NULL;

wchar_t *t_default_download_directory = NULL;
wchar_t *t_temp_download_directory = NULL;

wchar_t file_size_tooltip_text[ 32 ];
HWND g_hWnd_file_size_tooltip = NULL;

int advanced_spinner_width = 0;
int advanced_spinner_height = 0;

LRESULT CALLBACK AdvancedTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_download_history = _CreateWindowW( WC_BUTTON, ST_V_Enable_download_history, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD_HISTORY, NULL, NULL );
			g_hWnd_chk_quick_allocation = _CreateWindowW( WC_BUTTON, ST_V_Enable_quick_file_allocation, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_QUICK_ALLOCATION, NULL, NULL );
			g_hWnd_chk_sparse_file_allocation = _CreateWindowW( WC_BUTTON, ST_V_Enable_sparse_file_allocation, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SPARSE_FILE_ALLOCATION, NULL, NULL );
			g_hWnd_chk_set_filetime = _CreateWindowW( WC_BUTTON, ST_V_Set_date_and_time_of_file, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SET_FILETIME, NULL, NULL );
			g_hWnd_chk_update_redirected = _CreateWindowW( WC_BUTTON, ST_V_Update_redirected_URL_s__in_download_list, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_UPDATE_REDIRECTED, NULL, NULL );
			g_hWnd_chk_apply_initial_proxy = _CreateWindowW( WC_BUTTON, ST_V_Apply_initially_set_proxy, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_APPLY_INITIAL_PROXY, NULL, NULL );
			g_hWnd_chk_download_non_200_206 = _CreateWindowW( WC_BUTTON, ST_V_Download_non_200_and_non_206_responses, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD_NON_200_206, NULL, NULL );
			g_hWnd_chk_use_one_instance = _CreateWindowW( WC_BUTTON, ST_V_Allow_only_one_instance, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_USE_ONE_INSTANCE, NULL, NULL );
			g_hWnd_chk_prevent_standby = _CreateWindowW( WC_BUTTON, ST_V_Prevent_system_standby, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_PREVENT_STANDBY, NULL, NULL );
			g_hWnd_chk_resume_downloads = _CreateWindowW( WC_BUTTON, ST_V_Resume_previously_downloading, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_RESUME_DOWNLOADS, NULL, NULL );
			g_hWnd_chk_move_to_trash = _CreateWindowW( WC_BUTTON, ST_V_Move_deleted_downloads_to_Recycle_Bin, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_MOVE_TO_TRASH, NULL, NULL );
			g_hWnd_chk_override_list_prompts = _CreateWindowW( WC_BUTTON, ST_V_Override_download_list_action_prompts, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_OVERRIDE_LIST_PROMPTS, NULL, NULL );
			g_hWnd_chk_category_move = _CreateWindowW( WC_BUTTON, ST_V_Move_files_that_match_category, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_OVERRIDE_LIST_PROMPTS, NULL, NULL );

			g_hWnd_static_advanced_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_drag_and_drop_action = _CreateWindowW( WC_STATIC, ST_V_Drag_and_drop_URL_s__action_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_drag_and_drop_action = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_DRAG_AND_DROP_ACTION, NULL, NULL );
			_SendMessageW( g_hWnd_drag_and_drop_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_None );
			_SendMessageW( g_hWnd_drag_and_drop_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Download_immediately );
			_SendMessageW( g_hWnd_drag_and_drop_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Add_in_Stopped_state );
			_SendMessageW( g_hWnd_drag_and_drop_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Verify );

			_SendMessageW( g_hWnd_drag_and_drop_action, CB_SETCURSEL, cfg_drag_and_drop_action, 0 );


			g_hWnd_static_prompt_last_modified = _CreateWindowW( WC_STATIC, ST_V_When_a_file_has_been_modified_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_prompt_last_modified = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_PROMPT_LAST_MODIFIED, NULL, NULL );
			_SendMessageW( g_hWnd_prompt_last_modified, CB_ADDSTRING, 0, ( LPARAM )ST_V_Display_Prompt );
			_SendMessageW( g_hWnd_prompt_last_modified, CB_ADDSTRING, 0, ( LPARAM )ST_V_Continue_Download );
			_SendMessageW( g_hWnd_prompt_last_modified, CB_ADDSTRING, 0, ( LPARAM )ST_V_Restart_Download );
			_SendMessageW( g_hWnd_prompt_last_modified, CB_ADDSTRING, 0, ( LPARAM )ST_V_Skip_Download );

			_SendMessageW( g_hWnd_prompt_last_modified, CB_SETCURSEL, cfg_prompt_last_modified, 0 );


			g_hWnd_static_prompt_rename = _CreateWindowW( WC_STATIC, ST_V_When_a_file_already_exists_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_prompt_rename = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_PROMPT_RENAME, NULL, NULL );
			_SendMessageW( g_hWnd_prompt_rename, CB_ADDSTRING, 0, ( LPARAM )ST_V_Display_Prompt );
			_SendMessageW( g_hWnd_prompt_rename, CB_ADDSTRING, 0, ( LPARAM )ST_V_Rename_File );
			_SendMessageW( g_hWnd_prompt_rename, CB_ADDSTRING, 0, ( LPARAM )ST_V_Overwrite_File );
			_SendMessageW( g_hWnd_prompt_rename, CB_ADDSTRING, 0, ( LPARAM )ST_V_Skip_Download );

			_SendMessageW( g_hWnd_prompt_rename, CB_SETCURSEL, cfg_prompt_rename, 0 );


			g_hWnd_static_prompt_file_size = _CreateWindowW( WC_STATIC, ST_V_When_a_file_is_greater_than_or_equal_to_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_max_file_size = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_MAX_FILE_SIZE, NULL, NULL );

			_SendMessageW( g_hWnd_max_file_size, EM_LIMITTEXT, 20, 0 );

			g_hWnd_file_size_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			file_size_tooltip_text[ 0 ] = 0;

			TOOLINFO ti;
			_memzero( &ti, sizeof( TOOLINFO ) );
			ti.cbSize = sizeof( TOOLINFO );
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = g_hWnd_max_file_size;
			ti.lpszText = file_size_tooltip_text;

			_GetClientRect( hWnd, &ti.rect );
			_SendMessageW( g_hWnd_file_size_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&ti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_max_file_size );
			_SendMessageA( g_hWnd_max_file_size, WM_SETTEXT, 0, ( LPARAM )value );


			g_hWnd_prompt_file_size = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_PROMPT_FILE_SIZE, NULL, NULL );
			_SendMessageW( g_hWnd_prompt_file_size, CB_ADDSTRING, 0, ( LPARAM )ST_V_Display_Prompt );
			_SendMessageW( g_hWnd_prompt_file_size, CB_ADDSTRING, 0, ( LPARAM )ST_V_Continue_Download );
			_SendMessageW( g_hWnd_prompt_file_size, CB_ADDSTRING, 0, ( LPARAM )ST_V_Skip_Download );

			_SendMessageW( g_hWnd_prompt_file_size, CB_SETCURSEL, cfg_prompt_file_size, 0 );


			g_hWnd_static_shutdown_action = _CreateWindowW( WC_STATIC, ST_V_System_shutdown_action_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_shutdown_action = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_SHUTDOWN_ACTION, NULL, NULL );

			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_None );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Exit_program );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Log_off );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Lock );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Restart_system );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Sleep );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Hibernate );
			_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Shut_down );
			if ( g_is_windows_8_or_higher )
			{
				_SendMessageW( g_hWnd_shutdown_action, CB_ADDSTRING, 0, ( LPARAM )ST_V_Hybrid_shut_down );
			}

			_SendMessageW( g_hWnd_shutdown_action, CB_SETCURSEL, cfg_shutdown_action, 0 );


			g_hWnd_static_advanced_hoz2 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );


			g_hWnd_static_default_download_directory = _CreateWindowW( WC_STATIC, ST_V_Default_download_directory_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_default_download_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DEFAULT_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_chk_temp_download_directory = _CreateWindowW( WC_BUTTON, ST_V_Use_temporary_download_directory_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_USE_TEMP_DOWNLOAD_DIRECTORY, NULL, NULL );
			g_hWnd_temp_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_temp_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_temp_download_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TEMP_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_static_advanced_hoz3 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );


			g_hWnd_static_thread_count = _CreateWindowW( WC_STATIC, ST_V_Thread_pool_count_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that the spinner control can size itself.
			g_hWnd_thread_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 100, 23, hWnd, ( HMENU )EDIT_THREAD_COUNT, NULL, NULL );
			g_hWnd_ud_thread_count = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_thread_count, EM_LIMITTEXT, 10, 0 );
			_SendMessageW( g_hWnd_ud_thread_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_thread_count, 0 );
			_SendMessageW( g_hWnd_ud_thread_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_thread_count, UDM_SETRANGE32, 1, g_max_threads );
			_SendMessageW( g_hWnd_ud_thread_count, UDM_SETPOS, 0, cfg_thread_count );


			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_thread_count, &rc_spinner );
			advanced_spinner_width = rc_spinner.right - rc_spinner.left;
			advanced_spinner_height = rc_spinner.bottom - rc_spinner.top;


			SCROLLINFO si;
			_memzero( &si, sizeof( SCROLLINFO ) );
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = _SCALE_O_( 565 );	// Bottom of the last item in the window.
			si.nPage = rc.bottom - rc.top;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );


			_SendMessageW( g_hWnd_chk_download_history, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_quick_allocation, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_sparse_file_allocation, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_set_filetime, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_update_redirected, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_apply_initial_proxy, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_download_non_200_206, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_use_one_instance, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_prevent_standby, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_resume_downloads, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_move_to_trash, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_override_list_prompts, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_category_move, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_drag_and_drop_action, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_drag_and_drop_action, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_prompt_last_modified, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_prompt_last_modified, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_prompt_rename, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_prompt_rename, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_prompt_file_size, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_max_file_size, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_prompt_file_size, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_shutdown_action, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_shutdown_action, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_default_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_default_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_btn_default_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_temp_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_temp_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_btn_temp_download_directory, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_thread_count, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_thread_count, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			//

			if ( FocusCBProc == NULL )
			{
				FocusCBProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_chk_download_history, GWLP_WNDPROC );
			}
			_SetWindowLongPtrW( g_hWnd_chk_download_history, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_quick_allocation, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_sparse_file_allocation, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_set_filetime, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_update_redirected, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_apply_initial_proxy, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_download_non_200_206, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_use_one_instance, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_prevent_standby, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_resume_downloads, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_move_to_trash, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_override_list_prompts, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_category_move, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );
			_SetWindowLongPtrW( g_hWnd_chk_temp_download_directory, GWLP_WNDPROC, ( LONG_PTR )FocusCBSubProc );

			if ( FocusComboProc == NULL )
			{
				FocusComboProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_drag_and_drop_action, GWLP_WNDPROC );
			}
			_SetWindowLongPtrW( g_hWnd_drag_and_drop_action, GWLP_WNDPROC, ( LONG_PTR )FocusComboSubProc );
			_SetWindowLongPtrW( g_hWnd_prompt_last_modified, GWLP_WNDPROC, ( LONG_PTR )FocusComboSubProc );
			_SetWindowLongPtrW( g_hWnd_prompt_rename, GWLP_WNDPROC, ( LONG_PTR )FocusComboSubProc );
			_SetWindowLongPtrW( g_hWnd_prompt_file_size, GWLP_WNDPROC, ( LONG_PTR )FocusComboSubProc );
			_SetWindowLongPtrW( g_hWnd_shutdown_action, GWLP_WNDPROC, ( LONG_PTR )FocusComboSubProc );

			if ( FocusEditProc == NULL )
			{
				FocusEditProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_max_file_size, GWLP_WNDPROC );
			}
			_SetWindowLongPtrW( g_hWnd_max_file_size, GWLP_WNDPROC, ( LONG_PTR )FocusEditSubProc );
			//_SetWindowLongPtrW( g_hWnd_default_download_directory, GWLP_WNDPROC, ( LONG_PTR )FocusEditSubProc );
			//_SetWindowLongPtrW( g_hWnd_temp_download_directory, GWLP_WNDPROC, ( LONG_PTR )FocusEditSubProc );
			_SetWindowLongPtrW( g_hWnd_thread_count, GWLP_WNDPROC, ( LONG_PTR )FocusEditSubProc );

			if ( FocusBtnProc == NULL )
			{
				FocusBtnProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_btn_default_download_directory, GWLP_WNDPROC );
			}
			_SetWindowLongPtrW( g_hWnd_btn_default_download_directory, GWLP_WNDPROC, ( LONG_PTR )FocusBtnSubProc );
			_SetWindowLongPtrW( g_hWnd_btn_temp_download_directory, GWLP_WNDPROC, ( LONG_PTR )FocusBtnSubProc );

			//

			_SendMessageW( g_hWnd_chk_download_history, BM_SETCHECK, ( cfg_enable_download_history ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_quick_allocation, BM_SETCHECK, ( cfg_enable_quick_allocation ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_sparse_file_allocation, BM_SETCHECK, ( cfg_enable_sparse_file_allocation ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_EnableWindow( g_hWnd_chk_sparse_file_allocation, ( cfg_enable_quick_allocation ? FALSE : TRUE ) );
			_SendMessageW( g_hWnd_chk_set_filetime, BM_SETCHECK, ( cfg_set_filetime ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_update_redirected, BM_SETCHECK, ( cfg_update_redirected ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_apply_initial_proxy, BM_SETCHECK, ( cfg_apply_initial_proxy ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_download_non_200_206, BM_SETCHECK, ( cfg_download_non_200_206 ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_use_one_instance, BM_SETCHECK, ( cfg_use_one_instance ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_prevent_standby, BM_SETCHECK, ( cfg_prevent_standby ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_resume_downloads, BM_SETCHECK, ( cfg_resume_downloads ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_move_to_trash, BM_SETCHECK, ( cfg_move_to_trash ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_override_list_prompts, BM_SETCHECK, ( cfg_override_list_prompts ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_category_move, BM_SETCHECK, ( cfg_category_move ? BST_CHECKED : BST_UNCHECKED ), 0 );

			if ( cfg_use_temp_download_directory )
			{
				_SendMessageW( g_hWnd_chk_temp_download_directory, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_temp_download_directory, TRUE );
				_EnableWindow( g_hWnd_btn_temp_download_directory, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_temp_download_directory, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_temp_download_directory, FALSE );
				_EnableWindow( g_hWnd_btn_temp_download_directory, FALSE );
			}

			if ( cfg_default_download_directory != NULL )
			{
				t_default_download_directory = GlobalStrDupW( cfg_default_download_directory );
			}

			if ( cfg_temp_download_directory != NULL )
			{
				t_temp_download_directory = GlobalStrDupW( cfg_temp_download_directory );
			}

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			int spinner_width = _SCALE_O_( advanced_spinner_width );
			int spinner_height = _SCALE_O_( advanced_spinner_height );

			HDWP hdwp = _BeginDeferWindowPos( 36 );
			_DeferWindowPos( hdwp, g_hWnd_chk_download_history, HWND_TOP, 0, 0, rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_quick_allocation, HWND_TOP, 0, _SCALE_O_( 20 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_sparse_file_allocation, HWND_TOP, 0, _SCALE_O_( 40 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_set_filetime, HWND_TOP, 0, _SCALE_O_( 60 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_update_redirected, HWND_TOP, 0, _SCALE_O_( 80 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_apply_initial_proxy, HWND_TOP, 0, _SCALE_O_( 100 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_download_non_200_206, HWND_TOP, 0, _SCALE_O_( 120 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_use_one_instance, HWND_TOP, 0, _SCALE_O_( 140 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_prevent_standby, HWND_TOP, 0, _SCALE_O_( 160 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_resume_downloads, HWND_TOP, 0, _SCALE_O_( 180 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_move_to_trash, HWND_TOP, 0, _SCALE_O_( 200 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_override_list_prompts, HWND_TOP, 0, _SCALE_O_( 220 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_category_move, HWND_TOP, 0, _SCALE_O_( 240 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_advanced_hoz1, HWND_TOP, 0, _SCALE_O_( 266 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_drag_and_drop_action, HWND_TOP, 0, _SCALE_O_( 281 ), rc.right - _SCALE_O_( 165 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_drag_and_drop_action, HWND_TOP, rc.right - _SCALE_O_( 160 ), _SCALE_O_( 277 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_prompt_last_modified, HWND_TOP, 0, _SCALE_O_( 309 ), rc.right - _SCALE_O_( 165 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_prompt_last_modified, HWND_TOP, rc.right - _SCALE_O_( 160 ), _SCALE_O_( 305 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_prompt_rename, HWND_TOP, 0, _SCALE_O_( 337 ), rc.right - _SCALE_O_( 165 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_prompt_rename, HWND_TOP, rc.right - _SCALE_O_( 160 ), _SCALE_O_( 333 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_prompt_file_size, HWND_TOP, 0, _SCALE_O_( 365 ), rc.right - _SCALE_O_( 270 ), _SCALE_O_( 17 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_max_file_size, HWND_TOP, rc.right - _SCALE_O_( 265 ), _SCALE_O_( 361 ), _SCALE_O_( 100 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_prompt_file_size, HWND_TOP, rc.right - _SCALE_O_( 160 ), _SCALE_O_( 361 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_shutdown_action, HWND_TOP, 0, _SCALE_O_( 393 ), rc.right - _SCALE_O_( 165 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_shutdown_action, HWND_TOP, rc.right - _SCALE_O_( 160 ), _SCALE_O_( 389 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_advanced_hoz2, HWND_TOP, 0, _SCALE_O_( 422 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_default_download_directory, HWND_TOP, 0, _SCALE_O_( 430 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_default_download_directory, HWND_TOP, 0, _SCALE_O_( 448 ), rc.right - _SCALE_O_( 50 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_default_download_directory, HWND_TOP, rc.right - _SCALE_O_( 45 ), _SCALE_O_( 448 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_temp_download_directory, HWND_TOP, 0, _SCALE_O_( 478 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_temp_download_directory, HWND_TOP, 0, _SCALE_O_( 498 ), rc.right - _SCALE_O_( 50 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_temp_download_directory, HWND_TOP, rc.right - _SCALE_O_( 45 ), _SCALE_O_( 498 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_advanced_hoz3, HWND_TOP, 0, _SCALE_O_( 531 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_thread_count, HWND_TOP, 0, _SCALE_O_( 546 ), rc.right - ( _SCALE_O_( 115 ) + spinner_width ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_thread_count, HWND_TOP, rc.right - ( _SCALE_O_( 110 ) + spinner_width ), _SCALE_O_( 542 ), _SCALE_O_( 100 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_thread_count, HWND_TOP, rc.right - ( _SCALE_O_( 10 ) + spinner_width ), _SCALE_O_( 542 ), spinner_width, spinner_height, SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_options;
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			si.fMask |= SIF_RANGE | SIF_PAGE;
			si.nPos = _SCALE2_( si.nPos, dpi_options );
			si.nMin = 0;
			si.nMax = _SCALE_O_( 565 );	// Bottom of the last item in the window.
			si.nPage = rc.bottom - rc.top;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

			_ScrollWindow( hWnd, 0, -si.nPos, NULL, NULL );

			// Return value is ignored.
			return TRUE;
		}
		break;

		case WM_GETDLGCODE:
		{
			if ( wParam == VK_TAB )
			{
				// We're cheating here since we know this will be the first tab item in this window.
				// Normally we'd use: HWND hWnd_next = GetNextDlgTabItem( hWnd, hWnd, TRUE/FALSE );
				if ( ( _GetKeyState( VK_SHIFT ) & 0x8000 ) )
				{
					_SetFocus( g_hWnd_options_tree );
				}
				else
				{
					_SetFocus( g_hWnd_chk_download_history );
				}

				return DLGC_WANTTAB;
			}
			else
			{
				return _DefWindowProcW( hWnd, msg, wParam, lParam );
			}
		}
		break;

		case WM_PROPAGATE:
		{
			_SetFocus( hWnd );

			return 0;
		}
		break;

		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			_SetFocus( hWnd );

			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		{
			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			int delta = si.nPos;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { si.nPos -= _SCALE_O_( 10 ); } break;
					case SB_LINEDOWN: { si.nPos += _SCALE_O_( 10 ); } break;
					case SB_PAGEUP: { si.nPos -= si.nPage; } break;
					case SB_PAGEDOWN: { si.nPos += si.nPage; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { si.nPos = ( int )HIWORD( wParam ); } break;
					case SB_TOP: { si.nPos = 0; } break;
					case SB_BOTTOM: { si.nPos = ( si.nMax - si.nPage ) + 1; } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				si.nPos -= ( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * _SCALE_O_( 20 );
			}

			_SetScrollPos( hWnd, SB_VERT, si.nPos, TRUE );

			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			if ( si.nPos != delta )
			{
				_ScrollWindow( hWnd, 0, delta - si.nPos, NULL, NULL );
			}

			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			switch ( wParam )
			{
				case VK_PRIOR:
				case VK_NEXT:
				case VK_END:
				case VK_HOME:
				{
					SCROLLINFO si;
					si.cbSize = sizeof( SCROLLINFO );
					si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
					_GetScrollInfo( hWnd, SB_VERT, &si );

					int delta = si.nPos;

					switch ( wParam )
					{
						case VK_PRIOR: { si.nPos -= si.nPage; } break;
						case VK_NEXT: { si.nPos += si.nPage; } break;
						case VK_END: { si.nPos = ( si.nMax - si.nPage ) + 1; } break;
						case VK_HOME: { si.nPos = 0; } break;
					}

					_SetScrollPos( hWnd, SB_VERT, si.nPos, TRUE );

					si.fMask = SIF_POS;
					_GetScrollInfo( hWnd, SB_VERT, &si );

					if ( si.nPos != delta )
					{
						_ScrollWindow( hWnd, 0, delta - si.nPos, NULL, NULL );
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_DOWNLOAD_HISTORY:
				case BTN_SPARSE_FILE_ALLOCATION:
				case BTN_SET_FILETIME:
				case BTN_UPDATE_REDIRECTED:
				case BTN_APPLY_INITIAL_PROXY:
				case BTN_DOWNLOAD_NON_200_206:
				case BTN_USE_ONE_INSTANCE:
				case BTN_PREVENT_STANDBY:
				case BTN_RESUME_DOWNLOADS:
				case BTN_MOVE_TO_TRASH:
				case BTN_OVERRIDE_LIST_PROMPTS:
				case BTN_CATEGORY_MOVE:
				{
					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_QUICK_ALLOCATION:
				{
					_EnableWindow( g_hWnd_chk_sparse_file_allocation, ( _SendMessageW( g_hWnd_chk_quick_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? FALSE : TRUE ) );

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case EDIT_MAX_FILE_SIZE:
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
						else if ( num == 0 )
						{
							num = 1;

							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						unsigned int length = FormatSizes( file_size_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
						file_size_tooltip_text[ length ] = 0;

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_max_file_size;
						ti.lpszText = file_size_tooltip_text;
						_SendMessageW( g_hWnd_file_size_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );

						if ( num != cfg_max_file_size )
						{
							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
					}
				}
				break;

				case CB_DRAG_AND_DROP_ACTION:
				case CB_PROMPT_RENAME:
				case CB_PROMPT_FILE_SIZE:
				case CB_PROMPT_LAST_MODIFIED:
				case CB_SHUTDOWN_ACTION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
				}
				break;

				case BTN_DEFAULT_DOWNLOAD_DIRECTORY:
				{
					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_default_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_default_download_directory );
						t_default_download_directory = directory;

						_SendMessageW( g_hWnd_default_download_directory, WM_SETTEXT, 0, ( LPARAM )t_default_download_directory );

						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
				}
				break;

				case BTN_USE_TEMP_DOWNLOAD_DIRECTORY:
				{
					bool show_dialog = false;

					if ( _SendMessageW( g_hWnd_chk_temp_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						if ( _SendMessageW( g_hWnd_temp_download_directory, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
						{
							show_dialog = true;
						}

						_EnableWindow( g_hWnd_temp_download_directory, TRUE );
						_EnableWindow( g_hWnd_btn_temp_download_directory, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_temp_download_directory, FALSE );
						_EnableWindow( g_hWnd_btn_temp_download_directory, FALSE );
					}

					// Fall through if we haven't chosen a file yet.
					if ( !show_dialog )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );

						break;
					}
				}

				case BTN_TEMP_DOWNLOAD_DIRECTORY:
				{
					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_temporary_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_temp_download_directory );
						t_temp_download_directory = directory;

						_SendMessageW( g_hWnd_temp_download_directory, WM_SETTEXT, 0, ( LPARAM )t_temp_download_directory );

						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
					else
					{
						if ( _SendMessageW( g_hWnd_chk_temp_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED &&
							 _SendMessageW( g_hWnd_temp_download_directory, WM_GETTEXTLENGTH, 0, 0 ) == 0 )
						{
							_SendMessageW( g_hWnd_chk_temp_download_directory, BM_SETCHECK, BST_UNCHECKED, 0 );
							_EnableWindow( g_hWnd_temp_download_directory, FALSE );
							_EnableWindow( g_hWnd_btn_temp_download_directory, FALSE );
						}
					}
				}
				break;

				case EDIT_THREAD_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

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
							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
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
			if ( cfg_default_download_directory != NULL )
			{
				GlobalFree( cfg_default_download_directory );
			}

			// We want the length, so don't do GlobalStrDupW.
			g_default_download_directory_length = lstrlenW( t_default_download_directory );
			cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ) );
			_wmemcpy_s( cfg_default_download_directory, g_default_download_directory_length + 1, t_default_download_directory, g_default_download_directory_length );
			*( cfg_default_download_directory + g_default_download_directory_length ) = 0;	// Sanity.

			cfg_enable_download_history = ( _SendMessageW( g_hWnd_chk_download_history, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			bool enable_quick_allocation = ( _SendMessageW( g_hWnd_chk_quick_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			unsigned char display_notice = 0x00;
			if ( enable_quick_allocation != cfg_enable_quick_allocation )
			{
				cfg_enable_quick_allocation = enable_quick_allocation;

				display_notice = ( enable_quick_allocation ? 0x01 : 0x02 );
			}

			cfg_enable_sparse_file_allocation = ( _SendMessageW( g_hWnd_chk_sparse_file_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_set_filetime = ( _SendMessageW( g_hWnd_chk_set_filetime, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_update_redirected = ( _SendMessageW( g_hWnd_chk_update_redirected, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_apply_initial_proxy = ( _SendMessageW( g_hWnd_chk_apply_initial_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_download_non_200_206 = ( _SendMessageW( g_hWnd_chk_download_non_200_206, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_move_to_trash = ( _SendMessageW( g_hWnd_chk_move_to_trash, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_use_one_instance = ( _SendMessageW( g_hWnd_chk_use_one_instance, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_prevent_standby = ( _SendMessageW( g_hWnd_chk_prevent_standby, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_resume_downloads = ( _SendMessageW( g_hWnd_chk_resume_downloads, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_override_list_prompts = ( _SendMessageW( g_hWnd_chk_override_list_prompts, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
			cfg_category_move = ( _SendMessageW( g_hWnd_chk_category_move, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			char value[ 21 ];

			_SendMessageA( g_hWnd_thread_count, WM_GETTEXT, 11, ( LPARAM )value );
			unsigned long thread_count = _strtoul( value, NULL, 10 );

			if ( thread_count != cfg_thread_count )
			{
				cfg_thread_count = thread_count;

				display_notice |= 0x04;
			}

			//

			cfg_drag_and_drop_action = ( unsigned char )_SendMessageW( g_hWnd_drag_and_drop_action, CB_GETCURSEL, 0, 0 );

			cfg_prompt_rename = ( unsigned char )_SendMessageW( g_hWnd_prompt_rename, CB_GETCURSEL, 0, 0 );
			cfg_prompt_file_size = ( unsigned char )_SendMessageW( g_hWnd_prompt_file_size, CB_GETCURSEL, 0, 0 );
			_SendMessageA( g_hWnd_max_file_size, WM_GETTEXT, 21, ( LPARAM )value );
			cfg_max_file_size = strtoull( value );
			cfg_prompt_last_modified = ( unsigned char )_SendMessageW( g_hWnd_prompt_last_modified, CB_GETCURSEL, 0, 0 );

			unsigned char shutdown_action = ( unsigned char )_SendMessageW( g_hWnd_shutdown_action, CB_GETCURSEL, 0, 0 );

			if ( shutdown_action != cfg_shutdown_action )
			{
				cfg_shutdown_action = g_shutdown_action = shutdown_action;

				if ( cfg_shutdown_action == SHUTDOWN_ACTION_RESTART ||
					 cfg_shutdown_action == SHUTDOWN_ACTION_SLEEP ||
					 cfg_shutdown_action == SHUTDOWN_ACTION_HIBERNATE ||
					 cfg_shutdown_action == SHUTDOWN_ACTION_SHUT_DOWN ||
					 cfg_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN )
				{
					display_notice |= 0x08;
				}
			}

			cfg_use_temp_download_directory = ( _SendMessageW( g_hWnd_chk_temp_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

			if ( cfg_temp_download_directory != NULL )
			{
				GlobalFree( cfg_temp_download_directory );
			}

			// We want the length, so don't do GlobalStrDupW.
			g_temp_download_directory_length = lstrlenW( t_temp_download_directory );
			cfg_temp_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_temp_download_directory_length + 1 ) );
			_wmemcpy_s( cfg_temp_download_directory, g_temp_download_directory_length + 1, t_temp_download_directory, g_temp_download_directory_length );
			*( cfg_temp_download_directory + g_temp_download_directory_length ) = 0;	// Sanity.

			if ( display_notice == 0x01 || display_notice == 0x02 )
			{
				CMessageBoxW( hWnd, ( display_notice == 0x01 ? ST_V_A_restart_is_required_enable_allocation : ST_V_A_restart_is_required_disable_allocation ), PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
			}
			else if ( display_notice == 0x04 )
			{
				CMessageBoxW( hWnd, ST_V_A_restart_is_required_threads, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
			}
			else if ( display_notice == 0x08 )	
			{
				CMessageBoxW( hWnd, ST_V_A_restart_is_required_shutdown, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
			}
			else if ( display_notice & ( 0x01 | 0x02 | 0x04 | 0x08 ) )	// Multiple settings changed.
			{
				CMessageBoxW( hWnd, ST_V_A_restart_is_required, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
			}

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

			if ( t_temp_download_directory != NULL )
			{
				GlobalFree( t_temp_download_directory );
				t_temp_download_directory = NULL;
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
