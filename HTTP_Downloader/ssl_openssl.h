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

#ifndef _SSL_OPENSSL_H
#define _SSL_OPENSSL_H

#include "connection.h"

#include "lite_libssl.h"
#include "lite_libcrypto.h"

struct _SSL_O
{
	SSL					*ssl;
	BIO					*rbio;
	BIO					*wbio;
	SSL_SESSION			*ssl_session;
	bool				continue_decrypt;
};

void InitializeSSL_CTXs();
void FreeSSL_CTXs();

void InitializeServerSSL_CTX( unsigned char ssl_version, unsigned char certificate_type );

int WINAPIV new_session_cb( SSL *ssl, SSL_SESSION *sess );

void OpenSSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent );

char OpenSSL_DecryptRecv( SOCKET_CONTEXT *context, DWORD &io_size );

void OpenSSL_FreeInfo( _SSL_O **_ssl_o );

extern SSL_CTX *g_client_ssl_ctx[ PROTOCOL_COUNT ];
extern SSL_CTX *g_server_ssl_ctx;

#endif
