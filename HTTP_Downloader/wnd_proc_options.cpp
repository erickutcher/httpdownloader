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

#include "lite_gdi32.h"
#include "lite_normaliz.h"
#include "lite_winmm.h"

#include "file_operations.h"

#include "utilities.h"

#include "string_tables.h"

#include "system_tray.h"
#include "drop_window.h"

#include "options.h"

#define BTN_OK					1000
#define BTN_CANCEL				1001
#define BTN_APPLY				1002

HWND g_hWnd_options = NULL;

bool options_state_changed = false;

// Options Window
HWND g_hWnd_options_tab = NULL;
HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_appearance_tab = NULL;
HWND g_hWnd_connection_tab = NULL;
HWND g_hWnd_proxy_tab = NULL;
HWND g_hWnd_advanced_tab = NULL;

HWND g_hWnd_apply = NULL;

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

	unsigned int certificate_pkcs_password_length = ( unsigned int )_SendMessageW( g_hWnd_certificate_pkcs_password, WM_GETTEXTLENGTH, 0, 0 );
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

void SetAppearanceSettings()
{
	cfg_odd_row_background_color = t_odd_row_background_color;
	cfg_even_row_background_color = t_even_row_background_color;

	cfg_odd_row_highlight_color = t_odd_row_highlight_color;
	cfg_even_row_highlight_color = t_even_row_highlight_color;

	cfg_odd_row_highlight_font_color = t_odd_row_highlight_font_color;
	cfg_even_row_highlight_font_color = t_even_row_highlight_font_color;

	for ( unsigned char i = 0; i < NUM_COLORS; ++i )
	{
		*progress_colors[ i ] = t_progress_colors[ i ];
	}

	HFONT tmp_font = cfg_odd_row_font_settings.font;
	cfg_odd_row_font_settings.font = _CreateFontIndirectW( &t_odd_row_font_settings.lf );
	_DeleteObject( tmp_font );

	cfg_odd_row_font_settings.font_color = t_odd_row_font_settings.font_color;
	//cfg_odd_row_font_settings.lf = t_odd_row_font_settings.lf;
	_memcpy_s( &cfg_odd_row_font_settings.lf, sizeof( LOGFONT ), &t_odd_row_font_settings.lf, sizeof( LOGFONT ) );

	tmp_font = cfg_even_row_font_settings.font;
	cfg_even_row_font_settings.font = _CreateFontIndirectW( &t_even_row_font_settings.lf );
	_DeleteObject( tmp_font );

	cfg_even_row_font_settings.font_color = t_even_row_font_settings.font_color;
	//cfg_even_row_font_settings.lf = t_even_row_font_settings.lf;
	_memcpy_s( &cfg_even_row_font_settings.lf, sizeof( LOGFONT ), &t_even_row_font_settings.lf, sizeof( LOGFONT ) );

	// Re-measure the row height.
	// Get the row height for our listview control.
	AdjustRowHeight();

	// Force the files list to generate a WM_MEASUREITEM notification.
	RECT rc;
	_GetWindowRect( g_hWnd_files, &rc );
	WINDOWPOS wp;
	wp.hwnd = g_hWnd_files;
	wp.cx = rc.right - rc.left;
	wp.cy = rc.bottom - rc.top;
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	_SendMessageW( g_hWnd_files, WM_WINDOWPOSCHANGED, 0, ( LPARAM )&wp );

	_InvalidateRect( g_hWnd_files, NULL, TRUE );
}

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// WS_CLIPCHILDREN causes artifacts when entering a bad character in a number only edit control.
			g_hWnd_options_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, WC_TABCONTROL, NULL, WS_CHILD | /*WS_CLIPCHILDREN |*/ WS_TABSTOP | WS_VISIBLE, 10, 10, rc.right - 20, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_PARAM | TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_V_General;
			ti.lParam = ( LPARAM )&g_hWnd_general_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Appearance;
			ti.lParam = ( LPARAM )&g_hWnd_appearance_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Connection;
			ti.lParam = ( LPARAM )&g_hWnd_connection_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Proxy;
			ti.lParam = ( LPARAM )&g_hWnd_proxy_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 3, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Advanced;
			ti.lParam = ( LPARAM )&g_hWnd_advanced_tab;
			_SendMessageW( g_hWnd_options_tab, TCM_INSERTITEM, 4, ( LPARAM )&ti );	// Insert a new tab at the end.


			HWND g_hWnd_ok = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			HWND g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_apply = _CreateWindowW( WC_BUTTON, ST_V_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );


			_SendMessageW( g_hWnd_options_tab, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			// Set the tab control's font so we can get the correct tab height to adjust its children.
			RECT rc_tab;
			_GetClientRect( g_hWnd_options_tab, &rc );

			_SendMessageW( g_hWnd_options_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_appearance_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"appearance_tab", NULL, WS_CHILD | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_connection_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"connection_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_proxy_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"proxy_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );
			g_hWnd_advanced_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"advanced_tab", NULL, WS_CHILD | WS_VSCROLL | WS_TABSTOP, 15, ( rc_tab.bottom + rc_tab.top ) + 12, rc.right - 30, rc.bottom - ( ( rc_tab.bottom + rc_tab.top ) + 24 ), g_hWnd_options_tab, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_ok, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_apply, WM_SETFONT, ( WPARAM )g_hFont, 0 );

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
			HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_3DFACE ) );
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
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_HIDE );
					}
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
					int index = ( int )_SendMessageW( nmhdr->hwndFrom, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
					if ( index != -1 )
					{
						_SendMessageW( nmhdr->hwndFrom, TCM_GETITEM, index, ( LPARAM )&tie );	// Get the selected tab's information
						_ShowWindow( *( ( HWND * )tie.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )tie.lParam ) );
					}
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
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

					char value[ 21 ];

					if ( cfg_default_download_directory != NULL )
					{
						GlobalFree( cfg_default_download_directory );
					}

					// We want the length, so don't do GlobalStrDupW.
					g_default_download_directory_length = lstrlenW( t_default_download_directory );
					cfg_default_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_default_download_directory_length + 1 ) );
					_wmemcpy_s( cfg_default_download_directory, g_default_download_directory_length + 1, t_default_download_directory, g_default_download_directory_length );
					*( cfg_default_download_directory + g_default_download_directory_length ) = 0;	// Sanity.

					if ( cfg_sound_file_path != NULL )
					{
						GlobalFree( cfg_sound_file_path );
					}

					cfg_sound_file_path = GlobalStrDupW( t_sound_file_path );

					cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_show_notification = ( _SendMessageW( g_hWnd_chk_show_notification, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_show_tray_progress = ( _SendMessageW( g_hWnd_chk_show_tray_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool tray_icon = ( _SendMessageW( g_hWnd_chk_tray_icon, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// Add the tray icon if it was not previously enabled.
					if ( tray_icon )
					{
						if ( !cfg_tray_icon )
						{
							InitializeSystemTray( g_hWnd_main );
						}
						else	// Tray icon is still enabled.
						{
							if ( !cfg_show_tray_progress )
							{
								g_nid.uFlags &= ~NIF_INFO;
								g_nid.hIcon = g_default_tray_icon;
								_Shell_NotifyIconW( NIM_MODIFY, &g_nid );

								UninitializeIconValues();
							}
						}

						if ( cfg_show_tray_progress )
						{
							InitializeIconValues( g_hWnd_main );
						}
					}
					else if ( !tray_icon && cfg_tray_icon )	// Remove the tray icon if it was previously enabled.
					{
						// Make sure that the main window is not hidden before we delete the tray icon.
						if ( _IsWindowVisible( g_hWnd_main ) == FALSE )
						{
							_ShowWindow( g_hWnd_main, SW_SHOWNOACTIVATE );
						}

						_Shell_NotifyIconW( NIM_DELETE, &g_nid );

						UninitializeIconValues();
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
						if ( g_hWnd_search != NULL ){ _SetWindowPos( g_hWnd_search, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
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

					_SendMessageA( g_hWnd_drop_window_transparency, WM_GETTEXT, 4, ( LPARAM )value );
					cfg_drop_window_transparency = ( unsigned char )_strtoul( value, NULL, 10 );

					cfg_show_drop_window_progress = ( _SendMessageW( g_hWnd_chk_show_drop_window_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_enable_drop_window )
					{
						if ( g_hWnd_url_drop_window == NULL )
						{
							g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL );
							_SetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
							_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

							// Prevents it from stealing focus.
							_SetWindowPos( g_hWnd_url_drop_window, HWND_TOPMOST, cfg_drop_pos_x, cfg_drop_pos_y, 48, 48, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER );
						}
						else
						{
							_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

							if ( !cfg_show_drop_window_progress )
							{
								UpdateDropWindow( 0, 0, 0, 0, false );
							}
						}
					}
					else
					{
						if ( g_hWnd_url_drop_window != NULL )
						{
							_DestroyWindow( g_hWnd_url_drop_window );
						}
					}

					cfg_download_immediately = ( _SendMessageW( g_hWnd_chk_download_immediately, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_prevent_standby = ( _SendMessageW( g_hWnd_chk_prevent_standby, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_resume_downloads = ( _SendMessageW( g_hWnd_chk_resume_downloads, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_play_sound = ( _SendMessageW( g_hWnd_chk_play_sound, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_play_sound )
					{
						#ifndef WINMM_USE_STATIC_LIB
							if ( winmm_state == WINMM_STATE_SHUTDOWN )
							{
								InitializeWinMM();
							}
						#endif
					}

					bool show_gridlines = ( _SendMessageW( g_hWnd_chk_show_gridlines, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					// If we change this, then SetAppearanceSettings() will end up refreshing it for us.
					if ( show_gridlines != cfg_show_gridlines )
					{
						DWORD extended_style = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;
						if ( show_gridlines )
						{
							extended_style |= LVS_EX_GRIDLINES;
						}

						_SendMessageW( g_hWnd_files, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, extended_style );

						cfg_show_gridlines = show_gridlines;
					}

					SetAppearanceSettings();

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

					cfg_prompt_rename = ( unsigned char )_SendMessageW( g_hWnd_prompt_rename, CB_GETCURSEL, 0, 0 );
					cfg_prompt_file_size = ( unsigned char )_SendMessageW( g_hWnd_prompt_file_size, CB_GETCURSEL, 0, 0 );
					_SendMessageA( g_hWnd_max_file_size, WM_GETTEXT, 21, ( LPARAM )value );
					cfg_max_file_size = strtoull( value );
					cfg_prompt_last_modified = ( unsigned char )_SendMessageW( g_hWnd_prompt_last_modified, CB_GETCURSEL, 0, 0 );
					cfg_use_temp_download_directory = ( _SendMessageW( g_hWnd_chk_temp_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_temp_download_directory != NULL )
					{
						GlobalFree( cfg_temp_download_directory );
					}

					// We want the length, so don't do GlobalStrDupW.
					g_temp_download_directory_length = lstrlenW( t_temp_download_directory );
					cfg_temp_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( g_temp_download_directory_length + 1 ) );
					_wmemcpy_s( cfg_temp_download_directory, g_temp_download_directory_length + 1, t_temp_download_directory, g_temp_download_directory_length );
					*( cfg_temp_download_directory + g_temp_download_directory_length ) = 0;	// Sanity.

					//
					// HTTP proxy.
					//
					cfg_enable_proxy = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type = ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int hostname_length = ( unsigned int )_SendMessageW( g_hWnd_hostname, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname != NULL )
					{
						GlobalFree( cfg_hostname );
					}
					cfg_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname );

					_SendMessageW( g_hWnd_ip_address, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address );

					_SendMessageA( g_hWnd_port, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port = ( unsigned short )_strtoul( value, NULL, 10 );

					unsigned int auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_username, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username != NULL )
					{
						GlobalFree( cfg_proxy_auth_username );
					}
					cfg_proxy_auth_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_password, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
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

					hostname_length = ( unsigned int )_SendMessageW( g_hWnd_hostname_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname_s != NULL )
					{
						GlobalFree( cfg_hostname_s );
					}
					cfg_hostname_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname_s, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname_s );

					_SendMessageW( g_hWnd_ip_address_s, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address_s );

					_SendMessageA( g_hWnd_port_s, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port_s = ( unsigned short )_strtoul( value, NULL, 10 );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_username_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username_s != NULL )
					{
						GlobalFree( cfg_proxy_auth_username_s );
					}
					cfg_proxy_auth_username_s = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username_s, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username_s );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_password_s, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
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

					//
					// SOCKS5 proxy.
					//
					cfg_enable_proxy_socks = ( _SendMessageW( g_hWnd_chk_proxy_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_socks_type = ( _SendMessageW( g_hWnd_chk_type_socks5, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? SOCKS_TYPE_V5 : SOCKS_TYPE_V4 );

					cfg_address_type_socks = ( _SendMessageW( g_hWnd_chk_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					hostname_length = ( unsigned int )_SendMessageW( g_hWnd_hostname_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_hostname_socks != NULL )
					{
						GlobalFree( cfg_hostname_socks );
					}
					cfg_hostname_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_hostname_socks, WM_GETTEXT, hostname_length, ( LPARAM )cfg_hostname_socks );

					_SendMessageW( g_hWnd_ip_address_socks, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ip_address_socks );

					_SendMessageA( g_hWnd_port_socks, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_port_socks = ( unsigned short )_strtoul( value, NULL, 10 );

					cfg_use_authentication_socks = ( _SendMessageW( g_hWnd_chk_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_resolve_domain_names_v4a = ( _SendMessageW( g_hWnd_chk_resolve_domain_names_v4a, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_resolve_domain_names = ( _SendMessageW( g_hWnd_chk_resolve_domain_names, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_ident_username_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_ident_username_socks != NULL )
					{
						GlobalFree( cfg_proxy_auth_ident_username_socks );
					}
					cfg_proxy_auth_ident_username_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_ident_username_socks, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_ident_username_socks );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_username_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_username_socks != NULL )
					{
						GlobalFree( cfg_proxy_auth_username_socks );
					}
					cfg_proxy_auth_username_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_username_socks, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_username_socks );

					auth_length = ( unsigned int )_SendMessageW( g_hWnd_auth_password_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_proxy_auth_password_socks != NULL )
					{
						GlobalFree( cfg_proxy_auth_password_socks );
					}
					cfg_proxy_auth_password_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
					_SendMessageW( g_hWnd_auth_password_socks, WM_GETTEXT, auth_length, ( LPARAM )cfg_proxy_auth_password_socks );

					if ( normaliz_state == NORMALIZ_STATE_RUNNING )
					{
						if ( cfg_address_type_socks == 0 )
						{
							if ( g_punycode_hostname_socks != NULL )
							{
								GlobalFree( g_punycode_hostname_socks );
								g_punycode_hostname_socks = NULL;
							}

							int punycode_length = _IdnToAscii( 0, cfg_hostname_socks, hostname_length, NULL, 0 );

							if ( punycode_length > ( int )hostname_length )
							{
								g_punycode_hostname_socks = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
								_IdnToAscii( 0, cfg_hostname_socks, hostname_length, g_punycode_hostname_socks, punycode_length );
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

					if ( cfg_proxy_auth_username_socks != NULL )
					{
						if ( g_proxy_auth_username_socks != NULL )
						{
							GlobalFree( g_proxy_auth_username_socks );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_socks, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_username_socks, -1, g_proxy_auth_username_socks, auth_username_length, NULL, NULL ) - 1;
					}

					if ( cfg_proxy_auth_password_socks != NULL )
					{
						if ( g_proxy_auth_password_socks != NULL )
						{
							GlobalFree( g_proxy_auth_password_socks );
						}
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_socks, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_password_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_password_length ); // Size includes the null character.
						auth_password_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_password_socks, -1, g_proxy_auth_password_socks, auth_password_length, NULL, NULL ) - 1;
					}

					if ( cfg_proxy_auth_ident_username_socks != NULL )
					{
						if ( g_proxy_auth_ident_username_socks != NULL )
						{
							GlobalFree( g_proxy_auth_ident_username_socks );
						}
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_ident_username_socks, -1, NULL, 0, NULL, NULL );
						g_proxy_auth_ident_username_socks = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_username_length ); // Size includes the null character.
						auth_username_length = WideCharToMultiByte( CP_UTF8, 0, cfg_proxy_auth_ident_username_socks, -1, g_proxy_auth_ident_username_socks, auth_username_length, NULL, NULL ) - 1;
					}

					if ( display_notice == 1 )
					{
						_MessageBoxW( hWnd, ST_V_A_restart_is_required_allocation, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice == 2 )
					{
						_MessageBoxW( hWnd, ST_V_A_restart_is_required_threads, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
					}
					else if ( display_notice > 2 )	// Multiple settings changed.
					{
						_MessageBoxW( hWnd, ST_V_A_restart_is_required, PROGRAM_CAPTION, MB_APPLMODAL | MB_ICONINFORMATION );
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
