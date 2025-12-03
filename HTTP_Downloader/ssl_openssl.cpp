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

#include "ssl_openssl.h"

SSL_CTX *g_client_ssl_ctx[ PROTOCOL_COUNT ] = { NULL };
SSL_CTX *g_server_ssl_ctx = NULL;

void InitializeSSL_CTXs()
{
	DWORD protocol[ PROTOCOL_COUNT ] = { SSL2_VERSION,
										 SSL3_VERSION,
										 TLS1_VERSION,
										 TLS1_1_VERSION,
										 TLS1_2_VERSION,
										 TLS1_3_VERSION };

	const SSL_METHOD *ssl_method = _TLS_client_method();

	for ( unsigned char index = 0; index < PROTOCOL_COUNT; ++index )
	{
		g_client_ssl_ctx[ index ] = _SSL_CTX_new( ssl_method );
		if ( g_client_ssl_ctx[ index ] != NULL )
		{
			if ( _SSL_CTX_ctrl != NULL )
			{
				_SSL_CTX_ctrl( g_client_ssl_ctx[ index ], SSL_CTRL_SET_MIN_PROTO_VERSION, 0, NULL );
				_SSL_CTX_ctrl( g_client_ssl_ctx[ index ], SSL_CTRL_SET_MAX_PROTO_VERSION, protocol[ index ], NULL );

				_SSL_CTX_ctrl( g_client_ssl_ctx[ index ], SSL_CTRL_SET_SESS_CACHE_MODE, SSL_SESS_CACHE_CLIENT, NULL );
			}
			else if ( _SSL_CTX_set_min_proto_version != NULL &&
					  _SSL_CTX_set_max_proto_version != NULL &&
					  _SSL_CTX_set_session_cache_mode != NULL )	// BoringSSL functions.
			{
				_SSL_CTX_set_min_proto_version( g_client_ssl_ctx[ index ], 0 );
				_SSL_CTX_set_max_proto_version( g_client_ssl_ctx[ index ], ( uint16_t )protocol );

				_SSL_CTX_set_session_cache_mode( g_client_ssl_ctx[ index ], SSL_SESS_CACHE_CLIENT );
			}

			if ( _SSL_CTX_set_grease_enabled != NULL )
			{
				_SSL_CTX_set_grease_enabled( g_client_ssl_ctx[ index ], 1 );
			}

			_SSL_CTX_sess_set_new_cb( g_client_ssl_ctx[ index ], new_session_cb );
		}
	}
}

void FreeSSL_CTXs()
{
	for ( unsigned char index = 0; index < PROTOCOL_COUNT; ++index )
	{
		_SSL_CTX_free( g_client_ssl_ctx[ index ] );
		g_client_ssl_ctx[ index ] = NULL;
	}
}

void InitializeServerSSL_CTX( unsigned char ssl_version, unsigned char certificate_type )
{
	DWORD protocol = 0;

	switch ( ssl_version )
	{
		case 5:	protocol = TLS1_3_VERSION; break;
		case 4:	protocol = TLS1_2_VERSION; break;
		case 3:	protocol = TLS1_1_VERSION; break;
		case 2:	protocol = TLS1_VERSION; break;
		case 1:	protocol = SSL3_VERSION; break;
		case 0:	protocol = SSL2_VERSION; break;
	}

	const SSL_METHOD *ssl_method = _TLS_server_method();

	g_server_ssl_ctx = _SSL_CTX_new( ssl_method );
	if ( g_server_ssl_ctx != NULL )
	{
		if ( _SSL_CTX_ctrl != NULL )
		{
			_SSL_CTX_ctrl( g_server_ssl_ctx, SSL_CTRL_SET_MIN_PROTO_VERSION, 0, NULL );
			_SSL_CTX_ctrl( g_server_ssl_ctx, SSL_CTRL_SET_MAX_PROTO_VERSION, protocol, NULL );

			_SSL_CTX_ctrl( g_server_ssl_ctx, SSL_CTRL_SET_SESS_CACHE_MODE, SSL_SESS_CACHE_SERVER, NULL );
		}
		else if ( _SSL_CTX_set_min_proto_version != NULL &&
				  _SSL_CTX_set_max_proto_version != NULL &&
				  _SSL_CTX_set_session_cache_mode != NULL )	// BoringSSL functions.
		{
			_SSL_CTX_set_min_proto_version( g_server_ssl_ctx, 0 );
			_SSL_CTX_set_max_proto_version( g_server_ssl_ctx, ( uint16_t )protocol );

			_SSL_CTX_set_session_cache_mode( g_server_ssl_ctx, SSL_SESS_CACHE_SERVER );
		}

		if ( certificate_type == 1 )	// Public/Private Key Pair.
		{
			_SSL_CTX_use_certificate_file( g_server_ssl_ctx, g_certificate_cer_file_name, SSL_FILETYPE_PEM );
			_SSL_CTX_use_PrivateKey_file( g_server_ssl_ctx, g_certificate_key_file_name, SSL_FILETYPE_PEM );
		}
		else	// PKCS #12 File.
		{
			BIO *bio = NULL;
			PKCS12 *p12 = NULL;
			EVP_PKEY *key = NULL;
			X509 *cert = NULL;

			bio = _BIO_new_file( g_certificate_pkcs_file_name, "rb" );
			if ( bio != NULL )
			{
				p12 = _d2i_PKCS12_bio( bio, NULL );
				if ( p12 != NULL )
				{
					if ( _PKCS12_parse( p12, g_certificate_pkcs_password, &key, &cert, NULL ) != 0 )
					{
						_SSL_CTX_use_certificate( g_server_ssl_ctx, cert );
						_SSL_CTX_use_PrivateKey( g_server_ssl_ctx, key );
					}
				}
			}

			if ( cert != NULL ) { _X509_free( cert ); }
			if ( key != NULL ) { _EVP_PKEY_free( key ); }
			if ( p12 != NULL ) { _PKCS12_free( p12 ); }
			if ( bio != NULL ) { _BIO_free( bio ); }
		}
	}					
}

int WINAPIV new_session_cb( SSL *ssl, SSL_SESSION *sess )
{
	SOCKET_CONTEXT *context = ( SOCKET_CONTEXT * )_SSL_get_ex_data( ssl, 0 );
	if ( context != NULL )
	{
		EnterCriticalSection( &context->context_cs );

		if ( context->_ssl_o != NULL )
		{
			_SSL_SESSION_up_ref( sess );
			context->_ssl_o->ssl_session = sess;
		}

		LeaveCriticalSection( &context->context_cs );

		return 1;
	}

	return 0;
}

void OpenSSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent )
{
	sent = false;

	if ( context != NULL && context->_ssl_o != NULL && overlapped != NULL )
	{
		int nRet;
		DWORD dwFlags = 0;

		send_buf->buf = context->buffer;
		send_buf->len = context->buffer_size;

		// Read the encrypted data that's in our write BIO (created by a call to _SSL_write()) and send it through the socket.
		int read = _BIO_read( context->_ssl_o->wbio, send_buf->buf, send_buf->len );
		if ( read > 0 )
		{
			send_buf->len = read;

			sent = true;

			nRet = _WSASend( context->socket, send_buf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
			}
		}
	}
}

char OpenSSL_DecryptRecv( SOCKET_CONTEXT *context, DWORD &io_size )
{
	char content_status = CONTENT_STATUS_FAILED;

	DWORD bytes_decrypted = 0;

	if ( context != NULL && context->_ssl_o != NULL && context->_ssl_o->ssl != NULL )
	{
		if ( !context->_ssl_o->continue_decrypt )
		{
			// The data we received needs to be written to the read BIO for processing by the _SSL_() functions.
			/*int write = */_BIO_write( context->_ssl_o->rbio, context->wsabuf.buf, io_size );
		}
		else
		{
			context->_ssl_o->continue_decrypt = false;
		}

		int read;
		for ( ;; )
		{
			read = _SSL_read( context->_ssl_o->ssl, context->wsabuf.buf + bytes_decrypted, context->wsabuf.len - bytes_decrypted );
			if ( read < 0 )
			{
				break;
			}
			else if ( read == 0 )
			{
				context->_ssl_o->continue_decrypt = ( _SSL_pending( context->_ssl_o->ssl ) > 0 ? true : false );

				break;
			}

			bytes_decrypted += read;
		}

		if ( read <= 0 )
		{
			int error = _SSL_get_error( context->_ssl_o->ssl, read );

			if ( error == SSL_ERROR_WANT_READ )
			{
				content_status = CONTENT_STATUS_READ_MORE_CONTENT;
			}
			else if ( error == SSL_ERROR_WANT_WRITE ||
					  error == SSL_ERROR_SYSCALL )
			{
				int pending = _BIO_pending( context->_ssl_o->wbio );
				if ( pending > 0 )
				{
					bool sent = false;

					InterlockedIncrement( &context->pending_operations );

					context->overlapped.current_operation = IO_Write;
					context->overlapped.next_operation = ( error == SSL_ERROR_WANT_WRITE ? IO_GetContent : IO_Shutdown );

					OpenSSL_WSASend( context, &context->overlapped, &context->wsabuf, sent );
					if ( !sent )
					{
						InterlockedDecrement( &context->pending_operations );
					}
					else
					{
						content_status = CONTENT_STATUS_NONE;
					}
				}
			}
			/*else if ( error == SSL_ERROR_ZERO_RETURN )
			{
			}
			else	// Error.
			{
			}*/
		}
	}

	io_size = bytes_decrypted;

	return content_status;
}

void OpenSSL_FreeInfo( _SSL_O **_ssl_o )
{
	if ( *_ssl_o != NULL )
	{
		if ( ( *_ssl_o )->ssl != NULL ) { _SSL_free( ( *_ssl_o )->ssl ); }	// Will free the BIOs that have been assigned to it.
		if ( ( *_ssl_o )->ssl_session != NULL ) { _SSL_SESSION_free( ( *_ssl_o )->ssl_session ); }

		GlobalFree( *_ssl_o );

		*_ssl_o = NULL;
	}
}
