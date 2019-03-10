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
#include "lite_gdi32.h"

#define BTN_PROXY				1001

#define BTN_TYPE_HOST			1002
#define BTN_TYPE_IP_ADDRESS		1003
#define EDIT_HOST				1004
#define EDIT_IP_ADDRESS			1005
#define EDIT_PORT				1006

#define BTN_PROXY_S				1007

#define BTN_TYPE_HOST_S			1008
#define BTN_TYPE_IP_ADDRESS_S	1009
#define EDIT_HOST_S				1010
#define EDIT_IP_ADDRESS_S		1011
#define EDIT_PORT_S				1012

#define EDIT_AUTH_USERNAME		1013
#define EDIT_AUTH_PASSWORD		1014
#define EDIT_AUTH_USERNAME_S	1015
#define EDIT_AUTH_PASSWORD_S	1016

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

			g_hWnd_chk_proxy = _CreateWindowW( WC_BUTTON, ST_V_Enable_HTTP_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, rc.right, 20, hWnd, ( HMENU )BTN_PROXY, NULL, NULL );


			g_hWnd_chk_type_hostname = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 20, 200, 20, hWnd, ( HMENU )BTN_TYPE_HOST, NULL, NULL );
			g_hWnd_chk_type_ip_address = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 20, 110, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS, NULL, NULL );

			g_hWnd_hostname = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 40, 310, 20, hWnd, ( HMENU )EDIT_HOST, NULL, NULL );
			g_hWnd_ip_address = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 40, 310, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS, NULL, NULL );


			g_hWnd_static_colon = _CreateWindowW( WC_STATIC, ST_V__, WS_CHILD | WS_VISIBLE, 314, 42, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 25, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 40, 75, 20, hWnd, ( HMENU )EDIT_PORT, NULL, NULL );


			g_hWnd_static_auth_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 65, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 80, 150, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_auth_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 160, 65, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 160, 80, 150, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD, NULL, NULL );


			_SendMessageW( g_hWnd_hostname, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port, EM_LIMITTEXT, 5, 0 );


			//

			HWND hWnd_static_hoz = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 0, 110, rc.right, 5, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_chk_proxy_s = _CreateWindowW( WC_BUTTON, ST_V_Enable_HTTPS_proxy_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 120, rc.right, 20, hWnd, ( HMENU )BTN_PROXY_S, NULL, NULL );


			g_hWnd_chk_type_hostname_s = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP | WS_VISIBLE, 0, 140, 200, 20, hWnd, ( HMENU )BTN_TYPE_HOST_S, NULL, NULL );
			g_hWnd_chk_type_ip_address_s = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 205, 140, 110, 20, hWnd, ( HMENU )BTN_TYPE_IP_ADDRESS_S, NULL, NULL );

			g_hWnd_hostname_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 160, 310, 20, hWnd, ( HMENU )EDIT_HOST_S, NULL, NULL );
			g_hWnd_ip_address_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 160, 310, 20, hWnd, ( HMENU )EDIT_IP_ADDRESS_S, NULL, NULL );


			g_hWnd_static_colon_s = _CreateWindowW( WC_STATIC, ST_V__, WS_CHILD | WS_VISIBLE, 314, 162, 75, 15, hWnd, NULL, NULL, NULL );


			g_hWnd_static_port_s = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD | WS_VISIBLE, 320, 145, 75, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_port_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 320, 160, 75, 20, hWnd, ( HMENU )EDIT_PORT_S, NULL, NULL );


			g_hWnd_static_auth_username_s = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_VISIBLE, 0, 185, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_username_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 200, 150, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_USERNAME_S, NULL, NULL );

			g_hWnd_static_auth_password_s = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_VISIBLE, 160, 185, 150, 15, hWnd, NULL, NULL, NULL );
			g_hWnd_auth_password_s = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 160, 200, 150, 20, hWnd, ( HMENU )( HMENU )EDIT_AUTH_PASSWORD_S, NULL, NULL );


			_SendMessageW( g_hWnd_hostname_s, EM_LIMITTEXT, 254, 0 );
			_SendMessageW( g_hWnd_port_s, EM_LIMITTEXT, 5, 0 );


			//


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

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			//LOGFONT lf;
			//_memzero( &lf, sizeof( LOGFONT ) );
			//_GetObjectW( g_hFont, sizeof( LOGFONT ), &lf );
			hFont_copy_proxy = _CreateFontIndirectW( &g_default_log_font );
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
