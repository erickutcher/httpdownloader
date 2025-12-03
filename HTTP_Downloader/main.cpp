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

#include "globals.h"
#include "wnd_proc.h"

#include "utilities.h"

#include "file_operations.h"
#include "string_tables.h"

#include "lite_kernel32.h"
#include "lite_shell32.h"
#include "lite_advapi32.h"
#include "lite_comdlg32.h"
#include "lite_crypt32.h"
#include "lite_comdlg32.h"
#include "lite_comctl32.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"
#include "lite_ole32.h"
#include "lite_winmm.h"
#include "lite_zlib1.h"
#include "lite_powrprof.h"
#include "lite_normaliz.h"
#include "lite_pcre2.h"
#include "lite_psftp.h"
#include "lite_libssl.h"
#include "lite_libcrypto.h"

#include "treelistview.h"
#include "cmessagebox.h"

#include "connection.h"
#include "site_manager_utilities.h"
#include "ftp_parsing.h"
#include "sftp.h"
#include "categories.h"

#include "system_tray.h"

#include "dark_mode.h"

//#define USE_DEBUG_DIRECTORY

#ifdef USE_DEBUG_DIRECTORY
	#define BASE_DIRECTORY_FLAG CSIDL_APPDATA
#else
	#define BASE_DIRECTORY_FLAG CSIDL_LOCAL_APPDATA
#endif

SYSTEMTIME g_compile_time;

HANDLE g_downloader_ready_semaphore = NULL;

CRITICAL_SECTION worker_cs;				// Worker thread critical section.

CRITICAL_SECTION session_totals_cs;

CRITICAL_SECTION icon_cache_cs;

// Object variables
HWND g_hWnd_main = NULL;		// Handle to our main window.

HWND g_hWnd_active = NULL;		// Handle to the active window. Used to handle tab stops.

LOGFONT g_default_log_font;
HFONT g_hFont = NULL;			// Handle to our font object.

UINT CF_HTML = 0;
UINT CF_TREELISTVIEW = 0;

COLORREF g_CustColors[ 16 ];

int g_default_row_height = 0;

wchar_t *g_base_directory = NULL;
unsigned int g_base_directory_length = 0;

wchar_t *g_program_directory = NULL;
unsigned int g_program_directory_length = 0;

dllrbt_tree *g_icon_handles = NULL;

dllrbt_tree *g_site_info = NULL;
dllrbt_tree *g_sftp_fps_host_info = NULL;
dllrbt_tree *g_sftp_keys_host_info = NULL;
dllrbt_tree *g_category_info = NULL;

bool g_can_fast_allocate = false;

bool g_is_windows_8_or_higher = false;
bool g_can_use_tls_1_3 = false;

bool g_use_openssl = false;

bool g_can_perform_shutdown_action = false;
bool g_perform_shutdown_action = false;

bool g_clean_tray_icon = false;

#ifndef NTDLL_USE_STATIC_LIB
int APIENTRY _WinMain()
#else
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif
{
	MSG msg;
	msg.wParam = 0;	// Exit code.

	#ifndef NTDLL_USE_STATIC_LIB
		HINSTANCE hInstance = GetModuleHandleW( NULL );
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
		if ( !InitializeComDlg32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef COMCTL32_USE_STATIC_LIB
		if ( !InitializeComCtl32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef CRYPT32_USE_STATIC_LIB
		if ( !InitializeCrypt32() ){ goto UNLOAD_DLLS; }
	#endif
	#ifndef ZLIB1_USE_STATIC_LIB
		if ( !InitializeZLib1() )
		{
			UnInitializeZLib1();

			/*if ( _MessageBoxW( NULL, L"The zlib compression library (zlib1.dll) could not be loaded.\r\n\r\nCompressed downloads will need to be manually decompressed.\r\n\r\nWould you like to visit www.zlib.net to download the DLL file?", PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES )
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
			}*/
		}
	#endif
	#ifndef POWRPROF_USE_STATIC_LIB
		if ( !InitializePowrProf() )
		{
			UnInitializePowrProf();
		}
	#endif
	#ifndef NORMALIZ_USE_STATIC_LIB
		if ( !InitializeNormaliz() )
		{
			UnInitializeNormaliz();
		}
	#endif
	#ifndef PCRE2_USE_STATIC_LIB
		if ( !InitializePCRE2() )
		{
			UnInitializePCRE2();
		}
	#endif
	#ifndef PSFTP_USE_STATIC_LIB
		if ( !InitializePSFTP() )
		{
			UnInitializePSFTP();
		}
	#endif
	// Loaded only for SetFileInformationByHandle, GetUserDefaultLocaleName, and GetQueuedCompletionStatusEx.
	// If SetFileInformationByHandle doesn't exist (on Windows XP), then rename won't work when the file is in use.
	// But at least the program will run.
	#ifndef KERNEL32_USE_STATIC_LIB
		if ( !InitializeKernel32() )
		{
			UnInitializeKernel32();
		}
	#endif

	if ( !InitializeDPIFunctions() )
	{
		UnInitializeDPIFunctions();
	}

#ifdef ENABLE_LOGGING
	InitLogging();
#endif

	_memzero( &g_compile_time, sizeof( SYSTEMTIME ) );
	if ( hInstance != NULL )
	{
		IMAGE_DOS_HEADER *idh = ( IMAGE_DOS_HEADER * )hInstance;
		IMAGE_NT_HEADERS *inth = ( IMAGE_NT_HEADERS * )( ( BYTE * )idh + idh->e_lfanew );

		UnixTimeToSystemTime( inth->FileHeader.TimeDateStamp, &g_compile_time );
	}

	unsigned char fail_type = 0;

	CL_ARGS *cla = NULL;

	g_base_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
	if ( g_base_directory == NULL )
	{
		goto UNLOAD_DLLS;
	}

	g_program_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
	if ( g_program_directory != NULL )
	{
		g_program_directory_length = GetModuleFileNameW( NULL, g_program_directory, MAX_PATH );
		while ( g_program_directory_length != 0 && g_program_directory[ --g_program_directory_length ] != L'\\' );
		g_program_directory[ g_program_directory_length ] = 0;	// Sanity.
	}

	bool override_shutdown_action = false;

	bool load_openssl = true;	// Load the OpenSSL/BoringSSL DLLs.

	// Get the new base directory if the user supplied a path.
	bool default_directory = true;
	int argCount = 0;
	LPWSTR *szArgList = _CommandLineToArgvW( GetCommandLineW(), &argCount );
	if ( szArgList != NULL )
	{
		if ( argCount > 1 )
		{
			cla = ( CL_ARGS * )GlobalAlloc( GPTR, sizeof( CL_ARGS ) );
		}

		// The first parameter (index 0) is the path to the executable.
		for ( int arg = 1; arg < argCount; ++arg )
		{
			if ( szArgList[ arg ][ 0 ] == L'-' && szArgList[ arg ][ 1 ] == L'-' )
			{
				wchar_t *arg_name = szArgList[ arg ] + 2;
				int arg_name_length = lstrlenW( arg_name );

				if ( ( arg + 1 ) < argCount &&
					 arg_name_length == 14 && _StrCmpNIW( arg_name, L"base-directory", 14 ) == 0 )	// Set the base directory.
				{
					++arg;	// Move to the supplied directory.

					DWORD gfa = GetFileAttributesW( szArgList[ arg ] );
					if ( gfa != INVALID_FILE_ATTRIBUTES && ( gfa & FILE_ATTRIBUTE_DIRECTORY ) )
					{
						g_base_directory_length = lstrlenW( szArgList[ arg ] );
						if ( g_base_directory_length >= MAX_PATH )
						{
							g_base_directory_length = MAX_PATH - 1;
						}
						_wmemcpy_s( g_base_directory, MAX_PATH, szArgList[ arg ], g_base_directory_length );
						g_base_directory[ g_base_directory_length ] = 0;	// Sanity.

						default_directory = false;
					}
				}
				else if ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"portable", 8 ) == 0 )	// Portable mode (use the application's current directory for our base directory).
				{
					g_base_directory_length = lstrlenW( szArgList[ 0 ] );
					while ( g_base_directory_length != 0 && szArgList[ 0 ][ --g_base_directory_length ] != L'\\' );

					_wmemcpy_s( g_base_directory, MAX_PATH, szArgList[ 0 ], g_base_directory_length );
					g_base_directory[ g_base_directory_length ] = 0;	// Sanity.

					default_directory = false;
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 5 && _StrCmpNIW( arg_name, L"parts", 5 ) == 0 )	// Split download into parts.
				{
					++arg;

					unsigned char parts = ( unsigned char )_wcstoul( szArgList[ arg ], NULL, 10 );
					if ( parts > 100 )
					{
						parts = 100;
					}
					else if ( parts == 0 )
					{
						parts = 1;
					}

					cla->parts = parts;
					cla->use_parts = true;
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 11 && _StrCmpNIW( arg_name, L"speed-limit", 11 ) == 0 )	// Download speed limit.
				{
					++arg;

					cla->download_speed_limit = ( unsigned long long )wcstoull( szArgList[ arg ] );
					cla->use_download_speed_limit = true;
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 10 && _StrCmpNIW( arg_name, L"encryption", 10 ) == 0 )	// SSL / TLS version.
				{
					++arg;

					// We validate this after we know whether TLS 1.3 is supported.
					cla->ssl_version = ( unsigned char )_wcstoul( szArgList[ arg ], NULL, 10 );
				}
				else if ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"simulate", 8 ) == 0 )	// Simulate the download.
				{
					cla->download_operations |= DOWNLOAD_OPERATION_SIMULATE;
				}
				else if ( arg_name_length == 11 && _StrCmpNIW( arg_name, L"add-stopped", 11 ) == 0 )	// Add the download in the Stopped state.
				{
					cla->download_operations &= ~DOWNLOAD_OPERATION_VERIFY;
					cla->download_operations |= DOWNLOAD_OPERATION_ADD_STOPPED;
				}
				else if ( arg_name_length == 6 && _StrCmpNIW( arg_name, L"verify", 6 ) == 0 )	// Verify the URL can be downloaded.
				{
					cla->download_operations &= ~DOWNLOAD_OPERATION_ADD_STOPPED;
					cla->download_operations |= DOWNLOAD_OPERATION_VERIFY;
				}
				else if ( arg_name_length == 9 && _StrCmpNIW( arg_name, L"immediate", 9 ) == 0 )	// Download immediately.
				{
					cla->download_immediately = 1;
				}
				else if ( arg_name_length == 9 && _StrCmpNIW( arg_name, L"clipboard", 9 ) == 0 )	// Use any URL(s) on the clipboard.
				{
					cla->use_clipboard = true;
				}
				else if ( ( arg + 1 ) < argCount &&
						  ( arg_name_length == 3 && _StrCmpNIW( arg_name, L"url", 3 ) == 0 ) ||
						  ( arg_name_length == 12 && _StrCmpNIW( arg_name, L"header-field", 12 ) == 0 ) )	// A URL or header field was supplied.
				{
					wchar_t **cl_val = NULL;
					int *cl_val_length = NULL;

					if ( *arg_name == L'u' )
					{
						cl_val = &cla->urls;
						cl_val_length = &cla->urls_length;
					}
					else	// Header field.
					{
						cl_val = &cla->headers;
						cl_val_length = &cla->headers_length;
					}

					++arg;	// Move to the supplied value.

					int length = lstrlenW( szArgList[ arg ] );
					if ( length > 0 )
					{
						if ( *cl_val == NULL )
						{
							*cl_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( length + 2 + 1 ) );
							_wmemcpy_s( *cl_val + *cl_val_length, length + 1, szArgList[ arg ], length );
							*cl_val_length += length;
							( *cl_val )[ ( *cl_val_length )++ ] = L'\r';
							( *cl_val )[ ( *cl_val_length )++ ] = L'\n';
						}
						else
						{
							wchar_t *realloc_buffer = ( wchar_t * )GlobalReAlloc( *cl_val, sizeof( wchar_t ) * ( *cl_val_length + length + 2 + 1 ), GMEM_MOVEABLE );
							if ( realloc_buffer != NULL )
							{
								*cl_val = realloc_buffer;

								_wmemcpy_s( *cl_val + *cl_val_length, length + 1, szArgList[ arg ], length );
								*cl_val_length += length;
								( *cl_val )[ ( *cl_val_length )++ ] = L'\r';
								( *cl_val )[ ( *cl_val_length )++ ] = L'\n';
							}
						}

						( *cl_val )[ *cl_val_length ] = 0;	// Sanity.
					}
				}
				else if ( ( arg + 1 ) < argCount &&
						  ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"comments", 8 ) == 0 ) ||
						  ( arg_name_length == 13 && _StrCmpNIW( arg_name, L"cookie-string", 13 ) == 0 ) ||
						  ( arg_name_length == 9 && _StrCmpNIW( arg_name, L"post-data", 9 ) == 0 ) ||
						  ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"category", 8 ) == 0 ) ||
						  ( arg_name_length == 18 && _StrCmpNIW( arg_name, L"download-directory", 18 ) == 0 ) ||
						  ( arg_name_length == 16 && _StrCmpNIW( arg_name, L"download-history", 16 ) == 0 ) ||
						  ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"url-list", 8 ) == 0 ) ||
						  ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"username", 8 ) == 0 ) ||
						  ( arg_name_length == 8 && _StrCmpNIW( arg_name, L"password", 8 ) == 0 ) )	// A cookie string, form (POST) data, URL list file, output (download) directory, username, and or password was supplied.
				{
					wchar_t **cl_val = NULL;

					int length = lstrlenW( szArgList[ arg + 1 ] );
					if ( length > 0 )
					{
						if ( *arg_name == L'c' )
						{
							if ( arg_name[ 1 ] == L'a' )	// Category
							{
								cl_val = &cla->category;
								cla->category_length = length;
							}
							else
							{
								if ( arg_name[ 2 ] == L'o' )	// Cookies
								{
									cl_val = &cla->cookies;
									cla->cookies_length = length;
								}
								else	// Comments
								{
									cl_val = &cla->comments;
									cla->comments_length = length;
								}
							}
							
						}
						else if ( arg_name[ 0 ] == L'p' )
						{
							if ( arg_name[ 1 ] == L'o' )	// Post Data
							{
								cl_val = &cla->data;
								cla->data_length = length;
							}
							else	// Password
							{
								cl_val = &cla->password;
								cla->password_length = length;
							}
						}
						else if ( *arg_name == L'd' )
						{
							if ( arg_name_length == 18 )	// Download Directory
							{
								if ( CreateDirectoriesW( szArgList[ arg + 1 ], NULL ) != FALSE )
								{
									// Remove any trailing slash.
									while ( length > 0 )
									{
										if ( szArgList[ arg + 1 ][ length - 1 ] == L'\\' )
										{
											--length;
										}
										else
										{
											break;
										}
									}

									cl_val = &cla->download_directory;
									cla->download_directory_length = length;
									cla->use_download_directory = true;
								}
								else
								{
									++arg;

									continue;
								}
							}
							else// if ( arg_name_length == 16 )	// Download History
							{
								cl_val = &cla->download_history_file;
								cla->download_history_file_length = length;
							}
						}
						else if ( arg_name[ 0 ] == L'u' )
						{
							if ( arg_name[ 1 ] == L'r' )	// URL
							{
								cl_val = &cla->url_list_file;
								cla->url_list_file_length = length;
							}
							else	// Username
							{
								cl_val = &cla->username;
								cla->username_length = length;
							}
						}

						++arg;	// Move to the supplied value.

						if ( *cl_val != NULL )
						{
							GlobalFree( *cl_val );
							*cl_val = NULL;
						}

						*cl_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( length + 1 ) );
						_wmemcpy_s( *cl_val, length + 1, szArgList[ arg ], length );
						( *cl_val )[ length ] = 0;	// Sanity.
					}
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 10 && _StrCmpNIW( arg_name, L"proxy-type", 10 ) == 0 )	// Proxy type.
				{
					++arg;

					cla->proxy_type = ( unsigned char )_wcstoul( szArgList[ arg ], NULL, 10 );
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 16 && _StrCmpNIW( arg_name, L"proxy-ip-address", 16 ) == 0 )	// Proxy IP address.
				{
					++arg;

					unsigned int proxy_ip_address = 0;

					wchar_t *ipaddr = szArgList[ arg ];

					for ( char i = 3; i >= 0; --i )
					{
						wchar_t *ptr = ipaddr;
						while ( ptr != NULL && *ptr != NULL )
						{
							if ( *ptr == L'.' )
							{
								*ptr = 0;
								++ptr;

								break;
							}

							++ptr;
						}

						if ( *ptr == NULL && i > 0 )
						{
							break;
						}

						proxy_ip_address |= ( ( unsigned int )_wcstoul( ipaddr, NULL, 10 ) << ( 8 * i ) );

						if ( *ptr == NULL && i == 0 )
						{
							cla->proxy_ip_address = proxy_ip_address;
						}
						else
						{
							ipaddr = ptr;
						}
					}
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 10 && _StrCmpNIW( arg_name, L"proxy-port", 10 ) == 0 )	// Proxy port.
				{
					++arg;

					cla->proxy_port = ( unsigned short )_wcstoul( szArgList[ arg ], NULL, 10 );
				}
				else if ( arg_name_length == 26 && _StrCmpNIW( arg_name, L"proxy-resolve-domain-names", 26 ) == 0 )	// Resolve domain names.
				{
					cla->proxy_resolve_domain_names = true;
				}
				else if ( ( arg + 1 ) < argCount &&
						  ( arg_name_length == 14 && _StrCmpNIW( arg_name, L"proxy-hostname", 14 ) == 0 ) ||
						  ( arg_name_length == 14 && _StrCmpNIW( arg_name, L"proxy-username", 14 ) == 0 ) ||
						  ( arg_name_length == 14 && _StrCmpNIW( arg_name, L"proxy-password", 14 ) == 0 ) )	// Proxy hostname, username, and password.
				{
					wchar_t **cl_val = NULL;

					int length = lstrlenW( szArgList[ arg + 1 ] );
					if ( length > 0 )
					{
						if ( arg_name[ 6 ] == L'h' )
						{
							cl_val = &cla->proxy_hostname;
							cla->proxy_hostname_length = length;
						}
						else if ( arg_name[ 6 ] == L'u' )
						{
							cl_val = &cla->proxy_username;
							cla->proxy_username_length = length;
						}
						else if ( arg_name[ 6 ] == L'p' )
						{
							cl_val = &cla->proxy_password;
							cla->proxy_password_length = length;
						}

						++arg;	// Move to the supplied value.

						if ( *cl_val != NULL )
						{
							GlobalFree( *cl_val );
							*cl_val = NULL;
						}

						*cl_val = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( length + 1 ) );
						_wmemcpy_s( *cl_val, length + 1, szArgList[ arg ], length );
						( *cl_val )[ length ] = 0;	// Sanity.
					}
				}
				else if ( ( arg + 1 ) < argCount &&
						  arg_name_length == 15 && _StrCmpNIW( arg_name, L"shutdown-action", 15 ) == 0 )	// Shutdown action.
				{
					++arg;

					override_shutdown_action = true;

					g_shutdown_action = ( unsigned char )_wcstoul( szArgList[ arg ], NULL, 10 );
				}
				else if ( arg_name_length == 12 && _StrCmpNIW( arg_name, L"allow-rename", 12 ) == 0 )	// Rename/delete active files.
				{
					g_allow_rename = true;
				}
				else if ( arg_name_length == 15 && _StrCmpNIW( arg_name, L"disable-openssl", 15 ) == 0 )	// Don't use OpenSSL/BoringSSL functions.
				{
					load_openssl = false;
				}
#ifdef ENABLE_LOGGING
				else if ( ( arg + 1 ) < argCount &&
					 arg_name_length == 3 && _StrCmpNIW( arg_name, L"log", 3 ) == 0 )	// Log file.
				{
					++arg;

					unsigned int log_filter = 0;
					wchar_t *ptr = szArgList[ arg ];
					while ( ptr != NULL && *ptr != NULL )
					{
						if ( *ptr >= L'0' && *ptr <= L'9' )
						{
							unsigned char log_type = ( unsigned char )( *ptr - L'0' );
							switch ( log_type )
							{
								case 1: { log_filter |= LOG_INFO_MISC; } break;
								case 2: { log_filter |= LOG_INFO_ACTION; } break;
								case 3: { log_filter |= LOG_INFO_CON_STATE; } break;
								case 4: { log_filter |= LOG_WARNING; } break;
								case 5: { log_filter |= LOG_ERROR; } break;
							}
						}
						++ptr;
					}

					++arg;	// Move to the supplied filepath.

					OpenLog( szArgList[ arg ], log_filter );
				}
#endif
			}
			else
			{
				int file_length = lstrlenW( szArgList[ arg ] );
				unsigned int extension_offset = get_file_extension_offset( szArgList[ arg ], file_length );

				if ( ( file_length - extension_offset ) == 4 && _StrCmpNIW( szArgList[ arg ] + extension_offset, L".hdh", 4 ) == 0 )
				{
					if ( cla->download_history_file != NULL )
					{
						GlobalFree( cla->download_history_file );
						cla->download_history_file = NULL;
					}

					cla->download_history_file = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( file_length + 1 ) );
					_wmemcpy_s( cla->download_history_file, file_length + 1, szArgList[ arg ], file_length );
					cla->download_history_file[ file_length ] = 0;	// Sanity.

					cla->download_history_file_length = file_length;
				}
			}
		}

		// Free the parameter list.
		LocalFree( szArgList );
	}

	if ( load_openssl )
	{
		#ifndef LIBSSL_USE_STATIC_LIB
			unsigned char ssl_type = 0;	// 0 = BoringSSL, 1 = OpenSSL
			g_use_openssl = InitializeLibSSL( L"ssl.dll" );
			if ( !g_use_openssl )
			{
#ifdef _WIN64
				g_use_openssl = InitializeLibSSL( L"libssl-3-x64.dll" );
#else
				g_use_openssl = InitializeLibSSL( L"libssl-3.dll" );
#endif
				if ( !g_use_openssl )
				{
					UnInitializeLibSSL();
				}
				else
				{
					ssl_type = 1;
				}
			}
		#else
			// Set the type to whatever is statically linked.
			unsigned char ssl_type = 0;	// 0 = BoringSSL (default), 1 = OpenSSL
			g_use_openssl = true;
		#endif
		#ifndef LIBCRYPTO_USE_STATIC_LIB
			if ( g_use_openssl )
			{
				if ( ssl_type == 0 )
				{
					g_use_openssl = InitializeLibCrypto( L"crypto.dll" );
				}
				else if ( ssl_type == 1 )
				{
#ifdef _WIN64
					g_use_openssl = InitializeLibCrypto( L"libcrypto-3-x64.dll" );
#else
					g_use_openssl = InitializeLibCrypto( L"libcrypto-3.dll" );
#endif
				}

				if ( !g_use_openssl )
				{
					UnInitializeLibCrypto();
					UnInitializeLibSSL();
				}
			}
		#endif
	}

#ifdef ENABLE_DARK_MODE
	if ( IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN10 ), LOBYTE( _WIN32_WINNT_WIN10 ), 0 ) )
	{
		wchar_t dm_file_path[ MAX_PATH ];
		_wmemcpy_s( dm_file_path, MAX_PATH, g_program_directory, g_program_directory_length );
		g_base_directory_length = g_program_directory_length;
		_wmemcpy_s( dm_file_path + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\dark_mode\0", 11 );
		//dm_file_path[ g_base_directory_length + 10 ] = 0;	// Sanity.

		// See if the user wants to run the program with dark mode support.
		if ( GetFileAttributesW( dm_file_path ) != INVALID_FILE_ATTRIBUTES )
		{
			g_use_dark_mode = InitDarkMode();
		}
	}
#endif

	// Use our default directory if none was supplied or check if there's a "portable" file in the same directory.
	if ( default_directory )
	{
		//g_base_directory_length = GetModuleFileNameW( NULL, g_base_directory, MAX_PATH );
		//while ( g_base_directory_length != 0 && g_base_directory[ --g_base_directory_length ] != L'\\' );
		//g_base_directory[ g_base_directory_length ] = 0;	// Sanity.
		_wmemcpy_s( g_base_directory, MAX_PATH, g_program_directory, g_program_directory_length );
		g_base_directory_length = g_program_directory_length;
		_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\portable\0", 10 );

		// If there's a portable file in the same directory, then we'll use that directory as our base.
		// If not, then we'll use the APPDATA folder.
		if ( GetFileAttributesW( g_base_directory ) == INVALID_FILE_ATTRIBUTES )
		{
			_SHGetFolderPathW( NULL, BASE_DIRECTORY_FLAG, NULL, 0, g_base_directory );

			g_base_directory_length = lstrlenW( g_base_directory );
			_wmemcpy_s( g_base_directory + g_base_directory_length, MAX_PATH - g_base_directory_length, L"\\HTTP Downloader\0", 17 );
			g_base_directory_length += 16;
			//g_base_directory[ g_base_directory_length ] = 0;	// Sanity.

			// Check to see if the new path exists and create it if it doesn't.
			if ( GetFileAttributesW( g_base_directory ) == INVALID_FILE_ATTRIBUTES )
			{
				CreateDirectoryW( g_base_directory, NULL );
			}
		}
		/*else
		{
			g_base_directory[ g_base_directory_length ] = 0;	// Sanity.
		}*/
	}

	// Check to see if the new path exists and create it if it doesn't.
	if ( GetFileAttributesW( g_base_directory ) == INVALID_FILE_ATTRIBUTES )
	{
		CreateDirectoryW( g_base_directory, NULL );
	}

	InitializeCriticalSection( &worker_cs );

	InitializeCriticalSection( &session_totals_cs );

	InitializeCriticalSection( &icon_cache_cs );

	InitializeCriticalSection( &ftp_listen_info_cs );

	InitializeCriticalSection( &context_list_cs );
	InitializeCriticalSection( &active_download_list_cs );
	InitializeCriticalSection( &download_queue_cs );
	InitializeCriticalSection( &cmessagebox_prompt_cs );
	InitializeCriticalSection( &file_size_prompt_list_cs );
	InitializeCriticalSection( &rename_file_prompt_list_cs );
	InitializeCriticalSection( &last_modified_prompt_list_cs );
	InitializeCriticalSection( &fingerprint_prompt_list_cs );
	InitializeCriticalSection( &move_file_queue_cs );
	InitializeCriticalSection( &cleanup_cs );
	InitializeCriticalSection( &update_check_timeout_cs );
	InitializeCriticalSection( &file_allocation_cs );

	// Get the default message system font.
	NONCLIENTMETRICS ncm;
	_memzero( &ncm, sizeof( NONCLIENTMETRICS ) );
	if ( IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 ) )
	{
		ncm.cbSize = sizeof( NONCLIENTMETRICS );
	}
	else	// XP and earlier.
	{
		// https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-nonclientmetricsa
		ncm.cbSize = sizeof( NONCLIENTMETRICS ) - sizeof( ncm.iPaddedBorderWidth );
	}
	_SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 );

	//g_default_log_font = ncm.lfMessageFont;
	_memcpy_s( &g_default_log_font, sizeof( LOGFONT ), &ncm.lfMessageFont, sizeof( LOGFONT ) );

	// Set our global font to the LOGFONT value obtained from the system.
	g_hFont = _CreateFontIndirectW( &ncm.lfMessageFont );

	// Get the row height for our listview control.
	TEXTMETRIC tm;
	HDC hDC = _GetDC( NULL );
	HFONT ohf = ( HFONT )_SelectObject( hDC, g_hFont );
	_GetTextMetricsW( hDC, &tm );
	_SelectObject( hDC, ohf );	// Reset old font.
	_ReleaseDC( NULL, hDC );

	g_default_row_height = tm.tmHeight + tm.tmExternalLeading + 5;

	int icon_height = _GetSystemMetrics( SM_CYSMICON ) + 2;
	if ( g_default_row_height < icon_height )
	{
		g_default_row_height = icon_height;
	}

	_memset( g_CustColors, 0, sizeof( COLORREF ) * 16 );

	SetDefaultAppearance();

	_wmemcpy_s( g_program_directory + g_program_directory_length, MAX_PATH - g_program_directory_length, L"\\main.ico\0", 10 );
	if ( g_program_directory != NULL && GetFileAttributesW( g_program_directory ) != INVALID_FILE_ATTRIBUTES )
	{
		g_clean_tray_icon = true;
		g_default_tray_icon = ( HICON )_LoadImageW( GetModuleHandleW( NULL ), g_program_directory, IMAGE_ICON, 16, 16, LR_LOADFROMFILE );
	}
	else
	{
		g_default_tray_icon = ( HICON )_LoadImageW( GetModuleHandleW( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
	}

	// Default position if no settings were saved.
	cfg_pos_x = MAXINT;//( ( _GetSystemMetrics( SM_CXSCREEN ) - MIN_WIDTH ) / 2 );
	cfg_pos_y = MAXINT;//( ( _GetSystemMetrics( SM_CYSCREEN ) - MIN_HEIGHT ) / 2 );

	cfg_drop_pos_x = MAXINT;//( ( _GetSystemMetrics( SM_CXSCREEN ) - DW_WIDTH ) / 2 );
	cfg_drop_pos_y = MAXINT;//( ( _GetSystemMetrics( SM_CYSCREEN ) - DW_HEIGHT ) / 2 );

	SYSTEM_INFO systemInfo;
	GetSystemInfo( &systemInfo );

	if ( systemInfo.dwNumberOfProcessors > 0 )
	{
		g_max_threads = systemInfo.dwNumberOfProcessors * 2;	// Default is 2.
		cfg_thread_count = systemInfo.dwNumberOfProcessors;		// Default is 1.
	}

	CF_HTML = _RegisterClipboardFormatW( L"HTML Format" );
	CF_TREELISTVIEW = _RegisterClipboardFormatW( L"TreeListView Format" );

	g_is_windows_8_or_higher = IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN8 ), LOBYTE( _WIN32_WINNT_WIN8 ), 0 );

	g_can_use_tls_1_3 = ( g_use_openssl || IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_WIN_SERVER_2022 ), LOBYTE( _WIN32_WINNT_WIN_SERVER_2022 ), _WIN32_WINNT_WIN_SERVER_2022_BUILD ) );

	InitializeLocaleValues();

	read_config();

	if ( !override_shutdown_action )
	{
		g_shutdown_action = cfg_shutdown_action;
	}

	if ( cla != NULL )
	{
		if ( g_can_use_tls_1_3 && cla->ssl_version >= 6 ) { cla->ssl_version = 6; }	// TLS 1.3
		else if ( cla->ssl_version > 5 ) { cla->ssl_version = 5; }					// TLS 1.2
	}

	// See if there's an instance of the program running.
	HANDLE app_instance_mutex = CreateMutexW( NULL, 0, PROGRAM_CAPTION );
	if ( app_instance_mutex == NULL )
	{
		goto CLEANUP;
	}
	else	// There's already an instance of the program running.
	{
		if ( GetLastError() == ERROR_ALREADY_EXISTS && cfg_use_one_instance )
		{
			HWND hWnd_instance = _FindWindowW( L"class_http_downloader", NULL );
			if ( hWnd_instance != NULL )
			{
				// If we're passing command-line values, then the Add URLs window will take focus when the copy data is received.
				// If we're not passing command-line values, then show the main window.
				if ( cla != NULL )
				{
					unsigned int data_offset = 0;
					unsigned int data_size = sizeof( CL_ARGS );
					if ( cla->category_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->category_length + 1 ) ); }
					if ( cla->download_directory > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->download_directory_length + 1 ) ); }
					if ( cla->download_history_file > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->download_history_file_length + 1 ) ); }
					if ( cla->comments_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->comments_length + 1 ) ); }
					if ( cla->cookies_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->cookies_length + 1 ) ); }
					if ( cla->headers_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->headers_length + 1 ) ); }
					if ( cla->data_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->data_length + 1 ) ); }
					if ( cla->url_list_file_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->url_list_file_length + 1 ) ); }
					if ( cla->urls_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->urls_length + 1 ) ); }
					if ( cla->username_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->username_length + 1 ) ); }
					if ( cla->password_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->password_length + 1 ) ); }
					if ( cla->proxy_hostname_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->proxy_hostname_length + 1 ) ); }
					if ( cla->proxy_username_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->proxy_username_length + 1 ) ); }
					if ( cla->proxy_password_length > 0 ) { data_size += ( sizeof( wchar_t ) * ( cla->proxy_password_length + 1 ) ); }

					// Make a contiguous memory buffer.
					CL_ARGS *cla_data = ( CL_ARGS * )GlobalAlloc( GMEM_FIXED, data_size );
					_memcpy_s( cla_data, sizeof( CL_ARGS ), cla, sizeof( CL_ARGS ) );

					wchar_t *wbyte_buf = ( wchar_t * )( ( char * )cla_data + sizeof( CL_ARGS ) );

					cla_data->category = ( wchar_t * )data_offset;
					if ( cla->category_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->category, cla->category_length + 1 );
						data_offset += ( cla->category_length + 1 );
					}

					cla_data->download_directory = ( wchar_t * )data_offset;
					if ( cla->download_directory_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->download_directory, cla->download_directory_length + 1 );
						data_offset += ( cla->download_directory_length + 1 );
					}

					cla_data->download_history_file = ( wchar_t * )data_offset;
					if ( cla->download_history_file_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->download_history_file, cla->download_history_file_length + 1 );
						data_offset += ( cla->download_history_file_length + 1 );
					}

					cla_data->url_list_file = ( wchar_t * )data_offset;
					if ( cla->url_list_file_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->url_list_file, cla->url_list_file_length + 1 );
						data_offset += ( cla->url_list_file_length + 1 );
					}

					cla_data->urls = ( wchar_t * )data_offset;
					if ( cla->urls_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->urls, cla->urls_length + 1 );
						data_offset += ( cla->urls_length + 1 );
					}

					cla_data->comments = ( wchar_t * )data_offset;
					if ( cla->comments_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->comments, cla->comments_length + 1 );
						data_offset += ( cla->comments_length + 1 );
					}

					cla_data->cookies = ( wchar_t * )data_offset;
					if ( cla->cookies_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->cookies, cla->cookies_length + 1 );
						data_offset += ( cla->cookies_length + 1 );
					}

					cla_data->headers = ( wchar_t * )data_offset;
					if ( cla->headers_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->headers, cla->headers_length + 1 );
						data_offset += ( cla->headers_length + 1 );
					}

					cla_data->data = ( wchar_t * )data_offset;
					if ( cla->data_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->data, cla->data_length + 1 );
						data_offset += ( cla->data_length + 1 );
					}

					cla_data->username = ( wchar_t * )data_offset;
					if ( cla->username_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->username, cla->username_length + 1 );
						data_offset += ( cla->username_length + 1 );
					}

					cla_data->password = ( wchar_t * )data_offset;
					if ( cla->password_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->password, cla->password_length + 1 );
						data_offset += ( cla->password_length + 1 );
					}

					cla_data->proxy_hostname = ( wchar_t * )data_offset;
					if ( cla->proxy_hostname_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->proxy_hostname, cla->proxy_hostname_length + 1 );
						data_offset += ( cla->proxy_hostname_length + 1 );
					}

					cla_data->proxy_username = ( wchar_t * )data_offset;
					if ( cla->proxy_username_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->proxy_username, cla->proxy_username_length + 1 );
						data_offset += ( cla->proxy_username_length + 1 );
					}

					cla_data->proxy_password = ( wchar_t * )data_offset;
					if ( cla->proxy_password_length > 0 )
					{
						_wmemcpy_s( wbyte_buf + data_offset, data_size - data_offset, cla->proxy_password, cla->proxy_password_length + 1 );
						data_offset += ( cla->proxy_password_length + 1 );
					}

					COPYDATASTRUCT cds;
					cds.dwData = 0;
					cds.cbData = data_size;
					cds.lpData = ( PVOID )cla_data;
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

	if ( cfg_enable_quick_allocation ||
		 g_shutdown_action == SHUTDOWN_ACTION_RESTART ||
		 g_shutdown_action == SHUTDOWN_ACTION_SLEEP ||
		 g_shutdown_action == SHUTDOWN_ACTION_HIBERNATE ||
		 g_shutdown_action == SHUTDOWN_ACTION_SHUT_DOWN ||
		 g_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN )
	{
		// Check if we're an admin or have elevated privileges. Allow us to pre-allocate a file without zeroing it.
		HANDLE hToken, hLinkedToken;
		TOKEN_PRIVILEGES tp;
		LUID luid;
		_memzero( &luid, sizeof( LUID ) );

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

			if ( is_privileged == TRUE )
			{
				if ( cfg_enable_quick_allocation && _LookupPrivilegeValueW( NULL, SE_MANAGE_VOLUME_NAME, &luid ) )
				{
					tp.PrivilegeCount = 1;
					tp.Privileges[ 0 ].Luid = luid;
					tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

					if ( _AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), NULL, NULL ) )
					{
						g_can_fast_allocate = true;
					}
				}

				if ( g_shutdown_action == SHUTDOWN_ACTION_RESTART ||
					 g_shutdown_action == SHUTDOWN_ACTION_SLEEP ||
					 g_shutdown_action == SHUTDOWN_ACTION_HIBERNATE ||
					 g_shutdown_action == SHUTDOWN_ACTION_SHUT_DOWN ||
					 g_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN &&
					 _LookupPrivilegeValueW( NULL, SE_SHUTDOWN_NAME, &luid ) )
				{
					tp.PrivilegeCount = 1;
					tp.Privileges[ 0 ].Luid = luid;
					tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

					if ( _AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), NULL, NULL ) )
					{
						g_can_perform_shutdown_action = true;
					}
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
			if ( ( INT_PTR )_ShellExecuteW( NULL, L"runas", command_line_path, tmp_command_line_path, NULL, SW_SHOWNORMAL ) > 32 )
			{
				GlobalFree( command_line_path );

				goto CLEANUP;
			}

			GlobalFree( command_line_path );
		}
	}

	if ( cfg_play_sound || cfg_play_sound_fail )
	{
		#ifndef WINMM_USE_STATIC_LIB
			InitializeWinMM();
		#endif
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

		if ( cfg_address_type_socks == 0 )
		{
			hostname_length = lstrlenW( cfg_hostname_socks ) + 1;	// Include the NULL terminator.
			punycode_length = _IdnToAscii( 0, cfg_hostname_socks, hostname_length, NULL, 0 );

			if ( punycode_length > hostname_length )
			{
				g_punycode_hostname_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
				_IdnToAscii( 0, cfg_hostname_socks, hostname_length, g_punycode_hostname_socks, punycode_length );
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

	if ( cfg_proxy_auth_username_socks != NULL )
	{
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_socks, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_socks, -1, g_proxy_auth_username_socks, auth_username_length, NULL, NULL ) - 1;
	}

	if ( cfg_proxy_auth_password_socks != NULL )
	{
		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_socks, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_password_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
		auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_socks, -1, g_proxy_auth_password_socks, auth_password_length, NULL, NULL ) - 1;
	}

	if ( cfg_proxy_auth_ident_username_socks != NULL )
	{
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_ident_username_socks, -1, NULL, 0, NULL, NULL );
		g_proxy_auth_ident_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
		auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_ident_username_socks, -1, g_proxy_auth_ident_username_socks, auth_username_length, NULL, NULL ) - 1;
	}

	g_icon_handles = dllrbt_create( dllrbt_compare_i_w );
	g_shared_categories = dllrbt_create( dllrbt_compare_w );
	g_category_file_extensions = dllrbt_create( dllrbt_compare_i_w );

	g_site_info = dllrbt_create( dllrbt_compare_site_info );
	g_sftp_fps_host_info = dllrbt_create( dllrbt_compare_sftp_fps_host_info );
	g_sftp_keys_host_info = dllrbt_create( dllrbt_compare_sftp_keys_host_info );
	g_category_info = dllrbt_create( dllrbt_compare_category_info );

	read_site_info();
	read_sftp_fps_host_info();
	read_sftp_keys_host_info();
	read_category_info();

	if ( psftp_state == PSFTP_STATE_RUNNING )
	{
		_SFTP_InitGSSAPI();

		_SFTP_SetConfigInfo( 0, ( cfg_sftp_enable_compression ? 1 : 0 ) );
		_SFTP_SetConfigInfo( 1, ( cfg_sftp_attempt_gssapi_authentication ? 1 : 0 ) );
		_SFTP_SetConfigInfo( 2, ( cfg_sftp_attempt_gssapi_key_exchange ? 1 : 0 ) );

		_SFTP_SetConfigInfo( 3, cfg_sftp_keep_alive_time );
		_SFTP_SetConfigInfo( 4, cfg_sftp_rekey_time );
		_SFTP_SetConfigInfo( 5, cfg_sftp_gss_rekey_time );
		_SFTP_SetConfigInfo( 6, cfg_sftp_rekey_data_limit );

		_SFTP_SetAlgorithmPriorities( 0, cfg_priority_kex_algorithm, KEX_ALGORITHM_COUNT );
		_SFTP_SetAlgorithmPriorities( 1, cfg_priority_host_key, HOST_KEY_COUNT );
		_SFTP_SetAlgorithmPriorities( 2, cfg_priority_encryption_cipher, ENCRYPTION_CIPHER_COUNT );
	}

#ifdef ENABLE_LOGGING
	WriteLog( ( zlib1_state == ZLIB1_STATE_RUNNING ? LOG_INFO_MISC : LOG_WARNING ), "zlib1.dll %s", ( zlib1_state == ZLIB1_STATE_RUNNING ? "loaded" : "not loaded" ) );
	WriteLog( ( psftp_state == PSFTP_STATE_RUNNING ? LOG_INFO_MISC : LOG_WARNING ), "psftp.dll %s", ( psftp_state == PSFTP_STATE_RUNNING ? "loaded" : "not loaded" ) );
	WriteLog( ( pcre2_state == PCRE2_STATE_RUNNING ? LOG_INFO_MISC : LOG_WARNING ), "libpcre2-16-0.dll %s", ( pcre2_state == PCRE2_STATE_RUNNING ? "loaded" : "not loaded" ) );
#endif

	g_downloader_ready_semaphore = CreateSemaphore( NULL, 0, 1, NULL );

	HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, IOCPDownloader, NULL, 0, NULL );
	if ( thread != NULL )
	{
		CloseHandle( thread );

		// Wait for IOCPDownloader to set up the completion port. 10 second timeout in case we miss the release.
		WaitForSingleObject( g_downloader_ready_semaphore, 10000 );
		CloseHandle( g_downloader_ready_semaphore );
		g_downloader_ready_semaphore = NULL;
	}
	else
	{
		CloseHandle( g_downloader_ready_semaphore );

		fail_type = 3;
		goto CLEANUP;
	}

	// Initialize our window class.
	WNDCLASSEX wcex;
	_memzero( &wcex, sizeof( WNDCLASSEX ) );
	wcex.cbSize			= sizeof( WNDCLASSEX );
	wcex.style			= 0;//CS_VREDRAW | CS_HREDRAW;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= g_default_tray_icon;//_LoadIconW( hInstance, MAKEINTRESOURCE( IDI_ICON ) );
	wcex.hCursor		= _LoadCursorW( NULL, IDC_ARROW );
	wcex.hbrBackground	= ( HBRUSH )( COLOR_3DFACE + 1 );
	wcex.lpszMenuName	= NULL;
	wcex.hIconSm		= NULL;

#ifdef ENABLE_DARK_MODE
	if ( g_use_dark_mode )
	{
		if ( g_dm_buildNumber < WINDOWS_BUILD_1909 )
		{
			_AllowDarkModeForApp( true );
		}
		else
		{
			_SetPreferredAppMode( ForceDark );
		}

		wcex.hbrBackground = g_hBrush_window_background;
	}
#endif

	// Since the main window's children cover it up, we don't need to redraw the window.
	// This also prevents the status bar child from flickering during a window resize. Dumb!
	wcex.lpfnWndProc	= MainWndProc;
	wcex.lpszClassName	= L"class_http_downloader";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

//	wcex.style			= CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc	= AddURLsWndProc;
	wcex.lpszClassName	= L"class_add_urls";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= OptionsWndProc;
	wcex.lpszClassName	= L"class_options";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= UpdateDownloadWndProc;
	wcex.lpszClassName	= L"class_update_download";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= SearchWndProc;
	wcex.lpszClassName	= L"class_search";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= DownloadSpeedLimitWndProc;
	wcex.lpszClassName	= L"class_download_speed_limit";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= AddCategoryWndProc;
	wcex.lpszClassName	= L"class_add_category";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= SiteManagerWndProc;
	wcex.lpszClassName	= L"class_site_manager";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= CheckForUpdatesWndProc;
	wcex.lpszClassName	= L"class_check_for_updates";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.hIcon			= NULL;

	wcex.lpfnWndProc	= AdvancedTabWndProc;
	wcex.lpszClassName	= L"class_advanced_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= ProxyTabWndProc;
	wcex.lpszClassName	= L"class_proxy_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= ConnectionTabWndProc;
	wcex.lpszClassName	= L"class_connection_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= AppearanceTabWndProc;
	wcex.lpszClassName	= L"class_appearance_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= GeneralTabWndProc;
	wcex.lpszClassName	= L"class_general_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= FTPTabWndProc;
	wcex.lpszClassName	= L"class_ftp_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= SFTPTabWndProc;
	wcex.lpszClassName	= L"class_sftp_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= SFTPFpsTabWndProc;
	wcex.lpszClassName	= L"class_sftp_fps_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= SFTPKeysTabWndProc;
	wcex.lpszClassName	= L"class_sftp_keys_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= WebServerTabWndProc;
	wcex.lpszClassName	= L"class_web_server_tab";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.style		   |= CS_DBLCLKS;
	wcex.lpfnWndProc	= URLDropWndProc;
	wcex.lpszClassName	= L"class_url_drop_window";

	if ( !_RegisterClassExW( &wcex ) )
	{
		fail_type = 1;
		goto CLEANUP;
	}

	wcex.lpfnWndProc	= TreeListViewWndProc;
	wcex.lpszClassName	= L"TreeListView";

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

	// WS_EX_COMPOSITED stops the menu bar and status bar from flickering when the window is resized.
	g_hWnd_main = _CreateWindowExW( ( cfg_always_on_top ? WS_EX_TOPMOST : 0 ) | ( g_is_windows_8_or_higher ? 0 : WS_EX_COMPOSITED ), L"class_http_downloader", PROGRAM_CAPTION, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, cfg_width, cfg_height, NULL, NULL, NULL, NULL );

	if ( !g_hWnd_main )
	{
		fail_type = 2;
		goto CLEANUP;
	}

	if ( !cfg_tray_icon || !cfg_start_in_tray )
	{
		_ShowWindow( g_hWnd_main, ( cfg_min_max == 1 ? SW_MINIMIZE : ( cfg_min_max == 2 ? SW_MAXIMIZE : SW_SHOWNORMAL ) ) );
	}

	if ( cla != NULL )
	{
		// Only load if there's URLs that have been supplied.
		if ( cla->url_list_file != NULL || cla->urls != NULL || cla->download_history_file != NULL || cla->use_clipboard )
		{
			// cla values will be freed here.
			_SendMessageW( g_hWnd_main, WM_PROPAGATE, ( WPARAM )-2, ( LPARAM )cla );

			cla = NULL;	// Make sure it's not freed below.
		}
		else
		{
			FreeCommandLineArgs( &cla );
		}
	}

	if ( cfg_enable_drop_window )
	{
		g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"class_url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP, 0, 0, DW_WIDTH, DW_HEIGHT, NULL, NULL, NULL, NULL );
		_SetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
		_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

		// Prevents it from stealing focus.
		_SetWindowPos( g_hWnd_url_drop_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER );
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

	if ( site_list_changed ) { save_site_info(); }
	if ( sftp_fps_host_list_changed ) { save_sftp_fps_host_info(); }
	if ( sftp_keys_host_list_changed ) { save_sftp_keys_host_info(); }
	if ( category_list_changed ) { save_category_info(); }

	FreeCommandLineArgs( &cla );

	if ( g_base_directory != NULL ) { GlobalFree( g_base_directory ); }
	if ( g_program_directory != NULL ) { GlobalFree( g_program_directory ); }
	if ( cfg_default_download_directory != NULL ) { GlobalFree( cfg_default_download_directory ); }
	if ( cfg_temp_download_directory != NULL ) { GlobalFree( cfg_temp_download_directory ); }
	if ( cfg_sound_file_path != NULL ) { GlobalFree( cfg_sound_file_path ); }
	if ( cfg_sound_fail_file_path != NULL ) { GlobalFree( cfg_sound_fail_file_path ); }

	// FTP
	if ( cfg_ftp_hostname != NULL ) { GlobalFree( cfg_ftp_hostname ); }

	// HTTP proxy
	if ( cfg_hostname != NULL ) { GlobalFree( cfg_hostname ); }
	if ( g_punycode_hostname != NULL ) { GlobalFree( g_punycode_hostname ); }

	if ( cfg_proxy_auth_username != NULL ) { GlobalFree( cfg_proxy_auth_username ); }
	if ( cfg_proxy_auth_password != NULL ) { GlobalFree( cfg_proxy_auth_password ); }
	if ( g_proxy_auth_username != NULL ) { GlobalFree( g_proxy_auth_username ); }
	if ( g_proxy_auth_password != NULL ) { GlobalFree( g_proxy_auth_password ); }
	if ( g_proxy_auth_key != NULL ) { GlobalFree( g_proxy_auth_key ); }

	// HTTPS proxy
	if ( cfg_hostname_s != NULL ) { GlobalFree( cfg_hostname_s ); }
	if ( g_punycode_hostname_s != NULL ) { GlobalFree( g_punycode_hostname_s ); }

	if ( cfg_proxy_auth_username_s != NULL ) { GlobalFree( cfg_proxy_auth_username_s ); }
	if ( cfg_proxy_auth_password_s != NULL ) { GlobalFree( cfg_proxy_auth_password_s ); }
	if ( g_proxy_auth_username_s != NULL ) { GlobalFree( g_proxy_auth_username_s ); }
	if ( g_proxy_auth_password_s != NULL ) { GlobalFree( g_proxy_auth_password_s ); }
	if ( g_proxy_auth_key_s != NULL ) { GlobalFree( g_proxy_auth_key_s ); }

	// SOCKS5 proxy
	if ( cfg_hostname_socks != NULL ) { GlobalFree( cfg_hostname_socks ); }
	if ( g_punycode_hostname_socks != NULL ) { GlobalFree( g_punycode_hostname_socks ); }

	if ( cfg_proxy_auth_username_socks != NULL ) { GlobalFree( cfg_proxy_auth_username_socks ); }
	if ( cfg_proxy_auth_password_socks != NULL ) { GlobalFree( cfg_proxy_auth_password_socks ); }
	if ( g_proxy_auth_username_socks != NULL ) { GlobalFree( g_proxy_auth_username_socks ); }
	if ( g_proxy_auth_password_socks != NULL ) { GlobalFree( g_proxy_auth_password_socks ); }
	if ( g_proxy_auth_ident_username_socks != NULL ) { GlobalFree( g_proxy_auth_ident_username_socks ); }

	// Server

	CleanupServerInfo();

	if ( cfg_server_hostname != NULL ) { GlobalFree( cfg_server_hostname ); }

	if ( cfg_certificate_pkcs_file_name != NULL ) { GlobalFree( cfg_certificate_pkcs_file_name ); }
	if ( cfg_certificate_pkcs_password != NULL ) { GlobalFree( cfg_certificate_pkcs_password ); }

	if ( cfg_certificate_cer_file_name != NULL ) { GlobalFree( cfg_certificate_cer_file_name ); }
	if ( cfg_certificate_key_file_name != NULL ) { GlobalFree( cfg_certificate_key_file_name ); }

	if ( g_certificate_pkcs_file_name != NULL ) { GlobalFree( g_certificate_pkcs_file_name ); }
	if ( g_certificate_pkcs_password != NULL ) { GlobalFree( g_certificate_pkcs_password ); }

	if ( g_certificate_cer_file_name != NULL ) { GlobalFree( g_certificate_cer_file_name ); }
	if ( g_certificate_key_file_name != NULL ) { GlobalFree( g_certificate_key_file_name ); }

	if ( cfg_authentication_username != NULL ) { GlobalFree( cfg_authentication_username ); }
	if ( cfg_authentication_password != NULL ) { GlobalFree( cfg_authentication_password ); }

	node_type *node = dllrbt_get_head( g_icon_handles );
	while ( node != NULL )
	{
		ICON_INFO *ii = ( ICON_INFO * )node->val;

		if ( ii != NULL )
		{
			_DestroyIcon( ii->icon );
			GlobalFree( ii->file_extension );
			GlobalFree( ii );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( g_icon_handles );

	node = dllrbt_get_head( g_shared_categories );
	while ( node != NULL )
	{
		SHARED_CATEGORY_INFO *sci = ( SHARED_CATEGORY_INFO * )node->val;

		if ( sci != NULL )
		{
			GlobalFree( sci->category );
			GlobalFree( sci );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( g_shared_categories );

	node = dllrbt_get_head( g_category_file_extensions );
	while ( node != NULL )
	{
		CATEGORY_FILE_EXTENSION_INFO *cfei = ( CATEGORY_FILE_EXTENSION_INFO * )node->val;

		if ( cfei != NULL )
		{
			GlobalFree( cfei->file_extension );
			GlobalFree( cfei );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( g_category_file_extensions );

	node = dllrbt_get_head( g_site_info );
	while ( node != NULL )
	{
		FreeSiteInfo( ( SITE_INFO ** )&( node->val ) );

		node = node->next;
	}

	dllrbt_delete_recursively( g_site_info );

	//

	node = dllrbt_get_head( g_sftp_fps_host_info );
	while ( node != NULL )
	{
		DoublyLinkedList *dll_node = ( DoublyLinkedList * )node->val;

		while ( dll_node != NULL )
		{
			FreeSFTPFpsHostInfo( ( SFTP_FPS_HOST_INFO ** )&( dll_node->data ) );

			DoublyLinkedList *del_node = dll_node;
			dll_node = dll_node->next;

			GlobalFree( del_node );
		}

		node = node->next;
	}

	dllrbt_delete_recursively( g_sftp_fps_host_info );

	//

	node = dllrbt_get_head( g_sftp_keys_host_info );
	while ( node != NULL )
	{
		FreeSFTPKeysHostInfo( ( SFTP_KEYS_HOST_INFO ** )&( node->val ) );

		node = node->next;
	}

	dllrbt_delete_recursively( g_sftp_keys_host_info );

	//

	node = dllrbt_get_head( g_category_info );
	while ( node != NULL )
	{
		FreeCategoryInfo( ( CATEGORY_INFO_ ** )&( node->val ) );

		node = node->next;
	}

	dllrbt_delete_recursively( g_category_info );

	CleanupCategoryList();

	//

	if ( cfg_even_row_font_settings.font != NULL ){ _DeleteObject( cfg_even_row_font_settings.font ); }
	if ( cfg_odd_row_font_settings.font != NULL ){ _DeleteObject( cfg_odd_row_font_settings.font ); }

	// Delete our font.
	_DeleteObject( g_hFont );

	if ( g_clean_tray_icon )
	{
		_DestroyIcon( g_default_tray_icon );
	}

#ifdef ENABLE_DARK_MODE
	UninitDarkMode();
#endif

	if ( fail_type == 1 )
	{
		_MessageBoxW( NULL, L"Call to _RegisterClassExW failed!", PROGRAM_CAPTION, MB_ICONWARNING );
	}
	else if ( fail_type == 2 )
	{
		_MessageBoxW( NULL, L"Call to _CreateWindowExW failed!", PROGRAM_CAPTION, MB_ICONWARNING );
	}
	else if ( fail_type == 3 )
	{
		_MessageBoxW( NULL, L"Failed to create IOCPDownloader thread!", PROGRAM_CAPTION, MB_ICONWARNING );
	}

	UninitializeLocaleValues();

	DeleteCriticalSection( &context_list_cs );
	DeleteCriticalSection( &active_download_list_cs );
	DeleteCriticalSection( &download_queue_cs );
	DeleteCriticalSection( &cmessagebox_prompt_cs );
	DeleteCriticalSection( &file_size_prompt_list_cs );
	DeleteCriticalSection( &rename_file_prompt_list_cs );
	DeleteCriticalSection( &last_modified_prompt_list_cs );
	DeleteCriticalSection( &fingerprint_prompt_list_cs );
	DeleteCriticalSection( &move_file_queue_cs );
	DeleteCriticalSection( &cleanup_cs );
	DeleteCriticalSection( &update_check_timeout_cs );
	DeleteCriticalSection( &file_allocation_cs );

	DeleteCriticalSection( &ftp_listen_info_cs );

	DeleteCriticalSection( &icon_cache_cs );

	DeleteCriticalSection( &session_totals_cs );

	DeleteCriticalSection( &worker_cs );

	ReleaseMutex( app_instance_mutex );
	CloseHandle( app_instance_mutex );

#ifdef ENABLE_LOGGING
	CloseLog();
	UnInitLogging();
#endif

	if ( g_shutdown_action != SHUTDOWN_ACTION_NONE && g_perform_shutdown_action )
	{
		// SHTDN_REASON_FLAG_PLANNED = SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED
		if ( g_shutdown_action == SHUTDOWN_ACTION_LOG_OFF )
		{
			_ExitWindowsEx( EWX_LOGOFF, SHTDN_REASON_FLAG_PLANNED );
		}
		else if ( g_can_perform_shutdown_action )
		{
			if ( g_shutdown_action == SHUTDOWN_ACTION_RESTART )
			{
				_ExitWindowsEx( EWX_REBOOT, SHTDN_REASON_FLAG_PLANNED );
			}
			else if ( g_shutdown_action == SHUTDOWN_ACTION_SLEEP )
			{
				_SetSuspendState( FALSE, FALSE, FALSE );
			}
			else if ( g_shutdown_action == SHUTDOWN_ACTION_HIBERNATE )
			{
				_SetSuspendState( TRUE, FALSE, FALSE );
			}
			else if ( g_shutdown_action == SHUTDOWN_ACTION_SHUT_DOWN )
			{
				if ( _ExitWindowsEx( EWX_POWEROFF, SHTDN_REASON_FLAG_PLANNED ) == 0 )
				{
					_ExitWindowsEx( EWX_SHUTDOWN, SHTDN_REASON_FLAG_PLANNED );
				}
			}
			else if ( g_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN )
			{
				#define EWX_HYBRID_SHUTDOWN	0x00400000
				_ExitWindowsEx( EWX_SHUTDOWN | EWX_HYBRID_SHUTDOWN, SHTDN_REASON_FLAG_PLANNED );
			}
		}
	}

	// Delay loaded DLLs
	__SSL_library_uninit();

	// Wine crashes on exit if this is before SSL_library_uninit().
	if ( psftp_state == PSFTP_STATE_RUNNING )
	{
		_SFTP_UninitGSSAPI();
	}

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
	#ifndef UXTHEME_USE_STATIC_LIB
		UnInitializeUXTheme();
	#endif

UNLOAD_DLLS:

	UnInitializeDPIFunctions();

	#ifndef LIBCRYPTO_USE_STATIC_LIB
		UnInitializeLibCrypto();
	#endif
	#ifndef LIBSSL_USE_STATIC_LIB
		UnInitializeLibSSL();
	#endif
	#ifndef PSFTP_USE_STATIC_LIB
		UnInitializePSFTP();
	#endif
	#ifndef PCRE2_USE_STATIC_LIB
		UnInitializePCRE2();
	#endif
	#ifndef NORMALIZ_USE_STATIC_LIB
		UnInitializeNormaliz();
	#endif
	#ifndef POWRPROF_USE_STATIC_LIB
		UnInitializePowrProf();
	#endif
	#ifndef ZLIB1_USE_STATIC_LIB
		UnInitializeZLib1();
	#endif
	#ifndef KERNEL32_USE_STATIC_LIB
		UnInitializeKernel32();
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
	#else
		return ( int )msg.wParam;
	#endif
}
