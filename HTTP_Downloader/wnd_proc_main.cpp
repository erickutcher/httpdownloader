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

#include "globals.h"

#include "lite_gdi32.h"
#include "lite_ole32.h"
#include "lite_comctl32.h"
#include "lite_winmm.h"
//#include "lite_uxtheme.h"

#include "list_operations.h"
#include "file_operations.h"
#include "site_manager_utilities.h"
#include "sftp.h"

#include "connection.h"
#include "menus.h"

#include "http_parsing.h"
#include "utilities.h"

#include "drag_and_drop.h"

#include "string_tables.h"

#include "taskbar.h"

#include "system_tray.h"
#include "drop_window.h"

#include "treelistview.h"
#include "cmessagebox.h"

HWND g_hWnd_toolbar = NULL;
HWND g_hWnd_files_columns = NULL;		// The header control window for the listview.
HWND g_hWnd_tlv_files = NULL;
HWND g_hWnd_status = NULL;

wchar_t *tooltip_buffer = NULL;
int last_tooltip_item = -1;				// Prevent our hot tracking from calling the tooltip on the same item.

HIMAGELIST g_toolbar_imagelist = NULL;

HCURSOR wait_cursor = NULL;				// Temporary cursor while processing entries.

bool g_skip_list_draw = false;

int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

int g_border_width = 0;

unsigned char g_total_columns = 0;

unsigned long long g_session_total_downloaded = 0;
unsigned long long g_session_downloaded_speed = 0;

unsigned long long g_session_last_total_downloaded = 0;
unsigned long long g_session_last_downloaded_speed = 0;

wchar_t *g_size_prefix[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB" };

bool last_menu = false;					// true if context menu was last open, false if main menu was last open. See: WM_ENTERMENULOOP

HANDLE g_timer_semaphore = NULL;

bool use_drag_and_drop_main = true;		// Assumes OLE32_STATE_RUNNING is true.
IDropTarget *List_DropTarget;

bool use_taskbar_progress_main = true;	// Assumes OLE32_STATE_RUNNING is true.
_ITaskbarList3 *g_taskbar = NULL;

struct PROGRESS_INFO
{
	unsigned long long current_total_downloaded;
	unsigned long long current_total_file_size;
	unsigned char download_state;	// 0 = Downloading, 1 = Completed
};

PROGRESS_INFO g_progress_info;

UINT WM_TASKBARBUTTONCREATED = 0;

#define IDT_UPDATE_CHECK_TIMER	10000


/////////////////////////////////////////////////////


VOID CALLBACK UpdateCheckTimerProc( HWND hWnd, UINT /*msg*/, UINT /*idTimer*/, DWORD /*dwTime*/ )
{
	// We'll check the setting again in case the user turned it off before the 10 second grace period.
	// We'll also check to see if the update was manually checked.
	if ( cfg_check_for_updates && g_update_check_state == 0 )
	{
		// Create the update window so that our update check can send messages to it.
		if ( g_hWnd_check_for_updates == NULL )
		{
			g_hWnd_check_for_updates = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"check_for_updates", L"Check For Updates", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 400 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 135 ) / 2 ), 400, 135, NULL, NULL, NULL, NULL );

			g_update_check_state = 2;	// Automatic update check.

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, NULL, 0, NULL ) );
		}
	}

	_KillTimer( hWnd, IDT_UPDATE_CHECK_TIMER );
}

void ClearProgressBars()
{
	if ( g_progress_info.download_state == 1 )
	{
		g_progress_info.download_state = 0;

		if ( g_taskbar != NULL )
		{
			g_taskbar->lpVtbl->SetProgressState( g_taskbar, g_hWnd_main, TBPF_NOPROGRESS );
		}

		if ( cfg_enable_drop_window )
		{
			UpdateDropWindow( 0, 0, 0, 0, false );
		}

		if ( cfg_tray_icon )
		{
			g_nid.uFlags &= ~NIF_INFO;
			g_nid.hIcon = g_default_tray_icon;
			_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
		}
	}
}

void ResetSessionStatus()
{
	// Reset.
	_memzero( g_session_status_count, sizeof( unsigned int ) * 8 );
}

void FormatTooltipStatus()
{
	unsigned int buf_length = 0;

	wchar_t *status_strings[ 8 ] = { ST_V_Completed, ST_V_Stopped, ST_V_Timed_Out, ST_V_Failed, ST_V_File_IO_Error, ST_V_Skipped, ST_V_Authorization_Required, ST_V_Proxy_Authentication_Required };

	for ( unsigned char i = 0; i < 8; ++i )
	{
		if ( g_session_status_count[ i ] > 0 )
		{
			int ret = __snwprintf( g_nid.szInfo + buf_length, sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) - buf_length, L"%s%s: %lu", ( buf_length > 0 ? L"\r\n" : L"" ), status_strings[ i ], g_session_status_count[ i ] );

			if ( ret >= 0 )
			{
				buf_length += ret;
			}
			else
			{
				g_nid.szInfo[ sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) ] = 0;	// Sanity.

				break;
			}
		}
	}
}

unsigned int FormatSizes( wchar_t *buffer, unsigned int buffer_size, unsigned char toggle_type, unsigned long long data_size )
{
	unsigned int length;

	double divisor;

	if ( toggle_type == SIZE_FORMAT_AUTO )
	{
		if ( data_size >= ( 1ULL << 60 ) )
		{
			toggle_type = SIZE_FORMAT_EXABYTE;	// Exabyte
		}
		else if ( data_size >= ( 1ULL << 50 ) )
		{
			toggle_type = SIZE_FORMAT_PETABYTE;	// Petabyte
		}
		else if ( data_size >= ( 1ULL << 40 ) )
		{
			toggle_type = SIZE_FORMAT_TERABYTE;	// Terabyte
		}
		else if ( data_size >= ( 1 << 30 ) )
		{
			toggle_type = SIZE_FORMAT_GIGABYTE;	// Gigabyte
		}
		else if ( data_size >= ( 1 << 20 ) )
		{
			toggle_type = SIZE_FORMAT_MEGABYTE;	// Megabyte
		}
		else if ( data_size >= ( 1 << 10 ) )
		{
			toggle_type = SIZE_FORMAT_KILOBYTE;	// Kilobyte
		}
		else
		{
			toggle_type = SIZE_FORMAT_BYTE;		// Byte
		}
	}

	if ( toggle_type != SIZE_FORMAT_BYTE )
	{
		divisor = ( double )( 1ULL << ( toggle_type * 10 ) );

		unsigned long long i_percentage;
#ifdef _WIN64
		i_percentage = ( unsigned long long )( 100.0 * ( ( double )data_size / divisor ) );
#else
		// This leaves us with an integer in which the last digit will represent the decimal value.
		double f_percentage = 100.0 * ( ( double )data_size / divisor );
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
#endif
		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%I64u.%02lu %s", i_percentage, remainder, g_size_prefix[ toggle_type ] );
	}
	else
	{
		length = __snwprintf( buffer, buffer_size, L"%I64u %s", data_size, g_size_prefix[ toggle_type ] );
	}

	return length;
}

DWORD WINAPI UpdateWindow( LPVOID /*WorkThreadContext*/ )
{
	QFILETIME current_time, last_update;

	wchar_t title_text[ 128 ];
	wchar_t sb_downloaded_buf[ 128 ];
	wchar_t sb_download_speed_buf[ 128 ];

	bool update_text_values = false;

	unsigned int sb_downloaded_buf_length = 0;
	unsigned int sb_download_speed_buf_length = 0;

	bool run_timer = g_timers_running;
	unsigned char standby_counter = 0;

	COLORREF border_color_t, border_color_d;		// Tray and Drop window
	COLORREF progress_color_t, progress_color_d;	// Tray and Drop window

	unsigned char all_paused = 0;	// 0 = No state, 1 = all downloads are paused, 2 = a download is not paused

	last_update.ull = 0;

	unsigned char speed_buf_length = ( unsigned char )( ST_L_Download_speed_ > 102 ? 102 : ST_L_Download_speed_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
	_wmemcpy_s( sb_download_speed_buf, 128, ST_V_Download_speed_, speed_buf_length );
	sb_download_speed_buf[ speed_buf_length++ ] = ' ';

	unsigned char download_buf_length = ( unsigned char )( ST_L_Total_downloaded_ > 104 ? 104 : ST_L_Total_downloaded_ ); // Let's not overflow. 128 - ( ' ' + 22 + NULL ) = 104 remaining bytes for our string.
	_wmemcpy_s( sb_downloaded_buf, 128, ST_V_Total_downloaded_, download_buf_length );
	sb_downloaded_buf[ download_buf_length++ ] = ' ';

	_wmemcpy_s( title_text, 128, PROGRAM_CAPTION, 15 );

	_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ), PROGRAM_CAPTION, 15 );

	while ( !g_end_program )
	{
		// Update the status of our listview every second.
		WaitForSingleObject( g_timer_semaphore, ( run_timer ? 1000 : INFINITE ) );

		if ( g_end_program )
		{
			break;
		}

		// This will allow the timer to go through at least one loop after it's been disabled (g_timers_running == false).
		run_timer = g_timers_running;

		g_session_downloaded_speed = 0;

		if ( TryEnterCriticalSection( &worker_cs ) == TRUE )
		{
			if ( TryEnterCriticalSection( &active_download_list_cs ) == TRUE )
			{
				DoublyLinkedList *active_download_node = active_download_list;

				unsigned long long time_difference = 0;

				if ( g_taskbar != NULL )
				{
					g_taskbar->lpVtbl->SetProgressState( g_taskbar, g_hWnd_main, TBPF_NORMAL );
				}

				g_progress_info.current_total_downloaded = g_progress_info.current_total_file_size = 0;

				all_paused = 0;

				GetSystemTimeAsFileTime( &current_time.ft );

				// Calculate the download totals, speed, elapsed time, etc. while we have active connections.
				while ( active_download_node != NULL && !g_end_program )
				{
					DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )active_download_node->data;

					// Skip any files that can block us (large files that are currently allocating).
					if ( di != NULL )
					{
						if ( TryEnterCriticalSection( &di->di_cs ) == TRUE )
						{
							// If connecting, downloading, paused, or allocating then calculate the elapsed time.
							if ( IS_STATUS( di->status,
									STATUS_CONNECTING |
									STATUS_DOWNLOADING |
									STATUS_ALLOCATING_FILE ) )
							{
								di->time_elapsed = ( current_time.ull - di->start_time.QuadPart ) / FILETIME_TICKS_PER_SECOND;
							}

							// If downloading, then calculate the speed.
							if ( di->status == STATUS_DOWNLOADING )
							{
								// Determine the difference (in milliseconds) between the current time and our last update time.
								time_difference = ( current_time.ull - last_update.ull ) / ( FILETIME_TICKS_PER_SECOND / 1000 );	// Use milliseconds.

								// See if at least 1 second has elapsed since we last updated our speed and download time estimate.
								if ( time_difference >= 1000 )	// Measure in milliseconds for better precision. 1000 milliseconds = 1 second.
								{
									// Get the speed.
									di->speed = ( ( di->downloaded - di->last_downloaded ) * 1000 ) / time_difference;	// Multiply by 1000 to match the millisecond precision. Gives us bytes/second.

									// Get the time remaining.
									if ( di->speed > 0 )
									{
										if ( di->file_size > 0 && di->downloaded <= di->file_size )
										{
											// Get the remaining bytes and divide it by the speed.
											di->time_remaining = ( di->file_size - di->downloaded ) / di->speed;
										}
										else
										{
											di->time_remaining = 0;
										}

										// Don't include the group item's speed.
										if ( di->hosts <= 1 )
										{
											g_session_downloaded_speed += di->speed;
										}
									}
									else	// The remaining time will be unknown if the download stalls.
									{
										di->time_remaining = 0;
									}

									di->last_downloaded = di->downloaded;
								}

								g_progress_info.current_total_downloaded += di->downloaded;
								g_progress_info.current_total_file_size += di->file_size;

								all_paused = 2;
							}
							else if ( IS_STATUS( di->status, STATUS_PAUSED | STATUS_QUEUED ) )
							{
								di->time_remaining = 0;
								di->speed = 0;

								if ( all_paused == 0 )
								{
									all_paused = 1;
								}
							}

							LeaveCriticalSection( &di->di_cs );
						}
					}

					active_download_node = active_download_node->next;
				}

				last_update = current_time;

				LeaveCriticalSection( &active_download_list_cs );
			}

			LeaveCriticalSection( &worker_cs );
		}

		_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, 0, 0 );

		update_text_values = false;

		// Update our status bar with the download speed.
		if ( g_session_downloaded_speed != g_session_last_downloaded_speed )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + speed_buf_length, 128 - speed_buf_length, cfg_t_status_down_speed, g_session_downloaded_speed ) + speed_buf_length;
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
			sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )sb_download_speed_buf );

			g_session_last_downloaded_speed = g_session_downloaded_speed;

			update_text_values = true;
		}

		// Update our status bar with the download total.
		if ( g_session_total_downloaded != g_session_last_total_downloaded )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + download_buf_length, 128 - download_buf_length, cfg_t_status_downloaded, g_session_total_downloaded ) + download_buf_length;
			// NULL terminator is set in FormatSizes.

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )sb_downloaded_buf );

			g_session_last_total_downloaded = g_session_total_downloaded;

			update_text_values = true;
		}

		if ( run_timer )
		{
			if ( update_text_values )
			{
				int tooltip_offset = 15, title_text_offset = 15;

				// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
				sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + speed_buf_length, 128 - speed_buf_length, SIZE_FORMAT_AUTO, g_session_downloaded_speed ) + speed_buf_length;
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
				sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

				// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
				sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + download_buf_length, 128 - download_buf_length, SIZE_FORMAT_AUTO, g_session_total_downloaded ) + download_buf_length;
				// NULL terminator is set in FormatSizes.

				_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, L" - ", 3 );
				title_text_offset += 3;
				_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, sb_download_speed_buf, sb_download_speed_buf_length );
				title_text_offset += sb_download_speed_buf_length;

				_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, L" - ", 3 );
				title_text_offset += 3;
				_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, sb_downloaded_buf, sb_downloaded_buf_length );
				title_text_offset += sb_downloaded_buf_length;

				title_text[ title_text_offset ] = 0;	// Sanity.
				_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )title_text );

				if ( all_paused == 1 )
				{
					progress_color_t = cfg_color_t_p_p;
					border_color_t = cfg_color_t_p_b;

					progress_color_d = cfg_color_d_p_p;
					border_color_d = cfg_color_d_p_b;
				}
				else
				{
					progress_color_t = cfg_color_t_d_p;
					border_color_t = cfg_color_t_d_b;

					progress_color_d = cfg_color_d_d_p;
					border_color_d = cfg_color_d_d_b;
				}

				if ( cfg_enable_drop_window && cfg_show_drop_window_progress )
				{
					UpdateDropWindow( g_progress_info.current_total_downloaded, g_progress_info.current_total_file_size, border_color_d, progress_color_d );
				}

				if ( cfg_tray_icon )
				{
					if ( cfg_show_tray_progress )
					{
						g_nid.hIcon = CreateSystemTrayIcon( g_progress_info.current_total_downloaded, g_progress_info.current_total_file_size, border_color_t, progress_color_t );
					}
					else
					{
						g_nid.hIcon = g_default_tray_icon;
					}

					g_nid.uFlags &= ~NIF_INFO;

					_wmemcpy_s( g_nid.szTip + tooltip_offset, ( sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ) ) - tooltip_offset, L"\r\n", 2 );
					tooltip_offset += 2;
					_wmemcpy_s( g_nid.szTip + tooltip_offset, ( sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ) ) - tooltip_offset, sb_download_speed_buf, sb_download_speed_buf_length );
					tooltip_offset += sb_download_speed_buf_length;

					_wmemcpy_s( g_nid.szTip + tooltip_offset, ( sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ) ) - tooltip_offset, L"\r\n", 2 );
					tooltip_offset += 2;
					_wmemcpy_s( g_nid.szTip + tooltip_offset, ( sizeof( g_nid.szTip ) / sizeof( g_nid.szTip[ 0 ] ) ) - tooltip_offset, sb_downloaded_buf, sb_downloaded_buf_length );
					tooltip_offset += sb_downloaded_buf_length;

					g_nid.szTip[ tooltip_offset ] = 0;	// Sanity.
					_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
				}
			}

			g_progress_info.download_state = 0;	// Downloading.

			if ( g_taskbar != NULL )
			{
				if ( all_paused == 1 )
				{
					g_taskbar->lpVtbl->SetProgressState( g_taskbar, g_hWnd_main, TBPF_PAUSED );
				}

				if ( g_progress_info.current_total_file_size > 0 )
				{
					g_taskbar->lpVtbl->SetProgressValue( g_taskbar, g_hWnd_main, g_progress_info.current_total_downloaded, g_progress_info.current_total_file_size );
				}
			}

			// Sort all values that can change during a download.
			if ( cfg_sort_added_and_updating_items &&
				 cfg_sorted_column_index != COLUMN_NUM &&
				 cfg_sorted_column_index != COLUMN_DATE_AND_TIME_ADDED &&
				 cfg_sorted_column_index != COLUMN_DOWNLOAD_DIRECTORY &&
				 cfg_sorted_column_index != COLUMN_URL )
			{
				SORT_INFO si;
				si.column = GetColumnIndexFromVirtualIndex( cfg_sorted_column_index, download_columns, NUM_COLUMNS );
				si.hWnd = g_hWnd_tlv_files;
				si.direction = cfg_sorted_direction;

				_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
			}
		}
		else
		{
			_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )PROGRAM_CAPTION );

			g_progress_info.download_state = 1;	// Completed.

			bool error = ( ( g_session_status_count[ 2 ] > 0 || g_session_status_count[ 3 ] > 0 || g_session_status_count[ 4 ] > 0 ) ? true : false );
			progress_color_t = cfg_color_t_e_p;
			border_color_t = cfg_color_t_e_b;

			progress_color_d = cfg_color_d_e_p;
			border_color_d = cfg_color_d_e_b;

			if ( g_taskbar != NULL )
			{
				// If Timed Out, Failed, or File IO Error
				if ( error )
				{
					g_taskbar->lpVtbl->SetProgressState( g_taskbar, g_hWnd_main, TBPF_ERROR );

					g_taskbar->lpVtbl->SetProgressValue( g_taskbar, g_hWnd_main, 1, 1 );
				}
				else
				{
					g_taskbar->lpVtbl->SetProgressState( g_taskbar, g_hWnd_main, TBPF_NOPROGRESS );
				}
			}

			if ( cfg_enable_drop_window && cfg_show_drop_window_progress )
			{
				if ( error )
				{
					UpdateDropWindow( 1, 1, border_color_d, progress_color_d );
				}
				else
				{
					UpdateDropWindow( 0, 0, 0, 0, false );
				}
			}

			if ( cfg_tray_icon )
			{
				if ( cfg_show_tray_progress && error )
				{
					g_nid.hIcon = CreateSystemTrayIcon( 1, 1, border_color_t, progress_color_t );
				}
				else
				{
					g_nid.hIcon = g_default_tray_icon;
				}

				if ( cfg_show_notification )
				{
					FormatTooltipStatus();

					g_nid.uFlags |= NIF_INFO;
				}
				else
				{
					g_nid.uFlags &= ~NIF_INFO;
				}
				g_nid.szTip[ 15 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
			}

			ResetSessionStatus();

			// Sort all values that can change during a download.
			if ( cfg_sort_added_and_updating_items &&
				 cfg_sorted_column_index != COLUMN_NUM &&
				 cfg_sorted_column_index != COLUMN_DATE_AND_TIME_ADDED &&
				 cfg_sorted_column_index != COLUMN_DOWNLOAD_DIRECTORY &&
				 cfg_sorted_column_index != COLUMN_URL )
			{
				SORT_INFO si;
				si.column = GetColumnIndexFromVirtualIndex( cfg_sorted_column_index, download_columns, NUM_COLUMNS );
				si.hWnd = g_hWnd_tlv_files;
				si.direction = cfg_sorted_direction;

				_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
			}

			if ( cfg_play_sound && cfg_sound_file_path != NULL )
			{
				bool play = true;
				#ifndef WINMM_USE_STATIC_LIB
					if ( winmm_state == WINMM_STATE_SHUTDOWN )
					{
						play = false;	// Should have been loaded in main if cfg_play_sound was true. 
					}
				#endif

				if ( play ) { _PlaySoundW( cfg_sound_file_path, NULL, SND_ASYNC | SND_FILENAME ); }
			}

			HANDLE save_session_handle = NULL;
			if ( cfg_enable_download_history && g_download_history_changed )
			{
				save_session_handle = ( HANDLE )_CreateThread( NULL, 0, save_session, ( void * )NULL, 0, NULL );
			}

			if ( g_shutdown_action != SHUTDOWN_ACTION_NONE )
			{
				if ( save_session_handle != NULL )
				{
					WaitForSingleObject( save_session_handle, 30000 );
					CloseHandle( save_session_handle );
				}

				switch ( g_shutdown_action )
				{
					case SHUTDOWN_ACTION_EXIT_PROGRAM:
					{
						// Don't wait for the message to send.
						_PostMessageW( g_hWnd_main, WM_EXIT, 0, 0 );
					}
					break;

					case SHUTDOWN_ACTION_LOG_OFF:
					case SHUTDOWN_ACTION_RESTART:
					case SHUTDOWN_ACTION_SLEEP:
					case SHUTDOWN_ACTION_HIBERNATE:
					case SHUTDOWN_ACTION_SHUT_DOWN:
					case SHUTDOWN_ACTION_HYBRID_SHUT_DOWN:
					{
						g_perform_shutdown_action = true;

						// We need to exit the program cleanly and then perform the operation.
						// Don't wait for the message to send.
						_PostMessageW( g_hWnd_main, WM_EXIT, 0, 0 );
					}
					break;

					case SHUTDOWN_ACTION_LOCK:
					{
						_LockWorkStation();
					}
					break;
				}
			}
			else if ( save_session_handle != NULL )
			{
				CloseHandle( save_session_handle );
			}
		}

		if ( cfg_prevent_standby )
		{
			// I think the minimum standby time is 60 seconds. This should give us some wiggle room.
			if ( ++standby_counter >= 40 )
			{
				standby_counter = 0;

				SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED );
			}
		}
	}

	CloseHandle( g_timer_semaphore );
	g_timer_semaphore = NULL;

	_ExitThread( 0 );
	//return 0;
}

wchar_t *GetDownloadInfoString( DOWNLOAD_INFO *di, int column, int root_index, int item_index, wchar_t *tbuf, unsigned short tbuf_size )
{
	wchar_t *buf = NULL;

	// Save the appropriate text in our buffer for the current column.
	switch ( column )
	{
		case COLUMN_NUM:
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di == di->shared_info )
			{
				__snwprintf( buf, tbuf_size, L"%lu", root_index );
			}
			else
			{
				__snwprintf( buf, tbuf_size, L"%lu.%lu", root_index, item_index );
			}

			//__snwprintf( buf, tbuf_size, L"%lu", item_index );
		}
		break;

		case COLUMN_ACTIVE_PARTS:
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
			{
				if ( di->parts_limit > 0 )
				{
					__snwprintf( buf, tbuf_size, L"%lu/%lu/%lu [%lu/%lu]", di->active_parts, di->parts_limit, di->parts, di->active_hosts, di->hosts );
				}
				else
				{
					__snwprintf( buf, tbuf_size, L"%lu/%lu [%lu/%lu]", di->active_parts, di->parts, di->active_hosts, di->hosts );
				}
			}
			else
			{
				if ( di->parts_limit > 0 )
				{
					__snwprintf( buf, tbuf_size, L"%lu/%lu/%lu", di->active_parts, di->parts_limit, di->parts );
				}
				else
				{
					__snwprintf( buf, tbuf_size, L"%lu/%lu", di->active_parts, di->parts );
				}
			}
		}
		break;

		case COLUMN_DATE_AND_TIME_ADDED:
		{
			buf = di->shared_info->w_add_time;
		}
		break;

		case COLUMN_DOWNLOAD_DIRECTORY:
		{
			if ( !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				buf = di->shared_info->file_path;
			}
			else
			{
				buf = ST_V__Simulated_;
			}
		}
		break;

		case COLUMN_DOWNLOAD_SPEED:
		{
			if ( IS_STATUS( di->status, STATUS_DOWNLOADING ) )
			{
				buf = tbuf;	// Reset the buffer pointer.

				unsigned int length = FormatSizes( buf, tbuf_size, cfg_t_down_speed, di->speed );
				buf[ length++ ] = L'/';
				buf[ length++ ] = L's';
				buf[ length ] = 0;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case COLUMN_DOWNLOAD_SPEED_LIMIT:
		{
			if ( di->download_speed_limit > 0 )
			{
				buf = tbuf;	// Reset the buffer pointer.

				unsigned int length = FormatSizes( buf, tbuf_size, cfg_t_speed_limit, di->download_speed_limit );
				buf[ length++ ] = L'/';
				buf[ length++ ] = L's';
				buf[ length ] = 0;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case COLUMN_DOWNLOADED:
		{
			buf = tbuf;	// Reset the buffer pointer.

			FormatSizes( buf, tbuf_size, cfg_t_downloaded, ( IS_STATUS( di->status, STATUS_MOVING_FILE ) ? di->downloaded : di->last_downloaded ) );
		}
		break;

		case COLUMN_FILE_SIZE:
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di->file_size > 0 ||
			   ( di->status == STATUS_COMPLETED && di->file_size == 0 && di->last_downloaded == 0 ) ||
			   ( IS_STATUS( di->status, STATUS_MOVING_FILE ) && di->file_size == 0 && di->downloaded == 0 ) )
			{
				FormatSizes( buf, tbuf_size, cfg_t_file_size, di->file_size );
			}
			else
			{
				unsigned char prefix = ( cfg_t_file_size == SIZE_FORMAT_AUTO ? SIZE_FORMAT_BYTE : cfg_t_file_size );
				buf[ 0 ] = L'?';
				buf[ 1 ] = L' ';
				buf[ 2 ] = g_size_prefix[ prefix ][ 0 ];
				buf[ 3 ] = g_size_prefix[ prefix ][ 1 ];
				buf[ 4 ] = 0;
			}
		}
		break;

		/*case COLUMN_FILE_TYPE:
		{
		}
		break;*/

		case COLUMN_FILENAME:
		{
			buf = di->shared_info->file_path + di->shared_info->filename_offset;
		}
		break;

		case COLUMN_PROGRESS:
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di->file_size > 0 )
			{
				int i_percentage;
#ifdef _WIN64
				i_percentage = ( int )( 1000.0 * ( ( double )di->last_downloaded / ( double )di->file_size ) );
#else
				// Multiply the floating point division by 1000%.
				// This leaves us with an integer in which the last digit will represent the decimal value.
				double f_percentage = 1000.0 * ( ( double )di->last_downloaded / ( double )di->file_size );
				__asm
				{
					fld f_percentage;	//; Load the floating point value onto the FPU stack.
					fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
				}
#endif
				// Get the last digit (decimal value).
				int remainder = i_percentage % 10;
				// Divide the integer by (10%) to get it back in range of 0% to 100%.
				i_percentage /= 10;

				if ( di->status == STATUS_CONNECTING )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Connecting, i_percentage, remainder );
				}
				else if ( IS_STATUS( di->status, STATUS_RESTART ) )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Restarting, i_percentage, remainder );
				}
				else if ( IS_STATUS( di->status, STATUS_PAUSED ) )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Paused, i_percentage, remainder );
				}
				else if ( IS_STATUS( di->status, STATUS_QUEUED ) )
				{
					if ( IS_STATUS( di->status, STATUS_MOVING_FILE ) )
					{
						__snwprintf( buf, tbuf_size, L"%s - %s", ST_V_Moving_File, ST_V_Queued );
					}
					else
					{
						__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Queued, i_percentage, remainder );
					}
				}
				else if ( di->status == STATUS_COMPLETED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Completed, i_percentage, remainder );
				}
				else if ( di->status == STATUS_STOPPED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Stopped, i_percentage, remainder );
				}
				else if ( di->status == STATUS_TIMED_OUT )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Timed_Out, i_percentage, remainder );
				}
				else if ( di->status == STATUS_FAILED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Failed, i_percentage, remainder );
				}
				else if ( di->status == STATUS_FILE_IO_ERROR )
				{
					buf = ST_V_File_IO_Error;
				}
				else if ( di->status == STATUS_SKIPPED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Skipped, i_percentage, remainder );
				}
				else if ( di->status == STATUS_AUTH_REQUIRED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Authorization_Required, i_percentage, remainder );
				}
				else if ( di->status == STATUS_PROXY_AUTH_REQUIRED )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Proxy_Authentication_Required, i_percentage, remainder );
				}
				else if ( di->status == STATUS_ALLOCATING_FILE )
				{
					buf = ST_V_Allocating_File;
				}
				else if ( di->status == STATUS_MOVING_FILE )
				{
					__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Moving_File, i_percentage, remainder );
				}
				else	// Downloading.
				{
					__snwprintf( buf, tbuf_size, L"%d.%1d%%", i_percentage, remainder );
				}
			}
			else if ( di->status == STATUS_CONNECTING )
			{
				buf = ST_V_Connecting;
			}
			else if ( IS_STATUS( di->status, STATUS_RESTART ) )
			{
				buf = ST_V_Restarting;
			}
			else if ( IS_STATUS( di->status, STATUS_PAUSED ) )
			{
				buf = ST_V_Paused;
			}
			else if ( IS_STATUS( di->status, STATUS_QUEUED ) )
			{
				if ( IS_STATUS( di->status, STATUS_MOVING_FILE ) )
				{
					__snwprintf( buf, tbuf_size, L"%s - %s", ST_V_Moving_File, ST_V_Queued );
				}
				else
				{
					buf = ST_V_Queued;
				}
			}
			else if ( di->status == STATUS_COMPLETED )
			{
				if ( di->last_downloaded == 0 )
				{
					__snwprintf( buf, tbuf_size, L"%s - 100.0%%", ST_V_Completed );
				}
				else
				{
					buf = ST_V_Completed;
				}
			}
			else if ( di->status == STATUS_STOPPED )
			{
				buf = ST_V_Stopped;
			}
			else if ( di->status == STATUS_TIMED_OUT )
			{
				buf = ST_V_Timed_Out;
			}
			else if ( di->status == STATUS_FAILED )
			{
				buf = ST_V_Failed;
			}
			else if ( di->status == STATUS_FILE_IO_ERROR )
			{
				buf = ST_V_File_IO_Error;
			}
			else if ( di->status == STATUS_SKIPPED )
			{
				buf = ST_V_Skipped;
			}
			else if ( di->status == STATUS_AUTH_REQUIRED )
			{
				buf = ST_V_Authorization_Required;
			}
			else if ( di->status == STATUS_PROXY_AUTH_REQUIRED )
			{
				buf = ST_V_Proxy_Authentication_Required;
			}
			else if ( di->status == STATUS_ALLOCATING_FILE )
			{
				buf = ST_V_Allocating_File;
			}
			else if ( di->status == STATUS_MOVING_FILE )
			{
				buf = ST_V_Moving_File;
			}
			else	// Downloading.
			{
				buf = L"\x221E\0";	// Infinity symbol.
			}
		}
		break;

		case COLUMN_SSL_TLS_VERSION:
		{
			switch ( di->ssl_version )
			{
				case 0: { buf = ST_V_SSL_2_0; } break;
				case 1: { buf = ST_V_SSL_3_0; } break;
				case 2: { buf = ST_V_TLS_1_0; } break;
				case 3: { buf = ST_V_TLS_1_1; } break;
				case 4: { buf = ST_V_TLS_1_2; } break;
				default: { buf = L""; } break;
			}
		}
		break;

		case COLUMN_TIME_ELAPSED:
		case COLUMN_TIME_REMAINING:
		{
			// Use the infinity symbol for remaining time if it can't be calculated.
			if ( column == COLUMN_TIME_REMAINING &&
			   ( IS_STATUS( di->status, STATUS_CONNECTING | STATUS_PAUSED ) ||
			   ( di->status == STATUS_DOWNLOADING && ( di->file_size == 0 || di->speed == 0 ) ) ) )
			{
				buf = L"\x221E\0";	// Infinity symbol.
			}
			else
			{
				unsigned long long time_length = ( column == COLUMN_TIME_ELAPSED ? di->time_elapsed : di->time_remaining );

				if ( IS_STATUS( di->status, STATUS_DOWNLOADING ) || time_length > 0 )
				{
					buf = tbuf;	// Reset the buffer pointer.

					if ( time_length < 60 )	// Less than 1 minute.
					{
						__snwprintf( buf, tbuf_size, L"%I64us", time_length );
					}
					else if ( time_length < 3600 )	// Less than 1 hour.
					{
						__snwprintf( buf, tbuf_size, L"%I64um%02llus", time_length / 60, time_length % 60 );
					}
					else if ( time_length < 86400 )	// Less than 1 day.
					{
						__snwprintf( buf, tbuf_size, L"%I64uh%02llum%02llus", time_length / 3600, ( time_length / 60 ) % 60, time_length % 60 );
					}
					else	// More than 1 day.
					{
						__snwprintf( buf, tbuf_size, L"%I64ud%02lluh%02llum%02llus", time_length / 86400, ( time_length / 3600 ) % 24, ( time_length / 60 ) % 60, time_length % 60 );
					}
				}
				else
				{
					buf = L"";
				}
			}
		}
		break;

		case COLUMN_URL:
		{
			if ( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
			{
				buf = ST_V__DATA_;
			}
			else
			{
				buf = di->url;
			}
		}
		break;
	}

	return buf;
}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			g_hWnd_toolbar = _CreateWindowExW( WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL, CCS_NODIVIDER | WS_CHILDWINDOW | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | ( cfg_show_toolbar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			HWND hWnd_toolbar_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWnd_toolbar, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_toolbar, TB_SETTOOLTIPS, ( WPARAM )hWnd_toolbar_tooltip, 0 );

			_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\toolbar.bmp\0", 13 );
			if ( GetFileAttributesW( g_program_directory ) != INVALID_FILE_ATTRIBUTES )
			{
				g_toolbar_imagelist = _ImageList_LoadImageW( NULL, g_program_directory, 24, 0, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ), IMAGE_BITMAP, LR_LOADFROMFILE | LR_CREATEDIBSECTION );
			}
			else
			{
				g_toolbar_imagelist = _ImageList_LoadImageW( GetModuleHandleW( NULL ), MAKEINTRESOURCE( IDB_BITMAP_TOOLBAR ), 24, 0, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ), IMAGE_BITMAP, LR_CREATEDIBSECTION );
			}
			_SendMessageW( g_hWnd_toolbar, TB_SETIMAGELIST, 0, ( LPARAM )g_toolbar_imagelist );

			_SendMessageW( g_hWnd_toolbar, TB_BUTTONSTRUCTSIZE, ( WPARAM )sizeof( TBBUTTON ), 0 );

			// Allows us to use the iString value for tooltips.
			_SendMessageW( g_hWnd_toolbar, TB_SETMAXTEXTROWS, 0, 0 );

			TBBUTTON tbb[ 15 ] = 
			{
				{ MAKELONG( 0, 0 ),				  MENU_ADD_URLS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,					 ( INT_PTR )ST_V_Add_URL_s_ },
				{ MAKELONG( 1, 0 ),					MENU_REMOVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						 ( INT_PTR )ST_V_Remove },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 2, 0 ),					 MENU_START,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						  ( INT_PTR )ST_V_Start },
				{ MAKELONG( 3, 0 ),					 MENU_PAUSE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						  ( INT_PTR )ST_V_Pause },
				{ MAKELONG( 4, 0 ),					  MENU_STOP,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						   ( INT_PTR )ST_V_Stop },
				{ MAKELONG( 5, 0 ),				   MENU_RESTART,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						( INT_PTR )ST_V_Restart },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 6, 0 ),			  MENU_PAUSE_ACTIVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,				   ( INT_PTR )ST_V_Pause_Active },
				{ MAKELONG( 7, 0 ),				  MENU_STOP_ALL,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,					   ( INT_PTR )ST_V_Stop_All },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 8, 0 ),					MENU_SEARCH,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						 ( INT_PTR )ST_V_Search },
				{ MAKELONG( 9, 0 ),		MENU_GLOBAL_SPEED_LIMIT,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,	( INT_PTR )ST_V_Global_Download_Speed_Limit },
				{ MAKELONG( 10, 0 ),		  MENU_SITE_MANAGER,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,				   ( INT_PTR )ST_V_Site_Manager },
				{ MAKELONG( 11, 0 ),			   MENU_OPTIONS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						( INT_PTR )ST_V_Options }
			};

			_SendMessageW( g_hWnd_toolbar, TB_ADDBUTTONS, 15, ( LPARAM )&tbb );

			g_hWnd_tlv_files = _CreateWindowW( L"TreeListView", NULL, WS_VSCROLL | WS_HSCROLL | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_status = _CreateWindowW( STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILDWINDOW | ( cfg_show_status_bar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_toolbar, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_status, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop_main = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop_main )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_tlv_files, &List_DropTarget );
			}

			int status_bar_widths[] = { 250, 500, -1 };

			_SendMessageW( g_hWnd_status, SB_SETPARTS, 3, ( LPARAM )status_bar_widths );

			wchar_t status_bar_buf[ 128 ];
			unsigned char buf_length;

			buf_length = ( unsigned char )( ST_L_Download_speed_ > 102 ? 102 : ST_L_Download_speed_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
			_wmemcpy_s( status_bar_buf, 128, ST_V_Download_speed_, buf_length );
			status_bar_buf[ buf_length++ ] = ' ';
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			unsigned int length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_down_speed, 0 ) + buf_length;
			status_bar_buf[ length++ ] = L'/';
			status_bar_buf[ length++ ] = L's';
			status_bar_buf[ length ] = 0;

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )status_bar_buf );


			buf_length = ( unsigned char )( ST_L_Total_downloaded_ > 104 ? 104 : ST_L_Total_downloaded_ ); // Let's not overflow. 128 - ( ' ' + 22 + NULL ) = 104 remaining bytes for our string.
			_wmemcpy_s( status_bar_buf, 128, ST_V_Total_downloaded_, buf_length );
			status_bar_buf[ buf_length++ ] = ' ';
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_downloaded, 0 );
			// NULL terminator is set in FormatSizes.
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );


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
				length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_speed_limit, cfg_download_speed_limit ) + buf_length;
				status_bar_buf[ length++ ] = L'/';
				status_bar_buf[ length++ ] = L's';
				status_bar_buf[ length ] = 0;
			}

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 2, 0 ), ( LPARAM )status_bar_buf );

			if ( cfg_tray_icon )
			{
				InitializeSystemTray( hWnd );

				if ( cfg_show_tray_progress )
				{
					InitializeIconValues( hWnd );
				}
			}

			// Create the menus after we've gotten the total active columns (g_total_columns).
			CreateMenus();

			// Set our menu bar.
			_SetMenu( hWnd, g_hMenu );

			// Check again if we had threads queue up.
			if ( g_timer_semaphore == NULL )
			{
				g_timer_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

				//CloseHandle( _CreateThread( NULL, 0, UpdateWindow, NULL, 0, NULL ) );
				HANDLE timer_handle = _CreateThread( NULL, 0, UpdateWindow, NULL, 0, NULL );
				SetThreadPriority( timer_handle, THREAD_PRIORITY_LOWEST );
				CloseHandle( timer_handle );
			}

			if ( cfg_enable_download_history )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );

				// Include an empty string.
				iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH + 1 ) );
				_wmemcpy_s( iei->file_paths, MAX_PATH, g_base_directory, g_base_directory_length );
				_wmemcpy_s( iei->file_paths + ( g_base_directory_length + 1 ), MAX_PATH - ( g_base_directory_length - 1 ), L"download_history\0", 17 );
				iei->file_paths[ g_base_directory_length + 17 ] = 0;	// Sanity.
				iei->file_offset = ( unsigned short )( g_base_directory_length + 1 );
				iei->type = 0;	// Load during startup.

				// iei will be freed in the import_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, import_list, ( void * )iei, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( iei->file_paths );
					GlobalFree( iei );
				}
			}

			tooltip_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 512 );

			_memzero( &g_progress_info, sizeof( PROGRESS_INFO ) );

			WM_TASKBARBUTTONCREATED = _RegisterWindowMessageW( L"TaskbarButtonCreated" );

			if ( cfg_check_for_updates )
			{
				// Check after 10 seconds.
				_SetTimer( hWnd, IDT_UPDATE_CHECK_TIMER, 10000, ( TIMERPROC )UpdateCheckTimerProc );
			}

			// Windows 10 uses an invisible border that we need to take into account when snapping the window.
			if ( IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN10 ), LOBYTE( _WIN32_WINNT_WIN10 ), 0 ) )
			{
				/*bool use_theme = true;

				#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					use_theme = InitializeUXTheme();
				}
				#endif

				if ( use_theme && _IsThemeActive() == TRUE )*/
				{
					RECT rc, rc2;
					_GetWindowRect( hWnd, &rc );
					_GetClientRect( hWnd, &rc2 );

					g_border_width = ( ( rc.right - rc.left - rc2.right ) / 2 ) - 1; // Leave the 1 px border.
				}
			}

			return 0;
		}
		break;

		case WM_ENTERMENULOOP:
		{
			// If we've clicked the menu bar.
			if ( ( BOOL )wParam == FALSE )
			{
				// And a context menu was open, then revert the context menu additions.
				if ( last_menu )
				{
					UpdateMenus( true );

					last_menu = false;	// Prevent us from calling UpdateMenus again.
				}

				// Allow us to save the download history if there are any entries in the files listview.
				_EnableMenuItem( g_hMenu, MENU_SAVE_DOWNLOAD_HISTORY, ( TLV_GetTotalItemCount() > 0 ? MF_ENABLED : MF_GRAYED ) );
			}
			else if ( ( BOOL )wParam == TRUE )
			{
				last_menu = true;	// The last menu to be open was a context menu.
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			HandleCommand( hWnd, LOWORD( wParam ) );

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_toolbar || nmitem->hdr.hwndFrom == g_hWnd_status )
					{
						POINT p;
						_GetCursorPos( &p );

						_TrackPopupMenu( g_hMenuSub_view, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

				case NM_CLICK:
				{
					NMMOUSE *nm = ( NMMOUSE * )lParam;

					// Change the format of the panel if Ctrl is held while clicking the panel.
					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						if ( nm->hdr.hwndFrom == g_hWnd_status )
						{
							wchar_t status_bar_buf[ 128 ];
							unsigned char buf_length;

							if ( nm->dwItemSpec == 0 )
							{
								if ( cfg_t_status_down_speed >= SIZE_FORMAT_AUTO )
								{
									cfg_t_status_down_speed = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_status_down_speed;
								}

								buf_length = ( unsigned char )( ST_L_Download_speed_ > 102 ? 102 : ST_L_Download_speed_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
								_wmemcpy_s( status_bar_buf, 128, ST_V_Download_speed_, buf_length );
								status_bar_buf[ buf_length++ ] = ' ';
								// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
								unsigned int length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_down_speed, g_session_downloaded_speed ) + buf_length;
								status_bar_buf[ length++ ] = L'/';
								status_bar_buf[ length++ ] = L's';
								status_bar_buf[ length ] = 0;

								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )status_bar_buf );
							}
							else if ( nm->dwItemSpec == 1 )
							{
								if ( cfg_t_status_downloaded >= SIZE_FORMAT_AUTO )
								{
									cfg_t_status_downloaded = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_status_downloaded;
								}

								buf_length = ( unsigned char )( ST_L_Total_downloaded_ > 104 ? 104 : ST_L_Total_downloaded_ ); // Let's not overflow. 128 - ( ' ' + 22 + NULL ) = 104 remaining bytes for our string.
								_wmemcpy_s( status_bar_buf, 128, ST_V_Total_downloaded_, buf_length );
								status_bar_buf[ buf_length++ ] = ' ';
								// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
								FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_downloaded, g_session_total_downloaded );
								// NULL terminator is set in FormatSizes.

								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );
							}
							else if ( nm->dwItemSpec == 2 )
							{
								buf_length = ( unsigned char )( ST_L_Global_download_speed_limit_ > 102 ? 102 : ST_L_Global_download_speed_limit_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
								_wmemcpy_s( status_bar_buf, 128, ST_V_Global_download_speed_limit_, buf_length );
								status_bar_buf[ buf_length++ ] = ' ';

								if ( cfg_download_speed_limit == 0 )
								{
									_wmemcpy_s( status_bar_buf + buf_length, 128 - buf_length, ST_V_Unlimited, ST_L_Unlimited + 1 );
								}
								else
								{
									if ( cfg_t_status_speed_limit >= SIZE_FORMAT_AUTO )
									{
										cfg_t_status_speed_limit = SIZE_FORMAT_BYTE;
									}
									else
									{
										++cfg_t_status_speed_limit;
									}

									// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
									unsigned int length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_speed_limit, cfg_download_speed_limit ) + buf_length;
									status_bar_buf[ length++ ] = L'/';
									status_bar_buf[ length++ ] = L's';
									status_bar_buf[ length ] = 0;
								}

								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 2, 0 ), ( LPARAM )status_bar_buf );
							}
						}
					}
				}
				break;

				case NM_DBLCLK:
				{
					//NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;
					//NMMOUSE *nm = ( NMMOUSE * )lParam;

					HWND hwndFrom = ( ( LPNMHDR )lParam )->hwndFrom;

					if ( hwndFrom == g_hWnd_status )
					{
						// Let NM_CLICK handle fast (double) clicks if Ctrl is down.
						if ( !( GetKeyState( VK_CONTROL ) & 0x8000 ) )
						{
							if ( ( ( NMMOUSE * )lParam )->dwItemSpec == 2 )
							{
								_SendMessageW( hWnd, WM_COMMAND, MENU_GLOBAL_SPEED_LIMIT, 0 );
							}
						}
					}
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_WINDOWPOSCHANGED:
		{
			// Only handle main window changes.
			if ( hWnd == g_hWnd_main )
			{
				// Don't want to save minimized and maximized size and position values.
				if ( _IsIconic( hWnd ) == TRUE )
				{
					cfg_min_max = 1;
				}
				else if ( _IsZoomed( hWnd ) == TRUE )
				{
					cfg_min_max = 2;
				}
				else
				{
					// This will capture MoveWindow and SetWindowPos changes.
					WINDOWPOS *wp = ( WINDOWPOS * )lParam;

					if ( !( wp->flags & SWP_NOMOVE ) )
					{
						cfg_pos_x = wp->x;
						cfg_pos_y = wp->y;
					}

					if ( !( wp->flags & SWP_NOSIZE ) )
					{
						cfg_width = wp->cx;
						cfg_height = wp->cy;
					}

					cfg_min_max = 0;
				}
			}

			// Let it fall through so we can still get the WM_SIZE message.
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_SIZE:
		{
			RECT rc, rc_toolbar, rc_status;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 1 );

			if ( cfg_show_toolbar )
			{
				_SendMessageW( g_hWnd_toolbar, TB_AUTOSIZE, 0, 0 ); 

				_GetWindowRect( g_hWnd_toolbar, &rc_toolbar );

				rc.top = rc_toolbar.bottom - rc_toolbar.top;
				rc.bottom -= rc.top;
			}

			if ( cfg_show_status_bar )
			{
				_GetWindowRect( g_hWnd_status, &rc_status );

				_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom - ( rc_status.bottom - rc_status.top ), SWP_NOZORDER );

				// Apparently status bars want WM_SIZE to be called. (See MSDN)
				_SendMessageW( g_hWnd_status, WM_SIZE, 0, 0 );
			}
			else
			{
				_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER );
			}

			_EndDeferWindowPos( hdwp );

			if ( wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED )
			{
				ClearProgressBars();
			}

			return 0;
		}
		break;

		case WM_SIZING:
		{
			RECT *rc = ( RECT * )lParam;

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;

			return TRUE;
		}
		break;

		case WM_MOVING:
		{
			POINT cur_pos;
			RECT wa;
			RECT *rc = ( RECT * )lParam;

			_GetCursorPos( &cur_pos );
			_OffsetRect( rc, cur_pos.x - ( rc->left + cx ), cur_pos.y - ( rc->top + cy ) );

			// Allow our main window to attach to the desktop edge.
			_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );			
			if ( is_close( rc->left + g_border_width, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left - g_border_width, 0 );
			}
			else if ( is_close( wa.right, rc->right + g_border_width ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right + g_border_width, 0 );
			}

			if ( is_close( rc->top, wa.top ) )									// Attach to top of the desktop.
			{
				_OffsetRect( rc, 0, wa.top - rc->top );
			}
			else if ( is_close( wa.bottom, rc->bottom + g_border_width ) )		// Attach to bottom of the desktop.
			{
				_OffsetRect( rc, 0, wa.bottom - rc->bottom + g_border_width );
			}

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;
			cfg_width = rc->right - rc->left;
			cfg_height = rc->bottom - rc->top;

			return TRUE;
		}
		break;

		case WM_ENTERSIZEMOVE:
		{
			//Get the current position of our window before it gets moved.
			POINT cur_pos;
			RECT rc;
			_GetWindowRect( hWnd, &rc );
			_GetCursorPos( &cur_pos );
			cx = cur_pos.x - rc.left;
			cy = cur_pos.y - rc.top;

			return 0;
		}
		break;

		case WM_CHANGE_CURSOR:
		{
			// SetCursor must be called from the window thread.
			if ( wParam == TRUE )
			{
				wait_cursor = _LoadCursorW( NULL, IDC_APPSTARTING );	// Arrow + hourglass.
				_SetCursor( wait_cursor );
			}
			else
			{
				_SetCursor( _LoadCursorW( NULL, IDC_ARROW ) );	// Default arrow.
				wait_cursor = NULL;
			}

			return TRUE;
		}
		break;

		case WM_SETCURSOR:
		{
			if ( wait_cursor != NULL )
			{
				_SetCursor( wait_cursor );	// Keep setting our cursor if it reverts back to the default.
				return TRUE;
			}

			_DefWindowProcW( hWnd, msg, wParam, lParam );
			return FALSE;
		}
		break;

		case WM_SYSCOMMAND:
		{
			if ( wParam == SC_MINIMIZE && cfg_tray_icon && cfg_minimize_to_tray )
			{
				_ShowWindow( hWnd, SW_HIDE );

				return 0;
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_TRAY_NOTIFY:
		{
			switch ( lParam )
			{
				case WM_LBUTTONDOWN:
				{
					if ( _IsWindowVisible( hWnd ) == FALSE )
					{
						_ShowWindow( hWnd, SW_SHOW );
						_SetForegroundWindow( hWnd );
					}
					else
					{
						_ShowWindow( hWnd, SW_HIDE );
					}
				}
				break;

				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
				{
					_SetForegroundWindow( hWnd );	// Must set so that the menu can close.

					// Show our tray context menu as a popup.
					POINT p;
					_GetCursorPos( &p );
					_TrackPopupMenu( g_hMenuSub_tray, 0, p.x, p.y, 0, hWnd, NULL );
				}
				break;
			}

			return TRUE;
		}
		break;

		case WM_COPYDATA:
		{
			COPYDATASTRUCT *cds = ( COPYDATASTRUCT * )lParam;

			// Do not free lpData.
			if ( cds != NULL && cds->lpData != NULL )
			{
				CL_ARGS *cla = ( CL_ARGS * )cds->lpData;
				wchar_t *cl_val = ( wchar_t * )( ( char * )cla + sizeof( CL_ARGS ) );

				// Our pointers were used to store each string's offset past the CL_ARGS struct.
				// We're reverting them back to point to their respective string.
				cla->download_directory = ( cla->download_directory_length > 0 ? cl_val + ( unsigned int )cla->download_directory : NULL );
				cla->download_history_file = ( cla->download_history_file_length > 0 ? cl_val + ( unsigned int )cla->download_history_file : NULL );
				cla->url_list_file = ( cla->url_list_file_length > 0 ? cl_val + ( unsigned int )cla->url_list_file : NULL );
				cla->urls = ( cla->urls_length > 0 ? cl_val + ( unsigned int )cla->urls : NULL );
				cla->cookies = ( cla->cookies_length > 0 ? cl_val + ( unsigned int )cla->cookies : NULL );
				cla->headers = ( cla->headers_length > 0 ? cl_val + ( unsigned int )cla->headers : NULL );
				cla->data = ( cla->data_length > 0 ? cl_val + ( unsigned int )cla->data : NULL );
				cla->username = ( cla->username_length > 0 ? cl_val + ( unsigned int )cla->username : NULL );
				cla->password = ( cla->password_length > 0 ? cl_val + ( unsigned int )cla->password : NULL );
				cla->proxy_hostname = ( cla->proxy_hostname_length > 0 ? cl_val + ( unsigned int )cla->proxy_hostname : NULL );
				cla->proxy_username = ( cla->proxy_username_length > 0 ? cl_val + ( unsigned int )cla->proxy_username : NULL );
				cla->proxy_password = ( cla->proxy_password_length > 0 ? cl_val + ( unsigned int )cla->proxy_password : NULL );

				CL_ARGS *new_cla = ( CL_ARGS * )GlobalAlloc( GMEM_FIXED, sizeof( CL_ARGS ) );
				_memcpy_s( new_cla, sizeof( CL_ARGS ), cla, sizeof( CL_ARGS ) );

				if ( cla->download_directory != NULL )
				{
					new_cla->download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->download_directory_length + 1 ) );
					_wmemcpy_s( new_cla->download_directory, cla->download_directory_length + 1, cla->download_directory, cla->download_directory_length );
					new_cla->download_directory[ cla->download_directory_length ] = 0;	// Sanity.
				}

				if ( cla->download_history_file != NULL )
				{
					new_cla->download_history_file = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->download_history_file_length + 1 ) );
					_wmemcpy_s( new_cla->download_history_file, cla->download_history_file_length + 1, cla->download_history_file, cla->download_history_file_length );
					new_cla->download_history_file[ cla->download_history_file_length ] = 0;	// Sanity.
				}

				if ( cla->url_list_file != NULL )
				{
					new_cla->url_list_file = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->url_list_file_length + 1 ) );
					_wmemcpy_s( new_cla->url_list_file, cla->url_list_file_length + 1, cla->url_list_file, cla->url_list_file_length );
					new_cla->url_list_file[ cla->url_list_file_length ] = 0;	// Sanity.
				}

				if ( cla->urls != NULL )
				{
					new_cla->urls = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->urls_length + 1 ) );
					_wmemcpy_s( new_cla->urls, cla->urls_length + 1, cla->urls, cla->urls_length );
					new_cla->urls[ cla->urls_length ] = 0;	// Sanity.
				}

				if ( cla->cookies != NULL )
				{
					new_cla->cookies = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->cookies_length + 1 ) );
					_wmemcpy_s( new_cla->cookies, cla->cookies_length + 1, cla->cookies, cla->cookies_length );
					new_cla->cookies[ cla->cookies_length ] = 0;	// Sanity.
				}

				if ( cla->headers != NULL )
				{
					new_cla->headers = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->headers_length + 1 ) );
					_wmemcpy_s( new_cla->headers, cla->headers_length + 1, cla->headers, cla->headers_length );
					new_cla->headers[ cla->headers_length ] = 0;	// Sanity.
				}

				if ( cla->data != NULL )
				{
					new_cla->data = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->data_length + 1 ) );
					_wmemcpy_s( new_cla->data, cla->data_length + 1, cla->data, cla->data_length );
					new_cla->data[ cla->data_length ] = 0;	// Sanity.
				}

				if ( cla->username != NULL )
				{
					new_cla->username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->username_length + 1 ) );
					_wmemcpy_s( new_cla->username, cla->username_length + 1, cla->username, cla->username_length );
					new_cla->username[ cla->username_length ] = 0;	// Sanity.
				}

				if ( cla->password != NULL )
				{
					new_cla->password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->password_length + 1 ) );
					_wmemcpy_s( new_cla->password, cla->password_length + 1, cla->password, cla->password_length );
					new_cla->password[ cla->password_length ] = 0;	// Sanity.
				}

				if ( cla->proxy_hostname != NULL )
				{
					new_cla->proxy_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_hostname_length + 1 ) );
					_wmemcpy_s( new_cla->proxy_hostname, cla->proxy_hostname_length + 1, cla->proxy_hostname, cla->proxy_hostname_length );
					new_cla->proxy_hostname[ cla->proxy_hostname_length ] = 0;	// Sanity.
				}

				if ( cla->proxy_username != NULL )
				{
					new_cla->proxy_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_username_length + 1 ) );
					_wmemcpy_s( new_cla->proxy_username, cla->proxy_username_length + 1, cla->proxy_username, cla->proxy_username_length );
					new_cla->proxy_username[ cla->proxy_username_length ] = 0;	// Sanity.
				}

				if ( cla->proxy_password != NULL )
				{
					new_cla->proxy_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_password_length + 1 ) );
					_wmemcpy_s( new_cla->proxy_password, cla->proxy_password_length + 1, cla->proxy_password, cla->proxy_password_length );
					new_cla->proxy_password[ cla->proxy_password_length ] = 0;	// Sanity.
				}

				_SendMessageW( hWnd, WM_PROPAGATE, ( WPARAM )-2, ( LPARAM )new_cla );
			}

			return TRUE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == -2 )	// Load command-line parameters.
			{
				CL_ARGS *cla = ( CL_ARGS * )lParam;

				if ( cla != NULL )
				{
					// cla is freed in process_command_line_args.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, process_command_line_args, ( void * )cla, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						FreeCommandLineArgs( &cla );
					}
				}
			}
			else
			{
				if ( g_hWnd_add_urls == NULL )
				{
					g_hWnd_add_urls = _CreateWindowExW( ( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ) | ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"add_urls", ST_V_Add_URL_s_, WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 600 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 270 ) / 2 ), 600, 270, NULL, NULL, NULL, NULL );
				}

				_SendMessageW( g_hWnd_add_urls, WM_PROPAGATE, wParam, lParam );
			}

			return TRUE;
		}
		break;

		case WM_ALERT:
		{
			if ( wParam == 0 )
			{
				CMessageBoxW( hWnd, ( LPCWSTR )lParam, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
			}
			else if ( wParam == 1 )
			{
				if ( CMessageBoxW( hWnd, ST_V_PROMPT_The_specified_file_was_not_found, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )3, 0, NULL ) );	// Restart download (from the beginning).
				}
			}

			return TRUE;
		}
		break;

		case WM_ACTIVATE:
		{
			ClearProgressBars();

			_SetFocus( g_hWnd_tlv_files );

			return 0;
		}
		break;

		case WM_CLOSE:
		{
			if ( cfg_tray_icon && cfg_close_to_tray )
			{
				_ShowWindow( hWnd, SW_HIDE );
			}
			else
			{
				_SendMessageW( hWnd, WM_EXIT, 0, 0 );
			}
			return 0;
		}
		break;

		case WM_EXIT:
		{
			if ( g_hWnd_add_urls != NULL )
			{
				_EnableWindow( g_hWnd_add_urls, FALSE );
				_ShowWindow( g_hWnd_add_urls, SW_HIDE );
			}

			if ( g_hWnd_options != NULL )
			{
				_EnableWindow( g_hWnd_options, FALSE );
				_ShowWindow( g_hWnd_options, SW_HIDE );
			}

			if ( g_hWnd_update_download != NULL )
			{
				_EnableWindow( g_hWnd_update_download, FALSE );
				_ShowWindow( g_hWnd_update_download, SW_HIDE );
			}

			if ( g_hWnd_search != NULL )
			{
				_EnableWindow( g_hWnd_search, FALSE );
				_ShowWindow( g_hWnd_search, SW_HIDE );
			}

			if ( g_hWnd_download_speed_limit != NULL )
			{
				_EnableWindow( g_hWnd_download_speed_limit, FALSE );
				_ShowWindow( g_hWnd_download_speed_limit, SW_HIDE );
			}

			if ( g_hWnd_check_for_updates != NULL )
			{
				_EnableWindow( g_hWnd_check_for_updates, FALSE );
				_ShowWindow( g_hWnd_check_for_updates, SW_HIDE );
			}

			if ( g_hWnd_site_manager != NULL )
			{
				_EnableWindow( g_hWnd_site_manager, FALSE );
				_ShowWindow( g_hWnd_site_manager, SW_HIDE );
			}

			if ( g_hWnd_url_drop_window != NULL )
			{
				_EnableWindow( g_hWnd_url_drop_window, FALSE );
				_ShowWindow( g_hWnd_url_drop_window, SW_HIDE );
			}

			_ShowWindow( hWnd, SW_HIDE );

			g_end_program = true;

			// Exit our timer thread if it's active.
			if ( g_timer_semaphore != NULL )
			{
				ReleaseSemaphore( g_timer_semaphore, 1, NULL );
			}

			// Release the semaphore to complete the update check.
			if ( g_update_semaphore != NULL )
			{
				ReleaseSemaphore( g_update_semaphore, 1, NULL );
			}

			// If we're in a secondary thread, then kill it (cleanly) and wait for it to exit.
			if ( in_worker_thread )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, cleanup, ( void * )NULL, 0, NULL ) );
			}
			else	// Otherwise, destroy the window normally.
			{
				kill_worker_thread_flag = true;
				_SendMessageW( hWnd, WM_DESTROY_ALT, 0, 0 );
			}

			return TRUE;
		}
		break;

		case WM_DESTROY_ALT:
		{
			if ( ws2_32_state == WS2_32_STATE_RUNNING )
			{
				downloader_ready_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

				_WSASetEvent( g_cleanup_event[ 0 ] );

				// Wait for IOCPDownloader to clean up. 10 second timeout in case we miss the release.
				WaitForSingleObject( downloader_ready_semaphore, 10000 );
				CloseHandle( downloader_ready_semaphore );
				downloader_ready_semaphore = NULL;
			}

			if ( g_hWnd_add_urls != NULL )
			{
				_DestroyWindow( g_hWnd_add_urls );
			}

			if ( g_hWnd_options != NULL )
			{
				_DestroyWindow( g_hWnd_options );
			}

			if ( g_hWnd_update_download != NULL )
			{
				_DestroyWindow( g_hWnd_update_download );
			}

			if ( g_hWnd_search != NULL )
			{
				_DestroyWindow( g_hWnd_search );
			}

			if ( g_hWnd_download_speed_limit != NULL )
			{
				_DestroyWindow( g_hWnd_download_speed_limit );
			}

			if ( g_hWnd_check_for_updates != NULL )
			{
				_DestroyWindow( g_hWnd_check_for_updates );
			}

			if ( g_hWnd_site_manager != NULL )
			{
				_DestroyWindow( g_hWnd_site_manager );
			}

			if ( g_hWnd_url_drop_window != NULL )
			{
				_DestroyWindow( g_hWnd_url_drop_window );
			}

			if ( cfg_enable_download_history && g_download_history_changed )
			{
				_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\download_history\0", 18 );
				g_base_directory[ g_base_directory_length + 17 ] = 0;	// Sanity.

				save_download_history( g_base_directory );
				g_download_history_changed = false;
			}

			TREELISTNODE *tln = g_tree_list;
			while ( tln != NULL )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					// di->icon is stored in the icon_handles tree and is destroyed in main.
					GlobalFree( di->url );
					GlobalFree( di->cookies );
					GlobalFree( di->headers );
					GlobalFree( di->data );
					//GlobalFree( di->etag );
					GlobalFree( di->auth_info.username );
					GlobalFree( di->auth_info.password );

					if ( di->proxy_info != NULL )
					{
						GlobalFree( di->proxy_info->hostname );
						GlobalFree( di->proxy_info->punycode_hostname );
						GlobalFree( di->proxy_info->w_username );
						GlobalFree( di->proxy_info->w_password );
						GlobalFree( di->proxy_info->username );
						GlobalFree( di->proxy_info->password );
						GlobalFree( di->proxy_info->auth_key );
						GlobalFree( di->proxy_info );
					}

					while ( di->range_list != NULL )
					{
						DoublyLinkedList *range_node = di->range_list;
						di->range_list = di->range_list->next;

						GlobalFree( range_node->data );
						GlobalFree( range_node );
					}

					DeleteCriticalSection( &di->di_cs );

					GlobalFree( di->w_add_time );

					if ( di->hFile != INVALID_HANDLE_VALUE )
					{
						CloseHandle( di->hFile );
					}

					GlobalFree( di );
				}

				tln = TLV_NextNode( tln, true );
			}

			UpdateColumnOrders();

			DestroyMenus();

			if ( g_toolbar_imagelist != NULL )
			{
				_ImageList_Destroy( g_toolbar_imagelist );
			}

			if ( cfg_tray_icon )
			{
				if ( cfg_show_tray_progress )
				{
					UninitializeIconValues();
				}

				// Remove the icon from the notification area.
				_Shell_NotifyIconW( NIM_DELETE, &g_nid );
			}

			if ( use_drag_and_drop_main )
			{
				UnregisterDropWindow( g_hWnd_tlv_files, List_DropTarget );

				_OleUninitialize();
			}

			if ( use_taskbar_progress_main )
			{
				if ( g_taskbar != NULL )
				{
					g_taskbar->lpVtbl->Release( g_taskbar );
				}

				_CoUninitialize();
			}

			if ( tooltip_buffer != NULL )
			{
				GlobalFree( tooltip_buffer );
				tooltip_buffer = NULL;
			}

			_DestroyWindow( hWnd );

			return TRUE;
		}
		break;

		case WM_DESTROY:
		{
			_PostQuitMessage( 0 );
			return 0;
		}
		break;

		case WM_ENDSESSION:
		{
			// Save before we shut down/restart/log off of Windows.
			save_config();

			if ( site_list_changed )
			{
				save_site_info();

				site_list_changed = false;
			}

			if ( sftp_fps_host_list_changed )
			{
				save_sftp_fps_host_info();

				sftp_fps_host_list_changed = false;
			}

			if ( sftp_keys_host_list_changed )
			{
				save_sftp_keys_host_info();

				sftp_keys_host_list_changed = false;
			}

			if ( cfg_enable_download_history && g_download_history_changed )
			{
				_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\download_history\0", 18 );
				g_base_directory[ g_base_directory_length + 17 ] = 0;	// Sanity.

				save_download_history( g_base_directory );
				g_download_history_changed = false;
			}

			return 0;
		}
		break;

		case WM_RESET_PROGRESS:
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lParam;
			if ( di != NULL )
			{
				di->print_range_list = NULL;
			}

			return TRUE;
		}
		break;

		default:
		{
			if ( msg == WM_TASKBARBUTTONCREATED )
			{
				#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_taskbar_progress_main = InitializeOle32();
				}
				#endif

				if ( use_taskbar_progress_main )
				{
					_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

					_CoCreateInstance( _CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, _IID_ITaskbarList3, ( void ** )&g_taskbar );
				}
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	//return TRUE;
}
