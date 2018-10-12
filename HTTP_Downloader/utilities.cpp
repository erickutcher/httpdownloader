/*
	HTTP Downloader can download files through HTTP and HTTPS connections.
	Copyright (C) 2015-2018 Eric Kutcher

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

#include "globals.h"
#include "utilities.h"
#include "string_tables.h"

int cfg_pos_x = 0;
int cfg_pos_y = 0;
int cfg_width = MIN_WIDTH;
int cfg_height = MIN_HEIGHT;

char cfg_min_max = 0;	// 0 = normal, 1 = minimized, 2 = maximized.

int cfg_column_width1 = 35;
int cfg_column_width2 = 75;
int cfg_column_width3 = 200;
int cfg_column_width4 = 400;
int cfg_column_width5 = 110;
int cfg_column_width6 = 110;
int cfg_column_width7 = 110;
int cfg_column_width8 = 25;
int cfg_column_width9 = 200;
int cfg_column_width10 = 200;
int cfg_column_width11 = 90;
int cfg_column_width12 = 90;
int cfg_column_width13 = 100;
int cfg_column_width14 = 1000;

// Column (1-14) / Virtual position (0-13)
// Set the visible column to the position indicated in the virtual list.
char cfg_column_order1 = 0;		// 0 # (always 0)
char cfg_column_order2 = 7;		// 1 Active Parts
char cfg_column_order3 = 8;		// 2 Date and Time Added
char cfg_column_order4 = 5;		// 3 Download Directory
char cfg_column_order5 = 6;		// 4 Download Speed
char cfg_column_order6 = 9;		// 5 Downloaded
char cfg_column_order7 = 4;		// 6 File Size
char cfg_column_order8 = 11;	// 7 File Type
char cfg_column_order9 = 1;		// 8 Filename
char cfg_column_order10 = 10;	// 9 Progress
char cfg_column_order11 = 2;	// 10 Time Elapsed
char cfg_column_order12 = 3;	// 11 Time Remaining
char cfg_column_order13 = 12;	// 12 TLS/SSL Version
char cfg_column_order14 = 13;	// 13 URL

bool cfg_show_toolbar = false;
bool cfg_show_status_bar = true;

unsigned char cfg_t_downloaded = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
unsigned char cfg_t_file_size = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
unsigned char cfg_t_down_speed = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto

unsigned char cfg_t_status_downloaded = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto
unsigned char cfg_t_status_down_speed = SIZE_FORMAT_AUTO;	// 0 = Bytes, 1 = KB, 2 = MB, 3 = GB, 4 = auto

int cfg_drop_pos_x = 0;	// URL drop window.
int cfg_drop_pos_y = 0;	// URL drop window.

bool cfg_tray_icon = true;
bool cfg_close_to_tray = false;
bool cfg_minimize_to_tray = false;

bool cfg_always_on_top = false;
bool cfg_enable_download_history = true;
bool cfg_enable_quick_allocation = false;
bool cfg_set_filetime = false;
bool cfg_use_one_instance = false;
bool cfg_enable_drop_window = false;

unsigned long cfg_thread_count = 1;	// Default is 1.
unsigned long g_max_threads = 2;	// Default is 2.

unsigned char cfg_max_downloads = 10;

unsigned char cfg_retry_downloads_count = 2;
unsigned char cfg_retry_parts_count = 0;

unsigned char cfg_default_ssl_version = 4;	// Default is TLS 1.2.
unsigned char cfg_default_download_parts = 1;

unsigned char cfg_max_redirects = 10;

wchar_t *cfg_default_download_directory = NULL;

unsigned int g_default_download_directory_length = 0;

// Server

bool cfg_enable_server = false;
unsigned char cfg_server_address_type = 0;	// 0 = Host name, 1 = IP address
unsigned long cfg_server_ip_address = 2130706433;	// 127.0.0.1
wchar_t *cfg_server_hostname = NULL;
unsigned short cfg_server_port = 80;

unsigned char cfg_server_ssl_version = 2;	// TLS 1.0

bool cfg_server_enable_ssl = false;

unsigned char cfg_certificate_type = 0;	// PKCS

wchar_t *cfg_certificate_pkcs_file_name = NULL;
wchar_t *cfg_certificate_pkcs_password = NULL;

wchar_t *cfg_certificate_cer_file_name = NULL;
wchar_t *cfg_certificate_key_file_name = NULL;

bool cfg_use_authentication = false;
wchar_t *cfg_authentication_username = NULL;
wchar_t *cfg_authentication_password = NULL;
unsigned char cfg_authentication_type = AUTH_TYPE_BASIC;

// HTTP proxy
bool cfg_enable_proxy = false;
unsigned char cfg_address_type = 0;	// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname = NULL;
unsigned short cfg_port = 80;

wchar_t *cfg_proxy_auth_username = NULL;
wchar_t *cfg_proxy_auth_password = NULL;

// HTTPS proxy
bool cfg_enable_proxy_s = false;
unsigned char cfg_address_type_s = 0;	// 0 = Host name, 1 = IP address
unsigned long cfg_ip_address_s = 2130706433;	// 127.0.0.1
wchar_t *cfg_hostname_s = NULL;
unsigned short cfg_port_s = 443;

wchar_t *cfg_proxy_auth_username_s = NULL;
wchar_t *cfg_proxy_auth_password_s = NULL;

//

unsigned short cfg_timeout = 60;

char *download_columns[ NUM_COLUMNS ] = { &cfg_column_order1, &cfg_column_order2, &cfg_column_order3, &cfg_column_order4, &cfg_column_order5, &cfg_column_order6, &cfg_column_order7, &cfg_column_order8, &cfg_column_order9, &cfg_column_order10, &cfg_column_order11, &cfg_column_order12, &cfg_column_order13, &cfg_column_order14 };
int *download_columns_width[ NUM_COLUMNS ] = { &cfg_column_width1, &cfg_column_width2, &cfg_column_width3, &cfg_column_width4, &cfg_column_width5, &cfg_column_width6, &cfg_column_width7, &cfg_column_width8, &cfg_column_width9, &cfg_column_width10, &cfg_column_width11, &cfg_column_width12, &cfg_column_width13, &cfg_column_width14 };

HANDLE worker_semaphore = NULL;			// Blocks shutdown while a worker thread is active.
bool in_worker_thread = false;
bool kill_worker_thread_flag = false;	// Allow for a clean shutdown.

bool download_history_changed = false;

int dllrbt_compare_a( void *a, void *b )
{
	return lstrcmpA( ( char * )a, ( char * )b );
}

int dllrbt_compare_w( void *a, void *b )
{
	return lstrcmpW( ( wchar_t * )a, ( wchar_t * )b );
}

#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 8 - ( n ) ) ) )
#define ROTATE_RIGHT( x, n ) ( ( ( x ) >> ( n ) ) | ( ( x ) << ( 8 - ( n ) ) ) )

void encode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = 0; i < buffer_length; ++i )
	{
		*buffer ^= ( unsigned char )buffer_length;
		*buffer = ( *buffer + offset ) % 256;
		*buffer = ROTATE_LEFT( ( unsigned char )*buffer, offset % 8 );

		buffer++;
		--offset;
	}
}

void decode_cipher( char *buffer, int buffer_length )
{
	int offset = buffer_length + 128;
	for ( int i = buffer_length; i > 0; --i )
	{
		*buffer = ROTATE_RIGHT( ( unsigned char )*buffer, offset % 8 );
		*buffer = ( *buffer - offset ) % 256;
		*buffer ^= ( unsigned char )buffer_length;

		buffer++;
		--offset;
	}
}

wchar_t *GetMonth( unsigned short month )
{
	if ( month > 12 || month < 1 )
	{
		return L"";
	}

	return month_string_table[ month - 1 ];
}

wchar_t *GetDay( unsigned short day )
{
	if ( day > 6 )
	{
		return L"";
	}

	return day_string_table[ day ];
}

void UnixTimeToSystemTime( DWORD t, SYSTEMTIME *st )
{
	FILETIME ft;
	LARGE_INTEGER li;
	li.QuadPart = Int32x32To64( t, 10000000 ) + 116444736000000000;

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	FileTimeToSystemTime( &ft, st );
}

void OffsetVirtualIndices( int *arr, char *column_arr[], unsigned char num_columns, unsigned char total_columns )
{
	for ( unsigned char i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] == -1 )	// See which columns are disabled.
		{
			for ( unsigned char j = 0; j < total_columns; ++j )
			{
				if ( arr[ j ] >= i )	// Increment each virtual column that comes after the disabled column.
				{
					arr[ j ]++;
				}
			}
		}
	}
}

int GetVirtualIndexFromColumnIndex( int column_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( count == column_index )
			{
				return i;
			}
			else
			{
				++count;
			}
		}
	}

	return -1;
}

int GetColumnIndexFromVirtualIndex( int virtual_index, char *column_arr[], unsigned char num_columns )
{
	int count = 0;

	for ( int i = 0; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 && *column_arr[ i ] == virtual_index )
		{
			return count;
		}
		else if ( *column_arr[ i ] != -1 )
		{
			++count;
		}
	}

	return -1;
}

void SetDefaultColumnOrder()
{
	cfg_column_order1 = 0;		// 0 # (always 0)
	cfg_column_order2 = 7;		// 1 Active Parts
	cfg_column_order3 = 8;		// 2 Date and Time Added
	cfg_column_order4 = 5;		// 3 Download Directory
	cfg_column_order5 = 6;		// 4 Download Speed
	cfg_column_order6 = 9;		// 5 Downloaded
	cfg_column_order7 = 4;		// 6 File Size
	cfg_column_order8 = 11;		// 7 File Type
	cfg_column_order9 = 1;		// 8 Filename
	cfg_column_order10 = 10;	// 9 Progress
	cfg_column_order11 = 2;		// 10 Time Elapsed
	cfg_column_order12 = 3;		// 11 Time Remaining
	cfg_column_order13 = 12;	// 12 TLS/SSL Version
	cfg_column_order14 = 13;	// 13 URL
}

void UpdateColumnOrders()
{
	int arr[ NUM_COLUMNS ];
	int offset = 0;
	_SendMessageW( g_hWnd_files, LVM_GETCOLUMNORDERARRAY, g_total_columns, ( LPARAM )arr );
	for ( int i = 0; i < NUM_COLUMNS; ++i )
	{
		if ( *download_columns[ i ] != -1 )
		{
			*download_columns[ i ] = ( char )arr[ offset++ ];
		}
	}
}

void CheckColumnOrders( char *column_arr[], unsigned char num_columns )
{
	// Make sure the first column is always 0 or -1.
	if ( *column_arr[ 0 ] > 0 || *column_arr[ 0 ] < -1 )
	{
		SetDefaultColumnOrder();
		return;
	}

	// Look for duplicates, or values that are out of range.
	unsigned char *is_set = ( unsigned char * )GlobalAlloc( GMEM_FIXED, sizeof( unsigned char ) * num_columns );
	_memzero( is_set, sizeof( unsigned char ) * num_columns );

	// Check ever other column.
	for ( int i = 1; i < num_columns; ++i )
	{
		if ( *column_arr[ i ] != -1 )
		{
			if ( *column_arr[ i ] < num_columns )
			{
				if ( is_set[ *column_arr[ i ] ] == 0 )
				{
					is_set[ *column_arr[ i ] ] = 1;
				}
				else	// Revert duplicate values.
				{
					SetDefaultColumnOrder();

					break;
				}
			}
			else	// Revert out of range values.
			{
				SetDefaultColumnOrder();

				break;
			}
		}
	}

	GlobalFree( is_set );
}

void CheckColumnWidths()
{
	if ( cfg_column_width1 < 0 || cfg_column_width1 > 2560 ) { cfg_column_width1 = 35; }
	if ( cfg_column_width2 < 0 || cfg_column_width2 > 2560 ) { cfg_column_width2 = 75; }
	if ( cfg_column_width3 < 0 || cfg_column_width3 > 2560 ) { cfg_column_width3 = 200; }
	if ( cfg_column_width4 < 0 || cfg_column_width4 > 2560 ) { cfg_column_width4 = 400; }
	if ( cfg_column_width5 < 0 || cfg_column_width5 > 2560 ) { cfg_column_width5 = 110; }
	if ( cfg_column_width6 < 0 || cfg_column_width6 > 2560 ) { cfg_column_width6 = 110; }
	if ( cfg_column_width7 < 0 || cfg_column_width7 > 2560 ) { cfg_column_width7 = 110; }
	if ( cfg_column_width8 < 0 || cfg_column_width8 > 2560 ) { cfg_column_width8 = 25; }
	if ( cfg_column_width9 < 0 || cfg_column_width9 > 2560 ) { cfg_column_width9 = 200; }
	if ( cfg_column_width10 < 0 || cfg_column_width10 > 2560 ) { cfg_column_width10 = 200; }
	if ( cfg_column_width11 < 0 || cfg_column_width11 > 2560 ) { cfg_column_width11 = 90; }
	if ( cfg_column_width12 < 0 || cfg_column_width12 > 2560 ) { cfg_column_width12 = 90; }
	if ( cfg_column_width13 < 0 || cfg_column_width13 > 2560 ) { cfg_column_width13 = 100; }
	if ( cfg_column_width14 < 0 || cfg_column_width14 > 2560 ) { cfg_column_width14 = 1000; }
}

// Must use GlobalFree on this.
char *GlobalStrDupA( const char *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenA( _Str ) + sizeof( char );

	char *ret = ( char * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

// Must use GlobalFree on this.
wchar_t *GlobalStrDupW( const wchar_t *_Str )
{
	if ( _Str == NULL )
	{
		return NULL;
	}

	size_t size = lstrlenW( _Str ) * sizeof( wchar_t ) + sizeof( wchar_t );

	wchar_t *ret = ( wchar_t * )GlobalAlloc( GMEM_FIXED, size );

	if ( ret == NULL )
	{
		return NULL;
	}

	_memcpy_s( ret, size, _Str, size );

	return ret;
}

char *strnchr( const char *s, int c, int n )
{
	if ( s == NULL )
	{
		return NULL;
	}

	while ( *s != NULL && n-- > 0 )
	{
		if ( *s == c )
		{
			return ( ( char * )s );
		}
		++s;
	}
	
	return NULL;
}

// Default is base 10.
unsigned long long strtoull( char *str, bool base16 )
{
	if ( str == NULL )
	{
		return 0;
	}

	char *p = str;

	ULARGE_INTEGER uli;
	uli.QuadPart = 0;

	unsigned char digit = 0;

	if ( !base16 )
	{
		while ( *p && ( *p >= '0' && *p <= '9' ) )
		{
			if ( uli.QuadPart > ( ULLONG_MAX / 10 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 10;

			/*__asm
			{
				mov     eax, dword ptr [ uli.QuadPart + 4 ]
				cmp		eax, 0					;// See if our QuadPart's value extends to 64 bits.
				mov     ecx, 10					;// Store the base (10) multiplier (low order bits).
				jne     short hard10			;// If there are high order bits in QuadPart, then multiply/add high and low bits.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store the QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits.

				jmp		finish10				;// Store the results in our 64 bit value.

			hard10:

				push    ebx						;// Save value to stack.

				mul     ecx						;// Multiply the high order bits of QuadPart with the low order bits of base (10).
				mov     ebx, eax				;// Store the result.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits of QuadPart with the low order bits of base (10). edx = high, eax = low
				add     edx, ebx				;// Add the low order bits (ebx) to the high order bits (edx).

				pop     ebx						;// Restore value from stack.

			finish10:

				mov		uli.HighPart, edx		;// Store the high order bits.
				mov		uli.LowPart, eax		;// Store the low order bits.
			}*/

			digit = *p - '0';

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}
	else
	{
		while ( *p )
		{
			if ( *p >= '0' && *p <= '9' )
			{
				digit = *p - '0';
			}
			else if ( *p >= 'a' && *p <= 'f' )
			{
				digit = *p - 'a' + 10;
			}
			else if ( *p >= 'A' && *p <= 'F' )
			{
				digit = *p - 'A' + 10;
			}
			else
			{
				break;
			}

			if ( uli.QuadPart > ( ULLONG_MAX / 16 ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart *= 16;

			/*__asm
			{
				mov     eax, dword ptr [ uli.QuadPart + 4 ]
				cmp		eax, 0					;// See if our QuadPart's value extends to 64 bits.
				mov     ecx, 16					;// Store the base (16) multiplier (low order bits).
				jne     short hard16			;// If there are high order bits in QuadPart, then multiply/add high and low bits.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store the QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits.

				jmp		finish16				;// Store the results in our 64 bit value.

			hard16:

				push    ebx						;// Save value to stack.

				mul     ecx						;// Multiply the high order bits of QuadPart with the low order bits of base (16).
				mov     ebx, eax				;// Store the result.

				mov     eax, dword ptr [ uli.QuadPart + 0 ]	;// Store QuadPart's low order bits.
				mul     ecx						;// Multiply the low order bits of QuadPart with the low order bits of base (16). edx = high, eax = low
				add     edx, ebx				;// Add the low order bits (ebx) to the high order bits (edx).

				pop     ebx						;// Restore value from stack.

			finish16:

				mov		uli.HighPart, edx		;// Store the high order bits.
				mov		uli.LowPart, eax		;// Store the low order bits.
			}*/

			if ( uli.QuadPart > ( ULLONG_MAX - digit ) )
			{
				uli.QuadPart = ULLONG_MAX;
				break;
			}

			uli.QuadPart += digit;

			++p;
		}
	}

	return uli.QuadPart;
}

void EscapeFilename( wchar_t *filename )
{
	while ( filename != NULL && *filename != NULL )
	{
		if ( *filename == L'\\' ||
			 *filename == L'/' ||
			 *filename == L':' ||
			 *filename == L'*' ||
			 *filename == L'?' ||
			 *filename == L'\"' ||
			 *filename == L'<' ||
			 *filename == L'>' ||
			 *filename == L'|' )
		{
			*filename = L'_';
		}

		++filename;
	}
}

// Returns the position of the file extension if found, or the length of the filename if not found.
unsigned long get_file_extension_offset( wchar_t *filename, unsigned long length )
{
	unsigned long tmp_length = length;

	while ( tmp_length != 0 )
	{
		if ( filename[ --tmp_length ] == L'.' )
		{
			return tmp_length;
		}
	}

	return length;
}

/*wchar_t *get_extension_from_filename( wchar_t *filename, unsigned long length )
{
	while ( length != 0 && filename[ --length ] != L'.' );

	return filename + length;
}*/

char *GetUTF8Domain( wchar_t *domain )
{
	int domain_length = WideCharToMultiByte( CP_UTF8, 0, domain, -1, NULL, 0, NULL, NULL );
	char *utf8_domain = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * domain_length ); // Size includes the null character.
	WideCharToMultiByte( CP_UTF8, 0, domain, -1, utf8_domain, domain_length, NULL, NULL );

	return utf8_domain;
}

// Allocates a new string if characters need escaping. Otherwise, it returns NULL.
char *escape_csv( const char *string )
{
	char *escaped_string = NULL;
	char *q = NULL;
	const char *p = NULL;
	int c = 0;

	if ( string == NULL )
	{
		return NULL;
	}

	// Get the character count and offset it for any quotes.
	for ( c = 0, p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			++c;
		}
		else
		{
			c += 2;
		}
	}

	// If the string has no special characters to escape, then return NULL.
	if ( c <= ( p - string ) )
	{
		return NULL;
	}

	q = escaped_string = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( c + 1 ) );

	for ( p = string; *p != NULL; ++p ) 
	{
		if ( *p != '\"' )
		{
			*q = *p;
			++q;
		}
		else
		{
			*q++ = '\"';
			*q++ = '\"';
		}
	}

	*q = 0;	// Sanity.

	return escaped_string;
}

char to_hex_a( char code )
{
	static char hex[] = "0123456789ABCDEF";
	return hex[ code & 15 ];
}

wchar_t to_hex_w( char code )
{
	static wchar_t hex[] = L"0123456789ABCDEF";
	return hex[ code & 15 ];
}


char from_hex_a( char c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return is_digit_a( *c ) ? *c - '0' : *c - 'a' + 10;

	if ( is_digit_a( c ) )
	{
		return c - '0';
	}
	else if ( c - 'a' + 0U < 6U )
	{
		return c - 'a' + 10;
	}
	else if ( c - 'A' + 0U < 6U )
	{
		return c - 'A' + 10;
	}

	return c;
}

wchar_t from_hex_w( wchar_t c )
{
	//_CharLowerBuffW( ( LPWSTR )c, 1 );
	//return is_digit_w( *c ) ? *c - L'0' : *c - L'a' + 10;

	if ( is_digit_w( c ) )
	{
		return c - L'0';
	}
	else if ( c - L'a' + 0U < 6U )
	{
		return c - L'a' + 10;
	}
	else if ( c - L'A' + 0U < 6U )
	{
		return c - L'A' + 10;
	}

	return c;
}

bool is_hex_a( char c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return ( is_digit_a( *c ) || ( *c - 'a' + 0U < 6U ) );

	return ( is_digit_a( c ) || ( c - 'a' + 0U < 6U ) || ( c - 'A' + 0U < 6U ) );
}

bool is_hex_w( wchar_t c )
{
	//_CharLowerBuffA( ( LPSTR )c, 1 );
	//return ( is_digit_w( *c ) || ( *c - 'a' + 0U < 6U ) );

	return ( is_digit_w( c ) || ( c - L'a' + 0U < 6U ) || ( c - L'A' + 0U < 6U ) );
}

char *url_encode_a( char *str, unsigned int str_len, unsigned int *enc_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( ( str_len * 3 ) + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		/*if ( _IsCharAlphaNumericA( *pstr ) )
		{
			*pbuf++ = *pstr;
		}
		else if ( *pstr == ' ' )
		{
			*pbuf++ = '%';
			*pbuf++ = '2';
			*pbuf++ = '0';
		}
		else
		{
			*pbuf++ = '%';
			*pbuf++ = to_hex_a( *pstr >> 4 );
			*pbuf++ = to_hex_a( *pstr & 15 );
		}*/

		if ( *pstr == ' ' ||
			 *pstr == '<' ||
			 *pstr == '>' ||
			 *pstr == '#' ||
			 *pstr == '%' ||
			 *pstr == '\"' ||
			 *pstr == '{' ||
			 *pstr == '}' ||
			 *pstr == '|' ||
			 *pstr == '\\' ||
			 *pstr == '^' ||
			 *pstr == '[' ||
			 *pstr == ']' ||
			 *pstr == '`' ||
			 *pstr == 0x7F ||
			 ( *pstr >= 0x00 && *pstr <= 0x1F ) )
		{
			*pbuf++ = L'%';
			*pbuf++ = to_hex_a( *pstr >> 4 );
			*pbuf++ = to_hex_a( *pstr & 15 );
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = '\0';

	if ( enc_len != NULL )
	{
		*enc_len = pbuf - buf;
	}

	return buf;
}

wchar_t *url_encode_w( wchar_t *str, unsigned int str_len, unsigned int *enc_len )
{
	wchar_t *pstr = str;
	wchar_t *buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( ( str_len * 3 ) + 1 ) );
	wchar_t *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		/*if ( _IsCharAlphaNumericW( *pstr ) )
		{
			*pbuf++ = *pstr;
		}
		else if ( *pstr == L' ' )
		{
			*pbuf++ = L'%';
			*pbuf++ = L'2';
			*pbuf++ = L'0';
		}
		else
		{
			*pbuf++ = L'%';
			*pbuf++ = to_hex_w( *pstr >> 4 );
			*pbuf++ = to_hex_w( *pstr & 15 );
		}*/

		if ( *pstr == L' ' ||
			 *pstr == L'<' ||
			 *pstr == L'>' ||
			 *pstr == L'#' ||
			 *pstr == L'%' ||
			 *pstr == L'\"' ||
			 *pstr == L'{' ||
			 *pstr == L'}' ||
			 *pstr == L'|' ||
			 *pstr == L'\\' ||
			 *pstr == L'^' ||
			 *pstr == L'[' ||
			 *pstr == L']' ||
			 *pstr == L'`' ||
			 *pstr == 0x7F ||
			 ( *pstr >= 0x00 && *pstr <= 0x1F ) )
		{
			*pbuf++ = L'%';
			*pbuf++ = to_hex_w( *pstr >> 4 );
			*pbuf++ = to_hex_w( *pstr & 15 );
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = L'\0';

	if ( enc_len != NULL )
	{
		*enc_len = pbuf - buf;
	}

	return buf;
}

char *url_decode_a( char *str, unsigned int str_len, unsigned int *dec_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( str_len + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == '%' )
		{
			// Look at the next two characters.
			if ( ( ( pstr + 3 ) <= ( str + str_len ) ) )
			{
				// See if they're both hex values.
				if ( ( pstr[ 1 ] != NULL && is_hex_a( pstr[ 1 ] ) ) &&
					 ( pstr[ 2 ] != NULL && is_hex_a( pstr[ 2 ] ) ) )
				{
					*pbuf++ = from_hex_a( pstr[ 1 ] ) << 4 | from_hex_a( pstr[ 2 ] );
					pstr += 2;
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			else
			{
				*pbuf++ = *pstr;
			}
		}
		else if ( *pstr == '+' )
		{ 
			*pbuf++ = ' ';
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = '\0';

	if ( dec_len != NULL )
	{
		*dec_len = pbuf - buf;
	}

	return buf;
}

wchar_t *url_decode_w( wchar_t *str, unsigned int str_len, unsigned int *dec_len )
{
	wchar_t *pstr = str;
	wchar_t *buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( str_len + 1 ) );
	wchar_t *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == L'%' )
		{
			// Look at the next two characters.
			if ( ( ( pstr + 3 ) <= ( str + str_len ) ) )
			{
				// See if they're both hex values.
				if ( ( pstr[ 1 ] != NULL && is_hex_w( pstr[ 1 ] ) ) &&
					 ( pstr[ 2 ] != NULL && is_hex_w( pstr[ 2 ] ) ) )
				{
					*pbuf++ = from_hex_w( pstr[ 1 ] ) << 4 | from_hex_w( pstr[ 2 ] );
					pstr += 2;
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			else
			{
				*pbuf++ = *pstr;
			}
		}
		else if ( *pstr == L'+' )
		{ 
			*pbuf++ = L' ';
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = L'\0';

	if ( dec_len != NULL )
	{
		*dec_len = pbuf - buf;
	}

	return buf;
}

char *html_entity_decode_a( char *str, unsigned int str_len, unsigned int *dec_len )
{
	char *pstr = str;
	char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( str_len + 1 ) );
	char *pbuf = buf;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == '&' )
		{
			// Look at the next two characters.
			if ( ( ( pstr + 5 ) <= ( str + str_len ) ) )
			{
				// See if they're both hex values.
				if ( ( pstr[ 1 ] != NULL && pstr[ 1 ] == 'a' ) &&
					 ( pstr[ 2 ] != NULL && pstr[ 2 ] == 'm' ) &&
					 ( pstr[ 3 ] != NULL && pstr[ 3 ] == 'p' ) &&
					 ( pstr[ 4 ] != NULL && pstr[ 4 ] == ';' ) )
				{
					*pbuf++ = '&';
					pstr += 4;
				}
				else
				{
					*pbuf++ = *pstr;
				}
			}
			else
			{
				*pbuf++ = *pstr;
			}
		}
		else if ( *pstr == '+' )
		{ 
			*pbuf++ = ' ';
		}
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = '\0';

	if ( dec_len != NULL )
	{
		*dec_len = pbuf - buf;
	}

	return buf;
}

void kill_worker_thread()
{
	if ( in_worker_thread )
	{
		// This semaphore will be released when the thread gets killed.
		worker_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

		kill_worker_thread_flag = true;	// Causes secondary threads to cease processing and release the semaphore.

		// Wait for any active threads to complete. 5 second timeout in case we miss the release.
		WaitForSingleObject( worker_semaphore, 5000 );
		CloseHandle( worker_semaphore );
		worker_semaphore = NULL;
	}
}

// This will allow our main thread to continue while secondary threads finish their processing.
THREAD_RETURN cleanup( void *pArguments )
{
	kill_worker_thread();

	// DestroyWindow won't work on a window from a different thread. So we'll send a message to trigger it.
	_SendMessageW( g_hWnd_main, WM_DESTROY_ALT, 0, 0 );

	_ExitThread( 0 );
	return 0;
}

char *CreateMD5( BYTE *input, DWORD input_len )
{
	char *md5 = NULL;

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			if ( _CryptHashData( hHash, input, input_len, 0 ) )
			{
				DWORD cbHash = MD5_LENGTH;
				BYTE Hash[ MD5_LENGTH ];

				if ( _CryptGetHashParam( hHash, HP_HASHVAL, Hash, &cbHash, 0 ) )
				{
					DWORD md5_length = cbHash * 2;
					md5 = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( md5_length + 1 ) );

					CHAR digits[] = "0123456789abcdef";
					for ( DWORD i = 0; i < cbHash; ++i )
					{
						__snprintf( md5 + ( 2 * i ), md5_length - ( 2 * i ), "%c%c", digits[ Hash[ i ] >> 4 ], digits[ Hash[ i ] & 0xF ] );
					}
					md5[ md5_length ] = 0;	// Sanity.
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	return md5;
}

void CreateCNonce( char **cnonce, DWORD *cnonce_length )
{
	*cnonce = NULL;
	*cnonce_length = 0;

	HCRYPTPROV hProvider = NULL;

	if ( _CryptAcquireContextW( &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT ) )
	{
		BYTE rbuffer[ 8 ];
		_memzero( rbuffer, 8 );

		if ( _CryptGenRandom( hProvider, 8, ( BYTE * )&rbuffer ) )
		{
			*cnonce_length = 16;
			*cnonce = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( *cnonce_length + 1 ) );

			CHAR digits[] = "0123456789abcdef";
			for ( DWORD i = 0; i < 8; ++i )
			{
				__snprintf( *cnonce + ( 2 * i ), *cnonce_length - ( 2 * i ), "%c%c", digits[ rbuffer[ i ] >> 4 ], digits[ rbuffer[ i ] & 0xF ] );
			}
			*( *cnonce + *cnonce_length ) = 0;	// Sanity.
		}
	}

	if ( hProvider != NULL )
	{
		_CryptReleaseContext( hProvider, 0 );
	}
}

void GetMD5String( HCRYPTHASH *hHash, char **md5, DWORD *md5_length )
{
	DWORD cbHash = MD5_LENGTH;
	BYTE Hash[ MD5_LENGTH ];

	*md5 = NULL;
	*md5_length = 0;

	if ( _CryptGetHashParam( *hHash, HP_HASHVAL, Hash, &cbHash, 0 ) )
	{
		*md5_length = cbHash * 2;
		*md5 = ( char * )GlobalAlloc( GPTR, sizeof( char ) * ( *md5_length + 1 ) );

		CHAR digits[] = "0123456789abcdef";
		for ( DWORD i = 0; i < cbHash; ++i )
		{
			__snprintf( *md5 + ( 2 * i ), *md5_length - ( 2 * i ), "%c%c", digits[ Hash[ i ] >> 4 ], digits[ Hash[ i ] & 0xF ] );
		}
		*( *md5 + *md5_length ) = 0;	// Sanity.
	}
}

void CreateDigestAuthorizationInfo( char **nonce, unsigned long &nonce_length, char **opaque, unsigned long &opaque_length )
{
	char *HA1 = NULL;

	if ( *nonce != NULL )
	{
		GlobalFree( *nonce );
		*nonce = NULL;
	}

	nonce_length = 0;

	if ( *opaque != NULL )
	{
		GlobalFree( *opaque );
		*opaque = NULL;
	}

	opaque_length = 0;

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		BYTE rbuffer[ 16 ];

		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptGenRandom( hProv, 16, ( BYTE * )&rbuffer );

			_CryptHashData( hHash, rbuffer, 16, 0 );

			GetMD5String( &hHash, nonce, &nonce_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptGenRandom( hProv, 16, ( BYTE * )&rbuffer );

			_CryptHashData( hHash, rbuffer, 16, 0 );

			GetMD5String( &hHash, opaque, &opaque_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
}

void CreateDigestAuthorizationKey( char *username, char *password, char *method, char *resource, AUTH_INFO *auth_info, char **auth_key, DWORD *auth_key_length )
{
	*auth_key = NULL;
	*auth_key_length = 0;
	DWORD auth_key_offset = 0;

	char *HA1 = NULL;
	DWORD HA1_length = 0;

	char *HA2 = NULL;
	DWORD HA2_length = 0;

	char *response = NULL;
	DWORD response_length = 0;

	char *cnonce = NULL;
	DWORD cnonce_length = 0;

	int username_length = 0;
	int password_length = 0;
	int realm_length = 0;

	int nonce_length = 0;
	int uri_length = 0;
	int opaque_length = 0;
	int qop_length = 0;

	int method_length = 0;

	++auth_info->nc;	// Update regardless of whether we have a qop value. Don't want to get in an infinite loop.

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		// If auth_info.algorithm is not set, then assume it's MD5.

		// Create HA1.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			nonce_length = lstrlenA( auth_info->nonce );

			username_length = lstrlenA( username );
			password_length = lstrlenA( password );
			realm_length = lstrlenA( auth_info->realm );

			_CryptHashData( hHash, ( BYTE * )username, username_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->realm, realm_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )password, password_length, 0 );

			GetMD5String( &hHash, &HA1, &HA1_length );

			// MD5-sess
			if ( auth_info->algorithm == 2 )
			{
				if ( hHash != NULL )
				{
					_CryptDestroyHash( hHash );
					hHash = NULL;
				}

				if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
				{
					CreateCNonce( &cnonce, &cnonce_length );

					_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )auth_info->nonce, nonce_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )cnonce, cnonce_length, 0 );

					GlobalFree( HA1 );
					HA1 = NULL;
					HA1_length = 0;

					GetMD5String( &hHash, &HA1, &HA1_length );
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create HA2.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			uri_length = lstrlenA( resource );
			method_length = lstrlenA( method );

			_CryptHashData( hHash, ( BYTE * )method, method_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )resource, uri_length, 0 );

			// auth-int
			// We're not supporting this.
			// We'd have to stream in the HTTP payload body and who knows how large that could be. Forget it!
			if ( auth_info->qop_type == 2 )
			{
				char *entity_body = NULL;
				int entity_body_length = 0;

				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )entity_body, entity_body_length, 0 );
			}

			GetMD5String( &hHash, &HA2, &HA2_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create response.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->nonce, nonce_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );

			if ( auth_info->qop_type != 0 )
			{
				char ncount[ 9 ];
				__snprintf( ncount, 9, "%08x", auth_info->nc );	// Hex must be lowercase.

				if ( cnonce == NULL )
				{
					CreateCNonce( &cnonce, &cnonce_length );
				}

				qop_length = lstrlenA( auth_info->qop );

				_CryptHashData( hHash, ( BYTE * )ncount, 8, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )cnonce, cnonce_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->qop, qop_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			}

			_CryptHashData( hHash, ( BYTE * )HA2, HA2_length, 0 );

			GetMD5String( &hHash, &response, &response_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
	GlobalFree( HA2 );

	if ( auth_info->opaque != NULL )
	{
		opaque_length = lstrlenA( auth_info->opaque );
	}

	// Add up all the value lengths. Use an additional 128 for the value names.
	*auth_key_length = username_length + realm_length + nonce_length + uri_length + response_length + opaque_length + cnonce_length + 128;

	*auth_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( *auth_key_length + 1 ) );

	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "username=\"%s\", ", SAFESTRA( username ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "realm=\"%s\", ", SAFESTRA( auth_info->realm ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "nonce=\"%s\", ", SAFESTRA( auth_info->nonce ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "uri=\"%s\", ", SAFESTRA( resource ) );
	if ( auth_info->algorithm != 0 )
	{
		auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "algorithm=\"%s\", ", ( auth_info->algorithm == 2 ? "MD5-sess" : "MD5" ) );
	}
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "response=\"%s\", ", SAFESTRA( response ) );
	if ( auth_info->opaque != NULL )
	{
		auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "opaque=\"%s\", ", auth_info->opaque );
	}
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "qop=\"%s\", ", SAFESTRA( auth_info->qop ) );
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "nc=\"%08x\", ", auth_info->nc );	// Hex must be lowercase.
	auth_key_offset += __snprintf( *auth_key + auth_key_offset, *auth_key_length - auth_key_offset, "cnonce=\"%s\"", SAFESTRA( cnonce ) );

	GlobalFree( cnonce );
	GlobalFree( response );

	*auth_key_length = auth_key_offset;
}

void CreateBasicAuthorizationKey( char *username, int username_length, char *password, int password_length, char **auth_key, DWORD *auth_key_length )
{
	*auth_key = NULL;
	*auth_key_length = 0;

	if ( username_length < 0 )
	{
		username_length = lstrlenA( username );
	}
	if ( password_length < 0 )
	{
		password_length = lstrlenA( password );
	}
	int key_length = username_length + password_length + 1;	// Include ":".

	char *key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( key_length + 1 ) );
	_memcpy_s( key, key_length + 1, username, username_length );
	key[ username_length ] = ':';
	_memcpy_s( key + username_length + 1, ( key_length + 1 ) - ( username_length + 1 ), password, password_length );
	key[ key_length ] = 0;	// Sanity.

	_CryptBinaryToStringA( ( BYTE * )key, key_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, auth_key_length );	// auth_key_length WILL include the NULL terminator.

	*auth_key = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( *auth_key_length ) );
	_CryptBinaryToStringA( ( BYTE * )key, key_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, ( LPSTR )*auth_key, auth_key_length );	// auth_key_length DOES NOT include the NULL terminator.
	*( *auth_key + *auth_key_length ) = 0; // Sanity.

	GlobalFree( key );
}

bool VerifyDigestAuthorization( char *username, unsigned long username_length, char *password, unsigned long password_length, char *nonce, unsigned long nonce_length, char *opaque, unsigned long opaque_length, char *method, unsigned long method_length, AUTH_INFO *auth_info )
{
	bool ret = false;

	char *HA1 = NULL;
	DWORD HA1_length = 0;

	char *HA2 = NULL;
	DWORD HA2_length = 0;

	char *response = NULL;
	DWORD response_length = 0;

	int client_response_length = lstrlenA( auth_info->response );
	int client_nonce_length = lstrlenA( auth_info->nonce );
	int client_opaque_length = lstrlenA( auth_info->opaque );

	int cnonce_length = lstrlenA( auth_info->cnonce );
	int realm_length = lstrlenA( auth_info->realm );
	int uri_length = lstrlenA( auth_info->uri );
	int qop_length = lstrlenA( auth_info->qop );

	// We can verify realm, nonce, and opaque to ensure the client responded correctly.
	if ( realm_length != 22 || _StrCmpNA( auth_info->realm, "Authorization Required", 22 != 0 ) )
	{
		return false;
	}

	if ( client_nonce_length != nonce_length || _StrCmpNA( auth_info->nonce, nonce, nonce_length ) != 0 )
	{
		return false;
	}

	if ( client_opaque_length != opaque_length || _StrCmpNA( auth_info->opaque, opaque, opaque_length ) != 0 )
	{
		return false;
	}

	HCRYPTPROV hProv = NULL;
	if ( _CryptAcquireContextW( &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) )
	{
		HCRYPTHASH hHash = NULL;

		// If auth_info->algorithm is not set, then assume it's MD5.

		// Create HA1.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )username, username_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->realm, realm_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )password, password_length, 0 );

			GetMD5String( &hHash, &HA1, &HA1_length );

			// MD5-sess
			if ( auth_info->algorithm == 2 )
			{
				if ( hHash != NULL )
				{
					_CryptDestroyHash( hHash );
					hHash = NULL;
				}

				if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
				{
					_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )nonce, nonce_length, 0 );
					_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
					_CryptHashData( hHash, ( BYTE * )auth_info->cnonce, cnonce_length, 0 );

					GlobalFree( HA1 );
					HA1 = NULL;
					HA1_length = 0;

					GetMD5String( &hHash, &HA1, &HA1_length );
				}
			}
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create HA2.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )method, method_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )auth_info->uri, uri_length, 0 );

			// auth-int
			// We're not supporting this.
			// We'd have to stream in the HTTP payload body and who knows how large that could be. Forget it!
			if ( auth_info->qop_type == 2 )
			{
				char *entity_body = NULL;
				int entity_body_length = 0;

				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )entity_body, entity_body_length, 0 );
			}

			GetMD5String( &hHash, &HA2, &HA2_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}

		// Create response.
		if ( _CryptCreateHash( hProv, CALG_MD5, 0, 0, &hHash ) )
		{
			_CryptHashData( hHash, ( BYTE * )HA1, HA1_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			_CryptHashData( hHash, ( BYTE * )nonce, nonce_length, 0 );
			_CryptHashData( hHash, ( BYTE * )":", 1, 0 );

			if ( auth_info->qop_type != 0 )
			{
				char ncount[ 9 ];
				__snprintf( ncount, 9, "%08x", auth_info->nc );	// Hex must be lowercase.

				_CryptHashData( hHash, ( BYTE * )ncount, 8, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->cnonce, cnonce_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
				_CryptHashData( hHash, ( BYTE * )auth_info->qop, qop_length, 0 );
				_CryptHashData( hHash, ( BYTE * )":", 1, 0 );
			}

			_CryptHashData( hHash, ( BYTE * )HA2, HA2_length, 0 );

			GetMD5String( &hHash, &response, &response_length );
		}

		if ( hHash != NULL )
		{
			_CryptDestroyHash( hHash );
			hHash = NULL;
		}
	}

	if ( hProv != NULL )
	{
		_CryptReleaseContext( hProv, 0 );
	}

	GlobalFree( HA1 );
	GlobalFree( HA2 );

	if ( response != NULL )
	{
		if ( response_length == client_response_length && _StrCmpNA( response, auth_info->response, response_length ) == 0 )
		{
			ret = true;
		}

		GlobalFree( response );
	}

	return ret;
}

void ConstructRequest( SOCKET_CONTEXT *context, bool use_connect )
{
	unsigned int request_length = 0;

	if ( use_connect )
	{
		request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
				"CONNECT %s:%lu " \
				"HTTP/1.1\r\n" \
				"Host: %s:%lu\r\n"/* \
				"Cache-Control: no-cache\r\n"*/, context->request_info.host, context->request_info.port, context->request_info.host, context->request_info.port );
	}
	else
	{
		request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
				"GET %s " \
				"HTTP/1.1\r\n" \
				"Host: %s\r\n"/* \
				"Cache-Control: no-cache\r\n"*/, context->request_info.resource, context->request_info.host );

		//_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Accept-Encoding: gzip, deflate\r\n\0", 33 );
		//request_length += 32;

		if ( context->header_info.content_encoding == CONTENT_ENCODING_GZIP )
		{
			_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Accept-Encoding: gzip\r\n\0", 24 );
			request_length += 23;
		}
		else if ( context->header_info.content_encoding == CONTENT_ENCODING_DEFLATE )
		{
			_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Accept-Encoding: deflate\r\n\0", 27 );
			request_length += 26;
		}
		else
		{
			_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Accept-Encoding: identity\r\n\0", 28 );
			request_length += 27;
		}

		// If we're working with a range, then set it.
		if ( context->parts > 1 ||
		   ( context->header_info.range_info->range_start > 0 &&
			 context->header_info.range_info->range_end > 0 ) )
		{
			request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
					"Range: bytes=%llu-%llu\r\n", context->header_info.range_info->range_start, context->header_info.range_info->range_end );
		}

		/*if ( context->header_info.etag )
		{
			if ( context->download_info != NULL && context->download_info->etag != NULL )
			{
				request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
						"If-Match: %s\r\n", context->download_info->etag );
			}
		}*/

		// Add extra headers.
		if ( context->download_info != NULL && context->download_info->headers != NULL )
		{
			request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
					"%s", context->download_info->headers );
		}

		if ( context->header_info.digest_info != NULL )
		{
			if ( context->header_info.digest_info->auth_type == AUTH_TYPE_BASIC )
			{
				if ( context->download_info != NULL )
				{
					char *auth_key = NULL;
					DWORD auth_key_length = 0;
					CreateBasicAuthorizationKey( context->download_info->auth_info.username, -1, context->download_info->auth_info.password, -1, &auth_key, &auth_key_length );

					// Even though basic authorization doesn't use a nonce count, we'll use it so we know when to stop retrying the authorization.
					++context->header_info.digest_info->nc;

					if ( auth_key != NULL )
					{
						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Authorization: Basic ", 21 );
						request_length += 21;

						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, auth_key, auth_key_length );
						request_length += auth_key_length;

						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "\r\n\0", 3 );
						request_length += 2;

						GlobalFree( auth_key );
					}
				}
			}
			else if ( context->header_info.digest_info->auth_type == AUTH_TYPE_DIGEST )
			{
				if ( context->download_info != NULL )
				{
					char *auth_key = NULL;
					DWORD auth_key_length = 0;
					CreateDigestAuthorizationKey( context->download_info->auth_info.username,
												  context->download_info->auth_info.password,
												  ( use_connect ? "CONNECT" : "GET" ),
												  context->request_info.resource,
												  context->header_info.digest_info,
												  &auth_key,
												  &auth_key_length );

					if ( auth_key != NULL )
					{
						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Authorization: Digest ", 22 );
						request_length += 22;

						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, auth_key, auth_key_length );
						request_length += auth_key_length;

						_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "\r\n\0", 3 );
						request_length += 2;

						GlobalFree( auth_key );
					}
				}
			}
		}

		if ( context->header_info.cookies != NULL )
		{
			request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
					"Cookie: %s\r\n", context->header_info.cookies );
		}

		/*request_length += __snprintf( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length,
			"Referer: %s%s%s\r\n", context->request_info.protocol == PROTOCOL_HTTPS ? "https://" : "http://", context->request_info.host, context->request_info.resource );*/
	}

	char *proxy_auth_key = NULL;
	unsigned long proxy_auth_key_length = 0;

	char *proxy_auth_username = NULL;
	char *proxy_auth_password = NULL;

	if ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP )
	{
		proxy_auth_key = g_proxy_auth_key;
		proxy_auth_key_length = g_proxy_auth_key_length;

		proxy_auth_username = g_proxy_auth_username;
		proxy_auth_password = g_proxy_auth_password;
	}
	else if ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS )
	{
		proxy_auth_key = g_proxy_auth_key_s;
		proxy_auth_key_length = g_proxy_auth_key_length_s;

		proxy_auth_username = g_proxy_auth_username_s;
		proxy_auth_password = g_proxy_auth_password_s;
	}

	if ( ( cfg_enable_proxy && context->request_info.protocol == PROTOCOL_HTTP ) ||
		 ( cfg_enable_proxy_s && context->request_info.protocol == PROTOCOL_HTTPS ) )
	{
		if ( context->header_info.proxy_digest_info != NULL )
		{
			if ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_BASIC )
			{
				// Even though basic authorization doesn't use a nonce count, we'll use it so we know when to stop retrying the authorization.
				++context->header_info.proxy_digest_info->nc;

				_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Proxy-Authorization: Basic ", 27 );
				request_length += 27;

				_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, proxy_auth_key, proxy_auth_key_length );
				request_length += proxy_auth_key_length;

				_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "\r\n\0", 3 );
				request_length += 2;
			}
			else if ( context->header_info.proxy_digest_info->auth_type == AUTH_TYPE_DIGEST )
			{
				char *auth_key = NULL;
				DWORD auth_key_length = 0;
				CreateDigestAuthorizationKey( proxy_auth_username,
											  proxy_auth_password,
											  ( use_connect ? "CONNECT" : "GET" ),
											  context->request_info.resource,
											  context->header_info.proxy_digest_info,
											  &auth_key,
											  &auth_key_length );

				if ( auth_key != NULL )
				{
					_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Proxy-Authorization: Digest ", 28 );
					request_length += 28;

					_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, auth_key, auth_key_length );
					request_length += auth_key_length;

					_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "\r\n\0", 3 );
					request_length += 2;

					GlobalFree( auth_key );
				}
			}
		}
	}

	if ( context->parts > 1 )
	{
		_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Connection: keep-alive\r\n\r\n\0", 27 );
		request_length += 26;
	}
	else
	{
		_memcpy_s( context->wsabuf.buf + request_length, BUFFER_SIZE - request_length, "Connection: close\r\n\r\n\0", 22 );
		request_length += 21;
	}

	context->wsabuf.len = request_length;
}

