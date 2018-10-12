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

#include "lite_comdlg32.h"
#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_normaliz.h"

#include "file_operations.h"

#include "utilities.h"

#include "string_tables.h"

#define BTN_OK					1000
#define BTN_CANCEL				1001
#define BTN_APPLY				1002

#define BTN_TRAY_ICON			1003
#define BTN_MINIMIZE_TO_TRAY	1004
#define BTN_CLOSE_TO_TRAY		1005

#define BTN_ALWAYS_ON_TOP		1006

#define BTN_DOWNLOAD_HISTORY	1007

#define BTN_QUICK_ALLOCATION	1008

#define EDIT_MAX_DOWNLOADS		1009

#define EDIT_RETRY_DOWNLOADS_COUNT	1010
#define EDIT_RETRY_PARTS_COUNT	1011

#define EDIT_TIMEOUT			1012

#define EDIT_MAX_REDIRECTS		1013

#define CB_DEFAULT_SSL_VERSION	1014

#define EDIT_THREAD_COUNT		1015

#define EDIT_DEFAULT_DOWNLOAD_PARTS	1016
#define BTN_DEFAULT_DOWNLOAD_DIRECTORY	1017

#define BTN_PROXY				1018

#define BTN_TYPE_HOST			1019
#define BTN_TYPE_IP_ADDRESS		1020
#define EDIT_HOST				1021
#define EDIT_IP_ADDRESS			1022
#define EDIT_PORT				1023

#define BTN_PROXY_S				1024

#define BTN_TYPE_HOST_S			1025
#define BTN_TYPE_IP_ADDRESS_S	1026
#define EDIT_HOST_S				1027
#define EDIT_IP_ADDRESS_S		1028
#define EDIT_PORT_S				1029

#define EDIT_AUTH_USERNAME		1030
#define EDIT_AUTH_PASSWORD		1031
#define EDIT_AUTH_USERNAME_S	1032
#define EDIT_AUTH_PASSWORD_S	1033

#define BTN_ENABLE_SERVER				1034
#define BTN_TYPE_SERVER_HOST			1035
#define BTN_TYPE_SERVER_IP_ADDRESS		1036
#define EDIT_SERVER_HOST				1037
#define EDIT_SERVER_IP_ADDRESS			1038
#define EDIT_SERVER_PORT				1039
#define BTN_USE_AUTHENTICATION			1040
#define EDIT_AUTHENTICATION_USERNAME	1041
#define EDIT_AUTHENTICATION_PASSWORD	1042
#define BTN_AUTH_TYPE_BASIC				1043
#define BTN_AUTH_TYPE_DIGEST			1044

#define BTN_SERVER_ENABLE_SSL			1045
#define BTN_TYPE_PKCS					1046
#define BTN_TYPE_PAIR					1047
#define EDIT_CERTIFICATE_PKCS			1048
#define BTN_CERTIFICATE_PKCS			1049
#define EDIT_CERTIFICATE_PKCS_PASSWORD	1050
#define EDIT_CERTIFICATE_CER			1051
#define BTN_CERTIFICATE_CER				1052
#define EDIT_CERTIFICATE_KEY			1053
#define BTN_CERTIFICATE_KEY				1054
#define CB_SERVER_SSL_VERSION			1055

#define BTN_SET_FILETIME		1056
#define BTN_USE_ONE_INSTANCE	1057
#define BTN_ENABLE_DROP_WINDOW	1058


HWND g_hWnd_options = NULL;


bool options_state_changed = false;

wchar_t *t_default_download_directory = NULL;

// Free these when done.
wchar_t *certificate_pkcs_file_name = NULL;
wchar_t *certificate_cer_file_name = NULL;
wchar_t *certificate_key_file_name = NULL;

unsigned int certificate_pkcs_file_name_length = 0;
unsigned int certificate_cer_file_name_length = 0;
unsigned int certificate_key_file_name_length = 0;

HFONT hFont_copy_proxy = NULL;
HFONT hFont_copy_connection = NULL;


// Options Window
HWND g_hWnd_options_tab = NULL;
HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_connection_tab = NULL;
HWND g_hWnd_proxy_tab = NULL;

HWND g_hWnd_apply = NULL;


// Connection Tab
HWND g_hWnd_max_downloads = NULL;

HWND g_hWnd_retry_downloads_count = NULL;
HWND g_hWnd_retry_parts_count = NULL;

HWND g_hWnd_timeout = NULL;

HWND g_hWnd_max_redirects = NULL;

HWND g_hWnd_default_ssl_version = NULL;
HWND g_hWnd_default_download_parts = NULL;

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

int connection_tab_height = 0;
int connection_tab_scroll_pos = 0;

// Proxy Tab
// HTTP proxy
HWND g_hWnd_chk_proxy = NULL;

HWND g_hWnd_ip_address = NULL;
HWND g_hWnd_hostname = NULL;
HWND g_hWnd_port = NULL;

HWND g_hWnd_static_port = NULL;
HWND g_hWnd_static_colon = NULL;

HWND g_hWnd_chk_type_hostname = NULL;
HWND g_hWnd_chk_type_ip_address = NULL;

HWND g_hWnd_static_auth_username = NULL;
HWND g_hWnd_auth_username = NULL;
HWND g_hWnd_static_auth_password = NULL;
HWND g_hWnd_auth_password = NULL;

// HTTPS proxy
HWND g_hWnd_chk_proxy_s = NULL;

HWND g_hWnd_ip_address_s = NULL;
HWND g_hWnd_hostname_s = NULL;
HWND g_hWnd_port_s = NULL;

HWND g_hWnd_static_port_s = NULL;
HWND g_hWnd_static_colon_s = NULL;

HWND g_hWnd_chk_type_hostname_s = NULL;
HWND g_hWnd_chk_type_ip_address_s = NULL;

HWND g_hWnd_static_auth_username_s = NULL;
HWND g_hWnd_auth_username_s = NULL;
HWND g_hWnd_static_auth_password_s = NULL;
HWND g_hWnd_auth_password_s = NULL;

// General Tab
HWND g_hWnd_chk_tray_icon = NULL;
HWND g_hWnd_chk_minimize = NULL;
HWND g_hWnd_chk_close = NULL;

HWND g_hWnd_chk_always_on_top = NULL;
HWND g_hWnd_chk_download_history = NULL;
HWND g_hWnd_chk_quick_allocation = NULL;
HWND g_hWnd_chk_set_filetime = NULL;
HWND g_hWnd_chk_use_one_instance = NULL;
HWND g_hWnd_chk_enable_drop_window = NULL;

HWND g_hWnd_thread_count = NULL;

HWND g_hWnd_default_download_directory = NULL;
HWND g_hWnd_btn_default_download_directory = NULL;

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

	unsigned int server_hostname_length = _SendMessageW( g_hWnd_server_hostname, WM_GETTEXTLENGTH, 0, 0 );
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

	unsigned int authentication_username_length = _SendMessageW( g_hWnd_authentication_username, WM_GETTEXTLENGTH, 0, 0 );
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

	unsigned int authentication_password_length = _SendMessageW( g_hWnd_authentication_password, WM_GETTEXTLENGTH, 0, 0 );
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

	if ( cfg_certificate_pkcs_file_name != NULL )
	{
		unsigned int cfg_certificate_pkcs_file_name_length = lstrlenW( cfg_certificate_pkcs_file_name );

		if ( cfg_certificate_pkcs_file_name_length != certificate_pkcs_file_name_length || _StrCmpNW( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name, cfg_certificate_pkcs_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_pkcs_file_name );

			cfg_certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
			_wmemcpy_s( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length, certificate_pkcs_file_name, certificate_pkcs_file_name_length );
			cfg_certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
		_wmemcpy_s( cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length, certificate_pkcs_file_name, certificate_pkcs_file_name_length );
		cfg_certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.

		ssl_info_changed = true;
	}

	unsigned int certificate_pkcs_password_length = _SendMessageW( g_hWnd_certificate_pkcs_password, WM_GETTEXTLENGTH, 0, 0 );
	wchar_t *certificate_pkcs_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( certificate_pkcs_password_length + 1 ) );
	_SendMessageW( g_hWnd_certificate_pkcs_password, WM_GETTEXT, certificate_pkcs_password_length + 1, ( LPARAM )certificate_pkcs_password );

	if ( cfg_certificate_pkcs_password != NULL )
	{
		unsigned int cfg_certificate_pkcs_password_length = lstrlenW( cfg_certificate_pkcs_password );

		if ( cfg_certificate_pkcs_password_length != certificate_pkcs_password_length || _StrCmpNW( cfg_certificate_pkcs_password, certificate_pkcs_password, cfg_certificate_pkcs_password_length ) != 0 )
		{
			GlobalFree( cfg_certificate_pkcs_password );

			cfg_certificate_pkcs_password = certificate_pkcs_password;

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

		ssl_info_changed = true;
	}

	if ( cfg_certificate_cer_file_name != NULL )
	{
		unsigned int cfg_certificate_cer_file_name_length = lstrlenW( cfg_certificate_cer_file_name );

		if ( cfg_certificate_cer_file_name_length != certificate_cer_file_name_length || _StrCmpNW( cfg_certificate_cer_file_name, certificate_cer_file_name, cfg_certificate_cer_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_cer_file_name );

			cfg_certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
			_wmemcpy_s( cfg_certificate_cer_file_name, certificate_cer_file_name_length, certificate_cer_file_name, certificate_cer_file_name_length );
			cfg_certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
		_wmemcpy_s( cfg_certificate_cer_file_name, certificate_cer_file_name_length, certificate_cer_file_name, certificate_cer_file_name_length );
		cfg_certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.

		ssl_info_changed = true;
	}


	if ( cfg_certificate_key_file_name != NULL )
	{
		unsigned int cfg_certificate_key_file_name_length = lstrlenW( cfg_certificate_key_file_name );

		if ( cfg_certificate_key_file_name_length != certificate_key_file_name_length || _StrCmpNW( cfg_certificate_key_file_name, certificate_key_file_name, cfg_certificate_key_file_name_length ) != 0 )
		{
			GlobalFree( cfg_certificate_key_file_name );

			cfg_certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
			_wmemcpy_s( cfg_certificate_key_file_name, certificate_key_file_name_length, certificate_key_file_name, certificate_key_file_name_length );
			cfg_certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.

			ssl_info_changed = true;
		}
	}
	else
	{
		cfg_certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
		_wmemcpy_s( cfg_certificate_key_file_name, certificate_key_file_name_length, certificate_key_file_name, certificate_key_file_name_length );
		cfg_certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.

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
		if ( g_authentication_username != NULL ) { GlobalFree( g_authentication_username ); }
		g_authentication_username_length = 0;

		if ( g_authentication_password != NULL ) { GlobalFree( g_authentication_password ); }
		g_authentication_password_length = 0;

		if ( g_nonce != NULL ) { GlobalFree( g_nonce ); }
		g_nonce_length = 0;

		if ( g_opaque != NULL ) { GlobalFree( g_opaque ); }
		g_opaque_length = 0;

		if ( g_encoded_authentication != NULL ) { GlobalFree( g_encoded_authentication ); }
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

	if ( server_info_changed )
	{
		CleanupServer();	// Stop the server.

		if ( cfg_enable_server )
		{
			StartServer();
		}
	}
}


LRESULT CALLBACK GeneralTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_tray_icon = _CreateWindowW( WC_BUTTON, ST_Enable_System_Tray_icon_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_TRAY_ICON, NULL, NULL );
			g_hWnd_chk_minimize = _CreateWindowW( WC_BUTTON, ST_Minimize_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 20, 200, 20, hWnd, ( HMENU )BTN_MINIMIZE_TO_TRAY, NULL, NULL );
			g_hWnd_chk_close = _CreateWindowW( WC_BUTTON, ST_Close_to_System_Tray, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 40, 200, 20, hWnd, ( HMENU )BTN_CLOSE_TO_TRAY, NULL, NULL );

			g_hWnd_chk_always_on_top = _CreateWindowW( WC_BUTTON, ST_Always_on_top, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 65, 200, 20, hWnd, ( HMENU )BTN_ALWAYS_ON_TOP, NULL, NULL );

			g_hWnd_chk_download_history = _CreateWindowW( WC_BUTTON, ST_Enable_download_history, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 85, 200, 20, hWnd, ( HMENU )BTN_DOWNLOAD_HISTORY, NULL, NULL );

			g_hWnd_chk_quick_allocation = _CreateWindowW( WC_BUTTON, ST_Enable_quick_file_allocation, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 105, 325, 20, hWnd, ( HMENU )BTN_QUICK_ALLOCATION, NULL, NULL );

			g_hWnd_chk_set_filetime = _CreateWindowW( WC_BUTTON, ST_Set_date_and_time_of_file, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 125, 325, 20, hWnd, ( HMENU )BTN_SET_FILETIME, NULL, NULL );

			g_hWnd_chk_use_one_instance = _CreateWindowW( WC_BUTTON, ST_Allow_only_one_instance, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 145, 325, 20, hWnd, ( HMENU )BTN_USE_ONE_INSTANCE, NULL, NULL );

			g_hWnd_chk_enable_drop_window = _CreateWindowW( WC_BUTTON, ST_Enable_URL_drop_window, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 165, 325, 20, hWnd, ( HMENU )BTN_ENABLE_DROP_WINDOW, NULL, NULL );

			HWND hWnd_static_default_download_directory = _CreateWindowW( WC_STATIC, ST_Default_download_directory_, WS_CHILD | WS_VISIBLE, 0, 190, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 205, rc.right - 40, 20, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_default_download_directory = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 205, 35, 20, hWnd, ( HMENU )BTN_DEFAULT_DOWNLOAD_DIRECTORY, NULL, NULL );

			HWND hWnd_static_thread_count = _CreateWindowW( WC_STATIC, ST_Thread_pool_count_, WS_CHILD | WS_VISIBLE, 0, 230, 130, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_thread_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 245, 85, 20, hWnd, ( HMENU )EDIT_THREAD_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_thread_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 244, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_thread_count, EM_LIMITTEXT, 10, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_thread_count, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETRANGE32, 1, g_max_threads );
			_SendMessageW( hWnd_ud_thread_count, UDM_SETPOS, 0, cfg_thread_count );



			_SendMessageW( g_hWnd_chk_tray_icon, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_minimize, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_close, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_download_history, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_quick_allocation, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_set_filetime, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_use_one_instance, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_enable_drop_window, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_thread_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_thread_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_default_download_directory, WM_SETFONT, ( WPARAM )hFont, 0 );

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_TRAY_ICON:
				{
					if ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_EnableWindow( g_hWnd_chk_minimize, TRUE );
						_EnableWindow( g_hWnd_chk_close, TRUE );
					}
					else
					{
						_EnableWindow( g_hWnd_chk_minimize, FALSE );
						_EnableWindow( g_hWnd_chk_close, FALSE );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_CLOSE_TO_TRAY:
				case BTN_MINIMIZE_TO_TRAY:
				case BTN_ALWAYS_ON_TOP:
				case BTN_DOWNLOAD_HISTORY:
				case BTN_QUICK_ALLOCATION:
				case BTN_SET_FILETIME:
				case BTN_USE_ONE_INSTANCE:
				case BTN_ENABLE_DROP_WINDOW:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_THREAD_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > g_max_threads )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							__snprintf( value, 11, "%lu", g_max_threads );
							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )value );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_thread_count )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case BTN_DEFAULT_DOWNLOAD_DIRECTORY:
				{
					// Open a browse for folder dialog box.
					BROWSEINFO bi;
					_memzero( &bi, sizeof( BROWSEINFO ) );
					bi.hwndOwner = hWnd;
					bi.lpszTitle = ST_Select_the_default_download_directory;
					bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_VALIDATE;

					bool destroy = true;
					#ifndef OLE32_USE_STATIC_LIB
						if ( ole32_state == OLE32_STATE_SHUTDOWN )
						{
							destroy = InitializeOle32();
						}
					#endif

					if ( destroy )
					{
						// OleInitialize calls CoInitializeEx
						_OleInitialize( NULL );
					}

					LPITEMIDLIST lpiidl = _SHBrowseForFolderW( &bi );
					if ( lpiidl )
					{
						wchar_t *directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
						_memzero( directory, sizeof( wchar_t ) * MAX_PATH );

						// Get the directory path from the id list.
						_SHGetPathFromIDListW( lpiidl, ( LPTSTR )directory );

						if ( directory[ 0 ] != NULL )
						{
							if ( t_default_download_directory != NULL )
							{
								GlobalFree( t_default_download_directory );
							}

							t_default_download_directory = directory;

							_SendMessageW( g_hWnd_default_download_directory, WM_SETTEXT, 0, ( LPARAM )t_default_download_directory );

							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
						else
						{
							GlobalFree( directory );
						}

						if ( destroy )
						{
							_CoTaskMemFree( lpiidl );
						}
						else	// Warn of leak if we can't free.
						{
							_MessageBoxW( NULL, L"Item ID List was not freed.", PROGRAM_CAPTION, 0 );
						}
					}

					if ( destroy )
					{
						_OleUninitialize();
					}
				}
				break;
			}

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	return TRUE;
}

LRESULT CALLBACK ConnectionTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_max_downloads = _CreateWindowW( WC_STATIC, ST_Active_download_limit_, WS_CHILD | WS_VISIBLE, 0, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_downloads = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 15, 85, 20, hWnd, ( HMENU )EDIT_MAX_DOWNLOADS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_max_downloads = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 14, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_downloads, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_downloads, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETPOS, 0, cfg_max_downloads );


			HWND hWnd_static_retry_downloads_count = _CreateWindowW( WC_STATIC, ST_Retry_incomplete_downloads_, WS_CHILD | WS_VISIBLE, 150, 0, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_downloads_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 15, 85, 20, hWnd, ( HMENU )EDIT_RETRY_DOWNLOADS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_downloads_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 235, 14, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_downloads_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_downloads_count, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETPOS, 0, cfg_retry_downloads_count );

			//

			HWND hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_Default_download_parts_, WS_CHILD | WS_VISIBLE, 0, 45, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 85, 20, hWnd, ( HMENU )EDIT_DEFAULT_DOWNLOAD_PARTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 59, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_default_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_default_download_parts, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );


			HWND hWnd_static_retry_parts_count = _CreateWindowW( WC_STATIC, ST_Retry_incomplete_parts_, WS_CHILD | WS_VISIBLE, 150, 45, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_parts_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 60, 85, 20, hWnd, ( HMENU )EDIT_RETRY_PARTS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_parts_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 235, 59, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_parts_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_parts_count, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETPOS, 0, cfg_retry_parts_count );

			//

			HWND hWnd_static_timeout = _CreateWindowW( WC_STATIC, ST_Timeout__seconds__, WS_CHILD | WS_VISIBLE, 0, 90, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_timeout = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 105, 85, 20, hWnd, ( HMENU )EDIT_TIMEOUT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_timeout = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 85, 104, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_timeout, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBUDDY, ( WPARAM )g_hWnd_timeout, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETRANGE32, 10, 300 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETPOS, 0, cfg_timeout );
			if ( cfg_timeout == 0 )
			{
				_SendMessageW( g_hWnd_timeout, WM_SETTEXT, 0, ( LPARAM )L"0" );
			}


			HWND hWnd_static_max_redirects = _CreateWindowW( WC_STATIC, ST_Maximum_redirects_, WS_CHILD | WS_VISIBLE, 150, 90, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_redirects = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 105, 85, 20, hWnd, ( HMENU )EDIT_MAX_REDIRECTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_max_redirects = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 235, 104, _GetSystemMetrics( SM_CXVSCROLL ), 22, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_redirects, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_redirects, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETPOS, 0, cfg_max_redirects );

			//

			HWND hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_Default_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 135, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 150, 100, 20, hWnd, ( HMENU )CB_DEFAULT_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_2_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_3_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_1 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_2 );

			_SendMessageW( g_hWnd_default_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			//

			g_hWnd_chk_enable_server = _CreateWindowW( WC_BUTTON, ST_Enable_server_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 180, 200, 20, hWnd, ( HMENU )BTN_ENABLE_SERVER, NULL, NULL );

			g_hWnd_static_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 205, rc.right - 10, 5, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_type_server_hostname = _CreateWindowW( WC_BUTTON, ST_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 210, 150, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_HOST, NULL, NULL );
			g_hWnd_chk_type_server_ip_address = _CreateWindowW( WC_BUTTON, ST_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 210, 95, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_IP_ADDRESS, NULL, NULL );

			g_hWnd_server_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 230, 235, 20, hWnd, ( HMENU )EDIT_SERVER_HOST, NULL, NULL );
			g_hWnd_server_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 230, 235, 20, hWnd, ( HMENU )EDIT_SERVER_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_server_colon = _CreateWindowW( WC_STATIC, ST__, WS_CHILD | WS_VISIBLE, 239, 232, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_server_port = _CreateWindowW( WC_STATIC, ST_Port_, WS_CHILD | WS_VISIBLE, 245, 215, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 245, 230, 60, 20, hWnd, ( HMENU )EDIT_SERVER_PORT, NULL, NULL );

			g_hWnd_chk_use_authentication = _CreateWindowW( WC_BUTTON, ST_Require_authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 260, 200, 20, hWnd, ( HMENU )BTN_USE_AUTHENTICATION, NULL, NULL );

			g_hWnd_chk_authentication_type_basic = _CreateWindowW( WC_BUTTON, ST_Basic_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 15, 282, 135, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_BASIC, NULL, NULL );
			g_hWnd_chk_authentication_type_digest = _CreateWindowW( WC_BUTTON, ST_Digest_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 282, 135, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_DIGEST, NULL, NULL );

			g_hWnd_static_authentication_username = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 15, 305, 90, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 320, 100, 20, hWnd, ( HMENU )EDIT_AUTHENTICATION_USERNAME, NULL, NULL );

			g_hWnd_static_authentication_password = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 120, 305, 90, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 120, 320, 100, 20, hWnd, ( HMENU )EDIT_AUTHENTICATION_PASSWORD, NULL, NULL );


			g_hWnd_chk_server_enable_ssl = _CreateWindowW( WC_BUTTON, ST_Enable_SSL___TLS_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 350, 150, 20, hWnd, ( HMENU )BTN_SERVER_ENABLE_SSL, NULL, NULL );

			g_hWnd_static_hoz2 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 375, rc.right - 10, 5, hWnd, NULL, NULL, NULL );


			g_hWnd_chk_type_pkcs = _CreateWindowW( WC_BUTTON, ST_PKCS_NUM12_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 380, 90, 20, hWnd, ( HMENU )BTN_TYPE_PKCS, NULL, NULL );
			g_hWnd_chk_type_pair = _CreateWindowW( WC_BUTTON, ST_Public___Private_key_pair_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 90, 380, 150, 20, hWnd, ( HMENU )BTN_TYPE_PAIR, NULL, NULL );


			g_hWnd_static_certificate_pkcs_location = _CreateWindowW( WC_STATIC, ST_PKCS_NUM12_file_, WS_CHILD | WS_VISIBLE, 15, 405, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 420, rc.right - 65, 20, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS, NULL, NULL );
			g_hWnd_btn_certificate_pkcs_location = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 420, 35, 20, hWnd, ( HMENU )BTN_CERTIFICATE_PKCS, NULL, NULL );

			g_hWnd_static_certificate_pkcs_password = _CreateWindowW( WC_STATIC, ST_PKCS_NUM12_password_, WS_CHILD | WS_VISIBLE, 15, 445, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 460, rc.right - 65, 20, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS_PASSWORD, NULL, NULL );

			g_hWnd_static_certificate_cer_location = _CreateWindowW( WC_STATIC, ST_Certificate_file_, WS_CHILD | WS_VISIBLE, 15, 405, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_cer_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 420, rc.right - 65, 20, hWnd, ( HMENU )EDIT_CERTIFICATE_CER, NULL, NULL );
			g_hWnd_btn_certificate_cer_location = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 420, 35, 20, hWnd, ( HMENU )BTN_CERTIFICATE_CER, NULL, NULL );

			g_hWnd_static_certificate_key_location = _CreateWindowW( WC_STATIC, ST_Key_file_, WS_CHILD | WS_VISIBLE, 15, 445, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_key_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 460, rc.right - 65, 20, hWnd, ( HMENU )EDIT_CERTIFICATE_KEY, NULL, NULL );
			g_hWnd_btn_certificate_key_location = _CreateWindowW( WC_BUTTON, ST_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 460, 35, 20, hWnd, ( HMENU )BTN_CERTIFICATE_KEY, NULL, NULL );

			g_hWnd_static_server_ssl_version = _CreateWindowW( WC_STATIC, ST_Server_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 490, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 505, 100, 20, hWnd, ( HMENU )CB_SERVER_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_2_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_SSL_3_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_1 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_TLS_1_2 );

			_SendMessageW( g_hWnd_server_hostname, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_server_port, EM_LIMITTEXT, 5, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, EM_LIMITTEXT, 1024, 0 );	// 1024 characters + 1 NULL

			//

			connection_tab_scroll_pos = 0;
			connection_tab_height = ( rc.bottom - rc.top ) + 230;

			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = connection_tab_height;
			si.nPage = ( ( connection_tab_height / 2 ) + ( ( connection_tab_height % 2 ) != 0 ) );
			si.nPos = 0;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

			//

			_SendMessageW( g_hWnd_chk_enable_server, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_server_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_server_ip_address, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_server_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_server_colon, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_server_port, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_server_port, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_use_authentication, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_authentication_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_authentication_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_static_authentication_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_authentication_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_basic, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_digest, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_server_enable_ssl, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_pkcs, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_pair, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_pkcs_location, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_cer_location, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_key_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_certificate_key_location, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_key_location, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_server_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_server_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_max_downloads, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_max_downloads, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_retry_downloads_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_retry_downloads_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_retry_parts_count, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_retry_parts_count, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_ssl_version, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_default_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_ud_download_parts, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( hWnd_ud_timeout, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( hWnd_static_max_redirects, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_max_redirects, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			LOGFONT lf;
			_memzero( &lf, sizeof( LOGFONT ) );
			_GetObjectW( hFont, sizeof( LOGFONT ), &lf );
			hFont_copy_connection = _CreateFontIndirectW( &lf );
			_SendMessageW( g_hWnd_server_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_connection, 0 );


			if ( cfg_certificate_pkcs_file_name != NULL )
			{
				certificate_pkcs_file_name_length = lstrlenW( cfg_certificate_pkcs_file_name );
				certificate_pkcs_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_pkcs_file_name_length + 1 ) );
				_wmemcpy_s( certificate_pkcs_file_name, certificate_pkcs_file_name_length, cfg_certificate_pkcs_file_name, certificate_pkcs_file_name_length );
				certificate_pkcs_file_name[ certificate_pkcs_file_name_length ] = 0;	// Sanity.
			}

			if ( cfg_certificate_cer_file_name != NULL )
			{
				certificate_cer_file_name_length = lstrlenW( cfg_certificate_cer_file_name );
				certificate_cer_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_cer_file_name_length + 1 ) );
				_wmemcpy_s( certificate_cer_file_name, certificate_cer_file_name_length, cfg_certificate_cer_file_name, certificate_cer_file_name_length );
				certificate_cer_file_name[ certificate_cer_file_name_length ] = 0;	// Sanity.
			}

			if ( cfg_certificate_key_file_name != NULL )
			{
				certificate_key_file_name_length = lstrlenW( cfg_certificate_key_file_name );
				certificate_key_file_name = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t * ) * ( certificate_key_file_name_length + 1 ) );
				_wmemcpy_s( certificate_key_file_name, certificate_key_file_name_length, cfg_certificate_key_file_name, certificate_key_file_name_length );
				certificate_key_file_name[ certificate_key_file_name_length ] = 0;	// Sanity.
			}

			Set_Window_Settings();

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			_SetFocus( hWnd );

			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		case WM_VSCROLL:
		{
			int delta = 0;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { delta = -10; } break;
					case SB_LINEDOWN: { delta = 10; } break;
					case SB_PAGEUP: { delta = -50; } break;
					case SB_PAGEDOWN: { delta = 50; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { delta = ( int )HIWORD( wParam ) - connection_tab_scroll_pos; } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				delta = -( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * 20;
			}

			connection_tab_scroll_pos += delta;

			if ( connection_tab_scroll_pos < 0 )
			{
				delta -= connection_tab_scroll_pos;
				connection_tab_scroll_pos = 0;
			}
			else if ( connection_tab_scroll_pos > ( ( connection_tab_height / 2 ) + ( ( connection_tab_height % 2 ) != 0 ) ) )
			{
				delta -= ( connection_tab_scroll_pos - ( ( connection_tab_height / 2 ) + ( ( connection_tab_height % 2 ) != 0 ) ) );
				connection_tab_scroll_pos = ( ( connection_tab_height / 2 ) + ( ( connection_tab_height % 2 ) != 0 ) );
			}

			if ( delta != 0 )
			{
				_SetScrollPos( hWnd, SB_VERT, connection_tab_scroll_pos, TRUE );
				_ScrollWindow( hWnd, 0, -delta, NULL, NULL );
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case EDIT_MAX_DOWNLOADS:
				case EDIT_MAX_REDIRECTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						/*else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}*/

						if ( ( LOWORD( wParam ) == EDIT_MAX_DOWNLOADS && num != cfg_max_downloads ) ||
							 ( LOWORD( wParam ) == EDIT_MAX_REDIRECTS && num != cfg_max_redirects ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_RETRY_DOWNLOADS_COUNT:
				case EDIT_RETRY_PARTS_COUNT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( ( LOWORD( wParam ) == EDIT_RETRY_DOWNLOADS_COUNT && num != cfg_retry_downloads_count ) ||
							 ( LOWORD( wParam ) == EDIT_RETRY_PARTS_COUNT && num != cfg_retry_parts_count ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_TIMEOUT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 300 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"300" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
					else if ( HIWORD( wParam ) == EN_KILLFOCUS )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 4, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 0 && num < 10 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"10" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_timeout )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case CB_DEFAULT_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_DEFAULT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

						char value[ 11 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 11, ( LPARAM )value );
						unsigned long num = _strtoul( value, NULL, 10 );

						if ( num > 100 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"100" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}
						else if ( num == 0 )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"1" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num != cfg_default_download_parts )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_SERVER_PORT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
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
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_SERVER_IP_ADDRESS:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_SERVER_IP_ADDRESS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_server_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_server_hostname, SW_HIDE );
						_ShowWindow( g_hWnd_server_ip_address, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_AUTH_TYPE_BASIC:
				case BTN_AUTH_TYPE_DIGEST:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_SERVER_ENABLE_SSL:
				{
					if ( _SendMessageW( g_hWnd_chk_server_enable_ssl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						// Revert to saved port of ssl is already enabled.
						if ( cfg_server_enable_ssl )
						{
							char port[ 6 ];
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
							char port[ 6 ];
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_CERTIFICATE_PKCS:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					if ( certificate_pkcs_file_name != NULL )
					{
						_wcsncpy_s( file_name, MAX_PATH, certificate_pkcs_file_name, MAX_PATH );
						file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
					}

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"Personal Information Exchange (*.pfx;*.p12)\0*.pfx;*.p12\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = ST_Load_PKCS_NUM12_File;
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
						certificate_pkcs_file_name_length = lstrlenW( certificate_pkcs_file_name );

						_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETTEXT, 0, ( LPARAM )certificate_pkcs_file_name );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
					else
					{
						GlobalFree( file_name );
					}
				}
				break;

				case BTN_CERTIFICATE_CER:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					if ( certificate_cer_file_name != NULL )
					{
						_wcsncpy_s( file_name, MAX_PATH, certificate_cer_file_name, MAX_PATH );
						file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
					}

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"X.509 Certificate (*.cer;*.crt)\0*.cer;*.crt\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = ST_Load_X_509_Certificate_File;
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
						certificate_cer_file_name_length = lstrlenW( certificate_cer_file_name );

						_SendMessageW( g_hWnd_certificate_cer_location, WM_SETTEXT, 0, ( LPARAM )certificate_cer_file_name );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
					else
					{
						GlobalFree( file_name );
					}
				}
				break;

				case BTN_CERTIFICATE_KEY:
				{
					wchar_t *file_name = ( wchar_t * )GlobalAlloc( GPTR, sizeof( wchar_t ) * MAX_PATH );

					if ( certificate_key_file_name != NULL )
					{
						_wcsncpy_s( file_name, MAX_PATH, certificate_key_file_name, MAX_PATH );
						file_name[ MAX_PATH - 1 ] = 0;	// Sanity.
					}

					OPENFILENAME ofn;
					_memzero( &ofn, sizeof( OPENFILENAME ) );
					ofn.lStructSize = sizeof( OPENFILENAME );
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = L"Private Key (*.key)\0*.key\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = ST_Load_Private_Key_File;
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
						certificate_key_file_name_length = lstrlenW( certificate_key_file_name );

						_SendMessageW( g_hWnd_certificate_key_location, WM_SETTEXT, 0, ( LPARAM )certificate_key_file_name );

						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
					else
					{
						GlobalFree( file_name );
					}
				}
				break;

				case CB_SERVER_SSL_VERSION:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			GlobalFree( certificate_pkcs_file_name );
			certificate_pkcs_file_name = NULL;
			certificate_pkcs_file_name_length = 0;

			GlobalFree( certificate_cer_file_name );
			certificate_cer_file_name = NULL;
			certificate_cer_file_name_length = 0;

			GlobalFree( certificate_key_file_name );
			certificate_key_file_name = NULL;
			certificate_key_file_name_length = 0;

			_DeleteObject( hFont_copy_connection );
			hFont_copy_connection = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	return TRUE;
}

LRESULT CALLBACK ProxyTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	 switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			//

			g_hWnd_chk_proxy = _CreateWindowW( WC_BUTTON, ST_Enable_HTTP_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 200, 20, hWnd, ( HMENU )BTN_PROXY, NULL, NULL );


			g_hWnd_chk_type_hostname = _CreateWindowW( WC_BUTTON, ST_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 20, 150, 20, hWnd, ( HMENU )BTN_TYPE_HOST, NULL, NULL );
			g_hWnd_chk_type_ip_address = _CreateWindowW( WC_BUTTON, ST_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 20, 95, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS, NULL, NULL );

			g_hWnd_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 235, 20, hWnd, ( HMENU )EDIT_HOST, NULL, NULL );
			g_hWnd_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 40, 235, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_colon = _CreateWindowW( WC_STATIC, ST__, WS_CHILD | WS_VISIBLE, 239, 42, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port = _CreateWindowW( WC_STATIC, ST_Port_, WS_CHILD | WS_VISIBLE, 245, 25, 60, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 245, 40, 60, 20, hWnd, ( HMENU )EDIT_PORT, NULL, NULL );


			g_hWnd_static_auth_username = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 0, 65, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_auth_password = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 110, 65, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 80, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD, NULL, NULL );


			_SendMessageW( g_hWnd_hostname, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port, EM_LIMITTEXT, 5, 0 );


			//

			HWND hWnd_static_hoz = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 110, rc.right, 5, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_chk_proxy_s = _CreateWindowW( WC_BUTTON, ST_Enable_HTTPS_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, 200, 20, hWnd, ( HMENU )BTN_PROXY_S, NULL, NULL );


			g_hWnd_chk_type_hostname_s = _CreateWindowW( WC_BUTTON, ST_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 140, 150, 20, hWnd, ( HMENU )BTN_TYPE_HOST_S, NULL, NULL );
			g_hWnd_chk_type_ip_address_s = _CreateWindowW( WC_BUTTON, ST_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 150, 140, 95, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS_S, NULL, NULL );

			g_hWnd_hostname_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 160, 235, 20, hWnd, ( HMENU )EDIT_HOST_S, NULL, NULL );
			g_hWnd_ip_address_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 160, 235, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS_S, NULL, NULL );


			g_hWnd_static_colon_s = _CreateWindowW( WC_STATIC, ST__, WS_CHILD | WS_VISIBLE, 239, 162, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port_s = _CreateWindowW( WC_STATIC, ST_Port_, WS_CHILD | WS_VISIBLE, 245, 145, 60, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 245, 160, 60, 20, hWnd, ( HMENU )EDIT_PORT_S, NULL, NULL );


			g_hWnd_static_auth_username_s = _CreateWindowW( WC_STATIC, ST_Username_, WS_CHILD | WS_VISIBLE, 0, 185, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 200, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME_S, NULL, NULL );

			g_hWnd_static_auth_password_s = _CreateWindowW( WC_STATIC, ST_Password_, WS_CHILD | WS_VISIBLE, 110, 185, 100, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 200, 100, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD_S, NULL, NULL );


			_SendMessageW( g_hWnd_hostname_s, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port_s, EM_LIMITTEXT, 5, 0 );


			//


			_SendMessageW( g_hWnd_chk_proxy, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_hostname, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_colon, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_port, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_port, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_username, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_password, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_proxy_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_hostname_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_colon_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_port_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_port_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_username_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password_s, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_auth_password_s, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			LOGFONT lf;
			_memzero( &lf, sizeof( LOGFONT ) );
			_GetObjectW( hFont, sizeof( LOGFONT ), &lf );
			hFont_copy_proxy = _CreateFontIndirectW( &lf );
			_SendMessageW( g_hWnd_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_proxy, 0 );

			_SendMessageW( g_hWnd_ip_address_s, WM_SETFONT, ( WPARAM )hFont_copy_proxy, 0 );

			//
			// HTTP proxy.
			//
			_SendMessageW( g_hWnd_chk_proxy, BM_SETCHECK, ( cfg_enable_proxy ? BST_CHECKED : BST_UNCHECKED ), 0 );

			BOOL enable = ( cfg_enable_proxy ? TRUE : FALSE );

			_EnableWindow( g_hWnd_chk_type_hostname, enable );
			_EnableWindow( g_hWnd_chk_type_ip_address, enable );
			_EnableWindow( g_hWnd_hostname, enable );
			_EnableWindow( g_hWnd_ip_address, enable );
			_EnableWindow( g_hWnd_static_colon, enable );
			_EnableWindow( g_hWnd_static_port, enable );
			_EnableWindow( g_hWnd_port, enable );
			_EnableWindow( g_hWnd_static_auth_username, enable );
			_EnableWindow( g_hWnd_auth_username, enable );
			_EnableWindow( g_hWnd_static_auth_password, enable );
			_EnableWindow( g_hWnd_auth_password, enable );

			if ( cfg_address_type == 1 )
			{
				_SendMessageW( g_hWnd_chk_type_ip_address, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_hostname, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_hostname, SW_HIDE );
				_ShowWindow( g_hWnd_ip_address, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_hostname, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_ip_address, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_ip_address, SW_HIDE );
				_ShowWindow( g_hWnd_hostname, SW_SHOW );	
			}

			if ( cfg_hostname == NULL )
			{
				_SendMessageW( g_hWnd_hostname, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
			}
			else
			{
				_SendMessageW( g_hWnd_hostname, WM_SETTEXT, 0, ( LPARAM )cfg_hostname );
			}

			_SendMessageW( g_hWnd_ip_address, IPM_SETADDRESS, 0, cfg_ip_address );

			char value[ 6 ];
			_memzero( value, sizeof( char ) * 6 );
			__snprintf( value, 6, "%hu", cfg_port );
			_SendMessageA( g_hWnd_port, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_auth_username, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_username );
			_SendMessageW( g_hWnd_auth_password, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_password );

			//
			// HTTPS proxy.
			//
			_SendMessageW( g_hWnd_chk_proxy_s, BM_SETCHECK, ( cfg_enable_proxy_s ? BST_CHECKED : BST_UNCHECKED ), 0 );

			enable = ( cfg_enable_proxy_s ? TRUE : FALSE );

			_EnableWindow( g_hWnd_chk_type_hostname_s, enable );
			_EnableWindow( g_hWnd_chk_type_ip_address_s, enable );
			_EnableWindow( g_hWnd_hostname_s, enable );
			_EnableWindow( g_hWnd_ip_address_s, enable );
			_EnableWindow( g_hWnd_static_colon_s, enable );
			_EnableWindow( g_hWnd_static_port_s, enable );
			_EnableWindow( g_hWnd_port_s, enable );
			_EnableWindow( g_hWnd_static_auth_username_s, enable );
			_EnableWindow( g_hWnd_auth_username_s, enable );
			_EnableWindow( g_hWnd_static_auth_password_s, enable );
			_EnableWindow( g_hWnd_auth_password_s, enable );

			if ( cfg_address_type_s == 1 )
			{
				_SendMessageW( g_hWnd_chk_type_ip_address_s, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_hostname_s, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_hostname_s, SW_HIDE );
				_ShowWindow( g_hWnd_ip_address_s, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_hostname_s, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_ip_address_s, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_ip_address_s, SW_HIDE );
				_ShowWindow( g_hWnd_hostname_s, SW_SHOW );	
			}

			if ( cfg_hostname_s == NULL )
			{
				_SendMessageW( g_hWnd_hostname_s, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
			}
			else
			{
				_SendMessageW( g_hWnd_hostname_s, WM_SETTEXT, 0, ( LPARAM )cfg_hostname_s );
			}

			_SendMessageW( g_hWnd_ip_address_s, IPM_SETADDRESS, 0, cfg_ip_address_s );

			_memzero( value, sizeof( char ) * 6 );
			__snprintf( value, 6, "%hu", cfg_port_s );
			_SendMessageA( g_hWnd_port_s, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_auth_username_s, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_username_s );
			_SendMessageW( g_hWnd_auth_password_s, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_password_s );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case BTN_PROXY:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_type_hostname, enable );
					_EnableWindow( g_hWnd_chk_type_ip_address, enable );
					_EnableWindow( g_hWnd_hostname, enable );
					_EnableWindow( g_hWnd_ip_address, enable );
					_EnableWindow( g_hWnd_static_colon, enable );
					_EnableWindow( g_hWnd_static_port, enable );
					_EnableWindow( g_hWnd_port, enable );
					_EnableWindow( g_hWnd_static_auth_username, enable );
					_EnableWindow( g_hWnd_auth_username, enable );
					_EnableWindow( g_hWnd_static_auth_password, enable );
					_EnableWindow( g_hWnd_auth_password, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_PROXY_S:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_proxy_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_type_hostname_s, enable );
					_EnableWindow( g_hWnd_chk_type_ip_address_s, enable );
					_EnableWindow( g_hWnd_hostname_s, enable );
					_EnableWindow( g_hWnd_ip_address_s, enable );
					_EnableWindow( g_hWnd_static_colon_s, enable );
					_EnableWindow( g_hWnd_static_port_s, enable );
					_EnableWindow( g_hWnd_port_s, enable );
					_EnableWindow( g_hWnd_static_auth_username_s, enable );
					_EnableWindow( g_hWnd_auth_username_s, enable );
					_EnableWindow( g_hWnd_static_auth_password_s, enable );
					_EnableWindow( g_hWnd_auth_password_s, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case EDIT_PORT:
				case EDIT_PORT_S:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start = 0;

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

						if ( ( LOWORD( wParam ) == EDIT_PORT	&& num != cfg_port ) ||
							 ( LOWORD( wParam ) == EDIT_PORT_S	&& num != cfg_port_s ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_HOST:
				case EDIT_HOST_S:
				case EDIT_AUTH_USERNAME:
				case EDIT_AUTH_PASSWORD:
				case EDIT_AUTH_USERNAME_S:
				case EDIT_AUTH_PASSWORD_S:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case EDIT_IP_ADDRESS:
				case EDIT_IP_ADDRESS_S:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_apply, TRUE );
					}
				}
				break;

				case BTN_TYPE_HOST:
				{
					if ( _SendMessageW( g_hWnd_chk_type_hostname, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_ip_address, SW_HIDE );
						_ShowWindow( g_hWnd_hostname, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_HOST_S:
				{
					if ( _SendMessageW( g_hWnd_chk_type_hostname_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_ip_address_s, SW_HIDE );
						_ShowWindow( g_hWnd_hostname_s, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_IP_ADDRESS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_hostname, SW_HIDE );
						_ShowWindow( g_hWnd_ip_address, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;

				case BTN_TYPE_IP_ADDRESS_S:
				{
					if ( _SendMessageW( g_hWnd_chk_type_ip_address_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_hostname_s, SW_HIDE );
						_ShowWindow( g_hWnd_ip_address_s, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_apply, TRUE );
				}
				break;
			}

			return 0;
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_DESTROY:
		{
			_DeleteObject( hFont_copy_proxy );
			hFont_copy_proxy = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}
	return TRUE;
}

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_General;
			ti.lParam = ( LPARAM )&g_hWnd_general_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Connection;
			ti.lParam = ( LPARAM )&g_hWnd_connection_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_Proxy;
			ti.lParam = ( LPARAM )&g_hWnd_proxy_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.


			HWND g_hWnd_ok = _CreateWindowW( WC_BUTTON, ST_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			HWND g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_apply = _CreateWindowW( WC_BUTTON, ST_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );


			_SendMessageW( g_hWnd_options_tab, WM_SETFONT, ( WPARAM )hFont, 0 );

			// Set the tab control's font so we can get the correct tab height to adjust its children.
			RECT rc_tab;
			_GetClientRect( g_hWnd_options_tab, &rc );

			_SendMessageW( g_hWnd_options_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_connection_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"connection_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_proxy_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"proxy_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );



			_SendMessageW( g_hWnd_ok, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )hFont, 0 );
			_SendMessageW( g_hWnd_apply, WM_SETFONT, ( WPARAM )hFont, 0 );



			// Set settings.

			if ( cfg_tray_icon )
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_CHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, TRUE );
				_EnableWindow( g_hWnd_chk_close, TRUE );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_tray_icon, BM_SETCHECK, BST_UNCHECKED, 0 );
				_EnableWindow( g_hWnd_chk_minimize, FALSE );
				_EnableWindow( g_hWnd_chk_close, FALSE );
			}

			_SendMessageW( g_hWnd_chk_minimize, BM_SETCHECK, ( cfg_minimize_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_close, BM_SETCHECK, ( cfg_close_to_tray ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_always_on_top, BM_SETCHECK, ( cfg_always_on_top ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_download_history, BM_SETCHECK, ( cfg_enable_download_history ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_quick_allocation, BM_SETCHECK, ( cfg_enable_quick_allocation ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_set_filetime, BM_SETCHECK, ( cfg_set_filetime ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_use_one_instance, BM_SETCHECK, ( cfg_use_one_instance ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_enable_drop_window, BM_SETCHECK, ( cfg_enable_drop_window ? BST_CHECKED : BST_UNCHECKED ), 0 );

			/*char value[ 11 ];

			__snprintf( value, 11, "%lu", cfg_max_downloads );
			_SendMessageA( g_hWnd_max_downloads, WM_SETTEXT, 0, ( LPARAM )value );

			__snprintf( value, 11, "%lu", cfg_thread_count );
			_SendMessageA( g_hWnd_thread_count, WM_SETTEXT, 0, ( LPARAM )value );*/

			if ( cfg_default_download_directory != NULL )
			{
				t_default_download_directory = GlobalStrDupW( cfg_default_download_directory );
			}


			options_state_changed = false;
			_EnableWindow( g_hWnd_apply, FALSE );

			return 0;
		}
		break;
/*
		case WM_CTLCOLORSTATIC:
		{
			return ( LRESULT )( _GetSysColorBrush( COLOR_WINDOW ) );
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = _BeginPaint( hWnd, &ps );

			RECT client_rc, frame_rc;
			_GetClientRect( hWnd, &client_rc );

			// Create a memory buffer to draw to.
			HDC hdcMem = _CreateCompatibleDC( hDC );

			HBITMAP hbm = _CreateCompatibleBitmap( hDC, client_rc.right - client_rc.left, client_rc.bottom - client_rc.top );
			HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
			_DeleteObject( ohbm );
			_DeleteObject( hbm );

			// Fill the background.
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_MENU ) );
			_FillRect( hdcMem, &client_rc, color );
			_DeleteObject( color );

			frame_rc = client_rc;
			frame_rc.left += 10;
			frame_rc.right -= 10;
			frame_rc.top += 10;
			frame_rc.bottom -= 40;

			// Fill the frame.
			color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_WINDOW ) );
			_FillRect( hdcMem, &frame_rc, color );
			_DeleteObject( color );

			// Draw the frame's border.
			_DrawEdge( hdcMem, &frame_rc, EDGE_ETCHED, BF_RECT );

			// Draw our memory buffer to the main device context.
			_BitBlt( hDC, client_rc.left, client_rc.top, client_rc.right, client_rc.bottom, hdcMem, 0, 0, SRCCOPY );

			_DeleteDC( hdcMem );
			_EndPaint( hWnd, &ps );

			return 0;
		}
		break;
*/

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_HIDE );
					}

					return FALSE;
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						SetFocus( GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					TCITEM tie;
					_memzero( &tie, sizeof( TCITEM ) );
					tie.mask = TCIF_PARAM; // Get the lparam value
					int index = _SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )tie.lParam ) );
					}

					return FALSE;
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDOK:
				case BTN_OK:
				case BTN_APPLY:
				{
					if ( !options_state_changed )
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
						break;
					}

					if ( cfg_default_download_directory != NULL )
					{
						GlobalFree( cfg_default_download_directory );
					}

					g_default_download_directory_length = lstrlenW( t_default_download_directory );
					cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ) );
					//_memcpy_s( cfg_default_download_directory, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ), t_default_download_directory, sizeof( wchar_t ) * g_default_download_directory_length );
					_wmemcpy_s( cfg_default_download_directory, g_default_download_directory_length + 1, t_default_download_directory, g_default_download_directory_length );
					*( cfg_default_download_directory + g_default_download_directory_length ) = 0;	// Sanity.

					cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool tray_icon = ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// Add the tray icon if it was not previously enabled.
					if ( tray_icon && !cfg_tray_icon )
					{
						g_nid.cbSize = sizeof( g_nid );
						g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
						g_nid.hWnd = g_hWnd_main;
						g_nid.uCallbackMessage = WM_TRAY_NOTIFY;
						g_nid.uID = 1000;
						g_nid.hIcon = ( HICON )_LoadImageW( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ICON ), IMAGE_ICON, 16, 16, LR_SHARED );
						_wmemcpy_s( g_nid.szTip, sizeof( g_nid.szTip ), PROGRAM_CAPTION, 15 );
						g_nid.szTip[ 15 ] = 0;	// Sanity.
						_Shell_NotifyIconW( NIM_ADD, &g_nid );
					}
					else if ( !tray_icon && cfg_tray_icon )	// Remove the tray icon if it was previously enabled.
					{
						// Make sure that the main window is not hidden before we delete the tray icon.
						if ( _IsWindowVisible( g_hWnd_main ) == FALSE )
						{
							_ShowWindow( g_hWnd_main, SW_SHOWNOACTIVATE );
						}

						_Shell_NotifyIconW( NIM_DELETE, &g_nid );
					}

					cfg_tray_icon = tray_icon;

					bool always_on_top = ( _SendMessageW( g_hWnd_chk_always_on_top, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// Set any active windows if we've changed the extended style.
					if ( always_on_top != cfg_always_on_top )
					{
						cfg_always_on_top = always_on_top;

						if ( g_hWnd_main != NULL ){ _SetWindowPos( g_hWnd_main, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_add_urls != NULL ){ _SetWindowPos( g_hWnd_add_urls, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_options != NULL ){ _SetWindowPos( g_hWnd_options, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_update_download != NULL ){ _SetWindowPos( g_hWnd_update_download, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
					}

					cfg_enable_download_history = ( _SendMessageW( g_hWnd_chk_download_history, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool enable_quick_allocation = ( _SendMessageW( g_hWnd_chk_quick_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					unsigned char display_notice = 0;
					if ( enable_quick_allocation != cfg_enable_quick_allocation )
					{
						cfg_enable_quick_allocation = enable_quick_allocation;

						display_notice += 1;
					}

					cfg_set_filetime = ( _SendMessageW( g_hWnd_chk_set_filetime, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_use_one_instance = ( _SendMessageW( g_hWnd_chk_use_one_instance, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_enable_drop_window = ( _SendMessageW( g_hWnd_chk_enable_drop_window, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_enable_drop_window )
					{
						if ( g_hWnd_url_drop_window == NULL )
						{
							g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE, cfg_drop_pos_x, cfg_drop_pos_y, 48, 48, NULL, NULL, NULL, NULL );
							_SetWindowLongW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
							_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, 0x80, LWA_ALPHA );
						}
					}
					else
					{
						if ( g_hWnd_url_drop_window != NULL )
						{
							_DestroyWindow( g_hWnd_url_drop_window );
						}
					}

					char value[ 11 ];
					_SendMessageA( g_hWnd_max_downloads, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_max_downloads = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_retry_downloads_count, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_retry_downloads_count = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_retry_parts_count, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_retry_parts_count = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_timeout, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned short timeout = ( unsigned short )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_max_redirects, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_max_redirects = ( unsigned char )_strtoul( value, NULL, 10 );

					if ( timeout != cfg_timeout )
					{
						if ( cfg_timeout == 0 )
						{
							cfg_timeout = timeout;	// New value will not be 0 and the timeout thread will poll every 1 second.

							// Trigger the timeout thread to poll.
							if ( g_timeout_semaphore != NULL )
							{
								ReleaseSemaphore( g_timeout_semaphore, 1, NULL );
							}
						}
						else
						{
							cfg_timeout = timeout;
						}
					}

					_SendMessageA( g_hWnd_default_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
					cfg_default_download_parts = ( unsigned char )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_thread_count, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned long thread_count = _strtoul( value, NULL, 10 );

					cfg_default_ssl_version = ( unsigned char )_SendMessageW( g_hWnd_default_ssl_version, CB_GETCURSEL, 0, 0 );

					if ( thread_count != cfg_thread_count )
					{
						cfg_thread_count = thread_count;

						display_notice += 2;
					}

					//
					// HTTP proxy.
					//
					cfg_enable_proxy = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type = ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int hostname_length = _SendMessageW( g_hWnd_hostname, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname != NULL )
					{
						GlobalFree( cfg_hostname );
					}
					cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname );

					_SendMessageW( g_hWnd_ip_address, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address );

					_SendMessageA( g_hWnd_port, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port = ( unsigned short )_strtoul( value, NULL, 10 );

					unsigned int auth_length = _SendMessageW( g_hWnd_auth_username, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username != NULL )
					{
						GlobalFree( cfg_proxy_auth_username );
					}
					cfg_proxy_auth_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username );

					auth_length = _SendMessageW( g_hWnd_auth_password, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_password != NULL )
					{
						GlobalFree( cfg_proxy_auth_password );
					}
					cfg_proxy_auth_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_password, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_password );

					if ( normaliz_state == NORMALIZ_STATE_RUNNING )
					{
						if ( cfg_address_type == 0 )
						{
							if ( g_punycode_hostname != NULL )
							{
								GlobalFree( g_punycode_hostname );
								g_punycode_hostname = NULL;
							}

							int punycode_length = _IdnToAscii( 0, cfg_hostname, hostname_length, NULL, 0 );

							if ( punycode_length > ( int )hostname_length )
							{
								g_punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, cfg_hostname, hostname_length, g_punycode_hostname, punycode_length );
							}
						}
					}

					//
					// HTTPS proxy.
					//
					cfg_enable_proxy_s = ( _SendMessageW( g_hWnd_chk_proxy_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type_s = ( _SendMessageW( g_hWnd_chk_type_ip_address_s, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					hostname_length = _SendMessageW( g_hWnd_hostname_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname_s != NULL )
					{
						GlobalFree( cfg_hostname_s );
					}
					cfg_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname_s, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname_s );

					_SendMessageW( g_hWnd_ip_address_s, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address_s );

					_SendMessageA( g_hWnd_port_s, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port_s = ( unsigned short )_strtoul( value, NULL, 10 );

					auth_length = _SendMessageW( g_hWnd_auth_username_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username_s != NULL )
					{
						GlobalFree( cfg_proxy_auth_username_s );
					}
					cfg_proxy_auth_username_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username_s, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username_s );

					auth_length = _SendMessageW( g_hWnd_auth_password_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_password_s != NULL )
					{
						GlobalFree( cfg_proxy_auth_password_s );
					}
					cfg_proxy_auth_password_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_password_s, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_password_s );

					if ( normaliz_state == NORMALIZ_STATE_RUNNING )
					{
						if ( cfg_address_type_s == 0 )
						{
							if ( g_punycode_hostname_s != NULL )
							{
								GlobalFree( g_punycode_hostname_s );
								g_punycode_hostname_s = NULL;
							}

							int punycode_length = _IdnToAscii( 0, cfg_hostname_s, hostname_length, NULL, 0 );

							if ( punycode_length > ( int )hostname_length )
							{
								g_punycode_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, cfg_hostname_s, hostname_length, g_punycode_hostname_s, punycode_length );
							}
						}
					}

					// Sets any new settings for the server and resets its state if they've changed.
					SetServerSettings();

					save_config();

					int auth_username_length = 0, auth_password_length = 0;

					g_proxy_auth_key_length = 0;
					if ( g_proxy_auth_key != NULL )
					{
						GlobalFree( g_proxy_auth_key );
						g_proxy_auth_key = NULL;
					}

					if ( cfg_proxy_auth_username != NULL && cfg_proxy_auth_password != NULL )
					{
						if ( g_proxy_auth_username != NULL )
						{
							GlobalFree( g_proxy_auth_username );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username, -1, g_proxy_auth_username, auth_username_length, NULL, NULL ) - 1;

						if ( g_proxy_auth_password != NULL )
						{
							GlobalFree( g_proxy_auth_password );
						}
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password, -1, g_proxy_auth_password, auth_password_length, NULL, NULL ) - 1;

						CreateBasicAuthorizationKey( g_proxy_auth_username, auth_username_length, g_proxy_auth_password, auth_password_length, &g_proxy_auth_key, &g_proxy_auth_key_length );
					}

					g_proxy_auth_key_length_s = 0;
					if ( g_proxy_auth_key_s != NULL )
					{
						GlobalFree( g_proxy_auth_key_s );
						g_proxy_auth_key_s = NULL;
					}

					if ( cfg_proxy_auth_username_s != NULL && cfg_proxy_auth_password_s != NULL )
					{
						if ( g_proxy_auth_username_s != NULL )
						{
							GlobalFree( g_proxy_auth_username_s );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_username_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_s, -1, g_proxy_auth_username_s, auth_username_length, NULL, NULL ) - 1;

						if ( g_proxy_auth_password_s != NULL )
						{
							GlobalFree( g_proxy_auth_password_s );
						}
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_password_s = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_s, -1, g_proxy_auth_password_s, auth_password_length, NULL, NULL ) - 1;

						CreateBasicAuthorizationKey( g_proxy_auth_username_s, auth_username_length, g_proxy_auth_password_s, auth_password_length, &g_proxy_auth_key_s, &g_proxy_auth_key_length_s );
					}

					if ( display_notice == 1 )
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required_allocation, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice == 2 )
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required_threads, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice > 2 )	// Multiple settings changed.
					{
						_MessageBoxW( hWnd, ST_A_restart_is_required, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}

					options_state_changed = false;

					if ( LOWORD( wParam ) == BTN_APPLY )
					{
						_EnableWindow( g_hWnd_apply, FALSE );
					}
					else
					{
						_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
					}
				}
				break;

				case BTN_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;	
			}

			return 0;
		}

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

			return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_DestroyWindow( hWnd );

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			if ( t_default_download_directory != NULL )
			{
				GlobalFree( t_default_download_directory );
				t_default_download_directory = NULL;
			}

			g_hWnd_options = NULL;

			return 0;
		}
		break;

		default:
		{
			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return TRUE;
}
