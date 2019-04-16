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

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "globals.h"
#include "ssl.h"
#include "doublylinkedlist.h"
#include "dllrbt.h"
#include "zlib.h"

#include <mswsock.h>

#define BUFFER_SIZE				16384	// Maximum size of an SSL record.

#define MAX_FILE_SIZE			4294967296	// 4GB

#define STATUS_NONE						0x00000000
#define STATUS_CONNECTING				0x00000001
#define STATUS_DOWNLOADING				0x00000002
#define STATUS_PAUSED					0x00000004
#define STATUS_QUEUED					0x00000008
#define STATUS_COMPLETED				0x00000010
#define STATUS_STOPPED					0x00000020
#define STATUS_TIMED_OUT				0x00000040
#define STATUS_FAILED					0x00000080
#define STATUS_RESTART					0x00000100
#define STATUS_REMOVE					0x00000200
#define STATUS_DELETE					0x00000400
#define STATUS_FILE_IO_ERROR			0x00000800
#define STATUS_SKIPPED					0x00001000
#define STATUS_AUTH_REQUIRED			0x00002000
#define STATUS_PROXY_AUTH_REQUIRED		0x00004000
#define STATUS_UPDATING					0x00008000
#define STATUS_ALLOCATING_FILE			0x00010000
#define STATUS_MOVING_FILE				0x00020000

#define IS_STATUS( a, b )			( ( a ) & ( b ) )
#define IS_STATUS_NOT( a, b )		!( ( a ) & ( b ) )

#define CONNECTION_NONE			0
#define CONNECTION_KEEP_ALIVE	1
#define CONNECTION_CLOSE		2

#define METHOD_NONE			0
#define METHOD_GET			1
#define METHOD_POST			2
#define METHOD_CONNECT		3
#define METHOD_HEAD			4
#define METHOD_PUT			5
#define METHOD_DELETE		6
#define METHOD_OPTIONS		7
#define METHOD_TRACE		8
#define METHOD_PATCH		9
#define METHOD_UNHANDLED	10

#define CONTENT_ENCODING_NONE		0
#define CONTENT_ENCODING_GZIP		1
#define CONTENT_ENCODING_DEFLATE	2
#define CONTENT_ENCODING_UNHANDLED	3

#define AUTH_TYPE_NONE			0
#define AUTH_TYPE_BASIC			1
#define AUTH_TYPE_DIGEST		2
#define AUTH_TYPE_UNHANDLED		3

#define CONTENT_STATUS_FAILED			   -1
#define CONTENT_STATUS_NONE					0
#define CONTENT_STATUS_READ_MORE_HEADER		1
#define CONTENT_STATUS_READ_MORE_CONTENT	2
#define CONTENT_STATUS_GET_HEADER			3
#define CONTENT_STATUS_GET_CONTENT			4
#define CONTENT_STATUS_RENAME_FILE_PROMPT	5
#define CONTENT_STATUS_FILE_SIZE_PROMPT		6
#define CONTENT_STATUS_LAST_MODIFIED_PROMPT	7
#define CONTENT_STATUS_ALLOCATE_FILE		8
#define CONTENT_STATUS_HANDLE_RESPONSE		9	// Deals with HTTP status 206 and 401 responses.
#define CONTENT_STATUS_HANDLE_REQUEST		10

#define SOCKS_STATUS_FAILED				   -1
#define SOCKS_STATUS_NONE					0
#define SOCKS_STATUS_REQUEST_AUTH			1
#define SOCKS_STATUS_AUTH_SENT				2
#define SOCKS_STATUS_REQUEST_CONNECTION		3
#define SOCKS_STATUS_HANDLE_CONNECTION		4

#define SOCKS_TYPE_V4		0
#define SOCKS_TYPE_V5		1

#define TIME_OUT_FALSE		0
#define TIME_OUT_TRUE		1
#define TIME_OUT_RETRY		2

// For listen and accept functions
#define LA_STATUS_FAILED			   -1
#define LA_STATUS_UNKNOWN				0
#define LA_STATUS_OK					1

#define DOWNLOAD_OPERATION_NONE				0
#define DOWNLOAD_OPERATION_SIMULATE			1
#define DOWNLOAD_OPERATION_OVERRIDE_PROMPTS	2

enum PROTOCOL
{
	PROTOCOL_UNKNOWN,
	PROTOCOL_HTTP,
	PROTOCOL_HTTPS,
	PROTOCOL_RELATIVE
};

enum IO_OPERATION
{
	IO_Accept,
	IO_Connect,
	IO_ClientHandshakeReply,
	IO_ClientHandshakeResponse,
	IO_ServerHandshakeResponse,
	IO_ServerHandshakeReply,
	IO_GetCONNECTResponse,
	IO_SOCKSResponse,
	IO_GetRequest,
	IO_GetContent,
	IO_ResumeGetContent,
	IO_WriteFile,
	IO_Write,
	IO_Shutdown,
	IO_Close
};

struct URL_LOCATION
{
	char				*host;
	char				*resource;
	PROTOCOL			protocol;
	unsigned short		port;
	unsigned char		redirect_count;
};

struct RANGE_INFO
{
	unsigned long long	range_start;
	unsigned long long	range_end;
	unsigned long long	content_length;
	unsigned long long	content_offset;

	unsigned long long	file_write_offset;	// The offset of our written data. It may be larger than our content offset because of compression.
};

struct AUTH_INFO
{
	char				*realm;
	char				*nonce;
	char				*cnonce;
	char				*domain;
	char				*opaque;
	char				*qop;
	char				*uri;
	char				*username;
	char				*response;
	unsigned int		nc;
	char				qop_type;		// 0 = not found, 1 = auth, 2 = auth-int, 3 = unhandled
	char				algorithm;		// 0 = not found, 1 = MD5, 2 = MD5-sess, 3 = unhandled
	unsigned char		auth_type;		// 0 = none/not found, 1 = basic, 2 = digest, 3 = unhandled
};

struct HEADER_INFO
{
	URL_LOCATION		url_location;
	FILETIME			last_modified;
	//unsigned long long	content_length;
	unsigned long long	chunk_length;
	RANGE_INFO			*range_info;
	dllrbt_tree			*cookie_tree;
	char				*cookies;
	char				*end_of_header;
	char				*chunk_buffer;
	AUTH_INFO			*digest_info;
	AUTH_INFO			*proxy_digest_info;
	unsigned short		http_status;
	unsigned char		http_method;
	unsigned char		connection;			// 0 = none/not found, 1 = keep-alive, 2 = close
	unsigned char		content_encoding;	// 0 = none/not found, 1 = gzip, 2 = deflate, 3 = unhandled
	bool				chunked_transfer;
	//bool				etag;
	bool				got_chunk_start;
	bool				got_chunk_terminator;
};

struct AUTH_CREDENTIALS
{
	char				*username;
	char				*password;
};

struct POST_INFO
{
	char				*method;	// 1 = GET, 2 = POST
	char				*urls;
	char				*username;
	char				*password;
	char				*parts;
	char				*directory;
	char				*simulate_download;
	char				*cookies;
	char				*headers;
	char				*data;		// For POST payloads.
};

struct SOCKET_CONTEXT;

struct OVERLAPPEDEX
{
	WSAOVERLAPPED		overlapped;
	SOCKET_CONTEXT		*context;
	IO_OPERATION		current_operation;
	IO_OPERATION		next_operation;
};

struct DOWNLOAD_INFO;

struct SOCKET_CONTEXT
{
	char				buffer[ BUFFER_SIZE + 1 ];

	HEADER_INFO			header_info;

	z_stream			stream;

	CRITICAL_SECTION	context_cs;

	URL_LOCATION		request_info;

	OVERLAPPEDEX		overlapped;
	OVERLAPPEDEX		overlapped_close;

	DoublyLinkedList	context_node;	// Self reference to the g_context_list.
	DoublyLinkedList	parts_node;		// Self reference to the parts_list of this context's download_info.

	WSABUF				wsabuf;
	WSABUF				write_wsabuf;

	unsigned long long	content_offset;

	DoublyLinkedList	*range_node;	// Self reference to the range_list of this context's download_info.

	addrinfoW			*address_info;			// Address info of the server we're connecting to.
	addrinfoW			*proxy_address_info;	// Address info of the server that we want to proxy.

	char				*decompressed_buf;

	DOWNLOAD_INFO		*download_info;

	POST_INFO			*post_info;

	SSL					*ssl;
    SOCKET				socket;

	DWORD				current_bytes_read;

	unsigned int		decompressed_buf_size;

	unsigned int		status;

	volatile LONG		pending_operations;
	volatile LONG		timeout;

	char				content_status;

	unsigned char		part;
	unsigned char		parts;

	unsigned char		retries;			// The number of times a context connection has been retried.

	unsigned char		timed_out;

	unsigned char		cleanup;			// In cleanup function, or in worker thread doing/calling cleanup.

	unsigned char		got_filename;		// For Content-Disposition header fields. 0 = none/not found, 1 = renamed (doesn't exist), 2 = renamed (exists)
	unsigned char		got_last_modified;	// For Last-Modified header fields. 0 = none/not found, 1 = found

	bool				show_file_size_prompt;

	bool				is_allocated;

	bool				processed_header;

	bool				is_paused;			// The last IO has completed while status is in the paused state.
};

struct ADD_INFO
{
	AUTH_CREDENTIALS	auth_info;
	wchar_t				*download_directory;
	wchar_t				*urls;
	char				*utf8_cookies;
	char				*utf8_headers;
	char				*utf8_data;	// POST payload.
	unsigned char		parts;
	unsigned char		download_operations;
	unsigned char		method;		// 1 = GET, 2 = POST
	char				ssl_version;
};

struct RENAME_INFO
{
	DOWNLOAD_INFO		*di;
	wchar_t				*filename;
	unsigned short		filename_length;
};

struct DOWNLOAD_INFO
{
	wchar_t				file_path[ MAX_PATH ];
	CRITICAL_SECTION	shared_cs;
	DoublyLinkedList	download_node;		// Self reference to the active download_list.
	DoublyLinkedList	queue_node;			// Self reference to the download_queue.
	ULARGE_INTEGER		add_time;
	ULARGE_INTEGER		start_time;
	ULARGE_INTEGER		last_modified;
	unsigned long long	last_downloaded;
	unsigned long long	downloaded;
	unsigned long long	file_size;
	unsigned long long	speed;
	unsigned long long	time_remaining;
	unsigned long long	time_elapsed;
	AUTH_CREDENTIALS	auth_info;
	wchar_t				*url;
	wchar_t				*w_add_time;
	DoublyLinkedList	*range_list;		// Active ranges that make up each download part.
	DoublyLinkedList	*range_queue;		// Inactive ranges that make up each download part.
	DoublyLinkedList	*parts_list;		// The contexts that make up each download part.
	HICON				*icon;
	char				*cookies;
	char				*headers;
	char				*data;				// POST payload.
	//char				*etag;
	HANDLE				hFile;
	unsigned int		filename_offset;
	unsigned int		file_extension_offset;
	unsigned int		status;
	unsigned char		parts;
	unsigned char		active_parts;
	unsigned char		parts_limit;		// This is set if we reduce an active download's parts number.
	unsigned char		retries;			// The number of times a download has been retried.
	unsigned char		download_operations;
	unsigned char		method;				// 1 = GET, 2 = POST
	unsigned char		moving_state;		// 0 = None, 1 = Moving, 2 = Cancelling
	char				ssl_version;
	bool				processed_header;
};

SECURITY_STATUS SSL_WSAAccept( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAAccept_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSAConnect( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, char *host, bool &sent );
SECURITY_STATUS SSL_WSAConnect_Response( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );
SECURITY_STATUS SSL_WSAConnect_Reply( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSASend( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, WSABUF *send_buf, bool &sent );
SECURITY_STATUS SSL_WSARecv( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

SECURITY_STATUS SSL_WSARecv_Decrypt( SSL *ssl, LPWSABUF lpBuffers, DWORD &lpNumberOfBytesDecrypted );

SECURITY_STATUS SSL_WSAShutdown( SOCKET_CONTEXT *context, OVERLAPPEDEX *overlapped, bool &sent );

DWORD WINAPI IOCPDownloader( LPVOID pArgs );
DWORD WINAPI IOCPConnection( LPVOID WorkThreadContext );

SOCKET CreateListenSocket();
char CreateAcceptSocket( SOCKET listen_socket, bool use_ipv6 );
SOCKET_CONTEXT *UpdateCompletionPort( SOCKET socket, bool is_listen_socket );

SOCKET_CONTEXT *CreateSocketContext();
bool CreateConnection( SOCKET_CONTEXT *context, char *host, unsigned short port );
bool LoadConnectEx();
void CleanupConnection( SOCKET_CONTEXT *context );

SOCKET CreateSocket( bool IPv6 = false );

void FreeContexts();
void FreeListenContext();

void EnableTimers( bool timer_state );

DWORD WINAPI AddURL( void *add_info );
void StartDownload( DOWNLOAD_INFO *di, bool check_if_file_exits );

dllrbt_tree *CreateFilenameTree();
void DestroyFilenameTree( dllrbt_tree *filename_tree );
bool RenameFile( DOWNLOAD_INFO *di, dllrbt_tree *filename_tree, wchar_t *file_path, unsigned int filename_offset, unsigned int file_extension_offset );

THREAD_RETURN RenameFilePrompt( void *pArguments );
THREAD_RETURN FileSizePrompt( void *pArguments );
THREAD_RETURN LastModifiedPrompt( void *pArguments );

ICON_INFO *CacheIcon( DOWNLOAD_INFO *di, SHFILEINFO *sfi );

void FreePOSTInfo( POST_INFO **post_info );

void FreeAuthInfo( AUTH_INFO **auth_info );

void InitializeServerInfo();
void CleanupServerInfo();
void StartServer();
void CleanupServer();

extern HANDLE g_hIOCP;

extern bool g_end_program;

extern WSAEVENT g_cleanup_event[ 1 ];

extern CRITICAL_SECTION context_list_cs;				// Guard access to the global context list.
extern CRITICAL_SECTION active_download_list_cs;		// Guard access to the global active download list.
extern CRITICAL_SECTION download_queue_cs;				// Guard access to the download queue.
extern CRITICAL_SECTION file_size_prompt_list_cs;		// Guard access to the file size prompt list.
extern CRITICAL_SECTION rename_file_prompt_list_cs;		// Guard access to the rename file prompt list.
extern CRITICAL_SECTION last_modified_prompt_list_cs;	// Guard access to the last modified prompt list.
extern CRITICAL_SECTION move_file_queue_cs;				// Guard access to the move file queue.
extern CRITICAL_SECTION cleanup_cs;

extern DoublyLinkedList *g_context_list;

extern unsigned long total_downloading;
extern DoublyLinkedList *download_queue;

extern DoublyLinkedList *active_download_list;

extern DoublyLinkedList *file_size_prompt_list;		// List of downloads that need to be prompted to continue.
extern DoublyLinkedList *rename_file_prompt_list;	// List of downloads that need to be prompted to continue.
extern DoublyLinkedList *last_modified_prompt_list;	// List of downloads that need to be prompted to continue.

extern DoublyLinkedList *move_file_queue;			// List of downloads that need to be moved to a new folder.

extern bool file_size_prompt_active;
extern int g_file_size_cmb_ret;		// Message box prompt for large files sizes.

extern bool rename_file_prompt_active;
extern int g_rename_file_cmb_ret;	// Message box prompt to rename files.
extern int g_rename_file_cmb_ret2;	// Message box prompt to rename files.

extern bool last_modified_prompt_active;
extern int g_last_modified_cmb_ret;	// Message box prompt for modified files.

extern DOWNLOAD_INFO *g_update_download_info;		// The current item that we want to update.

// Server

extern wchar_t *g_server_punycode_hostname;

extern char *g_authentication_username;
extern char *g_authentication_password;
extern unsigned int g_authentication_username_length;
extern unsigned int g_authentication_password_length;

extern char *g_encoded_authentication;
extern DWORD g_encoded_authentication_length;

extern char *g_nonce;
extern unsigned long g_nonce_length;
extern char *g_opaque;
extern unsigned long g_opaque_length;

#endif
