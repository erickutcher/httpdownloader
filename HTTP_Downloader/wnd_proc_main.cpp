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

#include "lite_gdi32.h"
#include "lite_ole32.h"
#include "lite_comctl32.h"
#include "lite_winmm.h"

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

#include "categories.h"

#include "dark_mode.h"

HWND g_hWnd_toolbar = NULL;
HWND g_hWnd_files_columns = NULL;		// The header control window for the listview.
HWND g_hWnd_categories = NULL;
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

HCURSOR g_splitter_cursor = NULL;
bool g_splitter_moving = false;

//

HTREEITEM g_cat_tv_start_item = NULL;
HTREEITEM g_cat_tv_end_item = NULL;
bool g_cat_tv_dragging = false;
bool g_cat_tv_oob = false;	// Out of bounds.
RECT g_cat_lb_item_rc;

//

unsigned char g_total_columns = 0;

unsigned long long g_session_total_downloaded = 0;
unsigned long long g_session_downloaded_speed = 0;

unsigned long long g_session_last_total_downloaded = 0;
unsigned long long g_session_last_downloaded_speed = 0;

wchar_t *g_size_prefix[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB" };

bool last_menu = false;					// true if context menu was last open, false if main menu was last open. See: WM_ENTERMENULOOP

HANDLE g_timer_semaphore = NULL;
HANDLE g_timer_exit_semaphore = NULL;

bool use_drag_and_drop_main = true;		// Assumes OLE32_STATE_RUNNING is true.
IDropTarget *List_DropTarget;
IDropTarget *Tree_DropTarget;

bool use_taskbar_progress_main = false;	// Assume WM_TASKBARBUTTONCREATED is never called (Wine does this).
_ITaskbarList3 *g_taskbar = NULL;

struct PROGRESS_INFO
{
	unsigned long long current_total_downloaded;
	unsigned long long current_total_file_size;
	unsigned char download_state;	// 0 = Downloading, 1 = Completed
};

PROGRESS_INFO g_progress_info;

UINT WM_TASKBARCREATED = 0;
UINT WM_TASKBARBUTTONCREATED = 0;

#define IDT_UPDATE_CHECK_TIMER	10000

UINT current_dpi_main = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_main = 0;
HFONT hFont_main = NULL;

#define _SCALE_M_( x )						_SCALE_( ( x ), dpi_main )

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
			g_hWnd_check_for_updates = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_check_for_updates", ST_V_Check_For_Updates, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, 0, 441, 137, NULL, NULL, NULL, NULL );

			g_update_check_state = 2;	// Automatic update check.

			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, NULL, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
	}

	_KillTimer( hWnd, IDT_UPDATE_CHECK_TIMER );
}

HIMAGELIST UpdateToolbarIcons( HWND hWnd )
{
	HBITMAP hBmp;

	_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\toolbar.bmp\0", 13 );
	if ( GetFileAttributesW( g_program_directory ) != INVALID_FILE_ATTRIBUTES )
	{
		hBmp = ( HBITMAP )_LoadImageW( NULL, g_program_directory, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION );
	}
	else
	{
		hBmp = ( HBITMAP )_LoadImageW( GetModuleHandleW( NULL ), MAKEINTRESOURCE( IDB_BITMAP_TOOLBAR ), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	}

	HDC hDC = _GetDC( hWnd );

	HDC hdcMem_bmp = _CreateCompatibleDC( hDC );
	HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem_bmp, hBmp );
	_DeleteObject( ohbm );

	BITMAP bmp;
	_memzero( &bmp, sizeof( BITMAP ) );
	_GetObjectW( hBmp, sizeof( BITMAP ), &bmp );

	int res_height = _SCALE_M_( 24 );
	//int res_width = _SCALE_M_( 312 );
	int res_width = res_height * 13;	// Ensures proportionality.

	HBITMAP hBmp_scaled = _CreateCompatibleBitmap( hDC, res_width, res_height );

	HDC hdcMem_scaled = _CreateCompatibleDC( hDC );
	ohbm = ( HBITMAP )_SelectObject( hdcMem_scaled, hBmp_scaled );
	_DeleteObject( ohbm );

	_SetStretchBltMode( hdcMem_scaled, COLORONCOLOR );
	_StretchBlt( hdcMem_scaled, 0, 0, res_width, res_height, hdcMem_bmp, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY );

	_DeleteDC( hdcMem_scaled );
	_DeleteDC( hdcMem_bmp );
	_ReleaseDC( hWnd, hDC );

	// bmBitsPixel should match the ILC_COLOR4, ILC_COLOR8, etc. masks.
	/*if ( bmp.bmBitsPixel < 4 || bmp.bmBitsPixel > 32 )
	{
		bmp.bmBitsPixel = 8;	// ILC_COLOR8
	}*/
	// Height and width of each icon is the same.
	HIMAGELIST hil = _ImageList_Create( res_height, res_height, ILC_MASK | ILC_COLOR32, 13, 0 );
	_ImageList_AddMasked( hil, hBmp_scaled, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ) );

	_DeleteObject( hBmp_scaled );
	_DeleteObject( hBmp );

	return hil;
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
			g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
			g_nid.hIcon = g_default_tray_icon;
			_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
		}
	}
}

void ResetSessionStatus()
{
	// Reset.
	_memzero( g_session_status_count, sizeof( unsigned int ) * NUM_SESSION_STATUS );
}

bool FormatTooltipStatus()
{
	unsigned int buf_length = 0;

	wchar_t *status_strings[ NUM_SESSION_STATUS ] = { ST_V_Completed, ST_V_Stopped, ST_V_Timed_Out, ST_V_Failed, ST_V_File_IO_Error, ST_V_Skipped, ST_V_Authorization_Required, ST_V_Proxy_Authentication_Required, ST_V_Insufficient_Disk_Space };

	SetNotificationTitle( ST_V_Downloads_Have_Finished, ST_L_Downloads_Have_Finished );	// Downloads Have Finished

	for ( unsigned char i = 0; i < NUM_SESSION_STATUS; ++i )
	{
		if ( g_session_status_count[ i ] > 0 )
		{
			int ret = __snwprintf( g_nid.szInfo + buf_length, ( sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) ) - buf_length, L"%s%s: %lu", ( buf_length > 0 ? L"\r\n" : L"" ), status_strings[ i ], g_session_status_count[ i ] );

			if ( ret >= 0 )
			{
				buf_length += ret;
			}
			else
			{
				g_nid.szInfo[ ( sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) ) - 1 ] = 0;	// Sanity.

				break;
			}
		}
	}

	return ( buf_length != 0 ? true : false );
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

void UpdateSBItemCount()
{
	wchar_t status_bar_buf[ 64 ];
	unsigned char buf_length;

	buf_length = ( unsigned char )( ST_L_Items_ > 52 ? 52 : ST_L_Items_ ); // Let's not overflow. 64 - ( ' ' + 10 + NULL ) = 52 remaining bytes for our string.
	_wmemcpy_s( status_bar_buf, 64, ST_V_Items_, buf_length );
	status_bar_buf[ buf_length++ ] = ' ';

	if ( g_status_filter == STATUS_NONE )
	{
		__snwprintf( status_bar_buf + buf_length, 64 - buf_length, L"%lu", TLV_GetParentItemNodeCount() );
	}
	else
	{
		__snwprintf( status_bar_buf + buf_length, 64 - buf_length, L"%lu/%lu", TLV_GetRootItemCount(), TLV_GetParentItemNodeCount() );
	}

	_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 0, ( LPARAM )status_bar_buf );
	_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )status_bar_buf );
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

		_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, FALSE, FALSE );

		update_text_values = false;

		// Update our status bar with the download speed.
		if ( g_session_downloaded_speed != g_session_last_downloaded_speed )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + speed_buf_length, 128 - speed_buf_length, cfg_t_status_down_speed, g_session_downloaded_speed ) + speed_buf_length;
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
			sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 1, ( LPARAM )sb_download_speed_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )sb_download_speed_buf );

			g_session_last_downloaded_speed = g_session_downloaded_speed;

			update_text_values = true;
		}

		// Update our status bar with the download total.
		if ( g_session_total_downloaded != g_session_last_total_downloaded )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + download_buf_length, 128 - download_buf_length, cfg_t_status_downloaded, g_session_total_downloaded ) + download_buf_length;
			// NULL terminator is set in FormatSizes.

			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 2, ( LPARAM )sb_downloaded_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 2, 0 ), ( LPARAM )sb_downloaded_buf );

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
					g_nid.dwInfoFlags &= ~NIIF_NOSOUND;

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
			if ( !g_in_list_edit_mode &&
				 cfg_sort_added_and_updating_items &&
				 cfg_sorted_column_index != COLUMN_NUM &&
				 cfg_sorted_column_index != COLUMN_DATE_AND_TIME_ADDED &&
				 cfg_sorted_column_index != COLUMN_CATEGORY &&
				 cfg_sorted_column_index != COLUMN_DOWNLOAD_DIRECTORY &&
				 cfg_sorted_column_index != COLUMN_URL )
			{
				SORT_INFO si;
				si.column = cfg_sorted_column_index;
				si.hWnd = g_hWnd_tlv_files;
				si.direction = cfg_sorted_direction;

				_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
			}

			if ( g_refresh_list != 0x00 )
			{
				if ( g_refresh_list & 0x08 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x08 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_SELECT, NULL, NULL );
				}

				if ( g_refresh_list & 0x04 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x04 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_DRAG, NULL, NULL );
				}

				if ( g_refresh_list & 0x02 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x02 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_EDIT, NULL, NULL );
				}

				if ( g_refresh_list & 0x01 )
				{
					UpdateSBItemCount();

					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x01 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, FALSE );
				}
			}
		}
		else
		{
			_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )PROGRAM_CAPTION );

			g_progress_info.download_state = 1;	// Completed.

			bool error = ( ( g_session_status_count[ 2 ] > 0 || g_session_status_count[ 3 ] > 0 || g_session_status_count[ 4 ] > 0 || g_session_status_count[ 8 ] > 0 ) ? true : false );
			progress_color_t = cfg_color_t_e_p;
			border_color_t = cfg_color_t_e_b;

			progress_color_d = cfg_color_d_e_p;
			border_color_d = cfg_color_d_e_b;

			if ( g_taskbar != NULL )
			{
				// If Timed Out, Failed, File IO Error, or Insufficient Disk Space
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
					if ( FormatTooltipStatus() )
					{
						g_nid.uFlags |= NIF_INFO;
						if ( ( cfg_play_sound && cfg_sound_file_path != NULL ) ||
							 ( cfg_play_sound_fail && cfg_sound_fail_file_path != NULL ) )
						{
							g_nid.dwInfoFlags |= NIIF_NOSOUND;
						}
						else
						{
							g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
						}
					}
					else	// It's possible that nothing was downloaded (unsupported URL).
					{
						g_nid.uFlags &= ~NIF_INFO;
						g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
					}
				}
				else
				{
					g_nid.uFlags &= ~NIF_INFO;
					g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
				}
				g_nid.szTip[ 15 ] = 0;	// Sanity. PROGRAM_CAPTION length
				_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
			}

			ResetSessionStatus();

			// Sort all values that can change during a download.
			if ( !g_in_list_edit_mode &&
				 cfg_sort_added_and_updating_items &&
				 cfg_sorted_column_index != COLUMN_NUM &&
				 cfg_sorted_column_index != COLUMN_DATE_AND_TIME_ADDED &&
				 cfg_sorted_column_index != COLUMN_CATEGORY &&
				 cfg_sorted_column_index != COLUMN_DOWNLOAD_DIRECTORY &&
				 cfg_sorted_column_index != COLUMN_URL )
			{
				SORT_INFO si;
				si.column = cfg_sorted_column_index;
				si.hWnd = g_hWnd_tlv_files;
				si.direction = cfg_sorted_direction;

				_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
			}

			if ( g_refresh_list != 0x00 )
			{
				if ( g_refresh_list & 0x08 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x08 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_SELECT, NULL, NULL );
				}

				if ( g_refresh_list & 0x04 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x04 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_DRAG, NULL, NULL );
				}

				if ( g_refresh_list & 0x02 )
				{
					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x02 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_CANCEL_EDIT, NULL, NULL );
				}

				if ( g_refresh_list & 0x01 )
				{
					UpdateSBItemCount();

					InterlockedExchange( &g_refresh_list, ( g_refresh_list & ~0x01 ) );
					_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, FALSE );
				}
			}

			if ( !in_worker_thread )
			{
				UpdateMenus( true );
			}

			if ( ( cfg_play_sound && cfg_sound_file_path != NULL ) ||
				 ( cfg_play_sound_fail && cfg_sound_fail_file_path != NULL ) )
			{
				bool play = true;
				#ifndef WINMM_USE_STATIC_LIB
					if ( winmm_state == WINMM_STATE_SHUTDOWN )
					{
						play = false;	// Should have been loaded in main if cfg_play_sound or cfg_play_sound_fail was true.
					}
				#endif

				if ( play )
				{
					if ( error )
					{
						if ( cfg_play_sound_fail && cfg_sound_fail_file_path != NULL )
						{
							_PlaySoundW( cfg_sound_fail_file_path, NULL, SND_ASYNC | SND_FILENAME );
						}
					}
					else
					{
						if ( cfg_play_sound && cfg_sound_file_path != NULL )
						{
							_PlaySoundW( cfg_sound_file_path, NULL, SND_ASYNC | SND_FILENAME );
						}
					}
				}
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

	if ( g_timer_exit_semaphore != NULL )
	{
		ReleaseSemaphore( g_timer_exit_semaphore, 1, NULL );
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

		case COLUMN_CATEGORY:
		{
			buf = di->shared_info->category;
		}
		break;

		case COLUMN_COMMENTS:
		{
			if ( di->comments != NULL )
			{
				buf = tbuf;	// Reset the buffer pointer.

				// tbuf_size is always 128, so this will never happen.
				//if ( tbuf_size <= 4 )
				//{
				//	buf = L"...";
				//}
				//else
				//{
					int buf_length = __snwprintf( buf, tbuf_size, L"%.*s", tbuf_size, di->comments );
					if ( buf_length >= tbuf_size )
					{
						buf[ tbuf_size - 4 ] = L'.';
						buf[ tbuf_size - 3 ] = L'.';
						buf[ tbuf_size - 2 ] = L'.';
						buf[ tbuf_size - 1 ] = 0;	// Sanity.
					}
				//}
			}
			else
			{
				buf = L"";
			}
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
					if ( di->code != 0 && di->url != NULL )
					{
						if ( di->url[ 0 ] == L'h' || di->url[ 0 ] == L'H' )
						{
							__snwprintf( buf, tbuf_size, L"%s - %d.%1d%% - HTTP %d", ST_V_Failed, i_percentage, remainder, di->code );
						}
						else if ( di->url[ 0 ] == L'f' || di->url[ 0 ] == L'F' )
						{
							__snwprintf( buf, tbuf_size, L"%s - %d.%1d%% - FTP %d", ST_V_Failed, i_percentage, remainder, di->code );
						}
						else if ( di->url[ 0 ] == L's' || di->url[ 0 ] == L'S' )
						{
							__snwprintf( buf, tbuf_size, L"%s - %d.%1d%% - SFTP %d", ST_V_Failed, i_percentage, remainder, di->code );
						}
						else	// Shouldn't happen.
						{
							__snwprintf( buf, tbuf_size, L"%s - %d.%1d%% - %d", ST_V_Failed, i_percentage, remainder, di->code );
						}
					}
					else
					{
						__snwprintf( buf, tbuf_size, L"%s - %d.%1d%%", ST_V_Failed, i_percentage, remainder );
					}
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
				else if ( di->status == STATUS_INSUFFICIENT_DISK_SPACE )
				{
					buf = ST_V_Insufficient_Disk_Space;
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
				if ( di->code != 0 && di->url != NULL )
				{
					if ( di->url[ 0 ] == L'h' || di->url[ 0 ] == L'H' )
					{
						__snwprintf( buf, tbuf_size, L"%s - HTTP %d", ST_V_Failed, di->code );
					}
					else if ( di->url[ 0 ] == L'f' || di->url[ 0 ] == L'F' )
					{
						__snwprintf( buf, tbuf_size, L"%s - FTP %d", ST_V_Failed, di->code );
					}
					else if ( di->url[ 0 ] == L's' || di->url[ 0 ] == L'S' )
					{
						__snwprintf( buf, tbuf_size, L"%s - SFTP %d", ST_V_Failed, di->code );
					}
					else	// Shouldn't happen.
					{
						__snwprintf( buf, tbuf_size, L"%s - %d", ST_V_Failed, di->code );
					}
				}
				else
				{
					buf = ST_V_Failed;
				}
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
			else if ( di->status == STATUS_INSUFFICIENT_DISK_SPACE )
			{
				buf = ST_V_Insufficient_Disk_Space;
			}
			else	// Downloading.
			{
				buf = L"\x221E\0";	// Infinity symbol.
			}
		}
		break;

		case COLUMN_SSL_TLS_VERSION:
		{
			if ( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
			{
				buf = ST_V__DATA_;
			}
			else
			{
				switch ( di->ssl_version )
				{
					case 0: { buf = ST_V_SSL_2_0; } break;
					case 1: { buf = ST_V_SSL_3_0; } break;
					case 2: { buf = ST_V_TLS_1_0; } break;
					case 3: { buf = ST_V_TLS_1_1; } break;
					case 4: { buf = ST_V_TLS_1_2; } break;
					case 5: { buf = ST_V_TLS_1_3; } break;
					default: { buf = L""; } break;
				}
			}
		}
		break;

		case COLUMN_TIME_ELAPSED:
		case COLUMN_TIME_REMAINING:
		{
			// Use the infinity symbol for remaining time if it can't be calculated.
			if ( column == COLUMN_TIME_REMAINING &&
			 ( ( di->status == STATUS_CONNECTING || IS_STATUS( di->status, STATUS_PAUSED ) ) ||
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
			current_dpi_main = __GetDpiForWindow( hWnd );
			last_dpi_main = 0;
			hFont_main = UpdateFont( current_dpi_main );

			g_hWnd_toolbar = _CreateWindowExW( WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL, CCS_NODIVIDER | WS_CHILDWINDOW | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | ( cfg_show_toolbar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			HWND hWnd_toolbar_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWnd_toolbar, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_toolbar, TB_SETTOOLTIPS, ( WPARAM )hWnd_toolbar_tooltip, 0 );

			g_toolbar_imagelist = UpdateToolbarIcons( hWnd );
			_SendMessageW( g_hWnd_toolbar, TB_SETIMAGELIST, 0, ( LPARAM )g_toolbar_imagelist );

			_SendMessageW( g_hWnd_toolbar, TB_BUTTONSTRUCTSIZE, ( WPARAM )sizeof( TBBUTTON ), 0 );

			// Allows us to use the iString value for tooltips.
			_SendMessageW( g_hWnd_toolbar, TB_SETMAXTEXTROWS, 0, 0 );

			TBBUTTON tbb[ 16 ] = 
			{
				{ MAKELONG( 0, 0 ),				  MENU_ADD_URLS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,					 ( INT_PTR )ST_V_Add_URL_s_ },
				{ MAKELONG( 1, 0 ),					MENU_REMOVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						 ( INT_PTR )ST_V_Remove },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 2, 0 ),					 MENU_START,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						  ( INT_PTR )ST_V_Start },
				{ MAKELONG( 3, 0 ),					 MENU_PAUSE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						  ( INT_PTR )ST_V_Pause },
				{ MAKELONG( 4, 0 ),					  MENU_STOP,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						   ( INT_PTR )ST_V_Stop },
				{ MAKELONG( 5, 0 ),				   MENU_RESTART,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						( INT_PTR )ST_V_Restart },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 6, 0 ),			MENU_START_INACTIVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		( INT_PTR )ST_V_Start___Resume_Inactive },
				{ MAKELONG( 7, 0 ),			  MENU_PAUSE_ACTIVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,				   ( INT_PTR )ST_V_Pause_Active },
				{ MAKELONG( 8, 0 ),				  MENU_STOP_ALL,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,					   ( INT_PTR )ST_V_Stop_All },
				{				 0,							 -1,				  0,	  BTNS_SEP,	{ 0 }, 0,										   NULL },
				{ MAKELONG( 9, 0 ),					MENU_SEARCH,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						 ( INT_PTR )ST_V_Search },
				{ MAKELONG( 10, 0 ),	MENU_GLOBAL_SPEED_LIMIT,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,	( INT_PTR )ST_V_Global_Download_Speed_Limit },
				{ MAKELONG( 11, 0 ),		  MENU_SITE_MANAGER,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,				   ( INT_PTR )ST_V_Site_Manager },
				{ MAKELONG( 12, 0 ),			   MENU_OPTIONS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,						( INT_PTR )ST_V_Options }
			};

			_SendMessageW( g_hWnd_toolbar, TB_ADDBUTTONS, 16, ( LPARAM )&tbb );

			//

			g_splitter_cursor = _LoadCursorW( NULL, MAKEINTRESOURCE( IDC_SIZEWE ) );

			g_hWnd_categories = _CreateWindowExW( TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS | TVS_SHOWSELALWAYS | TVS_NOTOOLTIPS/*TVS_INFOTIP*/ | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			CreateCategoryTreeView( g_hWnd_categories );
			if ( cfg_show_categories )
			{
				_SetFocus( g_hWnd_categories );
			}

			//

			g_hWnd_tlv_files = _CreateWindowW( L"TreeListView", NULL, WS_VSCROLL | WS_HSCROLL | WS_TABSTOP | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_status = _CreateWindowW( STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | SBARS_TOOLTIPS | WS_CHILDWINDOW | ( cfg_show_status_bar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_toolbar, WM_SETFONT, ( WPARAM )hFont_main, 0 );
			_SendMessageW( g_hWnd_categories, WM_SETFONT, ( WPARAM )hFont_main, 0 );
			_SendMessageW( g_hWnd_status, WM_SETFONT, ( WPARAM )hFont_main, 0 );

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
				RegisterDropWindow( g_hWnd_categories, &Tree_DropTarget );
			}

			int status_bar_widths[] = { 104, 312, 520, -1 };

			_SendMessageW( g_hWnd_status, SB_SETPARTS, 4, ( LPARAM )status_bar_widths );

			wchar_t status_bar_buf[ 128 ];
			unsigned char buf_length;

			buf_length = ( unsigned char )( ST_L_Items_ > 125 ? 125 : ST_L_Items_ ); // Let's not overflow. 128 - ( ' ' + '0' + NULL ) = 125 remaining bytes for our string.
			_wmemcpy_s( status_bar_buf, 128, ST_V_Items_, buf_length );
			status_bar_buf[ buf_length++ ] = ' ';
			status_bar_buf[ buf_length++ ] = '0';
			status_bar_buf[ buf_length ] = 0;

			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 0, ( LPARAM )status_bar_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )status_bar_buf );

			buf_length = ( unsigned char )( ST_L_Download_speed_ > 102 ? 102 : ST_L_Download_speed_ ); // Let's not overflow. 128 - ( ' ' + 22 +  '/' + 's' + NULL ) = 102 remaining bytes for our string.
			_wmemcpy_s( status_bar_buf, 128, ST_V_Download_speed_, buf_length );
			status_bar_buf[ buf_length++ ] = ' ';
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			unsigned int length = FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_down_speed, 0 ) + buf_length;
			status_bar_buf[ length++ ] = L'/';
			status_bar_buf[ length++ ] = L's';
			status_bar_buf[ length ] = 0;

			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 1, ( LPARAM )status_bar_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );


			buf_length = ( unsigned char )( ST_L_Total_downloaded_ > 104 ? 104 : ST_L_Total_downloaded_ ); // Let's not overflow. 128 - ( ' ' + 22 + NULL ) = 104 remaining bytes for our string.
			_wmemcpy_s( status_bar_buf, 128, ST_V_Total_downloaded_, buf_length );
			status_bar_buf[ buf_length++ ] = ' ';
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			FormatSizes( status_bar_buf + buf_length, 128 - buf_length, cfg_t_status_downloaded, 0 );
			// NULL terminator is set in FormatSizes.
			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 2, ( LPARAM )status_bar_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 2, 0 ), ( LPARAM )status_bar_buf );


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

			_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 3, ( LPARAM )status_bar_buf );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 3, 0 ), ( LPARAM )status_bar_buf );

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

				HANDLE timer_handle = _CreateThread( NULL, 0, UpdateWindow, NULL, 0, NULL );
				if ( timer_handle != NULL )
				{
					SetThreadPriority( timer_handle, THREAD_PRIORITY_LOWEST );
					CloseHandle( timer_handle );
				}
			}

			if ( cfg_enable_download_history )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				if ( iei != NULL )
				{
					// Include an empty string.
					iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH + 1 ) );
					if ( iei->file_paths != NULL )
					{
						_wmemcpy_s( iei->file_paths, MAX_PATH, g_base_directory, g_base_directory_length );
						_wmemcpy_s( iei->file_paths + ( g_base_directory_length + 1 ), MAX_PATH - ( g_base_directory_length - 1 ), L"download_history\0", 17 );
						//iei->file_paths[ g_base_directory_length + 17 ] = 0;	// Sanity.
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
					else
					{
						GlobalFree( iei );
					}
				}
			}

			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, load_category_list, ( void * )NULL, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}

			tooltip_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 512 );

			_memzero( &g_progress_info, sizeof( PROGRESS_INFO ) );

			WM_TASKBARCREATED = _RegisterWindowMessageW( L"TaskbarCreated" );
			WM_TASKBARBUTTONCREATED = _RegisterWindowMessageW( L"TaskbarButtonCreated" );

			if ( cfg_check_for_updates )
			{
				// Check after 10 seconds.
				_SetTimer( hWnd, IDT_UPDATE_CHECK_TIMER, 10000, ( TIMERPROC )UpdateCheckTimerProc );
			}

			RECT rc, rc2;

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
					_GetWindowRect( hWnd, &rc );
					_GetClientRect( hWnd, &rc2 );

					g_border_width = ( ( rc.right - rc.left - rc2.right ) / 2 ) - 1; // Leave the 1 px border.
				}
			}

			int width = _SCALE_M_( cfg_width );
			int height = _SCALE_M_( cfg_height );

			rc.left = cfg_pos_x;
			rc.top = cfg_pos_y;
			rc.right = rc.left + width;
			rc.bottom = rc.top + height;
			HMONITOR hMon = _MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			// If the window is offscreen, then move it into the current monitor.
			if ( rc.right <= mi.rcMonitor.left ||
				 rc.left >= mi.rcMonitor.right ||
				 rc.bottom <= mi.rcMonitor.top ||
				 rc.top >= mi.rcMonitor.bottom )
			{
				rc.left = mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 );
				rc.top = mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 );
			}
			_SetWindowPos( hWnd, NULL, rc.left, rc.top, width, height, 0 );

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
			// Get our window codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case NM_RCLICK:
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					if ( nmhdr->hwndFrom == g_hWnd_toolbar || nmhdr->hwndFrom == g_hWnd_status )
					{
						POINT p;
						_GetCursorPos( &p );

						_TrackPopupMenu( g_hMenuSub_view, 0, p.x, p.y, 0, hWnd, NULL );
					}
					else if ( nmhdr->hwndFrom == g_hWnd_categories )
					{
						POINT p;
						_GetCursorPos( &p );

						TVHITTESTINFO tvht;
						_memzero( &tvht, sizeof( TVHITTESTINFO ) );
						tvht.pt.x = p.x;
						tvht.pt.y = p.y;
						_ScreenToClient( g_hWnd_categories, &tvht.pt );
						HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_HITTEST, 0, ( LPARAM )&tvht );
						if ( hti != NULL )
						{
							_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti );
							_SetFocus( g_hWnd_categories );

							TVITEM tvi;
							_memzero( &tvi, sizeof( TVITEM ) );
							tvi.mask = TVIF_PARAM;
							tvi.hItem = hti;
							_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvi );

							DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
							if ( dll_node != NULL )
							{
								CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
								if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO )
								{
									if ( cti->data != NULL )	// Update/Remove
									{
										_TrackPopupMenu( g_hMenuSub_categories_update_remove, 0, p.x, p.y, 0, hWnd, NULL );
									}
									else	// Add
									{
										_TrackPopupMenu( g_hMenuSub_categories_add, 0, p.x, p.y, 0, hWnd, NULL );
									}
								}
							}
						}
						else
						{
							_TrackPopupMenu( g_hMenuSub_categories_add, 0, p.x, p.y, 0, hWnd, NULL );
						}
					}
				}
				break;

				case NM_CLICK:
				{
					NMMOUSE *nmm = ( NMMOUSE * )lParam;

					// Change the format of the panel if Ctrl is held while clicking the panel.
					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						if ( nmm->hdr.hwndFrom == g_hWnd_status )
						{
							wchar_t status_bar_buf[ 128 ];
							unsigned char buf_length;

							if ( nmm->dwItemSpec == 1 )
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

								_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 1, ( LPARAM )status_bar_buf );
								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );
							}
							else if ( nmm->dwItemSpec == 2 )
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

								_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 2, ( LPARAM )status_bar_buf );
								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 2, 0 ), ( LPARAM )status_bar_buf );
							}
							else if ( nmm->dwItemSpec == 3 )
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

								_SendMessageW( g_hWnd_status, SB_SETTIPTEXT, 3, ( LPARAM )status_bar_buf );
								_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 3, 0 ), ( LPARAM )status_bar_buf );
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
							if ( ( ( NMMOUSE * )lParam )->dwItemSpec == 3 )
							{
								_SendMessageW( hWnd, WM_COMMAND, MENU_GLOBAL_SPEED_LIMIT, 0 );
							}
						}
					}
				}
				break;

				case TVN_SELCHANGED:			// The tree item that gains focus
				{
					NMTREEVIEW *nmtv = ( NMTREEVIEW * )lParam;

					unsigned int status = UINT_MAX;
					wchar_t *category = NULL;

					DoublyLinkedList *dll_node = ( DoublyLinkedList * )nmtv->itemNew.lParam;
					if ( dll_node != NULL )
					{
						CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
						if ( cti != NULL )
						{
							if ( cti->type == CATEGORY_TREE_INFO_TYPE_STATUS )
							{
								status = ( unsigned int )cti->data;
							}
							else if ( cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
							{
								CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;

								status = UINT_MAX - 1;

								SHARED_CATEGORY_INFO *sci = ( SHARED_CATEGORY_INFO * )dllrbt_find( g_shared_categories, ( void * )ci->category, true );
								if ( sci != NULL )
								{
									category = sci->category;
								}
							}
						}
					}

					if ( status != UINT_MAX && ( TLV_GetStatusFilter() != status || TLV_GetCategoryFilter() != category ) )
					{
						RefreshSelectedFilter( status, category );
					}
				}
				break;

				case TVN_BEGINLABELEDIT:
				{
					NMTVDISPINFO *tvdi = ( NMTVDISPINFO * )lParam;

					DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvdi->item.lParam;
					if ( dll_node != NULL )
					{
						CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
						if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
						{
							HWND hWnd_categories_edit = ( HWND )_SendMessageW( g_hWnd_categories, TVM_GETEDITCONTROL, 0, 0 );
							if ( hWnd_categories_edit != NULL )
							{
								_SendMessageW( hWnd_categories_edit, EM_LIMITTEXT, MAX_PATH - 1, 0 );

								CategoriesEditProc = ( WNDPROC )_GetWindowLongPtrW( hWnd_categories_edit, GWLP_WNDPROC );
								_SetWindowLongPtrW( hWnd_categories_edit, GWLP_WNDPROC, ( LONG_PTR )CategoriesEditSubProc );

#ifdef ENABLE_DARK_MODE
								if ( g_use_dark_mode )
								{
									EnumChildProc( hWnd_categories_edit, NULL );
									EnumTLWProc( g_hWnd_categories, NULL );
								}
#endif

								return 0;
							}
						}
					}

					return TRUE;
				}
				break;

				case TVN_ENDLABELEDIT:
				{
					NMTVDISPINFO *tvdi = ( NMTVDISPINFO * )lParam;

					if ( tvdi->item.pszText != NULL && tvdi->item.pszText[ 0 ] != 0 )
					{
						DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvdi->item.lParam;
						if ( dll_node != NULL )
						{
							CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
							if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
							{
								CATEGORY_INFO_ *old_ci = ( CATEGORY_INFO_ * )cti->data;
								if ( lstrcmpW( old_ci->category, tvdi->item.pszText ) != 0 )
								{
									CATEGORY_UPDATE_INFO *cui = ( CATEGORY_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_UPDATE_INFO ) );
									if ( cui != NULL )
									{
										cui->old_ci = old_ci;
										cui->update_type = 1;	// Update
										cui->hti = tvdi->item.hItem;

										CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )GlobalAlloc( GPTR, sizeof( CATEGORY_INFO_ ) );
										if ( ci != NULL )
										{
											ci->category = GlobalStrDupW( tvdi->item.pszText );

											ci->download_directory = GlobalStrDupW( old_ci->download_directory );

											cui->ci = ci;

											// cui is freed in handle_category_list.
											HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_category_list, ( void * )cui, 0, NULL );
											if ( thread != NULL )
											{
												CloseHandle( thread );
											}
											else
											{
												FreeCategoryInfo( &cui->ci );
												GlobalFree( cui );
											}
										}
										else
										{
											GlobalFree( cui );
										}
									}
								}
							}
						}
					}
				}
				break;

				case TVN_BEGINDRAG:
				{
					NMTREEVIEW *nmtv = ( NMTREEVIEW * )lParam;
					if ( nmtv != NULL && nmtv->itemNew.lParam != NULL )
					{
						DoublyLinkedList *dll_node = ( DoublyLinkedList * )nmtv->itemNew.lParam;
						if ( dll_node->data != NULL )
						{
							CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
							if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
							{
								g_cat_tv_start_item = g_cat_tv_end_item = nmtv->itemNew.hItem;

								*( HTREEITEM * )&g_cat_lb_item_rc = g_cat_tv_start_item;
								_SendMessageW( nmtv->hdr.hwndFrom, TVM_GETITEMRECT, TRUE, ( LPARAM )&g_cat_lb_item_rc );

								_SetCapture( hWnd );

								g_cat_tv_dragging = true;
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
						cfg_width = _UNSCALE_( wp->cx, dpi_main );
						cfg_height = _UNSCALE_( wp->cy, dpi_main );
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
			if ( wParam != SIZE_MINIMIZED )
			{
				RECT rc, rc_toolbar, rc_status;
				_GetClientRect( hWnd, &rc );

				// Allow our listview to resize in proportion to the main window.
				HDWP hdwp = _BeginDeferWindowPos( ( cfg_show_categories ? 2 : 1 ) );

				if ( cfg_show_toolbar )
				{
					_SendMessageW( g_hWnd_toolbar, TB_AUTOSIZE, 0, 0 ); 

					_GetWindowRect( g_hWnd_toolbar, &rc_toolbar );

					rc.top = rc_toolbar.bottom - rc_toolbar.top;
					rc.bottom -= rc.top;
				}

				if ( cfg_show_status_bar )
				{
					// Status bar will automatically resize its height to fit the font. No need to scale the height.
					_GetWindowRect( g_hWnd_status, &rc_status );
					int status_bar_height = rc_status.bottom - rc_status.top;

					if ( cfg_show_categories )
					{
						_DeferWindowPos( hdwp, g_hWnd_categories, HWND_TOP, rc.left, rc.top, _SCALE_M_( cfg_splitter_pos_x - SPLITTER_WIDTH ), rc.bottom - status_bar_height, SWP_NOZORDER );
						_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left + _SCALE_M_( cfg_splitter_pos_x ), rc.top, rc.right - _SCALE_M_( cfg_splitter_pos_x ), rc.bottom - status_bar_height, SWP_NOZORDER );
					}
					else
					{
						_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom - status_bar_height, SWP_NOZORDER );
					}

					//int status_bar_widths[] = { 104, 312, 520, -1 };
					int sbi_width = ( ( rc.right - rc.left ) - _SCALE_M_( 160 ) ) / 3;
					int status_bar_widths[ 4 ];
					status_bar_widths[ 0 ] = sbi_width / 2;
					status_bar_widths[ 1 ] = status_bar_widths[ 0 ] + sbi_width;
					status_bar_widths[ 2 ] = status_bar_widths[ 1 ] + sbi_width;
					status_bar_widths[ 3 ] = -1;
					_SendMessageW( g_hWnd_status, SB_SETPARTS, 4, ( LPARAM )status_bar_widths );

					// Apparently status bars want WM_SIZE to be called. (See MSDN)
					_SendMessageW( g_hWnd_status, WM_SIZE, 0, 0 );
				}
				else
				{
					if ( cfg_show_categories )
					{
						_DeferWindowPos( hdwp, g_hWnd_categories, HWND_TOP, rc.left, rc.top, _SCALE_M_( cfg_splitter_pos_x - SPLITTER_WIDTH ), rc.bottom, SWP_NOZORDER );
						_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left + _SCALE_M_( cfg_splitter_pos_x ), rc.top, rc.right - _SCALE_M_( cfg_splitter_pos_x ), rc.bottom, SWP_NOZORDER );
					}
					else
					{
						_DeferWindowPos( hdwp, g_hWnd_tlv_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER );
					}
				}

				_EndDeferWindowPos( hdwp );

				if ( wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED )
				{
					ClearProgressBars();
				}
			}

			return 0;
		}
		break;

		case WM_MOUSEMOVE:
		{
			if ( g_cat_tv_dragging )
			{
				POINT pt;
				pt.x = GET_X_LPARAM( lParam );
				pt.y = GET_Y_LPARAM( lParam );
				_ClientToScreen( hWnd, &pt );
				_ScreenToClient( g_hWnd_categories, &pt );

				TVHITTESTINFO tvht;
				_memzero( &tvht, sizeof( TVHITTESTINFO ) );
				tvht.pt.x = pt.x;
				tvht.pt.y = pt.y;
				HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_HITTEST, 0, ( LPARAM )&tvht );
				if ( hti != NULL )
				{
					HTREEITEM hti_parent = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_PARENT, ( LPARAM )hti );
					if ( hti_parent == g_hti_categories /*|| hti == g_hti_categories*/ )
					{
						g_cat_tv_oob = false;

						*( HTREEITEM * )&g_cat_lb_item_rc = hti;
						_SendMessageW( g_hWnd_categories, TVM_GETITEMRECT, TRUE, ( LPARAM )&g_cat_lb_item_rc );
					}
				}
				else if ( !g_cat_tv_oob && pt.y > 0 )
				{
					g_cat_tv_oob = true;

					int height = g_cat_lb_item_rc.bottom - g_cat_lb_item_rc.top;
					g_cat_lb_item_rc.top += height;
					g_cat_lb_item_rc.bottom += height;
				}

				RECT rc;
				_GetClientRect( g_hWnd_categories, &rc );
				g_cat_lb_item_rc.left = rc.left;
				g_cat_lb_item_rc.right = rc.right;

				HDC hDC = _GetDC( g_hWnd_categories );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, rc.right - rc.left, rc.bottom - rc.top );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );

				// Fill the background.
				HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
				_FillRect( hdcMem, &rc, color );
				_DeleteObject( color );

				_SendMessageW( g_hWnd_categories, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT ) );

				HPEN line_color;

#ifdef ENABLE_DARK_MODE
				if ( g_use_dark_mode )
				{
					line_color = _CreatePen( PS_DOT, 1, dm_color_list_highlight );
				}
				else
#endif
				{
					line_color = _CreatePen( PS_DOT, 1, ( COLORREF )_GetSysColor( COLOR_HOTLIGHT ) );
				}

				HPEN old_color = ( HPEN )_SelectObject( hdcMem, line_color );
				_DeleteObject( old_color );
				_MoveToEx( hdcMem, g_cat_lb_item_rc.left, g_cat_lb_item_rc.top, NULL );
				_LineTo( hdcMem, g_cat_lb_item_rc.right, g_cat_lb_item_rc.top );
				_DeleteObject( line_color );

				_BitBlt( hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0, SRCCOPY );

				_DeleteObject( hbm );

				_DeleteDC( hdcMem );
				_ReleaseDC( g_hWnd_categories, hDC );

				return TRUE;
			}
			else
			{
				_SetCursor( g_splitter_cursor );

				if ( ( wParam == MK_LBUTTON ) && g_splitter_moving )
				{
					RECT rc;
					_GetClientRect( hWnd, &rc );
					int splitter_pox_x = _UNSCALE_( GET_X_LPARAM( lParam ), dpi_main );
					if ( splitter_pox_x <= rc.right && splitter_pox_x >= SPLITTER_WIDTH )
					{
						cfg_splitter_pos_x = splitter_pox_x;

						_SendMessageW( hWnd, WM_SIZE, 0, MAKELPARAM( rc.right, rc.bottom ) );
					}
				}

				return 0;
			}
		}
		break;

		case WM_LBUTTONDOWN:
		{
			_SetCursor( g_splitter_cursor );

			g_splitter_moving = true;

			_SetCapture( hWnd );

			return 0;
		}
		break;

		case WM_LBUTTONUP:
		{
			if ( g_cat_tv_dragging ) 
			{
				POINT pt;
				pt.x = GET_X_LPARAM( lParam );
				pt.y = GET_Y_LPARAM( lParam );
				_ClientToScreen( hWnd, &pt );
				_ScreenToClient( g_hWnd_categories, &pt );

				TVHITTESTINFO tvht;
				_memzero( &tvht, sizeof( TVHITTESTINFO ) );
				tvht.pt.x = pt.x;
				tvht.pt.y = pt.y;
				g_cat_tv_end_item = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_HITTEST, 0, ( LPARAM )&tvht );
				if ( g_cat_tv_end_item != g_cat_tv_start_item )
				{
					TVINSERTSTRUCT tvis;
					_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );
					tvis.item.mask = TVIF_PARAM;
					tvis.item.hItem = g_cat_tv_start_item;
					_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvis.item );
					_SendMessageW( g_hWnd_categories, TVM_DELETEITEM, 0, ( LPARAM )g_cat_tv_start_item );

					tvis.hParent = g_hti_categories;

					DoublyLinkedList *dll_node = NULL;
					if ( tvis.item.lParam != NULL )
					{
						dll_node = ( DoublyLinkedList * )tvis.item.lParam;
						if ( dll_node->data != NULL )
						{
							CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
							if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
							{
								CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;
								tvis.item.pszText = ci->category;
							}
						}
					}

					HTREEITEM hti;
					if ( g_cat_tv_end_item != NULL )
					{
						hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_PARENT, ( LPARAM )g_cat_tv_end_item );
						if ( hti == g_hti_categories )
						{
							hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_PREVIOUS, ( LPARAM )g_cat_tv_end_item );
							tvis.hInsertAfter = ( hti != NULL ? hti : TVI_FIRST );
						}
						else
						{
							tvis.hInsertAfter = TVI_FIRST;
						}
					}
					else
					{
						tvis.hInsertAfter = TVI_LAST;
					}

					tvis.item.mask |= TVIF_TEXT;
					hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_INSERTITEM, 0, ( LPARAM )&tvis );
					_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti );

					hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_PREVIOUS, ( LPARAM )hti );
					tvis.item.mask = TVIF_PARAM;
					tvis.item.hItem = hti;
					_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvis.item );

					if ( tvis.item.lParam != NULL )
					{
						DoublyLinkedList *previous_dll_node = ( DoublyLinkedList * )tvis.item.lParam;
						
						// We're inserting at the head of the list.
						if ( previous_dll_node == dll_node )
						{
							// If we're moving a node that isn't already at the head.
							if ( dll_node != g_category_list )
							{
								DoublyLinkedList *prev = dll_node->prev;
								DoublyLinkedList *next = dll_node->next;

								DLL_RemoveNode( &g_treeview_list, dll_node );

								prev->next = next;
								dll_node->prev = g_category_list->prev;
								dll_node->next = g_category_list;
								g_category_list->prev->next = dll_node;
								g_category_list->prev = dll_node;
								g_category_list = dll_node;
							}
						}
						else
						{
							if ( previous_dll_node->next != dll_node )
							{
								if ( dll_node == g_category_list )
								{
									g_category_list = dll_node->next;
								}

								DLL_RemoveNode( &g_treeview_list, dll_node );

								// We're inserting at the end of the list.
								if ( previous_dll_node->next == NULL )
								{
									previous_dll_node->next = dll_node;
									dll_node->prev = previous_dll_node;
									g_treeview_list->prev = dll_node;
								}
								else
								{
									previous_dll_node->next->prev = dll_node;
									dll_node->next = previous_dll_node->next;
									previous_dll_node->next = dll_node;
									dll_node->prev = previous_dll_node;
								}
							}
						}
					}

					category_list_changed = true;
					g_update_add_category_window = true;
					g_update_update_category_window = true;
					g_update_site_manager_category_window = true;
				}

				g_cat_tv_oob = false;

				_ReleaseCapture();

				_InvalidateRect( g_hWnd_categories, NULL, TRUE );

				g_cat_tv_dragging = false;

				return TRUE;
			}
			else
			{
				_ReleaseCapture();

				g_splitter_moving = false;

				return 0;
			}
		}
		break;

		case WM_SIZING:
		{
			RECT *rc = ( RECT * )lParam;

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;

			cfg_width = _UNSCALE_( ( rc->right - rc->left ), dpi_main );
			cfg_height = _UNSCALE_( ( rc->bottom - rc->top ), dpi_main );

			return TRUE;
		}
		break;

		case WM_MOVING:
		{
			POINT cur_pos;
			//RECT wa;
			RECT *rc = ( RECT * )lParam;

			_GetCursorPos( &cur_pos );
			_OffsetRect( rc, cur_pos.x - ( rc->left + cx ), cur_pos.y - ( rc->top + cy ) );

			// Allow our main window to attach to the desktop edge.
			//_SystemParametersInfoW( SPI_GETWORKAREA, 0, &wa, 0 );
			HMONITOR hMon = _MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			if ( is_close( rc->left + g_border_width, mi.rcWork.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, mi.rcWork.left - rc->left - g_border_width, 0 );
			}
			else if ( is_close( mi.rcWork.right, rc->right - g_border_width ) )			// Attach to right side of the desktop.
			{
				_OffsetRect( rc, mi.rcWork.right - rc->right + g_border_width, 0 );
			}

			if ( is_close( rc->top, mi.rcWork.top ) )									// Attach to top of the desktop.
			{
				_OffsetRect( rc, 0, mi.rcWork.top - rc->top );
			}
			else if ( is_close( mi.rcWork.bottom, rc->bottom - g_border_width ) )		// Attach to bottom of the desktop.
			{
				_OffsetRect( rc, 0, mi.rcWork.bottom - rc->bottom + g_border_width );
			}

			// Save our settings for the position/dimensions of the window.
			cfg_pos_x = rc->left;
			cfg_pos_y = rc->top;

			cfg_width = _UNSCALE_( ( rc->right - rc->left ), dpi_main );
			cfg_height = _UNSCALE_( ( rc->bottom - rc->top ), dpi_main );

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

		case WM_GET_DPI:
		{
			return current_dpi_main;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_main;
			current_dpi_main = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_main );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_main );
			hFont_main = hFont;

			if ( g_toolbar_imagelist != NULL )
			{
				_ImageList_Destroy( g_toolbar_imagelist );
			}
			g_toolbar_imagelist = UpdateToolbarIcons( hWnd );
			_SendMessageW( g_hWnd_toolbar, TB_SETIMAGELIST, 0, ( LPARAM )g_toolbar_imagelist );

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_main == 0 )
			{
				RECT rc_mon;
				rc_mon.left = cfg_pos_x;
				rc_mon.top = cfg_pos_y;
				rc_mon.right = rc_mon.left + width;
				rc_mon.bottom = rc_mon.top + height;
				HMONITOR hMon = MonitorFromRect( &rc_mon, MONITOR_DEFAULTTONEAREST );	// This is a popup window and we can't use CW_USEDEFAULT. We'll place this window on the same monitor as the main window.
				MONITORINFO mi;
				mi.cbSize = sizeof( MONITORINFO );
				_GetMonitorInfoW( hMon, &mi );
				int pos_x = cfg_pos_x;
				int pos_y = cfg_pos_y;
				// If the window is offscreen, then move it into the current monitor.
				if ( pos_x + width <= mi.rcMonitor.left ||
					 pos_x >= mi.rcMonitor.right ||
					 pos_y + height <= mi.rcMonitor.top ||
					 pos_y >= mi.rcMonitor.bottom )
				{
					pos_x = mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 );
					pos_y = mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 );
				}
				_SetWindowPos( hWnd, NULL, pos_x, pos_y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER );
			}
			else
			{
				_SetWindowPos( hWnd, NULL, rc->left, rc->top, width, height, SWP_NOZORDER | SWP_NOACTIVATE );
			}

			last_dpi_main = last_dpi;

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
				case NIN_BALLOONUSERCLICK:
				{
					_ShowWindow( hWnd, SW_SHOW );
					_SetForegroundWindow( hWnd );
				}
				break;

				case WM_LBUTTONDBLCLK:
				{
					if ( _IsIconic( hWnd ) )	// If minimized, then restore the window.
					{
						_ShowWindow( hWnd, SW_RESTORE );
						_SetForegroundWindow( hWnd );
					}
				}
				break;

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

		case WM_PEER_CONNECTED:
		{
			if ( lParam != NULL )
			{
				SetNotificationTitle( ST_V_Remote_Connection, ST_L_Remote_Connection );	// Remote Connection

				int notification_length = __snwprintf( g_nid.szInfo, sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ), L"%s %lu", ST_V_URL_s__added_, ( unsigned int )wParam );

				wchar_t *peer_info = ( wchar_t * )lParam;

				int peer_info_length = lstrlenW( peer_info );
				if ( peer_info_length > 0 )
				{
					__snwprintf( g_nid.szInfo + notification_length, ( sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) ) - notification_length, L"\r\n\r\n%s", ( wchar_t * )lParam );
				}
				g_nid.uFlags |= NIF_INFO;
				g_nid.dwInfoFlags &= ~NIIF_NOSOUND;
				g_nid.szInfo[ ( sizeof( g_nid.szInfo ) / sizeof( g_nid.szInfo[ 0 ] ) ) - 1 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_MODIFY, &g_nid );
			}

			return 0;
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
				cla->category = ( cla->category_length > 0 ? cl_val + ( unsigned int )cla->category : NULL );
				cla->download_directory = ( cla->download_directory_length > 0 ? cl_val + ( unsigned int )cla->download_directory : NULL );
				cla->download_history_file = ( cla->download_history_file_length > 0 ? cl_val + ( unsigned int )cla->download_history_file : NULL );
				cla->url_list_file = ( cla->url_list_file_length > 0 ? cl_val + ( unsigned int )cla->url_list_file : NULL );
				cla->urls = ( cla->urls_length > 0 ? cl_val + ( unsigned int )cla->urls : NULL );
				cla->comments = ( cla->comments_length > 0 ? cl_val + ( unsigned int )cla->comments : NULL );
				cla->cookies = ( cla->cookies_length > 0 ? cl_val + ( unsigned int )cla->cookies : NULL );
				cla->data = ( cla->data_length > 0 ? cl_val + ( unsigned int )cla->data : NULL );
				cla->headers = ( cla->headers_length > 0 ? cl_val + ( unsigned int )cla->headers : NULL );
				cla->username = ( cla->username_length > 0 ? cl_val + ( unsigned int )cla->username : NULL );
				cla->password = ( cla->password_length > 0 ? cl_val + ( unsigned int )cla->password : NULL );
				cla->proxy_hostname = ( cla->proxy_hostname_length > 0 ? cl_val + ( unsigned int )cla->proxy_hostname : NULL );
				cla->proxy_username = ( cla->proxy_username_length > 0 ? cl_val + ( unsigned int )cla->proxy_username : NULL );
				cla->proxy_password = ( cla->proxy_password_length > 0 ? cl_val + ( unsigned int )cla->proxy_password : NULL );

				CL_ARGS *new_cla = ( CL_ARGS * )GlobalAlloc( GMEM_FIXED, sizeof( CL_ARGS ) );
				_memcpy_s( new_cla, sizeof( CL_ARGS ), cla, sizeof( CL_ARGS ) );

				if ( cla->category != NULL )
				{
					new_cla->category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->category_length + 1 ) );
					_wmemcpy_s( new_cla->category, cla->category_length + 1, cla->category, cla->category_length );
					new_cla->category[ cla->category_length ] = 0;	// Sanity.
				}

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

				if ( cla->comments != NULL )
				{
					new_cla->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->comments_length + 1 ) );
					_wmemcpy_s( new_cla->comments, cla->comments_length + 1, cla->comments, cla->comments_length );
					new_cla->comments[ cla->comments_length ] = 0;	// Sanity.
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
					g_hWnd_add_urls = _CreateWindowExW( ( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ) | ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_add_urls", ST_V_Add_URL_s_, WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 620 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 270 ) / 2 ), 620, 270, NULL, NULL, NULL, NULL );
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
					// Restart download (from the beginning).
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )3, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
				}
			}

			return TRUE;
		}
		break;

		case WM_ACTIVATE:
		{
			ClearProgressBars();

			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

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

			if ( g_hWnd_add_category != NULL )
			{
				_EnableWindow( g_hWnd_add_category, FALSE );
				_ShowWindow( g_hWnd_add_category, SW_HIDE );
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


			g_timer_exit_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

			g_end_program = true;

			// Exit our timer thread if it's active.
			if ( g_timer_semaphore != NULL )
			{
				ReleaseSemaphore( g_timer_semaphore, 1, NULL );
			}

			// 10 second timeout in case we miss the release.
			WaitForSingleObject( g_timer_exit_semaphore, 10000 );
			CloseHandle( g_timer_exit_semaphore );
			g_timer_exit_semaphore = NULL;

			// Release the semaphore to complete the update check.
			if ( g_update_semaphore != NULL )
			{
				ReleaseSemaphore( g_update_semaphore, 1, NULL );
			}

			// If we're in a secondary thread, then kill it (cleanly) and wait for it to exit.
			if ( in_worker_thread )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, cleanup, ( void * )NULL, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
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
			StopIOCPDownloader();

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

			if ( g_hWnd_add_category != NULL )
			{
				_DestroyWindow( g_hWnd_add_category );
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
				//g_base_directory[ g_base_directory_length + 17 ] = 0;	// Sanity.

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
					// di->category is stored in the g_shared_categories tree and is destroyed in main.
					GlobalFree( di->url );
					GlobalFree( di->comments );
					GlobalFree( di->cookies );
					GlobalFree( di->headers );
					GlobalFree( di->data );
					//GlobalFree( di->etag );
					GlobalFree( di->auth_info.username );
					GlobalFree( di->auth_info.password );

					if ( di->proxy_info != di->saved_proxy_info )
					{
						FreeProxyInfo( &di->saved_proxy_info );
					}
					FreeProxyInfo( &di->proxy_info );

					while ( di->range_list != NULL )
					{
						DoublyLinkedList *range_node = di->range_list;
						di->range_list = di->range_list->next;

						GlobalFree( range_node->data );
						GlobalFree( range_node );
					}

					DeleteCriticalSection( &di->di_cs );

					// The shared icon info will be cleaned up in main().
					// The shared category info will be cleaned up in main().
					// di->shared_info->comments is freed above.

					GlobalFree( di->new_file_path );
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
				UnregisterDropWindow( g_hWnd_categories, Tree_DropTarget );
				UnregisterDropWindow( g_hWnd_tlv_files, List_DropTarget );

				_OleUninitialize();
			}

			if ( use_taskbar_progress_main )
			{
				if ( g_taskbar != NULL )
				{
					g_taskbar->lpVtbl->Release( g_taskbar );
					g_taskbar = NULL;
				}

				_CoUninitialize();
			}

			if ( tooltip_buffer != NULL )
			{
				GlobalFree( tooltip_buffer );
				tooltip_buffer = NULL;
			}

			// Delete our font.
			_DeleteObject( hFont_main );

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

			if ( site_list_changed ) { save_site_info(); site_list_changed = false; }
			if ( sftp_fps_host_list_changed ) { save_sftp_fps_host_info(); sftp_fps_host_list_changed = false; }
			if ( sftp_keys_host_list_changed ) { save_sftp_keys_host_info(); sftp_keys_host_list_changed = false; }
			if ( category_list_changed ) { save_category_info(); category_list_changed = false; }

			if ( cfg_enable_download_history && g_download_history_changed )
			{
				_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\download_history\0", 18 );
				//g_base_directory[ g_base_directory_length + 17 ] = 0;	// Sanity.

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

#ifdef ENABLE_DARK_MODE
		// Get rid of the stupid line below the menu bar.
		case WM_NCPAINT:
		case WM_NCACTIVATE:
		{
			// Draw our scrollbars if there's any.
			LRESULT ret = _DefWindowProcW( hWnd, msg, wParam, lParam );

			if ( g_use_dark_mode )
			{
				MENUBARINFO mbi;
				mbi.cbSize = sizeof( MENUBARINFO );
				_GetMenuBarInfo( hWnd, OBJID_MENU, 0, &mbi );

				RECT rc;
				_GetWindowRect( hWnd, &rc );

				_OffsetRect( &mbi.rcBar, -rc.left, -rc.top );

				HDC hDC = _GetWindowDC( hWnd );

				HPEN line_color = _CreatePen( PS_SOLID, 1, dm_color_window_border );
				HPEN old_color = ( HPEN )_SelectObject( hDC, line_color );
				_DeleteObject( old_color );

				_MoveToEx( hDC, mbi.rcBar.left, mbi.rcBar.bottom, NULL );
				_LineTo( hDC, mbi.rcBar.right, mbi.rcBar.bottom );
				_DeleteObject( line_color );

				_ReleaseDC( hWnd, hDC );
			}

			return ret;
		}
		break;
#endif

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
				else
				{
					use_taskbar_progress_main = true;
				}

				if ( use_taskbar_progress_main )
				{
					_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

					_CoCreateInstance( _CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, _IID_ITaskbarList3, ( void ** )&g_taskbar );
				}
			}
			else if ( msg == WM_TASKBARCREATED )
			{
				// Show the system tray icon again if it disappeared.
				if ( cfg_tray_icon )
				{
					InitializeSystemTray( hWnd );

					if ( cfg_show_tray_progress )
					{
						InitializeIconValues( hWnd );
					}
				}
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	//return TRUE;
}
