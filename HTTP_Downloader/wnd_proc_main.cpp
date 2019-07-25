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

#include "globals.h"

#include "lite_gdi32.h"
#include "lite_ole32.h"
#include "lite_comctl32.h"
#include "lite_comdlg32.h"
#include "lite_winmm.h"

#include "list_operations.h"
#include "file_operations.h"

#include "login_manager_utilities.h"

#include "connection.h"
#include "menus.h"

#include "http_parsing.h"
#include "utilities.h"

#include "drag_and_drop.h"

#include "string_tables.h"

#include "taskbar.h"

#include "system_tray.h"
#include "drop_window.h"

HWND g_hWnd_toolbar = NULL;
HWND g_hWnd_files_columns = NULL;		// The header control window for the listview.
HWND g_hWnd_files = NULL;
HWND g_hWnd_status = NULL;

HWND g_hWnd_tooltip = NULL;

wchar_t *tooltip_buffer = NULL;
int last_tooltip_item = -1;				// Prevent our hot tracking from calling the tooltip on the same item.

HIMAGELIST g_toolbar_imagelist = NULL;

HCURSOR wait_cursor = NULL;				// Temporary cursor while processing entries.

bool skip_list_draw = false;

int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

unsigned char g_total_columns = 0;

unsigned long long session_total_downloaded = 0;
unsigned long long session_downloaded_speed = 0;

WNDPROC ListViewProc = NULL;			// Subclassed listview window.

HWND g_hWnd_lv_edit = NULL;				// Handle to the listview edit control.
WNDPROC EditProc = NULL;				// Subclassed listview edit window.
RECT current_edit_pos;					// Current position of the listview edit control.
bool edit_from_menu	= false;			// True if we activate the edit from our (rename) menu, or Ctrl + R.

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

// Sort function for columns.
int CALLBACK DMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	int arr[ NUM_COLUMNS ];

	SORT_INFO *si = ( SORT_INFO * )lParamSort;

	if ( si->hWnd == g_hWnd_files )
	{
		DOWNLOAD_INFO *di1 = ( DOWNLOAD_INFO * )( ( si->direction == 1 ) ? lParam1 : lParam2 );
		DOWNLOAD_INFO *di2 = ( DOWNLOAD_INFO * )( ( si->direction == 1 ) ? lParam2 : lParam1 );

		_SendMessageW( g_hWnd_files, LVM_GETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );

		// Offset the virtual indices to match the actual index.
		OffsetVirtualIndices( arr, download_columns, NUM_COLUMNS, g_total_columns );

		switch ( arr[ si->column ] )
		{
			case 3: { return _wcsicmp_s( di1->file_path, di2->file_path ); } break;
			case 7:	{ return _wcsicmp_s( di1->file_path + di1->file_extension_offset, di2->file_path + di2->file_extension_offset ); } break;
			case 8:	{ return _wcsicmp_s( di1->file_path + di1->filename_offset, di2->file_path + di2->filename_offset ); } break;
			case 13: { return _wcsicmp_s( di1->url, di2->url ); } break;

			case 4: { return ( di1->speed > di2->speed ); } break;
			case 5: { return ( di1->downloaded > di2->downloaded ); } break;
			case 6: { return ( di1->file_size > di2->file_size ); } break;
			case 2: { return ( di1->add_time.QuadPart > di2->add_time.QuadPart ); } break;
			case 11: { return ( di1->time_elapsed > di2->time_elapsed ); } break;
			case 12: { return ( di1->time_remaining > di2->time_remaining ); } break;

			case 9:
			{
				if ( di1->status == di2->status )
				{
					if ( di1->last_downloaded != 0 && di2->last_downloaded == 0 )
					{
						return 1;
					}
					else if ( di1->last_downloaded == 0 && di2->last_downloaded != 0 )
					{
						return -1;
					}
					else if ( di1->last_downloaded == 0 && di2->last_downloaded == 0 )
					{
						return 1;
					}
					else
					{
						if ( di1->file_size != 0 && di2->file_size == 0 )
						{
							return 1;
						}
						else if ( di1->file_size == 0 && di2->file_size != 0 )
						{
							return -1;
						}
						else if ( di1->file_size == 0 && di2->file_size == 0 )
						{
							return 1;
						}
#ifdef _WIN64
						int i_percentage1 = ( int )( 1000.f * ( ( float )di1->last_downloaded / ( float )di1->file_size ) );
						int i_percentage2 = ( int )( 1000.f * ( ( float )di2->last_downloaded / ( float )di2->file_size ) );
#else
						// Multiply the floating point division by 1000%.
						// This leaves us with an integer in which the last digit will represent the decimal value.
						float f_percentage1 = 1000.f * ( ( float )di1->last_downloaded / ( float )di1->file_size );
						int i_percentage1 = 0;
						__asm
						{
							fld f_percentage1;		//; Load the floating point value onto the FPU stack.
							fistp i_percentage1;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
						}

						// Multiply the floating point division by 1000%.
						// This leaves us with an integer in which the last digit will represent the decimal value.
						float f_percentage2 = 1000.f * ( ( float )di2->last_downloaded / ( float )di2->file_size );
						int i_percentage2 = 0;
						__asm
						{
							fld f_percentage2;		//; Load the floating point value onto the FPU stack.
							fistp i_percentage2;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
						}
#endif
						return i_percentage1 > i_percentage2;
					}

					//return ( ( di1->file_size - di1->downloaded ) > ( di2->file_size - di2->downloaded ) );
				}
				else
				{
					return ( di1->status > di2->status );
				}
			}
			break;

			case 10:
			{
				if ( di1->ssl_version == -1 )
				{
					return -1;
				}
				else if ( di2->ssl_version == -1 )
				{
					return 1;
				}
				else
				{
					return ( di1->ssl_version > di2->ssl_version );
				}
			}
			break;

			case 1:
			{
				if ( di1->parts > di2->parts )
				{
					return 1;
				}
				else if ( di1->parts < di2->parts )
				{
					return -1;
				}
				else
				{
					return ( di1->active_parts > di2->active_parts );
				}
			}
			break;

			default:
			{
				return 0;
			}
			break;
		}	
	}

	return 0;
}

unsigned int FormatSizes( wchar_t *buffer, unsigned int buffer_size, unsigned char toggle_type, unsigned long long data_size )
{
	unsigned int length = 0;

	if ( toggle_type == SIZE_FORMAT_AUTO )
	{
		if ( data_size > 1073741824 )
		{
			toggle_type = SIZE_FORMAT_GIGABYTE;	// Gigabyte
		}
		else if ( data_size > 1048576 )
		{
			toggle_type = SIZE_FORMAT_MEGABYTE;	// Megabyte
		}
		else if ( data_size > 1024 )
		{
			toggle_type = SIZE_FORMAT_KILOBYTE;	// Kilobyte
		}
		else
		{
			toggle_type = SIZE_FORMAT_BYTE;		// Byte
		}
	}

	if ( toggle_type == SIZE_FORMAT_KILOBYTE )
	{
#ifdef _WIN64
		unsigned int i_percentage = ( unsigned int )( 100.0f * ( float )data_size / 1024.0f );
#else
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1024.0f;
		unsigned int i_percentage = 0;
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
#endif
		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%lu.%02lu KB", i_percentage, remainder );
	}
	else if ( toggle_type == SIZE_FORMAT_MEGABYTE )
	{
#ifdef _WIN64
		unsigned int i_percentage = ( unsigned int )( 100.0f * ( float )data_size / 1048576.0f );
#else
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1048576.0f;
		unsigned int i_percentage = 0;
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
#endif
		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%lu.%02lu MB", i_percentage, remainder );
	}
	else if ( toggle_type == SIZE_FORMAT_GIGABYTE )
	{
#ifdef _WIN64
		unsigned int i_percentage = ( unsigned int )( 100.0f * ( float )data_size / 1073741824.0f );
#else
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1073741824.0f;
		unsigned int i_percentage = 0;
		__asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}
#endif
		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%lu.%02lu GB", i_percentage, remainder );
	}
	else
	{
		length = __snwprintf( buffer, buffer_size, L"%llu B", data_size );
	}

	return length;
}

DWORD WINAPI UpdateWindow( LPVOID WorkThreadContext )
{
	QFILETIME current_time, last_update;

	wchar_t title_text[ 128 ];
	wchar_t sb_downloaded_buf[ 64 ];
	wchar_t sb_download_speed_buf[ 64 ];

	bool update_text_values = false;

	unsigned int sb_downloaded_buf_length = 0;
	unsigned int sb_download_speed_buf_length = 0;

	unsigned long long last_session_total_downloaded = 0;
	unsigned long long last_session_downloaded_speed = 0;

	bool run_timer = g_timers_running;
	unsigned char standby_counter = 0;

	COLORREF border_color;
	COLORREF progress_color;

	unsigned char all_paused = 0;	// 0 = No state, 1 = all downloads are paused, 2 = a download is not paused

	last_update.ull = 0;

	unsigned char speed_buf_length = ( ST_L_Download_speed_ > 38 ? 38 : ST_L_Download_speed_ ); // Let's not overflow. 64 - ( ' ' + 22 +  '/' + 's' + NULL ) = 38 remaining bytes for our string.
	_wmemcpy_s( sb_download_speed_buf, 64, ST_V_Download_speed_, speed_buf_length );
	sb_download_speed_buf[ speed_buf_length ] = ' ';

	unsigned char download_buf_length = ( ST_L_Total_downloaded_ > 40 ? 40 : ST_L_Total_downloaded_ ); // Let's not overflow. 64 - ( ' ' + 22 + NULL ) = 40 remaining bytes for our string.
	_wmemcpy_s( sb_downloaded_buf, 64, ST_V_Total_downloaded_, download_buf_length );
	sb_downloaded_buf[ download_buf_length ] = ' ';

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

		session_downloaded_speed = 0;

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
						if ( TryEnterCriticalSection( &di->shared_cs ) == TRUE )
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

										session_downloaded_speed += di->speed;
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

							LeaveCriticalSection( &di->shared_cs );
						}
					}

					active_download_node = active_download_node->next;
				}

				last_update = current_time;

				LeaveCriticalSection( &active_download_list_cs );
			}

			LeaveCriticalSection( &worker_cs );
		}

		_InvalidateRect( g_hWnd_files, NULL, FALSE );

		update_text_values = false;

		// Update our status bar with the download speed.
		if ( session_downloaded_speed != last_session_downloaded_speed )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + ( speed_buf_length + 1 ), 64 - ( speed_buf_length + 1 ), cfg_t_status_down_speed, session_downloaded_speed ) + ( speed_buf_length + 1 );
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
			sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
			sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )sb_download_speed_buf );

			last_session_downloaded_speed = session_downloaded_speed;

			update_text_values = true;
		}

		// Update our status bar with the download total.
		if ( session_total_downloaded != last_session_total_downloaded )
		{
			// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
			sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + ( download_buf_length + 1 ), 64 - ( download_buf_length + 1 ), cfg_t_status_downloaded, session_total_downloaded ) + ( download_buf_length + 1 );
			// NULL terminator is set in FormatSizes.

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )sb_downloaded_buf );

			last_session_total_downloaded = session_total_downloaded;

			update_text_values = true;
		}

		if ( run_timer )
		{
			if ( update_text_values )
			{
				int tooltip_offset = 15, title_text_offset = 15;

				// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
				sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + ( speed_buf_length + 1 ), 64 - ( speed_buf_length + 1 ), SIZE_FORMAT_AUTO, session_downloaded_speed ) + ( speed_buf_length + 1 );
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
				sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

				// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
				sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + ( download_buf_length + 1 ), 64 - ( download_buf_length + 1 ), SIZE_FORMAT_AUTO, session_total_downloaded ) + ( download_buf_length + 1 );
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
					border_color = RGB( 0x40, 0x40, 0x00 );
					progress_color = RGB( 0xFF, 0xFF, 0x00 );
				}
				else
				{
					border_color = RGB( 0x00, 0x40, 0x00 );
					progress_color = RGB( 0x00, 0xFF, 0x00 );
				}

				if ( cfg_enable_drop_window && cfg_show_drop_window_progress )
				{
					UpdateDropWindow( g_progress_info.current_total_downloaded, g_progress_info.current_total_file_size, border_color, progress_color );
				}

				if ( cfg_tray_icon )
				{
					if ( cfg_show_tray_progress )
					{
						g_nid.hIcon = CreateSystemTrayIcon( g_progress_info.current_total_downloaded, g_progress_info.current_total_file_size, border_color, progress_color );
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
		}
		else
		{
			_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )PROGRAM_CAPTION );

			g_progress_info.download_state = 1;	// Completed.

			bool error = ( ( g_session_status_count[ 2 ] > 0 || g_session_status_count[ 3 ] > 0 || g_session_status_count[ 4 ] > 0 ) ? true : false );
			border_color = RGB( 0x40, 0x00, 0x00 );
			progress_color = RGB( 0xFF, 0x00, 0x00 );

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
					UpdateDropWindow( 1, 1, border_color, progress_color );
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
					g_nid.hIcon = CreateSystemTrayIcon( 1, 1, border_color, progress_color );
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
			if ( cfg_enable_download_history && download_history_changed )
			{
				save_session_handle = ( HANDLE )_CreateThread( NULL, 0, save_session, ( void * )NULL, 0, NULL );
			}

			if ( cfg_shutdown_action != SHUTDOWN_ACTION_NONE )
			{
				if ( save_session_handle != NULL )
				{
					WaitForSingleObject( save_session_handle, 30000 );
					CloseHandle( save_session_handle );
				}

				switch ( cfg_shutdown_action )
				{
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
	return 0;
}

LRESULT CALLBACK EditSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_WINDOWPOSCHANGING:
		{
			// Modify the position of the listview edit control. We're moving it to the Filename column.
			WINDOWPOS *wp = ( WINDOWPOS * )lParam;
			wp->x = current_edit_pos.left;
			wp->y = current_edit_pos.top;
			wp->cx = current_edit_pos.right - current_edit_pos.left + 1;
			wp->cy = current_edit_pos.bottom - current_edit_pos.top - 1;
		}
		break;
	}

	// Everything that we don't handle gets passed back to the parent to process.
	return CallWindowProc( EditProc, hWnd, msg, wParam, lParam );
}

wchar_t *GetDownloadInfoString( DOWNLOAD_INFO *di, int column, int item_index, wchar_t *tbuf, unsigned short tbuf_size )
{
	wchar_t *buf = NULL;

	// Save the appropriate text in our buffer for the current column.
	switch ( column )
	{
		case 0:	// NUM
		{
			buf = tbuf;	// Reset the buffer pointer.

			__snwprintf( buf, tbuf_size, L"%lu", item_index );
		}
		break;

		case 1:	// ACTIVE PARTS
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di->parts_limit > 0 )
			{
				__snwprintf( buf, tbuf_size, L"%lu/%lu/%lu", di->active_parts, di->parts_limit, di->parts );
			}
			else
			{
				__snwprintf( buf, tbuf_size, L"%lu/%lu", di->active_parts, di->parts );
			}
		}
		break;

		case 2:	// DATE AND TIME ADDED
		{
			buf = di->w_add_time;
		}
		break;

		case 3:	// DOWNLOAD DIRECTORY
		{
			if ( !( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				buf = di->file_path;
			}
			else
			{
				buf = ST_V__Simulated_;
			}
		}
		break;

		case 4:	// DOWNLOAD SPEED
		{
			if ( IS_STATUS( di->status, STATUS_DOWNLOADING ) )
			{
				buf = tbuf;	// Reset the buffer pointer.

				unsigned int length = FormatSizes( buf, tbuf_size, cfg_t_down_speed, di->speed );
				buf[ length ] = L'/';
				buf[ length + 1 ] = L's';
				buf[ length + 2 ] = 0;
			}
			else
			{
				buf = L"";
			}
		}
		break;

		case 5:	// DOWNLOADED
		{
			buf = tbuf;	// Reset the buffer pointer.

			FormatSizes( buf, tbuf_size, cfg_t_downloaded, ( IS_STATUS( di->status, STATUS_MOVING_FILE ) ? di->downloaded : di->last_downloaded ) );
		}
		break;

		case 6:	// FILE SIZE
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
				buf[ 0 ] = L'?';
				buf[ 1 ] = L' ';

				if ( cfg_t_file_size == SIZE_FORMAT_KILOBYTE )
				{
					buf[ 2 ] = L'K';
					buf[ 3 ] = L'B';
					buf[ 4 ] = 0;
				}
				else if ( cfg_t_file_size == SIZE_FORMAT_MEGABYTE )
				{
					buf[ 2 ] = L'M';
					buf[ 3 ] = L'B';
					buf[ 4 ] = 0;
				}
				else if ( cfg_t_file_size == SIZE_FORMAT_GIGABYTE )
				{
					buf[ 2 ] = L'G';
					buf[ 3 ] = L'B';
					buf[ 4 ] = 0;
				}
				else
				{
					buf[ 2 ] = L'B';
					buf[ 3 ] = 0;
				}
			}
		}
		break;

		/*case 7:	// FILE TYPE
		{
		}
		break;*/

		case 8:	// FILENAME
		{
			buf = di->file_path + di->filename_offset;
		}
		break;

		case 9:	// PROGRESS
		{
			buf = tbuf;	// Reset the buffer pointer.

			if ( di->file_size > 0 )
			{
#ifdef _WIN64
				int i_percentage = ( int )( 1000.f * ( ( float )di->last_downloaded / ( float )di->file_size ) );
#else
				// Multiply the floating point division by 1000%.
				// This leaves us with an integer in which the last digit will represent the decimal value.
				float f_percentage = 1000.f * ( ( float )di->last_downloaded / ( float )di->file_size );
				int i_percentage = 0;
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
					//buf = ST_V_Moving_File;
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

		case 10:	// SSL / TLS Version
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

		case 11:	// TIME ELAPSED
		case 12:	// TIME REMAINING
		{
			// Use the infinity symbol for remaining time if it can't be calculated.
			if ( column == 12 &&
			   ( IS_STATUS( di->status, STATUS_CONNECTING | STATUS_PAUSED ) ||
			   ( di->status == STATUS_DOWNLOADING && ( di->file_size == 0 || di->speed == 0 ) ) ) )
			{
				buf = L"\x221E\0";	// Infinity symbol.
			}
			else
			{
				unsigned long long time_length = ( column == 11 ? di->time_elapsed : di->time_remaining );

				if ( IS_STATUS( di->status, STATUS_DOWNLOADING ) || time_length > 0 )
				{
					buf = tbuf;	// Reset the buffer pointer.

					if ( time_length < 60 )	// Less than 1 minute.
					{
						__snwprintf( buf, tbuf_size, L"%llus", time_length );
					}
					else if ( time_length < 3600 )	// Less than 1 hour.
					{
						__snwprintf( buf, tbuf_size, L"%llum%02llus", time_length / 60, time_length % 60 );
					}
					else if ( time_length < 86400 )	// Less than 1 day.
					{
						__snwprintf( buf, tbuf_size, L"%lluh%02llum%02llus", time_length / 3600, ( time_length / 60 ) % 60, time_length % 60 );
					}
					else	// More than 1 day.
					{
						__snwprintf( buf, tbuf_size, L"%llud%02lluh%02llum%02llus", time_length / 86400, ( time_length / 3600 ) % 24, ( time_length / 60 ) % 60, time_length % 60 );
					}
				}
				else
				{
					buf = L"";
				}
			}
		}
		break;

		case 13:	// URL
		{
			buf = di->url;
		}
		break;
	}

	return buf;
}

LRESULT CALLBACK ListViewSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_DIVIDERDBLCLICK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					int largest_width;

					int virtual_index = GetVirtualIndexFromColumnIndex( nmh->iItem, download_columns, NUM_COLUMNS );

					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						largest_width = LVSCW_AUTOSIZE_USEHEADER;
					}
					else
					{
						largest_width = 26;	// 5 + 16 + 5.

						if ( virtual_index != 7 )	// File Type
						{
							wchar_t tbuf[ 128 ];

							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );

							int index = ( int )_SendMessageW( hWnd, LVM_GETTOPINDEX, 0, 0 );
							int index_end = ( int )_SendMessageW( hWnd, LVM_GETCOUNTPERPAGE, 0, 0 ) + index;

							RECT rc;
							HDC hDC = _GetDC( hWnd );
							HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
							_DeleteObject( ohf );

							for ( ; index <= index_end; ++index )
							{
								lvi.iItem = index;
								lvi.mask = LVIF_PARAM;
								if ( _SendMessageW( hWnd, LVM_GETITEM, 0, ( LPARAM )&lvi ) == TRUE )
								{
									DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
									if ( di != NULL )
									{
										wchar_t *buf = GetDownloadInfoString( di, virtual_index, index + 1, tbuf, 128 );

										if ( buf == NULL )
										{
											tbuf[ 0 ] = L'\0';
											buf = tbuf;
										}

										rc.bottom = rc.left = rc.right = rc.top = 0;

										_DrawTextW( hDC, buf, -1, &rc, DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT );

										int width = ( rc.right - rc.left ) + 10;	// 5 + 5 padding.
										if ( width > largest_width )
										{
											largest_width = width;
										}
									}
								}
								else
								{
									break;
								}
							}

							_ReleaseDC( hWnd, hDC );
						}
					}

					_SendMessageW( hWnd, LVM_SETCOLUMNWIDTH, nmh->iItem, largest_width );

					// Save our new column width.
					*download_columns_width[ virtual_index ] = ( int )_SendMessageW( hWnd, LVM_GETCOLUMNWIDTH, nmh->iItem, 0 );

					return TRUE;
				}
				break;
			}
		}
		break;
	}

	// Everything that we don't handle gets passed back to the parent to process.
	return CallWindowProc( ListViewProc, hWnd, msg, wParam, lParam );
}

void HandleCommand( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	switch ( LOWORD( wParam ) )
	{
		case MENU_OPEN_FILE:
		case MENU_OPEN_DIRECTORY:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
				if ( di != NULL && !( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
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

					wchar_t file_path[ MAX_PATH ];
					if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED )
					{
						GetTemporaryFilePath( di, file_path );
					}
					else
					{
						GetDownloadFilePath( di, file_path );
					}

					if ( LOWORD( wParam ) == MENU_OPEN_FILE )
					{
						// Set the verb to NULL so that unknown file types can be handled by the system.
						HINSTANCE hInst = _ShellExecuteW( NULL, NULL, file_path, NULL, NULL, SW_SHOWNORMAL );
						if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )
						{
							if ( _MessageBoxW( hWnd, ST_V_PROMPT_The_specified_file_was_not_found, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
							{
								CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )3, 0, NULL ) );	// Restart download (from the beginning).
							}
						}
					}
					else if ( LOWORD( wParam ) == MENU_OPEN_DIRECTORY )
					{
						HINSTANCE hInst = ( HINSTANCE )ERROR_FILE_NOT_FOUND;

						LPITEMIDLIST iidl = _ILCreateFromPathW( file_path );

						if ( iidl != NULL && _SHOpenFolderAndSelectItems( iidl, 0, NULL, 0 ) == S_OK )
						{
							hInst = ( HINSTANCE )ERROR_SUCCESS;
						}
						else
						{
							// Try opening the folder without selecting any file.
							hInst = _ShellExecuteW( NULL, L"open", di->file_path, NULL, NULL, SW_SHOWNORMAL );
						}

						// Use this instead of ILFree on Windows 2000 or later.
						if ( iidl != NULL )
						{
							_CoTaskMemFree( iidl );
						}

						if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )	// We're opening a folder, but it uses the same error code as a file if it's not found.
						{
							_MessageBoxW( hWnd, ST_V_The_specified_path_was_not_found, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
						}
					}

					if ( destroy )
					{
						_CoUninitialize();
					}
				}
			}
		}
		break;

		case MENU_SAVE_DOWNLOAD_HISTORY:
		{
			wchar_t *file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"CSV (Comma delimited) (*.csv)\0*.csv\0";
			ofn.lpstrDefExt = L"csv";
			ofn.lpstrTitle = ST_V_Save_Download_History;
			ofn.lpstrFile = file_path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				// file_path will be freed in the create_download_history_csv_file thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, create_download_history_csv_file, ( void * )file_path, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					GlobalFree( file_path );
				}
			}
			else
			{
				GlobalFree( file_path );
			}
		}
		break;

		case MENU_IMPORT_DOWNLOAD_HISTORY:
		{
			wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH * MAX_PATH ) );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"Download History\0*.*\0";
			ofn.lpstrTitle = ST_V_Import_Download_History;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH * MAX_PATH;
			ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY;

			if ( _GetOpenFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				iei->type = 1;	// Import from menu.
				iei->file_paths = file_name;
				iei->file_offset = ofn.nFileOffset;

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
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_EXPORT_DOWNLOAD_HISTORY:
		{
			wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"Download History\0*.*\0";
			//ofn.lpstrDefExt = L"txt";
			ofn.lpstrTitle = ST_V_Export_Download_History;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				iei->file_paths = file_name;

				// iei will be freed in the export_list thread.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, export_list, ( void * )iei, 0, NULL );
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
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_START:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_DOWNLOADING, 0, NULL ) );
		}
		break;

		case MENU_PAUSE:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_PAUSED, 0, NULL ) );
		}
		break;

		case MENU_STOP:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_STOPPED, 0, NULL ) );
		}
		break;

		case MENU_RESTART:
		{
			if ( _MessageBoxW( hWnd, ST_V_PROMPT_restart_selected_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_RESTART, 0, NULL ) );
			}
		}
		break;

		case MENU_PAUSE_ACTIVE:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )0, 0, NULL ) );
		}
		break;

		case MENU_STOP_ALL:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )1, 0, NULL ) );
		}
		break;

		case MENU_UPDATE_DOWNLOAD:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

				if ( lvi.lParam != NULL )
				{
					if ( g_hWnd_update_download == NULL )
					{
						g_hWnd_update_download = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"update_download", ST_V_Update_Download, WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 525 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 403 ) / 2 ), 525, 403, NULL, NULL, NULL, NULL );
					}
					else if ( _IsIconic( g_hWnd_update_download ) )	// If minimized, then restore the window.
					{
						_ShowWindow( g_hWnd_update_download, SW_RESTORE );
					}

					_SendMessageW( g_hWnd_update_download, WM_PROPAGATE, 0, lvi.lParam );
				}
			}
		}
		break;

		case MENU_QUEUE_TOP:
		case MENU_QUEUE_UP:
		case MENU_QUEUE_DOWN:
		case MENU_QUEUE_BOTTOM:
		{
			unsigned char handle_type = 0;

			switch ( LOWORD( wParam ) )
			{
				case MENU_QUEUE_TOP: { handle_type = 0; } break;
				case MENU_QUEUE_UP: { handle_type = 1; } break;
				case MENU_QUEUE_DOWN: { handle_type = 2; } break;
				case MENU_QUEUE_BOTTOM: { handle_type = 3; } break;
			}

			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_queue, ( void * )handle_type, 0, NULL ) );
		}
		break;

		case MENU_REMOVE:
		{
			if ( _MessageBoxW( hWnd, ST_V_PROMPT_remove_selected_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )0, 0, NULL ) );
			}
		}
		break;

		case MENU_REMOVE_COMPLETED:
		{
			if ( _MessageBoxW( hWnd, ST_V_PROMPT_remove_completed_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )2, 0, NULL ) );
			}
		}
		break;

		case MENU_REMOVE_AND_DELETE:
		{
			if ( _MessageBoxW( hWnd, ST_V_PROMPT_remove_and_delete_selected_entries, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )1, 0, NULL ) );
			}
		}
		break;

		case MENU_COPY_URLS:
		{
			CloseHandle( ( HANDLE )_CreateThread( NULL, 0, copy_urls, ( void * )NULL, 0, NULL ) );
		}
		break;

		case MENU_DELETE:
		{
			if ( _MessageBoxW( hWnd, ST_V_PROMPT_delete_selected_files, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				CloseHandle( ( HANDLE )_CreateThread( NULL, 0, delete_files, ( void * )NULL, 0, NULL ) );
			}
		}
		break;

		case MENU_RENAME:
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = ( int )_SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

			if ( lvi.iItem != -1 )
			{
				edit_from_menu = true;

				_SendMessageW( g_hWnd_files, LVM_EDITLABEL, lvi.iItem, 0 );
			}
		}
		break;

		case MENU_SELECT_ALL:
		{
			// Set the state of all items to selected.
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_STATE;
			lvi.state = LVIS_SELECTED;
			lvi.stateMask = LVIS_SELECTED;
			_SendMessageW( g_hWnd_files, LVM_SETITEMSTATE, -1, ( LPARAM )&lvi );

			//UpdateMenus( true );
		}
		break;

		case MENU_NUM:
		case MENU_ACTIVE_PARTS:
		case MENU_DATE_AND_TIME_ADDED:
		case MENU_DOWNLOAD_DIRECTORY:
		case MENU_DOWNLOAD_SPEED:
		case MENU_DOWNLOADED:
		case MENU_FILE_SIZE:
		case MENU_FILE_TYPE:
		case MENU_FILENAME:
		case MENU_PROGRESS:
		case MENU_SSL_TLS_VERSION:
		case MENU_TIME_ELAPSED:
		case MENU_TIME_REMAINING:
		case MENU_URL:
		{
			UpdateColumns( LOWORD( wParam ) );
		}
		break;

		case MENU_ADD_URLS:
		{
			if ( g_hWnd_add_urls == NULL )
			{
				g_hWnd_add_urls = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"add_urls", ST_V_Add_URL_s_, WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 600 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 263 ) / 2 ), 600, 263, NULL, NULL, NULL, NULL );
			}

			_SendMessageW( g_hWnd_add_urls, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_SHOW_TOOLBAR:
		{
			cfg_show_toolbar = !cfg_show_toolbar;

			LONG_PTR style = _GetWindowLongPtrW( g_hWnd_files, GWL_STYLE );

			if ( cfg_show_toolbar )
			{
				style |= WS_BORDER;

				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_TOOLBAR, MF_CHECKED );
				_ShowWindow( g_hWnd_toolbar, SW_SHOW );
			}
			else
			{
				style &= ~( WS_BORDER );

				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_TOOLBAR, MF_UNCHECKED );
				_ShowWindow( g_hWnd_toolbar, SW_HIDE );
			}

			UpdateMenus( true );

			// Show/hide the files border if the toolbar is/isn't visible.
			_SetWindowLongPtrW( g_hWnd_files, GWL_STYLE, style );

			_SendMessageW( hWnd, WM_SIZE, 0, 0 );
		}
		break;

		case MENU_SHOW_COLUMN_HEADERS:
		{
			cfg_show_column_headers = !cfg_show_column_headers;

			LONG_PTR style = _GetWindowLongPtrW( g_hWnd_files, GWL_STYLE );

			if ( cfg_show_column_headers )
			{
				style &= ~( LVS_NOCOLUMNHEADER );

				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_COLUMN_HEADERS, MF_CHECKED );
			}
			else
			{
				style |= LVS_NOCOLUMNHEADER;

				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_COLUMN_HEADERS, MF_UNCHECKED );
			}

			// Show/hide the files border if the toolbar is/isn't visible.
			_SetWindowLongPtrW( g_hWnd_files, GWL_STYLE, style );
		}
		break;

		case MENU_SHOW_STATUS_BAR:
		{
			cfg_show_status_bar = !cfg_show_status_bar;

			if ( cfg_show_status_bar )
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_STATUS_BAR, MF_CHECKED );
				_ShowWindow( g_hWnd_status, SW_SHOW );
			}
			else
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_STATUS_BAR, MF_UNCHECKED );
				_ShowWindow( g_hWnd_status, SW_HIDE );
			}

			_SendMessageW( hWnd, WM_SIZE, 0, 0 );
		}
		break;

		case MENU_SEARCH:
		{
			if ( g_hWnd_search == NULL )
			{
				g_hWnd_search = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"search", ST_V_Search, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 320 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 210 ) / 2 ), 320, 210, NULL, NULL, NULL, NULL );
			}

			_SendMessageW( g_hWnd_search, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_OPTIONS:
		{
			if ( g_hWnd_options == NULL )
			{
				g_hWnd_options = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"options", ST_V_Options, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 480 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 400 ) / 2 ), 480, 400, NULL, NULL, NULL, NULL );
				_ShowWindow( g_hWnd_options, SW_SHOWNORMAL );
			}
			_SetForegroundWindow( g_hWnd_options );
		}
		break;

		case MENU_HOME_PAGE:
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

		case MENU_ABOUT:
		{
			wchar_t msg[ 512 ];
			__snwprintf( msg, 512, L"HTTP Downloader is made free under the GPLv3 license.\r\n\r\n" \
								   L"Version 1.0.2.6 (%u-bit)\r\n\r\n" \
								   L"Built on %s, %s %d, %04d %d:%02d:%02d %s (UTC)\r\n\r\n" \
								   L"Copyright \xA9 2015-2019 Eric Kutcher",
#ifdef _WIN64
								   64,
#else
								   32,
#endif
								   ( g_compile_time.wDayOfWeek > 6 ? L"" : day_string_table[ g_compile_time.wDayOfWeek ].value ),
								   ( ( g_compile_time.wMonth > 12 || g_compile_time.wMonth < 1 ) ? L"" : month_string_table[ g_compile_time.wMonth - 1 ].value ),
								   g_compile_time.wDay,
								   g_compile_time.wYear,
								   ( g_compile_time.wHour > 12 ? g_compile_time.wHour - 12 : ( g_compile_time.wHour != 0 ? g_compile_time.wHour : 12 ) ),
								   g_compile_time.wMinute,
								   g_compile_time.wSecond,
								   ( g_compile_time.wHour >= 12 ? L"PM" : L"AM" ) );

			_MessageBoxW( hWnd, msg, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
		}
		break;

		case MENU_RESTORE:
		{
			if ( _IsIconic( hWnd ) )	// If minimized, then restore the window.
			{
				_ShowWindow( hWnd, SW_RESTORE );
			}
			else if ( _IsWindowVisible( hWnd ) == TRUE )	// If already visible, then flash the window.
			{
				_FlashWindow( hWnd, TRUE );
			}
			else	// If hidden, then show the window.
			{
				_ShowWindow( hWnd, SW_SHOW );
			}
		}
		break;

		case MENU_EXIT:
		{
			_SendMessageW( hWnd, WM_EXIT, 0, 0 );
		}
		break;
	}
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

			_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\toolbar.bmp\0", 13 );
			if ( GetFileAttributesW( base_directory ) != INVALID_FILE_ATTRIBUTES )
			{
				g_toolbar_imagelist = _ImageList_LoadImageW( NULL, base_directory, 24, 0, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ), IMAGE_BITMAP, LR_LOADFROMFILE | LR_CREATEDIBSECTION );
			}
			else
			{
				g_toolbar_imagelist = _ImageList_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDB_BITMAP_TOOLBAR ), 24, 0, ( COLORREF )RGB( 0xFF, 0x00, 0xFF ), IMAGE_BITMAP, LR_CREATEDIBSECTION );
			}
			_SendMessageW( g_hWnd_toolbar, TB_SETIMAGELIST, 0, ( LPARAM )g_toolbar_imagelist );

			_SendMessageW( g_hWnd_toolbar, TB_BUTTONSTRUCTSIZE, ( WPARAM )sizeof( TBBUTTON ), 0 );

			// Allows us to use the iString value for tooltips.
			_SendMessageW( g_hWnd_toolbar, TB_SETMAXTEXTROWS, 0, 0 );

			TBBUTTON tbb[ 13 ] = 
			{
				{ MAKELONG( 0, 0 ),			MENU_ADD_URLS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		( INT_PTR )ST_V_Add_URL_s_ },
				{ MAKELONG( 1, 0 ),			  MENU_REMOVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,			( INT_PTR )ST_V_Remove },
				{				 0,					   -1,				  0,	  BTNS_SEP,	{ 0 }, 0,							  NULL },
				{ MAKELONG( 2, 0 ),			   MENU_START,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,			 ( INT_PTR )ST_V_Start },
				{ MAKELONG( 3, 0 ),			   MENU_PAUSE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,			 ( INT_PTR )ST_V_Pause },
				{ MAKELONG( 4, 0 ),				MENU_STOP,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,			  ( INT_PTR )ST_V_Stop },
				{ MAKELONG( 5, 0 ),			 MENU_RESTART,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		   ( INT_PTR )ST_V_Restart },
				{				 0,					   -1,				  0,	  BTNS_SEP,	{ 0 }, 0,							  NULL },
				{ MAKELONG( 6, 0 ),		MENU_PAUSE_ACTIVE,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,	  ( INT_PTR )ST_V_Pause_Active },
				{ MAKELONG( 7, 0 ),			MENU_STOP_ALL,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		  ( INT_PTR )ST_V_Stop_All },
				{				 0,					   -1,				  0,	  BTNS_SEP,	{ 0 }, 0,							  NULL },
				{ MAKELONG( 8, 0 ),			  MENU_SEARCH,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		    ( INT_PTR )ST_V_Search },
				{ MAKELONG( 9, 0 ),			 MENU_OPTIONS,	TBSTATE_ENABLED, BTNS_AUTOSIZE,	{ 0 }, 0,		   ( INT_PTR )ST_V_Options }
			};

			_SendMessageW( g_hWnd_toolbar, TB_ADDBUTTONS, 13, ( LPARAM )&tbb );

			g_hWnd_files = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_EDITLABELS | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW | WS_VISIBLE | ( cfg_show_toolbar ? WS_BORDER : 0 ) | ( cfg_show_column_headers ? 0 : LVS_NOCOLUMNHEADER ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_files, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | ( cfg_show_gridlines ? LVS_EX_GRIDLINES : 0 ) | LVS_EX_HEADERDRAGDROP );

			g_hWnd_status = _CreateWindowW( STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILDWINDOW | ( cfg_show_status_bar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWnd_files, NULL, NULL, NULL );

			TOOLINFO tti;
			_memzero( &tti, sizeof( TOOLINFO ) );
			tti.cbSize = sizeof( TOOLINFO );
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = g_hWnd_files;

			_SendMessageW( g_hWnd_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );
			_SendMessageW( g_hWnd_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 2000 );

			_SendMessageW( g_hWnd_files, LVM_SETTOOLTIPS, ( WPARAM )g_hWnd_tooltip, 0 );


			_SendMessageW( g_hWnd_toolbar, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_files, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_status, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			ListViewProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_files, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_files, GWLP_WNDPROC, ( LONG_PTR )ListViewSubProc );

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop_main = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop_main )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_files, &List_DropTarget );
			}

			int status_bar_widths[] = { 250, -1 };

			_SendMessageW( g_hWnd_status, SB_SETPARTS, 2, ( LPARAM )status_bar_widths );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )( cfg_t_status_down_speed == SIZE_FORMAT_BYTE ? ST_V_Download_speed__0_B_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_KILOBYTE ? ST_V_Download_speed__0_00_KB_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_MEGABYTE ? ST_V_Download_speed__0_00_MB_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_GIGABYTE ? ST_V_Download_speed__0_00_GB_s : ST_V_Download_speed__0_B_s ) ) ) ) );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )( cfg_t_status_downloaded == SIZE_FORMAT_BYTE ? ST_V_Total_downloaded__0_B :
																					( cfg_t_status_downloaded == SIZE_FORMAT_KILOBYTE ? ST_V_Total_downloaded__0_00_KB :
																					( cfg_t_status_downloaded == SIZE_FORMAT_MEGABYTE ? ST_V_Total_downloaded__0_00_MB :
																					( cfg_t_status_downloaded == SIZE_FORMAT_GIGABYTE ? ST_V_Total_downloaded__0_00_GB : ST_V_Total_downloaded__0_B ) ) ) ) );

			int arr[ NUM_COLUMNS ];

			// Initialize our listview columns
			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

			for ( char i = 0; i < NUM_COLUMNS; ++i )
			{
				// Active Parts, Download Speed, Downloaded, File Size, Time Elapsed, Time Remaining
				if ( i == 1 || i == 4 || i == 5 || i == 6 || i == 11 || i == 12 )
				{
					lvc.fmt = LVCFMT_RIGHT;
				}
				else if ( i == 9 )	// Progress
				{
					lvc.fmt = LVCFMT_CENTER;
				}
				else
				{
					lvc.fmt = LVCFMT_LEFT;
				}

				if ( *download_columns[ i ] != -1 )
				{
					//lvc.pszText = download_string_table[ i ].value;
					lvc.pszText = g_locale_table[ DOWNLOAD_STRING_TABLE_OFFSET + i ].value;
					lvc.cx = *download_columns_width[ i ];
					_SendMessageW( g_hWnd_files, LVM_INSERTCOLUMN, g_total_columns, ( LPARAM )&lvc );

					arr[ g_total_columns++ ] = *download_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_files, LVM_SETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );

			g_hWnd_files_columns = ( HWND )_SendMessageW( g_hWnd_files, LVM_GETHEADER, 0, 0 );

			g_default_tray_icon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );

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
				_wmemcpy_s( iei->file_paths, MAX_PATH, base_directory, base_directory_length );
				_wmemcpy_s( iei->file_paths + ( base_directory_length + 1 ), MAX_PATH - ( base_directory_length - 1 ), L"download_history\0", 17 );
				iei->file_paths[ base_directory_length + 17 ] = 0;	// Sanity.
				iei->file_offset = ( unsigned short )( base_directory_length + 1 );
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
				_EnableMenuItem( g_hMenu, MENU_SAVE_DOWNLOAD_HISTORY, ( _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 ) > 0 ? MF_ENABLED : MF_DISABLED ) );
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
			HandleCommand( hWnd, wParam, lParam );

			return 0;
		}
		break;

		case WM_KEYDOWN:
		{
			// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
			if ( _GetKeyState( VK_CONTROL ) & 0x8000 )
			{
				// Determine which key was pressed.
				switch ( wParam )
				{
					case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_SELECT_ALL, 0 );
						}
					}
					break;

					case 'C':	// Copy URL(s).
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_COPY_URLS, 0 );
						}
					}
					break;

					case 'N':	// Open Add URL(s) window.
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_ADD_URLS, 0 );
					}
					break;

					case 'O':	// Open Options window.
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_OPTIONS, 0 );
					}
					break;

					case 'R':	// Remove selected items.
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_REMOVE, 0 );
						}
					}
					break;

					case 'S':	// Open Search window.
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_SEARCH, 0 );
					}
					break;

					case VK_DELETE:	// Remove and Delete selected items.
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_REMOVE_AND_DELETE, 0 );
						}
					}
					break;
				}
			}
			else
			{
				// Determine which key was pressed.
				switch ( wParam )
				{
					case VK_DELETE:	// Delete selected items.
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 ) > 0 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_DELETE, 0 );
						}
					}
					break;

					case VK_F2:	// Rename selected item.
					{
						if ( !in_worker_thread && _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 ) == 1 )
						{
							_SendMessageW( hWnd, WM_COMMAND, MENU_RENAME, 0 );
						}
					}
					break;
				}
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_BEGINDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );
					if ( hWnd_parent == g_hWnd_files )
					{
						// If we're editing a row item and the edit box is visible while we drag the column, then we need to close it.
						if ( _IsWindowVisible( g_hWnd_lv_edit ) == TRUE )
						{
							_SendMessageW( g_hWnd_files, LVM_CANCELEDITLABEL, 0, 0 );
						}
					}
				}
				break;

				case HDN_ENDDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					// Prevent the # columns from moving and the other columns from becoming the first column.
					if ( nmh->iItem == 0 || nmh->pitem->iOrder == 0 )
					{
						// Make sure the # columns are visible.
						HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );
						if ( hWnd_parent == g_hWnd_files && *download_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, download_columns, NUM_COLUMNS );
							return TRUE;
						}
					}
				}
				break;

				case HDN_ENDTRACK:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;

					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );
					if ( hWnd_parent == g_hWnd_files )
					{
						int index = GetVirtualIndexFromColumnIndex( nmh->iItem, download_columns, NUM_COLUMNS );

						if ( index != -1 )
						{
							*download_columns_width[ index ] = nmh->pitem->cxy;
						}
					}
				}
				break;

				case LVN_COLUMNCLICK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Change the format of the items in the column if Ctrl is held while clicking the column.
					if ( GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						int index = GetVirtualIndexFromColumnIndex( nmlv->iSubItem, download_columns, NUM_COLUMNS );

						// Change the size column info.
						if ( index != -1 )
						{
							if ( index == 4 )
							{
								if ( cfg_t_down_speed >= SIZE_FORMAT_AUTO )
								{
									cfg_t_down_speed = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_down_speed;
								}
								InvalidateRect( nmlv->hdr.hwndFrom, NULL, TRUE );
							}
							else if ( index == 5 )
							{
								if ( cfg_t_downloaded >= SIZE_FORMAT_AUTO )
								{
									cfg_t_downloaded = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_downloaded;
								}
								InvalidateRect( nmlv->hdr.hwndFrom, NULL, TRUE );
							}
							else if ( index == 6 )
							{
								if ( cfg_t_file_size >= SIZE_FORMAT_AUTO )
								{
									cfg_t_file_size = SIZE_FORMAT_BYTE;
								}
								else
								{
									++cfg_t_file_size;
								}
								InvalidateRect( nmlv->hdr.hwndFrom, NULL, TRUE );
							}
						}
					}
					else	// Normal column click. Sort the items in the column.
					{
						LVCOLUMN lvc;
						_memzero( &lvc, sizeof( LVCOLUMN ) );
						lvc.mask = LVCF_FMT | LVCF_ORDER;
						_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

						SORT_INFO si;
						si.column = lvc.iOrder;
						si.hWnd = nmlv->hdr.hwndFrom;

						if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
						{
							si.direction = 0;	// Now sort down.

							// Sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );
						}
						else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
						{
							si.direction = 1;	// Now sort up.

							// Sort up
							lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
						}
						else	// Column has no sorting set.
						{
							// Remove the sort format for all columns.
							for ( unsigned char i = 0; i < NUM_COLUMNS; ++i )
							{
								// Get the current format
								_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, i, ( LPARAM )&lvc );
								// Remove sort up and sort down
								lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
								_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, i, ( LPARAM )&lvc );
							}

							// Read current the format from the clicked column
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_GETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

							si.direction = 0;	// Start the sort going down.

							// Sort down to start.
							lvc.fmt = lvc.fmt | HDF_SORTDOWN;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );
						}

						_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )DMCompareFunc );
					}
				}
				break;

				case NM_RCLICK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_files )
					{
						if ( !in_worker_thread )
						{
							UpdateMenus( true );
						}

						_ClientToScreen( nmitem->hdr.hwndFrom, &nmitem->ptAction );

						_TrackPopupMenu( g_hMenuSub_download, 0, nmitem->ptAction.x, nmitem->ptAction.y, 0, hWnd, NULL );
					}
					else if ( nmitem->hdr.hwndFrom == g_hWnd_files_columns )
					{
						POINT p;
						_GetCursorPos( &p );

						_TrackPopupMenu( g_hMenuSub_column, 0, p.x, p.y, 0, hWnd, NULL );
					}
					else if ( nmitem->hdr.hwndFrom == g_hWnd_toolbar || nmitem->hdr.hwndFrom == g_hWnd_status )
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

					if ( nm->hdr.hwndFrom == g_hWnd_status )
					{
						wchar_t status_bar_buf[ 64 ];
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

							buf_length = ( ST_L_Download_speed_ > 38 ? 38 : ST_L_Download_speed_ ); // Let's not overflow. 64 - ( ' ' + 22 +  '/' + 's' + NULL ) = 38 remaining bytes for our string.
							_wmemcpy_s( status_bar_buf, 64, ST_V_Download_speed_, buf_length );
							status_bar_buf[ buf_length ] = ' ';
							// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
							unsigned int length = FormatSizes( status_bar_buf + ( buf_length + 1 ), 64 - ( buf_length + 1 ), cfg_t_status_down_speed, session_downloaded_speed ) + ( buf_length + 1 );
							status_bar_buf[ length ] = L'/';
							status_bar_buf[ length + 1 ] = L's';
							status_bar_buf[ length + 2 ] = 0;

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

							buf_length = ( ST_L_Total_downloaded_ > 40 ? 40 : ST_L_Total_downloaded_ ); // Let's not overflow. 64 - ( ' ' + 22 + NULL ) = 40 remaining bytes for our string.
							_wmemcpy_s( status_bar_buf, 64, ST_V_Total_downloaded_, buf_length );
							status_bar_buf[ buf_length ] = ' ';
							// The maximum length that FormatSizes can return is 22 bytes excluding the NULL terminator.
							FormatSizes( status_bar_buf + ( buf_length + 1 ), 64 - ( buf_length + 1 ), cfg_t_status_downloaded, session_total_downloaded );
							// NULL terminator is set in FormatSizes.

							_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );
						}
					}
				}
				break;

				case NM_DBLCLK:
				{
					NMITEMACTIVATE *nmitem = ( NMITEMACTIVATE * )lParam;

					if ( nmitem->hdr.hwndFrom == g_hWnd_files )
					{
						_SendMessageW( hWnd, WM_COMMAND, MENU_OPEN_DIRECTORY, 0 );
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					_SendMessageW( hWnd, WM_KEYDOWN, ( ( NMLVKEYDOWN * )lParam )->wVKey, 0 );
				}
				break;

				/*case LVN_DELETEITEM:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					// Item count will be 1 since the item hasn't yet been deleted.
					if ( _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) == 1 )
					{
						// Disable the menus that require at least one item in the list.
						UpdateMenus( true );
					}
				}
				break;*/

				case LVN_ITEMCHANGED:
				{
					//NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					if ( !in_worker_thread )
					{
						UpdateMenus( true );
					}
				}
				break;

				case LVN_HOTTRACK:
				{
					NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

					LVHITTESTINFO lvhti;
					_memzero( &lvhti, sizeof( LVHITTESTINFO ) );
					lvhti.pt = nmlv->ptAction;

					_SendMessageW( g_hWnd_files, LVM_HITTEST, 0, ( LPARAM )&lvhti );

					if ( lvhti.iItem != last_tooltip_item )
					{
						// Save the last item that was hovered so we don't have to keep calling everything below.
						last_tooltip_item = lvhti.iItem;

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_files;

						_SendMessageW( g_hWnd_tooltip, TTM_GETTOOLINFO, 0, ( LPARAM )&ti );

						ti.lpszText = NULL;	// If we aren't hovered over an item or the download info is NULL, then we'll end up not showing the tooltip text.

						if ( lvhti.iItem != -1 )
						{
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;
							lvi.iItem = lvhti.iItem;

							_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

							DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;

							if ( di != NULL )
							{
								if ( di->file_size > 0 )
								{
									__snwprintf( tooltip_buffer, 512, L"%s: %s\r\n%s: %llu / %llu bytes\r\n%s: %s", ST_V_Filename, di->file_path + di->filename_offset, ST_V_Downloaded, di->downloaded, di->file_size, ST_V_Added, di->w_add_time );
								}
								else
								{
									__snwprintf( tooltip_buffer, 512, L"%s: %s\r\n%s: %llu / ? bytes\r\n%s: %s", ST_V_Filename, di->file_path + di->filename_offset, ST_V_Downloaded, di->downloaded, ST_V_Added, di->w_add_time );
								}

								ti.lpszText = tooltip_buffer;

								if ( di->status == STATUS_DOWNLOADING )
								{
									last_tooltip_item = -2;	// Allow active downloads to update the tooltip if their item is rehovered.
								}
							}
						}

						_SendMessageW( g_hWnd_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );
					}
				}
				break;

				case LVN_BEGINLABELEDIT:
				{
					NMLVDISPINFO *pdi = ( NMLVDISPINFO * )lParam;

					bool skip_hit_test = edit_from_menu;
					edit_from_menu = false;

					// If no item is being edited or the filename column is not displayed, then cancel the edit.
					if ( pdi->item.iItem == -1 || *download_columns[ 8 ] == -1 )
					{
						return TRUE;
					}

					// Get the current list item text from its lParam.
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.iItem = pdi->item.iItem;
					lvi.mask = LVIF_PARAM;
					_SendMessageW( pdi->hdr.hwndFrom, LVM_GETITEM, 0, ( LPARAM )&lvi );

					DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
					if ( di != NULL )
					{
						if ( skip_hit_test )
						{
							current_edit_pos.top = GetColumnIndexFromVirtualIndex( *download_columns[ 8 ], download_columns, NUM_COLUMNS );	// Filename column index.
						}
						else
						{
							LVHITTESTINFO lvhti;
							_memzero( &lvhti, sizeof( LVHITTESTINFO  ) );
							_GetCursorPos( &lvhti.pt );
							_ScreenToClient( g_hWnd_files, &lvhti.pt ); 
							_SendMessageW( g_hWnd_files, LVM_SUBITEMHITTEST, 0, ( LPARAM )&lvhti );

							// Make sure it's the filename column that we hit.
							if ( GetVirtualIndexFromColumnIndex( lvhti.iSubItem, download_columns, NUM_COLUMNS ) != 8 )
							{
								return TRUE;
							}

							current_edit_pos.top = lvhti.iSubItem;	// Filename column index.
						}

						current_edit_pos.left = LVIR_BOUNDS;
						_SendMessageW( pdi->hdr.hwndFrom, LVM_GETSUBITEMRECT, pdi->item.iItem, ( LPARAM )&current_edit_pos );	// Get the bounding box of the Filename column we're editing.

						// Get the edit control that the listview creates.
						g_hWnd_lv_edit = ( HWND )_SendMessageW( pdi->hdr.hwndFrom, LVM_GETEDITCONTROL, 0, 0 );

						// Subclass our edit window to modify its position.
						EditProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_lv_edit, GWLP_WNDPROC );
						_SetWindowLongPtrW( g_hWnd_lv_edit, GWLP_WNDPROC, ( LONG_PTR )EditSubProc );

						// Set our edit control's text to the list item's text.
						_SendMessageW( g_hWnd_lv_edit, WM_SETTEXT, NULL, ( LPARAM )( di->file_path + di->filename_offset ) );

						// Get the length of the filename without the extension.
						int ext_len = lstrlenW( di->file_path + di->filename_offset );
						while ( ext_len != 0 && ( di->file_path + di->filename_offset )[ --ext_len ] != L'.' );

						// Select all the text except the file extension (if ext_len = 0, then everything is selected)
						_SendMessageW( g_hWnd_lv_edit, EM_SETSEL, 0, ext_len );

						// Limit the length of the filename so that the file directory + filename + NULL isn't greater than MAX_PATH.
						_SendMessageW( g_hWnd_lv_edit, EM_LIMITTEXT, MAX_PATH - ( di->filename_offset + 1 ), 0 );
					}

					// Allow the edit to proceed.
				}
				break;

				case LVN_ENDLABELEDIT:
				{
					NMLVDISPINFO *pdi = ( NMLVDISPINFO * )lParam;

					// Prevent the edit if there's no text.
					if ( pdi->item.pszText != NULL )
					{
						// Prevent the edit if the text length is 0.
						unsigned int filename_length = lstrlenW( pdi->item.pszText );
						if ( filename_length > 0 )
						{
							// Get the current list item text from its lParam.
							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.iItem = pdi->item.iItem;
							lvi.mask = LVIF_PARAM;
							_SendMessageW( pdi->hdr.hwndFrom, LVM_GETITEM, 0, ( LPARAM )&lvi );

							DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
							if ( di != NULL )
							{
								RENAME_INFO *ri = ( RENAME_INFO * )GlobalAlloc( GPTR, sizeof( RENAME_INFO ) );
								ri->di = di;
								ri->filename_length = filename_length;
								ri->filename = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( ri->filename_length + 1 ) );
								_wmemcpy_s( ri->filename, ri->filename_length + 1, pdi->item.pszText, ri->filename_length );
								ri->filename[ ri->filename_length ] = 0;	// Sanity.

								// ri is freed in rename_file.
								HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, rename_file, ( void * )ri, 0, NULL );
								if ( thread != NULL )
								{
									CloseHandle( thread );
								}
								else
								{
									GlobalFree( ri->filename );
									GlobalFree( ri );
								}
							}

							return TRUE;
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

				_DeferWindowPos( hdwp, g_hWnd_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom - ( rc_status.bottom - rc_status.top ), SWP_NOZORDER );

				// Apparently status bars want WM_SIZE to be called. (See MSDN)
				_SendMessageW( g_hWnd_status, WM_SIZE, 0, 0 );
			}
			else
			{
				_DeferWindowPos( hdwp, g_hWnd_files, HWND_TOP, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER );
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
			if ( is_close( rc->left, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left, 0 );
			}
			else if ( is_close( wa.right, rc->right ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right, 0 );
			}

			if ( is_close( rc->top, wa.top ) )				// Attach to top of the desktop.
			{
				_OffsetRect( rc, 0, wa.top - rc->top );
			}
			else if ( is_close( wa.bottom, rc->bottom ) )	// Attach to bottom of the desktop.
			{
				_OffsetRect( rc, 0, wa.bottom - rc->bottom );
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

		case WM_MEASUREITEM:
		{
			// Set the row height of the list view.
			if ( ( ( LPMEASUREITEMSTRUCT )lParam )->CtlType = ODT_LISTVIEW )
			{
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = g_row_height;// * 4;
			}
			return TRUE;
		}
		break;

		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The item we want to draw is our listview.
			if ( dis->CtlType == ODT_LISTVIEW && dis->itemData != NULL )
			{
				// Alternate item color's background.
				HBRUSH color = _CreateSolidBrush( ( dis->itemID & 1 ? cfg_even_row_background_color : cfg_odd_row_background_color ) );
				_FillRect( dis->hDC, &dis->rcItem, color );
				_DeleteObject( color );

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					if ( skip_list_draw )
					{
						return TRUE;	// Don't draw selected items because their lParam values are being deleted.
					}

					HBRUSH color = _CreateSolidBrush( ( dis->itemID & 1 ? cfg_even_row_highlight_color : cfg_odd_row_highlight_color ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
					selected = true;
				}

				// Get the item's text.
				wchar_t tbuf[ 128 ];
				wchar_t *buf = tbuf;

				// This is the full size of the row.
				RECT last_rc;

				// This will keep track of the current colunn's left position.
				int last_left = 0;

				int DT_ALIGN = 0;

				int arr[ NUM_COLUMNS ];
				int arr2[ NUM_COLUMNS ];

				int column_count = 0;
				if ( dis->hwndItem == g_hWnd_files )
				{
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );

					_memcpy_s( arr2, sizeof( int ) * NUM_COLUMNS, arr, sizeof( int ) * NUM_COLUMNS );

					// Offset the virtual indices to match the actual index.
					OffsetVirtualIndices( arr2, download_columns, NUM_COLUMNS, g_total_columns );

					column_count = g_total_columns;
				}

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )dis->itemData;

				LVCOLUMN lvc;
				_memzero( &lvc, sizeof( LVCOLUMN ) );
				lvc.mask = LVCF_WIDTH;

				// Loop through all the columns
				for ( int i = 0; i < column_count; ++i )
				{
					if ( dis->hwndItem == g_hWnd_files )
					{
						// Save the appropriate text in our buffer for the current column.
						buf = GetDownloadInfoString( di, arr2[ i ], dis->itemID + 1, tbuf, 128 );

						switch ( arr2[ i ] )
						{
							case 0:		// NUM
							case 2:		// DATE AND TIME ADDED
							case 3:		// DOWNLOAD DIRECTORY
							case 8:		// FILENAME
							case 10:	// SSL / TLS Version
							case 13:	// URL
							{
								DT_ALIGN = DT_LEFT;
							}
							break;

							case 1:		// ACTIVE PARTS
							case 4:		// DOWNLOAD SPEED
							case 5:		// DOWNLOADED
							case 6:		// FILE SIZE
							case 11:	// TIME ELAPSED
							case 12:	// TIME REMAINING
							{
								DT_ALIGN = DT_RIGHT;
							}
							break;

							/*case 7:	// FILE TYPE
							{
							}
							break;*/

							case 9:		// PROGRESS
							{
								DT_ALIGN = DT_CENTER;
							}
							break;
						}
					}

					if ( buf == NULL )
					{
						tbuf[ 0 ] = L'\0';
						buf = tbuf;
					}

					// Get the dimensions of the listview column
					_SendMessageW( dis->hwndItem, LVM_GETCOLUMN, arr[ i ], ( LPARAM )&lvc );

					last_rc = dis->rcItem;

					if ( arr2[ i ] != 9 )	// Not progress column.
					{
						// This will adjust the text to fit nicely into the rectangle.
						last_rc.left = 5 + last_left;
						last_rc.right = lvc.cx + last_left - 5;
					}
					else	// Adjust progress bar.
					{
						last_rc.left = 2 + last_left;
						last_rc.right = lvc.cx + last_left - 1;
						last_rc.top += 1;

						if ( cfg_show_gridlines )
						{
							last_rc.bottom -= 2;
						}
						else
						{
							--last_rc.bottom;
						}
					}

					// Save the last left position of our column.
					last_left += lvc.cx;

					// Save the height and width of this region.
					int width = last_rc.right - last_rc.left;
					if ( width <= 0 )
					{
						continue;
					}

					int height = last_rc.bottom - last_rc.top;

					// Normal text position.
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = width;
					rc.bottom = height;

					// Create and save a bitmap in memory to paint to.
					HDC hdcMem = _CreateCompatibleDC( dis->hDC );
					HBITMAP hbm = _CreateCompatibleBitmap( dis->hDC, width, height );
					HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
					_DeleteObject( ohbm );
					_DeleteObject( hbm );
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, ( dis->itemID & 1 ? cfg_even_row_font_settings.font : cfg_odd_row_font_settings.font ) );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					if ( arr2[ i ] == 7 )	// File Type
					{
						if ( di->icon != NULL )
						{
							int icon_top_offset = g_row_height - ( cfg_show_gridlines ? 18 : 16 );	// 16 pixel height + 2.
							if ( icon_top_offset > 0 )
							{
								if ( icon_top_offset & 1 )
								{
									++icon_top_offset;
								}

								icon_top_offset /= 2;
							}
							else
							{
								icon_top_offset = 1;
							}

							_DrawIconEx( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top + icon_top_offset, *di->icon, 0, 0, NULL, NULL, DI_NORMAL );
						}
					}
					else if ( arr2[ i ] == 9 )	// Progress
					{
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, WHITENESS );

						RECT rc_clip = rc;

						// Connecting, Downloading, Paused, Queued, Stopped.
						if ( IS_STATUS( di->status,
								STATUS_CONNECTING |
								STATUS_DOWNLOADING |
								STATUS_STOPPED ) )
						{
							if ( di->file_size > 0 )
							{
#ifdef _WIN64
								int i_percentage = ( int )( ( float )width * ( ( float )di->last_downloaded / ( float )di->file_size ) );
#else
								// Multiply the floating point division by 100%.
								float f_percentage = ( float )width * ( ( float )di->last_downloaded / ( float )di->file_size );
								int i_percentage = 0;
								__asm
								{
									fld f_percentage;	//; Load the floating point value onto the FPU stack.
									fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}
#endif
								rc_clip.right = i_percentage;
							}
							else
							{
								rc_clip.right = 0;
							}
						}

						COLORREF color_ref_body = 0, color_ref_border = 0, color_ref_background = 0, color_ref_body_text = 0, color_ref_background_text = 0;

						if		( di->status == STATUS_CONNECTING )			{ color_ref_body = cfg_color_4a; color_ref_background = cfg_color_4b; color_ref_body_text = cfg_color_4c; color_ref_background_text = cfg_color_4d; color_ref_border = cfg_color_4e; }
						else if ( IS_STATUS( di->status, STATUS_RESTART ) )	{ color_ref_body = cfg_color_12a; color_ref_background = cfg_color_12b; color_ref_body_text = cfg_color_12c; color_ref_background_text = cfg_color_12d; color_ref_border = cfg_color_12e; }
						else if ( IS_STATUS( di->status, STATUS_PAUSED ) )	{ color_ref_body = cfg_color_9a; color_ref_background = cfg_color_9b; color_ref_body_text = cfg_color_9c; color_ref_background_text = cfg_color_9d; color_ref_border = cfg_color_9e; }
						else if ( IS_STATUS( di->status, STATUS_QUEUED ) )	{ color_ref_body = cfg_color_11a; color_ref_background = cfg_color_11b; color_ref_body_text = cfg_color_11c; color_ref_background_text = cfg_color_11d; color_ref_border = cfg_color_11e; }
						else if ( di->status == STATUS_COMPLETED )			{ color_ref_body = cfg_color_3a; color_ref_background = cfg_color_3b; color_ref_body_text = cfg_color_3c; color_ref_background_text = cfg_color_3d; color_ref_border = cfg_color_3e; }
						else if ( di->status == STATUS_STOPPED )			{ color_ref_body = cfg_color_14a; color_ref_background = cfg_color_14b; color_ref_body_text = cfg_color_14c; color_ref_background_text = cfg_color_14d; color_ref_border = cfg_color_14e; }
						else if ( di->status == STATUS_TIMED_OUT )			{ color_ref_body = cfg_color_15a; color_ref_background = cfg_color_15b; color_ref_body_text = cfg_color_15c; color_ref_background_text = cfg_color_15d; color_ref_border = cfg_color_15e; }
						else if ( di->status == STATUS_FAILED )				{ color_ref_body = cfg_color_6a; color_ref_background = cfg_color_6b; color_ref_body_text = cfg_color_6c; color_ref_background_text = cfg_color_6d; color_ref_border = cfg_color_6e; }
						else if ( di->status == STATUS_FILE_IO_ERROR )		{ color_ref_body = cfg_color_7a; color_ref_background = cfg_color_7b; color_ref_body_text = cfg_color_7c; color_ref_background_text = cfg_color_7d; color_ref_border = cfg_color_7e; }
						else if ( di->status == STATUS_SKIPPED )			{ color_ref_body = cfg_color_13a; color_ref_background = cfg_color_13b; color_ref_body_text = cfg_color_13c; color_ref_background_text = cfg_color_13d; color_ref_border = cfg_color_13e; }
						else if ( di->status == STATUS_AUTH_REQUIRED )		{ color_ref_body = cfg_color_2a; color_ref_background = cfg_color_2b; color_ref_body_text = cfg_color_2c; color_ref_background_text = cfg_color_2d; color_ref_border = cfg_color_2e; }
						else if ( di->status == STATUS_PROXY_AUTH_REQUIRED ){ color_ref_body = cfg_color_10a; color_ref_background = cfg_color_10b; color_ref_body_text = cfg_color_10c; color_ref_background_text = cfg_color_10d; color_ref_border = cfg_color_10e; }
						else if	( di->status == STATUS_ALLOCATING_FILE )	{ color_ref_body = cfg_color_1a; color_ref_background = cfg_color_1b; color_ref_body_text = cfg_color_1c; color_ref_background_text = cfg_color_1d; color_ref_border = cfg_color_1e; }
						else if	( di->status == STATUS_MOVING_FILE )		{ color_ref_body = cfg_color_8a; color_ref_background = cfg_color_8b; color_ref_body_text = cfg_color_8c; color_ref_background_text = cfg_color_8d; color_ref_border = cfg_color_8e; }
						else												{ color_ref_body = cfg_color_5a; color_ref_background = cfg_color_5b; color_ref_body_text = cfg_color_5c; color_ref_background_text = cfg_color_5d; color_ref_border = cfg_color_5e; }

						HBRUSH color = _CreateSolidBrush( color_ref_background );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						_SetTextColor( hdcMem, color_ref_background_text );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );

						color = _CreateSolidBrush( color_ref_body );
						_FillRect( hdcMem, &rc_clip, color );
						_DeleteObject( color );

						_SetTextColor( hdcMem, color_ref_body_text );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, ( rc_clip.right - rc_clip.left ), height, hdcMem, 0, 0, SRCCOPY );



						HPEN hPen = _CreatePen( PS_SOLID, 1, color_ref_border );
						_SelectObject( dis->hDC, hPen );
						_SelectObject( dis->hDC, _GetStockObject( NULL_BRUSH ) );
						_Rectangle( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, dis->rcItem.left + last_rc.left + width, last_rc.top + height );
						_DeleteObject( hPen );

						//--last_rc.bottom;
						//_DrawEdge( dis->hDC, &last_rc, EDGE_RAISED, BF_RECT );
					}
					else
					{
						// Draw selected text
						if ( selected )
						{
							// Fill the background.
							HBRUSH color = _CreateSolidBrush( ( dis->itemID & 1 ? cfg_even_row_highlight_color : cfg_odd_row_highlight_color ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// White text.
							_SetTextColor( hdcMem, ( dis->itemID & 1 ? cfg_even_row_highlight_font_color : cfg_odd_row_highlight_font_color ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
						}
						else
						{
							// Fill the background.
							HBRUSH color = _CreateSolidBrush( ( dis->itemID & 1 ? cfg_even_row_background_color : cfg_odd_row_background_color ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							_SetTextColor( hdcMem, ( dis->itemID & 1 ? cfg_even_row_font_settings.font_color : cfg_odd_row_font_settings.font_color ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
						}
					}

					// Delete our back buffer.
					_DeleteDC( hdcMem );
				}
			}
			return TRUE;
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

				_SendMessageW( hWnd, WM_PROPAGATE, -2, ( LPARAM )new_cla );
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
					// cli is freed in process_command_line_args.
					HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, process_command_line_args, ( void * )cla, 0, NULL );
					if ( thread != NULL )
					{
						CloseHandle( thread );
					}
					else
					{
						GlobalFree( cla->download_directory );
						GlobalFree( cla->download_history_file );
						GlobalFree( cla->url_list_file );
						GlobalFree( cla->urls );
						GlobalFree( cla->cookies );
						GlobalFree( cla->headers );
						GlobalFree( cla->data );
						GlobalFree( cla->username );
						GlobalFree( cla->password );
						GlobalFree( cla );
					}
				}
			}
			else
			{
				if ( g_hWnd_add_urls == NULL )
				{
					g_hWnd_add_urls = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"add_urls", ST_V_Add_URL_s_, WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 600 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 263 ) / 2 ), 600, 263, NULL, NULL, NULL, NULL );
				}

				_SendMessageW( g_hWnd_add_urls, WM_PROPAGATE, wParam, lParam );
			}
		}
		break;

		case WM_ALERT:
		{
			if ( wParam == 0 )
			{
				_MessageBoxW( hWnd, ( LPCWSTR )lParam, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
			}
			else if ( wParam == 1 )
			{
				if ( _MessageBoxW( hWnd, ST_V_PROMPT_The_specified_file_was_not_found, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )3, 0, NULL ) );	// Restart download (from the beginning).
				}
			}
		}
		break;

		case WM_ACTIVATE:
		{
			ClearProgressBars();

			_SetFocus( g_hWnd_files );

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

			if ( g_hWnd_url_drop_window != NULL )
			{
				_DestroyWindow( g_hWnd_url_drop_window );
			}

			if ( cfg_enable_download_history && download_history_changed )
			{
				_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\download_history\0", 18 );
				base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

				save_download_history( base_directory );
				download_history_changed = false;
			}

			// Get the number of items in the listview.
			int num_items = ( int )_SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			// Go through each item, and free their lParam values.
			for ( lvi.iItem = 0; lvi.iItem < num_items; ++lvi.iItem )
			{
				_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
				if ( di != NULL )
				{
					// di->icon is stored in the icon_handles tree and is destroyed in main.
					GlobalFree( di->url );
					GlobalFree( di->w_add_time );
					GlobalFree( di->cookies );
					GlobalFree( di->headers );
					GlobalFree( di->data );
					//GlobalFree( di->etag );
					GlobalFree( di->auth_info.username );
					GlobalFree( di->auth_info.password );

					if ( di->hFile != INVALID_HANDLE_VALUE )
					{
						CloseHandle( di->hFile );
					}

					while ( di->range_list != NULL )
					{
						DoublyLinkedList *range_node = di->range_list;
						di->range_list = di->range_list->next;

						GlobalFree( range_node->data );
						GlobalFree( range_node );
					}

					while ( di->range_queue != NULL )
					{
						DoublyLinkedList *range_node = di->range_queue;
						di->range_queue = di->range_queue->next;

						GlobalFree( range_node->data );
						GlobalFree( range_node );
					}

					DeleteCriticalSection( &di->shared_cs );

					GlobalFree( di );
				}
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
				UnregisterDropWindow( g_hWnd_files, List_DropTarget );

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

			if ( login_list_changed )
			{
				save_login_info();

				login_list_changed = false;
			}

			if ( cfg_enable_download_history && download_history_changed )
			{
				_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\download_history\0", 18 );
				base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

				save_download_history( base_directory );
				download_history_changed = false;
			}

			return 0;
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

	return TRUE;
}
