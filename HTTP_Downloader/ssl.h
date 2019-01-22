/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2019 Eric Kutcher

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

#ifndef _SCHANNEL_SSL_H
#define _SCHANNEL_SSL_H

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>

#include "lite_ws2_32.h"

#define SSL_STATE_SHUTDOWN		0
#define SSL_STATE_RUNNING		1

#define SP_PROT_TLS1_1_SERVER		0x00000100
#define SP_PROT_TLS1_1_CLIENT		0x00000200
#define SP_PROT_TLS1_1				( SP_PROT_TLS1_1_SERVER | SP_PROT_TLS1_1_CLIENT )
#define SP_PROT_TLS1_2_SERVER		0x00000400
#define SP_PROT_TLS1_2_CLIENT		0x00000800
#define SP_PROT_TLS1_2				( SP_PROT_TLS1_2_SERVER | SP_PROT_TLS1_2_CLIENT )

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

struct SSL
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

	bool is_server;
	bool continue_decrypt;
};

int SSL_library_init( void );
int SSL_library_uninit( void );

SSL *SSL_new( DWORD protocol, bool is_server );
void SSL_free( SSL *ssl );

void ResetServerCredentials();
void ResetClientCredentials();

PCCERT_CONTEXT LoadPublicPrivateKeyPair( wchar_t *cer, wchar_t *key );
PCCERT_CONTEXT LoadPKCS12( wchar_t *p12_file, wchar_t *password );

extern PCCERT_CONTEXT g_pCertContext;

extern unsigned char ssl_state;

#endif
