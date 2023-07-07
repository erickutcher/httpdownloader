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

#ifndef _SFTP_PARSING_H
#define _SFTP_PARSING_H

#include "connection.h"

#define SFTP_CONTENT_STATUS_FAILED					CONTENT_STATUS_FAILED
#define SFTP_CONTENT_STATUS_NONE					CONTENT_STATUS_NONE

#define SFTP_CONTENT_STATUS_GET_CONTENT				CONTENT_STATUS_GET_CONTENT
#define SFTP_CONTENT_STATUS_FILE_SIZE_PROMPT		CONTENT_STATUS_FILE_SIZE_PROMPT
#define SFTP_CONTENT_STATUS_LAST_MODIFIED_PROMPT	CONTENT_STATUS_LAST_MODIFIED_PROMPT
#define SFTP_CONTENT_STATUS_ALLOCATE_FILE			CONTENT_STATUS_ALLOCATE_FILE
#define SFTP_CONTENT_STATUS_HANDLE_REQUEST			CONTENT_STATUS_HANDLE_REQUEST

#define SFTP_CONTENT_STATUS_READ_MORE_CONTENT		20
#define SFTP_CONTENT_STATUS_READ_MORE_CONTENT_1		( SFTP_CONTENT_STATUS_READ_MORE_CONTENT + 1 )
#define SFTP_CONTENT_STATUS_READ_MORE_CONTENT_2		( SFTP_CONTENT_STATUS_READ_MORE_CONTENT + 2 )

#define SFTP_CONTENT_STATUS_WRITE_CONTENT			30
#define SFTP_CONTENT_STATUS_WRITE_CONTENT_1			( SFTP_CONTENT_STATUS_WRITE_CONTENT + 1 )
#define SFTP_CONTENT_STATUS_WRITE_CONTENT_2			( SFTP_CONTENT_STATUS_WRITE_CONTENT + 2 )

#define SFTP_CONTENT_STATUS_KEY_NOT_FOUND			40
#define SFTP_CONTENT_STATUS_KEY_NOT_FOUND_1			( SFTP_CONTENT_STATUS_KEY_NOT_FOUND + 1 )
#define SFTP_CONTENT_STATUS_KEY_NOT_FOUND_2			( SFTP_CONTENT_STATUS_KEY_NOT_FOUND + 2 )

#define SFTP_CONTENT_STATUS_KEY_MISMATCH			50
#define SFTP_CONTENT_STATUS_KEY_MISMATCH_1			( SFTP_CONTENT_STATUS_KEY_MISMATCH + 1 )
#define SFTP_CONTENT_STATUS_KEY_MISMATCH_2			( SFTP_CONTENT_STATUS_KEY_MISMATCH + 2 )

struct SFTP_FPS_HOST_INFO
{
	wchar_t				*w_host;
	wchar_t				*w_key_algorithm;
	wchar_t				*w_key_fingerprint;

	char				*host;
	char				*key_algorithm;
	char				*key_fingerprint;

	unsigned short		port;

	bool				temporary;
};

struct SFTP_FPS_HOST_UPDATE_INFO
{
	SFTP_FPS_HOST_INFO *sfhi;
	SFTP_FPS_HOST_INFO *old_sfhi;
	int index;
	unsigned char update_type;	// 0 = Add, 1 = Remove
};

struct SFTP_KEYS_HOST_INFO
{
	wchar_t				*w_username;
	wchar_t				*w_host;
	wchar_t				*w_key_file_path;

	char				*username;
	char				*host;
	char				*key_file_path;

	unsigned short		port;

	bool				enable;	// Enable/Disable the key info.
};

struct SFTP_KEYS_HOST_UPDATE_INFO
{
	SFTP_KEYS_HOST_INFO *skhi;
	SFTP_KEYS_HOST_INFO *old_skhi;
	int index;
	unsigned char update_type;	// 0 = Add, 1 = Update, 2 = Remove
	bool enable;
};

struct KEY_PROMPT_INFO
{
	SOCKET_CONTEXT *context;

	wchar_t				*w_host;
	wchar_t				*w_key_algorithm;
	wchar_t				*w_key_size;
	wchar_t				*w_key_fingerprint;
	wchar_t				*w_key_fingerprints;	// For display in prompt.

	char				*host;
	char				*key_algorithm;
	char				*key_fingerprint;

	unsigned short		port;
	char type;	// 0 = Not found, 1 = Mismatch
};

char SFTP_ProcessFileInfo( SOCKET_CONTEXT *context );
char SFTP_WriteContent( SOCKET_CONTEXT *context );
char SFTP_ProcessRequest( SOCKET_CONTEXT *context, int *packet_type );

char SFTP_HandleContent( SOCKET_CONTEXT *context, IO_OPERATION current_operation, DWORD io_size );

char SFTP_MakeRangeRequest( SOCKET_CONTEXT *context );
char SFTP_HandleRequest( SOCKET_CONTEXT *context );

void ParseSFTPHost( wchar_t *w_host, char **host, unsigned short &port );

//

char read_sftp_fps_host_info();
char save_sftp_fps_host_info();

THREAD_RETURN load_sftp_fps_host_list( void *pArguments );
THREAD_RETURN handle_sftp_fps_host_list( void *pArguments );
void FreeSFTPFpsHostInfo( SFTP_FPS_HOST_INFO **sftp_fps_host_info );
int dllrbt_compare_sftp_fps_host_info( void *a, void *b );

char *CreateKeyInfoString( DoublyLinkedList *dll_node );

//

char read_sftp_keys_host_info();
char save_sftp_keys_host_info();

THREAD_RETURN load_sftp_keys_host_list( void *pArguments );
THREAD_RETURN handle_sftp_keys_host_list( void *pArguments );
void FreeSFTPKeysHostInfo( SFTP_KEYS_HOST_INFO **sftp_keys_host_info );
int dllrbt_compare_sftp_keys_host_info( void *a, void *b );

//

extern bool sftp_fps_host_list_changed;
extern bool skip_sftp_fps_host_list_draw;

extern bool sftp_keys_host_list_changed;
extern bool skip_sftp_keys_host_list_draw;

#endif
