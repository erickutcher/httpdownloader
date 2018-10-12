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
#include "wnd_proc.h"

#include "utilities.h"

#include "file_operations.h"

#include "ssl.h"

#include "lite_shell32.h"
#include "lite_advapi32.h"
#include "lite_comdlg32.h"
#include "lite_crypt32.h"
#include "lite_comdlg32.h"
#include "lite_comctl32.h"
#include "lite_gdi32.h"
#include "lite_ole32.h"
#include "lite_winmm.h"
#include "lite_zlib1.h"
#include "lite_normaliz.h"

#include "cmessagebox.h"

#include "connection.h"

//#define USE_DEBUG_DIRECTORY

#ifdef USE_DEBUG_DIRECTORY
	#define BASE_DIRECTORY_FLAG CSIDL_APPDATA
#else
	#define BASE_DIRECTORY_FLAG CSIDL_LOCAL_APPDATA
#endif

SYSTEMTIME g_compile_time;

HANDLE downloader_ready_semaphore = NULL;

CRITICAL_SECTION worker_cs;				// Worker thread critical section.

CRITICAL_SECTION session_totals_cs;

CRITICAL_SECTION icon_cache_cs;

// Object variables
HWND g_hWnd_main = NULL;		// Handle to our main window.

HWND g_hWnd_active = NULL;		// Handle to the active window. Used to handle tab stops.

HFONT hFont = NULL;				// Handle to our font object.

UINT CF_HTML = 0;

int row_height = 0;

wchar_t *base_directory = NULL;
unsigned int base_directory_length = 0;

dllrbt_tree *icon_handles = NULL;

bool can_fast_allocate = false;

#ifndef NTDLL_USE_STATIC_LIB
int APIENTRY _WinMain()
#else
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif
{
	#ifndef NTDLL_USE_STATIC_LIB
		HINSTANCE hInstance = GetModuleHandle( NULL );
	#endif

	#ifndef USER32_USE_STATIC_LIB
		if ( !InitializeUser32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		if ( !InitializeNTDLL() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		if ( !InitializeGDI32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef SHELL32_USE_STATIC_LIB
		if ( !InitializeShell32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef ADVAPI32_USE_STATIC_LIB
		if ( !InitializeAdvApi32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		if ( !InitializeComDlg32() ){ return false; }
	#endif
	#ifndef COMCTL32_USE_STATIC_LIB
		if ( !InitializeComCtl32() ){ return false; }
	#endif
	#ifndef CRYPT32_USE_STATIC_LIB
		if ( !InitializeCrypt32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef ZLIB1_USE_STATIC_LIB
		if ( !InitializeZLib1() )
		{
			if ( _MessageBoxW( NULL, L"The zlib compression library (zlib1.dll) could not be loaded.\r\n\r\nCompressed downloads will need to be manually decompressed.\r\n\r\nWould you like to visit www.zlib.net to download the DLL file?", PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
			{
				bool destroy = true;
				#ifndef OLE32_USE_STATIC_LIB
					if ( ole32_state == OLE32_STATE_SHUTDOWN )
					{
						destroy = InitializeOle32();
					}
				#endif

				if ( destroy )
				{
					_CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
				}

				_ShellExecuteW( NULL, L"open", L"http://www.zlib.net/", NULL, NULL, SW_SHOWNORMAL );

				if ( destroy )
				{
					_CoUninitialize();
				}
			}
		}
	#endif
	#ifndef NORMALIZ_USE_STATIC_LIB
		InitializeNormaliz();
	#endif

	_memzero( &g_compile_time, sizeof( SYSTEMTIME ) );
	if ( hInstance != NULL )
	{
		IMAGE_DOS_HEADER *idh = ( IMAGE_DOS_HEADER * )hInstance;
		IMAGE_NT_HEADERS *inth = ( IMAGE_NT_HEADERS * )( ( BYTE * )idh + idh->e_lfanew );

		UnixTimeToSystemTime( inth->FileHeader.TimeDateStamp, &g_compile_time );
	}

	unsigned char fail_type = 0;
	MSG msg;
	_memzero( &msg, sizeof( MSG ) );

	wchar_t *url_arg = NULL;
	unsigned int url_arg_length = 0;

	base_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );

	// Get the new base directory if the user supplied a path.
	bool default_directory = true;
	int argCount = 0;
	LPWSTR *szArgList = _CommandLineToArgvW( GetCommandLineW(), &argCount );
	if ( szArgList != NULL )
	{
		// The first parameter (index 0) is the path to the executable.
		for ( int arg = 1; arg < argCount; ++arg )
		{
			if ( szArgList[ arg ][ 0 ] == L'-' )
			{
				if ( szArgList[ arg ][ 1 ] == L'd' && szArgList[ arg ][ 2 ] == 0 )	// Set the base directory.
				{
					++arg;	// Move to the supplied directory.

					if ( GetFileAttributesW( szArgList[ arg ] ) == FILE_ATTRIBUTE_DIRECTORY )
					{
						base_directory_length = lstrlenW( szArgList[ arg ] );
						if ( base_directory_length >= MAX_PATH )
						{
							base_directory_length = MAX_PATH - 1;
						}
						_wmemcpy_s( base_directory, MAX_PATH, szArgList[ arg ], base_directory_length );
						base_directory[ base_directory_length ] = 0;	// Sanity.

						default_directory = false;
					}
				}
				else if ( szArgList[ arg ][ 1 ] == L'p' && szArgList[ arg ][ 2 ] == 0 )	// Portable mode (use the application's current directory for our base directory).
				{
					base_directory_length = lstrlenW( szArgList[ 0 ] );
					while ( base_directory_length != 0 && szArgList[ 0 ][ --base_directory_length ] != L'\\' );

					_wmemcpy_s( base_directory, MAX_PATH, szArgList[ 0 ], base_directory_length );
					base_directory[ base_directory_length ] = 0;	// Sanity.

					default_directory = false;
				}
				else if ( szArgList[ arg ][ 1 ] == L'u' && szArgList[ arg ][ 2 ] == 0 )	// A URL was supplied.
				{
					++arg;	// Move to the supplied URL.

					if ( url_arg != NULL )
					{
						GlobalFree( url_arg );
						url_arg = NULL;
					}

					url_arg_length = lstrlenW( szArgList[ arg ] );
					if ( url_arg_length > 0 )
					{
						url_arg = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( url_arg_length + 1 ) );
						_wmemcpy_s( url_arg, url_arg_length + 1, szArgList[ arg ], url_arg_length );
						url_arg[ url_arg_length ] = 0;	// Sanity.
					}
				}
			}
		}

		// Free the parameter list.
		LocalFree( szArgList );
	}

	// Use our default directory if none was supplied or check if there's a "portable" file in the same directory.
	if ( default_directory )
	{
		base_directory_length = GetCurrentDirectoryW( MAX_PATH, base_directory );
		_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\portable\0", 10 );

		// If there's a portable file in the same directory, then we'll use that directory as our base.
		// If not, then we'll use the APPDATA folder.
		if ( GetFileAttributesW( base_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			_SHGetFolderPathW( NULL, BASE_DIRECTORY_FLAG, NULL, 0, base_directory );

			base_directory_length = lstrlenW( base_directory );
			_wmemcpy_s( base_directory + base_directory_length, MAX_PATH - base_directory_length, L"\\HTTP Downloader\0", 17 );
			base_directory_length += 16;
			base_directory[ base_directory_length ] = 0;	// Sanity.

			// Check to see if the new path exists and create it if it doesn't.
			if ( GetFileAttributesW( base_directory ) == INVALID_FILE_ATTRIBUTES )
			{
				CreateDirectoryW( base_directory, NULL );
			}
		}
		else
		{
			base_directory[ base_directory_length ] = 0;	// Sanity.
		}
	}

	// Check to see if the new path exists and create it if it doesn't.
	if ( GetFileAttributesW( base_directory ) == INVALID_FILE_ATTRIBUTES )
	{
		CreateDirectoryW( base_directory, NULL );
	}

	InitializeCriticalSection( &worker_cs );

	InitializeCriticalSection( &session_totals_cs );

	InitializeCriticalSection( &icon_cache_cs );

	InitializeCriticalSection( &context_list_cs );
	InitializeCriticalSection( &active_download_list_cs );
	InitializeCriticalSection( &download_queue_cs );
	InitializeCriticalSection( &cmessagebox_prompt_cs );
	InitializeCriticalSection( &file_size_prompt_list_cs );
	InitializeCriticalSection( &rename_file_prompt_list_cs );
	InitializeCriticalSection( &cleanup_cs );

	// Get the default message system font.
	NONCLIENTMETRICS ncm;
	_memzero( &ncm, sizeof( NONCLIENTMETRICS ) );
	ncm.cbSize = sizeof( NONCLIENTMETRICS );
	_SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 );

	// Set our global font to the LOGFONT value obtained from the system.
	hFont = _CreateFontIndirectW( &ncm.lfMessageFont );

	// Get the row height for our listview control.
	TEXTMETRIC tm;
	HDC hDC = _GetDC( NULL );
	HFONT ohf = ( HFONT )_SelectObject( hDC, hFont );
	_GetTextMetricsW( hDC, &tm );
	_SelectObject( hDC, ohf );	// Reset old font.
	_ReleaseDC( NULL, hDC );

	row_height = tm.tmHeight + tm.tmExternalLeading + 5;

	int icon_height = _GetSystemMetrics( SM_CYSMICON ) + 2;
	if ( row_height < icon_height )
	{
		row_height = icon_height;
	}

	// Default position if no settings were saved.
	cfg_pos_x = ( ( _GetSystemMetrics( SM_CXSCREEN ) - MIN_WIDTH ) / 2 );
	cfg_pos_y = ( ( _GetSystemMetrics( SM_CYSCREEN ) - MIN_HEIGHT ) / 2 );

	cfg_drop_pos_x = ( ( _GetSystemMetrics( SM_CXSCREEN ) - 48 ) / 2 );
	cfg_drop_pos_y = ( ( _GetSystemMetrics( SM_CYSCREEN ) - 48 ) / 2 );

	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );

	if ( systemInfo.dwNumberOfProcessors > 0 )
	{
		g_max_threads = systemInfo.dwNumberOfProcessors * 2;	// Default is 2.
		cfg_thread_count = systemInfo.dwNumberOfProcessors;		// Default is 1.
	}

	CF_HTML = _RegisterClipboardFormatW( L"HTML Format" );

	read_config();

	// See if there's an instance of the program running.
	HANDLE app_instance_mutex = OpenMutexW( MUTEX_ALL_ACCESS, 0, L"HTTP Downloader" );
	if ( app_instance_mutex == NULL )
	{
		app_instance_mutex = CreateMutexW( NULL, 0, L"HTTP Downloader" );
		if ( app_instance_mutex == NULL )
		{
			goto CLEANUP;
		}
	}
	else	// There's already an instance of the program running.
	{
		if ( cfg_use_one_instance )
		{
			HWND hWnd_instance = FindWindow( L"http_downloader_class", NULL );
			if ( hWnd_instance != NULL )
			{
				// If we're passing a URL, then the Add URLs window will take focus when the copy data is received.
				// If we're not passing a URL, then show the main window.
				if ( url_arg != NULL )
				{
					COPYDATASTRUCT cds;
					cds.dwData = 0;
					cds.cbData = sizeof( wchar_t ) * ( url_arg_length + 1 );	// Include the NULL terminator.
					cds.lpData = ( PVOID )url_arg;
					_SendMessageW( hWnd_instance, WM_COPYDATA, 0, ( LPARAM )&cds );
				}
				else
				{
					_ShowWindow( hWnd_instance, SW_SHOW );
					_SetForegroundWindow( hWnd_instance );
				}
			}

			goto CLEANUP;
		}
	}

	if ( cfg_enable_quick_allocation )
	{
		// Check if we're an admin or have elevated privileges. Allow us to pre-allocate a file without zeroing it.
		HANDLE hToken, hLinkedToken;
		TOKEN_PRIVILEGES tp;
		LUID luid;

		BOOL is_member = FALSE, is_privileged = FALSE;

		SID_IDENTIFIER_AUTHORITY NtAuthority;	// Set to SECURITY_NT_AUTHORITY

		PSID AdministratorsGroup;

		DWORD retLen;
		TOKEN_ELEVATION_TYPE elevation_type;

		if ( _OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		{
			NtAuthority.Value[ 0 ] = 0;
			NtAuthority.Value[ 1 ] = 0;
			NtAuthority.Value[ 2 ] = 0;
			NtAuthority.Value[ 3 ] = 0;
			NtAuthority.Value[ 4 ] = 0;
			NtAuthority.Value[ 5 ] = 5;

			// See if we're in the administrator group.
			if ( _AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup ) )
			{
				if ( !_CheckTokenMembership( NULL, AdministratorsGroup, &is_member ) )
				{
					 is_member = FALSE;
				}

				// See if the SID was filtered.
				if ( is_member == FALSE )
				{
					if ( _GetTokenInformation( hToken, TokenLinkedToken, ( VOID * )&hLinkedToken, sizeof( HANDLE ), &retLen ) )
					{
						if ( !_CheckTokenMembership( hLinkedToken, AdministratorsGroup, &is_member ) )
						{
							is_member = FALSE;
						}
					}

					CloseHandle( hLinkedToken );
				}
			}

			_FreeSid( AdministratorsGroup );

			// Determine if we have elevated privileges.
			if ( _GetTokenInformation( hToken, TokenElevationType, &elevation_type, sizeof( TOKEN_ELEVATION_TYPE ), &retLen ) )
			{
				switch ( elevation_type )
				{
					case TokenElevationTypeFull: { is_privileged = TRUE; } break;
					case TokenElevationTypeLimited: { is_privileged = FALSE; } break;
					default: { is_privileged = is_member; } break;
				}
			}

			if ( is_privileged == TRUE && _LookupPrivilegeValueW( NULL, SE_MANAGE_VOLUME_NAME, &luid ) )
			{
				tp.PrivilegeCount = 1;
				tp.Privileges[ 0 ].Luid = luid;
				tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

				if ( _AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), NULL, NULL ) )
				{
					can_fast_allocate = true;
				}
			}
		}

		CloseHandle( hToken );
		////

		// Only prompt to elevate if we're not currently an administrator.
		if ( is_privileged == FALSE )
		{
			wchar_t *tmp_command_line_path = GetCommandLineW();	// DO NOT FREE!
			wchar_t *command_line_path = GlobalStrDupW( tmp_command_line_path );

			// Find the start of any parameters and zero out the space between file path and parameters.
			tmp_command_line_path = command_line_path;

			// See if our file path is quoted.
			bool quoted = false;
			if ( tmp_command_line_path != NULL )
			{
				if ( *tmp_command_line_path == L'\"' )
				{
					quoted = true;
				}

				++tmp_command_line_path;
			}

			// Find the end of the quote, or the first space.
			while ( tmp_command_line_path != NULL )
			{
				if ( quoted )
				{
					// Find the end of the quote.
					if ( *tmp_command_line_path == L'\"' )
					{
						// If the next character is not NULL, then we'll assume it's a space.
						if ( ++tmp_command_line_path != NULL )
						{
							*tmp_command_line_path = 0;	// Zero out the space.

							++tmp_command_line_path;	// Move to the parameters.
						}

						break;
					}
				}
				else
				{
					// Find the first space.
					if ( *tmp_command_line_path == L' ' )
					{
						*tmp_command_line_path = 0;	// Zero out the space.

						++tmp_command_line_path;	// Move to the parameters.

						break;
					}
				}

				++tmp_command_line_path;
			}

			// If successful, cleanup the current program and run the new one.
			// A return value that's greater than 32 means it was a success.
			if ( ( int )_ShellExecuteW( NULL, L"runas", command_line_path, tmp_command_line_path, NULL, SW_SHOWNORMAL ) > 32 )
			{
				GlobalFree( command_line_path );

				goto CLEANUP;
			}

			GlobalFree( command_line_path );
		}
	}

	if ( normaliz_state == NORMALIZ_STATE_RUNNING )
	{
		int hostname_length = 0;
		int punycode_length = 0;

		if ( cfg_address_type == 0 )
		{
			hostname_length = lstrlenW( cfg_hostname ) + 1;	// Include the NULL terminator.
			punycode_length = _IdnToAscii( 0, cfg_hostname, hostname_length, NULL, 0 );

			if ( punycode_length > hostname_length )
			{
				g_punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
				_IdnToAscii( 0, cfg_hostname, hostname_length, g_punycode_hostname, punycode_length );
			}
		}

		if ( cfg_address_type_s == 0 )
		{
			hostname_length = lstrlenW( cfg_hostname_s ) + 1;	// Include the NULL terminator.
			punycode_length = _IdnToAscii( 0, cfg_hostname_s, hostname_length, NULL, 0 );

			if ( punycode_length > hostname_length )
			{
				g_punycode_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
				_IdnToAscii( 0, cfg_hostname_s, hostname_length, g_punycode_hostname_s, punycode_length );
			}
		}
	}

	int auth_username_length = 0, auth_password_length = 0;

	if ( cfg_proxy_auth_username != NULL && cfg_proxy_auth_password != NULL )
	{
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, g_proxy_auth_username, auth_username_length, NULL, NULL ) - 1;

		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, g_proxy_auth_password, auth_password_length, NULL, NULL ) - 1;

		CreateBasicAuthorizationKey( g_proxy_auth_username, auth_username_length, g_proxy_auth_password, auth_password_length, &g_proxy_auth_key, &g_proxy_auth_key_length );
	}

	if ( cfg_proxy_auth_username_s != NULL && cfg_proxy_auth_password_s != NULL )
	{
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_username_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, g_proxy_auth_username_s, auth_username_length, NULL, NULL ) - 1;

		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_password_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, g_proxy_auth_password_s, auth_password_length, NULL, NULL ) - 1;

		CreateBasicAuthorizationKey( g_proxy_auth_username_s, auth_username_length, g_proxy_auth_password_s, auth_password_length, &g_proxy_auth_key_s, &g_proxy_auth_key_length_s );
	}

	icon_handles = dllrbt_create( dllrbt_compare_w );

	downloader_ready_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

	CloseHandle( _CreateThread( NULL, 0, IOCPDownloader, NULL, 0, NULL ) );

	// Wait for IOCPDownloader to set up the completion port. 10 second timeout in case we miss the release.
	WaitForSingleObject( downloader_ready_semaphore, 10000 );
	CloseHandle( downloader_ready_semaphore );
	downloader_ready_semaphore = NULL;

	// Initialize our window class.
	WNDCLASSEX wcex;
	_memzero( &wcex, sizeof( WNDCLASSEX ) );
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style          = 0;//CS_VREDRAW | CS_HREDRAW;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = _LoadIconW( hInstance, MAKEINTRESOURCE( IDI_ICON ) );
	wcex.hCursor        = _LoadCursorW( NULL, IDC_ARROW );
	wcex.hbrBackground  = ( HBRUSH )( COLOR_WINDOW );
	wcex.lpszMenuName   = NULL;
	wcex.hIconSm        = NULL;

	// Since the main window's children cover it up, we don't need to redraw the window.
	// This also prevents the status bar child from flickering during a window resize. Dumb!
	wcex.lpfnWndProc    = MainWndProc;
	wcex.lpszClassName  = L"http_downloader_class";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.style          = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc    = AddURLsWndProc;
	wcex.lpszClassName  = L"add_urls";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = OptionsWndProc;
	wcex.lpszClassName  = L"options";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = UpdateDownloadWndProc;
	wcex.lpszClassName  = L"update_download";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.hIcon			= NULL;
	wcex.hbrBackground  = ( HBRUSH )( COLOR_WINDOWFRAME );

	wcex.lpfnWndProc    = ProxyTabWndProc;
	wcex.lpszClassName  = L"proxy_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = ConnectionTabWndProc;
	wcex.lpszClassName  = L"connection_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = GeneralTabWndProc;
	wcex.lpszClassName  = L"general_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc    = URLDropWndProc;
	wcex.lpszClassName  = L"url_drop_window";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	if ( !InitializeCMessageBox( hInstance ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	g_hWnd_main = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ), L"http_downloader_class", PROGRAM_CAPTION, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, cfg_pos_x, cfg_pos_y, cfg_width, cfg_height, NULL, NULL, NULL, NULL );

	if ( !g_hWnd_main )
	{
		fail_type = 2;
		goto CLEANUP;
	}

	_ShowWindow( g_hWnd_main, ( cfg_min_max == 1 ? SW_MINIMIZE : ( cfg_min_max == 2 ? SW_MAXIMIZE : SW_SHOWNORMAL ) ) );

	if ( url_arg != NULL )
	{
		_SendMessageW( g_hWnd_main, WM_PROPAGATE, CF_UNICODETEXT, ( LPARAM )url_arg );

		GlobalFree( url_arg );
		url_arg = NULL;
	}

	if ( cfg_enable_drop_window )
	{
		g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE, cfg_drop_pos_x, cfg_drop_pos_y, 48, 48, NULL, NULL, NULL, NULL );
		_SetWindowLongW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
		_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, 0x80, LWA_ALPHA );
	}

	// Main message loop:
	while ( _GetMessageW( &msg, NULL, 0, 0 ) > 0 )
	{
		if ( g_hWnd_active == NULL || !_IsDialogMessageW( g_hWnd_active, &msg ) )	// Checks tab stops.
		{
			_TranslateMessage( &msg );
			_DispatchMessageW( &msg );
		}
	}

CLEANUP:

	save_config();

	if ( url_arg != NULL ) { GlobalFree( url_arg ); }

	if ( base_directory != NULL ) { GlobalFree( base_directory ); }
	if ( cfg_default_download_directory != NULL ) { GlobalFree( cfg_default_download_directory ); }

	if ( cfg_hostname != NULL ) { GlobalFree( cfg_hostname ); }
	if ( g_punycode_hostname != NULL ) { GlobalFree( g_punycode_hostname ); }

	if ( cfg_proxy_auth_username != NULL ) { GlobalFree( cfg_proxy_auth_username ); }
	if ( cfg_proxy_auth_password != NULL ) { GlobalFree( cfg_proxy_auth_password ); }
	if ( g_proxy_auth_username != NULL ) { GlobalFree( g_proxy_auth_username ); }
	if ( g_proxy_auth_password != NULL ) { GlobalFree( g_proxy_auth_password ); }
	if ( g_proxy_auth_key != NULL ) { GlobalFree( g_proxy_auth_key ); }

	if ( cfg_hostname_s != NULL ) { GlobalFree( cfg_hostname_s ); }
	if ( g_punycode_hostname_s != NULL ) { GlobalFree( g_punycode_hostname_s ); }

	if ( cfg_proxy_auth_username_s != NULL ) { GlobalFree( cfg_proxy_auth_username_s ); }
	if ( cfg_proxy_auth_password_s != NULL ) { GlobalFree( cfg_proxy_auth_password_s ); }
	if ( g_proxy_auth_username_s != NULL ) { GlobalFree( g_proxy_auth_username_s ); }
	if ( g_proxy_auth_password_s != NULL ) { GlobalFree( g_proxy_auth_password_s ); }
	if ( g_proxy_auth_key_s != NULL ) { GlobalFree( g_proxy_auth_key_s ); }

	// Server

	CleanupServerInfo();

	if ( cfg_server_hostname != NULL ) { GlobalFree( cfg_server_hostname ); }

	if ( cfg_certificate_pkcs_file_name != NULL ) { GlobalFree( cfg_certificate_pkcs_file_name ); }
	if ( cfg_certificate_pkcs_password != NULL ) { GlobalFree( cfg_certificate_pkcs_password ); }

	if ( cfg_certificate_cer_file_name != NULL ) { GlobalFree( cfg_certificate_cer_file_name ); }
	if ( cfg_certificate_key_file_name != NULL ) { GlobalFree( cfg_certificate_key_file_name ); }

	if ( cfg_authentication_username != NULL ) { GlobalFree( cfg_authentication_username ); }
	if ( cfg_authentication_password != NULL ) { GlobalFree( cfg_authentication_password ); }

	node_type *node = dllrbt_get_head( icon_handles );
	while ( node != NULL )
	{
		ICON_INFO *ii = ( ICON_INFO * )node->val;

		if ( ii != NULL )
		{
			DestroyIcon( ii->icon );
			GlobalFree( ii->file_extension );
			GlobalFree( ii );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( icon_handles );

	// Delete our font.
	_DeleteObject( hFont );

	if ( fail_type == 1 )
	{
		_MessageBoxA( NULL, "Call to _RegisterClassExW failed!", PROGRAM_CAPTION_A, MB_ICONWARNING );
	}
	else if ( fail_type == 2 )
	{
		_MessageBoxA( NULL, "Call to CreateWindow failed!", PROGRAM_CAPTION_A, MB_ICONWARNING );
	}

	DeleteCriticalSection( &context_list_cs );
	DeleteCriticalSection( &active_download_list_cs );
	DeleteCriticalSection( &download_queue_cs );
	DeleteCriticalSection( &cmessagebox_prompt_cs );
	DeleteCriticalSection( &file_size_prompt_list_cs );
	DeleteCriticalSection( &rename_file_prompt_list_cs );
	DeleteCriticalSection( &cleanup_cs );

	DeleteCriticalSection( &icon_cache_cs );

	DeleteCriticalSection( &session_totals_cs );

	DeleteCriticalSection( &worker_cs );

	ReleaseMutex( app_instance_mutex );
	CloseHandle( app_instance_mutex );

	// Delay loaded DLLs
	SSL_library_uninit();

	#ifndef WS2_32_USE_STATIC_LIB
		UnInitializeWS2_32();
	#else
		EndWS2_32();
	#endif
	#ifndef OLE32_USE_STATIC_LIB
		UnInitializeOle32();
	#endif
	#ifndef WINMM_USE_STATIC_LIB
		UnInitializeWinMM();
	#endif

UNLOAD_DLLS:

	#ifndef NORMALIZ_USE_STATIC_LIB
		UnInitializeNormaliz();
	#endif
	#ifndef ZLIB1_USE_STATIC_LIB
		UnInitializeZLib1();
	#endif
	#ifndef CRYPT32_USE_STATIC_LIB
		UnInitializeCrypt32();
	#endif
	#ifndef COMCTL32_USE_STATIC_LIB
		UnInitializeComCtl32();
	#endif
	#ifndef COMDLG32_USE_STATIC_LIB
		UnInitializeComDlg32();
	#endif
	#ifndef ADVAPI32_USE_STATIC_LIB
		UnInitializeAdvApi32();
	#endif
	#ifndef SHELL32_USE_STATIC_LIB
		UnInitializeShell32();
	#endif
	#ifndef GDI32_USE_STATIC_LIB
		UnInitializeGDI32();
	#endif
	#ifndef NTDLL_USE_STATIC_LIB
		UnInitializeNTDLL();
	#endif
	#ifndef USER32_USE_STATIC_LIB
		UnInitializeUser32();
	#endif

	#ifndef NTDLL_USE_STATIC_LIB
		ExitProcess( ( UINT )msg.wParam );
	#endif
	return ( int )msg.wParam;
}
