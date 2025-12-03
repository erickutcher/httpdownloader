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

#include "lite_kernel32.h"
#include "lite_pcre2.h"
#include "lite_normaliz.h"

#include "connection.h"

#include "doublylinkedlist.h"
#include "treelistview.h"

#include "categories.h"
#include "list_operations.h"
#include "file_operations.h"

#include "utilities.h"

#include "menus.h"

#include "string_tables.h"

void ProcessingList( bool processing )
{
	if ( processing )
	{
		//_SetWindowTextW( g_hWnd_main, L"HTTP Downloader - Please wait..." );	// Update the window title.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, TRUE, 0 );				// SetCursor only works from the main thread. Set it to an arrow with hourglass.
		UpdateMenus( false );													// Disable all processing menu items.
	}
	else
	{
		UpdateMenus( true );										// Enable the appropriate menu items.
		_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.
		_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, FALSE );	// Refresh the number column values.
		_SetFocus( g_hWnd_tlv_files );								// Give focus back to the listview to allow shortcut keys.
		//_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );			// Reset the window title.
	}
}

void SetContextStatus( SOCKET_CONTEXT *context, unsigned int status )
{
	if ( context != NULL )
	{
		EnterCriticalSection( &context->context_cs );

		// The paused operation has not completed or it has and is_paused is waiting to be set.
		// We'll fall through in IOCPConnection.
		if ( IS_STATUS( context->status, STATUS_PAUSED ) && !context->is_paused )
		{
			context->status = status;

			if ( context->cleanup == 0 )
			{
				context->cleanup = 1;	// Auto cleanup.
			}

			// The operation is probably stuck (server isn't responding). Force close the socket to release the operation.
			if ( context->pending_operations > 0 )
			{
				if ( context->socket != INVALID_SOCKET )
				{
					SOCKET s = context->socket;
					context->socket = INVALID_SOCKET;
					_shutdown( s, SD_BOTH );
					_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
				}
			}
		}
		else
		{
			// 1 = auto cleanup, 2 = force the cleanup.
			// Paused contexts shouldn't have any pending operations.
			unsigned char cleanup_type = ( IS_STATUS( context->status, STATUS_PAUSED ) ? 1 : 2 );

			context->status = status;

			if ( context->cleanup == 0 )
			{
				if ( context->ssh != NULL )
				{
					// We've initiated a clean shutdown of the SSH connection, but it's stuck.
					if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
					{
						// Force close the socket to release the operation.
						if ( context->socket != INVALID_SOCKET )
						{
							SOCKET s = context->socket;
							context->socket = INVALID_SOCKET;
							_shutdown( s, SD_BOTH );
							_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
						}
					}
					else
					{
						context->overlapped_close.current_operation = IO_SFTPCleanup;
					}
				}
				else
				{
					context->cleanup = cleanup_type;

					InterlockedIncrement( &context->pending_operations );
					context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
				}
			}
			else if ( context->cleanup == 12 )
			{
				if ( context->ssh == NULL )
				{
					// We've initiated a clean shutdown of the connection, but it's stuck.
					if ( context->overlapped_close.current_operation == IO_Write )
					{
						// Force close the socket to release the operation.
						if ( context->socket != INVALID_SOCKET )
						{
							SOCKET s = context->socket;
							context->socket = INVALID_SOCKET;
							_shutdown( s, SD_BOTH );
							_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
						}
					}
				}
			}
		}

		LeaveCriticalSection( &context->context_cs );
	}
}

THREAD_RETURN remove_items( void *pArguments )
{
	unsigned char handle_type = ( unsigned char )pArguments;	// 0 = remove, 1 = remove and delete

	unsigned int status = ( handle_type == 1 ? STATUS_DELETE : STATUS_NONE );

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	// Prevent the listviews from drawing while freeing lParam values.
	_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, TRUE, NULL );

	ProcessingList( true );

	bool delete_success = true;
	unsigned char error_type = 0;

	bool is_active = false;

	int sel_count = TLV_GetSelectedCount();
	int item_count = TLV_GetExpandedItemCount();
	int old_item_count = item_count;
	int visible_item_count = TLV_GetVisibleItemCount();
	int first_visible_index = TLV_GetFirstVisibleIndex();
	int old_first_visible_index = first_visible_index;
	int expanded_item_count = 0;
	int root_item_count = 0;
	int total_item_count = 0;
	TREELISTNODE *first_visible_node = TLV_GetFirstVisibleItem();

	int first_visible_root_index = TLV_GetFirstVisibleRootIndex();

	int sel_index = TLV_GetFirstSelectedIndex();
	TREELISTNODE *tln = TLV_GetFirstSelectedItem();

	// If the item is a child, then get the parent's sibling.
	if ( tln != NULL && tln->parent != NULL )
	{
		sel_index = TLV_GetParentIndex( tln->parent, sel_index );
		sel_index += ( tln->child_count + 1 );

		// Remove any selection flag from the children.
		TREELISTNODE *child_tln = tln;
		while ( child_tln != NULL )
		{
			child_tln->flag = TLVS_NONE;

			child_tln = child_tln->next;
		}

		tln = tln->parent->next;
	}

	TREELISTNODE *last_tln = TLV_GetLastSelectedItem();

	// If the item is a child, then get the parent.
	if ( last_tln != NULL && last_tln->parent != NULL )
	{
		last_tln = last_tln->parent;
	}

	while ( tln != NULL )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		if ( !( tln->flag & TLVS_SELECTED ) )
		{
			++sel_index;
			if ( tln->is_expanded )
			{
				sel_index += tln->child_count;

				// Remove any selection flag from the children.
				TREELISTNODE *child_tln = tln->child;
				while ( child_tln != NULL )
				{
					child_tln->flag = TLVS_NONE;

					child_tln = child_tln->next;
				}
			}

			tln = tln->next;

			continue;
		}

		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;

		if ( di != NULL )
		{
			DecrementStatusCount( di->status );
		}

		TREELISTNODE *tln_parent = tln;

		// We'll go through each child regardless of whether it's selected if a group is selected.
		if ( tln->data_type == TLVDT_GROUP && tln->child != NULL )
		{
			// Is our update window open and are we removing the item we want to update? Close the window if we are.
			if ( g_update_download_info != NULL && di == g_update_download_info )
			{
				g_update_download_info = NULL;

				_SendMessageW( g_hWnd_update_download, WM_DESTROY_ALT, 0, 0 );
			}

			tln = tln->child;
		}

		do
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			// Wait, specifically for CleanupConnection to do its thing.
			EnterCriticalSection( &cleanup_cs );

			di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
#ifdef ENABLE_LOGGING
				GenericLogEntry( di, LOG_INFO_ACTION, "Removing" );
#endif
				// Is our update window open and are we removing the item we want to update? Close the window if we are.
				if ( di == g_update_download_info )
				{
					g_update_download_info = NULL;

					_SendMessageW( g_hWnd_update_download, WM_DESTROY_ALT, 0, 0 );
				}

				_SendMessageW( g_hWnd_main, WM_RESET_PROGRESS, 0, ( LPARAM )di );

				EnterCriticalSection( &di->di_cs );

				DoublyLinkedList *context_node = di->parts_list;

				// If there are still active connections.
				if ( di->download_node.data != NULL )
				{
					is_active = true;

					SetStatus( di, STATUS_STOPPED | STATUS_REMOVE | status );

					LeaveCriticalSection( &di->di_cs );

					while ( context_node != NULL )
					{
						SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

						context_node = context_node->next;

						SetContextStatus( context, STATUS_STOPPED | STATUS_REMOVE | status );
					}
				}
				else	// No active parts.
				{
					if ( di->queue_node.data != NULL )
					{
						EnterCriticalSection( &download_queue_cs );

						DLL_RemoveNode( &download_queue, &di->queue_node );

						LeaveCriticalSection( &download_queue_cs );
					}

					LeaveCriticalSection( &di->di_cs );

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

					--di->shared_info->hosts;

					if ( handle_type == 1 )
					{
						if ( di->shared_info->hosts == 0 &&
							 /*di->shared_info->host_list != NULL &&*/
						  !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
						{
							wchar_t *file_path_delete;

							wchar_t file_path[ MAX_PATH + 1 ];
							if ( cfg_use_temp_download_directory && di->shared_info->status != STATUS_COMPLETED )
							{
								GetTemporaryFilePath( di, file_path );

								file_path_delete = file_path;
							}
							else
							{
								// We're freeing this anyway so it's safe to modify.
								di->shared_info->file_path[ di->shared_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.

								file_path_delete = di->shared_info->file_path;

								file_path[ 0 ] = 0;	// Flag for if we're moving to the Recycle Bin.
							}

							if ( cfg_move_to_trash )
							{
								int file_path_length = lstrlenW( file_path_delete );

								if ( file_path[ 0 ] == 0 )
								{
									_wmemcpy_s( file_path, MAX_PATH + 1, file_path_delete, file_path_length );
								}

								file_path[ file_path_length ] = 0;
								file_path[ file_path_length + 1 ] = 0;

								SHFILEOPSTRUCTW sfos;
								_memzero( &sfos, sizeof( SHFILEOPSTRUCTW ) );
								sfos.wFunc = FO_DELETE;
								sfos.pFrom = file_path;
								sfos.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;

								int error = _SHFileOperationW( &sfos );
								if ( error != 0 )
								{
									delete_success = false;

									if ( error == ERROR_ACCESS_DENIED )
									{
										error_type |= 1;
									}
									else if ( error == ERROR_FILE_NOT_FOUND )
									{
										error_type |= 2;
									}
									else if ( error == ERROR_PATH_NOT_FOUND )
									{
										error_type |= 4;
									}
								}
							}
							else if ( DeleteFileW( file_path_delete ) == FALSE )
							{
								delete_success = false;

								int error = GetLastError();
								if ( error == ERROR_ACCESS_DENIED )
								{
									error_type |= 1;
								}
								else if ( error == ERROR_FILE_NOT_FOUND )
								{
									error_type |= 2;
								}
								else if ( error == ERROR_PATH_NOT_FOUND )
								{
									error_type |= 4;
								}
							}

#ifdef ENABLE_LOGGING
							wchar_t *l_file_path;
							wchar_t t_l_file_path[ MAX_PATH ];
							bool is_temp = false;
							if ( cfg_use_temp_download_directory && di->shared_info->status != STATUS_COMPLETED ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
							else { GetDownloadFilePath( di, t_l_file_path ); }
							l_file_path = t_l_file_path;
							WriteLog( LOG_INFO_ACTION, "Deleting: %s%S", ( is_temp ? "temp | " : "" ), l_file_path );
#endif
						}
					}

					DeleteCriticalSection( &di->di_cs );

					if ( di->shared_info->hosts == 0 )
					{
						RemoveCachedIcon( di->shared_info );
						RemoveCachedCategory( di->shared_info->category );
						GlobalFree( di->shared_info->new_file_path );
						GlobalFree( di->shared_info->w_add_time );

						if ( di->shared_info->hFile != INVALID_HANDLE_VALUE )
						{
							CloseHandle( di->shared_info->hFile );
						}

						if ( di != di->shared_info )
						{
							GlobalFree( di->shared_info->comments );
							GlobalFree( di->shared_info );
						}
					}

					GlobalFree( di );
				}
			}

			LeaveCriticalSection( &cleanup_cs );

			tln = tln->next;
		}
		while ( tln_parent->child != NULL && tln != NULL );

		tln = tln_parent;

		bool last_sel = ( tln == last_tln ? true : false );

		if ( !is_active )
		{
			++root_item_count;

			++expanded_item_count;	// Include the parent.
		}
		else	// Active items will have their status set in SetStatus(). The appropriate item_counts will be set in there.
		{
			--old_item_count;	// Exclude the parent.
		}

		if ( tln->is_expanded )
		{
			expanded_item_count += tln->child_count;
		}

		// Update our first visible index and node depending on where the items are being removed.
		// This allows us to update the scrollbars and scroll position.
		if ( sel_index < old_first_visible_index )
		{
			if ( first_visible_node->parent != NULL )
			{
				int parent_index = TLV_GetParentIndex( first_visible_node, old_first_visible_index );

				if ( sel_index == parent_index )
				{
					first_visible_node = first_visible_node->parent->next;
				}
				else
				{
					--first_visible_root_index;
				}

				if ( tln->is_expanded && sel_index + tln->child_count < old_first_visible_index )
				{
					first_visible_index -= ( tln->child_count + 1 );
				}
				else if ( !tln->is_expanded && sel_index < old_first_visible_index )
				{
					--first_visible_index;
				}
				else	// The first visible is a child node.
				{
					first_visible_index -= ( old_first_visible_index - parent_index );

					item_count -= ( ( tln->child_count - ( old_first_visible_index - parent_index ) ) + 1 );
				}
			}
			else
			{
				if ( first_visible_index > 0 )
				{
					--first_visible_index;

					if ( tln->is_expanded )
					{
						if ( tln->child_count > first_visible_index )
						{
							first_visible_index = 0;
						}
						else
						{
							first_visible_index -= tln->child_count;
						}
					}

					--first_visible_root_index;
				}
			}
		}
		else// if ( sel_index >= old_first_visible_index )
		{
			int sel_item_count = ( tln->is_expanded ? tln->child_count : 0 ) + 1;

			int remaining_items = item_count - old_first_visible_index - sel_item_count;

			// We can remove the item and the list will move down. (We're at the end of the list).
			if ( remaining_items < visible_item_count )
			{
				sel_item_count = visible_item_count - remaining_items;

				for ( ; sel_item_count > 0 && first_visible_index > 0; --sel_item_count, --first_visible_index )
				{
					if ( first_visible_node->parent == NULL )
					{
						--first_visible_root_index;
					}

					first_visible_node = TLV_PrevNode( first_visible_node, false );
				}
			}
			else	// We can remove the item and the list will move up.
			{
				// Adjust this so that if the remaining items cause us to be at the end of the list, then it'll hit the if block above.
				item_count -= sel_item_count;

				if ( tln == first_visible_node )
				{
					if ( first_visible_node->parent != NULL )
					{
						first_visible_node = first_visible_node->parent->next;
					}
					else
					{
						first_visible_node = first_visible_node->next;
					}
				}
				else if ( tln == first_visible_node->parent )	// We're a child item.
				{
					int parent_index = TLV_GetParentIndex( first_visible_node, old_first_visible_index );

					first_visible_index -= ( ( old_first_visible_index - parent_index ) );

					first_visible_node = tln->next;
				}
			}
		}

		TREELISTNODE *del_tln = tln;

		++sel_index;
		if ( tln->is_expanded )
		{
			sel_index += tln->child_count;
		}

		tln = tln->next;

		TLV_RemoveNode( &g_tree_list, del_tln );

		if ( is_active )
		{
			di = ( DOWNLOAD_INFO * )del_tln->data;
			if ( di != NULL )
			{
				EnterCriticalSection( &di->di_cs );

				di->tln = NULL;

				LeaveCriticalSection( &di->di_cs );
			}
		}

		int tic = TLV_FreeTree( del_tln );
		if ( !is_active )
		{
			total_item_count += tic;
		}

		if ( last_sel )
		{
			break;
		}
	}

	// Update our internal values.
	if ( expanded_item_count == old_item_count )
	{
		first_visible_node = NULL;
	}
	TLV_SetFirstVisibleRootIndex( first_visible_root_index );
	TLV_SetFirstVisibleItem( first_visible_node );
	TLV_SetFirstVisibleIndex( first_visible_index );
	TLV_AddTotalItemCount( -total_item_count );
	TLV_AddExpandedItemCount( -expanded_item_count );
	TLV_AddRootItemCount( -root_item_count );
	TLV_SetFocusedItem( NULL );
	TLV_SetFocusedIndex( -1 );
	TLV_SetSelectedCount( 0 );
	TLV_ResetSelectionBounds();

	if ( handle_type == 1 && !delete_success )
	{
		if ( sel_count == 1 )
		{
			if ( error_type & 1 )	// Access Denied
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_File_is_in_use_cannot_delete );
			}
			else if ( error_type & 2 )	// File Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_The_specified_file_was_not_found );
			}
			else if ( error_type & 4 )	// Path Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_The_specified_path_was_not_found );
			}
		}
		else if ( sel_count > 1 )
		{
			if ( error_type & 1 )	// Access Denied
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_files_are_in_use );
			}

			if ( error_type & 2 )	// File Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_files_were_not_found );
			}

			if ( error_type & 4 )	// Path Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_paths_were_not_found );
			}
		}
	}

	g_download_history_changed = true;

	_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, FALSE, NULL );
	UpdateSBItemCount();

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_download_list( void *pArguments )
{
	unsigned char handle_type = ( unsigned char )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	EnterCriticalSection( &cleanup_cs );

	if ( handle_type == 0 )	// Pause active downloads.
	{
		// Go through each active download, and set their status to paused.
		EnterCriticalSection( &active_download_list_cs );

		DoublyLinkedList *active_download_node = active_download_list;

		while ( active_download_node != NULL && !g_end_program )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )active_download_node->data;
			if ( di != NULL )
			{
				DoublyLinkedList *context_node;
				unsigned int status;

				EnterCriticalSection( &di->di_cs );

				// Make sure the status is exclusively connecting or downloading.
				if ( di->status == STATUS_CONNECTING ||
					 di->status == STATUS_DOWNLOADING )
				{
#ifdef ENABLE_LOGGING
					if ( !( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data ) )
					{
						GenericLogEntry( di, LOG_INFO_CON_STATE, "Pausing" );
					}
#endif
					SetStatus( di, di->status | STATUS_PAUSED );

					context_node = di->parts_list;
					status = di->status;

					LeaveCriticalSection( &di->di_cs );

					while ( context_node != NULL )
					{
						SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

						context_node = context_node->next;

						if ( context != NULL )
						{
							EnterCriticalSection( &context->context_cs );

							context->is_paused = false;	// Set to true when last IO operation has completed.

							context->status = status;

							LeaveCriticalSection( &context->context_cs );
						}
					}
				}
				else
				{
					LeaveCriticalSection( &di->di_cs );
				}
			}

			active_download_node = active_download_node->next;
		}

		LeaveCriticalSection( &active_download_list_cs );
	}
	else if ( handle_type == 1 )	// Stop all active and queued downloads.
	{
		// We'll stop queued downloads first so that the active downloads don't trigger the queued downloads to start.

		// Go through each queued download, and set their status to stopped.
		EnterCriticalSection( &download_queue_cs );

		DoublyLinkedList *download_queue_node = download_queue;

		while ( download_queue_node != NULL && !g_end_program )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )download_queue_node->data;

			download_queue_node = download_queue_node->next;

			if ( di != NULL )
			{
				DoublyLinkedList *host_node = di->shared_info->host_list;
				while ( host_node != NULL )
				{
					DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
					if ( host_di != NULL /*&& host_di != di*/ )
					{
						EnterCriticalSection( &host_di->di_cs );

						if ( IS_STATUS( host_di->status, STATUS_QUEUED ) )
						{
#ifdef ENABLE_LOGGING
							GenericLogEntry( host_di, LOG_INFO_ACTION, "Stopping queued" );
#endif
							SetStatus( host_di, STATUS_STOPPED );
						}

						LeaveCriticalSection( &host_di->di_cs );
					}

					host_node = host_node->next;
				}

				EnterCriticalSection( &di->shared_info->di_cs );
				SetStatus( di->shared_info, STATUS_STOPPED );
				LeaveCriticalSection( &di->shared_info->di_cs );

				/*EnterCriticalSection( &di->di_cs );
				SetStatus( di, STATUS_STOPPED );
				LeaveCriticalSection( &di->di_cs );*/

				// Remove the item from the download queue.
				DLL_RemoveNode( &download_queue, &di->queue_node );
				di->queue_node.data = NULL;
			}
		}

		LeaveCriticalSection( &download_queue_cs );

		// Go through each active download, and set their status to stopped.
		EnterCriticalSection( &active_download_list_cs );

		DoublyLinkedList *active_download_node = active_download_list;

		while ( active_download_node != NULL && !g_end_program )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )active_download_node->data;
			if ( di != NULL )
			{
				DoublyLinkedList *context_node;

				EnterCriticalSection( &di->di_cs );

				// Connecting, Downloading, Paused, Queued.
				if ( IS_STATUS( di->status,
						STATUS_CONNECTING |
						STATUS_DOWNLOADING ) )
				{
#ifdef ENABLE_LOGGING
					if ( !( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data ) )
					{
						GenericLogEntry( di, LOG_INFO_CON_STATE, "Stopping active" );
					}
#endif
					// Download is active, close the connection.
					if ( di->download_node.data != NULL )
					{
						context_node = di->parts_list;

						LeaveCriticalSection( &di->di_cs );

						// di->status will be set to STATUS_STOPPED in CleanupConnection().
						while ( context_node != NULL )
						{
							SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							SetContextStatus( context, STATUS_STOPPED );
						}
					}
					else
					{
						if ( di->queue_node.data != NULL )	// Download is queued.
						{
							SetStatus( di, STATUS_STOPPED );

							EnterCriticalSection( &download_queue_cs );

							// Remove the item from the download queue.
							DLL_RemoveNode( &download_queue, &di->queue_node );
							di->queue_node.data = NULL;

							LeaveCriticalSection( &download_queue_cs );
						}

						LeaveCriticalSection( &di->di_cs );
					}
				}
				else
				{
					LeaveCriticalSection( &di->di_cs );
				}
			}

			active_download_node = active_download_node->next;
		}

		LeaveCriticalSection( &active_download_list_cs );
	}
	else if ( handle_type == 2 )	// Remove completed downloads.
	{
		// Prevent the listviews from drawing while freeing lParam values.
		_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, TRUE, NULL );

		int item_count = TLV_GetExpandedItemCount();
		int old_item_count = item_count;
		int visible_item_count = TLV_GetVisibleItemCount();
		int first_visible_index = TLV_GetFirstVisibleIndex();
		int old_first_visible_index = first_visible_index;
		int expanded_item_count = 0;
		int root_item_count = 0;
		int total_item_count = 0;
		int selected_count = 0;
		TREELISTNODE *first_visible_node = TLV_GetFirstVisibleItem();

		int first_visible_root_index = TLV_GetFirstVisibleRootIndex();

		TREELISTNODE *first_selection_node = NULL;
		int first_selection_index = -1;
		TREELISTNODE *last_selection_node = NULL;
		int last_selection_index = -1;

		TREELISTNODE *tln = g_tree_list;
		int item_index = 0;

		while ( tln != NULL )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			bool remove_item = false;

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
				// Make sure the download is completed and subsequently cleaned up before we remove it.
				EnterCriticalSection( &di->di_cs );

				if ( di->status == STATUS_COMPLETED )
				{
					remove_item = true;
				}

				LeaveCriticalSection( &di->di_cs );
			}

			if ( remove_item )
			{
				bool adjust_node_values = false;

				if ( di != NULL )
				{
					DecrementStatusCount( di->status );

					// Only adjust the node values (indices and nodes) if we're on the current filter.
					if ( g_status_filter == STATUS_NONE || IsFilterSet( di, g_status_filter ) )
					{
						adjust_node_values = true;
					}
				}

				TREELISTNODE *tln_parent = tln;

				// We'll go through each child regardless of whether it's selected if a group is selected.
				if ( tln->data_type == TLVDT_GROUP && tln->child != NULL )
				{
					// Is our update window open and are we removing the item we want to update? Close the window if we are.
					if ( di == g_update_download_info )
					{
						g_update_download_info = NULL;

						_SendMessageW( g_hWnd_update_download, WM_DESTROY_ALT, 0, 0 );
					}

					tln = tln->child;
				}

				do
				{
					// Stop processing and exit the thread.
					if ( kill_worker_thread_flag )
					{
						break;
					}

					di = ( DOWNLOAD_INFO * )tln->data;
					if ( di != NULL )
					{
#ifdef ENABLE_LOGGING
						GenericLogEntry( di, LOG_INFO_ACTION, "Removing completed" );
#endif
						// Is our update window open and are we removing the item we want to update? Close the window if we are.
						if ( di == g_update_download_info )
						{
							g_update_download_info = NULL;

							_SendMessageW( g_hWnd_update_download, WM_DESTROY_ALT, 0, 0 );
						}

						_SendMessageW( g_hWnd_main, WM_RESET_PROGRESS, 0, ( LPARAM )di );

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

						--di->shared_info->hosts;

						DeleteCriticalSection( &di->di_cs );

						if ( di->shared_info->hosts == 0 )
						{
							RemoveCachedIcon( di->shared_info );
							RemoveCachedCategory( di->shared_info->category );
							GlobalFree( di->shared_info->new_file_path );
							GlobalFree( di->shared_info->w_add_time );

							if ( di->shared_info->hFile != INVALID_HANDLE_VALUE )
							{
								CloseHandle( di->shared_info->hFile );
							}

							if ( di != di->shared_info )
							{
								GlobalFree( di->shared_info->comments );
								GlobalFree( di->shared_info );
							}
						}

						GlobalFree( di );
					}

					if ( tln->flag & TLVS_SELECTED )
					{
						++selected_count;

						if ( tln->flag & TLVS_FOCUSED )
						{
							TLV_SetFocusedItem( NULL );
							TLV_SetFocusedIndex( -1 );
						}
					}

					tln = tln->next;
				}
				while ( tln_parent->child != NULL && tln != NULL );

				tln = tln_parent;

				if ( adjust_node_values )
				{
					if ( tln->child != NULL && tln->flag & TLVS_SELECTED )
					{
						++selected_count;

						if ( tln->flag & TLVS_FOCUSED )
						{
							TLV_SetFocusedItem( NULL );
							TLV_SetFocusedIndex( -1 );
						}
					}

					++root_item_count;

					++expanded_item_count;	// Include the parent.
					if ( tln->is_expanded )
					{
						expanded_item_count += tln->child_count;
					}

					// Update our first visible index and node depending on where the items are being removed.
					// This allows us to update the scrollbars and scroll position.
					if ( item_index < old_first_visible_index )
					{
						if ( first_visible_node->parent != NULL )
						{
							int parent_index = TLV_GetParentIndex( first_visible_node, old_first_visible_index );

							if ( item_index == parent_index )
							{
								first_visible_node = first_visible_node->parent->next;
							}
							else
							{
								--first_visible_root_index;
							}

							if ( tln->is_expanded && item_index + tln->child_count < old_first_visible_index )
							{
								first_visible_index -= ( tln->child_count + 1 );
							}
							else if ( !tln->is_expanded && item_index < old_first_visible_index )
							{
								--first_visible_index;
							}
							else	// The first visible is a child node.
							{
								first_visible_index -= ( old_first_visible_index - parent_index );

								item_count -= ( ( tln->child_count - ( old_first_visible_index - parent_index ) ) + 1 );
							}
						}
						else
						{
							if ( first_visible_index > 0 )
							{
								--first_visible_index;

								if ( tln->is_expanded )
								{
									if ( tln->child_count > first_visible_index )
									{
										first_visible_index = 0;
									}
									else
									{
										first_visible_index -= tln->child_count;
									}
								}

								--first_visible_root_index;
							}
						}
					}
					else// if ( item_index >= old_first_visible_index )
					{
						int sel_item_count = ( tln->is_expanded ? tln->child_count : 0 ) + 1;

						int remaining_items = item_count - old_first_visible_index - sel_item_count;

						// We can remove the item and the list will move down. (We're at the end of the list).
						if ( remaining_items < visible_item_count )
						{
							sel_item_count = visible_item_count - remaining_items;

							for ( ; sel_item_count > 0 && first_visible_index > 0; --sel_item_count, --first_visible_index )
							{
								if ( first_visible_node->parent == NULL )
								{
									--first_visible_root_index;
								}

								first_visible_node = TLV_PrevNode( first_visible_node, false );
							}
						}
						else	// We can remove the item and the list will move up.
						{
							// Adjust this so that if the remaining items cause us to be at the end of the list, then it'll hit the if block above.
							item_count -= sel_item_count;

							if ( tln == first_visible_node )
							{
								if ( first_visible_node->parent != NULL )
								{
									first_visible_node = first_visible_node->parent->next;
								}
								else
								{
									first_visible_node = first_visible_node->next;
								}
							}
							else if ( tln == first_visible_node->parent )	// We're a child item.
							{
								int parent_index = TLV_GetParentIndex( first_visible_node, old_first_visible_index );

								first_visible_index -= ( ( old_first_visible_index - parent_index ) );

								first_visible_node = tln->next;
							}
						}
					}
				}

				TREELISTNODE *del_tln = tln;

				++item_index;	// Include the parent.
				if ( tln->is_expanded )
				{
					item_index += tln->child_count;
				}

				tln = tln->next;

				TLV_RemoveNode( &g_tree_list, del_tln );

				total_item_count += TLV_FreeTree( del_tln );
			}
			else
			{
				if ( tln->flag & TLVS_SELECTED )
				{
					last_selection_node = tln;
					last_selection_index = ( item_index - selected_count );

					if ( first_selection_node == NULL )
					{
						first_selection_node = last_selection_node;
						first_selection_index = last_selection_index;
					}
				}

				++item_index;	// Include the parent.
				if ( tln->is_expanded )
				{
					item_index += tln->child_count;
				}

				tln = tln->next;
			}
		}

		// Update our internal values.
		if ( expanded_item_count == old_item_count )
		{
			first_visible_node = NULL;
		}
		TLV_SetFirstVisibleRootIndex( first_visible_root_index );
		TLV_SetFirstVisibleItem( first_visible_node );
		TLV_SetFirstVisibleIndex( first_visible_index );
		TLV_AddTotalItemCount( -total_item_count );
		TLV_AddExpandedItemCount( -expanded_item_count );
		TLV_AddRootItemCount( -root_item_count );
		TLV_AddSelectedCount( -selected_count );
		TLV_ResetSelectionBounds();
		TLV_SetSelectionBounds( first_selection_index, first_selection_node );
		TLV_SetSelectionBounds( last_selection_index, last_selection_node );

		if ( g_tree_list != NULL )
		{
			TLV_SetSelectionBounds( 0, g_tree_list );
			TLV_SetSelectionBounds( TLV_GetExpandedItemCount() - 1, g_tree_list->prev );
		}

		g_download_history_changed = true;

		_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, FALSE, NULL );
		UpdateSBItemCount();
	}
	else if ( handle_type == 3 )	// Restart selected download (from the beginning).
	{
		TREELISTNODE *tln = TLV_GetFocusedItem();
		if ( tln != NULL )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
				EnterCriticalSection( &di->di_cs );

				// Ensure that there are no active parts downloading.
				if ( di->active_parts == 0 )
				{
					bool is_group = ( tln->data_type == ( TLVDT_GROUP | TLVDT_HOST ) ? false : true );
					DOWNLOAD_INFO *t_di = ( DOWNLOAD_INFO * )di->shared_info->host_list->data;

#ifdef ENABLE_LOGGING
					GenericLogEntry( t_di, LOG_INFO_ACTION, "Restarting" );
#endif

					g_download_history_changed = true;

					DoublyLinkedList *host_node = di->shared_info->host_list;
					while ( host_node != NULL )
					{
						DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
						if ( host_di != NULL )
						{
							ResetDownload( host_di, START_TYPE_HOST );

							// Last child, restart group using the first host in the list.
							if ( is_group && host_node->next == NULL )
							{
								ResetDownload( host_di, START_TYPE_GROUP );
							}
						}

						host_node = host_node->next;
					}

					// di->shared_info->downloaded will have been set to 0 above. It shouldn't get into a recursive loop.
					StartDownload( t_di, START_TYPE_GROUP, START_OPERATION_NONE );
				}

				LeaveCriticalSection( &di->di_cs );
			}
		}
	}

	LeaveCriticalSection( &cleanup_cs );

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_connection( void *pArguments )
{
	unsigned int status = ( unsigned int )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	bool process_all = false;

	TREELISTNODE *tln_last_parent = NULL;

	TREELISTNODE *tln;
	if ( status == UINT_MAX )
	{
		process_all = true;

		status = STATUS_DOWNLOADING;	// Start / Resume all downloads.

		tln = g_tree_list;
	}
	else
	{
		TLV_GetNextSelectedItem( NULL, 0, &tln );
	}

	while ( tln != NULL )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		unsigned int tmp_status;

		TREELISTNODE *tln_parent = tln;
		// The last host whose status is not completed.
		// We need this to know when to start group downloads.
		// Not used for restarting downloads.
		TREELISTNODE *tln_last_host = NULL;
		bool is_group = false;
		bool is_host_in_group = false;
		unsigned int parent_status = STATUS_NONE;

		// We'll go through each child regardless of whether it's selected if a group is selected.
		if ( tln->data_type == TLVDT_GROUP && tln->child != NULL )
		{
			if ( tln->data != NULL )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;

				parent_status = di->status;
			}

			tln = tln->child;

			tln_last_host = tln;

			do
			{
				if ( tln_last_host->prev != NULL )
				{
					tln_last_host = tln_last_host->prev;
				}

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln_last_host->data;
				if ( di->status != STATUS_COMPLETED )
				{
					break;
				}
			}
			while ( tln_last_host != tln );

			is_group = true;
		}
		else if ( tln->data_type == TLVDT_HOST )	// One or more children in a group is selected (but not the group item).
		{
			// Determine the status for all the children in the group that aren't selected.
			if ( tln_last_parent != tln->parent && tln->parent != NULL && tln->parent->data != NULL )
			{
				tln_last_parent = tln->parent;
			}

			is_host_in_group = true;
		}

		do
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			EnterCriticalSection( &cleanup_cs );

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
			if ( di != NULL )
			{
#ifdef ENABLE_LOGGING
				if ( di->status != STATUS_COMPLETED || status == STATUS_RESTART )
				{
					char log_status1[ 256 ];
					char log_status2[ 256 ];
					GetDownloadStatus( log_status1, 256, di->status );
					GetDownloadStatus( log_status2, 256, status );
					wchar_t *l_file_path;
					wchar_t t_l_file_path[ MAX_PATH ];
					bool is_temp = false;
					if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE )
					{
						l_file_path = L"Simulated";
					}
					else
					{
						if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
						else { GetDownloadFilePath( di, t_l_file_path ); }
						l_file_path = t_l_file_path;
					}
					WriteLog( LOG_INFO_CON_STATE, "Setting download status: %s -> %s | %s%S | %s%S", log_status1, log_status2, ( is_group ? "group | " : "" ), di->url, ( is_temp ? "temp | " : "" ), l_file_path );
				}
#endif

				EnterCriticalSection( &di->di_cs );

				// Make sure we're not already in the process of shutting down or closing the connection.
				if ( di->status != status )
				{
					if ( di->status != STATUS_COMPLETED )
					{
						DoublyLinkedList *context_node = di->parts_list;
						SOCKET_CONTEXT *context;

						// Make sure the status is exclusively connecting or downloading.
						if ( di->status == STATUS_CONNECTING ||
							 di->status == STATUS_DOWNLOADING )
						{
							if ( status == STATUS_STOPPED ||
								 status == STATUS_RESTART )	// Stop (close) the active connection.
							{
								// Restarts from the CleanupConnection function.
								if ( status == STATUS_RESTART )
								{
									SetStatus( di, STATUS_STOPPED | STATUS_RESTART );

									tmp_status = di->status;

									if ( is_group )
									{
										EnterCriticalSection( &di->shared_info->di_cs );
										SetStatus( di->shared_info, di->status );
										LeaveCriticalSection( &di->shared_info->di_cs );
									}
								}
								else
								{
									tmp_status = STATUS_STOPPED;
								}

								LeaveCriticalSection( &di->di_cs );

								// di->status will be set to STATUS_STOPPED in CleanupConnection().
								while ( context_node != NULL )
								{
									context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->status = tmp_status;

										if ( context->cleanup == 0 )
										{
											if ( context->ssh != NULL )
											{
												// We've initiated a clean shutdown of the SSH connection, but it's stuck.
												if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
												{
													// Force close the socket to release the operation.
													if ( context->socket != INVALID_SOCKET )
													{
														SOCKET s = context->socket;
														context->socket = INVALID_SOCKET;
														_shutdown( s, SD_BOTH );
														_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
													}
												}
												else
												{
													context->overlapped_close.current_operation = IO_SFTPCleanup;
												}
											}
											else
											{
												context->cleanup = 2;	// Force the cleanup.

												InterlockedIncrement( &context->pending_operations );
												context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
												PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
											}
										}
										else if ( context->cleanup == 12 )
										{
											if ( context->ssh == NULL )
											{
												// We've initiated a clean shutdown of the connection, but it's stuck.
												if ( context->overlapped_close.current_operation == IO_Write )
												{
													// Force close the socket to release the operation.
													if ( context->socket != INVALID_SOCKET )
													{
														SOCKET s = context->socket;
														context->socket = INVALID_SOCKET;
														_shutdown( s, SD_BOTH );
														_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
													}
												}
											}
										}

										LeaveCriticalSection( &context->context_cs );
									}
								}
							}
							else if ( status == STATUS_PAUSED )
							{
								SetStatus( di, di->status | STATUS_PAUSED );

								tmp_status = di->status;

								LeaveCriticalSection( &di->di_cs );

								while ( context_node != NULL )
								{
									context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->is_paused = false;	// Set to true when last IO operation has completed.

										context->status = tmp_status;

										LeaveCriticalSection( &context->context_cs );
									}
								}
							}
							else
							{
								LeaveCriticalSection( &di->di_cs );
							}
							/*else
							{
								SetStatus( di, status );

								LeaveCriticalSection( &di->di_cs );

								while ( context_node != NULL )
								{
									context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->status = status;

										LeaveCriticalSection( &context->context_cs );
									}
								}
							}*/
						}
						else if ( IS_STATUS( di->status,
									 STATUS_PAUSED |
									 STATUS_QUEUED ) )	// The download is currently paused, or queued.
						{
							if ( status == STATUS_DOWNLOADING ||
								 status == STATUS_RESTART )	// Resume downloading or restart download.
							{
								// Download is active, continue where we left off.
								if ( di->download_node.data != NULL )
								{
									// Restarts from the CleanupConnection function.
									if ( status == STATUS_RESTART )
									{
										SetStatus( di, STATUS_STOPPED | STATUS_RESTART );

										if ( is_group )
										{
											EnterCriticalSection( &di->shared_info->di_cs );
											SetStatus( di->shared_info, di->status );
											LeaveCriticalSection( &di->shared_info->di_cs );
										}
									}
									else
									{
										SetStatus( di, di->status & ~STATUS_PAUSED );
									}

									tmp_status = di->status;

									LeaveCriticalSection( &di->di_cs );

									// Run through our parts list and connect to each context.
									while ( context_node != NULL )
									{
										context = ( SOCKET_CONTEXT * )context_node->data;

										context_node = context_node->next;

										if ( context != NULL )
										{
											EnterCriticalSection( &context->context_cs );

											context->status = tmp_status;

											// The paused operation has not completed or it has and is_paused is waiting to be set (when the completion fails).
											// We'll fall through in IOCPConnection.
											//if ( !( context->status == STATUS_CONNECTING && !context->is_paused ) )
											if ( context->is_paused )
											{
												context->is_paused = false;	// Reset.

												InterlockedIncrement( &context->pending_operations );
												// Post a completion status to the completion port that we're going to continue with whatever it left off at.
												PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
											}

											LeaveCriticalSection( &context->context_cs );
										}
									}
								}
								else
								{
									if ( IS_STATUS( parent_status, STATUS_QUEUED ) || di->queue_node.data != NULL )	// Download is not active, attempt to resume or queue.
									{
										if ( g_total_downloading < cfg_max_downloads )
										{
											EnterCriticalSection( &download_queue_cs );

											if ( IS_STATUS( parent_status, STATUS_QUEUED ) || download_queue != NULL )
											{
												DLL_RemoveNode( &download_queue, &di->queue_node );
												di->queue_node.data = NULL;

												if ( is_group )
												{
													if ( status == STATUS_RESTART )
													{
														ResetDownload( di, START_TYPE_HOST );

														// Restart group using the first host in the list.
														if ( tln->next == NULL )
														{
															ResetDownload( di, START_TYPE_GROUP );

															// If we've restarted an active download, then we'll restart the group in the ConnectionCleanup().
															if ( IS_STATUS_NOT( di->shared_info->status, STATUS_STOPPED | STATUS_RESTART ) )
															{
																EnterCriticalSection( &di->shared_info->di_cs );
																di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
																LeaveCriticalSection( &di->shared_info->di_cs );

																StartDownload( ( DOWNLOAD_INFO * )di->shared_info->host_list->data, START_TYPE_GROUP, START_OPERATION_NONE );
															}
														}
													}
													else
													{
														ResetDownload( di, START_TYPE_NONE );

														SetStatus( di, STATUS_NONE );	// It's not queued anymore.

														if ( tln == tln_last_host )
														{
															DOWNLOAD_INFO *driver_di = NULL;

															// Find the first host that can act as a driver.
															DoublyLinkedList *host_node = di->shared_info->host_list;

															// Remove the Add Stopped flag if the entire group was added in the stopped state.
															if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
															{
																/*while ( host_node != NULL )
																{
																	DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
																	if ( host_di != NULL )
																	{
																		host_di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
																	}
																	host_node = host_node->next;
																}

																LeaveCriticalSection( &di->di_cs );*/

																EnterCriticalSection( &di->shared_info->di_cs );
																di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
																LeaveCriticalSection( &di->shared_info->di_cs );

																//EnterCriticalSection( &di->di_cs );
															}

															host_node = di->shared_info->host_list;
															while ( host_node != NULL )
															{
																DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
																if ( host_di->status != STATUS_COMPLETED &&
																  !( host_di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) )
																{
																	driver_di = ( DOWNLOAD_INFO * )host_node->data;

																	break;
																}
																host_node = host_node->next;
															}

															if ( driver_di != NULL )
															{
																StartDownload( driver_di, START_TYPE_GROUP, START_OPERATION_NONE );
															}
															else
															{
																/*// Remove the Add Stopped flag.
																host_node = di->shared_info->host_list;
																while ( host_node != NULL )
																{
																	DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
																	if ( host_di != NULL )
																	{
																		host_di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
																	}
																	host_node = host_node->next;
																}

																LeaveCriticalSection( &di->di_cs );*/

																EnterCriticalSection( &di->shared_info->di_cs );
																SetStatus( di->shared_info, STATUS_STOPPED );
																LeaveCriticalSection( &di->shared_info->di_cs );

																//EnterCriticalSection( &di->di_cs );
															}
														}
													}
												}
												else
												{
													//LeaveCriticalSection( &di->di_cs );

													EnterCriticalSection( &di->shared_info->di_cs );
													di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
													LeaveCriticalSection( &di->shared_info->di_cs );

													//EnterCriticalSection( &di->di_cs );

													SetStatus( di, STATUS_NONE );

													if ( status == STATUS_RESTART )
													{
														EnterCriticalSection( &di->shared_info->di_cs );
														di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
														LeaveCriticalSection( &di->shared_info->di_cs );
													}

													RestartDownload( di, ( status == STATUS_RESTART ? ( is_host_in_group ? START_TYPE_HOST_IN_GROUP : START_TYPE_HOST ) : START_TYPE_NONE ), START_OPERATION_NONE );
												}
											}

											LeaveCriticalSection( &download_queue_cs );
										}
										else
										{
											if ( status == STATUS_RESTART )
											{
												tmp_status = di->status;

												ResetDownload( di, ( is_host_in_group ? START_TYPE_HOST_IN_GROUP : START_TYPE_HOST ) );

												SetStatus( di, tmp_status );
											}
										}
										/*else
										{
											SetStatus( di, di->status | STATUS_QUEUED );	// Queued.
										}*/
									}

									LeaveCriticalSection( &di->di_cs );
								}
							}
							else if ( status == STATUS_STOPPED )	// Stop (close) the active connection.
							{
								// Download is active, close the connection.
								if ( di->download_node.data != NULL )
								{
									LeaveCriticalSection( &di->di_cs );

									// di->status will be set to STATUS_STOPPED in CleanupConnection().
									while ( context_node != NULL )
									{
										context = ( SOCKET_CONTEXT * )context_node->data;

										context_node = context_node->next;

										SetContextStatus( context, STATUS_STOPPED );
									}
								}
								else
								{
									// Only allow it to be stopped (when resuming) if the header has not been processed.
									// This would be like adding a new download.
									if ( !di->processed_header )
									{
										// If it's a group, then remove the Add Stopped flag.
										if ( is_group )
										{
											di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
										}
										else
										{
											di->download_operations |= DOWNLOAD_OPERATION_ADD_STOPPED;
										}
									}
									SetStatus( di, STATUS_STOPPED );
									if ( is_group && tln == tln_last_host )
									{
										//LeaveCriticalSection( &di->di_cs );

										EnterCriticalSection( &di->shared_info->di_cs );
										SetStatus( di->shared_info, STATUS_STOPPED );
										LeaveCriticalSection( &di->shared_info->di_cs );

										//EnterCriticalSection( &di->di_cs );
									}

									if ( di->queue_node.data != NULL )	// Download is queued.
									{
										EnterCriticalSection( &download_queue_cs );

										// Remove the item from the download queue.
										DLL_RemoveNode( &download_queue, &di->queue_node );
										di->queue_node.data = NULL;

										LeaveCriticalSection( &download_queue_cs );
									}

									LeaveCriticalSection( &di->di_cs );
								}
							}
							else
							{
								LeaveCriticalSection( &di->di_cs );
							}
							/*else
							{
								SetStatus( di, status );

								LeaveCriticalSection( &di->di_cs );

								while ( context_node != NULL )
								{
									context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->status = status;

										LeaveCriticalSection( &context->context_cs );
									}
								}
							}*/
						}
						else if ( IS_STATUS( di->status,
									 STATUS_STOPPED |
									 STATUS_TIMED_OUT |
									 STATUS_FAILED |
									 STATUS_FILE_IO_ERROR |
									 STATUS_SKIPPED |
									 STATUS_AUTH_REQUIRED |
									 STATUS_PROXY_AUTH_REQUIRED |
									 STATUS_INSUFFICIENT_DISK_SPACE ) )	// The download is currently stopped.
						{
							// If this is true, then we've attempted to restart before a connection operation has completed.
							if ( IS_STATUS( di->status, STATUS_RESTART ) && status == STATUS_STOPPED )
							{
								SetStatus( di, status );

								LeaveCriticalSection( &di->di_cs );

								while ( context_node != NULL )
								{
									context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->status = status;

										if ( context->cleanup == 0 )
										{
											context->cleanup = 1;	// Auto cleanup.
										}

										// The operation is probably stuck (server isn't responding). Force close the socket to release the operation.
										if ( context->pending_operations > 0 )
										{
											if ( context->socket != INVALID_SOCKET )
											{
												SOCKET s = context->socket;
												context->socket = INVALID_SOCKET;
												_shutdown( s, SD_BOTH );
												_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
											}
										}

										LeaveCriticalSection( &context->context_cs );
									}
								}
							}
							else if ( IS_STATUS_NOT( status, STATUS_PAUSED | STATUS_STOPPED ) )
							{
								// Ensure that the download is actually stopped and that there are no active parts downloading.
								if ( di->active_parts == 0 )
								{
									g_download_history_changed = true;

									if ( is_group )
									{
										if ( status == STATUS_RESTART )
										{
											ResetDownload( di, START_TYPE_HOST );

											// Restart group using the first host in the list.
											if ( tln->next == NULL )
											{
												ResetDownload( di, START_TYPE_GROUP );

												// If we've restarted an active download, then we'll restart the group in the ConnectionCleanup().
												if ( IS_STATUS_NOT( di->shared_info->status, STATUS_STOPPED | STATUS_RESTART ) )
												{
													EnterCriticalSection( &di->shared_info->di_cs );
													di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
													LeaveCriticalSection( &di->shared_info->di_cs );

													StartDownload( ( DOWNLOAD_INFO * )di->shared_info->host_list->data, START_TYPE_GROUP, ( parent_status == STATUS_SKIPPED ? ( START_OPERATION_CHECK_FILE | START_OPERATION_FORCE_PROMPT ) : START_OPERATION_NONE ) );
												}
											}
										}
										else
										{
											// This is the case where we start a group download and skip it before we call MakeRangeRequest().
											if ( !di->shared_info->processed_header && di->processed_header )
											{
												ResetDownload( di, START_TYPE_HOST );
											}
											else
											{
												ResetDownload( di, START_TYPE_NONE );
											}

											// Host has already been processed (assigned range info). Resume it.
											if ( di->shared_info->processed_header )
											{
												// If the group has already been processed, then each host has its own file size.
												// If it's 0, then it was skipped.
												if ( di->file_size == 0 )
												{
													SetStatus( di, STATUS_SKIPPED );
												}
												else
												{
													tmp_status = di->status;
													SetStatus( di, STATUS_NONE );

													StartDownload( di, START_TYPE_HOST_IN_GROUP, START_OPERATION_NONE/*( tmp_status == STATUS_SKIPPED ? START_OPERATION_CHECK_FILE : START_OPERATION_NONE )*/ );
												}
											}
											else	// We're starting an unprocessed group, or host in group.
											{
												// Start the group.
												if ( tln == tln_last_host )
												{
													DOWNLOAD_INFO *driver_di = NULL;

													// Find the first host that can act as a driver.
													DoublyLinkedList *host_node = di->shared_info->host_list;

													// Remove the Add Stopped flag if the entire group was added in the stopped state.
													if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
													{
														/*while ( host_node != NULL )
														{
															DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
															if ( host_di != NULL )
															{
																host_di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
															}
															host_node = host_node->next;
														}

														LeaveCriticalSection( &di->di_cs );*/

														EnterCriticalSection( &di->shared_info->di_cs );
														di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
														LeaveCriticalSection( &di->shared_info->di_cs );

														//EnterCriticalSection( &di->di_cs );
													}

													host_node = di->shared_info->host_list;
													while ( host_node != NULL )
													{
														DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
														if ( host_di->status != STATUS_COMPLETED &&
														  !( host_di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) )
														{
															driver_di = ( DOWNLOAD_INFO * )host_node->data;

															break;
														}
														host_node = host_node->next;
													}

													if ( driver_di != NULL )
													{
														//tmp_status = driver_di->status;
														//SetStatus( driver_di, STATUS_NONE );
														tmp_status = di->shared_info->status;

														SetStatus( di, STATUS_NONE );

														StartDownload( driver_di, START_TYPE_GROUP, ( tmp_status == STATUS_SKIPPED || IS_STATUS( parent_status, STATUS_QUEUED ) ? START_OPERATION_CHECK_FILE : START_OPERATION_NONE ) );
													}
													else
													{
														SetStatus( di, STATUS_NONE );

														/*// Remove the Add Stopped flag.
														host_node = di->shared_info->host_list;
														while ( host_node != NULL )
														{
															DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
															if ( host_di != NULL )
															{
																host_di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
															}
															host_node = host_node->next;
														}

														LeaveCriticalSection( &di->di_cs );*/

														EnterCriticalSection( &di->shared_info->di_cs );
														SetStatus( di->shared_info, STATUS_STOPPED );
														LeaveCriticalSection( &di->shared_info->di_cs );

														//EnterCriticalSection( &di->di_cs );
													}
												}
												else
												{
													SetStatus( di, STATUS_NONE );
												}
											}
										}
									}
									else
									{
										//LeaveCriticalSection( &di->di_cs );

										EnterCriticalSection( &di->shared_info->di_cs );
										di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
										LeaveCriticalSection( &di->shared_info->di_cs );

										//EnterCriticalSection( &di->di_cs );

										// Revert the status back to queued if the group is queued.
										if ( IS_STATUS( di->shared_info->status, STATUS_QUEUED ) )
										{
											di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
											SetStatus( di, STATUS_CONNECTING | STATUS_QUEUED );
										}
										else if ( is_host_in_group && di->shared_info->processed_header && di->file_size == 0 )
										{
											SetStatus( di, STATUS_SKIPPED );
										}
										else
										{
											tmp_status = di->status;
											SetStatus( di, STATUS_NONE );

											if ( status == STATUS_RESTART )
											{
												EnterCriticalSection( &di->shared_info->di_cs );
												di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
												LeaveCriticalSection( &di->shared_info->di_cs );
											}

											RestartDownload( di,
														   ( status == STATUS_RESTART ? ( is_host_in_group ? START_TYPE_HOST_IN_GROUP : START_TYPE_HOST ) : START_TYPE_NONE ),
														   ( tmp_status == STATUS_SKIPPED && ( !is_host_in_group || ( is_host_in_group && !di->shared_info->processed_header ) ) ? ( status == STATUS_RESTART ? ( START_OPERATION_CHECK_FILE | START_OPERATION_FORCE_PROMPT ) : START_OPERATION_CHECK_FILE ) : START_OPERATION_NONE ) );
										}
									}
								}

								LeaveCriticalSection( &di->di_cs );
							}
							else
							{
								LeaveCriticalSection( &di->di_cs );
							}
						}
						else if ( di->status == STATUS_MOVING_FILE )
						{
							if ( status == STATUS_STOPPED )
							{
								EnterCriticalSection( &di->shared_info->di_cs );
								di->shared_info->moving_state = 2;	// Cancel.
								LeaveCriticalSection( &di->shared_info->di_cs );
							}

							LeaveCriticalSection( &di->di_cs );
						}
						else
						{
							LeaveCriticalSection( &di->di_cs );
						}
						/*else
						{
							SetStatus( di, status );

							LeaveCriticalSection( &di->di_cs );

							while ( context_node != NULL )
							{
								context = ( SOCKET_CONTEXT * )context_node->data;

								context_node = context_node->next;

								if ( context != NULL )
								{
									EnterCriticalSection( &context->context_cs );

									context->status = status;

									LeaveCriticalSection( &context->context_cs );
								}
							}
						}*/
					}
					else
					{
						if ( status == STATUS_RESTART )
						{
							// Ensure that there are no active parts downloading.
							if ( di->active_parts == 0 )
							{
								g_download_history_changed = true;

								if ( is_group )
								{
									ResetDownload( di, START_TYPE_HOST );

									// Restart group using the first host in the list.
									if ( tln->next == NULL )
									{
										ResetDownload( di, START_TYPE_GROUP );

										// If we've restarted an active download, then we'll restart the group in the ConnectionCleanup().
										if ( IS_STATUS_NOT( di->shared_info->status, STATUS_STOPPED | STATUS_RESTART ) )
										{
											EnterCriticalSection( &di->shared_info->di_cs );
											di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
											LeaveCriticalSection( &di->shared_info->di_cs );

											StartDownload( ( DOWNLOAD_INFO * )di->shared_info->host_list->data, START_TYPE_GROUP, START_OPERATION_NONE );
										}
									}
								}
								else
								{
									EnterCriticalSection( &di->shared_info->di_cs );
									di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESTARTING;
									LeaveCriticalSection( &di->shared_info->di_cs );

									RestartDownload( di, ( is_host_in_group ? START_TYPE_HOST_IN_GROUP : START_TYPE_HOST ), START_OPERATION_NONE );
								}
							}
						}

						LeaveCriticalSection( &di->di_cs );
					}
				}
				else
				{
					if ( status == STATUS_STOPPED )
					{
						if ( is_group )
						{
							// Remove the Add Stopped flag.
							di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;

							//LeaveCriticalSection( &di->di_cs );

							EnterCriticalSection( &di->shared_info->di_cs );
							di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
							SetStatus( di->shared_info, STATUS_STOPPED );
							LeaveCriticalSection( &di->shared_info->di_cs );

							//EnterCriticalSection( &di->di_cs );
						}
					}

					LeaveCriticalSection( &di->di_cs );
				}
			}

			LeaveCriticalSection( &cleanup_cs );

			tln = tln->next;
		}
		while ( tln_parent->child != NULL && tln != NULL );

		tln = tln_parent;

		if ( process_all )
		{
			tln = tln->next;
		}
		else
		{
			TLV_GetNextSelectedItem( tln, 0, &tln );
		}
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_download_queue( void *pArguments )
{
	unsigned char handle_type = ( unsigned char )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( handle_type == 4 )
	{
		EnterCriticalSection( &download_queue_cs );

		if ( download_queue != NULL )
		{
#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Saving reordered queue" );
#endif
			download_queue = NULL;

			TREELISTNODE *tln = g_tree_list;

			while ( tln != NULL && !g_end_program )
			{
				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL && IS_STATUS( di->status, STATUS_QUEUED ) )
				{
					di->queue_node.next = NULL;
					di->queue_node.prev = NULL;

					DLL_AddNode( &download_queue, &di->queue_node, -1 );
				}

				tln = tln->next;
			}
		}

		LeaveCriticalSection( &download_queue_cs );

		if ( !g_end_program )
		{
			// Start any items that are in our download queue.
			if ( g_total_downloading < cfg_max_downloads )
			{
				StartQueuedItem();
			}
		}

		g_download_history_changed = true;
	}
	else
	{
		EnterCriticalSection( &cleanup_cs );

		TREELISTNODE *tln;
		TLV_GetNextSelectedItem( NULL, 0, &tln );
		if ( tln != NULL )
		{
			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;

			// Make sure the item is queued.
			if ( di != NULL && IS_STATUS( di->status, STATUS_QUEUED ) )
			{
				CRITICAL_SECTION *cs;
				DoublyLinkedList **queue;

				if ( IS_STATUS( di->status, STATUS_MOVING_FILE ) )
				{
					cs = &move_file_queue_cs;
					queue = &move_file_queue;
				}
				else
				{
					cs = &download_queue_cs;
					queue = &download_queue;
				}

				EnterCriticalSection( cs );

				if ( di->queue_node.data != NULL )
				{
					if ( handle_type == 0 )			// Move to the beginning of the queue.
					{
						// Make sure we're not the head.
						if ( &di->queue_node != *queue )
						{
							DLL_RemoveNode( queue, &di->queue_node );
							DLL_AddNode( queue, &di->queue_node, 0 );
						}
					}
					else if ( handle_type == 1 )	// Move forward one position in the queue.
					{
						// Make sure we're not the head.
						if ( &di->queue_node != *queue )
						{
							DoublyLinkedList *prev = di->queue_node.prev;
							if ( prev != NULL )
							{
								DLL_RemoveNode( queue, &di->queue_node );

								// If the node we're replacing is the head.
								if ( prev == *queue )
								{
									DLL_AddNode( queue, &di->queue_node, 0 );
								}
								else
								{
									di->queue_node.next = prev;
									di->queue_node.prev = prev->prev;

									if ( prev->prev != NULL )
									{
										prev->prev->next = &di->queue_node;
									}
									prev->prev = &di->queue_node;
								}
							}
						}
					}
					else if ( handle_type == 2 )	// Move back one position in the queue.
					{
						DoublyLinkedList *next = di->queue_node.next;
						if ( next != NULL )
						{
							DLL_RemoveNode( queue, &di->queue_node );

							// If the node we're replacing is the tail.
							if ( next->next == NULL )
							{
								DLL_AddNode( queue, &di->queue_node, -1 );
							}
							else
							{
								di->queue_node.prev = next;
								di->queue_node.next = next->next;

								if ( next->next != NULL )
								{
									next->next->prev = &di->queue_node;
								}
								next->next = &di->queue_node;
							}
						}
					}
					else if ( handle_type == 3 )	// Move to the end of the queue.
					{
						DLL_RemoveNode( queue, &di->queue_node );
						DLL_AddNode( queue, &di->queue_node, -1 );
					}
				}

				LeaveCriticalSection( cs );
			}
		}

		LeaveCriticalSection( &cleanup_cs );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_download_update( void *pArguments )
{
	ADD_INFO *ai = ( ADD_INFO * )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	EnterCriticalSection( &cleanup_cs );

	if ( ai != NULL )	// Update selected download (stops and resumes the download).
	{
		DOWNLOAD_INFO *di = g_update_download_info;
		if ( di != NULL )
		{
#ifdef ENABLE_LOGGING
			wchar_t *l_file_path;
			wchar_t t_l_file_path[ MAX_PATH ];
			bool is_temp = false;
			if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE )
			{
				l_file_path = L"Simulated";
			}
			else
			{
				if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
				else { GetDownloadFilePath( di, t_l_file_path ); }
				l_file_path = t_l_file_path;
			}

			if ( di == di->shared_info && di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
			{
				WriteLog( LOG_INFO_ACTION, "Updating: group | %S", l_file_path );
			}
			else
			{
				WriteLog( LOG_INFO_ACTION, "Updating: %s%S | %s%S", ( IS_GROUP( di ) ? "group | " : "" ), di->url, ( is_temp ? "temp | " : "" ), l_file_path );
			}
#endif
			EnterCriticalSection( &di->di_cs );

			di->download_speed_limit = ai->download_speed_limit;

			bool proxy_changed = false;

			if ( di->proxy_info == NULL && ai->proxy_info.type != 0 )
			{
				di->proxy_info = ( PROXY_INFO * )GlobalAlloc( GPTR, sizeof( PROXY_INFO ) );

				di->proxy_info->type = ai->proxy_info.type;
				di->proxy_info->ip_address = ai->proxy_info.ip_address;
				di->proxy_info->port = ai->proxy_info.port;
				di->proxy_info->address_type = ai->proxy_info.address_type;
				di->proxy_info->use_authentication = ai->proxy_info.use_authentication;
				di->proxy_info->resolve_domain_names = ai->proxy_info.resolve_domain_names;

				di->proxy_info->hostname = ai->proxy_info.hostname;
				ai->proxy_info.hostname = NULL;

				di->proxy_info->punycode_hostname = ai->proxy_info.punycode_hostname;
				ai->proxy_info.punycode_hostname = NULL;

				di->proxy_info->w_username = ai->proxy_info.w_username;
				ai->proxy_info.w_username = NULL;

				di->proxy_info->w_password = ai->proxy_info.w_password;
				ai->proxy_info.w_password = NULL;

				di->proxy_info->username = ai->proxy_info.username;
				ai->proxy_info.username = NULL;

				di->proxy_info->password = ai->proxy_info.password;
				ai->proxy_info.password = NULL;

				if ( ( ai->proxy_info.type == 1 || ai->proxy_info.type == 2 ) && ( di->proxy_info->username != NULL && di->proxy_info->password != NULL ) )
				{
					int proxy_username_length = lstrlenA( di->proxy_info->username );
					int proxy_password_length = lstrlenA( di->proxy_info->password );

					CreateBasicAuthorizationKey( di->proxy_info->username, proxy_username_length, di->proxy_info->password, proxy_password_length, &di->proxy_info->auth_key, &di->proxy_info->auth_key_length );
				}
				else
				{
					di->proxy_info->auth_key = NULL;
					di->proxy_info->auth_key_length = 0;
				}

				proxy_changed = true;
			}
			else if ( di->proxy_info != NULL &&
					( di->proxy_info->type != ai->proxy_info.type ||
					  di->proxy_info->ip_address != ai->proxy_info.ip_address ||
					  di->proxy_info->port != ai->proxy_info.port ||
					  di->proxy_info->address_type != ai->proxy_info.address_type ||
					  di->proxy_info->use_authentication != ai->proxy_info.use_authentication ||
					  di->proxy_info->resolve_domain_names != ai->proxy_info.resolve_domain_names ||
					  ( _StrCmpW( di->proxy_info->hostname, ai->proxy_info.hostname ) != 0 ) ||
					  ( _StrCmpW( di->proxy_info->punycode_hostname, ai->proxy_info.punycode_hostname ) != 0 ) ||
					  ( _StrCmpW( di->proxy_info->w_username, ai->proxy_info.w_username ) != 0 ) ||
					  ( _StrCmpW( di->proxy_info->w_password, ai->proxy_info.w_password ) != 0 ) ||
					  ( _StrCmpA( di->proxy_info->username, ai->proxy_info.username ) != 0 ) ||
					  ( _StrCmpA( di->proxy_info->password, ai->proxy_info.password ) != 0 ) ) )
			{
				di->proxy_info->type = ai->proxy_info.type;
				di->proxy_info->ip_address = ai->proxy_info.ip_address;
				di->proxy_info->port = ai->proxy_info.port;
				di->proxy_info->address_type = ai->proxy_info.address_type;
				di->proxy_info->use_authentication = ai->proxy_info.use_authentication;
				di->proxy_info->resolve_domain_names = ai->proxy_info.resolve_domain_names;

				// Swap values and free below.
				wchar_t *tmp_ptr_w = di->proxy_info->hostname;
				di->proxy_info->hostname = ai->proxy_info.hostname;
				ai->proxy_info.hostname = tmp_ptr_w;

				tmp_ptr_w = di->proxy_info->punycode_hostname;
				di->proxy_info->punycode_hostname = ai->proxy_info.punycode_hostname;
				ai->proxy_info.punycode_hostname = tmp_ptr_w;

				tmp_ptr_w = di->proxy_info->w_username;
				di->proxy_info->w_username = ai->proxy_info.w_username;
				ai->proxy_info.w_username = tmp_ptr_w;

				tmp_ptr_w = di->proxy_info->w_password;
				di->proxy_info->w_password = ai->proxy_info.w_password;
				ai->proxy_info.w_password = tmp_ptr_w;

				char *tmp_ptr = di->proxy_info->username;
				di->proxy_info->username = ai->proxy_info.username;
				ai->proxy_info.username = tmp_ptr;

				tmp_ptr = di->proxy_info->password;
				di->proxy_info->password = ai->proxy_info.password;
				ai->proxy_info.password = tmp_ptr;

				ai->proxy_info.auth_key = di->proxy_info->auth_key;
				di->proxy_info->auth_key = NULL;
				di->proxy_info->auth_key_length = 0;

				if ( ( ai->proxy_info.type == 1 || ai->proxy_info.type == 2 ) && ( di->proxy_info->username != NULL && di->proxy_info->password != NULL ) )
				{
					int proxy_username_length = lstrlenA( di->proxy_info->username );
					int proxy_password_length = lstrlenA( di->proxy_info->password );

					CreateBasicAuthorizationKey( di->proxy_info->username, proxy_username_length, di->proxy_info->password, proxy_password_length, &di->proxy_info->auth_key, &di->proxy_info->auth_key_length );
				}

				proxy_changed = true;
			}

			bool category_updated = ( _StrCmpW( di->category, ai->category ) != 0 ? true : false );
			if ( category_updated )
			{
				wchar_t *old_category = di->category;
				di->category = CacheCategory( ai->category );
				old_category = RemoveCachedCategory( old_category );

				if ( di->tln != NULL )
				{
					TREELISTNODE *tln = ( TREELISTNODE * )di->tln;
					int child_count = ( tln->is_expanded ? tln->child_count : 0 );

					wchar_t *category_filter = TLV_GetCategoryFilter();

					if ( category_filter != NULL )
					{
						// The download has been added to the currently filtered category.
						if ( di->category == category_filter )
						{
							TLV_AddTotalItemCount( 1 );
							TLV_AddExpandedItemCount( child_count + 1 );
							TLV_AddRootItemCount( 1 );
						}
						else if ( old_category == category_filter )	// The download has been removed from the currently filtered category.
						{
							TLV_AddTotalItemCount( -1 );
							TLV_AddExpandedItemCount( -( child_count + 1 ) );
							TLV_AddRootItemCount( -1 );
						}
					}
				}
			}

			bool file_path_changed = false;
			if ( !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) &&
				ai->download_directory != NULL &&
				_StrCmpW( di->shared_info->file_path, ai->download_directory ) != 0 )
			{
				wchar_t *tmp_wptr = di->shared_info->new_file_path;
				di->shared_info->new_file_path = ai->download_directory;
				ai->download_directory = tmp_wptr;

				// This must be saved so we can resume any active downloads in ProcessMoveQueue().
				di->shared_info->last_status = di->shared_info->status;

				file_path_changed = true;
			}

			if ( _StrCmpW( di->comments, ai->comments ) != 0 )
			{
				wchar_t *tmp_wptr = di->comments;
				di->comments = ai->comments;
				ai->comments = tmp_wptr;
			}

			// These are all values that, if changed, need to cause any active downloads to stop and start again.
			if ( proxy_changed ||
				 file_path_changed ||
				 di->parts_limit != ai->parts ||
			   ( ai->urls != NULL && _StrCmpW( di->url, ai->urls ) != 0 ) ||
				 _StrCmpA( di->cookies, ai->utf8_cookies ) != 0 ||
				 _StrCmpA( di->headers, ai->utf8_headers ) != 0 ||
				 _StrCmpA( di->data, ai->utf8_data ) != 0 ||
				 _StrCmpA( di->auth_info.username, ai->auth_info.username ) != 0 ||
				 _StrCmpA( di->auth_info.password, ai->auth_info.password ) != 0 ||
				 di->ssl_version != ai->ssl_version ||
				 di->method != ai->method )
			{
				if ( proxy_changed )
				{
					if ( di->proxy_info != di->saved_proxy_info )
					{
						FreeProxyInfo( &di->saved_proxy_info );
					}
					di->saved_proxy_info = di->proxy_info;
				}

				// Swap values and free below.
				char *tmp_ptr = di->cookies;
				di->cookies = ai->utf8_cookies;
				ai->utf8_cookies = tmp_ptr;

				tmp_ptr = di->headers;
				di->headers = ai->utf8_headers;
				ai->utf8_headers = tmp_ptr;

				tmp_ptr = di->data;
				di->data = ai->utf8_data;
				ai->utf8_data = tmp_ptr;

				tmp_ptr = di->auth_info.username;
				di->auth_info.username = ai->auth_info.username;
				ai->auth_info.username = tmp_ptr;

				tmp_ptr = di->auth_info.password;
				di->auth_info.password = ai->auth_info.password;
				ai->auth_info.password = tmp_ptr;

				di->ssl_version = ai->ssl_version;
				di->parts_limit = ai->parts;
				di->method = ai->method;

				if ( ai->urls != NULL )
				{
					wchar_t *tmp_ptr_w = di->url;
					di->url = ai->urls;
					ai->urls = tmp_ptr_w;
				}

				if ( di->shared_info->active_hosts == 0 )
				{
					if ( file_path_changed )
					{
						// Enabled for the Moving File progress.
						EnableTimers( true );

						AddToMoveFileQueue( di );
					}

					LeaveCriticalSection( &di->di_cs );
				}
				else
				{
					DoublyLinkedList *host_node = di->shared_info->host_list;

					LeaveCriticalSection( &di->di_cs );

					while ( host_node != NULL )
					{
						DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
						if ( host_di != NULL /*&& host_di != di*/ )
						{
							DoublyLinkedList *context_node = host_di->parts_list;

							// Download is active, close the connection.
							if ( host_di->download_node.data != NULL )
							{
								// host_di->status will be set to STATUS_UPDATING in CleanupConnection().
								while ( context_node != NULL )
								{
									SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									SetContextStatus( context, STATUS_UPDATING | ( file_path_changed ? STATUS_MOVING_FILE : STATUS_NONE ) );
								}
							}
						}

						host_node = host_node->next;
					}
				}
			}
			else
			{
				LeaveCriticalSection( &di->di_cs );
			}

			// Sort only the values that can be updated.
			if ( !g_in_list_edit_mode &&
				 cfg_sort_added_and_updating_items &&
			   ( cfg_sorted_column_index == COLUMN_ACTIVE_PARTS ||
				 cfg_sorted_column_index == COLUMN_CATEGORY ||
				 cfg_sorted_column_index == COLUMN_COMMENTS ||
				 cfg_sorted_column_index == COLUMN_DOWNLOAD_DIRECTORY ||
				 cfg_sorted_column_index == COLUMN_SSL_TLS_VERSION ||
				 cfg_sorted_column_index == COLUMN_URL ) )
			{
				SORT_INFO si;
				si.column = cfg_sorted_column_index;
				si.hWnd = g_hWnd_tlv_files;
				si.direction = cfg_sorted_direction;

				_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&si );
			}

			if ( category_updated )
			{
				UpdateSBItemCount();
			}
		}

		FreeAddInfo( &ai );

		g_download_history_changed = true;
	}

	g_update_download_info = NULL;

	LeaveCriticalSection( &cleanup_cs );

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN copy_urls( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	unsigned int buffer_size = 8192;
	unsigned int buffer_offset = 0;
	wchar_t *copy_buffer = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_size );	// Allocate 8 kilobytes.
	if ( copy_buffer != NULL )
	{
		TREELISTNODE *tln;
		TLV_GetNextSelectedItem( NULL, 0, &tln );
		while ( tln != NULL )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			TREELISTNODE *tln_parent = tln;

			// We'll go through each child regardless of whether it's selected if a group is selected.
			if ( tln->data_type == TLVDT_GROUP && tln->child != NULL )
			{
				tln = tln->child;

				if ( tln->data != NULL )
				{
					*( copy_buffer + buffer_offset ) = L'{';
					++buffer_offset;
					*( copy_buffer + buffer_offset ) = L'\r';
					++buffer_offset;
					*( copy_buffer + buffer_offset ) = L'\n';
					++buffer_offset;
				}
			}

			do
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					// We don't really need to do this since the URL will never change.
					EnterCriticalSection( &di->di_cs );

					int value_length = lstrlenW( di->url );
					while ( buffer_offset + value_length + 8 >= buffer_size )	// Add +8 for "\r\n", "{\r\n", and "\r\n}".
					{
						buffer_size += 8192;
						wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( copy_buffer, sizeof( wchar_t ) * buffer_size, GMEM_MOVEABLE );
						if ( realloc_buffer == NULL )
						{
							LeaveCriticalSection( &di->di_cs );

							goto CLEANUP;
						}

						copy_buffer = realloc_buffer;
					}
					_wmemcpy_s( copy_buffer + buffer_offset, buffer_size - buffer_offset, di->url, value_length );

					LeaveCriticalSection( &di->di_cs );

					buffer_offset += value_length;
				}

				tln = tln->next;

				if ( tln_parent->child != NULL && tln != NULL && tln->data != NULL )
				{
					*( copy_buffer + buffer_offset ) = L'\r';
					++buffer_offset;
					*( copy_buffer + buffer_offset ) = L'\n';
					++buffer_offset;
				}
			}
			while ( tln_parent->child != NULL && tln != NULL );

			tln = tln_parent;

			if ( tln_parent->child != NULL && tln_parent->child->data != NULL )
			{
				*( copy_buffer + buffer_offset ) = L'\r';
				++buffer_offset;
				*( copy_buffer + buffer_offset ) = L'\n';
				++buffer_offset;
				*( copy_buffer + buffer_offset ) = L'}';
				++buffer_offset;

				// Set tln to the last child so that TLV_GetNextSelectedItem can get the next selected.
				tln = ( tln_parent->child->prev != NULL ? tln_parent->child->prev : tln_parent->child );
			}

			TLV_GetNextSelectedItem( tln, 0, &tln );

			if ( tln != NULL && tln->data != NULL )
			{
				*( copy_buffer + buffer_offset ) = L'\r';
				++buffer_offset;
				*( copy_buffer + buffer_offset ) = L'\n';
				++buffer_offset;
			}
		}

		if ( _OpenClipboard( g_hWnd_tlv_files ) )
		{
			_EmptyClipboard();

			DWORD len = buffer_offset;

			// Allocate a global memory object for the text.
			HGLOBAL hglbCopy = GlobalAlloc( GMEM_MOVEABLE, sizeof( wchar_t ) * ( len + 1 ) );
			if ( hglbCopy != NULL )
			{
				// Lock the handle and copy the text to the buffer. lptstrCopy doesn't get freed.
				wchar_t *lptstrCopy = ( wchar_t * )GlobalLock( hglbCopy );
				if ( lptstrCopy != NULL )
				{
					_wmemcpy_s( lptstrCopy, len + 1, copy_buffer, len );
					lptstrCopy[ len ] = 0; // Sanity
				}

				GlobalUnlock( hglbCopy );

				if ( _SetClipboardData( CF_UNICODETEXT, hglbCopy ) == NULL )
				{
					GlobalFree( hglbCopy );	// Only free this Global memory if SetClipboardData fails.
				}

				_CloseClipboard();
			}
		}

CLEANUP:

		GlobalFree( copy_buffer );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN rename_file( void *pArguments )
{
	RENAME_INFO *ri = ( RENAME_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	// If we close the program while the rename edit box is visible, then it might get in here. We don't want that.
	if ( !kill_worker_thread_flag )
	{
		_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, TRUE, NULL );

		ProcessingList( true );

		EnterCriticalSection( &cleanup_cs );

		if ( ri != NULL )
		{
			if ( ri->filename != NULL  )
			{
				DOWNLOAD_INFO *di = ri->di;

				if ( di != NULL )
				{
					if ( di->shared_info->filename_offset > 0 )
					{
						unsigned int old_length = lstrlenW( di->shared_info->file_path + di->shared_info->filename_offset );

						if ( old_length != ri->filename_length || _StrCmpNW( ri->filename, di->shared_info->file_path + di->shared_info->filename_offset, old_length ) != 0 )
						{
							bool renamed = true;

							wchar_t old_file_path[ MAX_PATH ];
							if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED )
							{
								GetTemporaryFilePath( di, old_file_path );
							}
							else
							{
								GetDownloadFilePath( di, old_file_path );
							}
							unsigned int old_filename_offset = di->filename_offset;
							unsigned int old_file_extension_offset = di->file_extension_offset;

							if ( !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
							{
								if ( di->shared_info->hFile != INVALID_HANDLE_VALUE )
								{
									// Make sure SetFileInformationByHandle is available on our system.
									if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
									{
										FILE_RENAME_INFO *fri = ( FILE_RENAME_INFO * )GlobalAlloc( GPTR, sizeof( FILE_RENAME_INFO ) + ( sizeof( wchar_t ) * MAX_PATH ) );

										if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED )
										{
											_wmemcpy_s( fri->FileName, MAX_PATH, cfg_temp_download_directory, g_temp_download_directory_length );
											fri->FileName[ g_temp_download_directory_length ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
											_wmemcpy_s( fri->FileName + ( g_temp_download_directory_length + 1 ), MAX_PATH - ( g_temp_download_directory_length - 1 ), ri->filename, ri->filename_length );
											fri->FileName[ g_temp_download_directory_length + ri->filename_length + 1 ] = 0; // Sanity.
											fri->FileNameLength = g_temp_download_directory_length + ri->filename_length + 1;
										}
										else
										{
											_wmemcpy_s( fri->FileName, MAX_PATH, di->shared_info->file_path, di->shared_info->filename_offset );
											fri->FileName[ di->shared_info->filename_offset - 1 ] = L'\\';
											_wmemcpy_s( fri->FileName + di->shared_info->filename_offset, MAX_PATH - di->shared_info->filename_offset, ri->filename, ri->filename_length );
											fri->FileName[ di->shared_info->filename_offset + ri->filename_length ] = 0;	// Sanity.
											fri->FileNameLength = di->shared_info->filename_offset + ri->filename_length;
										}

										fri->ReplaceIfExists = FALSE;
										fri->RootDirectory = NULL;

										if ( _SetFileInformationByHandle( di->shared_info->hFile, FileRenameInfo, fri, sizeof( FILE_RENAME_INFO ) + ( sizeof( wchar_t ) * MAX_PATH ) ) == FALSE )
										{
											renamed = false;
										}

										GlobalFree( fri );
									}
									else
									{
										SetLastError( ERROR_ACCESS_DENIED );

										renamed = false;
									}
								}
								else
								{
									wchar_t new_file_path[ MAX_PATH ];

									if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED )
									{
										_wmemcpy_s( new_file_path, MAX_PATH, cfg_temp_download_directory, g_temp_download_directory_length );
										new_file_path[ g_temp_download_directory_length ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
										_wmemcpy_s( new_file_path + ( g_temp_download_directory_length + 1 ), MAX_PATH - ( g_temp_download_directory_length - 1 ), ri->filename, ri->filename_length );
										new_file_path[ g_temp_download_directory_length + ri->filename_length + 1 ] = 0;	// Sanity.
									}
									else
									{
										_wmemcpy_s( new_file_path, MAX_PATH, di->shared_info->file_path, MAX_PATH - di->shared_info->filename_offset );
										new_file_path[ di->shared_info->filename_offset - 1 ] = L'\\';
										_wmemcpy_s( new_file_path + di->shared_info->filename_offset, MAX_PATH - di->shared_info->filename_offset, ri->filename, ri->filename_length );
										new_file_path[ di->shared_info->filename_offset + ri->filename_length ] = 0;	// Sanity.
									}

									if ( MoveFileW( old_file_path, new_file_path ) == FALSE )
									{
										if ( GetLastError() != ERROR_FILE_NOT_FOUND )
										{
											renamed = false;
										}
									}
								}
							}

							if ( renamed )
							{
								_wmemcpy_s( di->shared_info->file_path + di->shared_info->filename_offset, MAX_PATH - di->shared_info->filename_offset, ri->filename, ri->filename_length );
								di->shared_info->file_path[ di->shared_info->filename_offset + ri->filename_length ] = 0;	// Sanity.

								// Get the new file extension offset.
								di->shared_info->file_extension_offset = di->shared_info->filename_offset + get_file_extension_offset( di->shared_info->file_path + di->shared_info->filename_offset, lstrlenW( di->shared_info->file_path + di->shared_info->filename_offset ) );

								DoublyLinkedList *context_node;

								// If we manually renamed our download, then prevent it from being set elsewhere.
								EnterCriticalSection( &di->di_cs );

								context_node = di->parts_list;

								LeaveCriticalSection( &di->di_cs );

								while ( context_node != NULL )
								{
									SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

									context_node = context_node->next;

									if ( context != NULL )
									{
										EnterCriticalSection( &context->context_cs );

										context->got_filename = 1;

										LeaveCriticalSection( &context->context_cs );
									}
								}

								RemoveCachedIcon( di->shared_info, old_file_path, old_filename_offset, old_file_extension_offset );

								HandleIconUpdate( di );

								g_download_history_changed = true;
							}
							else
							{
								// Alert the user, but don't hang up this thread.
								int error = GetLastError();
								if ( error == ERROR_ALREADY_EXISTS )
								{
									_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_There_is_already_a_file );
								}
								else if ( error == ERROR_FILE_NOT_FOUND )
								{
									_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 1, 0 );
								}
								else if ( error == ERROR_PATH_NOT_FOUND )
								{
									_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_The_specified_path_was_not_found );
								}
								else if ( error == ERROR_ACCESS_DENIED )
								{
									_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_File_is_in_use_cannot_rename );
								}
							}
						}
					}
				}

				GlobalFree( ri->filename );
			}

			GlobalFree( ri );
		}

		LeaveCriticalSection( &cleanup_cs );

		_SendMessageW( g_hWnd_tlv_files, TLVM_TOGGLE_DRAW, FALSE, NULL );

		ProcessingList( false );
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN create_download_history_csv_file( void *file_path )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( file_path != NULL )
	{
		save_download_history_csv_file( ( wchar_t * )file_path );

		GlobalFree( file_path );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN export_list( void *pArguments )
{
	importexportinfo *iei = ( importexportinfo * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( iei != NULL )
	{
		if ( iei->file_paths != NULL )
		{
			save_download_history( iei->file_paths );
	
			GlobalFree( iei->file_paths );
		}

		GlobalFree( iei );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN import_list( void *pArguments )
{
	importexportinfo *iei = ( importexportinfo * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	bool bad_format = false;

	wchar_t file_path[ MAX_PATH ];
	wchar_t *filename = NULL;

	if ( iei != NULL )
	{
		if ( iei->file_paths != NULL )
		{
			filename = iei->file_paths;	// The last file path will be an empty string.

			// The first string is actually a directory.
			_wmemcpy_s( file_path, MAX_PATH, filename, iei->file_offset );
			file_path[ iei->file_offset - 1 ] = '\\';

			filename += iei->file_offset;

			// Make sure the path is not the empty string.
			while ( *filename != NULL )
			{
				int filename_length = lstrlenW( filename ) + 1;	// Include the NULL terminator.

				_wmemcpy_s( file_path + iei->file_offset, MAX_PATH - iei->file_offset, filename, filename_length );

				if ( read_download_history( file_path, ( cfg_scroll_to_last_item && iei->type == 1 ) ) == -2 )
				{
					bad_format = true;
				}

				// Only save if we've imported - not loaded during startup.
				if ( iei->type == 1 )
				{
					g_download_history_changed = true;	// Assume that entries were added so that we can save the new history during shutdown.
				}

				// Move to the next string.
				filename = filename + filename_length;
			}

			GlobalFree( iei->file_paths );

			// Only display if the import from menu failed.
			if ( bad_format && iei->type == 1 )
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_File_format_is_incorrect );
			}
		}

		GlobalFree( iei );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN delete_files( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	wchar_t file_path[ MAX_PATH + 1 ];
	bool delete_success = true;
	unsigned char error_type = 0;

	int sel_count = TLV_GetSelectedCount();

	TREELISTNODE *tln;
	int sel_index = TLV_GetNextSelectedItem( NULL, 0, &tln );
	while ( tln != NULL )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
		if ( di != NULL )
		{
			bool allow_delete = true;

			unsigned int status = ( g_allow_rename ? STATUS_NONE : STATUS_DELETE );

			EnterCriticalSection( &di->di_cs );

			DoublyLinkedList *context_node = di->parts_list;

			// If there are still active connections.
			if ( di->download_node.data != NULL )
			{
				SetStatus( di, STATUS_STOPPED | status );

				LeaveCriticalSection( &di->di_cs );

				while ( context_node != NULL )
				{
					SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

					context_node = context_node->next;

					SetContextStatus( context, STATUS_STOPPED | status );
				}

				allow_delete = g_allow_rename;
			}
			else
			{
				LeaveCriticalSection( &di->di_cs );
			}

			// If the allow-rename switch is set, then we can rename files that are actively downloading.
			// They can't be opened while they're downloading.
			// They can be deleted while they're downloading.
			// This avoids us having to delete them in ConnectionCleanup().
			if ( allow_delete )
			{
				if ( !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
				{
					if ( cfg_use_temp_download_directory && di->status != STATUS_COMPLETED )
					{
						GetTemporaryFilePath( di, file_path );
					}
					else
					{
						GetDownloadFilePath( di, file_path );
					}

					if ( cfg_move_to_trash )
					{
						int file_path_length = lstrlenW( file_path );
						file_path[ file_path_length ] = 0;
						file_path[ file_path_length + 1 ] = 0;
						SHFILEOPSTRUCTW sfos;
						_memzero( &sfos, sizeof( SHFILEOPSTRUCTW ) );
						sfos.wFunc = FO_DELETE;
						sfos.pFrom = file_path;
						sfos.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;

						int error = _SHFileOperationW( &sfos );
						if ( error != 0 )
						{
							delete_success = false;

							if ( error == ERROR_ACCESS_DENIED )
							{
								error_type |= 1;
							}
							else if ( error == ERROR_FILE_NOT_FOUND )
							{
								error_type |= 2;
							}
							else if ( error == ERROR_PATH_NOT_FOUND )
							{
								error_type |= 4;
							}
						}
					}
					else if ( DeleteFileW( file_path ) == FALSE )
					{
						delete_success = false;

						int error = GetLastError();
						if ( error == ERROR_ACCESS_DENIED )
						{
							error_type |= 1;
						}
						else if ( error == ERROR_FILE_NOT_FOUND )
						{
							error_type |= 2;
						}
						else if ( error == ERROR_PATH_NOT_FOUND )
						{
							error_type |= 4;
						}
					}

#ifdef ENABLE_LOGGING
					wchar_t *l_file_path;
					wchar_t t_l_file_path[ MAX_PATH ];
					bool is_temp = false;
					if ( cfg_use_temp_download_directory && di->shared_info->status != STATUS_COMPLETED ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
					else { GetDownloadFilePath( di, t_l_file_path ); }
					l_file_path = t_l_file_path;
					WriteLog( LOG_INFO_ACTION, "Deleting: %s%S", ( is_temp ? "temp | " : "" ), l_file_path );
#endif

				}
			}
		}

		sel_index = TLV_GetNextSelectedItem( tln, sel_index, &tln );
	}

	if ( !delete_success )
	{
		if ( sel_count == 1 )
		{
			if ( error_type & 1 )	// Access Denied
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_File_is_in_use_cannot_delete );
			}
			else if ( error_type & 2 )	// File Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 1, 0 );
			}
			else if ( error_type & 4 )	// Path Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_The_specified_path_was_not_found );
			}
		}
		else if ( sel_count > 1 )
		{
			if ( error_type & 1 )	// Access Denied
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_files_are_in_use );
			}

			if ( error_type & 2 )	// File Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_files_were_not_found );
			}

			if ( error_type & 4 )	// Path Not Found.
			{
				_SendNotifyMessageW( g_hWnd_main, WM_ALERT, 0, ( LPARAM )ST_V_One_or_more_paths_were_not_found );
			}
		}
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN search_list( void *pArguments )
{
	SEARCH_INFO *si = ( SEARCH_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( si != NULL )
	{
		if ( si->text != NULL )
		{
			TREELISTNODE *last_sel_tln = NULL;

			TREELISTNODE *tln = TLV_GetFocusedItem();
			int current_item_index = TLV_GetFocusedIndex();

			// If we're searching for filenames.
			if ( si->type == 0 )
			{
				if ( tln != NULL )
				{
					if ( tln->parent != NULL )
					{
						// Include the parent.
						current_item_index = TLV_GetParentIndex( tln, current_item_index ) + 1;

						if ( tln->parent->is_expanded )
						{
							current_item_index += tln->parent->child_count;
						}

						tln = tln->parent->next;
					}
					else
					{
						++current_item_index;

						if ( tln->is_expanded )
						{
							current_item_index += tln->child_count;
						}

						tln = tln->next;
					}
				}
			}
			else
			{
				tln = TLV_NextNode( tln, false );
				if ( tln != NULL )
				{
					++current_item_index;
				}
			}

			if ( tln == NULL )
			{
				tln = g_tree_list;
				current_item_index = 0;
			}

			TREELISTNODE *start_tln = tln;

			TLV_ClearSelected( false, false );
			TLV_ResetSelectionBounds();

			int sel_count = 0;

			do
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )tln->data;
				if ( di != NULL )
				{
					// Only adjust the node values (indices and nodes) if we're on the current filter.
					if ( g_status_filter == STATUS_NONE || IsFilterSet( di, g_status_filter ) )
					{
						bool found_match = false;

						wchar_t *text;

						if ( si->type == 2 )
						{
							text = di->comments;
						}
						else
						{
							text = ( si->type == 1 ? di->url : ( di->shared_info->file_path + di->shared_info->filename_offset ) );
						}

						if ( si->search_flag == 0x04 )	// Regular expression search.
						{
							if ( pcre2_state == PCRE2_STATE_RUNNING )
							{
								int error_code;
								size_t error_offset;

								pcre2_code *regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )si->text, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );

								if ( regex_code != NULL )
								{
									pcre2_match_data *match = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );

									if ( match != NULL )
									{
										if ( _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )text, lstrlenW( text ), 0, 0, match, NULL ) >= 0 )
										{
											found_match = true;
										}

										_pcre2_match_data_free_16( match );
									}

									_pcre2_code_free_16( regex_code );
								}
							}
						}
						else
						{
							if ( si->search_flag == ( 0x01 | 0x02 ) )	// Match case and whole word.
							{
								if ( lstrcmpW( text, si->text ) == 0 )
								{
									found_match = true;
								}
							}
							else if ( si->search_flag == 0x02 )	// Match whole word.
							{
								if ( lstrcmpiW( text, si->text ) == 0 )
								{
									found_match = true;
								}
							}
							else if ( si->search_flag == 0x01 )	// Match case.
							{
								if ( _StrStrW( text, si->text ) != NULL )
								{
									found_match = true;
								}
							}
							else
							{
								if ( _StrStrIW( text, si->text ) != NULL )
								{
									found_match = true;
								}
							}
						}

						if ( found_match )
						{
							++sel_count;

							TLV_SetSelectionBounds( current_item_index, tln );

							TLV_SetFocusedItem( tln );
							TLV_SetFocusedIndex( current_item_index );

							if ( last_sel_tln != NULL )
							{
								last_sel_tln->flag = TLVS_SELECTED;
							}

							tln->flag = TLVS_SELECTED | TLVS_FOCUSED;

							last_sel_tln = tln;

							if ( !si->search_all )
							{
								int visible_item_count = TLV_GetVisibleItemCount();
								int first_visible_index = TLV_GetFirstVisibleIndex();

								if ( current_item_index >= first_visible_index + visible_item_count )
								{
									TREELISTNODE *first_visible_node = tln;

									for ( ; visible_item_count > 1 && first_visible_node != g_tree_list; --current_item_index, --visible_item_count )
									{
										first_visible_node = TLV_PrevNode( first_visible_node, false );
									}

									int root_index = TLV_GetFirstVisibleRootIndex();

									TREELISTNODE *current_first_visible_parent_node = TLV_GetFirstVisibleItem();
									current_first_visible_parent_node = ( current_first_visible_parent_node != NULL && current_first_visible_parent_node->parent != NULL ? current_first_visible_parent_node->parent : current_first_visible_parent_node );
									TREELISTNODE *first_visible_parent_node = ( first_visible_node != NULL && first_visible_node->parent != NULL ? first_visible_node->parent : first_visible_node );
									while ( current_first_visible_parent_node != NULL && current_first_visible_parent_node != first_visible_parent_node )
									{
										di = ( DOWNLOAD_INFO * )current_first_visible_parent_node->data;

										current_first_visible_parent_node = current_first_visible_parent_node->next;

										// This shouldn't be true since we're filtering as we're iterating across the entire list.
										if ( g_status_filter != STATUS_NONE && !IsFilterSet( di, g_status_filter ) )
										{
											continue;
										}

										++root_index;
									}

									TLV_SetFirstVisibleRootIndex( root_index );

									TLV_SetFirstVisibleItem( first_visible_node );
									TLV_SetFirstVisibleIndex( current_item_index );
								}
								else if ( current_item_index < first_visible_index )
								{
									int root_index = TLV_GetFirstVisibleRootIndex();

									TREELISTNODE *current_first_visible_parent_node = TLV_GetFirstVisibleItem();
									current_first_visible_parent_node = ( current_first_visible_parent_node != NULL && current_first_visible_parent_node->parent != NULL ? current_first_visible_parent_node->parent : current_first_visible_parent_node );
									TREELISTNODE *first_visible_parent_node = ( tln != NULL && tln->parent != NULL ? tln->parent : tln );
									while ( current_first_visible_parent_node != NULL && current_first_visible_parent_node != first_visible_parent_node )
									{
										di = ( DOWNLOAD_INFO * )current_first_visible_parent_node->data;

										current_first_visible_parent_node = current_first_visible_parent_node->prev;

										if ( g_status_filter != STATUS_NONE && !IsFilterSet( di, g_status_filter ) )
										{
											continue;
										}

										--root_index;
									}

									TLV_SetFirstVisibleRootIndex( root_index );

									TLV_SetFirstVisibleItem( tln );
									TLV_SetFirstVisibleIndex( current_item_index );
								}

								break;
							}
						}

						++current_item_index;

						// If we're searching for filenames.
						if ( si->type == 0 )
						{
							if ( tln->is_expanded )
							{
								current_item_index += tln->child_count;
							}
						}
					}
				}

				// If we're searching for filenames.
				if ( si->type == 0 )
				{
					// Search root nodes.
					tln = tln->next;
				}
				else
				{
					tln = TLV_NextNode( tln, false );
				}

				if ( tln == NULL )
				{
					tln = g_tree_list;
					current_item_index = 0;
				}
			}
			while ( tln != start_tln );

			TLV_SetSelectedCount( sel_count );

			GlobalFree( si->text );
		}

		GlobalFree( si );
	}

	_SendMessageW( g_hWnd_search, WM_PROPAGATE, 1, 0 );

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN filter_urls( void *pArguments )
{
	FILTER_INFO *fi = ( FILTER_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( fi != NULL )
	{
		if ( fi->text != NULL )
		{
			if ( fi->filter != NULL )
			{
				if ( pcre2_state == PCRE2_STATE_RUNNING )
				{
					int error_code;
					size_t error_offset;

					pcre2_code *regex_code = _pcre2_compile_16( ( PCRE2_SPTR16 )fi->filter, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL );

					if ( regex_code != NULL )
					{
						pcre2_match_data *match = _pcre2_match_data_create_from_pattern_16( regex_code, NULL );

						if ( match != NULL )
						{
							wchar_t *text_start = fi->text;
							wchar_t *text_end = fi->text;
							wchar_t *last_end = fi->text;

							while ( *text_start != NULL )
							{
								// Find the end of the URL (separated by \r\n).
								while ( *text_end != NULL )
								{
									if ( *text_end == L'\r' && *( text_end + 1 ) == L'\n' )
									{
										break;
									}

									++text_end;
								}

								wchar_t tmp_end = *text_end;
								*text_end = 0;
								size_t text_length = ( size_t )( text_end - text_start );

								int match_count = _pcre2_match_16( regex_code, ( PCRE2_SPTR16 )text_start, text_length, 0, 0, match, NULL );

								*text_end = tmp_end;	// Restore.

								if ( *text_end != NULL )
								{
									text_length += 2;	// Include the \r\n characters.

									text_end += 2;
								}

								if ( match_count >= 0 )
								{
									if ( text_start > fi->text )
									{
										_wmemcpy_s( last_end, text_end - last_end, text_start, text_length );
									}

									last_end += text_length;
								}

								text_start = text_end;
							}

							*last_end = 0; // Sanity.

							// fi->text is freed in WM_FILTER_TEXT.
							_SendMessageW( g_hWnd_add_urls, WM_FILTER_TEXT, 0, ( LPARAM )fi->text );
							fi->text = NULL;

							_pcre2_match_data_free_16( match );
						}

						_pcre2_code_free_16( regex_code );
					}
				}

				GlobalFree( fi->filter );
			}

			GlobalFree( fi->text );
		}

		GlobalFree( fi );
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN process_command_line_args( void *pArguments )
{
	CL_ARGS *cla = ( CL_ARGS * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	if ( cla != NULL )
	{
		if ( cla->download_history_file != NULL )
		{
			importexportinfo *iei = ( importexportinfo * )GlobalAlloc( GMEM_FIXED, sizeof( importexportinfo ) );
			if ( iei != NULL )
			{
				// Include an empty string.
				iei->file_paths = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * ( MAX_PATH + 1 ) );
				if ( iei->file_paths != NULL )
				{
					_wmemcpy_s( iei->file_paths, MAX_PATH, cla->download_history_file, cla->download_history_file_length );
					iei->file_paths[ cla->download_history_file_length ] = 0;	// Sanity.

					iei->file_offset = ( unsigned short )( cla->download_history_file_length );

					// Find the last occurance of "\" in the string.
					while ( iei->file_offset != 0 )
					{
						if ( iei->file_paths[ --iei->file_offset ] == L'\\' )
						{
							iei->file_paths[ iei->file_offset++ ] = 0;	// Sanity.

							break;
						}
					}

					iei->type = 1;	// Import from menu.

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

		wchar_t *url_list = NULL;
		unsigned int url_list_length = 0;

		if ( cla->url_list_file != NULL )
		{
			url_list = read_url_list_file( ( wchar_t * )cla->url_list_file, url_list_length );

			if ( url_list != NULL )
			{
				if ( cla->urls == NULL )
				{
					cla->urls = url_list;
					cla->urls_length = url_list_length;
				}
				else
				{
					wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( url_list, sizeof( wchar_t ) * ( url_list_length + cla->urls_length + 2 + 1 ), GMEM_MOVEABLE );
					if ( realloc_buffer != NULL )
					{
						url_list = realloc_buffer;

						url_list[ url_list_length++ ] = L'\r';
						url_list[ url_list_length++ ] = L'\n';
						_wmemcpy_s( url_list + url_list_length, cla->urls_length + 1, cla->urls, cla->urls_length );
						url_list_length += cla->urls_length;
						url_list[ url_list_length ] = 0;	// Sanity.

						GlobalFree( cla->urls );
						cla->urls = url_list;
						cla->urls_length = url_list_length;
					}
				}
			}
		}

		if ( cla->download_immediately == 1 )
		{
			int utf8_val_length = 0;

			ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );
			if ( ai != NULL )
			{
				ai->method = ( cla->data != NULL ? METHOD_POST : METHOD_GET );

				ai->download_operations = cla->download_operations;

				if ( cla->category != NULL )
				{
					ai->category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->category_length + 1 ) );
					_wmemcpy_s( ai->category, cla->category_length + 1, cla->category, cla->category_length );
					ai->category[ cla->category_length ] = 0;	// Sanity.
				}

				ai->use_download_directory = cla->use_download_directory;
				ai->download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
				if ( ai->download_directory != NULL )
				{
					if ( cla->use_download_directory && cla->download_directory != NULL )
					{
						_wmemcpy_s( ai->download_directory, MAX_PATH, cla->download_directory, cla->download_directory_length );
						ai->download_directory[ cla->download_directory_length ] = 0;	// Sanity.
					}
					else
					{
						_wmemcpy_s( ai->download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
						ai->download_directory[ g_default_download_directory_length ] = 0;	// Sanity.
					}
				}

				ai->use_parts = cla->use_parts;
				ai->parts = ( cla->use_parts ? cla->parts : cfg_default_download_parts );
				ai->use_download_speed_limit = cla->use_download_speed_limit;
				ai->download_speed_limit = ( cla->use_download_speed_limit ? cla->download_speed_limit : cfg_download_speed_limit );
				ai->ssl_version = ( cla->ssl_version != 0 ? cla->ssl_version : cfg_default_ssl_version + 1 );

				if ( cla->urls != NULL )
				{
					ai->urls = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->urls_length + 1 ) );
					_wmemcpy_s( ai->urls, cla->urls_length + 1, cla->urls, cla->urls_length );
					ai->urls[ cla->urls_length ] = 0;	// Sanity.
				}

				if ( cla->comments != NULL )
				{
					ai->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->comments_length + 1 ) );
					_wmemcpy_s( ai->comments, cla->comments_length + 1, cla->comments, cla->comments_length );
					ai->comments[ cla->comments_length ] = 0;	// Sanity.
				}

				if ( cla->cookies != NULL )
				{
					utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, cla->cookies, cla->cookies_length + 1, NULL, 0, NULL, NULL );
					ai->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, cla->cookies, cla->cookies_length + 1, ai->utf8_cookies, utf8_val_length, NULL, NULL );
				}

				if ( cla->headers != NULL )
				{
					utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, cla->headers, cla->headers_length + 1, NULL, 0, NULL, NULL );
					ai->utf8_headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, cla->headers, cla->headers_length + 1, ai->utf8_headers, utf8_val_length, NULL, NULL );
				}

				if ( cla->data != NULL )
				{
					utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, cla->data, cla->data_length + 1, NULL, 0, NULL, NULL );
					ai->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, cla->data, cla->data_length + 1, ai->utf8_data, utf8_val_length, NULL, NULL );
				}

				if ( cla->username != NULL )
				{
					utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, cla->username, cla->username_length + 1, NULL, 0, NULL, NULL );
					ai->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, cla->username, cla->username_length + 1, ai->auth_info.username, utf8_val_length, NULL, NULL );
				}

				if ( cla->password != NULL )
				{
					utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, cla->password, cla->password_length + 1, NULL, 0, NULL, NULL );
					ai->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, cla->password, cla->password_length + 1, ai->auth_info.password, utf8_val_length, NULL, NULL );
				}

				ai->proxy_info.type = cla->proxy_type;

				if ( ai->proxy_info.type != 0 )
				{
					if ( cla->proxy_hostname != NULL )
					{
						ai->proxy_info.hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_hostname_length + 1 ) );
						_wmemcpy_s( ai->proxy_info.hostname, cla->proxy_hostname_length + 1, cla->proxy_hostname, cla->proxy_hostname_length );
						ai->proxy_info.hostname[ cla->proxy_hostname_length ] = 0;	// Sanity.

						if ( normaliz_state == NORMALIZ_STATE_RUNNING )
						{
							int punycode_length = _IdnToAscii( 0, ai->proxy_info.hostname, cla->proxy_hostname_length + 1, NULL, 0 );

							if ( punycode_length > ( int )( cla->proxy_hostname_length + 1 ) )
							{
								ai->proxy_info.punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, ai->proxy_info.hostname, cla->proxy_hostname_length + 1, ai->proxy_info.punycode_hostname, punycode_length );
							}
						}

						ai->proxy_info.address_type = 0;
					}
					else
					{
						ai->proxy_info.ip_address = cla->proxy_ip_address;

						ai->proxy_info.address_type = 1;
					}

					if ( cla->proxy_port == 0 )
					{
						ai->proxy_info.port = ( cla->proxy_type == 1 ? 80 : ( cla->proxy_type == 2 ? 443 : 1080 ) );
					}

					if ( cla->proxy_username != NULL )
					{
						ai->proxy_info.use_authentication = true;	// For SOCKS v5 connections.

						ai->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_username_length + 1 ) );
						if ( ai->proxy_info.w_username != NULL )
						{
							_wmemcpy_s( ai->proxy_info.w_username, cla->proxy_username_length + 1, cla->proxy_username, cla->proxy_username_length );
							ai->proxy_info.w_username[ cla->proxy_username_length ] = 0;	// Sanity.

							utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, NULL, 0, NULL, NULL );
							ai->proxy_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
							WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, ai->proxy_info.username, utf8_val_length, NULL, NULL );
						}
					}

					if ( cla->proxy_password != NULL )
					{
						ai->proxy_info.use_authentication = true;	// For SOCKS v5 connections.

						ai->proxy_info.w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( cla->proxy_password_length + 1 ) );
						if ( ai->proxy_info.w_password != NULL )
						{
							_wmemcpy_s( ai->proxy_info.w_password, cla->proxy_password_length + 1, cla->proxy_password, cla->proxy_password_length );
							ai->proxy_info.w_password[ cla->proxy_password_length ] = 0;	// Sanity.

							utf8_val_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, NULL, 0, NULL, NULL );
							ai->proxy_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_val_length ); // Size includes the null character.
							WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, ai->proxy_info.password, utf8_val_length, NULL, NULL );
						}
					}

					ai->proxy_info.resolve_domain_names = cla->proxy_resolve_domain_names;
				}

				// ai is freed in AddURL.
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
				else
				{
					FreeAddInfo( &ai );
				}
			}
		}
		else if ( cla->urls != NULL || cla->use_clipboard )
		{
			_SendMessageW( g_hWnd_main, WM_PROPAGATE, ( WPARAM )-1, ( LPARAM )cla );
		}

		FreeCommandLineArgs( &cla );
	}

	ProcessingList( false );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN save_session( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( cfg_enable_download_history && g_download_history_changed )
	{
		wchar_t t_base_directory[ MAX_PATH ];

		_wmemcpy_s( t_base_directory, MAX_PATH, g_base_directory, g_base_directory_length );
		_wmemcpy_s( t_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\download_history\0", 18 );
		//t_base_directory[ g_base_directory_length + 17 ] = 0;	// Sanity.

		save_download_history( t_base_directory );
		g_download_history_changed = false;
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}
