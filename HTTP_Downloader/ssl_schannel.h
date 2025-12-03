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

#ifndef _SSL_SCHANNEL_H
#define _SSL_SCHANNEL_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SECURITY_WIN32
#include <security.h>

#define SCHANNEL_USE_BLACKLISTS

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#include <schannel.h>

#define SCH_CREDENTIALS_VERSION  0x00000005

typedef enum _eTlsAlgorithmUsage
{
	TlsParametersCngAlgUsageKeyExchange,
	TlsParametersCngAlgUsageSignature,
	TlsParametersCngAlgUsageCipher,
	TlsParametersCngAlgUsageDigest,
	TlsParametersCngAlgUsageCertSig
} eTlsAlgorithmUsage;

typedef struct _CRYPTO_SETTINGS
{
	eTlsAlgorithmUsage	eAlgorithmUsage;
	UNICODE_STRING		strCngAlgId;
	DWORD				cChainingModes;
	PUNICODE_STRING		rgstrChainingModes;
	DWORD				dwMinBitLength;
	DWORD				dwMaxBitLength;
} CRYPTO_SETTINGS, *PCRYPTO_SETTINGS;

typedef struct _TLS_PARAMETERS
{
	DWORD				cAlpnIds;
	PUNICODE_STRING		rgstrAlpnIds;
	DWORD				grbitDisabledProtocols;
	DWORD				cDisabledCrypto;
	PCRYPTO_SETTINGS	pDisabledCrypto;
	DWORD				dwFlags;
} TLS_PARAMETERS, *PTLS_PARAMETERS;

typedef struct _SCH_CREDENTIALS
{
	DWORD				dwVersion;
	DWORD				dwCredFormat;
	DWORD				cCreds;
	PCCERT_CONTEXT		*paCred;
	HCERTSTORE			hRootStore;

	DWORD				cMappers;
	struct _HMAPPER		**aphMappers;

	DWORD				dwSessionLifespan;
	DWORD				dwFlags;
	DWORD				cTlsParameters;
	PTLS_PARAMETERS		pTlsParameters;
} SCH_CREDENTIALS, *PSCH_CREDENTIALS;

#include "lite_ws2_32.h"

#include "connection.h"

#define SSL_STATE_SHUTDOWN		0
#define SSL_STATE_RUNNING		1

#define SP_PROT_TLS1_1_SERVER		0x00000100
#define SP_PROT_TLS1_1_CLIENT		0x00000200
#define SP_PROT_TLS1_1				( SP_PROT_TLS1_1_SERVER | SP_PROT_TLS1_1_CLIENT )
#define SP_PROT_TLS1_2_SERVER		0x00000400
#define SP_PROT_TLS1_2_CLIENT		0x00000800
#define SP_PROT_TLS1_2				( SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_2_CLIENT )
#define SP_PROT_TLS1_3_SERVER		0x00001000
#define SP_PROT_TLS1_3_CLIENT		0x00002000
#define SP_PROT_TLS1_3				( SP_PROT_TLS1_3_SERVER | SP_PROT_TLS1_3_CLIENT )

/*struct ACCEPT_DATA
{
	SecBuffer		InBuffers[ 2 ];
	SecBuffer		OutBuffers[ 1 ];

	SECURITY_STATUS scRet;

	bool			fInitContext;
	bool			fDoRead;
};

struct CONNECT_DATA
{
	SecBuffer		InBuffers[ 2 ];
	SecBuffer		OutBuffers[ 1 ];

	SECURITY_STATUS	scRet;

	bool			fDoRead;
};*/

struct ACCEPT_CONNECT_DATA
{
	SecBuffer		InBuffers[ 2 ];
	SecBuffer		OutBuffers[ 1 ];

	SECURITY_STATUS	scRet;

	bool			fInitContext;
	bool			fDoRead;
	bool			fRenegotiate;
};

struct RECV_DATA
{
	SecBuffer		Buffers[ 4 ];

	SECURITY_STATUS	scRet;
};

struct SEND_DATA
{
	SecPkgContext_StreamSizes Sizes;

	PUCHAR			pbDataBuffer;
};

struct SHUTDOWN_DATA
{
	SecBuffer		OutBuffers[ 1 ];
};

struct _SSL_S
{
	SEND_DATA				sd;
	ACCEPT_CONNECT_DATA		acd;
	RECV_DATA				rd;
	SHUTDOWN_DATA			sdd;

	CtxtHandle hContext;

	BYTE *pbRecDataBuf;
	BYTE *pbIoBuffer;

	SOCKET s;

	//DWORD dwProtocol;

	DWORD cbRecDataBuf;
	DWORD sbRecDataBuf;

	DWORD cbIoBuffer;
	DWORD sbIoBuffer;

	unsigned char protocol_index;

	bool is_server;
	bool continue_decrypt;
};

int __SSL_library_init( void );
int __SSL_library_uninit( void );

_SSL_S *__SSL_new( DWORD protocol, bool is_server );
void __SSL_free( _SSL_S *_ssl_s );

void ResetServerCredentials();
void ResetClientCredentials( unsigned char index );

SECURITY_STATUS SSL_WSAAccept( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSAConnect( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, char *host, bool &sent );
SECURITY_STATUS SSL_WSAConnect_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAConnect_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent );
SECURITY_STATUS SSL_WSARecv( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSARecv_Decrypt( _SSL_S *_ssl_s, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecrypted );
SECURITY_STATUS DecryptRecv( SOCKET_CONTEXT *context, DWORD &io_size );

PCCERT_CONTEXT LoadPublicPrivateKeyPair( wchar_t *cer, wchar_t *key );
PCCERT_CONTEXT LoadPKCS12( wchar_t *p12_file, wchar_t *password );

extern PCCERT_CONTEXT g_pCertContext;

extern unsigned char ssl_state;

#endif
