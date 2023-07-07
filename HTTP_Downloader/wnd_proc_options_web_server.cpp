/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2023 Eric Kutcher

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

#include "lite_comdlg32.h"
#include "lite_gdi32.h"
#include "connection.h"

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

// Free these when done.
wchar_t *certificate_pkcs_file_name = NULL;
wchar_t *certificate_cer_file_name = NULL;
wchar_t *certificate_key_file_name = NULL;

unsigned int certificate_pkcs_file_name_length = 0;
unsigned int certificate_cer_file_name_length = 0;
unsigned int certificate_key_file_name_length = 0;

HFONT hFont_copy_connection = NULL;

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

LRESULT CALLBACK WebServerTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			g_hWnd_chk_enable_server = _CreateWindowW( WC_BUTTON, ST_V_Enable_server_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, rc.right - 10, 20, hWnd, ( HMENU )BTN_ENABLE_SERVER, NULL, NULL );

			g_hWnd_static_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 25, rc.right, 1, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_type_server_hostname = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 30, 200, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_HOST, NULL, NULL );
			g_hWnd_chk_type_server_ip_address = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 30, 110, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_IP_ADDRESS, NULL, NULL );

			g_hWnd_server_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 50, 310, 23, hWnd, ( HMENU )EDIT_SERVER_HOST, NULL, NULL );
			g_hWnd_server_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 50, 310, 23, hWnd, ( HMENU )EDIT_SERVER_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_server_colon = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD | WS_VISIBLE, 310, 54, 10, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_static_server_port = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 35, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 50, 75, 23, hWnd, ( HMENU )EDIT_SERVER_PORT, NULL, NULL );

			g_hWnd_chk_use_authentication = _CreateWindowW( WC_BUTTON, ST_V_Require_authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, rc.right - 10, 20, hWnd, ( HMENU )BTN_USE_AUTHENTICATION, NULL, NULL );

			g_hWnd_chk_authentication_type_basic = _CreateWindowW( WC_BUTTON, ST_V_Basic_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 15, 102, 150, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_BASIC, NULL, NULL );
			g_hWnd_chk_authentication_type_digest = _CreateWindowW( WC_BUTTON, ST_V_Digest_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 170, 102, 150, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_DIGEST, NULL, NULL );

			g_hWnd_static_authentication_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 15, 125, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 140, 150, 23, hWnd, ( HMENU )EDIT_AUTHENTICATION_USERNAME, NULL, NULL );

			g_hWnd_static_authentication_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 175, 125, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 140, 150, 23, hWnd, ( HMENU )EDIT_AUTHENTICATION_PASSWORD, NULL, NULL );


			g_hWnd_chk_server_enable_ssl = _CreateWindowW( WC_BUTTON, ST_V_Enable_SSL___TLS_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 170, rc.right - 10, 20, hWnd, ( HMENU )BTN_SERVER_ENABLE_SSL, NULL, NULL );

			g_hWnd_static_hoz2 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 195, rc.right, 1, hWnd, NULL, NULL, NULL );


			g_hWnd_chk_type_pkcs = _CreateWindowW( WC_BUTTON, ST_V_PKCS_NUM12_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 200, 100, 20, hWnd, ( HMENU )BTN_TYPE_PKCS, NULL, NULL );
			g_hWnd_chk_type_pair = _CreateWindowW( WC_BUTTON, ST_V_Public___Private_key_pair_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 200, 250, 20, hWnd, ( HMENU )BTN_TYPE_PAIR, NULL, NULL );


			g_hWnd_static_certificate_pkcs_location = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_file_, WS_CHILD | WS_VISIBLE, 15, 225, rc.right - 55, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 240, rc.right - 55, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS, NULL, NULL );
			g_hWnd_btn_certificate_pkcs_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 240, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_PKCS, NULL, NULL );

			g_hWnd_static_certificate_pkcs_password = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_password_, WS_CHILD | WS_VISIBLE, 15, 268, rc.right - 55, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 283, rc.right - 55, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS_PASSWORD, NULL, NULL );

			g_hWnd_static_certificate_cer_location = _CreateWindowW( WC_STATIC, ST_V_Certificate_file_, WS_CHILD | WS_VISIBLE, 15, 225, rc.right - 55, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_cer_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 240, rc.right - 55, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_CER, NULL, NULL );
			g_hWnd_btn_certificate_cer_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 240, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_CER, NULL, NULL );

			g_hWnd_static_certificate_key_location = _CreateWindowW( WC_STATIC, ST_V_Key_file_, WS_CHILD | WS_VISIBLE, 15, 268, rc.right - 55, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_key_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 283, rc.right - 55, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_KEY, NULL, NULL );
			g_hWnd_btn_certificate_key_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 35, 283, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_KEY, NULL, NULL );

			g_hWnd_static_server_ssl_version = _CreateWindowW( WC_STATIC, ST_V_Server_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 317, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | CBS_DARK_MODE, rc.right - 100, 313, 100, 23, hWnd, ( HMENU )CB_SERVER_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_server_hostname, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_server_port, EM_LIMITTEXT, 5, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, EM_LIMITTEXT, 1024, 0 );	// 1024 characters + 1 NULL

			//

			_SendMessageW( g_hWnd_chk_enable_server, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_server_hostname, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_server_ip_address, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_server_hostname, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_server_colon, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_server_port, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_server_port, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_use_authentication, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_authentication_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_authentication_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_static_authentication_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_authentication_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_basic, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_authentication_type_digest, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_server_enable_ssl, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_pkcs, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_pair, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_pkcs_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_pkcs_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_cer_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_certificate_cer_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_cer_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_certificate_key_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_certificate_key_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_btn_certificate_key_location, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_server_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_server_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			//LOGFONT lf;
			//_memzero( &lf, sizeof( LOGFONT ) );
			//_GetObjectW( g_hFont, sizeof( LOGFONT ), &lf );
			hFont_copy_connection = _CreateFontIndirectW( &g_default_log_font );
			_SendMessageW( g_hWnd_server_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_connection, 0 );


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

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
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
						_EnableWindow( g_hWnd_options_apply, TRUE );
					}
				}
				break;

				case EDIT_SERVER_IP_ADDRESS:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_AUTH_TYPE_BASIC:
				case BTN_AUTH_TYPE_DIGEST:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
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

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"Personal Information Exchange (*.pfx;*.p12)\0*.pfx;*.p12\0All Files (*.*)\0*.*\0";
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

							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
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

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"X.509 Certificate (*.cer;*.crt)\0*.cer;*.crt\0All Files (*.*)\0*.*\0";
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

							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
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

						OPENFILENAME ofn;
						_memzero( &ofn, sizeof( OPENFILENAME ) );
						ofn.lStructSize = sizeof( OPENFILENAME );
						ofn.hwndOwner = hWnd;
						ofn.lpstrFilter = L"Private Key (*.key)\0*.key\0All Files (*.*)\0*.*\0";
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

							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
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
						options_state_changed = true;
						_EnableWindow( g_hWnd_options_apply, TRUE );
					}
				}
				break;
			}

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
	//return TRUE;
}
