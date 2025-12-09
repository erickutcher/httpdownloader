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

#include "site_manager_utilities.h"

#include "lite_normaliz.h"

#include "utilities.h"
#include "file_operations.h"

#include "http_parsing.h"
#include "connection.h"

bool site_list_changed = false;

void FreeSiteInfo( SITE_INFO **site_info )
{
	if ( *site_info != NULL )
	{
		if ( ( *site_info )->w_host != NULL ) { GlobalFree( ( *site_info )->w_host ); }
		if ( ( *site_info )->w_username != NULL ) { GlobalFree( ( *site_info )->w_username ); }
		if ( ( *site_info )->w_password != NULL ) { GlobalFree( ( *site_info )->w_password ); }
		if ( ( *site_info )->host != NULL ) { GlobalFree( ( *site_info )->host ); }
		if ( ( *site_info )->username != NULL ) { GlobalFree( ( *site_info )->username ); }
		if ( ( *site_info )->password != NULL ) { GlobalFree( ( *site_info )->password ); }
		if ( ( *site_info )->category != NULL ) { GlobalFree( ( *site_info )->category ); }
		if ( ( *site_info )->download_directory != NULL ) { GlobalFree( ( *site_info )->download_directory ); }
		if ( ( *site_info )->comments != NULL ) { GlobalFree( ( *site_info )->comments ); }
		if ( ( *site_info )->utf8_cookies != NULL ) { GlobalFree( ( *site_info )->utf8_cookies ); }
		if ( ( *site_info )->utf8_headers != NULL ) { GlobalFree( ( *site_info )->utf8_headers ); }
		if ( ( *site_info )->utf8_data != NULL ) { GlobalFree( ( *site_info )->utf8_data ); }
		if ( ( *site_info )->proxy_info.hostname != NULL ) { GlobalFree( ( *site_info )->proxy_info.hostname ); }
		if ( ( *site_info )->proxy_info.punycode_hostname != NULL ) { GlobalFree( ( *site_info )->proxy_info.punycode_hostname ); }
		if ( ( *site_info )->proxy_info.w_username != NULL ) { GlobalFree( ( *site_info )->proxy_info.w_username ); }
		if ( ( *site_info )->proxy_info.w_password != NULL ) { GlobalFree( ( *site_info )->proxy_info.w_password ); }
		if ( ( *site_info )->proxy_info.username != NULL ) { GlobalFree( ( *site_info )->proxy_info.username ); }
		if ( ( *site_info )->proxy_info.password != NULL ) { GlobalFree( ( *site_info )->proxy_info.password ); }

		GlobalFree( *site_info );

		*site_info = NULL;
	}
}

int dllrbt_compare_site_info( void *a, void *b )
{
	SITE_INFO *a1 = ( SITE_INFO * )a;
	SITE_INFO *b1 = ( SITE_INFO * )b;

	int ret = 0;

	if ( a1 == b1 )
	{
		return 0;
	}

	// Check the easiest comparisons first.
	if ( a1->protocol > b1->protocol )
	{
		ret = 1;
	}
	else if ( a1->protocol <  b1->protocol )
	{
		ret = -1;
	}
	else
	{
		if ( a1->port > b1->port )
		{
			ret = 1;
		}
		else if ( a1->port <  b1->port )
		{
			ret = -1;
		}
		else
		{
			wchar_t *host1_offset[ 128 ];
			int count1 = GetDomainParts( a1->host, host1_offset );

			wchar_t *host2_offset[ 128 ];
			int count2 = GetDomainParts( b1->host, host2_offset );

			if ( count1 == count2 )
			{
				for ( int i = count1; i > 0; --i )
				{
					wchar_t *start1 = host1_offset[ i - 1 ];
					wchar_t *start2 = host2_offset[ i - 1 ];

					wchar_t *end1 = host1_offset[ i ];
					wchar_t *end2 = host2_offset[ i ];

					if ( !( ( ( ( end1 - start1 ) == 2 ) && *start1 == L'*' && *( start1 + 1 ) == L'.' ) ||
							( ( ( end2 - start2 ) == 2 ) && *start2 == L'*' && *( start2 + 1 ) == L'.' ) ||
							( ( ( end1 - start1 ) == 1 ) && *start1 == L'*' && *( start1 + 1 ) == NULL ) ||
							( ( ( end2 - start2 ) == 1 ) && *start2 == L'*' && *( start2 + 1 ) == NULL ) ) )
					{
						wchar_t tmp1 = *end1;
						wchar_t tmp2 = *end2;

						*end1 = 0;
						*end2 = 0;

						ret = lstrcmpW( start1, start2 );

						*end1 = tmp1;	// Restore
						*end2 = tmp2;	// Restore

						if ( ret != 0 )
						{
							break;
						}
					}
				}
			}
			else
			{
				ret = ( count1 > count2 ? 1 : -1 );
			}
		}
	}

	return ret;
}

char read_site_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\site_settings\0", 15 );
	//g_base_directory[ g_base_directory_length + 14 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Reading site settings: %S", g_base_directory );
#endif

	HANDLE hFile_read = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile_read = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile_read, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		bool				enable;

		wchar_t				*site;

		//

		wchar_t				*category;

		bool				use_download_directory;
		wchar_t				*download_directory;

		bool				use_parts;
		unsigned char		parts;

		bool				use_download_speed_limit;
		unsigned long long	download_speed_limit;

		char				ssl_version;

		char				*username;
		char				*password;
		wchar_t				*w_username;
		wchar_t				*w_password;

		unsigned int		download_operations;

		//

		wchar_t				*comments;
		char				*cookies;
		char				*headers;
		unsigned char		method;
		char				*data;

		//

		unsigned char		proxy_type;

		unsigned char		proxy_address_type;

		unsigned int		hostname_length;
		wchar_t				*proxy_hostname;
		unsigned long		proxy_ip_address;

		unsigned short		proxy_port;

		char				*proxy_auth_username;
		char				*proxy_auth_password;
		wchar_t				*w_proxy_auth_username;
		wchar_t				*w_proxy_auth_password;

		bool				proxy_resolve_domain_names;
		bool				proxy_use_authentication;

		//

		unsigned char magic_identifier[ 4 ];
		BOOL bRet = ReadFile( hFile_read, magic_identifier, sizeof( unsigned char ) * 4, &read, NULL );
		if ( bRet != FALSE )
		{
#ifdef ENABLE_LOGGING
			lfz += 4;
#endif
			unsigned char version = magic_identifier[ 3 ] - 0x20;

			if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_SITES, 3 ) == 0 && version <= 0x0F )
			{
				char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 3 ) );	// 512 KB buffer.
				if ( buf != NULL )
				{
					DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

					while ( total_read < fz )
					{
						bRet = ReadFile( hFile_read, buf, sizeof( char ) * 524288, &read, NULL );
						if ( bRet == FALSE )
						{
							break;
						}

#ifdef ENABLE_LOGGING
						lfz += read;
#endif

						buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

						// This terminates wide character strings so we don't read past the buffer.
						buf[ read + 1 ] = 0;
						buf[ read + 2 ] = 0;

						/*// Make sure that we have at least part of the entry. This is the minimum size an entry could be.
						// Include 2 ints for username and password lengths.
						if ( read < ( sizeof( wchar_t ) + ( sizeof( int ) * 2 ) ) )
						{
							break;
						}*/

						total_read += read;

						// Prevent an infinite loop if a really really long entry causes us to jump back to the same point in the file.
						// If it's larger than our buffer, then the file is probably invalid/corrupt.
						if ( total_read == last_total )
						{
							break;
						}

						last_total = total_read;

						p = buf;
						offset = last_entry = 0;

						while ( offset < read )
						{
							site = NULL;
							username = NULL;
							password = NULL;
							w_username = NULL;
							w_password = NULL;

							category = NULL;
							download_directory = NULL;

							parts = 0;
							download_speed_limit = 0;

							comments = NULL;
							cookies = NULL;
							headers = NULL;
							data = NULL;

							hostname_length = 0;
							proxy_address_type = 0;
							proxy_hostname = NULL;
							proxy_ip_address = 0;
							proxy_port = 0;

							proxy_use_authentication = false;
							proxy_resolve_domain_names = false;

							proxy_auth_username = NULL;
							proxy_auth_password = NULL;
							w_proxy_auth_username = NULL;
							w_proxy_auth_password = NULL;

							//

							// Enable/Disable entry
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &enable, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							// Site
							int string_length = lstrlenW( ( wchar_t * )p ) + 1;

							offset += ( string_length * sizeof( wchar_t ) );
							if ( offset >= read ) { goto CLEANUP; }

							site = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
							_wmemcpy_s( site, string_length, p, string_length );
							*( site + ( string_length - 1 ) ) = 0;	// Sanity

							p += ( string_length * sizeof( wchar_t ) );

							// General Tab

							// Category
							if ( version >= 0x03 )
							{
								string_length = lstrlenW( ( wchar_t * )p ) + 1;

								offset += ( string_length * sizeof( wchar_t ) );
								if ( offset >= read ) { goto CLEANUP; }

								category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
								_wmemcpy_s( category, string_length, p, string_length );
								*( category + ( string_length - 1 ) ) = 0;	// Sanity

								p += ( string_length * sizeof( wchar_t ) );
							}

							// Use Download Directory
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &use_download_directory, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							if ( use_download_directory )
							{
								// Download Directory
								string_length = lstrlenW( ( wchar_t * )p ) + 1;

								offset += ( string_length * sizeof( wchar_t ) );
								if ( offset >= read ) { goto CLEANUP; }

								download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
								_wmemcpy_s( download_directory, string_length, p, string_length );
								*( download_directory + ( string_length - 1 ) ) = 0;	// Sanity

								p += ( string_length * sizeof( wchar_t ) );
							}

							// Use Parts
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &use_parts, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							if ( use_parts )
							{
								// Parts
								offset += sizeof( unsigned char );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &parts, sizeof( unsigned char ), p, sizeof( unsigned char ) );
								p += sizeof( unsigned char );
							}

							// Use Download Speed Limit
							offset += sizeof( bool );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &use_download_speed_limit, sizeof( bool ), p, sizeof( bool ) );
							p += sizeof( bool );

							if ( use_download_speed_limit )
							{
								// Download Speed Limit
								offset += sizeof( unsigned long long );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &download_speed_limit, sizeof( unsigned long long ), p, sizeof( unsigned long long ) );
								p += sizeof( unsigned long long );
							}

							// SSL Version
							offset += sizeof( char );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &ssl_version, sizeof( char ), p, sizeof( char ) );
							p += sizeof( char );

							// Username
							offset += sizeof( int );
							if ( offset >= read ) { goto CLEANUP; }

							// Length of the string - not including the NULL character.
							_memcpy_s( &string_length, sizeof( int ), p, sizeof( int ) );
							p += sizeof( int );

							offset += string_length;
							if ( offset >= read ) { goto CLEANUP; }
							if ( string_length > 0 )
							{
								// string_length does not contain the NULL character of the string.
								username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
								_memcpy_s( username, string_length, p, string_length );
								username[ string_length ] = 0; // Sanity;

								decode_cipher( username, string_length );

								w_username = UTF8StringToWideString( username, string_length + 1 );

								p += string_length;
							}

							// Password
							offset += sizeof( int );
							if ( offset >= read ) { goto CLEANUP; }

							// Length of the string - not including the NULL character.
							_memcpy_s( &string_length, sizeof( int ), p, sizeof( int ) );
							p += sizeof( int );

							offset += string_length;
							if ( offset >= read ) { goto CLEANUP; }
							if ( string_length > 0 )
							{
								// string_length does not contain the NULL character of the string.
								password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
								_memcpy_s( password, string_length, p, string_length );
								password[ string_length ] = 0; // Sanity;

								decode_cipher( password, string_length );

								w_password = UTF8StringToWideString( password, string_length + 1 );

								p += string_length;
							}

							// Download Operations
							if ( version >= 0x02 )
							{
								offset += sizeof( unsigned int );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &download_operations, sizeof( unsigned int ), p, sizeof( unsigned int ) );
								p += sizeof( unsigned int );
							}
							else
							{
								download_operations = 0;

								offset += sizeof( unsigned char );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &download_operations, sizeof( unsigned int ), p, sizeof( unsigned char ) );
								p += sizeof( unsigned char );
							}

							// Comments, Cookies, Headers, POST Data Tabs

							// Comments
							if ( version >= 0x03 )
							{
								string_length = lstrlenW( ( wchar_t * )p ) + 1;

								offset += ( string_length * sizeof( wchar_t ) );
								if ( offset >= read ) { goto CLEANUP; }

								// Let's not allocate an empty string.
								if ( string_length > 1 )
								{
									comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
									_memcpy_s( comments, string_length * sizeof( wchar_t ), p, string_length * sizeof( wchar_t ) );
									*( comments + ( string_length - 1 ) ) = 0;	// Sanity
								}

								p += ( string_length * sizeof( wchar_t ) );
							}

							// Cookies
							string_length = lstrlenA( ( char * )p ) + 1;

							offset += string_length;
							if ( offset >= read ) { goto CLEANUP; }

							// Let's not allocate an empty string.
							if ( string_length > 1 )
							{
								cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
								_memcpy_s( cookies, string_length, p, string_length );
								*( cookies + ( string_length - 1 ) ) = 0;	// Sanity
							}

							p += string_length;

							// Headers
							string_length = lstrlenA( ( char * )p ) + 1;

							offset += string_length;
							if ( offset >= read ) { goto CLEANUP; }

							// Let's not allocate an empty string.
							if ( string_length > 1 )
							{
								headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
								_memcpy_s( headers, string_length, p, string_length );
								*( headers + ( string_length - 1 ) ) = 0;	// Sanity
							}

							p += string_length;

							// Method
							offset += sizeof( unsigned char );
							if ( offset >= read ) { goto CLEANUP; }
							_memcpy_s( &method, sizeof( unsigned char ), p, sizeof( unsigned char ) );
							p += sizeof( unsigned char );

							if ( method == METHOD_POST )
							{
								// Data
								string_length = lstrlenA( ( char * )p ) + 1;

								offset += string_length;
								if ( offset >= read ) { goto CLEANUP; }

								// Let's not allocate an empty string.
								if ( string_length > 1 )
								{
									data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * string_length );
									_memcpy_s( data, string_length, p, string_length );
									*( data + ( string_length - 1 ) ) = 0;	// Sanity
								}

								p += string_length;
							}

							// Proxy Tab

							// Proxy Type
							offset += sizeof( unsigned char );
							if ( offset > read ) { goto CLEANUP; }
							_memcpy_s( &proxy_type, sizeof( unsigned char ), p, sizeof( unsigned char ) );
							p += sizeof( unsigned char );

							if ( proxy_type != 0 )
							{
								// Proxy Address Type
								offset += sizeof( unsigned char );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &proxy_address_type, sizeof( unsigned char ), p, sizeof( unsigned char ) );
								p += sizeof( unsigned char );

								if ( proxy_address_type == 0 )
								{
									// Proxy Hostname
									hostname_length = lstrlenW( ( wchar_t * )p );

									string_length = hostname_length + 1;

									offset += ( string_length * sizeof( wchar_t ) );
									if ( offset >= read ) { goto CLEANUP; }

									proxy_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
									_wmemcpy_s( proxy_hostname, string_length, p, string_length );
									*( proxy_hostname + ( string_length - 1 ) ) = 0;	// Sanity

									p += ( string_length * sizeof( wchar_t ) );
								}
								else// if ( proxy_address_type == 1 )
								{
									// Proxy IP Address
									offset += sizeof( unsigned long );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &proxy_ip_address, sizeof( unsigned long ), p, sizeof( unsigned long ) );
									p += sizeof( unsigned long );
								}

								// Proxy Port
								offset += sizeof( unsigned short );
								if ( offset >= read ) { goto CLEANUP; }
								_memcpy_s( &proxy_port, sizeof( unsigned short ), p, sizeof( unsigned short ) );
								p += sizeof( unsigned short );

								if ( proxy_type == 1 || proxy_type == 2 )	// HTTP and HTTPS
								{
									// Proxy Username
									offset += sizeof( unsigned short );
									if ( offset >= read ) { goto CLEANUP; }

									// Length of the string - not including the NULL character.
									_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
									p += sizeof( unsigned short );

									offset += string_length;
									if ( offset >= read ) { goto CLEANUP; }
									if ( string_length > 0 )
									{
										// string_length does not contain the NULL character of the string.
										proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_username, string_length, p, string_length );
										proxy_auth_username[ string_length ] = 0; // Sanity;

										decode_cipher( proxy_auth_username, string_length );

										w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

										p += string_length;
									}

									// Proxy Password
									offset += sizeof( unsigned short );
									if ( offset > read ) { goto CLEANUP; }

									// Length of the string - not including the NULL character.
									_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
									p += sizeof( unsigned short );

									offset += string_length;
									if ( offset > read ) { goto CLEANUP; }
									if ( string_length > 0 )
									{
										// string_length does not contain the NULL character of the string.
										proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_password, string_length, p, string_length );
										proxy_auth_password[ string_length ] = 0; // Sanity;

										decode_cipher( proxy_auth_password, string_length );

										w_proxy_auth_password = UTF8StringToWideString( proxy_auth_password, string_length + 1 );

										p += string_length;
									}
								}
								else if ( proxy_type == 3 )	// SOCKS v4
								{
									// Resolve Domain Names
									offset += sizeof( bool );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &proxy_resolve_domain_names, sizeof( bool ), p, sizeof( bool ) );
									p += sizeof( bool );

									// Proxy Username
									offset += sizeof( unsigned short );
									if ( offset > read ) { goto CLEANUP; }

									// Length of the string - not including the NULL character.
									_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
									p += sizeof( unsigned short );

									offset += string_length;
									if ( offset > read ) { goto CLEANUP; }
									if ( string_length > 0 )
									{
										// string_length does not contain the NULL character of the string.
										proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
										_memcpy_s( proxy_auth_username, string_length, p, string_length );
										proxy_auth_username[ string_length ] = 0; // Sanity;

										decode_cipher( proxy_auth_username, string_length );

										w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

										p += string_length;
									}
								}
								else if ( proxy_type == 4 )	// SOCKS v5
								{
									// Resolve Domain Names
									offset += sizeof( bool );
									if ( offset >= read ) { goto CLEANUP; }
									_memcpy_s( &proxy_resolve_domain_names, sizeof( bool ), p, sizeof( bool ) );
									p += sizeof( bool );

									// Use Authentication
									offset += sizeof( bool );
									if ( offset > read ) { goto CLEANUP; }
									_memcpy_s( &proxy_use_authentication, sizeof( bool ), p, sizeof( bool ) );
									p += sizeof( bool );

									if ( proxy_use_authentication )
									{
										//if ( offset == read ) { goto CLEANUP; }

										// Proxy Username
										offset += sizeof( unsigned short );
										if ( offset >= read ) { goto CLEANUP; }

										// Length of the string - not including the NULL character.
										_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
										p += sizeof( unsigned short );

										offset += string_length;
										if ( offset >= read ) { goto CLEANUP; }
										if ( string_length > 0 )
										{
											// string_length does not contain the NULL character of the string.
											proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
											_memcpy_s( proxy_auth_username, string_length, p, string_length );
											proxy_auth_username[ string_length ] = 0; // Sanity;

											decode_cipher( proxy_auth_username, string_length );

											w_proxy_auth_username = UTF8StringToWideString( proxy_auth_username, string_length + 1 );

											p += string_length;
										}

										// Proxy Password
										offset += sizeof( unsigned short );
										if ( offset > read ) { goto CLEANUP; }

										// Length of the string - not including the NULL character.
										_memcpy_s( &string_length, sizeof( unsigned short ), p, sizeof( unsigned short ) );
										p += sizeof( unsigned short );

										offset += string_length;
										if ( offset > read ) { goto CLEANUP; }
										if ( string_length > 0 )
										{
											// string_length does not contain the NULL character of the string.
											proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( string_length + 1 ) );
											_memcpy_s( proxy_auth_password, string_length, p, string_length );
											proxy_auth_password[ string_length ] = 0; // Sanity;

											decode_cipher( proxy_auth_password, string_length );

											w_proxy_auth_password = UTF8StringToWideString( proxy_auth_password, string_length + 1 );

											p += string_length;
										}
									}
								}
							}

							//

							last_entry = offset;	// This value is the ending offset of the last valid entry.

							unsigned int host_length = 0;
							unsigned int resource_length = 0;

							wchar_t *resource = NULL;

							SITE_INFO *si = ( SITE_INFO * )GlobalAlloc( GPTR, sizeof( SITE_INFO ) );
							if ( si != NULL )
							{
								ParseURL_W( site, NULL, si->protocol, &si->host, host_length, si->port, &resource, resource_length, NULL, NULL, NULL, NULL );
								GlobalFree( resource );

								si->enable = enable;

								si->w_host = site;

								//

								si->category = category;

								si->use_download_directory = use_download_directory;
								si->download_directory = download_directory;

								si->use_parts = use_parts;
								si->parts = parts;

								si->use_download_speed_limit = use_download_speed_limit;
								si->download_speed_limit = download_speed_limit;

								si->ssl_version = ssl_version;

								si->username = username;
								si->password = password;
								si->w_username = w_username;
								si->w_password = w_password;

								si->download_operations = download_operations;

								//

								si->comments = comments;
								si->utf8_cookies = cookies;
								si->utf8_headers = headers;
								si->method = method;
								si->utf8_data = data;

								//

								si->proxy_info.type = proxy_type;
								si->proxy_info.address_type = proxy_address_type;

								si->proxy_info.hostname = proxy_hostname;
								si->proxy_info.ip_address = proxy_ip_address;

								si->proxy_info.port = proxy_port;

								si->proxy_info.resolve_domain_names = proxy_resolve_domain_names;
								si->proxy_info.use_authentication = proxy_use_authentication;

								si->proxy_info.username = proxy_auth_username;
								si->proxy_info.password = proxy_auth_password;
								si->proxy_info.w_username = w_proxy_auth_username;
								si->proxy_info.w_password = w_proxy_auth_password;

								//

								if ( si->proxy_info.hostname != NULL )
								{
									if ( normaliz_state == NORMALIZ_STATE_RUNNING )
									{
										int punycode_length = _IdnToAscii( 0, si->proxy_info.hostname, hostname_length, NULL, 0 );

										if ( punycode_length > ( int )hostname_length )
										{
											si->proxy_info.punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
											_IdnToAscii( 0, si->proxy_info.hostname, hostname_length, si->proxy_info.punycode_hostname, punycode_length );
										}
									}
								}

								if ( dllrbt_insert( g_site_info, ( void * )si, ( void * )si ) != DLLRBT_STATUS_OK )
								{
									FreeSiteInfo( &si );
								}
							}
							else
							{
								GlobalFree( site );
								GlobalFree( username );
								GlobalFree( password );
								GlobalFree( w_username );
								GlobalFree( w_password );

								GlobalFree( category );
								GlobalFree( download_directory );

								GlobalFree( comments );
								GlobalFree( cookies );
								GlobalFree( headers );
								GlobalFree( data );

								GlobalFree( proxy_hostname );

								GlobalFree( proxy_auth_username );
								GlobalFree( proxy_auth_password );
								GlobalFree( w_proxy_auth_username );
								GlobalFree( w_proxy_auth_password );
							}

							continue;

			CLEANUP:
							GlobalFree( site );
							GlobalFree( username );
							GlobalFree( password );
							GlobalFree( w_username );
							GlobalFree( w_password );

							GlobalFree( category );
							GlobalFree( download_directory );

							GlobalFree( comments );
							GlobalFree( cookies );
							GlobalFree( headers );
							GlobalFree( data );

							GlobalFree( proxy_hostname );

							GlobalFree( proxy_auth_username );
							GlobalFree( proxy_auth_password );
							GlobalFree( w_proxy_auth_username );
							GlobalFree( w_proxy_auth_password );

							// Go back to the last valid entry.
							if ( total_read < fz )
							{
								total_read -= ( read - last_entry );
								SetFilePointer( hFile_read, total_read + 4, NULL, FILE_BEGIN );	// Offset past the magic identifier.
							}

							break;
						}
					}

					GlobalFree( buf );
				}
			}
			else
			{
				ret_status = -2;	// Bad file format.
			}
		}
		else
		{
			ret_status = -1;	// Can't open file for reading.
		}

		UnlockFileEx( hFile_read, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile_read );	
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for reading.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished reading site settings: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

char save_site_info()
{
	char ret_status = 0;
	char open_count = 0;

	_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\site_settings\0", 15 );
	//g_base_directory[ g_base_directory_length + 14 ] = 0;	// Sanity.

#ifdef ENABLE_LOGGING
	DWORD lfz = 0;
	WriteLog( LOG_INFO_MISC, "Saving site settings: %S", g_base_directory );
#endif

	HANDLE hFile = INVALID_HANDLE_VALUE;

RETRY_OPEN:

	hFile = CreateFile( g_base_directory, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		OVERLAPPED lfo;
		_memzero( &lfo, sizeof( OVERLAPPED ) );
		LockFileEx( hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &lfo );

		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_SITES, sizeof( char ) * 4 );	// Magic identifier for the site info.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( g_site_info );
		while ( node != NULL )
		{
			SITE_INFO *si = ( SITE_INFO * )node->val;
			if ( si != NULL )
			{
				// lstrlen is safe for NULL values.
				int url_length = ( lstrlenW( si->w_host ) + 1 ) * sizeof( wchar_t );

				int category_length = ( lstrlenW( si->category ) + 1 ) * sizeof( wchar_t );

				int download_directory_length = 0;
				if ( si->use_download_directory )
				{
					download_directory_length += ( lstrlenW( si->download_directory ) + 1 ) * sizeof( wchar_t );
				}

				int optional_extra_length = 0;
				if ( si->use_parts ) { optional_extra_length += sizeof( unsigned char ); };
				if ( si->use_download_speed_limit ) { optional_extra_length += sizeof( unsigned long long ); };

				int username_length = lstrlenA( si->username );
				int password_length = lstrlenA( si->password );

				int comments_length = ( lstrlenW( si->comments ) + 1 ) * sizeof( wchar_t );

				int cookies_length = lstrlenA( si->utf8_cookies ) + 1;
				int headers_length = lstrlenA( si->utf8_headers ) + 1;
				int data_length = 0;
				if ( si->method == METHOD_POST ) { data_length = lstrlenA( si->utf8_data ) + 1; }

				int proxy_username_length = 0;
				int proxy_password_length = 0;
				int proxy_address_length = 0;

				if ( si->proxy_info.type != 0 )
				{
					optional_extra_length += sizeof( unsigned char );

					if ( si->proxy_info.address_type == 0 )
					{
						proxy_address_length += ( lstrlenW( si->proxy_info.hostname ) + 1 ) * sizeof( wchar_t );
					}
					else// if ( si->proxy_address_type == 1 )
					{
						proxy_address_length += sizeof( unsigned long );
					}

					if ( si->proxy_info.type == 3 )	// SOCKS v4
					{
						optional_extra_length += sizeof( bool );
					}
					else if ( si->proxy_info.type == 4 )	// SOCKS v5
					{
						optional_extra_length += ( sizeof( bool ) * 2 );
					}

					if ( si->proxy_info.username != NULL )
					{
						proxy_username_length = lstrlenA( si->proxy_info.username );
						optional_extra_length += sizeof( unsigned short );
					}
					if ( si->proxy_info.password != NULL )
					{
						proxy_password_length = lstrlenA( si->proxy_info.password );
						optional_extra_length += sizeof( unsigned short );
					}
				}

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos +
								 url_length +
								 category_length +
								 download_directory_length +
								 username_length +
								 password_length +
								 comments_length +
								 cookies_length +
								 headers_length +
								 data_length +
								 proxy_address_length +
								 proxy_username_length +
								 proxy_password_length +
								 optional_extra_length +
							   ( sizeof( int ) * 3 ) +
							   ( sizeof( bool ) * 4 ) +
							   ( sizeof( char ) * 3 ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
#ifdef ENABLE_LOGGING
					lfz += write;
#endif
				}

				_memcpy_s( buf + pos, size - pos, &si->enable, sizeof( bool ) );
				pos += sizeof( bool );

				_memcpy_s( buf + pos, size - pos, si->w_host, url_length );
				pos += url_length;

				// General Tab

				_memcpy_s( buf + pos, size - pos, si->category, category_length );
				pos += category_length;

				_memcpy_s( buf + pos, size - pos, &si->use_download_directory, sizeof( bool ) );
				pos += sizeof( bool );

				if ( si->use_download_directory )
				{
					_memcpy_s( buf + pos, size - pos, si->download_directory, download_directory_length );
					pos += download_directory_length;
				}

				_memcpy_s( buf + pos, size - pos, &si->use_parts, sizeof( bool ) );
				pos += sizeof( bool );

				if ( si->use_parts )
				{
					_memcpy_s( buf + pos, size - pos, &si->parts, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );
				}

				_memcpy_s( buf + pos, size - pos, &si->use_download_speed_limit, sizeof( bool ) );
				pos += sizeof( bool );

				if ( si->use_download_speed_limit )
				{
					_memcpy_s( buf + pos, size - pos, &si->download_speed_limit, sizeof( unsigned long long ) );
					pos += sizeof( unsigned long long );
				}

				_memcpy_s( buf + pos, size - pos, &si->ssl_version, sizeof( char ) );
				pos += sizeof( char );

				if ( si->username != NULL )
				{
					_memcpy_s( buf + pos, size - pos, &username_length, sizeof( int ) );
					pos += sizeof( int );

					_memcpy_s( buf + pos, size - pos, si->username, username_length );
					encode_cipher( buf + pos, username_length );
					pos += username_length;
				}
				else
				{
					_memset( buf + pos, 0, sizeof( int ) );
					pos += sizeof( int );
				}

				if ( si->password != NULL )
				{
					_memcpy_s( buf + pos, size - pos, &password_length, sizeof( int ) );
					pos += sizeof( int );

					_memcpy_s( buf + pos, size - pos, si->password, password_length );
					encode_cipher( buf + pos, password_length );
					pos += password_length;
				}
				else
				{
					_memset( buf + pos, 0, sizeof( int ) );
					pos += sizeof( int );
				}

				_memcpy_s( buf + pos, size - pos, &si->download_operations, sizeof( unsigned int ) );
				pos += sizeof( unsigned int );

				// Cookies, Headers, POST Data Tabs

				_memcpy_s( buf + pos, size - pos, si->comments, comments_length );
				pos += comments_length;

				_memcpy_s( buf + pos, size - pos, si->utf8_cookies, cookies_length );
				pos += cookies_length;

				_memcpy_s( buf + pos, size - pos, si->utf8_headers, headers_length );
				pos += headers_length;

				_memcpy_s( buf + pos, size - pos, &si->method, sizeof( unsigned char ) );
				pos += sizeof( unsigned char );

				if ( si->method == METHOD_POST )
				{
					_memcpy_s( buf + pos, size - pos, si->utf8_data, data_length );
					pos += data_length;
				}

				// Proxy Tab

				_memcpy_s( buf + pos, size - pos, &si->proxy_info.type, sizeof( char ) );
				pos += sizeof( char );

				if ( si->proxy_info.type != 0 )
				{
					_memcpy_s( buf + pos, size - pos, &si->proxy_info.address_type, sizeof( unsigned char ) );
					pos += sizeof( unsigned char );

					if ( si->proxy_info.address_type == 0 )
					{
						_memcpy_s( buf + pos, size - pos, si->proxy_info.hostname, proxy_address_length );
						pos += proxy_address_length;
					}
					else// if ( si->proxy_address_type == 1 )
					{
						_memcpy_s( buf + pos, size - pos, &si->proxy_info.ip_address, sizeof( unsigned long ) );
						pos += sizeof( unsigned long );
					}

					_memcpy_s( buf + pos, size - pos, &si->proxy_info.port, sizeof( unsigned short ) );
					pos += sizeof( unsigned short );

					if ( si->proxy_info.type == 1 || si->proxy_info.type == 2 )	// HTTP and HTTPS
					{
						if ( si->proxy_info.username != NULL )
						{
							_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );

							_memcpy_s( buf + pos, size - pos, si->proxy_info.username, proxy_username_length );
							encode_cipher( buf + pos, proxy_username_length );
							pos += proxy_username_length;
						}
						else
						{
							_memset( buf + pos, 0, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );
						}

						if ( si->proxy_info.password != NULL )
						{
							_memcpy_s( buf + pos, size - pos, &proxy_password_length, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );

							_memcpy_s( buf + pos, size - pos, si->proxy_info.password, proxy_password_length );
							encode_cipher( buf + pos, proxy_password_length );
							pos += proxy_password_length;
						}
						else
						{
							_memset( buf + pos, 0, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );
						}
					}
					else if ( si->proxy_info.type == 3 )	// SOCKS v4
					{
						_memcpy_s( buf + pos, size - pos, &si->proxy_info.resolve_domain_names, sizeof( bool ) );
						pos += sizeof( bool );

						if ( si->proxy_info.username != NULL )
						{
							_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );

							_memcpy_s( buf + pos, size - pos, si->proxy_info.username, proxy_username_length );
							encode_cipher( buf + pos, proxy_username_length );
							pos += proxy_username_length;
						}
						else
						{
							_memset( buf + pos, 0, sizeof( unsigned short ) );
							pos += sizeof( unsigned short );
						}
					}
					else if ( si->proxy_info.type == 4 )	// SOCKS v5
					{
						_memcpy_s( buf + pos, size - pos, &si->proxy_info.resolve_domain_names, sizeof( bool ) );
						pos += sizeof( bool );

						_memcpy_s( buf + pos, size - pos, &si->proxy_info.use_authentication, sizeof( bool ) );
						pos += sizeof( bool );

						if ( si->proxy_info.use_authentication )
						{
							if ( si->proxy_info.username != NULL )
							{
								_memcpy_s( buf + pos, size - pos, &proxy_username_length, sizeof( unsigned short ) );
								pos += sizeof( unsigned short );

								_memcpy_s( buf + pos, size - pos, si->proxy_info.username, proxy_username_length );
								encode_cipher( buf + pos, proxy_username_length );
								pos += proxy_username_length;
							}
							else
							{
								_memset( buf + pos, 0, sizeof( unsigned short ) );
								pos += sizeof( unsigned short );
							}

							if ( si->proxy_info.password != NULL )
							{
								_memcpy_s( buf + pos, size - pos, &proxy_password_length, sizeof( unsigned short ) );
								pos += sizeof( unsigned short );

								_memcpy_s( buf + pos, size - pos, si->proxy_info.password, proxy_password_length );
								encode_cipher( buf + pos, proxy_password_length );
								pos += proxy_password_length;
							}
							else
							{
								_memset( buf + pos, 0, sizeof( unsigned short ) );
								pos += sizeof( unsigned short );
							}
						}
					}
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
#ifdef ENABLE_LOGGING
			lfz += write;
#endif
		}

		GlobalFree( buf );

		SetEndOfFile( hFile );

		UnlockFileEx( hFile, 0, MAXDWORD, MAXDWORD, &lfo );

		CloseHandle( hFile );
	}
	else
	{
		if ( GetLastError() == ERROR_SHARING_VIOLATION && ++open_count <= 5 )
		{
			Sleep( 200 );
			goto RETRY_OPEN;
		}

		ret_status = -1;	// Can't open file for writing.
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( ret_status == 0 ? LOG_INFO_MISC : LOG_ERROR ), "Finished saving site settings: %d | %lu bytes", ret_status, lfz );
#endif

	return ret_status;
}

THREAD_RETURN load_site_list( void * /*pArguments*/ )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM | LVIF_TEXT;

	node_type *node = dllrbt_get_head( g_site_info );
	while ( node != NULL )
	{
		SITE_INFO *si = ( SITE_INFO * )node->val;

		if ( si != NULL )
		{
			lvi.iItem = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETITEMCOUNT, 0, 0 );
			lvi.lParam = ( LPARAM )si;
			lvi.pszText = si->w_host;
			_SendMessageW( g_hWnd_site_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
		}

		node = node->next;
	}

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}

THREAD_RETURN handle_site_list( void *pArguments )
{
	SITE_UPDATE_INFO *sui = ( SITE_UPDATE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( sui != NULL )
	{
		if ( sui->update_type == 0 && sui->si != NULL )	// Add
		{
			SITE_INFO *si = sui->si;

#ifdef ENABLE_LOGGING
			WriteLog( LOG_INFO_ACTION, "Adding %S to site manager", si->w_host );
#endif

			unsigned char fail_type = 0;

			wchar_t *host = NULL;
			wchar_t *resource = NULL;

			unsigned int host_length = 0;
			unsigned int resource_length = 0;

			ParseURL_W( si->w_host, NULL, si->protocol, &host, host_length, si->port, &resource, resource_length, NULL, NULL, NULL, NULL );

			if ( si->protocol == PROTOCOL_HTTP ||
				 si->protocol == PROTOCOL_HTTPS ||
				 si->protocol == PROTOCOL_FTP ||
				 si->protocol == PROTOCOL_FTPS ||
				 si->protocol == PROTOCOL_FTPES ||
				 si->protocol == PROTOCOL_SFTP )
			{
				if ( si->protocol == PROTOCOL_HTTP )
				{
					host_length += 7;	// http://
				}
				else if ( si->protocol == PROTOCOL_HTTPS )
				{
					host_length += 8;	// https://
				}
				else if ( si->protocol == PROTOCOL_FTP )
				{
					host_length += 6;	// ftp://
				}
				else if ( si->protocol == PROTOCOL_FTPS )
				{
					host_length += 7;	// ftps://
				}
				else if ( si->protocol == PROTOCOL_FTPES )
				{
					host_length += 8;	// ftpes://
				}
				else if ( si->protocol == PROTOCOL_SFTP )
				{
					host_length += 7;	// sftp://
				}

				// See if there's a resource at the end of our host. We don't want it.
				// Skip the http(s):// and host.
				wchar_t *end = _StrChrW( si->w_host + host_length, L'/' );
				if ( end != NULL )
				{
					*end = 0;

					host_length = ( unsigned int )( end - si->w_host ) + 1;

					wchar_t *w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * host_length );
					_wmemcpy_s( w_host, host_length, si->w_host, host_length );

					GlobalFree( si->w_host );
					si->w_host = w_host;
				}

				GlobalFree( resource );

				int string_length = 0;	// Temporary value.

				si->host = host;

				si->username = WideStringToUTF8String( si->w_username, &string_length );
				si->password = WideStringToUTF8String( si->w_password, &string_length );

				si->proxy_info.username = WideStringToUTF8String( si->proxy_info.w_username, &string_length );
				si->proxy_info.password = WideStringToUTF8String( si->proxy_info.w_password, &string_length );

				if ( sui->old_si != NULL )
				{
					// Find the site info
					dllrbt_iterator *new_itr = dllrbt_find( g_site_info, ( void * )si, false );

					// Find the site info
					dllrbt_iterator *old_itr = dllrbt_find( g_site_info, ( void * )sui->old_si, false );

					if ( new_itr == NULL || old_itr == new_itr )
					{
						dllrbt_remove( g_site_info, old_itr );
					}
				}

				if ( dllrbt_insert( g_site_info, ( void * )si, ( void * )si ) != DLLRBT_STATUS_OK )
				{
					fail_type = 1;	// Already exits.
				}
				else
				{
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM | LVIF_TEXT;

					if ( sui->old_si != NULL )
					{
						lvi.iItem = sui->index;
						lvi.lParam = ( LPARAM )si;
						lvi.pszText = si->w_host;
						_SendMessageW( g_hWnd_site_list, LVM_SETITEM, 0, ( LPARAM )&lvi );

						FreeSiteInfo( &sui->old_si );
					}
					else
					{
						lvi.iItem = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETITEMCOUNT, 0, 0 );
						lvi.lParam = ( LPARAM )si;
						lvi.pszText = si->w_host;
						_SendMessageW( g_hWnd_site_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

						_SendMessageW( g_hWnd_site_manager, WM_PROPAGATE, 4, ( LPARAM )sui );	// Update selected host.
					}

					site_list_changed = true;
				}
			}
			else
			{
				fail_type = 2;	// Bad protocol.
			}

			if ( fail_type != 0 )
			{
				FreeSiteInfo( &si );

				_SendNotifyMessageW( g_hWnd_site_manager, WM_PROPAGATE, fail_type, 0 );
			}
		}
		else if ( sui->update_type == 1 )	// Remove
		{
			// Prevent the listviews from drawing while freeing lParam values.
			skip_site_list_draw = true;

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			int item_count = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			int *index_array = NULL;

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				_SendMessageW( g_hWnd_site_list, LVM_ENSUREVISIBLE, 0, FALSE );

				index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );
				if ( index_array != NULL )
				{
					lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

					_EnableWindow( g_hWnd_site_list, FALSE );	// Prevent any interaction with the listview while we're processing.

					// Create an index list of selected items (in reverse order).
					for ( int i = 0; i < sel_count; ++i )
					{
						lvi.iItem = index_array[ sel_count - 1 - i ] = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
					}

					_EnableWindow( g_hWnd_site_list, TRUE );	// Allow the listview to be interactive.
				}

				item_count = sel_count;
			}

			if ( handle_all || index_array != NULL )
			{
				// Go through each item, and free their lParam values.
				for ( int i = 0; i < item_count; ++i )
				{
					// Stop processing and exit the thread.
					if ( kill_worker_thread_flag )
					{
						break;
					}

					if ( handle_all )
					{
						lvi.iItem = i;
					}
					else
					{
						lvi.iItem = index_array[ i ];
					}

					_SendMessageW( g_hWnd_site_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

					SITE_INFO *si = ( SITE_INFO * )lvi.lParam;

					if ( !handle_all )
					{
						_SendMessageW( g_hWnd_site_list, LVM_DELETEITEM, index_array[ i ], 0 );
					}
					else if ( i >= ( item_count - 1 ) )
					{
						_SendMessageW( g_hWnd_site_list, LVM_DELETEALLITEMS, 0, 0 );
					}

					if ( si != NULL )
					{
#ifdef ENABLE_LOGGING
						WriteLog( LOG_INFO_ACTION, "Removing %S from site manager", si->w_host );
#endif
						// Find the site info
						dllrbt_iterator *itr = dllrbt_find( g_site_info, ( void * )si, false );
						if ( itr != NULL )
						{
							dllrbt_remove( g_site_info, itr );
						}

						FreeSiteInfo( &si );
					}
				}
			}

			_SendMessageW( g_hWnd_site_manager, WM_PROPAGATE, 3, 0 );	// Disable remove button.

			site_list_changed = true;

			skip_site_list_draw = false;

			if ( index_array != NULL )
			{
				GlobalFree( index_array );
			}
		}
		else if ( sui->update_type == 2 )	// Enable/Disable
		{
			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;
			lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

			int item_count = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			bool copy_all = false;
			if ( item_count == sel_count )
			{
				copy_all = true;
			}
			else
			{
				item_count = sel_count;
			}

			// Go through each item, and copy the URL.
			for ( int i = 0; i < item_count; ++i )
			{
				// Stop processing and exit the thread.
				if ( kill_worker_thread_flag )
				{
					break;
				}

				if ( copy_all )
				{
					lvi.iItem = i;
				}
				else
				{
					lvi.iItem = ( int )_SendMessageW( g_hWnd_site_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
				}

				_SendMessageW( g_hWnd_site_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				SITE_INFO *si = ( SITE_INFO * )lvi.lParam;

				if ( si != NULL )
				{
#ifdef ENABLE_LOGGING
					WriteLog( LOG_INFO_ACTION, "%s %S in site manager", ( sui->enable ? "Enabling" : "Disabling" ), si->w_host );
#endif
					si->enable = sui->enable;
				}
			}

			site_list_changed = true;
		}

		GlobalFree( sui );
	}

	_InvalidateRect( g_hWnd_site_list, NULL, FALSE );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	//return 0;
}
