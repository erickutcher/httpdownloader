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

#include "menus.h"
#include "utilities.h"

#include "connection.h"

#include "string_tables.h"

HMENU g_hMenu = NULL;

HMENU g_hMenuSub_edit = NULL;
HMENU g_hMenuSub_view = NULL;

HMENU g_hMenuSub_download = NULL;
HMENU g_hMenuSub_queue = NULL;
HMENU g_hMenuSub_column = NULL;
HMENU g_hMenuSub_tray = NULL;			// Handle to our tray menu.

void DestroyMenus()
{
	_DestroyMenu( g_hMenuSub_tray );
	_DestroyMenu( g_hMenuSub_download );
	_DestroyMenu( g_hMenuSub_queue );
	_DestroyMenu( g_hMenuSub_column );
}

void UpdateMenus( bool enable )
{
	if ( enable )
	{
		int item_count = _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );
		int sel_count = _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 );

		DOWNLOAD_INFO *di = NULL;

		// Retrieve the lParam value from the selected listview item.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = _SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );
			di = ( DOWNLOAD_INFO * )lvi.lParam;
		}

		if ( sel_count == 1 )
		{
			if ( di != NULL &&
				!di->simulate_download )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_DISABLED );
			}

			// Allow queue menu if item is queued.
			if ( di != NULL &&
				 di->status == STATUS_QUEUED )
			{
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_DISABLED );
			}
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_DISABLED );

			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_DISABLED );
		}

		if ( sel_count > 0 )
		{
			// Allow start if paused, queued, stopped, timed out, failed, skipped, or proxy authorization required.
			if ( di != NULL &&
			   ( di->file_size == 0 || ( di->downloaded < di->file_size ) ) &&
			   ( di->status == STATUS_PAUSED ||
			   ( di->status == STATUS_QUEUED && ( total_downloading < cfg_max_downloads ) ) ||
			   ( di->active_parts == 0 &&
			   ( di->status == STATUS_STOPPED ||
				 di->status == STATUS_TIMED_OUT ||
				 di->status == STATUS_FAILED ||
				 di->status == STATUS_SKIPPED ||
				 di->status == STATUS_PROXY_AUTH_REQUIRED ) ) ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_DISABLED );
			}

			// Allow pause if downloading.
			if ( di != NULL &&
				 di->status == STATUS_DOWNLOADING )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_DISABLED );
			}

			// Allow stop if connecting, downloading, paused, or queued.
			if ( di != NULL &&
			   ( di->status == STATUS_CONNECTING ||
			     di->status == STATUS_DOWNLOADING ||
				 di->status == STATUS_PAUSED ||
				 di->status == STATUS_QUEUED ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_DISABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_DISABLED );
			}

			// Allow remove for all statuses.
			_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_SELECTED, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_SELECTED, MF_ENABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_DISABLED );

			_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_SELECTED, MF_DISABLED );

			_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_DISABLED );

			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_SELECTED, MF_DISABLED );
		}

		if ( active_download_list != NULL )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );
			
		}
		else if ( download_queue != NULL )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_DISABLED );
		}

		if ( item_count > 0 )
		{
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_ENABLED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_ENABLED );
		}
		else
		{
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_DISABLED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_DISABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_DISABLED );
		}

		_EnableMenuItem( g_hMenuSub_download, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_DISABLED ) );

		_EnableMenuItem( g_hMenuSub_edit, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_DISABLED ) );
	}
	else
	{
		_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_SELECTED, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_download, MENU_SELECT_ALL, MF_DISABLED );

		_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_SELECTED, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_SELECT_ALL, MF_DISABLED );

		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_DISABLED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_DISABLED );
	}
}

void CreateMenus()
{
	g_hMenu = _CreateMenu();

	HMENU hMenuSub_file = _CreatePopupMenu();
	g_hMenuSub_edit = _CreatePopupMenu();
	g_hMenuSub_view = _CreatePopupMenu();
	HMENU hMenuSub_tools = _CreatePopupMenu();
	HMENU hMenuSub_help = _CreatePopupMenu();

	g_hMenuSub_download = _CreatePopupMenu();
	g_hMenuSub_queue = _CreatePopupMenu();
	g_hMenuSub_column = _CreatePopupMenu();

	g_hMenuSub_tray = _CreatePopupMenu();

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	// DOWNLOAD SUBMENU - QUEUE

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Move_To_Top;
	mii.cch = 11;
	mii.wID = MENU_QUEUE_TOP;
	_InsertMenuItemW( g_hMenuSub_queue, 0, TRUE, &mii );

	mii.dwTypeData = ST_Move_Up;
	mii.cch = 7;
	mii.wID = MENU_QUEUE_UP;
	_InsertMenuItemW( g_hMenuSub_queue, 1, TRUE, &mii );

	mii.dwTypeData = ST_Move_Down;
	mii.cch = 9;
	mii.wID = MENU_QUEUE_DOWN;
	_InsertMenuItemW( g_hMenuSub_queue, 2, TRUE, &mii );

	mii.dwTypeData = ST_Move_To_Bottom;
	mii.cch = 14;
	mii.wID = MENU_QUEUE_BOTTOM;
	_InsertMenuItemW( g_hMenuSub_queue, 3, TRUE, &mii );

	// DOWNLOAD MENU (right click)

	mii.dwTypeData = ST_Open_File;
	mii.cch = 9;
	mii.wID = MENU_OPEN_FILE;
	_InsertMenuItemW( g_hMenuSub_download, 0, TRUE, &mii );

	mii.dwTypeData = ST_Open_Directory;
	mii.cch = 14;
	mii.wID = MENU_OPEN_DIRECTORY;
	_InsertMenuItemW( g_hMenuSub_download, 1, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 2, TRUE, &mii );
	
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Start;
	mii.cch = 5;
	mii.wID = MENU_START;
	_InsertMenuItemW( g_hMenuSub_download, 3, TRUE, &mii );

	mii.dwTypeData = ST_Pause;
	mii.cch = 5;
	mii.wID = MENU_PAUSE;
	_InsertMenuItemW( g_hMenuSub_download, 4, TRUE, &mii );

	mii.dwTypeData = ST_Stop;
	mii.cch = 4;
	mii.wID = MENU_STOP;
	_InsertMenuItemW( g_hMenuSub_download, 5, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 6, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Queue;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_download, 7, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 8, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Remove_Selected;
	mii.cch = 15;
	mii.wID = MENU_REMOVE_SELECTED;
	_InsertMenuItemW( g_hMenuSub_download, 9, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 10, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Select_All;
	mii.cch = 10;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_download, 11, TRUE, &mii );

	//

	mii.dwTypeData = ST_NUM;
	mii.cch = 1;
	mii.wID = MENU_NUM;
	mii.fState = ( cfg_column_order1 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 0, TRUE, &mii );

	mii.dwTypeData = ST_Active_Parts;
	mii.cch = 12;
	mii.wID = MENU_ACTIVE_PARTS;
	mii.fState = ( cfg_column_order2 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 1, TRUE, &mii );

	mii.dwTypeData = ST_Date_and_Time_Added;
	mii.cch = 19;
	mii.wID = MENU_DATE_AND_TIME_ADDED;
	mii.fState = ( cfg_column_order3 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 2, TRUE, &mii );

	mii.dwTypeData = ST_Download_Directory;
	mii.cch = 18;
	mii.wID = MENU_DOWNLOAD_DIRECTORY;
	mii.fState = ( cfg_column_order4 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 3, TRUE, &mii );

	mii.dwTypeData = ST_Download_Speed;
	mii.cch = 10;
	mii.wID = MENU_DOWNLOAD_SPEED;
	mii.fState = ( cfg_column_order5 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 4, TRUE, &mii );

	mii.dwTypeData = ST_Downloaded;
	mii.cch = 10;
	mii.wID = MENU_DOWNLOADED;
	mii.fState = ( cfg_column_order6 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 5, TRUE, &mii );

	mii.dwTypeData = ST_File_Size;
	mii.cch = 9;
	mii.wID = MENU_FILE_SIZE;
	mii.fState = ( cfg_column_order7 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 6, TRUE, &mii );

	mii.dwTypeData = ST_File_Type;
	mii.cch = 9;
	mii.wID = MENU_FILE_TYPE;
	mii.fState = ( cfg_column_order8 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 7, TRUE, &mii );

	mii.dwTypeData = ST_Filename;
	mii.cch = 8;
	mii.wID = MENU_FILENAME;
	mii.fState = ( cfg_column_order9 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 8, TRUE, &mii );

	mii.dwTypeData = ST_Progress;
	mii.cch = 8;
	mii.wID = MENU_PROGRESS;
	mii.fState = ( cfg_column_order10 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 9, TRUE, &mii );

	mii.dwTypeData = ST_Time_Elapsed;
	mii.cch = 12;
	mii.wID = MENU_TIME_ELAPSED;
	mii.fState = ( cfg_column_order11 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 10, TRUE, &mii );

	mii.dwTypeData = ST_Time_Remaining;
	mii.cch = 14;
	mii.wID = MENU_TIME_REMAINING;
	mii.fState = ( cfg_column_order12 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 11, TRUE, &mii );

	mii.dwTypeData = ST_TLS___SSL_Version;
	mii.cch = 15;
	mii.wID = MENU_TLS_SSL_VERSION;
	mii.fState = ( cfg_column_order13 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 12, TRUE, &mii );

	mii.dwTypeData = ST_URL;
	mii.cch = 3;
	mii.wID = MENU_URL;
	mii.fState = ( cfg_column_order14 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 13, TRUE, &mii );

	//

	// TRAY MENU (for right click)
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Open_Download_List;
	mii.cch = 17;
	mii.wID = MENU_RESTORE;
	mii.fState = MFS_DEFAULT | MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_tray, 0, TRUE, &mii );

	mii.fState = MFS_ENABLED;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Options___;
	mii.cch = 10;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_tray, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Add_URL_s____;
	mii.cch = 13;
	mii.wID = MENU_ADD_URLS;
	_InsertMenuItemW( g_hMenuSub_tray, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Exit;
	mii.cch = 4;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_tray, 6, TRUE, &mii );

	//

	// FILE MENU
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Add_URL_s____;
	mii.cch = 14;
	mii.wID = MENU_ADD_URLS;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_E_xit;
	mii.cch = 5;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( hMenuSub_file, 2, TRUE, &mii );


	// EDIT MENU
	mii.dwTypeData = ST_St_art;
	mii.cch = 6;
	mii.wID = MENU_START;
	_InsertMenuItemW( g_hMenuSub_edit, 0, TRUE, &mii );

	mii.dwTypeData = ST__Pause;
	mii.cch = 7;
	mii.wID = MENU_PAUSE;
	_InsertMenuItemW( g_hMenuSub_edit, 1, TRUE, &mii );

	mii.dwTypeData = ST_Pause_Active;
	mii.cch = 12;
	mii.wID = MENU_PAUSE_ACTIVE;
	_InsertMenuItemW( g_hMenuSub_edit, 2, TRUE, &mii );

	mii.dwTypeData = ST_St_op;
	mii.cch = 5;
	mii.wID = MENU_STOP;
	_InsertMenuItemW( g_hMenuSub_edit, 3, TRUE, &mii );

	mii.dwTypeData = ST_Stop_All;
	mii.cch = 8;
	mii.wID = MENU_STOP_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 5, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Queue;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_edit, 6, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 7, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_Remove_Completed;
	mii.cch = 16;
	mii.wID = MENU_REMOVE_COMPLETED;
	_InsertMenuItemW( g_hMenuSub_edit, 8, TRUE, &mii );

	mii.dwTypeData = ST__Remove_Selected;
	mii.cch = 16;
	mii.wID = MENU_REMOVE_SELECTED;
	_InsertMenuItemW( g_hMenuSub_edit, 9, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 10, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST__Select_All;
	mii.cch = 11;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 11, TRUE, &mii );


	// VIEW MENU
	mii.dwTypeData = ST__Status_Bar;
	mii.cch = 11;
	mii.wID = MENU_SHOW_STATUS_BAR;
	mii.fState = ( cfg_show_status_bar ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 0, TRUE, &mii );


	// TOOLS MENU
	mii.dwTypeData = ST__Options___;
	mii.cch = 11;
	mii.wID = MENU_OPTIONS;
	mii.fState = 0;
	_InsertMenuItemW( hMenuSub_tools, 0, TRUE, &mii );


	// HELP MENU
	mii.dwTypeData = ST__About;
	mii.cch = 6;
	mii.wID = MENU_ABOUT;
	_InsertMenuItemW( hMenuSub_help, 0, TRUE, &mii );


	// MENU BAR
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST__File;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_file;
	_InsertMenuItemW( g_hMenu, 0, TRUE, &mii );

	mii.dwTypeData = ST__Edit;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_edit;
	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	mii.dwTypeData = ST__View;
	mii.cch = 5;
	mii.hSubMenu = g_hMenuSub_view;
	_InsertMenuItemW( g_hMenu, 2, TRUE, &mii );

	mii.dwTypeData = ST__Tools;
	mii.cch = 6;
	mii.hSubMenu = hMenuSub_tools;
	_InsertMenuItemW( g_hMenu, 3, TRUE, &mii );

	mii.dwTypeData = ST__Help;
	mii.cch = 5;
	mii.hSubMenu = hMenuSub_help;
	_InsertMenuItemW( g_hMenu, 4, TRUE, &mii );
}

void UpdateColumns( unsigned int menu_id )
{
	int arr[ NUM_COLUMNS ];
	int offset = 0;
	int index = 0;
	unsigned char menu_index = menu_id - COLUMN_MENU_OFFSET;

	if ( menu_index >= 0 && menu_index <= NUM_COLUMNS )
	{
		_SendMessageW( g_hWnd_files, LVM_GETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );
		for ( int i = 0; i < NUM_COLUMNS; ++i )
		{
			if ( *download_columns[ i ] != -1 )
			{
				*download_columns[ i ] = ( char )arr[ offset++ ];
			}
		}

		if ( *download_columns[ menu_index ] != -1 )
		{
			_CheckMenuItem( g_hMenuSub_column, menu_id, MF_UNCHECKED );

			--g_total_columns;

			switch ( menu_index )
			{
				case 0:
				{
					index = 0;
				}
				break;

				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				{
					index = GetColumnIndexFromVirtualIndex( *download_columns[ menu_index ], download_columns, NUM_COLUMNS );
				}
				break;

				case 13:
				{
					index = g_total_columns;
				}
				break;
			}

			*download_columns[ menu_index ] = -1;
			_SendMessageW( g_hWnd_files, LVM_DELETECOLUMN, index, 0 );
			
			if ( g_total_columns == 1 )
			{
				// Find the remaining menu item and disable it.
				for ( int j = 0; j < NUM_COLUMNS; ++j )
				{
					if ( *download_columns[ j ] != -1 )
					{
						_EnableMenuItem( g_hMenuSub_column, COLUMN_MENU_OFFSET + j, MF_DISABLED );

						break;
					}
				}
			}
		}
		else
		{
			if ( g_total_columns == 1 )
			{
				// Find the remaining menu item and disable it.
				for ( int j = 0; j < NUM_COLUMNS; ++j )
				{
					if ( *download_columns[ j ] != -1 )
					{
						_EnableMenuItem( g_hMenuSub_column, COLUMN_MENU_OFFSET + j, MF_ENABLED );

						break;
					}
				}
			}

			_CheckMenuItem( g_hMenuSub_column, menu_id, MF_CHECKED );

			switch ( menu_index )
			{
				case 0:
				{
					*download_columns[ menu_index ] = 0;

					// Update the virtual indices.
					for ( int j = 1; j < NUM_COLUMNS; ++j )
					{
						if ( *download_columns[ j ] != -1 )
						{
							++( *( download_columns[ j ] ) );
						}
					}
				}
				break;

				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 12:
				{
					*download_columns[ menu_index ] = g_total_columns;
					index = GetColumnIndexFromVirtualIndex( *download_columns[ menu_index ], download_columns, NUM_COLUMNS );
				}
				break;

				case 13:
				{
					*download_columns[ menu_index ] = g_total_columns;
					index = g_total_columns;
				}
				break;
			}

			LVCOLUMN lvc;
			_memzero( &lvc, sizeof( LVCOLUMN ) );
			lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_ORDER;
			lvc.iOrder = *download_columns[ menu_index ];
			lvc.pszText = download_string_table[ menu_index ];
			lvc.cx = *download_columns_width[ menu_index ];
			_SendMessageW( g_hWnd_files, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

			++g_total_columns;
		}
	}
}
