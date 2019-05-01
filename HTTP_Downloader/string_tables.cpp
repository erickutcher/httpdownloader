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

#include "globals.h"
#include "string_tables.h"
#include "lite_kernel32.h"

bool g_use_dynamic_locale = false;	// Did we get the strings from a file? We'll need to free the memory when we're done.

STRING_TABLE_DATA g_locale_table[ TOTAL_LOCALE_STRINGS ];

// Ordered by month.
STRING_TABLE_DATA month_string_table[] =
{
	{ L"January", 7 },
	{ L"February", 8 },
	{ L"March", 5 },
	{ L"April", 5 },
	{ L"May", 3 },
	{ L"June", 4 },
	{ L"July", 4 },
	{ L"August", 6 },
	{ L"September", 9 },
	{ L"October", 7 },
	{ L"November", 8 },
	{ L"December", 8 }
};

// Ordered by day.
STRING_TABLE_DATA day_string_table[] =
{
	{ L"Sunday", 6 },
	{ L"Monday", 6 },
	{ L"Tuesday", 7 },
	{ L"Wednesday", 9 },
	{ L"Thursday", 8 },
	{ L"Friday", 6 },
	{ L"Saturday", 8 }
};

STRING_TABLE_DATA download_string_table[] =
{
	{ L"#", 1 },
	{ L"Active Parts", 12 },
	{ L"Date and Time Added", 19 },
	{ L"Download Directory", 18 },
	{ L"Download Speed", 14 },
	{ L"Downloaded", 10 },
	{ L"File Size", 9 },
	{ L"File Type", 9 },
	{ L"Filename", 8 },
	{ L"Progress", 8 },
	{ L"SSL / TLS Version", 17 },
	{ L"Time Elapsed", 12 },
	{ L"Time Remaining", 14 },
	{ L"URL", 3 }
};

STRING_TABLE_DATA menu_string_table[] =
{
	{ L"#", 1 },
	{ L"&About", 6 },
	{ L"Active Parts", 12 },
	{ L"&Add URL(s)...\tCtrl+N", 21 },
	{ L"Add URL(s)...", 13 },
	{ L"Always on Top", 13 },
	{ L"&Column Headers", 15 },
	{ L"&Copy URL(s)\tCtrl+C", 19 },
	{ L"Copy URL(s)", 11 },
	{ L"Date and Time Added", 19 },
	{ L"&Delete\tDel", 11 },
	{ L"Delete", 6 },
	{ L"Download Directory", 18 },
	{ L"Download Speed", 14 },
	{ L"Downloaded", 10 },
	{ L"&Edit", 5 },
	{ L"E&xit", 5 },
	{ L"Exit", 4 },
	{ L"&Export Download History...", 27 },
	{ L"&File", 5 },
	{ L"File Size", 9 },
	{ L"File Type", 9 },
	{ L"Filename", 8 },
	{ L"&Help", 5 },
	{ L"HTTP Downloader &Home Page", 26 },
	{ L"&Import Download History...", 27 },
	{ L"Move Down", 9 },
	{ L"Move to Bottom", 14 },
	{ L"Move to Top", 11 },
	{ L"Move Up", 7 },
	{ L"Open Directory", 14 },
	{ L"Open File", 9 },
	{ L"Open Download List", 18 },
	{ L"&Options...\tCtrl+O", 18 },
	{ L"Options...", 10 },
	{ L"&Pause", 6 },
	{ L"Pause", 5 },
	{ L"Pause Active", 12 },
	{ L"Progress", 8 },
	{ L"Queue", 5 },
	{ L"&Remove\tCtrl+R", 14 },
	{ L"Remove", 6 },
	{ L"Remove and Delete\tCtrl+Del", 26 },
	{ L"Remove and Delete", 17 },
	{ L"Remove Completed", 16 },
	{ L"Rename\tF2", 9 },
	{ L"Rename", 6 },
	{ L"Restart", 7 },
	{ L"&Save Download History...", 25 },
	{ L"&Search...\tCtrl+S", 17 },
	{ L"&Select All\tCtrl+A", 18 },
	{ L"Select All", 10 },
	{ L"SSL / TLS Version", 17 },
	{ L"St&art", 6 },
	{ L"Start", 5 },
	{ L"&Status Bar", 11 },
	{ L"St&op", 5 },
	{ L"Stop", 4 },
	{ L"Stop All", 8 },
	{ L"Time Elapsed", 12 },
	{ L"Time Remaining", 14 },
	{ L"&Toolbar", 8 },
	{ L"&Tools", 6 },
	{ L"Update Download...", 18 },
	{ L"URL", 3 },
	{ L"&View", 5 }
};

STRING_TABLE_DATA options_string_table[] =
{
	{ L"Advanced", 8 },
	{ L"Appearance", 10 },
	{ L"Apply", 5 },
	{ L"Connection", 10 },
	{ L"General", 7 },
	{ L"OK", 2 },
	{ L"Proxy", 5 }
};

STRING_TABLE_DATA options_advanced_string_table[] =
{
	{ L"Allow only one instance of the program to run", 45 },
	{ L"Continue Download", 17 },
	{ L"Default download directory:", 27 },
	{ L"Display Prompt", 14 },
	{ L"Download drag and drop URL(s) immediately", 41 },
	{ L"Enable download history", 23 },
	{ L"Enable quick file allocation (administrator access required)", 60 },
	{ L"Hibernate", 9 },
	{ L"Hybrid shut down", 16 },
	{ L"Lock", 4 },
	{ L"Log off", 7 },
	{ L"None", 4 },
	{ L"Overwrite File", 14 },
	{ L"Prevent system standby while downloads are active", 49 },
	{ L"Rename File", 11 },
	{ L"Restart", 7 },
	{ L"Restart Download", 16 },
	{ L"Resume previously downloading files upon startup", 48 },
	{ L"Set date and time of file from server response", 46 },
	{ L"Shut down", 9 },
	{ L"Skip Download", 13 },
	{ L"Sleep", 5 },
	{ L"System shutdown action when all downloads finish:", 49 },
	{ L"Thread pool count:", 18 },
	{ L"Use temporary download directory:", 33 },
	{ L"When a file already exists:", 27 },
	{ L"When a file has been modified:", 30 },
	{ L"When a file is greater than or equal to (bytes):", 48 }
};

STRING_TABLE_DATA options_appearance_string_table[] =
{
	{ L"Background Color", 16 },
	{ L"Background Font Color", 21 },
	{ L"Border Color", 12 },
	{ L"Download list:", 14 },
	{ L"Even Row Background Color", 25 },
	{ L"Even Row Font", 13 },
	{ L"Even Row Font Color", 19 },
	{ L"Even Row Highlight Color", 24 },
	{ L"Even Row Highlight Font Color", 29 },
	{ L"Odd Row Background Color", 24 },
	{ L"Odd Row Font", 12 },
	{ L"Odd Row Font Color", 18 },
	{ L"Odd Row Highlight Color", 23 },
	{ L"Odd Row Highlight Font Color", 28 },
	{ L"Progress Color", 14 },
	{ L"Progress bar:", 13 },
	{ L"Progress Font Color", 19 },
	{ L"Show gridlines in download list", 31 }
};

STRING_TABLE_DATA options_connection_string_table[] =
{
	{ L":", 1 },
	{ L"Active download limit:", 22 },
	{ L"Basic Authentication", 20 },
	{ L"Certificate file:", 17 },
	{ L"Default download parts:", 23 },
	{ L"Default SSL / TLS version:", 26 },
	{ L"Digest Authentication", 21 },
	{ L"Enable server:", 14 },
	{ L"Enable SSL / TLS:", 17 },
	{ L"Hostname / IPv6 address:", 24 },
	{ L"IPv4 address:", 13 },
	{ L"Key file:", 9 },
	{ L"Load PKCS #12 File", 18 },
	{ L"Load Private Key File", 21 },
	{ L"Load X.509 Certificate File", 27 },
	{ L"Login Manager...", 16 },
	{ L"Maximum redirects:", 18 },
	{ L"PKCS #12:", 9 },
	{ L"PKCS #12 file:", 14 },
	{ L"PKCS #12 password:", 18 },
	{ L"Port:", 5 },
	{ L"Public / Private key pair:", 26 },
	{ L"Require authentication:", 23 },
	{ L"Retry incomplete downloads:", 27 },
	{ L"Retry incomplete parts:", 23 },
	{ L"Server SSL / TLS version:", 25 },
	{ L"Timeout (seconds):", 18 }
};

STRING_TABLE_DATA options_general_string_table[] =
{
	{ L"Always on top", 13 },
	{ L"Close to System Tray", 20 },
	{ L"Enable System Tray icon:", 24 },
	{ L"Enable URL drop window:", 23 },
	{ L"Load Download Finish Sound File", 31 },
	{ L"Minimize to System Tray", 23 },
	{ L"Play sound when all downloads finish:", 37 },
	{ L"Show notification when all downloads finish", 43 },
	{ L"Show progress bar", 17 },
	{ L"Transparency:", 13 }
};

STRING_TABLE_DATA options_proxy_string_table[] =
{
	{ L"Allow proxy to resolve domain names", 35 },
	{ L"Allow proxy to resolve domain names (use SOCKS v4a)", 51 },
	{ L"Hostname:", 9 },
	{ L"SOCKS v4", 8 },
	{ L"SOCKS v5", 8 },
	{ L"Use authentication:", 19 },
	{ L"Use HTTP proxy:", 15 },
	{ L"Use HTTPS proxy:", 16 },
	{ L"Use SOCKS proxy:", 16 }
};

STRING_TABLE_DATA cmessagebox_string_table[] =
{
	{ L"Continue", 8 },
	{ L"No", 2 },
	{ L"Overwrite", 9 },
	{ L"Remember choice", 15 },
	{ L"Skip", 4 },
	{ L"Skip remaining messages", 23 },
	{ L"Yes", 3 }
};

STRING_TABLE_DATA add_urls_string_table[] =
{
	{ L"Advanced options", 16 },
	{ L"Authentication", 14 },
	{ L"Cookies", 7 },
	{ L"Cookies:", 8 },
	{ L"Custom", 6 },
	{ L"Download", 8 },
	{ L"Download directory:", 19 },
	{ L"Download parts:", 15 },
	{ L"Headers", 7 },
	{ L"Headers:", 8 },
	{ L"Images", 6 },
	{ L"Music", 5 },
	{ L"Password:", 9 },
	{ L"POST Data", 9 },
	{ L"RegEx filter:", 13 },
	{ L"Send POST Data:", 15 },
	{ L"Simulate download", 17 },
	{ L"SSL / TLS version:", 18 },
	{ L"URL(s):", 7 },
	{ L"Username:", 9 },
	{ L"Videos", 6 }
};

STRING_TABLE_DATA search_string_table[] =
{
	{ L"Match case", 10 },
	{ L"Match whole word", 16 },
	{ L"Regular expression", 18 },
	{ L"Search", 6 },
	{ L"Search All", 10 },
	{ L"Search for:", 11 },
	{ L"Search Next", 11 },
	{ L"Search Type", 11 }
};

STRING_TABLE_DATA login_manager_string_table[] =
{
	{ L"Add", 3 },
	{ L"Close", 5 },
	{ L"Password", 8 },
	{ L"Remove", 6 },
	{ L"Show passwords", 14 },
	{ L"Site", 4 },
	{ L"Site:", 5 },
	{ L"Username", 8 }
};

STRING_TABLE_DATA common_string_table[] =
{
	{ L"...", 3 },
	{ L"[Simulated]", 11 },
	{ L"Add URL(s)", 10 },
	{ L"Added", 5 },
	{ L"Allocating File", 15 },
	{ L"Authorization Required", 22 },
	{ L"Cancel", 6 },
	{ L"Completed", 9 },
	{ L"Connecting", 10 },
	{ L"Download speed: ", 16 },
	{ L"Download speed: 0 B/s", 21 },
	{ L"Download speed: 0.00 GB/s", 25 },
	{ L"Download speed: 0.00 KB/s", 25 },
	{ L"Download speed: 0.00 MB/s", 25 },
	{ L"Downloading", 11 },
	{ L"Downloads Have Finished", 23 },
	{ L"Export Download History", 23 },
	{ L"Failed", 6 },
	{ L"File IO Error", 13 },
	{ L"Import Download History", 23 },
	{ L"Login Manager", 13 },
	{ L"Moving File", 11 },
	{ L"Options", 7 },
	{ L"Paused", 6 },
	{ L"Proxy Authentication Required", 29 },
	{ L"Queued", 6 },
	{ L"Restarting", 10 },
	{ L"Save Download History", 21 },
	{ L"Skipped", 7 },
	{ L"Stopped", 7 },
	{ L"SSL 2.0", 7 },
	{ L"SSL 3.0", 7 },
	{ L"Timed Out", 9 },
	{ L"TLS 1.0", 7 },
	{ L"TLS 1.1", 7 },
	{ L"TLS 1.2", 7 },
	{ L"Total downloaded: ", 18 },
	{ L"Total downloaded: 0 B", 21 },
	{ L"Total downloaded: 0.00 GB", 25 },
	{ L"Total downloaded: 0.00 KB", 25 },
	{ L"Total downloaded: 0.00 MB", 25 },
	{ L"URL:", 4 },
	{ L"Update", 6 },
	{ L"Update Download", 15 }
};

STRING_TABLE_DATA common_message_string_table[] =
{
	{ L"A protocol (HTTP or HTTPS) must be supplied.", 44 },
	{ L"A restart is required for these changes to take effect.", 55 },
	{ L"A restart is required to enable quick file allocation.", 54 },
	{ L"A restart is required to perform the system shutdown action.", 60 },
	{ L"A restart is required to update the thread pool count.", 54 },
	{ L"Are you sure you want to delete the selected files?", 51 },
	{ L"Are you sure you want to remove the completed entries?", 54 },
	{ L"Are you sure you want to remove and delete the selected entries?", 64 },
	{ L"Are you sure you want to remove the selected entries?", 53 },
	{ L"Are you sure you want to restart the selected entries?", 54 },
	{ L"One or more files are in use and cannot be deleted.", 51 },
	{ L"One or more files were not found.", 33 },
	{ L"Select the default download directory.", 38 },
	{ L"Select the download directory.", 30 },
	{ L"Select the temporary download directory.", 40 },
	{ L"The download will be resumed after it's updated.", 48 },
	{ L"The file is currently in use and cannot be deleted.", 51 },
	{ L"The file is currently in use and cannot be renamed.", 51 },
	{ L"The file(s) could not be imported because the format is incorrect.", 66 },
	{ L"The specified file was not found.\r\n\r\nDo you want to download the file again?", 76 },
	{ L"The specified path was not found.", 33 },
	{ L"The specified site already exists.", 34 },
	{ L"The specified site is invalid.", 30 },
	{ L"There is already a file with the same name in this location.", 60 },
	{ L"You must supply a download directory.", 37 }
};

void InitializeLocaleValues()
{
	unsigned short string_count = 0;

	bool use_locale_file = true;

	_memzero( g_locale_table, sizeof( STRING_TABLE_DATA ) * TOTAL_LOCALE_STRINGS );

	wchar_t directory[ MAX_PATH ];
	//int directory_length = GetCurrentDirectoryW( MAX_PATH, directory );
	int directory_length = GetModuleFileNameW( NULL, directory, MAX_PATH );
	while ( directory_length != 0 && directory[ --directory_length ] != L'\\' );
	directory[ directory_length ] = 0;	// Sanity.

	// Find the default locale.
	_wmemcpy_s( directory + directory_length, MAX_PATH - directory_length, L"\\locale\\default\0", 16 );
	directory[ directory_length + 15 ] = 0;	// Sanity.

	if ( GetFileAttributesW( directory ) == INVALID_FILE_ATTRIBUTES )
	{
		int locale_length = 0;

		// Make sure GetUserDefaultLocaleName is available on our system.
		if ( kernel32_state != KERNEL32_STATE_SHUTDOWN )
		{
			// LOCALE_NAME_MAX_LENGTH
			// Find a specific locale based on the system's default.
			locale_length = _GetUserDefaultLocaleName( directory + directory_length + 8, MAX_PATH - ( directory_length + 8 ) );
			directory[ directory_length + 8 + locale_length - 1 ] = 0;	// Sanity.
		}

		if ( locale_length == 0 )
		{
			use_locale_file = false;
		}
	}

	if ( use_locale_file )
	{
		HANDLE hFile_locale = CreateFile( directory, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile_locale != INVALID_HANDLE_VALUE )
		{
			char *locale_buf = NULL;
			DWORD read = 0, pos = 0;
			DWORD fz = GetFileSize( hFile_locale, NULL );

			if ( fz > sizeof( wchar_t ) && fz < 131072 )
			{
				locale_buf = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * fz + 2 );

				ReadFile( hFile_locale, locale_buf, sizeof( char ) * fz, &read, NULL );

				// Guarantee a NULL terminated (wide character) buffer.
				locale_buf[ fz ] = 0;
				locale_buf[ fz + 1 ] = 0;
			}
			else
			{
				use_locale_file = false;	// Incorrect file size.
			}

			CloseHandle( hFile_locale );

			if ( read > sizeof( wchar_t ) )
			{
				wchar_t *ptr = ( wchar_t * )locale_buf;
				wchar_t *last_ptr = ptr;
				wchar_t *ptr_end = ( wchar_t * )( ( char * )( locale_buf + read ) );

				while ( string_count < TOTAL_LOCALE_STRINGS && ++ptr < ptr_end )
				{
					if ( *ptr == NULL )
					{
						g_locale_table[ string_count ].value = last_ptr;
						g_locale_table[ string_count ].length = ( unsigned short )( ptr - last_ptr );

						++ptr;
						last_ptr = ptr;

						++string_count;
					}
				}

				g_use_dynamic_locale = true;
			}
			else
			{
				GlobalFree( locale_buf );

				use_locale_file = false;	// Incorrect file size.
			}
		}
		else
		{
			use_locale_file = false;	// Can't open file for reading.
		}
	}

	if ( !use_locale_file )
	{
		unsigned char i;

		for ( i = 0; i < MONTH_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = month_string_table[ i ]; }
		for ( i = 0; i < DAY_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = day_string_table[ i ]; }
		for ( i = 0; i < DOWNLOAD_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = download_string_table[ i ]; }
		for ( i = 0; i < MENU_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = menu_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_ADVANCED_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_advanced_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_APPEARANCE_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_appearance_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_CONNECTION_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_connection_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_GENERAL_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_general_string_table[ i ]; }
		for ( i = 0; i < OPTIONS_PROXY_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = options_proxy_string_table[ i ]; }
		for ( i = 0; i < CMESSAGEBOX_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = cmessagebox_string_table[ i ]; }
		for ( i = 0; i < ADD_URLS_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = add_urls_string_table[ i ]; }
		for ( i = 0; i < SEARCH_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = search_string_table[ i ]; }
		for ( i = 0; i < LOGIN_MANAGER_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = login_manager_string_table[ i ]; }
		for ( i = 0; i < COMMON_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = common_string_table[ i ]; }
		for ( i = 0; i < COMMON_MESSAGE_STRING_TABLE_SIZE; ++i ) { g_locale_table[ string_count++ ] = common_message_string_table[ i ]; }

		/*
		// Quick locale generation.
		HANDLE hFile_locale = CreateFile( L"en-US", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile_locale != INVALID_HANDLE_VALUE )
		{
			DWORD write = 0;

			for ( short j = 0; j < TOTAL_LOCALE_STRINGS; ++j )
			{
				// Include NULL terminator.
				WriteFile( hFile_locale, g_locale_table[ j ].value, sizeof( wchar_t ) * ( g_locale_table[ j ].length + 1 ), &write, NULL );
			}

			CloseHandle( hFile_locale );
		}
		*/
	}
}

void UninitializeLocaleValues()
{
	if ( g_use_dynamic_locale )
	{
		// The first pointer points to the entire buffer that we read from the file.
		// So we only need to delete it and nothing else.
		GlobalFree( g_locale_table[ 0 ].value );
	}
}
