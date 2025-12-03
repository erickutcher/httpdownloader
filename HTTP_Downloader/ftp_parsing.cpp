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

#include "ftp_parsing.h"
#include "http_parsing.h"

#include "ssl_openssl.h"

#include "utilities.h"

CRITICAL_SECTION ftp_listen_info_cs;

unsigned short g_ftp_current_port = 0;

SOCKET CreateFTPListenSocket( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return INVALID_SOCKET;
	}

	SOCKET socket = INVALID_SOCKET;
	bool try_local_address = false;

	unsigned char retry_bind_count = 0;

	struct addrinfoW *supplied_addr = NULL;
	struct addrinfo *local_addr = NULL;

	DWORD bytes = 0;
	GUID acceptex_guid = WSAID_ACCEPTEX;	// GUID to Microsoft specific extensions

	char hostname[ 256 ];
	_memzero( hostname, 256 );

	bool use_ipv6 = false;

TRY_NEW_PORT:

	int nRet = 0;

	sockaddr *ai_addr = NULL;
	size_t ai_addrlen = 0;

	struct addrinfoW supplied_hints;
	struct addrinfo local_hints;

	// Resolve the interface
	_memzero( &supplied_hints, sizeof( addrinfoW ) );
	supplied_hints.ai_flags = AI_PASSIVE;
	supplied_hints.ai_family = AF_INET;
	supplied_hints.ai_socktype = SOCK_STREAM;
	supplied_hints.ai_protocol = IPPROTO_IP;

	_memzero( &local_hints, sizeof( addrinfo ) );
	local_hints.ai_flags = AI_PASSIVE;
	local_hints.ai_socktype = SOCK_STREAM;
	local_hints.ai_protocol = IPPROTO_IP;

	SOCKET listen_socket = INVALID_SOCKET;

	//

	unsigned short port = 0;

	EnterCriticalSection( &ftp_listen_info_cs );

	if ( g_ftp_current_port < cfg_ftp_port_start || g_ftp_current_port > cfg_ftp_port_end )
	{
		g_ftp_current_port = cfg_ftp_port_start;
	}

	port = g_ftp_current_port++;

	LeaveCriticalSection( &ftp_listen_info_cs );

TRY_NEW_ADDRESS:

	if ( !try_local_address )
	{
		wchar_t cport[ 6 ];
		__snwprintf( cport, 6, L"%hu", port );

		// Use Hostname or IPv6 Address.
		if ( cfg_ftp_address_type == 0 )
		{
			nRet = _GetAddrInfoW( cfg_ftp_hostname, cport, &supplied_hints, &supplied_addr );
			if ( nRet == WSAHOST_NOT_FOUND )
			{
				use_ipv6 = true;

				supplied_hints.ai_family = AF_INET6;	// Try IPv6
				nRet = _GetAddrInfoW( cfg_ftp_hostname, cport, &supplied_hints, &supplied_addr );
			}

			if ( nRet != 0 )
			{
				goto CLEANUP;
			}
		}
		else	// Use IPv4 Address.
		{
			struct sockaddr_in src_addr;
			_memzero( &src_addr, sizeof( sockaddr_in ) );

			src_addr.sin_family = AF_INET;
			src_addr.sin_addr.s_addr = _htonl( cfg_ftp_ip_address );

			wchar_t wcs_ip[ 16 ];
			DWORD wcs_ip_length = 16;
			_WSAAddressToStringW( ( SOCKADDR * )&src_addr, sizeof( struct sockaddr_in ), NULL, wcs_ip, &wcs_ip_length );

			if ( _GetAddrInfoW( wcs_ip, cport, &supplied_hints, &supplied_addr ) != 0 )
			{
				goto CLEANUP;
			}
		}

		if ( supplied_addr == NULL )
		{
			goto CLEANUP;
		}

		ai_addr = supplied_addr->ai_addr;
		ai_addrlen = supplied_addr->ai_addrlen;
	}
	else
	{
		nRet = ( hostname[ 0 ] == NULL ? _gethostname( hostname, 256 ) : 0 );

		if ( nRet != 0 )
		{
			goto CLEANUP;
		}

		local_hints.ai_family = ( use_ipv6 ? AF_INET6 : AF_INET );

		char acport[ 6 ];
		__snprintf( acport, 6, "%hu", port );

		if ( _getaddrinfo( hostname, acport, &local_hints, &local_addr ) != 0 )
		{
			goto CLEANUP;
		}

		if ( local_addr == NULL )
		{
			goto CLEANUP;
		}

		ai_addr = local_addr->ai_addr;
		ai_addrlen = local_addr->ai_addrlen;
	}

	socket = CreateSocket( use_ipv6 );
	if ( socket == INVALID_SOCKET )
	{
		goto CLEANUP;
	}

	nRet = _bind( socket, ai_addr, ( int )ai_addrlen );
	if ( nRet == SOCKET_ERROR )
	{
		int error = _WSAGetLastError();

		// Retry the bind with a new port.
		if ( error == WSAEADDRINUSE &&
		   ( cfg_ftp_port_start == 0 || ( retry_bind_count < ( cfg_ftp_port_end - cfg_ftp_port_start ) ) ) &&
			 retry_bind_count < 10 )
		{
			if ( socket != INVALID_SOCKET )
			{
				_closesocket( socket );
			}

			if ( try_local_address )
			{
				if ( local_addr != NULL )
				{
					_freeaddrinfo( local_addr );
					local_addr = NULL;
				}
			}
			else
			{
				if ( supplied_addr != NULL )
				{
					_FreeAddrInfoW( supplied_addr );
					supplied_addr = NULL;
				}
			}

			++retry_bind_count;

			goto TRY_NEW_PORT;
		}
		else if ( error == WSAEADDRNOTAVAIL && !try_local_address )
		{
			if ( socket != INVALID_SOCKET )
			{
				_closesocket( socket );
			}

			try_local_address = true;

			goto TRY_NEW_ADDRESS;
		}
		else
		{
			goto CLEANUP;
		}
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

	//

	// Use the connection type to signify whether it's an IPV4 or IPV6 connection.
	context->header_info.http_method = ( use_ipv6 ? 2 : 1 );

	//

	// Get the IP address of our listening socket.
	context->header_info.url_location.host = ( char * )GlobalAlloc( GPTR, sizeof( char ) * INET6_ADDRSTRLEN );
	if ( context->header_info.url_location.host != NULL )
	{
		DWORD ip_length = INET6_ADDRSTRLEN;
		_WSAAddressToStringA( supplied_addr->ai_addr, ( DWORD )supplied_addr->ai_addrlen, NULL, context->header_info.url_location.host, &ip_length );

		if ( use_ipv6 )
		{
			// Exclude the brackets from the IPV6 string.
			if ( context->header_info.url_location.host[ 0 ] == '[' )
			{
				for ( unsigned char i = 1; i < ip_length; ++i )
				{
					if ( context->header_info.url_location.host[ i ] == ']' )
					{
						context->header_info.url_location.host[ i - 1 ] = 0;
						ip_length = i;

						break;
					}

					context->header_info.url_location.host[ i - 1 ] = context->header_info.url_location.host[ i ];
				}
			}
		}
		else
		{
			while ( ip_length > 0 )
			{
				if ( context->header_info.url_location.host[ ip_length ] == ':' )
				{
					context->header_info.url_location.host[ ip_length ] = 0;	// Don't include the port.

					break;
				}
				else if ( context->header_info.connection == FTP_MODE_ACTIVE && context->header_info.url_location.host[ ip_length ] == '.' )
				{
					context->header_info.url_location.host[ ip_length ] = ',';	// Replace periods with commas for Active mode.
				}

				--ip_length;
			}
		}
	}

	//

	// Get the port number of our listening socket.
	SOCKADDR_INET addr;
	_memzero( &addr, sizeof( addr ) );

	int len;
	SOCKADDR *u_addr;

	if ( use_ipv6 )
	{
		u_addr = ( SOCKADDR * )&addr.Ipv6;
		len = sizeof( addr.Ipv6 );
	}
	else
	{
		u_addr = ( SOCKADDR * )&addr.Ipv4;
		len = sizeof( addr.Ipv4 );
	}

	_getsockname( socket, u_addr, &len );

	if ( use_ipv6 )
	{
		port = addr.Ipv6.sin6_port;
	}
	else
	{
		port = addr.Ipv4.sin_port;
	}

	context->header_info.url_location.port = ( ( port & 0x00FF ) << 8 ) | ( ( port & 0xFF00 ) >> 8 );

	//

	listen_socket = socket;
	socket = INVALID_SOCKET;

CLEANUP:

	if ( socket != INVALID_SOCKET )
	{
		_closesocket( socket );
	}

	if ( supplied_addr != NULL )
	{
		_FreeAddrInfoW( supplied_addr );
	}

	if ( local_addr != NULL )
	{
		_freeaddrinfo( local_addr );
	}

	return listen_socket;
}

char CreateFTPAcceptSocket( SOCKET_CONTEXT *context )
{
	int nRet = 0;
	DWORD dwRecvNumBytes = 0;

	if ( context == NULL )
	{
		return LA_STATUS_FAILED;
	}

	// Create a listen context.
	context->ftp_context = UpdateCompletionPort( context->listen_socket, ( context->request_info.protocol == PROTOCOL_FTPS ? true : false ), ( context->download_info != NULL ? context->download_info->ssl_version : 0 ), true, false );
	if ( context->ftp_context == NULL )
	{
		return LA_STATUS_FAILED;
	}

	// Have the listen context point back to the Control context.
	// If a connection is accepted, a Data context will be created and the listen context will be freed.
	// The Data and Control contexts will be updated to point to each other.
	context->ftp_context->ftp_context = context;

	context->ftp_context->ftp_connection_type = FTP_CONNECTION_TYPE_LISTEN;	// Listen context.
	context->ftp_context->download_info = context->download_info;

	// The accept socket will inherit the listen socket's properties when it completes. IPv6 doesn't actually have to be set here.
	context->ftp_context->socket = CreateSocket( ( context->header_info.http_method == 2 ? true : false ) );
	if ( context->ftp_context->socket == INVALID_SOCKET )
	{
		return LA_STATUS_FAILED;
	}

	InterlockedIncrement( &context->ftp_context->pending_operations );

	// Accept a connection without waiting for any data. (dwReceiveDataLength = 0)
	nRet = _AcceptEx( context->listen_socket, context->ftp_context->socket, ( LPVOID )( context->ftp_context->buffer ), 0, sizeof( SOCKADDR_STORAGE ) + 16, sizeof( SOCKADDR_STORAGE ) + 16, &dwRecvNumBytes, ( OVERLAPPED * )&context->ftp_context->overlapped );
	if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
	{
		InterlockedDecrement( &context->ftp_context->pending_operations );

		return LA_STATUS_FAILED;
	}

	return LA_STATUS_OK;
}

char MakeRangeDataRequest( SOCKET_CONTEXT *context )
{
	char content_status = FTP_CONTENT_STATUS_FAILED;

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
				new_context->ftp_connection_type = FTP_CONNECTION_TYPE_CONTROL;

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

					new_context->request_info.redirect_count = context->request_info.redirect_count;	// This is being used to determine whether we've switched modes (fallback mode).
					new_context->header_info.connection = context->header_info.connection;				// This is being used as our mode value. (cfg_ftp_mode_type)
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

		content_status = FTP_CONTENT_STATUS_NONE;
	}

	return content_status;
}

void SetDataContextValues( SOCKET_CONTEXT *context, SOCKET_CONTEXT *new_context )
{
	if ( context != NULL && new_context != NULL )
	{
		context->ftp_context = new_context;
		new_context->ftp_context = context;
		new_context->ftp_connection_type = FTP_CONNECTION_TYPE_DATA;	// Data.

		new_context->processed_header = context->processed_header;

		new_context->part = context->part;
		new_context->parts = context->parts;

		//

		if ( context->header_info.connection == FTP_MODE_PASSIVE )			// Passive mode only.
		{
			new_context->request_info.host = GlobalStrDupA( context->header_info.url_location.host );	// We retrieved this on port 21.
		}
		else
		{
			new_context->request_info.host = GlobalStrDupA( context->request_info.host );
		}
		new_context->request_info.port = context->header_info.url_location.port;	// We retrieved this on port 21.
		new_context->request_info.resource = GlobalStrDupA( context->request_info.resource );
		
		if ( context->request_info.protocol == PROTOCOL_FTPES )	// If Control connection is explicit.
		{
			new_context->request_info.protocol = PROTOCOL_FTPS;	// Make Data connection implicit.
		}
		else
		{
			new_context->request_info.protocol = context->request_info.protocol;
		}

		new_context->header_info.range_info = context->header_info.range_info;

		//

		new_context->context_node.data = new_context;

		if ( context->download_info != NULL )
		{
			EnterCriticalSection( &context->download_info->di_cs );

			new_context->download_info = context->download_info;

			new_context->parts_node.data = new_context;
			DLL_AddNode( &new_context->download_info->parts_list, &new_context->parts_node, -1 );

			LeaveCriticalSection( &context->download_info->di_cs );
		}

		new_context->status = STATUS_CONNECTING;

		new_context->content_status = FTP_CONTENT_STATUS_GET_CONTENT;
	}
}

char MakeDataRequest( SOCKET_CONTEXT *context )
{
	char content_status = FTP_CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		context->processed_header = true;

		SOCKET_CONTEXT *new_context = CreateSocketContext();

		SetDataContextValues( context, new_context );

		EnterCriticalSection( &context_list_cs );

		DLL_AddNode( &g_context_list, &new_context->context_node, 0 );

		LeaveCriticalSection( &context_list_cs );

		if ( !CreateConnection( new_context, new_context->request_info.host, new_context->request_info.port ) )
		{
			new_context->status = STATUS_FAILED;

			InterlockedIncrement( &new_context->pending_operations );

			new_context->overlapped.current_operation = IO_Close;

			PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )new_context, ( OVERLAPPED * )&new_context->overlapped );
		}

		content_status = FTP_CONTENT_STATUS_NONE;
	}

	return content_status;
}

char MakeFTPResponse( SOCKET_CONTEXT *context )
{
	char content_status = FTP_CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		context->wsabuf.buf = context->buffer;

		switch ( context->content_status )
		{
			case FTP_CONTENT_STATUS_SET_AUTH:
			{
				if ( context->download_info != NULL && context->download_info->ssl_version <= 1 )
				{
					_memcpy_s( context->wsabuf.buf, context->buffer_size, "AUTH SSL\r\n", 10 );
				}
				else
				{
					_memcpy_s( context->wsabuf.buf, context->buffer_size, "AUTH TLS\r\n", 10 );
				}
				context->wsabuf.len = 10;
			}
			break;

			case FTP_CONTENT_STATUS_SET_PBSZ:
			{
				_memcpy_s( context->wsabuf.buf, context->buffer_size, "PBSZ 0\r\n", 8 );
				context->wsabuf.len = 8;
			}
			break;

			case FTP_CONTENT_STATUS_SET_PROT:
			{
				_memcpy_s( context->wsabuf.buf, context->buffer_size, "PROT P\r\n", 8 );
				context->wsabuf.len = 8;
			}
			break;

			case FTP_CONTENT_STATUS_SEND_USER:
			{
				if ( context->download_info->auth_info.username == NULL )
				{
					_memcpy_s( context->wsabuf.buf, context->buffer_size, "USER anonymous\r\n", 16 );
					context->wsabuf.len = 16;
				}
				else
				{
					context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size, "USER %s\r\n", context->download_info->auth_info.username );
				}
			}
			break;

			case FTP_CONTENT_STATUS_SEND_PASS:
			{
				if ( context->download_info->auth_info.password == NULL )
				{
					_memcpy_s( context->wsabuf.buf, context->buffer_size, "PASS user@example.com\r\n", 23 );
					context->wsabuf.len = 23;
				}
				else
				{
					context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size, "PASS %s\r\n", context->download_info->auth_info.password );
				}
			}
			break;

			case FTP_CONTENT_STATUS_SET_OPTS:
			{
				_memcpy_s( context->wsabuf.buf, context->buffer_size, "OPTS UTF8 ON\r\n", 14 );
				context->wsabuf.len = 14;
			}
			break;

			case FTP_CONTENT_STATUS_SET_TYPE:
			{
				_memcpy_s( context->wsabuf.buf, context->buffer_size, "TYPE I\r\n", 8 );
				context->wsabuf.len = 8;
			}
			break;

			case FTP_CONTENT_STATUS_GET_SIZE:
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
					"SIZE %s\r\n", context->request_info.resource );
			}
			break;

			case FTP_CONTENT_STATUS_GET_MDTM:
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
					"MDTM %s\r\n", context->request_info.resource );
			}
			break;

			case FTP_CONTENT_STATUS_SET_MODE:
			{
				if ( context->header_info.connection & FTP_MODE_ACTIVE )
				{
					if ( context->header_info.connection & FTP_MODE_EXTENDED )
					{
						// http_method is being used to determine if it's IPV4 or IPV6. 1 = IPV4, 2 = IPV6
						context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
							"EPRT |%lu|%s|%lu|\r\n", context->header_info.http_method, context->header_info.url_location.host, context->header_info.url_location.port );
					}
					else
					{
						context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
							"PORT %s,%lu,%lu\r\n", context->header_info.url_location.host, context->header_info.url_location.port >> 8, context->header_info.url_location.port & 0x00FF );
					}
				}
				else// if ( context->header_info.connection == FTP_MODE_PASSIVE )
				{
					if ( context->header_info.connection & FTP_MODE_EXTENDED )
					{
						_memcpy_s( context->wsabuf.buf, context->buffer_size, "EPSV\r\n", 6 );
					}
					else
					{
						_memcpy_s( context->wsabuf.buf, context->buffer_size, "PASV\r\n", 6 );
					}

					context->wsabuf.len = 6;
				}
			}
			break;

			case FTP_CONTENT_STATUS_SEND_RETR:
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
					"RETR %s\r\n", context->request_info.resource );
			}
			break;

			case FTP_CONTENT_STATUS_SEND_REST:
			{
				context->wsabuf.len = __snprintf( context->wsabuf.buf, context->buffer_size,
					"REST %I64u\r\n", context->header_info.range_info->range_start );
			}
			break;

			case FTP_CONTENT_STATUS_SEND_QUIT:
			{
				_memcpy_s( context->wsabuf.buf, context->buffer_size, "QUIT\r\n", 6 );
				context->wsabuf.len = 6;
			}
			break;
		}

		bool sent = false;
		int nRet = 0;
		DWORD dwFlags = 0;

		InterlockedIncrement( &context->pending_operations );

		context->overlapped.current_operation = IO_Write;
		context->overlapped.next_operation = IO_GetContent;

		if ( context->_ssl_s != NULL || context->_ssl_o != NULL )
		{
			if ( context->_ssl_s != NULL )
			{
				SSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
			}
			else// if ( context->_ssl_o != NULL )
			{
				if ( context->_ssl_o->ssl != NULL )
				{
					// Encrypts the buffer and writes it to the write BIO.
					int write = _SSL_write( context->_ssl_o->ssl, context->wsabuf.buf, context->wsabuf.len );
					if ( write > 0 )
					{
						// Make sure we have encrypted data to send.
						int pending = _BIO_pending( context->_ssl_o->wbio );
						if ( pending > 0 )
						{
							OpenSSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
						}
					}
				}
			}

			if ( !sent )
			{
				InterlockedDecrement( &context->pending_operations );

				content_status = FTP_CONTENT_STATUS_FAILED;
			}
			else
			{
				content_status = FTP_CONTENT_STATUS_NONE;
			}
		}
		else
		{
			nRet = _WSASend( context->socket, &context->wsabuf, 1, NULL, dwFlags, ( OVERLAPPED * )&context->overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				InterlockedDecrement( &context->pending_operations );

				content_status = FTP_CONTENT_STATUS_FAILED;
			}
			else
			{
				content_status = FTP_CONTENT_STATUS_NONE;
			}
		}
	}

	return content_status;
}

char SendFTPKeepAlive( SOCKET_CONTEXT *context )
{
	char content_status = FTP_CONTENT_STATUS_FAILED;

	if ( context != NULL )
	{
		if ( context->keep_alive_buffer == NULL )
		{
			context->keep_alive_buffer = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( BUFFER_SIZE + 1 ) );
		}

		if ( context->keep_alive_buffer != NULL )
		{
			context->content_status = FTP_CONTENT_STATUS_SEND_KEEP_ALIVE;

			context->keep_alive_wsabuf.buf = context->keep_alive_buffer;

			unsigned char random_number = 0;

			HCRYPTPROV hProvider = NULL;

			if ( _CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT ) )
			{
				BYTE rbuffer[ 1 ];
				rbuffer[ 0 ] = 0;

				if ( _CryptGenRandom( hProvider, 1, ( BYTE * )&rbuffer ) )
				{
					random_number = rbuffer[ 0 ] % 4;
				}
			}

			if ( hProvider != NULL )
			{
				_CryptReleaseContext( hProvider, 0 );
			}

			switch ( random_number )
			{
				case 1:
				{
					_memcpy_s( context->keep_alive_wsabuf.buf, 8, "PWD\r\n", 5 );
					context->keep_alive_wsabuf.len = 5;
				}
				break;

				case 2:
				{
					_memcpy_s( context->keep_alive_wsabuf.buf, 8, "TYPE I\r\n", 8 );
					context->keep_alive_wsabuf.len = 8;
				}
				break;

				case 3:
				{
					_memcpy_s( context->keep_alive_wsabuf.buf, 8, "TYPE A\r\n", 8 );
					context->keep_alive_wsabuf.len = 8;
				}
				break;

				case 0:
				default:
				{
					_memcpy_s( context->keep_alive_wsabuf.buf, 8, "NOOP\r\n", 6 );
					context->keep_alive_wsabuf.len = 6;
				}
				break;
			}

			bool sent = false;
			int nRet = 0;
			DWORD dwFlags = 0;

			InterlockedIncrement( &context->pending_operations );

			// We'll receive the reply/replies on the regular wsabuf buffer. A read operation should be pending while a Data connection is downloading.
			// All that IO_KeepAlive does is send any remaining data in the keep_alive_wsabuf buffer that hasn't been sent.
			context->overlapped_keep_alive.current_operation = IO_KeepAlive;
			context->overlapped_keep_alive.next_operation = IO_KeepAlive;

			if ( context->_ssl_s != NULL || context->_ssl_o != NULL )
			{
				if ( context->_ssl_s != NULL )
				{
					SSL_WSASend( context, &context->overlapped_keep_alive, &context->keep_alive_wsabuf, sent );
				}
				else// if ( context->_ssl_o != NULL )
				{
					if ( context->_ssl_o->ssl != NULL )
					{
						// Encrypts the buffer and writes it to the write BIO.
						int write = _SSL_write( context->_ssl_o->ssl, context->keep_alive_wsabuf.buf, context->keep_alive_wsabuf.len );
						if ( write > 0 )
						{
							// Make sure we have encrypted data to send.
							int pending = _BIO_pending( context->_ssl_o->wbio );
							if ( pending > 0 )
							{
								OpenSSL_WSASend( context, &context->overlapped_keep_alive, &context->keep_alive_wsabuf, sent );
							}
						}
					}
				}

				if ( !sent )
				{
					InterlockedDecrement( &context->pending_operations );

					content_status = FTP_CONTENT_STATUS_FAILED;
				}
				else
				{
					content_status = FTP_CONTENT_STATUS_NONE;
				}
			}
			else
			{
				nRet = _WSASend( context->socket, &context->keep_alive_wsabuf, 1, NULL, dwFlags, ( OVERLAPPED * )&context->overlapped_keep_alive, NULL );
				if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
				{
					InterlockedDecrement( &context->pending_operations );

					content_status = FTP_CONTENT_STATUS_FAILED;
				}
				else
				{
					content_status = FTP_CONTENT_STATUS_NONE;
				}
			}
		}
	}

	return content_status;
}

char ProcessFTPFileInfo( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return FTP_CONTENT_STATUS_FAILED;
	}

	char content_status = FTP_CONTENT_STATUS_NONE;

	if ( context->download_info != NULL )
	{
		// This won't be true since it happens when we get the file status (213).
		if ( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_VERIFY )
		{
			return FTP_CONTENT_STATUS_FAILED;	// Bail before we download more.
		}
		else if ( !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
		{
			content_status = HandleLastModifiedPrompt( context );

			if ( content_status != FTP_CONTENT_STATUS_NONE )
			{
				return content_status;
			}

			content_status = HandleFileSizePrompt( context );

			if ( content_status != FTP_CONTENT_STATUS_NONE )
			{
				return content_status;
			}

			if ( !context->is_allocated )
			{
				// Returns either FTP_CONTENT_STATUS_FAILED, FTP_CONTENT_STATUS_ALLOCATE_FILE, or FTP_CONTENT_STATUS_NONE.
				content_status = context->content_status = AllocateFile( context, IO_ResumeGetContent );

				if ( content_status != FTP_CONTENT_STATUS_NONE )
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

		content_status = FTP_CONTENT_STATUS_FAILED;	// Close the connection.
	}

	return content_status;
}

char HandleModeRequest( SOCKET_CONTEXT *context )
{
	if ( context == NULL )
	{
		return FTP_CONTENT_STATUS_FAILED;
	}

	char content_status = context->content_status = ProcessFTPFileInfo( context );

	if ( content_status != FTP_CONTENT_STATUS_NONE )
	{
		return content_status;
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
				MakeRangeDataRequest( context );
			}
		}
	}

	context->content_status = FTP_CONTENT_STATUS_SET_MODE;				// 200 if active successful, 229 if passive successful.

	// We need the IP and port that we're listening on if we're in Active mode.
	if ( context->header_info.connection & FTP_MODE_ACTIVE )
	{
		bool listener_failed = false;

		// Allow the Control context to control the listen socket (shutdown/close it).
		context->listen_socket = CreateFTPListenSocket( context );

		// Create the accept socket.
		if ( context->listen_socket != INVALID_SOCKET )
		{
			if ( CreateFTPAcceptSocket( context ) != LA_STATUS_OK )
			{
				listener_failed = true;
			}
		}
		else
		{
			listener_failed = true;
		}

		if ( listener_failed )
		{
			if ( cfg_ftp_enable_fallback_mode && context->request_info.redirect_count == 0 )
			{
				// We'll use this to deterine whether we've switched modes.
				++context->request_info.redirect_count;

				// Try Passive/Extended Passive mode.
				context->header_info.connection &= ~FTP_MODE_ACTIVE;
				context->header_info.connection |= FTP_MODE_PASSIVE;

				// Unlink the Control and Data contexts. Allow the Data context to clean itself up.
				EnterCriticalSection( &context->context_cs );

				if ( context->ftp_context != NULL )
				{
					context->ftp_context->download_info = NULL;
					context->ftp_context->ftp_context = NULL;

					context->ftp_context = NULL;
				}

				if ( context->listen_socket != INVALID_SOCKET )
				{
					_shutdown( context->listen_socket, SD_BOTH );
					_closesocket( context->listen_socket );
					context->listen_socket = INVALID_SOCKET;
				}

				LeaveCriticalSection( &context->context_cs );
			}
			else
			{
				context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
			}
		}
	}

	return FTP_CONTENT_STATUS_HANDLE_REQUEST;
}

char GetFTPResponseContent( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length )
{
	if ( context == NULL )
	{
		return FTP_CONTENT_STATUS_FAILED;
	}

	char content_status;

	if ( context->content_status != FTP_CONTENT_STATUS_GET_CONTENT )
	{
		// The status values are set in ProcessFTPFileInfo().
		if ( context->content_status == FTP_CONTENT_STATUS_LAST_MODIFIED_PROMPT ||
			 context->content_status == FTP_CONTENT_STATUS_FILE_SIZE_PROMPT ||
			 context->content_status == FTP_CONTENT_STATUS_ALLOCATE_FILE )
		{
			// Will set the content_status if successful.
			return HandleModeRequest( context );
		}
		else
		{
			// Make sure we've received a valid FTP reply. A minimum length of 6: "123 \r\n"
			if ( response_buffer_length < 6 || _memcmp( response_buffer + ( response_buffer_length - 2 ), "\r\n", 2 ) != 0 )
			{
				context->wsabuf.buf += response_buffer_length;
				context->wsabuf.len -= response_buffer_length;

				return FTP_CONTENT_STATUS_READ_MORE_CONTENT;
			}

			char ccode[ 4 ];
			int code = 0;

			char *multiline_start = NULL;
			char *last_line;
			char *end_of_line = response_buffer;
			char *end_of_buffer = response_buffer + response_buffer_length;

			while ( end_of_line < end_of_buffer )
			{
				last_line = end_of_line;

				end_of_line = _StrStrA( last_line, "\r\n" ) + 2;

				if ( ( end_of_line - last_line ) >= 6 )
				{
					if ( multiline_start != NULL )
					{
						if ( last_line[ 0 ] == multiline_start[ 0 ] &&
							 last_line[ 1 ] == multiline_start[ 1 ] &&
							 last_line[ 2 ] == multiline_start[ 2 ] &&
							 last_line[ 3 ] == ' ' )
						{
							multiline_start = NULL;
						}
					}
					else
					{
						ccode[ 0 ] = last_line[ 0 ];
						ccode[ 1 ] = last_line[ 1 ];
						ccode[ 2 ] = last_line[ 2 ];
						ccode[ 3 ] = 0;	// Sanity.

						code = _strtoul( ccode, NULL, 10 );

						// A transfer as successfully completed or the connection was closed.
						// Ignore any other code that follows.
						if ( code == 226 || code == 426 )
						{
							break;
						}

						// See if we need to search for the end of a multiline comment.
						if ( last_line[ 3 ] == '-' )
						{
							multiline_start = last_line;
						}
					}
				}
			}

			if ( multiline_start != NULL )
			{
				context->wsabuf.buf += response_buffer_length;
				context->wsabuf.len -= response_buffer_length;

				return FTP_CONTENT_STATUS_READ_MORE_CONTENT;
			}

			if ( context->download_info != NULL )
			{
				EnterCriticalSection( &context->download_info->di_cs );
				context->download_info->code = code;
				LeaveCriticalSection( &context->download_info->di_cs );
			}

			switch ( code )
			{
				case 220:	// Server ready, we'll send our username.
				{
					if ( context->request_info.protocol == PROTOCOL_FTPES )
					{
						context->content_status = FTP_CONTENT_STATUS_SET_AUTH;		// 234 if successful.
					}
					else if ( context->request_info.protocol == PROTOCOL_FTPS )
					{
						context->content_status = FTP_CONTENT_STATUS_SET_PBSZ;		// 200 if successful.
					}
					else
					{
						/*if ( context->download_info != NULL )
						{
							EnterCriticalSection( &context->download_info->di_cs );

							context->download_info->ssl_version = -1;

							LeaveCriticalSection( &context->download_info->di_cs );
						}*/

						context->content_status = FTP_CONTENT_STATUS_SEND_USER;		// 230 (no password needed) or 331 (password needed) if successful.
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 234:	// Authentication accepted. We need to upgrade the connection to SSL/TLS.
				{
					context->content_status = FTP_CONTENT_STATUS_SET_PBSZ;			// 200 if successful.

					bool connection_failed = false;

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
							case 0:	{ if ( shared_protocol < 4 ) { protocol |= SP_PROT_SSL2_CLIENT; } }
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

					if ( !connection_failed )
					{
						bool sent = false;

						InterlockedIncrement( &context->pending_operations );

						if ( g_use_openssl )
						{
							if ( context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
							{
								// This shouldn't return 1 here since we haven't sent the data in the BIO.
								int nRet = _SSL_connect( context->_ssl_o->ssl );
								if ( nRet <= 0 )
								{
									int error = _SSL_get_error( context->_ssl_o->ssl, nRet );

									if ( error == SSL_ERROR_WANT_READ )
									{
										int pending = _BIO_pending( context->_ssl_o->wbio );
										if ( pending > 0 )
										{
											context->overlapped.current_operation = IO_Write;
											context->overlapped.next_operation = IO_OpenSSLClientHandshake;
										
											OpenSSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
										}
									}
								}
							}
						}
						else
						{
							context->overlapped.next_operation = IO_ClientHandshakeResponse;

							SSL_WSAConnect( context, &context->overlapped, context->request_info.host, sent );
						}

						if ( !sent )
						{
							InterlockedDecrement( &context->pending_operations );
						}
						else
						{
							return FTP_CONTENT_STATUS_NONE;
						}
					}
				}
				break;

				case 331:	// Password required.
				{
					context->content_status = FTP_CONTENT_STATUS_SEND_PASS;					// 230 if successful.

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 230:	// Logged in. We'll set the options for UTF-8
				{
					context->content_status = FTP_CONTENT_STATUS_SET_OPTS;				// 200 if successful, 202 if already on.

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 200:	// Successful action completed. Figure out what and do the next step.
				case 202:	// Superfluous.
				case 257:	// Current working directory.
				{
					if ( context->content_status == FTP_CONTENT_STATUS_SET_PBSZ )
					{
						context->content_status = FTP_CONTENT_STATUS_SET_PROT;				// 200 if successful.
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SET_PROT )
					{
						context->content_status = FTP_CONTENT_STATUS_SEND_USER;				// 331 if successful.
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SET_OPTS )
					{
						context->content_status = FTP_CONTENT_STATUS_SET_TYPE;				// 200 if successful.
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SET_TYPE )
					{
						// We only need to get the size once.
						// We always need to get the last modified time to see if it's changed and to prompt the user.
						/*if ( context->processed_header )
						{
							context->content_status = FTP_CONTENT_STATUS_GET_MDTM;			// 213 if successful.
						}
						else*/
						{
							context->content_status = FTP_CONTENT_STATUS_GET_SIZE;			// 213 if successful.
						}
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SET_MODE )
					{
						if ( context->parts > 1 || ( context->download_info != NULL && IS_GROUP( context->download_info ) ) )
						{
							context->content_status = FTP_CONTENT_STATUS_SEND_REST;			// 350 if successful.
						}
						else
						{
							context->content_status = FTP_CONTENT_STATUS_SEND_RETR;			// 150 if successful.
						}
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SEND_KEEP_ALIVE )	// A decent server will send a reply while a transfer is in progress.
					{
						return FTP_CONTENT_STATUS_READ_MORE_CONTENT;	// Read more content on the non keep-alive overlapped value.
					}
					else	// A keep-alive reply was sent after we QUIT. Close the connection.
					{
						return FTP_CONTENT_STATUS_FAILED;
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 500:	// Command not recognized. (Probably an old server.)
				case 502:	// Command not implemented.
				{
					if ( context->content_status == FTP_CONTENT_STATUS_SET_OPTS )			// UTF8 is not supported.
					{
						context->content_status = FTP_CONTENT_STATUS_SET_TYPE;				// 200 if successful.
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_SET_MODE )		// EPSV or EPRT is not supported.
					{
						// Use the older version of the mode.
						if ( context->header_info.connection == ( FTP_MODE_ACTIVE | FTP_MODE_EXTENDED ) )
						{
							// http_method is being used to determine if it's IPV4 or IPV6. 1 = IPV4, 2 = IPV6
							// We can't use IPV6 here.
							if ( context->header_info.http_method == 1 && context->header_info.url_location.host != NULL )
							{
								char *phost = context->header_info.url_location.host;
								while ( phost != NULL && *phost != NULL )
								{
									if ( *phost == '.' )
									{
										*phost = ',';
									}

									++phost;
								}

								context->header_info.connection = FTP_MODE_ACTIVE;			// Try Active mode.
							}
							else
							{
								context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
							}
						}
						else if ( context->header_info.connection == ( FTP_MODE_PASSIVE | FTP_MODE_EXTENDED ) )
						{
							context->header_info.connection = FTP_MODE_PASSIVE;				// Try Passive mode.
						}
						else
						{
							context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
						}
					}
					else
					{
						context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 213:	// File status
				{
					if ( context->content_status == FTP_CONTENT_STATUS_GET_SIZE )
					{
						if ( response_buffer_length > 6 )
						{
							int content_length_length = response_buffer_length - 6;	// Exclude the "213 " and "\r\n".
							if ( content_length_length > 20 )
							{
								content_length_length = 20;
							}

							char clength[ 21 ];
							_memzero( clength, 21 );
							_memcpy_s( clength, 21, response_buffer + 4, content_length_length );
							clength[ 20 ] = 0;	// Sanity

							context->header_info.range_info->content_length = strtoull( clength );
						}

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
						if ( context->parts == 1 && context->header_info.range_info->content_length > 0 && context->header_info.range_info->range_end == 0 )
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
								// We'll have set the download_info->file_size in HandleModeRequest().
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

							if ( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_VERIFY )
							{
								bad_content_length = true;	// Bail before we download more.
							}

							LeaveCriticalSection( &context->download_info->shared_info->di_cs );

							if ( bad_content_length )
							{
								return FTP_CONTENT_STATUS_FAILED;	// Close the connection.
							}
						}

						context->content_status = FTP_CONTENT_STATUS_GET_MDTM;				// 213 if successful.

						return FTP_CONTENT_STATUS_HANDLE_REQUEST;
					}
					else if ( context->content_status == FTP_CONTENT_STATUS_GET_MDTM )
					{
						if ( context->download_info != NULL )
						{
							if ( context->got_last_modified == 0 && !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) && response_buffer_length >= 20 )
							{
								SYSTEMTIME date_time;
								_memzero( &date_time, sizeof( SYSTEMTIME ) );

								char ctval[ 5 ];
								
								ctval[ 0 ] = response_buffer[ 4 ];
								ctval[ 1 ] = response_buffer[ 5 ];
								ctval[ 2 ] = response_buffer[ 6 ];
								ctval[ 3 ] = response_buffer[ 7 ];
								ctval[ 4 ] = 0;	// Sanity.
								
								date_time.wYear = ( WORD )_strtoul( ctval, NULL, 10 );

								ctval[ 0 ] = response_buffer[ 8 ];
								ctval[ 1 ] = response_buffer[ 9 ];
								ctval[ 2 ] = 0;	// Sanity.

								date_time.wMonth = ( WORD )_strtoul( ctval, NULL, 10 );

								ctval[ 0 ] = response_buffer[ 10 ];
								ctval[ 1 ] = response_buffer[ 11 ];
								ctval[ 2 ] = 0;	// Sanity.

								date_time.wDay = ( WORD )_strtoul( ctval, NULL, 10 );

								ctval[ 0 ] = response_buffer[ 12 ];
								ctval[ 1 ] = response_buffer[ 13 ];
								ctval[ 2 ] = 0;	// Sanity.

								date_time.wHour = ( WORD )_strtoul( ctval, NULL, 10 );

								ctval[ 0 ] = response_buffer[ 14 ];
								ctval[ 1 ] = response_buffer[ 15 ];
								ctval[ 2 ] = 0;	// Sanity.

								date_time.wMinute = ( WORD )_strtoul( ctval, NULL, 10 );

								ctval[ 0 ] = response_buffer[ 16 ];
								ctval[ 1 ] = response_buffer[ 17 ];
								ctval[ 2 ] = 0;	// Sanity.

								date_time.wSecond = ( WORD )_strtoul( ctval, NULL, 10 );

								if ( response_buffer_length >= 24 )
								{
									// Exclude "."
									ctval[ 0 ] = response_buffer[ 24 ];
									ctval[ 1 ] = response_buffer[ 25 ];
									ctval[ 2 ] = response_buffer[ 26 ];
									ctval[ 2 ] = response_buffer[ 27 ];
									ctval[ 3 ] = 0;	// Sanity.

									date_time.wMilliseconds = ( WORD )_strtoul( ctval, NULL, 10 );
								}

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

							EnterCriticalSection( &context->download_info->di_cs );

							context->download_info->processed_header = true;

							LeaveCriticalSection( &context->download_info->di_cs );
						}

						if ( !context->processed_header || context->header_info.connection == 0 )
						{
							if ( !context->processed_header )
							{
								if ( IS_GROUP( context->download_info ) )
								{
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

							// We will use the extended mode by default since it supports IPV6.
							context->header_info.connection = ( cfg_ftp_mode_type == 1 ? FTP_MODE_ACTIVE : FTP_MODE_PASSIVE ) | FTP_MODE_EXTENDED;	// cfg_ftp_mode_type: 0 = Passive, 1 = Active
						}

						// Will set the content_status if successful.
						return HandleModeRequest( context );
					}
					else	// This shouldn't happen.
					{
						return FTP_CONTENT_STATUS_FAILED;	// Close the connection.
					}
				}
				break;

				case 227:	// Entering passive mode.
				{
					// So there's no standard format for this reply.
					// The IP and port may be surrounded by parentheses.
					// There's also no reason why the IP and port can't be in the middle of some text. Dumb!

					// 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).

					// We're going to mangle the buffer.

					bool parsed = false;
					unsigned char comma_count = 0;

					response_buffer[ response_buffer_length - 2 ] = 0;	// Sanity.

					char *field_start = response_buffer + ( response_buffer_length - 3 );

					while ( field_start > ( response_buffer + 3 ) )
					{
						if ( *field_start == ',' )
						{
							*field_start = ( comma_count > 1 ? '.' : 0 );

							++comma_count;
						}
						else if ( *field_start < 0x30 || *field_start > 0x39 )
						{
							if ( comma_count == 5 )
							{
								parsed = true;

								++field_start;

								break;
							}

							*field_start = 0;	// Sanity.
						}

						--field_start;
					}

					if ( parsed )
					{
						int val_length = lstrlenA( field_start );
						context->header_info.url_location.host = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( val_length + 1 ) );
						_memcpy_s( context->header_info.url_location.host, val_length + 1, field_start, val_length );
						context->header_info.url_location.host[ val_length ]	= 0;	// Sanity.

						field_start += ( val_length + 1 );

						val_length = lstrlenA( field_start );

						char cport[ 6 ];
						_memzero( cport, 6 );
						_memcpy_s( cport, 6, field_start, val_length );
						cport[ 5 ] = 0;	// Sanity

						context->header_info.url_location.port = ( ( unsigned short )_strtoul( cport, NULL, 10 ) << 8 );

						field_start += ( val_length + 1 );

						val_length = lstrlenA( field_start );

						_memzero( cport, 6 );
						_memcpy_s( cport, 6, field_start, val_length );
						cport[ 5 ] = 0;	// Sanity

						context->header_info.url_location.port |= ( unsigned short )_strtoul( cport, NULL, 10 );

						if ( context->parts > 1 || ( context->download_info != NULL && IS_GROUP( context->download_info ) ) )
						{
							context->content_status = FTP_CONTENT_STATUS_SEND_REST;			// 350 if successful.
						}
						else
						{
							MakeDataRequest( context );

							context->content_status = FTP_CONTENT_STATUS_SEND_RETR;			// 150 if successful.
						}
					}
					else
					{
						context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 229:	// Entering extended passive mode.
				{
					char *port_start = response_buffer + ( response_buffer_length - 3 );
					while ( port_start > response_buffer )
					{
						if ( *port_start == '(' )
						{
							break;
						}

						--port_start;
					}

					if ( ( response_buffer + ( response_buffer_length - 2 ) ) - port_start > 6 )
					{
						port_start += 4;
						char *port_end = _StrChrA( port_start, '|' );

						char cport[ 6 ];
						_memzero( cport, 6 );
						_memcpy_s( cport, 6, port_start, min( port_end - port_start, 5 ) );
						cport[ 5 ] = 0;	// Sanity

						context->header_info.url_location.port = ( unsigned short )_strtoul( cport, NULL, 10 );
					}

					if ( context->parts > 1 || ( context->download_info != NULL && IS_GROUP( context->download_info ) ) )
					{
						context->content_status = FTP_CONTENT_STATUS_SEND_REST;			// 350 if successful.
					}
					else
					{
						MakeDataRequest( context );

						context->content_status = FTP_CONTENT_STATUS_SEND_RETR;			// 150 if successful.
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 350:	// Requested file action pending further information.
				{
					if ( context->header_info.connection & FTP_MODE_PASSIVE )
					{
						MakeDataRequest( context );
					}

					context->content_status = FTP_CONTENT_STATUS_SEND_RETR;				// 150 if successful.

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 150:	// Opening file.
				{
					return FTP_CONTENT_STATUS_READ_MORE_CONTENT;
				}
				break;

				case 421:	// Rejected the connection. (Too many users, Active mode IP doesn't match Command IP).
				case 425:	// Can't open data connection.
				case 450:	// Requested file action not taken. (No TLS resumption).
				{
					if ( cfg_ftp_enable_fallback_mode && context->request_info.redirect_count == 0 )
					{
						// We'll use this to deterine whether we've switched modes.
						++context->request_info.redirect_count;

						context->content_status = FTP_CONTENT_STATUS_SET_MODE;				// 200 if active successful, 229 if passive successful.

						if ( context->header_info.connection & FTP_MODE_ACTIVE )
						{
							// Try Passive/Extended Passive mode.
							context->header_info.connection &= ~FTP_MODE_ACTIVE;
							context->header_info.connection |= FTP_MODE_PASSIVE;

							// Unlink the Control and Data contexts. Allow the Data context to clean itself up.
							EnterCriticalSection( &context->context_cs );

							if ( context->ftp_context != NULL )
							{
								context->ftp_context->download_info = NULL;
								context->ftp_context->ftp_context = NULL;

								context->ftp_context = NULL;
							}

							if ( context->listen_socket != INVALID_SOCKET )
							{
								_shutdown( context->listen_socket, SD_BOTH );
								_closesocket( context->listen_socket );
								context->listen_socket = INVALID_SOCKET;
							}

							LeaveCriticalSection( &context->context_cs );
						}
						else// if ( context->header_info.connection & FTP_MODE_PASSIVE )
						{
							// Try Passive/Extended Active mode.
							context->header_info.connection &= ~FTP_MODE_PASSIVE;
							context->header_info.connection |= FTP_MODE_ACTIVE;

							// Allow the Control context to control the listen socket (shutdown/close it).
							context->listen_socket = CreateFTPListenSocket( context );

							// Create the accept socket.
							if ( context->listen_socket != INVALID_SOCKET )
							{
								if ( CreateFTPAcceptSocket( context ) != LA_STATUS_OK )
								{
									context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
								}
							}
							else
							{
								context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
							}
						}
					}
					else
					{
						context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;
					}

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				case 530:	// Not logged in.
				{
					if ( !context->processed_header )
					{
						if ( context->download_info != NULL )
						{
							EnterCriticalSection( &context->download_info->di_cs );

							context->download_info->status = STATUS_AUTH_REQUIRED;
							context->status = STATUS_AUTH_REQUIRED;

							LeaveCriticalSection( &context->download_info->di_cs );
						}
					}
				}
				// Fall through.

				case 226:	// Data connection closed (Transfer successful).
				case 426:	// Connection closed or aborted.
				{
					context->ftp_connection_type = ( FTP_CONNECTION_TYPE_CONTROL | FTP_CONNECTION_TYPE_CONTROL_SUCCESS );	// Prevents us from closing Active mode listening sockets in CleanupConnection().

					context->content_status = FTP_CONTENT_STATUS_SEND_QUIT;

					return FTP_CONTENT_STATUS_HANDLE_REQUEST;
				}
				break;

				default:
				{
					context->status = STATUS_FAILED;

					if ( context->download_info != NULL &&
						 IS_GROUP( context->download_info ) )
					{
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
				break;
			}
		}
	}
	else
	{
		if ( context->download_info != NULL )
		{
			// If the first range request starts with 0, then the FTP server will attempt to serve the entire file.
			// That will cause us to get a larger response than we need. Make sure we handle only what we need and no more.
			if ( context->parts > 1 || IS_GROUP( context->download_info ) )
			{
				if ( context->header_info.range_info->content_offset + response_buffer_length > ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) )
				{
					response_buffer_length = ( unsigned int )( ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) - context->header_info.range_info->content_offset );
				}
			}

			char *output_buffer = response_buffer;
			unsigned int output_buffer_length = response_buffer_length;

			// Make sure the server isn't feeding us more data than they claim.
			if ( context->header_info.range_info->content_length > 0 &&
			   ( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) > ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) )
			{
				output_buffer_length -= ( unsigned int )( ( ( context->header_info.range_info->file_write_offset - context->header_info.range_info->range_start ) + output_buffer_length ) - ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) );
			}

			// Write buffer to file.

			if ( !( context->download_info->shared_info->download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
			{
				if ( context->download_info->shared_info->hFile != INVALID_HANDLE_VALUE )
				{
					LARGE_INTEGER li;
					li.QuadPart = context->header_info.range_info->file_write_offset;//context->header_info.range_info->range_start + context->header_info.range_info->content_offset;

					InterlockedIncrement( &context->pending_operations );

					context->overlapped.current_operation = IO_WriteFile;

					context->write_wsabuf.buf = output_buffer;
					context->write_wsabuf.len = output_buffer_length;

					context->overlapped.overlapped.hEvent = NULL;
					context->overlapped.overlapped.Internal = NULL;
					context->overlapped.overlapped.InternalHigh = NULL;
					//context->overlapped.Pointer = NULL;	// union
					context->overlapped.overlapped.Offset = li.LowPart;
					context->overlapped.overlapped.OffsetHigh = li.HighPart;

//					context->overlapped.context = context;

					context->content_status = FTP_CONTENT_STATUS_READ_MORE_CONTENT;

					content_status = FTP_CONTENT_STATUS_NONE;	// Exits IO_GetContent.

					context->content_offset = response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
					//context->header_info.range_info->content_offset += response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
					//context->header_info.range_info->file_write_offset += output_buffer_length;	// The size of the non-encoded/decoded data that we're writing to the file.

					EnterCriticalSection( &context->download_info->shared_info->di_cs );
					BOOL bRet = WriteFile( context->download_info->shared_info->hFile, context->write_wsabuf.buf, context->write_wsabuf.len, NULL, ( OVERLAPPED * )&context->overlapped );
					if ( bRet == FALSE && ( GetLastError() != ERROR_IO_PENDING ) )
					{
						InterlockedDecrement( &context->pending_operations );

						context->download_info->status = STATUS_FILE_IO_ERROR;
						context->status = STATUS_FILE_IO_ERROR;

						context->content_status = FTP_CONTENT_STATUS_FAILED;

						content_status = FTP_CONTENT_STATUS_FAILED;

						context->content_offset = 0;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						//context->header_info.range_info->content_offset -= response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.
						//context->header_info.range_info->file_write_offset -= output_buffer_length;	// The size of the non-encoded/decoded data that we're writing to the file.

						CloseHandle( context->download_info->shared_info->hFile );
						context->download_info->shared_info->hFile = INVALID_HANDLE_VALUE;
					}
					LeaveCriticalSection( &context->download_info->shared_info->di_cs );

					return content_status;
				}
				else	// Shouldn't happen.
				{
					return FTP_CONTENT_STATUS_FAILED;
				}
			}
			else	// Simulated download.
			{
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

				context->header_info.range_info->content_offset += response_buffer_length;	// The true amount that was downloaded. Allows us to resume if we stop the download.

				if ( ( context->parts > 1 || IS_GROUP( context->download_info ) ) &&
				   ( context->header_info.range_info->content_length == 0 ||
				   ( context->header_info.range_info->content_offset >= ( ( context->header_info.range_info->range_end - context->header_info.range_info->range_start ) + 1 ) ) ) )
				{
					return FTP_CONTENT_STATUS_FAILED;	// We have no more data, so just close the connection.
				}
				else
				{
					return FTP_CONTENT_STATUS_READ_MORE_CONTENT;
				}
			}
		}
	}

	return FTP_CONTENT_STATUS_FAILED;	// Close the connection.
}
