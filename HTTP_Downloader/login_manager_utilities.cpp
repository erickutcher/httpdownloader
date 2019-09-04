/*
	HTTP Downloader can download files through HTTP(S) and FTP(S) connections.
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

#include "login_manager_utilities.h"

#include "utilities.h"
#include "file_operations.h"

#include "http_parsing.h"
#include "connection.h"

bool login_list_changed = false;

int GetDomainParts( wchar_t *site, wchar_t *offsets[ 128 ] )
{
	int count = 0;
	wchar_t *ptr = site;
	wchar_t *ptr_s = ptr;

	while ( ptr != NULL && count < 127 )
	{
		if ( *ptr == L'.' )
		{
			offsets[ count++ ] = ptr_s;

			ptr_s = ptr + 1;
		}
		else if ( *ptr == NULL )
		{
			offsets[ count++ ] = ptr_s;

			break;
		}

		++ptr;
	}

	if ( ptr != NULL )
	{
		offsets[ count ] = ptr;	// End of string.
	}

	return count;
}

int dllrbt_compare_login_info( void *a, void *b )
{
	LOGIN_INFO *a1 = ( LOGIN_INFO * )a;
	LOGIN_INFO *b1 = ( LOGIN_INFO * )b;

	int ret;

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
							( ( ( end2 - start2 ) == 2 ) && *start2 == L'*' && *( start2 + 1 ) == L'.' ) ) )
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

char read_login_info()
{
	char ret_status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\http_downloader_logins\0", 24 );
	base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

	HANDLE hFile_read = CreateFile( base_directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_read != INVALID_HANDLE_VALUE )
	{
		DWORD read = 0, total_read = 0, offset = 0, last_entry = 0, last_total = 0;

		char *p = NULL;

		wchar_t				*site;
		char				*username;
		char				*password;
		wchar_t				*w_username;
		wchar_t				*w_password;

		char magic_identifier[ 4 ];
		ReadFile( hFile_read, magic_identifier, sizeof( char ) * 4, &read, NULL );
		if ( read == 4 && _memcmp( magic_identifier, MAGIC_ID_LOGINS, 4 ) == 0 )
		{
			DWORD fz = GetFileSize( hFile_read, NULL ) - 4;

			char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( 524288 + 1 ) );	// 512 KB buffer.

			while ( total_read < fz )
			{
				ReadFile( hFile_read, buf, sizeof( char ) * 524288, &read, NULL );

				buf[ read ] = 0;	// Guarantee a NULL terminated buffer.

				// Make sure that we have at least part of the entry. This is the minimum size an entry could be.
				// Include 3 wide NULL strings and 3 char NULL strings.
				// Include 2 ints for username and password lengths.
				// Include 1 unsigned char for range info.
				if ( read < ( sizeof( wchar_t ) + ( sizeof( int ) * 2 ) ) )
				{
					break;
				}

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

					// Site
					int string_length = lstrlenW( ( wchar_t * )p ) + 1;

					offset += ( string_length * sizeof( wchar_t ) );
					if ( offset >= read ) { goto CLEANUP; }

					site = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * string_length );
					_wmemcpy_s( site, string_length, p, string_length );
					*( site + ( string_length - 1 ) ) = 0;	// Sanity

					p += ( string_length * sizeof( wchar_t ) );

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
					if ( offset > read ) { goto CLEANUP; }

					// Length of the string - not including the NULL character.
					_memcpy_s( &string_length, sizeof( int ), p, sizeof( int ) );
					p += sizeof( int );

					offset += string_length;
					if ( offset > read ) { goto CLEANUP; }
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

					last_entry = offset;	// This value is the ending offset of the last valid entry.

					unsigned int host_length = 0;
					unsigned int resource_length = 0;

					wchar_t *resource = NULL;

					LOGIN_INFO *li = ( LOGIN_INFO * )GlobalAlloc( GPTR, sizeof( LOGIN_INFO ) );

					ParseURL_W( site, NULL, li->protocol, &li->host, host_length, li->port, &resource, resource_length, NULL, NULL, NULL, NULL );
					GlobalFree( resource );

					li->username = username;
					li->password = password;
					li->w_host = site;
					li->w_username = w_username;
					li->w_password = w_password;

					if ( dllrbt_insert( g_login_info, ( void * )li, ( void * )li ) != DLLRBT_STATUS_OK )
					{
						GlobalFree( li->w_host );
						GlobalFree( li->w_username );
						GlobalFree( li->w_password );
						GlobalFree( li->host );
						GlobalFree( li->username );
						GlobalFree( li->password );
						GlobalFree( li );
					}

					continue;

	CLEANUP:
					GlobalFree( site );
					GlobalFree( username );
					GlobalFree( password );
					GlobalFree( w_username );
					GlobalFree( w_password );

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
		else
		{
			ret_status = -2;	// Bad file format.
		}

		CloseHandle( hFile_read );	
	}
	else
	{
		ret_status = -1;	// Can't open file for reading.
	}

	return ret_status;
}

char save_login_info()
{
	char ret_status = 0;

	_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\http_downloader_logins\0", 24 );
	base_directory[ base_directory_length + 23 ] = 0;	// Sanity.

	HANDLE hFile = CreateFile( base_directory, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		//int size = ( 32768 + 1 );
		int size = ( 524288 + 1 );
		int pos = 0;
		DWORD write = 0;

		char *buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * size );

		_memcpy_s( buf + pos, size - pos, MAGIC_ID_LOGINS, sizeof( char ) * 4 );	// Magic identifier for the call log history.
		pos += ( sizeof( char ) * 4 );

		node_type *node = dllrbt_get_head( g_login_info );
		while ( node != NULL )
		{
			LOGIN_INFO *li = ( LOGIN_INFO * )node->val;
			if ( li != NULL )
			{
				// lstrlen is safe for NULL values.
				int url_length = ( lstrlenW( li->w_host ) + 1 ) * sizeof( wchar_t );

				int username_length = lstrlenA( li->username );
				int password_length = lstrlenA( li->password );

				// See if the next entry can fit in the buffer. If it can't, then we dump the buffer.
				if ( ( signed )( pos + url_length + username_length + password_length + ( sizeof( int ) * 2 ) ) > size )
				{
					// Dump the buffer.
					WriteFile( hFile, buf, pos, &write, NULL );
					pos = 0;
				}

				_memcpy_s( buf + pos, size - pos, li->w_host, url_length );
				pos += url_length;

				if ( li->username != NULL )
				{
					_memcpy_s( buf + pos, size - pos, &username_length, sizeof( int ) );
					pos += sizeof( int );

					_memcpy_s( buf + pos, size - pos, li->username, username_length );
					encode_cipher( buf + pos, username_length );
					pos += username_length;
				}
				else
				{
					_memset( buf + pos, 0, sizeof( int ) );
					pos += sizeof( int );
				}

				if ( li->password != NULL )
				{
					_memcpy_s( buf + pos, size - pos, &password_length, sizeof( int ) );
					pos += sizeof( int );

					_memcpy_s( buf + pos, size - pos, li->password, password_length );
					encode_cipher( buf + pos, password_length );
					pos += password_length;
				}
				else
				{
					_memset( buf + pos, 0, sizeof( int ) );
					pos += sizeof( int );
				}
			}

			node = node->next;
		}

		// If there's anything remaining in the buffer, then write it to the file.
		if ( pos > 0 )
		{
			WriteFile( hFile, buf, pos, &write, NULL );
		}

		GlobalFree( buf );

		CloseHandle( hFile );
	}
	else
	{
		ret_status = -1;	// Can't open file for writing.
	}

	return ret_status;
}

THREAD_RETURN load_login_list( void *pArguments )
{
	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	LVITEM lvi;
	_memzero( &lvi, sizeof( LVITEM ) );
	lvi.mask = LVIF_PARAM | LVIF_TEXT;

	node_type *node = dllrbt_get_head( g_login_info );
	while ( node != NULL )
	{
		LOGIN_INFO *li = ( LOGIN_INFO * )node->val;

		if ( li != NULL )
		{
			lvi.iItem = ( int )_SendMessageW( g_hWnd_login_list, LVM_GETITEMCOUNT, 0, 0 );
			lvi.lParam = ( LPARAM )li;
			lvi.pszText = li->w_host;
			_SendMessageW( g_hWnd_login_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );
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
	return 0;
}

THREAD_RETURN handle_login_list( void *pArguments )
{
	LOGIN_UPDATE_INFO *lui = ( LOGIN_UPDATE_INFO * )pArguments;

	// This will block every other thread from entering until the first thread is complete.
	EnterCriticalSection( &worker_cs );

	in_worker_thread = true;

	if ( lui != NULL )
	{
		if ( lui->update_type == 0 && lui->li != NULL )	// Add
		{
			LOGIN_INFO *li = lui->li;

			unsigned char fail_type = 0;

			wchar_t *host = NULL;
			wchar_t *resource = NULL;

			unsigned int host_length = 0;
			unsigned int resource_length = 0;

			ParseURL_W( li->w_host, NULL, li->protocol, &host, host_length, li->port, &resource, resource_length, NULL, NULL, NULL, NULL );

			if ( li->protocol == PROTOCOL_HTTP ||
				 li->protocol == PROTOCOL_HTTPS ||
				 li->protocol == PROTOCOL_FTP ||
				 li->protocol == PROTOCOL_FTPS ||
				 li->protocol == PROTOCOL_FTPES )
			{
				if ( li->protocol == PROTOCOL_HTTP )
				{
					host_length += 7;	// http://
				}
				else if ( li->protocol == PROTOCOL_HTTPS )
				{
					host_length += 8;	// https://
				}
				else if ( li->protocol == PROTOCOL_FTP )
				{
					host_length += 6;	// ftp://
				}
				else if ( li->protocol == PROTOCOL_FTPS )
				{
					host_length += 7;	// ftps://
				}
				else if ( li->protocol == PROTOCOL_FTPES )
				{
					host_length += 8;	// ftpes://
				}

				// See if there's a resource at the end of our host. We don't want it.
				// Skip the http(s):// and host.
				wchar_t *end = _StrChrW( li->w_host + host_length, L'/' );
				if ( end != NULL )
				{
					*end = 0;

					host_length = ( unsigned int )( end - li->w_host ) + 1;

					wchar_t *w_host = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * host_length );
					_wmemcpy_s( w_host, host_length, li->w_host, host_length );

					GlobalFree( li->w_host );
					li->w_host = w_host;
				}

				GlobalFree( resource );

				int string_length = 0;	// Temporary value.

				li->host = host;

				li->username = WideStringToUTF8String( li->w_username, &string_length );
				li->password = WideStringToUTF8String( li->w_password, &string_length );

				if ( dllrbt_insert( g_login_info, ( void * )li, ( void * )li ) != DLLRBT_STATUS_OK )
				{
					fail_type = 1;	// Already exits.
				}
				else
				{
					LVITEM lvi;
					_memzero( &lvi, sizeof( LVITEM ) );
					lvi.mask = LVIF_PARAM | LVIF_TEXT;
					lvi.iItem = ( int )_SendMessageW( g_hWnd_login_list, LVM_GETITEMCOUNT, 0, 0 );
					lvi.lParam = ( LPARAM )li;
					lvi.pszText = li->w_host;
					_SendMessageW( g_hWnd_login_list, LVM_INSERTITEM, 0, ( LPARAM )&lvi );

					login_list_changed = true;

					_SendMessageW( g_hWnd_login_manager, WM_PROPAGATE, 0, 0 );	// Clear entry.
				}
			}
			else
			{
				fail_type = 2;	// Bad protocol.
			}

			if ( fail_type != 0 )
			{
				GlobalFree( li->w_host );
				GlobalFree( li->w_username );
				GlobalFree( li->w_password );
				GlobalFree( li->host );
				GlobalFree( li->username );
				GlobalFree( li->password );
				GlobalFree( li );

				_SendNotifyMessageW( g_hWnd_login_manager, WM_PROPAGATE, fail_type, 0 );
			}
		}
		else if ( lui->update_type == 1 )	// Remove
		{
			// Prevent the listviews from drawing while freeing lParam values.
			skip_login_list_draw = true;

			LVITEM lvi;
			_memzero( &lvi, sizeof( LVITEM ) );
			lvi.mask = LVIF_PARAM;

			int item_count = ( int )_SendMessageW( g_hWnd_login_list, LVM_GETITEMCOUNT, 0, 0 );
			int sel_count = ( int )_SendMessageW( g_hWnd_login_list, LVM_GETSELECTEDCOUNT, 0, 0 );

			int *index_array = NULL;

			bool handle_all = false;
			if ( item_count == sel_count )
			{
				handle_all = true;
			}
			else
			{
				_SendMessageW( g_hWnd_login_list, LVM_ENSUREVISIBLE, 0, FALSE );

				index_array = ( int * )GlobalAlloc( GMEM_FIXED, sizeof( int ) * sel_count );

				lvi.iItem = -1;	// Set this to -1 so that the LVM_GETNEXTITEM call can go through the list correctly.

				_EnableWindow( g_hWnd_login_list, FALSE );	// Prevent any interaction with the listview while we're processing.

				// Create an index list of selected items (in reverse order).
				for ( int i = 0; i < sel_count; ++i )
				{
					lvi.iItem = index_array[ sel_count - 1 - i ] = ( int )_SendMessageW( g_hWnd_login_list, LVM_GETNEXTITEM, lvi.iItem, LVNI_SELECTED );
				}

				_EnableWindow( g_hWnd_login_list, TRUE );	// Allow the listview to be interactive.

				item_count = sel_count;
			}

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

				_SendMessageW( g_hWnd_login_list, LVM_GETITEM, 0, ( LPARAM )&lvi );

				LOGIN_INFO *li = ( LOGIN_INFO * )lvi.lParam;

				if ( !handle_all )
				{
					_SendMessageW( g_hWnd_login_list, LVM_DELETEITEM, index_array[ i ], 0 );
				}
				else if ( i >= ( item_count - 1 ) )
				{
					_SendMessageW( g_hWnd_login_list, LVM_DELETEALLITEMS, 0, 0 );
				}

				if ( li != NULL )
				{
					// Find the login info
					dllrbt_iterator *itr = dllrbt_find( g_login_info, ( void * )li, false );
					if ( itr != NULL )
					{
						dllrbt_remove( g_login_info, itr );
					}

					GlobalFree( li->w_host );
					GlobalFree( li->w_username );
					GlobalFree( li->w_password );
					GlobalFree( li->host );
					GlobalFree( li->username );
					GlobalFree( li->password );
					GlobalFree( li );
				}
			}

			_SendMessageW( g_hWnd_login_manager, WM_PROPAGATE, 3, 0 );	// Disable remove button.

			login_list_changed = true;

			skip_login_list_draw = false;
		}

		GlobalFree( lui );
	}

	_InvalidateRect( g_hWnd_login_list, NULL, FALSE );

	// Release the semaphore if we're killing the thread.
	if ( worker_semaphore != NULL )
	{
		ReleaseSemaphore( worker_semaphore, 1, NULL );
	}

	in_worker_thread = false;

	// We're done. Let other threads continue.
	LeaveCriticalSection( &worker_cs );

	_ExitThread( 0 );
	return 0;
}
