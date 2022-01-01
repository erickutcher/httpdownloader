/*
	HTTP Downloader can download files through HTTP(S), FTP(S), and SFTP connections.
	Copyright (C) 2015-2022 Eric Kutcher

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
#include "connection.h"
#include "lite_gdi32.h"

#define BTN_PROXY					1001

#define BTN_TYPE_HOST				1002
#define BTN_TYPE_IP_ADDRESS			1003
#define EDIT_HOST					1004
#define EDIT_IP_ADDRESS				1005
#define EDIT_PORT					1006

#define EDIT_AUTH_USERNAME			1007
#define EDIT_AUTH_PASSWORD			1008

#define BTN_PROXY_S					1009

#define BTN_TYPE_HOST_S				1010
#define BTN_TYPE_IP_ADDRESS_S		1011
#define EDIT_HOST_S					1012
#define EDIT_IP_ADDRESS_S			1013
#define EDIT_PORT_S					1014

#define EDIT_AUTH_USERNAME_S		1015
#define EDIT_AUTH_PASSWORD_S		1016

#define BTN_PROXY_SOCKS				1017

#define BTN_TYPE_SOCKS4				1018
#define BTN_TYPE_SOCKS5				1019

#define BTN_TYPE_HOST_SOCKS			1020
#define BTN_TYPE_IP_ADDRESS_SOCKS	1021
#define EDIT_HOST_SOCKS				1022
#define EDIT_IP_ADDRESS_SOCKS		1023
#define EDIT_PORT_SOCKS				1024

#define EDIT_AUTH_IDENT_USERNAME_SOCKS	1025

#define BTN_RESOLVE_DOMAIN_NAMES_V4A	1026

#define BTN_AUTHENTICATION_SOCKS	1027

#define EDIT_AUTH_USERNAME_SOCKS	1028
#define EDIT_AUTH_PASSWORD_SOCKS	1029

#define BTN_RESOLVE_DOMAIN_NAMES	1030

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

// SOCKS proxy
HWND g_hWnd_chk_proxy_socks = NULL;

HWND g_hWnd_chk_type_socks4 = NULL;
HWND g_hWnd_chk_type_socks5 = NULL;

HWND g_hWnd_ip_address_socks = NULL;
HWND g_hWnd_hostname_socks = NULL;
HWND g_hWnd_port_socks = NULL;

HWND g_hWnd_static_port_socks = NULL;
HWND g_hWnd_static_colon_socks = NULL;

HWND g_hWnd_chk_type_hostname_socks = NULL;
HWND g_hWnd_chk_type_ip_address_socks = NULL;

HWND g_hWnd_static_auth_ident_username_socks = NULL;
HWND g_hWnd_auth_ident_username_socks = NULL;

HWND g_hWnd_chk_resolve_domain_names_v4a = NULL;

HWND g_hWnd_chk_use_authentication_socks = NULL;

HWND g_hWnd_static_auth_username_socks = NULL;
HWND g_hWnd_auth_username_socks = NULL;
HWND g_hWnd_static_auth_password_socks = NULL;
HWND g_hWnd_auth_password_socks = NULL;

HWND g_hWnd_chk_resolve_domain_names = NULL;

HFONT hFont_copy_proxy = NULL;

LRESULT CALLBACK ProxyTabWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	 switch ( msg )
    {
		case WM_CREATE:
		{
			RECT rc;
			_GetClientRect( hWnd, &rc );

			//

			g_hWnd_chk_proxy = _CreateWindowW( WC_BUTTON, ST_V_Use_HTTP_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, rc.right - 10, 20, hWnd, ( HMENU )BTN_PROXY, NULL, NULL );


			g_hWnd_chk_type_hostname = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 20, 200, 20, hWnd, ( HMENU )BTN_TYPE_HOST, NULL, NULL );
			g_hWnd_chk_type_ip_address = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 20, 110, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS, NULL, NULL );

			g_hWnd_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 310, 23, hWnd, ( HMENU )EDIT_HOST, NULL, NULL );
			g_hWnd_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 40, 310, 23, hWnd, ( HMENU )EDIT_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_colon = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD | WS_VISIBLE, 310, 44, 10, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_static_port = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 25, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 40, 75, 23, hWnd, ( HMENU )EDIT_PORT, NULL, NULL );


			g_hWnd_static_auth_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 68, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 83, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_auth_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 160, 68, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 160, 83, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD, NULL, NULL );


			_SendMessageW( g_hWnd_hostname, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_port, EM_LIMITTEXT, 5, 0 );


			//

			/*HWND hWnd_static_proxy_hoz1 =*/ _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 116, rc.right, 1, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_chk_proxy_s = _CreateWindowW( WC_BUTTON, ST_V_Use_HTTPS_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 126, rc.right - 10, 20, hWnd, ( HMENU )BTN_PROXY_S, NULL, NULL );


			g_hWnd_chk_type_hostname_s = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 146, 200, 20, hWnd, ( HMENU )BTN_TYPE_HOST_S, NULL, NULL );
			g_hWnd_chk_type_ip_address_s = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 146, 110, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS_S, NULL, NULL );

			g_hWnd_hostname_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 166, 310, 23, hWnd, ( HMENU )EDIT_HOST_S, NULL, NULL );
			g_hWnd_ip_address_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 166, 310, 23, hWnd, ( HMENU )EDIT_IP_ADDRESS_S, NULL, NULL );


			g_hWnd_static_colon_s = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD | WS_VISIBLE, 310, 170, 10, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_static_port_s = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 151, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 166, 75, 23, hWnd, ( HMENU )EDIT_PORT_S, NULL, NULL );


			g_hWnd_static_auth_username_s = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 194, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 209, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME_S, NULL, NULL );

			g_hWnd_static_auth_password_s = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 160, 194, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 160, 209, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD_S, NULL, NULL );


			_SendMessageW( g_hWnd_hostname_s, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_port_s, EM_LIMITTEXT, 5, 0 );


			//


			/*HWND hWnd_static_proxy_hoz2 =*/ _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 242, rc.right, 1, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_chk_proxy_socks = _CreateWindowW( WC_BUTTON, ST_V_Use_SOCKS_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 252, 150, 20, hWnd, ( HMENU )BTN_PROXY_SOCKS, NULL, NULL );

			g_hWnd_chk_type_socks4 = _CreateWindowW( WC_BUTTON, ST_V_SOCKS_v4, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 155, 252, 90, 20, hWnd, ( HMENU )BTN_TYPE_SOCKS4, NULL, NULL );
			g_hWnd_chk_type_socks5 = _CreateWindowW( WC_BUTTON, ST_V_SOCKS_v5, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 250, 252, 90, 20, hWnd, ( HMENU )BTN_TYPE_SOCKS5, NULL, NULL );

			g_hWnd_chk_type_hostname_socks = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 272, 200, 20, hWnd, ( HMENU )BTN_TYPE_HOST_SOCKS, NULL, NULL );
			g_hWnd_chk_type_ip_address_socks = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 272, 110, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS_SOCKS, NULL, NULL );

			g_hWnd_hostname_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 292, 310, 23, hWnd, ( HMENU )EDIT_HOST_SOCKS, NULL, NULL );
			g_hWnd_ip_address_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 292, 310, 23, hWnd, ( HMENU )EDIT_IP_ADDRESS_SOCKS, NULL, NULL );


			g_hWnd_static_colon_socks = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD | WS_VISIBLE, 310, 296, 10, 15, hWnd, NULL, NULL, NULL );

			g_hWnd_static_port_socks = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 277, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 292, 75, 23, hWnd, ( HMENU )EDIT_PORT_SOCKS, NULL, NULL );


			g_hWnd_static_auth_ident_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 320, rc.right - 10, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_ident_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 335, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_IDENT_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_chk_resolve_domain_names_v4a = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names_v4a, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 363, rc.right - 10, 20, hWnd, ( HMENU )BTN_RESOLVE_DOMAIN_NAMES_V4A, NULL, NULL );

			g_hWnd_chk_use_authentication_socks = _CreateWindowW( WC_BUTTON, ST_V_Use_Authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 320, rc.right - 10, 20, hWnd, ( HMENU )BTN_AUTHENTICATION_SOCKS, NULL, NULL );

			g_hWnd_static_auth_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 15, 340, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 15, 355, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_static_auth_password_socks = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 175, 340, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 175, 355, 150, 23, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD_SOCKS, NULL, NULL );

			g_hWnd_chk_resolve_domain_names = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 383, rc.right - 10, 20, hWnd, ( HMENU )BTN_RESOLVE_DOMAIN_NAMES, NULL, NULL );

			_SendMessageW( g_hWnd_hostname_socks, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_port_socks, EM_LIMITTEXT, 5, 0 );

			_SendMessageW( g_hWnd_auth_username_socks, EM_LIMITTEXT, 255, 0 );
			_SendMessageW( g_hWnd_auth_password_socks, EM_LIMITTEXT, 255, 0 );


			_SendMessageW( g_hWnd_chk_proxy, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_hostname, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_colon, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_port, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_port, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_username, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_password, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			//

			_SendMessageW( g_hWnd_chk_proxy_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_hostname_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_colon_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_port_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_port_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_username_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_password_s, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			//

			_SendMessageW( g_hWnd_chk_proxy_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_socks4, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_socks5, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_type_hostname_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_chk_type_ip_address_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_hostname_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_colon_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_port_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_port_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_ident_username_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_ident_username_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_resolve_domain_names_v4a, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_use_authentication_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_username_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_username_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_static_auth_password_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );
			_SendMessageW( g_hWnd_auth_password_socks, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			_SendMessageW( g_hWnd_chk_resolve_domain_names, WM_SETFONT, ( WPARAM )g_hFont, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			//LOGFONT lf;
			//_memzero( &lf, sizeof( LOGFONT ) );
			//_GetObjectW( g_hFont, sizeof( LOGFONT ), &lf );
			hFont_copy_proxy = _CreateFontIndirectW( &g_default_log_font );
			_SendMessageW( g_hWnd_ip_address, WM_SETFONT, ( WPARAM )hFont_copy_proxy, 0 );
			_SendMessageW( g_hWnd_ip_address_s, WM_SETFONT, ( WPARAM )hFont_copy_proxy, 0 );
			_SendMessageW( g_hWnd_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_copy_proxy, 0 );

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

			//
			// SOCKS5 proxy.
			//
			_SendMessageW( g_hWnd_chk_proxy_socks, BM_SETCHECK, ( cfg_enable_proxy_socks ? BST_CHECKED : BST_UNCHECKED ), 0 );

			_SendMessageW( g_hWnd_chk_resolve_domain_names_v4a, BM_SETCHECK, ( cfg_resolve_domain_names_v4a ? BST_CHECKED : BST_UNCHECKED ), 0 );
			_SendMessageW( g_hWnd_chk_resolve_domain_names, BM_SETCHECK, ( cfg_resolve_domain_names ? BST_CHECKED : BST_UNCHECKED ), 0 );

			enable = ( cfg_enable_proxy_socks ? TRUE : FALSE );

			if ( cfg_socks_type == SOCKS_TYPE_V4 )
			{
				_SendMessageW( g_hWnd_chk_type_socks4, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_socks5, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_chk_use_authentication_socks, SW_HIDE );

				_ShowWindow( g_hWnd_static_auth_username_socks, SW_HIDE );
				_ShowWindow( g_hWnd_auth_username_socks, SW_HIDE );
				_ShowWindow( g_hWnd_static_auth_password_socks, SW_HIDE );
				_ShowWindow( g_hWnd_auth_password_socks, SW_HIDE );

				_ShowWindow( g_hWnd_chk_resolve_domain_names, SW_HIDE );

				_SendMessageW( g_hWnd_chk_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname_ );

				_ShowWindow( g_hWnd_static_auth_ident_username_socks, SW_SHOW );
				_ShowWindow( g_hWnd_auth_ident_username_socks, SW_SHOW );
				_ShowWindow( g_hWnd_chk_resolve_domain_names_v4a, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_socks4, BM_SETCHECK, BST_UNCHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_socks5, BM_SETCHECK, BST_CHECKED, 0 );

				_ShowWindow( g_hWnd_static_auth_ident_username_socks, SW_HIDE );
				_ShowWindow( g_hWnd_auth_ident_username_socks, SW_HIDE );
				_ShowWindow( g_hWnd_chk_resolve_domain_names_v4a, SW_HIDE );

				_SendMessageW( g_hWnd_chk_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

				_ShowWindow( g_hWnd_chk_use_authentication_socks, SW_SHOW );

				_ShowWindow( g_hWnd_static_auth_username_socks, SW_SHOW );
				_ShowWindow( g_hWnd_auth_username_socks, SW_SHOW );
				_ShowWindow( g_hWnd_static_auth_password_socks, SW_SHOW );
				_ShowWindow( g_hWnd_auth_password_socks, SW_SHOW );

				_ShowWindow( g_hWnd_chk_resolve_domain_names, SW_SHOW );
			}

			_EnableWindow( g_hWnd_chk_type_socks4, enable );
			_EnableWindow( g_hWnd_chk_type_socks5, enable );
			_EnableWindow( g_hWnd_chk_type_hostname_socks, enable );
			_EnableWindow( g_hWnd_chk_type_ip_address_socks, enable );
			_EnableWindow( g_hWnd_hostname_socks, enable );
			_EnableWindow( g_hWnd_ip_address_socks, enable );
			_EnableWindow( g_hWnd_static_colon_socks, enable );
			_EnableWindow( g_hWnd_static_port_socks, enable );
			_EnableWindow( g_hWnd_port_socks, enable );
			_EnableWindow( g_hWnd_chk_use_authentication_socks, enable );
			_EnableWindow( g_hWnd_chk_resolve_domain_names, enable );
			_EnableWindow( g_hWnd_static_auth_ident_username_socks, enable );
			_EnableWindow( g_hWnd_auth_ident_username_socks, enable );
			_EnableWindow( g_hWnd_chk_resolve_domain_names_v4a, enable );

			if ( cfg_enable_proxy_socks && !cfg_use_authentication_socks )
			{
				enable = FALSE;
			}

			_EnableWindow( g_hWnd_static_auth_username_socks, enable );
			_EnableWindow( g_hWnd_auth_username_socks, enable );
			_EnableWindow( g_hWnd_static_auth_password_socks, enable );
			_EnableWindow( g_hWnd_auth_password_socks, enable );

			_SendMessageW( g_hWnd_chk_use_authentication_socks, BM_SETCHECK, ( cfg_use_authentication_socks ? BST_CHECKED : BST_UNCHECKED ), 0 );

			if ( cfg_address_type_socks == 1 )
			{
				_SendMessageW( g_hWnd_chk_type_ip_address_socks, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_hostname_socks, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_hostname_socks, SW_HIDE );
				_ShowWindow( g_hWnd_ip_address_socks, SW_SHOW );
			}
			else
			{
				_SendMessageW( g_hWnd_chk_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );
				_SendMessageW( g_hWnd_chk_type_ip_address_socks, BM_SETCHECK, BST_UNCHECKED, 0 );

				_ShowWindow( g_hWnd_ip_address_socks, SW_HIDE );
				_ShowWindow( g_hWnd_hostname_socks, SW_SHOW );
			}

			if ( cfg_hostname_socks == NULL )
			{
				_SendMessageW( g_hWnd_hostname_socks, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
			}
			else
			{
				_SendMessageW( g_hWnd_hostname_socks, WM_SETTEXT, 0, ( LPARAM )cfg_hostname_socks );
			}

			_SendMessageW( g_hWnd_ip_address_socks, IPM_SETADDRESS, 0, cfg_ip_address_socks );

			_memzero( value, sizeof( char ) * 6 );
			__snprintf( value, 6, "%hu", cfg_port_socks );
			_SendMessageA( g_hWnd_port_socks, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_auth_username_socks, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_username_socks );
			_SendMessageW( g_hWnd_auth_password_socks, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_password_socks );

			_SendMessageW( g_hWnd_auth_ident_username_socks, WM_SETTEXT, 0, ( LPARAM )cfg_proxy_auth_ident_username_socks );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_PROXY_SOCKS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_proxy_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_chk_type_socks4, enable );
					_EnableWindow( g_hWnd_chk_type_socks5, enable );
					_EnableWindow( g_hWnd_chk_type_hostname_socks, enable );
					_EnableWindow( g_hWnd_chk_type_ip_address_socks, enable );
					_EnableWindow( g_hWnd_hostname_socks, enable );
					_EnableWindow( g_hWnd_ip_address_socks, enable );
					_EnableWindow( g_hWnd_static_colon_socks, enable );
					_EnableWindow( g_hWnd_static_port_socks, enable );
					_EnableWindow( g_hWnd_port_socks, enable );
					_EnableWindow( g_hWnd_chk_use_authentication_socks, enable );
					_EnableWindow( g_hWnd_chk_resolve_domain_names, enable );
					_EnableWindow( g_hWnd_static_auth_ident_username_socks, enable );
					_EnableWindow( g_hWnd_auth_ident_username_socks, enable );
					_EnableWindow( g_hWnd_chk_resolve_domain_names_v4a, enable );

					if ( enable == TRUE && ( _SendMessageW( g_hWnd_chk_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED ? TRUE : FALSE ) )
					{
						enable = FALSE;
					}

					_EnableWindow( g_hWnd_static_auth_username_socks, enable );
					_EnableWindow( g_hWnd_auth_username_socks, enable );
					_EnableWindow( g_hWnd_static_auth_password_socks, enable );
					_EnableWindow( g_hWnd_auth_password_socks, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_AUTHENTICATION_SOCKS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_static_auth_username_socks, enable );
					_EnableWindow( g_hWnd_auth_username_socks, enable );
					_EnableWindow( g_hWnd_static_auth_password_socks, enable );
					_EnableWindow( g_hWnd_auth_password_socks, enable );

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case EDIT_PORT:
				case EDIT_PORT_S:
				case EDIT_PORT_SOCKS:
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

						if ( ( LOWORD( wParam ) == EDIT_PORT	&& num != cfg_port ) ||
							 ( LOWORD( wParam ) == EDIT_PORT_S	&& num != cfg_port_s ) ||
							 ( LOWORD( wParam ) == EDIT_PORT_SOCKS	&& num != cfg_port_socks ) )
						{
							options_state_changed = true;
							_EnableWindow( g_hWnd_options_apply, TRUE );
						}
					}
				}
				break;

				case EDIT_HOST:
				case EDIT_AUTH_USERNAME:
				case EDIT_AUTH_PASSWORD:
				case EDIT_HOST_S:
				case EDIT_AUTH_USERNAME_S:
				case EDIT_AUTH_PASSWORD_S:
				case EDIT_HOST_SOCKS:
				case EDIT_AUTH_IDENT_USERNAME_SOCKS:
				case EDIT_AUTH_USERNAME_SOCKS:
				case EDIT_AUTH_PASSWORD_SOCKS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_options_apply, TRUE );
					}
				}
				break;

				case EDIT_IP_ADDRESS:
				case EDIT_IP_ADDRESS_S:
				case EDIT_IP_ADDRESS_SOCKS:
				{
					if ( HIWORD( wParam ) == EN_CHANGE )
					{
						options_state_changed = true;
						_EnableWindow( g_hWnd_options_apply, TRUE );
					}
				}
				break;

				case BTN_RESOLVE_DOMAIN_NAMES_V4A:
				case BTN_RESOLVE_DOMAIN_NAMES:
				{
					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_TYPE_HOST_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_hostname_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_ip_address_socks, SW_HIDE );
						_ShowWindow( g_hWnd_hostname_socks, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
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
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_TYPE_IP_ADDRESS_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_hostname_socks, SW_HIDE );
						_ShowWindow( g_hWnd_ip_address_socks, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_TYPE_SOCKS4:
				{
					if ( _SendMessageW( g_hWnd_chk_type_socks4, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_chk_use_authentication_socks, SW_HIDE );

						_ShowWindow( g_hWnd_static_auth_username_socks, SW_HIDE );
						_ShowWindow( g_hWnd_auth_username_socks, SW_HIDE );
						_ShowWindow( g_hWnd_static_auth_password_socks, SW_HIDE );
						_ShowWindow( g_hWnd_auth_password_socks, SW_HIDE );

						_ShowWindow( g_hWnd_chk_resolve_domain_names, SW_HIDE );

						_SendMessageW( g_hWnd_chk_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname_ );

						_ShowWindow( g_hWnd_static_auth_ident_username_socks, SW_SHOW );
						_ShowWindow( g_hWnd_auth_ident_username_socks, SW_SHOW );
						_ShowWindow( g_hWnd_chk_resolve_domain_names_v4a, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;

				case BTN_TYPE_SOCKS5:
				{
					if ( _SendMessageW( g_hWnd_chk_type_socks5, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_static_auth_ident_username_socks, SW_HIDE );
						_ShowWindow( g_hWnd_auth_ident_username_socks, SW_HIDE );
						_ShowWindow( g_hWnd_chk_resolve_domain_names_v4a, SW_HIDE );

						_SendMessageW( g_hWnd_chk_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

						_ShowWindow( g_hWnd_chk_use_authentication_socks, SW_SHOW );

						_ShowWindow( g_hWnd_static_auth_username_socks, SW_SHOW );
						_ShowWindow( g_hWnd_auth_username_socks, SW_SHOW );
						_ShowWindow( g_hWnd_static_auth_password_socks, SW_SHOW );
						_ShowWindow( g_hWnd_auth_password_socks, SW_SHOW );

						_ShowWindow( g_hWnd_chk_resolve_domain_names, SW_SHOW );
					}

					options_state_changed = true;
					_EnableWindow( g_hWnd_options_apply, TRUE );
				}
				break;
			}

			return 0;
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
	//return TRUE;
}
