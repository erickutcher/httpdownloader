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

#include "ssl_schannel.h"

#include "lite_dlls.h"
#include "lite_ntdll.h"
#include "lite_rpcrt4.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"

#include "connection.h"

HMODULE g_hSecurity = NULL;
PSecurityFunctionTableA g_pSSPI;

typedef BOOL ( WINAPI *pSslEmptyCacheW )( LPSTR pszTargetName, DWORD dwFlags );
pSslEmptyCacheW	_SslEmptyCacheW;

unsigned char ssl_state = SSL_STATE_SHUTDOWN;

CredHandle g_hCreds_server;
CredHandle g_hCreds_client[ PROTOCOL_COUNT ];

void ResetServerCredentials()
{
	if ( SecIsValidHandle( &g_hCreds_server ) )
	{
		if ( g_pSSPI != NULL )
		{
			g_pSSPI->FreeCredentialsHandle( &g_hCreds_server );
		}
		SecInvalidateHandle( &g_hCreds_server );
	}
}

void ResetClientCredentials( unsigned char index )
{
	if ( SecIsValidHandle( &g_hCreds_client[ index ] ) )
	{
		if ( g_pSSPI != NULL )
		{
			g_pSSPI->FreeCredentialsHandle( &g_hCreds_client[ index ] );
		}
		SecInvalidateHandle( &g_hCreds_client[ index ] );
	}
}

int __SSL_library_init( void )
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

	SecInvalidateHandle( &g_hCreds_server );
	for ( unsigned char index = 0; index < PROTOCOL_COUNT; ++index )
	{
		SecInvalidateHandle( &g_hCreds_client[ index ] );
	}

	return 1;
}

int __SSL_library_uninit( void )
{
	BOOL ret = 0;

	if ( ssl_state != SSL_STATE_SHUTDOWN )
	{
		ResetServerCredentials();
		for ( unsigned char index = 0; index < PROTOCOL_COUNT; ++index )
		{
			ResetClientCredentials( index );
		}

		_SslEmptyCacheW( NULL, 0 );

		ret = FreeLibrary( g_hSecurity );
	}

	ssl_state = SSL_STATE_SHUTDOWN;

	return ret;
}

_SSL_S *__SSL_new( DWORD protocol, bool is_server )
{
	SCH_CREDENTIALS sch_cred_new;
	SCHANNEL_CRED sch_cred_old;
	PVOID *SchannelCred = NULL;

	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;

	if ( g_pSSPI == NULL )
	{
		return NULL;
	}

	_SSL_S *_ssl_s = ( _SSL_S * )GlobalAlloc( GPTR, sizeof( _SSL_S ) );
	if ( _ssl_s == NULL )
	{
		return NULL;
	}

	_ssl_s->is_server = is_server;

	if ( is_server )
	{
		if ( !SecIsValidHandle( &g_hCreds_server ) )
		{
			if ( g_pCertContext != NULL )
			{
				if ( g_can_use_tls_1_3 )
				{
					_memzero( &sch_cred_new, sizeof( SCH_CREDENTIALS ) );

					sch_cred_new.dwVersion = SCH_CREDENTIALS_VERSION;
					sch_cred_new.cCreds = 1;
					sch_cred_new.paCred = &g_pCertContext;
					sch_cred_new.dwFlags = ( SCH_CRED_NO_SYSTEM_MAPPER | SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_REVOCATION_CHECK_CHAIN );
					sch_cred_new.cTlsParameters = 1;

					TLS_PARAMETERS tlsp;
					_memzero( &tlsp, sizeof( TLS_PARAMETERS ) );
					tlsp.grbitDisabledProtocols = ~protocol;

					sch_cred_new.pTlsParameters = ( TLS_PARAMETERS * )&tlsp;

					SchannelCred = ( PVOID * )&sch_cred_new;
				}
				else
				{
					_memzero( &sch_cred_old, sizeof( SCHANNEL_CRED ) );

					sch_cred_old.dwVersion = SCHANNEL_CRED_VERSION;
					sch_cred_old.cCreds = 1;
					sch_cred_old.paCred = &g_pCertContext;
					sch_cred_old.dwMinimumCipherStrength = ( DWORD )-1;
					sch_cred_old.grbitEnabledProtocols = protocol;
					sch_cred_old.dwFlags = ( SCH_CRED_NO_SYSTEM_MAPPER | SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_REVOCATION_CHECK_CHAIN );

					SchannelCred = ( PVOID * )&sch_cred_old;
				}

				scRet = g_pSSPI->AcquireCredentialsHandleA(
								NULL,
								UNISP_NAME_A,
								SECPKG_CRED_INBOUND,
								NULL,
								SchannelCred,
								NULL,
								NULL,
								&g_hCreds_server,
								&tsExpiry );
			}
			else
			{
				scRet = ~SEC_E_OK;
			}

			if ( scRet != SEC_E_OK )
			{
				ResetServerCredentials();

				GlobalFree( _ssl_s );
				_ssl_s = NULL;
			}
		}
	}
	else
	{
		if		( protocol & SP_PROT_TLS1_3_CLIENT ) { _ssl_s->protocol_index = 5; }
		else if ( protocol & SP_PROT_TLS1_2_CLIENT ) { _ssl_s->protocol_index = 4; }
		else if ( protocol & SP_PROT_TLS1_1_CLIENT ) { _ssl_s->protocol_index = 3; }
		else if ( protocol & SP_PROT_TLS1_CLIENT ) { _ssl_s->protocol_index = 2; }
		else if ( protocol & SP_PROT_SSL3_CLIENT ) { _ssl_s->protocol_index = 1; }
		//else if ( protocol & SP_PROT_SSL2_CLIENT ) { _ssl_s->protocol_index = 0; }
		
		if ( !SecIsValidHandle( &g_hCreds_client[ _ssl_s->protocol_index ] ) )
		{
			if ( g_can_use_tls_1_3 )
			{
				_memzero( &sch_cred_new, sizeof( SCH_CREDENTIALS ) );

				sch_cred_new.dwVersion = SCH_CREDENTIALS_VERSION;
				sch_cred_new.dwFlags = ( SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION );
				sch_cred_new.cTlsParameters = 1;

				TLS_PARAMETERS tlsp;
				_memzero( &tlsp, sizeof( TLS_PARAMETERS ) );
				tlsp.grbitDisabledProtocols = ~protocol;

				sch_cred_new.pTlsParameters = ( TLS_PARAMETERS * )&tlsp;

				SchannelCred = ( PVOID * )&sch_cred_new;
			}
			else
			{
				_memzero( &sch_cred_old, sizeof( SCHANNEL_CRED ) );

				sch_cred_old.dwVersion = SCHANNEL_CRED_VERSION;
				sch_cred_old.grbitEnabledProtocols = protocol;
				sch_cred_old.dwFlags = ( SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION );

				SchannelCred = ( PVOID * )&sch_cred_old;
			}

			// Create an SSPI credential.
			scRet = g_pSSPI->AcquireCredentialsHandleA(
							NULL,
							UNISP_NAME_A,
							SECPKG_CRED_OUTBOUND,
							NULL,
							SchannelCred,
							NULL,
							NULL,
							&g_hCreds_client[ _ssl_s->protocol_index ],
							&tsExpiry );

			if ( scRet != SEC_E_OK )
			{
				ResetClientCredentials( _ssl_s->protocol_index );

				GlobalFree( _ssl_s ); 
				_ssl_s = NULL;
			}
		}
	}

	return _ssl_s;
}

void __SSL_free( _SSL_S *_ssl_s )
{
	if ( _ssl_s == NULL )
	{
		return;
	}

	if ( g_pSSPI != NULL )
	{
		if ( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			g_pSSPI->FreeContextBuffer( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer );
			_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
		}

		/*if ( _ssl_s->ad.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			g_pSSPI->FreeContextBuffer( _ssl_s->ad.OutBuffers[ 0 ].pvBuffer );
			_ssl_s->ad.OutBuffers[ 0 ].pvBuffer = NULL;
		}

		if ( _ssl_s->cd.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			g_pSSPI->FreeContextBuffer( _ssl_s->cd.OutBuffers[ 0 ].pvBuffer );
			_ssl_s->cd.OutBuffers[ 0 ].pvBuffer = NULL;
		}*/

		if ( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
			_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
		}

		if ( SecIsValidHandle( &_ssl_s->hContext ) )
		{
			g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
			SecInvalidateHandle( &_ssl_s->hContext );
		}
	}

	if ( _ssl_s->sd.pbDataBuffer != NULL )
	{
		GlobalFree( _ssl_s->sd.pbDataBuffer );
		_ssl_s->sd.pbDataBuffer = NULL;
	}

	if ( _ssl_s->pbRecDataBuf != NULL )
	{
		GlobalFree( _ssl_s->pbRecDataBuf );
		_ssl_s->pbRecDataBuf = NULL;
	}

	if ( _ssl_s->pbIoBuffer != NULL )
	{
		GlobalFree( _ssl_s->pbIoBuffer );
		_ssl_s->pbIoBuffer = NULL;
	}

	GlobalFree( _ssl_s );
}

SECURITY_STATUS SSL_WSAAccept( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL )
	{
		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;

		// Begin our handshake.
		_ssl_s->acd.fInitContext = true;

		_ssl_s->cbIoBuffer = 0;

		_ssl_s->acd.fDoRead = true;

		_ssl_s->acd.scRet = SEC_I_CONTINUE_NEEDED;
		scRet = _ssl_s->acd.scRet;

		WSABUF encrypted_buf;

		// If buffer not large enough reallocate buffer
		if ( _ssl_s->sbIoBuffer <= _ssl_s->cbIoBuffer )
		{
			_ssl_s->sbIoBuffer += BUFFER_SIZE;

			if ( _ssl_s->pbIoBuffer == NULL )
			{
				_ssl_s->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, _ssl_s->sbIoBuffer );
			}
			else
			{
				_ssl_s->pbIoBuffer = ( PUCHAR )GlobalReAlloc( _ssl_s->pbIoBuffer, _ssl_s->sbIoBuffer, GMEM_MOVEABLE );
			}
		}

		if ( _ssl_s->pbIoBuffer != NULL )
		{
			sent = true;

			encrypted_buf.buf = ( char * )_ssl_s->pbIoBuffer + _ssl_s->cbIoBuffer;
			encrypted_buf.len = _ssl_s->sbIoBuffer - _ssl_s->cbIoBuffer;

			nRet = _WSARecv( _ssl_s->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
			}
		}

		if ( !sent )
		{
			_ssl_s->acd.scRet = SEC_E_INTERNAL_ERROR;
			scRet = _ssl_s->acd.scRet;
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		SecBufferDesc InBuffer;
		SecBufferDesc OutBuffer;
		TimeStamp tsExpiry;
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;
		scRet = _ssl_s->acd.scRet;

		dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT	|
					  ASC_REQ_REPLAY_DETECT		|
					  ASC_REQ_CONFIDENTIALITY	|
					  ASC_REQ_EXTENDED_ERROR	|
					  ASC_REQ_ALLOCATE_MEMORY	|
					  ASC_REQ_STREAM;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// Set up the input buffers. Buffer 0 is used to pass in data
			// received from the server. Schannel will consume some or all
			// of this. Leftover data (if any) will be placed in buffer 1 and
			// given a buffer type of SECBUFFER_EXTRA.

			_ssl_s->acd.InBuffers[ 0 ].pvBuffer = _ssl_s->pbIoBuffer;
			_ssl_s->acd.InBuffers[ 0 ].cbBuffer = _ssl_s->cbIoBuffer;
			_ssl_s->acd.InBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;

			_ssl_s->acd.InBuffers[ 1 ].pvBuffer = NULL;
			_ssl_s->acd.InBuffers[ 1 ].cbBuffer = 0;
			_ssl_s->acd.InBuffers[ 1 ].BufferType = SECBUFFER_EMPTY;

			InBuffer.cBuffers = 2;
			InBuffer.pBuffers = _ssl_s->acd.InBuffers;
			InBuffer.ulVersion = SECBUFFER_VERSION;

			// Set up the output buffers. These are initialized to NULL
			// so as to make it less likely we'll attempt to free random
			// garbage later.

			_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
			_ssl_s->acd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
			_ssl_s->acd.OutBuffers[ 0 ].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = _ssl_s->acd.OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			/*if ( !_ssl_s->acd.fInitContext )
			{
				dwSSPIFlags |= ASC_REQ_MUTUAL_AUTH;
			}*/

			_ssl_s->acd.scRet = g_pSSPI->AcceptSecurityContext(
									&g_hCreds_server,
									( _ssl_s->acd.fInitContext ? NULL : &_ssl_s->hContext ),
									&InBuffer,
									dwSSPIFlags,
									SECURITY_NATIVE_DREP,
									( _ssl_s->acd.fInitContext ? &_ssl_s->hContext : NULL ),
									&OutBuffer,
									&dwSSPIOutFlags,
									&tsExpiry );

			scRet = _ssl_s->acd.scRet;

			_ssl_s->acd.fInitContext = false;

			// If success (or if the error was one of the special extended ones), send the contents of the output buffer to the client.
			if ( scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED( scRet ) && ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) )
			{
				if ( _ssl_s->acd.OutBuffers[ 0 ].cbBuffer != 0 && _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					sent = true;

					context->wsabuf.buf = ( char * )_ssl_s->acd.OutBuffers[ 0 ].pvBuffer;
					context->wsabuf.len = _ssl_s->acd.OutBuffers[ 0 ].cbBuffer;

					overlapped->current_operation = IO_Write;

					nRet = _WSASend( _ssl_s->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;
						g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
						_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
						g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
						SecInvalidateHandle( &_ssl_s->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// _ssl_s->acd.OutBuffers[ 0 ].pvBuffer is freed in SSL_WSAAccept_Response (assuming we get a response).

					return SEC_I_CONTINUE_NEEDED;
				}
				else if ( scRet == SEC_E_OK )
				{
					// Store remaining data for further use
					if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )	// The extra data is actually the HTTP request.
					{
						_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );
						_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;
					}
					else
					{
						_ssl_s->cbIoBuffer = 0;

						if ( _ssl_s->pbIoBuffer != NULL )
						{
							GlobalFree( _ssl_s->pbIoBuffer );
							_ssl_s->pbIoBuffer = NULL;
						}

						_ssl_s->sbIoBuffer = 0;
					}

					if ( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
					{
						g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
						_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
					}
				}
			}
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		WSABUF encrypted_buf;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;
		scRet = _ssl_s->acd.scRet;

		// Created in our call to SSL_WSAAccept_Reply.
		g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
		_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// Server just requested client authentication. 
			if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS )
			{
				// Go around again.
				_ssl_s->acd.fDoRead = false;
				_ssl_s->acd.scRet = SEC_I_CONTINUE_NEEDED;

				return _ssl_s->acd.scRet;
			}

			// We need to read more data from the server and try again.
			if ( scRet == SEC_E_INCOMPLETE_MESSAGE ) 
			{
				return scRet;
			}

			// Copy any leftover data from the buffer, and go around again.
			if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )
			{
				_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );

				_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;
			}
			else
			{
				_ssl_s->cbIoBuffer = 0;
			}

			// Read client data.
			if ( _ssl_s->cbIoBuffer == 0 )
			{
				if ( _ssl_s->acd.fDoRead )
				{
					// Reallocate the buffer if it needs to be larger.
					if ( _ssl_s->sbIoBuffer <= _ssl_s->cbIoBuffer )
					{
						_ssl_s->sbIoBuffer += BUFFER_SIZE;

						if ( _ssl_s->pbIoBuffer == NULL )
						{
							_ssl_s->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, _ssl_s->sbIoBuffer );
						}
						else
						{
							_ssl_s->pbIoBuffer = ( PUCHAR )GlobalReAlloc( _ssl_s->pbIoBuffer, _ssl_s->sbIoBuffer, GMEM_MOVEABLE );
						}
					}

					if ( _ssl_s->pbIoBuffer != NULL )
					{
						sent = true;

						encrypted_buf.buf = ( char * )_ssl_s->pbIoBuffer + _ssl_s->cbIoBuffer;
						encrypted_buf.len = _ssl_s->sbIoBuffer - _ssl_s->cbIoBuffer;

						nRet = _WSARecv( _ssl_s->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							sent = false;
						}
					}

					if ( !sent )
					{
						if ( _ssl_s->pbIoBuffer != NULL )
						{
							GlobalFree( _ssl_s->pbIoBuffer );
							_ssl_s->pbIoBuffer = NULL;
						}

						_ssl_s->sbIoBuffer = 0;

						g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
						SecInvalidateHandle( &_ssl_s->hContext );

						return SEC_E_INTERNAL_ERROR;
					}
				}
				else
				{
					_ssl_s->acd.fDoRead = true;
				}
			}
		}
		else if ( scRet == SEC_E_OK )	// Handshake completed successfully.
		{
			// Store remaining data for further use
			if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )	// The extra data is actually the HTTP request.
			{
				_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );
				_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;
			}
			else
			{
				_ssl_s->cbIoBuffer = 0;

				if ( _ssl_s->pbIoBuffer != NULL )
				{
					GlobalFree( _ssl_s->pbIoBuffer );
					_ssl_s->pbIoBuffer = NULL;
				}

				_ssl_s->sbIoBuffer = 0;
			}
		}
		else if ( FAILED( scRet ) )	// Delete the security context in the case of a fatal error.
		{
			g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
			SecInvalidateHandle( &_ssl_s->hContext );
		}
	}

	// We've completed everything above. Return our final status.
	return scRet;
}

SECURITY_STATUS SSL_WSAConnect( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, char *host, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		SecBufferDesc OutBuffer;
		//SecBuffer OutBuffers[ 1 ];
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;
		//DWORD cbData;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;

		_ssl_s->cbIoBuffer = 0;

		_ssl_s->acd.fDoRead = true;

		_ssl_s->acd.scRet = SEC_I_CONTINUE_NEEDED;
		scRet = _ssl_s->acd.scRet;

		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	|
					  ISC_REQ_REPLAY_DETECT		|
					  ISC_REQ_CONFIDENTIALITY	|
					  ISC_RET_EXTENDED_ERROR	|
					  ISC_REQ_ALLOCATE_MEMORY	|
					  ISC_REQ_STREAM;

		// Initiate a ClientHello message and generate a token.

		_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
		_ssl_s->acd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		_ssl_s->acd.OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = _ssl_s->acd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		/*struct sockaddr_in sock;
		int slen = sizeof( struct sockaddr );
		_getpeername( _ssl_s->s, ( struct sockaddr * ) &sock, &slen );*/

		scRet = g_pSSPI->InitializeSecurityContextA(
						&g_hCreds_client[ _ssl_s->protocol_index ],
						NULL,
						host/*_inet_ntoa( sock.sin_addr )*/,
						dwSSPIFlags,
						0,
						SECURITY_NATIVE_DREP,
						NULL,
						0,
						&_ssl_s->hContext,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		if ( scRet != SEC_I_CONTINUE_NEEDED )
		{
			return scRet;
		}

		_ssl_s->acd.scRet = scRet;

		// Send response to server if there is one.
		if ( _ssl_s->acd.OutBuffers[ 0 ].cbBuffer != 0 && _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
		{
			sent = true;

			context->wsabuf.buf = ( char * )_ssl_s->acd.OutBuffers[ 0 ].pvBuffer;
			context->wsabuf.len = _ssl_s->acd.OutBuffers[ 0 ].cbBuffer;

			overlapped->current_operation = IO_Write;

			nRet = _WSASend( _ssl_s->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;

				g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );

				g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
				SecInvalidateHandle( &_ssl_s->hContext );

				_ssl_s->acd.scRet = SEC_E_INTERNAL_ERROR;
				scRet = _ssl_s->acd.scRet;
			}
		}
	}

	return scRet;
}

SECURITY_STATUS SSL_WSAConnect_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		WSABUF encrypted_buf;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;
		scRet = _ssl_s->acd.scRet;

		// Free the output buffer from our SSL_WSAConnect.
		g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
		_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;

		if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS ) 
		{
			// Server requested client authentication. 
			if ( scRet == SEC_I_INCOMPLETE_CREDENTIALS )
			{
				// Go around again.
				_ssl_s->acd.fDoRead = false;
				_ssl_s->acd.scRet = SEC_I_CONTINUE_NEEDED;
				
				return _ssl_s->acd.scRet;
			}

			if ( scRet == SEC_I_CONTINUE_NEEDED )
			{
				// Copy any leftover data from the buffer, and go around again.
				if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )
				{
					_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );

					_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;
				}
				else
				{
					_ssl_s->cbIoBuffer = 0;
				}
			}

			// Read server data
			if ( _ssl_s->cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE )
			{
				if ( _ssl_s->acd.fDoRead )
				{
					// If buffer not large enough reallocate buffer
					if ( _ssl_s->sbIoBuffer <= _ssl_s->cbIoBuffer )
					{
						_ssl_s->sbIoBuffer += BUFFER_SIZE;

						if ( _ssl_s->pbIoBuffer == NULL )
						{
							_ssl_s->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, _ssl_s->sbIoBuffer );
						}
						else
						{
							_ssl_s->pbIoBuffer = ( PUCHAR )GlobalReAlloc( _ssl_s->pbIoBuffer, _ssl_s->sbIoBuffer, GMEM_MOVEABLE );
						}
					}

					if ( _ssl_s->pbIoBuffer != NULL )
					{
						sent = true;

						encrypted_buf.buf = ( char * )_ssl_s->pbIoBuffer + _ssl_s->cbIoBuffer;
						encrypted_buf.len = _ssl_s->sbIoBuffer - _ssl_s->cbIoBuffer;

						nRet = _WSARecv( _ssl_s->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
						if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
						{
							sent = false;
						}
					}

					if ( !sent )
					{
						if ( _ssl_s->pbIoBuffer != NULL )
						{
							GlobalFree( _ssl_s->pbIoBuffer );
							_ssl_s->pbIoBuffer = NULL;
						}

						_ssl_s->sbIoBuffer = 0;

						g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
						SecInvalidateHandle( &_ssl_s->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// UPDATED AFTER COMPLETION IN THE REPLY CASE
					//_ssl_s->cbIoBuffer += cbData;
				}
				else
				{
					_ssl_s->acd.fDoRead = true;
				}
			}
			else
			{
				sent = true;

				// Do not post the _ssl_s->cbIoBuffer size.
				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );
			}
		}
		else if ( scRet == SEC_E_OK )	// Handshake completed successfully.
		{
			// Store remaining data for further use
			if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )
			{
				_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );
				_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;

				_ssl_s->acd.scRet = SEC_I_RENEGOTIATE;	// InitializeSecurityContext will reset this value inside SSL_WSAConnect_Reply().

				scRet = SSL_WSAConnect_Reply( context, overlapped, sent );
			}
			else
			{
				_ssl_s->cbIoBuffer = 0;

				if ( _ssl_s->pbIoBuffer != NULL )
				{
					GlobalFree( _ssl_s->pbIoBuffer );
					_ssl_s->pbIoBuffer = NULL;
				}

				_ssl_s->sbIoBuffer = 0;
			}
		}
		else if ( FAILED( scRet ) )	// Check for fatal error.
		{
			g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
			SecInvalidateHandle( &_ssl_s->hContext );
		}
	}

	// We've completed everything above. Return our final status.
	return scRet;
}

SECURITY_STATUS SSL_WSAConnect_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		SecBufferDesc InBuffer;
		SecBufferDesc OutBuffer;
		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;
		scRet = _ssl_s->acd.scRet;

		dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
					  ISC_REQ_REPLAY_DETECT     |
					  ISC_REQ_CONFIDENTIALITY   |
					  ISC_RET_EXTENDED_ERROR    |
					  ISC_REQ_ALLOCATE_MEMORY   |
					  ISC_REQ_STREAM;

		// SEC_I_RENEGOTIATE is not a return value of InitializeSecurityContext(),
		// but DecryptMessage() might return it and we'll need to perform the handshake again.
		// DecryptMessage() will return a SEC_I_RENEGOTIATE for TLS 1.3 connections.
		if ( scRet == SEC_I_CONTINUE_NEEDED ||
			 scRet == SEC_E_INCOMPLETE_MESSAGE ||
			 scRet == SEC_I_INCOMPLETE_CREDENTIALS ||
			 scRet == SEC_I_RENEGOTIATE )
		{
			// Set up the input buffers. Buffer 0 is used to pass in data
			// received from the server. Schannel will consume some or all
			// of this. Leftover data (if any) will be placed in buffer 1 and
			// given a buffer type of SECBUFFER_EXTRA.

			_ssl_s->acd.InBuffers[ 0 ].pvBuffer = _ssl_s->pbIoBuffer;
			_ssl_s->acd.InBuffers[ 0 ].cbBuffer = _ssl_s->cbIoBuffer;
			_ssl_s->acd.InBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;

			_ssl_s->acd.InBuffers[ 1 ].pvBuffer = NULL;
			_ssl_s->acd.InBuffers[ 1 ].cbBuffer = 0;
			_ssl_s->acd.InBuffers[ 1 ].BufferType = SECBUFFER_EMPTY;

			InBuffer.cBuffers = 2;
			InBuffer.pBuffers = _ssl_s->acd.InBuffers;
			InBuffer.ulVersion = SECBUFFER_VERSION;

			// Set up the output buffers. These are initialized to NULL
			// so as to make it less likely we'll attempt to free random
			// garbage later.

			_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
			_ssl_s->acd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
			_ssl_s->acd.OutBuffers[ 0 ].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = _ssl_s->acd.OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			_ssl_s->acd.scRet = g_pSSPI->InitializeSecurityContextA(
							&g_hCreds_client[ _ssl_s->protocol_index ],
							&_ssl_s->hContext,
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

			scRet = _ssl_s->acd.scRet;

			// If success (or if the error was one of the special extended ones), send the contents of the output buffer to the server.
			if ( scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED( scRet ) && ( dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR ) )
			{
				if ( _ssl_s->acd.OutBuffers[ 0 ].cbBuffer != 0 && _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					sent = true;

					context->wsabuf.buf = ( char * )_ssl_s->acd.OutBuffers[ 0 ].pvBuffer;
					context->wsabuf.len = _ssl_s->acd.OutBuffers[ 0 ].cbBuffer;

					overlapped->current_operation = IO_Write;

					nRet = _WSASend( _ssl_s->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
					if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
					{
						sent = false;

						g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
						_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;

						g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
						SecInvalidateHandle( &_ssl_s->hContext );

						return SEC_E_INTERNAL_ERROR;
					}

					// Freed in SSL_WSAConnect_Response (assuming we get a response).
					//g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
					//_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;

					return SEC_I_CONTINUE_NEEDED;
				}
			}

			if ( scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE )	// Request more data until we get something.
			{
				if ( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
					_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
				}

				sent = true;

				PostQueuedCompletionStatus( g_hIOCP, 0, ( ULONG_PTR )context, ( WSAOVERLAPPED * )overlapped );

				return scRet;
			}
			else if ( scRet == SEC_E_OK )
			{
				// Store remaining data for further use
				if ( _ssl_s->acd.InBuffers[ 1 ].BufferType == SECBUFFER_EXTRA && _ssl_s->pbIoBuffer != NULL )
				{
					_memmove( _ssl_s->pbIoBuffer, _ssl_s->pbIoBuffer + ( _ssl_s->cbIoBuffer - _ssl_s->acd.InBuffers[ 1 ].cbBuffer ), _ssl_s->acd.InBuffers[ 1 ].cbBuffer );
					_ssl_s->cbIoBuffer = _ssl_s->acd.InBuffers[ 1 ].cbBuffer;
				}
				else
				{
					_ssl_s->cbIoBuffer = 0;

					if ( _ssl_s->pbIoBuffer != NULL )
					{
						GlobalFree( _ssl_s->pbIoBuffer );
						_ssl_s->pbIoBuffer = NULL;
					}

					_ssl_s->sbIoBuffer = 0;
				}

				if ( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer != NULL )
				{
					g_pSSPI->FreeContextBuffer( _ssl_s->acd.OutBuffers[ 0 ].pvBuffer );
					_ssl_s->acd.OutBuffers[ 0 ].pvBuffer = NULL;
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

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		DWORD dwType;

		DWORD dwSSPIFlags;
		DWORD dwSSPIOutFlags;
		TimeStamp tsExpiry;

		SecBufferDesc OutBuffer;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;

		if ( _ssl_s == NULL )
		{
			return SOCKET_ERROR;
		}

		dwType = SCHANNEL_SHUTDOWN;

		_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = &dwType;	// NEVER MAKE THIS NULL. System will break and restart.
		_ssl_s->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		_ssl_s->sdd.OutBuffers[ 0 ].cbBuffer = sizeof( dwType );

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = _ssl_s->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		scRet = g_pSSPI->ApplyControlToken( &_ssl_s->hContext, &OutBuffer );
		if ( FAILED( scRet ) )
		{
			_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;

			return scRet;
		}

		// Build an SSL close notify message.

		_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
		_ssl_s->sdd.OutBuffers[ 0 ].BufferType = SECBUFFER_TOKEN;
		_ssl_s->sdd.OutBuffers[ 0 ].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = _ssl_s->sdd.OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		dwSSPIFlags = ASC_REQ_SEQUENCE_DETECT	|
					  ASC_REQ_REPLAY_DETECT		|
					  ASC_REQ_CONFIDENTIALITY	|
					  ASC_REQ_EXTENDED_ERROR	|
					  ASC_REQ_ALLOCATE_MEMORY	|
					  ASC_REQ_STREAM;

		scRet = g_pSSPI->AcceptSecurityContext(
						( _ssl_s->is_server ? &g_hCreds_server : &g_hCreds_client[ _ssl_s->protocol_index ] ),
						&_ssl_s->hContext,
						NULL,
						dwSSPIFlags,
						SECURITY_NATIVE_DREP,
						NULL,
						&OutBuffer,
						&dwSSPIOutFlags,
						&tsExpiry );

		if ( FAILED( scRet ) )
		{
			if ( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer != NULL )
			{
				g_pSSPI->FreeContextBuffer( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer );
				_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}

			return scRet;
		}

		// Send the close notify message to the server.
		if ( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer != NULL && _ssl_s->sdd.OutBuffers[ 0 ].cbBuffer != 0 )
		{
			sent = true;

			context->wsabuf.buf = ( char * )_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer;
			context->wsabuf.len = _ssl_s->sdd.OutBuffers[ 0 ].cbBuffer;

			overlapped->current_operation = IO_Write;

			nRet = _WSASend( _ssl_s->s, &context->wsabuf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
			if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
			{
				sent = false;
				g_pSSPI->FreeContextBuffer( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer );
				_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;

				scRet = SEC_E_INTERNAL_ERROR;
			}
			/*else	// Freed in SSL_free.
			{
				g_pSSPI->FreeContextBuffer( _ssl_s->sdd.OutBuffers[ 0 ].pvBuffer );
				_ssl_s->sdd.OutBuffers[ 0 ].pvBuffer = NULL;
			}*/
		}

		// Freed in SSL_free.
		// Free the security context.
		//g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
		//SecInvalidateHandle( &_ssl_s->hContext );
	}

	return scRet;
}

SECURITY_STATUS SSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL && g_pSSPI != NULL )
	{
		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;

		SecBuffer Buffers[ 4 ];
		DWORD cbMessage;
		SecBufferDesc Message;

		// _ssl_s->sd.pbDataBuffer is freed when we clean up the connection.
		if ( _ssl_s->sd.pbDataBuffer == NULL )
		{
			scRet = g_pSSPI->QueryContextAttributesA( &_ssl_s->hContext, SECPKG_ATTR_STREAM_SIZES, &_ssl_s->sd.Sizes );
			if ( scRet != SEC_E_OK )
			{
				return scRet;
			}

			// The size includes the SSL header, max message length (16 KB), and SSL trailer.
			_ssl_s->sd.pbDataBuffer = ( PUCHAR )GlobalAlloc( GPTR, ( _ssl_s->sd.Sizes.cbHeader + _ssl_s->sd.Sizes.cbMaximumMessage + _ssl_s->sd.Sizes.cbTrailer ) );
		}

		// Copy our message to the buffer. Truncate the message if it's larger than the maximum allowed size (16 KB).
		cbMessage = min( _ssl_s->sd.Sizes.cbMaximumMessage, ( DWORD )send_buf->len );
		_memcpy_s( _ssl_s->sd.pbDataBuffer + _ssl_s->sd.Sizes.cbHeader, _ssl_s->sd.Sizes.cbMaximumMessage, send_buf->buf, cbMessage );

		send_buf->len -= cbMessage;
		send_buf->buf += cbMessage;

		// Header location. (Beginning of the data buffer).
		Buffers[ 0 ].pvBuffer = _ssl_s->sd.pbDataBuffer;
		Buffers[ 0 ].cbBuffer = _ssl_s->sd.Sizes.cbHeader;
		Buffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;

		// Message location. (After the header).
		Buffers[ 1 ].pvBuffer = _ssl_s->sd.pbDataBuffer + _ssl_s->sd.Sizes.cbHeader;
		Buffers[ 1 ].cbBuffer = cbMessage;
		Buffers[ 1 ].BufferType = SECBUFFER_DATA;

		// Trailer location. (After the message).
		Buffers[ 2 ].pvBuffer = _ssl_s->sd.pbDataBuffer + _ssl_s->sd.Sizes.cbHeader + cbMessage;
		Buffers[ 2 ].cbBuffer = _ssl_s->sd.Sizes.cbTrailer;
		Buffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

		Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = Buffers;

		if ( g_pSSPI->EncryptMessage != NULL )
		{
			scRet = g_pSSPI->EncryptMessage( &_ssl_s->hContext, 0, &Message, 0 );
		}
		else
		{
			scRet = ( ( ENCRYPT_MESSAGE_FN )g_pSSPI->Reserved3 )( &_ssl_s->hContext, 0, &Message, 0 );
		}

		if ( FAILED( scRet ) )
		{
			return scRet;
		}

		sent = true;

		send_buf->buf = ( char * )_ssl_s->sd.pbDataBuffer;
		send_buf->len = Buffers[ 0 ].cbBuffer + Buffers[ 1 ].cbBuffer + Buffers[ 2 ].cbBuffer; // Calculate encrypted packet size

		nRet = _WSASend( _ssl_s->s, send_buf, 1, NULL, dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
		if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
		{
			sent = false;

			g_pSSPI->DeleteSecurityContext( &_ssl_s->hContext );
			SecInvalidateHandle( &_ssl_s->hContext );

			scRet = SEC_E_INTERNAL_ERROR;
		}
	}
	
	return scRet;
}

SECURITY_STATUS SSL_WSARecv( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent )
{
	sent = false;

	if ( context != NULL && context->_ssl_s != NULL && overlapped != NULL )
	{
		WSABUF encrypted_buf;

		int nRet;
		DWORD dwFlags = 0;

		_SSL_S *_ssl_s = context->_ssl_s;

		if ( _ssl_s->cbIoBuffer == 0 || _ssl_s->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			if ( _ssl_s->sbIoBuffer <= _ssl_s->cbIoBuffer )
			{
				_ssl_s->sbIoBuffer += BUFFER_SIZE;

				if ( _ssl_s->pbIoBuffer == NULL )
				{
					_ssl_s->pbIoBuffer = ( PUCHAR )GlobalAlloc( GPTR, _ssl_s->sbIoBuffer );
				}
				else
				{
					_ssl_s->pbIoBuffer = ( PUCHAR )GlobalReAlloc( _ssl_s->pbIoBuffer, _ssl_s->sbIoBuffer, GMEM_MOVEABLE );
				}
			}

			if ( _ssl_s->pbIoBuffer != NULL )
			{
				sent = true;

				encrypted_buf.buf = ( char * )_ssl_s->pbIoBuffer + _ssl_s->cbIoBuffer;
				encrypted_buf.len = _ssl_s->sbIoBuffer - _ssl_s->cbIoBuffer;

				nRet = _WSARecv( _ssl_s->s, &encrypted_buf, 1, NULL, &dwFlags, ( WSAOVERLAPPED * )overlapped, NULL );
				if ( nRet == SOCKET_ERROR && ( _WSAGetLastError() != ERROR_IO_PENDING ) )
				{
					sent = false;
				}
			}

			if ( !sent )
			{
				_ssl_s->rd.scRet = SEC_E_INTERNAL_ERROR;

				return _ssl_s->rd.scRet;
			}
			else
			{
				return SEC_E_OK;
			}
		}

		return _ssl_s->rd.scRet;
	}
	else
	{
		return SEC_E_INTERNAL_ERROR;
	}
}

SECURITY_STATUS SSL_WSARecv_Decrypt( _SSL_S *_ssl_s, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecrypted )
{
	lpNumberOfBytesDecrypted = 0;
	SecBufferDesc Message;
	SecBuffer *pDataBuffer;
	SecBuffer *pExtraBuffer;

	if ( _ssl_s == NULL || g_pSSPI == NULL )
	{
		return -1;
	}

	if ( _ssl_s->rd.scRet == SEC_I_CONTINUE_NEEDED )
	{
		// Handle any remaining data that was already decoded.
		if ( lpBuffers->buf != NULL && lpBuffers->len > 0 && _ssl_s->cbRecDataBuf > 0 && _ssl_s->pbRecDataBuf != NULL )
		{
			lpNumberOfBytesDecrypted = min( ( DWORD )lpBuffers->len, _ssl_s->cbRecDataBuf );
			_memcpy_s( lpBuffers->buf, lpBuffers->len, _ssl_s->pbRecDataBuf, lpNumberOfBytesDecrypted );

			DWORD rbytes = _ssl_s->cbRecDataBuf - lpNumberOfBytesDecrypted;
			if ( rbytes > 0 )
			{
				_memmove( _ssl_s->pbRecDataBuf, ( ( char * )_ssl_s->pbRecDataBuf ) + lpNumberOfBytesDecrypted, rbytes );
			}
			else
			{
				_ssl_s->rd.scRet = SEC_E_OK;
			}
			_ssl_s->cbRecDataBuf = rbytes;
		}

		return _ssl_s->rd.scRet;
	}

	_memzero( _ssl_s->rd.Buffers, sizeof( SecBuffer ) * 4 );

	//_ssl_s->cbIoBuffer = lpBuffers->len;

	// Attempt to decrypt the received data.
	_ssl_s->rd.Buffers[ 0 ].pvBuffer = _ssl_s->pbIoBuffer;
	_ssl_s->rd.Buffers[ 0 ].cbBuffer = _ssl_s->cbIoBuffer;
	_ssl_s->rd.Buffers[ 0 ].BufferType = SECBUFFER_DATA;

	_ssl_s->rd.Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
	_ssl_s->rd.Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
	_ssl_s->rd.Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = _ssl_s->rd.Buffers;

	if ( g_pSSPI->DecryptMessage != NULL )
	{
		_ssl_s->rd.scRet = g_pSSPI->DecryptMessage( &_ssl_s->hContext, &Message, 0, NULL );
	}
	else
	{
		_ssl_s->rd.scRet = ( ( DECRYPT_MESSAGE_FN )g_pSSPI->Reserved4 )( &_ssl_s->hContext, &Message, 0, NULL );
	}

	if ( _ssl_s->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
	{
		// The input buffer contains only a fragment of an encrypted record. Need to read some more data.
		return _ssl_s->rd.scRet;
	}

	// Sender has signaled end of session
	if ( _ssl_s->rd.scRet == SEC_I_CONTEXT_EXPIRED )
	{
		return _ssl_s->rd.scRet;
	}

	if ( _ssl_s->rd.scRet != SEC_E_OK && _ssl_s->rd.scRet != SEC_I_RENEGOTIATE )
	{
		return _ssl_s->rd.scRet;
	}

	// Locate data and (optional) extra buffers.
	pDataBuffer  = NULL;
	pExtraBuffer = NULL;
	for ( int i = 1; i < 4; ++i )
	{
		if ( pDataBuffer == NULL && _ssl_s->rd.Buffers[ i ].BufferType == SECBUFFER_DATA )
		{
			pDataBuffer = &_ssl_s->rd.Buffers[ i ];
		}

		if ( pExtraBuffer == NULL && _ssl_s->rd.Buffers[ i ].BufferType == SECBUFFER_EXTRA )
		{
			pExtraBuffer = &_ssl_s->rd.Buffers[ i ];
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
			if ( _ssl_s->sbRecDataBuf < rbytes ) 
			{
				_ssl_s->sbRecDataBuf = rbytes;

				if ( _ssl_s->pbRecDataBuf == NULL )
				{
					_ssl_s->pbRecDataBuf = ( PUCHAR )GlobalAlloc( GPTR, _ssl_s->sbRecDataBuf );
				}
				else
				{
					_ssl_s->pbRecDataBuf = ( PUCHAR )GlobalReAlloc( _ssl_s->pbRecDataBuf, _ssl_s->sbRecDataBuf, GMEM_MOVEABLE );
				}
			}

			if ( _ssl_s->pbRecDataBuf != NULL )
			{
				_memcpy_s( _ssl_s->pbRecDataBuf, _ssl_s->sbRecDataBuf, ( char * )pDataBuffer->pvBuffer + lpNumberOfBytesDecrypted, rbytes );
				_ssl_s->cbRecDataBuf = rbytes;

				_ssl_s->rd.scRet = SEC_I_CONTINUE_NEEDED;
			}
			else
			{
				_ssl_s->rd.scRet = SEC_E_INTERNAL_ERROR;
			}
		}
	}

	// Move any extra data to the input buffer.
	if ( pExtraBuffer != NULL && _ssl_s->pbIoBuffer != NULL )
	{
		_memmove( _ssl_s->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer );
		_ssl_s->cbIoBuffer = pExtraBuffer->cbBuffer;
	}
	else
	{
		_ssl_s->cbIoBuffer = 0;
	}

	/*if ( pDataBuffer != NULL && lpNumberOfBytesDecrypted != 0 )
	{
		return _ssl_s->rd.scRet;
	}

	if ( _ssl_s->rd.scRet == SEC_I_RENEGOTIATE )
	{
		return _ssl_s->rd.scRet;
	}*/

	return _ssl_s->rd.scRet;
}

SECURITY_STATUS DecryptRecv( SOCKET_CONTEXT *context, DWORD &io_size )
{
	SECURITY_STATUS scRet = SEC_E_INTERNAL_ERROR;

	if ( context != NULL && context->_ssl_s != NULL )
	{
		WSABUF wsa_decrypt;

		DWORD bytes_decrypted = 0;

		if ( context->_ssl_s->rd.scRet == SEC_E_INCOMPLETE_MESSAGE )
		{
			context->_ssl_s->cbIoBuffer += io_size;
		}
		else
		{
			context->_ssl_s->cbIoBuffer = io_size;
		}

		io_size = 0;
		
		context->_ssl_s->continue_decrypt = false;

		wsa_decrypt = context->wsabuf;

		// Decrypt our buffer.
		while ( context->_ssl_s->pbIoBuffer != NULL /*&& context->_ssl_s->cbIoBuffer > 0*/ )
		{
			scRet = SSL_WSARecv_Decrypt( context->_ssl_s, &wsa_decrypt, bytes_decrypted );

			io_size += bytes_decrypted;

			wsa_decrypt.buf += bytes_decrypted;
			wsa_decrypt.len -= bytes_decrypted;

			switch ( scRet )
			{
				// We've successfully decrypted a portion of the buffer.
				case SEC_E_OK:
				{
					// Decrypt more records if there are any.
					continue;
				}
				break;

				// The message was decrypted, but not all of it was copied to our wsabuf.
				// There may be incomplete records left to decrypt. DecryptRecv must be called again after processing wsabuf.
				case SEC_I_CONTINUE_NEEDED:
				{
					context->_ssl_s->continue_decrypt = true;

					return scRet;
				}
				break;

				case SEC_E_INCOMPLETE_MESSAGE:	// The message was incomplete. Request more data from the server.
				{
					return scRet;
				}
				break;

				case SEC_I_RENEGOTIATE:			// Client wants us to perform another handshake.
				{
					bool sent = false;

					context->_ssl_s->acd.scRet = SEC_I_RENEGOTIATE; // InitializeSecurityContext will reset this value inside SSL_WSAConnect_Reply().
					
					// If we end up handling IO_ClientHandshakeResponse and get an SEC_E_OK, then this will allow us to continue our IO_GetContent routine.
					context->_ssl_s->acd.fRenegotiate = true;

					InterlockedIncrement( &context->pending_operations );

					IO_OPERATION last_current_operation = context->overlapped.current_operation;
					IO_OPERATION last_next_operation = context->overlapped.next_operation;

					context->overlapped.current_operation = IO_ClientHandshakeResponse;
					context->overlapped.next_operation = IO_ClientHandshakeResponse;

					scRet = SSL_WSAConnect_Reply( context, &context->overlapped, sent );

					if ( !sent )
					{
						context->_ssl_s->acd.fRenegotiate = false;	// Reset.

						InterlockedDecrement( &context->pending_operations );

						context->overlapped.current_operation = last_current_operation;
						context->overlapped.next_operation = last_next_operation;

						scRet = SEC_E_INCOMPLETE_MESSAGE;	// If we return with this, then a SSL_WSARecv() will be performed.
					}
					else	// This shouldn't happen, but we'll handle it if it does. SSL_WSAConnect_Reply() will have performed a _WSASend().
					{
						return SEC_I_RENEGOTIATE;
					}
				}
				break;

				//case SEC_I_CONTEXT_EXPIRED:
				default:
				{
					context->_ssl_s->cbIoBuffer = 0;

					return scRet;
				}
				break;
			}
		}

		context->_ssl_s->cbIoBuffer = 0;
	}

	return scRet;
}

PCCERT_CONTEXT LoadPublicPrivateKeyPair( wchar_t *cer, wchar_t *key )
{
	char open_count = 0;

	bool failed = false;
	PCCERT_CONTEXT	pCertContext = NULL;
	HCRYPTPROV hProv = NULL;
	HCRYPTKEY hKey = NULL;


	HCERTSTORE hMyCertStore = NULL;
	HCRYPTMSG hCryptMsg = NULL;

	DWORD dwMsgAndCertEncodingType = 0;
	DWORD dwContentType = 0;
	DWORD dwFormatType = 0;

	LPWSTR pwszUuid = NULL;

	LPBYTE pbBuffer = NULL, pbKeyBlob = NULL;
	DWORD dwBufferLen = 0, cbKeyBlob = 0;
	BYTE *szPemPrivKey = NULL;

	#ifndef RPCRT4_USE_STATIC_LIB
		if ( rpcrt4_state == RPCRT4_STATE_SHUTDOWN )
		{
			if ( !InitializeRpcRt4() ){ return NULL; }
		}
	#endif

	//hMyCertStore = _CertOpenStore( CERT_STORE_PROV_FILENAME, 0, NULL, CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, cer );
	//pCertContext = _CertFindCertificateInStore( hMyCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );
	_CryptQueryObject( CERT_QUERY_OBJECT_FILE, cer, CERT_QUERY_CONTENT_FLAG_CERT, CERT_QUERY_FORMAT_FLAG_ALL, 0, &dwMsgAndCertEncodingType, &dwContentType, &dwFormatType, &hMyCertStore, &hCryptMsg, ( const void ** )&pCertContext );

	HANDLE hFile_cfg = INVALID_HANDLE_VALUE;

	OVERLAPPED lfo;

RETRY_OPEN:

	hFile_cfg = CreateFile( key, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_cfg, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		dwBufferLen = GetFileSize( hFile_cfg, NULL );

		szPemPrivKey = ( BYTE * )GlobalAlloc( GMEM_FIXED, sizeof( BYTE ) * dwBufferLen );
		if ( szPemPrivKey != NULL )
		{
			BOOL bRet = ReadFile( hFile_cfg, szPemPrivKey, sizeof( char ) * dwBufferLen, &dwBufferLen, NULL );
			if ( bRet != FALSE )
			{
				// Let's assume the key is also in the same format.
				if ( dwFormatType == CERT_QUERY_FORMAT_BASE64_ENCODED )	// PEM format.
				{
					if ( _CryptStringToBinaryA( ( LPCSTR )szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, NULL, &dwBufferLen, NULL, NULL ) == FALSE )
					{
						failed = true;
						goto CLEANUP;
					}

					pbBuffer = ( LPBYTE )GlobalAlloc( GMEM_FIXED, dwBufferLen );
					if ( _CryptStringToBinaryA( ( LPCSTR )szPemPrivKey, 0, CRYPT_STRING_BASE64HEADER, pbBuffer, &dwBufferLen, NULL, NULL ) == FALSE )
					{
						failed = true;
						goto CLEANUP;
					}
				}
				else	// DER format.
				{
					pbBuffer = szPemPrivKey;
					szPemPrivKey = NULL;
				}

				if ( _CryptDecodeObjectEx( dwMsgAndCertEncodingType, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, NULL, &cbKeyBlob ) == FALSE )
				{
					failed = true;
					goto CLEANUP;
				}

				pbKeyBlob = ( LPBYTE )GlobalAlloc( GMEM_FIXED, cbKeyBlob );
				if ( _CryptDecodeObjectEx( dwMsgAndCertEncodingType, PKCS_RSA_PRIVATE_KEY, pbBuffer, dwBufferLen, 0, NULL, pbKeyBlob, &cbKeyBlob ) == FALSE )
				{
					failed = true;
					goto CLEANUP;
				}

				UUID uuid;
				_UuidCreate( &uuid );
				_UuidToStringW( &uuid, ( RPC_WSTR * )&pwszUuid );

				if ( _CryptAcquireContextW( &hProv, pwszUuid, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET ) == FALSE )
				{
					failed = true;
					goto CLEANUP;
				}

				if ( _CryptImportKey( hProv, pbKeyBlob, cbKeyBlob, NULL, 0, &hKey ) == FALSE )
				{
					failed = true;
					goto CLEANUP;
				}

				CRYPT_KEY_PROV_INFO privateKeyData;
				_memzero( &privateKeyData, sizeof( CRYPT_KEY_PROV_INFO ) );
				privateKeyData.pwszContainerName = pwszUuid;
				privateKeyData.pwszProvName = MS_ENHANCED_PROV;
				privateKeyData.dwProvType = PROV_RSA_FULL;
				privateKeyData.dwFlags = 0;
				privateKeyData.dwKeySpec = AT_KEYEXCHANGE;

				if ( _CertSetCertificateContextProperty( pCertContext, CERT_KEY_PROV_INFO_PROP_ID, 0, &privateKeyData ) == FALSE )
				{
					failed = true;
				}
			}
			else
			{
				failed = true;
			}
		}
		else
		{
			failed = true;
		}
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}
	}

CLEANUP:

	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		UnlockFileEx( hFile_cfg, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_cfg );
	}

	if ( pwszUuid != NULL )
	{
		_RpcStringFreeW( ( RPC_WSTR * )&pwszUuid );
	}

	if ( pbBuffer != NULL )
	{
		GlobalFree( pbBuffer );
	}

	if ( szPemPrivKey != NULL )
	{
		GlobalFree( szPemPrivKey );
	}

	if ( pbKeyBlob != NULL )
	{
		GlobalFree( pbKeyBlob );
	}

	if ( hCryptMsg != NULL )
	{
		_CryptMsgClose( hCryptMsg );
	}

	if ( hKey != NULL )
	{
		_CryptDestroyKey( hKey );
	}
	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	if ( hMyCertStore != NULL )
	{
		_CertCloseStore( hMyCertStore, 0 );
	}

	if ( failed && pCertContext != NULL )
	{
		_CertFreeCertificateContext( pCertContext );
		pCertContext = NULL;
	}

	return pCertContext;
}

PCCERT_CONTEXT LoadPKCS12( wchar_t *p12_file, wchar_t *password )
{
	char open_count = 0;

	PCCERT_CONTEXT	pCertContext = NULL;

	HANDLE hFile_cfg = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_cfg = CreateFile( p12_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_cfg != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_cfg, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		HCERTSTORE hMyCertStore = NULL;

		CRYPT_DATA_BLOB cdb;
		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_cfg, NULL );

		cdb.cbData = fz;
		cdb.pbData = ( BYTE * )GlobalAlloc( GMEM_FIXED, sizeof( BYTE ) * fz );
		if ( cdb.pbData != NULL )
		{
			BOOL bRet = ReadFile( hFile_cfg, cdb.pbData, sizeof( char ) * fz, &read, NULL );
			if ( bRet != FALSE )
			{
				hMyCertStore = _PFXImportCertStore( &cdb, password, 0 );
				if ( hMyCertStore != NULL )
				{
					pCertContext = _CertFindCertificateInStore( hMyCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL );

					_CertCloseStore( hMyCertStore, 0 );
				}
			}

			GlobalFree( cdb.pbData );
		}

		UnlockFileEx( hFile_cfg, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_cfg );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}
	}

	return pCertContext;
}
