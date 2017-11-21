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

#include "connection.h"

#include "doublylinkedlist.h"

#include "list_operations.h"
#include "file_operations.h"

#include "menus.h"

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
		_InvalidateRect( g_hWnd_files, NULL, FALSE );				// Refresh the number column values.
		_SetFocus( g_hWnd_files );									// Give focus back to the listview to allow shortcut keys.
		//_SetWindowTextW( g_hWnd_main, PROGRAM_CAPTION );			// Reset the window title.
	}
}

THREAD_RETURN remove_items( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	// Prevent the listviews from drawing while freeing lParam values.
	skip_list_draw = true;

	ProcessingList( true );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;

	int item_count = _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );
	int sel_count = _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 );

	int *index_array = NULL;

	bool handle_all = false;
	if ( item_count == sel_count )
	{
		handle_all = true;
	}
	else
	{
		_SendMessageW( g_hWnd_files, LVM_ENSUREVISIBLE, 0, FALSE );

		index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

		lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

		_EnableWindow( g_hWnd_files, FALSE );	// Prevent any interaction with the listview while we're processing.

		// Create an index list of selected items (in reverse order).
		for ( int i = 0; i < sel_count; ++i )
		{
			lvi.iItem = index_array[ sel_count - 1 - i ] = _SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
		}

		_EnableWindow( g_hWnd_files, TRUE );	// Allow the listview to be interactive.

		item_count = sel_count;
	}	

	// Go through each item, and free their lParam values.
	for ( int i = 0; i < item_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		if ( handle_all )
		{
			lvi.iItem = i;
		}
		else
		{
			lvi.iItem = index_array[ i ];
		}

		// Wait, specifically for CleanupConnection to do its thing.
		EnterCriticalSection( &cleanup_cs );

		_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;

		if ( !handle_all )
		{
			_SendMessageW( g_hWnd_files, LVM_DELETEITEM, index_array[ i ], 0 );
		}
		else if ( i >= ( item_count - 1 ) )
		{
			_SendMessageW( g_hWnd_files, LVM_DELETEALLITEMS, 0, 0 );
		}

		if ( di != NULL )
		{
			EnterCriticalSection( &di->shared_cs );

			DoublyLinkedList *context_node = di->parts_list;
			SOCKET_CONTEXT *context = NULL;

			// If there are still active connections.
			if ( di->active_parts > 0 )
			{
				di->status = STATUS_STOP_AND_REMOVE;

				while ( context_node != NULL )
				{
					context = ( SOCKET_CONTEXT * )context_node->data;

					context_node = context_node->next;

					if ( context != NULL )
					{
						EnterCriticalSection( &context->context_cs );

						context->status = STATUS_STOP_AND_REMOVE;

						if ( context->cleanup == 0 )
						{
							context->cleanup = 2;	// Force the cleanup.

							InterlockedIncrement( &context->pending_operations );

							context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

							PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
						}

						LeaveCriticalSection( &context->context_cs );
					}
				}

				LeaveCriticalSection( &di->shared_cs );
			}
			else	// No active parts.
			{
				if ( di->queue_node.data != NULL )
				{
					EnterCriticalSection( &download_queue_cs );

					DLL_RemoveNode( &download_queue, &di->queue_node );

					LeaveCriticalSection( &download_queue_cs );
				}

				LeaveCriticalSection( &di->shared_cs );

				// di->icon is stored in the icon_handles tree. We'll destory it in the tree.

				EnterCriticalSection( &icon_cache_cs );
				// Find the icon info
				dllrbt_iterator *itr = dllrbt_find( icon_handles, ( void * )( di->file_path + di->file_extension_offset ), false );

				// Free its values and remove it from the tree if there are no other items using it.
				if ( itr != NULL )
				{
					ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
					if ( ii != NULL )
					{
						if ( --ii->count == 0 )
						{
							DestroyIcon( ii->icon );
							GlobalFree( ii->file_extension );
							GlobalFree( ii );

							dllrbt_remove( icon_handles, itr );
						}
					}
					else
					{
						dllrbt_remove( icon_handles, itr );
					}
				}
				LeaveCriticalSection( &icon_cache_cs );

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

		LeaveCriticalSection( &cleanup_cs );
	}

	if ( index_array != NULL )
	{
		GlobalFree( index_array );
	}

	download_history_changed = true;

	skip_list_draw = false;

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
	return 0;
}

THREAD_RETURN load_download_history( void *pArguments )
{
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\download_history\0", 18 );
	base_directory[ base_directory_length + 17 ] = 0;	// Sanity.

	read_download_history( base_directory );

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
	return 0;
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
				EnterCriticalSection( &di->shared_cs );

				if ( di->status == STATUS_DOWNLOADING )
				{
					di->status = STATUS_PAUSED;
				
					DoublyLinkedList *context_node = di->parts_list;
					SOCKET_CONTEXT *context = NULL;

					while ( context_node != NULL )
					{
						context = ( SOCKET_CONTEXT * )context_node->data;

						context_node = context_node->next;

						if ( context != NULL )
						{
							context->status = STATUS_PAUSED;
						}
					}
				}

				LeaveCriticalSection( &di->shared_cs );
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
				EnterCriticalSection( &di->shared_cs );

				di->status = STATUS_STOPPED;

				LeaveCriticalSection( &di->shared_cs );

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
				EnterCriticalSection( &di->shared_cs );

				DoublyLinkedList *context_node = di->parts_list;
				SOCKET_CONTEXT *context = NULL;

				if ( di->status == STATUS_CONNECTING ||
					 di->status == STATUS_DOWNLOADING ||
					 di->status == STATUS_PAUSED ||
					 di->status == STATUS_QUEUED )
				{
					// Download is active, close the connection.
					if ( di->download_node.data != NULL )
					{
						// di->status will be set to STATUS_STOPPED in CleanupConnection().
						while ( context_node != NULL )
						{
							context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							if ( context != NULL )
							{
								EnterCriticalSection( &context->context_cs );

								context->status = STATUS_STOPPED;

								if ( context->cleanup == 0 )
								{
									context->cleanup = 2;	// Force the cleanup.

									InterlockedIncrement( &context->pending_operations );

									context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

									PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
								}

								LeaveCriticalSection( &context->context_cs );
							}
						}
					}
					else if ( di->queue_node.data != NULL )	// Download is queued.
					{
						di->status = STATUS_STOPPED;

						EnterCriticalSection( &download_queue_cs );

						// Remove the item from the download queue.
						DLL_RemoveNode( &download_queue, &di->queue_node );
						di->queue_node.data = NULL;

						LeaveCriticalSection( &download_queue_cs );
					}
				}

				LeaveCriticalSection( &di->shared_cs );
			}

			active_download_node = active_download_node->next;
		}

		LeaveCriticalSection( &active_download_list_cs );
	}
	else if ( handle_type == 2 )	// Remove completed downloads.
	{
		// Get the number of items in the listview.
		int num_items = _SendMessageW( g_hWnd_files, LVM_GETITEMCOUNT, 0, 0 );

		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;

		_SendMessageW( g_hWnd_files, LVM_ENSUREVISIBLE, 0, FALSE );

		// Start from the end and work backwards.
		for ( lvi.iItem = num_items - 1; lvi.iItem >= 0; --lvi.iItem )
		{
			// Stop processing and exit the thread.
			if ( kill_worker_thread_flag )
			{
				break;
			}

			_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
			if ( di != NULL )
			{
				bool remove_item = false;

				EnterCriticalSection( &di->shared_cs );

				if ( di->status == STATUS_COMPLETED )
				{
					remove_item = true;
				}

				LeaveCriticalSection( &di->shared_cs );

				if ( remove_item )
				{
					_SendMessageW( g_hWnd_files, LVM_DELETEITEM, lvi.iItem, 0 );

					EnterCriticalSection( &icon_cache_cs );
					// Find the icon info
					dllrbt_iterator *itr = dllrbt_find( icon_handles, ( void * )( di->file_path + di->file_extension_offset ), false );

					// Free its values and remove it from the tree if there are no other items using it.
					if ( itr != NULL )
					{
						ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
						if ( ii != NULL )
						{
							if ( --ii->count == 0 )
							{
								DestroyIcon( ii->icon );
								GlobalFree( ii->file_extension );
								GlobalFree( ii );

								dllrbt_remove( icon_handles, itr );
							}
						}
						else
						{
							dllrbt_remove( icon_handles, itr );
						}
					}
					LeaveCriticalSection( &icon_cache_cs );

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
		}

	}
	else if ( handle_type == 3 )	// Restart selected download (from the beginning).
	{
		LVITEM lvi;
		_memzero( &lvi, sizeof( LVITEM ) );
		lvi.mask = LVIF_PARAM;
		lvi.iItem = _SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED );

		if ( lvi.iItem != -1 )
		{
			_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

			DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
			if ( di != NULL )
			{
				EnterCriticalSection( &di->shared_cs );

				// Ensure that there are no active parts downloading.
				if ( di->active_parts == 0 )
				{
					download_history_changed = true;

					while ( di->range_list != NULL )
					{
						DoublyLinkedList *range_node = di->range_list;
						di->range_list = di->range_list->next;

						GlobalFree( range_node->data );
						GlobalFree( range_node );
					}

					di->processed_header = false;

					di->downloaded = 0;

					// If we manually start a download, then set the incomplete retry attempts back to 0.
					di->retries = 0;

					// If we manually start a download that was added remotely, then allow the prompts to display.
					di->download_operations &= ~DOWNLOAD_OPERATION_OVERRIDE_PROMPTS;

					StartDownload( di, false );
				}

				LeaveCriticalSection( &di->shared_cs );
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
	return 0;
}

THREAD_RETURN handle_connection( void *pArguments )
{
	unsigned char status = ( unsigned char )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM;
	lvi.iItem = -1;

	int sel_count = _SendMessageW( g_hWnd_files, LVM_GETSELECTEDCOUNT, 0, 0 );

	int *index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

	lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

	_EnableWindow( g_hWnd_files, FALSE );	// Prevent any interaction with the listview while we're processing.

	for ( int i = 0; i < sel_count; ++i )
	{
		lvi.iItem = index_array[ i ] = _SendMessageW( g_hWnd_files, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
	}

	_EnableWindow( g_hWnd_files, TRUE );	// Allow the listview to be interactive.

	for ( int i = 0; i < sel_count; ++i )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		EnterCriticalSection( &cleanup_cs );

		lvi.iItem = index_array[ i ];

		_SendMessageW( g_hWnd_files, LVM_GETITEM, 0, ( LPARAM )&lvi );

		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lvi.lParam;
		if ( di != NULL )
		{
			EnterCriticalSection( &di->shared_cs );

			// Make sure we're not already in the process of shutting down or closing the connection.
			if ( di->status != STATUS_COMPLETED && di->status != status )
			{
				DoublyLinkedList *context_node = di->parts_list;
				SOCKET_CONTEXT *context = NULL;

				if ( context_node != NULL )
				{
					context = ( SOCKET_CONTEXT * )context_node->data;
				}

				if ( di->status == STATUS_CONNECTING ||
					 di->status == STATUS_DOWNLOADING )	// The download is currently connection or downloading.
				{
					if ( status == STATUS_STOPPED )	// Stop (close) the active connection.
					{
						// di->status will be set to STATUS_STOPPED in CleanupConnection().
						while ( context_node != NULL )
						{
							context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							if ( context != NULL )
							{
								EnterCriticalSection( &context->context_cs );

								context->status = STATUS_STOPPED;

								if ( context->cleanup == 0 )
								{
									context->cleanup = 2;	// Force the cleanup.

									InterlockedIncrement( &context->pending_operations );

									context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

									PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
								}

								LeaveCriticalSection( &context->context_cs );
							}
						}
					}
					else if ( status == STATUS_PAUSED )
					{
						di->status = STATUS_PAUSED;

						while ( context_node != NULL )
						{
							context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							if ( context != NULL )
							{
								context->status = STATUS_PAUSED;
							}
						}
					}
					/*else
					{
						di->status = status;

						while ( context_node != NULL )
						{
							context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							if ( context != NULL )
							{
								context->status = status );
							}
						}
					}*/
				}
				else if ( di->status == STATUS_PAUSED ||
						  di->status == STATUS_QUEUED )	// The download is currently paused, or queued.
				{
					if ( status == STATUS_DOWNLOADING )	// Resume downloading.
					{
						// Download is active, continue where we left off.
						if ( di->download_node.data != NULL )
						{
							di->status = STATUS_DOWNLOADING;

							// Run through our parts list and connect to each context.
							while ( context_node != NULL )
							{
								context = ( SOCKET_CONTEXT * )context_node->data;

								context_node = context_node->next;

								if ( context != NULL )
								{
									context->status = STATUS_DOWNLOADING;

									InterlockedIncrement( &context->pending_operations );

									// Post a completion status to the completion port that we're going to continue with whatever it left off at.
									PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
								}
							}
						}
						else if ( di->queue_node.data != NULL )	// Download is not active, attempt to resume or queue.
						{
							if ( total_downloading < cfg_max_downloads )
							{
								EnterCriticalSection( &download_queue_cs );

								if ( download_queue != NULL )
								{
									DLL_RemoveNode( &download_queue, &di->queue_node );
									di->queue_node.data = NULL;

									// If we manually start a download, then set the incomplete retry attempts back to 0.
									di->retries = 0;

									// If we manually start a download that was added remotely, then allow the prompts to display.
									di->download_operations &= ~DOWNLOAD_OPERATION_OVERRIDE_PROMPTS;

									StartDownload( di, false );
								}

								LeaveCriticalSection( &download_queue_cs );
							}
							/*else
							{
								di->status = STATUS_QUEUED;	// Queued.
							}*/
						}
					}
					else if ( status == STATUS_STOPPED )	// Stop (close) the active connection.
					{
						// Download is active, close the connection.
						if ( di->download_node.data != NULL )
						{
							// di->status will be set to STATUS_STOPPED in CleanupConnection().
							while ( context_node != NULL )
							{
								context = ( SOCKET_CONTEXT * )context_node->data;

								context_node = context_node->next;

								if ( context != NULL )
								{
									EnterCriticalSection( &context->context_cs );

									context->status = STATUS_STOPPED;

									if ( context->cleanup == 0 )
									{
										context->cleanup = 2;	// Force the cleanup.

										InterlockedIncrement( &context->pending_operations );

										context->overlapped_close.current_operation = ( context->ssl != NULL ? IO_Shutdown : IO_Close );

										PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
									}

									LeaveCriticalSection( &context->context_cs );
								}
							}
						}
						else if ( di->queue_node.data != NULL )	// Download is queued.
						{
							di->status = STATUS_STOPPED;

							EnterCriticalSection( &download_queue_cs );

							// Remove the item from the download queue.
							DLL_RemoveNode( &download_queue, &di->queue_node );
							di->queue_node.data = NULL;

							LeaveCriticalSection( &download_queue_cs );
						}
					}
					/*else
					{
						di->status = status;

						while ( context_node != NULL )
						{
							context = ( SOCKET_CONTEXT * )context_node->data;

							context_node = context_node->next;

							if ( context != NULL )
							{
								context->status = status );
							}
						}
					}*/
				}
				else if ( di->status == STATUS_STOPPED ||
						  di->status == STATUS_TIMED_OUT ||
						  di->status == STATUS_FAILED ||
						  di->status == STATUS_FILE_IO_ERROR ||
						  di->status == STATUS_PROXY_AUTH_REQUIRED )	// The download is currently stopped.
				{
					// Ensure that the download is actually stopped and that there are no active parts downloading.
					if ( di->active_parts == 0 )
					{
						download_history_changed = true;

						// If we manually start a download, then set the incomplete retry attempts back to 0.
						di->retries = 0;

						// If we manually start a download that was added remotely, then allow the prompts to display.
						di->download_operations &= ~DOWNLOAD_OPERATION_OVERRIDE_PROMPTS;

						StartDownload( di, false );
					}
				}
				else if ( di->status == STATUS_SKIPPED )
				{
					// Ensure that the download is actually stopped and that there are no active parts downloading.
					if ( di->active_parts == 0 )
					{
						download_history_changed = true;

						// If we manually start a download, then set the incomplete retry attempts back to 0.
						di->retries = 0;

						// If we manually start a download that was added remotely, then allow the prompts to display.
						di->download_operations &= ~DOWNLOAD_OPERATION_OVERRIDE_PROMPTS;

						StartDownload( di, true );
					}
				}
				/*else
				{
					di->status = status;

					while ( context_node != NULL )
					{
						context = ( SOCKET_CONTEXT * )context_node->data;

						context_node = context_node->next;

						if ( context != NULL )
						{
							SetContextStatus( context status );
						}
					}
				}*/
			}

			LeaveCriticalSection( &di->shared_cs );
		}

		LeaveCriticalSection( &cleanup_cs );
	}

	if ( index_array != NULL )
	{
		GlobalFree( index_array );
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
	return 0;
}

THREAD_RETURN handle_download_queue( void *pArguments )
{
	unsigned char handle_type = ( unsigned char )pArguments;

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	EnterCriticalSection( &cleanup_cs );

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
	
		// Make sure the item is queued.
		if ( di != NULL && di->status == STATUS_QUEUED )
		{
			EnterCriticalSection( &download_queue_cs );

			if ( di->queue_node.data != NULL )
			{
				if ( handle_type == 0 )			// Move to the beginning of the queue.
				{
					// Make sure we're not the head.
					if ( &di->queue_node != download_queue )
					{
						DLL_RemoveNode( &download_queue, &di->queue_node );
						DLL_AddNode( &download_queue, &di->queue_node, 0 );
					}
				}
				else if ( handle_type == 1 )	// Move forward one position in the queue.
				{
					// Make sure we're not the head.
					if ( &di->queue_node != download_queue )
					{
						DoublyLinkedList *prev = di->queue_node.prev;
						if ( prev != NULL )
						{
							DLL_RemoveNode( &download_queue, &di->queue_node );

							// If the node we're replacing is the head.
							if ( prev == download_queue )
							{
								DLL_AddNode( &download_queue, &di->queue_node, 0 );
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
						DLL_RemoveNode( &download_queue, &di->queue_node );

						// If the node we're replacing is the tail.
						if ( next->next == NULL )
						{
							DLL_AddNode( &download_queue, &di->queue_node, -1 );
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
					DLL_RemoveNode( &download_queue, &di->queue_node );
					DLL_AddNode( &download_queue, &di->queue_node, -1 );
				}
			}

			LeaveCriticalSection( &download_queue_cs );
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
	return 0;
}
