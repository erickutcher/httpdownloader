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

#include "lite_ole32.h"
#include "lite_comdlg32.h"

#include "menus.h"
#include "utilities.h"
#include "treelistview.h"
#include "categories.h"

#include "connection.h"
#include "list_operations.h"

#include "string_tables.h"
#include "cmessagebox.h"

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
	MENUITEMINFO mii;
	_memzero( &mii, sizeof( MENUITEMINFO ) );
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_STRING;

	TBBUTTONINFO tbb;
	_memzero( &tbb, sizeof( TBBUTTONINFO ) );
	tbb.cbSize = sizeof( TBBUTTONINFO );
	tbb.dwMask = TBIF_STATE;

	if ( enable )
	{
		DOWNLOAD_INFO *di = NULL;

		int item_count = TLV_GetExpandedItemCount();
		int sel_count = TLV_GetSelectedCount();

		bool is_group = false;	// Selected item is a group item or is a single host (not grouped).

		TREELISTNODE *tln = TLV_GetFocusedItem();
		if ( tln == NULL )
		{
			TLV_GetNextSelectedItem( NULL, 0, &tln );
		}

		if ( tln != NULL )
		{
			di = ( DOWNLOAD_INFO * )tln->data;

			if ( tln->data_type & TLVDT_GROUP )
			{
				is_group = true;
			}
		}

		if ( sel_count == 1 )
		{
			if ( di != NULL &&
				!( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
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
			if ( is_group &&
				 di != NULL &&
				 IS_STATUS( di->status, STATUS_QUEUED ) &&
				 !g_in_list_edit_mode )
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
			if ( is_group &&
				 *download_columns[ COLUMN_FILENAME ] != -1 )
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
			// Allow start if paused, queued, stopped, timed out, failed, file IO error, skipped, proxy authorization required, or insufficient disk space.
			if ( di != NULL &&
			 ( ( di->shared_info->file_size == 0 || ( di->shared_info->downloaded < di->shared_info->file_size ) ) &&
			   ( IS_STATUS( di->status, STATUS_PAUSED ) ||
			   ( IS_STATUS( di->status, STATUS_QUEUED ) && ( g_total_downloading < cfg_max_downloads ) ) ||
			   ( di->active_parts == 0 &&
				 IS_STATUS( di->status,
					STATUS_STOPPED |
					STATUS_TIMED_OUT |
					STATUS_FAILED |
					STATUS_FILE_IO_ERROR |
					STATUS_SKIPPED |
					STATUS_AUTH_REQUIRED |
					STATUS_PROXY_AUTH_REQUIRED |
					STATUS_INSUFFICIENT_DISK_SPACE ) ) ) ) ||
			   ( di->shared_info->downloaded >= di->shared_info->file_size && di->shared_info->file_size != 0 && IS_STATUS_NOT( di->shared_info->status, STATUS_COMPLETED | STATUS_QUEUED ) ) )
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
					STATUS_PROXY_AUTH_REQUIRED |
					STATUS_INSUFFICIENT_DISK_SPACE ) )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_RESTART, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_RESTART, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
			}

			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_RESTART, ( LPARAM )&tbb );

			if ( is_group )
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE, MF_ENABLED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE, MF_ENABLED );

				tbb.fsState = TBSTATE_ENABLED;
			}
			else
			{
				_EnableMenuItem( g_hMenuSub_download, MENU_REMOVE, MF_GRAYED );
				_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE, MF_GRAYED );

				tbb.fsState = TBSTATE_INDETERMINATE;
			}

			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_REMOVE, ( LPARAM )&tbb );

			if ( is_group &&
				 di != NULL &&
				!( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
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

			_EnableMenuItem( g_hMenuSub_tray, MENU_PAUSE_ACTIVE, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_tray, MENU_STOP_ALL, MF_ENABLED );

			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_PAUSE_ACTIVE, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_STOP_ALL, MF_ENABLED );

			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}
		else if ( download_queue != NULL )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );

			_EnableMenuItem( g_hMenuSub_tray, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_tray, MENU_STOP_ALL, MF_ENABLED );

			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_STOP_ALL, MF_ENABLED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_tray, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_tray, MENU_STOP_ALL, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_PAUSE_ACTIVE, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_STOP_ALL, MF_GRAYED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );
		}

		if ( item_count > 0 )
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_START_INACTIVE, MF_ENABLED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_ENABLED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_ENABLED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_ENABLED );

			_EnableMenuItem( g_hMenuSub_tray, MENU_START_INACTIVE, MF_ENABLED );

			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_START_INACTIVE, MF_ENABLED );

			tbb.fsState = TBSTATE_ENABLED;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START_INACTIVE, ( LPARAM )&tbb );
		}
		else
		{
			_EnableMenuItem( g_hMenuSub_edit, MENU_START_INACTIVE, MF_GRAYED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_PAUSE_ACTIVE, MF_GRAYED );
			//_EnableMenuItem( g_hMenuSub_edit, MENU_STOP_ALL, MF_GRAYED );
			_EnableMenuItem( g_hMenuSub_edit, MENU_REMOVE_COMPLETED, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_tray, MENU_START_INACTIVE, MF_GRAYED );

			_EnableMenuItem( g_hMenuSub_drag_drop, MENU_START_INACTIVE, MF_GRAYED );

			tbb.fsState = TBSTATE_INDETERMINATE;
			_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START_INACTIVE, ( LPARAM )&tbb );
		}

		_EnableMenuItem( g_hMenuSub_download, MENU_ADD_URLS, MF_ENABLED );

		_EnableMenuItem( g_hMenuSub_download, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_GRAYED ) );

		_EnableMenuItem( g_hMenuSub_edit, MENU_SELECT_ALL, ( sel_count != item_count ? MF_ENABLED : MF_GRAYED ) );

		wchar_t *item_text, *item_text2;
		UINT item_text_length, item_text_length2;
		if ( di != NULL && IS_STATUS( di->status, STATUS_PAUSED ) && sel_count > 0 )
		{
			item_text = ST_V_Resume;
			item_text_length = ST_L_Resume;

			item_text2 = ST_V_Resu_me;
			item_text_length2 = ST_L_Resu_me;
		}
		else
		{
			item_text = ST_V_Start;
			item_text_length = ST_L_Start;

			item_text2 = ST_V_St_art;
			item_text_length2 = ST_L_St_art;
		}

		mii.dwTypeData = item_text;
		mii.cch = item_text_length;

		_SetMenuItemInfoW( g_hMenuSub_edit, MENU_START, FALSE, &mii );

		mii.dwTypeData = item_text2;
		mii.cch = item_text_length2;

		_SetMenuItemInfoW( g_hMenuSub_download, MENU_START, FALSE, &mii );

		tbb.dwMask = TBIF_TEXT;
		tbb.pszText = item_text;
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
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
		_EnableMenuItem( g_hMenuSub_edit, MENU_START_INACTIVE, MF_GRAYED );
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

		_EnableMenuItem( g_hMenuSub_tray, MENU_START_INACTIVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_tray, MENU_PAUSE_ACTIVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_tray, MENU_STOP_ALL, MF_GRAYED );

		_EnableMenuItem( g_hMenuSub_drag_drop, MENU_START_INACTIVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_drag_drop, MENU_PAUSE_ACTIVE, MF_GRAYED );
		_EnableMenuItem( g_hMenuSub_drag_drop, MENU_STOP_ALL, MF_GRAYED );

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
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START_INACTIVE, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_PAUSE_ACTIVE, ( LPARAM )&tbb );
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_STOP_ALL, ( LPARAM )&tbb );

		mii.dwTypeData = ST_V_Start;
		mii.cch = ST_L_Start;

		_SetMenuItemInfoW( g_hMenuSub_edit, MENU_START, FALSE, &mii );

		mii.dwTypeData = ST_V_St_art;
		mii.cch = ST_L_St_art;

		_SetMenuItemInfoW( g_hMenuSub_download, MENU_START, FALSE, &mii );

		tbb.dwMask = TBIF_TEXT;
		tbb.pszText = ST_V_Start;
		_SendMessageW( g_hWnd_toolbar, TB_SETBUTTONINFO, MENU_START, ( LPARAM )&tbb );
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

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Enable_List_Edit_Mode;
	mii.cch = ST_L_Enable_List_Edit_Mode;
	mii.wID = MENU_LIST_EDIT_MODE;
	_InsertMenuItemW( g_hMenuSub_download, 12, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Queue;
	mii.cch = ST_L_Queue;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_download, 13, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 14, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Remove;
	mii.cch = ST_L_Remove;
	mii.wID = MENU_REMOVE;
	_InsertMenuItemW( g_hMenuSub_download, 15, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_and_Delete;
	mii.cch = ST_L_Remove_and_Delete;
	mii.wID = MENU_REMOVE_AND_DELETE;
	_InsertMenuItemW( g_hMenuSub_download, 16, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 17, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Delete;
	mii.cch = ST_L_Delete;
	mii.wID = MENU_DELETE;
	_InsertMenuItemW( g_hMenuSub_download, 18, TRUE, &mii );

	mii.dwTypeData = ST_V_Rename;
	mii.cch = ST_L_Rename;
	mii.wID = MENU_RENAME;
	_InsertMenuItemW( g_hMenuSub_download, 19, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 20, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Copy_URL_s_;
	mii.cch = ST_L_Copy_URL_s_;
	mii.wID = MENU_COPY_URLS;
	_InsertMenuItemW( g_hMenuSub_download, 21, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_download, 22, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Select_All;
	mii.cch = ST_L_Select_All;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_download, 23, TRUE, &mii );

	//

	for ( char i = 0; i < NUM_COLUMNS; ++i )
	{
		mii.dwTypeData = download_string_table[ i ].value;
		mii.cch = download_string_table[ i ].length;
		mii.wID = MENU_COLUMNS + i;
		mii.fState = ( *download_columns[ i ] != -1 ? MFS_CHECKED | ( g_total_columns > 1 ? MFS_ENABLED : MFS_DISABLED ) : MFS_UNCHECKED );
		_InsertMenuItemW( g_hMenuSub_column, i, TRUE, &mii );
	}

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
	mii.dwTypeData = ST_V_Start___Resume_Inactive;
	mii.cch = ST_L_Start___Resume_Inactive;
	mii.wID = MENU_START_INACTIVE;
	_InsertMenuItemW( g_hMenuSub_tray, 4, TRUE, &mii );

	mii.dwTypeData = ST_V_Pause_Active;
	mii.cch = ST_L_Pause_Active;
	mii.wID = MENU_PAUSE_ACTIVE;
	_InsertMenuItemW( g_hMenuSub_tray, 5, TRUE, &mii );

	mii.dwTypeData = ST_V_Stop_All;
	mii.cch = ST_L_Stop_All;
	mii.wID = MENU_STOP_ALL;
	_InsertMenuItemW( g_hMenuSub_tray, 6, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 7, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Options___;
	mii.cch = ST_L_Options___;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_tray, 8, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_tray, 9, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Exit;
	mii.cch = ST_L_Exit;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_tray, 10, TRUE, &mii );

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
	mii.dwTypeData = ST_V_Start___Resume_Inactive;
	mii.cch = ST_L_Start___Resume_Inactive;
	mii.wID = MENU_START_INACTIVE;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 6, TRUE, &mii );

	mii.dwTypeData = ST_V_Pause_Active;
	mii.cch = ST_L_Pause_Active;
	mii.wID = MENU_PAUSE_ACTIVE;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 7, TRUE, &mii );

	mii.dwTypeData = ST_V_Stop_All;
	mii.cch = ST_L_Stop_All;
	mii.wID = MENU_STOP_ALL;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 8, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 9, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Options___;
	mii.cch = ST_L_Options___;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 10, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 11, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Exit;
	mii.cch = ST_L_Exit;
	mii.wID = MENU_EXIT;
	_InsertMenuItemW( g_hMenuSub_drag_drop, 12, TRUE, &mii );

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
	mii.dwTypeData = ST_V_Start___Resume_Inactive;
	mii.cch = ST_L_Start___Resume_Inactive;
	mii.wID = MENU_START_INACTIVE;
	_InsertMenuItemW( g_hMenuSub_edit, 5, TRUE, &mii );

	mii.dwTypeData = ST_V_Pause_Active;
	mii.cch = ST_L_Pause_Active;
	mii.wID = MENU_PAUSE_ACTIVE;
	_InsertMenuItemW( g_hMenuSub_edit, 6, TRUE, &mii );

	mii.dwTypeData = ST_V_Stop_All;
	mii.cch = ST_L_Stop_All;
	mii.wID = MENU_STOP_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 7, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 8, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Update_Download____;
	mii.cch = ST_L__Update_Download____;
	mii.wID = MENU_UPDATE_DOWNLOAD;
	_InsertMenuItemW( g_hMenuSub_edit, 9, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 10, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Enable_List__Edit_Mode;
	mii.cch = ST_L_Enable_List__Edit_Mode;
	mii.wID = MENU_LIST_EDIT_MODE;
	_InsertMenuItemW( g_hMenuSub_edit, 11, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
	mii.dwTypeData = ST_V_Queue;
	mii.cch = ST_L_Queue;
	mii.hSubMenu = g_hMenuSub_queue;
	_InsertMenuItemW( g_hMenuSub_edit, 12, TRUE, &mii );

	mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 13, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Remove_;
	mii.cch = ST_L__Remove_;
	mii.wID = MENU_REMOVE;
	_InsertMenuItemW( g_hMenuSub_edit, 14, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_Completed_;
	mii.cch = ST_L_Remove_Completed_;
	mii.wID = MENU_REMOVE_COMPLETED;
	_InsertMenuItemW( g_hMenuSub_edit, 15, TRUE, &mii );

	mii.dwTypeData = ST_V_Remove_and_Delete_;
	mii.cch = ST_L_Remove_and_Delete_;
	mii.wID = MENU_REMOVE_AND_DELETE;
	_InsertMenuItemW( g_hMenuSub_edit, 16, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 17, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Delete_;
	mii.cch = ST_L__Delete_;
	mii.wID = MENU_DELETE;
	_InsertMenuItemW( g_hMenuSub_edit, 18, TRUE, &mii );

	mii.dwTypeData = ST_V_Rename_;
	mii.cch = ST_L_Rename_;
	mii.wID = MENU_RENAME;
	_InsertMenuItemW( g_hMenuSub_edit, 19, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 20, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Copy_URL_s_;
	mii.cch = ST_L__Copy_URL_s_;
	mii.wID = MENU_COPY_URLS;
	_InsertMenuItemW( g_hMenuSub_edit, 21, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( g_hMenuSub_edit, 22, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Select_All_;
	mii.cch = ST_L__Select_All_;
	mii.wID = MENU_SELECT_ALL;
	_InsertMenuItemW( g_hMenuSub_edit, 23, TRUE, &mii );


	// VIEW MENU
	mii.dwTypeData = ST_V__Toolbar;
	mii.cch = ST_L__Toolbar;
	mii.wID = MENU_SHOW_TOOLBAR;
	mii.fState = ( cfg_show_toolbar ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 0, TRUE, &mii );

	mii.dwTypeData = ST_V_C_ategories;
	mii.cch = ST_L_C_ategories;
	mii.wID = MENU_SHOW_CATEGORIES;
	mii.fState = ( cfg_show_categories ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 1, TRUE, &mii );

	mii.dwTypeData = ST_V__Column_Headers;
	mii.cch = ST_L__Column_Headers;
	mii.wID = MENU_SHOW_COLUMN_HEADERS;
	mii.fState = ( cfg_show_column_headers ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 2, TRUE, &mii );

	mii.dwTypeData = ST_V__Status_Bar;
	mii.cch = ST_L__Status_Bar;
	mii.wID = MENU_SHOW_STATUS_BAR;
	mii.fState = ( cfg_show_status_bar ? MFS_CHECKED : MFS_UNCHECKED );
	_InsertMenuItemW( g_hMenuSub_view, 3, TRUE, &mii );


	// TOOLS MENU
	mii.dwTypeData = ST_V__Search____;
	mii.cch = ST_L__Search____;
	mii.wID = MENU_SEARCH;
	mii.fState = 0;
	_InsertMenuItemW( hMenuSub_tools, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Global_Download_Speed__Limit____;
	mii.cch = ST_L_Global_Download_Speed__Limit____;
	mii.wID = MENU_GLOBAL_SPEED_LIMIT;
	_InsertMenuItemW( hMenuSub_tools, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V_Site__Manager____;
	mii.cch = ST_L_Site__Manager____;
	mii.wID = MENU_SITE_MANAGER;
	_InsertMenuItemW( hMenuSub_tools, 4, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_tools, 5, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Options____;
	mii.cch = ST_L__Options____;
	mii.wID = MENU_OPTIONS;
	_InsertMenuItemW( hMenuSub_tools, 6, TRUE, &mii );


	// HELP MENU
	mii.dwTypeData = ST_V_HTTP_Downloader__Home_Page;
	mii.cch = ST_L_HTTP_Downloader__Home_Page;
	mii.wID = MENU_HOME_PAGE;
	_InsertMenuItemW( hMenuSub_help, 0, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_help, 1, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__Check_for_Updates___;
	mii.cch = ST_L__Check_for_Updates___;
	mii.wID = MENU_CHECK_FOR_UPDATES;
	_InsertMenuItemW( hMenuSub_help, 2, TRUE, &mii );

	mii.fType = MFT_SEPARATOR;
	_InsertMenuItemW( hMenuSub_help, 3, TRUE, &mii );

	mii.fType = MFT_STRING;
	mii.dwTypeData = ST_V__About;
	mii.cch = ST_L__About;
	mii.wID = MENU_ABOUT;
	_InsertMenuItemW( hMenuSub_help, 4, TRUE, &mii );


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

void UpdateColumns( WORD menu_id )
{
	int arr[ NUM_COLUMNS ];
	unsigned char menu_index = ( unsigned char )( menu_id - COLUMN_MENU_OFFSET );

	if ( menu_index >= 0 && menu_index < NUM_COLUMNS )
	{
		int offset = 0;
		int index = 0;

		_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );
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
				case COLUMN_CATEGORY:
				case COLUMN_COMMENTS:
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
			_SendMessageW( g_hWnd_tlv_header, HDM_DELETEITEM, index, 0 );

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
				case COLUMN_CATEGORY:
				case COLUMN_COMMENTS:
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

			HDITEM hdi;
			_memzero( &hdi, sizeof( HDITEM ) );
			hdi.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_ORDER;
			hdi.iOrder = *download_columns[ menu_index ];

			if ( menu_index == COLUMN_ACTIVE_PARTS ||
				 menu_index == COLUMN_DOWNLOAD_SPEED ||
				 menu_index == COLUMN_DOWNLOAD_SPEED_LIMIT ||
				 menu_index == COLUMN_DOWNLOADED ||
				 menu_index == COLUMN_FILE_SIZE ||
				 menu_index == COLUMN_TIME_ELAPSED ||
				 menu_index == COLUMN_TIME_REMAINING )
			{
				hdi.fmt = HDF_RIGHT | HDF_STRING;
			}
			else if ( menu_index == COLUMN_PROGRESS )	// Progress
			{
				hdi.fmt = HDF_CENTER | HDF_STRING;
			}
			else
			{
				hdi.fmt = HDF_LEFT | HDF_STRING;
			}

			//hdi.pszText = download_string_table[ menu_index ].value;
			hdi.pszText = g_locale_table[ DOWNLOAD_STRING_TABLE_OFFSET + menu_index ].value;
			hdi.cxy = *download_columns_width[ menu_index ];
			_SendMessageW( g_hWnd_tlv_header, HDM_INSERTITEM, index, ( LPARAM )&hdi );

			++g_total_columns;
		}

		_SendMessageW( g_hWnd_tlv_header, HDM_GETORDERARRAY, g_total_columns, ( LPARAM )arr );

		offset = 0;
		for ( int i = 0; i < NUM_COLUMNS; ++i )
		{
			if ( *download_columns[ i ] != -1 )
			{
				*download_columns[ i ] = ( char )arr[ offset++ ];
			}
		}

		RECT rc;
		_memzero( &rc, sizeof( RECT ) );
		_SendMessageW( g_hWnd_tlv_header, HDM_GETITEMRECT, arr[ g_total_columns - 1 ], ( LPARAM )&rc );

		g_header_width = rc.right;

		_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, FALSE );
	}
}

void HandleCommand( HWND hWnd, WORD command )
{
	switch ( command )
	{
		case MENU_OPEN_FILE:
		case MENU_OPEN_DIRECTORY:
		{
			TREELISTNODE *tln = TLV_GetFocusedItem();
			if ( tln == NULL )
			{
				TLV_GetNextSelectedItem( NULL, 0, &tln );
			}

			DOWNLOAD_INFO *di = NULL;
			if ( tln != NULL )
			{
				di = ( DOWNLOAD_INFO * )tln->data;
			}

			if ( di != NULL && !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
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

				if ( command == MENU_OPEN_FILE )
				{
					// Set the verb to NULL so that unknown file types can be handled by the system.
					HINSTANCE hInst = _ShellExecuteW( NULL, NULL, file_path, NULL, NULL, SW_SHOWNORMAL );
					if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )
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
				}
				else if ( command == MENU_OPEN_DIRECTORY )
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
						hInst = _ShellExecuteW( NULL, L"open", di->shared_info->file_path, NULL, NULL, SW_SHOWNORMAL );
					}

					// Use this instead of ILFree on Windows 2000 or later.
					if ( iidl != NULL )
					{
						_CoTaskMemFree( iidl );
					}

					if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )	// We're opening a folder, but it uses the same error code as a file if it's not found.
					{
						CMessageBoxW( hWnd, ST_V_The_specified_path_was_not_found, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
					}
				}

				if ( destroy )
				{
					_CoUninitialize();
				}
			}
		}
		break;

		case MENU_SAVE_DOWNLOAD_HISTORY:
		{
			wchar_t *file_path = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			wchar_t filter[ 64 ];
			int filter_length = min( ST_L_CSV__Comma_delimited_, ( 64 - 16 ) );
			_wmemcpy_s( filter, 64, ST_V_CSV__Comma_delimited_, filter_length );
			_wmemcpy_s( filter + filter_length, 64 - filter_length, L" (*.csv)\0*.csv\0\0", 16 );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = filter;
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

			wchar_t filter[ 64 ];
			int filter_length = min( ST_L_Download_History, ( 64 - 16 ) );
			_wmemcpy_s( filter, 64, ST_V_Download_History, filter_length );
			_wmemcpy_s( filter + filter_length, 64 - filter_length, L" (*.hdh)\0*.hdh\0\0", 16 );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = filter;
			ofn.lpstrDefExt = L"hdh";
			ofn.lpstrTitle = ST_V_Import_Download_History;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH * MAX_PATH;
			ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY;

			if ( _GetOpenFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				if ( iei != NULL )
				{
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
			else
			{
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_EXPORT_DOWNLOAD_HISTORY:
		{
			wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

			wchar_t filter[ 64 ];
			int filter_length = min( ST_L_Download_History, ( 64 - 16 ) );
			_wmemcpy_s( filter, 64, ST_V_Download_History, filter_length );
			_wmemcpy_s( filter + filter_length, 64 - filter_length, L" (*.hdh)\0*.hdh\0\0", 16 );

			OPENFILENAME ofn;
			_memzero( &ofn, sizeof( OPENFILENAME ) );
			ofn.lStructSize = sizeof( OPENFILENAME );
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = filter;
			ofn.lpstrDefExt = L"hdh";
			ofn.lpstrTitle = ST_V_Export_Download_History;
			ofn.lpstrFile = file_name;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_READONLY;

			if ( _GetSaveFileNameW( &ofn ) )
			{
				importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
				if ( iei != NULL )
				{
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
			else
			{
				GlobalFree( file_name );
			}
		}
		break;

		case MENU_START:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_DOWNLOADING, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_PAUSE:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_PAUSED, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_STOP:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_STOPPED, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_RESTART:
		{
			if ( cfg_override_list_prompts || CMessageBoxW( hWnd, ST_V_PROMPT_restart_selected_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )STATUS_RESTART, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
		}
		break;

		case MENU_START_INACTIVE:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_connection, ( void * )UINT_MAX, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_PAUSE_ACTIVE:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )0, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_STOP_ALL:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )1, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_UPDATE_DOWNLOAD:
		{
			if ( TLV_GetSelectedCount() == 1 )
			{
				TREELISTNODE *tln = TLV_GetFocusedItem();
				if ( tln == NULL )
				{
					TLV_GetNextSelectedItem( NULL, 0, &tln );
				}

				DOWNLOAD_INFO *di = NULL;
				if ( tln != NULL )
				{
					di = ( DOWNLOAD_INFO * )tln->data;
				}

				if ( di != NULL )
				{
					if ( g_hWnd_update_download == NULL )
					{
						g_hWnd_update_download = _CreateWindowExW( ( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ) | ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_update_download", ST_V_Update_Download, WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 600, 376, NULL, NULL, NULL, NULL );
					}
					else if ( _IsIconic( g_hWnd_update_download ) )	// If minimized, then restore the window.
					{
						_ShowWindow( g_hWnd_update_download, SW_RESTORE );
					}

					_SendMessageW( g_hWnd_update_download, WM_PROPAGATE, 0, ( LPARAM )di/*lvi.lParam*/ );
				}
			}
		}
		break;

		case MENU_LIST_EDIT_MODE:
		{
			if ( g_in_list_edit_mode )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_queue, ( void * )4, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}

			g_in_list_edit_mode = !g_in_list_edit_mode;

			if ( g_in_list_edit_mode )
			{
				HDITEM hdi;
				_memzero( &hdi, sizeof( HDITEM ) );
				hdi.mask = HDI_FORMAT | HDI_ORDER;

				if ( cfg_sorted_column_index != 0 )
				{
					cfg_sorted_column_index = 0;

					g_download_history_changed = true;
				}

				// Remove the sort format for all columns.
				for ( unsigned char i = 1; _SendMessageW( g_hWnd_tlv_header, HDM_GETITEM, i, ( LPARAM )&hdi ) == TRUE; ++i )
				{
					// Remove sort up and sort down
					hdi.fmt = hdi.fmt & ( ~HDF_SORTUP ) & ( ~HDF_SORTDOWN );
					_SendMessageW( g_hWnd_tlv_header, HDM_SETITEM, i, ( LPARAM )&hdi );
				}

				cfg_sorted_direction = 0;

				_CheckMenuItem( g_hMenuSub_edit, MENU_LIST_EDIT_MODE, MF_CHECKED );
				_CheckMenuItem( g_hMenuSub_download, MENU_LIST_EDIT_MODE, MF_CHECKED );
			}
			else
			{
				_CheckMenuItem( g_hMenuSub_edit, MENU_LIST_EDIT_MODE, MF_UNCHECKED );
				_CheckMenuItem( g_hMenuSub_download, MENU_LIST_EDIT_MODE, MF_UNCHECKED );
			}
		}
		break;

		case MENU_QUEUE_TOP:
		case MENU_QUEUE_UP:
		case MENU_QUEUE_DOWN:
		case MENU_QUEUE_BOTTOM:
		{
			unsigned char handle_type = 0;

			switch ( command )
			{
				case MENU_QUEUE_TOP: { handle_type = 0; } break;
				case MENU_QUEUE_UP: { handle_type = 1; } break;
				case MENU_QUEUE_DOWN: { handle_type = 2; } break;
				case MENU_QUEUE_BOTTOM: { handle_type = 3; } break;
			}

			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_queue, ( void * )handle_type, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_REMOVE:
		{
			if ( cfg_override_list_prompts || CMessageBoxW( hWnd, ST_V_PROMPT_remove_selected_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )0, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
		}
		break;

		case MENU_REMOVE_COMPLETED:
		{
			if ( cfg_override_list_prompts || CMessageBoxW( hWnd, ST_V_PROMPT_remove_completed_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_download_list, ( void * )2, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
		}
		break;

		case MENU_REMOVE_AND_DELETE:
		{
			if ( cfg_override_list_prompts || CMessageBoxW( hWnd, ST_V_PROMPT_remove_and_delete_selected_entries, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, remove_items, ( void * )1, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
		}
		break;

		case MENU_COPY_URLS:
		{
			HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, copy_urls, ( void * )NULL, 0, NULL );
			if ( thread != NULL )
			{
				CloseHandle( thread );
			}
		}
		break;

		case MENU_DELETE:
		{
			if ( cfg_override_list_prompts || CMessageBoxW( hWnd, ST_V_PROMPT_delete_selected_files, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING | CMB_YESNO ) == CMBIDYES )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, delete_files, ( void * )NULL, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
		}
		break;

		case MENU_RENAME:
		{
			TREELISTNODE *tli_node = TLV_GetFocusedItem();
			if ( tli_node != NULL )
			{
				_SendMessageW( g_hWnd_tlv_files, TLVM_EDIT_LABEL, 0, 0 );
			}
		}
		break;

		case MENU_SELECT_ALL:
		{
			TLV_SelectAll( g_hWnd_tlv_files, false );

			UpdateMenus( true );
		}
		break;

		case MENU_NUM:
		case MENU_ACTIVE_PARTS:
		case MENU_CATEGORY:
		case MENU_COMMENTS:
		case MENU_DATE_AND_TIME_ADDED:
		case MENU_DOWNLOAD_DIRECTORY:
		case MENU_DOWNLOAD_SPEED:
		case MENU_DOWNLOAD_SPEED_LIMIT:
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
			UpdateColumns( command );
		}
		break;

		case MENU_ADD_URLS:
		{
			if ( g_hWnd_add_urls == NULL )
			{
				g_hWnd_add_urls = _CreateWindowExW( ( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ) | ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_add_urls", ST_V_Add_URL_s_, WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 620, 270, NULL, NULL, NULL, NULL );
			}

			_SendMessageW( g_hWnd_add_urls, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_SHOW_TOOLBAR:
		{
			cfg_show_toolbar = !cfg_show_toolbar;

			if ( cfg_show_toolbar )
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_TOOLBAR, MF_CHECKED );
				_ShowWindow( g_hWnd_toolbar, SW_SHOW );
			}
			else
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_TOOLBAR, MF_UNCHECKED );
				_ShowWindow( g_hWnd_toolbar, SW_HIDE );
			}

			_SendMessageW( hWnd, WM_SIZE, 0, 0 );
		}
		break;

		case MENU_SHOW_CATEGORIES:
		{
			cfg_show_categories = !cfg_show_categories;

			if ( cfg_show_categories )
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_CATEGORIES, MF_CHECKED );
				_ShowWindow( g_hWnd_categories, SW_SHOW );
				_SetFocus( g_hWnd_categories );
			}
			else
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_CATEGORIES, MF_UNCHECKED );
				_ShowWindow( g_hWnd_categories, SW_HIDE );
				_SetFocus( g_hWnd_tlv_files );
			}

			HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_ROOT, NULL );
			_SendMessageW( g_hWnd_categories, TVM_SELECTITEM, TVGN_CARET, ( LPARAM )hti );

			_SendMessageW( hWnd, WM_SIZE, 0, 0 );
		}
		break;

		case MENU_SHOW_COLUMN_HEADERS:
		{
			cfg_show_column_headers = !cfg_show_column_headers;

			if ( cfg_show_column_headers )
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_COLUMN_HEADERS, MF_CHECKED );
				_SendMessageW( g_hWnd_tlv_files, TLVM_SH_COLUMN_HEADERS, TRUE, 0 );
			}
			else
			{
				_CheckMenuItem( g_hMenuSub_view, MENU_SHOW_COLUMN_HEADERS, MF_UNCHECKED );
				_SendMessageW( g_hWnd_tlv_files, TLVM_SH_COLUMN_HEADERS, FALSE, 0 );
			}
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
				g_hWnd_search = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_search", ST_V_Search, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, 0, 400, 212, NULL, NULL, NULL, NULL );
			}

			_SendMessageW( g_hWnd_search, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_GLOBAL_SPEED_LIMIT:
		{
			if ( g_hWnd_download_speed_limit == NULL )
			{
				g_hWnd_download_speed_limit = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_download_speed_limit", ST_V_Global_Download_Speed_Limit, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, 0, 330, 123, NULL, NULL, NULL, NULL );
				_ShowWindow( g_hWnd_download_speed_limit, SW_SHOWNORMAL );
			}
			_SetForegroundWindow( g_hWnd_download_speed_limit );
		}
		break;

		case MENU_SITE_MANAGER:
		{
			if ( g_hWnd_site_manager == NULL )
			{
				// Painting issues and slowness with WS_EX_COMPOSITED on XP. I think the listview has something to do with it.
				g_hWnd_site_manager = _CreateWindowExW( /*( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ) |*/ ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_site_manager", ST_V_Site_Manager, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, MIN_WIDTH, MIN_HEIGHT, NULL, NULL, NULL, NULL );
			}

			_SendMessageW( g_hWnd_site_manager, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_OPTIONS:
		{
			if ( g_hWnd_options == NULL )
			{
				g_hWnd_options = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_options", ST_V_Options, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU /*| WS_THICKFRAME*/, CW_USEDEFAULT, 0, 755, 500, NULL, NULL, NULL, NULL );
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

		case MENU_CHECK_FOR_UPDATES:
		{
			g_update_check_state = 1;	// Manual update check.

			if ( g_hWnd_check_for_updates == NULL )
			{
				g_hWnd_check_for_updates = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_check_for_updates", ST_V_Check_For_Updates, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, 0, 441, 137, NULL, NULL, NULL, NULL );

				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, CheckForUpdates, NULL, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}

			_SendMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 0, 0 );
		}
		break;

		case MENU_ABOUT:
		{
			wchar_t msg[ 512 ];
			int msg_length = __snwprintf( msg, 512, L"%s\r\n\r\n" \
												    L"%s %lu.%lu.%lu.%lu%s%s%s%.0lu (%u-bit)\r\n\r\n" \
												    L"%s %s, %s %d, %04d %d:%02d:%02d %s (UTC)\r\n\r\n" \
												    L"%s \xA9 2015-2025 Eric Kutcher\r\n\r\n" \
												    L"%s ",
												    ST_V_LICENSE,
												    ST_V_VERSION,
													CURRENT_VERSION_A, CURRENT_VERSION_B, CURRENT_VERSION_C, CURRENT_VERSION_D,
#ifdef IS_BETA
													L" ", ST_V_BETA, L" ", BETA_VERSION,
#else
													L"", L"", L"", 0,
#endif
#ifdef _WIN64
												    64,
#else
												    32,
#endif
												    ST_V_BUILT,
												    ( g_compile_time.wDayOfWeek > 6 ? L"" : GetDay( g_compile_time.wDayOfWeek ) ),
												    ( ( g_compile_time.wMonth > 12 || g_compile_time.wMonth < 1 ) ? L"" : GetMonth( g_compile_time.wMonth ) ),
												    g_compile_time.wDay,
												    g_compile_time.wYear,
												    ( g_compile_time.wHour > 12 ? g_compile_time.wHour - 12 : ( g_compile_time.wHour != 0 ? g_compile_time.wHour : 12 ) ),
												    g_compile_time.wMinute,
												    g_compile_time.wSecond,
												    ( g_compile_time.wHour >= 12 ? L"PM" : L"AM" ),
												    ST_V_COPYRIGHT,
												    ST_V_Total_downloaded_ );

			FormatSizes( msg + msg_length, 512 - msg_length, SIZE_FORMAT_AUTO, cfg_total_downloaded );

			CMessageBoxW( hWnd, msg, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
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

		case MENU_CAT_ADD:
		case MENU_CAT_UPDATE:
		{
			if ( g_hWnd_add_category == NULL )
			{
				g_hWnd_add_category = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"class_add_category", ST_V_Add_Category, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, 0, 430, 196, NULL, NULL, NULL, NULL );
			}

			unsigned char type;
			HTREEITEM hti;

			if ( command == MENU_CAT_UPDATE )
			{
				type = 1;
				hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_CARET, NULL );
			}
			else
			{
				type = 0;
				hti = NULL;
			}

			_SendMessageW( g_hWnd_add_category, WM_PROPAGATE, type, ( LPARAM )hti );
		}
		break;

		case MENU_CAT_REMOVE:
		{
			CATEGORY_UPDATE_INFO *cui = ( CATEGORY_UPDATE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( CATEGORY_UPDATE_INFO ) );
			if ( cui != NULL )
			{
				cui->ci = NULL;
				cui->old_ci = NULL;
				cui->update_type = 2;	// Remove
				cui->hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_CARET, NULL );

				// cui is freed in handle_category_list.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, handle_category_list, ( void * )cui, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					//FreeCategoryInfo( &cui->ci );	// Already NULL.
					GlobalFree( cui );
				}
			}
		}
		break;

		case MENU_CAT_OPEN:
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

			HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_categories, TVM_GETNEXTITEM, TVGN_CARET, NULL );

			TVITEM tvi;
			_memzero( &tvi, sizeof( TVITEM ) );
			tvi.mask = TVIF_PARAM;
			tvi.hItem = hti;
			_SendMessageW( g_hWnd_categories, TVM_GETITEM, 0, ( LPARAM )&tvi );

			DoublyLinkedList *dll_node = ( DoublyLinkedList * )tvi.lParam;
			if ( dll_node != NULL )
			{
				CATEGORY_TREE_INFO *cti = ( CATEGORY_TREE_INFO * )dll_node->data;
				if ( cti != NULL && cti->type == CATEGORY_TREE_INFO_TYPE_CATEGORY_INFO && cti->data != NULL )
				{
					CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )cti->data;

					HINSTANCE hInst = _ShellExecuteW( NULL, L"open", ci->download_directory, NULL, NULL, SW_SHOWNORMAL );

					if ( hInst == ( HINSTANCE )ERROR_FILE_NOT_FOUND )	// We're opening a folder, but it uses the same error code as a file if it's not found.
					{
						CMessageBoxW( hWnd, ST_V_The_specified_path_was_not_found, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );
					}
				}
			}

			if ( destroy )
			{
				_CoUninitialize();
			}
		}
		break;
	}
}
