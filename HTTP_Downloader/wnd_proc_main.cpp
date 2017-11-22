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

#include "lite_gdi32.h"
#include "lite_ole32.h"

#include "list_operations.h"
#include "file_operations.h"

#include "connection.h"
#include "menus.h"

#include "http_parsing.h"
#include "utilities.h"

#include "drag_and_drop.h"

#include "string_tables.h"

HWND g_hWnd_files = NULL;
HWND g_hWnd_status = NULL;

HWND g_hWnd_tooltip = NULL;

wchar_t *tooltip_buffer = NULL;
int last_tooltip_item = -1;				// Prevent our hot tracking from calling the tooltip on the same item.

NOTIFYICONDATA g_nid;					// Tray icon information.

HCURSOR wait_cursor = NULL;				// Temporary cursor while processing entries.

bool skip_list_draw = false;

int cx = 0;								// Current x (left) position of the main window based on the mouse.
int cy = 0;								// Current y (top) position of the main window based on the mouse.

unsigned char g_total_columns = 0;

unsigned long long session_total_downloaded = 0;
unsigned long long session_downloaded_speed = 0;

HANDLE g_timer_semaphore = NULL;

IDropTarget *List_DropTarget;

// Sort function for columns.
int CALLBACK DMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	int arr[ NUM_COLUMNS ];

	sortinfo *si = ( sortinfo * )lParamSort;

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
			case 10: { return ( di1->time_elapsed > di2->time_elapsed ); } break;
			case 11: { return ( di1->time_remaining > di2->time_remaining ); } break;

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

						// Multiply the floating point division by 1000%.
						// This leaves us with an integer in which the last digit will represent the decimal value.
						float f_percentage1 = 1000.f * ( ( float )di1->last_downloaded / ( float )di1->file_size );
						int i_percentage1 = 0;
						_asm
						{
							fld f_percentage1;		//; Load the floating point value onto the FPU stack.
							fistp i_percentage1;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
						}

						// Multiply the floating point division by 1000%.
						// This leaves us with an integer in which the last digit will represent the decimal value.
						float f_percentage2 = 1000.f * ( ( float )di2->last_downloaded / ( float )di2->file_size );
						int i_percentage2 = 0;
						_asm
						{
							fld f_percentage2;		//; Load the floating point value onto the FPU stack.
							fistp i_percentage2;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
						}

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

			case 12:
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
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1024.0f;
		unsigned int i_percentage = 0;
		_asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}

		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%lu.%02lu KB", i_percentage, remainder );
	}
	else if ( toggle_type == SIZE_FORMAT_MEGABYTE )
	{
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1048576.0f;
		unsigned int i_percentage = 0;
		_asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}

		// Get the last digit (decimal value).
		unsigned int remainder = i_percentage % 100;
		i_percentage /= 100;

		length = __snwprintf( buffer, buffer_size, L"%lu.%02lu MB", i_percentage, remainder );
	}
	else if ( toggle_type == SIZE_FORMAT_GIGABYTE )
	{
		// This leaves us with an integer in which the last digit will represent the decimal value.
		float f_percentage = 100.0f * ( float )data_size / 1073741824.0f;
		unsigned int i_percentage = 0;
		_asm
		{
			fld f_percentage;	//; Load the floating point value onto the FPU stack.
			fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
		}

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

	last_update.ull = 0;

	_wmemcpy_s( sb_download_speed_buf, 64, ST_Download_speed_, 15 );
	sb_download_speed_buf[ 15 ] = ' ';

	_wmemcpy_s( sb_downloaded_buf, 64, ST_Total_downloaded_, 17 );
	sb_downloaded_buf[ 17 ] = ' ';

	_wmemcpy_s( title_text, 128, L"HTTP Downloader", 15 );

	_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"HTTP Downloader", 15 );

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
							if ( di->status == STATUS_CONNECTING ||
								 di->status == STATUS_DOWNLOADING ||
								 di->status == STATUS_PAUSED ||
								 di->status == STATUS_ALLOCATING_FILE )
							{
								di->time_elapsed = ( current_time.ull - di->start_time.QuadPart ) / FILETIME_TICKS_PER_SECOND;
							}

							// If downloading, then calculate the speed.
							if ( di->status == STATUS_DOWNLOADING )
							{
								// Determine the difference (in milliseconds) between the current time and our last update time.
								time_difference = ( current_time.ull - last_update.ull ) / ( FILETIME_TICKS_PER_SECOND / 1000 );	// Use milliseconds.

								// See if at least 1 second has elapsed since we last updated our speed and download time estimate.
								if ( time_difference > 1000 )	// Measure in milliseconds for better precision. 1000 milliseconds = 1 second.
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
							}
							else if ( di->status == STATUS_PAUSED || di->status == STATUS_QUEUED )
							{
								di->time_remaining = 0;
								di->speed = 0;
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
			sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + 16, 64 - 16, cfg_t_status_down_speed, session_downloaded_speed ) + 16;
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
			sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + 18, 64 - 18, cfg_t_status_downloaded, session_total_downloaded ) + 18;

			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )sb_downloaded_buf );

			last_session_total_downloaded = session_total_downloaded;

			update_text_values = true;
		}

		if ( run_timer )
		{
			if ( update_text_values )
			{
				int tooltip_offset = 15, title_text_offset = 15;

				sb_download_speed_buf_length = FormatSizes( sb_download_speed_buf + 16, 64 - 16, SIZE_FORMAT_AUTO, session_downloaded_speed ) + 16;
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L'/';
				sb_download_speed_buf[ sb_download_speed_buf_length++ ] = L's';
				sb_download_speed_buf[ sb_download_speed_buf_length ] = 0;	// Sanity.

				//if ( sb_download_speed_buf_length > 0 )
				//{
					_wmemcpy_s( g_nid.szTip + tooltip_offset, sizeof( g_nid.szTip ) - tooltip_offset, L"\r\n", 2 );
					tooltip_offset += 2;
					_wmemcpy_s( g_nid.szTip + tooltip_offset, sizeof( g_nid.szTip ) - tooltip_offset, sb_download_speed_buf, sb_download_speed_buf_length );
					tooltip_offset += sb_download_speed_buf_length;

					_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, L" - ", 3 );
					title_text_offset += 3;
					_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, sb_download_speed_buf, sb_download_speed_buf_length );
					title_text_offset += sb_download_speed_buf_length;
				//}

				sb_downloaded_buf_length = FormatSizes( sb_downloaded_buf + 18, 64 - 18, SIZE_FORMAT_AUTO, session_total_downloaded ) + 18;

				//if ( sb_downloaded_buf_length > 0 )
				//{
					_wmemcpy_s( g_nid.szTip + tooltip_offset, sizeof( g_nid.szTip ) - tooltip_offset, L"\r\n", 2 );
					tooltip_offset += 2;
					_wmemcpy_s( g_nid.szTip + tooltip_offset, sizeof( g_nid.szTip ) - tooltip_offset, sb_downloaded_buf, sb_downloaded_buf_length );
					tooltip_offset += sb_downloaded_buf_length;

					_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, L" - ", 3 );
					title_text_offset += 3;
					_wmemcpy_s( title_text + title_text_offset, 128 - title_text_offset, sb_downloaded_buf, sb_downloaded_buf_length );
					title_text_offset += sb_downloaded_buf_length;
				//}

				g_nid.szTip[ tooltip_offset ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_MODIFY, &g_nid );

				title_text[ title_text_offset ] = 0;	// Sanity.
				_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )title_text );
			}
		}
		else
		{
			/*if ( g_nid.szTip[ 0 ] == NULL )
			{
				_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"HTTP Downloader\0", 16 );
			}*/
			g_nid.szTip[ 15 ] = 0;	// Sanity.
			_Shell_NotifyIconW( NIM_MODIFY, &g_nid );

			_SendMessageW( g_hWnd_main, WM_SETTEXT, NULL, ( LPARAM )PROGRAM_CAPTION );
		}
	}

	CloseHandle( g_timer_semaphore );
	g_timer_semaphore = NULL;

	_ExitThread( 0 );
	return 0;
}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			g_hWnd_files = _CreateWindowW( WC_LISTVIEW, NULL, LVS_REPORT | LVS_OWNERDRAWFIXED | WS_CHILDWINDOW | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_files, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );

			g_hWnd_status = _CreateWindowW( STATUSCLASSNAME, NULL, SBARS_SIZEGRIP | WS_CHILDWINDOW | ( cfg_show_status_bar ? WS_VISIBLE : 0 ), 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hWnd_files, NULL, NULL, NULL );

			TOOLINFO ti;
			_memzero( &ti, sizeof( TOOLINFO ) );
			ti.cbSize = sizeof( TOOLINFO );
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = g_hWnd_files;

			_SendMessageW( g_hWnd_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&ti );
			_SendMessageW( g_hWnd_tooltip, TTM_SETMAXTIPWIDTH, 0, sizeof( wchar_t ) * ( 2 * MAX_PATH ) );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767 );
			_SendMessageW( g_hWnd_tooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 2000 );

			_SendMessageW( g_hWnd_files, LVM_SETTOOLTIPS, ( WPARAM )g_hWnd_tooltip, 0 );


			_SendMessageW( g_hWnd_files, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_status, WM_SETFONT, ( WPARAM )hFont, 0 );


			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_files, &List_DropTarget );
			}

			int status_bar_widths[] = { 250, -1 };

			_SendMessageW( g_hWnd_status, SB_SETPARTS, 2, ( LPARAM )status_bar_widths );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 0, 0 ), ( LPARAM )( cfg_t_status_down_speed == SIZE_FORMAT_BYTE ? ST_Download_speed__0_B_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_KILOBYTE ? ST_Download_speed__0_00_KB_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_MEGABYTE ? ST_Download_speed__0_00_MB_s :
																					( cfg_t_status_down_speed == SIZE_FORMAT_GIGABYTE ? ST_Download_speed__0_00_GB_s : ST_Download_speed__0_B_s ) ) ) ) );
			_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )( cfg_t_status_downloaded == SIZE_FORMAT_BYTE ? ST_Total_downloaded__0_B :
																					( cfg_t_status_downloaded == SIZE_FORMAT_KILOBYTE ? ST_Total_downloaded__0_00_KB :
																					( cfg_t_status_downloaded == SIZE_FORMAT_MEGABYTE ? ST_Total_downloaded__0_00_MB :
																					( cfg_t_status_downloaded == SIZE_FORMAT_GIGABYTE ? ST_Total_downloaded__0_00_GB : ST_Total_downloaded__0_B ) ) ) ) );

			int arr[ NUM_COLUMNS ];

			// Initialize our listview columns
			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

			for ( char i = 0; i < NUM_COLUMNS; ++i )
			{
				if ( i == 1 || i == 4 || i == 5 || i == 6 || i == 10 || i == 11 )
				{
					lvc.fmt = LVCFMT_RIGHT;
				}
				else if ( i == 9 )
				{
					lvc.fmt = LVCFMT_CENTER;
				}
				else
				{
					lvc.fmt = LVCFMT_LEFT;
				}

				if ( *download_columns[ i ] != -1 )
				{
					lvc.pszText = download_string_table[ i ];
					lvc.cx = *download_columns_width[ i ];
					_SendMessageW( g_hWnd_files, LVM_INSERTCOLUMN, g_total_columns, ( LPARAM )&lvc );

					arr[ g_total_columns++ ] = *download_columns[ i ];
				}
			}

			_SendMessageW( g_hWnd_files, LVM_SETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );

			if ( cfg_tray_icon )
			{
				_memzero( &g_nid, sizeof( NOTIFYICONDATA ) );
				g_nid.cbSize = sizeof( g_nid );
				g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				g_nid.hWnd = hWnd;
				g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
				g_nid.uID = 1000;
				g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
				_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), L"HTTP Downloader\0", 16 );
				g_nid.szTip[ 15 ] = 0;	// Sanity.
				_Shell_NotifyIconW( NIM_ADD, &g_nid );
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
				CloseHandle( _CreateThread( NULL, 0, load_download_history, NULL, 0, NULL ) );
			}

			tooltip_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * 512 );

			return 0;
		}
		break;

		/*case WM_ENTERMENULOOP:
		{
			if ( ( BOOL )wParam == FALSE )
			{
				UpdateMenus( true );
			}
		}
		break;*/

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case MENU_OPEN_FILE:
				case MENU_OPEN_DIRECTORY:
				{
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM;
					lvi.iItem = _SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

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

							if ( LOWORD( wParam ) == MENU_OPEN_FILE )
							{
								wchar_t file_path[ MAX_PATH ];
								_wmemcpy_s( file_path, MAX_PATH, di->file_path, MAX_PATH );
								if ( di->filename_offset > 0 )
								{
									file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
								}

								// Set the verb to NULL so that unknown file types can be handled by the system.
								HINSTANCE hInst = _ShellExecuteW( NULL, NULL, file_path, NULL, NULL, SW_SHOWNORMAL );
								if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )
								{
									if ( _MessageBoxW( hWnd, ST_The_specified_file_was_not_found, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
									{
										CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )3, 0, NULL ) );	// Restart download (from the beginning).
									}
								}
							}
							else if ( LOWORD( wParam ) == MENU_OPEN_DIRECTORY )
							{
								HINSTANCE hInst = _ShellExecuteW( NULL, L"open", di->file_path, NULL, NULL, SW_SHOWNORMAL );
								if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )	// We're opening a folder, but it uses the same error code as a file if it's not found.
								{
									_MessageBoxW( hWnd, ST_The_specified_path_was_not_found, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING );
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

				case MENU_REMOVE_COMPLETED:
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )2, 0, NULL ) );
				}
				break;

				case MENU_REMOVE_SELECTED:
				{
					CloseHandle( ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )NULL, 0, NULL ) );
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
				case MENU_TIME_ELAPSED:
				case MENU_TIME_REMAINING:
				case MENU_TLS_SSL_VERSION:
				case MENU_URL:
				{
					UpdateColumns( LOWORD( wParam ) );
				}
				break;

				case MENU_ADD_URLS:
				{
					if ( g_hWnd_add_urls == NULL )
					{
						g_hWnd_add_urls = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"add_urls", ST_Add_URL_s_, WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 505 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 240 ) / 2 ), 505, 240, NULL, NULL, NULL, NULL );
						_ShowWindow( g_hWnd_add_urls, SW_SHOWNORMAL );
					}
					else if ( _IsIconic( g_hWnd_add_urls ) )	// If minimized, then restore the window.
					{
						_ShowWindow( g_hWnd_add_urls, SW_RESTORE );
					}
					_SetForegroundWindow( g_hWnd_add_urls );
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

				case MENU_OPTIONS:
				{
					if ( g_hWnd_options == NULL )
					{
						g_hWnd_options = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"options", ST_Options, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 390 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 350 ) / 2 ), 390, 350, NULL, NULL, NULL, NULL );
						_ShowWindow( g_hWnd_options, SW_SHOWNORMAL );
					}
					_SetForegroundWindow( g_hWnd_options );
				}
				break;

				case MENU_ABOUT:
				{
					wchar_t msg[ 512 ];
					__snwprintf( msg, 512, L"HTTP Downloader is made free under the GPLv3 license.\r\n\r\n" \
										   L"Version 1.0.0.3\r\n\r\n" \
										   L"Built on %s, %s %d, %04d %d:%02d:%02d %s (UTC)\r\n\r\n" \
										   L"Copyright \xA9 2015-2017 Eric Kutcher", GetDay( g_compile_time.wDayOfWeek ), GetMonth( g_compile_time.wMonth ), g_compile_time.wDay, g_compile_time.wYear, ( g_compile_time.wHour > 12 ? g_compile_time.wHour - 12 : ( g_compile_time.wHour != 0 ? g_compile_time.wHour : 12 ) ), g_compile_time.wMinute, g_compile_time.wSecond, ( g_compile_time.wHour >= 12 ? L"PM" : L"AM" ) );

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
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;
			}

			return 0;
		}

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case HDN_ENDDRAG:
				{
					NMHEADER *nmh = ( NMHEADER * )lParam;
					HWND hWnd_parent = _GetParent( nmh->hdr.hwndFrom );

					// Prevent the # columns from moving and the other columns from becoming the first column.
					if ( nmh->iItem == 0 || nmh->pitem->iOrder == 0 )
					{
						// Make sure the # columns are visible.
						if ( hWnd_parent == g_hWnd_files && *download_columns[ 0 ] != -1 )
						{
							nmh->pitem->iOrder = GetColumnIndexFromVirtualIndex( nmh->iItem, download_columns, NUM_COLUMNS );
							return TRUE;
						}
					}

					return FALSE;
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

						sortinfo si;
						si.column = lvc.iOrder;
						si.hWnd = nmlv->hdr.hwndFrom;

						if ( HDF_SORTUP & lvc.fmt )	// Column is sorted upward.
						{
							si.direction = 0;	// Now sort down.

							// Sort down
							lvc.fmt = lvc.fmt & ( ~HDF_SORTUP ) | HDF_SORTDOWN;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, ( WPARAM )nmlv->iSubItem, ( LPARAM )&lvc );

							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )DMCompareFunc );
						}
						else if ( HDF_SORTDOWN & lvc.fmt )	// Column is sorted downward.
						{
							si.direction = 1;	// Now sort up.

							// Sort up
							lvc.fmt = lvc.fmt & ( ~HDF_SORTDOWN ) | HDF_SORTUP;
							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SETCOLUMN, nmlv->iSubItem, ( LPARAM )&lvc );

							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )DMCompareFunc );
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

							_SendMessageW( nmlv->hdr.hwndFrom, LVM_SORTITEMS, ( WPARAM )&si, ( LPARAM )( PFNLVCOMPARE )DMCompareFunc );
						}
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
					else
					{
						POINT p;
						_GetCursorPos( &p );

						_TrackPopupMenu( g_hMenuSub_column, 0, p.x, p.y, 0, hWnd, NULL );
					}
				}
				break;

				case NM_CLICK:
				{
					NMMOUSE *nm = ( NMMOUSE * )lParam;

					if ( nm->hdr.hwndFrom == g_hWnd_status )
					{
						wchar_t status_bar_buf[ 64 ];

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

							_wmemcpy_s( status_bar_buf, 64, ST_Download_speed_, 15 );
							status_bar_buf[ 15 ] = ' ';
							unsigned int length = FormatSizes( status_bar_buf + 16, 64 - 16, cfg_t_status_down_speed, session_downloaded_speed ) + 16;
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

							_wmemcpy_s( status_bar_buf, 64, ST_Total_downloaded_, 17 );
							status_bar_buf[ 17 ] = ' ';
							FormatSizes( status_bar_buf + 18, 64 - 18, cfg_t_status_downloaded, session_total_downloaded );

							_SendMessageW( g_hWnd_status, SB_SETTEXT, MAKEWPARAM( 1, 0 ), ( LPARAM )status_bar_buf );
						}
					}
				}
				break;

				case LVN_KEYDOWN:
				{
					// Make sure the control key is down and that we're not already in a worker thread. Prevents threads from queuing in case the user falls asleep on their keyboard.
					if ( _GetKeyState( VK_CONTROL ) & 0x8000 )
					{
						NMLISTVIEW *nmlv = ( NMLISTVIEW * )lParam;

						// Determine which key was pressed.
						switch ( ( ( LPNMLVKEYDOWN )lParam )->wVKey )
						{
							case 'A':	// Select all items if Ctrl + A is down and there are items in the list.
							{
								if ( !in_worker_thread && _SendMessageW( nmlv->hdr.hwndFrom, LVM_GETITEMCOUNT, 0, 0 ) > 0 )
								{
									_SendMessageW( hWnd, WM_COMMAND, MENU_SELECT_ALL, 0 );
								}
							}
							break;
						}
					}
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
					_memzero( &lvhti, sizeof( LVHITTESTINFO  ) );
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
									__snwprintf( tooltip_buffer, 512, L"Filename: %s\r\nDownloaded: %llu / %llu bytes\r\nAdded: %s", di->file_path + di->filename_offset, di->downloaded, di->file_size, di->w_add_time );
								}
								else
								{
									__snwprintf( tooltip_buffer, 512, L"Filename: %s\r\nDownloaded: %llu / ? bytes\r\nAdded: %s", di->file_path + di->filename_offset, di->downloaded, di->w_add_time );
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
			}

			return FALSE;
		}
		break;

		case WM_SIZE:
		{
			RECT rc, rc_status;
			_GetClientRect( hWnd, &rc );

			if ( cfg_show_status_bar )
			{
				_GetWindowRect( g_hWnd_status, &rc_status );

				_SetWindowPos( g_hWnd_files, NULL, rc.left, rc.top, rc.right, rc.bottom - ( rc_status.bottom - rc_status.top ), SWP_NOZORDER );

				// Apparently status bars want WM_SIZE to be called. (See MSDN)
				_SendMessageW( g_hWnd_status, WM_SIZE, 0, 0 );
			}
			else
			{
				_SetWindowPos( g_hWnd_files, NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOZORDER );
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
			if( is_close( rc->left, wa.left ) )				// Attach to left side of the desktop.
			{
				_OffsetRect( rc, wa.left - rc->left, 0 );
			}
			else if ( is_close( wa.right, rc->right ) )		// Attach to right side of the desktop.
			{
				_OffsetRect( rc, wa.right - rc->right, 0 );
			}

			if( is_close( rc->top, wa.top ) )				// Attach to top of the desktop.
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
				( ( LPMEASUREITEMSTRUCT )lParam )->itemHeight = row_height;// * 4;
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
				if ( dis->itemID % 2 )	// Even rows will have a light grey background.
				{
					HBRUSH color = _CreateSolidBrush( ( COLORREF )RGB( 0xF7, 0xF7, 0xF7 ) );
					_FillRect( dis->hDC, &dis->rcItem, color );
					_DeleteObject( color );
				}

				// Set the selected item's color.
				bool selected = false;
				if ( dis->itemState & ( ODS_FOCUS || ODS_SELECTED ) )
				{
					if ( skip_list_draw )
					{
						return TRUE;	// Don't draw selected items because their lParam values are being deleted.
					}

					HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
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
						switch ( arr2[ i ] )
						{
							case 0:	// NUM
							{
								DT_ALIGN = DT_LEFT;

								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 128, L"%lu", dis->itemID + 1 );
							}
							break;

							case 1:	// ACTIVE PARTS
							{
								DT_ALIGN = DT_RIGHT;

								buf = tbuf;	// Reset the buffer pointer.

								__snwprintf( buf, 128, L"%lu/%lu", di->active_parts, di->parts );
							}
							break;

							case 2:	// DATE AND TIME ADDED
							{
								DT_ALIGN = DT_LEFT;

								buf = di->w_add_time;
							}
							break;

							case 3:	// DOWNLOAD DIRECTORY
							{
								DT_ALIGN = DT_LEFT;

								if ( !( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
								{
									buf = di->file_path;
								}
								else
								{
									buf = ST__Simulated_;
								}
							}
							break;

							case 4:	// DOWNLOAD SPEED
							{
								DT_ALIGN = DT_RIGHT;

								if ( di->status != STATUS_DOWNLOADING && di->status != STATUS_PAUSED && di->speed == 0 )
								{
									buf = L"";
								}
								else
								{
									buf = tbuf;	// Reset the buffer pointer.

									unsigned int length = FormatSizes( buf, 128, cfg_t_down_speed, di->speed );
									buf[ length ] = L'/';
									buf[ length + 1 ] = L's';
									buf[ length + 2 ] = 0;
								}
							}
							break;

							case 5:	// DOWNLOADED
							{
								DT_ALIGN = DT_RIGHT;

								buf = tbuf;	// Reset the buffer pointer.

								FormatSizes( buf, 128, cfg_t_downloaded, di->last_downloaded );
							}
							break;

							case 6:	// FILE SIZE
							{
								DT_ALIGN = DT_RIGHT;

								buf = tbuf;	// Reset the buffer pointer.

								if ( di->file_size > 0 ||
								   ( di->status == STATUS_COMPLETED && di->file_size == 0 && di->last_downloaded == 0 ) )
								{
									FormatSizes( buf, 128, cfg_t_file_size, di->file_size );
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
								DT_ALIGN = DT_LEFT;

								buf = di->file_path + di->filename_offset;
							}
							break;

							case 9:	// PROGRESS
							{
								DT_ALIGN = DT_CENTER;

								buf = tbuf;	// Reset the buffer pointer.

								if ( di->file_size > 0 )
								{
									// Multiply the floating point division by 1000%.
									// This leaves us with an integer in which the last digit will represent the decimal value.
									float f_percentage = 1000.f * ( ( float )di->last_downloaded / ( float )di->file_size );
									int i_percentage = 0;
									_asm
									{
										fld f_percentage;	//; Load the floating point value onto the FPU stack.
										fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
									}

									// Get the last digit (decimal value).
									int remainder = i_percentage % 10;
									// Divide the integer by (10%) to get it back in range of 0% to 100%.
									i_percentage /= 10;

									if ( di->status == STATUS_CONNECTING )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Connecting, i_percentage, remainder );
									}
									else if ( di->status == STATUS_PAUSED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Paused, i_percentage, remainder );
									}
									else if ( di->status == STATUS_QUEUED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Queued, i_percentage, remainder );
									}
									else if ( di->status == STATUS_COMPLETED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Completed, i_percentage, remainder );
									}
									else if ( di->status == STATUS_STOPPED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Stopped, i_percentage, remainder );
									}
									else if ( di->status == STATUS_TIMED_OUT )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Timed_Out, i_percentage, remainder );
									}
									else if ( di->status == STATUS_FAILED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Failed, i_percentage, remainder );
									}
									else if ( di->status == STATUS_FILE_IO_ERROR )
									{
										buf = ST_File_IO_Error;
									}
									else if ( di->status == STATUS_SKIPPED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Skipped, i_percentage, remainder );
									}
									else if ( di->status == STATUS_AUTH_REQUIRED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Authorization_Required, i_percentage, remainder );
									}
									else if ( di->status == STATUS_PROXY_AUTH_REQUIRED )
									{
										__snwprintf( buf, 128, L"%s - %d.%1d%%", ST_Proxy_Authentication_Required, i_percentage, remainder );
									}
									else if ( di->status == STATUS_ALLOCATING_FILE )
									{
										buf = ST_Allocating_File;
									}
									else	// Downloading.
									{
										__snwprintf( buf, 128, L"%d.%1d%%", i_percentage, remainder );
									}
								}
								else if ( di->status == STATUS_CONNECTING )
								{
									buf = ST_Connecting;
								}
								else if ( di->status == STATUS_PAUSED )
								{
									buf = ST_Paused;
								}
								else if ( di->status == STATUS_QUEUED )
								{
									buf = ST_Queued;
								}
								else if ( di->status == STATUS_COMPLETED )
								{
									if ( di->last_downloaded == 0 )
									{
										__snwprintf( buf, 128, L"%s - 100.0%%", ST_Completed );
									}
									else
									{
										buf = ST_Completed;
									}
								}
								else if ( di->status == STATUS_STOPPED )
								{
									buf = ST_Stopped;
								}
								else if ( di->status == STATUS_TIMED_OUT )
								{
									buf = ST_Timed_Out;
								}
								else if ( di->status == STATUS_FAILED )
								{
									buf = ST_Failed;
								}
								else if ( di->status == STATUS_FILE_IO_ERROR )
								{
									buf = ST_File_IO_Error;
								}
								else if ( di->status == STATUS_SKIPPED )
								{
									buf = ST_Skipped;
								}
								else if ( di->status == STATUS_AUTH_REQUIRED )
								{
									buf = ST_Authorization_Required;
								}
								else if ( di->status == STATUS_PROXY_AUTH_REQUIRED )
								{
									buf = ST_Proxy_Authentication_Required;
								}
								else if ( di->status == STATUS_ALLOCATING_FILE )
								{
									buf = ST_Allocating_File;
								}
								else	// Downloading.
								{
									buf = L"\x221E\0";	// Infinity symbol.
								}
							}
							break;

							case 10:	// TIME ELAPSED
							case 11:	// TIME REMAINING
							{
								DT_ALIGN = DT_RIGHT;

								// Use the infinity symbol for remaining time if it can't be calculated.
								if ( arr2[ i ] == 11 &&
								   ( di->status == STATUS_CONNECTING ||
									 di->status == STATUS_PAUSED ||
								   ( di->status == STATUS_DOWNLOADING && ( di->file_size == 0 || di->speed == 0 ) ) ) )
								{
									buf = L"\x221E\0";	// Infinity symbol.
								}
								else
								{
									unsigned long long time_length = ( arr2[ i ] == 10 ? di->time_elapsed : di->time_remaining );

									if ( di->status != STATUS_DOWNLOADING && di->status != STATUS_PAUSED && time_length == 0 )
									{
										buf = L"";
									}
									else
									{
										buf = tbuf;	// Reset the buffer pointer.

										if ( time_length < 60 )	// Less than 1 minute.
										{
											__snwprintf( buf, 128, L"%llus", time_length );
										}
										else if ( time_length < 3600 )	// Less than 1 hour.
										{
											__snwprintf( buf, 128, L"%llum%02llus", time_length / 60, time_length % 60 );
										}
										else if ( time_length < 86400 )	// Less than 1 day.
										{
											__snwprintf( buf, 128, L"%lluh%02llum%02llus", time_length / 3600, ( time_length / 60 ) % 60, time_length % 60 );
										}
										else	// More than 1 day.
										{
											__snwprintf( buf, 128, L"%llud%02lluh%02llum%02llus", time_length / 86400, ( time_length / 3600 ) % 24, ( time_length / 60 ) % 60, time_length % 60 );
										}
									}
								}
							}
							break;

							case 12:	// TLS/SSL Version
							{
								DT_ALIGN = DT_LEFT;

								switch ( di->ssl_version )
								{
									case 0: { buf = ST_SSL_2_0; } break;
									case 1: { buf = ST_SSL_3_0; } break;
									case 2: { buf = ST_TLS_1_0; } break;
									case 3: { buf = ST_TLS_1_1; } break;
									case 4: { buf = ST_TLS_1_2; } break;
									default: { buf = L""; } break;
								}
							}
							break;

							case 13:	// URL
							{
								DT_ALIGN = DT_LEFT;

								buf = di->url;
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
						last_rc.bottom -= 2;
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
					HFONT ohf = ( HFONT )_SelectObject( hdcMem, hFont );
					_DeleteObject( ohf );

					// Transparent background for text.
					_SetBkMode( hdcMem, TRANSPARENT );

					if ( arr2[ i ] == 7 )	// File Type
					{
						if ( di->icon != NULL )
						{
							_DrawIconEx( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, *di->icon, 0, 0, NULL, NULL, DI_NORMAL );
						}
					}
					else if ( arr2[ i ] == 9 )	// Progress
					{
						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, WHITENESS );

						RECT rc_clip = rc;

						if ( di->status == STATUS_CONNECTING ||
							 di->status == STATUS_DOWNLOADING ||
							 di->status == STATUS_PAUSED ||
							 di->status == STATUS_QUEUED ||
							 di->status == STATUS_STOPPED )
						{
							if ( di->file_size > 0 )
							{
								// Multiply the floating point division by 100%.
								float f_percentage = ( float )width * ( ( float )di->last_downloaded / ( float )di->file_size );
								int i_percentage = 0;
								_asm
								{
									fld f_percentage;	//; Load the floating point value onto the FPU stack.
									fistp i_percentage;	//; Convert the floating point value into an integer, store it in an integer, and then pop it off the stack.
								}

								rc_clip.right = i_percentage;
							}
							else
							{
								rc_clip.right = 0;
							}
						}

						// Fill the background.
						HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						color = _CreateSolidBrush( ( COLORREF )RGB( 0x00, 0x00, 0x00 ) );
						_FillRect( hdcMem, &rc_clip, color );
						_DeleteObject( color );

						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCINVERT );

						color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
						_FillRect( hdcMem, &rc, color );
						_DeleteObject( color );

						_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
						_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCINVERT );

						COLORREF color_ref_body = 0, color_ref_border = 0;
						switch ( di->status )
						{
							case STATUS_FAILED:
							case STATUS_FILE_IO_ERROR:			{ color_ref_body = RGB( 0xFF, 0x80, 0x80 ); color_ref_border = RGB( 0xFF, 0x20, 0x20 ); } break;
							case STATUS_TIMED_OUT:				{ color_ref_body = RGB( 0xFF, 0xB0, 0x00 ); color_ref_border = RGB( 0xFF, 0x50, 0x00 ); } break;
							case STATUS_SKIPPED:				{ color_ref_body = RGB( 0x80, 0x80, 0xA0 ); color_ref_border = RGB( 0x40, 0x40, 0x80 ); } break;
							case STATUS_AUTH_REQUIRED:
							case STATUS_PROXY_AUTH_REQUIRED:	{ color_ref_body = RGB( 0xFF, 0x80, 0xFF ); color_ref_border = RGB( 0xA0, 0x40, 0xA0 ); } break;
							default:
							{
								if ( di->status == STATUS_ALLOCATING_FILE )
								{
									color_ref_body = RGB( 0x40, 0xC0, 0x40 ); color_ref_border = RGB( 0x00, 0x80, 0x00 );
								}
								else
								{
									color_ref_body = RGB( 0xA0, 0xA0, 0xFF ); color_ref_border = RGB( 0x40, 0x40, 0xFF );
								}
							}
							break;
						}

						color = _CreateSolidBrush( color_ref_body );
						_FillRect( hdcMem, &rc_clip, color );
						_DeleteObject( color );

						_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCPAINT );

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
							HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_HIGHLIGHT ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							// White text.
							_SetTextColor( hdcMem, RGB( 0xFF, 0xFF, 0xFF ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCCOPY );
						}
						else
						{
							// Fill the background.
							HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
							_FillRect( hdcMem, &rc, color );
							_DeleteObject( color );

							_SetTextColor( hdcMem, RGB( 0x00, 0x00, 0x00 ) );
							_DrawTextW( hdcMem, buf, -1, &rc, DT_NOPREFIX | DT_SINGLELINE | DT_ALIGN | DT_VCENTER | DT_END_ELLIPSIS );
							_BitBlt( dis->hDC, dis->rcItem.left + last_rc.left, last_rc.top, width, height, hdcMem, 0, 0, SRCAND );
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

					// Show our edit context menu as a popup.
					POINT p;
					_GetCursorPos( &p ) ;
					_TrackPopupMenu( g_hMenuSub_tray, 0, p.x, p.y, 0, hWnd, NULL );
				}
				break;
			}
		}
		break;

		case WM_PROPAGATE:
		{
			if ( g_hWnd_add_urls == NULL )
			{
				g_hWnd_add_urls = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"add_urls", ST_Add_URL_s_, WS_OVERLAPPEDWINDOW, ( ( _GetSystemMetrics( SM_CXSCREEN ) - 505 ) / 2 ), ( ( _GetSystemMetrics( SM_CYSCREEN ) - 240 ) / 2 ), 505, 240, NULL, NULL, NULL, NULL );
			}
			else if ( _IsIconic( g_hWnd_add_urls ) )	// If minimized, then restore the window.
			{
				_ShowWindow( g_hWnd_add_urls, SW_RESTORE );
			}

			SendMessageW( g_hWnd_add_urls, WM_PROPAGATE, wParam, lParam );
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

			if ( cfg_enable_download_history && download_history_changed )
			{
				_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\download_history\0", 18 );
				base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

				save_download_history( base_directory );
				download_history_changed = false;
			}

			// Get the number of items in the listview.
			int num_items = _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );

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

					DeleteCriticalSection( &di->shared_cs );

					GlobalFree( di );
				}
			}

			UpdateColumnOrders();

			DestroyMenus();

			if ( cfg_tray_icon )
			{
				// Remove the icon from the notification area.
				_Shell_NotifyIconW( NIM_DELETE, &g_nid );
			}

			if ( use_drag_and_drop )
			{
				UnregisterDropWindow( g_hWnd_files, List_DropTarget );

				_OleUninitialize();
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
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
