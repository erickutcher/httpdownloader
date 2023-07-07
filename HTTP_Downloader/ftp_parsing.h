/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2023 Eric Kutcher

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

#ifndef _FTP_PARSING_H
#define _FTP_PARSING_H

#include "connection.h"

#define FTP_CONTENT_STATUS_FAILED				CONTENT_STATUS_FAILED
#define FTP_CONTENT_STATUS_NONE					CONTENT_STATUS_NONE

#define FTP_CONTENT_STATUS_READ_MORE_CONTENT	CONTENT_STATUS_READ_MORE_CONTENT
#define FTP_CONTENT_STATUS_GET_CONTENT			CONTENT_STATUS_GET_CONTENT
#define FTP_CONTENT_STATUS_FILE_SIZE_PROMPT		CONTENT_STATUS_FILE_SIZE_PROMPT
#define FTP_CONTENT_STATUS_LAST_MODIFIED_PROMPT	CONTENT_STATUS_LAST_MODIFIED_PROMPT
#define FTP_CONTENT_STATUS_ALLOCATE_FILE		CONTENT_STATUS_ALLOCATE_FILE

#define FTP_CONTENT_STATUS_SET_AUTH				21	// Set the authentication type (SSL, TLS, etc.) for an explicit FTPS connection.
#define FTP_CONTENT_STATUS_SET_PBSZ				22	// Set the protection buffer size.
#define FTP_CONTENT_STATUS_SET_PROT				23	// Set the data channel protection type. (Used with SSL/TLS connections).

#define FTP_CONTENT_STATUS_SEND_USER			24	// Send username.
#define FTP_CONTENT_STATUS_SEND_PASS			25	// Send password.
#define FTP_CONTENT_STATUS_SET_OPTS				26	// Set options.
#define FTP_CONTENT_STATUS_SET_TYPE				27	// Set transfer type (ASCII/binary).
#define FTP_CONTENT_STATUS_GET_SIZE				28	// Get the size of a file.
#define FTP_CONTENT_STATUS_GET_MDTM				29	// Get the last modified time of a file.
#define FTP_CONTENT_STATUS_SET_MODE				30	// Set transfer mode (passive: PASV/EPSV), (active: PORT/EPRT). EPSV/EPRT supports IPV6.
#define FTP_CONTENT_STATUS_SEND_RETR			31	// Retrieve file (start transfer on data connection).
#define FTP_CONTENT_STATUS_SEND_REST			32	// Resume transfer at byte offset (resume transfer on data connection).
#define FTP_CONTENT_STATUS_SEND_QUIT			33	// Log out and disconnect from the server.

#define FTP_CONTENT_STATUS_HANDLE_REQUEST		34

#define FTP_CONTENT_STATUS_SEND_KEEP_ALIVE		35	// Sends random command (NOOP, PWD, TYPE I/A).

//

#define FTP_CONNECTION_TYPE_CONTROL				0x01
#define FTP_CONNECTION_TYPE_DATA				0x02
#define FTP_CONNECTION_TYPE_LISTEN				0x04
#define FTP_CONNECTION_TYPE_CONTROL_SUCCESS		0x08	// Data context completed successfully.
#define FTP_CONNECTION_TYPE_CONTROL_WAIT		0x10	// Transfer succeeded, but we need to wait for the Data context to cleanup.

#define FTP_MODE_PASSIVE					0x01
#define FTP_MODE_ACTIVE						0x02
#define FTP_MODE_EXTENDED					0x04

SOCKET CreateFTPListenSocket( SOCKET_CONTEXT *context );
char CreateFTPAcceptSocket( SOCKET_CONTEXT *context );

char GetFTPResponseContent( SOCKET_CONTEXT *context, char *response_buffer, unsigned int response_buffer_length );

void SetDataContextValues( SOCKET_CONTEXT *context, SOCKET_CONTEXT *new_context );
char MakeFTPResponse( SOCKET_CONTEXT *context );

char SendFTPKeepAlive( SOCKET_CONTEXT *context );

extern CRITICAL_SECTION ftp_listen_info_cs;

#endif
