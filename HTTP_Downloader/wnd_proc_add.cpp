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
#include "utilities.h"
#include "connection.h"

#include "lite_ole32.h"
#include "lite_gdi32.h"
#include "lite_uxtheme.h"
#include "lite_normaliz.h"
#include "lite_pcre2.h"

#include "categories.h"
#include "list_operations.h"

#include "drag_and_drop.h"
#include "folder_browser.h"

#include "string_tables.h"

#include "wnd_proc.h"
#include "cmessagebox.h"

#include "dark_mode.h"

#define ADD_URLS_WIDTH						620
#define MIN_ADVANCED_HEIGHT					513
#define MIN_SIMPLE_HEIGHT					270

#define BTN_DOWNLOAD						1000
#define BTN_ADD_CANCEL						1001
#define EDIT_DOWNLOAD_PARTS					1002
#define EDIT_ADD_SPEED_LIMIT				1003
#define CB_ADD_CATEGORY						1004
#define CHK_ADD_ENABLE_DOWNLOAD_DIRECTORY	1005
#define BTN_DOWNLOAD_DIRECTORY				1006
#define CHK_ADD_ENABLE_DOWNLOAD_PARTS		1007
#define CHK_ADD_ENABLE_SPEED_LIMIT			1008
#define CHK_SEND_DATA						1009
#define CHK_SIMULATE_DOWNLOAD				1010
#define BTN_ADVANCED						1011

#define CB_REGEX_FILTER_PRESET				1012
#define BTN_APPLY_FILTER					1013

#define EDIT_ADD_URLS						1014

#define BTN_ADD_DOWNLOAD					1015
#define BTN_VERIFY_DOWNLOAD					1016

//

#define CB_ADD_PROXY_TYPE					1017

#define BTN_ADD_TYPE_HOST_SOCKS				1018
#define BTN_ADD_TYPE_IP_ADDRESS_SOCKS		1019
#define EDIT_ADD_HOST_SOCKS					1020
#define EDIT_ADD_IP_ADDRESS_SOCKS			1021
#define EDIT_ADD_PORT_SOCKS					1022

#define EDIT_ADD_PROXY_AUTH_USERNAME		1023
#define EDIT_ADD_PROXY_AUTH_PASSWORD		1024

#define EDIT_ADD_AUTH_IDENT_USERNAME_SOCKS	1025

#define BTN_ADD_RESOLVE_DOMAIN_NAMES_V4A	1026

#define BTN_ADD_AUTHENTICATION_SOCKS		1027

#define EDIT_ADD_AUTH_USERNAME_SOCKS		1028
#define EDIT_ADD_AUTH_PASSWORD_SOCKS		1029

#define BTN_ADD_RESOLVE_DOMAIN_NAMES		1030

//

#define MENU_ADD_SPLIT_DOWNLOAD				10000
#define MENU_ADD_SPLIT_ADD					10001
#define MENU_ADD_SPLIT_VERIFY				10002

HWND g_hWnd_add_urls = NULL;

HWND g_hWnd_static_urls = NULL;
HWND g_hWnd_edit_add = NULL;

HWND g_hWnd_static_au_category = NULL;
HWND g_hWnd_au_category = NULL;

HWND g_hWnd_chk_add_enable_download_directory = NULL;
HWND g_hWnd_download_directory = NULL;
HWND g_hWnd_btn_download_directory = NULL;

HWND g_hWnd_chk_simulate_download = NULL;

HWND g_hWnd_chk_add_enable_download_parts = NULL;
HWND g_hWnd_download_parts = NULL;
HWND g_hWnd_ud_download_parts = NULL;

HWND g_hWnd_chk_add_enable_speed_limit = NULL;
HWND g_hWnd_add_speed_limit = NULL;

HWND g_hWnd_static_ssl_version = NULL;
HWND g_hWnd_ssl_version = NULL;

HWND g_hWnd_btn_authentication = NULL;
HWND g_hWnd_static_username = NULL;
HWND g_hWnd_edit_username = NULL;
HWND g_hWnd_static_password = NULL;
HWND g_hWnd_edit_password = NULL;

HWND g_hWnd_advanced_add_tab = NULL;

HWND g_hWnd_static_comments = NULL;
HWND g_hWnd_edit_comments = NULL;

HWND g_hWnd_static_cookies = NULL;
HWND g_hWnd_edit_cookies = NULL;

HWND g_hWnd_static_headers = NULL;
HWND g_hWnd_edit_headers = NULL;

HWND g_hWnd_chk_send_data = NULL;
HWND g_hWnd_edit_data = NULL;

HWND g_hWnd_chk_show_advanced_options = NULL;

HWND g_hWnd_btn_download = NULL;
HWND g_hWnd_btn_add_download = NULL;
HWND g_hWnd_btn_verify_download = NULL;
HWND g_hWnd_cancel = NULL;

HWND g_hWnd_static_regex_filter = NULL;
HWND g_hWnd_regex_filter_preset = NULL;
HWND g_hWnd_regex_filter = NULL;
HWND g_hWnd_btn_apply_filter = NULL;

//////

HWND g_hWnd_static_add_proxy_type = NULL;
HWND g_hWnd_add_proxy_type = NULL;

HWND g_hWnd_static_add_hoz1 = NULL;

HWND g_hWnd_add_ip_address_socks = NULL;
HWND g_hWnd_add_hostname_socks = NULL;
HWND g_hWnd_add_port_socks = NULL;

HWND g_hWnd_static_add_port_socks = NULL;
HWND g_hWnd_static_add_colon_socks = NULL;

HWND g_hWnd_chk_add_type_hostname_socks = NULL;
HWND g_hWnd_chk_add_type_ip_address_socks = NULL;

HWND g_hWnd_static_add_proxy_auth_username = NULL;
HWND g_hWnd_edit_add_proxy_auth_username = NULL;
HWND g_hWnd_static_add_proxy_auth_password = NULL;
HWND g_hWnd_edit_add_proxy_auth_password = NULL;

HWND g_hWnd_static_add_auth_ident_username_socks = NULL;
HWND g_hWnd_add_auth_ident_username_socks = NULL;

HWND g_hWnd_chk_add_resolve_domain_names_v4a = NULL;

HWND g_hWnd_chk_add_use_authentication_socks = NULL;

HWND g_hWnd_static_add_auth_username_socks = NULL;
HWND g_hWnd_add_auth_username_socks = NULL;
HWND g_hWnd_static_add_auth_password_socks = NULL;
HWND g_hWnd_add_auth_password_socks = NULL;

HWND g_hWnd_chk_add_resolve_domain_names = NULL;

//////

bool g_show_advanced = false;

wchar_t *t_au_category = NULL;
wchar_t *t_au_download_directory = NULL;

WNDPROC URLProc = NULL;
WNDPROC AddTabProc = NULL;

bool use_drag_and_drop_add = true;	// Assumes OLE32_STATE_RUNNING is true.
IDropTarget *Add_DropTarget;

bool use_add_split = false;
HMENU g_hMenu_add_split = NULL;

unsigned char add_split_type = 0;	// 0 = Download, 1 = Add, 2 = Verify

HBRUSH g_add_tab_brush = NULL;
int g_add_tab_width = 0;
bool g_add_use_theme = true;

wchar_t add_limit_tooltip_text[ 32 ];
HWND g_hWnd_add_limit_tooltip = NULL;

HFONT hFont_copy_add_proxy = NULL;

bool g_add_draw_tab_pane = false;

int add_urls_spinner_width = 0;
int add_urls_spinner_height = 0;

UINT current_dpi_add_urls = USER_DEFAULT_SCREEN_DPI;
UINT last_dpi_add_urls = 0;
HFONT hFont_add_urls = NULL;

#define _SCALE_AU_( x )						_SCALE_( ( x ), dpi_add_urls )

bool DisplayClipboardData()
{
	bool got_clipboard_data = false;

	// If there's any HTML conent on the clipboard, then parse out any URLs.
	if ( _IsClipboardFormatAvailable( CF_HTML ) == TRUE )
	{
		if ( _OpenClipboard( g_hWnd_add_urls ) )
		{
			HANDLE cbh = _GetClipboardData( CF_HTML );

			char *data = ( char * )GlobalLock( cbh );
			if ( data != NULL )
			{
				wchar_t *wdata = ParseHTMLClipboard( data );
				if ( wdata != NULL )
				{
					_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )wdata );

					// Append a newline after our pasted text.
					_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

					GlobalFree( wdata );

					got_clipboard_data = true;
				}

				GlobalUnlock( cbh );
			}

			_CloseClipboard();
		}
	}

	// There were no URLs in the HTML content, or it's just regular text.
	if ( _IsClipboardFormatAvailable( CF_UNICODETEXT ) == TRUE && !got_clipboard_data )
	{
		if ( _OpenClipboard( g_hWnd_add_urls ) )
		{
			HANDLE cbh = _GetClipboardData( CF_UNICODETEXT );

			wchar_t *data = ( wchar_t * )GlobalLock( cbh );
			if ( data != NULL )
			{
				char offset;
				if ( *data == L'{' && *( data + 1 ) == L'\r' && *( data + 2 ) == L'\n' )
				{
					offset = 3;
				}
				else
				{
					offset = 0;
				}

				// Make sure the text starts with a supported protocol.
				if ( _StrCmpNIW( data + offset, L"http://", 7 ) == 0 )
				{
					offset += 7;
				}
				else if ( _StrCmpNIW( data + offset, L"https://", 8 ) == 0 )
				{
					offset += 8;
				}
				else if ( _StrCmpNIW( data + offset, L"ftp://", 6 ) == 0 )
				{
					offset += 6;
				}
				else if ( _StrCmpNIW( data + offset, L"ftps://", 7 ) == 0 )
				{
					offset += 7;
				}
				else if ( _StrCmpNIW( data + offset, L"ftpes://", 8 ) == 0 )
				{
					offset += 8;
				}
				else if ( _StrCmpNIW( data + offset, L"sftp://", 7 ) == 0 )
				{
					offset += 7;
				}
				else
				{
					offset = 0;
				}

				// Make sure there's at least 3 characters after ://
				if ( offset > 0 )
				{
					for ( char i = 0; i < 3; ++i, ++offset )
					{
						if ( *( data + offset ) == NULL )
						{
							offset = 0;
							break;
						}
					}
				}

				if ( offset != 0 )
				{
					wchar_t *buf = NULL;
					SIZE_T data_size = GlobalSize( cbh );	// Includes NULL terminator.
					if ( data_size > 0 )
					{
						data_size = ( data_size - sizeof( wchar_t ) ) / sizeof( wchar_t );

						wchar_t *pstr = data;
						buf = ( wchar_t * )GlobalAlloc( GMEM_FIXED, ( sizeof( wchar_t ) * ( data_size * 2 ) ) + sizeof( wchar_t ) );
						wchar_t *pbuf = buf;

						if ( pbuf != NULL )
						{
							while ( pstr < ( data + data_size ) )
							{
								if ( *pstr == L'\r' && *( pstr + 1 ) == L'\n' )
								{
									*pbuf++ = *pstr++;
								}
								else if ( *pstr == L'\n' )
								{
									*pbuf++ = L'\r';
								}

								*pbuf++ = *pstr++;
							}

							*pbuf = L'\0';
						}
					}

					if ( buf != NULL )
					{
						_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )buf );

						GlobalFree( buf );
					}
					else
					{
						_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )data );
					}

					// Append a newline after our pasted text.
					_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

					got_clipboard_data = true;
				}

				GlobalUnlock( cbh );
			}

			_CloseClipboard();
		}
	}

	return got_clipboard_data;
}

void ShowHideAddProxyWindows( int index )
{
	if ( index == 0 )
	{
		_ShowWindow( g_hWnd_static_add_hoz1, SW_HIDE );

		_ShowWindow( g_hWnd_add_port_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_add_port_socks, SW_HIDE );
		_ShowWindow( g_hWnd_static_add_colon_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_add_type_hostname_socks, SW_HIDE );
		_ShowWindow( g_hWnd_chk_add_type_ip_address_socks, SW_HIDE );

		_ShowWindow( g_hWnd_add_ip_address_socks, SW_HIDE );
		_ShowWindow( g_hWnd_add_hostname_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_add_proxy_auth_username, SW_HIDE );
		_ShowWindow( g_hWnd_edit_add_proxy_auth_username, SW_HIDE );
		_ShowWindow( g_hWnd_static_add_proxy_auth_password, SW_HIDE );
		_ShowWindow( g_hWnd_edit_add_proxy_auth_password, SW_HIDE );

		_ShowWindow( g_hWnd_static_add_auth_ident_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_add_auth_ident_username_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_add_resolve_domain_names_v4a, SW_HIDE );

		_ShowWindow( g_hWnd_chk_add_use_authentication_socks, SW_HIDE );

		_ShowWindow( g_hWnd_static_add_auth_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_add_auth_username_socks, SW_HIDE );
		_ShowWindow( g_hWnd_static_add_auth_password_socks, SW_HIDE );
		_ShowWindow( g_hWnd_add_auth_password_socks, SW_HIDE );

		_ShowWindow( g_hWnd_chk_add_resolve_domain_names, SW_HIDE );
	}
	else
	{
		_ShowWindow( g_hWnd_static_add_hoz1, SW_SHOW );

		_ShowWindow( g_hWnd_add_port_socks, SW_SHOW );

		_ShowWindow( g_hWnd_static_add_port_socks, SW_SHOW );
		_ShowWindow( g_hWnd_static_add_colon_socks, SW_SHOW );

		_ShowWindow( g_hWnd_chk_add_type_hostname_socks, SW_SHOW );
		_ShowWindow( g_hWnd_chk_add_type_ip_address_socks, SW_SHOW );

		if ( _SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		{
			_ShowWindow( g_hWnd_add_ip_address_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_hostname_socks, SW_SHOW );
		}
		else
		{
			_ShowWindow( g_hWnd_add_hostname_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_ip_address_socks, SW_SHOW );
		}

		if ( index == 3 )
		{
			_ShowWindow( g_hWnd_static_add_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_static_add_proxy_auth_password, SW_HIDE );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_password, SW_HIDE );

			_ShowWindow( g_hWnd_chk_add_use_authentication_socks, SW_HIDE );

			_ShowWindow( g_hWnd_static_add_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_static_add_auth_password_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_password_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_add_resolve_domain_names, SW_HIDE );

			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname_ );

			_ShowWindow( g_hWnd_static_add_auth_ident_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_add_auth_ident_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_chk_add_resolve_domain_names_v4a, SW_SHOW );
		}
		else if ( index == 4 )
		{
			_ShowWindow( g_hWnd_static_add_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_username, SW_HIDE );
			_ShowWindow( g_hWnd_static_add_proxy_auth_password, SW_HIDE );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_password, SW_HIDE );

			_ShowWindow( g_hWnd_static_add_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_chk_add_resolve_domain_names_v4a, SW_HIDE );

			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

			_ShowWindow( g_hWnd_chk_add_use_authentication_socks, SW_SHOW );

			_ShowWindow( g_hWnd_static_add_auth_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_add_auth_username_socks, SW_SHOW );
			_ShowWindow( g_hWnd_static_add_auth_password_socks, SW_SHOW );
			_ShowWindow( g_hWnd_add_auth_password_socks, SW_SHOW );

			_ShowWindow( g_hWnd_chk_add_resolve_domain_names, SW_SHOW );
		}
		else
		{
			_ShowWindow( g_hWnd_static_add_proxy_auth_username, SW_SHOW );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_username, SW_SHOW );
			_ShowWindow( g_hWnd_static_add_proxy_auth_password, SW_SHOW );
			_ShowWindow( g_hWnd_edit_add_proxy_auth_password, SW_SHOW );

			_ShowWindow( g_hWnd_static_add_auth_ident_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_ident_username_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_add_resolve_domain_names_v4a, SW_HIDE );

			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, WM_SETTEXT, 0, ( LPARAM )ST_V_Hostname___IPv6_address_ );

			_ShowWindow( g_hWnd_chk_add_use_authentication_socks, SW_HIDE );

			_ShowWindow( g_hWnd_static_add_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_username_socks, SW_HIDE );
			_ShowWindow( g_hWnd_static_add_auth_password_socks, SW_HIDE );
			_ShowWindow( g_hWnd_add_auth_password_socks, SW_HIDE );

			_ShowWindow( g_hWnd_chk_add_resolve_domain_names, SW_HIDE );
		}
	}
}

LRESULT CALLBACK URLSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		/*case WM_CHAR:
		{
			// Replace enter with "\r\n" instead of "\n".
			if ( wParam == VK_RETURN )
			{
				_SendMessageA( hWnd, EM_REPLACESEL, TRUE, ( LPARAM )"\r\n" );

				return 0;
			}
			else
			{
				// Ctrl is down and 'a' has been pressed.
				// Prevents the annoying beep that would happen if we use WM_KEYDOWN.
				if ( wParam == 1 )
				{
					_SendMessageW( hWnd, EM_SETSEL, 0, -1 );	// Select all text.

					return 0;
				}
			}
		}
		break;*/

		case WM_GETDLGCODE:
		{
			// Don't process the tab key in the edit control. (Allows us to tab between controls)
			if ( wParam == VK_TAB )
			{
				return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
			}
		}
		break;

		case WM_KEYDOWN:
		{
			if ( hWnd == g_hWnd_edit_add )
			{
				if ( _GetKeyState( VK_CONTROL ) & 0x8000 )
				{
					if ( _GetKeyState( VK_SHIFT ) & 0x8000 && wParam == VK_RETURN )
					{
						// http://a.b
						if ( _SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 ) >= 10 )
						{
							_SendMessageW( _GetParent( hWnd ), WM_COMMAND, BTN_DOWNLOAD, 0 );
						}

						return 0;
					}
				}
			}
		}
		break;

		case WM_PASTE:
		{
			if ( DisplayClipboardData() )
			{
				return 0;
			}
		}
		break;
	}

	return _CallWindowProcW( URLProc, hWnd, msg, wParam, lParam );
}

LRESULT CALLBACK AddTabSubProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CTLCOLORSTATIC:
		{
			if ( g_add_use_theme && _IsThemeActive() == TRUE )
			{
				if ( ( HWND )lParam == g_hWnd_btn_authentication )
				{
					_SetBkMode( ( HDC )wParam, TRANSPARENT );

					if ( g_add_draw_tab_pane )
					{
						POINT pt;
						pt.x = 0; pt.y = 0;

						_MapWindowPoints( hWnd, ( HWND )lParam, &pt, 1 );
						_SetBrushOrgEx( ( HDC )wParam, pt.x, pt.y, NULL );

						return ( INT_PTR )g_add_tab_brush;
					}
					else
					{
						return ( INT_PTR )_GetSysColorBrush( COLOR_WINDOW );
					}
				}
			}
		}
		break;

		case WM_SIZE:
		{
			RECT rc_tab;
			_SendMessageW( hWnd, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( 1 );

			_DeferWindowPos( hdwp, g_hWnd_btn_authentication, HWND_TOP, _SCALE_AU_( 280 ), ( rc_tab.bottom - rc_tab.top ) + _SCALE_AU_( 65 ), _SCALE_AU_( 272 ), _SCALE_AU_( 72 ), SWP_NOZORDER );

			_EndDeferWindowPos( hdwp );
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_add_urls;
		}
		break;
	}

	return _CallWindowProcW( AddTabProc, hWnd, msg, wParam, lParam );
}

void ShowHideAddTabs( int sw_type )
{
	int index = ( int )_SendMessageW( g_hWnd_advanced_add_tab, TCM_GETCURSEL, 0, 0 );		// Get the selected tab
	if ( index != -1 )
	{
		switch ( index )
		{
			case 0:
			{
				_ShowWindow( g_hWnd_static_au_category, sw_type );
				_ShowWindow( g_hWnd_au_category, sw_type );

				_ShowWindow( g_hWnd_chk_add_enable_download_directory, sw_type );
				_ShowWindow( g_hWnd_download_directory, sw_type );
				_ShowWindow( g_hWnd_btn_download_directory, sw_type );

				_ShowWindow( g_hWnd_chk_add_enable_download_parts, sw_type );
				_ShowWindow( g_hWnd_download_parts, sw_type );
				_ShowWindow( g_hWnd_ud_download_parts, sw_type );

				_ShowWindow( g_hWnd_static_ssl_version, sw_type );
				_ShowWindow( g_hWnd_ssl_version, sw_type );

				_ShowWindow( g_hWnd_chk_add_enable_speed_limit, sw_type );
				_ShowWindow( g_hWnd_add_speed_limit, sw_type );

				_ShowWindow( g_hWnd_btn_authentication, sw_type );
				_ShowWindow( g_hWnd_static_username, sw_type );
				_ShowWindow( g_hWnd_edit_username, sw_type );
				_ShowWindow( g_hWnd_static_password, sw_type );
				_ShowWindow( g_hWnd_edit_password, sw_type );

				_ShowWindow( g_hWnd_chk_simulate_download, sw_type );
			}
			break;

			case 1:
			{
				_ShowWindow( g_hWnd_static_comments, sw_type );
				_ShowWindow( g_hWnd_edit_comments, sw_type );
			}
			break;

			case 2:
			{
				_ShowWindow( g_hWnd_static_cookies, sw_type );
				_ShowWindow( g_hWnd_edit_cookies, sw_type );
			}
			break;

			case 3:
			{
				_ShowWindow( g_hWnd_static_headers, sw_type );
				_ShowWindow( g_hWnd_edit_headers, sw_type );
			}
			break;

			case 4:
			{
				_ShowWindow( g_hWnd_chk_send_data, sw_type );
				_ShowWindow( g_hWnd_edit_data, sw_type );
			}
			break;

			case 5:
			{
				_ShowWindow( g_hWnd_static_add_proxy_type, sw_type );
				_ShowWindow( g_hWnd_add_proxy_type, sw_type );

				if ( sw_type == SW_SHOW )
				{
					index = ( int )_SendMessageW( g_hWnd_add_proxy_type, CB_GETCURSEL, 0, 0 );

					if ( index == CB_ERR )
					{
						index = 0;
					}
				}
				else
				{
					index = 0;
				}
	
				ShowHideAddProxyWindows( index );
			}
			break;
		}
	}
}

LRESULT CALLBACK AddURLsWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_CREATE:
		{
			current_dpi_add_urls = __GetDpiForWindow( hWnd );
			last_dpi_add_urls = ( current_dpi_add_urls == current_dpi_main ? current_dpi_add_urls : 0 );
			hFont_add_urls = UpdateFont( current_dpi_add_urls );

			g_hWnd_static_urls = _CreateWindowW( WC_STATIC, ST_V_URL_s__, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_add = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_URLS, NULL, NULL );

			_SendMessageW( g_hWnd_edit_add, EM_LIMITTEXT, 0, 0 );	// Maximum size.

			g_hWnd_advanced_add_tab = _CreateWindowW( /*WS_EX_CONTROLPARENT,*/ WC_TABCONTROL, NULL, WS_CHILD | /*WS_CLIPCHILDREN |*/ WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			TCITEM ti;
			_memzero( &ti, sizeof( TCITEM ) );
			ti.mask = TCIF_TEXT;	// The tab will have text and an lParam value.

			ti.pszText = ( LPWSTR )ST_V_General;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 0, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Comments;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 1, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Cookies;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 2, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Headers;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 3, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_POST_Data;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 4, ( LPARAM )&ti );	// Insert a new tab at the end.

			ti.pszText = ( LPWSTR )ST_V_Proxy;
			_SendMessageW( g_hWnd_advanced_add_tab, TCM_INSERTITEM, 5, ( LPARAM )&ti );	// Insert a new tab at the end.

			//

			g_hWnd_static_au_category = _CreateWindowW( WC_STATIC, ST_V_Category_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_au_category = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_ADD_CATEGORY, NULL, NULL );

			_SendMessageW( g_hWnd_au_category, CB_SETCURSEL, 0, 0 );

			g_hWnd_chk_add_enable_download_directory = _CreateWindowW( WC_BUTTON, ST_V_Download_directory_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_ADD_ENABLE_DOWNLOAD_DIRECTORY, NULL, NULL );
			g_hWnd_download_directory = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, cfg_default_download_directory, ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_btn_download_directory = _CreateWindowW( WC_BUTTON, ST_V_BTN___, WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD_DIRECTORY, NULL, NULL );


			g_hWnd_chk_add_enable_download_parts = _CreateWindowW( WC_BUTTON, ST_V_Download_parts_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_ADD_ENABLE_DOWNLOAD_PARTS, NULL, NULL );
			// Needs dimensions so that the spinner control can size itself.
			g_hWnd_download_parts = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 100, 23, hWnd, ( HMENU )EDIT_DOWNLOAD_PARTS, NULL, NULL );

			g_hWnd_ud_download_parts = _CreateWindowW( UPDOWN_CLASS, NULL, UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			_SendMessageW( g_hWnd_download_parts, EM_LIMITTEXT, 3, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETBUDDY, ( WPARAM )g_hWnd_download_parts, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETBASE, 10, 0 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETRANGE32, 1, 100 );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );

			RECT rc_spinner;
			_GetClientRect( g_hWnd_ud_download_parts, &rc_spinner );
			add_urls_spinner_width = rc_spinner.right - rc_spinner.left;
			add_urls_spinner_height = rc_spinner.bottom - rc_spinner.top;


			g_hWnd_chk_add_enable_speed_limit = _CreateWindowW( WC_BUTTON, ST_V_Download_speed_limit_bytes_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_ADD_ENABLE_SPEED_LIMIT, NULL, NULL );
			g_hWnd_add_speed_limit = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_SPEED_LIMIT, NULL, NULL );

			_SendMessageW( g_hWnd_add_speed_limit, EM_LIMITTEXT, 20, 0 );


			g_hWnd_add_limit_tooltip = _CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			add_limit_tooltip_text[ 0 ] = 0;

			TOOLINFO tti;
			_memzero( &tti, sizeof( TOOLINFO ) );
			tti.cbSize = sizeof( TOOLINFO );
			tti.uFlags = TTF_SUBCLASS;
			tti.hwnd = g_hWnd_add_speed_limit;
			tti.lpszText = add_limit_tooltip_text;

			_GetClientRect( hWnd, &tti.rect );
			_SendMessageW( g_hWnd_add_limit_tooltip, TTM_ADDTOOL, 0, ( LPARAM )&tti );


			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );


			g_hWnd_static_ssl_version = _CreateWindowW( WC_STATIC, ST_V_SSL___TLS_version_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_ssl_version = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, NULL, NULL, NULL );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_Default );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_2_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_SSL_3_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_0 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_1 );
			_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_2 );
			if ( g_can_use_tls_1_3 )
			{
				_SendMessageW( g_hWnd_ssl_version, CB_ADDSTRING, 0, ( LPARAM )ST_V_TLS_1_3 );
			}

			_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, 0, 0 );

			// Doesn't draw properly in XP if it's not a child of the tab control.
			g_hWnd_btn_authentication = _CreateWindowW( WC_BUTTON, ST_V_Authentication, BS_GROUPBOX | WS_CHILD, 0, 0, 0, 0, g_hWnd_advanced_add_tab, NULL, NULL, NULL );

			g_hWnd_static_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_simulate_download = _CreateWindowW( WC_BUTTON, ST_V_Simulate_download, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SIMULATE_DOWNLOAD, NULL, NULL );

			//

			g_hWnd_static_comments = _CreateWindowW( WC_STATIC, ST_V_Comments_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_comments = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_cookies = _CreateWindowW( WC_STATIC, ST_V_Cookies_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_cookies = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_headers = _CreateWindowW( WC_STATIC, ST_V_Headers_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_headers = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_send_data = _CreateWindowW( WC_BUTTON, ST_V_Send_POST_Data_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )CHK_SEND_DATA, NULL, NULL );
			g_hWnd_edit_data = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_HSCROLL | WS_VSCROLL | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			//

			g_hWnd_static_add_proxy_type = _CreateWindowW( WC_STATIC, ST_V_Use_proxy_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_add_proxy_type = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_ADD_PROXY_TYPE, NULL, NULL );
			_SendMessageW( g_hWnd_add_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_Default );
			_SendMessageW( g_hWnd_add_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_HTTP );
			_SendMessageW( g_hWnd_add_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_HTTPS );
			_SendMessageW( g_hWnd_add_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_SOCKS_v4 );
			_SendMessageW( g_hWnd_add_proxy_type, CB_ADDSTRING, 0, ( LPARAM )ST_V_SOCKS_v5 );

			_SendMessageW( g_hWnd_add_proxy_type, CB_SETCURSEL, 0, 0 );


			g_hWnd_static_add_hoz1 = _CreateWindowW( WC_STATIC, NULL, SS_ETCHEDHORZ | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_chk_add_type_hostname_socks = _CreateWindowW( WC_BUTTON, ST_V_Hostname___IPv6_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_GROUP | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_TYPE_HOST_SOCKS, NULL, NULL );
			g_hWnd_chk_add_type_ip_address_socks = _CreateWindowW( WC_BUTTON, ST_V_IPv4_address_, BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_TYPE_IP_ADDRESS_SOCKS, NULL, NULL );

			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );

			g_hWnd_add_hostname_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_HOST_SOCKS, NULL, NULL );
			// Needs a width and height when it's created because it's a stupid control.
			g_hWnd_add_ip_address_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_IPADDRESS, NULL, WS_CHILD | WS_TABSTOP, 0, 0, 310, 23, hWnd, ( HMENU )EDIT_ADD_IP_ADDRESS_SOCKS, NULL, NULL );


			g_hWnd_static_add_colon_socks = _CreateWindowW( WC_STATIC, ST_V_COLON, SS_CENTER | WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_static_add_port_socks = _CreateWindowW( WC_STATIC, ST_V_Port_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_port_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )EDIT_ADD_PORT_SOCKS, NULL, NULL );


			g_hWnd_static_add_proxy_auth_username = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_add_proxy_auth_username = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_ADD_PROXY_AUTH_USERNAME, NULL, NULL );

			g_hWnd_static_add_proxy_auth_password = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_edit_add_proxy_auth_password = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_ADD_PROXY_AUTH_PASSWORD, NULL, NULL );


			// v4

			g_hWnd_static_add_auth_ident_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_auth_ident_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_ADD_AUTH_IDENT_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_chk_add_resolve_domain_names_v4a = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names_v4a, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_RESOLVE_DOMAIN_NAMES_V4A, NULL, NULL );

			// v5

			g_hWnd_chk_add_use_authentication_socks = _CreateWindowW( WC_BUTTON, ST_V_Use_Authentication_, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_AUTHENTICATION_SOCKS, NULL, NULL );

			g_hWnd_static_add_auth_username_socks = _CreateWindowW( WC_STATIC, ST_V_Username_, WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_auth_username_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_ADD_AUTH_USERNAME_SOCKS, NULL, NULL );

			g_hWnd_static_add_auth_password_socks = _CreateWindowW( WC_STATIC, ST_V_Password_, WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			g_hWnd_add_auth_password_socks = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_DISABLED, 0, 0, 0, 0, hWnd, ( HMENU )( HMENU )EDIT_ADD_AUTH_PASSWORD_SOCKS, NULL, NULL );

			g_hWnd_chk_add_resolve_domain_names = _CreateWindowW( WC_BUTTON, ST_V_Allow_proxy_to_resolve_domain_names, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_RESOLVE_DOMAIN_NAMES, NULL, NULL );


			_SendMessageW( g_hWnd_add_hostname_socks, EM_LIMITTEXT, MAX_DOMAIN_LENGTH, 0 );
			_SendMessageW( g_hWnd_add_port_socks, EM_LIMITTEXT, 5, 0 );

			_SendMessageW( g_hWnd_add_auth_username_socks, EM_LIMITTEXT, 255, 0 );
			_SendMessageW( g_hWnd_add_auth_password_socks, EM_LIMITTEXT, 255, 0 );

			//

			g_hWnd_chk_show_advanced_options = _CreateWindowW( WC_BUTTON, ST_V_Advanced_options, BS_AUTOCHECKBOX | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADVANCED, NULL, NULL );

			use_add_split = IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 );

			if ( use_add_split )
			{
				g_hWnd_btn_download = _CreateWindowW( WC_BUTTON, ST_V_Download, BS_DEFSPLITBUTTON | BS_SPLITBUTTON | WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD, NULL, NULL );
			}
			else
			{
				g_hWnd_btn_download = _CreateWindowW( WC_BUTTON, ST_V_Download, BS_DEFPUSHBUTTON | WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_DOWNLOAD, NULL, NULL );
				g_hWnd_btn_add_download = _CreateWindowW( WC_BUTTON, ST_V_Add, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_DOWNLOAD, NULL, NULL );
				g_hWnd_btn_verify_download = _CreateWindowW( WC_BUTTON, ST_V_Verify, WS_CHILD | WS_DISABLED | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_VERIFY_DOWNLOAD, NULL, NULL );
			}

			g_hWnd_cancel = _CreateWindowW( WC_BUTTON, ST_V_Cancel, WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, hWnd, ( HMENU )BTN_ADD_CANCEL, NULL, NULL );

			// Need these windows to be created last to preserve tab order.

			DWORD enabled = ( pcre2_state == PCRE2_STATE_RUNNING ? 0 : WS_DISABLED );
			g_hWnd_static_regex_filter = _CreateWindowW( WC_STATIC, ST_V_RegEx_filter_, /*SS_OWNERDRAW |*/ SS_RIGHT | WS_CHILD | WS_VISIBLE | enabled, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );
			// Needs dimensions so that list displays in XP.
			g_hWnd_regex_filter_preset = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_COMBOBOX, NULL, CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | enabled | CBS_DARK_MODE, 0, 0, 100, 23, hWnd, ( HMENU )CB_REGEX_FILTER_PRESET, NULL, NULL );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Custom_Filter );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Images_Filter );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Music_Filter );
			_SendMessageW( g_hWnd_regex_filter_preset, CB_ADDSTRING, 0, ( LPARAM )ST_V_Videos_Filter );

			_SendMessageW( g_hWnd_regex_filter_preset, CB_SETCURSEL, 0, 0 );

			g_hWnd_regex_filter = _CreateWindowExW( WS_EX_CLIENTEDGE, WC_EDIT, NULL, ES_AUTOHSCROLL | WS_CHILD | WS_TABSTOP | WS_VISIBLE | enabled, 0, 0, 0, 0, hWnd, NULL, NULL, NULL );

			g_hWnd_btn_apply_filter = _CreateWindowW( WC_BUTTON, ST_V_Apply, WS_CHILD | WS_TABSTOP | WS_VISIBLE | enabled, 0, 0, 0, 0, hWnd, ( HMENU )BTN_APPLY_FILTER, NULL, NULL );

			//

			g_hMenu_add_split = _CreatePopupMenu();

			MENUITEMINFO mii;
			_memzero( &mii, sizeof( MENUITEMINFO ) );
			mii.cbSize = sizeof( MENUITEMINFO );
			mii.fMask = MIIM_TYPE | MIIM_ID;

			mii.dwTypeData = ST_V_Download;
			mii.cch = ST_L_Download;
			mii.wID = MENU_ADD_SPLIT_DOWNLOAD;
			_InsertMenuItemW( g_hMenu_add_split, 0, TRUE, &mii );

			mii.dwTypeData = ST_V_Add;
			mii.cch = ST_L_Add;
			mii.wID = MENU_ADD_SPLIT_ADD;
			_InsertMenuItemW( g_hMenu_add_split, 1, TRUE, &mii );

			mii.dwTypeData = ST_V_Verify;
			mii.cch = ST_L_Verify;
			mii.wID = MENU_ADD_SPLIT_VERIFY;
			_InsertMenuItemW( g_hMenu_add_split, 2, TRUE, &mii );

			_SetFocus( g_hWnd_edit_add );

			_SendMessageW( g_hWnd_btn_download, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			if ( !use_add_split )
			{
				_SendMessageW( g_hWnd_btn_add_download, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
				_SendMessageW( g_hWnd_btn_verify_download, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			}
			_SendMessageW( g_hWnd_cancel, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_urls, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_add, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_ssl_version, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_ssl_version, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_add_enable_speed_limit, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_speed_limit, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_add_enable_download_parts, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_download_parts, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_simulate_download, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_au_category, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_au_category, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_add_enable_download_directory, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_download_directory, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_btn_download_directory, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_btn_authentication, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_username, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_username, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_password, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_password, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_advanced_add_tab, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_comments, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_comments, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_cookies, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_cookies, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_headers, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_headers, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_send_data, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_data, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_show_advanced_options, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_static_regex_filter, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_regex_filter_preset, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_regex_filter, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_btn_apply_filter, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			//

			_SendMessageW( g_hWnd_static_add_proxy_type, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_proxy_type, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_chk_add_type_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_add_hostname_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_static_add_colon_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_static_add_port_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_port_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );



			_SendMessageW( g_hWnd_static_add_proxy_auth_username, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_add_proxy_auth_username, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_static_add_proxy_auth_password, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_edit_add_proxy_auth_password, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );


			_SendMessageW( g_hWnd_static_add_auth_ident_username_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_auth_ident_username_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_chk_add_resolve_domain_names_v4a, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_chk_add_use_authentication_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_static_add_auth_username_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_auth_username_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_static_add_auth_password_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );
			_SendMessageW( g_hWnd_add_auth_password_socks, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			_SendMessageW( g_hWnd_chk_add_resolve_domain_names, WM_SETFONT, ( WPARAM )hFont_add_urls, 0 );

			// Stupid control likes to delete the font object. :-/
			// We'll make a copy.
			hFont_copy_add_proxy = UpdateFont( current_dpi_add_urls );
			_SendMessageW( g_hWnd_add_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_copy_add_proxy, 0 );

			//

			t_au_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
			if ( t_au_download_directory != NULL )
			{
				_wmemcpy_s( t_au_download_directory, MAX_PATH, cfg_default_download_directory, g_default_download_directory_length );
				t_au_download_directory[ g_default_download_directory_length ] = 0;	// Sanity.
			}

			URLProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_edit_add, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_edit_add, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_comments, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_cookies, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_headers, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );
			_SetWindowLongPtrW( g_hWnd_edit_data, GWLP_WNDPROC, ( LONG_PTR )URLSubProc );

			// g_hWnd_btn_authentication doesn't draw properly in XP if it's not a child of the tab control.
			AddTabProc = ( WNDPROC )_GetWindowLongPtrW( g_hWnd_advanced_add_tab, GWLP_WNDPROC );
			_SetWindowLongPtrW( g_hWnd_advanced_add_tab, GWLP_WNDPROC, ( LONG_PTR )AddTabSubProc );

			#ifndef UXTHEME_USE_STATIC_LIB
				if ( uxtheme_state == UXTHEME_STATE_SHUTDOWN )
				{
					g_add_use_theme = InitializeUXTheme();
				}
			#endif

			#ifndef OLE32_USE_STATIC_LIB
				if ( ole32_state == OLE32_STATE_SHUTDOWN )
				{
					use_drag_and_drop_add = InitializeOle32();
				}
			#endif

			if ( use_drag_and_drop_add )
			{
				_OleInitialize( NULL );

				RegisterDropWindow( g_hWnd_edit_add, &Add_DropTarget );
			}

			int width = _SCALE_AU_( ADD_URLS_WIDTH );
			int height = _SCALE_AU_( MIN_SIMPLE_HEIGHT );

			HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
			MONITORINFO mi;
			mi.cbSize = sizeof( MONITORINFO );
			_GetMonitorInfoW( hMon, &mi );
			_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );

#ifdef ENABLE_DARK_MODE
			if ( g_use_dark_mode )
			{
				_EnumChildWindows( hWnd, EnumChildProc, NULL );
				_EnumThreadWindows( GetCurrentThreadId(), EnumTLWProc, NULL );
			}
#endif

			g_add_draw_tab_pane = !IsWindowsVersionOrGreater( HIBYTE( _WIN32_WINNT_VISTA ), LOBYTE( _WIN32_WINNT_VISTA ), 0 );

			return 0;
		}
		break;

		/*case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = ( DRAWITEMSTRUCT * )lParam;

			// The disabled static control causes flickering. The only way around it is to draw it ourselves.
			if ( dis->CtlType == ODT_STATIC )
			{
				// Need this if main window has WS_CLIPCHILDREN
				HBRUSH color = _CreateSolidBrush( ( COLORREF )_GetSysColor( COLOR_3DFACE ) );
				_FillRect( dis->hDC, &dis->rcItem, color );
				_DeleteObject( color );

				if ( _IsWindowEnabled( dis->hwndItem ) == FALSE )
				{
					_SetTextColor( dis->hDC, _GetSysColor( COLOR_GRAYTEXT ) );
				}
				_DrawTextW( dis->hDC, ST_V_RegEx_filter_, ST_L_RegEx_filter_, &dis->rcItem, DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_RIGHT );
			}

			return TRUE;
		}
		break;*/

		case WM_CTLCOLORSTATIC:
		{
			if ( g_add_use_theme && _IsThemeActive() == TRUE && ( HWND )lParam != g_hWnd_chk_show_advanced_options )
			{
				if ( ( HWND )lParam == g_hWnd_static_au_category ||
					 ( HWND )lParam == g_hWnd_chk_add_enable_download_directory ||
					 ( HWND )lParam == g_hWnd_chk_add_enable_download_parts ||
					 ( HWND )lParam == g_hWnd_static_ssl_version ||
					 ( HWND )lParam == g_hWnd_chk_add_enable_speed_limit ||
					 ( HWND )lParam == g_hWnd_static_username ||
					 ( HWND )lParam == g_hWnd_static_password ||
					 ( HWND )lParam == g_hWnd_chk_simulate_download ||
					 ( HWND )lParam == g_hWnd_static_comments ||
					 ( HWND )lParam == g_hWnd_static_cookies ||
					 ( HWND )lParam == g_hWnd_static_headers ||
					 ( HWND )lParam == g_hWnd_chk_send_data ||
					 ( HWND )lParam == g_hWnd_static_add_proxy_type ||
					 ( HWND )lParam == g_hWnd_static_add_port_socks ||
					 ( HWND )lParam == g_hWnd_static_add_colon_socks ||
					 ( HWND )lParam == g_hWnd_chk_add_type_hostname_socks ||
					 ( HWND )lParam == g_hWnd_chk_add_type_ip_address_socks ||
					 ( HWND )lParam == g_hWnd_static_add_proxy_auth_username ||
					 ( HWND )lParam == g_hWnd_static_add_proxy_auth_password ||
					 ( HWND )lParam == g_hWnd_static_add_auth_ident_username_socks ||
					 ( HWND )lParam == g_hWnd_chk_add_resolve_domain_names_v4a ||
					 ( HWND )lParam == g_hWnd_chk_add_use_authentication_socks ||
					 ( HWND )lParam == g_hWnd_static_add_auth_username_socks ||
					 ( HWND )lParam == g_hWnd_static_add_auth_password_socks ||
					 ( HWND )lParam == g_hWnd_chk_add_resolve_domain_names )
				{
					_SetBkMode( ( HDC )wParam, TRANSPARENT );

					if ( g_add_draw_tab_pane )
					{
						POINT pt;
						pt.x = 0; pt.y = 0;

						_MapWindowPoints( g_hWnd_advanced_add_tab, ( HWND )lParam, &pt, 1 );
						_SetBrushOrgEx( ( HDC )wParam, pt.x, pt.y, NULL );

						return ( INT_PTR )g_add_tab_brush;
					}
					else
					{
						return ( INT_PTR )_GetSysColorBrush( COLOR_WINDOW );
					}
				}
			}

			return _DefWindowProcW( hWnd, msg, wParam, lParam );
		}
		break;

		case WM_SIZE:
		{
			RECT rc, rc_tab;
			_GetClientRect( hWnd, &rc );

			int tab_width = rc.right - _SCALE_AU_( 20 );
			int tab_height = _SCALE_AU_( 234 );

			// This brush is refreshed whenever the tab changes size.
			// It's used to paint the background of static controls.
			// Windows XP has a gradient colored tab pane and setting the background of a static control to TRANSPARENT in WM_CTLCOLORSTATIC doesn't work well.
			if ( g_add_draw_tab_pane && ( wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED ) && ( _IsWindowVisible( g_hWnd_advanced_add_tab ) == TRUE && g_add_tab_width != tab_width ) )
			{
				g_add_tab_width = tab_width;

				HBRUSH old_brush = g_add_tab_brush;

				HDC hDC = _GetDC( g_hWnd_advanced_add_tab );

				// Create a memory buffer to draw to.
				HDC hdcMem = _CreateCompatibleDC( hDC );

				HBITMAP hbm = _CreateCompatibleBitmap( hDC, g_add_tab_width, tab_height );
				HBITMAP ohbm = ( HBITMAP )_SelectObject( hdcMem, hbm );
				_DeleteObject( ohbm );

				_SendMessageW( g_hWnd_advanced_add_tab, WM_PRINTCLIENT, ( WPARAM )hdcMem, ( LPARAM )( PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT ) );

				g_add_tab_brush = _CreatePatternBrush( hbm );

				_DeleteObject( hbm );

				_DeleteDC( hdcMem );
				_ReleaseDC( g_hWnd_advanced_add_tab, hDC );

				if ( old_brush != NULL )
				{
					_DeleteObject( old_brush );
				}
			}

			_SendMessageW( g_hWnd_advanced_add_tab, TCM_GETITEMRECT, 0, ( LPARAM )&rc_tab );

			int tab_child_y_offset = ( rc_tab.bottom - rc_tab.top ) + ( rc.bottom - _SCALE_AU_( 276 ) );

			int spinner_width = _SCALE_AU_( add_urls_spinner_width );
			int spinner_height = _SCALE_AU_( add_urls_spinner_height );

			// Allow our controls to move in relation to the parent window.
			HDWP hdwp = _BeginDeferWindowPos( ( use_add_split ? 58 : 60 ) );

			_DeferWindowPos( hdwp, g_hWnd_static_regex_filter, HWND_TOP, _SCALE_AU_( 105 ), _SCALE_AU_( 14 ), _SCALE_AU_( 90 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_regex_filter_preset, HWND_TOP, _SCALE_AU_( 200 ), _SCALE_AU_( 10 ), _SCALE_AU_( 100 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_regex_filter, HWND_TOP, _SCALE_AU_( 305 ), _SCALE_AU_( 10 ), rc.right - _SCALE_AU_( 389 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_apply_filter, HWND_TOP, rc.right - _SCALE_AU_( 80 ), _SCALE_AU_( 10 ), _SCALE_AU_( 70 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_urls, HWND_TOP, _SCALE_AU_( 10 ), _SCALE_AU_( 22 ), _SCALE_AU_( 90 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_add, HWND_BOTTOM, _SCALE_AU_( 10 ), _SCALE_AU_( 40 ), rc.right - _SCALE_AU_( 20 ), rc.bottom - ( g_show_advanced ? _SCALE_AU_( 326 ) : _SCALE_AU_( 83 ) ), 0 );

			_DeferWindowPos( hdwp, g_hWnd_advanced_add_tab, HWND_BOTTOM, _SCALE_AU_( 10 ), rc.bottom - _SCALE_AU_( 276 ), rc.right - _SCALE_AU_( 20 ), tab_height, 0 );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_au_category, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 12 ), _SCALE_AU_( 120 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_au_category, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 30 ), _SCALE_AU_( 120 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_enable_download_directory, HWND_TOP, _SCALE_AU_( 164 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 400 ), _SCALE_AU_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_directory, HWND_TOP, _SCALE_AU_( 164 ), tab_child_y_offset + _SCALE_AU_( 30 ), rc.right - _SCALE_AU_( 224 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_btn_download_directory, HWND_TOP, rc.right - _SCALE_AU_( 55 ), tab_child_y_offset + _SCALE_AU_( 30 ), _SCALE_AU_( 35 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_enable_download_parts, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 63 ), _SCALE_AU_( 120 ), _SCALE_AU_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_download_parts, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 83 ), _SCALE_AU_( 100 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ud_download_parts, HWND_TOP, _SCALE_AU_( 120 ), tab_child_y_offset + _SCALE_AU_( 83 ), spinner_width, spinner_height, SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_ssl_version, HWND_TOP, _SCALE_AU_( 164 ), tab_child_y_offset + _SCALE_AU_( 65 ), _SCALE_AU_( 125 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_ssl_version, HWND_TOP, _SCALE_AU_( 164 ), tab_child_y_offset + _SCALE_AU_( 83 ), _SCALE_AU_( 100 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_enable_speed_limit, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 116 ), _SCALE_AU_( 250 ), _SCALE_AU_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_speed_limit, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 136 ), _SCALE_AU_( 200 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_username, HWND_TOP, _SCALE_AU_( 301 ), tab_child_y_offset + _SCALE_AU_( 84 ), _SCALE_AU_( 120 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_username, HWND_TOP, _SCALE_AU_( 301 ), tab_child_y_offset + _SCALE_AU_( 102 ), _SCALE_AU_( 120 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_password, HWND_TOP, _SCALE_AU_( 431 ), tab_child_y_offset + _SCALE_AU_( 84 ), _SCALE_AU_( 120 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_password, HWND_TOP, _SCALE_AU_( 431 ), tab_child_y_offset + _SCALE_AU_( 102 ), _SCALE_AU_( 120 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_simulate_download, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 169 ), _SCALE_AU_( 400 ), _SCALE_AU_( 20 ), SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_comments, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 400 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_comments, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 28 ), rc.right - _SCALE_AU_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_AU_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_cookies, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 400 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_cookies, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 28 ), rc.right - _SCALE_AU_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_AU_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_headers, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 400 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_headers, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 28 ), rc.right - _SCALE_AU_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_AU_( 38 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_send_data, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 400 ), _SCALE_AU_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_data, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 30 ), rc.right - _SCALE_AU_( 40 ), ( tab_height - rc_tab.bottom ) - _SCALE_AU_( 40 ), SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_static_add_proxy_type, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 10 ), _SCALE_AU_( 150 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_proxy_type, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 28 ), _SCALE_AU_( 100 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_add_hoz1, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 61 ), rc.right - _SCALE_AU_( 40 ), _SCALE_AU_( 1 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_type_hostname_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 69 ), _SCALE_AU_( 200 ), _SCALE_AU_( 20 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_chk_add_type_ip_address_socks, HWND_TOP, _SCALE_AU_( 225 ), tab_child_y_offset + _SCALE_AU_( 69 ), _SCALE_AU_( 110 ), _SCALE_AU_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_add_hostname_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 89 ), _SCALE_AU_( 310 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_ip_address_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 89 ), _SCALE_AU_( 310 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_add_colon_socks, HWND_TOP, _SCALE_AU_( 331 ), tab_child_y_offset + _SCALE_AU_( 92 ), _SCALE_AU_( 8 ), _SCALE_AU_( 17 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_add_port_socks, HWND_TOP, _SCALE_AU_( 340 ), tab_child_y_offset + _SCALE_AU_( 71 ), _SCALE_AU_( 75 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_port_socks, HWND_TOP, _SCALE_AU_( 340 ), tab_child_y_offset + _SCALE_AU_( 89 ), _SCALE_AU_( 75 ), _SCALE_AU_( 23 ), SWP_NOZORDER );


			_DeferWindowPos( hdwp, g_hWnd_static_add_proxy_auth_username, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 118 ), _SCALE_AU_( 150 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_add_proxy_auth_username, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 136 ), _SCALE_AU_( 150 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_static_add_proxy_auth_password, HWND_TOP, _SCALE_AU_( 180 ), tab_child_y_offset + _SCALE_AU_( 118 ), _SCALE_AU_( 150 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_edit_add_proxy_auth_password, HWND_TOP, _SCALE_AU_( 180 ), tab_child_y_offset + _SCALE_AU_( 136 ), _SCALE_AU_( 150 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			// v4

			_DeferWindowPos( hdwp, g_hWnd_static_add_auth_ident_username_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 118 ), _SCALE_AU_( 400 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_auth_ident_username_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 136 ), _SCALE_AU_( 150 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_resolve_domain_names_v4a, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 164 ), rc.right - _SCALE_AU_( 40 ), _SCALE_AU_( 20 ), SWP_NOZORDER );


			// v5

			_DeferWindowPos( hdwp, g_hWnd_chk_add_use_authentication_socks, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 118 ), _SCALE_AU_( 400 ), _SCALE_AU_( 20 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_add_auth_username_socks, HWND_TOP, _SCALE_AU_( 35 ), tab_child_y_offset + _SCALE_AU_( 138 ), _SCALE_AU_( 150 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_auth_username_socks, HWND_TOP, _SCALE_AU_( 35 ), tab_child_y_offset + _SCALE_AU_( 156 ), _SCALE_AU_( 150 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_static_add_auth_password_socks, HWND_TOP, _SCALE_AU_( 195 ), tab_child_y_offset + _SCALE_AU_( 138 ), _SCALE_AU_( 150 ), _SCALE_AU_( 17 ), SWP_NOZORDER );
			_DeferWindowPos( hdwp, g_hWnd_add_auth_password_socks, HWND_TOP, _SCALE_AU_( 195 ), tab_child_y_offset + _SCALE_AU_( 156 ), _SCALE_AU_( 150 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			_DeferWindowPos( hdwp, g_hWnd_chk_add_resolve_domain_names, HWND_TOP, _SCALE_AU_( 20 ), tab_child_y_offset + _SCALE_AU_( 184 ), rc.right - _SCALE_AU_( 40 ), _SCALE_AU_( 20 ), SWP_NOZORDER );

			//

			_DeferWindowPos( hdwp, g_hWnd_chk_show_advanced_options, HWND_TOP, _SCALE_AU_( 10 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 210 ), _SCALE_AU_( 23 ), SWP_NOZORDER );

			if ( use_add_split )
			{
				_DeferWindowPos( hdwp, g_hWnd_btn_download, HWND_TOP, rc.right - _SCALE_AU_( 188 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 93 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			}
			else
			{
				_DeferWindowPos( hdwp, g_hWnd_btn_download, HWND_TOP, rc.right - _SCALE_AU_( 345 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 80 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, g_hWnd_btn_add_download, HWND_TOP, rc.right - _SCALE_AU_( 260 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 80 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
				_DeferWindowPos( hdwp, g_hWnd_btn_verify_download, HWND_TOP, rc.right - _SCALE_AU_( 175 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 80 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			}
			_DeferWindowPos( hdwp, g_hWnd_cancel, HWND_TOP, rc.right - _SCALE_AU_( 90 ), rc.bottom - _SCALE_AU_( 33 ), _SCALE_AU_( 80 ), _SCALE_AU_( 23 ), SWP_NOZORDER );
			_EndDeferWindowPos( hdwp );

			/*rc.left = 5;
			rc.top = 5;
			rc.right -= 65;
			rc.bottom -= ( g_show_advanced ? 438 : 153 );	// Add 20 to each of these numbers from above.
			_SendMessageW( g_hWnd_edit_add, EM_SETRECT, 0, ( LPARAM )&rc );*/

			return 0;
		}
		break;

		case WM_GET_DPI:
		{
			return current_dpi_add_urls;
		}
		break;

		case WM_DPICHANGED:
		{
			UINT last_dpi = current_dpi_add_urls;
			current_dpi_add_urls = HIWORD( wParam );

			HFONT hFont = UpdateFont( current_dpi_add_urls );
			EnumChildWindows( hWnd, EnumChildFontProc, ( LPARAM )hFont );
			_DeleteObject( hFont_add_urls );
			hFont_add_urls = hFont;

			// This stupid control doesn't adapt to the change in font size. It needs to be resized first.
			_SetWindowPos( g_hWnd_add_ip_address_socks, HWND_TOP, 0, 0, _SCALE_AU_( 310 ), _SCALE_AU_( 23 ), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
			_DeleteObject( hFont_copy_add_proxy );
			hFont_copy_add_proxy = UpdateFont( current_dpi_add_urls );
			_SendMessageW( g_hWnd_add_ip_address_socks, WM_SETFONT, ( WPARAM )hFont_copy_add_proxy, 0 );

			RECT *rc = ( RECT * )lParam;
			int width = rc->right - rc->left;
			int height = rc->bottom - rc->top;

			if ( last_dpi_add_urls == 0 )
			{
				HMONITOR hMon = _MonitorFromWindow( g_hWnd_main, MONITOR_DEFAULTTONEAREST );
				MONITORINFO mi;
				mi.cbSize = sizeof( MONITORINFO );
				_GetMonitorInfoW( hMon, &mi );
				_SetWindowPos( hWnd, NULL, mi.rcMonitor.left + ( ( ( mi.rcMonitor.right - mi.rcMonitor.left ) - width ) / 2 ), mi.rcMonitor.top + ( ( ( mi.rcMonitor.bottom - mi.rcMonitor.top ) - height ) / 2 ), width, height, 0 );
			}
			else
			{
				_SetWindowPos( hWnd, NULL, rc->left, rc->top, width, height, SWP_NOZORDER | SWP_NOACTIVATE );
			}

			last_dpi_add_urls = last_dpi;

			return 0;
		}
		break;

		case WM_GETMINMAXINFO:
		{
			// Set the minimum dimensions that the window can be sized to.
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.x = _SCALE_AU_( ADD_URLS_WIDTH );
			( ( MINMAXINFO * )lParam )->ptMinTrackSize.y = ( g_show_advanced ? _SCALE_AU_( MIN_ADVANCED_HEIGHT ) : _SCALE_AU_( MIN_SIMPLE_HEIGHT ) );

			return 0;
		}
		break;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case CB_ADD_CATEGORY:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						BOOL enable;

						int cur_sel = ( int )_SendMessageW( g_hWnd_au_category, CB_GETCURSEL, 0, 0 );
						if ( cur_sel == 0 )
						{
							enable = FALSE;

							GlobalFree( t_au_download_directory );
							t_au_download_directory = GlobalStrDupW( cfg_default_download_directory );
						}
						else
						{
							enable = TRUE;

							LRESULT ret = _SendMessageW( g_hWnd_au_category, CB_GETITEMDATA, cur_sel, 0 );
							if ( ret != CB_ERR )
							{
								CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )ret;
								if ( ci != NULL )
								{
									GlobalFree( t_au_download_directory );
									t_au_download_directory = GlobalStrDupW( ci->download_directory );
								}
							}
						}

						if ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED )
						{
							_EnableWindow( g_hWnd_chk_add_enable_download_directory, TRUE );

							_EnableWindow( g_hWnd_download_directory, enable );
							_EnableWindow( g_hWnd_btn_download_directory, enable );

							_SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_SETCHECK, ( enable == TRUE ? BST_CHECKED : BST_UNCHECKED ), 0 );

							_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_au_download_directory );
						}
					}
				}
				break;

				case CHK_ADD_ENABLE_DOWNLOAD_DIRECTORY:
				case BTN_DOWNLOAD_DIRECTORY:
				{
					if ( LOWORD( wParam ) == CHK_ADD_ENABLE_DOWNLOAD_DIRECTORY )
					{
						BOOL enable = ( _SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

						_EnableWindow( g_hWnd_download_directory, enable );
						_EnableWindow( g_hWnd_btn_download_directory, enable );

						if ( enable == FALSE )
						{
							break;
						}
					}

					wchar_t *directory = NULL;

					_BrowseForFolder( hWnd, ST_V_Select_the_download_directory, &directory );

					if ( directory != NULL )
					{
						GlobalFree( t_au_download_directory );
						t_au_download_directory = directory;

						_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_au_download_directory );
					}
				}
				break;

				case CHK_ADD_ENABLE_DOWNLOAD_PARTS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_add_enable_download_parts, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_download_parts, enable );
					_EnableWindow( g_hWnd_ud_download_parts, enable );
				}
				break;

				case EDIT_DOWNLOAD_PARTS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

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
					}
				}
				break;

				case CHK_ADD_ENABLE_SPEED_LIMIT:
				{
					_EnableWindow( g_hWnd_add_speed_limit, ( _SendMessageW( g_hWnd_chk_add_enable_speed_limit, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
				}
				break;

				case EDIT_ADD_SPEED_LIMIT:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						DWORD sel_start;

						char value[ 21 ];
						_SendMessageA( ( HWND )lParam, WM_GETTEXT, 21, ( LPARAM )value );
						unsigned long long num = strtoull( value );

						if ( num == 0xFFFFFFFFFFFFFFFF )
						{
							_SendMessageA( ( HWND )lParam, EM_GETSEL, ( WPARAM )&sel_start, NULL );

							_SendMessageA( ( HWND )lParam, WM_SETTEXT, 0, ( LPARAM )"18446744073709551615" );

							_SendMessageA( ( HWND )lParam, EM_SETSEL, sel_start, sel_start );
						}

						if ( num > 0 )
						{
							unsigned int length = FormatSizes( add_limit_tooltip_text, 32, SIZE_FORMAT_AUTO, num );
							add_limit_tooltip_text[ length++ ] = L'/';
							add_limit_tooltip_text[ length++ ] = L's';
							add_limit_tooltip_text[ length ] = 0;
						}
						else
						{
							_wmemcpy_s( add_limit_tooltip_text, 32, ST_V_Unlimited, ST_L_Unlimited + 1 );
						}

						TOOLINFO ti;
						_memzero( &ti, sizeof( TOOLINFO ) );
						ti.cbSize = sizeof( TOOLINFO );
						ti.hwnd = g_hWnd_add_speed_limit;
						ti.lpszText = add_limit_tooltip_text;
						_SendMessageW( g_hWnd_add_limit_tooltip, TTM_UPDATETIPTEXT, 0, ( LPARAM )&ti );
					}
				}
				break;

				case CHK_SIMULATE_DOWNLOAD:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? FALSE : TRUE );

					_EnableWindow( g_hWnd_chk_add_enable_download_directory, enable );

					_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )( enable == FALSE ? ST_V__Simulated_ : t_au_download_directory ) );

					if ( enable == TRUE && _SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_UNCHECKED )
					{
						enable = FALSE;
					}

					_EnableWindow( g_hWnd_download_directory, enable );
					_EnableWindow( g_hWnd_btn_download_directory, enable );
				}
				break;

				case CHK_SEND_DATA:
				{
					_EnableWindow( g_hWnd_edit_data, ( _SendMessageW( g_hWnd_chk_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE ) );
				}
				break;

				case CB_ADD_PROXY_TYPE:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( g_hWnd_add_proxy_type, CB_GETCURSEL, 0, 0 );

						if ( index != CB_ERR )
						{
							_SendMessageW( g_hWnd_add_hostname_socks, WM_SETTEXT, 0, ( LPARAM )L"localhost" );
							_SendMessageW( g_hWnd_add_ip_address_socks, IPM_SETADDRESS, 0, 2130706433 );	// 127.0.0.1

							char value[ 6 ];
							_memzero( value, sizeof( char ) * 6 );
							__snprintf( value, 6, "%hu", ( index == 1 ? 80 : ( index == 2 ? 443 : 1080 ) ) );
							_SendMessageA( g_hWnd_add_port_socks, WM_SETTEXT, 0, ( LPARAM )value );

							ShowHideAddProxyWindows( index );
						}
					}
				}
				break;

				case BTN_ADD_AUTHENTICATION_SOCKS:
				{
					BOOL enable = ( _SendMessageW( g_hWnd_chk_add_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? TRUE : FALSE );

					_EnableWindow( g_hWnd_static_add_auth_username_socks, enable );
					_EnableWindow( g_hWnd_add_auth_username_socks, enable );
					_EnableWindow( g_hWnd_static_add_auth_password_socks, enable );
					_EnableWindow( g_hWnd_add_auth_password_socks, enable );
				}
				break;

				case EDIT_ADD_PORT_SOCKS:
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
					}
				}
				break;

				case BTN_ADD_TYPE_HOST_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_add_ip_address_socks, SW_HIDE );
						_ShowWindow( g_hWnd_add_hostname_socks, SW_SHOW );
					}
				}
				break;

				case BTN_ADD_TYPE_IP_ADDRESS_SOCKS:
				{
					if ( _SendMessageW( g_hWnd_chk_add_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
					{
						_ShowWindow( g_hWnd_add_hostname_socks, SW_HIDE );
						_ShowWindow( g_hWnd_add_ip_address_socks, SW_SHOW );
					}
				}
				break;

				case IDOK:
				case BTN_DOWNLOAD:
				case BTN_ADD_DOWNLOAD:
				case BTN_VERIFY_DOWNLOAD:
				{
					unsigned int download_operations = ( _SendMessageW( g_hWnd_chk_simulate_download, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? DOWNLOAD_OPERATION_SIMULATE : DOWNLOAD_OPERATION_NONE );

					if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) && t_au_download_directory == NULL )
					{
						CMessageBoxW( hWnd, ST_V_You_must_supply_download_directory, PROGRAM_CAPTION, /*CMB_APPLMODAL |*/ CMB_ICONWARNING );

						_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( BTN_DOWNLOAD_DIRECTORY, 0 ), 0 );

						break;
					}

					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 )
					{
						ADD_INFO *ai = ( ADD_INFO * )GlobalAlloc( GPTR, sizeof( ADD_INFO ) );
						if ( ai != NULL )
						{
							// Category
							int cur_sel = ( int )_SendMessageW( g_hWnd_au_category, CB_GETCURSEL, 0, 0 );
							LRESULT ret = _SendMessageW( g_hWnd_au_category, CB_GETITEMDATA, cur_sel, 0 );
							if ( ret != CB_ERR )
							{
								CATEGORY_INFO_ *ci = ( CATEGORY_INFO_ * )ret;
								if ( ci != NULL )
								{
									ai->category = GlobalStrDupW( ci->category );
								}
							}

							// URLs
							wchar_t *edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_add, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							ai->urls = edit;

							// GENERAL TAB

							if ( !( download_operations & DOWNLOAD_OPERATION_SIMULATE ) )
							{
								ai->use_download_directory = ( _SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

								if ( ai->use_download_directory )
								{
									ai->download_directory = t_au_download_directory;

									int t_download_directory_length = lstrlenW( t_au_download_directory );
									t_au_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
									_wmemcpy_s( t_au_download_directory, MAX_PATH, ai->download_directory, t_download_directory_length );
									t_au_download_directory[ t_download_directory_length ] = 0;	// Sanity.
								}
							}

							char value[ 21 ];

							ai->use_parts = ( _SendMessageW( g_hWnd_chk_add_enable_download_parts, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

							if ( ai->use_parts )
							{
								_SendMessageA( g_hWnd_download_parts, WM_GETTEXT, 11, ( LPARAM )value );
								ai->parts = ( unsigned char )_strtoul( value, NULL, 10 );
							}

							ai->use_download_speed_limit = ( _SendMessageW( g_hWnd_chk_add_enable_speed_limit, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

							if ( ai->use_download_speed_limit )
							{
								_SendMessageA( g_hWnd_add_speed_limit, WM_GETTEXT, 21, ( LPARAM )value );
								ai->download_speed_limit = strtoull( value );
							}

							ai->ssl_version = ( char )_SendMessageW( g_hWnd_ssl_version, CB_GETCURSEL, 0, 0 );

							if ( LOWORD( wParam ) == BTN_ADD_DOWNLOAD || add_split_type == 1 )
							{
								download_operations |= DOWNLOAD_OPERATION_ADD_STOPPED;
							}
							else if ( LOWORD( wParam ) == BTN_VERIFY_DOWNLOAD || add_split_type == 2 )
							{
								download_operations |= DOWNLOAD_OPERATION_VERIFY;
							}

							ai->download_operations = download_operations;

							int utf8_length;

							// Username
							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_username, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length > 0 )
							{
								edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
								_SendMessageW( g_hWnd_edit_username, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

								utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
								ai->auth_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
								WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.username, utf8_length, NULL, NULL );

								GlobalFree( edit );
							}

							// Password
							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_password, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length > 0 )
							{
								edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
								_SendMessageW( g_hWnd_edit_password, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

								utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
								ai->auth_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
								WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->auth_info.password, utf8_length, NULL, NULL );

								GlobalFree( edit );
							}

							//

							// COMMENTS, COOKIES, HEADERS, DATA

							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_comments, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length > 0 )
							{
								ai->comments = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
								_SendMessageW( g_hWnd_edit_comments, WM_GETTEXT, edit_length + 1, ( LPARAM )ai->comments );
							}

							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_cookies, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length > 0 )
							{
								edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
								_SendMessageW( g_hWnd_edit_cookies, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

								utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
								ai->utf8_cookies = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
								WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_cookies, utf8_length, NULL, NULL );

								GlobalFree( edit );
							}

							// Must be at least 2 characters long. "a:" is a valid header name and value.
							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_headers, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length >= 2 )
							{
								edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 + 2 ) );	// Add 2 for \r\n
								_SendMessageW( g_hWnd_edit_headers, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

								wchar_t *edit_start = edit;

								// Skip newlines that might appear before the field name.
								while ( *edit_start == L'\r' || *edit_start == L'\n' )
								{
									--edit_length;
									++edit_start;
								}

								// Make sure the header has a colon somewhere in it and not at the very beginning.
								if ( edit_length >= 2 && *edit_start != L':' && _StrChrW( edit_start + 1, L':' ) != NULL )
								{
									// Make sure the header ends with a \r\n.
									if ( edit_start[ edit_length - 2 ] != '\r' && edit_start[ edit_length - 1 ] != '\n' )
									{
										edit_start[ edit_length ] = '\r';
										edit_start[ edit_length + 1 ] = '\n';
										edit_start[ edit_length + 2 ] = 0;	// Sanity.
									}

									utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit_start, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
									ai->utf8_headers = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
									WideCharToMultiByte( CP_UTF8, 0, edit_start, -1, ai->utf8_headers, utf8_length, NULL, NULL );
								}

								GlobalFree( edit );
							}

							ai->method = ( _SendMessageW( g_hWnd_chk_send_data, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? METHOD_POST : METHOD_GET );

							edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_data, WM_GETTEXTLENGTH, 0, 0 );
							if ( edit_length > 0 )
							{
								edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
								_SendMessageW( g_hWnd_edit_data, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

								utf8_length = WideCharToMultiByte( CP_UTF8, 0, edit, -1, NULL, 0, NULL, NULL );	// Size includes NULL character.
								ai->utf8_data = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * utf8_length ); // Size includes the NULL character.
								WideCharToMultiByte( CP_UTF8, 0, edit, -1, ai->utf8_data, utf8_length, NULL, NULL );

								GlobalFree( edit );
							}

							// PROXY

							ai->proxy_info.type = ( unsigned char )_SendMessageW( g_hWnd_add_proxy_type, CB_GETCURSEL, 0, 0 );

							if ( ai->proxy_info.type != 0 )
							{
								unsigned int hostname_length;

								ai->proxy_info.address_type = ( _SendMessageW( g_hWnd_chk_add_type_ip_address_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? 1 : 0 );

								if ( ai->proxy_info.address_type == 0 )
								{
									hostname_length = ( unsigned int )_SendMessageW( g_hWnd_add_hostname_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
									ai->proxy_info.hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * hostname_length );
									_SendMessageW( g_hWnd_add_hostname_socks, WM_GETTEXT, hostname_length, ( LPARAM )ai->proxy_info.hostname );

									if ( normaliz_state == NORMALIZ_STATE_RUNNING )
									{
										int punycode_length = _IdnToAscii( 0, ai->proxy_info.hostname, hostname_length, NULL, 0 );

										if ( punycode_length > ( int )hostname_length )
										{
											ai->proxy_info.punycode_hostname = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * punycode_length );
											_IdnToAscii( 0, ai->proxy_info.hostname, hostname_length, ai->proxy_info.punycode_hostname, punycode_length );
										}
									}
								}
								else
								{
									_SendMessageW( g_hWnd_add_ip_address_socks, IPM_GETADDRESS, 0, ( LPARAM )&ai->proxy_info.ip_address );
								}

								_SendMessageA( g_hWnd_add_port_socks, WM_GETTEXT, 6, ( LPARAM )value );
								ai->proxy_info.port = ( unsigned short )_strtoul( value, NULL, 10 );

								unsigned int auth_length;

								if ( ai->proxy_info.type == 1 || ai->proxy_info.type == 2 )	// HTTP and HTTPS
								{
									auth_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add_proxy_auth_username, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
									ai->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
									_SendMessageW( g_hWnd_edit_add_proxy_auth_username, WM_GETTEXT, auth_length, ( LPARAM )ai->proxy_info.w_username );

									auth_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add_proxy_auth_password, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
									ai->proxy_info.w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
									_SendMessageW( g_hWnd_edit_add_proxy_auth_password, WM_GETTEXT, auth_length, ( LPARAM )ai->proxy_info.w_password );
								}
								else if ( ai->proxy_info.type == 3 )	// SOCKS v4
								{
									ai->proxy_info.resolve_domain_names = ( _SendMessageW( g_hWnd_chk_add_resolve_domain_names_v4a, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

									auth_length = ( unsigned int )_SendMessageW( g_hWnd_add_auth_ident_username_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
									ai->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
									_SendMessageW( g_hWnd_add_auth_ident_username_socks, WM_GETTEXT, auth_length, ( LPARAM )ai->proxy_info.w_username );
								}
								else if ( ai->proxy_info.type == 4 )	// SOCKS v5
								{
									ai->proxy_info.resolve_domain_names = ( _SendMessageW( g_hWnd_chk_add_resolve_domain_names, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

									ai->proxy_info.use_authentication = ( _SendMessageW( g_hWnd_chk_add_use_authentication_socks, BM_GETCHECK, 0, 0 ) == BST_CHECKED ? true : false );

									if ( ai->proxy_info.use_authentication )
									{
										auth_length = ( unsigned int )_SendMessageW( g_hWnd_add_auth_username_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
										ai->proxy_info.w_username = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
										_SendMessageW( g_hWnd_add_auth_username_socks, WM_GETTEXT, auth_length, ( LPARAM )ai->proxy_info.w_username );

										auth_length = ( unsigned int )_SendMessageW( g_hWnd_add_auth_password_socks, WM_GETTEXTLENGTH, 0, 0 ) + 1;	// Include the NULL terminator.
										ai->proxy_info.w_password = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * auth_length );
										_SendMessageW( g_hWnd_add_auth_password_socks, WM_GETTEXT, auth_length, ( LPARAM )ai->proxy_info.w_password );
									}
								}

								if ( ai->proxy_info.w_username != NULL )
								{
									auth_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, NULL, 0, NULL, NULL );
									ai->proxy_info.username = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_length ); // Size includes the null character.
									WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_username, -1, ai->proxy_info.username, auth_length, NULL, NULL );
								}

								if ( ai->proxy_info.w_password != NULL )
								{
									auth_length = WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, NULL, 0, NULL, NULL );
									ai->proxy_info.password = ( char * )GlobalAlloc( GMEM_FIXED, sizeof( char ) * auth_length ); // Size includes the null character.
									WideCharToMultiByte( CP_UTF8, 0, ai->proxy_info.w_password, -1, ai->proxy_info.password, auth_length, NULL, NULL );
								}
							}

							//

							// ai is freed in AddURL.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, AddURL, ( void * )ai, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								FreeAddInfo( &ai );
							}
						}
					}

					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case BTN_ADD_CANCEL:
				{
					_SendMessageW( hWnd, WM_CLOSE, 0, 0 );
				}
				break;

				case MENU_ADD_SPLIT_DOWNLOAD:
				{
					add_split_type = 0;

					_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Download );
				}
				break;

				case MENU_ADD_SPLIT_ADD:
				{
					add_split_type = 1;

					_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Add );
				}
				break;

				case MENU_ADD_SPLIT_VERIFY:
				{
					add_split_type = 2;

					_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Verify );
				}
				break;

				case EDIT_ADD_URLS:
				{
					if ( HIWORD( wParam ) == EN_UPDATE )
					{
						BOOL enable = ( _SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 ) > 0 ? TRUE : FALSE );
						_EnableWindow( g_hWnd_btn_download, enable );
						if ( !use_add_split )
						{
							_EnableWindow( g_hWnd_btn_add_download, enable );
							_EnableWindow( g_hWnd_btn_verify_download, enable );
						}
					}
				}
				break;

				case BTN_ADVANCED:
				{
					g_show_advanced = !g_show_advanced;

					int sw_type = ( g_show_advanced ? SW_SHOW : SW_HIDE );

					// Adjust the window height.
					RECT rc;
					_GetWindowRect( hWnd, &rc );

					if ( g_show_advanced )
					{
						_SetWindowPos( hWnd, NULL, 0, 0, rc.right - rc.left, _SCALE_AU_( ( MIN_ADVANCED_HEIGHT - MIN_SIMPLE_HEIGHT ) ) + ( rc.bottom - rc.top ), SWP_NOMOVE );
					}
					else
					{
						if ( _IsZoomed( hWnd ) == FALSE )
						{
							_SetWindowPos( hWnd, NULL, 0, 0, rc.right - rc.left, ( rc.bottom - rc.top ) - _SCALE_AU_( ( MIN_ADVANCED_HEIGHT - MIN_SIMPLE_HEIGHT ) ), SWP_NOMOVE );
						}
					}

					_ShowWindow( g_hWnd_advanced_add_tab, sw_type );
					ShowHideAddTabs( sw_type );

					// Force a resize of our controls.
					_SendMessageW( hWnd, WM_SIZE, 0, 0 );
				}
				break;

				case CB_REGEX_FILTER_PRESET:
				{
					if ( HIWORD( wParam ) == CBN_SELCHANGE )
					{
						int index = ( int )_SendMessageW( ( HWND )lParam, CB_GETCURSEL, 0, 0 );

						wchar_t *filter = NULL;
						switch ( index )
						{
							case 1: { filter = L"(?i)(^((http|ftpe?)s?|sftp):\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(jp(e?g|e)|gif|png|bmp|tiff?|dib|ico)(\\?|#|$))"; } break;
							case 2: { filter = L"(?i)(^((http|ftpe?)s?|sftp):\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(mp3|wave?|flac?|ogg|m4a|wma|aac|midi?|ape|shn|wv|aiff?|oga)(\\?|#|$))"; } break;
							case 3: { filter = L"(?i)(^((http|ftpe?)s?|sftp):\\/\\/[^\\/\\s]+\\/[^\\?#\\s]+\\.(avi|mp[124]|m4v|mp(e?g|e)|mkv|webm|wmv|3gp|ogm|ogv|flv|vob)(\\?|#|$))"; } break;
						}

						_SendMessageW( g_hWnd_regex_filter, WM_SETTEXT, 0, ( LPARAM )filter );
					}
				}
				break;

				case BTN_APPLY_FILTER:
				{
					unsigned int edit_length = ( unsigned int )_SendMessageW( g_hWnd_edit_add, WM_GETTEXTLENGTH, 0, 0 );
					unsigned int regex_filter_length = ( unsigned int )_SendMessageW( g_hWnd_regex_filter, WM_GETTEXTLENGTH, 0, 0 );

					// http://a.b
					if ( edit_length >= 10 && regex_filter_length > 0 )
					{
						FILTER_INFO *fi = ( FILTER_INFO * )GlobalAlloc( GMEM_FIXED, sizeof( FILTER_INFO ) );
						if ( fi != NULL )
						{
							// URLs
							wchar_t *edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( edit_length + 1 ) );
							_SendMessageW( g_hWnd_edit_add, WM_GETTEXT, edit_length + 1, ( LPARAM )edit );

							fi->text = edit;

							// Filter
							edit = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * ( regex_filter_length + 1 ) );
							_SendMessageW( g_hWnd_regex_filter, WM_GETTEXT, regex_filter_length + 1, ( LPARAM )edit );

							fi->filter = edit;

							// fi is freed in filter_urls.
							HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, filter_urls, ( void * )fi, 0, NULL );
							if ( thread != NULL )
							{
								CloseHandle( thread );
							}
							else
							{
								GlobalFree( fi->filter );
								GlobalFree( fi->text );
								GlobalFree( fi );
							}
						}
					}
				}
				break;
			}

			return 0;
		}
		break;

		case WM_NOTIFY:
		{
			// Get our listview codes.
			switch ( ( ( LPNMHDR )lParam )->code )
			{
				case TCN_SELCHANGING:		// The tab that's about to lose focus
				{
					//NMHDR *nmhdr = ( NMHDR * )lParam;

					ShowHideAddTabs( SW_HIDE );
				}
				break;

				case TCN_SELCHANGE:			// The tab that gains focus
				{
					NMHDR *nmhdr = ( NMHDR * )lParam;

					HWND hWnd_focused = _GetFocus();
					if ( hWnd_focused != hWnd && hWnd_focused != nmhdr->hwndFrom )
					{
						_SetFocus( _GetWindow( nmhdr->hwndFrom, GW_CHILD ) );
					}

					ShowHideAddTabs( SW_SHOW );
				}
				break;

				case BCN_DROPDOWN:
				{
					NMBCDROPDOWN *nmbcddd = ( NMBCDROPDOWN * )lParam;

					RECT rc;
					_GetWindowRect( nmbcddd->hdr.hwndFrom, &rc );

					_TrackPopupMenu( g_hMenu_add_split, 0, rc.left, rc.bottom, 0, hWnd, NULL );
				}
				break;
			}

			return FALSE;
		}
		break;

		case WM_UPDATE_CATEGORY:
		{
			int sel_index = 0;

			if ( t_au_category != NULL )
			{
				sel_index = ( int )_SendMessageW( g_hWnd_au_category, CB_FINDSTRINGEXACT, 0, ( LPARAM )t_au_category );
				if ( sel_index < 0 )
				{
					sel_index = 0;
				}
			}

			_SendMessageW( g_hWnd_au_category, CB_SETCURSEL, sel_index, 0 );

			_SendMessageW( hWnd, WM_COMMAND, MAKEWPARAM( CB_ADD_CATEGORY/*_GetDlgCtrlID( g_hWnd_au_category )*/, CBN_SELCHANGE ), 0 );

			return TRUE;
		}
		break;

		case WM_PROPAGATE:
		{
			if ( wParam == -1 )
			{
				CL_ARGS *cla = ( CL_ARGS * )lParam;

				if ( cla != NULL )
				{
					if ( t_au_category != NULL )
					{
						GlobalFree( t_au_category );
						t_au_category = NULL;
					}

					if ( cla->category != NULL )
					{
						t_au_category = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );

						_wmemcpy_s( t_au_category, MAX_PATH, cla->category, cla->category_length );
						t_au_category[ cla->category_length ] = 0;	// Sanity.
					}

					if ( cla->download_directory != NULL && cla->download_directory_length < MAX_PATH )
					{
						if ( t_au_download_directory == NULL )
						{
							t_au_download_directory = ( wchar_t * )GlobalAlloc( GMEM_FIXED, sizeof( wchar_t ) * MAX_PATH );
						}

						_wmemcpy_s( t_au_download_directory, MAX_PATH, cla->download_directory, cla->download_directory_length );
						t_au_download_directory[ cla->download_directory_length ] = 0;	// Sanity.

						_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )t_au_download_directory );
					}

					if ( cla->use_download_directory )
					{
						_SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_SETCHECK, BST_CHECKED, 0 );
						_EnableWindow( g_hWnd_download_directory, TRUE );
						_EnableWindow( g_hWnd_btn_download_directory, TRUE );
					}

					if ( cla->urls != NULL )
					{
						_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )cla->urls );
					}

					bool got_clipboard_data = false;
					if ( cla->use_clipboard )
					{
						got_clipboard_data = DisplayClipboardData();
					}

					if ( cla->comments != NULL )
					{
						_SendMessageW( g_hWnd_edit_comments, EM_REPLACESEL, 0, ( LPARAM )cla->comments );
					}

					if ( cla->cookies != NULL )
					{
						_SendMessageW( g_hWnd_edit_cookies, EM_REPLACESEL, 0, ( LPARAM )cla->cookies );
					}

					if ( cla->headers != NULL )
					{
						_SendMessageW( g_hWnd_edit_headers, EM_REPLACESEL, 0, ( LPARAM )cla->headers );
					}

					if ( cla->data != NULL )
					{
						_SendMessageW( g_hWnd_chk_send_data, BM_SETCHECK, BST_CHECKED, 0 );
						_SendMessageW( g_hWnd_edit_data, EM_REPLACESEL, 0, ( LPARAM )cla->data );
						_EnableWindow( g_hWnd_edit_data, TRUE );
					}

					if ( cla->parts > 0 )
					{
						_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cla->parts );
					}

					if ( cla->use_parts )
					{
						_SendMessageW( g_hWnd_chk_add_enable_download_parts, BM_SETCHECK, BST_CHECKED, 0 );
						_EnableWindow( g_hWnd_download_parts, TRUE );
						_EnableWindow( g_hWnd_ud_download_parts, TRUE );
					}

					char value[ 21 ];
					_memzero( value, sizeof( char ) * 21 );
					__snprintf( value, 21, "%I64u", cla->download_speed_limit );
					_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

					if ( cla->use_download_speed_limit )
					{
						_SendMessageW( g_hWnd_chk_add_enable_speed_limit, BM_SETCHECK, BST_CHECKED, 0 );
						_EnableWindow( g_hWnd_add_speed_limit, TRUE );
					}

					if ( cla->ssl_version >= 0 )
					{
						_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, cla->ssl_version, 0 );
					}

					if ( cla->username != NULL )
					{
						_SendMessageW( g_hWnd_edit_username, WM_SETTEXT, 0, ( LPARAM )cla->username );
					}

					if ( cla->password != NULL )
					{
						_SendMessageW( g_hWnd_edit_password, WM_SETTEXT, 0, ( LPARAM )cla->password );
					}

					if ( cla->proxy_type != 0 )
					{
						_SendMessageW( g_hWnd_add_proxy_type, CB_SETCURSEL, cla->proxy_type, 0 );

						if ( cla->proxy_hostname != NULL )
						{
							_SendMessageW( g_hWnd_add_hostname_socks, WM_SETTEXT, 0, ( LPARAM )cla->proxy_hostname );

							_SendMessageW( g_hWnd_chk_add_type_ip_address_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );
						}
						else
						{
							_SendMessageW( g_hWnd_add_ip_address_socks, IPM_SETADDRESS, 0, cla->proxy_ip_address );

							_SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
							_SendMessageW( g_hWnd_chk_add_type_ip_address_socks, BM_SETCHECK, BST_CHECKED, 0 );
						}

						if ( cla->proxy_port == 0 )
						{
							cla->proxy_port = ( cla->proxy_type == 1 ? 80 : ( cla->proxy_type == 2 ? 443 : 1080 ) );
						}

						__snprintf( value, 6, "%hu", cla->proxy_port );
						_SendMessageA( g_hWnd_add_port_socks, WM_SETTEXT, 0, ( LPARAM )value );

						if ( cla->proxy_type == 1 || cla->proxy_type == 2 )
						{
							if ( cla->proxy_username != NULL )
							{
								_SendMessageW( g_hWnd_edit_add_proxy_auth_username, WM_SETTEXT, 0, ( LPARAM )cla->proxy_username );
							}

							if ( cla->proxy_password != NULL )
							{
								_SendMessageW( g_hWnd_edit_add_proxy_auth_password, WM_SETTEXT, 0, ( LPARAM )cla->proxy_password );
							}
						}
						else if ( cla->proxy_type == 3 )	// SOCKSv4
						{
							_SendMessageW( g_hWnd_chk_add_resolve_domain_names_v4a, BM_SETCHECK, ( cla->proxy_resolve_domain_names ? BST_CHECKED : BST_UNCHECKED ), 0 );

							if ( cla->proxy_username != NULL )
							{
								_SendMessageW( g_hWnd_add_auth_ident_username_socks, WM_SETTEXT, 0, ( LPARAM )cla->proxy_username );
							}
						}
						else if ( cla->proxy_type == 4 )	// SOCKSv5
						{
							_SendMessageW( g_hWnd_chk_add_resolve_domain_names, BM_SETCHECK, ( cla->proxy_resolve_domain_names ? BST_CHECKED : BST_UNCHECKED ), 0 );

							if ( cla->proxy_username != NULL || cla->proxy_password != NULL )
							{
								_SendMessageW( g_hWnd_chk_add_use_authentication_socks, BM_SETCHECK, BST_CHECKED, 0 );

								_SendMessageW( g_hWnd_add_auth_username_socks, WM_SETTEXT, 0, ( LPARAM )cla->proxy_username );
								_SendMessageW( g_hWnd_add_auth_password_socks, WM_SETTEXT, 0, ( LPARAM )cla->proxy_password );
							}
						}
					}

					if ( cla->download_operations & DOWNLOAD_OPERATION_SIMULATE )
					{
						_SendMessageW( g_hWnd_chk_simulate_download, BM_SETCHECK, BST_CHECKED, 0 );
						_EnableWindow( g_hWnd_download_directory, FALSE );
						_EnableWindow( g_hWnd_btn_download_directory, FALSE );
					}

					HWND hWnd_focus;

					if ( use_add_split )
					{
						if ( cla->download_operations & DOWNLOAD_OPERATION_VERIFY )
						{
							add_split_type = 2;

							_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Verify );
						}
						else if ( cla->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
						{
							add_split_type = 1;

							_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Add );
						}
						else
						{
							add_split_type = 0;

							_SendMessageW( g_hWnd_btn_download, WM_SETTEXT, 0, ( LPARAM )ST_V_Download );
						}

						hWnd_focus = g_hWnd_btn_download;
					}
					else
					{
						// Adjust the default push button since SetFocus doesn't work.
						if ( cla->download_operations & DOWNLOAD_OPERATION_VERIFY )
						{
							_SetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE ) | BS_DEFPUSHBUTTON );

							hWnd_focus = g_hWnd_btn_verify_download;
						}
						else if ( cla->download_operations & DOWNLOAD_OPERATION_ADD_STOPPED )
						{
							_SetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE ) | BS_DEFPUSHBUTTON );

							hWnd_focus = g_hWnd_btn_add_download;
						}
						else
						{
							_SetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
							_SetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE ) | BS_DEFPUSHBUTTON );

							hWnd_focus = g_hWnd_btn_download;
						}
					}

					// http://a.b + \r\n
					_SetFocus( ( ( cla->urls_length >= 12 || got_clipboard_data ) ? hWnd_focus : g_hWnd_edit_add ) );
				}
			}
			else if ( wParam != 0 )
			{
				char length = 0;

				if ( wParam == CF_UNICODETEXT || wParam == CF_HTML )
				{
					wchar_t *data = ( wchar_t * )lParam;
					if ( data != NULL ) { for ( ; length < 10 && *data != NULL; ++length, ++data ); }

					_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
				}
				else// if ( wParam == CF_TEXT )
				{
					char *data = ( char * )lParam;
					if ( data != NULL ) { for ( ; length < 10 && *data != NULL; ++length, ++data ); }

					_SendMessageA( g_hWnd_edit_add, EM_REPLACESEL, 0, lParam );
				}

				// Append a newline after our dropped text.
				_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, 0, ( LPARAM )L"\r\n" );

				// http://a.b
				_SetFocus( ( length == 10 ? g_hWnd_btn_download : g_hWnd_edit_add ) );
			}
			else
			{
				bool got_clipboard_data = false;

				if ( _IsWindowVisible( hWnd ) == FALSE )
				{
					got_clipboard_data = DisplayClipboardData();
				}

				_SetFocus( ( got_clipboard_data ? g_hWnd_btn_download : g_hWnd_edit_add ) );
			}

			if ( _IsWindowVisible( hWnd ) == FALSE )
			{
				HANDLE thread = ( HANDLE )_CreateThread( NULL, 0, load_window_category_list, ( void * )g_hWnd_au_category, 0, NULL );
				if ( thread != NULL )
				{
					CloseHandle( thread );
				}
			}
			else if ( wParam == -1 )
			{
				_SendMessageW( hWnd, WM_UPDATE_CATEGORY, 0, NULL );
			}

			_ShowWindow( hWnd, SW_SHOWNORMAL );
			_SetForegroundWindow( hWnd );

			return TRUE;
		}
		break;

		case WM_FILTER_TEXT:
		{
			_SendMessageW( g_hWnd_edit_add, EM_SETSEL, 0, -1 );
			_SendMessageW( g_hWnd_edit_add, EM_REPLACESEL, TRUE, lParam );

			_SetFocus( g_hWnd_edit_add );

			GlobalFree( ( HGLOBAL )lParam );

			return TRUE;
		}
		break;

		case WM_ACTIVATE:
		{
			// 0 = inactive, > 0 = active
			g_hWnd_active = ( wParam == 0 ? NULL : hWnd );

			return FALSE;
		}
		break;

		case WM_CLOSE:
		{
			_ShowWindow( hWnd, SW_HIDE );

			_SendMessageW( g_hWnd_edit_add, WM_SETTEXT, 0, NULL );

			// Reset to default.

			_EnableWindow( g_hWnd_btn_download, FALSE );
			if ( !use_add_split )
			{
				_EnableWindow( g_hWnd_btn_add_download, FALSE );
				_EnableWindow( g_hWnd_btn_verify_download, FALSE );

				_SetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_add_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
				_SetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_verify_download, GWL_STYLE ) & ~BS_DEFPUSHBUTTON );
				_SetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE, _GetWindowLongPtrW( g_hWnd_btn_download, GWL_STYLE ) | BS_DEFPUSHBUTTON );
			}

			_SendMessageW( g_hWnd_btn_download, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
			_SendMessageW( g_hWnd_cancel, BM_SETSTYLE, 0, TRUE );
			_SendMessageW( g_hWnd_btn_download_directory, BM_SETSTYLE, 0, TRUE );
			_SendMessageW( g_hWnd_btn_apply_filter, BM_SETSTYLE, 0, TRUE );

			_SendMessageW( g_hWnd_au_category, CB_SETCURSEL, 0, 0 );
			if ( t_au_category != NULL )
			{
				GlobalFree( t_au_category );
				t_au_category = NULL;
			}

			// Let's retain the last directory.
			//_SendMessageW( g_hWnd_download_directory, WM_SETTEXT, 0, ( LPARAM )cfg_default_download_directory );

			_SendMessageW( g_hWnd_chk_add_enable_download_directory, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_chk_add_enable_download_directory, TRUE );
			_EnableWindow( g_hWnd_download_directory, FALSE );
			_EnableWindow( g_hWnd_btn_download_directory, FALSE );

			_SendMessageW( g_hWnd_chk_add_enable_download_parts, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_download_parts, FALSE );
			_EnableWindow( g_hWnd_ud_download_parts, FALSE );
			_SendMessageW( g_hWnd_ud_download_parts, UDM_SETPOS, 0, cfg_default_download_parts );

			_SendMessageW( g_hWnd_chk_add_enable_speed_limit, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_add_speed_limit, FALSE );

			char value[ 21 ];
			_memzero( value, sizeof( char ) * 21 );
			__snprintf( value, 21, "%I64u", cfg_default_speed_limit );
			_SendMessageA( g_hWnd_add_speed_limit, WM_SETTEXT, 0, ( LPARAM )value );

			_SendMessageW( g_hWnd_ssl_version, CB_SETCURSEL, 0, 0 );

			_SendMessageW( g_hWnd_edit_username, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_password, WM_SETTEXT, 0, NULL );

			//

			_SendMessageW( g_hWnd_edit_comments, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_cookies, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_headers, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_data, WM_SETTEXT, 0, NULL );

			_SendMessageW( g_hWnd_chk_send_data, BM_SETCHECK, BST_UNCHECKED, 0 );
			_EnableWindow( g_hWnd_edit_data, FALSE );

			//

			_SendMessageW( g_hWnd_add_proxy_type, CB_SETCURSEL, 0, 0 );

			_SendMessageW( g_hWnd_chk_add_type_ip_address_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
			_SendMessageW( g_hWnd_chk_add_type_hostname_socks, BM_SETCHECK, BST_CHECKED, 0 );

			_SendMessageW( g_hWnd_add_hostname_socks, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_add_ip_address_socks, IPM_SETADDRESS, 0, NULL );
			_SendMessageW( g_hWnd_add_port_socks, WM_SETTEXT, 0, NULL );

			_SendMessageW( g_hWnd_edit_add_proxy_auth_username, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_edit_add_proxy_auth_password, WM_SETTEXT, 0, NULL );

			_SendMessageW( g_hWnd_add_auth_ident_username_socks, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_chk_add_resolve_domain_names_v4a, BM_SETCHECK, BST_UNCHECKED, 0 );

			_SendMessageW( g_hWnd_chk_add_use_authentication_socks, BM_SETCHECK, BST_UNCHECKED, 0 );
			_SendMessageW( g_hWnd_add_auth_username_socks, WM_SETTEXT, 0, NULL );
			_SendMessageW( g_hWnd_add_auth_password_socks, WM_SETTEXT, 0, NULL );
			_EnableWindow( g_hWnd_static_add_auth_username_socks, FALSE );
			_EnableWindow( g_hWnd_add_auth_username_socks, FALSE );
			_EnableWindow( g_hWnd_static_add_auth_password_socks, FALSE );
			_EnableWindow( g_hWnd_add_auth_password_socks, FALSE );
			_SendMessageW( g_hWnd_chk_add_resolve_domain_names, BM_SETCHECK, BST_UNCHECKED, 0 );

			ShowHideAddProxyWindows( 0 );

			//

			_SendMessageW( g_hWnd_chk_simulate_download, BM_SETCHECK, BST_UNCHECKED, 0 );

			_SendMessageW( g_hWnd_advanced_add_tab, TCM_SETCURFOCUS, 0, 0 );	// Causes TCN_SELCHANGE to get called.
			if ( !g_show_advanced )
			{
				ShowHideAddTabs( SW_HIDE );	// Reverts the SW_SHOW in TCN_SELCHANGE if the advanced options are hidden.
			}

			//

			return 0;
		}
		break;

		case WM_DESTROY:
		{
			// Delete our font.
			_DeleteObject( hFont_add_urls );

			_DeleteObject( hFont_copy_add_proxy );
			hFont_copy_add_proxy = NULL;

			if ( g_add_tab_brush != NULL )
			{
				_DeleteObject( g_add_tab_brush );
				g_add_tab_brush = NULL;
			}

			if ( use_drag_and_drop_add )
			{
				UnregisterDropWindow( g_hWnd_edit_add, Add_DropTarget );

				_OleUninitialize();
			}

			if ( t_au_category != NULL )
			{
				GlobalFree( t_au_category );
				t_au_category = NULL;
			}

			if ( t_au_download_directory != NULL )
			{
				GlobalFree( t_au_download_directory );
				t_au_download_directory = NULL;
			}

			_DestroyMenu( g_hMenu_add_split );

			g_hWnd_add_urls = NULL;

			g_show_advanced = false;

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
