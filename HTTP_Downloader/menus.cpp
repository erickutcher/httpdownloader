/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
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
HMENU g_hMenuSub_drag_drop = NULL;

void DestroyMenus()
{
	_DestroyMenu( g_hMenuSub_drag_drop );
	_DestroyMenu( g_hMenuSub_tray );
	_DestroyMenu( g_hMenuSub_download );
	_DestroyMenu( g_hMenuSub_queue );
	_DestroyMenu( g_hMenuSub_column );
}

void UpdateMenus( bool enable )
{
	TBBUTTONINFO tbb;
	_memzero( &tbb, sizeof( TBBUTTONINFO ) );
	tbb.cbSize = sizeof( TBBUTTONINFO );
	tbb.dwMask = TBIF_STATE;

	if ( enable )
	{
		int item_count = ( int )_SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );
		int sel_count = ( int )_SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 );

		DOWNLOAD_INFO *di = NULL;

		// Retrieve the lParam value from the selected listview item.
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = ( int )_SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		// See if something is at least highlighted.
		if ( lvi.iItem == -1 )
		{
			lvi.iItem = ( int )_SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_SELECTED );
		}

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );
			di = ( DOWNLOAD_INFO * )lvi.lParam;
		}

		if ( sel_count == 1 )
		{
			if ( di != NULL &&
				!( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_GRAYED );
			}

			// Allow download update if any of the following. Includes paused and queued.
			if ( di != NULL &&
				 IS_STATUS( di->status,
					STATUS_CONNECTING |
					STATUS_DOWNLOADING |
					STATUS_COMPLETED |
					STATUS_STOPPED |
					STATUS_TIMED_OUT |
					STATUS_FAILED |
					STATUS_SKIPPED |
					STATUS_AUTH_REQUIRED |
					STATUS_PROXY_AUTH_REQUIRED ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_UPDATE_DOWNLOAD, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_UPDATE_DOWNLOAD, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_UPDATE_DOWNLOAD, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_UPDATE_DOWNLOAD, MF_GRAYED );
			}

			// Allow queue menu if item is queued.
			if ( di != NULL &&
				 IS_STATUS( di->status, STATUS_QUEUED ) )
			{
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_GRAYED );
			}

			// Make sure our Filename column is visible.
			if ( *download_columns[ COLUMN_FILENAME ] != -1 )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RENAME, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RENAME, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RENAME, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RENAME, MF_GRAYED );
			}
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_download, MENU_UPDATE_DOWNLOAD, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_UPDATE_DOWNLOAD, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_download, MENU_RENAME, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_RENAME, MF_GRAYED );
		}

		if ( sel_count > 0 )
		{
			// Allow start if paused, queued, stopped, timed out, failed, file IO error, skipped, or proxy authorization required.
			if ( di != NULL &&
			   ( di->file_size == 0 || ( di->downloaded < di->file_size ) ) &&
			   ( IS_STATUS( di->status, STATUS_PAUSED ) ||
			   ( IS_STATUS( di->status, STATUS_QUEUED ) && ( total_downloading < cfg_max_downloads ) ) ||
			   ( di->active_parts == 0 &&
				 IS_STATUS( di->status,
					STATUS_STOPPED |
					STATUS_TIMED_OUT |
					STATUS_FAILED |
					STATUS_FILE_IO_ERROR |
					STATUS_SKIPPED |
					STATUS_AUTH_REQUIRED |
					STATUS_PROXY_AUTH_REQUIRED ) ) ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
			}

			// Allow pause if downloading.
			// Make sure the status is exclusively connecting or downloading.
			if ( di != NULL &&
			   ( di->status == STATUS_CONNECTING ||
				 di->status == STATUS_DOWNLOADING ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE, ( LPARAM )&tbb );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE, ( LPARAM )&tbb );
			}

			// Allow stop if connecting, downloading, moving file, paused, or queued.
			if ( di != NULL &&
				 IS_STATUS( di->status,
					STATUS_CONNECTING |
					STATUS_DOWNLOADING |
					STATUS_RESTART |
					STATUS_MOVING_FILE ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP, ( LPARAM )&tbb );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP, ( LPARAM )&tbb );
			}

			// Allow download restart if any of the following. Includes paused and queued.
			if ( di != NULL &&
				 IS_STATUS_NOT( di->status, STATUS_RESTART ) &&
				 IS_STATUS( di->status,
					STATUS_CONNECTING |
					STATUS_DOWNLOADING |
					STATUS_COMPLETED |
					STATUS_STOPPED |
					STATUS_TIMED_OUT |
					STATUS_FAILED |
					STATUS_FILE_IO_ERROR |
					STATUS_SKIPPED |
					STATUS_AUTH_REQUIRED |
					STATUS_PROXY_AUTH_REQUIRED ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_RESTART, ( LPARAM )&tbb );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
				_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_RESTART, ( LPARAM )&tbb );
			}

			// Allow remove for all statuses.
			_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE, MF_ENABLED );

			if ( di != NULL &&
				!( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_AND_DELETE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_AND_DELETE, MF_ENABLED );

				_EnableMenuItem( g_hMenuSub_download, MENU_DELETE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_DELETE, MF_ENABLED );
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_AND_DELETE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_AND_DELETE, MF_GRAYED );

				_EnableMenuItem( g_hMenuSub_download, MENU_DELETE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_DELETE, MF_GRAYED );
			}

			// Allow the URL copy.
			_EnableMenuItem( g_hMenuSub_download, MENU_COPY_URLS, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_COPY_URLS, MF_ENABLED );

			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_REMOVE, ( LPARAM )&tbb );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_AND_DELETE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_download, MENU_DELETE, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_download, MENU_COPY_URLS, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_AND_DELETE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_DELETE, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_edit, MENU_COPY_URLS, MF_GRAYED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_REMOVE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_RESTART, ( LPARAM )&tbb );
		}

		if ( active_download_list != NULL )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );

			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}
		else if ( download_queue != NULL )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_GRAYED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}

		if ( item_count > 0 )
		{
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_ENABLED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_ENABLED );
		}
		else
		{
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_GRAYED );
		}

		_EnableMenuItem( g_hMenuSub_download, MENU_ADD_URLS, MF_ENABLED );

		_EnableMenuItem( g_hMenuSub_download, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_GRAYED ) );

		_EnableMenuItem( g_hMenuSub_edit, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_GRAYED ) );
	}
	else
	{
		_EnableMenuItem( g_hMenuSub_download, MENU_ADD_URLS, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_FILE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_OPEN_DIRECTORY, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_START, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_PAUSE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_STOP, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_UPDATE_DOWNLOAD, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE_AND_DELETE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_DELETE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_RENAME, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_COPY_URLS, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_download, MENU_SELECT_ALL, MF_GRAYED );

		_EnableMenuItem( g_hMenuSub_edit, MENU_START, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_STOP, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_UPDATE_DOWNLOAD, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_AND_DELETE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_DELETE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_RENAME, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_COPY_URLS, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_edit, MENU_SELECT_ALL, MF_GRAYED );

		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_TOP, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_UP, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_DOWN, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_queue, MENU_QUEUE_BOTTOM, MF_GRAYED );

		tbb.fsState = TBSTATE_INDETERMINATE;
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_REMOVE, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_RESTART, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
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
	g_hMenuSub_drag_drop = _CreatePopupMenu();

	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;

	// DOWNLOAD SUBMENU - QUEUE

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Move_to_Top;
	mii.cch = ST_L_Move_to_Top;
	mii.wID = MENU_QUEUE_TOP;
	_InsertMenuItemW( g_hMenuSub_queue, 0, TRUE, &mii );

	mii.dwTypeData = ST_V_Move_Up;
	mii.cch = ST_L_Move_Up;
	mii.wID = MENU_QUEUE_UP;
	_InsertMenuItemW( g_hMenuSub_queue, 1, TRUE, &mii );

	mii.dwTypeData = ST_V_Move_Down;
	mii.cch = ST_L_Move_Down;
	mii.wID = MENU_QUEUE_DOWN;
	_InsertMenuItemW( g_hMenuSub_queue, 2, TRUE, &mii );

	mii.dwTypeData = ST_V_Move_to_Bottom;
	mii.cch = ST_L_Move_to_Bottom;
	mii.wID = MENU_QUEUE_BOTTOM;
	_InsertMenuItemW( g_hMenuSub_queue, 3, TRUE, &mii );

	// DOWNLOAD MENU (right click)

	mii.dwTypeData = ST_V_Add_URL_s____;
	mii.cch = ST_L_Add_URL_s____;
	mii.wID = MENU_ADD_URLS;
	_InsertMenuItemW( g_hMenuSub_download, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Open_File;
	mii.cch = ST_L_Open_File;
	mii.wID = MENU_OPEN_FILE;
	_InsertMenuItemW( g_hMenuSub_download, 2, TRUE, &mii );

	mii.dwTypeData = ST_V_Open_Directory;
	mii.cch = ST_L_Open_Directory;
	mii.wID = MENU_OPEN_DIRECTORY;
	_InsertMenuItemW( g_hMenuSub_download, 3, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 4, TRUE, &mii );
	
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Start;
	mii.cch = ST_L_Start;
	mii.wID = MENU_START;
	_InsertMenuItemW( g_hMenuSub_download, 5, TRUE, &mii );

	mii.dwTypeData = ST_V_Pause;
	mii.cch = ST_L_Pause;
	mii.wID = MENU_PAUSE;
	_InsertMenuItemW( g_hMenuSub_download, 6, TRUE, &mii );

	mii.dwTypeData = ST_V_Stop;
	mii.cch = ST_L_Stop;
	mii.wID = MENU_STOP;
	_InsertMenuItemW( g_hMenuSub_download, 7, TRUE, &mii );

	mii.dwTypeData = ST_V_Restart;
	mii.cch = ST_L_Restart;
	mii.wID = MENU_RESTART;
	_InsertMenuItemW( g_hMenuSub_download, 8, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 9, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Update_Download___;
	mii.cch = ST_L_Update_Download___;
	mii.wID = MENU_UPDATE_DOWNLOAD;
	_InsertMenuItemW( g_hMenuSub_download, 10, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 11, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Queue;
	mii.cch = ST_L_Queue;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_download, 12, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 13, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Remove;
	mii.cch = ST_L_Remove;
	mii.wID = MENU_REMOVE;
	_InsertMenuItemW( g_hMenuSub_download, 14, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_and_Delete;
	mii.cch = ST_L_Remove_and_Delete;
	mii.wID = MENU_REMOVE_AND_DELETE;
	_InsertMenuItemW( g_hMenuSub_download, 15, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 16, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Delete;
	mii.cch = ST_L_Delete;
	mii.wID = MENU_DELETE;
	_InsertMenuItemW( g_hMenuSub_download, 17, TRUE, &mii );

	mii.dwTypeData = ST_V_Rename;
	mii.cch = ST_L_Rename;
	mii.wID = MENU_RENAME;
	_InsertMenuItemW( g_hMenuSub_download, 18, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 19, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Copy_URL_s_;
	mii.cch = ST_L_Copy_URL_s_;
	mii.wID = MENU_COPY_URLS;
	_InsertMenuItemW( g_hMenuSub_download, 20, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 21, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Select_All;
	mii.cch = ST_L_Select_All;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_download, 22, TRUE, &mii );

	//

	mii.dwTypeData = ST_V_NUM;
	mii.cch = ST_L_NUM;
	mii.wID = MENU_NUM;
	mii.fState = ( cfg_column_order1 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 0, TRUE, &mii );

	mii.dwTypeData = ST_V_Active_Parts;
	mii.cch = ST_L_Active_Parts;
	mii.wID = MENU_ACTIVE_PARTS;
	mii.fState = ( cfg_column_order2 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 1, TRUE, &mii );

	mii.dwTypeData = ST_V_Date_and_Time_Added;
	mii.cch = ST_L_Date_and_Time_Added;
	mii.wID = MENU_DATE_AND_TIME_ADDED;
	mii.fState = ( cfg_column_order3 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 2, TRUE, &mii );

	mii.dwTypeData = ST_V_Download_Directory;
	mii.cch = ST_L_Download_Directory;
	mii.wID = MENU_DOWNLOAD_DIRECTORY;
	mii.fState = ( cfg_column_order4 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 3, TRUE, &mii );

	mii.dwTypeData = ST_V_Download_Speed;
	mii.cch = ST_L_Download_Speed;
	mii.wID = MENU_DOWNLOAD_SPEED;
	mii.fState = ( cfg_column_order5 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 4, TRUE, &mii );

	mii.dwTypeData = ST_V_Download_Speed_Limit;
	mii.cch = ST_L_Download_Speed_Limit;
	mii.wID = MENU_DOWNLOAD_SPEED_LIMIT;
	mii.fState = ( cfg_column_order6 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 5, TRUE, &mii );

	mii.dwTypeData = ST_V_Downloaded;
	mii.cch = ST_L_Downloaded;
	mii.wID = MENU_DOWNLOADED;
	mii.fState = ( cfg_column_order7 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 6, TRUE, &mii );

	mii.dwTypeData = ST_V_File_Size;
	mii.cch = ST_L_File_Size;
	mii.wID = MENU_FILE_SIZE;
	mii.fState = ( cfg_column_order8 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 7, TRUE, &mii );

	mii.dwTypeData = ST_V_File_Type;
	mii.cch = ST_L_File_Type;
	mii.wID = MENU_FILE_TYPE;
	mii.fState = ( cfg_column_order9 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 8, TRUE, &mii );

	mii.dwTypeData = ST_V_Filename;
	mii.cch = ST_L_Filename;
	mii.wID = MENU_FILENAME;
	mii.fState = ( cfg_column_order10 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 9, TRUE, &mii );

	mii.dwTypeData = ST_V_Progress;
	mii.cch = ST_L_Progress;
	mii.wID = MENU_PROGRESS;
	mii.fState = ( cfg_column_order11 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 10, TRUE, &mii );

	mii.dwTypeData = ST_V_SSL___TLS_Version;
	mii.cch = ST_L_SSL___TLS_Version;
	mii.wID = MENU_SSL_TLS_VERSION;
	mii.fState = ( cfg_column_order12 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 11, TRUE, &mii );

	mii.dwTypeData = ST_V_Time_Elapsed;
	mii.cch = ST_L_Time_Elapsed;
	mii.wID = MENU_TIME_ELAPSED;
	mii.fState = ( cfg_column_order13 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 12, TRUE, &mii );

	mii.dwTypeData = ST_V_Time_Remaining;
	mii.cch = ST_L_Time_Remaining;
	mii.wID = MENU_TIME_REMAINING;
	mii.fState = ( cfg_column_order14 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 13, TRUE, &mii );

	mii.dwTypeData = ST_V_URL;
	mii.cch = ST_L_URL;
	mii.wID = MENU_URL;
	mii.fState = ( cfg_column_order15 != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_column, 14, TRUE, &mii );

	//

	// TRAY MENU (for right click)
	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Open_Download_List;
	mii.cch = ST_L_Open_Download_List;
	mii.wID = MENU_RESTORE;
	mii.fState = MFS_DEFAULT | MFS_ENABLED;
	_InsertMenuItemW( g_hMenuSub_tray, 0, TRUE, &mii );

	mii.fState = MFS_ENABLED;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Add_URL_s____;
	mii.cch = ST_L_Add_URL_s____;
	mii.wID = MENU_ADD_URLS;
	_InsertMenuItemW( g_hMenuSub_tray, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Options___;
	mii.cch = ST_L_Options___;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_tray, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Exit;
	mii.cch = ST_L_Exit;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_tray, 6, TRUE, &mii );

	//

	// DRAG AND DROP MENU (for right click)
	mii.dwTypeData = ST_V_Always_on_Top;
	mii.cch = ST_L_Always_on_Top;
	mii.wID = MENU_ALWAYS_ON_TOP;
	mii.fState = MFS_CHECKED;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 0, TRUE, &mii );

	mii.fState = MFS_ENABLED;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Open_Download_List;
	mii.cch = ST_L_Open_Download_List;
	mii.wID = MENU_RESTORE;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Add_URL_s____;
	mii.cch = ST_L_Add_URL_s____;
	mii.wID = MENU_ADD_URLS;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Options___;
	mii.cch = ST_L_Options___;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 6, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 7, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Exit;
	mii.cch = ST_L_Exit;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 8, TRUE, &mii );

	//

	// FILE MENU
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Add_URL_s____;
	mii.cch = ST_L__Add_URL_s____;
	mii.wID = MENU_ADD_URLS;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Save_Download_History___;
	mii.cch = ST_L__Save_Download_History___;
	mii.wID = MENU_SAVE_DOWNLOAD_HISTORY;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Import_Download_History___;
	mii.cch = ST_L__Import_Download_History___;
	mii.wID = MENU_IMPORT_DOWNLOAD_HISTORY;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 4, TRUE, &mii );

	mii.dwTypeData = ST_V__Export_Download_History___;
	mii.cch = ST_L__Export_Download_History___;
	mii.wID = MENU_EXPORT_DOWNLOAD_HISTORY;
	mii.fState = MFS_ENABLED;
	_InsertMenuItemW( hMenuSub_file, 5, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_file, 6, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_E_xit;
	mii.cch = ST_L_E_xit;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( hMenuSub_file, 7, TRUE, &mii );


	// EDIT MENU
	mii.dwTypeData = ST_V_St_art;
	mii.cch = ST_L_St_art;
	mii.wID = MENU_START;
	_InsertMenuItemW( g_hMenuSub_edit, 0, TRUE, &mii );

	mii.dwTypeData = ST_V__Pause;
	mii.cch = ST_L__Pause;
	mii.wID = MENU_PAUSE;
	_InsertMenuItemW( g_hMenuSub_edit, 1, TRUE, &mii );

	mii.dwTypeData = ST_V_St_op;
	mii.cch = ST_L_St_op;
	mii.wID = MENU_STOP;
	_InsertMenuItemW( g_hMenuSub_edit, 2, TRUE, &mii );

	mii.dwTypeData = ST_V_Restart;
	mii.cch = ST_L_Restart;
	mii.wID = MENU_RESTART;
	_InsertMenuItemW( g_hMenuSub_edit, 3, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 4, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Pause_Active;
	mii.cch = ST_L_Pause_Active;
	mii.wID = MENU_PAUSE_ACTIVE;
	_InsertMenuItemW( g_hMenuSub_edit, 5, TRUE, &mii );

	mii.dwTypeData = ST_V_Stop_All;
	mii.cch = ST_L_Stop_All;
	mii.wID = MENU_STOP_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 6, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 7, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Update_Download___;
	mii.cch = ST_L_Update_Download___;
	mii.wID = MENU_UPDATE_DOWNLOAD;
	_InsertMenuItemW( g_hMenuSub_edit, 8, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 9, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Queue;
	mii.cch = ST_L_Queue;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_edit, 10, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 11, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Remove_;
	mii.cch = ST_L__Remove_;
	mii.wID = MENU_REMOVE;
	_InsertMenuItemW( g_hMenuSub_edit, 12, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_Completed;
	mii.cch = ST_L_Remove_Completed;
	mii.wID = MENU_REMOVE_COMPLETED;
	_InsertMenuItemW( g_hMenuSub_edit, 13, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_and_Delete_;
	mii.cch = ST_L_Remove_and_Delete_;
	mii.wID = MENU_REMOVE_AND_DELETE;
	_InsertMenuItemW( g_hMenuSub_edit, 14, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 15, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Delete_;
	mii.cch = ST_L__Delete_;
	mii.wID = MENU_DELETE;
	_InsertMenuItemW( g_hMenuSub_edit, 16, TRUE, &mii );

	mii.dwTypeData = ST_V_Rename_;
	mii.cch = ST_L_Rename_;
	mii.wID = MENU_RENAME;
	_InsertMenuItemW( g_hMenuSub_edit, 17, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 18, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Copy_URL_s_;
	mii.cch = ST_L__Copy_URL_s_;
	mii.wID = MENU_COPY_URLS;
	_InsertMenuItemW( g_hMenuSub_edit, 19, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 20, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Select_All_;
	mii.cch = ST_L__Select_All_;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 21, TRUE, &mii );


	// VIEW MENU
	mii.dwTypeData = ST_V__Toolbar;
	mii.cch = ST_L__Toolbar;
	mii.wID = MENU_SHOW_TOOLBAR;
	mii.fState = ( cfg_show_toolbar ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 0, TRUE, &mii );

	mii.dwTypeData = ST_V__Column_Headers;
	mii.cch = ST_L__Column_Headers;
	mii.wID = MENU_SHOW_COLUMN_HEADERS;
	mii.fState = ( cfg_show_column_headers ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 1, TRUE, &mii );

	mii.dwTypeData = ST_V__Status_Bar;
	mii.cch = ST_L__Status_Bar;
	mii.wID = MENU_SHOW_STATUS_BAR;
	mii.fState = ( cfg_show_status_bar ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 2, TRUE, &mii );


	// TOOLS MENU
	mii.dwTypeData = ST_V__Search____;
	mii.cch = ST_L__Search____;
	mii.wID = MENU_SEARCH;
	mii.fState = 0;
	_InsertMenuItemW( hMenuSub_tools, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Global_Download_Speed__Limit___;
	mii.cch = ST_L_Global_Download_Speed__Limit___;
	mii.wID = MENU_GLOBAL_SPEED_LIMIT;
	_InsertMenuItemW( hMenuSub_tools, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Options____;
	mii.cch = ST_L__Options____;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( hMenuSub_tools, 4, TRUE, &mii );


	// HELP MENU
	mii.dwTypeData = ST_V_HTTP_Downloader__Home_Page;
	mii.cch = ST_L_HTTP_Downloader__Home_Page;
	mii.wID = MENU_HOME_PAGE;
	_InsertMenuItemW( hMenuSub_help, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_help, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__About;
	mii.cch = ST_L__About;
	mii.wID = MENU_ABOUT;
	_InsertMenuItemW( hMenuSub_help, 2, TRUE, &mii );


	// MENU BAR
	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST_V__File;
	mii.cch = ST_L__File;
	mii.hSubMenu = hMenuSub_file;
	_InsertMenuItemW( g_hMenu, 0, TRUE, &mii );

	mii.dwTypeData = ST_V__Edit;
	mii.cch = ST_L__Edit;
	mii.hSubMenu = g_hMenuSub_edit;
	_InsertMenuItemW( g_hMenu, 1, TRUE, &mii );

	mii.dwTypeData = ST_V__View;
	mii.cch = ST_L__View;
	mii.hSubMenu = g_hMenuSub_view;
	_InsertMenuItemW( g_hMenu, 2, TRUE, &mii );

	mii.dwTypeData = ST_V__Tools;
	mii.cch = ST_L__Tools;
	mii.hSubMenu = hMenuSub_tools;
	_InsertMenuItemW( g_hMenu, 3, TRUE, &mii );

	mii.dwTypeData = ST_V__Help;
	mii.cch = ST_L__Help;
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
				case COLUMN_NUM:
				{
					index = 0;
				}
				break;

				case COLUMN_ACTIVE_PARTS:
				case COLUMN_DATE_AND_TIME_ADDED:
				case COLUMN_DOWNLOAD_DIRECTORY:
				case COLUMN_DOWNLOAD_SPEED:
				case COLUMN_DOWNLOAD_SPEED_LIMIT:
				case COLUMN_DOWNLOADED:
				case COLUMN_FILE_SIZE:
				case COLUMN_FILE_TYPE:
				case COLUMN_FILENAME:
				case COLUMN_PROGRESS:
				case COLUMN_SSL_TLS_VERSION:
				case COLUMN_TIME_ELAPSED:
				case COLUMN_TIME_REMAINING:
				{
					index = GetColumnIndexFromVirtualIndex( *download_columns[ menu_index ], download_columns, NUM_COLUMNS );
				}
				break;

				case COLUMN_URL:
				{
					index = g_total_columns;
				}
				break;
			}

			*download_columns[ menu_index ] = -1;
			_SendMessageW( g_hWnd_files, LVM_DELETECOLUMN, index, 0 );

			if ( menu_index == cfg_sorted_column_index )
			{
				cfg_sorted_column_index = 0;
			}

			if ( g_total_columns == 1 )
			{
				// Find the remaining menu item and disable it.
				for ( int j = 0; j < NUM_COLUMNS; ++j )
				{
					if ( *download_columns[ j ] != -1 )
					{
						_EnableMenuItem( g_hMenuSub_column, COLUMN_MENU_OFFSET + j, MF_GRAYED );

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
				case COLUMN_NUM:
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

				case COLUMN_ACTIVE_PARTS:
				case COLUMN_DATE_AND_TIME_ADDED:
				case COLUMN_DOWNLOAD_DIRECTORY:
				case COLUMN_DOWNLOAD_SPEED:
				case COLUMN_DOWNLOAD_SPEED_LIMIT:
				case COLUMN_DOWNLOADED:
				case COLUMN_FILE_SIZE:
				case COLUMN_FILE_TYPE:
				case COLUMN_FILENAME:
				case COLUMN_PROGRESS:
				case COLUMN_SSL_TLS_VERSION:
				case COLUMN_TIME_ELAPSED:
				case COLUMN_TIME_REMAINING:
				{
					*download_columns[ menu_index ] = g_total_columns;
					index = GetColumnIndexFromVirtualIndex( *download_columns[ menu_index ], download_columns, NUM_COLUMNS );
				}
				break;

				case COLUMN_URL:
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
			//lvc.pszText = download_string_table[ menu_index ].value;
			lvc.pszText = g_locale_table[ DOWNLOAD_STRING_TABLE_OFFSET + menu_index ].value;
			lvc.cx = *download_columns_width[ menu_index ];
			_SendMessageW( g_hWnd_files, LVM_INSERTCOLUMN, index, ( LPARAM )&lvc );

			++g_total_columns;
		}
	}
}
