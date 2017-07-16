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

#include "ssl_client.h"

#include "lite_dlls.h"
#include "lite_ntdll.h"

#include "connection.h"

HMODULE g_hSecurity = NULL;
PSecurityFunctionTableA g_pSSPI;

typedef BOOL ( WINAPI *pSslEmptyCacheW )( LPSTR pszTargetName, DWORD dwFlags );
pSslEmptyCacheW	_SslEmptyCacheW;

unsigned char ssl_state = SSL_STATE_SHUTDOWN;

CredHandle g_hCreds;

void ResetCredentials()
{
	if ( SecIsValidHandle( &g_hCreds ) )
	{
		g_pSSPI->FreeCredentialsHandle( &g_hCreds );
		SecInvalidateHandle( &g_hCreds );
	}
}

int SSL_library_init( void )
{
	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		return 1;
	}

	g_hSecurity = LoadLibraryDEMW( L"schannel.dll" );
	if ( g_hSecurity == NULL )
	{
		return 0;
	}

	_SslEmptyCacheW = ( pSslEmptyCacheW )GetProcAddress( g_hSecurity, "SslEmptyCacheW" );
	if ( _SslEmptyCacheW == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	INIT_SECURITY_INTERFACE_A pInitSecurityInterface = ( INIT_SECURITY_INTERFACE_A )GetProcAddress( g_hSecurity, SECURITY_ENTRYPOINT_ANSIA );
	if ( pInitSecurityInterface != NULL )
	{
		g_pSSPI = pInitSecurityInterface();
	}

	if ( g_pSSPI == NULL )
	{
		FreeLibrary( g_hSecurity );
		g_hSecurity = NULL;
		return 0;
	}

	ssl_state = SSL_STATE_RUNNING;

	SecInvalidateHandle( &g_hCreds );
	
	return 1;
}

int SSL_library_uninit( void )
{
	BOOL ret = 0;

	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		ResetCredentials();

		_SslEmptyCacheW( NULL, 0 );

		ret = FreeLibrary( g_hSecurity );
	}

	ssl_state = SSL_STATE_SHUTDOWN;

	return ret;
}

SSL *SSL_new( DWORD protocol )
{
	if ( g_hSecurity == NULL )
	{
		return NULL;
	}

	SSL *ssl = ( SSL * )GlobalAlloc( GPTR, sizeof( SSL ) );
	if ( ssl == NULL )
	{
		return NULL;
	}

	//ssl->dwProtocol = protocol;

	if ( !SecIsValidHandle( &g_hCreds ) )
	{
		SCHANNEL_CRED SchannelCred;
		TimeStamp tsExpiry;
		SECURITY_STATUS scRet;

		_memzero( &SchannelCred, sizeof( SchannelCred ) );

		SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
		SchannelCred.grbitEnabledProtocols = protocol;//ssl->dwProtocol;
		SchannelCred.dwFlags = ( SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION );

		// Create an SSPI credential.
		scRet = g_pSSPI->AcquireCredentialsHandleA(
						NULL,
						UNISP_NAME_A,
						SECPKG_CRED_OUTBOUND,
						NULL,
						&SchannelCred,
						NULL,
						NULL,
						&g_hCreds,
						&tsExpiry );

		if ( scRet != SEC_E_OK )
		{
			ResetCredentials();

			GlobalFree( ssl ); 
			ssl = NULL;
		}
	}

	return ssl;
}

void SSL_free( SSL *ssl )
{
	if ( ssl == NULL )
	{
		return;
	}

	if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
	{
		g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
		ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
	}

	if ( ssl->cd.OutBuffers[ 0 ].pvBuffer != NULL )
	{
		g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
		ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;
	}

	if ( ssl->sd.pbDataBuffer != NULL )
	{
		GlobalFree( ssl->sd.pbDataBuffer );
		ssl->sd.pbDataBuffer = NULL;
	}

	if ( ssl->pbRecDataBuf != NULL )
	{
		GlobalFree( ssl->pbRecDataBuf );
		ssl->pbRecDataBuf = NULL;
	}

	if ( ssl->pbIoBuffer != NULL )
	{
		GlobalFree( ssl->pbIoBuffer );
		ssl->pbIoBuffer = NULL;
	}

	if ( SecIsValidHandle( &ssl->hContext ) )
	{
		g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		SecInvalidateHandle( &ssl->hContext );
	}

	GlobalFree( ssl );
}

SECURITY_STATUS SSL_WSAConnect( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, char *host, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		SecBufferDesc OutBuffer;
		//SecBuffer OutBuffers[ 1 ];
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;
		//DWORD cbData;

		DWORD dwFlags = 0;

		SSL *ssl = context->ssl;

		ssl->cbIoBuffer = 0;

		ssl->cd.fDoRead = true;

		ssl->cd.scRet = SEC_I_CONTINUE_NEEDED;
		scRet = ssl->cd.scRet;

		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	|
					  ISC_REQ_REPLAY_DETECT		|
					  ISC_REQ_CONFIDENTIALITY	|
					  ISC_RET_EXTENDED_ERROR	|
					  ISC_REQ_ALLOCATE_MEMORY	|
					  ISC_REQ_STREAM;

		// Initiate a ClientHello message and generate a token.

		ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;
		ssl->cd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		ssl->cd.OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = ssl->cd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		/*struct sockaddr_in sock;
		int slen = sizeof( struct sockaddr );
		_getpeername( ssl->s, ( struct sockaddr * ) &sock, &slen );*/

		scRet = g_pSSPI->InitializeSecurityContextA(
						&g_hCreds,
						NULL,
						host/*_inet_ntoa( sock.sin_addr )*/,
						dwSSPIFlags,
						0,
						SECURITY_NATIVE_DREP,
						NULL,
						0,
						&ssl->hContext,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		if ( scRet != SEC_I_CONTINUE_NEEDED )
		{
			return scRet;
		}

		ssl->cd.scRet = scRet;

		// Send response to server if there is one.
		if ( ssl->cd.OutBuffers[ 0 ].cbBuffer != 0 && ssl->cd.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			sent = true;

			context->wsabuf.buf = ( char * )ssl->cd.OutBuffers[ 0 ].pvBuffer;
			context->wsabuf.len = ssl->cd.OutBuffers[ 0 ].cbBuffer;

			overlapped->current_operation = IO_Write;

			int nRet = _WSASend( ssl->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;

				g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );

				g_pSSPI->DeleteSecurityContext( &ssl->hContext );
				SecInvalidateHandle( &ssl->hContext );

				ssl->cd.scRet = SEC_E_INTERNAL_ERROR;
				scRet = ssl->cd.scRet;
			}
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAConnect_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		WSABUF encrypted_buf;

		SSL *ssl = context->ssl;
		scRet = ssl->cd.scRet;

		// Free the output buffer from our SSL_WSAConnect.
		g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
		ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// Server requested client authentication. 
			if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS )
			{
				// Go around again.
				ssl->cd.fDoRead = false;
				ssl->cd.scRet = SEC_I_CONTINUE_NEEDED;
				
				return ssl->cd.scRet;
			}

			if ( scRet == SEC_I_CONTINUE_NEEDED )
			{
				// Copy any leftover data from the buffer, and go around again.
				if ( ssl->cd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
				{
					_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->cd.InBuffers[ 1 ].cbBuffer ), ssl->cd.InBuffers[ 1 ].cbBuffer );

					ssl->cbIoBuffer = ssl->cd.InBuffers[ 1 ].cbBuffer;
				}
				else
				{
					ssl->cbIoBuffer = 0;
				}
			}

			// Read server data
			if ( ssl->cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE )
			{
				if ( ssl->cd.fDoRead )
				{
					// If buffer not large enough reallocate buffer
					if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
					{
						ssl->sbIoBuffer += 2048;

						if ( ssl->pbIoBuffer == NULL )
						{
							ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
						}
						else
						{
							ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
						}
					}

					sent = true;

					encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
					encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;

					DWORD dwFlags = 0;
					int nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;

						if ( ssl->pbIoBuffer != NULL )
						{
							GlobalFree( ssl->pbIoBuffer );
							ssl->pbIoBuffer = NULL;
						}

						ssl->sbIoBuffer = 0;

						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// UPDATED AFTER COMPLETION IN THE REPLY CASE
					//ssl->cbIoBuffer += cbData;
				}
				else
				{
					ssl->cd.fDoRead = true;
				}
			}
			else
			{
				// Do not post the ssl->cbIoBuffer size.
				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
			}
		}
		else if ( scRet == SEC_E_OK )	// Handshake completed successfully.
		{
			// Store remaining data for further use
			if ( ssl->cd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
			{
				_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->cd.InBuffers[ 1 ].cbBuffer ), ssl->cd.InBuffers[ 1 ].cbBuffer );
				ssl->cbIoBuffer = ssl->cd.InBuffers[ 1 ].cbBuffer;
			}
			else
			{
				ssl->cbIoBuffer = 0;

				if ( ssl->pbIoBuffer != NULL )
				{
					GlobalFree( ssl->pbIoBuffer );
					ssl->pbIoBuffer = NULL;
				}

				ssl->sbIoBuffer = 0;
			}
		}
		else if ( FAILED( scRet ) )	// Check for fatal error.
		{
			g_pSSPI->DeleteSecurityContext( &ssl->hContext );
			SecInvalidateHandle( &ssl->hContext );
		}
	}

	// We've completed everything above. Return our final status.
	return scRet;
}

SECURITY_STATUS SSL_WSAConnect_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		SecBufferDesc InBuffer;
		SecBufferDesc OutBuffer;
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;

		DWORD dwFlags = 0;

		SSL *ssl = context->ssl;
		scRet = ssl->cd.scRet;

		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
					  ISC_REQ_REPLAY_DETECT     |
					  ISC_REQ_CONFIDENTIALITY   |
					  ISC_RET_EXTENDED_ERROR    |
					  ISC_REQ_ALLOCATE_MEMORY   |
					  ISC_REQ_STREAM;


		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS )
		{
			// Set up the input buffers. Buffer 0 is used to pass in data
			// received from the server. Schannel will consume some or all
			// of this. Leftover data (if any) will be placed in buffer 1 and
			// given a buffer type of SECBUFFER_EXTRA.

			ssl->cd.InBuffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
			ssl->cd.InBuffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
			ssl->cd.InBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;

			ssl->cd.InBuffers[ 1 ].pvBuffer = NULL;
			ssl->cd.InBuffers[ 1 ].cbBuffer = 0;
			ssl->cd.InBuffers[ 1 ].BufferType = SECBUFFER_EMPTY;

			InBuffer.cBuffers = 2;
			InBuffer.pBuffers = ssl->cd.InBuffers;
			InBuffer.ulVersion = SECBUFFER_VERSION;

			// Set up the output buffers. These are initialized to NULL
			// so as to make it less likely we'll attempt to free random
			// garbage later.

			ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;
			ssl->cd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
			ssl->cd.OutBuffers[ 0 ].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = ssl->cd.OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			ssl->cd.scRet = g_pSSPI->InitializeSecurityContextA(
							&g_hCreds,
							&ssl->hContext,
							NULL,
							dwSSPIFlags,
							0,
							SECURITY_NATIVE_DREP,
							&InBuffer,
							0,
							NULL,
							&OutBuffer,
							&dwSSPIOutFlags,
							&tsExpiry );

			scRet = ssl->cd.scRet;

			// If success (or if the error was one of the special extended ones), send the contents of the output buffer to the server.
			if ( scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED( scRet ) && ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) )
			{
				if ( ssl->cd.OutBuffers[ 0 ].cbBuffer != 0 && ssl->cd.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					sent = true;

					context->wsabuf.buf = ( char * )ssl->cd.OutBuffers[ 0 ].pvBuffer;
					context->wsabuf.len = ssl->cd.OutBuffers[ 0 ].cbBuffer;

					overlapped->current_operation = IO_Write;

					int nRet = _WSASend( ssl->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;

						g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
						ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;

						g_pSSPI->DeleteSecurityContext( &ssl->hContext );
						SecInvalidateHandle( &ssl->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// Freed in SSL_WSAConnect_Response (assuming we get a response).
					//g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
					//ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;

					return SEC_I_CONTINUE_NEEDED;
				}
				else if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE )	// Request more data until we get something.
				{
					if ( ssl->cd.OutBuffers[ 0 ].pvBuffer != NULL )
					{
						g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
						ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;
					}

					PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

					return scRet;
				}
				else if ( scRet == SEC_E_OK )
				{
					// Store remaining data for further use
					if ( ssl->cd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA )
					{
						_memmove( ssl->pbIoBuffer, ssl->pbIoBuffer + ( ssl->cbIoBuffer - ssl->cd.InBuffers[ 1 ].cbBuffer ), ssl->cd.InBuffers[ 1 ].cbBuffer );
						ssl->cbIoBuffer = ssl->cd.InBuffers[ 1 ].cbBuffer;
					}
					else
					{
						ssl->cbIoBuffer = 0;

						if ( ssl->pbIoBuffer != NULL )
						{
							GlobalFree( ssl->pbIoBuffer );
							ssl->pbIoBuffer = NULL;
						}

						ssl->sbIoBuffer = 0;
					}

					if ( ssl->cd.OutBuffers[ 0 ].pvBuffer != NULL )
					{
						g_pSSPI->FreeContextBuffer( ssl->cd.OutBuffers[ 0 ].pvBuffer );
						ssl->cd.OutBuffers[ 0 ].pvBuffer = NULL;
					}
				}
			}
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		DWORD dwType;

		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;

		SecBufferDesc OutBuffer;

		DWORD dwFlags = 0;

		SSL *ssl = context->ssl;

		if ( ssl == NULL )
		{
			return SOCKET_ERROR;
		}

		dwType = SCHANNEL_SHUTDOWN;

		ssl->sdd.OutBuffers[ 0 ].pvBuffer = &dwType;	// NEVER MAKE THIS NULL. System will break and restart.
		ssl->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		ssl->sdd.OutBuffers[ 0 ].cbBuffer = sizeof( dwType );

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = ssl->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		scRet = g_pSSPI->ApplyControlToken( &ssl->hContext, &OutBuffer );
		if ( FAILED( scRet ) )
		{
			ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;

			return scRet;
		}

		// Build an SSL close notify message.

		ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
		ssl->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		ssl->sdd.OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = ssl->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT	|
					  ASC_REQ_REPLAY_DETECT		|
					  ASC_REQ_CONFIDENTIALITY	|
					  ASC_REQ_EXTENDED_ERROR	|
					  ASC_REQ_ALLOCATE_MEMORY	|
					  ASC_REQ_STREAM;

		scRet = g_pSSPI->AcceptSecurityContext(
						&g_hCreds,
						&ssl->hContext,
						NULL,
						dwSSPIFlags,
						SECURITY_NATIVE_DREP,
						NULL,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		if ( FAILED( scRet ) )
		{
			if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
			{
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}

			return scRet;
		}

		// Send the close notify message to the server.
		if ( ssl->sdd.OutBuffers[ 0 ].pvBuffer != NULL && ssl->sdd.OutBuffers[ 0 ].cbBuffer != 0 )
		{
			sent = true;

			context->wsabuf.buf = ( char * )ssl->sdd.OutBuffers[ 0 ].pvBuffer;
			context->wsabuf.len = ssl->sdd.OutBuffers[ 0 ].cbBuffer;

			overlapped->current_operation = IO_Write;

			int nRet = _WSASend( ssl->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;

				scRet = SEC_E_INTERNAL_ERROR;
			}
			/*else	// Freed in SSL_free.
			{
				g_pSSPI->FreeContextBuffer( ssl->sdd.OutBuffers[ 0 ].pvBuffer );
				ssl->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}*/
		}

		// Freed in SSL_free.
		// Free the security context.
		//g_pSSPI->DeleteSecurityContext( &ssl->hContext );
		//SecInvalidateHandle( &ssl->hContext );
	}

	return scRet;
}

SECURITY_STATUS WSAAPI SSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		SSL *ssl = context->ssl;

		DWORD dwFlags = 0;

		SecBuffer Buffers[ 4 ];
		DWORD cbMessage;
		SecBufferDesc Message;

		// ssl->sd.pbDataBuffer is freed when we clean up the connection.
		if ( ssl->sd.pbDataBuffer == NULL )
		{
			scRet = g_pSSPI->QueryContextAttributesA( &ssl->hContext, SECPKG_ATTR_STREAM_SIZES, &ssl->sd.Sizes );
			if ( scRet != SEC_E_OK )
			{
				return scRet;
			}

			// The size includes the SSL header, max message length (16 KB), and SSL trailer.
			ssl->sd.pbDataBuffer = ( PUCHAR )GlobalAlloc( GPTR, ( ssl->sd.Sizes.cbHeader + ssl->sd.Sizes.cbMaximumMessage + ssl->sd.Sizes.cbTrailer ) );
		}

		// Copy our message to the buffer. Truncate the message if it's larger than the maximum allowed size (16 KB).
		cbMessage = min( ssl->sd.Sizes.cbMaximumMessage, ( DWORD )send_buf->len );
		_memcpy_s( ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader, ssl->sd.Sizes.cbMaximumMessage, send_buf->buf, cbMessage );

		send_buf->len -= cbMessage;
		send_buf->buf += cbMessage;

		// Header location. (Beginning of the data buffer).
		Buffers[ 0 ].pvBuffer = ssl->sd.pbDataBuffer;
		Buffers[ 0 ].cbBuffer = ssl->sd.Sizes.cbHeader;
		Buffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;

		// Message location. (After the header).
		Buffers[ 1 ].pvBuffer = ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader;
		Buffers[ 1 ].cbBuffer = cbMessage;
		Buffers[ 1 ].BufferType = SECBUFFER_DATA;

		// Trailer location. (After the message).
		Buffers[ 2 ].pvBuffer = ssl->sd.pbDataBuffer + ssl->sd.Sizes.cbHeader + cbMessage;
		Buffers[ 2 ].cbBuffer = ssl->sd.Sizes.cbTrailer;
		Buffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

		Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = Buffers;

		if ( g_pSSPI->EncryptMessage != NULL )
		{
			scRet = g_pSSPI->EncryptMessage( &ssl->hContext, 0, &Message, 0 );
		}
		else
		{
			scRet = ( ( ENCRYPT_MESSAGE_FN )g_pSSPI->Reserved3 )( &ssl->hContext, 0, &Message, 0 );
		}

		if ( FAILED( scRet ) )
		{
			return scRet;
		}

		sent = true;

		context->wsabuf.buf = ( char * )ssl->sd.pbDataBuffer;
		context->wsabuf.len = Buffers[ 0 ].cbBuffer + Buffers[ 1 ].cbBuffer + Buffers[ 2 ].cbBuffer; // Calculate encrypted packet size

		overlapped->current_operation = IO_Write;

		int nRet = _WSASend( ssl->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;

			g_pSSPI->DeleteSecurityContext( &ssl->hContext );
			SecInvalidateHandle( &ssl->hContext );

			scRet = SEC_E_INTERNAL_ERROR;
		}
	}
	
	return scRet;
}


SECURITY_STATUS WSAAPI SSL_WSARecv( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	sent = false;

	if ( context != NULL && context->ssl != NULL && overlapped != NULL )
	{
		SSL *ssl = context->ssl;

		DWORD dwFlags = 0;
		WSABUF encrypted_buf;
		int nRet = 0;

		if ( ssl->cbIoBuffer == 0 || ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			if ( ssl->sbIoBuffer <= ssl->cbIoBuffer )
			{
				ssl->sbIoBuffer += 2048;

				if ( ssl->pbIoBuffer == NULL )
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbIoBuffer );
				}
				else
				{
					ssl->pbIoBuffer = ( PUCHAR )GlobalReAlloc( ssl->pbIoBuffer, ssl->sbIoBuffer, GMEM_MOVEABLE );
				}
			}

			sent = true;

			encrypted_buf.buf = ( char * )ssl->pbIoBuffer + ssl->cbIoBuffer;
			encrypted_buf.len = ssl->sbIoBuffer - ssl->cbIoBuffer;

			nRet = _WSARecv( ssl->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
				ssl->rd.scRet = SEC_E_INTERNAL_ERROR;

				return ssl->rd.scRet;
			}

			return SEC_E_OK;
		}

		return ssl->rd.scRet;
	}
	else
	{
		return SEC_E_INTERNAL_ERROR;
	}
}

SECURITY_STATUS SSL_WSARecv_Decrypt( SSL *ssl, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecrypted )
{
	lpNumberOfBytesDecrypted = 0;
	SecBufferDesc Message;
	SecBuffer *pDataBuffer;
	SecBuffer *pExtraBuffer;

	if ( ssl == NULL )
	{
		return -1;
	}

	if ( ssl->rd.scRet == SEC_I_CONTINUE_NEEDED )
	{
		// Handle any remaining data that was already decoded.
		if ( lpBuffers->buf != NULL && lpBuffers->len > 0 && ssl->cbRecDataBuf > 0 )
		{
			lpNumberOfBytesDecrypted = min( ( DWORD )lpBuffers->len, ssl->cbRecDataBuf );
			_memcpy_s( lpBuffers->buf, lpBuffers->len, ssl->pbRecDataBuf, lpNumberOfBytesDecrypted );

			DWORD rbytes = ssl->cbRecDataBuf - lpNumberOfBytesDecrypted;
			if ( rbytes > 0 )
			{
				_memmove( ssl->pbRecDataBuf, ( ( char * )ssl->pbRecDataBuf ) + lpNumberOfBytesDecrypted, rbytes );
			}
			else
			{
				ssl->rd.scRet = SEC_E_OK;
			}
			ssl->cbRecDataBuf = rbytes;
		}

		return ssl->rd.scRet;
	}

	_memzero( ssl->rd.Buffers, sizeof( SecBuffer ) * 4 );

	//ssl->cbIoBuffer = lpBuffers->len;

	// Attempt to decrypt the received data.
	ssl->rd.Buffers[ 0 ].pvBuffer = ssl->pbIoBuffer;
	ssl->rd.Buffers[ 0 ].cbBuffer = ssl->cbIoBuffer;
	ssl->rd.Buffers[ 0 ].BufferType = SECBUFFER_DATA;

	ssl->rd.Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
	ssl->rd.Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
	ssl->rd.Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = ssl->rd.Buffers;

	if ( g_pSSPI->DecryptMessage != NULL )
	{
		ssl->rd.scRet = g_pSSPI->DecryptMessage( &ssl->hContext, &Message, 0, NULL );
	}
	else
	{
		ssl->rd.scRet = ( ( DECRYPT_MESSAGE_FN )g_pSSPI->Reserved4 )( &ssl->hContext, &Message, 0, NULL );
	}

	if ( ssl->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		// The input buffer contains only a fragment of an encrypted record. Need to read some more data.
		return ssl->rd.scRet;
	}

	// Sender has signaled end of session
	if ( ssl->rd.scRet == SEC_I_CONTEXT_EXPIRED )
	{
		return ssl->rd.scRet;
	}

	if ( ssl->rd.scRet != SEC_E_OK && ssl->rd.scRet != SEC_I_RENEGOTIATE )
	{
		return ssl->rd.scRet;
	}

	// Locate data and (optional) extra buffers.
	pDataBuffer  = NULL;
	pExtraBuffer = NULL;
	for ( int i = 1; i < 4; i++ )
	{
		if ( pDataBuffer == NULL && ssl->rd.Buffers[ i ].BufferType == SECBUFFER_DATA )
		{
			pDataBuffer = &ssl->rd.Buffers[ i ];
		}

		if ( pExtraBuffer == NULL && ssl->rd.Buffers[ i ].BufferType == SECBUFFER_EXTRA )
		{
			pExtraBuffer = &ssl->rd.Buffers[ i ];
		}
	}

	// Return decrypted data.
	if ( pDataBuffer != NULL )
	{
		lpNumberOfBytesDecrypted = min( ( DWORD )lpBuffers->len, pDataBuffer->cbBuffer );
		_memcpy_s( lpBuffers->buf, lpBuffers->len, pDataBuffer->pvBuffer, lpNumberOfBytesDecrypted );

		// Remaining bytes.
		DWORD rbytes = pDataBuffer->cbBuffer - lpNumberOfBytesDecrypted;
		if ( rbytes > 0 )
		{
			if ( ssl->sbRecDataBuf < rbytes ) 
			{
				ssl->sbRecDataBuf = rbytes;

				if ( ssl->pbRecDataBuf == NULL )
				{
					ssl->pbRecDataBuf = ( PUCHAR )GlobalAlloc( GPTR, ssl->sbRecDataBuf );
				}
				else
				{
					ssl->pbRecDataBuf = ( PUCHAR )GlobalReAlloc( ssl->pbRecDataBuf, ssl->sbRecDataBuf, GMEM_MOVEABLE );
				}
			}
	
			_memcpy_s( ssl->pbRecDataBuf, ssl->sbRecDataBuf, ( char * )pDataBuffer->pvBuffer + lpNumberOfBytesDecrypted, rbytes );
			ssl->cbRecDataBuf = rbytes;

			ssl->rd.scRet = SEC_I_CONTINUE_NEEDED;
		}
	}

	// Move any extra data to the input buffer.
	if ( pExtraBuffer != NULL )
	{
		_memmove( ssl->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer );
		ssl->cbIoBuffer = pExtraBuffer->cbBuffer;
	}
	else
	{
		ssl->cbIoBuffer = 0;
	}

	/*if ( pDataBuffer != NULL && lpNumberOfBytesDecrypted != 0 )
	{
		return ssl->rd.scRet;
	}

	if ( ssl->rd.scRet == SEC_I_RENEGOTIATE )
	{
		return ssl->rd.scRet;
	}*/

	return ssl->rd.scRet;
}
