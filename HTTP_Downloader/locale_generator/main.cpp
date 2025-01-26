#include <windows.h>
#include <mlang.h>
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>

#define is_digit_w( c ) ( c - L'0' + 0U <= 9U )

typedef int ( WINAPI *pGetUserDefaultLocaleName )( LPWSTR lpLocaleName, int cchLocaleName );

wchar_t from_hex_w( wchar_t c )
{
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

bool is_hex_w( wchar_t c )
{
	return ( is_digit_w( c ) || ( c - L'a' + 0U < 6U ) || ( c - L'A' + 0U < 6U ) );
}

wchar_t *decode_w( wchar_t *str, unsigned int str_len, unsigned int *dec_len )
{
	wchar_t *pstr = str;
	wchar_t *pbuf = str;

	while ( pstr < ( str + str_len ) )
	{
		if ( *pstr == L'\r' )
		{
			*pbuf++ = 0;
			pstr++;
		}
		else if ( *pstr == L'\n' )
		{
			*pbuf++ = 0;
		}
		else if ( *pstr == L'\\' )
		{
			if ( ( ( pstr + 1 ) <= ( str + str_len ) ) )
			{
				if ( pstr[ 1 ] != NULL && pstr[ 1 ] == L't' )
				{
					*pbuf++ = L'\t';
					pstr++;
				}
				else if ( pstr[ 1 ] != NULL && pstr[ 1 ] == L'r' )
				{
					*pbuf++ = L'\r';
					pstr++;
				}
				else if ( pstr[ 1 ] != NULL && pstr[ 1 ] == L'n' )
				{
					*pbuf++ = L'\n';
					pstr++;
				}
				else if ( ( pstr[ 1 ] != NULL && pstr[ 1 ] == L'x' ) &&
						  ( pstr[ 2 ] != NULL && is_hex_w( pstr[ 2 ] ) ) &&
						  ( pstr[ 3 ] != NULL && is_hex_w( pstr[ 3 ] ) ) &&
						  ( pstr[ 4 ] != NULL && is_hex_w( pstr[ 4 ] ) ) &&
						  ( pstr[ 5 ] != NULL && is_hex_w( pstr[ 5 ] ) ) )
				{
					*pbuf++ = from_hex_w( pstr[ 2 ] ) << 12 |
							  from_hex_w( pstr[ 3 ] ) << 8 |
							  from_hex_w( pstr[ 4 ] ) << 4 |
							  from_hex_w( pstr[ 5 ] );
					pstr += 5;
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
		else
		{
			*pbuf++ = *pstr;
		}

		pstr++;
	}

	*pbuf = L'\0';

	if ( dec_len != NULL )
	{
		*dec_len = pbuf - str;
	}

	return str;
}

void OpenLocaleFile( wchar_t *file_path )
{
	HANDLE hFile_input = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_input != INVALID_HANDLE_VALUE )
	{
		unsigned char *locale_buf = NULL;
		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_input, NULL );

		if ( fz > sizeof( wchar_t ) && fz < 131072 )
		{
			locale_buf = ( unsigned char * )GlobalAlloc( GMEM_FIXED, sizeof( unsigned char ) * fz + 2 );
			if ( locale_buf != NULL )
			{
				// Look for a UTF-16 BOM (little endian or big endian) and ignore it.
				BOOL bRet = ReadFile( hFile_input, locale_buf, sizeof( unsigned char ) * 2, &read, NULL );
				if ( bRet != FALSE )
				{
					if ( read == 2 && ( ( locale_buf[ 0 ] == 0xFF && locale_buf[ 1 ] == 0xFE ) ||
										( locale_buf[ 0 ] == 0xFE && locale_buf[ 1 ] == 0xFF ) ) )
					{
						read = 0;
						fz -= 2;
					}
					bRet = ReadFile( hFile_input, locale_buf + read, ( sizeof( unsigned char ) * fz ) - read, &read, NULL );
					if ( bRet != FALSE )
					{
						// Guarantee a NULL terminated (wide character) buffer.
						locale_buf[ fz ] = 0;
						locale_buf[ fz + 1 ] = 0;
					}
					else
					{
						fz = 0;
					}
				}
				else
				{
					fz = 0;
				}
			}
		}

		CloseHandle( hFile_input );

		if ( fz > sizeof( wchar_t ) )
		{
			wchar_t *ptr = ( wchar_t * )locale_buf;
			wchar_t *last_ptr = ptr;
			wchar_t *ptr_end = ( wchar_t * )( ( unsigned char * )( locale_buf + fz ) );

			wchar_t *filename = PathFindFileNameW( file_path );
			wchar_t string_list[ MAX_PATH ];
			wsprintfW( string_list, L"string_list_%s.txt", filename );

			HANDLE hFile_string_list = CreateFile( string_list, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile_string_list != INVALID_HANDLE_VALUE )
			{
				DWORD write = 0;

				WriteFile( hFile_string_list, "\xEF\xBB\xBF", 3, &write, NULL );

				while ( ++ptr < ptr_end )
				{
					if ( *ptr == NULL )
					{
						int utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), NULL, 0, NULL, NULL );
						char *utf8_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * utf8_string_length );
						utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), utf8_val, utf8_string_length, NULL, NULL );

						WriteFile( hFile_string_list, utf8_val, utf8_string_length, &write, NULL );

						GlobalFree( utf8_val );

						++ptr;
						if ( ptr + 1 < ptr_end )
						{
							WriteFile( hFile_string_list, "\r\n", 2, &write, NULL );
						}

						last_ptr = ptr;
					}
					else if ( *ptr == L'\t' )
					{
						if ( ptr - last_ptr > 0 )
						{
							int utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), NULL, 0, NULL, NULL );
							char *utf8_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * utf8_string_length );
							utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), utf8_val, utf8_string_length, NULL, NULL );

							WriteFile( hFile_string_list, utf8_val, utf8_string_length, &write, NULL );

							GlobalFree( utf8_val );
						}

						WriteFile( hFile_string_list, "\\t", 2, &write, NULL );

						last_ptr = ptr + 1;
					}
					else if ( *ptr == L'\r' )
					{
						if ( ptr - last_ptr > 0 )
						{
							int utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), NULL, 0, NULL, NULL );
							char *utf8_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * utf8_string_length );
							utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), utf8_val, utf8_string_length, NULL, NULL );

							WriteFile( hFile_string_list, utf8_val, utf8_string_length, &write, NULL );

							GlobalFree( utf8_val );
						}

						WriteFile( hFile_string_list, "\\r", 2, &write, NULL );

						last_ptr = ptr + 1;
					}
					else if ( *ptr == L'\n' )
					{
						if ( ptr - last_ptr > 0 )
						{
							int utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), NULL, 0, NULL, NULL );
							char *utf8_val = ( char * )GlobalAlloc( GPTR, sizeof( char ) * utf8_string_length );
							utf8_string_length = WideCharToMultiByte( CP_UTF8, 0, last_ptr, ( ptr - last_ptr ), utf8_val, utf8_string_length, NULL, NULL );

							WriteFile( hFile_string_list, utf8_val, utf8_string_length, &write, NULL );

							GlobalFree( utf8_val );
						}

						WriteFile( hFile_string_list, "\\n", 2, &write, NULL );

						last_ptr = ptr + 1;
					}
				}

				CloseHandle( hFile_string_list );
			}
		}

		GlobalFree( locale_buf );
	}
}

void OpenStringList( wchar_t *file_path )
{
	bool loaded_function = false;

	pGetUserDefaultLocaleName _GetUserDefaultLocaleName;

	HMODULE hm = LoadLibraryW( L"kernel32.dll" );

	_GetUserDefaultLocaleName = ( pGetUserDefaultLocaleName )GetProcAddress( hm, "GetUserDefaultLocaleName" );
	if ( _GetUserDefaultLocaleName != NULL )
	{
		loaded_function = true;
	}

	HANDLE hFile_input = CreateFile( file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_input != INVALID_HANDLE_VALUE )
	{
		unsigned char *input_buf = NULL;
		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_input, NULL );

		// The input string list shouldn't be that large.
		if ( fz < 131072 )
		{
			input_buf = ( unsigned char * )GlobalAlloc( GMEM_FIXED, sizeof( unsigned char ) * ( fz + 1 ) );
			ReadFile( hFile_input, input_buf, sizeof( unsigned char ) * fz, &read, NULL );
			input_buf[ fz ] = 0;	// Make sure it's NULL terminated.

			unsigned char *ptr_input_buf = input_buf;

			// Look for a UTF-8 BOM and ignore it.
			if ( read >= 3 && ( ptr_input_buf[ 0 ] == 0xEF && ptr_input_buf[ 1 ] == 0xBB && ptr_input_buf[ 2 ] == 0xBF ) )
			{
				ptr_input_buf += 3;
			}

			// Convert our strings to a wide character version.
			int strings_length = MultiByteToWideChar( CP_UTF8, 0, ( char * )ptr_input_buf, -1, NULL, 0 );	// Include the NULL character.
			wchar_t *strings = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * strings_length );
			MultiByteToWideChar( CP_UTF8, 0, ( char * )ptr_input_buf, -1, strings, strings_length );

			GlobalFree( input_buf );

			// Escape our strings. Replace carriage return/newline pairs with NULL, and "\t", "\r", "\n", "\x00-FF" with their binary equivalents.
			unsigned int escaped_strings_length = 0;
			wchar_t *escaped_strings = decode_w( strings, strings_length - 1, &escaped_strings_length );

			wchar_t locale_name[ LOCALE_NAME_MAX_LENGTH ];
			int locale_length = 0;

			wchar_t *filename = PathFindFileNameW( file_path );
			if ( filename != NULL && _wcsnicmp( filename, L"string_list_", 12 ) == 0 )
			{
				filename += 12;
				wchar_t *ptr = filename;
				while ( *ptr != 0 && *ptr != L'.' ) { ++ptr; }
				*ptr = 0;

				locale_length = min( LOCALE_NAME_MAX_LENGTH, ( ptr - filename ) + 1 );
				memcpy_s( locale_name, sizeof( wchar_t ) * LOCALE_NAME_MAX_LENGTH, filename, sizeof( wchar_t ) * locale_length );
				locale_name[ locale_length - 1  ] = 0;	// Sanity.
			}

			if ( locale_length == 0 )
			{
				if ( loaded_function )
				{
					// Find a specific locale based on the system's default.
					locale_length = _GetUserDefaultLocaleName( locale_name, LOCALE_NAME_MAX_LENGTH );
					locale_name[ locale_length ] = 0;	// Sanity.
				}
				else	// Try COM function instead.
				{
					HRESULT hr = CoInitialize( NULL );
					if ( SUCCEEDED( hr ) )
					{
						IMultiLanguage *pml;

						hr = CoCreateInstance( CLSID_CMultiLanguage, NULL, CLSCTX_ALL, IID_IMultiLanguage, ( void ** )&pml );
						if ( SUCCEEDED( hr ) )
						{
							BSTR bs;
							LCID lcid = GetUserDefaultLCID();

							hr = pml->GetRfc1766FromLcid( lcid, &bs );
							if ( SUCCEEDED( hr ) )
							{
								locale_length = SysStringLen( bs );
								locale_length = min( LOCALE_NAME_MAX_LENGTH, ( locale_length + 1 ) );
								memcpy_s( locale_name, sizeof( wchar_t ) * LOCALE_NAME_MAX_LENGTH, bs, sizeof( OLECHAR ) * locale_length );
								locale_name[ locale_length - 1 ] = 0;	// Sanity.

								SysFreeString( bs );
							}

							pml->Release();
						}

						CoUninitialize();
					}
				}
			}

			// Default name if locale name wasn't found.
			if ( locale_length == 0 )
			{
				locale_name[ 0 ] = L'd';
				locale_name[ 1 ] = L'e';
				locale_name[ 2 ] = L'f';
				locale_name[ 3 ] = L'a';
				locale_name[ 4 ] = L'u';
				locale_name[ 5 ] = L'l';
				locale_name[ 6 ] = L't';
				locale_name[ 7 ] = 0;
			}

			// Write our escaped strings.
			HANDLE hFile_output = CreateFile( locale_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile_output != INVALID_HANDLE_VALUE )
			{
				DWORD write = 0;

				// Include NULL terminator.
				WriteFile( hFile_output, escaped_strings, sizeof( wchar_t ) * ( escaped_strings_length + 1 ), &write, NULL );

				CloseHandle( hFile_output );
			}

			GlobalFree( strings );

			CloseHandle( hFile_input );
		}
	}

	if ( hm != NULL )
	{
		FreeLibrary( hm );
	}
}

int wmain( int argc, wchar_t *argv[] )
{
	if ( argc == 3 )
	{
		if ( wcsncmp( argv[ 1 ], L"--string-list", 13 ) == 0 )
		{
			OpenStringList( argv[ 2 ] );
		}
		else if ( wcsncmp( argv[ 1 ], L"--locale", 8 ) == 0 )
		{
			OpenLocaleFile( argv[ 2 ] );
		}
	}
	else
	{
		wprintf( L"Locale Generator for HTTP Downloader\r\n" );
		wprintf( L"https://erickutcher.github.io/#HTTP_Downloader\r\n" );
		wprintf( L"\r\nExample usage:\r\n" );
		wprintf( L"Convert string list to locale: locale_generator.exe --string-list \"string_list.txt\"\r\n" );
		wprintf( L"Convert locale to string list: locale_generator.exe --locale \"en-US\"\r\n" );
		wprintf( L"\r\nPress any key to continue . . . " );
		getchar();
	}

	return 0;
}
