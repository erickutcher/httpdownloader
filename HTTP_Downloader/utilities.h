/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
	Copyright (C) 2015-2020 Eric Kutcher

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

#include "connection.h"
#include "lite_advapi32.h"
#include "lite_crypt32.h"

#define MD5_LENGTH	16

#define _WIN32_WINNT_VISTA		0x0600
//#define _WIN32_WINNT_WIN7		0x0601
#define _WIN32_WINNT_WIN8		0x0602
//#define _WIN32_WINNT_WINBLUE	0x0603

bool IsWindowsVersionOrGreater( WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor );

int dllrbt_compare_a( void *a, void *b );
int dllrbt_compare_w( void *a, void *b );

void encode_cipher( char *buffer, int buffer_length );
void decode_cipher( char *buffer, int buffer_length );

wchar_t *GetMonth( unsigned short month );
wchar_t *GetDay( unsigned short day );
void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st );

int CALLBACK DMCompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns );
int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns );
int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns );

void SetDefaultColumnOrder();
void UpdateColumnOrders();
void CheckColumnOrders( char *column_arr[], unsigned char num_columns );
void CheckColumnWidths();

void SetDefaultAppearance();
void AdjustRowHeight();

char *GlobalStrDupA( const char *_Str );
wchar_t *GlobalStrDupW( const wchar_t *_Str );

char *strnchr( const char *s, int c, int n );

void EscapeFilename( wchar_t *filename );
unsigned long get_file_extension_offset( wchar_t *filename, unsigned long length );
//wchar_t *get_extension_from_filename( wchar_t *filename, unsigned long length );

void GetDownloadFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] );
int GetTemporaryFilePath( DOWNLOAD_INFO *di, wchar_t file_path[] );

char *escape_csv( const char *string );

unsigned long long strtoull( char *str, bool base16 = false );
unsigned long long wcstoull( wchar_t *str, bool base16 = false );

char *GetUTF8Domain( wchar_t *domain );

char *url_encode_a( char *str, unsigned int str_len, unsigned int *enc_len );
char *url_decode_a( char *str, unsigned int str_len, unsigned int *dec_len );

wchar_t *url_encode_w( wchar_t *str, unsigned int str_len, unsigned int *enc_len );
wchar_t *url_decode_w( wchar_t *str, unsigned int str_len, unsigned int *dec_len );

char *html_entity_decode_a( char *str, unsigned int str_len, unsigned int *dec_len );

wchar_t *ParseHTMLClipboard( char *data );

THREAD_RETURN cleanup( void *pArguments );

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

#endif