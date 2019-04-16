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

#include "options.h"
#include "lite_comdlg32.h"
#include "lite_gdi32.h"

#include "connection.h"

#define EDIT_MAX_DOWNLOADS				1000
#define EDIT_RETRY_DOWNLOADS_COUNT		1001
#define EDIT_DEFAULT_DOWNLOAD_PARTS		1002
#define EDIT_RETRY_PARTS_COUNT			1003
#define EDIT_TIMEOUT					1004
#define EDIT_MAX_REDIRECTS				1005
#define CB_DEFAULT_SSL_VERSION			1006

#define BTN_ENABLE_SERVER				1007
#define BTN_TYPE_SERVER_HOST			1008
#define BTN_TYPE_SERVER_IP_ADDRESS		1009
#define EDIT_SERVER_HOST				1010
#define EDIT_SERVER_IP_ADDRESS			1011
#define EDIT_SERVER_PORT				1012
#define BTN_USE_AUTHENTICATION			1013
#define EDIT_AUTHENTICATION_USERNAME	1014
#define EDIT_AUTHENTICATION_PASSWORD	1015
#define BTN_AUTH_TYPE_BASIC				1016
#define BTN_AUTH_TYPE_DIGEST			1017

#define BTN_SERVER_ENABLE_SSL			1018
#define BTN_TYPE_PKCS					1019
#define BTN_TYPE_PAIR					1020
#define EDIT_CERTIFICATE_PKCS			1021
#define BTN_CERTIFICATE_PKCS			1022
#define EDIT_CERTIFICATE_PKCS_PASSWORD	1023
#define EDIT_CERTIFICATE_CER			1024
#define BTN_CERTIFICATE_CER				1025
#define EDIT_CERTIFICATE_KEY			1026
#define BTN_CERTIFICATE_KEY				1027
#define CB_SERVER_SSL_VERSION			1028

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

LRESULT CALLBACK ConnectionTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			HWND hWnd_static_max_downloads = _CreateWindowW( WC_STATIC, ST_V_Active_download_limit_, WS_CHILD | WS_VISIBLE, 0, 0, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_downloads = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 15, 100, 23, hWnd, ( HMENU )EDIT_MAX_DOWNLOADS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_max_downloads = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 100, 14, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_downloads, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_downloads, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_max_downloads, UDM_SETPOS, 0, cfg_max_downloads );


			HWND hWnd_static_retry_downloads_count = _CreateWindowW( WC_STATIC, ST_V_Retry_incomplete_downloads_, WS_CHILD | WS_VISIBLE, 200, 0, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_downloads_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 200, 15, 100, 23, hWnd, ( HMENU )EDIT_RETRY_DOWNLOADS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_downloads_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 300, 14, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_downloads_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_downloads_count, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_downloads_count, UDM_SETPOS, 0, cfg_retry_downloads_count );

			//

			HWND hWnd_static_download_parts = _CreateWindowW( WC_STATIC, ST_V_Default_download_parts_, WS_CHILD | WS_VISIBLE, 0, 45, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 60, 100, 23, hWnd, ( HMENU )EDIT_DEFAULT_DOWNLOAD_PARTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 100, 59, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_default_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_default_download_parts, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );


			HWND hWnd_static_retry_parts_count = _CreateWindowW( WC_STATIC, ST_V_Retry_incomplete_parts_, WS_CHILD | WS_VISIBLE, 200, 45, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_retry_parts_count = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 200, 60, 100, 23, hWnd, ( HMENU )EDIT_RETRY_PARTS_COUNT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_retry_parts_count = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 300, 59, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_retry_parts_count, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBUDDY, ( WPARAM )g_hWnd_retry_parts_count, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_retry_parts_count, UDM_SETPOS, 0, cfg_retry_parts_count );

			//

			HWND hWnd_static_timeout = _CreateWindowW( WC_STATIC, ST_V_Timeout__seconds__, WS_CHILD | WS_VISIBLE, 0, 90, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_timeout = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 105, 100, 23, hWnd, ( HMENU )EDIT_TIMEOUT, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_timeout = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 100, 104, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_timeout, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBUDDY, ( WPARAM )g_hWnd_timeout, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETRANGE32, 10, 300 );
			_SendMessageW( hWnd_ud_timeout, UDM_SETPOS, 0, cfg_timeout );
			if ( cfg_timeout == 0 )
			{
				_SendMessageW( g_hWnd_timeout, WM_SETTEXT, 0, ( LPARAM )L"0" );
			}


			HWND hWnd_static_max_redirects = _CreateWindowW( WC_STATIC, ST_V_Maximum_redirects_, WS_CHILD | WS_VISIBLE, 200, 90, 190, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_max_redirects = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 200, 105, 100, 23, hWnd, ( HMENU )EDIT_MAX_REDIRECTS, NULL, NULL );

			// Keep this unattached. Looks ugly inside the text box.
			HWND hWnd_ud_max_redirects = _CreateWindowW( UPDOWN_CLASS, NULL, /*UDS_ALIGNRIGHT |*/ UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE, 300, 104, _GetSystemMetrics( SM_CXVSCROLL ), 25, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_max_redirects, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETBUDDY, ( WPARAM )g_hWnd_max_redirects, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETBASE, 10, 0 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETRANGE32, 0, 100 );
			_SendMessageW( hWnd_ud_max_redirects, UDM_SETPOS, 0, cfg_max_redirects );

			//

			HWND hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_V_Default_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 138, rc.right - 10, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_default_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 153, 100, 23, hWnd, ( HMENU )CB_DEFAULT_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_default_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_default_ssl_version, CB_SETCURSEL, cfg_default_ssl_version, 0 );

			//

			g_hWnd_chk_enable_server = _CreateWindowW( WC_BUTTON, ST_V_Enable_server_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 183, rc.right - 10, 20, hWnd, ( HMENU )BTN_ENABLE_SERVER, NULL, NULL );

			g_hWnd_static_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 208, rc.right - 10, 5, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_type_server_hostname = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 213, 200, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_HOST, NULL, NULL );
			g_hWnd_chk_type_server_ip_address = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 213, 110, 20, hWnd, ( HMENU )BTN_TYPE_SERVER_IP_ADDRESS, NULL, NULL );

			g_hWnd_server_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 233, 310, 23, hWnd, ( HMENU )EDIT_SERVER_HOST, NULL, NULL );
			g_hWnd_server_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 233, 310, 23, hWnd, ( HMENU )EDIT_SERVER_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_server_colon = _CreateWindowW( WC_STATIC, ST_V__, WS_CHILD | WS_VISIBLE, 314, 237, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_server_port = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 218, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 233, 75, 23, hWnd, ( HMENU )EDIT_SERVER_PORT, NULL, NULL );

			g_hWnd_chk_use_authentication = _CreateWindowW( WC_BUTTON, ST_V_Require_authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 263, rc.right - 10, 20, hWnd, ( HMENU )BTN_USE_AUTHENTICATION, NULL, NULL );

			g_hWnd_chk_authentication_type_basic = _CreateWindowW( WC_BUTTON, ST_V_Basic_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 15, 285, 150, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_BASIC, NULL, NULL );
			g_hWnd_chk_authentication_type_digest = _CreateWindowW( WC_BUTTON, ST_V_Digest_Authentication, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 170, 285, 150, 20, hWnd, ( HMENU )BTN_AUTH_TYPE_DIGEST, NULL, NULL );

			g_hWnd_static_authentication_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 15, 308, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 323, 150, 23, hWnd, ( HMENU )EDIT_AUTHENTICATION_USERNAME, NULL, NULL );

			g_hWnd_static_authentication_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 175, 308, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_authentication_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 323, 150, 23, hWnd, ( HMENU )EDIT_AUTHENTICATION_PASSWORD, NULL, NULL );


			g_hWnd_chk_server_enable_ssl = _CreateWindowW( WC_BUTTON, ST_V_Enable_SSL___TLS_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 353, rc.right - 10, 20, hWnd, ( HMENU )BTN_SERVER_ENABLE_SSL, NULL, NULL );

			g_hWnd_static_hoz2 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 378, rc.right - 10, 5, hWnd, NULL, NULL, NULL );


			g_hWnd_chk_type_pkcs = _CreateWindowW( WC_BUTTON, ST_V_PKCS_NUM12_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 383, 100, 20, hWnd, ( HMENU )BTN_TYPE_PKCS, NULL, NULL );
			g_hWnd_chk_type_pair = _CreateWindowW( WC_BUTTON, ST_V_Public___Private_key_pair_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 110, 383, 250, 20, hWnd, ( HMENU )BTN_TYPE_PAIR, NULL, NULL );


			g_hWnd_static_certificate_pkcs_location = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_file_, WS_CHILD | WS_VISIBLE, 15, 408, rc.right - 65, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 423, rc.right - 65, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS, NULL, NULL );
			g_hWnd_btn_certificate_pkcs_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 423, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_PKCS, NULL, NULL );

			g_hWnd_static_certificate_pkcs_password = _CreateWindowW( WC_STATIC, ST_V_PKCS_NUM12_password_, WS_CHILD | WS_VISIBLE, 15, 451, rc.right - 65, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_pkcs_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_PASSWORD | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 466, rc.right - 65, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_PKCS_PASSWORD, NULL, NULL );

			g_hWnd_static_certificate_cer_location = _CreateWindowW( WC_STATIC, ST_V_Certificate_file_, WS_CHILD | WS_VISIBLE, 15, 408, rc.right - 65, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_cer_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 423, rc.right - 65, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_CER, NULL, NULL );
			g_hWnd_btn_certificate_cer_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 423, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_CER, NULL, NULL );

			g_hWnd_static_certificate_key_location = _CreateWindowW( WC_STATIC, ST_V_Key_file_, WS_CHILD | WS_VISIBLE, 15, 451, rc.right - 65, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_certificate_key_location = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 466, rc.right - 65, 23, hWnd, ( HMENU )EDIT_CERTIFICATE_KEY, NULL, NULL );
			g_hWnd_btn_certificate_key_location = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_VISIBLE, rc.right - 45, 466, 35, 23, hWnd, ( HMENU )BTN_CERTIFICATE_KEY, NULL, NULL );

			g_hWnd_static_server_ssl_version = _CreateWindowW( WC_STATIC, ST_V_Server_SSL___TLS_version_, WS_CHILD | WS_VISIBLE, 0, 496, rc.right - 10, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_server_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE, 0, 511, 100, 23, hWnd, ( HMENU )CB_SERVER_SSL_VERSION, NULL, NULL );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_server_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );

			_SendMessageW( g_hWnd_server_hostname, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_server_port, EM_LIMITTEXT, 5, 0 );
			_SendMessageW( g_hWnd_certificate_pkcs_password, EM_LIMITTEXT, 1024, 0 );	// 1024 characters + 1 NULL

			//

			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 534 + 13;	// Value is the position and height of the bottom most control. Needs 13px more padding for Windows 10.
			si.nPage = ( rc.bottom - rc.top );
			si.nPos = 0;
			_SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

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

			_SendMessageW( hWnd_static_max_downloads, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_max_downloads, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_retry_downloads_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_retry_downloads_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_retry_parts_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_retry_parts_count, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_default_ssl_version, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_default_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_ud_download_parts, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_timeout, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_timeout, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( hWnd_ud_timeout, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( hWnd_static_max_redirects, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_max_redirects, WM_SETFONT, ( WPARAM )g_hFont, 0 );

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
			SCROLLINFO si;
			si.cbSize = sizeof( SCROLLINFO );
			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			int delta = si.nPos;

			if ( msg == WM_VSCROLL )
			{
				// Only process the standard scroll bar.
				if ( lParam != NULL )
				{
					return _DefWindowProcW( hWnd, msg, wParam, lParam );
				}

				switch ( LOWORD( wParam ) )
				{
					case SB_LINEUP: { si.nPos -= 10; } break;
					case SB_LINEDOWN: { si.nPos += 10; } break;
					case SB_PAGEUP: { si.nPos -= 50; } break;
					case SB_PAGEDOWN: { si.nPos += 50; } break;
					//case SB_THUMBPOSITION:
					case SB_THUMBTRACK: { si.nPos = ( int )HIWORD( wParam ); } break;
					default: { return 0; } break;
				}
			}
			else if ( msg == WM_MOUSEWHEEL )
			{
				si.nPos -= ( GET_WHEEL_DELTA_WPARAM( wParam ) / WHEEL_DELTA ) * 20;
			}

			_SetScrollPos( hWnd, SB_VERT, si.nPos, TRUE );

			si.fMask = SIF_POS;
			_GetScrollInfo( hWnd, SB_VERT, &si );

			if ( si.nPos != delta )
			{
				_ScrollWindow( hWnd, 0, delta - si.nPos, NULL, NULL );
			}

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
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
	return TRUE;
}
