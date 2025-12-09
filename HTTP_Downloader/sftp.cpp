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

#include "sftp.h"
#include "http_parsing.h"
#include "utilities.h"
#include "options.h"
#include "file_operations.h"
#include "cmessagebox.h"

bool sftp_fps_host_list_changed = false;
bool sftp_keys_host_list_changed = false;

char SFTP_ProcessFileInfo( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return SFTP_CONTENT_STATUS_FAILED;
	}

	char content_status = SFTP_CONTENT_STATUS_NONE;

	if ( context->download_info != NULL )
	{
		if ( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_VERIFY )
		{
			return SFTP_CONTENT_STATUS_FAILED;	// Bail before we download more.
		}
		else if ( !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
		{
			content_status = HandleLastModifiedPrompt( context );

			if ( content_status != SFTP_CONTENT_STATUS_NONE )
			{
				return content_status;
			}

			content_status = HandleFileSizePrompt( context );

			if ( content_status != SFTP_CONTENT_STATUS_NONE )
			{
				return content_status;
			}

			if ( !context->is_allocated )
			{
				// Returns either SFTP_CONTENT_STATUS_FAILED, SFTP_CONTENT_STATUS_ALLOCATE_FILE, or SFTP_CONTENT_STATUS_NONE.
				content_status = context->content_status = AllocateFile( context, IO_SFTPResumeReadContent );

				if ( content_status != SFTP_CONTENT_STATUS_NONE )
				{
					return content_status;
				}
			}
			else
			{
				EnterCriticalSection( &context->download_info->di_cs );

				context->download_info->status = STATUS_DOWNLOADING;
				context->status = STATUS_DOWNLOADING;

				LeaveCriticalSection( &context->download_info->di_cs );

				// For groups.
				if ( IS_GROUP( context->download_info ) )
				{
					EnterCriticalSection( &context->download_info->shared_info->di_cs );

					context->download_info->shared_info->status = STATUS_DOWNLOADING;

					LeaveCriticalSection( &context->download_info->shared_info->di_cs );
				}
			}
		}
	}

	// Handle zero byte files.
	if ( context->header_info.range_info->content_length == 0 )
	{
		context->header_info.range_info->content_offset = 1;	// Prevents us from retrying the connection in CleanupConnection().

		content_status = SFTP_CONTENT_STATUS_FAILED;	// Close the connection.
	}

	return content_status;
}

char SFTP_WriteContent( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return SFTP_CONTENT_STATUS_FAILED;
	}

	char content_status = SFTP_CONTENT_STATUS_NONE;

	if ( context->download_info != NULL )
	{
		if ( !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
		{
			context->wsabuf.buf = NULL;
			context->wsabuf.len = 0;

			// Spin through the data until we get something to write or there's nothing left.
			while ( _SFTP_DownloadData( context->ssh, &context->wsabuf.buf, &context->wsabuf.len ) )
			{
				if ( context->wsabuf.len > 0 )
				{
					break;
				}
				else
				{
					_SFTP_FreeDownloadData( context->wsabuf.buf );
					context->wsabuf.buf = NULL;
					context->wsabuf.len = 0;
				}
			}

			unsigned int output_buffer_length = context->wsabuf.len;

			// Make sure the server isn't feeding us more data than they claim.
			if ( context->header_info.range_info->content_length > 0 &&
			   ( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) > ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
			{
				output_buffer_length -= ( unsigned int )( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) - ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) );
			}

			if ( output_buffer_length > 0 )
			{
				if ( context->download_info->shared_info->hFile != INVALID_HANDLE_VALUE )
				{
					LARGE_INTEGER li;
					li.QuadPart = context->header_info.range_info->file_write_offset;

					InterlockedIncrement( &context->pending_operations );

					context->overlapped.next_operation = IO_SFTPWriteContent;
					context->overlapped.current_operation = IO_WriteFile;

					context->write_wsabuf.buf = context->wsabuf.buf;
					context->write_wsabuf.len = output_buffer_length;

					context->overlapped.overlapped.hEvent = NULL;
					context->overlapped.overlapped.Internal = NULL;
					context->overlapped.overlapped.InternalHigh = NULL;
					//context->overlapped.Pointer = NULL;	// union
					context->overlapped.overlapped.Offset = li.LowPart;
					context->overlapped.overlapped.OffsetHigh = li.HighPart;

					context->content_offset = context->wsabuf.len;	// The true amount that was downloaded. Allows us to resume if we stop the download.

					//context->content_status = SFTP_CONTENT_STATUS_READ_MORE_CONTENT;

					content_status = SFTP_CONTENT_STATUS_READ_MORE_CONTENT;

					EnterCriticalSection( &context->download_info->shared_info->di_cs );
					BOOL bRet = WriteFile( context->download_info->shared_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( OVERLAPPED * )&context->overlapped );
					if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
					{
						InterlockedDecrement( &context->pending_operations );

						context->download_info->status = STATUS_FILE_IO_ERROR;
						context->status = STATUS_FILE_IO_ERROR;

						context->content_status = SFTP_CONTENT_STATUS_FAILED;

						content_status = SFTP_CONTENT_STATUS_FAILED;

						context->content_offset = 0;	// The true amount that was downloaded. Allows us to resume if we stop the download.

						CloseHandle( context->download_info->shared_info->hFile );
						context->download_info->shared_info->hFile = INVALID_HANDLE_VALUE;
					}
					LeaveCriticalSection( &context->download_info->shared_info->di_cs );
				}
				else	// Shouldn't happen.
				{
					return SFTP_CONTENT_STATUS_FAILED;
				}
			}
			/*else	// No more data, get next packet.
			{
				return SFTP_CONTENT_STATUS_NONE;
			}*/
		}
		else	// Simulated. Spin through the data.
		{
			while ( _SFTP_DownloadData( context->ssh, &context->wsabuf.buf, &context->wsabuf.len ) )
			{
				unsigned int output_buffer_length = context->wsabuf.len;

				// Make sure the server isn't feeding us more data than they claim.
				if ( context->header_info.range_info->content_length > 0 &&
				   ( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) > ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
				{
					output_buffer_length -= ( unsigned int )( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) - ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) );
				}

				if ( IS_GROUP( context->download_info ) )
				{
					EnterCriticalSection( &context->download_info->shared_info->di_cs );
					context->download_info->shared_info->downloaded += output_buffer_length;					// The total amount of data (decoded) that was saved/simulated.
					LeaveCriticalSection( &context->download_info->shared_info->di_cs );
				}

				EnterCriticalSection( &context->download_info->di_cs );
				context->download_info->downloaded += output_buffer_length;					// The total amount of data (decoded) that was saved/simulated.
				LeaveCriticalSection( &context->download_info->di_cs );

				EnterCriticalSection( &session_totals_cs );
				g_session_total_downloaded += output_buffer_length;
				cfg_total_downloaded += output_buffer_length;
				LeaveCriticalSection( &session_totals_cs );

				context->header_info.range_info->file_write_offset += output_buffer_length;	// The size of the non-encoded/decoded data that we would have written to a file.

				context->header_info.range_info->content_offset += output_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.

				_SFTP_FreeDownloadData( context->wsabuf.buf );
				context->wsabuf.buf = NULL;
				context->wsabuf.len = 0;

				// We may need to break out of the while loop if the data buffer is larger than the amount of data we want.
				if ( context->header_info.range_info->content_length == 0 ||
				   ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
				{
					return SFTP_CONTENT_STATUS_FAILED;	// We have no more data, so just close the connection.
				}
			}
		}
	}

	return content_status;
}

char SFTP_ProcessRequest( SOCKET_CONTEXT *context, int *packet_type )
{
	if ( context != NULL && context->ssh != NULL && packet_type != NULL )
	{
		INT op_ret;
		int ssh_status;

		*packet_type = 0;

		char i = 0;

		switch ( context->content_status )
		{
			case SFTP_CONTENT_STATUS_WRITE_CONTENT_1:
			case SFTP_CONTENT_STATUS_WRITE_CONTENT_2:
			{
				_SFTP_ProcessWriteRequest( context->ssh, context->ssh_wsabuf.len );

				// Do we need to write more?
				ssh_status = _SFTP_GetStatus( context->ssh );
				if ( ssh_status & SSH_STATUS_WRITE )
				{
					return context->content_status;
				}

				if ( context->content_status == SFTP_CONTENT_STATUS_WRITE_CONTENT_1 )
				{
					break;
				}
			}
			case SFTP_CONTENT_STATUS_READ_MORE_CONTENT_2:
			case SFTP_CONTENT_STATUS_KEY_NOT_FOUND_2:
			case SFTP_CONTENT_STATUS_KEY_MISMATCH_2:
			{
				i = 1;
			}
			break;
		}

		for ( ; i < 2; ++i )
		{
			if ( context->content_status == SFTP_CONTENT_STATUS_NONE )
			{
				_SFTP_ResetPacketInfo( context->ssh, i );
			}

			for ( ;; )
			{
				op_ret = _SFTP_CheckCallbackStatus( context->ssh );
				if ( op_ret == 1 )
				{
					op_ret = _SFTP_CheckBackendStatus( context->ssh );
					if ( op_ret == 1 )
					{
						ssh_status = 0;

						while ( _SFTP_CheckCallbacks( context->ssh ) )
						{
							_SFTP_RunCallbacks( context->ssh );

							// After running our callbacks, do we need to cleanup or write to the SSH object?
							ssh_status = _SFTP_GetStatus( context->ssh );

							op_ret = _SFTP_CheckCallbackStatus( context->ssh );
							if ( op_ret != 1 )
							{
								break;
							}
						}

						if ( ssh_status & SSH_STATUS_CLEANUP )
						{
							if ( ssh_status & SSH_STATUS_AUTHENTICATE )
							{
								context->status = STATUS_AUTH_REQUIRED;
							}

							return SFTP_CONTENT_STATUS_FAILED;
						}
						else if ( ssh_status & SSH_STATUS_KEY_NOT_FOUND )
						{
							return SFTP_CONTENT_STATUS_KEY_NOT_FOUND_1 + i;
						}
						else if ( ssh_status & SSH_STATUS_KEY_MISMATCH )
						{
							return SFTP_CONTENT_STATUS_KEY_MISMATCH_1 + i;
						}
						else if ( ssh_status & SSH_STATUS_WRITE )
						{
							return SFTP_CONTENT_STATUS_WRITE_CONTENT_1 + i;
						}

						if ( ssh_status == 0 )
						{
							return SFTP_CONTENT_STATUS_READ_MORE_CONTENT_1 + i;
						}
					}
					else
					{
						_SFTP_DownloadClose( context->ssh );
						_SFTP_DownloadCleanupPacket( context->ssh );
						_SFTP_DownloadCleanupTransfer( context->ssh );

						return SFTP_CONTENT_STATUS_FAILED;
					}
				}
				else
				{
					break;
				}
			}

			op_ret = _SFTP_GetRequestPacket( context->ssh, i );
			if ( op_ret == 1 )
			{
				return SFTP_CONTENT_STATUS_READ_MORE_CONTENT_1 + i;
			}

			if ( i == 0 )
			{
				_SFTP_PrepareRequestPacket( context->ssh );
			}

			context->content_status = SFTP_CONTENT_STATUS_NONE;
		}

		_SFTP_GetRequestPacketType( context->ssh, packet_type );

		return SFTP_CONTENT_STATUS_NONE;
	}
	else
	{
		return SFTP_CONTENT_STATUS_FAILED;
	}
}

char SFTP_HandleContent( SOCKET_CONTEXT *context, IO_OPERATION current_operation, DWORD io_size )
{
	if ( context == NULL )
	{
		return SFTP_CONTENT_STATUS_FAILED;
	}

	int nRet;
	DWORD dwFlags = 0;

	int packet_type = 0;
	INT op_ret;
	int ssh_status;

	switch ( current_operation )
	{
		case IO_SFTPReadContent:
		{
			if ( io_size == 0 )
			{
				ssh_status = _SFTP_GetStatus( context->ssh );
				if ( ssh_status & SSH_STATUS_INITIALIZED_FILE_HANDLE )
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
					_SFTP_DownloadClose( context->ssh );
				}
				else
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
					_SFTP_BackendClose( context->ssh );
				}

				context->overlapped.current_operation = IO_SFTPWriteContent;
			}
			else
			{
				_SFTP_ProcessGetRequestBuffer( context->ssh, context->buffer, io_size );
			}
		}
		break;

		case IO_SFTPResumeInit:	// A key prompt was shown and we've resumed where we left off.
		{
			ssh_status = _SFTP_GetStatus( context->ssh );
			if ( ssh_status & SSH_STATUS_WRITE )
			{
				if ( context->content_status == SFTP_CONTENT_STATUS_KEY_NOT_FOUND_1 || context->content_status == SFTP_CONTENT_STATUS_KEY_NOT_FOUND_2 )
				{
					context->content_status = ( context->content_status - SFTP_CONTENT_STATUS_KEY_NOT_FOUND ) + SFTP_CONTENT_STATUS_WRITE_CONTENT;
				}
				else if ( context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_1 || context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_2 )
				{
					context->content_status = ( context->content_status - SFTP_CONTENT_STATUS_KEY_MISMATCH ) + SFTP_CONTENT_STATUS_WRITE_CONTENT;
				}
			}

			// Remove key related status bits.
			ssh_status &= ~( SSH_STATUS_KEY_NOT_FOUND | SSH_STATUS_KEY_MISMATCH );
			_SFTP_SetStatus( context->ssh, ssh_status );

			goto RESUME_INIT;
		}
		break;

		case IO_SFTPResumeReadContent:
		{
			if ( context->content_status == SFTP_CONTENT_STATUS_ALLOCATE_FILE )
			{
				EnterCriticalSection( &context->download_info->di_cs );

				context->download_info->status = STATUS_DOWNLOADING;

				LeaveCriticalSection( &context->download_info->di_cs );

				// For groups.
				if ( IS_GROUP( context->download_info ) )
				{
					EnterCriticalSection( &context->download_info->shared_info->di_cs );

					context->download_info->shared_info->status = STATUS_DOWNLOADING;

					LeaveCriticalSection( &context->download_info->shared_info->di_cs );
				}
			}

			context->content_status = SFTP_ProcessFileInfo( context );

			if ( context->content_status != SFTP_CONTENT_STATUS_NONE )
			{
				if ( context->content_status == SFTP_CONTENT_STATUS_FAILED )
				{
					_SFTP_SetStatus( context->ssh, _SFTP_GetStatus( context->ssh ) | SSH_STATUS_BACKEND_CLOSED );
					_SFTP_BackendClose( context->ssh );

					context->overlapped.current_operation = IO_SFTPWriteContent;
				}
				else
				{
					return context->content_status;	// We're allocating or prompting.
				}
			}
			else
			{
				if ( !context->processed_header )
				{
					SFTP_HandleRequest( context );

					context->processed_header = true;
				}

				op_ret = _SFTP_GetHandle( context->ssh, context->request_info.resource );
				if ( op_ret != 0 )
				{
					_SFTP_SetStatus( context->ssh, _SFTP_GetStatus( context->ssh ) | SSH_STATUS_BACKEND_CLOSED );
					_SFTP_BackendClose( context->ssh );
				}

				context->overlapped.current_operation = IO_SFTPWriteContent;
			}
		}
		break;
	}

PROCESS_WRITE:

	context->content_status = SFTP_ProcessRequest( context, &packet_type );

	ssh_status = _SFTP_GetStatus( context->ssh );

	if ( context->content_status == SFTP_CONTENT_STATUS_NONE )
	{
		if ( ssh_status & SSH_STATUS_USER_CLEANUP )
		{
			ssh_status &= ~SSH_STATUS_USER_CLEANUP;
			ssh_status |= SSH_STATUS_BACKEND_CLOSED;

			_SFTP_SetStatus( context->ssh, ssh_status );

			_SFTP_DownloadCleanupPacket( context->ssh );
			_SFTP_DownloadCleanupTransfer( context->ssh );

			op_ret = _SFTP_BackendClose( context->ssh );
			if ( op_ret != 0 )
			{
				context->content_status = SFTP_CONTENT_STATUS_FAILED;
			}
			else
			{
				context->overlapped.current_operation = IO_SFTPWriteContent;
				goto PROCESS_WRITE;
			}
		}
		else if ( ssh_status & SSH_STATUS_BACKEND_CLOSED )
		{
			// Need to clean up any packets that are made since we're not using them.
			op_ret = _SFTP_DownloadCleanupPacket( context->ssh );
			if ( op_ret != 0 )
			{
				context->content_status = SFTP_CONTENT_STATUS_FAILED;
			}
			else
			{
				// Spin out everything in the backend and ignore everything we've received that we would have processed below.
				context->overlapped.current_operation = IO_SFTPWriteContent;
				goto PROCESS_WRITE;
			}
		}
	}

	if ( context->content_status == SFTP_CONTENT_STATUS_FAILED )
	{
		_SFTP_DownloadCleanupTransfer( context->ssh );

		InterlockedIncrement( &context->pending_operations );
		context->overlapped.current_operation = IO_Close;
		PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )&context->overlapped );

		return SFTP_CONTENT_STATUS_FAILED;
	}

	if ( context->content_status == SFTP_CONTENT_STATUS_KEY_NOT_FOUND_1 ||
		 context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_1 ||
		 context->content_status == SFTP_CONTENT_STATUS_KEY_NOT_FOUND_2 ||
		 context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_2 )
	{
		char *key_algorithm = NULL;
		char *key_fingerprint;
		char *md5_key_fingerprint = NULL;
		char *sha256_key_fingerprint = NULL;
		int key_size = 0;
		_SFTP_GetKeyInfo( context->ssh, &key_algorithm, &key_size, &md5_key_fingerprint, &sha256_key_fingerprint );	// Do not free these.

		// We prefer sha256 fingerprints.
		if ( sha256_key_fingerprint != NULL )
		{
			key_fingerprint = sha256_key_fingerprint;
		}
		else if ( md5_key_fingerprint != NULL )
		{
			key_fingerprint = md5_key_fingerprint;
		}
		else
		{
			key_fingerprint = NULL;
		}

		SFTP_FPS_HOST_INFO tsfhi;
		tsfhi.host = context->request_info.host;
		tsfhi.port = context->request_info.port;

		bool do_prompt = true;

		// Check for any key fingerprints.
		DoublyLinkedList *dll_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )&tsfhi, true );
		while ( dll_node != NULL )
		{
			SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dll_node->data;
			if ( sfhi != NULL &&
				 _StrCmpA( sfhi->key_algorithm, key_algorithm ) == 0 &&
			   ( _StrCmpA( sfhi->key_fingerprint, sha256_key_fingerprint ) == 0 || _StrCmpA( sfhi->key_fingerprint, md5_key_fingerprint ) == 0 ) )
			{
				do_prompt = false;

				break;
			}

			dll_node = dll_node->next;
		}

		if ( do_prompt )
		{
			context->status |= STATUS_INPUT_REQUIRED;

			KEY_PROMPT_INFO *kpi = ( KEY_PROMPT_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( KEY_PROMPT_INFO ) );
			if ( kpi != NULL )
			{
				kpi->context = context;

				char num_length = 0;
				if ( context->request_info.port != 22 )
				{
					num_length = 1;	// Include ":".
					unsigned short port = context->request_info.port;
					do
					{
						++num_length; 
						port /= 10;
					}
					while ( port );
				}

				int mb_length = MultiByteToWideChar( CP_UTF8, 0, context->request_info.host, -1, NULL, 0 );	// Include the NULL terminator.
				kpi->w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( mb_length + num_length ) );
				_memset( kpi->w_host, -3, sizeof( wchar_t ) * ( mb_length + num_length ) );
				MultiByteToWideChar( CP_UTF8, 0, context->request_info.host, -1, kpi->w_host, mb_length );
				if ( context->request_info.port != 22 )
				{
					__snwprintf( kpi->w_host + ( mb_length - 1 ), mb_length + num_length, L":%lu", context->request_info.port );
				}

				kpi->host = GlobalStrDupA( context->request_info.host );

				mb_length = MultiByteToWideChar( CP_UTF8, 0, key_algorithm, -1, NULL, 0 );	// Include the NULL terminator.
				kpi->w_key_algorithm = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * mb_length );
				MultiByteToWideChar( CP_UTF8, 0, key_algorithm, -1, kpi->w_key_algorithm, mb_length );

				kpi->key_algorithm = GlobalStrDupA( key_algorithm );

				num_length = 6;	// Include " bits\0".
				unsigned int t_key_size = key_size;
				do
				{
					++num_length; 
					t_key_size /= 10;
				}
				while ( t_key_size );

				kpi->w_key_size = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * num_length );
				__snwprintf( kpi->w_key_size, num_length, L"%lu bits", key_size );

				mb_length = MultiByteToWideChar( CP_UTF8, 0, key_fingerprint, -1, NULL, 0 );	// Include the NULL terminator.
				kpi->w_key_fingerprint = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * mb_length );
				MultiByteToWideChar( CP_UTF8, 0, key_fingerprint, -1, kpi->w_key_fingerprint, mb_length );

				kpi->key_fingerprint = GlobalStrDupA( key_fingerprint );

				int sha256_key_fingerprint_length = lstrlenA( sha256_key_fingerprint );
				int md5_key_fingerprint_length = lstrlenA( md5_key_fingerprint );
				kpi->w_key_fingerprints = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( sha256_key_fingerprint_length + md5_key_fingerprint_length + 3 ) );
				__snwprintf( kpi->w_key_fingerprints, ( sha256_key_fingerprint_length + md5_key_fingerprint_length + 3 ), L"%S\r\n%S", SAFESTRA( sha256_key_fingerprint ), SAFESTRA( md5_key_fingerprint ) );

				kpi->port = context->request_info.port;

				kpi->type = ( context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_1 || context->content_status == SFTP_CONTENT_STATUS_KEY_MISMATCH_2 ? 1 : 0 );

				// Add item to prompt queue and continue.
				EnterCriticalSection( &fingerprint_prompt_list_cs );

				DoublyLinkedList *context_node = DLL_CreateNode( ( void * )kpi );
				DLL_AddNode( &fingerprint_prompt_list, context_node, -1 );

				if ( !fingerprint_prompt_active )
				{
					fingerprint_prompt_active = true;

					// kpi is freed in FingerprintPrompt.
					HANDLE handle_prompt = ( HANDLE )_CreateThread( NULL, 0, PromptFingerprint, ( void * )NULL, 0, NULL );

					// Make sure our thread spawned.
					if ( handle_prompt == NULL )
					{
						GlobalFree( kpi->w_host );
						GlobalFree( kpi->w_key_algorithm );
						GlobalFree( kpi->w_key_size );
						GlobalFree( kpi->w_key_fingerprint );
						GlobalFree( kpi->w_key_fingerprints );
						GlobalFree( kpi->host );
						GlobalFree( kpi->key_algorithm );
						GlobalFree( kpi->key_fingerprint );
						GlobalFree( kpi );

						DLL_RemoveNode( &fingerprint_prompt_list, context_node );
						GlobalFree( context_node );

						fingerprint_prompt_active = false;
					}
					else
					{
						CloseHandle( handle_prompt );
					}
				}
			}

			LeaveCriticalSection( &fingerprint_prompt_list_cs );

			return SFTP_CONTENT_STATUS_NONE;
		}
		else	// Temporary key fingerprint already exists. Continue where we left off.
		{
			// Remove key related status bits.
			ssh_status &= ~( SSH_STATUS_KEY_NOT_FOUND | SSH_STATUS_KEY_MISMATCH );
			_SFTP_SetStatus( context->ssh, ssh_status );

//			goto PROCESS_WRITE;
		}
	}

RESUME_INIT:

	if ( !( ssh_status & SSH_STATUS_INITIALIZED ) )
	{
		if ( _SFTP_CheckInitStatus( context->ssh ) )
		{
			if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
			{
				InterlockedIncrement( &context->pending_operations );
				context->overlapped.current_operation = IO_Close;
				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )&context->overlapped );

				return SFTP_CONTENT_STATUS_FAILED;
			}
			else
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_INITIALIZED );
				_SFTP_InitSendVersion( context->ssh );

				context->overlapped.current_operation = IO_SFTPWriteContent;
				goto PROCESS_WRITE;
			}
		}
	}

	if ( context->content_status == SFTP_CONTENT_STATUS_READ_MORE_CONTENT ||
		 context->content_status == SFTP_CONTENT_STATUS_READ_MORE_CONTENT_1 ||
		 context->content_status == SFTP_CONTENT_STATUS_READ_MORE_CONTENT_2 )
	{
		context->wsabuf.buf = context->buffer;
		context->wsabuf.len = context->buffer_size;

		context->overlapped.current_operation = IO_SFTPReadContent;

		InterlockedIncrement( &context->pending_operations );

		nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )&context->overlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			InterlockedDecrement( &context->pending_operations );

			context->overlapped_close.current_operation = IO_SFTPCleanup;

			if ( ssh_status & SSH_STATUS_USER_CLEANUP )
			{
				ssh_status &= ~SSH_STATUS_USER_CLEANUP;
				ssh_status |= SSH_STATUS_BACKEND_CLOSED;

				_SFTP_SetStatus( context->ssh, ssh_status );

				_SFTP_DownloadCleanupPacket( context->ssh );
				_SFTP_DownloadCleanupTransfer( context->ssh );

				op_ret = _SFTP_BackendClose( context->ssh );
				if ( op_ret != 0 )
				{
					context->content_status = SFTP_CONTENT_STATUS_FAILED;

					_SFTP_DownloadCleanupTransfer( context->ssh );

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_Close;
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )&context->overlapped );

					return SFTP_CONTENT_STATUS_FAILED;
				}
				else
				{
					context->overlapped.current_operation = IO_SFTPWriteContent;
					goto PROCESS_WRITE;
				}
			}
			else if ( ssh_status & SSH_STATUS_BACKEND_CLOSED )
			{
				// Need to clean up any packets that are made since we're not using them.
				op_ret = _SFTP_DownloadCleanupPacket( context->ssh );
				if ( op_ret != 0 )
				{
					context->content_status = SFTP_CONTENT_STATUS_FAILED;

					_SFTP_DownloadCleanupTransfer( context->ssh );

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_Close;
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )&context->overlapped );

					return SFTP_CONTENT_STATUS_FAILED;
				}
				else
				{
					// Spin out everything in the backend and ignore everything we've received that we would have processed below.
					context->overlapped.current_operation = IO_SFTPWriteContent;
					goto PROCESS_WRITE;
				}
			}
			else if ( ssh_status & SSH_STATUS_INITIALIZED_FILE_HANDLE )
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
				_SFTP_DownloadClose( context->ssh );
			}
			else
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}

			context->overlapped.current_operation = IO_SFTPWriteContent;
			goto PROCESS_WRITE;
		}
		else
		{
			return SFTP_CONTENT_STATUS_READ_MORE_CONTENT;
		}
	}
	else if ( context->content_status == SFTP_CONTENT_STATUS_WRITE_CONTENT ||
			  context->content_status == SFTP_CONTENT_STATUS_WRITE_CONTENT_1 ||
			  context->content_status == SFTP_CONTENT_STATUS_WRITE_CONTENT_2 )
	{
		context->wsabuf.buf = context->ssh_wsabuf.buf;
		context->wsabuf.len = context->ssh_wsabuf.len;

		context->overlapped.current_operation = IO_Write;
		context->overlapped.next_operation = IO_SFTPWriteContent;

		InterlockedIncrement( &context->pending_operations );

		nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )&context->overlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			InterlockedDecrement( &context->pending_operations );

			context->overlapped_close.current_operation = IO_SFTPCleanup;

			context->overlapped.current_operation = IO_SFTPWriteContent;
			goto PROCESS_WRITE;
		}
		else
		{
			return SFTP_CONTENT_STATUS_WRITE_CONTENT;
		}
	}
	else	// Reset value if we're not reading/writing more.
	{
		context->content_status = SFTP_CONTENT_STATUS_NONE;
	}

	switch ( packet_type )
	{
		case SSH_FXP_VERSION:
		{
			op_ret = _SFTP_ProcessVersion( context->ssh );
			if ( op_ret != 0 )
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}
			else
			{
				if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
					_SFTP_BackendClose( context->ssh );
				}
				else
				{
					op_ret = _SFTP_GetAttributes( context->ssh, context->request_info.resource );
					if ( op_ret != 0 )
					{
						_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
						_SFTP_BackendClose( context->ssh );
					}
				}
			}
		}
		break;

		case SSH_FXP_ATTRS:
		{
			FILE_ATTRIBUTES file_attributes;

			op_ret = _SFTP_ProcessAttributes( context->ssh, &file_attributes );
			if ( op_ret != 0 )
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}
			else
			{
				if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
					_SFTP_BackendClose( context->ssh );
				}
				else
				{
					if ( context->got_last_modified == 0 )
					{
						if ( !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
						{
							SYSTEMTIME date_time;
							_memzero( &date_time, sizeof( SYSTEMTIME ) );

							UnixTimeToSystemTime( file_attributes.mtime, &date_time );

							SystemTimeToFileTime( &date_time, &context->header_info.last_modified );

							EnterCriticalSection( &context->download_info->di_cs );

							// A new download will have a last_modified value of 0. If it's been set and the times don't match, prompt the user.
							if ( context->download_info->last_modified.QuadPart > 0 &&
							   ( context->download_info->last_modified.HighPart != context->header_info.last_modified.dwHighDateTime ||
								 context->download_info->last_modified.LowPart != context->header_info.last_modified.dwLowDateTime ) )
							{
								context->got_last_modified = 2;	// Prompt.
							}
							else	// New downloads or matching times.
							{
								context->got_last_modified = 1;	// Continue.

								context->download_info->last_modified.HighPart = context->header_info.last_modified.dwHighDateTime;
								context->download_info->last_modified.LowPart = context->header_info.last_modified.dwLowDateTime;
							}

							LeaveCriticalSection( &context->download_info->di_cs );
						}
						else	// Simulation doesn't need to keep looking for it.
						{
							context->got_last_modified = 1;
						}
					}

					context->header_info.range_info->content_length = file_attributes.size;

					// Check the file size threshold (4GB).
					if ( context->header_info.range_info->content_length > cfg_max_file_size )
					{
						if ( !context->processed_header )
						{
							if ( context->download_info != NULL )
							{
								EnterCriticalSection( &context->download_info->shared_info->di_cs );

								// If we've processed the header for a group of hosts, then there's no need to prompt for every other host.
								// A single host will not have had its header processed so we can set this to true to prompt.
								context->show_file_size_prompt = !context->download_info->shared_info->processed_header;

								LeaveCriticalSection( &context->download_info->shared_info->di_cs );
							}
						}
					}

					// Make sure we can split the download into enough parts.
					// Parts will be adjusted in MakeHostRanges() if it's a group download.
					if ( context->download_info != NULL && !IS_GROUP( context->download_info ) &&
						 context->header_info.range_info->content_length < context->parts )
					{
						context->parts = ( context->header_info.range_info->content_length > 0 ? ( unsigned char )context->header_info.range_info->content_length : 1 );
					}

					// Set our range info even if we use one part.
					if ( context->header_info.range_info->content_length > 0 && context->header_info.range_info->range_end == 0 )
					{
						context->header_info.range_info->range_start = 0;
						context->header_info.range_info->range_end = context->header_info.range_info->content_length - 1;
					}

					if ( context->download_info != NULL )
					{
						bool bad_content_length = false;

						EnterCriticalSection( &context->download_info->di_cs );

						context->download_info->parts = context->parts;

						LeaveCriticalSection( &context->download_info->di_cs );

						EnterCriticalSection( &context->download_info->shared_info->di_cs );

						if ( !context->download_info->shared_info->processed_header )
						{
							context->download_info->shared_info->file_size = context->header_info.range_info->content_length;
						}
						else	// We've processed the header and verified the content_length.
						{
							// Make sure the (group) host's content_length is no more than our shared_info->file_size.
							// We'll have set the download_info->file_size in SFTP_HandleRequest().
							if ( IS_GROUP( context->download_info ) )
							{
								if ( context->header_info.range_info->content_length == context->download_info->shared_info->file_size )
								{
									context->header_info.range_info->content_length = context->download_info->file_size;
								}
								else
								{
									bad_content_length = true;
								}
							}
						}

						LeaveCriticalSection( &context->download_info->shared_info->di_cs );

						if ( bad_content_length )
						{
							_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
							_SFTP_BackendClose( context->ssh );

							context->overlapped.current_operation = IO_SFTPWriteContent;
							goto PROCESS_WRITE;
						}

						EnterCriticalSection( &context->download_info->di_cs );

						context->download_info->processed_header = true;

						LeaveCriticalSection( &context->download_info->di_cs );

						if ( !context->processed_header )
						{
							if ( IS_GROUP( context->download_info ) )
							{
								context->download_info->file_size = context->header_info.range_info->content_length;

								MakeHostRanges( context );

								DoublyLinkedList *host_node = context->download_info->shared_info->host_list;
								while ( host_node != NULL )
								{
									DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
									if ( host_di != NULL && host_di != context->download_info )
									{
										EnterCriticalSection( &host_di->di_cs );

										if ( host_di->status == STATUS_NONE )	// status might have been set to stopped when added.
										{
											host_di->status = STATUS_SKIPPED;
										}

										LeaveCriticalSection( &host_di->di_cs );
									}

									host_node = host_node->next;
								}
							}
						}
					}

					context->content_status = SFTP_ProcessFileInfo( context );

					if ( context->content_status != SFTP_CONTENT_STATUS_NONE )
					{
						if ( context->content_status == SFTP_CONTENT_STATUS_FAILED )
						{
							_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
							_SFTP_BackendClose( context->ssh );
						}
						else
						{
							return context->content_status;	// We're allocating or prompting.
						}
					}
					else
					{
						if ( !context->processed_header )
						{
							SFTP_HandleRequest( context );

							context->processed_header = true;
						}

						op_ret = _SFTP_GetHandle( context->ssh, context->request_info.resource );
						if ( op_ret != 0 )
						{
							_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
							_SFTP_BackendClose( context->ssh );
						}
					}
				}
			}
		}
		break;

		case SSH_FXP_HANDLE:
		{
			op_ret = _SFTP_ProcessDownloadHandle( context->ssh );
			if ( op_ret != 0 )
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}
			else
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_INITIALIZED_FILE_HANDLE );

				if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
					_SFTP_DownloadClose( context->ssh );
				}
				else
				{
					op_ret = _SFTP_DownloadInit( context->ssh, context->header_info.range_info->range_start, context->header_info.range_info->range_start + ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 );
					if ( op_ret != 0 )
					{
						_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
						_SFTP_DownloadClose( context->ssh );
					}
				}
			}
		}
		break;

		case SSH_FXP_STATUS:
		case SSH_FXP_DATA:
		{
			if ( ssh_status & SSH_STATUS_INITIALIZED_FILE_HANDLE )
			{
				op_ret = _SFTP_DownloadPrepareData( context->ssh );
				if ( op_ret == 0 )
				{
					op_ret = SFTP_WriteContent( context );
					if ( op_ret != SFTP_CONTENT_STATUS_READ_MORE_CONTENT )
					{
						op_ret = _SFTP_IsDownloadDone( context->ssh );
						if ( op_ret != 0 )	// Transfer is complete.
						{
							_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
							_SFTP_DownloadClose( context->ssh );
						}
						else
						{
							if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
							{
								_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
								_SFTP_DownloadClose( context->ssh );
							}
							else
							{
								op_ret = _SFTP_DownloadQueue( context->ssh );
								if ( op_ret != 0 )
								{
									_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
									_SFTP_DownloadClose( context->ssh );
								}
							}
						}
					}
					else
					{
						return SFTP_CONTENT_STATUS_READ_MORE_CONTENT;
					}
				}
				else
				{
					_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
					_SFTP_DownloadClose( context->ssh );
				}
			}
			else	// File not found and nothing was initialzed.
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}
		}
		break;

		default:
		{
			if ( ssh_status & SSH_STATUS_INITIALIZED_FILE_HANDLE )
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_BACKEND_CLOSED );
				_SFTP_BackendClose( context->ssh );
			}
			else
			{
				_SFTP_SetStatus( context->ssh, ssh_status | SSH_STATUS_USER_CLEANUP );
				_SFTP_BackendClose( context->ssh );
			}	
		}
		break;
	}

	context->overlapped.current_operation = IO_SFTPWriteContent;
	goto PROCESS_WRITE;
}

char SFTP_MakeRangeRequest( SOCKET_CONTEXT *context )
{
	char content_status = SFTP_CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		unsigned long long content_length = context->header_info.range_info->content_length;

		unsigned long long range_size = content_length / context->parts;
		unsigned long long range_offset = range_size;

		// Create a new connection for the remaining parts.
		if ( context->download_info != NULL )
		{
			EnterCriticalSection( &context->download_info->di_cs );

			context->download_info->processed_header = true;

			LeaveCriticalSection( &context->download_info->di_cs );

			// Set this if we're not a group.
			if ( context->download_info->shared_info->hosts == 1 )
			{
				context->header_info.range_info->range_start = context->header_info.range_info->content_offset;
				context->header_info.range_info->content_offset = 0;
			}
		}

		context->header_info.range_info->range_end = range_offset - 1;

		for ( unsigned char part = 2; part <= context->parts; ++part )
		{
			bool skip_context_creation = false;

			if ( context->download_info != NULL )
			{
				EnterCriticalSection( &context->download_info->di_cs );

				// Queue the ranges that won't be downloaded immediately. We'll skip the creation of the context below.
				if ( context->download_info->parts_limit > 0 && part > context->download_info->parts_limit )
				{
					RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );
					if ( ri != NULL )
					{
						ri->range_start = range_offset;

						if ( part < context->parts )
						{
							range_offset += range_size;
							ri->range_end = range_offset - 1;
						}
						else	// Make sure we have an accurate range end for the last part.
						{
							ri->range_end = content_length - 1;
						}

						//ri->content_length = content_length;
						ri->file_write_offset = ri->range_start;

						DoublyLinkedList *range_node = DLL_CreateNode( ( void * )ri );
						DLL_AddNode( &context->download_info->range_list, range_node, -1 );

						if ( context->download_info->range_queue == NULL )
						{
							context->download_info->range_queue = range_node;
						}
					}

					skip_context_creation = true;
				}

				LeaveCriticalSection( &context->download_info->di_cs );
			}

			if ( skip_context_creation )
			{
				continue;
			}

			// Save the request information, the header information (if we got any), and create a new connection.
			SOCKET_CONTEXT *new_context = CreateSocketContext();
			if ( new_context != NULL )
			{
				new_context->processed_header = true;

				new_context->part = part;
				new_context->parts = context->parts;

				new_context->got_filename = context->got_filename;						// No need to rename it again.
				new_context->got_last_modified = context->got_last_modified;			// No need to get the date/time again.
				new_context->show_file_size_prompt = context->show_file_size_prompt;	// No need to prompt again.

				new_context->request_info.host = GlobalStrDupA( context->request_info.host );
				new_context->request_info.port = context->request_info.port;
				new_context->request_info.resource = GlobalStrDupA( context->request_info.resource );
				new_context->request_info.protocol = context->request_info.protocol;

				RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );
				if ( ri != NULL )
				{
					new_context->header_info.range_info = ri;

					new_context->header_info.range_info->range_start = range_offset;// + 1;

					if ( new_context->part < context->parts )
					{
						range_offset += range_size;
						new_context->header_info.range_info->range_end = range_offset - 1;
					}
					else	// Make sure we have an accurate range end for the last part.
					{
						new_context->header_info.range_info->range_end = content_length - 1;
					}

					new_context->header_info.range_info->content_length = content_length;
					new_context->header_info.range_info->file_write_offset = new_context->header_info.range_info->range_start;

					//new_context->request_info.redirect_count = context->request_info.redirect_count;	// This is being used to determine whether we've switched modes (fallback mode).
					//new_context->header_info.connection = context->header_info.connection;				// This is being used as our mode value. (cfg_ftp_mode_type)
					//

					new_context->context_node.data = new_context;

					EnterCriticalSection( &context_list_cs );

					DLL_AddNode( &g_context_list, &new_context->context_node, 0 );

					LeaveCriticalSection( &context_list_cs );

					// Add to the parts list.
					if ( context->download_info != NULL )
					{
						EnterCriticalSection( &context->download_info->di_cs );

						new_context->download_info = context->download_info;

						++( new_context->download_info->active_parts );

						DoublyLinkedList *range_node = DLL_CreateNode( ( void * )ri );
						DLL_AddNode( &new_context->download_info->range_list, range_node, -1 );

						new_context->parts_node.data = new_context;
						DLL_AddNode( &new_context->download_info->parts_list, &new_context->parts_node, -1 );

						LeaveCriticalSection( &context->download_info->di_cs );

						EnterCriticalSection( &context->download_info->shared_info->di_cs );

						// For groups.
						if ( IS_GROUP( context->download_info ) )
						{
							++( context->download_info->shared_info->active_parts );
						}

						LeaveCriticalSection( &context->download_info->shared_info->di_cs );
					}

					new_context->status = STATUS_CONNECTING;

					if ( !CreateConnection( new_context, new_context->request_info.host, new_context->request_info.port ) )
					{
						new_context->status = STATUS_FAILED;

						InterlockedIncrement( &new_context->pending_operations );

						new_context->overlapped.current_operation = IO_Close;

						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )new_context, ( OVERLAPPED * )&new_context->overlapped );
					}
				}
			}
		}

		content_status = SFTP_CONTENT_STATUS_NONE;
	}

	return content_status;
}

char SFTP_HandleRequest( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return SFTP_CONTENT_STATUS_FAILED;
	}

	if ( !context->processed_header )
	{
		// Handle group parts if downloading from multiple hosts.
		if ( context->download_info != NULL )
		{
			EnterCriticalSection( &context->download_info->shared_info->di_cs );

			if ( IS_GROUP( context->download_info ) && !context->download_info->shared_info->processed_header )
			{
				context->download_info->shared_info->processed_header = true;

				LeaveCriticalSection( &context->download_info->shared_info->di_cs );

				DoublyLinkedList *host_node = context->download_info->shared_info->host_list;
				while ( host_node != NULL )
				{
					DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )host_node->data;
					// We don't need to start the context's download since it's already started.
					if ( di != NULL && di != context->download_info )
					{
						if ( di->processed_header )	// All usable hosts will have had their header processed in MakeHostRanges().
						{
							bool skip_start = false;

							EnterCriticalSection( &di->di_cs );

							// Skip starting downloads that were added in the stopped state.
							if ( di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
							{
								di->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
								di->status = STATUS_STOPPED;

								LeaveCriticalSection( &di->di_cs );

								EnterCriticalSection( &context->download_info->shared_info->di_cs );
								context->download_info->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
								LeaveCriticalSection( &context->download_info->shared_info->di_cs );

								skip_start = true;
							}
							else
							{
								LeaveCriticalSection( &di->di_cs );
							}

							if ( !skip_start )
							{
								StartDownload( di, START_TYPE_HOST_IN_GROUP, START_OPERATION_NONE );
							}
						}
					}

					host_node = host_node->next;
				}
			}
			else
			{
				LeaveCriticalSection( &context->download_info->shared_info->di_cs );
			}

			if ( context->parts > 1 )
			{
				SFTP_MakeRangeRequest( context );
			}
		}
	}

	return SFTP_CONTENT_STATUS_HANDLE_REQUEST;
}

THREAD_RETURN load_sftp_fps_host_list( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM | LVIF_TEXT;

	node_type *node = dllrbt_get_head( g_sftp_fps_host_info );
	while ( node != NULL )
	{
		DoublyLinkedList *dll_node = ( DoublyLinkedList * )node->val;
		while ( dll_node != NULL )
		{
			SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dll_node->data;

			if ( sfhi != NULL )
			{
				lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 );
				lvi.lParam = ( LPARAM )sfhi;
				lvi.pszText = sfhi->w_host;
				_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
			}

			dll_node = dll_node->next;
		}

		node = node->next;
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

void ParseSFTPHost( wchar_t *w_host, char **host, unsigned short &port )
{
	port = 0;

	wchar_t *str_pos_start = w_host;

	if ( str_pos_start != NULL )
	{
		wchar_t *str_pos_end = str_pos_start;

		wchar_t *str_port_start = str_pos_start;
		while ( *str_port_start != NULL )
		{
			if ( *str_port_start == L':' )
			{
				// If we have a well formed IPv6 address, then see if there was a port assigned to it.
				if ( *str_pos_start == L'[' && str_port_start > str_pos_start && *( str_port_start - 1 ) != L']' )
				{
					++str_port_start;

					str_pos_end = str_port_start;

					continue;
				}

				wchar_t tmp_end = *str_pos_end;
				*str_pos_end = 0;	// Temporary string terminator.
				int num = _wcstoul( str_port_start + 1, NULL, 10 );
				*str_pos_end = tmp_end;	// Restore string.

				port = ( unsigned short )( num > 65535 ? 0 : num );

				str_pos_end = str_port_start;	// New end of host.

				break;
			}

			++str_port_start;

			str_pos_end = str_port_start;
		}

		unsigned int host_length = ( unsigned int )( str_pos_end - str_pos_start );

		int key_fingerprint_length = WideCharToMultiByte( CP_UTF8, 0, str_pos_start, host_length, NULL, 0, NULL, NULL ) + 1;
		*host = ( char * )GlobalAlloc( GPTR, sizeof( char ) * key_fingerprint_length ); // Size includes the NULL character.
		if ( *host != NULL )
		{
			key_fingerprint_length = WideCharToMultiByte( CP_UTF8, 0, str_pos_start, host_length, *host, key_fingerprint_length, NULL, NULL );
			*( *host + key_fingerprint_length ) = 0;	// Sanity
		}

		if ( port == 0 )
		{
			port = 22;
		}
	}
}

THREAD_RETURN PromptFingerprint( void * /*pArguments*/ )
{
	bool skip_processing = false;

	do
	{
		KEY_PROMPT_INFO *kpi = NULL;
		SOCKET_CONTEXT *context = NULL;

		EnterCriticalSection( &fingerprint_prompt_list_cs );

		DoublyLinkedList *kpi_node = fingerprint_prompt_list;

		DLL_RemoveNode( &fingerprint_prompt_list, kpi_node );

		if ( kpi_node != NULL )
		{
			kpi = ( KEY_PROMPT_INFO * )kpi_node->data;
			context = kpi->context;

			GlobalFree( kpi_node );
		}

		LeaveCriticalSection( &fingerprint_prompt_list_cs );

		if ( context != NULL )
		{
			DOWNLOAD_INFO *di = context->download_info;

			// See if we added the key fingerprint already.
			// Don't prompt if we did.
			SFTP_FPS_HOST_INFO tsfhi;
			tsfhi.host = kpi->host;
			tsfhi.port = kpi->port;

			// Check for any key fingerprints.
			DoublyLinkedList *dll_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )&tsfhi, true );
			while ( dll_node != NULL )
			{
				SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dll_node->data;
				if ( sfhi != NULL &&
					_StrCmpA( sfhi->key_algorithm, kpi->key_algorithm ) == 0 &&
					_StrCmpA( sfhi->key_fingerprint, kpi->key_fingerprint ) == 0 )
				{
					di = NULL;

					break;
				}

				dll_node = dll_node->next;
			}

			if ( di != NULL )
			{
				if ( g_fingerprint_cmb_ret != CMBIDNOALL && g_fingerprint_cmb_ret != CMBIDYESALL )
				{
					wchar_t *prompt_message;
					UINT prompt_type;

					if ( kpi->type == 1 )	// Mismatch.
					{
						prompt_message = ST_V_PROMPT_mismatch_accept_the_server_host_key;

						prompt_type = CMB_ICONHAND;
					}
					else	// Not found.
					{
						prompt_message = ST_V_PROMPT_accept_the_server_host_key;

						prompt_type = CMB_ICONQUESTION;
					}

					g_fingerprint_cmb_ret = CPromptW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, prompt_type, ( void * )kpi );
				}

				EnterCriticalSection( &context->context_cs );

				context->status &= ~STATUS_INPUT_REQUIRED;

				// Remove key related status bits.
				int sftp_status = _SFTP_GetStatus( context->ssh ) & ~( SSH_STATUS_KEY_NOT_FOUND | SSH_STATUS_KEY_MISMATCH );
				_SFTP_SetStatus( context->ssh, sftp_status );

				// Close download.
				if ( g_fingerprint_cmb_ret == CMBIDNO )
				{
					// We will not have gotten the file_size at this point.
					if ( IS_GROUP( context->download_info ) )
					{
						EnterCriticalSection( &context->download_info->shared_info->di_cs );

						context->download_info->shared_info->status = STATUS_SKIPPED;

						DoublyLinkedList *host_node = context->download_info->shared_info->host_list;
						while ( host_node != NULL )
						{
							DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
							if ( host_di != NULL )
							{
								EnterCriticalSection( &host_di->di_cs );

								host_di->status = STATUS_SKIPPED;

								LeaveCriticalSection( &host_di->di_cs );
							}

							host_node = host_node->next;
						}

						LeaveCriticalSection( &context->download_info->shared_info->di_cs );
					}

					context->status = STATUS_SKIPPED;

					context->overlapped_close.current_operation = IO_SFTPCleanup;

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_SFTPReadContent;//IO_SFTPResumeInit;
					PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
				else	// Continue where we left off when getting the content.
				{
					bool add_to_tree = true;

					tsfhi.host = context->request_info.host;
					tsfhi.port = context->request_info.port;

					DoublyLinkedList *sfhi_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )&tsfhi, true );
					if ( sfhi_node != NULL && g_fingerprint_cmb_ret == CMBIDYESADD )
					{
						DoublyLinkedList *tmp_node = sfhi_node;
						while ( tmp_node != NULL )
						{
							SFTP_FPS_HOST_INFO *cached_sfhi = ( SFTP_FPS_HOST_INFO * )tmp_node->data;

							if ( _StrCmpA( kpi->key_algorithm, cached_sfhi->key_algorithm ) == 0 )
							{
								char *tmp_key_fingerprint = cached_sfhi->key_fingerprint;
								cached_sfhi->key_fingerprint = kpi->key_fingerprint;
								GlobalFree( tmp_key_fingerprint );
								kpi->key_fingerprint = NULL;

								wchar_t *tmp_w_key_fingerprint = cached_sfhi->w_key_fingerprint;
								cached_sfhi->w_key_fingerprint = kpi->w_key_fingerprint;
								GlobalFree( tmp_w_key_fingerprint );
								kpi->w_key_fingerprint = NULL;

								cached_sfhi->temporary = false;

								if ( g_hWnd_sftp_fps_host_list != NULL )
								{
									_InvalidateRect( g_hWnd_sftp_fps_host_list, NULL, FALSE );
								}

								sftp_fps_host_list_changed = true;

								add_to_tree = false;

								break;
							}

							tmp_node = tmp_node->next;
						}
					}

					if ( add_to_tree )
					{
						SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( SFTP_FPS_HOST_INFO ) );
						sfhi->host = kpi->host;
						sfhi->key_algorithm = kpi->key_algorithm;
						sfhi->key_fingerprint = kpi->key_fingerprint;
						sfhi->port = kpi->port;
						sfhi->w_host = kpi->w_host;
						sfhi->w_key_algorithm = kpi->w_key_algorithm;
						sfhi->w_key_fingerprint = kpi->w_key_fingerprint;
						sfhi->temporary = ( g_fingerprint_cmb_ret == CMBIDYESADD ? false : true );

						kpi->host = NULL;
						kpi->key_algorithm = NULL;
						kpi->key_fingerprint = NULL;
						kpi->w_host = NULL;
						kpi->w_key_algorithm = NULL;
						kpi->w_key_fingerprint = NULL;

						DoublyLinkedList *new_sfhi_node = DLL_CreateNode( ( void * )sfhi );
						if ( sfhi_node != NULL )
						{
							DLL_AddNode( &sfhi_node, new_sfhi_node, -1 );
						}
						else if ( dllrbt_insert( g_sftp_fps_host_info, ( void * )sfhi, ( void * )new_sfhi_node ) != DLLRBT_STATUS_OK )
						{
							GlobalFree( new_sfhi_node );

							FreeSFTPFpsHostInfo( &sfhi );
						}

						if ( sfhi != NULL )
						{
							if ( g_hWnd_sftp_fps_host_list != NULL )
							{
								LVITEM lvi;
								_memzero( &lvi, sizeof( LVITEM ) );
								lvi.mask = LVIF_PARAM | LVIF_TEXT;
								lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 );
								lvi.lParam = ( LPARAM )sfhi;
								lvi.pszText = sfhi->w_host;
								_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

								_InvalidateRect( g_hWnd_sftp_fps_host_list, NULL, FALSE );
							}

							if ( g_fingerprint_cmb_ret == CMBIDYESADD )
							{
								sftp_fps_host_list_changed = true;
							}
						}
					}

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_SFTPResumeInit;
					PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else	// No DOWNLOAD_INFO? Then skip. If we do have it, then continue were we left off.
			{
				EnterCriticalSection( &context->context_cs );

				InterlockedIncrement( &context->pending_operations );

				if ( context->download_info != NULL )
				{
					context->status &= ~STATUS_INPUT_REQUIRED;

					context->overlapped.current_operation = IO_SFTPResumeInit;
					PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
				else
				{
					context->status = STATUS_SKIPPED;

					context->overlapped_close.current_operation = IO_SFTPCleanup;

					context->overlapped.current_operation = IO_SFTPReadContent;
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}

			LeaveCriticalSection( &context->context_cs );
		}

		if ( kpi != NULL )
		{
			GlobalFree( kpi->w_host );
			GlobalFree( kpi->w_key_algorithm );
			GlobalFree( kpi->w_key_size );
			GlobalFree( kpi->w_key_fingerprint );
			GlobalFree( kpi->w_key_fingerprints );
			GlobalFree( kpi->host );
			GlobalFree( kpi->key_algorithm );
			GlobalFree( kpi->key_fingerprint );
			GlobalFree( kpi );
		}

		EnterCriticalSection( &fingerprint_prompt_list_cs );

		if ( fingerprint_prompt_list == NULL )
		{
			skip_processing = true;

			fingerprint_prompt_active = false;
		}

		LeaveCriticalSection( &fingerprint_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_sftp_fps_host_list( void *pArguments )
{
	SFTP_FPS_HOST_UPDATE_INFO *sfhui = ( SFTP_FPS_HOST_UPDATE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( sfhui != NULL )
	{
		if ( sfhui->update_type == 0 && sfhui->sfhi != NULL )	// Add
		{
			SFTP_FPS_HOST_INFO *sfhi = sfhui->sfhi;

#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Adding %S to SFTP fingerprint list", sfhi->w_host );
#endif

			char fail_type = 0;

			ParseSFTPHost( sfhi->w_host, &sfhi->host, sfhi->port );

			int wcs_length = WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_algorithm, -1, NULL, 0, NULL, NULL );
			sfhi->key_algorithm = ( char * )GlobalAlloc( GPTR, sizeof( char ) * wcs_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_algorithm, -1, sfhi->key_algorithm, wcs_length, NULL, NULL );

			wcs_length = WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_fingerprint, -1, NULL, 0, NULL, NULL );
			sfhi->key_fingerprint = ( char * )GlobalAlloc( GPTR, sizeof( char ) * wcs_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_fingerprint, -1, sfhi->key_fingerprint, wcs_length, NULL, NULL );

			sfhi->temporary = false;

			DoublyLinkedList *sfhi_node;
			DoublyLinkedList *tmp_node;
			SFTP_FPS_HOST_INFO *cached_sfhi;

			// Find the host info.
			// Does it already exist?
			dllrbt_iterator *new_itr = dllrbt_find( g_sftp_fps_host_info, ( void * )sfhi, false );
			if ( new_itr != NULL )
			{
				sfhi_node = ( DoublyLinkedList * )( ( node_type * )new_itr )->val;

				tmp_node = sfhi_node;
				while ( tmp_node != NULL )
				{
					cached_sfhi = ( SFTP_FPS_HOST_INFO * )tmp_node->data;

					if ( _StrCmpA( sfhi->key_algorithm, cached_sfhi->key_algorithm ) == 0 )
					{
						if ( _StrCmpA( sfhi->key_fingerprint, cached_sfhi->key_fingerprint ) == 0 )
						{
							if ( cached_sfhi == sfhui->old_sfhi )
							{
								// See if we're saving a temporary fingerprint.
								if ( cached_sfhi->temporary )
								{
									cached_sfhi->temporary = false;

									sftp_fps_host_list_changed = true;
								}

								fail_type = -1;	// Already exists, but don't warn.
							}
							else
							{
								fail_type = 1;	// Already exits.
							}

							break;
						}
						else if ( sfhui->old_sfhi != NULL && sfhui->old_sfhi->temporary && cached_sfhi != sfhui->old_sfhi ) // Mismatched key fingerprint when saving temporary value.
						{
							sfhui->old_sfhi->temporary = false;

							LVITEM lvi;
							_memzero( &lvi, sizeof( LVITEM ) );
							lvi.mask = LVIF_PARAM;

							int item_count = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 );

							// Go through each item.
							for ( int i = 0; i < item_count; ++i )
							{
								// Stop processing and exit the thread.
								if ( kill_worker_thread_flag )
								{
									break;
								}

								lvi.iItem = i;
								_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

								if ( ( SFTP_FPS_HOST_INFO * )lvi.lParam == cached_sfhi )
								{
									// Prevent the listviews from drawing while freeing lParam values.
									skip_sftp_fps_host_list_draw = true;

									_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_DELETEITEM, i, 0 );

									DLL_RemoveNode( &sfhi_node, tmp_node );

									GlobalFree( tmp_node );

									// Remove the old tree item and re-add it if there's still DoublyLinkedList nodes.
									dllrbt_remove( g_sftp_fps_host_info, new_itr );

									if ( sfhi_node != NULL )
									{
										dllrbt_insert( g_sftp_fps_host_info, sfhi_node->data, ( void * )sfhi_node );
									}

									FreeSFTPFpsHostInfo( &cached_sfhi );

									skip_sftp_fps_host_list_draw = false;

									break;
								}
							}

							sftp_fps_host_list_changed = true;

							fail_type = -1;	// Already exists, but don't warn.

							break;
						}
					}

					tmp_node = tmp_node->next;
				}
			}

			if ( fail_type == 0 )
			{
				if ( sfhui->old_sfhi != NULL )
				{
					// Find the host info
					dllrbt_iterator *old_itr = dllrbt_find( g_sftp_fps_host_info, ( void * )sfhui->old_sfhi, false );
					if ( old_itr != NULL )
					{
						sfhi_node = ( DoublyLinkedList * )( ( node_type * )old_itr )->val;

						tmp_node = sfhi_node;
						while ( tmp_node != NULL )
						{
							cached_sfhi = ( SFTP_FPS_HOST_INFO * )tmp_node->data;

							if ( _StrCmpA( sfhui->old_sfhi->key_algorithm, cached_sfhi->key_algorithm ) == 0 &&
								 _StrCmpA( sfhui->old_sfhi->key_fingerprint, cached_sfhi->key_fingerprint ) == 0 )
							{
								DLL_RemoveNode( &sfhi_node, tmp_node );

								GlobalFree( tmp_node );

								break;
							}

							tmp_node = tmp_node->next;
						}

						// Remove the old tree item and re-add it if there's still DoublyLinkedList nodes.
						dllrbt_remove( g_sftp_fps_host_info, old_itr );

						if ( sfhi_node != NULL )
						{
							dllrbt_insert( g_sftp_fps_host_info, sfhi_node->data, ( void * )sfhi_node );
						}
						else if ( old_itr == new_itr )	// Don't want to add to a nonexistent tree.
						{
							new_itr = NULL;
						}
					}
				}

				// Add the new info to an existing DoublyLinkedList or the tree.
				DoublyLinkedList *new_sfhi_node = DLL_CreateNode( ( void * )sfhi );
				if ( new_itr != NULL )
				{
					sfhi_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )sfhi, true );
					DLL_AddNode( &sfhi_node, new_sfhi_node, -1 );
				}
				else if ( dllrbt_insert( g_sftp_fps_host_info, ( void * )sfhi, ( void * )new_sfhi_node ) != DLLRBT_STATUS_OK )
				{
					GlobalFree( new_sfhi_node );

					fail_type = 1;	// Already exits.
				}
			}

			if ( fail_type == 0 )
			{
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM | LVIF_TEXT;
				lvi.lParam = ( LPARAM )sfhi;
				lvi.pszText = sfhi->w_host;

				if ( sfhui->old_sfhi != NULL )
				{
					lvi.iItem = sfhui->index;
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_SETITEM, 0, ( LPARAM )&lvi );

					FreeSFTPFpsHostInfo( &sfhui->old_sfhi );
				}
				else
				{
					lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 );
					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					_SendMessageW( g_hWnd_sftp_fps_tab, WM_PROPAGATE, 3, ( LPARAM )sfhui );	// Update selected host.
				}

				sftp_fps_host_list_changed = true;
			}
			else
			{
				FreeSFTPFpsHostInfo( &sfhi );

				if ( fail_type != 2 )
				{
					_SendNotifyMessageW( g_hWnd_sftp_fps_tab, WM_PROPAGATE, fail_type, 0 );
				}
			}
		}
		else if ( sfhui->update_type == 1 )	// Remove
		{
			// Prevent the listviews from drawing while freeing lParam values.
			skip_sftp_fps_host_list_draw = true;

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			int item_count = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			int *index_array = NULL;

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_ENSUREVISIBLE, 0, FALSE );

				index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );
				if ( index_array != NULL )
				{
					lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

					_EnableWindow( g_hWnd_sftp_fps_host_list, FALSE );	// Prevent any interaction with the listview while we're processing.

					// Create an index list of selected items (in reverse order).
					for ( int i = 0; i < sel_count; ++i )
					{
						lvi.iItem = index_array[ sel_count - 1 - i ] = ( int )_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
					}

					_EnableWindow( g_hWnd_sftp_fps_host_list, TRUE );	// Allow the listview to be interactive.
				}

				item_count = sel_count;
			}

			if ( handle_all || index_array != NULL )
			{
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

					_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )lvi.lParam;

					if ( !handle_all )
					{
						_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_DELETEITEM, index_array[ i ], 0 );
					}
					else if ( i >= ( item_count - 1 ) )
					{
						_SendMessageW( g_hWnd_sftp_fps_host_list, LVM_DELETEALLITEMS, 0, 0 );
					}

					if ( sfhi != NULL )
					{
#ifdef ENABLE_LOGGING
						WriteLog( LOG_INFO_ACTION, "Removing %S from SFTP fingerprint list", sfhi->w_host );
#endif
						// Find the host info
						dllrbt_iterator *old_itr = dllrbt_find( g_sftp_fps_host_info, ( void * )sfhi, false );
						if ( old_itr != NULL )
						{
							DoublyLinkedList *sfhi_node = ( DoublyLinkedList * )( ( node_type * )old_itr )->val;

							DoublyLinkedList *tmp_node = sfhi_node;
							while ( tmp_node != NULL )
							{
								SFTP_FPS_HOST_INFO *cached_sfhi = ( SFTP_FPS_HOST_INFO * )tmp_node->data;

								if ( _StrCmpA( sfhi->key_algorithm, cached_sfhi->key_algorithm ) == 0 &&
									 _StrCmpA( sfhi->key_fingerprint, cached_sfhi->key_fingerprint ) == 0 )
								{
									DLL_RemoveNode( &sfhi_node, tmp_node );

									GlobalFree( tmp_node );

									break;
								}

								tmp_node = tmp_node->next;
							}

							// Remove the old tree item and re-add it if there's still DoublyLinkedList nodes.
							dllrbt_remove( g_sftp_fps_host_info, old_itr );

							if ( sfhi_node != NULL )
							{
								dllrbt_insert( g_sftp_fps_host_info, sfhi_node->data, ( void * )sfhi_node );
							}
						}

						FreeSFTPFpsHostInfo( &sfhi );
					}
				}
			}

			_SendMessageW( g_hWnd_sftp_fps_tab, WM_PROPAGATE, 2, 0 );	// Disable remove button.

			sftp_fps_host_list_changed = true;

			skip_sftp_fps_host_list_draw = false;

			if ( index_array != NULL )
			{
				GlobalFree( index_array );
			}
		}

		GlobalFree( sfhui );
	}

	_InvalidateRect( g_hWnd_sftp_fps_host_list, NULL, FALSE );

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

void FreeSFTPFpsHostInfo( SFTP_FPS_HOST_INFO **sftp_fps_host_info )
{
	if ( *sftp_fps_host_info != NULL )
	{
		if ( ( *sftp_fps_host_info )->w_host != NULL ) { GlobalFree( ( *sftp_fps_host_info )->w_host ); }
		if ( ( *sftp_fps_host_info )->host != NULL ) { GlobalFree( ( *sftp_fps_host_info )->host ); }
		if ( ( *sftp_fps_host_info )->w_key_algorithm != NULL ) { GlobalFree( ( *sftp_fps_host_info )->w_key_algorithm ); }
		if ( ( *sftp_fps_host_info )->key_algorithm != NULL ) { GlobalFree( ( *sftp_fps_host_info )->key_algorithm ); }
		if ( ( *sftp_fps_host_info )->w_key_fingerprint != NULL ) { GlobalFree( ( *sftp_fps_host_info )->w_key_fingerprint ); }
		if ( ( *sftp_fps_host_info )->key_fingerprint != NULL ) { GlobalFree( ( *sftp_fps_host_info )->key_fingerprint ); }

		GlobalFree( *sftp_fps_host_info );

		*sftp_fps_host_info = NULL;
	}
}

int dllrbt_compare_sftp_fps_host_info( void *a, void *b )
{
	SFTP_FPS_HOST_INFO *a1 = ( SFTP_FPS_HOST_INFO * )a;
	SFTP_FPS_HOST_INFO *b1 = ( SFTP_FPS_HOST_INFO * )b;

	if ( a1 == b1 )
	{
		return 0;
	}

	int ret = lstrcmpA( a1->host, b1->host );
	if ( ret == 0 )
	{
		if ( a1->port > b1->port )
		{
			return 1;
		}
		else if ( a1->port < b1->port )
		{
			return -1;
		}
	}

	return ret;
}

char *CreateKeyInfoString( DoublyLinkedList *dll_node )
{
	char *ret = NULL;
	int ret_length = sizeof( unsigned int );
	unsigned int info_count = 0;

	while ( dll_node != NULL )
	{
		SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dll_node->data;

		if ( sfhi != NULL )
		{
			int key_algorithm_length = lstrlenA( sfhi->key_algorithm );
			int key_fingerprint_length = lstrlenA( sfhi->key_fingerprint );

			if ( ret != NULL )
			{
				char *reallocated_ret = ( char * )GlobalReAlloc( ret, sizeof( char ) * ( ret_length + ( key_fingerprint_length + 1 + key_algorithm_length ) + 1 + sizeof( unsigned int ) ), GMEM_MOVEABLE );
				if ( reallocated_ret == NULL )
				{
					break;
				}

				ret = reallocated_ret;
			}
			else
			{
				ret = ( char * )GlobalAlloc( GMEM_FIXED, ( key_fingerprint_length + 1 + key_algorithm_length ) + 1 + sizeof( unsigned int ) );
			}

			if ( ret != NULL )
			{
				_memcpy_s( ret + ret_length, key_algorithm_length, sfhi->key_algorithm, key_algorithm_length );
				ret_length += key_algorithm_length;
				ret[ ret_length++ ] = '\n';
				_memcpy_s( ret + ret_length, key_fingerprint_length, sfhi->key_fingerprint, key_fingerprint_length );
				ret_length += key_fingerprint_length;
				if ( dll_node->next != NULL )
				{
					ret[ ret_length++ ] = '\n';
				}
				else
				{
					ret[ ret_length ] = 0;	// Sanity.
				}

				++info_count;
			}
		}

		dll_node = dll_node->next;
	}

	if ( ret != NULL )
	{
		_memcpy_s( ret, sizeof( unsigned int ), &info_count, sizeof( unsigned int ) );
	}

	return ret;
}

char read_sftp_fps_host_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\sftp_fingerprint_settings\0", 27 );
	//g_base_directory[ g_base_directory_length + 26 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading SFTP fingerprints: %S", g_base_directory );
#endif

	HANDLE hFile_read = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_read = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_read, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		wchar_t *host;
		wchar_t *key_algorithm;
		wchar_t *key_fingerprint;

		//

		unsigned char magic_identifier[ 4 ];
		BOOL bRet = ReadFile( hFile_read, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
		if ( bRet != FALSE )
		{
#ifdef ENABLE_LOGGING
			lfz += 4;
#endif
			unsigned char version = magic_identifier[ 3 ] - 0x30;

			if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_SFTP_HOSTS, 3 ) == 0 && version <= 0x0F )
			{
				char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 3 ) );	// 512 KB buffer.
				if ( buf != NULL )
				{
					DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

					while ( total_read < fz )
					{
						bRet = ReadFile( hFile_read, buf, sizeof( char ) * 524288, &read, NULL );
						if ( bRet == FALSE )
						{
							break;
						}

	#ifdef ENABLE_LOGGING
						lfz += read;
	#endif

						buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

						// This terminates wide character strings so we don't read past the buffer.
						buf[ read + 1 ] = 0;
						buf[ read + 2 ] = 0;

						total_read += read;

						// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
						// If it's larger than our buffer, then the file is probably invalid/corrupt.
						if ( total_read == last_total )
						{
							break;
						}

						last_total = total_read;

						p = buf;
						offset = last_entry = 0;

						while ( offset < read )
						{
							host = NULL;
							key_algorithm = NULL;
							key_fingerprint = NULL;

							//

							// Host
							int string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( host, string_length, p, string_length );
							*( host + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// Key Algrithm
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset > read ) { goto CLEANUP; }

							key_algorithm = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( key_algorithm, string_length, p, string_length );
							*( key_algorithm + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// Key Fingerprint
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset > read ) { goto CLEANUP; }

							key_fingerprint = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( key_fingerprint, string_length, p, string_length );
							*( key_fingerprint + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							//

							last_entry = offset;	// This value is the ending offset of the last valid entry.

							SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )GlobalAlloc( GPTR, sizeof( SFTP_FPS_HOST_INFO ) );
							if ( sfhi != NULL )
							{
								sfhi->w_host = host;
								sfhi->w_key_algorithm = key_algorithm;
								sfhi->w_key_fingerprint = key_fingerprint;

								ParseSFTPHost( sfhi->w_host, &sfhi->host, sfhi->port );

								int wcs_length;
								if ( sfhi->w_key_algorithm != NULL )
								{
									wcs_length = WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_algorithm, -1, NULL, 0, NULL, NULL );
									sfhi->key_algorithm = ( char * )GlobalAlloc( GPTR, sizeof( char ) * wcs_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_algorithm, -1, sfhi->key_algorithm, wcs_length, NULL, NULL );
								}

								if ( sfhi->w_key_fingerprint != NULL )
								{
									wcs_length = WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_fingerprint, -1, NULL, 0, NULL, NULL );
									sfhi->key_fingerprint = ( char * )GlobalAlloc( GPTR, sizeof( char ) * wcs_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, sfhi->w_key_fingerprint, -1, sfhi->key_fingerprint, wcs_length, NULL, NULL );
								}

								//

								DoublyLinkedList *sfhi_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )sfhi, true );

								DoublyLinkedList *new_sfhi_node = DLL_CreateNode( ( void * )sfhi );
								if ( sfhi_node != NULL )
								{
									DLL_AddNode( &sfhi_node, new_sfhi_node, -1 );
								}
								else if ( dllrbt_insert( g_sftp_fps_host_info, ( void * )sfhi, ( void * )new_sfhi_node ) != DLLRBT_STATUS_OK )
								{
									GlobalFree( new_sfhi_node );

									FreeSFTPFpsHostInfo( &sfhi );
								}
							}
							else
							{
								GlobalFree( host );
								GlobalFree( key_algorithm );
								GlobalFree( key_fingerprint );
							}

							continue;

			CLEANUP:
							GlobalFree( host );
							GlobalFree( key_algorithm );
							GlobalFree( key_fingerprint );

							// Go back to the last valid entry.
							if ( total_read < fz )
							{
								total_read -= ( read - last_entry );
								SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
							}

							break;
						}
					}

					GlobalFree( buf );
				}
			}
			else
			{
				ret_status = -2;	// Bad file format.
			}
		}
		else
		{
			ret_status = -1;	// Can't open file for reading.
		}

		UnlockFileEx( hFile_read, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_read );	
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for reading.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading SFTP fingerprints: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_sftp_fps_host_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\sftp_fingerprint_settings\0", 27 );
	//g_base_directory[ g_base_directory_length + 26 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving SFTP fingerprints: %S", g_base_directory );
#endif

	HANDLE hFile = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_SFTP_HOSTS, sizeof( char ) * 4 );	// Magic identifier for the site info.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( g_sftp_fps_host_info );
		while ( node != NULL )
		{
			DoublyLinkedList *dll_node = ( DoublyLinkedList * )node->val;
			while ( dll_node != NULL )
			{
				SFTP_FPS_HOST_INFO *sfhi = ( SFTP_FPS_HOST_INFO * )dll_node->data;
				if ( sfhi != NULL && !sfhi->temporary )
				{
					// lstrlen is safe for NULL values.
					int host_length = ( lstrlenW( sfhi->w_host ) + 1 ) * sizeof( wchar_t );
					int key_algorithm_length = ( lstrlenW( sfhi->w_key_algorithm ) + 1 ) * sizeof( wchar_t );
					int key_fingerprint_length = ( lstrlenW( sfhi->w_key_fingerprint ) + 1 ) * sizeof( wchar_t );

					// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
					if ( ( signed )( pos + host_length + key_algorithm_length + key_fingerprint_length ) > size )
					{
						// Dump the buffer.
						WriteFile( hFile, buf, pos, &write, NULL );
						pos = 0;
#ifdef ENABLE_LOGGING
						lfz += write;
#endif
					}

					_memcpy_s( buf + pos, size - pos, sfhi->w_host, host_length );
					pos += host_length;

					_memcpy_s( buf + pos, size - pos, sfhi->w_key_algorithm, key_algorithm_length );
					pos += key_algorithm_length;

					_memcpy_s( buf + pos, size - pos, sfhi->w_key_fingerprint, key_fingerprint_length );
					pos += key_fingerprint_length;
				}

				dll_node = dll_node->next;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
#ifdef ENABLE_LOGGING
			lfz += write;
#endif
		}

		GlobalFree( buf );

		SetEndOfFile( hFile );

		UnlockFileEx( hFile, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving SFTP fingerprints: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

///////////////////////

THREAD_RETURN load_sftp_keys_host_list( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM | LVIF_TEXT;

	node_type *node = dllrbt_get_head( g_sftp_keys_host_info );
	while ( node != NULL )
	{
		SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )node->val;

		if ( skhi != NULL )
		{
			lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEMCOUNT, 0, 0 );
			lvi.lParam = ( LPARAM )skhi;
			lvi.pszText = skhi->w_host;
			_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
		}

		node = node->next;
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

void FreeSFTPKeysHostInfo( SFTP_KEYS_HOST_INFO **sftp_keys_host_info )
{
	if ( *sftp_keys_host_info != NULL )
	{
		if ( ( *sftp_keys_host_info )->w_username != NULL ) { GlobalFree( ( *sftp_keys_host_info )->w_username ); }
		if ( ( *sftp_keys_host_info )->username != NULL ) { GlobalFree( ( *sftp_keys_host_info )->username ); }
		if ( ( *sftp_keys_host_info )->w_host != NULL ) { GlobalFree( ( *sftp_keys_host_info )->w_host ); }
		if ( ( *sftp_keys_host_info )->host != NULL ) { GlobalFree( ( *sftp_keys_host_info )->host ); }
		if ( ( *sftp_keys_host_info )->w_key_file_path != NULL ) { GlobalFree( ( *sftp_keys_host_info )->w_key_file_path ); }
		if ( ( *sftp_keys_host_info )->key_file_path != NULL ) { GlobalFree( ( *sftp_keys_host_info )->key_file_path ); }

		GlobalFree( *sftp_keys_host_info );

		*sftp_keys_host_info = NULL;
	}
}

int dllrbt_compare_sftp_keys_host_info( void *a, void *b )
{
	SFTP_KEYS_HOST_INFO *a1 = ( SFTP_KEYS_HOST_INFO * )a;
	SFTP_KEYS_HOST_INFO *b1 = ( SFTP_KEYS_HOST_INFO * )b;

	if ( a1 == b1 )
	{
		return 0;
	}

	int ret = lstrcmpA( a1->host, b1->host );

	if ( ret == 0 )
	{
		ret = lstrcmpA( a1->username, b1->username );

		if ( ret == 0 )
		{
			if ( a1->port > b1->port )
			{
				return 1;
			}
			else if ( a1->port < b1->port )
			{
				return -1;
			}
		}
	}

	return ret;
}

THREAD_RETURN handle_sftp_keys_host_list( void *pArguments )
{
	SFTP_KEYS_HOST_UPDATE_INFO *skhui = ( SFTP_KEYS_HOST_UPDATE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( skhui != NULL )
	{
		if ( skhui->update_type == 0 && skhui->skhi != NULL )	// Add
		{
			SFTP_KEYS_HOST_INFO *skhi = skhui->skhi;

#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Adding %S to SFTP host key list", skhi->w_host );
#endif

			unsigned char fail_type = 0;

			ParseSFTPHost( skhi->w_host, &skhi->host, skhi->port );

			int username_length = WideCharToMultiByte( CP_UTF8, 0, skhi->w_username, -1, NULL, 0, NULL, NULL );
			skhi->username = ( char * )GlobalAlloc( GPTR, sizeof( char ) * username_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, skhi->w_username, -1, skhi->username, username_length, NULL, NULL );

			int key_file_path_length = WideCharToMultiByte( CP_UTF8, 0, skhi->w_key_file_path, -1, NULL, 0, NULL, NULL );
			skhi->key_file_path = ( char * )GlobalAlloc( GPTR, sizeof( char ) * key_file_path_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, skhi->w_key_file_path, -1, skhi->key_file_path, key_file_path_length, NULL, NULL );

			if ( skhui->old_skhi != NULL )
			{
				// Find the host info
				dllrbt_iterator *new_itr = dllrbt_find( g_sftp_keys_host_info, ( void * )skhi, false );

				// Find the host info
				dllrbt_iterator *old_itr = dllrbt_find( g_sftp_keys_host_info, ( void * )skhui->old_skhi, false );

				if ( new_itr == NULL || old_itr == new_itr )
				{
					dllrbt_remove( g_sftp_keys_host_info, old_itr );
				}
			}

			if ( dllrbt_insert( g_sftp_keys_host_info, ( void * )skhi, ( void * )skhi ) != DLLRBT_STATUS_OK )
			{
				fail_type = 1;	// Already exits.
			}
			else
			{
				LVITEM lvi;
				_memzero( &lvi, sizeof( LVITEM ) );
				lvi.mask = LVIF_PARAM | LVIF_TEXT;

				if ( skhui->old_skhi != NULL )
				{
					lvi.iItem = skhui->index;
					lvi.lParam = ( LPARAM )skhi;
					lvi.pszText = skhi->w_host;
					_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_SETITEM, 0, ( LPARAM )&lvi );

					FreeSFTPKeysHostInfo( &skhui->old_skhi );
				}
				else
				{
					lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )skhi;
					lvi.pszText = skhi->w_host;
					_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					_SendMessageW( g_hWnd_sftp_keys_tab, WM_PROPAGATE, 3, ( LPARAM )skhui );	// Update selected host.
				}

				sftp_keys_host_list_changed = true;
			}

			if ( fail_type != 0 )
			{
				FreeSFTPKeysHostInfo( &skhi );

				_SendNotifyMessageW( g_hWnd_sftp_keys_tab, WM_PROPAGATE, fail_type, 0 );
			}
		}
		else if ( skhui->update_type == 1 )	// Remove
		{
			// Prevent the listviews from drawing while freeing lParam values.
			skip_sftp_keys_host_list_draw = true;

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			int item_count = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			int *index_array = NULL;

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_ENSUREVISIBLE, 0, FALSE );

				index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );
				if ( index_array != NULL )
				{
					lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

					_EnableWindow( g_hWnd_sftp_keys_host_list, FALSE );	// Prevent any interaction with the listview while we're processing.

					// Create an index list of selected items (in reverse order).
					for ( int i = 0; i < sel_count; ++i )
					{
						lvi.iItem = index_array[ sel_count - 1 - i ] = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
					}

					_EnableWindow( g_hWnd_sftp_keys_host_list, TRUE );	// Allow the listview to be interactive.
				}

				item_count = sel_count;
			}

			if ( handle_all || index_array != NULL )
			{
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

					_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )lvi.lParam;

					if ( !handle_all )
					{
						_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_DELETEITEM, index_array[ i ], 0 );
					}
					else if ( i >= ( item_count - 1 ) )
					{
						_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_DELETEALLITEMS, 0, 0 );
					}

					if ( skhi != NULL )
					{
#ifdef ENABLE_LOGGING
						WriteLog( LOG_INFO_ACTION, "Removing %S from SFTP host key list", skhi->w_host );
#endif
						// Find the host info
						dllrbt_iterator *itr = dllrbt_find( g_sftp_keys_host_info, ( void * )skhi, false );
						if ( itr != NULL )
						{
							dllrbt_remove( g_sftp_keys_host_info, itr );
						}

						FreeSFTPKeysHostInfo( &skhi );
					}
				}
			}

			_SendMessageW( g_hWnd_sftp_keys_tab, WM_PROPAGATE, 2, 0 );	// Disable remove button.

			sftp_keys_host_list_changed = true;

			skip_sftp_keys_host_list_draw = false;

			if ( index_array != NULL )
			{
				GlobalFree( index_array );
			}
		}
		else if ( skhui->update_type == 2 )	// Enable/Disable
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

			int item_count = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			bool copy_all = false;
			if ( item_count == sel_count )
			{
				copy_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			// Go through each item, and copy the URL.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				if ( copy_all )
				{
					lvi.iItem = i;
				}
				else
				{
					lvi.iItem = ( int )_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_sftp_keys_host_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )lvi.lParam;

				if ( skhi != NULL )
				{
#ifdef ENABLE_LOGGING
					WriteLog( LOG_INFO_ACTION, "%s %S in SFTP host key list", ( skhui->enable ? "Enabling" : "Disabling" ), skhi->w_host );
#endif
					skhi->enable = skhui->enable;
				}
			}

			sftp_keys_host_list_changed = true;
		}

		GlobalFree( skhui );
	}

	_InvalidateRect( g_hWnd_sftp_keys_host_list, NULL, FALSE );

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

char read_sftp_keys_host_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\sftp_private_key_settings\0", 27 );
	//g_base_directory[ g_base_directory_length + 26 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading SFTP host keys: %S", g_base_directory );
#endif

	HANDLE hFile_read = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_read = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_read, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool enable;
		wchar_t *username;
		wchar_t *host;
		wchar_t *key_file_path;

		//

		unsigned char magic_identifier[ 4 ];
		BOOL bRet = ReadFile( hFile_read, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
		if ( bRet != FALSE )
		{
#ifdef ENABLE_LOGGING
			lfz += 4;
#endif
			unsigned char version = magic_identifier[ 3 ] - 0x40;

			if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_SFTP_KEYS, 3 ) == 0 && version <= 0x0F )
			{
				char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 3 ) );	// 512 KB buffer.
				if ( buf != NULL )
				{
					DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

					while ( total_read < fz )
					{
						bRet = ReadFile( hFile_read, buf, sizeof( char ) * 524288, &read, NULL );
						if ( bRet == FALSE )
						{
							break;
						}

	#ifdef ENABLE_LOGGING
						lfz += read;
	#endif

						buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

						// This terminates wide character strings so we don't read past the buffer.
						buf[ read + 1 ] = 0;
						buf[ read + 2 ] = 0;

						total_read += read;

						// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
						// If it's larger than our buffer, then the file is probably invalid/corrupt.
						if ( total_read == last_total )
						{
							break;
						}

						last_total = total_read;

						p = buf;
						offset = last_entry = 0;

						while ( offset < read )
						{
							username = NULL;
							host = NULL;
							key_file_path = NULL;

							//
							// Enable/Disable entry
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &enable, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							// Username
							int string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( username, string_length, p, string_length );
							*( username + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// Host
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( host, string_length, p, string_length );
							*( host + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// Key Fingerprint
							string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset > read ) { goto CLEANUP; }

							key_file_path = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( key_file_path, string_length, p, string_length );
							*( key_file_path + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							//

							last_entry = offset;	// This value is the ending offset of the last valid entry.

							SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )GlobalAlloc( GPTR, sizeof( SFTP_KEYS_HOST_INFO ) );
							if ( skhi != NULL )
							{
								skhi->enable = enable;
								skhi->w_username = username;
								skhi->w_host = host;
								skhi->w_key_file_path = key_file_path;

								ParseSFTPHost( skhi->w_host, &skhi->host, skhi->port );

								int mb_length;
								if ( skhi->w_username != NULL )
								{
									mb_length = WideCharToMultiByte( CP_UTF8, 0, skhi->w_username, -1, NULL, 0, NULL, NULL );
									skhi->username = ( char * )GlobalAlloc( GPTR, sizeof( char ) * mb_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, skhi->w_username, -1, skhi->username, mb_length, NULL, NULL );
								}

								if ( skhi->w_key_file_path != NULL )
								{
									mb_length = WideCharToMultiByte( CP_UTF8, 0, skhi->w_key_file_path, -1, NULL, 0, NULL, NULL );
									skhi->key_file_path = ( char * )GlobalAlloc( GPTR, sizeof( char ) * mb_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, skhi->w_key_file_path, -1, skhi->key_file_path, mb_length, NULL, NULL );
								}

								//

								if ( dllrbt_insert( g_sftp_keys_host_info, ( void * )skhi, ( void * )skhi ) != DLLRBT_STATUS_OK )
								{
									FreeSFTPKeysHostInfo( &skhi );
								}
							}
							else
							{
								GlobalFree( username );
								GlobalFree( host );
								GlobalFree( key_file_path );
							}

							continue;

			CLEANUP:
							GlobalFree( username );
							GlobalFree( host );
							GlobalFree( key_file_path );

							// Go back to the last valid entry.
							if ( total_read < fz )
							{
								total_read -= ( read - last_entry );
								SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
							}

							break;
						}
					}

					GlobalFree( buf );
				}
			}
			else
			{
				ret_status = -2;	// Bad file format.
			}
		}
		else
		{
			ret_status = -1;	// Can't open file for reading.
		}

		UnlockFileEx( hFile_read, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_read );	
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for reading.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading SFTP host keys: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_sftp_keys_host_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\sftp_private_key_settings\0", 27 );
	//g_base_directory[ g_base_directory_length + 26 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving SFTP host keys: %S", g_base_directory );
#endif

	HANDLE hFile = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_SFTP_KEYS, sizeof( char ) * 4 );	// Magic identifier for the site info.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( g_sftp_keys_host_info );
		while ( node != NULL )
		{
			SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )node->val;
			if ( skhi != NULL )
			{
				// lstrlen is safe for NULL values.
				int username_length = ( lstrlenW( skhi->w_username ) + 1 ) * sizeof( wchar_t );
				int host_length = ( lstrlenW( skhi->w_host ) + 1 ) * sizeof( wchar_t );
				int key_file_path_length = ( lstrlenW( skhi->w_key_file_path ) + 1 ) * sizeof( wchar_t );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos + username_length + host_length + key_file_path_length + sizeof( bool ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
#ifdef ENABLE_LOGGING
					lfz += write;
#endif
				}

				_memcpy_s( buf + pos, size - pos, &skhi->enable, sizeof( bool ) );
				pos += sizeof( bool );

				_memcpy_s( buf + pos, size - pos, skhi->w_username, username_length );
				pos += username_length;

				_memcpy_s( buf + pos, size - pos, skhi->w_host, host_length );
				pos += host_length;

				_memcpy_s( buf + pos, size - pos, skhi->w_key_file_path, key_file_path_length );
				pos += key_file_path_length;
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
#ifdef ENABLE_LOGGING
			lfz += write;
#endif
		}

		GlobalFree( buf );

		SetEndOfFile( hFile );

		UnlockFileEx( hFile, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving SFTP host keys: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}
