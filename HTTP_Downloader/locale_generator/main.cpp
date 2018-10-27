#include <windows.h>
#include <mlang.h>
#include <string.h>

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
						  ( pstr[ 3 ] != NULL && is_hex_w( pstr[ 3 ] ) ) )
				{
					*pbuf++ = from_hex_w( pstr[ 2 ] ) << 4 | from_hex_w( pstr[ 3 ] );
					pstr += 3;
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

int main()
{
	bool loaded_function = false;

	pGetUserDefaultLocaleName _GetUserDefaultLocaleName;

	HMODULE hm = LoadLibraryW( L"kernel32.dll" );

	_GetUserDefaultLocaleName = ( pGetUserDefaultLocaleName )GetProcAddress( hm, "GetUserDefaultLocaleName" );
	if ( _GetUserDefaultLocaleName != NULL )
	{
		loaded_function = true;
	}

	HANDLE hFile_input = CreateFile( L"string_list.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile_input != INVALID_HANDLE_VALUE )
	{
		char *input_buf = NULL;
		DWORD read = 0;
		DWORD fz = GetFileSize( hFile_input, NULL );

		// The input string list shouldn't be that large.
		if ( fz < 131072 )
		{
			input_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * ( fz + 1 ) );
			ReadFile( hFile_input, input_buf, sizeof( char ) * fz, &read, NULL );
			input_buf[ fz ] = 0;	// Make sure it's NULL terminated.

			// Convert our strings to a wide character version.
			int strings_length = MultiByteToWideChar( CP_UTF8, 0, input_buf, -1, NULL, 0 );	// Include the NULL character.
			wchar_t *strings = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * strings_length );
			MultiByteToWideChar( CP_UTF8, 0, input_buf, -1, strings, strings_length );

			GlobalFree( input_buf );

			// Escape our strings. Replace carriage return/newline pairs with NULL, and "\t", "\r", "\n", "\x00-FF" with their binary equivalents.
			unsigned int escaped_strings_length = 0;
			wchar_t *escaped_strings = decode_w( strings, strings_length - 1, &escaped_strings_length );

			wchar_t locale_name[ LOCALE_NAME_MAX_LENGTH ];
			int locale_length = 0;

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
							memcpy_s( locale_name, sizeof( wchar_t ) * LOCALE_NAME_MAX_LENGTH, bs, sizeof( OLECHAR ) * locale_length );
							locale_name[ locale_length ] = 0;	// Sanity.

							SysFreeString( bs );
						}

						pml->Release();
					}

					CoUninitialize();
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

	return 0;
}
