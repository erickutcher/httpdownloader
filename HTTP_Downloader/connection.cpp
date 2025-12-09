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
#include "connection.h"

//#include "lite_kernel32.h"
#include "lite_comdlg32.h"
#include "lite_ole32.h"
#include "lite_shell32.h"
#include "lite_zlib1.h"
#include "lite_normaliz.h"

#include "http_parsing.h"
#include "ftp_parsing.h"
#include "sftp.h"

#include "categories.h"

#include "utilities.h"
#include "site_manager_utilities.h"
#include "list_operations.h"

#include "string_tables.h"
#include "cmessagebox.h"

#include "menus.h"

#include "treelistview.h"
#include "doublylinkedlist.h"

#include "dark_mode.h"

#pragma warning( push )
#pragma warning( disable : 4201 )	// nonstandard extension used: nameless struct/union
#include <winioctl.h>
#pragma warning( pop )

HANDLE g_hIOCP = NULL;

WSAEVENT g_cleanup_event[ 1 ];

bool g_end_program = false;

DoublyLinkedList *g_context_list = NULL;

PCCERT_CONTEXT g_pCertContext = NULL;

SOCKET g_listen_socket = INVALID_SOCKET;
SOCKET_CONTEXT *g_listen_context = NULL;

// Server

char *g_server_domain = NULL;
unsigned short g_server_port = 80;
bool g_server_use_ipv6 = false;

char *g_authentication_username = NULL;
char *g_authentication_password = NULL;
unsigned int g_authentication_username_length = 0;
unsigned int g_authentication_password_length = 0;

char *g_encoded_authentication = NULL;
DWORD g_encoded_authentication_length = 0;

wchar_t *g_server_punycode_hostname = NULL;

extern char *g_nonce = NULL;
unsigned long g_nonce_length = 0;
extern char *g_opaque = NULL;
unsigned long g_opaque_length = 0;

// HTTP Proxy

wchar_t *g_punycode_hostname = NULL;

char *g_proxy_auth_username = NULL;
char *g_proxy_auth_password = NULL;
char *g_proxy_auth_key = NULL;
unsigned long g_proxy_auth_key_length = 0;

// HTTPS Proxy

wchar_t *g_punycode_hostname_s = NULL;

char *g_proxy_auth_username_s = NULL;
char *g_proxy_auth_password_s = NULL;
char *g_proxy_auth_key_s = NULL;
unsigned long g_proxy_auth_key_length_s = 0;

// SOCKS5 Proxy

wchar_t *g_punycode_hostname_socks = NULL;

char *g_proxy_auth_username_socks = NULL;
char *g_proxy_auth_password_socks = NULL;

char *g_proxy_auth_ident_username_socks = NULL;

////////////////////

unsigned long g_total_downloading = 0;
DoublyLinkedList *download_queue = NULL;

DoublyLinkedList *active_download_list = NULL;		// List of active DOWNLOAD_INFO objects.

DoublyLinkedList *file_size_prompt_list = NULL;		// List of downloads that need to be prompted to continue.
DoublyLinkedList *rename_file_prompt_list = NULL;	// List of downloads that need to be prompted to continue.
DoublyLinkedList *last_modified_prompt_list = NULL;	// List of downloads that need to be prompted to continue.
DoublyLinkedList *fingerprint_prompt_list = NULL;	// List of downloads that need to be prompted to continue.

DoublyLinkedList *move_file_queue = NULL;			// List of downloads that need to be moved to a new folder.

HANDLE g_timeout_semaphore = NULL;
HANDLE g_update_semaphore = NULL;

CRITICAL_SECTION context_list_cs;				// Guard access to the global context list.
CRITICAL_SECTION active_download_list_cs;		// Guard access to the global active download list.
CRITICAL_SECTION download_queue_cs;				// Guard access to the download queue.
CRITICAL_SECTION file_size_prompt_list_cs;		// Guard access to the file size prompt list.
CRITICAL_SECTION rename_file_prompt_list_cs;	// Guard access to the rename file prompt list.
CRITICAL_SECTION last_modified_prompt_list_cs;	// Guard access to the last modified prompt list.
CRITICAL_SECTION fingerprint_prompt_list_cs;	// Guard access to the file fingerprint prompt list.
CRITICAL_SECTION move_file_queue_cs;			// Guard access to the move file queue.
CRITICAL_SECTION cleanup_cs;
CRITICAL_SECTION update_check_timeout_cs;
CRITICAL_SECTION file_allocation_cs;

LPFN_ACCEPTEX _AcceptEx = NULL;
LPFN_CONNECTEX _ConnectEx = NULL;

bool file_size_prompt_active = false;
int g_file_size_cmb_ret = 0;	// Message box prompt for large files sizes.

bool rename_file_prompt_active = false;
int g_rename_file_cmb_ret = 0;	// Message box prompt to rename files.
int g_rename_file_cmb_ret2 = 0;	// Message box prompt to rename files.

bool last_modified_prompt_active = false;
int g_last_modified_cmb_ret = 0;	// Message box prompt for modified files.

bool fingerprint_prompt_active = false;
int g_fingerprint_cmb_ret = 0;	// Message box prompt for key fingerprints.

int g_file_not_exist_cmb_ret = 0;	// Message box prompt for non existent file.

bool move_file_process_active = false;

unsigned int g_session_status_count[ NUM_SESSION_STATUS ] = { 0 };	// 9 states that can be considered finished (Completed, Stopped, Failed, etc.)

volatile LONG g_status_count[ 16 ] = { 0 };

bool g_timers_running = false;

bool g_waiting_for_update = false;
unsigned long g_new_version = 0;
char *g_new_version_url = NULL;
char g_update_check_state;	// 0 manual update check, 1 automatic update check

#ifdef IS_BETA
unsigned long g_new_beta = 0;
#endif

void SetSessionStatusCount( unsigned int status )
{
	switch ( status )
	{
		case STATUS_COMPLETED:					{ ++g_session_status_count[ 0 ]; } break;
		case STATUS_STOPPED:					{ ++g_session_status_count[ 1 ]; } break;
		case STATUS_TIMED_OUT:					{ ++g_session_status_count[ 2 ]; } break;
		case STATUS_FAILED:						{ ++g_session_status_count[ 3 ]; } break;
		case STATUS_FILE_IO_ERROR:				{ ++g_session_status_count[ 4 ]; } break;
		case STATUS_SKIPPED:					{ ++g_session_status_count[ 5 ]; } break;
		case STATUS_AUTH_REQUIRED:				{ ++g_session_status_count[ 6 ]; } break;
		case STATUS_PROXY_AUTH_REQUIRED:		{ ++g_session_status_count[ 7 ]; } break;
		case STATUS_INSUFFICIENT_DISK_SPACE:	{ ++g_session_status_count[ 8 ]; } break;
	}
}

int GetStatusIndex( unsigned int status )
{
	int index = 0;

	if		( status == STATUS_CONNECTING )					{ index = 1; }
	else if ( IS_STATUS( status, STATUS_RESTART ) )			{ index = 9; }
	else if ( IS_STATUS( status, STATUS_PAUSED ) )			{ index = 3; }
	else if ( IS_STATUS( status, STATUS_QUEUED ) )			{ index = 4; }
	else if ( status == STATUS_DOWNLOADING )				{ index = 2; }
	else if ( status == STATUS_COMPLETED )					{ index = 5; }
	else if ( status == STATUS_STOPPED )					{ index = 6; }
	else if ( status == STATUS_TIMED_OUT )					{ index = 7; }
	else if ( status == STATUS_FAILED )						{ index = 8; }
	else if ( status == STATUS_FILE_IO_ERROR )				{ index = 10; }
	else if ( status == STATUS_SKIPPED )					{ index = 11; }
	else if ( status == STATUS_AUTH_REQUIRED )				{ index = 12; }
	else if ( status == STATUS_PROXY_AUTH_REQUIRED )		{ index = 13; }
	else if	( status == STATUS_ALLOCATING_FILE )			{ index = 14; }
	else if	( status == STATUS_MOVING_FILE )				{ index = 15; }
	else if ( status == STATUS_INSUFFICIENT_DISK_SPACE )	{ index = 16; }
	else if ( status == STATUS_REMOVE )						{ index = -1; }

	return index;
}
LONG DecrementStatusCount( unsigned int status )
{
	int index = GetStatusIndex( status );
	if ( index >= 0 && g_status_count[ index ] > 0 )
	{
		return InterlockedDecrement( &g_status_count[ index ] );
	}
	else
	{
		return 0;
	}
}

LONG IncrementStatusCount( unsigned int status )
{
	int index = GetStatusIndex( status );
	if ( index >= 0 )
	{
		return InterlockedIncrement( &g_status_count[ index ] );
	}
	else
	{
		return 0;
	}
}

void SetStatus( DOWNLOAD_INFO *di, unsigned int status )
{
	if ( di != NULL )
	{
		if ( di->hosts == 0 )
		{
			di->status = status;

			return;
		}

		bool update_tln = false;

		int item_index = 0;
		int item_child_count = 0;
		bool status_changed = ( di->status != status ? true : false );
		unsigned char item_state = 0;	// 0 = No change, 1 = Added, 2 = Removed

		TREELISTNODE *tln = ( TREELISTNODE * )di->tln;

		// See if the status is leaving the filtered view.
		if ( g_status_filter != STATUS_NONE && g_status_filter != CATEGORY_STATUS && IsFilterSet( di, g_status_filter ) )
		{
			if ( tln != NULL )
			{
				if ( tln->flag & TLVS_SELECTED )
				{
					tln->flag &= ~TLVS_SELECTED;
					TLV_AddSelectedCount( -1 );

					if ( tln == g_first_selection_node )
					{
						TREELISTNODE *first_selection_node;
						int first_selection_index = TLV_GetNextSelectedItem( tln, TLV_GetFirstSelectedIndex(), &first_selection_node );
						// Offset it since we're removing the first selected item.
						if ( first_selection_index > 0 )
						{
							--first_selection_index;
						}
						g_first_selection_node = first_selection_node;
						g_first_selection_index = g_first_selection_index;
					}

					// Cancel Selection.
					InterlockedExchange( &g_refresh_list, ( g_refresh_list | 0x08 ) );
				}

				if ( tln->flag & TLVS_FOCUSED )
				{
					tln->flag &= ~TLVS_FOCUSED;
					TLV_SetFocusedItem( NULL );
					TLV_SetFocusedIndex( -1 );

					// Cancel Rename and Drag.
					InterlockedExchange( &g_refresh_list, ( g_refresh_list | 0x02 | 0x04 ) );
				}
			}

			if ( status_changed )
			{
				if ( tln != NULL )
				{
					item_index = TLV_GetItemIndex( tln );
					if ( tln->is_expanded )
					{
						item_child_count = tln->child_count;
					}
				}

				item_state = 2;
			}

			int child_count = 0;

			if ( tln != NULL )
			{
				EnterCriticalSection( &di->di_cs );

				if ( !( tln->flag & TLVS_EXPANDING_COLLAPSING ) && tln->is_expanded )
				{
					update_tln = true;

					tln->flag |= TLVS_EXPANDING_COLLAPSING;

					child_count = tln->child_count;
				}

				LeaveCriticalSection( &di->di_cs );
			}

			TLV_AddExpandedItemCount( -( child_count + 1 ) );
		}

		DecrementStatusCount( di->status );

		di->status = status;

		// See if the status is entering the filtered view.
		if ( g_status_filter != STATUS_NONE && g_status_filter != CATEGORY_STATUS && IsFilterSet( di, g_status_filter ) )
		{
			if ( status_changed )
			{
				if ( tln != NULL )
				{
					item_index = TLV_GetItemIndex( tln );
					if ( tln->is_expanded )
					{
						item_child_count = tln->child_count;
					}
				}

				item_state = 1;
			}

			int child_count = 0;

			if ( tln != NULL && tln->is_expanded )
			{
				child_count = tln->child_count;
			}

			TLV_AddExpandedItemCount( child_count + 1 );
		}

		IncrementStatusCount( di->status );

		if ( tln != NULL && update_tln )
		{
			EnterCriticalSection( &di->di_cs );

			tln->flag &= ~TLVS_EXPANDING_COLLAPSING;

			LeaveCriticalSection( &di->di_cs );
		}

		// Handle filtered changes.
		if ( item_state != 0 )
		{
			if ( item_state == 1 )	// Added
			{
				int first_visible_index = TLV_GetFirstVisibleIndex();
				if ( item_index < first_visible_index )
				{
					TLV_SetFirstVisibleIndex( first_visible_index + ( 1 + item_child_count ) );
					TLV_SetFirstVisibleRootIndex( TLV_GetFirstVisibleRootIndex() + 1 );
				}

				TLV_AddTotalItemCount( 1 );
			}
			else if ( item_state == 2 ) // Removed
			{
				int first_visible_index = TLV_GetFirstVisibleIndex();
				if ( item_index < first_visible_index )
				{
					TLV_SetFirstVisibleIndex( first_visible_index - ( 1 + item_child_count ) );
					TLV_SetFirstVisibleRootIndex( TLV_GetFirstVisibleRootIndex() - 1 );
				}

				TLV_AddTotalItemCount( -1 );
			}

			int index = GetStatusIndex( g_status_filter );
			if ( index >= 0 )
			{
				TLV_SetRootItemCount( g_status_count[ index ] );
			}

			// The update timer will refresh the list every second if there's a change in the item count.
			InterlockedExchange( &g_refresh_list, ( g_refresh_list | 0x01 ) );
		}
	}
}

void SetSharedInfoStatus( DOWNLOAD_INFO *shared_info )
{
	if ( shared_info != NULL )
	{
		unsigned int shared_status = 0;
		unsigned int status = shared_info->status;

		DoublyLinkedList *host_node = shared_info->host_list;
		while ( host_node != NULL )
		{
			DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
			if ( host_di != NULL )
			{
				// Might happen if a group download doesn't connect (wrong SSL/TLS).
				// The non-driver hosts will not have had a status set.
				if ( host_di->status == STATUS_NONE )
				{
					SetStatus( host_di, STATUS_STOPPED );
				}

				if ( host_di->processed_header || host_di->status != STATUS_SKIPPED )
				{
					shared_status |= host_di->status;
				}
			}

			host_node = host_node->next;
		}

		if ( IS_STATUS( shared_status, STATUS_FILE_IO_ERROR ) )					{ status = STATUS_FILE_IO_ERROR; }
		else if ( IS_STATUS( shared_status, STATUS_ALLOCATING_FILE ) )			{ status = STATUS_ALLOCATING_FILE; }
		else if ( IS_STATUS( shared_status, STATUS_FAILED ) )					{ status = STATUS_FAILED; }
		else if ( IS_STATUS( shared_status, STATUS_TIMED_OUT ) )				{ status = STATUS_TIMED_OUT; }
		else if ( IS_STATUS( shared_status, STATUS_AUTH_REQUIRED ) )			{ status = STATUS_AUTH_REQUIRED; }
		else if ( IS_STATUS( shared_status, STATUS_PROXY_AUTH_REQUIRED ) )		{ status = STATUS_PROXY_AUTH_REQUIRED; }
		else if ( IS_STATUS( shared_status, STATUS_QUEUED ) )					{ status = STATUS_QUEUED; }
		else if ( IS_STATUS( shared_status, STATUS_MOVING_FILE ) )				{ status = STATUS_MOVING_FILE; }
		else if ( IS_STATUS( shared_status, STATUS_INSUFFICIENT_DISK_SPACE ) )	{ status = STATUS_INSUFFICIENT_DISK_SPACE; }
		else if ( IS_STATUS( shared_status, STATUS_PAUSED ) )					{ status = STATUS_PAUSED; }
		else if ( IS_STATUS( shared_status, STATUS_SKIPPED ) && shared_info->file_size > 0 && shared_info->downloaded != shared_info->file_size )
		{
			status = STATUS_SKIPPED;
		}
		else if ( IS_STATUS( shared_status, STATUS_RESTART ) )					{ status = STATUS_RESTART; }
		else if ( IS_STATUS( shared_status, STATUS_DOWNLOADING ) )				{ status = STATUS_DOWNLOADING; }
		else if ( IS_STATUS( shared_status, STATUS_CONNECTING ) )				{ status = STATUS_CONNECTING; }
		else if ( IS_STATUS( shared_status, STATUS_STOPPED ) )					{ status = STATUS_STOPPED; }
		else if ( shared_status == STATUS_COMPLETED || ( IS_STATUS( shared_status, STATUS_COMPLETED ) &&
				( ( shared_info->file_size > 0 && shared_info->downloaded == shared_info->file_size ) ||
				  ( shared_info->file_size == 0 && shared_info->downloaded > 0 ) ) ) )
		{
			status = STATUS_COMPLETED;
		}

		SetStatus( shared_info, status );
	}
}

// This should be done in a critical section.
void EnableTimers( bool timer_state )
{
	// Trigger the timers out of their infinite wait.
	if ( timer_state )
	{
		if ( !g_timers_running )
		{
			g_timers_running = true;

			if ( g_timeout_semaphore != NULL )
			{
				ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
			}

			if ( g_timer_semaphore != NULL )
			{
				ReleaseSemaphore( g_timer_semaphore, 1, NULL );
			}
		}
	}
	else	// Let the timers complete their current task and then wait indefinitely.
	{
		UpdateMenus( true );

		g_timers_running = false;
	}
}

DWORD WINAPI Timeout( LPVOID /*WorkThreadContext*/ )
{
	bool run_timer = g_timers_running;

	while ( !g_end_program )
	{
		// Check the timeout counter every second, or wait indefinitely if we're using the system default.
		WaitForSingleObject( g_timeout_semaphore, ( run_timer ? ( cfg_timeout > 0 ? 1000 : INFINITE ) : INFINITE ) );

		if ( g_end_program )
		{
			break;
		}

		// This will allow the timer to go through at least one loop after it's been disabled (g_timers_running == false).
		run_timer = g_timers_running;

		if ( TryEnterCriticalSection( &context_list_cs ) == TRUE )
		{
			DoublyLinkedList *context_node = g_context_list;

			// Go through the list of active connections.
			while ( context_node != NULL && context_node->data != NULL )
			{
				if ( g_end_program )
				{
					break;
				}

				SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )context_node->data;

				if ( TryEnterCriticalSection( &context->context_cs ) == TRUE )
				{
					if ( context->cleanup == 0 && IS_STATUS_NOT( context->status, STATUS_ALLOCATING_FILE | STATUS_INPUT_REQUIRED ) )
					{
						// Don't increment the Control connection's timeout value.
						// It'll be forced to time out if the Data connection times out.
						if ( context->ftp_context != NULL && context->ftp_connection_type & FTP_CONNECTION_TYPE_CONTROL )
						{
							if ( cfg_ftp_send_keep_alive && context->ftp_connection_type == FTP_CONNECTION_TYPE_CONTROL )
							{
								InterlockedIncrement( &context->keep_alive_timeout );	// Increment the timeout counter.

								if ( context->keep_alive_timeout >= 30 )
								{
									InterlockedExchange( &context->keep_alive_timeout, 0 );	// Reset timeout counter.

									SendFTPKeepAlive( context );
								}
							}
						}
						else
						{
							InterlockedIncrement( &context->timeout );	// Increment the timeout counter.

							// See if we've reached the timeout limit.
							if ( ( context->timeout >= cfg_timeout ) && ( cfg_timeout > 0 ) )
							{
								// Ignore paused and queued downloads.
								if ( IS_STATUS( context->status, STATUS_PAUSED | STATUS_QUEUED ) )
								{
									InterlockedExchange( &context->timeout, 0 );	// Reset timeout counter.
								}
								else
								{
									context->timed_out = TIME_OUT_TRUE;

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
							}
						}
					}

					LeaveCriticalSection( &context->context_cs );
				}

				context_node = context_node->next;
			}

			LeaveCriticalSection( &context_list_cs );
		}
	}

	CloseHandle( g_timeout_semaphore );
	g_timeout_semaphore = NULL;

	_ExitThread( 0 );
	//return 0;
}

void InitializeServerInfo()
{
	if ( cfg_server_enable_ssl )
	{
		if ( g_use_openssl )
		{
			InitializeServerSSL_CTX( cfg_server_ssl_version, cfg_certificate_type );
		}
		else
		{
			if ( cfg_certificate_type == 1 )	// Public/Private Key Pair.
			{
				g_pCertContext = LoadPublicPrivateKeyPair( cfg_certificate_cer_file_name, cfg_certificate_key_file_name );
			}
			else	// PKCS #12 File.
			{
				g_pCertContext = LoadPKCS12( cfg_certificate_pkcs_file_name, cfg_certificate_pkcs_password );
			}
		}
	}

	if ( cfg_use_authentication )
	{
		if ( cfg_authentication_username != NULL )
		{
			g_authentication_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, NULL, 0, NULL, NULL );
			g_authentication_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * g_authentication_username_length ); // Size includes the null character.
			g_authentication_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, g_authentication_username, g_authentication_username_length, NULL, NULL ) - 1;
		}

		if ( cfg_authentication_password != NULL )
		{
			g_authentication_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, NULL, 0, NULL, NULL );
			g_authentication_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * g_authentication_password_length ); // Size includes the null character.
			g_authentication_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, g_authentication_password, g_authentication_password_length, NULL, NULL ) - 1;
		}

		if ( cfg_authentication_type == AUTH_TYPE_DIGEST )
		{
			CreateDigestAuthorizationInfo( &g_nonce, g_nonce_length, &g_opaque, g_opaque_length );
		}
		else
		{
			CreateBasicAuthorizationKey( g_authentication_username, g_authentication_username_length, g_authentication_password, g_authentication_password_length, &g_encoded_authentication, &g_encoded_authentication_length );
		}
	}

	if ( normaliz_state == NORMALIZ_STATE_RUNNING )
	{
		if ( cfg_server_address_type == 0 )	// Hostname.
		{
			int hostname_length = lstrlenW( cfg_server_hostname ) + 1;	// Include the NULL terminator.
			int punycode_length = _IdnToAscii( 0, cfg_server_hostname, hostname_length, NULL, 0 );

			if ( punycode_length > hostname_length )
			{
				g_server_punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
				_IdnToAscii( 0, cfg_server_hostname, hostname_length, g_server_punycode_hostname, punycode_length );
			}
		}
	}
}

void CleanupServerInfo()
{
	if ( g_authentication_username != NULL )
	{
		GlobalFree( g_authentication_username );
		g_authentication_username = NULL;
	}

	g_authentication_username_length = 0;

	if ( g_authentication_password != NULL )
	{
		GlobalFree( g_authentication_password );
		g_authentication_password = NULL;
	}

	g_authentication_password_length = 0;

	if ( g_encoded_authentication != NULL )
	{
		GlobalFree( g_encoded_authentication );
		g_encoded_authentication = NULL;
	}

	if ( g_nonce != NULL )
	{
		GlobalFree( g_nonce );
		g_nonce = NULL;
	}

	g_nonce_length = 0;

	if ( g_opaque != NULL )
	{
		GlobalFree( g_opaque );
		g_opaque = NULL;
	}

	g_opaque_length = 0;

	if ( g_server_punycode_hostname != NULL )
	{
		GlobalFree( g_server_punycode_hostname );
		g_server_punycode_hostname = NULL;
	}

	if ( g_server_domain != NULL )
	{
		GlobalFree( g_server_domain );
		g_server_domain = NULL;
	}

	if ( cfg_server_enable_ssl )
	{
		if ( g_use_openssl )
		{
			if ( g_server_ssl_ctx != NULL )
			{
				_SSL_CTX_free( g_server_ssl_ctx );
				g_server_ssl_ctx = NULL;
			}
		}
		else
		{
			if ( g_pCertContext != NULL )
			{
				_CertFreeCertificateContext( g_pCertContext );
				g_pCertContext = NULL;
			}
		}
	}
}

void StartServer()
{
#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Starting web server" );
#endif

	g_listen_socket = CreateListenSocket();

	// Create the accept socket.
	if ( g_listen_socket != INVALID_SOCKET )
	{
		CreateAcceptSocket( g_listen_socket, g_server_use_ipv6 );
	}
}

void CleanupServer()
{
	// When we shutdown/close g_listen_socket, g_listen_context will complete with a status of FALSE and we can then clean it up.
	if ( g_listen_socket != INVALID_SOCKET )
	{
#ifdef ENABLE_LOGGING
		WriteLog( LOG_INFO_MISC, "Shutting down web server" );
#endif

		SOCKET del_listen_socket = g_listen_socket;
		g_listen_socket = INVALID_SOCKET;

		g_listen_context = NULL;	// Freed in CleanupConnection.

		_shutdown( del_listen_socket, SD_BOTH );
		_closesocket( del_listen_socket );
	}
}

THREAD_RETURN IOCPDownloader( void * /*pArguments*/ )
{
	HANDLE *g_ThreadHandles = NULL;

	DWORD dwThreadCount = cfg_thread_count;

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Initializing Winsock" );
#endif

	if ( ws2_32_state == WS2_32_STATE_SHUTDOWN )
	{
		#ifndef WS2_32_USE_STATIC_LIB
			if ( !InitializeWS2_32() ){ goto HARD_CLEANUP; }
		#else
			StartWS2_32();
		#endif
	}

	// Loads the CreateEx function pointer. Required for overlapped connects.
	if ( !LoadConnectEx() )
	{
		goto HARD_CLEANUP;
	}

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Initializing SSL" );
#endif

	// Load our SSL functions.
	if ( ssl_state == SSL_STATE_SHUTDOWN )
	{
		if ( __SSL_library_init() == 0 )
		{
			goto HARD_CLEANUP;
		}
	}

	if ( g_use_openssl )
	{
		InitializeSSL_CTXs();
	}

	if ( cfg_enable_server )
	{
		InitializeServerInfo();
	}

	g_ThreadHandles = ( HANDLE * )GlobalAlloc( GMEM_FIXED, sizeof( HANDLE ) * dwThreadCount );
	if ( g_ThreadHandles == NULL )
	{
		goto HARD_CLEANUP;
	}

	for ( unsigned int i = 0; i < dwThreadCount; ++i )
	{
		g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
	}

	g_cleanup_event[ 0 ] = _WSACreateEvent();
	if ( g_cleanup_event[ 0 ] == WSA_INVALID_EVENT )
	{
		goto CLEANUP;
	}

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Initializing IOCP with %lu threads", dwThreadCount );
#endif

	g_hIOCP = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( g_hIOCP == NULL )
	{
		goto CLEANUP;
	}

	_WSAResetEvent( g_cleanup_event[ 0 ] );

	HANDLE hThread;
	DWORD dwThreadId;

	// Spawn our IOCP worker threads.
	for ( DWORD dwCPU = 0; dwCPU < dwThreadCount; ++dwCPU )
	{
		// Create worker threads to service the overlapped I/O requests.
		hThread = _CreateThread( NULL, 0, IOCPConnection, g_hIOCP, 0, &dwThreadId );
		if ( hThread == NULL )
		{
			break;
		}

		g_ThreadHandles[ dwCPU ] = hThread;
	}

	if ( cfg_enable_server )
	{
		StartServer();
	}

	if ( g_downloader_ready_semaphore != NULL )
	{
		ReleaseSemaphore( g_downloader_ready_semaphore, 1, NULL );
	}

	g_timeout_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

	HANDLE timeout_handle = _CreateThread( NULL, 0, Timeout, NULL, 0, NULL );
	if ( timeout_handle != NULL )
	{
		SetThreadPriority( timeout_handle, THREAD_PRIORITY_LOWEST );
		CloseHandle( timeout_handle );
	}

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Waiting on IOCP threads" );
#endif

	_WSAWaitForMultipleEvents( 1, g_cleanup_event, TRUE, WSA_INFINITE, FALSE );

	g_end_program = true;

	// Causes the IOCP worker threads to exit.
	if ( g_hIOCP != NULL )
	{
		for ( DWORD i = 0; i < dwThreadCount; ++i )
		{
			PostQueuedCompletionStatus( g_hIOCP, 0, 0, NULL );
		}
	}

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Waiting for IOCP threads to exit" );
#endif

	// Make sure IOCP worker threads have exited.
	if ( WaitForMultipleObjects( dwThreadCount, g_ThreadHandles, TRUE, 1000 ) == WAIT_OBJECT_0 )
	{
		for ( DWORD i = 0; i < dwThreadCount; ++i )
		{
			if ( g_ThreadHandles[ i ] != INVALID_HANDLE_VALUE )
			{
				CloseHandle( g_ThreadHandles[ i ] );
				g_ThreadHandles[ i ] = INVALID_HANDLE_VALUE;
			}
		}
	}

	if ( g_timeout_semaphore != NULL )
	{
		ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
	}

	if ( g_listen_socket != INVALID_SOCKET )
	{
		_shutdown( g_listen_socket, SD_BOTH );
		_closesocket( g_listen_socket );
		g_listen_socket = INVALID_SOCKET;
	}

	// Clean up our listen context.
	FreeListenContext();

	// Clean up our context list.
	FreeContexts();

	if ( g_use_openssl )
	{
		FreeSSL_CTXs();
	}

	download_queue = NULL;
	g_total_downloading = 0;

	if ( g_hIOCP != NULL )
	{
		CloseHandle( g_hIOCP );
		g_hIOCP = NULL;
	}

CLEANUP:

	if ( g_ThreadHandles != NULL )
	{
		GlobalFree( g_ThreadHandles );
		g_ThreadHandles = NULL;
	}

	if ( g_cleanup_event[ 0 ] != WSA_INVALID_EVENT )
	{
		_WSACloseEvent( g_cleanup_event[ 0 ] );
		g_cleanup_event[ 0 ] = WSA_INVALID_EVENT;
	}

HARD_CLEANUP:

	if ( g_downloader_ready_semaphore != NULL )
	{
		ReleaseSemaphore( g_downloader_ready_semaphore, 1, NULL );
	}

#ifdef ENABLE_LOGGING
	WriteLog( LOG_INFO_MISC, "Exiting main IOCP thread" );
#endif

	_ExitThread( 0 );
	//return 0;
}

void StopIOCPDownloader()
{
	if ( ws2_32_state == WS2_32_STATE_RUNNING )
	{
		if ( g_downloader_ready_semaphore == NULL )
		{
			g_downloader_ready_semaphore = CreateSemaphore( NULL, 0, 1, NULL );
		}

		_WSASetEvent( g_cleanup_event[ 0 ] );

		// Wait for IOCPDownloader to clean up. 10 second timeout in case we miss the release.
		WaitForSingleObject( g_downloader_ready_semaphore, 10000 );
		CloseHandle( g_downloader_ready_semaphore );
		g_downloader_ready_semaphore = NULL;
	}
}

bool LoadConnectEx()
{
	bool ret = false;

	DWORD bytes = 0;
	GUID connectex_guid = WSAID_CONNECTEX;

	if ( _ConnectEx == NULL )
	{
		SOCKET tmp_socket = CreateSocket();
		if ( tmp_socket != INVALID_SOCKET )
		{
			// Load the ConnectEx extension function from the provider for this socket.
			ret = ( _WSAIoctl( tmp_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof( connectex_guid ), &_ConnectEx, sizeof( _ConnectEx ), &bytes, NULL, NULL ) == SOCKET_ERROR ? false : true );

			_closesocket( tmp_socket );
		}
	}
	else
	{
		ret = true;
	}

	return ret;
}

SOCKET_CONTEXT *UpdateCompletionPort( SOCKET socket, bool use_ssl, unsigned char ssl_version, bool add_context, bool is_server )
{
	SOCKET_CONTEXT *context = CreateSocketContext();
	if ( context )
	{
		context->socket = socket;

		context->overlapped.current_operation = IO_Accept;

		// Create an SSL/TLS object for incoming SSL/TLS connections, but not for SSL/TLS tunnel connections.
		if ( use_ssl )
		{
			DWORD protocol = 0;

			if ( g_use_openssl )
			{
				bool success = false;

				_SSL_O *_ssl_o = ( _SSL_O * )GlobalAlloc( GPTR, sizeof( _SSL_O ) );
				if ( _ssl_o != NULL )
				{
					context->_ssl_o = _ssl_o;

					context->_ssl_o->ssl = _SSL_new( ( is_server ? g_server_ssl_ctx : g_client_ssl_ctx[ ssl_version ] ) );
					if ( context->_ssl_o->ssl != NULL )
					{
						context->_ssl_o->rbio = _BIO_new( _BIO_s_mem() );
						context->_ssl_o->wbio = _BIO_new( _BIO_s_mem() );

						if ( context->_ssl_o->rbio != NULL && context->_ssl_o->wbio != NULL )
						{
							_SSL_set_bio( context->_ssl_o->ssl, context->_ssl_o->rbio, context->_ssl_o->wbio );

							success = true;
						}
						else
						{
							_BIO_free( context->_ssl_o->rbio );
							_BIO_free( context->_ssl_o->wbio );

							context->_ssl_o->rbio = NULL;
							context->_ssl_o->wbio = NULL;
						}
					}
				}

				if ( !success )
				{
					OpenSSL_FreeInfo( &context->_ssl_o );
				}
			}
			else
			{
				switch ( ssl_version )
				{
					case 5:	protocol |= SP_PROT_TLS1_3;
					case 4:	protocol |= SP_PROT_TLS1_2;
					case 3:	protocol |= SP_PROT_TLS1_1;
					case 2:	protocol |= SP_PROT_TLS1;
					case 1:	protocol |= SP_PROT_SSL3;
					case 0:	{ if ( ssl_version < 4 ) { protocol |= SP_PROT_SSL2; } }
				}

				_SSL_S *_ssl_s = __SSL_new( protocol, is_server );
				if ( _ssl_s == NULL )
				{
					DeleteCriticalSection( &context->context_cs );

					if ( context->buffer != NULL ) { GlobalFree( context->buffer ); }

					GlobalFree( context );
					context = NULL;

					return NULL;
				}

				_ssl_s->s = socket;

				context->_ssl_s = _ssl_s;
			}
		}

		g_hIOCP = CreateIoCompletionPort( ( HANDLE )socket, g_hIOCP, ( DWORD_PTR )context, 0 );
		if ( g_hIOCP != NULL )
		{
			if ( add_context )
			{
				RANGE_INFO *ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );
				context->header_info.range_info = ri;

				context->context_node.data = context;

				EnterCriticalSection( &context_list_cs );

				// Add to the global download list.
				DLL_AddNode( &g_context_list, &context->context_node, 0 );

				LeaveCriticalSection( &context_list_cs );
			}
		}
		else
		{
			if ( context->_ssl_s != NULL ) { __SSL_free( context->_ssl_s ); }

			OpenSSL_FreeInfo( &context->_ssl_o );

			DeleteCriticalSection( &context->context_cs );

			if ( context->buffer != NULL ) { GlobalFree( context->buffer ); }

			GlobalFree( context );
			context = NULL;
		}
	}

	return context;
}

SOCKET CreateListenSocket()
{
	int nRet = 0;

	SOCKET socket = INVALID_SOCKET;

	DWORD bytes = 0;
	GUID acceptex_guid = WSAID_ACCEPTEX;	// GUID to Microsoft specific extensions

	struct addrinfoW hints;
	struct addrinfoW *addrlocal = NULL;

	// Resolve the interface
	_memzero( &hints, sizeof( addrinfoW ) );
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_IP;

	SOCKET listen_socket = INVALID_SOCKET;

	wchar_t cport[ 6 ];
	__snwprintf( cport, 6, L"%hu", cfg_server_port );

	g_server_use_ipv6 = false;

	g_server_port = cfg_server_port;

	if ( g_server_domain != NULL )
	{
		GlobalFree( g_server_domain );
		g_server_domain = NULL;
	}

	// Use Hostname or IPv6 Address.
	if ( cfg_server_address_type == 0 )
	{
		wchar_t *hostname = ( g_server_punycode_hostname != NULL ? g_server_punycode_hostname : cfg_server_hostname );

		g_server_domain = GetUTF8Domain( hostname );

		nRet = _GetAddrInfoW( hostname, cport, &hints, &addrlocal );
		if ( nRet == WSAHOST_NOT_FOUND )
		{
			g_server_use_ipv6 = true;

			hints.ai_family = AF_INET6;	// Try IPv6
			nRet = _GetAddrInfoW( hostname, cport, &hints, &addrlocal );
		}

		if ( nRet != 0 )
		{
			goto CLEANUP;
		}

		// Check the IPv6 address' formatting. It should be surrounded by brackets.
		// GetAddrInfoW supports it with or without, but we want it to have it.
		if ( g_server_use_ipv6 )
		{
			if ( g_server_domain != NULL && *g_server_domain != '[' )
			{
				int g_server_domain_length = lstrlenA( g_server_domain );
				char *new_g_server_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( g_server_domain_length + 3 ) );	// 2 brackets and the NULL character.
				if ( new_g_server_domain != NULL )
				{
					new_g_server_domain[ 0 ] = '[';
					_memcpy_s( new_g_server_domain + 1, g_server_domain_length + 2, g_server_domain, g_server_domain_length );
					new_g_server_domain[ g_server_domain_length + 1 ] = ']';
					new_g_server_domain[ g_server_domain_length + 2 ] = 0;	// Sanity.
				}

				GlobalFree( g_server_domain );
				g_server_domain = new_g_server_domain;
			}
		}
	}
	else	// Use IPv4 Address.
	{
		struct sockaddr_in src_addr;
		_memzero( &src_addr, sizeof( sockaddr_in ) );

		src_addr.sin_family = AF_INET;
		src_addr.sin_addr.s_addr = _htonl( cfg_server_ip_address );

		wchar_t wcs_ip[ 16 ];
		DWORD wcs_ip_length = 16;
		_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

		g_server_domain = GetUTF8Domain( wcs_ip );

		if ( _GetAddrInfoW( wcs_ip, cport, &hints, &addrlocal ) != 0 )
		{
			goto CLEANUP;
		}
	}

	if ( addrlocal == NULL )
	{
		goto CLEANUP;
	}

	socket = CreateSocket( g_server_use_ipv6 );
	if ( socket == INVALID_SOCKET )
	{
		goto CLEANUP;
	}

	nRet = _bind( socket, addrlocal->ai_addr, ( int )addrlocal->ai_addrlen );
	if ( nRet == SOCKET_ERROR )
	{
		goto CLEANUP;
	}

	nRet = _listen( socket, SOMAXCONN );
	if ( nRet == SOCKET_ERROR )
	{
		goto CLEANUP;
	}

	// We need only do this once.
	if ( _AcceptEx == NULL )
	{
		// Load the AcceptEx extension function from the provider.
		// It doesn't matter what socket we use so long as it's valid.
		nRet = _WSAIoctl( socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof( acceptex_guid ), &_AcceptEx, sizeof( _AcceptEx ), &bytes, NULL, NULL );
		if ( nRet == SOCKET_ERROR )
		{
			goto CLEANUP;
		}
	}

	listen_socket = socket;
	socket = INVALID_SOCKET;

CLEANUP:

	if ( socket != INVALID_SOCKET )
	{
		_closesocket( socket );
	}

	if ( addrlocal != NULL )
	{
		_FreeAddrInfoW( addrlocal );
	}

	return listen_socket;
}

char CreateAcceptSocket( SOCKET listen_socket, bool use_ipv6 )
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;

	if ( g_listen_context == NULL )
	{
		g_listen_context = UpdateCompletionPort( listen_socket, cfg_server_enable_ssl, cfg_server_ssl_version, false, true );
		if ( g_listen_context == NULL )
		{
			return LA_STATUS_FAILED;
		}
	}

	// The accept socket will inherit the listen socket's properties when it completes. IPv6 doesn't actually have to be set here.
	g_listen_context->socket = CreateSocket( use_ipv6 );
	if ( g_listen_context->socket == INVALID_SOCKET )
	{
		return LA_STATUS_FAILED;
	}

	InterlockedIncrement( &g_listen_context->pending_operations );

	// Accept a connection without waiting for any data. (dwReceiveDataLength = 0)
	nRet = _AcceptEx( listen_socket, g_listen_context->socket, ( LPVOID )( g_listen_context->buffer ), 0, sizeof( SOCKADDR_STORAGE ) + 16, sizeof( SOCKADDR_STORAGE ) + 16, &dwRecvNumBytes, ( OVERLAPPED * )&g_listen_context->overlapped );
	if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		InterlockedDecrement( &g_listen_context->pending_operations );

		return LA_STATUS_FAILED;
	}

	return LA_STATUS_OK;
}

SOCKET CreateSocket( bool IPv6 )
{
	int nZero = 0;
	SOCKET socket = INVALID_SOCKET;

	socket = _WSASocketW( ( IPv6 ? AF_INET6 : AF_INET ), SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED ); 
	if ( socket != INVALID_SOCKET )
	{
		// Disable send buffering on the socket.
		_setsockopt( socket, SOL_SOCKET, SO_SNDBUF, ( char * )&nZero, sizeof( nZero ) );
	}

	return socket;
}

THREAD_RETURN PromptRenameFile( void *pArguments )
{
	unsigned char rename_only = ( unsigned char )pArguments;

	bool skip_processing = false;

	do
	{
		SOCKET_CONTEXT *context = NULL;

		EnterCriticalSection( &rename_file_prompt_list_cs );

		DoublyLinkedList *context_node = rename_file_prompt_list;

		DLL_RemoveNode( &rename_file_prompt_list, context_node );

		if ( context_node != NULL )
		{
			context = ( SOCKET_CONTEXT * )context_node->data;

			GlobalFree( context_node );
		}

		LeaveCriticalSection( &rename_file_prompt_list_cs );

		if ( context != NULL )
		{
			DOWNLOAD_INFO *di = context->download_info;
			if ( di != NULL )
			{
				wchar_t prompt_message[ MAX_PATH + 512 ];
				wchar_t file_path[ MAX_PATH ];

				int filename_offset;
				int file_extension_offset;

				if ( cfg_use_temp_download_directory )
				{
					int filename_length = GetTemporaryFilePath( di, file_path );

					filename_offset = g_temp_download_directory_length + 1;
					file_extension_offset = filename_offset + get_file_extension_offset( di->shared_info->file_path + di->shared_info->filename_offset, filename_length );
				}
				else
				{
					filename_offset = di->shared_info->filename_offset;
					file_extension_offset = di->shared_info->file_extension_offset;

					GetDownloadFilePath( di, file_path );
				}

				if ( rename_only == 0 )
				{
					// If the last return value was not set to remember our choice, then prompt again.
					if ( g_rename_file_cmb_ret != CMBIDRENAMEALL && g_rename_file_cmb_ret != CMBIDOVERWRITEALL && g_rename_file_cmb_ret != CMBIDSKIPALL )
					{
						__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___already_exists, file_path );

						g_rename_file_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_RENAMEOVERWRITESKIPALL );
					}
				}

				// Rename the file and try again.
				if ( rename_only == 1 || g_rename_file_cmb_ret == CMBIDRENAME || g_rename_file_cmb_ret == CMBIDRENAMEALL )
				{
					// Creates a tree of active and queued downloads.
					dllrbt_tree *add_files_tree = CreateFilenameTree();

					bool rename_succeeded;

					EnterCriticalSection( &di->di_cs );

					rename_succeeded = RenameFile( add_files_tree,
												   di->shared_info->file_path, &di->shared_info->filename_offset, &di->shared_info->file_extension_offset,
												   file_path, filename_offset, file_extension_offset );

					LeaveCriticalSection( &di->di_cs );

					// The tree is only used to determine duplicate filenames.
					DestroyFilenameTree( add_files_tree );

					if ( !rename_succeeded )
					{
						if ( g_rename_file_cmb_ret2 != CMBIDOKALL && !( di->shared_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS ) )
						{
							__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___could_not_be_renamed, file_path );

							g_rename_file_cmb_ret2 = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_OKALL );
						}

						EnterCriticalSection( &context->context_cs );

						context->status = STATUS_FILE_IO_ERROR;

						if ( context->cleanup == 0 )
						{
							context->cleanup = 1;	// Auto cleanup.

							InterlockedIncrement( &context->pending_operations );
							context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
						}
					}
					else	// Continue where we left off when getting the content.
					{
						EnterCriticalSection( &context->context_cs );

						EnterCriticalSection( &di->di_cs );
						di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESUME;
						LeaveCriticalSection( &di->di_cs );

						context->status &= ~STATUS_INPUT_REQUIRED;

						InterlockedIncrement( &context->pending_operations );
						context->overlapped.current_operation = IO_ResumeGetContent;
						PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
					}
				}
				else if ( g_rename_file_cmb_ret == CMBIDFAIL || g_rename_file_cmb_ret == CMBIDSKIP || g_rename_file_cmb_ret == CMBIDSKIPALL ) // Skip the rename or overwrite if the return value fails, or the user selected skip.
				{
					EnterCriticalSection( &context->context_cs );

					EnterCriticalSection( &di->di_cs );

					SetStatus( di, STATUS_SKIPPED );

					LeaveCriticalSection( &di->di_cs );

					context->status = STATUS_SKIPPED;

					if ( context->cleanup == 0 )
					{
						context->cleanup = 1;	// Auto cleanup.

						InterlockedIncrement( &context->pending_operations );
						context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
					}
				}
				else	// Continue where we left off when getting the content.
				{
					EnterCriticalSection( &context->context_cs );

					EnterCriticalSection( &di->di_cs );
					di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESUME;
					LeaveCriticalSection( &di->di_cs );

					context->status &= ~STATUS_INPUT_REQUIRED;

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_ResumeGetContent;
					PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else	// No DOWNLOAD_INFO? Then skip.
			{
				EnterCriticalSection( &context->context_cs );

				context->status = STATUS_SKIPPED;

				if ( context->cleanup == 0 )
				{
					context->cleanup = 1;	// Auto cleanup.

					InterlockedIncrement( &context->pending_operations );
					context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
				}
			}

			LeaveCriticalSection( &context->context_cs );
		}

		EnterCriticalSection( &rename_file_prompt_list_cs );

		if ( rename_file_prompt_list == NULL )
		{
			skip_processing = true;

			rename_file_prompt_active = false;
		}

		LeaveCriticalSection( &rename_file_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN PromptFileSize( void * /*pArguments*/ )
{
	bool skip_processing = false;

	do
	{
		SOCKET_CONTEXT *context = NULL;

		EnterCriticalSection( &file_size_prompt_list_cs );

		DoublyLinkedList *context_node = file_size_prompt_list;

		DLL_RemoveNode( &file_size_prompt_list, context_node );

		if ( context_node != NULL )
		{
			context = ( SOCKET_CONTEXT * )context_node->data;

			GlobalFree( context_node );
		}

		LeaveCriticalSection( &file_size_prompt_list_cs );

		if ( context != NULL )
		{
			DOWNLOAD_INFO *di = context->download_info;
			if ( di != NULL )
			{
				// If we don't want to prevent all large downloads, then prompt the user.
				if ( g_file_size_cmb_ret != CMBIDNOALL && g_file_size_cmb_ret != CMBIDYESALL )
				{
					wchar_t file_path[ MAX_PATH ];
					if ( cfg_use_temp_download_directory )
					{
						GetTemporaryFilePath( di, file_path );
					}
					else
					{
						GetDownloadFilePath( di, file_path );
					}

					wchar_t prompt_message[ MAX_PATH + 512 ];
					__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___will_be___size, file_path, di->shared_info->file_size );
					g_file_size_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_YESNOALL );
				}

				EnterCriticalSection( &context->context_cs );

				// Close all large downloads.
				if ( g_file_size_cmb_ret == CMBIDNO || g_file_size_cmb_ret == CMBIDNOALL )
				{
					EnterCriticalSection( &di->di_cs );

					SetStatus( di, STATUS_SKIPPED );

					LeaveCriticalSection( &di->di_cs );

					context->status = STATUS_SKIPPED;

					if ( context->cleanup == 0 )
					{
						InterlockedIncrement( &context->pending_operations );

						if ( context->ssh != NULL )
						{
							context->status &= ~STATUS_INPUT_REQUIRED;

							//context->cleanup = 0;
							context->overlapped_close.current_operation = IO_SFTPCleanup;

							context->overlapped.current_operation = IO_SFTPReadContent;//IO_SFTPResumeReadContent;
							PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
						}
						else
						{
							context->cleanup = 1;	// Auto cleanup.
							context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
						}
					}
				}
				else	// Continue where we left off when getting the content.
				{
					EnterCriticalSection( &di->di_cs );
					di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESUME;
					LeaveCriticalSection( &di->di_cs );

					context->status &= ~STATUS_INPUT_REQUIRED;

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = ( context->ssh != NULL ? IO_SFTPResumeReadContent : IO_ResumeGetContent );
					PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else	// No DOWNLOAD_INFO? Then skip.
			{
				EnterCriticalSection( &context->context_cs );

				context->status = STATUS_SKIPPED;

				if ( context->cleanup == 0 )
				{
					InterlockedIncrement( &context->pending_operations );

					if ( context->ssh != NULL )
					{
						context->status &= ~STATUS_INPUT_REQUIRED;

						//context->cleanup = 0;
						context->overlapped_close.current_operation = IO_SFTPCleanup;

						context->overlapped.current_operation = IO_SFTPReadContent;//IO_SFTPResumeReadContent;
						PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
					}
					else
					{
						context->cleanup = 1;	// Auto cleanup.
						context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
					}
				}
			}

			LeaveCriticalSection( &context->context_cs );
		}

		EnterCriticalSection( &file_size_prompt_list_cs );

		if ( file_size_prompt_list == NULL )
		{
			skip_processing = true;

			file_size_prompt_active = false;
		}

		LeaveCriticalSection( &file_size_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN PromptLastModified( void *pArguments )
{
	unsigned char restart_only = ( unsigned char )pArguments;

	bool skip_processing = false;

	do
	{
		SOCKET_CONTEXT *context = NULL;

		EnterCriticalSection( &last_modified_prompt_list_cs );

		DoublyLinkedList *context_node = last_modified_prompt_list;

		DLL_RemoveNode( &last_modified_prompt_list, context_node );

		if ( context_node != NULL )
		{
			context = ( SOCKET_CONTEXT * )context_node->data;

			GlobalFree( context_node );
		}

		LeaveCriticalSection( &last_modified_prompt_list_cs );

		if ( context != NULL )
		{
			DOWNLOAD_INFO *di = context->download_info;
			if ( di != NULL )
			{
				if ( context->processed_header && ( di->download_operations & DOWNLOAD_OPERATION_MODIFIED ) )
				{
					// This is a good reason why they say not to use the gotos.
					EnterCriticalSection( &context->context_cs );

					if ( di->download_operations & DOWNLOAD_OPERATION_MODIFIED_CONTINUE )
					{
						goto MOD_CONTINUE;
					}
					else if ( di->download_operations & DOWNLOAD_OPERATION_MODIFIED_RESTART )
					{
						goto MOD_RESTART;
					}
					else// if ( di->download_operations & DOWNLOAD_OPERATION_MODIFIED_SKIP )
					{
						goto MOD_SKIP;
					}
				}
				else
				{
					wchar_t prompt_message[ MAX_PATH + 512 ];

					wchar_t file_path[ MAX_PATH ];
					if ( cfg_use_temp_download_directory )
					{
						GetTemporaryFilePath( di, file_path );
					}
					else
					{
						GetDownloadFilePath( di, file_path );
					}

					if ( restart_only == 0 )
					{
						// If the last return value was not set to remember our choice, then prompt again.
						if ( g_last_modified_cmb_ret != CMBIDCONTINUEALL && g_last_modified_cmb_ret != CMBIDRESTARTALL && g_last_modified_cmb_ret != CMBIDSKIPALL )
						{
							__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___has_been_modified, file_path );

							g_last_modified_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_CONTINUERESTARTSKIPALL );
						}
					}

					EnterCriticalSection( &context->context_cs );

					// Restart the download.
					if ( restart_only == 1 || g_last_modified_cmb_ret == CMBIDRESTART || g_last_modified_cmb_ret == CMBIDRESTARTALL )
					{
						if ( context->processed_header )
						{
							di->download_operations |= DOWNLOAD_OPERATION_MODIFIED_RESTART;
						}

						EnterCriticalSection( &di->di_cs );

						SetStatus( di, STATUS_STOPPED | STATUS_RESTART );

						LeaveCriticalSection( &di->di_cs );
MOD_RESTART:
						context->status = STATUS_STOPPED | STATUS_RESTART;

						if ( context->cleanup == 0 )
						{
							InterlockedIncrement( &context->pending_operations );

							if ( context->ssh != NULL )
							{
								context->status &= ~STATUS_INPUT_REQUIRED;

								//context->cleanup = 0;
								context->overlapped_close.current_operation = IO_SFTPCleanup;

								context->overlapped.current_operation = IO_SFTPReadContent;//IO_SFTPResumeReadContent;
								PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
							}
							else
							{
								context->cleanup = 1;	// Auto cleanup.
								context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
								PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
							}
						}
					}
					else if ( g_last_modified_cmb_ret == CMBIDFAIL || g_last_modified_cmb_ret == CMBIDSKIP || g_last_modified_cmb_ret == CMBIDSKIPALL ) // Skip the download if the return value fails, or the user selected skip.
					{
						if ( context->processed_header )
						{
							di->download_operations |= DOWNLOAD_OPERATION_MODIFIED_SKIP;
						}

						EnterCriticalSection( &di->di_cs );

						SetStatus( di, STATUS_SKIPPED );

						LeaveCriticalSection( &di->di_cs );
MOD_SKIP:
						context->status = STATUS_SKIPPED;

						if ( context->cleanup == 0 )
						{
							InterlockedIncrement( &context->pending_operations );

							if ( context->ssh != NULL )
							{
								context->status &= ~STATUS_INPUT_REQUIRED;

								//context->cleanup = 0;
								context->overlapped_close.current_operation = IO_SFTPCleanup;

								context->overlapped.current_operation = IO_SFTPReadContent/*IO_SFTPResumeReadContent*/;
								PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
							}
							else
							{
								context->cleanup = 1;	// Auto cleanup.
								context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
								PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
							}
						}
					}
					else	// Continue where we left off when getting the content.
					{
						if ( context->processed_header )
						{
							di->download_operations |= DOWNLOAD_OPERATION_MODIFIED_CONTINUE;
						}

						EnterCriticalSection( &di->di_cs );

						di->last_modified.HighPart = context->header_info.last_modified.dwHighDateTime;
						di->last_modified.LowPart = context->header_info.last_modified.dwLowDateTime;

						LeaveCriticalSection( &di->di_cs );
MOD_CONTINUE:

						EnterCriticalSection( &di->di_cs );
						di->shared_info->download_operations |= DOWNLOAD_OPERATION_RESUME;
						LeaveCriticalSection( &di->di_cs );

						context->status &= ~STATUS_INPUT_REQUIRED;

						InterlockedIncrement( &context->pending_operations );
						context->overlapped.current_operation = ( context->ssh != NULL ? IO_SFTPResumeReadContent : IO_ResumeGetContent );
						PostQueuedCompletionStatus( g_hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
					}
				}
			}
			else	// No DOWNLOAD_INFO? Then skip.
			{
				EnterCriticalSection( &context->context_cs );

				context->status = STATUS_SKIPPED;

				if ( context->cleanup == 0 )
				{
					InterlockedIncrement( &context->pending_operations );

					if ( context->ssh != NULL )
					{
						context->status &= ~STATUS_INPUT_REQUIRED;

						//context->cleanup = 0;
						context->overlapped_close.current_operation = IO_SFTPCleanup;

						context->overlapped.current_operation = IO_SFTPReadContent;//IO_SFTPResumeReadContent;
						PostQueuedCompletionStatus( g_hIOCP, 0/*context->current_bytes_read*/, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
					}
					else
					{
						context->cleanup = 1;	// Auto cleanup.
						context->overlapped_close.current_operation = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? IO_Shutdown : IO_Close );
						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped_close );
					}
				}
			}

			LeaveCriticalSection( &context->context_cs );
		}

		EnterCriticalSection( &last_modified_prompt_list_cs );

		if ( last_modified_prompt_list == NULL )
		{
			skip_processing = true;

			last_modified_prompt_active = false;
		}

		LeaveCriticalSection( &last_modified_prompt_list_cs );
	}
	while ( !skip_processing );

	_ExitThread( 0 );
	//return 0;
}

DWORD WINAPI IOCPConnection( LPVOID WorkThreadContext )
{
	HANDLE hIOCP = ( HANDLE )WorkThreadContext;
	OVERLAPPEDEX *overlapped = NULL;
	DWORD io_size = 0;
	SOCKET_CONTEXT *context = NULL;
	IO_OPERATION *current_operation = NULL;
	IO_OPERATION *next_operation = NULL;

	BOOL completion_status = TRUE;

	bool use_ssl = false;

	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;
	bool sent = false;
	int nRet = 0;
	DWORD dwFlags = 0;

	for ( ;; )
	{
		completion_status = GetQueuedCompletionStatus( hIOCP, &io_size, ( ULONG_PTR * )&context, ( OVERLAPPED ** )&overlapped, INFINITE );

		if ( g_end_program )
		{
			break;
		}

		if ( overlapped != NULL && overlapped->context != NULL )
		{
			context = overlapped->context;

			current_operation = &overlapped->current_operation;
			next_operation = &overlapped->next_operation;
		}
		else
		{
			continue;
		}

		InterlockedExchange( &context->timeout, 0 );	// Reset timeout counter.

		use_ssl = ( context->_ssl_s != NULL || context->_ssl_o != NULL ? true : false );

		if ( completion_status == FALSE )
		{
			bool skip_process = false;

			EnterCriticalSection( &context->context_cs );

			InterlockedDecrement( &context->pending_operations );

			// This can happen if the connection timed out and we've forced the cleanup.
			if ( context->pending_operations > 0 )
			{
				skip_process = true;
			}

			context->cleanup = 1;	// Auto cleanup.

			LeaveCriticalSection( &context->context_cs );

			if ( skip_process )
			{
				continue;
			}
			else// if ( *current_operation != IO_Shutdown && *current_operation != IO_Close )
			{
				if ( *current_operation == IO_Connect )	// We couldn't establish a connection.
				{
					EnterCriticalSection( &context->context_cs );

					if ( IS_STATUS_NOT( context->status,
							STATUS_STOPPED |
							STATUS_REMOVE |
							STATUS_RESTART |
							STATUS_UPDATING ) )	// Stop, Stop and Remove, Restart, or Updating.
					{
						context->timed_out = TIME_OUT_RETRY;

						if ( IS_STATUS( context->status, STATUS_PAUSED ) )
						{
							context->is_paused = true;	// Tells us how to stop the download if it's pausing/paused.

							skip_process = true;
						}
					}

					LeaveCriticalSection( &context->context_cs );

					if ( skip_process )
					{
						continue;
					}
				}
				else if ( *current_operation == IO_WriteFile )
				{
					EnterCriticalSection( &context->context_cs );

					if ( context->download_info != NULL )
					{
						EnterCriticalSection( &context->download_info->di_cs );

						if ( GetLastError() == ERROR_DISK_FULL )
						{
							SetStatus( context->download_info, STATUS_INSUFFICIENT_DISK_SPACE );
							context->status = STATUS_INSUFFICIENT_DISK_SPACE;
						}
						else
						{
							SetStatus( context->download_info, STATUS_FILE_IO_ERROR );
							context->status = STATUS_FILE_IO_ERROR;
						}

						LeaveCriticalSection( &context->download_info->di_cs );
					}

					LeaveCriticalSection( &context->context_cs );
				}

				if ( context->ssh != NULL )
				{
					context->overlapped_close.current_operation = IO_SFTPCleanup;
				}
				else
				{
					*current_operation = IO_Close;	// Can't go through a shutdown routine if the connection is dead. So we can only close it.
				}
			}
		}
		else
		{
			EnterCriticalSection( &context->context_cs );

			InterlockedDecrement( &context->pending_operations );

			LeaveCriticalSection( &context->context_cs );

			if ( *current_operation == IO_GetContent ||
				 *current_operation == IO_SFTPReadContent ||
				 *current_operation == IO_GetCONNECTResponse ||
				 *current_operation == IO_SOCKSResponse )
			{
				// If there's no more data that was read.
				// Can occur when no file size has been set and the connection header is set to close.
				if ( io_size == 0 )
				{
					if ( context->ssh != NULL )
					{
						context->overlapped_close.current_operation = IO_SFTPCleanup;
					}
					else
					{
						if ( *current_operation != IO_GetContent )
						{
							// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
							*current_operation = IO_Close;
						}
						else
						{
							if ( context->_ssl_o != NULL )
							{
								// If there's more data that needs to be read from the BIO, then continue the last operation.
								int pending = _BIO_pending( context->_ssl_o->rbio );
								if ( pending > 0 )
								{
									io_size = pending;	// The amount of data that'll be read from the BIO.
								}
								else
								{
									*current_operation = IO_Shutdown;
								}
							}
							else
							{
								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							}
						}
					}
				}
				else
				{
					bool skip_process = false;
					bool sleep = false;

					EnterCriticalSection( &context->context_cs );

					if ( IS_STATUS( context->status,
							STATUS_STOPPED |
							STATUS_REMOVE |
							STATUS_RESTART |
							STATUS_UPDATING ) )	// Stop, Stop and Remove, Restart, or Updating.
					{
						if ( context->ssh != NULL )
						{
							context->overlapped_close.current_operation = IO_SFTPCleanup;
						}
						else
						{
							if ( *current_operation != IO_GetContent )
							{
								// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
								*current_operation = IO_Close;
							}
							else
							{
								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							}
						}
					}
					else if ( IS_STATUS( context->status, STATUS_PAUSED ) )	// Pause.
					{
						context->current_bytes_read = io_size;

						context->is_paused = true;	// Tells us how to stop the download if it's pausing/paused.

						skip_process = true;
					}
					else if ( ( cfg_download_speed_limit > 0 &&
							  ( g_session_total_downloaded - g_session_last_total_downloaded ) > cfg_download_speed_limit ) ||
							  ( context->download_info != NULL &&
							( ( context->download_info->shared_info->download_speed_limit > 0 &&
							  ( context->download_info->shared_info->downloaded - context->download_info->shared_info->last_downloaded ) > context->download_info->shared_info->download_speed_limit ) ||
							  ( context->download_info->download_speed_limit > 0 &&
							  ( context->download_info->downloaded - context->download_info->last_downloaded ) > context->download_info->download_speed_limit ) ) ) ) // Preempt the next receive.
					{
						sleep = true;

						context->current_bytes_read = io_size;

						InterlockedIncrement( &context->pending_operations );
						PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

						skip_process = true;
					}

					LeaveCriticalSection( &context->context_cs );

					// Make sure this isn't in the context's critical section.
					if ( sleep )
					{
						Sleep( 1 );	// Prevents high CPU usage for some reason.
					}

					if ( skip_process )
					{
						continue;
					}
				}
			}
		}

		switch ( *current_operation )
		{
			case IO_Accept:
			{
				bool free_context = false;

				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					SOCKET_CONTEXT *new_context = NULL;

					SOCKET_CONTEXT *ftp_control_context = context->ftp_context;
					bool is_ftp_data_connection;
					SOCKET listen_socket;

					if ( ftp_control_context != NULL )
					{
						EnterCriticalSection( &ftp_control_context->context_cs );

						// The Listen context points to the Control context and its download info and vice versa.
						// Set the pointers to NULL so that when it's freed it doesn't have access to the Control context anymore.
						context->ftp_context = NULL;
						context->download_info = NULL;

						// Make sure the Control context no longer has access to the Listen context.
						ftp_control_context->ftp_context = NULL;

						is_ftp_data_connection = true;

						listen_socket = ftp_control_context->listen_socket;

						//////////////////

						// Allow the accept socket to inherit the properties of the listen socket.
						if ( _setsockopt( context->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ( char * )&listen_socket, sizeof( SOCKET ) ) != SOCKET_ERROR )
						{
							// Create a new socket context with the inherited socket.
							new_context = UpdateCompletionPort( context->socket,
															  ( ftp_control_context->request_info.protocol == PROTOCOL_FTPS || ftp_control_context->request_info.protocol == PROTOCOL_FTPES ? true : false ),
															  ( ftp_control_context->download_info != NULL ? ftp_control_context->download_info->ssl_version : 0 ),
																true,
																false );

							// The Data context's socket has inherited the properties (and handle) of the Listen context's socket.
							// Invalidate the Listen context's socket so it doesn't shutdown/close the Data context's socket.
							context->socket = INVALID_SOCKET;

							SetDataContextValues( ftp_control_context, new_context );
						}
						else
						{
							InterlockedIncrement( &ftp_control_context->pending_operations );
							ftp_control_context->overlapped.current_operation = IO_Close;
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )ftp_control_context, ( WSAOVERLAPPED * )&ftp_control_context->overlapped );
						}

						LeaveCriticalSection( &ftp_control_context->context_cs );
					}
					else
					{
						is_ftp_data_connection = false;

						listen_socket = g_listen_socket;

						//////////////////

						// Allow the accept socket to inherit the properties of the listen socket.
						if ( _setsockopt( context->socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ( char * )&listen_socket, sizeof( SOCKET ) ) != SOCKET_ERROR )
						{
#ifdef ENABLE_LOGGING
							sockaddr_storage addr;
							_memzero( &addr, sizeof( sockaddr_storage ) );
							socklen_t addr_len = sizeof( sockaddr_storage );
							_getpeername( context->socket, ( sockaddr * )&addr, &addr_len );

							char cs_ip[ INET6_ADDRSTRLEN ];
							_memzero( cs_ip, INET6_ADDRSTRLEN );
							DWORD cs_ip_length = INET6_ADDRSTRLEN;
							if ( !_WSAAddressToStringA( ( sockaddr * )&addr, ( DWORD )addr_len, NULL, cs_ip, &cs_ip_length ) )
							{
								wchar_t cs_host[ NI_MAXHOST ];
								_memzero( cs_host, sizeof( wchar_t ) * NI_MAXHOST );
								_GetNameInfoW( ( sockaddr * )&addr, addr_len, cs_host, NI_MAXHOST, NULL, 0, 0 );
								WriteLog( LOG_INFO_CON_STATE, "Accepted client connection: %s (%S)", cs_ip, ( cs_host[ 0 ] != NULL ? cs_host : L"UNKNOWN HOST" ) );
							}
#endif
							// Create a new socket context with the inherited socket.
							new_context = UpdateCompletionPort( context->socket, cfg_server_enable_ssl, cfg_server_ssl_version, true, true );

							// The Data context's socket has inherited the properties (and handle) of the Listen context's socket.
							// Invalidate the Listen context's socket so it doesn't shutdown/close the Data context's socket.
							context->socket = INVALID_SOCKET;

							// Post another outstanding AcceptEx for our web server to listen on.
							CreateAcceptSocket( g_listen_socket, g_server_use_ipv6 );
						}
					}

					if ( new_context != NULL )
					{
						EnterCriticalSection( &cleanup_cs );

						EnableTimers( true );

						LeaveCriticalSection( &cleanup_cs );

						EnterCriticalSection( &new_context->context_cs );

						if ( new_context->cleanup == 0 )
						{
							InterlockedIncrement( &new_context->pending_operations );

							if ( is_ftp_data_connection )
							{
								if ( new_context->request_info.protocol == PROTOCOL_FTPS )	// Encrypted Data connections will always be FTPS (implicit).
								{
									// We need to perform a Connect here rather than Accept.
									// The server connects back to us, but we're responsible for initiating the handshake.

									sent = false;

									if ( g_use_openssl )
									{
										if ( new_context->_ssl_o != NULL && new_context->_ssl_o->ssl != NULL )
										{
											if ( _SSL_ctrl != NULL )
											{
												_SSL_ctrl( new_context->_ssl_o->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ( void * )new_context->request_info.host );
											}
											else if ( _SSL_set_tlsext_host_name != NULL )	// BoringSSL function.
											{
												_SSL_set_tlsext_host_name( new_context->_ssl_o->ssl, new_context->request_info.host );
											}

											if ( new_context->ftp_context != NULL &&
												 new_context->ftp_context->ftp_connection_type == FTP_CONNECTION_TYPE_CONTROL &&
												 new_context->ftp_context->_ssl_o != NULL &&
												 new_context->ftp_context->_ssl_o->ssl_session != NULL )
											{
												_SSL_set_session( new_context->_ssl_o->ssl, new_context->ftp_context->_ssl_o->ssl_session );
											}

											// This shouldn't return 1 here since we haven't sent the data in the BIO.
											nRet = _SSL_connect( new_context->_ssl_o->ssl );
											if ( nRet <= 0 )
											{
												int error = _SSL_get_error( new_context->_ssl_o->ssl, nRet );

												if ( error == SSL_ERROR_WANT_READ )
												{
													int pending = _BIO_pending( new_context->_ssl_o->wbio );
													if ( pending > 0 )
													{
														new_context->overlapped.current_operation = IO_Write;
														new_context->overlapped.next_operation = IO_OpenSSLClientHandshake;
													
														OpenSSL_WSASend( new_context, &new_context->overlapped, &new_context->wsabuf, sent );
													}
												}
											}
										}
									}
									else
									{
										new_context->overlapped.next_operation = IO_ClientHandshakeResponse;

										SSL_WSAConnect( new_context, &new_context->overlapped, new_context->request_info.host, sent );
									}
								}
								else	// Non-encrypted connections.
								{
									sent = true;

									new_context->overlapped.current_operation = IO_GetContent;

									new_context->wsabuf.buf = new_context->buffer;
									new_context->wsabuf.len = new_context->buffer_size;

									nRet = _WSARecv( new_context->socket, &new_context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )&new_context->overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										sent = false;
									}
								}

								free_context = true;	// The listen context can be freed.
							}
							else	// A connection has been made to our web server.
							{
								if ( cfg_server_enable_ssl )	// Accept incoming SSL/TLS connections.
								{
									sent = false;

									if ( g_use_openssl )
									{
										if ( new_context->_ssl_o != NULL && new_context->_ssl_o->ssl != NULL )
										{
											nRet = _SSL_accept( new_context->_ssl_o->ssl );
											if ( nRet <= 0 )
											{
												int error = _SSL_get_error( new_context->_ssl_o->ssl, nRet );

												if ( error == SSL_ERROR_WANT_READ )
												{
													sent = true;

													new_context->overlapped.current_operation = IO_OpenSSLServerHandshake;

													new_context->wsabuf.buf = new_context->buffer;
													new_context->wsabuf.len = new_context->buffer_size;

													nRet = _WSARecv( new_context->socket, &new_context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )&new_context->overlapped, NULL );
													if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
													{
														sent = false;
													}
												}
											}
										}
									}
									else
									{
										new_context->overlapped.current_operation = IO_ServerHandshakeReply;

										SSL_WSAAccept( new_context, &new_context->overlapped, sent );
									}
								}
								else	// Non-encrypted connections.
								{
									sent = true;

									new_context->overlapped.current_operation = IO_GetRequest;

									new_context->wsabuf.buf = new_context->buffer;
									new_context->wsabuf.len = new_context->buffer_size;

									nRet = _WSARecv( new_context->socket, &new_context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )&new_context->overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										sent = false;
									}
								}
							}

							if ( !sent )
							{
								new_context->overlapped.current_operation = IO_Close;
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )new_context, ( WSAOVERLAPPED * )&new_context->overlapped );
							}
						}
						else if ( new_context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
						{
							new_context->cleanup = 1;	// Auto cleanup.
						}
						else	// We've already shutdown and/or closed the connection.
						{
							InterlockedIncrement( &new_context->pending_operations );
							new_context->overlapped.current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )new_context, ( WSAOVERLAPPED * )&new_context->overlapped );
						}

						LeaveCriticalSection( &new_context->context_cs );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );

				if ( free_context )
				{
					CleanupConnection( context );
				}
			}
			break;

			case IO_Connect:
			{
				bool connection_failed = false;

				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// Allow the connect socket to inherit the properties of the previously set properties.
					// Must be done so that shutdown() will work.
					nRet = _setsockopt( context->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0 );
					if ( nRet != SOCKET_ERROR )
					{
						if ( context->request_info.protocol == PROTOCOL_HTTPS ||
							 context->request_info.protocol == PROTOCOL_FTPS )	// FTPES starts out unencrypted and is upgraded later.
						{
							char shared_protocol = ( context->download_info != NULL ? context->download_info->ssl_version : ( g_can_use_tls_1_3 ? 5 : 4 ) );
							DWORD protocol = 0;

							if ( g_use_openssl )
							{
								context->_ssl_o = ( _SSL_O * )GlobalAlloc( GPTR, sizeof( _SSL_O ) );
								if ( context->_ssl_o != NULL )
								{
									context->_ssl_o->ssl = _SSL_new( g_client_ssl_ctx[ shared_protocol ] );
									if ( context->_ssl_o->ssl != NULL )
									{
										_SSL_set_ex_data( context->_ssl_o->ssl, 0, ( void * )context );

										context->_ssl_o->rbio = _BIO_new( _BIO_s_mem() );
										context->_ssl_o->wbio = _BIO_new( _BIO_s_mem() );

										if ( context->_ssl_o->rbio != NULL && context->_ssl_o->wbio != NULL )
										{
											_SSL_set_bio( context->_ssl_o->ssl, context->_ssl_o->rbio, context->_ssl_o->wbio );

											if ( _SSL_ctrl != NULL )
											{
												_SSL_ctrl( context->_ssl_o->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ( void * )context->request_info.host );
											}
											else if ( _SSL_set_tlsext_host_name != NULL )	// BoringSSL function.
											{
												_SSL_set_tlsext_host_name( context->_ssl_o->ssl, context->request_info.host );
											}

											if ( context->ftp_context != NULL &&
												 context->ftp_context->ftp_connection_type == FTP_CONNECTION_TYPE_CONTROL &&
												 context->ftp_context->_ssl_o != NULL &&
												 context->ftp_context->_ssl_o->ssl_session != NULL )
											{
												_SSL_set_session( context->_ssl_o->ssl, context->ftp_context->_ssl_o->ssl_session );
											}
										}
										else
										{
											_BIO_free( context->_ssl_o->rbio );
											_BIO_free( context->_ssl_o->wbio );

											context->_ssl_o->rbio = NULL;
											context->_ssl_o->wbio = NULL;

											connection_failed = true;
										}
									}
									else
									{
										connection_failed = true;
									}
								}
								else
								{
									connection_failed = true;
								}
							}
							else
							{
								switch ( shared_protocol )
								{
									case 5:	protocol |= SP_PROT_TLS1_3_CLIENT;
									case 4:	protocol |= SP_PROT_TLS1_2_CLIENT;
									case 3:	protocol |= SP_PROT_TLS1_1_CLIENT;
									case 2:	protocol |= SP_PROT_TLS1_CLIENT;
									case 1:	protocol |= SP_PROT_SSL3_CLIENT;
									case 0:	{ if ( shared_protocol < 2 ) { protocol |= SP_PROT_SSL2_CLIENT; } }
								}

								_SSL_S *_ssl_s = __SSL_new( protocol, false );
								if ( _ssl_s != NULL )
								{
									_ssl_s->s = context->socket;

									context->_ssl_s = _ssl_s;
								}
								else
								{
									connection_failed = true;
								}
							}
						}
						else if ( context->request_info.protocol == PROTOCOL_SFTP )
						{
							if ( psftp_state == PSFTP_STATE_RUNNING )
							{
								char *key_info = NULL;

								SFTP_FPS_HOST_INFO tsfhi;
								tsfhi.host = context->request_info.host;
								tsfhi.port = context->request_info.port;

								DoublyLinkedList *dll_node = ( DoublyLinkedList * )dllrbt_find( g_sftp_fps_host_info, ( void * )&tsfhi, true );
								key_info = CreateKeyInfoString( dll_node );

								char *private_key_file_path = NULL;

								SFTP_KEYS_HOST_INFO tskhi;
								tskhi.username = context->download_info->auth_info.username;
								tskhi.host = context->request_info.host;
								tskhi.port = context->request_info.port;

								SFTP_KEYS_HOST_INFO *skhi = ( SFTP_KEYS_HOST_INFO * )dllrbt_find( g_sftp_keys_host_info, ( void * )&tskhi, true );
								if ( skhi != NULL && skhi->enable )
								{
									private_key_file_path = skhi->key_file_path;
								}

								context->ssh = ( SSH * )_SFTP_CreateSSHHandle( context->address_info->ai_canonname,
																			   context->download_info->auth_info.username, context->download_info->auth_info.password,
																			   key_info,
																			   private_key_file_path,
																			   &context->ssh_wsabuf );
								if ( context->ssh == NULL )
								{
									connection_failed = true;
								}

								GlobalFree( key_info );
							}
							else
							{
								connection_failed = true;
							}
						}

						bool use_socks_proxy = false;
						bool use_https_proxy = false;

						if ( context->download_info != NULL )
						{
							EnterCriticalSection( &context->download_info->di_cs );

							if ( !connection_failed )
							{
								SetStatus( context->download_info, STATUS_DOWNLOADING );

								if ( IS_STATUS( context->status, STATUS_PAUSED ) )
								{
									SetStatus( context->download_info, context->download_info->status | STATUS_PAUSED );

									context->is_paused = false;	// Set to true when last IO operation has completed.
								}

								context->status = context->download_info->status;
							}
							else
							{
								context->status = STATUS_FAILED;
							}

							LeaveCriticalSection( &context->download_info->di_cs );

							// For groups.
							if ( IS_GROUP( context->download_info ) )
							{
								EnterCriticalSection( &context->download_info->shared_info->di_cs );

								SetStatus( context->download_info->shared_info, context->download_info->status );

								LeaveCriticalSection( &context->download_info->shared_info->di_cs );
							}

							if ( cfg_enable_proxy_s ||
							   ( context->download_info->proxy_info != NULL &&
								 context->download_info->proxy_info->type == 2 ) )
							{
								use_https_proxy = true;
							}

							if ( cfg_enable_proxy_socks ||
							   ( context->download_info->proxy_info != NULL &&
							   ( context->download_info->proxy_info->type == 3 ||
								 context->download_info->proxy_info->type == 4 ) ) )
							{
								use_socks_proxy = true;
							}
						}

						if ( !connection_failed )
						{
							InterlockedIncrement( &context->pending_operations );

							// If it's an HTTPS or FTPS (not FTPES) request and we're not going through a SSL/TLS proxy, then begin the SSL/TLS handshake.
							if ( ( context->request_info.protocol == PROTOCOL_HTTPS ||
								   context->request_info.protocol == PROTOCOL_FTPS ) &&
								   !use_https_proxy && !use_socks_proxy )
							{
								sent = false;

								if ( g_use_openssl )
								{
									if ( context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
									{
										// This shouldn't return 1 here since we haven't sent the data in the BIO.
										nRet = _SSL_connect( context->_ssl_o->ssl );
										if ( nRet <= 0 )
										{
											int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

											if ( error == SSL_ERROR_WANT_READ )
											{
												int pending = _BIO_pending( context->_ssl_o->wbio );
												if ( pending > 0 )
												{
													*current_operation = IO_Write;
													*next_operation = IO_OpenSSLClientHandshake;
												
													OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
												}
											}
										}
									}
								}
								else
								{
									*next_operation = IO_ClientHandshakeResponse;

									SSL_WSAConnect( context, overlapped, context->request_info.host, sent );
								}

								if ( !sent )
								{
									InterlockedDecrement( &context->pending_operations );

									connection_failed = true;
								}
							}
							else if ( context->request_info.protocol == PROTOCOL_FTP ||
									  context->request_info.protocol == PROTOCOL_FTPES )	// FTPES starts out unencrypted and is upgraded later.
							{
								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = context->buffer_size;

								if ( use_socks_proxy )	// SOCKS5 request.
								{
									*current_operation = IO_Write;
									*next_operation = IO_SOCKSResponse;

									context->content_status = SOCKS_STATUS_REQUEST_AUTH;

									ConstructSOCKSRequest( context, 0 );

									nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										InterlockedDecrement( &context->pending_operations );

										connection_failed = true;
									}
								}
								else
								{
									*current_operation = IO_GetContent;

									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										InterlockedDecrement( &context->pending_operations );

										connection_failed = true;
									}
								}
							}
							else if ( context->request_info.protocol == PROTOCOL_SFTP )	// SFTP connection needs to receive initialization data.
							{
								*current_operation = IO_SFTPReadContent;

								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = context->buffer_size;

								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									InterlockedDecrement( &context->pending_operations );

									connection_failed = true;
								}
							}
							else	// HTTP and tunneled HTTPS requests send/recv data normally.
							{
								*current_operation = IO_Write;

								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = context->buffer_size;

								// Tunneled HTTPS requests need to send a CONNECT response before sending/receiving data.
								if ( context->request_info.protocol == PROTOCOL_HTTPS && use_https_proxy )
								{
									*next_operation = IO_GetCONNECTResponse;

									ConstructRequest( context, true );
								}
								else if ( use_socks_proxy )	// SOCKS5 request.
								{
									*next_operation = IO_SOCKSResponse;

									context->content_status = SOCKS_STATUS_REQUEST_AUTH;

									ConstructSOCKSRequest( context, 0 );
								}
								else	// HTTP request.
								{
									*next_operation = IO_GetContent;

									ConstructRequest( context, false );
								}

								nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									InterlockedDecrement( &context->pending_operations );

									connection_failed = true;
								}
							}
						}
					}
					else
					{
						connection_failed = true;
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					connection_failed = true;
				}

				if ( connection_failed )
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_OpenSSLClientHandshake:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					if ( io_size > 0 && context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
					{
						// The data we received needs to be written to the read BIO for processing by the _SSL_() functions.
						int write = _BIO_write( context->_ssl_o->rbio, context->wsabuf.buf, io_size );

						nRet = _SSL_connect( context->_ssl_o->ssl );
						if ( nRet == 1 )	// Successfully connected, now send our request.
						{
							if ( context->request_info.protocol == PROTOCOL_FTPS ||
								 context->request_info.protocol == PROTOCOL_FTPES )
							{
								*current_operation = IO_GetContent;
								*next_operation = IO_GetContent;

								if ( context->request_info.protocol == PROTOCOL_FTPES )
								{
									if ( MakeFTPResponse( context ) == FTP_CONTENT_STATUS_FAILED )
									{
										InterlockedIncrement( &context->pending_operations );
										*current_operation = IO_Shutdown;
										PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
								}
								else
								{
									sent = false;

									InterlockedIncrement( &context->pending_operations );

									context->wsabuf.buf = context->buffer;
									context->wsabuf.len = context->buffer_size;

									// Make sure we have encrypted data to send.
									int pending = _BIO_pending( context->_ssl_o->wbio );
									if ( pending > 0 )
									{
										*current_operation = IO_Write;

										OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
									}
									else
									{
										sent = true;

										nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
										if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
										{
											sent = false;
										}
									}

									if ( !sent )
									{
										*current_operation = IO_Shutdown;
										PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
								}
							}
							else
							{
								sent = false;

								InterlockedIncrement( &context->pending_operations );

								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = context->buffer_size;

								ConstructRequest( context, false );

								// Encrypts the buffer and writes it to the write BIO.
								write = _SSL_write( context->_ssl_o->ssl, context->wsabuf.buf, context->wsabuf.len );
								if ( write > 0 )
								{
									// Make sure we have encrypted data to send.
									int pending = _BIO_pending( context->_ssl_o->wbio );
									if ( pending > 0 )
									{
										*current_operation = IO_Write;
										*next_operation = IO_GetContent;

										OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
									}
								}

								if ( !sent )
								{
									*current_operation = IO_Shutdown;
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
						else
						{
							sent = false;

							InterlockedIncrement( &context->pending_operations );

							int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

							if ( error == SSL_ERROR_WANT_READ )
							{
								int pending = _BIO_pending( context->_ssl_o->wbio );
								if ( pending > 0 )
								{
									*current_operation = IO_Write;
									*next_operation = IO_OpenSSLClientHandshake;

									OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
								}
								else
								{
									sent = true;

									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										sent = false;
									}
								}
							}

							if ( !sent )
							{
								*current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
					else
					{
						*current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_OpenSSLServerHandshake:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// We could detect if the payload is an HTTP here and redirect them to an HTTPS URL.

					if ( io_size > 0 && context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
					{
						// The data we received needs to be written to the read BIO for processing by the _SSL_() functions.
						/*int write = */_BIO_write( context->_ssl_o->rbio, context->wsabuf.buf, io_size );

						nRet = _SSL_accept( context->_ssl_o->ssl );
						if ( nRet == 1 )	// Successfully connected, now get the request.
						{
							sent = false;

							InterlockedIncrement( &context->pending_operations );

							int pending = _BIO_pending( context->_ssl_o->wbio );
							if ( pending > 0 )
							{
								*current_operation = IO_Write;
								*next_operation = IO_GetRequest;

								OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
							}
							else
							{
								sent = true;

								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									sent = false;
								}
							}

							if ( !sent )
							{
								*current_operation = IO_Shutdown;
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else
						{
							sent = false;

							InterlockedIncrement( &context->pending_operations );

							int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

							if ( error == SSL_ERROR_WANT_READ )
							{
								int pending = _BIO_pending( context->_ssl_o->wbio );
								if ( pending > 0 )
								{
									*current_operation = IO_Write;
									*next_operation = IO_OpenSSLServerHandshake;

									OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
								}
								else
								{
									sent = true;

									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										sent = false;
									}
								}
							}

							if ( !sent )
							{
								*current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
					else
					{
						*current_operation = IO_Close;	// Handshake hasn't occurred so we need to close it.
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_ClientHandshakeResponse:
			case IO_ClientHandshakeReply:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					context->wsabuf.buf = context->buffer;
					context->wsabuf.len = context->buffer_size;

					InterlockedIncrement( &context->pending_operations );

					if ( *current_operation == IO_ClientHandshakeReply )
					{
						sent = false;
						scRet = SEC_E_INTERNAL_ERROR;

						if ( context->_ssl_s != NULL )
						{
							context->_ssl_s->cbIoBuffer += io_size;

							if ( context->_ssl_s->cbIoBuffer > 0 )
							{
								*current_operation = IO_ClientHandshakeResponse;
								*next_operation = IO_ClientHandshakeResponse;	// SSL_WSAConnect_Reply() might do a _WSASend() so we'll need to handle IO_ClientHandshakeResponse after it completes.

								scRet = SSL_WSAConnect_Reply( context, overlapped, sent );
							}
						}
					}
					else
					{
						*current_operation = IO_ClientHandshakeReply;
						*next_operation = IO_ClientHandshakeResponse;	// SSL_WSAConnect_Response() could call SSL_WSAConnect_Reply() which might do a _WSASend() so we'll need to handle IO_ClientHandshakeResponse after it completes.

						scRet = SSL_WSAConnect_Response( context, overlapped, sent );
					}

					if ( !sent )
					{
						InterlockedDecrement( &context->pending_operations );
					}

					if ( scRet == SEC_E_OK )
					{
						// Post request.

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = context->buffer_size;

						*next_operation = IO_GetContent;

						if ( context->_ssl_s != NULL && context->_ssl_s->acd.fRenegotiate )	// This shouldn't happen, but we'll handle it just in case.
						{
							context->_ssl_s->acd.fRenegotiate = false;	// Reset.

							InterlockedIncrement( &context->pending_operations );

							*next_operation = IO_GetContent;	// Continue where we left off before we had to renegotiate.

							SSL_WSARecv( context, overlapped, sent );
							if ( !sent )
							{
								*current_operation = IO_Shutdown;
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else if ( context->request_info.protocol == PROTOCOL_FTPS ||
								  context->request_info.protocol == PROTOCOL_FTPES )
						{
							*current_operation = IO_GetContent;

							if ( context->request_info.protocol == PROTOCOL_FTPES )
							{
								if ( MakeFTPResponse( context ) == FTP_CONTENT_STATUS_FAILED )
								{
									InterlockedIncrement( &context->pending_operations );
									*current_operation = IO_Shutdown;
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
							else
							{
								InterlockedIncrement( &context->pending_operations );

								SSL_WSARecv( context, overlapped, sent );
								if ( !sent )
								{
									*current_operation = IO_Shutdown;
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
						else	// HTTP
						{
							InterlockedIncrement( &context->pending_operations );

							*current_operation = IO_Write;

							ConstructRequest( context, false );

							SSL_WSASend( context, overlapped, &context->wsabuf, sent );
							if ( !sent )
							{
								*current_operation = IO_Shutdown;
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
					else if ( scRet != SEC_I_CONTINUE_NEEDED && scRet != SEC_E_INCOMPLETE_MESSAGE && scRet != SEC_I_INCOMPLETE_CREDENTIALS )
					{
						// Have seen SEC_E_ILLEGAL_MESSAGE (for a bad target name in InitializeSecurityContext), SEC_E_BUFFER_TOO_SMALL, and SEC_E_MESSAGE_ALTERED.

						InterlockedIncrement( &context->pending_operations );
						*current_operation = IO_Close;
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_ServerHandshakeResponse:
			case IO_ServerHandshakeReply:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// We process data from the client and write our reply.
					InterlockedIncrement( &context->pending_operations );

					if ( *current_operation == IO_ServerHandshakeReply )
					{
						sent = false;
						scRet = SEC_E_INTERNAL_ERROR;

						if ( context->_ssl_s != NULL )
						{
							context->_ssl_s->cbIoBuffer += io_size;

							*current_operation = IO_ServerHandshakeResponse;
							*next_operation = IO_ServerHandshakeResponse;

							scRet = SSL_WSAAccept_Reply( context, overlapped, sent );
						}
					}
					else
					{
						*current_operation = IO_ServerHandshakeReply;

						scRet = SSL_WSAAccept_Response( context, overlapped, sent );
					}

					if ( !sent )
					{
						InterlockedDecrement( &context->pending_operations );
					}

					if ( scRet == SEC_E_OK )	// If true, then no send was made.
					{
						sent = false;

						InterlockedIncrement( &context->pending_operations );

						*current_operation = IO_GetRequest;

						if ( context->_ssl_s != NULL )
						{
							if ( context->_ssl_s->cbIoBuffer > 0 )
							{
								sent = true;

								// The request was sent with the handshake.
								PostQueuedCompletionStatus( hIOCP, context->_ssl_s->cbIoBuffer, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
							else
							{
								context->wsabuf.buf = context->buffer;
								context->wsabuf.len = context->buffer_size;

								SSL_WSARecv( context, overlapped, sent );
							}
						}

						if ( !sent )
						{
							*current_operation = IO_Shutdown;
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( scRet == SEC_E_INCOMPLETE_MESSAGE && *current_operation == IO_ServerHandshakeResponse )
					{
						// An SEC_E_INCOMPLETE_MESSAGE after SSL_WSAAccept_Reply can indicate that it doesn't support SSL/TLS, but sent the request as plaintext.

						/*InterlockedIncrement( &context->pending_operations );

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = context->buffer_size;

						DWORD bytes_read = min( context->buffer_size, context->_ssl_s->cbIoBuffer );

						_memcpy_s( context->wsabuf.buf, context->buffer_size, context->_ssl_s->pbIoBuffer, bytes_read );
						*current_operation = IO_GetRequest;

						__SSL_free( context->_ssl_s );
						context->_ssl_s = NULL;

						PostQueuedCompletionStatus( hIOCP, bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );*/

						*current_operation = IO_Write;
						*next_operation = IO_Close;	// This is closed because the SSL connection was never established. An SSL shutdown would just fail.

						InterlockedIncrement( &context->pending_operations );

						context->wsabuf.buf = context->buffer;

						context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
							"HTTP/1.1 301 Moved Permanently\r\n" \
							"Location: https://%s:%hu/\r\n"
							"Content-Type: text/html\r\n" \
							"Content-Length: 120\r\n" \
							"Connection: close\r\n\r\n" \
							"<!DOCTYPE html><html><head><title>301 Moved Permanently</title></head><body><h1>301 Moved Permanently</h1></body></html>", g_server_domain, g_server_port );

						// We do a regular WSASend here since the connection was not encrypted.
						nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							*current_operation = IO_Close;
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( scRet != SEC_I_CONTINUE_NEEDED && scRet != SEC_E_INCOMPLETE_MESSAGE && scRet != SEC_I_INCOMPLETE_CREDENTIALS )	// Stop handshake and close the connection.
					{
						InterlockedIncrement( &context->pending_operations );
						*current_operation = IO_Close;
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_SFTPReadContent:
			case IO_SFTPWriteContent:
			case IO_SFTPResumeInit:
			case IO_SFTPResumeReadContent:
			//case IO_SFTPCleanup:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					SFTP_HandleContent( context, *current_operation, io_size );
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_GetCONNECTResponse:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					context->current_bytes_read = io_size + ( DWORD )( context->wsabuf.buf - context->buffer );

					context->wsabuf.buf = context->buffer;
					context->wsabuf.len = context->buffer_size;

					// Ensure it's NULL terminated so our parsers (ParseHTTPHeader(), etc.) don't read beyond it.
					context->wsabuf.buf[ context->current_bytes_read ] = 0;	// Sanity.

					char content_status = ParseHTTPHeader( context, context->wsabuf.buf, context->current_bytes_read );

					if ( content_status == CONTENT_STATUS_READ_MORE_HEADER )	// Request more header data.
					{
						InterlockedIncrement( &context->pending_operations );

						// wsabuf will be offset in ParseHTTPHeader() to handle more data.
						nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							*current_operation = IO_Close;
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == CONTENT_STATUS_FAILED )
					{
						context->status = STATUS_FAILED;

						InterlockedIncrement( &context->pending_operations );
						*current_operation = IO_Close;	// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
					else// if ( content_status == CONTENT_STATUS_GET_CONTENT );
					{
						// Any 2XX status is valid.
						if ( context->header_info.http_status >= 200 && context->header_info.http_status <= 299 )
						{
							context->got_filename = 0;
							context->got_last_modified = 0;
							context->show_file_size_prompt = false;

							context->header_info.chunk_length = 0;
							context->header_info.end_of_header = NULL;
							context->header_info.http_status = 0;
							context->header_info.connection = CONNECTION_NONE;
							context->header_info.content_encoding = CONTENT_ENCODING_NONE;
							context->header_info.chunked_transfer = false;
							//context->header_info.etag = false;
							context->header_info.got_chunk_start = false;
							context->header_info.got_chunk_terminator = false;

							context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 2XX request).

							// Do not reset the other range_info values.

							//

							context->content_status = CONTENT_STATUS_NONE;

							InterlockedIncrement( &context->pending_operations );

							sent = false;

							if ( g_use_openssl )
							{
								if ( context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
								{
									// This shouldn't return 1 here since we haven't sent the data in the BIO.
									nRet = _SSL_connect( context->_ssl_o->ssl );
									if ( nRet <= 0 )
									{
										int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

										if ( error == SSL_ERROR_WANT_READ )
										{
											int pending = _BIO_pending( context->_ssl_o->wbio );
											if ( pending > 0 )
											{
												*current_operation = IO_Write;
												*next_operation = IO_OpenSSLClientHandshake;
											
												OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
											}
										}
									}
								}
							}
							else
							{
								*next_operation = IO_ClientHandshakeResponse;

								SSL_WSAConnect( context, overlapped, context->request_info.host, sent );
							}

							if ( !sent )
							{
								*current_operation = IO_Close; // The tunneled HTTPS connection should be closed and not shutdown.
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else	// Proxy can't/won't tunnel SSL/TLS connections, or authentication is required.
						{
							bool skip_close = false;

							// Authentication is required.
							if ( context->header_info.http_status == 407 )
							{
								if ( context->header_info.proxy_digest_info != NULL &&
								   ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_BASIC || context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_DIGEST ) &&
									 context->header_info.proxy_digest_info->nc == 0 )
								{
									bool use_keep_alive_connection = false;

									// If we have a keep-alive connection and were sent all of the data,
									// then we can reuse the connection and not have to flush any remnant data from the buffer.
									if ( context->header_info.connection == CONNECTION_KEEP_ALIVE )
									{
										char *response_buffer = context->header_info.end_of_header;
										int response_buffer_length = context->current_bytes_read - ( DWORD )( context->header_info.end_of_header - context->wsabuf.buf );

										// Look for a chunk terminator.
										if ( context->header_info.chunked_transfer )
										{
											if ( ( response_buffer_length >= 5 ) && ( _memcmp( response_buffer + ( response_buffer_length - 5 ), "0\r\n\r\n", 5 ) != 0 ) )
											{
												use_keep_alive_connection = true;
											}
										}
										else if ( response_buffer_length >= context->header_info.range_info->content_length )	// See if the response data length is the same as the content length.
										{
											use_keep_alive_connection = true;
										}
									}

									context->header_info.connection = ( use_keep_alive_connection ? CONNECTION_KEEP_ALIVE : CONNECTION_CLOSE );

									if ( MakeRequest( context, IO_GetCONNECTResponse, true ) == CONTENT_STATUS_FAILED )
									{
										context->status = STATUS_FAILED;
									}
									else	// Request was sent, don't close the connection below.
									{
										skip_close = true;
									}
								}
								else	// Exhausted the nonce count.
								{
									context->status = STATUS_PROXY_AUTH_REQUIRED;
								}
							}
							else	// Unhandled status response.
							{
								context->status = STATUS_FAILED;
							}

							if ( !skip_close )
							{
								InterlockedIncrement( &context->pending_operations );
								*current_operation = IO_Close;	// We don't need to shutdown the SSL/TLS connection since it will not have been established yet.
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_SOCKSResponse:
			{
				char connection_status = 0;	// 0 = continue, 1 = fail, 2 = exit

				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					InterlockedIncrement( &context->pending_operations );

					context->current_bytes_read = io_size + ( DWORD )( context->wsabuf.buf - context->buffer );

					context->wsabuf.buf = context->buffer;
					context->wsabuf.len = context->buffer_size;

					context->wsabuf.buf[ context->current_bytes_read ] = 0;	// Sanity.

					if ( context->content_status == SOCKS_STATUS_REQUEST_AUTH )
					{
						if ( context->current_bytes_read < 2 )	// Request more data.
						{
							context->wsabuf.buf += context->current_bytes_read;
							context->wsabuf.len -= context->current_bytes_read;

							// wsabuf will be offset in ParseHTTPHeader() to handle more data.
							nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
							if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
							{
								connection_status = 1;	// Failed.
							}
						}
						else
						{
							if ( context->wsabuf.buf[ 1 ] == 0x5A )	// SOCKS4 - request granted.
							{
								context->content_status = SOCKS_STATUS_HANDLE_CONNECTION;
							}
							else if ( context->wsabuf.buf[ 1 ] == 0x00 )	// SOCKS5 - no authentication required.
							{
								context->content_status = SOCKS_STATUS_REQUEST_CONNECTION;
							}
							else if ( context->wsabuf.buf[ 1 ] == 0x02 )	// SOCKS5 - username and password authentication required.
							{
								if ( cfg_use_authentication_socks )
								{
									context->content_status = SOCKS_STATUS_AUTH_SENT;

									ConstructSOCKSRequest( context, 2 );

									*current_operation = IO_Write;

									nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										connection_status = 1;	// Failed.
									}
									else
									{
										connection_status = 2;	// Exit the case.
									}
								}
								else	// Server wants us to send it, but we don't have it enabled.
								{
									context->status = STATUS_PROXY_AUTH_REQUIRED;

									connection_status = 1;	// Failed.
								}
							}
							else	// Bad request, or unsupported authentication method.
							{
								connection_status = 1;	// Failed.
							}
						}
					}
					else if ( context->content_status == SOCKS_STATUS_AUTH_SENT )
					{
						if ( context->wsabuf.buf[ 1 ] == 0x00 )	// Username and password accepted.
						{
							context->content_status = SOCKS_STATUS_REQUEST_CONNECTION;
						}
						else	// We sent the username and password, but it was rejected.
						{
							context->status = STATUS_PROXY_AUTH_REQUIRED;

							connection_status = 1;	// Failed.
						}
					}

					// No problems, continue with our request.
					if ( connection_status == 0 )
					{
						if ( context->content_status == SOCKS_STATUS_REQUEST_CONNECTION )
						{
							*current_operation = IO_Write;

							context->content_status = SOCKS_STATUS_HANDLE_CONNECTION;

							ConstructSOCKSRequest( context, 1 );

							nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
							if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
							{
								connection_status = 1;	// Failed.
							}
						}
						else if ( context->content_status == SOCKS_STATUS_HANDLE_CONNECTION )
						{
							if ( context->ftp_connection_type == FTP_CONNECTION_TYPE_DATA )
							{
								context->content_status = CONTENT_STATUS_GET_CONTENT;	// This is the data connection and we want to start downloading from it.
							}
							else
							{
								context->content_status = CONTENT_STATUS_NONE;	// Reset.
							}

							if ( context->request_info.protocol == PROTOCOL_HTTPS ||
								 context->request_info.protocol == PROTOCOL_FTPS )	// FTPES starts out unencrypted and is upgraded later.
							{
								sent = false;

								if ( g_use_openssl )
								{
									if ( context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
									{
										// This shouldn't return 1 here since we haven't sent the data in the BIO.
										nRet = _SSL_connect( context->_ssl_o->ssl );
										if ( nRet <= 0 )
										{
											int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

											if ( error == SSL_ERROR_WANT_READ )
											{
												int pending = _BIO_pending( context->_ssl_o->wbio );
												if ( pending > 0 )
												{
													*current_operation = IO_Write;
													*next_operation = IO_OpenSSLClientHandshake;
												
													OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
												}
											}
										}
									}
								}
								else
								{
									*next_operation = IO_ClientHandshakeResponse;

									SSL_WSAConnect( context, overlapped, context->request_info.host, sent );
								}

								if ( !sent )
								{
									connection_status = 1;	// Failed.
								}
							}
							else
							{
								*next_operation = IO_GetContent;

								if ( context->request_info.protocol == PROTOCOL_FTP ||
									 context->request_info.protocol == PROTOCOL_FTPES )	// FTPES starts out unencrypted and is upgraded later.
								{
									*current_operation = IO_GetContent;

									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										*current_operation = IO_Close;
										PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
								}
								else	// HTTP
								{
									*current_operation = IO_Write;

									ConstructRequest( context, false );

									nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										connection_status = 1;
									}
								}
							}
						}
						else
						{
							context->status = STATUS_FAILED;

							connection_status = 1;	// Failed.
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					connection_status = 1;	// Failed.

					InterlockedIncrement( &context->pending_operations );
				}

				// If something failed.
				if ( connection_status == 1 )
				{
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_GetContent:
			case IO_ResumeGetContent:
			case IO_GetRequest:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					char content_status = CONTENT_STATUS_FAILED;

					DWORD bytes_decrypted = io_size;

					//if ( *current_operation == IO_GetContent || *current_operation == IO_GetRequest )
					if ( *current_operation != IO_ResumeGetContent )
					{
						context->current_bytes_read = 0;

						if ( context->_ssl_s != NULL )
						{
							// We'll need to decrypt any remaining undecrypted data as well as copy the decrypted data to our wsabuf.
							if ( context->_ssl_s->continue_decrypt )
							{
								bytes_decrypted = context->_ssl_s->cbIoBuffer;
							}

							scRet = DecryptRecv( context, bytes_decrypted );
						}
						else if ( context->_ssl_o != NULL )
						{
							content_status = OpenSSL_DecryptRecv( context, bytes_decrypted );
						}
					}
					else if ( *next_operation == IO_SparseFileAllocate )
					{
						*next_operation = IO_GetRequest;
						bytes_decrypted = 1;	// Allows us into the block below, but isn't used.
					}

					if ( bytes_decrypted > 0 )
					{
						//if ( *current_operation == IO_GetContent || *current_operation == IO_GetRequest )
						if ( *current_operation != IO_ResumeGetContent )
						{
							context->current_bytes_read = bytes_decrypted + ( DWORD )( context->wsabuf.buf - context->buffer );

							context->wsabuf.buf = context->buffer;
							context->wsabuf.len = context->buffer_size;

							// Ensure it's NULL terminated so our parsers (ParseHTTPHeader(), etc.) don't read beyond it.
							context->wsabuf.buf[ context->current_bytes_read ] = 0;	// Sanity.
						}
						else
						{
							*current_operation = IO_GetContent;
						}

						if ( *current_operation == IO_GetContent )
						{
							if ( context->request_info.protocol == PROTOCOL_FTP ||
								 context->request_info.protocol == PROTOCOL_FTPS ||
								 context->request_info.protocol == PROTOCOL_FTPES )
							{
								content_status = GetFTPResponseContent( context, context->wsabuf.buf, context->current_bytes_read );
							}
							else
							{
								content_status = GetHTTPResponseContent( context, context->wsabuf.buf, context->current_bytes_read );
							}
						}
						else// if ( *current_operation == IO_GetRequest )
						{
							if ( context->request_info.protocol != PROTOCOL_FTP &&
								 context->request_info.protocol != PROTOCOL_FTPS &&
								 context->request_info.protocol != PROTOCOL_FTPES )
							{
								content_status = GetHTTPRequestContent( context, context->wsabuf.buf, context->current_bytes_read );
							}
						}
					}
					else if ( context->_ssl_s != NULL )	// Handle responses from DecryptRecv().
					{
						if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
						{
							InterlockedIncrement( &context->pending_operations );

							//context->wsabuf.buf += bytes_decrypted;
							//context->wsabuf.len -= bytes_decrypted;

							SSL_WSARecv( context, overlapped, sent );
							if ( !sent )
							{
								InterlockedDecrement( &context->pending_operations );
							}
							else
							{
								content_status = CONTENT_STATUS_NONE;
							}
						}
						else if ( scRet == SEC_I_RENEGOTIATE )	// _WSASend() was called in SSL_WSAConnect_Reply() which in turn was called in DecryptRecv(). IO_ClientHandshakeResponse will handle the request. All of this is unlikely to happen.
						{
							content_status = CONTENT_STATUS_NONE;
						}

						// SEC_I_CONTEXT_EXPIRED may occur here.
					}
					else if ( context->status == STATUS_ALLOCATING_FILE )
					{
						context->status = STATUS_FILE_IO_ERROR;
					}

					if ( content_status == CONTENT_STATUS_FAILED )
					{
						InterlockedIncrement( &context->pending_operations );
						*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
						PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
					}
					else if ( content_status == CONTENT_STATUS_HANDLE_RESPONSE )
					{
						context->content_status = CONTENT_STATUS_GET_CONTENT;

						if ( MakeRangeRequest( context ) == CONTENT_STATUS_FAILED )
						{
							InterlockedIncrement( &context->pending_operations );
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == CONTENT_STATUS_HANDLE_REQUEST )
					{
						context->content_status = CONTENT_STATUS_GET_CONTENT;

						if ( MakeResponse( context ) == CONTENT_STATUS_FAILED )
						{
							InterlockedIncrement( &context->pending_operations );
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == FTP_CONTENT_STATUS_HANDLE_REQUEST )
					{
						if ( MakeFTPResponse( context ) == FTP_CONTENT_STATUS_FAILED )
						{
							InterlockedIncrement( &context->pending_operations );
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else if ( content_status == CONTENT_STATUS_READ_MORE_CONTENT || content_status == CONTENT_STATUS_READ_MORE_HEADER ) // Read more header information, or continue to read more content. Do not reset context->wsabuf since it may have been offset to handle partial data.
					{
						sent = true;

						InterlockedIncrement( &context->pending_operations );

						//*current_operation = IO_GetContent;

						if ( context->_ssl_s != NULL )
						{
							if ( context->_ssl_s->continue_decrypt )
							{
								// We need to post a non-zero status to avoid our code shutting down the connection.
								// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
								PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
							else
							{
								SSL_WSARecv( context, overlapped, sent );
							}
						}
						else if ( context->_ssl_o != NULL )
						{
							// There's more data to be read with _SSL_read().
							if ( context->_ssl_o->continue_decrypt )
							{
								// We need to post a non-zero status to avoid our code shutting down the connection.
								// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
								PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
							else
							{
								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									sent = false;
								}
							}
						}
						else
						{
							nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
							if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
							{
								sent = false;
							}
						}

						if ( !sent )
						{
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_SparseFileAllocate:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					FILE_ZERO_DATA_INFORMATION fzdi;
					fzdi.FileOffset.QuadPart = 0;
					fzdi.BeyondFinalZero.QuadPart = context->header_info.range_info->content_length;	// This will be greater than 0.

					InterlockedIncrement( &context->pending_operations );

					*current_operation = *next_operation;		// Will either be IO_ResumeGetContent or IO_SFTPResumeReadContent.
					*next_operation = IO_SparseFileAllocate;	// Lets IO_ResumeGetContent know that we want to resume even though io_size will be 0 after the DeviceIoControl call.

					EnterCriticalSection( &context->download_info->shared_info->di_cs );
					// We don't need to write a byte at the end of the file to allocate all the space.
					BOOL bRet = DeviceIoControl( context->download_info->shared_info->hFile, FSCTL_SET_ZERO_DATA, &fzdi, sizeof( FILE_ZERO_DATA_INFORMATION ), NULL, 0, NULL, ( WSAOVERLAPPED * )overlapped );
					if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
					{
						if ( GetLastError() == ERROR_DISK_FULL )
						{
							SetStatus( context->download_info, STATUS_INSUFFICIENT_DISK_SPACE );
							context->status = STATUS_INSUFFICIENT_DISK_SPACE;
						}
						else
						{
							SetStatus( context->download_info, STATUS_FILE_IO_ERROR );
							context->status = STATUS_FILE_IO_ERROR;
						}

						LeaveCriticalSection( &context->download_info->shared_info->di_cs );

						context->is_allocated = false;

						if ( context->ssh != NULL )
						{
							InterlockedDecrement( &context->pending_operations );

							context->overlapped_close.current_operation = IO_SFTPCleanup;

							*current_operation = IO_SFTPReadContent;

							SFTP_HandleContent( context, IO_SFTPReadContent, 0 );	// Causes the download or backend to get closed.
						}
						else
						{
							*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
					else
					{
						LeaveCriticalSection( &context->download_info->shared_info->di_cs );
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_WriteFile:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					if ( IS_GROUP( context->download_info ) )
					{
						EnterCriticalSection( &context->download_info->shared_info->di_cs );
						context->download_info->shared_info->downloaded += io_size;				// The total amount of data (decoded) that was saved/simulated.
						LeaveCriticalSection( &context->download_info->shared_info->di_cs );
					}

					EnterCriticalSection( &context->download_info->di_cs );
					context->download_info->downloaded += io_size;				// The total amount of data (decoded) that was saved/simulated.
					LeaveCriticalSection( &context->download_info->di_cs );

					EnterCriticalSection( &session_totals_cs );
					g_session_total_downloaded += io_size;
					cfg_total_downloaded += io_size;
					LeaveCriticalSection( &session_totals_cs );

					context->header_info.range_info->file_write_offset += io_size;	// The size of the non-encoded/decoded data that we're writing to the file.

					// Make sure we've written everything before we do anything else.
					if ( io_size < context->write_wsabuf.len )
					{
						InterlockedIncrement( &context->pending_operations );

						context->write_wsabuf.buf += io_size;
						context->write_wsabuf.len -= io_size;

						EnterCriticalSection( &context->download_info->shared_info->di_cs );
						BOOL bRet = WriteFile( context->download_info->shared_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( WSAOVERLAPPED * )overlapped );
						if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
						{
							LeaveCriticalSection( &context->download_info->shared_info->di_cs );

							if ( context->ssh != NULL )
							{
								InterlockedDecrement( &context->pending_operations );

								context->overlapped_close.current_operation = IO_SFTPCleanup;

								*current_operation = IO_SFTPReadContent;

								SFTP_HandleContent( context, IO_SFTPReadContent, 0 );	// Causes the download or backend to get closed.
							}
							else
							{
								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
						else
						{
							LeaveCriticalSection( &context->download_info->shared_info->di_cs );
						}
					}
					else
					{
						// We had set the overlapped structure for file operations, but now we need to reset it for socket operations.
						_memzero( &overlapped->overlapped, sizeof( WSAOVERLAPPED ) );

						//

						context->header_info.range_info->content_offset += context->content_offset;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						context->content_offset = 0;

						if ( context->ssh != NULL )
						{
							_SFTP_FreeDownloadData( context->write_wsabuf.buf );
							context->write_wsabuf.buf = NULL;
							context->write_wsabuf.len = 0;

							context->wsabuf.buf = context->buffer;
							context->wsabuf.len = context->buffer_size;

							context->content_status = SFTP_CONTENT_STATUS_NONE;

							INT op_ret = SFTP_WriteContent( context );
							if ( op_ret != SFTP_CONTENT_STATUS_READ_MORE_CONTENT )
							{
								op_ret = _SFTP_IsDownloadDone( context->ssh );
								if ( op_ret != 0 )	// Transfer is complete.
								{
									_SFTP_SetStatus( context->ssh, _SFTP_GetStatus( context->ssh ) | SSH_STATUS_USER_CLEANUP );
									_SFTP_DownloadClose( context->ssh );
								}
								else
								{
									if ( context->overlapped_close.current_operation == IO_SFTPCleanup )
									{
										_SFTP_SetStatus( context->ssh, _SFTP_GetStatus( context->ssh ) | SSH_STATUS_USER_CLEANUP );
										_SFTP_DownloadClose( context->ssh );
									}
									else
									{
										op_ret = _SFTP_DownloadQueue( context->ssh );
										if ( op_ret != 0 )
										{
											_SFTP_SetStatus( context->ssh, _SFTP_GetStatus( context->ssh ) | SSH_STATUS_USER_CLEANUP );
											_SFTP_DownloadClose( context->ssh );
										}
									}
								}

								context->overlapped.current_operation = IO_SFTPWriteContent;

								SFTP_HandleContent( context, IO_SFTPWriteContent, 0 );
							}
						}
						else
						{
							char content_status = context->content_status;

							// Reset so we don't try to process the header again.
							context->content_status = CONTENT_STATUS_GET_CONTENT;

							if ( context->header_info.chunked_transfer )
							{
								if ( ( context->parts == 1 && context->header_info.connection == CONNECTION_KEEP_ALIVE && context->header_info.got_chunk_terminator ) ||
								   ( ( context->parts > 1 || ( context->download_info != NULL && IS_GROUP( context->download_info ) ) ) &&
									 ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
								{
									InterlockedIncrement( &context->pending_operations );
									*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

									content_status = CONTENT_STATUS_NONE;
								}
							}
							else
							{
								// We need to force the keep-alive connections closed since the server will just keep it open after we've gotten all the data.
								if ( ( ( ( context->request_info.protocol == PROTOCOL_FTP ||
										   context->request_info.protocol == PROTOCOL_FTPS ||
										   context->request_info.protocol == PROTOCOL_FTPES ) && ( context->parts > 1 || ( context->download_info != NULL && IS_GROUP( context->download_info ) ) ) ) ||
									   context->header_info.connection == CONNECTION_KEEP_ALIVE ) &&
									 ( context->header_info.range_info->content_length == 0 ||
									 ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
								{
									InterlockedIncrement( &context->pending_operations );
									*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

									content_status = CONTENT_STATUS_NONE;
								}
							}

							//

							if ( content_status == CONTENT_STATUS_FAILED )
							{
								InterlockedIncrement( &context->pending_operations );
								*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
							else if ( content_status == CONTENT_STATUS_HANDLE_RESPONSE )
							{
								if ( MakeRangeRequest( context ) == CONTENT_STATUS_FAILED )
								{
									InterlockedIncrement( &context->pending_operations );
									*current_operation = ( use_ssl ? IO_Shutdown : IO_Close );
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
							else if ( content_status == CONTENT_STATUS_READ_MORE_CONTENT || content_status == CONTENT_STATUS_READ_MORE_HEADER ) // Read more header information, or continue to read more content. Do not reset context->wsabuf since it may have been offset to handle partial data.
							{
								InterlockedIncrement( &context->pending_operations );

								*current_operation = IO_GetContent;

								if ( context->_ssl_s != NULL )
								{
									if ( context->_ssl_s->continue_decrypt )
									{
										// We need to post a non-zero status to avoid our code shutting down the connection.
										// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
										PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
									else
									{
										SSL_WSARecv( context, overlapped, sent );
										if ( !sent )
										{
											*current_operation = IO_Shutdown;
											PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
										}
									}
								}
								else if ( context->_ssl_o != NULL )
								{
									// There's more data to be read with _SSL_read().
									if ( context->_ssl_o->continue_decrypt )
									{
										// We need to post a non-zero status to avoid our code shutting down the connection.
										// We'll use context->current_bytes_read for that, but it can be anything that's not zero.
										PostQueuedCompletionStatus( hIOCP, context->current_bytes_read, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
									else
									{
										nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
										if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
										{
											*current_operation = IO_Shutdown;
											PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
										}
									}
								}
								else
								{
									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										*current_operation = IO_Close;
										PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
									}
								}
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_Write:
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 || context->cleanup >= 10 )
				{
					if ( context->cleanup >= 10 )
					{
						context->cleanup -= 10;
					}

					InterlockedIncrement( &context->pending_operations );

					// Make sure we've sent everything before we do anything else.
					if ( io_size < context->wsabuf.len )
					{
						context->wsabuf.buf += io_size;
						context->wsabuf.len -= io_size;

						// We do a regular WSASend here since that's what we last did in SSL_WSASend/OpenSSL_WSASend.
						nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							if ( context->ssh != NULL )
							{
								InterlockedDecrement( &context->pending_operations );

								context->overlapped_close.current_operation = IO_SFTPCleanup;

								*current_operation = IO_SFTPReadContent;

								SFTP_HandleContent( context, IO_SFTPReadContent, 0 );	// Causes the download or backend to get closed.
							}
							else
							{
								*current_operation = IO_Close;
								PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
							}
						}
					}
					else	// All the data that we wanted to send has been sent. Post our next operation.
					{
						*current_operation = *next_operation;

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = context->buffer_size;

						if ( *current_operation == IO_SFTPWriteContent )
						{
							InterlockedDecrement( &context->pending_operations );

							SFTP_HandleContent( context, IO_SFTPWriteContent, 0 );
						}
						else if ( *current_operation == IO_ServerHandshakeResponse ||
								  *current_operation == IO_ClientHandshakeResponse ||
								  *current_operation == IO_Shutdown ||
								  *current_operation == IO_Close )
						{
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
						else	// Read more data.
						{
							if ( *current_operation != IO_GetCONNECTResponse &&
								 *current_operation != IO_SOCKSResponse &&
								  use_ssl )
							{
								if ( context->_ssl_s != NULL )
								{
									SSL_WSARecv( context, overlapped, sent );
								}
								else// if ( context->_ssl_o != NULL )
								{
									sent = true;

									nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
									if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
									{
										sent = false;
									}
								}

								if ( !sent )
								{
									if ( *current_operation == IO_OpenSSLClientHandshake ||
										 *current_operation == IO_OpenSSLServerHandshake )
									{
										*current_operation = IO_Close;	// The handshake will not have completed so we should close it.
									}
									else
									{
										*current_operation = IO_Shutdown;
									}
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
							else
							{
								nRet = _WSARecv( context->socket, &context->wsabuf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
								if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
								{
									*current_operation = IO_Close;
									PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
								}
							}
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_KeepAlive:	// For FTP keep-alive requests.
			{
				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 )
				{
					// Make sure we've sent everything before we do anything else.
					if ( io_size < context->keep_alive_wsabuf.len )
					{
						context->keep_alive_wsabuf.buf += io_size;
						context->keep_alive_wsabuf.len -= io_size;

						InterlockedIncrement( &context->pending_operations );

						// We do a regular WSASend here since that's what we last did in SSL_WSASend.
						nRet = _WSASend( context->socket, &context->keep_alive_wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							*current_operation = IO_Close;
							PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
						}
					}
				}
				else if ( context->cleanup == 2 )	// If we've forced the cleanup, then allow it to continue its steps.
				{
					context->cleanup = 1;	// Auto cleanup.
				}
				else	// We've already shutdown and/or closed the connection.
				{
					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}

				LeaveCriticalSection( &context->context_cs );
			}
			break;

			case IO_Shutdown:
			{
				bool fall_through = true;

				EnterCriticalSection( &context->context_cs );

				if ( context->cleanup == 0 || context->cleanup == 2 )
				{
					context->cleanup += 10;	// Allow IO_Write to continue to process.

					sent = false;

					InterlockedIncrement( &context->pending_operations );

					if ( context->_ssl_s != NULL )
					{
						*next_operation = IO_Close;

						context->wsabuf.buf = context->buffer;
						context->wsabuf.len = context->buffer_size;

						SSL_WSAShutdown( context, overlapped, sent );
					}
					else if ( context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
					{
						nRet = _SSL_shutdown( context->_ssl_o->ssl );
						if ( nRet <= 0 )
						{
							int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

							if ( error == SSL_ERROR_SYSCALL )
							{
								int pending = _BIO_pending( context->_ssl_o->wbio );
								if ( pending > 0 )
								{
									*current_operation = IO_Write;
									*next_operation = IO_Close;	// _SSL_shutdown() must not be called again if _SSL_get_error() is SSL_ERROR_SYSCALL or SSL_ERROR_SSL.

									OpenSSL_WSASend( context, overlapped, &context->wsabuf, sent );
								}
							}
						}
					}

					// We'll fall through the IO_Shutdown to IO_Close.
					if ( !sent )
					{
						context->cleanup -= 10;

						InterlockedDecrement( &context->pending_operations );
						*current_operation = IO_Close;
					}
					else	// The shutdown sent data. IO_Close will be called in IO_Write.
					{
						fall_through = false;
					}
				}
				/*else	// We've already shutdown and/or closed the connection.
				{
					fall_through = false;

					InterlockedIncrement( &context->pending_operations );
					*current_operation = IO_Close;
					PostQueuedCompletionStatus( hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
				}*/

				LeaveCriticalSection( &context->context_cs );

				if ( !fall_through )
				{
					break;
				}
			}

			case IO_Close:
			{
				bool cleanup = true;

				EnterCriticalSection( &context->context_cs );

				if ( context->pending_operations > 0 )
				{
					cleanup = false;

					context->cleanup = 1;	// Auto cleanup.

					if ( context->socket != INVALID_SOCKET )
					{
						SOCKET s = context->socket;
						context->socket = INVALID_SOCKET;
						_shutdown( s, SD_BOTH );
						_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
					}
				}

				LeaveCriticalSection( &context->context_cs );

				if ( cleanup )
				{
					CleanupConnection( context );
				}
			}
			break;
		}
	}

	_ExitThread( 0 );
	//return 0;
}

SOCKET_CONTEXT *CreateSocketContext()
{
	SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )GlobalAlloc( GPTR, sizeof( SOCKET_CONTEXT ) );
	if ( context != NULL )
	{
		context->buffer = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( BUFFER_SIZE + 1 ) );
		if ( context->buffer != NULL )
		{
			context->buffer_size = BUFFER_SIZE;

			context->wsabuf.buf = context->buffer;
			context->wsabuf.len = context->buffer_size;

			context->socket = INVALID_SOCKET;
			context->listen_socket = INVALID_SOCKET;

			context->overlapped.context = context;
			context->overlapped_close.context = context;
			context->overlapped_keep_alive.context = context;

			InitializeCriticalSection( &context->context_cs );
		}
		else
		{
			GlobalFree( context );
			context = NULL;
		}
	}

	return context;
}

bool CreateConnection( SOCKET_CONTEXT *context, char *host, unsigned short port )
{
	if ( context == NULL || host == NULL )
	{
		return false;
	}

	int nRet = 0;

	struct addrinfoW hints;

	bool use_ipv6 = false;

	wchar_t *whost = NULL, *t_whost = NULL;
	wchar_t wcs_ip[ 16 ];
	wchar_t wport[ 6 ];

	bool _enable_proxy_socks;
	unsigned char _socks_type = SOCKS_TYPE_V4;
	bool _resolve_domain_names = false;

	PROXY_INFO *pi = NULL;

	if ( context->download_info != NULL &&
		 context->download_info->proxy_info != NULL &&
		 context->download_info->proxy_info->type != 0 )
	{
		pi = context->download_info->proxy_info;

		if ( pi->type == 3 )
		{
			_enable_proxy_socks = true;
			_socks_type = SOCKS_TYPE_V4;
			_resolve_domain_names = pi->resolve_domain_names;
		}
		else if ( pi->type == 4 )
		{
			_enable_proxy_socks = true;
			_socks_type = SOCKS_TYPE_V5;
			_resolve_domain_names = cfg_resolve_domain_names;
		}
		else
		{
			_enable_proxy_socks = false;
		}
	}
	else
	{
		_enable_proxy_socks = cfg_enable_proxy_socks;
		_socks_type = cfg_socks_type;
		_resolve_domain_names = cfg_resolve_domain_names;
	}

	if ( context->address_info == NULL )
	{
		// Resolve the remote host.
		_memzero( &hints, sizeof( addrinfoW ) );
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_IP;
		hints.ai_flags = AI_CANONNAME;

		bool use_supplied_params = false;

		wchar_t *_hostname = NULL;
		wchar_t *_punycode_hostname = NULL;
		unsigned long _ip_address = 0;

		unsigned char _address_type = 0;

		if ( pi != NULL )
		{
			_hostname = pi->hostname;
			_punycode_hostname = pi->punycode_hostname;
			_ip_address = pi->ip_address;
			port = pi->port;

			_address_type = pi->address_type;
		}
		else if ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP )
		{
			_hostname = cfg_hostname;
			_punycode_hostname = g_punycode_hostname;
			_ip_address = cfg_ip_address;
			port = cfg_port;

			_address_type = cfg_address_type;
		}
		else if ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS )
		{
			_hostname = cfg_hostname_s;
			_punycode_hostname = g_punycode_hostname_s;
			_ip_address = cfg_ip_address_s;
			port = cfg_port_s;

			_address_type = cfg_address_type_s;
		}
		else if ( cfg_enable_proxy_socks )
		{
			_hostname = cfg_hostname_socks;
			_punycode_hostname = g_punycode_hostname_socks;
			_ip_address = cfg_ip_address_socks;
			port = cfg_port_socks;

			_address_type = cfg_address_type_socks;
		}
		else
		{
			use_supplied_params = true;
		}

		__snwprintf( wport, 6, L"%hu", port );

		if ( use_supplied_params )
		{
			int whost_length = MultiByteToWideChar( CP_UTF8, 0, host, -1, NULL, 0 );	// Include the NULL terminator.
			whost = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * whost_length );
			MultiByteToWideChar( CP_UTF8, 0, host, -1, whost, whost_length );

			t_whost = whost;
		}
		else
		{
			if ( _address_type == 0 )
			{
				whost = ( _punycode_hostname != NULL ? _punycode_hostname : _hostname );
			}
			else
			{
				struct sockaddr_in src_addr;
				_memzero( &src_addr, sizeof( sockaddr_in ) );

				src_addr.sin_family = AF_INET;
				src_addr.sin_addr.s_addr = _htonl( _ip_address );

				DWORD wcs_ip_length = 16;
				_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

				whost = wcs_ip;
			}
		}

		nRet = _GetAddrInfoW( whost, wport, &hints, &context->address_info );
		if ( nRet == WSAHOST_NOT_FOUND )
		{
			use_ipv6 = true;

			hints.ai_family = AF_INET6;	// Try IPv6
			nRet = _GetAddrInfoW( whost, wport, &hints, &context->address_info );
		}

		GlobalFree( t_whost );

		if ( nRet != 0 )
		{
			return false;
		}
	}

	if ( _enable_proxy_socks &&
		 context->proxy_address_info == NULL &&
		 !_resolve_domain_names )
	{
		_memzero( &hints, sizeof( addrinfoW ) );
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_IP;
		hints.ai_flags = AI_CANONNAME;

		__snwprintf( wport, 6, L"%hu", context->request_info.port );

		int whost_length = MultiByteToWideChar( CP_UTF8, 0, context->request_info.host, -1, NULL, 0 );	// Include the NULL terminator.
		whost = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * whost_length );
		MultiByteToWideChar( CP_UTF8, 0, context->request_info.host, -1, whost, whost_length );

		nRet = _GetAddrInfoW( whost, wport, &hints, &context->proxy_address_info );
		if ( nRet == WSAHOST_NOT_FOUND && _socks_type == SOCKS_TYPE_V5 )	// Allow IPv6 for SOCKS 5
		{
			hints.ai_family = AF_INET6;	// Try IPv6
			_GetAddrInfoW( whost, wport, &hints, &context->proxy_address_info );
		}

		GlobalFree( whost );
	}

	SOCKET socket = CreateSocket( use_ipv6 );
	if ( socket == INVALID_SOCKET )
	{
		return false;
	}

	context->socket = socket;

	g_hIOCP = CreateIoCompletionPort( ( HANDLE )socket, g_hIOCP, 0/*( ULONG_PTR )context*/, 0 );
	if ( g_hIOCP == NULL )
	{
		return false;
	}

	// Socket must be bound before we can use it with ConnectEx.
	struct sockaddr_in ipv4_addr;
	struct sockaddr_in6 ipv6_addr;

	if ( use_ipv6 )
	{
		_memzero( &ipv6_addr, sizeof( ipv6_addr ) );
		ipv6_addr.sin6_family = AF_INET6;
		//ipv6_addr.sin6_addr = in6addr_any;	// This assignment requires the CRT, but it's all zeros anyway and it gets set by _memzero().
		//ipv6_addr.sin6_port = 0;
		nRet = _bind( socket, ( SOCKADDR * )&ipv6_addr, sizeof( ipv6_addr ) );
	}
	else
	{
		_memzero( &ipv4_addr, sizeof( ipv4_addr ) );
		ipv4_addr.sin_family = AF_INET;
		//ipv4_addr.sin_addr.s_addr = INADDR_ANY;
		//ipv4_addr.sin_port = 0;
		nRet = _bind( socket, ( SOCKADDR * )&ipv4_addr, sizeof( ipv4_addr ) );
	}

	if ( nRet == SOCKET_ERROR )
	{
		return false;
	}

	// Attempt to connect to the host.
	InterlockedIncrement( &context->pending_operations );

	context->overlapped.current_operation = IO_Connect;

	DWORD lpdwBytesSent = 0;
	BOOL bRet = _ConnectEx( socket, context->address_info->ai_addr, ( int )context->address_info->ai_addrlen, NULL, 0, &lpdwBytesSent, ( OVERLAPPED * )&context->overlapped );
	if ( bRet == FALSE && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		InterlockedDecrement( &context->pending_operations );

		/*if ( context->address_info != NULL )
		{
			_FreeAddrInfoW( context->address_info );
			context->address_info = NULL;
		}*/

		return false;
	}

	/*if ( context->address_info != NULL )
	{
		_FreeAddrInfoW( context->address_info );
		context->address_info = NULL;
	}*/

	return true;
}

void UpdateRangeList( DOWNLOAD_INFO *di )
{
	if ( di == NULL )
	{
		return;
	}

	RANGE_INFO *ri;
	RANGE_INFO *ri_copy;
	DoublyLinkedList *range_node = di->range_list;
	DoublyLinkedList *range_node_copy;

	if ( range_node != NULL )
	{
		unsigned short range_info_count = 0;

		DoublyLinkedList *active_range_list = NULL;

		// Determine the number of ranges that still need downloading.
		while ( range_node != NULL )
		{
			ri = ( RANGE_INFO * )range_node->data;

			// Check if our range still needs to download.
			if ( ri != NULL && ( ri->content_offset < ( ( ri->range_end - ri->range_start ) + 1 ) ) )
			{
				ri_copy = ( RANGE_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( RANGE_INFO ) );
				if ( ri_copy != NULL )
				{
					++range_info_count;

					ri_copy->content_length = ri->content_length;
					ri_copy->content_offset = ri->content_offset;
					ri_copy->file_write_offset = ri->file_write_offset;
					ri_copy->range_end = ri->range_end;
					ri_copy->range_start = ri->range_start;

					DoublyLinkedList *new_range_node = DLL_CreateNode( ( void * )ri_copy );
					DLL_AddNode( &active_range_list, new_range_node, -1 );
				}
			}

			range_node = range_node->next;
		}

		// Can we split any remaining parts to fill the total?
		if ( range_info_count > 0 && range_info_count < di->parts )
		{
			unsigned short parts = di->parts / range_info_count;
			unsigned short rem_parts = di->parts % range_info_count;

			range_node = di->range_list;
			range_node_copy = active_range_list;

			while ( range_node_copy != NULL )
			{
				unsigned short t_parts = parts;

				if ( rem_parts > 0 )	// Distribute any remainder parts amongst the remaining ranges.
				{
					++t_parts;
					--rem_parts;
				}

				ri_copy = ( RANGE_INFO * )range_node_copy->data;

				unsigned long long remaining_length = ( ( ri_copy->range_end - ri_copy->range_start ) + 1 ) - ri_copy->content_offset;

				// We'll only use 1 part for this range since it's too small to split up and the remainder of parts will be used for the next range.
				if ( remaining_length < t_parts )
				{
					rem_parts += ( t_parts - 1 );
					t_parts = 1;
				}

				unsigned long long range_size = remaining_length / t_parts;
				unsigned long long range_offset = ri_copy->range_start + ri_copy->content_offset;
				unsigned long long range_end = ri_copy->range_end;

				for ( unsigned short part = 1; part <= t_parts; ++part )
				{
					// Download was restarted before a range list was created.
					if ( range_node == NULL )
					{
						ri = ( RANGE_INFO * )GlobalAlloc( GMEM_FIXED/*GPTR*/, sizeof( RANGE_INFO ) );
						if ( ri != NULL )
						{
							range_node = DLL_CreateNode( ( void * )ri );
							DLL_AddNode( &di->range_list, range_node, -1 );
						}

						range_node = NULL;
					}
					else
					{
						ri = ( RANGE_INFO * )range_node->data;

						range_node = range_node->next;
					}

					if ( ri != NULL )
					{
						// Reuse this range info.
						ri->content_length = 0;
						ri->content_offset = 0;

						if ( part == 1 )
						{
							ri->range_start = range_offset;
						}
						else
						{
							ri->range_start = range_offset + 1;
						}

						if ( part < t_parts )
						{
							range_offset += range_size;
							ri->range_end = range_offset;
						}
						else	// Make sure we have an accurate range end for the last part.
						{
							ri->range_end = range_end;
						}

						ri->file_write_offset = ri->range_start;
					}
					else
					{
						--part;
						--t_parts;
					}
				}

				range_node_copy = range_node_copy->next;
			}

			di->range_list_end = range_node;

			// Zero out the unused range info.
			/*while ( range_node != NULL )
			{
				ri = ( RANGE_INFO * )range_node->data;
				ri->content_length = 0;
				ri->content_offset = 0;
				ri->file_write_offset = 0;
				ri->range_end = 0;
				ri->range_start = 0;

				range_node = range_node->next;
			}*/
		}

		while ( active_range_list != NULL )
		{
			range_node = active_range_list;
			active_range_list = active_range_list->next;

			GlobalFree( range_node->data );
			GlobalFree( range_node );
		}
	}
	else
	{
		ri = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );
		range_node = DLL_CreateNode( ( void * )ri );
		DLL_AddNode( &di->range_list, range_node, -1 );
	}
}

// reset_type: 1 = file beginning, 2 = host beginning
void ResetDownload( DOWNLOAD_INFO *di, unsigned char reset_type, bool reset_progress )
{
	if ( di != NULL )
	{
		if ( reset_type == START_TYPE_HOST )	// Reset host not in a group.
		{
			if ( reset_progress )
			{
				_SendMessageW( g_hWnd_main, WM_RESET_PROGRESS, 0, ( LPARAM )di );
			}

			while ( di->range_list != NULL )
			{
				DoublyLinkedList *range_node = di->range_list;
				di->range_list = di->range_list->next;

				GlobalFree( range_node->data );
				GlobalFree( range_node );
			}

			di->range_list_end = NULL;

			di->processed_header = false;

			di->last_modified.QuadPart = 0;

			EnterCriticalSection( &di->shared_info->di_cs );

			di->shared_info->downloaded -= di->downloaded;

			LeaveCriticalSection( &di->shared_info->di_cs );

			di->last_downloaded = 0;
			di->downloaded = 0;
			di->file_size = 0;
			SetStatus( di, STATUS_NONE );
		}
		else if ( reset_type == START_TYPE_HOST_IN_GROUP )	// Reset host in a group.
		{
			DoublyLinkedList *range_node = di->range_list;

			unsigned long long content_length = di->file_size;
			if ( content_length > 0 )
			{
				unsigned long long range_size = content_length / di->parts;
				unsigned long long range_offset;

				RANGE_INFO *ri;
				if ( range_node != NULL )
				{
					ri = ( RANGE_INFO * )range_node->data;

					range_offset = ri->range_start;
					content_length += range_offset;
				}
				else
				{
					ri = NULL;
					range_offset = 0;
				}

				unsigned short t_parts = di->parts;

				for ( unsigned short part = 1; part <= t_parts; ++part )
				{
					if ( range_node != NULL )
					{
						ri = ( RANGE_INFO * )range_node->data;

						range_node = range_node->next;
					}
					else
					{
						ri = ( RANGE_INFO * )GlobalAlloc( GMEM_FIXED/*GPTR*/, sizeof( RANGE_INFO ) );
						if ( ri != NULL )
						{
							range_node = DLL_CreateNode( ( void * )ri );
							DLL_AddNode( &di->range_list, range_node, -1 );
						}

						range_node = NULL;
					}

					if ( ri != NULL )
					{
						ri->content_length = 0;
						ri->content_offset = 0;
						//ri->file_write_offset = 0;
						//ri->range_start = 0;
						//ri->range_end = 0;

						ri->range_start = range_offset;

						if ( part < t_parts )
						{
							range_offset += range_size;
							ri->range_end = range_offset - 1;
						}
						else	// Make sure we have an accurate range end for the last part.
						{
							ri->range_end = content_length - 1;
						}

						ri->file_write_offset = ri->range_start;
					}
					else
					{
						--part;
						--t_parts;
					}
				}

				// Extra range nodes were allocated. Remove them.
				if ( range_node != NULL )
				{
					di->range_list->prev = range_node->prev;

					range_node->prev->next = NULL;

					while ( range_node != NULL )
					{
						DoublyLinkedList *del_range_node = range_node;
						range_node = range_node->next;

						GlobalFree( del_range_node->data );
						GlobalFree( del_range_node );
					}
				}
			}

			di->last_modified.QuadPart = 0;

			EnterCriticalSection( &di->shared_info->di_cs );

			di->shared_info->downloaded -= di->downloaded;

			LeaveCriticalSection( &di->shared_info->di_cs );

			di->last_downloaded = 0;
			di->downloaded = 0;
			SetStatus( di, STATUS_NONE );
		}
		else if ( reset_type == START_TYPE_GROUP )	// Reset group
		{
			EnterCriticalSection( &di->shared_info->di_cs );

			di->shared_info->processed_header = false;

			di->shared_info->last_downloaded = 0;
			di->shared_info->downloaded = 0;
			di->shared_info->file_size = 0;
			SetStatus( di->shared_info, STATUS_NONE );

			LeaveCriticalSection( &di->shared_info->di_cs );
		}

		// If we manually start a download, then set the incomplete retry attempts back to 0.
		di->retries = 0;

		di->start_time.QuadPart = 0;

		di->download_operations &= ~( DOWNLOAD_OPERATION_ADD_STOPPED | DOWNLOAD_OPERATION_MODIFIED );

		EnterCriticalSection( &di->shared_info->di_cs );

		if ( di->shared_info->active_hosts == 0 )
		{
			di->shared_info->start_time.QuadPart = 0;
		}

		// If we manually start a download that was added remotely, then allow the prompts to display.
		di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_OVERRIDE_PROMPTS;

		LeaveCriticalSection( &di->shared_info->di_cs );
	}
}

void RestartDownload( DOWNLOAD_INFO *di, unsigned char restart_type, unsigned char start_operation )
{
	if ( di != NULL )
	{
		ResetDownload( di, restart_type );
		StartDownload( di, restart_type, start_operation );
	}
}

void SetSkippedStatus( DOWNLOAD_INFO *di, unsigned char start_type )
{
	if ( di != NULL )
	{
		if ( IS_GROUP( di ) && start_type == START_TYPE_GROUP )
		{
			EnterCriticalSection( &di->shared_info->di_cs );
			SetStatus( di->shared_info, STATUS_SKIPPED );
			LeaveCriticalSection( &di->shared_info->di_cs );

			DoublyLinkedList *host_node = di->shared_info->host_list;
			while ( host_node != NULL )
			{
				DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
				if ( host_di != NULL )
				{
					EnterCriticalSection( &host_di->di_cs );
					SetStatus( host_di, STATUS_SKIPPED );
					LeaveCriticalSection( &host_di->di_cs );
				}

				host_node = host_node->next;
			}
		}
		else
		{
			SetStatus( di, STATUS_SKIPPED );
		}
	}
}

void StartDownload( DOWNLOAD_INFO *di, unsigned char start_type, unsigned char start_operation )
{
	if ( di == NULL )
	{
		return;
	}

	unsigned char add_state = 0;

	PROTOCOL protocol = PROTOCOL_UNKNOWN;
	wchar_t *host = NULL;
	wchar_t *resource = NULL;
	unsigned short port = 0;

	bool skip_start = false;

	EnterCriticalSection( &di->di_cs );

	wchar_t prompt_message[ MAX_PATH + 512 ];
	wchar_t temp_file_path[ MAX_PATH ];
	wchar_t final_file_path[ MAX_PATH ];

	int temp_filename_offset = 0, final_filename_offset = 0;
	int temp_file_extension_offset = 0, final_file_extension_offset = 0;

	bool prompt_final_path = false;
	bool prompt_temp_path = false;

	if ( cfg_use_temp_download_directory )
	{
		int filename_length = GetTemporaryFilePath( di, temp_file_path );

		temp_filename_offset = g_temp_download_directory_length + 1;
		temp_file_extension_offset = temp_filename_offset + get_file_extension_offset( di->shared_info->file_path + di->shared_info->filename_offset, filename_length );

		// See if the file exits.
		if ( GetFileAttributesW( temp_file_path ) != INVALID_FILE_ATTRIBUTES )
		{
			prompt_temp_path = true;
		}
	}

	GetDownloadFilePath( di, final_file_path );

	final_filename_offset = di->shared_info->filename_offset;
	final_file_extension_offset = di->shared_info->file_extension_offset;

	// See if the file exits.
	if ( GetFileAttributesW( final_file_path ) != INVALID_FILE_ATTRIBUTES )
	{
		prompt_final_path = true;
	}

	if ( prompt_final_path || prompt_temp_path )
	{
		if ( start_operation & START_OPERATION_CHECK_FILE )
		{
			di->download_operations &= ~( DOWNLOAD_OPERATION_RENAME | DOWNLOAD_OPERATION_OVERWRITE );

			if ( cfg_prompt_rename == 0 && di->shared_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS )
			{
				SetSkippedStatus( di, start_type );

				skip_start = true;
			}
			else
			{
				// If the last return value was not set to remember our choice, then prompt again.
				if ( ( cfg_prompt_rename == 0 || start_operation & START_OPERATION_FORCE_PROMPT ) &&
					 g_rename_file_cmb_ret != CMBIDRENAMEALL &&
					 g_rename_file_cmb_ret != CMBIDOVERWRITEALL &&
					 g_rename_file_cmb_ret != CMBIDSKIPALL )
				{
					__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___already_exists, ( prompt_final_path ? final_file_path : temp_file_path ) );

					g_rename_file_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_RENAMEOVERWRITESKIPALL );
				}

				// Rename the file and try again.
				if ( ( cfg_prompt_rename == 1 && !( start_operation & START_OPERATION_FORCE_PROMPT ) ) ||
				   ( ( cfg_prompt_rename == 0 || start_operation & START_OPERATION_FORCE_PROMPT ) &&
					 ( g_rename_file_cmb_ret == CMBIDRENAME ||
					   g_rename_file_cmb_ret == CMBIDRENAMEALL ) ) )
				{
					// Creates a tree of active and queued downloads.
					dllrbt_tree *add_files_tree = CreateFilenameTree();

					bool rename_succeeded;

					if ( prompt_temp_path )
					{
						rename_succeeded = RenameFile( add_files_tree,
													   di->shared_info->file_path, &di->shared_info->filename_offset, &di->shared_info->file_extension_offset,
													   temp_file_path, temp_filename_offset, temp_file_extension_offset );
					}
					else
					{
						rename_succeeded = RenameFile( add_files_tree,
													   di->shared_info->file_path, &di->shared_info->filename_offset, &di->shared_info->file_extension_offset,
													   final_file_path, final_filename_offset, final_file_extension_offset );
					}

					if ( prompt_final_path )
					{
						di->download_operations |= DOWNLOAD_OPERATION_RENAME;	// When moving the file, we'll automatically rename it instead of prompting.
					}

					// The tree is only used to determine duplicate filenames.
					DestroyFilenameTree( add_files_tree );

					if ( !rename_succeeded )
					{
						if ( g_rename_file_cmb_ret2 != CMBIDOKALL && !( di->shared_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS ) )
						{
							__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___could_not_be_renamed, ( prompt_final_path ? final_file_path : temp_file_path ) );

							g_rename_file_cmb_ret2 = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_OKALL );
						}

						SetSkippedStatus( di, start_type );

						skip_start = true;
					}
				}
				else if ( ( cfg_prompt_rename == 3 && !( start_operation & START_OPERATION_FORCE_PROMPT ) ) ||
						( ( cfg_prompt_rename == 0 || start_operation & START_OPERATION_FORCE_PROMPT ) &&
						  ( g_rename_file_cmb_ret == CMBIDFAIL ||
							g_rename_file_cmb_ret == CMBIDSKIP ||
							g_rename_file_cmb_ret == CMBIDSKIPALL ) ) ) // Skip the rename or overwrite if the return value fails, or the user selected skip.
				{
					SetSkippedStatus( di, start_type );

					skip_start = true;
				}
				else	// Overwrite
				{
					if ( prompt_final_path )
					{
						di->shared_info->download_operations |= DOWNLOAD_OPERATION_OVERWRITE;	// When moving the file, we'll automatically overwrite it instead of prompting.
					}
				}
			}
		}
	}
	else if ( !( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) &&
				 di->shared_info->downloaded > 0 )	// If the file doesn't exist and it's been partially downloaded, then ask to restart.
	{
		if ( g_file_not_exist_cmb_ret != CMBIDNOALL && g_file_not_exist_cmb_ret != CMBIDYESALL )
		{
			g_file_not_exist_cmb_ret = CMessageBoxW( g_hWnd_main, ST_V_PROMPT_The_specified_file_was_not_found, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_YESNOALL );
		}

		if ( g_file_not_exist_cmb_ret == CMBIDYES || g_file_not_exist_cmb_ret == CMBIDYESALL )
		{
			bool is_group = IS_GROUP( di );

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
			StartDownload( ( DOWNLOAD_INFO * )di->shared_info->host_list->data, ( is_group ? START_TYPE_GROUP : START_TYPE_HOST ), start_operation );
		}
		else
		{
			SetSkippedStatus( di, start_type );
		}

		skip_start = true;
	}

	LeaveCriticalSection( &di->di_cs );

	if ( skip_start )
	{
		return;
	}

	unsigned int host_length = 0;
	unsigned int resource_length = 0;

	ParseURL_W( di->url, NULL, protocol, &host, host_length, port, &resource, resource_length, NULL, NULL, NULL, NULL );

	wchar_t *w_resource;

	if ( protocol == PROTOCOL_FTP ||
		 protocol == PROTOCOL_FTPS ||
		 protocol == PROTOCOL_FTPES ||
		 protocol == PROTOCOL_SFTP )
	{
		w_resource = url_decode_w( resource, resource_length, &resource_length );

		if ( w_resource != NULL )
		{
			GlobalFree( resource );
			resource = w_resource;
		}
	}

	w_resource = resource;

	if ( protocol != PROTOCOL_FTP &&
		 protocol != PROTOCOL_FTPS &&
		 protocol != PROTOCOL_FTPES &&
		 protocol != PROTOCOL_SFTP )
	{
		while ( w_resource != NULL && *w_resource != NULL )
		{
			if ( *w_resource == L'#' )
			{
				*w_resource = 0;
				resource_length = ( unsigned int )( w_resource - resource );

				break;
			}

			++w_resource;
		}
	}

	/*w_resource = url_encode_w( resource, resource_length, &w_resource_length );

	// Did we encode anything?
	if ( resource_length != w_resource_length )
	{
		GlobalFree( resource );
		resource = w_resource;

		resource_length = w_resource_length;
	}
	else
	{
		GlobalFree( w_resource );
	}*/

	if ( normaliz_state == NORMALIZ_STATE_RUNNING )
	{
		int punycode_length = _IdnToAscii( 0, host, host_length, NULL, 0 );

		if ( ( unsigned int )punycode_length > host_length )
		{
			wchar_t *punycode = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( punycode_length + 1 ) );
			host_length = _IdnToAscii( 0, host, host_length, punycode, punycode_length );
			punycode[ host_length ] = 0;	// Sanity.

			GlobalFree( host );
			host = punycode;
		}
	}

	unsigned char part = 1;

	//EnterCriticalSection( &cleanup_cs );

	di->code = 0;

	// If the number of ranges is less than the total number of parts that's been set for the download,
	// then the remaining ranges will be split to equal the total number of parts.
	UpdateRangeList( di );

	di->print_range_list = di->range_list;
	di->range_queue = NULL;

	// The download completed, but its status was not set to STATUS_COMPLETED.
	bool finish_cleanup = ( di->shared_info->downloaded >= di->shared_info->file_size && di->shared_info->file_size != 0 );

	DoublyLinkedList *range_node = di->range_list;

	while ( range_node != di->range_list_end )
	{
		RANGE_INFO *ri = ( RANGE_INFO * )range_node->data;

		// Check if our range still needs to download.
		if ( ( ri != NULL && ( ri->content_offset < ( ( ri->range_end - ri->range_start ) + 1 ) ) ) || finish_cleanup )
		{
			// Split the remaining range_list off into the range_queue.
			if ( di->parts_limit > 0 && part > di->parts_limit )
			{
				di->range_queue = range_node;

				break;
			}

			// Check the state of our downloads/queue once.
			if ( add_state == 0 )
			{
				if ( g_total_downloading < cfg_max_downloads )
				{
					add_state = 1;	// Create the connection.

					FILETIME ft;

					// Set the start time only if we've manually started the download.
					if ( IS_GROUP( di ) )
					{
						EnterCriticalSection( &di->shared_info->di_cs );
						if ( di->shared_info->active_hosts == 0 )
						{
							if ( di->shared_info->start_time.QuadPart == 0 )
							{
								GetSystemTimeAsFileTime( &ft );
								di->shared_info->start_time.LowPart = ft.dwLowDateTime;
								di->shared_info->start_time.HighPart = ft.dwHighDateTime;
							}

							SetStatus( di->shared_info, STATUS_CONNECTING );
						}
						LeaveCriticalSection( &di->shared_info->di_cs );
					}

					if ( di->start_time.QuadPart == 0 )
					{
						GetSystemTimeAsFileTime( &ft );
						di->start_time.LowPart = ft.dwLowDateTime;
						di->start_time.HighPart = ft.dwHighDateTime;
					}

					SetStatus( di, STATUS_CONNECTING );

					EnterCriticalSection( &active_download_list_cs );

					// Add to the global active download list.
					di->download_node.data = di;
					DLL_AddNode( &active_download_list, &di->download_node, -1 );

					if ( IS_GROUP( di ) )
					{
						EnterCriticalSection( &di->shared_info->di_cs );
						if ( di->shared_info->download_node.data == NULL )
						{
							di->shared_info->download_node.data = di->shared_info;

							DLL_AddNode( &active_download_list, &di->shared_info->download_node, -1 );
						}
						LeaveCriticalSection( &di->shared_info->di_cs );
					}

					LeaveCriticalSection( &active_download_list_cs );

					EnterCriticalSection( &cleanup_cs );

					++g_total_downloading;

					EnableTimers( true );

					LeaveCriticalSection( &cleanup_cs );

					//

					EnterCriticalSection( &di->shared_info->di_cs );

					++di->shared_info->active_hosts;

					LeaveCriticalSection( &di->shared_info->di_cs );
				}
				else
				{
					add_state = 2;	// Queue the download.

					if ( di->queue_node.data == NULL && !( di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) )
					{
						SetStatus( di, STATUS_CONNECTING | STATUS_QUEUED );	// Queued.

						// If we haven't processed the header information, then the other items (non-driver hosts) aren't going to be started and won't queue naturally.
						// We'll do it here.
						if ( IS_GROUP( di ) && !di->shared_info->processed_header )
						{
							EnterCriticalSection( &di->shared_info->di_cs );
							SetStatus( di->shared_info, STATUS_CONNECTING | STATUS_QUEUED );	// Queued.
							LeaveCriticalSection( &di->shared_info->di_cs );

							DoublyLinkedList *host_node = di->shared_info->host_list;
							while ( host_node != NULL )
							{
								DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
								if ( host_di != NULL )
								{
									if ( host_di->status != STATUS_COMPLETED &&
									  !( host_di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) )	// status might have been set to stopped when added.
									{
										SetStatus( host_di, STATUS_CONNECTING | STATUS_QUEUED );
									}
								}

								host_node = host_node->next;
							}
						}

						EnterCriticalSection( &download_queue_cs );

						// Add to the global download queue.
						di->queue_node.data = di;
						DLL_AddNode( &download_queue, &di->queue_node, -1 );

						LeaveCriticalSection( &download_queue_cs );
					}
				}
			}

			if ( IS_GROUP( di ) )
			{
				EnterCriticalSection( &di->shared_info->di_cs );

				if ( di->shared_info->active_hosts <= 1 && add_state == 1 )	// Include the host we're starting.
				{
					di->shared_info->last_downloaded = di->shared_info->downloaded;
				}

				LeaveCriticalSection( &di->shared_info->di_cs );
			}

			di->last_downloaded = di->downloaded;

			if ( add_state == 1 )
			{
				// Save the request information, the header information (if we got any), and create a new connection.
				SOCKET_CONTEXT *context = CreateSocketContext();

				if ( protocol == PROTOCOL_FTP ||
					 protocol == PROTOCOL_FTPS ||
					 protocol == PROTOCOL_FTPES )
				{
					context->ftp_connection_type = FTP_CONNECTION_TYPE_CONTROL;
				}

				context->processed_header = di->processed_header;

				context->part = part;
				context->parts = ( unsigned char )di->parts;

				// If we've processed the header, then we would have already gotten a content disposition filename.
				if ( di->processed_header )
				{
					context->got_filename = 1;
				}

				// The first host in the host list is what drives every other host. No other host should prompt.
				if ( di->shared_info->processed_header &&
					 IS_GROUP( di ) &&
					 di->shared_info->host_list != NULL &&
					 di != ( DOWNLOAD_INFO * )di->shared_info->host_list->data )
				{
					context->got_filename = 1;				// No need to rename it again.
//					context->got_last_modified = 1;			// No need to get the date/time again.
					context->show_file_size_prompt = false;	// No need to prompt again.
				}

				context->request_info.port = port;
				context->request_info.protocol = protocol;

				int cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, host, host_length + 1, NULL, 0, NULL, NULL );
				char *utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, host, host_length + 1, utf8_cfg_val, cfg_val_length, NULL, NULL );

				context->request_info.host = utf8_cfg_val;

				cfg_val_length = WideCharToMultiByte( CP_UTF8, 0, resource, resource_length + 1, NULL, 0, NULL, NULL );
				utf8_cfg_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * cfg_val_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, resource, resource_length + 1, utf8_cfg_val, cfg_val_length, NULL, NULL );

				context->request_info.resource = utf8_cfg_val;

				context->download_info = di;

				ri->range_start += ri->content_offset;	// Begin where we left off.
				ri->content_offset = 0;	// Reset.
				ri->content_length = 0;	// Group hosts need to have their content_length reset so we can compare it with shared_info->file_size.

				context->header_info.range_info = ri;

				if ( di->cookies != NULL )
				{
					char *new_cookies = NULL;

					// This value will be saved
					if ( !ParseCookieValues( di->cookies, &context->header_info.cookie_tree, &new_cookies ) )
					{
						GlobalFree( new_cookies );
						new_cookies = NULL;
					}

					// If we got a new cookie.
					if ( new_cookies != NULL )
					{
						// Then see if the new cookie is not blank.
						if ( new_cookies[ 0 ] != NULL )
						{
							context->header_info.cookies = new_cookies;
						}
						else	// Otherwise, if the cookie is blank, then free it.
						{
							GlobalFree( new_cookies );
						}
					}
				}

				//

				// Add to the parts list.
				context->parts_node.data = context;

				EnterCriticalSection( &di->di_cs );

				DLL_AddNode( &di->parts_list, &context->parts_node, -1 );

				++di->active_parts;

				LeaveCriticalSection( &di->di_cs );

				EnterCriticalSection( &di->shared_info->di_cs );

				// For groups.
				if ( IS_GROUP( di ) )
				{
					++di->shared_info->active_parts;
				}

				LeaveCriticalSection( &di->shared_info->di_cs );

				//

				// Add to the global download list.
				context->context_node.data = context;

				EnterCriticalSection( &context_list_cs );

				DLL_AddNode( &g_context_list, &context->context_node, 0 );

				LeaveCriticalSection( &context_list_cs );

				//

#ifdef ENABLE_LOGGING
				GenericLogEntry( di, LOG_INFO_CON_STATE, "Connecting" );
#endif

				context->status = STATUS_CONNECTING;

				if ( finish_cleanup || !CreateConnection( context, context->request_info.host, context->request_info.port ) )
				{
					context->status = ( finish_cleanup ? STATUS_NONE : STATUS_FAILED );

					InterlockedIncrement( &context->pending_operations );
					context->overlapped.current_operation = IO_Close;
					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
				}
			}
			else if ( add_state == 2 )
			{
				add_state = 3;	// Skip adding anymore values to the queue.

				EnterCriticalSection( &di->di_cs );

				di->active_parts = 0;

				LeaveCriticalSection( &di->di_cs );
			}

			++part;
		}

		range_node = range_node->next;
	}

	//LeaveCriticalSection( &cleanup_cs );

	GlobalFree( host );
	GlobalFree( resource );
}

dllrbt_tree *CreateFilenameTree()
{
	DOWNLOAD_INFO *di = NULL;
	wchar_t *filename = NULL;

	// Make a tree of active and queued downloads to find filenames that need to be renamed.
	dllrbt_tree *filename_tree = dllrbt_create( dllrbt_compare_w );

	EnterCriticalSection( &download_queue_cs );

	DoublyLinkedList *tmp_node = download_queue;
	while ( tmp_node != NULL )
	{
		di = ( DOWNLOAD_INFO * )tmp_node->data;
		if ( di != NULL )
		{
			filename = GlobalStrDupW( di->shared_info->file_path + di->shared_info->filename_offset );

			if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( filename );
			}
		}

		tmp_node = tmp_node->next;
	}

	LeaveCriticalSection( &download_queue_cs );

	EnterCriticalSection( &active_download_list_cs );

	tmp_node = active_download_list;
	while ( tmp_node != NULL )
	{
		di = ( DOWNLOAD_INFO * )tmp_node->data;
		if ( di != NULL )
		{
			filename = GlobalStrDupW( di->shared_info->file_path + di->shared_info->filename_offset );

			if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
			{
				GlobalFree( filename );
			}
		}

		tmp_node = tmp_node->next;
	}

	LeaveCriticalSection( &active_download_list_cs );

	return filename_tree;
}

void DestroyFilenameTree( dllrbt_tree *filename_tree )
{
	// The tree is only used to determine duplicate filenames.
	node_type *node = dllrbt_get_head( filename_tree );
	while ( node != NULL )
	{
		wchar_t *filename = ( wchar_t * )node->val;

		if ( filename != NULL )
		{
			GlobalFree( filename );
		}

		node = node->next;
	}
	dllrbt_delete_recursively( filename_tree );
}

bool RenameFile( dllrbt_tree *filename_tree, wchar_t *old_file_path, unsigned int *old_filename_offset, unsigned int *old_file_extension_offset, wchar_t *new_file_path, unsigned int new_filename_offset, unsigned int new_file_extension_offset )
{
	unsigned int rename_count = 0;

	// The maximum folder path length is 248 (including the trailing '\').
	// The maximum file name length in the case above is 11 (not including the NULL terminator).
	// The total is 259 characters (not including the NULL terminator).
	// MAX_PATH is 260.

	// We don't want to overwrite the download info until the very end.
	wchar_t t_file_path[ MAX_PATH ];
	_wmemcpy_s( t_file_path, MAX_PATH, new_file_path, MAX_PATH );

	t_file_path[ new_filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.

	do
	{
		while ( dllrbt_find( filename_tree, ( void * )( t_file_path + new_filename_offset ), false ) != NULL )
		{
			// If there's a file extension, then put the counter before it.
			int ret = __snwprintf( t_file_path + new_file_extension_offset, MAX_PATH - new_file_extension_offset - 1, L" (%lu)%s", ++rename_count, new_file_path + new_file_extension_offset );

			// Can't rename.
			if ( ret < 0 )
			{
				return false;
			}
		}

		// Add the new filename to the add files tree.
		wchar_t *filename = GlobalStrDupW( t_file_path + new_filename_offset );

		if ( dllrbt_insert( filename_tree, ( void * )filename, ( void * )filename ) != DLLRBT_STATUS_OK )
		{
			GlobalFree( filename );
		}
	}
	while ( GetFileAttributesW( t_file_path ) != INVALID_FILE_ATTRIBUTES );

	// Set the new filename.
	_wmemcpy_s( old_file_path + *old_filename_offset, MAX_PATH - *old_filename_offset, t_file_path + new_filename_offset, MAX_PATH - *old_filename_offset );
	old_file_path[ MAX_PATH - 1 ] = 0;	// Sanity.

	// Get the new file extension offset.
	*old_file_extension_offset = *old_filename_offset + get_file_extension_offset( old_file_path + *old_filename_offset, lstrlenW( old_file_path + *old_filename_offset ) );

	return true;
}

void UpdateDownloadDirectoryInfo( DOWNLOAD_INFO *di, wchar_t *new_download_directory, unsigned int new_download_directory_length )
{
	if ( di != NULL )
	{
		new_download_directory_length = min( MAX_PATH, new_download_directory_length + 1 );	// Include NULL character.

		unsigned int file_name_length = lstrlenW( di->file_path + di->filename_offset );
		file_name_length = min( file_name_length, ( int )( MAX_PATH - new_download_directory_length - 1 ) );

		_memmove( di->file_path + new_download_directory_length, di->file_path + di->filename_offset, sizeof( wchar_t ) * ( file_name_length + 1 ) );
		_wmemcpy_s( di->file_path, new_download_directory_length, new_download_directory, new_download_directory_length );
		di->file_path[ MAX_PATH - 1 ] = 0;	// Sanity.

		di->file_extension_offset = min( MAX_PATH - 1, di->file_extension_offset + ( int )( new_download_directory_length - di->filename_offset ) );
		di->filename_offset = new_download_directory_length;
	}
}

ICON_INFO *CacheIcon( DOWNLOAD_INFO *di )
{
	ICON_INFO *ii = NULL;

	if ( di != NULL )
	{
		SHFILEINFO sfi;
		sfi.hIcon = NULL;

		wchar_t *file_extension = di->file_path + di->file_extension_offset;

		if ( cfg_show_embedded_icon && lstrcmpiW( file_extension, L".exe" ) == 0 )
		{
			if ( !( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				wchar_t file_path[ MAX_PATH ];
				UINT extracted_icons = 0;

				// Get the icon from its temporary directory or its download directory.
				if ( cfg_use_temp_download_directory )
				{
					GetTemporaryFilePath( di, file_path );

					extracted_icons = _ExtractIconExW( file_path, 0, NULL, &sfi.hIcon, 1 );
				}

				_wmemcpy_s( file_path, MAX_PATH, di->file_path, MAX_PATH );
				if ( di->filename_offset > 0 )
				{
					file_path[ di->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
				}

				if ( extracted_icons != 1 )
				{
					extracted_icons = _ExtractIconExW( file_path, 0, NULL, &sfi.hIcon, 1 );
				}

				if ( extracted_icons == 1 )
				{
					file_extension = file_path;
				}
			}
		}

		// Cache our file's icon.
		EnterCriticalSection( &icon_cache_cs );
		ii = ( ICON_INFO * )dllrbt_find( g_icon_handles, ( void * )file_extension, true );
		if ( ii == NULL )
		{
			if ( sfi.hIcon == NULL )
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

				// Use an unknown file type icon for extensionless files.
				_SHGetFileInfoW( ( di->file_path[ di->file_extension_offset ] != 0 ? file_extension : L" " ), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof( SHFILEINFO ), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON );

				if ( destroy )
				{
					_CoUninitialize();
				}
			}

			ii = ( ICON_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( ICON_INFO ) );
			if ( ii != NULL )
			{
				ii->file_extension = GlobalStrDupW( file_extension );
				ii->icon = sfi.hIcon;

				ii->count = 1;

				if ( dllrbt_insert( g_icon_handles, ( void * )ii->file_extension, ( void * )ii ) != DLLRBT_STATUS_OK )
				{
					_DestroyIcon( ii->icon );
					GlobalFree( ii->file_extension );
					GlobalFree( ii );
					ii = NULL;
				}
			}
		}
		else
		{
			if ( sfi.hIcon != NULL )
			{
				_DestroyIcon( sfi.hIcon );
			}

			++( ii->count );
		}
		LeaveCriticalSection( &icon_cache_cs );
	}

	return ii;
}

void RemoveCachedIcon( DOWNLOAD_INFO *di, wchar_t *file_path, unsigned int filename_offset, unsigned int file_extension_offset )
{
	if ( di != NULL )
	{
		if ( file_path == NULL )
		{
			file_path = di->file_path;
			filename_offset = di->filename_offset;
			file_extension_offset = di->file_extension_offset;
		}

		wchar_t full_file_path[ MAX_PATH ];
		wchar_t *file_extension = file_path + file_extension_offset;

		dllrbt_iterator *itr = NULL;

		EnterCriticalSection( &icon_cache_cs );

		if ( cfg_show_embedded_icon && lstrcmpiW( file_extension, L".exe" ) == 0 )
		{
			bool find_full_file_path = true;

			itr = dllrbt_find( g_icon_handles, ( void * )file_extension, false );
			if ( itr != NULL )
			{
				ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
				if ( ii != NULL && &ii->icon == di->icon )
				{
					find_full_file_path = false;
				}
			}

			if ( find_full_file_path )
			{
				_wmemcpy_s( full_file_path, MAX_PATH, file_path, MAX_PATH );
				if ( filename_offset > 0 )
				{
					full_file_path[ filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
				}

				// Find the icon info
				itr = dllrbt_find( g_icon_handles, ( void * )full_file_path, false );
			}
		}

		if ( itr == NULL )
		{
			// Find the icon info
			itr = dllrbt_find( g_icon_handles, ( void * )file_extension, false );
		}

		// Free its values and remove it from the tree if there are no other items using it.
		if ( itr != NULL )
		{
			ICON_INFO *ii = ( ICON_INFO * )( ( node_type * )itr )->val;
			if ( ii != NULL )
			{
				if ( --ii->count == 0 )
				{
					_DestroyIcon( ii->icon );
					GlobalFree( ii->file_extension );
					GlobalFree( ii );

					dllrbt_remove( g_icon_handles, itr );
				}
			}
			else
			{
				dllrbt_remove( g_icon_handles, itr );
			}
		}
		LeaveCriticalSection( &icon_cache_cs );
	}
}

void UpdateCachedIcon( DOWNLOAD_INFO *di )
{
	if ( di != NULL && !( di->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
	{
		wchar_t *file_extension = di->file_path + di->file_extension_offset;

		if ( lstrcmpiW( file_extension, L".exe" ) == 0 )
		{
			RemoveCachedIcon( di );

			// Cache our file's icon.
			ICON_INFO *ii = CacheIcon( di );

			di->icon = ( ii != NULL ? &ii->icon : NULL );
		}
	}
}

wchar_t *ParseURLSettings( wchar_t *url_list, ADD_INFO *ai )
{
	if ( ai != NULL )
	{
		wchar_t *category = NULL;
		wchar_t *download_directory = NULL;
		wchar_t *username = NULL;
		wchar_t *password = NULL;
		wchar_t *comments = NULL;
		wchar_t *cookies = NULL;
		wchar_t *data = NULL;
		wchar_t *headers = NULL;
		int headers_length = 0;
		wchar_t *proxy_hostname = NULL;
		int proxy_hostname_length = 0;
		wchar_t *proxy_username = NULL;
		wchar_t *proxy_password = NULL;

		while ( *url_list != 0 )
		{
			if ( *url_list == L'-' && *( url_list + 1 ) == L'-' )
			{
				url_list += 2;

				wchar_t *param_name_start = url_list;
				wchar_t *param_name_end = NULL;
				wchar_t *param_value_start = NULL;
				wchar_t *param_value_end = NULL;

				while ( *url_list != 0 )
				{
					if ( *url_list == L' ' && param_name_end == NULL )
					{
						param_name_end = url_list;
						*param_name_end = 0;	// This will be safe to zero out.

						++url_list;

						param_value_start = url_list;
					}
					else if ( *url_list == L'\r' && *( url_list + 1 ) == L'\n' )
					{
						if ( param_name_end == NULL )
						{
							param_name_end = url_list;
						}
						else
						{
							param_value_end = url_list;
							*param_value_end = 0;	// This will be safe to zero out.
						}

						url_list += 2;

						break;
					}
					
					++url_list;
				}

				if ( param_name_end == NULL )
				{
					param_name_end = url_list;
				}
				else if ( param_value_start != NULL && param_value_end == NULL )
				{
					param_value_end = url_list;
				}

				unsigned int param_name_length = ( unsigned int )( param_name_end - param_name_start );

				if ( param_name_length == 5 && _StrCmpNIW( param_name_start, L"parts", 5 ) == 0 )	// Split download into parts.
				{
					if ( param_value_start != NULL )
					{
						ai->use_parts = true;

						ai->parts = ( unsigned char )_wcstoul( param_value_start, NULL, 10 );
						if ( ai->parts > 100 )
						{
							ai->parts = 100;
						}
						else if ( ai->parts == 0 )
						{
							ai->parts = 1;
						}
					}
				}
				else if (  param_name_length == 11 && _StrCmpNIW( param_name_start, L"speed-limit", 11 ) == 0 )	// Download speed limit.
				{
					if ( param_value_start != NULL )
					{
						ai->use_download_speed_limit = true;

						ai->download_speed_limit = ( unsigned long long )wcstoull( param_value_start );
					}
				}
				else if ( param_name_length == 10 && _StrCmpNIW( param_name_start, L"encryption", 10 ) == 0 )	// SSL / TLS version.
				{
					if ( param_value_start != NULL )
					{
						ai->ssl_version = ( unsigned char )_wcstoul( param_value_start, NULL, 10 );
						if ( g_can_use_tls_1_3 && ai->ssl_version >= 6 )
						{
							ai->ssl_version = 6;	// TLS 1.3
						}
						else if ( ai->ssl_version > 5 )
						{
							ai->ssl_version = 5;	// TLS 1.2
						}
					}
				}
				else if ( param_name_length == 8 && _StrCmpNIW( param_name_start, L"simulate", 8 ) == 0 )	// Simulate the download.
				{
					ai->download_operations |= DOWNLOAD_OPERATION_SIMULATE;
				}
				else if ( param_name_length == 11 && _StrCmpNIW( param_name_start, L"add-stopped", 11 ) == 0 )	// Add the download in the Stopped state.
				{
					ai->download_operations &= ~DOWNLOAD_OPERATION_VERIFY;
					ai->download_operations |= DOWNLOAD_OPERATION_ADD_STOPPED;
				}
				else if ( param_name_length == 6 && _StrCmpNIW( param_name_start, L"verify", 6 ) == 0 )	// Verify the URL can be downloaded.
				{
					ai->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
					ai->download_operations |= DOWNLOAD_OPERATION_VERIFY;
				}
				else if ( ( param_name_length == 8 && _StrCmpNIW( param_name_start, L"comments", 8 ) == 0 ) ||
						  ( param_name_length == 13 && _StrCmpNIW( param_name_start, L"cookie-string", 13 ) == 0 ) ||
						  ( param_name_length == 9 && _StrCmpNIW( param_name_start, L"post-data", 9 ) == 0 ) ||
						  ( param_name_length == 8 && _StrCmpNIW( param_name_start, L"category", 8 ) == 0 ) ||
						  ( param_name_length == 18 && _StrCmpNIW( param_name_start, L"download-directory", 18 ) == 0 ) ||
						  ( param_name_length == 8 && _StrCmpNIW( param_name_start, L"username", 8 ) == 0 ) ||
						  ( param_name_length == 8 && _StrCmpNIW( param_name_start, L"password", 8 ) == 0 ) ||
						  ( param_name_length == 14 && _StrCmpNIW( param_name_start, L"proxy-hostname", 14 ) == 0 ) ||
						  ( param_name_length == 14 && _StrCmpNIW( param_name_start, L"proxy-username", 14 ) == 0 ) ||
						  ( param_name_length == 14 && _StrCmpNIW( param_name_start, L"proxy-password", 14 ) == 0 ) )
				{
					if ( param_value_start != NULL )
					{
						wchar_t **param_value = NULL;
						int param_value_length = ( int )( param_value_end - param_value_start );
						if ( param_value_length > 0 )
						{
							if ( *param_name_start == L'c' )
							{
								if ( param_name_start[ 1 ] == L'o' && param_name_start[ 2 ] == L'm' )	// Comments
								{
									param_value = &comments;
								}
								else if ( param_name_start[ 1 ] == L'o' && param_name_start[ 2 ] == L'o' )	// Cookies
								{
									param_value = &cookies;
								}
								else	// Category
								{
									param_value = &category;
								}
							}
							else if ( param_name_start[ 0 ] == L'p' )
							{
								if ( param_name_start[ 1 ] == L'o' )	// Post Data
								{
									param_value = &data;
								}
								else if ( param_name_start[ 6 ] == L'h' )	// Proxy Hostname
								{
									param_value = &proxy_hostname;
									proxy_hostname_length = param_value_length;
								}
								else if ( param_name_start[ 6 ] == L'u' )	// Proxy Username
								{
									param_value = &proxy_username;
								}
								else if ( param_name_start[ 6 ] == L'p' )	// Proxy Password
								{
									param_value = &proxy_password;
								}
								else	// Password
								{
									param_value = &password;
								}
							}
							else if ( *param_name_start == L'd' )
							{
								// Remove any quotes.
								if ( param_value_start[ 0 ] == L'\"' )
								{
									--param_value_length;
									++param_value_start;
								}
								if ( param_value_length > 0 && param_value_start[ param_value_length - 1 ] == L'\"' )
								{
									--param_value_length;
									param_value_start[ param_value_length ] = 0;	// Safe to zero out.
								}

								if ( CreateDirectoriesW( param_value_start, NULL ) != FALSE )
								{
									// Remove any trailing slash.
									while ( param_value_length > 0 )
									{
										if ( param_value_start[ param_value_length - 1 ] == L'\\' )
										{
											--param_value_length;
										}
										else
										{
											break;
										}
									}

									param_value = &download_directory;
								}
								else
								{
									continue;
								}
							}
							else if ( *param_name_start == L'u' )
							{
								param_value = &username;
							}

							if ( *param_value != NULL )
							{
								GlobalFree( *param_value );
								*param_value = NULL;
							}

							*param_value = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( param_value_length + 1 ) );
							_wmemcpy_s( *param_value, param_value_length + 1, param_value_start, param_value_length );
							( *param_value )[ param_value_length ] = 0;	// Sanity.
						}
					}
				}
				else if ( param_name_length == 12 && _StrCmpNIW( param_name_start, L"header-field", 12 ) == 0 )
				{
					if ( param_value_start != NULL )
					{
						int param_value_length = ( int )( param_value_end - param_value_start );
						if ( param_value_length > 0 )
						{
							if ( headers == NULL )
							{
								headers = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( param_value_length + 2 + 1 ) );
								_wmemcpy_s( headers + headers_length, param_value_length + 1, param_value_start, param_value_length );
								headers_length += param_value_length;
								headers[ headers_length++ ] = L'\r';
								headers[ headers_length++ ] = L'\n';
							}
							else
							{
								wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( headers, sizeof( wchar_t ) * ( headers_length + param_value_length + 2 + 1 ), GMEM_MOVEABLE );
								if ( realloc_buffer != NULL )
								{
									headers = realloc_buffer;

									_wmemcpy_s( headers + headers_length, param_value_length + 1, param_value_start, param_value_length );
									headers_length += param_value_length;
									headers[ headers_length++ ] = L'\r';
									headers[ headers_length++ ] = L'\n';
								}
							}

							headers[ headers_length ] = 0;	// Sanity.
						}
					}
				}
				else if ( param_name_length == 10 && _StrCmpNIW( param_name_start, L"proxy-type", 10 ) == 0 )	// Proxy type.
				{
					ai->proxy_info.type = ( unsigned char )_wcstoul( param_value_start, NULL, 10 );
				}
				else if ( param_name_length == 16 && _StrCmpNIW( param_name_start, L"proxy-ip-address", 16 ) == 0 )	// Proxy IP address.
				{
					if ( param_value_start != NULL )
					{
						unsigned int proxy_ip_address = 0;

						wchar_t *ipaddr = param_value_start;

						for ( char i = 3; i >= 0; --i )
						{
							wchar_t *ptr = ipaddr;
							while ( ptr != NULL && *ptr != NULL )
							{
								if ( *ptr == L'.' )
								{
									*ptr = 0;
									++ptr;

									break;
								}

								++ptr;
							}

							if ( *ptr == NULL && i > 0 )
							{
								break;
							}

							proxy_ip_address |= ( ( unsigned int )_wcstoul( ipaddr, NULL, 10 ) << ( 8 * i ) );

							if ( *ptr == NULL && i == 0 )
							{
								ai->proxy_info.ip_address = proxy_ip_address;
							}
							else
							{
								ipaddr = ptr;
							}
						}
					}

					ai->proxy_info.address_type = 1;
				}
				else if ( param_name_length == 10 && _StrCmpNIW( param_name_start, L"proxy-port", 10 ) == 0 )	// Proxy port.
				{
					ai->proxy_info.port = ( unsigned short )_wcstoul( param_value_start, NULL, 10 );
				}
				else if ( param_name_length == 26 && _StrCmpNIW( param_name_start, L"proxy-resolve-domain-names", 26 ) == 0 )	// Resolve domain names.
				{
					ai->proxy_info.resolve_domain_names = true;
				}

				continue;
			}
			else
			{
				break;
			}
		}

		if ( category != NULL )
		{
			ai->category = category;
		}

		if ( download_directory != NULL )
		{
			ai->use_download_directory = true;
			ai->download_directory = download_directory;
		}

		if ( username != NULL )
		{
			int utf8_length = WideCharToMultiByte( CP_UTF8, 0, username, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			ai->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, username, -1, ai->auth_info.username, utf8_length, NULL, NULL );

			GlobalFree( username );
		}

		if ( password != NULL )
		{
			int utf8_length = WideCharToMultiByte( CP_UTF8, 0, password, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			ai->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, password, -1, ai->auth_info.password, utf8_length, NULL, NULL );

			GlobalFree( password );
		}

		if ( comments != NULL )
		{
			ai->comments = comments;
		}

		if ( headers != NULL )
		{
			int utf8_length = WideCharToMultiByte( CP_UTF8, 0, headers, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			ai->utf8_headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, headers, -1, ai->utf8_headers, utf8_length, NULL, NULL );

			GlobalFree( headers );
		}

		if ( cookies != NULL )
		{
			int utf8_length = WideCharToMultiByte( CP_UTF8, 0, cookies, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			ai->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, cookies, -1, ai->utf8_cookies, utf8_length, NULL, NULL );

			GlobalFree( cookies );
		}

		if ( data != NULL )
		{
			ai->method = METHOD_POST;

			int utf8_length = WideCharToMultiByte( CP_UTF8, 0, data, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
			ai->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
			WideCharToMultiByte( CP_UTF8, 0, data, -1, ai->utf8_data, utf8_length, NULL, NULL );

			GlobalFree( data );
		}
		else
		{
			ai->method = METHOD_GET;
		}

		if ( ai->proxy_info.type != 0 )
		{
			if ( proxy_hostname != NULL )
			{
				ai->proxy_info.hostname = proxy_hostname;

				if ( normaliz_state == NORMALIZ_STATE_RUNNING )
				{
					int punycode_length = _IdnToAscii( 0, ai->proxy_info.hostname, proxy_hostname_length + 1, NULL, 0 );

					if ( punycode_length > ( proxy_hostname_length + 1 ) )
					{
						ai->proxy_info.punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
						_IdnToAscii( 0, ai->proxy_info.hostname, proxy_hostname_length + 1, ai->proxy_info.punycode_hostname, punycode_length );
					}
				}

				ai->proxy_info.address_type = 0;
			}

			if ( ai->proxy_info.port == 0 )
			{
				ai->proxy_info.port = ( ai->proxy_info.type == 1 ? 80 : ( ai->proxy_info.type == 2 ? 443 : 1080 ) );
			}

			int auth_length;

			if ( proxy_username != NULL )
			{
				ai->proxy_info.use_authentication = true;	// For SOCKS v5 connections.

				ai->proxy_info.w_username = proxy_username;

				auth_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, NULL, 0, NULL, NULL );
				ai->proxy_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, ai->proxy_info.username, auth_length, NULL, NULL );
			}

			if ( proxy_password != NULL )
			{
				ai->proxy_info.use_authentication = true;	// For SOCKS v5 connections.

				ai->proxy_info.w_password = proxy_password;

				auth_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, NULL, 0, NULL, NULL );
				ai->proxy_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, ai->proxy_info.password, auth_length, NULL, NULL );
			}
		}
		else
		{
			GlobalFree( proxy_hostname );
			GlobalFree( proxy_username );
			GlobalFree( proxy_password );
		}
	}

	return url_list;
}

DWORD WINAPI AddURL( void *add_info )
{
	if ( add_info == NULL )
	{
		_ExitThread( 0 );
		//return 0;
	}

	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	ProcessingList( true );

	//

	int total_item_count = 0;
	int expanded_item_count = 0;
	int root_item_count = 0;

	//

	SITE_INFO *si = NULL;
	SITE_INFO *last_si = NULL;

	ADD_INFO *ai = ( ADD_INFO * )add_info;
	ADD_INFO u_ai;	// Per URL add info.
	ADD_INFO g_ai;	// Per Group add info.

	wchar_t *url_list = ai->urls;

	wchar_t *host = NULL;
	wchar_t *resource = NULL;

	PROTOCOL protocol = PROTOCOL_UNKNOWN;
	unsigned short port = 0;

	unsigned int host_length = 0;
	unsigned int resource_length = 0;

	wchar_t *category = NULL;

	int category_length = 0;

	int ai_category_length = 0;

	wchar_t *download_directory = NULL;

	unsigned char parts;
	unsigned long long download_speed_limit;
	char ssl_version;

	unsigned int download_operations = DOWNLOAD_OPERATION_NONE;

	unsigned char method = METHOD_NONE;

	wchar_t *url_username = NULL;
	wchar_t *url_password = NULL;

	unsigned int url_username_length = 0;
	unsigned int url_password_length = 0;

	//

	char *username = NULL;
	char *password = NULL;

	int username_length = 0;
	int password_length = 0;

	int ai_username_length = 0;	// The original length from our auth_info struct.
	int ai_password_length = 0;

	//

	wchar_t *comments = NULL;

	int comments_length = 0;

	int ai_comments_length = 0;

	//

	char *cookies = NULL;
	char *headers = NULL;
	char *data = NULL;

	int cookies_length = 0;
	int headers_length = 0;
	int data_length = 0;

	int ai_cookies_length = 0;
	int ai_headers_length = 0;
	int ai_data_length = 0;

	//

	if ( ai->category != NULL )
	{
		category_length = ai_category_length = lstrlenW( ai->category );
	}

	if ( ai->auth_info.username != NULL )
	{
		username_length = ai_username_length = lstrlenA( ai->auth_info.username );
	}

	if ( ai->auth_info.password != NULL )
	{
		password_length = ai_password_length = lstrlenA( ai->auth_info.password );
	}

	if ( ai->comments != NULL )
	{
		comments_length = ai_comments_length = lstrlenW( ai->comments );
	}

	if ( ai->utf8_cookies != NULL )
	{
		cookies_length = ai_cookies_length = lstrlenA( ai->utf8_cookies );
	}

	if ( ai->utf8_headers != NULL )
	{
		headers_length = ai_headers_length = lstrlenA( ai->utf8_headers );
	}

	//

	PROXY_INFO proxy_info;
	_memzero( &proxy_info, sizeof( PROXY_INFO ) );

	int proxy_hostname_length = 0;
	int proxy_punycode_hostname_length = 0;

	int ai_proxy_hostname_length = 0;
	int ai_proxy_punycode_hostname_length = 0;

	int proxy_w_username_length = 0;
	int proxy_w_password_length = 0;

	int proxy_username_length = 0;
	int proxy_password_length = 0;

	int ai_proxy_username_length = 0;
	int ai_proxy_password_length = 0;

	int ai_proxy_w_username_length = 0;
	int ai_proxy_w_password_length = 0;

	//

	if ( ai->proxy_info.hostname != NULL )
	{
		proxy_hostname_length = ai_proxy_hostname_length = lstrlenW( ai->proxy_info.hostname );
	}

	if ( ai->proxy_info.punycode_hostname != NULL )
	{
		proxy_punycode_hostname_length = ai_proxy_punycode_hostname_length = lstrlenW( ai->proxy_info.punycode_hostname );
	}

	if ( ai->proxy_info.w_username != NULL )
	{
		proxy_w_username_length = ai_proxy_w_username_length = lstrlenW( ai->proxy_info.w_username );
	}

	if ( ai->proxy_info.w_password != NULL )
	{
		proxy_w_password_length = ai_proxy_w_password_length = lstrlenW( ai->proxy_info.w_password );
	}

	if ( ai->proxy_info.username != NULL )
	{
		proxy_username_length = ai_proxy_username_length = lstrlenA( ai->proxy_info.username );
	}

	if ( ai->proxy_info.password != NULL )
	{
		proxy_password_length = ai_proxy_password_length = lstrlenA( ai->proxy_info.password );
	}

	//

	if ( ai->ssl_version > 0 )
	{
		ssl_version = ai->ssl_version - 1;
	}
	else
	{
		ssl_version = -1;
	}

	parts = ai->parts;
	download_speed_limit = ai->download_speed_limit;

	if ( ai->method == METHOD_NONE )
	{
		ai->method = METHOD_GET;
	}

	method = ai->method;

	if ( ai->method == METHOD_POST )
	{
		if ( ai->utf8_data != NULL )
		{
			data_length = ai_data_length = lstrlenA( ai->utf8_data );
		}
	}

	TREELISTNODE *first_added_tln = NULL;
	TREELISTNODE *last_added_tln = NULL;

	while ( url_list != NULL )
	{
		// Stop processing and exit the thread.
		if ( kill_worker_thread_flag )
		{
			break;
		}

		wchar_t *group_start = NULL;
		wchar_t *group_end = NULL;
		wchar_t *next_url = NULL;	// The line after the group.

		// Ignore anything before our URL/group (spaces, tabs, newlines, etc.)
		while ( *url_list != 0 && ( *url_list == L'\r' ||
									*url_list == L'\n' ||
									*url_list == L' ' ||
									*url_list == L'\t' ||
									*url_list == L'\f' ) )
		{
			++url_list;
		}

		// See if we're overriding the filename.
		wchar_t *filename_start = url_list;
		wchar_t *filename_end;

		unsigned int w_filename_length = 0;

		if ( *filename_start == L'[' )
		{
			++filename_start;
			url_list = filename_start;

			wchar_t *url_start = _StrStrW( url_list, L"://" );
			if ( url_start != NULL )
			{
				// Walk back to find the end of the filename.
				while ( --url_start >= filename_start )
				{
					if ( *url_start == L']' )
					{
						filename_end = url_start;

						w_filename_length = ( unsigned int )( filename_end - filename_start );

						*url_start = 0;	// Sanity.

						EscapeFilename( filename_start );

						url_list = url_start + 1;

						if ( _StrCmpNIW( url_list, L"{\r\n", 3 ) == 0 )
						{
							group_start = url_list;
						}

						break;
					}
				}
			}
			else
			{
				break;
			}
		}

		unsigned group_count = 0;
		bool is_group = false;
		bool is_single_host_group = false;
		unsigned int group_item_count = 0;
		_memzero( &g_ai, sizeof( ADD_INFO ) );

		if ( group_start == NULL )
		{
			// Ignore anything before our URL/group (spaces, tabs, newlines, etc.)
			while ( *url_list != 0 && ( *url_list == L'\r' ||
										*url_list == L'\n' ||
										*url_list == L' ' ||
										*url_list == L'\t' ||
										*url_list == L'\f' ) )
			{
				++url_list;
			}

			if ( _StrCmpNIW( url_list, L"{\r\n", 3 ) == 0 )
			{
				group_start = url_list;
			}
		}

		if ( group_start != NULL )
		{
			group_start += 3;
			group_end = group_start;

			while ( group_end != NULL )
			{
				group_end = _StrChrW( group_end, L'}' );
				if ( group_end != NULL )
				{
					if ( *( group_end + 1 ) == NULL )
					{
						is_group = true;

						break;
					}
					else if ( *( group_end + 1 ) == L'\r' && *( group_end + 2 ) == L'\n' )
					{
						// Parse URL settings.
						next_url = ParseURLSettings( group_end + 3, &g_ai );

						if ( *next_url == NULL )
						{
							next_url = NULL;
						}

						is_group = true;

						break;
					}

					++group_end;
				}
			}

			if ( group_end == NULL )
			{
				break;	// Can't continue with the Add.
			}
		}

		TREELISTNODE *tln_parent = NULL;
		DOWNLOAD_INFO *shared_info = NULL;
		DOWNLOAD_INFO *di = NULL;

		if ( is_group )
		{
			url_list = group_start;
		}

		do
		{
			if ( is_group )
			{
				// Remove anything before our URL (spaces, tabs, newlines, etc.)
				while ( *url_list != 0 && url_list != group_end && ( ( *url_list != L'h' && *url_list != L'H' ) &&
																	 ( *url_list != L'f' && *url_list != L'F' ) &&
																	 ( *url_list != L's' && *url_list != L'S' ) ) )
				{
					++url_list;
				}

				if ( url_list == group_end )
				{
					break;
				}
			}
			else
			{
				// Remove anything before our URL (spaces, tabs, newlines, etc.)
				while ( *url_list != 0 && ( ( *url_list != L'h' && *url_list != L'H' ) &&
											( *url_list != L'f' && *url_list != L'F' ) &&
											( *url_list != L's' && *url_list != L'S' ) ) )
				{
					++url_list;
				}
			}

			// Find the end of the current url.
			wchar_t *current_url = url_list;

			int current_url_length = 0;

			bool decode_converted_resource = false;
			unsigned int white_space_count = 0;

			while ( *url_list != NULL )
			{
				if ( *url_list == L'%' )
				{
					decode_converted_resource = true;
				}
				else if ( *url_list == L' ' )
				{
					++white_space_count;
				}
				else if ( *url_list == L'\r' && *( url_list + 1 ) == L'\n' )
				{
					*url_list = 0;	// Sanity.

					current_url_length = ( int )( url_list - current_url );

					url_list += 2;

					break;
				}

				++url_list;
			}

			if ( *url_list == NULL )
			{
				current_url_length = ( int )( url_list - current_url );

				url_list = NULL;
			}

			// Remove whitespace at the end of our URL.
			while ( current_url_length > 0 )
			{
				if ( current_url[ current_url_length - 1 ] != L' ' && current_url[ current_url_length - 1 ] != L'\t' && current_url[ current_url_length - 1 ] != L'\f' )
				{
					break;
				}
				else
				{
					if ( current_url[ current_url_length - 1 ] == L' ' )
					{
						--white_space_count;
					}

					current_url[ current_url_length - 1 ] = 0;	// Sanity.
				}

				--current_url_length;
			}

			wchar_t *current_url_encoded = NULL;

			if ( white_space_count > 0 && *current_url != L'f' && *current_url != L'F' )
			{
				wchar_t *pstr = current_url;
				current_url_encoded = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( current_url_length + ( white_space_count * 2 ) + 1 ) );
				wchar_t *pbuf = current_url_encoded;

				while ( pstr < ( current_url + current_url_length ) )
				{
					if ( *pstr == L' ' )
					{
						pbuf[ 0 ] = L'%';
						pbuf[ 1 ] = L'2';
						pbuf[ 2 ] = L'0';

						pbuf = pbuf + 3;
					}
					else
					{
						*pbuf++ = *pstr;
					}

					++pstr;
				}

				*pbuf = L'\0';
			}

			last_si = ( last_si == ( SITE_INFO * )&u_ai ? NULL : si );

			_memzero( &u_ai, sizeof( ADD_INFO ) );

			if ( url_list != NULL )
			{
				wchar_t *t_url_list = url_list;

				// Parse URL settings.
				url_list = ParseURLSettings( url_list, &u_ai );

				if ( is_group )
				{
					++group_item_count;

					// Does the group download only contain one URL? If so, then it'll be treated as a non-group download.
					if ( group_item_count == 1 && *url_list == L'}' )
					{
						is_single_host_group = true;
					}
				}

				// We've moved the pointer of the URL list. Maybe we got some valid settings.
				if ( url_list != t_url_list )
				{
					// We're only using last_si to trigger the (si != last_si) conditional below.
					// The structure's variables are never used.
					last_si = ( SITE_INFO * )&u_ai;
				}

				if ( *url_list == NULL )
				{
					url_list = NULL;
				}
			}

			// Reset.
			protocol = PROTOCOL_UNKNOWN;
			host = NULL;
			resource = NULL;
			port = 0;

			host_length = 0;
			resource_length = 0;

			url_username = NULL;
			url_password = NULL;

			url_username_length = 0;
			url_password_length = 0;

			ParseURL_W( ( current_url_encoded != NULL ? current_url_encoded : current_url ), NULL, protocol, &host, host_length, port, &resource, resource_length, &url_username, &url_username_length, &url_password, &url_password_length );

			SITE_INFO tsi;
			tsi.host = host;
			tsi.protocol = protocol;
			tsi.port = port;
			si = ( SITE_INFO * )dllrbt_find( g_site_info, ( void * )&tsi, true );
			if ( si != NULL && !si->enable )
			{
				si = NULL;
			}

			// If we get the same site info, then the variables below will still be set.
			if ( si == NULL || si != last_si )
			{
				if ( u_ai.category != NULL )
				{
					category = u_ai.category;
					category_length = lstrlenW( category );
				}
				else if ( is_group && g_ai.category != NULL )
				{
					category = g_ai.category;
					category_length = lstrlenW( category );
				}
				else if ( ai->category != NULL )
				{
					category = ai->category;
					category_length = ai_category_length;
				}
				else if ( si != NULL && si->category != NULL )
				{
					category = si->category;
					category_length = lstrlenW( category );
				}
				else
				{
					category = NULL;
					category_length = 0;
				}

				CATEGORY_INFO_ *ci = NULL;
				if ( category != NULL )
				{
					CATEGORY_INFO_ t_ci;
					t_ci.category = category;
					ci = ( CATEGORY_INFO_ * )dllrbt_find( g_category_info, ( void * )&t_ci, true );
				}

				if ( u_ai.category != NULL && ci != NULL && ci->download_directory != NULL && !u_ai.use_download_directory )
				{
					download_directory = ci->download_directory;
				}
				else if ( u_ai.use_download_directory )
				{
					download_directory = u_ai.download_directory;
				}
				else if ( is_group && g_ai.category != NULL && ci != NULL && ci->download_directory != NULL && !g_ai.use_download_directory )
				{
					download_directory = ci->download_directory;
				}
				else if ( is_group && g_ai.use_download_directory )
				{
					download_directory = g_ai.download_directory;
				}
				else if ( ai->category != NULL && ci != NULL && ci->download_directory != NULL && !ai->use_download_directory )
				{
					download_directory = ci->download_directory;
				}
				else if ( ai->use_download_directory )
				{
					download_directory = ai->download_directory;
				}
				else if ( si != NULL && si->category != NULL && ci != NULL && ci->download_directory != NULL && !si->use_download_directory )
				{
					download_directory = ci->download_directory;
				}
				else if ( si != NULL && si->use_download_directory )
				{
					download_directory = si->download_directory;
				}
				else
				{
					download_directory = cfg_default_download_directory;
				}

				if ( u_ai.use_parts )
				{
					parts = u_ai.parts;
				}
				else if ( is_group && g_ai.use_parts )
				{
					parts = g_ai.parts;
				}
				else if ( ai->use_parts )
				{
					parts = ai->parts;
				}
				else if ( si != NULL && si->use_parts )
				{
					parts = si->parts;
				}
				else
				{
					parts = cfg_default_download_parts;
				}

				if ( u_ai.ssl_version > 0 )
				{
					ssl_version = u_ai.ssl_version - 1;
				}
				else if ( is_group && g_ai.ssl_version > 0 )
				{
					ssl_version = g_ai.ssl_version - 1;
				}
				else if ( ai->ssl_version > 0 )
				{
					ssl_version = ai->ssl_version - 1;
				}
				else if ( si != NULL && si->ssl_version > 0 )
				{
					ssl_version = si->ssl_version - 1;
				}
				else
				{
					ssl_version = cfg_default_ssl_version;
				}

				if ( u_ai.use_download_speed_limit )
				{
					download_speed_limit = u_ai.download_speed_limit;
				}
				/*else if ( is_group && g_ai.use_download_speed_limit )
				{
					download_speed_limit = g_ai.download_speed_limit;
				}*/
				else if ( ai->use_download_speed_limit )
				{
					download_speed_limit = ai->download_speed_limit;
				}
				else if ( si != NULL && si->use_download_speed_limit )
				{
					download_speed_limit = si->download_speed_limit;
				}
				else
				{
					download_speed_limit = cfg_default_speed_limit;
				}

				download_operations = u_ai.download_operations | g_ai.download_operations | ai->download_operations | ( si != NULL ? si->download_operations : DOWNLOAD_OPERATION_NONE );	// Include u_ai, ai and si together.

				if ( url_username != NULL )
				{
					username = NULL;
					username_length = 0;
				}
				else if ( u_ai.auth_info.username != NULL )
				{
					username = u_ai.auth_info.username;
					username_length = lstrlenA( username );
				}
				else if ( is_group && g_ai.auth_info.username != NULL )
				{
					username = g_ai.auth_info.username;
					username_length = lstrlenA( username );
				}
				else if ( ai->auth_info.username != NULL )
				{
					username = ai->auth_info.username;
					username_length = ai_username_length;
				}
				else if ( si != NULL && si->username != NULL )
				{
					username = si->username;
					username_length = lstrlenA( username );
				}
				else
				{
					username = NULL;
					username_length = 0;
				}

				if ( url_password != NULL )
				{
					password = NULL;
					password_length = 0;
				}
				else if ( u_ai.auth_info.password != NULL )
				{
					password = u_ai.auth_info.password;
					password_length = lstrlenA( password );
				}
				else if ( is_group && g_ai.auth_info.password != NULL )
				{
					password = g_ai.auth_info.password;
					password_length = lstrlenA( password );
				}
				else if ( ai->auth_info.password != NULL )
				{
					password = ai->auth_info.password;
					password_length = ai_password_length;
				}
				else if ( si != NULL && si->password != NULL )
				{
					password = si->password;
					password_length = lstrlenA( password );
				}
				else
				{
					password = NULL;
					password_length = 0;
				}

				if ( u_ai.comments != NULL )
				{
					comments = u_ai.comments;
					comments_length = lstrlenW( comments );
				}
				else if ( is_group && g_ai.comments != NULL )
				{
					comments = g_ai.comments;
					comments_length = lstrlenW( comments );
				}
				else if ( ai->comments != NULL )
				{
					comments = ai->comments;
					comments_length = ai_comments_length;
				}
				else if ( si != NULL && si->comments != NULL )
				{
					comments = si->comments;
					comments_length = lstrlenW( comments );
				}
				else
				{
					comments = NULL;
					comments_length = 0;
				}

				if ( u_ai.utf8_cookies != NULL )
				{
					cookies = u_ai.utf8_cookies;
					cookies_length = lstrlenA( cookies );
				}
				else if ( is_group && g_ai.utf8_cookies != NULL )
				{
					cookies = g_ai.utf8_cookies;
					cookies_length = lstrlenA( cookies );
				}
				else if ( ai->utf8_cookies != NULL )
				{
					cookies = ai->utf8_cookies;
					cookies_length = ai_cookies_length;
				}
				else if ( si != NULL && si->utf8_cookies != NULL )
				{
					cookies = si->utf8_cookies;
					cookies_length = lstrlenA( cookies );
				}
				else
				{
					cookies = NULL;
					cookies_length = 0;
				}

				if ( u_ai.utf8_headers != NULL )
				{
					headers = u_ai.utf8_headers;
					headers_length = lstrlenA( headers );
				}
				else if ( is_group && g_ai.utf8_headers != NULL )
				{
					headers = g_ai.utf8_headers;
					headers_length = lstrlenA( headers );
				}
				else if ( ai->utf8_headers != NULL )
				{
					headers = ai->utf8_headers;
					headers_length = ai_headers_length;
				}
				else if ( si != NULL && si->utf8_headers != NULL )
				{
					headers = si->utf8_headers;
					headers_length = lstrlenA( headers );
				}
				else
				{
					headers = NULL;
					headers_length = 0;
				}

				if ( u_ai.method == METHOD_POST )
				{
					method = METHOD_POST;

					data = u_ai.utf8_data;
					data_length = lstrlenA( data );
				}
				else if ( is_group && g_ai.method == METHOD_POST )
				{
					method = METHOD_POST;

					data = g_ai.utf8_data;
					data_length = lstrlenA( data );
				}
				else if ( ai->method == METHOD_POST )
				{
					method = METHOD_POST;

					data = ai->utf8_data;
					data_length = ai_data_length;
				}
				else if ( si != NULL && si->method == METHOD_POST )
				{
					method = METHOD_POST;

					data = si->utf8_data;
					data_length = lstrlenA( data );
				}
				else
				{
					method = METHOD_GET;

					data = NULL;
					data_length = 0;
				}

				//

				if ( u_ai.proxy_info.type != 0 )
				{
					proxy_info.type = u_ai.proxy_info.type;
					proxy_info.hostname = u_ai.proxy_info.hostname;
					proxy_info.punycode_hostname = u_ai.proxy_info.punycode_hostname;
					proxy_info.ip_address = u_ai.proxy_info.ip_address;
					proxy_info.port = u_ai.proxy_info.port;
					proxy_info.address_type = u_ai.proxy_info.address_type;
					proxy_info.type = u_ai.proxy_info.type;
					proxy_info.use_authentication = u_ai.proxy_info.use_authentication;
					proxy_info.w_username = u_ai.proxy_info.w_username;
					proxy_info.w_password = u_ai.proxy_info.w_password;
					proxy_info.username = u_ai.proxy_info.username;
					proxy_info.password = u_ai.proxy_info.password;
					proxy_info.resolve_domain_names = u_ai.proxy_info.resolve_domain_names;

					proxy_hostname_length = lstrlenW( u_ai.proxy_info.hostname );
					proxy_punycode_hostname_length = lstrlenW( u_ai.proxy_info.punycode_hostname );
					proxy_w_username_length = lstrlenW( u_ai.proxy_info.w_username );
					proxy_w_password_length = lstrlenW( u_ai.proxy_info.w_password );
					proxy_username_length = lstrlenA( u_ai.proxy_info.username );
					proxy_password_length = lstrlenA( u_ai.proxy_info.password );
				}
				else if ( is_group && g_ai.proxy_info.type != 0 )
				{
					proxy_info.type = g_ai.proxy_info.type;
					proxy_info.hostname = g_ai.proxy_info.hostname;
					proxy_info.punycode_hostname = g_ai.proxy_info.punycode_hostname;
					proxy_info.ip_address = g_ai.proxy_info.ip_address;
					proxy_info.port = g_ai.proxy_info.port;
					proxy_info.address_type = g_ai.proxy_info.address_type;
					proxy_info.type = g_ai.proxy_info.type;
					proxy_info.use_authentication = g_ai.proxy_info.use_authentication;
					proxy_info.w_username = g_ai.proxy_info.w_username;
					proxy_info.w_password = g_ai.proxy_info.w_password;
					proxy_info.username = g_ai.proxy_info.username;
					proxy_info.password = g_ai.proxy_info.password;
					proxy_info.resolve_domain_names = g_ai.proxy_info.resolve_domain_names;

					proxy_hostname_length = lstrlenW( g_ai.proxy_info.hostname );
					proxy_punycode_hostname_length = lstrlenW( g_ai.proxy_info.punycode_hostname );
					proxy_w_username_length = lstrlenW( g_ai.proxy_info.w_username );
					proxy_w_password_length = lstrlenW( g_ai.proxy_info.w_password );
					proxy_username_length = lstrlenA( g_ai.proxy_info.username );
					proxy_password_length = lstrlenA( g_ai.proxy_info.password );
				}
				else if ( ai->proxy_info.type != 0 )
				{
					proxy_info.type = ai->proxy_info.type;
					proxy_info.hostname = ai->proxy_info.hostname;
					proxy_info.punycode_hostname = ai->proxy_info.punycode_hostname;
					proxy_info.ip_address = ai->proxy_info.ip_address;
					proxy_info.port = ai->proxy_info.port;
					proxy_info.address_type = ai->proxy_info.address_type;
					proxy_info.type = ai->proxy_info.type;
					proxy_info.use_authentication = ai->proxy_info.use_authentication;
					proxy_info.w_username = ai->proxy_info.w_username;
					proxy_info.w_password = ai->proxy_info.w_password;
					proxy_info.username = ai->proxy_info.username;
					proxy_info.password = ai->proxy_info.password;
					proxy_info.resolve_domain_names = ai->proxy_info.resolve_domain_names;

					proxy_hostname_length = ai_proxy_hostname_length;
					proxy_punycode_hostname_length = ai_proxy_punycode_hostname_length;
					proxy_w_username_length = ai_proxy_w_username_length;
					proxy_w_password_length = ai_proxy_w_password_length;
					proxy_username_length = ai_proxy_username_length;
					proxy_password_length = ai_proxy_password_length;
				}
				else if ( si != NULL && si->proxy_info.type != 0 )
				{
					proxy_info.type = si->proxy_info.type;
					proxy_info.hostname = si->proxy_info.hostname;
					proxy_info.punycode_hostname = si->proxy_info.punycode_hostname;
					proxy_info.ip_address = si->proxy_info.ip_address;
					proxy_info.port = si->proxy_info.port;
					proxy_info.address_type = si->proxy_info.address_type;
					proxy_info.type = si->proxy_info.type;
					proxy_info.use_authentication = si->proxy_info.use_authentication;
					proxy_info.w_username = si->proxy_info.w_username;
					proxy_info.w_password = si->proxy_info.w_password;
					proxy_info.username = si->proxy_info.username;
					proxy_info.password = si->proxy_info.password;
					proxy_info.resolve_domain_names = si->proxy_info.resolve_domain_names;

					proxy_hostname_length = lstrlenW( si->proxy_info.hostname );
					proxy_punycode_hostname_length = lstrlenW( si->proxy_info.punycode_hostname );
					proxy_w_username_length = lstrlenW( si->proxy_info.w_username );
					proxy_w_password_length = lstrlenW( si->proxy_info.w_password );
					proxy_username_length = lstrlenA( si->proxy_info.username );
					proxy_password_length = lstrlenA( si->proxy_info.password );
				}
				else
				{
					proxy_info.type = 0;
				}
			}

			// The username and password could be encoded.
			if ( url_username != NULL )
			{
				int val_length = WideCharToMultiByte( CP_UTF8, 0, url_username, url_username_length + 1, NULL, 0, NULL, NULL );
				char *utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * val_length ); // Size includes the null character.
				WideCharToMultiByte( CP_UTF8, 0, url_username, url_username_length + 1, utf8_val, val_length, NULL, NULL );

				url_username_length = 0;
				username = url_decode_a( utf8_val, val_length - 1, &url_username_length );
				username_length = url_username_length;
				GlobalFree( utf8_val );

				//

				if ( url_password != NULL )
				{
					val_length = WideCharToMultiByte( CP_UTF8, 0, url_password, url_password_length + 1, NULL, 0, NULL, NULL );
					utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * val_length ); // Size includes the null character.
					WideCharToMultiByte( CP_UTF8, 0, url_password, url_password_length + 1, utf8_val, val_length, NULL, NULL );

					url_password_length = 0;
					password = url_decode_a( utf8_val, val_length - 1, &url_password_length );
					password_length = url_password_length;
					GlobalFree( utf8_val );
				}
				else
				{
					password = NULL;
					password_length = 0;
				}
			}

			if ( ( protocol != PROTOCOL_UNKNOWN && protocol != PROTOCOL_RELATIVE ) &&
				   host != NULL && resource != NULL && port != 0 )
			{
				bool set_shared_info;

				if ( shared_info == NULL )
				{
					shared_info = ( DOWNLOAD_INFO * )GlobalAlloc( GPTR, sizeof( DOWNLOAD_INFO ) );
					set_shared_info = true;
				}
				else
				{
					set_shared_info = false;
				}

				if ( is_group && !is_single_host_group )
				{
					di = ( DOWNLOAD_INFO * )GlobalAlloc( GPTR, sizeof( DOWNLOAD_INFO ) );
				}
				else
				{
					di = shared_info;
				}

				if ( di != NULL )
				{
					di->shared_info = shared_info;

					if ( di->shared_info != NULL )
					{
						InitializeCriticalSection( &di->di_cs );

						if ( set_shared_info )
						{
							shared_info->shared_info = shared_info;

							if ( is_group && !is_single_host_group )
							{
								InitializeCriticalSection( &shared_info->di_cs );

								if ( g_ai.use_download_speed_limit )
								{
									di->shared_info->download_speed_limit = g_ai.download_speed_limit;
								}
								else if ( ai->use_download_speed_limit )
								{
									di->shared_info->download_speed_limit = ai->download_speed_limit;
								}
								else
								{
									di->shared_info->download_speed_limit = cfg_default_speed_limit;
								}

								wchar_t *t_comments = NULL;
								int t_comments_length = 0;

								if ( g_ai.comments != NULL )
								{
									t_comments = g_ai.comments;
									t_comments_length = lstrlenW( comments );
								}
								else if ( ai->comments != NULL )
								{
									t_comments = ai->comments;
									t_comments_length = ai_comments_length;
								}

								if ( t_comments != NULL && t_comments_length > 0 )
								{
									di->shared_info->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( t_comments_length + 1 ) );
									_wmemcpy_s( di->shared_info->comments, t_comments_length + 1, t_comments, t_comments_length );
									di->shared_info->comments[ t_comments_length ] = 0;	// Sanity.
								}
							}

							if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
							{
								di->shared_info->filename_offset = lstrlenW( download_directory );
								_wmemcpy_s( di->shared_info->file_path, MAX_PATH, download_directory, di->shared_info->filename_offset );
								di->shared_info->file_path[ di->shared_info->filename_offset ] = 0;	// Sanity.

								++di->shared_info->filename_offset;	// Include the NULL terminator.
							}
							else
							{
								di->shared_info->filename_offset = 1;
							}

							if ( w_filename_length > 0 )
							{
								w_filename_length = min( w_filename_length, ( int )( MAX_PATH - di->shared_info->filename_offset - 1 ) );

								_wmemcpy_s( di->shared_info->file_path + di->shared_info->filename_offset, MAX_PATH - di->shared_info->filename_offset, filename_start, w_filename_length );
								di->shared_info->file_path[ di->shared_info->filename_offset + w_filename_length ] = 0;	// Sanity.

								di->shared_info->download_operations |= DOWNLOAD_OPERATION_OVERRIDE_FILENAME;
							}
							else
							{
								// Try to create a filename from the resource path.
								wchar_t *directory_ptr = resource;
								wchar_t *current_directory = resource;
								wchar_t *last_directory = NULL;

								// Iterate forward because '/' can be found after '#'.
								while ( *directory_ptr != NULL )
								{
									if ( *directory_ptr == L'?' || *directory_ptr == L'#' )
									{
										*directory_ptr = 0;	// Sanity.

										break;
									}
									else if ( *directory_ptr == L'/' )
									{
										last_directory = current_directory;
										current_directory = directory_ptr + 1; 
									}

									++directory_ptr;
								}

								if ( *current_directory == NULL )
								{
									// Adjust for '/'. current_directory will always be at least 1 greater than last_directory.
									if ( last_directory != NULL && ( current_directory - 1 ) - last_directory > 0 )
									{
										w_filename_length = ( unsigned int )( ( current_directory - 1 ) - last_directory );
										current_directory = last_directory;
									}
									else	// No filename could be made from the resource path. Use the host name instead.
									{
										w_filename_length = host_length;
										current_directory = host;

										di->shared_info->download_operations |= DOWNLOAD_OPERATION_GET_EXTENSION;
									}
								}
								else
								{
									w_filename_length = ( unsigned int )( directory_ptr - current_directory );
								}

								wchar_t *directory = NULL;

								// Why do we do this?
								// If the URL has UTF8 characters that have been URL encoded, then we need to convert them into a readable wide char format.
								if ( decode_converted_resource )
								{
									int val_length = WideCharToMultiByte( CP_UTF8, 0, current_directory, w_filename_length + 1, NULL, 0, NULL, NULL );
									char *utf8_val = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * val_length ); // Size includes the null character.
									WideCharToMultiByte( CP_UTF8, 0, current_directory, w_filename_length + 1, utf8_val, val_length, NULL, NULL );

									unsigned int directory_length = 0;
									char *c_directory = url_decode_a( utf8_val, val_length - 1, &directory_length );
									GlobalFree( utf8_val );

									val_length = MultiByteToWideChar( CP_UTF8, 0, c_directory, directory_length + 1, NULL, 0 );	// Include the NULL terminator.
									directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * val_length );
									w_filename_length = MultiByteToWideChar( CP_UTF8, 0, c_directory, directory_length + 1, directory, val_length ) - 1;

									GlobalFree( c_directory );	
								}
								else
								{
									directory = url_decode_w( current_directory, w_filename_length, &w_filename_length );
								}

								w_filename_length = min( w_filename_length, ( int )( MAX_PATH - di->shared_info->filename_offset - 1 ) );

								_wmemcpy_s( di->shared_info->file_path + di->shared_info->filename_offset, MAX_PATH - di->shared_info->filename_offset, directory, w_filename_length );
								di->shared_info->file_path[ di->shared_info->filename_offset + w_filename_length ] = 0;	// Sanity.

								EscapeFilename( di->shared_info->file_path + di->shared_info->filename_offset );

								GlobalFree( directory );
							}

							di->shared_info->file_extension_offset = di->shared_info->filename_offset + ( ( di->shared_info->download_operations & DOWNLOAD_OPERATION_GET_EXTENSION ) ? w_filename_length : get_file_extension_offset( di->shared_info->file_path + di->shared_info->filename_offset, w_filename_length ) );

							// Couldn't get an extension, try to get one from the Content-Type header field.
							if ( di->shared_info->file_extension_offset == ( di->shared_info->filename_offset + w_filename_length ) )
							{
								di->shared_info->download_operations |= DOWNLOAD_OPERATION_GET_EXTENSION;
							}

							if ( category == NULL || category_length == 0 )
							{
								if ( cfg_category_move )
								{
									unsigned int file_extension_offset = min( MAX_PATH - 1, di->shared_info->file_extension_offset + 1 );	// Exclude the period.

									// Find the category file extension info.
									CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )dllrbt_find( g_category_file_extensions, ( void * )( di->shared_info->file_path + file_extension_offset ), true );
									if ( cfei != NULL && cfei->ci != NULL )
									{
										if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
										{
											UpdateDownloadDirectoryInfo( di->shared_info, cfei->ci->download_directory, lstrlenW( cfei->ci->download_directory ) );
										}

										di->shared_info->category = CacheCategory( cfei->ci->category );
									}
								}
							}

							if ( di->shared_info->category == NULL && category != NULL && category_length > 0 )
							{
								di->shared_info->category = CacheCategory( category );
							}

							di->shared_info->hFile = INVALID_HANDLE_VALUE;

							//

							// Cache our file's icon.
							ICON_INFO *ii = CacheIcon( di->shared_info );

							if ( ii != NULL )
							{
								di->shared_info->icon = &ii->icon;
							}

							//

							SYSTEMTIME st;
							FILETIME ft;

							GetLocalTime( &st );
							SystemTimeToFileTime( &st, &ft );

							di->shared_info->add_time.LowPart = ft.dwLowDateTime;
							di->shared_info->add_time.HighPart = ft.dwHighDateTime;

							int buffer_length = 0;

							#ifndef NTDLL_USE_STATIC_LIB
								//buffer_length = 64;	// Should be enough to hold most translated values.
								buffer_length = __snwprintf( NULL, 0, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
							#else
								buffer_length = _scwprintf( L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) ) + 1;	// Include the NULL character.
							#endif

							di->shared_info->w_add_time = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * buffer_length );

							__snwprintf( di->shared_info->w_add_time, buffer_length, L"%s, %s %d, %04d %d:%02d:%02d %s", GetDay( st.wDayOfWeek ), GetMonth( st.wMonth ), st.wDay, st.wYear, ( st.wHour > 12 ? st.wHour - 12 : ( st.wHour != 0 ? st.wHour : 12 ) ), st.wMinute, st.wSecond, ( st.wHour >= 12 ? L"PM" : L"AM" ) );

							//

							di->shared_info->ssl_version = -1;
						}

						if ( current_url_encoded != NULL )
						{
							di->url = current_url_encoded;
							current_url_encoded = NULL;
						}
						else
						{
							di->url = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( current_url_length + 1 ) );
							_wmemcpy_s( di->url, current_url_length + 1, current_url, current_url_length );
							di->url[ current_url_length ] = 0;	// Sanity.
						}

						di->parts = parts;
						di->download_speed_limit = download_speed_limit;
						if ( protocol == PROTOCOL_SFTP )
						{
							di->ssl_version = -1;
						}
						else
						{
							di->ssl_version = ssl_version;
						}

						di->shared_info->download_operations |= download_operations;

						if ( download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
						{
							di->status = STATUS_STOPPED;	// Don't SetStatus() here.
							di->download_operations |= DOWNLOAD_OPERATION_ADD_STOPPED;
						}

						if ( username != NULL && username_length > 0 )
						{
							di->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( username_length + 1 ) );
							_memcpy_s( di->auth_info.username, username_length + 1, username, username_length );
							di->auth_info.username[ username_length ] = 0;	// Sanity.
						}

						if ( password != NULL && password_length > 0 )
						{
							di->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( password_length + 1 ) );
							_memcpy_s( di->auth_info.password, password_length + 1, password, password_length );
							di->auth_info.password[ password_length ] = 0;	// Sanity.
						}

						if ( comments != NULL && comments_length > 0 )
						{
							di->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( comments_length + 1 ) );
							_wmemcpy_s( di->comments, comments_length + 1, comments, comments_length );
							di->comments[ comments_length ] = 0;	// Sanity.
						}

						if ( cookies != NULL && cookies_length > 0 )
						{
							di->cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( cookies_length + 1 ) );
							_memcpy_s( di->cookies, cookies_length + 1, cookies, cookies_length );
							di->cookies[ cookies_length ] = 0;	// Sanity.
						}

						if ( headers != NULL && headers_length > 0 )
						{
							di->headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( headers_length + 1 ) );
							_memcpy_s( di->headers, headers_length + 1, headers, headers_length );
							di->headers[ headers_length ] = 0;	// Sanity.
						}

						di->method = method;

						if ( method == METHOD_POST )
						{
							if ( data != NULL && data_length > 0 )
							{
								di->data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( data_length + 1 ) );
								_memcpy_s( di->data, data_length + 1, data, data_length );
								di->data[ data_length ] = 0;	// Sanity.
							}
						}

						//

						if ( proxy_info.type != 0 )
						{
							di->proxy_info = di->saved_proxy_info = ( PROXY_INFO * )GlobalAlloc( GPTR, sizeof( PROXY_INFO ) );

							di->proxy_info->type = proxy_info.type;
							di->proxy_info->ip_address = proxy_info.ip_address;
							di->proxy_info->port = proxy_info.port;
							di->proxy_info->address_type = proxy_info.address_type;
							di->proxy_info->use_authentication = proxy_info.use_authentication;
							di->proxy_info->resolve_domain_names = proxy_info.resolve_domain_names;

							if ( proxy_info.hostname != NULL )
							{
								di->proxy_info->hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( proxy_hostname_length + 1 ) );
								_wmemcpy_s( di->proxy_info->hostname, proxy_hostname_length + 1, proxy_info.hostname, proxy_hostname_length + 1 );
							}
							else
							{
								di->proxy_info->hostname = NULL;
							}

							if ( proxy_info.punycode_hostname != NULL )
							{
								di->proxy_info->punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( proxy_punycode_hostname_length + 1 ) );
								_wmemcpy_s( di->proxy_info->punycode_hostname, proxy_punycode_hostname_length + 1, proxy_info.punycode_hostname, proxy_punycode_hostname_length + 1 );
							}
							else
							{
								di->proxy_info->punycode_hostname = NULL;
							}

							if ( proxy_info.w_username != NULL )
							{
								di->proxy_info->w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( proxy_w_username_length + 1 ) );
								_wmemcpy_s( di->proxy_info->w_username, proxy_w_username_length + 1, proxy_info.w_username, proxy_w_username_length + 1 );
							}
							else
							{
								di->proxy_info->w_username = NULL;
							}

							if ( proxy_info.w_password != NULL )
							{
								di->proxy_info->w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( proxy_w_password_length + 1 ) );
								_wmemcpy_s( di->proxy_info->w_password, proxy_w_password_length + 1, proxy_info.w_password, proxy_w_password_length + 1 );
							}
							else
							{
								di->proxy_info->w_password = NULL;
							}

							if ( proxy_info.username != NULL )
							{
								di->proxy_info->username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( proxy_username_length + 1 ) );
								_memcpy_s( di->proxy_info->username, proxy_username_length + 1, proxy_info.username, proxy_username_length + 1 );
							}
							else
							{
								di->proxy_info->username = NULL;
							}

							if ( proxy_info.password != NULL )
							{
								di->proxy_info->password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( proxy_password_length + 1 ) );
								_memcpy_s( di->proxy_info->password, proxy_password_length + 1, proxy_info.password, proxy_password_length + 1 );
							}
							else
							{
								di->proxy_info->password = NULL;
							}

							if ( ( proxy_info.type == 1 || proxy_info.type == 2 ) && ( di->proxy_info->username != NULL && di->proxy_info->password != NULL ) )
							{
								CreateBasicAuthorizationKey( di->proxy_info->username, proxy_username_length, di->proxy_info->password, proxy_password_length, &di->proxy_info->auth_key, &di->proxy_info->auth_key_length );
							}
							else
							{
								di->proxy_info->auth_key = NULL;
								di->proxy_info->auth_key_length = 0;
							}
						}

						//

						++di->shared_info->hosts;

						di->shared_info_node.data = di;
						DLL_AddNode( &di->shared_info->host_list, &di->shared_info_node, -1 );

						if ( tln_parent == NULL )
						{
							if ( is_group && !is_single_host_group )
							{
								// print_range_list is used in DrawTreeListView
								shared_info->print_range_list = shared_info->host_list;
							}

							if ( download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
							{
								shared_info->status = STATUS_STOPPED;	// Don't SetStatus() here.
							}

							tln_parent = ( TREELISTNODE * )GlobalAlloc( GPTR, sizeof( TREELISTNODE ) );
							shared_info->tln = tln_parent;
							tln_parent->data = shared_info;
							tln_parent->data_type = TLVDT_GROUP | ( ( !is_group || is_single_host_group ) ? TLVDT_HOST : 0 );

							TLV_AddNode( &g_tree_list, tln_parent, -1 );

							if ( g_status_filter != STATUS_NONE )
							{
								if ( IsFilterSet( shared_info, g_status_filter ) )
								{
									++total_item_count;
									++root_item_count;
									++expanded_item_count;
								}
							}
							else
							{
								++total_item_count;
								++root_item_count;
								++expanded_item_count;
							}

							if ( first_added_tln == NULL )
							{
								first_added_tln = tln_parent;
							}

							last_added_tln = tln_parent;
						}

						if ( di != shared_info )
						{
							di->shared_info->parts += parts;

							++tln_parent->child_count;

							TREELISTNODE *tln = ( TREELISTNODE * )GlobalAlloc( GPTR, sizeof( TREELISTNODE ) );
							tln->data = di;
							tln->data_type = TLVDT_HOST;
							tln->parent = tln_parent;

							TLV_AddNode( &tln_parent->child, tln, -1, true );

							if ( cfg_expand_added_group_items )
							{
								tln_parent->is_expanded = true;
							}

							if ( g_status_filter != STATUS_NONE )
							{
								if ( IsFilterSet( shared_info, g_status_filter ) )
								{
									++total_item_count;

									if ( tln_parent->is_expanded )
									{
										++expanded_item_count;
									}
								}
							}
							else
							{
								++total_item_count;

								if ( tln_parent->is_expanded )
								{
									++expanded_item_count;
								}
							}
						}

#ifdef ENABLE_LOGGING
						GenericLogEntry( di, LOG_INFO_ACTION, "Added URL" );
#endif
					}
					else
					{
						GlobalFree( di );
						di = NULL;
					}
				}
				else
				{
					GlobalFree( shared_info );
					shared_info = NULL;
				}
			}

			GlobalFree( current_url_encoded );
			GlobalFree( host );
			GlobalFree( resource );

			// If we got a username and password from the URL, then the username and password character strings were allocated and we need to free them.
			if ( url_username != NULL ) { GlobalFree( username ); GlobalFree( url_username ); }
			if ( url_password != NULL ) { GlobalFree( password ); GlobalFree( url_password ); }

			if ( u_ai.category != NULL ) { GlobalFree( u_ai.category ); }
			if ( u_ai.download_directory != NULL ) { GlobalFree( u_ai.download_directory ); }
			if ( u_ai.auth_info.username != NULL ) { GlobalFree( u_ai.auth_info.username ); }
			if ( u_ai.auth_info.password != NULL ) { GlobalFree( u_ai.auth_info.password ); }
			if ( u_ai.comments != NULL ) { GlobalFree( u_ai.comments ); }
			if ( u_ai.utf8_cookies != NULL ) { GlobalFree( u_ai.utf8_cookies ); }
			if ( u_ai.utf8_headers != NULL ) { GlobalFree( u_ai.utf8_headers ); }
			if ( u_ai.utf8_data != NULL ) { GlobalFree( u_ai.utf8_data ); }

			if ( u_ai.proxy_info.hostname != NULL ) { GlobalFree( u_ai.proxy_info.hostname ); }
			if ( u_ai.proxy_info.punycode_hostname != NULL ) { GlobalFree( u_ai.proxy_info.punycode_hostname ); }
			if ( u_ai.proxy_info.w_username != NULL ) { GlobalFree( u_ai.proxy_info.w_username ); }
			if ( u_ai.proxy_info.w_password != NULL ) { GlobalFree( u_ai.proxy_info.w_password ); }
			if ( u_ai.proxy_info.username != NULL ) { GlobalFree( u_ai.proxy_info.username ); }
			if ( u_ai.proxy_info.password != NULL ) { GlobalFree( u_ai.proxy_info.password ); }

			// Limit the total number of hosts in a group to 100.
			if ( ++group_count >= 100 )
			{
				group_end = NULL;
			}
		}
		while ( group_end != NULL && url_list != NULL && url_list < group_end );

		if ( is_group )
		{
			url_list = next_url;

			if ( g_ai.category != NULL ) { GlobalFree( g_ai.category ); }
			if ( g_ai.download_directory != NULL ) { GlobalFree( g_ai.download_directory ); }
			if ( g_ai.auth_info.username != NULL ) { GlobalFree( g_ai.auth_info.username ); }
			if ( g_ai.auth_info.password != NULL ) { GlobalFree( g_ai.auth_info.password ); }
			if ( g_ai.comments != NULL ) { GlobalFree( g_ai.comments ); }
			if ( g_ai.utf8_cookies != NULL ) { GlobalFree( g_ai.utf8_cookies ); }
			if ( g_ai.utf8_headers != NULL ) { GlobalFree( g_ai.utf8_headers ); }
			if ( g_ai.utf8_data != NULL ) { GlobalFree( g_ai.utf8_data ); }

			if ( g_ai.proxy_info.hostname != NULL ) { GlobalFree( g_ai.proxy_info.hostname ); }
			if ( g_ai.proxy_info.punycode_hostname != NULL ) { GlobalFree( g_ai.proxy_info.punycode_hostname ); }
			if ( g_ai.proxy_info.w_username != NULL ) { GlobalFree( g_ai.proxy_info.w_username ); }
			if ( g_ai.proxy_info.w_password != NULL ) { GlobalFree( g_ai.proxy_info.w_password ); }
			if ( g_ai.proxy_info.username != NULL ) { GlobalFree( g_ai.proxy_info.username ); }
			if ( g_ai.proxy_info.password != NULL ) { GlobalFree( g_ai.proxy_info.password ); }
		}
	}

	if ( cfg_tray_icon && cfg_enable_server && cfg_show_remote_connection_notification )
	{
		_SendMessageW( g_hWnd_main, WM_PEER_CONNECTED, ( WPARAM )total_item_count, ( LPARAM )ai->peer_info );
	}

	FreeAddInfo( &ai );

	TLV_AddRootItemCount( root_item_count );
	TLV_AddExpandedItemCount( expanded_item_count );
	TLV_AddTotalItemCount( total_item_count );

	if ( TLV_GetFirstVisibleItem() == NULL )
	{
		TLV_SetFirstVisibleRootIndex( 0 );
		TLV_SetFirstVisibleItem( g_tree_list );
		TLV_SetFirstVisibleIndex( 0 );
	}

	if ( !g_in_list_edit_mode &&
		 cfg_sort_added_and_updating_items &&
		 cfg_sorted_column_index != COLUMN_NUM )	// #
	{
		SORT_INFO sort_info;
		sort_info.column = cfg_sorted_column_index;
		sort_info.hWnd = g_hWnd_tlv_files;
		sort_info.direction = cfg_sorted_direction;

		_SendMessageW( g_hWnd_tlv_files, TLVM_SORT_ITEMS, NULL, ( LPARAM )&sort_info );
	}

	UpdateSBItemCount();

//	ProcessingList( false );
	UpdateMenus( true );										// Enable the appropriate menu items.
	_SendMessageW( g_hWnd_main, WM_CHANGE_CURSOR, FALSE, 0 );	// Reset the cursor.
	_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, ( cfg_scroll_to_last_item ? TRUE : FALSE ) );
	_SetFocus( g_hWnd_tlv_files );								// Give focus back to the listview to allow shortcut keys.

	// Process all of the items we've added.
	// This prevents the window from stalling while adding and starting a lot of items at the same time.
	while ( first_added_tln != NULL )
	{
		DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )first_added_tln->data;
		if ( di != NULL )
		{
			SetStatus( di, di->status );

			// If we're a group, then this gets the first host and starts it.
			// If not, then it's just a self reference.
			di = ( DOWNLOAD_INFO * )di->shared_info->host_list->data;

			/*if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
			{
				di->shared_info->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
			}*/

			DOWNLOAD_INFO *driver_di = NULL;

			// Find the first host that can act as a driver.
			DoublyLinkedList *host_node = di->shared_info->host_list;
			while ( host_node != NULL )
			{
				DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
				if ( !( host_di->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED ) )
				{
					driver_di = ( DOWNLOAD_INFO * )host_node->data;

					break;
				}
				host_node = host_node->next;
			}

			if ( driver_di != NULL )
			{
				StartDownload( driver_di, ( IS_GROUP( driver_di ) ? START_TYPE_GROUP : START_TYPE_NONE ), ( driver_di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ? START_OPERATION_NONE : START_OPERATION_CHECK_FILE ) );
			}

			g_download_history_changed = true;
		}

		if ( first_added_tln == last_added_tln )
		{
			break;
		}

		first_added_tln = first_added_tln->next;
	}

	_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, FALSE, FALSE );	// If the downloads were set to queued in StartDownload().
	if ( cfg_scroll_to_last_item )
	{
		UpdateMenus( true );											// Handles the last item if cfg_scroll_to_last_item is true.
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

void StartQueuedItem()
{
	EnterCriticalSection( &download_queue_cs );

	if ( download_queue != NULL )
	{
		DoublyLinkedList *download_queue_node = download_queue;

		DOWNLOAD_INFO *di;

		// Run through our download queue and start the first context that hasn't been paused or stopped.
		// Continue to dequeue if we haven't hit our maximum allowed active downloads.
		while ( download_queue_node != NULL )
		{
			di = ( DOWNLOAD_INFO * )download_queue_node->data;

			download_queue_node = download_queue_node->next;

			if ( di != NULL )
			{
				// Remove the item from the download queue.
				DLL_RemoveNode( &download_queue, &di->queue_node );
				di->queue_node.data = NULL;

				DoublyLinkedList *host_node = di->shared_info->host_list;
				DOWNLOAD_INFO *driver_di = NULL;

				LeaveCriticalSection( &download_queue_cs );

				bool is_group;

				if ( host_node != NULL && host_node->data != NULL )
				{
					is_group = IS_GROUP( ( ( DOWNLOAD_INFO * )host_node->data ) ) && !di->shared_info->processed_header;
				}
				else
				{
					is_group = false;
				}

				if ( is_group )
				{
					// Find the first host that can act as a driver.
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
				}
				else
				{
					driver_di = di;
				}

				if ( driver_di != NULL )
				{
					SetStatus( driver_di, STATUS_NONE );

					StartDownload( driver_di, ( is_group ? START_TYPE_GROUP : START_TYPE_NONE ), START_OPERATION_NONE );
				}
				else
				{
					// Remove the Add Stopped flag.
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

					EnterCriticalSection( &di->shared_info->di_cs );
					SetStatus( di->shared_info, STATUS_STOPPED );
					LeaveCriticalSection( &di->shared_info->di_cs );
				}

				EnterCriticalSection( &download_queue_cs );

				// Exit the loop if we've hit our maximum allowed active downloads.
				if ( g_total_downloading >= cfg_max_downloads )
				{
					break;
				}
			}
		}
	}

	LeaveCriticalSection( &download_queue_cs );
}

bool RetryTimedOut( SOCKET_CONTEXT *context )
{
	// Attempt to connect to a new address if we time out.
	if ( context != NULL &&
		 context->timed_out == TIME_OUT_RETRY && 
		 context->address_info != NULL &&
		 context->address_info->ai_next != NULL )
	{
		if ( context->socket != INVALID_SOCKET )
		{
			_shutdown( context->socket, SD_BOTH );
			_closesocket( context->socket );
			context->socket = INVALID_SOCKET;
		}

		if ( context->_ssl_s != NULL )
		{
			__SSL_free( context->_ssl_s );
			context->_ssl_s = NULL;
		}

		OpenSSL_FreeInfo( &context->_ssl_o );

		if ( context->address_info != NULL )
		{
			addrinfoW *old_address_info = context->address_info;
			context->address_info = context->address_info->ai_next;
			old_address_info->ai_next = NULL;

			_FreeAddrInfoW( old_address_info );
		}

		// If we're going to restart the download, then we need to reset these values.
		context->header_info.chunk_length = 0;
		context->header_info.end_of_header = NULL;
		context->header_info.http_status = 0;
		context->header_info.connection = CONNECTION_NONE;
		context->header_info.content_encoding = CONTENT_ENCODING_NONE;
		context->header_info.chunked_transfer = false;
		//context->header_info.etag = false;
		context->header_info.got_chunk_start = false;
		context->header_info.got_chunk_terminator = false;

		if ( context->header_info.range_info != NULL )
		{
			context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

			context->header_info.range_info->range_start += context->header_info.range_info->content_offset;	// Begin where we left off.
			context->header_info.range_info->content_offset = 0;	// Reset.
		}

		context->content_status = CONTENT_STATUS_NONE;

		context->timed_out = TIME_OUT_FALSE;

		context->status = STATUS_CONNECTING;

		context->cleanup = 0;	// Reset. Can only be set in CleanupConnection and if there's no more pending operations.

		// Connect to the remote server.
		if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
		{
			context->status = STATUS_FAILED;
		}
		else
		{
			return true;
		}
	}

	return false;
}

DWORD CALLBACK MoveFileProgress( LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER /*StreamSize*/, LARGE_INTEGER /*StreamBytesTransferred*/, DWORD /*dwStreamNumber*/, DWORD /*dwCallbackReason*/, HANDLE /*hSourceFile*/, HANDLE /*hDestinationFile*/, LPVOID lpData )
{
	DOWNLOAD_INFO *di = ( DOWNLOAD_INFO * )lpData;

	if ( di != NULL )
	{
		if ( di->shared_info->moving_state == 0 )
		{
			di->shared_info->moving_state = 1;	// Move file.
		}

		di->shared_info->last_downloaded = TotalBytesTransferred.QuadPart;

		if ( di->shared_info->moving_state == 2 )
		{
			di->shared_info->last_downloaded = TotalFileSize.QuadPart; // Reset.

			return PROGRESS_CANCEL;
		}
		else
		{
			return PROGRESS_CONTINUE;
		}
	}
	else
	{
		return PROGRESS_CANCEL;
	}
}

THREAD_RETURN ProcessMoveQueue( void * /*pArguments*/ )
{
	DOWNLOAD_INFO *di = NULL;

	bool skip_processing = false;

	wchar_t prompt_message[ MAX_PATH + 512 ];
	wchar_t file_path[ MAX_PATH ];

	wchar_t *old_file_path = NULL;
	wchar_t *new_file_path = NULL;
	int file_path_length = 0;
	int filename_length = 0;
	unsigned int filename_offset = 0;
	unsigned int file_extension_offset = 0;

	do
	{
		EnterCriticalSection( &move_file_queue_cs );

		DoublyLinkedList *move_file_queue_node = move_file_queue;

		if ( move_file_queue_node != NULL )
		{
			di = ( DOWNLOAD_INFO * )move_file_queue_node->data;

			DLL_RemoveNode( &move_file_queue, move_file_queue_node );
		}

		LeaveCriticalSection( &move_file_queue_cs );

		if ( di != NULL )
		{
			di->queue_node.data = NULL;

			if ( di->shared_info->new_file_path != NULL )
			{
				file_path_length = lstrlenW( di->shared_info->new_file_path );
				filename_length = lstrlenW( di->shared_info->file_path + di->shared_info->filename_offset );

				_wmemcpy_s( file_path, MAX_PATH, di->shared_info->new_file_path, file_path_length );
				file_path[ file_path_length ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.
				_wmemcpy_s( file_path + ( file_path_length + 1 ), MAX_PATH - ( file_path_length - 1 ), di->shared_info->file_path + di->shared_info->filename_offset, filename_length );
				file_path[ file_path_length + filename_length + 1 ] = 0;	// Sanity.

				old_file_path = di->shared_info->file_path;
				new_file_path = file_path;

				filename_offset = file_path_length + 1;
				file_extension_offset = filename_offset + ( int )( di->shared_info->file_extension_offset - di->shared_info->filename_offset );
			}
			else
			{
				GetTemporaryFilePath( di, file_path );

				old_file_path = file_path;
				new_file_path = di->shared_info->file_path;

				filename_offset = di->shared_info->filename_offset;
				file_extension_offset = di->shared_info->file_extension_offset;
			}

			di->shared_info->file_path[ di->shared_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.

			SetStatus( di->shared_info, di->shared_info->status & ~STATUS_QUEUED );

			bool resume_download = false;

			DWORD move_type = MOVEFILE_COPY_ALLOWED;

			for ( ;; )
			{
				if ( MoveFileWithProgressW( old_file_path, new_file_path, MoveFileProgress, di, move_type ) == FALSE )
				{
					DWORD gle = GetLastError();

					if ( gle == ERROR_ALREADY_EXISTS ||
						 gle == ERROR_FILE_EXISTS )	// Exists on same drive, exists on different drive.
					{
						if ( cfg_prompt_rename == 0 && di->shared_info->download_operations & DOWNLOAD_OPERATION_OVERRIDE_PROMPTS )
						{
							SetStatus( di->shared_info, STATUS_SKIPPED );
						}
						else
						{
							// If the last return value was not set to remember our choice, then prompt again.
							if ( cfg_prompt_rename == 0 &&
								 g_rename_file_cmb_ret != CMBIDRENAMEALL &&
								 g_rename_file_cmb_ret != CMBIDOVERWRITEALL &&
								 g_rename_file_cmb_ret != CMBIDSKIPALL )
							{
								if ( di->download_operations & DOWNLOAD_OPERATION_RENAME )	// We set this when adding the URL and received a prompt to rename/overwrite/skip.
								{
									g_rename_file_cmb_ret = CMBIDRENAME;
								}
								else if ( di->download_operations & DOWNLOAD_OPERATION_OVERWRITE )	// We set this when adding the URL and received a prompt to rename/overwrite/skip.
								{
									g_rename_file_cmb_ret = CMBIDOVERWRITE;
								}
								else
								{
									__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___already_exists, new_file_path );

									g_rename_file_cmb_ret = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_RENAMEOVERWRITESKIPALL );
								}
							}

							// Rename the file and try again.
							if ( cfg_prompt_rename == 1 ||
							   ( cfg_prompt_rename == 0 && ( g_rename_file_cmb_ret == CMBIDRENAME ||
															 g_rename_file_cmb_ret == CMBIDRENAMEALL ) ) )
							{
								// Creates a tree of active and queued downloads.
								dllrbt_tree *add_files_tree = CreateFilenameTree();

								bool rename_succeeded = RenameFile( add_files_tree,
																	new_file_path, &filename_offset, &file_extension_offset,
																	new_file_path, filename_offset, file_extension_offset );

								// The tree is only used to determine duplicate filenames.
								DestroyFilenameTree( add_files_tree );

								if ( !rename_succeeded )
								{
									// DOWNLOAD_OPERATION_RENAME = If we didn't show a prompt to rename when adding the URL, then prompt here.
									if ( g_rename_file_cmb_ret2 != CMBIDOKALL &&
									  !( di->shared_info->download_operations & ( DOWNLOAD_OPERATION_OVERRIDE_PROMPTS | DOWNLOAD_OPERATION_RENAME ) ) )
									{
										__snwprintf( prompt_message, MAX_PATH + 512, ST_V_PROMPT___could_not_be_renamed, file_path );

										g_rename_file_cmb_ret2 = CMessageBoxW( g_hWnd_main, prompt_message, PROGRAM_CAPTION, CMB_ICONWARNING | CMB_OKALL );
									}

									SetStatus( di->shared_info, STATUS_SKIPPED );
								}
								else
								{
									continue;	// Try the move with our new filename.
								}
							}
							else if ( cfg_prompt_rename == 3 ||
									( cfg_prompt_rename == 0 && ( g_rename_file_cmb_ret == CMBIDFAIL ||
																  g_rename_file_cmb_ret == CMBIDSKIP ||
																  g_rename_file_cmb_ret == CMBIDSKIPALL ) ) ) // Skip the rename or overwrite if the return value fails, or the user selected skip.
							{
								SetStatus( di->shared_info, STATUS_SKIPPED );
							}
							else	// Overwrite.
							{
								move_type |= MOVEFILE_REPLACE_EXISTING;

								continue;
							}
						}
					}
					else// if ( gle == ERROR_REQUEST_ABORTED )
					{
						// ERROR_FILE_NOT_FOUND might happen if we have it set to override and the same file is downloaded multiple times.

						SetStatus( di->shared_info, STATUS_STOPPED );
					}
					/*else
					{
						SetStatus( di->shared_info, STATUS_FILE_IO_ERROR );
					}*/
				}
				else
				{
					if ( di->shared_info->new_file_path != NULL )
					{
						_wmemcpy_s( di->shared_info->file_path, MAX_PATH, file_path, MAX_PATH );
						di->shared_info->file_path[ MAX_PATH - 1 ] = 0;	// Sanity.

						di->shared_info->filename_offset = filename_offset;
						di->shared_info->file_extension_offset = file_extension_offset;

						if ( IS_STATUS( di->shared_info->last_status, STATUS_CONNECTING | STATUS_DOWNLOADING ) )
						{
							SetStatus( di->shared_info, STATUS_NONE );

							resume_download = true;
						}
						else
						{
							SetStatus( di->shared_info, di->shared_info->last_status );
						}

						di->shared_info->last_status = STATUS_NONE;
					}
					else
					{
						SetStatus( di->shared_info, STATUS_COMPLETED );
					}
				}

				break;
			}

#ifdef ENABLE_LOGGING
			char log_status[ 256 ];
			GetDownloadStatus( log_status, 256, di->shared_info->status );
			WriteLog( LOG_INFO_ACTION, "Move file status: %s | %S | %S -> %S", log_status, di->url, file_path, di->shared_info->file_path );
#endif

			di->shared_info->file_path[ di->shared_info->filename_offset - 1 ] = 0;	// Restore.

			if ( cfg_show_embedded_icon )
			{
				UpdateCachedIcon( di->shared_info );
			}

			if ( di->shared_info->new_file_path != NULL )
			{
				GlobalFree( di->shared_info->new_file_path );
				di->shared_info->new_file_path = NULL;
			}

			if ( resume_download )
			{
				DoublyLinkedList *host_node = di->shared_info->host_list;

				while ( host_node != NULL )
				{
					DOWNLOAD_INFO *host_di = ( DOWNLOAD_INFO * )host_node->data;
					if ( host_di != NULL /*&& host_di != di*/ )
					{
						SetStatus( host_di, STATUS_NONE );

						StartDownload( host_di, START_TYPE_NONE, START_OPERATION_NONE );
					}

					host_node = host_node->next;
				}
			}
		}

		EnterCriticalSection( &move_file_queue_cs );

		if ( move_file_queue == NULL )
		{
			skip_processing = true;

			move_file_process_active = false;
		}

		LeaveCriticalSection( &move_file_queue_cs );
	}
	while ( !skip_processing );

	EnterCriticalSection( &cleanup_cs );

	if ( g_total_downloading == 0 )
	{
		EnableTimers( false );
	}

	LeaveCriticalSection( &cleanup_cs );

	_ExitThread( 0 );
	//return 0;
}

void AddToMoveFileQueue( DOWNLOAD_INFO *di )
{
	if ( di != NULL )
	{
		// Add item to move file queue and continue.
		EnterCriticalSection( &move_file_queue_cs );

		di->queue_node.data = di;
		DLL_AddNode( &move_file_queue, &di->queue_node, -1 );

		SetStatus( di->shared_info, STATUS_MOVING_FILE | STATUS_QUEUED );

		if ( !move_file_process_active )
		{
			move_file_process_active = true;

			HANDLE handle_move_queue = ( HANDLE )_CreateThread( NULL, 0, ProcessMoveQueue, NULL, 0, NULL );

			// Make sure our thread spawned.
			if ( handle_move_queue == NULL )
			{
				DLL_RemoveNode( &move_file_queue, &di->queue_node );
				di->queue_node.data = NULL;

				move_file_process_active = false;

				SetStatus( di->shared_info, STATUS_STOPPED );
			}
			else
			{
				CloseHandle( handle_move_queue );
			}
		}

		LeaveCriticalSection( &move_file_queue_cs );
	}
}

void ReallocateParts( SOCKET_CONTEXT *context )
{
	if ( context != NULL && context->download_info != NULL )
	{
		DOWNLOAD_INFO *di = context->download_info;

		// Find the context with the largest remaining part.
		DoublyLinkedList *parts_list = di->parts_list;
		SOCKET_CONTEXT *reallocated_context = NULL;
		unsigned long long remaining_size = 0;
		while ( parts_list != NULL )
		{
			SOCKET_CONTEXT *active_context = ( SOCKET_CONTEXT * )parts_list->data;
			if ( active_context->header_info.range_info != NULL &&
			   ( active_context->header_info.range_info->content_offset < ( ( active_context->header_info.range_info->range_end - active_context->header_info.range_info->range_start ) + 1 ) ) )
			{
				unsigned long long tmp_remaining_size = ( ( active_context->header_info.range_info->range_end - active_context->header_info.range_info->range_start ) + 1 ) - active_context->header_info.range_info->content_offset;
				if ( tmp_remaining_size > remaining_size )
				{
					remaining_size = tmp_remaining_size;
					reallocated_context = active_context;
				}
			}

			parts_list = parts_list->next;
		}

		// If the remaining size is greater than the threshold size, then we'll split it.
		if ( reallocated_context != NULL &&
			 reallocated_context->header_info.range_info != NULL &&
			 remaining_size > cfg_reallocate_threshold_size )
		{
			// Adjust each context's part number depending on which context we're splitting.
			parts_list = di->parts_list;
			while ( parts_list != NULL )
			{
				SOCKET_CONTEXT *active_context = ( SOCKET_CONTEXT * )parts_list->data;
				if ( context->part <= reallocated_context->part && active_context->part > context->part && active_context->part <= reallocated_context->part )
				{
					--active_context->part;
				}
				else if ( context->part > reallocated_context->part && active_context->part < context->part && active_context->part > reallocated_context->part )
				{
					++active_context->part;
				}

				parts_list = parts_list->next;
			}

			// Save the request information, the header information (if we got any), and create a new connection.
			SOCKET_CONTEXT *new_context = CreateSocketContext();
			if ( new_context != NULL )
			{
				new_context->processed_header = true;

				new_context->part = reallocated_context->part + 1;
				new_context->parts = reallocated_context->parts;

				new_context->got_filename = reallocated_context->got_filename;	// No need to rename it again.
				new_context->got_last_modified = reallocated_context->got_last_modified;	// No need to get the date/time again.
				new_context->show_file_size_prompt = reallocated_context->show_file_size_prompt;	// No need to prompt again.

				new_context->request_info.host = GlobalStrDupA( reallocated_context->request_info.host );
				new_context->request_info.port = reallocated_context->request_info.port;
				new_context->request_info.resource = GlobalStrDupA( reallocated_context->request_info.resource );
				new_context->request_info.protocol = reallocated_context->request_info.protocol;

				if ( reallocated_context->request_info.protocol == PROTOCOL_FTP ||
					 reallocated_context->request_info.protocol == PROTOCOL_FTPS ||
					 reallocated_context->request_info.protocol == PROTOCOL_FTPES )
				{
					new_context->ftp_connection_type = FTP_CONNECTION_TYPE_CONTROL;

					new_context->request_info.redirect_count = reallocated_context->request_info.redirect_count;	// This is being used to determine whether we've switched modes (fallback mode).
					new_context->header_info.connection = reallocated_context->header_info.connection;				// This is being used as our mode value. (cfg_ftp_mode_type)
				}
				else if ( reallocated_context->request_info.protocol == PROTOCOL_HTTP ||
						  reallocated_context->request_info.protocol == PROTOCOL_HTTPS )
				{
					new_context->request_info.auth_info.username = GlobalStrDupA( reallocated_context->request_info.auth_info.username );
					new_context->request_info.auth_info.password = GlobalStrDupA( reallocated_context->request_info.auth_info.password );

					new_context->header_info.cookie_tree = CopyCookieTree( reallocated_context->header_info.cookie_tree );
					new_context->header_info.cookies = GlobalStrDupA( reallocated_context->header_info.cookies );

					// We can copy the digest info so that we don't have to make any extra requests to 401 and 407 responses.
					if ( reallocated_context->header_info.digest_info != NULL )
					{
						new_context->header_info.digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );

						new_context->header_info.digest_info->algorithm = reallocated_context->header_info.digest_info->algorithm;
						new_context->header_info.digest_info->auth_type = reallocated_context->header_info.digest_info->auth_type;
						new_context->header_info.digest_info->qop_type = reallocated_context->header_info.digest_info->qop_type;

						new_context->header_info.digest_info->domain = GlobalStrDupA( reallocated_context->header_info.digest_info->domain );
						new_context->header_info.digest_info->nonce = GlobalStrDupA( reallocated_context->header_info.digest_info->nonce );
						new_context->header_info.digest_info->opaque = GlobalStrDupA( reallocated_context->header_info.digest_info->opaque );
						new_context->header_info.digest_info->qop = GlobalStrDupA( reallocated_context->header_info.digest_info->qop );
						new_context->header_info.digest_info->realm = GlobalStrDupA( reallocated_context->header_info.digest_info->realm );
					}

					if ( reallocated_context->header_info.proxy_digest_info != NULL )
					{
						new_context->header_info.proxy_digest_info = ( AUTH_INFO * )GlobalAlloc( GPTR, sizeof( AUTH_INFO ) );

						new_context->header_info.proxy_digest_info->algorithm = reallocated_context->header_info.proxy_digest_info->algorithm;
						new_context->header_info.proxy_digest_info->auth_type = reallocated_context->header_info.proxy_digest_info->auth_type;
						new_context->header_info.proxy_digest_info->qop_type = reallocated_context->header_info.proxy_digest_info->qop_type;

						new_context->header_info.proxy_digest_info->domain = GlobalStrDupA( reallocated_context->header_info.proxy_digest_info->domain );
						new_context->header_info.proxy_digest_info->nonce = GlobalStrDupA( reallocated_context->header_info.proxy_digest_info->nonce );
						new_context->header_info.proxy_digest_info->opaque = GlobalStrDupA( reallocated_context->header_info.proxy_digest_info->opaque );
						new_context->header_info.proxy_digest_info->qop = GlobalStrDupA( reallocated_context->header_info.proxy_digest_info->qop );
						new_context->header_info.proxy_digest_info->realm = GlobalStrDupA( reallocated_context->header_info.proxy_digest_info->realm );
					}
				}

				// Reuse the completed range_node from the current context.
				DoublyLinkedList *range_node = di->range_list;
				while ( range_node != NULL && range_node->data != context->header_info.range_info )
				{
					range_node = range_node->next;
				}

				RANGE_INFO *ri;

				// The completed range_node's next sibling needs to have its values adjusted.
				// Its start value will become the completed range_node's start value.
				// Its content offset will include everything between it and the completed range_node's start value.
				if ( range_node != NULL && range_node->next != NULL )
				{
					RANGE_INFO *completed_range_info = ( RANGE_INFO * )range_node->data;
					RANGE_INFO *next_range_info = ( RANGE_INFO * )range_node->next->data;

					next_range_info->content_offset += ( next_range_info->range_start - completed_range_info->range_start );
					next_range_info->range_start = completed_range_info->range_start;

					DLL_RemoveNode( &di->range_list, range_node );

					di->print_range_list = di->range_list;

					// We'll reuse the existing range node.
					ri = ( RANGE_INFO * )range_node->data;
				}
				else	// We can't apply the adjustments above to the last node since it has no next sibling, so we'll just create one.
				{
					ri = ( RANGE_INFO * )GlobalAlloc( GMEM_FIXED/*GPTR*/, sizeof( RANGE_INFO ) );
					if ( ri != NULL )
					{
						range_node = DLL_CreateNode( ( void * )ri );
					}
				}

				if ( ri != NULL )
				{
					ri->content_length = 0;
					ri->content_offset = 0;

					// Split the reallocated context's range_info.
					ri->range_end = reallocated_context->header_info.range_info->range_end;
					reallocated_context->header_info.range_info->range_end -= ( remaining_size / 2 );
					ri->range_start = reallocated_context->header_info.range_info->range_end + 1;
					ri->file_write_offset = ri->range_start;

					if ( reallocated_context->request_info.protocol != PROTOCOL_HTTP &&
						 reallocated_context->request_info.protocol != PROTOCOL_HTTPS )
					{
						ri->content_length = reallocated_context->header_info.range_info->content_length;
					}

					new_context->header_info.range_info = ri;

					//

					new_context->context_node.data = new_context;

					EnterCriticalSection( &context_list_cs );

					DLL_AddNode( &g_context_list, &new_context->context_node, 0 );

					LeaveCriticalSection( &context_list_cs );

					// Add to the parts list.
					if ( reallocated_context->download_info != NULL )
					{
						EnterCriticalSection( &reallocated_context->download_info->di_cs );

						new_context->download_info = reallocated_context->download_info;

						++( new_context->download_info->active_parts );

						// Find the reallocated context's range_info node.
						DoublyLinkedList *reallocated_range_node = reallocated_context->download_info->range_list;
						while ( reallocated_range_node != NULL && reallocated_range_node->data != reallocated_context->header_info.range_info )
						{
							reallocated_range_node = reallocated_range_node->next;
						}

						// Insert the new range_node into the range_list.
						if ( reallocated_range_node != NULL )
						{
							range_node->next = reallocated_range_node->next;
							if ( reallocated_range_node->next != NULL )
							{
								reallocated_range_node->next->prev = range_node;
							}
							else
							{
								new_context->download_info->range_list->prev = range_node;
							}
							range_node->prev = reallocated_range_node;
							reallocated_range_node->next = range_node;
						}
						else	// Shouldn't happen.
						{
							GlobalFree( range_node->data );
							GlobalFree( range_node );
						}

						new_context->parts_node.data = new_context;

						// Insert the new context into the parts_list.
						new_context->parts_node.next = reallocated_context->parts_node.next;
						if ( reallocated_context->parts_node.next != NULL )
						{
							reallocated_context->parts_node.next->prev = &new_context->parts_node;
						}
						else
						{
							new_context->download_info->parts_list->prev = &new_context->parts_node;
						}
						new_context->parts_node.prev = &reallocated_context->parts_node;
						reallocated_context->parts_node.next = &new_context->parts_node;

						LeaveCriticalSection( &reallocated_context->download_info->di_cs );

						EnterCriticalSection( &reallocated_context->download_info->shared_info->di_cs );

						// For groups.
						if ( IS_GROUP( reallocated_context->download_info ) )
						{
							++( reallocated_context->download_info->shared_info->active_parts );
						}

						LeaveCriticalSection( &reallocated_context->download_info->shared_info->di_cs );
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
	}
}

bool CleanupFTPContexts( SOCKET_CONTEXT *context )
{
	bool skip_cleanup = false;

	if ( context != NULL )
	{
		EnterCriticalSection( &cleanup_cs );

		// This forces the FTP control context to cleanup everything.
		if ( context->download_info != NULL )
		{
			DOWNLOAD_INFO *di = context->download_info;

			EnterCriticalSection( &di->di_cs );

			EnterCriticalSection( &context->context_cs );

			// We want the control port to handle everything.
			if ( context->ftp_context != NULL )
			{
				if ( context->ftp_connection_type & ( FTP_CONNECTION_TYPE_CONTROL | FTP_CONNECTION_TYPE_CONTROL_SUCCESS ) )	// Control context.
				{
					context->cleanup = 0;	// Reset.

					// Force the listen context to complete if it's still waiting.
					// This will have been set to INVALID_SOCKET if the AcceptEx completed.
					if ( context->ftp_connection_type == FTP_CONNECTION_TYPE_CONTROL &&
						 context->listen_socket != INVALID_SOCKET )
					{
						_shutdown( context->listen_socket, SD_BOTH );
						_closesocket( context->listen_socket );
						context->listen_socket = INVALID_SOCKET;
					}

					context->ftp_connection_type = ( FTP_CONNECTION_TYPE_CONTROL | FTP_CONNECTION_TYPE_CONTROL_WAIT );	// Wait for Data to finish.

					skip_cleanup = true;
				}
				else	// Data context.
				{
					if ( context->timed_out != TIME_OUT_FALSE )
					{
						// Force the Control connection to time out.
						// Make sure it's larger than 300 and less than LONG_MAX.
						InterlockedExchange( &context->ftp_context->timeout, SHRT_MAX );
					}

					if ( context->ftp_context->ftp_connection_type & FTP_CONNECTION_TYPE_CONTROL_WAIT )	// Control is waiting.
					{
						InterlockedIncrement( &context->ftp_context->pending_operations );

						context->ftp_context->overlapped_close.current_operation = IO_Close;	// No need to shutdown.

						PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context->ftp_context, ( OVERLAPPED * )&context->ftp_context->overlapped_close );
					}

					context->ftp_context->ftp_context = NULL;

					DLL_RemoveNode( &context->download_info->parts_list, &context->parts_node );

					context->download_info = NULL;
				}
			}

			LeaveCriticalSection( &context->context_cs );

			LeaveCriticalSection( &di->di_cs );
		}

		LeaveCriticalSection( &cleanup_cs );
	}

	return skip_cleanup;
}

bool SetCleanupState( SOCKET_CONTEXT *context )
{
	bool skip_cleanup = false;

	if ( context != NULL )
	{
		EnterCriticalSection( &context->context_cs );

		// If we've forced the cleanup, then skip everything below and wait for the pending operation to enter CleanupConnection to clean things up.
		if ( context->cleanup == 2 )
		{
			context->cleanup = 1;	// All pending operations will shutdown/close and enter CleanupConnection to clean things up.

			skip_cleanup = true;
		}
		else if ( context->cleanup == 12 )	// We forced the cleanup and are waiting for IO_Write to complete.
		{
			context->cleanup = 10;	// Let IO_Write do its thing.

			skip_cleanup = true;
		}
		else if ( context->cleanup == 10 )
		{
			skip_cleanup = true;
		}
		else	// 0 or 1
		{
			context->cleanup = 1;	// All pending operations will shutdown/close and enter CleanupConnection to clean things up.
		}

		LeaveCriticalSection( &context->context_cs );
	}

	return skip_cleanup;
}

void CleanupConnection( SOCKET_CONTEXT *context )
{
	if ( context != NULL )
	{
		// Returns true if there's pending operations that need to complete first.
		if ( SetCleanupState( context ) )
		{
			return;
		}

		// Returns true if we need to wait for the Data context to complete.
		if ( CleanupFTPContexts( context ) )
		{
			return;
		}

		// Check if our context timed out and if it has any additional addresses to connect to.
		// If it does, then reuse the context and connect to the new address.
		if ( RetryTimedOut( context ) )
		{
			return;
		}

		bool retry_context_connection = false;

		// This critical section must encompass the (context->download_info != NULL) section below so that any listview manipulation (like remove_items(...))
		// doesn't affect the queuing/starting proceedure.
		EnterCriticalSection( &cleanup_cs );

		EnterCriticalSection( &context_list_cs );

		// Remove from the global download list.
		DLL_RemoveNode( &g_context_list, &context->context_node );

		LeaveCriticalSection( &context_list_cs );

		// Remove from the parts list.
		if ( !g_end_program )
		{
			DOWNLOAD_INFO *di = context->download_info;
			if ( di != NULL )
			{
				bool update_menus = false;

				bool incomplete_part = false;

				if ( context->header_info.range_info != NULL &&  
				   ( context->header_info.range_info->content_offset < ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
				{
					incomplete_part = true;
				}

				// Don't retry the download if we're verifying it.
				bool verifying_download = ( di->shared_info->download_operations & DOWNLOAD_OPERATION_VERIFY ? true : false );

				// Connecting, Downloading, Paused.
				if ( !verifying_download &&
					  incomplete_part &&
					  context->retries < cfg_retry_parts_count &&
				    ( IS_STATUS( context->status,
						 STATUS_CONNECTING |
						 STATUS_DOWNLOADING ) ) )
				{
					++context->retries;

					if ( context->socket != INVALID_SOCKET )
					{
						_shutdown( context->socket, SD_BOTH );
						_closesocket( context->socket );
						context->socket = INVALID_SOCKET;
					}

					if ( context->_ssl_s != NULL )
					{
						__SSL_free( context->_ssl_s );
						context->_ssl_s = NULL;
					}

					OpenSSL_FreeInfo( &context->_ssl_o );

					EnterCriticalSection( &context_list_cs );

					DLL_AddNode( &g_context_list, &context->context_node, 0 );

					LeaveCriticalSection( &context_list_cs );

					// If we're going to restart the download, then we need to reset these values.
					context->header_info.chunk_length = 0;
					context->header_info.end_of_header = NULL;
					context->header_info.http_status = 0;
					context->header_info.connection = CONNECTION_NONE;
					context->header_info.content_encoding = CONTENT_ENCODING_NONE;
					context->header_info.chunked_transfer = false;
					//context->header_info.etag = false;
					context->header_info.got_chunk_start = false;
					context->header_info.got_chunk_terminator = false;

					if ( context->header_info.range_info != NULL )
					{
						context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

						context->header_info.range_info->range_start += context->header_info.range_info->content_offset;	// Begin where we left off.
						context->header_info.range_info->content_offset = 0;	// Reset.
					}

					context->content_status = CONTENT_STATUS_NONE;

					// Remember if we timed out in case we failed to connect.
					unsigned char timed_out = context->timed_out;

					context->timed_out = TIME_OUT_FALSE;

					context->status = STATUS_CONNECTING;

					context->cleanup = 0;	// Reset. Can only be set in CleanupConnection and if there's no more pending operations.

					// Connect to the remote server.
					if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
					{
						context->status = STATUS_FAILED;

						context->timed_out = timed_out;

						EnterCriticalSection( &context_list_cs );

						DLL_RemoveNode( &g_context_list, &context->context_node );

						LeaveCriticalSection( &context_list_cs );
					}
					else
					{
						retry_context_connection = true;
					}
				}

				if ( !retry_context_connection )
				{
					EnterCriticalSection( &download_queue_cs );

					// If the context we're cleaning up is in the download queue.
					if ( di->queue_node.data != NULL )
					{
						DLL_RemoveNode( &download_queue, &di->queue_node );
						di->queue_node.data = NULL;
					}

					LeaveCriticalSection( &download_queue_cs );

					EnterCriticalSection( &di->di_cs );

					DLL_RemoveNode( &di->parts_list, &context->parts_node );

					if ( di->active_parts > 0 )
					{
						DOWNLOAD_INFO *shared_info = di->shared_info;
						bool is_group = IS_GROUP( di );
						unsigned char shared_active_parts = 0;

						// If incomplete_part is tested below and is true and the new range fails, then the download will stop.
						// If incomplete_part is not tested, then all queued ranges will be tried until they either all succeed or all fail.
						if ( /*!incomplete_part &&*/
							 IS_STATUS_NOT( context->status,
								STATUS_STOPPED |
								STATUS_REMOVE |
								STATUS_RESTART |
								STATUS_UPDATING ) &&
							 di->range_queue != NULL &&
							 di->range_queue != di->range_list_end )
						{
							// Add back to the parts list.
							DLL_AddNode( &di->parts_list, &context->parts_node, -1 );

							DoublyLinkedList *range_queue_node = di->range_queue;
							di->range_queue = di->range_queue->next;

							context->retries = 0;

							if ( context->socket != INVALID_SOCKET )
							{
								_shutdown( context->socket, SD_BOTH );
								_closesocket( context->socket );
								context->socket = INVALID_SOCKET;
							}

							if ( context->_ssl_s != NULL )
							{
								__SSL_free( context->_ssl_s );
								context->_ssl_s = NULL;
							}

							OpenSSL_FreeInfo( &context->_ssl_o );

							EnterCriticalSection( &context_list_cs );

							DLL_AddNode( &g_context_list, &context->context_node, 0 );

							LeaveCriticalSection( &context_list_cs );

							// If we're going to restart the download, then we need to reset these values.
							context->header_info.chunk_length = 0;
							context->header_info.end_of_header = NULL;
							context->header_info.http_status = 0;
							context->header_info.connection = CONNECTION_NONE;
							context->header_info.content_encoding = CONTENT_ENCODING_NONE;
							context->header_info.chunked_transfer = false;
							//context->header_info.etag = false;
							context->header_info.got_chunk_start = false;
							context->header_info.got_chunk_terminator = false;

							context->header_info.range_info = ( RANGE_INFO * )range_queue_node->data;

							if ( context->header_info.range_info != NULL )
							{
								context->header_info.range_info->content_length = 0;	// We must reset this to get the real request length (not the length of the 401/407 request).

								context->header_info.range_info->range_start += context->header_info.range_info->content_offset;	// Begin where we left off.
								context->header_info.range_info->content_offset = 0;	// Reset.
							}

							context->content_status = CONTENT_STATUS_NONE;

							context->timed_out = TIME_OUT_FALSE;

							context->status = STATUS_CONNECTING;

							context->cleanup = 0;	// Reset. Can only be set in CleanupConnection and if there's no more pending operations.

							// Connect to the remote server.
							if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
							{
								context->status = STATUS_FAILED;

								EnterCriticalSection( &context_list_cs );

								DLL_RemoveNode( &g_context_list, &context->context_node );

								LeaveCriticalSection( &context_list_cs );

								--di->active_parts;

								++shared_active_parts;

								retry_context_connection = false;
							}
							else
							{
								retry_context_connection = true;
							}
						}
						else
						{
							--di->active_parts;

							++shared_active_parts;

							if ( cfg_reallocate_parts &&
								 di->parts > 1 && di->parts_limit != 1 &&
								 !incomplete_part &&
								 context->status == STATUS_DOWNLOADING )
							{
								ReallocateParts( context );
							}
						}

						// There are no more active connections.
						if ( di->active_parts == 0 )
						{
							bool move_file = false;

							TREELISTNODE *tln = TLV_GetFocusedItem();
							if ( tln == NULL )
							{
								TLV_GetNextSelectedItem( NULL, 0, &tln );
							}

							// Is this the currently focused/selected item?
							if ( tln != NULL && di == tln->data )
							{
								update_menus = true;
							}

							bool incomplete_download = false;

							// Go through our range list and see if any connections have not fully completed.
							DoublyLinkedList *range_node = di->range_list;
							while ( range_node != di->range_list_end )
							{
								RANGE_INFO *range_info = ( RANGE_INFO * )range_node->data;
								if ( range_info->content_offset < ( ( range_info->range_end - range_info->range_start ) + 1 ) )
								{
									incomplete_download = true;

									break;
								}
								
								range_node = range_node->next;
							}

							if ( incomplete_download )
							{
								// Connecting, Downloading, Paused.
								if ( IS_STATUS( context->status,
										STATUS_CONNECTING |
										STATUS_DOWNLOADING ) )
								{
									// If any of our connections timed out (after we have no more active connections), then set our status to timed out.
									if ( context->timed_out != TIME_OUT_FALSE )
									{
										SetStatus( di, STATUS_TIMED_OUT );
									}
									else
									{
										SetStatus( di, STATUS_STOPPED );
									}
								}
								else
								{
									incomplete_download = false;

									SetStatus( di, context->status & ~STATUS_DELETE );
								}
							}
							else if ( di->status != STATUS_FILE_IO_ERROR && di->status != STATUS_INSUFFICIENT_DISK_SPACE )
							{
								SetStatus( di, STATUS_COMPLETED );
							}

#ifdef ENABLE_LOGGING
							char log_status[ 256 ];
							GetDownloadStatus( log_status, 256, di->status );
							wchar_t *l_file_path;
							wchar_t t_l_file_path[ MAX_PATH ];
							bool is_temp = false;
							if ( di->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE )
							{
								l_file_path = L"Simulated";
							}
							else
							{
								if ( cfg_use_temp_download_directory ) { GetTemporaryFilePath( di, t_l_file_path ); is_temp = true; }
								else { GetDownloadFilePath( di, t_l_file_path ); }
								l_file_path = t_l_file_path;
							}
							WriteLog( LOG_INFO_CON_STATE, "Download cleanup status: %s | %s%S | %s%S", log_status, ( is_group ? "group | " : "" ), di->url, ( is_temp ? "temp | " : "" ), l_file_path );
#endif

							EnterCriticalSection( &active_download_list_cs );

							// Remove the node from the active download list.
							DLL_RemoveNode( &active_download_list, &di->download_node );
							di->download_node.data = NULL;

							di->last_downloaded = di->downloaded;

							--g_total_downloading;

							LeaveCriticalSection( &active_download_list_cs );

							di->time_remaining = 0;
							di->speed = 0;

							FILETIME ft;
							GetSystemTimeAsFileTime( &ft );
							ULARGE_INTEGER current_time;
							current_time.HighPart = ft.dwHighDateTime;
							current_time.LowPart = ft.dwLowDateTime;

							di->time_elapsed = ( current_time.QuadPart - di->start_time.QuadPart ) / FILETIME_TICKS_PER_SECOND;

							// Stop and Remove.
							if ( IS_STATUS( context->status, STATUS_REMOVE ) )
							{
								GlobalFree( di->url );
								GlobalFree( di->comments );
								//di->comments = NULL;	// This might be shared_info->comments. Set it to NULL so we don't double free.
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

								// Safe to free this here since the listview item will have been removed.
								while ( di->range_list != NULL )
								{
									range_node = di->range_list;
									di->range_list = di->range_list->next;

									GlobalFree( range_node->data );
									GlobalFree( range_node );
								}

								context->download_info = NULL;

								LeaveCriticalSection( &di->di_cs );

								EnterCriticalSection( &shared_info->di_cs );

								shared_info->print_range_list = NULL;
								DLL_RemoveNode( &shared_info->host_list, &di->shared_info_node );

								LeaveCriticalSection( &shared_info->di_cs );

								if ( is_group )
								{
									DeleteCriticalSection( &di->di_cs );

									GlobalFree( di );
								}
							}
							else if ( IS_STATUS( context->status, STATUS_RESTART ) )
							{
								if ( IS_STATUS( shared_info->status, STATUS_RESTART ) )
								{
									ResetDownload( di, START_TYPE_HOST, false );		// Reset host (free range info).

									if ( is_group )
									{
										// We've attempted to restart the group.
										if ( shared_info->active_hosts == 1 )
										{
											ResetDownload( di, START_TYPE_GROUP );
											StartDownload( ( DOWNLOAD_INFO * )di->shared_info->host_list->data, START_TYPE_GROUP, START_OPERATION_NONE );
										}
									}
									else
									{
										StartDownload( di, START_TYPE_HOST, START_OPERATION_NONE );
									}
								}
								else
								{
									RestartDownload( di, ( is_group ? START_TYPE_HOST_IN_GROUP : START_TYPE_HOST ), START_OPERATION_NONE );
								}

								LeaveCriticalSection( &di->di_cs );
							}
							else
							{
								if ( incomplete_download )
								{
									if ( !verifying_download &&
										  di->retries < cfg_retry_downloads_count )
									{
										++di->retries;

										SetStatus( di, STATUS_NONE );

										StartDownload( di, START_TYPE_NONE, START_OPERATION_NONE );
									}
									else
									{
										SetSessionStatusCount( di->status );
									}
								}
								else if ( IS_STATUS( di->status, STATUS_UPDATING ) )
								{
									if ( IS_STATUS_NOT( di->status, STATUS_MOVING_FILE ) )
									{
										SetStatus( di, STATUS_NONE );

										StartDownload( di, START_TYPE_NONE, START_OPERATION_NONE );
									}
									else
									{
										if ( IS_STATUS_NOT( shared_info->status, STATUS_UPDATING | STATUS_MOVING_FILE ) )
										{
											SetStatus( shared_info, STATUS_UPDATING | STATUS_MOVING_FILE );
										}

										move_file = true;
									}
								}
								else if ( di->status == STATUS_COMPLETED )
								{
									/*SetSessionStatusCount( di->status );

									if ( cfg_use_temp_download_directory &&
									  !( shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
									{
										AddToMoveFileQueue( di );
									}*/

									move_file = true;
								}
								else
								{
									SetSessionStatusCount( di->status );
								}

								LeaveCriticalSection( &di->di_cs );
							}

							bool free_shared_info = false;
							EnterCriticalSection( &shared_info->di_cs );

							// Remove the flag so we can resume downloading the file.
							shared_info->download_operations &= ~DOWNLOAD_OPERATION_VERIFY;

							// If we're verifying the download, then reset the range values so we can start at the beginning.
							if ( verifying_download )
							{
								range_node = shared_info->range_list;

								while ( range_node != NULL )
								{
									RANGE_INFO *ri = ( RANGE_INFO * )range_node->data;
									if ( ri != NULL )
									{
										ri->content_length = 0;
										ri->content_offset = 0;
										ri->range_start = 0;
										ri->range_end = 0;
										ri->file_write_offset = 0;
									}

									range_node = range_node->next;
								}
							}

							// For groups.
							if ( is_group )
							{
								shared_info->active_parts -= shared_active_parts;
							}

							--shared_info->active_hosts;

							if ( shared_info->active_hosts == 0 )
							{
								// Remove the flag.
								shared_info->download_operations &= ~( DOWNLOAD_OPERATION_RESTARTING | DOWNLOAD_OPERATION_RESUME );

								if ( move_file )
								{
									SetSessionStatusCount( di->status );

									if ( ( cfg_use_temp_download_directory || IS_STATUS( shared_info->status, STATUS_MOVING_FILE ) ) &&
									  !( shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
									{
										AddToMoveFileQueue( di );
									}
									else
									{
										move_file = false;
									}
								}

								if ( is_group )
								{
									if ( IS_STATUS( context->status, STATUS_REMOVE ) )
									{
										SetStatus( shared_info, STATUS_REMOVE );
									}
									else
									{
										SetSharedInfoStatus( shared_info );
									}

									EnterCriticalSection( &active_download_list_cs );

									// Remove the node from the active download list.
									DLL_RemoveNode( &active_download_list, &shared_info->download_node );
									shared_info->download_node.data = NULL;

									shared_info->last_downloaded = shared_info->downloaded;

									LeaveCriticalSection( &active_download_list_cs );

									shared_info->time_remaining = 0;
									shared_info->speed = 0;

									shared_info->time_elapsed = ( current_time.QuadPart - shared_info->start_time.QuadPart ) / FILETIME_TICKS_PER_SECOND;
								}
								else
								{
									if ( IS_STATUS( context->status, STATUS_REMOVE ) )
									{
										SetStatus( shared_info, STATUS_REMOVE );
									}
								}

								if ( shared_info->hFile != INVALID_HANDLE_VALUE )
								{
									CloseHandle( shared_info->hFile );
									shared_info->hFile = INVALID_HANDLE_VALUE;
								}

								if ( cfg_show_embedded_icon && !move_file )
								{
									UpdateCachedIcon( shared_info );
								}

								if ( IS_STATUS( context->status, STATUS_REMOVE ) )
								{
									RemoveCachedIcon( shared_info );
									RemoveCachedCategory( shared_info->category );
									if ( is_group )
									{
										GlobalFree( shared_info->comments );
									}
									GlobalFree( shared_info->new_file_path );
									GlobalFree( shared_info->w_add_time );

									free_shared_info = true;
								}

								// Do we want to delete the file as well?
								if ( !( shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) &&
									IS_STATUS( context->status, STATUS_DELETE ) )
								{
									wchar_t *file_path_delete;

									wchar_t file_path[ MAX_PATH + 1 ];
									if ( cfg_use_temp_download_directory )
									{
										GetTemporaryFilePath( shared_info, file_path );

										file_path_delete = file_path;
									}
									else
									{
										if ( free_shared_info )
										{
											// We're freeing this anyway so it's safe to modify.
											shared_info->file_path[ shared_info->filename_offset - 1 ] = L'\\';	// Replace the download directory NULL terminator with a directory slash.

											file_path_delete = shared_info->file_path;
										}
										else
										{
											GetDownloadFilePath( shared_info, file_path );

											file_path_delete = file_path;
										}
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

										_SHFileOperationW( &sfos );
									}
									else
									{
										DeleteFileW( file_path_delete );
									}

#ifdef ENABLE_LOGGING
									wchar_t *l_file_path;
									wchar_t t_l_file_path[ MAX_PATH ];
									bool is_temp = false;
									if ( cfg_use_temp_download_directory && shared_info->status != STATUS_COMPLETED ) { GetTemporaryFilePath( shared_info, t_l_file_path ); is_temp = true; }
									else { GetDownloadFilePath( shared_info, t_l_file_path ); }
									l_file_path = t_l_file_path;
									WriteLog( LOG_INFO_ACTION, "Deleting: %s%S", ( is_temp ? "temp | " : "" ), l_file_path );
#endif
								}
							}

							LeaveCriticalSection( &shared_info->di_cs );

							if ( free_shared_info )
							{
								DeleteCriticalSection( &shared_info->di_cs );

								GlobalFree( shared_info );
							}
						}
						else
						{
							LeaveCriticalSection( &di->di_cs );

							EnterCriticalSection( &shared_info->di_cs );

							if ( is_group )
							{
								shared_info->active_parts -= shared_active_parts;
							}

							LeaveCriticalSection( &shared_info->di_cs );
						}
					}
					else
					{
						LeaveCriticalSection( &di->di_cs );
					}
				}

				// The currently focused or first selected item has finished downloading.
				if ( update_menus )
				{
					UpdateMenus( true );
				}
			}
		}

		if ( context->update_status != 0 )
		{
			EnterCriticalSection( &update_check_timeout_cs );

			g_waiting_for_update = false;

			// Release the semaphore to complete the update check.
			if ( g_update_semaphore != NULL )
			{
				ReleaseSemaphore( g_update_semaphore, 1, NULL );
			}

			LeaveCriticalSection( &update_check_timeout_cs );
		}

		if ( !retry_context_connection )
		{
			if ( context->socket != INVALID_SOCKET )
			{
				_shutdown( context->socket, SD_BOTH );
				_closesocket( context->socket );
				context->socket = INVALID_SOCKET;
			}

			if ( context->listen_socket != INVALID_SOCKET )
			{
				_shutdown( context->listen_socket, SD_BOTH );
				_closesocket( context->listen_socket );
				context->listen_socket = INVALID_SOCKET;
			}

			if ( context->_ssl_s != NULL ) { __SSL_free( context->_ssl_s ); }

			OpenSSL_FreeInfo( &context->_ssl_o );

			if ( context->ssh != NULL && psftp_state == PSFTP_STATE_RUNNING ) { _SFTP_FreeSSHHandle( context->ssh ); }

			if ( context->address_info != NULL ) { _FreeAddrInfoW( context->address_info ); }
			if ( context->proxy_address_info != NULL ) { _FreeAddrInfoW( context->proxy_address_info ); }

			if ( context->decompressed_buf != NULL ) { GlobalFree( context->decompressed_buf ); }
			if ( zlib1_state == ZLIB1_STATE_RUNNING ) { _inflateEnd( &context->stream ); }

			FreePOSTInfo( &context->post_info );

			FreeAuthInfo( &context->header_info.digest_info );
			FreeAuthInfo( &context->header_info.proxy_digest_info );

			if ( context->header_info.url_location.host != NULL ) { GlobalFree( context->header_info.url_location.host ); }
			if ( context->header_info.url_location.resource != NULL ) { GlobalFree( context->header_info.url_location.resource ); }
			if ( context->header_info.url_location.auth_info.username != NULL ) { GlobalFree( context->header_info.url_location.auth_info.username ); }
			if ( context->header_info.url_location.auth_info.password != NULL ) { GlobalFree( context->header_info.url_location.auth_info.password ); }

			if ( context->header_info.chunk_buffer != NULL ) { GlobalFree( context->header_info.chunk_buffer ); }

			if ( context->header_info.cookies != NULL ) { GlobalFree( context->header_info.cookies ); }
			if ( context->header_info.cookie_tree != NULL )
			{
				node_type *node = dllrbt_get_head( context->header_info.cookie_tree );
				while ( node != NULL )
				{
					COOKIE_CONTAINER *cc = ( COOKIE_CONTAINER * )node->val;
					if ( cc != NULL )
					{
						GlobalFree( cc->cookie_name );
						GlobalFree( cc->cookie_value );
						GlobalFree( cc );
					}

					node = node->next;
				}

				dllrbt_delete_recursively( context->header_info.cookie_tree );
			}

			// Created for the Check for Update routine.
			if ( context->update_status != 0x00 && context->header_info.range_info != NULL ) { GlobalFree( context->header_info.range_info ); }

			if ( context->request_info.host != NULL ) { GlobalFree( context->request_info.host ); }
			if ( context->request_info.resource != NULL ) { GlobalFree( context->request_info.resource ); }

			if ( context->request_info.auth_info.username != NULL ) { GlobalFree( context->request_info.auth_info.username ); }
			if ( context->request_info.auth_info.password != NULL ) { GlobalFree( context->request_info.auth_info.password ); }

			// context->download_info is freed in WM_DESTROY.

			DeleteCriticalSection( &context->context_cs );

			if ( context->buffer != NULL ){ GlobalFree( context->buffer ); }
			if ( context->keep_alive_buffer != NULL ){ GlobalFree( context->keep_alive_buffer ); }

			GlobalFree( context );
		}

		LeaveCriticalSection( &cleanup_cs );
	}

	//

	if ( !g_end_program )
	{
		// Start any items that are in our download queue.
		if ( g_total_downloading < cfg_max_downloads )
		{
			StartQueuedItem();
		}

		EnterCriticalSection( &cleanup_cs );

		// Turn off our timers if we're not currently downloading, or moving anything.
		if ( g_total_downloading == 0 )
		{
			EnterCriticalSection( &move_file_queue_cs );

			if ( !move_file_process_active )
			{
				EnableTimers( false );
			}

			LeaveCriticalSection( &move_file_queue_cs );
		}

		LeaveCriticalSection( &cleanup_cs );
	}
}

void FreeProxyInfo( PROXY_INFO **proxy_info )
{
	if ( *proxy_info != NULL )
	{
		if ( ( *proxy_info )->hostname != NULL ) { GlobalFree( ( *proxy_info )->hostname ); }
		if ( ( *proxy_info )->punycode_hostname != NULL ) { GlobalFree( ( *proxy_info )->punycode_hostname ); }
		if ( ( *proxy_info )->w_username != NULL ) { GlobalFree( ( *proxy_info )->w_username ); }
		if ( ( *proxy_info )->w_password != NULL ) { GlobalFree( ( *proxy_info )->w_password ); }
		if ( ( *proxy_info )->username != NULL ) { GlobalFree( ( *proxy_info )->username ); }
		if ( ( *proxy_info )->password != NULL ) { GlobalFree( ( *proxy_info )->password ); }
		if ( ( *proxy_info )->auth_key != NULL ) { GlobalFree( ( *proxy_info )->auth_key ); }

		GlobalFree( *proxy_info );

		*proxy_info = NULL;
	}
}

void FreePOSTInfo( POST_INFO **post_info )
{
	if ( *post_info != NULL )
	{
		if ( ( *post_info )->method != NULL ) { GlobalFree( ( *post_info )->method ); }
		if ( ( *post_info )->urls != NULL ) { GlobalFree( ( *post_info )->urls ); }
		if ( ( *post_info )->directory != NULL ) { GlobalFree( ( *post_info )->directory ); }
		if ( ( *post_info )->parts != NULL ) { GlobalFree( ( *post_info )->parts ); }
		if ( ( *post_info )->ssl_tls_version != NULL ) { GlobalFree( ( *post_info )->ssl_tls_version ); }
		if ( ( *post_info )->username != NULL ) { GlobalFree( ( *post_info )->username ); }
		if ( ( *post_info )->password != NULL ) { GlobalFree( ( *post_info )->password ); }
		if ( ( *post_info )->download_speed_limit != NULL ) { GlobalFree( ( *post_info )->download_speed_limit ); }
		if ( ( *post_info )->download_operations != NULL ) { GlobalFree( ( *post_info )->download_operations ); }
		if ( ( *post_info )->cookies != NULL ) { GlobalFree( ( *post_info )->cookies ); }
		if ( ( *post_info )->headers != NULL ) { GlobalFree( ( *post_info )->headers ); }
		if ( ( *post_info )->data != NULL ) { GlobalFree( ( *post_info )->data ); }
		if ( ( *post_info )->proxy_type != NULL ) { GlobalFree( ( *post_info )->proxy_type ); }
		if ( ( *post_info )->proxy_hostname_ip != NULL ) { GlobalFree( ( *post_info )->proxy_hostname_ip ); }
		if ( ( *post_info )->proxy_port != NULL ) { GlobalFree( ( *post_info )->proxy_port ); }
		if ( ( *post_info )->proxy_username != NULL ) { GlobalFree( ( *post_info )->proxy_username ); }
		if ( ( *post_info )->proxy_password != NULL ) { GlobalFree( ( *post_info )->proxy_password ); }
		if ( ( *post_info )->proxy_resolve_domain_names != NULL ) { GlobalFree( ( *post_info )->proxy_resolve_domain_names ); }
		if ( ( *post_info )->proxy_use_authentication != NULL ) { GlobalFree( ( *post_info )->proxy_use_authentication ); }

		GlobalFree( *post_info );

		*post_info = NULL;
	}
}

void FreeAuthInfo( AUTH_INFO **auth_info )
{
	if ( *auth_info != NULL )
	{
		if ( ( *auth_info )->domain != NULL ) { GlobalFree( ( *auth_info )->domain ); }
		if ( ( *auth_info )->nonce != NULL ) { GlobalFree( ( *auth_info )->nonce ); }
		if ( ( *auth_info )->opaque != NULL ) { GlobalFree( ( *auth_info )->opaque ); }
		if ( ( *auth_info )->qop != NULL ) { GlobalFree( ( *auth_info )->qop ); }
		if ( ( *auth_info )->realm != NULL ) { GlobalFree( ( *auth_info )->realm ); }

		if ( ( *auth_info )->cnonce != NULL ) { GlobalFree( ( *auth_info )->cnonce ); }
		if ( ( *auth_info )->uri != NULL ) { GlobalFree( ( *auth_info )->uri ); }
		if ( ( *auth_info )->response != NULL ) { GlobalFree( ( *auth_info )->response ); }
		if ( ( *auth_info )->username != NULL ) { GlobalFree( ( *auth_info )->username ); }

		GlobalFree( *auth_info );

		*auth_info = NULL;
	}
}

void FreeAddInfo( ADD_INFO **add_info )
{
	if ( *add_info != NULL )
	{
		if ( ( *add_info )->peer_info != NULL ) { GlobalFree( ( *add_info )->peer_info ); }

		if ( ( *add_info )->proxy_info.hostname != NULL ) { GlobalFree( ( *add_info )->proxy_info.hostname ); }
		if ( ( *add_info )->proxy_info.punycode_hostname != NULL ) { GlobalFree( ( *add_info )->proxy_info.punycode_hostname ); }
		if ( ( *add_info )->proxy_info.w_username != NULL ) { GlobalFree( ( *add_info )->proxy_info.w_username ); }
		if ( ( *add_info )->proxy_info.w_password != NULL ) { GlobalFree( ( *add_info )->proxy_info.w_password ); }
		if ( ( *add_info )->proxy_info.username != NULL ) { GlobalFree( ( *add_info )->proxy_info.username ); }
		if ( ( *add_info )->proxy_info.password != NULL ) { GlobalFree( ( *add_info )->proxy_info.password ); }
		if ( ( *add_info )->proxy_info.auth_key != NULL ) { GlobalFree( ( *add_info )->proxy_info.auth_key ); }

		if ( ( *add_info )->comments != NULL ) { GlobalFree( ( *add_info )->comments ); }
		if ( ( *add_info )->utf8_data != NULL ) { GlobalFree( ( *add_info )->utf8_data ); }
		if ( ( *add_info )->utf8_headers != NULL ) { GlobalFree( ( *add_info )->utf8_headers ); }
		if ( ( *add_info )->utf8_cookies != NULL ) { GlobalFree( ( *add_info )->utf8_cookies ); }
		if ( ( *add_info )->auth_info.username != NULL ) { GlobalFree( ( *add_info )->auth_info.username ); }
		if ( ( *add_info )->auth_info.password != NULL ) { GlobalFree( ( *add_info )->auth_info.password ); }
		if ( ( *add_info )->download_directory != NULL ) { GlobalFree( ( *add_info )->download_directory ); }
		if ( ( *add_info )->category != NULL ) { GlobalFree( ( *add_info )->category ); }
		if ( ( *add_info )->urls != NULL ) { GlobalFree( ( *add_info )->urls ); }

		GlobalFree( *add_info );

		*add_info = NULL;
	}
}

// Free all context structures in the global list of context structures.
void FreeContexts()
{
	DoublyLinkedList *context_node = g_context_list;
	DoublyLinkedList *del_context_node = NULL;

	while ( context_node != NULL )
	{
		del_context_node = context_node;
		context_node = context_node->next;

		CleanupConnection( ( SOCKET_CONTEXT * )del_context_node->data );
	}

	g_context_list = NULL;
}

void FreeListenContext()
{
	if ( g_listen_context != NULL )
	{
#ifdef ENABLE_LOGGING
		WriteLog( LOG_INFO_MISC, "Shutting down web server" );
#endif

		CleanupConnection( g_listen_context );
		g_listen_context = NULL;
	}
}

THREAD_RETURN CheckForUpdates( void * /*pArguments*/ )
{
	// Only need one check going on at a time.
	if ( TryEnterCriticalSection( &worker_cs ) == TRUE )
	{
#ifdef ENABLE_LOGGING
		WriteLog( LOG_INFO_MISC, "Checking for updates" );
#endif
		in_worker_thread = true;

		SOCKET_CONTEXT *context = CreateSocketContext();

		if ( context != NULL )
		{
			unsigned int host_length = 0;
			unsigned int resource_length = 0;

			char *update_check_url;

#ifdef ENABLE_DARK_MODE
#ifdef IS_BETA
			update_check_url = DM_UPDATE_CHECK_URL_BETA;
#else
			update_check_url = DM_UPDATE_CHECK_URL;
#endif
#else
#ifdef IS_BETA
			update_check_url = UPDATE_CHECK_URL_BETA;
#else
			update_check_url = UPDATE_CHECK_URL;
#endif
#endif

			ParseURL_A( update_check_url, NULL, context->request_info.protocol, &context->request_info.host, host_length, context->request_info.port, &context->request_info.resource, resource_length, NULL, NULL, NULL, NULL );

			context->update_status = 0x01;	// Check for updates.

			context->part = 1;
			context->parts = 1;

			context->header_info.range_info = ( RANGE_INFO * )GlobalAlloc( GPTR, sizeof( RANGE_INFO ) );

			// Add to the global download list.
			context->context_node.data = context;

			EnterCriticalSection( &context_list_cs );

			DLL_AddNode( &g_context_list, &context->context_node, 0 );

			LeaveCriticalSection( &context_list_cs );

			//

			context->status = STATUS_CONNECTING;

			g_waiting_for_update = true;

			if ( !CreateConnection( context, context->request_info.host, context->request_info.port ) )
			{
				g_waiting_for_update = false;

				context->status = STATUS_FAILED;

				InterlockedIncrement( &context->pending_operations );

				context->overlapped.current_operation = IO_Close;

				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( OVERLAPPED * )&context->overlapped );
			}
			else
			{
				// This semaphore will be released when the update check is complete.
				g_update_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

				// Wait for any active threads to complete. 30 second timeout in case we miss the release.
				WaitForSingleObject( g_update_semaphore, 30000 );

				EnterCriticalSection( &update_check_timeout_cs );

				if ( g_waiting_for_update )
				{
					g_waiting_for_update = false;

					if ( context->socket != INVALID_SOCKET )
					{
						SOCKET s = context->socket;
						context->socket = INVALID_SOCKET;
						_shutdown( s, SD_BOTH );
						_closesocket( s );	// Saves us from having to post if there's already a pending IO operation. Should force the operation to complete.
					}
				}

				LeaveCriticalSection( &update_check_timeout_cs );

				CloseHandle( g_update_semaphore );
				g_update_semaphore = NULL;

#ifdef ENABLE_LOGGING
				unsigned long version_a = g_new_version >> 24;
				unsigned long version_b = ( g_new_version & 0x00FF0000 ) >> 16;
				unsigned long version_c = ( g_new_version & 0x0000FF00 ) >> 8;
				unsigned long version_d = ( g_new_version & 0x000000FF );

				WriteLog( LOG_INFO_MISC, "Update response: %lu.%lu.%lu.%lu", version_a, version_b, version_c, version_d );
#endif
			}

			if ( g_hWnd_check_for_updates != NULL )
			{
				if ( g_new_version > CURRENT_VERSION )
				{
					_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 1, 0 );		// New version
				}
				else if ( g_new_version == CURRENT_VERSION )
				{
#ifdef IS_BETA
					if ( g_new_beta > BETA_VERSION )
					{
						_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 1, 0 );		// New version
					}
					else if ( g_new_beta == BETA_VERSION )
					{
#endif
						if ( g_update_check_state == 2 )	// Automatic update check.
						{
							_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 4, 0 );	// Destroy the window.
						}
						else
						{
							_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 2, 0 );	// Up to date
						}
#ifdef IS_BETA
					}
					else
					{
						_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 3, 0 );		// Error
					}
#endif
				}
				else
				{
					_SendNotifyMessageW( g_hWnd_check_for_updates, WM_PROPAGATE, 3, 0 );		// Error
				}
			}
		}

		// Release the semaphore if we're killing the thread.
		if ( worker_semaphore != NULL )
		{
			ReleaseSemaphore( worker_semaphore, 1, NULL );
		}

		in_worker_thread = false;

		// We're done. Let other threads continue.
		LeaveCriticalSection( &worker_cs );
	}

	_ExitThread( 0 );
	//return 0;
}
