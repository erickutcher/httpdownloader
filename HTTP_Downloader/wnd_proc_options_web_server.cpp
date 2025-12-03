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

#include "options.h"

#include "lite_normaliz.h"
#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "connection.h"
#include "utilities.h"

#define BTN_ENABLE_SERVER				1000
#define BTN_TYPE_SERVER_HOST			1001
#define BTN_TYPE_SERVER_IP_ADDRESS		1002
#define EDIT_SERVER_HOST				1003
#define EDIT_SERVER_IP_ADDRESS			1004
#define EDIT_SERVER_PORT				1005
#define BTN_USE_AUTHENTICATION			1006
#define EDIT_AUTHENTICATION_USERNAME	1007
#define EDIT_AUTHENTICATION_PASSWORD	1008
#define BTN_AUTH_TYPE_BASIC				1009
#define BTN_AUTH_TYPE_DIGEST			1010

#define BTN_SERVER_ENABLE_SSL			1011
#define BTN_TYPE_PKCS					1012
#define BTN_TYPE_PAIR					1013
#define EDIT_CERTIFICATE_PKCS			1014
#define BTN_CERTIFICATE_PKCS			1015
#define EDIT_CERTIFICATE_PKCS_PASSWORD	1016
#define EDIT_CERTIFICATE_CER			1017
#define BTN_CERTIFICATE_CER				1018
#define EDIT_CERTIFICATE_KEY			1019
#define BTN_CERTIFICATE_KEY				1020
#define CB_SERVER_SSL_VERSION			1021
#define BTN_CONNECTION_NOTIFICATION		1022


HWND g_hWnd_chk_enable_server = NULL;
HWND g_hWnd_static_hoz1 = NULL;
HWND g_hWnd_chk_type_server_hostname = NULL;
HWND g_hWnd_chk_type_server_ip_address = NULL;
HWND g_hWnd_server_hostname = NULL;
HWND g_hWnd_server_ip_address = NULL;
HWND g_hWnd_static_server_colon = NULL;
HWND g_hWnd_static_server_port = NULL;
HWND g_hWnd_server_port = NULL;
HWND g_hWnd_chk_use_authentication = NULL;
HWND g_hWnd_static_authentication_username = NULL;
HWND g_hWnd_authentication_username = NULL;
HWND g_hWnd_static_authentication_password = NULL;
HWND g_hWnd_authentication_password = NULL;
HWND g_hWnd_chk_authentication_type_basic = NULL;
HWND g_hWnd_chk_authentication_type_digest = NULL;

HWND g_hWnd_chk_server_enable_ssl = NULL;
HWND g_hWnd_static_hoz2 = NULL;
HWND g_hWnd_chk_type_pkcs = NULL;
HWND g_hWnd_chk_type_pair = NULL;
HWND g_hWnd_static_certificate_pkcs_location = NULL;
HWND g_hWnd_certificate_pkcs_location = NULL;
HWND g_hWnd_btn_certificate_pkcs_location = NULL;
HWND g_hWnd_static_certificate_pkcs_password = NULL;
HWND g_hWnd_certificate_pkcs_password = NULL;
HWND g_hWnd_static_certificate_cer_location = NULL;
HWND g_hWnd_certificate_cer_location = NULL;
HWND g_hWnd_btn_certificate_cer_location = NULL;
HWND g_hWnd_static_certificate_key_location = NULL;
HWND g_hWnd_certificate_key_location = NULL;
HWND g_hWnd_btn_certificate_key_location = NULL;
HWND g_hWnd_static_server_ssl_version = NULL;
HWND g_hWnd_server_ssl_version = NULL;
HWND g_hWnd_chk_remote_connection_notification = NULL;

// Free these when done.
wchar_t *certificate_pkcs_file_name = NULL;
wchar_t *certificate_cer_file_name = NULL;
wchar_t *certificate_key_file_name = NULL;

unsigned int certificate_pkcs_file_name_length = 0;
unsigned int certificate_cer_file_name_length = 0;
unsigned int certificate_key_file_name_length = 0;

HFONT hFont_copy_ws_ip_address = NULL;

void SetServerSettings()
{
	bool ssl_info_changed = false;
	bool server_info_changed = false;
	bool auth_info_changed = false;

	// SERVER

	bool enable_server = cfg_enable_server;
	cfg_enable_server = ( _SendMessageW( g_hWnd_chk_enable_server, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
	if ( cfg_enable_server != enable_server )
	{
		server_info_changed = true;
	}

	unsigned char server_address_type = cfg_server_address_type;
	cfg_server_address_type = ( _SendMessageW( g_hWnd_chk_type_server_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );
	if ( cfg_server_address_type != server_address_type )
	{
		server_info_changed = true;
	}

	unsigned int server_hostname_length = ( unsigned int )_SendMessageW( g_hWnd_server_hostname, WM_GETTEXTLENGTH, 0, 0 );
	wchar_t *server_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( server_hostname_length + 1 ) );
	_SendMessageW( g_hWnd_server_hostname, WM_GETTEXT, server_hostname_length + 1, ( LPARAM )server_hostname );

	if ( cfg_server_hostname != NULL )
	{
		unsigned int cfg_server_hostname_length = lstrlenW( cfg_server_hostname );

		if ( cfg_server_hostname_length != server_hostname_length || _StrCmpNW( cfg_server_hostname, server_hostname, server_hostname_length ) != 0 )
		{
			GlobalFree( cfg_server_hostname );

			cfg_server_hostname = server_hostname;

			server_info_changed = true;
		}
		else
		{
			GlobalFree( server_hostname );
		}
	}
	else
	{
		cfg_server_hostname = server_hostname;

		server_info_changed = true;
	}

	if ( normaliz_state == NORMALIZ_STATE_RUNNING )
	{
		if ( cfg_server_address_type == 0 )
		{
			if ( g_server_punycode_hostname != NULL )
			{
				GlobalFree( g_server_punycode_hostname );
				g_server_punycode_hostname = NULL;
			}

			int punycode_length = _IdnToAscii( 0, cfg_server_hostname, server_hostname_length, NULL, 0 );

			if ( punycode_length > ( int )server_hostname_length )
			{
				g_server_punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
				_IdnToAscii( 0, cfg_server_hostname, server_hostname_length, g_server_punycode_hostname, punycode_length );
			}
		}
	}

	unsigned long server_ip_address = cfg_server_ip_address;
	_SendMessageW( g_hWnd_server_ip_address, IPM_GETADDRESS, 0, ( LPARAM )&cfg_server_ip_address );
	if ( cfg_server_ip_address != server_ip_address )
	{
		server_info_changed = true;
	}

	char value[ 11 ];
	_SendMessageA( g_hWnd_server_port, WM_GETTEXT, 6, ( LPARAM )value );
	unsigned short server_port = cfg_server_port;
	cfg_server_port = ( unsigned short )_strtoul( value, NULL, 10 );
	if ( cfg_server_port != server_port )
	{
		server_info_changed = true;
	}

	// AUTH

	bool use_authentication = cfg_use_authentication;
	cfg_use_authentication = ( _SendMessageW( g_hWnd_chk_use_authentication, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
	if ( cfg_use_authentication != use_authentication )
	{
		auth_info_changed = true;
	}

	unsigned int authentication_username_length = ( unsigned int )_SendMessageW( g_hWnd_authentication_username, WM_GETTEXTLENGTH, 0, 0 );
	wchar_t *authentication_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( authentication_username_length + 1 ) );
	_SendMessageW( g_hWnd_authentication_username, WM_GETTEXT, authentication_username_length + 1, ( LPARAM )authentication_username );

	if ( cfg_authentication_username != NULL )
	{
		unsigned int cfg_authentication_username_length = lstrlenW( cfg_authentication_username );

		if ( cfg_authentication_username_length != authentication_username_length || _StrCmpNW( cfg_authentication_username, authentication_username, authentication_username_length ) != 0 )
		{
			GlobalFree( cfg_authentication_username );

			cfg_authentication_username = authentication_username;

			auth_info_changed = true;
		}
		else
		{
			GlobalFree( authentication_username );
		}
	}
	else
	{
		cfg_authentication_username = authentication_username;

		auth_info_changed = true;
	}

	unsigned int authentication_password_length = ( unsigned int )_SendMessageW( g_hWnd_authentication_password, WM_GETTEXTLENGTH, 0, 0 );
	wchar_t *authentication_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( authentication_password_length + 1 ) );
	_SendMessageW( g_hWnd_authentication_password, WM_GETTEXT, authentication_password_length + 1, ( LPARAM )authentication_password );

	if ( cfg_authentication_password != NULL )
	{
		unsigned int cfg_authentication_password_length = lstrlenW( cfg_authentication_password );

		if ( cfg_authentication_password_length != authentication_password_length || _StrCmpNW( cfg_authentication_password, authentication_password, authentication_password_length ) != 0 )
		{
			GlobalFree( cfg_authentication_password );

			cfg_authentication_password = authentication_password;

			auth_info_changed = true;
		}
		else
		{
			GlobalFree( authentication_password );
		}
	}
	else
	{
		cfg_authentication_password = authentication_password;

		auth_info_changed = true;
	}

	unsigned char authentication_type = cfg_authentication_type;
	cfg_authentication_type = ( _SendMessageW( g_hWnd_chk_authentication_type_basic, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? AUTH_TYPE_BASIC : AUTH_TYPE_DIGEST );
	if ( cfg_authentication_type != authentication_type )
	{
		auth_info_changed = true;
	}

	// SSL

	bool server_enable_ssl = cfg_server_enable_ssl;
	cfg_server_enable_ssl = ( _SendMessageW( g_hWnd_chk_server_enable_ssl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
	if ( cfg_server_enable_ssl != server_enable_ssl )
	{
		ssl_info_changed = true;
	}


	unsigned char certificate_type = cfg_certificate_type;
	cfg_certificate_type = ( _SendMessageW( g_hWnd_chk_type_pair, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );
	if ( cfg_certificate_type != certificate_type )
	{
		ssl_info_changed = true;
	}

	unsigned int cfg_certificate_pkcs_file_name_length;
	if ( cfg_certificate_pkcs_file_name != NULL )
	{
		cfg_certificate_pkcs_file_name_length = lstrlenW( cfg_certificate_pkcs_file_name );

		if ( cfg_certificate_pkcs_file_name_length != certificate_pkcs_file_name_length || _StrCmpNW( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name, cfg_certificate_pkcs_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_pkcs_file_name );

			cfg_certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
			if ( cfg_certificate_pkcs_file_name != NULL )
			{
				_wmemcpy_s( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length, certificate_pkcs_file_name, certificate_pkcs_file_name_length );
				cfg_certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.
			}

			GlobalFree( g_certificate_pkcs_file_name );
			g_certificate_pkcs_file_name = WideStringToUTF8String( cfg_certificate_pkcs_file_name, ( int * )&cfg_certificate_pkcs_file_name_length );

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
		if ( cfg_certificate_pkcs_file_name != NULL )
		{
			_wmemcpy_s( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length, certificate_pkcs_file_name, certificate_pkcs_file_name_length );
			cfg_certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.
		}

		g_certificate_pkcs_file_name = WideStringToUTF8String( cfg_certificate_pkcs_file_name, ( int * )&cfg_certificate_pkcs_file_name_length );

		ssl_info_changed = true;
	}

	unsigned int certificate_pkcs_password_length = ( unsigned int )_SendMessageW( g_hWnd_certificate_pkcs_password, WM_GETTEXTLENGTH, 0, 0 );
	wchar_t *certificate_pkcs_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( certificate_pkcs_password_length + 1 ) );
	_SendMessageW( g_hWnd_certificate_pkcs_password, WM_GETTEXT, certificate_pkcs_password_length + 1, ( LPARAM )certificate_pkcs_password );

	unsigned int cfg_certificate_pkcs_password_length;
	if ( cfg_certificate_pkcs_password != NULL )
	{
		cfg_certificate_pkcs_password_length = lstrlenW( cfg_certificate_pkcs_password );

		if ( cfg_certificate_pkcs_password_length != certificate_pkcs_password_length || _StrCmpNW( cfg_certificate_pkcs_password, certificate_pkcs_password, cfg_certificate_pkcs_password_length ) != 0 )
		{
			GlobalFree( cfg_certificate_pkcs_password );

			cfg_certificate_pkcs_password = certificate_pkcs_password;

			GlobalFree( g_certificate_pkcs_password );
			g_certificate_pkcs_password = WideStringToUTF8String( cfg_certificate_pkcs_password, ( int * )&cfg_certificate_pkcs_password_length );

			ssl_info_changed = true;
		}
		else
		{
			GlobalFree( certificate_pkcs_password );
		}
	}
	else
	{
		cfg_certificate_pkcs_password = certificate_pkcs_password;

		g_certificate_pkcs_password = WideStringToUTF8String( cfg_certificate_pkcs_password, ( int * )&cfg_certificate_pkcs_password_length );

		ssl_info_changed = true;
	}

	unsigned int cfg_certificate_cer_file_name_length;
	if ( cfg_certificate_cer_file_name != NULL )
	{
		cfg_certificate_cer_file_name_length = lstrlenW( cfg_certificate_cer_file_name );

		if ( cfg_certificate_cer_file_name_length != certificate_cer_file_name_length || _StrCmpNW( cfg_certificate_cer_file_name, certificate_cer_file_name, cfg_certificate_cer_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_cer_file_name );

			cfg_certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
			if ( cfg_certificate_cer_file_name != NULL )
			{
				_wmemcpy_s( cfg_certificate_cer_file_name, certificate_cer_file_name_length, certificate_cer_file_name, certificate_cer_file_name_length );
				cfg_certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.
			}

			GlobalFree( g_certificate_cer_file_name );
			g_certificate_cer_file_name = WideStringToUTF8String( cfg_certificate_cer_file_name, ( int * )&cfg_certificate_cer_file_name_length );

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
		if ( cfg_certificate_cer_file_name != NULL )
		{
			_wmemcpy_s( cfg_certificate_cer_file_name, certificate_cer_file_name_length, certificate_cer_file_name, certificate_cer_file_name_length );
			cfg_certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.
		}

		g_certificate_cer_file_name = WideStringToUTF8String( cfg_certificate_cer_file_name, ( int * )&cfg_certificate_cer_file_name_length );

		ssl_info_changed = true;
	}

	unsigned int cfg_certificate_key_file_name_length;
	if ( cfg_certificate_key_file_name != NULL )
	{
		cfg_certificate_key_file_name_length = lstrlenW( cfg_certificate_key_file_name );

		if ( cfg_certificate_key_file_name_length != certificate_key_file_name_length || _StrCmpNW( cfg_certificate_key_file_name, certificate_key_file_name, cfg_certificate_key_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_key_file_name );

			cfg_certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
			if ( cfg_certificate_key_file_name != NULL )
			{
				_wmemcpy_s( cfg_certificate_key_file_name, certificate_key_file_name_length, certificate_key_file_name, certificate_key_file_name_length );
				cfg_certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.
			}

			GlobalFree( g_certificate_key_file_name );
			g_certificate_key_file_name = WideStringToUTF8String( cfg_certificate_key_file_name, ( int * )&cfg_certificate_key_file_name_length );

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
		if ( cfg_certificate_key_file_name != NULL )
		{
			_wmemcpy_s( cfg_certificate_key_file_name, certificate_key_file_name_length, certificate_key_file_name, certificate_key_file_name_length );
			cfg_certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.
		}

		g_certificate_key_file_name = WideStringToUTF8String( cfg_certificate_key_file_name, ( int * )&cfg_certificate_key_file_name_length );

		ssl_info_changed = true;
	}

	unsigned char server_ssl_version = cfg_server_ssl_version;
	cfg_server_ssl_version = ( unsigned char )_SendMessageW( g_hWnd_server_ssl_version, CB_GETCURSEL, 0, 0 );
	if ( cfg_server_ssl_version != server_ssl_version )
	{
		ssl_info_changed = true;
	}

	if ( auth_info_changed )
	{
		if ( g_authentication_username != NULL ) { GlobalFree( g_authentication_username ); g_authentication_username = NULL; }
		g_authentication_username_length = 0;

		if ( g_authentication_password != NULL ) { GlobalFree( g_authentication_password ); g_authentication_password = NULL; }
		g_authentication_password_length = 0;

		if ( g_nonce != NULL ) { GlobalFree( g_nonce ); g_nonce = NULL; }
		g_nonce_length = 0;

		if ( g_opaque != NULL ) { GlobalFree( g_opaque ); g_opaque = NULL; }
		g_opaque_length = 0;

		if ( g_encoded_authentication != NULL ) { GlobalFree( g_encoded_authentication ); g_encoded_authentication = NULL; }
		g_encoded_authentication_length = 0;

		if ( cfg_use_authentication )
		{
			if ( cfg_authentication_username != NULL )
			{
				g_authentication_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, NULL, 0, NULL, NULL );
				g_authentication_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * g_authentication_username_length ); // Size includes the null character.
				g_authentication_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_username, -1, g_authentication_username, g_authentication_username_length, NULL, NULL ) - 1;
			}

			if ( cfg_authentication_password != NULL )
			{
				g_authentication_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, NULL, 0, NULL, NULL );
				g_authentication_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * g_authentication_password_length ); // Size includes the null character.
				g_authentication_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_authentication_password, -1, g_authentication_password, g_authentication_password_length, NULL, NULL ) - 1;
			}

			if ( cfg_authentication_type == AUTH_TYPE_DIGEST )
			{
				CreateDigestAuthorizationInfo( &g_nonce, g_nonce_length, &g_opaque, g_opaque_length );
			}
			else
			{
				CreateBasicAuthorizationKey( g_authentication_username, g_authentication_username_length, g_authentication_password, g_authentication_password_length, &g_encoded_authentication, &g_encoded_authentication_length );
			}
		}
	}

	if ( ssl_info_changed )
	{
		if ( g_use_openssl )
		{
			if ( g_server_ssl_ctx != NULL )
			{
				_SSL_CTX_free( g_server_ssl_ctx );
				g_server_ssl_ctx = NULL;
			}

			if ( cfg_server_enable_ssl )
			{
				InitializeServerSSL_CTX( cfg_server_ssl_version, cfg_certificate_type );
			}
		}
		else
		{
			if ( g_pCertContext != NULL )
			{
				_CertFreeCertificateContext( g_pCertContext );
				g_pCertContext = NULL;
			}

			if ( cfg_server_enable_ssl )
			{
				if ( cfg_certificate_type == 1 )	// Public/Private Key Pair.
				{
					g_pCertContext = LoadPublicPrivateKeyPair( cfg_certificate_cer_file_name, cfg_certificate_key_file_name );
				}
				else	// PKCS #12 File.
				{
					g_pCertContext = LoadPKCS12( cfg_certificate_pkcs_file_name, cfg_certificate_pkcs_password );
				}
			}

			ResetServerCredentials();
		}
	}

	cfg_show_remote_connection_notification = ( _SendMessageW( g_hWnd_chk_remote_connection_notification, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

	if ( server_info_changed || ssl_info_changed )
	{
		CleanupServer();	// Stop the server.

		if ( cfg_enable_server )
		{
			StartServer();
		}
	}
}

void Enable_Disable_SSL_Windows( BOOL enable )
{
	_EnableWindow( g_hWnd_chk_type_pkcs, enable );
	_EnableWindow( g_hWnd_chk_type_pair, enable );

	_EnableWindow( g_hWnd_static_certificate_pkcs_location, enable );
	_EnableWindow( g_hWnd_certificate_pkcs_location, enable );
	_EnableWindow( g_hWnd_btn_certificate_pkcs_location, enable );

	_EnableWindow( g_hWnd_static_certificate_pkcs_password, enable );
	_EnableWindow( g_hWnd_certificate_pkcs_password, enable );

	_EnableWindow( g_hWnd_static_certificate_cer_location, enable );
	_EnableWindow( g_hWnd_certificate_cer_location, enable );
	_EnableWindow( g_hWnd_btn_certificate_cer_location, enable );

	_EnableWindow( g_hWnd_static_certificate_key_location, enable );
	_EnableWindow( g_hWnd_certificate_key_location, enable );
	_EnableWindow( g_hWnd_btn_certificate_key_location, enable );

	_EnableWindow( g_hWnd_static_server_ssl_version, enable );
	_EnableWindow( g_hWnd_server_ssl_version, enable );
}

void Enable_Disable_Authentication_Windows( BOOL enable )
{
	_EnableWindow( g_hWnd_static_authentication_username, enable );
	_EnableWindow( g_hWnd_authentication_username, enable );
	_EnableWindow( g_hWnd_static_authentication_password, enable );
	_EnableWindow( g_hWnd_authentication_password, enable );
	_EnableWindow( g_hWnd_chk_authentication_type_basic, enable );
	_EnableWindow( g_hWnd_chk_authentication_type_digest, enable );
}

void Enable_Disable_Windows( BOOL enable )
{
	_EnableWindow( g_hWnd_static_hoz1, enable );
	_EnableWindow( g_hWnd_static_hoz2, enable );

	_EnableWindow( g_hWnd_chk_type_server_hostname, enable );
	_EnableWindow( g_hWnd_chk_type_server_ip_address, enable );

	_EnableWindow( g_hWnd_server_hostname, enable );
	_EnableWindow( g_hWnd_server_ip_address, enable );

	_EnableWindow( g_hWnd_static_server_colon, enable );

	_EnableWindow( g_hWnd_static_server_port, enable );
	_EnableWindow( g_hWnd_server_port, enable );

	_EnableWindow( g_hWnd_chk_use_authentication, enable );

	_EnableWindow( g_hWnd_chk_server_enable_ssl, enable );

	if ( !g_t_tray_icon ) // If the System Tray icon is not enabled in the General options, then don't enable the option here.
	{
		enable = FALSE;
	}
	_EnableWindow( g_hWnd_chk_remote_connection_notification, enable );
}

void Set_Window_Settings()
{
	if ( cfg_server_address_type == 1 )
	{
		_SendMessageW( g_hWnd_chk_type_server_ip_address, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_chk_type_server_hostname, BM_SETCHECK, BST_UNCHECKED, 0 );

		_ShowWindow( g_hWnd_server_hostname, SW_HIDE );
		_ShowWindow( g_hWnd_server_ip_address, SW_SHOW );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_type_server_hostname, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_chk_type_server_ip_address, BM_SETCHECK, BST_UNCHECKED, 0 );

		_ShowWindow( g_hWnd_server_ip_address, SW_HIDE );
		_ShowWindow( g_hWnd_server_hostname, SW_SHOW );	
	}

	if ( cfg_server_hostname == NULL )
	{
		_SendMessageW( g_hWnd_server_hostname, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
	}
	else
	{
		_SendMessageW( g_hWnd_server_hostname, WM_SETTEXT, 0, ( LPARAM )cfg_server_hostname );
	}

	_SendMessageW( g_hWnd_server_ip_address, IPM_SETADDRESS, 0, cfg_server_ip_address );

	char value[ 21 ];
	_memzero( value, sizeof( char ) * 21 );
	__snprintf( value, 21, "%hu", cfg_server_port );
	_SendMessageA( g_hWnd_server_port, WM_SETTEXT, 0, ( LPARAM )value );

	_SendMessageW( g_hWnd_chk_server_enable_ssl, BM_SETCHECK, ( cfg_server_enable_ssl ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_chk_use_authentication, BM_SETCHECK, ( cfg_use_authentication ? BST_CHECKED : BST_UNCHECKED ), 0 );

	_SendMessageW( g_hWnd_authentication_username, WM_SETTEXT, 0, ( LPARAM )cfg_authentication_username );
	_SendMessageW( g_hWnd_authentication_password, WM_SETTEXT, 0, ( LPARAM )cfg_authentication_password );

	if ( cfg_authentication_type == AUTH_TYPE_DIGEST )
	{
		_SendMessageW( g_hWnd_chk_authentication_type_basic, BM_SETCHECK, BST_UNCHECKED, 0 );
		_SendMessageW( g_hWnd_chk_authentication_type_digest, BM_SETCHECK, BST_CHECKED, 0 );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_authentication_type_basic, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_chk_authentication_type_digest, BM_SETCHECK, BST_UNCHECKED, 0 );
	}

	if ( cfg_certificate_type == 1 )
	{
		_SendMessageW( g_hWnd_chk_type_pair, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_chk_type_pkcs, BM_SETCHECK, BST_UNCHECKED, 0 );

		_ShowWindow( g_hWnd_static_certificate_pkcs_location, SW_HIDE );
		_ShowWindow( g_hWnd_certificate_pkcs_location, SW_HIDE );
		_ShowWindow( g_hWnd_btn_certificate_pkcs_location, SW_HIDE );

		_ShowWindow( g_hWnd_static_certificate_pkcs_password, SW_HIDE );
		_ShowWindow( g_hWnd_certificate_pkcs_password, SW_HIDE );

		_ShowWindow( g_hWnd_static_certificate_cer_location, SW_SHOW );
		_ShowWindow( g_hWnd_certificate_cer_location, SW_SHOW );
		_ShowWindow( g_hWnd_btn_certificate_cer_location, SW_SHOW );

		_ShowWindow( g_hWnd_static_certificate_key_location, SW_SHOW );
		_ShowWindow( g_hWnd_certificate_key_location, SW_SHOW );
		_ShowWindow( g_hWnd_btn_certificate_key_location, SW_SHOW );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_type_pkcs, BM_SETCHECK, BST_CHECKED, 0 );
		_SendMessageW( g_hWnd_chk_type_pair, BM_SETCHECK, BST_UNCHECKED, 0 );

		_ShowWindow( g_hWnd_static_certificate_cer_location, SW_HIDE );
		_ShowWindow( g_hWnd_certificate_cer_location, SW_HIDE );
		_ShowWindow( g_hWnd_btn_certificate_cer_location, SW_HIDE );

		_ShowWindow( g_hWnd_static_certificate_key_location, SW_HIDE );
		_ShowWindow( g_hWnd_certificate_key_location, SW_HIDE );
		_ShowWindow( g_hWnd_btn_certificate_key_location, SW_HIDE );

		_ShowWindow( g_hWnd_static_certificate_pkcs_location, SW_SHOW );
		_ShowWindow( g_hWnd_certificate_pkcs_location, SW_SHOW );
		_ShowWindow( g_hWnd_btn_certificate_pkcs_location, SW_SHOW );

		_ShowWindow( g_hWnd_static_certificate_pkcs_password, SW_SHOW );
		_ShowWindow( g_hWnd_certificate_pkcs_password, SW_SHOW );
	}

	_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETTEXT, 0, ( LPARAM )cfg_certificate_pkcs_file_name );
	_SendMessageW( g_hWnd_certificate_pkcs_password, WM_SETTEXT, 0, ( LPARAM )cfg_certificate_pkcs_password );

	_SendMessageW( g_hWnd_certificate_cer_location, WM_SETTEXT, 0, ( LPARAM )cfg_certificate_cer_file_name );
	_SendMessageW( g_hWnd_certificate_key_location, WM_SETTEXT, 0, ( LPARAM )cfg_certificate_key_file_name );

	_SendMessageW( g_hWnd_server_ssl_version, CB_SETCURSEL, cfg_server_ssl_version, 0 );

	_SendMessageW( g_hWnd_chk_remote_connection_notification, BM_SETCHECK, ( cfg_show_remote_connection_notification ? BST_CHECKED : BST_UNCHECKED ), 0 );

	if ( cfg_enable_server )
	{
		_SendMessageW( g_hWnd_chk_enable_server, BM_SETCHECK, BST_CHECKED, 0 );
		Enable_Disable_Windows( TRUE );

		Enable_Disable_SSL_Windows( ( ( cfg_server_enable_ssl ) ? TRUE : FALSE ) );
		Enable_Disable_Authentication_Windows( ( ( cfg_use_authentication ) ? TRUE : FALSE ) );
	}
	else
	{
		_SendMessageW( g_hWnd_chk_enable_server, BM_SETCHECK, BST_UNCHECKED, 0 );
		Enable_Disable_Windows( FALSE );

		Enable_Disable_SSL_Windows( FALSE );
		Enable_Disable_Authentication_Windows( FALSE );
	}
}

LRESULT CALLBACK WebServerTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_enable_server = _CreateWindowW( WC_BUTTON, ST_V_Enable_server_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ENABLE_SERVER, NULL, NULL );

			g_hWnd_static_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_type_server_hostname = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_SERVER_HOST, NULL, NULL );
			g_hWnd_chk_type_server_ip_address = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_SERVER_IP_ADDRESS, NULL, NULL );

			g_hWnd_server_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SERVER_HOST, NULL, NULL );
			// Needs a width and height when it's created because it's a stupid control.
			g_hWnd_server_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 0, 310, 23, hWnd, ( HMENU )EDIT_SERVER_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_server_colon = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_server_port = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_server_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_SERVER_PORT, NULL, NULL );

			g_hWnd_chk_use_authentication = _CreateWindowW( WC_BUTTON, ST_V_Require_authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_USE_AUTHENTICATION, NULL, NULL );

			g_hWnd_chk_authentication_type_basic = _CreateWindowW( WC_BUTTON, ST_V_Basic_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_AUTH_TYPE_BASIC, NULL, NULL );
			g_hWnd_chk_authentication_type_digest = _CreateWindowW( WC_BUTTON, ST_V_Digest_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_AUTH_TYPE_DIGEST, NULL, NULL );

			g_hWnd_static_authentication_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_AUTHENTICATION_USERNAME, NULL, NULL );

			g_hWnd_static_authentication_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_AUTHENTICATION_PASSWORD, NULL, NULL );


			g_hWnd_chk_server_enable_ssl = _CreateWindowW( WC_BUTTON, ST_V_Enable_SSL___TLS_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_SERVER_ENABLE_SSL, NULL, NULL );

			g_hWnd_static_hoz2 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );


			g_hWnd_chk_type_pkcs = _CreateWindowW( WC_BUTTON, ST_V_PKCS_NUM12_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_PKCS, NULL, NULL );
			g_hWnd_chk_type_pair = _CreateWindowW( WC_BUTTON, ST_V_Public___Private_key_pair_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_TYPE_PAIR, NULL, NULL );


			g_hWnd_static_certificate_pkcs_location = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_file_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS, NULL, NULL );
			g_hWnd_btn_certificate_pkcs_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CERTIFICATE_PKCS, NULL, NULL );

			g_hWnd_static_certificate_pkcs_password = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_password_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS_PASSWORD, NULL, NULL );

			g_hWnd_static_certificate_cer_location = _CreateWindowW( WC_STATIC, ST_V_Certificate_file_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_cer_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_CERTIFICATE_CER, NULL, NULL );
			g_hWnd_btn_certificate_cer_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CERTIFICATE_CER, NULL, NULL );

			g_hWnd_static_certificate_key_location = _CreateWindowW( WC_STATIC, ST_V_Key_file_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_key_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_CERTIFICATE_KEY, NULL, NULL );
			g_hWnd_btn_certificate_key_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CERTIFICATE_KEY, NULL, NULL );

			g_hWnd_static_server_ssl_version = _CreateWindowW( WC_STATIC, ST_V_Server_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_server_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, 0, 0, 0, 0, hWnd, ( HMENU )CB_SERVER_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );
			if ( g_can_use_tls_1_3 )
			{
				_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_3 );
			}

			g_hWnd_chk_remote_connection_notification = _CreateWindowW( WC_BUTTON, ST_V_Show_notification_for_remote_connections, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_CONNECTION_NOTIFICATION, NULL, NULL );

			_SendMessageW( g_hWnd_server_hostname, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_server_port, EM_LIMITTEXT, 5, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, EM_LIMITTEXT, 1024, 0 );	// 1024 characters + 1 NULL

			//

			_SendMessageW( g_hWnd_chk_enable_server, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_type_server_hostname, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_type_server_ip_address, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_server_hostname, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_server_colon, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_server_port, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_server_port, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_use_authentication, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_authentication_username, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_authentication_username, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_static_authentication_password, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_authentication_password, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_basic, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_digest, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_server_enable_ssl, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_type_pkcs, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_chk_type_pair, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_btn_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_password, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_btn_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_certificate_key_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_certificate_key_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_btn_certificate_key_location, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_static_server_ssl_version, WM_SETFONT, ( WPARAM )hFont_options, 0 );
			_SendMessageW( g_hWnd_server_ssl_version, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			_SendMessageW( g_hWnd_chk_remote_connection_notification, WM_SETFONT, ( WPARAM )hFont_options, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			hFont_copy_ws_ip_address = UpdateFont( current_dpi_options );
			_SendMessageW( g_hWnd_server_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_ws_ip_address, 0 );


			if ( cfg_certificate_pkcs_file_name != NULL )
			{
				certificate_pkcs_file_name_length = lstrlenW( cfg_certificate_pkcs_file_name );
				certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
				if ( certificate_pkcs_file_name != NULL )
				{
					_wmemcpy_s( certificate_pkcs_file_name, certificate_pkcs_file_name_length, cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length );
					certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.
				}
			}

			if ( cfg_certificate_cer_file_name != NULL )
			{
				certificate_cer_file_name_length = lstrlenW( cfg_certificate_cer_file_name );
				certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
				if ( certificate_cer_file_name != NULL )
				{
					_wmemcpy_s( certificate_cer_file_name, certificate_cer_file_name_length, cfg_certificate_cer_file_name, certificate_cer_file_name_length );
					certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.
				}
			}

			if ( cfg_certificate_key_file_name != NULL )
			{
				certificate_key_file_name_length = lstrlenW( cfg_certificate_key_file_name );
				certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
				if ( certificate_key_file_name != NULL )
				{
					_wmemcpy_s( certificate_key_file_name, certificate_key_file_name_length, cfg_certificate_key_file_name, certificate_key_file_name_length );
					certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.
				}
			}

			Set_Window_Settings();

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HDWP hdwp = _BeginDeferWindowPos( 34 );
			_DeferWindowPos( hdwp, g_hWnd_chk_enable_server, HWND_TOP, 0, 0, rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_hoz1, HWND_TOP, 0, _SCALE_O_( 26 ), rc.right, _SCALE_O_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_type_server_hostname, HWND_TOP, 0, _SCALE_O_( 34 ), _SCALE_O_( 200 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_type_server_ip_address, HWND_TOP, _SCALE_O_( 205 ), _SCALE_O_( 34 ), _SCALE_O_( 110 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_server_hostname, HWND_TOP, 0, _SCALE_O_( 54 ), _SCALE_O_( 310 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_server_ip_address, HWND_TOP, 0, _SCALE_O_( 54 ), _SCALE_O_( 310 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_server_colon, HWND_TOP, _SCALE_O_( 311 ), _SCALE_O_( 57 ), _SCALE_O_( 8 ), _SCALE_O_( 17 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_server_port, HWND_TOP, _SCALE_O_( 320 ), _SCALE_O_( 36 ), _SCALE_O_( 75 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_server_port, HWND_TOP, _SCALE_O_( 320 ), _SCALE_O_( 54 ), _SCALE_O_( 75 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_use_authentication, HWND_TOP, 0, _SCALE_O_( 84 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_authentication_type_basic, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 106 ), _SCALE_O_( 212 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_authentication_type_digest, HWND_TOP, _SCALE_O_( 230 ), _SCALE_O_( 106 ), _SCALE_O_( 212 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_authentication_username, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 129 ), _SCALE_O_( 150 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_authentication_username, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 147 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_authentication_password, HWND_TOP, _SCALE_O_( 175 ), _SCALE_O_( 129 ), _SCALE_O_( 150 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_authentication_password, HWND_TOP, _SCALE_O_( 175 ), _SCALE_O_( 147 ), _SCALE_O_( 150 ), _SCALE_O_( 23 ), SWP_NOZORDER );


			_DeferWindowPos( hdwp, g_hWnd_chk_server_enable_ssl, HWND_TOP, 0, _SCALE_O_( 177 ), rc.right - _SCALE_O_( 10 ), _SCALE_O_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_hoz2, HWND_TOP, 0, _SCALE_O_( 203 ), rc.right, _SCALE_O_( 1 ), SWP_NOZORDER );


			_DeferWindowPos( hdwp, g_hWnd_chk_type_pkcs, HWND_TOP, 0, _SCALE_O_( 211 ), _SCALE_O_( 100 ), _SCALE_O_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_type_pair, HWND_TOP, _SCALE_O_( 110 ), _SCALE_O_( 211 ), _SCALE_O_( 250 ), _SCALE_O_( 20 ), SWP_NOZORDER );


			_DeferWindowPos( hdwp, g_hWnd_static_certificate_pkcs_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 233 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_certificate_pkcs_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 251 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_certificate_pkcs_location, HWND_TOP, rc.right - _SCALE_O_( 35 ), _SCALE_O_( 251 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_certificate_pkcs_password, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 282 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_certificate_pkcs_password, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 300 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_certificate_cer_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 233 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_certificate_cer_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 251 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_certificate_cer_location, HWND_TOP, rc.right - _SCALE_O_( 35 ), _SCALE_O_( 251 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_certificate_key_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 282 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_certificate_key_location, HWND_TOP, _SCALE_O_( 15 ), _SCALE_O_( 300 ), rc.right - _SCALE_O_( 55 ), _SCALE_O_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_certificate_key_location, HWND_TOP, rc.right - _SCALE_O_( 35 ), _SCALE_O_( 300 ), _SCALE_O_( 35 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_server_ssl_version, HWND_TOP, 0, _SCALE_O_( 337 ), rc.right - _SCALE_O_( 105 ), _SCALE_O_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_server_ssl_version, HWND_TOP, rc.right - _SCALE_O_( 100 ), _SCALE_O_( 333 ), _SCALE_O_( 100 ), _SCALE_O_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_remote_connection_notification, HWND_TOP, 0, _SCALE_O_( 365 ), rc.right, _SCALE_O_( 20 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_options;
		}
		break;

		case WM_DPICHANGED_AFTERPARENT:
		{
			// This stupid control doesn't adapt to the change in font size. It needs to be resized first.
			_SetWindowPos( g_hWnd_server_ip_address, HWND_TOP, 0, 0, _SCALE_O_( 310 ), _SCALE_O_( 23 ), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
			_DeleteObject( hFont_copy_ws_ip_address );
			hFont_copy_ws_ip_address = UpdateFont( current_dpi_options );
			_SendMessageW( g_hWnd_server_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_ws_ip_address, 0 );

			// Return value is ignored.
			return TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case BTN_ENABLE_SERVER:
				{
					if ( _SendMessageW( g_hWnd_chk_enable_server, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						Enable_Disable_Windows( TRUE );
						Enable_Disable_SSL_Windows( ( ( _SendMessageW( g_hWnd_chk_server_enable_ssl, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? TRUE : FALSE ) );
						Enable_Disable_Authentication_Windows( ( ( _SendMessageW( g_hWnd_chk_use_authentication, BM_GETCHECK, 0, 0 ) == BST_CHECKED ) ? TRUE : FALSE ) );
					}
					else
					{
						Enable_Disable_Windows( FALSE );
						Enable_Disable_SSL_Windows( FALSE );
						Enable_Disable_Authentication_Windows( FALSE );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case EDIT_SERVER_PORT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 6, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 65535 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"65535" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_server_port )
						{
							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
					}
				}
				break;

				case EDIT_AUTHENTICATION_USERNAME:
				case EDIT_AUTHENTICATION_PASSWORD:
				case EDIT_CERTIFICATE_PKCS_PASSWORD:
				case EDIT_SERVER_HOST:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
				}
				break;

				case EDIT_SERVER_IP_ADDRESS:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
				}
				break;

				case BTN_TYPE_SERVER_HOST:
				{
					if ( _SendMessageW( g_hWnd_chk_type_server_hostname, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_server_ip_address, SW_HIDE );
						_ShowWindow( g_hWnd_server_hostname, SW_SHOW );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_TYPE_SERVER_IP_ADDRESS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_server_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_server_hostname, SW_HIDE );
						_ShowWindow( g_hWnd_server_ip_address, SW_SHOW );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_USE_AUTHENTICATION:
				{
					if ( _SendMessageW( g_hWnd_chk_use_authentication, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						Enable_Disable_Authentication_Windows( TRUE );
					}
					else
					{
						Enable_Disable_Authentication_Windows( FALSE );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_AUTH_TYPE_BASIC:
				case BTN_AUTH_TYPE_DIGEST:
				case BTN_CONNECTION_NOTIFICATION:
				{
					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_SERVER_ENABLE_SSL:
				{
					char port[ 6 ];

					if ( _SendMessageW( g_hWnd_chk_server_enable_ssl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						// Revert to saved port of ssl is already enabled.
						if ( cfg_server_enable_ssl )
						{
							_memzero( port, sizeof( char ) * 6 );
							__snprintf( port, 6, "%hu", cfg_server_port );
							_SendMessageA( g_hWnd_server_port, WM_SETTEXT, 0, ( LPARAM )port );
						}
						else	// Otherwise, use the default https port.
						{
							_SendMessageA( g_hWnd_server_port, WM_SETTEXT, 0, ( LPARAM )"443" );
						}
						Enable_Disable_SSL_Windows( TRUE );
					}
					else
					{
						// Revert to saved port if ssl is disabled.
						if ( !cfg_server_enable_ssl )
						{
							_memzero( port, sizeof( char ) * 6 );
							__snprintf( port, 6, "%hu", cfg_server_port );
							_SendMessageA( g_hWnd_server_port, WM_SETTEXT, 0, ( LPARAM )port );
						}
						else	// Otherwise, use the default http port.
						{
							_SendMessageA( g_hWnd_server_port, WM_SETTEXT, 0, ( LPARAM )"80" );
						}
						Enable_Disable_SSL_Windows( FALSE );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_TYPE_PKCS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_pkcs, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_static_certificate_cer_location, SW_HIDE );
						_ShowWindow( g_hWnd_certificate_cer_location, SW_HIDE );
						_ShowWindow( g_hWnd_btn_certificate_cer_location, SW_HIDE );

						_ShowWindow( g_hWnd_static_certificate_key_location, SW_HIDE );
						_ShowWindow( g_hWnd_certificate_key_location, SW_HIDE );
						_ShowWindow( g_hWnd_btn_certificate_key_location, SW_HIDE );

						_ShowWindow( g_hWnd_static_certificate_pkcs_location, SW_SHOW );
						_ShowWindow( g_hWnd_certificate_pkcs_location, SW_SHOW );
						_ShowWindow( g_hWnd_btn_certificate_pkcs_location, SW_SHOW );

						_ShowWindow( g_hWnd_static_certificate_pkcs_password, SW_SHOW );
						_ShowWindow( g_hWnd_certificate_pkcs_password, SW_SHOW );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_TYPE_PAIR:
				{
					if ( _SendMessageW( g_hWnd_chk_type_pair, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_static_certificate_pkcs_location, SW_HIDE );
						_ShowWindow( g_hWnd_certificate_pkcs_location, SW_HIDE );
						_ShowWindow( g_hWnd_btn_certificate_pkcs_location, SW_HIDE );

						_ShowWindow( g_hWnd_static_certificate_pkcs_password, SW_HIDE );
						_ShowWindow( g_hWnd_certificate_pkcs_password, SW_HIDE );

						_ShowWindow( g_hWnd_static_certificate_cer_location, SW_SHOW );
						_ShowWindow( g_hWnd_certificate_cer_location, SW_SHOW );
						_ShowWindow( g_hWnd_btn_certificate_cer_location, SW_SHOW );

						_ShowWindow( g_hWnd_static_certificate_key_location, SW_SHOW );
						_ShowWindow( g_hWnd_certificate_key_location, SW_SHOW );
						_ShowWindow( g_hWnd_btn_certificate_key_location, SW_SHOW );
					}

					_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
				}
				break;

				case BTN_CERTIFICATE_PKCS:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
					if ( file_name != NULL )
					{
						if ( certificate_pkcs_file_name != NULL )
						{
							_wcsncpy_s( file_name, MAX_PATH, certificate_pkcs_file_name, MAX_PATH );
							file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
						}

						wchar_t filter[ 256 ];
						int filter_length = min( ST_L_Personal_Information_Exchange, ( 256 - 39 ) );
						_wmemcpy_s( filter, 256, ST_V_Personal_Information_Exchange, filter_length );
						_wmemcpy_s( filter + filter_length, 256 - filter_length, L" (*.pfx;*.p12)\0*.pfx;*.p12\0", 27 );
						int filter_offset = filter_length + 27;
						filter_length = min( ST_L_All_Files, ( 256 - 12 - filter_offset ) );
						_wmemcpy_s( filter + filter_offset, 256 - filter_offset, ST_V_All_Files, filter_length );
						_wmemcpy_s( filter + filter_offset + filter_length, 256 - filter_offset - filter_length, L" (*.*)\0*.*\0\0", 12 );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = filter;
						ofn.lpstrTitle = ST_V_Load_PKCS_NUM12_File;
						ofn.lpstrFile = file_name;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

						if ( _GetOpenFileNameW( &ofn ) )
						{
							if ( certificate_pkcs_file_name != NULL )
							{
								GlobalFree( certificate_pkcs_file_name );
							}

							certificate_pkcs_file_name = file_name;
							certificate_pkcs_file_name_length = ( certificate_pkcs_file_name != NULL ? lstrlenW( certificate_pkcs_file_name ) : 0 );

							_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETTEXT, 0, ( LPARAM )certificate_pkcs_file_name );

							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
						else
						{
							GlobalFree( file_name );
						}
					}
				}
				break;

				case BTN_CERTIFICATE_CER:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
					if ( file_name != NULL )
					{
						if ( certificate_cer_file_name != NULL )
						{
							_wcsncpy_s( file_name, MAX_PATH, certificate_cer_file_name, MAX_PATH );
							file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
						}

						wchar_t filter[ 256 ];
						int filter_length = min( ST_L_X_509_Certificates, ( 256 - 39 ) );
						_wmemcpy_s( filter, 256, ST_V_X_509_Certificates, filter_length );
						_wmemcpy_s( filter + filter_length, 256 - filter_length, L" (*.cer;*.crt)\0*.cer;*.crt\0", 27 );
						int filter_offset = filter_length + 27;
						filter_length = min( ST_L_All_Files, ( 256 - 12 - filter_offset ) );
						_wmemcpy_s( filter + filter_offset, 256 - filter_offset, ST_V_All_Files, filter_length );
						_wmemcpy_s( filter + filter_offset + filter_length, 256 - filter_offset - filter_length, L" (*.*)\0*.*\0\0", 12 );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = filter;
						ofn.lpstrTitle = ST_V_Load_X_509_Certificate_File;
						ofn.lpstrFile = file_name;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

						if ( _GetOpenFileNameW( &ofn ) )
						{
							if ( certificate_cer_file_name != NULL )
							{
								GlobalFree( certificate_cer_file_name );
							}

							certificate_cer_file_name = file_name;
							certificate_cer_file_name_length = ( certificate_cer_file_name != NULL ? lstrlenW( certificate_cer_file_name ) : 0 );

							_SendMessageW( g_hWnd_certificate_cer_location, WM_SETTEXT, 0, ( LPARAM )certificate_cer_file_name );

							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
						else
						{
							GlobalFree( file_name );
						}
					}
				}
				break;

				case BTN_CERTIFICATE_KEY:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );
					if ( file_name != NULL )
					{
						if ( certificate_key_file_name != NULL )
						{
							_wcsncpy_s( file_name, MAX_PATH, certificate_key_file_name, MAX_PATH );
							file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
						}

						wchar_t filter[ 256 ];
						int filter_length = min( ST_L_Private_Keys, ( 256 - 27 ) );
						_wmemcpy_s( filter, 256, ST_V_Private_Keys, filter_length );
						_wmemcpy_s( filter + filter_length, 256 - filter_length, L" (*.key)\0*.key\0", 15 );
						int filter_offset = filter_length + 15;
						filter_length = min( ST_L_All_Files, ( 256 - 12 - filter_offset ) );
						_wmemcpy_s( filter + filter_offset, 256 - filter_offset, ST_V_All_Files, filter_length );
						_wmemcpy_s( filter + filter_offset + filter_length, 256 - filter_offset - filter_length, L" (*.*)\0*.*\0\0", 12 );

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = filter;
						ofn.lpstrTitle = ST_V_Load_Private_Key_File;
						ofn.lpstrFile = file_name;
						ofn.nMaxFile = MAX_PATH;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;

						if ( _GetOpenFileNameW( &ofn ) )
						{
							if ( certificate_key_file_name != NULL )
							{
								GlobalFree( certificate_key_file_name );
							}

							certificate_key_file_name = file_name;
							certificate_key_file_name_length = ( certificate_key_file_name != NULL ? lstrlenW( certificate_key_file_name ) : 0 );

							_SendMessageW( g_hWnd_certificate_key_location, WM_SETTEXT, 0, ( LPARAM )certificate_key_file_name );

							_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
						}
						else
						{
							GlobalFree( file_name );
						}
					}
				}
				break;

				case CB_SERVER_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						_SendMessageW( g_hWnd_options, WM_OPTIONS_CHANGED, TRUE, 0 );
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_SHOWWINDOW:
		{
			if ( lParam == 0 && wParam == TRUE )
			{
				BOOL enable = ( _SendMessageW( g_hWnd_chk_enable_server, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );
				if ( !g_t_tray_icon ) // If the System Tray icon is not enabled in the General options, then don't enable the option here.
				{
					enable = FALSE;
				}
				_EnableWindow( g_hWnd_chk_remote_connection_notification, enable );
			}

			return 0;
		}
		break;

		case WM_SAVE_OPTIONS:
		{
			// Sets any new settings for the server and resets its state if they've changed.
			SetServerSettings();

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( certificate_pkcs_file_name != NULL )
			{
				GlobalFree( certificate_pkcs_file_name );
				certificate_pkcs_file_name = NULL;
			}
			certificate_pkcs_file_name_length = 0;

			if ( certificate_cer_file_name != NULL )
			{
				GlobalFree( certificate_cer_file_name );
				certificate_cer_file_name = NULL;
			}
			certificate_cer_file_name_length = 0;

			if ( certificate_key_file_name != NULL )
			{
				GlobalFree( certificate_key_file_name );
				certificate_key_file_name = NULL;
			}
			certificate_key_file_name_length = 0;

			_DeleteObject( hFont_copy_ws_ip_address );
			hFont_copy_ws_ip_address = NULL;

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				CleanupButtonGlyphs( hWnd );
			}
#endif

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	//return TRUE;
}
