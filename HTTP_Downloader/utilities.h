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

#ifndef _UTILITIES_H
#define _UTILITIES_H

//#define	ENABLE_LOGGING

#include "connection.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"

#define MD5_LENGTH	16

#define _WIN32_WINNT_VISTA				0x0600
//#define _WIN32_WINNT_WIN7				0x0601
#define _WIN32_WINNT_WIN8				0x0602
//#define _WIN32_WINNT_WINBLUE			0x0603
#define _WIN32_WINNT_WIN10				0x0A00
#define _WIN32_WINNT_WIN_SERVER_2022	0x0A00	// Same as 10
//#define _WIN32_WINNT_WIN11			0x0A00	// Same as 10

#define _WIN32_WINNT_WIN_SERVER_2022_BUILD	20384	// First instance of full TLS 1.3 support.
//#define _WIN32_WINNT_WIN11_BUILD			22000	// First build of Windows 11.

bool IsWindowsVersionOrGreater( WORD wMajorVersion, WORD wMinorVersion, WORD dwBuildNumber );

int dllrbt_compare_a( void *a, void *b );
int dllrbt_compare_w( void *a, void *b );
int dllrbt_compare_i_w( void *a, void *b );

void encode_cipher( char *buffer, int buffer_length );
void decode_cipher( char *buffer, int buffer_length );

wchar_t *GetMonth( unsigned short month );
wchar_t *GetDay( unsigned short day );
void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st );

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns );
int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns );
int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns );

void SetDefaultColumnOrder();
void UpdateColumnOrders();
void CheckColumnOrders( char *column_arr[], unsigned char num_columns );
void CheckColumnWidths();

void SetDefaultAppearance();

BOOL CALLBACK EnumChildFontProc( HWND hWnd, LPARAM lParam );
HFONT UpdateFont( UINT dpi );

char *GlobalStrDupA( const char *_Str );
wchar_t *GlobalStrDupW( const wchar_t *_Str );

wchar_t *UTF8StringToWideString( char *utf8_string, int string_length );
char *WideStringToUTF8String( wchar_t *wide_string, int *utf8_string_length, int buffer_offset = 0 );

char *strnchr( const char *s, int c, int n );

void EscapeFilename( wchar_t *filename );
unsigned long get_file_extension_offset( wchar_t *filename, unsigned long length );
//wchar_t *get_extension_from_filename( wchar_t *filename, unsigned long length );

void GetDownloadFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] );
int GetTemporaryFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] );

BOOL CreateDirectoriesW( LPWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes );

wchar_t *GetDownloadInfoString( DOWNLOAD_INFO *di, int column, int root_index, int item_index, wchar_t *tbuf, unsigned short tbuf_size );

char *escape_csv( const char *string );

unsigned long long strtoull( char *str, bool base16 = false );
unsigned long long wcstoull( wchar_t *str, bool base16 = false );

int GetDomainParts( wchar_t *site, wchar_t *offsets[ 128 ] );

char *GetUTF8Domain( wchar_t *domain );

char *url_encode_a( char *str, unsigned int str_len, unsigned int *enc_len );
char *url_decode_a( char *str, unsigned int str_len, unsigned int *dec_len );

wchar_t *url_encode_w( wchar_t *str, unsigned int str_len, unsigned int *enc_len );
wchar_t *url_decode_w( wchar_t *str, unsigned int str_len, unsigned int *dec_len );

char *html_entity_decode_a( char *str, unsigned int str_len, unsigned int *dec_len );

wchar_t *ParseHTMLClipboard( char *data );

THREAD_RETURN cleanup( void *pArguments );

void FreeCommandLineArgs( CL_ARGS **cla );

char *CreateMD5( BYTE *input, DWORD input_len );
void CreateCNonce( char **cnonce, DWORD *cnonce_length );
void GetMD5String( HCRYPTHASH *hHash, char **md5, DWORD *md5_length );
void CreateDigestAuthorizationInfo( char **nonce, unsigned long &nonce_length, char **opaque, unsigned long &opaque_length );
void CreateDigestAuthorizationKey( char *username, char *password, char *method, char *resource, AUTH_INFO *auth_info, char **auth_key, DWORD *auth_key_length );
void CreateBasicAuthorizationKey( char *username, int username_length, char *password, int password_length, char **auth_key, DWORD *auth_key_length );
bool VerifyDigestAuthorization( char *username, unsigned long username_length, char *password, unsigned long password_length, char *nonce, unsigned long nonce_length, char *opaque, unsigned long opaque_length, char *method, unsigned long method_length, AUTH_INFO *auth_info );
void ConstructRequest( SOCKET_CONTEXT *context, bool use_connect );
void ConstructSOCKSRequest( SOCKET_CONTEXT *context, unsigned char request_type );

unsigned int FormatSizes( wchar_t *buffer, unsigned int buffer_size, unsigned char toggle_type, unsigned long long data_size );
void UpdateSBItemCount();

#ifdef ENABLE_LOGGING

#define LOG_INFO_MISC		0x00000001
#define LOG_INFO_ACTION		0x00000002
#define LOG_INFO_CON_STATE	0x00000004
#define LOG_INFO			( LOG_INFO_MISC | LOG_INFO_ACTION | LOG_INFO_CON_STATE )
#define LOG_WARNING			0x00000008
#define LOG_ERROR			0x00000010

// ntdll
//typedef int ( WINAPIV *p_vsnwprintf )( wchar_t *buffer, size_t count, const wchar_t *format, va_list argptr );
typedef int ( WINAPIV *p_vsnprintf )( char *buffer, size_t count, const char *format, va_list argptr );
typedef void ( WINAPIV *pRtlGetNtVersionNumbers )( LPDWORD major, LPDWORD minor, LPDWORD build );

//extern p_vsnwprintf				__vsnwprintf;
extern p_vsnprintf				__vsnprintf;
extern pRtlGetNtVersionNumbers	_RtlGetNtVersionNumbers;

// ws2_32
/*typedef int ( WSAAPI *pGetNameInfoW )( const SOCKADDR *pSockaddr, socklen_t SockaddrLength, PWCHAR pNodeBuffer, DWORD NodeBufferSize, PWCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags );
typedef int ( WSAAPI *pgetpeername )( SOCKET s, struct sockaddr *name, int *namelen );

extern pGetNameInfoW	_GetNameInfoW;
extern pgetpeername		_getpeername;*/

//

bool InitLogging();
bool UnInitLogging();

void OpenLog( wchar_t *file_path, unsigned int log_filter );
void CloseLog();
void WriteLog( unsigned int type, const char *format, ... );

#define LOG_BUFFER_SIZE	65536

extern char *g_log_buffer;
extern unsigned int g_log_buffer_offset;

#define LOG_BUFFER			( g_log_buffer + g_log_buffer_offset )
#define LOG_BUFFER_OFFSET	( LOG_BUFFER_SIZE - g_log_buffer_offset )

void GetDownloadStatus( char *buf, unsigned short buf_size, unsigned int status );
void GenericLogEntry( DOWNLOAD_INFO *di, unsigned int type, char *msg );

#endif

#endif
