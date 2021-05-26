/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2021 Eric Kutcher

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
#include "site_manager_utilities.h"
#include "sftp.h"

#include "ftp_parsing.h"
#include "utilities.h"

#include "string_tables.h"

#include "treelistview.h"
#include "system_tray.h"
#include "drop_window.h"

#include "options.h"
#include "cmessagebox.h"

#define BTN_OK					1000
#define BTN_CANCEL				1001
#define BTN_APPLY				1002

HWND g_hWnd_options = NULL;

bool options_state_changed = false;

// Options Window
HWND g_hWnd_options_tree = NULL;
HWND g_hWnd_general_tab = NULL;
HWND g_hWnd_appearance_tab = NULL;
HWND g_hWnd_connection_tab = NULL;
HWND g_hWnd_web_server_tab = NULL;
HWND g_hWnd_ftp_tab = NULL;
HWND g_hWnd_sftp_tab = NULL;
HWND g_hWnd_sftp_fps_tab = NULL;
HWND g_hWnd_sftp_keys_tab = NULL;
HWND g_hWnd_proxy_tab = NULL;
HWND g_hWnd_advanced_tab = NULL;

HWND g_hWnd_options_ok = NULL;
HWND g_hWnd_options_cancel = NULL;
HWND g_hWnd_options_apply = NULL;

WNDPROC TreeViewProc = NULL;

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
	cfg_background_color = t_background_color;
	cfg_gridline_color = t_gridline_color;
	cfg_selection_marquee_color = t_selection_marquee_color;

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

	for ( unsigned char i = 0; i < TD_NUM_COLORS; ++i )
	{
		*td_progress_colors[ i ] = t_td_progress_colors[ i ];
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

	_SendMessageW( g_hWnd_tlv_files, TLVM_REFRESH_LIST, TRUE, FALSE );
}

LRESULT CALLBACK TreeViewSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_KEYDOWN:
		{
			// Prevent the expand/collapse of parents.
			if ( wParam == VK_LEFT || wParam == VK_RIGHT )
			{
				return 0;
			}
		}
		break;
	}

	return _CallWindowProcW( TreeViewProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK OptionsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_options_tree = _CreateWindowExW( WS_EX_CLIENTEDGE | TVS_EX_DOUBLEBUFFER, WC_TREEVIEW, NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 10, 10, 120, rc.bottom - 50, hWnd, NULL, NULL, NULL );


			TVINSERTSTRUCT tvis;
			_memzero( &tvis, sizeof( TVINSERTSTRUCT ) );

			tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
			tvis.item.state = TVIS_SELECTED | TVIS_EXPANDED;
			tvis.item.stateMask = TVIS_SELECTED | TVIS_EXPANDED;
			tvis.item.pszText = ST_V_General;
			tvis.item.lParam = ( LPARAM )&g_hWnd_general_tab;

			tvis.hParent = TVI_ROOT;
			tvis.hInsertAfter = TVI_FIRST;

			HTREEITEM hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.state = TVIS_EXPANDED;
			tvis.item.pszText = ST_V_Appearance;
			tvis.item.lParam = ( LPARAM )&g_hWnd_appearance_tab;

			tvis.hParent = NULL;
			tvis.hInsertAfter = hti;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Connection;
			tvis.item.lParam = ( LPARAM )&g_hWnd_connection_tab;

			tvis.hParent = NULL;
			tvis.hInsertAfter = hti;

			HTREEITEM hti_connection = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_FTP;
			tvis.item.lParam = ( LPARAM )&g_hWnd_ftp_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = hti_connection;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_SFTP;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = hti;

			HTREEITEM hti_sftp = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Fingerprints;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_fps_tab;

			tvis.hParent = hti_sftp;
			tvis.hInsertAfter = hti_sftp;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Private_Keys;
			tvis.item.lParam = ( LPARAM )&g_hWnd_sftp_keys_tab;

			tvis.hParent = hti_sftp;
			tvis.hInsertAfter = hti;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Proxy;
			tvis.item.lParam = ( LPARAM )&g_hWnd_proxy_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = hti;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Server;
			tvis.item.lParam = ( LPARAM )&g_hWnd_web_server_tab;

			tvis.hParent = hti_connection;
			tvis.hInsertAfter = hti;

			hti = ( HTREEITEM )_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			tvis.item.pszText = ST_V_Advanced;
			tvis.item.lParam = ( LPARAM )&g_hWnd_advanced_tab;

			tvis.hParent = NULL;
			tvis.hInsertAfter = hti;

			_SendMessageW( g_hWnd_options_tree, TVM_INSERTITEM, 0, ( LPARAM )&tvis );

			// WS_EX_CONTROLPARENT for tab key access.
			g_hWnd_general_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"general_tab", NULL, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_appearance_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"appearance_tab", NULL, WS_VSCROLL | WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_connection_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"connection_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_web_server_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"web_server_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_ftp_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"ftp_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"sftp_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_fps_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"sftp_fps_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_sftp_keys_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"sftp_keys_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_proxy_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"proxy_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );
			g_hWnd_advanced_tab = _CreateWindowExW( WS_EX_CONTROLPARENT, L"advanced_tab", NULL, WS_CHILD | WS_TABSTOP, 140, 10, rc.right - 150, rc.bottom - 50, hWnd, NULL, NULL, NULL );

			g_hWnd_options_ok = _CreateWindowW( WC_BUTTON, ST_V_OK, BS_DEFPUSHBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 260, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_OK, NULL, NULL );
			g_hWnd_options_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 175, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_CANCEL, NULL, NULL );
			g_hWnd_options_apply = _CreateWindowW( WC_BUTTON, ST_V_Apply, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, rc.right - 90, rc.bottom - 32, 80, 23, hWnd, ( HMENU )BTN_APPLY, NULL, NULL );

			_SendMessageW( g_hWnd_options_ok, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_options_cancel, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_options_apply, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			TreeViewProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_options_tree, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_options_tree, GWLP_WNDPROC, ( LONG_PTR )TreeViewSubProc );

			options_state_changed = false;
			_EnableWindow( g_hWnd_options_apply, FALSE );

			HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - 720 ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - 500 ) / 2 ), 720, 500, 0 );

			_SetFocus( g_hWnd_options_tree );

			return 0;
		}
		break;

		case WM_SIZE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			// Allow our listview to resize in proportion to the main window.
			HDWP hdwp = _BeginDeferWindowPos( 14 );
			_DeferWindowPos( hdwp, g_hWnd_options_tree, HWND_TOP, 0, 0, 120, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );

			_DeferWindowPos( hdwp, g_hWnd_general_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_appearance_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_connection_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_web_server_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_ftp_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_sftp_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_sftp_fps_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_sftp_keys_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_proxy_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );
			_DeferWindowPos( hdwp, g_hWnd_advanced_tab, HWND_TOP, 0, 0, rc.right - 150, rc.bottom - 50, SWP_NOZORDER | SWP_NOMOVE );

			_DeferWindowPos( hdwp, g_hWnd_options_ok, HWND_TOP, rc.right - 260, rc.bottom - 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
			_DeferWindowPos( hdwp, g_hWnd_options_cancel, HWND_TOP, rc.right - 175, rc.bottom - 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
			_DeferWindowPos( hdwp, g_hWnd_options_apply, HWND_TOP, rc.right - 90, rc.bottom - 32, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
			_EndDeferWindowPos( hdwp );

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our treeview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TVN_SELCHANGING:		// The tree item that's about to lose focus
				{
					NMTREEVIEW *nmtv = ( NMTREEVIEW * )lParam;

					if ( nmtv->itemOld.lParam != NULL )
					{
						_ShowWindow( *( ( HWND * )nmtv->itemOld.lParam ), SW_HIDE );
					}
				}
				break;

				case TVN_SELCHANGED:			// The tree item that gains focus
				{
					NMTREEVIEW *nmtv = ( NMTREEVIEW * )lParam;

					if ( nmtv->itemNew.lParam != NULL )
					{
						_ShowWindow( *( ( HWND * )nmtv->itemNew.lParam ), SW_SHOW );

						_SetFocus( *( ( HWND * )nmtv->itemNew.lParam ) );
					}
				}
				break;

				case NM_DBLCLK:
				{
					return TRUE;
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

					cfg_minimize_to_tray = ( _SendMessageW( g_hWnd_chk_minimize_to_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_close_to_tray = ( _SendMessageW( g_hWnd_chk_close_to_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_start_in_tray = ( _SendMessageW( g_hWnd_chk_start_in_tray, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
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
						if ( g_hWnd_update_download != NULL ){ _SetWindowPos( g_hWnd_update_download, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_search != NULL ){ _SetWindowPos( g_hWnd_search, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_download_speed_limit != NULL ){ _SetWindowPos( g_hWnd_download_speed_limit, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_check_for_updates != NULL ){ _SetWindowPos( g_hWnd_check_for_updates, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_site_manager != NULL ){ _SetWindowPos( g_hWnd_site_manager, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
						if ( g_hWnd_options != NULL ){ _SetWindowPos( g_hWnd_options, ( cfg_always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST ), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ); }
					}

					cfg_check_for_updates = ( _SendMessageW( g_hWnd_chk_check_for_updates_startup, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_enable_download_history = ( _SendMessageW( g_hWnd_chk_download_history, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					bool enable_quick_allocation = ( _SendMessageW( g_hWnd_chk_quick_allocation, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					unsigned char display_notice = 0x00;
					if ( enable_quick_allocation != cfg_enable_quick_allocation )
					{
						cfg_enable_quick_allocation = enable_quick_allocation;

						display_notice = ( enable_quick_allocation ? 0x01 : 0x02 );
					}

					cfg_set_filetime = ( _SendMessageW( g_hWnd_chk_set_filetime, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_update_redirected = ( _SendMessageW( g_hWnd_chk_update_redirected, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_use_one_instance = ( _SendMessageW( g_hWnd_chk_use_one_instance, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_enable_drop_window = ( _SendMessageW( g_hWnd_chk_enable_drop_window, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					_SendMessageA( g_hWnd_drop_window_transparency, WM_GETTEXT, 4, ( LPARAM )value );
					cfg_drop_window_transparency = ( unsigned char )_strtoul( value, NULL, 10 );

					cfg_show_drop_window_progress = ( _SendMessageW( g_hWnd_chk_show_drop_window_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					if ( cfg_enable_drop_window )
					{
						if ( g_hWnd_url_drop_window == NULL )
						{
							g_hWnd_url_drop_window = _CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_NOACTIVATE | WS_EX_TOPMOST, L"url_drop_window", NULL, WS_CLIPCHILDREN | WS_POPUP, 0, 0, DW_WIDTH, DW_HEIGHT, NULL, NULL, NULL, NULL );
							_SetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE, _GetWindowLongPtrW( g_hWnd_url_drop_window, GWL_EXSTYLE ) | WS_EX_LAYERED );
							_SetLayeredWindowAttributes( g_hWnd_url_drop_window, 0, cfg_drop_window_transparency, LWA_ALPHA );

							// Prevents it from stealing focus.
							_SetWindowPos( g_hWnd_url_drop_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER );
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

					cfg_sort_added_and_updating_items = ( _SendMessageW( g_hWnd_chk_sort_added_and_updating_items, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_expand_added_group_items = ( _SendMessageW( g_hWnd_chk_expand_added_group_items, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_scroll_to_last_item = ( _SendMessageW( g_hWnd_chk_scroll_to_last_item, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_show_gridlines = ( _SendMessageW( g_hWnd_chk_show_gridlines, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_draw_full_rows = ( _SendMessageW( g_hWnd_chk_draw_full_rows, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					cfg_draw_all_rows = ( _SendMessageW( g_hWnd_chk_draw_all_rows, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_show_part_progress = ( _SendMessageW( g_hWnd_chk_show_part_progress, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

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

					_SendMessageA( g_hWnd_default_speed_limit, WM_GETTEXT, 21, ( LPARAM )value );
					cfg_default_speed_limit = strtoull( value );

					_SendMessageA( g_hWnd_thread_count, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned long thread_count = _strtoul( value, NULL, 10 );

					cfg_default_ssl_version = ( unsigned char )_SendMessageW( g_hWnd_default_ssl_version, CB_GETCURSEL, 0, 0 );

					if ( thread_count != cfg_thread_count )
					{
						cfg_thread_count = thread_count;

						display_notice |= 0x04;
					}

					//

					cfg_drag_and_drop_action = ( unsigned char )_SendMessageW( g_hWnd_drag_and_drop_action, CB_GETCURSEL, 0, 0 );

					cfg_prompt_rename = ( unsigned char )_SendMessageW( g_hWnd_prompt_rename, CB_GETCURSEL, 0, 0 );
					cfg_prompt_file_size = ( unsigned char )_SendMessageW( g_hWnd_prompt_file_size, CB_GETCURSEL, 0, 0 );
					_SendMessageA( g_hWnd_max_file_size, WM_GETTEXT, 21, ( LPARAM )value );
					cfg_max_file_size = strtoull( value );
					cfg_prompt_last_modified = ( unsigned char )_SendMessageW( g_hWnd_prompt_last_modified, CB_GETCURSEL, 0, 0 );

					unsigned char shutdown_action = ( unsigned char )_SendMessageW( g_hWnd_shutdown_action, CB_GETCURSEL, 0, 0 );

					if ( shutdown_action != cfg_shutdown_action )
					{
						cfg_shutdown_action = g_shutdown_action = shutdown_action;

						if ( cfg_shutdown_action == SHUTDOWN_ACTION_RESTART ||
							 cfg_shutdown_action == SHUTDOWN_ACTION_SLEEP ||
							 cfg_shutdown_action == SHUTDOWN_ACTION_HIBERNATE ||
							 cfg_shutdown_action == SHUTDOWN_ACTION_SHUT_DOWN ||
							 cfg_shutdown_action == SHUTDOWN_ACTION_HYBRID_SHUT_DOWN )
						{
							display_notice |= 0x08;
						}
					}

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


					// FTP
					cfg_ftp_mode_type = ( _SendMessageW( g_hWnd_chk_active_mode, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					cfg_ftp_enable_fallback_mode = ( _SendMessageW( g_hWnd_chk_fallback_mode, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_ftp_address_type = ( _SendMessageW( g_hWnd_chk_type_ftp_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					unsigned int hostname_length = ( unsigned int )_SendMessageW( g_hWnd_ftp_hostname, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
					if ( cfg_ftp_hostname != NULL )
					{
						GlobalFree( cfg_ftp_hostname );
					}
					cfg_ftp_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
					_SendMessageW( g_hWnd_ftp_hostname, WM_GETTEXT, hostname_length, ( LPARAM )cfg_ftp_hostname );

					_SendMessageW( g_hWnd_ftp_ip_address, IPM_GETADDRESS, 0, ( LPARAM )&cfg_ftp_ip_address );

					_SendMessageA( g_hWnd_ftp_port_start, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_ftp_port_start = ( unsigned short )_strtoul( value, NULL, 10 );

					_SendMessageA( g_hWnd_ftp_port_end, WM_GETTEXT, 6, ( LPARAM )value );
					cfg_ftp_port_end = ( unsigned short )_strtoul( value, NULL, 10 );

					cfg_ftp_send_keep_alive = ( _SendMessageW( g_hWnd_chk_send_keep_alive, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );


					// SFTP
					unsigned short sftp_update = 0x0000;

					bool sftp_enable_compression = ( _SendMessageW( g_hWnd_chk_enable_compression, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					if ( sftp_enable_compression != cfg_sftp_enable_compression ) { sftp_update |= 0x0001; }
					cfg_sftp_enable_compression = sftp_enable_compression;

					bool sftp_attempt_gssapi_authentication = ( _SendMessageW( g_hWnd_chk_attempt_gssapi_authentication, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					if ( sftp_attempt_gssapi_authentication != cfg_sftp_attempt_gssapi_authentication ) { sftp_update |= 0x0002; }
					cfg_sftp_attempt_gssapi_authentication = sftp_attempt_gssapi_authentication;

					bool sftp_attempt_gssapi_key_exchange = ( _SendMessageW( g_hWnd_chk_attempt_gssapi_key_exchange, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );
					if ( sftp_attempt_gssapi_key_exchange != cfg_sftp_attempt_gssapi_key_exchange ) { sftp_update |= 0x0004; }
					cfg_sftp_attempt_gssapi_key_exchange = sftp_attempt_gssapi_key_exchange;

					_SendMessageA( g_hWnd_sftp_keep_alive_time, WM_GETTEXT, 11, ( LPARAM )value );
					int sftp_keep_alive_time = ( int )_strtoul( value, NULL, 10 );
					if ( sftp_keep_alive_time != cfg_sftp_keep_alive_time ) { sftp_update |= 0x0008; }
					cfg_sftp_keep_alive_time = sftp_keep_alive_time;

					_SendMessageA( g_hWnd_sftp_rekey_time, WM_GETTEXT, 11, ( LPARAM )value );
					int sftp_rekey_time = ( int )_strtoul( value, NULL, 10 );
					if ( sftp_rekey_time != cfg_sftp_rekey_time ) { sftp_update |= 0x0010; }
					cfg_sftp_rekey_time = sftp_rekey_time;

					_SendMessageA( g_hWnd_sftp_gss_rekey_time, WM_GETTEXT, 11, ( LPARAM )value );
					int sftp_gss_rekey_time = ( int )_strtoul( value, NULL, 10 );
					if ( sftp_gss_rekey_time != cfg_sftp_gss_rekey_time ) { sftp_update |= 0x0020; }
					cfg_sftp_gss_rekey_time = sftp_gss_rekey_time;

					_SendMessageA( g_hWnd_sftp_rekey_data_limit, WM_GETTEXT, 11, ( LPARAM )value );
					unsigned long sftp_rekey_data_limit = ( unsigned long )_strtoul( value, NULL, 10 );
					if ( sftp_rekey_data_limit != cfg_sftp_rekey_data_limit ) { sftp_update |= 0x0040; }
					cfg_sftp_rekey_data_limit = sftp_rekey_data_limit;

					char i;
					for ( i = 0; i < KEX_ALGORITHM_COUNT; ++i )
					{
						if ( cfg_priority_kex_algorithm[ i ] != g_priority_kex_algorithm[ i ] )
						{ cfg_priority_kex_algorithm[ i ] = g_priority_kex_algorithm[ i ]; sftp_update |= 0x0080; }
					}
					for ( i = 0; i < HOST_KEY_COUNT; ++i )
					{
						if ( cfg_priority_host_key[ i ] != g_priority_host_key[ i ] )
						{ cfg_priority_host_key[ i ] = g_priority_host_key[ i ]; sftp_update |= 0x0100; }
					}
					for ( i = 0; i < ENCRYPTION_CIPHER_COUNT; ++i )
					{
						if ( cfg_priority_encryption_cipher[ i ] != g_priority_encryption_cipher[ i ] )
						{ cfg_priority_encryption_cipher[ i ] = g_priority_encryption_cipher[ i ]; sftp_update |= 0x0200; }
					}

					if ( psftp_state == PSFTP_STATE_RUNNING )
					{
						if ( sftp_update & 0x0001 ) { _SFTP_SetConfigInfo( 0, ( cfg_sftp_enable_compression ? 1 : 0 ) ); }
						if ( sftp_update & 0x0002 ) { _SFTP_SetConfigInfo( 1, ( cfg_sftp_attempt_gssapi_authentication ? 1 : 0 ) ); }
						if ( sftp_update & 0x0004 ) { _SFTP_SetConfigInfo( 2, ( cfg_sftp_attempt_gssapi_key_exchange ? 1 : 0 ) ); }

						if ( sftp_update & 0x0008 ) { _SFTP_SetConfigInfo( 3, cfg_sftp_keep_alive_time ); }
						if ( sftp_update & 0x0010 ) { _SFTP_SetConfigInfo( 4, cfg_sftp_rekey_time ); }
						if ( sftp_update & 0x0020 ) { _SFTP_SetConfigInfo( 5, cfg_sftp_gss_rekey_time ); }
						if ( sftp_update & 0x0040 ) { _SFTP_SetConfigInfo( 6, cfg_sftp_rekey_data_limit ); }

						if ( sftp_update & 0x0080 ) { _SFTP_SetAlgorithmPriorities( 0, cfg_priority_kex_algorithm, KEX_ALGORITHM_COUNT ); }
						if ( sftp_update & 0x0100 ) { _SFTP_SetAlgorithmPriorities( 1, cfg_priority_host_key, HOST_KEY_COUNT ); }
						if ( sftp_update & 0x0200 ) { _SFTP_SetAlgorithmPriorities( 2, cfg_priority_encryption_cipher, ENCRYPTION_CIPHER_COUNT ); }
					}

					//
					// HTTP proxy.
					//
					cfg_enable_proxy = ( _SendMessageW( g_hWnd_chk_proxy, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

					cfg_address_type = ( _SendMessageW( g_hWnd_chk_type_ip_address, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

					hostname_length = ( unsigned int )_SendMessageW( g_hWnd_hostname, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
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

					if ( site_list_changed )
					{
						save_site_info();

						site_list_changed = false;
					}

					if ( sftp_fps_host_list_changed )
					{
						save_sftp_fps_host_info();

						sftp_fps_host_list_changed = false;
					}

					if ( sftp_keys_host_list_changed )
					{
						save_sftp_keys_host_info();

						sftp_keys_host_list_changed = false;
					}

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

					if ( display_notice == 0x01 || display_notice == 0x02 )
					{
						CMessageBoxW( hWnd, ( display_notice == 0x01 ? ST_V_A_restart_is_required_enable_allocation : ST_V_A_restart_is_required_disable_allocation ), PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
					}
					else if ( display_notice == 0x04 )
					{
						CMessageBoxW( hWnd, ST_V_A_restart_is_required_threads, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
					}
					else if ( display_notice == 0x08 )	
					{
						CMessageBoxW( hWnd, ST_V_A_restart_is_required_shutdown, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
					}
					else if ( display_notice & ( 0x01 | 0x02 | 0x04 | 0x08 ) )	// Multiple settings changed.
					{
						CMessageBoxW( hWnd, ST_V_A_restart_is_required, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONINFORMATION );
					}

					options_state_changed = false;

					if ( LOWORD( wParam ) == BTN_APPLY )
					{
						_EnableWindow( g_hWnd_options_apply, FALSE );
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

			_SetFocus( g_hWnd_options_tree );

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
	//return TRUE;
}
